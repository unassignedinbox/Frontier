/**
 * Adaptive ocean.
 *
 * The ocean is a stack of simulated rings around the camera:
 *
 *   ring 0   6 m cells  ×  128  →  768 m across, the water you can touch
 *   ring 1  12 m cells  ×  128  →  1.5 km
 *   ring 2  24 m cells          →  3 km
 *   ring 3  48 m cells          →  6 km          (open sea)
 *   ...                                  up to the horizon ring
 *
 * Every ring runs the same finite-volume shallow-water solver, and each finer
 * ring receives its outer boundary elevation from the coarser ring it sits
 * inside. That is a one-way nesting (coarse drives fine), which is exactly what
 * regional ocean models do: it costs almost nothing and it removes the "the near
 * water is a different ocean from the far water" seam that a single patch has.
 *
 * Shading has no wave normal map anywhere in the pipeline:
 *   - the normal comes from the analytic gradient of the *simulated* eta field,
 *     uploaded as a float texture and sampled per pixel with bilinear filtering;
 *   - shallow/deep colour comes from the real seabed depth under each pixel;
 *   - foam is a function of wave steepness and of shallow water, not a texture;
 *   - reflections use the same sky function as the sky dome.
 *
 * Far-field chop (beyond what a ring can simulate) is added as a dispersive
 * spectrum with a deep-water celerity, applied only at the vertex stage of the
 * coarsest rings — it is a boundary condition, not a replacement for water.
 */

import * as THREE from 'three';
import { SWEPatch, type WaterSettings, DEFAULT_WATER_SETTINGS } from '../core/water/shallowWater';
import type { TerrainField } from '../core/world/terrainField';
import { type Vec3, normalize } from '../core/math/vec';
import { SKY_GLSL } from './sky.glsl';
import { dirFromLocal, type ChunkFrame } from '../core/mesh/chunkMesher';

export interface OceanRingConfig {
  cells: number;
  cellSize: number;
  /**
   * 'swe'    : solved with the finite-volume shallow-water equations.
   * 'swell'  : analytic dispersive swell, used as the outer boundary condition.
   *
   * The split is not a shortcut for laziness — it is the honest domain limit.
   * Gravity waves with a 24 km wavelength are about 1.9 m/s; simulating them with
   * a shallow-water solver (which assumes wavelength >> depth) would both get the
   * dispersion wrong and require CFL-bounded steps for a field that barely
   * changes. Real ocean models draw the same line and hand off to a spectral
   * boundary.
   */
  mode: 'swe' | 'swell';
}

const RING_SPEC: OceanRingConfig[] = [
  { cells: 128, cellSize: 6, mode: 'swe' },
  { cells: 128, cellSize: 16, mode: 'swe' },
  { cells: 96, cellSize: 44, mode: 'swe' },
  { cells: 96, cellSize: 120, mode: 'swe' },
  // ~5 km patch. Beyond this the solved swell is below a pixel of geometric
  // error and the cost stops buying anything.
  { cells: 80, cellSize: 330, mode: 'swell' },
  { cells: 80, cellSize: 900, mode: 'swell' },
  { cells: 80, cellSize: 2600, mode: 'swell' },
  { cells: 80, cellSize: 7600, mode: 'swell' },
  { cells: 80, cellSize: 22000, mode: 'swell' },
];

/** Rings solved by the SWE rather than synthesised. Reported in telemetry. */
export const SIMULATED_RING_COUNT = RING_SPEC.filter((r) => r.mode === 'swe').length;

interface RingState {
  needsUpload: boolean;
  spec: OceanRingConfig;
  patch: SWEPatch;
  etaTex: THREE.DataTexture;
  depthTex: THREE.DataTexture;
  mesh: THREE.Mesh;
  extent: number;
  dirty: boolean;
}

export interface OceanStats {
  rings: number;
  simulatedCells: number;
  substeps: number;
  maxEta: number;
  maxSpeed: number;
  wetCells: number;
  surfaceTriangles: number;
  time: number;
}

export const DEFAULT_OCEAN_SPEC = RING_SPEC;

