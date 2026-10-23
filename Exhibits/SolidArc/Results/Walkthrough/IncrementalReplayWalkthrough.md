# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0
- Frame: 1600 x 900 · 126 frames · 14.1035 s · slowest frame 436.655 ms
- Script: /home/user/Frontier/Exhibits/SolidArc/Code/Scripts/IncrementalReplayWalkthrough.txt
- Expectations: 29 / 29 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 12 | `steps == 8` | 8 | 🟢 |
| 13 | `refused == 0` | 0 | 🟢 |
| 14 | `sound == 1` | 1 | 🟢 |
| 18 | `focused-step == Blend#7` | Blend#7 | 🟢 |
| 24 | `status contains Fillet1.Size` | Fillet1.Size = 0.3 · 175 ms | 🟢 |
| 25 | `refused == 0` | 0 | 🟢 |
| 26 | `sound == 1` | 1 | 🟢 |
| 27 | `replay-resumed == 6` | 6 | 🟢 |
| 37 | `status contains Cylinder2.Radius` | Cylinder2.Radius = 0.35 · 220 ms | 🟢 |
| 38 | `refused == 0` | 0 | 🟢 |
| 39 | `replay-resumed == 4` | 4 | 🟢 |
| 45 | `replay-resumed == 8` | 8 | 🟢 |
| 46 | `undo == 6` | 6 | 🟢 |
| 47 | `redo == 1` | 1 | 🟢 |
| 51 | `replay-resumed == 8` | 8 | 🟢 |
| 52 | `redo == 0` | 0 | 🟢 |
| 63 | `status contains Block1.Height` | Block1.Height = 1.2 · 249 ms | 🟢 |
| 64 | `refused == 0` | 0 | 🟢 |
| 65 | `replay-resumed == 1` | 1 | 🟢 |
| 71 | `status contains Block1.Width` | Block1.Width = 4.4 · 260 ms | 🟢 |
| 72 | `refused == 0` | 0 | 🟢 |
| 73 | `sound == 1` | 1 | 🟢 |
| 74 | `replay-resumed == 0` | 0 | 🟢 |
| 84 | `selected-edges == 1` | Edge(Extrude#2.Cap(Start)/Extrude#2.Side(Line1)) | 🟢 |
| 87 | `bevel-active == 1` | 1 | 🟢 |
| 88 | `preview-resumed == 8` | 8 | 🟢 |
| 92 | `bevel-active == 0` | 0 | 🟢 |
| 93 | `steps == 8` | 8 | 🟢 |
| 94 | `replay-resumed == 0` | 0 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 140ReplayTailEdit | 25 | `1bcdd84f6fc7eec4` |
| 141ReplayMiddleEdit | 47 | `37b527eb2be00b66` |
| 142ReplayUndo | 54 | `e4d5fdcc292473d1` |
| 143ReplayRedo | 61 | `0d9835c9b3125ce6` |
| 144ReplayFullEdit | 98 | `95aff2a9880400e6` |
| 145ReplayPreview | 120 | `d89492ded4125e28` |

## Journal

- Fillet1.Size = 0.3 · 175 ms
- Cylinder2.Radius = 0.35 · 220 ms
- Undo
- Redo
- Block1.Height = 1.2 · 249 ms
- Block1.Width = 4.4 · 260 ms
- Selection cleared
- Selected Edge(Extrude#2.Cap(Start)|Extrude#2.Side(Line1))
- Fillet (B): move horizontally · D fillet · C chamfer · V variable size · F flip its anchor · click edits the edges · Enter / right-click applies · Esc aborts
- Bevel cancelled (document untouched)
