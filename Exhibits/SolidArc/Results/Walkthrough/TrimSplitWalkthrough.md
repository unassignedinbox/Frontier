# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 900 · 563 frames · 54.2004 s · slowest frame 1289.32 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/TrimSplitWalkthrough.txt
- Expectations: 60 / 60 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 13 | `steps == 0` | 0 | 🟢 |
| 33 | `steps == 2` | 2 | 🟢 |
| 34 | `volume ~ 2` | 2 | 🟢 |
| 35 | `bodies == 1` | 1 | 🟢 |
| 42 | `status contains focus a sketch step or select a curve` | Cut (C): focus a sketch step or select a curve (the cutter), pick the body in mode 4 (e... | 🟢 |
| 43 | `steps == 2` | 2 | 🟢 |
| 54 | `focused-caption == Line1` | Line1 | 🟢 |
| 57 | `refused == 0` | 0 | 🟢 |
| 58 | `steps == 4` | 4 | 🟢 |
| 59 | `bodies == 2` | 2 | 🟢 |
| 60 | `body-volumes contains Cut1 0.600000` | Extrude1 1.400000 · Cut1 0.600000 | 🟢 |
| 61 | `body-volumes contains 1.400000` | Extrude1 1.400000 · Cut1 0.600000 | 🟢 |
| 62 | `selected-bodies contains Cut#4` | Cut#4 | 🟢 |
| 63 | `pick-mode == 4` | 4 | 🟢 |
| 64 | `sound == 1` | 1 | 🟢 |
| 65 | `status contains Cut` | Cut (Extrude1 by Line1) · 272 ms | 🟢 |
| 77 | `bodies == 1` | 1 | 🟢 |
| 78 | `steps == 5` | 5 | 🟢 |
| 79 | `body-volumes contains 1.400000` | Extrude1 1.400000 | 🟢 |
| 82 | `bodies == 2` | 2 | 🟢 |
| 85 | `bodies == 1` | 1 | 🟢 |
| 98 | `focused-caption == Circle1` | Circle1 | 🟢 |
| 101 | `refused == 0` | 0 | 🟢 |
| 102 | `bodies == 2` | 2 | 🟢 |
| 103 | `body-volumes contains Cut2 0.282743` | Extrude1 1.117257 · Cut2 0.282743 | 🟢 |
| 105 | `genus == 1` | 1 | 🟢 |
| 113 | `bodies == 1` | 1 | 🟢 |
| 114 | `genus == 1` | 1 | 🟢 |
| 115 | `volume ~ 1.1172566612` | 1.117256661 | 🟢 |
| 125 | `curves == 11` | Section1,Between4,Between5,Section2,Section3,Section4,Section5,Section6,Between1,Betwee... | 🟢 |
| 141 | `selected-curves == Section1,Section2,Section3,Section4,Section5,Section6` | Section1,Section2,Section3,Section4,Section5,Section6 | 🟢 |
| 144 | `sheet == 1` | 1 | 🟢 |
| 145 | `bodies == 1` | 1 | 🟢 |
| 146 | `sound == 1` | 1 | 🟢 |
| 163 | `focused-caption == Curve1` | Curve1 | 🟢 |
| 166 | `refused == 0` | 0 | 🟢 |
| 167 | `bodies == 2` | 2 | 🟢 |
| 168 | `sound == 1` | 1 | 🟢 |
| 169 | `selected-bodies contains Cut#` | Cut#14 | 🟢 |
| 181 | `bodies == 1` | 1 | 🟢 |
| 182 | `sheet == 1` | 1 | 🟢 |
| 183 | `sound == 1` | 1 | 🟢 |
| 205 | `drawing == 0` |  | 🟢 |
| 206 | `sketch-curves == 5` | 5 | 🟢 |
| 209 | `trim == 1` | 1 | 🟢 |
| 212 | `trim-hover contains Line1` | Line1 · Line1 | 🟢 |
| 213 | `trim-refusal == 0` |  | 🟢 |
| 235 | `trim-commits == 5` | 5 | 🟢 |
| 236 | `sketch-curves == 10` | 10 | 🟢 |
| 237 | `refused == 0` | 0 | 🟢 |
| 243 | `trim == 0` | 0 | 🟢 |
| 244 | `focused-caption == Line1` | Line1 | 🟢 |
| 249 | `trim == 1` | 1 | 🟢 |
| 252 | `trim == 0` | 0 | 🟢 |
| 255 | `trim == 1` | 1 | 🟢 |
| 258 | `trim == 0` | 0 | 🟢 |
| 259 | `sketch-curves == 10` | 10 | 🟢 |
| 260 | `focused-caption == Line1` | Line1 | 🟢 |
| 274 | `volume ~ 1.237928331467` | 1.237928331 | 🟢 |
| 275 | `sound == 1` | 1 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 102CutLine | 121 | `67b1f5c84b0a88a3` |
| 103CutRemoved | 141 | `afe2fbf326db027e` |
| 104CutCore | 187 | `eb0e91850fe6f9fd` |
| 105CutHole | 195 | `00f37bdecc27eeef` |
| 106PanelCut | 350 | `962381388f2cf670` |
| 107PanelTrimmed | 358 | `435e3583bfee8f04` |
| 108TrimHover | 427 | `54a064ba0718dafe` |
| 109TrimStar | 484 | `e3cc7c0707552cae` |
| 110StarExtruded | 563 | `868a7015e5602ff8` |

## Journal

- New document (previous recovery copy kept)
- Rectangle on plane XY (or the body face under the first click): click two corners · Alt centre mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Rectangle1 · 0 ms
- Fill Rectangle1 · 0 ms
- Extrude Rectangle1 (E): move horizontally · Q union · W difference · E intersect · R new body · Enter / right-click applies · Esc aborts
- Bevel size 1 m (pinned)
- Extrude1 · Rectangle1 · New · 7 ms
- Selection cleared
- Cut (C): focus a sketch step or select a curve (the cutter), pick the body in mode 4 (else the active body)
- Line on plane XY (or the body face under the first click): click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Line1 · 5 ms
- Cut (Extrude1 by Line1) · 272 ms
- View Isometric
- Framed the document (Space)
- Remove (Cut1) · 6 ms
- Undo
- Redo
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Circle1 · 6 ms
- Cut (Extrude1 by Circle1) · 78 ms
- View Isometric
- Remove (Cut2) · 14 ms
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
- Cut (Loft1 by Curve1) · 1133 ms
- View Isometric
- Framed the document (Space)
- Remove (Cut1) · 28 ms
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
- Extrude1 · Line1 · New · 14 ms
- View Isometric
- Framed the document (Space)
