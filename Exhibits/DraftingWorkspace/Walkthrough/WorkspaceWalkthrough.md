# DraftingWorkspaceCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0
- Frame: 1600 x 900 · 424 frames · 47.4766 s · slowest frame 1567.26 ms
- Script: Experimental/DraftingWorkspace/Scripts/WorkspaceWalkthrough.txt
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
| 81 | `bevel-active == 1` | 1 | 🟢 |
| 82 | `bevel-chamfer == 1` | 1 | 🟢 |
| 83 | `bevel-refusal contains blend ends on a curved face` | Blend#10: Blend: vertex V67: blend ends on a curved face (cap on non-planar faces) | 🟢 |
| 87 | `bevel-active == 0` | 0 | 🟢 |
| 88 | `faces == 12` | 12 | 🟢 |
| 89 | `steps == 9` | 9 | 🟢 |
| 93 | `faces == 11` | 11 | 🟢 |
| 94 | `redo == 1` | 1 | 🟢 |
| 96 | `faces == 12` | 12 | 🟢 |
| 98 | `faces == 11` | 11 | 🟢 |
| 100 | `faces == 12` | 12 | 🟢 |
| 104 | `camera-yaw ~ -0.7853981634` | -0.7853981634 | 🟢 |
| 106 | `camera-yaw ~ -1.7453981634` | -1.745398163 | 🟢 |
| 107 | `camera-pitch ~ 0.6154797087` | 0.6154797087 | 🟢 |
| 109 | `camera-pitch ~ 0.2154797087` | 0.2154797087 | 🟢 |
| 111 | `camera-yaw ~ -1.7453981634` | -1.745398163 | 🟢 |
| 112 | `camera-target-x != 2` | 1.531803777 | 🟢 |
| 114 | `camera-target-z != 0.875` | 1.184619988 | 🟢 |
| 116 | `camera-distance < 7.9` | 6.232551794 | 🟢 |
| 121 | `camera-pitch ~ 1.5707963268` | 1.570796327 | 🟢 |
| 122 | `grid-plane == 0` | 0 | 🟢 |
| 125 | `orthographic == 1` | 1 | 🟢 |
| 127 | `camera-pitch ~ 0` | 0 | 🟢 |
| 128 | `camera-yaw ~ -1.5707963268` | -1.570796327 | 🟢 |
| 129 | `grid-plane == 1` | 1 | 🟢 |
| 132 | `grid-plane == 2` | 2 | 🟢 |
| 135 | `grid-plane == 2` | 2 | 🟢 |
| 139 | `grid == 0` | 0 | 🟢 |
| 143 | `camera-pitch ~ 0.6154797087` | 0.6154797087 | 🟢 |
| 144 | `grid-plane == 0` | 0 | 🟢 |
| 151 | `selected-faces == 1` | Extrude#4.Cap(End) | 🟢 |
| 152 | `selected-faces contains Extrude#4.Cap(End)` | Extrude#4.Cap(End) | 🟢 |
| 155 | `steps == 10` | 10 | 🟢 |
| 156 | `sound == 1` | 1 | 🟢 |
| 157 | `status contains Push` | Push 0.25 on 1 face(s) · 590 ms | 🟢 |
| 161 | `focused-step == Extrude#2` | Extrude#2 | 🟢 |
| 167 | `status contains Block1.Height` | Block1.Height = 1.25 · 604 ms | 🟢 |
| 168 | `refused == 0` | 0 | 🟢 |
| 169 | `sound == 1` | 1 | 🟢 |
| 196 | `steps == 12` | 12 | 🟢 |
| 197 | `refused == 0` | 0 | 🟢 |
| 198 | `sound == 1` | 1 | 🟢 |
| 199 | `genus == 2` | 2 | 🟢 |
| 204 | `dirty == 0` | 0 | 🟢 |
| 205 | `file:_ClaudeScratch/DraftingWorkspace/Bracket.drafting.toml == 1` | 1 | 🟢 |
| 207 | `file:_ClaudeScratch/DraftingWorkspace/Recovery.drafting.toml == 1` | 1 | 🟢 |
| 209 | `steps == 0` | 0 | 🟢 |
| 211 | `file:_ClaudeScratch/DraftingWorkspace/Recovery.previous.drafting.toml == 1` | 1 | 🟢 |
| 214 | `steps > 9` | 12 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 01Workspace | 4 | `e8659fc7fa8e95ac` |
| 02FacesSelected | 24 | `91c61f21cfaa875f` |
| 03EdgeSelected | 46 | `6cbd5836fcca1e91` |
| 04BevelPreview | 75 | `a45ab10ad9b1242e` |
| 05FilletApplied | 86 | `d4f77049d4bfb398` |
| 06BevelRefusal | 102 | `15fcd90ce9deb503` |
| 07Navigated | 178 | `5ef9d616c483b7d6` |
| 08ViewCubeTop | 185 | `5c1e831cd2a59442` |
| 09FrontParallel | 197 | `cddb6f969b2facb4` |
| 09RightParallel | 204 | `f33f992c35d22770` |
| 10FaceEdit | 253 | `dbb5ad7cd46dcd71` |
| 11ParameterEdit | 280 | `487a8a82e59fa451` |
| 12Construct | 351 | `311692be2c923044` |
| 13Constructed | 360 | `38b4a046d3e39946` |
| 14NewDocument | 415 | `d619206a71126874` |
| 15Restored | 424 | `0e43da23145a723f` |

## Journal

- Selected Extrude#2.Cap(End)
- Toggled Extrude#4.Cap(End)
- Toggled Extrude#4.Cap(End)
- Selection cleared
- Selected Edge(Extrude#2.Cap(Start)|Extrude#2.Side(Line1))
- Fillet session (B): move horizontally, Enter applies
- Bevel size 0.3 m (pinned)
- Fillet 0.3 on 1 edge(s) · 459 ms
- Selected Edge(Extrude#2.Cap(End)|Extrude#2.Side(Line1))
- Chamfer session (Shift+B): move horizontally, Enter applies
- Bevel cancelled (document untouched)
- Undo
- Redo
- Undo
- Redo
- Selection cleared
- View Top (orientation cube)
- Autosaved _ClaudeScratch/DraftingWorkspace/Recovery.drafting.toml
- View Front
- View Right
- View Isometric
- View Top
- View Isometric
- Selected Extrude#4.Cap(End)
- Push 0.25 on 1 face(s) · 590 ms
- Block1.Height = 1.25 · 604 ms
- Autosaved _ClaudeScratch/DraftingWorkspace/Recovery.drafting.toml
- Cylinder3 · Difference · 777 ms
- Saved _ClaudeScratch/DraftingWorkspace/Bracket.drafting.toml
- Autosaved _ClaudeScratch/DraftingWorkspace/Recovery.drafting.toml
- New document (previous recovery copy kept)
- Opened _ClaudeScratch/DraftingWorkspace/Recovery.previous.drafting.toml · 12 steps
