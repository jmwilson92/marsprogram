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

	const FJsonValue& HumanJson = Root["human"];
	Out.Human.O2KgPerSol = HumanJson["o2_kg_per_sol"].RequireNumber("human.o2_kg_per_sol", OutError);
	Out.Human.Co2KgPerSol = HumanJson["co2_kg_per_sol"].RequireNumber("human.co2_kg_per_sol", OutError);
	Out.Human.WaterDrinkKgPerSol = HumanJson["waterDrink_kg_per_sol"].RequireNumber("human.waterDrink_kg_per_sol", OutError);
	Out.Human.WaterHygieneKgPerSol = HumanJson["waterHygiene_kg_per_sol"].RequireNumber("human.waterHygiene_kg_per_sol", OutError);
	Out.Human.FoodDryKgPerSol = HumanJson["foodDry_kg_per_sol"].RequireNumber("human.foodDry_kg_per_sol", OutError);
	Out.Human.KcalPerSol = HumanJson["kcal_per_sol"].RequireNumber("human.kcal_per_sol", OutError);
	Out.Human.CareerDoseMSv = HumanJson["careerDose_mSv"].RequireNumber("human.careerDose_mSv", OutError);
	Out.Human.DeepSpaceDoseMSvPerDay = HumanJson["deepSpaceDose_mSv_per_day"].RequireNumber("human.deepSpaceDose_mSv_per_day", OutError);
	Out.Human.SurfaceDoseMSvPerSol = HumanJson["surfaceDose_mSv_per_sol"].RequireNumber("human.surfaceDose_mSv_per_sol", OutError);

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

namespace
{
/** Reads a JSON string array into a vector, tolerating an absent field. */
void ReadStringArray(const FJsonValue& Value, std::vector<std::string>& Out)
{
	if (!Value.IsArray())
	{
		return;
	}
	for (size_t I = 0; I < Value.Num(); ++I)
	{
		Out.push_back(Value[I].AsString());
	}
}

/** Loads a top-level array file and reports a useful error when it is not one. */
bool LoadArrayFile(const std::string& Path, FJsonValue& Out, std::string& OutError)
{
	if (!ParseJsonFile(Path, Out, OutError))
	{
		return false;
	}
	if (!Out.IsArray() || Out.Num() == 0)
	{
		OutError = Path + ": expected a non-empty array";
		return false;
	}
	return true;
}
} // namespace

const FTechRow* FAresData::FindTech(const std::string& Id) const
{
	for (const FTechRow& Row : Tech)
	{
		if (Row.Id == Id) { return &Row; }
	}
	return nullptr;
}

const FCargoRow* FAresData::FindCargo(const std::string& Id) const
{
	for (const FCargoRow& Row : Cargo)
	{
		if (Row.Id == Id) { return &Row; }
	}
	return nullptr;
}

const FSiteRow* FAresData::FindSite(const std::string& Id) const
{
	for (const FSiteRow& Row : Sites)
	{
		if (Row.Id == Id) { return &Row; }
	}
	return nullptr;
}

bool FAresData::IsSitesTableMarsOnly() const
{
	for (const FSiteRow& Row : Sites)
	{
		if (!Row.Body.empty() && Row.Body != "mars")
		{
			return false;
		}
	}
	return true;
}

