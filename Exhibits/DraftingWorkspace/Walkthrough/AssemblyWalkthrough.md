# DraftingWorkspaceCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0
- Frame: 1600 x 900 · 415 frames · 116.481 s · slowest frame 7945.51 ms
- Script: Experimental/DraftingWorkspace/Scripts/AssemblyWalkthrough.txt
- Expectations: 56 / 56 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 13 | `bodies == 1` | 1 | 🟢 |
| 36 | `refused == 0` | 0 | 🟢 |
| 62 | `refused == 0` | 0 | 🟢 |
| 63 | `bodies == 3` | 3 | 🟢 |
| 64 | `body-captions contains Cylinder3` | Block1,Cylinder3,Block2 | 🟢 |
| 65 | `body-captions contains Block2` | Block1,Cylinder3,Block2 | 🟢 |
| 66 | `centroid:Cylinder3 == (6.000000, 1.250000, 0.750000)` | (6.000000, 1.250000, 0.750000) | 🟢 |
| 74 | `joint == 1` | 1 | 🟢 |
| 75 | `joint-phase == 0` | 0 | 🟢 |
| 76 | `joint-kind == Revolute` | Revolute | 🟢 |
| 79 | `joint-hover contains circle centre on Cylinder3` | circle centre on Cylinder3 | 🟢 |
| 86 | `joint-phase == 1` | 1 | 🟢 |
| 89 | `joint-hover contains circle centre on Block1` | circle centre on Block1 | 🟢 |
| 92 | `joint-phase == 2` | 2 | 🟢 |
| 93 | `joint-preview == 1` | 1 | 🟢 |
| 94 | `joint-refusal == 0` |  | 🟢 |
| 95 | `joint-picks contains circle centre on Cylinder3 -> circle centre on Block1` | circle centre on Cylinder3 -> circle centre on Block1 | 🟢 |
| 96 | `preview-resumed > 0` | 12 | 🟢 |
| 102 | `joint-flipped == 1` | 1 | 🟢 |
| 108 | `joint-refusal == 0` |  | 🟢 |
| 114 | `joint == 0` | 0 | 🟢 |
| 115 | `joint-commits == 1` | 1 | 🟢 |
| 116 | `refused == 0` | 0 | 🟢 |
| 117 | `joints == 1` | 1 | 🟢 |
| 118 | `joint-gap ~ 0.25` | 0.25 | 🟢 |
| 119 | `centroid:Cylinder3 == (3.000000, 1.250000, 0.500000)` | (3.000000, 1.250000, 0.500000) | 🟢 |
| 128 | `joint-phase == 1` | 1 | 🟢 |
| 131 | `joint-phase == 1` | 1 | 🟢 |
| 132 | `status contains another body` | Joint: pick the second joint origin on another body | 🟢 |
| 135 | `joint == 0` | 0 | 🟢 |
| 143 | `selected-faces == 2` | Extrude#12.Cap(End),Extrude#10.Cap(End) | 🟢 |
| 146 | `joint-phase == 2` | 2 | 🟢 |
| 147 | `joint-kind == Revolute` | Revolute | 🟢 |
| 148 | `joint-picks contains plane centroid on Block2 -> plane centroid on Cylinder3` | plane centroid on Block2 -> plane centroid on Cylinder3 | 🟢 |
| 157 | `joint-commits == 2` | 2 | 🟢 |
| 158 | `joints == 2` | 2 | 🟢 |
| 159 | `joint-gap ~ 0.25` | 0.25 | 🟢 |
| 160 | `bounds:Block2 == (2.800000, 0.250000, 1.250000) … (3.200000, 2.250000, 1.450000)` | (2.800000, 0.250000, 1.250000) … (3.200000, 2.250000, 1.450000) | 🟢 |
| 161 | `focused-step == Joint#14` | Joint#14 | 🟢 |
| 166 | `undo == 9` | 9 | 🟢 |
| 169 | `refused == 0` | 0 | 🟢 |
| 170 | `parameter-drag > 2` | 6 | 🟢 |
| 171 | `undo == 10` | 10 | 🟢 |
| 175 | `undo == 9` | 9 | 🟢 |
| 176 | `bounds:Block2 == (2.800000, 0.250000, 1.250000) … (3.200000, 2.250000, 1.450000)` | (2.800000, 0.250000, 1.250000) … (3.200000, 2.250000, 1.450000) | 🟢 |
| 185 | `selected-bodies == 2` | Extrude#12,Extrude#2 | 🟢 |
| 188 | `measure contains Block2 + Block1` | Block2 + Block1 | 🟢 |
| 189 | `measure-interference contains Clear` | Clear | 🟢 |
| 190 | `measure-distance ~ 0.25` | 0.25 | 🟢 |
| 200 | `focused-step == Joint#13` | Joint#13 | 🟢 |
| 206 | `refused == 0` | 0 | 🟢 |
| 207 | `parameter-drag > 0` | 7 | 🟢 |
| 208 | `bounds:Block2 == (2.800000, 0.250000, 0.900000) … (3.200000, 2.250000, 1.100000)` | (2.800000, 0.250000, 0.900000) … (3.200000, 2.250000, 1.100000) | 🟢 |
| 220 | `measure-interference contains Overlap` | Overlap | 🟢 |
| 221 | `measure-overlap ~ 0.05792063` | 0.05792063028 | 🟢 |
| 222 | `measure-overlap-edges > 3` | 24 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 179AssemblyParts | 175 | `724764ad6123d455` |
| 180JointHover | 186 | `603f94996cb3627d` |
| 181JointPreview | 207 | `17666897ce65ed96` |
| 182JointFlipped | 230 | `f7d20c83d5951563` |
| 183JointApplied | 238 | `e18fba1dd757b6d7` |
| 184JointLeverPreview | 301 | `f847fc58539c5613` |
| 185JointLever | 309 | `1394e673cacc9669` |
| 186JointStepDrag | 325 | `95680497c4dfdeff` |
| 187InterferenceClear | 359 | `a3c215bb13aedafd` |
| 188JointStepOffset | 392 | `1aef9e450642bdfc` |
| 189InterferenceOverlap | 415 | `cba3187bd21bfc1a` |

