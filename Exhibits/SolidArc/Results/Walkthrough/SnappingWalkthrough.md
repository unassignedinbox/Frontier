# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0
- Frame: 1600 x 900 · 325 frames · 46.4215 s · slowest frame 482.226 ms
- Script: /home/user/Frontier/Exhibits/SolidArc/Code/Scripts/SnappingWalkthrough.txt
- Expectations: 62 / 62 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 19 | `steps == 9` | 9 | 🟢 |
| 26 | `snap contains Endpoint` | Endpoint · Edge Edge(Extrude#2.Cap(Start)/Extrude#2.Side(Line1)) | 🟢 |
| 27 | `snap-x == 0` | 0 | 🟢 |
| 28 | `snap-y == 0` | 0 | 🟢 |
| 29 | `snap-z == 0` | 0 | 🟢 |
| 33 | `drawing-points == 1` | 1 | 🟢 |
| 34 | `drawing-last-x == 0` | 0 | 🟢 |
| 35 | `drawing-last-y == 0` | 0 | 🟢 |
| 40 | `snap contains Tangent` | Tangent · Circle1 | 🟢 |
| 41 | `snap-x ~ -1.569338679` | -1.569338679 | 🟢 |
| 42 | `snap-y ~ 1.617768868` | 1.617768868 | 🟢 |
| 46 | `drawing-points == 2` | 2 | 🟢 |
| 47 | `drawing-last-x ~ -1.569338679` | -1.569338679 | 🟢 |
| 52 | `snap-lock contains tangent` | tangent | 🟢 |
| 55 | `snap contains tangent` | Axis · Lock tangent | 🟢 |
| 56 | `drawing-pointer-x ~ -2.265620126` | -2.265620126 | 🟢 |
| 57 | `drawing-pointer-y ~ 2.335537736` | 2.335537736 | 🟢 |
| 61 | `drawing-points == 3` | 3 | 🟢 |
| 62 | `snap-lock contains none` | none | 🟢 |
| 67 | `snap-lock contains normal` | normal | 🟢 |
| 70 | `drawing-pointer-x ~ -2.624504560` | -2.62450456 | 🟢 |
| 71 | `drawing-pointer-y ~ 1.987397012` | 1.987397012 | 🟢 |
| 75 | `drawing-points == 4` | 4 | 🟢 |
| 80 | `snap-lock contains none` | none | 🟢 |
| 86 | `snap contains Centre` | Centre · Circle1 | 🟢 |
| 87 | `snap-x == -2` | -2 | 🟢 |
| 88 | `snap-y ~ 1.2` | 1.2 | 🟢 |
| 92 | `snap contains Midpoint` | Midpoint · Edge Edge(Extrude#2.Cap(Start)/Extrude#2.Side(Line1)) | 🟢 |
| 93 | `snap-x == 2` | 2 | 🟢 |
| 94 | `snap-y == 0` | 0 | 🟢 |
| 98 | `drawing-points == 5` | 5 | 🟢 |
| 103 | `snap contains Perpendicular` | Perpendicular · Circle1 | 🟢 |
| 104 | `snap-x ~ -1.425304229` | -1.425304229 | 🟢 |
| 105 | `snap-y ~ 1.027591269` | 1.027591269 | 🟢 |
| 109 | `drawing-points == 6` | 6 | 🟢 |
| 110 | `snapped-points == 4` | 4 | 🟢 |
| 113 | `steps == 10` | 10 | 🟢 |
| 122 | `drawing-square == 1` | 1 | 🟢 |
| 125 | `drawing-pointer-x ~ -3.8` | -3.8 | 🟢 |
| 126 | `drawing-pointer-y ~ 2.8` | 2.8 | 🟢 |
| 130 | `steps == 11` | 11 | 🟢 |
| 136 | `constrain == 1` | 1 | 🟢 |
| 145 | `snap contains Centre` | Centre · Circle1 | 🟢 |
| 146 | `snap-x == -2` | -2 | 🟢 |
| 150 | `sketch-points contains (-2, 1.2)` | (-3.8, 1.2) (-2, 1.2) (-2, 2.8) (-3.8, 2.8) | 🟢 |
| 151 | `snapped-points == 5` | 5 | 🟢 |
| 166 | `snap contains Endpoint · Line1 · Rectangle1` | Endpoint · Line1 · Rectangle1 | 🟢 |
| 167 | `transform-shift-x == -2` | -2 | 🟢 |
| 168 | `transform-shift-y ~ 1.2` | 1.2 | 🟢 |
| 169 | `transform-shift-z == 0` | 0 | 🟢 |
| 170 | `transform-readout contains Endpoint` | Move screen (-2, 1.2, 0) · Endpoint Edge Edge(Extrude#2.Cap(Start)/Extrude#2.Side(Line... | 🟢 |
| 174 | `transform-commits == 1` | 1 | 🟢 |
| 175 | `volume ~ 10.22278` | 10.22277375 | 🟢 |
| 183 | `snap lacks Endpoint` | none | 🟢 |
| 188 | `snap contains Endpoint` | Endpoint · Edge Edge(Extrude#2.Cap(Start)/Extrude#2.Side(Line3)) | 🟢 |
| 189 | `snap-x == 0` | 0 | 🟢 |
| 190 | `snap-y == 2.5` | 2.5 | 🟢 |
| 205 | `steps == 11` | 11 | 🟢 |
| 220 | `snap contains Endpoint · Curve Section2` | Endpoint · Curve Section2 | 🟢 |
| 221 | `pole-snap-gap == 0` | 0 | 🟢 |
| 225 | `pole-edits == 1` | 1 | 🟢 |
| 226 | `snapped-points == 6` | 6 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 160SnapEndpoint | 29 | `c0e7bf51f8859d87` |
| 161SnapTangent | 42 | `f28cc94d0a0d9c8d` |
| 162SnapLockTangent | 60 | `7d3db6d93115941a` |
| 163SnapLockNormal | 78 | `63f9c4b0802ce5f7` |
| 164SnapBinormalRefused | 92 | `92e570da56385acd` |
| 165SnapCentre | 98 | `58853102216264bb` |
| 166SnapMidpoint | 104 | `05339289d664b976` |
| 167SnapPerpendicular | 117 | `0bca509a98aa239e` |
| 168SnapSquare | 150 | `ff980cbe5ecb448c` |
| 169SnapConstraintDrag | 180 | `119b2789fb49bcd1` |
| 170SnapMove | 216 | `cab59f666071009f` |
| 171SnapXRay | 249 | `738e38d19ac1b135` |
| 172SnapPoleDrag | 314 | `f41c5ae27d64281d` |

## Journal

- Circle on plane XY: click centre and rim · V two-point mode · Esc aborts
- Circle1 · 44 ms
- Line on plane XY: click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · Esc aborts
- Lock tangent through the last point · the same key releases it · object snaps align along it
- Autosaved _ClaudeScratch/SolidArcWorkspace/SnapRecovery.solidarc
- Lock normal through the last point · the same key releases it · object snaps align along it
- Lock binormal leaves the XY drawing plane: refused
- Line1 · 44 ms
- Rectangle on plane XY: click two corners · Alt centre mode · Esc aborts
- Rectangle: square (s releases)
- Rectangle1 · 45 ms
- Constrain Rectangle1 · 4 constraint(s) · DOF 4 · click points / curves (earlier sketches link), then D N H V O Q M Y L [ ] C · Shift+D reference · drag points · Esc ends
- Drag Line1.End of Rectangle1 · 45 ms
- Constrain ended
- Selected Block1 (11 faces)
- Move 1 body(ies) · X / Y / Z axis · Shift+X / Y / Z plane · type a value · click / Enter commits · Esc cancels
- Autosaved _ClaudeScratch/SolidArcWorkspace/SnapRecovery.solidarc
- Move (Block1) · 66 ms
- Undo
- Line on plane XY: click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · Esc aborts
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
- Move pole 2 of Section3 · 4 ms
- Moved pole 2 of Section3 by 350.00 mm · 4 ms
- Undo
