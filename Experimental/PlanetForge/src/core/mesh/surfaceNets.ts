/**
 * Naive Surface Nets (a.k.a. dual contouring on the sample lattice).
 *
 * Why not Marching Cubes: Nets produces one vertex per crossed cell, so the
 * triangle count is roughly a third lower for the same isosurface quality and,
 * crucially, the mesh is watertight by construction — the cracks between chunks
 * that plague blocky MC meshes simply do not exist here. It also emits quads
 * (two triangles) which map cleanly onto an Unreal static-mesh import.
 *
 * Why not heightfield meshing: a crack, a cave or an arch is not single-valued
 * in height. If the authoring tool is going to support the erosion/chemical
 * weathering work later, the extractor has to be volumetric from day one.
 *
 * The heightfield path below is still provided, and still *derived from the same
 * field*: it is a fast batching path for open terrain, used only when the chunk
 * is provably simpler than the SDF (see chunkComplexity).
 */

import type { Vec3 } from '../math/vec';

export interface MeshBuffers {
  /** Interleaved xyz, chunk-local metres. */
  positions: Float32Array;
  normals: Float32Array;
  /** 0..1 vertex tint (land/coast/sea/strata). */
  colors: Float32Array;
  indices: Uint32Array;
  quadCount: number;
  triangleCount: number;
}

export const EMPTY_MESH: MeshBuffers = {
  positions: new Float32Array(0),
  normals: new Float32Array(0),
  colors: new Float32Array(0),
  indices: new Uint32Array(0),
  quadCount: 0,
  triangleCount: 0,
};

export interface SurfaceNetsInput {
  /** Number of cells per side. Samples are (cells+1)³. */
  cells: number;
  /** Chunk-local extent in metres. */
  size: number;
  /** Signed scalar at a lattice sample. Negative inside solid. */
  sample: (i: number, j: number, k: number, x: number, y: number, z: number) => number;
  /**
   * Optional map from lattice-space metres (0..size on each axis) to chunk-local
   * render space. Lets a caller mesh in a Cartesian lattice while placing the
   * result on a sphere, or pack a non-cubic altitude band onto the vertical axis —
   * without changing the topology code or the seam behaviour.
   */
  place?: (x: number, y: number, z: number) => [number, number, number];
  /**
   * Optional per-vertex tint. Receives both the placed render-space point and the
   * lattice-space position, so the caller can look up its own per-column data
   * without inverting the placement.
   */
  tint?: (p: Vec3, lattice: Vec3) => [number, number, number];
  /** Scalar value of the isosurface. */
  iso?: number;
}

/**
 * Extract the isosurface of a signed distance volume.
 *
 * Convention: negative = solid. Vertices are placed at the average of the edge
 * crossings in a cell, which is the "naive" (non-QEF) dual scheme — visually
 * indistinguishable for smooth fields and 5–10× cheaper than solving QEFs.
 */
