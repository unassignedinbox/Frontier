import * as THREE from 'three';
import { UI } from './UI';
import { WorldFrame } from './WorldFrame';
import { CameraRig } from './CameraRig';
import { InputController } from './InputController';
import { TerrainStreamer } from './TerrainStreamer';
import { SdfHeroChunk } from './SdfHeroChunk';
import { OceanSystem } from './OceanSystem';
import { DebugOverlay, type LodExtent } from './DebugOverlay';
import { PlanetMinimap } from './PlanetMinimap';
import {
  exportHeightmapBundle, exportSDFJSON, exportVisibleTerrainOBJ, importSDFJSON,
} from './Exporters';
import {
  DEFAULT_EDITOR_SETTINGS, TOOL_INFO, type EditorSettings,
} from './settings';
import {
  createAfricaPreset, createArchipelagoPreset, createFlatWorld,
  createRandomContinent, SdfWorld, type BrushSettings, type SculptTool,
} from '../world/SdfWorld';
import { clamp, formatLatLon, formatMetres } from '../world/math';

const WORLD_PARAM_KEYS = new Set(['seaLevel', 'coastNoise', 'oceanDepth', 'landElevation'] as const);
const STRUCTURAL_WATER_KEYS = new Set(['gridSize', 'cellSize', 'farLevels'] as const);

type Mode = 'editor' | 'viewer';

interface PickResult {
  x: number;
  y: number;
  z: number;
}

function createSky(scene: THREE.Scene): void {
  const material = new THREE.ShaderMaterial({
    uniforms: { uSunDir: { value: new THREE.Vector3(0.38, 0.86, 0.34).normalize() } },
    vertexShader: /* glsl */`
      varying vec3 vDir;
      void main() {
        vDir = normalize(position);
        gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0);
      }
    `,
    fragmentShader: /* glsl */`
      precision highp float;
      uniform vec3 uSunDir;
      varying vec3 vDir;
      void main() {
        float h = clamp(vDir.y * 0.5 + 0.5, 0.0, 1.0);
        vec3 horizon = vec3(0.62, 0.74, 0.83);
        vec3 zenith = vec3(0.12, 0.30, 0.52);
        vec3 col = mix(horizon, zenith, pow(h, 0.7));
        float sun = pow(max(dot(normalize(vDir), normalize(uSunDir)), 0.0), 420.0);
        float glow = pow(max(dot(normalize(vDir), normalize(uSunDir)), 0.0), 8.0);
        col += vec3(1.0, 0.86, 0.62) * sun * 1.4 + vec3(0.30, 0.22, 0.12) * glow;
        gl_FragColor = vec4(col, 1.0);
      }
    `,
    side: THREE.BackSide,
    depthWrite: false,
    depthTest: false,
    fog: false,
  });
  const mesh = new THREE.Mesh(new THREE.SphereGeometry(900000, 32, 16), material);
  mesh.name = 'Procedural sky';
  mesh.frustumCulled = false;
  mesh.renderOrder = -100;
  scene.add(mesh);
}

export class FrontierApp {
  root: HTMLElement;
  mode: Mode;
  settings: EditorSettings;
  world: SdfWorld;
  frame = new WorldFrame();
  camera: THREE.PerspectiveCamera;
  rig: CameraRig;
  renderer: THREE.WebGLRenderer;
  scene = new THREE.Scene();
  ui: UI;
  terrain: TerrainStreamer;
  hero: SdfHeroChunk;
  ocean: OceanSystem;
  debug: DebugOverlay;
  minimap: PlanetMinimap;
  input: InputController;

  private hoverWorld = new THREE.Vector3();
  private hoverValid = false;
  private pointerInside = false;
  private activeStrokeId: string | null = null;
  private lastWaterPos = new THREE.Vector3();
  private lastPaintRebuild = 0;
  private lastDropTime = 0;
  private raycaster = new THREE.Raycaster();
  private ndc = new THREE.Vector2();
  private lastTime = performance.now();
  private fps = 60;
  private statAccum = 0;
  private saveTimer = 0;
  private startCamera = { x: -1350000, y: 260, z: 720000 };

