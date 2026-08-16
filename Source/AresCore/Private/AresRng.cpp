#include "AresCore/AresRng.h"

#include <cmath>

namespace Ares
{
namespace
{
/**
 * JS Math.imul is a 32-bit multiply with wraparound. Unsigned 32-bit multiply in
 * C++ has the identical bit pattern, so this reproduces the reference exactly.
 */
inline uint32_t Imul(uint32_t A, uint32_t B)
{
	return A * B;
}

/** FNV-1a-style string hash. Mirrors hash32() in the reference. */
uint32_t Hash32(const std::string& Text, uint32_t Seed)
{
	uint32_t H = Seed;
	for (const char C : Text)
	{
		// charCodeAt() yields UTF-16 code units; stream names are ASCII, where
		// the code unit equals the byte. Cast through unsigned char so a
		// non-ASCII name cannot sign-extend and diverge silently.
		H = Imul(H ^ static_cast<uint32_t>(static_cast<unsigned char>(C)), 16777619u);
	}
	return H;
}

struct FMulberryStep
{
	uint32_t State;
	double Value;
};

/**
 * mulberry32. Every intermediate in the JS original is coerced back to 32 bits
 * by the bitwise operators, so plain uint32_t arithmetic matches it exactly.
 */
FMulberryStep StepMulberry(uint32_t A)
{
	A += 0x6d2b79f5u;
	uint32_t T = A;
	T = Imul(T ^ (T >> 15), T | 1u);
	T ^= T + Imul(T ^ (T >> 7), T | 61u);
	const uint32_t Result = T ^ (T >> 14);
	return FMulberryStep{ A, static_cast<double>(Result) / 4294967296.0 };
}
} // namespace

std::string FAresRng::FormatSeed(uint32_t Seed)
{
	static const char* Digits = "0123456789ABCDEF";
	std::string Out(8, '0');
	for (int I = 7; I >= 0; --I)
	{
		Out[static_cast<size_t>(I)] = Digits[Seed & 0xFu];
		Seed >>= 4;
	}
	return Out;
}

uint32_t FAresRng::DeriveStreamSeed(uint32_t InRootSeed, const std::string& Name)
{
	return Hash32(Name, InRootSeed ^ 0x811c9dc5u);
}

FRngStreamState& FAresRng::StreamFor(const std::string& Name)
{
	const auto It = Streams.find(Name);
	if (It != Streams.end())
	{
		return It->second;
	}
	FRngStreamState Fresh;
	Fresh.Seed = DeriveStreamSeed(RootSeed, Name);
	// Fresh streams start with State == Seed so the first Next() is Step(Seed).
	Fresh.State = Fresh.Seed;
	Fresh.Counter = 0;
	return Streams.emplace(Name, Fresh).first->second;
}

double FAresRng::Next(const std::string& StreamName)
{
	FRngStreamState& Slot = StreamFor(StreamName);
	const FMulberryStep Stepped = StepMulberry(Slot.State);
	Slot.State = Stepped.State;
	Slot.Counter += 1;
	return Stepped.Value;
}

double FAresRng::Float(const std::string& StreamName, double Min, double Max)
{
	return Min + Next(StreamName) * (Max - Min);
}

int64_t FAresRng::Int(const std::string& StreamName, double Min, double Max)
{
	const double Lo = std::ceil(Min);
	const double Hi = std::floor(Max);
	return static_cast<int64_t>(Lo + std::floor(Next(StreamName) * (Hi - Lo + 1.0)));
}

bool FAresRng::Chance(const std::string& StreamName, double P)
{
	return Next(StreamName) < P;
}

uint64_t FAresRng::GetCounter(const std::string& StreamName) const
{
	const auto It = Streams.find(StreamName);
	return It == Streams.end() ? 0 : It->second.Counter;
}

bool FAresRng::HasStream(const std::string& StreamName) const
{
	return Streams.find(StreamName) != Streams.end();
}

std::map<std::string, FRngStreamState> FAresRng::Serialize() const
{
	return Streams;
}

FAresRng FAresRng::Deserialize(uint32_t InRootSeed, const std::map<std::string, FRngStreamState>& InStreams)
{
	FAresRng Rng(InRootSeed);
	Rng.Streams = InStreams;
	return Rng;
}

} // namespace Ares
