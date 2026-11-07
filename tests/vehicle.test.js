/**
 * Hover flight model. Exercises the real `Vehicle` from src/sim/vehicle.js.
 */
import { test } from 'node:test';
import assert from 'node:assert/strict';
import { CFG } from '../src/config.js';
import * as THREE from 'three';
import { Vehicle } from '../src/sim/vehicle.js';

const DT = 1 / 120;
const NEUTRAL = { steer: 0, pitch: 0, throttle: 1, brake: 0, boost: false, drift: false };

function makeVehicle(overrides = {}) {
  const cfg = structuredClone(CFG);
  Object.assign(cfg.craft, overrides);
  const v = new Vehicle(cfg, { terrain: () => 0 });
  v.reset(new THREE.Vector3(0, 200, 0), 0);
  return v;
}

function run(v, seconds, cmd = NEUTRAL) {
  const n = Math.round(seconds / DT);
  for (let i = 0; i < n; i++) v.update(DT, cmd);
}

test('thrust accelerates the craft and drag caps it near topSpeed', () => {
  const v = makeVehicle();
  run(v, 1);
  assert.ok(v.speed > 20, `after 1s only ${v.speed.toFixed(1)} m/s`);
  run(v, 25);
  assert.ok(v.speed > CFG.craft.topSpeed * 0.9,
    `never reached cruise: ${v.speed.toFixed(1)} of ${CFG.craft.topSpeed}`);
  assert.ok(v.speed <= CFG.craft.topSpeed + 1e-6, 'exceeded the hard speed cap');
});

test('air brake decelerates', () => {
  const v = makeVehicle();
  run(v, 10);
  const before = v.speed;
  run(v, 2, { ...NEUTRAL, throttle: 0, brake: 1 });
  assert.ok(v.speed < before * 0.75, `brake only took ${before.toFixed(1)} -> ${v.speed.toFixed(1)}`);
});

test('steering rotates the heading continuously, not in discrete lanes', () => {
  const v = makeVehicle();
  run(v, 4);
  const headings = new Set();
  for (let i = 0; i < 180; i++) {
    v.update(DT, { ...NEUTRAL, steer: 0.5 });
    headings.add(v.yaw.toFixed(6));
  }
  // A lane runner snaps between a handful of states; a flight model does not.
  assert.ok(headings.size > 170, `only ${headings.size} distinct headings — steering is quantised`);
  assert.ok(Math.abs(v.yaw) > 0.5, `half-lock for 1.5s only turned ${v.yaw.toFixed(3)} rad`);
  // The turn must be a real arc, not a snap: radius = v / yawRate stays finite.
  assert.ok(v.speed / Math.abs(v.yaw / 1.5) > 40, 'turn radius implausibly tight for the speed');
});

test('the mass keeps going straight after the nose turns (this is the drift)', () => {
  const v = makeVehicle();
  run(v, 12); // build up speed
  const before = v.velocity.clone();
  v.update(DT, { ...NEUTRAL, steer: 1 });
  const after = v.velocity.clone();
  // Velocity direction must NOT match the new heading direction on frame 1.
  const velDir = after.clone().normalize();
  const misalign = 1 - velDir.dot(v.heading);
  assert.ok(misalign > 1e-5, 'velocity snapped to heading — no inertia, that is a runner not a racer');
  assert.ok(before.length() > 50, 'test needs the craft already at speed');
});

test('grip scrubs lateral slip off over time', () => {
  const v = makeVehicle();
  run(v, 10);
  run(v, 1, { ...NEUTRAL, steer: 1, drift: true });
  const slipping = v.slipRatio;
  assert.ok(slipping > 0.03, `drift produced no slip (${slipping})`);
  run(v, 2.5, { ...NEUTRAL, steer: 0 });
  assert.ok(v.slipRatio < slipping, `slip did not recover: ${v.slipRatio} vs ${slipping}`);
});