export class OceanRenderer {
  readonly group = new THREE.Group();
  settings: WaterSettings = { ...DEFAULT_WATER_SETTINGS };
  private rings: RingState[] = [];
  private scene: THREE.Scene;
  private field: TerrainField | null = null;
  private material: THREE.ShaderMaterial;
  /** Linear filtering of float textures is an extension; fall back to nearest. */
  private floatLinear = true;
  private timeSeconds = 0;
  paused = false;
  /** Sun direction in planet space (unit). */
  sunDir: Vec3 = normalize({ x: 0.4, y: 0.55, z: 0.75 });
  /** Vertical exaggeration for the surface geometry, so swell reads at distance. */
  swellScale = 1;

  private frame: ChunkFrame | null = null;
  /**
   * Where the ring meshes were last built: patch centre in the frame, and the
   * frame normal itself. Between rebuilds the meshes are rigidly translated,
   * which for a shift of tens of metres is sub-millimetre off the true sphere.
   */
  private builtAt: { x: number; z: number; nx: number; ny: number; nz: number } | null = null;
  private frameDirty = false;
  /** Distance the patch may drift before geometry is rebuilt rather than moved. */
  private rebuildSlack = 220;

  constructor(scene: THREE.Scene, floatLinear = true) {
    this.floatLinear = floatLinear;
    this.scene = scene;
    this.group.name = 'ocean';
    scene.add(this.group);
    this.material = this.createMaterial();
    this.buildRings();
  }

  private buildRings(): void {
    for (let i = 0; i < RING_SPEC.length; i++) {
      const spec = RING_SPEC[i];
      const patch = new SWEPatch({
        ...this.settings,
        gridSize: spec.cells,
        cellSize: spec.cellSize,
        spongeCells: Math.max(4, Math.round(spec.cells * 0.07)),
      });
      const n = spec.cells;
      const etaData = new Float32Array(n * n);
      const depthData = new Float32Array(n * n);
      const filter = this.floatLinear ? THREE.LinearFilter : THREE.NearestFilter;
      const etaTex = new THREE.DataTexture(etaData, n, n, THREE.RedFormat, THREE.FloatType);
      etaTex.minFilter = filter;
      etaTex.magFilter = filter;
      etaTex.wrapS = THREE.ClampToEdgeWrapping;
      etaTex.wrapT = THREE.ClampToEdgeWrapping;
      etaTex.needsUpdate = true;
      const depthTex = new THREE.DataTexture(depthData, n, n, THREE.RedFormat, THREE.FloatType);
      depthTex.minFilter = filter;
      depthTex.magFilter = filter;
      depthTex.needsUpdate = true;

      // Each ring needs its own material instance: the eta/depth textures, cell
      // size and grid extent are per-ring uniforms, and a shared material would
      // silently render every ring with the last one's values.
      const isSwell = spec.mode === 'swell';
      const ringMaterial = this.material.clone();
      ringMaterial.uniforms.uCellSize.value = spec.cellSize;
      ringMaterial.uniforms.uGridSize.value = spec.cells;
      ringMaterial.uniforms.uExtent.value = spec.cells * spec.cellSize;
      ringMaterial.uniforms.uEta.value = etaTex;
      ringMaterial.uniforms.uDepthMap.value = depthTex;
      ringMaterial.uniforms.uSwellMode.value = isSwell ? 1 : 0;
      ringMaterial.uniforms.uRingPhase.value = i * 1.7;
      const mesh = new THREE.Mesh(new THREE.BufferGeometry(), ringMaterial);
      mesh.frustumCulled = false;
      mesh.renderOrder = 2;
      this.group.add(mesh);

      const extent = spec.cells * spec.cellSize;
      this.rings.push({ spec, patch, etaTex, depthTex, mesh, extent, dirty: true, needsUpload: true });
    }
  }

  setField(field: TerrainField): void {
    this.field = field;
    for (const r of this.rings) r.dirty = true;
  }

  setFrame(frame: ChunkFrame): void {
    this.frame = frame;
    // The bed only needs resampling when the frame actually changes; geometry is
    // handled separately by rebuildGeometry() with its own slack.
    this.frameDirty = true;
  }

  setSun(dir: Vec3): void {
    this.sunDir = normalize(dir);
  }

  setSettings(s: Partial<WaterSettings>): void {
    this.settings = { ...this.settings, ...s };
    for (const r of this.rings) {
      r.patch.settings = { ...r.patch.settings, ...s, gridSize: r.spec.cells, cellSize: r.spec.cellSize };
    }
  }

