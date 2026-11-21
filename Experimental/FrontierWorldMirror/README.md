# Frontier World Mirror — Tier 1 GTX (`C++20 <-> HTML` Bidirectional Mirror)

Duplicated from `Experimental/ProjectZeroEditor/` (which remains untouched as the clean UI shell specification) to serve as the **1:1 live HTML mirror** of the native ISO C++20 editor (`Engine/GeometricRaster/GlobalIllumination/ShowcaseGalleryScene.{h,cpp}`, `TieredWorldRenderSequence.{h,cpp}`, `OutlinerPanel`, `ViewportPanel`, and `InspectorPanel`).

## Default Scene — Frontier Lobby

A fresh profile opens the **Frontier Lobby** (`BuildLobbyLevel()` in `LevelSystem.js`): a 12.00 ×
10.00 m travertine atrium with full-height structural glazing, a suspended ceiling coffer, brushed
stainless columns, a walnut reception desk, a backlit bronze sign pylon and two showroom exhibits —
the `ControlVehicle` BVH mesh and the **Atrium Display Aircraft** (`PropId: lobby_aircraft`, a
38-part high-wing monoplane registered in `WorldPropLibrary.js`). The level carries the physical
`Sun` + `Atmosphere` + `Volumetric Clouds` + `Moon` + `Stars` environment and two LTC area lights.

Built-in levels, in switcher order: `Frontier Lobby` → `Cornell-Studio Gallery` → `Look-Dev Plinth
Studio` → `Empty Proving Ground`. Create Level templates: Lobby, Cornell-Studio Gallery, Wind &
Vortex ForceField Lab, Look-Dev Plinth Studio, Empty Proving Ground, Duplicate Active Level.

## Synchronized Capabilities

- **Exact 25-Entry Cornell-Studio Gallery Outliner**:
  - `Showcase` (`10` OpenPBR specimens: `Gallery Floor`, `Gallery Back Wall`, `Crimson Left Wall`, `Emerald Right Wall`, `Clearcoat Roof Baffle`, `Anisotropic Gold Pedestal`, `Mirror Chrome Sphere`, `Plaster Specimen West`, `Plaster Specimen East`, `Abbe Crown Glass Orb`)
  - `Cameras` (`Editor Camera` at `(0.00, -3.55, 0.42)` targeting `(0.00, 0.55, 0.15)`, `43.0°` vertical FOV)
  - `World` (`Sun` directional solar emitter at `normalize(0.42, -0.55, 0.72)`)
  - `Lighting` (`Ceiling Softbox` clipped-horizon LTC polygonal area quad at `(0.55, 0.35, 1.82)`, `1.70 × 1.30 m`)
- **Calibrated WebGL2 Tier 1 GTX World Renderer (`GtxWorldViewport.jsx`)**:
  - OpenPBR surface scattering (Fujii Oren-Nayar, Anisotropic GGX, Adobe F82 conductor tint, Turquin energy compensation, Dielectric Clearcoat)
  - Solar directional shadow with GTX Contact-Clip + SDF Cone Penumbra (zero VSM)
  - Clipped-horizon Polygonal Quad LTC area light integration
  - 2-scale GTX Ambient & 2.2 m Chassis Occlusion + 6-Cone SDF Diffuse Global Illumination with live Crimson & Emerald wall multi-bounce color bleed
  - Single-bounce gallery specular reflections + 2-layer Abbe (`Abbe = 26`, `IOR = 1.52`) dispersive crown glass peel + ACES filmic tonemap
- **C++-Parity Inspector Panels**: `NativePanels.json` (SourceCommit `9864969`) drives the Sun,
  Atmosphere-Sky, Clouds, Moon, Stars, Fog, Wind, Lens Flare, Post/Tonemapping and Render Settings
  control sets; each subject is paginated as `Menu A | Menu B | Menu C` by `InspectorCarousel.jsx`.
  Geometry subjects expose `Overview · Transform · Material · Physics · UV · Geometry` quick
  controls plus an `Open Component Viewer` hand-off; the Editor Camera carries `Lens Flare` and
  `Tonemapping` as sub-components.
- **Interactive `EditorCamera` + Outliner/Inspector Sync**:
  - Orbit (`Drag`), Pan (`Shift+Drag` / `Middle-Drag`), Free-Look (`Right-Drag`), Dolly (`Wheel`), Fly (`Right+WASD`/`QE`), Frame Selected (`Focus` button), Click-to-Pick 3D viewport selection, and bidirectional `Position` / `Target` / `Focal Length` editing in the Inspector.

## Rebuild, Verify & Serve

```bash
node Experimental/FrontierWorldMirror/Build.mjs                                   # standalone index.html
node Experimental/FrontierWorldMirror/WorldMirrorValidation.mjs                   # 20 parity suites
npm install --prefix /tmp/smoke jsdom@25
JSDOM_DIR=/tmp/smoke node Experimental/FrontierWorldMirror/Smoke.mjs              # live-bundle boot smoke
python3 Experimental/FrontierWorldMirror/Serve.py 8098
```
