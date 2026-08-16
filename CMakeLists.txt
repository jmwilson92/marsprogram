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

if(MSVC)
  # /utf-8 is REQUIRED, not cosmetic: the sources contain UTF-8 string literals
  # and DataTests asserts on their exact byte length. Without it MSVC decodes
  # them as the system code page and that test fails for the wrong reason.
  #
  # /fp:precise is MSVC's default; set explicitly so determinism does not
  # depend on a toolchain default staying put.
  #
  # No /WX here. The strict warnings-as-errors gate runs on Clang/GCC (below),
  # which is where the code was developed and is verified clean; turning it on
  # for a compiler this project has not been vetted against would fail the
  # build on warning-set differences rather than on real defects.
  set(ARES_WARNINGS /W4 /utf-8 /fp:precise /permissive-)
else()
  # Determinism guardrail: -ffp-contract=off stops the compiler fusing
  # multiply-add in the clock and Kepler solver, which is exactly the kind of
  # silent drift the determinism test exists to catch.
  set(ARES_WARNINGS
    -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion -Werror
    -ffp-contract=off)
endif()

target_compile_options(AresCore PRIVATE ${ARES_WARNINGS})

# ------------------------------------------------------------------- tests ---

file(GLOB ARES_TEST_SOURCES CONFIGURE_DEPENDS Tests/*.cpp Tests/Harness/*.cpp)

add_executable(AresCoreTests ${ARES_TEST_SOURCES})
target_link_libraries(AresCoreTests PRIVATE AresCore)
target_include_directories(AresCoreTests PRIVATE Tests Tests/Harness)

if(MSVC)
  target_compile_options(AresCoreTests PRIVATE /W3 /utf-8 /fp:precise)
else()
  target_compile_options(AresCoreTests PRIVATE -Wall -Wextra -ffp-contract=off)
endif()

target_compile_definitions(AresCoreTests PRIVATE
  ARES_PROJECT_ROOT="${CMAKE_CURRENT_SOURCE_DIR}")

enable_testing()
add_test(NAME AresCore COMMAND AresCoreTests)
