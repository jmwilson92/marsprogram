// Port of ../reference src/core/rng.js, checked against the JS oracle.
//
// These assertions are EXACT. The RNG is pure 32-bit integer arithmetic, so
// there is no platform-dependent rounding to excuse a mismatch: if a draw
// differs from the reference by one ulp, the port is wrong.

#include "AresCore/AresRng.h"

#include <string>
#include <vector>

#include "Harness/AresTest.h"
#include "OracleFixture.h"

using namespace Ares;

ARES_TEST(Rng, StreamsMatchReferenceExactly)
{
	const auto& Cases = AresTest::Oracle()["rng"]["cases"];
	CHECK_TRUE(Cases.Num() > 0);

	for (size_t C = 0; C < Cases.Num(); ++C)
	{
		const auto& Case = Cases[C];
		const uint32_t Seed = static_cast<uint32_t>(Case["seed"].AsNumber());
		FAresRng Rng(Seed);

		for (const auto& [Name, Slot] : Case["streams"].AsObject())
		{
			const auto& Expected = Slot["first16"];
			for (size_t I = 0; I < Expected.Num(); ++I)
			{
				const double Actual = Rng.Next(Name);
				CHECK_EXACT(Actual, Expected[I].AsNumber(), "stream draw");
			}
		}
	}
}

ARES_TEST(Rng, StreamsAreIndependent)
{
	// A new roll in one stream must not desync another — the whole reason the
	// reference splits streams by name in the first place.
	const auto& Ind = AresTest::Oracle()["rng"]["independence"];

	FAresRng Alone(42);
	const auto& ExpectedAlone = Ind["edlAlone"];
	for (size_t I = 0; I < ExpectedAlone.Num(); ++I)
	{
		CHECK_EXACT(Alone.Next("edl"), ExpectedAlone[I].AsNumber(), "edl alone");
	}

	FAresRng Interleaved(42);
	const auto& ExpectedInterleaved = Ind["edlInterleaved"];
	for (size_t I = 0; I < ExpectedInterleaved.Num(); ++I)
	{
		Interleaved.Next("incidents");
		Interleaved.Next("weather");
		CHECK_EXACT(Interleaved.Next("edl"), ExpectedInterleaved[I].AsNumber(), "edl interleaved");
	}

	// And the two edl sequences must be the same sequence.
	for (size_t I = 0; I < ExpectedAlone.Num() && I < ExpectedInterleaved.Num(); ++I)
	{
		CHECK_EXACT(ExpectedInterleaved[I].AsNumber(), ExpectedAlone[I].AsNumber(), "independence");
	}
}

ARES_TEST(Rng, HelperApiMatchesReference)
{
	const auto& Helpers = AresTest::Oracle()["rng"]["helpers"];
	FAresRng Rng(2024);

	const auto& Ints = Helpers["ints_1_6"];
	for (size_t I = 0; I < Ints.Num(); ++I)
	{
		CHECK_INT(Rng.Int("helpers", 1, 6), (long long)Ints[I].AsNumber(), "Int(1,6)");
	}

	const auto& Floats = Helpers["floats_neg1_1"];
	for (size_t I = 0; I < Floats.Num(); ++I)
	{
		CHECK_EXACT(Rng.Float("helpers", -1.0, 1.0), Floats[I].AsNumber(), "Float(-1,1)");
	}

	const auto& Chances = Helpers["chance_p30"];
	for (size_t I = 0; I < Chances.Num(); ++I)
	{
		const bool Actual = Rng.Chance("helpers", 0.3);
		CHECK_INT(Actual ? 1 : 0, Chances[I].AsBool() ? 1 : 0, "Chance(0.3)");
	}
}

ARES_TEST(Rng, SerializeRestoreResumesSequence)
{
	const auto& Ser = AresTest::Oracle()["rng"]["serialization"];

	FAresRng Rng(7);
	const auto& Before = Ser["before"];
	for (size_t I = 0; I < Before.Num(); ++I)
	{
		CHECK_EXACT(Rng.Next("edl"), Before[I].AsNumber(), "before snapshot");
	}

	const auto Blob = Rng.Serialize();

	const auto& After = Ser["after"];
	for (size_t I = 0; I < After.Num(); ++I)
	{
		CHECK_EXACT(Rng.Next("edl"), After[I].AsNumber(), "after snapshot");
	}

	// Restoring the snapshot must replay exactly what the original produced next.
	FAresRng Restored = FAresRng::Deserialize(7, Blob);
	const auto& AfterRestore = Ser["afterRestore"];
	for (size_t I = 0; I < AfterRestore.Num(); ++I)
	{
		CHECK_EXACT(Restored.Next("edl"), AfterRestore[I].AsNumber(), "after restore");
	}
}

ARES_TEST(Rng, SerializedStateMatchesReferenceFields)
{
	const auto& Serialized = AresTest::Oracle()["rng"]["serialization"]["serialized"];
	CHECK_INT(static_cast<long long>(Serialized["rootSeed"].AsNumber()), 7, "rootSeed");

	FAresRng Rng(7);
	for (int I = 0; I < 5; ++I)
	{
		Rng.Next("edl");
	}
	const auto Blob = Rng.Serialize();
	const auto It = Blob.find("edl");
	CHECK_TRUE(It != Blob.end());
	if (It == Blob.end())
	{
		return;
	}

	const auto& ExpectedEdl = Serialized["streams"]["edl"];
	CHECK_INT(It->second.Seed, static_cast<long long>(ExpectedEdl["seed"].AsNumber()), "edl seed");
	CHECK_INT(It->second.State, static_cast<long long>(ExpectedEdl["state"].AsNumber()), "edl state");
	CHECK_INT(static_cast<long long>(It->second.Counter),
		static_cast<long long>(ExpectedEdl["counter"].AsNumber()), "edl counter");
}

ARES_TEST(Rng, FormatSeedIsEightHexDigits)
{
	CHECK_STR(FAresRng::FormatSeed(0u), "00000000", "seed 0");
	CHECK_STR(FAresRng::FormatSeed(0xDEADBEEFu), "DEADBEEF", "seed DEADBEEF");
	CHECK_STR(FAresRng::FormatSeed(0x1u), "00000001", "seed 1");
}

ARES_TEST(Rng, DrawsStayInUnitInterval)
{
	// A cheap invariant that would catch a sign or shift error the oracle
	// samples happened to miss.
	FAresRng Rng(0xC0FFEEu);
	for (int I = 0; I < 20000; ++I)
	{
		const double X = Rng.Next("sweep");
		CHECK_TRUE(X >= 0.0 && X < 1.0);
		if (!(X >= 0.0 && X < 1.0))
		{
			break;
		}
	}
	CHECK_INT(static_cast<long long>(Rng.GetCounter("sweep")), 20000, "counter");
}
