/**
 * Shared procedural geometry builders.
 *
 * All meshes are generated on the CPU from seeded noise and parametric
 * formulas: no imported art. Geometry is kept in *local* metres with the
 * origin at the logical pivot (ground centre for buildings, joint for limbs).
 */
import {
  BoxGeometry,
  BufferGeometry,
  Color,
  CatmullRomCurve3,
  Euler,
  CylinderGeometry,
  ExtrudeGeometry,
  Float32BufferAttribute,
  IcosahedronGeometry,
  InstancedMesh,
  Material,
  Matrix4,
  Mesh,
  Quaternion,
  Shape,
  SphereGeometry,
  TorusGeometry,
  TubeGeometry,
  Vector3,
} from 'three';
import { mergeGeometries, mergeVertices } from 'three/examples/jsm/utils/BufferGeometryUtils.js';
import { RoundedBoxGeometry } from 'three/examples/jsm/geometries/RoundedBoxGeometry.js';
import { Simplex3 } from '../core/noise';

const TAU = Math.PI * 2;

/** Ensure a geometry is ready to merge with others (non-indexed, no uv). */
function mergeReady(g: BufferGeometry): BufferGeometry {
  const c = g.index ? g.toNonIndexed() : g.clone();
  if (c.hasAttribute('uv')) c.deleteAttribute('uv');
  if (!c.hasAttribute('normal')) c.computeVertexNormals();
  return c;
}

/** Concatenate many geometries into one draw. */
export function mergeAll(input: BufferGeometry[]): BufferGeometry {
  const list = input.filter((g) => g.getAttribute('position') !== undefined && g.getAttribute('position').count > 0);
  if (list.length === 0) return new BufferGeometry();
  const prepared = list.map(mergeReady);
  const merged = mergeGeometries(prepared, false);
  if (!merged) throw new Error('mergeAll: geometry attribute mismatch');
  prepared.forEach((g) => g.dispose());
  return merged;
}

/** Removes duplicated seam/pole vertices and recomputes smooth normals. */
export function weldSmooth(g: BufferGeometry): BufferGeometry {
  const positionOnly = new BufferGeometry();
  positionOnly.setAttribute('position', g.getAttribute('position'));
  if (g.index) positionOnly.setIndex(g.index);
  const welded = mergeVertices(positionOnly, 1e-4);
  welded.computeVertexNormals();
  return welded;
}

export function mesh(geo: BufferGeometry, mat: Material, opts: { cast?: boolean; receive?: boolean } = {}): Mesh {
  const m = new Mesh(geo, mat);
  m.castShadow = opts.cast ?? true;
  m.receiveShadow = opts.receive ?? true;
  return m;
}

/** Bake a translation / Euler rotation / scale into a geometry (in place). */
export function place(
  g: BufferGeometry,
  pos: [number, number, number],
  rot: [number, number, number] = [0, 0, 0],
  scale: [number, number, number] = [1, 1, 1],
): BufferGeometry {
  const m = new Matrix4().compose(
    new Vector3(pos[0], pos[1], pos[2]),
    new Quaternion().setFromEuler(new Euler(rot[0], rot[1], rot[2], 'YXZ')),
    new Vector3(scale[0], scale[1], scale[2]),
  );
  g.applyMatrix4(m);
  return g;
}

/* -------------------------------------------------------------------------- */
/*                               Organic forms                                */
/* -------------------------------------------------------------------------- */

export interface BeanOptions {
  /** Length along +Y, the bone direction. */
  length: number;
  /** Maximum radius. */
  radius: number;
  /** Cross-section squash on X / Z. */
  squashX?: number;
  squashZ?: number;
  /** Radius multiplier at the far end (1 = no taper). */
  taper?: number;
  rings?: number;
  segments?: number;
  noise?: Simplex3;
  noiseAmp?: number;
  noiseFreq?: number;
}

/**
 * A jellybean-like limb: a superellipse tube whose profile is rounded at both
 * ends. Origin at the base joint, extends along +Y to `length`.
 */
