/**
 * Procedural gingerbread house.
 *
 * Built in local metres with the ground centre at the origin and the front
 * facade (door) on +Z. Shingle rhythm, drip lengths, window placement and
 * gumdrop colours are drawn from the seed, so each house differs slightly.
 */
import {
  BufferGeometry,
  Color,
  CylinderGeometry,
  Group,
  Matrix4,
  Quaternion,
  Vector3,
} from 'three';
import { RNG } from '../core/rng';
import { MAT, jellyMaterial } from '../materials/library';
import {
  box,
  cylinder,
  domeGeometry,
  gableGeometry,
  instanced,
  mesh,
  mergeAll,
  pipeAlong,
  pipedLoop,
  sphere,
  torusArc,
  wallBlock,
  windowUnit,
  gableDomeMatrices,
  rimDrips,
} from './builders';

export const GB = {
  W: 7.0,
  D: 6.0,
  WALL_H: 3.4,
  ROOF_H: 2.8,
  OVERHANG: 0.5,
  WINDOW_Y: 2.1,
};

export interface GingerbreadHouseInfo {
  group: Group;
  /** Footprint (local, centred) used for pathfinding obstacles. */
  footprint: { w: number; d: number };
  /** Front step / door position in local space. */
  doorLocal: Vector3;
}

