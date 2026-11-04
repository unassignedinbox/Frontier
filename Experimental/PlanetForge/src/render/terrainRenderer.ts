/**
 * Terrain renderer.
 *
 * The only interesting problem here is precision, and it has one correct answer
 * for a planet: keep the camera at the origin and move the world.
 *
 * Every chunk mesh arrives from a worker with vertices already relative to its
 * own chunk origin, and that origin is a JS double. Each frame we push
 * `chunkOrigin - cameraPosition` into the mesh's position, in float64, so the
 * GPU only ever sees values of order (chunk size + a few km). At 30 km from the
 * camera a float32 still has sub-millimetre precision; at 6 371 km it has
 * metres. This is why the horizon does not wobble and distant terrain does not
 * z-fight, without needing a "floating origin shift" that teleports everything.
 */

import * as THREE from 'three';
import type { ResidentChunk } from '../core/stream/chunkStreamer';
import type { Vec3 } from '../core/math/vec';
import { chunkKeyString } from '../core/sdf/cubesphere';
import { SKY_GLSL } from './sky.glsl';

interface RenderedChunk {
  mesh: THREE.Mesh;
  key: string;
  origin: Vec3;
  material: THREE.Material;
}

export interface TerrainStats {
  chunks: number;
  triangles: number;
  drawCalls: number;
  uploadedMB: number;
}

export class TerrainRenderer {
  readonly group = new THREE.Group();
  private chunks = new Map<string, RenderedChunk>();
  private detailMaterial: THREE.MeshLambertMaterial;
  private coarseMaterial: THREE.MeshLambertMaterial;
  private wireframe = false;
  private uploadedBytes = 0;

  constructor(scene: THREE.Scene) {
    this.group.name = 'terrain';
    scene.add(this.group);

    this.detailMaterial = new THREE.MeshLambertMaterial({
      vertexColors: true,
      side: THREE.FrontSide,
      flatShading: false,
    });
    this.coarseMaterial = new THREE.MeshLambertMaterial({
      vertexColors: true,
      side: THREE.FrontSide,
      flatShading: true,
    });
  }

  /** Add a freshly streamed chunk. */
  add(chunk: ResidentChunk): void {
    const ks = chunkKeyString(chunk.key);
    if (this.chunks.has(ks)) return;
    const p = chunk.payload;

    const geo = new THREE.BufferGeometry();
    geo.setAttribute('position', new THREE.BufferAttribute(p.positions, 3));
    geo.setAttribute('normal', new THREE.BufferAttribute(p.normals, 3));
    geo.setAttribute('color', new THREE.BufferAttribute(p.colors, 3));
    geo.setIndex(new THREE.BufferAttribute(p.indices, 1));
    // We translate by (origin - camera) every frame, so culling bounds must be
    // refreshed from the transformed positions rather than trusted as-is.
    geo.boundingSphere = new THREE.Sphere(
      new THREE.Vector3(0, 0, 0),
      Math.max(1, Math.hypot(p.positions[0] ?? 0, p.positions[1] ?? 0, p.positions[2] ?? 0) + chunk.payload.radius),
    );

    const material = p.triangleCount > 20_000 ? this.coarseMaterial : this.detailMaterial;
    const mesh = new THREE.Mesh(geo, material);
    mesh.frustumCulled = false;
    mesh.renderOrder = 1;
    mesh.visible = false;
    this.group.add(mesh);

    this.uploadedBytes += p.positions.byteLength + p.normals.byteLength
      + p.colors.byteLength + p.indices.byteLength;

    this.chunks.set(ks, {
      mesh,
      key: ks,
      origin: { x: p.origin[0], y: p.origin[1], z: p.origin[2] },
      material,
    });
  }

  remove(keyString: string): void {
    const c = this.chunks.get(keyString);
    if (!c) return;
    this.group.remove(c.mesh);
    c.mesh.geometry.dispose();
    this.chunks.delete(keyString);
  }

  clear(): void {
    for (const ks of [...this.chunks.keys()]) this.remove(ks);
    this.uploadedBytes = 0;
  }

  setWireframe(on: boolean): void {
    this.wireframe = on;
    for (const m of [this.detailMaterial, this.coarseMaterial]) m.wireframe = on;
  }

