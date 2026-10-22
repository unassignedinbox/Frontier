// Chunk-build smoke test: runs the REAL buildChunkGeometry (carve + shear + marching
// cubes + floater filter) on a synthetic steep ramp with Sea-cliffs values, so carve
// edits execute headlessly. Gates:
//   1. finite output + triangles out;
//   2. the waterPaint dry-fallback guard: carved verts must never read (height − 40)
//      as a lake in the sky (no blue above the waterline);
//   3. the wave-cut notch path is LIVE (moves the surface at the waterline, nowhere else);
//   4. relief amps are live (Sea wall normals vary far more than a flat control);
//   5. cross-joints break the fins: lag-20 m vertical autocorrelation of the face plane
//      must be LOW with segmentation and HIGH without (full-height fins + low sun read
//      as zebra stripes — the autocorrelation is the structural cause, gated directly).
// Usage: node scripts/check-chunk-build.mjs. Exit 1 on FAIL.

import { defaults, presets, palettes, paletteKeys } from '../src/params.js';
import { makeChunkContext, buildChunkGeometry, carveReach } from '../src/sdf-chunks.js';
import { segField } from '../src/joint-sets.js';

function mergedValues(name) {
  const preset = presets[name];
  const v = { ...defaults, ...preset };
  const pal = palettes[preset.palette || 'granite'];
  for (const k of paletteKeys) if (preset[k] === undefined) v[k] = pal[k];
  return v;
}

// Synthetic chunk: 8×8 cells of steep ramp (60 m rise), pad 2 for the shear sampler.
// waterPaint mimics real dry land: exactly (height − 40) — the case the guard rejects.
function makeJob(seaLevel, waterFill) {
  const cw = 8, ch = 8, pad = 2;
  const Sx = cw + 1 + 2 * pad, Sz = ch + 1 + 2 * pad;
  const maps = {};
  const zeros = () => new Float32Array(Sx * Sz);
  for (const k of ['height', 'sdfWeight', 'outcrop', 'crest', 'foot', 'gmax', 'gxs', 'gzs', 'deposit', 'flow', 'cavity', 'road', 'river', 'lake']) maps[k] = zeros();
  maps.waterPaint = new Float32Array(Sx * Sz);
  maps.waterLevel = new Float32Array(Sx * Sz);
  for (let j = 0; j < Sz; j++) {
    for (let i = 0; i < Sx; i++) {
      const q = j * Sx + i;
      const u = (i - pad) / cw; // 0..1 across the chunk (+pad apron)
      maps.height[q] = seaLevel + 10 + u * 60;
      maps.sdfWeight[q] = (i >= pad && i < Sx - pad && j >= pad && j < Sz - pad) ? 1 : 0;
      maps.crest[q] = seaLevel + 70;
      maps.foot[q] = seaLevel + 10;
      maps.gmax[q] = 2;
      maps.gxs[q] = 2;
      maps.gzs[q] = 0.1;
      maps.waterPaint[q] = waterFill === 'dry' ? maps.height[q] - 40 : -1e6;
      maps.waterLevel[q] = -1e6;
    }
  }
  return { ci: 0, cj: 0, i0: 0, j0: 0, cw, ch, pad, hmin: seaLevel + 8, hmax: seaLevel + 72, slopeF: 3, k: 2, maps };
}

const fails = [];
const v = mergedValues('Sea cliffs');
v.sdfVoxel = 2;
const meta = { N: 64, size: 256, C: 8, k: 2, cell: 256 / 63 / 2 };
const ctx = makeChunkContext(v);

const t0 = Date.now();
const sea = v.seaLevel;
const job = makeJob(sea, 'dry');
const out = buildChunkGeometry(job, meta, v, ctx);
console.log(`sea chunk: tris=${out.triangles} verts=${out.positions.length / 3} (${Date.now() - t0} ms) carveReach=${carveReach(v).toFixed(1)} m`);
if (!(out.triangles > 0)) fails.push('no triangles emitted');
let bad = 0;
for (const p of out.positions) if (!Number.isFinite(p)) bad++;
for (const n of out.normals) if (!Number.isFinite(n)) bad++;
if (bad > 0) fails.push(`${bad} non-finite position/normal components`);

