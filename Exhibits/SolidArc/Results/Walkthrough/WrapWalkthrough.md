# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 810 · 471 frames · 28.6184 s · slowest frame 331.845 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/WrapWalkthrough.txt
- Expectations: 37 / 37 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 18 | `grid-plane == 0` | 0 | 🟢 |
| 37 | `sketches == 1` | Arc1 | 🟢 |
| 44 | `selected-curves == 1` | Arc1 | 🟢 |
| 45 | `curve-radius > 0.999` | 1 | 🟢 |
| 46 | `curve-radius < 1.001` | 1 | 🟢 |
| 49 | `transform == 1` | 1 | 🟢 |
| 52 | `transform-readout contains 1.5` | Scale xyz ×1.5 · typed 1.5 | 🟢 |
| 55 | `transform == 0` | 0 | 🟢 |
| 56 | `refused == 0` | 0 | 🟢 |
| 57 | `selected-curves == 1` | Arc1 | 🟢 |
| 58 | `curve-radius > 1.499` | 1.5 | 🟢 |
| 59 | `curve-radius < 1.501` | 1.5 | 🟢 |
| 68 | `sweep == 1` | 1 | 🟢 |
| 71 | `sweep-amount ~ 0.3` | 0.3 | 🟢 |
| 75 | `sweep == 0` | 0 | 🟢 |
| 76 | `refused == 0` | 0 | 🟢 |
| 77 | `sketches == 2` | Arc1,Offset1 | 🟢 |
| 78 | `status contains Offset` | Offset Arc1 by 0.3 · 0 ms | 🟢 |
| 91 | `sketches == 3` | Arc1,Offset1,Circle1 | 🟢 |
| 94 | `grid-plane == 1` | 1 | 🟢 |
| 95 | `orthographic == 0` | 0 | 🟢 |
| 124 | `sketches == 6` | Arc1,Offset1,Circle1,Rectangle1,Line1,Rectangle2 | 🟢 |
| 145 | `selected-curves == 4` | Rectangle1,Line1,Rectangle2,Circle1 | 🟢 |
| 153 | `wrap-pending == 1` | 1 | 🟢 |
| 154 | `status contains popup` | Wrap: perspective view — pick the projection axis in the popup (a parallel view, nump... | 🟢 |
| 158 | `wrap-pending == 0` | 0 | 🟢 |
| 159 | `refused == 0` | 0 | 🟢 |
| 160 | `wrap-curves == 10` | 10 | 🟢 |
| 161 | `selected-curves == 10` | Wrap1,Wrap2,Wrap3,Wrap4,Wrap5,Wrap6,Wrap7,Wrap8,Wrap9,Wrap10 | 🟢 |
| 162 | `status contains Wrap` | Wrap: 10 3D curve(s) round Circle1, projected along y · Ctrl+Z undoes | 🟢 |
| 174 | `selected-curves == 0` |  | 🟢 |
| 179 | `orthographic == 1` | 1 | 🟢 |
| 196 | `selected-curves == 4` | Rectangle1,Line1,Rectangle2,Circle1 | 🟢 |
| 201 | `wrap-pending == 0` | 0 | 🟢 |
| 202 | `refused == 0` | 0 | 🟢 |
| 203 | `wrap-curves == 10` | 10 | 🟢 |
| 204 | `status contains along the view` | Project: 10 3D curve(s) round Circle1, projected along the view · Ctrl+Z undoes | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 325SketchScaled | 93 | `d0a570eafdc26a2b` |
| 326CurveOffsetButton | 99 | `69852688f6a0adc7` |
| 327OffsetPreview | 111 | `af4d196653a8d3e7` |
| 328WrapPicked | 298 | `2e60db6a6fa317d2` |
| 329WrapAxisPopup | 308 | `c3d11d0347f21766` |
| 330WrapAroundCan | 348 | `c50cfc45aac17782` |
| 331ProjectOntoCan | 471 | `afbaccbdfbd258e7` |

## Journal

- New document (previous recovery copy kept)
- Arc on plane XY (or the body face under the first click): click start, end, then a point on the arc · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Arc1 · 0 ms
- Selection cleared
- Selected sketch Arc1 · 1 selected · L lofts the picks in order · the gizmo moves / turns them
- Scale 1 sketch(es) · X / Y / Z axis · type a value · click / Enter commits · Esc cancels
- Autosaved _ClaudeScratch/SolidArcWorkspace/WrapRecovery.solidarc
- Scale (Arc1) · 0 ms
- Offset Arc1 · move sideways (Shift fine, Ctrl free) or type a value · click / Enter commits · Esc cancels
- Offset Arc1 by 0.3 · 0 ms
- Selection cleared
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Circle1 · 0 ms
- Selection cleared
- Rectangle on plane XZ (or the body face under the first click): click two corners · Alt centre mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Rectangle1 · 0 ms
- Selection cleared
- Line on plane XZ (or the body face under the first click): click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Line1 · 0 ms
- Selection cleared
- Rectangle on plane XZ (or the body face under the first click): click two corners · Alt centre mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Rectangle2 · 0 ms
- Selection cleared
- Selected sketch Rectangle1 · 1 selected · L lofts the picks in order · the gizmo moves / turns them
- Selected sketch Line1 · 2 selected · L lofts the picks in order · the gizmo moves / turns them
- Autosaved _ClaudeScratch/SolidArcWorkspace/WrapRecovery.solidarc
- Selected sketch Rectangle2 · 3 selected · L lofts the picks in order · the gizmo moves / turns them
- Selected sketch Circle1 · 4 selected · L lofts the picks in order · the gizmo moves / turns them
- Wrap: perspective view — pick the projection axis in the popup (a parallel view, numpad 5, projects along the view by itself)
- Wrap Rectangle1, Line1, Rectangle2 round Circle1 · 3 ms
- Wrap: 10 3D curve(s) round Circle1, projected along y · Ctrl+Z undoes
- Undo
- Selected sketch Rectangle1 · 1 selected · L lofts the picks in order · the gizmo moves / turns them
- Autosaved _ClaudeScratch/SolidArcWorkspace/WrapRecovery.solidarc
- Selected sketch Line1 · 2 selected · L lofts the picks in order · the gizmo moves / turns them
- Selected sketch Rectangle2 · 3 selected · L lofts the picks in order · the gizmo moves / turns them
- Selected sketch Circle1 · 4 selected · L lofts the picks in order · the gizmo moves / turns them
- Project Rectangle1, Line1, Rectangle2 round Circle1 · 11 ms
- Project: 10 3D curve(s) round Circle1, projected along the view · Ctrl+Z undoes
