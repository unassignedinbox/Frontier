// ============================================================================
// SpecimenBay · Prototype 02 — Redesigned Outliner + 1-or-Many Selection Model
//   · Every imported asset is treated as 1 single object in the Outliner.
//   · 1 object selected in Outliner -> 1 item in Inspector & 1 object on Stage.
//   · Many objects selected in Outliner -> Many objects in Inspector (shared
//     batch header + per-object cards) & all selected objects staged cleanly
//     side-by-side on the 3D Stage!
//   · Full 4-Mode Collision Editor (Convex Hull, Bounding Box, Primitives,
//     Use Mesh Faces) working on BOTH single & multi-object selections.
//   · Stress Fracture left cleanly blank.
// ============================================================================

import {
  THREE,
  Stage,
  IconRenderer,
  meshObject,
  lodGeometry,
  assetSlots,
  VEHICLE_PALETTE,
} from '../../Experimental/Shared/Studio/Render.js';
import { createCameraHud } from '../../Experimental/Shared/Studio/Camera.js';
import { createAsset, runStep } from '../../Experimental/Shared/GeometryCompile/CompileSteps.js';
import { createCompileContext } from '../../Experimental/Shared/GeometryCompile/CompileContext.js';
import { COMPILE_SAMPLES, assetFromFile, fetchModel, displayName } from '../../Experimental/Shared/GeometryCompile/CompileSources.js';
import { readFacet } from '../../Experimental/Shared/GeometryCompile/Engine/ImportFormats.js';
import { constructSource, SOURCE_PRESETS } from '../../Experimental/Shared/GeometryCompile/Engine/SourceGeometry.js';
import { formatObj, formatUvSvg } from '../../Experimental/Shared/GeometryCompile/Engine/ObjExport.js';
import { compileSparseSdfVolume, extractSdfSliceRgba, serializeVolumeFvol } from '../../Experimental/GeometryCompiler/Engine/SparseSdfBaker.js';
import { SdfRaymarchViewport } from '../../Experimental/GeometryCompiler/Engine/SdfRaymarchViewport.js';

const $ = (id) => document.getElementById(id);
const El = (tag, cls, html) => {
  const n = document.createElement(tag);
  if (cls) n.className = cls;
  if (html !== undefined) n.innerHTML = html;
  return n;
};
const FmtK = (n) => (n >= 1000 ? `${(n / 1000).toFixed(1)}k` : `${n}`);

function boxOf(geo) {
  const pos = geo?.positions;
  if (!pos || !pos.length) return { min: { x: -0.5, y: 0, z: -0.5 }, max: { x: 0.5, y: 1, z: 0.5 } };
  let x0 = Infinity, y0 = Infinity, z0 = Infinity, x1 = -Infinity, y1 = -Infinity, z1 = -Infinity;
  for (let i = 0; i < pos.length; i += 3) {
    const x = pos[i], y = pos[i + 1], z = pos[i + 2];
    if (x < x0) x0 = x; if (x > x1) x1 = x;
    if (y < y0) y0 = y; if (y > y1) y1 = y;
    if (z < z0) z0 = z; if (z > z1) z1 = z;
  }
  return { min: { x: x0, y: y0, z: z0 }, max: { x: x1, y: y1, z: z1 } };
}

// ============================================================================
// REAL 3D QUICKHULL (Convex Hull Builder)
// ============================================================================
function ComputeConvexHull3D(positions, maxVerts = 48, inflation = 0) {
  const n = positions.length / 3;
  if (n < 4) return null;
  const stride = Math.max(1, Math.floor(n / 900));
  const pts = [];
  let cx = 0, cy = 0, cz = 0;
  for (let i = 0; i < n; i += stride) {
    const x = positions[i * 3], y = positions[i * 3 + 1], z = positions[i * 3 + 2];
    pts.push([x, y, z]);
    cx += x; cy += y; cz += z;
  }
  cx /= pts.length; cy /= pts.length; cz /= pts.length;

  // Pick directional extreme points (Fibonacci sphere directions) up to maxVerts
  const dirs = [];
  const golden = Math.PI * (3 - Math.sqrt(5));
  const dirCount = Math.max(16, Math.min(maxVerts * 2, 240));
  for (let i = 0; i < dirCount; i++) {
    const y = 1 - (i / (dirCount - 1)) * 2;
    const r = Math.sqrt(Math.max(0, 1 - y * y));
    const t = golden * i;
    dirs.push([Math.cos(t) * r, y, Math.sin(t) * r]);
  }
  const pickedMap = new Map();
  for (const d of dirs) {
    let bestI = 0, bestD = -Infinity;
    for (let i = 0; i < pts.length; i++) {
      const p = pts[i];
      const dot = p[0] * d[0] + p[1] * d[1] + p[2] * d[2];
      if (dot > bestD) { bestD = dot; bestI = i; }
    }
    const p = pts[bestI];
    const key = `${Math.round(p[0] * 1000)},${Math.round(p[1] * 1000)},${Math.round(p[2] * 1000)}`;
    if (!pickedMap.has(key)) pickedMap.set(key, p);
  }
  const V = [...pickedMap.values()].slice(0, maxVerts);
  if (V.length < 4) return null;

  // Apply inflation outward from centroid
  const verts = V.map(([x, y, z]) => {
    if (!inflation) return [x, y, z];
    const dx = x - cx, dy = y - cy, dz = z - cz;
    const len = Math.hypot(dx, dy, dz) || 1;
    return [x + (dx / len) * inflation, y + (dy / len) * inflation, z + (dz / len) * inflation];
  });

  // Incremental 3D Convex Hull
  const sub = (a, b) => [a[0] - b[0], a[1] - b[1], a[2] - b[2]];
  const cross = (a, b) => [a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]];
  const dot = (a, b) => a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
  const norm = (a) => { const l = Math.hypot(a[0], a[1], a[2]) || 1; return [a[0] / l, a[1] / l, a[2] / l]; };

  let i0 = 0, i1 = 1, i2 = 2, i3 = 3;
  // Find initial non-degenerate tetrahedron
  let maxDist = -1;
  for (let i = 1; i < verts.length; i++) {
    const d = Math.hypot(...sub(verts[i], verts[i0]));
    if (d > maxDist) { maxDist = d; i1 = i; }
  }
  const e01 = sub(verts[i1], verts[i0]);
  maxDist = -1;
  for (let i = 0; i < verts.length; i++) {
    if (i === i0 || i === i1) continue;
    const area = Math.hypot(...cross(e01, sub(verts[i], verts[i0])));
    if (area > maxDist) { maxDist = area; i2 = i; }
  }
  const n012 = cross(e01, sub(verts[i2], verts[i0]));
  maxDist = -1;
  for (let i = 0; i < verts.length; i++) {
    if (i === i0 || i === i1 || i === i2) continue;
    const vol = Math.abs(dot(n012, sub(verts[i], verts[i0])));
    if (vol > maxDist) { maxDist = vol; i3 = i; }
  }
  if (maxDist <= 1e-8) return null;

  const makeFace = (a, b, c, inside) => {
    let nrm = cross(sub(verts[b], verts[a]), sub(verts[c], verts[a]));
    if (dot(nrm, sub(verts[inside], verts[a])) > 0) {
      [b, c] = [c, b];
      nrm = [-nrm[0], -nrm[1], -nrm[2]];
    }
    nrm = norm(nrm);
    return { a, b, c, n: nrm, d: dot(nrm, verts[a]) };
  };

  let faces = [
    makeFace(i0, i1, i2, i3),
    makeFace(i0, i1, i3, i2),
    makeFace(i0, i2, i3, i1),
    makeFace(i1, i2, i3, i0),
  ];
  const used = new Set([i0, i1, i2, i3]);

  for (let idx = 0; idx < verts.length; idx++) {
    if (used.has(idx)) continue;
    const p = verts[idx];
    const visible = [];
    const kept = [];
    for (const f of faces) {
      if (dot(f.n, p) - f.d > 1e-5) visible.push(f);
      else kept.push(f);
    }
    if (!visible.length) continue;
    const edgeCounts = new Map();
    const addEdge = (u, v) => {
      const rev = `${v}_${u}`;
      if (edgeCounts.has(rev)) edgeCounts.delete(rev);
      else edgeCounts.set(`${u}_${v}`, [u, v]);
    };
    for (const f of visible) {
      addEdge(f.a, f.b);
      addEdge(f.b, f.c);
      addEdge(f.c, f.a);
    }
    faces = kept;
    for (const [u, v] of edgeCounts.values()) {
      const nrm = norm(cross(sub(verts[v], verts[u]), sub(p, verts[u])));
      faces.push({ a: u, b: v, c: idx, n: nrm, d: dot(nrm, verts[u]) });
    }
  }

  // Compute exact enclosed volume and build BufferGeometry
  let volume = 0;
  const posArr = new Float32Array(faces.length * 9);
  const usedVerts = new Set();
  for (let i = 0; i < faces.length; i++) {
    const f = faces[i];
    usedVerts.add(f.a); usedVerts.add(f.b); usedVerts.add(f.c);
    const pa = verts[f.a], pb = verts[f.b], pc = verts[f.c];
    posArr.set(pa, i * 9);
    posArr.set(pb, i * 9 + 3);
    posArr.set(pc, i * 9 + 6);
    volume += dot(pa, cross(pb, pc)) / 6;
  }
  const geo = new THREE.BufferGeometry();
  geo.setAttribute('position', new THREE.BufferAttribute(posArr, 3));
  geo.computeVertexNormals();
  return {
    geometry: geo,
    vertCount: usedVerts.size,
    triCount: faces.length,
    volume: Math.abs(volume),
  };
}

// ============================================================================
// GEOMETRY MERGING: Build ControlVehicle as 1 Single Unified Imported Asset
// ============================================================================
function MergeRawFacets(items) {
  let totalVerts = 0, totalIdx = 0;
  for (const it of items) {
    totalVerts += it.raw.positions.length / 3;
    totalIdx += it.raw.indices.length;
  }
  const positions = new Float32Array(totalVerts * 3);
  const indices = new Uint32Array(totalIdx);
  const families = new Uint16Array(totalIdx / 3);
  let vOff = 0, iOff = 0, fOff = 0;

  for (const it of items) {
    const p = it.raw.positions;
    const idx = it.raw.indices;
    const fam = it.raw.families;
    const [tx, ty, tz] = it.offset || [0, 0, 0];
    const flip180Z = !!it.flip180Z;
    const nv = p.length / 3;
    for (let v = 0; v < nv; v++) {
      let x = p[v * 3], y = p[v * 3 + 1], z = p[v * 3 + 2];
      if (flip180Z) { x = -x; y = -y; }
      positions[(vOff + v) * 3]     = x + tx;
      positions[(vOff + v) * 3 + 1] = y + ty;
      positions[(vOff + v) * 3 + 2] = z + tz;
    }
    for (let j = 0; j < idx.length; j++) indices[iOff + j] = idx[j] + vOff;
    const nTris = idx.length / 3;
    for (let t = 0; t < nTris; t++) {
      families[fOff + t] = it.familyOverride !== undefined ? it.familyOverride : (fam ? fam[t] : 0);
    }
    vOff += nv;
    iOff += idx.length;
    fOff += nTris;
  }
  return { positions, indices, families, format: 'FCT1' };
}

// ============================================================================
// STATE
// ============================================================================
let NextObjNum = 1;
let NextPrimNum = 1;

const State = {
  objects: [],        // Array of unified Asset Objects (1 imported asset = 1 object!)
  selectedIds: [],    // 1 id -> single inspector; N ids -> multi-object inspector!
  lastClickedId: null,
  searchQuery: '',
  aspect: 'overview', // overview | collision | sdf | uv | lod | bounds | portrait | finish | fracture
  channel: 'lit',     // lit | unlit | density | normals | uv | lod | bounds
  view: 'persp',
  wireframe: false,
  grid: true,
  pivot: false,
  spin: false,
  stageLayout: 'lineup', // lineup | grid (for multi-object staging)
  // Collision viewport debug flags
  colDrawStyle: 'both',  // both | wire | solid
  colGhostMesh: true,
  colShowNormals: false,
  // SDF viewport debug state
  sdfMode: 'raymarch',   // raymarch | slice | cutaway
  sdfAxis: 'y',
  sdfSlicePos: 0.5,
  sdfIso: 0,
  // Portrait settings
  portraitBg: 'dark',
  portraitRes: 256,
  // Active collapsed states for multi-object inspector cards
  collapsedCards: new Set(),
};

let stage = null;
let iconRenderer = null;
let sdfViewport = null;
let sceneRoot = null;
let colliderRoot = null;
let helperRoot = null;
let toastTimer = null;
const compileCtx = createCompileContext();

