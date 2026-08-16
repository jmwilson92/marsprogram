// Ports of ../reference src/util/math.js, checked against the JS oracle.

#include "AresCore/AresMath.h"

#include "Harness/AresTest.h"
#include "OracleFixture.h"

using namespace Ares;

namespace
{
/**
 * Transcendental tolerance. sin/cos/tan/atan come from libm, which is not
 * bit-identical across platforms or even across glibc versions, so anything
 * routed through them is compared relatively rather than exactly. The RNG,
 * which is pure 32-bit integer work, is held to exact equality elsewhere.
 */
constexpr double TRIG_TOL = 1e-12;
} // namespace

ARES_TEST(Math, WrapDegMatchesReference)
{
	const auto& Cases = AresTest::Oracle()["math"]["wrapDeg"];
	CHECK_TRUE(Cases.Num() > 0);
	for (size_t I = 0; I < Cases.Num(); ++I)
	{
		const double In = Cases[I]["in"].AsNumber();
		const double Expected = Cases[I]["out"].AsNumber();
		CHECK_EXACT(WrapDeg(In), Expected, "WrapDeg");
	}
}

ARES_TEST(Math, WrapRadPiMatchesReference)
{
	const auto& Cases = AresTest::Oracle()["math"]["wrapRadPi"];
	CHECK_TRUE(Cases.Num() > 0);
	for (size_t I = 0; I < Cases.Num(); ++I)
	{
		const double In = Cases[I]["in"].AsNumber();
		const double Expected = Cases[I]["out"].AsNumber();
		CHECK_NEAR(WrapRadPi(In), Expected, TRIG_TOL, "WrapRadPi");
	}
}

ARES_TEST(Math, ClampAndLerpMatchReference)
{
	const auto& Clamps = AresTest::Oracle()["math"]["clamp"];
	for (size_t I = 0; I < Clamps.Num(); ++I)
	{
		const auto& In = Clamps[I]["in"];
		CHECK_EXACT(Clamp(In[(size_t)0].AsNumber(), In[(size_t)1].AsNumber(), In[(size_t)2].AsNumber()),
			Clamps[I]["out"].AsNumber(), "Clamp");
	}

	const auto& Lerps = AresTest::Oracle()["math"]["lerp"];
	for (size_t I = 0; I < Lerps.Num(); ++I)
	{
		const auto& In = Lerps[I]["in"];
		CHECK_EXACT(Lerp(In[(size_t)0].AsNumber(), In[(size_t)1].AsNumber(), In[(size_t)2].AsNumber()),
			Lerps[I]["out"].AsNumber(), "Lerp");
	}
}

ARES_TEST(Math, KeplerMeanToTrueMatchesReference)
{
	const auto& Cases = AresTest::Oracle()["math"]["meanToTrueAnomaly"];
	CHECK_TRUE(Cases.Num() > 0);
	for (size_t I = 0; I < Cases.Num(); ++I)
	{
		const double M = Cases[I]["in"][(size_t)0].AsNumber();
		const double Ecc = Cases[I]["in"][(size_t)1].AsNumber();
		CHECK_NEAR(MeanToTrueAnomaly(M, Ecc), Cases[I]["out"].AsNumber(), TRIG_TOL, "MeanToTrueAnomaly");
	}
}

ARES_TEST(Math, KeplerTrueToMeanMatchesReference)
{
	const auto& Cases = AresTest::Oracle()["math"]["trueToMeanAnomaly"];
	CHECK_TRUE(Cases.Num() > 0);
	for (size_t I = 0; I < Cases.Num(); ++I)
	{
		const double Nu = Cases[I]["in"][(size_t)0].AsNumber();
		const double Ecc = Cases[I]["in"][(size_t)1].AsNumber();
		CHECK_NEAR(TrueToMeanAnomaly(Nu, Ecc), Cases[I]["out"].AsNumber(), TRIG_TOL, "TrueToMeanAnomaly");
	}
}

ARES_TEST(Math, KeplerRoundTripIsStable)
{
	// Independent of the oracle: M -> nu -> M must return the input.
	const double Ecc = AresTest::Balance().Orbit.Eccentricity;
	for (double M = 0.0; M < 360.0; M += 7.5)
	{
		const double Nu = MeanToTrueAnomaly(M, Ecc);
		const double Back = TrueToMeanAnomaly(Nu, Ecc);
		CHECK_NEAR(Back, M, 1e-9, "Kepler round trip");
	}
}
