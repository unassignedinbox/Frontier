/**
 * Chunk mesher: one cube-sphere chunk -> renderable geometry.
 *
 * Both paths are the *same* clamped dual-contouring lattice over the chunk's
 * altitude band. That single decision buys two things that matter on a planet:
 *
 *   1. Watertight mixed LOD. A chunk at LOD 12 and its neighbour at LOD 11
 *      evaluate the field at identical physical points along their shared border,
 *      so the dual mesh closes by construction. No skirt stitches, no T-junction
 *      cracks, and a radial chunk can sit next to a volumetric one.
 *
 *   2. Free curvature correctness. Vertices are placed at dir * (R + h), so there
 *      is no tangent-plane sag — the "flat plate meets the sky" artefact cannot
 *      occur. Planet scale is a property of the model, not a mitigation.
 *
 *   RADIAL      scalar = altitude - terrainHeight. The vertical axis is separable,
 *               so all coast/height cost is paid once per column. Used wherever
 *               the field is single-valued in radius: most of a planet.
 *
 *   VOLUMETRIC  scalar = the true 3D SDF including fracture carving. Same lattice,
 *               same border behaviour, but able to express overhangs, caves and
 *               crack walls that no height function can.
 *
 * Vertices are written relative to the chunk origin. The render layer subtracts
 * the camera in float64, so nothing planet-sized reaches the GPU as float32.
 */

import { type Vec3, cross, dot, mul, normalize, sub } from '../math/vec';
import { chunkBounds, faceSTToDir, type ChunkKey, chunkEdgeMetresLocal } from '../sdf/cubesphere';
import { TerrainField } from '../world/terrainField';
import type { TerrainWorld } from '../world/terrainWorld';
import { shadeSurface } from '../world/shading';
import { computeNormals, surfaceNets, type MeshBuffers } from './surfaceNets';

export type MeshMode = 'radial' | 'volumetric';

export interface ChunkMeshOptions {
  /** Lattice cells per side. 16 far, 32 default, 48 hero. */
  cells: number;
  /** Force the volumetric path regardless of field complexity. */
  forceVolumetric?: boolean;
  /** How far below the lowest surface sample the volume reaches, metres. */
  volumeDepth?: number;
  /** Full strata/coast tint. Roughly +30 %; off for far LODs. */
  detail: boolean;
}

export interface ChunkMeshResult extends MeshBuffers {
  key: ChunkKey;
  mode: MeshMode;
  origin: Vec3;
  radius: number;
  minAltitude: number;
  maxAltitude: number;
  buildMs: number;
}

export interface ChunkFrame {
  origin: Vec3;
  right: Vec3;
  up: Vec3;
  normal: Vec3;
  edgeMetres: number;
}

function emptyResult(key: ChunkKey, origin: Vec3, mode: MeshMode): ChunkMeshResult {
  return {
    positions: new Float32Array(0), normals: new Float32Array(0),
    colors: new Float32Array(0), indices: new Uint32Array(0),
    quadCount: 0, triangleCount: 0, key, mode, origin,
    radius: 0, minAltitude: 0, maxAltitude: 0, buildMs: 0,
  };
}

/** Tangent frame for a chunk, shared by every chunk on the same face. */
export function chunkFrame(key: ChunkKey, world: TerrainWorld, field: TerrainField): ChunkFrame {
  const b = chunkBounds(key);
  const normal = b.centerDir;
  const sMid = (b.s0 + b.s1) * 0.5;
  const tMid = (b.t0 + b.t1) * 0.5;
  const eps = 1e-3;
  const pS = faceSTToDir(key.face, sMid + eps, tMid);
  const right = normalize(sub(pS, mul(normal, dot(pS, normal))));
  const up = normalize(cross(normal, right));
  const altitude = field.heightAt(normal);
  return {
    origin: mul(normal, world.planetRadius + altitude),
    right, up, normal,
    edgeMetres: chunkEdgeMetresLocal(key, world.planetRadius),
  };
}

/** Unit direction of a chunk-local (x, z) offset from the frame origin. */
export function dirFromLocal(frame: ChunkFrame, x: number, z: number, planetRadius: number): Vec3 {
  const a = x / planetRadius;
  const b = z / planetRadius;
  return normalize({
    x: frame.normal.x + frame.right.x * a + frame.up.x * b,
    y: frame.normal.y + frame.right.y * a + frame.up.y * b,
    z: frame.normal.z + frame.right.z * a + frame.up.z * b,
  });
}

/**
 * Does this world need the volumetric path?
 *
 * Fractures undercut, so a document containing one is meshed as a volume. This is
 * a property of the document rather than a guess from noisy samples, so it is
 * stable across LODs and cache entries.
 */
export function worldNeedsVolume(world: TerrainWorld): boolean {
  for (const op of world.operators) if (op.type === 'fracture') return true;
  return false;
}

