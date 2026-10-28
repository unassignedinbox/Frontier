# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 810 · 313 frames · 24.2008 s · slowest frame 327.897 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/OrientWalkthrough.txt
- Expectations: 43 / 43 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 16 | `face-orient == 1` | 1 | 🟢 |
| 33 | `refused == 0` | 0 | 🟢 |
| 34 | `bodies == 1` | 1 | 🟢 |
| 35 | `volume ~ 8` | 8 | 🟢 |
| 42 | `face-plane == 1` | Extrude#2.Side(Line2) | 🟢 |
| 43 | `drawing-on-face == 1` | 1 | 🟢 |
| 44 | `drawing-normal-x ~ 1` | 1 | 🟢 |
| 52 | `refused == 0` | 0 | 🟢 |
| 53 | `plane-normal-x ~ 1` | 1 | 🟢 |
| 54 | `plane-normal-z ~ 0` | 0 | 🟢 |
| 66 | `refused == 0` | 0 | 🟢 |
| 67 | `bodies == 1` | 1 | 🟢 |
| 68 | `volume ~ 8.502654824574368` | 8.502654825 | 🟢 |
| 69 | `scene-high-x ~ 2` | 2 | 🟢 |
| 74 | `drawing-solid-kind == 1` | Cylinder | 🟢 |
| 79 | `drawing-on-face == 1` | 1 | 🟢 |
| 80 | `drawing-normal-y ~ -1` | -1 | 🟢 |
| 87 | `drawing-lift ~ 0.5` | 0.5 | 🟢 |
| 91 | `refused == 0` | 0 | 🟢 |
| 92 | `bodies == 2` | 2 | 🟢 |
| 93 | `volume ~ 8.75398223686155` | 8.753982237 | 🟢 |
| 94 | `plane-normal-y ~ -1` | -1 | 🟢 |
| 95 | `scene-low-y ~ -1.5` | -1.5 | 🟢 |
| 114 | `refused == 0` | 0 | 🟢 |
| 115 | `bodies == 3` | 3 | 🟢 |
| 116 | `volume ~ 10.75398223686155` | 10.75398224 | 🟢 |
| 123 | `drawing-on-face == 1` | 1 | 🟢 |
| 124 | `drawing-normal-x ~ 0.4472135955` | 0.4472135955 | 🟢 |
| 125 | `drawing-normal-z ~ 0.894427191` | 0.894427191 | 🟢 |
| 132 | `drawing-lift ~ 0.5` | 0.5 | 🟢 |
| 136 | `refused == 0` | 0 | 🟢 |
| 137 | `bodies == 4` | 4 | 🟢 |
| 138 | `volume ~ 10.89535390627309` | 10.89535391 | 🟢 |
| 139 | `plane-normal-x ~ 0.4472135955` | 0.4472135955 | 🟢 |
| 140 | `plane-normal-z ~ 0.894427191` | 0.894427191 | 🟢 |
| 147 | `face-plane == 1` | Extrude#6.Side(Circle1) | 🟢 |
| 152 | `refused == 0` | 0 | 🟢 |
| 153 | `plane-normal-x > 0.99` | 0.9999356986 | 🟢 |
| 160 | `face-orient == 0` | 0 | 🟢 |
| 163 | `face-plane == 0` |  | 🟢 |
| 164 | `drawing-normal-z ~ 1` | 1 | 🟢 |
| 170 | `refused == 0` | 0 | 🟢 |
| 171 | `plane-normal-z ~ 1` | 1 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 257CircleOnSideFace | 77 | `39332c606e33b7fd` |
| 258CylinderOnFrontFace | 148 | `cf8f262297bc9816` |
| 259CylinderOnSlope | 237 | `4c6a8b98e5da9158` |
| 260OrientOff | 290 | `4771ef87b5a8ef8b` |
| 261OrientedBodies | 313 | `9cea7c6054e1b12d` |

## Journal

- New document (previous recovery copy kept)
- Block: click the first corner · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Block: click the opposite corner
- Block: move up, click the height
- Block1 · New body · 0 ms
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Circle1 · 0 ms
- Fill Circle1 · 0 ms
- Extrude Circle1 (E, extrude manifold): the profile follows the pointer — into the body cuts a pocket, out of it adds · Q union · W difference · E intersect · R new body · A auto · Enter / right-click applies · Esc aborts
- Bevel size 1 m (pinned)
- Extrude1 · Circle1 · Union · 1 ms
- Cylinder: click the base centre · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Cylinder: click the radius
- Cylinder: move up, click the height
- Autosaved _ClaudeScratch/SolidArcWorkspace/OrientRecovery.solidarc
- Cylinder1 · New body · 1 ms
- Wedge: click the first corner · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Wedge: click the opposite corner
- Wedge: move up, click the height
- Autosaved _ClaudeScratch/SolidArcWorkspace/OrientRecovery.solidarc
- Wedge1 · New body · 1 ms
- Cylinder: click the base centre · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Cylinder: click the radius
- Cylinder: move up, click the height
- Autosaved _ClaudeScratch/SolidArcWorkspace/OrientRecovery.solidarc
- Cylinder2 · New body · 1 ms
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Circle2 · 1 ms
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Circle3 · 1 ms
- Framed the document (Space)
