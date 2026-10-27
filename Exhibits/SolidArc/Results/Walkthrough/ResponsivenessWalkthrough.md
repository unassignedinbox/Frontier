# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 900 · 458 frames · 60.0191 s · slowest frame 1025.1 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/ResponsivenessWalkthrough.txt
- Expectations: 45 / 45 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 32 | `bodies == 1` | 1 | 🟢 |
| 33 | `refused == 0` | 0 | 🟢 |
| 44 | `face-plane == 1` | Extrude#2.Side(Circle1) | 🟢 |
| 49 | `refused == 0` | 0 | 🟢 |
| 50 | `plane-normal-y < -0.9999` | -0.9999607933 | 🟢 |
| 51 | `plane-offset ~ 1.7` | 1.7 | 🟢 |
| 58 | `profile-sink > 0.011806` | 0.02103223203 | 🟢 |
| 59 | `profile-sink < 0.03` | 0.02103223203 | 🟢 |
| 67 | `bodies == 1` | 1 | 🟢 |
| 68 | `refused == 0` | 0 | 🟢 |
| 69 | `sound == 1` | 1 | 🟢 |
| 70 | `extrude-start < -0.011806` | -0.02103223203 | 🟢 |
| 71 | `volume ~ 5.485591014531` | 5.485591015 | 🟢 |
| 82 | `selected-edges == 1` | Edge(Extrude#2.Side(Circle1)/Extrude#4.Side(Circle1)) | 🟢 |
| 87 | `bevel-chamfer == 1` | 1 | 🟢 |
| 88 | `bevel-active == 1` | 1 | 🟢 |
| 89 | `bevel-stages == 2` | 2 | 🟢 |
| 120 | `bevel-stages < 5` | 2 | 🟢 |
| 122 | `bevel-stages < 6` | 2 | 🟢 |
| 126 | `bevel-chamfer == 1` | 1 | 🟢 |
| 134 | `bevel-active == 0` | 0 | 🟢 |
| 135 | `refused == 0` | 0 | 🟢 |
| 136 | `sound == 1` | 1 | 🟢 |
| 143 | `selected-edges == 1` | Edge(Extrude#4.Cap(End)/Extrude#4.Side(Circle1)) | 🟢 |
| 148 | `bevel-chamfer == 1` | 1 | 🟢 |
| 156 | `bevel-active == 0` | 0 | 🟢 |
| 157 | `refused == 0` | 0 | 🟢 |
| 158 | `sound == 1` | 1 | 🟢 |
| 165 | `selected-bodies == 1` | Extrude#2 | 🟢 |
| 174 | `refused == 0` | 0 | 🟢 |
| 175 | `gizmo-commits == 4` | 4 | 🟢 |
| 176 | `volume ~ 5.484407611` | 5.484407611 | 🟢 |
| 184 | `transform == 1` | 1 | 🟢 |
| 189 | `transform-rigid == 1` | 1 | 🟢 |
| 192 | `transform == 0` | 0 | 🟢 |
| 193 | `transform-commits == 1` | 1 | 🟢 |
| 194 | `refused == 0` | 0 | 🟢 |
| 200 | `transform == 1` | 1 | 🟢 |
| 207 | `transform == 0` | 0 | 🟢 |
| 208 | `transform-commits == 2` | 2 | 🟢 |
| 209 | `refused == 0` | 0 | 🟢 |
| 210 | `volume ~ 5.484407611` | 5.484407611 | 🟢 |
| 215 | `transform == 1` | 1 | 🟢 |
| 220 | `transform == 0` | 0 | 🟢 |
| 221 | `transform-commits == 2` | 2 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 262UnionOnSide | 130 | `8d85cc112cb33143` |
| 263ChamferSweep | 227 | `d2e9f17ed26c3c36` |
| 264MovedFourTimes | 389 | `f98c05650be16966` |
| 265RigidMove | 418 | `0bec3083212e9077` |
| 266RightClickCommit | 458 | `b91340846b1cf841` |

## Journal

- New document (previous recovery copy kept)
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Circle1 · 0 ms
- Fill Circle1 · 0 ms
- Extrude Circle1 (E): move horizontally · Q union · W difference · E intersect · R new body · Enter / right-click applies · Esc aborts
- Bevel size 0.6 m (pinned)
- Extrude1 · Circle1 · New · 0 ms
- View Isometric
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Autosaved _ClaudeScratch/SolidArcWorkspace/ResponsivenessRecovery.solidarc
- Circle2 · 0 ms
- Fill Circle2 · 0 ms
- Extrude Circle2 (E): move horizontally · Q union · W difference · E intersect · R new body · Enter / right-click applies · Esc aborts
- Bevel size 0.3 m (pinned)
- Extrude2 · Circle2 · Union · 26 ms
- Selected Edge(Extrude#2.Side(Circle1)|Extrude#4.Side(Circle1))
- Fillet (B): drag anywhere or the ring handle to size (it stops at the largest that fits) · drag a corner diamond for a variable size · type in the card · D fillet · C chamfer · V taper · Alt+click an edge adds its ring · Enter / right-click applies · Esc aborts · starts at 0.02 m to fit the edges
- Bevel size 0.02 m (pinned)
- Chamfer 0.02 on 1 edge(s) · 581 ms
- Selected Edge(Extrude#4.Cap(End)|Extrude#4.Side(Circle1))
- Fillet (B): drag anywhere or the ring handle to size (it stops at the largest that fits) · drag a corner diamond for a variable size · type in the card · D fillet · C chamfer · V taper · Alt+click an edge adds its ring · Enter / right-click applies · Esc aborts
- Bevel size 0.05 m (pinned)
- Chamfer 0.05 on 1 edge(s) · 717 ms
- Selected Extrude1 (7 faces)
- Move (Extrude1) · 230 ms
- Gizmo X move 0.3 · Move (Extrude1) · 230 ms
- Move (Extrude1) · 249 ms
- Gizmo X move 0.3 · Move (Extrude1) · 249 ms
- Move (Extrude1) · 202 ms
- Gizmo X move 0.3 · Move (Extrude1) · 202 ms
- Move (Extrude1) · 153 ms
- Gizmo X move 0.3 · Move (Extrude1) · 153 ms
- Move 1 body(ies) · X / Y / Z axis · Shift+X / Y / Z plane · type a value · click / Enter commits · Esc cancels
- Move (Extrude1) · 158 ms
- Move 1 body(ies) · X / Y / Z axis · Shift+X / Y / Z plane · type a value · click / Enter commits · Esc cancels
- Move (Extrude1) · 157 ms
- Move 1 body(ies) · X / Y / Z axis · Shift+X / Y / Z plane · type a value · click / Enter commits · Esc cancels
- Move cancelled
