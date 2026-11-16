// Boot: the compile context (workers + icon baker, used headlessly for the ControlVehicle sample), the asset library,
// the import paths (file picker, drag-and-drop, sample sources) and the Geometry Compiler hand-off. The compiler is a
// separate workspace (Experimental/GeometryCompiler/Compile.html) framed full-window; see CompileExchange.js.
import { $, toast } from '../Shared/Studio/Ui.js';
import { hydrateIcons } from '../Shared/Studio/Icons.js';
import { createAsset, compileAll, fmt } from '../Shared/GeometryCompile/CompileSteps.js';
import { readFacet } from '../Shared/GeometryCompile/Engine/ImportFormats.js';
import { createCompileContext } from '../Shared/GeometryCompile/CompileContext.js';
import { COMPILE_SAMPLES, assetFromFile, fetchModel } from '../Shared/GeometryCompile/CompileSources.js';
import { hostCompiler } from '../Shared/GeometryCompile/CompileExchange.js';
import { createBlueprint, addComponent, findComponent, VEHICLE_SHEET } from './Blueprint.js';
import { VEHICLE_PALETTE } from '../Shared/Studio/Render.js';
import { createComponentViewer } from './ComponentViewer.js';

const assets = new Map();
const { ctx, iconRenderer } = createCompileContext();

const HOST_CHANNEL = 'frontier-component-viewer';
const isFrontierHost = new URLSearchParams(location.search).get('host') === 'frontier-editor' && window.parent && window.parent !== window;
let activeWorldEntityId = null;
let vehicleBp = null;
const worldBlueprints = new Map();

function postToFrontierHost(payload) {
  if (!isFrontierHost) return;
  try { window.parent.postMessage({ channel: HOST_CHANNEL, ...payload }, '*'); } catch {}
}

// ------------------------------------------------------------------------------------------------- import paths
// Sessions run in the Geometry Compiler page; it hands back a compiled copy of the asset (or a cancel).
const compiler = hostCompiler($('Compile'), {
  onFinish(asset, { cancelled, open, existing }) {
    if (cancelled) { viewer.refresh(); return; }
    assets.set(asset.id, asset);
    postToFrontierHost({ type: 'asset-compiled', asset });
    if (existing) { viewer.refresh(); toast(`${asset.name} recompiled · placed meshes updated`); return; }
    viewer.refresh();
    if (open) {
      const bp = createBlueprintFor(asset);
      viewer.open(bp);
      toast(`${asset.name} compiled · opened as ${bp.name}`);
    }
  },
  onError(error) { $('Compile').hidden = true; toast(`Geometry Compiler: ${error.message || error}`); },
});

function openCompile(asset, options = {}) {
  compiler.open(asset, options);
}

async function importFiles(files) {
  for (const file of files) {
    try {
      openCompile(await assetFromFile(file));
      return; // one compile session at a time; the rest can be imported after
    } catch (error) {
      toast(String(error.message || error));
    }
  }
}

$('FileInput').addEventListener('change', (e) => { importFiles(e.target.files); e.target.value = ''; });
addEventListener('dragover', (e) => { if ([...e.dataTransfer.types].includes('Files')) e.preventDefault(); });
addEventListener('drop', (e) => { if (e.dataTransfer.files.length && !e.defaultPrevented) { e.preventDefault(); importFiles(e.dataTransfer.files); } });

