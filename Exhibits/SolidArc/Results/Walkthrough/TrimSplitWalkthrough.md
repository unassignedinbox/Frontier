# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 810 · 702 frames · 47.28 s · slowest frame 588.094 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/TrimSplitWalkthrough.txt
- Expectations: 77 / 77 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 16 | `steps == 0` | 0 | 🟢 |
| 36 | `steps == 2` | 2 | 🟢 |
| 37 | `volume ~ 2` | 2 | 🟢 |
| 38 | `bodies == 1` | 1 | 🟢 |
| 45 | `status contains focus a sketch step or select a curve` | Cut (C): focus a sketch step or select a curve (the cutter), pick the body in mode 4 (e... | 🟢 |
| 46 | `steps == 2` | 2 | 🟢 |
| 57 | `selected-edges == 4` | Edge(Extrude#2.Cap(End)/Extrude#2.Side(Line1)),Edge(Extrude#2.Cap(Start)/Extrude#2.Side... | 🟢 |
| 58 | `ring-picks == 1` | 1 | 🟢 |
| 62 | `selected-edges == 1` | Edge(Extrude#2.Cap(End)/Extrude#2.Side(Line1)) | 🟢 |
| 63 | `ring-picks == 2` | 2 | 🟢 |
| 66 | `selected-edges == 0` |  | 🟢 |
| 77 | `focused-caption == Line1` | Line1 | 🟢 |
| 80 | `refused == 0` | 0 | 🟢 |
| 81 | `steps == 4` | 4 | 🟢 |
| 82 | `bodies == 2` | 2 | 🟢 |
| 83 | `body-volumes contains Cut1 0.600000` | Extrude1 1.400000 · Cut1 0.600000 | 🟢 |
| 84 | `body-volumes contains 1.400000` | Extrude1 1.400000 · Cut1 0.600000 | 🟢 |
| 85 | `selected-bodies contains Cut#4` | Cut#4 | 🟢 |
| 86 | `pick-mode == 4` | 4 | 🟢 |
| 87 | `sound == 1` | 1 | 🟢 |
| 88 | `status contains Cut` | Cut (Extrude1 by Line1) · 2 ms | 🟢 |
| 100 | `bodies == 1` | 1 | 🟢 |
| 101 | `steps == 5` | 5 | 🟢 |
| 102 | `body-volumes contains 1.400000` | Extrude1 1.400000 | 🟢 |
| 105 | `bodies == 2` | 2 | 🟢 |
| 108 | `bodies == 1` | 1 | 🟢 |
| 121 | `focused-caption == Circle1` | Circle1 | 🟢 |
| 124 | `refused == 0` | 0 | 🟢 |
| 125 | `bodies == 2` | 2 | 🟢 |
| 126 | `body-volumes contains Cut2 0.282743` | Extrude1 1.117257 · Cut2 0.282743 | 🟢 |
| 128 | `genus == 1` | 1 | 🟢 |
| 136 | `bodies == 1` | 1 | 🟢 |
| 137 | `genus == 1` | 1 | 🟢 |
| 138 | `volume ~ 1.1172566612` | 1.117256661 | 🟢 |
| 148 | `curves == 11` | Section1,Between4,Between5,Section2,Section3,Section4,Section5,Section6,Between1,Betwee... | 🟢 |
| 164 | `selected-curves == Section1,Section2,Section3,Section4,Section5,Section6` | Section1,Section2,Section3,Section4,Section5,Section6 | 🟢 |
| 167 | `sheet == 1` | 1 | 🟢 |
| 168 | `bodies == 1` | 1 | 🟢 |
| 169 | `sound == 1` | 1 | 🟢 |
| 186 | `focused-caption == Curve1` | Curve1 | 🟢 |
| 189 | `refused == 0` | 0 | 🟢 |
| 190 | `bodies == 2` | 2 | 🟢 |
| 191 | `sound == 1` | 1 | 🟢 |
| 192 | `selected-bodies contains Cut#` | Cut#14 | 🟢 |
| 204 | `bodies == 1` | 1 | 🟢 |
| 205 | `sheet == 1` | 1 | 🟢 |
| 206 | `sound == 1` | 1 | 🟢 |
| 228 | `drawing == 0` |  | 🟢 |
| 229 | `sketch-curves == 5` | 5 | 🟢 |
| 232 | `trim == 1` | 1 | 🟢 |
| 235 | `trim-hover contains Line1` | Line1 · Line1 | 🟢 |
| 236 | `trim-refusal == 0` |  | 🟢 |
| 258 | `trim-commits == 5` | 5 | 🟢 |
| 259 | `sketch-curves == 10` | 10 | 🟢 |
| 260 | `refused == 0` | 0 | 🟢 |
| 266 | `trim == 0` | 0 | 🟢 |
| 267 | `focused-caption == Line1` | Line1 | 🟢 |
| 272 | `trim == 1` | 1 | 🟢 |
| 275 | `trim == 0` | 0 | 🟢 |
| 278 | `trim == 1` | 1 | 🟢 |
| 281 | `trim == 0` | 0 | 🟢 |
| 282 | `sketch-curves == 10` | 10 | 🟢 |
| 283 | `focused-caption == Line1` | Line1 | 🟢 |
| 297 | `volume ~ 1.237928331467` | 1.237928331 | 🟢 |
| 298 | `sound == 1` | 1 | 🟢 |
| 321 | `sketch-curves == 1` | 1 | 🟢 |
| 324 | `split == 1` | 1 | 🟢 |
| 327 | `trim-breaks == 1` | 1 | 🟢 |
| 331 | `refused == 0` | 0 | 🟢 |
| 332 | `sketch-curves == 2` | 2 | 🟢 |
| 333 | `status contains Cut Line` | Cut Line1 of Line1 · 0 ms | 🟢 |
| 336 | `trim-refusal contains on an end` | the pointer is on an end of Line1: Cut needs a point inside the curve | 🟢 |
| 339 | `split == 0` | 0 | 🟢 |
| 346 | `sketch-curves == 1` | 1 | 🟢 |
| 351 | `trim-breaks == 2` | 2 | 🟢 |
| 355 | `sketch-curves == 2` | 2 | 🟢 |
| 358 | `split == 0` | 0 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 294EdgeRing | 88 | `b63b6a39d1b1c262` |
| 102CutLine | 152 | `025a7de6d28c04e2` |
| 103CutRemoved | 172 | `aa27dd5fa73332d0` |
| 104CutCore | 218 | `985b8b80bb3bf488` |
| 105CutHole | 226 | `f69445948e9af9e3` |
| 106PanelCut | 379 | `82a31b8c4b7bc3d9` |
| 107PanelTrimmed | 387 | `53bc1d0a756b212a` |
| 108TrimHover | 456 | `c17cf94b10e35e18` |
| 109TrimStar | 513 | `da824400e3a9c75c` |
| 110StarExtruded | 592 | `fd076b7899a29fee` |
| 292CutLineHover | 639 | `e1c596b19f98419d` |
| 293CutCircleHover | 688 | `5a15b79c00c472aa` |

## Journal

- New document (previous recovery copy kept)
- Rectangle on plane XY (or the body face under the first click): click two corners · Alt centre mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Rectangle1 · 0 ms
- Fill Rectangle1 · 0 ms
- Extrude Rectangle1 (E): the profile follows the pointer along its normal · Q union · W difference · E intersect · R new body · A auto · Enter / right-click applies · Esc aborts
- Bevel size 1 m (pinned)
- Extrude1 · Rectangle1 · New · 0 ms
- Selection cleared
- Cut (C): focus a sketch step or select a curve (the cutter), pick the body in mode 4 (else the active body)
- Edge ring: 4 edge(s) selected · Alt loop · Ctrl+Alt ring · Shift toggles · double-click tangent chain
- Edge loop: 1 edge(s) selected · Alt loop · Ctrl+Alt ring · Shift toggles · double-click tangent chain
- Autosaved _ClaudeScratch/SolidArcWorkspace/TrimSplitRecovery.solidarc
- Selection cleared
- Line on plane XY (or the body face under the first click): click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Line1 · 0 ms
- Cut (Extrude1 by Line1) · 2 ms
- View Isometric
- Framed the document (Space)
- Remove (Cut1) · 0 ms
- Undo
- Redo
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Circle1 · 0 ms
- Cut (Extrude1 by Circle1) · 4 ms
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
- Cut (Loft1 by Curve1) · 471 ms
- View Isometric
- Framed the document (Space)
- Remove (Cut1) · 14 ms
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
- Extrude Line1 (E): the profile follows the pointer along its normal · Q union · W difference · E intersect · R new body · A auto · Enter / right-click applies · Esc aborts
- Bevel size 1 m (pinned)
- Extrude1 · Line1 · New · 1 ms
- View Isometric
- Framed the document (Space)
- New document (previous recovery copy kept)
- Line on plane XY (or the body face under the first click): click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Autosaved _ClaudeScratch/SolidArcWorkspace/TrimSplitRecovery.solidarc
- Line1 · 0 ms
- Cut: click a line, arc or circle where it should break in two (a circle becomes two half arcs) · hold Ctrl to snap onto a crossing / end / midpoint: every curve through it breaks there (overlapping circles: both, at both crossings) · Esc ends
- Cut Line1 of Line1 · 0 ms
- Cut ended
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Circle1 · 0 ms
- Cut: click a line, arc or circle where it should break in two (a circle becomes two half arcs) · hold Ctrl to snap onto a crossing / end / midpoint: every curve through it breaks there (overlapping circles: both, at both crossings) · Esc ends
- Cut Circle1 of Circle1 · 0 ms
- Cut ended
