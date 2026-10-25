# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0
- Frame: 1600 x 900 · 299 frames · 25.6845 s · slowest frame 466.54 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/SketchCornersWalkthrough.txt
- Expectations: 18 / 18 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 30 | `volume ~ 2` | 2 | 🟢 |
| 37 | `bevel-active == 1` | 1 | 🟢 |
| 38 | `bevel-verb == 4` | 4 | 🟢 |
| 44 | `bevel-refusal == 0` |  | 🟢 |
| 45 | `bevel-preview-volume ~ 1.946350` | 1.946349541 | 🟢 |
| 49 | `bevel-active == 0` | 0 | 🟢 |
| 50 | `volume ~ 1.946350` | 1.946349541 | 🟢 |
| 51 | `sketch-curve-total == 8` | 8 | 🟢 |
| 52 | `sound == 1` | 1 | 🟢 |
| 68 | `picked-points == 1` | 1 | 🟢 |
| 73 | `bevel-chamfer == 1` | 1 | 🟢 |
| 79 | `bevel-refusal == 0` |  | 🟢 |
| 82 | `sketch-curve-total == 13` | 13 | 🟢 |
| 114 | `sketches == 5` | Rectangle1,Rectangle2,Line1,Line2,Line3 | 🟢 |
| 121 | `panels == 1` | 1 | 🟢 |
| 128 | `refused == 0` | 0 | 🟢 |
| 129 | `panels == 2` | 2 | 🟢 |
| 130 | `sheet == 1` | 1 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 239SketchFilletPreview | 92 | `3a63948b6d5f2a1e` |
| 240SketchFillet | 100 | `4b83ee05078a29d1` |
| 241SketchChamferCorner | 172 | `0c9d2d46f8ed52d4` |
| 242FanLoft | 299 | `dfc11717626c7191` |

## Journal

- New document (previous recovery copy kept)
- Rectangle on plane XY (or the body face under the first click): click two corners · Alt centre mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Rectangle1 · 0 ms
- Fill Rectangle1 · 0 ms
- Extrude Rectangle1 (E): move horizontally · Q union · W difference · E intersect · R new body · Enter / right-click applies · Esc aborts
- Bevel size 1 m (pinned)
- Extrude1 · Rectangle1 · New · 7 ms
- Selected sketch Rectangle1 · 1 selected · L lofts the picks in order · the gizmo moves / turns them
- Fillet sketch corners (B): move horizontally · D fillet · C chamfer · Enter / right-click applies · Esc aborts
- Bevel size 0.25 m (pinned)
- Fillet r 0.25 · every corner of Rectangle1 · 32 ms
- Selection cleared
- Rectangle on plane XY (or the body face under the first click): click two corners · Alt centre mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Rectangle2 · 21 ms
- Rectangle2 · point 2 · 1 picked · the gizmo moves them in the sketch plane (drag a point to move it directly)
- Fillet sketch corners (B): move horizontally · D fillet · C chamfer · Enter / right-click applies · Esc aborts
- Bevel size 0.3 m (pinned)
- Chamfer 0.3 · 1 corner of Rectangle2 · 21 ms
- Selection cleared
- Line on plane XY (or the body face under the first click): click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Line1 · 22 ms
- Line on plane XY (or the body face under the first click): click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Line2 · 20 ms
- Line on plane XY (or the body face under the first click): click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Line3 · 21 ms
- Selected sketch Line1 · 1 selected · L lofts the picks in order · the gizmo moves / turns them
- Selected sketch Line2 · 2 selected · L lofts the picks in order · the gizmo moves / turns them
- Loft1 · 2 sections · 21 ms
- Lofted 2 curve(s) into a panel (sound) · 1 panel(s) · open sections stay a sheet · 21 ms
- Selected sketch Line1 · 1 selected · L lofts the picks in order · the gizmo moves / turns them
- Selected sketch Line3 · 2 selected · L lofts the picks in order · the gizmo moves / turns them
- Loft2 · 2 sections · 21 ms
- Lofted 2 curve(s) into a panel (sound) · 2 panel(s) · open sections stay a sheet · 21 ms