// A blueprint wrapping one asset: root → Static Mesh (+ a collision component matching the compiled shape).
function createBlueprintFor(asset) {
  const bp = createBlueprint(`BP_${asset.name.replace(/[^\w]+/g, '')}`);
  const mesh = addComponent(bp, 'StaticMesh', { name: asset.name.replace(/[^\w]+/g, ''), props: { asset: asset.id } });
  const c = asset.out.collision;
  if (c && c.type === 'box') addComponent(bp, 'BoxCollision', { parentId: mesh.id, name: 'Bounds', props: { extent: c.half.map((v) => +v.toFixed(3)), location: c.centre.map((v) => +v.toFixed(3)) } });
  if (c && c.type === 'sphere') addComponent(bp, 'SphereCollision', { parentId: mesh.id, name: 'Bounds', props: { radius: +c.radius.toFixed(3), location: c.centre.map((v) => +v.toFixed(3)) } });
  if (c && c.type === 'capsule') addComponent(bp, 'CapsuleCollision', { parentId: mesh.id, name: 'Bounds', props: { radius: +c.radius.toFixed(3), halfHeight: +c.halfHeight.toFixed(3), location: c.centre.map((v) => +v.toFixed(3)), rotation: c.axis === 0 ? [0, 0, 90] : c.axis === 2 ? [90, 0, 0] : [0, 0, 0] } });
  const g = asset.out.geometry;
  const r = g ? g.bounds.radius : 1;
  addComponent(bp, 'PointLight', { name: 'KeyLight', props: { location: [r * 1.2, r * 1.6 + 0.4, r * 1.2], intensity: 4 + r * 4, range: r * 6 + 2 } });
  bp.dirty = true;
  return bp;
}
window.FrontierComponentViewer = { createBlueprintFor, assets };

// ------------------------------------------------------------------------------------------------------ samples
const samples = COMPILE_SAMPLES.map((s) => ({ ...s, async open() { openCompile(await s.source()); } }));

// ---------------------------------------------------------------------------------------------- vehicle sample
const VEHICLE_PARTS = [
  { key: 'body', file: 'ControlVehicleBody.facet', name: 'ControlVehicle Body', tags: 'vehicle, body, fct1', collision: 'box', normals: 'angle' },
  { key: 'tyre', file: 'ControlVehicleTyre.facet', name: 'ControlVehicle Tyre', tags: 'vehicle, wheel, fct1', collision: 'box', normals: 'smooth' },
  { key: 'rim', file: 'ControlVehicleRim.facet', name: 'ControlVehicle Rim', tags: 'vehicle, wheel, fct1', collision: 'none', normals: 'angle' },
  { key: 'wing', build: buildWing, name: 'Rear Wing', tags: 'vehicle, wing, aero', collision: 'box', normals: 'angle' },
];

// A rear wing built from boxes (Y-up, metres, origin at the mounting point; +X is the car's forward direction): carbon
// main plane with a gurney flap, body-colour endplates and blue pylons, using the Vehicle Lab material families.
//   Angle: 8° angle of attack for downforce — the leading edge (front, +X) sits LOW and the trailing edge (rear, −X)
//   HIGH, and the gurney flap stands up off the trailing edge.  (It was tilted the other way, which is a lift wing.)
//   Span: endplate outer faces line up with the body's sides (ControlVehicleBody is 2.4996 m wide, ±1.2498 m).
const WING = { aoa: 8, bodyHalfWidth: 1.2498, chordHalf: 0.17, planeHalfThick: 0.016, plateHalfThick: 0.008, planeY: 0.25 };
function buildWing() {
  const pos = [], idx = [], fam = [];
  const box = (c, h, family, tilt = 0) => {
    const base = pos.length / 3, cs = Math.cos(tilt), sn = Math.sin(tilt);
    for (let i = 0; i < 8; i++) {
      const x = (i & 1 ? 1 : -1) * h[0], y = (i & 2 ? 1 : -1) * h[1], z = (i & 4 ? 1 : -1) * h[2];
      pos.push(c[0] + x * cs - y * sn, c[1] + x * sn + y * cs, c[2] + z);
    }
    const quads = [[0, 2, 3, 1], [4, 5, 7, 6], [0, 1, 5, 4], [2, 6, 7, 3], [0, 4, 6, 2], [1, 3, 7, 5]];
    for (const [a, b, c2, d] of quads) { idx.push(base + a, base + b, base + c2, base + a, base + c2, base + d); fam.push(family, family); }
  };
  const { aoa, bodyHalfWidth, chordHalf, planeHalfThick, plateHalfThick, planeY } = WING;
  const tilt = -aoa * Math.PI / 180;                       // negative about Z: front edge down, rear edge up
  const plateZ = bodyHalfWidth - plateHalfThick;            // endplate centres, outer faces flush with the body sides
  const span = plateZ - plateHalfThick;                     // main plane meets the endplates' inner faces
  box([0, planeY, 0], [chordHalf, planeHalfThick, span], 8, tilt);
  // Gurney flap: a 44 mm lip standing on the trailing edge's top surface, square to the plane.
  const te = [-chordHalf * Math.cos(tilt) - planeHalfThick * Math.sin(tilt), planeY - chordHalf * Math.sin(tilt) + planeHalfThick * Math.cos(tilt)];
  box([te[0] + 0.008 * Math.cos(tilt) - 0.022 * Math.sin(tilt), te[1] + 0.008 * Math.sin(tilt) + 0.022 * Math.cos(tilt), 0], [0.008, 0.022, span], 6, tilt);
  for (const z of [-plateZ, plateZ]) box([-0.01, 0.23, z], [0.2, 0.1, plateHalfThick], 6);
  // Pylons spread with the wider span (about 45 % out from the centre line), so the plane is not cantilevered too far.
  for (const z of [-0.56, 0.56]) box([0.02, 0.11, z], [0.05, 0.11, 0.01], 10);
  return { positions: new Float32Array(pos), indices: new Uint32Array(idx), families: new Uint32Array(fam), format: 'Procedural', notes: ['Rear wing built in Main.js for the Dock socket demo'] };
}

