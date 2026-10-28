# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 810 · 467 frames · 44.4279 s · slowest frame 2954.35 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/AssemblyWalkthrough.txt
- Expectations: 56 / 56 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 16 | `bodies == 1` | 1 | 🟢 |
| 43 | `refused == 0` | 0 | 🟢 |
| 73 | `refused == 0` | 0 | 🟢 |
| 74 | `bodies == 3` | 3 | 🟢 |
| 75 | `body-captions contains Cylinder3` | Block1,Cylinder3,Block2 | 🟢 |
| 76 | `body-captions contains Block2` | Block1,Cylinder3,Block2 | 🟢 |
| 77 | `centroid:Cylinder3 == (6.000000, 1.250000, 0.750000)` | (6.000000, 1.250000, 0.750000) | 🟢 |
| 85 | `joint == 1` | 1 | 🟢 |
| 86 | `joint-phase == 0` | 0 | 🟢 |
| 87 | `joint-kind == Revolute` | Revolute | 🟢 |
| 90 | `joint-hover contains circle centre on Cylinder3` | circle centre on Cylinder3 | 🟢 |
| 97 | `joint-phase == 1` | 1 | 🟢 |
| 100 | `joint-hover contains circle centre on Block1` | circle centre on Block1 | 🟢 |
| 103 | `joint-phase == 2` | 2 | 🟢 |
| 104 | `joint-preview == 1` | 1 | 🟢 |
| 105 | `joint-refusal == 0` |  | 🟢 |
| 106 | `joint-picks contains circle centre on Cylinder3 -> circle centre on Block1` | circle centre on Cylinder3 -> circle centre on Block1 | 🟢 |
| 107 | `preview-resumed > 0` | 12 | 🟢 |
| 113 | `joint-flipped == 1` | 1 | 🟢 |
| 119 | `joint-refusal == 0` |  | 🟢 |
| 125 | `joint == 0` | 0 | 🟢 |
| 126 | `joint-commits == 1` | 1 | 🟢 |
| 127 | `refused == 0` | 0 | 🟢 |
| 128 | `joints == 1` | 1 | 🟢 |
| 129 | `joint-gap ~ 0.25` | 0.25 | 🟢 |
| 130 | `centroid:Cylinder3 == (3.000000, 1.250000, 0.500000)` | (3.000000, 1.250000, 0.500000) | 🟢 |
| 139 | `joint-phase == 1` | 1 | 🟢 |
| 142 | `joint-phase == 1` | 1 | 🟢 |
| 143 | `status contains another body` | Joint: pick the second joint origin on another body | 🟢 |
| 146 | `joint == 0` | 0 | 🟢 |
| 154 | `selected-faces == 2` | Extrude#12.Cap(End),Extrude#10.Cap(End) | 🟢 |
| 157 | `joint-phase == 2` | 2 | 🟢 |
| 158 | `joint-kind == Revolute` | Revolute | 🟢 |
| 159 | `joint-picks contains plane centroid on Block2 -> plane centroid on Cylinder3` | plane centroid on Block2 -> plane centroid on Cylinder3 | 🟢 |
| 168 | `joint-commits == 2` | 2 | 🟢 |
| 169 | `joints == 2` | 2 | 🟢 |
| 170 | `joint-gap ~ 0.25` | 0.25 | 🟢 |
| 171 | `bounds:Block2 == (2.800000, 0.250000, 1.250000) … (3.200000, 2.250000, 1.450000)` | (2.800000, 0.250000, 1.250000) … (3.200000, 2.250000, 1.450000) | 🟢 |
| 172 | `focused-step == Joint#14` | Joint#14 | 🟢 |
| 177 | `undo == 9` | 9 | 🟢 |
| 180 | `refused == 0` | 0 | 🟢 |
| 181 | `parameter-drag > 2` | 6 | 🟢 |
| 182 | `undo == 10` | 10 | 🟢 |
| 186 | `undo == 9` | 9 | 🟢 |
| 187 | `bounds:Block2 == (2.800000, 0.250000, 1.250000) … (3.200000, 2.250000, 1.450000)` | (2.800000, 0.250000, 1.250000) … (3.200000, 2.250000, 1.450000) | 🟢 |
| 196 | `selected-bodies == 2` | Extrude#12,Extrude#2 | 🟢 |
| 199 | `measure contains Block2 + Block1` | Block2 + Block1 | 🟢 |
| 200 | `measure-interference contains Clear` | Clear | 🟢 |
| 201 | `measure-distance ~ 0.25` | 0.25 | 🟢 |
| 216 | `focused-step == Joint#13` | Joint#13 | 🟢 |
| 222 | `refused == 0` | 0 | 🟢 |
| 223 | `parameter-drag > 0` | 7 | 🟢 |
| 224 | `bounds:Block2 == (2.800000, 0.250000, 0.900000) … (3.200000, 2.250000, 1.100000)` | (2.800000, 0.250000, 0.900000) … (3.200000, 2.250000, 1.100000) | 🟢 |
| 236 | `measure-interference contains Overlap` | Overlap | 🟢 |
| 237 | `measure-overlap ~ 0.05792063` | 0.05792063028 | 🟢 |
| 238 | `measure-overlap-edges > 3` | 24 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 179AssemblyParts | 200 | `0b140a33a7ec8547` |
| 180JointHover | 211 | `077af5cc5323912b` |
| 181JointPreview | 232 | `d4c7f8b9438afaf6` |
| 182JointFlipped | 255 | `89a0a10baa64f30e` |
| 183JointApplied | 263 | `9dd5d1f1b84dca60` |
| 184JointLeverPreview | 326 | `a43a03b1534698c4` |
| 185JointLever | 334 | `07341bf09eab620d` |
| 186JointStepDrag | 353 | `d8b5626466b761c6` |
| 187InterferenceClear | 387 | `209ed56f32883c3d` |
| 188JointStepOffset | 444 | `897064ead6a4abb3` |
| 189InterferenceOverlap | 467 | `35150f2627f722ee` |

