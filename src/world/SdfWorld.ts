/**
 * Frontier world model.
 *
 * The source of truth is a signed field, not a painted bitmap:
 *
 *   landSdf(x,z)    2D continent/coast SDF with CSG land + gulf brushes
 *   heightAt(x,z)   SDF-derived bathymetry/elevation (the heightmap is a cache/projection)
 *   sdf3D(x,y,z)    3D solid-earth SDF; Marching Cubes extracts its zero isosurface
 *
 * The 3D field is a vertical-distance SDF (`y - height`) capped by an earth
 * bottom and an optional chunk box. That is a valid signed field for terrain
 * without overhangs; it also makes voxel sampling exact because the field is
 * linear in y. Full Euclidean 3D CSG primitives can be layered on later for
 * caves and arches without changing the streaming/meshing contract.
 */

import {
  clamp, distanceToSegment, fbm, geoToLocal, lerp, localToGeo, mixColor,
  polygonSdf, ridgedNoise, smoothMax, smoothMin, smoothstep, valueNoise,
  type RGB, type Vec2,
} from './math';

export type FeatureKind =
  | 'land' | 'water' | 'island' | 'continent' | 'gulf'
  | 'raise' | 'lower' | 'flatten' | 'smooth' | 'ridge' | 'basin'
  | 'plateau' | 'cityPad';

export interface SdfFeature {
  id: string;
  kind: FeatureKind;
  shape?: 'capsule' | 'ellipse';
  x1: number;
  z1: number;
  x2: number;
  z2: number;
  radius: number;
  radiusZ?: number;
  rotation?: number;
  strength: number;
  target?: number;
  feather?: number;
  seed?: number;
  label?: string;
}

export interface WorldDefinition {
  name: string;
  seed: number;
  seaLevel: number;
  oceanDepth: number;
  shelfWidth: number;
  coastWidth: number;
  coastNoise: number;
  landElevation: number;
  planetRadius: number;
  originLon: number;
  originLat: number;
  polygon: Vec2[];
  features: SdfFeature[];
}

export interface BrushSettings {
  radiusKm: number;
  strength: number;
  target: number;
  feather: number;
}

export type SculptTool =
  | 'inspect' | 'continent' | 'gulf' | 'island' | 'raise' | 'lower'
  | 'flatten' | 'smooth' | 'ridge' | 'cityPad' | 'waterDrop' | 'waterPush';

let featureSerial = 0;
function nextId(): string {
  featureSerial += 1;
  return `f${Date.now().toString(36)}-${featureSerial.toString(36)}`;
}

const KM = 1000;

function kmPoints(points: Vec2[]): Vec2[] {
  return points.map(([x, z]) => [x * KM, z * KM] as Vec2);
}

const LAND_FEATURES = new Set<FeatureKind>(['land', 'water', 'island', 'continent', 'gulf']);
const HEIGHT_FEATURES = new Set<FeatureKind>(['raise', 'lower', 'flatten', 'smooth', 'ridge', 'basin', 'plateau', 'cityPad']);

export function createFlatWorld(name = 'Flat plate'): WorldDefinition {
  return {
    name,
    seed: 1337,
    seaLevel: 0,
    oceanDepth: 4500,
    shelfWidth: 180000,
    coastWidth: 22000,
    coastNoise: 90000,
    landElevation: 1100,
    planetRadius: 6_371_000,
    originLon: 20,
    originLat: 0,
    polygon: [],
    features: [],
  };
}