  constructor(root: HTMLElement, mode: Mode) {
    this.root = root;
    this.mode = mode;
    this.settings = { ...DEFAULT_EDITOR_SETTINGS, ...this.loadSettings() };
    this.world = this.loadWorld() ?? new SdfWorld(createAfricaPreset());
    this.syncSettingsFromWorld();
    this.frame.setOrigin(this.world.def.originLon, this.world.def.originLat);
    this.computeStartCamera();
    this.frame.setCameraWorld(this.startCamera.x, this.startCamera.y, this.startCamera.z);

    this.camera = new THREE.PerspectiveCamera(65, window.innerWidth / window.innerHeight, 0.5, 2_000_000);
    this.rig = new CameraRig(this.camera, this.frame);
    this.rig.speed = this.settings.cameraSpeed;

    this.ui = new UI(root, mode, this);
    this.renderer = new THREE.WebGLRenderer({
      canvas: this.ui.canvas,
      antialias: true,
      alpha: false,
      powerPreference: 'high-performance',
      logarithmicDepthBuffer: true,
    });
    this.renderer.setPixelRatio(Math.min(1.75, window.devicePixelRatio || 1));
    this.renderer.setSize(window.innerWidth, window.innerHeight, false);
    this.renderer.outputColorSpace = THREE.SRGBColorSpace;
    this.renderer.toneMapping = THREE.ACESFilmicToneMapping;
    this.renderer.toneMappingExposure = 1.08;
    this.scene.background = new THREE.Color(0x91b3ca);
    this.scene.fog = new THREE.FogExp2(0x9ab8cc, 0.0000028);
    createSky(this.scene);
    const hemi = new THREE.HemisphereLight(0xdcefff, 0x25321f, 1.15);
    this.scene.add(hemi);
    const sun = new THREE.DirectionalLight(0xfff1d6, 2.35);
    sun.position.set(380000, 860000, 340000);
    this.scene.add(sun);

    this.terrain = new TerrainStreamer(this.scene, this.frame, this.world);
    this.hero = new SdfHeroChunk(this.scene, this.frame, this.terrain.material);
    this.hero.setEnabled(this.settings.showSdfHero);
    this.hero.onFailure = () => {
      this.terrain.setHeroReady(false);
      this.ui.toast('SDF worker unavailable; clipmaps cover the centre.', 'error');
    };
    this.ocean = new OceanSystem(this.scene, this.frame, this.world, this.settings);
    this.debug = new DebugOverlay(this.scene, this.frame);
    this.minimap = new PlanetMinimap(this.ui.minimapCanvas, this.world, this.frame);
    this.minimap.onTeleport = (x, z) => this.teleportFromMap(x, z);

    this.input = new InputController(this.ui.canvas, this.rig, {
      onBrushStart: (x, y, shift) => this.onBrushStart(x, y, shift),
      onBrushMove: (x, y, dt, shift) => this.onBrushMove(x, y, dt, shift),
      onBrushEnd: () => this.onBrushEnd(),
      onHover: (x, y) => this.onHover(x, y),
      onKey: (code, down, event) => this.onKey(code, down, event),
      onWheel: () => this.onWheel(),
    });
    this.ui.canvas.addEventListener('pointerleave', () => {
      this.pointerInside = false;
      this.minimap.clearBrush();
    });
    window.addEventListener('resize', () => this.resize());
    this.minimap.markDirty();
    this.applySettings();
    this.updateDebugExtents();
    this.ui.toast(mode === 'editor' ? 'World Forge ready — RMB to look, WASD to fly.' : 'Runtime viewer ready — RMB to look, WASD to fly.', 'success');
  }

  start(): void {
    requestAnimationFrame(this.animate);
  }

  private loadSettings(): Partial<EditorSettings> {
    try {
      const raw = localStorage.getItem('frontier.settings.v1');
      return raw ? JSON.parse(raw) : {};
    } catch { return {}; }
  }

  private loadWorld(): SdfWorld | null {
    try {
      const raw = localStorage.getItem('frontier.world.v1');
      return raw ? SdfWorld.fromJSON(JSON.parse(raw)) : null;
    } catch { return null; }
  }

  private syncSettingsFromWorld(): void {
    this.settings.seaLevel = this.world.def.seaLevel;
    this.settings.coastNoise = this.world.def.coastNoise;
    this.settings.oceanDepth = this.world.def.oceanDepth;
    this.settings.landElevation = this.world.def.landElevation;
  }

