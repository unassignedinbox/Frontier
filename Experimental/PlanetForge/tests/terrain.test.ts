import { describe, expect, it } from 'vitest';
import { TerrainField } from '../src/core/world/terrainField';
import {
  createEmptyWorld, createPreset, cap, capsule, band, worldHash, cloneWorld,
  type TerrainWorld,
} from '../src/core/world/terrainWorld';
import { latLonToDir } from '../src/core/sdf/cubesphere';
import { buildChunkMesh, worldNeedsVolume } from '../src/core/mesh/chunkMesher';
import { chunkEdgeMetresLocal } from '../src/core/sdf/cubesphere';
import { surfaceNets } from '../src/core/mesh/surfaceNets';
import { normalize, dot, mul } from '../src/core/math/vec';

const R = 6_371_000;

function sphereDensity(radius: number) {
  return (x: number, y: number, z: number): number => Math.hypot(x, y, z) - radius;
}

describe('terrain field: land / sea CSG', () => {
  it('puts land inside a landmass cap and sea outside it', () => {
    const world = createEmptyWorld();
    world.operators.push(cap('island', 'land', 0, 0, 800_000, R));
    const field = new TerrainField(world);

    const inside = field.heightAt(latLonToDir(0, 0));
    const outside = field.heightAt(latLonToDir(0, 120));
    expect(inside).toBeGreaterThan(0);
    expect(outside).toBeLessThan(0);
    expect(inside).toBeGreaterThan(50);
    // Abyssal plain is bounded by the configured ocean depth.
    expect(outside).toBeGreaterThan(-world.oceanDepth - 400);
  });

  it('subtracting a sea returns that area to water', () => {
    const world = createEmptyWorld();
    world.operators.push(cap('continent', 'land', 0, 0, 1_500_000, R));
    const field = new TerrainField(world);
    const before = field.heightAt(latLonToDir(0, 0));
    expect(before).toBeGreaterThan(0);

    world.operators.push(cap('inland-sea', 'water', 0, 0, 400_000, R));
    field.reindex();
    const after = field.heightAt(latLonToDir(0, 0));
    expect(after).toBeLessThan(0);
  });

  it('is deterministic across instances and independent of evaluation order', () => {
    const a = new TerrainField(createPreset('continents'));
    const b = new TerrainField(createPreset('continents'));
    const dirs = [latLonToDir(12, 30), latLonToDir(-40, 70), latLonToDir(60, -10)];
    for (const d of dirs) {
      expect(a.heightAt(d)).toBeCloseTo(b.heightAt(d), 9);
      expect(a.densityAt(mul(d, R + 100))).toBeCloseTo(b.densityAt(mul(d, R + 100)), 9);
    }
  });

  it('document hash changes when the document changes', () => {
    const w = createPreset('continents');
    const h1 = worldHash(w);
    const w2 = cloneWorld(w);
    expect(worldHash(w2)).toBe(h1);
    w2.operators.push(cap('extra', 'land', 10, 10, 200_000, R));
    expect(worldHash(w2)).not.toBe(h1);
  });

  it('a ridge raises the surface along its capsule', () => {
    const world = createEmptyWorld();
    world.operators.push(cap('land', 'land', 0, 0, 2_000_000, R, { roughness: 0 }));
    const flat = new TerrainField(world);
    const before = flat.heightAt(latLonToDir(0, 0));
    world.operators.push(capsule('range', 'ridge', -5, 0, 5, 0, 220_000, 2200));
    flat.reindex();
    const after = flat.heightAt(latLonToDir(0, 0));
    expect(after).toBeGreaterThan(before + 200);
  });
});

describe('density field', () => {
  it('is negative inside the earth and positive above the surface', () => {
    const field = new TerrainField(createPreset('continents'));
    for (const [lat, lon] of [[0, 0], [30, 30], [-20, 140]]) {
      const dir = latLonToDir(lat, lon);
      const h = field.heightAt(dir);
      const above = field.densityAt(mul(dir, R + h + 500));
      const below = field.densityAt(mul(dir, R + h - 500));
      expect(above).toBeGreaterThan(0);
      expect(below).toBeLessThan(0);
    }
  });

  it('a fracture network opens real volume in the field', () => {
    const world = createEmptyWorld();
    world.operators.push(cap('land', 'land', 0, 0, 2_000_000, R, { roughness: 0 }));
    const field = new TerrainField(world);
    const dir = latLonToDir(0, 0);
    const h = field.heightAt(dir);
    const samplePoint = mul(dir, R + h + 4);
    expect(field.densityAt(samplePoint)).toBeGreaterThan(0);

    world.operators.push({
      id: 'cracks',
      type: 'fracture',
      pattern: 'voronoi',
      cellSize: 900,
      width: 40,
      depth: 320,
      minAltitude: -10,
      minLandDistance: 0,
      seed: 12345,
    });
    field.reindex();
    expect(worldNeedsVolume(world)).toBe(true);
    // Walk a line east along the surface; a crack must open at least one sample
    // in the top 20 m into empty space (positive density).
    const up = normalize(latLonToDir(0, 0));
    const east = normalize({ x: -up.z, y: 0, z: up.x });
    let foundHole = false;
    for (let i = 0; i < 400 && !foundHole; i++) {
      const d = normalize({ x: up.x + east.x * i * 2e-5, y: up.y, z: up.z + east.z * i * 2e-5 });
      const hh = field.heightAt(d);
      for (let dy = -20; dy <= 0; dy += 5) {
        if (field.densityAt(mul(d, R + hh + dy)) > 0) { foundHole = true; break; }
      }
    }
    expect(foundHole).toBe(true);
  });
});

