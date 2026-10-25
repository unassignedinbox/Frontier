# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0
- Frame: 1600 x 900 · 295 frames · 25.6236 s · slowest frame 503.472 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/FillWalkthrough.txt
- Expectations: 44 / 44 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 12 | `steps == 0` | 0 | 🟢 |
| 27 | `drawing == 0` |  | 🟢 |
| 28 | `steps == 2` | 2 | 🟢 |
| 29 | `sketches == Circle1,Circle2` | Circle1,Circle2 | 🟢 |
| 30 | `focused-caption == Circle2` | Circle2 | 🟢 |
| 31 | `sketch-filled == 0` | 0 | 🟢 |
| 32 | `tint-loops == 2` | 2 | 🟢 |
| 38 | `fill == 1` | 1 | 🟢 |
| 41 | `fill-hover == region` | region | 🟢 |
| 42 | `fill-loops == 1` | 1 | 🟢 |
| 43 | `status contains Fill` | Fill: hover a closed region (overlaps split into their own regions) · click fills it ... | 🟢 |
| 47 | `refused == 0` | 0 | 🟢 |
| 48 | `fill-commits == 1` | 1 | 🟢 |
| 49 | `steps == 3` | 3 | 🟢 |
| 50 | `sketch-filled == 1` | 1 | 🟢 |
| 51 | `fill == 1` | 1 | 🟢 |
| 56 | `fill-hover == region` | region | 🟢 |
| 57 | `fill-loops == 1` | 1 | 🟢 |
| 61 | `fill-commits == 2` | 2 | 🟢 |
| 62 | `steps == 4` | 4 | 🟢 |
| 63 | `sketch-filled == 1` | 1 | 🟢 |
| 66 | `fill == 0` | 0 | 🟢 |
| 71 | `bevel-active == 1` | 1 | 🟢 |
| 79 | `steps == 5` | 5 | 🟢 |
| 80 | `bodies == 1` | 1 | 🟢 |
| 81 | `volume ~ 1.913222954981` | 1.913222955 | 🟢 |
| 82 | `sound == 1` | 1 | 🟢 |
| 93 | `steps == 6` | 6 | 🟢 |
| 94 | `sketch-filled == 0` | 0 | 🟢 |
| 97 | `sketch-filled == 1` | 1 | 🟢 |
| 100 | `sketch-filled == 0` | 0 | 🟢 |
| 110 | `steps == 7` | 7 | 🟢 |
| 111 | `bodies == 2` | 2 | 🟢 |
| 112 | `volume ~ 1.913222954981` | 1.913222955 | 🟢 |
| 126 | `selected-bodies contains Extrude#7` | Extrude#7 | 🟢 |
| 129 | `refused == 0` | 0 | 🟢 |
| 130 | `flips == 1` | 1 | 🟢 |
| 131 | `steps == 8` | 8 | 🟢 |
| 132 | `status contains Flip` | Flip normals (Extrude2) · 10 ms | 🟢 |
| 136 | `flips == 0` | 0 | 🟢 |
| 137 | `steps == 7` | 7 | 🟢 |
| 144 | `construct-section == 2` | 2 | 🟢 |
| 155 | `image-planes == 1` | 1 | 🟢 |
| 156 | `status contains Image plane EmptyWorkspace` | Image plane EmptyWorkspace: 1600 × 960 px on the front (XZ) plane, 4.000 × 2.400 m (I... | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 246FillTint | 50 | `458b7329bb4bf494` |
| 247FillLens | 64 | `987b1838de773465` |
| 248FillCrescent | 79 | `c2cda6d752de4303` |
| 249FillWalls | 198 | `55c7f5e964b0ae8f` |
| 250FlipNormals | 219 | `bb58b70bab2c786b` |
| 251ConstructReference | 241 | `10e3243eed3caa8b` |
| 252ImagePlane | 295 | `5fed5cfe3431691b` |

## Journal

- New document (previous recovery copy kept)
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Circle1 · 0 ms
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Circle2 · 0 ms
- Fill: hover a closed region (overlaps split into their own regions) · click fills it · Esc ends
- Fill region into Fill1 · 0 ms
- Fill region into Fill2 · 0 ms
- Fill ended
- Extrude Fill2 (E): move horizontally · Q union · W difference · E intersect · R new body · Enter / right-click applies · Esc aborts
- Bevel size 1 m (pinned)
- Extrude1 · Fill2 · New · 21 ms
- Rectangle on plane XY (or the body face under the first click): click two corners · Alt centre mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Rectangle1 · 9 ms
- Fill Rectangle1 · 10 ms
- Unfill Rectangle1 · 9 ms
- Extrude Rectangle1 (E): move horizontally · Q union · W difference · E intersect · R new body · Enter / right-click applies · Esc aborts
- Bevel size 1 m (pinned)
- Extrude2 · Rectangle1 · Union · 10 ms
- View Isometric
- Framed the document (Space)
- Selected Extrude2 (4 faces)
- Autosaved _ClaudeScratch/SolidArcWorkspace/FillRecovery.solidarc
- Flip normals (Extrude2) · 10 ms
- Flip normals (Extrude2) · 9 ms
- Autosaved _ClaudeScratch/SolidArcWorkspace/FillRecovery.solidarc
- Image plane EmptyWorkspace: 1600 × 960 px on the front (XZ) plane, 4.000 × 2.400 m (Inspector: opacity, crop, placement)
- View Front
- Framed the document (Space)
