# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0
- Frame: 1600 x 900 · 494 frames · 50.4778 s · slowest frame 593.298 ms
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
| 173 | `status contains Push` | Push 0.25 on 1 face(s) · 144 ms | 🟢 |
| 177 | `focused-step == Extrude#2` | Extrude#2 | 🟢 |
| 183 | `status contains Block1.Height` | Block1.Height = 1.25 · 433 ms | 🟢 |
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
| 01Workspace | 4 | `2a9ef83654f4b054` |
| 02FacesSelected | 24 | `e50be00e6da98665` |
| 03EdgeSelected | 46 | `19af8d358b302c7a` |
| 04BevelPreview | 75 | `ad966170d66521c9` |
| 05FilletApplied | 86 | `ef6b7962ea78b3c8` |
| 06BevelRefusal | 107 | `57e2b70b2dddcb54` |
| 07Navigated | 183 | `64fbae2f3dbf33c0` |
| 08ViewCubeTop | 190 | `43f2ba07c353a86e` |
| 09FrontParallel | 209 | `1c19b32c1ad8b2cf` |
| 09RightParallel | 223 | `b2bd2a9a648240a3` |
| 10FaceEdit | 308 | `3cc59097f4a0089a` |
| 11ParameterEdit | 338 | `7326328f8057ad6a` |
| 12Construct | 421 | `bbf7d7ed4939ed49` |
| 13Constructed | 430 | `fbd31cc5c6559709` |
| 14NewDocument | 485 | `c00f15ce4cc60f85` |
| 15Restored | 494 | `cb64caf279667acc` |

## Journal

- Selected Extrude#2.Cap(End)
- Added Extrude#4.Cap(End)
- Removed Extrude#4.Cap(End)
- Selection cleared
- Selected Edge(Extrude#2.Cap(Start)|Extrude#2.Side(Line1))
- Fillet (B): move horizontally · D fillet · C chamfer · V variable size · F flip its anchor · click edits the edges · Enter / right-click applies · Esc aborts
- Bevel size 0.3 m (pinned)
- Fillet 0.3 on 1 edge(s) · 139 ms
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
- Push 0.25 on 1 face(s) · 144 ms
- Block1.Height = 1.25 · 433 ms
- Cylinder: click the base centre · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Autosaved _ClaudeScratch/SolidArcWorkspace/Recovery.solidarc
- Cylinder3 · Difference · 135 ms
- Saved _ClaudeScratch/SolidArcWorkspace/Bracket.solidarc
- Autosaved _ClaudeScratch/SolidArcWorkspace/Recovery.solidarc
- New document (previous recovery copy kept)
- Opened _ClaudeScratch/SolidArcWorkspace/Recovery.previous.solidarc · 12 steps
