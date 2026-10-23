# 🧩 SolidArc — port evidence

**TL;DR** — Evidence for the SolidArc application port (`Documentation/PLAN-SolidArcPort.md`). Step S0 holds the reference captures of the HTML SolidArc UI (`Experimental/SolidArc`), which the C++ Outliner, Viewport and Inspector are matched against 1:1.

| Folder | Contents |
|---|---|
| `Images/HtmlReference/` | Headless Chromium captures of the HTML UI at 1600 × 960 (vite on port 5176, native geometry service on 8081) |

## 🔍 Reference captures

| Capture | State |
|---|---|
| [`EmptyWorkspace.png`](Images/HtmlReference/EmptyWorkspace.png) | New document: Outliner (counts, search, filter chips, collection), viewport (grid, axes, view cube, tool rail, Select / Cut / Trim bar, history strip), Inspector (empty state, Start constructing tiles) |
| [`ExampleLoaded.png`](Images/HtmlReference/ExampleLoaded.png) | `3D example` loaded: two bodies in the Outliner, shaded viewport with silhouette edges, 5 history steps |
| [`BodySelected.png`](Images/HtmlReference/BodySelected.png) | FirstBody picked in the viewport: active row with green rail, Inspector header with Faces / Edges / Vertices tiles, Bevel history, Geometry / Transform / Topology tabs, Dimensions fields + sliders |

📝 Capture tool: `@sparticuz/chromium` + `puppeteer-core` from npm in `_ClaudeScratch/Capture` (scratch, not committed), WebGL through SwiftShader.
