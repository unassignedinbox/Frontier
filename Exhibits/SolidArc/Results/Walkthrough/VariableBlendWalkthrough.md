# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 810 · 105 frames · 9.20369 s · slowest frame 340.411 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/VariableBlendWalkthrough.txt
- Expectations: 31 / 31 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 14 | `faces == 11` | 11 | 🟢 |
| 15 | `steps == 8` | 8 | 🟢 |
| 20 | `selected-edges == 1` | Edge(Extrude#2.Cap(Start)/Extrude#2.Side(Line1)) | 🟢 |
| 23 | `bevel-active == 1` | 1 | 🟢 |
| 24 | `bevel-variable == 0` | 0 | 🟢 |
| 30 | `bevel-size ~ 0.3` | 0.3 | 🟢 |
| 31 | `bevel-pinned == 1` | 1 | 🟢 |
| 37 | `bevel-variable == 1` | 1 | 🟢 |
| 38 | `bevel-end-size ~ 0.15` | 0.15 | 🟢 |
| 44 | `bevel-end-size ~ 0.1` | 0.1 | 🟢 |
| 45 | `bevel-refusal == 0` |  | 🟢 |
| 46 | `bevel-preview-volume ~ 10.185606734` | 10.18560673 | 🟢 |
| 51 | `bevel-active == 1` | 1 | 🟢 |
| 57 | `bevel-flipped == 1` | 1 | 🟢 |
| 58 | `bevel-refusal == 0` |  | 🟢 |
| 59 | `bevel-preview-volume ~ 10.185606734` | 10.18560673 | 🟢 |
| 65 | `bevel-chamfer == 1` | 1 | 🟢 |
| 66 | `bevel-refusal == 0` |  | 🟢 |
| 67 | `bevel-preview-volume ~ 10.136107080` | 10.13610708 | 🟢 |
| 71 | `bevel-chamfer == 0` | 0 | 🟢 |
| 76 | `bevel-active == 0` | 0 | 🟢 |
| 77 | `steps == 9` | 9 | 🟢 |
| 78 | `faces == 12` | 12 | 🟢 |
| 79 | `refused == 0` | 0 | 🟢 |
| 80 | `sound == 1` | 1 | 🟢 |
| 81 | `volume ~ 10.185606734` | 10.18560673 | 🟢 |
| 82 | `status contains 0.3 → 0.1` | Fillet 0.3 → 0.1 on 1 edge(s) · 7 ms | 🟢 |
| 87 | `faces == 11` | 11 | 🟢 |
| 88 | `steps == 8` | 8 | 🟢 |
| 90 | `faces == 12` | 12 | 🟢 |
| 91 | `volume ~ 10.185606734` | 10.18560673 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 22VariableFillet | 72 | `7808d956dd19c7ef` |
| 23VariableFlipped | 79 | `13bb84ed827368f1` |
| 24VariableChamfer | 86 | `067822115eeb4e77` |
| 25VariableApplied | 98 | `a089d475cf6d21c2` |

## Journal

- Selected Edge(Extrude#2.Cap(Start)|Extrude#2.Side(Line1))
- Fillet (B): drag anywhere or the ring handle to size (it stops at the largest that fits) · drag a corner diamond for a variable size · type in the card · D fillet · C chamfer · V taper · Alt+click an edge loop, Ctrl+Alt+click a ring (Shift adds) · Enter / right-click applies · Esc aborts
- Bevel size 0.3 m (pinned)
- Variable size: 0.3 m at the anchor corner → 0.15 m (F flips the anchor)
- Bevel end size 0.1 m
- View Front
- Anchor flipped: the start size is at each edge's end corner
- Fillet 0.3 → 0.1 on 1 edge(s) · 7 ms
- Undo
- Redo
