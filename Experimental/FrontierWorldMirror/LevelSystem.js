//==========================================================================================================================================
//                                                        LEVELSYSTEM.JS
//==========================================================================================================================================
// Multi-Level persistence, built-in Frontier levels, and Create Level template generator for FrontierWorldMirror.

import {
  BuildMaterialRecord,
  CanonicalEditorCamera,
  ShowcaseInitialRows,
  ShowcaseInitialValues,
} from "./ShowcaseGalleryScene.js";
import { BuildDefaultDiagnosticForceRig } from "./ForceFieldCatalog.js";
import {
  BuildCylinderConeMaterial,
  BuildTrafficConeMaterial,
} from "./WorldDragDrop.js";

export const LEVEL_STORAGE_KEY = "Frontier.WorldMirrorLevels.v4";

const Clone = (Value) => JSON.parse(JSON.stringify(Value));

function BuildFrontierLobbyLevel(
  CustomId = "level-frontier-lobby",
  CustomName = "Frontier Lobby",
  CustomNotes = "",
) {
  const Rows = [
    { Id: "scene", Name: CustomName, Panel: "group", Icon: "folder-scene", Parent: null, Description: "Architectural Motorsport Showroom & Arrival Atrium" },
    {
      Id: "hero-vehicle",
      Name: "BP_ControlVehicle",
      PropId: "bp_control_vehicle",
      BlueprintKind: "BP_ControlVehicle",
      Panel: "geometry",
      Icon: "editor-cube",
      Parent: "scene",
      Preview: true,
      Description: "Exact Component Inspector BP_ControlVehicle · 25.1k tris FCT1 · double-click to inspect",
    },
    { Id: "lobby-terrazzo-floor", Name: "Atrium Basalt Terrazzo Floor", PrimitiveShape: "box", Panel: "geometry", Icon: "editor-cube", Parent: "scene", Preview: true, Description: "Honed slate terrazzo · 18.0 × 16.0 m" },
    { Id: "lobby-turntable-outer", Name: "Showroom Turntable Brass Ring", PrimitiveShape: "cylinder", Panel: "geometry", Icon: "editor-cylinder", Parent: "scene", Preview: true, Description: "Brushed champagne titanium plinth · Ø 7.4 m" },
    { Id: "lobby-turntable-deck", Name: "Showroom Turntable Obsidian Deck", PrimitiveShape: "cylinder", Panel: "geometry", Icon: "editor-cylinder", Parent: "scene", Preview: true, Description: "Satin basalt turntable deck · Ø 6.85 m" },
    { Id: "lobby-feature-wall", Name: "Travertine Feature Wall", PrimitiveShape: "box", Panel: "geometry", Icon: "editor-cube", Parent: "scene", Preview: true, Description: "Honed warm Roman limestone backdrop · 16.4 × 4.7 m" },
    { Id: "lobby-wall-plinth", Name: "Feature Wall Bronze Baseboard", PrimitiveShape: "box", Panel: "geometry", Icon: "editor-cube", Parent: "scene", Preview: true, Description: "Brushed bronze architectural sill" },
    { Id: "lobby-pilaster-1", Name: "Bronze Pilaster · West 1", PrimitiveShape: "box", Panel: "geometry", Icon: "editor-cube", Parent: "scene", Preview: true, Description: "Fluted architectural bronze column" },
    { Id: "lobby-pilaster-2", Name: "Bronze Pilaster · West 2", PrimitiveShape: "box", Panel: "geometry", Icon: "editor-cube", Parent: "scene", Preview: true, Description: "Fluted architectural bronze column" },
    { Id: "lobby-pilaster-3", Name: "Bronze Pilaster · Center W", PrimitiveShape: "box", Panel: "geometry", Icon: "editor-cube", Parent: "scene", Preview: true, Description: "Fluted architectural bronze column" },
    { Id: "lobby-pilaster-4", Name: "Bronze Pilaster · Center E", PrimitiveShape: "box", Panel: "geometry", Icon: "editor-cube", Parent: "scene", Preview: true, Description: "Fluted architectural bronze column" },
    { Id: "lobby-pilaster-5", Name: "Bronze Pilaster · East 1", PrimitiveShape: "box", Panel: "geometry", Icon: "editor-cube", Parent: "scene", Preview: true, Description: "Fluted architectural bronze column" },
    { Id: "lobby-pilaster-6", Name: "Bronze Pilaster · East 2", PrimitiveShape: "box", Panel: "geometry", Icon: "editor-cube", Parent: "scene", Preview: true, Description: "Fluted architectural bronze column" },
    { Id: "lobby-terracotta-wall", Name: "Terracotta Acoustic Gallery Wall", PrimitiveShape: "box", Panel: "geometry", Icon: "editor-cube", Parent: "scene", Preview: true, Description: "Warm terracotta GI bounce wall" },
    { Id: "lobby-walnut-fin-1", Name: "Walnut Acoustic Fin · South", PrimitiveShape: "box", Panel: "geometry", Icon: "editor-cube", Parent: "scene", Preview: true, Description: "Dark walnut architectural louver" },
    { Id: "lobby-walnut-fin-2", Name: "Walnut Acoustic Fin · Center", PrimitiveShape: "box", Panel: "geometry", Icon: "editor-cube", Parent: "scene", Preview: true, Description: "Dark walnut architectural louver" },
    { Id: "lobby-walnut-fin-3", Name: "Walnut Acoustic Fin · North", PrimitiveShape: "box", Panel: "geometry", Icon: "editor-cube", Parent: "scene", Preview: true, Description: "Dark walnut architectural louver" },
    { Id: "lobby-mullion-1", Name: "Atrium Mullion · South", PrimitiveShape: "box", Panel: "geometry", Icon: "editor-cube", Parent: "scene", Preview: true, Description: "Anodized curtain-wall column" },
    { Id: "lobby-mullion-2", Name: "Atrium Mullion · Mid-S", PrimitiveShape: "box", Panel: "geometry", Icon: "editor-cube", Parent: "scene", Preview: true, Description: "Anodized curtain-wall column" },
    { Id: "lobby-mullion-3", Name: "Atrium Mullion · Mid-N", PrimitiveShape: "box", Panel: "geometry", Icon: "editor-cube", Parent: "scene", Preview: true, Description: "Anodized curtain-wall column" },
    { Id: "lobby-mullion-4", Name: "Atrium Mullion · North", PrimitiveShape: "box", Panel: "geometry", Icon: "editor-cube", Parent: "scene", Preview: true, Description: "Anodized curtain-wall column" },
    { Id: "lobby-glass-pane-1", Name: "Abbe Atrium Curtain Glass · S", PrimitiveShape: "box", Panel: "geometry", Icon: "editor-cube", Parent: "scene", Preview: true, Description: "Dispersive architectural curtain glass" },
    { Id: "lobby-glass-pane-2", Name: "Abbe Atrium Curtain Glass · N", PrimitiveShape: "box", Panel: "geometry", Icon: "editor-cube", Parent: "scene", Preview: true, Description: "Dispersive architectural curtain glass" },
    { Id: "lobby-ceiling-north", Name: "Coffered Skylight Canopy · N", PrimitiveShape: "box", Panel: "geometry", Icon: "editor-cube", Parent: "scene", Preview: true, Description: "Matte acoustic ceiling soffit" },
    { Id: "lobby-ceiling-south", Name: "Coffered Skylight Canopy · S", PrimitiveShape: "box", Panel: "geometry", Icon: "editor-cube", Parent: "scene", Preview: true, Description: "Matte acoustic ceiling soffit" },
    { Id: "lobby-ceiling-west", Name: "Coffered Skylight Canopy · W", PrimitiveShape: "box", Panel: "geometry", Icon: "editor-cube", Parent: "scene", Preview: true, Description: "Matte acoustic ceiling soffit" },
    { Id: "lobby-ceiling-east", Name: "Coffered Skylight Canopy · E", PrimitiveShape: "box", Panel: "geometry", Icon: "editor-cube", Parent: "scene", Preview: true, Description: "Matte acoustic ceiling soffit" },
    { Id: "lobby-skylight-beam-1", Name: "Skylight Louver Beam · South", PrimitiveShape: "box", Panel: "geometry", Icon: "editor-cube", Parent: "scene", Preview: true, Description: "Structural dark-steel skylight baffle" },
    { Id: "lobby-skylight-beam-2", Name: "Skylight Louver Beam · North", PrimitiveShape: "box", Panel: "geometry", Icon: "editor-cube", Parent: "scene", Preview: true, Description: "Structural dark-steel skylight baffle" },
    { Id: "lobby-reception-base", Name: "Concierge Reception Monolith", PrimitiveShape: "box", Panel: "geometry", Icon: "editor-cube", Parent: "scene", Preview: true, Description: "Honed obsidian reception counter" },
    { Id: "lobby-reception-top", Name: "Concierge Travertine Countertop", PrimitiveShape: "box", Panel: "geometry", Icon: "editor-cube", Parent: "scene", Preview: true, Description: "Beveled travertine stone reception slab" },
    { Id: "lobby-lounge-base", Name: "Lounge Bench Steel Frame", PrimitiveShape: "box", Panel: "geometry", Icon: "editor-cube", Parent: "scene", Preview: true, Description: "Matte black steel lounge plinth" },
    { Id: "lobby-lounge-cushion", Name: "Lounge Cognac Leather Bench", PrimitiveShape: "box", Panel: "geometry", Icon: "editor-cube", Parent: "scene", Preview: true, Description: "Saddle-cognac architectural seating" },
    { Id: "lobby-sculpture-plinth", Name: "Gallery Glass Plinth", PrimitiveShape: "cylinder", Panel: "geometry", Icon: "editor-cylinder", Parent: "scene", Preview: true, Description: "Brushed bronze showcase pedestal" },
    { Id: "lobby-crown-orb", Name: "Abbe Crown Glass Sculpture", PrimitiveShape: "sphere", Panel: "geometry", Icon: "editor-sphere", Parent: "scene", Preview: true, Description: "Dispersive optical crown-glass sphere · Vd=26" },
    { Id: "lobby-chrome-orb", Name: "Mirror Chrome Reference Orb", PrimitiveShape: "sphere", Panel: "geometry", Icon: "editor-sphere", Parent: "scene", Preview: true, Description: "Specular mirror reference sphere" },
    { Id: "lobby-totem-west", Name: "Telemetry Pylon · West", PrimitiveShape: "box", Panel: "geometry", Icon: "editor-cube", Parent: "scene", Preview: true, Description: "Wayfinding monolith totem" },
    { Id: "lobby-totem-east", Name: "Telemetry Pylon · East", PrimitiveShape: "box", Panel: "geometry", Icon: "editor-cube", Parent: "scene", Preview: true, Description: "Wayfinding monolith totem" },
    { Id: "lobby-cinema-cam", Name: "Showroom Cinema Camera", PropId: "cinema_camera", Panel: "camera", Icon: "camera", Parent: "scene", Preview: true, Description: "Studio cinema rig aimed at hero turntable" },
    { Id: "cameras", Name: "Cameras", Panel: "group", Icon: "camera", Parent: null, Description: "Editor cameras" },
    { Id: "camera", Name: "Editor Camera", Panel: "camera", Icon: "camera", Parent: "cameras", Description: "Permanent editor navigation camera" },
    { Id: "post", Name: "ACES Post-Process", Panel: "post", Icon: "environment-exposure", Parent: "cameras", Description: "Exposure & tonemapping" },
    { Id: "world", Name: "World", Panel: "group", Icon: "folder-world", Parent: null, Description: "Solar & sky environment" },
    { Id: "sun", Name: "Lobby Skylight Sun", Panel: "sun", Icon: "sun", Parent: "world", Preview: true, Description: "Golden afternoon raking sun · 5.6 W/m²" },
    { Id: "lighting", Name: "Lighting", Panel: "group", Icon: "folder-generic", Parent: null, Description: "Architectural LTC area lights" },
    { Id: "lobby-skylight-key", Name: "Showroom Skylight Softbox", Panel: "light", Icon: "editor-area-light", Parent: "lighting", Preview: true, Description: "LTC Skylight · 4.60 × 2.50 m" },
    { Id: "lobby-wall-cove", Name: "Travertine Wall Grazing Cove", Panel: "light", Icon: "editor-area-light", Parent: "lighting", Preview: true, Description: "Warm champagne wall-washer · 7.40 × 0.55 m" },
    { Id: "lobby-reception-strip", Name: "Concierge Pendant Strip", Panel: "light", Icon: "editor-area-light", Parent: "lighting", Preview: true, Description: "Warm architectural strip · 2.40 × 0.65 m" },
    { Id: "lobby-pavilion-fill", Name: "Atrium Pavilion Skylight Fill", Panel: "light", Icon: "editor-area-light", Parent: "lighting", Preview: true, Description: "Cool daylight fill · 2.80 × 1.20 m" },
  ];

  const BronzeMat = () => BuildMaterialRecord({ Type: "Metal", BaseLinear: [0.58, 0.44, 0.27], Metallic: 0.92, Roughness: 0.24 });
  const MullionMat = () => BuildMaterialRecord({ Type: "Metal", BaseLinear: [0.11, 0.12, 0.14], Metallic: 0.86, Roughness: 0.30 });
  const CeilingMat = () => BuildMaterialRecord({ Type: "Standard", BaseLinear: [0.14, 0.15, 0.17], Roughness: 0.78 });
  const WalnutMat = () => BuildMaterialRecord({ Type: "Standard", BaseLinear: [0.16, 0.09, 0.05], Roughness: 0.54 });
  const CurtainGlass = () => ({
    ...BuildMaterialRecord({ Type: "Glass", BaseLinear: [0.90, 0.96, 1.0], Roughness: 0.03, Transmission: 0.92, Ior: 1.52 }),
    AbbeDispersionNumber: 34,
    AttenuationLinear: [0.92, 0.97, 1.0],
  });

  const Values = {
    "hero-vehicle": {
      Position: [0.0, 1.35, 0.13],
      Rotation: [0, 0, -28],
      Scale: [1, 1, 1],
      PropId: "bp_control_vehicle",
      BlueprintKind: "BP_ControlVehicle",
      PaintHex: "#e4e440",
      WingVisible: true,
      HeadlightsOn: true,
      "Cast shadows": true,
      GI: true,
      ...BuildMaterialRecord({ Type: "Clear coat", BaseLinear: [0.68, 0.80, 0.0], Metallic: 0.04, Roughness: 0.20, Coat: 0.92, CoatRoughness: 0.05 }),
    },
    "lobby-terrazzo-floor": {
      Position: [0.0, 1.0, -0.10],
      Rotation: [0, 0, 0],
      Scale: [18.0, 16.0, 0.20],
      "Cast shadows": true,
      GI: true,
      ...BuildMaterialRecord({ Type: "Clear coat", BaseLinear: [0.11, 0.12, 0.14], Roughness: 0.24, Coat: 0.58, CoatRoughness: 0.09 }),
    },
    "lobby-turntable-outer": {
      Position: [0.0, 1.35, 0.06],
      Rotation: [0, 0, 0],
      Scale: [7.4, 7.4, 0.12],
      "Cast shadows": true,
      GI: true,
      ...BuildMaterialRecord({ Type: "Metal", BaseLinear: [0.64, 0.56, 0.44], Metallic: 0.90, Roughness: 0.24 }),
    },
    "lobby-turntable-deck": {
      Position: [0.0, 1.35, 0.07],
      Rotation: [0, 0, 0],
      Scale: [6.88, 6.88, 0.12],
      "Cast shadows": true,
      GI: true,
      ...BuildMaterialRecord({ Type: "Clear coat", BaseLinear: [0.055, 0.060, 0.070], Roughness: 0.28, Coat: 0.45, CoatRoughness: 0.10 }),
    },
    "lobby-feature-wall": {
      Position: [0.0, 6.95, 2.35],
      Rotation: [0, 0, 0],
      Scale: [16.4, 0.32, 4.70],
      "Cast shadows": true,
      GI: true,
      ...BuildMaterialRecord({ Type: "Standard", BaseLinear: [0.62, 0.56, 0.48], Roughness: 0.64 }),
    },
    "lobby-wall-plinth": {
      Position: [0.0, 6.68, 0.14],
      Rotation: [0, 0, 0],
      Scale: [15.4, 0.26, 0.28],
      "Cast shadows": true,
      GI: true,
      ...BronzeMat(),
    },
    "lobby-pilaster-1": { Position: [-6.0, 6.68, 2.35], Rotation: [0, 0, 0], Scale: [0.24, 0.24, 4.70], "Cast shadows": true, GI: true, ...BronzeMat() },
    "lobby-pilaster-2": { Position: [-3.6, 6.68, 2.35], Rotation: [0, 0, 0], Scale: [0.24, 0.24, 4.70], "Cast shadows": true, GI: true, ...BronzeMat() },
    "lobby-pilaster-3": { Position: [-1.2, 6.68, 2.35], Rotation: [0, 0, 0], Scale: [0.24, 0.24, 4.70], "Cast shadows": true, GI: true, ...BronzeMat() },
    "lobby-pilaster-4": { Position: [1.2, 6.68, 2.35], Rotation: [0, 0, 0], Scale: [0.24, 0.24, 4.70], "Cast shadows": true, GI: true, ...BronzeMat() },
    "lobby-pilaster-5": { Position: [3.6, 6.68, 2.35], Rotation: [0, 0, 0], Scale: [0.24, 0.24, 4.70], "Cast shadows": true, GI: true, ...BronzeMat() },
    "lobby-pilaster-6": { Position: [6.0, 6.68, 2.35], Rotation: [0, 0, 0], Scale: [0.24, 0.24, 4.70], "Cast shadows": true, GI: true, ...BronzeMat() },
    "lobby-terracotta-wall": {
      Position: [-7.6, 1.0, 2.35],
      Rotation: [0, 0, 0],
      Scale: [0.34, 14.0, 4.70],
      "Cast shadows": true,
      GI: true,
      ...BuildMaterialRecord({ Type: "Standard", BaseLinear: [0.52, 0.16, 0.09], Roughness: 0.68 }),
    },
    "lobby-walnut-fin-1": { Position: [-7.25, -2.0, 2.35], Rotation: [0, 0, 0], Scale: [0.22, 0.18, 4.65], "Cast shadows": true, GI: true, ...WalnutMat() },
    "lobby-walnut-fin-2": { Position: [-7.25, 1.2, 2.35], Rotation: [0, 0, 0], Scale: [0.22, 0.18, 4.65], "Cast shadows": true, GI: true, ...WalnutMat() },
    "lobby-walnut-fin-3": { Position: [-7.25, 4.4, 2.35], Rotation: [0, 0, 0], Scale: [0.22, 0.18, 4.65], "Cast shadows": true, GI: true, ...WalnutMat() },
    "lobby-mullion-1": { Position: [7.45, -4.2, 2.35], Rotation: [0, 0, 0], Scale: [0.34, 0.34, 4.70], "Cast shadows": true, GI: true, ...MullionMat() },
    "lobby-mullion-2": { Position: [7.45, -0.6, 2.35], Rotation: [0, 0, 0], Scale: [0.34, 0.34, 4.70], "Cast shadows": true, GI: true, ...MullionMat() },
    "lobby-mullion-3": { Position: [7.45, 3.0, 2.35], Rotation: [0, 0, 0], Scale: [0.34, 0.34, 4.70], "Cast shadows": true, GI: true, ...MullionMat() },
    "lobby-mullion-4": { Position: [7.45, 6.6, 2.35], Rotation: [0, 0, 0], Scale: [0.34, 0.34, 4.70], "Cast shadows": true, GI: true, ...MullionMat() },
    "lobby-glass-pane-1": { Position: [7.55, 1.2, 2.22], Rotation: [0, 0, 0], Scale: [0.08, 3.25, 4.35], "Cast shadows": false, GI: false, ...CurtainGlass() },
    "lobby-glass-pane-2": { Position: [7.55, 4.8, 2.22], Rotation: [0, 0, 0], Scale: [0.08, 3.25, 4.35], "Cast shadows": false, GI: false, ...CurtainGlass() },
    "lobby-ceiling-north": { Position: [0.0, 5.45, 4.68], Rotation: [0, 0, 0], Scale: [16.2, 3.4, 0.28], "Cast shadows": true, GI: true, ...CeilingMat() },
    "lobby-ceiling-south": { Position: [0.0, -4.4, 4.68], Rotation: [0, 0, 0], Scale: [16.2, 3.2, 0.28], "Cast shadows": true, GI: true, ...CeilingMat() },
    "lobby-ceiling-west": { Position: [-5.8, 0.55, 4.68], Rotation: [0, 0, 0], Scale: [4.2, 7.0, 0.28], "Cast shadows": true, GI: true, ...CeilingMat() },
    "lobby-ceiling-east": { Position: [5.8, 0.55, 4.68], Rotation: [0, 0, 0], Scale: [4.2, 7.0, 0.28], "Cast shadows": true, GI: true, ...CeilingMat() },
    "lobby-skylight-beam-1": { Position: [0.0, -0.35, 4.52], Rotation: [0, 0, 0], Scale: [7.6, 0.22, 0.32], "Cast shadows": true, GI: true, ...MullionMat() },
    "lobby-skylight-beam-2": { Position: [0.0, 3.05, 4.52], Rotation: [0, 0, 0], Scale: [7.6, 0.22, 0.32], "Cast shadows": true, GI: true, ...MullionMat() },
    "lobby-reception-base": {
      Position: [-4.55, -2.15, 0.44],
      Rotation: [0, 0, 14],
      Scale: [2.50, 0.78, 0.88],
      "Cast shadows": true,
      GI: true,
      ...BuildMaterialRecord({ Type: "Standard", BaseLinear: [0.08, 0.085, 0.095], Roughness: 0.36 }),
    },
    "lobby-reception-top": {
      Position: [-4.55, -2.15, 0.92],
      Rotation: [0, 0, 14],
      Scale: [2.62, 0.88, 0.10],
      "Cast shadows": true,
      GI: true,
      ...BuildMaterialRecord({ Type: "Standard", BaseLinear: [0.68, 0.62, 0.54], Roughness: 0.34 }),
    },
    "lobby-lounge-base": {
      Position: [4.65, -2.15, 0.16],
      Rotation: [0, 0, -16],
      Scale: [2.25, 0.86, 0.32],
      "Cast shadows": true,
      GI: true,
      ...MullionMat(),
    },
    "lobby-lounge-cushion": {
      Position: [4.65, -2.15, 0.38],
      Rotation: [0, 0, -16],
      Scale: [2.18, 0.80, 0.16],
      "Cast shadows": true,
      GI: true,
      ...BuildMaterialRecord({ Type: "Standard", BaseLinear: [0.34, 0.16, 0.07], Roughness: 0.52 }),
    },
    "lobby-sculpture-plinth": {
      Position: [4.65, 4.15, 0.45],
      Rotation: [0, 0, 0],
      Scale: [0.96, 0.96, 0.90],
      "Cast shadows": true,
      GI: true,
      ...BronzeMat(),
    },
    "lobby-crown-orb": {
      Position: [4.65, 4.15, 1.24],
      Rotation: [0, 0, 0],
      Scale: [0.68, 0.68, 0.68],
      "Cast shadows": false,
      GI: false,
      AbbeDispersionNumber: 26,
      AttenuationLinear: [0.88, 0.96, 1.0],
      ...BuildMaterialRecord({ Type: "Glass", BaseLinear: [0.94, 0.98, 1.0], Roughness: 0.02, Transmission: 0.96, Ior: 1.54 }),
    },
    "lobby-chrome-orb": {
      Position: [-4.65, 4.15, 0.48],
      Rotation: [0, 0, 0],
      Scale: [0.92, 0.92, 0.92],
      "Cast shadows": true,
      GI: true,
      ...BuildMaterialRecord({ Type: "Metal", BaseLinear: [0.92, 0.93, 0.95], Metallic: 1.0, Roughness: 0.05 }),
    },
    "lobby-totem-west": {
      Position: [-5.35, 1.55, 1.15],
      Rotation: [0, 0, 22],
      Scale: [0.16, 0.68, 2.30],
      "Cast shadows": true,
      GI: true,
      ...BuildMaterialRecord({ Type: "Metal", BaseLinear: [0.14, 0.16, 0.19], Metallic: 0.82, Roughness: 0.28 }),
    },
    "lobby-totem-east": {
      Position: [5.35, 1.55, 1.15],
      Rotation: [0, 0, -22],
      Scale: [0.16, 0.68, 2.30],
      "Cast shadows": true,
      GI: true,
      ...BuildMaterialRecord({ Type: "Metal", BaseLinear: [0.14, 0.16, 0.19], Metallic: 0.82, Roughness: 0.28 }),
    },
    "lobby-cinema-cam": {
      Position: [3.85, -3.55, 0.0],
      Rotation: [0, 0, 42],
      Scale: [1, 1, 1],
      PropId: "cinema_camera",
    },
    camera: {
      Position: [5.45, -5.35, 1.98],
      Target: [-0.15, 1.25, 0.68],
      "Focal Length": 28.0,
      "Sensor Width": 36.0,
      Aperture: 4.0,
      "Subject Distance": 7.6,
    },
    post: { Exposure: 1.04, "Exposure EV": 0.05, Tonemap: "ACES", Gamma: 2.2 },
    sun: {
      Direction: [0.56, -0.46, 0.69],
      SolarIrradiance: [5.6, 4.95, 4.10],
      SkyRadiance: [0.16, 0.25, 0.42],
      "Angular Diameter": 2.2,
      Temperature: 5150,
      Intensity: 5.6,
    },
    "lobby-skylight-key": {
      Position: [0.0, 1.35, 4.25],
      Rotation: [0, 0, 0],
      Scale: [4.6, 2.5, 1.0],
      Intensity: 8.2,
      EmittedRadiance: [8.2, 7.85, 7.35],
    },
    "lobby-wall-cove": {
      Position: [0.0, 6.20, 4.25],
      Rotation: [0, 0, 0],
      Scale: [7.4, 0.55, 1.0],
      Intensity: 11.8,
      EmittedRadiance: [11.8, 8.6, 5.1],
    },
    "lobby-reception-strip": {
      Position: [-4.55, -2.15, 3.85],
      Rotation: [0, 0, 14],
      Scale: [2.4, 0.65, 1.0],
      Intensity: 7.4,
      EmittedRadiance: [7.4, 6.7, 5.7],
    },
    "lobby-pavilion-fill": {
      Position: [4.5, -0.5, 4.15],
      Rotation: [0, 0, 0],
      Scale: [2.8, 1.2, 1.0],
      Intensity: 6.0,
      EmittedRadiance: [5.8, 6.6, 7.8],
    },
  };

  return {
    Id: CustomId,
    Name: CustomName,
    Notes: CustomNotes || "Architectural motorsport showroom & arrival lobby with travertine pilasters, terracotta bounce gallery, Abbe glass curtain wall, LTC skylights, and BP_ControlVehicle.",
    Rows,
    Values,
    Hidden: {},
    Collapsed: {},
    Selected: "hero-vehicle",
  };
}

