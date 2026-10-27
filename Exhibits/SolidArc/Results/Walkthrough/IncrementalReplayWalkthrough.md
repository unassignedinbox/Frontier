# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 900 · 130 frames · 17.3114 s · slowest frame 527.122 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/IncrementalReplayWalkthrough.txt
- Expectations: 29 / 29 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 15 | `steps == 8` | 8 | 🟢 |
| 16 | `refused == 0` | 0 | 🟢 |
| 17 | `sound == 1` | 1 | 🟢 |
| 21 | `focused-step == Blend#7` | Blend#7 | 🟢 |
| 27 | `status contains Fillet1.Size` | Fillet1.Size = 0.3 · 4 ms | 🟢 |
| 28 | `refused == 0` | 0 | 🟢 |
| 29 | `sound == 1` | 1 | 🟢 |
| 30 | `replay-resumed == 6` | 6 | 🟢 |
| 40 | `status contains Cylinder2.Radius` | Cylinder2.Radius = 0.35 · 9 ms | 🟢 |
| 41 | `refused == 0` | 0 | 🟢 |
| 42 | `replay-resumed == 4` | 4 | 🟢 |
| 48 | `replay-resumed == 8` | 8 | 🟢 |
| 49 | `undo == 6` | 6 | 🟢 |
| 50 | `redo == 1` | 1 | 🟢 |
| 54 | `replay-resumed == 8` | 8 | 🟢 |
| 55 | `redo == 0` | 0 | 🟢 |
| 66 | `status contains Block1.Height` | Block1.Height = 1.2 · 12 ms | 🟢 |
| 67 | `refused == 0` | 0 | 🟢 |
| 68 | `replay-resumed == 1` | 1 | 🟢 |
| 74 | `status contains Block1.Width` | Block1.Width = 4.4 · 12 ms | 🟢 |
| 75 | `refused == 0` | 0 | 🟢 |
| 76 | `sound == 1` | 1 | 🟢 |
| 77 | `replay-resumed == 0` | 0 | 🟢 |
| 87 | `selected-edges == 1` | Edge(Extrude#2.Cap(Start)/Extrude#2.Side(Line1)) | 🟢 |
| 90 | `bevel-active == 1` | 1 | 🟢 |
| 91 | `preview-resumed == 8` | 8 | 🟢 |
| 95 | `bevel-active == 0` | 0 | 🟢 |
| 96 | `steps == 8` | 8 | 🟢 |
| 97 | `replay-resumed == 0` | 0 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 140ReplayTailEdit | 29 | `653e9de05fc461f0` |
| 141ReplayMiddleEdit | 51 | `f9d73db60813916b` |
| 142ReplayUndo | 58 | `f3c64ae780039128` |
| 143ReplayRedo | 65 | `40ab35728615fea4` |
| 144ReplayFullEdit | 102 | `a9d85e56d29858cf` |
| 145ReplayPreview | 124 | `a80de8cf580d6310` |

## Journal

- Fillet1.Size = 0.3 · 4 ms
- Cylinder2.Radius = 0.35 · 9 ms
- Undo
- Redo
- Block1.Height = 1.2 · 12 ms
- Block1.Width = 4.4 · 12 ms
- Selection cleared
- Selected Edge(Extrude#2.Cap(Start)|Extrude#2.Side(Line1))
- Fillet (B): drag anywhere or the ring handle to size (it stops at the largest that fits) · drag a corner diamond for a variable size · type in the card · D fillet · C chamfer · V taper · Alt+click an edge adds its ring · Enter / right-click applies · Esc aborts
- Bevel cancelled (document untouched)
