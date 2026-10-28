# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 900 · 226 frames · 16.4487 s · slowest frame 347.791 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/ExtrudeAxisWalkthrough.txt
- Expectations: 47 / 47 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 37 | `refused == 0` | 0 | 🟢 |
| 38 | `bodies == 1` | 1 | 🟢 |
| 39 | `volume ~ 8` | 8 | 🟢 |
| 40 | `faces == 6` | 6 | 🟢 |
| 48 | `knife-pending == 1` | 1 | 🟢 |
| 57 | `refused == 0` | 0 | 🟢 |
| 58 | `knife == 0` | 0 | 🟢 |
| 59 | `volume ~ 8` | 8 | 🟢 |
| 60 | `faces == 7` | 7 | 🟢 |
| 65 | `selected-faces == 1` | Extrude#2.Side(Line2)/1 | 🟢 |
| 68 | `bevel-verb == 1` | 1 | 🟢 |
| 69 | `bevel-axis == 1` | 1 | 🟢 |
| 72 | `bevel-size > -0.31` | -0.2976 | 🟢 |
| 73 | `bevel-size < -0.29` | -0.2976 | 🟢 |
| 74 | `bevel-preview-volume > 7.75` | 7.761764472 | 🟢 |
| 75 | `bevel-preview-volume < 7.77` | 7.761764472 | 🟢 |
| 79 | `bevel-size > 0.79` | 0.7979 | 🟢 |
| 80 | `bevel-size < 0.81` | 0.7979 | 🟢 |
| 81 | `bevel-preview-volume > 8.62` | 8.631508714 | 🟢 |
| 82 | `bevel-preview-volume < 8.645` | 8.631508714 | 🟢 |
| 86 | `bevel-size > -0.31` | -0.2976 | 🟢 |
| 87 | `bevel-size < -0.29` | -0.2976 | 🟢 |
| 90 | `refused == 0` | 0 | 🟢 |
| 91 | `bodies == 1` | 1 | 🟢 |
| 92 | `volume > 7.75` | 7.761764472 | 🟢 |
| 93 | `volume < 7.77` | 7.761764472 | 🟢 |
| 94 | `faces == 10` | 10 | 🟢 |
| 95 | `sound == 1` | 1 | 🟢 |
| 114 | `drawing == 0` |  | 🟢 |
| 115 | `refused == 0` | 0 | 🟢 |
| 122 | `bevel-verb == 2` | 2 | 🟢 |
| 123 | `bevel-axis == 1` | 1 | 🟢 |
| 126 | `bevel-size > 0.89` | 0.8989 | 🟢 |
| 127 | `bevel-size < 0.91` | 0.8989 | 🟢 |
| 128 | `bevel-combination == Union` | Union | 🟢 |
| 129 | `bevel-preview-volume > 8.07` | 8.085368472 | 🟢 |
| 130 | `bevel-preview-volume < 8.10` | 8.085368472 | 🟢 |
| 133 | `bevel-size > -0.31` | -0.296 | 🟢 |
| 134 | `bevel-size < -0.29` | -0.296 | 🟢 |
| 135 | `bevel-combination == Difference` | Difference | 🟢 |
| 136 | `bevel-preview-volume > 7.64` | 7.655204472 | 🟢 |
| 137 | `bevel-preview-volume < 7.665` | 7.655204472 | 🟢 |
| 141 | `refused == 0` | 0 | 🟢 |
| 142 | `bodies == 1` | 1 | 🟢 |
| 143 | `volume > 7.64` | 7.655204472 | 🟢 |
| 144 | `volume < 7.665` | 7.655204472 | 🟢 |
| 145 | `sound == 1` | 1 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 317ExtrudeAxisPushIn | 122 | `de48196d683d0ed6` |
| 318ExtrudeAxisPullOut | 132 | `f34c71ce1b566c5c` |
| 319ExtrudeEdgeNotch | 148 | `bcb665d3af0cbce6` |
| 320ExtrudeAxisSketch | 219 | `58351ad082f30a9c` |

## Journal

- New document (previous recovery copy kept)
- Block: click the first corner · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Block: click the opposite corner
- Block: move up, click the height
- Block1 · New body · 0 ms
- Rectangle on plane XY (or the body face under the first click): click two corners · Alt centre mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- 2D cutter · Rectangle · draw it on a face (live dimensions) · it imprints into the face (nothing is cut) · then pick the new face and E pushes it in / pulls it out · Esc cancels
- 2D cutter · Rectangle · draw it on a face (live dimensions) · it imprints into the face (nothing is cut) · then pick the new face and E pushes it in / pulls it out · Esc cancels
- Autosaved _ClaudeScratch/SolidArcWorkspace/ExtrudeAxisRecovery.solidarc
- Rectangle1 · 0 ms
- Imprint1 · Rectangle1 on Block1 · 1 ms
- Imprint1 · Rectangle1 on Block1 · 7 faces · click the new region and E: drag in to push it down, out to pull it up
- Selected Extrude#2.Side(Line2)/1
- Extrude (E): the face follows the pointer along its normal (Blender) · out adds, in pushes / cuts a pocket (extrude manifold) · click edits the faces · Enter / right-click applies · Esc aborts
- ExtrudeFaces -0.2976 on 1 face(s) · 1 ms
- Rectangle on plane XY (or the body face under the first click): click two corners · Alt centre mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Autosaved _ClaudeScratch/SolidArcWorkspace/ExtrudeAxisRecovery.solidarc
- Rectangle2 · 0 ms
- Fill Rectangle2 · 0 ms
- Extrude Rectangle2 (E, extrude manifold): the profile follows the pointer — into the body cuts a pocket, out of it adds · Q union · W difference · E intersect · R new body · A auto · Enter / right-click applies · Esc aborts
- Extrude1 · Rectangle2 · Difference · 1 ms