export function buildChunkMesh(
  key: ChunkKey, world: TerrainWorld, field: TerrainField, opts: ChunkMeshOptions,
): ChunkMeshResult {
  const t0 = now();
  const cells = Math.max(4, Math.min(96, Math.round(opts.cells)));
  const frame = chunkFrame(key, world, field);
  const volume = opts.forceVolumetric ?? worldNeedsVolume(world);
  // Coarse chunks skip the finest relief octaves: at hundreds of metres per
  // cell, sub-cell noise costs time and shows nothing.
  const previousQuality = field.octaveScale;
  const cellMetres = frame.edgeMetres / cells;
  field.octaveScale = cellMetres < 40 ? 1 : cellMetres < 160 ? 0.8 : 0.6;
  const result = buildLattice(key, world, field, frame, cells, opts, volume);
  field.octaveScale = previousQuality;
  result.buildMs = now() - t0;
  return result;
}

/* -------------------------------------------------------------- lattice build */

interface Band {
  topAlt: number;
  range: number;
  /** Chunk-local y units per metre of altitude. */
  yPerAlt: number;
}

function buildLattice(
  key: ChunkKey, world: TerrainWorld, field: TerrainField,
  frame: ChunkFrame, n: number, opts: ChunkMeshOptions, volume: boolean,
): ChunkMeshResult {
  const size = frame.edgeMetres;
  const s = n + 1;
  const R = world.planetRadius;
  const half = size * 0.5;

  // ---- columns: the expensive part, evaluated once -------------------------
  const height = new Float32Array(s * s);
  const coast = new Float32Array(s * s);
  const dirs: Vec3[] = new Array(s * s);
  let minH = Infinity, maxH = -Infinity;
  for (let j = 0; j < s; j++) {
    for (let i = 0; i < s; i++) {
      const dir = dirFromLocal(frame, (i / n) * size - half, (j / n) * size - half, R);
      const idx = i + j * s;
      const c = field.coastDistance(dir);
      const h = field.heightAt(dir, c);
      coast[idx] = c;
      height[idx] = h;
      dirs[idx] = dir;
      if (h < minH) minH = h;
      if (h > maxH) maxH = h;
    }
  }

  // ---- altitude band, packed into a cubic lattice --------------------------
  const topAlt = maxH + Math.max(16, size * 0.006);
  const bottomAlt = volume
    ? Math.min(minH - (opts.volumeDepth ?? Math.max(650, size * 0.25)), topAlt - 240)
    : minH - Math.max(60, size * 0.05);
  const range = Math.max(1, topAlt - bottomAlt);
  const band: Band = { topAlt, range, yPerAlt: size / range };
  const altOf = (y: number): number => topAlt - (size - y) / band.yPerAlt;

  const place = (x: number, y: number, z: number): [number, number, number] => {
    const dir = dirFromLocal(frame, x - half, z - half, R);
    const r = R + altOf(y);
    return [dir.x * r - frame.origin.x, dir.y * r - frame.origin.y, dir.z * r - frame.origin.z];
  };

  const colAt = (x: number, z: number): number => {
    const i = Math.max(0, Math.min(n, Math.round((x / size) * n)));
    const j = Math.max(0, Math.min(n, Math.round((z / size) * n)));
    return i + j * s;
  };

  // Slope from the height lattice: one diff per vertex, no extra field calls.
  const slopeOf = (idx: number, i: number, j: number): number => {
    const hr = height[Math.min(n, i + 1) + j * s] ?? height[idx];
    const hd = height[i + Math.min(n, j + 1) * s] ?? height[idx];
    return Math.hypot(hr - height[idx], hd - height[idx]) / (size / n);
  };

  const sample = (i: number, j: number, k: number, _x: number, y: number, z: number): number => {
    void i; void j; void k;
    const idx = colAt(_x, z);
    const alt = altOf(y);
    if (!volume) return alt - height[idx];
    const dir = dirs[idx];
    return field.densityAtColumn(mul(dir, R + alt), dir, coast[idx], height[idx]);
  };

  const tint = (_p: Vec3, lattice: Vec3): [number, number, number] => {
    const alt = altOf(lattice.y);
    if (!opts.detail) {
      // Cheap far tint: sea vs land from the sampled altitude.
      return alt < 0 ? [0.14, 0.25, 0.32] : [0.32, 0.38, 0.24];
    }
    const i = Math.max(0, Math.min(n, Math.round((lattice.x / size) * n)));
    const j = Math.max(0, Math.min(n, Math.round((lattice.z / size) * n)));
    const idx = i + j * s;
    const info = field.surfaceInfo(dirs[idx], alt, coast[idx], slopeOf(idx, i, j));
    const c = shadeSurface(world, dirs[idx], info, info.slope);
    return [c.r, c.g, c.b];
  };

  const mesh = surfaceNets({ cells: n, size, sample, place, tint });

  if (mesh.indices.length === 0) {
    return { ...emptyResult(key, frame.origin, volume ? 'volumetric' : 'radial'), minAltitude: bottomAlt, maxAltitude: topAlt };
  }

  return {
    positions: mesh.positions,
    colors: mesh.colors,
    indices: mesh.indices,
    normals: computeNormals(mesh.positions, mesh.indices),
    quadCount: mesh.quadCount,
    triangleCount: mesh.triangleCount,
    key,
    mode: volume ? 'volumetric' : 'radial',
    origin: frame.origin,
    radius: size * 0.72,
    minAltitude: bottomAlt,
    maxAltitude: topAlt,
    buildMs: 0,
  };
}

function now(): number {
  return typeof performance !== 'undefined' ? performance.now() : Date.now();
}
