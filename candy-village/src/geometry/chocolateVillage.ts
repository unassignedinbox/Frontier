/**
 * Chocolate & candy village architecture: cottages, fudge houses, candy shops,
 * a tiered chocolate fountain, lollipop lamp posts and a bridge.
 *
 * Every building is generated from its footprint and a seeded RNG. Origins
 * are at ground centre; faces point toward +Z ("front").
 */
import {
  BufferGeometry,
  Color,
  ConeGeometry,
  CylinderGeometry,
  Group,
  Matrix4,
  Quaternion,
  TorusGeometry,
  Vector3,
} from 'three';
import { RNG } from '../core/rng';
import { MAT } from '../materials/library';
import {
  box,
  cylinder,
  domeGeometry,
  gableDomeMatrices,
  gableGeometry,
  instanced,
  mesh,
  mergeAll,
  pipedLoop,
  rimDrips,
  sphere,
  torusArc,
  wallBlock,
  windowUnit,
} from './builders';

export type BuildingKind = 'cottage' | 'fudge' | 'shop';

export interface BuildingSpec {
  kind: BuildingKind;
  w: number;
  d: number;
  h: number;
  seed: number;
}

export interface BuildingInfo {
  group: Group;
  footprint: { w: number; d: number };
}

/** Front-facing windows and a door along the +Z facade. */
function facadeOpenings(spec: BuildingSpec, baseY: number, glass: BufferGeometry[][], frames: BufferGeometry[][]): void {
  const { w, d, h } = spec;
  const hd = d / 2;
  const cols = w > 6 ? 2 : 1;
  const floors = h > 5.5 ? 2 : 1;
  const floorH = h / floors;
  for (let f = 0; f < floors; f++) {
    for (let c = 0; c < cols; c++) {
      const x = cols === 1 ? 0 : (c === 0 ? -1 : 1) * (w * 0.24);
      const y = baseY + f * floorH + floorH * 0.55;
      if (f === 0 && cols === 1) continue; // single-width ground floor: door instead
      const u = windowUnit(x, y, hd - 0.02, 0, [1.3, 1.5]);
      frames.push(u.frame);
      glass.push(u.glass);
    }
    // Side windows, one per side per floor.
    const zz = d * 0.2 * (f % 2 === 0 ? 1 : -1);
    const side = windowUnit(w / 2 - 0.02, baseY + f * floorH + floorH * 0.55, zz, Math.PI / 2, [1.2, 1.4]);
    const sideL = windowUnit(-w / 2 + 0.02, baseY + f * floorH + floorH * 0.55, zz, -Math.PI / 2, [1.2, 1.4]);
    frames.push(side.frame, sideL.frame);
    glass.push(side.glass, sideL.glass);
  }
}

function doorway(group: Group, spec: BuildingSpec, baseY: number, mat: 'chocolate' | 'peppermint' | 'icing'): void {
  const hd = spec.d / 2;
  const doorMat = mat === 'chocolate' ? MAT.chocolate() : mat === 'peppermint' ? MAT.peppermintRed() : MAT.icing();
  group.add(mesh(box(1.2, 2.1, 0.16, [0, baseY + 1.05, hd + 0.07]), doorMat));
  group.add(mesh(torusArc(0.6, 0.1, Math.PI, [0, baseY + 2.1, hd + 0.1]), MAT.icing()));
  group.add(mesh(sphere(0.08, [0.36, baseY + 1.05, hd + 0.18]), MAT.swirlLemon()));
}

/** Gable roof with snow drifts (icing domes) and drips. */
function gableRoof(group: Group, spec: BuildingSpec, baseY: number, rng: RNG, roofMat = MAT.chocolateRoof(), snow = true): number {
  const { w, d } = spec;
  const roofH = Math.min(3.2, w * 0.42);
  const overhang = 0.5;
  const roof = gableGeometry(w + overhang * 2, roofH, d + 0.8);
  roof.translate(0, baseY, 0);
  group.add(mesh(roof, roofMat));
  if (snow) {
    const domes = gableDomeMatrices({
      eaveX: w / 2 + overhang,
      wallTop: baseY,
      roofH,
      halfDepth: d / 2 + 0.2,
      rowSpacing: 0.5,
      colSpacing: 0.6,
      rng,
      chance: 0.55,
    });
    group.add(instanced(domeGeometry(0.3, 0.5), MAT.icing(), domes));
  }
  return roofH;
}