// ============================================================================
// OBJECT FACTORY (1 Imported Asset = 1 Object)
// ============================================================================
function DefaultCollisionConfig(asset) {
  const b = boxOf(asset.out.geometry);
  const sz = [b.max.x - b.min.x, b.max.y - b.min.y, b.max.z - b.min.z];
  const ctr = [(b.min.x + b.max.x) / 2, (b.min.y + b.max.y) / 2, (b.min.z + b.max.z) / 2];
  return {
    mode: 'convex',       // convex | box | primitives | mesh_faces
    hullMaxVerts: 48,
    hullInflation: 0.004,
    hullCache: null,
    boxPadding: 0,
    boxSize: sz.map((v) => +v.toFixed(3)),
    boxCenter: ctr.map((v) => +v.toFixed(3)),
    primitives: AutoFitPrimitives(asset),
    selectedPrimId: null,
    meshSource: 'base',   // base | lod1 | lod2
    meshDoubleSided: false,
    meshWeld: true,
    meshMargin: 0,
    preset: 'solid',      // solid | static | vehicle | trigger
    friction: 0.65,
    restitution: 0.15,
    density: 1200,
  };
}

function AutoFitPrimitives(asset) {
  const b = boxOf(asset.out.geometry);
  const w = b.max.x - b.min.x;
  const h = b.max.y - b.min.y;
  const d = b.max.z - b.min.z;
  const cx = (b.min.x + b.max.x) / 2;
  const cy = (b.min.y + b.max.y) / 2;
  const cz = (b.min.z + b.max.z) / 2;

  // Smart auto-fit: if elongated vehicle-like proportions, fit chassis box + 4 wheel cylinders; else fitted box or cylinder/sphere
  if (w > 4.2 && d > 1.8 && h < 2.2) {
    return [
      { id: `PRIM-${NextPrimNum++}`, type: 'box', name: 'Chassis Body Box', pos: [cx, cy + 0.08, cz], size: [+(w * 0.96).toFixed(2), +(h * 0.72).toFixed(2), +(d * 0.88).toFixed(2)], rot: [0, 0, 0], radius: 0.5, height: 1.0, visible: true },
      { id: `PRIM-${NextPrimNum++}`, type: 'cylinder', name: 'Wheel · FL Cyl', pos: [1.73, 0.51, -1.05], size: [1.02, 0.4, 1.02], rot: [90, 0, 0], radius: 0.51, height: 0.40, visible: true },
      { id: `PRIM-${NextPrimNum++}`, type: 'cylinder', name: 'Wheel · FR Cyl', pos: [1.73, 0.51,  1.05], size: [1.02, 0.4, 1.02], rot: [90, 0, 0], radius: 0.51, height: 0.40, visible: true },
      { id: `PRIM-${NextPrimNum++}`, type: 'cylinder', name: 'Wheel · RL Cyl', pos: [-1.67, 0.51, -1.05], size: [1.02, 0.4, 1.02], rot: [90, 0, 0], radius: 0.51, height: 0.40, visible: true },
      { id: `PRIM-${NextPrimNum++}`, type: 'cylinder', name: 'Wheel · RR Cyl', pos: [-1.67, 0.51,  1.05], size: [1.02, 0.4, 1.02], rot: [90, 0, 0], radius: 0.51, height: 0.40, visible: true },
    ];
  }
  const aspectRatio = Math.max(w, d) / Math.max(0.01, Math.min(w, d));
  if (aspectRatio < 1.25 && Math.abs(h - w) < w * 0.35) {
    return [{
      id: `PRIM-${NextPrimNum++}`,
      type: 'sphere',
      name: 'Fitted Bounding Sphere',
      pos: [+cx.toFixed(2), +cy.toFixed(2), +cz.toFixed(2)],
      size: [+w.toFixed(2), +h.toFixed(2), +d.toFixed(2)],
      rot: [0, 0, 0],
      radius: +(Math.max(w, h, d) * 0.5).toFixed(2),
      height: +h.toFixed(2),
      visible: true,
    }];
  }
  return [{
    id: `PRIM-${NextPrimNum++}`,
    type: h > Math.max(w, d) * 1.15 ? 'capsule' : 'box',
    name: h > Math.max(w, d) * 1.15 ? 'Fitted Capsule' : 'Fitted Box',
    pos: [+cx.toFixed(2), +cy.toFixed(2), +cz.toFixed(2)],
    size: [+w.toFixed(2), +h.toFixed(2), +d.toFixed(2)],
    rot: [0, 0, 0],
    radius: +(Math.max(w, d) * 0.46).toFixed(2),
    height: +h.toFixed(2),
    visible: true,
  }];
}

function EnsureHull(obj) {
  const c = obj.collision;
  if (!c.hullCache || c.hullCache.maxVerts !== c.hullMaxVerts || c.hullCache.inflation !== c.hullInflation) {
    const hull = ComputeConvexHull3D(obj.asset.out.geometry.positions, c.hullMaxVerts, c.hullInflation);
    c.hullCache = hull ? { ...hull, maxVerts: c.hullMaxVerts, inflation: c.hullInflation } : null;
  }
  return c.hullCache;
}

function RenderIconForAsset(asset, overrides = {}, size = 128) {
  if (!iconRenderer || !asset?.out?.geometry) return null;
  const grp = meshObject(asset, { channel: 'lit', overrides });
  return iconRenderer.render(grp, { size });
}

async function CompileAssetCore(asset) {
  await runStep(asset, 'source', compileCtx);
  await runStep(asset, 'geometry', compileCtx);
  asset.out.icon = RenderIconForAsset(asset);
  return asset;
}

function BuildSlots(asset) {
  return assetSlots(asset).map((s) => ({
    family: s.family,
    name: s.name,
    color: s.color,
    roughness: s.roughness ?? 0.5,
    metalness: s.metalness ?? 0.05,
  }));
}

async function WrapAssetAsObject(asset, formatTag = 'MESH', colMode = 'convex') {
  await CompileAssetCore(asset);
  const col = DefaultCollisionConfig(asset);
  col.mode = colMode;
  col.selectedPrimId = col.primitives[0]?.id || null;
  const obj = {
    id: `OBJ-${NextObjNum++}`,
    name: asset.name,
    format: formatTag,
    asset,
    visible: true,
    slots: BuildSlots(asset),
    collision: col,
    sdf: { volume: null, resolution: 64, band: 3, baking: false },
    uvBaked: false,
    lodBaked: false,
    activeLod: -1,
  };
  EnsureHull(obj);
  return obj;
}

// ============================================================================
// INITIAL SPECIMEN LIBRARY (Each asset = 1 unified object in Outliner)
// ============================================================================
async function LoadInitialObjects() {
  // 1. ControlVehicle — 1 unified imported vehicle object (Body + 4 Wheels at exact VEHICLE_SHEET coordinates!)
  const bodyRaw = readFacet(await fetchModel('ControlVehicleBody.facet'));
  const tyreRaw = readFacet(await fetchModel('ControlVehicleTyre.facet'));
  const rimRaw  = readFacet(await fetchModel('ControlVehicleRim.facet'));

  // Exact Frontier VEHICLE_SHEET.axleMount coordinates in Z-up space
  const axleMounts = [
    { offset: [ 1.7274,  1.0475, 0.0914], flip180Z: false },
    { offset: [ 1.7274, -1.0475, 0.0914], flip180Z: true  },
    { offset: [-1.6686,  1.0475, 0.0914], flip180Z: false },
    { offset: [-1.6686, -1.0475, 0.0914], flip180Z: true  },
  ];
  const vehicleParts = [{ raw: bodyRaw, offset: [0, 0, 0] }];
  for (const ax of axleMounts) {
    vehicleParts.push({ raw: tyreRaw, offset: ax.offset, flip180Z: ax.flip180Z });
    vehicleParts.push({ raw: rimRaw,  offset: ax.offset, flip180Z: ax.flip180Z });
  }
  const vehicleRaw = MergeRawFacets(vehicleParts);
  const vehicleAsset = createAsset({
    name: 'ControlVehicle',
    fileName: 'ControlVehicle.facet',
    raw: vehicleRaw,
    palette: VEHICLE_PALETTE,
    settings: { source: { up: 'z' }, geometry: { centre: 'ground', normals: 'angle' } },
  });
  const vehicleObj = await WrapAssetAsObject(vehicleAsset, 'FCT1', 'convex');

  // 2. ControlVehicleWheel — 1 standalone wheel object
  const wheelRaw = MergeRawFacets([
    { raw: tyreRaw, offset: [0, 0, 0] },
    { raw: rimRaw,  offset: [0, 0, 0] },
  ]);
    const wheelAsset = createAsset({
    name: 'Vehicle Wheel',
    fileName: 'ControlVehicleWheel.facet',
    raw: wheelRaw,
    palette: VEHICLE_PALETTE,
    settings: { source: { up: 'z' }, geometry: { centre: 'ground', normals: 'smooth' } },
  });
  const wheelObj = await WrapAssetAsObject(wheelAsset, 'FCT1', 'primitives');

  // 3. Carved Torus — Frontier-1 procedural specimen
  const torusSrc = constructSource('torus', 12000);
  const torusAsset = createAsset({
    name: 'Carved Torus',
    fileName: 'carved_torus.preset',
    raw: { positions: torusSrc.positions, indices: torusSrc.indices, families: null, format: 'Procedural' },
    settings: { geometry: { centre: 'ground', normals: 'smooth' } },
  });
  const torusObj = await WrapAssetAsObject(torusAsset, 'MESH', 'convex');
  if (torusObj.slots[0]) torusObj.slots[0].color = '#c79a52';

  // 4. Fluted Vase — Frontier-1 procedural specimen
  const vaseSrc = constructSource('vase', 10000);
  const vaseAsset = createAsset({
    name: 'Fluted Vase',
    fileName: 'fluted_vase.preset',
    raw: { positions: vaseSrc.positions, indices: vaseSrc.indices, families: null, format: 'Procedural' },
    settings: { geometry: { centre: 'ground', normals: 'smooth' } },
  });
  const vaseObj = await WrapAssetAsObject(vaseAsset, 'MESH', 'box');
  if (vaseObj.slots[0]) vaseObj.slots[0].color = '#7aa4b8';

  // 5. Asteroid — Frontier-1 procedural specimen
  const rockSrc = constructSource('asteroid', 9000);
  const rockAsset = createAsset({
    name: 'Asteroid',
    fileName: 'asteroid.preset',
    raw: { positions: rockSrc.positions, indices: rockSrc.indices, families: null, format: 'Procedural' },
    settings: { geometry: { centre: 'ground', normals: 'angle' } },
  });
  const rockObj = await WrapAssetAsObject(rockAsset, 'MESH', 'mesh_faces');
  if (rockObj.slots[0]) rockObj.slots[0].color = '#8b8378';

  // Re-render thumbnails with custom slot colours
  for (const o of [torusObj, vaseObj, rockObj]) {
    o.asset.out.icon = RenderIconForAsset(o.asset, OverridesMap(o));
  }

  State.objects = [vehicleObj, wheelObj, torusObj, vaseObj, rockObj];
  State.selectedIds = [vehicleObj.id];
  State.lastClickedId = vehicleObj.id;
}

function OverridesMap(obj) {
  const m = {};
  for (const sl of obj.slots) m[sl.family] = sl.color;
  return m;
}

function SelectedObjects() {
  const set = new Set(State.selectedIds);
  return State.objects.filter((o) => set.has(o.id));
}

function Toast(msg) {
  const el = $('Toast');
  el.textContent = msg;
  el.classList.remove('hidden');
  clearTimeout(toastTimer);
  toastTimer = setTimeout(() => el.classList.add('hidden'), 2600);
}

// ============================================================================
// 3D STAGE & COLLISION VISUALIZATION (Single OR Multi-Object Staging!)
// ============================================================================
function ComputeStagingOffsets(objs) {
  if (objs.length <= 1) return objs.map(() => [0, 0, 0]);
  const sizes = objs.map((o) => {
    const b = boxOf(o.asset.out.geometry);
    return {
      w: Math.max(0.6, b.max.x - b.min.x),
      h: Math.max(0.6, b.max.y - b.min.y),
      d: Math.max(0.6, b.max.z - b.min.z),
      cx: (b.min.x + b.max.x) / 2,
      minY: b.min.y,
      cz: (b.min.z + b.max.z) / 2,
    };
  });

  if (State.stageLayout === 'grid' && objs.length >= 3) {
    const cols = Math.ceil(Math.sqrt(objs.length));
    const cellX = Math.max(...sizes.map((s) => s.w)) + 1.4;
    const cellZ = Math.max(...sizes.map((s) => s.d)) + 1.4;
    const rows = Math.ceil(objs.length / cols);
    return objs.map((_, i) => {
      const c = i % cols;
      const r = Math.floor(i / cols);
      return [
        (c - (cols - 1) / 2) * cellX - sizes[i].cx,
        -sizes[i].minY,
        (r - (rows - 1) / 2) * cellZ - sizes[i].cz,
      ];
    });
  }

  // Clean horizontal lineup along X with generous spacing
  const gap = 1.25;
  const totalW = sizes.reduce((a, s) => a + s.w, 0) + gap * (objs.length - 1);
  let cursor = -totalW / 2;
  return sizes.map((s) => {
    const x = cursor + s.w / 2 - s.cx;
    cursor += s.w + gap;
    return [x, -s.minY, -s.cz];
  });
}

