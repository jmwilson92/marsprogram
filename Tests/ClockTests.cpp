// Port of ../reference src/core/clock.js, checked against the JS oracle.
// This is the clock test brief §8/M0 asks for.

#include "AresCore/SimClock.h"

#include "AresCore/AresData.h"
#include "Harness/AresTest.h"
#include "OracleFixture.h"

using namespace Ares;

namespace
{
constexpr double TOL = 1e-11;

void CheckClockAgainst(const FSimClock& Clock, const FJsonValue& Expected, const char* Label)
{
	CHECK_NEAR(Clock.EarthDay, Expected["earthDay"].AsNumber(), TOL, "earthDay");
	CHECK_NEAR(Clock.Sol, Expected["sol"].AsNumber(), TOL, "sol");
	CHECK_INT(Clock.MarsYear, (long long)Expected["marsYear"].AsNumber(), "marsYear");
	CHECK_NEAR(Clock.SolOfYear, Expected["solOfYear"].AsNumber(), TOL, "solOfYear");
	CHECK_NEAR(Clock.Ls, Expected["Ls"].AsNumber(), TOL, "Ls");
	CHECK_INT(Clock.TickCount, (long long)Expected["tickCount"].AsNumber(), "tickCount");
	CHECK_NEAR(Clock.InsolationFactor, Expected["insolationFactor"].AsNumber(), TOL, "insolationFactor");
	CHECK_NEAR(Clock.SolarConstantWm2, Expected["solarConstant_wm2"].AsNumber(), TOL, "solarConstant_wm2");
	CHECK_INT(Clock.WindowIndex, (long long)Expected["windowIndex"].AsNumber(), "windowIndex");
	CHECK_NEAR(Clock.NextWindowEarthDay, Expected["nextWindowEarthDay"].AsNumber(), TOL, "nextWindowEarthDay");
	CHECK_NEAR(Clock.DaysToWindow, Expected["daysToWindow"].AsNumber(), TOL, "daysToWindow");
	CHECK_INT(Clock.bWindowJustOpened ? 1 : 0,
		Expected["windowJustOpened"].AsBool() ? 1 : 0, "windowJustOpened");
	(void)Label;
}
} // namespace

ARES_TEST(Clock, EpochPinsMatchReference)
{
	// The reference computes this from Date.UTC arithmetic; we compute it from
	// integer civil-date math. They must agree, or every Ls in the game is wrong.
	CHECK_EXACT(EarthDaysLs0ToEpoch(),
		AresTest::Oracle()["constants"]["EARTH_DAYS_LS0_TO_EPOCH"].AsNumber(),
		"EARTH_DAYS_LS0_TO_EPOCH");

	CHECK_INT(MARS_YEAR_AT_LS0,
		(long long)AresTest::Oracle()["constants"]["MARS_YEAR_AT_LS0"].AsNumber(),
		"MARS_YEAR_AT_LS0");
}

ARES_TEST(Clock, BalanceDerivedConstantsMatchReference)
{
	// The reference hardcodes these in constants.js; we read them from
	// balance.json. Both must land on the same numbers.
	const FAresData& Data = AresTest::Balance();
	const auto& C = AresTest::Oracle()["constants"];

	CHECK_EXACT(Data.Time.SolSeconds, C["SOL_SECONDS"].AsNumber(), "solSeconds");
	CHECK_EXACT(Data.Time.EarthDaySeconds, C["EARTH_DAY_SECONDS"].AsNumber(), "earthDaySeconds");
	CHECK_EXACT(Data.Time.SolsPerEarthDay(), C["SOLS_PER_EARTH_DAY"].AsNumber(), "solsPerEarthDay");
	CHECK_EXACT(Data.Time.EarthDaysPerSol(), C["EARTH_DAYS_PER_SOL"].AsNumber(), "earthDaysPerSol");
	CHECK_EXACT(Data.Time.MarsYearSols, C["MARS_YEAR_SOLS"].AsNumber(), "marsYearSols");
	CHECK_EXACT(Data.Time.SynodicEarthDays, C["SYNODIC_EARTH_DAYS"].AsNumber(), "synodicEarthDays");
	CHECK_EXACT(Data.Time.FirstWindowEarthDay, C["FIRST_WINDOW_EARTH_DAY"].AsNumber(), "firstWindowEarthDay");
	CHECK_EXACT(Data.Orbit.Eccentricity, C["MARS_ECCENTRICITY"].AsNumber(), "eccentricity");
	CHECK_EXACT(Data.Orbit.LsPerihelionDeg, C["LS_PERIHELION_DEG"].AsNumber(), "lsPerihelionDeg");
	CHECK_EXACT(Data.Orbit.SolarConstantMeanWm2, C["SOLAR_CONSTANT_MEAN_WM2"].AsNumber(), "solarConstant");
}

ARES_TEST(Clock, InitialStateMatchesReference)
{
	const FSimClock Clock = MakeClock(AresTest::Balance());
	CheckClockAgainst(Clock, AresTest::Oracle()["clock"]["initial"], "initial");
}

ARES_TEST(Clock, EarthDayTraceMatchesReference)
{
	const FAresData& Data = AresTest::Balance();
	FSimClock Clock = MakeClock(Data);

	const auto& Trace = AresTest::Oracle()["clock"]["earthDayTrace"];
	CHECK_TRUE(Trace.Num() > 0);

	size_t Next = 0;
	for (int Day = 1; Day <= 2000 && Next < Trace.Num(); ++Day)
	{
		const FClockAdvanceResult Result = Advance(Clock, Data);
		const auto& Sample = Trace[Next];
		if (static_cast<int>(Sample["day"].AsNumber()) == Day)
		{
			CheckClockAgainst(Clock, Sample, "earthDayTrace");
			CHECK_INT(Result.bWindowOpened ? 1 : 0,
				Sample["windowOpened"].AsBool() ? 1 : 0, "windowOpened");
			++Next;
		}
	}
	CHECK_INT(static_cast<long long>(Next), static_cast<long long>(Trace.Num()), "samples consumed");
}