  private scheduleSave(): void {
    window.clearTimeout(this.saveTimer);
    this.saveTimer = window.setTimeout(() => {
      try {
        localStorage.setItem('frontier.world.v1', JSON.stringify(this.world.toJSON()));
        localStorage.setItem('frontier.settings.v1', JSON.stringify(this.settings));
      } catch (e) {
        console.warn('Could not persist Frontier world', e);
      }
    }, 350);
  }

  private computeStartCamera(): void {
    const x = -1350000;
    const z = 720000;
    const ground = Math.max(this.world.seaLevel, this.world.heightAt(x, z));
    this.startCamera = { x, y: ground + 320, z };
  }

  private applySettings(): void {
    this.rig.speed = this.settings.cameraSpeed;
    this.terrain.setWireframe(this.settings.showWireframe);
    this.terrain.setDebugLod(this.settings.showLodColors);
    this.hero.setEnabled(this.settings.showSdfHero);
    this.ocean.materials.near.wireframe = this.settings.showWaterWireframe;
    this.ocean.materials.far.wireframe = this.settings.showWaterWireframe;
    this.debug.setShowLod(this.settings.showLodRings);
    this.ui.setMapVisible(this.settings.showPlanetMap);
    this.ui.setHelpVisible(this.settings.showHelp);
    this.ui.setPlanetExpanded(this.settings.planetExpanded);

    let changed = false;
    for (const key of WORLD_PARAM_KEYS) {
      const value = this.settings[key];
      if (this.world.def[key] !== value) {
        this.world.setParam(key, value);
        changed = true;
      }
    }
    if (changed) this.onWorldEdited();
    this.updateDebugExtents();
    this.scheduleSave();
  }

  setSetting(key: keyof EditorSettings | string, value: any): void {
    (this.settings as any)[key] = value;
    if (key === 'tool') {
      this.setTool(value as SculptTool);
      return;
    }
    if (STRUCTURAL_WATER_KEYS.has(key as any)) {
      this.ocean.rebuild(this.settings);
      this.updateDebugExtents();
      this.scheduleSave();
      return;
    }
    if (WORLD_PARAM_KEYS.has(key as any)) {
      this.world.setParam(key as any, value);
      this.onWorldEdited();
      return;
    }
    this.applySettings();
  }

  setTool(tool: SculptTool): void {
    this.settings.tool = tool;
    this.settings.brushRadiusKm = TOOL_INFO[tool].radiusKm;
    this.ui.setActiveTool(tool);
    this.ui.refresh();
    this.updateBrushGizmo();
    this.scheduleSave();
  }

  private onWorldEdited(): void {
    this.terrain.markDirty();
    this.ocean.onWorldChanged();
    this.hero.markDirty(this.world);
    this.minimap.markDirty();
    this.updateDebugExtents();
    this.scheduleSave();
  }

  private setWorld(world: SdfWorld): void {
    this.world = world;
    this.frame.setOrigin(world.def.originLon, world.def.originLat);
    this.terrain.setWorld(world);
    this.ocean.setWorld(world);
    this.hero.markDirty(world);
    this.minimap.setWorld(world);
    this.syncSettingsFromWorld();
    this.ui.refresh();
    this.computeStartCamera();
    this.resetCamera(false);
    this.onWorldEdited();
    this.ui.toast(`Loaded ${world.def.name}`, 'success');
  }

  loadPreset(name: string): void {
    let def;
    switch (name) {
      case 'flat': def = createFlatWorld(); break;
      case 'africa': def = createAfricaPreset(); break;
      case 'archipelago': def = createArchipelagoPreset(); break;
      case 'random': def = createRandomContinent(); break;
      default: return;
    }
    this.setWorld(new SdfWorld(def));
  }

  async importSDFFile(file: File): Promise<void> {
    try {
      const world = await importSDFJSON(file);
      this.setWorld(world);
    } catch (e) {
      this.ui.toast(`Import failed: ${e instanceof Error ? e.message : String(e)}`, 'error');
    }
  }

