/**
 * Cube-sphere addressing for a planet.
 *
 * A planet cannot be a flat 100 km plate, and a lat/long grid produces
 * pathological cells at the poles. The world is therefore addressed as the six
 * faces of a cube projected onto the sphere, and every terrain chunk is a square
 * patch on one face.
 *
 * Coordinates, in order of increasing local-ness:
 *
 *   face s,t       s,t in [0,1] on face f      — chunk indexing, disk cache keys
 *   face u,v       u,v in [-1,1] on face f     — equal-angle projection plane
 *   dir            unit vector, planet-centred — the SDF authoring space
 *   surface point  dir * (R + h)               — render space, doubles in JS
 *
 * The SDF is authored on `dir` (3D), which is why there are no poles and no
 * seams in the field, and why chunking is independent of field representation.
 *
 * The projection is equal-angle: dir = normalize(n + right*u + up*v). Cell area
 * therefore varies by at most 3^1.5 ≈ 5.2× between a face centre and a face
 * corner; every consumer that cares (LOD selection, splat radii) measures real
 * chunk edge length instead of assuming uniformity, so this is handled rather
 * than hidden.
 */

import { type Vec3, normalize, cross, dot } from '../math/vec';
import { seedFor } from '../math/noise';

export const FACE_PLUS_X = 0;
export const FACE_MINUS_X = 1;
export const FACE_PLUS_Y = 2;
export const FACE_MINUS_Y = 3;
export const FACE_PLUS_Z = 4;
export const FACE_MINUS_Z = 5;
export const FACE_COUNT = 6;

export const FACE_NAMES = ['+X', '-X', '+Y', '-Y', '+Z', '-Z'] as const;

/** Basis of a face: right = increasing s, up = increasing t, normal = face dir. */
const FACE_BASIS: ReadonlyArray<{ normal: Vec3; right: Vec3; up: Vec3 }> = [
  { normal: { x: 1, y: 0, z: 0 }, right: { x: 0, y: 0, z: -1 }, up: { x: 0, y: 1, z: 0 } },
  { normal: { x: -1, y: 0, z: 0 }, right: { x: 0, y: 0, z: 1 }, up: { x: 0, y: 1, z: 0 } },
  { normal: { x: 0, y: 1, z: 0 }, right: { x: 1, y: 0, z: 0 }, up: { x: 0, y: 0, z: -1 } },
  { normal: { x: 0, y: -1, z: 0 }, right: { x: 1, y: 0, z: 0 }, up: { x: 0, y: 0, z: 1 } },
  { normal: { x: 0, y: 0, z: 1 }, right: { x: 1, y: 0, z: 0 }, up: { x: 0, y: 1, z: 0 } },
  { normal: { x: 0, y: 0, z: -1 }, right: { x: -1, y: 0, z: 0 }, up: { x: 0, y: 1, z: 0 } },
];

/** (face, s, t in [0,1]) -> unit direction. */
export function faceSTToDir(face: number, s: number, t: number): Vec3 {
  const b = FACE_BASIS[face];
  const u = s * 2 - 1;
  const v = t * 2 - 1;
  return normalize({
    x: b.normal.x + b.right.x * u + b.up.x * v,
    y: b.normal.y + b.right.y * u + b.up.y * v,
    z: b.normal.z + b.right.z * u + b.up.z * v,
  });
}

/** Unit direction -> (face, s, t). Exact inverse of {@link faceSTToDir}. */
export function dirToFaceST(dir: Vec3): { face: number; s: number; t: number } {
  const ax = Math.abs(dir.x), ay = Math.abs(dir.y), az = Math.abs(dir.z);
  let face: number;
  if (ax >= ay && ax >= az) face = dir.x >= 0 ? FACE_PLUS_X : FACE_MINUS_X;
  else if (ay >= az) face = dir.y >= 0 ? FACE_PLUS_Y : FACE_MINUS_Y;
  else face = dir.z >= 0 ? FACE_PLUS_Z : FACE_MINUS_Z;

  const b = FACE_BASIS[face];
  const n = dot(dir, b.normal);
  const inv = n > 1e-12 ? 1 / n : 1e12;
  const u = dot(dir, b.right) * inv;
  const v = dot(dir, b.up) * inv;
  return { face, s: (u + 1) * 0.5, t: (v + 1) * 0.5 };
}