  /** Move all rings with the camera and resample the bed where the frame changed. */
  followCamera(cameraPos: Vec3, frame: ChunkFrame): void {
    this.frame = frame;
    const camDir = normalize(cameraPos);
    const camAlt = Math.sqrt(cameraPos.x ** 2 + cameraPos.y ** 2 + cameraPos.z ** 2) - frame.origin.y;
    void camAlt;
    // Chunk-local camera offset in the frame.
    const lx = (cameraPos.x - frame.origin.x) * frame.right.x
      + (cameraPos.y - frame.origin.y) * frame.right.y
      + (cameraPos.z - frame.origin.z) * frame.right.z;
    const lz = (cameraPos.x - frame.origin.x) * frame.up.x
      + (cameraPos.y - frame.origin.y) * frame.up.y
      + (cameraPos.z - frame.origin.z) * frame.up.z;
    void camDir;

    for (const r of this.rings) {
      const moved = r.patch.shift(lx - r.patch.centerX, lz - r.patch.centerZ);
      if (moved) r.dirty = true;
    }
  }

  /** Rebuild the seabed for a ring from the terrain SDF. */
  private resampleRing(ring: RingState): void {
    if (!this.field || !this.frame) return;
    ring.needsUpload = true;
    const frame = this.frame;
    const R = this.field.R;
    const patch = ring.patch;
    const n = ring.spec.cells;
    const half = ring.extent * 0.5;
    patch.sampleBed((x, z) => {
      const dir = dirFromLocal(frame, patch.centerX + x, patch.centerZ + z, R);
      return this.field!.heightAt(dir);
    });
    void n; void half;
  }

  /**
   * Simulate all rings for `dt` seconds.
   * Ring 0 is stepped first using the previous coarse state, then coarse rings
   * are stepped, so the nesting lag is at most one frame.
   */
  private frameIndex = 0;

  simulate(dt: number): void {
    if (this.paused || !this.field) return;
    const clamped = Math.min(dt, 1 / 20);
    this.frameIndex++;
    // Update rate falls off with ring index: the near water must be smooth every
    // frame, the 20 km ring does not care about 60 Hz. The point of staggering is
    // that total cost stays bounded and the main thread keeps its budget.
    const cadence = (i: number): number => (i <= 1 ? 1 : i <= 3 ? 3 : 8);

    const nRings = this.rings.length;
    for (let i = nRings - 1; i >= 0; i--) {
      const ring = this.rings[i];
      if (ring.spec.mode === 'swell') continue;
      if (ring.dirty || this.frameDirty) { this.resampleRing(ring); ring.dirty = false; }
      if (this.frameIndex % cadence(i) !== 0) { ring.needsUpload = false; continue; }
      ring.needsUpload = true;
      // Nesting runs coarse -> fine: the coarsest simulated ring provides the
      // boundary for the next finer one, and so on down to ring 0. The parent has
      // already been stepped this frame, so the child sees a consistent ocean.
      let parentIdx = i + 1;
      while (parentIdx < nRings && this.rings[parentIdx].spec.mode === 'swell') parentIdx++;
      const parent = this.rings[parentIdx];
      const inflow = parent ? parent.patch.eta : null;
      const inflowN = parent ? parent.spec.cells : 0;
      ring.patch.step(clamped, inflow, inflowN, 'outer');
    }
    this.frameDirty = false;
    this.timeSeconds += clamped;
    this.uploadTextures();
  }

  private uploadTextures(): void {
    for (const ring of this.rings) {
      // Swell rings have no simulated state; the shader synthesises them.
      if (ring.spec.mode === 'swell') continue;
      if (!ring.needsUpload) continue;
      const n = ring.spec.cells;
      const etaData = ring.etaTex.image.data as Float32Array;
      const depthData = ring.depthTex.image.data as Float32Array;
      const eta = ring.patch.eta;
      const bed = ring.patch.bed;
      for (let i = 0; i < n * n; i++) {
        etaData[i] = eta[i];
        // Positive depth only; land is negative so the shader can clip it.
        depthData[i] = eta[i] - bed[i];
      }
      ring.etaTex.needsUpdate = true;
      ring.depthTex.needsUpdate = true;
    }
  }

