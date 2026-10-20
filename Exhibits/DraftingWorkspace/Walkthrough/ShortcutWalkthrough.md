# DraftingWorkspaceCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0
- Frame: 1600 x 900 · 126 frames · 17.0889 s · slowest frame 1113.85 ms
- Script: Experimental/DraftingWorkspace/Scripts/ShortcutWalkthrough.txt
- Expectations: 41 / 41 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 7 | `faces == 11` | 11 | 🟢 |
| 8 | `steps == 8` | 8 | 🟢 |
| 10 | `pick-mode == 2` | 2 | 🟢 |
| 12 | `selected-edges == 24` | Edge(Extrude#2.Cap(Start)/Extrude#2.Side(Line1)),Edge(Extrude#2.Side(Line1)/Extrude#2.S... | 🟢 |
| 14 | `pick-mode == 1` | 1 | 🟢 |
| 16 | `selected-faces == 11` | Extrude#2.Side(Line1),Extrude#2.Side(Line2),Extrude#2.Side(Line3),Extrude#2.Side(Line4)... | 🟢 |
| 17 | `selected-edges == 0` |  | 🟢 |
| 19 | `selected-faces == 0` |  | 🟢 |
| 27 | `selected-edges == 1` | Edge(Extrude#2.Cap(Start)/Extrude#2.Side(Line1)) | 🟢 |
| 28 | `undo == 5` | 5 | 🟢 |
| 31 | `bevel-active == 1` | 1 | 🟢 |
| 32 | `bevel-chamfer == 0` | 0 | 🟢 |
| 37 | `bevel-chamfer == 1` | 1 | 🟢 |
| 38 | `bevel-pinned == 0` | 0 | 🟢 |
| 42 | `bevel-active == 0` | 0 | 🟢 |
| 43 | `faces == 11` | 11 | 🟢 |
| 44 | `steps == 8` | 8 | 🟢 |
| 45 | `undo == 5` | 5 | 🟢 |
| 55 | `bevel-pinned == 1` | 1 | 🟢 |
| 56 | `bevel-size ~ 0.3` | 0.3 | 🟢 |
| 59 | `bevel-active == 0` | 0 | 🟢 |
| 60 | `faces == 12` | 12 | 🟢 |
| 61 | `steps == 9` | 9 | 🟢 |
| 62 | `sound == 1` | 1 | 🟢 |
| 66 | `faces == 11` | 11 | 🟢 |
| 67 | `redo == 1` | 1 | 🟢 |
| 69 | `faces == 12` | 12 | 🟢 |
| 70 | `redo == 0` | 0 | 🟢 |
| 74 | `focused-step == Blend#9` | Blend#9 | 🟢 |
| 77 | `steps == 8` | 8 | 🟢 |
| 78 | `faces == 11` | 11 | 🟢 |
| 79 | `focused-step != Blend#9` |  | 🟢 |
| 84 | `camera-target-x != 2` | 1.422371101 | 🟢 |
| 86 | `camera-target-x ~ 2` | 2 | 🟢 |
| 87 | `camera-target-z ~ 0.875` | 0.875 | 🟢 |
| 94 | `selected-faces contains Extrude#4.Cap(End)` | Extrude#4.Cap(End) | 🟢 |
| 97 | `steps == 9` | 9 | 🟢 |
| 98 | `refused == 0` | 0 | 🟢 |
| 99 | `sound == 1` | 1 | 🟢 |
| 100 | `volume > 10.60` | 10.61547283 | 🟢 |
| 101 | `volume < 10.63` | 10.61547283 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 19BevelSignChamfer | 43 | `2fb22b6e5a04e4d2` |
| 20ShortcutExtrude | 126 | `d2b0ea306bebaf0a` |

## Journal

- Selected all edges
- Selected all faces
- Selection cleared
- Selected Edge(Extrude#2.Cap(Start)|Extrude#2.Side(Line1))
- Fillet session (B): move horizontally, Enter applies
- Bevel cancelled (document untouched)
- Fillet session (B): move horizontally, Enter applies
- Bevel size 0.3 m (pinned)
- Fillet 0.3 on 1 edge(s) · 467 ms
- Undo
- Redo
- Delete Fillet2 · 349 ms
- Selection cleared
- Selected Extrude#4.Cap(End)
- ExtrudeFaces 0.5 on 1 face(s) · 450 ms
