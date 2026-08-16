// Ports of ../reference src/sim/economy.js, politics.js and research.js,
// checked against a 2000-day run of the reference itself.
//
// The oracle runs the same four phases in the same pipeline order, so a
// mismatch here means the port diverged, not that the systems were configured
// differently.

#include "AresCore/ProgramSystems.h"

#include <string>

#include "AresCore/AresConstants.h"
#include "AresCore/AresData.h"
#include "AresCore/ProgramState.h"
#include "Harness/AresTest.h"
#include "OracleFixture.h"

using namespace Ares;

namespace
{
/** Money is in whole dollars against a 4.2 billion budget; 1e-9 relative is generous. */
constexpr double MONEY_TOL = 1e-9;
constexpr double SUPPORT_TOL = 1e-9;

/** Full content set, loaded once. Tech is needed for the starter-tech grant. */
const FAresData& FullData()
{
	static FAresData Data;
	static bool bLoaded = false;
	if (!bLoaded)
	{
		std::string Error;
		if (!FAresData::LoadAll(AresTestPaths::ProjectRoot() + "/Data", Data, Error))
		{
			std::printf("FATAL: LoadAll failed: %s\n", Error.c_str());
			std::exit(2);
		}
		bLoaded = true;
	}
	return Data;
}
} // namespace

ARES_TEST(Data, AllTablesLoadWithExpectedRowCounts)
{
	const FAresData& D = FullData();

	CHECK_INT(static_cast<long long>(D.Tech.size()), 40, "tech rows");
	CHECK_INT(static_cast<long long>(D.Contractors.size()), 8, "contractor rows");
	CHECK_INT(static_cast<long long>(D.Cargo.size()), 19, "cargo rows");
	CHECK_INT(static_cast<long long>(D.Roles.size()), 9, "role rows");
	CHECK_INT(static_cast<long long>(D.Resources.size()), 39, "resource rows");
	CHECK_INT(static_cast<long long>(D.Sites.size()), 10, "site rows");

	// Spot-check a row of each shape, so a silently-empty parse cannot pass.
	const FTechRow* Starship = D.FindTech("starship_cargo");
	CHECK_TRUE(Starship != nullptr);
	if (Starship)
	{
		CHECK_TRUE(Starship->bStartCompleted);
		CHECK_EXACT(Starship->StartingMaturity, 0.55, "starship_cargo maturity");
		CHECK_INT(Starship->Tier, 1, "starship_cargo tier");
	}

	const FCargoRow* Seat = D.FindCargo("crew_seat");
	CHECK_TRUE(Seat != nullptr);
	if (Seat)
	{
		CHECK_EXACT(Seat->MassKg, 180.0, "crew_seat mass");
		CHECK_INT(Seat->Crew, 1, "crew_seat crew");
	}

	const FSiteRow* Jezero = D.FindSite("jezero_north");
	CHECK_TRUE(Jezero != nullptr);
	if (Jezero)
	{
		CHECK_EXACT(Jezero->Lat, 18.4, "jezero lat");
	}

	// Human consumption rates the lab tick depends on.
	CHECK_EXACT(D.Human.FoodDryKgPerSol, 1.8, "foodDry_kg_per_sol");
	CHECK_EXACT(D.Human.WaterDrinkKgPerSol, 3.5, "waterDrink_kg_per_sol");
	CHECK_EXACT(D.Human.CareerDoseMSv, 1000.0, "careerDose_mSv");

	// DOCUMENTED GAP (brief §4.4): still no lunar sites. Flip when M5 adds them.
	CHECK_TRUE(D.IsSitesTableMarsOnly());
}

