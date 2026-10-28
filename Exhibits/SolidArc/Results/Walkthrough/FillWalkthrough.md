# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 810 · 444 frames · 32.6621 s · slowest frame 340.579 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/FillWalkthrough.txt
- Expectations: 64 / 64 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 15 | `steps == 0` | 0 | 🟢 |
| 30 | `drawing == 0` |  | 🟢 |
| 31 | `steps == 2` | 2 | 🟢 |
| 32 | `sketches == Circle1,Circle2` | Circle1,Circle2 | 🟢 |
| 33 | `focused-caption == Circle2` | Circle2 | 🟢 |
| 34 | `sketch-filled == 0` | 0 | 🟢 |
| 35 | `tint-loops == 2` | 2 | 🟢 |
| 41 | `fill == 1` | 1 | 🟢 |
| 44 | `fill-hover == region` | region | 🟢 |
| 45 | `fill-loops == 1` | 1 | 🟢 |
| 46 | `status contains Fill` | Fill: hover a closed region (overlaps split into their own regions) · click fills it ... | 🟢 |
| 50 | `refused == 0` | 0 | 🟢 |
| 51 | `fill-commits == 1` | 1 | 🟢 |
| 52 | `steps == 3` | 3 | 🟢 |
| 53 | `sketch-filled == 1` | 1 | 🟢 |
| 54 | `fill == 1` | 1 | 🟢 |
| 59 | `fill-hover == region` | region | 🟢 |
| 60 | `fill-loops == 1` | 1 | 🟢 |
| 64 | `fill-commits == 2` | 2 | 🟢 |
| 65 | `steps == 4` | 4 | 🟢 |
| 66 | `sketch-filled == 1` | 1 | 🟢 |
| 69 | `fill == 0` | 0 | 🟢 |
| 74 | `fill == 1` | 1 | 🟢 |
| 77 | `fill == 0` | 0 | 🟢 |
| 78 | `steps == 4` | 4 | 🟢 |
| 81 | `fill == 1` | 1 | 🟢 |
| 84 | `fill == 0` | 0 | 🟢 |
| 85 | `steps == 4` | 4 | 🟢 |
| 90 | `bevel-active == 1` | 1 | 🟢 |
| 98 | `steps == 5` | 5 | 🟢 |
| 99 | `bodies == 1` | 1 | 🟢 |
| 100 | `volume ~ 1.913222954981` | 1.913222955 | 🟢 |
| 101 | `sound == 1` | 1 | 🟢 |
| 112 | `steps == 6` | 6 | 🟢 |
| 113 | `sketch-filled == 0` | 0 | 🟢 |
| 116 | `sketch-filled == 1` | 1 | 🟢 |
| 119 | `sketch-filled == 0` | 0 | 🟢 |
| 129 | `steps == 7` | 7 | 🟢 |
| 130 | `bodies == 2` | 2 | 🟢 |
| 131 | `volume ~ 1.913222954981` | 1.913222955 | 🟢 |
| 145 | `selected-bodies contains Extrude#7` | Extrude#7 | 🟢 |
| 148 | `refused == 0` | 0 | 🟢 |
| 149 | `flips == 1` | 1 | 🟢 |
| 150 | `steps == 8` | 8 | 🟢 |
| 151 | `status contains Flip` | Flip normals (Extrude2) · 0 ms | 🟢 |
| 155 | `flips == 0` | 0 | 🟢 |
| 156 | `steps == 7` | 7 | 🟢 |
| 163 | `construct-section == 2` | 2 | 🟢 |
| 174 | `image-planes == 1` | 1 | 🟢 |
| 175 | `status contains Image plane EmptyWorkspace` | Image plane EmptyWorkspace: 1600 × 960 px on the front (XZ) plane, 4.000 × 2.400 m (I... | 🟢 |
| 191 | `image-focused == EmptyWorkspace` | EmptyWorkspace | 🟢 |
| 196 | `image-photo == 1` | 1 | 🟢 |
| 199 | `image-mirrored == 1` | 1 | 🟢 |
| 202 | `image-crop-left > 0` | 754 | 🟢 |
| 207 | `image-opacity > 0.25` | 0.3899999857 | 🟢 |
| 208 | `image-opacity < 0.55` | 0.3899999857 | 🟢 |
| 212 | `image-locked == 1` | 1 | 🟢 |
| 215 | `image-planes-drawn == 0` | 0 | 🟢 |
| 218 | `image-planes-drawn == 1` | 1 | 🟢 |
| 221 | `image-locked == 0` | 0 | 🟢 |
| 224 | `image-crop-left == 0` | 0 | 🟢 |
| 229 | `image-photo == 0` | 0 | 🟢 |
| 230 | `image-mirrored == 0` | 0 | 🟢 |
| 233 | `image-focused != EmptyWorkspace` |  | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 246FillTint | 51 | `99a6c8fb7b0c823f` |
| 247FillLens | 65 | `f1702bd4797e3a86` |
| 248FillCrescent | 80 | `cb271e6b538f5bee` |
| 249FillWalls | 225 | `ddf8aee7025a7970` |
| 250FlipNormals | 246 | `60c66b943dae7b5b` |
| 251ConstructReference | 268 | `e7407f0dfa544d20` |
| 252ImagePlane | 322 | `b548e57c06acebf5` |
| 298ImageProperties | 381 | `47308591bd5ad28f` |

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
- Extrude Fill2 (E): the profile follows the pointer along its normal · Q union · W difference · E intersect · R new body · A auto · Enter / right-click applies · Esc aborts
- Bevel size 1 m (pinned)
- Extrude1 · Fill2 · New · 1 ms
- Rectangle on plane XY (or the body face under the first click): click two corners · Alt centre mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Rectangle1 · 0 ms
- Fill Rectangle1 · 0 ms
- Unfill Rectangle1 · 0 ms
- Extrude Rectangle1 (E): the profile follows the pointer along its normal · Q union · W difference · E intersect · R new body · A auto · Enter / right-click applies · Esc aborts
- Bevel size 1 m (pinned)
- Extrude2 · Rectangle1 · Union · 0 ms
- View Isometric
- Framed the document (Space)
- Selected Extrude2 (4 faces)
- Autosaved _ClaudeScratch/SolidArcWorkspace/FillRecovery.solidarc
- Flip normals (Extrude2) · 0 ms
- Flip normals (Extrude2) · 0 ms
- Autosaved _ClaudeScratch/SolidArcWorkspace/FillRecovery.solidarc
- Image plane EmptyWorkspace: 1600 × 960 px on the front (XZ) plane, 4.000 × 2.400 m (Inspector: opacity, crop, placement)
- View Front
- Framed the document (Space)
- EmptyWorkspace · Inspector › Image planes: opacity, line art / photo, mirror, crop, width, turn, centre
- Autosaved _ClaudeScratch/SolidArcWorkspace/FillRecovery.solidarc
- EmptyWorkspace locked (placement and crop frozen)
- EmptyWorkspace hidden
- EmptyWorkspace shown
- EmptyWorkspace unlocked
- EmptyWorkspace let go
