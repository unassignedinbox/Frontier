# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 810 · 129 frames · 7.95482 s · slowest frame 346.258 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/PointCutWalkthrough.txt
- Expectations: 20 / 20 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 29 | `sketches == 2` | Circle1,Circle2 | 🟢 |
| 30 | `sketch-curve-total == 2` | 2 | 🟢 |
| 35 | `split == 1` | 1 | 🟢 |
| 38 | `trim-point contains Intersection · Circle1 × Circle2` | Intersection · Circle1 × Circle2 | 🟢 |
| 39 | `trim-point-curves == 2` | 2 | 🟢 |
| 40 | `trim-breaks == 4` | 4 | 🟢 |
| 46 | `refused == 0` | 0 | 🟢 |
| 47 | `sketch-curve-total == 4` | 4 | 🟢 |
| 48 | `status contains Cut 2 curve` | Cut 2 curve(s) at (0.6, 0.8, 0) · Circle1, Circle2 · 0 ms | 🟢 |
| 51 | `split == 0` | 0 | 🟢 |
| 59 | `selected-curves == 1` | Circle1 | 🟢 |
| 62 | `transform == 1` | 1 | 🟢 |
| 67 | `transform-readout contains Move x (3, 0, 0)` | Move x (3, 0, 0) · typed 3 | 🟢 |
| 70 | `sketches == 3` | Circle1,Circle2,Circle3 | 🟢 |
| 71 | `sketch-curve-total == 6` | 6 | 🟢 |
| 72 | `selected-curves == 1` | Circle3 | 🟢 |
| 73 | `refused == 0` | 0 | 🟢 |
| 79 | `transform == 1` | 1 | 🟢 |
| 82 | `transform == 0` | 0 | 🟢 |
| 83 | `sketches == 3` | Circle1,Circle2,Circle3 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 308PointCutHover | 66 | `55a613d418d2d802` |
| 309PointCutDone | 81 | `32c242f8c230b030` |
| 310SketchDuplicate | 117 | `ef31807ea424304b` |

## Journal

- New document (previous recovery copy kept)
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Circle1 · 0 ms
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Circle2 · 0 ms
- Cut: click a line, arc or circle where it should break in two (a circle becomes two half arcs) · hold Ctrl to snap onto a crossing / end / midpoint: every curve through it breaks there (overlapping circles: both, at both crossings) · Esc ends
- Cut 2 curve(s) at (0.6, 0.8, 0) · Circle1, Circle2 · 0 ms
- Cut ended
- Selected sketch Circle1 · 1 selected · L lofts the picks in order · the gizmo moves / turns them
- Duplicate + Move 1 sketch(es) · X / Y / Z axis · Shift+X / Y / Z plane · type a value · click / Enter commits (at once: a copy in place) · Esc cancels
- Autosaved _ClaudeScratch/SolidArcWorkspace/PointCutRecovery.solidarc
- Duplicate (Circle1) · 0 ms
- Duplicate + Move 1 sketch(es) · X / Y / Z axis · Shift+X / Y / Z plane · type a value · click / Enter commits (at once: a copy in place) · Esc cancels
- Move cancelled
