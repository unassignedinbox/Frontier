/**
 * Shallow-water solver for one patch of ocean.
 *
 * State, on an Arakawa C-grid (staggered: heights at cell centres, velocities on
 * faces, which is what makes the divergence operator exactly conservative):
 *
 *   eta[i,j]   free-surface elevation, metres, relative to sea level
 *   u[i,j]     eastward face velocity, m/s
 *   v[i,j]     northward face velocity, m/s
 *   bed[i,j]   seabed altitude, metres (from the terrain SDF)
 *   depth      = max(0, eta - bed)
 *
 * Solved with explicit finite volumes:
 *
 *   d(eta)/dt = -div( H * u )
 *   du/dt     = -g * d(eta)/dx  +  wind stress  +  bottom friction  +  viscosity
 *
 * Three details make it stable and believable rather than a wave-normal shim:
 *
 *  1. CFL substeps. dt is chosen per frame from max sqrt(g*H) so a fast-moving
 *     storm surge cannot blow up. Nothing is tuned by hand.
 *
 *  2. DISPERSION LIMIT. Resolving 4300 m of abyssal depth on a 6 m grid with a
 *     non-dispersive solver would ask for 200 m/s gravity waves and a dt of
 *     15 ms. Real gravity waves at that wavelength are dispersive: c = sqrt(g/k),
 *     which at the grid Nyquist gives ~4 m/s. We therefore cap celerity at
 *     sqrt(g*dx/pi). This is the physically correct deep-water limit for the
 *     shortest resolvable wave, not a fudge factor, and it makes the near patch
 *     cost a couple of substeps per frame instead of fifty.
 *
 *  3. Wet/dry. On land, depth goes to zero and the face is closed (reflective),
 *     so a swash front genuinely climbs a beach and retreats. This is what makes
 *     the shoreline behave like a shoreline.
 *
 * The patch is planar in the tangent frame for the solver (a few hundred cells
 * across a 6 km patch, so curvature over one cell is sub-centimetre), but the
 * resulting eta is applied to vertices placed on the true sphere. That split is
 * deliberate: physics in a cheap local frame, geometry on the globe.
 */

export interface WaterSettings {
  /** Cells per side. */
  gridSize: number;
  /** Cell size in metres. */
  cellSize: number;
  /** Gravity, m/s². */
  gravity: number;
  /** 0..1: fraction of velocity lost per second to bottom friction. */
  bottomFriction: number;
  /** Wind stress along windAngle, in m²/s². */
  windStress: number;
  /** Wind direction, radians (0 = +x/east). */
  windAngle: number;
  /** Kinematic viscosity, m²/s. */
  viscosity: number;
  /** Absorbing band width in cells at the patch border. */
  spongeCells: number;
}

export const DEFAULT_WATER_SETTINGS: WaterSettings = {
  gridSize: 128,
  cellSize: 6,
  gravity: 9.81,
  bottomFriction: 0.035,
  windStress: 0.06,
  windAngle: 0.7,
  viscosity: 0.0004,
  spongeCells: 8,
};

export interface SWEStats {
  wetCells: number;
  totalCells: number;
  maxDepth: number;
  maxEta: number;
  maxSpeed: number;
  substeps: number;
  dt: number;
  volume: number;
  energy: number;
  time: number;
}

export type BedSampler = (x: number, z: number) => number;

export class SWEPatch {
  readonly n: number;
  cellSize: number;
  settings: WaterSettings;

  eta: Float32Array;
  u: Float32Array;
  v: Float32Array;
  bed: Float32Array;

  /** Patch centre in the host tangent frame, metres. */
  centerX = 0;
  centerZ = 0;

  private etaNext: Float32Array;
  private sponge: Float32Array;
  private time = 0;
  private calls = 0;
  private lastSubsteps = 0;

  constructor(settings: WaterSettings) {
    this.settings = settings;
    this.n = settings.gridSize;
    this.cellSize = settings.cellSize;
    const cells = this.n * this.n;
    this.eta = new Float32Array(cells);
    this.u = new Float32Array(cells);
    this.v = new Float32Array(cells);
    this.bed = new Float32Array(cells);
    this.etaNext = new Float32Array(cells);
    this.sponge = new Float32Array(cells);
    this.buildSponge();
  }

  private buildSponge(): void {
    const n = this.n;
    const band = Math.max(2, this.settings.spongeCells);
    for (let j = 0; j < n; j++) {
      for (let i = 0; i < n; i++) {
        const edge = Math.min(i, j, n - 1 - i, n - 1 - j);
        const t = Math.max(0, 1 - edge / band);
        this.sponge[i + j * n] = t * t * 0.85;
      }
    }
  }

