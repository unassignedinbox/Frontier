/**
 * The authored world document.
 *
 * This is the single source of truth that gets saved, diffed, sent to workers
 * and eventually converted to a mesh. It is deliberately a *list of parametric
 * SDF operators* rather than a voxel or height grid:
 *
 *   - it is resolution independent, so the same document produces a 15 km near
 *     chunk and a 2000 km background chunk;
 *   - an edit touches only the operators it changes, so streaming only needs to
 *     re-mesh chunks whose bounds overlap that operator;
 *   - it is tiny on disk (kilobytes, not gigabytes), which matters on a machine
 *     with an HDD: the cache stores generated *meshes*, never the field.
 *
 * Coordinate contract: every primitive stores unit *directions* on the planet
 * sphere (planet-centred, Y = polar axis) plus sizes in metres. That makes the
 * document independent of planet radius and seam-free across cube faces.
 */

import { type Vec3, normalize, v3, cross, dist } from '../math/vec';
import { latLonToDir } from '../sdf/cubesphere';
import { seedFor } from '../math/noise';
import type {
  BandPrimitive, CapPrimitive, CapsulePrimitive, EllipsePrimitive, Primitive,
} from '../sdf/primitives';

export type FracturePattern = 'voronoi' | 'radial' | 'desiccation';

/**
 * Crack / fracture network.
 *
 * This answers "can cracks be SDF instead of post-process": yes. A crack network
 * is the zero set of `F2 - F1` in a 3D Worley/Voronoi partition, eroded inward by
 * a width that tapers with depth. Subtracting that from the terrain field gives
 * real geometry with real walls, which a heightmap cannot express because a crack
 * is not single-valued in height.
 */
export interface FracturePrimitive {
  id: string;
  type: 'fracture';
  pattern: FracturePattern;
  /** Cell size in metres — sets crack spacing. */
  cellSize: number;
  /** Maximum crack width in metres at the surface. */
  width: number;
  /** Depth in metres below which cracks close up. */
  depth: number;
  /** Only fracture where altitude is above this (metres). */
  minAltitude: number;
  /** Keep cracks off the coast: require this much land distance (metres). */
  minLandDistance: number;
  /** Region restriction, optional. */
  mask?: { center: Vec3; radius: number };
  seed: number;
}

export type TerrainOperator = Primitive | FracturePrimitive;

export type BrushMode =
  | 'raise' | 'lower' | 'smooth' | 'flatten' | 'plateau' | 'canyon' | 'rough';

/** One sculpt dab. Dabs are kept, not baked, so the field stays analytic. */
export interface SculptStroke {
  id: string;
  mode: BrushMode;
  /** Surface direction at the dab centre. */
  dir: Vec3;
  /** Radius in metres. */
  radius: number;
  /** 0..1 strength. */
  strength: number;
  /** Target altitude for flatten/plateau, metres. */
  target: number;
  /** Falloff hardness: 0 = very soft, 1 = hard edge. */
  hardness: number;
  seed: number;
}

/** Noise-driven relief that is not painted by hand. */
export interface ReliefSpec {
  /** Hills/plains roughness on land, metres. */
  landAmplitude: number;
  landFrequency: number;
  /** Abyssal hills + seamounts, metres. */
  seaAmplitude: number;
  seaFrequency: number;
  /** Mountain-belt noise added on continental interiors, metres. */
  mountainAmplitude: number;
  mountainFrequency: number;
  /** Ridged-noise crest sharpening, 0..1. */
  ridgeSharpness: number;
  /** Domain-warp strength for the relief field, in radians. */
  warp: number;
}

export interface TerrainWorld {
  version: number;
  name: string;
  seed: number;
  /** Planet radius in metres. The sea level surface is exactly this radius. */
  planetRadius: number;
  /** Shelf width: how far the continental shelf extends, metres. */
  shelfWidth: number;
  /** Abyssal depth below sea level, metres. */
  oceanDepth: number;
  /** Typical coastal plain height, metres. */
  coastHeight: number;
  /** Interior plateau height, metres. */
  inlandHeight: number;
  /** 0..1: how much of the coastline is softened into beaches. */
  beachWidth: number;
  relief: ReliefSpec;
  operators: TerrainOperator[];
  strokes: SculptStroke[];
}

