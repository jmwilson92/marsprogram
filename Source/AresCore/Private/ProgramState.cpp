#include "AresCore/ProgramState.h"

#include <cstring>

#include "AresCore/AresData.h"

namespace Ares
{
namespace
{
/* FNV-1a 64. Cheap, stable, and dependency-free — this is a change detector, not a MAC. */
constexpr uint64_t FNV_OFFSET = 1469598103934665603ull;
constexpr uint64_t FNV_PRIME = 1099511628211ull;

struct FHasher
{
	uint64_t Value = FNV_OFFSET;

	void Bytes(const void* Data, size_t Size)
	{
		const auto* P = static_cast<const unsigned char*>(Data);
		for (size_t I = 0; I < Size; ++I)
		{
			Value ^= P[I];
			Value *= FNV_PRIME;
		}
	}

	void U64(uint64_t X) { Bytes(&X, sizeof(X)); }
	void I64(int64_t X) { Bytes(&X, sizeof(X)); }
	void I32(int32_t X) { Bytes(&X, sizeof(X)); }
	void U32(uint32_t X) { Bytes(&X, sizeof(X)); }
	void Bool(bool X) { const uint8_t B = X ? 1u : 0u; Bytes(&B, 1); }

	/**
	 * Hash the exact bit pattern, but fold -0.0 to +0.0 first. They compare
	 * equal numerically, so treating them as different states would report
	 * false determinism failures.
	 */
	void Real(FReal X)
	{
		if (X == 0.0) { X = 0.0; }
		uint64_t Bits;
		std::memcpy(&Bits, &X, sizeof(Bits));
		U64(Bits);
	}

