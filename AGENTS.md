# Frontier UI & Viewer Workspace — Agent Instructions

> **Read this file at the start of every session.** It preserves all saved repositories, visual references,
> naming rules, prototype approval gates, and server/render commands so the user never has to repeat them.

---

## 1 · Saved Repositories & Reference Sources

| Role | Repository / Path | Branch | Commit / Tag | Local Mirror |
| --- | --- | --- | --- | --- |
| **Component Viewer + Asset Viewer Baseline** | `https://github.com/SultanAladin/Frontier` (`Experimental/ComponentViewer` + sibling `Experimental/` deps) | `arena/19265b6e-frontier` | `1fef98c8dc1131d9f38637bd6f300843ff05d807` | `Experimental/` |
| **UI Visual Inspiration (General UI)** | `https://github.com/streamlinkinbox/Frontier-OLD` (`SourceImages/General UI/`) | `main` | `4cab9133ef01022019f0bf4c71b9b328791306bd` | `Reference/GeneralUI/` |
| **Frontier Naming & Style Governance** | `https://github.com/SultanAladin/Frontier` (`EngineDocs/AgenticInstuctions/`, `SYSTEM_NOMENCLATURE.md`) | `arena/19265b6e-frontier` | `1fef98c8dc1131d9f38637bd6f300843ff05d807` | `EngineDocs/AgenticInstuctions/`, `SYSTEM_NOMENCLATURE.md` |

Full provenance and approval ledger live in `Documentation/SAVED_REPOS.md`.

---

## 2 · Mandatory Governance (Read First)

1. `EngineDocs/AgenticInstuctions/SKILL-Naming.md` — master naming procedure (§0 construct-from-mechanism + 6-gate reject test) and banned blacklist.
2. `SYSTEM_NOMENCLATURE.md` — local naming exceptions, proposals, and banned words.
3. `EngineDocs/AgenticInstuctions/SKILL-Emoji.md` — sole authority on approved emojis (plain `⭐` only; `🌟` / `✨` and non-red/green dots are forbidden).
4. `EngineDocs/AgenticInstuctions/SKILL-CompactOutputProtocol.md` — compact tables/diagrams first; mandatory comparison table before asking any user decision.
5. `EngineDocs/AgenticInstuctions/SKILL-formatting.md` — source & Markdown formatting rules.

---

## 3 · Non-Negotiable Product & Approval Rules

| # | Rule | Enforcement |
| --- | --- | --- |
| **R1** | **Original Frontier Naming — Zero Engine Cloning** | This is **NOT Unreal Engine** and must never copy naming from Unreal, Unity, Godot, Blender, or any other engine (ban `Blueprint`, `BP_`, `Actor`, `Pawn`, `Construction Script`, `Static Mesh`, `Details`, `Inspector`, `Prefab`, `GameObject`, `Nanite`, `Parent/Child`, `Root`, `Hidden in game`, etc.). Every proposed name must be creative, technical, domain-grounded, and **approved by the user** before applying to the viewers. |
| **R2** | **Separate Prototype Gate (One by One)** | Build every UI card/panel redesign as its own **isolated standalone HTML prototype** under `Prototypes/NN-<Name>/`. **Never** merge or inject an unapproved prototype into `Experimental/ComponentViewer/` (Component Viewer or Asset Viewer). Integrate into the live viewers **only after explicit user approval** for that specific prototype. |
| **R3** | **Visual Inspiration from `Reference/GeneralUI/`** | Derive surface styling, typography, spacing, micro-visualizations, and card anatomy directly from the 9 reference images in `Reference/GeneralUI/` (sourced from `streamlinkinbox/Frontier-OLD/SourceImages/General UI`). |
| **R4** | **Interactive Infographic UI First** | Prefer **interactive infographic UI** and unique tactile micro-controls (hatched vs active comparison bars, clickable stepped telemetry curves, segmented range-zone calibration rails, concentric radial rings, interactive dot-matrix maps, orbital pads, rotary dials) over walls of plain `<input type="range">` sliders. Use sliders only where genuinely superior, styled as tactile instruments. |
| **R5** | **Render & Visually Inspect Every Card** | Always render standalone HTML cards (and affected viewer pages) to PNG via `Tools/RenderHtml.mjs` and inspect every rendered screenshot with `read_file` before presenting to the user. Verify layout, contrast, alignment, and interactive states visually — never ship blind HTML. |
| **R6** | **Component Viewer Theme Unification + Bespoke Layout** | Share Component Viewer's exact styling and theme colour system (`Experimental/Shared/Studio/Studio.css`: `#090909` canvas, `#121212` docks, `#171717` panels, `#262626` seams, `#ffb238` amber selection, `#46d267` green, `#4d9bff` blue, `#ff6b5e` red, `#e8e8e8` white active pills, `DM Sans`, trapezoid `.document-tab`) across all Frontier studio prototypes while keeping each editor's layout, workflow, and tactile instruments purpose-built for its domain. |
| **R7** | **Multi-Part Assets, Complete Settings & Viewport Debugging** | Asset tools must support multi-part imported assets cleanly in both the Outliner and 3D viewport (expandable part lists, selection/focus, visibility, soloing, per-part inspection). Every opened sub-record inspector (Distance Field, UV Atlas, Surface Strata, Form & Bounds, Portrait Capture, Surface Finish, Stress Fracture, Material Studio) must expose its **full real engine parameter set** and paired **viewport viewing & debugging controls** directly inside the stage. |