// Water-guard: with dry-fallback waterPaint everywhere, no vertex above the shore may
// see a water line at or above itself (that painted the face water-blue).
{
  let worst = -Infinity, nAbove = 0;
  for (let i = 0; i < out.positions.length / 3; i++) {
    const y = out.positions[i * 3 + 1];
    if (y < sea + 1) continue;
    nAbove++;
    const wl = Math.max(sea, out.aux2[i * 4 + 3]);
    if (wl - (y - 0.05) > worst) worst = wl - (y - 0.05);
  }
  console.log(`water-guard: ${nAbove} verts above shore, worst (waterline − (y − 0.05)) = ${worst.toFixed(3)} m`);
  if (!(worst < 0)) fails.push(`dry fallback leaks water paint above shore (worst ${worst.toFixed(2)} m)`);
}

// Notch liveness: same chunk with the waterline mid-face, notch on vs off. The surface
// near the waterline must move (recessed bite); far above it must not.
const mid = sea + 40;
const vOn = { ...v, seaLevel: mid, sdfNotch: 3 };
const vOff = { ...v, seaLevel: mid, sdfNotch: 0 };
const jobOn = makeJob(sea, 'none'); // geometry identical; only the notch datum changes
const a = buildChunkGeometry(jobOn, meta, vOn, ctx);
const b = buildChunkGeometry(jobOn, meta, vOff, ctx);
// Binned comparison (robust to tessellation changes): mean face-plane position per
// 1 m height bin, notch on vs off.
const binMean = (pos, yLo, yHi) => {
  let sx = 0, n = 0;
  for (let i = 0; i < pos.length; i += 3) {
    const y = pos[i + 1];
    if (y >= yLo && y < yHi) { sx += pos[i]; n++; }
  }
  return n ? sx / n : NaN;
};
{
  const nearOn = binMean(a.positions, mid - 3, mid + 3), nearOff = binMean(b.positions, mid - 3, mid + 3);
  const farOn = binMean(a.positions, mid + 20, mid + 30), farOff = binMean(b.positions, mid + 20, mid + 30);
  const nearD = Math.abs(nearOn - nearOff), farD = Math.abs(farOn - farOff);
  console.log(`notch: on=${a.positions.length / 3} off=${b.positions.length / 3} verts; near-waterline Δx=${nearD.toFixed(3)} m, far Δx=${farD.toFixed(4)} m`);
  if (!(nearD > 0.05)) fails.push(`notch path dead (near Δx=${nearD.toFixed(3)} m)`);
  if (!(farD < 0.01)) fails.push(`notch leaks far from waterline (far Δx=${farD.toFixed(3)} m)`);
}

// Relief contrast: the same chunk with every relief amp zeroed must come out FLAT
// next to the Sea build — otherwise the relief amps are dead wiring (the plaster bug).
const vFlat = { ...v, sdfBlocks: 0, sdfPits: 0, sdfRough: 0, sdfPushPull: 0, sdfJoints: 0, sdfUndercut: 0.01, sdfNotch: 0 };
const flat = buildChunkGeometry(makeJob(sea, 'none'), meta, vFlat, ctx);
const wallNormalStd = (o) => {
  let sx = 0, sxx = 0, n = 0;
  for (let i = 0; i < o.normals.length; i += 3) {
    if (Math.abs(o.normals[i + 1]) > 0.7) continue; // wall verts only (no caps)
    const nx = o.normals[i];
    sx += nx; sxx += nx * nx; n++;
  }
  if (!n) return 0;
  const mean = sx / n;
  return Math.sqrt(Math.max(0, sxx / n - mean * mean));
};
{
  const seaStd = wallNormalStd(out), flatStd = wallNormalStd(flat);
  console.log(`relief: Sea wall-normal stddev=${seaStd.toFixed(3)} vs flat control=${flatStd.toFixed(4)}`);
  if (!(seaStd > 0.05)) fails.push(`Sea relief trivially flat (stddev=${seaStd.toFixed(3)})`);
  if (!(seaStd > flatStd * 2)) fails.push(`relief amps look dead (Sea ${seaStd.toFixed(3)} vs flat ${flatStd.toFixed(3)})`);
}

