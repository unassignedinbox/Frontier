# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 900 · 408 frames · 52.1086 s · slowest frame 525.085 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/SnappingWalkthrough.txt
- Expectations: 78 / 78 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 23 | `steps == 9` | 9 | 🟢 |
| 30 | `snap contains Endpoint` | Endpoint · Edge Edge(Extrude#2.Cap(Start)/Extrude#2.Side(Line1)) | 🟢 |
| 31 | `snap-x == 0` | 0 | 🟢 |
| 32 | `snap-y == 0` | 0 | 🟢 |
| 33 | `snap-z == 0` | 0 | 🟢 |
| 37 | `drawing-points == 1` | 1 | 🟢 |
| 38 | `drawing-last-x == 0` | 0 | 🟢 |
| 39 | `drawing-last-y == 0` | 0 | 🟢 |
| 44 | `snap contains Tangent` | Tangent · Circle1 | 🟢 |
| 45 | `snap-x ~ -1.569338679` | -1.569338679 | 🟢 |
| 46 | `snap-y ~ 1.617768868` | 1.617768868 | 🟢 |
| 50 | `drawing-points == 2` | 2 | 🟢 |
| 51 | `drawing-last-x ~ -1.569338679` | -1.569338679 | 🟢 |
| 56 | `snap-lock contains tangent` | tangent | 🟢 |
| 59 | `snap contains tangent` | Axis · Lock tangent | 🟢 |
| 60 | `drawing-pointer-x ~ -2.265620126` | -2.265620126 | 🟢 |
| 61 | `drawing-pointer-y ~ 2.335537736` | 2.335537736 | 🟢 |
| 65 | `drawing-points == 3` | 3 | 🟢 |
| 66 | `snap-lock contains none` | none | 🟢 |
| 71 | `snap-lock contains normal` | normal | 🟢 |
| 74 | `drawing-pointer-x ~ -2.624504560` | -2.62450456 | 🟢 |
| 75 | `drawing-pointer-y ~ 1.987397012` | 1.987397012 | 🟢 |
| 79 | `drawing-points == 4` | 4 | 🟢 |
| 84 | `snap-lock contains none` | none | 🟢 |
| 90 | `snap contains Centre` | Centre · Circle1 | 🟢 |
| 91 | `snap-x == -2` | -2 | 🟢 |
| 92 | `snap-y ~ 1.2` | 1.2 | 🟢 |
| 96 | `snap contains Midpoint` | Midpoint · Edge Edge(Extrude#2.Cap(Start)/Extrude#2.Side(Line1)) | 🟢 |
| 97 | `snap-x == 2` | 2 | 🟢 |
| 98 | `snap-y == 0` | 0 | 🟢 |
| 102 | `drawing-points == 5` | 5 | 🟢 |
| 107 | `snap contains Perpendicular` | Perpendicular · Circle1 | 🟢 |
| 108 | `snap-x ~ -1.425304229` | -1.425304229 | 🟢 |
| 109 | `snap-y ~ 1.027591269` | 1.027591269 | 🟢 |
| 113 | `drawing-points == 6` | 6 | 🟢 |
| 114 | `snapped-points == 4` | 4 | 🟢 |
| 117 | `steps == 10` | 10 | 🟢 |
| 126 | `drawing-square == 1` | 1 | 🟢 |
| 129 | `drawing-pointer-x ~ -3.8` | -3.8 | 🟢 |
| 130 | `drawing-pointer-y ~ 2.8` | 2.8 | 🟢 |
| 134 | `steps == 11` | 11 | 🟢 |
| 140 | `constrain == 1` | 1 | 🟢 |
| 149 | `snap contains Centre` | Centre · Circle1 | 🟢 |
| 150 | `snap-x == -2` | -2 | 🟢 |
| 154 | `sketch-points contains (-2, 1.2)` | (-3.8, 1.2) (-2, 1.2) (-2, 2.8) (-3.8, 2.8) | 🟢 |
| 155 | `snapped-points == 5` | 5 | 🟢 |
| 170 | `snap contains Endpoint · Line1 · Rectangle1` | Endpoint · Line1 · Rectangle1 | 🟢 |
| 171 | `transform-shift-x == -2` | -2 | 🟢 |
| 172 | `transform-shift-y ~ 1.2` | 1.2 | 🟢 |
| 173 | `transform-shift-z == 0` | 0 | 🟢 |
| 174 | `transform-readout contains Endpoint` | Move screen (-2, 1.2, 0) · Endpoint Edge Edge(Extrude#2.Cap(Start)/Extrude#2.Side(Line... | 🟢 |
| 178 | `transform-commits == 1` | 1 | 🟢 |
| 179 | `volume ~ 10.22278` | 10.22277375 | 🟢 |
| 187 | `snap lacks Endpoint` | Quarter · Edge Edge(Extrude#4.Side(Circle1)/Extrude#4.Side(Circle1)) | 🟢 |
| 192 | `snap contains Endpoint` | Endpoint · Edge Edge(Extrude#2.Cap(Start)/Extrude#2.Side(Line3)) | 🟢 |
| 193 | `snap-x == 0` | 0 | 🟢 |
| 194 | `snap-y == 2.5` | 2.5 | 🟢 |
| 209 | `steps == 11` | 11 | 🟢 |
| 226 | `snap contains Endpoint · Curve Section2` | Endpoint · Curve Section2 | 🟢 |
| 227 | `pole-snap-gap == 0` | 0 | 🟢 |
| 231 | `pole-edits == 1` | 1 | 🟢 |
| 232 | `snapped-points == 6` | 6 | 🟢 |
| 253 | `snap-grid == 0` | 0 | 🟢 |
| 258 | `snap == none` | none | 🟢 |
| 259 | `drawing-pointer-x > 0.52` | 0.5331536522 | 🟢 |
| 260 | `drawing-pointer-x < 0.55` | 0.5331536522 | 🟢 |
| 261 | `drawing-pointer-y > 0.70` | 0.7149105791 | 🟢 |
| 262 | `drawing-pointer-y < 0.725` | 0.7149105791 | 🟢 |
| 268 | `snap contains Midpoint` | Midpoint · Line1 | 🟢 |
| 269 | `snap-x ~ 1` | 1 | 🟢 |
| 270 | `snap-y ~ 0` | 0 | 🟢 |
| 273 | `snap contains Quarter` | Quarter · Line1 | 🟢 |
| 274 | `snap-x ~ 0.5` | 0.5 | 🟢 |
| 278 | `snap lacks curve` | none | 🟢 |
| 281 | `snap contains On curve` | On curve · Line1 | 🟢 |
| 282 | `snap-y ~ 0` | 0 | 🟢 |
| 283 | `snap-x > 1.7` | 1.752940562 | 🟢 |
| 284 | `snap-x < 1.82` | 1.752940562 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 160SnapEndpoint | 30 | `59190876cde1b3a9` |
| 161SnapTangent | 43 | `0ac9d854f8a3d7fe` |
| 162SnapLockTangent | 61 | `cb445d7267488b2d` |
| 163SnapLockNormal | 79 | `a097d56e3d2689ba` |
| 164SnapBinormalRefused | 93 | `e34174542429f6b9` |
| 165SnapCentre | 99 | `a6392ced61db93de` |
| 166SnapMidpoint | 105 | `53a15d55b98519d8` |
| 167SnapPerpendicular | 118 | `bcf33ac103cfc7f1` |
| 168SnapSquare | 151 | `45a79d2f1fff0e6e` |
| 169SnapConstraintDrag | 181 | `c4b64239d179f6f5` |
| 170SnapMove | 217 | `62987d7b72868f82` |
| 171SnapXRay | 250 | `584f4c4a5f829bea` |
| 172SnapPoleDrag | 322 | `f876da4d2118f6e9` |
| 295SnapQuarter | 392 | `1cc1b4197ca20d5f` |
| 296SnapOnCurve | 402 | `d8c3e5459397d85f` |

## Journal

- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Circle1 · 1 ms
- Line on plane XY (or the body face under the first click): click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Lock tangent through the last point · the same key releases it · object snaps align along it
- Autosaved _ClaudeScratch/SolidArcWorkspace/SnapRecovery.solidarc
- Lock normal through the last point · the same key releases it · object snaps align along it
- Lock binormal leaves the XY drawing plane: refused
- Line1 · 1 ms
- Rectangle on plane XY (or the body face under the first click): click two corners · Alt centre mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Rectangle: square (s releases)
- Rectangle1 · 1 ms
- Constrain Rectangle1 · 4 constraint(s) · DOF 4 · click points / curves (earlier sketches link), then D N H V O Q M Y L [ ] C · Shift+D reference · drag points · Esc ends
- Drag Line1.End of Rectangle1 · 1 ms
- Constrain ended
- Selected Block1 (11 faces)
- Move 1 body(ies) · X / Y / Z axis · Shift+X / Y / Z plane · type a value · click / Enter commits · Esc cancels
- Autosaved _ClaudeScratch/SolidArcWorkspace/SnapRecovery.solidarc
- Move (Block1) · 1 ms
- Undo
- Line on plane XY (or the body face under the first click): click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- X-ray on · faces 45 % · hidden edges pickable
- Line aborted (document untouched)
- X-ray off
- New document (previous recovery copy kept)
- Autosaved _ClaudeScratch/SolidArcWorkspace/SnapRecovery.solidarc
- Import 11 curve(s) · 2 ms
- Framed the document (Space)
- Imported 11 curve(s) from Experimental/VehicleSurfacing/Samples/SyntheticBonnet.toml · 44 poles · worst fit 0.056 mm
- View Isometric
- Framed the document (Space)
- Move pole 2 of Section3 · 2 ms
- Moved pole 2 of Section3 by 350.00 mm · 2 ms
- Undo
- New document (previous recovery copy kept)
- Line on plane XY (or the body face under the first click): click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Line1 · 0 ms
- Selection cleared
- Line on plane XY (or the body face under the first click): click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Autosaved _ClaudeScratch/SolidArcWorkspace/SnapRecovery.solidarc
- Line aborted (document untouched)