  uiAction(action: string): void {
    switch (action) {
      case 'preset-flat': this.loadPreset('flat'); break;
      case 'preset-africa': this.loadPreset('africa'); break;
      case 'preset-archipelago': this.loadPreset('archipelago'); break;
      case 'preset-random': this.loadPreset('random'); break;
      case 'undo': this.world.undo(); this.onWorldEdited(); this.ui.toast('Removed last SDF feature'); break;
      case 'clear-edits': this.world.clearFeatures(); this.onWorldEdited(); this.ui.toast('Cleared SDF edit stack'); break;
      case 'reset-camera': this.resetCamera(); break;
      case 'focus-brush': this.focusBrush(); break;
      case 'water-drop': this.waterDropAtCursor(); break;
      case 'storm': this.ocean.storm(this.hoverValid ? this.hoverWorld.x : this.frame.cameraWorld.x, this.hoverValid ? this.hoverWorld.z : this.frame.cameraWorld.z); this.ui.toast('Injected storm pulse'); break;
      case 'calm': this.ocean.solver.calm(); this.ui.toast('Surface state calmed'); break;
      case 'reset-solver': this.ocean.rebuild(this.settings); this.ui.toast('Rebuilt solver and ocean clipmaps'); break;
      case 'toggle-map': this.setSetting('showPlanetMap', !this.settings.showPlanetMap); break;
      case 'toggle-help': this.setSetting('showHelp', !this.settings.showHelp); break;
      case 'toggle-planet': this.setSetting('planetExpanded', !this.settings.planetExpanded); break;
      case 'toggle-pause': this.setSetting('paused', !this.settings.paused); this.ui.refresh(); break;
      case 'export-sdf': exportSDFJSON(this.world); this.ui.toast('Exported editable SDF JSON', 'success'); break;
      case 'export-heightmap':
        exportHeightmapBundle(this.world, this.frame, 200, 1024)
          .then(() => this.ui.toast('Exported 16-bit heightmap PNG + metadata', 'success'))
          .catch(e => this.ui.toast(`Heightmap export failed: ${String(e)}`, 'error'));
        break;
      case 'export-obj': exportVisibleTerrainOBJ(this.terrain, this.hero, this.frame); this.ui.toast('Baked visible SDF terrain to OBJ', 'success'); break;
      case 'copy-link': {
        const url = new URL('index.html', window.location.href).href;
        navigator.clipboard?.writeText(url)
          .then(() => this.ui.toast('Runtime viewer link copied', 'success'))
          .catch(() => this.ui.toast(url));
        break;
      }
      default: break;
    }
  }

  private brushSettings(): BrushSettings {
    return {
      radiusKm: this.settings.brushRadiusKm,
      strength: this.settings.brushStrength,
      target: this.settings.brushTarget,
      feather: this.settings.brushFeather,
    };
  }

  private pickWorld(clientX: number, clientY: number): PickResult | null {
    const rect = this.ui.canvas.getBoundingClientRect();
    this.ndc.set(
      ((clientX - rect.left) / rect.width) * 2 - 1,
      -((clientY - rect.top) / rect.height) * 2 + 1,
    );
    this.camera.updateMatrixWorld();
    this.raycaster.setFromCamera(this.ndc, this.camera);
    const ray = this.raycaster.ray;
    const ox = ray.origin.x + this.frame.cameraWorld.x;
    const oy = ray.origin.y + this.frame.cameraWorld.y;
    const oz = ray.origin.z + this.frame.cameraWorld.z;
    const dx = ray.direction.x;
    const dy = ray.direction.y;
    const dz = ray.direction.z;
    const sample = (t: number) => oy + dy * t - this.world.heightAt(ox + dx * t, oz + dz * t);

    let prevT = 0;
    let prevF = sample(0);
    let t = 20;
    for (let i = 0; i < 56 && t < 2_000_000; i++) {
      const f = sample(t);
      if (prevF >= 0 && f <= 0) {
        let lo = prevT, hi = t;
        for (let b = 0; b < 22; b++) {
          const mid = (lo + hi) * 0.5;
          if (sample(mid) > 0) lo = mid; else hi = mid;
        }
        const hitT = (lo + hi) * 0.5;
        const x = ox + dx * hitT;
        const z = oz + dz * hitT;
        return { x, y: this.world.heightAt(x, z), z };
      }
      prevT = t;
      prevF = f;
      t *= 1.38;
    }

    // Fallback: sea-level plane, then clamp onto the SDF surface.
    const plane = new THREE.Plane(new THREE.Vector3(0, 1, 0), -this.world.seaLevel);
    const local = new THREE.Vector3();
    if (ray.intersectPlane(plane, local)) {
      const x = local.x + this.frame.cameraWorld.x;
      const z = local.z + this.frame.cameraWorld.z;
      return { x, y: Math.max(this.world.seaLevel, this.world.heightAt(x, z)), z };
    }
    const x = ox + dx * 1000;
    const z = oz + dz * 1000;
    return { x, y: Math.max(this.world.seaLevel, this.world.heightAt(x, z)), z };
  }

