/**
 * 2.5D shallow-water simulation.
 *
 * This is a finite-volume solver on an Arakawa C-grid, NOT a scrolling wave
 * normal map and NOT a displacement texture. State variables are:
 *
 *   eta[i,j]  free-surface elevation relative to sea level (cell centres)
 *   u[i,j]    east-west velocity on vertical faces
 *   v[i,j]    north-south velocity on horizontal faces
 *   bed[i,j]  SDF-derived bathymetry (cell centres)
 *
 * It solves the nonlinear shallow-water equations with wet/dry faces, a CFL
 * adaptive substep count, reflective land boundaries and an absorbing sponge
 * at the clipmap edge. The ocean mesh reads eta directly; wave appearance is
 * a consequence of the solved height and velocity fields.
 */

import { clamp, smoothstep } from '../world/math';
import type { SdfWorld } from '../world/SdfWorld';

export interface WaterSettings {
  gridSize: number;       // cells per side in the simulated near patch
  cellSize: number;       // metres per simulated cell
  farLevels: number;      // tessellated LOD rings beyond the simulated patch
  gravity: number;
  damping: number;
  wind: number;
  windAngle: number;      // degrees
  waveHeight: number;     // far-field analytic boundary amplitude, metres
  waveMaker: number;      // open-boundary swell injected into the solver, metres
  sponge: number;
  simSpeed: number;
  paused: boolean;
  dropSize: number;
}

export const DEFAULT_WATER_SETTINGS: WaterSettings = {
  gridSize: 128,
  cellSize: 8,
  farLevels: 9,
  gravity: 9.81,
  damping: 0.016,
  wind: 0.35,
  windAngle: 38,
  waveHeight: 1.15,
  waveMaker: 0.10,
  sponge: 0.85,
  simSpeed: 1,
  paused: false,
  dropSize: 1.6,
};

export interface WaterStats {
  wetCells: number;
  totalCells: number;
  maxDepth: number;
  maxVelocity: number;
  substeps: number;
  stableDt: number;
  mass: number;
  massDrift: number;
  energy: number;
  time: number;
}

function bilinear(arr: Float64Array, gx: number, gz: number, n: number): number {
  const x = clamp(gx, 0, n - 1);
  const y = clamp(gz, 0, n - 1);
  const i0 = Math.floor(x);
  const j0 = Math.floor(y);
  const i1 = Math.min(n - 1, i0 + 1);
  const j1 = Math.min(n - 1, j0 + 1);
  const tx = x - i0;
  const ty = y - j0;
  const a = arr[i0 + j0 * n];
  const b = arr[i1 + j0 * n];
  const c = arr[i0 + j1 * n];
  const d = arr[i1 + j1 * n];
  return (a * (1 - tx) + b * tx) * (1 - ty) + (c * (1 - tx) + d * tx) * ty;
}

export class ShallowWaterSolver {
  readonly n: number;
  readonly dx: number;
  readonly seaLevel: number;
  centerX: number;
  centerZ: number;

  eta: Float64Array;
  bed: Float64Array;
  u: Float64Array;  // (n+1) * n
  v: Float64Array;  // n * (n+1)
  private fluxU: Float64Array;
  private fluxV: Float64Array;
  private sponge: Float64Array;

  time = 0;
  mass0 = 1;
  lastSubsteps = 1;
  lastStableDt = 0;

  constructor(n: number, dx: number, world: SdfWorld, centerX: number, centerZ: number) {
    this.n = n;
    this.dx = dx;
    this.seaLevel = world.seaLevel;
    this.centerX = centerX;
    this.centerZ = centerZ;
    const count = n * n;
    this.eta = new Float64Array(count);
    this.bed = new Float64Array(count);
    this.u = new Float64Array((n + 1) * n);
    this.v = new Float64Array(n * (n + 1));
    this.fluxU = new Float64Array((n + 1) * n);
    this.fluxV = new Float64Array(n * (n + 1));
    this.sponge = new Float64Array(count);
    this.rebuild(world, centerX, centerZ);
  }

