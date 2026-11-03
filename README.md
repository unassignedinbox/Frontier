# Frontier — World Forge

A runnable planet-scale world **blockout editor** and runtime viewer built from scratch in this repository. It is not a scrolling wave-normal demo: the ocean near field is an actual finite-volume 2.5D shallow-water solver, and the terrain source of truth is a signed distance field that is voxelised into a mesh.

The repo was empty apart from `README.md`, so this is a self-contained TypeScript + Three.js reference implementation. The engine-independent `SdfWorld` and `ShallowWaterSolver` modules can be ported to Unreal, Unity or Godot without depending on WebGL.

---

## Run it

```bash
npm install
npm run dev
```

Open the Vite URL:

- `/editor.html` — **World Forge**, the separate authoring editor
- `/index.html` — clean **runtime viewer** with the same world and camera

Other commands:

```bash
npm run typecheck
npm test
npm run build
npm run preview
```

The editor and viewer share the current world through `localStorage`. Use **Export editable SDF JSON** to move a blockout between machines.

---

## Controls

Unreal-editor-style viewport:

| Input | Action |
| --- | --- |
| Right mouse drag | Look |
| Middle mouse drag | Pan |
| `W A S D` | Fly |
| `Q` / `E` or `Space` | Down / up |
| `Shift` / `Ctrl` | 5× boost / 0.2× slow |
| Mouse wheel | Change fly speed |
| Left mouse drag | Sculpt SDF or disturb water |
| `1`–`0`, `B`, `N` | Tool shortcuts |
| `F` / `R` | Focus brush / reset camera |
| `P`, `H`, `G`, `L`, `V` | Pause, help, globe, LOD tint, SDF chunk |

---

## What is actually implemented

### 1. Separate editor, not a game-scene hack

`editor.html` builds a dedicated World Forge UI with:

- continent / gulf / island SDF brushes;
- raise, lower, flatten, smooth, ridge and city-pad strokes;
- water drop and momentum brushes;
- Africa-like continent, archipelago, random-continent and flat-plate presets;
- planet cartographer generated from the same SDF, clickable to teleport;
- bake/export panel and live telemetry;
- viewer page without authoring panels.

### 2. Planet-scale frame and camera-relative rendering

`WorldFrame` keeps absolute local-tangent planet coordinates in JS doubles. WebGL only receives `world - cameraWorld`, so meshes stay in Float32-friendly ranges while the camera can travel thousands of kilometres. Terrain and ocean clipmaps snap to their own LOD grids and stream around the camera.

The current projection is a local tangent plane around a configurable geographic origin, with an orthographic planet map for context. It is deliberately a **world blockout** coordinate system, not yet a full spherical quadtree with pole handling.

### 3. SDF is the source of truth; heightmap is a projection

`src/world/SdfWorld.ts`:

- `landSdf(x,z)` — polygon continent outline, domain-warped coastline noise, smooth CSG unions/subtractions for continents, gulfs and islands;
- `heightAt(x,z)` — SDF-derived continental shelf, seabed, relief, ridges, basins and editor height strokes;
- `sdf3D(x,y,z)` — 3D solid-earth signed field with a bottom cap and optional voxel-chunk box;
- `colorAt(...)` — deterministic bathymetry / elevation colouring.

The near-field `SdfHeroChunk` sends the serialised world definition to a Web Worker. `sdf.worker.ts` samples the 3D field and runs **Marching Cubes** on the zero isosurface, returning positions, analytic SDF-gradient normals and colours. Farther terrain rings use the same SDF projected to a height grid, with geometry clipmaps, LOD holes and vertical skirts.

This is a vertical-distance SDF (`y - height`) rather than a Euclidean distance transform; it is exact for terrain without overhangs and is cheap enough for interactive continent blockouts. Full 3D CSG primitives can be added for caves/arches without changing the meshing contract.

### 4. Real 2.5D water, adaptive like a water-Nanite budget

`src/ocean/ShallowWaterSolver.ts` solves the nonlinear shallow-water equations on an Arakawa C-grid:

- `eta` free-surface elevation at cell centres;
- `u`/`v` face velocities;
- `bed` from `SdfWorld.heightAt`;
- conservative depth-flux divergence;
- pressure-gradient momentum;
- wet/dry face handling and reflective land boundaries;
- absorbing sponge and optional boundary swell;
- CFL-derived substep count;
- whole-cell clipmap shifts that preserve surface state when the camera moves.