/** Rustic chocolate cottage: dark walls, sugar-snowed gable roof and melted drips. */
function buildCottage(spec: BuildingSpec): BuildingInfo {
  const rng = new RNG(spec.seed);
  const g = new Group();
  const plinthH = 0.5;
  g.add(mesh(box(spec.w + 0.4, plinthH, spec.d + 0.4, [0, plinthH / 2, 0]), MAT.fudgePale()));
  const walls = wallBlock(spec.w, spec.h, spec.d, 0.14).translate(0, plinthH, 0);
  g.add(mesh(walls, MAT.chocolate()));
  gableRoof(g, spec, plinthH + spec.h, rng);
  g.add(mesh(rimDrips(spec.w, spec.d, plinthH + spec.h, rng, { skip: 0.5, spacing: 0.7 }), MAT.chocolate()));

  const frames: BufferGeometry[][] = [];
  const glass: BufferGeometry[][] = [];
  facadeOpenings(spec, plinthH, glass, frames);
  g.add(mesh(mergeAll(frames.flat()), MAT.chocolate()));
  g.add(mesh(mergeAll(glass.flat()), MAT.sugarGlassCool()));
  doorway(g, spec, plinthH, 'chocolate');

  // Chocolate chimney with a sugar cap.
  const cx = spec.w * 0.28;
  g.add(mesh(box(0.8, 2.0, 0.8, [cx, plinthH + spec.h + 1.2, -spec.d * 0.15]), MAT.chocolate()));
  g.add(mesh(box(1.0, 0.2, 1.0, [cx, plinthH + spec.h + 2.25, -spec.d * 0.15]), MAT.icing()));

  return { group: g, footprint: { w: spec.w + 0.8, d: spec.d + 0.8 } };
}

/** Fudge-brick house with pastel icing roof, awning and gumdrop window boxes. */
function buildFudgeHouse(spec: BuildingSpec): BuildingInfo {
  const rng = new RNG(spec.seed);
  const g = new Group();
  const plinthH = 0.35;
  g.add(mesh(box(spec.w + 0.6, plinthH, spec.d + 0.6, [0, plinthH / 2, 0]), MAT.chocolate()));
  g.add(mesh(wallBlock(spec.w, spec.h, spec.d, 0.06).translate(0, plinthH, 0), MAT.fudge()));

  const roofMat = rng.chance(0.5) ? MAT.icingPink() : MAT.icingMint();
  const roofH = gableRoof(g, spec, plinthH + spec.h, rng, roofMat, false);
  // Shingle domes in the roof colour, tinted with sprinkles via the icing shader.
  const domes = gableDomeMatrices({
    eaveX: spec.w / 2 + 0.5,
    wallTop: plinthH + spec.h,
    roofH,
    halfDepth: spec.d / 2 + 0.2,
    rowSpacing: 0.4,
    colSpacing: 0.45,
    rng,
  });
  g.add(instanced(domeGeometry(0.22, 0.45), roofMat, domes));

  const frames: BufferGeometry[][] = [];
  const glass: BufferGeometry[][] = [];
  facadeOpenings(spec, plinthH, glass, frames);
  g.add(mesh(mergeAll(frames.flat()), MAT.icing()));
  g.add(mesh(mergeAll(glass.flat()), MAT.sugarGlass()));
  doorway(g, spec, plinthH, 'icing');

  // Peppermint awning over the door.
  const awning = box(2.2, 0.12, 1.1, [0, 0, 0]);
  awning.rotateX(-0.35);
  awning.translate(0, plinthH + 2.6, spec.d / 2 + 0.6);
  g.add(mesh(awning, MAT.peppermintMint()));

  // Gumdrop window boxes beneath the front windows.
  const gum: Matrix4[] = [];
  const gumColours: Color[] = ['#ff4d6d', '#ffd23f', '#7b61ff', '#ff9f1c'].map((c) => new Color(c));
  for (let i = -3; i <= 3; i++) {
    gum.push(new Matrix4().compose(new Vector3(i * 0.42, plinthH + 1.2, spec.d / 2 + 0.28), new Quaternion(), new Vector3(0.14, 0.11, 0.14)));
  }
  g.add(mesh(box(spec.w * 0.52, 0.18, 0.4, [0, plinthH + 1.05, spec.d / 2 + 0.2]), MAT.peppermintRed()));
  g.add(instanced(sphere(1, [0, 0, 0], 1, 10), MAT.icing(), gum, gum.map((_, i) => gumColours[i % gumColours.length])));

  return { group: g, footprint: { w: spec.w + 0.6, d: spec.d + 0.6 } };
}

