#include "AresCore/SimClock.h"

#include <cstdio>

#include "AresCore/AresData.h"

namespace Ares
{

std::string FEarthDate::ToIso() const
{
	char Buffer[32];
	std::snprintf(Buffer, sizeof(Buffer), "%d-%02u-%02u", Year, Month, Day);
	return std::string(Buffer);
}

/*
 * Days-from-civil / civil-from-days, after Howard Hinnant's chrono algorithms.
 * Pure integer arithmetic on the proleptic Gregorian calendar, so the calendar
 * never accumulates floating-point drift the way a milliseconds-since-epoch
 * division would.
 */
int64_t DaysFromCivil(int32_t Year, uint32_t Month, uint32_t Day)
{
	// Kept entirely in signed 64-bit. The published form of this algorithm mixes
	// unsigned intermediates with negative month offsets, which is correct only
	// by wraparound and trips -Wsign-conversion.
	const int64_t M = static_cast<int64_t>(Month);
	const int64_t D = static_cast<int64_t>(Day);

	int64_t Y = static_cast<int64_t>(Year) - (M <= 2 ? 1 : 0);
	const int64_t Era = (Y >= 0 ? Y : Y - 399) / 400;
	const int64_t Yoe = Y - Era * 400;                                     // [0, 399]
	const int64_t Doy = (153 * (M + (M > 2 ? -3 : 9)) + 2) / 5 + D - 1;    // [0, 365]
	const int64_t Doe = Yoe * 365 + Yoe / 4 - Yoe / 100 + Doy;             // [0, 146096]
	return Era * 146097 + Doe - 719468;
}

FEarthDate CivilFromDays(int64_t Days)
{
	Days += 719468;
	const int64_t Era = (Days >= 0 ? Days : Days - 146096) / 146097;
	const int64_t Doe = Days - Era * 146097;                               // [0, 146096]
	const int64_t Yoe = (Doe - Doe / 1460 + Doe / 36524 - Doe / 146096) / 365; // [0, 399]
	const int64_t Y = Yoe + Era * 400;
	const int64_t Doy = Doe - (365 * Yoe + Yoe / 4 - Yoe / 100);           // [0, 365]
	const int64_t Mp = (5 * Doy + 2) / 153;                                // [0, 11]
	const int64_t D = Doy - (153 * Mp + 2) / 5 + 1;                        // [1, 31]
	const int64_t M = Mp + (Mp < 10 ? 3 : -9);                             // [1, 12]

	FEarthDate Out;
	Out.Year = static_cast<int32_t>(Y + (M <= 2 ? 1 : 0));
	Out.Month = static_cast<uint32_t>(M);
	Out.Day = static_cast<uint32_t>(D);
	return Out;
}

namespace
{
/** Days from the Unix epoch to program epoch 2040-01-01. */
int64_t EpochDayFromUnix()
{
	static const int64_t Cached = DaysFromCivil(EPOCH_YEAR, EPOCH_MONTH, EPOCH_DAY);
	return Cached;
}
} // namespace

FReal EarthDaysLs0ToEpoch()
{
	static const FReal Cached = static_cast<FReal>(
		DaysFromCivil(EPOCH_YEAR, EPOCH_MONTH, EPOCH_DAY) - DaysFromCivil(LS0_YEAR, LS0_MONTH, LS0_DAY));
	return Cached;
}

FReal MeanAnomalyAtLs0(const FAresData& Data)
{
	// True anomaly is 90 deg when perihelion is pinned at Ls 270.
	return TrueToMeanAnomaly(WrapDeg(0.0 - Data.Orbit.LsPerihelionDeg), Data.Orbit.Eccentricity);
}

FEarthDate EarthDateFromDay(FReal EarthDay)
{
	const int64_t Whole = static_cast<int64_t>(std::floor(EarthDay));
	return CivilFromDays(EpochDayFromUnix() + Whole);
}

std::string FormatEarthDate(FReal EarthDay)
{
	return EarthDateFromDay(EarthDay).ToIso();
}

std::string SeasonName(FReal Ls)
{
	const FReal X = WrapDeg(Ls);
	if (X < 90.0) return "northern spring";
	if (X < 180.0) return "northern summer";
	if (X < 270.0) return "northern autumn";
	return "northern winter";
}

FReal OrbitalDistanceFactor(FReal Ls, const FAresData& Data)
{
	const FReal Ecc = Data.Orbit.Eccentricity;
	const FReal Nu = DegToRad(WrapDeg(Ls - Data.Orbit.LsPerihelionDeg));
	const FReal R = (1.0 - Ecc * Ecc) / (1.0 + Ecc * std::cos(Nu));
	return 1.0 / (R * R);
}

FReal ProgramYear(FReal EarthDay)
{
	return EarthDay / 365.2425;
}

void RefreshDerived(FSimClock& Clock, const FAresData& Data)
{
	const FReal SolsPerEarthDay = Data.Time.SolsPerEarthDay();
	const FReal MarsYearSols = Data.Time.MarsYearSols;

	const FReal SolsSinceLs0 = (EarthDaysLs0ToEpoch() + Clock.EarthDay) * SolsPerEarthDay;
	const FReal MyOffset = SolsSinceLs0 / MarsYearSols;
	Clock.MarsYear = MARS_YEAR_AT_LS0 + static_cast<int32_t>(std::floor(MyOffset));

	FReal Soy = SolsSinceLs0 - std::floor(MyOffset) * MarsYearSols;
	if (Soy < 0.0)
	{
		Soy += MarsYearSols;
	}
	Clock.SolOfYear = Soy;

	const FReal M = MeanAnomalyAtLs0(Data) + (360.0 * SolsSinceLs0) / MarsYearSols;
	const FReal Nu = MeanToTrueAnomaly(M, Data.Orbit.Eccentricity);
	Clock.Ls = WrapDeg(Nu + Data.Orbit.LsPerihelionDeg);

	Clock.InsolationFactor = OrbitalDistanceFactor(Clock.Ls, Data);
	Clock.SolarConstantWm2 = Data.Orbit.SolarConstantMeanWm2 * Clock.InsolationFactor;

	Clock.NextWindowEarthDay = Data.Time.FirstWindowEarthDay + Clock.WindowIndex * Data.Time.SynodicEarthDays;
	Clock.DaysToWindow = Clock.NextWindowEarthDay - Clock.EarthDay;
}

namespace
{
void ConsumeWindows(FSimClock& Clock, const FAresData& Data)
{
	Clock.bWindowJustOpened = false;
	// A single timewarped batch can skip past a window; consume every crossed index.
	while (Clock.EarthDay + 1e-9 >= Data.Time.FirstWindowEarthDay + Clock.WindowIndex * Data.Time.SynodicEarthDays)
	{
		Clock.bWindowJustOpened = true;
		Clock.WindowIndex += 1;
	}
	Clock.NextWindowEarthDay = Data.Time.FirstWindowEarthDay + Clock.WindowIndex * Data.Time.SynodicEarthDays;
	Clock.DaysToWindow = Clock.NextWindowEarthDay - Clock.EarthDay;
}
} // namespace

FSimClock MakeClock(const FAresData& Data)
{
	FSimClock Clock;
	Clock.EarthDay = 0.0;
	Clock.Sol = 0.0;
	Clock.MarsYear = MARS_YEAR_AT_LS0;
	Clock.SolOfYear = 0.0;
	Clock.Ls = 0.0;
	Clock.TickCount = 0;
	Clock.Speed = ESimSpeed::Pause;
	Clock.TickUnit = ETickUnit::EarthDay;
	Clock.InsolationFactor = 1.0;
	Clock.SolarConstantWm2 = Data.Orbit.SolarConstantMeanWm2;
	Clock.WindowIndex = 0;
	Clock.NextWindowEarthDay = Data.Time.FirstWindowEarthDay;
	Clock.DaysToWindow = Data.Time.FirstWindowEarthDay;
	Clock.bWindowJustOpened = false;
	RefreshDerived(Clock, Data);
	return Clock;
}

FClockAdvanceResult Advance(FSimClock& Clock, const FAresData& Data)
{
	if (Clock.TickUnit == ETickUnit::Sol)
	{
		Clock.Sol += 1.0;
		Clock.EarthDay += Data.Time.EarthDaysPerSol();
	}
	else
	{
		Clock.EarthDay += 1.0;
		Clock.Sol += Data.Time.SolsPerEarthDay();
	}
	Clock.TickCount += 1;
	RefreshDerived(Clock, Data);
	ConsumeWindows(Clock, Data);

	FClockAdvanceResult Result;
	Result.bWindowOpened = Clock.bWindowJustOpened;
	return Result;
}

FSimClock InterpolateClock(const FSimClock& Clock, const FAresData& Data, FReal ExtraTicks)
{
	if (ExtraTicks == 0.0)
	{
		return Clock;
	}
	FSimClock Copy = Clock;
	if (Copy.TickUnit == ETickUnit::Sol)
	{
		Copy.Sol += ExtraTicks;
		Copy.EarthDay += ExtraTicks * Data.Time.EarthDaysPerSol();
	}
	else
	{
		Copy.EarthDay += ExtraTicks;
		Copy.Sol += ExtraTicks * Data.Time.SolsPerEarthDay();
	}
	RefreshDerived(Copy, Data);
	Copy.bWindowJustOpened = false;
	return Copy;
}

} // namespace Ares