  private onHover(clientX: number, clientY: number): void {
    const hit = this.pickWorld(clientX, clientY);
    this.pointerInside = true;
    if (!hit) { this.hoverValid = false; return; }
    this.hoverWorld.set(hit.x, Math.max(hit.y, this.world.seaLevel), hit.z);
    this.hoverValid = true;
    this.minimap.setBrush(hit.x, hit.z, this.settings.brushRadiusKm * 1000);
    this.updateBrushGizmo();
  }

  private onBrushStart(clientX: number, clientY: number, shift: boolean): void {
    const hit = this.pickWorld(clientX, clientY);
    if (!hit) return;
    this.hoverWorld.set(hit.x, Math.max(hit.y, this.world.seaLevel), hit.z);
    this.hoverValid = true;
    const tool = this.settings.tool;
    const radius = Math.max(2, this.settings.brushRadiusKm * 1000);
    if (tool === 'inspect') {
      const g = this.world.localToGeo(hit.x, hit.z);
      this.ui.toast(`${formatLatLon(g.lat, g.lon)} · elevation ${formatMetres(hit.y - this.world.seaLevel)}`);
      return;
    }
    if (tool === 'waterDrop') {
      this.ocean.addDrop(hit.x, hit.z, radius, this.settings.dropSize * (shift ? -1 : 1));
      this.lastDropTime = performance.now();
      return;
    }
    if (tool === 'waterPush') {
      this.lastWaterPos.copy(this.hoverWorld);
      return;
    }
    this.activeStrokeId = this.world.beginStroke(tool, hit.x, hit.z, this.brushSettings());
    this.lastPaintRebuild = performance.now();
    this.onWorldEdited();
  }

  private onBrushMove(clientX: number, clientY: number, dt: number, shift: boolean): void {
    const hit = this.pickWorld(clientX, clientY);
    if (!hit) return;
    this.hoverWorld.set(hit.x, Math.max(hit.y, this.world.seaLevel), hit.z);
    this.hoverValid = true;
    const tool = this.settings.tool;
    const radius = Math.max(2, this.settings.brushRadiusKm * 1000);
    if (tool === 'waterDrop') {
      const now = performance.now();
      if (now - this.lastDropTime > 28) {
        this.ocean.addDrop(hit.x, hit.z, radius, this.settings.dropSize * 0.18 * (shift ? -1 : 1));
        this.lastDropTime = now;
      }
      return;
    }
    if (tool === 'waterPush') {
      const vx = clamp((hit.x - this.lastWaterPos.x) / Math.max(dt, 0.016) * 0.018, -8, 8);
      const vz = clamp((hit.z - this.lastWaterPos.z) / Math.max(dt, 0.016) * 0.018, -8, 8);
      this.ocean.addMomentum(hit.x, hit.z, radius * 1.6, vx, vz);
      this.lastWaterPos.copy(this.hoverWorld);
      return;
    }
    if (!this.activeStrokeId) return;
    this.world.updateStroke(this.activeStrokeId, hit.x, hit.z, this.brushSettings(), false);
    const now = performance.now();
    if (now - this.lastPaintRebuild > 90) {
      this.onWorldEdited();
      this.lastPaintRebuild = now;
    }
  }

  private onBrushEnd(): void {
    if (this.activeStrokeId) {
      this.world.markChanged();
      this.onWorldEdited();
      this.activeStrokeId = null;
    }
  }

