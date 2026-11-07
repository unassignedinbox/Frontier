/**
 * Terrain heightfield and the chocolate river ribbon.
 */
import { BufferGeometry, Float32BufferAttribute, Mesh, PlaneGeometry } from 'three';
import { createProcMaterial } from '../materials/procMaterial';
import { MAT } from '../materials/library';
import { RIVER_Y, type WorldLayout } from '../world/layout';

/** Heightfield terrain at 1 m resolution, tinted by zone through vertex colours. */
export function buildTerrain(layout: WorldLayout): Mesh {
  const size = layout.half * 2;
  const seg = Math.round(size);
  const geo = new PlaneGeometry(size, size, seg, seg);
  geo.rotateX(-Math.PI / 2);
  const pos = geo.getAttribute('position');
  const colours = new Float32Array(pos.count * 3);
  for (let i = 0; i < pos.count; i++) {
    const x = pos.getX(i);
    const z = pos.getZ(i);
    pos.setY(i, layout.heightAt(x, z));
    const [r, g, b] = layout.tintAt(x, z);
    colours[i * 3] = r;
    colours[i * 3 + 1] = g;
    colours[i * 3 + 2] = b;
  }
  geo.setAttribute('color', new Float32BufferAttribute(colours, 3));
  geo.computeVertexNormals();

  const mat = createProcMaterial({ kind: 'candyGround', params: { x: 0.08 }, bump: 0.8 });
  mat.vertexColors = true;
  mat.name = 'terrain';
  const mesh = new Mesh(geo, mat);
  mesh.receiveShadow = true;
  mesh.castShadow = false;
  mesh.name = 'Terrain';
  return mesh;
}

/** Flat ribbon of animated chocolate that follows the river centreline. */
export function buildRiver(layout: WorldLayout): Mesh {
  const span = layout.half;
  const step = 0.75;
  const positions: number[] = [];
  const normals: number[] = [];
  const indices: number[] = [];
  let row = 0;
  for (let x = -span; x <= span + 1e-6; x += step) {
    const c = layout.riverCenter(x);
    const hw = layout.riverHalfWidth(x) + 1.0;
    positions.push(x, RIVER_Y, c - hw, x, RIVER_Y, c + hw);
    normals.push(0, 1, 0, 0, 1, 0);
    if (row > 0) {
      const a = (row - 1) * 2;
      const b = row * 2;
      indices.push(a, b, a + 1, a + 1, b, b + 1);
    }
    row++;
  }
  const geo = new BufferGeometry();
  geo.setAttribute('position', new Float32BufferAttribute(positions, 3));
  geo.setAttribute('normal', new Float32BufferAttribute(normals, 3));
  geo.setIndex(indices);
  const mesh = new Mesh(geo, MAT.chocolateRiver());
  mesh.name = 'ChocolateRiver';
  mesh.castShadow = false;
  mesh.receiveShadow = true;
  return mesh;
}

/** Thin confetti-covered road ribbon following the layout road polyline. */
export function buildRoad(layout: WorldLayout): Mesh {
  const positions: number[] = [];
  const normals: number[] = [];
  const indices: number[] = [];
  const pts = layout.road;
  const half = 1.6;
  for (let i = 0; i < pts.length; i++) {
    const a = pts[Math.max(0, i - 1)];
    const b = pts[Math.min(pts.length - 1, i + 1)];
    const dx = b.x - a.x;
    const dz = b.z - a.z;
    const L = Math.hypot(dx, dz) || 1;
    const nx = -dz / L;
    const nz = dx / L;
    const y0 = layout.heightAt(pts[i].x, pts[i].z) + 0.06;
    positions.push(pts[i].x + nx * half, y0, pts[i].z + nz * half, pts[i].x - nx * half, y0, pts[i].z - nz * half);
    normals.push(0, 1, 0, 0, 1, 0);
    if (i > 0) {
      const p = (i - 1) * 2;
      const q = i * 2;
      indices.push(p, q, p + 1, p + 1, q, q + 1);
    }
  }
  const geo = new BufferGeometry();
  geo.setAttribute('position', new Float32BufferAttribute(positions, 3));
  geo.setAttribute('normal', new Float32BufferAttribute(normals, 3));
  geo.setIndex(indices);
  const mat = MAT.candyGroundConfetti();
  const mesh = new Mesh(geo, mat);
  mesh.name = 'Road';
  mesh.receiveShadow = true;
  mesh.castShadow = false;
  return mesh;
}
