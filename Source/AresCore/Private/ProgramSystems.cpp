#include "AresCore/ProgramSystems.h"

#include <algorithm>

#include "AresCore/AresConstants.h"
#include "AresCore/AresData.h"
#include "AresCore/ProgramState.h"
#include "AresCore/SimClock.h"

namespace Ares
{
namespace
{
/** Whole program day. Both economy and politics guard on this. */
int64_t WholeDay(const FSimClock& Clock)
{
	return static_cast<int64_t>(std::floor(Clock.EarthDay));
}
} // namespace

bool HasMilestone(const FProgramState& State, const std::string& MilestoneKey)
{
	const auto& M = State.Research.Milestones;
	return std::find(M.begin(), M.end(), MilestoneKey) != M.end();
}

bool AwardRp(FProgramState& State, const std::string& MilestoneKey, FReal Amount)
{
	if (Amount == 0.0)
	{
		return false;
	}
	if (HasMilestone(State, MilestoneKey))
	{
		return false;
	}

	State.Research.Milestones.push_back(MilestoneKey);
	State.Research.Points += Amount;
	return true;
}

bool IsProgramInFlight(const FProgramState& State)
{
	if (!State.Missions.InTransit.empty())
	{
		return true;
	}
	// The reference keys off mission status strings on the planned list; a
	// committed-but-not-yet-launched stack still counts as "the program is
	// doing something", which is what support responds to.
	for (const FMissionRecord& Mission : State.Missions.Planned)
	{
		if (Mission.Phase == "go" || Mission.Phase == "in_flight")
		{
			return true;
		}
	}
	return false;
}

bool IsProgramLanded(const FProgramState& State)
{
	return !State.Colony.SiteId.empty();
}

void TickClockPhase(FProgramState& State, const FAresData& Data)
{
	const FClockAdvanceResult Result = Advance(State.Clock, Data);

	if (Result.bWindowOpened)
	{
		// Keyed by window index so a timewarp that crosses two windows pays
		// twice, and a re-tick of the same day pays nothing.
		AwardRp(State, "window_" + std::to_string(State.Clock.WindowIndex), WINDOW_OPEN_RP);
	}
}

void TickResearchPhase(FProgramState& State, const FAresData& Data)
{
	for (FLeoLab& Lab : State.Research.Labs)
	{
		if (Lab.bOffline)
		{
			continue;
		}

		if (Lab.Crew > 0)
		{
			// Consumption rates come from balance.json's human block, not from
			// literals — these are exactly the numbers brief §9 means.
			const FReal Heads = static_cast<FReal>(Lab.Crew);
			Lab.FoodKg -= Data.Human.FoodDryKgPerSol * Heads;
			Lab.WaterKg -= Data.Human.WaterDrinkKgPerSol * Heads;
			Lab.AirDays -= LAB_AIR_DAYS_PER_DAY;

			if (Lab.FoodKg <= 0.0 || Lab.WaterKg <= 0.0 || Lab.AirDays <= 0.0)
			{
				// Optimus is fine; the humans are not.
				Lab.bOffline = true;
				Lab.FoodKg = std::max(0.0, Lab.FoodKg);
				Lab.WaterKg = std::max(0.0, Lab.WaterKg);
				Lab.AirDays = std::max(0.0, Lab.AirDays);
				continue;
			}
		}

		State.Research.Points += (Lab.RpPerDay > 0.0) ? Lab.RpPerDay : LAB_RP_PER_DAY_ROBOTIC;
	}
}

void TickEconomy(FProgramState& State, const FAresData& Data)
{
	(void)Data;

	FBudgetState& Budget = State.Budget;
	if (Budget.AnnualUsd == 0.0)
	{
		return;
	}

	const int64_t Day = WholeDay(State.Clock);
	if (Day == Budget.LastOpsDay)
	{
		return;
	}
	Budget.LastOpsDay = Day;

	// Quiet ops burn, so idle years still cost something.
	Budget.RemainingUsd =
		std::max(0.0, Budget.RemainingUsd - Budget.AnnualUsd * OPS_BURN_FRACTION_PER_DAY);

	if (Day > 0 && Day % QUARTER_DAYS == 0)
	{
		Budget.RemainingUsd += Budget.AnnualUsd / 4.0;

		// Reserves above the cap are not banked — they are evidence you are
		// sitting on public money, and politics charges you for it.
		const FReal Cap = Budget.AnnualUsd * HOARD_CAP_MULTIPLE;
		if (Budget.RemainingUsd > Cap)
		{
			Budget.RemainingUsd = Cap;
			Budget.bHoarded = true;
		}
		else
		{
			Budget.bHoarded = false;
		}
	}
}

void TickPolitics(FProgramState& State, const FAresData& Data)
{
	(void)Data;

	FPoliticsState& Politics = State.Politics;

	const int64_t Day = WholeDay(State.Clock);
	if (Politics.LastDay == Day)
	{
		return;
	}
	Politics.LastDay = Day;

	const bool bLanded = IsProgramLanded(State);
	const bool bInFlight = IsProgramInFlight(State);

	FReal Drift = SUPPORT_DRIFT_IDLE;
	if (bLanded)
	{
		Drift = SUPPORT_DRIFT_LANDED;
	}
	else if (bInFlight)
	{
		Drift = SUPPORT_DRIFT_IN_FLIGHT;
	}

	if (State.Budget.bHoarded)
	{
		Drift += SUPPORT_HOARD_PENALTY;
	}

	Politics.Support = Clamp(Politics.Support + Drift, 0.0, 100.0);

	if (Politics.Support < SUPPORT_CANCEL_THRESHOLD)
	{
		// Counted per quarter, not per day: the threshold is about sustained
		// political failure, not a bad afternoon.
		if (Day % QUARTER_DAYS == 0)
		{
			Politics.QuartersUnderThreshold += 1;
		}
	}
	else
	{
		Politics.QuartersUnderThreshold = 0;
	}

	if (Politics.QuartersUnderThreshold >= SUPPORT_CANCEL_QUARTERS && !Politics.bCancelled)
	{
		Politics.bCancelled = true;
		// A program with boots on the ground is not cancelled, it is abandoned —
		// the funding stops but the people are still there.
		Politics.CancellationReason = bLanded ? "abandonment" : "cancelled";
	}
}

void TickProgramDay(FProgramState& State, const FAresData& Data)
{
	// Order from pipeline.js TICK_PHASES. Economy must precede politics: the
	// hoarding flag economy sets is read by politics the same day.
	TickClockPhase(State, Data);
	TickResearchPhase(State, Data);
	TickEconomy(State, Data);
	TickPolitics(State, Data);
}

int32_t TickProgramDays(FProgramState& State, const FAresData& Data, int32_t Days)
{
	int32_t Ran = 0;
	for (int32_t I = 0; I < Days; ++I)
	{
		TickProgramDay(State, Data);
		++Ran;

		// Timewarping past the end of the program would be a strange thing to
		// let happen silently; the caller gets to see where it stopped.
		if (State.Politics.bCancelled)
		{
			break;
		}
	}
	return Ran;
}

} // namespace Ares
