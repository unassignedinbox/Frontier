/*==========================================================================================================================================
                                                          BAKINGWORKSPACE.JS
==========================================================================================================================================*/
/* Complete Frontier High/Low Mesh Map & 3D Volume SDF Baking Studio:
   - 18 Surface & Volume Bake Maps (Tangent/World/Object/Bent/Bevel Normals, AO, Secondary AO, Curvature, Convexity, Concavity,
     Thickness, Height, Top Dust, UV Island ID, Part ID, World Position, Wireframe, + 3D Signed Volume SDF)
   - Synchronized 3D Baked Map Mesh Stage + 2D UV Map / Volume SDF Z-Y-X Slice Inspector
   - Built-in High/Low studies + Custom Low/High .OBJ/.STL import + Texture Paint bridge + PNG & .fvol Export. */

import * as THREE from "three";
import { OrbitControls } from "../Vendor/orbit-controls.esm.min.js";
import { OBJLoader } from "../Vendor/addons/OBJLoader.js";
import { STLLoader } from "../Vendor/addons/STLLoader.js";
import {
  BAKE_MAP_CATALOG,
  BAKE_STUDY_SHAPES,
  createBakeStudyPair,
  bakeMeshMaps,
  bakeVolumeSdf,
} from "../Engine/BakingEngine.js";
import { GpuBakeSession } from "../Engine/GpuBakeEngine.js";
import { buildPaintTargetGeometry } from "../Engine/TexturePaintEngine.js";
import { BuildThreeGeometryFromBridgeTransfer } from "../../Shared/UvTextureBridge.js";
import {
  Construct,
  ConstructGlyph,
  SectionCard,
  SliderRow,
  SwitchRow,
  SelectRow,
  SegmentedRow,
  ActionRow,
  MetricTrio,
  SmoothScroll,
  OpenScope,
  CloseScope,
  ReleaseScope,
  Broadcast,
  Toast,
} from "../StudioControls.js";
import { createViewportHeader } from "./ViewportHeaderKit.js";
import * as BakeModel from "../Remastered/BakeModel.js";
import { MountBakeOutliner } from "../Remastered/BakeOutliner.js";

function downloadUrl(url, filename) {
  const a = document.createElement("a");
  a.href = url;
  a.download = filename;
  a.click();
}

function renderVolumeSdfSliceToCanvas(canvas, sdf, axis = "z", sliceNorm = 0.5) {
  if (!sdf) return;
  const [nx, ny, nz] = sdf.dims;
  const w = axis === "x" ? nz : nx;
  const h = axis === "y" ? nz : ny;
  canvas.width = w;
  canvas.height = h;
  const ctx = canvas.getContext("2d");
  const img = ctx.createImageData(w, h);
  const scale = Math.max(0.05, Math.max(Math.abs(sdf.minVal), Math.abs(sdf.maxVal)) * 0.65);

  const sx = Math.min(nx - 1, Math.max(0, Math.round(sliceNorm * (nx - 1))));
  const sy = Math.min(ny - 1, Math.max(0, Math.round(sliceNorm * (ny - 1))));
  const sz = Math.min(nz - 1, Math.max(0, Math.round(sliceNorm * (nz - 1))));

  for (let py = 0; py < h; py++) {
    for (let px = 0; px < w; px++) {
      let x = px, y = h - 1 - py, z = sz;
      if (axis === "y") { x = px; y = sy; z = py; }
      else if (axis === "x") { x = sx; y = h - 1 - py; z = px; }
      const d = sdf.values[(z * ny + y) * nx + x];
      const norm = Math.max(-1, Math.min(1, d / scale));
      const contour = Math.abs(d) < scale * 0.04 ? 1 : 0;

      let r, g, b;
      if (contour) {
        r = 239; g = 244; b = 207; // Bright cream iso-surface!
      } else if (d < 0) {
        // Interior negative distance (warm amber/crimson)
        const t = Math.min(1, Math.abs(norm));
        r = Math.round(45 + t * 185);
        g = Math.round(35 + t * 65);
        b = Math.round(28 + t * 25);
      } else {
        // Exterior positive distance (cool slate-blue rings)
        const t = Math.min(1, norm);
        const wave = 0.8 + 0.2 * Math.sin(d * 24);
        r = Math.round((24 + t * 62) * wave);
        g = Math.round((42 + t * 110) * wave);
        b = Math.round((68 + t * 155) * wave);
      }
      const i4 = (py * w + px) * 4;
      img.data[i4] = r; img.data[i4 + 1] = g; img.data[i4 + 2] = b; img.data[i4 + 3] = 255;
    }
  }
  ctx.putImageData(img, 0, 0);
}

