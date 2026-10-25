# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0
- Frame: 1600 x 900 · 294 frames · 24.0211 s · slowest frame 467.074 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/SketchCornersWalkthrough.txt
- Expectations: 18 / 18 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 28 | `volume ~ 2` | 2 | 🟢 |
| 35 | `bevel-active == 1` | 1 | 🟢 |
| 36 | `bevel-verb == 4` | 4 | 🟢 |
| 42 | `bevel-refusal == 0` |  | 🟢 |
| 43 | `bevel-preview-volume ~ 1.946350` | 1.946349541 | 🟢 |
| 47 | `bevel-active == 0` | 0 | 🟢 |
| 48 | `volume ~ 1.946350` | 1.946349541 | 🟢 |
| 49 | `sketch-curve-total == 8` | 8 | 🟢 |
| 50 | `sound == 1` | 1 | 🟢 |
| 66 | `picked-points == 1` | 1 | 🟢 |
| 71 | `bevel-chamfer == 1` | 1 | 🟢 |
| 77 | `bevel-refusal == 0` |  | 🟢 |
| 80 | `sketch-curve-total == 13` | 13 | 🟢 |
| 112 | `sketches == 5` | Rectangle1,Rectangle2,Line1,Line2,Line3 | 🟢 |
| 119 | `panels == 1` | 1 | 🟢 |
| 126 | `refused == 0` | 0 | 🟢 |
| 127 | `panels == 2` | 2 | 🟢 |
| 128 | `sheet == 1` | 1 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 239SketchFilletPreview | 87 | `81ce96ff2d7861b6` |
| 240SketchFillet | 95 | `804807a3811ab25a` |
| 241SketchChamferCorner | 167 | `f4a2c02092d6f5cd` |
| 242FanLoft | 294 | `22864bb0924722fd` |

## Journal

- New document (previous recovery copy kept)
- Rectangle on plane XY (or the body face under the first click): click two corners · Alt centre mode · hold Ctrl to snap to ends / midpoints / centres · Esc aborts
- Rectangle1 · 0 ms
- Extrude Rectangle1 (E): move horizontally · Q union · W difference · E intersect · R new body · Enter / right-click applies · Esc aborts
- Bevel size 1 m (pinned)
- Extrude1 · Rectangle1 · New · 7 ms
- Selected sketch Rectangle1 · 1 selected · L lofts the picks in order · the gizmo moves / turns them
- Fillet sketch corners (B): move horizontally · D fillet · C chamfer · Enter / right-click applies · Esc aborts
- Bevel size 0.25 m (pinned)
- Fillet r 0.25 · every corner of Rectangle1 · 32 ms
- Selection cleared
- Rectangle on plane XY (or the body face under the first click): click two corners · Alt centre mode · hold Ctrl to snap to ends / midpoints / centres · Esc aborts
- Rectangle2 · 23 ms
- Rectangle2 · point 2 · 1 picked · the gizmo moves them in the sketch plane (drag a point to move it directly)
- Fillet sketch corners (B): move horizontally · D fillet · C chamfer · Enter / right-click applies · Esc aborts
- Bevel size 0.3 m (pinned)
- Chamfer 0.3 · 1 corner of Rectangle2 · 21 ms
- Selection cleared
- Line on plane XY (or the body face under the first click): click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres · Esc aborts
- Line1 · 21 ms
- Line on plane XY (or the body face under the first click): click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres · Esc aborts
- Line2 · 21 ms
- Line on plane XY (or the body face under the first click): click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres · Esc aborts
- Line3 · 20 ms
- Selected sketch Line1 · 1 selected · L lofts the picks in order · the gizmo moves / turns them
- Selected sketch Line2 · 2 selected · L lofts the picks in order · the gizmo moves / turns them
- Loft1 · 2 sections · 22 ms
- Lofted 2 curve(s) into a panel (sound) · 1 panel(s) · open sections stay a sheet · 22 ms
- Selected sketch Line1 · 1 selected · L lofts the picks in order · the gizmo moves / turns them
- Selected sketch Line3 · 2 selected · L lofts the picks in order · the gizmo moves / turns them
- Loft2 · 2 sections · 22 ms
- Lofted 2 curve(s) into a panel (sound) · 2 panel(s) · open sections stay a sheet · 22 ms
