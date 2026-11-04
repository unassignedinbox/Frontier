/**
 * TerrainField — the analytic field.
 *
 * Two entry points matter, and they are the same function:
 *
 *   densityAt(p)  -> 3D signed distance in metres, negative inside the solid.
 *                     This is what the mesher (Surface Nets) surfaces, and the
 *                     reason there are no heightmap limitations: overhangs,
 *                     caves, arches and cracks are all legal because the mesh is
 *                     extracted from a volume, not from a height function.
 *
 *   heightAt(dir) -> altitude of the *surface* above sea level, in metres.
 *                     Used by the ocean bed, the camera ground clamp and the
 *                     coarse fallback. Derived from the same operators, so it is
 *                     consistent with the mesh by construction.
 *
 * The mesher never samples `heightAt`; it samples `densityAt`. Height exists for
 * queries that genuinely need a function of (lat, lon).
 */

import { type Vec3, clamp, mix, normalize, smoothstep, dot, v3 } from '../math/vec';
import { billow3, fbm3, hash3i, ridged3 } from '../math/noise';
import {
  type CapPrimitive, type CapsulePrimitive, type EllipsePrimitive,
  evaluateCap, evaluateCapsule, evaluateEllipse, smoothMask, smoothMax, smoothMin,
} from '../sdf/primitives';
import type {
  FracturePrimitive, SculptStroke, TerrainOperator, TerrainWorld,
} from './terrainWorld';

export interface SurfaceInfo {
  /** Metres above sea level. */
  altitude: number;
  /** Smooth land mask: 1 inland, 0 at sea. */
  landness: number;
  /** Signed coast distance in metres; positive inland. */
  coast: number;
  /** Index of the topmost stratigraphic band at this altitude, -1 if none. */
  stratum: number;
  /** 0..1 crack mask at the surface (1 = inside a crack). */
  fracture: number;
  /** 0..1 steepness estimate, cheap (from local height difference). */
  slope: number;
}

const LARGE = 1e12;

export class TerrainField {
  world: TerrainWorld;
  private landCaps: CapPrimitive[] = [];
  private waterCaps: CapPrimitive[] = [];
  private landCapsules: CapsulePrimitive[] = [];
  private waterCapsules: CapsulePrimitive[] = [];
  private ridges: CapsulePrimitive[] = [];
  private trenches: CapsulePrimitive[] = [];
  private flats: CapsulePrimitive[] = [];
  private ellipses: EllipsePrimitive[] = [];
  private fractures: FracturePrimitive[] = [];
  private strokes: SculptStroke[] = [];

  readonly R: number;

  /**
   * Noise octave scale, 1 = full detail.
   *
   * Far chunks do not need six octaves of relief: at 400 km per chunk, sub-cell
   * detail is below one pixel and costs more than it shows. Workers set this per
   * job, which is safe because each worker owns its own field and meshes jobs
   * sequentially. Everything that is *visible* in silhouette (coast distance,
   * large relief) keeps full precision.
   */
  octaveScale = 1;

  constructor(world: TerrainWorld) {
    this.world = world;
    this.R = world.planetRadius;
    this.reindex();
  }

  private oct(base: number, scaleWeight: number): number {
    return Math.max(1, Math.round(base * (1 - (1 - this.octaveScale) * scaleWeight)));
  }

  /** Rebuild the typed buckets after an edit. Cheap: tens of operators. */
  reindex(): void {
    const ops = this.world.operators;
    this.landCaps = [];
    this.waterCaps = [];
    this.landCapsules = [];
    this.waterCapsules = [];
    this.ridges = [];
    this.trenches = [];
    this.flats = [];
    this.ellipses = [];
    this.fractures = [];
    for (const op of ops) {
      switch (op.type) {
        case 'cap':
          if (op.kind === 'land') this.landCaps.push(op); else this.waterCaps.push(op);
          break;
        case 'capsule':
          if (op.kind === 'ridge') this.ridges.push(op);
          else if (op.kind === 'trench') this.trenches.push(op);
          else if (op.kind === 'land') this.landCapsules.push(op);
          else if (op.kind === 'water') this.waterCapsules.push(op);
          else if (op.kind === 'flatten') this.flats.push(op);
          break;
        case 'ellipse':
          this.ellipses.push(op);
          break;
        case 'fracture':
          this.fractures.push(op);
          break;
        case 'band':
          break;
      }
    }
    this.strokes = this.world.strokes;
  }

