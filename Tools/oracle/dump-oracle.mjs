/**
 * ORACLE GENERATOR — expected values for the AresCore automation tests.
 *
 * Per the project brief §9: "Every ported system gets an automation test whose
 * expected values come from running the JS reference."
 *
 * This script imports the reference implementation's dependency-free core modules
 * (src/core/clock.js, src/core/rng.js, src/util/math.js) and dumps their outputs
 * to Tools/oracle/oracle.json. The C++ tests in Tests/ read that file and assert
 * bit-for-bit agreement. If the reference changes, regenerate and the C++ tests
 * will tell you exactly what drifted.
 *
 * The reference has no `npm run test:sim` script (package.json defines only
 * dev/build/preview) and the scripts/smoke-*.mjs files are render smokes that
 * require three.js. So this file IS the sim oracle; there was not one before.
 *
 * Usage:
 *   node Tools/oracle/dump-oracle.mjs --ref /path/to/mars > Tools/oracle/oracle.json
 */

import { writeFileSync } from 'node:fs';
import { resolve } from 'node:path';
import { pathToFileURL } from 'node:url';

function argValue(flag, fallback) {
  const i = process.argv.indexOf(flag);
  return i >= 0 && process.argv[i + 1] ? process.argv[i + 1] : fallback;
}

const REF = resolve(argValue('--ref', '/workspace/jmwilson92/mars'));
const OUT = resolve(argValue('--out', new URL('./oracle.json', import.meta.url).pathname));

const mod = (p) => import(pathToFileURL(resolve(REF, p)).href);

const clockMod = await mod('src/core/clock.js');
const rngMod = await mod('src/core/rng.js');
const mathMod = await mod('src/util/math.js');
const constMod = await mod('src/core/constants.js');

const { createClockState, advance, refreshDerived, earthDateFromDay, seasonName, orbitalDistanceFactor } = clockMod;
const { createRng } = rngMod;
const { wrapDeg, wrapRadPi, meanToTrueAnomaly, trueToMeanAnomaly, clamp, lerp } = mathMod;

/* ---------------------------------------------------------------- constants */

const constants = {
  SOL_SECONDS: constMod.SOL_SECONDS,
  EARTH_DAY_SECONDS: constMod.EARTH_DAY_SECONDS,
  SOLS_PER_EARTH_DAY: constMod.SOLS_PER_EARTH_DAY,
  EARTH_DAYS_PER_SOL: constMod.EARTH_DAYS_PER_SOL,
  MARS_YEAR_SOLS: constMod.MARS_YEAR_SOLS,
  SYNODIC_EARTH_DAYS: constMod.SYNODIC_EARTH_DAYS,
  MARS_ECCENTRICITY: constMod.MARS_ECCENTRICITY,
  LS_PERIHELION_DEG: constMod.LS_PERIHELION_DEG,
  SOLAR_CONSTANT_MEAN_WM2: constMod.SOLAR_CONSTANT_MEAN_WM2,
  EARTH_DAYS_LS0_TO_EPOCH: constMod.EARTH_DAYS_LS0_TO_EPOCH,
  MARS_YEAR_AT_LS0: constMod.MARS_YEAR_AT_LS0,
  FIRST_WINDOW_EARTH_DAY: constMod.FIRST_WINDOW_EARTH_DAY,
};

/* --------------------------------------------------------------- math cases */

const mathCases = {
  wrapDeg: [-720, -450, -1, 0, 359.9, 360, 361, 1080].map((x) => ({ in: x, out: wrapDeg(x) })),
  wrapRadPi: [-7, -3.2, -Math.PI, 0, Math.PI, 3.2, 7].map((x) => ({ in: x, out: wrapRadPi(x) })),
  clamp: [
    { in: [-5, 0, 10], out: clamp(-5, 0, 10) },
    { in: [5, 0, 10], out: clamp(5, 0, 10) },
    { in: [15, 0, 10], out: clamp(15, 0, 10) },
  ],
  lerp: [
    { in: [0, 10, 0], out: lerp(0, 10, 0) },
    { in: [0, 10, 0.25], out: lerp(0, 10, 0.25) },
    { in: [0, 10, 1], out: lerp(0, 10, 1) },
    { in: [-4, 4, 0.5], out: lerp(-4, 4, 0.5) },
  ],
  // Kepler round-trip at Mars eccentricity, the exact call the clock makes.
  meanToTrueAnomaly: [0, 15, 45, 90, 135, 180, 225, 270, 315, 359].map((m) => ({
    in: [m, constMod.MARS_ECCENTRICITY],
    out: meanToTrueAnomaly(m, constMod.MARS_ECCENTRICITY),
  })),
  trueToMeanAnomaly: [0, 15, 45, 90, 135, 225, 270, 315, 359].map((nu) => ({
    in: [nu, constMod.MARS_ECCENTRICITY],
    out: trueToMeanAnomaly(nu, constMod.MARS_ECCENTRICITY),
  })),
  orbitalDistanceFactor: [0, 45, 90, 180, 251, 270, 300].map((ls) => ({
    in: ls,
    out: orbitalDistanceFactor(ls),
  })),
};

