# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 810 · 223 frames · 16.9343 s · slowest frame 275.635 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/EdgeStrokeWalkthrough.txt
- Expectations: 20 / 20 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 31 | `bodies == 1` | 1 | 🟢 |
| 32 | `refused == 0` | 0 | 🟢 |
| 43 | `face-plane == 1` | Extrude#2.Side(Circle1) | 🟢 |
| 48 | `refused == 0` | 0 | 🟢 |
| 49 | `plane-normal-y < -0.9999` | -0.9999977355 | 🟢 |
| 50 | `plane-offset ~ 1.7` | 1.7 | 🟢 |
| 57 | `profile-sink > 0.011806` | 0.0208670383 | 🟢 |
| 58 | `profile-sink < 0.03` | 0.0208670383 | 🟢 |
| 66 | `bodies == 1` | 1 | 🟢 |
| 67 | `refused == 0` | 0 | 🟢 |
| 68 | `sound == 1` | 1 | 🟢 |
| 69 | `extrude-start < -0.011806` | -0.0208670383 | 🟢 |
| 70 | `volume ~ 5.485591014531` | 5.485591015 | 🟢 |
| 79 | `selected-edges == 1` | Edge(Extrude#4.Cap(End)/Extrude#4.Side(Circle1)) | 🟢 |
| 91 | `bevel-active == 0` | 0 | 🟢 |
| 92 | `refused == 0` | 0 | 🟢 |
| 93 | `sound == 1` | 1 | 🟢 |
| 103 | `refused == 0` | 0 | 🟢 |
| 111 | `refused == 0` | 0 | 🟢 |
| 119 | `refused == 0` | 0 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 267EdgeStrokesWhole | 181 | `6f58bdf796b34eca` |
| 268EdgeStrokesCloser | 195 | `451e95905f3902eb` |
| 269EdgeStrokesCloseUp | 209 | `44ef2e76668c8e6b` |
| 270EdgeStrokesVeryClose | 223 | `974b6ca085463a36` |

## Journal

- New document (previous recovery copy kept)
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Circle1 · 0 ms
- Fill Circle1 · 0 ms
- Extrude Circle1 (E): the profile follows the pointer along its normal · Q union · W difference · E intersect · R new body · A auto · Enter / right-click applies · Esc aborts
- Bevel size 0.6 m (pinned)
- Extrude1 · Circle1 · New · 0 ms
- View Isometric
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Autosaved _ClaudeScratch/SolidArcWorkspace/EdgeStrokeRecovery.solidarc
- Circle2 · 0 ms
- Fill Circle2 · 0 ms
- Extrude Circle2 (E): the profile follows the pointer along its normal · Q union · W difference · E intersect · R new body · A auto · Enter / right-click applies · Esc aborts
- Bevel size 0.3 m (pinned)
- Extrude2 · Circle2 · Union · 10 ms
- Selected Edge(Extrude#4.Cap(End)|Extrude#4.Side(Circle1))
- Fillet (B): drag anywhere or the ring handle to size (it stops at the largest that fits) · drag a corner diamond for a variable size · type in the card · D fillet · C chamfer · V taper · Alt+click an edge loop, Ctrl+Alt+click a ring (Shift adds) · Enter / right-click applies · Esc aborts · starts at 0.02 m to fit the edges
- Bevel size 0.05 m (pinned)
- Fillet 0.05 on 1 edge(s) · 11 ms
- Autosaved _ClaudeScratch/SolidArcWorkspace/EdgeStrokeRecovery.solidarc
