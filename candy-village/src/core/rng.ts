/**
 * Deterministic random number generation.
 *
 * Every procedural decision in the world (layout, tree placement, colours,
 * guard flavours) flows from a single integer seed, so a given seed always
 * rebuilds the exact same village.
 */

/** 32-bit integer hash (Thomas Wang style). Deterministic, cheap. */
export function hashInt(x: number): number {
  let h = x | 0;
  h = Math.imul(h ^ (h >>> 16), 0x7feb352d);
  h = Math.imul(h ^ (h >>> 15), 0x846ca68b);
  h ^= h >>> 16;
  return h >>> 0;
}

/** Hash a pair of integers to a float in [0, 1). */
export function hash2f(x: number, y: number, seed = 0): number {
  return hashInt(hashInt(x * 374761393 + seed) ^ (y * 668265263)) / 4294967296;
}

export class RNG {
  private state: number;

  constructor(seed: number) {
    this.state = hashInt(seed | 0) || 1;
  }

  /** mulberry32: float in [0, 1). */
  next(): number {
    this.state = (this.state + 0x6d2b79f5) >>> 0;
    let t = this.state;
    t = Math.imul(t ^ (t >>> 15), t | 1);
    t ^= t + Math.imul(t ^ (t >>> 7), t | 61);
    return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
  }

  /** Uniform float in [a, b). */
  range(a: number, b: number): number {
    return a + (b - a) * this.next();
  }

  /** Uniform integer in [a, b] (inclusive). */
  int(a: number, b: number): number {
    return Math.min(b, Math.floor(a + (b - a + 1) * this.next()));
  }

  chance(p: number): boolean {
    return this.next() < p;
  }

  pick<T>(items: readonly T[]): T {
    if (items.length === 0) throw new Error('RNG.pick: empty array');
    return items[Math.floor(this.next() * items.length)];
  }

  /** Normally distributed sample (Box-Muller). */
  gauss(sigma = 1): number {
    const u = Math.max(1e-12, this.next());
    const v = this.next();
    return sigma * Math.sqrt(-2 * Math.log(u)) * Math.cos(2 * Math.PI * v);
  }

  /** Derive an independent generator for a sub-system. */
  fork(salt: number): RNG {
    return new RNG(hashInt(this.state ^ hashInt(salt)));
  }
}
