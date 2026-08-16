// AresCore — the program simulation (brief §7).
//
// Each system is a free function taking (FProgramState&, const FAresData&).
// No hidden state, no singletons, no member functions on a god object — brief
// §3.1. That is also what makes them trivially testable headless: build a
// state, tick it, compare against the JS reference.
//
// Ported from ../reference src/sim/economy.js, politics.js and research.js.
// The tick ORDER is not incidental: it comes from src/sim/pipeline.js
// TICK_PHASES, and economy must run before politics because politics reads the
// hoarding flag economy sets that same day.

#pragma once

#include <string>

#include "AresCore/AresApi.h"
#include "AresCore/AresMath.h"

namespace Ares
{

struct FAresData;
struct FProgramState;

/**
 * Awards research points against a named milestone, once ever.
 *
 * Brief §7: "RP earned only from flown milestones". The key is what makes it
 * once-only — re-awarding the same milestone is a no-op, so a system can call
 * this every tick without bookkeeping. Returns true if it actually paid out.
 */
ARESCORE_API bool AwardRp(FProgramState& State, const std::string& MilestoneKey, FReal Amount);

/** True when a milestone has already been paid. */
ARESCORE_API bool HasMilestone(const FProgramState& State, const std::string& MilestoneKey);

/**
 * Clock phase. Advances one day and pays the transfer-window milestone.
 * Runs FIRST — every other system reads the day it sets.
 */
ARESCORE_API void TickClockPhase(FProgramState& State, const FAresData& Data);

/** Research phase. LEO labs produce RP and crewed ones consume stores. */
ARESCORE_API void TickResearchPhase(FProgramState& State, const FAresData& Data);

/**
 * Economy phase. A quiet daily ops burn, plus a quarterly appropriation capped
 * at twice annual — exceeding the cap sets the hoarding flag that politics
 * punishes. Idle years still cost money; that is the point.
 */
ARESCORE_API void TickEconomy(FProgramState& State, const FAresData& Data);

/**
 * Politics phase. Support drifts by posture, hoarding accelerates the decline,
 * and four quarters below the threshold ends the program.
 */
ARESCORE_API void TickPolitics(FProgramState& State, const FAresData& Data);

/**
 * One whole program day, in the reference's pipeline order.
 * This is what UProgramSubsystem calls; the individual phases are exposed for
 * tests and for the day-batching path.
 */
ARESCORE_API void TickProgramDay(FProgramState& State, const FAresData& Data);

/**
 * Advances many days. Used by timewarp and by the background task that runs
 * long skips off the game thread (brief §3.2). Stops early if the program is
 * cancelled, returning the number of days actually run.
 */
ARESCORE_API int32_t TickProgramDays(FProgramState& State, const FAresData& Data, int32_t Days);

/** True when any mission is committed or flying. Politics reads this. */
ARESCORE_API bool IsProgramInFlight(const FProgramState& State);

/** True once a surface site is occupied. */
ARESCORE_API bool IsProgramLanded(const FProgramState& State);

} // namespace Ares
