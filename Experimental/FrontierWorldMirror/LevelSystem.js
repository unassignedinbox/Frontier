//==========================================================================================================================================
//                                                        LEVELSYSTEM.JS
//==========================================================================================================================================
// Multi-Level persistence, built-in Frontier levels, and Create Level template generator for FrontierWorldMirror.

import {
  BuildMaterialRecord,
  CameraFlareDefaults,
  CameraTonemapDefaults,
  CanonicalEditorCamera,
  CanonicalRenderSettings,
  ResolveRenderSettings,
  ShowcaseInitialRows,
  ShowcaseInitialValues,
} from "./ShowcaseGalleryScene.js";
import { BuildDefaultDiagnosticForceRig } from "./ForceFieldCatalog.js";

export const LEVEL_STORAGE_KEY = "Frontier.WorldMirrorLevels.v7";

const Clone = (Value) => JSON.parse(JSON.stringify(Value));

// Canonical component defaults for the Editor Camera's Lens Flare &
// Tonemapping sub-components live in ShowcaseGalleryScene.js (imported above).

export function EnsureRenderSettingsInLevel(Lvl) {
  if (!Lvl || !Array.isArray(Lvl.Rows)) return Lvl;
  let Rows = Lvl.Rows;
  const HasRenderSettings = Rows.some((R) => R.Id === "render-settings");
  if (!HasRenderSettings) {
    const CamIdx = Rows.findIndex((R) => R.Id === "camera");
    const RenderRow = {
      Id: "render-settings",
      Name: "Render Settings",
      Panel: "render-settings",
      Icon: "render-settings",
      Parent: "camera",
      Description: "Shadows, GI & Reflections · Per-pipeline quality",
    };
    Rows = [...Rows];
    if (CamIdx >= 0) Rows.splice(CamIdx + 1, 0, RenderRow);
    else Rows.push(RenderRow);
  }
  // Editor camera carries two components (Blueprint-style sub-entries):
  // Lens Flare and Tonemapping. Injected into every level so the camera's
  // component model is complete regardless of template.
  const CameraSubs = [
    {
      Id: "camera-flare",
      Name: "Lens Flare",
      Panel: "flare",
      Icon: "lens-flare",
      Parent: "camera",
      Description: "Editor camera lens flare component",
    },
    {
      Id: "camera-tonemap",
      Name: "Tonemapping",
      Panel: "post",
      Icon: "environment-exposure",
      Parent: "camera",
      Description: "Editor camera tonemapping component",
    },
  ].filter((Sub) => !Rows.some((R) => R.Id === Sub.Id));
  if (CameraSubs.length) {
    const CamIdx = Rows.findIndex((R) => R.Id === "camera");
    Rows = [...Rows];
    if (CamIdx >= 0) Rows.splice(CamIdx + 1, 0, ...CameraSubs);
    else Rows.push(...CameraSubs);
  }
  let Values = {
    ...(Lvl.Values || {}),
    "render-settings": ResolveRenderSettings(Lvl.Values),
  };
  if (!Values["camera-flare"]) {
    Values["camera-flare"] = Clone(CameraFlareDefaults);
  }
  if (!Values["camera-tonemap"]) {
    Values["camera-tonemap"] = Clone(CameraTonemapDefaults);
  }
  if (Lvl.Id === "level-showcase-gallery" && !Rows.some((R) => R.Id === "control-vehicle")) {
    const VehicleRow = ShowcaseInitialRows.find((R) => R.Id === "control-vehicle");
    if (VehicleRow) {
      Rows = [...Rows];
      const SphereIdx = Rows.findIndex((R) => R.Id === "abbe-sphere");
      if (SphereIdx >= 0) Rows.splice(SphereIdx + 1, 0, Clone(VehicleRow));
      else Rows.push(Clone(VehicleRow));
      if (!Values["control-vehicle"] && ShowcaseInitialValues["control-vehicle"]) {
        Values["control-vehicle"] = Clone(ShowcaseInitialValues["control-vehicle"]);
      }
    }
  }
  for (const EnvId of ["atmosphere", "clouds", "moon", "stars"]) {
    if (!Rows.some((R) => R.Panel === EnvId)) {
      const EnvRow = ShowcaseInitialRows.find((R) => R.Id === EnvId);
      if (EnvRow) {
        Rows = [...Rows];
        const SunIdx = Rows.findIndex((R) => R.Panel === "sun" || R.Id === "world");
        if (SunIdx >= 0) Rows.splice(SunIdx + 1, 0, Clone(EnvRow));
        else Rows.push(Clone(EnvRow));
      }
    }
    if (!Values[EnvId] && ShowcaseInitialValues[EnvId]) {
      Values[EnvId] = Clone(ShowcaseInitialValues[EnvId]);
    }
  }
  return { ...Lvl, Rows, Values };
}

