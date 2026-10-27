# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 900 · 324 frames · 29.277 s · slowest frame 508.006 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/GizmoWalkthrough.txt
- Expectations: 34 / 34 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 30 | `bodies == 1` | 1 | 🟢 |
| 31 | `volume ~ 3` | 3 | 🟢 |
| 38 | `selected-bodies == 1` | Extrude#2 | 🟢 |
| 39 | `gizmo-target == 2` | bodies | 🟢 |
| 40 | `gizmo-modes == 1` | 1 | 🟢 |
| 46 | `gizmo-commits == 1` | 1 | 🟢 |
| 47 | `volume ~ 3` | 3 | 🟢 |
| 48 | `refused == 0` | 0 | 🟢 |
| 57 | `gizmo-modes == 7` | 7 | 🟢 |
| 63 | `gizmo-commits == 2` | 2 | 🟢 |
| 64 | `volume ~ 3` | 3 | 🟢 |
| 67 | `gizmo-commits == 3` | 3 | 🟢 |
| 68 | `volume > 3` | 4.2 | 🟢 |
| 76 | `volume ~ 3` | 3 | 🟢 |
| 81 | `selected-faces == 1` | Extrude#2.Cap(End) | 🟢 |
| 82 | `gizmo-target == 1` | faces | 🟢 |
| 88 | `gizmo-commits == 4` | 4 | 🟢 |
| 89 | `volume > 3` | 4.2 | 🟢 |
| 90 | `refused == 0` | 0 | 🟢 |
| 96 | `gizmo-modes == 2` | 2 | 🟢 |
| 97 | `selected-faces == 1` | Extrude#2.Cap(End) | 🟢 |
| 100 | `gizmo-commits == 5` | 5 | 🟢 |
| 101 | `refused == 0` | 0 | 🟢 |
| 110 | `gizmo-modes == 1` | 1 | 🟢 |
| 127 | `gizmo-target == 3` | sketches | 🟢 |
| 128 | `gizmo-targets == 1` | 1 | 🟢 |
| 132 | `gizmo-commits == 6` | 6 | 🟢 |
| 135 | `selected-curves == 2` | Circle2,Circle1 | 🟢 |
| 139 | `panels == 0` | 0 | 🟢 |
| 140 | `last-step == 1` | Loft#3 | 🟢 |
| 141 | `bodies == 1` | 1 | 🟢 |
| 142 | `volume > 1` | 1.282817 | 🟢 |
| 143 | `sound == 1` | 1 | 🟢 |
| 145 | `volume ~ 1.282817` | 1.282817 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 218GizmoMove | 65 | `251364fa81fedfe5` |
| 219GizmoCombined | 104 | `931026bb31113f74` |
| 220GizmoScaled | 144 | `3b0f21b7b520e65b` |
| 221GizmoFace | 170 | `541283a62cab65a4` |
| 222GizmoFaceRotate | 216 | `d8c67c2a69bca605` |
| 223GizmoSketch | 287 | `dcf6ef9dcfa2186a` |
| 224GizmoLoft | 324 | `aff3e47b9683bd63` |

## Journal

- New document (previous recovery copy kept)
- Block: click the first corner · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Block: click the opposite corner
- Block: move up, click the height
- Block1 · New body · 0 ms
- Selected Block1 (6 faces)
- Move (Block1) · 0 ms
- Gizmo Z move 0.5 · Move (Block1) · 0 ms
- Gizmo: rotate
- Gizmo: scale
- Gizmo: move + rotate
- Gizmo: rotate + scale
- Gizmo: move + rotate + scale
- Rotate (Block1) · 0 ms
- Gizmo Z rotate -20.0° · Rotate (Block1) · 0 ms
- Scale (Block1) · 0 ms
- Gizmo X scale 1.400x · Scale (Block1) · 0 ms
- Undo
- Undo
- Undo
- Selected Extrude#2.Cap(End)
- Offset 0.4 on 1 face(s) · 1 ms
- Gizmo Z move 0.4 · push / pull · Offset 0.4 on 1 face(s) · 1 ms
- Gizmo: rotate + scale
- Gizmo: rotate
- Rotate -0.08727 on 1 face(s) · 1 ms
- Gizmo X rotate -5.0° · face rotate · Rotate -0.08727 on 1 face(s) · 1 ms
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
- Loft1 · 2 sections · solid · 6 ms
- Lofted 2 closed section(s) into a solid shell (2 plane caps) · sound · 6 ms
