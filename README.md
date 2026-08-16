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

**Close the editor first**, then from the repo root:

```
.\Build.bat
```

That builds the `AresEditor` target, Win64 Development. It finds the engine via
`UE_ROOT`, then the registry, then the usual install paths; set `UE_ROOT`
yourself if it cannot. `Build.bat Ares` builds the packaged game target instead,
and a second argument picks the configuration.

The IDE route is equivalent:

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

## Art direction

**Stylized, not photoreal.** Saturated palette, readable silhouettes over
surface detail, soft wide-angle sun, bloom carried high. Closer to a stylized
console game than to a NASA documentary.

This deliberately overrules the original brief §1, which specified "grounded,
procedural, NASA-flight-controller. Not comedic. Not arcade." That direction
still governs TONE — the writing, the callouts, the terminal language stay dry
and institutional. It no longer governs the RENDER.

Two consequences worth knowing:

**Art is assignable, not hardcoded.** `ACapeCampus` draws everything through
`FCapeMeshSlot` properties. Each slot takes a mesh and a material; the layout
code only ever asks for sizes in centimetres and each slot works out its own
scale from the assigned mesh's bounds. Drop a Fab asset into a slot and the
campus redraws with it — no layout code changes. Leave a slot empty and you get
an engine primitive with a flat tint, which is the programmer-art fallback and
is what it looks like today.

**Lighting is code, art is not.** `ACapeSky` owns sun, atmosphere, sky light,
fog, clouds and colour grading, all as properties. That is the largest visual
lever available without binary assets, and it is why the same blockout can look
like a debug scene or like a place depending only on numbers.

Marketplace and Fab content is approved for this project, overriding brief §2.

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

**A child component's mobility may not be stricter than its parent's.** Attaching
a Static component to a Movable root does not error — Unreal logs a warning,
aborts the attach, and the component renders in *world* space while everything
else in the actor renders in *actor* space. Two frames, silently. Symptom:
geometry looks nearly right but anything positioned via `GetActorTransform()`
lands offset from it.

**A placed actor keeps the defaults it was placed with.** `ACapeCampus` builds
its `Buildings` array in the constructor, but an instance already in a level has
that array *serialized*. Changing a constructor default does not reach it, and a
newly added struct member loads as its C++ initializer rather than the value the
constructor would have set. After changing layout defaults, delete the placed
actor and drop a fresh one — or right-click the property and Reset to Default.

**Close the editor before building.** Live Coding locks the binaries; the build
fails and the editor keeps running stale DLLs, which presents as classes
silently not existing (a flying `ADefaultPawn` instead of `AAresCharacter`).

**A braced list in a range-for must be homogeneous.** `for (UBase* P : { A, B,
Derived })` does not compile: the range expression deduces
`std::initializer_list<T>` from its *elements*, the loop variable's declared
type gets no say, and derived-to-base is not applied during that deduction. One
derived pointer in the list makes `T` undeducible — MSVC C3535. `GroundCover`
is the hierarchical subclass and needs an explicit `static_cast` to sit in the
same list as the plain instanced components. GCC catches this too, but only if
the loop is in a file the headless build compiles, which no `AresGame` file is.

**Decoration collides.** Anything `ACapeCampus` draws through a paint bucket is
a real, blocking primitive, so a facade band run across the door face is a
barricade at shin height across the only way into the building — geometrically
correct, visually invisible, and it makes the room unreachable. Every band or
prop that crosses a doorway has to be punched the way `AddWall` punches a wall.
`AddBelt` does this; anything new that skins a face must too.

**Two arrays holding the same components must be cleared together.**
`ClearGenerated()` destroys everything in `Generated`, and the paint buckets are
tracked in `PaintBuckets` as well. Resetting only the first leaves the second
full of destroyed components that `GetPaint` will hand straight back — no crash,
no warning, the dressing pass just draws nothing on the second rebuild.

**Nothing may fall through to `StructureSlot` by accident.**
`SlotForComponent` returns it for any component it does not recognise, which
silently applies the walls' fit mode and random yaw to the caller. Components
that are not art slots need an explicit case — `PaintSlot` is the fixed one the
paint buckets use.

---

## Reference implementation

The design document is the working web prototype at
[`jmwilson92/mars`](https://github.com/jmwilson92/mars) — Vite + three.js,
~10,800 lines. Port the **rules**, not the code; do not transpile JavaScript.
Read `docs/PORT_ASSESSMENT.md` before touching a system, and note that where the
brief and the reference disagree, the brief wins.
