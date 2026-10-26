# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0
- Frame: 1600 x 900 · 417 frames · 53.4822 s · slowest frame 4335.11 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/ResponsivenessWalkthrough.txt
- Expectations: 37 / 37 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 29 | `bodies == 1` | 1 | 🟢 |
| 30 | `refused == 0` | 0 | 🟢 |
| 41 | `face-plane == 1` | Extrude#2.Side(Circle1) | 🟢 |
| 46 | `refused == 0` | 0 | 🟢 |
| 47 | `plane-normal-y < -0.9999` | -0.9999607933 | 🟢 |
| 48 | `plane-offset ~ 1.7` | 1.7 | 🟢 |
| 55 | `profile-sink > 0.011806` | 0.02103223203 | 🟢 |
| 56 | `profile-sink < 0.03` | 0.02103223203 | 🟢 |
| 64 | `bodies == 1` | 1 | 🟢 |
| 65 | `refused == 0` | 0 | 🟢 |
| 66 | `sound == 1` | 1 | 🟢 |
| 67 | `extrude-start < -0.011806` | -0.02103223203 | 🟢 |
| 68 | `volume ~ 5.485591014531` | 5.485591015 | 🟢 |
| 79 | `selected-edges == 1` | Edge(Extrude#2.Side(Circle1)/Extrude#4.Side(Circle1)) | 🟢 |
| 84 | `bevel-chamfer == 1` | 1 | 🟢 |
| 85 | `bevel-active == 1` | 1 | 🟢 |
| 86 | `bevel-stages == 2` | 2 | 🟢 |
| 117 | `bevel-stages < 5` | 3 | 🟢 |
| 119 | `bevel-stages < 6` | 4 | 🟢 |
| 123 | `bevel-chamfer == 1` | 1 | 🟢 |
| 131 | `bevel-active == 0` | 0 | 🟢 |
| 132 | `refused == 0` | 0 | 🟢 |
| 133 | `sound == 1` | 1 | 🟢 |
| 140 | `selected-edges == 1` | Edge(Extrude#4.Cap(End)/Extrude#4.Side(Circle1)) | 🟢 |
| 145 | `bevel-chamfer == 1` | 1 | 🟢 |
| 153 | `bevel-active == 0` | 0 | 🟢 |
| 154 | `refused == 0` | 0 | 🟢 |
| 155 | `sound == 1` | 1 | 🟢 |
| 162 | `selected-bodies == 1` | Extrude#2 | 🟢 |
| 171 | `refused == 0` | 0 | 🟢 |
| 172 | `gizmo-commits == 4` | 4 | 🟢 |
| 173 | `volume ~ 5.484407611` | 5.484407611 | 🟢 |
| 181 | `transform == 1` | 1 | 🟢 |
| 186 | `transform-rigid == 1` | 1 | 🟢 |
| 189 | `transform == 0` | 0 | 🟢 |
| 190 | `transform-commits == 1` | 1 | 🟢 |
| 191 | `refused == 0` | 0 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 262UnionOnSide | 129 | `aa5084c1097ef888` |
| 263ChamferSweep | 226 | `cb1bd5fee29ea960` |
| 264MovedFourTimes | 388 | `257711936cad6031` |
| 265RigidMove | 417 | `9917d6a4947efe99` |

## Journal

- New document (previous recovery copy kept)
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Circle1 · 0 ms
- Fill Circle1 · 0 ms
- Extrude Circle1 (E): move horizontally · Q union · W difference · E intersect · R new body · Enter / right-click applies · Esc aborts
- Bevel size 0.6 m (pinned)
- Extrude1 · Circle1 · New · 11 ms
- View Isometric
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Autosaved _ClaudeScratch/SolidArcWorkspace/ResponsivenessRecovery.solidarc
- Circle2 · 8 ms
- Fill Circle2 · 7 ms
- Extrude Circle2 (E): move horizontally · Q union · W difference · E intersect · R new body · Enter / right-click applies · Esc aborts
- Bevel size 0.3 m (pinned)
- Extrude2 · Circle2 · Union · 134 ms
- Selected Edge(Extrude#2.Side(Circle1)|Extrude#4.Side(Circle1))
- Fillet (B): move horizontally · D fillet · C chamfer · V variable size · F flip its anchor · click edits the edges · Enter / right-click applies · Esc aborts
- Bevel size 0.02 m (pinned)
- Chamfer 0.02 on 1 edge(s) · 1045 ms
- Selected Edge(Extrude#4.Cap(End)|Extrude#4.Side(Circle1))
- Fillet (B): move horizontally · D fillet · C chamfer · V variable size · F flip its anchor · click edits the edges · Enter / right-click applies · Esc aborts
- Bevel size 0.05 m (pinned)
- Chamfer 0.05 on 1 edge(s) · 1235 ms
- Selected Extrude1 (7 faces)
- Move (Extrude1) · 394 ms
- Gizmo X move 0.3 · Move (Extrude1) · 394 ms
- Move (Extrude1) · 228 ms
- Gizmo X move 0.3 · Move (Extrude1) · 228 ms
- Move (Extrude1) · 234 ms
- Gizmo X move 0.3 · Move (Extrude1) · 234 ms
- Move (Extrude1) · 226 ms
- Gizmo X move 0.3 · Move (Extrude1) · 226 ms
- Move 1 body(ies) · X / Y / Z axis · Shift+X / Y / Z plane · type a value · click / Enter commits · Esc cancels
- Move (Extrude1) · 223 ms