function BuildLookDevStudioLevel() {
  const Rows = [
    { Id: "scene", Name: "Look-Dev Studio", Panel: "group", Icon: "folder-scene", Parent: null, Description: "Material Look-Dev collection" },
    {
      Id: "studio-floor",
      Name: "Slate Cyclorama Floor",
      PrimitiveShape: "box",
      Panel: "geometry",
      Icon: "editor-cube",
      Parent: "scene",
      Preview: true,
      Description: "Matte charcoal studio floor · 6.40 × 6.40 m",
    },
    {
      Id: "studio-backdrop",
      Name: "Studio Backdrop Wall",
      PrimitiveShape: "box",
      Panel: "geometry",
      Icon: "editor-cube",
      Parent: "scene",
      Preview: true,
      Description: "Neutral mid-grey backdrop · 6.40 × 3.00 m",
    },
    {
      Id: "titanium-plinth",
      Name: "Brushed Titanium Plinth",
      PrimitiveShape: "cylinder",
      Panel: "geometry",
      Icon: "editor-cylinder",
      Parent: "scene",
      Preview: true,
      Description: "Anisotropic conductor pedestal · R 0.68 m",
    },
    {
      Id: "hero-clearcoat-orb",
      Name: "Cobalt Clear-Coat Sphere",
      PrimitiveShape: "sphere",
      Panel: "geometry",
      Icon: "editor-sphere",
      Parent: "scene",
      Preview: true,
      Description: "Dual-layer automotive clear-coat hero sphere",
    },
    {
      Id: "velvet-torus",
      Name: "Crimson Velvet Torus",
      PrimitiveShape: "torus",
      Panel: "geometry",
      Icon: "editor-torus",
      Parent: "scene",
      Preview: true,
      Description: "Fabric Charlie sheen specimen",
    },
    {
      Id: "jade-subsurface-orb",
      Name: "Subsurface Jade Orb",
      PrimitiveShape: "sphere",
      Panel: "geometry",
      Icon: "editor-sphere",
      Parent: "scene",
      Preview: true,
      Description: "Wrap-lit subsurface scattering specimen",
    },
    { Id: "cameras", Name: "Cameras", Panel: "group", Icon: "camera", Parent: null, Description: "Editor cameras" },
    { Id: "camera", Name: "Editor Camera", Panel: "camera", Icon: "camera", Parent: "cameras", Description: "Permanent editor navigation camera" },
    { Id: "post", Name: "ACES Post-Process", Panel: "post", Icon: "environment-exposure", Parent: "cameras", Description: "Exposure & tonemapping" },
    { Id: "world", Name: "World", Panel: "group", Icon: "folder-world", Parent: null, Description: "Solar & sky environment" },
    { Id: "sun", Name: "Key Rim Sun", Panel: "sun", Icon: "sun", Parent: "world", Preview: true, Description: "Directional · 4.2 W/m²" },
    { Id: "lighting", Name: "Lighting", Panel: "group", Icon: "folder-generic", Parent: null, Description: "Studio LTC area lights" },
    {
      Id: "key-softbox",
      Name: "Studio Overhead Softbox",
      Panel: "light",
      Icon: "editor-area-light",
      Parent: "lighting",
      Preview: true,
      Description: "LTC Quad · 2.10 × 1.50 m",
    },
  ];

  const VelvetMat = BuildMaterialRecord({
    Type: "Fabric",
    BaseLinear: [0.38, 0.04, 0.09],
    Roughness: 0.82,
  });
  VelvetMat.Material.Channels.sheen = { Source: "Fill", Fill: "#f09ab0" };
  VelvetMat.Material.Channels.sheenRoughness = { Source: "Fill", Fill: 0.45 };

  const JadeMat = BuildMaterialRecord({
    Type: "Subsurface",
    BaseLinear: [0.14, 0.52, 0.36],
    Roughness: 0.26,
  });
  JadeMat.Material.Channels.subsurface = { Source: "Fill", Fill: "#4be0a2" };
  JadeMat.Material.Channels.thickness = { Source: "Fill", Fill: 0.35 };

  const Values = {
    "studio-floor": {
      Position: [0, 0.3, -0.68],
      Rotation: [0, 0, 0],
      Scale: [6.4, 6.4, 0.16],
      "Cast shadows": true,
      GI: true,
      ...BuildMaterialRecord({ Type: "Standard", BaseLinear: [0.16, 0.17, 0.19], Roughness: 0.46 }),
    },
    "studio-backdrop": {
      Position: [0, 2.55, 0.82],
      Rotation: [0, 0, 0],
      Scale: [6.4, 0.18, 3.0],
      "Cast shadows": true,
      GI: true,
      ...BuildMaterialRecord({ Type: "Standard", BaseLinear: [0.26, 0.28, 0.32], Roughness: 0.62 }),
    },
    "titanium-plinth": {
      Position: [0, 0.45, -0.36],
      Rotation: [0, 0, 0],
      Scale: [1.35, 1.35, 0.48],
      "Cast shadows": true,
      GI: true,
      ...BuildMaterialRecord({ Type: "Metal", BaseLinear: [0.66, 0.69, 0.74], Metallic: 1.0, Roughness: 0.22, Anisotropy: 0.65 }),
    },
    "hero-clearcoat-orb": {
      Position: [0, 0.45, 0.38],
      Rotation: [0, 0, 0],
      Scale: [1.02, 1.02, 1.02],
      "Cast shadows": true,
      GI: true,
      ...BuildMaterialRecord({ Type: "Clear coat", BaseLinear: [0.06, 0.24, 0.86], Metallic: 0.25, Roughness: 0.28, Coat: 1.0, CoatRoughness: 0.04 }),
    },
    "velvet-torus": {
      Position: [-1.18, 0.35, -0.22],
      Rotation: [28, -18, 15],
      Scale: [0.92, 0.92, 0.56],
      "Cast shadows": true,
      GI: true,
      ...VelvetMat,
    },
    "jade-subsurface-orb": {
      Position: [1.16, 0.35, -0.2],
      Rotation: [0, 0, 0],
      Scale: [0.84, 0.84, 0.84],
      "Cast shadows": true,
      GI: true,
      ...JadeMat,
    },
    camera: {
      Position: [0.0, -3.15, 0.48],
      Target: [0.0, 0.42, 0.08],
      "Focal Length": 30.0,
      "Sensor Width": 36.0,
      Aperture: 4.0,
      "Subject Distance": 3.6,
    },
    sun: {
      Direction: [-0.48, -0.52, 0.70],
      SolarIrradiance: [4.2, 4.05, 3.9],
      SkyRadiance: [0.14, 0.20, 0.34],
      Intensity: 4.2,
      "Angular Diameter": 2.4,
      Temperature: 5600,
    },
    "key-softbox": {
      Position: [0.25, -0.15, 1.95],
      Rotation: [0, 0, 0],
      Scale: [2.1, 1.5, 1.0],
      Intensity: 8.8,
      EmittedRadiance: [8.8, 8.2, 7.6],
      ReferenceInspector: {
        Type: "LuminousSurface",
        Properties: {
          lumens: 2800,
          dimmer: 1,
          color: "#fff6ec",
          temperature: 5500,
          useTemp: false,
          width: 2.1,
          height: 1.5,
          pos: [0.25, -0.15, 1.95],
          rot: [0, 0, 0],
        },
      },
    },
  };

  return {
    Id: "level-lookdev-studio",
    Name: "Look-Dev Plinth Studio",
    Notes: "Three-specimen OpenPBR material stage (Clear-Coat, Charlie Velvet Sheen, Subsurface Jade).",
    Rows,
    Values,
    Hidden: {},
    Collapsed: {},
    Selected: "hero-clearcoat-orb",
  };
}

