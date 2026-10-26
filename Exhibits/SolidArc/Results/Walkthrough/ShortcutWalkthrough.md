# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 900 · 263 frames · 35.0659 s · slowest frame 575.609 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/ShortcutWalkthrough.txt
- Expectations: 73 / 73 passed

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
| 118 | `camera-target-x != 2` | 1.286515912 | 🟢 |
| 120 | `camera-target-x ~ 2` | 2 | 🟢 |
| 121 | `camera-target-z ~ 0.875` | 0.875 | 🟢 |
| 123 | `camera-pitch ~ 1.5707963268` | 1.570796327 | 🟢 |
| 124 | `grid-plane == 0` | 0 | 🟢 |
| 126 | `grid-plane == 1` | 1 | 🟢 |
| 128 | `grid-plane == 2` | 2 | 🟢 |
| 130 | `orthographic == 1` | 1 | 🟢 |
| 132 | `orthographic == 0` | 0 | 🟢 |
| 136 | `xray == 1` | 1 | 🟢 |
| 137 | `surfaces == 1` | 1 | 🟢 |
| 139 | `xray == 0` | 0 | 🟢 |
| 150 | `selected-faces contains Extrude#4.Cap(End)` | Extrude#4.Cap(End) | 🟢 |
| 153 | `bevel-active == 1` | 1 | 🟢 |
| 154 | `bevel-size ~ 0.5` | 0.5 | 🟢 |
| 157 | `bevel-active == 0` | 0 | 🟢 |
| 158 | `steps == 9` | 9 | 🟢 |
| 159 | `refused == 0` | 0 | 🟢 |
| 160 | `sound == 1` | 1 | 🟢 |
| 161 | `volume > 10.60` | 10.61547283 | 🟢 |
| 162 | `volume < 10.63` | 10.61547283 | 🟢 |
| 170 | `selected-faces == 1` | Extrude#4.Cap(End) | 🟢 |
| 173 | `steps == 10` | 10 | 🟢 |
| 174 | `volume > 10.99` | 11.00817191 | 🟢 |
| 175 | `volume < 11.02` | 11.00817191 | 🟢 |
| 180 | `drawing-solid == 1` | 1 | 🟢 |
| 181 | `drawing-solid-kind == 1` | Cylinder | 🟢 |
| 182 | `construct-combination == 4` | 4 | 🟢 |
| 185 | `construct-combination == 2` | 2 | 🟢 |
| 188 | `drawing-solid == 0` | 0 | 🟢 |
| 189 | `construct-open == 0` | 0 | 🟢 |
| 190 | `steps == 10` | 10 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 19BevelSignChamfer | 64 | `d6f2f55bd59d9075` |
| 21TangentChain | 144 | `86679ef35b128818` |
| 20ShortcutExtrude | 231 | `1f50f6802aefd011` |

## Journal

- Selected Block1 (11 faces)
- Selection cleared
- Selected Edge(Extrude#2.Cap(Start)|Extrude#2.Side(Line1))
- Fillet (B): move horizontally · D fillet · C chamfer · V variable size · F flip its anchor · click edits the edges · Enter / right-click applies · Esc aborts
- Bevel cancelled (document untouched)
- Fillet (B): move horizontally · D fillet · C chamfer · V variable size · F flip its anchor · click edits the edges · Enter / right-click applies · Esc aborts
- Bevel size 0.3 m (pinned)
- Fillet 0.3 on 1 edge(s) · 143 ms
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
- ExtrudeFaces 0.5 on 1 face(s) · 96 ms
- Selection cleared
- Selected Extrude#4.Cap(End)
- ExtrudeFaces 0.5 on 1 face(s) · 99 ms
- Cylinder: click the base centre · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Cylinder: Difference
- Cylinder aborted (document untouched)