  cellWorldX(i: number): number { return this.centerX + (i + 0.5 - this.n / 2) * this.dx; }
  cellWorldZ(j: number): number { return this.centerZ + (j + 0.5 - this.n / 2) * this.dx; }

  rebuild(world: SdfWorld, centerX: number, centerZ: number): void {
    this.centerX = centerX;
    this.centerZ = centerZ;
    this.seaLevelMutable(world.seaLevel);
    for (let j = 0; j < this.n; j++) {
      for (let i = 0; i < this.n; i++) {
        const k = i + j * this.n;
        this.bed[k] = world.heightAt(this.cellWorldX(i), this.cellWorldZ(j));
        this.eta[k] = 0;
        const edge = Math.min(i, j, this.n - 1 - i, this.n - 1 - j);
        this.sponge[k] = 1 - smoothstep(0, this.n * 0.14, edge);
      }
    }
    this.u.fill(0);
    this.v.fill(0);
    this.fluxU.fill(0);
    this.fluxV.fill(0);
    this.time = 0;
    this.mass0 = Math.max(1e-9, this.computeMass());
  }

  private seaLevelMutable(value: number): void {
    // `seaLevel` is readonly for callers, but world edits can change it.
    (this as { seaLevel: number }).seaLevel = value;
  }

  /** Re-sample bathymetry after an SDF edit. Surface state is reset for clarity. */
  rebathymetry(world: SdfWorld): void {
    this.rebuild(world, this.centerX, this.centerZ);
  }

  /** Remove surface disturbances without changing bathymetry or grid position. */
  calm(): void {
    this.eta.fill(0);
    this.u.fill(0);
    this.v.fill(0);
    this.fluxU.fill(0);
    this.fluxV.fill(0);
    this.mass0 = Math.max(1e-9, this.computeMass());
  }

  private fillNewBed(world: SdfWorld, i: number, j: number): number {
    return world.heightAt(this.cellWorldX(i), this.cellWorldZ(j));
  }

  private shiftArray(
    src: Float64Array, nx: number, ny: number, shiftX: number, shiftY: number,
    fill: (i: number, j: number) => number,
  ): Float64Array {
    const dst = new Float64Array(src.length);
    for (let j = 0; j < ny; j++) {
      const sj = j + shiftY;
      for (let i = 0; i < nx; i++) {
        const si = i + shiftX;
        const d = i + j * nx;
        if (si >= 0 && si < nx && sj >= 0 && sj < ny) dst[d] = src[si + sj * nx];
        else dst[d] = fill(i, j);
      }
    }
    return dst;
  }

  /** Translate the solver by whole cells as the camera crosses a snap boundary. */
  shift(deltaCellsX: number, deltaCellsZ: number, world: SdfWorld, newCenterX: number, newCenterZ: number): void {
    if (!deltaCellsX && !deltaCellsZ) return;
    const oldX = this.centerX;
    const oldZ = this.centerZ;
    this.centerX = newCenterX;
    this.centerZ = newCenterZ;
    this.seaLevelMutable(world.seaLevel);

    const n = this.n;
    this.eta = this.shiftArray(this.eta, n, n, deltaCellsX, deltaCellsZ, () => 0);
    this.bed = this.shiftArray(this.bed, n, n, deltaCellsX, deltaCellsZ, (i, j) => this.fillNewBed(world, i, j));
    this.u = this.shiftArray(this.u, n + 1, n, deltaCellsX, deltaCellsZ, () => 0);
    this.v = this.shiftArray(this.v, n, n + 1, deltaCellsX, deltaCellsZ, () => 0);
    // Sponge is index-space only and does not move.
    void oldX; void oldZ;
  }

  private faceDepth(surfaceA: number, bedA: number, surfaceB: number, bedB: number): number {
    return Math.max(0, Math.min(surfaceA, surfaceB) - Math.max(bedA, bedB));
  }

