// AresCore — constants that are NOT in data/*.json.
//
// Brief §9: "if a number appears in data/*.json, read it from the DataTable.
// Never duplicate a balance constant in C++." Everything that balance.json
// defines (sol length, Mars year, synodic period, eccentricity, perihelion Ls,
// solar constant, first window) is therefore loaded through FAresData, NOT
// declared here.
//
// The reference's src/core/constants.js duplicates those balance numbers in
// code; its own header comment concedes the debt ("B2 will fold these into
// /data/balance.json"). We do not repeat that.
//
// What remains below are the campaign calendar pins, which appear nowhere in
// data/*.json and have no balance meaning — they fix where program Year 0 sits
// on the civil calendar and which Mars year that corresponds to.

#pragma once

#include <cstdint>

namespace Ares
{

/** Civil epoch for program Year 0: 2040-01-01 UTC. */
inline constexpr int32_t EPOCH_YEAR = 2040;
inline constexpr uint32_t EPOCH_MONTH = 1;
inline constexpr uint32_t EPOCH_DAY = 1;

/**
 * MY46 Ls=0 falls on 2039-11-30. Pinning the dual calendar here puts Earth
 * 2040-01-01 a few weeks into Mars northern spring, which is what the
 * reference's opening state assumes.
 */
inline constexpr int32_t LS0_YEAR = 2039;
inline constexpr uint32_t LS0_MONTH = 11;
inline constexpr uint32_t LS0_DAY = 30;
inline constexpr int32_t MARS_YEAR_AT_LS0 = 46;

/* ---------------------------------------------------------------------------
 * Tuning constants that are NOT in any data table.
 *
 * These live in the reference's sim modules as literals and have no home in
 * balance.json. Brief §9 only forbids duplicating a number that IS in the data;
 * these are not, so they sit here rather than being scattered through the tick
 * functions. They SHOULD migrate into balance.json — see docs/PORT_ASSESSMENT.md
 * — at which point they move to FAresData and these go away.
 * ------------------------------------------------------------------------- */

/** economy.js: quiet daily burn, as a fraction of the annual appropriation. */
inline constexpr double OPS_BURN_FRACTION_PER_DAY = 0.00018;

/** economy.js / politics.js: a program quarter, in Earth days. */
inline constexpr int64_t QUARTER_DAYS = 91;

/** economy.js: reserves above this multiple of annual count as hoarding. */
inline constexpr double HOARD_CAP_MULTIPLE = 2.0;

/** politics.js: daily support drift by program posture. */
inline constexpr double SUPPORT_DRIFT_IDLE = -0.012;
inline constexpr double SUPPORT_DRIFT_IN_FLIGHT = -0.004;
inline constexpr double SUPPORT_DRIFT_LANDED = 0.004;

/** politics.js: additional daily penalty while hoarding. */
inline constexpr double SUPPORT_HOARD_PENALTY = -0.03;

/** politics.js: below this for CANCEL_QUARTERS quarters ends the program. */
inline constexpr double SUPPORT_CANCEL_THRESHOLD = 20.0;
inline constexpr int32_t SUPPORT_CANCEL_QUARTERS = 4;

/** pipeline.js clock phase: RP awarded when a transfer window opens. */
inline constexpr double WINDOW_OPEN_RP = 25.0;

/** research.js: LEO lab output. Crew beats robots, but robots do not eat. */
inline constexpr double LAB_RP_PER_DAY_ROBOTIC = 3.0;
inline constexpr double LAB_RP_PER_DAY_CREWED = 4.0;

/** research.js: a crewed lab goes offline below any of these. */
inline constexpr double LAB_AIR_DAYS_PER_DAY = 1.0;

/** Timewarp rates offered in Mission Control (brief §5, reference SPEEDS). */
enum class ESimSpeed : uint8_t
{
	Pause = 0,
	X1 = 1,
	X3 = 3,
	X10 = 10,
	X30 = 30,
};

/**
 * Program time advances in whole days; Mars-surface operations tick in sols.
 * Brief §3.1: two rates, one clock, explicit conversion.
 */
enum class ETickUnit : uint8_t
{
	EarthDay,
	Sol,
};

} // namespace Ares
