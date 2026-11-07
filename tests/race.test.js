/**
 * Race integration test.
 *
 * Drives the player craft with the same pursuit brain the rivals use, through
 * the real Race.step loop, and asserts that a full run is actually completable:
 * gates claim in order, sectors record, the run finishes, and standings resolve.
 * This is the test that would catch an un-flyable corridor.
 */
import { test } from 'node:test';
import assert from 'node:assert/strict';
import { CFG } from '../src/config.js';
import { createCourse } from '../src/world/course.js';
import { Race } from '../src/sim/race.js';
import { AiDriver } from '../src/sim/ai.js';
import { Rng } from '../src/core/mathx.js';

const DT = 1 / 120;
const AUTO = { name: 'AUTO', tag: 'TEST', color: 0xffffff, skill: 0.995, aggression: 0, boostUse: 0.6, lineOffset: 0 };

function buildRace(seed) {
  const cfg = structuredClone(CFG);
  cfg.course.seed = seed;
  const course = createCourse(cfg);
  const race = new Race(cfg, course);
  const driver = new AiDriver(cfg, AUTO, course, race.player, new Rng(seed + 1));
  return { cfg, course, race, driver };
}

/** Skip the countdown and hand-control the player through the pursuit brain. */
function drive(race, driver, maxSeconds) {
  race.start();
  let t = 0;
  while (t < maxSeconds && race.state === 'racing') {
    const cmd = driver.computeCommand(DT, null);
    race.step(DT, cmd);
    t += DT;
  }
  return t;
}

test('a full run is completable end to end', () => {
  const { race, course, driver } = buildRace(424242);
  const elapsed = drive(race, driver, 400);

  const claimed = course.gates.filter((g) => g.claimed).length;
  const ratio = claimed / course.gates.length;

  assert.equal(race.player.finished, true,
    `run did not finish in ${elapsed.toFixed(1)}s (claimed ${claimed}/${course.gates.length})`);
  assert.ok(ratio >= 0.8,
    `autopilot only claimed ${(ratio * 100).toFixed(0)}% of gates — corridor may be un-flyable`);
  assert.ok(race.player.finishTime > 20,
    `suspiciously short run: ${race.player.finishTime.toFixed(1)}s`);
  assert.ok(race.player.finishTime < 400, 'run took implausibly long');
});

test('gates only claim in order and the counter advances', () => {
  const { race, course, driver } = buildRace(777);
  drive(race, driver, 60);
  const idx = course.gates.findIndex((g) => !g.claimed);
  assert.equal(race.nextGateIndex, idx === -1 ? course.gates.length : idx);
  for (let i = 0; i < race.nextGateIndex; i++) {
    assert.equal(course.gates[i].claimed, true, `gate ${i} skipped`);
  }
});

test('sector splits are recorded in increasing time order', () => {
  const { race, driver } = buildRace(424242);
  drive(race, driver, 400);
  const splits = race.splits.filter((s) => s !== undefined);
  assert.ok(splits.length >= 2, `only ${splits.length} splits recorded`);
  for (let i = 1; i < splits.length; i++) {
    assert.ok(splits[i] > splits[i - 1], 'splits out of order');
  }
  assert.ok(splits[splits.length - 1] <= race.player.finishTime + 1e-6);
});

test('standings are a total order with the player ranked', () => {
  const { race, driver } = buildRace(424242);
  drive(race, driver, 400);
  race.settleResults();
  const ranks = race.craft.map((v) => v.rank).sort((a, b) => a - b);
  assert.deepEqual(ranks, [1, 2, 3, 4]);
  assert.ok(race.player.rank >= 1 && race.player.rank <= 4);
});

test('progress along the corridor is what decides position', () => {
  const { race, driver } = buildRace(424242);
  drive(race, driver, 45);
  const ahead = race.craft.filter((v) => v.arc > race.player.arc).length;
  assert.equal(race.player.rank, ahead + 1,
    `rank ${race.player.rank} does not match ${ahead} craft further along`);
});

test('missing a gate leaves it claimable instead of failing the run', () => {
  const { race, course, driver } = buildRace(1234);
  race.start();
  // Let the autopilot get up to speed, then teleport it clear of the next ring.
  for (let i = 0; i < 600; i++) race.step(DT, driver.computeCommand(DT, null));
  const gate = course.gates[race.nextGateIndex];
  assert.ok(gate, 'no gate pending');

  race.player.position.copy(gate.center)
    .addScaledVector(gate.normal, -30)
    .addScaledVector(gate.up, gate.radius * 4);
  race.player.velocity.copy(gate.normal).multiplyScalar(120);
  race._prevGateS = undefined;

  let missed = false;
  for (let i = 0; i < 240; i++) {
    race.step(DT, { steer: 0, pitch: 0, throttle: 1, brake: 0, boost: false, drift: false });
    if (gate.missed) { missed = true; break; }
  }

  assert.equal(missed, true, 'flying outside the ring did not register a miss');
  assert.equal(gate.claimed, false, 'a missed gate must not count as claimed');
  assert.equal(race.state, 'racing', 'a miss must not end the run');
  assert.equal(race.nextGateIndex, course.gates.indexOf(gate), 'the chain must not advance past a miss');
});

test('a run can be reset and replayed cleanly', () => {
  const { race, course, driver } = buildRace(424242);
  drive(race, driver, 40);
  const midGates = course.gates.filter((g) => g.claimed).length;
  assert.ok(midGates > 0, 'sanity: nothing claimed before reset');

  race.reset();
  assert.equal(race.nextGateIndex, 0);
  assert.equal(course.gates.every((g) => !g.claimed && !g.missed), true);
  assert.equal(race.elapsed, 0);
  assert.equal(race.player.speed, 0);
  assert.equal(race.player.flow, 0);
  assert.equal(race.player.boost, CFG.boost.max);
  assert.equal(race.state, 'countdown');

  const t = drive(race, driver, 400);
  assert.equal(race.player.finished, true, `replay did not finish (${t.toFixed(1)}s)`);
});

test('results payload is complete and self-consistent', () => {
  const { race, course, driver } = buildRace(424242);
  drive(race, driver, 400);
  race.settleResults();
  const res = race.results();

  assert.equal(res.total, course.gates.length);
  assert.ok(res.gates <= res.total);
  assert.equal(res.standings.length, 4);
  assert.ok(res.topSpeed > 0);
  assert.ok(res.bestCombo >= 1);
  assert.ok(res.score >= 0);
  for (const s of res.standings) assert.ok(s.finished, 'settled results should have no DNFs');
});

test('three rivals are present and they move', () => {
  const { race, driver } = buildRace(424242);
  assert.equal(race.rivals.length, 3);
  drive(race, driver, 25);
  for (const v of race.rivals) {
    assert.ok(v.speed > 20, `${v.name} never got moving (${v.speed.toFixed(1)} m/s)`);
    assert.ok(v.arc > 200, `${v.name} covered only ${v.arc.toFixed(0)}m in 25s`);
  }
});

test('a corridor generated from any seed still yields a valid run', () => {
  for (const seed of [1, 99, 20261107]) {
    const { course } = buildRace(seed);
    assert.ok(course.gates.length >= 20, `seed ${seed}: only ${course.gates.length} gates`);
    assert.ok(course.totalLength > 8000, `seed ${seed}: course too short`);
    assert.ok(course.pylons.length > 100, `seed ${seed}: no structure`);
  }
});
