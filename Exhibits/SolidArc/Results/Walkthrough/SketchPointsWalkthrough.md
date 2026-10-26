# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 900 · 324 frames · 21.749 s · slowest frame 367.112 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/SketchPointsWalkthrough.txt
- Expectations: 38 / 38 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 22 | `drawing == 1` | Line | 🟢 |
| 23 | `drawing-segment == 1` | 1 | 🟢 |
| 28 | `drawing == 0` |  | 🟢 |
| 29 | `sketches == 1` | Line1 | 🟢 |
| 36 | `drawing-form == 2` | Bezier | 🟢 |
| 47 | `drawing == 0` |  | 🟢 |
| 48 | `sketches == 2` | Line1,Curve1 | 🟢 |
| 55 | `drawing-form == 3` | NURBS | 🟢 |
| 66 | `sketches == 3` | Line1,Curve1,Curve2 | 🟢 |
| 71 | `drawing-form == 4` | Cubic spline | 🟢 |
| 74 | `drawing == 0` |  | 🟢 |
| 94 | `volume ~ 2` | 2 | 🟢 |
| 95 | `bodies == 1` | 1 | 🟢 |
| 100 | `pick-mode == 1` | 1 | 🟢 |
| 112 | `live-staged == 1` | 1 | 🟢 |
| 113 | `live-volume ~ 3` | 3 | 🟢 |
| 114 | `volume ~ 2` | 2 | 🟢 |
| 118 | `point-commits == 1` | 1 | 🟢 |
| 119 | `volume ~ 3` | 3 | 🟢 |
| 120 | `refused == 0` | 0 | 🟢 |
| 121 | `picked-points == 1` | 1 | 🟢 |
| 122 | `picked-point-x ~ 3` | 3 | 🟢 |
| 123 | `picked-point-y ~ 1` | 1 | 🟢 |
| 126 | `gizmo-target == 4` | points | 🟢 |
| 135 | `live-staged == 1` | 1 | 🟢 |
| 136 | `live-volume > 3.05` | 6.6 | 🟢 |
| 140 | `gizmo-commits == 1` | 1 | 🟢 |
| 141 | `volume > 3.05` | 6.6 | 🟢 |
| 142 | `refused == 0` | 0 | 🟢 |
| 147 | `picked-points == 2` | 2 | 🟢 |
| 154 | `gizmo-modes == 7` | 7 | 🟢 |
| 162 | `gizmo-target == 2` | bodies | 🟢 |
| 172 | `live-staged == 1` | 1 | 🟢 |
| 173 | `live-volume > 7.0` | 10.56 | 🟢 |
| 174 | `volume < 7.0` | 6.6 | 🟢 |
| 180 | `gizmo-commits == 1` | 1 | 🟢 |
| 181 | `volume < 7.0` | 6.6 | 🟢 |
| 182 | `volume > 3.05` | 6.6 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 225SketchTools | 23 | `3719c3ebea58db56` |
| 226SketchPoints | 209 | `6fdb71ef895bf934` |
| 227PointDragLive | 227 | `bd49f8bf815d182b` |
| 228PointGizmoLive | 250 | `8b3d3294ca6d666d` |
| 229PointsGizmoAll | 283 | `e33e2807501f524d` |
| 230BodyScaleLive | 313 | `cfadf9b5ed5552e4` |

## Journal

- New document (previous recovery copy kept)
- Line on plane XY (or the body face under the first click): click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Line: click the start, then the end · Esc aborts
- Line1 · 0 ms
- Curve on plane XY (or the body face under the first click): click points · 1 Hermite · 2 Bezier · 3 NURBS · 4 cubic spline · Enter / right-click finishes · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Curve · Bezier: click points · 1-4 switch form · Enter / right-click finishes · Ctrl+Z drops a point · Esc aborts
- Autosaved _ClaudeScratch/SolidArcWorkspace/SketchPointsRecovery.solidarc
- Curve1 · Bezier · 0 ms
- Curve on plane XY (or the body face under the first click): click points · 1 Hermite · 2 Bezier · 3 NURBS · 4 cubic spline · Enter / right-click finishes · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Curve · NURBS: click points · 1-4 switch form · Enter / right-click finishes · Ctrl+Z drops a point · Esc aborts
- Autosaved _ClaudeScratch/SolidArcWorkspace/SketchPointsRecovery.solidarc
- Curve2 · NURBS · 0 ms
- Curve on plane XY (or the body face under the first click): click points · 1 Hermite · 2 Bezier · 3 NURBS · 4 cubic spline · Enter / right-click finishes · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Curve · Cubic spline: click points · 1-4 switch form · Enter / right-click finishes · Ctrl+Z drops a point · Esc aborts
- Curve aborted (document untouched)
- Rectangle on plane XY (or the body face under the first click): click two corners · Alt centre mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Autosaved _ClaudeScratch/SolidArcWorkspace/SketchPointsRecovery.solidarc
- Rectangle1 · 0 ms
- Fill Rectangle1 · 0 ms
- Extrude Rectangle1 (E): move horizontally · Q union · W difference · E intersect · R new body · Enter / right-click applies · Esc aborts
- Bevel size 1 m (pinned)
- Extrude1 · Rectangle1 · New · 7 ms
- Drag Line2.End of Rectangle1 · 8 ms
- Drag Line2.End of Rectangle1 · 8 ms
- Drag Line2.End of Rectangle1 · 11 ms
- Gizmo Y move 1.2 · 1 sketch point · Drag Line2.End of Rectangle1 · 11 ms
- Rectangle1 · point 3 · 2 picked · the gizmo moves them in the sketch plane (drag a point to move it directly)
- Gizmo: rotate
- Gizmo: scale
- Gizmo: move + rotate
- Gizmo: rotate + scale
- Gizmo: move + rotate + scale
- Autosaved _ClaudeScratch/SolidArcWorkspace/SketchPointsRecovery.solidarc
- Selected Extrude1 (6 faces)
- Gizmo cancelled