export function surfaceNets(input: SurfaceNetsInput): MeshBuffers {
  const n = input.cells;
  const s = n + 1;
  const iso = input.iso ?? 0;
  const size = input.size;
  const step = size / n;

  // Sample the lattice once. `field` is indexed [i + j*s + k*s*s].
  const field = new Float32Array(s * s * s);
  let idx = 0;
  for (let k = 0; k < s; k++) {
    for (let j = 0; j < s; j++) {
      for (let i = 0; i < s; i++) {
        field[idx++] = input.sample(i, j, k, i * step, j * step, k * step);
      }
    }
  }

  // Per cell we need two things: whether it straddles the surface, and where its
  // vertex goes. Precompute a column-major scratch of the eight corner values so
  // the three edge loops below share one access pattern.
  const cellInside = new Uint8Array(n * n * n);
  const cellVertex = new Int32Array(n * n * n).fill(-1);
  const posList: number[] = [];
  const colList: number[] = [];
  const stride = s * s;
  const corner = new Float32Array(8);
  // Corner c has lattice offsets (c&1, (c>>1)&1, (c>>2)&1) within its cell.
  const cw = new Int32Array(8 * 3);
  for (let c = 0; c < 8; c++) {
    cw[c * 3] = c & 1;
    cw[c * 3 + 1] = (c >> 1) & 1;
    cw[c * 3 + 2] = (c >> 2) & 1;
  }
  let vcount = 0;

  for (let k = 0; k < n; k++) {
    for (let j = 0; j < n; j++) {
      for (let i = 0; i < n; i++) {
        const base = i + j * s + k * stride;
        corner[0] = field[base];
        corner[1] = field[base + 1];
        corner[3] = field[base + 1 + s];
        corner[2] = field[base + s];
        corner[4] = field[base + stride];
        corner[5] = field[base + 1 + stride];
        corner[7] = field[base + 1 + s + stride];
        corner[6] = field[base + s + stride];

        let inside = 0;
        let outside = 0;
        for (let c = 0; c < 8; c++) {
          if (corner[c] < iso) inside++; else outside++;
        }
        if (inside === 0 || outside === 0) continue;

        const cell = i + j * n + k * n * n;
        cellInside[cell] = 1;

        // Average the crossings of the 12 cube edges.
        let ax = 0, ay = 0, az = 0, cnt = 0;
        for (let e = 0; e < 12; e++) {
          const a = EDGE_CORNERS[e * 2];
          const b = EDGE_CORNERS[e * 2 + 1];
          const fa = corner[a];
          const fb = corner[b];
          if ((fa < iso) === (fb < iso)) continue;
          const t = (iso - fa) / (fb - fa);
          ax += cw[a * 3] + (cw[b * 3] - cw[a * 3]) * t;
          ay += cw[a * 3 + 1] + (cw[b * 3 + 1] - cw[a * 3 + 1]) * t;
          az += cw[a * 3 + 2] + (cw[b * 3 + 2] - cw[a * 3 + 2]) * t;
          cnt++;
        }
        if (cnt === 0) continue;
        const lx = (ax / cnt + i) * step;
        const ly = (ay / cnt + j) * step;
        const lz = (az / cnt + k) * step;
        let px = lx, py = ly, pz = lz;
        if (input.place) [px, py, pz] = input.place(lx, ly, lz);
        cellVertex[cell] = vcount++;
        posList.push(px, py, pz);
        if (input.tint) {
          const c = input.tint({ x: px, y: py, z: pz }, { x: lx, y: ly, z: lz });
          colList.push(c[0], c[1], c[2]);
        } else {
          colList.push(1, 1, 1);
        }
      }
    }
  }

  if (vcount === 0) return { ...EMPTY_MESH };

  const positions = new Float32Array(posList);
  const colors = new Float32Array(colList);

  // One quad per sign-changing lattice edge, joining the four cells around it.
  const indices: number[] = [];
  const cv = (i: number, j: number, k: number): number => cellVertex[i + j * n + k * n * n];

  let quads = 0;
  const quad = (a: number, b: number, c: number, d: number, flip: boolean): void => {
    if (a < 0 || b < 0 || c < 0 || d < 0) return;
    quads++;
    if (flip) indices.push(a, b, c, a, c, d);
    else indices.push(a, c, b, a, d, c);
  };

  for (let k = 0; k < n; k++) {
    for (let j = 0; j < n; j++) {
      for (let i = 0; i < n; i++) {
        const base = i + j * s + k * stride;
        const c0 = field[base];

        // X edge (i -> i+1)
        if ((c0 < iso) !== (field[base + 1] < iso) && j > 0 && k > 0) {
          quad(cv(i, j - 1, k - 1), cv(i, j, k - 1), cv(i, j, k), cv(i, j - 1, k), c0 < iso);
        }
        // Y edge (j -> j+1)
        if ((c0 < iso) !== (field[base + s] < iso) && i > 0 && k > 0) {
          quad(cv(i - 1, j, k - 1), cv(i - 1, j, k), cv(i, j, k), cv(i, j, k - 1), c0 >= iso);
        }
        // Z edge (k -> k+1)
        if ((c0 < iso) !== (field[base + stride] < iso) && i > 0 && j > 0) {
          quad(cv(i - 1, j - 1, k), cv(i - 1, j, k), cv(i, j, k), cv(i, j - 1, k), c0 < iso);
        }
      }
    }
  }

  const indexArray = new Uint32Array(indices);
  return {
    positions,
    colors,
    indices: indexArray,
    normals: computeNormals(positions, indexArray),
    quadCount: quads,
    triangleCount: indexArray.length / 3,
  };
}

/** Cube edges as corner-index pairs. Corner c has offsets (c&1, c>>1&1, c>>2&1). */
const EDGE_CORNERS = new Uint8Array([
  0, 1, 2, 3, 4, 5, 6, 7,   // x-direction edges
  0, 2, 1, 3, 4, 6, 5, 7,   // y-direction edges
  0, 4, 1, 5, 2, 6, 3, 7,   // z-direction edges
]);

/**
 * Area-weighted smooth normals. Chunk borders get normals from one-sided
 * geometry; the shading stage blends them against the analytic SDF gradient so
 * chunk seams do not show as lighting creases.
 */
