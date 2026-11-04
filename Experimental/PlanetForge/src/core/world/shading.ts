/**
 * Procedural surface shading derived from the SDF, not from a texture atlas.
 *
 * Every terrain vertex carries a tint computed from the *same* field the mesh was
 * extracted from: coast distance, altitude, slope, strata index and fracture
 * mask. That means colours are consistent across chunk borders (they are a pure
 * function of position) and they survive LOD changes without seams.
 *
 * This is deliberately not a biome system yet — it is the material *signal* set
 * that the later erosion/strata work will consume.
 */

import { type Vec3, clamp, mix, smoothstep } from '../math/vec';
import { fbm3 } from '../math/noise';
import type { TerrainWorld } from './terrainWorld';
import type { SurfaceInfo } from './terrainField';

export interface Tint {
  r: number;
  g: number;
  b: number;
}

const BEACH: Tint = { r: 0.80, g: 0.72, b: 0.53 };
const GRASS: Tint = { r: 0.30, g: 0.42, b: 0.22 };
const DRY_GRASS: Tint = { r: 0.55, g: 0.53, b: 0.30 };
const FOREST: Tint = { r: 0.17, g: 0.30, b: 0.16 };
const ROCK: Tint = { r: 0.40, g: 0.38, b: 0.35 };
const DARK_ROCK: Tint = { r: 0.24, g: 0.23, b: 0.22 };
const SNOW: Tint = { r: 0.92, g: 0.94, b: 0.97 };
const SHALLOW: Tint = { r: 0.34, g: 0.60, b: 0.58 };
const DEEP: Tint = { r: 0.09, g: 0.20, b: 0.28 };
const SILT: Tint = { r: 0.36, g: 0.33, b: 0.26 };

function mixTint(a: Tint, b: Tint, t: number): Tint {
  return { r: mix(a.r, b.r, t), g: mix(a.g, b.g, t), b: mix(a.b, b.b, t) };
}

/**
 * Shade a surface point.
 *
 * @param dir   unit direction on the planet
 * @param info  surface description from TerrainField.surfaceInfo()
 * @param slope vertical metres per horizontal metre
 */
export function shadeSurface(
  world: TerrainWorld, dir: Vec3, info: SurfaceInfo, slope: number,
): Tint {
  const { altitude, coast } = info;
  const absLat = Math.abs(Math.asin(clamp(dir.y, -1, 1))) * (180 / Math.PI);

  // ---- underwater -----------------------------------------------------------
  if (altitude < 0) {
    const depth = -altitude;
    // Shallow shelves are bright, abyssal plains are nearly black-blue.
    let t = smoothstep(0, 1400, depth);
    let c = mixTint(SHALLOW, DEEP, t);
    // Silt fans near the coast.
    const silt = smoothstep(120_000, 0, Math.abs(coast)) * 0.45;
    c = mixTint(c, SILT, silt);
    // Abyssal-hill mottling so the seabed is not a flat gradient.
    const n = fbm3(dir.x * 900, dir.y * 900, dir.z * 900, world.seed + 41, 3) * 0.5 + 0.5;
    t = n;
    return mixTint(c, { r: c.r * 0.85, g: c.g * 0.85, b: c.b * 0.9 }, t * 0.5);
  }

  // ---- beach ---------------------------------------------------------------
  const beachBand = 4 + 14 * world.beachWidth;
  const beachT = 1 - smoothstep(0, beachBand, altitude);

  // ---- base land cover by climate proxy ------------------------------------
  // Latitude drives a very coarse aridity/forest model; the erosion work later
  // will replace this with real moisture.
  const equator = 1 - Math.abs(absLat / 90);
  const arid = smoothstep(14, 30, Math.abs(absLat)) * (1 - smoothstep(46, 62, Math.abs(absLat)));
  const wet = clamp(equator * 0.85 + (1 - arid) * 0.35, 0, 1);

  let base = mixTint(GRASS, FOREST, wet);
  base = mixTint(base, DRY_GRASS, arid * 0.8);
  base = mixTint(base, BEACH, beachT);

  // ---- rock on steep terrain ----------------------------------------------
  const rockT = smoothstep(0.55, 1.9, slope);
  let rock = mixTint(ROCK, DARK_ROCK, smoothstep(1200, 3400, altitude));
  rock = mixTint(rock, SILT, 0.12);
  let c = mixTint(base, rock, rockT * 0.92);

  // ---- strata bands --------------------------------------------------------
  // Bands are datums from the world document. Where they outcrop on a slope, we
  // bias the tint — this is what will later become real sediment layers.
  let bandT = 0;
  for (const op of world.operators) {
    if (op.type !== 'band') continue;
    const dd = Math.abs(altitude - op.altitude);
    const band = 1 - smoothstep(op.thickness * 0.35, op.thickness, dd);
    if (band <= 0) continue;
    const hue = fract(op.seed * 0.61803398875);
    const bandTint: Tint = {
      r: 0.36 + hue * 0.28,
      g: 0.32 + ((hue * 7) % 1) * 0.22,
      b: 0.26 + ((hue * 13) % 1) * 0.20,
    };
    c = mixTint(c, bandTint, band * 0.55 * (0.35 + rockT * 0.65));
    bandT = Math.max(bandT, band);
  }
  void bandT;

  // ---- snow ---------------------------------------------------------------
  const snowLine = 3800 - absLat * 26; // colder toward the poles
  const snow = smoothstep(snowLine, snowLine + 900, altitude)
    * (1 - smoothstep(0.85, 2.2, slope));
  c = mixTint(c, SNOW, clamp(snow, 0, 1));

  // ---- cracks darken ------------------------------------------------------
  if (info.fracture > 0) {
    c = mixTint(c, DARK_ROCK, clamp(info.fracture * 0.9, 0, 1));
    c = { r: c.r * 0.55, g: c.g * 0.55, b: c.b * 0.55 };
  }

  // ---- fine albedo variation ----------------------------------------------
  const grain = fbm3(dir.x * 4200, dir.y * 4200, dir.z * 4200, world.seed + 77, 2) * 0.5 + 0.5;
  const k = 0.86 + grain * 0.28;
  return { r: clamp(c.r * k, 0, 1), g: clamp(c.g * k, 0, 1), b: clamp(c.b * k, 0, 1) };
}

const fract = (v: number): number => v - Math.floor(v);

/** Blend a vertex tint toward the analytic value at a chunk border. */
export function seamBlend(local: Tint, analytic: Tint, t: number): Tint {
  return mixTint(local, analytic, clamp(t, 0, 1));
}
