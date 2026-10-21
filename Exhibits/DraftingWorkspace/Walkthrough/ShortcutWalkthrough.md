# DraftingWorkspaceCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0
- Frame: 1600 x 900 · 256 frames · 28.8019 s · slowest frame 896.489 ms
- Script: /home/user/Frontier/Experimental/DraftingWorkspace/Scripts/ShortcutWalkthrough.txt
- Expectations: 68 / 68 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 7 | `faces == 11` | 11 | 🟢 |
| 8 | `steps == 8` | 8 | 🟢 |
| 10 | `pick-mode == 2` | 2 | 🟢 |
| 12 | `pick-mode == 3` | 3 | 🟢 |
| 14 | `pick-mode == 4` | 4 | 🟢 |
| 18 | `selected-faces == 11` | Extrude#2.Side(Line1),Extrude#2.Side(Line2),Extrude#2.Side(Line3),Extrude#2.Side(Line4)... | 🟢 |
| 20 | `selected-faces == 0` |  | 🟢 |
| 22 | `pick-mode == 5` | 5 | 🟢 |
| 24 | `pick-mode == 1` | 1 | 🟢 |
| 26 | `pick-mode == 5` | 5 | 🟢 |
| 34 | `selected-edges == 1` | Edge(Extrude#2.Cap(Start)/Extrude#2.Side(Line1)) | 🟢 |
| 35 | `undo == 5` | 5 | 🟢 |
| 38 | `bevel-active == 1` | 1 | 🟢 |
| 39 | `bevel-chamfer == 0` | 0 | 🟢 |
| 41 | `bevel-chamfer == 1` | 1 | 🟢 |
| 43 | `bevel-chamfer == 0` | 0 | 🟢 |
| 48 | `bevel-chamfer == 1` | 1 | 🟢 |
| 49 | `bevel-pinned == 0` | 0 | 🟢 |
| 53 | `bevel-active == 0` | 0 | 🟢 |
| 54 | `faces == 11` | 11 | 🟢 |
| 55 | `steps == 8` | 8 | 🟢 |
| 56 | `undo == 5` | 5 | 🟢 |
| 66 | `bevel-pinned == 1` | 1 | 🟢 |
| 67 | `bevel-size ~ 0.3` | 0.3 | 🟢 |
| 70 | `bevel-active == 0` | 0 | 🟢 |
| 71 | `faces == 12` | 12 | 🟢 |
| 72 | `steps == 9` | 9 | 🟢 |
| 73 | `sound == 1` | 1 | 🟢 |
| 77 | `faces == 11` | 11 | 🟢 |
| 78 | `redo == 1` | 1 | 🟢 |
| 80 | `faces == 12` | 12 | 🟢 |
| 81 | `redo == 0` | 0 | 🟢 |
| 90 | `selected-edges == 1` | Edge(Extrude#2.Cap(Start)/Extrude#2.Side(Line2)) | 🟢 |
| 94 | `selected-edges == 2` | Edge(Extrude#2.Cap(Start)/Extrude#2.Side(Line2)),Edge(Extrude#2.Cap(End)/Extrude#2.Side... | 🟢 |
| 96 | `selected-edges == 1` | Edge(Extrude#2.Cap(Start)/Extrude#2.Side(Line2)) | 🟢 |
| 101 | `selected-edges == 5` | Edge(Extrude#2.Cap(Start)/Extrude#2.Side(Line2)),Edge(Blend#9.Strip(Edge(Extrude#2.Cap(... | 🟢 |
| 107 | `focused-step == Blend#9` | Blend#9 | 🟢 |
| 110 | `steps == 8` | 8 | 🟢 |
| 111 | `faces == 11` | 11 | 🟢 |
| 112 | `focused-step != Blend#9` |  | 🟢 |
| 117 | `camera-target-x != 2` | 1.422371101 | 🟢 |
| 119 | `camera-target-x ~ 2` | 2 | 🟢 |
| 120 | `camera-target-z ~ 0.875` | 0.875 | 🟢 |
| 122 | `camera-pitch ~ 1.5707963268` | 1.570796327 | 🟢 |
| 123 | `grid-plane == 0` | 0 | 🟢 |
| 125 | `grid-plane == 1` | 1 | 🟢 |
| 127 | `grid-plane == 2` | 2 | 🟢 |
| 129 | `orthographic == 1` | 1 | 🟢 |
| 131 | `orthographic == 0` | 0 | 🟢 |
| 135 | `surfaces == 0` | 0 | 🟢 |
| 137 | `surfaces == 1` | 1 | 🟢 |
| 146 | `selected-faces contains Extrude#4.Cap(End)` | Extrude#4.Cap(End) | 🟢 |
| 149 | `bevel-active == 1` | 1 | 🟢 |
| 150 | `bevel-size ~ 0.5` | 0.5 | 🟢 |
| 153 | `bevel-active == 0` | 0 | 🟢 |
| 154 | `steps == 9` | 9 | 🟢 |
| 155 | `refused == 0` | 0 | 🟢 |
| 156 | `sound == 1` | 1 | 🟢 |
| 157 | `volume > 10.60` | 10.61547283 | 🟢 |
| 158 | `volume < 10.63` | 10.61547283 | 🟢 |
| 166 | `selected-faces == 1` | Extrude#4.Cap(End) | 🟢 |
| 169 | `steps == 10` | 10 | 🟢 |
| 170 | `volume > 10.99` | 11.00817191 | 🟢 |
| 171 | `volume < 11.02` | 11.00817191 | 🟢 |
| 176 | `construct-open == 1` | 1 | 🟢 |
| 179 | `construct-combination == 2` | 2 | 🟢 |
| 182 | `construct-open == 0` | 0 | 🟢 |
| 183 | `steps == 10` | 10 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 19BevelSignChamfer | 64 | `e9532bc20a75faf9` |
| 21TangentChain | 144 | `9b57d08577a87410` |
| 20ShortcutExtrude | 224 | `3ee60081793df5e0` |

## Journal

- Selected the body (11 faces)
- Selection cleared
- Selected Edge(Extrude#2.Cap(Start)|Extrude#2.Side(Line1))
- Fillet (B): move horizontally · D fillet · C chamfer · V variable size · F flip its anchor · click edits the edges · Enter / right-click applies · Esc aborts
- Bevel cancelled (document untouched)
- Fillet (B): move horizontally · D fillet · C chamfer · V variable size · F flip its anchor · click edits the edges · Enter / right-click applies · Esc aborts
- Bevel size 0.3 m (pinned)
- Fillet 0.3 on 1 edge(s) · 358 ms
- Undo
- Redo
- Selection cleared
- Selected Edge(Extrude#2.Cap(Start)|Extrude#2.Side(Line2))
- Added Edge(Extrude#2.Cap(End)|Extrude#2.Side(Line3))
- Removed Edge(Extrude#2.Cap(End)|Extrude#2.Side(Line3))
- Selection cleared
- Autosaved _ClaudeScratch/DraftingWorkspace/ShortcutRecovery.drafting.toml
- Extended along the tangent chain: 5 edge(s)
- Selection cleared
- Delete Fillet2 · 260 ms
- Selection cleared
- Framed the document (Space)
- X-ray on (edges only)
- X-ray off
- Autosaved _ClaudeScratch/DraftingWorkspace/ShortcutRecovery.drafting.toml
- View Isometric
- Selected Extrude#4.Cap(End)
- Extrude (E): move horizontally · click edits the faces · Enter / right-click applies · Esc aborts
- ExtrudeFaces 0.5 on 1 face(s) · 310 ms
- Selection cleared
- Selected Extrude#4.Cap(End)
- ExtrudeFaces 0.5 on 1 face(s) · 374 ms
- Selection cleared
