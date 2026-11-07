/**
 * Procedural jellybean guard: a massive colloidal-candy sentinel built from
 * noise-displaced jellybean limbs, a sugar-coated face, a chocolate bearskin
 * hat and a lollipop mace.
 *
 * This file only builds the hierarchy and geometry. Motion lives in
 * guardController.ts, which drives the bones with IK each frame.
 */
import { BufferGeometry, Group, Mesh, Object3D, Quaternion, Vector3 } from 'three';
import { Simplex3 } from '../core/noise';
import { RNG } from '../core/rng';
import { MAT, guardJelly } from '../materials/library';
import type { ProcMaterial } from '../materials/procMaterial';
import { beanGeometry, blobGeometry, box, cylinder, mesh, mergeAll, sphere } from '../geometry/builders';

/** Local (unscaled) rig dimensions in metres. The root applies the guard scale. */
export const RIG = {
  scale: 0.8,
  hipHeight: 2.5,
  thigh: 1.25,
  shin: 1.25,
  torso: 1.9,
  shoulderY: 1.35,
  shoulderX: 1.05,
  upperArm: 1.4,
  foreArm: 1.25,
  club: 2.4,
  hipX: 0.55,
  ankleHeight: 0.35,
};

export interface Limb {
  /** Joint at the top of the limb (shoulder or hip). */
  root: Object3D;
  /** Joint at the middle (elbow or knee). */
  mid: Object3D;
  /** Joint at the end (wrist or ankle). */
  end: Object3D;
}

export interface GuardRig {
  root: Group;
  body: Group;
  hips: Object3D;
  chest: Object3D;
  head: Object3D;
  /** +X side arm: carries the lollipop mace. */
  armClub: Limb;
  /** -X side arm: swings freely. */
  armFree: Limb;
  legA: Limb;
  legB: Limb;
  club: Group;
  /** Jelly material shared by all body parts; wobble is driven per guard. */
  jelly: ProcMaterial;
  flavour: string;
  name: string;
}

export const FLAVOURS = [
  { name: 'Strawberry', hex: '#ff3b6b', accent: 'swirlPink' },
  { name: 'Lime', hex: '#7ee04a', accent: 'swirlMint' },
  { name: 'Blueberry', hex: '#3b82ff', accent: 'swirlGrape' },
  { name: 'Lemon', hex: '#ffd21f', accent: 'swirlLemon' },
  { name: 'Grape', hex: '#9b5cff', accent: 'swirlGrape' },
] as const;

/** Joint with a child mesh, rotated so that its +Y points along the limb. */
function jointWithChild(parent: Object3D, pos: [number, number, number], rest: Quaternion | null): Object3D {
  const j = new Object3D();
  j.position.set(pos[0], pos[1], pos[2]);
  if (rest) j.quaternion.copy(rest);
  parent.add(j);
  return j;
}

