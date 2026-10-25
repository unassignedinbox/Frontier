# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0
- Frame: 1600 x 900 · 104 frames · 11.1817 s · slowest frame 467.319 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/VariableBlendWalkthrough.txt
- Expectations: 31 / 31 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 11 | `faces == 11` | 11 | 🟢 |
| 12 | `steps == 8` | 8 | 🟢 |
| 17 | `selected-edges == 1` | Edge(Extrude#2.Cap(Start)/Extrude#2.Side(Line1)) | 🟢 |
| 20 | `bevel-active == 1` | 1 | 🟢 |
| 21 | `bevel-variable == 0` | 0 | 🟢 |
| 27 | `bevel-size ~ 0.3` | 0.3 | 🟢 |
| 28 | `bevel-pinned == 1` | 1 | 🟢 |
| 34 | `bevel-variable == 1` | 1 | 🟢 |
| 35 | `bevel-end-size ~ 0.15` | 0.15 | 🟢 |
| 41 | `bevel-end-size ~ 0.1` | 0.1 | 🟢 |
| 42 | `bevel-refusal == 0` |  | 🟢 |
| 43 | `bevel-preview-volume ~ 10.185606734` | 10.18560673 | 🟢 |
| 48 | `bevel-active == 1` | 1 | 🟢 |
| 54 | `bevel-flipped == 1` | 1 | 🟢 |
| 55 | `bevel-refusal == 0` |  | 🟢 |
| 56 | `bevel-preview-volume ~ 10.185606734` | 10.18560673 | 🟢 |
| 62 | `bevel-chamfer == 1` | 1 | 🟢 |
| 63 | `bevel-refusal == 0` |  | 🟢 |
| 64 | `bevel-preview-volume ~ 10.136107080` | 10.13610708 | 🟢 |
| 68 | `bevel-chamfer == 0` | 0 | 🟢 |
| 73 | `bevel-active == 0` | 0 | 🟢 |
| 74 | `steps == 9` | 9 | 🟢 |
| 75 | `faces == 12` | 12 | 🟢 |
| 76 | `refused == 0` | 0 | 🟢 |
| 77 | `sound == 1` | 1 | 🟢 |
| 78 | `volume ~ 10.185606734` | 10.18560673 | 🟢 |
| 79 | `status contains 0.3 → 0.1` | Fillet 0.3 → 0.1 on 1 edge(s) · 152 ms | 🟢 |
| 84 | `faces == 11` | 11 | 🟢 |
| 85 | `steps == 8` | 8 | 🟢 |
| 87 | `faces == 12` | 12 | 🟢 |
| 88 | `volume ~ 10.185606734` | 10.18560673 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 22VariableFillet | 71 | `f03291bf227de0ba` |
| 23VariableFlipped | 78 | `0aca0cefcf8c684d` |
| 24VariableChamfer | 85 | `209e836c4ed063a3` |
| 25VariableApplied | 97 | `962d7e6ba898d5ae` |

## Journal

- Selected Edge(Extrude#2.Cap(Start)|Extrude#2.Side(Line1))
- Fillet (B): move horizontally · D fillet · C chamfer · V variable size · F flip its anchor · click edits the edges · Enter / right-click applies · Esc aborts
- Bevel size 0.3 m (pinned)
- Variable size: 0.3 m at the anchor corner → 0.15 m (F flips the anchor)
- Bevel end size 0.1 m
- View Front
- Anchor flipped: the start size is at each edge's end corner
- Fillet 0.3 → 0.1 on 1 edge(s) · 152 ms
- Undo
- Redo
