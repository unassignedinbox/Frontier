# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 900 · 201 frames · 19.4508 s · slowest frame 512.828 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/InsetWalkthrough.txt
- Expectations: 26 / 26 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 26 | `bodies == 1` | 1 | 🟢 |
| 27 | `volume ~ 3` | 3 | 🟢 |
| 28 | `faces == 6` | 6 | 🟢 |
| 35 | `selected-faces == 1` | Extrude#2.Cap(End) | 🟢 |
| 38 | `bevel-active == 1` | 1 | 🟢 |
| 39 | `bevel-verb == 3` | 3 | 🟢 |
| 40 | `bevel-depth-pointer == 0` | 0 | 🟢 |
| 49 | `bevel-size ~ 0.25` | 0.25 | 🟢 |
| 50 | `bevel-refusal == 0` |  | 🟢 |
| 51 | `bevel-preview-volume ~ 3` | 3 | 🟢 |
| 57 | `bevel-depth-pointer == 1` | 1 | 🟢 |
| 63 | `bevel-depth ~ -0.2` | -0.2 | 🟢 |
| 64 | `bevel-preview-volume ~ 2.7` | 2.7 | 🟢 |
| 68 | `bevel-active == 0` | 0 | 🟢 |
| 69 | `refused == 0` | 0 | 🟢 |
| 70 | `volume ~ 2.7` | 2.7 | 🟢 |
| 71 | `faces == 11` | 11 | 🟢 |
| 80 | `bevel-active == 1` | 1 | 🟢 |
| 81 | `bevel-verb == 3` | 3 | 🟢 |
| 91 | `bevel-preview-volume ~ 2.908` | 2.908 | 🟢 |
| 94 | `bevel-active == 0` | 0 | 🟢 |
| 95 | `volume ~ 2.908` | 2.908 | 🟢 |
| 96 | `faces == 16` | 16 | 🟢 |
| 109 | `bevel-refusal > 0` | Change#5: InsetFaces: F22 has holes (inset of hole loops) | 🟢 |
| 113 | `bevel-active == 0` | 0 | 🟢 |
| 114 | `volume ~ 2.908` | 2.908 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 212InsetOpened | 68 | `152fd93ec379b9a8` |
| 213InsetFlat | 86 | `d1ead754358adc08` |
| 214InsetPocketPreview | 108 | `1940fd8c6c0972b6` |
| 215InsetPocket | 116 | `74bd48e269b32b1f` |
| 216InsetBoss | 165 | `e48c6b7d69950180` |
| 217InsetRefused | 195 | `709857c1cf9b4807` |

## Journal

- New document (previous recovery copy kept)
- Block: click the first corner · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Block: click the opposite corner
- Block: move up, click the height
- Block1 · New body · 9 ms
- Selected Extrude#2.Cap(End)
- Inset (O / I): move horizontally · D distance · H depth (− pocket) · click edits the faces · Enter / right-click applies · Esc aborts
- Inset distance 0.25 m (pinned)
- Inset depth -0.2 m
- Inset 0.25 depth -0.2 on 1 face(s) · 17 ms
- Selected Change#3.Generated(Extrude#2.Cap(End))
- Inset (O / I): move horizontally · D distance · H depth (− pocket) · click edits the faces · Enter / right-click applies · Esc aborts
- Inset distance 0.1 m (pinned)
- Inset depth 0.2 m
- Inset 0.1 depth 0.2 on 1 face(s) · 23 ms
- Selected Extrude#2.Cap(End)
- Inset (O / I): move horizontally · D distance · H depth (− pocket) · click edits the faces · Enter / right-click applies · Esc aborts
- Inset distance 0.8 m (pinned)
- Bevel cancelled (document untouched)
