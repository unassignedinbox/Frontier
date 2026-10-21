# FOUND — the fracture / fluid / wind / particle editor UI

You were right: it exists, and it was **only in commits on `arena/*` branches** — never merged to any
`main`/`master`, which is why you don't see it. This repo (`unassignedinbox/Frontier`) itself only had an
empty initial-commit README.

## TL;DR

The editor UI you described is **Project-Zero** (`Experimental/ProjectZeroEditor/`):

| You said | What it is in Project-Zero |
|---|---|
| fracture editor | `FracturePanel.jsx` card on geometry objects + expanded page `Experimental/FractureEditor/index.html` |
| fluid editor (smoke gas) | `GasPanel.jsx` (Gas Domain) → expanded **Fluid** app at `Experimental/Fluid/dist/index.html` |
| wind editor | `WindPanel.jsx` — World → Wind inspector |
| particle editor | `GasPanel.jsx` emitter sheet → expanded **ParticleEditor** at `Experimental/ParticleEditor/index.html` |

### Why "fracture editor does not have a render" / "I don't see it"

- Project-Zero (browser version) is **UI-only**: its own index says *"browser-local controls, no engine
  connection"*, its viewport is `CheckerViewport.jsx` (an SVG checker), and the Fracture **card** only shows a
  diagram glyph (`FractureGlyph` from `FractureProjection.js`). There is no 3D render of fragments in the card.
- The Fracture card only appears when a **geometry** row is selected, and its expand **↗** button is
  **disabled until you switch on "Enable fracture"**. That button opens the standalone Fracture Editor
  (`window.open("../FractureEditor/index.html?object=" + Id)` in `FractureSpecification.js`), and *that* page
  **does** render: `FracturePanel.js` uses three.js (`THREE.WebGLRenderer` + `OrbitControls`).

So: select **Cube/Sphere/Cylinder/Cone → Fracture card → Enable fracture → ↗**.

## Where it was found (searched all branches/commits of all 5 repos)

Method: full branch enumeration + filename/history search across every ref (partial clones,
`git log --all -- <path>`, `ls-tree` per branch). GitHub code search returns nothing for these terms because
it only indexes default branches — and on the default branches there is no fracture editor.

### 1. `SultanAladin/Frontier-` — **the main source** ✅
Branch **`arena/10e50e56-frontier`** (tip `4010ac4`, 2026-10-11). It is the only branch containing the newest
editor commits (`524a4f9`, `0c2214c`, 2026-10-10). Ten sibling arena branches contain the same editor folders
slightly older (`215a6f39`, `ff2840bc`, `561c913e`, `24db141b`, `5c85f535`, `4f07c82a`, `b5d061d9`,
`ca455774`, `f4cc60b8`, plus `9563020c` without ParticleEditor).

Key commits:
- `5835836` *Port FracturePanel.jsx and mount it on the object sheet* (2026-10-08)
- `a05133b` *Port the editor's base meshes and the fracture editor page to native C++* → `Engine/Editor/FractureEditorSurface.h`
- `b87f7ed` *Port the WindEditor dialog to the native editor, 1:1*
- `88181f6` *Merge the Particle Editor and give the gas emitter an editor of its own* (2026-10-09)
- `a8db8b9` *The expanded Fluid editor, natively: the page the gas card's arrow opens*
- `e5a33a8` *Add scene emitters to the browser editor and import the Frontier engine tree* (import of the browser editors)

Recovered from this branch (now under `Exhibits/`):
`ProjectZeroEditor/` (prebuilt self-contained `index.html`, 4.9 MB — no external assets),
`FractureEditor/` (prebuilt `index.html` + sources + `Captures/*.png` proof screenshots),
`Fluid/` (source; `dist/` built here), `ParticleEditor/` (no build step),
`FrontierEditor/` (source; `dist/` built here — the older editor, also on `Frontier-` main @ `28ec065`,
**without** fracture), plus `Docs/{BrowserEditorHistory,ParticleEditor,FrontierEditorComponents,
BaseGeometryAndFractureEditorNotes,MainEditorIntegration}.md` (copied to `Exhibits/Docs/`).

Note: branch `main` (2026-09-27) has `FrontierEditor` only — no ProjectZero/Fracture/Fluid/Particle apps.
Also on the arena branches are the **native C++ ports**, not runnable in a browser:
`Engine/Editor/{FractureCardSurface,FractureEditorSurface,FluidEditorSurface,GasCardSurface,
WindEditorSurface,FluidEditorControls}.h`, `Engine/VolumetricDynamics/Gas*.h`,
`Exhibits/Workbench/{Fracture,FractureEditor,FluidEditor,WindEditor,GasFluid}/Native*.cpp` +
proof galleries under `Exhibits/Gallery/{FractureEditorNative,FluidEditorNative,WindEditorNative,...}/`.
(Not copied here; they need the native Vulkan build.)

