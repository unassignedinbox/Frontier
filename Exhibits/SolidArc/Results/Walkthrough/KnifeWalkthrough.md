# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 900 · 468 frames · 53.4787 s · slowest frame 555.783 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/KnifeWalkthrough.txt
- Expectations: 71 / 71 passed

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
| 210 | `knife == 1` | 1 | 🟢 |
| 211 | `knife-mode == Sketch` | Sketch | 🟢 |
| 212 | `knife-pending == 1` | 1 | 🟢 |
| 213 | `drawing == Rectangle` | Rectangle | 🟢 |
| 222 | `knife-pending == 0` | 0 | 🟢 |
| 223 | `knife-through == 1` | 1 | 🟢 |
| 226 | `knife-through == 0` | 0 | 🟢 |
| 229 | `knife-depth ~ 0.1` | 0.1 | 🟢 |
| 232 | `refused == 0` | 0 | 🟢 |
| 233 | `knife-commits == 7` | 7 | 🟢 |
| 234 | `volume ~ 7.525828330588459` | 7.525828331 | 🟢 |
| 237 | `knife-mode == Sketch` | Sketch | 🟢 |
| 238 | `drawing == Rectangle` | Rectangle | 🟢 |
| 243 | `knife-mode == Insert` | Insert | 🟢 |
| 244 | `knife-insert == Square pocket` | Square pocket | 🟢 |
| 249 | `knife-mode == Sketch` | Sketch | 🟢 |
| 250 | `drawing == Circle` | Circle | 🟢 |
| 253 | `knife == 0` | 0 | 🟢 |
| 256 | `drawing == Circle` | Circle | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 271KnifeSketchStaged | 92 | `6767b9e2006fd234` |
| 272KnifeThrough | 144 | `c94dfde5aaf42daf` |
| 273KnifeInsertPlaced | 192 | `de288b7e0cb18d68` |
| 274KnifeObject | 338 | `24e18b6775c91d11` |
| 275KnifeToolMenu | 366 | `1418de4e4081da85` |

## Journal

