# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 900 · 1075 frames · 198.04 s · slowest frame 559.777 ms
- Script: ../../Exhibits/SolidArc/Code/Scripts/KnifeWalkthrough.txt
- Expectations: 155 / 155 passed

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
| 273 | `drawing == Circle` | Circle | 🟢 |
| 277 | `knife-pending == 0` | 0 | 🟢 |
| 278 | `knife-through == 0` | 0 | 🟢 |
| 283 | `knife-depth > 0.55` | 0.59 | 🟢 |
| 284 | `knife-depth < 0.65` | 0.59 | 🟢 |
| 285 | `knife-refusal == 0` |  | 🟢 |
| 289 | `knife == 0` | 0 | 🟢 |
| 290 | `volume ~ 7.525828330588459` | 7.525828331 | 🟢 |
| 300 | `knife == 1` | 1 | 🟢 |
| 301 | `knife-pending == 1` | 1 | 🟢 |
| 302 | `drawing == Line` | Line | 🟢 |
| 311 | `knife-pending == 0` | 0 | 🟢 |
| 312 | `knife-slice == 1` | 1 | 🟢 |
| 313 | `knife-refusal == 0` |  | 🟢 |
| 316 | `knife-refusal == 0` |  | 🟢 |
| 320 | `refused == 0` | 0 | 🟢 |
| 321 | `knife == 0` | 0 | 🟢 |
| 322 | `knife-commits == 8` | 8 | 🟢 |
| 323 | `bodies == 3` | 3 | 🟢 |
| 324 | `volume ~ 7.525828330588459` | 7.525828331 | 🟢 |
| 329 | `selected-faces > 0` | Extrude#2.Cap(End) | 🟢 |
| 336 | `selected-bodies > 0` | Cut#21 | 🟢 |
| 348 | `selected-bodies == 1` | Import#13 | 🟢 |
| 351 | `knife == 1` | 1 | 🟢 |
| 352 | `knife-mode == Object` | Object | 🟢 |
| 353 | `knife-placed == 0` | 0 | 🟢 |
| 356 | `knife-hover == 1` | 1 | 🟢 |
| 357 | `knife-align == 1` | 1 | 🟢 |
| 358 | `knife-rigid == 1` | 1 | 🟢 |
| 359 | `knife-target == Block1` | Block1 | 🟢 |
| 365 | `knife-placed == 1` | 1 | 🟢 |
| 366 | `knife-align == 1` | 1 | 🟢 |
| 369 | `knife-sink < -0.1` | -0.25 | 🟢 |
| 372 | `knife-sink ~ 0.03` | 0.03 | 🟢 |
| 373 | `knife-refusal == 0` |  | 🟢 |
| 377 | `refused == 0` | 0 | 🟢 |
| 378 | `knife == 0` | 0 | 🟢 |
| 379 | `knife-commits == 9` | 9 | 🟢 |
| 380 | `bodies == 2` | 2 | 🟢 |
| 381 | `volume ~ 7.487428330588459` | 7.487428332 | 🟢 |
| 392 | `knife-pending == 1` | 1 | 🟢 |
| 395 | `knife-pending == 0` | 0 | 🟢 |
| 396 | `sketch-filled == 1` | 1 | 🟢 |
| 399 | `knife-depth ~ 0.1` | 0.1 | 🟢 |
| 402 | `refused == 0` | 0 | 🟢 |
| 403 | `knife == 0` | 0 | 🟢 |
| 404 | `knife-commits == 10` | 10 | 🟢 |
| 405 | `volume < 7.487428330588459` | 7.481428332 | 🟢 |
| 416 | `knife-pending == 0` | 0 | 🟢 |
| 419 | `knife-depth ~ 0.05` | 0.05 | 🟢 |
| 422 | `refused == 0` | 0 | 🟢 |
| 423 | `knife-commits == 11` | 11 | 🟢 |
| 428 | `drawing == Polygon` | Polygon | 🟢 |
| 431 | `drawing == Polygon` | Polygon | 🟢 |
| 432 | `knife-pending == 1` | 1 | 🟢 |
| 435 | `drawing-sides == 5` | 5 | 🟢 |
| 439 | `knife-pending == 0` | 0 | 🟢 |
| 444 | `refused == 0` | 0 | 🟢 |
| 445 | `knife-commits == 12` | 12 | 🟢 |
| 470 | `bodies == 1` | 1 | 🟢 |
| 471 | `volume ~ 8` | 8 | 🟢 |
| 478 | `knife == 1` | 1 | 🟢 |
| 479 | `knife-pending == 1` | 1 | 🟢 |
| 480 | `drawing-solid == 1` | 1 | 🟢 |
| 481 | `drawing-solid-kind == Block` | Block | 🟢 |
| 490 | `drawing-lift ~ -0.3` | -0.3 | 🟢 |
| 491 | `knife-target == Block1` | Block1 | 🟢 |
| 495 | `refused == 0` | 0 | 🟢 |
| 496 | `knife == 0` | 0 | 🟢 |
| 497 | `knife-commits == 13` | 13 | 🟢 |
| 498 | `bodies == 1` | 1 | 🟢 |
| 499 | `volume ~ 7.952` | 7.952 | 🟢 |
| 504 | `drawing-solid-kind == Cylinder` | Cylinder | 🟢 |
| 513 | `drawing-lift ~ -0.5` | -0.5 | 🟢 |
| 518 | `refused == 0` | 0 | 🟢 |
| 519 | `knife-commits == 14` | 14 | 🟢 |
| 520 | `bodies == 2` | 2 | 🟢 |
| 521 | `volume ~ 7.954513274122872` | 7.954513274 | 🟢 |
| 526 | `drawing-solid-kind == Sphere` | Sphere | 🟢 |
| 531 | `knife-target == Block1` | Block1 | 🟢 |
| 535 | `refused == 0` | 0 | 🟢 |
| 536 | `knife-commits == 15` | 15 | 🟢 |
| 537 | `bodies == 2` | 2 | 🟢 |
| 538 | `volume < 7.95` | 7.897964606 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 271KnifeSketchStaged | 92 | `478e863113e21f2e` |
| 272KnifeThrough | 144 | `baee71918436bb71` |
| 273KnifeInsertPlaced | 192 | `60eaff1f00357b14` |
| 274KnifeObject | 338 | `3fe1a0bf63107d8d` |
| 275KnifeToolMenu | 366 | `db2499c2736fa7b3` |
| 281KnifeDrawingDimensions | 517 | `98e9a62e8dbc3942` |
| 282KnifeCutterDimensions | 539 | `e1fee86614f0b56e` |
| 283KnifeLineSlice | 598 | `41c21efa0cdf1615` |
| 285KnifeCutterFollows | 659 | `9e5beeb086cae502` |
| 286KnifeCutterPlaced | 683 | `11b6fd67529bf2ca` |
| 287KnifePolygonSides | 839 | `f69ce2194a9b7e0c` |
| 288KnifeBoxCutter | 968 | `a5ec5024d4e8b0b8` |
| 289KnifeSphereCutter | 1066 | `4c0bf4d0726670f1` |

