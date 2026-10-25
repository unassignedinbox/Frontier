# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0
- Frame: 1600 x 900 · 296 frames · 27.1908 s · slowest frame 472.934 ms
- Script: /home/user/Frontier/Exhibits/SolidArc/Code/Scripts/ResponsivenessWalkthrough.txt
- Expectations: 16 / 16 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 11 | `bodies == 1` | 1 | 🟢 |
| 12 | `steps == 4` | 4 | 🟢 |
| 22 | `selected-edges == 1` | Edge(Extrude#4.Cap(End)/Extrude#4.Side(Circle1)) | 🟢 |
| 27 | `bevel-active == 1` | 1 | 🟢 |
| 116 | `bevel-active == 0` | 0 | 🟢 |
| 117 | `refused == 0` | 0 | 🟢 |
| 124 | `selected-bodies == 1` | Extrude#2 | 🟢 |
| 133 | `refused == 0` | 0 | 🟢 |
| 134 | `gizmo-commits == 4` | 4 | 🟢 |
| 142 | `transform == 1` | 1 | 🟢 |
| 147 | `transform-rigid == 1` | 1 | 🟢 |
| 148 | `transform-preview-volume > 5.5603` | 5.560340706 | 🟢 |
| 149 | `transform-preview-volume < 5.5604` | 5.560340706 | 🟢 |
| 152 | `transform == 0` | 0 | 🟢 |
| 153 | `transform-commits == 1` | 1 | 🟢 |
| 154 | `refused == 0` | 0 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 262UnionOnSide | 5 | `2bda4aeed22501dd` |
| 263ChamferSweep | 152 | `3f1ce7a70479bd86` |
| 264MovedFourTimes | 267 | `4ab5b4687edd625e` |
| 265RigidMove | 296 | `f75b7b8e1d42e612` |

## Journal

- Selected Edge(Extrude#4.Cap(End)|Extrude#4.Side(Circle1))
- Fillet (B): move horizontally · D fillet · C chamfer · V variable size · F flip its anchor · click edits the edges · Enter / right-click applies · Esc aborts
- Bevel size 0.05 m (pinned)
- Chamfer 0.05 on 1 edge(s) · 33 ms
- Selected Extrude1 (7 faces)
- Move (Extrude1) · 17 ms
- Gizmo X move 0.3 · Move (Extrude1) · 17 ms
- Move (Extrude1) · 17 ms
- Gizmo X move 0.3 · Move (Extrude1) · 17 ms
- Move (Extrude1) · 17 ms
- Gizmo X move 0.3 · Move (Extrude1) · 17 ms
- Move (Extrude1) · 17 ms
- Gizmo X move 0.3 · Move (Extrude1) · 17 ms
- Move 1 body(ies) · X / Y / Z axis · Shift+X / Y / Z plane · type a value · click / Enter commits · Esc cancels
- Move (Extrude1) · 17 ms