- New document (previous recovery copy kept)
- Block: click the first corner · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Block: click the opposite corner
- Block: move up, click the height
- Block1 · New body · 15 ms
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Knife · draw a Circle on a face (toolbar menu or card: Circle / Rectangle / Polygon / Slot / Line) · then move sideways for the depth · Tab: Insert library · Esc cancels
- Circle1 · 10 ms
- Knife · Circle1 into Block1 · move sideways for the depth (Shift fine, Ctrl free) or type it · T through · Q add · W cut · E intersect · R separate · click / right-click / Enter applies
- Knife1 · Circle1 · Difference · depth 0.5 · 35 ms
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Knife · draw a Circle on a face (toolbar menu or card: Circle / Rectangle / Polygon / Slot / Line) · then move sideways for the depth · Tab: Insert library · Esc cancels
- Rectangle on plane XY (or the body face under the first click): click two corners · Alt centre mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Rectangle1 · 13 ms
- Knife · Rectangle1 into Block1 · move sideways for the depth (Shift fine, Ctrl free) or type it · T through · Q add · W cut · E intersect · R separate · click / right-click / Enter applies
- Knife: through
- Knife2 · Rectangle1 · Difference · depth 2.06 · 62 ms
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Knife · draw a Circle on a face (toolbar menu or card: Circle / Rectangle / Polygon / Slot / Line) · then move sideways for the depth · Tab: Insert library · Esc cancels
- Knife · Insert Square pocket · point at a face, click to place, move to size, [ / ] turn · 7 insert(s) in 117 ms
- Knife · Square pocket placed on Block1 · move sideways to size it (Shift fine, Ctrl free) or type a width · [ / ] turn 15° · click / right-click / Enter applies
- Knife: turn 15°
- Autosaved _ClaudeScratch/SolidArcWorkspace/KnifeRecovery.solidarc
- Knife3 · Square pocket · Difference · 72 ms
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Knife · draw a Circle on a face (toolbar menu or card: Circle / Rectangle / Polygon / Slot / Line) · then move sideways for the depth · Tab: Insert library · Esc cancels
- Knife · Insert Square pocket · point at a face, click to place, move to size, [ / ] turn · 7 insert(s) in 117 ms
- Knife: Add
- Knife · Square pocket placed on Block1 · move sideways to size it (Shift fine, Ctrl free) or type a width · [ / ] turn 15° · click / right-click / Enter applies
- Autosaved _ClaudeScratch/SolidArcWorkspace/KnifeRecovery.solidarc
- Knife4 · Square pocket · Union · 88 ms
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Knife · draw a Circle on a face (toolbar menu or card: Circle / Rectangle / Polygon / Slot / Line) · then move sideways for the depth · Tab: Insert library · Esc cancels
- Knife · Insert Square pocket · point at a face, click to place, move to size, [ / ] turn · 7 insert(s) in 117 ms
- Knife: Separate
- Knife · Square pocket placed on Block1 · move sideways to size it (Shift fine, Ctrl free) or type a width · [ / ] turn 15° · click / right-click / Enter applies
- Autosaved _ClaudeScratch/SolidArcWorkspace/KnifeRecovery.solidarc
- Knife5 · Square pocket · New body · 22 ms
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Knife · draw a Circle on a face (toolbar menu or card: Circle / Rectangle / Polygon / Slot / Line) · then move sideways for the depth · Tab: Insert library · Esc cancels
- Knife · Insert Square pocket · point at a face, click to place, move to size, [ / ] turn · 7 insert(s) in 117 ms
- Insert saved: _ClaudeScratch/SolidArcWorkspace/KnifeInserts/Squarepocket3.step · library 8 insert(s)
- Knife cancelled (document untouched)
- Selected Block1 (22 faces)
- Autosaved _ClaudeScratch/SolidArcWorkspace/KnifeRecovery.solidarc
- Added Square pocket3 · 2 bodies picked
- Knife · Object · the first picked body is cut by the others (Shift-pick cutters) · K keeps the cutters · click / right-click / Enter applies
- Knife: cutters kept
- Knife5 · Difference (Block1 − Square pocket3) · cutters kept · 94 ms
- Selection cleared
- Rectangle on plane XY (or the body face under the first click): click two corners · Alt centre mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Knife · draw a Rectangle on a face (toolbar menu or card: Circle / Rectangle / Polygon / Slot / Line) · then move sideways for the depth · Tab: Insert library · Esc cancels
- Knife · Rectangle · draw it on a face, then move sideways for the depth (T through)
- Autosaved _ClaudeScratch/SolidArcWorkspace/KnifeRecovery.solidarc
- Rectangle2 · 20 ms
- Knife · Rectangle2 into Block1 · move sideways for the depth (Shift fine, Ctrl free) or type it · T through · Q add · W cut · E intersect · R separate · click / right-click / Enter applies
- Knife: to depth
- Knife6 · Rectangle2 · Difference · depth 0.1 · 95 ms
- Rectangle on plane XY (or the body face under the first click): click two corners · Alt centre mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Knife · draw a Rectangle on a face (toolbar menu or card: Circle / Rectangle / Polygon / Slot / Line) · then move sideways for the depth · Tab: Insert library · Esc cancels
- Knife · Insert Square pocket · point at a face, click to place, move to size, [ / ] turn · 8 insert(s) in 122 ms
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Knife · Circle · draw it on a face, then move sideways for the depth (T through)
- Autosaved _ClaudeScratch/SolidArcWorkspace/KnifeRecovery.solidarc
- Circle aborted (document untouched)
- Knife cancelled (document untouched)
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Knife · draw a Circle on a face (toolbar menu or card: Circle / Rectangle / Polygon / Slot / Line) · then move sideways for the depth · Tab: Insert library · Esc cancels
- Circle aborted (document untouched)
- Knife cancelled (document untouched)