  /* ------------------------------------------------------------ coast / land */

  /**
   * Signed coast distance in metres. Positive inland, negative offshore.
   * Soft-union of landmasses, soft-subtraction of seas and gulfs.
   */
  coastDistance(dir: Vec3): number {
    // SDF convention: negative inside the solid land. The coast distance is the
    // negation of that, so positive means inland — which is what every consumer
    // downstream (beach band, shelf, ridge gating, fracture mask) expects.
    let land = LARGE;
    let found = false;
    for (let i = 0; i < this.landCaps.length; i++) {
      const c = this.landCaps[i];
      const d = evaluateCap(dir, c, this.R);
      // Broad phase: skip far caps without evaluating the noise warp.
      if (d > 3_000_000) continue;
      land = found ? smoothMin(land, d, c.blend) : d;
      found = true;
    }
    for (let i = 0; i < this.landCapsules.length; i++) {
      const c = this.landCapsules[i];
      const d = evaluateCapsule(dir, c, this.R);
      if (d > 3_000_000) continue;
      land = found ? smoothMin(land, d, c.blend) : d;
      found = true;
    }
    if (!found) return -this.world.shelfWidth * 4; // no land at all: deep ocean

    for (let i = 0; i < this.waterCaps.length; i++) {
      const c = this.waterCaps[i];
      const d = evaluateCap(dir, c, this.R);
      if (d > 3_000_000) continue;
      land = smoothMax(land, -d, c.blend);
    }
    for (let i = 0; i < this.waterCapsules.length; i++) {
      const c = this.waterCapsules[i];
      const d = evaluateCapsule(dir, c, this.R);
      if (d > 3_000_000) continue;
      land = smoothMax(land, -d, c.blend);
    }
    return -land;
  }

  /**
   * Conservative radius of the terrain surface at a direction, padded by relief.
   * Used by the camera clamp and the picker so they never need to know whether a
   * given direction is land or sea.
   */
  surfaceRadiusApprox(dir: Vec3): number {
    return this.R + this.heightAt(dir);
  }

  /* ---------------------------------------------------------------- heights */

  /**
   * Surface altitude in metres, from the same operator set the density uses.
   * `coast` may be supplied when a caller already has it.
   */
  heightAt(dir: Vec3, coastIn?: number): number {
    const w = this.world;
    const coast = coastIn ?? this.coastDistance(dir);
    const land = smoothstep(-w.beachWidth * 4000, w.beachWidth * 4000, coast);

    // Bathymetry: shallow shelf near the coast, abyssal plain further out.
    const offshore = Math.max(0, -coast);
    const shelf = smoothstep(0, w.shelfWidth, offshore);
    const seabed = -(60 + (w.oceanDepth - 60) * Math.pow(shelf, 0.7));

    // Continental interior: coastal plain rising to an interior plateau.
    const inland = smoothstep(0, 900_000, Math.max(0, coast));
    const base = w.coastHeight + (w.inlandHeight - w.coastHeight) * inland;

    const r = w.relief;
    const landness = land;
    const sea = 1 - land;

    // Large-scale relief, domain-warped so it does not look like a noise grid.
    let h = mix(seabed, base, landness);

    if (landness > 0.001) {
      const warp = r.warp;
      const f = r.landFrequency / this.R;
      const wx = fbm3(dir.x * f * 6.1 + 11, dir.y * f * 6.1, dir.z * f * 6.1, w.seed + 7, 2) * warp;
      const wy = fbm3(dir.x * f * 6.1, dir.y * f * 6.1 + 21, dir.z * f * 6.1, w.seed + 8, 2) * warp;
      const land1 = fbm3(dir.x * f + wx, dir.y * f + wy, dir.z * f, w.seed + 3, this.oct(5, 1));
      const bills = fbm3(dir.x * f * 2.4, dir.y * f * 2.4, dir.z * f * 2.4, w.seed + 4, this.oct(3, 1));
      h += land1 * r.landAmplitude * landness;
      h += (bills - 0.45) * r.landAmplitude * 0.55 * landness;

      // Mountain belts: ridged noise, gated by continentalness so ranges do not
      // spawn in the sea, and shaped by the operator ridges below.
      const mf = r.mountainFrequency / this.R;
      const ridgeN = ridged3(dir.x * mf, dir.y * mf, dir.z * mf, w.seed + 5, this.oct(4, 1));
      const belt = Math.pow(ridgeN, 1 + (1 - r.ridgeSharpness) * 2);
      h += belt * r.mountainAmplitude * inland * landness;
    }

    if (sea > 0.001) {
      const sf = r.seaFrequency / this.R;
      const n = fbm3(dir.x * sf, dir.y * sf, dir.z * sf, w.seed + 9, this.oct(4, 1));
      h += n * r.seaAmplitude * sea;
    }

    // Explicit operators.
    for (let i = 0; i < this.ridges.length; i++) {
      h += this.ridgeContribution(this.ridges[i], dir, coast);
    }
    for (let i = 0; i < this.trenches.length; i++) {
      const t = this.trenches[i];
      const d = evaluateCapsule(dir, t, this.R);
      if (d > t.radius * 3) continue;
      const m = 1 - smoothstep(-t.radius * 0.2, t.radius * 1.05, d);
      h -= t.height * m;
    }
    for (let i = 0; i < this.ellipses.length; i++) {
      const e = this.ellipses[i];
      const d = evaluateEllipse(dir, e, this.R);
      if (d > Math.abs(e.blend) + 1) continue;
      const m = smoothMask(d, Math.abs(e.blend));
      if (e.kind === 'shelf') {
        // Raise/lower the seabed only.
        h += e.height * m * sea;
      } else {
        h += e.height * m * landness;
      }
    }
    for (let i = 0; i < this.flats.length; i++) {
      const f = this.flats[i];
      const d = evaluateCapsule(dir, f, this.R);
      if (d > f.radius) continue;
      const m = 1 - smoothstep(0, f.radius, d);
      h = mix(h, f.height, m * 0.9);
    }

    // Sculpt dabs, newest last so later strokes win locally.
    if (this.strokes.length) h = this.applyStrokes(dir, h);

    return h;
  }

