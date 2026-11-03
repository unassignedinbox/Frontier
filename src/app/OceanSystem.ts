import * as THREE from 'three';
import { clamp } from '../world/math';
import type { SdfWorld } from '../world/SdfWorld';
import { ShallowWaterSolver, type WaterSettings, type WaterStats } from '../ocean/ShallowWaterSolver';
import { createWaterMaterials } from '../shaders/water';
import { buildRingGeometry } from './RingBuilder';
import type { WorldFrame } from './WorldFrame';

interface FarRing {
  level: number;
  half: number;
  cell: number;
  innerHalf: number;
  centerX: number;
  centerZ: number;
  geometryCenterX: number;
  geometryCenterZ: number;
  mesh: THREE.Mesh;
  vertexCount: number;
  activeCells: number;
  dirty: boolean;
  queued: boolean;
}

export interface OceanBudget {
  simulatedCells: number;
  simulatedVertices: number;
  farVertices: number;
  actualVertices: number;
  uniformVertices: number;
  saving: number;
  maxExtentKm: number;
}

/**
 * Adaptive ocean = simulated near patch + tessellated clipmap rings.
 *
 * Near: finite-volume shallow-water state, one simulated cell per `cellSize`.
 * Far: progressively coarser geometry with analytic open-boundary waves.
 * This is the water analogue of a Nanite/clipmap budget: detail follows the
 * camera instead of spending near-field tessellation across the whole ocean.
 */
export class OceanSystem {
  solver: ShallowWaterSolver;
  nearMesh: THREE.Mesh;
  farRings: FarRing[] = [];
  materials = createWaterMaterials();

  private scene: THREE.Scene;
  private frame: WorldFrame;
  private world: SdfWorld;
  private settings: WaterSettings;
  private nearGeometry: THREE.BufferGeometry;
  private nearN: number;
  private nearDx: number;
  private nearHalf: number;
  private farN = 64;
  private structuralKey = '';
  private lastWorldVersion = -1;
  private farSeaLevel = 0;
  private queue: FarRing[] = [];

  constructor(scene: THREE.Scene, frame: WorldFrame, world: SdfWorld, settings: WaterSettings) {
    this.scene = scene;
    this.frame = frame;
    this.world = world;
    this.settings = settings;
    this.nearN = settings.gridSize;
    this.nearDx = settings.cellSize;
    this.nearHalf = this.nearN * this.nearDx / 2;
    this.farSeaLevel = world.seaLevel;
    this.structuralKey = this.key(settings);

    const cx = Math.round(frame.cameraWorld.x / this.nearDx) * this.nearDx;
    const cz = Math.round(frame.cameraWorld.z / this.nearDx) * this.nearDx;
    this.solver = new ShallowWaterSolver(this.nearN, this.nearDx, world, cx, cz);
    this.nearGeometry = this.createNearGeometry();
    this.nearMesh = new THREE.Mesh(this.nearGeometry, this.materials.near);
    this.nearMesh.name = 'Simulated 2.5D shallow water';
    this.nearMesh.frustumCulled = false;
    this.nearMesh.renderOrder = 3;
    scene.add(this.nearMesh);
    this.createFarRings();
    this.lastWorldVersion = world.version;
  }

  setWorld(world: SdfWorld): void {
    this.world = world;
    this.onWorldChanged();
  }

  onWorldChanged(): void {
    this.solver.rebathymetry(this.world);
    const delta = this.world.seaLevel - this.farSeaLevel;
    if (Math.abs(delta) > 1e-6) {
      for (const ring of this.farRings) {
        const pos = ring.mesh.geometry.getAttribute('position');
        for (let i = 0; i < pos.count; i++) pos.setY(i, pos.getY(i) + delta);
        pos.needsUpdate = true;
      }
      this.farSeaLevel = this.world.seaLevel;
    }
    for (const ring of this.farRings) this.updateFarDepth(ring);
    this.lastWorldVersion = this.world.version;
    this.updateNearGeometry();
  }

  private key(s: WaterSettings): string {
    return `${s.gridSize}|${s.cellSize}|${s.farLevels}`;
  }

