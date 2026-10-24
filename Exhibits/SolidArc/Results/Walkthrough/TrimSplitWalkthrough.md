# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0
- Frame: 1600 x 900 · 531 frames · 33.3389 s · slowest frame 1033.61 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/TrimSplitWalkthrough.txt
- Expectations: 54 / 54 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 13 | `steps == 0` | 0 | 🟢 |
| 31 | `steps == 2` | 2 | 🟢 |
| 32 | `volume ~ 2` | 2 | 🟢 |
| 33 | `bodies == 1` | 1 | 🟢 |
| 40 | `status contains focus a sketch step or select a curve` | Cut (C): focus a sketch step or select a curve (the cutter), pick the body in mode 4 (e... | 🟢 |
| 41 | `steps == 2` | 2 | 🟢 |
| 52 | `focused-caption == Line1` | Line1 | 🟢 |
| 55 | `refused == 0` | 0 | 🟢 |
| 56 | `steps == 4` | 4 | 🟢 |
| 57 | `bodies == 2` | 2 | 🟢 |
| 58 | `body-volumes contains Cut1 0.600000` | Extrude1 1.400000 · Cut1 0.600000 | 🟢 |
| 59 | `body-volumes contains 1.400000` | Extrude1 1.400000 · Cut1 0.600000 | 🟢 |
| 60 | `selected-bodies contains Cut#4` | Cut#4 | 🟢 |
| 61 | `pick-mode == 4` | 4 | 🟢 |
| 62 | `sound == 1` | 1 | 🟢 |
| 63 | `status contains Cut` | Cut (Extrude1 by Line1) · 190 ms | 🟢 |
| 75 | `bodies == 1` | 1 | 🟢 |
| 76 | `steps == 5` | 5 | 🟢 |
| 77 | `body-volumes contains 1.400000` | Extrude1 1.400000 | 🟢 |
| 80 | `bodies == 2` | 2 | 🟢 |
| 83 | `bodies == 1` | 1 | 🟢 |
| 96 | `focused-caption == Circle1` | Circle1 | 🟢 |
| 99 | `refused == 0` | 0 | 🟢 |
| 100 | `bodies == 2` | 2 | 🟢 |
| 101 | `body-volumes contains Cut2 0.282743` | Extrude1 1.117257 · Cut2 0.282743 | 🟢 |
| 103 | `genus == 1` | 1 | 🟢 |
| 111 | `bodies == 1` | 1 | 🟢 |
| 112 | `genus == 1` | 1 | 🟢 |
| 113 | `volume ~ 1.1172566612` | 1.117256661 | 🟢 |
| 123 | `curves == 11` | Section1,Between4,Between5,Section2,Section3,Section4,Section5,Section6,Between1,Betwee... | 🟢 |
| 139 | `selected-curves == Section1,Section2,Section3,Section4,Section5,Section6` | Section1,Section2,Section3,Section4,Section5,Section6 | 🟢 |
| 142 | `sheet == 1` | 1 | 🟢 |
| 143 | `bodies == 1` | 1 | 🟢 |
| 144 | `sound == 1` | 1 | 🟢 |
| 161 | `focused-caption == Curve1` | Curve1 | 🟢 |
| 164 | `refused == 0` | 0 | 🟢 |
| 165 | `bodies == 2` | 2 | 🟢 |
| 166 | `sound == 1` | 1 | 🟢 |
| 167 | `selected-bodies contains Cut#` | Cut#14 | 🟢 |
| 179 | `bodies == 1` | 1 | 🟢 |
| 180 | `sheet == 1` | 1 | 🟢 |
| 181 | `sound == 1` | 1 | 🟢 |
| 203 | `drawing == 0` |  | 🟢 |
| 204 | `sketch-curves == 5` | 5 | 🟢 |
| 207 | `trim == 1` | 1 | 🟢 |
| 210 | `trim-hover contains Line1` | Line1 · Line1 | 🟢 |
| 211 | `trim-refusal == 0` |  | 🟢 |
| 233 | `trim-commits == 5` | 5 | 🟢 |
| 234 | `sketch-curves == 10` | 10 | 🟢 |
| 235 | `refused == 0` | 0 | 🟢 |
| 241 | `trim == 0` | 0 | 🟢 |
| 242 | `focused-caption == Line1` | Line1 | 🟢 |
| 254 | `volume ~ 1.237928331467` | 1.237928331 | 🟢 |
| 255 | `sound == 1` | 1 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 102CutLine | 116 | `b4f8709c37ab9ad8` |
| 103CutRemoved | 136 | `c367e69b120c2c29` |
| 104CutCore | 182 | `021615879b898d77` |
| 105CutHole | 190 | `5e9b1d59914f3da2` |
| 106PanelCut | 345 | `f5e791d932a90afe` |
| 107PanelTrimmed | 353 | `bee671bf6044a522` |
| 108TrimHover | 422 | `061fd4f297cbd56d` |
| 109TrimStar | 479 | `e2b6b129eca17c07` |
| 110StarExtruded | 531 | `c290bd72d857ee21` |

## Journal

- New document (previous recovery copy kept)
- Rectangle on plane XY (or the body face under the first click): click two corners · Alt centre mode · hold Ctrl to snap to ends / midpoints / centres · Esc aborts
- Rectangle1 · 0 ms
- Extrude Rectangle1 (E): move horizontally · Q union · W difference · E intersect · R new body · Enter / right-click applies · Esc aborts
- Bevel size 1 m (pinned)
- Extrude1 · Rectangle1 · New · 5 ms
- Selection cleared
- Cut (C): focus a sketch step or select a curve (the cutter), pick the body in mode 4 (else the active body)
- Line on plane XY (or the body face under the first click): click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres · Esc aborts
- Line1 · 4 ms
- Cut (Extrude1 by Line1) · 190 ms
- View Isometric
- Framed the document (Space)
- Remove (Cut1) · 4 ms
- Undo
- Redo
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres · Esc aborts
- Circle1 · 5 ms
- Cut (Extrude1 by Circle1) · 60 ms
- View Isometric
- Remove (Cut2) · 12 ms
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
- Lofted 6 curve(s) → panel (sound) · 1 panel(s) · open sections stay a sheet · 1 ms
- Curve on plane XY (or the body face under the first click): click points · 1 Hermite · 2 Bezier · 3 NURBS · 4 cubic spline · Enter / right-click finishes · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres · Esc aborts
- Autosaved _ClaudeScratch/SolidArcWorkspace/TrimSplitRecovery.solidarc
- Curve1 · Hermite · 1 ms
- Cut (Loft1 by Curve1) · 930 ms
- View Isometric
- Framed the document (Space)
- Remove (Cut1) · 19 ms
- New document (previous recovery copy kept)
- Line on plane XY (or the body face under the first click): click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres · Esc aborts
- Autosaved _ClaudeScratch/SolidArcWorkspace/TrimSplitRecovery.solidarc
- Line1 · 0 ms
- Trim: click the piece of a curve to remove (between its crossings) · Esc ends
- Trim Line1 of Line1 · 0 ms
- Trim Line2 of Line1 · 0 ms
- Trim Line3 of Line1 · 0 ms
- Trim Line4 of Line1 · 0 ms
- Trim Line5 of Line1 · 0 ms
- Trim ended
- Extrude Line1 (E): move horizontally · Q union · W difference · E intersect · R new body · Enter / right-click applies · Esc aborts
- Bevel size 1 m (pinned)
- Extrude1 · Line1 · New · 12 ms
- View Isometric
- Framed the document (Space)
