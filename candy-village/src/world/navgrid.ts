/**
 * Walkability grid and A* path planner for the giant guards.
 *
 * The grid is rasterised from the same layout data the meshes use: houses,
 * trees, the fountain and the river are obstacles, bridges re-open the river,
 * and steep slopes are blocked. Paths are A* on an 8-connected grid followed by
 * a line-of-sight string-pull so guards walk straight lines where possible.
 */
import type { Obstacle, WorldLayout } from './layout';
import type { Vec2Like } from './layout';

/** Clearance added around every obstacle so a 2.5 m-wide guard fits. */
const GUARD_CLEARANCE = 1.2;
const CELL = 1.0;

class MinHeap {
  private keys: number[] = [];
  private vals: number[] = [];

  get size(): number {
    return this.vals.length;
  }

  push(key: number, val: number): void {
    const k = this.keys;
    const v = this.vals;
    k.push(key);
    v.push(val);
    let i = k.length - 1;
    while (i > 0) {
      const p = (i - 1) >> 1;
      if (k[p] <= k[i]) break;
      [k[p], k[i]] = [k[i], k[p]];
      [v[p], v[i]] = [v[i], v[p]];
      i = p;
    }
  }

  pop(): number {
    const k = this.keys;
    const v = this.vals;
    const top = v[0];
    const lastK = k.pop()!;
    const lastV = v.pop()!;
    if (k.length > 0) {
      k[0] = lastK;
      v[0] = lastV;
      let i = 0;
      for (;;) {
        const l = 2 * i + 1;
        const r = l + 1;
        let m = i;
        if (l < k.length && k[l] < k[m]) m = l;
        if (r < k.length && k[r] < k[m]) m = r;
        if (m === i) break;
        [k[m], k[i]] = [k[i], k[m]];
        [v[m], v[i]] = [v[i], v[m]];
        i = m;
      }
    }
    return top;
  }
}

export class NavGrid {
  readonly n: number;
  readonly half: number;
  private readonly walk: Uint8Array;
  constructor(layout: WorldLayout) {
    this.half = layout.half;
    this.n = Math.round((2 * layout.half) / CELL);
    this.walk = new Uint8Array(this.n * this.n);

    // 1. Terrain: rivers and steep slopes are blocked.
    const heights = new Float32Array(this.n * this.n);
    for (let j = 0; j < this.n; j++) {
      for (let i = 0; i < this.n; i++) {
        const [x, z] = this.cellCentre(i, j);
        heights[j * this.n + i] = layout.heightAt(x, z);
      }
    }
    for (let j = 0; j < this.n; j++) {
      for (let i = 0; i < this.n; i++) {
        const [x, z] = this.cellCentre(i, j);
        const idx = j * this.n + i;
        let ok = Math.abs(x) < layout.half - 2 && Math.abs(z) < layout.half - 2;
        if (ok && layout.inRiver(x, z, 1.0)) ok = false;
        if (ok && i + 1 < this.n && j + 1 < this.n) {
          const dx = Math.abs(heights[idx] - heights[idx + 1]);
          const dz = Math.abs(heights[idx] - heights[idx + this.n]);
          if (Math.max(dx, dz) > 1.2) ok = false;
        }
        this.walk[idx] = ok ? 1 : 0;
      }
    }

    // 2. Obstacles, inflated by guard clearance.
    for (const o of layout.obstacles) this.stampObstacle(o);

    // 3. Bridges re-open the river crossing.
    for (const b of layout.walkOverrides) {
      this.stampRect(b.x, b.z, b.hw, b.hd, b.yaw, 0, true);
    }
  }

  /** World (x, z) of a cell centre. */
  cellCentre(i: number, j: number): [number, number] {
    return [-this.half + (i + 0.5) * CELL, -this.half + (j + 0.5) * CELL];
  }

  private cellOf(x: number, z: number): [number, number] {
    const i = Math.floor((x + this.half) / CELL);
    const j = Math.floor((z + this.half) / CELL);
    return [Math.min(this.n - 1, Math.max(0, i)), Math.min(this.n - 1, Math.max(0, j))];
  }

  private stampObstacle(o: Obstacle): void {
    if (o.type === 'circle') {
      const r = o.r + GUARD_CLEARANCE;
      this.stampCircle(o.x, o.z, r);
    } else {
      this.stampRect(o.x, o.z, o.hw, o.hd, o.yaw, GUARD_CLEARANCE, false);
    }
  }

  private stampCircle(cx: number, cz: number, r: number): void {
    const [i0, j0] = this.cellOf(cx - r, cz - r);
    const [i1, j1] = this.cellOf(cx + r, cz + r);
    for (let j = j0; j <= j1; j++) {
      for (let i = i0; i <= i1; i++) {
        const [x, z] = this.cellCentre(i, j);
        if (Math.hypot(x - cx, z - cz) <= r) this.walk[j * this.n + i] = 0;
      }
    }
  }

  private stampRect(cx: number, cz: number, hw: number, hd: number, yaw: number, clear: number, open: boolean): void {
    const R = Math.hypot(hw, hd) + clear;
    const [i0, j0] = this.cellOf(cx - R, cz - R);
    const [i1, j1] = this.cellOf(cx + R, cz + R);
    const c = Math.cos(-yaw);
    const s = Math.sin(-yaw);
    for (let j = j0; j <= j1; j++) {
      for (let i = i0; i <= i1; i++) {
        const [x, z] = this.cellCentre(i, j);
        const dx = x - cx;
        const dz = z - cz;
        const lx = dx * c + dz * s;
        const lz = -dx * s + dz * c;
        if (Math.abs(lx) <= hw + clear && Math.abs(lz) <= hd + clear) {
          this.walk[j * this.n + i] = open ? 1 : 0;
        }
      }
    }
  }

