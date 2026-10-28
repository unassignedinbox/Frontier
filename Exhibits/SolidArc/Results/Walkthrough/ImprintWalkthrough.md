# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 810 · 216 frames · 14.611 s · slowest frame 334.158 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/ImprintWalkthrough.txt
- Expectations: 30 / 30 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 31 | `refused == 0` | 0 | 🟢 |
| 32 | `bodies == 1` | 1 | 🟢 |
| 33 | `volume ~ 8` | 8 | 🟢 |
| 34 | `faces == 6` | 6 | 🟢 |
| 42 | `knife == 1` | 1 | 🟢 |
| 43 | `knife-pending == 1` | 1 | 🟢 |
| 44 | `drawing == Rectangle` | Rectangle | 🟢 |
| 53 | `refused == 0` | 0 | 🟢 |
| 54 | `knife == 0` | 0 | 🟢 |
| 55 | `bodies == 1` | 1 | 🟢 |
| 56 | `volume ~ 8` | 8 | 🟢 |
| 57 | `faces == 7` | 7 | 🟢 |
| 58 | `sound == 1` | 1 | 🟢 |
| 63 | `selected-faces == 1` | Extrude#2.Cap(End)/2 | 🟢 |
| 72 | `bevel-preview-volume ~ 7.925` | 7.925 | 🟢 |
| 75 | `refused == 0` | 0 | 🟢 |
| 76 | `volume ~ 7.925` | 7.925 | 🟢 |
| 77 | `faces == 11` | 11 | 🟢 |
| 78 | `sound == 1` | 1 | 🟢 |
| 85 | `knife-pending == 1` | 1 | 🟢 |
| 94 | `refused == 0` | 0 | 🟢 |
| 95 | `knife == 0` | 0 | 🟢 |
| 96 | `volume ~ 7.925` | 7.925 | 🟢 |
| 97 | `faces == 12` | 12 | 🟢 |
| 100 | `selected-faces == 1` | Extrude#2.Cap(End)/1/1 | 🟢 |
| 108 | `bevel-preview-volume ~ 7.425` | 7.425 | 🟢 |
| 112 | `refused == 0` | 0 | 🟢 |
| 113 | `bodies == 1` | 1 | 🟢 |
| 114 | `volume ~ 7.425` | 7.425 | 🟢 |
| 115 | `sound == 1` | 1 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 305ImprintMenu | 65 | `f7204dfc0233f52f` |
| 306ImprintIsland | 109 | `6d6ffc644b9f881c` |
| 307ImprintStep | 209 | `20f7955b22aa1608` |

## Journal

- New document (previous recovery copy kept)
- Block: click the first corner · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Block: click the opposite corner
- Block: move up, click the height
- Block1 · New body · 0 ms
- Rectangle on plane XY (or the body face under the first click): click two corners · Alt centre mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- 2D cutter · Rectangle · draw it on a face (live dimensions) · it imprints into the face (nothing is cut) · then pick the new face and E pushes it in / pulls it out · Esc cancels
- 2D cutter · Rectangle · draw it on a face (live dimensions) · it imprints into the face (nothing is cut) · then pick the new face and E pushes it in / pulls it out · Esc cancels
- Autosaved _ClaudeScratch/SolidArcWorkspace/ImprintRecovery.solidarc
- Rectangle1 · 0 ms
- Imprint1 · Rectangle1 on Block1 · 1 ms
- Imprint1 · Rectangle1 on Block1 · 7 faces · click the new region and E: drag in to push it down, out to pull it up
- Selected Extrude#2.Cap(End)/2
- Extrude (E): the face follows the pointer along its normal (Blender) · out adds, in pushes / cuts a pocket (extrude manifold) · click edits the faces · Enter / right-click applies · Esc aborts
- Bevel size -0.3 m (pinned)
- ExtrudeFaces -0.3 on 1 face(s) · 1 ms
- Line on plane XY (or the body face under the first click): click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- 2D cutter · Line · draw it on a face (live dimensions) · it imprints into the face (nothing is cut) · then pick the new face and E pushes it in / pulls it out · Esc cancels
- 2D cutter · Line · draw it on a face (live dimensions) · it imprints into the face (nothing is cut) · then pick the new face and E pushes it in / pulls it out · Esc cancels
- Autosaved _ClaudeScratch/SolidArcWorkspace/ImprintRecovery.solidarc
- Line1 · 0 ms
- Imprint2 · Line1 on Block1 · 2 ms
- Imprint2 · Line1 on Block1 · 12 faces · click the new region and E: drag in to push it down, out to pull it up
- Selected Extrude#2.Cap(End)/1/1
- Extrude (E): the face follows the pointer along its normal (Blender) · out adds, in pushes / cuts a pocket (extrude manifold) · click edits the faces · Enter / right-click applies · Esc aborts
- Bevel size -0.5 m (pinned)
- ExtrudeFaces -0.5 on 1 face(s) · 2 ms
