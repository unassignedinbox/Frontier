# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 900 · 211 frames · 20.3804 s · slowest frame 504.557 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/TrimJoinWalkthrough.txt
- Expectations: 17 / 17 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 37 | `bodies == 1` | 1 | 🟢 |
| 38 | `volume ~ 0.785398` | 0.7853981634 | 🟢 |
| 39 | `sketches == 2` | Rectangle1,Circle1 | 🟢 |
| 46 | `trim-hover contains Circle1` | Circle1 · Circle1 | 🟢 |
| 47 | `trim-refusal == 0` |  | 🟢 |
| 51 | `trim-commits == 1` | 1 | 🟢 |
| 52 | `hidden-curves == 1` | 1 | 🟢 |
| 53 | `sketches == 3` | Rectangle1,Circle1,Circle2 | 🟢 |
| 54 | `bodies == 1` | 1 | 🟢 |
| 55 | `volume ~ 0.785398` | 0.7853981634 | 🟢 |
| 74 | `sketches == 5` | Rectangle1,Circle1,Circle2,Rectangle2,Circle3 | 🟢 |
| 79 | `selected-curves == 2` | Rectangle2,Circle3 | 🟢 |
| 83 | `sketches == 4` | Rectangle1,Circle1,Circle2,Rectangle2 | 🟢 |
| 84 | `sketch-curves == 5` | 5 | 🟢 |
| 85 | `selected-curves == 1` | Rectangle2 | 🟢 |
| 101 | `refused == 0` | 0 | 🟢 |
| 102 | `volume ~ 1.318141` | 1.318141502 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 235TrimHoverExtruded | 94 | `d5d7ac1ac64dc219` |
| 236TrimDetached | 104 | `88427b70ca798c60` |
| 237JoinPicked | 165 | `89da8e67edb31902` |
| 238JoinExtruded | 211 | `3b186edfeca4a96a` |

## Journal

- New document (previous recovery copy kept)
- Rectangle on plane XY (or the body face under the first click): click two corners · Alt centre mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Rectangle1 · 0 ms
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Circle1 · 0 ms
- Fill Circle1 · 0 ms
- Extrude Circle1 (E): move horizontally · Q union · W difference · E intersect · R new body · Enter / right-click applies · Esc aborts
- Bevel size 1 m (pinned)
- Extrude1 · Circle1 · New · 18 ms
- Trim: click the piece of a curve to remove (between its crossings) · Esc ends
- Trim Circle1 of Circle1 into Circle2 (Circle1 kept hidden for the steps that read it) · 14 ms
- Trim ended
- Rectangle on plane XY (or the body face under the first click): click two corners · Alt centre mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Rectangle2 · 12 ms
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Circle3 · 13 ms
- Selected sketch Rectangle2 · 1 selected · L lofts the picks in order · the gizmo moves / turns them
- Selected sketch Circle3 · 2 selected · L lofts the picks in order · the gizmo moves / turns them
- Join (Rectangle2, Circle3) · 12 ms
- Fill Rectangle2 · 12 ms
- Extrude Rectangle2 (E): move horizontally · Q union · W difference · E intersect · R new body · Enter / right-click applies · Esc aborts
- Extrude Boolean: New body
- Bevel size 1 m (pinned)
- Extrude2 · Rectangle2 · New body · 23 ms
