# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0
- Frame: 1600 x 900 · 531 frames · 45.2438 s · slowest frame 1273.51 ms
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
| 63 | `status contains Cut` | Cut (Extrude1 by Line1) · 275 ms | 🟢 |
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
| 102CutLine | 116 | `b2adb859c80f077e` |
| 103CutRemoved | 136 | `0fd5d99aec4ad29a` |
| 104CutCore | 182 | `a22fee1b230c3f53` |
| 105CutHole | 190 | `57419ddb13dfb585` |
| 106PanelCut | 345 | `4a4ffc795c9a158d` |
| 107PanelTrimmed | 353 | `bec8dd77c164d2a9` |
| 108TrimHover | 422 | `5ff1bf9c93600941` |
| 109TrimStar | 479 | `0f5132aa45d85fbf` |
| 110StarExtruded | 531 | `cd5c1f4550a611c9` |

## Journal

- New document (previous recovery copy kept)
- Rectangle on plane XY (or the body face under the first click): click two corners · Alt centre mode · hold Ctrl to snap to ends / midpoints / centres · Esc aborts
- Rectangle1 · 0 ms
- Extrude Rectangle1 (E): move horizontally · Q union · W difference · E intersect · R new body · Enter / right-click applies · Esc aborts
- Bevel size 1 m (pinned)
- Extrude1 · Rectangle1 · New · 7 ms
- Selection cleared
- Cut (C): focus a sketch step or select a curve (the cutter), pick the body in mode 4 (else the active body)
- Line on plane XY (or the body face under the first click): click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres · Esc aborts
- Line1 · 5 ms
- Cut (Extrude1 by Line1) · 275 ms
- View Isometric
- Framed the document (Space)
- Remove (Cut1) · 6 ms
- Undo
- Redo
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres · Esc aborts
- Circle1 · 6 ms
- Cut (Extrude1 by Circle1) · 78 ms
- View Isometric
- Remove (Cut2) · 15 ms
- New document (previous recovery copy kept)
- Import 11 curve(s) · 3 ms
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
- Curve on plane XY (or the body face under the first click): click points · 1 Hermite · 2 Bezier · 3 NURBS · 4 cubic spline · Enter / right-click finishes · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres · Esc aborts
- Autosaved _ClaudeScratch/SolidArcWorkspace/TrimSplitRecovery.solidarc
- Curve1 · Hermite · 1 ms
- Cut (Loft1 by Curve1) · 1107 ms
- View Isometric
- Framed the document (Space)
- Remove (Cut1) · 28 ms
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
- Extrude1 · Line1 · New · 14 ms
- View Isometric
- Framed the document (Space)
