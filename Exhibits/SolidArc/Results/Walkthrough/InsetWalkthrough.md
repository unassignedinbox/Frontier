# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 900 · 202 frames · 19.1786 s · slowest frame 522.004 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/InsetWalkthrough.txt
- Expectations: 26 / 26 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 29 | `bodies == 1` | 1 | 🟢 |
| 30 | `volume ~ 3` | 3 | 🟢 |
| 31 | `faces == 6` | 6 | 🟢 |
| 38 | `selected-faces == 1` | Extrude#2.Cap(End) | 🟢 |
| 41 | `bevel-active == 1` | 1 | 🟢 |
| 42 | `bevel-verb == 3` | 3 | 🟢 |
| 43 | `bevel-depth-pointer == 0` | 0 | 🟢 |
| 52 | `bevel-size ~ 0.25` | 0.25 | 🟢 |
| 53 | `bevel-refusal == 0` |  | 🟢 |
| 54 | `bevel-preview-volume ~ 3` | 3 | 🟢 |
| 60 | `bevel-depth-pointer == 1` | 1 | 🟢 |
| 66 | `bevel-depth ~ -0.2` | -0.2 | 🟢 |
| 67 | `bevel-preview-volume ~ 2.7` | 2.7 | 🟢 |
| 71 | `bevel-active == 0` | 0 | 🟢 |
| 72 | `refused == 0` | 0 | 🟢 |
| 73 | `volume ~ 2.7` | 2.7 | 🟢 |
| 74 | `faces == 11` | 11 | 🟢 |
| 83 | `bevel-active == 1` | 1 | 🟢 |
| 84 | `bevel-verb == 3` | 3 | 🟢 |
| 94 | `bevel-preview-volume ~ 2.908` | 2.908 | 🟢 |
| 97 | `bevel-active == 0` | 0 | 🟢 |
| 98 | `volume ~ 2.908` | 2.908 | 🟢 |
| 99 | `faces == 16` | 16 | 🟢 |
| 112 | `bevel-refusal > 0` | Change#5: InsetFaces: F22 has holes (inset of hole loops) | 🟢 |
| 116 | `bevel-active == 0` | 0 | 🟢 |
| 117 | `volume ~ 2.908` | 2.908 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 212InsetOpened | 69 | `5d1a070c98f9f43a` |
| 213InsetFlat | 87 | `d44ccc8b5730fe3e` |
| 214InsetPocketPreview | 109 | `b563bda3fceee3a6` |
| 215InsetPocket | 117 | `ef4786677a0940e1` |
| 216InsetBoss | 166 | `fefadeb6d1a7b125` |
| 217InsetRefused | 196 | `b913255081f80c71` |

## Journal

- New document (previous recovery copy kept)
- Block: click the first corner · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Block: click the opposite corner
- Block: move up, click the height
- Block1 · New body · 0 ms
- Selected Extrude#2.Cap(End)
- Inset (O / I): move horizontally · D distance · H depth (− pocket) · click edits the faces · Enter / right-click applies · Esc aborts
- Inset distance 0.25 m (pinned)
- Inset depth -0.2 m
- Inset 0.25 depth -0.2 on 1 face(s) · 1 ms
- Selected Change#3.Generated(Extrude#2.Cap(End))
- Inset (O / I): move horizontally · D distance · H depth (− pocket) · click edits the faces · Enter / right-click applies · Esc aborts
- Inset distance 0.1 m (pinned)
- Inset depth 0.2 m
- Inset 0.1 depth 0.2 on 1 face(s) · 1 ms
- Selected Extrude#2.Cap(End)
- Inset (O / I): move horizontally · D distance · H depth (− pocket) · click edits the faces · Enter / right-click applies · Esc aborts
- Inset distance 0.8 m (pinned)
- Bevel cancelled (document untouched)
