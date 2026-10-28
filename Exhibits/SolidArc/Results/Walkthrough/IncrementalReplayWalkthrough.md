# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 810 · 184 frames · 16.0424 s · slowest frame 335.434 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/IncrementalReplayWalkthrough.txt
- Expectations: 29 / 29 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 15 | `steps == 8` | 8 | 🟢 |
| 16 | `refused == 0` | 0 | 🟢 |
| 17 | `sound == 1` | 1 | 🟢 |
| 26 | `focused-step == Blend#7` | Blend#7 | 🟢 |
| 32 | `status contains Fillet1.Size` | Fillet1.Size = 0.3 · 2 ms | 🟢 |
| 33 | `refused == 0` | 0 | 🟢 |
| 34 | `sound == 1` | 1 | 🟢 |
| 35 | `replay-resumed == 6` | 6 | 🟢 |
| 50 | `status contains Cylinder2.Radius` | Cylinder2.Radius = 0.35 · 4 ms | 🟢 |
| 51 | `refused == 0` | 0 | 🟢 |
| 52 | `replay-resumed == 4` | 4 | 🟢 |
| 58 | `replay-resumed == 8` | 8 | 🟢 |
| 59 | `undo == 6` | 6 | 🟢 |
| 60 | `redo == 1` | 1 | 🟢 |
| 64 | `replay-resumed == 8` | 8 | 🟢 |
| 65 | `redo == 0` | 0 | 🟢 |
| 81 | `status contains Block1.Height` | Block1.Height = 1.2 · 6 ms | 🟢 |
| 82 | `refused == 0` | 0 | 🟢 |
| 83 | `replay-resumed == 1` | 1 | 🟢 |
| 89 | `status contains Block1.Width` | Block1.Width = 4.4 · 6 ms | 🟢 |
| 90 | `refused == 0` | 0 | 🟢 |
| 91 | `sound == 1` | 1 | 🟢 |
| 92 | `replay-resumed == 0` | 0 | 🟢 |
| 102 | `selected-edges == 1` | Edge(Extrude#2.Cap(Start)/Extrude#2.Side(Line1)) | 🟢 |
| 105 | `bevel-active == 1` | 1 | 🟢 |
| 106 | `preview-resumed == 8` | 8 | 🟢 |
| 110 | `bevel-active == 0` | 0 | 🟢 |
| 111 | `steps == 8` | 8 | 🟢 |
| 112 | `replay-resumed == 0` | 0 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 140ReplayTailEdit | 47 | `f5bf6277f17cd4fb` |
| 141ReplayMiddleEdit | 87 | `4ecb3b5791883e6f` |
| 142ReplayUndo | 94 | `4c21b6d03e59cff9` |
| 143ReplayRedo | 101 | `8f48d5b632b026aa` |
| 144ReplayFullEdit | 156 | `8f18e5855269a38d` |
| 145ReplayPreview | 178 | `adce24858f04d786` |

## Journal

- Fillet1.Size = 0.3 · 2 ms
- Autosaved _ClaudeScratch/SolidArcWorkspace/ReplayRecovery.solidarc
- Cylinder2.Radius = 0.35 · 4 ms
- Undo
- Redo
- Autosaved _ClaudeScratch/SolidArcWorkspace/ReplayRecovery.solidarc
- Block1.Height = 1.2 · 6 ms
- Block1.Width = 4.4 · 6 ms
- Selection cleared
- Selected Edge(Extrude#2.Cap(Start)|Extrude#2.Side(Line1))
- Fillet (B): drag anywhere or the ring handle to size (it stops at the largest that fits) · drag a corner diamond for a variable size · type in the card · D fillet · C chamfer · V taper · Alt+click an edge loop, Ctrl+Alt+click a ring (Shift adds) · Enter / right-click applies · Esc aborts
- Bevel cancelled (document untouched)