async function loadVehicle(report) {
  const parts = {};
  let i = 0;
  for (const part of VEHICLE_PARTS) {
    report(`Reading ${part.file}`, i / VEHICLE_PARTS.length);
    const bytes = part.build ? null : await fetchModel(part.file);
    const raw = part.build ? part.build() : readFacet(bytes);
    const asset = createAsset({
      name: part.name, fileName: part.file || `${part.key}.procedural`, fileSize: bytes ? bytes.byteLength : 0, raw, origin: 'sample', palette: VEHICLE_PALETTE,
      settings: {
        source: { up: part.build ? 'y' : 'z' }, geometry: { centre: 'none', normals: part.normals }, collision: { type: part.collision },
        sdf: { resolution: 40 }, icon: { yaw: part.key === 'body' ? 40 : 60, pitch: 18 },
        meta: { name: part.name, category: 'Vehicle part', tags: part.tags, notes: part.build ? 'Procedural rear wing for the Dock socket demo.' : 'Vehicle Lab ControlVehicle source mesh (Frontier-OLD · VehicleProvingGrounds/Archive).' },
      },
    });
    assets.set(asset.id, asset);
    await compileAll(asset, ctx, (step, f) => report(`Compiling ${part.name} · ${step}`, (i + f * 0.9) / VEHICLE_PARTS.length));
    asset.compiledAt = Date.now();
    parts[part.key] = asset;
    i++;
  }
  return parts;
}

