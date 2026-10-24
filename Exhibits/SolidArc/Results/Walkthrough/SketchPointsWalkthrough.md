# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0
- Frame: 1600 x 960 · 319 frames · 20.1138 s · slowest frame 365.297 ms
- Script: /home/user/Frontier/Exhibits/SolidArc/Code/Scripts/SketchPointsWalkthrough.txt
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
| 92 | `volume ~ 2` | 2 | 🟢 |
| 93 | `bodies == 1` | 1 | 🟢 |
| 98 | `pick-mode == 1` | 1 | 🟢 |
| 110 | `live-staged == 1` | 1 | 🟢 |
| 111 | `live-volume ~ 3` | 3 | 🟢 |
| 112 | `volume ~ 2` | 2 | 🟢 |
| 116 | `point-commits == 1` | 1 | 🟢 |
| 117 | `volume ~ 3` | 3 | 🟢 |
| 118 | `refused == 0` | 0 | 🟢 |
| 119 | `picked-points == 1` | 1 | 🟢 |
| 120 | `picked-point-x ~ 3` | 3 | 🟢 |
| 121 | `picked-point-y ~ 1` | 1 | 🟢 |
| 124 | `gizmo-target == 4` | points | 🟢 |
| 133 | `live-staged == 1` | 1 | 🟢 |
| 134 | `live-volume > 3.05` | 6 | 🟢 |
| 138 | `gizmo-commits == 1` | 1 | 🟢 |
| 139 | `volume > 3.05` | 6 | 🟢 |
| 140 | `refused == 0` | 0 | 🟢 |
| 145 | `picked-points == 2` | 2 | 🟢 |
| 152 | `gizmo-modes == 7` | 7 | 🟢 |
| 160 | `gizmo-target == 2` | bodies | 🟢 |
| 170 | `live-staged == 1` | 1 | 🟢 |
| 171 | `live-volume > 7.0` | 9.6 | 🟢 |
| 172 | `volume < 7.0` | 6 | 🟢 |
| 178 | `gizmo-commits == 1` | 1 | 🟢 |
| 179 | `volume < 7.0` | 6 | 🟢 |
| 180 | `volume > 3.05` | 6 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 225SketchTools | 23 | `d289a99170803503` |
| 226SketchPoints | 204 | `7654700e764dc2ed` |
| 227PointDragLive | 222 | `78ed12865f5dbcd7` |
| 228PointGizmoLive | 245 | `7eac30d41ccc7f67` |
| 229PointsGizmoAll | 278 | `fcb6aad84cbf76e3` |
| 230BodyScaleLive | 308 | `7036b2d343ba8df1` |

## Journal

- New document (previous recovery copy kept)
- Line on plane XY: click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · Esc aborts
- Line: click the start, then the end · Esc aborts
- Line1 · 0 ms
- Curve on plane XY: click points · 1 Hermite · 2 Bezier · 3 NURBS · 4 cubic spline · Enter / right-click finishes · Ctrl+Z drops a point · Esc aborts
- Curve · Bezier: click points · 1-4 switch form · Enter / right-click finishes · Ctrl+Z drops a point · Esc aborts
- Autosaved /home/user/Frontier/_ClaudeScratch/tmp/rt/SketchPoints/Rec.solidarc
- Curve1 · Bezier · 0 ms
- Curve on plane XY: click points · 1 Hermite · 2 Bezier · 3 NURBS · 4 cubic spline · Enter / right-click finishes · Ctrl+Z drops a point · Esc aborts
- Curve · NURBS: click points · 1-4 switch form · Enter / right-click finishes · Ctrl+Z drops a point · Esc aborts
- Autosaved /home/user/Frontier/_ClaudeScratch/tmp/rt/SketchPoints/Rec.solidarc
- Curve2 · NURBS · 0 ms
- Curve on plane XY: click points · 1 Hermite · 2 Bezier · 3 NURBS · 4 cubic spline · Enter / right-click finishes · Ctrl+Z drops a point · Esc aborts
- Curve · Cubic spline: click points · 1-4 switch form · Enter / right-click finishes · Ctrl+Z drops a point · Esc aborts
- Curve aborted (document untouched)
- Rectangle on plane XY: click two corners · Alt centre mode · Esc aborts
- Autosaved /home/user/Frontier/_ClaudeScratch/tmp/rt/SketchPoints/Rec.solidarc
- Rectangle1 · 0 ms
- Extrude Rectangle1 (E): move horizontally · Q union · W difference · E intersect · R new body · Enter / right-click applies · Esc aborts
- Bevel size 1 m (pinned)
- Extrude1 · Rectangle1 · New · 5 ms
- Drag Line2.End of Rectangle1 · 7 ms
- Drag Line2.End of Rectangle1 · 7 ms
- Drag Line2.End of Rectangle1 · 10 ms
- Gizmo Y move 1 · 1 sketch point · Drag Line2.End of Rectangle1 · 10 ms
- Rectangle1 · point 3 · 2 picked · the gizmo moves them in the sketch plane (drag a point to move it directly)
- Gizmo: rotate
- Gizmo: scale
- Gizmo: move + rotate
- Gizmo: rotate + scale
- Gizmo: move + rotate + scale
- Autosaved /home/user/Frontier/_ClaudeScratch/tmp/rt/SketchPoints/Rec.solidarc
- Selected Extrude1 (6 faces)
- Gizmo cancelled
