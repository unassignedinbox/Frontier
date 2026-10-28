# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 810 · 202 frames · 13.7688 s · slowest frame 335.02 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/ShellsWalkthrough.txt
- Expectations: 34 / 34 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 31 | `bodies == 1` | 1 | 🟢 |
| 32 | `volume ~ 4.6` | 4.6 | 🟢 |
| 37 | `drawing == 1` | Rectangle | 🟢 |
| 40 | `face-plane == 1` | Extrude#2.Cap(End) | 🟢 |
| 41 | `face-plane-z ~ 1` | 1 | 🟢 |
| 47 | `drawing-points == 1` | 1 | 🟢 |
| 48 | `drawing-last-x ~ 1.1` | 1.1 | 🟢 |
| 49 | `drawing-last-y ~ 0` | 0 | 🟢 |
| 52 | `drawing-points == 0` | 0 | 🟢 |
| 57 | `drawing-points == 1` | 1 | 🟢 |
| 58 | `drawing-last-x ~ 1.15` | 1.15 | 🟢 |
| 59 | `drawing-last-y ~ 0` | 0 | 🟢 |
| 60 | `snapped-points == 1` | 1 | 🟢 |
| 66 | `drawing == 0` |  | 🟢 |
| 67 | `sketches == 2` | Block1,Rectangle1 | 🟢 |
| 68 | `last-sketch-z ~ 1` | 1 | 🟢 |
| 69 | `last-sketch-normal-z ~ 1` | 1 | 🟢 |
| 78 | `sketches == 3` | Block1,Rectangle1,Circle1 | 🟢 |
| 79 | `last-sketch-z ~ 1` | 1 | 🟢 |
| 84 | `face-plane == 0` |  | 🟢 |
| 89 | `sketches == 4` | Block1,Rectangle1,Circle1,Circle2 | 🟢 |
| 90 | `last-sketch-z ~ 0` | 0 | 🟢 |
| 95 | `selected-curves == 2` | Circle1,Circle2 | 🟢 |
| 98 | `refused == 0` | 0 | 🟢 |
| 99 | `last-step == 1` | Loft#6 | 🟢 |
| 100 | `panels == 0` | 0 | 🟢 |
| 101 | `bodies == 2` | 2 | 🟢 |
| 102 | `sound == 1` | 1 | 🟢 |
| 103 | `volume ~ 6.432596` | 6.432595715 | 🟢 |
| 109 | `hidden-curves == 1` | 1 | 🟢 |
| 112 | `locked == 1` | 1 | 🟢 |
| 115 | `selected-bodies == 0` |  | 🟢 |
| 119 | `locked == 0` | 0 | 🟢 |
| 122 | `hidden-curves == 0` | 0 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 231SketchFacePlane | 62 | `991a967e0dff2885` |
| 232SketchMidpointOnFace | 87 | `b485f4f938d44c61` |
| 233SolidLoftShell | 162 | `6fc48c0beddab65a` |
| 234HiddenLocked | 187 | `a3954474bb283119` |

## Journal

- New document (previous recovery copy kept)
- Block: click the first corner · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Block: click the opposite corner
- Block: move up, click the height
- Block1 · New body · 0 ms
- Rectangle on plane XY (or the body face under the first click): click two corners · Alt centre mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Point dropped · 0 left
- Autosaved _ClaudeScratch/SolidArcWorkspace/ShellsRecovery.solidarc
- Rectangle1 · 0 ms
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Circle1 · 0 ms
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Circle2 · 0 ms
- Selected sketch Circle1 · 1 selected · L lofts the picks in order · the gizmo moves / turns them
- Selected sketch Circle2 · 2 selected · L lofts the picks in order · the gizmo moves / turns them
- Loft1 · 2 sections · solid · 4 ms
- Lofted 2 closed section(s) into a solid shell (2 plane caps) · sound · 4 ms
- Rectangle1 hidden
- Block1 locked (shown, not pickable)
- Block1 is locked (Outliner lock to unlock)
- Block1 unlocked
- Rectangle1 shown