export function beanGeometry(o: BeanOptions): BufferGeometry {
  const rings = o.rings ?? 22;
  const segs = o.segments ?? 18;
  const sx = o.squashX ?? 1;
  const sz = o.squashZ ?? 1;
  const taper = o.taper ?? 1;
  const positions: number[] = [];
  const indices: number[] = [];

  for (let i = 0; i <= rings; i++) {
    const t = i / rings;
    const profile = Math.pow(Math.sin(Math.PI * t), 0.55);
    const r0 = o.radius * profile * (1 + (taper - 1) * t);
    const y = t * o.length;
    for (let j = 0; j <= segs; j++) {
      const a = (j / segs) * TAU;
      const c = Math.cos(a);
      const s = Math.sin(a);
      let r = r0;
      if (o.noise && o.noiseAmp) {
        const n = o.noise.fbm(c * o.noiseFreq! + 3.1, t * 2.2 * o.noiseFreq!, s * o.noiseFreq! + 7.7, 3);
        r *= 1 + o.noiseAmp * n;
      }
      positions.push(c * sx * r, y, s * sz * r);
    }
  }
  for (let i = 0; i < rings; i++) {
    for (let j = 0; j < segs; j++) {
      const a = i * (segs + 1) + j;
      const b = a + segs + 1;
      indices.push(a, b, a + 1, a + 1, b, b + 1);
    }
  }
  const g = new BufferGeometry();
  g.setAttribute('position', new Float32BufferAttribute(positions, 3));
  g.setIndex(indices);
  return weldSmooth(g);
}

/**
 * Noise-displaced blob (gumdrop, canopy, boulder). `freq` sets lumpiness,
 * `amp` the fractional radial variation.
 */
export function blobGeometry(
  radius: number,
  noise: Simplex3,
  opts: { detail?: number; amp?: number; freq?: number; sx?: number; sy?: number; sz?: number; seed?: number } = {},
): BufferGeometry {
  const base = new IcosahedronGeometry(1, opts.detail ?? 2);
  base.deleteAttribute('normal');
  base.deleteAttribute('uv');
  const g = mergeVertices(base, 1e-4);
  const pos = g.getAttribute('position');
  const amp = opts.amp ?? 0.15;
  const freq = opts.freq ?? 1.2;
  const off = (opts.seed ?? 0) * 0.37;
  const v = new Vector3();
  for (let i = 0; i < pos.count; i++) {
    v.fromBufferAttribute(pos, i).normalize();
    const n = noise.fbm(v.x * freq + off, v.y * freq, v.z * freq - off, 3);
    const r = radius * (1 + amp * n);
    pos.setXYZ(i, v.x * r * (opts.sx ?? 1), v.y * r * (opts.sy ?? 1), v.z * r * (opts.sz ?? 1));
  }
  pos.needsUpdate = true;
  g.computeVertexNormals();
  return g;
}

/** Half-dome (scalloped icing) that sits with its flat side on the local XZ plane. */
export function domeGeometry(radius: number, flatten = 0.4): BufferGeometry {
  const g = new SphereGeometry(radius, 12, 6, 0, TAU, 0, Math.PI / 2);
  g.scale(1, flatten, 1);
  g.deleteAttribute('uv');
  return g;
}

/** Tapered, dripping sugar-glaze strand. Top stays at y = 0 and it hangs downwards. */
export function dripGeometry(length: number, radius: number): BufferGeometry {
  const body = new CylinderGeometry(radius, radius * 0.8, length, 8, 1);
  body.translate(0, -length / 2, 0);
  const bulb = new SphereGeometry(radius * 1.35, 8, 6);
  bulb.translate(0, -length, 0);
  return mergeAll([body, bulb]);
}

/** Rounded tube along a closed polyline (icing piping, trim). */
export function pipedLoop(points: Vector3[], radius: number, closed = true): BufferGeometry {
  const curve = new CatmullRomCurve3(points, closed, 'centripetal');
  const g = new TubeGeometry(curve, Math.max(16, points.length * 10), radius, 8, closed);
  g.deleteAttribute('uv');
  return g;
}

/** Open pipe along a sequence of points (e.g. a candy-cane arc). */
export function pipeAlong(points: Vector3[], radius: number): BufferGeometry {
  const curve = new CatmullRomCurve3(points, false, 'centripetal');
  const g = new TubeGeometry(curve, Math.max(12, points.length * 10), radius, 10, false);
  g.deleteAttribute('uv');
  return g;
}

/* -------------------------------------------------------------------------- */
/*                                Architecture                                */
/* -------------------------------------------------------------------------- */

