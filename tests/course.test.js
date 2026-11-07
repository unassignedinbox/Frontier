/**
 * Course generation invariants. These run the real `createCourse` from src/.
 */
import { test } from 'node:test';
import assert from 'node:assert/strict';
import { CFG } from '../src/config.js';
import { createCourse } from '../src/world/course.js';

const cfg = structuredClone(CFG);
cfg.course.seed = 424242;

test('corridor is reproducible from its seed', () => {
  const a = createCourse(cfg);
  const b = createCourse(cfg);
  assert.equal(a.gates.length, b.gates.length);
  assert.equal(a.totalLength, b.totalLength);
  for (let i = 0; i < a.gates.length; i++) {
    assert.ok(a.gates[i].center.distanceTo(b.gates[i].center) < 1e-9, `gate ${i} differs`);
  }
});

test('a different seed produces a different corridor', () => {
  const other = structuredClone(cfg);
  other.course.seed = 999;
  const a = createCourse(cfg);
  const b = createCourse(other);
  const same = a.gates.every((g, i) => g.center.distanceTo(b.gates[i].center) < 1e-6);
  assert.equal(same, false);
});

test('course is a point-to-point run, not a circuit', () => {
  const c = createCourse(cfg);
  const start = c.samples[0];
  const end = c.samples[c.sampleCount - 1];
  // Start and finish must be far apart — a closed loop would put them together.
  assert.ok(start.distanceTo(end) > c.totalLength * 0.5,
    `start/finish only ${start.distanceTo(end).toFixed(0)}m apart on a ${c.totalLength.toFixed(0)}m course`);
});

test('dense samples march monotonically forward in Z', () => {
  const c = createCourse(cfg);
  for (let i = 1; i < c.sampleCount; i++) {
    assert.ok(c.samples[i].z > c.samples[i - 1].z, `z went backwards at sample ${i}`);
  }
});

test('gate chain is ordered along the corridor and covers it', () => {
  const c = createCourse(cfg);
  assert.ok(c.gates.length >= 20, `expected a real gate chain, got ${c.gates.length}`);
  for (let i = 1; i < c.gates.length; i++) {
    assert.ok(c.gates[i].arc > c.gates[i - 1].arc, `gate ${i} arc out of order`);
    assert.ok(c.gates[i].index === i);
  }
  assert.ok(c.gates[0].arc > 0);
  assert.ok(c.gates[c.gates.length - 1].arc < c.totalLength);
});

test('gates force real 3-D steering: they move laterally AND vertically', () => {
  const c = createCourse(cfg);
  let lateralSwing = 0, verticalSwing = 0, directionChanges = 0;
  let prevSide = 0;
  for (let i = 1; i < c.gates.length; i++) {
    const a = c.gates[i - 1], b = c.gates[i];
    lateralSwing = Math.max(lateralSwing, Math.abs(b.center.x - a.center.x));
    verticalSwing = Math.max(verticalSwing, Math.abs(b.center.y - a.center.y));
    const side = Math.sign(b.center.x - a.center.x);
    if (side !== 0 && prevSide !== 0 && side !== prevSide) directionChanges++;
    if (side !== 0) prevSide = side;
  }
  // A lane runner would have zero vertical movement and no direction changes.
  assert.ok(verticalSwing > 5, `gates never move vertically (max ${verticalSwing.toFixed(2)}m)`);
  assert.ok(lateralSwing > 40, `gates never move laterally (max ${lateralSwing.toFixed(2)}m)`);
  assert.ok(directionChanges >= 4, `corridor never changes direction (${directionChanges})`);
});

test('gate normals are unit length and face down-corridor', () => {
  const c = createCourse(cfg);
  for (const g of c.gates) {
    assert.ok(Math.abs(g.normal.length() - 1) < 1e-6, 'normal not normalized');
    assert.ok(g.radius >= CFG.course.gateRadiusMin - 1e-6);
    assert.ok(g.radius <= CFG.course.gateRadiusMax + 1e-6);
  }
});

test('sectors partition every gate exactly once', () => {
  const c = createCourse(cfg);
  const seen = new Set();
  for (const s of c.sectors) {
    for (let i = s.firstGate; i <= s.lastGate; i++) {
      assert.ok(!seen.has(i), `gate ${i} in two sectors`);
      seen.add(i);
    }
  }
  assert.equal(seen.size, c.gates.length, 'sectors do not cover the whole chain');
});

test('terrain height is finite everywhere sampled', () => {
  const c = createCourse(cfg);
  for (let i = 0; i < c.sampleCount; i += 37) {
    const s = c.samples[i];
    for (const dx of [-600, -120, 0, 120, 600]) {
      const h = c.terrainHeight(s.x + dx, s.z);
      assert.ok(Number.isFinite(h), `non-finite terrain at (${s.x + dx}, ${s.z})`);
    }
  }
});

test('the racing line has clearance above the canyon floor', () => {
  const c = createCourse(cfg);
  let minClearance = Infinity;
  for (let i = 0; i < c.sampleCount; i += 11) {
    const s = c.samples[i];
    minClearance = Math.min(minClearance, s.y - c.terrainHeight(s.x, s.z));
  }
  assert.ok(minClearance > CFG.craft.hoverHeight,
    `racing line only ${minClearance.toFixed(1)}m above the deck`);
  // Gates must also be flyable, not buried.
  for (const g of c.gates) {
    const floor = c.terrainHeight(g.center.x, g.center.z);
    assert.ok(g.center.y - g.radius > floor - 4,
      `gate ${g.index} clips the terrain`);
  }
});

test('nearest() returns the arc length actually travelled', () => {
  const c = createCourse(cfg);
  const mid = Math.floor(c.sampleCount / 2);
  const p = c.samples[mid];
  const n = c.nearest(p, 0, c.sampleCount);
  assert.equal(n.index, mid);
  assert.ok(Math.abs(n.arc - c.arclen[mid]) < 1e-9);
  assert.ok(Math.abs(n.lateral) < 1e-6, 'centreline should read zero lateral');
});

test('structure is generated inside and outside the corridor', () => {
  const c = createCourse(cfg);
  assert.ok(c.pylons.length > 100, `only ${c.pylons.length} pylons`);
  const kinds = new Set(c.pylons.map((p) => p.kind));
  for (const k of ['wall', 'slalom', 'spire']) {
    assert.ok(kinds.has(k), `missing pylon kind ${k}`);
  }
  assert.ok(c.boostPads.length > 3, 'no boost pads generated');
  for (const p of c.pylons) {
    assert.ok(Number.isFinite(p.x) && Number.isFinite(p.z) && p.h > 0);
  }
});
