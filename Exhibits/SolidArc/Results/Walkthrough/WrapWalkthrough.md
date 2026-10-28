# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 810 · 546 frames · 47.5656 s · slowest frame 485.273 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/WrapWalkthrough.txt
- Expectations: 46 / 46 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 19 | `grid-plane == 0` | 0 | 🟢 |
| 38 | `sketches == 1` | Arc1 | 🟢 |
| 45 | `selected-curves == 1` | Arc1 | 🟢 |
| 46 | `curve-radius > 0.999` | 1 | 🟢 |
| 47 | `curve-radius < 1.001` | 1 | 🟢 |
| 50 | `transform == 1` | 1 | 🟢 |
| 53 | `transform-readout contains 1.5` | Scale xyz ×1.5 · typed 1.5 | 🟢 |
| 56 | `transform == 0` | 0 | 🟢 |
| 57 | `refused == 0` | 0 | 🟢 |
| 58 | `selected-curves == 1` | Arc1 | 🟢 |
| 59 | `curve-radius > 1.499` | 1.5 | 🟢 |
| 60 | `curve-radius < 1.501` | 1.5 | 🟢 |
| 69 | `sweep == 1` | 1 | 🟢 |
| 72 | `sweep-amount ~ 0.3` | 0.3 | 🟢 |
| 76 | `sweep == 0` | 0 | 🟢 |
| 77 | `refused == 0` | 0 | 🟢 |
| 78 | `sketches == 2` | Arc1,Offset1 | 🟢 |
| 79 | `status contains Offset` | Offset Arc1 by 0.3 · 0 ms | 🟢 |
| 92 | `sketches == 3` | Arc1,Offset1,Circle1 | 🟢 |
| 95 | `grid-plane == 1` | 1 | 🟢 |
| 96 | `orthographic == 0` | 0 | 🟢 |
| 125 | `sketches == 6` | Arc1,Offset1,Circle1,Rectangle1,Line1,Rectangle2 | 🟢 |
| 146 | `selected-curves == 4` | Rectangle1,Line1,Rectangle2,Circle1 | 🟢 |
| 152 | `wrap-auto contains round z` | auto: round z (the curve's plane), along y (the shapes' plane) | 🟢 |
| 153 | `wrap-auto contains along y` | auto: round z (the curve's plane), along y (the shapes' plane) | 🟢 |
| 157 | `wrap-pending == 0` | 0 | 🟢 |
| 158 | `status contains round z` | Wrap: 10 3D curve(s) round Circle1, auto: round z (the curve's plane), along y (the sha... | 🟢 |
| 159 | `status contains along y` | Wrap: 10 3D curve(s) round Circle1, auto: round z (the curve's plane), along y (the sha... | 🟢 |
| 160 | `refused == 0` | 0 | 🟢 |
| 161 | `wrap-curves == 10` | 10 | 🟢 |
| 162 | `selected-curves == 10` | Wrap1,Wrap2,Wrap3,Wrap4,Wrap5,Wrap6,Wrap7,Wrap8,Wrap9,Wrap10 | 🟢 |
| 163 | `status contains Wrap` | Wrap: 10 3D curve(s) round Circle1, auto: round z (the curve's plane), along y (the sha... | 🟢 |
| 175 | `selected-curves == 0` |  | 🟢 |
| 180 | `orthographic == 1` | 1 | 🟢 |
| 197 | `selected-curves == 4` | Rectangle1,Line1,Rectangle2,Circle1 | 🟢 |
| 202 | `wrap-pending == 0` | 0 | 🟢 |
| 203 | `refused == 0` | 0 | 🟢 |
| 204 | `wrap-curves == 10` | 10 | 🟢 |
| 205 | `status contains along y` | Project: 10 3D curve(s) round Circle1, auto: round z (the curve's plane), along y (the ... | 🟢 |
| 218 | `selected-curves == 0` |  | 🟢 |
| 221 | `grid-plane == 0` | 0 | 🟢 |
| 230 | `sketches == 7` | Arc1,Offset1,Circle1,Rectangle1,Line1,Rectangle2,Rectangle3 | 🟢 |
| 241 | `selected-curves == 2` | Rectangle3,Circle1 | 🟢 |
| 244 | `wrap-auto lacks round` |  | 🟢 |
| 247 | `wrap-pending == 1` | 1 | 🟢 |
| 248 | `status contains popup` | Wrap: the shapes share the curve's plane, so the axis is not implied — pick the proje... | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 325SketchScaled | 93 | `820500ffdb69df57` |
| 326CurveOffsetButton | 99 | `7e3a51abcac19c7d` |
| 327OffsetPreview | 111 | `9d8c9c0439e2afa1` |
| 328WrapPicked | 298 | `3bf61b22637ebe5f` |
| 329WrapAutoAxes | 300 | `ed63ee3f9fa11558` |
| 330WrapAroundCan | 340 | `be910ea6843605a9` |
| 331ProjectOntoCan | 463 | `e2e4a973496f563c` |
| 347WrapAxisPopupSamePlane | 546 | `8ad324a936646cb3` |

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
- Wrap Rectangle1, Line1, Rectangle2 round Circle1 · 4 ms
- Wrap: 10 3D curve(s) round Circle1, auto: round z (the curve's plane), along y (the shapes' plane) · Ctrl+Z undoes
- Undo
- Selected sketch Rectangle1 · 1 selected · L lofts the picks in order · the gizmo moves / turns them
- Autosaved _ClaudeScratch/SolidArcWorkspace/WrapRecovery.solidarc
- Selected sketch Line1 · 2 selected · L lofts the picks in order · the gizmo moves / turns them
- Selected sketch Rectangle2 · 3 selected · L lofts the picks in order · the gizmo moves / turns them
- Selected sketch Circle1 · 4 selected · L lofts the picks in order · the gizmo moves / turns them
- Project Rectangle1, Line1, Rectangle2 round Circle1 · 19 ms
- Project: 10 3D curve(s) round Circle1, auto: round z (the curve's plane), along y (the shapes' plane) · Ctrl+Z undoes
- Undo
- Rectangle on plane XY (or the body face under the first click): click two corners · Alt centre mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Rectangle3 · 0 ms
- Selection cleared
- Selected sketch Rectangle3 · 1 selected · L lofts the picks in order · the gizmo moves / turns them
- Selected sketch Circle1 · 2 selected · L lofts the picks in order · the gizmo moves / turns them
- Autosaved _ClaudeScratch/SolidArcWorkspace/WrapRecovery.solidarc
- Wrap: the shapes share the curve's plane, so the axis is not implied — pick the projection axis in the popup (a parallel view, numpad 5, projects along the view by itself)