ARES_TEST(Program, OpeningStateMatchesReference)
{
	const FAresData& D = FullData();
	const auto& Opening = AresTest::Oracle()["program"]["opening"];

	const char* Difficulties[] = { "DIRECTOR", "ADMINISTRATOR", "AUSTERITY", "IRONMAN" };
	for (const char* Difficulty : Difficulties)
	{
		const FProgramState S = MakeProgramState(D, 1u, Difficulty);
		const auto& Expected = Opening[Difficulty];

		CHECK_EXACT(S.Budget.AnnualUsd, Expected["annual"].AsNumber(), "annual");
		CHECK_EXACT(S.Budget.RemainingUsd, Expected["remaining"].AsNumber(), "remaining");
		CHECK_EXACT(S.Politics.Support, Expected["support"].AsNumber(), "support");

		// Starter tech must match exactly — it seeds maturity, which scales EDL
		// success, so a wrong list quietly changes every landing in the game.
		const auto& ExpectedTech = Expected["completedTech"];
		CHECK_INT(static_cast<long long>(S.Research.Completed.size()),
			static_cast<long long>(ExpectedTech.Num()), "starter tech count");

		for (size_t I = 0; I < ExpectedTech.Num(); ++I)
		{
			const std::string Id = ExpectedTech[I].AsString();
			const bool bFound = std::find(S.Research.Completed.begin(),
				S.Research.Completed.end(), Id) != S.Research.Completed.end();
			CHECK_TRUE(bFound);

			const auto It = S.Research.Maturity.find(Id);
			CHECK_TRUE(It != S.Research.Maturity.end());
			if (It != S.Research.Maturity.end())
			{
				CHECK_EXACT(It->second,
					Expected["maturity"][Id].AsNumber(), "starter maturity");
			}
		}
	}
}

ARES_TEST(Program, TwoThousandDayTraceMatchesReference)
{
	// The headline test: run the program idle for 2000 days and compare budget,
	// support, hoarding, quarters-under-threshold, RP and cancellation against
	// the reference at nineteen sampled days.
	const FAresData& D = FullData();
	FProgramState State = MakeProgramState(D, 0xA11CE5u, DIFFICULTY_ADMINISTRATOR);

	const auto& Trace = AresTest::Oracle()["program"]["trace"];
	CHECK_TRUE(Trace.Num() > 0);

	size_t Next = 0;
	for (int Day = 1; Day <= 2000 && Next < Trace.Num(); ++Day)
	{
		TickProgramDay(State, D);

		const auto& Sample = Trace[Next];
		if (static_cast<int>(Sample["day"].AsNumber()) != Day)
		{
			continue;
		}

		CHECK_NEAR(State.Clock.EarthDay, Sample["earthDay"].AsNumber(), 1e-12, "earthDay");
		CHECK_NEAR(State.Budget.RemainingUsd, Sample["remaining"].AsNumber(), MONEY_TOL, "remaining");
		CHECK_EXACT(State.Budget.AnnualUsd, Sample["annual"].AsNumber(), "annual");
		CHECK_INT(State.Budget.bHoarded ? 1 : 0, Sample["hoarded"].AsBool() ? 1 : 0, "hoarded");
		CHECK_NEAR(State.Politics.Support, Sample["support"].AsNumber(), SUPPORT_TOL, "support");
		CHECK_INT(State.Politics.QuartersUnderThreshold,
			(long long)Sample["quartersUnderThreshold"].AsNumber(), "quarters under");
		CHECK_NEAR(State.Research.Points, Sample["researchPoints"].AsNumber(), 1e-12, "RP");
		CHECK_INT(State.Clock.WindowIndex, (long long)Sample["windowIndex"].AsNumber(), "windowIndex");

		const bool bExpectCancelled = !Sample["cancellation"].IsNull();
		CHECK_INT(State.Politics.bCancelled ? 1 : 0, bExpectCancelled ? 1 : 0, "cancelled");
		if (bExpectCancelled)
		{
			CHECK_STR(State.Politics.CancellationReason,
				Sample["cancellation"].AsString(), "cancellation reason");
		}

		++Next;
	}

	CHECK_INT(static_cast<long long>(Next), static_cast<long long>(Trace.Num()), "samples consumed");
}

