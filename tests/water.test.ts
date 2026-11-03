import { describe, expect, it } from 'vitest';
import { DEFAULT_WATER_SETTINGS, ShallowWaterSolver, type WaterSettings } from '../src/ocean/ShallowWaterSolver';
import { createAfricaPreset, createFlatWorld, SdfWorld } from '../src/world/SdfWorld';

function closedSettings(overrides: Partial<WaterSettings> = {}): WaterSettings {
  return {
    ...DEFAULT_WATER_SETTINGS,
    gridSize: 64,
    cellSize: 2,
    farLevels: 4,
    gravity: 9.81,
    damping: 0,
    wind: 0,
    waveMaker: 0,
    sponge: 0,
    waveHeight: 0,
    simSpeed: 1,
    paused: false,
    ...overrides,
  };
}

describe('2.5D shallow-water solver', () => {
  it('conserves volume with reflective boundaries and no forcing', () => {
    const world = new SdfWorld(createFlatWorld());
    const settings = closedSettings();
    const solver = new ShallowWaterSolver(64, 2, world, 0, 0);
    solver.addDrop(0, 0, 18, 0.8);
    const before = solver.stats().mass;
    for (let i = 0; i < 30; i++) solver.advance(1 / 60, settings, world);
    const after = solver.stats().mass;
    expect(Math.abs(after - before) / before).toBeLessThan(1e-7);
  });

  it('adapts substep count to the gravity-wave CFL limit', () => {
    const world = new SdfWorld(createAfricaPreset());
    const settings = closedSettings({ cellSize: 8 });
    const solver = new ShallowWaterSolver(64, 8, world, 6_000_000, 0);
    solver.advance(0.05, settings, world);
    expect(solver.stats().maxDepth).toBeGreaterThan(3000);
    expect(solver.lastSubsteps).toBeGreaterThanOrEqual(2);
    expect(solver.lastStableDt).toBeLessThan(0.03);
  });

  it('respects wet/dry SDF bathymetry when a drop lands on terrain', () => {
    const world = new SdfWorld(createFlatWorld());
    world.beginStroke('raise', 0, 0, { radiusKm: 0.12, strength: 600, target: 0, feather: 0.2 });
    const solver = new ShallowWaterSolver(32, 4, world, 0, 0);
    const k = 15 + 15 * 32;
    expect(solver.bed[k]).toBeGreaterThan(solver.seaLevel);
    solver.addDrop(0, 0, 20, 2.0);
    expect(solver.eta[k]).toBeLessThanOrEqual(solver.bed[k] - solver.seaLevel + 1e-9);
    expect(solver.stats().wetCells).toBeLessThan(solver.stats().totalCells);
  });

  it('preserves surface state when the simulated clipmap is shifted by whole cells', () => {
    const world = new SdfWorld(createFlatWorld());
    const settings = closedSettings({ cellSize: 4 });
    void settings;
    const solver = new ShallowWaterSolver(48, 4, world, 0, 0);
    solver.addDrop(0, 0, 16, 1.2);
    const before = solver.sampleEta(0, 0);
    solver.shift(1, 0, world, 4, 0);
    const after = solver.sampleEta(0, 0);
    expect(after).toBeCloseTo(before, 3);
    expect(solver.centerX).toBe(4);
  });
});
