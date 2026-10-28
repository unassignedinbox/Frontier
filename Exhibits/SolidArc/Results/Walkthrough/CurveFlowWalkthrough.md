# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 810 · 275 frames · 16.1031 s · slowest frame 356.189 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/CurveFlowWalkthrough.txt
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
| 125 | `refused == 0` | 0 | 🟢 |
| 126 | `sketches == 4` | Curve1,Line1,Line2,Flow1 | 🟢 |
| 127 | `status contains Flow` | Flow Line1, Line2 along Curve1 into Flow1 (stretched) · 1 ms | 🟢 |
| 128 | `focused-step contains Sketch` | Sketch#4 | 🟢 |
| 134 | `inspector-tab == 3` | 3 | 🟢 |
| 140 | `inspector-tab == 0` | 0 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 311PointOnCurveHover | 68 | `a02fc85e87acdf00` |
| 312PointOnCurveAdded | 78 | `7937ac6e853bdca8` |
| 313PointOnCurveMoved | 97 | `475d7df84eddc30e` |
| 314FlowPicked | 241 | `21dbfbb5220e0982` |
| 315FlowAlongCurve | 252 | `aab11f559b340fa7` |
| 316HistoryTab | 266 | `0c06a832da3888a7` |

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
- Flow Line1, Line2 along Curve1 into Flow1 (stretched) · 1 ms
