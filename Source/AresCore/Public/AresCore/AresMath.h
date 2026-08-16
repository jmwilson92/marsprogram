// AresCore — pure simulation math.
//
// HARD RULE (brief §9): this module never includes Engine.h, and in fact never
// includes any Unreal header at all. It is standard C++20 so it compiles and
// unit-tests headless, outside the editor, on any toolchain.
//
// Ported from ../reference src/util/math.js. Semantics are matched exactly,
// including JavaScript's truncated fmod behaviour for the wrap helpers.

#pragma once

#include <cmath>

#include "AresCore/AresApi.h"

namespace Ares
{
/** Program math runs in double throughout; §3.3's astro frame needs the range. */
using FReal = double;

inline constexpr FReal PI_ = 3.14159265358979323846;
inline constexpr FReal TAU_ = 2.0 * PI_;

inline FReal Clamp(FReal X, FReal Lo, FReal Hi)
{
	return X < Lo ? Lo : (X > Hi ? Hi : X);
}

inline FReal Lerp(FReal A, FReal B, FReal T)
{
	return A + (B - A) * T;
}

inline FReal DegToRad(FReal D)
{
	return (D * PI_) / 180.0;
}

inline FReal RadToDeg(FReal R)
{
	return (R * 180.0) / PI_;
}

/** Wrap to [0, 360). Mirrors JS ((d % 360) + 360) % 360 — std::fmod matches JS %. */
inline FReal WrapDeg(FReal D)
{
	return std::fmod(std::fmod(D, 360.0) + 360.0, 360.0);
}

/** Wrap to (-PI, PI]. */
inline FReal WrapRadPi(FReal A)
{
	FReal X = std::fmod(std::fmod(A, TAU_) + TAU_, TAU_);
	if (X > PI_)
	{
		X -= TAU_;
	}
	return X;
}

/**
 * Kepler: true anomaly (deg) -> mean anomaly (deg).
 * tan(nu/2) is undefined at 180 deg; the perihelion-frame Mars never sits
 * exactly there at Ls0, which is the only call site in the clock.
 */
ARESCORE_API FReal TrueToMeanAnomaly(FReal NuDeg, FReal Ecc);

/**
 * Kepler: mean anomaly (deg) -> true anomaly (deg).
 * Newton iteration on the eccentric anomaly, 12 steps max, 1e-14 convergence —
 * identical iteration count and tolerance to the reference so results agree.
 */
ARESCORE_API FReal MeanToTrueAnomaly(FReal MDeg, FReal Ecc);

} // namespace Ares
