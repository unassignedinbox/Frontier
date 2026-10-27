# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 900 · 440 frames · 44.4388 s · slowest frame 513.482 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/BevelHandlesWalkthrough.txt
- Expectations: 59 / 59 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 32 | `refused == 0` | 0 | 🟢 |
| 33 | `volume ~ 8` | 8 | 🟢 |
| 41 | `selected-edges == 1` | Edge(Extrude#2.Cap(End)/Extrude#2.Side(Line1)) | 🟢 |
| 44 | `bevel-active == 1` | 1 | 🟢 |
| 45 | `bevel-size ~ 0.25` | 0.25 | 🟢 |
| 46 | `bevel-span ~ 2` | 2 | 🟢 |
| 47 | `bevel-fitted ~ 0.2` | 0.2 | 🟢 |
| 48 | `bevel-handles == 3` | 3 | 🟢 |
| 49 | `bevel-corners == 0` | 0 | 🟢 |
| 50 | `bevel-refusal == 0` |  | 🟢 |
| 51 | `bevel-preview-volume ~ 7.973174770424681` | 7.97317477 | 🟢 |
| 58 | `bevel-active == 1` | 1 | 🟢 |
| 59 | `selected-edges == 1` | Edge(Extrude#2.Cap(End)/Extrude#2.Side(Line1)) | 🟢 |
| 62 | `bevel-active == 1` | 1 | 🟢 |
| 63 | `selected-edges == 1` | Edge(Extrude#2.Cap(End)/Extrude#2.Side(Line1)) | 🟢 |
| 64 | `bevel-size != 0.25` | 0.6452 | 🟢 |
| 65 | `bevel-chamfer == 0` | 0 | 🟢 |
| 66 | `bevel-pinned == 1` | 1 | 🟢 |
| 67 | `bevel-refusal == 0` |  | 🟢 |
| 75 | `bevel-size ~ 0.25` | 0.25 | 🟢 |
| 76 | `bevel-pinned == 1` | 1 | 🟢 |
| 79 | `bevel-corners == 1` | 1 | 🟢 |
| 80 | `bevel-variable == 0` | 0 | 🟢 |
| 81 | `bevel-corner-1 != 0.25` | 0.193 | 🟢 |
| 82 | `bevel-corner-2 ~ 0.25` | 0.25 | 🟢 |
| 83 | `bevel-refusal == 0` |  | 🟢 |
| 92 | `bevel-corners == 1` | 1 | 🟢 |
| 93 | `bevel-corner-1 ~ 0.4` | 0.4 | 🟢 |
| 94 | `bevel-refusal == 0` |  | 🟢 |
| 98 | `bevel-active == 0` | 0 | 🟢 |
| 99 | `refused == 0` | 0 | 🟢 |
| 100 | `sound == 1` | 1 | 🟢 |
| 101 | `status contains corner size` | Fillet 0.25 · 1 corner size(s) on 1 edge(s) · 11 ms | 🟢 |
| 122 | `volume ~ 0.008` | 0.008 | 🟢 |
| 131 | `selected-edges == 1` | Edge(Extrude#2.Cap(End)/Extrude#2.Side(Line1)) | 🟢 |
| 134 | `bevel-active == 1` | 1 | 🟢 |
| 135 | `bevel-span ~ 0.2` | 0.2 | 🟢 |
| 136 | `bevel-size ~ 0.02` | 0.02 | 🟢 |
| 137 | `bevel-refusal == 0` |  | 🟢 |
| 138 | `bevel-handles == 3` | 3 | 🟢 |
| 154 | `selected-edges == 4` | Edge(Extrude#2.Cap(End)/Extrude#2.Side(Line1)),Edge(Extrude#2.Cap(End)/Extrude#2.Side(L... | 🟢 |
| 157 | `bevel-handles == 5` | 5 | 🟢 |
| 158 | `bevel-refusal == 0` |  | 🟢 |
| 161 | `bevel-corners == 1` | 1 | 🟢 |
| 162 | `bevel-corner-2 > 0.021` | 0.047 | 🟢 |
| 163 | `bevel-refusal == 0` |  | 🟢 |
| 167 | `bevel-active == 0` | 0 | 🟢 |
| 168 | `refused == 0` | 0 | 🟢 |
| 169 | `sound == 1` | 1 | 🟢 |
| 178 | `volume ~ 0.008` | 0.008 | 🟢 |
| 185 | `selected-edges == 1` | Edge(Extrude#2.Cap(End)/Extrude#2.Side(Line1)) | 🟢 |
| 188 | `bevel-pinned == 1` | 1 | 🟢 |
| 194 | `bevel-refusal == 0` |  | 🟢 |
| 195 | `bevel-size < 0.5` | 0.1998 | 🟢 |
| 196 | `bevel-size > 0.01` | 0.1998 | 🟢 |
| 197 | `bevel-limit-stops > 0` | 1 | 🟢 |
| 205 | `bevel-refusal == 0` |  | 🟢 |
| 206 | `bevel-size < 0.5` | 0.1358 | 🟢 |
| 209 | `bevel-active == 0` | 0 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 276BevelHandles | 80 | `e1888c9df357536c` |
| 277BevelCornerDragged | 151 | `1fbf11217dc0950d` |
| 278BevelCornerTyped | 174 | `010b85fb280db923` |
| 279BevelFitted | 273 | `d0c8471d10afe6dd` |
| 280BevelLoopCorner | 338 | `5268ec359514c211` |
| 297BevelBound | 410 | `c7005414f449be34` |

## Journal

- New document (previous recovery copy kept)
- Block: click the first corner · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Block: click the opposite corner
- Block: move up, click the height
- Block1 · New body · 0 ms
- Selected Edge(Extrude#2.Cap(End)|Extrude#2.Side(Line1))
- Fillet (B): drag anywhere or the ring handle to size (it stops at the largest that fits) · drag a corner diamond for a variable size · type in the card · D fillet · C chamfer · V taper · Alt+click an edge adds its ring · Enter / right-click applies · Esc aborts
- Bevel: the edges stay picked (click an edge to add / remove it, Alt+click its ring) · drag anywhere or the handles to size
- Bevel size 0.25 m (pinned)
- Corner 1: 0.193 m (variable · 1 corner size(s))
- Corner 1: 0.4 m
- Fillet 0.25 · 1 corner size(s) on 1 edge(s) · 11 ms
- New document (previous recovery copy kept)
- Block: click the first corner · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Block: click the opposite corner
- Autosaved _ClaudeScratch/SolidArcWorkspace/BevelHandlesRecovery.solidarc
- Block: move up, click the height
- Block1 · New body · 0 ms
- Selection cleared
- Selected Edge(Extrude#2.Cap(End)|Extrude#2.Side(Line1))
- Fillet (B): drag anywhere or the ring handle to size (it stops at the largest that fits) · drag a corner diamond for a variable size · type in the card · D fillet · C chamfer · V taper · Alt+click an edge adds its ring · Enter / right-click applies · Esc aborts · starts at 0.02 m to fit the edges
- Bevel cancelled (document untouched)
- Autosaved _ClaudeScratch/SolidArcWorkspace/BevelHandlesRecovery.solidarc
- Selected Edge(Extrude#2.Cap(End)|Extrude#2.Side(Line1))
- Added Edge(Extrude#2.Cap(End)|Extrude#2.Side(Line2))
- Added Edge(Extrude#2.Cap(End)|Extrude#2.Side(Line3))
- Added Edge(Extrude#2.Cap(End)|Extrude#2.Side(Line4))
- Fillet (B): drag anywhere or the ring handle to size (it stops at the largest that fits) · drag a corner diamond for a variable size · type in the card · D fillet · C chamfer · V taper · Alt+click an edge adds its ring · Enter / right-click applies · Esc aborts
- Corner 2: 0.047 m (variable · 1 corner size(s))
- Fillet 0.02 · 1 corner size(s) on 4 edge(s) · 59 ms
- Selection cleared
- Undo
- Selection cleared
- Selected Edge(Extrude#2.Cap(End)|Extrude#2.Side(Line1))
- Fillet (B): drag anywhere or the ring handle to size (it stops at the largest that fits) · drag a corner diamond for a variable size · type in the card · D fillet · C chamfer · V taper · Alt+click an edge adds its ring · Enter / right-click applies · Esc aborts
- Bevel size 0.5 m is past the largest these edges take: held at 0.1998 m
- Bevel cancelled (document untouched)
