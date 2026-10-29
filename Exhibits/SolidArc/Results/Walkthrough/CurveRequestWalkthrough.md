# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 810 · 587 frames · 55.5217 s · slowest frame 507.389 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/CurveRequestWalkthrough.txt
- Expectations: 37 / 37 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 22 | `grid-plane == 2` | 2 | 🟢 |
| 47 | `sketches == 2` | Curve1,Curve2 | 🟢 |
| 56 | `selected-curves == 2` | Curve1,Curve2 | 🟢 |
| 62 | `trim == 1` | 1 | 🟢 |
| 65 | `trim == 0` | 0 | 🟢 |
| 66 | `refused == 0` | 0 | 🟢 |
| 67 | `last-step contains Loft` | Loft#3 | 🟢 |
| 68 | `bodies == 1` | 1 | 🟢 |
| 69 | `sound == 1` | 1 | 🟢 |
| 72 | `orthographic == 0` | 0 | 🟢 |
| 82 | `selected-faces == 1` | Loft#3.Side(Surface) | 🟢 |
| 86 | `refused == 0` | 0 | 🟢 |
| 87 | `flips == 1` | 1 | 🟢 |
| 88 | `sound == 1` | 1 | 🟢 |
| 116 | `gizmo-target == 4` | points | 🟢 |
| 117 | `gizmo-flat == 1` | 1 | 🟢 |
| 118 | `gizmo-arrows == 2` | 2 | 🟢 |
| 124 | `gizmo-flat == 0` | 0 | 🟢 |
| 125 | `gizmo-arrows == 3` | 3 | 🟢 |
| 131 | `refused == 0` | 0 | 🟢 |
| 132 | `last-step contains Curve#` | Curve#2 | 🟢 |
| 133 | `sketches == 0` |  | 🟢 |
| 160 | `grid-plane == 0` | 0 | 🟢 |
| 167 | `snap contains curve` | On curve · Spline1 · Curve1 · 3D (the curve leaves the plane) | 🟢 |
| 168 | `snap-z > 0.5` | 0.7981191452 | 🟢 |
| 178 | `refused == 0` | 0 | 🟢 |
| 179 | `last-step contains Curve#` | Curve#2 | 🟢 |
| 180 | `selected-curves == 1` | Curve2 | 🟢 |
| 186 | `hidden-curves == 1` | 1 | 🟢 |
| 187 | `status contains hidden` | Hide selected · 0 body(ies), 1 curve(s) hidden | 🟢 |
| 191 | `hidden-curves == 0` | 0 | 🟢 |
| 234 | `sketch-curve-total == 3` | 3 | 🟢 |
| 237 | `trim == 1` | 1 | 🟢 |
| 240 | `trim-refusal == 0` |  | 🟢 |
| 244 | `refused == 0` | 0 | 🟢 |
| 245 | `sketch-curve-total == 4` | 4 | 🟢 |
| 248 | `trim == 0` | 0 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 351CurveLoftPicked | 110 | `af0233a1ee5ab2af` |
| 352CurveLoftSheet | 146 | `3de4ca4e504419a7` |
| 353CurveFlipFace | 165 | `c886fb7cb1b8d860` |
| 354CurveFlipped | 179 | `522cbc1e4b2c7324` |
| 355GizmoFlat | 252 | `748f1480cce2481d` |
| 356GizmoSpace | 276 | `f1806de01abb7d6a` |
| 357GizmoLifted | 297 | `0b8156d650e9ddb1` |
| 358AltCurveSnap | 397 | `95d47e1f160b0771` |
| 359AltCurveDrawn | 424 | `17649a8253a1f977` |
| 360CurveHidden | 432 | `4e98d92eab1cac18` |
| 361CurveTrimHover | 571 | `1f561fa93cfa4aec` |
| 362CurveTrimmed | 587 | `b3c37420f83471b2` |

## Journal

- New document (previous recovery copy kept)
- Curve on plane YZ (or the body face under the first click): click points · 1 Hermite · 2 Bezier · 3 NURBS · 4 cubic spline · Enter / right-click finishes · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Curve1 · Hermite · 0 ms
- Selection cleared
- Curve on plane YZ (or the body face under the first click): click points · 1 Hermite · 2 Bezier · 3 NURBS · 4 cubic spline · Enter / right-click finishes · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Curve2 · Hermite · 0 ms
- Selection cleared
- Selected sketch Curve1 · 1 selected · L lofts the picks in order · the gizmo moves / turns them
- Selected sketch Curve2 · 2 selected · L lofts the picks in order · the gizmo moves / turns them
- Trim: click the piece of a curve to remove (between its crossings) · Esc ends
- Autosaved _ClaudeScratch/SolidArcWorkspace/CurveRequestRecovery.solidarc
- Trim ended
- Loft1 · 2 sections · 1 ms
- Lofted 2 curve(s) into a panel (sound) · 1 panel(s) · open sections stay a sheet · 1 ms
- Selected Loft#3.Side(Surface)
- Autosaved _ClaudeScratch/SolidArcWorkspace/CurveRequestRecovery.solidarc
- Flip normals (1 face(s) of Loft1) · 3 ms
- New document (previous recovery copy kept)
- Curve on plane YZ (or the body face under the first click): click points · 1 Hermite · 2 Bezier · 3 NURBS · 4 cubic spline · Enter / right-click finishes · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Autosaved _ClaudeScratch/SolidArcWorkspace/CurveRequestRecovery.solidarc
- Curve1 · Hermite · 0 ms
- Selection cleared
- Curve1 · point 1 · 1 picked · the gizmo moves them in the sketch plane (drag a point to move it directly)
- Autosaved _ClaudeScratch/SolidArcWorkspace/CurveRequestRecovery.solidarc
- Lift Curve1 points off its plane · 0 ms
- Gizmo Z move -0.1 · 1 sketch point · Lift Curve1 points off its plane · 0 ms
- Selection cleared
- New document (previous recovery copy kept)
- Curve on plane YZ (or the body face under the first click): click points · 1 Hermite · 2 Bezier · 3 NURBS · 4 cubic spline · Enter / right-click finishes · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Autosaved _ClaudeScratch/SolidArcWorkspace/CurveRequestRecovery.solidarc
- Curve1 · Hermite · 0 ms
- Selection cleared
- Curve on plane XY (or the body face under the first click): click points · 1 Hermite · 2 Bezier · 3 NURBS · 4 cubic spline · Enter / right-click finishes · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Autosaved _ClaudeScratch/SolidArcWorkspace/CurveRequestRecovery.solidarc
- Curve2 · 3D curve through 3 points · 0 ms
- Hide selected · 0 body(ies), 1 curve(s) hidden
- Unhide all · 0 body(ies), 0 curve(s) hidden
- New document (previous recovery copy kept)
- Curve on plane XY (or the body face under the first click): click points · 1 Hermite · 2 Bezier · 3 NURBS · 4 cubic spline · Enter / right-click finishes · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Autosaved _ClaudeScratch/SolidArcWorkspace/CurveRequestRecovery.solidarc
- Curve1 · Hermite · 0 ms
- Selection cleared
- Line on plane XY (or the body face under the first click): click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Line1 · 0 ms
- Selection cleared
- Line on plane XY (or the body face under the first click): click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Line2 · 0 ms
- Selection cleared
- Trim: click the piece of a curve to remove (between its crossings) · Esc ends
- Trim Spline1 of Curve1 · 0 ms
- Trim ended
