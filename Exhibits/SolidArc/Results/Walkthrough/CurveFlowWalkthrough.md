# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 900 · 270 frames · 46.8392 s · slowest frame 778.545 ms
- Script: Exhibits/SolidArc/Code/Scripts/CurveFlowWalkthrough.txt
- Expectations: 25 / 25 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 24 | `drawing-form contains ubic` | Cubic spline | 🟢 |
| 35 | `sketches == 1` | Curve1 | 🟢 |
| 36 | `point-total == 4` | 4 | 🟢 |
| 41 | `insert == 1` | 1 | 🟢 |
| 42 | `pick-mode == 1` | 1 | 🟢 |
| 45 | `trim-hover contains Spline1` | Curve1 · Spline1 | 🟢 |
| 46 | `insert-point == 4` | 4 | 🟢 |
| 47 | `trim-breaks == 1` | 1 | 🟢 |
| 53 | `refused == 0` | 0 | 🟢 |
| 54 | `point-total == 5` | 5 | 🟢 |
| 55 | `picked-points == 1` | 1 | 🟢 |
| 56 | `status contains Insert point on` | Insert point on Spline1 of Curve1 · 0 ms | 🟢 |
| 62 | `refused == 0` | 0 | 🟢 |
| 63 | `point-total == 5` | 5 | 🟢 |
| 67 | `insert == 0` | 0 | 🟢 |
| 78 | `snap lacks curve` | none | 🟢 |
| 81 | `snap contains curve` | On curve · Spline1 · Curve1 | 🟢 |
| 106 | `sketches == 3` | Curve1,Line1,Line2 | 🟢 |
| 117 | `selected-curves == 3` | Curve1,Line1,Line2 | 🟢 |
| 124 | `refused == 0` | 0 | 🟢 |
| 125 | `sketches == 4` | Curve1,Line1,Line2,Flow1 | 🟢 |
| 126 | `status contains Flow` | Flow Line1, Line2 along Curve1 into Flow1 (stretched) · 2 ms | 🟢 |
| 127 | `focused-step contains Sketch` | Sketch#4 | 🟢 |
| 133 | `inspector-tab == 3` | 3 | 🟢 |
| 139 | `inspector-tab == 0` | 0 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 311PointOnCurveHover | 68 | `ab1292399b2dae0c` |
| 312PointOnCurveAdded | 78 | `c5e27a1026cfaa82` |
| 313PointOnCurveMoved | 97 | `d3eef8bc391bf68b` |
| 314FlowPicked | 236 | `b27fa837ad6b534f` |
| 315FlowAlongCurve | 247 | `7bca5c776b22f6d3` |
| 316HistoryTab | 261 | `8b8ea76dc1b79884` |

## Journal

- New document (previous recovery copy kept)
- Curve on plane XY (or the body face under the first click): click points · 1 Hermite · 2 Bezier · 3 NURBS · 4 cubic spline · Enter / right-click finishes · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Curve form: Cubic spline
- Curve1 · Cubic spline · 0 ms
- Point: click a curve to add a point there (the shape stays) · Ctrl snaps onto its midpoint / quarter points / crossings · drag a point to move it · Esc ends
- Insert point on Spline1 of Curve1 · 0 ms
- Drag Spline1.Point2 of Curve1 · 0 ms
- Drag Spline1.Point2 of Curve1 · 0 ms
- Point ended
- Undo
- Line on plane XY (or the body face under the first click): click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Line aborted (document untouched)
- Selection cleared
- Autosaved _ClaudeScratch/SolidArcWorkspace/CurveFlowRecovery.solidarc
- Line on plane XY (or the body face under the first click): click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Line1 · 0 ms
- Line on plane XY (or the body face under the first click): click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Line2 · 0 ms
- Selected sketch Curve1 · 1 selected · L lofts the picks in order · the gizmo moves / turns them
- Selected sketch Line1 · 2 selected · L lofts the picks in order · the gizmo moves / turns them
- Selected sketch Line2 · 3 selected · L lofts the picks in order · the gizmo moves / turns them
- Autosaved _ClaudeScratch/SolidArcWorkspace/CurveFlowRecovery.solidarc
- Flow Line1, Line2 along Curve1 into Flow1 (stretched) · 2 ms
