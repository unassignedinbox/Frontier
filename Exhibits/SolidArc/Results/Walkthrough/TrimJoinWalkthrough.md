# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0
- Frame: 1600 x 960 · 201 frames · 27.0043 s · slowest frame 674.078 ms
- Script: /home/user/Frontier/Exhibits/SolidArc/Code/Scripts/TrimJoinWalkthrough.txt
- Expectations: 17 / 17 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 35 | `bodies == 1` | 1 | 🟢 |
| 36 | `volume ~ 0.785398` | 0.7853981634 | 🟢 |
| 37 | `sketches == 2` | Rectangle1,Circle1 | 🟢 |
| 44 | `trim-hover contains Circle1` | Circle1 · Circle1 | 🟢 |
| 45 | `trim-refusal == 0` |  | 🟢 |
| 49 | `trim-commits == 1` | 1 | 🟢 |
| 50 | `hidden-curves == 1` | 1 | 🟢 |
| 51 | `sketches == 3` | Rectangle1,Circle1,Circle2 | 🟢 |
| 52 | `bodies == 1` | 1 | 🟢 |
| 53 | `volume ~ 0.785398` | 0.7853981634 | 🟢 |
| 72 | `sketches == 5` | Rectangle1,Circle1,Circle2,Rectangle2,Circle3 | 🟢 |
| 77 | `selected-curves == 2` | Rectangle2,Circle3 | 🟢 |
| 81 | `sketches == 4` | Rectangle1,Circle1,Circle2,Rectangle2 | 🟢 |
| 82 | `sketch-curves == 5` | 5 | 🟢 |
| 83 | `selected-curves == 1` | Rectangle2 | 🟢 |
| 97 | `refused == 0` | 0 | 🟢 |
| 98 | `volume ~ 1.318141` | 1.318141502 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 235TrimHoverExtruded | 89 | `12a75d5392bbf4ab` |
| 236TrimDetached | 99 | `0b23bd7138c1f5a0` |
| 237JoinPicked | 160 | `6b1955034e10f0b3` |
| 238JoinExtruded | 201 | `7020c6d4e22f35e0` |

## Journal

- New document (previous recovery copy kept)
- Rectangle on plane XY (or the body face under the first click): click two corners · Alt centre mode · hold Ctrl to snap to ends / midpoints / centres · Esc aborts
- Rectangle1 · 0 ms
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres · Esc aborts
- Circle1 · 0 ms
- Extrude Circle1 (E): move horizontally · Q union · W difference · E intersect · R new body · Enter / right-click applies · Esc aborts
- Bevel size 1 m (pinned)
- Extrude1 · Circle1 · New · 12 ms
- Trim: click the piece of a curve to remove (between its crossings) · Esc ends
- Trim Circle1 of Circle1 into Circle2 (Circle1 kept hidden for the steps that read it) · 18 ms
- Trim ended
- Rectangle on plane XY (or the body face under the first click): click two corners · Alt centre mode · hold Ctrl to snap to ends / midpoints / centres · Esc aborts
- Rectangle2 · 19 ms
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres · Esc aborts
- Circle3 · 11 ms
- Selected sketch Rectangle2 · 1 selected · L lofts the picks in order · the gizmo moves / turns them
- Selected sketch Circle3 · 2 selected · L lofts the picks in order · the gizmo moves / turns them
- Join (Rectangle2, Circle3) · 11 ms
- Extrude Rectangle2 (E): move horizontally · Q union · W difference · E intersect · R new body · Enter / right-click applies · Esc aborts
- Extrude Boolean: New body
- Bevel size 1 m (pinned)
- Extrude2 · Rectangle2 · New body · 20 ms
