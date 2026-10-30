# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 810 · 610 frames · 46.1001 s · slowest frame 428.541 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/CurveRequestWalkthrough.txt
- Expectations: 41 / 41 passed

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
| 97 | `selected-faces == 0` |  | 🟢 |
| 101 | `refused == 0` | 0 | 🟢 |
| 102 | `flips == 2` | 2 | 🟢 |
| 103 | `sound == 1` | 1 | 🟢 |
| 131 | `gizmo-target == 4` | points | 🟢 |
| 132 | `gizmo-flat == 1` | 1 | 🟢 |
| 133 | `gizmo-arrows == 2` | 2 | 🟢 |
| 139 | `gizmo-flat == 0` | 0 | 🟢 |
| 140 | `gizmo-arrows == 3` | 3 | 🟢 |
| 146 | `refused == 0` | 0 | 🟢 |
| 147 | `last-step contains Curve#` | Curve#2 | 🟢 |
| 148 | `sketches == 0` |  | 🟢 |
| 175 | `grid-plane == 0` | 0 | 🟢 |
| 182 | `snap contains curve` | On curve · Spline1 · Curve1 · 3D (the curve leaves the plane) | 🟢 |
| 183 | `snap-z > 0.5` | 0.7981191452 | 🟢 |
| 193 | `refused == 0` | 0 | 🟢 |
| 194 | `last-step contains Curve#` | Curve#2 | 🟢 |
| 195 | `selected-curves == 1` | Curve2 | 🟢 |
| 201 | `hidden-curves == 1` | 1 | 🟢 |
| 202 | `status contains hidden` | Hide selected · 0 body(ies), 1 curve(s) hidden | 🟢 |
| 206 | `hidden-curves == 0` | 0 | 🟢 |
| 249 | `sketch-curve-total == 3` | 3 | 🟢 |
| 252 | `trim == 1` | 1 | 🟢 |
| 255 | `trim-refusal == 0` |  | 🟢 |
| 259 | `refused == 0` | 0 | 🟢 |
| 260 | `sketch-curve-total == 4` | 4 | 🟢 |
| 263 | `trim == 0` | 0 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 351CurveLoftPicked | 110 | `764c6e46a99555d4` |
| 352CurveLoftSheet | 146 | `fc00c7941643668d` |
| 353CurveFlipFace | 165 | `6d1ec4440f561bb9` |
| 354CurveFlipped | 176 | `05fad324f36507ca` |
| 378NormalsCardObject | 191 | `1169bb916ff64bfd` |
| 379NormalsObjectFlipped | 202 | `fca3da15190582b6` |
| 355GizmoFlat | 275 | `22318751f418f496` |
| 356GizmoSpace | 299 | `f02c26870bf83c2d` |
| 357GizmoLifted | 320 | `a68c2812444cec76` |
| 358AltCurveSnap | 420 | `653aa72cabad1e0b` |
| 359AltCurveDrawn | 447 | `46e21f0669a6b2ba` |
| 360CurveHidden | 455 | `7429e89742968fb5` |
| 361CurveTrimHover | 594 | `9f035154264688c7` |
| 362CurveTrimmed | 610 | `f0cb8525b7923222` |

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
- Selected Loft1 (1 faces)
- Flip normals (Loft1) · 3 ms
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
