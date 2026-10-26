# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 900 · 410 frames · 51.0995 s · slowest frame 517.179 ms
- Script: ../../Exhibits/SolidArc/Code/Scripts/ConstructDrawingWalkthrough.txt
- Expectations: 50 / 50 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 13 | `steps == 0` | 0 | 🟢 |
| 19 | `construct-section == 1` | 1 | 🟢 |
| 23 | `construct-section == 0` | 0 | 🟢 |
| 28 | `drawing-solid == 1` | 1 | 🟢 |
| 29 | `drawing-solid-kind == 0` | Block | 🟢 |
| 30 | `construct-combination == 4` | 4 | 🟢 |
| 35 | `drawing-points == 1` | 1 | 🟢 |
| 41 | `drawing-points == 2` | 2 | 🟢 |
| 44 | `drawing-lift ~ 1` | 1 | 🟢 |
| 48 | `drawing-solid == 0` | 0 | 🟢 |
| 49 | `refused == 0` | 0 | 🟢 |
| 50 | `bodies == 1` | 1 | 🟢 |
| 51 | `volume ~ 3` | 3 | 🟢 |
| 56 | `drawing-solid-kind == 1` | Cylinder | 🟢 |
| 68 | `refused == 0` | 0 | 🟢 |
| 69 | `bodies == 2` | 2 | 🟢 |
| 70 | `volume ~ 3.942477796` | 3.942477796 | 🟢 |
| 77 | `drawing-solid-kind == 4` | Cone | 🟢 |
| 87 | `refused == 0` | 0 | 🟢 |
| 88 | `bodies == 3` | 3 | 🟢 |
| 89 | `volume ~ 4.319468915` | 4.319468915 | 🟢 |
| 96 | `drawing-solid-kind == 6` | Prism | 🟢 |
| 97 | `drawing-sides == 6` | 6 | 🟢 |
| 100 | `drawing-sides == 7` | 7 | 🟢 |
| 110 | `refused == 0` | 0 | 🟢 |
| 111 | `bodies == 4` | 4 | 🟢 |
| 112 | `volume ~ 5.107555049` | 5.107555049 | 🟢 |
| 128 | `refused == 0` | 0 | 🟢 |
| 129 | `bodies == 5` | 5 | 🟢 |
| 130 | `volume ~ 6.884083841` | 6.884083841 | 🟢 |
| 139 | `drawing-elevation ~ 1` | 1 | 🟢 |
| 144 | `drawing-lift ~ 0.5` | 0.5 | 🟢 |
| 147 | `refused == 0` | 0 | 🟢 |
| 148 | `bodies == 6` | 6 | 🟢 |
| 149 | `volume ~ 7.134083841` | 7.134083841 | 🟢 |
| 159 | `drawing == Polygon` | Polygon | 🟢 |
| 167 | `drawing == Polygon` | Polygon | 🟢 |
| 168 | `drawing-sides == 7` | 7 | 🟢 |
| 171 | `drawing-sides == 8` | 8 | 🟢 |
| 176 | `drawing-sides == 6` | 6 | 🟢 |
| 179 | `drawing-sides == 5` | 5 | 🟢 |
| 183 | `drawing == 0` |  | 🟢 |
| 184 | `refused == 0` | 0 | 🟢 |
| 185 | `sketches contains Polygon1` | Block1,Cylinder1,Cone1,Prism1,Torus1,Block2,Polygon1 | 🟢 |
| 197 | `drawing == 0` |  | 🟢 |
| 206 | `drawing-solid == 0` | 0 | 🟢 |
| 207 | `construct-open == 1` | 1 | 🟢 |
| 208 | `construct-combination == 4` | 4 | 🟢 |
| 212 | `construct-open == 0` | 0 | 🟢 |
| 213 | `bodies == 6` | 6 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 200ConstructSolids | 15 | `00d69a43001d17bd` |
| 201ConstructSketch | 24 | `921bd2f62841a6ec` |
| 202BoxBase | 55 | `e0e8ce4def389d59` |
| 203BoxHeight | 68 | `fa0ac9337b3c8a74` |
| 204CylinderHeight | 105 | `adf80f6fe56991a9` |
| 205ConeHeight | 147 | `07adee4f9228d411` |
| 206PrismHeight | 193 | `33bd32e370f538d1` |
| 207TorusTube | 235 | `5d0a9fc6795bc1a7` |
| 208PolygonDimensions | 312 | `3f7caeebb57cd369` |
| 284PolygonSides | 346 | `938fd236fd822709` |
| 209LineDimensions | 372 | `877b4fc2f926ef62` |
| 210ConstructExact | 397 | `6b42d69e52163843` |
| 211ConstructBodies | 410 | `b03304c39f327262` |

## Journal

- New document (previous recovery copy kept)
- Block: click the first corner · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Block: click the opposite corner
- Block: move up, click the height
- Block1 · New body · 0 ms
- Cylinder: click the base centre · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Cylinder: click the radius
- Cylinder: move up, click the height
- Cylinder1 · New body · 1 ms
- Cone: click the base centre · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Cone: click the radius
- Cone: move up, click the apex height
- Autosaved ../tmp/rt/ConstructDrawing/Rec.solidarc
- Cone1 · New body · 1 ms
- Prism: click the centre · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- 7 sides
- Prism: click a vertex (+ / − sides)
- Prism: move up, click the height
- Autosaved ../tmp/rt/ConstructDrawing/Rec.solidarc
- Prism1 · New body · 1 ms
- Torus: click the centre · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Torus: click the major radius (ring centre line)
- Torus: click the tube (minor) radius
- Autosaved ../tmp/rt/ConstructDrawing/Rec.solidarc
- Torus1 · New body · 5 ms
- Block: click the first corner · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Block: click the opposite corner
- Block: move up, click the height
- Block2 · New body · 2 ms
- Polygon on plane XY (or the body face under the first click): click the centre, then a vertex · + / − sides · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Autosaved ../tmp/rt/ConstructDrawing/Rec.solidarc
- Polygon · 7 sides · wheel (or + / −) changes the sides · click / right-click / Enter confirms · Esc aborts
- 8 sides
- 7 sides
- 6 sides
- 5 sides
- Polygon1 · 2 ms
- Line on plane XY (or the body face under the first click): click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Line aborted (document untouched)
- Block: click the first corner · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Block: click the opposite corner
- Autosaved ../tmp/rt/ConstructDrawing/Rec.solidarc
- Selection cleared
- Framed the document (Space)