// ControlVehicle as a Blueprint class: Vehicle Lab's sockets become component transforms (authored Z-up → Y-up:
// x, y, z → x, z, −y), Vehicle Lab's sheets become the Vehicle Movement component.
function buildVehicleBlueprint(parts) {
  const bp = createBlueprint('BP_ControlVehicle', 'Pawn');
  const body = addComponent(bp, 'StaticMesh', { name: 'Body', props: { asset: parts.body.id, location: [0, +VEHICLE_SHEET.groundOffset.toFixed(5), 0], mass: 1220, simulatePhysics: true } });
  // Each wheel hangs from a Spring socket at its axle mount (meets road bumps, squats and lifts with acceleration);
  // the front ones turn on a Hinge socket (±32° steering lock) between the spring and the wheel.
  const names = ['FL', 'FR', 'RL', 'RR'];
  const wheels = VEHICLE_SHEET.axleMount.map((m, i) => {
    const spring = addComponent(bp, 'Socket', { parentId: body.id, name: `Suspension${names[i]}`, props: { kind: 'spring', location: [m[0], m[2], -m[1]], springHz: 2, damping: 0.42, springTravel: 0.09, bumpSpacing: 7, loadGain: 1 } });
    const mount = m[0] > 0 ? addComponent(bp, 'Socket', { parentId: spring.id, name: `Steer${names[i]}`, props: { kind: 'hinge', angleMin: -32, angleMax: 32, pose: 0.5 } }) : spring;
    const tyre = addComponent(bp, 'StaticMesh', { parentId: mount.id, name: `Wheel${names[i]}`, props: { asset: parts.tyre.id, rotation: [0, m[1] < 0 ? 180 : 0, 0], mass: 20 } });
    addComponent(bp, 'StaticMesh', { parentId: tyre.id, name: `Wheel${names[i]}Rim`, props: { asset: parts.rim.id } });
    return tyre;
  });
  addComponent(bp, 'SpotLight', { parentId: body.id, name: 'HeadlightL', props: { location: [2.95, 0.42, -0.78], rotation: [0, 0, -4], intensity: 30, angle: 28, range: 30 } });
  addComponent(bp, 'SpotLight', { parentId: body.id, name: 'HeadlightR', props: { location: [2.95, 0.42, 0.78], rotation: [0, 0, -4], intensity: 30, angle: 28, range: 30 } });
  // Vehicle Lab's rear wing socket, now a Dock keyed “wing”: Simulate runs a pull test (the wing tears off above
  // 1.2 kN, drifts out and snaps back inside 0.3 m). Release / Connect work in the editor too.
  const wingDock = addComponent(bp, 'Socket', { parentId: body.id, name: 'RearWingSocket', props: { kind: 'dock', location: [-2.9822, 0.8367, 0], key: 'wing', snapRadius: 0.3, breakForce: 1200, drive: 'pull' } });
  addComponent(bp, 'StaticMesh', { parentId: wingDock.id, name: 'RearWing', props: { asset: parts.wing.id, tags: 'wing', mass: 6 } });
  addComponent(bp, 'Socket', { parentId: body.id, name: 'TowStrap', props: { kind: 'tether', location: [-3.1, 0.1, 0], length: 0.32, swing: 30, tetherDamping: 0.3 } });
  addComponent(bp, 'Arrow', { parentId: body.id, name: 'Forward', props: { location: [3.2, 0.2, 0], length: 0.8 } });
  addComponent(bp, 'Camera', { name: 'ChaseCamera', props: { location: [-7.2, 2.4, 0], rotation: [0, 0, -12], fov: 62 } });
  addComponent(bp, 'BoxCollision', { parentId: body.id, name: 'FrontBumperTrigger', props: { location: [3.05, 0.1, 0], extent: [0.18, 0.28, 1.05], trigger: true } });
  const vm = addComponent(bp, 'VehicleMovement', { name: 'VehicleMovement' });
  vm.props.wheels = wheels.map((w) => w.id);
  bp.dirty = false;
  return bp;
}

// ------------------------------------------------------------------------------- Frontier Editor world bridge
function buildProceduralWorldMesh(entity) {
  const shape = String(entity?.Shape || 'box').toLowerCase();
  const sx = Math.max(0.15, Number(entity?.Size?.[0] ?? 1.0));
  const sy = Math.max(0.15, Number(entity?.Size?.[2] ?? 1.0)); // Z-up height -> Y-up
  const sz = Math.max(0.15, Number(entity?.Size?.[1] ?? 1.0));
  const pos = [], idx = [], fam = [];
  const pushTri = (a, b, c) => { idx.push(a, b, c); fam.push(0); };

  if (shape === 'sphere' || shape === 'orb') {
    const rx = sx * 0.5, ry = sy * 0.5, rz = sz * 0.5;
    const segU = 24, segV = 14;
    for (let v = 0; v <= segV; v++) {
      const phi = (v / segV) * Math.PI;
      const sp = Math.sin(phi), cp = Math.cos(phi);
      for (let u = 0; u <= segU; u++) {
        const theta = (u / segU) * Math.PI * 2;
        pos.push(Math.cos(theta) * sp * rx, cp * ry, Math.sin(theta) * sp * rz);
      }
    }
    const stride = segU + 1;
    for (let v = 0; v < segV; v++) {
      for (let u = 0; u < segU; u++) {
        const a = v * stride + u, b = a + 1, c = a + stride, d = c + 1;
        if (v > 0) pushTri(a, c, b);
        if (v < segV - 1) pushTri(b, c, d);
      }
    }
  } else if (shape === 'cylinder' || shape === 'cone') {
    const rBot = Math.max(sx, sz) * 0.5;
    const rTop = shape === 'cone' ? rBot * 0.08 : rBot;
    const hh = sy * 0.5;
    const seg = 24;
    const botCenter = 0; pos.push(0, -hh, 0);
    const topCenter = 1; pos.push(0, hh, 0);
    const ringStart = 2;
    for (let i = 0; i < seg; i++) {
      const a = (i / seg) * Math.PI * 2;
      const ca = Math.cos(a), sa = Math.sin(a);
      pos.push(ca * rBot, -hh, sa * rBot);
      pos.push(ca * rTop, hh, sa * rTop);
    }
    for (let i = 0; i < seg; i++) {
      const n = (i + 1) % seg;
      const b0 = ringStart + i * 2, t0 = b0 + 1;
      const b1 = ringStart + n * 2, t1 = b1 + 1;
      pushTri(b0, t0, b1); pushTri(b1, t0, t1);
      pushTri(botCenter, b0, b1);
      pushTri(topCenter, t1, t0);
    }
  } else {
    const hx = sx * 0.5, hy = sy * 0.5, hz = sz * 0.5;
    for (let i = 0; i < 8; i++) {
      pos.push((i & 1 ? 1 : -1) * hx, (i & 2 ? 1 : -1) * hy, (i & 4 ? 1 : -1) * hz);
    }
    const quads = [[0, 2, 3, 1], [4, 5, 7, 6], [0, 1, 5, 4], [2, 6, 7, 3], [0, 4, 6, 2], [1, 3, 7, 5]];
    for (const [a, b, c, d] of quads) { pushTri(a, b, c); pushTri(a, c, d); }
  }
  return {
    positions: new Float32Array(pos),
    indices: new Uint32Array(idx),
    families: new Uint32Array(fam),
    format: 'Frontier World Actor',
    notes: [`Synthesized from Frontier World actor "${entity?.Name || 'Actor'}" (${shape})`],
  };
}

