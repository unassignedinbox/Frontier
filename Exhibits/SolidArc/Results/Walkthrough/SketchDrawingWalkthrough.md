# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0
- Frame: 1600 x 900 · 349 frames · 33.6822 s · slowest frame 1343.2 ms
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
| 44 | `bevel-active == 1` | 1 | 🟢 |
| 45 | `bevel-verb == 2` | 2 | 🟢 |
| 46 | `bevel-combination == New` | New | 🟢 |
| 52 | `bevel-size ~ 1.5` | 1.5 | 🟢 |
| 53 | `bevel-refusal == 0` |  | 🟢 |
| 54 | `bevel-preview-volume ~ 3` | 3 | 🟢 |
| 57 | `bevel-active == 0` | 0 | 🟢 |
| 58 | `steps == 2` | 2 | 🟢 |
| 59 | `volume ~ 3` | 3 | 🟢 |
| 60 | `faces == 6` | 6 | 🟢 |
| 61 | `sound == 1` | 1 | 🟢 |
| 66 | `drawing == Circle` | Circle | 🟢 |
| 67 | `drawing-centre == 1` | 1 | 🟢 |
| 72 | `steps == 3` | 3 | 🟢 |
| 73 | `focused-caption == Circle1` | Circle1 | 🟢 |
| 76 | `bevel-combination == Union` | Union | 🟢 |
| 79 | `bevel-combination == Difference` | Difference | 🟢 |
| 85 | `bevel-preview-volume ~ 2.2460177631` | 2.246017763 | 🟢 |
| 88 | `volume ~ 2.2460177631` | 2.246017763 | 🟢 |
| 89 | `genus == 1` | 1 | 🟢 |
| 90 | `sound == 1` | 1 | 🟢 |
| 96 | `drawing == Line` | Line | 🟢 |
| 103 | `drawing-points == 3` | 3 | 🟢 |
| 106 | `drawing == 0` |  | 🟢 |
| 107 | `sketches == Rectangle1,Circle1,Line1` | Rectangle1,Circle1,Line1 | 🟢 |
| 108 | `sketch-curves == 3` | 3 | 🟢 |
| 113 | `bevel-combination == Intersection` | Intersection | 🟢 |
| 116 | `bevel-combination == Union` | Union | 🟢 |
| 122 | `bevel-preview-volume ~ 2.5793510965` | 2.579351096 | 🟢 |
| 125 | `volume ~ 2.5793510965` | 2.579351096 | 🟢 |
| 126 | `sound == 1` | 1 | 🟢 |
| 132 | `drawing == Curve` | Curve | 🟢 |
| 133 | `drawing-form == Hermite` | Hermite | 🟢 |
| 140 | `drawing-points == 3` | 3 | 🟢 |
| 143 | `drawing-points == 2` | 2 | 🟢 |
| 150 | `drawing-form == Bezier` | Bezier | 🟢 |
| 153 | `drawing-form == NURBS` | NURBS | 🟢 |
| 156 | `drawing-form == Cubic spline` | Cubic spline | 🟢 |
| 160 | `drawing-form == Hermite` | Hermite | 🟢 |
| 166 | `drawing == 0` |  | 🟢 |
| 167 | `focused-caption == Curve1` | Curve1 | 🟢 |
| 168 | `sketch-curves == 1` | 1 | 🟢 |
| 178 | `bevel-refusal == 0` |  | 🟢 |
| 179 | `bevel-preview-volume ~ 2.4563510965` | 2.456351096 | 🟢 |
| 182 | `volume ~ 2.4563510965` | 2.456351096 | 🟢 |
| 183 | `sound == 1` | 1 | 🟢 |
| 184 | `refused == 0` | 0 | 🟢 |
| 189 | `drawing == Arc` | Arc | 🟢 |
| 196 | `drawing == Arc` | Arc | 🟢 |
| 197 | `drawing-refusal contains collinear` | Arc points are collinear | 🟢 |
| 200 | `drawing-points == 2` | 2 | 🟢 |
| 203 | `drawing == 0` |  | 🟢 |
| 204 | `steps == 8` | 8 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 30RectanglePreview | 32 | `67c5e6768694a6b9` |
| 31CurveForms | 243 | `06076f5af3152a83` |
| 32HermiteClosing | 254 | `39cd0d0a6d7073ce` |
| 33DrawnSolid | 349 | `ee074d072f4abc30` |

## Journal

- New document (previous recovery copy kept)
- Rectangle on plane XY (or the body face under the first click): click two corners · Alt centre mode · hold Ctrl to snap to ends / midpoints / centres · Esc aborts
- Rectangle1 · 0 ms
- Extrude Rectangle1 (E): move horizontally · Q union · W difference · E intersect · R new body · Enter / right-click applies · Esc aborts
- Bevel size 1.5 m (pinned)
- Extrude1 · Rectangle1 · New · 11 ms
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres · Esc aborts
- Circle1 · 6 ms
- Extrude Circle1 (E): move horizontally · Q union · W difference · E intersect · R new body · Enter / right-click applies · Esc aborts
- Extrude Boolean: Difference
- Bevel size -1.5 m (pinned)
- Extrude2 · Circle1 · Difference · 34 ms
- Line on plane XY (or the body face under the first click): click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres · Esc aborts
- Line1 · 15 ms
- Extrude Line1 (E): move horizontally · Q union · W difference · E intersect · R new body · Enter / right-click applies · Esc aborts
- Extrude Boolean: Intersection
- Extrude Boolean: Union
- Bevel size 1 m (pinned)
- Extrude3 · Line1 · Union · 52 ms
- Curve on plane XY (or the body face under the first click): click points · 1 Hermite · 2 Bezier · 3 NURBS · 4 cubic spline · Enter / right-click finishes · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres · Esc aborts
- Point dropped · 2 left
- Autosaved _ClaudeScratch/SolidArcWorkspace/SketchRecovery.solidarc
- Curve form: Bezier
- Curve form: NURBS
- Curve form: Cubic spline
- Curve form: Hermite
- Curve1 · Hermite · 19 ms
- Extrude Curve1 (E): move horizontally · Q union · W difference · E intersect · R new body · Enter / right-click applies · Esc aborts
- Extrude Boolean: Difference
- Bevel size -0.5 m (pinned)
- Extrude4 · Curve1 · Difference · 836 ms
- Arc on plane XY (or the body face under the first click): click start, end, then a point on the arc · hold Ctrl to snap to ends / midpoints / centres · Esc aborts
- Arc points are collinear
- Point dropped · 2 left
- Autosaved _ClaudeScratch/SolidArcWorkspace/SketchRecovery.solidarc
- Arc aborted (document untouched)
- View Isometric