  private ensureStructure(settings: WaterSettings): void {
    this.settings = settings;
    const key = this.key(settings);
    if (key !== this.structuralKey) this.rebuild(settings);
  }

  rebuild(settings: WaterSettings = this.settings): void {
    this.settings = settings;
    this.structuralKey = this.key(settings);
    this.nearN = settings.gridSize;
    this.nearDx = settings.cellSize;
    this.nearHalf = this.nearN * this.nearDx / 2;
    this.farSeaLevel = this.world.seaLevel;

    const cx = Math.round(this.frame.cameraWorld.x / this.nearDx) * this.nearDx;
    const cz = Math.round(this.frame.cameraWorld.z / this.nearDx) * this.nearDx;
    this.solver = new ShallowWaterSolver(this.nearN, this.nearDx, this.world, cx, cz);

    const oldNear = this.nearGeometry;
    this.nearGeometry = this.createNearGeometry();
    this.nearMesh.geometry = this.nearGeometry;
    oldNear.dispose();

    for (const ring of this.farRings) {
      ring.mesh.geometry.dispose();
      ring.mesh.removeFromParent();
    }
    this.farRings = [];
    this.queue = [];
    this.createFarRings();
    this.lastWorldVersion = this.world.version;
    this.updateNearGeometry();
  }

  private createNearGeometry(): THREE.BufferGeometry {
    const n = this.nearN;
    const verts = (n + 1) * (n + 1);
    const positions = new Float32Array(verts * 3);
    const wet = new Float32Array(verts);
    const depth = new Float32Array(verts);
    const indices: number[] = [];
    for (let j = 0; j <= n; j++) {
      for (let i = 0; i <= n; i++) {
        const v = i + j * (n + 1);
        positions[v * 3] = (i - n / 2) * this.nearDx;
        positions[v * 3 + 1] = this.world.seaLevel;
        positions[v * 3 + 2] = (j - n / 2) * this.nearDx;
      }
    }
    for (let j = 0; j < n; j++) {
      for (let i = 0; i < n; i++) {
        const a = i + j * (n + 1);
        const b = a + 1;
        const c = a + (n + 1);
        const d = c + 1;
        indices.push(a, c, b, b, c, d);
      }
    }
    const geometry = new THREE.BufferGeometry();
    const posAttr = new THREE.BufferAttribute(positions, 3);
    posAttr.setUsage(THREE.DynamicDrawUsage);
    const wetAttr = new THREE.BufferAttribute(wet, 1);
    wetAttr.setUsage(THREE.DynamicDrawUsage);
    const depthAttr = new THREE.BufferAttribute(depth, 1);
    depthAttr.setUsage(THREE.DynamicDrawUsage);
    geometry.setAttribute('position', posAttr);
    geometry.setAttribute('wet', wetAttr);
    geometry.setAttribute('depth', depthAttr);
    geometry.setIndex(new THREE.Uint32BufferAttribute(indices, 1));
    geometry.boundingSphere = new THREE.Sphere(new THREE.Vector3(0, 0, 0), this.nearHalf * 2 + 1000);
    return geometry;
  }

  private createFarRings(): void {
    const levels = Math.max(1, Math.floor(this.settings.farLevels));
    for (let level = 0; level < levels; level++) {
      const half = this.nearHalf * Math.pow(2, level + 1);
      const cell = (2 * half) / this.farN;
      const snap = Math.max(this.nearDx, cell);
      const centerX = Math.round(this.frame.cameraWorld.x / snap) * snap;
      const centerZ = Math.round(this.frame.cameraWorld.z / snap) * snap;
      const innerHalf = level === 0
        ? this.nearHalf * 0.94
        : this.nearHalf * Math.pow(2, level) * 0.96;
      const geometry = this.buildFarGeometry(half, innerHalf, centerX, centerZ);
      const mesh = new THREE.Mesh(geometry, this.materials.far);
      mesh.name = `Ocean tessellated LOD ${level}`;
      mesh.frustumCulled = false;
      mesh.renderOrder = 2;
      this.scene.add(mesh);
      this.farRings.push({
        level, half, cell, innerHalf, centerX, centerZ,
        geometryCenterX: centerX, geometryCenterZ: centerZ, mesh,
        vertexCount: geometry.getAttribute('position').count,
        activeCells: 0,
        dirty: false, queued: false,
      });
    }
  }