function BuildTrainingAreaLevel(
  CustomId = "level-training-area",
  CustomName = "Training Area",
  CustomNotes = "",
) {
  const Rows = [
    { Id: "scene", Name: CustomName, Panel: "group", Icon: "folder-scene", Parent: null, Description: "Motorsport Proving Course, Slalom Range & Pit Gantry" },
    {
      Id: "hero-vehicle",
      Name: "BP_ControlVehicle",
      PropId: "bp_control_vehicle",
      BlueprintKind: "BP_ControlVehicle",
      Panel: "geometry",
      Icon: "editor-cube",
      Parent: "scene",
      Preview: true,
      Description: "Exact Component Inspector BP_ControlVehicle staged at Start Gate",
    },
    { Id: "train-asphalt", Name: "Proving Ground Tarmac Pad", PrimitiveShape: "box", Panel: "geometry", Icon: "editor-cube", Parent: "scene", Preview: true, Description: "Dark proving-ground asphalt · 26.0 × 30.0 m" },
    { Id: "train-start-line", Name: "Start Grid Stripe", PrimitiveShape: "box", Panel: "geometry", Icon: "editor-cube", Parent: "scene", Preview: true, Description: "Painted thermoplastic start line" },
    { Id: "train-brake-line", Name: "Braking Threshold Marker", PrimitiveShape: "box", Panel: "geometry", Icon: "editor-cube", Parent: "scene", Preview: true, Description: "Painted braking gate stripe" },
    { Id: "train-cone-1", Name: "Slalom Cone · Gate 1", PropId: "slalom_cone", Panel: "geometry", Icon: "editor-cone", Parent: "scene", Preview: true, Description: "Reflective motorsport slalom cone" },
    { Id: "train-cone-2", Name: "Slalom Cone · Gate 2", PropId: "slalom_cone", Panel: "geometry", Icon: "editor-cone", Parent: "scene", Preview: true, Description: "Reflective motorsport slalom cone" },
    { Id: "train-cone-3", Name: "Slalom Cone · Gate 3", PropId: "slalom_cone", Panel: "geometry", Icon: "editor-cone", Parent: "scene", Preview: true, Description: "Reflective motorsport slalom cone" },
    { Id: "train-cone-4", Name: "Slalom Cone · Gate 4", PropId: "slalom_cone", Panel: "geometry", Icon: "editor-cone", Parent: "scene", Preview: true, Description: "Reflective motorsport slalom cone" },
    { Id: "train-bollard-1", Name: "Lane Delineator · West", PropId: "bollard_cone", Panel: "geometry", Icon: "editor-cylinder", Parent: "scene", Preview: true, Description: "Flexible proving lane delineator" },
    { Id: "train-bollard-2", Name: "Lane Delineator · East", PropId: "bollard_cone", Panel: "geometry", Icon: "editor-cylinder", Parent: "scene", Preview: true, Description: "Flexible proving lane delineator" },
    { Id: "train-curb-left", Name: "Apex Rumble Kerb · West", PropId: "rumble_curb", Panel: "geometry", Icon: "editor-cube", Parent: "scene", Preview: true, Description: "FIA red/white apex curb" },
    { Id: "train-curb-right", Name: "Apex Rumble Kerb · East", PropId: "rumble_curb", Panel: "geometry", Icon: "editor-cube", Parent: "scene", Preview: true, Description: "FIA red/white apex curb" },
    { Id: "train-barrier-e1", Name: "Jersey Barrier · East South", PropId: "jersey_barrier", Panel: "geometry", Icon: "editor-cube", Parent: "scene", Preview: true, Description: "Precast concrete safety barrier" },
    { Id: "train-barrier-e2", Name: "Jersey Barrier · East North", PropId: "jersey_barrier", Panel: "geometry", Icon: "editor-cube", Parent: "scene", Preview: true, Description: "Precast concrete safety barrier" },
    { Id: "train-tecpro-1", Name: "Tecpro Energy Barrier · Chicane", PropId: "tecpro_block", Panel: "geometry", Icon: "editor-cube", Parent: "scene", Preview: true, Description: "FIA impact-absorbing safety block" },
    { Id: "train-tires-1", Name: "Tire Stack · Chicane Apex", PropId: "tire_stack", Panel: "geometry", Icon: "editor-torus", Parent: "scene", Preview: true, Description: "Motorsport tire barrier stack" },
    { Id: "train-tires-2", Name: "Tire Stack · Pit Entry", PropId: "tire_stack", Panel: "geometry", Icon: "editor-torus", Parent: "scene", Preview: true, Description: "Motorsport tire barrier stack" },
    { Id: "train-pit-roof", Name: "Pit Gantry Canopy Roof", PrimitiveShape: "box", Panel: "geometry", Icon: "editor-cube", Parent: "scene", Preview: true, Description: "Covered steel telemetry bay roof" },
    { Id: "train-pit-col-1", Name: "Pit Gantry Column · SW", PrimitiveShape: "box", Panel: "geometry", Icon: "editor-cube", Parent: "scene", Preview: true, Description: "Structural H-beam steel pillar" },
    { Id: "train-pit-col-2", Name: "Pit Gantry Column · SE", PrimitiveShape: "box", Panel: "geometry", Icon: "editor-cube", Parent: "scene", Preview: true, Description: "Structural H-beam steel pillar" },
    { Id: "train-pit-col-3", Name: "Pit Gantry Column · NW", PrimitiveShape: "box", Panel: "geometry", Icon: "editor-cube", Parent: "scene", Preview: true, Description: "Structural H-beam steel pillar" },
    { Id: "train-pit-col-4", Name: "Pit Gantry Column · NE", PrimitiveShape: "box", Panel: "geometry", Icon: "editor-cube", Parent: "scene", Preview: true, Description: "Structural H-beam steel pillar" },
    { Id: "train-pit-board", Name: "Pit Telemetry Board", PropId: "pit_board", Panel: "geometry", Icon: "editor-cube", Parent: "scene", Preview: true, Description: "Trackside lap & split board" },
    { Id: "train-toolbox", Name: "Pit Mechanic Tool Chest", PropId: "toolbox_chest", Panel: "geometry", Icon: "editor-cube", Parent: "scene", Preview: true, Description: "Red steel rolling garage tool chest" },
    { Id: "train-fuel-drum", Name: "Race Fuel Drum", PropId: "fuel_drum", Panel: "geometry", Icon: "editor-cylinder", Parent: "scene", Preview: true, Description: "200L race fuel drum" },
    { Id: "train-phantom-cam", Name: "High-Speed Tracking Camera", PropId: "highspeed_phantom_cam", Panel: "camera", Icon: "camera", Parent: "scene", Preview: true, Description: "High-speed telemetry tracking camera" },
    { Id: "train-target-wall", Name: "Calibration Target Wall", PrimitiveShape: "box", Panel: "geometry", Icon: "editor-cube", Parent: "scene", Preview: true, Description: "Concrete physics & range backdrop" },
    { Id: "train-cal-chrome", Name: "Range Chrome Calibration Orb", PrimitiveShape: "sphere", Panel: "geometry", Icon: "editor-sphere", Parent: "scene", Preview: true, Description: "Mirror calibration sphere" },
    { Id: "train-cal-glass", Name: "Range Abbe Glass Orb", PrimitiveShape: "sphere", Panel: "geometry", Icon: "editor-sphere", Parent: "scene", Preview: true, Description: "Optical glass calibration sphere" },
    { Id: "cameras", Name: "Cameras", Panel: "group", Icon: "camera", Parent: null, Description: "Editor cameras" },
    { Id: "camera", Name: "Editor Camera", Panel: "camera", Icon: "camera", Parent: "cameras", Description: "Permanent editor navigation camera" },
    { Id: "post", Name: "ACES Post-Process", Panel: "post", Icon: "environment-exposure", Parent: "cameras", Description: "Exposure & tonemapping" },
    { Id: "world", Name: "World", Panel: "group", Icon: "folder-world", Parent: null, Description: "Proving ground solar sky" },
    { Id: "sun", Name: "Proving Ground Sun", Panel: "sun", Icon: "sun", Parent: "world", Preview: true, Description: "Crisp daylight sun · 6.4 W/m²" },
    { Id: "lighting", Name: "Lighting", Panel: "group", Icon: "folder-generic", Parent: null, Description: "Pit & track worklights" },
    { Id: "train-pit-light", Name: "Pit Bay Overhead Truss Light", Panel: "light", Icon: "editor-area-light", Parent: "lighting", Preview: true, Description: "LTC Pit Worklight · 3.60 × 1.10 m" },
    { Id: "train-range-fill", Name: "Start Gate Overhead Key", Panel: "light", Icon: "editor-area-light", Parent: "lighting", Preview: true, Description: "LTC Gantry Strip · 4.20 × 0.85 m" },
  ];

  const SteelMat = () => BuildMaterialRecord({ Type: "Metal", BaseLinear: [0.16, 0.18, 0.21], Metallic: 0.86, Roughness: 0.34 });
  const PaintStripe = () => BuildMaterialRecord({ Type: "Standard", BaseLinear: [0.82, 0.82, 0.80], Roughness: 0.72 });

  const Values = {
    "hero-vehicle": {
      Position: [0.0, -5.6, 0.0],
      Rotation: [0, 0, 90],
      Scale: [1, 1, 1],
      PropId: "bp_control_vehicle",
      BlueprintKind: "BP_ControlVehicle",
      PaintHex: "#e4e440",
      WingVisible: true,
      HeadlightsOn: true,
      "Cast shadows": true,
      GI: true,
      ...BuildMaterialRecord({ Type: "Clear coat", BaseLinear: [0.68, 0.80, 0.0], Metallic: 0.04, Roughness: 0.20, Coat: 0.92, CoatRoughness: 0.05 }),
    },
    "train-asphalt": {
      Position: [0.0, 2.0, -0.10],
      Rotation: [0, 0, 0],
      Scale: [26.0, 30.0, 0.20],
      "Cast shadows": true,
      GI: true,
      ...BuildMaterialRecord({ Type: "Standard", BaseLinear: [0.15, 0.16, 0.18], Roughness: 0.84 }),
    },
    "train-start-line": { Position: [0.0, -2.0, 0.008], Rotation: [0, 0, 0], Scale: [6.4, 0.26, 0.016], "Cast shadows": false, GI: true, ...PaintStripe() },
    "train-brake-line": { Position: [0.0, 8.8, 0.008], Rotation: [0, 0, 0], Scale: [6.4, 0.34, 0.016], "Cast shadows": false, GI: true, ...BuildMaterialRecord({ Type: "Standard", BaseLinear: [0.76, 0.18, 0.08], Roughness: 0.70 }) },
    "train-cone-1": { Position: [-1.15, 0.4, 0.38], Rotation: [0, 0, 0], Scale: [1, 1, 1], PropId: "slalom_cone" },
    "train-cone-2": { Position: [1.15, 3.2, 0.38], Rotation: [0, 0, 0], Scale: [1, 1, 1], PropId: "slalom_cone" },
    "train-cone-3": { Position: [-1.15, 6.0, 0.38], Rotation: [0, 0, 0], Scale: [1, 1, 1], PropId: "slalom_cone" },
    "train-cone-4": { Position: [1.15, 8.8, 0.38], Rotation: [0, 0, 0], Scale: [1, 1, 1], PropId: "slalom_cone" },
    "train-bollard-1": { Position: [-3.2, -1.8, 0.48], Rotation: [0, 0, 0], Scale: [1, 1, 1], PropId: "bollard_cone" },
    "train-bollard-2": { Position: [3.2, -1.8, 0.48], Rotation: [0, 0, 0], Scale: [1, 1, 1], PropId: "bollard_cone" },
    "train-curb-left": { Position: [-3.5, 4.6, 0.06], Rotation: [0, 0, 90], Scale: [1.8, 1, 1], PropId: "rumble_curb" },
    "train-curb-right": { Position: [3.5, 4.6, 0.06], Rotation: [0, 0, 90], Scale: [1.8, 1, 1], PropId: "rumble_curb" },
    "train-barrier-e1": { Position: [6.2, -2.5, 0.46], Rotation: [0, 0, 90], Scale: [1.5, 1, 1], PropId: "jersey_barrier" },
    "train-barrier-e2": { Position: [6.2, 4.8, 0.46], Rotation: [0, 0, 90], Scale: [1.5, 1, 1], PropId: "jersey_barrier" },
    "train-tecpro-1": { Position: [3.8, 10.6, 0.42], Rotation: [0, 0, -25], Scale: [1.2, 1, 1], PropId: "tecpro_block" },
    "train-tires-1": { Position: [-4.2, 8.4, 0.56], Rotation: [0, 0, 0], Scale: [1, 1, 1], PropId: "tire_stack" },
    "train-tires-2": { Position: [-4.2, -0.6, 0.56], Rotation: [0, 0, 0], Scale: [1, 1, 1], PropId: "tire_stack" },
    "train-pit-roof": { Position: [-6.4, -4.6, 3.65], Rotation: [0, 0, 0], Scale: [4.8, 7.6, 0.24], "Cast shadows": true, GI: true, ...SteelMat() },
    "train-pit-col-1": { Position: [-8.5, -8.1, 1.82], Rotation: [0, 0, 0], Scale: [0.24, 0.24, 3.64], "Cast shadows": true, GI: true, ...SteelMat() },
    "train-pit-col-2": { Position: [-4.3, -8.1, 1.82], Rotation: [0, 0, 0], Scale: [0.24, 0.24, 3.64], "Cast shadows": true, GI: true, ...SteelMat() },
    "train-pit-col-3": { Position: [-8.5, -1.1, 1.82], Rotation: [0, 0, 0], Scale: [0.24, 0.24, 3.64], "Cast shadows": true, GI: true, ...SteelMat() },
    "train-pit-col-4": { Position: [-4.3, -1.1, 1.82], Rotation: [0, 0, 0], Scale: [0.24, 0.24, 3.64], "Cast shadows": true, GI: true, ...SteelMat() },
    "train-pit-board": { Position: [-4.5, -5.6, 0.72], Rotation: [0, 0, 72], Scale: [1, 1, 1], PropId: "pit_board" },
    "train-toolbox": { Position: [-6.4, -3.4, 0.48], Rotation: [0, 0, 90], Scale: [1, 1, 1], PropId: "toolbox_chest" },
    "train-fuel-drum": { Position: [-6.8, -5.9, 0.45], Rotation: [0, 0, 0], Scale: [1, 1, 1], PropId: "fuel_drum" },
    "train-phantom-cam": { Position: [4.2, -6.2, 0.0], Rotation: [0, 0, 128], Scale: [1, 1, 1], PropId: "highspeed_phantom_cam" },
    "train-target-wall": {
      Position: [0.0, 14.2, 1.65],
      Rotation: [0, 0, 0],
      Scale: [13.5, 0.42, 3.30],
      "Cast shadows": true,
      GI: true,
      ...BuildMaterialRecord({ Type: "Standard", BaseLinear: [0.46, 0.44, 0.40], Roughness: 0.76 }),
    },
    "train-cal-chrome": {
      Position: [6.8, 1.1, 0.48],
      Rotation: [0, 0, 0],
      Scale: [0.96, 0.96, 0.96],
      "Cast shadows": true,
      GI: true,
      ...BuildMaterialRecord({ Type: "Metal", BaseLinear: [0.92, 0.93, 0.95], Metallic: 1.0, Roughness: 0.04 }),
    },
    "train-cal-glass": {
      Position: [6.8, -0.4, 0.48],
      Rotation: [0, 0, 0],
      Scale: [0.92, 0.92, 0.92],
      "Cast shadows": false,
      GI: false,
      AbbeDispersionNumber: 26,
      AttenuationLinear: [0.88, 0.96, 1.0],
      ...BuildMaterialRecord({ Type: "Glass", BaseLinear: [0.94, 0.98, 1.0], Roughness: 0.02, Transmission: 0.96, Ior: 1.54 }),
    },
    camera: {
      Position: [5.4, -10.8, 2.55],
      Target: [-0.2, -1.4, 0.72],
      "Focal Length": 30.0,
      "Sensor Width": 36.0,
      Aperture: 5.6,
      "Subject Distance": 10.5,
    },
    post: { Exposure: 1.0, "Exposure EV": 0.0, Tonemap: "ACES", Gamma: 2.2 },
    sun: {
      Direction: [0.46, -0.44, 0.77],
      SolarIrradiance: [6.4, 5.95, 5.25],
      SkyRadiance: [0.22, 0.35, 0.58],
      "Angular Diameter": 1.8,
      Temperature: 5800,
      Intensity: 6.4,
    },
    "train-pit-light": {
      Position: [-6.4, -4.6, 3.38],
      Rotation: [0, 0, 0],
      Scale: [3.6, 1.1, 1.0],
      Intensity: 9.2,
      EmittedRadiance: [8.8, 9.2, 9.8],
    },
    "train-range-fill": {
      Position: [0.0, -5.6, 4.4],
      Rotation: [0, 0, 0],
      Scale: [4.2, 0.85, 1.0],
      Intensity: 5.8,
      EmittedRadiance: [6.2, 5.8, 5.2],
    },
  };

  return {
    Id: CustomId,
    Name: CustomName,
    Notes: CustomNotes || "Outdoor GI proving ground with BP_ControlVehicle at Start Gate, cone slalom lane, FIA barriers, covered pit telemetry gantry, and calibration targets.",
    Rows,
    Values,
    Hidden: {},
    Collapsed: {},
    Selected: "hero-vehicle",
  };
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
    BuildFrontierLobbyLevel(),
    BuildTrainingAreaLevel(),
    {
      Id: "level-showcase-gallery",
      Name: "Cornell-Studio Gallery",
      Notes: "Canonical C++ <-> HTML Architectural Cornell-Studio Gallery.",
      Rows: Clone(ShowcaseInitialRows),
      Values: Clone(ShowcaseInitialValues),
      Hidden: {},
      Collapsed: {},
      Selected: "chrome-sphere",
    },
    BuildLookDevStudioLevel(),
    BuildEmptyProvingGroundLevel(),
  ];
}

