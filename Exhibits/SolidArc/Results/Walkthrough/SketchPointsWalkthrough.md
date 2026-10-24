# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0
- Frame: 1600 x 960 · 319 frames · 32.7929 s · slowest frame 441.067 ms
- Script: /home/user/Frontier/Exhibits/SolidArc/Code/Scripts/SketchPointsWalkthrough.txt
- Expectations: 37 / 37 passed

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
| 169 | `live-staged == 1` | 1 | 🟢 |
| 170 | `live-volume > 6.1` | 9.6 | 🟢 |
| 171 | `volume ~ 6` | 6 | 🟢 |
| 177 | `gizmo-commits == 1` | 1 | 🟢 |
| 178 | `volume ~ 6` | 6 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 225SketchTools | 23 | `de85c496b2e08433` |
| 226SketchPoints | 204 | `4d9deb59887af1a4` |
| 227PointDragLive | 222 | `ded1c7141c6ec24c` |
| 228PointGizmoLive | 245 | `dd21cc86e74af86a` |
| 229PointsGizmoAll | 278 | `b41ef40496ef77c2` |
| 230BodyScaleLive | 308 | `e00c4b18133950bd` |

## Journal

- New document (previous recovery copy kept)
- Line on plane XY: click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · Esc aborts
- Line: click the start, then the end · Esc aborts
- Line1 · 0 ms
- Curve on plane XY: click points · 1 Hermite · 2 Bezier · 3 NURBS · 4 cubic spline · Enter / right-click finishes · Ctrl+Z drops a point · Esc aborts
- Curve · Bezier: click points · 1-4 switch form · Enter / right-click finishes · Ctrl+Z drops a point · Esc aborts
- Autosaved /home/user/Frontier/_ClaudeScratch/tmp/rt/sp/Rec.solidarc
- Curve1 · Bezier · 0 ms
- Curve on plane XY: click points · 1 Hermite · 2 Bezier · 3 NURBS · 4 cubic spline · Enter / right-click finishes · Ctrl+Z drops a point · Esc aborts
- Curve · NURBS: click points · 1-4 switch form · Enter / right-click finishes · Ctrl+Z drops a point · Esc aborts
- Autosaved /home/user/Frontier/_ClaudeScratch/tmp/rt/sp/Rec.solidarc
- Curve2 · NURBS · 0 ms
- Curve on plane XY: click points · 1 Hermite · 2 Bezier · 3 NURBS · 4 cubic spline · Enter / right-click finishes · Ctrl+Z drops a point · Esc aborts
- Curve · Cubic spline: click points · 1-4 switch form · Enter / right-click finishes · Ctrl+Z drops a point · Esc aborts
- Curve aborted (document untouched)
- Rectangle on plane XY: click two corners · Alt centre mode · Esc aborts
- Autosaved /home/user/Frontier/_ClaudeScratch/tmp/rt/sp/Rec.solidarc
- Rectangle1 · 0 ms
- Extrude Rectangle1 (E): move horizontally · Q union · W difference · E intersect · R new body · Enter / right-click applies · Esc aborts
- Bevel size 1 m (pinned)
- Extrude1 · Rectangle1 · New · 7 ms
- Drag Line2.End of Rectangle1 · 9 ms
- Drag Line2.End of Rectangle1 · 9 ms
- Drag Line2.End of Rectangle1 · 14 ms
- Gizmo Y move 1 · 1 sketch point · Drag Line2.End of Rectangle1 · 14 ms
- Rectangle1 · point 3 · 2 picked · the gizmo moves them in the sketch plane (drag a point to move it directly)
- Gizmo: rotate
- Gizmo: scale
- Gizmo: move + rotate
- Gizmo: rotate + scale
- Gizmo: move + rotate + scale
- Autosaved /home/user/Frontier/_ClaudeScratch/tmp/rt/sp/Rec.solidarc
- Selected Extrude1 (6 faces)
- Gizmo cancelled