function BuildPrimitiveGeometry(p) {
  if (p.type === 'sphere') return new THREE.SphereGeometry(Math.max(0.05, p.radius), 24, 16);
  if (p.type === 'cylinder') return new THREE.CylinderGeometry(Math.max(0.05, p.radius), Math.max(0.05, p.radius), Math.max(0.05, p.height), 24);
  if (p.type === 'capsule') {
    const r = Math.max(0.05, p.radius);
    const len = Math.max(0.05, p.height - 2 * r);
    return new THREE.CapsuleGeometry(r, len, 8, 20);
  }
  return new THREE.BoxGeometry(Math.max(0.05, p.size[0]), Math.max(0.05, p.size[1]), Math.max(0.05, p.size[2]));
}

function AddColliderVisualForObject(obj, offset, isMulti) {
  const c = obj.collision;
  const grp = new THREE.Group();
  grp.position.set(offset[0], offset[1], offset[2]);

  const drawSolid = State.colDrawStyle !== 'wire';
  const drawWire  = State.colDrawStyle !== 'solid';

  const makeMat = (hex, opacity = 0.22) => new THREE.MeshStandardMaterial({
    color: hex,
    transparent: true,
    opacity,
    roughness: 0.45,
    metalness: 0.05,
    depthWrite: false,
    side: THREE.DoubleSide,
  });
  const makeWire = (geo, hex, opacity = 0.92) => new THREE.LineSegments(
    new THREE.EdgesGeometry(geo, 12),
    new THREE.LineBasicMaterial({ color: hex, transparent: true, opacity }),
  );

  if (c.mode === 'convex') {
    const hull = EnsureHull(obj);
    if (hull) {
      const hex = 0x46d267;
      if (drawSolid) grp.add(new THREE.Mesh(hull.geometry, makeMat(hex, 0.22)));
      if (drawWire)  grp.add(makeWire(hull.geometry, 0x67eb86, 0.95));
      if (State.colShowNormals) {
        const pos = hull.geometry.attributes.position;
        const nrm = hull.geometry.attributes.normal;
        const pts = [];
        for (let i = 0; i < pos.count; i += 3) {
          const mx = (pos.getX(i) + pos.getX(i + 1) + pos.getX(i + 2)) / 3;
          const my = (pos.getY(i) + pos.getY(i + 1) + pos.getY(i + 2)) / 3;
          const mz = (pos.getZ(i) + pos.getZ(i + 1) + pos.getZ(i + 2)) / 3;
          pts.push(mx, my, mz, mx + nrm.getX(i) * 0.22, my + nrm.getY(i) * 0.22, mz + nrm.getZ(i) * 0.22);
        }
        const ng = new THREE.BufferGeometry();
        ng.setAttribute('position', new THREE.Float32BufferAttribute(pts, 3));
        grp.add(new THREE.LineSegments(ng, new THREE.LineBasicMaterial({ color: 0xffb238 })));
      }
    }
  } else if (c.mode === 'box') {
    const pad = c.boxPadding * 2;
    const geo = new THREE.BoxGeometry(
      Math.max(0.05, c.boxSize[0] + pad),
      Math.max(0.05, c.boxSize[1] + pad),
      Math.max(0.05, c.boxSize[2] + pad),
    );
    const holder = new THREE.Group();
    holder.position.set(c.boxCenter[0], c.boxCenter[1], c.boxCenter[2]);
    if (drawSolid) holder.add(new THREE.Mesh(geo, makeMat(0x46d267, 0.2)));
    if (drawWire)  holder.add(makeWire(geo, 0x72f090, 0.96));
    grp.add(holder);
  } else if (c.mode === 'primitives') {
    for (const p of c.primitives) {
      if (!p.visible) continue;
      const geo = BuildPrimitiveGeometry(p);
      const isActive = !isMulti && p.id === c.selectedPrimId;
      const solidHex = isActive ? 0xffb238 : 0x46d267;
      const wireHex  = isActive ? 0xffcb6b : 0x6ee88c;
      const holder = new THREE.Group();
      holder.position.set(p.pos[0], p.pos[1], p.pos[2]);
      holder.rotation.set(
        THREE.MathUtils.degToRad(p.rot[0]),
        THREE.MathUtils.degToRad(p.rot[1]),
        THREE.MathUtils.degToRad(p.rot[2]),
      );
      if (drawSolid) holder.add(new THREE.Mesh(geo, makeMat(solidHex, isActive ? 0.26 : 0.2)));
      if (drawWire)  holder.add(makeWire(geo, wireHex, 0.95));
      grp.add(holder);
    }
  } else if (c.mode === 'mesh_faces') {
    let geo;
    if (c.meshSource !== 'base' && obj.asset.out.lod) {
      const lvl = c.meshSource === 'lod2' ? Math.min(2, obj.asset.out.lod.levels.length - 1) : Math.min(1, obj.asset.out.lod.levels.length - 1);
      geo = lodGeometry(obj.asset, lvl);
    } else {
      const g = obj.asset.out.geometry;
      geo = new THREE.BufferGeometry();
      geo.setAttribute('position', new THREE.BufferAttribute(g.positions, 3));
      geo.setIndex(new THREE.BufferAttribute(g.indices, 1));
      geo.computeVertexNormals();
    }
    if (drawSolid) grp.add(new THREE.Mesh(geo, makeMat(0x46d267, 0.22)));
    if (drawWire) {
      const wf = new THREE.Mesh(geo, new THREE.MeshBasicMaterial({ color: 0x6ff08f, wireframe: true, transparent: true, opacity: 0.45 }));
      grp.add(wf);
    }
  }

  colliderRoot.add(grp);
}

function RebuildScene(refit = false) {
  if (!stage) return;
  sceneRoot.clear();
  colliderRoot.clear();
  helperRoot.clear();

  const sel = SelectedObjects();
  const visibleSel = sel.filter((o) => o.visible);
  const offsets = ComputeStagingOffsets(visibleSel);
  const isMulti = visibleSel.length > 1;
  const inCollision = State.aspect === 'collision';

  const multiPill = $('MultiStagePill');
  if (isMulti && State.aspect !== 'fracture') {
    multiPill.classList.remove('hidden');
    $('MultiStageText').textContent = `${visibleSel.length} Objects Staged (${State.stageLayout === 'grid' ? 'Grid' : 'Lineup'})`;
  } else {
    multiPill.classList.add('hidden');
  }

  visibleSel.forEach((obj, idx) => {
    const off = offsets[idx];
    const showVisualMesh = !(inCollision && !State.colGhostMesh && State.colDrawStyle === 'solid');
    if (showVisualMesh) {
      const ch = inCollision && State.colGhostMesh ? 'collision' : (State.channel === 'bounds' ? 'lit' : State.channel);
      const meshGrp = meshObject(obj.asset, {
        channel: ch,
        lod: State.aspect === 'lod' ? obj.activeLod : -1,
        overrides: OverridesMap(obj),
      });
      meshGrp.position.set(off[0], off[1], off[2]);
      meshGrp.traverse((child) => {
        if (child.isMesh && child.material) {
          const mats = Array.isArray(child.material) ? child.material : [child.material];
          mats.forEach((m, mi) => {
            const slot = obj.slots[mi];
            if (slot && ch === 'lit') {
              m.roughness = slot.roughness;
              m.metalness = slot.metalness;
            }
            if (State.wireframe) m.wireframe = true;
          });
        }
      });
      sceneRoot.add(meshGrp);
    }

    // Collision visualizer whenever in Collision aspect
    if (inCollision) {
      AddColliderVisualForObject(obj, off, isMulti);
    }

    // Bounds wireframe when Bounds channel or Form & Bounds aspect
    if (State.channel === 'bounds' || State.aspect === 'bounds') {
      const b = boxOf(obj.asset.out.geometry);
      const box3 = new THREE.Box3(
        new THREE.Vector3(b.min.x + off[0], b.min.y + off[1], b.min.z + off[2]),
        new THREE.Vector3(b.max.x + off[0], b.max.y + off[1], b.max.z + off[2]),
      );
      helperRoot.add(new THREE.Box3Helper(box3, 0xffb238));
    }

    // Floor footprint selection ring when multiple objects are staged side-by-side
    if (isMulti) {
      const b = boxOf(obj.asset.out.geometry);
      const w = Math.max(0.7, b.max.x - b.min.x) + 0.35;
      const d = Math.max(0.7, b.max.z - b.min.z) + 0.35;
      const cx = (b.min.x + b.max.x) / 2 + off[0];
      const cz = (b.min.z + b.max.z) / 2 + off[2];
      const rect = new THREE.EdgesGeometry(new THREE.BoxGeometry(w, 0.01, d));
      const plate = new THREE.LineSegments(rect, new THREE.LineBasicMaterial({ color: 0xffb238, transparent: true, opacity: 0.65 }));
      plate.position.set(cx, 0.015, cz);
      helperRoot.add(plate);
    }

    // Pivot axes
    if (State.pivot) {
      const ax = new THREE.AxesHelper(1.1);
      ax.position.set(off[0], off[1] + 0.02, off[2]);
      helperRoot.add(ax);
    }
  });

  if (stage.grid) stage.grid.visible = State.grid;

  if (refit) FitCameraToSelection();
  UpdateSdfOverlay();
}

function FitCameraToSelection() {
  if (!stage) return;
  const box = new THREE.Box3();
  if (sceneRoot.children.length) box.expandByObject(sceneRoot);
  if (colliderRoot.children.length) box.expandByObject(colliderRoot);
  if (box.isEmpty()) {
    box.set(new THREE.Vector3(-2, 0, -2), new THREE.Vector3(2, 2, 2));
  }
  stage.rig.frame(box, { yaw: 38, pitch: 22, animate: false, fill: 1.15 });
}

function ApplyCameraPreset(view) {
  State.view = view;
  if (!stage) return;
  const box = new THREE.Box3();
  if (sceneRoot.children.length) box.expandByObject(sceneRoot);
  if (colliderRoot.children.length) box.expandByObject(colliderRoot);
  if (box.isEmpty()) box.set(new THREE.Vector3(-2, 0, -2), new THREE.Vector3(2, 2, 2));
  if (view === 'front') stage.rig.frame(box, { yaw: 0, pitch: 5, animate: false });
  else if (view === 'side') stage.rig.frame(box, { yaw: 90, pitch: 5, animate: false });
  else if (view === 'top') stage.rig.frame(box, { yaw: 0, pitch: 85, animate: false });
  else if (view === 'iso') stage.rig.frame(box, { yaw: 45, pitch: 35, animate: false });
  else stage.rig.frame(box, { yaw: 38, pitch: 22, animate: false });
}

// ============================================================================
// SDF 3D RAYMARCH OVERLAY
// ============================================================================
function UpdateSdfOverlay() {
  const ov = $('SdfOverlay');
  const sel = SelectedObjects();
  const primary = sel[0];
  const showSdf = State.aspect === 'sdf' && primary && primary.sdf.volume && State.sdfMode === 'raymarch';
  ov.classList.toggle('hidden', !showSdf);
  if (!showSdf) return;

  if (!sdfViewport) {
    sdfViewport = new SdfRaymarchViewport($('SdfGpuCanvas'), $('SdfHudCanvas'));
    sdfViewport.initialize();
  }
  sdfViewport.uploadVolume(primary.sdf.volume);
  sdfViewport.render({
    yaw: stage?.rig?.yaw ?? 0.68,
    pitch: stage?.rig?.pitch ?? 0.34,
    distance: 2.8,
    isoLevel: State.sdfIso,
    cutaway: State.sdfMode === 'cutaway',
    cutAxis: State.sdfAxis,
    cutPos: State.sdfSlicePos,
  }, primary.sdf.volume);
}