ARES_TEST(Program, IdlePlayEndsInCancellation)
{
	// The M2 acceptance criterion, as a test: play the program idle and it dies.
	// Reserves pile up past the 2x cap, hoarding costs 0.03 support a day, and
	// four quarters under 20 finishes it.
	const FAresData& D = FullData();
	FProgramState State = MakeProgramState(D, 7u, DIFFICULTY_ADMINISTRATOR);

	const int32_t Ran = TickProgramDays(State, D, 3000);

	CHECK_TRUE(State.Politics.bCancelled);
	CHECK_STR(State.Politics.CancellationReason, "cancelled", "reason");
	CHECK_TRUE(State.Budget.bHoarded);
	CHECK_TRUE(Ran < 3000);

	// Roughly five program years, which is the pacing brief §8/M2 asks for.
	const FReal Years = ProgramYear(State.Clock.EarthDay);
	CHECK_TRUE(Years > 4.0 && Years < 6.0);
	if (!(Years > 4.0 && Years < 6.0))
	{
		std::printf("        cancelled after %.2f program years (day %.0f)\n",
			Years, State.Clock.EarthDay);
	}
}

ARES_TEST(Program, SpendingKeepsSupportAlive)
{
	// The inverse: a program that spends its appropriation never trips the
	// hoarding penalty, so support decays at the slow idle rate instead.
	const FAresData& D = FullData();
	FProgramState State = MakeProgramState(D, 7u, DIFFICULTY_ADMINISTRATOR);

	for (int32_t Day = 0; Day < 1800; ++Day)
	{
		TickProgramDay(State, D);
		// Spend it down below the cap each quarter, as a real program would.
		const FReal Cap = State.Budget.AnnualUsd * HOARD_CAP_MULTIPLE;
		if (State.Budget.RemainingUsd > Cap * 0.8)
		{
			State.Budget.RemainingUsd = Cap * 0.5;
		}
	}

	CHECK_TRUE(!State.Budget.bHoarded);
	CHECK_TRUE(!State.Politics.bCancelled);
	// Idle drift alone is -0.012/day; over 1800 days that is about -21.6.
	CHECK_TRUE(State.Politics.Support > 30.0);
}

ARES_TEST(Program, AwardRpIsOncePerMilestone)
{
	const FAresData& D = FullData();
	FProgramState State = MakeProgramState(D, 1u);

	CHECK_TRUE(AwardRp(State, "first_leo", 40.0));
	CHECK_NEAR(State.Research.Points, 40.0, 1e-12, "points after first award");

	// Same key again pays nothing. This is what lets a system call AwardRp
	// every tick without tracking whether it already did.
	CHECK_TRUE(!AwardRp(State, "first_leo", 40.0));
	CHECK_NEAR(State.Research.Points, 40.0, 1e-12, "points unchanged");

	CHECK_TRUE(AwardRp(State, "first_crewed_leo", 60.0));
	CHECK_NEAR(State.Research.Points, 100.0, 1e-12, "points after second award");

	// Zero is not a milestone.
	CHECK_TRUE(!AwardRp(State, "nothing", 0.0));
	CHECK_TRUE(!HasMilestone(State, "nothing"));
}

ARES_TEST(Program, TransferWindowPaysRpOnce)
{
	const FAresData& D = FullData();
	FProgramState State = MakeProgramState(D, 1u);

	// First window is at day 210; nothing before it.
	for (int32_t Day = 0; Day < 209; ++Day)
	{
		TickProgramDay(State, D);
	}
	CHECK_NEAR(State.Research.Points, 0.0, 1e-12, "no RP before the window");

	TickProgramDay(State, D);
	CHECK_NEAR(State.Research.Points, WINDOW_OPEN_RP, 1e-12, "RP on window open");

	// And it does not pay again the next day.
	TickProgramDay(State, D);
	CHECK_NEAR(State.Research.Points, WINDOW_OPEN_RP, 1e-12, "no repeat payment");
}