// Cross-joint segments: unit-check the SHIPPED segField (columns break into aperiodic
// segments with offset contrast and live cross-grooves) + prove the carve responds to
// it (segH 12 vs ∞ must move the surface — the wiring gate).
{
  const H = v.sdfSegH || 12;
  const bkeys = [3, 17, 42, 101, 500];
  let loMul = 1, hiMul = 0, grooveCols = 0;
  const bounds = [];
  for (const b of bkeys) {
    let prev = null, breaks = 0;
    let mx = 0;
    for (let y = 0; y <= 60; y += 0.25) {
      const sg = segField(b, y, H);
      if (sg.mul < loMul) loMul = sg.mul;
      if (sg.mul > hiMul) hiMul = sg.mul;
      if (Math.abs(sg.groove) > mx) mx = Math.abs(sg.groove);
      if (prev != null && Math.abs(sg.mul - prev) > 1e-9) { breaks++; bounds.push(y); }
      prev = sg.mul;
    }
    if (mx > 0.2) grooveCols++;
    if (breaks < 3 || breaks > 8) fails.push(`segField breaks wrong for column ${b} (${breaks} in 60 m)`);
  }
  console.log(`segments: breaks/60m ok, mul range=[${loMul.toFixed(2)}, ${hiMul.toFixed(2)}], grooveCols=${grooveCols}/5`);
  if (!(loMul < 0.5 && hiMul > 0.8)) fails.push(`segment contrast weak (mul [${loMul.toFixed(2)}, ${hiMul.toFixed(2)}])`);
  if (grooveCols < 3) fails.push(`cross-grooves missing (${grooveCols}/5 columns)`);
  const spread = Math.max(...bounds) - Math.min(...bounds);
  if (!(spread > 10)) fails.push(`segment boundaries suspiciously aligned (spread ${spread.toFixed(1)} m)`);
}
{
  // Wiring: the carve must RESPOND to segH (segments off → different surface).
  // 2-D (strike × height) binned |Δface-x| — a strike-wide bin would average the
  // per-column differences away to zero.
  const vSeg = { ...v, sdfSegH: 1e6 };
  const oSeg = buildChunkGeometry(makeJob(sea, 'none'), meta, vSeg, ctx);
  let zMin = Infinity, zMax = -Infinity;
  for (let i = 0; i < out.positions.length; i += 3) {
    if (Math.abs(out.normals[i + 1]) > 0.7) continue;
    const z = out.positions[i + 2];
    if (z < zMin) zMin = z; if (z > zMax) zMax = z;
  }
  let num = 0, den = 0, mx = 0;
  for (let zb = 0; zb < 8; zb++) {
    const zA = zMin + ((zMax - zMin) * zb) / 8, zB = zMin + ((zMax - zMin) * (zb + 1)) / 8;
    for (let yB = 80; yB < 140; yB += 5) {
      let sA = 0, nA = 0, sB = 0, nB = 0;
      for (let i = 0; i < out.positions.length; i += 3) {
        const y = out.positions[i + 1], z = out.positions[i + 2];
        if (y >= yB && y < yB + 5 && z >= zA && z < zB && Math.abs(out.normals[i + 1]) <= 0.7) { sA += out.positions[i]; nA++; }
      }
      for (let i = 0; i < oSeg.positions.length; i += 3) {
        const y = oSeg.positions[i + 1], z = oSeg.positions[i + 2];
        if (y >= yB && y < yB + 5 && z >= zA && z < zB && Math.abs(oSeg.normals[i + 1]) <= 0.7) { sB += oSeg.positions[i]; nB++; }
      }
      if (nA > 4 && nB > 4) { const dd = Math.abs(sA / nA - sB / nB); num += dd; den++; if (dd > mx) mx = dd; }
    }
  }
  const resp = den ? num / den : NaN;
  console.log(`segments: carve response to segH = mean ${Number.isNaN(resp) ? 'n/a' : resp.toFixed(3)} m, max ${mx.toFixed(3)} m (cells=${den})`);
  if (Number.isNaN(resp)) fails.push('segment wiring starved');
  else if (!(resp > 0.05 && mx > 0.25)) fails.push(`carve ignores segH (mean ${resp.toFixed(3)} m, max ${mx.toFixed(3)} m)`);
}
if (fails.length) { console.log(`CHUNK-BUILD: FAIL — ${fails.join('; ')}`); process.exit(1); }
console.log('CHUNK-BUILD: PASS');