// ============================================================================
// OUTLINER RENDERING (1 Imported Asset = 1 Clean Row; Multi-Select Support!)
// ============================================================================
function RenderOutliner() {
  const list = $('OutlinerList');
  list.innerHTML = '';

  const q = State.searchQuery.trim().toLowerCase();
  const filtered = State.objects.filter((o) => !q || o.name.toLowerCase().includes(q) || o.format.toLowerCase().includes(q));
  const selSet = new Set(State.selectedIds);
  const selOrder = new Map(State.selectedIds.map((id, i) => [id, i + 1]));

  const colLabel = { convex: 'Hull', box: 'Box', primitives: 'Prims', mesh_faces: 'Faces' };

  for (const obj of filtered) {
    const isSel = selSet.has(obj.id);
    const tris = obj.asset.out.geometry.indices.length / 3;
    const row = El('div', `outliner-row${isSel ? ' selected' : ''}${obj.visible ? '' : ' hidden-obj'}`);
    row.dataset.objId = obj.id;
    row.setAttribute('role', 'option');
    row.setAttribute('aria-selected', String(isSel));

    const check = El('button', 'ol-check', isSel ? '✓' : '');
    check.type = 'button';
    check.title = 'Toggle in multi-selection';
    check.addEventListener('click', (e) => {
      e.stopPropagation();
      ToggleObjectSelection(obj.id);
    });

    const thumb = El('div', 'ol-thumb');
    if (obj.asset.out.icon) {
      const img = document.createElement('img');
      img.src = obj.asset.out.icon;
      img.alt = obj.name;
      thumb.appendChild(img);
    }

    const meta = El('div', 'ol-meta');
    const orderBadge = isSel && State.selectedIds.length > 1 ? `<span class="ol-order-badge">#${selOrder.get(obj.id)}</span>` : '';
    meta.innerHTML = `
      <div class="ol-name-row">
        <span class="ol-name">${obj.name}</span>
        ${orderBadge}
      </div>
      <div class="ol-sub">
        <span>${obj.format} · ${FmtK(tris)}</span>
        <span class="ol-col-tag">${colLabel[obj.collision.mode] || 'Hull'}</span>
      </div>
    `;

    const eye = El('button', `ol-eye${obj.visible ? '' : ' off'}`, obj.visible ? '◉' : '○');
    eye.type = 'button';
    eye.title = obj.visible ? 'Hide object' : 'Show object';
    eye.addEventListener('click', (e) => {
      e.stopPropagation();
      obj.visible = !obj.visible;
      RenderOutliner();
      RebuildScene(false);
    });

    row.append(check, thumb, meta, eye);
    row.addEventListener('click', (e) => HandleOutlinerRowClick(obj.id, e));
    list.appendChild(row);
  }

  // Update counts & pills across header / outliner / inspector
  const nSel = State.selectedIds.length;
  const selObjs = SelectedObjects();
  const totalTris = selObjs.reduce((acc, o) => acc + o.asset.out.geometry.indices.length / 3, 0);

  const badge = $('OutlinerSelBadge');
  badge.textContent = `${nSel} selected`;
  badge.classList.toggle('multi', nSel > 1);

  $('OutlinerFootSummary').textContent = nSel > 1
    ? `${State.objects.length} objects · ${nSel} sel · ${FmtK(totalTris)} tris`
    : `${State.objects.length} objects · ${FmtK(totalTris)} tris`;

  $('TopSelectionMeta').textContent = nSel === 1
    ? `${selObjs[0]?.name || 'None'} · ${FmtK(totalTris)} tris`
    : `${nSel} Objects Selected · ${FmtK(totalTris)} tris`;

  const inspBadge = $('InspectorScopeBadge');
  inspBadge.textContent = nSel === 1 ? '1 OBJECT' : `${nSel} OBJECTS`;
  inspBadge.classList.toggle('multi', nSel > 1);
}

function HandleOutlinerRowClick(id, e) {
  if (e.shiftKey && State.lastClickedId) {
    const ids = State.objects.map((o) => o.id);
    const a = ids.indexOf(State.lastClickedId);
    const b = ids.indexOf(id);
    if (a >= 0 && b >= 0) {
      const [lo, hi] = a < b ? [a, b] : [b, a];
      State.selectedIds = ids.slice(lo, hi + 1);
    }
  } else if (e.metaKey || e.ctrlKey) {
    ToggleObjectSelection(id);
    return;
  } else {
    State.selectedIds = [id];
    State.lastClickedId = id;
  }
  SyncAllUI(true);
}

function ToggleObjectSelection(id) {
  const idx = State.selectedIds.indexOf(id);
  if (idx >= 0) {
    if (State.selectedIds.length > 1) State.selectedIds.splice(idx, 1);
  } else {
    State.selectedIds.push(id);
    State.lastClickedId = id;
  }
  SyncAllUI(true);
}

// ============================================================================
// CONTEXTUAL VIEWPORT DEBUG STRIP
// ============================================================================
function RenderDebugStrip() {
  const strip = $('DebugStrip');
  strip.innerHTML = '';
  const sel = SelectedObjects();
  if (!sel.length || State.aspect === 'overview' || State.aspect === 'bounds' || State.aspect === 'portrait' || State.aspect === 'finish' || State.aspect === 'fracture') {
    strip.classList.add('hidden');
    return;
  }
  strip.classList.remove('hidden');

  if (State.aspect === 'collision') {
    strip.appendChild(El('span', 'debug-strip-label', 'COLLISION DEBUG'));

    // Mode switcher (applies to 1 or ALL selected!)
    const activeMode = sel.every((o) => o.collision.mode === sel[0].collision.mode) ? sel[0].collision.mode : null;
    const modeSeg = El('div', 'seg');
    [['convex', 'Convex Hull'], ['box', 'Bounding Box'], ['primitives', 'Primitives'], ['mesh_faces', 'Use Mesh Faces']].forEach(([m, label]) => {
      const b = El('button', activeMode === m ? 'on' : '', label);
      b.addEventListener('click', () => {
        sel.forEach((o) => { o.collision.mode = m; if (m === 'convex') EnsureHull(o); });
        SyncAllUI(false);
      });
      modeSeg.appendChild(b);
    });
    strip.appendChild(modeSeg);

    // Draw style
    const drawSeg = El('div', 'seg');
    [['both', 'Wire + Solid'], ['wire', 'Wire Only'], ['solid', 'Collider Only']].forEach(([d, label]) => {
      const b = El('button', State.colDrawStyle === d ? 'on' : '', label);
      b.addEventListener('click', () => { State.colDrawStyle = d; RenderDebugStrip(); RebuildScene(false); });
      drawSeg.appendChild(b);
    });
    strip.appendChild(drawSeg);

    const ghostBtn = El('button', `chip${State.colGhostMesh ? ' on' : ''}`, 'Ghost Mesh');
    ghostBtn.addEventListener('click', () => { State.colGhostMesh = !State.colGhostMesh; RenderDebugStrip(); RebuildScene(false); });
    strip.appendChild(ghostBtn);

    const nrmBtn = El('button', `chip${State.colShowNormals ? ' on' : ''}`, 'Hull Normals');
    nrmBtn.addEventListener('click', () => { State.colShowNormals = !State.colShowNormals; RenderDebugStrip(); RebuildScene(false); });
    strip.appendChild(nrmBtn);
    return;
  }

  if (State.aspect === 'sdf') {
    strip.appendChild(El('span', 'debug-strip-label', 'DISTANCE FIELD DEBUG'));
    const mSeg = El('div', 'seg');
    [['raymarch', 'Raymarch 3D'], ['slice', 'Slice Plane'], ['cutaway', 'Cutaway']].forEach(([m, lbl]) => {
      const b = El('button', State.sdfMode === m ? 'on' : '', lbl);
      b.addEventListener('click', () => { State.sdfMode = m; RenderDebugStrip(); UpdateSdfOverlay(); });
      mSeg.appendChild(b);
    });
    strip.appendChild(mSeg);

    const axSeg = El('div', 'seg');
    ['x', 'y', 'z'].forEach((ax) => {
      const b = El('button', State.sdfAxis === ax ? 'on' : '', ax.toUpperCase());
      b.addEventListener('click', () => { State.sdfAxis = ax; RenderDebugStrip(); RenderInspector(); UpdateSdfOverlay(); });
      axSeg.appendChild(b);
    });
    strip.appendChild(axSeg);

    const rng = El('label', 'debug-range', `Slice <input type="range" min="0" max="1" step="0.02" value="${State.sdfSlicePos}">`);
    rng.querySelector('input').addEventListener('input', (e) => { State.sdfSlicePos = +e.target.value; RenderInspector(); UpdateSdfOverlay(); });
    strip.appendChild(rng);
    return;
  }

  if (State.aspect === 'uv') {
    strip.appendChild(El('span', 'debug-strip-label', 'UV ATLAS DEBUG'));
    const b1 = El('button', `chip${State.channel === 'uv' ? ' on' : ''}`, 'Checker Overlay');
    b1.addEventListener('click', () => { State.channel = State.channel === 'uv' ? 'lit' : 'uv'; SyncStageBar(); RenderDebugStrip(); RebuildScene(false); });
    strip.appendChild(b1);
    return;
  }

  if (State.aspect === 'lod') {
    strip.appendChild(El('span', 'debug-strip-label', 'SURFACE STRATA DEBUG'));
    const b1 = El('button', `chip${State.channel === 'lod' ? ' on' : ''}`, 'Strata Colouring');
    b1.addEventListener('click', () => { State.channel = State.channel === 'lod' ? 'lit' : 'lod'; SyncStageBar(); RenderDebugStrip(); RebuildScene(false); });
    strip.appendChild(b1);
  }
}

// ============================================================================
// UI CONTROL BUILDERS (Authentic Component Viewer Studio.css Controls)
// ============================================================================
function AddSlider(parent, label, value, min, max, step, fmt, onInput) {
  const wrap = El('div', 'ctrl-row');
  const head = El('div', 'ctrl-row-head', `<span>${label}</span><strong>${fmt(value)}</strong>`);
  const input = document.createElement('input');
  input.type = 'range';
  input.min = min;
  input.max = max;
  input.step = step;
  input.value = value;
  input.addEventListener('input', () => {
    const v = Number(input.value);
    head.querySelector('strong').textContent = fmt(v);
    onInput(v);
  });
  wrap.append(head, input);
  parent.appendChild(wrap);
}

function AddToggle(parent, label, checked, onChange) {
  const row = El('label', 'toggle-line');
  row.innerHTML = `<span>${label}</span>`;
  const input = document.createElement('input');
  input.type = 'checkbox';
  input.className = 'switch';
  input.checked = !!checked;
  input.addEventListener('change', () => onChange(input.checked));
  row.appendChild(input);
  parent.appendChild(row);
}

function AddVec3(parent, label, vals, step, onChange) {
  const wrap = El('div', 'ctrl-row');
  wrap.innerHTML = `<div class="ctrl-row-head"><span>${label}</span></div>`;
  const grid = El('div', 'vec3');
  [['X', 'axis-x'], ['Y', 'axis-y'], ['Z', 'axis-z']].forEach(([ax, cls], i) => {
    const cell = El('div', `axis ${cls}`);
    cell.innerHTML = `<i>${ax}</i><input type="number" step="${step}" value="${Number(vals[i]).toFixed(2)}" />`;
    cell.querySelector('input').addEventListener('change', (e) => {
      vals[i] = Number(e.target.value) || 0;
      onChange([...vals]);
    });
    grid.appendChild(cell);
  });
  wrap.appendChild(grid);
  parent.appendChild(wrap);
}

// ============================================================================
// INSPECTOR ROUTER: 1 OBJECT SELECTED -> 1 ITEM | MANY SELECTED -> MANY ITEMS!
// ============================================================================
const ASPECT_TITLES = {
  overview: 'Overview',
  collision: 'Collision',
  sdf: 'Distance Field',
  uv: 'UV Atlas',
  lod: 'Surface Strata',
  bounds: 'Form & Bounds',
  portrait: 'Portrait',
  finish: 'Surface Finish',
  fracture: 'Fracture',
};

function RenderInspector() {
  const body = $('InspectorBody');
  body.innerHTML = '';
  $('InspectorHeading').textContent = ASPECT_TITLES[State.aspect] || 'Inspector';

  const blankOv = $('BlankStageOverlay');
  blankOv.classList.toggle('hidden', State.aspect !== 'fracture');

  // Stress Fracture is ALWAYS cleanly blank
  if (State.aspect === 'fracture') {
    body.appendChild(El('div', 'blank-insp-panel', `
      <div>
        <strong>Blank Workspace</strong>
        <p>Stress Fracture is intentionally left blank here so it can be authored cleanly later.</p>
      </div>
    `));
    return;
  }

  const sel = SelectedObjects();
  if (!sel.length) {
    body.appendChild(El('div', 'blank-insp-panel', `<div><strong>No Object Selected</strong><p>Select 1 or more objects in the Outliner.</p></div>`));
    return;
  }

  // ROUTE: 1 object selected => 1 item in Inspector; Many objects selected => Many objects in Inspector!
  if (sel.length === 1) {
    RenderSingleObjectInspector(body, sel[0]);
  } else {
    RenderMultiObjectInspector(body, sel);
  }
}

