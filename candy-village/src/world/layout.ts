/**
 * Procedural world layout.
 *
 * One integer seed determines the terrain, the meandering chocolate river, the
 * road and bridge, the village houses, the plaza props, the candy forest and
 * the guard spawn points. The layout is pure data (no three.js), which keeps
 * it testable and lets the navigation grid be built from the same source of
 * truth as the rendered meshes.
 */
import { RNG } from '../core/rng';
import { Simplex3 } from '../core/noise';
import { clamp, dist2, distToSegment2, smoothstep } from '../core/math';

export const WORLD_HALF = 110;
export const PLAZA_R = 9;
export const RIVER_Y = -0.4;

export type BuildingType = 'cottage' | 'fudge' | 'shop';
export type ForestKind = 'lollipop' | 'cane' | 'pine' | 'gumdrop' | 'boulder' | 'reed';

export interface Vec2Like {
  x: number;
  z: number;
}

export interface HouseSpec {
  kind: BuildingType;
  x: number;
  z: number;
  rotY: number;
  w: number;
  d: number;
  h: number;
  seed: number;
}

export interface ForestItem {
  kind: ForestKind;
  x: number;
  y: number;
  z: number;
  rotY: number;
  scale: number;
  variant: number;
  tiltX?: number;
  tiltZ?: number;
}

export interface BridgeSpec {
  a: Vec2Like;
  b: Vec2Like;
  width: number;
  deckY: number;
}

export type Obstacle =
  | { type: 'circle'; x: number; z: number; r: number }
  | { type: 'rect'; x: number; z: number; hw: number; hd: number; yaw: number };

export interface WorldLayout {
  seed: number;
  half: number;
  heightAt(x: number, z: number): number;
  riverCenter(x: number): number;
  riverHalfWidth(x: number): number;
  /** Signed-ish test: true when (x, z) lies in the water channel (plus margin). */
  inRiver(x: number, z: number, margin?: number): boolean;
  /** Vertex tint for the terrain mesh (linear RGB). */
  tintAt(x: number, z: number): [number, number, number];
  houses: HouseSpec[];
  gingerbread: { x: number; z: number; rotY: number; seed: number };
  road: Vec2Like[];
  bridges: BridgeSpec[];
  lamps: Vec2Like[];
  fountain: Vec2Like;
  guardSpawns: Vec2Like[];
  forest: ForestItem[];
  obstacles: Obstacle[];
  /** Walkable exclusion areas where grid cells should be re-opened (bridge decks). */
  walkOverrides: { x: number; z: number; hw: number; hd: number; yaw: number }[];
}

/** Village landmarks that are not randomised beyond the seed-driven jitter. */
const GH_BASE = { x: -44, z: -28 };