async function openWorldEntityBlueprint(entity, compiledAsset) {
  activeWorldEntityId = entity?.Id || null;
  if (entity?.BlueprintKind === 'BP_ControlVehicle' && vehicleBp) {
    viewer.open(vehicleBp);
    toast(`Inspecting ${entity.Name || 'BP_ControlVehicle'} · Blueprint Inspector`);
    return;
  }
  if (compiledAsset && compiledAsset.id) {
    assets.set(compiledAsset.id, compiledAsset);
    let bp = worldBlueprints.get(entity.Id);
    if (!bp) {
      bp = createBlueprintFor(compiledAsset);
      bp.name = `BP_${String(entity?.Name || compiledAsset.name || 'CompiledAsset').replace(/[^\w]+/g, '')}`;
      bp.dirty = false;
      worldBlueprints.set(entity.Id, bp);
    }
    viewer.open(bp);
    toast(`Inspecting ${bp.name} · Compiled Geometry Asset`);
    return;
  }
  const cacheKey = entity?.Id || entity?.Name || 'world-actor';
  let bp = worldBlueprints.get(cacheKey);
  if (!bp) {
    const safeName = String(entity?.Name || 'WorldActor').replace(/[^\w]+/g, '') || 'WorldActor';
    const baseHex = entity?.ColorHex || '#c8ccd4';
    const raw = buildProceduralWorldMesh(entity);
    const palette = [{
      name: entity?.MaterialName || 'WorldSurface',
      color: baseHex,
      roughness: Number(entity?.Roughness ?? 0.48),
      metalness: Number(entity?.Metallic ?? 0.08),
    }];
    const asset = createAsset({
      name: entity?.Name || safeName,
      fileName: `${safeName}.world`,
      fileSize: raw.positions.byteLength + raw.indices.byteLength,
      raw,
      origin: 'world',
      palette,
      settings: {
        source: { up: 'y' },
        geometry: { centre: 'none', normals: entity?.Shape === 'sphere' ? 'smooth' : 'angle' },
        collision: { type: entity?.Shape === 'sphere' ? 'sphere' : 'box' },
        sdf: { resolution: 32 },
        meta: { name: entity?.Name || safeName, category: entity?.Category || 'World Actor', tags: 'frontier, world', notes: `Live actor inspected from ${entity?.LevelName || 'Frontier World'}.` },
      },
    });
    assets.set(asset.id, asset);
    await compileAll(asset, ctx);
    asset.compiledAt = Date.now();
    bp = createBlueprintFor(asset);
    bp.name = `BP_${safeName}`;
    if (entity?.IsAreaLight) {
      addComponent(bp, 'PointLight', { name: 'AreaEmitter', props: { location: [0, 0.25, 0], intensity: 14, range: 12, color: entity?.EmissiveHex || '#fff4e0' } });
    }
    if (entity?.IsCamera) {
      addComponent(bp, 'Camera', { name: 'WorldCamera', props: { location: [0, 0.35, 0], fov: 50 } });
    }
    bp.dirty = false;
    worldBlueprints.set(cacheKey, bp);
  }
  viewer.open(bp);
  toast(`Inspecting ${bp.name} · Frontier World Actor`);
}