// ----------------------------------------------------------------------------
// A. SINGLE-OBJECT INSPECTOR (1 object selected -> 1 item in Inspector)
// ----------------------------------------------------------------------------
function RenderSingleObjectInspector(body, obj) {
  const b = boxOf(obj.asset.out.geometry);
  const sz = [b.max.x - b.min.x, b.max.y - b.min.y, b.max.z - b.min.z];
  const tris = obj.asset.out.geometry.indices.length / 3;

  // Crisp single-object header
  const hero = El('div', 'insp-hero');
  hero.innerHTML = `
    <div class="insp-hero-thumb">${obj.asset.out.icon ? `<img src="${obj.asset.out.icon}" alt="${obj.name}" />` : ''}</div>
    <div class="insp-hero-meta">
      <span class="eyebrow">${obj.format} · ${ASPECT_TITLES[State.aspect]}</span>
      <strong>${obj.name}</strong>
      <small>${tris.toLocaleString()} tris · ${sz.map((v) => v.toFixed(2)).join(' × ')} m</small>
    </div>
  `;
  body.appendChild(hero);

  if (State.aspect === 'overview') RenderOverviewSingle(body, obj, sz, tris);
  else if (State.aspect === 'collision') RenderCollisionSingle(body, obj);
  else if (State.aspect === 'sdf') RenderSdfSingle(body, obj);
  else if (State.aspect === 'uv') RenderUvSingle(body, obj);
  else if (State.aspect === 'lod') RenderLodSingle(body, obj);
  else if (State.aspect === 'bounds') RenderBoundsSingle(body, obj, sz);
  else if (State.aspect === 'portrait') RenderPortraitSingle(body, obj);
  else if (State.aspect === 'finish') RenderFinishSingle(body, obj);
}

function RenderOverviewSingle(body, obj, sz, tris) {
  const maxSpan = Math.max(...sz);
  const kpi = El('div', 'kpi-grid', `
    <div class="kpi-box"><strong>${FmtK(tris)}</strong><span>Triangles</span></div>
    <div class="kpi-box"><strong>${maxSpan.toFixed(2)} m</strong><span>Max Span</span></div>
  `);
  body.appendChild(kpi);

  const colNames = { convex: 'Convex Hull', box: 'Bounding Box', primitives: `Primitives (${obj.collision.primitives.length})`, mesh_faces: 'Use Mesh Faces' };
  const info = El('div', 'card', `
    <div class="kv-row"><span>Dimensions</span><span>${sz.map((v) => v.toFixed(2)).join(' × ')} m</span></div>
    <div class="kv-row"><span>Active Collision</span><span>${colNames[obj.collision.mode]}</span></div>
    <div class="kv-row"><span>Material Slots</span><span>${obj.slots.length} slot${obj.slots.length === 1 ? '' : 's'}</span></div>
    <div class="kv-row"><span>Source Format</span><span>${obj.format} (1 unified object)</span></div>
  `);
  body.appendChild(info);

  body.appendChild(El('div', 'sec-label', '<span>Studio Workspaces</span><span>Click any plate</span>'));
  const grid = El('div', 'plates-grid');
  const plates = [
    {
      id: 'collision', title: 'Collision Editor', foot: colNames[obj.collision.mode], dot: 'ready',
      vis: `<svg viewBox="0 0 120 48"><polygon points="32,39 22,22 39,9 82,11 98,25 84,39" fill="rgba(70,210,103,0.14)" stroke="#46d267" stroke-width="1.5"/></svg>`,
    },
    {
      id: 'sdf', title: 'Distance Field', foot: obj.sdf.volume ? `${obj.sdf.resolution}³ baked` : 'Unbaked', dot: obj.sdf.volume ? 'ready' : '',
      vis: `<svg viewBox="0 0 120 48"><ellipse cx="60" cy="24" rx="34" ry="15" fill="none" stroke="rgba(255,178,56,0.35)"/><ellipse cx="60" cy="24" rx="22" ry="9" fill="none" stroke="rgba(255,178,56,0.65)"/><ellipse cx="60" cy="24" rx="10" ry="4" fill="none" stroke="#ffb238"/></svg>`,
    },
    {
      id: 'uv', title: 'UV Atlas', foot: obj.uvBaked ? 'Packed' : 'Unbaked', dot: obj.uvBaked ? 'ready' : '',
      vis: `<svg viewBox="0 0 120 48"><rect x="18" y="8" width="38" height="32" rx="3" fill="rgba(77,155,255,0.12)" stroke="#4d9bff"/><rect x="61" y="8" width="40" height="14" rx="3" fill="rgba(77,155,255,0.08)" stroke="#4d9bff"/><rect x="61" y="26" width="26" height="14" rx="3" fill="rgba(77,155,255,0.08)" stroke="#4d9bff"/></svg>`,
    },
    {
      id: 'lod', title: 'Surface Strata', foot: obj.lodBaked ? '4 tiers' : 'Unbaked', dot: obj.lodBaked ? 'ready' : '',
      vis: `<svg viewBox="0 0 120 48"><rect x="24" y="10" width="14" height="30" rx="2" fill="rgba(255,178,56,0.2)" stroke="#ffb238"/><rect x="44" y="18" width="14" height="22" rx="2" fill="#1b1b20" stroke="#555"/><rect x="64" y="25" width="14" height="15" rx="2" fill="#1b1b20" stroke="#444"/><rect x="84" y="31" width="14" height="9" rx="2" fill="#1b1b20" stroke="#3a3a40"/></svg>`,
    },
    {
      id: 'bounds', title: 'Form & Bounds', foot: `${maxSpan.toFixed(1)} m span`, dot: 'ready',
      vis: `<svg viewBox="0 0 120 48"><rect x="26" y="11" width="68" height="26" rx="3" fill="none" stroke="#4d9bff" stroke-dasharray="3 2"/><rect x="38" y="16" width="44" height="16" rx="2" fill="rgba(255,178,56,0.12)" stroke="#ffb238"/></svg>`,
    },
    {
      id: 'portrait', title: 'Portrait', foot: `${State.portraitRes} px`, dot: 'ready',
      vis: obj.asset.out.icon ? `<img src="${obj.asset.out.icon}" alt="Portrait" />` : '',
    },
    {
      id: 'finish', title: 'Surface Finish', foot: `${obj.slots.length} slot${obj.slots.length === 1 ? '' : 's'}`, dot: 'ready',
      vis: `<svg viewBox="0 0 120 48">${obj.slots.slice(0, 4).map((s, i) => `<circle cx="${40 + i * 14}" cy="24" r="10" fill="${s.color}" stroke="#121212" stroke-width="1.5"/>`).join('')}</svg>`,
    },
    {
      id: 'fracture', title: 'Stress Fracture', foot: 'Blank', dot: 'blank',
      vis: `<svg viewBox="0 0 120 48"><rect x="26" y="12" width="68" height="24" rx="5" fill="none" stroke="#3c3c44" stroke-dasharray="3 3"/></svg>`,
    },
  ];
  for (const p of plates) {
    const card = El('button', 'plate-card');
    card.type = 'button';
    card.innerHTML = `
      <div class="plate-card-head"><span>${p.title}</span><span>↗</span></div>
      <div class="plate-vis">${p.vis}</div>
      <div class="plate-foot"><span>${p.foot}</span><i class="adot ${p.dot}"></i></div>
    `;
    card.addEventListener('click', () => SetAspect(p.id));
    grid.appendChild(card);
  }
  body.appendChild(grid);
}