## Journal

- Cylinder3 · New body · 212 ms
- Autosaved _ClaudeScratch/DraftingWorkspace/AssemblyRecovery.drafting.toml
- Block2 · New body · 192 ms
- Framed the document (Space)
- Joint: pick the joint origin on the moving body (an edge or a face)
- Joint: circle centre on Cylinder3 moves · now pick the joint origin on the body it joins
- Autosaved _ClaudeScratch/DraftingWorkspace/AssemblyRecovery.drafting.toml
- Joint: Cylinder3 onto Block1 · R V S C P L B kind · drag the values · F flip · Enter applies
- Joint: flipped (origins aligned)
- Revolute joint (Cylinder3 → Block1) · 191 ms
- Joint: pick the joint origin on the moving body (an edge or a face)
- Joint: circle centre on Cylinder3 moves · now pick the joint origin on the body it joins
- Joint: pick the second joint origin on another body
- Joint cancelled
- Selected Extrude#12.Cap(End)
- Autosaved _ClaudeScratch/DraftingWorkspace/AssemblyRecovery.drafting.toml
- Added Extrude#10.Cap(End)
- Joint: Block2 onto Cylinder3 · R V S C P L B kind · drag the values · F flip · Enter applies
- Revolute joint (Block2 → Cylinder3) · 247 ms
- Revolute2.Angle = 120 · 298 ms
- Undo
- Selection cleared
- Selected Block2 (6 faces)
- Added Block1 · 2 bodies picked
- Measure · Block2 + Block1 · Volume 10.3828 m³
- Measure off
- Autosaved _ClaudeScratch/DraftingWorkspace/AssemblyRecovery.drafting.toml
- Selection cleared
- Revolute1.Offset = -0.1 · 240 ms
- Selected Block2 (6 faces)
- Added Block1 · 2 bodies picked
- Measure · Block2 + Block1 · Volume 10.3828 m³
