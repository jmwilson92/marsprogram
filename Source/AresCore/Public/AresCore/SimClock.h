// AresCore — FSimClock (brief §3.1).
//
// Fixed-step. Program time advances in whole days; flight time advances in
// seconds. Two rates, one clock, explicit conversion. Ported from
// ../reference src/core/clock.js including the dual Earth-day / Mars-sol / Ls
// calendar and the synodic transfer-window countdown.
//
// Every balance number the clock uses (sol length, Mars year, synodic period,
// eccentricity, perihelion Ls, solar constant, first window) is read from
// FAresData, i.e. from data/balance.json. The clock holds none of its own.

#pragma once

#include <cstdint>
#include <string>

#include "AresCore/AresApi.h"
#include "AresCore/AresConstants.h"
#include "AresCore/AresMath.h"

namespace Ares
{

struct FAresData;

/** A civil Earth date, as shown on Mission Control's date readout. */
struct FEarthDate
{
	int32_t Year = 0;
	uint32_t Month = 0;
	uint32_t Day = 0;

	/** ISO YYYY-MM-DD. */
	ARESCORE_API std::string ToIso() const;
};

struct FSimClock
{
	/** Earth days since program epoch. Whole-day steps in EarthDay tick mode. */
	FReal EarthDay = 0.0;
	/** Mars sols since program epoch. */
	FReal Sol = 0.0;

	int32_t MarsYear = 0;
	FReal SolOfYear = 0.0;
	/** Areocentric solar longitude, degrees, [0, 360). Drives Mars seasons. */
	FReal Ls = 0.0;

	int64_t TickCount = 0;
	ESimSpeed Speed = ESimSpeed::Pause;
	ETickUnit TickUnit = ETickUnit::EarthDay;

	/** 1/r^2 relative to Mars' semi-major axis. */
	FReal InsolationFactor = 1.0;
	FReal SolarConstantWm2 = 0.0;

	/** Earth-Mars transfer windows, counted off the synodic period. */
	int32_t WindowIndex = 0;
	FReal NextWindowEarthDay = 0.0;
	FReal DaysToWindow = 0.0;
	bool bWindowJustOpened = false;
};

struct FClockAdvanceResult
{
	bool bWindowOpened = false;
};

/** Days between two proleptic Gregorian civil dates. Exact integer arithmetic. */
ARESCORE_API int64_t DaysFromCivil(int32_t Year, uint32_t Month, uint32_t Day);
ARESCORE_API FEarthDate CivilFromDays(int64_t Days);

/** Builds a clock at program day 0 with all derived fields populated. */
ARESCORE_API FSimClock MakeClock(const FAresData& Data);

/** Recomputes Mars year / Ls / insolation / window countdown from EarthDay. */
ARESCORE_API void RefreshDerived(FSimClock& Clock, const FAresData& Data);

/** Advances exactly one tick in the clock's current tick unit. */
ARESCORE_API FClockAdvanceResult Advance(FSimClock& Clock, const FAresData& Data);

/**
 * Returns a copy advanced by ExtraTicks without consuming transfer windows.
 * Used for smooth UI interpolation between simulation ticks — it must never
 * mutate program state, which is why it returns a value.
 */
ARESCORE_API FSimClock InterpolateClock(const FSimClock& Clock, const FAresData& Data, FReal ExtraTicks);

ARESCORE_API FEarthDate EarthDateFromDay(FReal EarthDay);
ARESCORE_API std::string FormatEarthDate(FReal EarthDay);

/** Mars season name for a solar longitude, northern-hemisphere referenced. */
ARESCORE_API std::string SeasonName(FReal Ls);

/** Insolation scale factor at a given Ls: (1/r)^2 in units of the semi-major axis. */
ARESCORE_API FReal OrbitalDistanceFactor(FReal Ls, const FAresData& Data);

/** Mean anomaly at Ls = 0, derived from the orbit balance. */
ARESCORE_API FReal MeanAnomalyAtLs0(const FAresData& Data);

/** Earth days between the MY46 Ls=0 pin and the program epoch. */
ARESCORE_API FReal EarthDaysLs0ToEpoch();

/** Program years elapsed, Julian year. Mirrors the reference's programYear(). */
ARESCORE_API FReal ProgramYear(FReal EarthDay);

} // namespace Ares
