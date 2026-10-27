# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 900 · 558 frames · 82.7784 s · slowest frame 599.823 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/WorkspaceWalkthrough.txt
- Expectations: 84 / 84 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 11 | `faces == 11` | 11 | 🟢 |
| 12 | `steps == 8` | 8 | 🟢 |
| 13 | `refused == 0` | 0 | 🟢 |
| 14 | `sound == 1` | 1 | 🟢 |
| 19 | `hover contains Cap(End)` | Extrude#2.Cap(End) | 🟢 |
| 21 | `selected-faces contains Extrude#2.Cap(End)` | Extrude#2.Cap(End) | 🟢 |
| 25 | `selected-faces == 2` | Extrude#2.Cap(End),Extrude#4.Cap(End) | 🟢 |
| 28 | `selected-faces == 1` | Extrude#2.Cap(End) | 🟢 |
| 30 | `selected-faces == 0` |  | 🟢 |
| 34 | `pick-mode == 2` | 2 | 🟢 |
| 38 | `selected-edges == 1` | Edge(Extrude#2.Cap(Start)/Extrude#2.Side(Line1)) | 🟢 |
| 39 | `selected-edges contains Side(Line1)` | Edge(Extrude#2.Cap(Start)/Extrude#2.Side(Line1)) | 🟢 |
| 44 | `bevel-active == 1` | 1 | 🟢 |
| 45 | `bevel-chamfer == 0` | 0 | 🟢 |
| 47 | `preview-faces == 12` | 12 | 🟢 |
| 48 | `faces == 11` | 11 | 🟢 |
| 49 | `bevel-pinned == 1` | 1 | 🟢 |
| 56 | `bevel-size > 0.05` | 0.121 | 🟢 |
| 62 | `bevel-size ~ 0.3` | 0.3 | 🟢 |
| 63 | `bevel-pinned == 1` | 1 | 🟢 |
| 64 | `preview-faces == 12` | 12 | 🟢 |
| 65 | `faces == 11` | 11 | 🟢 |
| 66 | `undo == 5` | 5 | 🟢 |
| 70 | `bevel-size ~ 0.3` | 0.3 | 🟢 |
| 73 | `bevel-active == 0` | 0 | 🟢 |
| 74 | `faces == 12` | 12 | 🟢 |
| 75 | `steps == 9` | 9 | 🟢 |
| 76 | `undo == 6` | 6 | 🟢 |
| 77 | `sound == 1` | 1 | 🟢 |
| 86 | `selected-edges == 1` | Edge(Extrude#2.Cap(End)/Extrude#2.Side(Line1)) | 🟢 |
| 91 | `bevel-active == 1` | 1 | 🟢 |
| 92 | `bevel-chamfer == 1` | 1 | 🟢 |
| 93 | `bevel-size ~ 0.02` | 0.02 | 🟢 |
| 94 | `bevel-refusal == 0` |  | 🟢 |
| 101 | `bevel-refusal == 0` |  | 🟢 |
| 102 | `bevel-size < 0.3` | 0.199998 | 🟢 |
| 103 | `bevel-size > 0.02` | 0.199998 | 🟢 |
| 104 | `bevel-limit-stops > 0` | 2 | 🟢 |
| 108 | `bevel-active == 0` | 0 | 🟢 |
| 109 | `faces == 12` | 12 | 🟢 |
| 110 | `steps == 9` | 9 | 🟢 |
| 114 | `faces == 11` | 11 | 🟢 |
| 115 | `redo == 1` | 1 | 🟢 |
| 117 | `faces == 12` | 12 | 🟢 |
| 119 | `faces == 11` | 11 | 🟢 |
| 121 | `faces == 12` | 12 | 🟢 |
| 125 | `camera-yaw ~ -0.7853981634` | -0.7853981634 | 🟢 |
| 127 | `camera-yaw ~ -1.7453981634` | -1.745398163 | 🟢 |
| 128 | `camera-pitch ~ 0.6154797087` | 0.6154797087 | 🟢 |
| 130 | `camera-pitch ~ 0.2154797087` | 0.2154797087 | 🟢 |
| 132 | `camera-yaw ~ -1.7453981634` | -1.745398163 | 🟢 |
| 133 | `camera-target-x != 2` | 1.527270756 | 🟢 |
| 135 | `camera-target-z != 0.875` | 1.187617692 | 🟢 |
| 137 | `camera-distance < 7.9` | 6.232551794 | 🟢 |
| 142 | `camera-pitch ~ 1.5707963268` | 1.570796327 | 🟢 |
| 143 | `grid-plane == 0` | 0 | 🟢 |
| 146 | `orthographic == 1` | 1 | 🟢 |
| 150 | `camera-pitch ~ 0` | 0 | 🟢 |
| 151 | `camera-yaw ~ -1.5707963268` | -1.570796327 | 🟢 |
| 152 | `grid-plane == 1` | 1 | 🟢 |
| 157 | `grid-plane == 2` | 2 | 🟢 |
| 162 | `grid-plane == 2` | 2 | 🟢 |
| 170 | `grid == 0` | 0 | 🟢 |
| 178 | `camera-pitch ~ 0.6154797087` | 0.6154797087 | 🟢 |
| 179 | `grid-plane == 0` | 0 | 🟢 |
| 186 | `selected-faces == 1` | Extrude#4.Cap(End) | 🟢 |
| 187 | `selected-faces contains Extrude#4.Cap(End)` | Extrude#4.Cap(End) | 🟢 |
| 190 | `steps == 10` | 10 | 🟢 |
| 191 | `sound == 1` | 1 | 🟢 |
| 192 | `status contains Push` | Push 0.25 on 1 face(s) · 3 ms | 🟢 |
| 201 | `focused-step == Extrude#2` | Extrude#2 | 🟢 |
| 207 | `status contains Block1.Height` | Block1.Height = 1.25 · 16 ms | 🟢 |
| 208 | `refused == 0` | 0 | 🟢 |
| 209 | `sound == 1` | 1 | 🟢 |
| 240 | `steps == 12` | 12 | 🟢 |
| 241 | `refused == 0` | 0 | 🟢 |
| 242 | `sound == 1` | 1 | 🟢 |
| 243 | `genus == 2` | 2 | 🟢 |
| 248 | `dirty == 0` | 0 | 🟢 |
| 249 | `file:_ClaudeScratch/SolidArcWorkspace/Bracket.solidarc == 1` | 1 | 🟢 |
| 251 | `file:_ClaudeScratch/SolidArcWorkspace/Recovery.solidarc == 1` | 1 | 🟢 |
| 253 | `steps == 0` | 0 | 🟢 |
| 255 | `file:_ClaudeScratch/SolidArcWorkspace/Recovery.previous.solidarc == 1` | 1 | 🟢 |
| 258 | `steps > 9` | 12 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 01Workspace | 5 | `60c46ffca2603b31` |
| 02FacesSelected | 25 | `c711472e66136638` |
| 03EdgeSelected | 47 | `d496806838a7f4d8` |
| 04BevelPreview | 82 | `b57efd5ee1095595` |
| 05FilletApplied | 93 | `37e4216486566daa` |
| 06BevelRefusal | 153 | `93af0882b9461012` |
| 07Navigated | 229 | `6610595ff5f3e7e1` |
| 08ViewCubeTop | 236 | `f585e516f2e63105` |
| 09FrontParallel | 255 | `6abbc51bd3b0a98a` |
| 09RightParallel | 269 | `6cf55c284bbe7229` |
| 10FaceEdit | 354 | `b9a056b599d7e505` |
| 11ParameterEdit | 402 | `01615f81bb177796` |
| 12Construct | 485 | `6f73e741cfe2dfba` |
| 13Constructed | 494 | `b73055f1fa64d5e6` |
| 14NewDocument | 549 | `4859007518eefb4c` |
| 15Restored | 558 | `ef5e661f70bcc74d` |

## Journal

- Selected Extrude#2.Cap(End)
- Added Extrude#4.Cap(End)
- Removed Extrude#4.Cap(End)
- Selection cleared
- Selected Edge(Extrude#2.Cap(Start)|Extrude#2.Side(Line1))
- Fillet (B): drag anywhere or the ring handle to size (it stops at the largest that fits) · drag a corner diamond for a variable size · type in the card · D fillet · C chamfer · V taper · Alt+click an edge loop, Ctrl+Alt+click a ring (Shift adds) · Enter / right-click applies · Esc aborts
- Bevel size 0.3 m (pinned)
- Fillet 0.3 on 1 edge(s) · 3 ms
- Selected Edge(Extrude#2.Cap(End)|Extrude#2.Side(Line1))
- Fillet (B): drag anywhere or the ring handle to size (it stops at the largest that fits) · drag a corner diamond for a variable size · type in the card · D fillet · C chamfer · V taper · Alt+click an edge loop, Ctrl+Alt+click a ring (Shift adds) · Enter / right-click applies · Esc aborts · starts at 0.02 m to fit the edges
- Bevel size 0.3 m is past the largest these edges take: held at 0.2826 m
- Bevel: 0.2 m is the largest size these edges take · it stops there
- Bevel cancelled (document untouched)
- Undo
- Redo
- Undo
- Redo
- Selection cleared
- View Top (orientation cube)
- Autosaved _ClaudeScratch/SolidArcWorkspace/Recovery.solidarc
- View Front
- View Right
- View Isometric
- View Top
- View Isometric
- Selected Extrude#4.Cap(End)
- Push 0.25 on 1 face(s) · 3 ms
- Block1.Height = 1.25 · 16 ms
- Cylinder: click the base centre · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Autosaved _ClaudeScratch/SolidArcWorkspace/Recovery.solidarc
- Cylinder3 · Difference · 12 ms
- Saved _ClaudeScratch/SolidArcWorkspace/Bracket.solidarc
- Autosaved _ClaudeScratch/SolidArcWorkspace/Recovery.solidarc
- New document (previous recovery copy kept)
- Opened _ClaudeScratch/SolidArcWorkspace/Recovery.previous.solidarc · 12 steps
