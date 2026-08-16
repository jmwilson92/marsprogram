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

#include "AresCore/AresApi.h"
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
	ARESCORE_API FReal StartingAnnualFor(const std::string& Difficulty) const;
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

/** balance.json -> "human". Consumption rates and dose limits. */
struct FHumanBalance
{
	FReal O2KgPerSol = 0.0;
	FReal Co2KgPerSol = 0.0;
	FReal WaterDrinkKgPerSol = 0.0;
	FReal WaterHygieneKgPerSol = 0.0;
	FReal FoodDryKgPerSol = 0.0;
	FReal KcalPerSol = 0.0;
	FReal CareerDoseMSv = 0.0;
	FReal DeepSpaceDoseMSvPerDay = 0.0;
	FReal SurfaceDoseMSvPerSol = 0.0;
};

/** balance.json -> "cabin". Ship interior blockout spec (brief §4.3). */
struct FCabinBalance
{
	FReal InnerRadiusM = 0.0;
	FReal DeckHeightM = 0.0;
	int32_t Decks = 0;
	int32_t Seats = 0;
};

/** tech.json row. Brief §7: tech has a minimum calendar duration, not just a cost. */
struct FTechRow
{
	std::string Id;
	std::string Name;
	std::string Branch;
	std::string Description;
	int32_t Tier = 0;
	FReal CostRp = 0.0;
	FReal CostUsd = 0.0;
	/** You cannot buy time. Research occupies calendar days regardless of RP. */
	FReal MinDurationDays = 0.0;
	std::vector<std::string> Prereqs;
	std::vector<std::string> Unlocks;
	FReal StartingMaturity = 0.0;
	bool bStartCompleted = false;
	bool bSpeculative = false;
};

/** contractors.json row. Drives cost, reliability and schedule (brief §7). */
struct FContractorRow
{
	std::string Id;
	std::string Name;
	std::string Description;
	std::vector<std::string> Specialties;
	FReal CostMultiplier = 1.0;
	FReal ReliabilityMod = 0.0;
	FReal ScheduleMod = 1.0;
	FReal PoliticalWeight = 0.0;
};

/** cargo.json row. The VAB manifest picks from these. */
struct FCargoRow
{
	std::string Id;
	std::string Name;
	std::string Category;
	std::string Description;
	std::string TechRequired;
	FReal MassKg = 0.0;
	FReal VolumeM3 = 0.0;
	FReal CostUsd = 0.0;
	FReal PowerKw = 0.0;
	int32_t Crew = 0;
	int32_t CrewCapacity = 0;
};

/** roles.json row. */
struct FRoleRow
{
	std::string Id;
	std::string Name;
	std::string Class;
	std::string DefaultTask;
	std::vector<std::string> Tasks;
};

/** resources.json row. */
struct FResourceRow
{
	std::string Id;
	std::string Name;
	std::string Class;
	std::string Store;
	FReal DensityKgM3 = 0.0;
};

/**
 * sites.json row.
 *
 * NOTE (brief §4.4): every shipped record is a MARS site and none carries a
 * body discriminator. L_Surface is meant to serve Moon and Mars from this
 * table, so M5 must add lunar records and a "body" field. Until then Body
 * loads empty and IsMarsOnlyTable() reports true.
 */
struct FSiteRow
{
	std::string Id;
	std::string Name;
	std::string Body;
	std::string Notes;
	FReal Lat = 0.0;
	FReal Lon = 0.0;
	FReal ElevationM = 0.0;
	FReal WaterIceDepthM = 0.0;
	FReal WaterIceAbundance = 0.0;
	FReal RegolithQuality = 0.0;
	FReal TerrainRoughness = 0.0;
	FReal SolarFactor = 0.0;
	FReal ScienceValue = 0.0;
	FReal DustStormExposure = 0.0;
};

struct FAresData
{
	std::string Version;
	FTimeBalance Time;
	FOrbitBalance Orbit;
	FEconomyBalance Economy;
	FEdlBalance Edl;
	FDeltaVBalance DeltaV;
	FHumanBalance Human;
	FCabinBalance Cabin;

	std::vector<FTechRow> Tech;
	std::vector<FContractorRow> Contractors;
	std::vector<FCargoRow> Cargo;
	std::vector<FRoleRow> Roles;
	std::vector<FResourceRow> Resources;
	std::vector<FSiteRow> Sites;

	/** Lookup by id. Returns nullptr when absent. */
	ARESCORE_API const FTechRow* FindTech(const std::string& Id) const;
	ARESCORE_API const FCargoRow* FindCargo(const std::string& Id) const;
	ARESCORE_API const FSiteRow* FindSite(const std::string& Id) const;

	/** True while sites.json carries no lunar records (see FSiteRow). */
	ARESCORE_API bool IsSitesTableMarsOnly() const;

	/**
	 * Loads and validates balance.json. Returns false with OutError set when a
	 * required field is missing — content problems must fail at load, not
	 * silently balance the game to zero.
	 */
	static ARESCORE_API bool LoadBalance(const FJsonValue& Root, FAresData& Out, std::string& OutError);
	static ARESCORE_API bool LoadBalanceFile(const std::string& Path, FAresData& Out, std::string& OutError);

	/**
	 * Loads every table in a Data/ directory. Validation mirrors the reference's
	 * loader.js: a required field missing anywhere fails the whole load, because
	 * a half-loaded content set produces a game that is subtly mis-balanced
	 * rather than obviously broken.
	 */
	static ARESCORE_API bool LoadAll(const std::string& DataDir, FAresData& Out, std::string& OutError);
};

} // namespace Ares
