# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0
- Frame: 1600 x 900 · 494 frames · 41.4417 s · slowest frame 506.868 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/WorkspaceWalkthrough.txt
- Expectations: 78 / 78 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 8 | `faces == 11` | 11 | 🟢 |
| 9 | `steps == 8` | 8 | 🟢 |
| 10 | `refused == 0` | 0 | 🟢 |
| 11 | `sound == 1` | 1 | 🟢 |
| 16 | `hover contains Cap(End)` | Extrude#2.Cap(End) | 🟢 |
| 18 | `selected-faces contains Extrude#2.Cap(End)` | Extrude#2.Cap(End) | 🟢 |
| 22 | `selected-faces == 2` | Extrude#2.Cap(End),Extrude#4.Cap(End) | 🟢 |
| 25 | `selected-faces == 1` | Extrude#2.Cap(End) | 🟢 |
| 27 | `selected-faces == 0` |  | 🟢 |
| 31 | `pick-mode == 2` | 2 | 🟢 |
| 35 | `selected-edges == 1` | Edge(Extrude#2.Cap(Start)/Extrude#2.Side(Line1)) | 🟢 |
| 36 | `selected-edges contains Side(Line1)` | Edge(Extrude#2.Cap(Start)/Extrude#2.Side(Line1)) | 🟢 |
| 41 | `bevel-active == 1` | 1 | 🟢 |
| 42 | `bevel-chamfer == 0` | 0 | 🟢 |
| 44 | `preview-faces == 12` | 12 | 🟢 |
| 45 | `faces == 11` | 11 | 🟢 |
| 50 | `bevel-size > 0.25` | 0.3968 | 🟢 |
| 56 | `bevel-size ~ 0.3` | 0.3 | 🟢 |
| 57 | `bevel-pinned == 1` | 1 | 🟢 |
| 58 | `preview-faces == 12` | 12 | 🟢 |
| 59 | `faces == 11` | 11 | 🟢 |
| 60 | `undo == 5` | 5 | 🟢 |
| 64 | `bevel-size ~ 0.3` | 0.3 | 🟢 |
| 67 | `bevel-active == 0` | 0 | 🟢 |
| 68 | `faces == 12` | 12 | 🟢 |
| 69 | `steps == 9` | 9 | 🟢 |
| 70 | `undo == 6` | 6 | 🟢 |
| 71 | `sound == 1` | 1 | 🟢 |
| 78 | `selected-edges == 1` | Edge(Extrude#2.Cap(End)/Extrude#2.Side(Line1)) | 🟢 |
| 83 | `bevel-active == 1` | 1 | 🟢 |
| 84 | `bevel-chamfer == 1` | 1 | 🟢 |
| 85 | `bevel-refusal contains consumed by the blend` | Blend#10: Blend: edge E98: consumed by the blend (blend wider than the edge) (Circle, o... | 🟢 |
| 89 | `bevel-active == 0` | 0 | 🟢 |
| 90 | `faces == 12` | 12 | 🟢 |
| 91 | `steps == 9` | 9 | 🟢 |
| 95 | `faces == 11` | 11 | 🟢 |
| 96 | `redo == 1` | 1 | 🟢 |
| 98 | `faces == 12` | 12 | 🟢 |
| 100 | `faces == 11` | 11 | 🟢 |
| 102 | `faces == 12` | 12 | 🟢 |
| 106 | `camera-yaw ~ -0.7853981634` | -0.7853981634 | 🟢 |
| 108 | `camera-yaw ~ -1.7453981634` | -1.745398163 | 🟢 |
| 109 | `camera-pitch ~ 0.6154797087` | 0.6154797087 | 🟢 |
| 111 | `camera-pitch ~ 0.2154797087` | 0.2154797087 | 🟢 |
| 113 | `camera-yaw ~ -1.7453981634` | -1.745398163 | 🟢 |
| 114 | `camera-target-x != 2` | 1.42168656 | 🟢 |
| 116 | `camera-target-z != 0.875` | 1.257440933 | 🟢 |
| 118 | `camera-distance < 7.9` | 6.232551794 | 🟢 |
| 123 | `camera-pitch ~ 1.5707963268` | 1.570796327 | 🟢 |
| 124 | `grid-plane == 0` | 0 | 🟢 |
| 127 | `orthographic == 1` | 1 | 🟢 |
| 131 | `camera-pitch ~ 0` | 0 | 🟢 |
| 132 | `camera-yaw ~ -1.5707963268` | -1.570796327 | 🟢 |
| 133 | `grid-plane == 1` | 1 | 🟢 |
| 138 | `grid-plane == 2` | 2 | 🟢 |
| 143 | `grid-plane == 2` | 2 | 🟢 |
| 151 | `grid == 0` | 0 | 🟢 |
| 159 | `camera-pitch ~ 0.6154797087` | 0.6154797087 | 🟢 |
| 160 | `grid-plane == 0` | 0 | 🟢 |
| 167 | `selected-faces == 1` | Extrude#4.Cap(End) | 🟢 |
| 168 | `selected-faces contains Extrude#4.Cap(End)` | Extrude#4.Cap(End) | 🟢 |
| 171 | `steps == 10` | 10 | 🟢 |
| 172 | `sound == 1` | 1 | 🟢 |
| 173 | `status contains Push` | Push 0.25 on 1 face(s) · 107 ms | 🟢 |
| 177 | `focused-step == Extrude#2` | Extrude#2 | 🟢 |
| 183 | `status contains Block1.Height` | Block1.Height = 1.25 · 364 ms | 🟢 |
| 184 | `refused == 0` | 0 | 🟢 |
| 185 | `sound == 1` | 1 | 🟢 |
| 216 | `steps == 12` | 12 | 🟢 |
| 217 | `refused == 0` | 0 | 🟢 |
| 218 | `sound == 1` | 1 | 🟢 |
| 219 | `genus == 2` | 2 | 🟢 |
| 224 | `dirty == 0` | 0 | 🟢 |
| 225 | `file:_ClaudeScratch/SolidArcWorkspace/Bracket.solidarc == 1` | 1 | 🟢 |
| 227 | `file:_ClaudeScratch/SolidArcWorkspace/Recovery.solidarc == 1` | 1 | 🟢 |
| 229 | `steps == 0` | 0 | 🟢 |
| 231 | `file:_ClaudeScratch/SolidArcWorkspace/Recovery.previous.solidarc == 1` | 1 | 🟢 |
| 234 | `steps > 9` | 12 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 01Workspace | 4 | `6d6d1701e80c02f3` |
| 02FacesSelected | 24 | `bcbc8ee4420118c2` |
| 03EdgeSelected | 46 | `876f6d6971fb0fd4` |
| 04BevelPreview | 75 | `868991e4678bfaa3` |
| 05FilletApplied | 86 | `903dfb63325b533e` |
| 06BevelRefusal | 107 | `81d052f0815796f9` |
| 07Navigated | 183 | `6d7ad8c95c6cee0b` |
| 08ViewCubeTop | 190 | `daf00225f071227c` |
| 09FrontParallel | 209 | `96d4035c11fb3ea2` |
| 09RightParallel | 223 | `f4d7348d3fe90864` |
| 10FaceEdit | 308 | `a63efcd1a8768cb6` |
| 11ParameterEdit | 338 | `421a041c862109b0` |
| 12Construct | 421 | `027a1a0e26348fec` |
| 13Constructed | 430 | `742663198f43063c` |
| 14NewDocument | 485 | `3bb798b348654517` |
| 15Restored | 494 | `0f569e1ec9a58e4d` |

## Journal

- Selected Extrude#2.Cap(End)
- Added Extrude#4.Cap(End)
- Removed Extrude#4.Cap(End)
- Selection cleared
- Selected Edge(Extrude#2.Cap(Start)|Extrude#2.Side(Line1))
- Fillet (B): move horizontally · D fillet · C chamfer · V variable size · F flip its anchor · click edits the edges · Enter / right-click applies · Esc aborts
- Bevel size 0.3 m (pinned)
- Fillet 0.3 on 1 edge(s) · 116 ms
- Selected Edge(Extrude#2.Cap(End)|Extrude#2.Side(Line1))
- Fillet (B): move horizontally · D fillet · C chamfer · V variable size · F flip its anchor · click edits the edges · Enter / right-click applies · Esc aborts
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
- Push 0.25 on 1 face(s) · 107 ms
- Block1.Height = 1.25 · 364 ms
- Cylinder: click the base centre · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Autosaved _ClaudeScratch/SolidArcWorkspace/Recovery.solidarc
- Cylinder3 · Difference · 112 ms
- Saved _ClaudeScratch/SolidArcWorkspace/Bracket.solidarc
- Autosaved _ClaudeScratch/SolidArcWorkspace/Recovery.solidarc
- New document (previous recovery copy kept)
- Opened _ClaudeScratch/SolidArcWorkspace/Recovery.previous.solidarc · 12 steps
