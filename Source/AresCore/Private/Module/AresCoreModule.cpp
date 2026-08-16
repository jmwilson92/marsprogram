// The ONLY file in AresCore that includes an Unreal header.
//
// UBT requires every module to implement a module interface. That boilerplate
// is quarantined here, in Private/Module/, which the headless CMake build
// deliberately does not glob (CMakeLists.txt globs Private/*.cpp only, not
// recursively). Everything else in AresCore is plain C++20 and compiles in
// both builds from the same source.
//
// If you find yourself wanting to add a second file to this directory, that is
// a signal the code belongs in AresGame instead.

#include "Modules/ModuleManager.h"

IMPLEMENT_MODULE(FDefaultModuleImpl, AresCore);
