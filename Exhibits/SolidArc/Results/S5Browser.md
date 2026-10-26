# 📝 S5 — SolidArc in the Frontier project browser

**TL;DR** — The upstream project browser now treats `.solidarc` as a CAD file. It decodes each document with the SolidArc library, renders the real bodies through the browser's own reference Vulkan renderer (thumbnail and live reader), shows a refusal for damaged files instead of a stand-in, and **Open in SolidArc** starts `SolidArcWorkspace --open <file>`. `SolidArcBrowserValidation` passes 15/15 on SwiftShader, and the started workspace process captured the opened document. ctest 28/28 (every SolidArc test plus both upstream raster validations).

## 🏷️ What changed

| Area | Change | File |
|---|---|---|
| Catalogue | `.solidarc` → CAD; workspace label **SolidArc / CAD**; launch target **SolidArc workspace**; entries and preview configurations carry document triangles + residence key | `Engine/ContentInterchange/ProjectBrowserCatalogue.h` |
| Renderer | `ResidenceFor` uses the configuration's document triangles (key `document:<path:size:mtime>`) instead of the actuator fixture; same BrowserPreview / BrowserDrafting programs | `Engine/Render/BrowserPreviewRendering.cpp` |
| Panel | `QueryDocumentGeometry`, `QueryConfiguredThumbnail`, `LaunchWorkspaceDocument` callbacks; live reader, zoom and capture enabled for decoded documents; refusal text; **Open in SolidArc** label; hand-off dialog reports the outcome | `UI/Editor/ProjectBrowserPanel.{h,cpp}` |
| SolidArc preview | Load → replay → tessellate → browser frame: (x, y, z) → (x, z, −y), fit ≤ 5.6 wide / ≤ 3.2 tall, on the reference floor; bodies cycle the five reference surfaces | `UI/SolidArc/Browser/SolidArcBrowserPreview.{h,cpp}` |
| SolidArc link | `ConnectSolidArcBrowser`, `LaunchSolidArcWorkspace` (CreateProcessW / fork + exec with an exec-failure pipe), `LocateSolidArcWorkspace` (beside the running exe) | `UI/SolidArc/Browser/SolidArcBrowserLink.{h,cpp}` |
| Windows host | Connects the link; adds `QueryConfiguredThumbnail` on the shared preview renderer | `Experimental/Frontier/Native/WindowsFrontier.cpp` |
| Build | `FrontierSolidArcDocument` (no Vulkan / ImGui) split from `FrontierSolidArcUI`; `FrontierSolidArcBrowser`; `Frontier` links it and depends on `SolidArc`; `stb_image` made internal (`STB_IMAGE_STATIC`) so it can sit beside upstream's copy | `Experimental/SolidArc/Native/SolidArc.cmake`, `UI/SolidArc/Document/SolidArcImagePlane.cpp` |
| Overlay | The five upstream files are replaced only while their sha256 matches the 65cf0f1 pin (still matching at 18d988e) | `Experimental/SolidArc/Native/Tooling/OverlayFrontier.py` |

## 🏁 SolidArcBrowserValidation (SwiftShader, 1600 × 960)

| Expectation | Observed | Result |
|---|---|---|
| Three `.solidarc` files attach as CAD entries | 3 attached | 🟢 |
| CAD category and SolidArc labels | SolidArc / CAD · SolidArc workspace | 🟢 |
| CAD filter lists the attached documents first | Damaged, Assembly study, Bracket, Precision mount, Actuator sketches | 🟢 |
| Bracket decodes to real geometry | 1 body · 11 faces · 8 steps | 🟢 |
| Assembly study decodes to three bodies | 3 bodies · 15 faces · 12 steps | 🟢 |
| Damaged document is refused, not substituted | TOML: Error while parsing table header | 🟢 |
| Library thumbnails rendered for both documents | reference renderer, document triangles | 🟢 |
| Reader live view rendered from the document | live view ready | 🟢 |
| Sample entries keep the preview-only launch dialog | no hand-off | 🟢 |
| Open in SolidArc starts the workspace process | SolidArc workspace started with this document | 🟢 |
| The started workspace opened Bracket.solidarc | child capture written | 🟢 |
| Preview renderer accepted every configuration | no refusals | 🟢 |

The full table, including the setup rows, is in `S5Browser/BrowserValidation.md`. On Linux the hand-off starts `SolidArcCapture`, which accepts the same `--open` argument as `SolidArcWorkspace`.

## 🔍 Captures

| Capture | Shows |
|---|---|
| `S5Browser/BrowserLibrary.png` | Library: Assembly study and Bracket thumbnails (CAD contours with hidden lines), Damaged placeholder, sample tiles unchanged |
| `S5Browser/BrowserReader.png` | Reader: live orbitable view of the 3-body document, "SolidArc · Reference Vulkan", 5,482 triangles, Document row, **Open in SolidArc** |
| `S5Browser/BrowserRefusal.png` | Damaged file: decoder message, "SolidArc · document refused", no stand-in image |
| `S5Browser/BrowserSampleLaunch.png` | Reference: unchanged sample launch dialog (preview only) |
| `S5Browser/BrowserHandOff.png` | Hand-off dialog: "Opened in SolidArc.", the target and the file path |
| `S5Browser/SolidArcOpened.png` | The started SolidArc process with Bracket.solidarc loaded (8 steps, Fillet1 / Chamfer1) |

## ⚠️ Open

- The Windows host (`WindowsFrontier.cpp`, CreateProcessW branch) is not compiled here; the Linux harness runs the same link code through the fork + exec branch.
- Upstream `ProjectBrowserValidation` passes from the repository root (770 drawn states). Under ctest it fails at "Blank project creation" because its `add_test` has no `WORKING_DIRECTORY`; this test and `CreateProject` are untouched.
- Build note: after the overlay seats a replaced file, touch it. Otherwise ninja can keep objects built from the pristine upstream copy, which leaves a header layout mismatch.
