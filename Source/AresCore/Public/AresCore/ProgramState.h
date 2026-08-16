// AresCore — FProgramState (brief §3.1).
//
// The root state struct, split into per-system sub-structs. The reference keeps
// one giant mutable blob in src/core/state.js (createInitialState returns a
// single nested object every system reaches into); the brief calls that a
// mistake and this does not repeat it. Each sub-struct is owned by exactly one
// system's tick function.
//
// M0 defines the shapes and the identity/clock/RNG spine. The per-system tick
// functions (TickPolitics, TickBudget, TickResearch, ...) land in M2, as free
// functions taking (FProgramState&, const FAresData&) — no hidden state, no
// singletons inside AresCore.

#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "AresCore/AresApi.h"
#include "AresCore/AresMath.h"
#include "AresCore/AresRng.h"
#include "AresCore/SimClock.h"

namespace Ares
{

struct FAresData;

/** Bumped whenever a save-breaking field change lands. Brief §8/M2 wants this explicit. */
inline constexpr int32_t ARES_SAVE_SCHEMA_VERSION = 1;

/** Difficulty keys as spelled in balance.json's economy.startingAnnual. */
inline constexpr const char* DIFFICULTY_DIRECTOR = "DIRECTOR";
inline constexpr const char* DIFFICULTY_ADMINISTRATOR = "ADMINISTRATOR";
inline constexpr const char* DIFFICULTY_AUSTERITY = "AUSTERITY";
inline constexpr const char* DIFFICULTY_IRONMAN = "IRONMAN";

/* ------------------------------------------------------------------ budget */

/** One debit or credit. The reference keeps a full ledger and so do we. */
struct FLedgerEntry
{
	FReal EarthDay = 0.0;
	std::string Category;
	std::string Note;
	FReal AmountUsd = 0.0;
};

struct FBudgetAllocation
{
	FReal Rd = 0.0;
	FReal Mfg = 0.0;
	FReal Ops = 0.0;
	FReal Outreach = 0.0;
	FReal Reserve = 0.0;
};

struct FBudgetState
{
	FReal AnnualUsd = 0.0;
	FReal RemainingUsd = 0.0;
	FBudgetAllocation Allocation;
	std::vector<FLedgerEntry> Ledger;
	/** Set when reserves exceed 2x annual; politics reads it as a hoarding penalty. */
	bool bHoarded = false;
};

/* ---------------------------------------------------------------- politics */

struct FPoliticsState
{
	/** 0-100 public/congressional support. Program dies below 20 for four quarters. */
	FReal Support = 0.0;
	std::string Administration;
	std::string Mandate;
	int32_t QuartersUnderThreshold = 0;
	bool bCancelled = false;
	std::string CancellationReason;
};

/* ---------------------------------------------------------------- research */

struct FResearchActive
{
	std::string TechId;
	/** Minimum calendar duration: brief §7 — you cannot buy time. */
	FReal DaysRemaining = 0.0;
	FReal PointsCommitted = 0.0;
};

struct FResearchState
{
	FReal Points = 0.0;
	std::vector<std::string> Completed;
	FResearchActive Active;
	bool bHasActive = false;
	/** Tech id -> maturity 0..1. Scales EDL success and reliability. */
	std::map<std::string, FReal> Maturity;
	/** Flown milestones already awarded, so RP is never double-paid. */
	std::vector<std::string> Milestones;
};

/* ------------------------------------------------------------------- fleet */

/** An individual hull with a name and a service history (brief §7). */
struct FVehicleHull
{
	std::string Id;
	std::string Name;
	std::string Variant;   // tanker / crew / cargo / robotic
	std::string Location;  // pad / leo / transit / surface / lost
	FReal PropellantFraction = 0.0;
	int32_t ReuseCount = 0;
	FReal Wear = 0.0;
	bool bLost = false;
};

struct FFleetState
{
	std::vector<FVehicleHull> Ships;
	std::vector<FVehicleHull> Boosters;
	/** Set by an accident review board; blocks launch commit while non-zero. */
	FReal GroundedUntilEarthDay = 0.0;
};

/* ----------------------------------------------------------------- mission */

struct FMissionRecord
{
	std::string Id;
	std::string Type;
	std::string ShipId;
	std::string SiteId;
	FReal CommitEarthDay = 0.0;
	FReal ArrivalEarthDay = 0.0;
	FReal DeltaVBudgetMps = 0.0;
	FReal PropellantMarginFraction = 0.0;
	std::string Phase;
	bool bCrewed = false;
};

struct FMissionState
{
	std::vector<FMissionRecord> Planned;
	std::vector<FMissionRecord> InTransit;
	std::vector<FMissionRecord> Completed;
	std::vector<FMissionRecord> Failed;
};

/* -------------------------------------------------------------------- crew */

struct FCrewMember
{
	std::string Id;
	std::string Name;
	std::string RoleId;
	std::string Location;
	/** Against the 1000 mSv career limit in balance.json human.careerDose_mSv. */
	FReal DoseMSv = 0.0;
	FReal Morale = 0.0;
	bool bAlive = true;
};

struct FCrewState
{
	std::vector<FCrewMember> Roster;
	std::vector<FCrewMember> Lost;
	std::vector<FCrewMember> Candidates;
};

/* ------------------------------------------------------------------ colony */

/**
 * Mars end-game. Brief §7 is explicit that this is NOT milestone-critical and
 * ports in M6, so the shape is a placeholder the colony tick will fill.
 */
struct FColonyState
{
	std::string SiteId;
	FReal PowerGenerationKw = 0.0;
	FReal PowerDemandKw = 0.0;
	FReal StorageKwh = 0.0;
	int32_t DeficitSols = 0;
	bool bLifeSupportPowered = true;
	std::map<std::string, FReal> Resources;
};

/* --------------------------------------------------------------- contracts */

/** New in this project; the reference has contractors but no contract vehicle. */
enum class EContractKind : uint8_t
{
	CostPlus,
	FixedPrice,
};

struct FContract
{
	std::string Id;
	std::string ContractorId;
	EContractKind Kind = EContractKind::CostPlus;
	FReal ValueUsd = 0.0;
	FReal PaidUsd = 0.0;
	FReal OverrunFraction = 0.0;
	std::vector<std::string> MilestonesPaid;
	bool bActive = true;
};

struct FContractState
{
	std::vector<FContract> Contracts;
	/** Contractor id -> standing, moved by awards and snubs. */
	std::map<std::string, FReal> Relationships;
};

/* ------------------------------------------------------------------- state */

struct FProgramMeta
{
	uint32_t Seed = 0;
	int32_t SchemaVersion = ARES_SAVE_SCHEMA_VERSION;
	std::string Difficulty = DIFFICULTY_ADMINISTRATOR;
	FReal PlaytimeSeconds = 0.0;
};

struct FProgramState
{
	FProgramMeta Meta;
	FSimClock Clock;
	FAresRng Rng;

	FBudgetState Budget;
	FPoliticsState Politics;
	FResearchState Research;
	FFleetState Fleet;
	FMissionState Missions;
	FCrewState Crew;
	FColonyState Colony;
	FContractState Contracts;
};

/**
 * Builds the opening state: clock at day 0, RNG seeded, budget and support
 * taken from balance.json for the chosen difficulty.
 */
ARESCORE_API FProgramState MakeProgramState(const FAresData& Data, uint32_t Seed,
	const std::string& Difficulty = DIFFICULTY_ADMINISTRATOR);

/**
 * Order-independent-free, stable hash of everything that affects the run.
 * Brief §9: a determinism test runs 2000 simulated days twice from the same
 * seed and asserts identical hashes. Doubles are hashed by their exact bit
 * pattern, so any drift shows up rather than being rounded away.
 */
ARESCORE_API uint64_t HashProgramState(const FProgramState& State);

} // namespace Ares
