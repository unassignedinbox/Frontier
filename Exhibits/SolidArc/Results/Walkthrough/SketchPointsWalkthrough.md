# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 900 · 325 frames · 29.1667 s · slowest frame 508.364 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/SketchPointsWalkthrough.txt
- Expectations: 38 / 38 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 25 | `drawing == 1` | Line | 🟢 |
| 26 | `drawing-segment == 1` | 1 | 🟢 |
| 31 | `drawing == 0` |  | 🟢 |
| 32 | `sketches == 1` | Line1 | 🟢 |
| 39 | `drawing-form == 2` | Bezier | 🟢 |
| 50 | `drawing == 0` |  | 🟢 |
| 51 | `sketches == 2` | Line1,Curve1 | 🟢 |
| 58 | `drawing-form == 3` | NURBS | 🟢 |
| 69 | `sketches == 3` | Line1,Curve1,Curve2 | 🟢 |
| 74 | `drawing-form == 4` | Cubic spline | 🟢 |
| 77 | `drawing == 0` |  | 🟢 |
| 97 | `volume ~ 2` | 2 | 🟢 |
| 98 | `bodies == 1` | 1 | 🟢 |
| 103 | `pick-mode == 1` | 1 | 🟢 |
| 115 | `live-staged == 1` | 1 | 🟢 |
| 116 | `live-volume ~ 3` | 3 | 🟢 |
| 117 | `volume ~ 2` | 2 | 🟢 |
| 121 | `point-commits == 1` | 1 | 🟢 |
| 122 | `volume ~ 3` | 3 | 🟢 |
| 123 | `refused == 0` | 0 | 🟢 |
| 124 | `picked-points == 1` | 1 | 🟢 |
| 125 | `picked-point-x ~ 3` | 3 | 🟢 |
| 126 | `picked-point-y ~ 1` | 1 | 🟢 |
| 129 | `gizmo-target == 4` | points | 🟢 |
| 138 | `live-staged == 1` | 1 | 🟢 |
| 139 | `live-volume > 3.05` | 6.6 | 🟢 |
| 143 | `gizmo-commits == 1` | 1 | 🟢 |
| 144 | `volume > 3.05` | 6.6 | 🟢 |
| 145 | `refused == 0` | 0 | 🟢 |
| 150 | `picked-points == 2` | 2 | 🟢 |
| 157 | `gizmo-modes == 7` | 7 | 🟢 |
| 165 | `gizmo-target == 2` | bodies | 🟢 |
| 175 | `live-staged == 1` | 1 | 🟢 |
| 176 | `live-volume > 7.0` | 10.56 | 🟢 |
| 177 | `volume < 7.0` | 6.6 | 🟢 |
| 183 | `gizmo-commits == 1` | 1 | 🟢 |
| 184 | `volume < 7.0` | 6.6 | 🟢 |
| 185 | `volume > 3.05` | 6.6 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 225SketchTools | 24 | `1d7a57290c1d37ee` |
| 226SketchPoints | 210 | `407504c7939946d6` |
| 227PointDragLive | 228 | `4fad0d6b2fa5982f` |
| 228PointGizmoLive | 251 | `485ce23b28edfa55` |
| 229PointsGizmoAll | 284 | `7dea3cd5d6506517` |
| 230BodyScaleLive | 314 | `0802160913d2134c` |

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
- Extrude1 · Rectangle1 · New · 0 ms
- Drag Line2.End of Rectangle1 · 0 ms
- Drag Line2.End of Rectangle1 · 0 ms
- Drag Line2.End of Rectangle1 · 0 ms
- Gizmo Y move 1.2 · 1 sketch point · Drag Line2.End of Rectangle1 · 0 ms
- Rectangle1 · point 3 · 2 picked · the gizmo moves them in the sketch plane (drag a point to move it directly)
- Gizmo: rotate
- Gizmo: scale
- Gizmo: move + rotate
- Gizmo: rotate + scale
- Gizmo: move + rotate + scale
- Autosaved _ClaudeScratch/SolidArcWorkspace/SketchPointsRecovery.solidarc
- Selected Extrude1 (6 faces)
- Gizmo cancelled
