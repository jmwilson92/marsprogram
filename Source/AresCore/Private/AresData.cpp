#include "AresCore/AresData.h"

#include "AresCore/AresJson.h"

namespace Ares
{

FReal FEconomyBalance::StartingAnnualFor(const std::string& Difficulty) const
{
	if (Difficulty == "DIRECTOR") return StartingAnnualDirector;
	if (Difficulty == "AUSTERITY") return StartingAnnualAusterity;
	if (Difficulty == "IRONMAN") return StartingAnnualIronman;
	// ADMINISTRATOR is the default difficulty and the reference's fallback.
	return StartingAnnualAdministrator;
}

bool FAresData::LoadBalance(const FJsonValue& Root, FAresData& Out, std::string& OutError)
{
	OutError.clear();
	if (!Root.IsObject())
	{
		OutError = "balance.json: expected a top-level object";
		return false;
	}

	Out.Version = Root["version"].AsString();

	const FJsonValue& TimeJson = Root["time"];
	Out.Time.SolSeconds = TimeJson["solSeconds"].RequireNumber("time.solSeconds", OutError);
	Out.Time.EarthDaySeconds = TimeJson["earthDaySeconds"].RequireNumber("time.earthDaySeconds", OutError);
	Out.Time.MarsYearSols = TimeJson["marsYearSols"].RequireNumber("time.marsYearSols", OutError);
	Out.Time.MarsYearEarthDays = TimeJson["marsYearEarthDays"].RequireNumber("time.marsYearEarthDays", OutError);
	Out.Time.SynodicEarthDays = TimeJson["synodicEarthDays"].RequireNumber("time.synodicEarthDays", OutError);
	Out.Time.FirstWindowEarthDay = TimeJson["firstWindowEarthDay"].RequireNumber("time.firstWindowEarthDay", OutError);

	const FJsonValue& OrbitJson = Root["orbit"];
	Out.Orbit.Eccentricity = OrbitJson["eccentricity"].RequireNumber("orbit.eccentricity", OutError);
	Out.Orbit.AxialTiltDeg = OrbitJson["axialTiltDeg"].RequireNumber("orbit.axialTiltDeg", OutError);
	Out.Orbit.LsPerihelionDeg = OrbitJson["lsPerihelionDeg"].RequireNumber("orbit.lsPerihelionDeg", OutError);
	Out.Orbit.SolarConstantMeanWm2 = OrbitJson["solarConstantMean_wm2"].RequireNumber("orbit.solarConstantMean_wm2", OutError);
	Out.Orbit.GravityMps2 = OrbitJson["gravity_mps2"].RequireNumber("orbit.gravity_mps2", OutError);
	Out.Orbit.RadiusKm = OrbitJson["radius_km"].RequireNumber("orbit.radius_km", OutError);
	Out.Orbit.SurfacePressurePa = OrbitJson["surfacePressure_pa"].RequireNumber("orbit.surfacePressure_pa", OutError);

	const FJsonValue& EconomyJson = Root["economy"];
	const FJsonValue& Starting = EconomyJson["startingAnnual"];
	Out.Economy.StartingAnnualDirector = Starting["DIRECTOR"].RequireNumber("economy.startingAnnual.DIRECTOR", OutError);
	Out.Economy.StartingAnnualAdministrator = Starting["ADMINISTRATOR"].RequireNumber("economy.startingAnnual.ADMINISTRATOR", OutError);
	Out.Economy.StartingAnnualAusterity = Starting["AUSTERITY"].RequireNumber("economy.startingAnnual.AUSTERITY", OutError);
	Out.Economy.StartingAnnualIronman = Starting["IRONMAN"].RequireNumber("economy.startingAnnual.IRONMAN", OutError);
	Out.Economy.StartingSupport = EconomyJson["startingSupport"].RequireNumber("economy.startingSupport", OutError);
	Out.Economy.StarshipLaunchUsd = EconomyJson["starshipLaunch_usd"].RequireNumber("economy.starshipLaunch_usd", OutError);
	Out.Economy.BoosterRefuelUsd = EconomyJson["boosterRefuel_usd"].RequireNumber("economy.boosterRefuel_usd", OutError);
	Out.Economy.StarshipPayloadKg = EconomyJson["starshipPayload_kg"].RequireNumber("economy.starshipPayload_kg", OutError);
	Out.Economy.StarshipPayloadM3 = EconomyJson["starshipPayload_m3"].RequireNumber("economy.starshipPayload_m3", OutError);
	Out.Economy.RpBuyUsd = EconomyJson["rpBuy_usd"].RequireNumber("economy.rpBuy_usd", OutError);

	const FJsonValue& EdlJson = Root["edl"];
	Out.Edl.EarlySuccess = EdlJson["earlySuccess"].RequireNumber("edl.earlySuccess", OutError);
	Out.Edl.MatureSuccess = EdlJson["matureSuccess"].RequireNumber("edl.matureSuccess", OutError);
	const FJsonValue& Stages = EdlJson["stages"];
	if (!Stages.IsArray() || Stages.Num() == 0)
	{
		if (OutError.empty()) { OutError = "balance.json: edl.stages must be a non-empty array"; }
	}
	else
	{
		for (size_t I = 0; I < Stages.Num(); ++I)
		{
			Out.Edl.StageIds.push_back(Stages[I]["id"].AsString());
			Out.Edl.StageLabels.push_back(Stages[I]["label"].AsString());
		}
	}

	// Mars legs are required; the lunar legs the brief adds are not in the
	// shipped data file yet, so they are read optionally and left at zero.
	const FJsonValue& Dv = Root["deltaV_mps"];
	Out.DeltaV.EarthToLeo = Dv["earthToLeo"].RequireNumber("deltaV_mps.earthToLeo", OutError);
	Out.DeltaV.LeoToTmi = Dv["leoToTmi"].RequireNumber("deltaV_mps.leoToTmi", OutError);
	Out.DeltaV.TmiToCapturePropulsive = Dv["tmiToCapturePropulsive"].RequireNumber("deltaV_mps.tmiToCapturePropulsive", OutError);
	Out.DeltaV.TmiToAerocapture = Dv["tmiToAerocapture"].RequireNumber("deltaV_mps.tmiToAerocapture", OutError);
	Out.DeltaV.OrbitToSurfaceEdl = Dv["orbitToSurfaceEdl"].RequireNumber("deltaV_mps.orbitToSurfaceEdl", OutError);
	Out.DeltaV.SurfaceToLmo = Dv["surfaceToLmo"].RequireNumber("deltaV_mps.surfaceToLmo", OutError);
	Out.DeltaV.LmoToTei = Dv["lmoToTei"].RequireNumber("deltaV_mps.lmoToTei", OutError);

	Out.DeltaV.LeoToTli = Dv["leoToTli"].AsNumber(0.0);
	Out.DeltaV.TliToLlo = Dv["tliToLlo"].AsNumber(0.0);
	Out.DeltaV.LloToSurface = Dv["lloToSurface"].AsNumber(0.0);
	Out.DeltaV.SurfaceToLlo = Dv["surfaceToLlo"].AsNumber(0.0);
	Out.DeltaV.LloToTei = Dv["lloToTei"].AsNumber(0.0);

	const FJsonValue& CabinJson = Root["cabin"];
	Out.Cabin.InnerRadiusM = CabinJson["innerRadius_m"].RequireNumber("cabin.innerRadius_m", OutError);
	Out.Cabin.DeckHeightM = CabinJson["deckHeight_m"].RequireNumber("cabin.deckHeight_m", OutError);
	Out.Cabin.Decks = static_cast<int32_t>(CabinJson["decks"].RequireNumber("cabin.decks", OutError));
	Out.Cabin.Seats = static_cast<int32_t>(CabinJson["seats"].RequireNumber("cabin.seats", OutError));

	if (!OutError.empty())
	{
		OutError = "balance.json: " + OutError;
		return false;
	}
	return true;
}

bool FAresData::LoadBalanceFile(const std::string& Path, FAresData& Out, std::string& OutError)
{
	FJsonValue Root;
	if (!ParseJsonFile(Path, Root, OutError))
	{
		return false;
	}
	return LoadBalance(Root, Out, OutError);
}

} // namespace Ares
