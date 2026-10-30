# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 810 · 301 frames · 25.351 s · slowest frame 447.057 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/BevelMergeWalkthrough.txt
- Expectations: 28 / 28 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 32 | `volume ~ 8` | 8 | 🟢 |
| 43 | `selected-edges == 4` | Edge(Extrude#2.Cap(End)/Extrude#2.Side(Line1)),Edge(Extrude#2.Cap(End)/Extrude#2.Side(L... | 🟢 |
| 46 | `bevel-active == 1` | 1 | 🟢 |
| 47 | `bevel-merges == 2` | 2 | 🟢 |
| 48 | `bevel-merge-first ~ 1` | 1 | 🟢 |
| 56 | `bevel-size ~ 0.98` | 0.98 | 🟢 |
| 57 | `bevel-refusal == 0` |  | 🟢 |
| 63 | `bevel-merge-snaps > 0` | 3 | 🟢 |
| 64 | `bevel-size ~ 1` | 1 | 🟢 |
| 65 | `bevel-refusal == 0` |  | 🟢 |
| 66 | `bevel-preview-volume ~ 6.666666666666667` | 6.666666667 | 🟢 |
| 72 | `bevel-chamfer == 1` | 1 | 🟢 |
| 73 | `bevel-size ~ 1` | 1 | 🟢 |
| 74 | `bevel-refusal == 0` |  | 🟢 |
| 75 | `bevel-preview-volume ~ 5.333333333333333` | 5.333333333 | 🟢 |
| 79 | `refused == 0` | 0 | 🟢 |
| 80 | `sound == 1` | 1 | 🟢 |
| 81 | `volume ~ 5.333333333333333` | 5.333333333 | 🟢 |
| 108 | `selected-edges == 2` | Edge(Extrude#2.Cap(End)/Extrude#2.Side(Line1)),Edge(Extrude#2.Cap(End)/Extrude#2.Side(L... | 🟢 |
| 116 | `bevel-size ~ 1` | 1 | 🟢 |
| 117 | `bevel-refusal == 0` |  | 🟢 |
| 118 | `bevel-preview-volume ~ 7.141592653589793` | 7.141592654 | 🟢 |
| 122 | `bevel-size ~ 1` | 1 | 🟢 |
| 123 | `bevel-refusal == 0` |  | 🟢 |
| 124 | `bevel-preview-volume ~ 6` | 6 | 🟢 |
| 128 | `refused == 0` | 0 | 🟢 |
| 129 | `sound == 1` | 1 | 🟢 |
| 130 | `volume ~ 6` | 6 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 373BevelMergeNear | 122 | `c4955773976628cf` |
| 374BevelMergeApexFillet | 141 | `9bfe134b153d8e53` |
| 375BevelMergeApexChamfer | 158 | `4c2189948c792e8c` |
| 376BevelMergeRidgeFillet | 274 | `f577077135de2f23` |
| 377BevelMergeRidgeChamfer | 291 | `6e670799abc522ed` |

## Journal

- New document (previous recovery copy kept)
- Block: click the first corner · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Block: click the opposite corner
- Block: move up, click the height
- Block1 · New body · 0 ms
- Selected Edge(Extrude#2.Cap(End)|Extrude#2.Side(Line1))
- Added Edge(Extrude#2.Cap(End)|Extrude#2.Side(Line2))
- Added Edge(Extrude#2.Cap(End)|Extrude#2.Side(Line3))
- Added Edge(Extrude#2.Cap(End)|Extrude#2.Side(Line4))
- Fillet (B): drag anywhere or the ring handle to size (it stops at the largest that fits) · drag a corner diamond for a variable size · type in the card · D fillet · C chamfer · V taper · Alt+click an edge loop, Ctrl+Alt+click a ring (Shift adds) · Enter / right-click applies · Esc aborts
- Bevel size 0.98 m (pinned)
- Bevel size 1 m (handle)
- Chamfer 1 on 4 edge(s) · 1 ms
- New document (previous recovery copy kept)
- Block: click the first corner · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Block: click the opposite corner
- Autosaved _ClaudeScratch/SolidArcWorkspace/BevelMergeRecovery.solidarc
- Block: move up, click the height
- Block1 · New body · 0 ms
- Selected Edge(Extrude#2.Cap(End)|Extrude#2.Side(Line1))
- Added Edge(Extrude#2.Cap(End)|Extrude#2.Side(Line3))
- Fillet (B): drag anywhere or the ring handle to size (it stops at the largest that fits) · drag a corner diamond for a variable size · type in the card · D fillet · C chamfer · V taper · Alt+click an edge loop, Ctrl+Alt+click a ring (Shift adds) · Enter / right-click applies · Esc aborts
- Bevel size 1 m (pinned)
- Chamfer 1 on 2 edge(s) · 1 ms
