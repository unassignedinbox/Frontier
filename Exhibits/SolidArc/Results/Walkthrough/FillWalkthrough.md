# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 900 · 321 frames · 33.437 s · slowest frame 501.627 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/FillWalkthrough.txt
- Expectations: 50 / 50 passed

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
| 71 | `fill == 1` | 1 | 🟢 |
| 74 | `fill == 0` | 0 | 🟢 |
| 75 | `steps == 4` | 4 | 🟢 |
| 78 | `fill == 1` | 1 | 🟢 |
| 81 | `fill == 0` | 0 | 🟢 |
| 82 | `steps == 4` | 4 | 🟢 |
| 87 | `bevel-active == 1` | 1 | 🟢 |
| 95 | `steps == 5` | 5 | 🟢 |
| 96 | `bodies == 1` | 1 | 🟢 |
| 97 | `volume ~ 1.913222954981` | 1.913222955 | 🟢 |
| 98 | `sound == 1` | 1 | 🟢 |
| 109 | `steps == 6` | 6 | 🟢 |
| 110 | `sketch-filled == 0` | 0 | 🟢 |
| 113 | `sketch-filled == 1` | 1 | 🟢 |
| 116 | `sketch-filled == 0` | 0 | 🟢 |
| 126 | `steps == 7` | 7 | 🟢 |
| 127 | `bodies == 2` | 2 | 🟢 |
| 128 | `volume ~ 1.913222954981` | 1.913222955 | 🟢 |
| 142 | `selected-bodies contains Extrude#7` | Extrude#7 | 🟢 |
| 145 | `refused == 0` | 0 | 🟢 |
| 146 | `flips == 1` | 1 | 🟢 |
| 147 | `steps == 8` | 8 | 🟢 |
| 148 | `status contains Flip` | Flip normals (Extrude2) · 10 ms | 🟢 |
| 152 | `flips == 0` | 0 | 🟢 |
| 153 | `steps == 7` | 7 | 🟢 |
| 160 | `construct-section == 2` | 2 | 🟢 |
| 171 | `image-planes == 1` | 1 | 🟢 |
| 172 | `status contains Image plane EmptyWorkspace` | Image plane EmptyWorkspace: 1600 × 960 px on the front (XZ) plane, 4.000 × 2.400 m (I... | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 246FillTint | 50 | `495c7a39dfe7e055` |
| 247FillLens | 64 | `5e8e837290180de7` |
| 248FillCrescent | 79 | `31f39f49944231d7` |
| 249FillWalls | 224 | `aaa7ca2593a3e48c` |
| 250FlipNormals | 245 | `82d1f2b6e1173674` |
| 251ConstructReference | 267 | `5c002bff951881f1` |
| 252ImagePlane | 321 | `14b851a73e5d19c1` |

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
- Fill: hover a closed region (overlaps split into their own regions) · click fills it · Esc ends
- Fill ended
- Fill: hover a closed region (overlaps split into their own regions) · click fills it · Esc ends
- Fill ended
- Extrude Fill2 (E): move horizontally · Q union · W difference · E intersect · R new body · Enter / right-click applies · Esc aborts
- Bevel size 1 m (pinned)
- Extrude1 · Fill2 · New · 14 ms
- Rectangle on plane XY (or the body face under the first click): click two corners · Alt centre mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Rectangle1 · 9 ms
- Fill Rectangle1 · 9 ms
- Unfill Rectangle1 · 9 ms
- Extrude Rectangle1 (E): move horizontally · Q union · W difference · E intersect · R new body · Enter / right-click applies · Esc aborts
- Bevel size 1 m (pinned)
- Extrude2 · Rectangle1 · Union · 9 ms
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
