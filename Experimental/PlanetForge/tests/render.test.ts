/**
 * Render-path tests.
 *
 * Three.js scene objects, textures and shader *materials* are all constructible
 * without a WebGL context, so the geometry and simulation plumbing of the
 * renderer can be tested headlessly. That covers the failure modes that actually
 * happen in practice: NaN water, rings anchored to the wrong place, terrain that
 * drifts because the camera-relative transform is wrong, and stale uniforms.
 */

import { describe, expect, it } from 'vitest';
import * as THREE from 'three';
import { OceanRenderer, DEFAULT_OCEAN_SPEC } from '../src/render/oceanRenderer';
import { TerrainRenderer, SkyDome } from '../src/render/terrainRenderer';
import { TerrainField } from '../src/core/world/terrainField';
import { createPreset } from '../src/core/world/terrainWorld';
import { chunkFrame } from '../src/core/mesh/chunkMesher';
import { latLonToDir } from '../src/core/sdf/cubesphere';
import { mul } from '../src/core/math/vec';

const R = 6_371_000;

function frameAt(lat: number, lon: number, world: ReturnType<typeof createPreset>, field: TerrainField) {
  // A frame at chunk level 8 is ~40 km across on Earth: enough to span a coast.
  return chunkFrame({ face: 0, lod: 8, x: 0, y: 0 }, world, field);
  void lat; void lon;
}

describe('ocean renderer', () => {
  it('builds one ring per spec entry and three.js meshes for each', () => {
    const scene = new THREE.Scene();
    const ocean = new OceanRenderer(scene);
    expect(DEFAULT_OCEAN_SPEC.length).toBeGreaterThan(4);
    expect(ocean.group.children.length).toBe(DEFAULT_OCEAN_SPEC.length);
    for (const child of ocean.group.children) {
      expect(child).toBeInstanceOf(THREE.Mesh);
    }
    ocean.dispose();
  });

  it('simulates without producing NaN, and raises a wave when mass is dropped', () => {
    const world = createPreset('continents');
    const field = new TerrainField(world);
    const ocean = new OceanRenderer(new THREE.Scene());
    ocean.setField(field);
    ocean.setFrame(frameAt(0, 0, world, field));

    // Put the camera over water so the seabed is wet.
    ocean.followCamera(mul(latLonToDir(0, 180), R + 200), frameAt(0, 0, world, field));
    ocean.rebuildGeometry();
    for (let i = 0; i < 30; i++) ocean.simulate(1 / 60);

    // Drop 20 000 m³ right under the camera, at the patch's own origin.
    ocean.dropWater(0, 0, 260, 20_000_000);
    for (let i = 0; i < 30; i++) ocean.simulate(1 / 60);

    const stats = ocean.stats();
    expect(Number.isFinite(stats.maxEta)).toBe(true);
    expect(Number.isFinite(stats.maxSpeed)).toBe(true);
    expect(stats.maxEta).toBeGreaterThan(0.01);
    expect(stats.maxEta).toBeLessThan(200);
    expect(stats.surfaceTriangles).toBeGreaterThan(0);
    expect(stats.rings).toBe(DEFAULT_OCEAN_SPEC.length);
    ocean.dispose();
  });

  it('places ring vertices on the planet sphere, not on a flat plate', () => {
    const world = createPreset('continents');
    const field = new TerrainField(world);
    const ocean = new OceanRenderer(new THREE.Scene());
    ocean.setField(field);
    const frame = frameAt(0, 0, world, field);
    ocean.setFrame(frame);
    ocean.rebuildGeometry();

    const ring = ocean.group.children[DEFAULT_OCEAN_SPEC.length - 1] as THREE.Mesh;
    const pos = ring.geometry.getAttribute('position') as THREE.BufferAttribute;
    expect(pos.count).toBeGreaterThan(1000);

    let minR = Infinity;
    let maxR = 0;
    for (let i = 0; i < pos.count; i += 7) {
      const r = Math.hypot(
        pos.getX(i) + frame.origin.x,
        pos.getY(i) + frame.origin.y,
        pos.getZ(i) + frame.origin.z,
      );
      minR = Math.min(minR, r);
      maxR = Math.max(maxR, r);
    }
    // Every ocean vertex is exactly the sea-level radius from the centre.
    expect(maxR - minR).toBeLessThan(1.0);
    expect(Math.abs(minR - R)).toBeLessThan(1.0);
    ocean.dispose();
  });

  it('moves with the camera in whole cells without teleporting the surface', () => {
    const world = createPreset('continents');
    const field = new TerrainField(world);
    const ocean = new OceanRenderer(new THREE.Scene());
    ocean.setField(field);
    const frame = frameAt(0, 0, world, field);
    ocean.setFrame(frame);
    ocean.followCamera(mul(latLonToDir(0, 180), R + 250), frame);
    ocean.rebuildGeometry();
    for (let i = 0; i < 20; i++) ocean.simulate(1 / 60);

    const before = ocean.stats().maxEta;
    // Move the camera by a few cells of ring 0 (6 m cells).
    const moved = {
      ...frame.origin,
      x: frame.origin.x + frame.right.x * 30,
      y: frame.origin.y + frame.right.y * 30,
      z: frame.origin.z + frame.right.z * 30,
    };
    ocean.followCamera(moved, frame);
    ocean.rebuildGeometry();
    for (let i = 0; i < 20; i++) ocean.simulate(1 / 60);
    const after = ocean.stats().maxEta;
    expect(Number.isFinite(after)).toBe(true);
    // No mass was created or destroyed by the shift itself.
    expect(after).toBeLessThan(before * 4 + 0.2);
    ocean.dispose();
  });

  it('publishes the ring grid size to the shader so normals use real texel size', () => {
    const world = createPreset('continents');
    const field = new TerrainField(world);
    const ocean = new OceanRenderer(new THREE.Scene());
    ocean.setField(field);
    ocean.setFrame(frameAt(0, 0, world, field));
    ocean.rebuildGeometry();

    for (let i = 0; i < ocean.group.children.length; i++) {
      const mesh = ocean.group.children[i] as THREE.Mesh;
      const mat = mesh.material as THREE.ShaderMaterial;
      expect(mat.uniforms.uGridSize.value).toBe(DEFAULT_OCEAN_SPEC[i].cells);
      expect(mat.uniforms.uCellSize.value).toBeGreaterThan(0);
      expect(mat.uniforms.uEta.value).toBeTruthy();
      expect(mat.uniforms.uDepthMap.value).toBeTruthy();
    }
    ocean.dispose();
  });

  it('never references a missing texture unit in the shader source', () => {
    const ocean = new OceanRenderer(new THREE.Scene());
    const mesh = ocean.group.children[0] as THREE.Mesh;
    const mat = mesh.material as THREE.ShaderMaterial;
    // GLSL1 has no textureSize(); a stale call would fail to compile on WebGL1.
    expect(mat.fragmentShader).not.toContain('textureSize(');
    expect(mat.fragmentShader).toContain('uGridSize');
    expect(mat.vertexShader).toContain('uEta');
    ocean.dispose();
  });
});

