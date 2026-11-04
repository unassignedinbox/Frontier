/**
 * PlanetForge — entry point / orchestration.
 *
 * Frame loop, in order:
 *   1. camera input
 *   2. LOD cut from the camera             (pure geometry, cheap)
 *   3. streamer update: queue/evict        (no per-sample maths on main thread)
 *   4. ocean simulate (rings, ~2–8 ms)
 *   5. ocean + terrain transform relative to the camera
 *   6. draw
 *
 * Nothing in the loop allocates a planet-sized buffer, and nothing grows with
 * world size: the cost is a function of the *budget* (chunks, cells, rings),
 * not of the planet.
 */

import * as THREE from 'three';
import { EditorUI, TOOLS, type OutlinerItem, type ToolDef, type ToolId } from '../editor/ui';
import { CameraRig } from './cameraRig';
import {
  type TerrainWorld, createPreset, makeId, cap, ellipse,
  type SculptStroke, type BrushMode, cloneWorld,
} from '../core/world/terrainWorld';
import { TerrainField, type SurfaceInfo } from '../core/world/terrainField';
import { ChunkStreamer, type ResidentChunk } from '../core/stream/chunkStreamer';
import { MeshCache } from '../core/stream/meshCache';
import { DEFAULT_LOD_SETTINGS, selectChunks, type LodSettings } from '../core/stream/lod';
import {
  dirToFaceST, chunkBounds, chunkEdgeMetresLocal, dirToLatLon,
  latLonToDir, type ChunkKey, angleBetween, tangentFrame,
} from '../core/sdf/cubesphere';
import { TerrainRenderer, SkyDome, createAtmosphere } from '../render/terrainRenderer';
import { OceanRenderer } from '../render/oceanRenderer';
import { type Vec3, add, clamp, cross, dot, len, mul, normalize, sub, v3, DEG } from '../core/math/vec';
import { seedFor } from '../core/math/noise';

const KM = 1000;

interface BrushState {
  radius: number;
  strength: number;
  feather: number;
  target: number;
}

export class PlanetForge {
  private container: HTMLElement;
  private renderer: THREE.WebGLRenderer;
  private scene = new THREE.Scene();
  private rig: CameraRig;
  private ui!: EditorUI;
  private world: TerrainWorld = createPreset('continents');
  private field: TerrainField;
  private streamer: ChunkStreamer;
  private terrain: TerrainRenderer;
  private ocean: OceanRenderer;
  private sky = new SkyDome();
  private atmosphere: THREE.Mesh;
  private cache: MeshCache;

  private lod: LodSettings = { ...DEFAULT_LOD_SETTINGS };
  private brush: BrushState = { radius: 900 * KM, strength: 0.6, feather: 0.55, target: 600 };
  private activeTool: ToolId = 'landmass';
  private undoStack: TerrainWorld[] = [];
  private pointerNdc = { x: 0, y: 0 };
  private hovering = false;
  private dragging = false;
  private dragStart: { x: number; y: number; ndcX: number; ndcY: number } | null = null;
  private lastDragDir: Vec3 | null = null;
  private keys = new Set<string>();
  private lastFrame = performance.now();
  private lodBias = 1;
  private surfaceDirty = false;
  private lodTimer = 0;
  private statsTimer = 0;
  private hoverInfo: SurfaceInfo | null = null;
  private hoverDir: Vec3 | null = null;
  private sunDir: Vec3 = normalize(v3(0.45, 0.62, 0.64));
  private brushCursor: HTMLElement;
  private diskCacheReady = false;
  private running = false;

