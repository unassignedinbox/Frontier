# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 900 · 1208 frames · 136.871 s · slowest frame 576.683 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/KnifeWalkthrough.txt
- Expectations: 179 / 179 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 32 | `refused == 0` | 0 | 🟢 |
| 33 | `bodies == 1` | 1 | 🟢 |
| 34 | `volume ~ 8` | 8 | 🟢 |
| 40 | `knife == 1` | 1 | 🟢 |
| 41 | `knife-pending == 1` | 1 | 🟢 |
| 42 | `knife-mode == Sketch` | Sketch | 🟢 |
| 45 | `face-plane == 1` | Extrude#2.Cap(End) | 🟢 |
| 52 | `knife-pending == 0` | 0 | 🟢 |
| 53 | `sketch-filled == 1` | 1 | 🟢 |
| 56 | `knife-depth ~ 0.5` | 0.5 | 🟢 |
| 57 | `knife-target == Block1` | Block1 | 🟢 |
| 58 | `knife-refusal == 0` |  | 🟢 |
| 62 | `refused == 0` | 0 | 🟢 |
| 63 | `knife == 0` | 0 | 🟢 |
| 64 | `knife-commits == 1` | 1 | 🟢 |
| 65 | `bodies == 1` | 1 | 🟢 |
| 66 | `volume ~ 7.858628330588459` | 7.858628331 | 🟢 |
| 81 | `knife-pending == 0` | 0 | 🟢 |
| 84 | `knife-through == 1` | 1 | 🟢 |
| 88 | `refused == 0` | 0 | 🟢 |
| 89 | `knife-commits == 2` | 2 | 🟢 |
| 90 | `volume ~ 7.538628330588459` | 7.538628331 | 🟢 |
| 98 | `knife-mode == Insert` | Insert | 🟢 |
| 99 | `knife-pending == 0` | 0 | 🟢 |
| 100 | `knife-inserts > 6` | 10 | 🟢 |
| 101 | `knife-insert == Square pocket` | Square pocket | 🟢 |
| 106 | `knife-hover == 1` | 1 | 🟢 |
| 109 | `knife-size ~ 0.4` | 0.4 | 🟢 |
| 112 | `knife-placed == 1` | 1 | 🟢 |
| 115 | `knife-turn ~ 15` | 15 | 🟢 |
| 119 | `refused == 0` | 0 | 🟢 |
| 120 | `knife-commits == 3` | 3 | 🟢 |
| 121 | `bodies == 1` | 1 | 🟢 |
| 122 | `volume ~ 7.519428330588459` | 7.519428331 | 🟢 |
| 135 | `knife-hover == 1` | 1 | 🟢 |
| 142 | `refused == 0` | 0 | 🟢 |
| 143 | `knife-commits == 4` | 4 | 🟢 |
| 144 | `bodies == 1` | 1 | 🟢 |
| 145 | `volume ~ 7.538628330588459` | 7.538628331 | 🟢 |
| 164 | `refused == 0` | 0 | 🟢 |
| 165 | `knife-commits == 5` | 5 | 🟢 |
| 166 | `bodies == 2` | 2 | 🟢 |
| 167 | `volume ~ 7.561028330588459` | 7.561028331 | 🟢 |
| 168 | `selected-bodies contains Import` | Import#13 | 🟢 |
| 175 | `knife-inserts > 7` | 11 | 🟢 |
| 178 | `knife == 0` | 0 | 🟢 |
| 189 | `knife-mode == Object` | Object | 🟢 |
| 195 | `refused == 0` | 0 | 🟢 |
| 196 | `knife-commits == 6` | 6 | 🟢 |
| 197 | `bodies == 2` | 2 | 🟢 |
| 198 | `volume ~ 7.541828330588459` | 7.541828331 | 🟢 |
| 199 | `sound == 1` | 1 | 🟢 |
| 213 | `knife == 1` | 1 | 🟢 |
| 214 | `knife-mode == Sketch` | Sketch | 🟢 |
| 215 | `knife-pending == 1` | 1 | 🟢 |
| 216 | `drawing == Rectangle` | Rectangle | 🟢 |
| 225 | `knife-pending == 0` | 0 | 🟢 |
| 226 | `knife-through == 1` | 1 | 🟢 |
| 229 | `knife-through == 0` | 0 | 🟢 |
| 232 | `knife-depth ~ 0.1` | 0.1 | 🟢 |
| 235 | `refused == 0` | 0 | 🟢 |
| 236 | `knife-commits == 7` | 7 | 🟢 |
| 237 | `volume ~ 7.525828330588459` | 7.525828331 | 🟢 |
| 240 | `knife-mode == Sketch` | Sketch | 🟢 |
| 241 | `drawing == Rectangle` | Rectangle | 🟢 |
| 246 | `knife-mode == Insert` | Insert | 🟢 |
| 247 | `knife-insert == Square pocket` | Square pocket | 🟢 |
| 252 | `knife-mode == Sketch` | Sketch | 🟢 |
| 253 | `drawing == Circle` | Circle | 🟢 |
| 256 | `knife == 0` | 0 | 🟢 |
| 259 | `drawing == Circle` | Circle | 🟢 |
| 276 | `drawing == Circle` | Circle | 🟢 |
| 280 | `knife-pending == 0` | 0 | 🟢 |
| 281 | `knife-through == 0` | 0 | 🟢 |
| 286 | `knife-depth > 0.55` | 0.59 | 🟢 |
| 287 | `knife-depth < 0.65` | 0.59 | 🟢 |
| 288 | `knife-refusal == 0` |  | 🟢 |
| 292 | `knife == 0` | 0 | 🟢 |
| 293 | `volume ~ 7.525828330588459` | 7.525828331 | 🟢 |
| 303 | `knife == 1` | 1 | 🟢 |
| 304 | `knife-pending == 1` | 1 | 🟢 |
| 305 | `drawing == Line` | Line | 🟢 |
| 314 | `knife-pending == 0` | 0 | 🟢 |
| 315 | `knife-slice == 1` | 1 | 🟢 |
| 316 | `knife-refusal == 0` |  | 🟢 |
| 319 | `knife-refusal == 0` |  | 🟢 |
| 323 | `refused == 0` | 0 | 🟢 |
| 324 | `knife == 0` | 0 | 🟢 |
| 325 | `knife-commits == 8` | 8 | 🟢 |
| 326 | `bodies == 3` | 3 | 🟢 |
| 327 | `volume ~ 7.525828330588459` | 7.525828331 | 🟢 |
| 332 | `selected-faces > 0` | Extrude#2.Cap(End) | 🟢 |
| 339 | `selected-bodies > 0` | Cut#21 | 🟢 |
| 351 | `selected-bodies == 1` | Import#13 | 🟢 |
| 354 | `knife == 1` | 1 | 🟢 |
| 355 | `knife-mode == Object` | Object | 🟢 |
| 356 | `knife-placed == 0` | 0 | 🟢 |
| 359 | `knife-hover == 1` | 1 | 🟢 |
| 360 | `knife-align == 1` | 1 | 🟢 |
| 361 | `knife-rigid == 1` | 1 | 🟢 |
| 362 | `knife-target == Block1` | Block1 | 🟢 |
| 368 | `knife-placed == 1` | 1 | 🟢 |
| 369 | `knife-align == 1` | 1 | 🟢 |
| 372 | `knife-sink < -0.1` | -0.25 | 🟢 |
| 375 | `knife-sink ~ 0.03` | 0.03 | 🟢 |
| 376 | `knife-refusal == 0` |  | 🟢 |
| 380 | `refused == 0` | 0 | 🟢 |
| 381 | `knife == 0` | 0 | 🟢 |
| 382 | `knife-commits == 9` | 9 | 🟢 |
| 383 | `bodies == 2` | 2 | 🟢 |
| 384 | `volume ~ 7.487428330588459` | 7.487428332 | 🟢 |
| 395 | `knife-pending == 1` | 1 | 🟢 |
| 398 | `knife-pending == 0` | 0 | 🟢 |
| 399 | `sketch-filled == 1` | 1 | 🟢 |
| 402 | `knife-depth ~ 0.1` | 0.1 | 🟢 |
| 405 | `refused == 0` | 0 | 🟢 |
| 406 | `knife == 0` | 0 | 🟢 |
| 407 | `knife-commits == 10` | 10 | 🟢 |
| 408 | `volume < 7.487428330588459` | 7.481428332 | 🟢 |
| 419 | `knife-pending == 0` | 0 | 🟢 |
| 422 | `knife-depth ~ 0.05` | 0.05 | 🟢 |
| 425 | `refused == 0` | 0 | 🟢 |
| 426 | `knife-commits == 11` | 11 | 🟢 |
| 431 | `drawing == Polygon` | Polygon | 🟢 |
| 434 | `drawing == Polygon` | Polygon | 🟢 |
| 435 | `knife-pending == 1` | 1 | 🟢 |
| 438 | `drawing-sides == 5` | 5 | 🟢 |
| 442 | `knife-pending == 0` | 0 | 🟢 |
| 447 | `refused == 0` | 0 | 🟢 |
| 448 | `knife-commits == 12` | 12 | 🟢 |
| 473 | `bodies == 1` | 1 | 🟢 |
| 474 | `volume ~ 8` | 8 | 🟢 |
| 481 | `knife == 1` | 1 | 🟢 |
| 482 | `knife-pending == 1` | 1 | 🟢 |
| 483 | `drawing-solid == 1` | 1 | 🟢 |
| 484 | `drawing-solid-kind == Block` | Block | 🟢 |
| 493 | `drawing-lift ~ -0.3` | -0.3 | 🟢 |
| 494 | `knife-target == Block1` | Block1 | 🟢 |
| 498 | `refused == 0` | 0 | 🟢 |
| 499 | `knife == 0` | 0 | 🟢 |
| 500 | `knife-commits == 13` | 13 | 🟢 |
| 501 | `bodies == 1` | 1 | 🟢 |
| 502 | `volume ~ 7.952` | 7.952 | 🟢 |
| 507 | `drawing-solid-kind == Cylinder` | Cylinder | 🟢 |
| 516 | `drawing-lift ~ -0.5` | -0.5 | 🟢 |
| 521 | `refused == 0` | 0 | 🟢 |
| 522 | `knife-commits == 14` | 14 | 🟢 |
| 523 | `bodies == 2` | 2 | 🟢 |
| 524 | `volume ~ 7.954513274122872` | 7.954513274 | 🟢 |
| 529 | `drawing-solid-kind == Sphere` | Sphere | 🟢 |
| 534 | `knife-target == Block1` | Block1 | 🟢 |
| 538 | `refused == 0` | 0 | 🟢 |
| 539 | `knife-commits == 15` | 15 | 🟢 |
| 540 | `bodies == 2` | 2 | 🟢 |
| 541 | `volume < 7.95` | 7.897964606 | 🟢 |
| 552 | `knife-mode == Insert` | Insert | 🟢 |
| 553 | `knife-insert == Triangle` | Triangle | 🟢 |
| 558 | `knife-hover == 1` | 1 | 🟢 |
| 561 | `knife-placed == 1` | 1 | 🟢 |
| 562 | `knife-sizing == 1` | 1 | 🟢 |
| 567 | `knife-size ~ 0.4` | 0.4 | 🟢 |
| 568 | `knife-turn ~ 0` | 0 | 🟢 |
| 572 | `knife-sizing == 0` | 0 | 🟢 |
| 573 | `knife-placed == 1` | 1 | 🟢 |
| 576 | `knife-insert-depth > 0.15` | 0.25 | 🟢 |
| 580 | `knife-insert-depth ~ 0.2` | 0.2 | 🟢 |
| 583 | `refused == 0` | 0 | 🟢 |
| 584 | `knife == 0` | 0 | 🟢 |
| 585 | `knife-commits == 16` | 16 | 🟢 |
| 586 | `knife-removed ~ 0.013856406460551` | 0.01385640646 | 🟢 |
| 591 | `knife-insert == Round hole` | Round hole | 🟢 |
| 596 | `knife-hover == 1` | 1 | 🟢 |
| 599 | `knife-size ~ 0.2` | 0.2 | 🟢 |
| 602 | `knife-placed == 1` | 1 | 🟢 |
| 603 | `knife-sizing == 0` | 0 | 🟢 |
| 606 | `knife-insert-through == 1` | 1 | 🟢 |
| 609 | `refused == 0` | 0 | 🟢 |
| 610 | `knife-commits == 17` | 17 | 🟢 |
| 611 | `knife-removed ~ 0.0628318530718` | 0.06283185307 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 271KnifeSketchStaged | 93 | `485a6366f0ace7b2` |
| 272KnifeThrough | 145 | `b37a30f000f1f24b` |
| 273KnifeInsertPlaced | 193 | `a7a6a7f5ffd99862` |
| 274KnifeObject | 339 | `1f688c841c06be1b` |
| 275KnifeToolMenu | 367 | `6b5768d1ce1bcd91` |
| 281KnifeDrawingDimensions | 518 | `5ca2f70cf16ea6f9` |
| 282KnifeCutterDimensions | 540 | `b7d2dbd0f1e9bd8c` |
| 283KnifeLineSlice | 599 | `e9f8a33b4600c61b` |
| 285KnifeCutterFollows | 660 | `fb6d1cddb5d77d20` |
| 286KnifeCutterPlaced | 684 | `3f9bf7f8b64db356` |
| 287KnifePolygonSides | 840 | `aaa18a5b1bf1327f` |
| 288KnifeBoxCutter | 969 | `9991fe781af9a262` |
| 289KnifeSphereCutter | 1067 | `7d8ae4f91f179f9e` |
| 290KnifeInsertDrag | 1121 | `ac09e6f17f12334a` |
| 291KnifeInsertDepth | 1135 | `0b107a2c3fb60c7d` |

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
- Knife2 · Rectangle1 · Difference · depth 2.06 · 5 ms
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Knife · draw a Circle on a face (toolbar menu or card: Circle / Rectangle / Polygon / Slot / Line) · then pull along the normal for the depth · Tab: Insert library · Esc cancels
- Knife · Insert Square pocket · point at a face · press for the centre, drag out the size and turn, release, then move for the depth · 10 insert(s) in 76 ms
- Knife · Square pocket centred on Block1 · drag out its size and turn (Ctrl free) · release for the depth
- Knife · Square pocket width 0.4 m · 0° · move along the normal for the depth (Ctrl free), T through, or type it · click / right-click / Enter cuts · Esc lifts
- Knife: turn 15°
- Autosaved _ClaudeScratch/SolidArcWorkspace/KnifeRecovery.solidarc
- Knife3 · Square pocket · Difference · 7 ms
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Knife · draw a Circle on a face (toolbar menu or card: Circle / Rectangle / Polygon / Slot / Line) · then pull along the normal for the depth · Tab: Insert library · Esc cancels
- Knife · Insert Square pocket · point at a face · press for the centre, drag out the size and turn, release, then move for the depth · 10 insert(s) in 76 ms
- Knife: Add
- Knife · Square pocket centred on Block1 · drag out its size and turn (Ctrl free) · release for the depth
- Knife · Square pocket width 0.4 m · 0° · move along the normal for the depth (Ctrl free), T through, or type it · click / right-click / Enter cuts · Esc lifts
- Autosaved _ClaudeScratch/SolidArcWorkspace/KnifeRecovery.solidarc
- Knife4 · Square pocket · Union · 6 ms
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Knife · draw a Circle on a face (toolbar menu or card: Circle / Rectangle / Polygon / Slot / Line) · then pull along the normal for the depth · Tab: Insert library · Esc cancels
- Knife · Insert Square pocket · point at a face · press for the centre, drag out the size and turn, release, then move for the depth · 10 insert(s) in 76 ms
- Knife: Separate
- Knife · Square pocket centred on Block1 · drag out its size and turn (Ctrl free) · release for the depth
- Knife · Square pocket width 0.4 m · 0° · move along the normal for the depth (Ctrl free), T through, or type it · click / right-click / Enter cuts · Esc lifts
- Autosaved _ClaudeScratch/SolidArcWorkspace/KnifeRecovery.solidarc
- Knife5 · Square pocket · New body · 1 ms
- Knife · Square pocket3 follows the pointer onto a face (Ctrl: along its normal · [ / ] turn) · click / right-click places it · then in / out along the normal · click / right-click / Enter cuts
- Knife · Insert Square pocket · point at a face · press for the centre, drag out the size and turn, release, then move for the depth · 10 insert(s) in 76 ms
- Insert saved: _ClaudeScratch/SolidArcWorkspace/KnifeInserts/Squarepocket34.step · library 11 insert(s)
- Knife cancelled (document untouched)
- Selected Block1 (22 faces)
- Autosaved _ClaudeScratch/SolidArcWorkspace/KnifeRecovery.solidarc
- Added Square pocket3 · 2 bodies picked
- Knife · Object · the first picked body is cut by the others (Shift-pick cutters) · K keeps the cutters · click / right-click / Enter applies
- Knife: cutters kept
- Knife5 · Difference (Block1 − Square pocket3) · cutters kept · 8 ms
- Selection cleared
- Rectangle on plane XY (or the body face under the first click): click two corners · Alt centre mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Knife · draw a Rectangle on a face (toolbar menu or card: Circle / Rectangle / Polygon / Slot / Line) · then pull along the normal for the depth · Tab: Insert library · Esc cancels
- Knife · Rectangle · draw it on a face (live W / H / R dimensions), then pull along the normal for the depth (T through)
- Autosaved _ClaudeScratch/SolidArcWorkspace/KnifeRecovery.solidarc
- Rectangle2 · 1 ms
- Knife · Rectangle2 into Block1 · pull along the face normal for the depth (Ctrl free) or type it · T through · Q add · W cut · E intersect · R separate · click / right-click / Enter applies
- Knife: to depth
- Knife6 · Rectangle2 · Difference · depth 0.1 · 8 ms
- Rectangle on plane XY (or the body face under the first click): click two corners · Alt centre mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Knife · draw a Rectangle on a face (toolbar menu or card: Circle / Rectangle / Polygon / Slot / Line) · then pull along the normal for the depth · Tab: Insert library · Esc cancels
- Knife · Insert Square pocket · point at a face · press for the centre, drag out the size and turn, release, then move for the depth · 11 insert(s) in 83 ms
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
- Cut (Block1 by Line1) · 27 ms
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
- Knife8 · Rectangle3 · Difference · depth 0.1 · 9 ms
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Knife · draw a Circle on a face (toolbar menu or card: Circle / Rectangle / Polygon / Slot / Line) · then pull along the normal for the depth · Tab: Insert library · Esc cancels
- Knife · Circle · draw it on a face (live W / H / R dimensions), then pull along the normal for the depth (T through)
- Circle3 · 2 ms
- Knife · Circle3 into Block1 · pull along the face normal for the depth (Ctrl free) or type it · T through · Q add · W cut · E intersect · R separate · click / right-click / Enter applies
- Knife9 · Circle3 · Difference · depth 0.05 · 15 ms
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
- Knife · Insert Triangle · point at a face · press for the centre, drag out the size and turn, release, then move for the depth · 11 insert(s) in 83 ms
- Knife · Insert Triangle · point at a face · press for the centre, drag out the size and turn, release, then move for the depth · 11 insert(s) in 83 ms
- Knife · Triangle centred on Block1 · drag out its size and turn (Ctrl free) · release for the depth
- Knife · Triangle width 0.4 m · 0° · move along the normal for the depth (Ctrl free), T through, or type it · click / right-click / Enter cuts · Esc lifts
- Autosaved _ClaudeScratch/SolidArcWorkspace/KnifeRecovery.solidarc
- Knife4 · Triangle · Difference · 8 ms
- Knife · Insert Round hole · point at a face · press for the centre, drag out the size and turn, release, then move for the depth · 11 insert(s) in 83 ms
- Knife · Insert Round hole · point at a face · press for the centre, drag out the size and turn, release, then move for the depth · 11 insert(s) in 83 ms
- Autosaved _ClaudeScratch/SolidArcWorkspace/KnifeRecovery.solidarc
- Knife · Round hole centred on Block1 · drag out its size and turn (Ctrl free) · release for the depth
- Knife · Round hole width 0.2 m · 0° · move along the normal for the depth (Ctrl free), T through, or type it · click / right-click / Enter cuts · Esc lifts
- Knife: insert straight through
- Knife5 · Round hole · Difference · 10 ms
