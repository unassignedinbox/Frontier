// True-3D cliffs: local SDF chunks over the steep parts of the refined heightfield, seamless with the
// surrounding heightfield mesh by construction (see Frontier/Experimental/SdfCliffLab for the
// isolated proof). Pure JS — chunk meshing runs in sdf.worker.js.
//
//   field  f(p) = mix( y − h(x,z),  (y − h) / |∇|  + carve(p),  W(x,z) )
//   W      cliff weight: 1 on cells steeper than `sdfAngle`, fading to 0 over `sdfBlend` cells, 0 on
//          every face shared with a non-chunked cell and on the map border
//   carve  ≥ 0 removes rock along the normal: soft strata beds (same strata model as the heightfield,
//          evaluated at the 3-D point's elevation) are undercut by pocket noise, plus joints,
//          weathering pits and fine roughness — so hard beds are left as lips and overhangs
//   mesh   marching tetrahedra on a voxel grid aligned with the heightfield cells; where W == 0 the
//          field is linear along every edge, so iso-crossings sit exactly on the heightfield's own
//          piecewise-linear border → watertight with the terrain mesh, which skips the covered quads

import { SimplexNoise, GradientNoise3, smoothstep, lerp, clamp01 } from './noise.js';
import { jointField, makeJointSets } from './joint-sets.js';
import { makeBedTable, bedAt, bedJitter, bedHardness } from './strata-model.js';
import { NO_WATER } from './features.js';
import { edgeTable, triTable } from './mc-tables.js';
import { makeDetail } from './detail.js';

const DX8 = [1, -1, 0, 0, 1, 1, -1, -1], DZ8 = [0, 0, 1, -1, 1, -1, 1, -1];

// Chunks are C×C heightfield cells; the last row / column may be narrower (grid cells are rarely a
// multiple of the chunk size). Chunk (ci, cj) covers cells [ci·C, min(ci·C + C, N−1)).
export function chunkCellsFor(N, want) {
  return Math.max(4, Math.min(64, Math.round(want || 16)));
}

// ---- selection on the refined field --------------------------------------------------------------
export function selectChunks(fine, v) {
  const { resolution: N, worldSize: size, height } = fine;
  const cell = size / (N - 1);
  const tanA = Math.tan(((v.sdfAngle || 58) * Math.PI) / 180);
  const r = Math.max(1, Math.min(6, Math.round(v.sdfBlend || 2)));
  const total = N * N;
  // chamfer distance (in cells) from cliff nodes, up to r
  const dist = new Uint8Array(total).fill(255);
  for (let j = 1; j < N - 1; j++) {
    for (let i = 1; i < N - 1; i++) {
      const idx = j * N + i;
      const gx = (height[idx + 1] - height[idx - 1]) / (2 * cell), gz = (height[idx + N] - height[idx - N]) / (2 * cell);
      if (gx * gx + gz * gz > tanA * tanA) dist[idx] = 0;
    }
  }
  for (let pass = 1; pass <= r; pass++) {
    for (let j = 1; j < N - 1; j++) {
      for (let i = 1; i < N - 1; i++) {
        const idx = j * N + i;
        if (dist[idx] !== 255) continue;
        for (let d = 0; d < 8; d++) {
          if (dist[idx + DX8[d] + DZ8[d] * N] === pass - 1) { dist[idx] = pass; break; }
        }
      }
    }
  }
  const W = new Float32Array(total);
  for (let idx = 0; idx < total; idx++) {
    const d = dist[idx];
    if (d === 255) continue;
    const i = idx % N, j = (idx - i) / N;
    if (i === 0 || j === 0 || i === N - 1 || j === N - 1) continue;
    W[idx] = smoothstep(0, 1, 1 - d / (r + 1));
  }

  const C = chunkCellsFor(N, v.sdfChunk || 16);
  const nc = Math.ceil((N - 1) / C);
  const candidates = [];
  for (let cj = 0; cj < nc; cj++) {
    for (let ci = 0; ci < nc; ci++) {
      const i0 = ci * C, j0 = cj * C, i1 = Math.min(N - 1, i0 + C), j1 = Math.min(N - 1, j0 + C);
      let sum = 0, hmin = Infinity, hmax = -Infinity, g2max = 0;
      for (let j = j0; j <= j1; j++) {
        for (let i = i0; i <= i1; i++) {
          const idx = j * N + i;
          sum += W[idx];
          const h = height[idx]; if (h < hmin) hmin = h; if (h > hmax) hmax = h;
          if (i > 0 && j > 0 && i < N - 1 && j < N - 1) {
            const gx = (height[idx + 1] - height[idx - 1]) / (2 * cell), gz = (height[idx + N] - height[idx - N]) / (2 * cell);
            const g2 = gx * gx + gz * gz; if (g2 > g2max) g2max = g2;
          }
        }
      }
      if (sum > 0) candidates.push({ ci, cj, i0, j0, cw: i1 - i0, ch: j1 - j0, hmin, hmax, sum, slopeF: Math.sqrt(1 + g2max) });
    }
  }
  candidates.sort((a, b) => b.sum - a.sum);
  const maxChunks = Math.max(1, Math.round(v.sdfMaxChunks || 600));
  const list = candidates.slice(0, maxChunks);
  const active = new Uint8Array(nc * nc);
  for (const c of list) active[c.cj * nc + c.ci] = 1;
  // W must be 0 on faces shared with inactive chunks: zero every node that touches a non-active chunk
  if (list.length < candidates.length) {
    for (let j = 0; j < N; j++) {
      for (let i = 0; i < N; i++) {
        const idx = j * N + i;
        if (W[idx] === 0) continue;
        const ci0 = Math.max(0, Math.floor((i - 1) / C)), ci1 = Math.min(nc - 1, Math.floor(i / C));
        const cj0 = Math.max(0, Math.floor((j - 1) / C)), cj1 = Math.min(nc - 1, Math.floor(j / C));
        let inactive = false;
        for (let cj = cj0; cj <= cj1 && !inactive; cj++) for (let ci = ci0; ci <= ci1; ci++) if (!active[cj * nc + ci]) { inactive = true; break; }
        if (inactive) W[idx] = 0;
      }
    }
  }
  // fade for the heightfield mesh displacement: 0 on/inside active footprints, 1 three cells away
  const fade = new Float32Array(total).fill(1);
  for (const c of list) {
    for (let j = c.j0; j <= c.j0 + c.ch; j++) for (let i = c.i0; i <= c.i0 + c.cw; i++) fade[j * N + i] = 0;
  }
  for (let pass = 1; pass <= 3; pass++) {
    const level = pass / 4;
    for (let j = 1; j < N - 1; j++) {
      for (let i = 1; i < N - 1; i++) {
        const idx = j * N + i;
        if (fade[idx] < 1) continue;
        for (let d = 0; d < 8; d++) {
          const f = fade[idx + DX8[d] + DZ8[d] * N];
          if (f < level - 1e-6) { fade[idx] = level; break; }
        }
      }
    }
  }
  fine.sdfWeight = W;
  fine.sdfFade = fade;
  return { active, nc, C, list, W, fade, candidates: candidates.length };
}

