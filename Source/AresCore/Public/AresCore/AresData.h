// AresCore — immutable loaded content (brief §3.1: FAresData).
//
// Loaded once at startup from data/*.json and never mutated afterwards. Every
// balance number the simulation uses comes from here, so a designer can retune
// the game without a recompile.
//
// M0 loads only the time/orbit/economy/EDL slices that FSimClock and
// FProgramState need. M2 extends this with the cargo, tech, sites, contractors,
// resources and roles tables and the DataTable importers in AresEditor. The
// struct is laid out so that growth is additive.

#pragma once

#include <string>
#include <vector>

#include "AresCore/AresMath.h"

namespace Ares
{

class FJsonValue;

/** balance.json -> "time" */
struct FTimeBalance
{
	FReal SolSeconds = 0.0;
	FReal EarthDaySeconds = 0.0;
	FReal MarsYearSols = 0.0;
	FReal MarsYearEarthDays = 0.0;
	FReal SynodicEarthDays = 0.0;
	FReal FirstWindowEarthDay = 0.0;

	FReal SolsPerEarthDay() const { return EarthDaySeconds / SolSeconds; }
	FReal EarthDaysPerSol() const { return SolSeconds / EarthDaySeconds; }
};

/** balance.json -> "orbit" */
struct FOrbitBalance
{
	FReal Eccentricity = 0.0;
	FReal AxialTiltDeg = 0.0;
	FReal LsPerihelionDeg = 0.0;
	FReal SolarConstantMeanWm2 = 0.0;
	FReal GravityMps2 = 0.0;
	FReal RadiusKm = 0.0;
	FReal SurfacePressurePa = 0.0;
};

/** balance.json -> "economy" */
struct FEconomyBalance
{
	FReal StartingAnnualDirector = 0.0;
	FReal StartingAnnualAdministrator = 0.0;
	FReal StartingAnnualAusterity = 0.0;
	FReal StartingAnnualIronman = 0.0;
	FReal StartingSupport = 0.0;
	FReal StarshipLaunchUsd = 0.0;
	FReal BoosterRefuelUsd = 0.0;
	FReal StarshipPayloadKg = 0.0;
	FReal StarshipPayloadM3 = 0.0;
	FReal RpBuyUsd = 0.0;

	/** Difficulty key as spelled in balance.json: DIRECTOR/ADMINISTRATOR/AUSTERITY/IRONMAN. */
	FReal StartingAnnualFor(const std::string& Difficulty) const;
};

/** balance.json -> "edl" */
struct FEdlBalance
{
	FReal EarlySuccess = 0.0;
	FReal MatureSuccess = 0.0;
	std::vector<std::string> StageIds;
	std::vector<std::string> StageLabels;
};

/**
 * balance.json -> "deltaV_mps".
 *
 * NOTE (brief §5.1): the shipped balance.json carries only the Mars legs. The
 * five lunar legs the brief specifies (leoToTli, tliToLlo, lloToSurface,
 * surfaceToLlo, lloToTei) are NOT present in the data file, so they load as
 * zero and IsLunarComplete() reports false. M5 adds them to the data file
 * rather than hardcoding them here.
 */
struct FDeltaVBalance
{
	FReal EarthToLeo = 0.0;
	FReal LeoToTmi = 0.0;
	FReal TmiToCapturePropulsive = 0.0;
	FReal TmiToAerocapture = 0.0;
	FReal OrbitToSurfaceEdl = 0.0;
	FReal SurfaceToLmo = 0.0;
	FReal LmoToTei = 0.0;

	FReal LeoToTli = 0.0;
	FReal TliToLlo = 0.0;
	FReal LloToSurface = 0.0;
	FReal SurfaceToLlo = 0.0;
	FReal LloToTei = 0.0;

	bool IsLunarComplete() const
	{
		return LeoToTli > 0.0 && TliToLlo > 0.0 && LloToSurface > 0.0
			&& SurfaceToLlo > 0.0 && LloToTei > 0.0;
	}
};

/** balance.json -> "cabin". Ship interior blockout spec (brief §4.3). */
struct FCabinBalance
{
	FReal InnerRadiusM = 0.0;
	FReal DeckHeightM = 0.0;
	int32_t Decks = 0;
	int32_t Seats = 0;
};

struct FAresData
{
	std::string Version;
	FTimeBalance Time;
	FOrbitBalance Orbit;
	FEconomyBalance Economy;
	FEdlBalance Edl;
	FDeltaVBalance DeltaV;
	FCabinBalance Cabin;

	/**
	 * Loads and validates balance.json. Returns false with OutError set when a
	 * required field is missing — content problems must fail at load, not
	 * silently balance the game to zero.
	 */
	static bool LoadBalance(const FJsonValue& Root, FAresData& Out, std::string& OutError);
	static bool LoadBalanceFile(const std::string& Path, FAresData& Out, std::string& OutError);
};

} // namespace Ares
