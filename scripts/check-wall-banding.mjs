// Wall-banding audit: renders the STRUCTURE terms of the cliff paint on a synthetic
// vertical wall through the REAL stratigraphic column and the REAL joint sets, and
// measures how much of the albedo variance is locked to bed index (horizontal banding)
// or to strike-parallel columns (vertical striping).
//
// Mirrors the wall path (wall = 1) of surface-shader.js line by line:
//   - wallBed: one quiet rock tone + aperiodic district drift (NO bed structure)
//   - joint grooves + lip-gated bedding ledges (hard-over-soft only, broken along strike)
//   - per-column block tone with vertical drift (NO bed-index term)
//   - runoff streaks with district patchiness + distance fade
// Deliberately OMITS the aperiodic finish terms (grain tint, oxide pockets, spalls,
// flakes, moss): they cannot create bands, they only dilute the bed-locked share — so
// leaving them out makes this test STRICTER than the full render.
//
// One emulation: the shader fades streaks by camera distance; the audit renders two
// footprints instead (near 0.8 m px = full streaks, far 3 m px = halved streaks).
//
// Usage: node scripts/check-wall-banding.mjs [--preset "Sea cliffs"]
// Exit 1 on FAIL (regression gate for `npm run check:wall`).

import { PNG } from 'pngjs';
import { writeFileSync } from 'node:fs';
import { defaults, presets, palettes, paletteKeys } from '../src/params.js';
import { makeBedTable, bedAt, bedJitter } from '../src/strata-model.js';
import { makeJointSets, jointField, jshash } from '../src/joint-sets.js';

const args = process.argv.slice(2);
const presetName = args.includes('--preset') ? args[args.indexOf('--preset') + 1] : 'Sea cliffs';

// ---- merged values exactly as the app builds them --------------------------------
function mergedValues(name) {
  const preset = presets[name];
  if (!preset) throw new Error(`unknown preset ${name}`);
  const v = { ...defaults, ...preset };
  const pal = palettes[preset.palette || 'granite'];
  for (const k of paletteKeys) if (preset[k] === undefined) v[k] = pal[k];
  return v;
}

// ---- exact port of cgHash3 / cgNoise3 (value only; albedo needs no gradients) ----
function cgHash3(x, y, z) {
  const X = (x + 4096) >>> 0, Y = (y + 4096) >>> 0, Z = (z + 4096) >>> 0;
  let h = (Math.imul(X, 1597334677) ^ Math.imul(Y, 3812015801) ^ Math.imul(Z, 2798796415)) >>> 0;
  h = Math.imul(h ^ (h >>> 16), 2246822519) >>> 0;
  h = Math.imul(h ^ (h >>> 13), 3266489917) >>> 0;
  h = (h ^ (h >>> 16)) >>> 0;
  return h / 4294967295;
}
function cgNoise3(x, y, z) {
  const cx = Math.floor(x), cy = Math.floor(y), cz = Math.floor(z);
  const fx = x - cx, fy = y - cy, fz = z - cz;
  const ux = fx * fx * fx * (fx * (fx * 6 - 15) + 10);
  const uy = fy * fy * fy * (fy * (fy * 6 - 15) + 10);
  const uz = fz * fz * fz * (fz * (fz * 6 - 15) + 10);
  const a = cgHash3(cx, cy, cz), b = cgHash3(cx + 1, cy, cz);
  const c = cgHash3(cx, cy + 1, cz), d = cgHash3(cx + 1, cy + 1, cz);
  const e = cgHash3(cx, cy, cz + 1), f = cgHash3(cx + 1, cy, cz + 1);
  const g = cgHash3(cx, cy + 1, cz + 1), h = cgHash3(cx + 1, cy + 1, cz + 1);
  const ab = a + (b - a) * ux, cd = c + (d - c) * ux;
  const ef = e + (f - e) * ux, gh = g + (h - g) * ux;
  return (ab + (cd - ab) * uy) * (1 - uz) + (ef + (gh - ef) * uy) * uz;
}

