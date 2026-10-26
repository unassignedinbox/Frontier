# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 900 · 345 frames · 39.4614 s · slowest frame 535.061 ms
- Script: ../../Exhibits/SolidArc/Code/Scripts/KnifeWalkthrough.txt
- Expectations: 52 / 52 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 29 | `refused == 0` | 0 | 🟢 |
| 30 | `bodies == 1` | 1 | 🟢 |
| 31 | `volume ~ 8` | 8 | 🟢 |
| 37 | `knife == 1` | 1 | 🟢 |
| 38 | `knife-pending == 1` | 1 | 🟢 |
| 39 | `knife-mode == Sketch` | Sketch | 🟢 |
| 42 | `face-plane == 1` | Extrude#2.Cap(End) | 🟢 |
| 49 | `knife-pending == 0` | 0 | 🟢 |
| 50 | `sketch-filled == 1` | 1 | 🟢 |
| 53 | `knife-depth ~ 0.5` | 0.5 | 🟢 |
| 54 | `knife-target == Block1` | Block1 | 🟢 |
| 55 | `knife-refusal == 0` |  | 🟢 |
| 59 | `refused == 0` | 0 | 🟢 |
| 60 | `knife == 0` | 0 | 🟢 |
| 61 | `knife-commits == 1` | 1 | 🟢 |
| 62 | `bodies == 1` | 1 | 🟢 |
| 63 | `volume ~ 7.858628330588459` | 7.858628331 | 🟢 |
| 78 | `knife-pending == 0` | 0 | 🟢 |
| 81 | `knife-through == 1` | 1 | 🟢 |
| 85 | `refused == 0` | 0 | 🟢 |
| 86 | `knife-commits == 2` | 2 | 🟢 |
| 87 | `volume ~ 7.538628330588459` | 7.538628331 | 🟢 |
| 95 | `knife-mode == Insert` | Insert | 🟢 |
| 96 | `knife-pending == 0` | 0 | 🟢 |
| 97 | `knife-inserts > 6` | 7 | 🟢 |
| 98 | `knife-insert == Square pocket` | Square pocket | 🟢 |
| 103 | `knife-hover == 1` | 1 | 🟢 |
| 106 | `knife-size ~ 0.4` | 0.4 | 🟢 |
| 109 | `knife-placed == 1` | 1 | 🟢 |
| 112 | `knife-turn ~ 15` | 15 | 🟢 |
| 116 | `refused == 0` | 0 | 🟢 |
| 117 | `knife-commits == 3` | 3 | 🟢 |
| 118 | `bodies == 1` | 1 | 🟢 |
| 119 | `volume ~ 7.519428330588459` | 7.519428331 | 🟢 |
| 132 | `knife-hover == 1` | 1 | 🟢 |
| 139 | `refused == 0` | 0 | 🟢 |
| 140 | `knife-commits == 4` | 4 | 🟢 |
| 141 | `bodies == 1` | 1 | 🟢 |
| 142 | `volume ~ 7.538628330588459` | 7.538628331 | 🟢 |
| 161 | `refused == 0` | 0 | 🟢 |
| 162 | `knife-commits == 5` | 5 | 🟢 |
| 163 | `bodies == 2` | 2 | 🟢 |
| 164 | `volume ~ 7.561028330588459` | 7.561028331 | 🟢 |
| 165 | `selected-bodies contains Import` | Import#13 | 🟢 |
| 172 | `knife-inserts > 7` | 8 | 🟢 |
| 175 | `knife == 0` | 0 | 🟢 |
| 186 | `knife-mode == Object` | Object | 🟢 |
| 192 | `refused == 0` | 0 | 🟢 |
| 193 | `knife-commits == 6` | 6 | 🟢 |
| 194 | `bodies == 2` | 2 | 🟢 |
| 195 | `volume ~ 7.541828330588459` | 7.541828331 | 🟢 |
| 196 | `sound == 1` | 1 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 271KnifeSketchStaged | 92 | `5f2d5a2b6e413820` |
| 272KnifeThrough | 144 | `335bc279ac759a16` |
| 273KnifeInsertPlaced | 192 | `021b7cbecec9eb3f` |
| 274KnifeObject | 338 | `f56a116fa1b84c22` |

