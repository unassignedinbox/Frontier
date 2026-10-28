# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 810 · 411 frames · 35.8313 s · slowest frame 330.895 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/ConstructDrawingWalkthrough.txt
- Expectations: 50 / 50 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 16 | `steps == 0` | 0 | 🟢 |
| 22 | `construct-section == 1` | 1 | 🟢 |
| 26 | `construct-section == 0` | 0 | 🟢 |
| 31 | `drawing-solid == 1` | 1 | 🟢 |
| 32 | `drawing-solid-kind == 0` | Block | 🟢 |
| 33 | `construct-combination == 4` | 4 | 🟢 |
| 38 | `drawing-points == 1` | 1 | 🟢 |
| 44 | `drawing-points == 2` | 2 | 🟢 |
| 47 | `drawing-lift ~ 1` | 1 | 🟢 |
| 51 | `drawing-solid == 0` | 0 | 🟢 |
| 52 | `refused == 0` | 0 | 🟢 |
| 53 | `bodies == 1` | 1 | 🟢 |
| 54 | `volume ~ 3` | 3 | 🟢 |
| 59 | `drawing-solid-kind == 1` | Cylinder | 🟢 |
| 71 | `refused == 0` | 0 | 🟢 |
| 72 | `bodies == 2` | 2 | 🟢 |
| 73 | `volume ~ 3.942477796` | 3.942477796 | 🟢 |
| 80 | `drawing-solid-kind == 4` | Cone | 🟢 |
| 90 | `refused == 0` | 0 | 🟢 |
| 91 | `bodies == 3` | 3 | 🟢 |
| 92 | `volume ~ 4.319468915` | 4.319468915 | 🟢 |
| 99 | `drawing-solid-kind == 6` | Prism | 🟢 |
| 100 | `drawing-sides == 6` | 6 | 🟢 |
| 103 | `drawing-sides == 7` | 7 | 🟢 |
| 113 | `refused == 0` | 0 | 🟢 |
| 114 | `bodies == 4` | 4 | 🟢 |
| 115 | `volume ~ 5.107555049` | 5.107555049 | 🟢 |
| 131 | `refused == 0` | 0 | 🟢 |
| 132 | `bodies == 5` | 5 | 🟢 |
| 133 | `volume ~ 6.884083841` | 6.884083841 | 🟢 |
| 142 | `drawing-elevation ~ 1` | 1 | 🟢 |
| 147 | `drawing-lift ~ 0.5` | 0.5 | 🟢 |
| 150 | `refused == 0` | 0 | 🟢 |
| 151 | `bodies == 6` | 6 | 🟢 |
| 152 | `volume ~ 7.134083841` | 7.134083841 | 🟢 |
| 162 | `drawing == Polygon` | Polygon | 🟢 |
| 170 | `drawing == Polygon` | Polygon | 🟢 |
| 171 | `drawing-sides == 7` | 7 | 🟢 |
| 174 | `drawing-sides == 8` | 8 | 🟢 |
| 179 | `drawing-sides == 6` | 6 | 🟢 |
| 182 | `drawing-sides == 5` | 5 | 🟢 |
| 186 | `drawing == 0` |  | 🟢 |
| 187 | `refused == 0` | 0 | 🟢 |
| 188 | `sketches contains Polygon1` | Block1,Cylinder1,Cone1,Prism1,Torus1,Block2,Polygon1 | 🟢 |
| 200 | `drawing == 0` |  | 🟢 |
| 209 | `drawing-solid == 0` | 0 | 🟢 |
| 210 | `construct-open == 1` | 1 | 🟢 |
| 211 | `construct-combination == 4` | 4 | 🟢 |
| 215 | `construct-open == 0` | 0 | 🟢 |
| 216 | `bodies == 6` | 6 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 200ConstructSolids | 16 | `a6358dec78503f33` |
| 201ConstructSketch | 25 | `fa61ef421e064126` |
| 202BoxBase | 56 | `6d8d9b46421f8a42` |
| 203BoxHeight | 69 | `8039826ba031c1ba` |
| 204CylinderHeight | 106 | `710af5b5600e1940` |
| 205ConeHeight | 148 | `b52a9a199d6f03e1` |
| 206PrismHeight | 194 | `d96a5718d9fce0ce` |
| 207TorusTube | 236 | `56a9ec7e2fd372e2` |
| 208PolygonDimensions | 313 | `740de50b9a14fcd1` |
| 284PolygonSides | 347 | `4540f60480df418f` |
| 209LineDimensions | 373 | `24447a7f56270ba7` |
| 210ConstructExact | 398 | `694788feca6232e8` |
| 211ConstructBodies | 411 | `c5618ecd5ea8b701` |

## Journal

- New document (previous recovery copy kept)
- Block: click the first corner · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Block: click the opposite corner
- Block: move up, click the height
- Block1 · New body · 0 ms
- Cylinder: click the base centre · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Cylinder: click the radius
- Cylinder: move up, click the height
- Cylinder1 · New body · 0 ms
- Cone: click the base centre · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Cone: click the radius
- Cone: move up, click the apex height
- Autosaved _ClaudeScratch/SolidArcWorkspace/ConstructRecovery.solidarc
- Cone1 · New body · 1 ms
- Prism: click the centre · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- 7 sides
- Prism: click a vertex (+ / − sides)
- Prism: move up, click the height
- Autosaved _ClaudeScratch/SolidArcWorkspace/ConstructRecovery.solidarc
- Prism1 · New body · 1 ms
- Torus: click the centre · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Torus: click the major radius (ring centre line)
- Torus: click the tube (minor) radius
- Autosaved _ClaudeScratch/SolidArcWorkspace/ConstructRecovery.solidarc
- Torus1 · New body · 2 ms
- Block: click the first corner · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Block: click the opposite corner
- Block: move up, click the height
- Block2 · New body · 1 ms
- Polygon on plane XY (or the body face under the first click): click the centre, then a vertex · + / − sides · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Autosaved _ClaudeScratch/SolidArcWorkspace/ConstructRecovery.solidarc
- Polygon · 7 sides · wheel (or + / −) changes the sides · click / right-click / Enter confirms · Esc aborts
- 8 sides
- 7 sides
- 6 sides
- 5 sides
- Polygon1 · 1 ms
- Line on plane XY (or the body face under the first click): click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Line aborted (document untouched)
- Block: click the first corner · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Autosaved _ClaudeScratch/SolidArcWorkspace/ConstructRecovery.solidarc
- Selection cleared
- Framed the document (Space)
