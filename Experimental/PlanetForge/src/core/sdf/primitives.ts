/**
 * SDF primitives in *planet space*.
 *
 * All terrain operators are signed fields on non-normalised planet points:
 * `p = dir * (R + h)`. A sphere of radius `R` is the reference sea floor, so
 * brush radii and feature sizes are honest metres.
 *
 *  - Spherical operators (landmass, island, shelf) are angular caps. They report
 *    a chord distance that matches great-circle arc length to within
 *    O(r³/R²) ≈ 0.4 % for a 100 km feature on Earth — plenty for smooth
 *    booleans, and cheap enough to evaluate per-sample in a worker.
 *  - Everything is a pure function of (p, params). Nothing is baked, so editing
 *    one feature re-evaluates only the chunks whose bounds touch it.
 */

import {
  type Vec3, cross, dist, dot, mul, normalize, sub, v3, clamp, smoothstep,
} from '../math/vec';
import { fbm3 } from '../math/noise';

/** Earth-radius metres per radian of arc, used to report cap distances in metres. */
const R_UNIT = 6_371_000;

export interface PrimitiveBase {
  id: string;
  /** Soft-blend width in metres. 0 = hard boolean. */
  blend: number;
}

/** Landmass, lake, island, shelf: an angular cap with noisy radius. */
export interface CapPrimitive extends PrimitiveBase {
  type: 'cap';
  kind: 'land' | 'water';
  center: Vec3;
  /** Angular radius, radians. */
  radius: number;
  /** Coastline warp amplitude in radians. 0 = perfect circle. */
  roughness: number;
  /** Coastline warp frequency in cycles per radian of surface. */
  frequency: number;
  /** >1 stretches along `stretchDir`, <1 squashes. 1 = round. */
  stretch: number;
  stretchDir: Vec3;
  seed: number;
}

/** Capsule: mountain range, rift, river corridor, isthmus, trench. */
export interface CapsulePrimitive extends PrimitiveBase {
  type: 'capsule';
  kind: 'ridge' | 'trench' | 'land' | 'water';
  a: Vec3;
  b: Vec3;
  radius: number;
  /** Crest height in metres for ridges, depth for trenches. */
  height: number;
  roughness: number;
  seed: number;
}

/** Elliptical plateau / basin / shelf dome. */
export interface EllipsePrimitive extends PrimitiveBase {
  type: 'ellipse';
  kind: 'plateau' | 'basin' | 'shelf';
  center: Vec3;
  semiMajor: number;
  semiMinor: number;
  rotation: number;
  /** Peak offset in metres; negative for basins. */
  height: number;
  seed: number;
}

/** Stratification datum — a sediment layer's depositional surface. */
export interface BandPrimitive extends PrimitiveBase {
  type: 'band';
  /** Absolute altitude, metres (negative = below sea level). */
  altitude: number;
  /** Thickness, metres. */
  thickness: number;
  /** Optional regional mask: the band only exists inside this cap. */
  mask?: { center: Vec3; radius: number };
  /** Slope tilt across the band, metres per 1000 km along +X of the frame. */
  tilt: number;
  seed: number;
}

export type Primitive = CapPrimitive | CapsulePrimitive | EllipsePrimitive | BandPrimitive;

/* ------------------------------------------------------------------- helpers */

/** Smooth 0..1 ramp, clamped. */
const smoothstep01 = (e0: number, e1: number, x: number): number => {
  const t = Math.max(0, Math.min(1, (x - e0) / (e1 - e0 || 1e-9)));
  return t * t * (3 - 2 * t);
};

/**
 * Warp a direction by a vector-valued noise field and re-project to the sphere.
 * This is what separates "a circle" from "a continent".
 */
export function warpDir(dir: Vec3, seed: number, amplitude: number, frequency: number): Vec3 {
  if (amplitude <= 0) return dir;
  const f = frequency;
  const n1 = fbm3(dir.x * f, dir.y * f, dir.z * f, seed + 101, 3);
  const n2 = fbm3(dir.x * f + 7.1, dir.y * f - 3.3, dir.z * f + 1.7, seed + 202, 3);
  const n3 = fbm3(dir.x * f - 2.2, dir.y * f + 5.5, dir.z * f - 4.4, seed + 303, 3);
  return normalize({
    x: dir.x + n1 * amplitude,
    y: dir.y + n2 * amplitude,
    z: dir.z + n3 * amplitude,
  });
}

export function distPointSegment(p: Vec3, a: Vec3, b: Vec3): number {
  const ab = sub(b, a);
  const ap = sub(p, a);
  const denom = dot(ab, ab);
  const t = denom > 1e-9 ? clamp(dot(ap, ab) / denom, 0, 1) : 0;
  return dist(p, { x: a.x + ab.x * t, y: a.y + ab.y * t, z: a.z + ab.z * t });
}

/** Local tangent basis at a direction: returns [right, up]. */
export function tangentBasis(n: Vec3): [Vec3, Vec3] {
  const up0: Vec3 = Math.abs(n.y) > 0.99 ? v3(0, 0, 1) : v3(0, 1, 0);
  const right = normalize(cross(up0, n));
  const up = normalize(cross(n, right));
  return [right, up];
}