## Journal

- New document (previous recovery copy kept)
- Block: click the first corner · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Block: click the opposite corner
- Block: move up, click the height
- Block1 · New body · 0 ms
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Knife · draw a Circle on a face (toolbar menu or card: Circle / Rectangle / Polygon / Slot / Line) · then pull along the normal for the depth · Tab: Insert library · Esc cancels
- Circle1 · 0 ms
- Knife · Circle1 into Block1 · pull along the face normal for the depth (Ctrl free) or type it · T through · Q add · W cut · E intersect · R separate · click / right-click / Enter applies
- Knife1 · Circle1 · Difference · depth 0.5 · 3 ms
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Knife · draw a Circle on a face (toolbar menu or card: Circle / Rectangle / Polygon / Slot / Line) · then pull along the normal for the depth · Tab: Insert library · Esc cancels
- Rectangle on plane XY (or the body face under the first click): click two corners · Alt centre mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Rectangle1 · 0 ms
- Knife · Rectangle1 into Block1 · pull along the face normal for the depth (Ctrl free) or type it · T through · Q add · W cut · E intersect · R separate · click / right-click / Enter applies
- Knife: through
- Knife2 · Rectangle1 · Difference · depth 2.06 · 4 ms
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Knife · draw a Circle on a face (toolbar menu or card: Circle / Rectangle / Polygon / Slot / Line) · then pull along the normal for the depth · Tab: Insert library · Esc cancels
- Knife · Insert Square pocket · point at a face, click to place, move to size, [ / ] turn · 7 insert(s) in 65 ms
- Knife · Square pocket placed on Block1 · move sideways to size it (Shift fine, Ctrl free) or type a width · [ / ] turn 15° · click / right-click / Enter applies
- Knife: turn 15°
- Autosaved ../tmp/rt/Knife/Rec.solidarc
- Knife3 · Square pocket · Difference · 7 ms
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Knife · draw a Circle on a face (toolbar menu or card: Circle / Rectangle / Polygon / Slot / Line) · then pull along the normal for the depth · Tab: Insert library · Esc cancels
- Knife · Insert Square pocket · point at a face, click to place, move to size, [ / ] turn · 7 insert(s) in 65 ms
- Knife: Add
- Knife · Square pocket placed on Block1 · move sideways to size it (Shift fine, Ctrl free) or type a width · [ / ] turn 15° · click / right-click / Enter applies
- Autosaved ../tmp/rt/Knife/Rec.solidarc
- Knife4 · Square pocket · Union · 6 ms
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Knife · draw a Circle on a face (toolbar menu or card: Circle / Rectangle / Polygon / Slot / Line) · then pull along the normal for the depth · Tab: Insert library · Esc cancels
- Knife · Insert Square pocket · point at a face, click to place, move to size, [ / ] turn · 7 insert(s) in 65 ms
- Knife: Separate
- Knife · Square pocket placed on Block1 · move sideways to size it (Shift fine, Ctrl free) or type a width · [ / ] turn 15° · click / right-click / Enter applies
- Autosaved ../tmp/rt/Knife/Rec.solidarc
- Knife5 · Square pocket · New body · 2 ms
- Knife · Square pocket3 follows the pointer onto a face (Ctrl: along its normal · [ / ] turn) · click / right-click places it · then in / out along the normal · click / right-click / Enter cuts
- Knife · Insert Square pocket · point at a face, click to place, move to size, [ / ] turn · 7 insert(s) in 65 ms
- Insert saved: ../tmp/rt/Knife/Inserts/Squarepocket3.step · library 8 insert(s)
- Knife cancelled (document untouched)
- Selected Block1 (22 faces)
- Autosaved ../tmp/rt/Knife/Rec.solidarc
- Added Square pocket3 · 2 bodies picked
- Knife · Object · the first picked body is cut by the others (Shift-pick cutters) · K keeps the cutters · click / right-click / Enter applies
- Knife: cutters kept
- Knife5 · Difference (Block1 − Square pocket3) · cutters kept · 8 ms
- Selection cleared
- Rectangle on plane XY (or the body face under the first click): click two corners · Alt centre mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Knife · draw a Rectangle on a face (toolbar menu or card: Circle / Rectangle / Polygon / Slot / Line) · then pull along the normal for the depth · Tab: Insert library · Esc cancels
- Knife · Rectangle · draw it on a face (live W / H / R dimensions), then pull along the normal for the depth (T through)
- Autosaved ../tmp/rt/Knife/Rec.solidarc
- Rectangle2 · 1 ms
- Knife · Rectangle2 into Block1 · pull along the face normal for the depth (Ctrl free) or type it · T through · Q add · W cut · E intersect · R separate · click / right-click / Enter applies
- Knife: to depth
- Knife6 · Rectangle2 · Difference · depth 0.1 · 9 ms
- Rectangle on plane XY (or the body face under the first click): click two corners · Alt centre mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Knife · draw a Rectangle on a face (toolbar menu or card: Circle / Rectangle / Polygon / Slot / Line) · then pull along the normal for the depth · Tab: Insert library · Esc cancels
- Knife · Insert Square pocket · point at a face, click to place, move to size, [ / ] turn · 8 insert(s) in 71 ms
- Autosaved ../tmp/rt/Knife/Rec.solidarc
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Knife · Circle · draw it on a face (live W / H / R dimensions), then pull along the normal for the depth (T through)
- Circle aborted (document untouched)
- Knife cancelled (document untouched)
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Knife · draw a Circle on a face (toolbar menu or card: Circle / Rectangle / Polygon / Slot / Line) · then pull along the normal for the depth · Tab: Insert library · Esc cancels
- Circle aborted (document untouched)
- Knife cancelled (document untouched)
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Knife · draw a Circle on a face (toolbar menu or card: Circle / Rectangle / Polygon / Slot / Line) · then pull along the normal for the depth · Tab: Insert library · Esc cancels
- Knife · Circle · draw it on a face (live W / H / R dimensions), then pull along the normal for the depth (T through)
- Circle2 · 2 ms
- Knife · Circle2 into Block1 · pull along the face normal for the depth (Ctrl free) or type it · T through · Q add · W cut · E intersect · R separate · click / right-click / Enter applies
- Knife cancelled (document untouched)
- Autosaved ../tmp/rt/Knife/Rec.solidarc
- Knife · Sketch Circle2 · pull along the face normal for the depth (as Construct heights) · T through
- Line on plane XY (or the body face under the first click): click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Knife · Line · draw it on a face (live W / H / R dimensions), then pull along the normal for the depth (T through)
- Line1 · 2 ms
- Knife · Line1 slices Block1 through (two bodies) · click / right-click / Enter splits it · Esc cancels
- Cut (Block1 by Line1) · 17 ms
- Selected Extrude#2.Cap(End)
- Selection cleared
- Selected Cut1 (6 faces)
- Selection cleared
- Autosaved ../tmp/rt/Knife/Rec.solidarc
- Selected Square pocket3 (6 faces)
- Knife · Square pocket3 follows the pointer onto a face (Ctrl: along its normal · [ / ] turn) · click / right-click places it · then in / out along the normal · click / right-click / Enter cuts
- Knife · Square pocket3 placed on Block1 · move the pointer along the normal to push it in / pull it out (Ctrl free) or type a depth · click / right-click / Enter cuts · Esc lifts it
- Knife7 · Difference (Block1 − Square pocket3 placed) · 9 ms
- Selection cleared
- Rectangle on plane XY (or the body face under the first click): click two corners · Alt centre mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Knife · draw a Rectangle on a face (toolbar menu or card: Circle / Rectangle / Polygon / Slot / Line) · then pull along the normal for the depth · Tab: Insert library · Esc cancels
- Knife · Rectangle · draw it on a face (live W / H / R dimensions), then pull along the normal for the depth (T through)
- Rectangle3 · 2 ms
- Knife · Rectangle3 into Block1 · pull along the face normal for the depth (Ctrl free) or type it · T through · Q add · W cut · E intersect · R separate · click / right-click / Enter applies
- Knife8 · Rectangle3 · Difference · depth 0.1 · 9 ms
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Knife · draw a Circle on a face (toolbar menu or card: Circle / Rectangle / Polygon / Slot / Line) · then pull along the normal for the depth · Tab: Insert library · Esc cancels
- Knife · Circle · draw it on a face (live W / H / R dimensions), then pull along the normal for the depth (T through)
- Circle3 · 2 ms
- Knife · Circle3 into Block1 · pull along the face normal for the depth (Ctrl free) or type it · T through · Q add · W cut · E intersect · R separate · click / right-click / Enter applies
- Knife9 · Circle3 · Difference · depth 0.05 · 17 ms
- Polygon on plane XY (or the body face under the first click): click the centre, then a vertex · + / − sides · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Knife · draw a Polygon on a face (toolbar menu or card: Circle / Rectangle / Polygon / Slot / Line) · then pull along the normal for the depth · Tab: Insert library · Esc cancels
- Knife · Polygon · draw it on a face (live W / H / R dimensions), then pull along the normal for the depth (T through)
- Polygon · 6 sides · wheel (or + / −) changes the sides · click / right-click / Enter confirms · Esc aborts
- 5 sides
- Autosaved ../tmp/rt/Knife/Rec.solidarc
- Polygon1 · 3 ms
- Knife · Polygon1 into Block1 · pull along the face normal for the depth (Ctrl free) or type it · T through · Q add · W cut · E intersect · R separate · click / right-click / Enter applies
- Knife10 · Polygon1 · Difference · depth 0.05 · 16 ms
- Selection cleared
- New document (previous recovery copy kept)
- Block: click the first corner · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Block: click the opposite corner
- Autosaved ../tmp/rt/Knife/Rec.solidarc
- Block: move up, click the height
- Block1 · New body · 1 ms
- Block: click the first corner · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Knife · Block cutter · click the first corner on a face (live dimensions) · the height goes into the body (Q: out of it) · the cut shows as you pull · Q / W / E / R · K keep · Esc cancels
- Block: click the opposite corner
- Autosaved ../tmp/rt/Knife/Rec.solidarc
- Block: move up, click the height
- Knife1 · Block · Difference (Block1) · 2 ms
- Cylinder: click the base centre · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Knife · Cylinder cutter · click the base centre on a face (live dimensions) · the height goes into the body (Q: out of it) · the cut shows as you pull · Q / W / E / R · K keep · Esc cancels
- Cylinder: click the radius
- Cylinder: move up, click the height
- Autosaved ../tmp/rt/Knife/Rec.solidarc
- Knife: the cutter stays as a body (K)
- Knife2 · Cylinder · Difference (Block1) · cutter kept · 5 ms
- Sphere: click the centre · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Knife · Sphere cutter · click the centre on a face (live dimensions) · the height goes into the body (Q: out of it) · the cut shows as you pull · Q / W / E / R · K keep · Esc cancels
- Sphere: click the radius
- Autosaved ../tmp/rt/Knife/Rec.solidarc
- Knife3 · Sphere · Difference (Block1) · 7 ms
