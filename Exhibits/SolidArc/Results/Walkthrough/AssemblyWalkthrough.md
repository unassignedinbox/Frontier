# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 900 · 442 frames · 63.1412 s · slowest frame 3975.96 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/AssemblyWalkthrough.txt
- Expectations: 56 / 56 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 13 | `bodies == 1` | 1 | 🟢 |
| 40 | `refused == 0` | 0 | 🟢 |
| 70 | `refused == 0` | 0 | 🟢 |
| 71 | `bodies == 3` | 3 | 🟢 |
| 72 | `body-captions contains Cylinder3` | Block1,Cylinder3,Block2 | 🟢 |
| 73 | `body-captions contains Block2` | Block1,Cylinder3,Block2 | 🟢 |
| 74 | `centroid:Cylinder3 == (6.000000, 1.250000, 0.750000)` | (6.000000, 1.250000, 0.750000) | 🟢 |
| 82 | `joint == 1` | 1 | 🟢 |
| 83 | `joint-phase == 0` | 0 | 🟢 |
| 84 | `joint-kind == Revolute` | Revolute | 🟢 |
| 87 | `joint-hover contains circle centre on Cylinder3` | circle centre on Cylinder3 | 🟢 |
| 94 | `joint-phase == 1` | 1 | 🟢 |
| 97 | `joint-hover contains circle centre on Block1` | circle centre on Block1 | 🟢 |
| 100 | `joint-phase == 2` | 2 | 🟢 |
| 101 | `joint-preview == 1` | 1 | 🟢 |
| 102 | `joint-refusal == 0` |  | 🟢 |
| 103 | `joint-picks contains circle centre on Cylinder3 -> circle centre on Block1` | circle centre on Cylinder3 -> circle centre on Block1 | 🟢 |
| 104 | `preview-resumed > 0` | 12 | 🟢 |
| 110 | `joint-flipped == 1` | 1 | 🟢 |
| 116 | `joint-refusal == 0` |  | 🟢 |
| 122 | `joint == 0` | 0 | 🟢 |
| 123 | `joint-commits == 1` | 1 | 🟢 |
| 124 | `refused == 0` | 0 | 🟢 |
| 125 | `joints == 1` | 1 | 🟢 |
| 126 | `joint-gap ~ 0.25` | 0.25 | 🟢 |
| 127 | `centroid:Cylinder3 == (3.000000, 1.250000, 0.500000)` | (3.000000, 1.250000, 0.500000) | 🟢 |
| 136 | `joint-phase == 1` | 1 | 🟢 |
| 139 | `joint-phase == 1` | 1 | 🟢 |
| 140 | `status contains another body` | Joint: pick the second joint origin on another body | 🟢 |
| 143 | `joint == 0` | 0 | 🟢 |
| 151 | `selected-faces == 2` | Extrude#12.Cap(End),Extrude#10.Cap(End) | 🟢 |
| 154 | `joint-phase == 2` | 2 | 🟢 |
| 155 | `joint-kind == Revolute` | Revolute | 🟢 |
| 156 | `joint-picks contains plane centroid on Block2 -> plane centroid on Cylinder3` | plane centroid on Block2 -> plane centroid on Cylinder3 | 🟢 |
| 165 | `joint-commits == 2` | 2 | 🟢 |
| 166 | `joints == 2` | 2 | 🟢 |
| 167 | `joint-gap ~ 0.25` | 0.25 | 🟢 |
| 168 | `bounds:Block2 == (2.800000, 0.250000, 1.250000) … (3.200000, 2.250000, 1.450000)` | (2.800000, 0.250000, 1.250000) … (3.200000, 2.250000, 1.450000) | 🟢 |
| 169 | `focused-step == Joint#14` | Joint#14 | 🟢 |
| 174 | `undo == 9` | 9 | 🟢 |
| 177 | `refused == 0` | 0 | 🟢 |
| 178 | `parameter-drag > 2` | 6 | 🟢 |
| 179 | `undo == 10` | 10 | 🟢 |
| 183 | `undo == 9` | 9 | 🟢 |
| 184 | `bounds:Block2 == (2.800000, 0.250000, 1.250000) … (3.200000, 2.250000, 1.450000)` | (2.800000, 0.250000, 1.250000) … (3.200000, 2.250000, 1.450000) | 🟢 |
| 193 | `selected-bodies == 2` | Extrude#12,Extrude#2 | 🟢 |
| 196 | `measure contains Block2 + Block1` | Block2 + Block1 | 🟢 |
| 197 | `measure-interference contains Clear` | Clear | 🟢 |
| 198 | `measure-distance ~ 0.25` | 0.25 | 🟢 |
| 208 | `focused-step == Joint#13` | Joint#13 | 🟢 |
| 214 | `refused == 0` | 0 | 🟢 |
| 215 | `parameter-drag > 0` | 7 | 🟢 |
| 216 | `bounds:Block2 == (2.800000, 0.250000, 0.900000) … (3.200000, 2.250000, 1.100000)` | (2.800000, 0.250000, 0.900000) … (3.200000, 2.250000, 1.100000) | 🟢 |
| 228 | `measure-interference contains Overlap` | Overlap | 🟢 |
| 229 | `measure-overlap ~ 0.05792063` | 0.05792063028 | 🟢 |
| 230 | `measure-overlap-edges > 3` | 24 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 179AssemblyParts | 199 | `d783c7ce2aeec588` |
| 180JointHover | 210 | `fd9c8fe1a4acfd49` |
| 181JointPreview | 231 | `eede09f64ec2bead` |
| 182JointFlipped | 254 | `cb4db38b99988053` |
| 183JointApplied | 262 | `fa1ce1030a24450d` |
| 184JointLeverPreview | 325 | `7fdc35e107b17419` |
| 185JointLever | 333 | `1e6a838a5893c407` |
| 186JointStepDrag | 352 | `75714abd89557271` |
| 187InterferenceClear | 386 | `597ef42df4089ebe` |
| 188JointStepOffset | 419 | `c8cedb3cb158f414` |
| 189InterferenceOverlap | 442 | `420d574c42376621` |

