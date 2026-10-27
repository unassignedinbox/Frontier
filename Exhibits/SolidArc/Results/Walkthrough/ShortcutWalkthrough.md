# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 900 · 270 frames · 35.3739 s · slowest frame 539.335 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/ShortcutWalkthrough.txt
- Expectations: 75 / 75 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 10 | `faces == 11` | 11 | 🟢 |
| 11 | `steps == 8` | 8 | 🟢 |
| 13 | `pick-mode == 2` | 2 | 🟢 |
| 15 | `pick-mode == 3` | 3 | 🟢 |
| 17 | `pick-mode == 4` | 4 | 🟢 |
| 21 | `selected-bodies == 1` | Extrude#2 | 🟢 |
| 22 | `selected-faces == 0` |  | 🟢 |
| 24 | `selected-bodies == 0` |  | 🟢 |
| 26 | `pick-mode == 5` | 5 | 🟢 |
| 28 | `pick-mode == 1` | 1 | 🟢 |
| 30 | `pick-mode == 5` | 5 | 🟢 |
| 38 | `selected-edges == 1` | Edge(Extrude#2.Cap(Start)/Extrude#2.Side(Line1)) | 🟢 |
| 39 | `undo == 5` | 5 | 🟢 |
| 42 | `bevel-active == 1` | 1 | 🟢 |
| 43 | `bevel-chamfer == 0` | 0 | 🟢 |
| 45 | `bevel-chamfer == 1` | 1 | 🟢 |
| 47 | `bevel-chamfer == 0` | 0 | 🟢 |
| 48 | `bevel-pinned == 1` | 1 | 🟢 |
| 55 | `bevel-chamfer == 0` | 0 | 🟢 |
| 56 | `bevel-size ~ 0.001` | 0.001 | 🟢 |
| 57 | `bevel-pinned == 0` | 0 | 🟢 |
| 61 | `bevel-active == 0` | 0 | 🟢 |
| 62 | `faces == 11` | 11 | 🟢 |
| 63 | `steps == 8` | 8 | 🟢 |
| 64 | `undo == 5` | 5 | 🟢 |
| 74 | `bevel-pinned == 1` | 1 | 🟢 |
| 75 | `bevel-size ~ 0.3` | 0.3 | 🟢 |
| 78 | `bevel-active == 0` | 0 | 🟢 |
| 79 | `faces == 12` | 12 | 🟢 |
| 80 | `steps == 9` | 9 | 🟢 |
| 81 | `sound == 1` | 1 | 🟢 |
| 85 | `faces == 11` | 11 | 🟢 |
| 86 | `redo == 1` | 1 | 🟢 |
| 88 | `faces == 12` | 12 | 🟢 |
| 89 | `redo == 0` | 0 | 🟢 |
| 98 | `selected-edges == 1` | Edge(Extrude#2.Cap(Start)/Extrude#2.Side(Line2)) | 🟢 |
| 102 | `selected-edges == 2` | Edge(Extrude#2.Cap(Start)/Extrude#2.Side(Line2)),Edge(Extrude#2.Cap(End)/Extrude#2.Side... | 🟢 |
| 104 | `selected-edges == 1` | Edge(Extrude#2.Cap(Start)/Extrude#2.Side(Line2)) | 🟢 |
| 109 | `selected-edges == 5` | Edge(Extrude#2.Cap(Start)/Extrude#2.Side(Line2)),Edge(Blend#9.Strip(Edge(Extrude#2.Cap(... | 🟢 |
| 115 | `focused-step == Blend#9` | Blend#9 | 🟢 |
| 118 | `steps == 8` | 8 | 🟢 |
| 119 | `faces == 11` | 11 | 🟢 |
| 120 | `focused-step != Blend#9` |  | 🟢 |
| 125 | `camera-target-x != 2` | 1.286515912 | 🟢 |
| 127 | `camera-target-x ~ 2` | 2 | 🟢 |
| 128 | `camera-target-z ~ 0.875` | 0.875 | 🟢 |
| 130 | `camera-pitch ~ 1.5707963268` | 1.570796327 | 🟢 |
| 131 | `grid-plane == 0` | 0 | 🟢 |
| 133 | `grid-plane == 1` | 1 | 🟢 |
| 135 | `grid-plane == 2` | 2 | 🟢 |
| 137 | `orthographic == 1` | 1 | 🟢 |
| 139 | `orthographic == 0` | 0 | 🟢 |
| 143 | `xray == 1` | 1 | 🟢 |
| 144 | `surfaces == 1` | 1 | 🟢 |
| 146 | `xray == 0` | 0 | 🟢 |
| 157 | `selected-faces contains Extrude#4.Cap(End)` | Extrude#4.Cap(End) | 🟢 |
| 160 | `bevel-active == 1` | 1 | 🟢 |
| 161 | `bevel-size ~ 0.5` | 0.5 | 🟢 |
| 164 | `bevel-active == 0` | 0 | 🟢 |
| 165 | `steps == 9` | 9 | 🟢 |
| 166 | `refused == 0` | 0 | 🟢 |
| 167 | `sound == 1` | 1 | 🟢 |
| 168 | `volume > 10.60` | 10.61547283 | 🟢 |
| 169 | `volume < 10.63` | 10.61547283 | 🟢 |
| 177 | `selected-faces == 1` | Extrude#4.Cap(End) | 🟢 |
| 180 | `steps == 10` | 10 | 🟢 |
| 181 | `volume > 10.99` | 11.00817191 | 🟢 |
| 182 | `volume < 11.02` | 11.00817191 | 🟢 |
| 187 | `drawing-solid == 1` | 1 | 🟢 |
| 188 | `drawing-solid-kind == 1` | Cylinder | 🟢 |
| 189 | `construct-combination == 4` | 4 | 🟢 |
| 192 | `construct-combination == 2` | 2 | 🟢 |
| 195 | `drawing-solid == 0` | 0 | 🟢 |
| 196 | `construct-open == 0` | 0 | 🟢 |
| 197 | `steps == 10` | 10 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 19BevelSizeFloor | 71 | `00c6cdef60dedb29` |
| 21TangentChain | 151 | `605fb0307a261923` |
| 20ShortcutExtrude | 238 | `a159cfb2b4b57721` |

## Journal

- Selected Block1 (11 faces)
- Selection cleared
- Selected Edge(Extrude#2.Cap(Start)|Extrude#2.Side(Line1))
- Fillet (B): drag anywhere or the ring handle to size (it stops at the largest that fits) · drag a corner diamond for a variable size · type in the card · D fillet · C chamfer · V taper · Alt+click an edge adds its ring · Enter / right-click applies · Esc aborts
- Bevel cancelled (document untouched)
- Fillet (B): drag anywhere or the ring handle to size (it stops at the largest that fits) · drag a corner diamond for a variable size · type in the card · D fillet · C chamfer · V taper · Alt+click an edge adds its ring · Enter / right-click applies · Esc aborts
- Bevel size 0.3 m (pinned)
- Fillet 0.3 on 1 edge(s) · 4 ms
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
- Delete Fillet2 · 1 ms
- Selection cleared
- Framed the document (Space)
- X-ray on · faces 45 % · hidden edges pickable
- X-ray off
- Autosaved _ClaudeScratch/SolidArcWorkspace/ShortcutRecovery.solidarc
- View Isometric
- Selected Extrude#4.Cap(End)
- Extrude (E): move horizontally · click edits the faces · Enter / right-click applies · Esc aborts
- ExtrudeFaces 0.5 on 1 face(s) · 2 ms
- Selection cleared
- Selected Extrude#4.Cap(End)
- ExtrudeFaces 0.5 on 1 face(s) · 3 ms
- Cylinder: click the base centre · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Cylinder: Difference
- Cylinder aborted (document untouched)
