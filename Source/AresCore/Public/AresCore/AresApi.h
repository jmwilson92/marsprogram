// AresCore — symbol visibility across the module boundary.
//
// Under Unreal every module is a DLL, and Windows exports nothing unless it is
// told to. Without an export annotation, AresGame links against an empty
// surface and every AresCore symbol comes back as LNK2019.
//
// UBT defines ARESCORE_API on the compiler command line, but NOT as a
// __declspec directly — it defines it as `DLLEXPORT` when building the module
// and `DLLIMPORT` when consuming it. Those two are themselves macros, declared
// in UE's platform headers (Windows/WindowsPlatform.h and friends).
//
// AresCore includes no UE headers by design, so DLLEXPORT/DLLIMPORT would
// otherwise expand to bare undefined identifiers and every declaration would
// fail with "missing type specifier - int assumed" / C2086 redefinition. This
// header supplies them.
//
// Redefinition is safe: C++ permits a macro to be redefined identically, and
// these match UE's own spelling token for token, so it does not matter whether
// a translation unit reaches UE's platform header before or after this one.
// The #ifndef guards make that explicit anyway.
//
// The headless CMake build (see CMakeLists.txt) defines no ARESCORE_API at
// all, so it falls through to empty and everything stays visible in the static
// library.
//
// Annotations are applied PER FUNCTION rather than per class on purpose.
// Several AresCore types hold std::string / std::map members, and exporting a
// whole class with standard-library members trips MSVC C4251
// ("needs to have dll-interface to be used by clients").

#pragma once

#if defined(_WIN32) || defined(_WIN64)
	#ifndef DLLEXPORT
	#define DLLEXPORT __declspec(dllexport)
	#endif
	#ifndef DLLIMPORT
	#define DLLIMPORT __declspec(dllimport)
	#endif
#else
	#ifndef DLLEXPORT
	#define DLLEXPORT __attribute__((visibility("default")))
	#endif
	#ifndef DLLIMPORT
	#define DLLIMPORT __attribute__((visibility("default")))
	#endif
#endif

// Only the headless build reaches this; under UBT the macro already exists.
#ifndef ARESCORE_API
#define ARESCORE_API
#endif
