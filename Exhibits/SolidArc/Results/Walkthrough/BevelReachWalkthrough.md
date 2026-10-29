# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 810 · 275 frames · 26.711 s · slowest frame 498.202 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/BevelReachWalkthrough.txt
- Expectations: 16 / 16 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 32 | `volume ~ 8` | 8 | 🟢 |
| 39 | `bevel-active == 1` | 1 | 🟢 |
| 40 | `bevel-span ~ 2` | 2 | 🟢 |
| 45 | `bevel-size ~ 2` | 2 | 🟢 |
| 46 | `bevel-refusal == 0` |  | 🟢 |
| 47 | `bevel-preview-volume ~ 6.283185307179586` | 6.283185307 | 🟢 |
| 53 | `bevel-chamfer == 1` | 1 | 🟢 |
| 54 | `bevel-size ~ 2` | 2 | 🟢 |
| 55 | `bevel-refusal == 0` |  | 🟢 |
| 56 | `bevel-preview-volume ~ 4` | 4 | 🟢 |
| 64 | `bevel-size ~ 2` | 2 | 🟢 |
| 65 | `bevel-preview-volume ~ 6.283185307179586` | 6.283185307 | 🟢 |
| 68 | `bevel-active == 0` | 0 | 🟢 |
| 69 | `refused == 0` | 0 | 🟢 |
| 70 | `volume ~ 6.283185307179586` | 6.283185307 | 🟢 |
| 71 | `sound == 1` | 1 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 363BevelReachFillet | 155 | `c6443d27235f33d0` |
| 364BevelReachChamfer | 172 | `4da6815b15256bef` |
| 365BevelReachApplied | 275 | `ef388eeea5c68ca2` |

## Journal

- New document (previous recovery copy kept)
- Block: click the first corner · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Block: click the opposite corner
- Block: move up, click the height
- Block1 · New body · 1 ms
- Selected Edge(Extrude#2.Cap(End)|Extrude#2.Side(Line1))
- Fillet (B): drag anywhere or the ring handle to size (it stops at the largest that fits) · drag a corner diamond for a variable size · type in the card · D fillet · C chamfer · V taper · Alt+click an edge loop, Ctrl+Alt+click a ring (Shift adds) · Enter / right-click applies · Esc aborts
- Fillet 2 on 1 edge(s) · 1 ms
