# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0
- Frame: 1600 x 900 · 429 frames · 72.0123 s · slowest frame 719.71 ms
- Script: /home/user/Frontier/Exhibits/SolidArc/Code/Scripts/WorkspaceWalkthrough.txt
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
| 50 | `bevel-size > 0.25` | 0.4561 | 🟢 |
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
| 114 | `camera-target-x != 2` | 1.531803777 | 🟢 |
| 116 | `camera-target-z != 0.875` | 1.184619988 | 🟢 |
| 118 | `camera-distance < 7.9` | 6.232551794 | 🟢 |
| 123 | `camera-pitch ~ 1.5707963268` | 1.570796327 | 🟢 |
| 124 | `grid-plane == 0` | 0 | 🟢 |
| 127 | `orthographic == 1` | 1 | 🟢 |
| 129 | `camera-pitch ~ 0` | 0 | 🟢 |
| 130 | `camera-yaw ~ -1.5707963268` | -1.570796327 | 🟢 |
| 131 | `grid-plane == 1` | 1 | 🟢 |
| 134 | `grid-plane == 2` | 2 | 🟢 |
| 137 | `grid-plane == 2` | 2 | 🟢 |
| 141 | `grid == 0` | 0 | 🟢 |
| 145 | `camera-pitch ~ 0.6154797087` | 0.6154797087 | 🟢 |
| 146 | `grid-plane == 0` | 0 | 🟢 |
| 153 | `selected-faces == 1` | Extrude#4.Cap(End) | 🟢 |
| 154 | `selected-faces contains Extrude#4.Cap(End)` | Extrude#4.Cap(End) | 🟢 |
| 157 | `steps == 10` | 10 | 🟢 |
| 158 | `sound == 1` | 1 | 🟢 |
| 159 | `status contains Push` | Push 0.25 on 1 face(s) · 144 ms | 🟢 |
| 163 | `focused-step == Extrude#2` | Extrude#2 | 🟢 |
| 169 | `status contains Block1.Height` | Block1.Height = 1.25 · 527 ms | 🟢 |
| 170 | `refused == 0` | 0 | 🟢 |
| 171 | `sound == 1` | 1 | 🟢 |
| 198 | `steps == 12` | 12 | 🟢 |
| 199 | `refused == 0` | 0 | 🟢 |
| 200 | `sound == 1` | 1 | 🟢 |
| 201 | `genus == 2` | 2 | 🟢 |
| 206 | `dirty == 0` | 0 | 🟢 |
| 207 | `file:_ClaudeScratch/SolidArcWorkspace/Bracket.solidarc == 1` | 1 | 🟢 |
| 209 | `file:_ClaudeScratch/SolidArcWorkspace/Recovery.solidarc == 1` | 1 | 🟢 |
| 211 | `steps == 0` | 0 | 🟢 |
| 213 | `file:_ClaudeScratch/SolidArcWorkspace/Recovery.previous.solidarc == 1` | 1 | 🟢 |
| 216 | `steps > 9` | 12 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 01Workspace | 4 | `ee7986afe88daf6f` |
| 02FacesSelected | 24 | `25eb1ab7ef3d96bc` |
| 03EdgeSelected | 46 | `1dea6e6aa3eb541a` |
| 04BevelPreview | 75 | `c17036ee4840150e` |
| 05FilletApplied | 86 | `ca2060b737767766` |
| 06BevelRefusal | 107 | `dcd439c708c313b1` |
| 07Navigated | 183 | `64f0fb510b3e4f69` |
| 08ViewCubeTop | 190 | `572a6e724939f180` |
| 09FrontParallel | 202 | `4f3fdf61f95bb0a6` |
| 09RightParallel | 209 | `3c7ffe0f14be0c49` |
| 10FaceEdit | 258 | `75ee63ad0dac6646` |
| 11ParameterEdit | 285 | `7fb618315c3e9709` |
| 12Construct | 356 | `d56a6e175f57ac73` |
| 13Constructed | 365 | `215e0b1ad6f75691` |
| 14NewDocument | 420 | `7595bf3f0bd02941` |
| 15Restored | 429 | `45f46655939b7401` |

## Journal

- Selected Extrude#2.Cap(End)
- Added Extrude#4.Cap(End)
- Removed Extrude#4.Cap(End)
- Selection cleared
- Selected Edge(Extrude#2.Cap(Start)|Extrude#2.Side(Line1))
- Fillet (B): move horizontally · D fillet · C chamfer · V variable size · F flip its anchor · click edits the edges · Enter / right-click applies · Esc aborts
- Bevel size 0.3 m (pinned)
- Fillet 0.3 on 1 edge(s) · 136 ms
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
- Block1.Height = 1.25 · 527 ms
- Autosaved _ClaudeScratch/SolidArcWorkspace/Recovery.solidarc
- Cylinder3 · Difference · 131 ms
- Saved _ClaudeScratch/SolidArcWorkspace/Bracket.solidarc
- Autosaved _ClaudeScratch/SolidArcWorkspace/Recovery.solidarc
- New document (previous recovery copy kept)
- Opened _ClaudeScratch/SolidArcWorkspace/Recovery.previous.solidarc · 12 steps