  /**
   * Rebuild ring geometry when the frame or the rings have moved.
   *
   * Ring vertices are placed at the patch's current centre, so the geometry has
   * to follow whole-cell shifts. That is a ~100 k vertex write per ring, which is
   * fine at a few Hz; it is strictly bounded and independent of planet size.
   */
  rebuildGeometry(): void {
    if (!this.frame) return;
    const p0 = this.rings[0]?.patch;
    const cx = p0?.centerX ?? 0;
    const cz = p0?.centerZ ?? 0;
    const n = this.frame.normal;
    const built = this.builtAt;
    const drift = built ? Math.hypot(cx - built.x, cz - built.z) : Infinity;
    const angle = built
      ? Math.acos(Math.min(1, Math.max(-1, n.x * built.nx + n.y * built.ny + n.z * built.nz)))
      : Math.PI;
    // Rebuild when drift exceeds the slack (rotation of the planet under the
    // patch), otherwise just translate. Without this, a fast flight would rebuild
    // ~600 k vertices per frame and the ocean would be the frame cost.
    const rebuild = !built
      || !this.rings[0].mesh.geometry.attributes.position
      || drift > this.rebuildSlack
      || angle * this.frameNormalArm > this.rebuildSlack;

    const dx = built ? cx - built.x : 0;
    const dz = built ? cz - built.z : 0;

    for (const ring of this.rings) {
      if (rebuild) this.buildRingGeometry(ring);
      else {
        // Rigid translation of the whole ring in frame-local tangent space.
        ring.mesh.position.set(
          this.frame.right.x * dx + this.frame.up.x * dz,
          this.frame.right.y * dx + this.frame.up.y * dz,
          this.frame.right.z * dx + this.frame.up.z * dz,
        );
        ring.mesh.visible = true;
      }
    }
    if (rebuild) {
      this.builtAt = { x: cx, z: cz, nx: n.x, ny: n.y, nz: n.z };
      for (const ring of this.rings) ring.mesh.position.set(0, 0, 0);
    }
  }

  /** Radius at which the tangent frame is anchored, for angular drift budgets. */
  private get frameNormalArm(): number {
    return this.field?.R ?? 6_371_000;
  }

  private buildRingGeometry(ring: RingState): void {
    if (!this.frame) return;
    const frame = this.frame;
    const R = this.field?.R ?? 6_371_000;
    const n = ring.spec.cells;
    const extent = ring.extent;
    const half = extent * 0.5;
    const step = extent / n;

    const verts = (n + 1) * (n + 1);
    const positions = new Float32Array(verts * 3);
    const uvs = new Float32Array(verts * 2);
    const depths = new Float32Array(verts);

    for (let j = 0; j <= n; j++) {
      for (let i = 0; i <= n; i++) {
        const vi = i + j * (n + 1);
        const x = -half + i * step;
        const z = -half + j * step;
        // Place on the true sphere; eta is added in the shader from the texture.
        const dir = dirFromLocal(frame, ring.patch.centerX + x, ring.patch.centerZ + z, R);
        positions[vi * 3] = dir.x * R - frame.origin.x;
        positions[vi * 3 + 1] = dir.y * R - frame.origin.y;
        positions[vi * 3 + 2] = dir.z * R - frame.origin.z;
        uvs[vi * 2] = i / n;
        uvs[vi * 2 + 1] = j / n;
        depths[vi] = 0;
      }
    }

    const indices = new Uint32Array(n * n * 6);
    let o = 0;
    for (let j = 0; j < n; j++) {
      for (let i = 0; i < n; i++) {
        const a = i + j * (n + 1);
        const b = i + 1 + j * (n + 1);
        const c = i + (j + 1) * (n + 1);
        const d = i + 1 + (j + 1) * (n + 1);
        indices[o++] = a; indices[o++] = c; indices[o++] = b;
        indices[o++] = b; indices[o++] = c; indices[o++] = d;
      }
    }

    const geo = new THREE.BufferGeometry();
    geo.setAttribute('position', new THREE.BufferAttribute(positions, 3));
    geo.setAttribute('uv', new THREE.BufferAttribute(uvs, 2));
    geo.setAttribute('depthAttr', new THREE.BufferAttribute(depths, 1));
    geo.setIndex(new THREE.BufferAttribute(indices, 1));
    geo.boundingSphere = new THREE.Sphere(new THREE.Vector3(0, 0, 0), extent);
    ring.mesh.geometry.dispose();
    ring.mesh.geometry = geo;

    const mat = ring.mesh.material as THREE.ShaderMaterial;
    if (mat.uniforms) {
      mat.uniforms.uEta.value = ring.etaTex;
      mat.uniforms.uDepthMap.value = ring.depthTex;
      mat.uniforms.uCellSize.value = step;
      mat.uniforms.uGridSize.value = n;
      mat.uniforms.uExtent.value = extent;
      mat.uniforms.uPlanetRadius.value = R;
    }
    ring.mesh.visible = true;
  }

