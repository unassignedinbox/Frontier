# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 900 · 132 frames · 20.9531 s · slowest frame 534.915 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/XRayWalkthrough.txt
- Expectations: 36 / 36 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 14 | `steps == 8` | 8 | 🟢 |
| 15 | `volume ~ 10.22278` | 10.22277375 | 🟢 |
| 16 | `xray == 0` | 0 | 🟢 |
| 18 | `pick-mode == 2` | 2 | 🟢 |
| 21 | `hover lacks Side(Line3)` | Extrude#6.Side(Circle1) | 🟢 |
| 28 | `box-edges == 17` | 17 | 🟢 |
| 29 | `selected-edges == 17` | Edge(Extrude#2.Cap(Start)/Extrude#2.Side(Line1)),Edge(Extrude#2.Side(Line1)/Extrude#2.S... | 🟢 |
| 30 | `selected-edges lacks Edge(Extrude#2.Cap(Start)|Extrude#2.Side(Line3))` | Edge(Extrude#2.Cap(Start)/Extrude#2.Side(Line1)),Edge(Extrude#2.Side(Line1)/Extrude#2.S... | 🟢 |
| 33 | `selected-edges == 0` |  | 🟢 |
| 38 | `xray == 1` | 1 | 🟢 |
| 39 | `surfaces == 1` | 1 | 🟢 |
| 40 | `status contains X-ray on` | X-ray on · faces 45 % · hidden edges pickable | 🟢 |
| 43 | `box-edges == 24` | 24 | 🟢 |
| 44 | `selected-edges == 24` | Edge(Extrude#2.Cap(Start)/Extrude#2.Side(Line1)),Edge(Extrude#2.Side(Line1)/Extrude#2.S... | 🟢 |
| 45 | `selected-edges contains Edge(Extrude#2.Cap(Start)|Extrude#2.Side(Line3))` | Edge(Extrude#2.Cap(Start)/Extrude#2.Side(Line1)),Edge(Extrude#2.Side(Line1)/Extrude#2.S... | 🟢 |
| 48 | `selected-edges == 0` |  | 🟢 |
| 53 | `hover == Edge(Extrude#2.Cap(Start)|Extrude#2.Side(Line3))` | Edge(Extrude#2.Cap(Start)/Extrude#2.Side(Line3)) | 🟢 |
| 59 | `selected-edges == 1` | Edge(Extrude#2.Cap(Start)/Extrude#2.Side(Line3)) | 🟢 |
| 60 | `selected-edges contains Edge(Extrude#2.Cap(Start)|Extrude#2.Side(Line3))` | Edge(Extrude#2.Cap(Start)/Extrude#2.Side(Line3)) | 🟢 |
| 67 | `bevel-active == 1` | 1 | 🟢 |
| 68 | `bevel-chamfer == 0` | 0 | 🟢 |
| 69 | `bevel-size ~ 0.25` | 0.25 | 🟢 |
| 70 | `preview-faces == 12` | 12 | 🟢 |
| 74 | `bevel-active == 0` | 0 | 🟢 |
| 75 | `steps == 9` | 9 | 🟢 |
| 76 | `faces == 12` | 12 | 🟢 |
| 77 | `sound == 1` | 1 | 🟢 |
| 78 | `volume ~ 10.16913` | 10.16912329 | 🟢 |
| 85 | `pick-mode == 3` | 3 | 🟢 |
| 88 | `selected-faces == 1` | Extrude#2.Cap(End) | 🟢 |
| 89 | `selected-faces contains Cap(End)` | Extrude#2.Cap(End) | 🟢 |
| 90 | `selected-edges == 0` |  | 🟢 |
| 97 | `xray == 0` | 0 | 🟢 |
| 102 | `hover lacks Side(Line3)` | Extrude#6.Side(Circle1) | 🟢 |
| 105 | `steps == 8` | 8 | 🟢 |
| 106 | `volume ~ 10.22278` | 10.22277375 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 146XRayBoxOpaque | 32 | `fa1037a073f26d9c` |
| 147XRayBoxThrough | 61 | `edcf75caa2b4aafc` |
| 148XRayHover | 70 | `eadd4b9c2da920bb` |
| 149XRayPicked | 79 | `d75bc6682a136064` |
| 150XRayFillet | 86 | `7877ca5887f85a81` |
| 151XRayFilleted | 93 | `f1c7098a33afc0d2` |

## Journal

- Box selected 0 curve(s) + 17 edge(s) · 17 edge(s) selected
- Selection cleared
- X-ray on · faces 45 % · hidden edges pickable
- Box selected 0 curve(s) + 24 edge(s) (x-ray) · 24 edge(s) selected
- Selection cleared
- Selected Edge(Extrude#2.Cap(Start)|Extrude#2.Side(Line3))
- Fillet (B): drag anywhere or the ring handle to size (it stops at the largest that fits) · drag a corner diamond for a variable size · type in the card · D fillet · C chamfer · V taper · Alt+click an edge adds its ring · Enter / right-click applies · Esc aborts
- Fillet 0.25 on 1 edge(s) · 3 ms
- Selection cleared
- Selected Extrude#2.Cap(End)
- Selection cleared
- X-ray off
- Autosaved _ClaudeScratch/SolidArcWorkspace/XRayRecovery.solidarc
- Undo