function BuildAutomotiveConeSlalomLevel(
  CustomId = "level-cone-slalom",
  CustomName = "Automotive Cone Slalom",
  CustomNotes = "",
) {
  const Rows = [
    { Id: "scene", Name: CustomName, Panel: "group", Icon: "folder-scene", Parent: null, Description: "Automotive proving ground" },
    {
      Id: "asphalt-pad",
      Name: "Proving Ground Asphalt",
      PrimitiveShape: "box",
      Panel: "geometry",
      Icon: "editor-cube",
      Parent: "scene",
      Preview: true,
      Description: "Dark tarmac proving pad · 8.40 × 8.40 m",
    },
    {
      Id: "traffic-cone-1",
      Name: "Traffic Cone · Gate A1",
      PrimitiveShape: "cone",
      Panel: "geometry",
      Icon: "editor-cone",
      Parent: "scene",
      Preview: true,
      Description: "Tapered car traffic cone · reflective collar",
    },
    {
      Id: "traffic-cone-2",
      Name: "Traffic Cone · Apex B",
      PrimitiveShape: "cone",
      Panel: "geometry",
      Icon: "editor-cone",
      Parent: "scene",
      Preview: true,
      Description: "Tapered car traffic cone · reflective collar",
    },
    {
      Id: "traffic-cone-3",
      Name: "Traffic Cone · Exit C1",
      PrimitiveShape: "cone",
      Panel: "geometry",
      Icon: "editor-cone",
      Parent: "scene",
      Preview: true,
      Description: "Tapered car traffic cone · reflective collar",
    },
    {
      Id: "cylinder-cone-1",
      Name: "Cylinder Cone · Lane West",
      PrimitiveShape: "cylinder-cone",
      Panel: "geometry",
      Icon: "editor-cylinder",
      Parent: "scene",
      Preview: true,
      Description: "Cylinder-like car lane delineator post",
    },
    {
      Id: "cylinder-cone-2",
      Name: "Cylinder Cone · Lane East",
      PrimitiveShape: "cylinder-cone",
      Panel: "geometry",
      Icon: "editor-cylinder",
      Parent: "scene",
      Preview: true,
      Description: "Cylinder-like car lane delineator post",
    },
    {
      Id: "cylinder-cone-3",
      Name: "Cylinder Cone · Finish Post",
      PrimitiveShape: "cylinder-cone",
      Panel: "geometry",
      Icon: "editor-cylinder",
      Parent: "scene",
      Preview: true,
      Description: "Cylinder-like car lane delineator post",
    },
    { Id: "cameras", Name: "Cameras", Panel: "group", Icon: "camera", Parent: null, Description: "Editor cameras" },
    { Id: "camera", Name: "Editor Camera", Panel: "camera", Icon: "camera", Parent: "cameras", Description: "Permanent editor navigation camera" },
    { Id: "post", Name: "ACES Post-Process", Panel: "post", Icon: "environment-exposure", Parent: "cameras", Description: "Exposure & tonemapping" },
    { Id: "world", Name: "World", Panel: "group", Icon: "folder-world", Parent: null, Description: "Solar & sky environment" },
    { Id: "sun", Name: "Track Sun", Panel: "sun", Icon: "sun", Parent: "world", Preview: true, Description: "Directional · 4.6 W/m²" },
    { Id: "lighting", Name: "Lighting", Panel: "group", Icon: "folder-generic", Parent: null, Description: "Track area lights" },
    {
      Id: "pit-rig",
      Name: "Pit Lane Overhead Rig",
      Panel: "light",
      Icon: "editor-area-light",
      Parent: "lighting",
      Preview: true,
      Description: "LTC Quad · 2.60 × 1.60 m",
    },
  ];

  const Values = {
    "asphalt-pad": {
      Position: [0, 0.5, -0.68],
      Rotation: [0, 0, 0],
      Scale: [8.4, 8.4, 0.16],
      "Cast shadows": true,
      GI: true,
      ...BuildMaterialRecord({ Type: "Standard", BaseLinear: [0.09, 0.095, 0.105], Roughness: 0.68 }),
    },
    "traffic-cone-1": {
      Position: [-0.95, -0.85, -0.21],
      Rotation: [0, 0, 0],
      Scale: [0.48, 0.48, 0.78],
      "Cast shadows": true,
      GI: true,
      Material: BuildTrafficConeMaterial(),
    },
    "traffic-cone-2": {
      Position: [0.15, 0.25, -0.21],
      Rotation: [0, 0, 0],
      Scale: [0.48, 0.48, 0.78],
      "Cast shadows": true,
      GI: true,
      Material: BuildTrafficConeMaterial(),
    },
    "traffic-cone-3": {
      Position: [-0.82, 1.45, -0.21],
      Rotation: [0, 0, 0],
      Scale: [0.48, 0.48, 0.78],
      "Cast shadows": true,
      GI: true,
      Material: BuildTrafficConeMaterial(),
    },
    "cylinder-cone-1": {
      Position: [-1.75, -0.15, -0.16],
      Rotation: [0, 0, 0],
      Scale: [0.40, 0.40, 0.88],
      "Cast shadows": true,
      GI: true,
      Material: BuildCylinderConeMaterial(),
    },
    "cylinder-cone-2": {
      Position: [1.35, -0.35, -0.16],
      Rotation: [0, 0, 0],
      Scale: [0.40, 0.40, 0.88],
      "Cast shadows": true,
      GI: true,
      Material: BuildCylinderConeMaterial(),
    },
    "cylinder-cone-3": {
      Position: [1.18, 1.35, -0.16],
      Rotation: [0, 0, 0],
      Scale: [0.40, 0.40, 0.88],
      "Cast shadows": true,
      GI: true,
      Material: BuildCylinderConeMaterial(),
    },
    camera: {
      Position: [0.0, -4.35, 1.42],
      Target: [0.0, 0.35, -0.08],
      "Focal Length": CanonicalEditorCamera.FocalLengthMm,
      "Sensor Width": CanonicalEditorCamera.SensorWidthMm,
      Speed: 1.0,
    },
    post: { Exposure: 0.1 },
    sun: {
      Direction: [-0.46, -0.52, 0.72],
      Irradiance: [4.5, 4.15, 3.65],
      "Angular Radius": 0.0093,
      "Sky Radiance": [0.22, 0.30, 0.44],
    },
    "pit-rig": {
      Position: [0, 0.3, 2.35],
      Intensity: 7.2,
      Width: 2.6,
      Height: 1.6,
      EmittedRadiance: [7.2, 7.0, 6.6],
    },
  };

  return {
    Id: CustomId,
    Name: CustomName,
    Notes: CustomNotes || "Automotive chicane stage with Traffic Cones & Cylinder Cones.",
    Rows,
    Values,
    Hidden: {},
    Collapsed: {},
    Selected: "traffic-cone-2",
  };
}

