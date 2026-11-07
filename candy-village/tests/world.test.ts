import { describe, expect, it } from 'vitest';
import { Scene } from 'three';
import { generateLayout, PLAZA_R } from '../src/world/layout';
import { NavGrid } from '../src/world/navgrid';
import { buildWorld } from '../src/world/world';
import { scoreForDistance, createTarget } from '../src/world/targets';
import type { ImpactEvent, GuardState } from '../src/guard/guardController';

const SEEDS = [1, 42, 20241224];

describe('layout invariants', () => {
  for (const seed of SEEDS) {
    it(`seed ${seed}: houses, lamps, spawns, forest respect the design`, () => {
      const L = generateLayout(seed);
      expect(L.houses.length).toBeGreaterThanOrEqual(8);
      for (const h of L.houses) {
        const r = Math.hypot(h.x, h.z);
        expect(r).toBeGreaterThanOrEqual(PLAZA_R);
        expect(r).toBeLessThanOrEqual(40);
        expect(L.inRiver(h.x, h.z, 0)).toBe(false);
      }
      for (const p of [...L.lamps, ...L.guardSpawns]) expect(L.inRiver(p.x, p.z, 0)).toBe(false);
      for (const f of L.forest) {
        expect(Math.hypot(f.x, f.z)).toBeGreaterThanOrEqual(44);
        expect(L.inRiver(f.x, f.z, 0)).toBe(false);
      }
      expect(Math.hypot(L.gingerbread.x - -44, L.gingerbread.z - -28)).toBeLessThan(6);
      expect(L.riverCenter(0)).toBeCloseTo(3 * Math.sin(1.3) - 8, 5);
      for (let x = -100; x <= 100; x += 7) {
        for (let z = -100; z <= 100; z += 7) {
          expect(Number.isFinite(L.heightAt(x, z))).toBe(true);
        }
      }
    });
  }
});

describe('navigation', () => {
  it('paths exist from every guard spawn to the plaza and the gingerbread house', () => {
    const L = generateLayout(42);
    const nav = new NavGrid(L);
    for (const s of L.guardSpawns) {
      expect(nav.findPath(s, { x: 0, z: 12 })).not.toBeNull();
      expect(nav.findPath(s, { x: L.gingerbread.x, z: L.gingerbread.z })).not.toBeNull();
    }
  });
});

describe('scoring', () => {
  it('awards 3/2/1 rings within radius 1 and nothing outside', () => {
    expect(scoreForDistance(0.2)).toBe(3);
    expect(scoreForDistance(0.5)).toBe(2);
    expect(scoreForDistance(0.9)).toBe(1);
    expect(scoreForDistance(1.01)).toBe(0);
  });
  it('target objects construct with a radius of 1 m', () => {
    const t = createTarget(1, 3, 4, 0);
    expect(t.radius).toBe(1);
    t.pulse();
    expect(t.hits).toBe(1);
  });
});

describe('guard strike cycle', () => {
  it('walks, winds up, strikes and recovers, reporting an impact', () => {
    const scene = new Scene();
    const world = buildWorld(scene, 42);
    const guard = world.guards[0];
    const target = createTarget(1, guard.pos.x + 6, guard.pos.z + 3, world.layout.heightAt(guard.pos.x + 6, guard.pos.z + 3));
    guard.setAccuracyError(0);
    const impacts: ImpactEvent[] = [];
    guard.onImpact = (e) => impacts.push(e);
    expect(guard.order(target, world.nav)).toBe(true);

    const seen = new Set<GuardState>();
    const dt = 1 / 60;
    let t = 0;
    for (let i = 0; i < 60 * 40 && impacts.length === 0; i++) {
      t += dt;
      guard.update(dt, t, (x, z) => world.layout.heightAt(x, z));
      seen.add(guard.state);
      for (const v of [guard.tip.x, guard.tip.y, guard.tip.z, guard.pos.x, guard.pos.z]) {
        if (!Number.isFinite(v)) throw new Error(`non-finite state at t=${t} (${guard.state})`);
      }
    }
    expect(seen.has('walk')).toBe(true);
    expect(seen.has('windup')).toBe(true);
    expect(seen.has('strike')).toBe(true);
    expect(impacts.length).toBe(1);
    const tip = impacts[0].tip;
    const d = Math.hypot(tip.x - target.x, tip.z - target.z);
    console.log(`[guard] impact ${d.toFixed(2)} m from centre, tip y ${tip.y.toFixed(2)}`);
    expect(d).toBeLessThan(3);
    // Recover then return to idle.
    for (let i = 0; i < 60 * 5 && guard.state !== 'idle'; i++) {
      t += dt;
      guard.update(dt, t, (x, z) => world.layout.heightAt(x, z));
    }
    expect(guard.state).toBe('idle');
  });
});