export function buildJellyGuard(seed: number, flavourIndex: number): GuardRig {
  const rng = new RNG(seed);
  const noise = new Simplex3(seed ^ 0x51ed2701);
  const flavour = FLAVOURS[flavourIndex % FLAVOURS.length];
  const jelly = guardJelly(flavour.hex);
  const accentMat = MAT[flavour.accent]();

  const root = new Group();
  root.name = `JellyGuard:${flavour.name}`;
  root.scale.setScalar(RIG.scale);
  const body = new Group();
  root.add(body);
  const hips = new Object3D();
  hips.position.set(0, RIG.hipHeight, 0);
  body.add(hips);

  // Torso (chest joint sits at the hips, the bean rises along +Y).
  const chest = new Object3D();
  chest.position.set(0, 0.05, 0);
  hips.add(chest);
  const torsoGeo = beanGeometry({
    length: RIG.torso,
    radius: 1.0,
    squashZ: 0.82,
    taper: 0.86,
    noise,
    noiseAmp: 0.06,
    noiseFreq: 1.3,
    rings: 26,
    segments: 22,
  });
  chest.add(mesh(torsoGeo, jelly));
  // Belly pad, a slightly bigger pillow of jelly.
  chest.add(mesh(blobGeometry(0.85, noise, { detail: 2, amp: 0.08, freq: 1.6, sy: 0.7, seed: rng.int(1, 999) }).translate(0, 0.9, 0.5), jelly));

  // Head with sugar-coated face and bearskin hat.
  const head = new Object3D();
  head.position.set(0, RIG.torso, 0);
  chest.add(head);
  head.add(mesh(blobGeometry(0.95, noise, { detail: 3, amp: 0.07, freq: 1.5, sy: 0.92, seed: rng.int(1, 999) }).translate(0, 0.95, 0), jelly));
  for (const sx of [-0.36, 0.36]) {
    head.add(mesh(sphere(0.24, [sx, 1.05, 0.78]), MAT.icing()));
    head.add(mesh(sphere(0.11, [sx, 1.03, 1.0]), MAT.chocolate()));
  }
  head.add(mesh(sphere(0.5, [0, 0.52, 0.8], 0.4, 12), MAT.licorice()));
  const teeth: BufferGeometry[] = [];
  for (let i = -2; i <= 2; i++) teeth.push(box(0.14, 0.14, 0.08, [i * 0.16, 0.66, 0.9]));
  head.add(mesh(mergeAll(teeth), MAT.icing()));
  // Bearskin hat (tall chocolate cylinder with a sugar-stripe band).
  head.add(mesh(cylinder(0.82, 0.92, 1.45, [0, 2.0, 0], 24), MAT.chocolate()));
  head.add(mesh(cylinder(0.94, 0.94, 0.22, [0, 1.42, 0], 24), MAT.peppermintRed()));
  head.add(mesh(sphere(0.4, [0, 2.8, 0]), MAT.chocolate()));

  // Arms.
  let maceGroup: Group | null = null;
  const buildArm = (side: 1 | -1, isClub: boolean): Limb => {
    const shoulderRest = new Quaternion().setFromAxisAngle(new Vector3(0, 0, 1), Math.PI);
    const shoulder = jointWithChild(chest, [side * RIG.shoulderX, RIG.shoulderY, 0], shoulderRest);
    shoulder.add(mesh(sphere(0.34, [0, 0, 0]), jelly));
    shoulder.add(mesh(beanGeometry({ length: RIG.upperArm, radius: 0.3, noise, noiseAmp: 0.05, noiseFreq: 2, squashX: 1, squashZ: 0.95 }), jelly));
    const elbow = jointWithChild(shoulder, [0, RIG.upperArm, 0], null);
    elbow.add(mesh(sphere(0.3, [0, 0, 0]), jelly));
    elbow.add(mesh(beanGeometry({ length: RIG.foreArm, radius: 0.27, noise, noiseAmp: 0.05, noiseFreq: 2 }), jelly));
    const wrist = jointWithChild(elbow, [0, RIG.foreArm, 0], null);
    wrist.add(mesh(sphere(0.36, [0, 0, 0]), jelly));
    if (isClub) {
      // Lollipop mace: peppermint haft with a swirl head.
      const club = new Group();
      club.name = 'LollipopMace';
      club.add(mesh(cylinder(0.1, 0.12, RIG.club, [0, RIG.club / 2, 0], 12), MAT.peppermintRed()));
      club.add(mesh(sphere(0.5, [0, RIG.club, 0]), accentMat));
      wrist.add(club);
      maceGroup = club;
    }
    return { root: shoulder, mid: elbow, end: wrist };
  };
  const armClub = buildArm(1, true);
  const armFree = buildArm(-1, false);
  if (!maceGroup) throw new Error('guard rig: mace missing');

  // Legs.
  const buildLeg = (side: 1 | -1): Limb => {
    const hipRest = new Quaternion().setFromAxisAngle(new Vector3(0, 0, 1), Math.PI);
    const hip = jointWithChild(hips, [side * RIG.hipX, 0, 0], hipRest);
    hip.add(mesh(sphere(0.42, [0, 0, 0]), jelly));
    hip.add(mesh(beanGeometry({ length: RIG.thigh, radius: 0.44, noise, noiseAmp: 0.05, noiseFreq: 1.8 }), jelly));
    const knee = jointWithChild(hip, [0, RIG.thigh, 0], null);
    knee.add(mesh(sphere(0.4, [0, 0, 0]), jelly));
    knee.add(mesh(beanGeometry({ length: RIG.shin, radius: 0.38, taper: 0.8, noise, noiseAmp: 0.05, noiseFreq: 1.8 }), jelly));
    const ankle = jointWithChild(knee, [0, RIG.shin, 0], null);
    // Rounded, forward-leaning boot.
    ankle.add(mesh(sphere(0.5, [0, -0.1, 0.22], 0.55, 14).scale(1, 1, 1.6), jelly));
    return { root: hip, mid: knee, end: ankle };
  };
  const legA = buildLeg(1);
  const legB = buildLeg(-1);

  return {
    root,
    body,
    hips,
    chest,
    head,
    armClub,
    armFree,
    legA,
    legB,
    club: maceGroup,
    jelly,
    flavour: flavour.hex,
    name: flavour.name,
  };
}

/** Enables shadows on every mesh of a rig. */
export function shadowAll(root: Object3D): void {
  root.traverse((o) => {
    if (o instanceof Mesh) {
      o.castShadow = true;
      o.receiveShadow = true;
    }
  });
}