/** Triangular prism for a gable roof: base on y=0, ridge at y=height, extruded along z. */
export function gableGeometry(width: number, height: number, depth: number): BufferGeometry {
  const shape = new Shape();
  shape.moveTo(-width / 2, 0);
  shape.lineTo(width / 2, 0);
  shape.lineTo(0, height);
  shape.closePath();
  const g = new ExtrudeGeometry(shape, { depth, bevelEnabled: false, steps: 1 });
  g.translate(0, 0, -depth / 2);
  g.deleteAttribute('uv');
  return g;
}

/** Soft-cornered wall block with its base at y = 0. */
export function wallBlock(w: number, h: number, d: number, radius: number): BufferGeometry {
  const g = new RoundedBoxGeometry(w, h, d, 3, Math.min(radius, w / 2 - 0.01, d / 2 - 0.01));
  g.translate(0, h / 2, 0);
  g.deleteAttribute('uv');
  return g;
}

export function box(w: number, h: number, d: number, pos: [number, number, number] = [0, 0, 0]): BufferGeometry {
  const g = new BoxGeometry(w, h, d);
  g.translate(pos[0], pos[1], pos[2]);
  g.deleteAttribute('uv');
  return g;
}

export function cylinder(rTop: number, rBottom: number, h: number, pos: [number, number, number] = [0, 0, 0], seg = 16): BufferGeometry {
  const g = new CylinderGeometry(rTop, rBottom, h, seg, 1);
  g.translate(pos[0], pos[1], pos[2]);
  g.deleteAttribute('uv');
  return g;
}

export function sphere(r: number, pos: [number, number, number] = [0, 0, 0], sy = 1, seg = 14): BufferGeometry {
  const g = new SphereGeometry(r, seg, Math.max(6, Math.floor(seg * 0.7)));
  g.scale(1, sy, 1);
  g.translate(pos[0], pos[1], pos[2]);
  g.deleteAttribute('uv');
  return g;
}

export function torusArc(R: number, tube: number, arc: number, pos: [number, number, number], rotX = 0, rotY = 0, rotZ = 0): BufferGeometry {
  const g = new TorusGeometry(R, tube, 8, 24, arc);
  g.rotateX(rotX);
  g.rotateY(rotY);
  g.rotateZ(rotZ);
  g.translate(pos[0], pos[1], pos[2]);
  g.deleteAttribute('uv');
  return g;
}

/* -------------------------------------------------------------------------- */
/*                                 Instancing                                 */
/* -------------------------------------------------------------------------- */

export interface Placement {
  x: number;
  y: number;
  z: number;
  rotY?: number;
  scale?: number;
  sx?: number;
  sy?: number;
  sz?: number;
  /** Optional tilt (radians) around X and Z. */
  tiltX?: number;
  tiltZ?: number;
}

export function placementMatrix(p: Placement): Matrix4 {
  const q = new Quaternion().setFromEuler(new Euler(p.tiltX ?? 0, p.rotY ?? 0, p.tiltZ ?? 0, 'YXZ'));
  const s = p.scale ?? 1;
  const m = new Matrix4().compose(
    new Vector3(p.x, p.y, p.z),
    q,
    new Vector3(s * (p.sx ?? 1), s * (p.sy ?? 1), s * (p.sz ?? 1)),
  );
  return m;
}

export function instanced(geo: BufferGeometry, mat: Material, placements: Placement[] | Matrix4[], colors?: Color[]): InstancedMesh {
  const im = new InstancedMesh(geo, mat, placements.length);
  im.castShadow = true;
  im.receiveShadow = true;
  placements.forEach((p, i) => {
    im.setMatrixAt(i, p instanceof Matrix4 ? p : placementMatrix(p));
    if (colors) im.setColorAt(i, colors[i % colors.length]);
  });
  im.instanceMatrix.needsUpdate = true;
  if (colors && im.instanceColor) im.instanceColor.needsUpdate = true;
  return im;
}

/** Rotate a 2D (x, z) offset by yaw around the Y axis. */
export function rotateXZ(x: number, z: number, yaw: number): [number, number] {
  const c = Math.cos(yaw);
  const s = Math.sin(yaw);
  return [x * c + z * s, -x * s + z * c];
}

/* -------------------------------------------------------------------------- */
/*                     Shared architectural detail helpers                    */
/* -------------------------------------------------------------------------- */

