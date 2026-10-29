# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 810 · 173 frames · 16.9931 s · slowest frame 530.827 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/WrapPrecisionWalkthrough.txt
- Expectations: 12 / 12 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 20 | `grid-plane == 2` | 2 | 🟢 |
| 39 | `sketches == 1` | Curve1 | 🟢 |
| 44 | `grid-plane == 1` | 1 | 🟢 |
| 53 | `sketches == 2` | Curve1,Circle1 | 🟢 |
| 66 | `selected-curves == 2` | Circle1,Curve1 | 🟢 |
| 71 | `wrap-auto contains round x` | auto: round x (the curve's plane), along y (the shapes' plane) | 🟢 |
| 74 | `refused == 0` | 0 | 🟢 |
| 75 | `wrap-curves == 1` | 1 | 🟢 |
| 76 | `status contains round x` | Wrap: 1 3D curve(s) round Curve1, auto: round x (the curve's plane), along y (the shape... | 🟢 |
| 81 | `orthographic == 0` | 0 | 🟢 |
| 87 | `orthographic == 1` | 1 | 🟢 |
| 93 | `selected-curves == 0` |  | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 348WrapPrecisionPerspective | 157 | `5a6d5c00380e5f87` |
| 349WrapPrecisionParallel | 165 | `88f70441a4e0006a` |
| 350WrapPrecisionClean | 173 | `c057f05e1ce3cddc` |

## Journal

- New document (previous recovery copy kept)
- Curve on plane YZ (or the body face under the first click): click points · 1 Hermite · 2 Bezier · 3 NURBS · 4 cubic spline · Enter / right-click finishes · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Curve1 · Hermite · 0 ms
- Selection cleared
- Circle on plane XZ (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Circle1 · 0 ms
- Selection cleared
- Selected sketch Circle1 · 1 selected · L lofts the picks in order · the gizmo moves / turns them
- Selected sketch Curve1 · 2 selected · L lofts the picks in order · the gizmo moves / turns them
- Autosaved _ClaudeScratch/SolidArcWorkspace/WrapPrecisionRecovery.solidarc
- Wrap Circle1 round Curve1 · 157 ms
- Wrap: 1 3D curve(s) round Curve1, auto: round x (the curve's plane), along y (the shapes' plane) · Ctrl+Z undoes
- Selection cleared