/* -------------------------------------------------------------- clock cases */

function snapClock(c) {
  return {
    earthDay: c.earthDay,
    sol: c.sol,
    marsYear: c.marsYear,
    solOfYear: c.solOfYear,
    Ls: c.Ls,
    tickCount: c.tickCount,
    insolationFactor: c.insolationFactor,
    solarConstant_wm2: c.solarConstant_wm2,
    windowIndex: c.windowIndex,
    nextWindowEarthDay: c.nextWindowEarthDay,
    daysToWindow: c.daysToWindow,
    windowJustOpened: c.windowJustOpened,
  };
}

// Initial state, then a trace of Earth-day advances sampled at key days.
const clockInitial = snapClock(createClockState());

const SAMPLE_DAYS = new Set([
  1, 2, 7, 30, 90, 180, 209, 210, 211, 365, 500, 687, 779, 780, 990, 1000, 1500, 1560, 2000,
]);
const clockTrace = [];
{
  const st = { clock: createClockState() };
  for (let d = 1; d <= 2000; d++) {
    const r = advance(st);
    if (SAMPLE_DAYS.has(d)) {
      clockTrace.push({ day: d, windowOpened: r.windowOpened, ...snapClock(st.clock) });
    }
  }
}

// Sol-unit ticking (the Mars-surface tick unit) sampled the same way.
const solTrace = [];
{
  const st = { clock: createClockState() };
  st.clock.tickUnit = constMod.TICK_UNITS.SOL;
  for (let t = 1; t <= 700; t++) {
    const r = advance(st);
    if (t === 1 || t === 10 || t === 100 || t === 334 || t === 668 || t === 669 || t === 700) {
      solTrace.push({ tick: t, windowOpened: r.windowOpened, ...snapClock(st.clock) });
    }
  }
}

const dateCases = [0, 1, 31, 59, 365, 366, 730, 1461, 2000, 3652].map((d) => ({
  in: d,
  out: earthDateFromDay(d),
}));

const seasonCases = [0, 45, 89.9, 90, 179.9, 180, 269.9, 270, 359.9].map((ls) => ({
  in: ls,
  out: seasonName(ls),
}));

/* ---------------------------------------------------------------- rng cases */

// Fixed seeds; the C++ FAresRng must reproduce these exactly.
const rngSeeds = [0, 1, 42, 0xdeadbeef, 0x811c9dc5, 123456789];
const rngStreamNames = ['edl', 'incidents', 'politics', 'weather', 'crew'];

const rngCases = [];
for (const seed of rngSeeds) {
  const rng = createRng(seed >>> 0);
  const streams = {};
  for (const name of rngStreamNames) {
    const s = rng.stream(name);
    streams[name] = { first16: Array.from({ length: 16 }, () => s.next()) };
  }
  rngCases.push({ seed: seed >>> 0, streams });
}

// Stream independence: interleaving draws on one stream must not perturb another.
const rngIndependence = (() => {
  const a = createRng(42);
  const edlAlone = Array.from({ length: 8 }, () => a.stream('edl').next());
  const b = createRng(42);
  const edlInterleaved = [];
  for (let i = 0; i < 8; i++) {
    b.stream('incidents').next();
    b.stream('weather').next();
    edlInterleaved.push(b.stream('edl').next());
  }
  return { edlAlone, edlInterleaved };
})();

// Helper API surface (int/chance/pick) on a known seed.
const rngHelpers = (() => {
  const rng = createRng(2024);
  const s = rng.stream('helpers');
  return {
    ints_1_6: Array.from({ length: 12 }, () => s.int(1, 6)),
    floats_neg1_1: Array.from({ length: 8 }, () => s.float(-1, 1)),
    chance_p30: Array.from({ length: 16 }, () => s.chance(0.3)),
  };
})();

// Serialize / restore must resume the identical sequence.
const rngSerialization = (() => {
  const rng = createRng(7);
  const s = rng.stream('edl');
  const before = Array.from({ length: 5 }, () => s.next());
  const blob = rng.serialize();
  const after = Array.from({ length: 5 }, () => s.next());
  const restored = createRng(7, blob);
  const afterRestore = Array.from({ length: 5 }, () => restored.stream('edl').next());
  return { before, after, afterRestore, serialized: blob };
})();

/* ------------------------------------------------------------------- output */

const oracle = {
  _generator: 'Tools/oracle/dump-oracle.mjs',
  _reference: REF,
  _generatedFrom: 'jmwilson92/mars @ src/core + src/util',
  constants,
  math: mathCases,
  clock: {
    initial: clockInitial,
    earthDayTrace: clockTrace,
    solTrace,
    earthDate: dateCases,
    season: seasonCases,
  },
  rng: {
    streamNames: rngStreamNames,
    cases: rngCases,
    independence: rngIndependence,
    helpers: rngHelpers,
    serialization: rngSerialization,
  },
};

// Full float precision matters — these are exact-match assertions downstream.
writeFileSync(OUT, JSON.stringify(oracle, null, 2) + '\n');
process.stderr.write(`oracle written: ${OUT}\n`);