  isWalkable(x: number, z: number): boolean {
    const [i, j] = this.cellOf(x, z);
    return this.walk[j * this.n + i] === 1;
  }

  /** Nearest walkable point to (x, z), searched in growing rings. */
  nearestWalkable(x: number, z: number, maxRadius = 25): Vec2Like | null {
    if (this.isWalkable(x, z)) return { x, z };
    for (let r = 1; r <= maxRadius; r++) {
      let best: Vec2Like | null = null;
      let bestD = Infinity;
      for (let a = 0; a < 16 * r; a++) {
        const ang = (a / (16 * r)) * Math.PI * 2;
        const px = x + Math.cos(ang) * r;
        const pz = z + Math.sin(ang) * r;
        if (this.isWalkable(px, pz)) {
          const d = Math.hypot(px - x, pz - z);
          if (d < bestD) {
            bestD = d;
            best = { x: px, z: pz };
          }
        }
      }
      if (best) return best;
    }
    return null;
  }

  /** True when the straight segment a-b stays on walkable cells. */
  lineOfSight(a: Vec2Like, b: Vec2Like): boolean {
    const len = Math.hypot(b.x - a.x, b.z - a.z);
    const steps = Math.max(1, Math.ceil(len / 0.4));
    for (let k = 1; k < steps; k++) {
      const t = k / steps;
      if (!this.isWalkable(a.x + (b.x - a.x) * t, a.z + (b.z - a.z) * t)) return false;
    }
    return true;
  }

  /**
   * A* from `from` to `to`. Returns world-space waypoints (excluding the start
   * point), or null if unreachable.
   */
  findPath(from: Vec2Like, to: Vec2Like): Vec2Like[] | null {
    const start = this.nearestWalkable(from.x, from.z);
    const goal = this.nearestWalkable(to.x, to.z);
    if (!start || !goal) return null;
    const [si, sj] = this.cellOf(start.x, start.z);
    const [gi, gj] = this.cellOf(goal.x, goal.z);
    const startIdx = sj * this.n + si;
    const goalIdx = gj * this.n + gi;
    if (startIdx === goalIdx) return [goal];

    const N = this.n * this.n;
    const g = new Float32Array(N).fill(Infinity);
    const parent = new Int32Array(N).fill(-1);
    const closed = new Uint8Array(N);
    const heap = new MinHeap();
    const h = (idx: number): number => {
      const i = idx % this.n;
      const j = (idx - i) / this.n;
      const dx = Math.abs(i - gi);
      const dz = Math.abs(j - gj);
      return Math.max(dx, dz) + (Math.SQRT2 - 1) * Math.min(dx, dz);
    };
    g[startIdx] = 0;
    heap.push(h(startIdx), startIdx);
    const dirs = [
      [1, 0, 1], [-1, 0, 1], [0, 1, 1], [0, -1, 1],
      [1, 1, Math.SQRT2], [1, -1, Math.SQRT2], [-1, 1, Math.SQRT2], [-1, -1, Math.SQRT2],
    ];
    let found = false;
    let iterations = 0;
    while (heap.size > 0 && iterations++ < N * 4) {
      const cur = heap.pop();
      if (closed[cur]) continue;
      if (cur === goalIdx) {
        found = true;
        break;
      }
      closed[cur] = 1;
      const ci = cur % this.n;
      const cj = (cur - ci) / this.n;
      for (const [di, dj, cost] of dirs) {
        const ni = ci + di;
        const nj = cj + dj;
        if (ni < 0 || nj < 0 || ni >= this.n || nj >= this.n) continue;
        const nIdx = nj * this.n + ni;
        if (!this.walk[nIdx] || closed[nIdx]) continue;
        if (di !== 0 && dj !== 0) {
          // Forbid corner cutting between two blocked orthogonal cells.
          if (!this.walk[cj * this.n + ni] || !this.walk[nj * this.n + ci]) continue;
        }
        const tentative = g[cur] + cost;
        if (tentative < g[nIdx]) {
          g[nIdx] = tentative;
          parent[nIdx] = cur;
          heap.push(tentative + h(nIdx), nIdx);
        }
      }
    }
    if (!found) return null;

    const cells: Vec2Like[] = [];
    for (let idx = goalIdx; idx !== -1; idx = parent[idx]) {
      const i = idx % this.n;
      const j = (idx - i) / this.n;
      const [x, z] = this.cellCentre(i, j);
      cells.push({ x, z });
    }
    cells.reverse();
    cells[0] = { x: from.x, z: from.z };
    cells[cells.length - 1] = { x: to.x, z: to.z };
    return this.smooth(cells);
  }

  /** Greedy line-of-sight string pulling. */
  private smooth(pts: Vec2Like[]): Vec2Like[] {
    const out: Vec2Like[] = [];
    let i = 0;
    while (i < pts.length - 1) {
      let j = pts.length - 1;
      while (j > i + 1 && !this.lineOfSight(pts[i], pts[j])) j--;
      out.push(pts[j]);
      i = j;
    }
    return out;
  }
}
