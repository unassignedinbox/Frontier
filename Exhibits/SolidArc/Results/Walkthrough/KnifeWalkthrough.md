# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 810 · 1616 frames · 123.789 s · slowest frame 332.73 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/KnifeWalkthrough.txt
- Expectations: 233 / 233 passed

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
| 100 | `knife-inserts > 6` | 12 | 🟢 |
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
| 175 | `knife-inserts > 7` | 13 | 🟢 |
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
| 372 | `knife-sink < -0.1` | -0.26 | 🟢 |
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
| 568 | `knife-aspect > 1.1` | 1.154700559 | 🟢 |
| 569 | `knife-turn ~ 0` | 0 | 🟢 |
| 573 | `knife-sizing == 0` | 0 | 🟢 |
| 574 | `knife-placed == 1` | 1 | 🟢 |
| 577 | `knife-insert-depth > 0.15` | 0.22 | 🟢 |
| 581 | `knife-insert-depth ~ 0.2` | 0.2 | 🟢 |
| 584 | `refused == 0` | 0 | 🟢 |
| 585 | `knife == 0` | 0 | 🟢 |
| 586 | `knife-commits == 16` | 16 | 🟢 |
| 587 | `knife-removed ~ 0.016` | 0.01600000029 | 🟢 |
| 592 | `knife-insert == Round hole` | Round hole | 🟢 |
| 597 | `knife-hover == 1` | 1 | 🟢 |
| 600 | `knife-size ~ 0.2` | 0.2 | 🟢 |
| 603 | `knife-placed == 1` | 1 | 🟢 |
| 604 | `knife-sizing == 1` | 1 | 🟢 |
| 605 | `knife-size-by-move == 1` | 1 | 🟢 |
| 606 | `knife-size ~ 0.2` | 0.2 | 🟢 |
| 609 | `knife-insert-through == 1` | 1 | 🟢 |
| 612 | `refused == 0` | 0 | 🟢 |
| 613 | `knife-commits == 17` | 17 | 🟢 |
| 614 | `knife-removed ~ 0.0628318530718` | 0.06283185307 | 🟢 |
| 637 | `bodies == 1` | 1 | 🟢 |
| 638 | `volume ~ 8` | 8 | 🟢 |
| 643 | `knife == 1` | 1 | 🟢 |
| 644 | `knife-pending == 1` | 1 | 🟢 |
| 645 | `drawing == Line` | Line | 🟢 |
| 662 | `knife-pending == 1` | 1 | 🟢 |
| 665 | `knife-pending == 0` | 0 | 🟢 |
| 666 | `knife-slice == 1` | 1 | 🟢 |
| 667 | `knife-refusal == 0` |  | 🟢 |
| 671 | `refused == 0` | 0 | 🟢 |
| 672 | `knife == 0` | 0 | 🟢 |
| 673 | `bodies == 2` | 2 | 🟢 |
| 674 | `volume ~ 8` | 8 | 🟢 |
| 685 | `selected-edges == 6` | Edge(Extrude#2.Cap(End)/Extrude#2.Side(Line4)),Edge(Extrude#2.Cap(End)/Extrude#2.Side(L... | 🟢 |
| 689 | `selected-edges == 4` | Edge(Extrude#2.Cap(End)/Extrude#2.Side(Line4)),Edge(Extrude#2.Cap(Start)/Extrude#2.Side... | 🟢 |
| 692 | `selected-edges == 2` | Edge(Extrude#2.Cap(Start)/Extrude#2.Side(Line4)),Edge(Cut#4.Side(Line2)/Extrude#2.Cap(S... | 🟢 |
| 695 | `selected-edges == 8` | Edge(Extrude#2.Cap(Start)/Extrude#2.Side(Line4)),Edge(Cut#4.Side(Line2)/Extrude#2.Cap(S... | 🟢 |
| 725 | `bodies == 1` | 1 | 🟢 |
| 726 | `volume ~ 8` | 8 | 🟢 |
| 731 | `knife-insert == Square pocket` | Square pocket | 🟢 |
| 736 | `knife-hover == 1` | 1 | 🟢 |
| 739 | `knife-sizing == 1` | 1 | 🟢 |
| 740 | `knife-size-by-move == 1` | 1 | 🟢 |
| 745 | `knife-size ~ 0.6` | 0.6 | 🟢 |
| 746 | `knife-aspect > 0.3` | 0.3333333333 | 🟢 |
| 747 | `knife-aspect < 0.37` | 0.3333333333 | 🟢 |
| 751 | `knife-sizing == 0` | 0 | 🟢 |
| 752 | `knife-placed == 1` | 1 | 🟢 |
| 755 | `knife-insert-depth ~ 0.1` | 0.1 | 🟢 |
| 758 | `refused == 0` | 0 | 🟢 |
| 759 | `knife == 0` | 0 | 🟢 |
| 760 | `knife-removed > 0.0114` | 0.012 | 🟢 |
| 761 | `knife-removed < 0.0127` | 0.012 | 🟢 |
| 766 | `drawing == Line` | Line | 🟢 |
| 785 | `knife-slice == 1` | 1 | 🟢 |
| 786 | `knife-refusal == 0` |  | 🟢 |
| 791 | `knife-blends == 1` | 1 | 🟢 |
| 792 | `knife-blend-shape == 0` | 0 | 🟢 |
| 795 | `knife-blend-size ~ 0.1` | 0.1 | 🟢 |
| 800 | `knife-blends == 2` | 2 | 🟢 |
| 801 | `knife-blend-focus == 2` | 2 | 🟢 |
| 804 | `knife-blend-shape == 1` | 1 | 🟢 |
| 807 | `knife-blend-size ~ 0.05` | 0.05 | 🟢 |
| 808 | `knife-refusal == 0` |  | 🟢 |
| 812 | `refused == 0` | 0 | 🟢 |
| 813 | `knife == 0` | 0 | 🟢 |
| 814 | `bodies == 2` | 2 | 🟢 |
| 815 | `knife-removed ~ 0.006792036732051` | 0.006792036732 | 🟢 |
| 818 | `bodies == 1` | 1 | 🟢 |
| 819 | `volume > 7.9873` | 7.988 | 🟢 |
| 820 | `volume < 7.9886` | 7.988 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 271KnifeSketchStaged | 93 | `5c8ad605dc1bd4eb` |
| 272KnifeThrough | 145 | `7b4e449f341f947c` |
| 273KnifeInsertPlaced | 193 | `8be7475548350c22` |
| 274KnifeObject | 339 | `a8358fe6cc0d56d7` |
| 275KnifeToolMenu | 367 | `b53389a18ba40cc7` |
| 281KnifeDrawingDimensions | 506 | `02804400e9c23ef3` |
| 282KnifeCutterDimensions | 528 | `52b0040f1d4184ea` |
| 283KnifeLineSlice | 583 | `90e03c2c6d0b1237` |
| 285KnifeCutterFollows | 644 | `1274f273b034e5e0` |
| 286KnifeCutterPlaced | 668 | `28e74dd35d22db11` |
| 287KnifePolygonSides | 812 | `7734172b0f8c864a` |
| 288KnifeBoxCutter | 941 | `81f9a56c19eb50de` |
| 289KnifeSphereCutter | 1039 | `acacdedfe3b8f392` |
| 290KnifeInsertDrag | 1093 | `0504662c43bcfdb9` |
| 291KnifeInsertDepth | 1107 | `8d608c7edb462d08` |
| 299KnifePolyline | 1301 | `f0d483e0c36c9a1c` |
| 300EdgeLoop | 1325 | `038d56ba2668df79` |
| 301KnifeInsertBase | 1454 | `abaf074c82e2e051` |
| 302KnifeSliceBlends | 1603 | `11aa1515f6b9e25e` |

## Journal

- New document (previous recovery copy kept)
- Block: click the first corner · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Block: click the opposite corner
- Block: move up, click the height
- Block1 · New body · 0 ms
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Knife · draw a Circle on a face (toolbar menu or card: Circle / Rectangle / Polygon / Slot / Line / Polyline) · then pull along the normal for the depth · Tab: Insert library · Esc cancels
- Circle1 · 0 ms
- Knife · Circle1 into Block1 · pull along the face normal for the depth (Ctrl free) or type it · T through · Q add · W cut · E intersect · R separate · click / right-click / Enter applies
- Knife1 · Circle1 · Difference · depth 0.5 · 1 ms
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Knife · draw a Circle on a face (toolbar menu or card: Circle / Rectangle / Polygon / Slot / Line / Polyline) · then pull along the normal for the depth · Tab: Insert library · Esc cancels
- Rectangle on plane XY (or the body face under the first click): click two corners · Alt centre mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Rectangle1 · 0 ms
- Knife · Rectangle1 into Block1 · pull along the face normal for the depth (Ctrl free) or type it · T through · Q add · W cut · E intersect · R separate · click / right-click / Enter applies
- Knife: through
- Knife2 · Rectangle1 · Difference · depth 2.06 · 2 ms
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Knife · draw a Circle on a face (toolbar menu or card: Circle / Rectangle / Polygon / Slot / Line / Polyline) · then pull along the normal for the depth · Tab: Insert library · Esc cancels
- Knife · Insert Square pocket · point at a face · press for the centre, drag out the size and turn, release, then move for the depth · 12 insert(s) in 63 ms
- Knife · Square pocket base corner on Block1 · drag to the opposite corner (Shift square, Ctrl free) · release for the depth
- Knife · move to the opposite base corner (Shift square, Ctrl free) · click to set it, then the depth
- Knife: turn 15°
- Autosaved _ClaudeScratch/SolidArcWorkspace/KnifeRecovery.solidarc
- Knife3 · Square pocket · Difference · 2 ms
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Knife · draw a Circle on a face (toolbar menu or card: Circle / Rectangle / Polygon / Slot / Line / Polyline) · then pull along the normal for the depth · Tab: Insert library · Esc cancels
- Knife · Insert Square pocket · point at a face · press for the centre, drag out the size and turn, release, then move for the depth · 12 insert(s) in 63 ms
- Knife: Add
- Knife · Square pocket base corner on Block1 · drag to the opposite corner (Shift square, Ctrl free) · release for the depth
- Knife · move to the opposite base corner (Shift square, Ctrl free) · click to set it, then the depth
- Autosaved _ClaudeScratch/SolidArcWorkspace/KnifeRecovery.solidarc
- Knife4 · Square pocket · Union · 3 ms
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Knife · draw a Circle on a face (toolbar menu or card: Circle / Rectangle / Polygon / Slot / Line / Polyline) · then pull along the normal for the depth · Tab: Insert library · Esc cancels
- Knife · Insert Square pocket · point at a face · press for the centre, drag out the size and turn, release, then move for the depth · 12 insert(s) in 63 ms
- Knife: Separate
- Knife · Square pocket base corner on Block1 · drag to the opposite corner (Shift square, Ctrl free) · release for the depth
- Knife · move to the opposite base corner (Shift square, Ctrl free) · click to set it, then the depth
- Autosaved _ClaudeScratch/SolidArcWorkspace/KnifeRecovery.solidarc
- Knife5 · Square pocket · New body · 1 ms
- Knife · Square pocket3 follows the pointer onto a face (Ctrl: along its normal · [ / ] turn) · click / right-click places it · then in / out along the normal · click / right-click / Enter cuts
- Knife · Insert Square pocket · point at a face · press for the centre, drag out the size and turn, release, then move for the depth · 12 insert(s) in 63 ms
- Insert saved: _ClaudeScratch/SolidArcWorkspace/KnifeInserts/Squarepocket36.step · library 13 insert(s)
- Knife cancelled (document untouched)
- Selected Block1 (22 faces)
- Autosaved _ClaudeScratch/SolidArcWorkspace/KnifeRecovery.solidarc
- Added Square pocket3 · 2 bodies picked
- Knife · Object · the first picked body is cut by the others (Shift-pick cutters) · K keeps the cutters · click / right-click / Enter applies
- Knife: cutters kept
- Knife5 · Difference (Block1 − Square pocket3) · cutters kept · 3 ms
- Selection cleared
- Rectangle on plane XY (or the body face under the first click): click two corners · Alt centre mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Knife · draw a Rectangle on a face (toolbar menu or card: Circle / Rectangle / Polygon / Slot / Line / Polyline) · then pull along the normal for the depth · Tab: Insert library · Esc cancels
- Knife · Rectangle · draw it on a face (live W / H / R dimensions), then pull along the normal for the depth (T through)
- Autosaved _ClaudeScratch/SolidArcWorkspace/KnifeRecovery.solidarc
- Rectangle2 · 1 ms
- Knife · Rectangle2 into Block1 · pull along the face normal for the depth (Ctrl free) or type it · T through · Q add · W cut · E intersect · R separate · click / right-click / Enter applies
- Knife: to depth
- Knife6 · Rectangle2 · Difference · depth 0.1 · 3 ms
- Rectangle on plane XY (or the body face under the first click): click two corners · Alt centre mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Knife · draw a Rectangle on a face (toolbar menu or card: Circle / Rectangle / Polygon / Slot / Line / Polyline) · then pull along the normal for the depth · Tab: Insert library · Esc cancels
- Knife · Insert Square pocket · point at a face · press for the centre, drag out the size and turn, release, then move for the depth · 13 insert(s) in 78 ms
- Autosaved _ClaudeScratch/SolidArcWorkspace/KnifeRecovery.solidarc
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Knife · Circle · draw it on a face (live W / H / R dimensions), then pull along the normal for the depth (T through)
- Circle aborted (document untouched)
- Knife cancelled (document untouched)
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Knife · draw a Circle on a face (toolbar menu or card: Circle / Rectangle / Polygon / Slot / Line / Polyline) · then pull along the normal for the depth · Tab: Insert library · Esc cancels
- Circle aborted (document untouched)
- Knife cancelled (document untouched)
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Knife · draw a Circle on a face (toolbar menu or card: Circle / Rectangle / Polygon / Slot / Line / Polyline) · then pull along the normal for the depth · Tab: Insert library · Esc cancels
- Knife · Circle · draw it on a face (live W / H / R dimensions), then pull along the normal for the depth (T through)
- Circle2 · 1 ms
- Knife · Circle2 into Block1 · pull along the face normal for the depth (Ctrl free) or type it · T through · Q add · W cut · E intersect · R separate · click / right-click / Enter applies
- Knife cancelled (document untouched)
- Knife · Sketch Circle2 · pull along the face normal for the depth (as Construct heights) · T through
- Line on plane XY (or the body face under the first click): click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Knife · Line · draw it on a face (live W / H / R dimensions), then pull along the normal for the depth (T through)
- Autosaved _ClaudeScratch/SolidArcWorkspace/KnifeRecovery.solidarc
- Line1 · 1 ms
- Knife · Line1 slices Block1 through (two bodies) · click edges of the pieces to fillet / chamfer them (D / C, type a size) · right-click / Enter splits it · Esc cancels
- Cut (Block1 by Line1) · 7 ms
- Selected Extrude#2.Cap(End)
- Selection cleared
- Selected Cut1 (6 faces)
- Selection cleared
- Autosaved _ClaudeScratch/SolidArcWorkspace/KnifeRecovery.solidarc
- Selected Square pocket3 (6 faces)
- Knife · Square pocket3 follows the pointer onto a face (Ctrl: along its normal · [ / ] turn) · click / right-click places it · then in / out along the normal · click / right-click / Enter cuts
- Knife · Square pocket3 placed on Block1 · move the pointer along the normal to push it in / pull it out (Ctrl free) or type a depth · click / right-click / Enter cuts · Esc lifts it
- Knife7 · Difference (Block1 − Square pocket3 placed) · 4 ms
- Selection cleared
- Rectangle on plane XY (or the body face under the first click): click two corners · Alt centre mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Knife · draw a Rectangle on a face (toolbar menu or card: Circle / Rectangle / Polygon / Slot / Line / Polyline) · then pull along the normal for the depth · Tab: Insert library · Esc cancels
- Knife · Rectangle · draw it on a face (live W / H / R dimensions), then pull along the normal for the depth (T through)
- Rectangle3 · 2 ms
- Knife · Rectangle3 into Block1 · pull along the face normal for the depth (Ctrl free) or type it · T through · Q add · W cut · E intersect · R separate · click / right-click / Enter applies
- Knife8 · Rectangle3 · Difference · depth 0.1 · 5 ms
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Knife · draw a Circle on a face (toolbar menu or card: Circle / Rectangle / Polygon / Slot / Line / Polyline) · then pull along the normal for the depth · Tab: Insert library · Esc cancels
- Knife · Circle · draw it on a face (live W / H / R dimensions), then pull along the normal for the depth (T through)
- Circle3 · 2 ms
- Knife · Circle3 into Block1 · pull along the face normal for the depth (Ctrl free) or type it · T through · Q add · W cut · E intersect · R separate · click / right-click / Enter applies
- Knife9 · Circle3 · Difference · depth 0.05 · 5 ms
- Polygon on plane XY (or the body face under the first click): click the centre, then a vertex · + / − sides · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Knife · draw a Polygon on a face (toolbar menu or card: Circle / Rectangle / Polygon / Slot / Line / Polyline) · then pull along the normal for the depth · Tab: Insert library · Esc cancels
- Knife · Polygon · draw it on a face (live W / H / R dimensions), then pull along the normal for the depth (T through)
- Polygon · 6 sides · wheel (or + / −) changes the sides · click / right-click / Enter confirms · Esc aborts
- 5 sides
- Autosaved _ClaudeScratch/SolidArcWorkspace/KnifeRecovery.solidarc
- Polygon1 · 2 ms
- Knife · Polygon1 into Block1 · pull along the face normal for the depth (Ctrl free) or type it · T through · Q add · W cut · E intersect · R separate · click / right-click / Enter applies
- Knife10 · Polygon1 · Difference · depth 0.05 · 5 ms
- Selection cleared
- New document (previous recovery copy kept)
- Block: click the first corner · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Block: click the opposite corner
- Autosaved _ClaudeScratch/SolidArcWorkspace/KnifeRecovery.solidarc
- Block: move up, click the height
- Block1 · New body · 0 ms
- Block: click the first corner · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Knife · Block cutter · click the first corner on a face (live dimensions) · the height goes into the body (Q: out of it) · the cut shows as you pull · Q / W / E / R · K keep · Esc cancels
- Block: click the opposite corner
- Autosaved _ClaudeScratch/SolidArcWorkspace/KnifeRecovery.solidarc
- Block: move up, click the height
- Knife1 · Block · Difference (Block1) · 1 ms
- Cylinder: click the base centre · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Knife · Cylinder cutter · click the base centre on a face (live dimensions) · the height goes into the body (Q: out of it) · the cut shows as you pull · Q / W / E / R · K keep · Esc cancels
- Cylinder: click the radius
- Cylinder: move up, click the height
- Autosaved _ClaudeScratch/SolidArcWorkspace/KnifeRecovery.solidarc
- Knife: the cutter stays as a body (K)
- Knife2 · Cylinder · Difference (Block1) · cutter kept · 2 ms
- Sphere: click the centre · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Knife · Sphere cutter · click the centre on a face (live dimensions) · the height goes into the body (Q: out of it) · the cut shows as you pull · Q / W / E / R · K keep · Esc cancels
- Sphere: click the radius
- Autosaved _ClaudeScratch/SolidArcWorkspace/KnifeRecovery.solidarc
- Knife3 · Sphere · Difference (Block1) · 3 ms
- Knife · Insert Triangle · point at a face · press for the centre, drag out the size and turn, release, then move for the depth · 13 insert(s) in 78 ms
- Knife · Insert Triangle · point at a face · press for the centre, drag out the size and turn, release, then move for the depth · 13 insert(s) in 78 ms
- Knife · Triangle base corner on Block1 · drag to the opposite corner (Shift square, Ctrl free) · release for the depth
- Knife · Triangle base 0.4 × 0.4619 m · 0° · move along the normal for the depth (Ctrl free), T through, or type it · click / right-click / Enter cuts · Esc lifts
- Autosaved _ClaudeScratch/SolidArcWorkspace/KnifeRecovery.solidarc
- Knife4 · Triangle · Difference · 3 ms
- Knife · Insert Round hole · point at a face · press for the centre, drag out the size and turn, release, then move for the depth · 13 insert(s) in 78 ms
- Knife · Insert Round hole · point at a face · press for the centre, drag out the size and turn, release, then move for the depth · 13 insert(s) in 78 ms
- Autosaved _ClaudeScratch/SolidArcWorkspace/KnifeRecovery.solidarc
- Knife · Round hole base corner on Block1 · drag to the opposite corner (Shift square, Ctrl free) · release for the depth
- Knife · move to the opposite base corner (Shift square, Ctrl free) · click to set it, then the depth
- Knife: insert straight through
- Knife5 · Round hole · Difference · 4 ms
- New document (previous recovery copy kept)
- Block: click the first corner · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Block: click the opposite corner
- Autosaved _ClaudeScratch/SolidArcWorkspace/KnifeRecovery.solidarc
- Block: move up, click the height
- Block1 · New body · 0 ms
- Line on plane XY (or the body face under the first click): click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Knife · draw a Polyline on a face (toolbar menu or card: Circle / Rectangle / Polygon / Slot / Line / Polyline) · then pull along the normal for the depth · Tab: Insert library · Esc cancels
- Knife · Polyline · click points across the body (Ctrl snaps) · Enter / right-click ends the chain · it slices the body along every segment
- Autosaved _ClaudeScratch/SolidArcWorkspace/KnifeRecovery.solidarc
- Line1 · 0 ms
- Knife · Line1 slices Block1 through (two bodies) · click edges of the pieces to fillet / chamfer them (D / C, type a size) · right-click / Enter splits it · Esc cancels
- Cut (Block1 by Line1) · 3 ms
- Edge loop: 6 edge(s) selected · Alt loop · Ctrl+Alt ring · Shift toggles · double-click tangent chain
- Edge ring: 4 edge(s) selected · Alt loop · Ctrl+Alt ring · Shift toggles · double-click tangent chain
- Edge loop: 6 edge(s) removed · Alt loop · Ctrl+Alt ring · Shift toggles · double-click tangent chain
- Autosaved _ClaudeScratch/SolidArcWorkspace/KnifeRecovery.solidarc
- Edge loop: 6 edge(s) added · Alt loop · Ctrl+Alt ring · Shift toggles · double-click tangent chain
- Selection cleared
- New document (previous recovery copy kept)
- Block: click the first corner · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Block: click the opposite corner
- Block: move up, click the height
- Block1 · New body · 0 ms
- Knife · Insert Square pocket · point at a face · press for the centre, drag out the size and turn, release, then move for the depth · 13 insert(s) in 78 ms
- Knife · Insert Square pocket · point at a face · press for the centre, drag out the size and turn, release, then move for the depth · 13 insert(s) in 78 ms
- Knife · Square pocket base corner on Block1 · drag to the opposite corner (Shift square, Ctrl free) · release for the depth
- Knife · move to the opposite base corner (Shift square, Ctrl free) · click to set it, then the depth
- Autosaved _ClaudeScratch/SolidArcWorkspace/KnifeRecovery.solidarc
- Knife · Square pocket base 0.6 × 0.2 m · 0° · move along the normal for the depth (Ctrl free), T through, or type it · click / right-click / Enter cuts · Esc lifts
- Knife1 · Square pocket · Difference · 1 ms
- Line on plane XY (or the body face under the first click): click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Knife · draw a Polyline on a face (toolbar menu or card: Circle / Rectangle / Polygon / Slot / Line / Polyline) · then pull along the normal for the depth · Tab: Insert library · Esc cancels
- Knife · Polyline · click points across the body (Ctrl snaps) · Enter / right-click ends the chain · it slices the body along every segment
- Autosaved _ClaudeScratch/SolidArcWorkspace/KnifeRecovery.solidarc
- Line1 · 0 ms
- Knife · Line1 slices Block1 through (two bodies) · click edges of the pieces to fillet / chamfer them (D / C, type a size) · right-click / Enter splits it · Esc cancels
- Knife · fillet 0.05 m on Edge(Extrude#2.Side(Line1)|Extrude#2.Side(Line2)) of Block1 (edge blend 1) · D fillet · C chamfer · type a size · more edges, then right-click / Enter
- Autosaved _ClaudeScratch/SolidArcWorkspace/KnifeRecovery.solidarc
- Knife · fillet 0.1 m on Edge(Extrude#2.Side(Line1)|Extrude#2.Side(Line4)) of Cut1 (edge blend 2) · D fillet · C chamfer · type a size · more edges, then right-click / Enter
- Knife · edge blend 2: chamfer 0.1 m
- Cut (Block1 by Line1) + 2 edge blend(s) · 6 ms
- Undo
