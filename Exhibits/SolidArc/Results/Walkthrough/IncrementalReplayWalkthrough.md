# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0
- Frame: 1600 x 900 · 129 frames · 14.6743 s · slowest frame 463.891 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/IncrementalReplayWalkthrough.txt
- Expectations: 29 / 29 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 12 | `steps == 8` | 8 | 🟢 |
| 13 | `refused == 0` | 0 | 🟢 |
| 14 | `sound == 1` | 1 | 🟢 |
| 18 | `focused-step == Blend#7` | Blend#7 | 🟢 |
| 24 | `status contains Fillet1.Size` | Fillet1.Size = 0.3 · 173 ms | 🟢 |
| 25 | `refused == 0` | 0 | 🟢 |
| 26 | `sound == 1` | 1 | 🟢 |
| 27 | `replay-resumed == 6` | 6 | 🟢 |
| 37 | `status contains Cylinder2.Radius` | Cylinder2.Radius = 0.35 · 214 ms | 🟢 |
| 38 | `refused == 0` | 0 | 🟢 |
| 39 | `replay-resumed == 4` | 4 | 🟢 |
| 45 | `replay-resumed == 8` | 8 | 🟢 |
| 46 | `undo == 6` | 6 | 🟢 |
| 47 | `redo == 1` | 1 | 🟢 |
| 51 | `replay-resumed == 8` | 8 | 🟢 |
| 52 | `redo == 0` | 0 | 🟢 |
| 63 | `status contains Block1.Height` | Block1.Height = 1.2 · 251 ms | 🟢 |
| 64 | `refused == 0` | 0 | 🟢 |
| 65 | `replay-resumed == 1` | 1 | 🟢 |
| 71 | `status contains Block1.Width` | Block1.Width = 4.4 · 264 ms | 🟢 |
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
| 140ReplayTailEdit | 28 | `b492cb471b32bc92` |
| 141ReplayMiddleEdit | 50 | `1c5b22197bdea934` |
| 142ReplayUndo | 57 | `f85c646bf94c1154` |
| 143ReplayRedo | 64 | `d7d280c943d2dbe0` |
| 144ReplayFullEdit | 101 | `2839f06a4c9b5c85` |
| 145ReplayPreview | 123 | `d55700ccb60cf7cd` |

## Journal

- Fillet1.Size = 0.3 · 173 ms
- Cylinder2.Radius = 0.35 · 214 ms
- Undo
- Redo
- Block1.Height = 1.2 · 251 ms
- Block1.Width = 4.4 · 264 ms
- Selection cleared
- Selected Edge(Extrude#2.Cap(Start)|Extrude#2.Side(Line1))
- Fillet (B): move horizontally · D fillet · C chamfer · V variable size · F flip its anchor · click edits the edges · Enter / right-click applies · Esc aborts
- Bevel cancelled (document untouched)