/** A deliberately recognisable Africa-like blockout with gulfs, a channel and highlands. */
export function createAfricaPreset(): WorldDefinition {
  // Local tangent-plane kilometres: +x east, +z south, origin ~20°E / 0°N.
  const outline: Vec2[] = [
    [-2050, -3500], [-1600, -3700], [-1050, -3650], [-350, -3550], [350, -3450],
    [1050, -3300], [1650, -3050], [1950, -2650], [2250, -2400], [2350, -1950],
    [2280, -1500], [2650, -1150], [3050, -720], [2900, -260], [2600, 220],
    [2350, 720], [2100, 1250], [1850, 1900], [1550, 2600], [1150, 3250],
    [620, 3700], [120, 3850], [-200, 3620], [-520, 3100], [-760, 2500],
    [-980, 1850], [-1180, 1200], [-1420, 720], [-1680, 420], [-1830, 120],
    [-1720, -40], [-1450, 120], [-1250, 380], [-1280, 720], [-1530, 980],
    [-1800, 1180], [-2030, 900], [-2150, 420], [-2280, -120], [-2380, -760],
    [-2400, -1400], [-2250, -2050], [-2200, -2750],
  ];

  const features: SdfFeature[] = [
    // Carve the Gulf of Guinea and the north-east seas out of the land SDF.
    { id: 'gulf-guinea', kind: 'gulf', shape: 'capsule', x1: -1750 * KM, z1: 520 * KM, x2: -1050 * KM, z2: 780 * KM, radius: 290 * KM, strength: 0, label: 'Gulf of Guinea' },
    { id: 'red-sea', kind: 'gulf', shape: 'capsule', x1: 1870 * KM, z1: -2700 * KM, x2: 2350 * KM, z2: -1350 * KM, radius: 135 * KM, strength: 0, label: 'Red Sea' },
    { id: 'gulf-aden', kind: 'gulf', shape: 'capsule', x1: 2350 * KM, z1: -1050 * KM, x2: 3150 * KM, z2: -670 * KM, radius: 170 * KM, strength: 0, label: 'Gulf of Aden' },
    { id: 'mozambique', kind: 'gulf', shape: 'capsule', x1: 2190 * KM, z1: 1650 * KM, x2: 2460 * KM, z2: 3000 * KM, radius: 215 * KM, strength: 0, label: 'Mozambique Channel' },

    // Islands are additive SDF unions, not decals.
    { id: 'madagascar', kind: 'island', shape: 'ellipse', x1: 2790 * KM, z1: 2300 * KM, x2: 2790 * KM, z2: 2300 * KM, radius: 220 * KM, radiusZ: 620 * KM, rotation: 0.18, strength: 0, label: 'Madagascar' },
    { id: 'canaries', kind: 'island', shape: 'ellipse', x1: -2050 * KM, z1: -2850 * KM, x2: -2050 * KM, z2: -2850 * KM, radius: 90 * KM, radiusZ: 260 * KM, rotation: -0.3, strength: 0, label: 'Canary ridge' },
    { id: 'cape-verde', kind: 'island', shape: 'ellipse', x1: -2550 * KM, z1: -1450 * KM, x2: -2550 * KM, z2: -1450 * KM, radius: 95 * KM, radiusZ: 150 * KM, strength: 0, label: 'Cape Verde' },

    // Continental relief from SDF-shaped height modifiers.
    { id: 'atlas', kind: 'ridge', shape: 'capsule', x1: -1850 * KM, z1: -3150 * KM, x2: -650 * KM, z2: -2920 * KM, radius: 260 * KM, strength: 1700, label: 'Atlas' },
    { id: 'ahaggar', kind: 'plateau', shape: 'ellipse', x1: -700 * KM, z1: -2150 * KM, x2: -700 * KM, z2: -2150 * KM, radius: 700 * KM, radiusZ: 430 * KM, strength: 900, label: 'Ahaggar' },
    { id: 'ethiopian', kind: 'plateau', shape: 'ellipse', x1: 2250 * KM, z1: -900 * KM, x2: 2250 * KM, z2: -900 * KM, radius: 520 * KM, radiusZ: 430 * KM, strength: 2300, label: 'Ethiopian Highlands' },
    { id: 'east-rift', kind: 'ridge', shape: 'capsule', x1: 2050 * KM, z1: -450 * KM, x2: 1650 * KM, z2: 2200 * KM, radius: 250 * KM, strength: 1500, label: 'East African Rift' },
    { id: 'drakensberg', kind: 'ridge', shape: 'capsule', x1: 1050 * KM, z1: 2750 * KM, x2: 1750 * KM, z2: 3200 * KM, radius: 240 * KM, strength: 1900, label: 'Drakensberg' },
    { id: 'congo', kind: 'basin', shape: 'ellipse', x1: 250 * KM, z1: 450 * KM, x2: 250 * KM, z2: 450 * KM, radius: 950 * KM, radiusZ: 620 * KM, strength: 420, label: 'Congo Basin' },
    { id: 'sahara', kind: 'plateau', shape: 'ellipse', x1: -500 * KM, z1: -1750 * KM, x2: -500 * KM, z2: -1750 * KM, radius: 1750 * KM, radiusZ: 720 * KM, strength: 420, label: 'Sahara plate' },
  ];

  return {
    ...createFlatWorld('Africa + gulfs'),
    seed: 20420,
    coastNoise: 125000,
    landElevation: 1250,
    polygon: kmPoints(outline),
    features,
  };
}

