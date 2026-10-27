# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 900 · 697 frames · 66.1531 s · slowest frame 936.834 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/TrimSplitWalkthrough.txt
- Expectations: 75 / 75 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 16 | `steps == 0` | 0 | 🟢 |
| 36 | `steps == 2` | 2 | 🟢 |
| 37 | `volume ~ 2` | 2 | 🟢 |
| 38 | `bodies == 1` | 1 | 🟢 |
| 45 | `status contains focus a sketch step or select a curve` | Cut (C): focus a sketch step or select a curve (the cutter), pick the body in mode 4 (e... | 🟢 |
| 46 | `steps == 2` | 2 | 🟢 |
| 56 | `selected-edges == 4` | Edge(Extrude#2.Cap(End)/Extrude#2.Side(Line1)),Edge(Extrude#2.Cap(Start)/Extrude#2.Side... | 🟢 |
| 57 | `ring-picks == 1` | 1 | 🟢 |
| 61 | `selected-edges == 0` |  | 🟢 |
| 72 | `focused-caption == Line1` | Line1 | 🟢 |
| 75 | `refused == 0` | 0 | 🟢 |
| 76 | `steps == 4` | 4 | 🟢 |
| 77 | `bodies == 2` | 2 | 🟢 |
| 78 | `body-volumes contains Cut1 0.600000` | Extrude1 1.400000 · Cut1 0.600000 | 🟢 |
| 79 | `body-volumes contains 1.400000` | Extrude1 1.400000 · Cut1 0.600000 | 🟢 |
| 80 | `selected-bodies contains Cut#4` | Cut#4 | 🟢 |
| 81 | `pick-mode == 4` | 4 | 🟢 |
| 82 | `sound == 1` | 1 | 🟢 |
| 83 | `status contains Cut` | Cut (Extrude1 by Line1) · 5 ms | 🟢 |
| 95 | `bodies == 1` | 1 | 🟢 |
| 96 | `steps == 5` | 5 | 🟢 |
| 97 | `body-volumes contains 1.400000` | Extrude1 1.400000 | 🟢 |
| 100 | `bodies == 2` | 2 | 🟢 |
| 103 | `bodies == 1` | 1 | 🟢 |
| 116 | `focused-caption == Circle1` | Circle1 | 🟢 |
| 119 | `refused == 0` | 0 | 🟢 |
| 120 | `bodies == 2` | 2 | 🟢 |
| 121 | `body-volumes contains Cut2 0.282743` | Extrude1 1.117257 · Cut2 0.282743 | 🟢 |
| 123 | `genus == 1` | 1 | 🟢 |
| 131 | `bodies == 1` | 1 | 🟢 |
| 132 | `genus == 1` | 1 | 🟢 |
| 133 | `volume ~ 1.1172566612` | 1.117256661 | 🟢 |
| 143 | `curves == 11` | Section1,Between4,Between5,Section2,Section3,Section4,Section5,Section6,Between1,Betwee... | 🟢 |
| 159 | `selected-curves == Section1,Section2,Section3,Section4,Section5,Section6` | Section1,Section2,Section3,Section4,Section5,Section6 | 🟢 |
| 162 | `sheet == 1` | 1 | 🟢 |
| 163 | `bodies == 1` | 1 | 🟢 |
| 164 | `sound == 1` | 1 | 🟢 |
| 181 | `focused-caption == Curve1` | Curve1 | 🟢 |
| 184 | `refused == 0` | 0 | 🟢 |
| 185 | `bodies == 2` | 2 | 🟢 |
| 186 | `sound == 1` | 1 | 🟢 |
| 187 | `selected-bodies contains Cut#` | Cut#14 | 🟢 |
| 199 | `bodies == 1` | 1 | 🟢 |
| 200 | `sheet == 1` | 1 | 🟢 |
| 201 | `sound == 1` | 1 | 🟢 |
| 223 | `drawing == 0` |  | 🟢 |
| 224 | `sketch-curves == 5` | 5 | 🟢 |
| 227 | `trim == 1` | 1 | 🟢 |
| 230 | `trim-hover contains Line1` | Line1 · Line1 | 🟢 |
| 231 | `trim-refusal == 0` |  | 🟢 |
| 253 | `trim-commits == 5` | 5 | 🟢 |
| 254 | `sketch-curves == 10` | 10 | 🟢 |
| 255 | `refused == 0` | 0 | 🟢 |
| 261 | `trim == 0` | 0 | 🟢 |
| 262 | `focused-caption == Line1` | Line1 | 🟢 |
| 267 | `trim == 1` | 1 | 🟢 |
| 270 | `trim == 0` | 0 | 🟢 |
| 273 | `trim == 1` | 1 | 🟢 |
| 276 | `trim == 0` | 0 | 🟢 |
| 277 | `sketch-curves == 10` | 10 | 🟢 |
| 278 | `focused-caption == Line1` | Line1 | 🟢 |
| 292 | `volume ~ 1.237928331467` | 1.237928331 | 🟢 |
| 293 | `sound == 1` | 1 | 🟢 |
| 316 | `sketch-curves == 1` | 1 | 🟢 |
| 319 | `split == 1` | 1 | 🟢 |
| 322 | `trim-breaks == 1` | 1 | 🟢 |
| 326 | `refused == 0` | 0 | 🟢 |
| 327 | `sketch-curves == 2` | 2 | 🟢 |
| 328 | `status contains Cut Line` | Cut Line1 of Line1 · 0 ms | 🟢 |
| 331 | `trim-refusal contains on an end` | the pointer is on an end of Line1: Cut needs a point inside the curve | 🟢 |
| 334 | `split == 0` | 0 | 🟢 |
| 341 | `sketch-curves == 1` | 1 | 🟢 |
| 346 | `trim-breaks == 2` | 2 | 🟢 |
| 350 | `sketch-curves == 2` | 2 | 🟢 |
| 353 | `split == 0` | 0 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 294EdgeRing | 88 | `d03bdd1327f32f6a` |
| 102CutLine | 145 | `d7982e9f17a89115` |
| 103CutRemoved | 165 | `6bd9e88d9119868d` |
| 104CutCore | 211 | `ac0ded3e8509d9f2` |
| 105CutHole | 219 | `c572ddada97080c6` |
| 106PanelCut | 374 | `d561acedfb64c27c` |
| 107PanelTrimmed | 382 | `b9897d9e39217417` |
| 108TrimHover | 451 | `f69dcb91943e4a82` |
| 109TrimStar | 508 | `0a75a5954e733ea6` |
| 110StarExtruded | 587 | `5fb09a98a0a9c4fc` |
| 292CutLineHover | 634 | `d973c02237e31ae4` |
| 293CutCircleHover | 683 | `2e3f7c9d321c7209` |

## Journal

- New document (previous recovery copy kept)
- Rectangle on plane XY (or the body face under the first click): click two corners · Alt centre mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Rectangle1 · 0 ms
- Fill Rectangle1 · 0 ms
- Extrude Rectangle1 (E): move horizontally · Q union · W difference · E intersect · R new body · Enter / right-click applies · Esc aborts
- Bevel size 1 m (pinned)
- Extrude1 · Rectangle1 · New · 0 ms
- Selection cleared
- Cut (C): focus a sketch step or select a curve (the cutter), pick the body in mode 4 (else the active body)
- Edge ring: 4 edge(s) (Alt+Shift+click: the tangent chain)
- Selection cleared
- Autosaved _ClaudeScratch/SolidArcWorkspace/TrimSplitRecovery.solidarc
- Line on plane XY (or the body face under the first click): click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Line1 · 0 ms
- Cut (Extrude1 by Line1) · 5 ms
- View Isometric
- Framed the document (Space)
- Remove (Cut1) · 0 ms
- Undo
- Redo
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Circle1 · 0 ms
- Cut (Extrude1 by Circle1) · 12 ms
- View Isometric
- Remove (Cut2) · 0 ms
- New document (previous recovery copy kept)
- Import 11 curve(s) · 2 ms
- Framed the document (Space)
- Imported 11 curve(s) from Experimental/VehicleSurfacing/Samples/SyntheticBonnet.toml · 44 poles · worst fit 0.056 mm
- View Isometric
- Framed the document (Space)
- Selected curve Section1 · 4 poles · fit 0.045 mm · 1 selected
- Selected curve Section2 · 4 poles · fit 0.049 mm · 2 selected
- Autosaved _ClaudeScratch/SolidArcWorkspace/TrimSplitRecovery.solidarc
- Selected curve Section3 · 4 poles · fit 0.042 mm · 3 selected
- Selected curve Section4 · 4 poles · fit 0.049 mm · 4 selected
- Selected curve Section5 · 4 poles · fit 0.045 mm · 5 selected
- Selected curve Section6 · 4 poles · fit 0.046 mm · 6 selected
- Loft1 · 6 sections · 1 ms
- Lofted 6 curve(s) into a panel (sound) · 1 panel(s) · open sections stay a sheet · 1 ms
- Curve on plane XY (or the body face under the first click): click points · 1 Hermite · 2 Bezier · 3 NURBS · 4 cubic spline · Enter / right-click finishes · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Autosaved _ClaudeScratch/SolidArcWorkspace/TrimSplitRecovery.solidarc
- Curve1 · Hermite · 1 ms
- Cut (Loft1 by Curve1) · 773 ms
- View Isometric
- Framed the document (Space)
- Remove (Cut1) · 29 ms
- New document (previous recovery copy kept)
- Line on plane XY (or the body face under the first click): click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Autosaved _ClaudeScratch/SolidArcWorkspace/TrimSplitRecovery.solidarc
- Line1 · 0 ms
- Trim: click the piece of a curve to remove (between its crossings) · Esc ends
- Trim Line1 of Line1 · 0 ms
- Trim Line2 of Line1 · 0 ms
- Trim Line3 of Line1 · 0 ms
- Trim Line4 of Line1 · 0 ms
- Trim Line5 of Line1 · 0 ms
- Trim ended
- Trim: click the piece of a curve to remove (between its crossings) · Esc ends
- Trim ended
- Trim: click the piece of a curve to remove (between its crossings) · Esc ends
- Trim ended
- Autosaved _ClaudeScratch/SolidArcWorkspace/TrimSplitRecovery.solidarc
- Fill Line1 · 0 ms
- Extrude Line1 (E): move horizontally · Q union · W difference · E intersect · R new body · Enter / right-click applies · Esc aborts
- Bevel size 1 m (pinned)
- Extrude1 · Line1 · New · 1 ms
- View Isometric
- Framed the document (Space)
- New document (previous recovery copy kept)
- Line on plane XY (or the body face under the first click): click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Autosaved _ClaudeScratch/SolidArcWorkspace/TrimSplitRecovery.solidarc
- Line1 · 0 ms
- Cut: click a line, arc or circle where it should break in two (a circle becomes two half arcs) · Esc ends
- Cut Line1 of Line1 · 0 ms
- Cut ended
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Circle1 · 0 ms
- Cut: click a line, arc or circle where it should break in two (a circle becomes two half arcs) · Esc ends
- Cut Circle1 of Circle1 · 0 ms
- Cut ended