  private buildFarGeometry(half: number, innerHalf: number, centerX: number, centerZ: number): THREE.BufferGeometry {
    const built = buildRingGeometry({
      n: this.farN,
      halfExtent: half,
      innerHalf,
      centerX,
      centerZ,
      heightAt: () => this.world.seaLevel,
      colorAt: () => [1, 1, 1],
      skirtDrop: 3,
    });
    const geometry = built.geometry;
    const pos = geometry.getAttribute('position');
    const depth = new Float32Array(pos.count);
    for (let i = 0; i < pos.count; i++) {
      depth[i] = this.world.seaLevel - this.world.heightAt(centerX + pos.getX(i), centerZ + pos.getZ(i));
    }
    geometry.setAttribute('depth', new THREE.Float32BufferAttribute(depth, 1));
    geometry.boundingSphere = new THREE.Sphere(new THREE.Vector3(0, 0, 0), half * 3);
    return geometry;
  }

  private updateFarDepth(ring: FarRing): void {
    const geometry = ring.mesh.geometry;
    const pos = geometry.getAttribute('position');
    const depthAttr = geometry.getAttribute('depth') as THREE.BufferAttribute | undefined;
    if (!depthAttr || depthAttr.count !== pos.count) {
      const depth = new Float32Array(pos.count);
      for (let i = 0; i < pos.count; i++) {
        depth[i] = this.world.seaLevel - this.world.heightAt(ring.geometryCenterX + pos.getX(i), ring.geometryCenterZ + pos.getZ(i));
      }
      geometry.setAttribute('depth', new THREE.Float32BufferAttribute(depth, 1));
    } else {
      for (let i = 0; i < pos.count; i++) {
        depthAttr.setX(i, this.world.seaLevel - this.world.heightAt(ring.geometryCenterX + pos.getX(i), ring.geometryCenterZ + pos.getZ(i)));
      }
      depthAttr.needsUpdate = true;
    }
  }

  private rebuildFarRing(ring: FarRing): void {
    const geometry = this.buildFarGeometry(ring.half, ring.innerHalf, ring.centerX, ring.centerZ);
    ring.mesh.geometry.dispose();
    ring.mesh.geometry = geometry;
    ring.geometryCenterX = ring.centerX;
    ring.geometryCenterZ = ring.centerZ;
    ring.vertexCount = geometry.getAttribute('position').count;
    ring.dirty = false;
    ring.queued = false;
  }

  private updateNearGeometry(): void {
    const n = this.nearN;
    const dx = this.nearDx;
    const solver = this.solver;
    const pos = this.nearGeometry.getAttribute('position') as THREE.BufferAttribute;
    const wet = this.nearGeometry.getAttribute('wet') as THREE.BufferAttribute;
    const depth = this.nearGeometry.getAttribute('depth') as THREE.BufferAttribute;
    const sea = this.world.seaLevel;
    for (let j = 0; j <= n; j++) {
      const wz = solver.centerZ + (j - n / 2) * dx;
      for (let i = 0; i <= n; i++) {
        const wx = solver.centerX + (i - n / 2) * dx;
        const v = i + j * (n + 1);
        const eta = solver.sampleEta(wx, wz);
        const d = solver.sampleDepth(wx, wz);
        pos.setXYZ(v, (i - n / 2) * dx, sea + eta, (j - n / 2) * dx);
        wet.setX(v, clamp(d / 0.55, 0, 1));
        depth.setX(v, d);
      }
    }
    pos.needsUpdate = true;
    wet.needsUpdate = true;
    depth.needsUpdate = true;
  }