/* ---------------------------------------------------------------- evaluators */

/**
 * Signed distance in metres to the cap boundary (negative inside).
 *
 * The cap is an angular disc: great-circle distance to the centre, compared
 * against an angular radius. Elongation is applied in the tangent plane at the
 * centre, and coastline noise multiplies the radius — so a 100 km island gets
 * ~10 km of coast wobble and a 3000 km continent gets ~300 km, rather than both
 * getting the same absolute displacement.
 */
export function evaluateCap(p: Vec3, cap: CapPrimitive, planetRadius: number): number {
  void planetRadius;
  const dir = normalize(p);
  const cosTheta = clamp(dot(dir, cap.center), -1, 1);
  const theta = Math.acos(cosTheta);

  // Angular radius after coastline noise (noise in [-1,1]).
  let radius = cap.radius;
  if (cap.roughness > 0) {
    // Broad phase: the noise can only move the boundary by ±roughness × radius,
    // so anything further than 4x that from the mean boundary gets the exact
    // noise-free answer. The gate is faded to exactly zero at that distance, so
    // skipping the noise is continuous and cannot produce a terrace.
    const edge = Math.abs(theta - cap.radius) / Math.max(1e-9, cap.radius * cap.roughness);
    if (edge < 4) {
      const gate = 1 - smoothstep01(2.5, 4, edge);
      const f = cap.frequency;
      const octaves = cap.roughness > 0.05 ? 3 : 2;
      const n = fbm3(dir.x * f, dir.y * f, dir.z * f, cap.seed, octaves);
      radius *= 1 + n * cap.roughness * gate;
    }
  }
  radius = Math.max(1e-6, radius);

  if (theta < 1e-7) return -radius * R_UNIT;

  // Effective angular distance, with elongation applied along one bearing.
  let dAng = theta;
  if (Math.abs(cap.stretch - 1) > 1e-3) {
    // Offset direction in the tangent plane at the cap centre.
    const tangent = sub(dir, mul(cap.center, cosTheta));
    const tl = Math.sqrt(dot(tangent, tangent));
    if (tl > 1e-9) {
      const tdir = mul(tangent, 1 / tl);
      const axis = normalize(cap.stretchDir);
      // Project the stretch axis into the tangent plane, then measure the angle
      // between the offset direction and it.
      const a = sub(axis, mul(cap.center, dot(axis, cap.center)));
      const al = Math.sqrt(dot(a, a));
      const k = cap.stretch;
      if (al > 1e-9) {
        const adir = mul(a, 1 / al);
        const along = dot(tdir, adir);
        const perp = Math.sqrt(Math.max(0, 1 - along * along));
        // >1 stretches along the axis, so the distance to the boundary is
        // smaller along that bearing.
        dAng = theta * Math.sqrt((along / k) ** 2 + perp * perp);
      }
    }
  }
  return (dAng - radius) * R_UNIT;
}

/** Signed distance in metres to the capsule surface. */
export function evaluateCapsule(p: Vec3, cap: CapsulePrimitive, planetRadius: number): number {
  void planetRadius;
  const d = distPointSegment(p, cap.a, cap.b);
  if (cap.roughness <= 0) return d - cap.radius;
  const s = 1 / (cap.radius * 6);
  const n = fbm3(p.x * s, p.y * s, p.z * s, cap.seed, 2);
  return d - cap.radius * (1 + n * cap.roughness);
}

/** Signed distance in metres to the tangent-plane ellipse boundary. */
export function evaluateEllipse(p: Vec3, el: EllipsePrimitive, planetRadius: number): number {
  const n = el.center;
  const [r0, u0] = tangentBasis(n);
  const c = Math.cos(el.rotation);
  const s = Math.sin(el.rotation);
  const right = { x: r0.x * c + u0.x * s, y: r0.y * c + u0.y * s, z: r0.z * c + u0.z * s };
  const up = { x: u0.x * c - r0.x * s, y: u0.y * c - r0.y * s, z: u0.z * c - r0.z * s };
  const pl = Math.sqrt(dot(p, p)) || planetRadius;
  const dir = mul(p, 1 / pl);
  const lx = dot(dir, right) * pl;
  const ly = dot(dir, up) * pl;
  // Normalised radius, then converted back to metres with the local scale.
  const k = Math.sqrt((lx * lx) / (el.semiMajor * el.semiMajor) + (ly * ly) / (el.semiMinor * el.semiMinor));
  const scale = Math.min(el.semiMajor, el.semiMinor);
  return (k - 1) * scale;
}

/** Smooth inside-mask for a field value. 1 deep inside, 0 outside. */
export function smoothMask(d: number, blend: number): number {
  if (blend <= 0) return d <= 0 ? 1 : 0;
  return 1 - smoothstep(-blend, blend, d);
}

/** Smooth max/min for blending two signed distances without creasing. */
export function smoothMin(a: number, b: number, k: number): number {
  if (k <= 0) return Math.min(a, b);
  const h = clamp(0.5 + 0.5 * (b - a) / k, 0, 1);
  return b + (a - b) * h - k * h * (1 - h);
}

export function smoothMax(a: number, b: number, k: number): number {
  return -smoothMin(-a, -b, k);
}