export const WORLD_VERSION = 1;

export const DEFAULT_RELIEF: ReliefSpec = {
  landAmplitude: 190,
  landFrequency: 14,
  seaAmplitude: 260,
  seaFrequency: 9,
  mountainAmplitude: 2600,
  mountainFrequency: 5.5,
  ridgeSharpness: 0.65,
  warp: 0.35,
};

export function createEmptyWorld(name = 'Untitled world'): TerrainWorld {
  return {
    version: WORLD_VERSION,
    name,
    seed: 20260101,
    planetRadius: 6_371_000,
    shelfWidth: 190_000,
    oceanDepth: 4300,
    coastHeight: 120,
    inlandHeight: 780,
    beachWidth: 0.5,
    relief: { ...DEFAULT_RELIEF },
    operators: [],
    strokes: [],
  };
}

/* --------------------------------------------------------------- constructors */

export function cap(
  id: string, kind: 'land' | 'water', lat: number, lon: number,
  radiusMetres: number, planetRadius: number,
  opts: Partial<Omit<CapPrimitive, 'type' | 'id' | 'kind' | 'center' | 'radius' | 'stretchDir'>> & {
    stretchDirLat?: number; stretchDirLon?: number;
  } = {},
): CapPrimitive {
  const stretchDir = opts.stretchDirLat !== undefined && opts.stretchDirLon !== undefined
    ? latLonToDir(opts.stretchDirLat, opts.stretchDirLon)
    : { x: 0, y: 1, z: 0 };
  return {
    type: 'cap',
    id,
    kind,
    center: latLonToDir(lat, lon),
    radius: radiusMetres / planetRadius,
    roughness: opts.roughness ?? 0.055,
    frequency: opts.frequency ?? 3.1,
    stretch: opts.stretch ?? 1,
    stretchDir,
    blend: opts.blend ?? 45_000,
    seed: opts.seed ?? seedFor(id),
  };
}

export function capsule(
  id: string, kind: CapsulePrimitive['kind'],
  latA: number, lonA: number, latB: number, lonB: number,
  radiusMetres: number, height: number,
  opts: Partial<Pick<CapsulePrimitive, 'roughness' | 'blend' | 'seed'>> = {},
): CapsulePrimitive {
  return {
    type: 'capsule',
    id,
    kind,
    a: latLonToDir(latA, lonA),
    b: latLonToDir(latB, lonB),
    radius: radiusMetres,
    height,
    roughness: opts.roughness ?? 0.22,
    blend: opts.blend ?? 30_000,
    seed: opts.seed ?? seedFor(id),
  };
}

export function ellipse(
  id: string, kind: EllipsePrimitive['kind'],
  lat: number, lon: number,
  semiMajor: number, semiMinor: number, rotation: number, height: number,
  opts: Partial<Pick<EllipsePrimitive, 'blend' | 'seed'>> = {},
): EllipsePrimitive {
  return {
    type: 'ellipse',
    id,
    kind,
    center: latLonToDir(lat, lon),
    semiMajor,
    semiMinor,
    rotation,
    height,
    blend: opts.blend ?? 60_000,
    seed: opts.seed ?? seedFor(id),
  };
}

export function band(
  id: string, altitude: number, thickness: number,
  opts: Partial<Omit<BandPrimitive, 'type' | 'id' | 'altitude' | 'thickness'>> = {},
): BandPrimitive {
  return {
    type: 'band',
    id,
    altitude,
    thickness,
    blend: opts.blend ?? 0,
    tilt: opts.tilt ?? 0,
    mask: opts.mask,
    seed: opts.seed ?? seedFor(id),
  };
}

/* ------------------------------------------------------------------- presets */

