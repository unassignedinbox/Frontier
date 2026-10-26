# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 900 · 222 frames · 26.4229 s · slowest frame 447.54 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/EdgeStrokeWalkthrough.txt
- Expectations: 20 / 20 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 28 | `bodies == 1` | 1 | 🟢 |
| 29 | `refused == 0` | 0 | 🟢 |
| 40 | `face-plane == 1` | Extrude#2.Side(Circle1) | 🟢 |
| 45 | `refused == 0` | 0 | 🟢 |
| 46 | `plane-normal-y < -0.9999` | -0.9999607933 | 🟢 |
| 47 | `plane-offset ~ 1.7` | 1.7 | 🟢 |
| 54 | `profile-sink > 0.011806` | 0.02103223203 | 🟢 |
| 55 | `profile-sink < 0.03` | 0.02103223203 | 🟢 |
| 63 | `bodies == 1` | 1 | 🟢 |
| 64 | `refused == 0` | 0 | 🟢 |
| 65 | `sound == 1` | 1 | 🟢 |
| 66 | `extrude-start < -0.011806` | -0.02103223203 | 🟢 |
| 67 | `volume ~ 5.485591014531` | 5.485591015 | 🟢 |
| 76 | `selected-edges == 1` | Edge(Extrude#4.Cap(End)/Extrude#4.Side(Circle1)) | 🟢 |
| 88 | `bevel-active == 0` | 0 | 🟢 |
| 89 | `refused == 0` | 0 | 🟢 |
| 90 | `sound == 1` | 1 | 🟢 |
| 100 | `refused == 0` | 0 | 🟢 |
| 108 | `refused == 0` | 0 | 🟢 |
| 116 | `refused == 0` | 0 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 267EdgeStrokesWhole | 180 | `47f561fa339ffaac` |
| 268EdgeStrokesCloser | 194 | `ba54ad3757d3f44b` |
| 269EdgeStrokesCloseUp | 208 | `60ec92ca114c4006` |
| 270EdgeStrokesVeryClose | 222 | `6a67b1875947137b` |

## Journal

- New document (previous recovery copy kept)
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Circle1 · 0 ms
- Fill Circle1 · 0 ms
- Extrude Circle1 (E): move horizontally · Q union · W difference · E intersect · R new body · Enter / right-click applies · Esc aborts
- Bevel size 0.6 m (pinned)
- Extrude1 · Circle1 · New · 12 ms
- View Isometric
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Autosaved _ClaudeScratch/SolidArcWorkspace/EdgeStrokeRecovery.solidarc
- Circle2 · 7 ms
- Fill Circle2 · 7 ms
- Extrude Circle2 (E): move horizontally · Q union · W difference · E intersect · R new body · Enter / right-click applies · Esc aborts
- Bevel size 0.3 m (pinned)
- Extrude2 · Circle2 · Union · 131 ms
- Selected Edge(Extrude#4.Cap(End)|Extrude#4.Side(Circle1))
- Fillet (B): move horizontally · D fillet · C chamfer · V variable size · F flip its anchor · click edits the edges · Enter / right-click applies · Esc aborts
- Bevel size 0.05 m (pinned)
- Fillet 0.05 on 1 edge(s) · 216 ms
- Autosaved _ClaudeScratch/SolidArcWorkspace/EdgeStrokeRecovery.solidarc