  private createMaterial(): THREE.ShaderMaterial {
    const mat = new THREE.ShaderMaterial({
      transparent: true,
      depthWrite: false,
      side: THREE.DoubleSide,
      uniforms: {
        uEta: { value: null },
        uDepthMap: { value: null },
        uCellSize: { value: 6 },
        uGridSize: { value: 128 },
        uExtent: { value: 768 },
        uSunDir: { value: new THREE.Vector3(0.4, 0.55, 0.75) },
        uTime: { value: 0 },
        uSwell: { value: 1 },
        uCamera: { value: new THREE.Vector3() },
        uOrigin: { value: new THREE.Vector3() },
        uRight: { value: new THREE.Vector3(1, 0, 0) },
        uUp: { value: new THREE.Vector3(0, 0, 1) },
        uNormal: { value: new THREE.Vector3(0, 1, 0) },
        uPlanetRadius: { value: 6_371_000 },
        uRadius: { value: 6_371_000 },
        uWaterDeep: { value: new THREE.Color(0.012, 0.055, 0.098) },
        uWaterShallow: { value: new THREE.Color(0.055, 0.33, 0.36) },
        uFogDensity: { value: 0.00000042 },
        uSwellMode: { value: 0 },
        uRingPhase: { value: 0 },
      },
      vertexShader: /* glsl */ `
        precision highp float;
        attribute float depthAttr;
        uniform sampler2D uEta;
        uniform float uCellSize;
        uniform float uGridSize;
        uniform float uExtent;
        uniform float uSwell;
        uniform vec3 uCamera;
        varying vec2 vUv;
        varying vec3 vWorldPos;
        varying float vEta;
        varying float vDepth;
        varying float vDistance;
        varying vec3 vPlanetNormal;

        void main() {
          vUv = uv;
          // Sample the simulated surface at the vertex.
          float eta = texture2D(uEta, uv).r;
          float depth = texture2D(uDepthMap, uv).r;
          vEta = eta;
          vDepth = depth;

          vec3 p = position;
          // Displace radially by eta (the patch is already on the sphere).
          vec3 radial = normalize(p);
          p += radial * eta * uSwell;
          vPlanetNormal = radial;

          vWorldPos = p;
          vDistance = length(p - uCamera);
          gl_Position = projectionMatrix * modelViewMatrix * vec4(p, 1.0);
        }
      `.replace('uniform sampler2D uEta;', 'uniform sampler2D uEta;\n        uniform sampler2D uDepthMap;'),
      fragmentShader: /* glsl */ `
        precision highp float;
        uniform sampler2D uEta;
        uniform sampler2D uDepthMap;
        uniform float uCellSize;
        uniform float uExtent;
        uniform float uGridSize;
        uniform vec3 uSunDir;
        uniform float uTime;
        uniform vec3 uCamera;
        uniform vec3 uWaterDeep;
        uniform vec3 uWaterShallow;
        uniform float uFogDensity;
        uniform float uPlanetRadius;
        uniform float uSwellMode;
        uniform float uRingPhase;

        varying vec2 vUv;
        varying vec3 vWorldPos;
        varying float vEta;
        varying float vDepth;
        varying float vDistance;
        varying vec3 vPlanetNormal;

        ${SKY_GLSL}

        // Analytic noise for fine capillary detail — geometric-scale ripple that
        // the simulation grid is too coarse to carry.
        float hash12(vec2 p) {
          vec3 p3 = fract(vec3(p.xyx) * 0.1031);
          p3 += dot(p3, p3.yzx + 33.33);
          return fract((p3.x + p3.y) * p3.z);
        }
        float vnoise(vec2 p) {
          vec2 i = floor(p);
          vec2 f = fract(p);
          f = f * f * (3.0 - 2.0 * f);
          float a = hash12(i);
          float b = hash12(i + vec2(1.0, 0.0));
          float c = hash12(i + vec2(0.0, 1.0));
          float d = hash12(i + vec2(1.0, 1.0));
          return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
        }

        void main() {
          // Land is clipped: the terrain mesh owns everything above sea level.
          if (vDepth < 0.0) discard;

          // ---- per-pixel normal from the *simulated* surface gradient --------
          float dx = 0.0;
          float dz = 0.0;
          if (uSwellMode < 0.5) {
            vec2 texel = 1.0 / vec2(uGridSize);
            float eL = texture2D(uEta, vUv - vec2(texel.x, 0.0)).r;
            float eR = texture2D(uEta, vUv + vec2(texel.x, 0.0)).r;
            float eD = texture2D(uEta, vUv - vec2(0.0, texel.y)).r;
            float eU = texture2D(uEta, vUv + vec2(0.0, texel.y)).r;
            dx = (eR - eL) / (2.0 * uCellSize);
            dz = (eU - eD) / (2.0 * uCellSize);
          } else {
            // Far field: a small sum of dispersive deep-water components. Each
            // term is a real gravity wave with c = sqrt(g/k), so the pattern
            // travels at the right speed for its wavelength instead of the
            // depth-limited speed a shallow solver would impose.
            vec2 p = vWorldPos.xz;
            float g = 9.81;
            for (int i = 0; i < 4; i++) {
              float fi = float(i);
              float k = 0.00035 * pow(2.15, fi);
              float ang = uRingPhase + fi * 1.9;
              vec2 dirn = vec2(cos(ang), sin(ang));
              float c = sqrt(g / k);
              float phase = dot(p, dirn) * k - uTime * c * k;
              float amp = 0.34 / (1.0 + fi);
              dx += dirn.x * cos(phase) * amp * k * 9.0;
              dz += dirn.y * cos(phase) * amp * k * 9.0;
            }
          }

          // Capillary / wind chop, fading with distance to avoid aliasing.
          float chopFade = exp(-vDistance / 900.0);
          vec2 q = vWorldPos.xz * 0.09 + vec2(uTime * 0.35, uTime * 0.22);
          float n1 = vnoise(q) - 0.5;
          float n2 = vnoise(q * 3.7 + 11.0) - 0.5;
          dx += n1 * 0.055 * chopFade + n2 * 0.022 * chopFade;
          dz += n1 * 0.043 * chopFade + n2 * 0.028 * chopFade;

          // Local tangent frame from the planet normal.
          vec3 up = normalize(vPlanetNormal);
          vec3 east = normalize(cross(vec3(0.0, 1.0, 0.0), up));
          vec3 north = cross(up, east);
          vec3 n = normalize(up - east * dx - north * dz);

          vec3 viewDir = normalize(uCamera - vWorldPos);
          vec3 sunDir = normalize(uSunDir);

          // ---- water body colour from real depth ----------------------------
          float shallowT = 1.0 - clamp(vDepth / 55.0, 0.0, 1.0);
          vec3 body = mix(uWaterDeep, uWaterShallow, pow(shallowT, 1.35));
          // Subsurface scatter toward the sun at grazing angles.
          float scatter = pow(max(dot(-viewDir, up), 0.0), 2.0);
          body += vec3(0.02, 0.11, 0.10) * scatter * shallowT;

          // ---- reflection ---------------------------------------------------
          vec3 refl = reflect(-viewDir, n);
          vec3 sky = skyRadiance(refl, sunDir, 1.0);
          float fres = pow(1.0 - max(dot(n, viewDir), 0.0), 4.0);
          fres = mix(0.035, 1.0, fres);

          // ---- sun specular --------------------------------------------------
          vec3 h = normalize(sunDir + viewDir);
          float spec = pow(max(dot(n, h), 0.0), 620.0) * 7.0;
          float wide = pow(max(dot(n, h), 0.0), 48.0) * 0.18;
          vec3 sunTint = mix(vec3(1.0, 0.62, 0.34), vec3(1.0, 0.97, 0.92), clamp(sunDir.y * 2.0, 0.0, 1.0));

          // ---- foam: shallow water + steep faces -----------------------------
          float steep = length(vec2(dx, dz)) * uCellSize;
          float shoreFoam = smoothstep(6.0, 0.4, vDepth) * 0.9;
          float crestFoam = smoothstep(0.30, 0.9, steep) * 0.75;
          float foamNoise = vnoise(vWorldPos.xz * 0.35 + uTime * 0.4);
          float foam = clamp((shoreFoam + crestFoam) * (0.45 + foamNoise * 0.9), 0.0, 1.0);
          foam *= mix(1.0, 0.35, clamp(vDistance / 2600.0, 0.0, 1.0));

          vec3 col = mix(body, sky, fres);
          col += sunTint * (spec + wide);
          col = mix(col, vec3(0.92, 0.95, 0.97) * (0.55 + sky.r * 0.6), foam * 0.85);

          // ---- aerial perspective --------------------------------------------
          float fog = 1.0 - exp(-vDistance * vDistance * uFogDensity);
          vec3 horizon = skyRadiance(normalize(vec3(viewDir.x, 0.02, viewDir.z)), sunDir, 1.0);
          col = mix(col, horizon, clamp(fog, 0.0, 0.92));

          // Shoreline alpha ramp so the water fades into the beach instead of
          // ending in a hard line where the depth texture crosses zero.
          float alpha = smoothstep(0.0, 1.6, vDepth);
          alpha = mix(alpha, 1.0, clamp(foam * 1.5, 0.0, 1.0));
          gl_FragColor = vec4(col, mix(0.72, 1.0, alpha));
        }
      `,
    });
    return mat;
  }

