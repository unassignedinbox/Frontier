# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 810 · 330 frames · 31.5127 s · slowest frame 466.737 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/BevelReachWalkthrough.txt
- Expectations: 32 / 32 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 35 | `volume ~ 8` | 8 | 🟢 |
| 42 | `bevel-active == 1` | 1 | 🟢 |
| 43 | `bevel-span ~ 2` | 2 | 🟢 |
| 51 | `bevel-size ~ 2` | 2 | 🟢 |
| 52 | `bevel-refusal == 0` |  | 🟢 |
| 53 | `bevel-preview-volume ~ 6.283185307179586` | 6.283185307 | 🟢 |
| 59 | `bevel-chamfer == 1` | 1 | 🟢 |
| 60 | `bevel-size ~ 2` | 2 | 🟢 |
| 61 | `bevel-refusal == 0` |  | 🟢 |
| 62 | `bevel-preview-volume ~ 4` | 4 | 🟢 |
| 71 | `bevel-size ~ 3` | 3 | 🟢 |
| 72 | `bevel-reach > 3` | 3.464101615 | 🟢 |
| 73 | `bevel-refusal == 0` |  | 🟢 |
| 74 | `bevel-preview-volume ~ 1` | 1 | 🟢 |
| 80 | `bevel-size ~ 3` | 3 | 🟢 |
| 81 | `bevel-refusal == 0` |  | 🟢 |
| 82 | `bevel-preview-volume ~ 4.363248321487495` | 4.363248321 | 🟢 |
| 88 | `bevel-size > 3` | 3.464101615 | 🟢 |
| 89 | `bevel-refusal == 0` |  | 🟢 |
| 92 | `bevel-active == 0` | 0 | 🟢 |
| 109 | `bevel-active == 0` | 0 | 🟢 |
| 110 | `refused == 0` | 0 | 🟢 |
| 111 | `volume ~ 7` | 7 | 🟢 |
| 114 | `selected-edges == 1` | Edge(Blend#3.Strip(Edge(Extrude#2.Cap(End)/Extrude#2.Side(Line1)))/Extrude#2.Cap(End)) | 🟢 |
| 124 | `bevel-size ~ 2.4` | 2.4 | 🟢 |
| 125 | `bevel-span ~ 1` | 1 | 🟢 |
| 126 | `bevel-refusal == 0` |  | 🟢 |
| 127 | `bevel-preview-volume ~ 6.752153182631247` | 6.752153183 | 🟢 |
| 131 | `bevel-active == 0` | 0 | 🟢 |
| 132 | `refused == 0` | 0 | 🟢 |
| 133 | `volume ~ 6.752153182631247` | 6.752153183 | 🟢 |
| 134 | `sound == 1` | 1 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 363BevelReachFillet | 101 | `1afd8275b948a874` |
| 364BevelReachChamfer | 118 | `9eb0d3e305e5aa4b` |
| 365BevelReachChamferPast | 141 | `ad175ac8cdb23b4e` |
| 366BevelReachFilletPast | 158 | `29774507a1dbee9c` |
| 367BevelReachObtuse | 319 | `f06dd7adc04c8c95` |
| 368BevelReachApplied | 330 | `1e6f2683289a4e02` |

## Journal

- New document (previous recovery copy kept)
- Block: click the first corner · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Block: click the opposite corner
- Block: move up, click the height
- Block1 · New body · 0 ms
- Selected Edge(Extrude#2.Cap(End)|Extrude#2.Side(Line1))
- Fillet (B): drag anywhere or the ring handle to size (it stops at the largest that fits) · drag a corner diamond for a variable size · type in the card · D fillet · C chamfer · V taper · Alt+click an edge loop, Ctrl+Alt+click a ring (Shift adds) · Enter / right-click applies · Esc aborts
- Bevel size 2 m (pinned)
- Bevel size 3 m (pinned)
- Bevel cancelled (document untouched)
- Selected Edge(Extrude#2.Cap(End)|Extrude#2.Side(Line1))
- Fillet (B): drag anywhere or the ring handle to size (it stops at the largest that fits) · drag a corner diamond for a variable size · type in the card · D fillet · C chamfer · V taper · Alt+click an edge loop, Ctrl+Alt+click a ring (Shift adds) · Enter / right-click applies · Esc aborts · starts at 0.2 m to fit the edges
- Bevel size 1 m (pinned)
- Chamfer 1 on 1 edge(s) · 1 ms
- Selected Edge(Blend#3.Strip(Edge(Extrude#2.Cap(End)|Extrude#2.Side(Line1)))|Extrude#2.Cap(End))
- Fillet (B): drag anywhere or the ring handle to size (it stops at the largest that fits) · drag a corner diamond for a variable size · type in the card · D fillet · C chamfer · V taper · Alt+click an edge loop, Ctrl+Alt+click a ring (Shift adds) · Enter / right-click applies · Esc aborts · starts at 0.1 m to fit the edges
- Bevel size 2.4 m (pinned)
- Fillet 2.4 on 1 edge(s) · 1 ms
