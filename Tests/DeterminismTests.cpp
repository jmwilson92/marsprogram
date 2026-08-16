// Brief §9: "Determinism is testable. Add a test that runs 2000 simulated days
// twice from the same seed and asserts identical state hashes. Keep it green."

#include "AresCore/ProgramState.h"

#include <string>
#include <vector>

#include "AresCore/AresData.h"
#include "Harness/AresTest.h"
#include "OracleFixture.h"

using namespace Ares;

namespace
{
/**
 * Runs the program forward N days, exercising the RNG the way the real systems
 * will. Until the M2 tick functions land, this stands in for them: the point is
 * that the same seed drives the same stream draws in the same order.
 */
FProgramState RunDays(const FAresData& Data, uint32_t Seed, int Days)
{
	FProgramState State = MakeProgramState(Data, Seed);

	for (int Day = 0; Day < Days; ++Day)
	{
		const FClockAdvanceResult Result = Advance(State.Clock, Data);

		// A daily roll on two independent streams.
		const double WeatherRoll = State.Rng.Next("weather");
		if (WeatherRoll < 0.02)
		{
			State.Politics.Support -= 0.01;
		}

		if (State.Rng.Chance("incidents", 0.005))
		{
			State.Fleet.GroundedUntilEarthDay = State.Clock.EarthDay + 30.0;
		}

		// A window opening draws from a third stream, so stream ordering is
		// exercised non-uniformly across the run.
		if (Result.bWindowOpened)
		{
			const int64_t Pick = State.Rng.Int("planning", 0, 9);
			State.Research.Points += static_cast<FReal>(Pick);
			State.Budget.Ledger.push_back(FLedgerEntry{
				State.Clock.EarthDay, "window", "transfer window opened", -1.0e6 });
		}
	}

	return State;
}
} // namespace

ARES_TEST(Determinism, TwoThousandDaysTwiceSameSeedSameHash)
{
	const FAresData& Data = AresTest::Balance();

	const FProgramState A = RunDays(Data, 0xA11CE5u, 2000);
	const FProgramState B = RunDays(Data, 0xA11CE5u, 2000);

	const uint64_t HashA = HashProgramState(A);
	const uint64_t HashB = HashProgramState(B);

	CHECK_TRUE(HashA == HashB);
	if (HashA != HashB)
	{
		std::printf("        run A hash = %llu\n        run B hash = %llu\n",
			(unsigned long long)HashA, (unsigned long long)HashB);
	}

	// Sanity: the run actually did something, so the test cannot pass by
	// comparing two identical empty states.
	CHECK_INT(A.Clock.TickCount, 2000, "ticks");
	CHECK_TRUE(A.Clock.WindowIndex >= 2);
	CHECK_TRUE(A.Rng.GetCounter("weather") == 2000);
}

ARES_TEST(Determinism, DifferentSeedsDiverge)
{
	const FAresData& Data = AresTest::Balance();

	const uint64_t HashA = HashProgramState(RunDays(Data, 1u, 2000));
	const uint64_t HashB = HashProgramState(RunDays(Data, 2u, 2000));

	// If these matched, the hash would not be reading the RNG state and the
	// determinism test above would be vacuous.
	CHECK_TRUE(HashA != HashB);
}

ARES_TEST(Determinism, ResumingFromSerializedRngContinuesIdentically)
{
	const FAresData& Data = AresTest::Balance();

	// Run 1000 days, snapshot, run 1000 more.
	FProgramState Straight = MakeProgramState(Data, 0x5EEDu);
	for (int I = 0; I < 2000; ++I)
	{
		Advance(Straight.Clock, Data);
		Straight.Rng.Next("weather");
	}

	FProgramState Halted = MakeProgramState(Data, 0x5EEDu);
	for (int I = 0; I < 1000; ++I)
	{
		Advance(Halted.Clock, Data);
		Halted.Rng.Next("weather");
	}
	// Simulate a save/load boundary.
	const auto Blob = Halted.Rng.Serialize();
	Halted.Rng = FAresRng::Deserialize(Halted.Meta.Seed, Blob);
	for (int I = 0; I < 1000; ++I)
	{
		Advance(Halted.Clock, Data);
		Halted.Rng.Next("weather");
	}

	CHECK_TRUE(HashProgramState(Straight) == HashProgramState(Halted));
}

ARES_TEST(Determinism, HashChangesWhenAnyTrackedFieldChanges)
{
	const FAresData& Data = AresTest::Balance();
	const FProgramState Base = MakeProgramState(Data, 99u);
	const uint64_t BaseHash = HashProgramState(Base);

	{
		FProgramState S = Base;
		S.Politics.Support += 0.0001;
		CHECK_TRUE(HashProgramState(S) != BaseHash);
	}
	{
		FProgramState S = Base;
		S.Budget.RemainingUsd -= 1.0;
		CHECK_TRUE(HashProgramState(S) != BaseHash);
	}
	{
		FProgramState S = Base;
		S.Research.Completed.push_back("tech_x");
		CHECK_TRUE(HashProgramState(S) != BaseHash);
	}
	{
		FProgramState S = Base;
		S.Crew.Roster.push_back(FCrewMember{});
		CHECK_TRUE(HashProgramState(S) != BaseHash);
	}
	{
		// Wall-clock playtime is deliberately excluded — it must NOT change the hash.
		FProgramState S = Base;
		S.Meta.PlaytimeSeconds += 1234.5;
		CHECK_TRUE(HashProgramState(S) == BaseHash);
	}
}