function BuildEmptyProvingGroundLevel(Id = "level-proving-ground", Name = "Empty Proving Ground", Notes = "") {
  const Rows = [
    { Id: "scene", Name, Panel: "group", Icon: "folder-scene", Parent: null, Description: "Level collection" },
    {
      Id: "ground-plane",
      Name: "Proving Ground Slab",
      PrimitiveShape: "box",
      Panel: "geometry",
      Icon: "editor-cube",
      Parent: "scene",
      Preview: true,
      Description: "Architectural reference slab · 8.00 × 8.00 m",
    },
    {
      Id: "starter-cube",
      Name: "Starter Blockout Cube",
      PrimitiveShape: "box",
      Panel: "geometry",
      Icon: "editor-cube",
      Parent: "scene",
      Preview: true,
      Description: "Origin 1 m calibration cube",
    },
    { Id: "cameras", Name: "Cameras", Panel: "group", Icon: "camera", Parent: null, Description: "Editor cameras" },
    { Id: "camera", Name: "Editor Camera", Panel: "camera", Icon: "camera", Parent: "cameras", Description: "Permanent editor navigation camera" },
    { Id: "post", Name: "ACES Post-Process", Panel: "post", Icon: "environment-exposure", Parent: "cameras", Description: "Exposure & tonemapping" },
    { Id: "world", Name: "World", Panel: "group", Icon: "folder-world", Parent: null, Description: "Solar environment" },
    { Id: "sun", Name: "Sun", Panel: "sun", Icon: "sun", Parent: "world", Preview: true, Description: "Directional · 5.4 W/m²" },
  ];

  const Values = {
    scene: { Notes: Notes || "Clean architectural proving ground level." },
    "ground-plane": {
      Position: [0, 0.4, -0.68],
      Rotation: [0, 0, 0],
      Scale: [8.0, 8.0, 0.16],
      "Cast shadows": true,
      GI: true,
      ...BuildMaterialRecord({ Type: "Standard", BaseLinear: [0.58, 0.59, 0.61], Roughness: 0.68 }),
    },
    "starter-cube": {
      Position: [0, 0.4, -0.1],
      Rotation: [0, 0, 18],
      Scale: [1.0, 1.0, 1.0],
      "Cast shadows": true,
      GI: true,
      ...BuildMaterialRecord({ Type: "Standard", BaseLinear: [0.84, 0.56, 0.24], Roughness: 0.34 }),
    },
    camera: {
      Position: [...CanonicalEditorCamera.Position],
      Target: [...CanonicalEditorCamera.Target],
      "Focal Length": CanonicalEditorCamera.FocalLengthMm,
      "Sensor Width": CanonicalEditorCamera.SensorWidthMm,
      Aperture: CanonicalEditorCamera.ApertureFStop,
      "Subject Distance": CanonicalEditorCamera.SubjectDistanceM,
    },
    sun: {
      Direction: [0.45, -0.58, 0.68],
      SolarIrradiance: [5.4, 5.1, 4.65],
      SkyRadiance: [0.18, 0.26, 0.40],
      Intensity: 5.4,
      "Angular Diameter": 2.06,
      Temperature: 5650,
    },
  };

  return {
    Id,
    Name,
    Notes: Notes || "Clean architectural blockout level.",
    Rows,
    Values,
    Hidden: {},
    Collapsed: {},
    Selected: "starter-cube",
  };
}

export function BuildDefaultLevels() {
  return [
    EnsureRenderSettingsInLevel(BuildLobbyLevel()),
    EnsureRenderSettingsInLevel({
      Id: "level-showcase-gallery",
      Name: "Cornell-Studio Gallery",
      Notes: "Canonical C++ <-> HTML Architectural Cornell-Studio Gallery with ControlVehicle.",
      Rows: Clone(ShowcaseInitialRows),
      Values: Clone(ShowcaseInitialValues),
      Hidden: {},
      Collapsed: {},
      Selected: "chrome-sphere",
    }),
    EnsureRenderSettingsInLevel(BuildLookDevStudioLevel()),
    EnsureRenderSettingsInLevel(BuildEmptyProvingGroundLevel()),
  ];
}