	void Str(const std::string& S)
	{
		U64(static_cast<uint64_t>(S.size()));
		Bytes(S.data(), S.size());
	}
};

void HashClock(FHasher& H, const FSimClock& C)
{
	H.Real(C.EarthDay);
	H.Real(C.Sol);
	H.I32(C.MarsYear);
	H.Real(C.SolOfYear);
	H.Real(C.Ls);
	H.I64(C.TickCount);
	H.U32(static_cast<uint32_t>(C.Speed));
	H.U32(static_cast<uint32_t>(C.TickUnit));
	H.Real(C.InsolationFactor);
	H.Real(C.SolarConstantWm2);
	H.I32(C.WindowIndex);
	H.Real(C.NextWindowEarthDay);
	H.Real(C.DaysToWindow);
	H.Bool(C.bWindowJustOpened);
}

void HashRng(FHasher& H, const FAresRng& Rng)
{
	H.U32(Rng.GetRootSeed());
	// std::map iterates in key order, so the digest does not depend on the
	// order streams happened to be created in.
	const auto Streams = Rng.Serialize();
	H.U64(static_cast<uint64_t>(Streams.size()));
	for (const auto& [Name, Slot] : Streams)
	{
		H.Str(Name);
		H.U32(Slot.Seed);
		H.U32(Slot.State);
		H.U64(Slot.Counter);
	}
}

void HashHull(FHasher& H, const FVehicleHull& V)
{
	H.Str(V.Id);
	H.Str(V.Name);
	H.Str(V.Variant);
	H.Str(V.Location);
	H.Real(V.PropellantFraction);
	H.I32(V.ReuseCount);
	H.Real(V.Wear);
	H.Bool(V.bLost);
}

void HashMissions(FHasher& H, const std::vector<FMissionRecord>& Rows)
{
	H.U64(static_cast<uint64_t>(Rows.size()));
	for (const auto& M : Rows)
	{
		H.Str(M.Id);
		H.Str(M.Type);
		H.Str(M.ShipId);
		H.Str(M.SiteId);
		H.Real(M.CommitEarthDay);
		H.Real(M.ArrivalEarthDay);
		H.Real(M.DeltaVBudgetMps);
		H.Real(M.PropellantMarginFraction);
		H.Str(M.Phase);
		H.Bool(M.bCrewed);
	}
}

void HashCrew(FHasher& H, const std::vector<FCrewMember>& Rows)
{
	H.U64(static_cast<uint64_t>(Rows.size()));
	for (const auto& C : Rows)
	{
		H.Str(C.Id);
		H.Str(C.Name);
		H.Str(C.RoleId);
		H.Str(C.Location);
		H.Real(C.DoseMSv);
		H.Real(C.Morale);
		H.Bool(C.bAlive);
	}
}
} // namespace

FProgramState MakeProgramState(const FAresData& Data, uint32_t Seed, const std::string& Difficulty)
{
	FProgramState State;

	State.Meta.Seed = Seed;
	State.Meta.SchemaVersion = ARES_SAVE_SCHEMA_VERSION;
	State.Meta.Difficulty = Difficulty;
	State.Meta.PlaytimeSeconds = 0.0;

	State.Clock = MakeClock(Data);
	State.Rng = FAresRng(Seed);

	const FReal Annual = Data.Economy.StartingAnnualFor(Difficulty);
	State.Budget.AnnualUsd = Annual;
	State.Budget.RemainingUsd = Annual;
	State.Budget.bHoarded = false;

	State.Politics.Support = Data.Economy.StartingSupport;
	State.Politics.QuartersUnderThreshold = 0;
	State.Politics.bCancelled = false;

	State.Research.Points = 0.0;

	// Starter tech, mirroring the reference's applyDataToState: rows flagged
	// startCompleted are already researched, and carry their starting maturity.
	// Maturity is what scales EDL success between balance.json's earlySuccess
	// and matureSuccess, so a fresh program is not flying blind.
	for (const FTechRow& Row : Data.Tech)
	{
		if (!Row.bStartCompleted)
		{
			continue;
		}
		State.Research.Completed.push_back(Row.Id);
		State.Research.Maturity[Row.Id] = Row.StartingMaturity;
	}

	State.Colony.bLifeSupportPowered = true;

	return State;
}

uint64_t HashProgramState(const FProgramState& State)
{
	FHasher H;

	H.U32(State.Meta.Seed);
	H.I32(State.Meta.SchemaVersion);
	H.Str(State.Meta.Difficulty);
	// PlaytimeSeconds is wall-clock, not simulation state — deliberately excluded
	// so a determinism run is not sensitive to how fast the machine executed it.

	HashClock(H, State.Clock);
	HashRng(H, State.Rng);

	H.Real(State.Budget.AnnualUsd);
	H.Real(State.Budget.RemainingUsd);
	H.Real(State.Budget.Allocation.Rd);
	H.Real(State.Budget.Allocation.Mfg);
	H.Real(State.Budget.Allocation.Ops);
	H.Real(State.Budget.Allocation.Outreach);
	H.Real(State.Budget.Allocation.Reserve);
	H.Bool(State.Budget.bHoarded);
	H.U64(static_cast<uint64_t>(State.Budget.Ledger.size()));
	for (const auto& E : State.Budget.Ledger)
	{
		H.Real(E.EarthDay);
		H.Str(E.Category);
		H.Str(E.Note);
		H.Real(E.AmountUsd);
	}

	H.Real(State.Politics.Support);
	H.Str(State.Politics.Administration);
	H.Str(State.Politics.Mandate);
	H.I32(State.Politics.QuartersUnderThreshold);
	H.Bool(State.Politics.bCancelled);
	H.Str(State.Politics.CancellationReason);

	H.Real(State.Research.Points);
	H.U64(static_cast<uint64_t>(State.Research.Completed.size()));
	for (const auto& Id : State.Research.Completed) { H.Str(Id); }
	H.Bool(State.Research.bHasActive);
	H.Str(State.Research.Active.TechId);
	H.Real(State.Research.Active.DaysRemaining);
	H.Real(State.Research.Active.PointsCommitted);
	H.U64(static_cast<uint64_t>(State.Research.Maturity.size()));
	for (const auto& [Id, M] : State.Research.Maturity) { H.Str(Id); H.Real(M); }
	H.U64(static_cast<uint64_t>(State.Research.Milestones.size()));
	for (const auto& Id : State.Research.Milestones) { H.Str(Id); }

	H.U64(static_cast<uint64_t>(State.Fleet.Ships.size()));
	for (const auto& V : State.Fleet.Ships) { HashHull(H, V); }
	H.U64(static_cast<uint64_t>(State.Fleet.Boosters.size()));
	for (const auto& V : State.Fleet.Boosters) { HashHull(H, V); }
	H.Real(State.Fleet.GroundedUntilEarthDay);

	HashMissions(H, State.Missions.Planned);
	HashMissions(H, State.Missions.InTransit);
	HashMissions(H, State.Missions.Completed);
	HashMissions(H, State.Missions.Failed);

	HashCrew(H, State.Crew.Roster);
	HashCrew(H, State.Crew.Lost);
	HashCrew(H, State.Crew.Candidates);

	H.Str(State.Colony.SiteId);
	H.Real(State.Colony.PowerGenerationKw);
	H.Real(State.Colony.PowerDemandKw);
	H.Real(State.Colony.StorageKwh);
	H.I32(State.Colony.DeficitSols);
	H.Bool(State.Colony.bLifeSupportPowered);
	H.U64(static_cast<uint64_t>(State.Colony.Resources.size()));
	for (const auto& [Id, Amount] : State.Colony.Resources) { H.Str(Id); H.Real(Amount); }

	H.U64(static_cast<uint64_t>(State.Contracts.Contracts.size()));
	for (const auto& C : State.Contracts.Contracts)
	{
		H.Str(C.Id);
		H.Str(C.ContractorId);
		H.U32(static_cast<uint32_t>(C.Kind));
		H.Real(C.ValueUsd);
		H.Real(C.PaidUsd);
		H.Real(C.OverrunFraction);
		H.U64(static_cast<uint64_t>(C.MilestonesPaid.size()));
		for (const auto& Id : C.MilestonesPaid) { H.Str(Id); }
		H.Bool(C.bActive);
	}
	H.U64(static_cast<uint64_t>(State.Contracts.Relationships.size()));
	for (const auto& [Id, R] : State.Contracts.Relationships) { H.Str(Id); H.Real(R); }

	return H.Value;
}

} // namespace Ares
