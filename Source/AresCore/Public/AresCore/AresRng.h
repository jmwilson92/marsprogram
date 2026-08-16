// AresCore — seeded, serializable, deterministic RNG.
//
// Ported from ../reference src/core/rng.js. Named mulberry32 streams: each
// stream carries its own state and counter so a new roll in `incidents` cannot
// desync `edl`. Brief §3.1: every random outcome in the game draws from this,
// and same seed + same inputs must produce the same run.
//
// This is pure 32-bit integer arithmetic, so it is bit-exact across compilers
// and platforms — unlike anything that routes through libm. Sim decisions that
// must be reproducible should key off these draws, not off transcendental math.

#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace Ares
{

/** One named stream's serializable state. */
struct FRngStreamState
{
	uint32_t Seed = 0;
	uint32_t State = 0;
	uint64_t Counter = 0;
};

class FAresRng
{
public:
	FAresRng() = default;
	explicit FAresRng(uint32_t InRootSeed)
		: RootSeed(InRootSeed)
	{
	}

	uint32_t GetRootSeed() const { return RootSeed; }

	/** Formats the seed the way the UI shows it: 8 uppercase hex digits. */
	static std::string FormatSeed(uint32_t Seed);

	/**
	 * Derives a stream's seed from the root. Streams are created lazily on first
	 * use, exactly as the reference does, so adding a new stream later cannot
	 * shift the sequence of any existing one.
	 */
	static uint32_t DeriveStreamSeed(uint32_t RootSeed, const std::string& Name);

	/** Uniform double in [0, 1). */
	double Next(const std::string& StreamName);

	/** Uniform double in [Min, Max). */
	double Float(const std::string& StreamName, double Min = 0.0, double Max = 1.0);

	/** Uniform integer in [Min, Max], inclusive on both ends. */
	int64_t Int(const std::string& StreamName, double Min, double Max);

	/** True with probability P. */
	bool Chance(const std::string& StreamName, double P);

	/** Uniform pick. Returns Fallback when Items is empty. */
	template <typename T>
	const T& Pick(const std::string& StreamName, const std::vector<T>& Items, const T& Fallback)
	{
		if (Items.empty())
		{
			return Fallback;
		}
		const int64_t Index = Int(StreamName, 0.0, static_cast<double>(Items.size()) - 1.0);
		return Items[static_cast<size_t>(Index)];
	}

	uint64_t GetCounter(const std::string& StreamName) const;
	bool HasStream(const std::string& StreamName) const;

	/** Round-trips through save/load. Restoring resumes the identical sequence. */
	std::map<std::string, FRngStreamState> Serialize() const;
	static FAresRng Deserialize(uint32_t InRootSeed, const std::map<std::string, FRngStreamState>& InStreams);

private:
	FRngStreamState& StreamFor(const std::string& Name);

	uint32_t RootSeed = 0;
	std::map<std::string, FRngStreamState> Streams;
};

} // namespace Ares