  constructor(container: HTMLElement) {
    this.container = container;
    this.renderer = new THREE.WebGLRenderer({
      antialias: true,
      powerPreference: 'high-performance',
      logarithmicDepthBuffer: true,
    });
    this.renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));
    this.renderer.outputColorSpace = THREE.SRGBColorSpace;
    this.renderer.toneMapping = THREE.ACESFilmicToneMapping;
    this.renderer.toneMappingExposure = 1.05;
    this.renderer.setClearColor(0x05060a);

    this.rig = new CameraRig((dir) => this.field.surfaceRadiusApprox(dir));
    this.field = new TerrainField(this.world);

    this.scene.add(this.sky.mesh);
    this.atmosphere = createAtmosphere(this.world.planetRadius);
    this.scene.add(this.atmosphere);

    this.terrain = new TerrainRenderer(this.scene);
    // Linear filtering of float textures is an OpenGL ES 3.0 / WebGL2 extension;
    // without it the ocean falls back to nearest sampling rather than rendering
    // black.
    const gl = this.renderer.getContext();
    const floatLinear = !!(gl.getExtension('OES_texture_float_linear')
      || gl.getExtension('EXT_color_buffer_float'));
    this.ocean = new OceanRenderer(this.scene, floatLinear);

    this.cache = new MeshCache();
    this.streamer = new ChunkStreamer({
      workerCount: Math.max(2, Math.min(6, (navigator.hardwareConcurrency || 4) - 1)),
      settings: this.lod,
      maxResident: 900,
      maxBytes: 340 * 1024 * 1024,
      detailDistance: 2_400_000,
      cache: this.cache,
    });

    this.brushCursor = document.createElement('div');
    this.brushCursor.className = 'pf-brush-cursor';

    this.ui = new EditorUI(container, {
      onTool: (id) => this.setTool(id),
      onSlider: (id, v) => this.onSlider(id, v),
      onToggle: (id, v) => this.onToggle(id, v),
      onAction: (id) => this.onAction(id),
      onSelectOperator: () => undefined,
      onRemoveOperator: (id) => this.removeOperator(id),
      onFlyToOperator: (id) => this.flyToOperator(id),
    });

    this.setupScene();
    this.setupInput();
    this.setupToggles();
    this.attachStreamerCallbacks();
  }

  /* ------------------------------------------------------------------ setup */

  private setupScene(): void {
    // Lights: a sun that matches the sky shader, plus a dim hemisphere so the
    // shadowed side of a mountain is not pure black.
    const sun = new THREE.DirectionalLight(0xfff2e0, 2.4);
    sun.position.set(this.sunDir.x * 100, this.sunDir.y * 100, this.sunDir.z * 100);
    this.scene.add(sun);
    this.scene.add(new THREE.HemisphereLight(0x9fb8e8, 0x24201c, 0.55));
    this.scene.add(this.sky.mesh);

    this.ocean.setField(this.field);
    this.ocean.setSun(this.sunDir);
    this.wireLogOfSun = sun;
  }

  private wireLogOfSun: THREE.DirectionalLight | null = null;

  private attachStreamerCallbacks(): void {
    this.streamer.onChunkReady((chunk: ResidentChunk) => this.terrain.add(chunk));
    this.streamer.onChunkEvicted((ks: string) => this.terrain.remove(ks));
  }

  private setupInput(): void {
    const viewport = this.container.querySelector('#pf-viewport') as HTMLElement;
    viewport.appendChild(this.renderer.domElement);
    viewport.appendChild(this.brushCursor);

    const canvas = this.renderer.domElement;
    canvas.addEventListener('contextmenu', (e) => e.preventDefault());

    canvas.addEventListener('pointerdown', (e) => {
      canvas.setPointerCapture(e.pointerId);
      if (e.button === 2) {
        this.dragging = true;
        this.dragStart = { x: e.clientX, y: e.clientY, ndcX: this.pointerNdc.x, ndcY: this.pointerNdc.y };
        return;
      }
      if (e.button === 1) {
        this.dragging = true;
        this.dragStart = { x: e.clientX, y: e.clientY, ndcX: 0, ndcY: 0 };
        return;
      }
      if (e.button === 0) {
        this.dragging = true;
        this.dragStart = { x: e.clientX, y: e.clientY, ndcX: this.pointerNdc.x, ndcY: this.pointerNdc.y };
        this.lastDragDir = null;
        this.applyTool();
      }
    });

    canvas.addEventListener('pointermove', (e) => {
      const rect = canvas.getBoundingClientRect();
      this.pointerNdc.x = ((e.clientX - rect.left) / rect.width) * 2 - 1;
      this.pointerNdc.y = -((e.clientY - rect.top) / rect.height) * 2 + 1;
      this.hovering = true;

      if (this.dragging && this.dragStart) {
        const dx = e.clientX - this.dragStart.x;
        const dy = e.clientY - this.dragStart.y;
        const right = e.buttons & 2;
        const middle = e.buttons & 4;
        const left = e.buttons & 1;
        if (right) {
          this.rig.yaw(-dx * 0.0032);
          this.rig.pitch(dy * 0.0032);
          this.dragStart.x = e.clientX;
          this.dragStart.y = e.clientY;
        } else if (middle) {
          const scale = this.rig.speed * 0.14;
          this.rig.move(-dx * scale, 0, dy * scale);
          this.dragStart.x = e.clientX;
          this.dragStart.y = e.clientY;
        } else if (left) {
          this.applyTool();
        }
      }
      this.updateBrushCursor();
    });

    canvas.addEventListener('pointerup', (e) => {
      canvas.releasePointerCapture(e.pointerId);
      this.dragging = false;
      this.dragStart = null;
      this.lastDragDir = null;
    });

    canvas.addEventListener('pointerleave', () => { this.hovering = false; this.brushCursor.style.display = 'none'; });

    canvas.addEventListener('wheel', (e) => {
      e.preventDefault();
      const factor = Math.exp(-e.deltaY * 0.0012);
      this.rig.speed = clamp(this.rig.speed * factor, 0.5, 4_000_000);
    }, { passive: false });

    window.addEventListener('keydown', (e) => {
      if ((e.target as HTMLElement)?.tagName === 'INPUT') return;
      this.keys.add(e.key.toLowerCase());
      if (e.key === 'f' || e.key === 'F') this.flyToHover();
      if (e.key === 'r' || e.key === 'R') this.resetCamera();
      if ((e.ctrlKey || e.metaKey) && (e.key === 'z' || e.key === 'Z')) { e.preventDefault(); this.undo(); }
      if (e.key === 'g' || e.key === 'G') this.toggleGlobe();
    });
    window.addEventListener('keyup', (e) => this.keys.delete(e.key.toLowerCase()));
    window.addEventListener('blur', () => this.keys.clear());

    window.addEventListener('resize', () => this.resize());
    this.resize();
  }

  private setupToggles(): void {
    const bind = (id: string, key: string, fn: (v: boolean) => void): void => {
      const el = this.container.querySelector(`#${id}`) as HTMLInputElement | null;
      if (!el) return;
      el.addEventListener('change', () => {
        fn(el.checked);
        this.onToggle(key, el.checked);
      });
    };
    bind('pf-toggle-wire', 'wire', (v) => this.terrain.setWireframe(v));
    bind('pf-toggle-ocean', 'ocean', (v) => { this.ocean.group.visible = v; });
    bind('pf-toggle-water-pause', 'waterPause', (v) => { this.ocean.paused = v; });
    bind('pf-toggle-lod', 'lod', (v) => {
      // LOD debug tints far chunks by level instead of by material.
      this.lodBias = v ? 2.0 : 1.0;
    });
  }

  private resize(): void {
    const viewport = this.container.querySelector('#pf-viewport') as HTMLElement;
    const w = viewport.clientWidth || window.innerWidth;
    const h = viewport.clientHeight || window.innerHeight;
    this.renderer.setSize(w, h, false);
    this.rig.resize(w, h);
  }

  /* ------------------------------------------------------------------ tools */

  private setTool(id: ToolId): void {
    this.activeTool = id;
    const def = TOOLS.find((t) => t.id === id) as ToolDef;
    // Give each tool a sensible default radius when it is selected.
    this.brush.radius = def.radius;
    // The UI can call back into here while its own constructor is still running.
    if (this.ui) this.ui.setBrushPill(def.radius);
  }

  private onSlider(id: string, value: number): void {
    switch (id) {
      case 'brushRadiusNorm': {
        const def = TOOLS.find((t) => t.id === this.activeTool) as ToolDef;
        // Log scale from 40 m to 4000 km so the slider is usable at every scale.
        const min = Math.log(40);
        const max = Math.log(4.0e6);
        this.brush.radius = Math.exp(min + (max - min) * value);
        this.ui.setBrushPill(this.brush.radius);
        void def;
        break;
      }
      case 'brushStrength': this.brush.strength = value; break;
      case 'brushFeather': this.brush.feather = value; break;
      case 'brushTarget': this.brush.target = value; break;
      case 'seaLevelOffset': {
        this.world.coastHeight = 120 + value;
        this.worldModified();
        break;
      }
      case 'shelfWidth': this.world.shelfWidth = value * KM; this.worldModified(); break;
      case 'oceanDepth': this.world.oceanDepth = value; this.worldModified(); break;
      case 'relief': {
        const scale = value;
        this.world.relief.landAmplitude = 190 * scale;
        this.world.relief.mountainAmplitude = 2600 * scale;
        this.world.relief.seaAmplitude = 260 * scale;
        this.worldModified();
        break;
      }
      case 'coastRoughness': {
        const scale = value;
        for (const op of this.world.operators) {
          if (op.type === 'cap') op.roughness = 0.075 * scale;
        }
        this.worldModified();
        break;
      }
      case 'windStress': this.ocean.setSettings({ windStress: value }); break;
      case 'windAngle': this.ocean.setSettings({ windAngle: value * DEG }); break;
      case 'swellScale': this.ocean.swellScale = value; break;
      case 'friction': this.ocean.setSettings({ bottomFriction: value }); break;
      case 'maxChunks': {
        this.lod.maxChunks = Math.round(value);
        this.rebuildStreamerSettings();
        break;
      }
      case 'meshCells': {
        this.lod.cells = Math.round(value);
        this.rebuildStreamerSettings();
        break;
      }
      case 'lodBias': this.lodBias = value; break;
    }
  }

  private onToggle(id: string, value: boolean): void {
    if (id === 'wire') this.terrain.setWireframe(value);
    if (id === 'ocean') this.ocean.group.visible = value;
    if (id === 'waterPause') this.ocean.paused = value;
  }

  private onAction(id: string): void {
    if (id.startsWith('preset:')) {
      const name = id.slice(7) as 'continents' | 'archipelago' | 'pangea';
      this.pushUndo();
      this.world = createPreset(name);
      this.onWorldReplaced();
      this.ui.setStatus(`Loaded preset “${this.world.name}”`);
    } else if (id === 'export') {
      this.exportWorld();
    } else if (id === 'undo') {
      this.undo();
    } else if (id === 'clearStrokes') {
      this.pushUndo();
      this.world.strokes = [];
      this.worldModified();
    }
  }

  private rebuildStreamerSettings(): void {
    this.streamer.dispose();
    this.streamer = new ChunkStreamer({
      workerCount: Math.max(2, Math.min(6, (navigator.hardwareConcurrency || 4) - 1)),
      settings: this.lod,
      maxResident: this.lod.maxChunks,
      maxBytes: 340 * 1024 * 1024,
      detailDistance: 2_400_000,
      cache: this.cache,
    });
    this.attachStreamerCallbacks();
    this.terrain.clear();
    this.streamer.setWorld(this.world);
    this.surfaceDirty = true;
  }

  /* --------------------------------------------------------------- authoring */

  private applyTool(): void {
    if (!this.hoverDir) return;
    const dir = this.hoverDir;

    switch (this.activeTool) {
      case 'inspect': return;
      case 'teleport': this.flyToHover(); return;
      case 'landmass': this.addLandmass(dir, 'land', 1.0); return;
      case 'island': this.addLandmass(dir, 'land', 0.35); return;
      case 'sea': this.addLandmass(dir, 'water', 1.0); return;
      case 'shelf': {
        this.pushUndo();
        const latlon = dirToLatLon(dir);
        this.world.operators.push(ellipse(
          makeId('shelf'), 'shelf', latlon.lat, latlon.lon,
          this.brush.radius, this.brush.radius * 0.7, Math.random() * Math.PI,
          this.brush.target * 3,
          { blend: this.brush.radius * 0.6 },
        ));
        this.refreshOperatorList();
        this.worldModified();
        return;
      }
      case 'raise': case 'lower': case 'smooth': case 'flatten':
      case 'plateau': case 'canyon': case 'rough': {
        this.paintStroke(dir);
        return;
      }
      case 'waterDrop': {
        const local = this.worldToChunkLocal(dir);
        if (local) this.ocean.dropWater(local.x, local.z, Math.max(4, this.brush.radius), 900 * this.brush.strength);
        return;
      }
      case 'waterPush': {
        const local = this.worldToChunkLocal(dir);
        if (local) this.ocean.pushWater(local.x, local.z, Math.max(6, this.brush.radius), 2.4 * this.brush.strength, 0.6);
        return;
      }
    }
  }

  private addLandmass(dir: Vec3, kind: 'land' | 'water', scale: number): void {
    this.pushUndo();
    const ll = dirToLatLon(dir);
    const radius = this.brush.radius * scale;
    // Elongate along the drag direction if the user dragged; otherwise vary by
    // seed so repeated clicks do not produce identical circles.
    const stretch = 1.0 + (seedFor(Date.now()) % 100) / 220;
    const op = cap(
      makeId(kind === 'land' ? 'land' : 'sea'),
      kind,
      ll.lat, ll.lon,
      radius,
      this.world.planetRadius,
      {
        roughness: 0.055 + this.brush.feather * 0.075,
        frequency: 2.2 + (1 - this.brush.feather) * 3.4,
        stretch,
        stretchDirLat: ll.lat + 60,
        stretchDirLon: ll.lon,
        blend: Math.max(8000, radius * 0.11),
        seed: seedFor(Date.now(), ll.lat, ll.lon),
      },
    );
    this.world.operators.push(op);
    this.refreshOperatorList();
    this.worldModified();
    this.ui.setStatus(`${kind === 'land' ? 'Landmass' : 'Sea'} added · ${fmtKm(radius)} radius`, 'info');
  }

  private paintStroke(dir: Vec3): void {
    // Accumulate a stroke per second rather than per pixel: a 120 Hz drag should
    // not deposit 120 dabs in the same place.
    const isPaint = this.activeTool !== 'raise' && this.activeTool !== 'lower'
      && this.activeTool !== 'canyon' && this.activeTool !== 'rough';
    void isPaint;
    if (this.lastDragDir && angleBetween(this.lastDragDir, dir) * this.world.planetRadius < this.brush.radius * 0.16) {
      return;
    }
    this.lastDragDir = dir;
    this.pushUndo(true);

    const mode = this.activeTool as BrushMode;
    const stroke: SculptStroke = {
      id: makeId('stroke'),
      mode,
      dir,
      radius: this.brush.radius,
      strength: this.brush.strength * 0.55,
      target: this.brush.target,
      hardness: 1 - this.brush.feather,
      seed: seedFor(Date.now(), Math.random()),
    };
    this.world.strokes.push(stroke);
    // Keep the stroke list bounded: past a few hundred, older dabs collapse into
    // the same visual result while costing every sample in the field.
    if (this.world.strokes.length > 900) this.world.strokes.splice(0, this.world.strokes.length - 900);
    this.worldModified(true);
  }

  private worldToChunkLocal(dir: Vec3): { x: number; z: number } | null {
    const frame = this.oceanFrame();
    if (!frame) return null;
    const p = mul(dir, this.world.planetRadius);
    const d = sub(p, frame.origin);
    return { x: dot(d, frame.right), z: dot(d, frame.up) };
  }

  private oceanFrame(): ReturnType<typeof import('../core/mesh/chunkMesher')['chunkFrame']> | null {
    const st = dirToFaceST(normalize(this.rig.position));
    const lodLevel = Math.min(this.lod.maxLod, 8);
    const n = Math.pow(2, lodLevel);
    const key: ChunkKey = {
      face: st.face,
      lod: lodLevel,
      x: Math.min(n - 1, Math.max(0, Math.floor(st.s * n))),
      y: Math.min(n - 1, Math.max(0, Math.floor(st.t * n))),
    };
    // Cheap inline frame: normal at the camera, tangent axes from the face.
    const b = chunkBounds(key);
    void b;
    const dir = normalize(this.rig.position);
    const { right, up } = tangentFrame(dir);
    return {
      origin: mul(dir, this.world.planetRadius),
      right, up, normal: dir,
      edgeMetres: chunkEdgeMetresLocal(key, this.world.planetRadius),
    };
  }

  private pushUndo(coalesce = false): void {
    if (coalesce && this.undoStack.length > 0) {
      // Strokes already coalesce visually; only snapshot every so often.
      if (this.world.strokes.length % 24 !== 0) return;
    }
    this.undoStack.push(cloneWorld(this.world));
    if (this.undoStack.length > 24) this.undoStack.shift();
  }

  private undo(): void {
    const prev = this.undoStack.pop();
    if (!prev) { this.ui.setStatus('Nothing to undo', 'warn'); return; }
    this.world = prev;
    this.onWorldReplaced();
    this.ui.setStatus('Undo');
  }

  private removeOperator(id: string): void {
    this.pushUndo();
    this.world.operators = this.world.operators.filter((o) => o.id !== id);
    this.refreshOperatorList();
    this.worldModified();
  }

  private flyToOperator(id: string): void {
    const op = this.world.operators.find((o) => o.id === id);
    if (!op) return;
    const dir = op.type === 'cap' || op.type === 'ellipse' ? op.center
      : op.type === 'capsule' ? normalize(add(op.a, op.b))
        : op.type === 'fracture' || op.type === 'band' ? (op.mask?.center ?? v3(0, 1, 0))
          : v3(0, 1, 0);
    const radius = op.type === 'cap' ? op.radius * this.world.planetRadius : 400 * KM;
    this.rig.placeAtDirection(dir, this.world.planetRadius + Math.max(20 * KM, radius * 2.2));
  }

  private flyToHover(): void {
    if (!this.hoverDir) return;
    this.rig.placeAtDirection(this.hoverDir, this.world.planetRadius + Math.max(1200, this.rig.altitude * 0.35));
  }

  private resetCamera(): void {
    const dir = latLonToDir(34, 18);
    this.rig.placeAtDirection(dir, this.world.planetRadius + 180 * KM);
  }

  private toggleGlobe(): void {
    const dir = normalize(this.rig.position);
    this.rig.placeAtDirection(dir, this.world.planetRadius * 2.6);
  }

  /* ------------------------------------------------------------ world edits */

  private onWorldReplaced(): void {
    this.field = new TerrainField(this.world);
    this.streamer.setWorld(this.world);
    this.terrain.clear();
    this.ocean.setField(this.field);
    this.surfaceDirty = true;
    this.ui.setWorldName(this.world.name);
    this.refreshOperatorList();
  }

  /**
   * A local edit re-evaluates the field but must not throw away the whole
   * resident set: only chunks whose bounds overlap the edited region are
   * invalidated. That is what keeps a sculpt stroke at 60 fps instead of
   * re-meshing the visible planet every dab.
   */
  private worldModified(localOnly = false): void {
    this.field.reindex();
    this.surfaceDirty = true;
    // Bump the epoch so in-flight results from the old field are discarded.
    this.streamer.setWorld(this.world);
    if (localOnly && this.hoverDir) {
      const centre = this.hoverDir;
      const radius = this.brush.radius * 1.35;
      this.streamer.invalidate((key) => {
        const b = chunkBounds(key);
        const edge = chunkEdgeMetresLocal(key, this.world.planetRadius);
        const dist = angleBetween(normalize(b.centerDir), centre) * this.world.planetRadius;
        return dist < radius + edge * 0.9;
      });
    } else {
      this.terrain.clear();
    }
  }

  private refreshOperatorList(): void {
    const items: OutlinerItem[] = this.world.operators.map((op) => ({
      id: op.id,
      label: opLabel(op),
      kind: opShortKind(op.type, op.type === 'capsule' ? op.kind : op.type === 'cap' ? op.kind : op.type),
    }));
    this.ui.setOutliner(items);
  }

  private exportWorld(): void {
    const json = JSON.stringify(this.world, null, 2);
    const blob = new Blob([json], { type: 'application/json' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = `${this.world.name.replace(/\s+/g, '-').toLowerCase()}.planetforge.json`;
    a.click();
    setTimeout(() => URL.revokeObjectURL(url), 1000);
    this.ui.setStatus(`Exported SDF document · ${(json.length / 1024).toFixed(1)} KB`, 'info');
  }

  /* ----------------------------------------------------------------- render */

  private updateHover(): void {
    if (!this.hovering) { this.hoverDir = null; return; }
    const hit = this.rig.pickTerrain(this.pointerNdc.x, this.pointerNdc.y, (dir) => this.field.heightAt(dir));
    if (!hit) { this.hoverDir = null; this.brushCursor.style.display = 'none'; return; }
    this.hoverDir = hit.dir;
    this.hoverInfo = this.field.surfaceInfo(hit.dir, hit.altitude);
    const ll = dirToLatLon(hit.dir);
    this.ui.setCoordinates(ll.lat, ll.lon, hit.altitude, this.hoverInfo.coast);
  }

  private updateBrushCursor(): void {
    if (!this.hoverDir || !this.hovering) { this.brushCursor.style.display = 'none'; return; }
    const viewport = this.container.querySelector('#pf-viewport') as HTMLElement;
    const rect = viewport.getBoundingClientRect();
    const px = (this.pointerNdc.x * 0.5 + 0.5) * rect.width;
    const py = (-this.pointerNdc.y * 0.5 + 0.5) * rect.height;
    // Project the brush radius to pixels using the distance to the hit point.
    const camAlt = Math.max(1, this.rig.altitude);
    const angular = this.brush.radius / Math.max(1, camAlt);
    const pxRadius = clamp((angular / (Math.tan((this.rig.fov * DEG) / 2))) * rect.height * 0.5, 6, rect.height);
    this.brushCursor.style.display = 'block';
    this.brushCursor.style.left = `${px}px`;
    this.brushCursor.style.top = `${py}px`;
    this.brushCursor.style.width = `${pxRadius * 2}px`;
    this.brushCursor.style.height = `${pxRadius * 2}px`;
  }

  /** Called every frame. */
  frame(): void {
    const now = performance.now();
    const rawDelta = now - this.lastFrame;
    const dt = Math.min(0.05, rawDelta / 1000);
    this.lastFrame = now;
    this.frameTimes.push(rawDelta);
    if (this.frameTimes.length > 60) this.frameTimes.shift();

    this.handleMovement(dt);
    this.updateHover();
    this.updateBrushCursor();

    // ---- LOD cut + streaming (throttled: the cut does not need 60 Hz) ------
    this.lodTimer += dt;
    if (this.lodTimer > 0.12 || this.surfaceDirty) {
      this.lodTimer = 0;
      this.lod.screenErrorMetres = DEFAULT_LOD_SETTINGS.screenErrorMetres / this.lodBias;
      const cut = selectChunks(this.rig.position, this.rig.forward, this.lod);
      this.streamer.update(cut.requests, this.rig.position);
      this.surfaceDirty = false;
    }

    // ---- cameras ----------------------------------------------------------
    const viewport = this.container.querySelector('#pf-viewport') as HTMLElement;
    this.rig.syncToThree(viewport.clientWidth / Math.max(1, viewport.clientHeight));

    // Ocean is camera-anchored: rings follow the camera in whole cells, so the
    // near water is always the water you are looking at, and the far rings still
    // resolve the open sea out to the horizon without a second simulation.
    const oceanFrame = this.oceanFrame();
    if (oceanFrame) {
      this.ocean.setFrame(oceanFrame);
      this.ocean.followCamera(this.rig.position, oceanFrame);
      this.ocean.rebuildGeometry();
      this.ocean.simulate(dt);
      this.ocean.update(this.rig.camera);
    }

    this.terrain.sync(this.rig.position, this.rig.camera.matrixWorld);
    this.sky.update(this.rig.camera, this.sunDir, this.rig.far * 0.9);

    // Atmosphere shell follows the planet in camera-relative space.
    this.atmosphere.position.set(
      -this.rig.position.x,
      -this.rig.position.y,
      -this.rig.position.z,
    );
    (this.atmosphere.material as THREE.ShaderMaterial).uniforms.uSunDir.value.set(
      this.sunDir.x, this.sunDir.y, this.sunDir.z,
    );

    if (this.wireLogOfSun) {
      this.wireLogOfSun.position.set(this.sunDir.x * 1e6, this.sunDir.y * 1e6, this.sunDir.z * 1e6);
    }

    this.renderer.render(this.scene, this.rig.camera);

    // ---- telemetry --------------------------------------------------------
    this.statsTimer += dt;
    if (this.statsTimer > 0.4) {
      this.statsTimer = 0;
      this.publishStats();
    }
  }

  private handleMovement(dt: number): void {
    const k = this.keys;
    let right = 0;
    let forward = 0;
    let up = 0;
    if (k.has('w')) forward += 1;
    if (k.has('s')) forward -= 1;
    if (k.has('a')) right -= 1;
    if (k.has('d')) right += 1;
    if (k.has('e') || k.has(' ')) up += 1;
    if (k.has('q')) up -= 1;
    if (right === 0 && forward === 0 && up === 0) return;

    let speed = this.rig.speed;
    if (k.has('shift')) speed *= this.rig.boost;
    if (k.has('control')) speed *= this.rig.slow;
    // Scale fly speed with altitude so the same key speed works on the ground
    // and in orbit — a fixed 90 m/s is useless at 6000 km.
    const alt = Math.max(3, this.rig.altitude);
    speed = Math.max(speed, alt * 0.55);

    this.rig.move(right * speed * dt, forward * speed * dt, up * speed * dt);
  }

  private frameTimes: number[] = [];

  private publishStats(): void {
    const ts = this.terrain.stats();
    const ss = this.streamer.getStats();
    const os = this.ocean.stats();
    const rows: Array<[string, string]> = [
      ['FPS', this.frameTimes.length
        ? `${(1000 / (this.frameTimes.reduce((a, b) => a + b, 0) / this.frameTimes.length)).toFixed(0)}`
        : '—'],
      ['Chunks', `${ts.chunks} / ${ss.pending} queued`],
      ['Triangles', fmtCount(ts.triangles)],
      ['Terrain VRAM', `${ts.uploadedMB.toFixed(1)} MB`],
      ['Water tris', fmtCount(os.surfaceTriangles)],
      ['Water cells', fmtCount(os.simulatedCells)],
      ['Water substeps', `${os.substeps}`],
      ['Wave height', `${os.maxEta.toFixed(2)} m`],
      ['Mesh avg', `${ss.avgBuildMs.toFixed(1)} ms`],
      ['Cache', ss.cacheHits > 0 ? `${ss.cacheHits} hits` : `${ss.evicted} evicted`],
      ['Altitude', `${fmtKm(this.rig.altitude)}`],
    ];
    this.ui.setStats(rows);
    if (ss.lastError) this.ui.setStatus(`Worker: ${ss.lastError}`, 'error');
  }

  /* ------------------------------------------------------------------- boot */

  async start(): Promise<void> {
    this.running = true;
    this.resetCamera();
    this.ui.setWorldName(this.world.name);
    this.refreshOperatorList();

    const cached = await this.cache.open();
    this.diskCacheReady = cached;
    this.ui.setStatus(cached
      ? 'Mesh cache online — revisits are streamed from disk, not re-meshed'
      : 'Mesh cache unavailable (private window?) — streaming from memory only', 'info');

    this.streamer.setWorld(this.world);
    const loop = (): void => {
      if (!this.running) return;
      this.frame();
      requestAnimationFrame(loop);
    };
    requestAnimationFrame(loop);
  }

  get ready(): boolean { return this.diskCacheReady; }

  dispose(): void {
    this.running = false;
    this.streamer.dispose();
    this.ocean.dispose();
    this.renderer.dispose();
  }
}

/* ---------------------------------------------------------------- utilities */

function fmtKm(metres: number): string {
  if (Math.abs(metres) >= 1_000_000) return `${(metres / 1_000_000).toFixed(2)} Mm`;
  if (Math.abs(metres) >= 10_000) return `${Math.round(metres / 1000)} km`;
  if (Math.abs(metres) >= 1000) return `${(metres / 1000).toFixed(1)} km`;
  return `${Math.round(metres)} m`;
}

function fmtCount(n: number): string {
  if (n >= 1e6) return `${(n / 1e6).toFixed(2)}M`;
  if (n >= 1e3) return `${(n / 1e3).toFixed(n >= 1e5 ? 0 : 1)}k`;
  return `${Math.round(n)}`;
}

function opLabel(op: TerrainWorld['operators'][number]): string {
  return op.id.replace(/[-_]/g, ' ').replace(/\b\w/g, (c) => c.toUpperCase());
}

function opShortKind(type: string, kind: string): string {
  if (type === 'cap') return kind === 'land' ? 'land' : 'sea';
  if (type === 'capsule') return kind;
  if (type === 'ellipse') return kind;
  if (type === 'band') return 'strata';
  return 'crack';
}

export { cross, len };