// ----------------------------------------------------------------------------
// COLLISION EDITOR — SINGLE OBJECT (Full 4 Modes + Physics + Export)
// ----------------------------------------------------------------------------
function RenderCollisionSingle(body, obj) {
  const c = obj.collision;
  body.appendChild(El('div', 'sec-label', `<span>Collider Representation</span><span>${c.mode.replace('_', ' ')}</span>`));

  const modeGrid = El('div', 'col-mode-grid');
  const modes = [
    ['convex', 'Convex Hull', 'Tight auto-fitted 3D hull'],
    ['box', 'Bounding Box', 'Fast AABB box with editable extents'],
    ['primitives', 'Primitives', 'Compound Boxes, Spheres, Capsules & Cyls'],
    ['mesh_faces', 'Use Mesh Faces', 'Exact triangle-mesh collider'],
  ];
  for (const [id, title, desc] of modes) {
    const btn = El('button', `col-mode-card${c.mode === id ? ' on' : ''}`);
    btn.type = 'button';
    btn.dataset.colMode = id;
    btn.innerHTML = `<strong><span>${title}</span><i class="col-dot"></i></strong><small>${desc}</small>`;
    btn.addEventListener('click', () => {
      c.mode = id;
      if (id === 'convex') EnsureHull(obj);
      SyncAllUI(false);
    });
    modeGrid.appendChild(btn);
  }
  body.appendChild(modeGrid);

  // Mode-specific controls card
  const modeCard = El('div', 'card');

  if (c.mode === 'convex') {
    const hull = EnsureHull(obj);
    modeCard.innerHTML = `
      <div class="kv-row"><span>Hull Vertices · Faces</span><span>${hull ? `${hull.vertCount} verts · ${hull.triCount} tris` : '—'}</span></div>
      <div class="kv-row"><span>Enclosed Volume</span><span>${hull ? `${hull.volume.toFixed(2)} m³` : '—'}</span></div>
    `;
    const vWrap = El('div', 'ctrl-row');
    vWrap.innerHTML = `<div class="ctrl-row-head"><span>Max Hull Vertices</span><strong>${c.hullMaxVerts}</strong></div>`;
    const vSeg = El('div', 'seg-pills');
    [16, 32, 48, 64, 128].forEach((n) => {
      const b = El('button', c.hullMaxVerts === n ? 'on' : '', String(n));
      b.addEventListener('click', () => { c.hullMaxVerts = n; c.hullCache = null; EnsureHull(obj); RenderInspector(); RebuildScene(false); });
      vSeg.appendChild(b);
    });
    vWrap.appendChild(vSeg);
    modeCard.appendChild(vWrap);

    AddSlider(modeCard, 'Hull Inflation / Margin', c.hullInflation * 1000, 0, 50, 1, (v) => `+${v.toFixed(0)} mm`, (v) => {
      c.hullInflation = v / 1000;
      c.hullCache = null;
      EnsureHull(obj);
      RebuildScene(false);
    });

    const rebuildBtn = El('button', 'btn-primary', 'Rebuild 3D Convex Hull');
    rebuildBtn.addEventListener('click', () => { c.hullCache = null; EnsureHull(obj); RenderInspector(); RebuildScene(false); Toast(`Rebuilt hull for ${obj.name}`); });
    modeCard.appendChild(rebuildBtn);
  } else if (c.mode === 'box') {
    const vol = (c.boxSize[0] + c.boxPadding * 2) * (c.boxSize[1] + c.boxPadding * 2) * (c.boxSize[2] + c.boxPadding * 2);
    modeCard.innerHTML = `
      <div class="kv-row"><span>Box Dimensions</span><span>${c.boxSize.map((v) => (v + c.boxPadding * 2).toFixed(2)).join(' × ')} m</span></div>
      <div class="kv-row"><span>Collider Volume</span><span>${vol.toFixed(2)} m³</span></div>
    `;
    AddVec3(modeCard, 'Box Size W / H / D (m)', c.boxSize, 0.05, (v) => { c.boxSize = v; RebuildScene(false); });
    AddVec3(modeCard, 'Center Offset (m)', c.boxCenter, 0.05, (v) => { c.boxCenter = v; RebuildScene(false); });
    AddSlider(modeCard, 'Contact Padding Margin', c.boxPadding * 1000, 0, 80, 1, (v) => `${v.toFixed(0)} mm`, (v) => { c.boxPadding = v / 1000; RebuildScene(false); });

    const fitBtn = El('button', 'btn-primary', 'Auto-Fit Tight Bounding Box');
    fitBtn.addEventListener('click', () => {
      const b = boxOf(obj.asset.out.geometry);
      c.boxSize = [+(b.max.x - b.min.x).toFixed(3), +(b.max.y - b.min.y).toFixed(3), +(b.max.z - b.min.z).toFixed(3)];
      c.boxCenter = [+((b.min.x + b.max.x) / 2).toFixed(3), +((b.min.y + b.max.y) / 2).toFixed(3), +((b.min.z + b.max.z) / 2).toFixed(3)];
      c.boxPadding = 0;
      RenderInspector(); RebuildScene(false);
    });
    modeCard.appendChild(fitBtn);
  } else if (c.mode === 'primitives') {
    const headRow = El('div', 'ctrl-row-head', `<span>Compound Shapes</span><strong>${c.primitives.length} shape${c.primitives.length === 1 ? '' : 's'}</strong>`);
    modeCard.appendChild(headRow);

    const addRow = El('div', 'btn-row-4');
    [['box', '+ Box'], ['sphere', '+ Sphere'], ['capsule', '+ Capsule'], ['cylinder', '+ Cyl']].forEach(([t, lbl]) => {
      const b = El('button', 'mini-act-btn', lbl);
      b.addEventListener('click', () => {
        const bnd = boxOf(obj.asset.out.geometry);
        const np = {
          id: `PRIM-${NextPrimNum++}`,
          type: t,
          name: `${t[0].toUpperCase() + t.slice(1)} ${c.primitives.length + 1}`,
          pos: [0, +((bnd.min.y + bnd.max.y) / 2).toFixed(2), 0],
          size: [1.0, 1.0, 1.0],
          rot: [0, 0, 0],
          radius: 0.5,
          height: 1.0,
          visible: true,
        };
        c.primitives.push(np);
        c.selectedPrimId = np.id;
        RenderInspector(); RebuildScene(false);
      });
      addRow.appendChild(b);
    });
    modeCard.appendChild(addRow);

    const utilRow = El('div', 'btn-row-2');
    const autoBtn = El('button', 'mini-act-btn', 'Auto-Fit Object');
    autoBtn.addEventListener('click', () => {
      c.primitives = AutoFitPrimitives(obj.asset);
      c.selectedPrimId = c.primitives[0]?.id || null;
      RenderInspector(); RebuildScene(false);
    });
    const clrBtn = El('button', 'mini-act-btn', 'Clear All');
    clrBtn.addEventListener('click', () => { c.primitives = []; c.selectedPrimId = null; RenderInspector(); RebuildScene(false); });
    utilRow.append(autoBtn, clrBtn);
    modeCard.appendChild(utilRow);

    const list = El('div', 'prim-list');
    const primTag = { box: 'BOX', sphere: 'SPH', capsule: 'CAP', cylinder: 'CYL' };
    for (const p of c.primitives) {
      const row = El('div', `prim-row${p.id === c.selectedPrimId ? ' on' : ''}`);
      row.innerHTML = `<span class="prim-type-pill">${primTag[p.type] || 'BOX'}</span><span class="prim-name">${p.name}</span>`;
      const dup = El('button', 'prim-mini-btn', '+');
      dup.title = 'Duplicate shape';
      dup.addEventListener('click', (e) => {
        e.stopPropagation();
        const cp = { ...structuredClone(p), id: `PRIM-${NextPrimNum++}`, name: `${p.name} Copy`, pos: [p.pos[0] + 0.3, p.pos[1], p.pos[2]] };
        c.primitives.push(cp);
        c.selectedPrimId = cp.id;
        RenderInspector(); RebuildScene(false);
      });
      const del = El('button', 'prim-mini-btn', '×');
      del.title = 'Remove shape';
      del.addEventListener('click', (e) => {
        e.stopPropagation();
        c.primitives = c.primitives.filter((x) => x.id !== p.id);
        if (c.selectedPrimId === p.id) c.selectedPrimId = c.primitives[0]?.id || null;
        RenderInspector(); RebuildScene(false);
      });
      row.append(dup, del);
      row.addEventListener('click', () => { c.selectedPrimId = p.id; RenderInspector(); RebuildScene(false); });
      list.appendChild(row);
    }
    modeCard.appendChild(list);

    const activePrim = c.primitives.find((p) => p.id === c.selectedPrimId);
    if (activePrim) {
      const typeSeg = El('div', 'seg-pills');
      [['box', 'Box'], ['sphere', 'Sphere'], ['capsule', 'Capsule'], ['cylinder', 'Cylinder']].forEach(([t, lbl]) => {
        const b = El('button', activePrim.type === t ? 'on' : '', lbl);
        b.addEventListener('click', () => { activePrim.type = t; RenderInspector(); RebuildScene(false); });
        typeSeg.appendChild(b);
      });
      modeCard.appendChild(typeSeg);
      AddVec3(modeCard, 'Center Position (m)', activePrim.pos, 0.05, (v) => { activePrim.pos = v; RebuildScene(false); });
      if (activePrim.type === 'box') {
        AddVec3(modeCard, 'Box Size W / H / D (m)', activePrim.size, 0.05, (v) => { activePrim.size = v; RebuildScene(false); });
      } else {
        AddSlider(modeCard, 'Radius', activePrim.radius, 0.05, 3.0, 0.02, (v) => `${v.toFixed(2)} m`, (v) => { activePrim.radius = v; RebuildScene(false); });
        if (activePrim.type !== 'sphere') {
          AddSlider(modeCard, 'Height / Length', activePrim.height, 0.1, 4.0, 0.02, (v) => `${v.toFixed(2)} m`, (v) => { activePrim.height = v; RebuildScene(false); });
        }
      }
      AddVec3(modeCard, 'Rotation (°)', activePrim.rot, 5, (v) => { activePrim.rot = v; RebuildScene(false); });
    }
  } else if (c.mode === 'mesh_faces') {
    const tris = obj.asset.out.geometry.indices.length / 3;
    const bvhKb = Math.round((tris * 36) / 1024);
    modeCard.innerHTML = `
      <div class="kv-row"><span>Collider Triangles</span><span>${tris.toLocaleString()} exact faces</span></div>
      <div class="kv-row"><span>BVH Memory Estimate</span><span>${bvhKb} KB</span></div>
      <div class="kv-row"><span>Collision Type</span><span>Complex Triangle Mesh</span></div>
    `;
    const srcWrap = El('div', 'ctrl-row');
    srcWrap.innerHTML = `<div class="ctrl-row-head"><span>Source Face Stream</span><strong>${c.meshSource.toUpperCase()}</strong></div>`;
    const srcSeg = El('div', 'seg-pills');
    [['base', 'Base Mesh'], ['lod1', 'LOD 1'], ['lod2', 'LOD 2']].forEach(([s, lbl]) => {
      const b = El('button', c.meshSource === s ? 'on' : '', lbl);
      b.addEventListener('click', () => { c.meshSource = s; RenderInspector(); RebuildScene(false); });
      srcSeg.appendChild(b);
    });
    srcWrap.appendChild(srcSeg);
    modeCard.appendChild(srcWrap);
    AddToggle(modeCard, 'Double-Sided Face Collisions', c.meshDoubleSided, (v) => { c.meshDoubleSided = v; });
    AddToggle(modeCard, 'Weld Degenerate Triangles', c.meshWeld, (v) => { c.meshWeld = v; });
    AddSlider(modeCard, 'Surface Contact Margin', c.meshMargin * 1000, 0, 25, 1, (v) => `${v.toFixed(0)} mm`, (v) => { c.meshMargin = v / 1000; });
  }
  body.appendChild(modeCard);

  // Physics Surface & Export Card
  body.appendChild(El('div', 'sec-label', `<span>Physics Surface &amp; Export</span><span>${c.preset}</span>`));
  const physCard = El('div', 'card');
  const pSeg = El('div', 'seg-pills');
  [['solid', 'Solid'], ['static', 'Static'], ['vehicle', 'Vehicle'], ['trigger', 'Trigger']].forEach(([p, lbl]) => {
    const b = El('button', c.preset === p ? 'on' : '', lbl);
    b.addEventListener('click', () => { c.preset = p; RenderInspector(); });
    pSeg.appendChild(b);
  });
  physCard.appendChild(pSeg);
  AddSlider(physCard, 'Static Friction', c.friction, 0, 1.5, 0.05, (v) => v.toFixed(2), (v) => { c.friction = v; });
  AddSlider(physCard, 'Restitution (Bounce)', c.restitution, 0, 1, 0.05, (v) => v.toFixed(2), (v) => { c.restitution = v; });
  AddSlider(physCard, 'Material Density', c.density, 100, 8000, 50, (v) => `${v.toFixed(0)} kg/m³`, (v) => { c.density = v; });
  const expBtn = El('button', 'btn-secondary', 'Export Collision Descriptor (.json)');
  expBtn.addEventListener('click', () => DownloadCollisionJson([obj]));
  physCard.appendChild(expBtn);
  body.appendChild(physCard);
}

function DownloadCollisionJson(objs) {
  const payload = objs.map((o) => ({
    object: o.name,
    mode: o.collision.mode,
    preset: o.collision.preset,
    friction: o.collision.friction,
    restitution: o.collision.restitution,
    density: o.collision.density,
    boxSize: o.collision.boxSize,
    boxCenter: o.collision.boxCenter,
    primitives: o.collision.primitives,
    hullMaxVerts: o.collision.hullMaxVerts,
  }));
  const a = document.createElement('a');
  a.href = URL.createObjectURL(new Blob([JSON.stringify(payload.length === 1 ? payload[0] : payload, null, 2)], { type: 'application/json' }));
  a.download = objs.length === 1 ? `${objs[0].name}_collision.json` : `frontier_batch_collision_${objs.length}.json`;
  a.click();
  Toast(`Exported collision JSON for ${objs.length} object${objs.length > 1 ? 's' : ''}`);
}

// ----------------------------------------------------------------------------
// OTHER SINGLE-OBJECT ASPECTS (SDF, UV, LOD, Bounds, Portrait, Finish)
// ----------------------------------------------------------------------------
function RenderSdfSingle(body, obj) {
  const s = obj.sdf;
  const card = El('div', 'card');
  const rWrap = El('div', 'ctrl-row');
  rWrap.innerHTML = `<div class="ctrl-row-head"><span>Voxel Resolution</span><strong>${s.resolution}³</strong></div>`;
  const rSeg = El('div', 'seg-pills');
  [32, 64, 96].forEach((r) => {
    const b = El('button', s.resolution === r ? 'on' : '', `${r}³`);
    b.addEventListener('click', () => { s.resolution = r; RenderInspector(); });
    rSeg.appendChild(b);
  });
  rWrap.appendChild(rSeg);
  card.appendChild(rWrap);
  AddSlider(card, 'Narrow-Band Width', s.band, 1, 6, 1, (v) => `${v} voxels`, (v) => { s.band = v; });
  AddSlider(card, 'Iso-Surface Offset', State.sdfIso, -0.1, 0.1, 0.005, (v) => `${(v * 1000).toFixed(0)} mm`, (v) => { State.sdfIso = v; UpdateSdfOverlay(); });

  const bakeBtn = El('button', 'btn-primary', s.volume ? 'Re-Bake Sparse 8×8×8 SDF' : 'Bake Sparse 8×8×8 SDF');
  bakeBtn.addEventListener('click', () => BakeSdfForObject(obj));
  card.appendChild(bakeBtn);
  body.appendChild(card);

  if (s.volume) {
    const stats = El('div', 'card', `
      <div class="kv-row"><span>Active Bricks</span><span>${s.volume.brickCount} bricks (8³)</span></div>
      <div class="kv-row"><span>Grid Resolution</span><span>${s.resolution} × ${s.resolution} × ${s.resolution}</span></div>
    `);
    const sliceBox = El('div', 'insp-canvas-box');
    const cnv = document.createElement('canvas');
    cnv.width = s.resolution;
    cnv.height = s.resolution;
    const ctx = cnv.getContext('2d');
    const rgba = extractSdfSliceRgba(s.volume, State.sdfAxis, State.sdfSlicePos);
    ctx.putImageData(new ImageData(rgba, s.resolution, s.resolution), 0, 0);
    sliceBox.appendChild(cnv);
    stats.appendChild(sliceBox);
    const dl = El('button', 'btn-secondary', 'Export Sparse SDF (.fvol)');
    dl.addEventListener('click', () => {
      const bin = serializeVolumeFvol(s.volume);
      const a = document.createElement('a');
      a.href = URL.createObjectURL(new Blob([bin], { type: 'application/octet-stream' }));
      a.download = `${obj.name}.fvol`;
      a.click();
    });
    stats.appendChild(dl);
    body.appendChild(stats);
  }
}

function BakeSdfForObject(obj) {
  const g = obj.asset.out.geometry;
  obj.sdf.volume = compileSparseSdfVolume(g.positions, g.indices, { resolution: obj.sdf.resolution, narrowBandVoxels: obj.sdf.band });
  RenderInspector();
  UpdateSdfOverlay();
  Toast(`Baked ${obj.sdf.resolution}³ SDF for ${obj.name}`);
}

