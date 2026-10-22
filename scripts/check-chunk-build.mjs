// Chunk-build smoke test: runs the REAL buildChunkGeometry (carve + shear + marching
// cubes + floater filter) on a synthetic steep ramp with Sea-cliffs values, so carve
// edits (lip-gated seams, wave-cut notch, roughness rebalance) execute headlessly.
// Asserts finite output + triangles out, and that the notch path is LIVE (toggling
// sdfNotch moves the surface near the waterline).
// Usage: node scripts/check-chunk-build.mjs. Exit 1 on FAIL.

import { defaults, presets, palettes, paletteKeys } from '../src/params.js';
import { makeChunkContext, buildChunkGeometry, carveReach } from '../src/sdf-chunks.js';
import { NO_WATER } from '../src/features.js';

function mergedValues(name) {
  const preset = presets[name];
  const v = { ...defaults, ...preset };
  const pal = palettes[preset.palette || 'granite'];
  for (const k of paletteKeys) if (preset[k] === undefined) v[k] = pal[k];
  return v;
}

// Synthetic chunk: 8×8 cells of steep ramp (60 m rise), pad 2 for the shear sampler.
function makeJob(seaLevel) {
  const cw = 8, ch = 8, pad = 2;
  const Sx = cw + 1 + 2 * pad, Sz = ch + 1 + 2 * pad;
  const maps = {};
  const zeros = () => new Float32Array(Sx * Sz);
  for (const k of ['height', 'sdfWeight', 'outcrop', 'crest', 'foot', 'gmax', 'gxs', 'gzs', 'deposit', 'flow', 'cavity', 'road', 'river', 'lake']) maps[k] = zeros();
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
    }
  }
  maps.waterLevel = new Float32Array(Sx * Sz).fill(NO_WATER);
  maps.waterPaint = new Float32Array(Sx * Sz).fill(NO_WATER);
  return { ci: 0, cj: 0, i0: 0, j0: 0, cw, ch, pad, hmin: seaLevel + 8, hmax: seaLevel + 72, slopeF: 3, k: 2, maps };
}

const fails = [];
const v = mergedValues('Sea cliffs');
v.sdfVoxel = 2;
const meta = { N: 64, size: 256, C: 8, k: 2, cell: 256 / 63 / 2 };
const ctx = makeChunkContext(v);

const t0 = Date.now();
const sea = v.seaLevel;
const job = makeJob(sea);
const out = buildChunkGeometry(job, meta, v, ctx);
console.log(`sea chunk: tris=${out.triangles} verts=${out.positions.length / 3} (${Date.now() - t0} ms) carveReach=${carveReach(v).toFixed(1)} m`);
if (!(out.triangles > 0)) fails.push('no triangles emitted');
let bad = 0;
for (const p of out.positions) if (!Number.isFinite(p)) bad++;
for (const n of out.normals) if (!Number.isFinite(n)) bad++;
if (bad > 0) fails.push(`${bad} non-finite position/normal components`);

// Notch liveness: same chunk with the waterline mid-face, notch on vs off. The surface
// near the waterline must move (recessed bite); far above it must not.
const mid = sea + 40;
const vOn = { ...v, seaLevel: mid, sdfNotch: 3 };
const vOff = { ...v, seaLevel: mid, sdfNotch: 0 };
const jobOn = makeJob(sea); // geometry identical; only the notch datum changes
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
const flat = buildChunkGeometry(makeJob(sea), meta, vFlat, ctx);
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
  if (!(seaStd > flatStd * 3)) fails.push(`relief amps look dead (Sea ${seaStd.toFixed(3)} vs flat ${flatStd.toFixed(3)})`);
}
if (fails.length) { console.log(`CHUNK-BUILD: FAIL — ${fails.join('; ')}`); process.exit(1); }
console.log('CHUNK-BUILD: PASS');