### 2. `unassignedinbox/Slate` — **the origin** (main branch is just a README; 165 arena branches)
- Branch `arena/01a0fd48-slate`: the original HTML Project-Zero + FractureEditor under
  `Frontier/Experimental/{ProjectZeroEditor,FractureEditor}/`, e.g.
  `70a50f59` *Unify fracture and plasticity workflows in an HTML authoring review* and
  `dee73c2a` *Bind fracture authoring to scene objects and preserve closed fragment geometry* (2026-10-05).
  This is an **earlier copy** of the same apps recovered newer from `Frontier-` above.
- Branch `arena/06b26e41-slate`: **`particle-editor/` = Flux Particle Studio** (`609de1c5`, 2026-10-08),
  a separate AE-style GPU particle editor with a prebuilt `dist/`. → copied to `Exhibits/FluxParticleStudio/`.
- Branch `arena/01a102c5-slate`: **`WebFluidLab/`** (`215549b3` *Realtime Fluid Lab: WebGPU/WebGL2 SPH*,
  2026-10-03). → copied to `Exhibits/WebFluidLab/`, `dist/` built here.
  (This branch also has an older `WebFluid/` snapshot in its history; the Lab superseded it.)

### 3. `SultanAladin/Frontier` — one related prototype
- `Documentation/Prototypes/FluidNodeEditor/FluidNodeEditor.html` is on `master`, but its JS
  (`FluidEditorEntry.js`, `Fluid/`, `Graph/`, `Presentation/`) is **missing there** — recovered complete
  (12 files) from branch **`smrt-soft-shadows`**. → `Exhibits/FluidNodeEditor/`.
  It loads three.js from `unpkg.com` via import map, so it needs internet in the browser.
- Its native `Internal/EngineContext/Interface/Workspaces/Simulation/` is a small C++ panel only;
  `Experimental/SlateReference/.../FractureSequence.js` on other branches is vehicle-proving-ground code,
  not an editor UI.

### 4. `SultanAladin/Slate` — nothing matching ❌
Filename search across all 20 branches + content grep of `master` HTML prototypes: no fracture/fluid/
wind/particle editor UI. It has the native C++ `Engine/SlateUI/.../EditorPanel` and HTML reference
prototypes (`References/UVeditor`, asset browsers, etc.), but no simulation editors.

### 5. `unassignedinbox/Frontier` (this repo) — was empty ❌
Single initial commit (`66a97d3`, just `# Frontier`). Everything recovered above now lives here.

## What's in this repo now — everything under `Exhibits/` (one reference place)

Catalog with open-URLs + origins: [`Exhibits/README.md`](Exhibits/README.md).

```
index.html                     launcher (open this)
FOUND.md                       this file
Exhibits/
  README.md                    ★ catalog — every exhibit, its URL and origin
  ProjectZeroEditor/           ★ the editor UI (prebuilt index.html)
  FractureEditor/              standalone fracture editor (prebuilt index.html, three.js render)
  Fluid/                       gas/smoke fluid editor — dist/ built here
  ParticleEditor/              standalone particle editor (no build)
  FrontierEditor/              older browser editor — dist/ built here
  RoadEditor/                  spline road editor from Slate (prebuilt dist/)
  SolidArc/                    geometry workspace from Frontier-
  FluxParticleStudio/          Flux particle editor from Slate (prebuilt dist/)
  FluidNodeEditor/             fluid node-editor from SultanAladin/Frontier (needs CDN at runtime)
  NodeFlowEditor/              node-flow editor + terrain from SultanAladin/Frontier (needs CDN at runtime)
  WebFluidLab/                 SPH fluid lab from Slate — dist/ built here
  CliffSequence/  Ocean/  DeformationIntegrator/  DistanceIntegrator/
  RadianceIntegrator/  RadianceProjection/  RadianceSequence/  SpatialHud/
                               remaining browser apps from the same Frontier- branch
  NativeSurfaces/              16 native C++ editor-surface headers (reference only, not runnable here)
  EngineContent/Fonts/         DM Sans TTFs the built Fluid CSS expects at ../../../EngineContent/…
  Docs/                        the editor docs copied from Frontier-
Docs/                          3 upstream review pages the Deform/Distance/Radiance exhibits link to
EngineContent/                 root font copy read by FractureEditor/Build.mjs (../../EngineContent)
```