describe('surface nets', () => {
  it('extracts a closed sphere whose surface area matches 4*pi*r^2', () => {
    const radius = 40;
    const size = 100;
    const mesh = surfaceNets({
      cells: 48,
      size,
      sample: (_i, _j, _k, x, y, z) => sphereDensity(radius)(
        x - size / 2, y - size / 2, z - size / 2,
      ),
    });
    expect(mesh.triangleCount).toBeGreaterThan(0);

    // Area via the sum of triangle areas.
    let area = 0;
    const p = mesh.positions;
    for (let t = 0; t < mesh.indices.length; t += 3) {
      const a = mesh.indices[t] * 3;
      const b = mesh.indices[t + 1] * 3;
      const c = mesh.indices[t + 2] * 3;
      const ab = { x: p[b] - p[a], y: p[b + 1] - p[a + 1], z: p[b + 2] - p[a + 2] };
      const ac = { x: p[c] - p[a], y: p[c + 1] - p[a + 1], z: p[c + 2] - p[a + 2] };
      const cx = ab.y * ac.z - ab.z * ac.y;
      const cy = ab.z * ac.x - ab.x * ac.z;
      const cz = ab.x * ac.y - ab.y * ac.x;
      area += 0.5 * Math.hypot(cx, cy, cz);
    }
    const expected = 4 * Math.PI * radius * radius;
    expect(area / expected).toBeGreaterThan(0.9);
    expect(area / expected).toBeLessThan(1.1);
  });

  it('produces a watertight surface: every edge is shared by exactly two triangles', () => {
    const size = 80;
    const mesh = surfaceNets({
      cells: 24,
      size,
      sample: (_i, _j, _k, x, y, z) => Math.hypot(x - size / 2, y - size / 2, z - size / 2) - 26,
    });
    const edgeCount = new Map<string, number>();
    for (let t = 0; t < mesh.indices.length; t += 3) {
      const tri = [mesh.indices[t], mesh.indices[t + 1], mesh.indices[t + 2]];
      for (let e = 0; e < 3; e++) {
        const a = tri[e];
        const b = tri[(e + 1) % 3];
        const key = a < b ? `${a}_${b}` : `${b}_${a}`;
        edgeCount.set(key, (edgeCount.get(key) ?? 0) + 1);
      }
    }
    let open = 0;
    for (const count of edgeCount.values()) if (count !== 2) open++;
    expect(open).toBe(0);
  });

  it('emits nothing for an empty region', () => {
    const mesh = surfaceNets({ cells: 12, size: 50, sample: () => 5 });
    expect(mesh.triangleCount).toBe(0);
    expect(mesh.positions.length).toBe(0);
  });
});