const clamp01 = (t) => (t < 0 ? 0 : t > 1 ? 1 : t);
const sstep = (a, b, t) => { const u = clamp01((t - a) / (b - a)); return u * u * (3 - 2 * u); };
const mix = (a, b, t) => a + (b - a) * t;
const lum = (c) => (c[0] + c[1] + c[2]) / 3;
function hexLin(hex) { // sRGB hex → linear working triple (as THREE.Color.setStyle stores it)
  const n = parseInt(hex.slice(1), 16);
  const f = (u) => { const s = u / 255; return s <= 0.04045 ? s / 12.92 : Math.pow((s + 0.055) / 1.055, 2.4); };
  return [f((n >> 16) & 255), f((n >> 8) & 255), f(n & 255)];
}

// ---- the wall --------------------------------------------------------------------
const v = mergedValues(presetName);
const table = makeBedTable(v);
const jsets = makeJointSets(v.seed || 1);
const rockA = hexLin(v.rockA), rockB = hexLin(v.rockB);
const dipRad = (v.strataDip * Math.PI) / 180, dirRad = (v.strataDipDirection * Math.PI) / 180;
const dipX = Math.tan(dipRad) * Math.cos(dirRad), dipZ = Math.tan(dipRad) * Math.sin(dirRad);
const lateral = v.strataLateral == null ? 0.18 : v.strataLateral;
const bandScale = v.strataBandScale || 1;
const jointDomain = Math.max(14, (v.jointSpacing || 6) * 5);
const jsp = Math.min(60, Math.max(1.5, v.jointSpacing || 6));
const wallBase = lum([mix(rockB[0], rockA[0], 0.55), mix(rockB[1], rockA[1], 0.55), mix(rockB[2], rockA[2], 0.55)]);

// Wall: 200 m of strike × 150 m of height, placed mid-tile. Sea cliffs faces sit
// roughly y ∈ [seaLevel, seaLevel + 150]; other presets use the same window shape.
const W2 = 256, H2 = 192, SPAN_X = 200, SPAN_Y = 150;
const X0 = 60, Z0 = 40;
const Y0 = (v.seaLevel != null && v.waterEnabled ? v.seaLevel : 0) + 5;