  private ridgeContribution(r: CapsulePrimitive, dir: Vec3, coast: number): number {
    const d = evaluateCapsule(dir, r, this.R);
    if (d > r.radius * 3.2) return 0;
    // Cross-section: sharp crest, long apron.
    const t = clamp(d / (r.radius * 2.2), 0, 1);
    const cross = Math.pow(1 - t, 1.6);
    if (cross <= 0) return 0;
    // Along-crest modulation so the range is not a smooth tube.
    const s = 1 / (r.radius * 3);
    const n = ridged3(dir.x * s, dir.y * s, dir.z * s, r.seed + 61, 3);
    const landGate = smoothstep(-r.radius, r.radius * 0.5, coast);
    return r.height * cross * (0.45 + n * 1.25) * (0.25 + 0.75 * landGate);
  }

  private applyStrokes(dir: Vec3, h: number): number {
    let out = h;
    for (let i = 0; i < this.strokes.length; i++) {
      const st = this.strokes[i];
      // Cheap rejection via dot product before the trig-free distance test.
      const dotv = dot(dir, st.dir);
      if (dotv < 0.9) {
        const ang = Math.acos(clamp(dotv, -1, 1)) * this.R;
        if (ang > st.radius) continue;
      }
      const dist = Math.acos(clamp(dotv, -1, 1)) * this.R;
      if (dist > st.radius) continue;
      const t = dist / st.radius;
      const hardness = clamp(st.hardness, 0, 1);
      const falloff = 1 - smoothstep(hardness * 0.9, 1, t);
      const k = falloff * st.strength;
      switch (st.mode) {
        case 'raise': out += k * st.radius * 0.22; break;
        case 'lower': out -= k * st.radius * 0.22; break;
        case 'smooth': {
          const f = 1 / (this.R * 0.0004);
          const n = fbm3(dir.x * f, dir.y * f, dir.z * f, st.seed, 3);
          out = mix(out, out * 0.55 + n * 120, k * 0.6);
          break;
        }
        case 'flatten': out = mix(out, st.target, k * 0.85); break;
        case 'plateau': out = mix(out, Math.max(out, st.target), k * 0.9); break;
        case 'canyon': out -= k * st.radius * 0.45; break;
        case 'rough': {
          const f = 1 / (st.radius * 0.08);
          const n = billow3(dir.x * f, dir.y * f, dir.z * f, st.seed, 4);
          out += (n - 0.5) * k * st.radius * 0.3;
          break;
        }
      }
    }
    return out;
  }

