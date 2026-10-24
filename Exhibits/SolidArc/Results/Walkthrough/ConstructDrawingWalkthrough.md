# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0
- Frame: 1600 x 900 · 376 frames · 28.5699 s · slowest frame 362.256 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/ConstructDrawingWalkthrough.txt
- Expectations: 45 / 45 passed

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
| 158 | `drawing == Polygon` | Polygon | 🟢 |
| 166 | `drawing == 0` |  | 🟢 |
| 167 | `refused == 0` | 0 | 🟢 |
| 168 | `sketches contains Polygon1` | Block1,Cylinder1,Cone1,Prism1,Torus1,Block2,Polygon1 | 🟢 |
| 180 | `drawing == 0` |  | 🟢 |
| 189 | `drawing-solid == 0` | 0 | 🟢 |
| 190 | `construct-open == 1` | 1 | 🟢 |
| 191 | `construct-combination == 4` | 4 | 🟢 |
| 195 | `construct-open == 0` | 0 | 🟢 |
| 196 | `bodies == 6` | 6 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 200ConstructSolids | 15 | `4b6e435df7648aa6` |
| 201ConstructSketch | 24 | `b7f9e1b9bd6a1a6a` |
| 202BoxBase | 55 | `b907e5beb3558454` |
| 203BoxHeight | 68 | `c53aae315bc5efcb` |
| 204CylinderHeight | 105 | `692b6da528cb2465` |
| 205ConeHeight | 147 | `37e15ec7ae5e6050` |
| 206PrismHeight | 193 | `4b2e40e8bec858e5` |
| 207TorusTube | 235 | `53c344e18fe016b1` |
| 208PolygonDimensions | 312 | `247503de4f0e58fe` |
| 209LineDimensions | 338 | `07a5f43bdd6151e4` |
| 210ConstructExact | 363 | `19788885e0474ed4` |
| 211ConstructBodies | 376 | `0075bdcf515d6e3d` |

## Journal

- New document (previous recovery copy kept)
- Block: click the first corner · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Block: click the opposite corner
- Block: move up, click the height
- Block1 · New body · 6 ms
- Cylinder: click the base centre · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Cylinder: click the radius
- Cylinder: move up, click the height
- Cylinder1 · New body · 15 ms
- Cone: click the base centre · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Cone: click the radius
- Cone: move up, click the apex height
- Autosaved _ClaudeScratch/SolidArcWorkspace/ConstructRecovery.solidarc
- Cone1 · New body · 22 ms
- Prism: click the centre · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- 7 sides
- Prism: click a vertex (+ / − sides)
- Prism: move up, click the height
- Autosaved _ClaudeScratch/SolidArcWorkspace/ConstructRecovery.solidarc
- Prism1 · New body · 26 ms
- Torus: click the centre · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Torus: click the major radius (ring centre line)
- Torus: click the tube (minor) radius
- Autosaved _ClaudeScratch/SolidArcWorkspace/ConstructRecovery.solidarc
- Torus1 · New body · 52 ms
- Block: click the first corner · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Block: click the opposite corner
- Block: move up, click the height
- Block2 · New body · 38 ms
- Polygon on plane XY (or the body face under the first click): click the centre, then a vertex · + / − sides · hold Ctrl to snap to ends / midpoints / centres · Esc aborts
- Autosaved _ClaudeScratch/SolidArcWorkspace/ConstructRecovery.solidarc
- Polygon1 · 35 ms
- Line on plane XY (or the body face under the first click): click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres · Esc aborts
- Line aborted (document untouched)
- Block: click the first corner · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Block: click the opposite corner
- Autosaved _ClaudeScratch/SolidArcWorkspace/ConstructRecovery.solidarc
- Selection cleared
- Framed the document (Space)
