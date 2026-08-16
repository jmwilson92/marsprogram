#include "AresTest.h"

#include <cstring>
#include <string>

#include "TestPaths.h"

namespace AresTest
{

int RunAll(const char* Filter)
{
	std::string LastSuite;
	int Ran = 0;

	for (const FCase& Case : Registry())
	{
		const std::string Full = Case.Suite + "." + Case.Name;
		if (Filter && *Filter && Full.find(Filter) == std::string::npos)
		{
			continue;
		}
		if (Case.Suite != LastSuite)
		{
			std::printf("\n[%s]\n", Case.Suite.c_str());
			LastSuite = Case.Suite;
		}

		CurrentCase() = Full;
		const int Before = FailureCount();
		Case.Body();
		++Ran;
		if (FailureCount() == Before)
		{
			std::printf("  ok    %s\n", Case.Name.c_str());
		}
	}

	std::printf("\n----------------------------------------------------------\n");
	std::printf("%d cases, %d checks, %d failures\n", Ran, CheckCount(), FailureCount());
	if (FailureCount() == 0)
	{
		std::printf("PASS\n");
	}
	else
	{
		std::printf("FAIL\n");
	}
	return FailureCount() == 0 ? 0 : 1;
}

} // namespace AresTest

int main(int argc, char** argv)
{
	const char* Filter = nullptr;
	for (int I = 1; I < argc; ++I)
	{
		if (std::strcmp(argv[I], "--filter") == 0 && I + 1 < argc)
		{
			Filter = argv[++I];
		}
	}

	std::printf("AresCore automation tests\n");
	std::printf("project root : %s\n", AresTestPaths::ProjectRoot().c_str());
	std::printf("oracle       : %s\n", AresTestPaths::OraclePath().c_str());
	if (Filter)
	{
		std::printf("filter       : %s\n", Filter);
	}

	return AresTest::RunAll(Filter);
}