  /**
   * Everything about a vertical column that does not depend on altitude.
   *
   * The field is separable in the radial direction: `density(p) = |p| - (R + h)`
   * plus fracture carving. So a volumetric mesher can evaluate the expensive part
   * once per column and then walk altitude cheaply — which is the difference
   * between a 350 ms chunk and a 40 ms chunk.
   */
  columnAt(dir: Vec3): { coast: number; height: number } {
    const coast = this.coastDistance(dir);
    return { coast, height: this.heightAt(dir, coast) };
  }

  /** Density from a precomputed column, avoiding repeat coast/height work. */
  densityAtColumn(p: Vec3, dir: Vec3, coast: number, height: number): number {
    const pl = Math.sqrt(p.x * p.x + p.y * p.y + p.z * p.z) || this.R;
    const surface = this.R + height;
    let d = pl - surface;
    if (this.fractures.length === 0) return d;
    if (!this.nearAnyFracture(dir, pl)) return d;
    const depth = surface - pl;
    for (let i = 0; i < this.fractures.length; i++) {
      const f = this.fractures[i];
      if (depth > f.depth) continue;
      const amount = this.fractureAmount(f, p, dir, height, coast);
      if (amount > 0) d -= amount;
    }
    return d;
  }

  /* ---------------------------------------------------------------- density */

  /**
   * 3D signed distance in metres, negative inside the solid earth.
   * `p` is a planet-space point: dir * (R + altitude).
   */
  densityAt(p: Vec3): number {
    const pl = Math.sqrt(p.x * p.x + p.y * p.y + p.z * p.z) || this.R;
    const dir = { x: p.x / pl, y: p.y / pl, z: p.z / pl };
    const nearFracture = this.nearAnyFracture(dir, pl);

    // A sample far outside every crack band is a plain radial distance; skipping
    // the coast/height evaluation there is what keeps volume meshing affordable.
    if (!nearFracture) {
      const h = this.heightAt(dir);
      return pl - (this.R + h);
    }

    const coast = this.coastDistance(dir);
    const h = this.heightAt(dir, coast);
    let d = pl - (this.R + h);
    const depth = this.R + h - pl;
    for (let i = 0; i < this.fractures.length; i++) {
      const f = this.fractures[i];
      if (depth > f.depth) continue;
      const amount = this.fractureAmount(f, p, dir, h, coast);
      if (amount > 0) d -= amount;
    }
    return d;
  }

  /** Cheap band test: is this point within the fracture depth range? */
  private nearAnyFracture(dir: Vec3, pl: number): boolean {
    if (this.fractures.length === 0) return false;
    const alt = pl - this.R;
    for (let i = 0; i < this.fractures.length; i++) {
      const f = this.fractures[i];
      if (alt < f.minAltitude - f.depth) continue;
      if (f.mask) {
        const ang = Math.acos(clamp(dot(dir, f.mask.center), -1, 1));
        if (ang > f.mask.radius) continue;
      }
      return true;
    }
    return false;
  }

  /** Metres of material removed by one fracture network at this point. */
  private fractureAmount(
    f: FracturePrimitive, p: Vec3, dir: Vec3, h: number, coast: number,
  ): number {
    if (h < f.minAltitude) return 0;
    if (coast < f.minLandDistance) return 0;
    if (f.mask) {
      const ang = Math.acos(clamp(dot(dir, f.mask.center), -1, 1));
      if (ang > f.mask.radius) return 0;
    }
    const cs = f.cellSize;
    const x = p.x / cs, y = p.y / cs, z = p.z / cs;
    let edge: number;
    if (f.pattern === 'radial') {
      // Cracks radiating from the region centre.
      const [d1, d2] = worley2(x * 0.35, z * 0.35, f.seed, true);
      edge = (d2 - d1) * cs;
    } else {
      const [d1, d2] = worley3(x, y, z, f.seed);
      edge = (d2 - d1) * cs;
    }
    if (f.pattern === 'desiccation') {
      // Wide shallow polygons: bias toward the cell interior edges.
      edge *= 0.55;
    }
    if (edge >= f.width) return 0;
    const depthTaper = 1 - smoothstep(f.depth * 0.35, f.depth, this.R + h - Math.sqrt(dot(p, p)));
    const w = f.width * Math.max(0, depthTaper);
    return w > 0 ? Math.max(0, w - edge) * 0.5 : 0;
  }

  /* --------------------------------------------------------------- sampling */

