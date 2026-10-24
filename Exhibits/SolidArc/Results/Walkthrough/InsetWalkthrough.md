# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0
- Frame: 1600 x 960 · 201 frames · 14.4621 s · slowest frame 394.94 ms
- Script: /home/user/Frontier/Exhibits/SolidArc/Code/Scripts/InsetWalkthrough.txt
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
| 79 | `bevel-active == 1` | 1 | 🟢 |
| 80 | `bevel-verb == 3` | 3 | 🟢 |
| 90 | `bevel-preview-volume ~ 2.908` | 2.908 | 🟢 |
| 93 | `bevel-active == 0` | 0 | 🟢 |
| 94 | `volume ~ 2.908` | 2.908 | 🟢 |
| 95 | `faces == 16` | 16 | 🟢 |
| 108 | `bevel-refusal > 0` | Change#5: InsetFaces: F22 has holes (inset of hole loops) | 🟢 |
| 112 | `bevel-active == 0` | 0 | 🟢 |
| 113 | `volume ~ 2.908` | 2.908 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 212InsetOpened | 68 | `d67b9ea83ea81225` |
| 213InsetFlat | 86 | `e6b8d410d5b06528` |
| 214InsetPocketPreview | 108 | `824948fce83dc1a8` |
| 215InsetPocket | 116 | `b0205620a5e5885d` |
| 216InsetBoss | 165 | `de91d72e8774b762` |
| 217InsetRefused | 195 | `e3f291ee2d3f69bd` |

## Journal

- New document (previous recovery copy kept)
- Block: click the first corner · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Block: click the opposite corner
- Block: move up, click the height
- Block1 · New body · 6 ms
- Selected Extrude#2.Cap(End)
- Inset (O / I): move horizontally · D distance · H depth (− pocket) · click edits the faces · Enter / right-click applies · Esc aborts
- Bevel size 0.25 m (pinned)
- Inset depth -0.2 m
- Inset 0.25 depth -0.2 on 1 face(s) · 16 ms
- Selected Change#3.Generated(Extrude#2.Cap(End))
- Inset (O / I): move horizontally · D distance · H depth (− pocket) · click edits the faces · Enter / right-click applies · Esc aborts
- Bevel size 0.1 m (pinned)
- Inset depth 0.2 m
- Inset 0.1 depth 0.2 on 1 face(s) · 21 ms
- Selected Extrude#2.Cap(End)
- Inset (O / I): move horizontally · D distance · H depth (− pocket) · click edits the faces · Enter / right-click applies · Esc aborts
- Bevel size 0.8 m (pinned)
- Bevel cancelled (document untouched)
