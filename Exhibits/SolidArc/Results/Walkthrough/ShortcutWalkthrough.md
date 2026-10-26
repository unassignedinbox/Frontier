# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 900 · 263 frames · 32.2495 s · slowest frame 505.928 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/ShortcutWalkthrough.txt
- Expectations: 74 / 74 passed

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
| 49 | `bevel-chamfer == 0` | 0 | 🟢 |
| 50 | `bevel-size ~ 0.001` | 0.001 | 🟢 |
| 51 | `bevel-pinned == 0` | 0 | 🟢 |
| 55 | `bevel-active == 0` | 0 | 🟢 |
| 56 | `faces == 11` | 11 | 🟢 |
| 57 | `steps == 8` | 8 | 🟢 |
| 58 | `undo == 5` | 5 | 🟢 |
| 68 | `bevel-pinned == 1` | 1 | 🟢 |
| 69 | `bevel-size ~ 0.3` | 0.3 | 🟢 |
| 72 | `bevel-active == 0` | 0 | 🟢 |
| 73 | `faces == 12` | 12 | 🟢 |
| 74 | `steps == 9` | 9 | 🟢 |
| 75 | `sound == 1` | 1 | 🟢 |
| 79 | `faces == 11` | 11 | 🟢 |
| 80 | `redo == 1` | 1 | 🟢 |
| 82 | `faces == 12` | 12 | 🟢 |
| 83 | `redo == 0` | 0 | 🟢 |
| 92 | `selected-edges == 1` | Edge(Extrude#2.Cap(Start)/Extrude#2.Side(Line2)) | 🟢 |
| 96 | `selected-edges == 2` | Edge(Extrude#2.Cap(Start)/Extrude#2.Side(Line2)),Edge(Extrude#2.Cap(End)/Extrude#2.Side... | 🟢 |
| 98 | `selected-edges == 1` | Edge(Extrude#2.Cap(Start)/Extrude#2.Side(Line2)) | 🟢 |
| 103 | `selected-edges == 5` | Edge(Extrude#2.Cap(Start)/Extrude#2.Side(Line2)),Edge(Blend#9.Strip(Edge(Extrude#2.Cap(... | 🟢 |
| 109 | `focused-step == Blend#9` | Blend#9 | 🟢 |
| 112 | `steps == 8` | 8 | 🟢 |
| 113 | `faces == 11` | 11 | 🟢 |
| 114 | `focused-step != Blend#9` |  | 🟢 |
| 119 | `camera-target-x != 2` | 1.286515912 | 🟢 |
| 121 | `camera-target-x ~ 2` | 2 | 🟢 |
| 122 | `camera-target-z ~ 0.875` | 0.875 | 🟢 |
| 124 | `camera-pitch ~ 1.5707963268` | 1.570796327 | 🟢 |
| 125 | `grid-plane == 0` | 0 | 🟢 |
| 127 | `grid-plane == 1` | 1 | 🟢 |
| 129 | `grid-plane == 2` | 2 | 🟢 |
| 131 | `orthographic == 1` | 1 | 🟢 |
| 133 | `orthographic == 0` | 0 | 🟢 |
| 137 | `xray == 1` | 1 | 🟢 |
| 138 | `surfaces == 1` | 1 | 🟢 |
| 140 | `xray == 0` | 0 | 🟢 |
| 151 | `selected-faces contains Extrude#4.Cap(End)` | Extrude#4.Cap(End) | 🟢 |
| 154 | `bevel-active == 1` | 1 | 🟢 |
| 155 | `bevel-size ~ 0.5` | 0.5 | 🟢 |
| 158 | `bevel-active == 0` | 0 | 🟢 |
| 159 | `steps == 9` | 9 | 🟢 |
| 160 | `refused == 0` | 0 | 🟢 |
| 161 | `sound == 1` | 1 | 🟢 |
| 162 | `volume > 10.60` | 10.61547283 | 🟢 |
| 163 | `volume < 10.63` | 10.61547283 | 🟢 |
| 171 | `selected-faces == 1` | Extrude#4.Cap(End) | 🟢 |
| 174 | `steps == 10` | 10 | 🟢 |
| 175 | `volume > 10.99` | 11.00817191 | 🟢 |
| 176 | `volume < 11.02` | 11.00817191 | 🟢 |
| 181 | `drawing-solid == 1` | 1 | 🟢 |
| 182 | `drawing-solid-kind == 1` | Cylinder | 🟢 |
| 183 | `construct-combination == 4` | 4 | 🟢 |
| 186 | `construct-combination == 2` | 2 | 🟢 |
| 189 | `drawing-solid == 0` | 0 | 🟢 |
| 190 | `construct-open == 0` | 0 | 🟢 |
| 191 | `steps == 10` | 10 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 19BevelSizeFloor | 64 | `32895ba6f303320e` |
| 21TangentChain | 144 | `3d0b6ca92b34694f` |
| 20ShortcutExtrude | 231 | `cbf6447caed0c3ef` |

## Journal

- Selected Block1 (11 faces)
- Selection cleared
- Selected Edge(Extrude#2.Cap(Start)|Extrude#2.Side(Line1))
- Fillet (B): drag the handles (corner handles make it variable) or drag / move anywhere · D fillet · C chamfer · V variable size · click an edge to add / remove it · Enter / right-click applies · Esc aborts
- Bevel cancelled (document untouched)
- Fillet (B): drag the handles (corner handles make it variable) or drag / move anywhere · D fillet · C chamfer · V variable size · click an edge to add / remove it · Enter / right-click applies · Esc aborts
- Bevel size 0.3 m (pinned)
- Fillet 0.3 on 1 edge(s) · 138 ms
- Undo
- Redo
- Selection cleared
- Selected Edge(Extrude#2.Cap(Start)|Extrude#2.Side(Line2))
- Added Edge(Extrude#2.Cap(End)|Extrude#2.Side(Line3))
- Removed Edge(Extrude#2.Cap(End)|Extrude#2.Side(Line3))
- Selection cleared
- Autosaved _ClaudeScratch/SolidArcWorkspace/ShortcutRecovery.solidarc
- Extended along the tangent chain: 5 edge(s)
- Selection cleared
- Delete Fillet2 · 45 ms
- Selection cleared
- Framed the document (Space)
- X-ray on · faces 45 % · hidden edges pickable
- X-ray off
- Autosaved _ClaudeScratch/SolidArcWorkspace/ShortcutRecovery.solidarc
- View Isometric
- Selected Extrude#4.Cap(End)
- Extrude (E): move horizontally · click edits the faces · Enter / right-click applies · Esc aborts
- ExtrudeFaces 0.5 on 1 face(s) · 95 ms
- Selection cleared
- Selected Extrude#4.Cap(End)
- ExtrudeFaces 0.5 on 1 face(s) · 99 ms
- Cylinder: click the base centre · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Cylinder: Difference
- Cylinder aborted (document untouched)