ARES_TEST(Clock, SolTraceMatchesReference)
{
	const FAresData& Data = AresTest::Balance();
	FSimClock Clock = MakeClock(Data);
	Clock.TickUnit = ETickUnit::Sol;

	const auto& Trace = AresTest::Oracle()["clock"]["solTrace"];
	CHECK_TRUE(Trace.Num() > 0);

	size_t Next = 0;
	for (int Tick = 1; Tick <= 700 && Next < Trace.Num(); ++Tick)
	{
		const FClockAdvanceResult Result = Advance(Clock, Data);
		const auto& Sample = Trace[Next];
		if (static_cast<int>(Sample["tick"].AsNumber()) == Tick)
		{
			CheckClockAgainst(Clock, Sample, "solTrace");
			CHECK_INT(Result.bWindowOpened ? 1 : 0,
				Sample["windowOpened"].AsBool() ? 1 : 0, "windowOpened");
			++Next;
		}
	}
	CHECK_INT(static_cast<long long>(Next), static_cast<long long>(Trace.Num()), "samples consumed");
}

ARES_TEST(Clock, EarthDateMatchesReference)
{
	const auto& Cases = AresTest::Oracle()["clock"]["earthDate"];
	CHECK_TRUE(Cases.Num() > 0);
	for (size_t I = 0; I < Cases.Num(); ++I)
	{
		const double Day = Cases[I]["in"].AsNumber();
		const auto& Expected = Cases[I]["out"];
		const FEarthDate Actual = EarthDateFromDay(Day);
		CHECK_INT(Actual.Year, (long long)Expected["year"].AsNumber(), "year");
		CHECK_INT(Actual.Month, (long long)Expected["month"].AsNumber(), "month");
		CHECK_INT(Actual.Day, (long long)Expected["day"].AsNumber(), "day");
		CHECK_STR(Actual.ToIso(), Expected["iso"].AsString(), "iso");
	}
}

ARES_TEST(Clock, SeasonNamesMatchReference)
{
	const auto& Cases = AresTest::Oracle()["clock"]["season"];
	for (size_t I = 0; I < Cases.Num(); ++I)
	{
		CHECK_STR(SeasonName(Cases[I]["in"].AsNumber()), Cases[I]["out"].AsString(), "season");
	}
}

ARES_TEST(Clock, OrbitalDistanceFactorMatchesReference)
{
	const auto& Cases = AresTest::Oracle()["math"]["orbitalDistanceFactor"];
	for (size_t I = 0; I < Cases.Num(); ++I)
	{
		CHECK_NEAR(OrbitalDistanceFactor(Cases[I]["in"].AsNumber(), AresTest::Balance()),
			Cases[I]["out"].AsNumber(), 1e-12, "orbitalDistanceFactor");
	}
}

ARES_TEST(Clock, FirstTransferWindowOpensOnScheduledDay)
{
	// balance.json puts the first Earth-Mars window at day 210; it must open on
	// that exact tick and the next one a synodic period later.
	const FAresData& Data = AresTest::Balance();
	FSimClock Clock = MakeClock(Data);

	int OpenedOnDay = -1;
	for (int Day = 1; Day <= 300; ++Day)
	{
		if (Advance(Clock, Data).bWindowOpened)
		{
			OpenedOnDay = Day;
			break;
		}
	}
	CHECK_INT(OpenedOnDay, (long long)Data.Time.FirstWindowEarthDay, "first window day");
	CHECK_INT(Clock.WindowIndex, 1, "window index after first");
	CHECK_NEAR(Clock.NextWindowEarthDay,
		Data.Time.FirstWindowEarthDay + Data.Time.SynodicEarthDays, 1e-12, "second window day");
}

ARES_TEST(Clock, InterpolateDoesNotMutateOrConsumeWindows)
{
	const FAresData& Data = AresTest::Balance();
	FSimClock Clock = MakeClock(Data);
	for (int Day = 1; Day <= 209; ++Day)
	{
		Advance(Clock, Data);
	}

	const FSimClock Before = Clock;
	// Interpolating across the window boundary must not consume it — only
	// Advance() may change program state.
	const FSimClock Lerped = InterpolateClock(Clock, Data, 5.0);

	CHECK_EXACT(Clock.EarthDay, Before.EarthDay, "source untouched");
	CHECK_INT(Clock.WindowIndex, Before.WindowIndex, "window index untouched");
	CHECK_NEAR(Lerped.EarthDay, Before.EarthDay + 5.0, 1e-12, "interpolated day");
	CHECK_INT(Lerped.WindowIndex, Before.WindowIndex, "interpolation consumed no window");
	CHECK_INT(Lerped.bWindowJustOpened ? 1 : 0, 0, "interpolation raised no window flag");
}

ARES_TEST(Clock, ProgramYearAdvancesWithEarthDays)
{
	CHECK_NEAR(ProgramYear(0.0), 0.0, 1e-12, "year 0");
	CHECK_NEAR(ProgramYear(365.2425), 1.0, 1e-12, "year 1");
	CHECK_NEAR(ProgramYear(365.2425 * 5.0), 5.0, 1e-12, "year 5");
}
