/**
 * Render-layer smoke tests.
 *
 * Three.js object construction does not need a WebGL context, so we can build
 * the real World and CraftView headlessly and check the invariants that matter
 * most: the mesh you SEE is the surface the sim COLLIDES with.
 */
import { test } from 'node:test';
import assert from 'node:assert/strict';
import * as THREE from 'three';
import { CFG } from '../src/config.js';
import { createCourse } from '../src/world/course.js';
import { World } from '../src/render/scene.js';
import { CraftView } from '../src/render/craft.js';
import { Race } from '../src/sim/race.js';
import { fmtTime } from '../src/ui/hud.js';

const cfg = structuredClone(CFG);
cfg.course.seed = 20261107;

function build() {
  const course = createCourse(cfg);
  const scene = new THREE.Scene();
  const world = new World(scene, course, cfg);
  return { course, scene, world };
}

test('world builds terrain, gates and structure into the scene', () => {
  const { course, scene, world } = build();
  assert.ok(world.terrainChunks.length > 8, `only ${world.terrainChunks.length} terrain chunks`);
  assert.equal(world.gateViews.length, course.gates.length, 'a view per gate');
  assert.equal(world.pylonMesh.count, course.pylons.length);
  assert.equal(world.bandMesh.count, course.pylons.length);
  assert.equal(world.shardMesh.count, course.shards.length);
  assert.ok(scene.children.length > 20, 'scene looks empty');
});

test('the terrain mesh height equals the height the sim collides with', () => {
  const { course, world } = build();
  let checked = 0;
  for (const chunk of world.terrainChunks) {
    const pos = chunk.geometry.attributes.position;
    // Chunk geometry is centred, world offset lives on the mesh transform.
    for (let i = 0; i < pos.count; i += 37) {
      const wx = pos.getX(i) + chunk.position.x;
      const wz = pos.getZ(i) + chunk.position.z;
      const expected = course.terrainHeight(wx, wz);
      assert.ok(Math.abs(pos.getY(i) - expected) < 1e-3,
        `visual ${pos.getY(i).toFixed(2)} != collision ${expected.toFixed(2)} at (${wx.toFixed(0)}, ${wz.toFixed(0)})`);
      checked++;
    }
  }
  assert.ok(checked > 200, `only sampled ${checked} vertices`);
});

test('every gate ring is placed at its gate centre and faces along its normal', () => {
  const { course, world } = build();
  const axis = new THREE.Vector3();
  for (let i = 0; i < world.gateViews.length; i++) {
    const gv = world.gateViews[i];
    assert.ok(gv.grp.position.distanceTo(course.gates[i].center) < 1e-6, `gate ${i} mispositioned`);
    axis.set(0, 0, 1).applyQuaternion(gv.grp.quaternion);
    assert.ok(axis.dot(course.gates[i].normal) > 0.999, `gate ${i} ring not facing its normal`);
    const ring = gv.ring;
    assert.ok(Math.abs(ring.geometry.parameters.radius - course.gates[i].radius) < 1e-6,
      `gate ${i} ring radius does not match the collision radius`);
  }
});

test('gate rings are big enough to fly through at top speed', () => {
  const { course } = build();
  for (const g of course.gates) {
    assert.ok(g.radius >= 20, `gate ${g.index} radius ${g.radius.toFixed(1)}m is too tight for 460 km/h`);
  }
});

test('craft view tracks the simulated transform and produces finite geometry', () => {
  const { course } = build();
  const race = new Race(cfg, course);
  const view = new CraftView(race.player, cfg);
  const terrain = (x, z) => course.terrainHeight(x, z);

  race.start();
  for (let i = 0; i < 240; i++) {
    race.step(1 / 120, { steer: Math.sin(i * 0.05), pitch: 0, throttle: 1, brake: 0, boost: i > 120, drift: i > 180 });
    view.update(1 / 120, terrain);
  }

  assert.ok(view.group.position.distanceTo(race.player.position) < 1e-9, 'view not tracking the sim');
  const pos = view.trail.geometry.attributes.position;
  for (let i = 0; i < pos.array.length; i++) {
    assert.ok(Number.isFinite(pos.array[i]), `non-finite trail vertex at ${i}`);
  }
  for (const disc of view.engineDiscs) {
    assert.ok(disc.scale.x > 0 && Number.isFinite(disc.scale.x));
    assert.ok(disc.material.opacity > 0 && disc.material.opacity <= 1);
  }
  assert.ok(view.groundGlow.visible, 'ground cushion should be on when near the deck');
});

test('world.update survives frames at speed, mid-surge and after impacts', () => {
  const { course, world } = build();
  const race = new Race(cfg, course);
  const camera = new THREE.PerspectiveCamera(74, 16 / 9, 0.5, 12000);
  race.start();

  for (let i = 0; i < 900; i++) {
    race.step(1 / 120, {
      steer: Math.sin(i * 0.03), pitch: Math.cos(i * 0.02) * 0.4,
      throttle: 1, brake: 0, boost: i % 200 < 60, drift: i % 130 < 30
    });
    camera.position.copy(race.player.position).addScaledVector(race.player.heading, -26);
    camera.updateMatrixWorld();
    world.update(1 / 120, race, camera);
  }

  const sp = world.streaks.geometry.attributes.position;
  for (let i = 0; i < sp.array.length; i++) {
    assert.ok(Number.isFinite(sp.array[i]), `non-finite streak vertex at ${i}`);
  }
  // Gates behind the player must be hidden, otherwise we draw 13 km of rings.
  const hidden = world.gateViews.filter((g) => !g.grp.visible).length;
  assert.ok(hidden > 0, 'no gate culling is happening');
});

test('fmtTime formats race clocks correctly', () => {
  assert.equal(fmtTime(0), '0:00.000');
  assert.equal(fmtTime(1.5), '0:01.500');
  assert.equal(fmtTime(61.234), '1:01.234');
  assert.equal(fmtTime(-3), '0:00.000');
  assert.equal(fmtTime(Infinity), '0:00.000');
});