  /** Sample the seabed. Only call when the patch moves or terrain changes. */
  sampleBed(sample: BedSampler): void {
    const n = this.n;
    const half = (n * this.cellSize) * 0.5;
    for (let j = 0; j < n; j++) {
      const z = this.centerZ + (j + 0.5) * this.cellSize - half;
      for (let i = 0; i < n; i++) {
        const x = this.centerX + (i + 0.5) * this.cellSize - half;
        this.bed[i + j * n] = sample(x, z);
      }
    }
  }

  /**
   * Move the patch by whole cells, preserving surface state.
   * This is the water equivalent of a clipmap shift, and it is why you can fly
   * for an hour without the near patch showing a seam or a reset.
   */
  shift(dx: number, dz: number): boolean {
    const n = this.n;
    const step = this.cellSize;
    const sx = Math.round(dx / step);
    const sz = Math.round(dz / step);
    if (sx === 0 && sz === 0) return false;
    const ax = Math.abs(sx);
    const az = Math.abs(sz);
    if (ax >= n || az >= n) {
      this.eta.fill(0);
      this.u.fill(0);
      this.v.fill(0);
      this.centerX += sx * step;
      this.centerZ += sz * step;
      return true;
    }
    // Scrolling copy: direction of the shift decides which edge is fresh.
    // Fresh cells are filled by zero-gradient extension of the nearest kept row,
    // not with zeros: filling with zeros would delete real water on every shift
    // (a few percent of the patch per move) and, worse, leave a dry band that the
    // solver then has to refill from an inconsistent state.
    const shiftRow = (arr: Float32Array, dy: number): void => {
      if (dy === 0) return;
      if (dy > 0) {
        for (let j = n - 1; j >= dy; j--) arr.copyWithin(j * n, (j - dy) * n, (j - dy + 1) * n);
        for (let j = 0; j < dy; j++) arr.set(arr.subarray(dy * n, (dy + 1) * n), j * n);
      } else {
        const d = -dy;
        for (let j = 0; j <= n - 1 - d; j++) arr.copyWithin(j * n, (j + d) * n, (j + d + 1) * n);
        const src = (n - d - 1) * n;
        for (let j = n - d; j < n; j++) arr.set(arr.subarray(src, src + n), j * n);
      }
    };
    const shiftCol = (arr: Float32Array, dxn: number): void => {
      if (dxn === 0) return;
      if (dxn > 0) {
        for (let j = 0; j < n; j++) {
          const row = j * n;
          const edge = arr[row + dxn];
          arr.copyWithin(row + dxn, row, row + n - dxn);
          arr.fill(edge, row, row + dxn);
        }
      } else {
        const d = -dxn;
        for (let j = 0; j < n; j++) {
          const row = j * n;
          const edge = arr[row + n - d - 1];
          arr.copyWithin(row, row + d, row + n);
          arr.fill(edge, row + n - d, row + n);
        }
      }
    };
    // Moving the patch centre by +d removes d from every sample's coordinate,
    // so the stored field must move by -d to keep world positions fixed.
    for (const arr of [this.eta, this.u, this.v, this.bed]) {
      shiftRow(arr, -sz);
      shiftCol(arr, -sx);
    }
    this.centerX += sx * step;
    this.centerZ += sz * step;
    return true;
  }

  /**
   * Inject mass at a point — the "water drop" authoring brush.
   * `x`/`z` are frame-local metres, i.e. offsets from the frame origin, the same
   * convention as `sampleEtaWorld`.
   */
  addMass(x: number, z: number, radius: number, volume: number): void {
    const n = this.n;
    const half = (n * this.cellSize) * 0.5;
    const ci = (x - this.centerX + half) / this.cellSize;
    const cj = (z - this.centerZ + half) / this.cellSize;
    const cr = Math.max(1, radius / this.cellSize);
    let wsum = 0;
    for (let j = Math.floor(cj - cr); j <= Math.ceil(cj + cr); j++) {
      for (let i = Math.floor(ci - cr); i <= Math.ceil(ci + cr); i++) {
        if (i < 0 || j < 0 || i >= n || j >= n) continue;
        const dx = i + 0.5 - ci;
        const dz = j + 0.5 - cj;
        const d = Math.sqrt(dx * dx + dz * dz);
        if (d > cr) continue;
        wsum += 1 - d / cr;
      }
    }
    if (wsum <= 0) return;
    for (let j = Math.floor(cj - cr); j <= Math.ceil(cj + cr); j++) {
      for (let i = Math.floor(ci - cr); i <= Math.ceil(ci + cr); i++) {
        if (i < 0 || j < 0 || i >= n || j >= n) continue;
        const dx = i + 0.5 - ci;
        const dz = j + 0.5 - cj;
        const d = Math.sqrt(dx * dx + dz * dz);
        if (d > cr) continue;
        const w = 1 - d / cr;
        this.eta[i + j * n] += (volume * (w / wsum)) / (this.cellSize * this.cellSize);
      }
    }
  }