// ---- jobs for the worker (slices of every map, transferable) ---------------------------------------
export function packChunkJobs(fine, chunks, v) {
  const { resolution: N } = fine;
  const { C, list } = chunks;
  const names = ['height', 'hardness', 'deposit', 'flow', 'cavity', 'road', 'river', 'lake', 'waterLevel', 'waterPaint', 'sdfWeight', 'outcrop', 'crest', 'foot', 'gmax', 'gxs', 'gzs'];
  const jobs = [], transfer = [];
  const cell = fine.worldSize / (N - 1);
  const maxCarve = carveReach(v);
  const leanR = Math.max(2, Math.round(Math.min(80, (v.sdfLeanReach || 24)) / cell));
  const vertical = Math.max(0, v.sdfVertical || 0);
  if (((v.sdfLean || 0) > 0 || vertical > 0) && (!fine.crest || fine._crestR !== leanR)) {
    // local crest / foot of the face (separable max / min filters + a blur) for the lean
    const r = leanR;
    fine._crestR = r;
    const run = (src, pick) => {
      const a = new Float32Array(N * N), b = new Float32Array(N * N);
      for (let j = 0; j < N; j++) for (let i = 0; i < N; i++) { let m = src[j * N + i]; for (let d = -r; d <= r; d++) { const ii = i + d; if (ii >= 0 && ii < N) m = pick(m, src[j * N + ii]); } a[j * N + i] = m; }
      for (let j = 0; j < N; j++) for (let i = 0; i < N; i++) { let m = a[j * N + i]; for (let d = -r; d <= r; d++) { const jj = j + d; if (jj >= 0 && jj < N) m = pick(m, a[jj * N + i]); } b[j * N + i] = m; }
      // box blur so the lean does not step where the filter window changes
      for (let j = 0; j < N; j++) for (let i = 0; i < N; i++) { let sum = 0, n = 0; for (let dj = -2; dj <= 2; dj++) { const jj = j + dj; if (jj < 0 || jj >= N) continue; for (let di = -2; di <= 2; di++) { const ii = i + di; if (ii < 0 || ii >= N) continue; sum += b[jj * N + ii]; n++; } } a[j * N + i] = sum / n; }
      return a;
    };
    fine.crest = run(fine.height, Math.max);
    fine.foot = run(fine.height, Math.min);
    // steepest gradient in the window (→ plan width of the face) and the smoothed uphill direction
    const H = fine.height, gm = new Float32Array(N * N), gx = new Float32Array(N * N), gz = new Float32Array(N * N);
    for (let j = 1; j < N - 1; j++) for (let i = 1; i < N - 1; i++) {
      const idx = j * N + i;
      const a = (H[idx + 1] - H[idx - 1]) / (2 * cell), b = (H[idx + N] - H[idx - N]) / (2 * cell);
      gx[idx] = a; gz[idx] = b; gm[idx] = Math.hypot(a, b);
    }
    fine.gmax = run(gm, Math.max);
    // the direction is weighted by the gradient magnitude so the face dictates it, not the plateau
    const dirBox = (src) => {
      const a = new Float32Array(N * N), b = new Float32Array(N * N);
      for (let j = 0; j < N; j++) for (let i = 0; i < N; i++) { let sum = 0; for (let d = -r; d <= r; d++) { const ii = i + d; if (ii >= 0 && ii < N) sum += src[j * N + ii]; } a[j * N + i] = sum; }
      for (let j = 0; j < N; j++) for (let i = 0; i < N; i++) { let sum = 0; for (let d = -r; d <= r; d++) { const jj = j + d; if (jj >= 0 && jj < N) sum += a[jj * N + i]; } b[j * N + i] = sum; }
      return b;
    };
    const wx = new Float32Array(N * N), wz = new Float32Array(N * N), g2 = new Float32Array(N * N);
    for (let i = 0; i < N * N; i++) { wx[i] = gx[i] * gm[i]; wz[i] = gz[i] * gm[i]; g2[i] = gm[i] * gm[i]; }
    fine.gxs = dirBox(wx); fine.gzs = dirBox(wz);
    // steepness-weighted mean gradient of the window: the face's own slope (the plateau and the
    // foot hardly count, the steepest cell does not dominate) → plan width of the face
    const sg = dirBox(gm), sg2 = dirBox(g2);
    // the plan width of the face lies between (crest − foot) / max slope and / mean slope: use the
    // geometric mean of the two (calibrated on escarpments: vertical = 1 → a vertical wall)
    const gmx = fine.gmax;
    for (let i = 0; i < N * N; i++) gmx[i] = sg[i] > 1e-6 ? Math.sqrt(gmx[i] * (sg2[i] / sg[i])) : gmx[i];
  }
  // slices are padded so the vertical-wall shear can sample the heightfield beyond the chunk
  const pad = vertical > 0 ? Math.min(24, Math.ceil((Math.min(80, v.sdfLeanReach || 24) * Math.max(1, vertical) * 0.6) / cell) + 1) : 0;
  for (const c of list) {
    const Sx = c.cw + 1 + 2 * pad, Sz = c.ch + 1 + 2 * pad;
    const maps = {};
    for (const name of names) {
      const src = fine[name];
      const out = new Float32Array(Sx * Sz);
      if (src) {
        for (let j = 0; j < Sz; j++) {
          const jj = Math.min(N - 1, Math.max(0, c.j0 + j - pad));
          for (let i = 0; i < Sx; i++) { const ii = Math.min(N - 1, Math.max(0, c.i0 + i - pad)); out[j * Sx + i] = src[jj * N + ii]; }
        }
      } else if (name === 'waterLevel' || name === 'waterPaint') out.fill(NO_WATER);
      maps[name] = out; transfer.push(out.buffer);
    }
    jobs.push({ ci: c.ci, cj: c.cj, i0: c.i0, j0: c.j0, cw: c.cw, ch: c.ch, pad, hmin: c.hmin, hmax: c.hmax, slopeF: c.slopeF, maps });
  }
  // voxel resolution: explicit, or adaptive per chunk within the voxel budget. Only the band
  // around the surface is polygonised, so the estimate is footprint × band thickness; every
  // chunk is guaranteed k=1 and the heroes upgrade to k=2..4 while the budget lasts, biggest
  // faces first (the list is W-sum ordered) — so the great walls get crisp blocks instead of
  // everything melting at k=1. Upgrades past k=2 go to the top 12 only (build time is cubic).
  let kMax = 1;
  if (v.sdfVoxel >= 1) {
    kMax = Math.max(1, Math.min(4, Math.round(v.sdfVoxel)));
    for (const job of jobs) job.k = kMax;
  } else {
    const budget = Math.max(1e6, (v.sdfVoxelBudget || 24) * 1e6);
    let remaining = budget;
    const est1 = [];
    for (const c of list) {
      let band = (maxCarve * Math.min(3, c.slopeF) * 2) / cell + 6;
      // a sheared (vertical) wall is polygonised over the whole foot…crest band of each column
      if (vertical > 0) band = Math.max(band, 0.6 * ((c.hmax - c.hmin) / cell) + 6);
      const e = c.cw * c.ch * band;
      est1.push(e); remaining -= e;
    }
    const kk = jobs.map(() => 1);
    for (let pass = 2; pass <= 4; pass++) {
      for (let j = 0; j < jobs.length; j++) {
        if (kk[j] !== pass - 1) continue;
        if (pass >= 3 && j >= 12) continue;
        const cost = est1[j] * (pass * pass * pass - (pass - 1) * (pass - 1) * (pass - 1));
        if (cost <= remaining) { kk[j] = pass; remaining -= cost; }
      }
    }
    jobs.forEach((job, j) => { job.k = kk[j]; if (kk[j] > kMax) kMax = kk[j]; });
  }
  return { jobs, transfer, meta: { N, size: fine.worldSize, C, k: kMax, cell: cell / kMax } };
}

