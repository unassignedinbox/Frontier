# 📝 PLAN — SolidArc port into SultanAladin/Frontier (approved decisions, 2026-10-23)

**TL;DR** — SolidArc becomes a second workspace application in `SultanAladin/Frontier`, built like Texture Workspace. Its three panels (Outliner, Viewport, Inspector) are ported 1:1 from the HTML UI (`Experimental/SolidArc`) using Frontier's own controls, theme, dockspace, CAD / matcap shaders and project browser. The geometry library moves to production under `Engine/GeometryProcessing/`. The camera's shared parts move to Engine. Plasticity navigation stays in the SolidArc app.

## 🏷️ Approved decisions

| Topic | Decision | Source |
|---|---|---|
| Application name | **SolidArc**; the word "Drafting" is retired | user |
| Panels to port | **Outliner, Viewport, Inspector, 1:1** with the HTML look | user |
| Geometry library location | **`Engine/GeometryProcessing/`** (production). The word "Modelling" is not used | user |
| Camera | Keep the SolidArc camera. Its shared parts (orbit / pan / dolly, view presets, fit-to-bounds, pixel → ray, projection) move to Engine; **Plasticity navigation** stays in `UI/SolidArc/` | user |
| Target line | Upstream branch **`Frontier`** (PR base). `arena/e9e7fc61-frontier` is the head of PR #16 | `gh pr list` |

## 🧩 Target layout (upstream tree)

| Path | Contents | Now |
|---|---|---|
| `Engine/GeometryProcessing/` | topology, geometry, Booleans, blends, sketch solver, exchange, tessellation, assembly, drawing | `Engine/GeometryProcessing/` |
| `Engine/Render/` (shared camera) | orbit camera mathematics, framing, view presets, picking ray | `Experimental/DraftingWorkspace/Source/Viewport/DraftingCamera.*` |
| `UI/SolidArc/` | Outliner, Viewport, Inspector panels, Plasticity navigation and keymap | `Experimental/DraftingWorkspace/Source/Interface/` |
| `Experimental/SolidArc/Native/` | `SolidArcApplication.cpp` → exe `SolidArcWorkspace` | DraftingWorkspace host |
| `Exhibits/SolidArc/{README.md, Code, Images, Results}` | validation, walkthroughs, captures | `Exhibits/DraftingWorkspace`, `Exhibits/GeometryProcessing` |

## 🏁 Steps

| Step | Work | Evidence |
|---|---|---|
| S0 | HTML reference captures of the 3 panels (headless Chromium from npm) | reference PNGs |
| S1 | Adopt the upstream CMake, ExternalPackages pins and `VulkanPresentation`; drop duplicate ImGui / GLFW / volk pins | builds, all tests pass |
| S2 | Move the library to `Engine/GeometryProcessing/`; rename Drafting → SolidArc; `.drafting.toml` → `.solidarc` (old files still open) | kernel ctest + workspace ctest unchanged |
| S3 | Rebuild the Outliner and Inspector with ControlKit / PanelDockspace / ThemeStructure, matching `WorkspacePanel.css` | side-by-side HTML vs C++ captures |
| S4 | Viewport on upstream `CadSurface` / `CadEdge` / `MatCapSurface`, shared camera in Engine, Plasticity navigation kept | viewport gates + captures |
| S5 | Project browser: Cad category, launch path, thumbnails | browser capture |
| S6 | Validation to `Exhibits/SolidArc/`; Windows cross-build and package | test tables + `.exe` |
| S7 | PR into upstream `Frontier` after PR #16 merges (Actions only with approval) | merged tree builds |

## 🚧 Progress

| Step | State |
|---|---|
| S0 | 🚧 3 HTML reference captures in `Exhibits/SolidArc/Images/HtmlReference/` (empty, example loaded, body selected) |
| S2 (library move) | 🟢 `Experimental/SolidArcRemastered` → `Engine/GeometryProcessing` (library `FrontierGeometryProcessing`, namespace `Frontier::GeometryProcessing`, root-relative includes); validation, tooling and viewer → `Exhibits/GeometryProcessing/Code`; tables → `Results/`, galleries → `Images/`. The HTML app's legacy service `Engine/Modelling/SolidArc` → `Experimental/SolidArc/Service`. Gates: GeometryProcessing ctest 30/30 (3025 s), DraftingDocumentValidation 🟢, HTML service rebuilt from the new path |
| S2 (rename + layout) | 🟢 DraftingWorkspace → **SolidArc**: UI library `FrontierSolidArcUI` from `UI/SolidArc/{Document, Interface, Viewport, Shaders}` (namespace `Frontier::SolidArc`); app project `Experimental/SolidArc/Native/` (exe target `SolidArc`, output `SolidArcWorkspace`); validations, capture and 24 scripts in `Exhibits/SolidArc/Code`, reports in `Results/`. Documents: `Format = 'SolidArc'`, extension `.solidarc`; pre-rename `DraftingWorkspace` / `.drafting.toml` files still open (W8 check). HTML host scripts `Native/` → `Service/Host/`. Gates: SolidArc ctest **25/25** on SwiftShader (DocumentValidation 🟢, ViewportValidation 🟢, walkthroughs 1091/1091 re-run + Blueprint 48 needs the BMW picture) |
| S1 (upstream build) | 🟢 `OverlayFrontier.py` links the SolidArc tree into an upstream checkout (`65cf0f1`), pins volk and seats `IconDomain::SolidArc`; `SolidArc.cmake` builds on upstream `FrontierImGui` + `FrontierInterface`. Gates: upstream configure + build EXIT 0, DocumentValidation 🟢, SolidArc ctest **25/25** on SwiftShader. 🚧 still open: standalone build via the overlay, windowed host on `VulkanPresentation` |
| S3a (shell + Outliner) | 🟢 HTML header (tab, nav, Import / Export / Save as, kernel tag, Save), document bar, status bar, grid 300 \| 1fr \| 330; `SolidArcOutliner` (tiles, Ctrl+F search, filter chips, collection rows with ACTIVE / SELECTED, eye toggles, construct launcher); construct popover (Solids); construction history strip with undo / redo. Drawn with `SolidArcStyleSheet` on upstream `PixelSpace` / `IconArt` (57 SolidArc SVG icons, DM Sans Light / Regular). Captures: `Exhibits/SolidArc/Results/S3aShell/`. Gates: SolidArc ctest 25/25 with the new shell. Next: S3b Inspector, S3c viewport chrome |
| S3 (navigation) | 🟢 Plasticity navigation in `SolidArcNavigation` |
| S4 (shared camera, part) | 🟢 `Engine/Render/OrbitCamera` (`FrontierOrbitCamera`): Z-up orbit / pan / dolly, view presets, fit-to-bounds, Vulkan clip matrices, pixel rays; fields named `Azimuth` / `Elevation` / `Distance` / `Target` like upstream `BrowserPreviewCamera`. SolidArc keeps scene framing and edge box selection in `SolidArcCamera`; Plasticity navigation stays in the interface (moves to `SolidArcNavigation` in S3) |

## ⚠️ Open points

- 🟢 Upstream write access: accepted (`push: true` on `SultanAladin/Frontier`).
- 🚩 The histories are unrelated (`git merge-base` is empty), so the port lands as new commits on top of upstream `Frontier`, not as a merge of this branch.