  addDrop(worldX: number, worldZ: number, radius: number, amplitude: number): void {
    const gx = (worldX - this.centerX) / this.dx + this.n / 2 - 0.5;
    const gz = (worldZ - this.centerZ) / this.dx + this.n / 2 - 0.5;
    const rad = Math.max(1, radius / this.dx);
    const sigma2 = 2 * rad * rad * 0.35;
    const i0 = Math.max(0, Math.floor(gx - rad * 2));
    const i1 = Math.min(this.n - 1, Math.ceil(gx + rad * 2));
    const j0 = Math.max(0, Math.floor(gz - rad * 2));
    const j1 = Math.min(this.n - 1, Math.ceil(gz + rad * 2));
    for (let j = j0; j <= j1; j++) {
      for (let i = i0; i <= i1; i++) {
        const d2 = (i - gx) * (i - gx) + (j - gz) * (j - gz);
        const k = i + j * this.n;
        const dryFloor = this.bed[k] - this.seaLevel;
        this.eta[k] = Math.max(this.eta[k], dryFloor);
        if (this.seaLevel + this.eta[k] - this.bed[k] <= 0.02) continue;
        this.eta[k] += amplitude * Math.exp(-d2 / sigma2);
      }
    }
  }

  addMomentum(worldX: number, worldZ: number, radius: number, vx: number, vz: number): void {
    const gx = (worldX - this.centerX) / this.dx + this.n / 2 - 0.5;
    const gz = (worldZ - this.centerZ) / this.dx + this.n / 2 - 0.5;
    const rad = Math.max(1, radius / this.dx);
    const sigma2 = 2 * rad * rad * 0.4;
    for (let j = Math.max(0, Math.floor(gz - rad * 2)); j <= Math.min(this.n - 1, Math.ceil(gz + rad * 2)); j++) {
      for (let i = Math.max(0, Math.floor(gx - rad * 2)); i <= Math.min(this.n, Math.ceil(gx + rad * 2)); i++) {
        const d2 = (i - gx) * (i - gx) + (j - gz) * (j - gz);
        const w = Math.exp(-d2 / sigma2);
        if (j < this.n) this.u[i + j * (this.n + 1)] += vx * w;
        if (i < this.n) this.v[i + j * this.n] += vz * w;
      }
    }
  }

  private computeMass(): number {
    let mass = 0;
    for (let k = 0; k < this.eta.length; k++) {
      mass += Math.max(0, this.seaLevel + this.eta[k] - this.bed[k]) * this.dx * this.dx;
    }
    return mass;
  }

  private maxState(): { depth: number; velocity: number } {
    let depth = 0;
    let velocity = 0;
    for (let k = 0; k < this.eta.length; k++) depth = Math.max(depth, this.seaLevel + this.eta[k] - this.bed[k]);
    for (let k = 0; k < this.u.length; k++) velocity = Math.max(velocity, Math.abs(this.u[k]));
    for (let k = 0; k < this.v.length; k++) velocity = Math.max(velocity, Math.abs(this.v[k]));
    return { depth: Math.max(0, depth), velocity };
  }

  advance(frameDt: number, settings: WaterSettings, world: SdfWorld): void {
    this.seaLevelMutable(world.seaLevel);
    if (settings.paused) { this.lastSubsteps = 0; return; }
    const dt = clamp(frameDt, 0, 1 / 20) * Math.max(0, settings.simSpeed);
    if (dt <= 0) return;
    const state = this.maxState();
    const waveSpeed = Math.sqrt(Math.max(0.01, settings.gravity * state.depth));
    const stableDt = 0.42 * this.dx / (waveSpeed + state.velocity + 1);
    const substeps = clamp(Math.ceil(dt / Math.max(1e-4, stableDt)), 1, 10);
    const h = dt / substeps;
    this.lastSubsteps = substeps;
    this.lastStableDt = stableDt;
    for (let s = 0; s < substeps; s++) {
      this.step(h, settings, world);
      this.time += h;
    }
  }

