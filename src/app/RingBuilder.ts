import * as THREE from 'three';
import type { RGB } from '../world/math';

/**
 * Geometry clipmap ring builder.
 *
 * Each ring is a square grid centred on a snapped camera position. Cells inside
 * `innerHalf` are omitted so finer rings win. Boundary edges grow vertical
 * skirts, which hide height discontinuities between LODs without wasting
 * triangles across the whole world at the near-field density.
 */

export interface RingGeometryOptions {
  n: number;
  halfExtent: number;
  innerHalf: number;
  centerX: number;
  centerZ: number;
  heightAt: (worldX: number, worldZ: number) => number;
  colorAt: (worldX: number, worldZ: number, height: number) => RGB;
  skirtDrop: number;
}

export interface RingGeometryResult {
  geometry: THREE.BufferGeometry;
  vertexCount: number;
  activeCells: number;
  cellSize: number;
}

export function buildRingGeometry(opts: RingGeometryOptions): RingGeometryResult {
  const { n, halfExtent, innerHalf, centerX, centerZ, heightAt, colorAt, skirtDrop } = opts;
  const cell = (2 * halfExtent) / n;
  const stride = n + 1;
  const positions: number[] = [];
  const colors: number[] = [];
  const indices: number[] = [];
  const active = new Uint8Array(n * n);

  const localAt = (i: number) => -halfExtent + i * cell;
  const centerActive = (i: number, j: number) => {
    if (i < 0 || j < 0 || i >= n || j >= n) return false;
    const cx = localAt(i) + cell * 0.5;
    const cz = localAt(j) + cell * 0.5;
    return Math.abs(cx) > innerHalf || Math.abs(cz) > innerHalf;
  };

  for (let j = 0; j <= n; j++) {
    const lz = localAt(j);
    const wz = centerZ + lz;
    for (let i = 0; i <= n; i++) {
      const lx = localAt(i);
      const wx = centerX + lx;
      const h = heightAt(wx, wz);
      positions.push(lx, h, lz);
      const c = colorAt(wx, wz, h);
      colors.push(c[0], c[1], c[2]);
    }
  }

  for (let j = 0; j < n; j++) {
    for (let i = 0; i < n; i++) {
      const isActive = centerActive(i, j);
      active[i + j * n] = isActive ? 1 : 0;
      if (!isActive) continue;
      const a = i + j * stride;
      const b = a + 1;
      const c = a + stride;
      const d = c + 1;
      // Up-facing winding in Three.js x/y/z coordinates.
      indices.push(a, c, b, b, c, d);
    }
  }

  const pushSkirt = (v0: number, v1: number) => {
    const p0 = v0 * 3;
    const p1 = v1 * 3;
    const x0 = positions[p0], y0 = positions[p0 + 1], z0 = positions[p0 + 2];
    const x1 = positions[p1], y1 = positions[p1 + 1], z1 = positions[p1 + 2];
    const bottom = Math.min(y0, y1) - skirtDrop;
    const base = positions.length / 3;
    positions.push(
      x0, y0, z0,
      x1, y1, z1,
      x0, bottom, z0,
      x1, bottom, z1,
    );
    const c0: RGB = [colors[p0], colors[p0 + 1], colors[p0 + 2]];
    const c1: RGB = [colors[p1], colors[p1 + 1], colors[p1 + 2]];
    const dark = 0.55;
    colors.push(
      c0[0], c0[1], c0[2],
      c1[0], c1[1], c1[2],
      c0[0] * dark, c0[1] * dark, c0[2] * dark,
      c1[0] * dark, c1[1] * dark, c1[2] * dark,
    );
    indices.push(base, base + 2, base + 1, base + 1, base + 2, base + 3);
  };

  let activeCells = 0;
  for (let j = 0; j < n; j++) {
    for (let i = 0; i < n; i++) {
      if (!active[i + j * n]) continue;
      activeCells++;
      const a = i + j * stride;
      const b = a + 1;
      const c = a + stride;
      const d = c + 1;
      if (!centerActive(i - 1, j)) pushSkirt(a, c);
      if (!centerActive(i + 1, j)) pushSkirt(b, d);
      if (!centerActive(i, j - 1)) pushSkirt(a, b);
      if (!centerActive(i, j + 1)) pushSkirt(c, d);
    }
  }

  const geometry = new THREE.BufferGeometry();
  geometry.setAttribute('position', new THREE.Float32BufferAttribute(positions, 3));
  geometry.setAttribute('color', new THREE.Float32BufferAttribute(colors, 3));
  geometry.setIndex(indices);
  geometry.computeVertexNormals();
  geometry.boundingSphere = new THREE.Sphere(new THREE.Vector3(0, 0, 0), halfExtent * 3 + skirtDrop);
  return {
    geometry,
    vertexCount: positions.length / 3,
    activeCells,
    cellSize: cell,
  };
}