/** Two drawn-out continents, an archipelago, a gulf and two mountain belts. */
export function createContinentsPreset(): TerrainWorld {
  const w = createEmptyWorld('Terra Nova');
  const R = w.planetRadius;

  w.operators.push(
    // Northern continent — long, stretched north-south along the 20° meridian.
    cap('continent-north', 'land', 34, 18, 2_050_000, R, {
      roughness: 0.075, frequency: 3.4, stretch: 0.62,
      stretchDirLat: 0, stretchDirLon: 90, blend: 60_000,
    }),
    // Southern continent, offset so the two do not mirror each other.
    cap('continent-south', 'land', -27, 44, 1_750_000, R, {
      roughness: 0.085, frequency: 2.9, stretch: 0.74,
      stretchDirLat: 30, stretchDirLon: 130, blend: 60_000,
    }),
    // A third, smaller eastern landmass.
    cap('continent-east', 'land', 4, 108, 1_250_000, R, {
      roughness: 0.10, frequency: 3.8, stretch: 1.35,
      stretchDirLat: 0, stretchDirLon: 20, blend: 50_000,
    }),
    // Inland sea carved out of the northern continent.
    cap('sea-inland', 'water', 40, 26, 520_000, R, {
      roughness: 0.075, frequency: 5.5, stretch: 1.4,
      stretchDirLat: 20, stretchDirLon: 70, blend: 22_000,
    }),
    // Ocean gulf biting in from the west.
    cap('gulf-west', 'water', 8, -22, 780_000, R, {
      roughness: 0.06, frequency: 3.0, stretch: 1.7,
      stretchDirLat: 70, stretchDirLon: 0, blend: 40_000,
    }),
    // Archipelago: a chain of small islands along an arc.
    ...makeArchipelago('arc', 20, 138, 21, 115_000, 7, R),
    // Mountain belts.
    capsule('range-north', 'ridge', 22, 4, 46, 34, 190_000, 2500, { roughness: 0.28 }),
    capsule('range-south', 'ridge', -40, 32, -14, 58, 210_000, 2050, { roughness: 0.3 }),
    capsule('rift-east', 'trench', 10, 96, -18, 104, 70_000, 900, { roughness: 0.2 }),
    // Interior plateau and one deep basin.
    ellipse('plateau-central', 'plateau', 30, 12, 620_000, 420_000, 0.35, 760),
    ellipse('basin-south', 'basin', -33, 46, 480_000, 320_000, -0.2, 420),
  );
  w.relief.mountainAmplitude = 2400;
  w.operators.push(
    band('strata-flood', -60, 40),
    band('strata-shelf', -900, 120),
    band('strata-terrace', 480, 60, { tilt: 0.12 }),
  );
  return w;
}

/** Many small islands and shallow seas — stresses the streaming and the coast shading. */
export function createArchipelagoPreset(): TerrainWorld {
  const w = createEmptyWorld('Thousand Isles');
  const R = w.planetRadius;
  w.seed = 99127;
  w.oceanDepth = 3600;
  w.shelfWidth = 120_000;
  w.relief.landAmplitude = 120;
  w.relief.seaAmplitude = 340;
  w.operators.push(
    ...makeArchipelago('main', -6, 20, 34, 130_000, 16, R, 1.9),
    cap('atoll-north', 'land', 44, 40, 620_000, R, { roughness: 0.12, frequency: 4.6 }),
    capsule('ridge-volcanic', 'ridge', -20, 8, 16, 46, 150_000, 1300, { roughness: 0.35 }),
  );
  return w;
}

/** A single supercontinent with a long rift valley and a wide gulf. */
export function createPangeaPreset(): TerrainWorld {
  const w = createEmptyWorld('Pangea');
  const R = w.planetRadius;
  w.seed = 5521;
  w.oceanDepth = 4800;
  w.operators.push(
    cap('pangea-core', 'land', 2, 14, 3_100_000, R, {
      roughness: 0.065, frequency: 2.4, stretch: 0.85,
      stretchDirLat: 0, stretchDirLon: 0, blend: 80_000,
    }),
    cap('gulf-south', 'water', -24, 24, 900_000, R, { roughness: 0.05, frequency: 3.2, blend: 30_000 }),
    cap('sea-north', 'water', 30, 8, 620_000, R, { roughness: 0.07, frequency: 4.2, blend: 26_000 }),
    capsule('rift-main', 'trench', 28, 2, -20, 22, 110_000, 1400, { roughness: 0.18 }),
    capsule('range-west', 'ridge', -6, -20, 24, -6, 260_000, 3200, { roughness: 0.25 }),
    ellipse('plateau-interior', 'plateau', 4, 18, 900_000, 640_000, 0.1, 900),
  );
  return w;
}