export function computeNormals(positions: Float32Array, indices: Uint32Array): Float32Array {
  const normals = new Float32Array(positions.length);
  for (let t = 0; t < indices.length; t += 3) {
    const ia = indices[t] * 3;
    const ib = indices[t + 1] * 3;
    const ic = indices[t + 2] * 3;
    const ax = positions[ib] - positions[ia];
    const ay = positions[ib + 1] - positions[ia + 1];
    const az = positions[ib + 2] - positions[ia + 2];
    const bx = positions[ic] - positions[ia];
    const by = positions[ic + 1] - positions[ia + 1];
    const bz = positions[ic + 2] - positions[ia + 2];
    const nx = ay * bz - az * by;
    const ny = az * bx - ax * bz;
    const nz = ax * by - ay * bx;
    normals[ia] += nx; normals[ia + 1] += ny; normals[ia + 2] += nz;
    normals[ib] += nx; normals[ib + 1] += ny; normals[ib + 2] += nz;
    normals[ic] += nx; normals[ic + 1] += ny; normals[ic + 2] += nz;
  }
  for (let i = 0; i < normals.length; i += 3) {
    const l = Math.hypot(normals[i], normals[i + 1], normals[i + 2]) || 1;
    normals[i] /= l; normals[i + 1] /= l; normals[i + 2] /= l;
  }
  return normals;
}

/**
 * Heightfield fallback.
 *
 * Still fed from the SDF (`height(dir)`), never from a stored bitmap. Used only
 * for chunks whose complexity test says the surface is monotonic in radius, so
 * the topology is guaranteed identical to what the volume mesher would produce.
 */
export interface HeightfieldInput {
  cells: number;
  size: number;
  /** Altitude in metres at chunk-local (x,z). */
  height: (x: number, z: number) => number;
  /** Chunk-local base altitude; vertices are relative to this. */
  baseAltitude: number;
  tint?: (x: number, y: number, z: number, altitude: number) => [number, number, number];
  /** Radial offset applied to the whole patch (planet curvature baked in by caller). */
  origin?: Vec3;
}

export function heightfieldMesh(input: HeightfieldInput): MeshBuffers {
  const n = input.cells;
  const s = n + 1;
  const verts = s * s;
  const positions = new Float32Array(verts * 3);
  const colors = new Float32Array(verts * 3);
  const heights = new Float32Array(verts);

  for (let j = 0; j < s; j++) {
    for (let i = 0; i < s; i++) {
      const x = (i / n) * input.size - input.size * 0.5;
      const z = (j / n) * input.size - input.size * 0.5;
      const y = input.height(x, z);
      heights[i + j * s] = y;
      const vi = (i + j * s) * 3;
      positions[vi] = x + (input.origin?.x ?? 0);
      positions[vi + 1] = y - input.baseAltitude + (input.origin?.y ?? 0);
      positions[vi + 2] = z + (input.origin?.z ?? 0);
      const c = input.tint ? input.tint(x, y, z, y) : [1, 1, 1];
      colors[vi] = c[0]; colors[vi + 1] = c[1]; colors[vi + 2] = c[2];
    }
  }

  const indices = new Uint32Array(n * n * 6);
  let o = 0;
  for (let j = 0; j < n; j++) {
    for (let i = 0; i < n; i++) {
      const a = i + j * s;
      const b = i + 1 + j * s;
      const c = i + (j + 1) * s;
      const d = i + 1 + (j + 1) * s;
      indices[o++] = a; indices[o++] = c; indices[o++] = b;
      indices[o++] = b; indices[o++] = c; indices[o++] = d;
    }
  }

  return {
    positions,
    colors,
    indices,
    normals: computeNormals(positions, indices),
    quadCount: n * n,
    triangleCount: indices.length / 3,
  };
}

/** Merge already-built meshes into one buffer set (used by the OBJ exporter). */
export function mergeMeshes(meshes: MeshBuffers[]): MeshBuffers {
  let vTotal = 0;
  let iTotal = 0;
  for (const m of meshes) { vTotal += m.positions.length; iTotal += m.indices.length; }
  const positions = new Float32Array(vTotal);
  const normals = new Float32Array(vTotal);
  const colors = new Float32Array(vTotal);
  const indices = new Uint32Array(iTotal);
  let vo = 0, io = 0, base = 0;
  for (const m of meshes) {
    positions.set(m.positions, vo);
    normals.set(m.normals, vo);
    colors.set(m.colors, vo);
    for (let i = 0; i < m.indices.length; i++) indices[io + i] = m.indices[i] + base;
    base += m.positions.length / 3;
    vo += m.positions.length;
    io += m.indices.length;
  }
  return {
    positions, normals, colors, indices,
    quadCount: meshes.reduce((a, m) => a + m.quadCount, 0),
    triangleCount: indices.length / 3,
  };
}
