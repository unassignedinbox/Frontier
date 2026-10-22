# DraftingWorkspaceCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0
- Frame: 1600 x 900 · 256 frames · 25.5216 s · slowest frame 388.263 ms
- Script: /home/user/Frontier/Experimental/DraftingWorkspace/Scripts/ShortcutWalkthrough.txt
- Expectations: 69 / 69 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 7 | `faces == 11` | 11 | 🟢 |
| 8 | `steps == 8` | 8 | 🟢 |
| 10 | `pick-mode == 2` | 2 | 🟢 |
| 12 | `pick-mode == 3` | 3 | 🟢 |
| 14 | `pick-mode == 4` | 4 | 🟢 |
| 18 | `selected-bodies == 1` | Extrude#2 | 🟢 |
| 19 | `selected-faces == 0` |  | 🟢 |
| 21 | `selected-bodies == 0` |  | 🟢 |
| 23 | `pick-mode == 5` | 5 | 🟢 |
| 25 | `pick-mode == 1` | 1 | 🟢 |
| 27 | `pick-mode == 5` | 5 | 🟢 |
| 35 | `selected-edges == 1` | Edge(Extrude#2.Cap(Start)/Extrude#2.Side(Line1)) | 🟢 |
| 36 | `undo == 5` | 5 | 🟢 |
| 39 | `bevel-active == 1` | 1 | 🟢 |
| 40 | `bevel-chamfer == 0` | 0 | 🟢 |
| 42 | `bevel-chamfer == 1` | 1 | 🟢 |
| 44 | `bevel-chamfer == 0` | 0 | 🟢 |
| 49 | `bevel-chamfer == 1` | 1 | 🟢 |
| 50 | `bevel-pinned == 0` | 0 | 🟢 |
| 54 | `bevel-active == 0` | 0 | 🟢 |
| 55 | `faces == 11` | 11 | 🟢 |
| 56 | `steps == 8` | 8 | 🟢 |
| 57 | `undo == 5` | 5 | 🟢 |
| 67 | `bevel-pinned == 1` | 1 | 🟢 |
| 68 | `bevel-size ~ 0.3` | 0.3 | 🟢 |
| 71 | `bevel-active == 0` | 0 | 🟢 |
| 72 | `faces == 12` | 12 | 🟢 |
| 73 | `steps == 9` | 9 | 🟢 |
| 74 | `sound == 1` | 1 | 🟢 |
| 78 | `faces == 11` | 11 | 🟢 |
| 79 | `redo == 1` | 1 | 🟢 |
| 81 | `faces == 12` | 12 | 🟢 |
| 82 | `redo == 0` | 0 | 🟢 |
| 91 | `selected-edges == 1` | Edge(Extrude#2.Cap(Start)/Extrude#2.Side(Line2)) | 🟢 |
| 95 | `selected-edges == 2` | Edge(Extrude#2.Cap(Start)/Extrude#2.Side(Line2)),Edge(Extrude#2.Cap(End)/Extrude#2.Side... | 🟢 |
| 97 | `selected-edges == 1` | Edge(Extrude#2.Cap(Start)/Extrude#2.Side(Line2)) | 🟢 |
| 102 | `selected-edges == 5` | Edge(Extrude#2.Cap(Start)/Extrude#2.Side(Line2)),Edge(Blend#9.Strip(Edge(Extrude#2.Cap(... | 🟢 |
| 108 | `focused-step == Blend#9` | Blend#9 | 🟢 |
| 111 | `steps == 8` | 8 | 🟢 |
| 112 | `faces == 11` | 11 | 🟢 |
| 113 | `focused-step != Blend#9` |  | 🟢 |
| 118 | `camera-target-x != 2` | 1.422371101 | 🟢 |
| 120 | `camera-target-x ~ 2` | 2 | 🟢 |
| 121 | `camera-target-z ~ 0.875` | 0.875 | 🟢 |
| 123 | `camera-pitch ~ 1.5707963268` | 1.570796327 | 🟢 |
| 124 | `grid-plane == 0` | 0 | 🟢 |
| 126 | `grid-plane == 1` | 1 | 🟢 |
| 128 | `grid-plane == 2` | 2 | 🟢 |
| 130 | `orthographic == 1` | 1 | 🟢 |
| 132 | `orthographic == 0` | 0 | 🟢 |
| 136 | `surfaces == 0` | 0 | 🟢 |
| 138 | `surfaces == 1` | 1 | 🟢 |
| 147 | `selected-faces contains Extrude#4.Cap(End)` | Extrude#4.Cap(End) | 🟢 |
| 150 | `bevel-active == 1` | 1 | 🟢 |
| 151 | `bevel-size ~ 0.5` | 0.5 | 🟢 |
| 154 | `bevel-active == 0` | 0 | 🟢 |
| 155 | `steps == 9` | 9 | 🟢 |
| 156 | `refused == 0` | 0 | 🟢 |
| 157 | `sound == 1` | 1 | 🟢 |
| 158 | `volume > 10.60` | 10.61547283 | 🟢 |
| 159 | `volume < 10.63` | 10.61547283 | 🟢 |
| 167 | `selected-faces == 1` | Extrude#4.Cap(End) | 🟢 |
| 170 | `steps == 10` | 10 | 🟢 |
| 171 | `volume > 10.99` | 11.00817191 | 🟢 |
| 172 | `volume < 11.02` | 11.00817191 | 🟢 |
| 177 | `construct-open == 1` | 1 | 🟢 |
| 180 | `construct-combination == 2` | 2 | 🟢 |
| 183 | `construct-open == 0` | 0 | 🟢 |
| 184 | `steps == 10` | 10 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 19BevelSignChamfer | 64 | `33a9a5043c341eed` |
| 21TangentChain | 144 | `698a17d297b1d25f` |
| 20ShortcutExtrude | 224 | `e425ed671dd52b8b` |

## Journal

- Selected Block1 (11 faces)
- Selection cleared
- Selected Edge(Extrude#2.Cap(Start)|Extrude#2.Side(Line1))
- Fillet (B): move horizontally · D fillet · C chamfer · V variable size · F flip its anchor · click edits the edges · Enter / right-click applies · Esc aborts
- Bevel cancelled (document untouched)
- Fillet (B): move horizontally · D fillet · C chamfer · V variable size · F flip its anchor · click edits the edges · Enter / right-click applies · Esc aborts
- Bevel size 0.3 m (pinned)
- Fillet 0.3 on 1 edge(s) · 134 ms
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
- Delete Fillet2 · 44 ms
- Selection cleared
- Framed the document (Space)
- X-ray on (edges only)
- X-ray off
- Autosaved _ClaudeScratch/DraftingWorkspace/ShortcutRecovery.drafting.toml
- View Isometric
- Selected Extrude#4.Cap(End)
- Extrude (E): move horizontally · click edits the faces · Enter / right-click applies · Esc aborts
- ExtrudeFaces 0.5 on 1 face(s) · 93 ms
- Selection cleared
- Selected Extrude#4.Cap(End)
- ExtrudeFaces 0.5 on 1 face(s) · 100 ms
- Selection cleared
