// JSON reader and content loading. Guards brief §9's data-driven rule: if a
// number is in data/*.json, the sim reads it from there.

#include "AresCore/AresData.h"

#include <string>

#include "AresCore/AresJson.h"
#include "Harness/AresTest.h"
#include "OracleFixture.h"

using namespace Ares;

ARES_TEST(Json, ParsesScalarsAndNesting)
{
	FJsonValue V;
	std::string Error;
	const std::string Text = R"({
		"n": -12.5e2, "s": "a\"b\\c\n", "t": true, "f": false, "z": null,
		"arr": [1, 2, {"deep": [3]}], "obj": {"k": {"k2": 7}}
	})";
	CHECK_TRUE(ParseJson(Text, V, Error));
	CHECK_STR(Error, "", "no error");

	CHECK_EXACT(V["n"].AsNumber(), -1250.0, "number");
	CHECK_STR(V["s"].AsString(), "a\"b\\c\n", "escaped string");
	CHECK_TRUE(V["t"].AsBool());
	CHECK_TRUE(!V["f"].AsBool(true));
	CHECK_TRUE(V["z"].IsNull());
	CHECK_INT(static_cast<long long>(V["arr"].Num()), 3, "array size");
	CHECK_EXACT(V["arr"][(size_t)2]["deep"][(size_t)0].AsNumber(), 3.0, "nested array");
	CHECK_EXACT(V["obj"]["k"]["k2"].AsNumber(), 7.0, "nested object");
}

ARES_TEST(Json, MissingLookupsAreSafeNotCrashes)
{
	// The loader leans on this: a missing field yields a null value rather than
	// throwing, so RequireNumber can report a useful path instead of a stack trace.
	FJsonValue V;
	std::string Error;
	CHECK_TRUE(ParseJson("{\"a\":1}", V, Error));
	CHECK_TRUE(V["nope"].IsNull());
	CHECK_TRUE(V["nope"]["deeper"].IsNull());
	CHECK_TRUE(V["nope"][(size_t)5].IsNull());
	CHECK_EXACT(V["nope"].AsNumber(-1.0), -1.0, "fallback");
	CHECK_TRUE(!V.HasField("nope"));
	CHECK_TRUE(V.HasField("a"));
}

ARES_TEST(Json, RejectsMalformedInputWithPosition)
{
	FJsonValue V;
	std::string Error;
	CHECK_TRUE(!ParseJson("{\"a\": }", V, Error));
	CHECK_TRUE(!Error.empty());

	Error.clear();
	CHECK_TRUE(!ParseJson("[1, 2", V, Error));
	CHECK_TRUE(!Error.empty());

	Error.clear();
	CHECK_TRUE(!ParseJson("{\"a\":1} trailing", V, Error));
	CHECK_TRUE(!Error.empty());
}

ARES_TEST(Json, ParsesUnicodeEscapes)
{
	FJsonValue V;
	std::string Error;
	CHECK_TRUE(ParseJson(R"({"s":"µ — 🚀"})", V, Error));
	// micro sign (2 bytes) + space + em dash (3) + space + rocket (4)
	CHECK_INT(static_cast<long long>(V["s"].AsString().size()), 11, "utf-8 byte length");
}

ARES_TEST(Data, AllShippedTablesParse)
{
	// Every file copied unchanged from the reference must load.
	const char* Files[] = {
		"balance.json", "cargo.json", "contractors.json",
		"resources.json", "roles.json", "sites.json", "tech.json",
	};
	for (const char* Name : Files)
	{
		FJsonValue Root;
		std::string Error;
		const bool bOk = ParseJsonFile(AresTestPaths::DataPath(Name), Root, Error);
		CHECK_TRUE(bOk);
		if (!bOk)
		{
			std::printf("        %s: %s\n", Name, Error.c_str());
			continue;
		}
		CHECK_TRUE(Root.Num() > 0);
	}
}