  /**
   * Full surface description for shading and telemetry.
   * Pass `altitudeIn` when the caller already sampled the height — that skips a
   * second full field evaluation per vertex, which is most of the cost.
   */
  surfaceInfo(
    dir: Vec3, altitudeIn?: number, coastIn?: number, slopeIn?: number,
  ): SurfaceInfo {
    const coast = coastIn ?? this.coastDistance(dir);
    const altitude = altitudeIn ?? this.heightAt(dir, coast);
    const landness = smoothstep(-this.world.beachWidth * 4000, this.world.beachWidth * 4000, coast);

    // Slope: callers that already have a height lattice should pass it in, since
    // it is free there. Otherwise take one tangential finite difference.
    let slope = slopeIn;
    if (slope === undefined) {
      const eps = 100 / this.R;
      const up0: Vec3 = Math.abs(dir.y) > 0.99 ? v3(0, 0, 1) : v3(0, 1, 0);
      const right = normalize({ x: dir.y * up0.z - dir.z * up0.y, y: dir.z * up0.x - dir.x * up0.z, z: dir.x * up0.y - dir.y * up0.x });
      const h1 = this.heightAt(normalize({ x: dir.x + right.x * eps, y: dir.y + right.y * eps, z: dir.z + right.z * eps }));
      slope = Math.abs(h1 - altitude) / (eps * this.R);
    }

    let stratum = -1;
    let best = -Infinity;
    for (const op of this.world.operators) {
      if (op.type !== 'band') continue;
      if (op.mask) {
        const ang = Math.acos(clamp(dot(dir, op.mask.center), -1, 1));
        if (ang > op.mask.radius) continue;
      }
      if (altitude >= op.altitude && op.altitude > best) { best = op.altitude; stratum = op.altitude; }
    }

    let fracture = 0;
    if (this.fractures.length) {
      const p = { x: dir.x * (this.R + altitude), y: dir.y * (this.R + altitude), z: dir.z * (this.R + altitude) };
      for (const f of this.fractures) {
        const a = this.fractureAmount(f, p, dir, altitude, coast);
        if (a > 0) fracture = Math.max(fracture, Math.min(1, a * 2));
      }
    }

    return { altitude, landness, coast, stratum, fracture, slope };
  }
}

/* ------------------------------------------------------------- worley / cracks */

/** 3D Worley returning [F1, F2] in cell units. */
export function worley3(x: number, y: number, z: number, seed: number): [number, number] {
  const xi = Math.floor(x), yi = Math.floor(y), zi = Math.floor(z);
  let f1 = Infinity, f2 = Infinity;
  const fx = x - xi, fy = y - yi, fz = z - zi;
  for (let dz = -1; dz <= 1; dz++) {
    for (let dy = -1; dy <= 1; dy++) {
      for (let dx = -1; dx <= 1; dx++) {
        const cx = xi + dx, cy = yi + dy, cz = zi + dz;
        const jx = hash3i(cx, cy, cz, seed);
        const jy = hash3i(cx, cy, cz, seed ^ 0x9e3779b9);
        const jz = hash3i(cx, cy, cz, seed ^ 0x85ebca6b);
        const px = dx + jx - fx;
        const py = dy + jy - fy;
        const pz = dz + jz - fz;
        const d = px * px + py * py + pz * pz;
        if (d < f1) { f2 = f1; f1 = d; } else if (d < f2) { f2 = d; }
      }
    }
  }
  return [Math.sqrt(f1), Math.sqrt(f2)];
}

/** 2D Worley, optionally offset so cracks line up with a radial centre. */
export function worley2(x: number, y: number, seed: number, _radial = false): [number, number] {
  const xi = Math.floor(x), yi = Math.floor(y);
  let f1 = Infinity, f2 = Infinity;
  const fx = x - xi, fy = y - yi;
  for (let dy = -1; dy <= 1; dy++) {
    for (let dx = -1; dx <= 1; dx++) {
      const cx = xi + dx, cy = yi + dy;
      const jx = hash3i(cx, cy, 0, seed);
      const jy = hash3i(cx, cy, 1, seed);
      const px = dx + jx - fx;
      const py = dy + jy - fy;
      const d = px * px + py * py;
      if (d < f1) { f2 = f1; f1 = d; } else if (d < f2) { f2 = d; }
    }
  }
  return [Math.sqrt(f1), Math.sqrt(f2)];
}

/** Type guard helper for the editor's operator list. */
export function isHeightOperator(op: TerrainOperator): boolean {
  if (op.type === 'capsule') return op.kind === 'ridge' || op.kind === 'trench';
  if (op.type === 'ellipse') return true;
  return false;
}