export function createArchipelagoPreset(): WorldDefinition {
  const def = createFlatWorld('Archipelago blockout');
  def.seed = 8821;
  def.coastNoise = 70000;
  for (let i = 0; i < 22; i++) {
    const a = i * 2.399;
    const r = 450 + i * 135;
    const x = Math.cos(a) * r * KM;
    const z = Math.sin(a) * r * KM;
    def.features.push({
      id: `isle-${i}`, kind: 'island', shape: 'ellipse',
      x1: x, z1: z, x2: x, z2: z,
      radius: (70 + valueNoise(i, 3, def.seed) * 190) * KM,
      radiusZ: (90 + valueNoise(i, 9, def.seed) * 260) * KM,
      rotation: a * 0.25, strength: 0, label: `Island ${i + 1}`,
    });
  }
  return def;
}

export function createRandomContinent(seed = Math.floor(Math.random() * 1e6)): WorldDefinition {
  const def = createFlatWorld(`Generated continent ${seed}`);
  def.seed = seed;
  def.coastNoise = 150000;
  const points: Vec2[] = [];
  const count = 38;
  for (let i = 0; i < count; i++) {
    const a = (i / count) * Math.PI * 2;
    const n = fbm(Math.cos(a) * 2 + 4, Math.sin(a) * 2 + 4, seed, 4);
    const r = (1650 + n * 1750) * KM;
    const squeezeZ = 0.78 + valueNoise(i, 71, seed) * 0.38;
    points.push([Math.cos(a) * r * 1.1, Math.sin(a) * r * squeezeZ]);
  }
  def.polygon = points;
  for (let i = 0; i < 4; i++) {
    const a = valueNoise(i, 17, seed) * Math.PI * 2;
    const r = (900 + valueNoise(i, 29, seed) * 1400) * KM;
    const x = Math.cos(a) * r;
    const z = Math.sin(a) * r;
    def.features.push({
      id: `rand-gulf-${i}`, kind: 'gulf', shape: 'capsule',
      x1: x, z1: z, x2: x + Math.cos(a + 1.2) * 700 * KM, z2: z + Math.sin(a + 1.2) * 700 * KM,
      radius: (120 + valueNoise(i, 41, seed) * 180) * KM,
      strength: 0, label: `Inland sea ${i + 1}`,
    });
  }
  return def;
}

function normaliseDefinition(raw: Partial<WorldDefinition>): WorldDefinition {
  const base = createFlatWorld(raw.name || 'Imported world');
  const def: WorldDefinition = { ...base, ...raw } as WorldDefinition;
  def.polygon = Array.isArray(def.polygon) ? def.polygon : [];
  def.features = Array.isArray(def.features) ? def.features : [];
  def.features = def.features.map((f, i) => ({
    ...f,
    id: f.id || `import-${i}`,
    shape: f.shape || 'capsule',
    strength: Number.isFinite(f.strength) ? f.strength : 0,
    radius: Math.max(1, Number.isFinite(f.radius) ? f.radius : 1000),
    feather: Number.isFinite(f.feather) ? f.feather : undefined,
  }));
  return def;
}

export class SdfWorld {
  def: WorldDefinition;
  version = 0;

  constructor(def: WorldDefinition = createAfricaPreset()) {
    this.def = normaliseDefinition(def);
  }

  static fromJSON(json: unknown): SdfWorld {
    return new SdfWorld(normaliseDefinition(json as Partial<WorldDefinition>));
  }

  cloneDefinition(): WorldDefinition {
    return JSON.parse(JSON.stringify(this.def)) as WorldDefinition;
  }

  toJSON(): WorldDefinition {
    return this.cloneDefinition();
  }

  markChanged(): void {
    this.version += 1;
  }

