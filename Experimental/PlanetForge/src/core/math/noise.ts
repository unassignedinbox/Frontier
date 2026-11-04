/**
 * Deterministic 3D gradient + value noise.
 *
 * Everything in the terrain pipeline must be a pure function of position and
 * seed: chunk meshes are built in workers in arbitrary order, cached on disk and
 * reused across sessions, so any hidden global state would produce seams.
 */

const F = 4294967296;

/** Integer hash -> [0,1). Fast, deterministic, seedable. */
export function hash3i(x: number, y: number, z: number, seed: number): number {
  let h = seed | 0;
  h = (Math.imul(h ^ (x | 0), 0x27d4eb2d) + 0x9e3779b9) | 0;
  h = (Math.imul(h ^ (y | 0), 0x165667b1) + 0x85ebca6b) | 0;
  h = (Math.imul(h ^ (z | 0), 0x9e3779b9) + 0xc2b2ae35) | 0;
  h ^= h >>> 15;
  h = Math.imul(h, 0x2545f491) | 0;
  h ^= h >>> 13;
  return (h >>> 0) / F;
}

/** Three decorrelated hashes in [0,1) from one lattice point. */
export function hash3v(x: number, y: number, z: number, seed: number): [number, number, number] {
  const a = hash3i(x, y, z, seed);
  const b = hash3i(x, y, z, seed ^ 0x5f356495);
  const c = hash3i(x, y, z, seed ^ 0x1b873593);
  return [a, b, c];
}

const fade = (t: number): number => t * t * t * (t * (t * 6 - 15) + 10);

/** Trilinear value noise in [0,1). */
export function valueNoise3(x: number, y: number, z: number, seed: number): number {
  const xi = Math.floor(x), yi = Math.floor(y), zi = Math.floor(z);
  const xf = fade(x - xi), yf = fade(y - yi), zf = fade(z - zi);
  const c000 = hash3i(xi, yi, zi, seed);
  const c100 = hash3i(xi + 1, yi, zi, seed);
  const c010 = hash3i(xi, yi + 1, zi, seed);
  const c110 = hash3i(xi + 1, yi + 1, zi, seed);
  const c001 = hash3i(xi, yi, zi + 1, seed);
  const c101 = hash3i(xi + 1, yi, zi + 1, seed);
  const c011 = hash3i(xi, yi + 1, zi + 1, seed);
  const c111 = hash3i(xi + 1, yi + 1, zi + 1, seed);
  const x00 = c000 + (c100 - c000) * xf;
  const x10 = c010 + (c110 - c010) * xf;
  const x01 = c001 + (c101 - c001) * xf;
  const x11 = c011 + (c111 - c011) * xf;
  const y0 = x00 + (x10 - x00) * yf;
  const y1 = x01 + (x11 - x01) * yf;
  return y0 + (y1 - y0) * zf;
}

/**
 * Interleaved gradient noise — the cheapest useful dither, used for stochastic
 * sampling in the ocean/wake solvers and for anti-banding in strata shading.
 */
export function gradientNoise(x: number, y: number): number {
  const f = 0.06711056 * x + 0.00583715 * y;
  return f - Math.floor(f);
}

/** Fractal Brownian motion, output in [-1,1]. */
export function fbm3(
  x: number, y: number, z: number,
  seed: number, octaves = 4, lacunarity = 2.0, gain = 0.5,
): number {
  let amp = 1, freq = 1, sum = 0, norm = 0;
  for (let o = 0; o < octaves; o++) {
    sum += (valueNoise3(x * freq, y * freq, z * freq, seed + o * 1013) * 2 - 1) * amp;
    norm += amp;
    amp *= gain;
    freq *= lacunarity;
  }
  return sum / norm;
}

/** Ridged multifractal, output in [0,1]: sharp crests, useful for mountain belts. */
export function ridged3(
  x: number, y: number, z: number,
  seed: number, octaves = 5, lacunarity = 2.05, gain = 0.5,
): number {
  let amp = 1, freq = 1, sum = 0, norm = 0, prev = 1;
  for (let o = 0; o < octaves; o++) {
    let n = 1 - Math.abs(valueNoise3(x * freq, y * freq, z * freq, seed + o * 7717) * 2 - 1);
    n *= n;
    n *= prev;
    prev = n;
    sum += n * amp;
    norm += amp;
    amp *= gain;
    freq *= lacunarity;
  }
  return sum / norm;
}

/** Billowy noise, output in [0,1] — rounded hills, dunes, sediment lobes. */
export function billow3(
  x: number, y: number, z: number, seed: number, octaves = 4,
): number {
  let amp = 1, freq = 1, sum = 0, norm = 0;
  for (let o = 0; o < octaves; o++) {
    sum += Math.abs(valueNoise3(x * freq, y * freq, z * freq, seed + o * 3121) * 2 - 1) * amp;
    norm += amp;
    amp *= 0.5;
    freq *= 2.03;
  }
  return sum / norm;
}

/**
 * Domain warp: offsets a sample by a vector-valued low-frequency noise field.
 * This is what turns blobby union'd primitives into believable coastlines.
 */
export function warp3(
  x: number, y: number, z: number, seed: number,
  amplitude: number, frequency: number,
): [number, number, number] {
  const wx = fbm3(x * frequency, y * frequency, z * frequency, seed + 11, 3);
  const wy = fbm3(x * frequency + 5.2, y * frequency - 1.3, z * frequency + 2.7, seed + 12, 3);
  const wz = fbm3(x * frequency - 3.1, y * frequency + 4.4, z * frequency - 0.8, seed + 13, 3);
  return [x + wx * amplitude, y + wy * amplitude, z + wz * amplitude];
}

/** Derive a stable sub-seed for a region/node so edits stay local. */
export function seedFor(...parts: (number | string)[]): number {
  let h = 0x811c9dc5;
  for (const p of parts) {
    const s = typeof p === 'number' ? p.toString(36) : p;
    for (let i = 0; i < s.length; i++) {
      h ^= s.charCodeAt(i);
      h = Math.imul(h, 0x01000193) | 0;
    }
  }
  return h >>> 0;
}
