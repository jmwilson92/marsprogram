// FProgramState construction and the sub-struct split (brief §3.1).

#include "AresCore/ProgramState.h"

#include "AresCore/AresData.h"
#include "Harness/AresTest.h"
#include "OracleFixture.h"

using namespace Ares;

ARES_TEST(State, OpeningStateSeedsFromBalance)
{
	const FAresData& Data = AresTest::Balance();
	const FProgramState S = MakeProgramState(Data, 0x1234u, DIFFICULTY_ADMINISTRATOR);

	CHECK_INT(S.Meta.Seed, 0x1234, "seed");
	CHECK_INT(S.Meta.SchemaVersion, ARES_SAVE_SCHEMA_VERSION, "schema version");
	CHECK_STR(S.Meta.Difficulty, "ADMINISTRATOR", "difficulty");

	// Budget and support come from balance.json, not from constants in C++.
	CHECK_EXACT(S.Budget.AnnualUsd, Data.Economy.StartingAnnualAdministrator, "annual");
	CHECK_EXACT(S.Budget.RemainingUsd, Data.Economy.StartingAnnualAdministrator, "remaining");
	CHECK_EXACT(S.Politics.Support, Data.Economy.StartingSupport, "support");

	CHECK_EXACT(S.Clock.EarthDay, 0.0, "clock at day 0");
	CHECK_INT(S.Clock.MarsYear, MARS_YEAR_AT_LS0, "mars year at epoch");
	CHECK_INT(S.Rng.GetRootSeed(), 0x1234, "rng root seed");
}

ARES_TEST(State, DifficultyChangesOpeningBudget)
{
	const FAresData& Data = AresTest::Balance();
	const FProgramState Director = MakeProgramState(Data, 1u, DIFFICULTY_DIRECTOR);
	const FProgramState Ironman = MakeProgramState(Data, 1u, DIFFICULTY_IRONMAN);

	CHECK_EXACT(Director.Budget.AnnualUsd, 6000000000.0, "director annual");
	CHECK_EXACT(Ironman.Budget.AnnualUsd, 2200000000.0, "ironman annual");
	CHECK_TRUE(Director.Budget.AnnualUsd > Ironman.Budget.AnnualUsd);
}

ARES_TEST(State, SubStructsStartEmpty)
{
	// The state is split per system rather than being one mutable blob; a fresh
	// program owns no fleet, missions, crew, colony or contracts yet.
	const FProgramState S = MakeProgramState(AresTest::Balance(), 5u);

	CHECK_INT(static_cast<long long>(S.Fleet.Ships.size()), 0, "ships");
	CHECK_INT(static_cast<long long>(S.Fleet.Boosters.size()), 0, "boosters");
	CHECK_INT(static_cast<long long>(S.Missions.Planned.size()), 0, "planned");
	CHECK_INT(static_cast<long long>(S.Missions.InTransit.size()), 0, "in transit");
	CHECK_INT(static_cast<long long>(S.Crew.Roster.size()), 0, "crew");
	CHECK_INT(static_cast<long long>(S.Contracts.Contracts.size()), 0, "contracts");
	CHECK_INT(static_cast<long long>(S.Budget.Ledger.size()), 0, "ledger");
	CHECK_EXACT(S.Research.Points, 0.0, "research points");
	CHECK_TRUE(!S.Research.bHasActive);
	CHECK_TRUE(!S.Politics.bCancelled);
	CHECK_TRUE(S.Colony.bLifeSupportPowered);
}

ARES_TEST(State, CopyingStateIsValueSemantics)
{
	// UProgramSubsystem hands read-only snapshots to UI (brief §3.2); those
	// snapshots must be independent copies, not aliases.
	FProgramState A = MakeProgramState(AresTest::Balance(), 7u);
	FProgramState B = A;

	B.Politics.Support = 1.0;
	B.Budget.Ledger.push_back(FLedgerEntry{});
	B.Rng.Next("edl");

	CHECK_TRUE(A.Politics.Support != B.Politics.Support);
	CHECK_INT(static_cast<long long>(A.Budget.Ledger.size()), 0, "source ledger untouched");
	CHECK_INT(static_cast<long long>(A.Rng.GetCounter("edl")), 0, "source rng untouched");
	CHECK_INT(static_cast<long long>(B.Rng.GetCounter("edl")), 1, "copy rng advanced");
}