/** Window unit facing outward +Z at the given local position. Returns frame and glass parts. */
export function windowUnit(
  x: number,
  y: number,
  z: number,
  rotY: number,
  size: [number, number] = [1.5, 1.6],
): { frame: BufferGeometry[]; glass: BufferGeometry[] } {
  const [w, h] = size;
  const parts = {
    frame: [
      box(w, h, 0.14, [0, 0, 0.07]),
      box(w * 0.8, 0.08, 0.16, [0, 0, 0.12]),
      box(0.08, h * 0.8, 0.16, [0, 0, 0.12]),
    ],
    glass: [box(w * 0.77, h * 0.78, 0.04, [0, 0, 0.16])],
  };
  for (const g of parts.frame) place(g, [x, y, z], [0, rotY, 0]);
  for (const g of parts.glass) place(g, [x, y, z], [0, rotY, 0]);
  return parts;
}

/**
 * Scalloped dome transforms laid over a gable roof's two slopes.
 * The roof spans x in [-eaveX, eaveX], base at y = wallTop, ridge at wallTop + roofH.
 */
export function gableDomeMatrices(opts: {
  eaveX: number;
  wallTop: number;
  roofH: number;
  halfDepth: number;
  rowSpacing: number;
  colSpacing: number;
  rng: { next(): number; range(a: number, b: number): number; chance(p: number): boolean };
  chance?: number;
}): Matrix4[] {
  const { eaveX, wallTop, roofH, halfDepth, rowSpacing, colSpacing, rng } = opts;
  const slopeLen = Math.hypot(eaveX, roofH);
  const dir = new Vector3(-eaveX, roofH, 0).normalize();
  const normal = new Vector3(dir.y, -dir.x, 0).normalize();
  const out: Matrix4[] = [];
  const rows = Math.floor(slopeLen / rowSpacing);
  const cols = Math.floor((halfDepth * 2 + 0.5) / colSpacing);
  for (const side of [1, -1]) {
    const d = new Vector3(dir.x * side, dir.y, 0);
    const n = new Vector3(normal.x * side, normal.y, 0);
    const eave = new Vector3(eaveX * side, wallTop, 0);
    const orient = new Quaternion().setFromUnitVectors(new Vector3(0, 1, 0), n);
    for (let r = 0; r < rows; r++) {
      const along = (r + 0.7) * rowSpacing;
      if (along > slopeLen - 0.2) continue;
      for (let c = 0; c <= cols; c++) {
        if (opts.chance !== undefined && !rng.chance(opts.chance)) continue;
        const z = -halfDepth - colSpacing * 0.5 + c * colSpacing + (r % 2 ? colSpacing * 0.5 : 0) + rng.range(-0.02, 0.02);
        const p = eave.clone().addScaledVector(d, along).add(new Vector3(0, 0, z)).addScaledVector(n, 0.02);
        out.push(new Matrix4().compose(p, orient, new Vector3(1, 1, 1)));
      }
    }
  }
  return out;
}

/** Icing or chocolate drips hanging from the top rim of a rectangular wall footprint. */
export function rimDrips(
  w: number,
  d: number,
  y: number,
  rng: { next(): number; range(a: number, b: number): number; chance(p: number): boolean },
  opts: { skip?: number; spacing?: number; len?: [number, number]; radius?: [number, number] } = {},
): BufferGeometry {
  const drips: BufferGeometry[] = [];
  const hw = w / 2;
  const hd = d / 2;
  const runs: [number, number, number, number, number, number][] = [
    [-hw, hd, hw, hd, 0, 1],
    [-hw, -hd, hw, -hd, 0, -1],
    [hw, -hd, hw, hd, 1, 0],
    [-hw, -hd, -hw, hd, -1, 0],
  ];
  const spacing = opts.spacing ?? 0.55;
  const len = opts.len ?? [0.25, 0.8];
  const rad = opts.radius ?? [0.07, 0.11];
  for (const [x0, z0, x1, z1, nx, nz] of runs) {
    const L = Math.hypot(x1 - x0, z1 - z0);
    const count = Math.floor(L / spacing);
    for (let i = 0; i <= count; i++) {
      if (rng.chance(opts.skip ?? 0.35)) continue;
      const t = (i + rng.range(0.2, 0.8)) / (count + 1);
      const g = dripGeometry(rng.range(len[0], len[1]), rng.range(rad[0], rad[1]));
      place(g, [x0 + (x1 - x0) * t + nx * 0.12, y - 0.05, z0 + (z1 - z0) * t + nz * 0.12]);
      drips.push(g);
    }
  }
  return drips.length ? mergeAll(drips) : new BufferGeometry();
}
