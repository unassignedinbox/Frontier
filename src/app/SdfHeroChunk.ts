import * as THREE from 'three';
import type { SdfWorld, WorldDefinition } from '../world/SdfWorld';
import type { WorldFrame } from './WorldFrame';

/** Near-field voxel chunk: the 3D SDF is converted to triangles in a worker. */
export class SdfHeroChunk {
  readonly mesh: THREE.Mesh;
  readonly halfX = 768;
  readonly halfZ = 768;
  readonly centerY = -500;
  readonly halfY = 5500;
  readonly resolution = 40;

  enabled = true;
  ready = false;
  failed = false;
  vertexCount = 0;
  triangleCount = 0;
  fieldSamples = 0;
  onFailure?: () => void;

  private frame: WorldFrame;
  private worker: Worker;
  private busy = false;
  private pending = false;
  private requestId = 0;
  private definition: WorldDefinition | null = null;
  private worldVersion = -1;
  private centerX = Number.NaN;
  private centerZ = Number.NaN;

  constructor(scene: THREE.Scene, frame: WorldFrame, material: THREE.Material) {
    this.frame = frame;
    this.mesh = new THREE.Mesh(new THREE.BufferGeometry(), material);
    this.mesh.name = 'SDF Marching Cubes hero chunk';
    this.mesh.frustumCulled = false;
    this.mesh.visible = false;
    this.mesh.renderOrder = 0;
    scene.add(this.mesh);

    this.worker = new Worker(new URL('../world/sdf.worker.ts', import.meta.url), { type: 'module' });
    this.worker.onmessage = this.onMessage;
    this.worker.onerror = this.onError;
  }

  setEnabled(value: boolean): void {
    this.enabled = value;
    this.mesh.visible = value && this.ready;
  }

  markDirty(world: SdfWorld): void {
    this.definition = world.cloneDefinition();
    this.worldVersion = world.version;
    if (this.enabled && !this.failed) this.request();
  }

  update(frame: WorldFrame, world: SdfWorld): void {
    this.frame = frame;
    if (!this.enabled || this.failed) return;
    if (this.worldVersion !== world.version || !this.definition) this.markDirty(world);
    const snap = this.halfX * 0.5;
    const cx = Math.round(frame.cameraWorld.x / snap) * snap;
    const cz = Math.round(frame.cameraWorld.z / snap) * snap;
    if (cx !== this.centerX || cz !== this.centerZ) {
      this.centerX = cx;
      this.centerZ = cz;
      this.request();
    }
    this.mesh.position.set(
      this.centerX - frame.cameraWorld.x,
      -frame.cameraWorld.y,
      this.centerZ - frame.cameraWorld.z,
    );
  }

  private request(): void {
    if (!this.definition || this.failed) return;
    if (this.busy) { this.pending = true; return; }
    if (Number.isNaN(this.centerX)) {
      this.centerX = Math.round(this.frame.cameraWorld.x / (this.halfX * 0.5)) * (this.halfX * 0.5);
      this.centerZ = Math.round(this.frame.cameraWorld.z / (this.halfX * 0.5)) * (this.halfX * 0.5);
    }
    this.busy = true;
    this.pending = false;
    this.requestId += 1;
    this.worker.postMessage({
      id: this.requestId,
      definition: this.definition,
      centerX: this.centerX,
      centerZ: this.centerZ,
      halfX: this.halfX,
      halfZ: this.halfZ,
      centerY: this.centerY,
      halfY: this.halfY,
      resolution: this.resolution,
    });
  }

  private onMessage = (event: MessageEvent<any>) => {
    const data = event.data;
    if (data.id !== this.requestId) return; // stale result; a newer request is in flight
    this.busy = false;
    if (data.error) {
      this.failed = true;
      this.ready = false;
      this.mesh.visible = false;
      this.onFailure?.();
    } else {
      const geometry = new THREE.BufferGeometry();
      geometry.setAttribute('position', new THREE.BufferAttribute(new Float32Array(data.positions), 3));
      geometry.setAttribute('normal', new THREE.BufferAttribute(new Float32Array(data.normals), 3));
      geometry.setAttribute('color', new THREE.BufferAttribute(new Float32Array(data.colors), 3));
      geometry.computeBoundingSphere();
      const old = this.mesh.geometry;
      this.mesh.geometry = geometry;
      old.dispose();
      this.vertexCount = data.count;
      this.triangleCount = Math.floor(data.count / 3);
      this.fieldSamples = data.fieldSamples;
      this.ready = true;
      this.mesh.visible = this.enabled;
      this.mesh.position.set(
        data.centerX - this.frame.cameraWorld.x,
        -this.frame.cameraWorld.y,
        data.centerZ - this.frame.cameraWorld.z,
      );
    }
    if (this.pending && !this.failed) this.request();
  };

  private onError = (event: ErrorEvent) => {
    console.error('SDF mesher worker failed', event);
    this.failed = true;
    this.ready = false;
    this.busy = false;
    this.pending = false;
    this.mesh.visible = false;
    this.onFailure?.();
  };

  stats(): string {
    if (this.failed) return 'SDF worker unavailable';
    if (!this.ready) return 'SDF meshing…';
    return `${this.triangleCount.toLocaleString()} SDF tris · ${this.fieldSamples.toLocaleString()} voxels`;
  }

  dispose(): void {
    this.worker.terminate();
    this.mesh.geometry.dispose();
    this.mesh.removeFromParent();
  }
}
