# MARS

A realistic, single-player space-agency simulation. Congressional appropriations,
contractor relationships and program-cancellation risk are the game. You walk
your space center on foot, you ride Starship to orbit and to Mars — and you never
once fly it. Every launch, transfer and landing is executed by a flight director
system while you watch.

**Engine:** Unreal Engine 5.8 · C++ · `Ares.uproject`

---

## Status

**M0 — Skeleton. Complete.**

| Milestone | State |
|---|---|
| M0 — Skeleton | ✅ **Accepted** — verified headless and under UE 5.8 |
| M1 — Walk the Cape | in progress |
| M2 — The program is real | not started |
| M3 — One flight, end to end | not started |
| M4 — Ride it | not started |
| M5 — Destinations | not started |
| M6 — Depth | not started |

`AresCore` is verified twice over:

| Where | What | Result |
|---|---|---|
| Headless, clang/Linux | 42 cases, 21,207 assertions vs. the JS reference | ✅ pass |
| UBT, MSVC/Windows, UE 5.8 | `Ares.Core.Clock`, `.Rng`, `.Determinism` | ✅ pass |

All four modules compile under UnrealBuildTool. The determinism test — 2000
simulated days twice from one seed — produces the same state hash under both
toolchains, so the simulation does not depend on compiler or platform.

---

## Layout

```
Ares.uproject            UE 5.8 project, four modules
Config/                  Lumen + Nanite + VSM + TSR
Data/                    Balance and content tables, copied unchanged from the reference
Source/
  AresCore/              Pure simulation. NO Engine dependency, no UE headers at all.
  AresGame/              Actors, subsystems, player character  (M1+)
  AresUI/                UMG terminals and HUD                 (M1+)
  AresEditor/            Validation and JSON→DataTable import  (M2+)
Tests/                   Headless automation tests for AresCore
Tools/oracle/            Generates expected values from the JS reference
docs/PORT_ASSESSMENT.md  What carries over from the reference, and what does not
CMakeLists.txt           Headless build — verifies AresCore needs no engine
```

### Why there are two builds

`AresCore` must compile and unit-test headless (brief §3.1), so it contains no
Unreal headers — only standard C++20. UBT builds it from `AresCore.Build.cs` as
part of the game; CMake builds the *same sources* with no engine present.

That second build is not a convenience, it is the enforcement mechanism: **if
`AresCore` ever grows an engine dependency, the CMake build stops working.**

The single exception is `Source/AresCore/Private/Module/AresCoreModule.cpp`,
which holds the `IMPLEMENT_MODULE` boilerplate UBT requires. The CMake glob
deliberately does not reach into that subdirectory.

---

## Building and testing

### Headless (no Unreal required)

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j
./build/AresCoreTests                    # all suites
./build/AresCoreTests --filter Clock     # one suite
```

Built with `-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion
-Werror` and `-ffp-contract=off`. The float contraction flag is not cosmetic:
letting the compiler reassociate floating-point work would break determinism.

### Regenerating the test oracle

Expected values come from running the JS reference, per brief §9. The reference
ships no sim test script, so this generates one:

```bash
git clone https://github.com/jmwilson92/mars /path/to/mars
node Tools/oracle/dump-oracle.mjs --ref /path/to/mars
```

Rerun after any reference change; the C++ tests will report exactly what drifted.

### Under Unreal

```
Ares.uproject → right-click → Generate Project Files → build the Editor target
```

Automation tests: **Session Frontend → Automation → `Ares.Core`**, or headless:

```
UnrealEditor-Cmd Ares.uproject -ExecCmds="Automation RunTests Ares.Core" -unattended -nullrhi
```

---

## Rules that are not negotiable

1. **`AresCore` never includes `Engine.h`** — or any UE header. Needing one means
   the code belongs in `AresGame`.
2. **The player never controls the vehicle.** No throttle, no attitude, no
   staging, no landing, no "advanced mode". The decision lives in the stack, not
   the stick.
3. **Balance numbers live in `Data/`, never in C++.** If it is in a JSON table,
   read it from the table.
4. **Determinism is testable and stays green.** Same seed + same inputs = same run.
5. **No marketplace assets or third-party plugins** without explicit approval.

---

## Working notes: what the headless build cannot catch

`AresCore` is verified two ways, but `AresGame`, `AresUI` and `AresEditor` are
Engine-dependent and compile **only** under UnrealBuildTool. Failures found the
hard way, recorded so they are not rediscovered:

**MSVC C4458 — "declaration of X hides class member" is an ERROR under UE.**
GCC's `-Wshadow` does not flag a local that shadows a *class member* from
inside a member function, so the headless build stays green over it. This has
bitten twice: locals named `Time`/`Orbit`/`Economy` in a static `FAresData`
method, and locals named `Slot` in a `UUserWidget` method (`UWidget::Slot` is
inherited by every widget). Before writing a local, check it does not collide
with a base-class member — `Slot`, `Owner`, `Role`, `Tags`, `Children`,
`Canvas`, `RootComponent`, `InputComponent`, `Controller` are the usual traps.
Prefix constructor and setter parameters with `In`.

**Windows exports nothing by default.** UBT defines `ARESCORE_API` as
`DLLEXPORT`, itself a macro from UE's platform headers that `AresCore` does not
include — see `AresApi.h`. Build with `-DARES_SIMULATE_UBT_API=ON` to reproduce
that define headlessly.

**UBT property names drift between 5.x releases.** `bUseUnityBuild` is on
`TargetRules`, not `ModuleRules`; `bUseAVX` was superseded by `MinCpuArchX64`.
Prefer omitting a build-tuning property over guessing its current name.

**Declare the modules you actually call.** A transitive dependency may supply
headers without supplying link symbols. `AresGame` calls `UUserWidget` directly,
so it lists `UMG` itself rather than leaning on `AresUI` re-exporting it —
otherwise the build compiles and then fails at link with LNK2019.

**Close the editor before building.** Live Coding locks the binaries; the build
fails and the editor keeps running stale DLLs, which presents as classes
silently not existing (a flying `ADefaultPawn` instead of `AAresCharacter`).

---

## Reference implementation

The design document is the working web prototype at
[`jmwilson92/mars`](https://github.com/jmwilson92/mars) — Vite + three.js,
~10,800 lines. Port the **rules**, not the code; do not transpile JavaScript.
Read `docs/PORT_ASSESSMENT.md` before touching a system, and note that where the
brief and the reference disagree, the brief wins.
