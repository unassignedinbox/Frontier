/**
 * World assembly: turns the seeded layout into a populated three.js scene and
 * owns the things that update every frame (guards, lights, sky clock).
 */
import {
  AmbientLight,
  DirectionalLight,
  Group,
  HemisphereLight,
  Mesh,
  PointLight,
  Scene,
  ShaderMaterial,
  Vector3,
} from 'three';
import { Simplex3 } from '../core/noise';
import { RNG } from '../core/rng';
import { buildBuilding, buildBridge, buildFountain, buildLollipopLamp } from '../geometry/chocolateVillage';
import { buildForest } from '../geometry/candyForest';
import { buildGingerbreadHouse } from '../geometry/gingerbreadHouse';
import { buildRiver, buildRoad, buildTerrain } from '../geometry/terrain';
import { buildJellyGuard, shadowAll } from '../guard/jellyGuard';
import { JellyGuard } from '../guard/guardController';
import { generateLayout, type WorldLayout } from './layout';
import { NavGrid } from './navgrid';
import { buildSky, SUN_DIRECTION } from './sky';
import { setProcTime } from '../materials/procMaterial';

export interface World {
  seed: number;
  layout: WorldLayout;
  nav: NavGrid;
  root: Group;
  terrain: Mesh;
  guards: JellyGuard[];
  sun: DirectionalLight;
  skyMaterial: ShaderMaterial;
  lampLights: PointLight[];
  update(time: number): void;
}

export function buildWorld(scene: Scene, seed: number): World {
  const layout = generateLayout(seed);
  const root = new Group();
  root.name = `CandyVillage:${seed}`;
  scene.add(root);

  // Sky and environment.
  const sky = buildSky();
  scene.add(sky.mesh);

  // Ground.
  const terrain = buildTerrain(layout);
  root.add(terrain);
  root.add(buildRiver(layout));
  root.add(buildRoad(layout));

  // Village.
  const village = new Group();
  village.name = 'Village';
  for (const spec of layout.houses) {
    const b = buildBuilding({ kind: spec.kind, w: spec.w, d: spec.d, h: spec.h, seed: spec.seed });
    b.group.position.set(spec.x, layout.heightAt(spec.x, spec.z), spec.z);
    b.group.rotation.y = spec.rotY;
    shadowAll(b.group);
    village.add(b.group);
  }

  // Gingerbread house on the far bank.
  const gh = buildGingerbreadHouse(layout.gingerbread.seed);
  gh.group.position.set(layout.gingerbread.x, layout.heightAt(layout.gingerbread.x, layout.gingerbread.z), layout.gingerbread.z);
  gh.group.rotation.y = layout.gingerbread.rotY;
  shadowAll(gh.group);
  village.add(gh.group);

  // Bridges.
  for (const br of layout.bridges) {
    const b = buildBridge(new Vector3(br.a.x, 0, br.a.z), new Vector3(br.b.x, 0, br.b.z), br.width, br.deckY);
    shadowAll(b.group);
    village.add(b.group);
  }

  // Plaza: fountain and lollipop lamps.
  const fountain = buildFountain();
  fountain.position.set(0, layout.heightAt(0, 0), 0);
  shadowAll(fountain);
  village.add(fountain);

  const lampLights: PointLight[] = [];
  for (const l of layout.lamps) {
    const lamp = buildLollipopLamp();
    lamp.group.position.set(l.x, layout.heightAt(l.x, l.z), l.z);
    shadowAll(lamp.group);
    village.add(lamp.group);
    const light = new PointLight(0xffb3d9, 6, 22, 1.6);
    light.position.set(l.x, layout.heightAt(l.x, l.z) + lamp.headY, l.z);
    village.add(light);
    lampLights.push(light);
  }
  root.add(village);

  // Candy forest.
  const forest = buildForest(layout.forest, new Simplex3(seed ^ 0x2f6e2b1));
  shadowAll(forest);
  root.add(forest);

  // Guards.
  const guardRng = new RNG(seed ^ 0x5151);
  const guards: JellyGuard[] = layout.guardSpawns.map((spawn, i) => {
    const rig = buildJellyGuard(seed + i * 7919, i);
    root.add(rig.root);
    shadowAll(rig.root);
    const g = new JellyGuard(rig, i, guardRng.fork(i));
    const face = Math.atan2(-spawn.x, -spawn.z);
    g.placeAt(spawn.x, spawn.z, face, (x, z) => layout.heightAt(x, z));
    return g;
  });

  // Navigation is built after the layout, from the same source of truth.
  const nav = new NavGrid(layout);

  // Lights: warm sun, candy-sky hemisphere fill, faint ambient.
  const sun = new DirectionalLight(0xfff1dc, 3.2);
  sun.position.copy(SUN_DIRECTION).multiplyScalar(160);
  sun.target.position.set(0, 0, 0);
  sun.castShadow = true;
  sun.shadow.mapSize.set(2048, 2048);
  const cam = sun.shadow.camera;
  cam.left = -90;
  cam.right = 90;
  cam.top = 90;
  cam.bottom = -90;
  cam.near = 20;
  cam.far = 400;
  sun.shadow.bias = -0.0004;
  sun.shadow.normalBias = 0.04;
  scene.add(sun);
  scene.add(sun.target);
  const hemi = new HemisphereLight(0xdfeeff, 0x6b3b2a, 1.1);
  scene.add(hemi);
  scene.add(new AmbientLight(0xffffff, 0.08));

  return {
    seed,
    layout,
    nav,
    root,
    terrain,
    guards,
    sun,
    skyMaterial: sky.material,
    lampLights,
    update(time: number) {
      setProcTime(time);
      sky.material.uniforms.uTime.value = time;
      lampLights.forEach((l, i) => {
        l.intensity = 5.5 + 0.8 * Math.sin(time * 2.3 + i * 1.7);
      });
    },
  };
}
