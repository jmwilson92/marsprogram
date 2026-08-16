// AresCore — symbol visibility across the module boundary.
//
// Under Unreal every module is a DLL, and Windows exports nothing unless it is
// told to. UBT defines ARESCORE_API on the compiler command line — dllexport
// while building AresCore, dllimport for anything consuming it. Without the
// annotation, AresGame links against an empty surface and every AresCore
// symbol comes back as LNK2019.
//
// The headless CMake build produces a static library and defines no such
// macro, so it falls through to empty and everything stays visible. That is
// why this file defines the fallback rather than including a UE header: the
// no-Unreal-headers rule in AresCore.Build.cs still holds.
//
// Annotations are applied PER FUNCTION rather than per class on purpose.
// Several AresCore types hold std::string / std::map members, and exporting a
// whole class with standard-library members trips MSVC C4251
// ("needs to have dll-interface to be used by clients").

#pragma once

#ifndef ARESCORE_API
#define ARESCORE_API
#endif