/** Orthonormal tangent frame on the surface at `dir`. */
export function tangentFrame(dir: Vec3): { right: Vec3; up: Vec3; normal: Vec3 } {
  const up0: Vec3 = Math.abs(dir.y) > 0.99 ? { x: 0, y: 0, z: 1 } : { x: 0, y: 1, z: 0 };
  const right = normalize(cross(up0, dir));
  const up = normalize(cross(dir, right));
  return { right, up, normal: dir };
}

/** Great-circle angular distance between two unit directions, radians. */
export function angleBetween(a: Vec3, b: Vec3): number {
  return Math.acos(Math.min(1, Math.max(-1, dot(a, b))));
}

/** Square patch on one cube face. `lod` is the number of halvings of the face. */
export interface ChunkKey {
  face: number;
  lod: number;
  x: number;
  y: number;
}

export function chunkKeyString(key: ChunkKey): string {
  return `${key.face}/${key.lod}/${key.x}/${key.y}`;
}

export function parseChunkKey(s: string): ChunkKey {
  const [face, lod, x, y] = s.split('/').map(Number);
  return { face, lod, x, y };
}

/** Stable numeric id used for cache directory names and worker map keys. */
export function chunkKeyId(key: ChunkKey): number {
  return seedFor('chunk', key.face, key.lod, key.x, key.y);
}

export const chunkEquals = (a: ChunkKey, b: ChunkKey): boolean =>
  a.face === b.face && a.lod === b.lod && a.x === b.x && a.y === b.y;

/** Nominal edge length in metres (face centre density); measure locally for exact. */
export function chunkEdgeMetres(key: ChunkKey, planetRadius: number): number {
  return (planetRadius * (Math.PI / 2)) / Math.pow(2, key.lod);
}

/** True edge length in metres at the chunk centre — varies across a face. */
export function chunkEdgeMetresLocal(key: ChunkKey, planetRadius: number): number {
  const b = chunkBounds(key);
  const step = 1 / Math.pow(2, key.lod);
  const p0 = faceSTToDir(key.face, b.s0, (b.t0 + b.t1) * 0.5);
  const p1 = faceSTToDir(key.face, b.s0 + step, (b.t0 + b.t1) * 0.5);
  return angleBetween(p0, p1) * planetRadius;
}

export function chunkBounds(key: ChunkKey): {
  s0: number; t0: number; s1: number; t1: number; centerDir: Vec3;
} {
  const n = Math.pow(2, key.lod);
  const s0 = key.x / n;
  const t0 = key.y / n;
  const s1 = (key.x + 1) / n;
  const t1 = (key.y + 1) / n;
  return { s0, t0, s1, t1, centerDir: faceSTToDir(key.face, (s0 + s1) * 0.5, (t0 + t1) * 0.5) };
}

export function chunkParent(key: ChunkKey): ChunkKey | null {
  if (key.lod === 0) return null;
  return { face: key.face, lod: key.lod - 1, x: key.x >> 1, y: key.y >> 1 };
}

export function chunkChildren(key: ChunkKey): ChunkKey[] {
  const l = key.lod + 1;
  return [
    { face: key.face, lod: l, x: key.x * 2, y: key.y * 2 },
    { face: key.face, lod: l, x: key.x * 2 + 1, y: key.y * 2 },
    { face: key.face, lod: l, x: key.x * 2, y: key.y * 2 + 1 },
    { face: key.face, lod: l, x: key.x * 2 + 1, y: key.y * 2 + 1 },
  ];
}

export function rootChunks(): ChunkKey[] {
  const out: ChunkKey[] = [];
  for (let f = 0; f < FACE_COUNT; f++) out.push({ face: f, lod: 0, x: 0, y: 0 });
  return out;
}

/** Latitude/longitude of a direction, degrees. Y is the polar axis. */
export function dirToLatLon(dir: Vec3): { lat: number; lon: number } {
  return {
    lat: Math.asin(Math.max(-1, Math.min(1, dir.y))) * (180 / Math.PI),
    lon: Math.atan2(dir.z, dir.x) * (180 / Math.PI),
  };
}

export function latLonToDir(latDeg: number, lonDeg: number): Vec3 {
  const lat = latDeg * (Math.PI / 180);
  const lon = lonDeg * (Math.PI / 180);
  const cl = Math.cos(lat);
  return { x: cl * Math.cos(lon), y: Math.sin(lat), z: cl * Math.sin(lon) };
}