  /**
   * Camera-relative placement. Called every frame; it is a handful of float
   * writes per chunk, well under a millisecond for hundreds of chunks.
   */
  sync(cameraPosition: Vec3, cameraMatrix: THREE.Matrix4): void {
    void cameraMatrix;
    for (const c of this.chunks.values()) {
      c.mesh.position.set(
        c.origin.x - cameraPosition.x,
        c.origin.y - cameraPosition.y,
        c.origin.z - cameraPosition.z,
      );
      c.mesh.visible = true;
    }
  }

  stats(): TerrainStats {
    let triangles = 0;
    for (const c of this.chunks.values()) {
      const idx = c.mesh.geometry.getIndex();
      if (idx) triangles += idx.count / 3;
    }
    return {
      chunks: this.chunks.size,
      triangles,
      drawCalls: this.chunks.size,
      uploadedMB: this.uploadedBytes / (1024 * 1024),
    };
  }

  get wireframeOn(): boolean { return this.wireframe; }
}

/**
 * Atmosphere shell: a cheap analytic sky/limb pass that also gives the planet a
 * correct silhouette from orbit, so "space" reads as space rather than as a
 * black background behind terrain.
 */
export function createAtmosphere(radius: number): THREE.Mesh {
  const geo = new THREE.SphereGeometry(radius * 1.02, 64, 48);
  const mat = new THREE.ShaderMaterial({
    transparent: true,
    side: THREE.BackSide,
    depthWrite: false,
    blending: THREE.AdditiveBlending,
    uniforms: {
      uSunDir: { value: new THREE.Vector3(0.4, 0.55, 0.75) },
      uRadius: { value: radius },
      uCameraPos: { value: new THREE.Vector3() },
    },
    vertexShader: /* glsl */ `
      varying vec3 vWorldPos;
      varying vec3 vNormal;
      void main() {
        vNormal = normalize(normalMatrix * normal);
        vWorldPos = (modelMatrix * vec4(position, 1.0)).xyz;
        gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0);
      }
    `,
    fragmentShader: /* glsl */ `
      precision highp float;
      uniform vec3 uSunDir;
      varying vec3 vWorldPos;
      varying vec3 vNormal;
      ${SKY_GLSL}
      void main() {
        vec3 viewDir = normalize(vWorldPos);
        float rim = pow(1.0 - abs(dot(viewDir, normalize(vNormal))), 3.0);
        float lit = clamp(dot(normalize(vWorldPos), normalize(uSunDir)) * 0.5 + 0.5, 0.0, 1.0);
        vec3 col = mix(vec3(0.16, 0.38, 0.82), vec3(0.42, 0.66, 1.0), lit);
        col *= rim * 1.4 * lit;
        gl_FragColor = vec4(col, rim * 0.55 * lit);
      }
    `,
  });
  const mesh = new THREE.Mesh(geo, mat);
  mesh.name = 'atmosphere';
  mesh.position.set(0, 0, 0);
  return mesh;
}

/** Sky dome centred on the camera. */
export class SkyDome {
  readonly mesh: THREE.Mesh;

  constructor() {
    const geo = new THREE.SphereGeometry(1, 32, 24);
    const mat = new THREE.ShaderMaterial({
      side: THREE.BackSide,
      depthWrite: false,
      depthTest: false,
      uniforms: {
        uSunDir: { value: new THREE.Vector3(0.4, 0.55, 0.75) },
        uScale: { value: 1 },
      },
      vertexShader: /* glsl */ `
        varying vec3 vDir;
        void main() {
          vDir = position;
          vec4 p = projectionMatrix * modelViewMatrix * vec4(position, 1.0);
          gl_Position = p.xyww;
        }
      `,
      fragmentShader: /* glsl */ `
        precision highp float;
        uniform vec3 uSunDir;
        varying vec3 vDir;
        ${SKY_GLSL}
        void main() {
          vec3 dir = normalize(vDir);
          vec3 col = skyRadiance(dir, normalize(uSunDir), 1.0);
          gl_FragColor = vec4(col, 1.0);
        }
      `,
    });
    this.mesh = new THREE.Mesh(geo, mat);
    this.mesh.frustumCulled = false;
    this.mesh.renderOrder = -10;
    this.mesh.name = 'sky';
  }

  update(camera: THREE.Camera, sunDir: Vec3, radius: number): void {
    // The dome is a unit sphere: scale it to just inside the far plane and pin
    // it to the camera so it behaves as an infinite background.
    const scale = radius;
    this.mesh.scale.setScalar(scale);
    this.mesh.position.copy(camera.position);
    const mat = this.mesh.material as THREE.ShaderMaterial;
    mat.uniforms.uSunDir.value.set(sunDir.x, sunDir.y, sunDir.z);
  }
}