// largest distance the carve can move the surface along the normal
export function carveReach(v) {
  const undercut = Math.max(0, v.sdfUndercut == null ? 6 : v.sdfUndercut);
  const pits = v.sdfPits == null ? 0.3 : v.sdfPits, joints = v.sdfJoints == null ? 0.5 : v.sdfJoints;
  const detail = (v.detailRelief || 0) + (v.rockyAmount || 0) + (v.cragAmount || 0);
  const blocks = Math.max(0, v.sdfBlocks || 0);
  return undercut * (1 + 0.5 * pits + 0.9 * joints) + 1.5 + detail + Math.max(0, v.sdfPushPull || 0) + Math.max(0, v.sdfLean || 0)
    + blocks * 1.5 + (blocks > 0 ? Math.min(v.sdfBlockSize || 8, 12) * 0.6 : 0);
}


// ---- strata hardness at a 3-D point (same model as heightfield.applyStrata) -----------------------
// column(x, z) caches everything that depends on the plan position; at(col, y) is cheap per voxel.
export function makeStrataSampler(params) {
  const size = params.worldSize;
  const dipRad = (params.strataDip * Math.PI) / 180, dirRad = (params.strataDipDirection * Math.PI) / 180;
  const gx = Math.tan(dipRad) * Math.cos(dirRad), gz = Math.tan(dipRad) * Math.sin(dirRad);
  const detail = new SimplexNoise(params.seed * 5 + 77);
  const contrast = params.hardnessContrast;
  const table = makeBedTable(params);
  const lateral = params.strataLateral == null ? 0.18 : params.strataLateral;
  function column(xw, zw) {
    const x = xw + size / 2, z = zw + size / 2;
    return { x, z, tilt: gx * x + gz * z, jitter: bedJitter(x, z, lateral), layers: new Map(), bed: {} };
  }
  // returns [hardness, fraction within bed (0 base → 1 top), bed thickness, bed index]
  function at(col, y) {
    const bed = bedAt(table, (y + col.tilt) / col.jitter, col.bed);
    const bi = bed.index;
    let lens = col.layers.get(bi);
    if (lens === undefined) {
      lens = 0.12 * detail.fbm(col.x * 0.006 + bi * 3.7, col.z * 0.006, 3);
      col.layers.set(bi, lens);
    }
    const hard = lerp(0.35, clamp01(bedHardness(bed) + lens), contrast);
    return [hard, bed.f, bed.thick, bi];
  }
  return { column, at, table, sample: (xw, zw, y) => at(column(xw, zw), y) };
}

