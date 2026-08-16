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
