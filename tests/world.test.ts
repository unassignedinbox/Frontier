import { describe, expect, it } from 'vitest';
import {
  createAfricaPreset, createFlatWorld, createRandomContinent, SdfWorld,
  type BrushSettings,
} from '../src/world/SdfWorld';

const brush: BrushSettings = { radiusKm: 50, strength: 500, target: 20, feather: 0.3 };

describe('SDF world model', () => {
  it('starts as a flat ocean plate and keeps the 3D field sign correct', () => {
    const world = new SdfWorld(createFlatWorld());
    const h = world.heightAt(0, 0);
    expect(world.landSdf(0, 0)).toBeGreaterThan(0);
    expect(h).toBeLessThan(world.seaLevel);
    expect(world.sdf3D(0, h + 10, 0)).toBeGreaterThan(0); // air/water above earth
    expect(world.sdf3D(0, h - 10, 0)).toBeLessThan(0);    // inside earth
    expect(world.sdf3D(0, -7000, 0)).toBeGreaterThan(0);  // below bottom cap
  });

  it('uses CSG island strokes to change both the land SDF and derived heightmap', () => {
    const world = new SdfWorld(createFlatWorld());
    world.beginStroke('island', 0, 0, brush);
    expect(world.landSdf(0, 0)).toBeLessThan(0);
    expect(world.heightAt(0, 0)).toBeGreaterThan(world.seaLevel);
    world.beginStroke('gulf', 0, 0, brush);
    expect(world.landSdf(0, 0)).toBeGreaterThan(0);
  });

  it('keeps the Africa blockout bounded and deterministic', () => {
    const world = new SdfWorld(createAfricaPreset());
    for (const [x, z] of [[0, 0], [-2e6, 1e6], [2.5e6, -1e6], [4e6, 4e6]] as [number, number][]) {
      const h = world.heightAt(x, z);
      const d = world.landSdf(x, z);
      expect(Number.isFinite(h)).toBe(true);
      expect(Number.isFinite(d)).toBe(true);
      expect(h).toBeGreaterThan(-6500);
      expect(h).toBeLessThan(8000);
    }
    expect(world.heightAt(123456, -234567)).toBe(world.heightAt(123456, -234567));
  });

  it('serialises the editable SDF definition without losing features', () => {
    const world = new SdfWorld(createRandomContinent(4242));
    const json = world.toJSON();
    const restored = SdfWorld.fromJSON(JSON.parse(JSON.stringify(json)));
    expect(restored.def.features.length).toBe(world.def.features.length);
    expect(restored.heightAt(0, 0)).toBeCloseTo(world.heightAt(0, 0), 6);
  });
});