bool FAresData::LoadAll(const std::string& DataDir, FAresData& Out, std::string& OutError)
{
	const std::string Sep = "/";

	if (!LoadBalanceFile(DataDir + Sep + "balance.json", Out, OutError))
	{
		return false;
	}

	// --- tech.json
	{
		FJsonValue Rows;
		if (!LoadArrayFile(DataDir + Sep + "tech.json", Rows, OutError)) { return false; }
		for (size_t I = 0; I < Rows.Num(); ++I)
		{
			const FJsonValue& R = Rows[I];
			FTechRow Row;
			Row.Id = R["id"].AsString();
			Row.Name = R["name"].AsString();
			Row.Branch = R["branch"].AsString();
			Row.Description = R["description"].AsString();
			Row.Tier = static_cast<int32_t>(R["tier"].AsNumber());
			Row.CostRp = R["cost_rp"].AsNumber();
			Row.CostUsd = R["cost_usd"].AsNumber();
			Row.MinDurationDays = R["minDuration_days"].AsNumber();
			Row.StartingMaturity = R["startingMaturity"].AsNumber();
			Row.bStartCompleted = R["startCompleted"].AsBool();
			Row.bSpeculative = R["speculative"].AsBool();
			ReadStringArray(R["prereqs"], Row.Prereqs);
			ReadStringArray(R["unlocks"], Row.Unlocks);
			if (Row.Id.empty() || Row.Name.empty())
			{
				OutError = "tech.json: row missing id or name";
				return false;
			}
			Out.Tech.push_back(std::move(Row));
		}
	}

	// --- contractors.json
	{
		FJsonValue Rows;
		if (!LoadArrayFile(DataDir + Sep + "contractors.json", Rows, OutError)) { return false; }
		for (size_t I = 0; I < Rows.Num(); ++I)
		{
			const FJsonValue& R = Rows[I];
			FContractorRow Row;
			Row.Id = R["id"].AsString();
			Row.Name = R["name"].AsString();
			Row.Description = R["description"].AsString();
			Row.CostMultiplier = R["costMultiplier"].AsNumber(1.0);
			Row.ReliabilityMod = R["reliabilityMod"].AsNumber();
			Row.ScheduleMod = R["scheduleMod"].AsNumber(1.0);
			Row.PoliticalWeight = R["politicalWeight"].AsNumber();
			ReadStringArray(R["specialties"], Row.Specialties);
			if (Row.Id.empty())
			{
				OutError = "contractors.json: row missing id";
				return false;
			}
			Out.Contractors.push_back(std::move(Row));
		}
	}

	// --- cargo.json
	{
		FJsonValue Rows;
		if (!LoadArrayFile(DataDir + Sep + "cargo.json", Rows, OutError)) { return false; }
		for (size_t I = 0; I < Rows.Num(); ++I)
		{
			const FJsonValue& R = Rows[I];
			FCargoRow Row;
			Row.Id = R["id"].AsString();
			Row.Name = R["name"].AsString();
			Row.Category = R["category"].AsString();
			Row.Description = R["description"].AsString();
			Row.TechRequired = R["techRequired"].AsString();
			Row.MassKg = R["mass_kg"].AsNumber();
			Row.VolumeM3 = R["volume_m3"].AsNumber();
			Row.CostUsd = R["cost"].AsNumber();
			Row.PowerKw = R["power_kw"].AsNumber();
			Row.Crew = static_cast<int32_t>(R["crew"].AsNumber());
			Row.CrewCapacity = static_cast<int32_t>(R["crewCapacity"].AsNumber());
			if (Row.Id.empty())
			{
				OutError = "cargo.json: row missing id";
				return false;
			}
			Out.Cargo.push_back(std::move(Row));
		}
	}

	// --- roles.json
	{
		FJsonValue Rows;
		if (!LoadArrayFile(DataDir + Sep + "roles.json", Rows, OutError)) { return false; }
		for (size_t I = 0; I < Rows.Num(); ++I)
		{
			const FJsonValue& R = Rows[I];
			FRoleRow Row;
			Row.Id = R["id"].AsString();
			Row.Name = R["name"].AsString();
			Row.Class = R["class"].AsString();
			Row.DefaultTask = R["defaultTask"].AsString();
			ReadStringArray(R["tasks"], Row.Tasks);
			Out.Roles.push_back(std::move(Row));
		}
	}

	// --- resources.json
	{
		FJsonValue Rows;
		if (!LoadArrayFile(DataDir + Sep + "resources.json", Rows, OutError)) { return false; }
		for (size_t I = 0; I < Rows.Num(); ++I)
		{
			const FJsonValue& R = Rows[I];
			FResourceRow Row;
			Row.Id = R["id"].AsString();
			Row.Name = R["name"].AsString();
			Row.Class = R["class"].AsString();
			Row.Store = R["store"].AsString();
			Row.DensityKgM3 = R["density_kg_m3"].AsNumber();
			Out.Resources.push_back(std::move(Row));
		}
	}

	// --- sites.json
	{
		FJsonValue Rows;
		if (!LoadArrayFile(DataDir + Sep + "sites.json", Rows, OutError)) { return false; }
		for (size_t I = 0; I < Rows.Num(); ++I)
		{
			const FJsonValue& R = Rows[I];
			FSiteRow Row;
			Row.Id = R["id"].AsString();
			Row.Name = R["name"].AsString();
			// Absent in every shipped record — see FSiteRow's note.
			Row.Body = R["body"].AsString();
			Row.Notes = R["notes"].AsString();
			Row.Lat = R["lat"].AsNumber();
			Row.Lon = R["lon"].AsNumber();
			Row.ElevationM = R["elevation_m"].AsNumber();
			Row.WaterIceDepthM = R["waterIce_depth_m"].AsNumber();
			Row.WaterIceAbundance = R["waterIce_abundance"].AsNumber();
			Row.RegolithQuality = R["regolithQuality"].AsNumber();
			Row.TerrainRoughness = R["terrainRoughness"].AsNumber();
			Row.SolarFactor = R["solarFactor"].AsNumber();
			Row.ScienceValue = R["scienceValue"].AsNumber();
			Row.DustStormExposure = R["dustStormExposure"].AsNumber();
			if (Row.Id.empty())
			{
				OutError = "sites.json: row missing id";
				return false;
			}
			Out.Sites.push_back(std::move(Row));
		}
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