  private step(h: number, settings: WaterSettings, world: SdfWorld): void {
    const n = this.n;
    const dx = this.dx;
    const g = settings.gravity;
    const angle = settings.windAngle * Math.PI / 180;
    const windX = Math.cos(angle) * settings.wind;
    const windZ = Math.sin(angle) * settings.wind;
    const damp = Math.exp(-Math.max(0, settings.damping) * h);
    const spongeRate = settings.sponge;
    const t = this.time;

    // Open-boundary swell: a low-amplitude sinusoidal surface condition at the
    // upwind edge. Set waveMaker to zero for a closed, mass-conserving basin.
    if (settings.waveMaker > 0) {
      const omega = 2 * Math.PI / 6.5;
      for (let i = 0; i < n; i++) {
        const x = this.cellWorldX(i);
        const phase = omega * t + x / 2600;
        const a = settings.waveMaker * Math.sin(phase);
        const kTop = i;
        const kBottom = i + (n - 1) * n;
        if (this.seaLevel + this.eta[kTop] - this.bed[kTop] > 0.02) {
          this.eta[kTop] += (a - this.eta[kTop]) * 0.06;
        }
        if (this.seaLevel + this.eta[kBottom] - this.bed[kBottom] > 0.02) {
          this.eta[kBottom] += (-a * 0.35 - this.eta[kBottom]) * 0.04;
        }
      }
    }

    // Momentum: u faces.
    for (let j = 0; j < n; j++) {
      this.u[j * (n + 1)] = 0;
      this.u[n + j * (n + 1)] = 0;
      for (let i = 1; i < n; i++) {
        const left = i - 1 + j * n;
        const right = i + j * n;
        const surfaceL = this.seaLevel + this.eta[left];
        const surfaceR = this.seaLevel + this.eta[right];
        const depth = this.faceDepth(surfaceL, this.bed[left], surfaceR, this.bed[right]);
        const idx = i + j * (n + 1);
        if (depth <= 1e-4) { this.u[idx] = 0; continue; }
        const sponge = 0.5 * (this.sponge[left] + this.sponge[right]);
        const gust = 0.65 + 0.35 * Math.sin(t * 0.8 + this.cellWorldX(i) * 0.000013 + this.cellWorldZ(j) * 0.000011);
        let value = this.u[idx] - g * h * (this.eta[right] - this.eta[left]) / dx;
        value += windX * h * 0.045 * gust * (1 - sponge);
        value *= damp * (1 - sponge * spongeRate * h * 1.8);
        this.u[idx] = clamp(value, -25, 25);
      }
    }

    // Momentum: v faces.
    for (let i = 0; i < n; i++) {
      this.v[i] = 0;
      this.v[i + n * n] = 0;
      for (let j = 1; j < n; j++) {
        const top = i + (j - 1) * n;
        const bottom = i + j * n;
        const surfaceT = this.seaLevel + this.eta[top];
        const surfaceB = this.seaLevel + this.eta[bottom];
        const depth = this.faceDepth(surfaceT, this.bed[top], surfaceB, this.bed[bottom]);
        const idx = i + j * n;
        if (depth <= 1e-4) { this.v[idx] = 0; continue; }
        const sponge = 0.5 * (this.sponge[top] + this.sponge[bottom]);
        const gust = 0.65 + 0.35 * Math.sin(t * 0.73 + this.cellWorldX(i) * 0.000009 - this.cellWorldZ(j) * 0.000017);
        let value = this.v[idx] - g * h * (this.eta[bottom] - this.eta[top]) / dx;
        value += windZ * h * 0.045 * gust * (1 - sponge);
        value *= damp * (1 - sponge * spongeRate * h * 1.8);
        this.v[idx] = clamp(value, -25, 25);
      }
    }

    // Face fluxes from the updated velocities.
    this.fluxU.fill(0);
    this.fluxV.fill(0);
    for (let j = 0; j < n; j++) {
      for (let i = 1; i < n; i++) {
        const left = i - 1 + j * n;
        const right = i + j * n;
        const depth = this.faceDepth(
          this.seaLevel + this.eta[left], this.bed[left],
          this.seaLevel + this.eta[right], this.bed[right],
        );
        this.fluxU[i + j * (n + 1)] = this.u[i + j * (n + 1)] * depth;
      }
      for (let i = 0; i < n; i++) {
        const top = i + (j - 1) * n;
        const bottom = i + j * n;
        if (j === 0) { this.fluxV[i] = 0; continue; }
        const depth = this.faceDepth(
          this.seaLevel + this.eta[top], this.bed[top],
          this.seaLevel + this.eta[bottom], this.bed[bottom],
        );
        this.fluxV[i + j * n] = this.v[i + j * n] * depth;
      }
    }

    // Continuity: conservative divergence of depth flux.
    for (let j = 0; j < n; j++) {
      for (let i = 0; i < n; i++) {
        const k = i + j * n;
        const div = (
          this.fluxU[i + 1 + j * (n + 1)] - this.fluxU[i + j * (n + 1)] +
          this.fluxV[i + (j + 1) * n] - this.fluxV[i + j * n]
        ) / dx;
        this.eta[k] -= h * div;
        const dryFloor = this.bed[k] - this.seaLevel;
        if (this.eta[k] < dryFloor) this.eta[k] = dryFloor;
        if (spongeRate > 0 && this.sponge[k] > 0) {
          this.eta[k] += (0 - this.eta[k]) * this.sponge[k] * spongeRate * h * 2.2;
          if (this.eta[k] < dryFloor) this.eta[k] = dryFloor;
        }
      }
    }
    void world;
  }

