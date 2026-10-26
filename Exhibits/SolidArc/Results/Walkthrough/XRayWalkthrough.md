# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 900 · 131 frames · 21.1689 s · slowest frame 519.77 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/XRayWalkthrough.txt
- Expectations: 36 / 36 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 11 | `steps == 8` | 8 | 🟢 |
| 12 | `volume ~ 10.22278` | 10.22277375 | 🟢 |
| 13 | `xray == 0` | 0 | 🟢 |
| 15 | `pick-mode == 2` | 2 | 🟢 |
| 18 | `hover lacks Side(Line3)` | Extrude#6.Side(Circle1) | 🟢 |
| 25 | `box-edges == 17` | 17 | 🟢 |
| 26 | `selected-edges == 17` | Edge(Extrude#2.Cap(Start)/Extrude#2.Side(Line1)),Edge(Extrude#2.Side(Line1)/Extrude#2.S... | 🟢 |
| 27 | `selected-edges lacks Edge(Extrude#2.Cap(Start)|Extrude#2.Side(Line3))` | Edge(Extrude#2.Cap(Start)/Extrude#2.Side(Line1)),Edge(Extrude#2.Side(Line1)/Extrude#2.S... | 🟢 |
| 30 | `selected-edges == 0` |  | 🟢 |
| 35 | `xray == 1` | 1 | 🟢 |
| 36 | `surfaces == 1` | 1 | 🟢 |
| 37 | `status contains X-ray on` | X-ray on · faces 45 % · hidden edges pickable | 🟢 |
| 40 | `box-edges == 24` | 24 | 🟢 |
| 41 | `selected-edges == 24` | Edge(Extrude#2.Cap(Start)/Extrude#2.Side(Line1)),Edge(Extrude#2.Side(Line1)/Extrude#2.S... | 🟢 |
| 42 | `selected-edges contains Edge(Extrude#2.Cap(Start)|Extrude#2.Side(Line3))` | Edge(Extrude#2.Cap(Start)/Extrude#2.Side(Line1)),Edge(Extrude#2.Side(Line1)/Extrude#2.S... | 🟢 |
| 45 | `selected-edges == 0` |  | 🟢 |
| 50 | `hover == Edge(Extrude#2.Cap(Start)|Extrude#2.Side(Line3))` | Edge(Extrude#2.Cap(Start)/Extrude#2.Side(Line3)) | 🟢 |
| 56 | `selected-edges == 1` | Edge(Extrude#2.Cap(Start)/Extrude#2.Side(Line3)) | 🟢 |
| 57 | `selected-edges contains Edge(Extrude#2.Cap(Start)|Extrude#2.Side(Line3))` | Edge(Extrude#2.Cap(Start)/Extrude#2.Side(Line3)) | 🟢 |
| 64 | `bevel-active == 1` | 1 | 🟢 |
| 65 | `bevel-chamfer == 0` | 0 | 🟢 |
| 66 | `bevel-size ~ 0.25` | 0.25 | 🟢 |
| 67 | `preview-faces == 12` | 12 | 🟢 |
| 71 | `bevel-active == 0` | 0 | 🟢 |
| 72 | `steps == 9` | 9 | 🟢 |
| 73 | `faces == 12` | 12 | 🟢 |
| 74 | `sound == 1` | 1 | 🟢 |
| 75 | `volume ~ 10.16913` | 10.16912329 | 🟢 |
| 82 | `pick-mode == 3` | 3 | 🟢 |
| 85 | `selected-faces == 1` | Extrude#2.Cap(End) | 🟢 |
| 86 | `selected-faces contains Cap(End)` | Extrude#2.Cap(End) | 🟢 |
| 87 | `selected-edges == 0` |  | 🟢 |
| 94 | `xray == 0` | 0 | 🟢 |
| 99 | `hover lacks Side(Line3)` | Extrude#6.Side(Circle1) | 🟢 |
| 102 | `steps == 8` | 8 | 🟢 |
| 103 | `volume ~ 10.22278` | 10.22277375 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 146XRayBoxOpaque | 31 | `437b24c9c9853d03` |
| 147XRayBoxThrough | 60 | `70ac827aee54a053` |
| 148XRayHover | 69 | `b6ec88b99ac63bf7` |
| 149XRayPicked | 78 | `e79533afcfc9598c` |
| 150XRayFillet | 85 | `e6dd292cafec455e` |
| 151XRayFilleted | 92 | `592bd73e64691299` |

## Journal

- Box selected 0 curve(s) + 17 edge(s) · 17 edge(s) selected
- Selection cleared
- X-ray on · faces 45 % · hidden edges pickable
- Box selected 0 curve(s) + 24 edge(s) (x-ray) · 24 edge(s) selected
- Selection cleared
- Selected Edge(Extrude#2.Cap(Start)|Extrude#2.Side(Line3))
- Fillet (B): drag the handles (corner handles make it variable) or drag / move anywhere · D fillet · C chamfer · V variable size · click an edge to add / remove it · Enter / right-click applies · Esc aborts
- Fillet 0.25 on 1 edge(s) · 140 ms
- Selection cleared
- Selected Extrude#2.Cap(End)
- Selection cleared
- X-ray off
- Autosaved _ClaudeScratch/SolidArcWorkspace/XRayRecovery.solidarc
- Undo