The mesh reads `eta` directly. There is **no wave texture, normal map or scrolling image** in the simulation path.

`src/app/OceanSystem.ts` wraps that near patch with progressively coarser tessellated ocean rings:

- near: simulated `gridSize²` cells at `cellSize` metres;
- far: `farLevels` clipmap rings with LOD holes, skirts, SDF shoreline clipping and analytic open-boundary vertex waves;
- far waves are a boundary condition, not the simulation;
- telemetry reports the uniform-grid equivalent and the vertex saving.

At the defaults (`128²` cells at 8 m, 9 far rings), the near patch is about 1 km across while the ocean extends hundreds of kilometres. A uniform near-density grid over that extent would need hundreds of millions of vertices; the clipmap uses tens of thousands.

### 5. Streaming LOD and debugging

- `TerrainStreamer`: 10 snapped terrain clipmap rings from 24 m cells to ~786 km extent; rebuild queue; hole skirts; LOD tint and wireframe.
- `OceanSystem`: near simulated patch plus tessellated far rings; ring rebuild queue; shoreline depth attributes.
- `DebugOverlay`: brush footprint and colour-coded adaptive LOD boundaries.
- `PlanetMinimap`: cached orthographic globe with lat/lon graticule, camera marker, brush marker and click-to-teleport.

### 6. Baking and interchange

- **SDF JSON** — authoritative editable definition with polygon, seed, world parameters and feature list.
- **Heightmap PNG + metadata** — RG channels hold a 16-bit normalised height, B holds land mask; JSON sidecar records sea level, min/max, extent and cell size.
- **Visible terrain OBJ** — currently streamed terrain rings plus the Marching Cubes hero chunk in world metres.

---

## Architecture map

```text
src/
  main.ts                     page bootstrap
  app/
    FrontierApp.ts            orchestration, input, HUD, editing actions
    UI.ts                     World Forge / viewer UI
    WorldFrame.ts             large-world camera-relative coordinates
    CameraRig.ts              Unreal-style fly camera
    InputController.ts        pointer + keyboard routing
    TerrainStreamer.ts        SDF-derived terrain clipmaps
    SdfHeroChunk.ts           worker-backed Marching Cubes chunk
    OceanSystem.ts            adaptive near sim + far tessellated rings
    RingBuilder.ts            hole/skirt clipmap geometry
    DebugOverlay.ts           brush + LOD gizmos
    PlanetMinimap.ts          orthographic SDF globe
    Exporters.ts              SDF / heightmap / OBJ downloads
  world/
    math.ts                   noise, polygon SDF, geo projection
    SdfWorld.ts               signed-field world model + presets
    sdf.worker.ts             real SDF -> triangle mesh
  ocean/
    ShallowWaterSolver.ts     finite-volume 2.5D SWE
  shaders/
    water.ts                  geometry-displaced water, no textures
  styles.css
tests/
  world.test.ts
  water.test.ts
```

---

## Tests

`npm test` covers:

- 3D SDF sign conventions and bottom cap;
- CSG island/gulf effects on both land SDF and derived height;
- deterministic world serialisation;
- shallow-water volume conservation with closed reflective boundaries;
- CFL-adaptive substepping in deep water;
- wet/dry bathymetry rejection of drops on land;
- preservation of surface state across whole-cell clipmap shifts.

---

## Honest limitations / next steps

This is a strong interactive reference, not a shipping Unreal planet renderer:

- the planet is a local tangent-plane blockout, not a full spherical cubed-globe quadtree;
- the solver runs on the CPU in typed arrays; a GPU compute port would raise the near-grid ceiling;
- far rings use analytic Gerstner-like basis waves as an open-boundary condition, while only the near patch is simulated;
- the 3D SDF currently has no caves or overhangs;
- coastline/terrain shading is procedural colour, not a biome/material system;
- no multi-user collaboration or engine asset import pipeline yet.

The important part is that the data flow is correct: **SDF → mesh**, **heightmap as cache**, **shallow-water state → surface geometry**, and **camera-following adaptive detail instead of a wave texture**.
