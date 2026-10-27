# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 900 · 176 frames · 16.1359 s · slowest frame 529.085 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/FilletDragWalkthrough.txt
- Expectations: 12 / 12 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 34 | `sketch-curve-total == 8` | 8 | 🟢 |
| 35 | `arc-widest ~ 90` | 90 | 🟢 |
| 51 | `live-staged == 1` | 1 | 🟢 |
| 52 | `live-arc-widest < 180` | 90 | 🟢 |
| 56 | `point-commits == 1` | 1 | 🟢 |
| 57 | `arc-widest ~ 90` | 90 | 🟢 |
| 58 | `sketch-curve-total == 8` | 8 | 🟢 |
| 72 | `live-arc-widest < 180` | 90 | 🟢 |
| 75 | `arc-widest ~ 90` | 90 | 🟢 |
| 78 | `arc-widest ~ 90` | 90 | 🟢 |
| 91 | `orthographic == 1` | 1 | 🟢 |
| 95 | `orthographic == 0` | 0 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 253FilletCentreDragLive | 95 | `6c461ed944e2c794` |
| 254FilletCentreDragged | 102 | `05883ce347eeb286` |
| 255CubeParallel | 168 | `7b8257c7b9d9540b` |
| 256CubePerspective | 176 | `0f15f331d707108b` |

## Journal

- New document (previous recovery copy kept)
- Rectangle on plane XY (or the body face under the first click): click two corners · Alt centre mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Rectangle1 · 0 ms
- Selected sketch Rectangle1 · 1 selected · L lofts the picks in order · the gizmo moves / turns them
- Fillet sketch corners (B): drag anywhere to size (it stops at the largest that fits) · type a size · D fillet · C chamfer · Enter / right-click applies · Esc aborts
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
