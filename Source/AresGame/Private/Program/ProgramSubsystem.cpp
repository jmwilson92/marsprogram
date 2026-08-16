#include "Program/ProgramSubsystem.h"

#include "Misc/Paths.h"

#include "AresCore/ProgramSystems.h"
#include "AresCore/SimClock.h"

namespace
{
/** Days of program time per real second, by rate. */
double DaysPerSecondFor(EProgramSpeed Speed)
{
	switch (Speed)
	{
	case EProgramSpeed::X1:  return 1.0;
	case EProgramSpeed::X3:  return 3.0;
	case EProgramSpeed::X10: return 10.0;
	case EProgramSpeed::X30: return 30.0;
	case EProgramSpeed::Paused:
	default: return 0.0;
	}
}
} // namespace

void UProgramSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (!LoadContent())
	{
		// Deliberately fatal-ish in logging terms: a program running on
		// half-loaded balance data is worse than one that refuses to start,
		// because it looks like it works.
		UE_LOG(LogTemp, Error, TEXT("ProgramSubsystem: content failed to load; program not started."));
		return;
	}

	StartNewProgram(0, TEXT("ADMINISTRATOR"));
}

void UProgramSubsystem::Deinitialize()
{
	OnProgramChanged.Clear();
	Super::Deinitialize();
}

bool UProgramSubsystem::LoadContent()
{
	// Data/ sits beside the .uproject and is staged as UFS (see DefaultGame.ini).
	const FString DataDir = FPaths::Combine(FPaths::ProjectDir(), TEXT("Data"));
	const std::string DataDirUtf8(TCHAR_TO_UTF8(*DataDir));

	std::string Error;
	if (!Ares::FAresData::LoadAll(DataDirUtf8, Data, Error))
	{
		UE_LOG(LogTemp, Error, TEXT("ProgramSubsystem: %s"), *FString(Error.c_str()));
		bDataLoaded = false;
		return false;
	}

	UE_LOG(LogTemp, Log,
		TEXT("ProgramSubsystem: loaded %d tech, %d cargo, %d contractors, %d sites."),
		static_cast<int32>(Data.Tech.size()),
		static_cast<int32>(Data.Cargo.size()),
		static_cast<int32>(Data.Contractors.size()),
		static_cast<int32>(Data.Sites.size()));

	bDataLoaded = true;
	return true;
}

void UProgramSubsystem::StartNewProgram(int32 Seed, const FString& Difficulty)
{
	if (!bDataLoaded)
	{
		return;
	}

	State = Ares::MakeProgramState(Data, static_cast<uint32>(Seed),
		std::string(TCHAR_TO_UTF8(*Difficulty)));

	Speed = EProgramSpeed::Paused;
	DayAccumulator = 0.0;

	UE_LOG(LogTemp, Log, TEXT("ProgramSubsystem: new program, seed %d, %s, $%.1fB/yr, support %.0f."),
		Seed, *Difficulty, State.Budget.AnnualUsd / 1e9, State.Politics.Support);

	OnProgramChanged.Broadcast();
}

void UProgramSubsystem::SetSpeed(EProgramSpeed InSpeed)
{
	if (Speed == InSpeed)
	{
		return;
	}
	Speed = InSpeed;
	// Dropping the fraction on a rate change stops a pause/resume from
	// delivering a surprise partial day.
	DayAccumulator = 0.0;
	OnProgramChanged.Broadcast();
}

float UProgramSubsystem::GetDaysPerSecond() const
{
	return static_cast<float>(DaysPerSecondFor(Speed));
}

void UProgramSubsystem::Tick(float DeltaTime)
{
	if (!bDataLoaded || Speed == EProgramSpeed::Paused || State.Politics.bCancelled)
	{
		return;
	}

	DayAccumulator += DaysPerSecondFor(Speed) * static_cast<double>(DeltaTime);
	if (DayAccumulator < 1.0)
	{
		return;
	}

	const int32 Whole = FMath::Min(static_cast<int32>(DayAccumulator), MaxDaysPerFrame);
	DayAccumulator -= static_cast<double>(Whole);

	const int32 Ran = Ares::TickProgramDays(State, Data, Whole);

	if (State.Politics.bCancelled)
	{
		// Nothing to watch after the program ends; stopping the clock is what a
		// real cancellation feels like.
		Speed = EProgramSpeed::Paused;
		DayAccumulator = 0.0;
		UE_LOG(LogTemp, Warning, TEXT("ProgramSubsystem: PROGRAM %s on day %.0f."),
			*FString(State.Politics.CancellationReason.c_str()), State.Clock.EarthDay);
	}

	if (Ran > 0)
	{
		OnProgramChanged.Broadcast();
	}
}

TStatId UProgramSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UProgramSubsystem, STATGROUP_Tickables);
}

int32 UProgramSubsystem::SkipDays(int32 Days)
{
	if (!bDataLoaded || Days <= 0)
	{
		return 0;
	}

	const int32 Ran = Ares::TickProgramDays(State, Data, Days);
	if (State.Politics.bCancelled)
	{
		Speed = EProgramSpeed::Paused;
	}
	if (Ran > 0)
	{
		OnProgramChanged.Broadcast();
	}
	return Ran;
}

int32 UProgramSubsystem::SkipToNextWindow(int32 MaxDays)
{
	if (!bDataLoaded)
	{
		return 0;
	}

	// Round up: landing exactly on the boundary should open the window rather
	// than stop one day short of it.
	const int32 Days = FMath::Clamp(
		FMath::CeilToInt(static_cast<float>(State.Clock.DaysToWindow)), 1, MaxDays);
	return SkipDays(Days);
}

/* ------------------------------------------------------------------ readers */

float UProgramSubsystem::GetEarthDay() const { return static_cast<float>(State.Clock.EarthDay); }

FString UProgramSubsystem::GetEarthDateIso() const
{
	return FString(Ares::FormatEarthDate(State.Clock.EarthDay).c_str());
}

float UProgramSubsystem::GetProgramYear() const
{
	return static_cast<float>(Ares::ProgramYear(State.Clock.EarthDay));
}

float UProgramSubsystem::GetBudgetRemainingUsd() const
{
	return static_cast<float>(State.Budget.RemainingUsd);
}

float UProgramSubsystem::GetBudgetAnnualUsd() const
{
	return static_cast<float>(State.Budget.AnnualUsd);
}

bool UProgramSubsystem::IsHoarding() const { return State.Budget.bHoarded; }

float UProgramSubsystem::GetSupport() const { return static_cast<float>(State.Politics.Support); }

int32 UProgramSubsystem::GetQuartersUnderThreshold() const
{
	return State.Politics.QuartersUnderThreshold;
}

bool UProgramSubsystem::IsCancelled() const { return State.Politics.bCancelled; }

FString UProgramSubsystem::GetCancellationReason() const
{
	return FString(State.Politics.CancellationReason.c_str());
}

float UProgramSubsystem::GetResearchPoints() const
{
	return static_cast<float>(State.Research.Points);
}

float UProgramSubsystem::GetDaysToNextWindow() const
{
	return static_cast<float>(State.Clock.DaysToWindow);
}

int32 UProgramSubsystem::GetCrewCount() const
{
	return static_cast<int32>(State.Crew.Roster.size());
}

int32 UProgramSubsystem::GetShipCount() const
{
	return static_cast<int32>(State.Fleet.Ships.size());
}
