/**
 * Performance characterisation.
 *
 * Run with:  npx vitest run tools/bench.test.ts
 *
 * These are the numbers that decide whether a planet-scale SDF editor is usable
 * on a mid-range machine, so they are measured rather than asserted in prose:
 *
 *   - how long one 32³ chunk takes to mesh, and how much RAM it costs;
 *   - what the LOD cut actually looks like at ground level and from orbit;
 *   - how much of that is the field and how much is the extractor;
 *   - how the volumetric (cracked) path compares with the radial path.
 */

import { describe, it } from 'vitest';
import { TerrainField } from '../src/core/world/terrainField';
import { createPreset } from '../src/core/world/terrainWorld';
import { buildChunkMesh } from '../src/core/mesh/chunkMesher';
import { selectChunks, DEFAULT_LOD_SETTINGS } from '../src/core/stream/lod';
import { chunkBounds, chunkEdgeMetresLocal, latLonToDir } from '../src/core/sdf/cubesphere';
import { mul } from '../src/core/math/vec';

const R = 6_371_000;

function report(label: string, value: string): void {
  // eslint-disable-next-line no-console
  console.log(`  ${label.padEnd(38)} ${value}`);
}

describe('performance', () => {
  it('meshes chunks at interactive rates and reports the cost', () => {
    const world = createPreset('continents');
    const field = new TerrainField(world);

    for (const cells of [16, 32, 48]) {
      const keys = [];
      for (let i = 0; i < 6; i++) keys.push({ face: 0, lod: 12, x: 100 + i, y: 120 + i });

      const t0 = performance.now();
      let tris = 0;
      let bytes = 0;
      let minMs = Infinity;
      let maxMs = 0;
      for (const key of keys) {
        const m = buildChunkMesh(key, world, field, { cells, detail: true });
        tris += m.triangleCount;
        bytes += m.positions.byteLength + m.normals.byteLength + m.colors.byteLength + m.indices.byteLength;
        minMs = Math.min(minMs, m.buildMs);
        maxMs = Math.max(maxMs, m.buildMs);
      }
      const total = performance.now() - t0;
      report(`cells=${cells} avg build (detail on)`, `${(total / keys.length).toFixed(1)} ms  [${minMs.toFixed(1)}–${maxMs.toFixed(1)}]`);
      report(`cells=${cells} triangles / chunk`, `${Math.round(tris / keys.length)}`);
      report(`cells=${cells} KB / chunk uploaded`, `${(bytes / keys.length / 1024).toFixed(0)} KB`);
    }

    // Coarse far chunks should be much cheaper because detail shading is off.
    const t1 = performance.now();
    for (let i = 0; i < 6; i++) {
      buildChunkMesh({ face: 1, lod: 8, x: 30 + i, y: 40 + i }, world, field, { cells: 32, detail: false });
    }
    report('cells=32 far LOD (detail off)', `${((performance.now() - t1) / 6).toFixed(1)} ms/chunk`);
  }, 60_000);

  it('shows what the streaming cut costs at ground level and in orbit', () => {
    const settings = { ...DEFAULT_LOD_SETTINGS };
    const ground = {
      camera: mul(latLonToDir(34, 18), R + 2_000),
      view: latLonToDir(34, 20),
    };
    const low = selectChunks(ground.camera, ground.view, settings);
    report('cut @ 2 km altitude: chunks', `${low.requests.length}`);
    report('cut @ 2 km altitude: LOD range',
      `${Math.min(...low.requests.map((r) => r.key.lod))}–${Math.max(...low.requests.map((r) => r.key.lod))}`);
    report('cut @ 2 km altitude: triangles', `${(low.predictedTriangles / 1e6).toFixed(2)} M`);
    report('cut @ 2 km altitude: dropped by budget', `${low.dropped}`);

    const orbit = {
      camera: mul(latLonToDir(34, 18), R + 4_000_000),
      view: latLonToDir(34, 18),
    };
    const high = selectChunks(orbit.camera, orbit.view, settings);
    report('cut @ 4000 km altitude: chunks', `${high.requests.length}`);
    report('cut @ 4000 km altitude: LOD range',
      `${Math.min(...high.requests.map((r) => r.key.lod))}–${Math.max(...high.requests.map((r) => r.key.lod))}`);
    report('cut @ 4000 km altitude: triangles', `${(high.predictedTriangles / 1e6).toFixed(2)} M`);

    // Chunk edge lengths at the finest LOD the budget allows.
    const finest = Math.max(...high.requests.map((r) => r.key.lod));
    const edge = chunkEdgeMetresLocal({ face: 0, lod: finest, x: 1, y: 1 }, R);
    report(`edge length at LOD ${finest}`, `${(edge / 1000).toFixed(1)} km`);
  }, 60_000);

  it('compares the radial and volumetric paths on the same chunk', () => {
    const world = createPreset('continents');
    const field = new TerrainField(world);
    const key = { face: 2, lod: 11, x: 640, y: 620 };

    const t0 = performance.now();
    const radial = buildChunkMesh(key, world, field, { cells: 32, detail: true });
    const radialMs = performance.now() - t0;

    const t1 = performance.now();
    const volume = buildChunkMesh(key, world, field, { cells: 32, detail: false, forceVolumetric: true });
    const volumeMs = performance.now() - t1;

    report('radial chunk build', `${radialMs.toFixed(1)} ms · ${radial.triangleCount} tris`);
    report('volumetric chunk build', `${volumeMs.toFixed(1)} ms · ${volume.triangleCount} tris`);
    report('volumetric mode', volume.mode);
    const b = chunkBounds(key);
    void b;
  }, 60_000);

  it('keeps per-chunk memory flat as LOD deepens (no planet-sized buffers)', () => {
    const world = createPreset('archipelago');
    const field = new TerrainField(world);
    let previous = 0;
    for (const lod of [4, 8, 12, 16]) {
      const key = { face: 3, lod, x: Math.min(1, 1) * (Math.pow(2, lod) - 1), y: 1 };
      const m = buildChunkMesh(key, world, field, { cells: 32, detail: false });
      const kb = (m.positions.byteLength + m.normals.byteLength + m.colors.byteLength + m.indices.byteLength) / 1024;
      report(`LOD ${String(lod).padStart(2)} chunk upload`, `${kb.toFixed(0)} KB · ${m.triangleCount} tris`);
      if (previous > 0) {
        // Within a factor of a few: the mesh detail is set by `cells`, not by how
        // much planet that chunk covers.
        if (kb > previous * 6 + 64 || kb < previous / 6 - 64) {
          throw new Error(`chunk memory scaled with LOD: ${previous} KB -> ${kb} KB`);
        }
      }
      previous = kb;
    }
  }, 60_000);
});