// ---- marching cubes ------------------------------------------------------------------------------
const CORNER = [[0, 0, 0], [1, 0, 0], [1, 1, 0], [0, 1, 0], [0, 0, 1], [1, 0, 1], [1, 1, 1], [0, 1, 1]];
const EDGES = [[0, 1], [1, 2], [2, 3], [3, 0], [4, 5], [5, 6], [6, 7], [7, 4], [0, 4], [1, 5], [2, 6], [3, 7]];

export function makeChunkContext(v) {
  return {
    seed: (v.seed | 0) >>> 0,
    strata: makeStrataSampler(v),
    pocket: new GradientNoise3((v.seed || 1) * 41 + 9),
    joint: new SimplexNoise((v.seed || 1) * 23 + 5),
    rough: new GradientNoise3((v.seed || 1) * 31 + 11),
    push: new GradientNoise3((v.seed || 1) * 37 + 29),
    jsets: makeJointSets(v.seed || 1),
  };
}

export function buildChunkGeometry(job, meta, v, ctx) {
  const { N, size } = meta;
  const pad = job.pad || 0;
  const Sx = job.cw + 1 + 2 * pad, Sz = job.ch + 1 + 2 * pad;
  const cell = size / (N - 1);
  const k = v.sdfVoxel >= 1 ? Math.max(1, Math.min(4, Math.round(v.sdfVoxel))) : (job.k || meta.k || 1);
  const vox = cell / k;
  const nx = job.cw * k, nz = job.ch * k;
  const x0 = (job.i0 / (N - 1) - 0.5) * size, z0 = (job.j0 / (N - 1) - 0.5) * size;
  const m = job.maps;

  // local bilinear sampler on the (padded) slice (u, w in cells from the chunk origin)
  const bil = (arr, u, w) => {
    u += pad; w += pad;
    const i = Math.min(Sx - 2, Math.max(0, Math.floor(u))), j = Math.min(Sz - 2, Math.max(0, Math.floor(w)));
    const fu = Math.min(1, Math.max(0, u - i)), fw = Math.min(1, Math.max(0, w - j));
    const a = arr[j * Sx + i], b = arr[j * Sx + i + 1], c = arr[(j + 1) * Sx + i], d = arr[(j + 1) * Sx + i + 1];
    return (a + (b - a) * fu) * (1 - fw) + (c + (d - c) * fu) * fw;
  };
  const grad = (u, w) => {
    u += pad; w += pad;
    const i = Math.min(Sx - 2, Math.max(0, Math.floor(u))), j = Math.min(Sz - 2, Math.max(0, Math.floor(w)));
    const fu = Math.min(1, Math.max(0, u - i)), fw = Math.min(1, Math.max(0, w - j));
    const h = m.height;
    const a = h[j * Sx + i], b = h[j * Sx + i + 1], c = h[(j + 1) * Sx + i], d = h[(j + 1) * Sx + i + 1];
    return [((b - a) * (1 - fw) + (d - c) * fw) / cell, ((c - a) * (1 - fu) + (d - b) * fu) / cell];
  };

  const undercut = Math.max(0, v.sdfUndercut == null ? 6 : v.sdfUndercut);
  const pocketScale = Math.max(1, v.sdfPockets || 12);
  const pits = v.sdfPits == null ? 0.3 : v.sdfPits;
  const joints = v.sdfJoints == null ? 0.5 : v.sdfJoints;
  // roughness wavelength is tied to the voxel size (≥ 5 voxels) so it cannot alias into stair-steps
  const roughAmp = (v.sdfRough == null ? 0.5 : v.sdfRough) * Math.min(1.2, vox * 0.3);
  const roughFreq = 1 / (vox * 5);
  const bedPower = v.sdfBedContrast == null ? 1 : v.sdfBedContrast; // bed-relief contrast: partings + per-bed steps
  const hasOutcrop = !!m.outcrop;

  // the same rocky facets / crags / relief as the heightfield mesh, evaluated in 3-D on the carved
  // faces (feature size clamped to ≥ 2.5 voxels so blocks cannot alias into voxel steps)
  const detail = makeDetail({ worldSize: size, resolution: N }, { ...v, detailRelief: 0 }, { minScale: vox * 2.5, limit: 1e9 }); // fbm relief is already there as `rough`
  // true 3-D push–pull: blocky buttresses stand out of the face (negative carve = outward) and
  // recesses go back into it, varying with height so a block can overhang the recess below it
  const pushAmp = Math.max(0, v.sdfPushPull || 0);
  const pushL = Math.max(vox * 4, v.sdfPushScale || 18);
  // lean: the face is cut back progressively from its crest to its foot, so the profile is | / —
  // vertical to overhanging — instead of the back-leaning slope a heightfield gives
  const leanAmp = Math.max(0, v.sdfLean || 0);
  const hasLean = leanAmp > 0 && !!m.crest && !!m.foot;
  // vertical wall: the face is sheared so its crest moves out and its foot moves in — at 1 the
  // contours of the face collapse onto one line (a vertical wall), above 1 the wall overhangs
  // (\ /). A point at height y takes the heightfield from a plan position shifted uphill by
  // (t − ½)·W·vertical, t = (y − foot)/(crest − foot), W = plan width of the face in the heightfield
  const vertical = Math.max(0, v.sdfVertical || 0);
  const hasVertical = vertical > 0 && !!m.crest && !!m.foot && !!m.gmax && pad > 0;
  const vertMaxW = Math.min(80, v.sdfLeanReach || 24) * 1.2;
  const needCF = hasLean || hasVertical;
  // rock mass: the face is bedding planes x two regional joint sets (world-anchored planes
  // shared with the surface shader, so paint and geometry describe the same blocks). Each
  // joint-bounded block stands proud or sits back, some have fallen out (scar sockets), and
  // thin partings recess at the bed boundaries. Bed relief is deliberately quiet - deep
  // soft-bed slots were half of the banded-wallpaper look.
  const blockAmp = Math.max(0, v.sdfBlocks || 0);
  const blockLoss = Math.min(1, Math.max(0, v.sdfBlockLoss == null ? 0.12 : v.sdfBlockLoss));
  const jointSpacing = Math.max(1.5, v.jointSpacing || 6);
  const jointWidth = Math.max(0.15, v.jointWidth || 0.35);
  const jointDropout = Math.min(0.9, Math.max(0, v.jointDropout == null ? 0.25 : v.jointDropout));
  const jointStagger = Math.min(2, Math.max(0, v.jointStagger == null ? 0.8 : v.jointStagger));
  const jointDomain = Math.max(14, jointSpacing * 5);
  const jointOpts = { width: jointWidth, dropout: jointDropout, stagger: jointStagger, domain: jointDomain, loss: blockLoss, vary: v.jointVary == null ? 0.7 : v.jointVary };
  const seamW = Math.max(0.35, vox * 0.6);
  const seamDepth = 0.15 * undercut, bedStep = 0.12 * undercut;
  const jointDepthM = 1.5, scarDepth = 1.2 + blockAmp * 0.35, pitDepthM = 2;
  const bump = roughAmp + detail.amp + pushAmp + blockAmp;
  const maxCarve = carveReach(v);
  // everything that depends on the plan position only
  function column(x, z) {
    const u = (x - x0) / cell, wq = (z - z0) / cell;
    const h = bil(m.height, u, wq);
    const w = bil(m.sdfWeight, u, wq);
    const [gx, gz] = grad(u, wq);
    // (y − h)/slopeF is only a valid distance while the heightfield is a graph over the
    // column; past ~72° it collapses toward 0 and the carve term takes over the sign, so
    // near-vertical steps turn into noise-defined spikes and flakes. Cap it: a true step
    // then renders as its (one-cell) ramp plus carve instead of flying off.
    const slopeF = Math.min(3, Math.sqrt(1 + gx * gx + gz * gz));
    const gl = Math.hypot(gx, gz);
    const steep = smoothstep(0.7, 1.7, gl);
    const oc = hasOutcrop ? bil(m.outcrop, u, wq) : 0;
    const crest = needCF ? bil(m.crest, u, wq) : 0, foot = needCF ? bil(m.foot, u, wq) : 0;
    let vW = 0, vdx = 0, vdz = 0;
    if (hasVertical && w > 1e-6) {
      const gm = bil(m.gmax, u, wq);
      const dxs = bil(m.gxs, u, wq), dzs = bil(m.gzs, u, wq);
      const dl = Math.hypot(dxs, dzs);
      if (gm > 0.5 && dl > 1e-6 && crest - foot > 2) {
        // the window-smoothed slope underestimates narrow fins and crest brinks, where the
        // full-window shift would sample across to unrelated ground and tear the wall (spikes,
        // ragged rims, eaten fins) — so the shift width never exceeds what the LOCAL slope
        // implies, and the shear fades out where the smoothed uphill direction disagrees with
        // the local gradient (notches, fins, hairpins: no single shift direction is valid there
        // and the steep heightfield + carve already describe the rock); on a planar ramp the
        // two agree and nothing changes
        vW = Math.min(vertMaxW, (crest - foot) / Math.max(gm, gl)) * vertical * w;
        if (gl > 1e-3) vW *= smoothstep(0.05, 0.3, gl) * smoothstep(0.2, 0.6, (gx * dxs + gz * dzs) / (gl * dl));
        else vW = 0;
        vdx = dxs / dl; vdz = dzs / dl; // unit uphill direction
        if (vW > 1e-6) {
          // walk the shift line: from foot to crest the shifted samples must climb the SAME
          // face monotonically — columns whose shift crosses a wall laterally, straddles a
          // notch, or slides off a fin sample unrelated ground and grow detached curtains
          // and folds next to the wall, so the shear fades out with the violation
          const faceH = crest - foot;
          let prev = -Infinity, dip = 0;
          for (let s = 0; s <= 4; s++) {
            const tt = s / 4, e = tt * tt * (3 - 2 * tt), sh = ((e - 0.5) * vW) / cell;
            const hs = bil(m.height, u + vdx * sh, wq + vdz * sh);
            if (s > 0 && hs < prev) dip = Math.max(dip, prev - hs);
            if (hs > prev) prev = hs;
          }
          vW *= 1 - smoothstep(faceH * 0.06, faceH * 0.22, dip);
        }
      }
    }
    return { x, z, h, w, slopeF, steep, oc, crest, foot, vW, vdx, vdz, jc: null,
      strata: w > 1e-6 && (steep > 0 || vW > 0) ? ctx.strata.column(x, z) : null };
  }
  function carve(col, y) {
    const { x, z } = col;
    const [hard, f, thick, bi] = ctx.strata.at(col.strata, y);
    // the joint field of this bed at this plan position (cached: constant per column + bed)
    let jc = col.jc;
    if (!jc || jc.bi !== bi) jc = col.jc = { bi, ...jointField(x, z, bi, ctx.jsets, jointSpacing, thick, jointOpts) };
    // bedding relief: thin recessed partings AT the bed boundaries, hard beds faintly proud
    const edge = Math.min(f, 1 - f) * thick;
    const seam = (1 - smoothstep(0, seamW, edge)) * seamDepth;
    const hardness = smoothstep(0.45, 0.7, hard);
    const bedOff = (0.45 - hardness) * bedStep;
    const bedRelief = (seam + bedOff) * bedPower * (1 - col.oc);
    // planar joints: grooves at the true joint planes, biting hardest in hard beds
    const jointCarve = jc.groove * joints * jointDepthM * (0.35 + 0.65 * hardness) * (1 - col.oc);
    // blocks: each joint-bounded block stands proud or sits back; some have fallen out
    let block = 0;
    if (blockAmp > 0) {
      const bh = (0.35 + 0.65 * smoothstep(0.35, 0.6, hard)) * (1 - col.oc);
      block = -jc.off * blockAmp * bh;
      if (jc.scar) {
        // no block falls out of the top bed right under the crest (it would leave the lip floating)
        const guard = needCF ? 1 - smoothstep(col.crest - 1.5 * thick, col.crest - 0.3 * thick, y) : 1;
        block += jc.scar * guard * scarDepth;
      }
    }
    // vesicles / weathering pits in hard rock (small; the blotchy soft-bed pocks are gone)
    const n = 0.5 + 0.5 * ctx.pocket.fbm(x / pocketScale, y / (pocketScale * 0.6), z / pocketScale, 3, 2.1, 0.55);
    const pit = smoothstep(0.62, 0.8, n) * pits * pitDepthM * hardness * (1 - col.oc);
    const rough = ctx.rough.fbm(x * roughFreq, y * roughFreq, z * roughFreq, 2, 2.1, 0.55) * roughAmp;
    const det = detail.amp > 0 ? detail.at(x, y, z, 1 / col.slopeF, hard) : 0;
    // buttress-scale swell of the face plane (smooth: the old blockified lumps read as noise)
    let push = 0;
    if (pushAmp > 0) push = pushAmp * ctx.push.fbm(x / pushL, y / (pushL * 0.6), z / pushL, 2, 2.1, 0.5) * (1 - col.oc * 0.7);
    let lean = 0;
    if (hasLean) {
      const faceH = Math.max(4, col.crest - col.foot);
      const down = Math.min(1, Math.max(0, (col.crest - y) / faceH));
      lean = leanAmp * down * (1 - 0.5 * smoothstep(0.85, 1, down)) * (1 - col.oc);
    }
    const d = bedRelief + jointCarve + block + pit + rough - det - push + lean;
    return d * col.steep * col.w;
  }
  function fCol(col, y) {
    let h = col.h, slopeF = col.slopeF, steep = col.steep;
    if (col.vW > 0) {
      const t = Math.min(1, Math.max(0, (y - col.foot) / Math.max(1, col.crest - col.foot)));
      // eased so the plateau behind the crest and the ground before the foot are not dragged
      const e = t * t * (3 - 2 * t);
      const shift = (e - 0.5) * col.vW;
      const u = (col.x + col.vdx * shift - x0) / cell, wq = (col.z + col.vdz * shift - z0) / cell;
      h = bil(m.height, u, wq);
      const [gx, gz] = grad(u, wq);
      const g = Math.hypot(gx, gz);
      slopeF = Math.min(3, Math.sqrt(1 + g * g));
      steep = Math.max(steep, smoothstep(0.7, 1.7, g));
    }
    const plain = y - h;
    if (col.w <= 1e-6 || steep <= 0) return plain;
    // outside the band where carving can flip the sign the field is just the plain distance
    if (plain < -maxCarve * slopeF - 2 * vox || plain > bump + 2 * vox) return plain;
    const dist = plain / slopeF;
    return plain + (dist + carve(col, y) - plain) * col.w;
  }

  // vertical range: carving d along the normal moves the surface up to d·|∇| vertically
  const reach = maxCarve * Math.min(3, job.slopeF || 3);
  let yLo = Math.floor((job.hmin - reach - 2 * vox) / vox) * vox;
  let yHi = Math.ceil((job.hmax + bump + 2 * vox) / vox) * vox;
  if (hasVertical) {
    // the sheared wall spans foot…crest in every column of the face
    let fmin = Infinity, cmax = -Infinity;
    for (let j = pad; j < Sz - pad; j++) for (let i = pad; i < Sx - pad; i++) { const q = j * Sx + i; if (m.sdfWeight[q] > 1e-6) { if (m.foot[q] < fmin) fmin = m.foot[q]; if (m.crest[q] > cmax) cmax = m.crest[q]; } }
    if (fmin < Infinity) { yLo = Math.min(yLo, Math.floor((fmin - 2 * vox) / vox) * vox); yHi = Math.max(yHi, Math.ceil((cmax + bump + 2 * vox) / vox) * vox); }
  }
  const ny = Math.max(1, Math.round((yHi - yLo) / vox));
  const sx = nx + 1, sy = ny + 1, sz = nz + 1;
  // only the band around the surface is sampled / polygonised; nodes outside it are the plain
  // distance, whose sign is known from the column height
  const vals = new Float32Array(sx * sy * sz);
  const bandLo = new Int32Array(sx * sz), bandHi = new Int32Array(sx * sz);
  for (let jz = 0; jz < sz; jz++) {
    const z = z0 + jz * vox;
    for (let jx = 0; jx < sx; jx++) {
      const col = column(x0 + jx * vox, z);
      let lo = Math.max(0, Math.floor((col.h - maxCarve * col.slopeF - 2 * vox - yLo) / vox) - 1);
      let hi = Math.min(ny, Math.ceil((col.h + bump + 2 * vox - yLo) / vox) + 1);
      if (col.vW > 0) {
        lo = Math.min(lo, Math.max(0, Math.floor((col.foot - 2 * vox - yLo) / vox) - 1));
        hi = Math.max(hi, Math.min(ny, Math.ceil((col.crest + bump + 2 * vox - yLo) / vox) + 1));
      }
      bandLo[jz * sx + jx] = lo; bandHi[jz * sx + jx] = hi;
      const base = jz * sy * sx + jx;
      for (let jy = 0; jy < lo; jy++) vals[base + jy * sx] = yLo + jy * vox - col.h;
      for (let jy = lo; jy <= hi; jy++) vals[base + jy * sx] = fCol(col, yLo + jy * vox);
      for (let jy = hi + 1; jy < sy; jy++) vals[base + jy * sx] = yLo + jy * vox - col.h;
    }
  }
  const idx = (ix, iy, iz) => (iz * sy + iy) * sx + ix;

  // welded output: one vertex per crossed grid edge (axis, ix, iy, iz)
  let positions = [], indices = [];
  const edgeIds = new Int32Array(3 * sx * sy * sz).fill(-1);
  const EDGE_AXIS = [0, 1, 0, 1, 0, 1, 0, 1, 2, 2, 2, 2];
  const EDGE_OFF = [[0, 0, 0], [1, 0, 0], [0, 1, 0], [0, 0, 0], [0, 0, 1], [1, 0, 1], [0, 1, 1], [0, 0, 1], [0, 0, 0], [1, 0, 0], [1, 1, 0], [0, 1, 0]];
  const cv = new Float64Array(8), cp = new Float64Array(24);
  const vid = new Int32Array(12);
  for (let iz = 0; iz < nz; iz++) {
    for (let ix = 0; ix < nx; ix++) {
      const c00 = iz * sx + ix, c10 = c00 + 1, c01 = c00 + sx, c11 = c01 + 1;
      const iyLo = Math.max(0, Math.min(bandLo[c00], bandLo[c10], bandLo[c01], bandLo[c11]));
      const iyHi = Math.min(ny - 1, Math.max(bandHi[c00], bandHi[c10], bandHi[c01], bandHi[c11]));
      for (let iy = iyLo; iy <= iyHi; iy++) {
        let cube = 0;
        for (let c = 0; c < 8; c++) {
          const val = vals[idx(ix + CORNER[c][0], iy + CORNER[c][1], iz + CORNER[c][2])];
          cv[c] = val; if (val < 0) cube |= 1 << c;
          cp[c * 3] = x0 + (ix + CORNER[c][0]) * vox; cp[c * 3 + 1] = yLo + (iy + CORNER[c][1]) * vox; cp[c * 3 + 2] = z0 + (iz + CORNER[c][2]) * vox;
        }
        const edges = edgeTable[cube];
        if (edges === 0) continue;
        for (let e = 0; e < 12; e++) {
          if (!(edges & (1 << e))) continue;
          const off = EDGE_OFF[e];
          const key = (EDGE_AXIS[e] * sz + iz + off[2]) * sy * sx + (iy + off[1]) * sx + ix + off[0];
          let id = edgeIds[key];
          if (id < 0) {
            const a = EDGES[e][0], b = EDGES[e][1];
            const t = cv[a] / (cv[a] - cv[b]);
            id = positions.length / 3;
            positions.push(cp[a * 3] + (cp[b * 3] - cp[a * 3]) * t, cp[a * 3 + 1] + (cp[b * 3 + 1] - cp[a * 3 + 1]) * t, cp[a * 3 + 2] + (cp[b * 3 + 2] - cp[a * 3 + 2]) * t);
            edgeIds[key] = id;
          }
          vid[e] = id;
        }
        // field gradient over the cube → outward direction for the winding check
        const gx = (cv[1] + cv[2] + cv[5] + cv[6]) - (cv[0] + cv[3] + cv[4] + cv[7]);
        const gy = (cv[2] + cv[3] + cv[6] + cv[7]) - (cv[0] + cv[1] + cv[4] + cv[5]);
        const gz = (cv[4] + cv[5] + cv[6] + cv[7]) - (cv[0] + cv[1] + cv[2] + cv[3]);
        const base = cube * 16;
        for (let t = 0; triTable[base + t] !== -1; t += 3) {
          let ia = vid[triTable[base + t]], ib = vid[triTable[base + t + 1]], ic = vid[triTable[base + t + 2]];
          if (ia === ib || ib === ic || ia === ic) continue;
          const ax = positions[ia * 3], ay = positions[ia * 3 + 1], az = positions[ia * 3 + 2];
          const ux = positions[ib * 3] - ax, uy = positions[ib * 3 + 1] - ay, uz = positions[ib * 3 + 2] - az;
          const wx = positions[ic * 3] - ax, wy = positions[ic * 3 + 1] - ay, wz = positions[ic * 3 + 2] - az;
          const nx_ = uy * wz - uz * wy, ny_ = uz * wx - ux * wz, nz_ = ux * wy - uy * wx;
          if (nx_ * gx + ny_ * gy + nz_ * gz < 0) { const tmp = ib; ib = ic; ic = tmp; }
          indices.push(ia, ib, ic);
        }
      }
    }
  }

  // drop floater flakes and detached blocks: the carve can pinch off small components
  // (thin lips, block-loss cubes) that hover next to the wall. Keep every component with
  // ≥1% of the triangles (a second rock mass in the chunk is always bigger than that) and
  // never drop the largest; compact the vertices so no orphans ship to the worker result.
  {
    const nv = positions.length / 3, nt = indices.length / 3;
    if (nt > 0) {
      const parent = new Int32Array(nv);
      for (let i = 0; i < nv; i++) parent[i] = i;
      const find = (a) => { while (parent[a] !== a) { parent[a] = parent[parent[a]]; a = parent[a]; } return a; };
      for (let t = 0; t < nt; t++) {
        const r0 = find(indices[t * 3]);
        let r = r0;
        const r1 = find(indices[t * 3 + 1]), r2 = find(indices[t * 3 + 2]);
        if (r1 !== r) { parent[r1] = r; }
        if (r2 !== r) { parent[r2] = r; }
      }
      const compTris = new Map();
      for (let t = 0; t < nt; t++) { const r = find(indices[t * 3]); compTris.set(r, (compTris.get(r) || 0) + 1); }
      let best = -1, bestN = -1;
      for (const [r, n] of compTris) if (n > bestN) { bestN = n; best = r; }
      const keepN = Math.max(1, Math.ceil(nt * 0.01));
      if (compTris.size > 1) {
        const remap = new Int32Array(nv).fill(-1);
        const npos = [], nidx = [];
        for (let t = 0; t < nt; t++) {
          const r = find(indices[t * 3]);
          if (r !== best && compTris.get(r) < keepN) continue;
          for (let k = 0; k < 3; k++) {
            const v = indices[t * 3 + k];
            if (remap[v] < 0) { remap[v] = npos.length / 3; npos.push(positions[v * 3], positions[v * 3 + 1], positions[v * 3 + 2]); }
            nidx.push(remap[v]);
          }
        }
        positions = npos; indices = nidx;
      }
    }
  }

  const count = positions.length / 3;
  const pos = new Float32Array(positions);
  const normals = new Float32Array(count * 3);
  const aux = new Float32Array(count * 4), aux2 = new Float32Array(count * 4);
  const g = new Float64Array(3);
  const gAt = (ix, iy, iz) => {
    const xa = Math.max(0, ix - 1), xb = Math.min(nx, ix + 1), ya = Math.max(0, iy - 1), yb = Math.min(ny, iy + 1), za = Math.max(0, iz - 1), zb = Math.min(nz, iz + 1);
    g[0] = (vals[idx(xb, iy, iz)] - vals[idx(xa, iy, iz)]) / (xb - xa);
    g[1] = (vals[idx(ix, yb, iz)] - vals[idx(ix, ya, iz)]) / (yb - ya);
    g[2] = (vals[idx(ix, iy, zb)] - vals[idx(ix, iy, za)]) / (zb - za);
  };
  for (let n = 0; n < count; n++) {
    const x = pos[n * 3], y = pos[n * 3 + 1], z = pos[n * 3 + 2];
    const u = Math.min(nx - 1e-6, Math.max(0, (x - x0) / vox)), vv = Math.min(ny - 1e-6, Math.max(0, (y - yLo) / vox)), w = Math.min(nz - 1e-6, Math.max(0, (z - z0) / vox));
    const ix = Math.floor(u), iy = Math.floor(vv), iz = Math.floor(w), fx = u - ix, fy = vv - iy, fz = w - iz;
    let sx_ = 0, sy_ = 0, sz_ = 0;
    for (let c = 0; c < 8; c++) {
      const wgt = (CORNER[c][0] ? fx : 1 - fx) * (CORNER[c][1] ? fy : 1 - fy) * (CORNER[c][2] ? fz : 1 - fz);
      if (wgt === 0) continue;
      gAt(ix + CORNER[c][0], iy + CORNER[c][1], iz + CORNER[c][2]);
      sx_ += g[0] * wgt; sy_ += g[1] * wgt; sz_ += g[2] * wgt;
    }
    const l = Math.hypot(sx_, sy_, sz_) || 1;
    normals[n * 3] = sx_ / l; normals[n * 3 + 1] = sy_ / l; normals[n * 3 + 2] = sz_ / l;
    // surface channels sampled by XZ (the shader is world-space, no UVs)
    const cu = (x - x0) / cell, cw = (z - z0) / cell;
    aux[n * 4] = bil(m.deposit, cu, cw);
    aux[n * 4 + 1] = bil(m.flow, cu, cw);
    // hardness from the 3-D strata model so beds read correctly on the undercut faces
    aux[n * 4 + 2] = ctx.strata.sample(x, z, y)[0];
    aux[n * 4 + 3] = bil(m.cavity, cu, cw);
    aux2[n * 4] = bil(m.road, cu, cw);
    aux2[n * 4 + 1] = bil(m.river, cu, cw);
    aux2[n * 4 + 2] = bil(m.lake, cu, cw);
    aux2[n * 4 + 3] = m.waterPaint ? bil(m.waterPaint, cu, cw) : m.waterLevel[Math.min(Sz - 1, Math.max(0, Math.round(cw) + pad)) * Sx + Math.min(Sx - 1, Math.max(0, Math.round(cu) + pad))];
  }
  const index = count > 65535 ? new Uint32Array(indices) : new Uint16Array(indices);
  return { positions: pos, normals, aux, aux2, index, voxels: nx * ny * nz, triangles: indices.length / 3 };
}