export function buildGingerbreadHouse(seed: number): GingerbreadHouseInfo {
  const rng = new RNG(seed);
  const { W, D, WALL_H, ROOF_H, OVERHANG, WINDOW_Y } = GB;
  const group = new Group();
  group.name = 'GingerbreadHouse';

  const eaveX = W / 2 + OVERHANG;
  const halfD = D / 2;
  const roofHalfDepth = (D + 0.8) / 2;

  /* --- Walls and roof ------------------------------------------------------ */
  group.add(mesh(wallBlock(W, WALL_H, D, 0.28), MAT.gingerbread()));
  const roofGeo = gableGeometry(W + OVERHANG * 2, ROOF_H, D + 0.8);
  roofGeo.translate(0, WALL_H, 0);
  group.add(mesh(roofGeo, MAT.gingerbreadDark()));

  /* --- Scalloped royal-icing shingles on both slopes ---------------------- */
  const shingleGeo = domeGeometry(0.27, 0.42);
  const shingles = gableDomeMatrices({
    eaveX,
    wallTop: WALL_H,
    roofH: ROOF_H,
    halfDepth: halfD + 0.25,
    rowSpacing: 0.42,
    colSpacing: 0.5,
    rng,
  });
  group.add(instanced(shingleGeo, MAT.icing(), shingles));

  /* --- Piped icing trim ---------------------------------------------------- */
  const trimR = 0.12;
  const trim: BufferGeometry[] = [];
  for (const zSign of [1, -1]) {
    const zz = zSign * (roofHalfDepth + 0.02);
    trim.push(
      pipedLoop(
        [
          new Vector3(-eaveX, WALL_H + 0.05, zz),
          new Vector3(0, WALL_H + ROOF_H + 0.05, zz),
          new Vector3(eaveX, WALL_H + 0.05, zz),
        ],
        trimR,
        true,
      ),
    );
  }
  for (const side of [1, -1]) {
    trim.push(pipeAlong([new Vector3(side * eaveX, WALL_H + 0.06, -roofHalfDepth), new Vector3(side * eaveX, WALL_H + 0.06, roofHalfDepth)], trimR));
  }
  trim.push(pipeAlong([new Vector3(0, WALL_H + ROOF_H + 0.08, -roofHalfDepth), new Vector3(0, WALL_H + ROOF_H + 0.08, roofHalfDepth)], trimR));
  const o = 0.12;
  trim.push(
    pipedLoop(
      [
        new Vector3(-W / 2 - o, WALL_H - 0.02, -halfD - o),
        new Vector3(W / 2 + o, WALL_H - 0.02, -halfD - o),
        new Vector3(W / 2 + o, WALL_H - 0.02, halfD + o),
        new Vector3(-W / 2 - o, WALL_H - 0.02, halfD + o),
      ],
      0.15,
      true,
    ),
  );
  group.add(mesh(mergeAll(trim), MAT.icing()));

  /* --- Icing drips hanging from the wall-top rim -------------------------- */
  const drips = rimDrips(W, D, WALL_H, rng, { skip: 0.35, spacing: 0.55 });
  group.add(mesh(drips, MAT.icing()));

  /* --- Front facade: door, arch, pillars, windows ------------------------- */
  group.add(mesh(box(1.2, 2.2, 0.14, [0, 1.1, halfD + 0.07]), MAT.chocolate()));
  group.add(mesh(torusArc(0.6, 0.11, Math.PI, [0, 2.2, halfD + 0.1]), MAT.icing()));
  group.add(mesh(sphere(0.09, [0.34, 1.05, halfD + 0.16]), MAT.swirlLemon()));

  for (const sx of [-0.9, 0.9]) {
    group.add(mesh(cylinder(0.14, 0.14, 2.3, [sx, 1.15, halfD + 0.2], 16), MAT.peppermintRed()));
    group.add(mesh(sphere(0.2, [sx, 2.35, halfD + 0.2]), MAT.icing()));
  }

  const frames: BufferGeometry[] = [];
  const glass: BufferGeometry[] = [];
  const addWindow = (x: number, y: number, z: number, rotY: number) => {
    const w = windowUnit(x, y, z, rotY);
    frames.push(...w.frame);
    glass.push(...w.glass);
  };
  for (const x of [-1.95, 1.95]) {
    addWindow(x, WINDOW_Y, halfD - 0.02, 0);
    addWindow(x, WINDOW_Y, -halfD + 0.02, Math.PI);
  }
  for (const z of [-1.3, 1.3]) {
    addWindow(W / 2 - 0.02, WINDOW_Y, z, Math.PI / 2);
    addWindow(-W / 2 + 0.02, WINDOW_Y, z, -Math.PI / 2);
  }
  group.add(mesh(mergeAll(frames), MAT.icing()));
  group.add(mesh(mergeAll(glass), MAT.sugarGlass()));

  /* --- Chimney with marshmallow smoke ------------------------------------- */
  const chimneyX = 1.8;
  const chimneyZ = -1.2;
  const chimneyBase = 3.8;
  group.add(mesh(wallBlock(0.9, 2.7, 0.9, 0.1).translate(chimneyX, chimneyBase, chimneyZ), MAT.gingerbreadDark()));
  group.add(mesh(box(1.15, 0.22, 1.15, [chimneyX, chimneyBase + 2.7, chimneyZ]), MAT.icing()));
  const smoke = [0, 1, 2].map((i) =>
    sphere(0.22 + i * 0.05, [chimneyX + 0.15 * i, chimneyBase + 3.0 + i * 0.45, chimneyZ - 0.1 * i], 0.9, 10),
  );
  group.add(mesh(mergeAll(smoke), MAT.icing(), { cast: false }));

  /* --- Gumdrops on sills, ridge and yard ------------------------------------ */
  const gumColours = ['#ff4d6d', '#ffd23f', '#3dd6a4', '#7b61ff', '#ff9f1c', '#4cc9f0'].map((c) => new Color(c));
  const gumMatrices: Matrix4[] = [];
  const gumColourList: Color[] = [];
  const addGum = (x: number, y: number, z: number, s: number) => {
    gumMatrices.push(new Matrix4().compose(new Vector3(x, y, z), new Quaternion(), new Vector3(s, s * 0.7, s)));
    gumColourList.push(rng.pick(gumColours));
  };
  for (const x of [-1.95, 1.95]) {
    for (let i = -2; i <= 2; i++) addGum(x + i * 0.32, 1.2, halfD + 0.22, 0.14);
  }
  for (let z = -roofHalfDepth + 0.5; z <= roofHalfDepth - 0.5; z += 0.8) addGum(0, WALL_H + ROOF_H + 0.2, z, 0.17);
  for (let i = 0; i < 26; i++) {
    const a = rng.range(0, Math.PI * 2);
    const r = rng.range(halfD + 1.2, halfD + 3.8);
    addGum(Math.cos(a) * r * 1.3, 0.14, Math.sin(a) * r, rng.range(0.14, 0.24));
  }
  const gumGeo = sphere(1, [0, 0, 0], 1, 12);
  group.add(instanced(gumGeo, jellyMaterial('#ffffff', { sugar: 0.5 }), gumMatrices, gumColourList));

  /* --- Cookie path leading out of the front door -------------------------- */
  const cookieGeo = new CylinderGeometry(0.42, 0.42, 0.14, 14, 1);
  cookieGeo.deleteAttribute('uv');
  const cookies: Matrix4[] = [];
  for (let i = 0; i < 7; i++) {
    const z = halfD + 1.6 + i * 0.95;
    const q = new Quaternion().setFromAxisAngle(new Vector3(0, 1, 0), rng.range(0, Math.PI * 2));
    cookies.push(new Matrix4().compose(new Vector3(rng.range(-0.15, 0.15), 0.07, z), q, new Vector3(1, 1, 1)));
  }
  group.add(instanced(cookieGeo, MAT.gingerbread(), cookies));

  return {
    group,
    footprint: { w: W + OVERHANG * 2, d: D + 0.8 },
    doorLocal: new Vector3(0, 0, halfD + 0.3),
  };
}
