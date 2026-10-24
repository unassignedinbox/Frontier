# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0
- Frame: 1600 x 960 · 336 frames · 30.8562 s · slowest frame 386.883 ms
- Script: /home/user/Frontier/Exhibits/SolidArc/Code/Scripts/GizmoWalkthrough.txt
- Expectations: 33 / 33 passed

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
| 135 | `panels > 0` | 1 | 🟢 |
| 138 | `bodies == 1` | 1 | 🟢 |
| 139 | `volume > 1` | 1.282817 | 🟢 |
| 140 | `sound == 1` | 1 | 🟢 |
| 142 | `volume ~ 1.282817` | 1.282817 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 218GizmoMove | 64 | `f73327fe17736bce` |
| 219GizmoCombined | 103 | `b502faef6e2e001a` |
| 220GizmoScaled | 143 | `58d054dbe9ef99d2` |
| 221GizmoFace | 169 | `c5e6b1de64d9d778` |
| 222GizmoFaceRotate | 215 | `f35da1f98bc4fbb5` |
| 223GizmoSketch | 286 | `c18d0017516ddb3c` |
| 224GizmoLoft | 336 | `80e8307500f83be0` |

## Journal

- New document (previous recovery copy kept)
- Block: click the first corner · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Block: click the opposite corner
- Block: move up, click the height
- Block1 · New body · 7 ms
- Selected Block1 (6 faces)
- Move (Block1) · 6 ms
- Gizmo Z move 0.5 · Move (Block1) · 6 ms
- Gizmo: rotate
- Gizmo: scale
- Gizmo: move + rotate
- Gizmo: rotate + scale
- Gizmo: move + rotate + scale
- Rotate (Block1) · 6 ms
- Gizmo Z rotate -20.0° · Rotate (Block1) · 6 ms
- Scale (Block1) · 18 ms
- Gizmo X scale 1.400x · Scale (Block1) · 18 ms
- Undo
- Undo
- Undo
- Selected Extrude#2.Cap(End)
- Offset 0.4 on 1 face(s) · 13 ms
- Gizmo Z move 0.4 · push / pull · Offset 0.4 on 1 face(s) · 13 ms
- Gizmo: rotate + scale
- Gizmo: rotate
- Rotate -0.08727 on 1 face(s) · 16 ms
- Gizmo X rotate -5.0° · face rotate · Rotate -0.08727 on 1 face(s) · 16 ms
- New document (previous recovery copy kept)
- Gizmo: move + rotate
- Gizmo: move
- Circle on plane XY: click centre and rim · V two-point mode · Esc aborts
- Circle1 · 0 ms
- Circle on plane XY: click centre and rim · V two-point mode · Esc aborts
- Circle2 · 0 ms
- Selected sketch Circle2 · 1 selected · L lofts the picks in order · the gizmo moves / turns them
- Move (Circle2) · 0 ms
- Gizmo Z move 0.7 · Move (Circle2) · 0 ms
- Selected sketch Circle1 · 2 selected · L lofts the picks in order · the gizmo moves / turns them
- Loft1 · 2 sections · 1 ms
- Lofted 2 curve(s) → panel (sound) · 1 panel(s) · 1 ms
- Close1 · plane caps · 6 ms
- Closed → solid · volume 1.28282 m³ · sound · 6 ms
