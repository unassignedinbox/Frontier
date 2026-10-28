# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 810 · 294 frames · 25.9421 s · slowest frame 334.349 ms
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
| 109 | `selected-edges == 5` | Edge(Extrude#2.Cap(Start)/Extrude#2.Side(Line2)),Edge(Extrude#2.Side(Line2)/Extrude#2.S... | 🟢 |
| 120 | `focused-step == Blend#9` | Blend#9 | 🟢 |
| 123 | `steps == 8` | 8 | 🟢 |
| 124 | `faces == 11` | 11 | 🟢 |
| 125 | `focused-step != Blend#9` |  | 🟢 |
| 130 | `camera-target-x != 2` | 1.416778567 | 🟢 |
| 132 | `camera-target-x ~ 2` | 2 | 🟢 |
| 133 | `camera-target-z ~ 0.875` | 0.875 | 🟢 |
| 135 | `camera-pitch ~ 1.5707963268` | 1.570796327 | 🟢 |
| 136 | `grid-plane == 0` | 0 | 🟢 |
| 138 | `grid-plane == 1` | 1 | 🟢 |
| 140 | `grid-plane == 2` | 2 | 🟢 |
| 142 | `orthographic == 1` | 1 | 🟢 |
| 144 | `orthographic == 0` | 0 | 🟢 |
| 148 | `xray == 1` | 1 | 🟢 |
| 149 | `surfaces == 1` | 1 | 🟢 |
| 151 | `xray == 0` | 0 | 🟢 |
| 162 | `selected-faces contains Extrude#4.Cap(End)` | Extrude#4.Cap(End) | 🟢 |
| 165 | `bevel-active == 1` | 1 | 🟢 |
| 166 | `bevel-size ~ 0.5` | 0.5 | 🟢 |
| 169 | `bevel-active == 0` | 0 | 🟢 |
| 170 | `steps == 9` | 9 | 🟢 |
| 171 | `refused == 0` | 0 | 🟢 |
| 172 | `sound == 1` | 1 | 🟢 |
| 173 | `volume > 10.60` | 10.61547283 | 🟢 |
| 174 | `volume < 10.63` | 10.61547283 | 🟢 |
| 182 | `selected-faces == 1` | Extrude#4.Cap(End) | 🟢 |
| 185 | `steps == 10` | 10 | 🟢 |
| 186 | `volume > 10.99` | 11.00817191 | 🟢 |
| 187 | `volume < 11.02` | 11.00817191 | 🟢 |
| 192 | `drawing-solid == 1` | 1 | 🟢 |
| 193 | `drawing-solid-kind == 1` | Cylinder | 🟢 |
| 194 | `construct-combination == 4` | 4 | 🟢 |
| 197 | `construct-combination == 2` | 2 | 🟢 |
| 200 | `drawing-solid == 0` | 0 | 🟢 |
| 201 | `construct-open == 0` | 0 | 🟢 |
| 202 | `steps == 10` | 10 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 19BevelSizeFloor | 71 | `90631aa3f3ce1bb4` |
| 21TangentChain | 151 | `8a81ba7bb396dbf3` |
| 20ShortcutExtrude | 262 | `d10760078e99b7e8` |

## Journal

- Selected Block1 (11 faces)
- Selection cleared
- Selected Edge(Extrude#2.Cap(Start)|Extrude#2.Side(Line1))
- Fillet (B): drag anywhere or the ring handle to size (it stops at the largest that fits) · drag a corner diamond for a variable size · type in the card · D fillet · C chamfer · V taper · Alt+click an edge loop, Ctrl+Alt+click a ring (Shift adds) · Enter / right-click applies · Esc aborts
- Bevel cancelled (document untouched)
- Fillet (B): drag anywhere or the ring handle to size (it stops at the largest that fits) · drag a corner diamond for a variable size · type in the card · D fillet · C chamfer · V taper · Alt+click an edge loop, Ctrl+Alt+click a ring (Shift adds) · Enter / right-click applies · Esc aborts
- Bevel size 0.3 m (pinned)
- Fillet 0.3 on 1 edge(s) · 2 ms
- Undo
- Redo
- Selection cleared
- Selected Edge(Extrude#2.Cap(Start)|Extrude#2.Side(Line2))
- Added Edge(Extrude#2.Cap(End)|Extrude#2.Side(Line3))
- Removed Edge(Extrude#2.Cap(End)|Extrude#2.Side(Line3))
- Selection cleared
- Autosaved _ClaudeScratch/SolidArcWorkspace/ShortcutRecovery.solidarc
- Edge loop: 5 edge(s) added · Alt loop · Ctrl+Alt ring · Shift toggles · double-click tangent chain
- Selection cleared
- Delete Fillet2 · 1 ms
- Selection cleared
- Framed the document (Space)
- X-ray on · faces 45 % · hidden edges pickable
- X-ray off
- Autosaved _ClaudeScratch/SolidArcWorkspace/ShortcutRecovery.solidarc
- View Isometric
- Selected Extrude#4.Cap(End)
- Extrude (E): the face follows the pointer along its normal (Blender) · out adds, in pushes / cuts a pocket (extrude manifold) · click edits the faces · Enter / right-click applies · Esc aborts
- ExtrudeFaces 0.5 on 1 face(s) · 2 ms
- Selection cleared
- Selected Extrude#4.Cap(End)
- ExtrudeFaces 0.5 on 1 face(s) · 2 ms
- Cylinder: click the base centre · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Cylinder: Difference
- Cylinder aborted (document untouched)