  setParam<K extends keyof WorldDefinition>(key: K, value: WorldDefinition[K]): void {
    this.def[key] = value;
    this.markChanged();
  }

  get seaLevel(): number { return this.def.seaLevel; }

  featureDistance(f: SdfFeature, x: number, z: number): number {
    if (f.shape === 'ellipse') {
      const cx = f.x1;
      const cz = f.z1;
      const rx = Math.max(1, f.radius);
      const rz = Math.max(1, f.radiusZ ?? f.radius);
      const rot = f.rotation ?? 0;
      const dx0 = x - cx;
      const dz0 = z - cz;
      const c = Math.cos(rot), s = Math.sin(rot);
      const dx = (dx0 * c + dz0 * s) / rx;
      const dz = (-dx0 * s + dz0 * c) / rz;
      return (Math.sqrt(dx * dx + dz * dz) - 1) * Math.min(rx, rz);
    }
    return distanceToSegment(x, z, f.x1, f.z1, f.x2, f.z2) - f.radius;
  }

  featureWeight(f: SdfFeature, d: number): number {
    const feather = Math.max(1, f.feather ?? f.radius * 0.32);
    return 1 - smoothstep(0, feather, Math.max(0, d));
  }

  /** Negative inside land, positive in ocean. Includes domain-warped coastline. */
  landSdf(x: number, z: number): number {
    if (this.def.polygon.length < 3 && !this.def.features.some(f => LAND_FEATURES.has(f.kind))) {
      return 1e8;
    }
    const seed = this.def.seed;
    const warpScale = this.def.coastNoise;
    const wx = (fbm(x / 520000, z / 520000, seed + 17, 4) - 0.5) * 2 * warpScale;
    const wz = (fbm(x / 520000, z / 520000, seed + 53, 4) - 0.5) * 2 * warpScale;
    const fine = (fbm(x / 85000, z / 85000, seed + 91, 3) - 0.5) * warpScale * 0.16;
    let d = polygonSdf(this.def.polygon, x + wx, z + wz) + fine;

    for (const f of this.def.features) {
      if (!LAND_FEATURES.has(f.kind)) continue;
      const fd = this.featureDistance(f, x, z);
      const k = Math.max(1000, f.radius * 0.22);
      if (f.kind === 'water' || f.kind === 'gulf') {
        // Subtract the brush from land: inside the gulf must become positive.
        d = smoothMax(d, -fd, k);
      } else {
        // Union land / island / continent stamps.
        d = smoothMin(d, fd, k);
      }
    }
    return d;
  }

  /** Base bathymetry + elevation before editor height strokes. */
  baseHeight(x: number, z: number): number {
    const def = this.def;
    const d = this.landSdf(x, z);
    const offshore = Math.max(0, d);
    const shelf = Math.exp(-offshore / def.shelfWidth);
    const seabedNoise = (fbm(x / 150000, z / 150000, def.seed + 211, 4) - 0.5) * 260 * (0.25 + 0.75 * shelf);
    let h = -def.oceanDepth + (def.oceanDepth - 130) * shelf + seabedNoise;

    if (def.polygon.length < 3 && !def.features.some(f => LAND_FEATURES.has(f.kind))) {
      // A genuinely flat starter plate: a few metres of survey noise only.
      return -80 + (fbm(x / 40000, z / 40000, def.seed + 7, 2) - 0.5) * 3;
    }

    const inland = clamp(-d / def.coastWidth, 0, 1);
    const landBlend = inland * inland * (3 - 2 * inland);
    const hills = fbm(x / 290000, z / 290000, def.seed + 307, 5);
    const ridges = ridgedNoise(x / 140000, z / 140000, def.seed + 401, 4);
    let landH = 35 + def.landElevation * (0.12 + 0.62 * hills + 0.38 * ridges);

    for (const f of def.features) {
      if (f.kind !== 'ridge' && f.kind !== 'basin' && f.kind !== 'plateau') continue;
      const fd = this.featureDistance(f, x, z);
      const w = this.featureWeight(f, fd) * landBlend;
      if (w <= 0) continue;
      if (f.kind === 'ridge') {
        const r = ridgedNoise(x / 90000, z / 90000, (f.seed ?? def.seed) + 13, 4);
        landH += f.strength * w * (0.45 + 0.55 * r);
      } else if (f.kind === 'basin') {
        landH -= f.strength * w;
      } else {
        landH += f.strength * w;
      }
    }
    h = lerp(h, landH, landBlend);
    return h;
  }