function collectBlueprintSummary() {
  const bp = viewer.blueprint;
  if (!bp) return null;
  const wing = bp.components.find((c) => c.name === 'RearWing');
  const headL = bp.components.find((c) => c.name === 'HeadlightL');
  const headR = bp.components.find((c) => c.name === 'HeadlightR');
  return {
    entityId: activeWorldEntityId,
    blueprintName: bp.name,
    componentCount: bp.components.length,
    wingVisible: wing ? wing.props.visible !== false : true,
    headlightsOn: headL || headR ? ((headL?.props.visible !== false && (headL?.props.intensity ?? 1) > 0) || (headR?.props.visible !== false && (headR?.props.intensity ?? 1) > 0)) : true,
  };
}

if (isFrontierHost) {
  const returnBtn = $('ReturnToWorldBtn');
  if (returnBtn) {
    returnBtn.hidden = false;
    returnBtn.addEventListener('click', () => {
      postToFrontierHost({ type: 'sync', close: true, summary: collectBlueprintSummary() });
    });
  }
  $('CompileBpBtn')?.addEventListener('click', () => {
    setTimeout(() => postToFrontierHost({ type: 'sync', close: false, summary: collectBlueprintSummary() }), 30);
  });
  addEventListener('message', async (e) => {
    const m = e.data;
    if (!m || m.channel !== HOST_CHANNEL) return;
    if (m.type === 'register-assets' && Array.isArray(m.assets)) {
      for (const a of m.assets) { if (a?.id) assets.set(a.id, a); }
      viewer.refresh();
    } else if (m.type === 'inspect-entity') {
      if (Array.isArray(m.compiledAssets)) {
        for (const a of m.compiledAssets) { if (a?.id) assets.set(a.id, a); }
      }
      await openWorldEntityBlueprint(m.entity, m.compiledAsset);
    } else if (m.type === 'open-control-vehicle' && vehicleBp) {
      activeWorldEntityId = m.entityId || 'hero-vehicle';
      viewer.open(vehicleBp);
    }
  });
}

// ---------------------------------------------------------------------------------------------------------- boot
const viewer = createComponentViewer({ assets, iconRenderer, openCompile, importFiles, samples });
hydrateIcons(document);

(async () => {
  const boot = $('Boot');
  const report = (text, f) => { $('BootText').textContent = text; $('BootBar').style.width = `${Math.round(f * 100)}%`; };
  try {
    const parts = await loadVehicle(report);
    report('Assembling BP_ControlVehicle', 1);
    const bp = buildVehicleBlueprint(parts);
    vehicleBp = bp;
    viewer.open(bp);
    bp.icon = null;
    boot.classList.add('done');
    setTimeout(() => boot.remove(), 400);
    const tris = Object.values(parts).reduce((n, a) => n + a.out.geometry.indices.length / 3, 0);
    toast(`ControlVehicle compiled · ${fmt(tris)} tris · 3 assets`);
    postToFrontierHost({ type: 'ready', blueprintName: bp.name, triangles: tris });
  } catch (error) {
    console.error(error);
    $('BootText').textContent = `Could not load the ControlVehicle sample: ${error.message}. Import an object to start.`;
    boot.classList.add('failed');
    const bp = createBlueprint('BP_Empty');
    viewer.open(bp);
    setTimeout(() => boot.remove(), 2500);
    postToFrontierHost({ type: 'ready', error: String(error.message || error) });
  }
})();

export { assets, findComponent };