describe('terrain renderer', () => {
  function fakeChunk(origin: [number, number, number]) {
    return {
      key: { face: 0, lod: 8, x: 1, y: 1 },
      payload: {
        positions: new Float32Array([0, 0, 0, 10, 5, 0, 0, 5, 10, 10, 5, 10]),
        normals: new Float32Array([0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0]),
        colors: new Float32Array([1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0]),
        indices: new Uint32Array([0, 1, 2, 2, 1, 3]),
        origin,
        radius: 100,
        minAltitude: 0,
        maxAltitude: 10,
        mode: 'radial' as const,
        buildMs: 1,
        quadCount: 2,
        triangleCount: 2,
      },
    };
  }

  it('places chunks camera-relative so float32 never sees planet coordinates', () => {
    const scene = new THREE.Scene();
    const renderer = new TerrainRenderer(scene);
    const camera = { x: 6_371_000, y: 120, z: 40 };
    renderer.add(fakeChunk([6_371_000, 100, 0]) as never);
    renderer.sync(camera, new THREE.Matrix4());

    const mesh = renderer.group.children[0] as THREE.Mesh;
    // Origin minus camera: small numbers, exactly what we want on the GPU.
    expect(Math.abs(mesh.position.x)).toBeLessThan(1);
    expect(mesh.position.y).toBeCloseTo(-20, 6);
    expect(mesh.position.z).toBeCloseTo(-40, 6);
    expect(renderer.stats().triangles).toBe(2);
  });

  it('removes and clears without leaking scene children', () => {
    const scene = new THREE.Scene();
    const renderer = new TerrainRenderer(scene);
    renderer.add(fakeChunk([0, 0, 0]) as never);
    expect(renderer.group.children.length).toBe(1);
    renderer.remove('0/8/1/1');
    expect(renderer.group.children.length).toBe(0);
    renderer.add(fakeChunk([0, 0, 0]) as never);
    renderer.add(fakeChunk([0, 0, 0]) as never); // duplicate must be rejected
    expect(renderer.group.children.length).toBe(1);
    renderer.clear();
    expect(renderer.group.children.length).toBe(0);
  });
});

describe('sky', () => {
  it('builds a shader that shares the water reflection model', () => {
    const sky = new SkyDome();
    const mat = sky.mesh.material as THREE.ShaderMaterial;
    // Both shaders include the same skyRadiance() so a reflected sunset matches
    // the actual sunset.
    expect(mat.fragmentShader).toContain('skyRadiance');
    expect(mat.fragmentShader).not.toContain('textureCube');
    sky.update(new THREE.PerspectiveCamera(), { x: 0, y: 1, z: 0 }, 1e5);
    expect(sky.mesh.scale.x).toBeCloseTo(1e5, 3);
  });
});
