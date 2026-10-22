# DraftingWorkspaceCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0
- Frame: 1600 x 900 · 496 frames · 30.2169 s · slowest frame 1284.68 ms
- Script: /home/user/Frontier/Experimental/DraftingWorkspace/Scripts/TrimSplitWalkthrough.txt
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
| 63 | `status contains Cut` | Cut (Extrude1 by Line1) · 206 ms | 🟢 |
| 73 | `bodies == 1` | 1 | 🟢 |
| 74 | `steps == 5` | 5 | 🟢 |
| 75 | `body-volumes contains 1.400000` | Extrude1 1.400000 | 🟢 |
| 78 | `bodies == 2` | 2 | 🟢 |
| 81 | `bodies == 1` | 1 | 🟢 |
| 93 | `focused-caption == Circle1` | Circle1 | 🟢 |
| 96 | `refused == 0` | 0 | 🟢 |
| 97 | `bodies == 2` | 2 | 🟢 |
| 98 | `body-volumes contains Cut2 0.282743` | Extrude1 1.117257 · Cut2 0.282743 | 🟢 |
| 100 | `genus == 1` | 1 | 🟢 |
| 106 | `bodies == 1` | 1 | 🟢 |
| 107 | `genus == 1` | 1 | 🟢 |
| 108 | `volume ~ 1.1172566612` | 1.117256661 | 🟢 |
| 118 | `curves == 11` | Section1,Between4,Between5,Section2,Section3,Section4,Section5,Section6,Between1,Betwee... | 🟢 |
| 132 | `selected-curves == Section1,Section2,Section3,Section4,Section5,Section6` | Section1,Section2,Section3,Section4,Section5,Section6 | 🟢 |
| 135 | `sheet == 1` | 1 | 🟢 |
| 136 | `bodies == 1` | 1 | 🟢 |
| 137 | `sound == 1` | 1 | 🟢 |
| 154 | `focused-caption == Curve1` | Curve1 | 🟢 |
| 157 | `refused == 0` | 0 | 🟢 |
| 158 | `bodies == 2` | 2 | 🟢 |
| 159 | `sound == 1` | 1 | 🟢 |
| 160 | `selected-bodies contains Cut#` | Cut#14 | 🟢 |
| 170 | `bodies == 1` | 1 | 🟢 |
| 171 | `sheet == 1` | 1 | 🟢 |
| 172 | `sound == 1` | 1 | 🟢 |
| 194 | `drawing == 0` |  | 🟢 |
| 195 | `sketch-curves == 5` | 5 | 🟢 |
| 198 | `trim == 1` | 1 | 🟢 |
| 201 | `trim-hover contains Line1` | Line1 · Line1 | 🟢 |
| 202 | `trim-refusal == 0` |  | 🟢 |
| 224 | `trim-commits == 5` | 5 | 🟢 |
| 225 | `sketch-curves == 10` | 10 | 🟢 |
| 226 | `refused == 0` | 0 | 🟢 |
| 232 | `trim == 0` | 0 | 🟢 |
| 233 | `focused-caption == Line1` | Line1 | 🟢 |
| 245 | `volume ~ 1.237928331467` | 1.237928331 | 🟢 |
| 246 | `sound == 1` | 1 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 102CutLine | 109 | `f3360f77edf586c6` |
| 103CutRemoved | 129 | `9157fbba2504849f` |
| 104CutCore | 168 | `d0500da511b9b115` |
| 105CutHole | 176 | `6c4ea142002ebb86` |
| 106PanelCut | 317 | `037e8e1457ce629d` |
| 107PanelTrimmed | 325 | `1a401ada66389675` |
| 108TrimHover | 394 | `67bac72212082878` |
| 109TrimStar | 451 | `5bda2d8bef560d3e` |
| 110StarExtruded | 496 | `a7cef983c125db64` |

## Journal

- New document (previous recovery copy kept)
- Rectangle on plane XY: click two corners · Alt centre mode · Esc aborts
- Rectangle1 · 0 ms
- Extrude Rectangle1 (E): move horizontally · Q union · W difference · E intersect · R new body · Enter / right-click applies · Esc aborts
- Bevel size 1 m (pinned)
- Extrude1 · Rectangle1 · New · 5 ms
- Selection cleared
- Cut (C): focus a sketch step or select a curve (the cutter), pick the body in mode 4 (else the active body)
- Line on plane XY: click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · Esc aborts
- Line1 · 6 ms
- Cut (Extrude1 by Line1) · 206 ms
- View Isometric
- Framed the document (Space)
- Remove (Cut1) · 202 ms
- Undo
- Redo
- Circle on plane XY: click centre and rim · V two-point mode · Esc aborts
- Circle1 · 203 ms
- Cut (Extrude1 by Circle1) · 263 ms
- View Isometric
- Remove (Cut2) · 251 ms
- New document (previous recovery copy kept)
- Import 11 curve(s) · 1 ms
- Framed the document (Space)
- Imported 11 curve(s) from Experimental/VehicleSurfacing/Samples/SyntheticBonnet.toml · 44 poles · worst fit 0.056 mm
- View Isometric
- Framed the document (Space)
- Selected curve Section1 · 4 poles · fit 0.045 mm · 1 selected
- Selected curve Section2 · 4 poles · fit 0.049 mm · 2 selected
- Selected curve Section3 · 4 poles · fit 0.042 mm · 3 selected
- Autosaved _ClaudeScratch/DraftingWorkspace/TrimSplitRecovery.drafting.toml
- Selected curve Section4 · 4 poles · fit 0.049 mm · 4 selected
- Selected curve Section5 · 4 poles · fit 0.045 mm · 5 selected
- Selected curve Section6 · 4 poles · fit 0.046 mm · 6 selected
- Loft1 · 6 sections · 2 ms
- Lofted 6 curve(s) → panel (sound) · 1 panel(s) · 2 ms
- Curve on plane XY: click points · 1 Hermite · 2 Bezier · 3 NURBS · 4 cubic spline · Enter / right-click finishes · Ctrl+Z drops a point · Esc aborts
- Autosaved _ClaudeScratch/DraftingWorkspace/TrimSplitRecovery.drafting.toml
- Curve1 · Hermite · 2 ms
- Cut (Loft1 by Curve1) · 1159 ms
- View Isometric
- Framed the document (Space)
- Remove (Cut1) · 948 ms
- New document (previous recovery copy kept)
- Line on plane XY: click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · Esc aborts
- Autosaved _ClaudeScratch/DraftingWorkspace/TrimSplitRecovery.drafting.toml
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
- Extrude1 · Line1 · New · 11 ms
- View Isometric
- Framed the document (Space)