Also from the same branches, added when consolidating into `Exhibits/`:
`road-editor/` (unassignedinbox/Slate `arena/06b26e41-slate`) → `RoadEditor/`;
`Documentation/Prototypes/NodeFlowEditor` (SultanAladin/Frontier `smrt-soft-shadows`, 11 files) → `NodeFlowEditor/`;
the rest of `Experimental/*` with an HTML entry (Frontier- `arena/10e50e56-frontier`) → the 9 folders above
(`Experimental/Liquid` + `Experimental/Water` are README-only stubs upstream, not copied); and
`Engine/Editor/*Surface.h` (+2 `*Controls.h`, 16 files, same branch) → `NativeSurfaces/` for reference.

**Layout is load-bearing:** Project-Zero opens `../FractureEditor/index.html?object=…`,
iframes `../Fluid/dist/index.html` and `../ParticleEditor/index.html`. Keep those four folders as siblings.

`FractureEditor/FracturePanel.html` is only the *template* (`INLINE_STYLE`/`INLINE_SCRIPT` placeholders)
that `Build.mjs` turns into `FractureEditor/index.html` — don't serve the template directly.

## Run it

Everything is static once built. From this repo's root:

```bash
python3 -m http.server 8080 --bind 0.0.0.0
# → http://localhost:8080/                      (launcher)
# → http://localhost:8080/Exhibits/ProjectZeroEditor/index.html
```

Rebuild the built ones (Node ≥ 22) if you change sources:

```bash
cd Exhibits/Fluid          && npm install && npm run build   # → dist/ (PLAYWRIGHT_SKIP_BROWSER_DOWNLOAD=1 to skip browser download)
cd Exhibits/FrontierEditor && npm install && npm run build   # → dist/
cd Exhibits/WebFluidLab    && npm install && npx tsc && npx vite build --base ./   # --base ./ so dist works when served from a subfolder (its config has no base)
cd Exhibits/FractureEditor && npm install && npm run build   # node Build.mjs → regenerates index.html (esbuild + three)
# ProjectZeroEditor/index.html is prebuilt; rebuilding it needs the full Frontier- tree
# (Build.mjs reads EngineContent/, Engine/, Exhibits/ from the original repo root).
```

## One repair made during recovery (read this if you rebuild)

Commit `6096936` (*Rename banned words across the tree*) had renamed three.js's `BufferAttribute` /
`BufferGeometry` → `StoreAttribute` / `StoreGeometry` in
`FractureEditor/SourceDepot/Fragmentation/src/core/convex.ts` — the follow-up restore commits missed that
file, so building the fracture editor **from source fails** on `arena/10e50e56-frontier`
(the committed prebuilt `index.html` predates the sweep and is unaffected).
Fix applied in this repo: that one import + its 4 uses renamed back to `Buffer*`.
Proof the fix is exact: `node CheckGeometry.mjs` passes, and `npm run build` now regenerates
`FractureEditor/index.html` **byte-identical** (same md5) to the committed prebuilt.
No other recovered file contains sweep-renamed three.js names (scanned).
Fonts were also copied to `EngineContent/Fonts/SunReference/` at this repo's root (what
`FractureEditor/Build.mjs` reads via `../../`) and kept under `Exhibits/EngineContent/…` (what the built
Fluid CSS requests at runtime).

## Verification done here

- All launcher targets served `200` with correct JS/CSS MIME types; every asset referenced by the built
  apps (Fluid, FrontierEditor, WebFluidLab, Flux) resolves; ParticleEditor's full ES-module graph resolves;
  Project-Zero's `../FractureEditor/`, `../Fluid/dist/`, `../ParticleEditor/` targets all resolve.
- `Exhibits/ParticleEditor: node CheckForceFields.mjs` → **PASS 91**.
- `Exhibits/Fluid: npm test` → **22/22 pass**; `npm run build` clean.
- `Exhibits/FractureEditor: node CheckGeometry.mjs` → **passed (168 material/primitive/seed cases…)** after the fix above.
- `FrontierEditor` and `WebFluidLab` production builds clean (WebFluidLab rebuilt with `--base ./`).
- Not run: the Playwright/Chromium screenshot checks (`CheckBrowser.mjs` etc.) — no browser binary in
  this sandbox. Their earlier evidence is preserved in `Exhibits/FractureEditor/Captures/`.

## Suggested next step (not done here)

If you want the fracture fragments rendered *inside* Project-Zero, the missing piece is a renderer for the
checker viewport / fracture card — the data path already exists (`FractureSpecification.js` publishes settings
via localStorage, `FractureDepot.js` stores fragments). The standalone Fracture page's three.js viewport
(`FracturePanel.js`) is the renderer to reuse.
