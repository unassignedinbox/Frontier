# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0
- Frame: 1600 x 900 · 97 frames · 11.7693 s · slowest frame 419.351 ms
- Script: /home/user/Frontier/Exhibits/SolidArc/Code/Scripts/VariableBlendWalkthrough.txt
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
| 46 | `bevel-active == 1` | 1 | 🟢 |
| 52 | `bevel-flipped == 1` | 1 | 🟢 |
| 53 | `bevel-refusal == 0` |  | 🟢 |
| 54 | `bevel-preview-volume ~ 10.185606734` | 10.18560673 | 🟢 |
| 60 | `bevel-chamfer == 1` | 1 | 🟢 |
| 61 | `bevel-refusal == 0` |  | 🟢 |
| 62 | `bevel-preview-volume ~ 10.136107080` | 10.13610708 | 🟢 |
| 66 | `bevel-chamfer == 0` | 0 | 🟢 |
| 71 | `bevel-active == 0` | 0 | 🟢 |
| 72 | `steps == 9` | 9 | 🟢 |
| 73 | `faces == 12` | 12 | 🟢 |
| 74 | `refused == 0` | 0 | 🟢 |
| 75 | `sound == 1` | 1 | 🟢 |
| 76 | `volume ~ 10.185606734` | 10.18560673 | 🟢 |
| 77 | `status contains 0.3 → 0.1` | Fillet 0.3 → 0.1 on 1 edge(s) · 163 ms | 🟢 |
| 82 | `faces == 11` | 11 | 🟢 |
| 83 | `steps == 8` | 8 | 🟢 |
| 85 | `faces == 12` | 12 | 🟢 |
| 86 | `volume ~ 10.185606734` | 10.18560673 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 22VariableFillet | 64 | `13de59d96502ed26` |
| 23VariableFlipped | 71 | `5207028642a24a38` |
| 24VariableChamfer | 78 | `e26773826f8afb0a` |
| 25VariableApplied | 90 | `9ba35812157f64d4` |

## Journal

- Selected Edge(Extrude#2.Cap(Start)|Extrude#2.Side(Line1))
- Fillet (B): move horizontally · D fillet · C chamfer · V variable size · F flip its anchor · click edits the edges · Enter / right-click applies · Esc aborts
- Bevel size 0.3 m (pinned)
- Variable size: 0.3 m at the anchor corner → 0.15 m (F flips the anchor)
- Bevel end size 0.1 m
- View Front
- Anchor flipped: the start size is at each edge's end corner
- Fillet 0.3 → 0.1 on 1 edge(s) · 163 ms
- Undo
- Redo
