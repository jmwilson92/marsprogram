# Headless build for AresCore + its automation tests.
#
# This is NOT how the game ships — Unreal Build Tool compiles AresCore from
# Source/AresCore/AresCore.Build.cs as part of Ares.uproject. This CMake file
# exists because brief §3.1 requires AresCore to "compile and unit-test
# headless", and because it lets the simulation be verified on a machine with
# no engine installed and in CI.
#
# The two builds compile the exact same sources. If AresCore ever fails to
# configure here, it has grown an engine dependency it is not allowed to have.
#
#   cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
#   cmake --build build -j
#   ./build/AresCoreTests

cmake_minimum_required(VERSION 3.16)
project(AresCore LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "" FORCE)
endif()

# ---------------------------------------------------------------- AresCore ---

file(GLOB ARES_CORE_SOURCES CONFIGURE_DEPENDS Source/AresCore/Private/*.cpp)

add_library(AresCore STATIC ${ARES_CORE_SOURCES})
target_include_directories(AresCore PUBLIC Source/AresCore/Public)

target_compile_options(AresCore PRIVATE
  -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion -Werror)

# Determinism guardrails. Fast-math would let the compiler reassociate the
# floating-point work in the clock and Kepler solver, which is exactly the kind
# of silent drift the determinism test exists to catch.
target_compile_options(AresCore PRIVATE -ffp-contract=off)

# ------------------------------------------------------------------- tests ---

file(GLOB ARES_TEST_SOURCES CONFIGURE_DEPENDS Tests/*.cpp Tests/Harness/*.cpp)

add_executable(AresCoreTests ${ARES_TEST_SOURCES})
target_link_libraries(AresCoreTests PRIVATE AresCore)
target_include_directories(AresCoreTests PRIVATE Tests Tests/Harness)
target_compile_options(AresCoreTests PRIVATE -Wall -Wextra -ffp-contract=off)
target_compile_definitions(AresCoreTests PRIVATE
  ARES_PROJECT_ROOT="${CMAKE_CURRENT_SOURCE_DIR}")

enable_testing()
add_test(NAME AresCore COMMAND AresCoreTests)