  sampleEta(worldX: number, worldZ: number): number {
    const gx = (worldX - this.centerX) / this.dx + this.n / 2 - 0.5;
    const gz = (worldZ - this.centerZ) / this.dx + this.n / 2 - 0.5;
    return bilinear(this.eta, gx, gz, this.n);
  }

  sampleDepth(worldX: number, worldZ: number): number {
    const gx = (worldX - this.centerX) / this.dx + this.n / 2 - 0.5;
    const gz = (worldZ - this.centerZ) / this.dx + this.n / 2 - 0.5;
    const bed = bilinear(this.bed, gx, gz, this.n);
    return Math.max(0, this.seaLevel + bilinear(this.eta, gx, gz, this.n) - bed);
  }

  sampleVelocity(worldX: number, worldZ: number): [number, number] {
    const gx = (worldX - this.centerX) / this.dx + this.n / 2 - 0.5;
    const gz = (worldZ - this.centerZ) / this.dx + this.n / 2 - 0.5;
    const i = clamp(Math.round(gx), 0, this.n - 1);
    const j = clamp(Math.round(gz), 0, this.n - 1);
    return [
      0.5 * (this.u[i + j * (this.n + 1)] + this.u[i + 1 + j * (this.n + 1)]),
      0.5 * (this.v[i + j * this.n] + this.v[i + (j + 1) * this.n]),
    ];
  }

  stats(): WaterStats {
    const state = this.maxState();
    const mass = this.computeMass();
    let wet = 0;
    let energy = 0;
    for (let k = 0; k < this.eta.length; k++) {
      const depth = this.seaLevel + this.eta[k] - this.bed[k];
      if (depth > 0.02) wet++;
      energy += Math.abs(this.eta[k]) * Math.max(0, depth) * this.dx * this.dx;
    }
    return {
      wetCells: wet,
      totalCells: this.eta.length,
      maxDepth: state.depth,
      maxVelocity: state.velocity,
      substeps: this.lastSubsteps,
      stableDt: this.lastStableDt,
      mass,
      massDrift: (mass - this.mass0) / this.mass0,
      energy,
      time: this.time,
    };
  }
}