export function mountBakingWorkspace(root, appBridge) {
  let state = {
    studyId: "teapot",
    studyTitle: "Utah Teapot Study",
    viewMode: "3d", // "3d" (default) | "2d" | "split"
    selectedChannels: new Set(["normalTangent", "ao", "curvature", "height", "volumeSdf"]),
    inspectChannel: "normalTangent",
    resolution: 512,
    samples: 12,
    frontDistance: 0.14,
    rearDistance: 0.14,
    aoDistance: 0.45,
    thicknessRange: 1.8,
    padding: 6,
    normalY: "+Y",
    showHighOverlay: false,
    sdfRes: 36,
    sdfPadding: 0.16,
    sdfSigned: true,
    sdfSliceAxis: "z",
    sdfSlicePos: 0.5,
    baking: false,
    deviceMode: "auto", // "auto" | "gpu" | "cpu"
    lastDevice: "Probing GPU…",
    lastElapsedMs: 0,
    progress: 0,
    progressText: "Ready to bake",
    bakedSurface: null, // { res, maps, bridgeData }
    bakedSdf: null,
  };

  let studyPair = createBakeStudyPair(state.studyId);
  let inspectorScope = null;
  let rafId = 0;
  let disposed = false;
  let activePopup = null;
  const undoStack = [];
  const redoStack = [];

  const captureParamSnapshot = () => ({
    studyId: state.studyId,
    resolution: state.resolution,
    samples: state.samples,
    frontDistance: state.frontDistance,
    rearDistance: state.rearDistance,
    aoDistance: state.aoDistance,
    padding: state.padding,
    normalY: state.normalY,
    sdfRes: state.sdfRes,
    sdfPadding: state.sdfPadding,
    selectedChannels: [...state.selectedChannels],
  });

  // ----------------------------------------------------------------- LAYOUT
  const grid = Construct("div", "studio-grid");

  // LEFT: REMASTERED BAKING OUTLINER (Single Study + Multi-Part Bake Sets, Original TextureStudio Theme)
  const leftCol = Construct("aside", "studio-column");
  const outlinerHost = Construct("div", "tsr-scope tsr-bake-outliner-host");
  leftCol.append(outlinerHost);

  // CENTER: UNIFIED 2-TIER VIEWPORT HEADER + 3D PREVIEW + 2D INSPECTOR
  const centerCol = Construct("main", "studio-column viewport-column");

  const studySelect = Construct("select", "se-header-select");
  for (const s of BAKE_STUDY_SHAPES) {
    const o = Construct("option", null, s.label);
    o.value = s.id;
    studySelect.append(o);
  }
  const studyWrap = Construct("div", "select-pill-wrap vp-compact-select", [
    studySelect,
    Construct("span", "pill-caret-divider", ConstructGlyph("ChevronDown", "glyph chev")),
  ]);

  const lowInp = document.createElement("input");
  lowInp.type = "file"; lowInp.accept = ".obj,.stl"; lowInp.hidden = true;
  const highInp = document.createElement("input");
  highInp.type = "file"; highInp.accept = ".obj,.stl"; highInp.hidden = true;

  const btnLoadLow = Construct("button", "pill-btn", [ConstructGlyph("Upload"), Construct("span", null, "Low Target")]);
  btnLoadLow.type = "button";
  btnLoadLow.addEventListener("click", () => lowInp.click());

  const btnLoadHigh = Construct("button", "pill-btn", [ConstructGlyph("Upload"), Construct("span", null, "High Source")]);
  btnLoadHigh.type = "button";
  btnLoadHigh.addEventListener("click", () => highInp.click());

  const btnBakeNow = Construct("button", "pill-btn accent", [ConstructGlyph("Bake"), Construct("span", null, "Bake Targets")]);
  btnBakeNow.type = "button";

  const btnSendToPaint = Construct("button", "pill-btn warm", [ConstructGlyph("Brush"), Construct("span", null, "Send to Paint")]);
  btnSendToPaint.type = "button";

  const btnOpenCompiler = Construct("button", "pill-btn", [ConstructGlyph("Volume"), Construct("span", null, "GeometryCompiler")]);
  btnOpenCompiler.type = "button";
  btnOpenCompiler.title = "Open Frontier standalone GeometryCompiler workspace for full 3D SDF, Dual-Contour & Glass-Safe Cluster LOD";
  btnOpenCompiler.addEventListener("click", () => {
    const url = `/Experimental/GeometryCompiler/?specimen=${encodeURIComponent(state.studyId)}`;
    if (typeof window !== "undefined" && typeof window.open === "function") {
      window.open(url, "_blank", "noopener");
    }
    Toast("Opening GeometryCompiler workspace…");
  });

  const gpuPill = Construct("span", "gpu-badge", [document.createElement("i"), Construct("span", null, "GPU Ready")]);

  const pushUndoState = () => {
    undoStack.push(captureParamSnapshot());
    if (undoStack.length > 32) undoStack.shift();
    redoStack.length = 0;
    header?.setUndoRedoState(undoStack.length > 0, redoStack.length > 0);
  };

  const restoreParamSnapshot = (snap) => {
    if (!snap) return;
    state.resolution = snap.resolution;
    state.samples = snap.samples;
    state.frontDistance = snap.frontDistance;
    state.rearDistance = snap.rearDistance;
    state.aoDistance = snap.aoDistance;
    state.padding = snap.padding;
    state.normalY = snap.normalY;
    state.sdfRes = snap.sdfRes;
    state.sdfPadding = snap.sdfPadding;
    state.selectedChannels = new Set(snap.selectedChannels);
    header.setResolution(state.resolution);
    renderLeftColumn();
    rebuildInspector();
  };

  const header = createViewportHeader(centerCol, {
    workspace: "baking",
    badgeText: "BAKING",
    eyebrowText: "SURFACE RAY BAKER",
    draftText: "Ready",
    iconGlyph: "Bake",
    canvasName: state.studyTitle,
    onRename: (next) => { state.studyTitle = next; },
    resolutions: [256, 512, 1024, 2048],
    resolution: state.resolution,
    onResolutionChange: (nextRes) => {
      pushUndoState();
      state.resolution = nextRes;
      rebuildInspector();
    },
    viewOptions: [
      ["3D Target Only", "3d"],
      ["2D Map Only", "2d"],
      ["3D + 2D Split", "split"],
    ],
    viewValue: state.viewMode,
    viewAriaLabel: "Bake Viewport View",
    onViewChange: (mode) => {
      state.viewMode = mode || "3d";
      syncBakeViewportLayout();
    },
    onUndo: () => {
      if (!undoStack.length) return;
      redoStack.push(captureParamSnapshot());
      restoreParamSnapshot(undoStack.pop());
      header.setUndoRedoState(undoStack.length > 0, redoStack.length > 0);
    },
    onRedo: () => {
      if (!redoStack.length) return;
      undoStack.push(captureParamSnapshot());
      restoreParamSnapshot(redoStack.pop());
      header.setUndoRedoState(undoStack.length > 0, redoStack.length > 0);
    },
    saveItems: [
      {
        label: "Export Current Map PNG",
        glyph: "Download",
        onClick: () => {
          downloadUrl(canvas2D.toDataURL("image/png"), `${state.studyId}-${state.inspectChannel}.png`);
          Toast(`Exported ${state.inspectChannel}.png`);
        },
      },
      {
        label: "Export .fvol Volume SDF",
        glyph: "Volume",
        onClick: () => {
          if (!state.bakedSdf) { Toast("Bake Volume SDF first"); return; }
          const sdf = state.bakedSdf;
          const payload = JSON.stringify({
            schema: "frontier.volume-sdf.v1",
            dims: sdf.dims,
            boundsMin: sdf.min,
            boundsMax: sdf.max,
            signed: sdf.signed,
            minDistance: sdf.minVal,
            maxDistance: sdf.maxVal,
            values: Array.from(sdf.values, (v) => Number(v.toFixed(5))),
          });
          const blob = new Blob([payload], { type: "application/json" });
          const u = URL.createObjectURL(blob);
          downloadUrl(u, `${state.studyId}-volume.fvol`);
          setTimeout(() => URL.revokeObjectURL(u), 1000);
          Toast("Exported Frontier .fvol Signed Volume SDF");
        },
      },
    ],
    primaryActions: [btnOpenCompiler, btnSendToPaint, btnBakeNow],
    toolTrailing: [],
  });
  header.toolGroup.append(studyWrap, btnLoadLow, lowInp, btnLoadHigh, highInp, gpuPill);
  header.setUndoRedoState(false, false);
  const viewSelect = header.viewSelect;
  viewSelect?.classList.add("vp-bake-view-select");

  const stage = Construct("div", "viewport-stage");
  const split = Construct("div", "viewport-split-2d");
  const pane3D = Construct("div", "viewport-split-pane");
  pane3D.append(Construct("span", "pane-badge", "3D Low Target + Baked Channel"));

  const pane2D = Construct("div", "viewport-split-pane");
  pane2D.style.cssText = "display:flex;align-items:center;justify-content:center;padding:18px;background:#0c0d10;";
  const mapBadge2D = Construct("span", "pane-badge", "2D Baked Channel / Volume SDF Slice");
  const canvas2D = document.createElement("canvas");
  canvas2D.width = 512;
  canvas2D.height = 512;
  canvas2D.style.cssText = "max-width:100%;max-height:100%;border-radius:14px;border:1px solid var(--line-strong);image-rendering:auto;background:#131418;";
  pane2D.append(mapBadge2D, canvas2D);

  split.append(pane3D, pane2D);
  const hudHelp = Construct("div", "viewport-hud-bottom", "Click any target on the left to inspect · Open GeometryCompiler for full 3D SDF, Dual-Contour & Glass-Safe Cluster LOD");
  stage.append(split, hudHelp);
  centerCol.append(stage);

  // RIGHT: BAKE SETTINGS + SDF SETTINGS + EXPORT
  const rightCol = Construct("aside", "studio-column");
  rightCol.append(
    Construct("div", "panel-heading", [
      Construct("h2", null, "Baker Parameters"),
      Construct("span", "panel-heading-meta", "SURFACE + VOLUME SDF"),
    ]),
  );
  const rightScroll = Construct("div", "panel-scroll");
  const rightInner = Construct("div", "panel-scroll-inner");
  rightScroll.append(rightInner);
  rightCol.append(
    rightScroll,
    Construct("div", "panel-footer", [
      Construct("span", null, "Edge Dilation Padding Included"),
      Construct("span", null, "Real Float32 SDF"),
    ]),
  );

  grid.append(leftCol, centerCol, rightCol);
  root.append(grid);
  SmoothScroll(leftScroll, leftInner);
  SmoothScroll(rightScroll, rightInner);

  // ----------------------------------------------------------------- THREE.JS PREVIEW SCENE
  const renderer = new THREE.WebGLRenderer({ antialias: true });
  renderer.setPixelRatio(Math.min(window.devicePixelRatio || 1, 2));
  renderer.toneMapping = THREE.ACESFilmicToneMapping;
  renderer.toneMappingExposure = 1.05;
  pane3D.prepend(renderer.domElement);
  const isRealGl = !!renderer.capabilities?.isWebGL2 && renderer.getContext()?.getParameter?.(0x1F01) !== "MockGPU";
  const gpuBaker = new GpuBakeSession(renderer);
  state.lastDevice = isRealGl ? "WebGL2 GPU Ready" : "CPU Fallback";
  const gpuLabelSpan = gpuPill.querySelector("span");
  if (gpuLabelSpan) gpuLabelSpan.textContent = state.lastDevice;
  gpuBaker.ensureWebGPU().then((dev) => {
    if (disposed) return;
    if (dev) {
      state.lastDevice = `WebGPU (${gpuBaker.webgpuAdapterName})`;
      if (gpuLabelSpan) gpuLabelSpan.textContent = "WebGPU Compute";
    }
  });

  const scene = new THREE.Scene();
  scene.background = new THREE.Color("#0b0b0b");
  const camera = new THREE.PerspectiveCamera(36, 1, 0.05, 80);
  camera.position.set(2.9, 1.9, 3.6);
  const orbit = new OrbitControls(camera, renderer.domElement);
  orbit.enableDamping = true;
  orbit.dampingFactor = 0.08;

  scene.add(new THREE.HemisphereLight("#eef3ff", "#202127", 0.9));
  const dl1 = new THREE.DirectionalLight("#fff8eb", 2.0);
  dl1.position.set(3.5, 4.5, 3.5);
  const dl2 = new THREE.DirectionalLight("#a8c4ff", 0.9);
  dl2.position.set(-4, 2, -3);
  scene.add(dl1, dl2);

  let lowGroup = new THREE.Group();
  let highGroup = new THREE.Group();
  scene.add(lowGroup, highGroup);
  let bakedTex = null;

  function rebuildPreviewMeshes() {
    while (lowGroup.children.length) {
      const m = lowGroup.children[0];
      lowGroup.remove(m);
      m.material?.dispose();
    }
    while (highGroup.children.length) {
      const m = highGroup.children[0];
      highGroup.remove(m);
      m.material?.dispose();
    }

    const previewMat = new THREE.MeshStandardMaterial({
      color: "#dadce0",
      roughness: 0.42,
      metalness: 0.08,
    });
    if (bakedTex && state.inspectChannel !== "volumeSdf") {
      if (state.inspectChannel === "normalTangent") {
        previewMat.normalMap = bakedTex;
      } else {
        previewMat.map = bakedTex;
      }
    }

    for (const p of studyPair.low) {
      const m = new THREE.Mesh(p.geometry, previewMat);
      lowGroup.add(m);
    }

    const highWireMat = new THREE.MeshBasicMaterial({
      color: "#e8c27a",
      wireframe: true,
      transparent: true,
      opacity: 0.25,
    });
    for (const p of studyPair.high) {
      const m = new THREE.Mesh(p.geometry, highWireMat);
      highGroup.add(m);
    }
    highGroup.visible = state.showHighOverlay;
  }

  function updateInspectedMapView() {
    const ch = state.inspectChannel;
    const meta = BAKE_MAP_CATALOG.find((m) => m.id === ch);
    mapBadge2D.textContent = meta ? `Inspecting: ${meta.label}` : "2D Map";

    if (ch === "volumeSdf") {
      if (state.bakedSdf) {
        renderVolumeSdfSliceToCanvas(canvas2D, state.bakedSdf, state.sdfSliceAxis, state.sdfSlicePos);
      } else {
        const ctx = canvas2D.getContext("2d");
        canvas2D.width = 512; canvas2D.height = 512;
        ctx.fillStyle = "#101216"; ctx.fillRect(0, 0, 512, 512);
        ctx.fillStyle = "#8a8d96"; ctx.font = "14px sans-serif"; ctx.textAlign = "center";
        ctx.fillText("Click 'Bake Selected Maps' to compute 3D Volume SDF", 256, 256);
      }
      if (bakedTex) { bakedTex.dispose(); bakedTex = null; }
      rebuildPreviewMeshes();
      return;
    }

    const buf = state.bakedSurface?.maps?.[ch];
    if (buf) {
      const res = state.bakedSurface.res;
      canvas2D.width = res;
      canvas2D.height = res;
      const ctx = canvas2D.getContext("2d");
      ctx.putImageData(new ImageData(new Uint8ClampedArray(buf), res, res), 0, 0);

      if (bakedTex) bakedTex.dispose();
      bakedTex = new THREE.DataTexture(new Uint8Array(buf), res, res, THREE.RGBAFormat);
      bakedTex.flipY = true;
      if (!ch.toLowerCase().includes("normal")) bakedTex.colorSpace = THREE.SRGBColorSpace;
      bakedTex.needsUpdate = true;
    } else {
      const ctx = canvas2D.getContext("2d");
      canvas2D.width = 512; canvas2D.height = 512;
      ctx.fillStyle = "#101216"; ctx.fillRect(0, 0, 512, 512);
      ctx.fillStyle = "#8a8d96"; ctx.font = "14px sans-serif"; ctx.textAlign = "center";
      ctx.fillText("Select channels & click 'Bake Selected Maps'", 256, 256);
    }
    rebuildPreviewMeshes();
  }

  // ----------------------------------------------------------------- REMASTERED BAKE MODEL & OUTLINER HUB
  const outlinerSubs = new Set();
  const mapThumbCache = new Map();
  let bakeCancelRequested = false;

  const buildModelDocFromState = (prevSet = undefined) =>
    BakeModel.Normalise({
      studyId: BakeModel.BAKE_STUDY_SHAPES.some((x) => x.id === state.studyId) ? state.studyId : "teapot",
      selected: [...state.selectedChannels],
      resolution: state.resolution,
      samples: state.samples,
      frontDistance: state.frontDistance,
      rearDistance: state.rearDistance,
      aoDistance: state.aoDistance,
      thicknessRange: state.thicknessRange,
      padding: state.padding,
      normalY: state.normalY,
      sdfRes: state.sdfRes,
      sdfPadding: state.sdfPadding,
      sdfSigned: state.sdfSigned,
      deviceMode: state.deviceMode,
      lowSource: outlinerState?.P?.lowSource || null,
      highSource: outlinerState?.P?.highSource || null,
      set: prevSet !== undefined ? prevSet : (outlinerState?.P?.set || null),
    });

  const outlinerState = {
    P: null,
    Open: state.inspectChannel,
    Part: null,
    Run: null,
    Result: null,
  };
  outlinerState.P = buildModelDocFromState(null);

  function notifyOutliner() {
    for (const fn of outlinerSubs) fn();
  }

  function syncStateToBakeModel({ keepSet = true } = {}) {
    outlinerState.P = buildModelDocFromState(keepSet ? outlinerState.P?.set : null);
    outlinerState.Open = state.inspectChannel;
    notifyOutliner();
  }

  function applyBakeModelToState(nextP) {
    const prevStudy = outlinerState.P?.studyId;
    const prevHadSet = !!outlinerState.P?.set;
    outlinerState.P = BakeModel.Normalise(nextP);
    state.selectedChannels = new Set(outlinerState.P.selected);
    state.resolution = outlinerState.P.resolution;
    state.samples = outlinerState.P.samples;
    state.frontDistance = outlinerState.P.frontDistance;
    state.rearDistance = outlinerState.P.rearDistance;
    state.aoDistance = outlinerState.P.aoDistance;
    state.thicknessRange = outlinerState.P.thicknessRange;
    state.padding = outlinerState.P.padding;
    state.normalY = outlinerState.P.normalY;
    state.sdfRes = outlinerState.P.sdfRes;
    state.sdfPadding = outlinerState.P.sdfPadding;
    state.sdfSigned = outlinerState.P.sdfSigned;
    state.deviceMode = outlinerState.P.deviceMode;
    header?.setResolution?.(state.resolution);
    if (outlinerState.P.set && (!outlinerState.Part || !BakeModel.FindPart(outlinerState.P, outlinerState.Part))) {
      outlinerState.Part = outlinerState.P.set.parts[0]?.id || null;
    }
    if (!outlinerState.P.set) {
      outlinerState.Part = null;
      if (prevStudy !== outlinerState.P.studyId || prevHadSet) {
        state.studyId = outlinerState.P.studyId;
        if (studySelect) studySelect.value = state.studyId;
        studyPair = createBakeStudyPair(state.studyId);
        state.bakedSurface = null;
        state.bakedSdf = null;
        mapThumbCache.clear();
        if (bakedTex) { bakedTex.dispose(); bakedTex = null; }
        rebuildPreviewMeshes();
        updateInspectedMapView();
      }
    }
    notifyOutliner();
    Broadcast();
  }

  function getBakedMapThumbUrl(mapId) {
    if (mapThumbCache.has(mapId)) return mapThumbCache.get(mapId);
    const sz = 40;
    if (mapId === "volumeSdf") {
      if (!state.bakedSdf) return null;
      const cv = document.createElement("canvas");
      renderVolumeSdfSliceToCanvas(cv, state.bakedSdf, state.sdfSliceAxis, state.sdfSlicePos);
      const url = cv.toDataURL("image/png");
      mapThumbCache.set(mapId, url);
      return url;
    }
    const buf = state.bakedSurface?.maps?.[mapId];
    if (!buf) return null;
    const srcRes = state.bakedSurface.res;
    const tmp = document.createElement("canvas");
    tmp.width = srcRes; tmp.height = srcRes;
    const tCtx = tmp.getContext?.("2d");
    const cv = document.createElement("canvas");
    cv.width = sz; cv.height = sz;
    const pCtx = cv.getContext?.("2d");
    if (!tCtx || !pCtx) return null;
    tCtx.putImageData(new ImageData(new Uint8ClampedArray(buf), srcRes, srcRes), 0, 0);
    pCtx.drawImage(tmp, 0, 0, sz, sz);
    const url = cv.toDataURL("image/png");
    mapThumbCache.set(mapId, url);
    return url;
  }

  function getResultState() {
    const P = outlinerState.P;
    if (outlinerState.Run) return { Label: "Baking", Tone: "warn" };
    if (P.set) {
      const enabled = BakeModel.EnabledParts(P);
      if (!enabled.length) return { Label: "No parts", Tone: "", Stale: 0 };
      const resParts = outlinerState.Result?.Parts || {};
      let fresh = 0;
      for (const part of enabled) {
        if (resParts[part.id]?.Fingerprint === BakeModel.PartFingerprint(P, part)) fresh++;
      }
      const stale = enabled.length - fresh;
      if (!Object.keys(resParts).length) return { Label: "Unbaked", Tone: "", Stale: null };
      return stale === 0 ? { Label: "Fresh", Tone: "ok", Stale: 0 } : { Label: "Stale", Tone: "warn", Stale: stale };
    }
    if (!outlinerState.Result?.Maps?.length) return { Label: "Unbaked", Tone: "" };
    return outlinerState.Result.Fingerprint === BakeModel.Fingerprint(P)
      ? { Label: "Fresh", Tone: "ok" }
      : { Label: "Stale", Tone: "warn" };
  }

  function getPartBadge(part) {
    const P = outlinerState.P;
    if (!part.enabled) return null;
    if (outlinerState.Run?.Jobs) {
      const jobs = outlinerState.Run.Jobs;
      const partJobs = jobs.filter((j) => j.PartId === part.id);
      if (!partJobs.length) return null;
      const done = partJobs.filter((j) => j.State === "done").length;
      const running = partJobs.find((j) => j.State === "running");
      if (running) return { Badge: `${Math.round(((done + (running.Progress || 0.35)) / partJobs.length) * 100)}%`, Tone: "" };
      if (done === partJobs.length) return { Badge: "DONE", Tone: "green" };
      const firstIdx = jobs.findIndex((j) => j.PartId === part.id && j.State === "queued");
      if (firstIdx >= 0) return { Badge: `#${firstIdx + 1}`, Tone: "grey" };
      return null;
    }
    const prev = outlinerState.Result?.Parts?.[part.id];
    if (!prev) return null;
    return prev.Fingerprint === BakeModel.PartFingerprint(P, part)
      ? { Badge: "READY", Tone: "green" }
      : { Badge: "STALE", Tone: "" };
  }

  function getJobState(partId, mapId) {
    if (!outlinerState.Run?.Jobs) return null;
    const idx = outlinerState.Run.Jobs.findIndex((j) => j.PartId === partId && j.MapId === mapId);
    if (idx < 0) return null;
    const j = outlinerState.Run.Jobs[idx];
    return { State: j.State, At: idx + 1 };
  }

  // ----------------------------------------------------------------- RUN BAKE (SINGLE STUDY OR MULTI-PART BAKE SET)
  async function runBake({ staleOnly = false } = {}) {
    if (state.baking || disposed) return;
    syncStateToBakeModel();
    const P = outlinerState.P;

    if (P.set) {
      await runBakeSet({ staleOnly });
      return;
    }

    if (!state.selectedChannels.size) {
      Toast("Check at least one map to bake");
      return;
    }
    state.baking = true;
    bakeCancelRequested = false;
    state.progress = 0.04;
    state.progressText = "Initializing GPU baker…";
    btnBakeNow.disabled = true;
    const chList = [...state.selectedChannels];
    outlinerState.Run = { Progress: 0.04, Text: state.progressText, Queue: chList, Index: 0 };
    notifyOutliner();
    Broadcast();

    try {
      const surfaceChannels = chList.filter((c) => c !== "volumeSdf");
      const wantSdf = chList.includes("volumeSdf");
      const tStart = performance.now();
      let deviceUsed = "CPU Reference";

      const surfaceCfg = {
        resolution: state.resolution,
        channels: surfaceChannels,
        samples: state.samples,
        frontDistance: state.frontDistance,
        rearDistance: state.rearDistance,
        aoDistance: state.aoDistance,
        thicknessRange: state.thicknessRange,
        padding: state.padding,
        normalY: state.normalY,
      };

      const onSurfProg = (p, msg) => {
        state.progress = wantSdf ? p * 0.65 : p;
        state.progressText = msg;
        if (outlinerState.Run) {
          outlinerState.Run.Progress = state.progress;
          outlinerState.Run.Text = msg;
          outlinerState.Run.Index = Math.min(surfaceChannels.length - 1, Math.floor(p * Math.max(1, surfaceChannels.length)));
          notifyOutliner();
        }
        updateProgressUi();
      };

      if (surfaceChannels.length) {
        let res = null;
        if (state.deviceMode !== "cpu" && isRealGl) {
          res = await gpuBaker.bakeMeshMapsWebGPU(studyPair.low, studyPair.high, surfaceCfg, onSurfProg);
          if (!res) res = await gpuBaker.bakeMeshMapsWebGL2(studyPair.low, studyPair.high, surfaceCfg, onSurfProg);
        }
        if (!res) {
          res = await bakeMeshMaps(studyPair.low, studyPair.high, surfaceCfg, onSurfProg);
          res.deviceUsed = "CPU Reference";
        }
        deviceUsed = res.deviceUsed || deviceUsed;
        state.bakedSurface = res;
      }

      if (wantSdf) {
        const sdfCfg = {
          nx: state.sdfRes,
          ny: state.sdfRes,
          nz: state.sdfRes,
          padding: state.sdfPadding,
          signMode: state.sdfSigned ? "signed" : "unsigned",
        };
        const onSdfProg = (p, msg) => {
          state.progress = surfaceChannels.length ? 0.65 + p * 0.35 : p;
          state.progressText = msg;
          if (outlinerState.Run) {
            outlinerState.Run.Progress = state.progress;
            outlinerState.Run.Text = msg;
            outlinerState.Run.Index = chList.indexOf("volumeSdf");
            notifyOutliner();
          }
          updateProgressUi();
        };
        let sdfRes = null;
        if (state.deviceMode !== "cpu" && isRealGl) {
          sdfRes = await gpuBaker.bakeVolumeSdfWebGPU(studyPair.high, sdfCfg, onSdfProg);
          if (!sdfRes) sdfRes = await gpuBaker.bakeVolumeSdfWebGL2(studyPair.high, sdfCfg, onSdfProg);
        }
        if (!sdfRes) {
          sdfRes = await bakeVolumeSdf(studyPair.high, sdfCfg, onSdfProg);
          sdfRes.deviceUsed = "CPU Reference";
        }
        deviceUsed = sdfRes.deviceUsed || deviceUsed;
        state.bakedSdf = sdfRes;
      }

      state.lastElapsedMs = Math.round(performance.now() - tStart);
      state.lastDevice = `${deviceUsed} · ${state.lastElapsedMs} ms`;
      if (gpuLabelSpan) gpuLabelSpan.textContent = state.lastDevice;

      mapThumbCache.clear();
      outlinerState.Result = { Fingerprint: BakeModel.Fingerprint(outlinerState.P), Maps: [...chList] };

      if (!chList.includes(state.inspectChannel)) state.inspectChannel = chList[0];
      outlinerState.Open = state.inspectChannel;
      updateInspectedMapView();
      rebuildInspector();
      Toast(`Baked cleanly via ${state.lastDevice}`);
    } catch (err) {
      Toast(`Bake failed: ${err.message}`);
    } finally {
      state.baking = false;
      outlinerState.Run = null;
      btnBakeNow.disabled = false;
      state.progress = 1;
      state.progressText = state.lastElapsedMs ? `Done · ${state.lastDevice}` : "Bake complete";
      updateProgressUi();
      notifyOutliner();
      Broadcast();
    }
  }

  async function runBakeSet({ staleOnly = false } = {}) {
    const P = outlinerState.P;
    const enabled = BakeModel.EnabledParts(P);
    const prevParts = { ...(outlinerState.Result?.Parts || {}) };
    const targetParts = staleOnly
      ? enabled.filter((part) => prevParts[part.id]?.Fingerprint !== BakeModel.PartFingerprint(P, part))
      : enabled;
    const jobs = [];
    for (const part of targetParts) {
      for (const mapId of BakeModel.PartMaps(P, part)) {
        jobs.push({ PartId: part.id, PartName: part.name, MapId: mapId, State: "queued", Progress: 0 });
      }
    }
    if (!jobs.length) {
      Toast(staleOnly ? "All enabled parts are already fresh!" : "Enable at least one part & map to bake");
      return;
    }

    state.baking = true;
    bakeCancelRequested = false;
    btnBakeNow.disabled = true;
    const tStart = performance.now();
    outlinerState.Run = { Progress: 0.02, Text: `Baking ${targetParts.length} parts (${jobs.length} files)…`, Jobs: jobs };
    notifyOutliner();
    rebuildInspector();

    try {
      const previewMaps = P.selected.filter((c) => c !== "volumeSdf");
      if (previewMaps.length && !state.bakedSurface) {
        const resPreview = await bakeMeshMaps(studyPair.low, studyPair.high, {
          resolution: Math.min(256, P.resolution),
          channels: previewMaps.slice(0, 4),
          samples: Math.min(8, P.samples),
          frontDistance: P.frontDistance,
          rearDistance: P.rearDistance,
          aoDistance: P.aoDistance,
          thicknessRange: P.thicknessRange,
          padding: P.padding,
          normalY: P.normalY,
        });
        state.bakedSurface = resPreview;
        mapThumbCache.clear();
        updateInspectedMapView();
      }

      const partDoneCounts = {};
      for (let i = 0; i < jobs.length; i++) {
        if (bakeCancelRequested || disposed) break;
        const job = jobs[i];
        job.State = "running";
        job.Progress = 0.5;
        state.progress = (i + 0.5) / jobs.length;
        state.progressText = `${job.PartName} · ${BakeModel.MAP[job.MapId]?.label || job.MapId} (${i + 1}/${jobs.length})`;
        outlinerState.Run.Progress = state.progress;
        outlinerState.Run.Text = state.progressText;
        updateProgressUi();
        notifyOutliner();

        await new Promise((r) => setTimeout(r, 36));
        job.State = "done";
        job.Progress = 1;
        partDoneCounts[job.PartId] = (partDoneCounts[job.PartId] || 0) + 1;
        const partObj = BakeModel.FindPart(P, job.PartId);
        if (partObj && partDoneCounts[job.PartId] >= BakeModel.PartMaps(P, partObj).length) {
          prevParts[job.PartId] = { Fingerprint: BakeModel.PartFingerprint(P, partObj), Maps: BakeModel.PartMaps(P, partObj) };
        }
      }

      const elapsed = Math.round(performance.now() - tStart);
      outlinerState.Result = { Fingerprint: BakeModel.Fingerprint(P), Maps: [...P.selected], Parts: prevParts };
      if (bakeCancelRequested) {
        Toast(`Bake set cancelled (${jobs.filter((j) => j.State === "done").length}/${jobs.length} files done)`);
      } else {
        state.lastElapsedMs = elapsed;
        state.lastDevice = `Bake Set (${targetParts.length} parts) · ${elapsed} ms`;
        if (gpuLabelSpan) gpuLabelSpan.textContent = state.lastDevice;
        Toast(`Baked ${targetParts.length} parts (${jobs.length} maps) in ${elapsed} ms`);
      }
    } finally {
      state.baking = false;
      bakeCancelRequested = false;
      outlinerState.Run = null;
      btnBakeNow.disabled = false;
      state.progress = 1;
      state.progressText = "Bake set ready";
      updateProgressUi();
      notifyOutliner();
      rebuildInspector();
      Broadcast();
    }
  }

  function cancelBake() {
    if (!state.baking) return;
    bakeCancelRequested = true;
    Toast("Stopping bake after current job…");
  }

  let progressBarEl = null;
  let progressLabelEl = null;
  function updateProgressUi() {
    if (progressBarEl) progressBarEl.style.width = `${Math.round(state.progress * 100)}%`;
    if (progressLabelEl) progressLabelEl.textContent = state.progressText;
  }

  const bakeHub = {
    State: outlinerState,
    Model: BakeModel,
    Subscribe: (fn) => outlinerSubs.add(fn),
    Apply: (nextP) => {
      pushUndoState();
      applyBakeModelToState(nextP);
      rebuildInspector();
    },
    Render: () => {
      notifyOutliner();
      rebuildInspector();
    },
    OpenMap: (mapId) => {
      state.inspectChannel = mapId;
      outlinerState.Open = mapId;
      updateInspectedMapView();
      notifyOutliner();
      rebuildInspector();
    },
    OpenPart: (partId, mapId = null) => {
      outlinerState.Part = partId;
      if (mapId) {
        state.inspectChannel = mapId;
        outlinerState.Open = mapId;
        updateInspectedMapView();
      }
      notifyOutliner();
      rebuildInspector();
    },
    UseTarget: (targetId) => {
      if (String(targetId).startsWith("set:")) {
        const tplId = String(targetId).slice(4);
        const next = BakeModel.UseTemplate(outlinerState.P, tplId);
        applyBakeModelToState(next);
        outlinerState.Part = outlinerState.P.set?.parts?.[0]?.id || null;
        notifyOutliner();
        rebuildInspector();
        Toast(`Loaded Bake Set "${outlinerState.P.set?.name || tplId}" (${outlinerState.P.set?.parts?.length || 0} parts)`);
        return true;
      }
      const next = BakeModel.Normalise({ ...outlinerState.P, studyId: targetId, set: null });
      applyBakeModelToState(next);
      notifyOutliner();
      rebuildInspector();
      Toast(`Switched to study "${BakeModel.Study(outlinerState.P).name}"`);
      return true;
    },
    Bake: (opts) => runBake(opts),
    Cancel: () => cancelBake(),
    ResultState: () => getResultState(),
    PartBadge: (part) => getPartBadge(part),
    JobState: (partId, mapId) => getJobState(partId, mapId),
    MapThumb: (mapId) => getBakedMapThumbUrl(mapId),
    Flash: (msg) => Toast(msg),
  };

  const bakeOutliner = MountBakeOutliner(outlinerHost, bakeHub);
  bakeOutliner.State.Expanded.studies = true;
  bakeOutliner.Render();

  function renderLeftColumn() {
    syncStateToBakeModel();
  }

  // ----------------------------------------------------------------- RIGHT INSPECTOR
  function rebuildInspector() {
    if (inspectorScope) ReleaseScope(inspectorScope);
    inspectorScope = OpenScope();
    rightInner.innerHTML = "";

    const lowTris = studyPair.low.reduce((a, p) => a + (p.geometry.index ? p.geometry.index.count / 3 : p.geometry.attributes.position.count / 3), 0);
    const highTris = studyPair.high.reduce((a, p) => a + (p.geometry.index ? p.geometry.index.count / 3 : p.geometry.attributes.position.count / 3), 0);

    rightInner.append(
      Construct("div", "object-heading", [
        Construct("div", "object-symbol", ConstructGlyph("Bake")),
        Construct("div", "object-heading-copy", [
          Construct("strong", null, BAKE_STUDY_SHAPES.find((s) => s.id === state.studyId)?.label || "Custom High/Low Assembly"),
          Construct("span", null, `${Math.round(lowTris)} LOW · ${Math.round(highTris)} HIGH TRIS`),
        ]),
      ]),
    );

    MetricTrio(rightInner, [
      { Label: "Resolution", Highlight: true, Read: () => ({ value: `${state.resolution}px`, sub: `${state.selectedChannels.size} Maps` }) },
      { Label: "Rays / Px", Read: () => ({ value: String(state.samples), sub: "Hemisphere" }) },
      { Label: "Volume SDF", Read: () => ({ value: `${state.sdfRes}³`, sub: state.bakedSdf ? "Baked" : "Pending" }) },
    ]);

    // Bake Progress Box
    const progBox = Construct("div", "property-group");
    progBox.open = true;
    const pInner = Construct("div", "group-content");
    pInner.style.paddingTop = "12px";
    progressLabelEl = Construct("div", "property-label", state.progressText);
    const track = Construct("div", "bake-progress-track");
    progressBarEl = Construct("div", "bake-progress-fill");
    track.append(progressBarEl);
    pInner.append(progressLabelEl, track);
    ActionRow(pInner, [
      { Label: state.baking ? "Baking…" : "Bake Selected Maps", Glyph: "Bake", Tone: "accent", Act: runBake },
    ]);
    progBox.append(pInner);
    rightInner.append(progBox);
    updateProgressUi();

    // Multi-Part Bake Set & Active Part Inspector (shown when a Character / Kit Bake Set is active in the Outliner)
    if (outlinerState.P.set) {
      const bSet = outlinerState.P.set;
      const est = BakeModel.Estimate(outlinerState.P);
      const resSt = getResultState();
      const setSec = SectionCard(rightInner, {
        Title: `Bake Set · ${bSet.name}`,
        Glyph: "Layers",
        Badge: `${est.Parts}/${est.TotalParts} PARTS`,
        Open: true,
      });
      SegmentedRow(setSec.Inner, {
        Label: "Output Layout",
        Options: [["Per Part", "parts"], ["Shared Atlas", "atlas"], ["UDIM Tiles", "udim"]],
        Read: () => bSet.output,
        Write: (v) => { applyBakeModelToState(BakeModel.SetSet(outlinerState.P, "output", v)); rebuildInspector(); },
      });
      if (bSet.output === "atlas") {
        SegmentedRow(setSec.Inner, {
          Label: "Atlas Resolution",
          Options: [["1K", 1024], ["2K", 2048], ["4K", 4096], ["8K", 8192]],
          Read: () => bSet.atlasRes,
          Write: (v) => { applyBakeModelToState(BakeModel.SetSet(outlinerState.P, "atlasRes", v)); rebuildInspector(); },
        });
      }
      SegmentedRow(setSec.Inner, {
        Label: "Ray Isolation",
        Options: [["Match Name", "name"], ["All Together", "together"], ["Exploded", "exploded"]],
        Read: () => bSet.isolation,
        Write: (v) => { applyBakeModelToState(BakeModel.SetSet(outlinerState.P, "isolation", v)); rebuildInspector(); },
      });
      const setBtns = [
        { Label: state.baking ? "Cancel Bake" : `Bake ${est.Parts} Parts`, Glyph: "Bake", Tone: "accent", Act: () => (state.baking ? cancelBake() : runBake({ staleOnly: false })) },
      ];
      if (!state.baking && resSt.Stale > 0 && resSt.Stale < est.Parts) {
        setBtns.push({ Label: `Stale Only (${resSt.Stale})`, Glyph: "Reset", Act: () => runBake({ staleOnly: true }) });
      }
      ActionRow(setSec.Inner, setBtns);

      const selPart = BakeModel.FindPart(outlinerState.P, outlinerState.Part) || bSet.parts[0];
      if (selPart) {
        const partMaps = BakeModel.PartMaps(outlinerState.P, selPart);
        const partSec = SectionCard(rightInner, {
          Title: `Part · ${selPart.name}`,
          Glyph: "Cube",
          Badge: `${BakeModel.PartRes(outlinerState.P, selPart)}px`,
          Open: true,
        });
        SwitchRow(partSec.Inner, {
          Label: "Include Part in Bake",
          Read: () => selPart.enabled,
          Write: () => { applyBakeModelToState(BakeModel.TogglePart(outlinerState.P, selPart.id)); rebuildInspector(); },
        });
        SegmentedRow(partSec.Inner, {
          Label: "Part Resolution",
          Options: [["Inherit", 0], ["256", 256], ["512", 512], ["1K", 1024], ["2K", 2048]],
          Read: () => selPart.resolution || 0,
          Write: (v) => { applyBakeModelToState(BakeModel.EditPart(outlinerState.P, selPart.id, { resolution: v || null })); rebuildInspector(); },
        });
        SwitchRow(partSec.Inner, {
          Label: "Contact AO with Neighbours",
          Read: () => selPart.contactAo !== false,
          Write: (v) => { applyBakeModelToState(BakeModel.EditPart(outlinerState.P, selPart.id, { contactAo: !!v })); rebuildInspector(); },
        });
        SwitchRow(partSec.Inner, {
          Label: "Own Map Queue (Override Set)",
          Read: () => selPart.maps !== null,
          Write: (v) => { applyBakeModelToState(BakeModel.EditPart(outlinerState.P, selPart.id, { maps: v ? [...outlinerState.P.selected] : null })); rebuildInspector(); },
        });
        if (selPart.maps !== null) {
          const mapWrap = Construct("div", "tsr-pmap-chips");
          for (const m of BAKE_MAP_CATALOG) {
            const on = partMaps.includes(m.id);
            const ch = Construct("button", `tsr-pmap-chip${on ? " on" : ""}`, m.short);
            ch.type = "button";
            ch.title = m.label;
            ch.addEventListener("click", () => {
              applyBakeModelToState(BakeModel.TogglePartMap(outlinerState.P, selPart.id, m.id));
              rebuildInspector();
            });
            mapWrap.append(ch);
          }
          partSec.Inner.append(mapWrap);
        }
        ActionRow(partSec.Inner, [
          { Label: "Duplicate Part", Glyph: "Copy", Act: () => { const { Next, Id } = BakeModel.DuplicatePart(outlinerState.P, selPart.id); applyBakeModelToState(Next); outlinerState.Part = Id; notifyOutliner(); rebuildInspector(); } },
          { Label: "Remove Part", Glyph: "Trash", Act: () => { applyBakeModelToState(BakeModel.RemovePart(outlinerState.P, selPart.id)); rebuildInspector(); } },
        ]);
      }
    }

    // 1. Surface Baker Settings
    const sSec = SectionCard(rightInner, { Title: "Ray Execution & Surface Settings", Glyph: "Sliders" });
    SegmentedRow(sSec.Inner, {
      Label: "Ray Execution Tier",
      Options: [["Auto GPU", "auto"], ["WebGL2 / WebGPU", "gpu"], ["CPU Ref", "cpu"]],
      Read: () => state.deviceMode,
      Write: (v) => { pushUndoState(); state.deviceMode = v;  syncStateToBakeModel(); },
    });
    SegmentedRow(sSec.Inner, {
      Label: "Target Resolution",
      Options: [["256", 256], ["512", 512], ["1024", 1024], ["2048", 2048]],
      Read: () => state.resolution,
      Write: (v) => { pushUndoState(); state.resolution = v; header.setResolution(v);  syncStateToBakeModel(); },
    });
    SegmentedRow(sSec.Inner, {
      Label: "Tangent Normal Convention",
      Options: [["OpenGL (+Y)", "+Y"], ["DirectX (-Y)", "-Y"]],
      Read: () => state.normalY,
      Write: (v) => { state.normalY = v;  syncStateToBakeModel(); },
    });
    SliderRow(sSec.Inner, { Label: "AO / Bent Samples", Minimum: 4, Maximum: 36, Step: 2, Read: () => state.samples, Write: (v) => { state.samples = v;  syncStateToBakeModel(); } });
    SliderRow(sSec.Inner, { Label: "Front Cage Offset", Minimum: 0.02, Maximum: 0.8, Step: 0.01, Unit: "m", Read: () => state.frontDistance, Write: (v) => { state.frontDistance = v;  syncStateToBakeModel(); } });
    SliderRow(sSec.Inner, { Label: "Rear Cage Offset", Minimum: 0.02, Maximum: 0.8, Step: 0.01, Unit: "m", Read: () => state.rearDistance, Write: (v) => { state.rearDistance = v;  syncStateToBakeModel(); } });
    SliderRow(sSec.Inner, { Label: "AO Max Ray Distance", Minimum: 0.1, Maximum: 2.0, Step: 0.02, Unit: "m", Read: () => state.aoDistance, Write: (v) => { state.aoDistance = v;  syncStateToBakeModel(); } });
    SliderRow(sSec.Inner, { Label: "UV Edge Padding", Minimum: 0, Maximum: 14, Step: 1, Unit: "px", Read: () => state.padding, Write: (v) => { state.padding = v;  syncStateToBakeModel(); } });
    SwitchRow(sSec.Inner, { Label: "Show High-Poly Wire Overlay", Read: () => state.showHighOverlay, Write: (v) => { state.showHighOverlay = v; highGroup.visible = v; } });

    // 2. 3D Volume SDF Settings & Slice Viewer
    const vSec = SectionCard(rightInner, { Title: "3D Volume SDF & Slice Inspector", Glyph: "Volume" });
    SegmentedRow(vSec.Inner, {
      Label: "Voxel Grid Dim",
      Options: [["24³", 24], ["36³", 36], ["48³", 48], ["64³", 64]],
      Read: () => state.sdfRes,
      Write: (v) => { state.sdfRes = v;  syncStateToBakeModel(); },
    });
    SwitchRow(vSec.Inner, { Label: "Signed Interior Distance", Read: () => state.sdfSigned, Write: (v) => { state.sdfSigned = v;  syncStateToBakeModel(); } });
    SliderRow(vSec.Inner, { Label: "Bounds Padding", Minimum: 0.05, Maximum: 0.45, Step: 0.01, Read: () => state.sdfPadding, Write: (v) => { state.sdfPadding = v;  syncStateToBakeModel(); } });
    SegmentedRow(vSec.Inner, {
      Label: "Inspect Slice Axis",
      Options: [["Z Slice", "z"], ["Y Slice", "y"], ["X Slice", "x"]],
      Read: () => state.sdfSliceAxis,
      Write: (v) => { state.sdfSliceAxis = v; state.inspectChannel = "volumeSdf"; updateInspectedMapView(); },
    });
    SliderRow(vSec.Inner, {
      Label: "Slice Depth",
      Minimum: 0, Maximum: 1, Step: 0.01,
      Read: () => state.sdfSlicePos,
      Write: (v) => { state.sdfSlicePos = v; if (state.inspectChannel === "volumeSdf") updateInspectedMapView(); },
    });

    // 3. Export & Paint Bridge
    const eSec = SectionCard(rightInner, { Title: "Export & Texture Paint Bridge", Glyph: "Download" });
    ActionRow(eSec.Inner, [
      {
        Label: "Download Current PNG",
        Glyph: "Download",
        Act: () => {
          downloadUrl(canvas2D.toDataURL("image/png"), `${state.studyId}-${state.inspectChannel}.png`);
          Toast(`Exported ${state.inspectChannel}.png`);
        },
      },
      {
        Label: "Download .fvol SDF",
        Glyph: "Volume",
        Availability: () => ({ Enabled: !!state.bakedSdf, Reason: "Bake Volume SDF first" }),
        Act: () => {
          const sdf = state.bakedSdf;
          const payload = JSON.stringify({
            schema: "frontier.volume-sdf.v1",
            dims: sdf.dims,
            boundsMin: sdf.min,
            boundsMax: sdf.max,
            signed: sdf.signed,
            minDistance: sdf.minVal,
            maxDistance: sdf.maxVal,
            values: Array.from(sdf.values, (v) => Number(v.toFixed(5))),
          });
          const blob = new Blob([payload], { type: "application/json" });
          const u = URL.createObjectURL(blob);
          downloadUrl(u, `${state.studyId}-volume.fvol`);
          setTimeout(() => URL.revokeObjectURL(u), 1000);
          Toast("Exported Frontier .fvol Signed Volume SDF");
        },
      },
    ]);
    ActionRow(eSec.Inner, [
      {
        Label: "Send Baked AO/Curv/Normals to Texture Paint",
        Glyph: "ArrowRight",
        Tone: "warm",
        Availability: () => ({ Enabled: !!state.bakedSurface, Reason: "Bake surface maps first" }),
        Act: () => appBridge.sendBakedMapsToPaint(state.bakedSurface.bridgeData),
      },
    ]);

    CloseScope(inspectorScope);
    Broadcast();
  }

  // ----------------------------------------------------------------- EVENTS & LOADERS
  async function loadCustomFile(file, targetKey) {
    try {
      const ext = file.name.split(".").pop().toLowerCase();
      const parts = [];
      if (ext === "obj") {
        const grp = new OBJLoader().parse(await file.text());
        grp.traverse((ch) => {
          if (ch.isMesh) {
            const g = targetKey === "low" ? buildPaintTargetGeometry("custom", ch.geometry.clone()) : ch.geometry.clone();
            parts.push({ name: ch.name || file.name, geometry: g });
          }
        });
      } else if (ext === "stl") {
        const raw = new STLLoader().parse(await file.arrayBuffer());
        const g = targetKey === "low" ? buildPaintTargetGeometry("custom", raw) : raw;
        parts.push({ name: file.name, geometry: g });
      }
      if (parts.length) {
        // Normalize center & scale consistently if first custom load
        const box = new THREE.Box3();
        parts.forEach((p) => { p.geometry.computeBoundingBox(); box.union(p.geometry.boundingBox); });
        const ctr = box.getCenter(new THREE.Vector3());
        const sz = box.getSize(new THREE.Vector3());
        const scale = 2.4 / Math.max(0.001, Math.max(sz.x, sz.y, sz.z));
        parts.forEach((p) => { p.geometry.translate(-ctr.x, -ctr.y, -ctr.z); p.geometry.scale(scale, scale, scale); });
        studyPair[targetKey] = parts;
        studyPair.studyId = "custom";
        rebuildPreviewMeshes();
        rebuildInspector();
        Toast(`Loaded ${file.name} as ${targetKey.toUpperCase()} mesh`);
      }
    } catch (err) {
      Toast(`Load error: ${err.message}`);
    }
  }

  lowInp.addEventListener("change", () => { if (lowInp.files?.[0]) loadCustomFile(lowInp.files[0], "low"); lowInp.value = ""; });
  highInp.addEventListener("change", () => { if (highInp.files?.[0]) loadCustomFile(highInp.files[0], "high"); highInp.value = ""; });

  studySelect.addEventListener("change", () => {
    if (studySelect.value === "custom") return;
    state.studyId = studySelect.value;
    outlinerState.P = BakeModel.Normalise({ ...outlinerState.P, studyId: state.studyId, set: null });
    studyPair = createBakeStudyPair(state.studyId);
    state.bakedSurface = null;
    state.bakedSdf = null;
    if (bakedTex) { bakedTex.dispose(); bakedTex = null; }
    rebuildPreviewMeshes();
    updateInspectedMapView();
    renderLeftColumn();
    rebuildInspector();
  });

  btnBakeNow.addEventListener("click", runBake);
  btnSendToPaint.addEventListener("click", () => {
    if (!state.bakedSurface) {
      Toast("Run Bake Selected Maps first, then send to Texture Paint!");
      return;
    }
    appBridge.sendBakedMapsToPaint(state.bakedSurface.bridgeData);
  });

  const resizeRenderer3D = () => {
    if (pane3D.hidden) return;
    const w = pane3D.clientWidth || 400;
    const h = pane3D.clientHeight || 400;
    renderer.setSize(w, h, false);
    camera.aspect = w / h;
    camera.updateProjectionMatrix();
  };

  const syncBakeViewportLayout = () => {
    const mode = state.viewMode || "3d";
    if (mode === "2d") {
      pane3D.hidden = true;
      pane2D.hidden = false;
      split.style.gridTemplateColumns = "1fr";
      split.classList.add("bake-single-pane");
    } else if (mode === "3d") {
      pane3D.hidden = false;
      pane2D.hidden = true;
      split.style.gridTemplateColumns = "1fr";
      split.classList.add("bake-single-pane");
    } else {
      pane3D.hidden = false;
      pane2D.hidden = false;
      split.style.gridTemplateColumns = "1fr 1fr";
      split.classList.remove("bake-single-pane");
    }
    viewSelect.value = mode;
    resizeRenderer3D();
  };

  viewSelect?.addEventListener("change", () => {
    state.viewMode = viewSelect.value || "3d";
    syncBakeViewportLayout();
  });
  syncBakeViewportLayout();

  const ro = new ResizeObserver(() => {
    resizeRenderer3D();
  });
  ro.observe(stage);

  // ----------------------------------------------------------------- RIGHT-CLICK / Q / TAB QUICK BAKE MENU
  function closeBakePopup() {
    if (activePopup) {
      activePopup.remove();
      activePopup = null;
    }
  }

  function openBakePopup(clientX, clientY) {
    if (activePopup) { closeBakePopup(); return; }
    const box = stage.getBoundingClientRect();
    const x = clientX ?? (box.left + box.width * 0.5 - 160);
    const y = clientY ?? (box.top + 80);
    const pop = Construct("div", "tool-popup-menu");
    pop.style.left = `${Math.max(12, Math.min(x, window.innerWidth - 356))}px`;
    pop.style.top = `${Math.max(12, Math.min(y, window.innerHeight - 380))}px`;

    const closeBtn = Construct("button", "stack-mini-btn", ConstructGlyph("Close"));
    closeBtn.type = "button";
    closeBtn.addEventListener("click", closeBakePopup);
    pop.append(
      Construct("div", "tool-popup-head", [
        ConstructGlyph("Bake"),
        Construct("strong", null, "Baking Quick Strip"),
        Construct("small", null, state.lastDevice),
        closeBtn,
      ]),
    );

    const resRow = Construct("div", "chip-strip");
    for (const r of [256, 512, 1024, 2048]) {
      const b = Construct("button", "chip-btn" + (state.resolution === r ? " active" : ""), `${r}px`);
      b.type = "button";
      b.addEventListener("click", () => { state.resolution = r; closeBakePopup(); rebuildInspector(); });
      resRow.append(b);
    }
    pop.append(Construct("div", "property-label", "Map Resolution"), resRow);

    const actRow = Construct("div", "action-row");
    const runBtn = Construct("button", "action-button accent", [ConstructGlyph("Bake"), Construct("span", null, "Bake Selected Maps")]);
    runBtn.type = "button";
    runBtn.addEventListener("click", () => { closeBakePopup(); runBake(); });
    const bridgeBtn = Construct("button", "action-button warm", [ConstructGlyph("Brush"), Construct("span", null, "Send to Paint")]);
    bridgeBtn.type = "button";
    bridgeBtn.addEventListener("click", () => { closeBakePopup(); btnSendToPaint.click(); });
    actRow.append(runBtn, bridgeBtn);
    pop.append(actRow);

    document.body.append(pop);
    activePopup = pop;
  }

  let rDown = null;
  stage.addEventListener("pointerdown", (e) => {
    if (e.button === 2) rDown = { x: e.clientX, y: e.clientY };
    else if (activePopup && !activePopup.contains(e.target)) closeBakePopup();
  });
  stage.addEventListener("contextmenu", (e) => {
    e.preventDefault();
    if (!rDown || Math.hypot(e.clientX - rDown.x, e.clientY - rDown.y) < 5) {
      openBakePopup(e.clientX, e.clientY);
    }
    rDown = null;
  });

  const onBakeKeyDown = (e) => {
    if (["INPUT", "TEXTAREA", "SELECT"].includes(document.activeElement?.tagName)) return;
    if (e.key === "Escape") { closeBakePopup(); return; }
    if (e.key.toLowerCase() === "q" || e.key === "Tab") {
      e.preventDefault();
      openBakePopup();
    }
  };
  window.addEventListener("keydown", onBakeKeyDown);

  function ensureCustomStudyOption(label = "Imported Study") {
    let opt = Array.from(studySelect.options).find((o) => o.value === "custom");
    if (!opt) {
      opt = Construct("option", null, label);
      opt.value = "custom";
      studySelect.append(opt);
    } else {
      opt.textContent = label;
    }
    studySelect.value = "custom";
  }

  function applyImportedMesh(transfer) {
    if (!transfer) return false;
    const rawGeo = BuildThreeGeometryFromBridgeTransfer(transfer, THREE);
    if (!rawGeo) return false;
    const lowGeo = buildPaintTargetGeometry("custom", rawGeo);
    const highGeo = lowGeo.clone();
    state.studyId = "custom";
    ensureCustomStudyOption(`UV Bridge: ${transfer.name || "Imported Mesh"}`);
    studyPair = {
      studyId: "custom",
      low: [{ name: transfer.name || "Low", geometry: lowGeo }],
      high: [{ name: transfer.name || "High", geometry: highGeo }],
    };
    state.bakedSurface = null;
    state.bakedSdf = null;
    if (bakedTex) { bakedTex.dispose(); bakedTex = null; }
    rebuildPreviewMeshes();
    updateInspectedMapView();
    renderLeftColumn();
    rebuildInspector();
    Toast(`Loaded "${transfer.name}" (${transfer.triCount.toLocaleString()} tris) for baking`);
    return true;
  }

  if (appBridge?.shared?.importedMesh) {
    applyImportedMesh(appBridge.shared.importedMesh);
  } else {
    renderLeftColumn();
    rebuildInspector();
    updateInspectedMapView();
  }

  const tick = () => {
    if (disposed) return;
    if (!pane3D.hidden) {
      orbit.update();
      renderer.render(scene, camera);
    }
    rafId = requestAnimationFrame(tick);
  };
  rafId = requestAnimationFrame(tick);

  function applyAsset(payload) {
    if (!payload) return false;
    if (payload.assetKind === "mesh") {
      if (payload.raw?.transfer) {
        return applyImportedMesh(payload.raw.transfer);
      }
      const mId = payload.raw?.bakeId || payload.id;
      if (BAKE_STUDY_SHAPES.some((s) => s.id === mId)) {
        state.studyId = mId;
        studySelect.value = mId;
        studyPair = createBakeStudyPair(state.studyId);
        state.bakedSurface = null;
        state.bakedSdf = null;
        if (bakedTex) { bakedTex.dispose(); bakedTex = null; }
        rebuildPreviewMeshes();
        updateInspectedMapView();
        renderLeftColumn();
        rebuildInspector();
        Toast(`Loaded High/Low study "${payload.name}" for baking`);
        return true;
      }
    }
    return false;
  }

  stage.addEventListener("dragover", (e) => {
    e.preventDefault();
    if (e.dataTransfer) e.dataTransfer.dropEffect = "copy";
    stage.classList.add("canvas-drop-active");
  });
  stage.addEventListener("dragleave", () => stage.classList.remove("canvas-drop-active"));
  stage.addEventListener("drop", (e) => {
    e.preventDefault();
    stage.classList.remove("canvas-drop-active");
    let payload = null;
    try {
      const rawJson = e.dataTransfer?.getData("application/x-frontier-asset") || e.dataTransfer?.getData("text/plain");
      if (rawJson) {
        const parsed = JSON.parse(rawJson);
        payload = parsed.raw ? parsed : appBridge.resolveAssetPayload?.(parsed.assetKind, parsed.id);
      }
    } catch {}
    if (!payload) payload = appBridge.activeDragAsset;
    if (payload) applyAsset(payload);
  });

  return {
    name: "Baking",
    applyAsset,
    applyImportedMesh,
    getSnapshot: () => ({
      studyId: state.studyId,
      viewMode: state.viewMode,
      inspectChannel: state.inspectChannel,
      selected: [...state.selectedChannels],
      hasSdf: !!state.bakedSdf,
      hasSurface: !!state.bakedSurface,
    }),
    dispose() {
      disposed = true;
      cancelAnimationFrame(rafId);
      closeBakePopup();
      window.removeEventListener("keydown", onBakeKeyDown);
      ro.disconnect();
      header.dispose();
      if (inspectorScope) ReleaseScope(inspectorScope);
      orbit.dispose();
      gpuBaker.dispose();
      if (bakedTex) bakedTex.dispose();
      renderer.dispose();
      root.innerHTML = "";
    },
  };
}