## Journal

- Cylinder: click the base centre · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Cylinder3 · New body · 87 ms
- Block: click the first corner · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Autosaved _ClaudeScratch/SolidArcWorkspace/AssemblyRecovery.solidarc
- Block2 · New body · 83 ms
- Framed the document (Space)
- Joint: pick the joint origin on the moving body (an edge or a face)
- Joint: circle centre on Cylinder3 moves · now pick the joint origin on the body it joins
- Autosaved _ClaudeScratch/SolidArcWorkspace/AssemblyRecovery.solidarc
- Joint: Cylinder3 onto Block1 · R V S C P L B kind · drag the values · F flip · Enter / right-click applies
- Joint: flipped (origins aligned)
- Revolute joint (Cylinder3 → Block1) · 81 ms
- Joint: pick the joint origin on the moving body (an edge or a face)
- Joint: circle centre on Cylinder3 moves · now pick the joint origin on the body it joins
- Joint: pick the second joint origin on another body
- Joint cancelled
- Selected Extrude#12.Cap(End)
- Autosaved _ClaudeScratch/SolidArcWorkspace/AssemblyRecovery.solidarc
- Added Extrude#10.Cap(End)
- Joint: Block2 onto Cylinder3 · R V S C P L B kind · drag the values · F flip · Enter / right-click applies
- Revolute joint (Block2 → Cylinder3) · 94 ms
- Revolute2.Angle = 120 · 110 ms
- Undo
- Selection cleared
- Selected Block2 (6 faces)
- Added Block1 · 2 bodies picked
- Measure · Block2 + Block1 · Volume 10.3828 m³
- Measure off
- Autosaved _ClaudeScratch/SolidArcWorkspace/AssemblyRecovery.solidarc
- Selection cleared
- Revolute1.Offset = -0.1 · 104 ms
- Selected Block2 (6 faces)
- Added Block1 · 2 bodies picked
- Measure · Block2 + Block1 · Volume 10.3828 m³