  /** The exported heightmap is a projection of the SDF, never the source of truth. */
  heightAt(x: number, z: number): number {
    let h = this.baseHeight(x, z);
    for (const f of this.def.features) {
      if (!HEIGHT_FEATURES.has(f.kind) || f.kind === 'ridge' || f.kind === 'basin' || f.kind === 'plateau') continue;
      const fd = this.featureDistance(f, x, z);
      const w = this.featureWeight(f, fd);
      if (w <= 0) continue;
      switch (f.kind) {
        case 'raise': h += f.strength * w; break;
        case 'lower': h -= f.strength * w; break;
        case 'flatten':
        case 'cityPad': h = lerp(h, f.target ?? 0, w); break;
        case 'smooth': {
          const r = f.radius * 0.75;
          const avg = (
            this.baseHeight(x + r, z) + this.baseHeight(x - r, z) +
            this.baseHeight(x, z + r) + this.baseHeight(x, z - r)
          ) * 0.25;
          h = lerp(h, avg, w * 0.9);
          break;
        }
        default: break;
      }
    }
    return h;
  }

  heightAboveSea(x: number, z: number): number {
    return this.heightAt(x, z) - this.def.seaLevel;
  }

  waterDepthAt(x: number, z: number): number {
    return Math.max(0, this.def.seaLevel - this.heightAt(x, z));
  }

  /** 3D solid earth SDF used by the voxel mesher. Negative = inside earth. */
  sdf3D(x: number, y: number, z: number, bounds?: { cx: number; cz: number; halfX: number; halfZ: number }): number {
    const h = this.heightAt(x, z);
    let d = Math.max(y - h, -6500 - y);
    if (bounds) {
      const box = Math.max(
        Math.abs(x - bounds.cx) - bounds.halfX,
        Math.abs(z - bounds.cz) - bounds.halfZ,
      );
      d = Math.max(d, box);
    }
    return d;
  }

  sdfGradient(x: number, y: number, z: number, e: number, bounds?: { cx: number; cz: number; halfX: number; halfZ: number }): [number, number, number] {
    const gx = this.sdf3D(x + e, y, z, bounds) - this.sdf3D(x - e, y, z, bounds);
    const gy = this.sdf3D(x, y + e, z, bounds) - this.sdf3D(x, y - e, z, bounds);
    const gz = this.sdf3D(x, y, z + e, bounds) - this.sdf3D(x, y, z - e, bounds);
    const len = Math.hypot(gx, gy, gz) || 1;
    return [gx / len, gy / len, gz / len];
  }

  normalAt(x: number, z: number, e = 30): [number, number, number] {
    const dhdx = (this.heightAt(x + e, z) - this.heightAt(x - e, z)) / (2 * e);
    const dhdz = (this.heightAt(x, z + e) - this.heightAt(x, z - e)) / (2 * e);
    const len = Math.hypot(dhdx, 1, dhdz);
    return [-dhdx / len, 1 / len, -dhdz / len];
  }

  colorAt(x: number, z: number, h: number, landD: number, lodTint?: RGB): RGB {
    const hs = h - this.def.seaLevel;
    const variation = (fbm(x / 42000, z / 42000, this.def.seed + 811, 3) - 0.5) * 0.09;
    let c: RGB;
    const deep: RGB = [0.018, 0.055, 0.105];
    const mid: RGB = [0.025, 0.13, 0.22];
    const shelf: RGB = [0.06, 0.30, 0.38];
    const sand: RGB = [0.66, 0.59, 0.39];
    const grass: RGB = [0.20, 0.36, 0.16];
    const forest: RGB = [0.11, 0.27, 0.13];
    const rock: RGB = [0.36, 0.34, 0.31];
    const snow: RGB = [0.88, 0.91, 0.94];

    if (hs < -2200) c = deep;
    else if (hs < -450) c = mixColor(deep, mid, smoothstep(-2200, -450, hs));
    else if (hs < -35) c = mixColor(mid, shelf, smoothstep(-450, -35, hs));
    else if (hs < 12) c = mixColor(shelf, sand, smoothstep(-35, 12, hs));
    else if (hs < 220) c = mixColor(sand, grass, smoothstep(12, 220, hs));
    else if (hs < 900) c = mixColor(grass, forest, smoothstep(220, 900, hs));
    else if (hs < 1900) c = mixColor(forest, rock, smoothstep(900, 1900, hs));
    else c = mixColor(rock, snow, smoothstep(1900, 3200, hs));

    // Slight coastal bleaching and deterministic hue variation.
    const coast = 1 - smoothstep(0, 90000, Math.abs(landD));
    c = mixColor(c, sand, coast * 0.13);
    c = [clamp(c[0] + variation, 0, 1), clamp(c[1] + variation, 0, 1), clamp(c[2] + variation, 0, 1)];
    if (lodTint) c = [c[0] * lodTint[0], c[1] * lodTint[1], c[2] * lodTint[2]];
    return c;
  }