function evalWallPixel(x, y, footprint) {
  const z = Z0;
  const sxj = x + v.worldSize / 2, szj = z + v.worldSize / 2;
  const bandY = y + dipX * sxj + dipZ * szj;
  const warp = (cgNoise3(x * 0.012, y * 0.012, z * 0.012) * 1.5 + cgNoise3(x * 0.07 + 3, y * 0.07, z * 0.07) * 0.5) * 0.15;
  const t = (bandY + warp) / (bedJitter(sxj, szj, lateral) * bandScale);
  const bed = bedAt(table, t);
  const bf = bed.f, bedHard = bed.hard;
  // wallBed — the wall base: NO bed structure, only district drift (wallFlat = 1)
  const distDrift = (cgNoise3(x * 0.016 + 4.2, y * 0.016 + 4.2, z * 0.016 + 4.2) - 0.5) * 0.16
    + (cgNoise3(x * 0.05 + 9.1, y * 0.05 + 9.1, z * 0.05 + 9.1) - 0.5) * 0.1;
  const bandCol = wallBase * (1 + distDrift);
  const seamNoise = sstep(0.35, 0.7, cgNoise3(x * 0.09, y * 0.4, z * 0.09));
  // joints — the REAL shared joint field
  const bedThickM = Math.max(0.3, bed.thick);
  const spacing = jsp * Math.min(1.6, Math.max(0.65, 0.65 + (bedThickM / Math.max(0.5, jsp)) * 0.35));
  const jf = jointField(x, z, bed.index, jsets, jsp, bed.thick, {
    width: v.jointWidth || 0.35, dropout: v.jointDropout == null ? 0.3 : v.jointDropout,
    stagger: v.jointStagger == null ? 1 : v.jointStagger, domain: jointDomain,
    loss: v.sdfBlockLoss == null ? 0.12 : v.sdfBlockLoss, vary: v.jointVary == null ? 0.7 : v.jointVary,
  });
  // lip-gated bedding ledge (hard-over-soft only, broken along strike)
  const hardN = bf > 0.5 ? table.hard[Math.min(bed.index + 1, table.count - 1)] : table.hard[Math.max(bed.index - 1, 0)];
  let lip = clamp01((bf > 0.5 ? hardN - bedHard : bedHard - hardN) * 2.2);
  lip *= 0.35 + 0.65 * seamNoise;
  const dB = Math.min(bf, 1 - bf) * bedThickM;
  const gB = (1 - sstep(0, (v.jointWidth || 0.35) * 1.2, dB)) * lip;
  const jointFade = 1 - sstep(0.25, 1.0, footprint / (v.jointWidth || 0.35));
  const patch = clamp01(cgNoise3(x / 145 + 21, y / 145, z / 145) * 0.5 + 0.5);
  const faceVar = mix(1, 0.3 + 1.35 * patch, v.facePatch == null ? 0.5 : v.facePatch);
  const jointAmt = (v.jointStrength || 0) * jointFade * 1 * mix(1, 0.3 + 1.15 * faceVar, 0.85);
  // per-column block tone with vertical drift (no bed term — brick courses are gone)
  const bkey = jf.cellA * 73 + jf.cellB * 191;
  const hb = jshash(bkey, 7, 501) - 0.5;
  const drift = 0.45 + 0.55 * cgNoise3(bkey * 0.37, y * 0.02, 7.0);
  const toneFade = (1 - sstep(spacing * 0.5, spacing * 2.0, footprint)) * (v.jointBlocks == null ? 0.6 : v.jointBlocks);
  const blockTone = hb * 0.3 * toneFade * drift;
  const crackAO = Math.max(Math.max(jf.groove, gB), jf.hair * 0.4) * jointAmt;
  let rock = [0, 1, 2].map((i) => mix(rockA[i], bandCol, (v.strataContrast == null ? 0.7 : v.strataContrast) * 1));
  rock = rock.map((c) => c * (1 + blockTone));
  rock = rock.map((c) => c * (1 - crackAO * 0.42));
  // (hardness tint is gated off walls: × (1 − wallFlat) = 0 — nothing to do)
  // runoff streaks with district patchiness + distance fade (footprint emulates distance)
  const st = cgNoise3(x / v.streakScale, (y * 0.05) / v.streakScale, z / v.streakScale) * 0.6
    + cgNoise3((x * 3.6) / v.streakScale + 5, (y * 0.1) / v.streakScale + 5, (z * 3.6) / v.streakScale + 5) * 0.4;
  const streakPatch = cgNoise3(x * 0.02 + 13, y * 0.02 + 13, z * 0.02 + 13);
  let streak = sstep(0.56, 0.78, st) * (v.streakAmount == null ? 1 : v.streakAmount) * mix(0.3, 1.0, sstep(0.35, 0.65, streakPatch));
  streak *= footprint > 1.5 ? 0.45 : 1.0;
  const wet = Math.min(1, Math.max(0, (v.wetness == null ? 0.6 : v.wetness) * streak * 0.7));
  rock = rock.map((c) => c * (1 - wet * 0.5));
  return { rgb: rock, bed: bed.index };
}

function render(footprint, spanX = SPAN_X, spanY = SPAN_Y, x0 = X0, y0 = Y0) {
  const lumGrid = new Float32Array(W2 * H2);
  const beds = new Int32Array(W2 * H2);
  const png = new PNG({ width: W2, height: H2 });
  for (let j = 0; j < H2; j++) {
    for (let i = 0; i < W2; i++) {
      const x = x0 + (i / (W2 - 1)) * spanX;
      const y = y0 + (1 - j / (H2 - 1)) * spanY; // row 0 = top
      const r = evalWallPixel(x, y, footprint);
      lumGrid[j * W2 + i] = lum(r.rgb);
      beds[j * W2 + i] = r.bed;
      const o = (j * W2 + i) * 4;
      for (let c = 0; c < 3; c++) png.data[o + c] = Math.round(255 * Math.pow(Math.max(0, r.rgb[c]), 1 / 2.0));
      png.data[o + 3] = 255;
    }
  }
  return { lumGrid, beds, png };
}

function quantile(sorted, q) {
  const i = Math.min(sorted.length - 1, Math.max(0, q * (sorted.length - 1)));
  const lo = Math.floor(i), hi = Math.ceil(i);
  return sorted[lo] + (sorted[hi] - sorted[lo]) * (i - lo);
}