// ---------------------------------------------------------------------------------------------------------------------------------
// LOBBY · default Frontier level: architectural atrium with ControlVehicle + Atrium Display Aircraft
// ---------------------------------------------------------------------------------------------------------------------------------
function BuildLobbyLevel(
  CustomId = "level-lobby",
  CustomName = "Frontier Lobby",
  CustomNotes = "",
) {
  const AircraftScale = 0.72;
  const Rows = [
    {
      Id: "scene",
      Name: CustomName,
      Panel: "group",
      Icon: "folder-scene",
      Parent: null,
      Description: "Frontier lobby collection",
    },
    {
      Id: "lobby-floor",
      Name: "Lobby Travertine Floor",
      PrimitiveShape: "box",
      Panel: "geometry",
      Icon: "editor-cube",
      Parent: "scene",
      Preview: true,
      Description: "Honed travertine floor slab · 12.00 × 10.00 m",
    },
    {
      Id: "lobby-display-rug",
      Name: "Lobby Display Rug",
      PrimitiveShape: "box",
      Panel: "geometry",
      Icon: "editor-cube",
      Parent: "scene",
      Preview: true,
      Description: "Woven charcoal display rug · 10.40 × 6.40 m",
    },
    {
      Id: "lobby-back-wall-west",
      Name: "Lobby Plaster Wall West",
      PrimitiveShape: "box",
      Panel: "geometry",
      Icon: "editor-cube",
      Parent: "scene",
      Preview: true,
      Description: "Warm lime plaster backdrop, west of the portal · 3.80 × 4.00 m",
    },
    {
      Id: "lobby-back-wall-east",
      Name: "Lobby Plaster Wall East",
      PrimitiveShape: "box",
      Panel: "geometry",
      Icon: "editor-cube",
      Parent: "scene",
      Preview: true,
      Description: "Warm lime plaster backdrop, east of the portal · 3.80 × 4.00 m",
    },
    {
      Id: "lobby-back-header",
      Name: "Lobby Portal Header",
      PrimitiveShape: "box",
      Panel: "geometry",
      Icon: "editor-cube",
      Parent: "scene",
      Preview: true,
      Description: "Plaster header closing the glazed portal · 4.40 × 0.57 m",
    },
    {
      Id: "lobby-west-wall",
      Name: "Lobby Basalt West Wall",
      PrimitiveShape: "box",
      Panel: "geometry",
      Icon: "editor-cube",
      Parent: "scene",
      Preview: true,
      Description: "Dark basalt side wall · 10.00 × 4.00 m",
    },
    {
      Id: "lobby-east-wall",
      Name: "Lobby Basalt East Wall",
      PrimitiveShape: "box",
      Panel: "geometry",
      Icon: "editor-cube",
      Parent: "scene",
      Preview: true,
      Description: "Dark basalt side wall · 10.00 × 4.00 m",
    },
    {
      Id: "lobby-glazed-portal",
      Name: "Lobby Glazed Portal",
      PrimitiveShape: "box",
      Panel: "geometry",
      Icon: "editor-cube",
      Parent: "scene",
      Preview: true,
      Description: "Structural glazing · 4.40 × 2.90 m · IOR 1.52, Abbe 26",
    },
    {
      Id: "lobby-roof-coffer",
      Name: "Lobby Roof Coffer",
      PrimitiveShape: "box",
      Panel: "geometry",
      Icon: "editor-cube",
      Parent: "scene",
      Preview: true,
      Description: "Suspended ceiling coffer over the reception bay · 12.00 × 2.60 m",
    },
    {
      Id: "lobby-west-column",
      Name: "Lobby West Column",
      PrimitiveShape: "cylinder",
      Panel: "geometry",
      Icon: "editor-cylinder",
      Parent: "scene",
      Preview: true,
      Description: "Brushed stainless column · Ø 0.48 m × 4.00 m",
    },
    {
      Id: "lobby-east-column",
      Name: "Lobby East Column",
      PrimitiveShape: "cylinder",
      Panel: "geometry",
      Icon: "editor-cylinder",
      Parent: "scene",
      Preview: true,
      Description: "Brushed stainless column · Ø 0.48 m × 4.00 m",
    },
    {
      Id: "lobby-reception-desk",
      Name: "Lobby Reception Desk",
      PrimitiveShape: "box",
      Panel: "geometry",
      Icon: "editor-cube",
      Parent: "scene",
      Preview: true,
      Description: "Walnut reception counter · 2.20 × 1.10 m",
    },
    {
      Id: "lobby-desk-counter",
      Name: "Reception Quartz Counter",
      PrimitiveShape: "box",
      Panel: "geometry",
      Icon: "editor-cube",
      Parent: "scene",
      Preview: true,
      Description: "Honed quartz worktop · 2.35 × 1.00 m",
    },
    {
      Id: "lobby-sign-pylon",
      Name: "Frontier Sign Pylon",
      PrimitiveShape: "box",
      Panel: "geometry",
      Icon: "editor-cube",
      Parent: "scene",
      Preview: true,
      Description: "Patinated bronze monolith · 1.05 × 1.95 m",
    },
    {
      Id: "lobby-sign-panel",
      Name: "Frontier Sign Light Panel",
      PrimitiveShape: "box",
      Panel: "geometry",
      Icon: "editor-cube",
      Parent: "scene",
      Preview: true,
      Description: "Backlit sign face · 0.78 × 1.35 m · 3.4 nit",
    },
    {
      Id: "control-vehicle",
      Name: "ControlVehicle",
      PrimitiveShape: "vehicle",
      Panel: "geometry",
      Icon: "vehicle",
      Parent: "scene",
      Preview: true,
      Description: "ControlVehicle BVH Mesh · Body + 4 Wheels/Rims · Clearcoat",
    },
    {
      Id: "lobby-aircraft",
      Name: "Atrium Display Aircraft",
      PropId: "lobby_aircraft",
      Panel: "geometry",
      Icon: "editor-mesh",
      Parent: "scene",
      Preview: true,
      Description: "High-wing monoplane · 6.16 × 4.68 × 1.84 m · 3-blade prop & smoked canopy",
    },
    { Id: "cameras", Name: "Cameras", Panel: "group", Icon: "camera", Parent: null, Description: "Editor cameras" },
    {
      Id: "camera",
      Name: "Editor Camera",
      Panel: "camera",
      Icon: "camera",
      Parent: "cameras",
      Description: "Permanent editor navigation camera · 30 mm · 43.0° FOV equivalent",
    },
    { Id: "world", Name: "World", Panel: "group", Icon: "folder-world", Parent: null, Description: "Solar & sky environment" },
    {
      Id: "sun",
      Name: "Sun",
      Panel: "sun",
      Icon: "sun",
      Parent: "world",
      Preview: true,
      Description: "Directional · late-afternoon solar emitter",
    },
    {
      Id: "atmosphere",
      Name: "Atmosphere",
      Panel: "atmosphere",
      Icon: "sky-scattering",
      Parent: "world",
      Description: "Rayleigh · Mie · Ozone physical sky",
    },
    {
      Id: "clouds",
      Name: "Volumetric Clouds",
      Panel: "clouds",
      Icon: "clouds",
      Parent: "world",
      Description: "3D raymarched cumulus & stratus slab",
    },
    { Id: "moon", Name: "Moon", Panel: "moon", Icon: "moon", Parent: "world", Description: "Synodic phase · lunar disc & glow" },
    { Id: "stars", Name: "Stars", Panel: "stars", Icon: "stars", Parent: "world", Description: "Deterministic celestial starfield" },
    { Id: "lighting", Name: "Lighting", Panel: "group", Icon: "folder-generic", Parent: null, Description: "Lobby LTC area lights" },
    {
      Id: "lobby-ceiling-light",
      Name: "Lobby Ceiling Lightfield",
      Panel: "light",
      Icon: "editor-area-light",
      Parent: "lighting",
      Preview: true,
      Description: "LTC Quad · 4.30 × 1.90 m · 6.4, 6.0, 5.3 nit",
    },
    {
      Id: "lobby-reception-light",
      Name: "Reception Downlight",
      Panel: "light",
      Icon: "editor-area-light",
      Parent: "lighting",
      Preview: true,
      Description: "LTC Quad · 1.60 × 1.00 m · 7.8, 6.6, 4.8 nit",
    },
  ];

  const Values = {
    scene: {
      Notes: CustomNotes || "Frontier lobby: travertine atrium with ControlVehicle and Atrium Display Aircraft.",
    },
    "lobby-floor": {
      Position: [0, 0.6, -0.68],
      Rotation: [0, 0, 0],
      Scale: [12.0, 10.0, 0.16],
      "Cast shadows": true,
      GI: true,
      Notes: "Honed travertine floor receiving Contact+PCSS solar shadows and lobby GI bounce.",
      ...BuildMaterialRecord({
        Type: "Clear coat",
        BaseLinear: [0.46, 0.42, 0.36],
        Roughness: 0.30,
        Coat: 0.45,
        CoatRoughness: 0.12,
      }),
    },
    "lobby-display-rug": {
      Position: [0, 1.3, -0.594],
      Rotation: [0, 0, 0],
      Scale: [10.40, 6.40, 0.012],
      "Cast shadows": true,
      GI: true,
      Notes: "Low-pile charcoal rug grounding both showroom exhibits.",
      ...BuildMaterialRecord({ Type: "Fabric", BaseLinear: [0.055, 0.058, 0.065], Roughness: 0.88 }),
    },
    "lobby-back-wall-west": {
      Position: [-4.1, 5.0, 1.4],
      Rotation: [0, 0, 0],
      Scale: [3.80, 0.18, 4.0],
      "Cast shadows": true,
      GI: true,
      Notes: "Warm plaster backdrop west of the glazed portal.",
      ...BuildMaterialRecord({ Type: "Standard", BaseLinear: [0.54, 0.51, 0.46], Roughness: 0.74 }),
    },
    "lobby-back-wall-east": {
      Position: [4.1, 5.0, 1.4],
      Rotation: [0, 0, 0],
      Scale: [3.80, 0.18, 4.0],
      "Cast shadows": true,
      GI: true,
      Notes: "Warm plaster backdrop east of the glazed portal.",
      ...BuildMaterialRecord({ Type: "Standard", BaseLinear: [0.54, 0.51, 0.46], Roughness: 0.74 }),
    },
    "lobby-back-header": {
      Position: [0, 5.0, 3.115],
      Rotation: [0, 0, 0],
      Scale: [4.40, 0.18, 0.57],
      "Cast shadows": true,
      GI: true,
      Notes: "Plaster header closing the top of the full-height glazed portal.",
      ...BuildMaterialRecord({ Type: "Standard", BaseLinear: [0.54, 0.51, 0.46], Roughness: 0.74 }),
    },
    "lobby-west-wall": {
      Position: [-6.0, 0.6, 1.4],
      Rotation: [0, 0, 0],
      Scale: [0.18, 10.0, 4.0],
      "Cast shadows": true,
      GI: true,
      Notes: "Dark basalt side wall driving cool diffuse colour bleed into the atrium.",
      ...BuildMaterialRecord({ Type: "Standard", BaseLinear: [0.15, 0.16, 0.18], Roughness: 0.52 }),
    },
    "lobby-east-wall": {
      Position: [6.0, 0.6, 1.4],
      Rotation: [0, 0, 0],
      Scale: [0.18, 10.0, 4.0],
      "Cast shadows": true,
      GI: true,
      Notes: "Dark basalt side wall mirroring the west enclosure.",
      ...BuildMaterialRecord({ Type: "Standard", BaseLinear: [0.15, 0.16, 0.18], Roughness: 0.52 }),
    },
    "lobby-glazed-portal": {
      Position: [0, 4.93, 1.115],
      Rotation: [0, 0, 0],
      Scale: [4.40, 0.12, 3.43],
      "Cast shadows": false,
      GI: false,
      Notes: "Structural glazing through which the physical sky and volumetric clouds are visible.",
      ...BuildMaterialRecord({
        Type: "Glass",
        BaseLinear: [0.93, 0.97, 1.0],
        Roughness: 0.02,
        Transmission: 1.0,
        Ior: 1.52,
        AbbeNumber: 26.0,
        AttenuationLinear: [0.88, 0.94, 0.97],
      }),
    },
    "lobby-roof-coffer": {
      Position: [0, 4.2, 3.02],
      Rotation: [0, 0, 0],
      Scale: [12.0, 2.60, 0.18],
      "Cast shadows": true,
      GI: true,
      Notes: "Suspended ceiling coffer over the reception bay; shades the back of the atrium.",
      ...BuildMaterialRecord({ Type: "Standard", BaseLinear: [0.34, 0.33, 0.31], Roughness: 0.66 }),
    },
    "lobby-west-column": {
      Position: [-2.60, 4.60, 1.4],
      Rotation: [0, 0, 0],
      Scale: [0.48, 0.48, 4.0],
      "Cast shadows": true,
      GI: true,
      Notes: "Brushed stainless column with anisotropic highlight streaks.",
      ...BuildMaterialRecord({
        Type: "Metal",
        BaseLinear: [0.70, 0.72, 0.76],
        Metallic: 1.0,
        Roughness: 0.18,
        Anisotropy: 0.5,
      }),
    },
    "lobby-east-column": {
      Position: [2.60, 4.60, 1.4],
      Rotation: [0, 0, 0],
      Scale: [0.48, 0.48, 4.0],
      "Cast shadows": true,
      GI: true,
      Notes: "Brushed stainless column with anisotropic highlight streaks.",
      ...BuildMaterialRecord({
        Type: "Metal",
        BaseLinear: [0.70, 0.72, 0.76],
        Metallic: 1.0,
        Roughness: 0.18,
        Anisotropy: 0.5,
      }),
    },
    "lobby-reception-desk": {
      Position: [-4.60, 4.35, -0.02],
      Rotation: [0, 0, 0],
      Scale: [2.20, 1.10, 1.10],
      "Cast shadows": true,
      GI: true,
      Notes: "Walnut reception counter with open coat sheen.",
      ...BuildMaterialRecord({
        Type: "Clear coat",
        BaseLinear: [0.24, 0.14, 0.075],
        Roughness: 0.30,
        Coat: 0.65,
        CoatRoughness: 0.10,
      }),
    },
    "lobby-desk-counter": {
      Position: [-4.60, 4.35, 0.56],
      Rotation: [0, 0, 0],
      Scale: [2.35, 1.00, 0.07],
      "Cast shadows": true,
      GI: true,
      Notes: "Honed quartz worktop capping the reception counter.",
      ...BuildMaterialRecord({
        Type: "Clear coat",
        BaseLinear: [0.84, 0.83, 0.80],
        Roughness: 0.16,
        Coat: 0.5,
        CoatRoughness: 0.08,
      }),
    },
    "lobby-sign-pylon": {
      Position: [4.60, 4.55, 0.36],
      Rotation: [0, 0, 0],
      Scale: [1.05, 0.32, 1.95],
      "Cast shadows": true,
      GI: true,
      Notes: "Patinated bronze monolith anchoring the west reception corner.",
      ...BuildMaterialRecord({
        Type: "Metal",
        BaseLinear: [0.30, 0.22, 0.14],
        Metallic: 1.0,
        Roughness: 0.34,
      }),
    },
    "lobby-sign-panel": {
      Position: [4.60, 4.37, 0.46],
      Rotation: [0, 0, 0],
      Scale: [0.80, 0.06, 1.40],
      "Cast shadows": false,
      GI: true,
      Notes: "Backlit Frontier sign face emitting warm-white light into the lobby.",
      ...BuildMaterialRecord({
        Type: "Unlit",
        BaseLinear: [1.0, 0.94, 0.84],
        Roughness: 0.3,
        Emission: 3.4,
      }),
    },
    "control-vehicle": {
      Position: [-2.80, 1.30, -0.522],
      Rotation: [0, 0, -18],
      Scale: [0.62, 0.62, 0.62],
      "Cast shadows": true,
      GI: true,
      Notes: "ControlVehicle BVH triangle mesh (Body + 4 Rims + 4 Tyres) displayed on the lobby floor.",
      ...BuildMaterialRecord({
        Type: "Clear coat",
        BaseLinear: [0.90, 0.92, 0.96],
        Metallic: 0.24,
        Roughness: 0.14,
        Coat: 1.0,
        CoatRoughness: 0.03,
      }),
    },
    "lobby-aircraft": {
      Position: [2.20, 1.30, -0.588 + 0.98 * AircraftScale],
      Rotation: [0, 0, 42],
      Scale: [AircraftScale, AircraftScale, AircraftScale],
      "Cast shadows": true,
      GI: true,
      Notes: "Atrium showroom aircraft: ivory airframe, cobalt livery, smoked canopy and chrome spinner.",
    },
    camera: {
      Position: [0.0, -6.40, 1.50],
      Target: [0.0, 0.50, 1.10],
      "Focal Length": 28.0,
      "Sensor Width": 36.0,
      Aperture: 4.0,
      "Subject Distance": 6.95,
      Speed: 1.0,
    },
    sun: {
      Direction: [0.58, -0.55, 0.60],
      SolarIrradiance: [5.6, 5.15, 4.6],
      SkyRadiance: [0.18, 0.25, 0.40],
      Intensity: 5.6,
      "Angular Diameter": 1.62,
      Temperature: 5450,
      "Colour source": 0,
      "Sun Tint": "#fff1d8",
      "Local Hours": 15.2,
      "Day of Month": 4,
      Month: 10,
      Latitude: -26.19,
      Longitude: 28.32,
      Animate: false,
      Speed: 1,
      Notes: "Late-afternoon solar emitter: direct sun plus the physical atmosphere transmittance model.",
    },
    atmosphere: {
      Rayleigh: 1.0,
      Mie: 1.15,
      "Mie Anisotropy": 0.78,
      Ozone: 1.2,
      "Rayleigh Scale H": 8000,
      "Mie Scale H": 1200,
      Atmosphere: 60000,
      "Horizon Glow": 1.15,
      "White Line": 1.0,
      "Line at civil only": true,
      "Sky Tint": "#eaf2ff",
      "Sky Brightness": 1.05,
      "Ground Albedo": "#796e60",
      Notes: "Bruneton/Hillaire physical Rayleigh, Mie, and Ozone sky atmosphere.",
    },
    clouds: {
      Enabled: true,
      Type: 0,
      Coverage: 0.34,
      Density: 1.9,
      "Feature Scale": 0.36,
      Base: 1200,
      Thickness: 900,
      Ceiling: 14000,
      "Follow Wind": true,
      Anvil: 0.42,
      Anisotropy: 0.55,
      "Shadow Enabled": true,
      Notes: "Real-time 3D volumetric raymarched clouds with Beer-Lambert self-shadowing.",
    },
    moon: {
      Visible: false,
      "Follow Sky": false,
      Phase: 0.62,
      Bright: 1.6,
      Glow: 0.8,
      Size: 1.35,
      Azimuth: 300,
      Elevation: 28,
      Roll: 0,
      Pitch: 0,
      Preset: 0,
      "moon0:Preset": 0,
      "moon0:Visible": false,
      "moon0:Follow Sky": false,
      "moon0:Phase": 0.62,
      "moon0:Bright": 1.6,
      "moon0:Glow": 0.8,
      "moon0:Size": 1.35,
      "moon0:Azimuth": 300,
      "moon0:Elevation": 28,
      Notes: "Daytime lobby: lunar disc parked below the horizon until the day cycle brings it up.",
    },
    stars: {
      "Star field": true,
      Brightness: 1.0,
      "Point Size": 1.0,
      "Limiting magnitude": 5.5,
      Twinkle: true,
      "Twinkle depth": 30,
      "Twinkle rate": 1.2,
      "Celestial rotation": 0,
      Notes: "Procedural deterministic celestial starfield with atmospheric scintillation.",
    },
    "lobby-ceiling-light": {
      Position: [0, 2.0, 2.90],
      Rotation: [0, 0, 0],
      Scale: [4.30, 1.90, 1.0],
      Intensity: 6.4,
      EmittedRadiance: [6.4, 6.0, 5.3],
      "Cast shadows": true,
      GI: true,
      ReferenceInspector: {
        Type: "LuminousSurface",
        Properties: {
          lumens: 2400,
          dimmer: 1,
          color: "#fff2e0",
          temperature: 5200,
          useTemp: false,
          width: 4.3,
          height: 1.9,
          twoSided: false,
          spread: 165,
          pos: [0, 2.0, 2.9],
          rot: [0, 0, 0],
          castShadows: true,
          shadowSoftness: 1,
          affectSpecular: true,
        },
      },
      Notes: "Clipped-horizon LTC quad light recessed into the lobby coffer.",
    },
    "lobby-reception-light": {
      Position: [-4.60, 4.35, 2.55],
      Rotation: [0, 0, 0],
      Scale: [1.60, 1.00, 1.0],
      Intensity: 7.8,
      EmittedRadiance: [7.8, 6.6, 4.8],
      "Cast shadows": true,
      GI: true,
      ReferenceInspector: {
        Type: "LuminousSurface",
        Properties: {
          lumens: 1800,
          dimmer: 1,
          color: "#ffe9c8",
          temperature: 3600,
          useTemp: false,
          width: 1.6,
          height: 1.0,
          twoSided: false,
          spread: 150,
          pos: [-4.60, 4.35, 2.55],
          rot: [0, 0, 0],
          castShadows: true,
          shadowSoftness: 1,
          affectSpecular: true,
        },
      },
      Notes: "Warm LTC downlight washing the reception counter and walnut desk.",
    },
  };

  return {
    Id: CustomId,
    Name: CustomName,
    Notes: CustomNotes || "Frontier lobby: ControlVehicle and Atrium Display Aircraft under a physical sun, sky and volumetric clouds.",
    Rows,
    Values,
    Hidden: {},
    Collapsed: {},
    Selected: "lobby-aircraft",
  };
}