## Journal

- New document (previous recovery copy kept)
- Block: click the first corner · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Block: click the opposite corner
- Block: move up, click the height
- Block1 · New body · 16 ms
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Knife · draw a circle on a face (card: Rectangle / Polygon / Slot / Line) · then move sideways for the depth · Tab: Insert library · Esc cancels
- Circle1 · 12 ms
- Knife · Circle1 into Block1 · move sideways for the depth (Shift fine, Ctrl free) or type it · T through · Q add · W cut · E intersect · R separate · click / right-click / Enter applies
- Knife1 · Circle1 · Difference · depth 0.5 · 35 ms
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Knife · draw a circle on a face (card: Rectangle / Polygon / Slot / Line) · then move sideways for the depth · Tab: Insert library · Esc cancels
- Rectangle on plane XY (or the body face under the first click): click two corners · Alt centre mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Rectangle1 · 16 ms
- Knife · Rectangle1 into Block1 · move sideways for the depth (Shift fine, Ctrl free) or type it · T through · Q add · W cut · E intersect · R separate · click / right-click / Enter applies
- Knife: through
- Knife2 · Rectangle1 · Difference · depth 2.06 · 62 ms
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Knife · draw a circle on a face (card: Rectangle / Polygon / Slot / Line) · then move sideways for the depth · Tab: Insert library · Esc cancels
- Knife · Insert Square pocket · point at a face, click to place, move to size, [ / ] turn · 7 insert(s) in 175 ms
- Knife · Square pocket placed on Block1 · move sideways to size it (Shift fine, Ctrl free) or type a width · [ / ] turn 15° · click / right-click / Enter applies
- Knife: turn 15°
- Autosaved ../tmp/rt/Knife/Rec.solidarc
- Knife3 · Square pocket · Difference · 71 ms
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Knife · draw a circle on a face (card: Rectangle / Polygon / Slot / Line) · then move sideways for the depth · Tab: Insert library · Esc cancels
- Knife · Insert Square pocket · point at a face, click to place, move to size, [ / ] turn · 7 insert(s) in 175 ms
- Knife: Add
- Knife · Square pocket placed on Block1 · move sideways to size it (Shift fine, Ctrl free) or type a width · [ / ] turn 15° · click / right-click / Enter applies
- Autosaved ../tmp/rt/Knife/Rec.solidarc
- Knife4 · Square pocket · Union · 81 ms
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Knife · draw a circle on a face (card: Rectangle / Polygon / Slot / Line) · then move sideways for the depth · Tab: Insert library · Esc cancels
- Knife · Insert Square pocket · point at a face, click to place, move to size, [ / ] turn · 7 insert(s) in 175 ms
- Knife: Separate
- Knife · Square pocket placed on Block1 · move sideways to size it (Shift fine, Ctrl free) or type a width · [ / ] turn 15° · click / right-click / Enter applies
- Autosaved ../tmp/rt/Knife/Rec.solidarc
- Knife5 · Square pocket · New body · 22 ms
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Knife · draw a circle on a face (card: Rectangle / Polygon / Slot / Line) · then move sideways for the depth · Tab: Insert library · Esc cancels
- Knife · Insert Square pocket · point at a face, click to place, move to size, [ / ] turn · 7 insert(s) in 175 ms
- Insert saved: ../tmp/rt/Knife/Inserts/Squarepocket3.step · library 8 insert(s)
- Knife cancelled (document untouched)
- Selected Block1 (22 faces)
- Autosaved ../tmp/rt/Knife/Rec.solidarc
- Added Square pocket3 · 2 bodies picked
- Knife · Object · the first picked body is cut by the others (Shift-pick cutters) · K keeps the cutters · click / right-click / Enter applies
- Knife: cutters kept
- Knife5 · Difference (Block1 − Square pocket3) · cutters kept · 110 ms
