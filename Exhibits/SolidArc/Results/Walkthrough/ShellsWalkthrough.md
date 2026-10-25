# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0
- Frame: 1600 x 900 · 201 frames · 17.6526 s · slowest frame 475.775 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/ShellsWalkthrough.txt
- Expectations: 34 / 34 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 28 | `bodies == 1` | 1 | 🟢 |
| 29 | `volume ~ 4.6` | 4.6 | 🟢 |
| 34 | `drawing == 1` | Rectangle | 🟢 |
| 37 | `face-plane == 1` | Extrude#2.Cap(End) | 🟢 |
| 38 | `face-plane-z ~ 1` | 1 | 🟢 |
| 44 | `drawing-points == 1` | 1 | 🟢 |
| 45 | `drawing-last-x ~ 1.1` | 1.1 | 🟢 |
| 46 | `drawing-last-y ~ 0` | 0 | 🟢 |
| 49 | `drawing-points == 0` | 0 | 🟢 |
| 54 | `drawing-points == 1` | 1 | 🟢 |
| 55 | `drawing-last-x ~ 1.15` | 1.15 | 🟢 |
| 56 | `drawing-last-y ~ 0` | 0 | 🟢 |
| 57 | `snapped-points == 1` | 1 | 🟢 |
| 63 | `drawing == 0` |  | 🟢 |
| 64 | `sketches == 2` | Block1,Rectangle1 | 🟢 |
| 65 | `last-sketch-z ~ 1` | 1 | 🟢 |
| 66 | `last-sketch-normal-z ~ 1` | 1 | 🟢 |
| 75 | `sketches == 3` | Block1,Rectangle1,Circle1 | 🟢 |
| 76 | `last-sketch-z ~ 1` | 1 | 🟢 |
| 81 | `face-plane == 0` |  | 🟢 |
| 86 | `sketches == 4` | Block1,Rectangle1,Circle1,Circle2 | 🟢 |
| 87 | `last-sketch-z ~ 0` | 0 | 🟢 |
| 92 | `selected-curves == 2` | Circle1,Circle2 | 🟢 |
| 95 | `refused == 0` | 0 | 🟢 |
| 96 | `last-step == 1` | Loft#6 | 🟢 |
| 97 | `panels == 0` | 0 | 🟢 |
| 98 | `bodies == 2` | 2 | 🟢 |
| 99 | `sound == 1` | 1 | 🟢 |
| 100 | `volume ~ 6.432596` | 6.432595715 | 🟢 |
| 106 | `hidden-curves == 1` | 1 | 🟢 |
| 109 | `locked == 1` | 1 | 🟢 |
| 112 | `selected-bodies == 0` |  | 🟢 |
| 116 | `locked == 0` | 0 | 🟢 |
| 119 | `hidden-curves == 0` | 0 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 231SketchFacePlane | 61 | `868d8a201e974fa4` |
| 232SketchMidpointOnFace | 86 | `cad4b0b603ffa994` |
| 233SolidLoftShell | 161 | `a23ca5604fa25010` |
| 234HiddenLocked | 186 | `69e2f5ff6ee70427` |

## Journal

- New document (previous recovery copy kept)
- Block: click the first corner · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Block: click the opposite corner
- Block: move up, click the height
- Block1 · New body · 12 ms
- Rectangle on plane XY (or the body face under the first click): click two corners · Alt centre mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Point dropped · 0 left
- Autosaved _ClaudeScratch/SolidArcWorkspace/ShellsRecovery.solidarc
- Rectangle1 · 8 ms
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Circle1 · 8 ms
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Circle2 · 8 ms
- Selected sketch Circle1 · 1 selected · L lofts the picks in order · the gizmo moves / turns them
- Selected sketch Circle2 · 2 selected · L lofts the picks in order · the gizmo moves / turns them
- Loft1 · 2 sections · solid · 17 ms
- Lofted 2 closed section(s) into a solid shell (2 plane caps) · sound · 17 ms
- Rectangle1 hidden
- Block1 locked (shown, not pickable)
- Block1 is locked (Outliner lock to unlock)
- Block1 unlocked
- Rectangle1 shown
