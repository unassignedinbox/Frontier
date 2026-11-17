# Saved Repositories & Prototype Ledger

🧩 Permanent registry of upstream repositories, UI inspiration commits, and per-prototype approval status.

---

## 1 · Saved Upstream Repositories

| Alias | Repository URL | Branch | Exact Commit | Subtree Used | Local Path |
| --- | --- | --- | --- | --- | --- |
| **Frontier (SultanAladin)** | `https://github.com/SultanAladin/Frontier` | `arena/19265b6e-frontier` | `1fef98c8dc1131d9f38637bd6f300843ff05d807` | `Experimental/ComponentViewer`, `Experimental/Shared`, `Experimental/GeometryCompiler`, `Experimental/FrontierWorldMirror`, `Experimental/FractureEditor`, `Experimental/StudioIconSet`, `EngineDocs/AgenticInstuctions`, `SYSTEM_NOMENCLATURE.md` | `Experimental/`, `EngineDocs/AgenticInstuctions/`, `SYSTEM_NOMENCLATURE.md` |
| **Frontier-OLD (General UI Inspiration)** | `https://github.com/streamlinkinbox/Frontier-OLD` | `main` | `4cab9133ef01022019f0bf4c71b9b328791306bd` | `SourceImages/General UI/` (9 unique UI inspiration boards; 1 exact duplicate removed) | `Reference/GeneralUI/` |

### Visual Inspiration Index (`Reference/GeneralUI/`)

| File | Key UI Motifs Adopted |
| --- | --- |
| `0bab27674eea6c04ccaf172f194ce6ef.jpg` | Bento KPI tiles, stepped area telemetry curve with dashed target line & peak highlight bands, top pill bar, `↗` card expand corner icon |
| `13edfbc6a1110a5abd8ec9099429cf07.jpg` | Floating dark glass telemetry overlays, dot-matrix state progress, nested concentric radial rings (`80% GFI`), bottom timeline scrubber |
| `146a70d9e465a1a9def640bca5aa68f8.jpg` | Glowing status indicator dot, multi-marker calibration line (`31.2 / 19.2`), stacked media folder cards, acid-lime active pill chip |
| `14d2fdb22c3e179f6ad5fa4152d663ee.jpg` | Oversized light numerals with dimmed trailing digits, diagonal-hatched baseline vs solid colour comparison blocks with `%` delta badge, mini sparkline cards |
| `2efc05feefe1c066a44f6541cb74efb5.jpg` | Clean white active pill tab, dashed target zone, tactile rounded floating sheet |
| `34f26a48a78f0564a9a140e70f9530bd.jpg` | Reorderable pill rows with 6-dot drag handles, lift-elevation on drag, dashed `+ Add` bottom slot |
| `9b74ac9391a154dd048b5ce971ec3809.jpg` | High-contrast hero callout juxtaposed with dark media thumbnail strip & circular arrow buttons |
| `bef5178e3f075257202d8236789c9a73.jpg` | Bright yellow/lime top active pill, expandable out-of-range alert banner, segmented biomarker range-zone bars, dual-track pattern strip, conic ring gauges |
| `c10f343c6071539724e7bd61e0fe565b.jpg` | Translucent alert drawer, subtle red-tinted warning cards, vertical left hairline list dividers, pill search bar |

---

## 2 · Naming & Prototype Approval Ledger

> 🔴 **Gate rule:** Nothing moves from `Prototypes/` into `Experimental/ComponentViewer/` until its status is `🟢 Approved` by the user.

| ID | Item | Path | Status | Notes |
| --- | --- | --- | --- | --- |
| **N0** | Frontier-Original Naming Taxonomy (replaces all Unreal/engine terms) | Proposed in chat + `SYSTEM_NOMENCLATURE.md` | 🚧 Pending Approval | Must be approved before renaming live viewers |
| **P01** | Surface Strata Card (Obsidian + Acid-Lime `#d8f35a`) | `Prototypes/01-SurfaceStrataCard/index.html` | 🚧 Pending Approval | Port `8097` · Hatched waterfall bars, draggable deviation curve, calibration rail, live cluster preview (`Preview.png`) |
| **P02** | SpecimenBay — Standalone Asset Previewer (Component Viewer Theme: `Studio.css`) | `Prototypes/02-AssetPreviewer/index.html` | 🚧 Pending Approval | Port `8098` · Exact Component Viewer styling (`Studio.css`), redesigned clean Outliner treating every imported asset as **1 unified object** (`ControlVehicle` body+wheels merged as 1 object, `Vehicle Wheel`, `Carved Torus`, `Fluted Vase`, `Asteroid`, plus `+ Import` & `+ Add Specimen`), **1-vs-Many selection model** (1 selected → 1 item in Inspector & centered stage; many selected via click/checkbox/⌘/⇧ → shared batch controls + per-object cards in Inspector & side-by-side multi-object staging), dedicated **4-mode Collision Editor** (`Convex Hull`, `Bounding Box`, compound `Primitives` Box/Sphere/Capsule/Cylinder, `Use Mesh Faces`) for both single and multi-selections, full `Distance Field` (live `SdfRaymarchViewport`), `UV Atlas`, `Surface Strata`, `Form & Bounds`, `Portrait`, and `Surface Finish` inspectors, with `Stress Fracture` left cleanly blank (`Preview.png`) |
| **P03** | Surface Finish Studio — Standalone Material & Texture Editor (Component Viewer Theme: `Studio.css`) | `Prototypes/03-MaterialStudio/index.html` | 🚧 Pending Approval | Port `8099` · Unified Component Viewer styling (`Studio.css`), isolated `ShaderBall.mesh` (+ Cloth/Bevel Cube/Sphere) WebGL viewport, material debug channels (`Lit PBR`, `Base Colour`, `Roughness`, `Metallic`, `Normals`, `UV Check`), studio lighting rigs (`Softbox`, `Amber Rim`, `Overcast`, `Dark Lab`) + live `STUDIO LIGHTS` bar (`Exposure`, `Light Angle`, `Env Boost`), Material Library filmstrip, 2D Roughness × Metallic Finish Matrix + BRDF lobe, procedural + uploaded live maps, Fresnel IOR curve, full `Surface`/`Texture`/`Optics`/`Glow` tabs (`Preview.png`) |
| **P04** | Spatial Placement / Transform Card | Planned (`Prototypes/04-SpatialPlacementCard/`) | 🏳️ Queued | After P02 & P03 feedback |
| **P05** | Joint / Socket Kinematics Card | Planned (`Prototypes/05-JointKinematicsCard/`) | 🏳️ Queued | After prior approval |
| **P06** | Assembly Overview / Verification Card | Planned (`Prototypes/06-AssemblyOverviewCard/`) | 🏳️ Queued | After prior approval |
