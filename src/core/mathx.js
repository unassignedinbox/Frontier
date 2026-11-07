/**
 * mathx — dependency-free scalar math + seeded noise.
 * Pure ES module so the sim and its unit tests share exactly one source of truth.
 */

export const TAU = Math.PI * 2;

export const clamp = (v, lo, hi) => (v < lo ? lo : v > hi ? hi : v);
export const clamp01 = (v) => (v < 0 ? 0 : v > 1 ? 1 : v);
export const lerp = (a, b, t) => a + (b - a) * t;
export const invLerp = (a, b, v) => (b === a ? 0 : (v - a) / (b - a));

export function smoothstep(edge0, edge1, x) {
  const t = clamp01((x - edge0) / (edge1 - edge0 || 1e-6));
  return t * t * (3 - 2 * t);
}

/** Frame-rate independent exponential approach. */
export function damp(a, b, lambda, dt) {
  return lerp(a, b, 1 - Math.exp(-lambda * dt));
}

/** Move `a` toward `b` by at most `maxDelta`. */
export function approach(a, b, maxDelta) {
  const d = b - a;
  if (Math.abs(d) <= maxDelta) return b;
  return a + Math.sign(d) * maxDelta;
}

/** Wrap an angle into (-PI, PI]. */
export function wrapPi(a) {
  a %= TAU;
  if (a > Math.PI) a -= TAU;
  if (a < -Math.PI) a += TAU;
  return a;
}

/** Shortest signed angular distance from a to b. */
export function angleDelta(a, b) {
  return wrapPi(b - a);
}

/** Shortest-path angular damp. */
export function dampAngle(a, b, lambda, dt) {
  return a + angleDelta(a, b) * (1 - Math.exp(-lambda * dt));
}

export const signOrZero = (v) => (v > 0 ? 1 : v < 0 ? -1 : 0);

// ---------------------------------------------------------------------------
// Deterministic RNG — course generation must be reproducible from a seed.
// ---------------------------------------------------------------------------

/** mulberry32: tiny, fast, good enough for procedural layout. */
export function mulberry32(seed) {
  let a = seed >>> 0;
  return function () {
    a = (a + 0x6d2b79f5) >>> 0;
    let t = a;
    t = Math.imul(t ^ (t >>> 15), t | 1);
    t ^= t + Math.imul(t ^ (t >>> 7), t | 61);
    return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
  };
}

export class Rng {
  constructor(seed = 1) {
    this.seed = seed >>> 0;
    this._next = mulberry32(this.seed);
  }
  next() { return this._next(); }
  range(lo, hi) { return lo + (hi - lo) * this._next(); }
  int(lo, hi) { return Math.floor(this.range(lo, hi + 1)); }
  pick(arr) { return arr[Math.min(arr.length - 1, Math.floor(this._next() * arr.length))]; }
  chance(p) { return this._next() < p; }
  sign() { return this._next() < 0.5 ? -1 : 1; }
  /** Approximate normal via sum of uniforms. */
  gauss(mean = 0, sd = 1) {
    return mean + (this._next() + this._next() + this._next() - 1.5) * (sd * 1.1547);
  }
}

// ---------------------------------------------------------------------------
// Value noise built on a seeded permutation table.
// ---------------------------------------------------------------------------

const fade = (t) => t * t * t * (t * (t * 6 - 15) + 10);

export class Noise {
  constructor(seed = 1) {
    const rng = new Rng(seed);
    const p = new Uint8Array(256);
    for (let i = 0; i < 256; i++) p[i] = i;
    for (let i = 255; i > 0; i--) {
      const j = Math.floor(rng.next() * (i + 1));
      const t = p[i]; p[i] = p[j]; p[j] = t;
    }
    this.perm = new Uint8Array(512);
    for (let i = 0; i < 512; i++) this.perm[i] = p[i & 255];
  }

  /** 1D value noise in [-1, 1]. */
  n1(x) {
    const xi = Math.floor(x);
    const xf = x - xi;
    const a = this.perm[xi & 255] / 127.5 - 1;
    const b = this.perm[(xi + 1) & 255] / 127.5 - 1;
    return lerp(a, b, fade(xf));
  }

  /** 2D value noise in [-1, 1]. */
  n2(x, y) {
    const xi = Math.floor(x), yi = Math.floor(y);
    const xf = x - xi, yf = y - yi;
    const X = xi & 255, Y = yi & 255;
    const p = this.perm;
    const aa = p[p[X] + Y] / 127.5 - 1;
    const ba = p[p[X + 1] + Y] / 127.5 - 1;
    const ab = p[p[X] + Y + 1] / 127.5 - 1;
    const bb = p[p[X + 1] + Y + 1] / 127.5 - 1;
    const u = fade(xf), v = fade(yf);
    return lerp(lerp(aa, ba, u), lerp(ab, bb, u), v);
  }

  /** Fractal Brownian motion, 2D. */
  fbm2(x, y, octaves = 4, lacunarity = 2.0, gain = 0.5) {
    let amp = 1, freq = 1, sum = 0, norm = 0;
    for (let i = 0; i < octaves; i++) {
      sum += amp * this.n2(x * freq, y * freq);
      norm += amp;
      amp *= gain;
      freq *= lacunarity;
    }
    return sum / (norm || 1);
  }

  /** FBM along a single axis — used for the corridor spline wander. */
  fbm1(x, octaves = 4, lacunarity = 2.0, gain = 0.5) {
    let amp = 1, freq = 1, sum = 0, norm = 0;
    for (let i = 0; i < octaves; i++) {
      sum += amp * this.n1(x * freq);
      norm += amp;
      amp *= gain;
      freq *= lacunarity;
    }
    return sum / (norm || 1);
  }
}

/** Signed angle between two 3D directions around an up axis (radians). */
export function signedAngleXZ(fx, fz, tx, tz) {
  const dot = fx * tx + fz * tz;
  const cross = fx * tz - fz * tx;
  return Math.atan2(cross, dot);
}