function metrics(lumGrid, beds) {
  let mean = 0;
  for (const u of lumGrid) mean += u;
  mean /= lumGrid.length;
  const rows = [];
  for (let j = 0; j < H2; j++) {
    let s = 0;
    for (let i = 0; i < W2; i++) s += lumGrid[j * W2 + i];
    rows.push(s / W2 / mean);
  }
  rows.sort((a, b) => a - b);
  const rowRange = quantile(rows, 0.95) - quantile(rows, 0.05);
  const cols = [];
  for (let i = 0; i < W2; i++) {
    let s = 0;
    for (let j = 0; j < H2; j++) s += lumGrid[j * W2 + i];
    cols.push(s / H2 / mean);
  }
  cols.sort((a, b) => a - b);
  const colRange = quantile(cols, 0.95) - quantile(cols, 0.05);
  const groups = new Map();
  let tot = 0;
  for (let k = 0; k < lumGrid.length; k++) {
    const d = lumGrid[k] / mean - 1;
    tot += d * d;
    const g = groups.get(beds[k]) || { n: 0, s: 0 };
    g.n++; g.s += lumGrid[k] / mean - 1;
    groups.set(beds[k], g);
  }
  let between = 0;
  for (const g of groups.values()) between += (g.s * g.s) / g.n;
  const bedShare = tot > 1e-9 ? between / tot : 0;
  const ids = [...groups.keys()].sort((a, b) => a - b);
  let maxStep = 0;
  for (let k = 1; k < ids.length; k++) {
    const a = groups.get(ids[k - 1]), b = groups.get(ids[k]);
    maxStep = Math.max(maxStep, Math.abs(a.s / a.n - b.s / b.n));
  }
  return { mean, rowRange, colRange, bedShare, maxStep, nBeds: ids.length };
}

const t0 = Date.now();
const near = render(SPAN_X / W2);   // footprint ≈ 0.8 m — close inspection
const far = render(3.0);            // footprint 3 m — the postcard view
const close = render(0.12, 24, 18); // 0.1 m px over 24×18 m — joints + ledges resolved (eyeball only)
const mn = metrics(near.lumGrid, near.beds);
const mf = metrics(far.lumGrid, far.beds);
const mc = metrics(close.lumGrid, close.beds);
console.log(`preset: ${presetName}  wall x∈[${X0},${X0 + SPAN_X}] y∈[${Y0.toFixed(0)},${(Y0 + SPAN_Y).toFixed(0)}]  (${Date.now() - t0} ms)`);
console.log(`near (0.8 m px): beds=${mn.nBeds} rowRange=${mn.rowRange.toFixed(3)} colRange=${mn.colRange.toFixed(3)} bedShare=${mn.bedShare.toFixed(3)} maxStep=${mn.maxStep.toFixed(3)}`);
console.log(`far  (3 m px):   beds=${mf.nBeds} rowRange=${mf.rowRange.toFixed(3)} colRange=${mf.colRange.toFixed(3)} bedShare=${mf.bedShare.toFixed(3)} maxStep=${mf.maxStep.toFixed(3)}`);
console.log(`close (0.1 m px, 24 m span): beds=${mc.nBeds} rowRange=${mc.rowRange.toFixed(3)} colRange=${mc.colRange.toFixed(3)} bedShare=${mc.bedShare.toFixed(3)} maxStep=${mc.maxStep.toFixed(3)}`);
writeFileSync('/tmp/wall-near.png', PNG.sync.write(near.png));
writeFileSync('/tmp/wall-far.png', PNG.sync.write(far.png));
writeFileSync('/tmp/wall-close.png', PNG.sync.write(close.png));
console.log('wrote /tmp/wall-near.png /tmp/wall-far.png /tmp/wall-close.png');

const fails = []; // declared early: the groove-tracking probe below pushes here

