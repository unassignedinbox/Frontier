# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 900 · 370 frames · 38.4038 s · slowest frame 1009.9 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/SketchDrawingWalkthrough.txt
- Expectations: 64 / 64 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 17 | `steps == 0` | 0 | 🟢 |
| 20 | `grid-plane == 0` | 0 | 🟢 |
| 25 | `drawing == Rectangle` | Rectangle | 🟢 |
| 26 | `drawing-plane == 0` | 0 | 🟢 |
| 27 | `drawing-centre == 0` | 0 | 🟢 |
| 32 | `drawing-points == 1` | 1 | 🟢 |
| 38 | `drawing == 0` |  | 🟢 |
| 39 | `steps == 1` | 1 | 🟢 |
| 40 | `sketches == Rectangle1` | Rectangle1 | 🟢 |
| 41 | `focused-caption == Rectangle1` | Rectangle1 | 🟢 |
| 42 | `sketch-curves == 4` | 4 | 🟢 |
| 49 | `bevel-active == 1` | 1 | 🟢 |
| 50 | `bevel-verb == 2` | 2 | 🟢 |
| 51 | `bevel-combination == New` | New | 🟢 |
| 57 | `bevel-size ~ 1.5` | 1.5 | 🟢 |
| 58 | `bevel-refusal == 0` |  | 🟢 |
| 59 | `bevel-preview-volume ~ 3` | 3 | 🟢 |
| 62 | `bevel-active == 0` | 0 | 🟢 |
| 63 | `steps == 2` | 2 | 🟢 |
| 64 | `volume ~ 3` | 3 | 🟢 |
| 65 | `faces == 6` | 6 | 🟢 |
| 66 | `sound == 1` | 1 | 🟢 |
| 71 | `drawing == Circle` | Circle | 🟢 |
| 72 | `drawing-centre == 1` | 1 | 🟢 |
| 77 | `steps == 3` | 3 | 🟢 |
| 78 | `focused-caption == Circle1` | Circle1 | 🟢 |
| 83 | `bevel-combination == Union` | Union | 🟢 |
| 86 | `bevel-combination == Difference` | Difference | 🟢 |
| 92 | `bevel-preview-volume ~ 2.2460177631` | 2.246017763 | 🟢 |
| 95 | `volume ~ 2.2460177631` | 2.246017763 | 🟢 |
| 96 | `genus == 1` | 1 | 🟢 |
| 97 | `sound == 1` | 1 | 🟢 |
| 103 | `drawing == Line` | Line | 🟢 |
| 110 | `drawing-points == 3` | 3 | 🟢 |
| 113 | `drawing == 0` |  | 🟢 |
| 114 | `sketches == Rectangle1,Circle1,Line1` | Rectangle1,Circle1,Line1 | 🟢 |
| 115 | `sketch-curves == 3` | 3 | 🟢 |
| 122 | `bevel-combination == Intersection` | Intersection | 🟢 |
| 125 | `bevel-combination == Union` | Union | 🟢 |
| 131 | `bevel-preview-volume ~ 2.5793510965` | 2.579351096 | 🟢 |
| 134 | `volume ~ 2.5793510965` | 2.579351096 | 🟢 |
| 135 | `sound == 1` | 1 | 🟢 |
| 141 | `drawing == Curve` | Curve | 🟢 |
| 142 | `drawing-form == Hermite` | Hermite | 🟢 |
| 149 | `drawing-points == 3` | 3 | 🟢 |
| 152 | `drawing-points == 2` | 2 | 🟢 |
| 159 | `drawing-form == Bezier` | Bezier | 🟢 |
| 162 | `drawing-form == NURBS` | NURBS | 🟢 |
| 165 | `drawing-form == Cubic spline` | Cubic spline | 🟢 |
| 169 | `drawing-form == Hermite` | Hermite | 🟢 |
| 175 | `drawing == 0` |  | 🟢 |
| 176 | `focused-caption == Curve1` | Curve1 | 🟢 |
| 177 | `sketch-curves == 1` | 1 | 🟢 |
| 189 | `bevel-refusal == 0` |  | 🟢 |
| 190 | `bevel-preview-volume ~ 2.4563510965` | 2.456351096 | 🟢 |
| 193 | `volume ~ 2.4563510965` | 2.456351096 | 🟢 |
| 194 | `sound == 1` | 1 | 🟢 |
| 195 | `refused == 0` | 0 | 🟢 |
| 200 | `drawing == Arc` | Arc | 🟢 |
| 207 | `drawing == Arc` | Arc | 🟢 |
| 208 | `drawing-refusal contains collinear` | Arc points are collinear | 🟢 |
| 211 | `drawing-points == 2` | 2 | 🟢 |
| 214 | `drawing == 0` |  | 🟢 |
| 215 | `steps == 8` | 8 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 30RectanglePreview | 33 | `1a824a177c8a470c` |
| 31CurveForms | 259 | `71c659a21b53fcc2` |
| 32HermiteClosing | 270 | `a33954bf4d136d33` |
| 33DrawnSolid | 370 | `bf7d0ed3bb42c46f` |

## Journal

- New document (previous recovery copy kept)
- Rectangle on plane XY (or the body face under the first click): click two corners · Alt centre mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Rectangle1 · 0 ms
- Fill Rectangle1 · 0 ms
- Extrude Rectangle1 (E): move horizontally · Q union · W difference · E intersect · R new body · Enter / right-click applies · Esc aborts
- Bevel size 1.5 m (pinned)
- Extrude1 · Rectangle1 · New · 0 ms
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Circle1 · 0 ms
- Fill Circle1 · 0 ms
- Extrude Circle1 (E): move horizontally · Q union · W difference · E intersect · R new body · Enter / right-click applies · Esc aborts
- Extrude Boolean: Difference
- Bevel size -1.5 m (pinned)
- Extrude2 · Circle1 · Difference · 5 ms
- Line on plane XY (or the body face under the first click): click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Line1 · 0 ms
- Fill Line1 · 0 ms
- Extrude Line1 (E): move horizontally · Q union · W difference · E intersect · R new body · Enter / right-click applies · Esc aborts
- Extrude Boolean: Intersection
- Extrude Boolean: Union
- Bevel size 1 m (pinned)
- Extrude3 · Line1 · Union · 6 ms
- Curve on plane XY (or the body face under the first click): click points · 1 Hermite · 2 Bezier · 3 NURBS · 4 cubic spline · Enter / right-click finishes · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Point dropped · 2 left
- Autosaved _ClaudeScratch/SolidArcWorkspace/SketchRecovery.solidarc
- Curve form: Bezier
- Curve form: NURBS
- Curve form: Cubic spline
- Curve form: Hermite
- Curve1 · Hermite · 1 ms
- Fill Curve1 · 1 ms
- Extrude Curve1 (E): move horizontally · Q union · W difference · E intersect · R new body · Enter / right-click applies · Esc aborts
- Extrude Boolean: Difference
- Bevel size -0.5 m (pinned)
- Extrude4 · Curve1 · Difference · 794 ms
- Arc on plane XY (or the body face under the first click): click start, end, then a point on the arc · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Arc points are collinear
- Point dropped · 2 left
- Autosaved _ClaudeScratch/SolidArcWorkspace/SketchRecovery.solidarc
- Arc aborted (document untouched)
- View Isometric