/** Two-storey candy shop with a swirl cone roof, striped awning and lollipop finial. */
function buildCandyShop(spec: BuildingSpec): BuildingInfo {
  const rng = new RNG(spec.seed);
  const g = new Group();
  const plinthH = 0.6;
  g.add(mesh(box(spec.w + 0.5, plinthH, spec.d + 0.5, [0, plinthH / 2, 0]), MAT.chocolate()));
  g.add(mesh(wallBlock(spec.w, spec.h, spec.d, 0.2).translate(0, plinthH, 0), MAT.icing()));
  // Chocolate band between the storeys.
  g.add(mesh(box(spec.w + 0.25, 0.3, spec.d + 0.25, [0, plinthH + spec.h * 0.5, 0]), MAT.chocolate()));

  const roofR = Math.max(spec.w, spec.d) * 0.62;
  const roofH = 3.4;
  const cone = new ConeGeometry(roofR, roofH, 28, 1);
  cone.translate(0, plinthH + spec.h + roofH / 2, 0);
  cone.deleteAttribute('uv');
  g.add(mesh(cone, rng.chance(0.5) ? MAT.swirlMint() : MAT.swirlPink()));
  // Lollipop finial.
  const topY = plinthH + spec.h + roofH;
  g.add(mesh(cylinder(0.05, 0.05, 1.2, [0, topY + 0.6, 0], 8), MAT.licorice()));
  g.add(mesh(sphere(0.32, [0, topY + 1.35, 0]), MAT.swirlGlow()));

  // Icing trim ring around the top of the walls.
  const rim = pipedLoop(
    [
      new Vector3(-spec.w / 2 - 0.1, plinthH + spec.h, -spec.d / 2 - 0.1),
      new Vector3(spec.w / 2 + 0.1, plinthH + spec.h, -spec.d / 2 - 0.1),
      new Vector3(spec.w / 2 + 0.1, plinthH + spec.h, spec.d / 2 + 0.1),
      new Vector3(-spec.w / 2 - 0.1, plinthH + spec.h, spec.d / 2 + 0.1),
    ],
    0.14,
    true,
  );
  g.add(mesh(rim, MAT.icing()));

  const frames: BufferGeometry[][] = [];
  const glass: BufferGeometry[][] = [];
  facadeOpenings(spec, plinthH, glass, frames);
  g.add(mesh(mergeAll(frames.flat()), MAT.chocolate()));
  g.add(mesh(mergeAll(glass.flat()), MAT.sugarGlass()));

  // Striped awning across the shop front.
  const awning = box(spec.w * 0.8, 0.1, 1.4, [0, 0, 0]);
  awning.rotateX(-0.3);
  awning.translate(0, plinthH + 2.5, spec.d / 2 + 0.7);
  g.add(mesh(awning, MAT.peppermintRed()));
  doorway(g, spec, plinthH, 'peppermint');

  return { group: g, footprint: { w: spec.w + 0.5, d: spec.d + 0.5 } };
}

export function buildBuilding(spec: BuildingSpec): BuildingInfo {
  switch (spec.kind) {
    case 'cottage':
      return buildCottage(spec);
    case 'fudge':
      return buildFudgeHouse(spec);
    case 'shop':
      return buildCandyShop(spec);
  }
}

/* -------------------------------------------------------------------------- */
/*                               Plaza and props                              */
/* -------------------------------------------------------------------------- */

