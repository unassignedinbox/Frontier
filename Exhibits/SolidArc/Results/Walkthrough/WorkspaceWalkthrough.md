# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 900 · 509 frames · 98.5149 s · slowest frame 898.71 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/WorkspaceWalkthrough.txt
- Expectations: 81 / 81 passed

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
| 79 | `selected-edges == 1` | Edge(Extrude#2.Cap(End)/Extrude#2.Side(Line1)) | 🟢 |
| 84 | `bevel-active == 1` | 1 | 🟢 |
| 85 | `bevel-chamfer == 1` | 1 | 🟢 |
| 86 | `bevel-size ~ 0.02` | 0.02 | 🟢 |
| 87 | `bevel-refusal == 0` |  | 🟢 |
| 93 | `bevel-refusal contains consumed by the blend` | Blend#10: Blend: edge E98: consumed by the blend (blend wider than the edge) (Circle, o... | 🟢 |
| 94 | `bevel-showing ~ 0.02` | 0.02 | 🟢 |
| 98 | `bevel-active == 0` | 0 | 🟢 |
| 99 | `faces == 12` | 12 | 🟢 |
| 100 | `steps == 9` | 9 | 🟢 |
| 104 | `faces == 11` | 11 | 🟢 |
| 105 | `redo == 1` | 1 | 🟢 |
| 107 | `faces == 12` | 12 | 🟢 |
| 109 | `faces == 11` | 11 | 🟢 |
| 111 | `faces == 12` | 12 | 🟢 |
| 115 | `camera-yaw ~ -0.7853981634` | -0.7853981634 | 🟢 |
| 117 | `camera-yaw ~ -1.7453981634` | -1.745398163 | 🟢 |
| 118 | `camera-pitch ~ 0.6154797087` | 0.6154797087 | 🟢 |
| 120 | `camera-pitch ~ 0.2154797087` | 0.2154797087 | 🟢 |
| 122 | `camera-yaw ~ -1.7453981634` | -1.745398163 | 🟢 |
| 123 | `camera-target-x != 2` | 1.42168656 | 🟢 |
| 125 | `camera-target-z != 0.875` | 1.257440933 | 🟢 |
| 127 | `camera-distance < 7.9` | 6.232551794 | 🟢 |
| 132 | `camera-pitch ~ 1.5707963268` | 1.570796327 | 🟢 |
| 133 | `grid-plane == 0` | 0 | 🟢 |
| 136 | `orthographic == 1` | 1 | 🟢 |
| 140 | `camera-pitch ~ 0` | 0 | 🟢 |
| 141 | `camera-yaw ~ -1.5707963268` | -1.570796327 | 🟢 |
| 142 | `grid-plane == 1` | 1 | 🟢 |
| 147 | `grid-plane == 2` | 2 | 🟢 |
| 152 | `grid-plane == 2` | 2 | 🟢 |
| 160 | `grid == 0` | 0 | 🟢 |
| 168 | `camera-pitch ~ 0.6154797087` | 0.6154797087 | 🟢 |
| 169 | `grid-plane == 0` | 0 | 🟢 |
| 176 | `selected-faces == 1` | Extrude#4.Cap(End) | 🟢 |
| 177 | `selected-faces contains Extrude#4.Cap(End)` | Extrude#4.Cap(End) | 🟢 |
| 180 | `steps == 10` | 10 | 🟢 |
| 181 | `sound == 1` | 1 | 🟢 |
| 182 | `status contains Push` | Push 0.25 on 1 face(s) · 144 ms | 🟢 |
| 186 | `focused-step == Extrude#2` | Extrude#2 | 🟢 |
| 192 | `status contains Block1.Height` | Block1.Height = 1.25 · 437 ms | 🟢 |
| 193 | `refused == 0` | 0 | 🟢 |
| 194 | `sound == 1` | 1 | 🟢 |
| 225 | `steps == 12` | 12 | 🟢 |
| 226 | `refused == 0` | 0 | 🟢 |
| 227 | `sound == 1` | 1 | 🟢 |
| 228 | `genus == 2` | 2 | 🟢 |
| 233 | `dirty == 0` | 0 | 🟢 |
| 234 | `file:_ClaudeScratch/SolidArcWorkspace/Bracket.solidarc == 1` | 1 | 🟢 |
| 236 | `file:_ClaudeScratch/SolidArcWorkspace/Recovery.solidarc == 1` | 1 | 🟢 |
| 238 | `steps == 0` | 0 | 🟢 |
| 240 | `file:_ClaudeScratch/SolidArcWorkspace/Recovery.previous.solidarc == 1` | 1 | 🟢 |
| 243 | `steps > 9` | 12 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 01Workspace | 4 | `80a16b11741267c9` |
| 02FacesSelected | 24 | `7efbb335a4dc6291` |
| 03EdgeSelected | 46 | `2d51f301c1ff4f2b` |
| 04BevelPreview | 75 | `8cc74d98ff9842af` |
| 05FilletApplied | 86 | `975a509ecc3d9f8f` |
| 06BevelRefusal | 122 | `742fc597b5da8171` |
| 07Navigated | 198 | `3fe99fc4ed2e3c79` |
| 08ViewCubeTop | 205 | `bd0447e97ee32db9` |
| 09FrontParallel | 224 | `c8a9d925d1e1061f` |
| 09RightParallel | 238 | `11978eff7618b764` |
| 10FaceEdit | 323 | `70b5ffb456a80874` |
| 11ParameterEdit | 353 | `1a6ec68b1afc4522` |
| 12Construct | 436 | `f9a88a2587108cf3` |
| 13Constructed | 445 | `6678472e3abada8e` |
| 14NewDocument | 500 | `33eb2be79135e99d` |
| 15Restored | 509 | `be5b1bd3ce21b6a3` |

## Journal

- Selected Extrude#2.Cap(End)
- Added Extrude#4.Cap(End)
- Removed Extrude#4.Cap(End)
- Selection cleared
- Selected Edge(Extrude#2.Cap(Start)|Extrude#2.Side(Line1))
- Fillet (B): drag the handles (corner handles make it variable) or drag / move anywhere · D fillet · C chamfer · V variable size · click an edge to add / remove it · Enter / right-click applies · Esc aborts
- Bevel size 0.3 m (pinned)
- Fillet 0.3 on 1 edge(s) · 195 ms
- Selected Edge(Extrude#2.Cap(End)|Extrude#2.Side(Line1))
- Fillet (B): drag the handles (corner handles make it variable) or drag / move anywhere · D fillet · C chamfer · V variable size · click an edge to add / remove it · Enter / right-click applies · Esc aborts · starts at 0.02 m to fit the edges
- Bevel size 0.3 m (pinned)
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
- Block1.Height = 1.25 · 437 ms
- Cylinder: click the base centre · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Autosaved _ClaudeScratch/SolidArcWorkspace/Recovery.solidarc
- Cylinder3 · Difference · 134 ms
- Saved _ClaudeScratch/SolidArcWorkspace/Bracket.solidarc
- Autosaved _ClaudeScratch/SolidArcWorkspace/Recovery.solidarc
- New document (previous recovery copy kept)
- Opened _ClaudeScratch/SolidArcWorkspace/Recovery.previous.solidarc · 12 steps