export const LEVEL_TEMPLATES = Object.freeze([
  {
    Id: "gallery",
    Category: "Architectural",
    Badge: "GI SHOWCASE",
    Spec: "17 entities · Softbox + Abbe Glass",
    Title: "Cornell-Studio Gallery",
    Subtitle: "Canonical colour-bleed enclosure with chrome, gold plinth & crown glass",
  },
  {
    Id: "force-lab",
    Category: "Simulation",
    Badge: "VECTORFIELD LAB",
    Spec: "3 forces · Wind + Vortex + Curl",
    Title: "Wind & Vortex ForceField Lab",
    Subtitle: "Cornell-Studio stage pre-wired with Linear Wind, Cyclonic Vortex & Curl Turbulence",
  },
  {
    Id: "lobby",
    Category: "Lobby",
    Badge: "DEFAULT SCENE",
    Spec: "32 entities · ControlVehicle + Aircraft",
    Title: "Frontier Lobby",
    Subtitle: "Travertine atrium with ControlVehicle, Atrium Display Aircraft, physical sky & volumetric clouds",
  },
  {
    Id: "lookdev",
    Category: "Look-Dev",
    Badge: "MATERIAL LAB",
    Spec: "12 entities · Cyclorama + Plinth",
    Title: "Look-Dev Plinth Studio",
    Subtitle: "Infinite cyclorama stage with Clear-Coat, Charlie Velvet & Subsurface spheres",
  },
  {
    Id: "empty",
    Category: "Blank",
    Badge: "CLEAN SLATE",
    Spec: "7 entities · Ground + Sun",
    Title: "Empty Proving Ground",
    Subtitle: "Minimal ground slab, calibration cube, directional Sun & Editor Camera",
  },
  {
    Id: "duplicate",
    Category: "Architectural",
    Badge: "CLONE ACTIVE",
    Spec: "1:1 snapshot · Active level",
    Title: "Duplicate Current Level",
    Subtitle: "Clone all entities, transforms, forces & 20-channel materials from active level",
  },
]);

