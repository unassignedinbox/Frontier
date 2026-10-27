# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 900 · 1207 frames · 166.471 s · slowest frame 642.927 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/KnifeWalkthrough.txt
- Expectations: 179 / 179 passed

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
| 97 | `knife-inserts > 6` | 8 | 🟢 |
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
| 172 | `knife-inserts > 7` | 9 | 🟢 |
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
| 549 | `knife-mode == Insert` | Insert | 🟢 |
| 550 | `knife-insert == Triangle` | Triangle | 🟢 |
| 555 | `knife-hover == 1` | 1 | 🟢 |
| 558 | `knife-placed == 1` | 1 | 🟢 |
| 559 | `knife-sizing == 1` | 1 | 🟢 |
| 564 | `knife-size ~ 0.4` | 0.4 | 🟢 |
| 565 | `knife-turn ~ 0` | 0 | 🟢 |
| 569 | `knife-sizing == 0` | 0 | 🟢 |
| 570 | `knife-placed == 1` | 1 | 🟢 |
| 573 | `knife-insert-depth > 0.15` | 0.25 | 🟢 |
| 577 | `knife-insert-depth ~ 0.2` | 0.2 | 🟢 |
| 580 | `refused == 0` | 0 | 🟢 |
| 581 | `knife == 0` | 0 | 🟢 |
| 582 | `knife-commits == 16` | 16 | 🟢 |
| 583 | `knife-removed ~ 0.013856406460551` | 0.01385640646 | 🟢 |
| 588 | `knife-insert == Round hole` | Round hole | 🟢 |
| 593 | `knife-hover == 1` | 1 | 🟢 |
| 596 | `knife-size ~ 0.2` | 0.2 | 🟢 |
| 599 | `knife-placed == 1` | 1 | 🟢 |
| 600 | `knife-sizing == 0` | 0 | 🟢 |
| 603 | `knife-insert-through == 1` | 1 | 🟢 |
| 606 | `refused == 0` | 0 | 🟢 |
| 607 | `knife-commits == 17` | 17 | 🟢 |
| 608 | `knife-removed ~ 0.0628318530718` | 0.06283185307 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 271KnifeSketchStaged | 92 | `82a3e52294778ab0` |
| 272KnifeThrough | 144 | `e9f2e5956bffe0a4` |
| 273KnifeInsertPlaced | 192 | `2017c2d4aa16b175` |
| 274KnifeObject | 338 | `21d9ad0114ba4b27` |
| 275KnifeToolMenu | 366 | `00c596622b8f2da1` |
| 281KnifeDrawingDimensions | 517 | `e364cec14c03c6dc` |
| 282KnifeCutterDimensions | 539 | `2790c45bc209031b` |
| 283KnifeLineSlice | 598 | `f83102b0adece50f` |
| 285KnifeCutterFollows | 659 | `9fab9d8d24fe2e86` |
| 286KnifeCutterPlaced | 683 | `c526f491ea1a7aab` |
| 287KnifePolygonSides | 839 | `8260be05a9f30dc3` |
| 288KnifeBoxCutter | 968 | `3ccfeea5baa04ee5` |
| 289KnifeSphereCutter | 1066 | `154f07ad4e55709e` |
| 290KnifeInsertDrag | 1120 | `d9ab1382db730d1c` |
| 291KnifeInsertDepth | 1134 | `7940811e7a198d1f` |

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
- Knife2 · Rectangle1 · Difference · depth 2.06 · 6 ms
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Knife · draw a Circle on a face (toolbar menu or card: Circle / Rectangle / Polygon / Slot / Line) · then pull along the normal for the depth · Tab: Insert library · Esc cancels
- Knife · Insert Square pocket · point at a face · press for the centre, drag out the size and turn, release, then move for the depth · 8 insert(s) in 80 ms
- Knife · Square pocket centred on Block1 · drag out its size and turn (Ctrl free) · release for the depth
- Knife · Square pocket width 0.4 m · 0° · move along the normal for the depth (Ctrl free), T through, or type it · click / right-click / Enter cuts · Esc lifts
- Knife: turn 15°
- Autosaved _ClaudeScratch/SolidArcWorkspace/KnifeRecovery.solidarc
- Knife3 · Square pocket · Difference · 7 ms
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Knife · draw a Circle on a face (toolbar menu or card: Circle / Rectangle / Polygon / Slot / Line) · then pull along the normal for the depth · Tab: Insert library · Esc cancels
- Knife · Insert Square pocket · point at a face · press for the centre, drag out the size and turn, release, then move for the depth · 8 insert(s) in 80 ms
- Knife: Add
- Knife · Square pocket centred on Block1 · drag out its size and turn (Ctrl free) · release for the depth
- Knife · Square pocket width 0.4 m · 0° · move along the normal for the depth (Ctrl free), T through, or type it · click / right-click / Enter cuts · Esc lifts
- Autosaved _ClaudeScratch/SolidArcWorkspace/KnifeRecovery.solidarc
- Knife4 · Square pocket · Union · 7 ms
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Knife · draw a Circle on a face (toolbar menu or card: Circle / Rectangle / Polygon / Slot / Line) · then pull along the normal for the depth · Tab: Insert library · Esc cancels
- Knife · Insert Square pocket · point at a face · press for the centre, drag out the size and turn, release, then move for the depth · 8 insert(s) in 80 ms
- Knife: Separate
- Knife · Square pocket centred on Block1 · drag out its size and turn (Ctrl free) · release for the depth
- Knife · Square pocket width 0.4 m · 0° · move along the normal for the depth (Ctrl free), T through, or type it · click / right-click / Enter cuts · Esc lifts
- Autosaved _ClaudeScratch/SolidArcWorkspace/KnifeRecovery.solidarc
- Knife5 · Square pocket · New body · 1 ms
- Knife · Square pocket3 follows the pointer onto a face (Ctrl: along its normal · [ / ] turn) · click / right-click places it · then in / out along the normal · click / right-click / Enter cuts
- Knife · Insert Square pocket · point at a face · press for the centre, drag out the size and turn, release, then move for the depth · 8 insert(s) in 80 ms
- Insert saved: _ClaudeScratch/SolidArcWorkspace/KnifeInserts/Squarepocket32.step · library 9 insert(s)
- Knife cancelled (document untouched)
- Selected Block1 (22 faces)
- Autosaved _ClaudeScratch/SolidArcWorkspace/KnifeRecovery.solidarc
- Added Square pocket3 · 2 bodies picked
- Knife · Object · the first picked body is cut by the others (Shift-pick cutters) · K keeps the cutters · click / right-click / Enter applies
- Knife: cutters kept
- Knife5 · Difference (Block1 − Square pocket3) · cutters kept · 9 ms
- Selection cleared
- Rectangle on plane XY (or the body face under the first click): click two corners · Alt centre mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Knife · draw a Rectangle on a face (toolbar menu or card: Circle / Rectangle / Polygon / Slot / Line) · then pull along the normal for the depth · Tab: Insert library · Esc cancels
- Knife · Rectangle · draw it on a face (live W / H / R dimensions), then pull along the normal for the depth (T through)
- Autosaved _ClaudeScratch/SolidArcWorkspace/KnifeRecovery.solidarc
- Rectangle2 · 2 ms
- Knife · Rectangle2 into Block1 · pull along the face normal for the depth (Ctrl free) or type it · T through · Q add · W cut · E intersect · R separate · click / right-click / Enter applies
- Knife: to depth
- Knife6 · Rectangle2 · Difference · depth 0.1 · 9 ms
- Rectangle on plane XY (or the body face under the first click): click two corners · Alt centre mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Knife · draw a Rectangle on a face (toolbar menu or card: Circle / Rectangle / Polygon / Slot / Line) · then pull along the normal for the depth · Tab: Insert library · Esc cancels
- Knife · Insert Square pocket · point at a face · press for the centre, drag out the size and turn, release, then move for the depth · 9 insert(s) in 75 ms
- Autosaved _ClaudeScratch/SolidArcWorkspace/KnifeRecovery.solidarc
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
- Autosaved _ClaudeScratch/SolidArcWorkspace/KnifeRecovery.solidarc
- Knife · Sketch Circle2 · pull along the face normal for the depth (as Construct heights) · T through
- Line on plane XY (or the body face under the first click): click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Knife · Line · draw it on a face (live W / H / R dimensions), then pull along the normal for the depth (T through)
- Line1 · 2 ms
- Knife · Line1 slices Block1 through (two bodies) · click / right-click / Enter splits it · Esc cancels
- Cut (Block1 by Line1) · 18 ms
- Selected Extrude#2.Cap(End)
- Selection cleared
- Selected Cut1 (6 faces)
- Selection cleared
- Autosaved _ClaudeScratch/SolidArcWorkspace/KnifeRecovery.solidarc
- Selected Square pocket3 (6 faces)
- Knife · Square pocket3 follows the pointer onto a face (Ctrl: along its normal · [ / ] turn) · click / right-click places it · then in / out along the normal · click / right-click / Enter cuts
- Knife · Square pocket3 placed on Block1 · move the pointer along the normal to push it in / pull it out (Ctrl free) or type a depth · click / right-click / Enter cuts · Esc lifts it
- Knife7 · Difference (Block1 − Square pocket3 placed) · 8 ms
- Selection cleared
- Rectangle on plane XY (or the body face under the first click): click two corners · Alt centre mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Knife · draw a Rectangle on a face (toolbar menu or card: Circle / Rectangle / Polygon / Slot / Line) · then pull along the normal for the depth · Tab: Insert library · Esc cancels
- Knife · Rectangle · draw it on a face (live W / H / R dimensions), then pull along the normal for the depth (T through)
- Rectangle3 · 2 ms
- Knife · Rectangle3 into Block1 · pull along the face normal for the depth (Ctrl free) or type it · T through · Q add · W cut · E intersect · R separate · click / right-click / Enter applies
- Knife8 · Rectangle3 · Difference · depth 0.1 · 10 ms
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Knife · draw a Circle on a face (toolbar menu or card: Circle / Rectangle / Polygon / Slot / Line) · then pull along the normal for the depth · Tab: Insert library · Esc cancels
- Knife · Circle · draw it on a face (live W / H / R dimensions), then pull along the normal for the depth (T through)
- Circle3 · 2 ms
- Knife · Circle3 into Block1 · pull along the face normal for the depth (Ctrl free) or type it · T through · Q add · W cut · E intersect · R separate · click / right-click / Enter applies
- Knife9 · Circle3 · Difference · depth 0.05 · 19 ms
- Polygon on plane XY (or the body face under the first click): click the centre, then a vertex · + / − sides · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Knife · draw a Polygon on a face (toolbar menu or card: Circle / Rectangle / Polygon / Slot / Line) · then pull along the normal for the depth · Tab: Insert library · Esc cancels
- Knife · Polygon · draw it on a face (live W / H / R dimensions), then pull along the normal for the depth (T through)
- Polygon · 6 sides · wheel (or + / −) changes the sides · click / right-click / Enter confirms · Esc aborts
- 5 sides
- Autosaved _ClaudeScratch/SolidArcWorkspace/KnifeRecovery.solidarc
- Polygon1 · 3 ms
- Knife · Polygon1 into Block1 · pull along the face normal for the depth (Ctrl free) or type it · T through · Q add · W cut · E intersect · R separate · click / right-click / Enter applies
- Knife10 · Polygon1 · Difference · depth 0.05 · 15 ms
- Selection cleared
- New document (previous recovery copy kept)
- Block: click the first corner · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Block: click the opposite corner
- Autosaved _ClaudeScratch/SolidArcWorkspace/KnifeRecovery.solidarc
- Block: move up, click the height
- Block1 · New body · 1 ms
- Block: click the first corner · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Knife · Block cutter · click the first corner on a face (live dimensions) · the height goes into the body (Q: out of it) · the cut shows as you pull · Q / W / E / R · K keep · Esc cancels
- Block: click the opposite corner
- Autosaved _ClaudeScratch/SolidArcWorkspace/KnifeRecovery.solidarc
- Block: move up, click the height
- Knife1 · Block · Difference (Block1) · 2 ms
- Cylinder: click the base centre · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Knife · Cylinder cutter · click the base centre on a face (live dimensions) · the height goes into the body (Q: out of it) · the cut shows as you pull · Q / W / E / R · K keep · Esc cancels
- Cylinder: click the radius
- Cylinder: move up, click the height
- Autosaved _ClaudeScratch/SolidArcWorkspace/KnifeRecovery.solidarc
- Knife: the cutter stays as a body (K)
- Knife2 · Cylinder · Difference (Block1) · cutter kept · 5 ms
- Sphere: click the centre · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Knife · Sphere cutter · click the centre on a face (live dimensions) · the height goes into the body (Q: out of it) · the cut shows as you pull · Q / W / E / R · K keep · Esc cancels
- Sphere: click the radius
- Autosaved _ClaudeScratch/SolidArcWorkspace/KnifeRecovery.solidarc
- Knife3 · Sphere · Difference (Block1) · 7 ms
- Knife · Insert Triangle · point at a face · press for the centre, drag out the size and turn, release, then move for the depth · 9 insert(s) in 75 ms
- Knife · Insert Triangle · point at a face · press for the centre, drag out the size and turn, release, then move for the depth · 9 insert(s) in 75 ms
- Knife · Triangle centred on Block1 · drag out its size and turn (Ctrl free) · release for the depth
- Knife · Triangle width 0.4 m · 0° · move along the normal for the depth (Ctrl free), T through, or type it · click / right-click / Enter cuts · Esc lifts
- Autosaved _ClaudeScratch/SolidArcWorkspace/KnifeRecovery.solidarc
- Knife4 · Triangle · Difference · 15 ms
- Knife · Insert Round hole · point at a face · press for the centre, drag out the size and turn, release, then move for the depth · 9 insert(s) in 75 ms
- Knife · Insert Round hole · point at a face · press for the centre, drag out the size and turn, release, then move for the depth · 9 insert(s) in 75 ms
- Autosaved _ClaudeScratch/SolidArcWorkspace/KnifeRecovery.solidarc
- Knife · Round hole centred on Block1 · drag out its size and turn (Ctrl free) · release for the depth
- Knife · Round hole width 0.2 m · 0° · move along the normal for the depth (Ctrl free), T through, or type it · click / right-click / Enter cuts · Esc lifts
- Knife: insert straight through
- Knife5 · Round hole · Difference · 11 ms