ARES_TEST(Program, CrewedLabStarvesAndGoesOffline)
{
	// Brief §7: LEO labs generate RP; crewed ones consume stores and can fail.
	const FAresData& D = FullData();
	FProgramState State = MakeProgramState(D, 1u);

	FLeoLab Lab;
	Lab.Id = "lab_1";
	Lab.Crew = 2;
	Lab.Robots = 1;
	Lab.RpPerDay = LAB_RP_PER_DAY_CREWED;
	// Three days of food for two people at 1.8 kg/sol each.
	Lab.FoodKg = 1.8 * 2 * 3;
	Lab.WaterKg = 10000.0;
	Lab.AirDays = 10000.0;
	State.Research.Labs.push_back(Lab);

	for (int32_t Day = 0; Day < 3; ++Day)
	{
		TickProgramDay(State, D);
	}
	CHECK_TRUE(!State.Research.Labs[0].bOffline);
	CHECK_NEAR(State.Research.Points, LAB_RP_PER_DAY_CREWED * 3.0, 1e-9, "RP over three days");

	// Fourth day takes food negative: the lab goes offline and stops paying.
	TickProgramDay(State, D);
	CHECK_TRUE(State.Research.Labs[0].bOffline);
	CHECK_NEAR(State.Research.Labs[0].FoodKg, 0.0, 1e-12, "food clamped to zero");

	const FReal PointsAtFailure = State.Research.Points;
	TickProgramDay(State, D);
	CHECK_NEAR(State.Research.Points, PointsAtFailure, 1e-12, "offline lab pays nothing");
}

ARES_TEST(Program, RoboticLabNeverStarves)
{
	// Optimus does not eat. A robotic lab runs indefinitely at the lower rate.
	const FAresData& D = FullData();
	FProgramState State = MakeProgramState(D, 1u);

	FLeoLab Lab;
	Lab.Id = "lab_robotic";
	Lab.Crew = 0;
	Lab.Robots = 2;
	Lab.RpPerDay = LAB_RP_PER_DAY_ROBOTIC;
	State.Research.Labs.push_back(Lab);

	for (int32_t Day = 0; Day < 100; ++Day)
	{
		TickProgramDay(State, D);
	}

	CHECK_TRUE(!State.Research.Labs[0].bOffline);
	// 100 days of lab output, plus nothing else — no window inside 100 days.
	CHECK_NEAR(State.Research.Points, LAB_RP_PER_DAY_ROBOTIC * 100.0, 1e-9, "robotic RP");
}

ARES_TEST(Program, TickingTheSameDayTwiceIsIdempotent)
{
	// Economy and politics both guard on the whole day. A UI that re-ticks, or
	// a snapshot taken mid-frame, must not double-charge the program.
	const FAresData& D = FullData();
	FProgramState State = MakeProgramState(D, 1u);

	TickProgramDay(State, D);

	const FReal RemainingAfterOne = State.Budget.RemainingUsd;
	const FReal SupportAfterOne = State.Politics.Support;

	// Re-run the phases WITHOUT advancing the clock.
	TickEconomy(State, D);
	TickPolitics(State, D);

	CHECK_EXACT(State.Budget.RemainingUsd, RemainingAfterOne, "budget not double-charged");
	CHECK_EXACT(State.Politics.Support, SupportAfterOne, "support not double-drifted");
}

ARES_TEST(Program, TickIsDeterministicAcrossRuns)
{
	// Brief §9, applied to the program systems rather than just the clock.
	const FAresData& D = FullData();

	FProgramState A = MakeProgramState(D, 0xBEEFu);
	FProgramState B = MakeProgramState(D, 0xBEEFu);
	TickProgramDays(A, D, 2000);
	TickProgramDays(B, D, 2000);

	CHECK_TRUE(HashProgramState(A) == HashProgramState(B));
}