function BuildForceFieldLabLevel(NewId, CleanName, Notes = "") {
  const Rows = Clone(ShowcaseInitialRows);
  const Values = Clone(ShowcaseInitialValues);
  Rows.push({
    Id: "forces",
    Name: "Forces",
    Panel: "group",
    Icon: "folder-world",
    Parent: null,
    Description: "19-Law VectorField SimulationSpace",
  });
  const Rig = BuildDefaultDiagnosticForceRig();
  for (const Node of Rig) {
    Rows.push({
      Id: Node.Id,
      Name: Node.Name,
      Panel: "force",
      ForceModel: Node.Force.Model,
      Icon: `force-model:${Node.Force.Model}`,
      Parent: "forces",
      Preview: true,
      Description: `${Node.Force.Model} · R=${Node.Force.Radius}m`,
    });
    Values[Node.Id] = {
      Powered: true,
      Position: [...Node.Position],
      Rotation: [...Node.Rotation],
      Scale: [1, 1, 1],
      Force: Clone(Node.Force),
    };
  }
  return {
    Id: NewId,
    Name: CleanName,
    Notes: Notes || "VectorField Studio Wind + Cyclonic Vortex + Curl Turbulence stage.",
    Rows,
    Values,
    Hidden: {},
    Collapsed: {},
    Selected: Rig[1]?.Id || Rig[0]?.Id,
  };
}