---

## 4 · Visual Theme & Component Viewer Styling (`Experimental/Shared/Studio/Studio.css`)

- **Canonical Surfaces:** `#090909` outer canvas, `--dock: #121212`, `--panel: #171717`, `--seam: #262626` `2px` hairline splits, `#191919` bento tiles, `#0b0b0d` viewport well, `#0a0a0a` tab strip with trapezoid `.document-tab` (`#26262c` inactive / `#121212` active).
- **Canonical Accents:** `--amber: #ffb238` (active selection `3px` left stripe & warning/stale state), `--green: #46d267` (ready/live/switches), `--blue: #4d9bff` (entity markers), `--red: #ff6b5e` (errors), `#e8e8e8` white active pills (`#111` ink), text hierarchy `#f0f0f0` / `#c4c4c4` / `#888` / `#5c5c5c`.
- **Canonical Controls & Typography:** `DM Sans` (`300/400/500`), pill `.segmented` switchers, `26px` pill range tracks (`#454545` fill on `#242424`), `.switch` toggles, RGB `.vec3` axis badges (`#e0605a` / `#6fcf6a` / `#5a8de0`), `.cam-hud` dial + focal chips, plus tactile infographic instruments from `Reference/GeneralUI/` styled in this unified theme.

---

## 5 · Local Servers & Visual Rendering Commands

| Target | Command | Port | Route |
| --- | --- | --- | --- |
| **Component Viewer (Baseline)** | `python3 Experimental/ComponentViewer/Serve.py 8095` | `8095` | `/Experimental/ComponentViewer/` |
| **Asset Viewer Prototype (Baseline)** | `python3 Experimental/ComponentViewer/ServeAssetViewer.py 8096` | `8096` | `/Experimental/ComponentViewer/AssetViewer.html` |
| **Standalone UI Prototype 01 — Surface Strata Card** | `python3 Prototypes/ServePrototypes.py 8097` | `8097` | `/Prototypes/01-SurfaceStrataCard/` |
| **Standalone UI Prototype 02 — SpecimenBay Asset Previewer** | `python3 Prototypes/02-AssetPreviewer/ServeSpecimenBay.py 8098` | `8098` | `/Prototypes/02-AssetPreviewer/` |
| **Standalone UI Prototype 03 — Surface Finish Atelier (Material Studio)** | `python3 Prototypes/03-MaterialStudio/ServeMaterialStudio.py 8099` | `8099` | `/Prototypes/03-MaterialStudio/` |

### Rendering HTML Cards to PNG for Visual Inspection

```bash
node Tools/RenderHtml.mjs <url-or-file> <output.png> [width] [height]
```

- Uses headless Chromium (`@sparticuz/chromium` + `puppeteer-core` installed in `/tmp/render/node_modules`) with SwiftShader WebGL and bundled `al2023` shared libs in `/tmp/al2023/lib`.
- Store inspection renders in `_Scratch/renders/` (git-ignored) and inspect via `read_file`.