## Journal

- Cylinder: click the base centre · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Cylinder3 · New body · 1 ms
- Block: click the first corner · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Autosaved _ClaudeScratch/SolidArcWorkspace/AssemblyRecovery.solidarc
- Block2 · New body · 1 ms
- Framed the document (Space)
- Joint: pick the joint origin on the moving body (an edge or a face)
- Joint: circle centre on Cylinder3 moves · now pick the joint origin on the body it joins
- Autosaved _ClaudeScratch/SolidArcWorkspace/AssemblyRecovery.solidarc
- Joint: Cylinder3 onto Block1 · R V S C P L B kind · drag the values · F flip · Enter / right-click applies
- Joint: flipped (origins aligned)
- Revolute joint (Cylinder3 → Block1) · 1 ms
- Joint: pick the joint origin on the moving body (an edge or a face)
- Joint: circle centre on Cylinder3 moves · now pick the joint origin on the body it joins
- Joint: pick the second joint origin on another body
- Joint cancelled
- Selected Extrude#12.Cap(End)
- Autosaved _ClaudeScratch/SolidArcWorkspace/AssemblyRecovery.solidarc
- Added Extrude#10.Cap(End)
- Joint: Block2 onto Cylinder3 · R V S C P L B kind · drag the values · F flip · Enter / right-click applies
- Revolute joint (Block2 → Cylinder3) · 17 ms
- Revolute2.Angle = 120 · 17 ms
- Undo
- Selection cleared
- Selected Block2 (6 faces)
- Added Block1 · 2 bodies picked
- Measure · Block2 + Block1 · Volume 10.3828 m³
- Measure off
- Autosaved _ClaudeScratch/SolidArcWorkspace/AssemblyRecovery.solidarc
- Selection cleared
- Revolute1.Offset = -0.1 · 17 ms
- Selected Block2 (6 faces)
- Added Block1 · 2 bodies picked
- Measure · Block2 + Block1 · Volume 10.3828 m³