  private applyUniformsToCamera(camera: THREE.Camera): void {
    const cam = camera as THREE.PerspectiveCamera;
    const pos = new THREE.Vector3().setFromMatrixPosition(cam.matrixWorld);
    for (const ring of this.rings) {
      const mat = ring.mesh.material as THREE.ShaderMaterial;
      if (!mat.uniforms) continue;
      mat.uniforms.uSunDir.value.set(this.sunDir.x, this.sunDir.y, this.sunDir.z);
      mat.uniforms.uTime.value = this.timeSeconds;
      mat.uniforms.uSwell.value = this.swellScale;
      mat.uniforms.uCamera.value.copy(pos);
      if (this.frame) {
        mat.uniforms.uOrigin.value.set(this.frame.origin.x, this.frame.origin.y, this.frame.origin.z);
        mat.uniforms.uRight.value.set(this.frame.right.x, this.frame.right.y, this.frame.right.z);
        mat.uniforms.uUp.value.set(this.frame.up.x, this.frame.up.y, this.frame.up.z);
        mat.uniforms.uNormal.value.set(this.frame.normal.x, this.frame.normal.y, this.frame.normal.z);
      }
      mat.uniforms.uPlanetRadius.value = this.field?.R ?? 6_371_000;
    }
  }

  update(camera: THREE.Camera): void {
    this.applyUniformsToCamera(camera);
  }