describe('chunk meshing', () => {
  it('places radial vertices on the exact sphere, not a tangent plate', () => {
    const world = createEmptyWorld('flat');
    // A deliberately motionless world so the only curvature left is the planet.
    world.operators.push(cap('ocean-world', 'land', 0, 0, 3.0, R, { roughness: 0 }));
    world.relief.landAmplitude = 0;
    world.relief.mountainAmplitude = 0;
    world.relief.seaAmplitude = 0;
    world.relief.warp = 0;
    world.coastHeight = 100;
    world.inlandHeight = 100;
    world.oceanDepth = 100;
    world.shelfWidth = 1;
    const field = new TerrainField(world);
    const key = { face: 0, lod: 4, x: 7, y: 7 };
    const mesh = buildChunkMesh(key, world, field, { cells: 16, detail: false });
    expect(mesh.mode).toBe('radial');
    expect(mesh.triangleCount).toBeGreaterThan(0);

    // Mid-edge vertex: distance from the planet centre must include the sagitta
    // of a flat plate if the mesher were wrong. Check the sphere sag instead.
    const o = mesh.origin;
    const rOrigin = Math.hypot(o.x, o.y, o.z);
    const p = mesh.positions;
    let minRadius = Infinity;
    let maxRadius = 0;
    for (let i = 0; i < p.length; i += 3) {
      const r = Math.hypot(p[i] + o.x, p[i + 1] + o.y, p[i + 2] + o.z);
      minRadius = Math.min(minRadius, r);
      maxRadius = Math.max(maxRadius, r);
    }
    // On a sphere every vertex is the same distance from the planet centre. On a
    // tangent plate the corners would be ~d²/2R *below* the centre — for this
    // chunk that is tens of metres, which this test would catch immediately.
    const spread = maxRadius - minRadius;
    // A tangent plate over this chunk would sag ~half²/2R below the centre
    // (kilometres). A sphere sags zero. Require the sphere.
    const half = chunkEdgeMetresLocal(key, R) * 0.5;
    const plateSag = (half * half) / (2 * R);
    expect(plateSag).toBeGreaterThan(100);
    expect(spread).toBeLessThan(plateSag * 0.02);
    // Smooth world: the whole chunk must sit on one sphere. The exact altitude
    // is the field's business; what matters here is that it is a sphere at all.
    expect(Math.abs(rOrigin - R)).toBeLessThan(2000);
  });

  it('meshes a fractured world volumetrically and finds geometry', () => {
    const world: TerrainWorld = createEmptyWorld();
    world.operators.push(cap('land', 'land', 0, 0, 2.0, R, { roughness: 0 }));
    world.operators.push({
      id: 'cracks', type: 'fracture', pattern: 'voronoi',
      cellSize: 1400, width: 60, depth: 260,
      minAltitude: -20, minLandDistance: 0, seed: 7,
    });
    const field = new TerrainField(world);
    const key = { face: 2, lod: 7, x: 64, y: 64 };
    const mesh = buildChunkMesh(key, world, field, { cells: 24, detail: false });
    expect(mesh.mode).toBe('volumetric');
    expect(mesh.triangleCount).toBeGreaterThan(100);
  });

  it('is deterministic: the same chunk twice gives identical vertices', () => {
    const world = createPreset('archipelago');
    const field = new TerrainField(world);
    const key = { face: 4, lod: 8, x: 100, y: 140 };
    const a = buildChunkMesh(key, world, field, { cells: 16, detail: true });
    const b = buildChunkMesh(key, world, field, { cells: 16, detail: true });
    expect(a.positions.length).toBe(b.positions.length);
    for (let i = 0; i < a.positions.length; i += 37) {
      expect(a.positions[i]).toBeCloseTo(b.positions[i], 9);
    }
  });

  it('keeps chunk vertex counts proportional to cells, not to planet size', () => {
    const world = createPreset('continents');
    const field = new TerrainField(world);
    for (const lod of [3, 9, 14]) {
      const mesh = buildChunkMesh({ face: 0, lod, x: 0, y: 0 }, world, field, { cells: 16, detail: false });
      // 16x16 cells = 289 vertices + a skirt-free radial grid.
      expect(mesh.positions.length / 3).toBeLessThanOrEqual(17 * 17);
    }
  });

  it('keeps the volume path inside a bounded vertex budget per chunk', () => {
    const world = createEmptyWorld();
    world.operators.push(cap('land', 'land', 0, 0, 2.0, R));
    world.operators.push({
      id: 'cracks', type: 'fracture', pattern: 'voronoi',
      cellSize: 2000, width: 40, depth: 300, minAltitude: -50, minLandDistance: 0, seed: 3,
    });
    const field = new TerrainField(world);
    const mesh = buildChunkMesh({ face: 0, lod: 6, x: 32, y: 32 }, world, field, { cells: 32, detail: false });
    // One vertex per crossed cell at most.
    expect(mesh.positions.length / 3).toBeLessThanOrEqual(32 * 32 * 32);
  });
});

describe('strata', () => {
  it('reports the topmost band beneath the surface', () => {
    const world = createEmptyWorld();
    world.operators.push(cap('land', 'land', 0, 0, 3.0, R, { roughness: 0 }));
    world.operators.push(band('b1', -100, 50));
    world.operators.push(band('b2', 400, 50));
    const field = new TerrainField(world);
    const info = field.surfaceInfo(latLonToDir(0, 0));
    if (info.altitude > 400) expect(info.stratum).toBe(400);
    else if (info.altitude > -100) expect(info.stratum).toBe(-100);
    else expect(info.stratum).toBe(-Infinity as unknown as number);
  });
});

describe('fracture mask region', () => {
  it('leaves terrain outside the mask intact', () => {
    const world = createEmptyWorld();
    world.operators.push(cap('land', 'land', 0, 0, 3.0, R, { roughness: 0 }));
    world.operators.push({
      id: 'cracks', type: 'fracture', pattern: 'voronoi',
      cellSize: 800, width: 50, depth: 250,
      minAltitude: -100, minLandDistance: 0,
      mask: { center: latLonToDir(0, 0), radius: 0.02 },
      seed: 99,
    });
    const field = new TerrainField(world);
    const inside = mul(latLonToDir(0, 0), R + 200);
    const outside = mul(latLonToDir(0, 45), R + 200);
    // Outside the mask, density is the plain radial distance minus nothing.
    const dirOut = normalize(outside);
    expect(field.densityAt(outside)).toBeCloseTo(
      Math.sqrt(dot(outside, outside)) - (R + field.heightAt(dirOut)), 3,
    );
    void inside;
  });
});