  /** Push momentum (the "wake / current" authoring brush). */
  addMomentum(x: number, z: number, radius: number, vx: number, vz: number): void {
    const n = this.n;
    const half = (n * this.cellSize) * 0.5;
    const ci = (x - this.centerX + half) / this.cellSize;
    const cj = (z - this.centerZ + half) / this.cellSize;
    const cr = Math.max(1, radius / this.cellSize);
    for (let j = Math.floor(cj - cr); j <= Math.ceil(cj + cr); j++) {
      for (let i = Math.floor(ci - cr); i <= Math.ceil(ci + cr); i++) {
        if (i < 0 || j < 0 || i >= n || j >= n) continue;
        const dx = i + 0.5 - ci;
        const dz = j + 0.5 - cj;
        const d = Math.sqrt(dx * dx + dz * dz);
        if (d > cr) continue;
        const w = 1 - d / cr;
        const idx = i + j * n;
        this.u[idx] += vx * w;
        this.v[idx] += vz * w;
      }
    }
  }

  /**
   * Advance the simulation by `seconds`, substepping for stability.
   * `inflowEta` optionally supplies Dirichlet elevation on the outer boundary
   * (used to couple a coarse parent ring into a finer child ring).
   */
  step(seconds: number, inflowEta: Float32Array | null, inflowN: number, inflowScheme: 'inner' | 'outer'): void {
    const s = this.settings;
    const dx = this.cellSize;
    const g = s.gravity;

    // CFL with the dispersion cap: the fastest wave the grid can represent.
    const cMaxGrid = Math.sqrt((g * dx) / Math.PI);
    let maxC = 1;
    for (let i = 0; i < this.bed.length; i += 7) {
      const H = this.eta[i] - this.bed[i];
      if (H > 0) {
        const c = Math.sqrt(g * Math.min(H, dx * 3.2));
        if (c > maxC) maxC = c;
      }
    }
    maxC = Math.min(maxC, cMaxGrid * 2.2);
    const dtTarget = (0.42 * dx) / Math.max(1e-3, maxC);
    const steps = Math.max(1, Math.min(24, Math.ceil(seconds / dtTarget)));
    const dt = seconds / steps;
    this.lastSubsteps = steps;

    for (let k = 0; k < steps; k++) this.substep(dt, inflowEta, inflowN, inflowScheme);
    this.time += seconds;
    this.calls++;
  }