// Groove tracking: can the eye follow joint grooves across bed boundaries? Sample the
// REAL joint groove field on a (strike × height) grid through the real beds; correlate
// adjacent rows within one bed (grooves exist and continue) vs across a boundary (the
// lattice jogs by stagger + rescale, 30% of grooves terminate by dropout). Trackable
// grooves are the zebra's skeleton: within must be ~1, across must break below half.
{
  const rows = 100, cols = 200, yA = Y0, yB = Y0 + SPAN_Y, SPAN_S = 40;
  const grid = new Float32Array(rows * cols);
  const rowBed = new Int32Array(rows);
  for (let r = 0; r < rows; r++) {
    const y = yA + (r / (rows - 1)) * (yB - yA);
    const bedVotes = new Map();
    for (let c = 0; c < cols; c++) {
      const x = X0 + (c / (cols - 1)) * SPAN_S;
      const z = Z0;
      const sxj = x + v.worldSize / 2, szj = z + v.worldSize / 2;
      const t = (y + dipX * sxj + dipZ * szj) / (bedJitter(sxj, szj, lateral) * bandScale);
      const bed = bedAt(table, t);
      bedVotes.set(bed.index, (bedVotes.get(bed.index) || 0) + 1);
      const jf = jointField(x, z, bed.index, jsets, jsp, bed.thick, {
        width: v.jointWidth || 0.35, dropout: v.jointDropout == null ? 0.3 : v.jointDropout,
        stagger: v.jointStagger == null ? 1 : v.jointStagger, domain: jointDomain,
        loss: 0, vary: v.jointVary == null ? 0.7 : v.jointVary,
      });
      grid[r * cols + c] = jf.groove;
    }
    let best = -1, bn = -1;
    for (const [k, n] of bedVotes) if (n > bn) { bn = n; best = k; }
    rowBed[r] = best;
  }
  // Masked to the groove support (cols where EITHER row has groove energy): grooves
  // are sparse, and unmasked Pearson is inflated by the shared all-zero background.
  const rowCorr = (r1, r2) => {
    const xs = [], ys = [];
    for (let c = 0; c < cols; c++) {
      const a = grid[r1 * cols + c], b = grid[r2 * cols + c];
      if (a > 0.05 || b > 0.05) { xs.push(a); ys.push(b); }
    }
    if (xs.length < 10) return NaN;
    const mx = xs.reduce((s, t) => s + t, 0) / xs.length, my = ys.reduce((s, t) => s + t, 0) / ys.length;
    let sxy = 0, sxx = 0, syy = 0;
    for (let k = 0; k < xs.length; k++) { sxy += (xs[k] - mx) * (ys[k] - my); sxx += (xs[k] - mx) * (xs[k] - mx); syy += (ys[k] - my) * (ys[k] - my); }
    return sxx < 1e-9 || syy < 1e-9 ? NaN : sxy / Math.sqrt(sxx * syy);
  };
  let wSum = 0, wN = 0, aSum = 0, aN = 0;
  for (let r = 0; r + 1 < rows; r++) {
    const cc = rowCorr(r, r + 1);
    if (Number.isNaN(cc)) continue;
    if (rowBed[r] === rowBed[r + 1]) { wSum += cc; wN++; } else { aSum += cc; aN++; }
  }
  const within = wN ? wSum / wN : NaN, across = aN ? aSum / aN : NaN;
  const f1 = (x) => (Number.isNaN(x) ? 'n/a' : x.toFixed(3));
  console.log(`grooves: row-correlation within-bed=${f1(within)} (n=${wN}) across-bed=${f1(across)} (n=${aN})`);
  if (Number.isNaN(within) || Number.isNaN(across)) fails.push('groove tracking starved');
  else {
    if (!(within > 0.8)) fails.push(`grooves not continuous within beds (${within.toFixed(3)})`);
    if (!(across < 0.5)) fails.push(`grooves trackable across beds — zebra skeleton (${across.toFixed(3)})`);
  }
}

// Regression gates: the far view is what the user judges. Bed-locked structure must be
// a whisper there, and vertical striping must stay well below the joint spacing scale.
const gate = (name, val, lim) => { if (val > lim) fails.push(`${name}=${val.toFixed(3)} > ${lim}`); };
gate('far.rowRange', mf.rowRange, 0.1);
gate('far.bedShare', mf.bedShare, 0.12);
gate('far.maxStep', mf.maxStep, 0.08);
gate('far.colRange', mf.colRange, 0.12);
gate('near.rowRange', mn.rowRange, 0.16);
gate('near.maxStep', mn.maxStep, 0.12);
if (fails.length) { console.log(`WALL-BANDING: FAIL — ${fails.join('; ')}`); process.exit(1); }
console.log('WALL-BANDING: PASS');
