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
| `Engine/GeometryProcessing/` | topology, geometry, Booleans, blends, sketch solver, exchange, tessellation, assembly, drawing | `Experimental/SolidArcRemastered/Source/` |
| `Engine/Render/` (shared camera) | orbit camera mathematics, framing, view presets, picking ray | `Experimental/DraftingWorkspace/Source/Viewport/DraftingCamera.*` |
| `UI/SolidArc/` | Outliner, Viewport, Inspector panels, Plasticity navigation and keymap | `Experimental/DraftingWorkspace/Source/Interface/` |
| `Experimental/SolidArc/Native/` | `SolidArcApplication.cpp` → exe `SolidArcWorkspace` | DraftingWorkspace host |
| `Exhibits/SolidArc/{README.md, Code, Images, Results}` | validation, walkthroughs, captures | `Exhibits/DraftingWorkspace`, `Exhibits/SolidArcRemastered` |

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

## ⚠️ Open points

- 🚩 Upstream write access: the invitation (write) to `SultanAladin/Frontier` is pending, not accepted.
- 🚩 The histories are unrelated (`git merge-base` is empty), so the port lands as new commits on top of upstream `Frontier`, not as a merge of this branch.
