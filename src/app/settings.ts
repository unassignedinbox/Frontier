import { DEFAULT_WATER_SETTINGS, type WaterSettings } from '../ocean/ShallowWaterSolver';
import type { SculptTool } from '../world/SdfWorld';

export interface EditorSettings extends WaterSettings {
  tool: SculptTool;
  brushRadiusKm: number;
  brushStrength: number;
  brushTarget: number;
  brushFeather: number;
  seaLevel: number;
  coastNoise: number;
  oceanDepth: number;
  landElevation: number;
  cameraSpeed: number;
  showWireframe: boolean;
  showLodColors: boolean;
  showLodRings: boolean;
  showSdfHero: boolean;
  showPlanetMap: boolean;
  showHelp: boolean;
  showWaterWireframe: boolean;
  planetExpanded: boolean;
}

export const DEFAULT_EDITOR_SETTINGS: EditorSettings = {
  ...DEFAULT_WATER_SETTINGS,
  tool: 'continent',
  brushRadiusKm: 120,
  brushStrength: 520,
  brushTarget: 45,
  brushFeather: 0.34,
  seaLevel: 0,
  coastNoise: 125000,
  oceanDepth: 4500,
  landElevation: 1250,
  cameraSpeed: 700,
  showWireframe: false,
  showLodColors: false,
  showLodRings: true,
  showSdfHero: true,
  showPlanetMap: true,
  showHelp: true,
  showWaterWireframe: false,
  planetExpanded: false,
};

export const TOOL_INFO: Record<SculptTool, { label: string; hint: string; radiusKm: number; color: number }> = {
  inspect: { label: 'Inspect', hint: 'Read coordinates without editing', radiusKm: 4, color: 0xb0bec5 },
  continent: { label: 'Continent', hint: 'Add land to the continent SDF', radiusKm: 180, color: 0x8bc34a },
  gulf: { label: 'Gulf / sea', hint: 'Subtract water from the land SDF', radiusKm: 100, color: 0x29b6f6 },
  island: { label: 'Island', hint: 'Union an island into the ocean', radiusKm: 35, color: 0xffd54f },
  raise: { label: 'Raise', hint: 'Add elevation to the height projection', radiusKm: 25, color: 0xffb74d },
  lower: { label: 'Lower', hint: 'Carve elevation', radiusKm: 25, color: 0xba68c8 },
  flatten: { label: 'Flatten', hint: 'Blend toward target elevation', radiusKm: 12, color: 0x90a4ae },
  smooth: { label: 'Smooth', hint: 'Low-pass the SDF height projection', radiusKm: 20, color: 0x80cbc4 },
  ridge: { label: 'Ridge', hint: 'Add noisy mountain relief', radiusKm: 60, color: 0xa1887f },
  cityPad: { label: 'City pad', hint: 'Flatten a base for a city blockout', radiusKm: 3, color: 0xf06292 },
  waterDrop: { label: 'Water drop', hint: 'Inject mass into the shallow-water solver', radiusKm: 0.05, color: 0x4fc3f7 },
  waterPush: { label: 'Push water', hint: 'Drag momentum into the 2.5D solver', radiusKm: 0.08, color: 0x0288d1 },
};
