# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 810 · 248 frames · 16.6673 s · slowest frame 343.729 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/ExtrudeManifoldWalkthrough.txt
- Expectations: 32 / 32 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 33 | `refused == 0` | 0 | 🟢 |
| 34 | `bodies == 1` | 1 | 🟢 |
| 35 | `volume ~ 8` | 8 | 🟢 |
| 42 | `drawing-on-face == 1` | 1 | 🟢 |
| 55 | `drawing == 0` |  | 🟢 |
| 56 | `refused == 0` | 0 | 🟢 |
| 59 | `bevel-verb == 2` | 2 | 🟢 |
| 60 | `bevel-material == -1` | -1 | 🟢 |
| 61 | `bevel-auto == 1` | 1 | 🟢 |
| 62 | `bevel-combination == Union` | Union | 🟢 |
| 68 | `bevel-combination == Difference` | Difference | 🟢 |
| 69 | `bevel-auto == 1` | 1 | 🟢 |
| 70 | `bevel-preview-volume ~ 7.745` | 7.745 | 🟢 |
| 74 | `refused == 0` | 0 | 🟢 |
| 75 | `bodies == 1` | 1 | 🟢 |
| 76 | `volume ~ 7.745` | 7.745 | 🟢 |
| 77 | `faces == 12` | 12 | 🟢 |
| 78 | `sound == 1` | 1 | 🟢 |
| 94 | `drawing == 0` |  | 🟢 |
| 99 | `bevel-material == -1` | -1 | 🟢 |
| 100 | `bevel-auto == 1` | 1 | 🟢 |
| 106 | `bevel-combination == Union` | Union | 🟢 |
| 109 | `refused == 0` | 0 | 🟢 |
| 110 | `volume ~ 7.873` | 7.873 | 🟢 |
| 111 | `faces == 16` | 16 | 🟢 |
| 116 | `selected-faces == 1` | Extrude#6.Cap(End) | 🟢 |
| 119 | `bevel-verb == 1` | 1 | 🟢 |
| 125 | `bevel-preview-volume ~ 7.585` | 7.585 | 🟢 |
| 129 | `refused == 0` | 0 | 🟢 |
| 130 | `bodies == 1` | 1 | 🟢 |
| 131 | `volume ~ 7.585` | 7.585 | 🟢 |
| 132 | `sound == 1` | 1 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 303ExtrudeManifoldPocket | 130 | `e5b364da1190c89d` |
| 304ExtrudeManifoldFacePush | 241 | `c9c8fed4bf420381` |

## Journal

- New document (previous recovery copy kept)
- Block: click the first corner · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Block: click the opposite corner
- Block: move up, click the height
- Block1 · New body · 0 ms
- Line on plane XY (or the body face under the first click): click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Autosaved _ClaudeScratch/SolidArcWorkspace/ManifoldRecovery.solidarc
- Line1 · 0 ms
- Extrude Line1 (E, extrude manifold): the profile follows the pointer — into the body cuts a pocket, out of it adds · Q union · W difference · E intersect · R new body · A auto · Enter / right-click applies · Esc aborts
- Bevel size -0.3 m (pinned)
- Extrude1 · Line1 · Difference · 1 ms
- Line on plane XY (or the body face under the first click): click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Line2 · 0 ms
- Fill Line2 · 0 ms
- Extrude Line2 (E, extrude manifold): the profile follows the pointer — into the body cuts a pocket, out of it adds · Q union · W difference · E intersect · R new body · A auto · Enter / right-click applies · Esc aborts
- Bevel size 0.4 m (pinned)
- Extrude2 · Line2 · Union · 1 ms
- Selected Extrude#6.Cap(End)
- Extrude (E): the face follows the pointer along its normal (Blender) · out adds, in pushes / cuts a pocket (extrude manifold) · click edits the faces · Enter / right-click applies · Esc aborts
- Bevel size -0.9 m (pinned)
- ExtrudeFaces -0.9 on 1 face(s) · 2 ms