  mapColorAt(lon: number, lat: number): RGB {
    const [x, z] = geoToLocal(lon, lat, this.def.originLon, this.def.originLat);
    const d = this.landSdf(x, z);
    const h = this.heightAt(x, z);
    const c = this.colorAt(x, z, h, d);
    const hs = h - this.def.seaLevel;
    if (hs < -250) {
      const t = smoothstep(-4500, -250, hs);
      return mixColor([0.012, 0.035, 0.085], c, t);
    }
    return c;
  }

  localToGeo(x: number, z: number): { lon: number; lat: number } {
    return localToGeo(x, z, this.def.originLon, this.def.originLat);
  }

  geoToLocal(lon: number, lat: number): Vec2 {
    return geoToLocal(lon, lat, this.def.originLon, this.def.originLat);
  }

  makeStrokeFeature(tool: SculptTool, x: number, z: number, settings: BrushSettings): SdfFeature | null {
    const radius = Math.max(20, settings.radiusKm * KM);
    const feather = Math.max(20, radius * settings.feather);
    const base: Omit<SdfFeature, 'kind'> = {
      id: nextId(), shape: 'capsule', x1: x, z1: z, x2: x, z2: z,
      radius, feather, strength: settings.strength, target: settings.target,
      label: tool,
    };
    switch (tool) {
      case 'continent': return { ...base, kind: 'continent', strength: 0 };
      case 'gulf': return { ...base, kind: 'gulf', strength: 0 };
      case 'island': return { ...base, kind: 'island', strength: 0 };
      case 'raise': return { ...base, kind: 'raise' };
      case 'lower': return { ...base, kind: 'lower' };
      case 'flatten': return { ...base, kind: 'flatten' };
      case 'smooth': return { ...base, kind: 'smooth', strength: 0 };
      case 'ridge': return { ...base, kind: 'ridge', strength: settings.strength };
      case 'cityPad': return { ...base, kind: 'cityPad' };
      default: return null;
    }
  }

  beginStroke(tool: SculptTool, x: number, z: number, settings: BrushSettings): string | null {
    const f = this.makeStrokeFeature(tool, x, z, settings);
    if (!f) return null;
    this.def.features.push(f);
    this.markChanged();
    return f.id;
  }

  updateStroke(id: string, x: number, z: number, settings?: BrushSettings, markChanged = true): void {
    const f = this.def.features.find(item => item.id === id);
    if (!f) return;
    f.x2 = x;
    f.z2 = z;
    if (settings) {
      f.radius = Math.max(20, settings.radiusKm * KM);
      f.feather = Math.max(20, f.radius * settings.feather);
      f.strength = settings.strength;
      f.target = settings.target;
    }
    // During a drag the app batches version changes so the solver is not
    // re-bathymetrised on every pointer sample.
    if (markChanged) this.markChanged();
  }

  removeFeature(id: string): void {
    const i = this.def.features.findIndex(f => f.id === id);
    if (i >= 0) {
      this.def.features.splice(i, 1);
      this.markChanged();
    }
  }

  undo(): void {
    if (this.def.features.length) {
      this.def.features.pop();
      this.markChanged();
    }
  }

  clearFeatures(): void {
    this.def.features = [];
    this.markChanged();
  }

  get featureCount(): number { return this.def.features.length; }
}
