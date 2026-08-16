#include "AresCore/AresMath.h"

namespace Ares
{

FReal TrueToMeanAnomaly(FReal NuDeg, FReal Ecc)
{
	const FReal Nu = DegToRad(WrapDeg(NuDeg));
	const FReal Half = Nu / 2.0;
	const FReal E = 2.0 * std::atan(std::sqrt((1.0 - Ecc) / (1.0 + Ecc)) * std::tan(Half));
	const FReal M = E - Ecc * std::sin(E);
	return WrapDeg(RadToDeg(M));
}

FReal MeanToTrueAnomaly(FReal MDeg, FReal Ecc)
{
	const FReal M = WrapRadPi(DegToRad(MDeg));
	FReal E = M;
	for (int I = 0; I < 12; ++I)
	{
		const FReal dE = (E - Ecc * std::sin(E) - M) / (1.0 - Ecc * std::cos(E));
		E -= dE;
		if (std::fabs(dE) < 1e-14)
		{
			break;
		}
	}
	const FReal Nu = 2.0 * std::atan(std::sqrt((1.0 + Ecc) / (1.0 - Ecc)) * std::tan(E / 2.0));
	return WrapDeg(RadToDeg(Nu));
}

} // namespace Ares