function RenderUvSingle(body, obj) {
  const card = El('div', 'card');
  const bakeBtn = El('button', 'btn-primary', obj.uvBaked ? 'Re-Pack UV Atlas' : 'Unwrap & Pack UV Atlas');
  bakeBtn.addEventListener('click', async () => {
    await runStep(obj.asset, 'uv', compileCtx);
    obj.uvBaked = true;
    State.channel = 'uv';
    SyncAllUI(false);
    Toast(`UV Atlas packed for ${obj.name}`);
  });
  card.appendChild(bakeBtn);

  if (obj.asset.out.uv) {
    const uv = obj.asset.out.uv;
    card.appendChild(El('div', 'kv-row', `<span>Charts</span><span>${uv.layout?.charts?.length ?? 1} charts</span>`));
    card.appendChild(El('div', 'kv-row', `<span>Efficiency</span><span>${Math.round((uv.layout?.efficiency ?? 0.78) * 100)}%</span>`));
    const exp = El('button', 'btn-secondary', 'Export UV Map (.svg)');
    exp.addEventListener('click', () => {
      const svg = formatUvSvg(uv.layout, uv.uvs, obj.name);
      const a = document.createElement('a');
      a.href = URL.createObjectURL(new Blob([svg], { type: 'image/svg+xml' }));
      a.download = `${obj.name}_uv.svg`;
      a.click();
    });
    card.appendChild(exp);
  }
  body.appendChild(card);
}

function RenderLodSingle(body, obj) {
  const card = El('div', 'card');
  const bakeBtn = El('button', 'btn-primary', obj.lodBaked ? 'Rebuild Surface Strata (LODs)' : 'Compile Surface Strata (LOD Chain)');
  bakeBtn.addEventListener('click', async () => {
    await runStep(obj.asset, 'lod', compileCtx);
    obj.lodBaked = true;
    obj.activeLod = 0;
    SyncAllUI(false);
    Toast(`Surface Strata compiled for ${obj.name}`);
  });
  card.appendChild(bakeBtn);

  if (obj.asset.out.lod) {
    const seg = El('div', 'seg-pills');
    const levels = obj.asset.out.lod.levels || [];
    levels.forEach((lv, i) => {
      const b = El('button', obj.activeLod === i ? 'on' : '', `LOD ${i}`);
      b.addEventListener('click', () => { obj.activeLod = i; RenderInspector(); RebuildScene(false); });
      seg.appendChild(b);
    });
    card.appendChild(seg);
    levels.forEach((lv, i) => {
      card.appendChild(El('div', 'kv-row', `<span>LOD ${i}</span><span>${FmtK((lv.indices?.length || 0) / 3)} tris</span>`));
    });
  }
  body.appendChild(card);
}

function RenderBoundsSingle(body, obj, sz) {
  const card = El('div', 'card', `
    <div class="kv-row"><span>Width (X)</span><span>${sz[0].toFixed(3)} m</span></div>
    <div class="kv-row"><span>Height (Y)</span><span>${sz[1].toFixed(3)} m</span></div>
    <div class="kv-row"><span>Depth (Z)</span><span>${sz[2].toFixed(3)} m</span></div>
    <div class="kv-row"><span>Vertices</span><span>${(obj.asset.out.geometry.positions.length / 3).toLocaleString()}</span></div>
  `);
  const exp = El('button', 'btn-primary', 'Export Clean Mesh (.obj)');
  exp.addEventListener('click', () => {
    const g = obj.asset.out.geometry;
    const txt = formatObj(g.positions, g.indices, obj.asset.out.uv?.uvs || null, obj.name);
    const a = document.createElement('a');
    a.href = URL.createObjectURL(new Blob([txt], { type: 'text/plain' }));
    a.download = `${obj.name}.obj`;
    a.click();
    Toast(`Exported ${obj.name}.obj`);
  });
  card.appendChild(exp);
  body.appendChild(card);
}

function RenderPortraitSingle(body, obj) {
  const card = El('div', 'card');
  const box = El('div', 'insp-canvas-box');
  if (obj.asset.out.icon) box.innerHTML = `<img src="${obj.asset.out.icon}" alt="Portrait" />`;
  card.appendChild(box);
  const reBtn = El('button', 'btn-primary', 'Re-Render Studio Portrait');
  reBtn.addEventListener('click', () => {
    obj.asset.out.icon = RenderIconForAsset(obj.asset, OverridesMap(obj), State.portraitRes);
    RenderOutliner(); RenderInspector(); Toast(`Portrait refreshed for ${obj.name}`);
  });
  const dlBtn = El('button', 'btn-secondary', 'Download Portrait (.png)');
  dlBtn.addEventListener('click', () => {
    if (!obj.asset.out.icon) return;
    const a = document.createElement('a');
    a.href = obj.asset.out.icon;
    a.download = `${obj.name}_portrait.png`;
    a.click();
  });
  card.append(reBtn, dlBtn);
  body.appendChild(card);
}

function RenderFinishSingle(body, obj) {
  body.appendChild(El('div', 'sec-label', `<span>Material Slots</span><span>${obj.slots.length} slots</span>`));
  for (const sl of obj.slots) {
    const sc = El('div', 'slot-card');
    const head = El('div', 'slot-card-head');
    const inp = document.createElement('input');
    inp.type = 'color';
    inp.className = 'slot-swatch';
    inp.value = sl.color;
    const hexLbl = El('span', '', sl.color);
    inp.addEventListener('input', () => {
      sl.color = inp.value;
      hexLbl.textContent = inp.value;
      RebuildScene(false);
    });
    head.append(inp, El('strong', '', sl.name), hexLbl);
    sc.appendChild(head);
    AddSlider(sc, 'Roughness', sl.roughness, 0.05, 1, 0.02, (v) => v.toFixed(2), (v) => { sl.roughness = v; RebuildScene(false); });
    AddSlider(sc, 'Metallic', sl.metalness, 0, 1, 0.02, (v) => v.toFixed(2), (v) => { sl.metalness = v; RebuildScene(false); });
    body.appendChild(sc);
  }
}

// ----------------------------------------------------------------------------
// B. MULTI-OBJECT INSPECTOR (Many Objects Selected -> Many Objects in Inspector!)
// ----------------------------------------------------------------------------
function RenderMultiObjectInspector(body, objs) {
  const totalTris = objs.reduce((a, o) => a + o.asset.out.geometry.indices.length / 3, 0);

  // 1. Multi-Selection Shared Header + Quick Jump Chips
  const banner = El('div', 'multi-banner');
  banner.innerHTML = `
    <div class="multi-banner-top">
      <strong>${objs.length} Objects Selected</strong>
      <span>${FmtK(totalTris)} total tris</span>
    </div>
    <div class="multi-banner-sub">Editing ${objs.length} selected Outliner objects together. Adjust batch settings below or expand each object card individually.</div>
  `;
  const chipStrip = El('div', 'multi-chip-strip');
  objs.forEach((o, i) => {
    const ch = El('button', 'multi-obj-chip', `<i>#${i + 1}</i>${o.name}`);
    ch.title = `Solo-focus ${o.name}`;
    ch.addEventListener('click', () => { State.selectedIds = [o.id]; SyncAllUI(true); });
    chipStrip.appendChild(ch);
  });
  banner.appendChild(chipStrip);

  // Stage Layout Switcher (Lineup | Grid)
  const laySeg = El('div', 'seg-pills');
  [['lineup', 'Stage Lineup'], ['grid', 'Stage Grid']].forEach(([l, lbl]) => {
    const b = El('button', State.stageLayout === l ? 'on' : '', lbl);
    b.addEventListener('click', () => { State.stageLayout = l; RenderInspector(); RebuildScene(true); });
    laySeg.appendChild(b);
  });
  banner.appendChild(laySeg);
  body.appendChild(banner);

  // 2. Shared Batch Controls for the Active Aspect
  if (State.aspect === 'collision') {
    body.appendChild(El('div', 'sec-label', `<span>Batch Collision (All ${objs.length})</span><span>Apply to all</span>`));
    const batchCard = El('div', 'card');
    const sharedMode = objs.every((o) => o.collision.mode === objs[0].collision.mode) ? objs[0].collision.mode : null;
    const mSeg = El('div', 'seg-pills');
    [['convex', 'Hull'], ['box', 'Box'], ['primitives', 'Prims'], ['mesh_faces', 'Mesh']].forEach(([m, lbl]) => {
      const b = El('button', sharedMode === m ? 'on' : '', lbl);
      b.addEventListener('click', () => {
        objs.forEach((o) => { o.collision.mode = m; if (m === 'convex') EnsureHull(o); });
        SyncAllUI(false);
        Toast(`Applied ${lbl} collider to ${objs.length} objects`);
      });
      mSeg.appendChild(b);
    });
    batchCard.appendChild(mSeg);

    const pSeg = El('div', 'seg-pills');
    const sharedPreset = objs.every((o) => o.collision.preset === objs[0].collision.preset) ? objs[0].collision.preset : null;
    [['solid', 'Solid'], ['static', 'Static'], ['vehicle', 'Vehicle'], ['trigger', 'Trigger']].forEach(([p, lbl]) => {
      const b = El('button', sharedPreset === p ? 'on' : '', lbl);
      b.addEventListener('click', () => {
        objs.forEach((o) => { o.collision.preset = p; });
        RenderInspector();
      });
      pSeg.appendChild(b);
    });
    batchCard.appendChild(pSeg);

    const expRow = El('div', 'btn-row-2');
    const reb = El('button', 'mini-act-btn', `Rebuild All (${objs.length})`);
    reb.addEventListener('click', () => {
      objs.forEach((o) => { o.collision.hullCache = null; EnsureHull(o); });
      RenderInspector(); RebuildScene(false);
      Toast(`Rebuilt colliders for ${objs.length} objects`);
    });
    const exp = El('button', 'mini-act-btn', 'Export Batch JSON');
    exp.addEventListener('click', () => DownloadCollisionJson(objs));
    expRow.append(reb, exp);
    batchCard.appendChild(expRow);
    body.appendChild(batchCard);
  } else if (State.aspect === 'sdf') {
    const bBtn = El('button', 'btn-primary', `Bake Distance Field for All ${objs.length} Objects`);
    bBtn.addEventListener('click', () => { objs.forEach(BakeSdfForObject); });
    body.appendChild(bBtn);
  } else if (State.aspect === 'uv') {
    const bBtn = El('button', 'btn-primary', `Unwrap & Pack UVs for All ${objs.length} Objects`);
    bBtn.addEventListener('click', async () => {
      for (const o of objs) { await runStep(o.asset, 'uv', compileCtx); o.uvBaked = true; }
      State.channel = 'uv';
      SyncAllUI(false);
      Toast(`UVs packed for ${objs.length} objects`);
    });
    body.appendChild(bBtn);
  } else if (State.aspect === 'lod') {
    const bBtn = El('button', 'btn-primary', `Compile Surface Strata for All ${objs.length} Objects`);
    bBtn.addEventListener('click', async () => {
      for (const o of objs) { await runStep(o.asset, 'lod', compileCtx); o.lodBaked = true; o.activeLod = 0; }
      SyncAllUI(false);
      Toast(`Surface Strata compiled for ${objs.length} objects`);
    });
    body.appendChild(bBtn);
  }

  // 3. INDIVIDUAL OBJECT CARDS (Many Objects in Inspector — 1 card per selected object!)
  body.appendChild(El('div', 'sec-label', `<span>Selected Objects (${objs.length})</span><span>Per-object settings</span>`));

  const colNames = { convex: 'Convex Hull', box: 'Bounding Box', primitives: 'Primitives', mesh_faces: 'Use Mesh Faces' };

  objs.forEach((obj, idx) => {
    const b = boxOf(obj.asset.out.geometry);
    const sz = [b.max.x - b.min.x, b.max.y - b.min.y, b.max.z - b.min.z];
    const tris = obj.asset.out.geometry.indices.length / 3;
    const collapsed = State.collapsedCards.has(obj.id);

    const card = El('div', 'multi-object-card');
    card.dataset.inspObjId = obj.id;
    const head = El('div', 'multi-object-card-head');
    head.innerHTML = `
      <div class="moc-thumb">${obj.asset.out.icon ? `<img src="${obj.asset.out.icon}" alt="${obj.name}" />` : ''}</div>
      <div class="moc-info">
        <strong>#${idx + 1} · ${obj.name}</strong>
        <span>${obj.format} · ${FmtK(tris)} tris · ${sz.map((v) => v.toFixed(1)).join('×')} m</span>
      </div>
    `;
    const focusBtn = El('button', 'moc-focus-btn', 'Solo');
    focusBtn.title = `Select only ${obj.name}`;
    focusBtn.addEventListener('click', (e) => {
      e.stopPropagation();
      State.selectedIds = [obj.id];
      SyncAllUI(true);
    });
    head.appendChild(focusBtn);
    head.addEventListener('click', () => {
      if (State.collapsedCards.has(obj.id)) State.collapsedCards.delete(obj.id);
      else State.collapsedCards.add(obj.id);
      RenderInspector();
    });
    card.appendChild(head);

    if (!collapsed) {
      const cBody = El('div', 'multi-object-card-body');
      if (State.aspect === 'collision') {
        const c = obj.collision;
        const mSeg = El('div', 'seg-pills');
        [['convex', 'Hull'], ['box', 'Box'], ['primitives', 'Prims'], ['mesh_faces', 'Mesh']].forEach(([m, lbl]) => {
          const bt = El('button', c.mode === m ? 'on' : '', lbl);
          bt.addEventListener('click', () => {
            c.mode = m;
            if (m === 'convex') EnsureHull(obj);
            SyncAllUI(false);
          });
          mSeg.appendChild(bt);
        });
        cBody.appendChild(mSeg);

        if (c.mode === 'convex') {
          const hull = EnsureHull(obj);
          cBody.appendChild(El('div', 'kv-row', `<span>Hull</span><span>${hull ? `${hull.vertCount}v · ${hull.triCount}f · ${hull.volume.toFixed(2)} m³` : '—'}</span>`));
          AddSlider(cBody, 'Inflation', c.hullInflation * 1000, 0, 50, 1, (v) => `+${v.toFixed(0)} mm`, (v) => {
            c.hullInflation = v / 1000; c.hullCache = null; EnsureHull(obj); RebuildScene(false);
          });
        } else if (c.mode === 'box') {
          cBody.appendChild(El('div', 'kv-row', `<span>Box Extents</span><span>${c.boxSize.map((v) => v.toFixed(2)).join(' × ')} m</span>`));
          AddSlider(cBody, 'Padding', c.boxPadding * 1000, 0, 80, 1, (v) => `${v.toFixed(0)} mm`, (v) => { c.boxPadding = v / 1000; RebuildScene(false); });
        } else if (c.mode === 'primitives') {
          cBody.appendChild(El('div', 'kv-row', `<span>Compound Primitives</span><span>${c.primitives.length} shapes</span>`));
        } else {
          cBody.appendChild(El('div', 'kv-row', `<span>Triangle Collider</span><span>${tris.toLocaleString()} exact faces</span>`));
        }
        cBody.appendChild(El('div', 'kv-row', `<span>Physics Preset</span><span>${c.preset} · μ ${c.friction.toFixed(2)}</span>`));
      } else if (State.aspect === 'finish') {
        for (const sl of obj.slots.slice(0, 3)) {
          const row = El('div', 'slot-card-head');
          const inp = document.createElement('input');
          inp.type = 'color';
          inp.className = 'slot-swatch';
          inp.value = sl.color;
          inp.addEventListener('input', () => { sl.color = inp.value; RebuildScene(false); });
          row.append(inp, El('strong', '', sl.name), El('span', '', sl.color));
          cBody.appendChild(row);
        }
      } else {
        cBody.innerHTML = `
          <div class="kv-row"><span>Collider</span><span>${colNames[obj.collision.mode]}</span></div>
          <div class="kv-row"><span>Distance Field</span><span>${obj.sdf.volume ? `${obj.sdf.resolution}³ baked` : 'Unbaked'}</span></div>
          <div class="kv-row"><span>UV / Strata</span><span>${obj.uvBaked ? 'UV Ready' : 'UV Unbaked'} · ${obj.lodBaked ? 'LODs Ready' : 'LODs Unbaked'}</span></div>
          <div class="kv-row"><span>Material Slots</span><span>${obj.slots.length} slot${obj.slots.length === 1 ? '' : 's'}</span></div>
        `;
      }
      card.appendChild(cBody);
    }
    body.appendChild(card);
  });
}

