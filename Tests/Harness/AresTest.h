// Minimal headless test harness for AresCore.
//
// Brief §2 forbids third-party dependencies without approval, so this is not
// gtest/Catch — it is ~100 lines of assertion plumbing. Under UE these same
// test bodies get wrapped in IMPLEMENT_SIMPLE_AUTOMATION_TEST; the assertions
// are deliberately shaped so that swap is mechanical.

#pragma once

#include <cmath>
#include <cstdio>
#include <functional>
#include <string>
#include <vector>

namespace AresTest
{

struct FCase
{
	std::string Suite;
	std::string Name;
	std::function<void()> Body;
};

inline std::vector<FCase>& Registry()
{
	static std::vector<FCase> Cases;
	return Cases;
}

inline int& FailureCount()
{
	static int Count = 0;
	return Count;
}

inline int& CheckCount()
{
	static int Count = 0;
	return Count;
}

inline std::string& CurrentCase()
{
	static std::string Name;
	return Name;
}

struct FRegistrar
{
	FRegistrar(const char* Suite, const char* Name, std::function<void()> Body)
	{
		Registry().push_back(FCase{ Suite, Name, std::move(Body) });
	}
};

inline void Fail(const char* File, int Line, const std::string& Message)
{
	++FailureCount();
	std::printf("  FAIL  %s\n        %s:%d\n        %s\n",
		CurrentCase().c_str(), File, Line, Message.c_str());
}

inline void CheckTrue(bool Condition, const char* Expr, const char* File, int Line)
{
	++CheckCount();
	if (!Condition)
	{
		Fail(File, Line, std::string("expected true: ") + Expr);
	}
}

template <typename T>
void CheckEqual(const T& Actual, const T& Expected, const char* Label, const char* File, int Line)
{
	++CheckCount();
	if (!(Actual == Expected))
	{
		std::string Msg = std::string(Label) + ": values differ";
		Fail(File, Line, Msg);
	}
}

inline void CheckEqualStr(const std::string& Actual, const std::string& Expected,
	const char* Label, const char* File, int Line)
{
	++CheckCount();
	if (Actual != Expected)
	{
		Fail(File, Line, std::string(Label) + ": expected \"" + Expected + "\", got \"" + Actual + "\"");
	}
}

inline void CheckEqualInt(long long Actual, long long Expected,
	const char* Label, const char* File, int Line)
{
	++CheckCount();
	if (Actual != Expected)
	{
		char Buffer[256];
		std::snprintf(Buffer, sizeof(Buffer), "%s: expected %lld, got %lld", Label, Expected, Actual);
		Fail(File, Line, Buffer);
	}
}

/** Exact bit-for-bit double comparison. Used for RNG output and integer-valued fields. */
inline void CheckExact(double Actual, double Expected, const char* Label, const char* File, int Line)
{
	++CheckCount();
	if (!(Actual == Expected))
	{
		char Buffer[320];
		std::snprintf(Buffer, sizeof(Buffer), "%s: expected %.17g, got %.17g (delta %.3g)",
			Label, Expected, Actual, Actual - Expected);
		Fail(File, Line, Buffer);
	}
}

/**
 * Relative comparison for values that pass through libm. sin/cos/tan/atan are
 * not guaranteed identical to the last ulp across platforms, so anything
 * derived from them is compared with a tolerance rather than exactly.
 */
inline void CheckNear(double Actual, double Expected, double Tolerance,
	const char* Label, const char* File, int Line)
{
	++CheckCount();
	const double Scale = std::fmax(1.0, std::fabs(Expected));
	const double Delta = std::fabs(Actual - Expected);
	if (!(Delta <= Tolerance * Scale))
	{
		char Buffer[320];
		std::snprintf(Buffer, sizeof(Buffer), "%s: expected %.17g, got %.17g (delta %.3g > tol %.3g)",
			Label, Expected, Actual, Delta, Tolerance * Scale);
		Fail(File, Line, Buffer);
	}
}

int RunAll(const char* Filter);

} // namespace AresTest

#define ARES_TEST(Suite, Name)                                                    \
	static void Suite##_##Name##_Body();                                          \
	static ::AresTest::FRegistrar Suite##_##Name##_Reg(                           \
		#Suite, #Name, Suite##_##Name##_Body);                                    \
	static void Suite##_##Name##_Body()

#define CHECK_TRUE(Cond) ::AresTest::CheckTrue((Cond), #Cond, __FILE__, __LINE__)
#define CHECK_INT(Actual, Expected, Label) \
	::AresTest::CheckEqualInt((long long)(Actual), (long long)(Expected), Label, __FILE__, __LINE__)
#define CHECK_STR(Actual, Expected, Label) \
	::AresTest::CheckEqualStr((Actual), (Expected), Label, __FILE__, __LINE__)
#define CHECK_EXACT(Actual, Expected, Label) \
	::AresTest::CheckExact((Actual), (Expected), Label, __FILE__, __LINE__)
#define CHECK_NEAR(Actual, Expected, Tol, Label) \
	::AresTest::CheckNear((Actual), (Expected), (Tol), Label, __FILE__, __LINE__)
