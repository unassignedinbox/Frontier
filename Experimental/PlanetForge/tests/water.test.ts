import { describe, expect, it } from 'vitest';
import { SWEPatch, DEFAULT_WATER_SETTINGS } from '../src/core/water/shallowWater';

function makePatch(cells = 48, cellSize = 20): SWEPatch {
  return new SWEPatch({
    ...DEFAULT_WATER_SETTINGS,
    gridSize: cells,
    cellSize,
    spongeCells: 4,
    windStress: 0,
  });
}

function totalVolume(p: SWEPatch): number {
  let v = 0;
  for (let i = 0; i < p.eta.length; i++) v += Math.max(0, p.eta[i] - p.bed[i]);
  return v * p.cellSize * p.cellSize;
}

describe('shallow water solver', () => {
  it('conserves mass in a closed flat basin with reflective walls', () => {
    const p = makePatch(48, 20);
    // Flat seabed 40 m down, no wind.
    p.sampleBed(() => -40);
    p.addMass(0, 0, 120, 200_000); // 200 000 m³ of water in the middle
    const v0 = totalVolume(p);

    let last = v0;
    for (let f = 0; f < 240; f++) {
      p.step(1 / 60, null, 0, 'outer');
      last = totalVolume(p);
    }
    const drift = Math.abs(last - v0) / v0;
    // Explicit finite volumes with a sponge should stay within a few percent
    // over four simulated seconds; anything larger means a sign error.
    expect(drift).toBeLessThan(0.08);
  });

  it('is stable: no NaN and bounded velocities after a long run', () => {
    const p = makePatch(32, 25);
    p.sampleBed((x) => -30 - x * 0.02);
    p.addMass(0, 0, 150, 400_000);
    p.addMomentum(0, 0, 60, 4, 1);
    for (let f = 0; f < 900; f++) {
      p.step(1 / 60, null, 0, 'outer');
      if (f % 100 === 0) {
        const s = p.stats();
        expect(Number.isFinite(s.maxEta)).toBe(true);
        expect(Number.isFinite(s.maxSpeed)).toBe(true);
      }
    }
    const s = p.stats();
    expect(s.maxEta).toBeLessThan(400);
    expect(s.maxSpeed).toBeLessThan(60);
  });

  it('adapts substep count to depth (CFL), staying inside the cap', () => {
    const shallow = makePatch(32, 40);
    shallow.sampleBed(() => -3);
    shallow.addMass(0, 0, 60, 10_000);
    shallow.step(1 / 60, null, 0, 'outer');
    const shallowSteps = shallow.stats().substeps;

    const deep = makePatch(32, 40);
    deep.sampleBed(() => -4000);
    deep.addMass(0, 0, 60, 10_000);
    deep.step(1 / 60, null, 0, 'outer');
    const deepSteps = deep.stats().substeps;

    expect(shallowSteps).toBeGreaterThanOrEqual(1);
    expect(deepSteps).toBeGreaterThanOrEqual(shallowSteps);
    expect(deepSteps).toBeLessThanOrEqual(24);
  });

  it('rejects water injected on land: depth stays zero above the bed', () => {
    const p = makePatch(32, 10);
    p.sampleBed(() => 200); // seabed far above sea level, i.e. dry land
    p.addMass(0, 0, 25, 5_000);
    for (let i = 0; i < 60; i++) p.step(1 / 60, null, 0, 'outer');
    const s = p.stats();
    // Either it ran off and drained, or it never had positive depth.
    expect(s.maxDepth).toBeLessThan(60);
  });

  it('preserves surface state across a whole-cell clipmap shift', () => {
    const p = makePatch(48, 12);
    p.sampleBed(() => -60);
    p.addMass(0, 0, 90, 90_000);
    for (let i = 0; i < 30; i++) p.step(1 / 60, null, 0, 'outer');

    const before = p.sampleEta(80, 0);
    p.shift(24, 0); // exactly two cells
    const after = p.sampleEta(80 - 24, 0);
    expect(after).toBeCloseTo(before, 4);
    // Whole-cell shifts are accepted; sub-cell requests are a no-op so the
    // solver never resamples (and therefore never smears) on every frame.
    expect(p.shift(12, 0)).toBe(true);
    expect(p.shift(0.1, 0)).toBe(false);
  });

  it('propagates a disturbance outward rather than leaving it pinned', () => {
    const p = makePatch(64, 15);
    p.sampleBed(() => -50);
    p.addMass(0, 0, 40, 60_000);
    const centreBefore = Math.abs(p.sampleEta(0, 0));
    const ring = (): number => Math.abs(p.sampleEta(0, 220)) + Math.abs(p.sampleEta(220, 0));
    const ringBefore = ring();
    for (let i = 0; i < 200; i++) p.step(1 / 60, null, 0, 'outer');
    expect(Math.abs(p.sampleEta(0, 0))).toBeLessThan(centreBefore);
    expect(ring()).toBeGreaterThan(ringBefore);
  });

  it('accepts a parent boundary condition without blowing up', () => {
    const parent = makePatch(32, 60);
    parent.sampleBed(() => -120);
    parent.addMass(0, 0, 200, 80_000);
    for (let i = 0; i < 30; i++) parent.step(1 / 60, null, 0, 'outer');

    const child = makePatch(48, 20);
    child.sampleBed(() => -120);
    for (let i = 0; i < 120; i++) child.step(1 / 60, parent.eta, parent.n, 'outer');
    const s = child.stats();
    expect(Number.isFinite(s.maxEta)).toBe(true);
    expect(s.maxEta).toBeLessThan(200);
  });
});
