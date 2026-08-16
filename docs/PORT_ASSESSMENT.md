# Port assessment — reference web prototype → Unreal

**Status:** reconstructed. The project brief says to read
`../reference/docs/PORT_ASSESSMENT.md` first. That file does not exist — the
reference repo (`jmwilson92/mars`) has no `docs/` directory at all. This
document is its replacement, written from a direct read of the reference at
commit `9e42718`.

Reference: 10,835 lines of ES modules (Vite + vanilla JS + three.js).

| Area | Lines | Verdict |
|---|---:|---|
| `src/core/` | 745 | **Port.** Dependency-free, deterministic, the real spine. |
| `src/sim/` | 1,864 | **Port**, in milestone order. Rules only, not structure. |
| `src/util/` | 95 | **Port.** Trivial, exact. |
| `src/render/` | 6,803 | **Reference only.** three.js scene graph; Unreal replaces it. Ship + MC geometry is a blockout spec. |
| `src/ui/` | 1,051 | **Reference only.** DOM panels; UMG replaces them. Read for information architecture. |
| `data/*.json` | 7 files | **Copy unchanged.** Done — see `Data/`. |

---

## What has been ported (M0)

| Reference | Ares | Notes |
|---|---|---|
| `src/util/math.js` | `AresCore/AresMath.{h,cpp}` | Exact. Kepler solver iteration count and tolerance preserved. |
| `src/core/rng.js` | `AresCore/AresRng.{h,cpp}` | **Bit-exact.** Verified against the JS oracle. |
| `src/core/clock.js` | `AresCore/SimClock.{h,cpp}` | Dual calendar, Ls, synodic windows. Calendar math replaced (see below). |
| `src/core/constants.js` | `AresCore/AresData.{h,cpp}` | **Deliberately not ported as constants** — see "Conflicts" #1. |
| `src/core/state.js` | `AresCore/ProgramState.{h,cpp}` | Restructured — see "Conflicts" #2. |
| `src/core/loader.js` | `AresCore/AresData.{h,cpp}` | Validation preserved; fails loudly on missing fields. |

Verified by 42 test cases / ~21,200 assertions in `Tests/`, with expected values
generated from the JS reference by `Tools/oracle/dump-oracle.mjs`.

---

## Conflicts with the reference (brief §9 requires flagging these)

### 1. Balance constants live in data, not in code

`src/core/constants.js` hardcodes `SOL_SECONDS`, `MARS_YEAR_SOLS`,
`SYNODIC_EARTH_DAYS`, `MARS_ECCENTRICITY`, `LS_PERIHELION_DEG`,
`SOLAR_CONSTANT_MEAN_WM2` and `FIRST_WINDOW_EARTH_DAY` — every one of which
*also* appears in `data/balance.json`. The file's own header concedes the debt:

> `B2 will fold these into /data/balance.json; clock reads this module so that swap is one import.`

Brief §9 forbids repeating this. `FSimClock` therefore takes `FAresData` and
holds no constants of its own. `Tests/ClockTests.cpp :: BalanceDerivedConstantsMatchReference`
asserts the data file and the reference's hardcoded values agree, so the two
cannot silently drift apart.

Genuinely absent from `balance.json`, and so still in C++ (`AresConstants.h`):
the campaign calendar pins — epoch `2040-01-01`, `MY46 Ls=0` at `2039-11-30`,
`MARS_YEAR_AT_LS0 = 46`. These have no balance meaning.

### 2. One mutable blob → eight sub-structs

`createInitialState()` returns a single nested object (`state.earth.budget`,
`state.earth.politics`, `state.mars.power`, …) that every system reaches into
freely. Brief §3.1 calls this a mistake. `FProgramState` splits it into
`FBudgetState`, `FPoliticsState`, `FResearchState`, `FFleetState`,
`FMissionState`, `FCrewState`, `FColonyState`, `FContractState`, each owned by
exactly one tick function.

Note the reference's shape is Earth/Mars-partitioned, not system-partitioned;
the split is a genuine restructure, not a rename.

### 3. `src/sim` → `src/render` coupling is real but shallow

The brief flags this. It is exactly three files importing one symbol:

```
src/sim/research.js:2   import { MISSION_TYPES } from '../render/phases.js';
src/sim/planning.js:1   import { MISSION_TYPES } from '../render/phases.js';
src/sim/fleet.js:1      import { MISSION_TYPES } from '../render/phases.js';
```