// ============================================================================
// NAVIGATION & IMPORT HANDLERS
// ============================================================================
function SetAspect(aspect) {
  State.aspect = aspect;
  document.querySelectorAll('.aspect-pill').forEach((b) => b.classList.toggle('on', b.dataset.aspect === aspect));
  SyncAllUI(false);
}

function SyncStageBar() {
  document.querySelectorAll('#ChannelSeg button').forEach((b) => b.classList.toggle('on', b.dataset.ch === State.channel));
  document.querySelectorAll('#ViewSeg button').forEach((b) => b.classList.toggle('on', b.dataset.view === State.view));
  $('WireChip').classList.toggle('on', State.wireframe);
  $('GridChip').classList.toggle('on', State.grid);
  $('PivotChip').classList.toggle('on', State.pivot);
  $('SpinChip').classList.toggle('on', State.spin);
}

function SyncAllUI(refit = false) {
  SyncStageBar();
  RenderOutliner();
  RenderDebugStrip();
  RenderInspector();
  RebuildScene(refit);
}

async function ImportFilesAsSingleObjects(files) {
  const added = [];
  for (const file of files) {
    try {
      const asset = await assetFromFile(file);
      const ext = file.name.split('.').pop()?.toUpperCase() || 'MESH';
      const obj = await WrapAssetAsObject(asset, ext, 'convex');
      State.objects.push(obj);
      added.push(obj.id);
    } catch (err) {
      Toast(`Import failed: ${err.message}`);
    }
  }
  if (added.length) {
    State.selectedIds = added;
    State.lastClickedId = added[added.length - 1];
    SyncAllUI(true);
    Toast(`Imported ${added.length} object${added.length > 1 ? 's' : ''} into Outliner`);
  }
}

function PopulateAddPopover() {
  const list = $('AddPopoverList');
  list.innerHTML = '';
  const items = [
    ...SOURCE_PRESETS.map((p) => ({
      label: p.label,
      note: p.note,
      async build() {
        const src = constructSource(p.id, 10000);
        const asset = createAsset({
          name: p.label,
          fileName: `${p.id}.preset`,
          raw: { positions: src.positions, indices: src.indices, families: null, format: 'Procedural' },
          settings: { geometry: { centre: 'ground', normals: 'smooth' } },
        });
        return WrapAssetAsObject(asset, 'MESH', 'convex');
      },
    })),
    {
      label: 'ControlVehicle Body',
      note: 'FCT1 chassis only',
      async build() {
        const raw = readFacet(await fetchModel('ControlVehicleBody.facet'));
        const asset = createAsset({
          name: 'ControlVehicle Body',
          fileName: 'ControlVehicleBody.facet',
          raw,
          palette: VEHICLE_PALETTE,
          settings: { source: { up: 'z' }, geometry: { centre: 'ground', normals: 'angle' } },
        });
        return WrapAssetAsObject(asset, 'FCT1', 'convex');
      },
    },
  ];
  for (const it of items) {
    const b = El('button', 'add-pop-item', `<strong>${it.label}</strong><span>${it.note}</span>`);
    b.addEventListener('click', async () => {
      $('AddPopover').classList.add('hidden');
      const obj = await it.build();
      State.objects.push(obj);
      State.selectedIds = [obj.id];
      State.lastClickedId = obj.id;
      SyncAllUI(true);
      Toast(`Added ${obj.name} as 1 object`);
    });
    list.appendChild(b);
  }
}

function WireEvents() {
  // Top Aspect Strip
  document.querySelectorAll('.aspect-pill').forEach((btn) => {
    btn.addEventListener('click', () => SetAspect(btn.dataset.aspect));
  });

  // Outliner Search & Quick Buttons
  $('OutlinerSearch').addEventListener('input', (e) => {
    State.searchQuery = e.target.value;
    RenderOutliner();
  });
  $('SelectAllBtn').addEventListener('click', () => {
    State.selectedIds = State.objects.map((o) => o.id);
    SyncAllUI(true);
  });
  $('SoloFirstBtn').addEventListener('click', () => {
    if (State.selectedIds.length) State.selectedIds = [State.selectedIds[0]];
    else if (State.objects.length) State.selectedIds = [State.objects[0].id];
    SyncAllUI(true);
  });
  $('ShowAllBtn').addEventListener('click', () => {
    State.objects.forEach((o) => { o.visible = true; });
    SyncAllUI(false);
  });
  $('AddSpecimenBtn').addEventListener('click', () => $('AddPopover').classList.toggle('hidden'));
  $('AddPopoverClose').addEventListener('click', () => $('AddPopover').classList.add('hidden'));

  $('DupSelectedBtn').addEventListener('click', async () => {
    const sel = SelectedObjects();
    const newIds = [];
    for (const o of sel) {
      const clonedAsset = createAsset({
        name: `${o.name} Copy`,
        fileName: o.asset.fileName,
        raw: o.asset.raw,
        palette: o.asset.palette,
        settings: structuredClone(o.asset.settings),
      });
      const copy = await WrapAssetAsObject(clonedAsset, o.format, o.collision.mode);
      copy.slots = structuredClone(o.slots);
      State.objects.push(copy);
      newIds.push(copy.id);
    }
    if (newIds.length) {
      State.selectedIds = newIds;
      SyncAllUI(true);
      Toast(`Duplicated ${newIds.length} object${newIds.length > 1 ? 's' : ''}`);
    }
  });

  $('DelSelectedBtn').addEventListener('click', () => {
    if (State.objects.length <= 1) { Toast('Keep at least 1 object in Outliner'); return; }
    const delSet = new Set(State.selectedIds);
    const remaining = State.objects.filter((o) => !delSet.has(o.id));
    if (!remaining.length) { Toast('Keep at least 1 object in Outliner'); return; }
    State.objects = remaining;
    State.selectedIds = [remaining[0].id];
    SyncAllUI(true);
  });

  // Import file picker + Drag & Drop
  $('ImportBtn').addEventListener('click', () => $('FileInput').click());
  $('FileInput').addEventListener('change', (e) => {
    if (e.target.files?.length) ImportFilesAsSingleObjects([...e.target.files]);
    e.target.value = '';
  });

  const well = $('ViewportWell');
  well.addEventListener('dragover', (e) => { e.preventDefault(); $('DropOverlay').classList.remove('hidden'); });
  well.addEventListener('dragleave', () => $('DropOverlay').classList.add('hidden'));
  well.addEventListener('drop', (e) => {
    e.preventDefault();
    $('DropOverlay').classList.add('hidden');
    if (e.dataTransfer?.files?.length) ImportFilesAsSingleObjects([...e.dataTransfer.files]);
  });

  // Stage Bar
  document.querySelectorAll('#ChannelSeg button').forEach((b) => {
    b.addEventListener('click', () => { State.channel = b.dataset.ch; SyncStageBar(); RenderDebugStrip(); RebuildScene(false); });
  });
  document.querySelectorAll('#ViewSeg button').forEach((b) => {
    b.addEventListener('click', () => { ApplyCameraPreset(b.dataset.view); SyncStageBar(); });
  });
  $('WireChip').addEventListener('click', () => { State.wireframe = !State.wireframe; SyncStageBar(); RebuildScene(false); });
  $('GridChip').addEventListener('click', () => { State.grid = !State.grid; SyncStageBar(); RebuildScene(false); });
  $('PivotChip').addEventListener('click', () => { State.pivot = !State.pivot; SyncStageBar(); RebuildScene(false); });
  $('SpinChip').addEventListener('click', () => { State.spin = !State.spin; SyncStageBar(); });
  $('FitChip').addEventListener('click', () => FitCameraToSelection());
}

// ============================================================================
// BOOT
// ============================================================================
async function Boot() {
  stage = new Stage($('StageCanvas'), { grid: true, shadows: true });
  createCameraHud(stage.rig, $('ViewportWell'));
  iconRenderer = new IconRenderer();

  sceneRoot = new THREE.Group();
  colliderRoot = new THREE.Group();
  helperRoot = new THREE.Group();
  stage.scene.add(sceneRoot, colliderRoot, helperRoot);

  stage.frameCallbacks.add((dt) => {
    if (State.spin && State.aspect !== 'fracture') {
      sceneRoot.rotation.y += dt * 0.45;
      colliderRoot.rotation.y = sceneRoot.rotation.y;
      helperRoot.rotation.y = sceneRoot.rotation.y;
    }
    if (State.aspect === 'sdf' && sdfViewport) UpdateSdfOverlay();
  });

  PopulateAddPopover();
  WireEvents();
  await LoadInitialObjects();
  SyncAllUI(true);
}

Boot().catch((err) => {
  console.error('SpecimenBay boot error:', err);
  Toast(`Boot error: ${err.message}`);
});
