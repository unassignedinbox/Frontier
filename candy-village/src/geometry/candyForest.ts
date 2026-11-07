/**
 * Candy forest: lollipop trees, candy-cane trees, peppermint pines, gumdrop
 * bushes, chocolate boulders and licorice reeds. Each species is a single
 * procedural geometry instanced per placement, so hundreds of trees cost only
 * a handful of draw calls.
 */
import { BufferGeometry, Color, ConeGeometry, Group, Matrix4 } from 'three';
import { Simplex3 } from '../core/noise';
import { MAT, jellyMaterial } from '../materials/library';
import { blobGeometry, cylinder, instanced, mergeAll, placementMatrix, sphere, torusArc, type Placement } from './builders';
import type { ForestItem, ForestKind } from '../world/layout';

export const FOREST_RADIUS: Record<ForestKind, number> = {
  lollipop: 0.35,
  cane: 0.4,
  pine: 0.7,
  gumdrop: 0.95,
  boulder: 0.9,
  reed: 0,
};

const LOLLI_COLOURS = [MAT.swirlPink, MAT.swirlMint, MAT.swirlLemon, MAT.swirlGrape];
const GUM_COLOURS = ['#ff4d6d', '#ffd23f', '#3dd6a4', '#7b61ff', '#ff9f1c', '#4cc9f0', '#ff7bd5'];

/** Shared procedural geometry per species, built once per world. */
interface ForestGeometries {
  lolliStick: BufferGeometry;
  lolliCanopy: BufferGeometry;
  caneTrunk: BufferGeometry;
  pine: BufferGeometry;
  pineTrunk: BufferGeometry;
  bush: BufferGeometry;
  boulder: BufferGeometry;
  reed: BufferGeometry;
}

function buildGeometries(noise: Simplex3): ForestGeometries {
  const lolliStick = cylinder(0.12, 0.15, 2.9, [0, 1.45, 0], 10);
  const lolliCanopy = blobGeometry(1.35, noise, { detail: 2, amp: 0.13, freq: 0.9, sy: 0.82 });
  lolliCanopy.translate(0, 3.25, 0);

  const caneTrunk = mergeAll([
    cylinder(0.22, 0.27, 4.2, [0, 2.1, 0], 14),
    torusArc(0.6, 0.18, Math.PI * 1.4, [-0.6, 4.2, 0]),
  ]);

  const tiers: BufferGeometry[] = [];
  for (let i = 0; i < 4; i++) {
    const cone = new ConeGeometry(1.55 - i * 0.34, 1.7, 16, 1);
    cone.translate(0, 1.2 + i * 0.95, 0);
    cone.deleteAttribute('uv');
    tiers.push(cone);
  }
  tiers.push(sphere(0.22, [0, 5.1, 0]));
  const pine = mergeAll(tiers);
  const pineTrunk = cylinder(0.2, 0.28, 1.5, [0, 0.75, 0], 10);

  const bush = blobGeometry(0.85, noise, { detail: 2, amp: 0.22, freq: 1.4, sy: 0.75, seed: 3 });
  bush.translate(0, 0.6, 0);
  const boulder = blobGeometry(0.85, noise, { detail: 2, amp: 0.25, freq: 1.1, sy: 0.7, seed: 9 });
  boulder.translate(0, 0.45, 0);
  const reed = cylinder(0.05, 0.06, 1.3, [0, 0.65, 0], 6);

  return { lolliStick, lolliCanopy, caneTrunk, pine, pineTrunk, bush, boulder, reed };
}

/** Builds the forest group from layout placements. */
export function buildForest(items: ForestItem[], noise: Simplex3): Group {
  const geo = buildGeometries(noise);
  const group = new Group();
  group.name = 'CandyForest';

  const byKind = new Map<ForestKind, ForestItem[]>();
  for (const it of items) {
    const list = byKind.get(it.kind) ?? [];
    list.push(it);
    byKind.set(it.kind, list);
  }
  const place = (it: ForestItem): Placement => ({ x: it.x, y: it.y, z: it.z, rotY: it.rotY, scale: it.scale, tiltX: it.tiltX, tiltZ: it.tiltZ });

  // Lollipop trees: a shared stick, with the canopy split by colour variant.
  const lolli = byKind.get('lollipop') ?? [];
  if (lolli.length) {
    group.add(instanced(geo.lolliStick, MAT.licorice(), lolli.map(place)));
    for (let v = 0; v < LOLLI_COLOURS.length; v++) {
      const sub = lolli.filter((it) => it.variant === v);
      if (sub.length) group.add(instanced(geo.lolliCanopy, LOLLI_COLOURS[v](), sub.map(place)));
    }
  }

  const canes = byKind.get('cane') ?? [];
  if (canes.length) group.add(instanced(geo.caneTrunk, MAT.peppermintRed(), canes.map(place)));

  const pines = byKind.get('pine') ?? [];
  if (pines.length) {
    group.add(instanced(geo.pine, MAT.peppermintMint(), pines.map(place)));
    group.add(instanced(geo.pineTrunk, MAT.licorice(), pines.map(place)));
  }

  const bushes = byKind.get('gumdrop') ?? [];
  if (bushes.length) {
    const colours = bushes.map((it) => new Color(GUM_COLOURS[it.variant % GUM_COLOURS.length]));
    group.add(instanced(geo.bush, jellyMaterial('#ffffff', { sugar: 0.75 }), bushes.map(place), colours));
  }

  const boulders = byKind.get('boulder') ?? [];
  if (boulders.length) group.add(instanced(geo.boulder, MAT.chocolate(), boulders.map(place)));

  const reeds = byKind.get('reed') ?? [];
  if (reeds.length) group.add(instanced(geo.reed, MAT.licorice(), reeds.map(place)));

  return group;
}

/** Smallest object-space helper for tests: matrix for a placement. */
export function forestMatrix(it: ForestItem): Matrix4 {
  return placementMatrix({ x: it.x, y: it.y, z: it.z, rotY: it.rotY, scale: it.scale });
}
