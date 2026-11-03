import * as THREE from 'three';
import { clamp, type RGB } from '../world/math';
import type { SdfWorld } from '../world/SdfWorld';
import { buildRingGeometry } from './RingBuilder';
import type { WorldFrame } from './WorldFrame';

interface TerrainRing {
  level: number;
  half: number;
  cell: number;
  innerHalf: number;
  centerX: number;
  centerZ: number;
  mesh: THREE.Mesh;
  vertexCount: number;
  activeCells: number;
  dirty: boolean;
  queued: boolean;
}

const LOD_TINTS: RGB[] = [
  [1.00, 0.92, 0.92], [0.92, 1.00, 0.92], [0.90, 0.94, 1.00],
  [1.00, 0.96, 0.86], [0.96, 0.88, 1.00], [0.86, 1.00, 0.98],
  [1.00, 0.88, 0.88], [0.88, 0.92, 1.00], [0.98, 1.00, 0.88],
  [1.00, 0.90, 0.98],
];

/** Streaming terrain clipmaps generated from the SDF height projection. */
export class TerrainStreamer {
  readonly rings: TerrainRing[] = [];
  readonly material: THREE.MeshStandardMaterial;
  showLodColors = false;
  heroReady = false;
  heroHalf = 768;

  private scene: THREE.Scene;
  private frame: WorldFrame;
  private world: SdfWorld;
  private queue: TerrainRing[] = [];
  private readonly n = 64;
  private readonly levelCount = 10;
  private readonly baseHalf = 768;

  constructor(scene: THREE.Scene, frame: WorldFrame, world: SdfWorld) {
    this.scene = scene;
    this.frame = frame;
    this.world = world;
    this.material = new THREE.MeshStandardMaterial({
      vertexColors: true,
      roughness: 0.94,
      metalness: 0,
      side: THREE.DoubleSide,
      flatShading: false,
      fog: true,
    });

    for (let level = 0; level < this.levelCount; level++) {
      const half = this.baseHalf * Math.pow(2, level);
      const cell = (2 * half) / this.n;
      const snap = Math.max(16, cell);
      const centerX = Math.round(frame.cameraWorld.x / snap) * snap;
      const centerZ = Math.round(frame.cameraWorld.z / snap) * snap;
      const mesh = new THREE.Mesh(new THREE.BufferGeometry(), this.material);
      mesh.name = `Terrain LOD ${level}`;
      mesh.frustumCulled = false;
      mesh.renderOrder = 0;
      scene.add(mesh);
      const ring: TerrainRing = {
        level, half, cell,
        innerHalf: level === 0 ? 0 : this.baseHalf * Math.pow(2, level - 1) * 0.96,
        centerX, centerZ, mesh,
        vertexCount: 0, activeCells: 0, dirty: true, queued: false,
      };
      this.rings.push(ring);
    }
    this.buildAllNow();
  }

  setWorld(world: SdfWorld): void {
    this.world = world;
    this.markDirty();
  }

  setHeroReady(ready: boolean): void {
    if (this.heroReady === ready) return;
    this.heroReady = ready;
    this.markDirty();
  }

  setWireframe(value: boolean): void { this.material.wireframe = value; }

  setDebugLod(value: boolean): void {
    this.showLodColors = value;
    this.markDirty();
  }

  markDirty(): void {
    for (const ring of this.rings) {
      if (!ring.dirty) ring.dirty = true;
      if (!ring.queued) {
        ring.queued = true;
        this.queue.push(ring);
      }
    }
  }

  private innerHalfFor(ring: TerrainRing): number {
    if (ring.level === 0) return this.heroReady ? this.heroHalf * 0.86 : 0;
    return this.baseHalf * Math.pow(2, ring.level - 1) * 0.96;
  }

  private rebuild(ring: TerrainRing): void {
    ring.innerHalf = this.innerHalfFor(ring);
    const tint = LOD_TINTS[ring.level % LOD_TINTS.length];
    const result = buildRingGeometry({
      n: this.n,
      halfExtent: ring.half,
      innerHalf: ring.innerHalf,
      centerX: ring.centerX,
      centerZ: ring.centerZ,
      heightAt: (wx, wz) => this.world.heightAt(wx, wz),
      colorAt: (wx, wz, h) => {
        const d = this.world.landSdf(wx, wz);
        const c = this.world.colorAt(wx, wz, h, d);
        if (!this.showLodColors) return c;
        return [
          clamp(c[0] * tint[0], 0, 1),
          clamp(c[1] * tint[1], 0, 1),
          clamp(c[2] * tint[2], 0, 1),
        ];
      },
      skirtDrop: Math.max(120, ring.cell * 1.5),
    });
    ring.mesh.geometry.dispose();
    ring.mesh.geometry = result.geometry;
    ring.vertexCount = result.vertexCount;
    ring.activeCells = result.activeCells;
    ring.dirty = false;
    ring.queued = false;
  }

  private buildAllNow(): void {
    for (const ring of this.rings) {
      ring.queued = false;
      this.rebuild(ring);
    }
    this.queue.length = 0;
  }

  update(_dt: number, frame: WorldFrame): void {
    this.frame = frame;
    const cam = frame.cameraWorld;
    for (const ring of this.rings) {
      const snap = Math.max(16, ring.cell);
      const cx = Math.round(cam.x / snap) * snap;
      const cz = Math.round(cam.z / snap) * snap;
      if (cx !== ring.centerX || cz !== ring.centerZ) {
        ring.centerX = cx;
        ring.centerZ = cz;
        ring.dirty = true;
        if (!ring.queued) {
          ring.queued = true;
          this.queue.push(ring);
        }
      }
      ring.mesh.position.set(ring.centerX - cam.x, -cam.y, ring.centerZ - cam.z);
    }

    let processed = 0;
    while (this.queue.length > 0 && processed < 2) {
      const ring = this.queue.shift();
      if (!ring || !ring.dirty) continue;
      this.rebuild(ring);
      processed++;
    }
  }

  stats(): { vertices: number; activeCells: number; rings: number; maxExtentKm: number } {
    let vertices = 0;
    let cells = 0;
    for (const r of this.rings) { vertices += r.vertexCount; cells += r.activeCells; }
    return {
      vertices,
      activeCells: cells,
      rings: this.rings.length,
      maxExtentKm: (this.rings[this.rings.length - 1].half * 2) / 1000,
    };
  }

  dispose(): void {
    for (const ring of this.rings) {
      ring.mesh.geometry.dispose();
      ring.mesh.removeFromParent();
    }
    this.material.dispose();
    void this.scene;
  }
}
