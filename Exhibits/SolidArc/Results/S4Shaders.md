# 📝 S4 — shared CAD shader modules (Frontier CadStroke / MatCapLookup)

**TL;DR** — Upstream `CadEdge` and `MatCapSurface` and the SolidArc edge / surface programs now draw from the same two Slang modules in `Engine/Render/Shader/`. Upstream raster evidence is byte-identical before and after; the SolidArc viewport is pixel-identical; SolidArc ctest 25/25 plus both upstream raster validations 🟢 (27/27).

## 🏷️ Modules

| Module | Contents | Included by |
|---|---|---|
| `CadStroke.slang` | `ClipEdgeBoundary`, `ClipCadSegment` (w > 1e-5, z ≥ 0), `ExpandCadStroke` (six-corner quad, Reach px across, Overlap px along), `CadStrokeCoverage` | `CadEdge`, `SolidArcEdge` (visible + x-ray occluded fragments) |
| `MatCapLookup.slang` | `MatCapCoordinates` (scaled normalize, cancelling-normal centre lookup, uv = (½ + ½x, ½ − ½y)) | `MatCapSurface`, `SolidArcSurface` (matcap layers, pulled 0.5 % inside the disc) |

## 🏁 Upstream raster evidence (SwiftShader, sha256 prefix)

| File | Before (65cf0f1 shaders) | After (shared modules) | Same |
|---|---|---|---|
| `MatCapRasterEvidence/MatCapBracket.ppm` | `da87cc6cd75621bc` | `da87cc6cd75621bc` | 🟢 |
| `MatCapRasterEvidence/MatCapCalibration.ppm` | `2863242de4ef131f` | `2863242de4ef131f` | 🟢 |
| `MatCapRasterEvidence/MatCapRasterChecks.json` | `03bdcee14b4f704a` | `03bdcee14b4f704a` | 🟢 |
| `MatCapRasterEvidence/MatCapThumbnail.ppm` | `8644c3519cfaf4ff` | `8644c3519cfaf4ff` | 🟢 |
| `SurfaceRasterEvidence/CadDepthOcclusion.ppm` | `2d97be33a9165b06` | `2d97be33a9165b06` | 🟢 |
| `SurfaceRasterEvidence/CadNearClip.ppm` | `1c103b19bbefd5c9` | `1c103b19bbefd5c9` | 🟢 |
| `SurfaceRasterEvidence/CadOrthographic.ppm` | `91ad0a92753d3c08` | `91ad0a92753d3c08` | 🟢 |
| `SurfaceRasterEvidence/CadPerspective.ppm` | `08c4785c1b56ae18` | `08c4785c1b56ae18` | 🟢 |
| `SurfaceRasterEvidence/CadRasterChecks.json` | `e33941104ee526a2` | `e33941104ee526a2` | 🟢 |
| `SurfaceRasterEvidence/CadThumbnail.ppm` | `e4f4499037e410ec` | `e4f4499037e410ec` | 🟢 |

Reflection JSON of `CadEdge*`, `CadSurface*` and `MatCapSurface*` entries: identical. SPIR-V of `CadEdgeVertex`, `CadEdgeFragment` and `MatCapSurfaceFragment` changed (code moved into functions); `CadSurface*` and `MatCapSurfaceVertex` unchanged.

## 🔍 SolidArc

| Check | Result |
|---|---|
| `ViewportLook.txt` captures (Specimen, BodySelected, Maximized) vs S3c | 🟢 viewport identical; only the status-bar frame time differs |
| `MatcapWalkthrough.txt` captures 90–95, old vs new archive | 🟢 viewport identical (0 px); only the replay-time text differs |
| SolidArc ctest | 🟢 25/25 (+ `SurfaceRasterizationValidation`, `MatCapRasterizationValidation`) |

`CadSurface` is not shared: its fixed facing tint has no SolidArc counterpart (Studio shading, zebra, section caps and selection tints stay in `SolidArcSurface`).