  update(dt: number, frame: WorldFrame, world: SdfWorld, settings: WaterSettings): void {
    this.frame = frame;
    this.world = world;
    this.ensureStructure(settings);
    if (this.lastWorldVersion !== world.version) this.onWorldChanged();

    const cam = frame.cameraWorld;
    const cx = Math.round(cam.x / this.nearDx) * this.nearDx;
    const cz = Math.round(cam.z / this.nearDx) * this.nearDx;
    if (cx !== this.solver.centerX || cz !== this.solver.centerZ) {
      const sx = Math.round((cx - this.solver.centerX) / this.nearDx);
      const sz = Math.round((cz - this.solver.centerZ) / this.nearDx);
      this.solver.shift(sx, sz, world, cx, cz);
    }

    this.solver.advance(dt, settings, world);
    this.updateNearGeometry();
    this.nearMesh.position.set(this.solver.centerX - cam.x, -cam.y, this.solver.centerZ - cam.z);

    const angle = settings.windAngle * Math.PI / 180;
    const windVec = this.materials.uniforms.uWind.value as THREE.Vector2;
    windVec.set(Math.cos(angle) * settings.wind, Math.sin(angle) * settings.wind);
    (this.materials.uniforms.uCameraWorld.value as THREE.Vector2).set(cam.x, cam.z);
    this.materials.uniforms.uTime.value = this.solver.time;
    this.materials.uniforms.uWaveHeight.value = settings.waveHeight;
    this.materials.uniforms.uWaveSpeed.value = 0.65 + settings.wind * 0.8;

    for (const ring of this.farRings) {
      const snap = Math.max(this.nearDx, ring.cell);
      const rx = Math.round(cam.x / snap) * snap;
      const rz = Math.round(cam.z / snap) * snap;
      if (rx !== ring.centerX || rz !== ring.centerZ) {
        ring.centerX = rx;
        ring.centerZ = rz;
        ring.dirty = true;
        if (!ring.queued) {
          ring.queued = true;
          this.queue.push(ring);
        }
      }
      ring.mesh.position.set(ring.centerX - cam.x, -cam.y, ring.centerZ - cam.z);
    }
    let processed = 0;
    while (this.queue.length && processed < 1) {
      const ring = this.queue.shift();
      if (!ring || !ring.dirty) continue;
      this.rebuildFarRing(ring);
      processed++;
    }
  }

  addDrop(x: number, z: number, radius = 24, amplitude = 1.5): void {
    this.solver.addDrop(x, z, radius, amplitude);
  }

  addMomentum(x: number, z: number, radius = 40, vx = 0, vz = 0): void {
    this.solver.addMomentum(x, z, radius, vx, vz);
  }

  storm(x: number, z: number): void {
    this.solver.addDrop(x, z, 120, 2.8);
    this.solver.addMomentum(x, z, 160, 1.2, -0.8);
  }

  surfaceAt(x: number, z: number): number {
    return this.world.seaLevel + this.solver.sampleEta(x, z);
  }

  budget(): OceanBudget {
    const maxHalf = this.nearHalf * Math.pow(2, Math.max(1, this.settings.farLevels));
    const uniformVertices = Math.pow((2 * maxHalf) / this.nearDx + 1, 2);
    const simulatedVertices = (this.nearN + 1) * (this.nearN + 1);
    let farVertices = 0;
    for (const ring of this.farRings) farVertices += ring.vertexCount;
    const actual = simulatedVertices + farVertices;
    return {
      simulatedCells: this.nearN * this.nearN,
      simulatedVertices,
      farVertices,
      actualVertices: actual,
      uniformVertices,
      saving: uniformVertices / Math.max(1, actual),
      maxExtentKm: (maxHalf * 2) / 1000,
    };
  }

  stats(): WaterStats {
    return this.solver.stats();
  }

  get nearExtent(): number { return this.nearHalf * 2; }

  dispose(): void {
    this.nearGeometry.dispose();
    this.nearMesh.removeFromParent();
    for (const ring of this.farRings) {
      ring.mesh.geometry.dispose();
      ring.mesh.removeFromParent();
    }
    this.materials.near.dispose();
    this.materials.far.dispose();
  }
}