`MISSION_TYPES` and `PHASE` are simulation vocabulary that got filed under
`render/` by accident. They move into `AresCore` in M3. No further sim→render
coupling exists — everything else in `src/sim/` imports only from `core/`,
`util/` or `sim/`.

### 4. Date math

The reference derives the calendar from `Date.UTC()` milliseconds and divides.
`SimClock.cpp` uses integer proleptic-Gregorian civil-date arithmetic
(`DaysFromCivil` / `CivilFromDays`) instead, so the calendar cannot accumulate
floating-point drift over a 45-year campaign. Both agree that
`EARTH_DAYS_LS0_TO_EPOCH == 32`; the test asserts it.

### 5. `cabin.decks: 3`

`balance.json` says three decks; brief §4.3 specifies seven deck functions
(flight/cupola, crew, galley, science, ECLSS, airlock, payload). The brief wins.
Flagging it because the number is currently load-bearing in the reference's
`deckBuilder.js`, and M4 will need to change the data rather than work around it.

---

## Data gaps — content that must be authored, not ported

The tables are **Mars-only**. The brief's Moon content has no data behind it.

| Gap | Detail | Milestone |
|---|---|---|
| Lunar delta-v legs | `deltaV_mps` has the Mars legs only. `leoToTli`, `tliToLlo`, `lloToSurface`, `surfaceToLlo`, `lloToTei` are absent; the brief supplies values. | M5 |
| Lunar sites | `sites.json` is 10 Mars records with no `body` discriminator. §4.4 wants one parameterized map serving both. | M5 |
| Per-body physics | `balance.json` has a single `orbit` block hardcoding Mars (`gravity_mps2: 3.72`, `radius_km: 3389.5`). The Moon needs 1.62 / 1737.4 and its own sky/atmosphere record. | M5 |
| Contracts | `contractors.json` has contractors but no contract vehicle — cost-plus vs fixed-price, milestone payouts and protests are new design. | M6 |

Two tripwire tests guard these so they cannot be quietly hardcoded in C++
instead of added to the data files:
`DataTests.cpp :: MarsDeltaVLegsPresentLunarLegsNotYet` and
`:: SitesTableIsMarsOnly`. **Both are expected to fail when M5 lands, and must
be flipped rather than deleted.**

---

## No test oracle existed

The brief calls `../reference/npm run test:sim` the oracle. That script does not
exist — `package.json` defines only `dev`, `build`, `preview`. The four
`scripts/smoke-*.mjs` files are *render* smokes that `import * as THREE` and
will not run without `npm install`.

`Tools/oracle/dump-oracle.mjs` is the replacement: it imports the reference's
dependency-free core modules and dumps expected values to `oracle.json`, which
the C++ tests assert against. Regenerate whenever the reference changes:

```bash
node Tools/oracle/dump-oracle.mjs --ref /path/to/mars
```

---

## Carry-over quality by system

| System | Reference | Assessment |
|---|---:|---|
| `clock.js` | 132 | Clean. Ported verbatim in rules. |
| `rng.js` | 108 | Clean. Ported bit-exact. |
| `economy.js` | 24 | Thin — `annual/4` quarterly, hoarding above 2×. Brief §7's appropriations cycle is nearly all new. |
| `politics.js` | 35 | Thin. Drift constants confirmed accurate. Committees, elections and hearings are all new. |
| `research.js` | 139 | Solid. RP from flown milestones; labs at 3/day, 4 with crew. Minimum durations already modelled. |
| `fleet.js` | 467 | Substantial and worth close reading. Hull identity, reuse, wear. |
| `planning.js` | 365 | Substantial. Closest thing to the flight director's commit-time solve. |
| `crew.js` | 176 | Good. Dose tracking against the 1000 mSv career limit. |
| `colony.js` + `power`/`brownout`/`pipeline` | 343 | Well developed, but M6 per brief §7 — explicitly not milestone-critical. |
| `worker.js` | 267 | Structural only. The Web Worker becomes a UE background task (§3.2). |

Roughly half of `src/sim/` is colony survival, which the brief defers to last.
The agency-campaign systems the brief centres on — contracts, appropriations,
accident review boards, elections — are largely **new design**, not ports.
