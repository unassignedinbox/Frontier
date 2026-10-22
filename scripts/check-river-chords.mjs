// Regression check: river receiver links must always be adjacent (8-neighbourhood).
// A long-range receiver link (sink -> pour point across a closed depression) is drawn
// by the centreline rasteriser as a ruler-straight ribbon hundreds of metres long that
// ignores the terrain, and a flat pit floor fans a whole bundle of them to one exit.
// Synthetic case: smooth tilted plane + one big closed bowl.
import { simulateRivers } from '../src/hydrology.js';

const N = 256, size = 1024, cell = size / (N - 1);
const H = new Float32Array(N * N);
const cx = N * 0.5, cy = N * 0.52, R = 62;
for (let j = 0; j < N; j++) for (let i = 0; i < N; i++) {
  const dx = i - cx, dy = j - cy;
  const r2 = (dx * dx + dy * dy) / (R * R);
  H[j * N + i] = 120 - i * cell * 0.02 - 26 * Math.exp(-r2 * 2.2);
}
const out = simulateRivers(H, N, size, {
  catchment: 0.04, widthScale: 25, maxWidth: 60, depthScale: 1, bankAngle: 35,
  maxBank: 20, waterDepth: 0, braiding: 0, lakes: 0, seaLevel: -100,
}, 7);

let failures = 0;
const fail = (msg) => { failures++; console.error('FAIL:', msg); };

// 1. every receiver link is adjacent (no chords possible)
let jumps = 0, maxJump = 0;
for (let c = 0; c < N * N; c++) {
  const d = out.down[c];
  if (d < 0) continue;
  const cheb = Math.max(Math.abs((d % N) - (c % N)), Math.abs(((d - (d % N)) / N) - ((c - (c % N)) / N)));
  if (cheb > 1) { jumps++; if (cheb > maxJump) maxJump = cheb; }
}
if (jumps > 0) fail(`${jumps} non-adjacent receiver links, longest ${maxJump} cells`);
else console.log('PASS: all receiver links adjacent (0 jump links)');

// 2. the drainage graph is acyclic (sampled chain walks terminate)
let cyclic = 0;
for (let c = 0; c < N * N; c += 7) {
  let p = c, n = 0;
  const seen = new Set();
  while (p >= 0 && n++ < 4000 && !seen.has(p)) { seen.add(p); p = out.down[p]; }
  if (p >= 0 && seen.has(p)) cyclic++;
}
if (cyclic > 0) fail(`${cyclic} sampled chains cycle`);
else console.log('PASS: drainage graph acyclic on sampled chains');

// 3. the bowl still drains: a trunk leaves the hollow to the east, and the network
// inside the bowl is dendritic (no single straight chord across it)
let east = 0, inBowl = 0;
for (let j = 0; j < N; j++) for (let i = 0; i < N; i++) {
  if (out.riverMask[j * N + i] < 0.5) continue;
  if (i > cx + R) east++;
  if (Math.hypot(i - cx, j - cy) < R) inBowl++;
}
if (east < 50) fail(`bowl does not drain east (${east} trunk cells)`);
else console.log(`PASS: bowl drains east (${east} trunk cells, ${inBowl} in-bowl channel cells)`);

if (!Number.isFinite(out.stats.maxKm2)) fail('non-finite stats');
if (failures > 0) { console.error(`${failures} FAILURE(S)`); process.exit(1); }
console.log('check-river-chords: OK');
