# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 900 · 212 frames · 19.7595 s · slowest frame 506.84 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/TrimJoinWalkthrough.txt
- Expectations: 17 / 17 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 40 | `bodies == 1` | 1 | 🟢 |
| 41 | `volume ~ 0.785398` | 0.7853981634 | 🟢 |
| 42 | `sketches == 2` | Rectangle1,Circle1 | 🟢 |
| 49 | `trim-hover contains Circle1` | Circle1 · Circle1 | 🟢 |
| 50 | `trim-refusal == 0` |  | 🟢 |
| 54 | `trim-commits == 1` | 1 | 🟢 |
| 55 | `hidden-curves == 1` | 1 | 🟢 |
| 56 | `sketches == 3` | Rectangle1,Circle1,Circle2 | 🟢 |
| 57 | `bodies == 1` | 1 | 🟢 |
| 58 | `volume ~ 0.785398` | 0.7853981634 | 🟢 |
| 77 | `sketches == 5` | Rectangle1,Circle1,Circle2,Rectangle2,Circle3 | 🟢 |
| 82 | `selected-curves == 2` | Rectangle2,Circle3 | 🟢 |
| 86 | `sketches == 4` | Rectangle1,Circle1,Circle2,Rectangle2 | 🟢 |
| 87 | `sketch-curves == 5` | 5 | 🟢 |
| 88 | `selected-curves == 1` | Rectangle2 | 🟢 |
| 104 | `refused == 0` | 0 | 🟢 |
| 105 | `volume ~ 1.318141` | 1.318141502 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 235TrimHoverExtruded | 95 | `2997ac14c1fffde2` |
| 236TrimDetached | 105 | `954ed4a261642dab` |
| 237JoinPicked | 166 | `8c800082f5efdbc4` |
| 238JoinExtruded | 212 | `20e13b69213990aa` |

## Journal

- New document (previous recovery copy kept)
- Rectangle on plane XY (or the body face under the first click): click two corners · Alt centre mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Rectangle1 · 0 ms
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Circle1 · 0 ms
- Fill Circle1 · 0 ms
- Extrude Circle1 (E): move horizontally · Q union · W difference · E intersect · R new body · Enter / right-click applies · Esc aborts
- Bevel size 1 m (pinned)
- Extrude1 · Circle1 · New · 0 ms
- Trim: click the piece of a curve to remove (between its crossings) · Esc ends
- Trim Circle1 of Circle1 into Circle2 (Circle1 kept hidden for the steps that read it) · 0 ms
- Trim ended
- Rectangle on plane XY (or the body face under the first click): click two corners · Alt centre mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Rectangle2 · 0 ms
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Circle3 · 0 ms
- Selected sketch Rectangle2 · 1 selected · L lofts the picks in order · the gizmo moves / turns them
- Selected sketch Circle3 · 2 selected · L lofts the picks in order · the gizmo moves / turns them
- Join (Rectangle2, Circle3) · 0 ms
- Fill Rectangle2 · 0 ms
- Extrude Rectangle2 (E): move horizontally · Q union · W difference · E intersect · R new body · Enter / right-click applies · Esc aborts
- Extrude Boolean: New body
- Bevel size 1 m (pinned)
- Extrude2 · Rectangle2 · New body · 1 ms
