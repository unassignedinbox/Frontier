/// <reference lib="webworker" />
import { MeshBasicMaterial } from 'three';
import { MarchingCubes } from 'three/examples/jsm/objects/MarchingCubes.js';
import { SdfWorld, type WorldDefinition } from './SdfWorld';

/**
 * Converts the 3D SDF into a real triangle mesh with Marching Cubes.
 * Runs off the UI thread so continent blockout edits and camera flight do not
 * stall while the voxel field is sampled.
 */

interface MeshRequest {
  id: number;
  definition: WorldDefinition;
  centerX: number;
  centerZ: number;
  halfX: number;
  halfZ: number;
  centerY: number;
  halfY: number;
  resolution: number;
}

const scope = self as unknown as DedicatedWorkerGlobalScope;

scope.onmessage = (event: MessageEvent<MeshRequest>) => {
  const req = event.data;
  try {
    const world = SdfWorld.fromJSON(req.definition);
    const size = req.resolution;
    const half = size / 2;
    const size2 = size * size;
    const bounds = { cx: req.centerX, cz: req.centerZ, halfX: req.halfX * 0.985, halfZ: req.halfZ * 0.985 };

    const mc = new MarchingCubes(size, new MeshBasicMaterial(), false, true, 30000);
    mc.isolation = 0;
    mc.reset();
    const field = (mc as any).field as Float32Array;
    const palette = (mc as any).palette as Float32Array;

    for (let z = 0; z < size; z++) {
      const fz = (z - half) / half;
      const wz = req.centerZ + fz * req.halfZ;
      for (let y = 0; y < size; y++) {
        const fy = (y - half) / half;
        const wy = req.centerY + fy * req.halfY;
        for (let x = 0; x < size; x++) {
          const fx = (x - half) / half;
          const wx = req.centerX + fx * req.halfX;
          const idx = size2 * z + size * y + x;
          field[idx] = world.sdf3D(wx, wy, wz, bounds);
          const h = world.heightAt(wx, wz);
          const d = world.landSdf(wx, wz);
          const c = world.colorAt(wx, wz, h, d);
          palette[idx * 3] = c[0];
          palette[idx * 3 + 1] = c[1];
          palette[idx * 3 + 2] = c[2];
        }
      }
    }

    mc.update();
    const count = (mc as any).count as number; // vertex count, non-indexed
    const posAttr = mc.geometry.getAttribute('position');
    const positions = new Float32Array(count * 3);
    const normals = new Float32Array(count * 3);
    const colors = new Float32Array(count * 3);
    const eps = Math.max(8, req.halfX / size * 0.75);

    for (let v = 0; v < count; v++) {
      const px = posAttr.getX(v);
      const py = posAttr.getY(v);
      const pz = posAttr.getZ(v);
      const wx = req.centerX + px * req.halfX;
      const wy = req.centerY + py * req.halfY;
      const wz = req.centerZ + pz * req.halfZ;
      positions[v * 3] = px * req.halfX;
      positions[v * 3 + 1] = wy;
      positions[v * 3 + 2] = pz * req.halfZ;
      const n = world.sdfGradient(wx, wy, wz, eps, bounds);
      normals[v * 3] = n[0];
      normals[v * 3 + 1] = n[1];
      normals[v * 3 + 2] = n[2];
      const h = world.heightAt(wx, wz);
      const d = world.landSdf(wx, wz);
      const c = world.colorAt(wx, wz, h, d);
      colors[v * 3] = c[0];
      colors[v * 3 + 1] = c[1];
      colors[v * 3 + 2] = c[2];
    }

    scope.postMessage({
      id: req.id,
      centerX: req.centerX,
      centerZ: req.centerZ,
      count,
      positions: positions.buffer,
      normals: normals.buffer,
      colors: colors.buffer,
      fieldSamples: size * size * size,
      resolution: size,
    }, [positions.buffer, normals.buffer, colors.buffer]);
  } catch (error) {
    scope.postMessage({ id: req.id, error: error instanceof Error ? error.message : String(error) });
  }
};