/** Tiered chocolate fountain, centred on the origin. */
export function buildFountain(): Group {
  const g = new Group();
  g.name = 'ChocolateFountain';
  g.add(mesh(new CylinderGeometry(3.4, 3.6, 0.6, 36).translate(0, 0.3, 0), MAT.fudgePale()));
  g.add(mesh(new CylinderGeometry(3.0, 3.0, 0.08, 36).translate(0, 0.62, 0), MAT.chocolateFountain(), { cast: false }));
  const rim = new TorusGeometry(3.25, 0.14, 10, 48);
  rim.rotateX(Math.PI / 2);
  rim.translate(0, 0.66, 0);
  rim.deleteAttribute('uv');
  g.add(mesh(rim, MAT.icingPink()));

  g.add(mesh(cylinder(1.8, 1.9, 0.5, [0, 0.9, 0], 28), MAT.fudgePale()));
  g.add(mesh(cylinder(1.25, 1.4, 0.36, [0, 1.42, 0], 28), MAT.icingMint()));
  g.add(mesh(cylinder(0.36, 0.5, 1.5, [0, 2.25, 0], 20), MAT.peppermintRed()));
  g.add(mesh(cylinder(0.95, 0.6, 0.28, [0, 3.2, 0], 24), MAT.icingPink()));
  g.add(mesh(sphere(0.28, [0, 3.5, 0]), MAT.swirlPink()));
  // Chocolate pool sitting on the top bowl.
  g.add(mesh(new CylinderGeometry(0.85, 0.85, 0.06, 24).translate(0, 3.34, 0), MAT.chocolateFountain(), { cast: false }));
  return g;
}

/** Lollipop lamp: licorice pole and a glowing swirl lollipop head. */
export function buildLollipopLamp(): { group: Group; headY: number } {
  const g = new Group();
  g.add(mesh(cylinder(0.07, 0.1, 3.3, [0, 1.65, 0], 10), MAT.licorice()));
  g.add(mesh(cylinder(0.25, 0.25, 0.2, [0, 0.1, 0], 14), MAT.fudgePale()));
  const headY = 3.7;
  g.add(mesh(sphere(0.42, [0, headY, 0]), MAT.swirlGlow(), { cast: false }));
  g.add(mesh(cylinder(0.03, 0.03, 0.4, [0, headY - 0.5, 0], 8), MAT.licorice()));
  return { group: g, headY };
}

/**
 * A chocolate bridge spanning from `a` to `b` at height `deckY`.
 * Railings are liquorice, the deck is fudge.
 */
export function buildBridge(a: Vector3, b: Vector3, width: number, deckY: number): { group: Group; length: number; yaw: number } {
  const g = new Group();
  const len = Math.hypot(b.x - a.x, b.z - a.z) + 1.4;
  const yaw = Math.atan2(b.x - a.x, b.z - a.z);
  const mid = new Vector3((a.x + b.x) / 2, deckY, (a.z + b.z) / 2);
  g.position.copy(mid);
  g.rotation.y = yaw;
  g.add(mesh(box(width, 0.42, len, [0, 0, 0]), MAT.fudge()));
  const rails: BufferGeometry[] = [];
  for (const side of [-1, 1]) {
    rails.push(box(0.1, 0.1, len, [side * (width / 2 - 0.05), 0.65, 0]));
    const posts = Math.floor(len / 1.5);
    for (let i = 0; i <= posts; i++) {
      const z = -len / 2 + (i * len) / posts;
      rails.push(box(0.1, 0.65, 0.1, [side * (width / 2 - 0.05), 0.33, z]));
    }
  }
  g.add(mesh(mergeAll(rails), MAT.licorice()));
  // Supports so the deck reads as grounded at each bank.
  const pillars: BufferGeometry[] = [];
  for (const end of [-1, 1]) {
    pillars.push(cylinder(0.45, 0.55, 5, [0, -2.5 - 0.21, end * (len / 2 - 0.9)], 12));
  }
  g.add(mesh(mergeAll(pillars), MAT.fudgePale()));
  return { group: g, length: len, yaw };
}