export type PresetName = 'continents' | 'archipelago' | 'pangea';

export function createPreset(name: PresetName): TerrainWorld {
  switch (name) {
    case 'archipelago': return createArchipelagoPreset();
    case 'pangea': return createPangeaPreset();
    default: return createContinentsPreset();
  }
}

function makeArchipelago(
  prefix: string, lat: number, lon: number, count: number,
  spacingMetres: number, radiusMetres: number, planetRadius: number, arc = 1,
): CapPrimitive[] {
  const out: CapPrimitive[] = [];
  const degPerRad = 180 / Math.PI;
  const dLat = (spacingMetres / planetRadius) * degPerRad;
  for (let i = 0; i < count; i++) {
    const t = (i / Math.max(1, count - 1)) - 0.5;
    const wob = Math.sin(i * 2.1) * 0.8;
    const la = lat + t * dLat * Math.cos(wob) + Math.sin(i * 0.9) * 2.2;
    const lo = lon + t * dLat * Math.sin(wob + arc) + Math.cos(i * 1.3) * 3.1;
    const r = radiusMetres * (0.55 + 0.75 * Math.abs(Math.sin(i * 1.7 + 0.4)));
    out.push(cap(`${prefix}-isle-${i}`, 'land', la, lo, r, planetRadius, {
      roughness: 0.11, frequency: 4.4 + (i % 3) * 0.7, blend: 18_000,
    }));
  }
  return out;
}

/* ------------------------------------------------------------- serialisation */

export function cloneWorld(w: TerrainWorld): TerrainWorld {
  return structuredClone(w);
}

/** Deterministic 64-bit-ish hash over the document, used as a cache key. */
export function worldHash(w: TerrainWorld): string {
  const s = JSON.stringify(w);
  let h1 = 0x811c9dc5;
  let h2 = 0x1000193;
  for (let i = 0; i < s.length; i++) {
    h1 = Math.imul(h1 ^ s.charCodeAt(i), 0x01000193) | 0;
    h2 = (h2 + Math.imul(s.charCodeAt(i) + i, 0x27d4eb2d)) | 0;
  }
  return (h1 >>> 0).toString(36) + (h2 >>> 0).toString(36);
}

/* ------------------------------------------------------------------ utilities */

export function operatorCenter(op: TerrainOperator): Vec3 {
  switch (op.type) {
    case 'cap': case 'ellipse': return op.center;
    case 'capsule': return normalize({ x: op.a.x + op.b.x, y: op.a.y + op.b.y, z: op.a.z + op.b.z });
    case 'fracture': return op.mask ? op.mask.center : v3(0, 1, 0);
    case 'band': return op.mask ? op.mask.center : v3(0, 1, 0);
  }
}

/** Conservative influence radius in metres for broad-phase culling. */
export function operatorRadiusMetres(op: TerrainOperator, planetRadius: number): number {
  switch (op.type) {
    case 'cap': return op.radius * planetRadius * (1 + op.roughness) + Math.abs(op.blend) + 4000;
    case 'capsule': return dist(op.a, op.b) * planetRadius * 0.5 * 0.55 + op.radius + Math.abs(op.blend) + op.radius * op.roughness + 4000;
    case 'ellipse': return Math.max(op.semiMajor, op.semiMinor) + Math.abs(op.blend) + 4000;
    case 'band': return op.mask ? op.mask.radius * planetRadius + 8000 : Math.PI * planetRadius;
    case 'fracture': return op.mask ? op.mask.radius * planetRadius : op.cellSize * 4;
  }
}

export function distanceDirToDir(a: Vec3, b: Vec3, planetRadius: number): number {
  return dist(a, b) * planetRadius;
}

export function makeId(prefix: string): string {
  return `${prefix}-${Math.random().toString(36).slice(2, 8)}`;
}

export { cross };
