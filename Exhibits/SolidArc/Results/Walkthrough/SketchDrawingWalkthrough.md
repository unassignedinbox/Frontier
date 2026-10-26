# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 900 · 369 frames · 42.6734 s · slowest frame 1114.05 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/SketchDrawingWalkthrough.txt
- Expectations: 64 / 64 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 14 | `steps == 0` | 0 | 🟢 |
| 17 | `grid-plane == 0` | 0 | 🟢 |
| 22 | `drawing == Rectangle` | Rectangle | 🟢 |
| 23 | `drawing-plane == 0` | 0 | 🟢 |
| 24 | `drawing-centre == 0` | 0 | 🟢 |
| 29 | `drawing-points == 1` | 1 | 🟢 |
| 35 | `drawing == 0` |  | 🟢 |
| 36 | `steps == 1` | 1 | 🟢 |
| 37 | `sketches == Rectangle1` | Rectangle1 | 🟢 |
| 38 | `focused-caption == Rectangle1` | Rectangle1 | 🟢 |
| 39 | `sketch-curves == 4` | 4 | 🟢 |
| 46 | `bevel-active == 1` | 1 | 🟢 |
| 47 | `bevel-verb == 2` | 2 | 🟢 |
| 48 | `bevel-combination == New` | New | 🟢 |
| 54 | `bevel-size ~ 1.5` | 1.5 | 🟢 |
| 55 | `bevel-refusal == 0` |  | 🟢 |
| 56 | `bevel-preview-volume ~ 3` | 3 | 🟢 |
| 59 | `bevel-active == 0` | 0 | 🟢 |
| 60 | `steps == 2` | 2 | 🟢 |
| 61 | `volume ~ 3` | 3 | 🟢 |
| 62 | `faces == 6` | 6 | 🟢 |
| 63 | `sound == 1` | 1 | 🟢 |
| 68 | `drawing == Circle` | Circle | 🟢 |
| 69 | `drawing-centre == 1` | 1 | 🟢 |
| 74 | `steps == 3` | 3 | 🟢 |
| 75 | `focused-caption == Circle1` | Circle1 | 🟢 |
| 80 | `bevel-combination == Union` | Union | 🟢 |
| 83 | `bevel-combination == Difference` | Difference | 🟢 |
| 89 | `bevel-preview-volume ~ 2.2460177631` | 2.246017763 | 🟢 |
| 92 | `volume ~ 2.2460177631` | 2.246017763 | 🟢 |
| 93 | `genus == 1` | 1 | 🟢 |
| 94 | `sound == 1` | 1 | 🟢 |
| 100 | `drawing == Line` | Line | 🟢 |
| 107 | `drawing-points == 3` | 3 | 🟢 |
| 110 | `drawing == 0` |  | 🟢 |
| 111 | `sketches == Rectangle1,Circle1,Line1` | Rectangle1,Circle1,Line1 | 🟢 |
| 112 | `sketch-curves == 3` | 3 | 🟢 |
| 119 | `bevel-combination == Intersection` | Intersection | 🟢 |
| 122 | `bevel-combination == Union` | Union | 🟢 |
| 128 | `bevel-preview-volume ~ 2.5793510965` | 2.579351096 | 🟢 |
| 131 | `volume ~ 2.5793510965` | 2.579351096 | 🟢 |
| 132 | `sound == 1` | 1 | 🟢 |
| 138 | `drawing == Curve` | Curve | 🟢 |
| 139 | `drawing-form == Hermite` | Hermite | 🟢 |
| 146 | `drawing-points == 3` | 3 | 🟢 |
| 149 | `drawing-points == 2` | 2 | 🟢 |
| 156 | `drawing-form == Bezier` | Bezier | 🟢 |
| 159 | `drawing-form == NURBS` | NURBS | 🟢 |
| 162 | `drawing-form == Cubic spline` | Cubic spline | 🟢 |
| 166 | `drawing-form == Hermite` | Hermite | 🟢 |
| 172 | `drawing == 0` |  | 🟢 |
| 173 | `focused-caption == Curve1` | Curve1 | 🟢 |
| 174 | `sketch-curves == 1` | 1 | 🟢 |
| 186 | `bevel-refusal == 0` |  | 🟢 |
| 187 | `bevel-preview-volume ~ 2.4563510965` | 2.456351096 | 🟢 |
| 190 | `volume ~ 2.4563510965` | 2.456351096 | 🟢 |
| 191 | `sound == 1` | 1 | 🟢 |
| 192 | `refused == 0` | 0 | 🟢 |
| 197 | `drawing == Arc` | Arc | 🟢 |
| 204 | `drawing == Arc` | Arc | 🟢 |
| 205 | `drawing-refusal contains collinear` | Arc points are collinear | 🟢 |
| 208 | `drawing-points == 2` | 2 | 🟢 |
| 211 | `drawing == 0` |  | 🟢 |
| 212 | `steps == 8` | 8 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 30RectanglePreview | 32 | `f1d9e94493571388` |
| 31CurveForms | 258 | `cfeb57f711c50f8c` |
| 32HermiteClosing | 269 | `e977d3f64b4ebed0` |
| 33DrawnSolid | 369 | `1e6cb89ebb835135` |

## Journal

- New document (previous recovery copy kept)
- Rectangle on plane XY (or the body face under the first click): click two corners · Alt centre mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Rectangle1 · 0 ms
- Fill Rectangle1 · 0 ms
- Extrude Rectangle1 (E): move horizontally · Q union · W difference · E intersect · R new body · Enter / right-click applies · Esc aborts
- Bevel size 1.5 m (pinned)
- Extrude1 · Rectangle1 · New · 10 ms
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Circle1 · 7 ms
- Fill Circle1 · 6 ms
- Extrude Circle1 (E): move horizontally · Q union · W difference · E intersect · R new body · Enter / right-click applies · Esc aborts
- Extrude Boolean: Difference
- Bevel size -1.5 m (pinned)
- Extrude2 · Circle1 · Difference · 35 ms
- Line on plane XY (or the body face under the first click): click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Line1 · 22 ms
- Fill Line1 · 15 ms
- Extrude Line1 (E): move horizontally · Q union · W difference · E intersect · R new body · Enter / right-click applies · Esc aborts
- Extrude Boolean: Intersection
- Extrude Boolean: Union
- Bevel size 1 m (pinned)
- Extrude3 · Line1 · Union · 51 ms
- Curve on plane XY (or the body face under the first click): click points · 1 Hermite · 2 Bezier · 3 NURBS · 4 cubic spline · Enter / right-click finishes · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Point dropped · 2 left
- Autosaved _ClaudeScratch/SolidArcWorkspace/SketchRecovery.solidarc
- Curve form: Bezier
- Curve form: NURBS
- Curve form: Cubic spline
- Curve form: Hermite
- Curve1 · Hermite · 21 ms
- Fill Curve1 · 20 ms
- Extrude Curve1 (E): move horizontally · Q union · W difference · E intersect · R new body · Enter / right-click applies · Esc aborts
- Extrude Boolean: Difference
- Bevel size -0.5 m (pinned)
- Extrude4 · Curve1 · Difference · 836 ms
- Arc on plane XY (or the body face under the first click): click start, end, then a point on the arc · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Arc points are collinear
- Point dropped · 2 left
- Autosaved _ClaudeScratch/SolidArcWorkspace/SketchRecovery.solidarc
- Arc aborted (document untouched)
- View Isometric
