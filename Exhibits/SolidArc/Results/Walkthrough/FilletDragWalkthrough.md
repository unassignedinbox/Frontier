# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 900 · 175 frames · 11.4209 s · slowest frame 416.867 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/FilletDragWalkthrough.txt
- Expectations: 12 / 12 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 31 | `sketch-curve-total == 8` | 8 | 🟢 |
| 32 | `arc-widest ~ 90` | 90 | 🟢 |
| 48 | `live-staged == 1` | 1 | 🟢 |
| 49 | `live-arc-widest < 180` | 90 | 🟢 |
| 53 | `point-commits == 1` | 1 | 🟢 |
| 54 | `arc-widest ~ 90` | 90 | 🟢 |
| 55 | `sketch-curve-total == 8` | 8 | 🟢 |
| 69 | `live-arc-widest < 180` | 90 | 🟢 |
| 72 | `arc-widest ~ 90` | 90 | 🟢 |
| 75 | `arc-widest ~ 90` | 90 | 🟢 |
| 88 | `orthographic == 1` | 1 | 🟢 |
| 92 | `orthographic == 0` | 0 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 253FilletCentreDragLive | 94 | `6cf90df8873cf60f` |
| 254FilletCentreDragged | 101 | `0fe09cb2e4827f07` |
| 255CubeParallel | 167 | `4a947263235efdb7` |
| 256CubePerspective | 175 | `3aca7d205522837f` |

## Journal

- New document (previous recovery copy kept)
- Rectangle on plane XY (or the body face under the first click): click two corners · Alt centre mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Rectangle1 · 0 ms
- Selected sketch Rectangle1 · 1 selected · L lofts the picks in order · the gizmo moves / turns them
- Fillet sketch corners (B): move horizontally · D fillet · C chamfer · Enter / right-click applies · Esc aborts
- Bevel size 0.3 m (pinned)
- Fillet r 0.3 · every corner of Rectangle1 · 0 ms
- Selection cleared
- Drag Arc1.Centre of Rectangle1 · 1 ms
- Drag Arc1.Centre of Rectangle1 · 1 ms
- Undo
- Drag Point0 of Rectangle1 · 2 ms
- Drag Point0 of Rectangle1 · 2 ms
- Undo
- Selection cleared
- View Isometric
- Autosaved _ClaudeScratch/SolidArcWorkspace/FilletDragRecovery.solidarc