export function generateLayout(seed: number): WorldLayout {
  const rng = new RNG(seed);
  const noise = new Simplex3(seed ^ 0x9e3779b9);
  const gh = {
    x: GH_BASE.x + rng.range(-3, 3),
    z: GH_BASE.z + rng.range(-2, 2),
    rotY: 0,
    seed: rng.int(1, 1e6),
  };
  gh.rotY = Math.atan2(-gh.x, -gh.z); // front faces the plaza

  /* ----------------------------- Terrain ------------------------------- */
  const riverCenter = (x: number) => 8 * Math.sin(x * 0.028) + 3 * Math.sin(x * 0.07 + 1.3) - 8;
  const riverHalfWidth = (x: number) => 3.3 + 0.3 * Math.sin(x * 0.05 + 0.7);
  const inRiver = (x: number, z: number, margin = 0) => Math.abs(z - riverCenter(x)) < riverHalfWidth(x) + margin;

  const heightAt = (x: number, z: number): number => {
    const r = Math.hypot(x, z);
    let h = 5.5 * noise.fbm(x * 0.012, 0.5, z * 0.012, 4) * smoothstep(45, 65, r);
    h += 0.12 * noise.noise(x * 0.25, z * 0.25, 1.7);
    const dg = Math.hypot(x - gh.x, z - gh.z);
    h = h + (2.2 - h) * (1 - smoothstep(7, 13, dg));
    const dz = Math.abs(z - riverCenter(x));
    const carve = 1 - smoothstep(riverHalfWidth(x) - 0.2, riverHalfWidth(x) + 2.0, dz);
    h = h + (-1.6 - h) * carve;
    return h;
  };

  const tintAt = (x: number, z: number): [number, number, number] => {
    const r = Math.hypot(x, z);
    const forest = smoothstep(42, 56, r);
    const gy = 1 - smoothstep(6, 14, Math.hypot(x - gh.x, z - gh.z));
    const r0 = 1 + forest * 0.02 - gy * 0.15;
    const g0 = 1 - forest * 0.28 + gy * 0.12;
    const b0 = 1 - forest * 0.12 + gy * 0.05;
    return [r0, g0, b0];
  };

  /* -------------------------- Road & bridge ---------------------------- */
  const plazaPoint = { x: 0, z: 0 };
  const front = { x: gh.x + Math.sin(gh.rotY) * 4.6, z: gh.z + Math.cos(gh.rotY) * 4.6 };
  const mid = { x: front.x * 0.5, z: front.z * 0.5 };
  const perp = { x: -(front.z - plazaPoint.z), z: front.x - plazaPoint.x };
  const pl = Math.hypot(perp.x, perp.z) || 1;
  const bend = 5;
  const control = { x: mid.x + (perp.x / pl) * bend, z: mid.z + (perp.z / pl) * bend };
  const road: Vec2Like[] = [];
  const roadSteps = 160;
  for (let i = 0; i <= roadSteps; i++) {
    const t = i / roadSteps;
    const a = (1 - t) * (1 - t);
    const b = 2 * (1 - t) * t;
    const c = t * t;
    road.push({
      x: a * plazaPoint.x + b * control.x + c * front.x,
      z: a * plazaPoint.z + b * control.z + c * front.z,
    });
  }
  // Bridge over any river crossing along the road.
  const bridges: BridgeSpec[] = [];
  let first = -1;
  let last = -1;
  road.forEach((p, i) => {
    if (inRiver(p.x, p.z, 0.4)) {
      if (first < 0) first = i;
      last = i;
    }
  });
  if (first >= 0) {
    const a = road[Math.max(0, first - 3)];
    const b = road[Math.min(road.length - 1, last + 3)];
    const deckY = Math.max(0.6, heightAt(a.x, a.z) + 0.35, heightAt(b.x, b.z) + 0.35);
    bridges.push({ a, b, width: 3.2, deckY });
  }

  const distToRoad = (x: number, z: number): number => {
    let best = Infinity;
    for (let i = 0; i < road.length - 1; i++) {
      const d = distToSegment2({ x, z }, road[i], road[i + 1]);
      if (d < best) best = d;
    }
    return best;
  };

  /* ------------------------------ Village ------------------------------ */
  const houses: HouseSpec[] = [];
  const houseRadius = (w: number, d: number) => Math.hypot(w, d) / 2;
  const houseTarget = 16;
  for (let attempt = 0; attempt < 1600 && houses.length < houseTarget; attempt++) {
    const r = Math.sqrt(rng.next()) * 27 + 11; // area-uniform annulus 11..38 m
    const ang = rng.range(0, Math.PI * 2);
    const x = Math.cos(ang) * r;
    const z = Math.sin(ang) * r;
    const kindRoll = rng.next();
    const kind: BuildingType = kindRoll < 0.42 ? 'cottage' : kindRoll < 0.78 ? 'fudge' : 'shop';
    const w = kind === 'shop' ? rng.range(6, 7.2) : rng.range(4.6, 6.6);
    const d = kind === 'shop' ? rng.range(5.4, 6.4) : rng.range(4.4, 6);
    const h = kind === 'shop' ? 5.6 : rng.range(3.4, 4.6);
    const rad = houseRadius(w, d) + 1.4; // includes the guard's clearance
    if (r < PLAZA_R + rad) continue;
    if (Math.abs(z - riverCenter(x)) < riverHalfWidth(x) + 4 + rad) continue;
    if (distToRoad(x, z) < 3.8 + rad) continue;
    if (Math.hypot(x - gh.x, z - gh.z) < 14 + rad) continue;
    if (houses.some((o) => dist2(o, { x, z }) < houseRadius(o.w, o.d) + rad + 0.6)) continue;
    const toCentre = Math.atan2(-x, -z);
    houses.push({ kind, x, z, rotY: toCentre + rng.range(-0.22, 0.22), w, d, h, seed: rng.int(1, 1e6) });
  }

  /* ------------------------------ Plaza -------------------------------- */
  // The river crosses the north half of the plaza, so lamps and guard spawns
  // search around their nominal angle for dry ground.
  const dryAt = (x: number, z: number) => !inRiver(x, z, 1.5);
  const onRing = (angle: number, radius: number, ok: (x: number, z: number) => boolean): Vec2Like => {
    // Walk outwards from the nominal angle in both directions, up to a full turn.
    for (let k = 0; k <= 250; k++) {
      const a = angle + (k % 2 === 0 ? 1 : -1) * Math.ceil(k / 2) * 0.05;
      const x = Math.cos(a) * radius;
      const z = Math.sin(a) * radius;
      if (ok(x, z)) return { x, z };
    }
    return { x: Math.cos(angle) * radius, z: Math.sin(angle) * radius };
  };
  const lamps: Vec2Like[] = [0, 1, 2, 3].map((i) => onRing(Math.PI / 4 + (i * Math.PI) / 2, 6.4, dryAt));
  const guardSpawns: Vec2Like[] = [0, 1, 2].map((i) => onRing((i * 2 * Math.PI) / 3 + 0.4, 6.6, dryAt));

  /* ------------------------------ Forest ------------------------------- */
  const forest: ForestItem[] = [];
  const grid = new SpatialHash(4);
  const pickKind = (): ForestKind => {
    const roll = rng.next();
    if (roll < 0.26) return 'lollipop';
    if (roll < 0.36) return 'cane';
    if (roll < 0.52) return 'pine';
    if (roll < 0.66) return 'gumdrop';
    if (roll < 0.72) return 'boulder';
    return 'reed';
  };
  const TREE_CLEAR: Record<ForestKind, number> = { lollipop: 2.2, cane: 2.4, pine: 2.6, gumdrop: 2.6, boulder: 2.4, reed: 0.7 };
  for (let attempt = 0; attempt < 14000 && forest.length < 1000; attempt++) {
    const x = rng.range(-WORLD_HALF + 2, WORLD_HALF - 2);
    const z = rng.range(-WORLD_HALF + 2, WORLD_HALF - 2);
    const r = Math.hypot(x, z);
    if (r < 44) continue;
    const density = smoothstep(42, 56, r) * (0.25 + 0.75 * smoothstep(-0.35, 0.3, noise.noise(x * 0.03, z * 0.03, 5)));
    if (!rng.chance(density)) continue;
    if (Math.hypot(x - gh.x, z - gh.z) < 15) continue;
    if (inRiver(x, z, 3)) continue;
    if (distToRoad(x, z) < 3.5) continue;
    if (houses.some((o) => Math.hypot(o.x - x, o.z - z) < houseRadius(o.w, o.d) + 2.5)) continue;
    const kind = pickKind();
    if (grid.anyWithin(x, z, TREE_CLEAR[kind])) continue;
    const scale = kind === 'reed' ? rng.range(0.8, 1.4) : rng.range(0.85, 1.35);
    forest.push({
      kind,
      x,
      y: heightAt(x, z),
      z,
      rotY: rng.range(0, Math.PI * 2),
      scale,
      variant: rng.int(0, 3),
      tiltX: kind === 'reed' ? rng.range(-0.2, 0.2) : undefined,
      tiltZ: kind === 'reed' ? rng.range(-0.2, 0.2) : undefined,
    });
    grid.insert(x, z);
  }

  /* ----------------------------- Obstacles ------------------------------ */
  const obstacles: Obstacle[] = [];
  for (const h of houses) {
    obstacles.push({ type: 'rect', x: h.x, z: h.z, hw: h.w / 2 + 0.5, hd: h.d / 2 + 0.5, yaw: h.rotY });
  }
  obstacles.push({ type: 'rect', x: gh.x, z: gh.z, hw: 4.2, hd: 3.9, yaw: gh.rotY });
  obstacles.push({ type: 'circle', x: 0, z: 0, r: 3.9 });
  for (const l of lamps) obstacles.push({ type: 'circle', x: l.x, z: l.z, r: 0.5 });
  const walkOverrides: WorldLayout['walkOverrides'] = [];
  for (const f of forest) {
    const r = { lollipop: 0.4, cane: 0.45, pine: 0.75, gumdrop: 1.0, boulder: 0.95, reed: 0 }[f.kind];
    if (r > 0) obstacles.push({ type: 'circle', x: f.x, z: f.z, r: r * f.scale });
  }
  for (const b of bridges) {
    const mx = (b.a.x + b.b.x) / 2;
    const mz = (b.a.z + b.b.z) / 2;
    const len = Math.hypot(b.b.x - b.a.x, b.b.z - b.a.z) + 1.4;
    walkOverrides.push({ x: mx, z: mz, hw: b.width / 2 + 0.3, hd: len / 2, yaw: Math.atan2(b.b.x - b.a.x, b.b.z - b.a.z) });
  }

  return {
    seed,
    half: WORLD_HALF,
    heightAt,
    riverCenter,
    riverHalfWidth,
    inRiver,
    tintAt,
    houses,
    gingerbread: gh,
    road,
    bridges,
    lamps,
    fountain: { x: 0, z: 0 },
    guardSpawns,
    forest,
    obstacles,
    walkOverrides,
  };
}

/** Coarse hash grid for fast "is there something nearby" queries. */
class SpatialHash {
  private cells = new Map<string, Vec2Like[]>();
  constructor(private readonly size: number) {}

  private key(ix: number, iz: number): string {
    return `${ix},${iz}`;
  }

  insert(x: number, z: number): void {
    const k = this.key(Math.floor(x / this.size), Math.floor(z / this.size));
    const list = this.cells.get(k) ?? [];
    list.push({ x, z });
    this.cells.set(k, list);
  }

  anyWithin(x: number, z: number, radius: number): boolean {
    const span = Math.ceil(radius / this.size);
    const cx = Math.floor(x / this.size);
    const cz = Math.floor(z / this.size);
    for (let i = -span; i <= span; i++) {
      for (let j = -span; j <= span; j++) {
        const list = this.cells.get(this.key(cx + i, cz + j));
        if (!list) continue;
        for (const p of list) {
          if (Math.hypot(p.x - x, p.z - z) < radius) return true;
        }
      }
    }
    return false;
  }
}

export const clampWorld = (v: number): number => clamp(v, -WORLD_HALF + 1, WORLD_HALF - 1);