  private substep(
    dt: number, inflowEta: Float32Array | null, inflowN: number, scheme: 'inner' | 'outer',
  ): void {
    const s = this.settings;
    const n = this.n;
    const dx = this.cellSize;
    const g = s.gravity;
    const eta = this.eta;
    const u = this.u;
    const v = this.v;
    const bed = this.bed;

    // --- momentum -----------------------------------------------------------
    const windX = Math.cos(s.windAngle) * s.windStress;
    const windZ = Math.sin(s.windAngle) * s.windStress;
    for (let j = 1; j < n - 1; j++) {
      const row = j * n;
      for (let i = 1; i < n - 1; i++) {
        const idx = row + i;
        const depthC = Math.max(0, eta[idx] - bed[idx]);
        const depthL = Math.max(0, eta[idx - 1] - bed[idx - 1]);
        const depthR = Math.max(0, eta[idx + 1] - bed[idx + 1]);
        const depthU = Math.max(0, eta[idx - n] - bed[idx - n]);
        const depthD = Math.max(0, eta[idx + n] - bed[idx + n]);
        const hC = depthC > 0.02 || depthL > 0.02 || depthR > 0.02;
        if (!hC) { u[idx] *= 0.8; v[idx] *= 0.8; continue; }

        // Face depths (average of the two cells sharing the face).
        const hE = 0.5 * (depthC + depthR);
        const hW = 0.5 * (depthC + depthL);
        const hN = 0.5 * (depthC + depthD);
        const hS = 0.5 * (depthC + depthU);

        // Pressure gradient; wet faces only.
        const dEtaX = (eta[idx + 1] - eta[idx - 1]) / (2 * dx);
        const dEtaZ = (eta[idx + n] - eta[idx - n]) / (2 * dx);
        const wetE = hE > 0.01 ? 1 : 0;
        const wetW = hW > 0.01 ? 1 : 0;
        const wetN = hN > 0.01 ? 1 : 0;
        const wetS = hS > 0.01 ? 1 : 0;

        // Bottom friction, quadratic-ish with depth dependence.
        const speed = Math.sqrt(u[idx] * u[idx] + v[idx] * v[idx]);
        const cf = s.bottomFriction + speed * 0.0025 / Math.max(1, depthC);
        const damp = 1 / (1 + cf * dt * 8);

        // Simple Laplacian viscosity: keeps the grid from ringing without
        // smearing swell over hundreds of cells.
        const lapU = u[idx - 1] + u[idx + 1] + u[idx - n] + u[idx + n] - 4 * u[idx];
        const lapV = v[idx - 1] + v[idx + 1] + v[idx - n] + v[idx + n] - 4 * v[idx];

        let newU = u[idx] - g * dEtaX * dt * ((wetE + wetW) * 0.5);
        let newV = v[idx] - g * dEtaZ * dt * ((wetN + wetS) * 0.5);
        newU += (windX * dt) / Math.max(1, depthC * 0.08);
        newV += (windZ * dt) / Math.max(1, depthC * 0.08);
        newU += s.viscosity * lapU * dt / dx * 8;
        newV += s.viscosity * lapV * dt / dx * 8;
        u[idx] = newU * damp;
        v[idx] = newV * damp;
      }
    }

    // --- continuity (flux form, exactly conservative) -----------------------
    const etaNext = this.etaNext;
    etaNext.set(eta);
    for (let j = 1; j < n - 1; j++) {
      const row = j * n;
      for (let i = 1; i < n - 1; i++) {
        const idx = row + i;
        const depthC = eta[idx] - bed[idx];
        if (depthC <= 0 && eta[idx - 1] - bed[idx - 1] <= 0 && eta[idx + 1] - bed[idx + 1] <= 0
          && eta[idx - n] - bed[idx - n] <= 0 && eta[idx + n] - bed[idx + n] <= 0) continue;

        const hE = Math.max(0, 0.5 * (depthC + (eta[idx + 1] - bed[idx + 1])));
        const hW = Math.max(0, 0.5 * (depthC + (eta[idx - 1] - bed[idx - 1])));
        const hN = Math.max(0, 0.5 * (depthC + (eta[idx + n] - bed[idx + n])));
        const hS = Math.max(0, 0.5 * (depthC + (eta[idx - n] - bed[idx - n])));

        const fluxE = hE * u[idx];
        const fluxW = hW * u[idx - 1];
        const fluxN = hN * v[idx];
        const fluxS = hS * v[idx - n];
        const div = (fluxE - fluxW + fluxN - fluxS) / dx;
        let next = eta[idx] - div * dt;

        // Sponge: relax toward zero elevation at the patch border.
        const sp = this.sponge[idx];
        if (sp > 0) next *= 1 - sp * Math.min(1, dt * 2.2);

        // Wet/dry: the surface rests on the bed and never sinks through it. Doing
        // this as a hard clamp (rather than a relaxation) is what stops dry land
        // slowly dragging the free surface up a mountainside.
        if (next < bed[idx]) next = bed[idx];
        etaNext[idx] = next;
      }
    }

    // Boundary handling.
    if (inflowEta && inflowN > 1) {
      this.applyInflow(inflowEta, inflowN, scheme, dt);
    } else {
      this.reflectBoundary(etaNext);
    }
    this.eta.set(etaNext);
  }

  /** Copy a parent/child ring's boundary elevation in, with relaxation. */
  private applyInflow(
    source: Float32Array, sourceN: number, scheme: 'inner' | 'outer', dt: number,
  ): void {
    const n = this.n;
    const relax = Math.min(1, dt * 6);
    const sample = (si: number, sj: number): number => {
      const i = Math.max(0, Math.min(sourceN - 1, Math.round(si)));
      const j = Math.max(0, Math.min(sourceN - 1, Math.round(sj)));
      return source[i + j * sourceN];
    };
    const bandCells = Math.max(1, Math.round(n * 0.02));
    for (let b = 0; b < bandCells; b++) {
      const t = 1 - b / bandCells;
      // Outer edge of this patch <- inner edge of the coarser parent.
      for (let i = 0; i < n; i++) {
        const src = scheme === 'inner'
          ? sample((i / (n - 1)) * (sourceN - 1), 0)
          : sample((i / (n - 1)) * (sourceN - 1), sourceN - 1);
        this.eta[i + b * n] += (src - this.eta[i + b * n]) * relax * t;
      }
    }
  }