export const LEVEL_TEMPLATES = Object.freeze([
  {
    Id: "lobby",
    Category: "Architectural",
    Badge: "DEFAULT · GI",
    Spec: "BP_ControlVehicle · Skylight + Travertine",
    Title: "Frontier Lobby",
    Subtitle: "Architectural showroom atrium with BP_ControlVehicle, travertine pilasters, terracotta GI wall & Abbe glass",
  },
  {
    Id: "training",
    Category: "Automotive",
    Badge: "PROVING GROUND",
    Spec: "BP_ControlVehicle · Slalom + Pit Gantry",
    Title: "Training Area",
    Subtitle: "GI proving range with Start Gate vehicle, cone slalom corridor, FIA barriers & covered pit gantry",
  },
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
    Id: "slalom",
    Category: "Automotive",
    Badge: "CAR CHICANE",
    Spec: "6 cones · Traffic + Cylinder Cones",
    Title: "Automotive Cone Slalom",
    Subtitle: "Tarmac proving ground pre-staged with Traffic Cones & Cylinder Cones",
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
    return Copy;
  }

  if (TemplateId === "lobby") {
    return BuildFrontierLobbyLevel(NewId, CleanName, Notes);
  }

  if (TemplateId === "training") {
    return BuildTrainingAreaLevel(NewId, CleanName, Notes);
  }

  if (TemplateId === "gallery") {
    return {
      Id: NewId,
      Name: CleanName,
      Notes: Notes || "Cornell-Studio Gallery level instance.",
      Rows: Clone(ShowcaseInitialRows),
      Values: Clone(ShowcaseInitialValues),
      Hidden: {},
      Collapsed: {},
      Selected: "chrome-sphere",
    };
  }

  if (TemplateId === "force-lab") {
    return BuildForceFieldLabLevel(NewId, CleanName, Notes);
  }

  if (TemplateId === "slalom") {
    return BuildAutomotiveConeSlalomLevel(NewId, CleanName, Notes);
  }

  if (TemplateId === "lookdev") {
    const Base = BuildLookDevStudioLevel();
    Base.Id = NewId;
    Base.Name = CleanName;
    if (Notes) Base.Notes = Notes;
    Base.Rows[0].Name = CleanName;
    return Base;
  }

  return BuildEmptyProvingGroundLevel(NewId, CleanName, Notes);
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
    const Raw = window.localStorage.getItem(LEVEL_STORAGE_KEY);
    if (!Raw) return { ActiveLevelId: Defaults[0].Id, Levels: Defaults };
    const Parsed = JSON.parse(Raw);
    if (!Array.isArray(Parsed?.Levels) || Parsed.Levels.length === 0) {
      return { ActiveLevelId: Defaults[0].Id, Levels: Defaults };
    }
    const Active = Parsed.Levels.some((L) => L.Id === Parsed.ActiveLevelId)
      ? Parsed.ActiveLevelId
      : Parsed.Levels[0].Id;
    return { ActiveLevelId: Active, Levels: Parsed.Levels };
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
