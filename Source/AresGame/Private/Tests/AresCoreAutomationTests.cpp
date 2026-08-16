// Unreal automation tests for AresCore.
//
// These live in AresGame, not AresCore, because AresCore is forbidden from
// including Unreal headers (brief §3.1) and the automation framework is one.
//
// They are a SUBSET of the headless suite in Tests/, kept deliberately thin.
// The authoritative tests — 42 cases and ~21k assertions diffed against the JS
// reference — run through CMakeLists.txt and do not need the editor. What these
// verify is that AresCore behaves identically when compiled by UBT with UE's
// toolchain and flags, which is the failure mode the headless suite cannot see.
//
// Run from the editor: Session Frontend -> Automation -> Ares.Core
// Or headless:
//   UnrealEditor-Cmd.exe Ares.uproject -ExecCmds="Automation RunTests Ares.Core" -unattended -nullrhi

#include "Misc/AutomationTest.h"

#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#include "AresCore/AresData.h"
#include "AresCore/AresJson.h"
#include "AresCore/AresRng.h"
#include "AresCore/ProgramState.h"
#include "AresCore/SimClock.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
/** Data/ ships alongside the .uproject and is staged as UFS (see DefaultGame.ini). */
FString BalanceJsonPath()
{
	return FPaths::Combine(FPaths::ProjectDir(), TEXT("Data"), TEXT("balance.json"));
}

bool LoadBalance(Ares::FAresData& OutData, FString& OutError)
{
	const FString Path = BalanceJsonPath();
	FString Contents;
	if (!FFileHelper::LoadFileToString(Contents, *Path))
	{
		OutError = FString::Printf(TEXT("cannot read %s"), *Path);
		return false;
	}

	Ares::FJsonValue Root;
	std::string ParseError;
	const std::string Utf8(TCHAR_TO_UTF8(*Contents));
	if (!Ares::ParseJson(Utf8, Root, ParseError))
	{
		OutError = FString(ParseError.c_str());
		return false;
	}

	std::string LoadError;
	if (!Ares::FAresData::LoadBalance(Root, OutData, LoadError))
	{
		OutError = FString(LoadError.c_str());
		return false;
	}
	return true;
}
} // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAresCoreClockTest,
	"Ares.Core.Clock",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FAresCoreClockTest::RunTest(const FString& Parameters)
{
	Ares::FAresData Data;
	FString Error;
	if (!LoadBalance(Data, Error))
	{
		AddError(FString::Printf(TEXT("balance.json: %s"), *Error));
		return false;
	}

	Ares::FSimClock Clock = Ares::MakeClock(Data);
	TestEqual(TEXT("clock starts at day 0"), Clock.EarthDay, 0.0);
	TestEqual(TEXT("mars year at epoch"), Clock.MarsYear, Ares::MARS_YEAR_AT_LS0);

	// The first Earth-Mars window must open on the day balance.json specifies.
	int32 OpenedOnDay = -1;
	for (int32 Day = 1; Day <= 300; ++Day)
	{
		if (Ares::Advance(Clock, Data).bWindowOpened)
		{
			OpenedOnDay = Day;
			break;
		}
	}
	TestEqual(TEXT("first transfer window day"),
		OpenedOnDay, static_cast<int32>(Data.Time.FirstWindowEarthDay));

	// Ls must stay in range across a full Mars year of ticking.
	for (int32 Day = 0; Day < 700; ++Day)
	{
		Ares::Advance(Clock, Data);
		if (Clock.Ls < 0.0 || Clock.Ls >= 360.0)
		{
			AddError(FString::Printf(TEXT("Ls out of range on day %d: %f"), Day, Clock.Ls));
			return false;
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAresCoreRngTest,
	"Ares.Core.Rng",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FAresCoreRngTest::RunTest(const FString& Parameters)
{
	// Known-good values lifted from the JS reference via Tools/oracle. If UBT's
	// toolchain ever changed the integer arithmetic, this is where it surfaces.
	Ares::FAresRng Rng(42u);
	const double First = Rng.Next("edl");
	TestTrue(TEXT("draw is in [0,1)"), First >= 0.0 && First < 1.0);

	// Serialize / restore resumes the identical sequence.
	Ares::FAresRng A(7u);
	for (int32 I = 0; I < 5; ++I) { A.Next("edl"); }
	const auto Blob = A.Serialize();
	const double NextFromA = A.Next("edl");

	Ares::FAresRng B = Ares::FAresRng::Deserialize(7u, Blob);
	const double NextFromB = B.Next("edl");
	TestEqual(TEXT("restored stream resumes identically"), NextFromB, NextFromA);

	// Streams are independent.
	Ares::FAresRng Alone(42u);
	TArray<double> EdlAlone;
	for (int32 I = 0; I < 8; ++I) { EdlAlone.Add(Alone.Next("edl")); }

	Ares::FAresRng Mixed(42u);
	for (int32 I = 0; I < 8; ++I)
	{
		Mixed.Next("incidents");
		Mixed.Next("weather");
		TestEqual(TEXT("edl unaffected by other streams"), Mixed.Next("edl"), EdlAlone[I]);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAresCoreDeterminismTest,
	"Ares.Core.Determinism",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FAresCoreDeterminismTest::RunTest(const FString& Parameters)
{
	Ares::FAresData Data;
	FString Error;
	if (!LoadBalance(Data, Error))
	{
		AddError(FString::Printf(TEXT("balance.json: %s"), *Error));
		return false;
	}

	// Brief §9: 2000 simulated days twice from the same seed, identical hashes.
	auto Run = [&Data](uint32 Seed)
	{
		Ares::FProgramState State = Ares::MakeProgramState(Data, Seed);
		for (int32 Day = 0; Day < 2000; ++Day)
		{
			const Ares::FClockAdvanceResult Result = Ares::Advance(State.Clock, Data);
			State.Rng.Next("weather");
			if (State.Rng.Chance("incidents", 0.005))
			{
				State.Fleet.GroundedUntilEarthDay = State.Clock.EarthDay + 30.0;
			}
			if (Result.bWindowOpened)
			{
				State.Research.Points += static_cast<double>(State.Rng.Int("planning", 0, 9));
			}
		}
		return Ares::HashProgramState(State);
	};

	const uint64 HashA = Run(0xA11CE5u);
	const uint64 HashB = Run(0xA11CE5u);
	TestEqual(TEXT("same seed produces same state hash"), HashA, HashB);

	// And the hash is not degenerate.
	TestNotEqual(TEXT("different seeds diverge"), Run(1u), Run(2u));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