export function CreateNewLevelRecord({ Name, TemplateId, Notes = "", CurrentLevel }) {
  const CleanName = String(Name || "").trim() || "Untitled Level";
  const NewId =
    "level-" +
    CleanName.toLowerCase().replace(/[^a-z0-9]+/g, "-").replace(/^-|-$/g, "") +
    "-" +
    Math.random().toString(36).slice(2, 6);

  if (TemplateId === "duplicate" && CurrentLevel) {
    const Copy = Clone(CurrentLevel);
    Copy.Id = NewId;
    Copy.Name = CleanName;
    Copy.Notes = Notes || `Duplicated from ${CurrentLevel.Name}`;
    if (Copy.Rows?.[0]?.Id === "scene") Copy.Rows[0].Name = CleanName;
    return EnsureRenderSettingsInLevel(Copy);
  }

  if (TemplateId === "gallery") {
    return EnsureRenderSettingsInLevel({
      Id: NewId,
      Name: CleanName,
      Notes: Notes || "Cornell-Studio Gallery level instance.",
      Rows: Clone(ShowcaseInitialRows),
      Values: Clone(ShowcaseInitialValues),
      Hidden: {},
      Collapsed: {},
      Selected: "chrome-sphere",
    });
  }

  if (TemplateId === "force-lab") {
    return EnsureRenderSettingsInLevel(BuildForceFieldLabLevel(NewId, CleanName, Notes));
  }

  if (TemplateId === "lobby") {
    return EnsureRenderSettingsInLevel(BuildLobbyLevel(NewId, CleanName, Notes));
  }

  if (TemplateId === "lookdev") {
    const Base = BuildLookDevStudioLevel();
    Base.Id = NewId;
    Base.Name = CleanName;
    if (Notes) Base.Notes = Notes;
    Base.Rows[0].Name = CleanName;
    return EnsureRenderSettingsInLevel(Base);
  }

  return EnsureRenderSettingsInLevel(BuildEmptyProvingGroundLevel(NewId, CleanName, Notes));
}