test('holding drift trades grip for turn-in and earns flow', () => {
  const a = makeVehicle();
  const b = makeVehicle();
  run(a, 10); run(b, 10);
  const yaw0 = a.yaw;
  run(a, 1.2, { ...NEUTRAL, steer: 1 });
  run(b, 1.2, { ...NEUTRAL, steer: 1, drift: true });
  assert.ok(Math.abs(b.yaw - yaw0) > Math.abs(a.yaw - yaw0), 'drift did not increase yaw rate');
  assert.ok(b.flow > a.flow, `drift earned no flow (${b.flow} vs ${a.flow})`);
});

test('boost out-accelerates the base thrust and drains the tank', () => {
  const a = makeVehicle();
  const b = makeVehicle();
  run(a, 3); run(b, 3);
  const sa = a.speed;
  run(b, 1.5, { ...NEUTRAL, boost: true });
  run(a, 1.5, NEUTRAL);
  assert.ok(b.speed > a.speed, 'boost did not increase speed');
  assert.ok(b.boost < CFG.boost.max, 'boost was not consumed');
  assert.ok(sa > 0);
});

test('the boost tank runs dry and stops helping', () => {
  const v = makeVehicle();
  run(v, 2);
  run(v, 8, { ...NEUTRAL, boost: true });
  assert.ok(v.boost < 1, `tank should be spent, still holds ${v.boost}`);
  assert.equal(v.boosting, false, 'should stop boosting when empty');
});

test('altitude control climbs and dives', () => {
  const v = makeVehicle();
  run(v, 6);
  const y0 = v.position.y;
  run(v, 2, { ...NEUTRAL, pitch: 1 });
  assert.ok(v.position.y > y0, 'climb input did not gain altitude');
  const y1 = v.position.y;
  run(v, 3, { ...NEUTRAL, pitch: -1 });
  assert.ok(v.position.y < y1, 'dive input did not lose altitude');
});

test('the craft cannot sink through the terrain', () => {
  const cfg = structuredClone(CFG);
  const v = new Vehicle(cfg, { terrain: () => 100 });
  v.reset(new THREE.Vector3(0, 108, 0), 0);
  v.velocity.set(0, -80, 0);
  run(v, 2, { ...NEUTRAL, throttle: 0 });
  assert.ok(v.position.y >= 100 + cfg.craft.hoverHeight - 0.5,
    `craft sank to ${v.position.y.toFixed(2)}, floor is ${100 + cfg.craft.hoverHeight}`);
});

test('a full flow meter fires a Tachyon Surge and raises the ceiling', () => {
  const v = makeVehicle();
  run(v, 6);
  const baseCap = v.topSpeed;
  v.addFlow(CFG.flow.max);
  v.update(DT, NEUTRAL);
  assert.ok(v.surgeActive, 'surge did not trigger at full flow');
  assert.equal(v.flow, 0, 'surge should consume the flow meter');
  assert.ok(v.topSpeed > baseCap, 'surge did not raise the speed ceiling');
  assert.ok(v.topSpeed === CFG.craft.topSpeedSurge);
});

test('a solid impact bleeds speed and breaks the chain', () => {
  const v = makeVehicle();
  run(v, 10);
  const before = v.speed;
  v.comboBroken = false;
  v.applyImpact(1);
  assert.ok(v.speed < before * 0.8, `impact barely slowed the craft (${before} -> ${v.speed})`);
  assert.equal(v.comboBroken, true);
});

test('skimming structure feeds the flow meter', () => {
  const v = makeVehicle();
  const f0 = v.flow;
  v.registerSkim(1);
  assert.ok(v.flow > f0, 'skim awarded no flow');
});

test('the model is deterministic for a fixed command stream', () => {
  const a = makeVehicle();
  const b = makeVehicle();
  const cmds = [];
  for (let i = 0; i < 600; i++) {
    cmds.push({
      steer: Math.sin(i * 0.07), pitch: Math.cos(i * 0.03) * 0.5,
      throttle: 1, brake: i % 200 < 10 ? 1 : 0, boost: i % 90 < 20, drift: i % 50 < 12
    });
  }
  for (const c of cmds) { a.update(DT, c); b.update(DT, c); }
  assert.ok(a.position.distanceTo(b.position) < 1e-9, 'same inputs diverged');
  assert.equal(a.speed, b.speed);
});