ARES_TEST(Data, BalanceLoadsExpectedValues)
{
	const FAresData& D = AresTest::Balance();

	CHECK_STR(D.Version, "1.0.0", "version");
	CHECK_EXACT(D.Economy.StartingSupport, 58.0, "startingSupport");
	CHECK_EXACT(D.Economy.StarshipLaunchUsd, 850000000.0, "starshipLaunch_usd");
	CHECK_EXACT(D.Economy.BoosterRefuelUsd, 52000000.0, "boosterRefuel_usd");

	// Brief §7: the ~16x reuse delta is meant to drive the whole economy.
	CHECK_TRUE(D.Economy.StarshipLaunchUsd / D.Economy.BoosterRefuelUsd > 16.0);

	CHECK_EXACT(D.Edl.EarlySuccess, 0.72, "edl.earlySuccess");
	CHECK_EXACT(D.Edl.MatureSuccess, 0.96, "edl.matureSuccess");
	CHECK_INT(static_cast<long long>(D.Edl.StageIds.size()), 8, "edl stage count");
	CHECK_STR(D.Edl.StageIds.front(), "entry", "first stage");
	CHECK_STR(D.Edl.StageIds.back(), "touchdown", "last stage");

	CHECK_EXACT(D.Cabin.InnerRadiusM, 4.15, "cabin.innerRadius_m");
	CHECK_EXACT(D.Cabin.DeckHeightM, 2.48, "cabin.deckHeight_m");
}

ARES_TEST(Data, DifficultyBudgetsResolve)
{
	const FEconomyBalance& E = AresTest::Balance().Economy;
	CHECK_EXACT(E.StartingAnnualFor("DIRECTOR"), 6000000000.0, "DIRECTOR");
	CHECK_EXACT(E.StartingAnnualFor("ADMINISTRATOR"), 4200000000.0, "ADMINISTRATOR");
	CHECK_EXACT(E.StartingAnnualFor("AUSTERITY"), 2600000000.0, "AUSTERITY");
	CHECK_EXACT(E.StartingAnnualFor("IRONMAN"), 2200000000.0, "IRONMAN");
	// Unknown keys fall back to ADMINISTRATOR, matching the reference loader.
	CHECK_EXACT(E.StartingAnnualFor("NONSENSE"), 4200000000.0, "fallback");
}

ARES_TEST(Data, MarsDeltaVLegsPresentLunarLegsNotYet)
{
	const FDeltaVBalance& Dv = AresTest::Balance().DeltaV;

	CHECK_EXACT(Dv.EarthToLeo, 9400.0, "earthToLeo");
	CHECK_EXACT(Dv.LeoToTmi, 3600.0, "leoToTmi");
	CHECK_EXACT(Dv.OrbitToSurfaceEdl, 700.0, "orbitToSurfaceEdl");

	// DOCUMENTED GAP (brief §5.1): the shipped balance.json has no lunar legs.
	// M5 adds leoToTli/tliToLlo/lloToSurface/surfaceToLlo/lloToTei to the DATA
	// FILE. This assertion is the tripwire: when the data lands, it fails and
	// must be flipped, which is how we avoid quietly hardcoding them in C++.
	CHECK_TRUE(!Dv.IsLunarComplete());
}

ARES_TEST(Data, MissingRequiredFieldFailsLoudly)
{
	FJsonValue Root;
	std::string Error;
	CHECK_TRUE(ParseJson(R"({"version":"x","time":{"solSeconds":88775}})", Root, Error));

	FAresData Out;
	std::string LoadError;
	// A truncated balance file must be rejected, not silently zero-balanced.
	CHECK_TRUE(!FAresData::LoadBalance(Root, Out, LoadError));
	CHECK_TRUE(!LoadError.empty());
}

ARES_TEST(Data, SitesTableIsMarsOnly)
{
	// DOCUMENTED GAP (brief §4.4): L_Surface is meant to serve Moon and Mars
	// from sites.json, but every shipped record is a Mars site and none carries
	// a body discriminator. M5 must add lunar records and a "body" field.
	FJsonValue Sites;
	std::string Error;
	CHECK_TRUE(ParseJsonFile(AresTestPaths::DataPath("sites.json"), Sites, Error));
	CHECK_TRUE(Sites.IsArray());
	CHECK_INT(static_cast<long long>(Sites.Num()), 10, "site count");

	int WithBody = 0;
	for (size_t I = 0; I < Sites.Num(); ++I)
	{
		CHECK_TRUE(!Sites[I]["id"].AsString().empty());
		if (Sites[I].HasField("body"))
		{
			++WithBody;
		}
	}
	CHECK_INT(WithBody, 0, "sites carrying a body discriminator");
}
