// Locates data/ and the oracle relative to the project root.
//
// ARES_PROJECT_ROOT is injected by the build (CMake) so the tests can run from
// any working directory; ARES_PROJECT_ROOT_ENV overrides it for CI.

#pragma once

#include <cstdlib>
#include <string>

#ifndef ARES_PROJECT_ROOT
#define ARES_PROJECT_ROOT "."
#endif

namespace AresTestPaths
{

inline std::string ProjectRoot()
{
	if (const char* Override = std::getenv("ARES_PROJECT_ROOT"))
	{
		if (*Override)
		{
			return std::string(Override);
		}
	}
	return std::string(ARES_PROJECT_ROOT);
}

inline std::string OraclePath()
{
	return ProjectRoot() + "/Tools/oracle/oracle.json";
}

inline std::string BalancePath()
{
	return ProjectRoot() + "/Data/balance.json";
}

inline std::string DataPath(const std::string& FileName)
{
	return ProjectRoot() + "/Data/" + FileName;
}

} // namespace AresTestPaths
