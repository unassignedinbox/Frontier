# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 900 · 344 frames · 35.1425 s · slowest frame 502.045 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/BevelHandlesWalkthrough.txt
- Expectations: 49 / 49 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 29 | `refused == 0` | 0 | 🟢 |
| 30 | `volume ~ 8` | 8 | 🟢 |
| 38 | `selected-edges == 1` | Edge(Extrude#2.Cap(End)/Extrude#2.Side(Line1)) | 🟢 |
| 41 | `bevel-active == 1` | 1 | 🟢 |
| 42 | `bevel-size ~ 0.25` | 0.25 | 🟢 |
| 43 | `bevel-span ~ 2` | 2 | 🟢 |
| 44 | `bevel-fitted ~ 0.2` | 0.2 | 🟢 |
| 45 | `bevel-handles == 3` | 3 | 🟢 |
| 46 | `bevel-corners == 0` | 0 | 🟢 |
| 47 | `bevel-refusal == 0` |  | 🟢 |
| 48 | `bevel-preview-volume ~ 7.973174770424681` | 7.97317477 | 🟢 |
| 55 | `bevel-active == 1` | 1 | 🟢 |
| 56 | `selected-edges == 1` | Edge(Extrude#2.Cap(End)/Extrude#2.Side(Line1)) | 🟢 |
| 59 | `bevel-active == 1` | 1 | 🟢 |
| 60 | `selected-edges == 1` | Edge(Extrude#2.Cap(End)/Extrude#2.Side(Line1)) | 🟢 |
| 61 | `bevel-size != 0.25` | 0.3962 | 🟢 |
| 62 | `bevel-chamfer == 0` | 0 | 🟢 |
| 63 | `bevel-pinned == 1` | 1 | 🟢 |
| 64 | `bevel-refusal == 0` |  | 🟢 |
| 72 | `bevel-size ~ 0.25` | 0.25 | 🟢 |
| 73 | `bevel-pinned == 1` | 1 | 🟢 |
| 76 | `bevel-corners == 1` | 1 | 🟢 |
| 77 | `bevel-variable == 0` | 0 | 🟢 |
| 78 | `bevel-corner-1 != 0.25` | 0.193 | 🟢 |
| 79 | `bevel-corner-2 ~ 0.25` | 0.25 | 🟢 |
| 80 | `bevel-refusal == 0` |  | 🟢 |
| 89 | `bevel-corners == 1` | 1 | 🟢 |
| 90 | `bevel-corner-1 ~ 0.4` | 0.4 | 🟢 |
| 91 | `bevel-refusal == 0` |  | 🟢 |
| 95 | `bevel-active == 0` | 0 | 🟢 |
| 96 | `refused == 0` | 0 | 🟢 |
| 97 | `sound == 1` | 1 | 🟢 |
| 98 | `status contains corner size` | Fillet 0.25 · 1 corner size(s) on 1 edge(s) · 48 ms | 🟢 |
| 119 | `volume ~ 0.008` | 0.008 | 🟢 |
| 128 | `selected-edges == 1` | Edge(Extrude#2.Cap(End)/Extrude#2.Side(Line1)) | 🟢 |
| 131 | `bevel-active == 1` | 1 | 🟢 |
| 132 | `bevel-span ~ 0.2` | 0.2 | 🟢 |
| 133 | `bevel-size ~ 0.02` | 0.02 | 🟢 |
| 134 | `bevel-refusal == 0` |  | 🟢 |
| 135 | `bevel-handles == 3` | 3 | 🟢 |
| 151 | `selected-edges == 4` | Edge(Extrude#2.Cap(End)/Extrude#2.Side(Line1)),Edge(Extrude#2.Cap(End)/Extrude#2.Side(L... | 🟢 |
| 154 | `bevel-handles == 5` | 5 | 🟢 |
| 155 | `bevel-refusal == 0` |  | 🟢 |
| 158 | `bevel-corners == 1` | 1 | 🟢 |
| 159 | `bevel-corner-2 > 0.021` | 0.047 | 🟢 |
| 160 | `bevel-refusal == 0` |  | 🟢 |
| 164 | `bevel-active == 0` | 0 | 🟢 |
| 165 | `refused == 0` | 0 | 🟢 |
| 166 | `sound == 1` | 1 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 276BevelHandles | 79 | `582e5598db44222e` |
| 277BevelCornerDragged | 150 | `b938db41f5ce5775` |
| 278BevelCornerTyped | 173 | `7991d91cc10e8eb7` |
| 279BevelFitted | 272 | `e3a441f28c75c308` |
| 280BevelLoopCorner | 337 | `2e7ffd4910db12f3` |

## Journal

- New document (previous recovery copy kept)
- Block: click the first corner · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Block: click the opposite corner
- Block: move up, click the height
- Block1 · New body · 15 ms
- Selected Edge(Extrude#2.Cap(End)|Extrude#2.Side(Line1))
- Fillet (B): drag the handles (corner handles make it variable) or drag / move anywhere · D fillet · C chamfer · V variable size · click an edge to add / remove it · Enter / right-click applies · Esc aborts
- Bevel: the edges stay picked (click an edge to add / remove it) · drag the handles or drag / move anywhere to size
- Bevel size 0.25 m (pinned)
- Corner 1: 0.193 m (variable · 1 corner size(s))
- Corner 1: 0.4 m
- Fillet 0.25 · 1 corner size(s) on 1 edge(s) · 48 ms
- New document (previous recovery copy kept)
- Block: click the first corner · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Block: click the opposite corner
- Autosaved _ClaudeScratch/SolidArcWorkspace/BevelHandlesRecovery.solidarc
- Block: move up, click the height
- Block1 · New body · 1 ms
- Selection cleared
- Selected Edge(Extrude#2.Cap(End)|Extrude#2.Side(Line1))
- Fillet (B): drag the handles (corner handles make it variable) or drag / move anywhere · D fillet · C chamfer · V variable size · click an edge to add / remove it · Enter / right-click applies · Esc aborts · starts at 0.02 m to fit the edges
- Bevel cancelled (document untouched)
- Autosaved _ClaudeScratch/SolidArcWorkspace/BevelHandlesRecovery.solidarc
- Selected Edge(Extrude#2.Cap(End)|Extrude#2.Side(Line1))
- Added Edge(Extrude#2.Cap(End)|Extrude#2.Side(Line2))
- Added Edge(Extrude#2.Cap(End)|Extrude#2.Side(Line3))
- Added Edge(Extrude#2.Cap(End)|Extrude#2.Side(Line4))
- Fillet (B): drag the handles (corner handles make it variable) or drag / move anywhere · D fillet · C chamfer · V variable size · click an edge to add / remove it · Enter / right-click applies · Esc aborts
- Corner 2: 0.047 m (variable · 1 corner size(s))
- Fillet 0.02 · 1 corner size(s) on 4 edge(s) · 112 ms
