# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 900 · 307 frames · 32.8553 s · slowest frame 520.341 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/SketchCornersWalkthrough.txt
- Expectations: 18 / 18 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 33 | `volume ~ 2` | 2 | 🟢 |
| 40 | `bevel-active == 1` | 1 | 🟢 |
| 41 | `bevel-verb == 4` | 4 | 🟢 |
| 47 | `bevel-refusal == 0` |  | 🟢 |
| 48 | `bevel-preview-volume ~ 1.946350` | 1.946349541 | 🟢 |
| 52 | `bevel-active == 0` | 0 | 🟢 |
| 53 | `volume ~ 1.946350` | 1.946349541 | 🟢 |
| 54 | `sketch-curve-total == 8` | 8 | 🟢 |
| 55 | `sound == 1` | 1 | 🟢 |
| 74 | `picked-points == 1` | 1 | 🟢 |
| 79 | `bevel-chamfer == 1` | 1 | 🟢 |
| 85 | `bevel-refusal == 0` |  | 🟢 |
| 88 | `sketch-curve-total == 13` | 13 | 🟢 |
| 120 | `sketches == 5` | Rectangle1,Rectangle2,Line1,Line2,Line3 | 🟢 |
| 127 | `panels == 1` | 1 | 🟢 |
| 134 | `refused == 0` | 0 | 🟢 |
| 135 | `panels == 2` | 2 | 🟢 |
| 136 | `sheet == 1` | 1 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 239SketchFilletPreview | 93 | `4db6db35a4482bec` |
| 240SketchFillet | 101 | `c4cda0114c06131b` |
| 241SketchChamferCorner | 180 | `c54876c08246405b` |
| 242FanLoft | 307 | `a8be9a4322240b03` |

## Journal

- New document (previous recovery copy kept)
- Rectangle on plane XY (or the body face under the first click): click two corners · Alt centre mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Rectangle1 · 0 ms
- Fill Rectangle1 · 0 ms
- Extrude Rectangle1 (E): move horizontally · Q union · W difference · E intersect · R new body · A auto · Enter / right-click applies · Esc aborts
- Bevel size 1 m (pinned)
- Extrude1 · Rectangle1 · New · 0 ms
- Selected sketch Rectangle1 · 1 selected · L lofts the picks in order · the gizmo moves / turns them
- Fillet sketch corners (B): drag anywhere to size (it stops at the largest that fits) · type a size · D fillet · C chamfer · Enter / right-click applies · Esc aborts
- Bevel size 0.25 m (pinned)
- Fillet r 0.25 · every corner of Rectangle1 · 1 ms
- Selection cleared
- Rectangle on plane XY (or the body face under the first click): click two corners · Alt centre mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Rectangle2 · 1 ms
- Rectangle2 · point 2 · 1 picked · the gizmo moves them in the sketch plane (drag a point to move it directly)
- Fillet sketch corners (B): drag anywhere to size (it stops at the largest that fits) · type a size · D fillet · C chamfer · Enter / right-click applies · Esc aborts
- Bevel size 0.3 m (pinned)
- Chamfer 0.3 · 1 corner of Rectangle2 · 1 ms
- Selection cleared
- Line on plane XY (or the body face under the first click): click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Line1 · 1 ms
- Line on plane XY (or the body face under the first click): click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Line2 · 1 ms
- Line on plane XY (or the body face under the first click): click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Line3 · 1 ms
- Selected sketch Line1 · 1 selected · L lofts the picks in order · the gizmo moves / turns them
- Selected sketch Line2 · 2 selected · L lofts the picks in order · the gizmo moves / turns them
- Loft1 · 2 sections · 1 ms
- Lofted 2 curve(s) into a panel (sound) · 1 panel(s) · open sections stay a sheet · 1 ms
- Selected sketch Line1 · 1 selected · L lofts the picks in order · the gizmo moves / turns them
- Selected sketch Line3 · 2 selected · L lofts the picks in order · the gizmo moves / turns them
- Loft2 · 2 sections · 1 ms
- Lofted 2 curve(s) into a panel (sound) · 2 panel(s) · open sections stay a sheet · 1 ms
