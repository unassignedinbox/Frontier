# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0
- Frame: 1600 x 900 · 323 frames · 26.6078 s · slowest frame 498.168 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/GizmoWalkthrough.txt
- Expectations: 34 / 34 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 27 | `bodies == 1` | 1 | 🟢 |
| 28 | `volume ~ 3` | 3 | 🟢 |
| 35 | `selected-bodies == 1` | Extrude#2 | 🟢 |
| 36 | `gizmo-target == 2` | bodies | 🟢 |
| 37 | `gizmo-modes == 1` | 1 | 🟢 |
| 43 | `gizmo-commits == 1` | 1 | 🟢 |
| 44 | `volume ~ 3` | 3 | 🟢 |
| 45 | `refused == 0` | 0 | 🟢 |
| 54 | `gizmo-modes == 7` | 7 | 🟢 |
| 60 | `gizmo-commits == 2` | 2 | 🟢 |
| 61 | `volume ~ 3` | 3 | 🟢 |
| 64 | `gizmo-commits == 3` | 3 | 🟢 |
| 65 | `volume > 3` | 4.2 | 🟢 |
| 73 | `volume ~ 3` | 3 | 🟢 |
| 78 | `selected-faces == 1` | Extrude#2.Cap(End) | 🟢 |
| 79 | `gizmo-target == 1` | faces | 🟢 |
| 85 | `gizmo-commits == 4` | 4 | 🟢 |
| 86 | `volume > 3` | 4.2 | 🟢 |
| 87 | `refused == 0` | 0 | 🟢 |
| 93 | `gizmo-modes == 2` | 2 | 🟢 |
| 94 | `selected-faces == 1` | Extrude#2.Cap(End) | 🟢 |
| 97 | `gizmo-commits == 5` | 5 | 🟢 |
| 98 | `refused == 0` | 0 | 🟢 |
| 107 | `gizmo-modes == 1` | 1 | 🟢 |
| 124 | `gizmo-target == 3` | sketches | 🟢 |
| 125 | `gizmo-targets == 1` | 1 | 🟢 |
| 129 | `gizmo-commits == 6` | 6 | 🟢 |
| 132 | `selected-curves == 2` | Circle2,Circle1 | 🟢 |
| 136 | `panels == 0` | 0 | 🟢 |
| 137 | `last-step == 1` | Loft#3 | 🟢 |
| 138 | `bodies == 1` | 1 | 🟢 |
| 139 | `volume > 1` | 1.282817 | 🟢 |
| 140 | `sound == 1` | 1 | 🟢 |
| 142 | `volume ~ 1.282817` | 1.282817 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 218GizmoMove | 64 | `e8a20a0edc28dd32` |
| 219GizmoCombined | 103 | `7b70823422eac266` |
| 220GizmoScaled | 143 | `4c5feb0855273a72` |
| 221GizmoFace | 169 | `f116e6960e2229fa` |
| 222GizmoFaceRotate | 215 | `f88befc50edb5e8d` |
| 223GizmoSketch | 286 | `c8d2e52d680fe8af` |
| 224GizmoLoft | 323 | `835d0e078a74a100` |

## Journal

- New document (previous recovery copy kept)
- Block: click the first corner · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Block: click the opposite corner
- Block: move up, click the height
- Block1 · New body · 9 ms
- Selected Block1 (6 faces)
- Move (Block1) · 9 ms
- Gizmo Z move 0.5 · Move (Block1) · 9 ms
- Gizmo: rotate
- Gizmo: scale
- Gizmo: move + rotate
- Gizmo: rotate + scale
- Gizmo: move + rotate + scale
- Rotate (Block1) · 9 ms
- Gizmo Z rotate -20.0° · Rotate (Block1) · 9 ms
- Scale (Block1) · 20 ms
- Gizmo X scale 1.400x · Scale (Block1) · 20 ms
- Undo
- Undo
- Undo
- Selected Extrude#2.Cap(End)
- Offset 0.4 on 1 face(s) · 17 ms
- Gizmo Z move 0.4 · push / pull · Offset 0.4 on 1 face(s) · 17 ms
- Gizmo: rotate + scale
- Gizmo: rotate
- Rotate -0.08727 on 1 face(s) · 18 ms
- Gizmo X rotate -5.0° · face rotate · Rotate -0.08727 on 1 face(s) · 18 ms
- New document (previous recovery copy kept)
- Gizmo: move + rotate
- Gizmo: move
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Circle1 · 0 ms
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Circle2 · 0 ms
- Selected sketch Circle2 · 1 selected · L lofts the picks in order · the gizmo moves / turns them
- Move (Circle2) · 0 ms
- Gizmo Z move 0.7 · Move (Circle2) · 0 ms
- Selected sketch Circle1 · 2 selected · L lofts the picks in order · the gizmo moves / turns them
- Loft1 · 2 sections · solid · 9 ms
- Lofted 2 closed section(s) into a solid shell (2 plane caps) · sound · 9 ms