export function LoadSavedLevelsState() {
  const Defaults = BuildDefaultLevels();
  if (typeof window === "undefined") {
    return { ActiveLevelId: Defaults[0].Id, Levels: Defaults };
  }
  try {
    window.localStorage.removeItem("Frontier.WorldMirrorLevels.v1");
    window.localStorage.removeItem("Frontier.WorldMirrorLevels.v2");
    window.localStorage.removeItem("Frontier.WorldMirrorLevels.v3");
    window.localStorage.removeItem("Frontier.WorldMirrorLevels.v4");
    window.localStorage.removeItem("Frontier.WorldMirrorLevels.v5");
    window.localStorage.removeItem("Frontier.WorldMirrorLevels.v6");
    const Raw = window.localStorage.getItem(LEVEL_STORAGE_KEY);
    if (!Raw) return { ActiveLevelId: Defaults[0].Id, Levels: Defaults };
    const Parsed = JSON.parse(Raw);
    if (!Array.isArray(Parsed?.Levels) || Parsed.Levels.length === 0) {
      return { ActiveLevelId: Defaults[0].Id, Levels: Defaults };
    }
    const NormalizedLevels = Parsed.Levels.map((L) => EnsureRenderSettingsInLevel(L));
    const Active = NormalizedLevels.some((L) => L.Id === Parsed.ActiveLevelId)
      ? Parsed.ActiveLevelId
      : NormalizedLevels[0].Id;
    return { ActiveLevelId: Active, Levels: NormalizedLevels };
  } catch {
    return { ActiveLevelId: Defaults[0].Id, Levels: Defaults };
  }
}

export function ResetSavedLevelsState() {
  const Defaults = BuildDefaultLevels();
  const Fresh = { ActiveLevelId: Defaults[0].Id, Levels: Defaults };
  if (typeof window !== "undefined") {
    try {
      window.localStorage.removeItem("Frontier.WorldMirrorLevels.v1");
      window.localStorage.removeItem("Frontier.WorldMirrorLevels.v2");
      window.localStorage.removeItem("Frontier.WorldMirrorLevels.v6");
      window.localStorage.removeItem(LEVEL_STORAGE_KEY);
    } catch {}
  }
  return Fresh;
}

export function SaveLevelsState(State) {
  if (typeof window === "undefined") return;
  try {
    window.localStorage.setItem(LEVEL_STORAGE_KEY, JSON.stringify(State));
  } catch {}
}