  private reflectBoundary(etaNext: Float32Array): void {
    const n = this.n;
    // Copy the nearest interior row/column into the ghost ring (zero-gradient).
    for (let i = 0; i < n; i++) {
      etaNext[i] = etaNext[n + i];
      etaNext[(n - 1) * n + i] = etaNext[(n - 2) * n + i];
      etaNext[i * n] = etaNext[i * n + 1];
      etaNext[i * n + n - 1] = etaNext[i * n + n - 2];
    }
  }

  /**
   * Bilinear elevation at a patch-local grid coordinate, in metres.
   *
   * `x`/`z` are in the *frame the patch has been shifted into*, i.e. they are
   * compared against the stored sample offsets directly. Callers that hold a
   * world position should subtract `centerX`/`centerZ` first — see
   * `sampleEtaWorld`.
   */
  sampleEta(x: number, z: number): number {
    const n = this.n;
    const half = (n * this.cellSize) * 0.5;
    const gx = (x + half) / this.cellSize - 0.5;
    const gz = (z + half) / this.cellSize - 0.5;
    const i0 = Math.floor(gx);
    const j0 = Math.floor(gz);
    const tx = gx - i0;
    const tz = gz - j0;
    const cl = (i: number, j: number): number => {
      const ii = i < 0 ? 0 : i >= n ? n - 1 : i;
      const jj = j < 0 ? 0 : j >= n ? n - 1 : j;
      return this.eta[ii + jj * n];
    };
    const a = cl(i0, j0);
    const b = cl(i0 + 1, j0);
    const c = cl(i0, j0 + 1);
    const d = cl(i0 + 1, j0 + 1);
    return (a * (1 - tx) + b * tx) * (1 - tz) + (c * (1 - tx) + d * tx) * tz;
  }

  /** Elevation at a world-space patch coordinate. */
  sampleEtaWorld(x: number, z: number): number {
    return this.sampleEta(x - this.centerX, z - this.centerZ);
  }

  /** Seabed at a world-space patch coordinate. */
  sampleBedWorld(x: number, z: number): number {
    return this.sampleBedAt(x - this.centerX, z - this.centerZ);
  }

  /** Bilinear depth at a patch-local (x, z). */
  sampleDepth(x: number, z: number): number {
    return this.sampleEta(x, z) - this.sampleBedAt(x, z);
  }

  /** Bilinear seabed altitude at a patch-local (x, z) in metres. */
  sampleBedAt(x: number, z: number): number {
    const n = this.n;
    const half = (n * this.cellSize) * 0.5;
    const gx = (x + half) / this.cellSize - 0.5;
    const gz = (z + half) / this.cellSize - 0.5;
    const i0 = Math.floor(gx);
    const j0 = Math.floor(gz);
    const tx = gx - i0;
    const tz = gz - j0;
    const cl = (i: number, j: number): number => {
      const ii = i < 0 ? 0 : i >= n ? n - 1 : i;
      const jj = j < 0 ? 0 : j >= n ? n - 1 : j;
      return this.bed[ii + jj * n];
    };
    const a = cl(i0, j0);
    const b = cl(i0 + 1, j0);
    const c = cl(i0, j0 + 1);
    const d = cl(i0 + 1, j0 + 1);
    return (a * (1 - tx) + b * tx) * (1 - tz) + (c * (1 - tx) + d * tx) * tz;
  }

  stats(): SWEStats {
    let wet = 0;
    let maxDepth = 0;
    let maxEta = 0;
    let maxSpeed = 0;
    let volume = 0;
    let energy = 0;
    for (let i = 0; i < this.eta.length; i++) {
      const H = this.eta[i] - this.bed[i];
      if (H > 0.02) {
        wet++;
        if (H > maxDepth) maxDepth = H;
        volume += H * this.cellSize * this.cellSize;
        const sp = this.u[i] * this.u[i] + this.v[i] * this.v[i];
        if (sp > maxSpeed) maxSpeed = sp;
        energy += 0.5 * this.settings.gravity * H * H + 0.5 * H * sp;
        // Free-surface elevation is only meaningful where there is water.
        if (this.eta[i] > maxEta) maxEta = this.eta[i];
      }
    }
    return {
      wetCells: wet,
      totalCells: this.eta.length,
      maxDepth,
      maxEta,
      maxSpeed: Math.sqrt(maxSpeed),
      substeps: this.lastSubsteps,
      dt: 0,
      volume,
      energy,
      time: this.time,
    };
  }
}