  private onKey(code: string, down: boolean, event: KeyboardEvent): void {
    if (!down) return;
    const shortcuts: Record<string, SculptTool> = {
      Digit1: 'inspect', Digit2: 'continent', Digit3: 'gulf', Digit4: 'island',
      Digit5: 'raise', Digit6: 'lower', Digit7: 'flatten', Digit8: 'smooth',
      Digit9: 'ridge', Digit0: 'cityPad', KeyB: 'waterDrop', KeyN: 'waterPush',
    };
    if (shortcuts[code]) { this.setTool(shortcuts[code]); event.preventDefault(); return; }
    switch (code) {
      case 'KeyF': this.focusBrush(); break;
      case 'KeyR': this.resetCamera(); break;
      case 'KeyP': this.uiAction('toggle-pause'); break;
      case 'KeyH': this.setSetting('showHelp', !this.settings.showHelp); break;
      case 'KeyG': this.setSetting('showPlanetMap', !this.settings.showPlanetMap); break;
      case 'KeyL': this.setSetting('showLodColors', !this.settings.showLodColors); this.ui.refresh(); break;
      case 'KeyV': this.setSetting('showSdfHero', !this.settings.showSdfHero); this.ui.refresh(); break;
      case 'Escape': this.onBrushEnd(); break;
      default: break;
    }
  }

  private onWheel(): void {
    this.settings.cameraSpeed = this.rig.speed;
    this.ui.refresh();
    this.scheduleSave();
  }

  private waterDropAtCursor(): void {
    const x = this.hoverValid ? this.hoverWorld.x : this.frame.cameraWorld.x;
    const z = this.hoverValid ? this.hoverWorld.z : this.frame.cameraWorld.z;
    this.ocean.addDrop(x, z, Math.max(20, this.settings.brushRadiusKm * 1000), this.settings.dropSize);
    this.ui.toast('Injected a mass pulse into η');
  }

  private focusBrush(): void {
    if (!this.hoverValid) { this.ui.toast('Hover the viewport first'); return; }
    this.frame.cameraWorld.x = this.hoverWorld.x;
    this.frame.cameraWorld.z = this.hoverWorld.z;
    this.frame.cameraWorld.y = Math.max(this.hoverWorld.y + 220, this.world.seaLevel + 180);
    this.hero.markDirty(this.world);
    this.terrain.markDirty();
    this.ocean.onWorldChanged();
    this.ui.toast('Camera focused on brush target');
  }

  resetCamera(announce = true): void {
    this.computeStartCamera();
    this.rig.reset(this.startCamera.x, this.startCamera.y, this.startCamera.z);
    this.hero.markDirty(this.world);
    this.terrain.markDirty();
    this.ocean.onWorldChanged();
    if (announce) this.ui.toast('Camera reset');
  }

  teleportFromMap(x: number, z: number): void {
    const ground = Math.max(this.world.seaLevel, this.world.heightAt(x, z));
    const g = this.world.localToGeo(x, z);
    this.frame.setCameraWorld(x, ground + 300, z);
    this.hero.markDirty(this.world);
    this.terrain.markDirty();
    this.ocean.onWorldChanged();
    this.ui.toast(`Teleported to ${formatLatLon(g.lat, g.lon)}`);
  }

  private updateBrushGizmo(): void {
    const visible = this.pointerInside && this.hoverValid && this.settings.tool !== 'inspect';
    const radius = Math.max(2, this.settings.brushRadiusKm * 1000);
    this.debug.setBrush(
      this.hoverWorld.x,
      this.hoverWorld.y,
      this.hoverWorld.z,
      radius,
      visible,
      TOOL_INFO[this.settings.tool].color,
    );
  }

  private updateDebugExtents(): void {
    const nearHalf = this.ocean.nearExtent / 2;
    const extents: LodExtent[] = [{ half: nearHalf, label: 'SIM', color: [1.0, 0.82, 0.25] }];
    const palette: LodExtent['color'][] = [
      [0.35, 0.95, 1.0], [0.55, 0.85, 1.0], [0.72, 0.72, 1.0],
      [0.95, 0.62, 1.0], [1.0, 0.55, 0.75], [1.0, 0.72, 0.48],
      [0.95, 0.95, 0.55], [0.62, 1.0, 0.72], [0.55, 1.0, 0.95],
      [0.72, 0.9, 1.0], [0.9, 0.7, 1.0], [1.0, 0.8, 0.6],
    ];
    for (let i = 0; i < this.settings.farLevels; i++) {
      extents.push({ half: nearHalf * Math.pow(2, i + 1), label: `LOD${i}`, color: palette[i % palette.length] });
    }
    this.debug.setExtents(extents);
  }

  private clampCameraToGround(): void {
    const cam = this.frame.cameraWorld;
    const ground = Math.max(this.world.seaLevel + 6, this.world.heightAt(cam.x, cam.z) + 10);
    if (cam.y < ground) cam.y = ground;
  }