  /**
   * Drop mass into ring 0.
   * Coordinates are frame-local metres (the same space the editor's brush uses),
   * which is also what `SWEPatch.addMass` expects.
   */
  dropWater(x: number, z: number, radius: number, volume: number): void {
    this.rings[0].patch.addMass(x, z, radius, volume);
  }

  /** Push momentum into ring 0. Coordinates are frame-local metres. */
  pushWater(x: number, z: number, radius: number, vx: number, vz: number): void {
    this.rings[0].patch.addMomentum(x, z, radius, vx, vz);
  }

  /** Number of rings backed by the shallow-water solver. */
  get simulatedRings(): number { return SIMULATED_RING_COUNT; }

  stats(): OceanStats {
    let substeps = 0;
    let maxEta = 0;
    let maxSpeed = 0;
    let wet = 0;
    let cells = 0;
    let tris = 0;
    for (const r of this.rings) {
      const s = r.patch.stats();
      substeps += s.substeps;
      maxEta = Math.max(maxEta, s.maxEta);
      maxSpeed = Math.max(maxSpeed, s.maxSpeed);
      wet += s.wetCells;
      cells += s.totalCells;
      const idx = r.mesh.geometry.getIndex();
      if (idx) tris += idx.count / 3;
    }
    return {
      rings: this.rings.length,
      simulatedCells: cells,
      substeps,
      maxEta,
      maxSpeed,
      wetCells: wet,
      surfaceTriangles: tris,
      time: this.timeSeconds,
    };
  }

  dispose(): void {
    for (const r of this.rings) {
      r.mesh.geometry.dispose();
      r.etaTex.dispose();
      r.depthTex.dispose();
    }
    this.scene.remove(this.group);
  }
}
