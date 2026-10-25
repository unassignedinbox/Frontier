# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0
- Frame: 1600 x 900 · 312 frames · 28.3153 s · slowest frame 489.92 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/OrientWalkthrough.txt
- Expectations: 43 / 43 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 13 | `face-orient == 1` | 1 | 🟢 |
| 30 | `refused == 0` | 0 | 🟢 |
| 31 | `bodies == 1` | 1 | 🟢 |
| 32 | `volume ~ 8` | 8 | 🟢 |
| 39 | `face-plane == 1` | Extrude#2.Side(Line2) | 🟢 |
| 40 | `drawing-on-face == 1` | 1 | 🟢 |
| 41 | `drawing-normal-x ~ 1` | 1 | 🟢 |
| 49 | `refused == 0` | 0 | 🟢 |
| 50 | `plane-normal-x ~ 1` | 1 | 🟢 |
| 51 | `plane-normal-z ~ 0` | 0 | 🟢 |
| 63 | `refused == 0` | 0 | 🟢 |
| 64 | `bodies == 1` | 1 | 🟢 |
| 65 | `volume ~ 8.502654824574368` | 8.502654825 | 🟢 |
| 66 | `scene-high-x ~ 2` | 2 | 🟢 |
| 71 | `drawing-solid-kind == 1` | Cylinder | 🟢 |
| 76 | `drawing-on-face == 1` | 1 | 🟢 |
| 77 | `drawing-normal-y ~ -1` | -1 | 🟢 |
| 84 | `drawing-lift ~ 0.5` | 0.5 | 🟢 |
| 88 | `refused == 0` | 0 | 🟢 |
| 89 | `bodies == 2` | 2 | 🟢 |
| 90 | `volume ~ 8.75398223686155` | 8.753982237 | 🟢 |
| 91 | `plane-normal-y ~ -1` | -1 | 🟢 |
| 92 | `scene-low-y ~ -1.5` | -1.5 | 🟢 |
| 111 | `refused == 0` | 0 | 🟢 |
| 112 | `bodies == 3` | 3 | 🟢 |
| 113 | `volume ~ 10.75398223686155` | 10.75398224 | 🟢 |
| 120 | `drawing-on-face == 1` | 1 | 🟢 |
| 121 | `drawing-normal-x ~ 0.4472135955` | 0.4472135955 | 🟢 |
| 122 | `drawing-normal-z ~ 0.894427191` | 0.894427191 | 🟢 |
| 129 | `drawing-lift ~ 0.5` | 0.5 | 🟢 |
| 133 | `refused == 0` | 0 | 🟢 |
| 134 | `bodies == 4` | 4 | 🟢 |
| 135 | `volume ~ 10.89535390627309` | 10.89535391 | 🟢 |
| 136 | `plane-normal-x ~ 0.4472135955` | 0.4472135955 | 🟢 |
| 137 | `plane-normal-z ~ 0.894427191` | 0.894427191 | 🟢 |
| 144 | `face-plane == 1` | Extrude#6.Side(Circle1) | 🟢 |
| 149 | `refused == 0` | 0 | 🟢 |
| 150 | `plane-normal-x > 0.99` | 0.9997394334 | 🟢 |
| 157 | `face-orient == 0` | 0 | 🟢 |
| 160 | `face-plane == 0` |  | 🟢 |
| 161 | `drawing-normal-z ~ 1` | 1 | 🟢 |
| 167 | `refused == 0` | 0 | 🟢 |
| 168 | `plane-normal-z ~ 1` | 1 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 257CircleOnSideFace | 76 | `89119d9b81a3e11d` |
| 258CylinderOnFrontFace | 147 | `eed842cd1b638e94` |
| 259CylinderOnSlope | 236 | `6458b994f4af6b69` |
| 260OrientOff | 289 | `4f54e78371964a53` |
| 261OrientedBodies | 312 | `738a681a0ef5240a` |

## Journal

- New document (previous recovery copy kept)
- Block: click the first corner · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Block: click the opposite corner
- Block: move up, click the height
- Block1 · New body · 15 ms
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Circle1 · 10 ms
- Fill Circle1 · 10 ms
- Extrude Circle1 (E): move horizontally · Q union · W difference · E intersect · R new body · Enter / right-click applies · Esc aborts
- Bevel size 1 m (pinned)
- Extrude1 · Circle1 · Union · 33 ms
- Cylinder: click the base centre · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Cylinder: click the radius
- Cylinder: move up, click the height
- Autosaved _ClaudeScratch/SolidArcWorkspace/OrientRecovery.solidarc
- Cylinder1 · New body · 22 ms
- Wedge: click the first corner · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Wedge: click the opposite corner
- Wedge: move up, click the height
- Autosaved _ClaudeScratch/SolidArcWorkspace/OrientRecovery.solidarc
- Wedge1 · New body · 39 ms
- Cylinder: click the base centre · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Cylinder: click the radius
- Cylinder: move up, click the height
- Autosaved _ClaudeScratch/SolidArcWorkspace/OrientRecovery.solidarc
- Cylinder2 · New body · 51 ms
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Circle2 · 45 ms
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Circle3 · 44 ms
- Framed the document (Space)