  private resize(): void {
    const w = window.innerWidth;
    const h = window.innerHeight;
    this.renderer.setSize(w, h, false);
    this.camera.aspect = w / h;
    this.camera.updateProjectionMatrix();
    this.minimap.markDirty();
  }

  private updateHud(dt: number): void {
    this.fps = this.fps * 0.9 + (1 / Math.max(dt, 1e-4)) * 0.1;
    this.statAccum += dt;
    if (this.statAccum < 0.16) return;
    this.statAccum = 0;
    const geo = this.frame.geo();
    const water = this.ocean.stats();
    const budget = this.ocean.budget();
    const terrainStats = this.terrain.stats();
    const waveSpeed = Math.sqrt(Math.max(0.01, this.settings.gravity * water.maxDepth));
    const info = TOOL_INFO[this.settings.tool];
    this.ui.setTopbar({
      mode: this.mode === 'editor' ? 'WORLD FORGE / EDITOR' : 'RUNTIME VIEWER',
      worldName: this.world.def.name,
      coords: formatLatLon(geo.lat, geo.lon),
      altitude: `ALT ${formatMetres(this.frame.cameraWorld.y)}`,
      speed: `${formatMetres(this.rig.speed)}/s`,
      status: `${info.label}${this.settings.paused ? ' · PAUSED' : ''}`,
    });
    this.ui.setStats({
      'FPS': this.fps.toFixed(0),
      'WebGL draws / tris': `${this.renderer.info.render.calls} / ${this.renderer.info.render.triangles.toLocaleString()}`,
      'Terrain clipmap': `${terrainStats.vertices.toLocaleString()} verts · ${terrainStats.rings} LODs · ${terrainStats.maxExtentKm.toFixed(0)} km`,
      'SDF → mesh': this.hero.stats(),
      'Simulated SWE': `${budget.simulatedCells.toLocaleString()} cells · ${formatMetres(this.settings.cellSize)}/cell`,
      'Wet / dry cells': `${water.wetCells.toLocaleString()} / ${(water.totalCells - water.wetCells).toLocaleString()}`,
      'CFL substeps / dt': `${water.substeps} · ${(water.stableDt * 1000).toFixed(1)} ms`,
      'Shallow-wave speed': `${waveSpeed.toFixed(1)} m/s`,
      'Max |u| / depth': `${water.maxVelocity.toFixed(2)} m/s · ${formatMetres(water.maxDepth)}`,
      'Surface mass drift': `${(water.massDrift * 100).toFixed(3)}%`,
      'Far tessellated water': `${budget.farVertices.toLocaleString()} verts · ${budget.maxExtentKm.toFixed(0)} km`,
      'Uniform-grid equivalent': `${(budget.uniformVertices / 1e6).toFixed(1)} M verts at near density`,
      'Adaptive saving': `${budget.saving.toFixed(0)}× fewer vertices`,
      'Ocean near patch': `${formatMetres(this.ocean.nearExtent)} across`,
      'Brush': `${info.label} · ${this.settings.brushRadiusKm < 1 ? formatMetres(this.settings.brushRadiusKm * 1000) : `${this.settings.brushRadiusKm.toFixed(1)} km`}`,
      'SDF features': `${this.world.featureCount}`,
      'Solver time': `${water.time.toFixed(1)} s`,
    });
  }

  private animate = (now: number): void => {
    requestAnimationFrame(this.animate);
    const dt = Math.min(0.05, Math.max(0.0001, (now - this.lastTime) / 1000));
    this.lastTime = now;

    this.rig.update(dt);
    this.clampCameraToGround();
    this.hero.update(this.frame, this.world);
    this.terrain.setHeroReady(this.hero.enabled && this.hero.ready);
    this.terrain.update(dt, this.frame);
    this.ocean.update(dt, this.frame, this.world, this.settings);
    this.debug.update(this.frame, this.world.seaLevel, this.settings.showLodRings);
    this.updateBrushGizmo();
    this.minimap.render();
    this.renderer.render(this.scene, this.camera);
    this.updateHud(dt);
  };

  dispose(): void {
    this.input.dispose();
    this.minimap.dispose();
    this.debug.dispose();
    this.ocean.dispose();
    this.hero.dispose();
    this.terrain.dispose();
    this.renderer.dispose();
  }
}
