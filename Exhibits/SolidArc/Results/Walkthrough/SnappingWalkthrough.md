# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0
- Frame: 1600 x 900 · 332 frames · 24.7969 s · slowest frame 346.321 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/SnappingWalkthrough.txt
- Expectations: 62 / 62 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 20 | `steps == 9` | 9 | 🟢 |
| 27 | `snap contains Endpoint` | Endpoint · Edge Edge(Extrude#2.Cap(Start)/Extrude#2.Side(Line1)) | 🟢 |
| 28 | `snap-x == 0` | 0 | 🟢 |
| 29 | `snap-y == 0` | 0 | 🟢 |
| 30 | `snap-z == 0` | 0 | 🟢 |
| 34 | `drawing-points == 1` | 1 | 🟢 |
| 35 | `drawing-last-x == 0` | 0 | 🟢 |
| 36 | `drawing-last-y == 0` | 0 | 🟢 |
| 41 | `snap contains Tangent` | Tangent · Circle1 | 🟢 |
| 42 | `snap-x ~ -1.569338679` | -1.569338679 | 🟢 |
| 43 | `snap-y ~ 1.617768868` | 1.617768868 | 🟢 |
| 47 | `drawing-points == 2` | 2 | 🟢 |
| 48 | `drawing-last-x ~ -1.569338679` | -1.569338679 | 🟢 |
| 53 | `snap-lock contains tangent` | tangent | 🟢 |
| 56 | `snap contains tangent` | Axis · Lock tangent | 🟢 |
| 57 | `drawing-pointer-x ~ -2.265620126` | -2.265620126 | 🟢 |
| 58 | `drawing-pointer-y ~ 2.335537736` | 2.335537736 | 🟢 |
| 62 | `drawing-points == 3` | 3 | 🟢 |
| 63 | `snap-lock contains none` | none | 🟢 |
| 68 | `snap-lock contains normal` | normal | 🟢 |
| 71 | `drawing-pointer-x ~ -2.624504560` | -2.62450456 | 🟢 |
| 72 | `drawing-pointer-y ~ 1.987397012` | 1.987397012 | 🟢 |
| 76 | `drawing-points == 4` | 4 | 🟢 |
| 81 | `snap-lock contains none` | none | 🟢 |
| 87 | `snap contains Centre` | Centre · Circle1 | 🟢 |
| 88 | `snap-x == -2` | -2 | 🟢 |
| 89 | `snap-y ~ 1.2` | 1.2 | 🟢 |
| 93 | `snap contains Midpoint` | Midpoint · Edge Edge(Extrude#2.Cap(Start)/Extrude#2.Side(Line1)) | 🟢 |
| 94 | `snap-x == 2` | 2 | 🟢 |
| 95 | `snap-y == 0` | 0 | 🟢 |
| 99 | `drawing-points == 5` | 5 | 🟢 |
| 104 | `snap contains Perpendicular` | Perpendicular · Circle1 | 🟢 |
| 105 | `snap-x ~ -1.425304229` | -1.425304229 | 🟢 |
| 106 | `snap-y ~ 1.027591269` | 1.027591269 | 🟢 |
| 110 | `drawing-points == 6` | 6 | 🟢 |
| 111 | `snapped-points == 4` | 4 | 🟢 |
| 114 | `steps == 10` | 10 | 🟢 |
| 123 | `drawing-square == 1` | 1 | 🟢 |
| 126 | `drawing-pointer-x ~ -3.8` | -3.8 | 🟢 |
| 127 | `drawing-pointer-y ~ 2.8` | 2.8 | 🟢 |
| 131 | `steps == 11` | 11 | 🟢 |
| 137 | `constrain == 1` | 1 | 🟢 |
| 146 | `snap contains Centre` | Centre · Circle1 | 🟢 |
| 147 | `snap-x == -2` | -2 | 🟢 |
| 151 | `sketch-points contains (-2, 1.2)` | (-3.8, 1.2) (-2, 1.2) (-2, 2.8) (-3.8, 2.8) | 🟢 |
| 152 | `snapped-points == 5` | 5 | 🟢 |
| 167 | `snap contains Endpoint · Line1 · Rectangle1` | Endpoint · Line1 · Rectangle1 | 🟢 |
| 168 | `transform-shift-x == -2` | -2 | 🟢 |
| 169 | `transform-shift-y ~ 1.2` | 1.2 | 🟢 |
| 170 | `transform-shift-z == 0` | 0 | 🟢 |
| 171 | `transform-readout contains Endpoint` | Move screen (-2, 1.2, 0) · Endpoint Edge Edge(Extrude#2.Cap(Start)/Extrude#2.Side(Line... | 🟢 |
| 175 | `transform-commits == 1` | 1 | 🟢 |
| 176 | `volume ~ 10.22278` | 10.22277375 | 🟢 |
| 184 | `snap lacks Endpoint` | none | 🟢 |
| 189 | `snap contains Endpoint` | Endpoint · Edge Edge(Extrude#2.Cap(Start)/Extrude#2.Side(Line3)) | 🟢 |
| 190 | `snap-x == 0` | 0 | 🟢 |
| 191 | `snap-y == 2.5` | 2.5 | 🟢 |
| 206 | `steps == 11` | 11 | 🟢 |
| 223 | `snap contains Endpoint · Curve Section2` | Endpoint · Curve Section2 | 🟢 |
| 224 | `pole-snap-gap == 0` | 0 | 🟢 |
| 228 | `pole-edits == 1` | 1 | 🟢 |
| 229 | `snapped-points == 6` | 6 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 160SnapEndpoint | 29 | `2a0e8222e9aea116` |
| 161SnapTangent | 42 | `1d4536d0bddb4eda` |
| 162SnapLockTangent | 60 | `61f12f3512375430` |
| 163SnapLockNormal | 78 | `67a96528b1dc2848` |
| 164SnapBinormalRefused | 92 | `4ec25709ec8eba87` |
| 165SnapCentre | 98 | `dc3f060d31db95f8` |
| 166SnapMidpoint | 104 | `e568b7924ef2da26` |
| 167SnapPerpendicular | 117 | `2e7dd6cf89a86c2b` |
| 168SnapSquare | 150 | `fbc5e5f9e2fd489d` |
| 169SnapConstraintDrag | 180 | `ef6104c7a46711d5` |
| 170SnapMove | 216 | `4338789e04980fbd` |
| 171SnapXRay | 249 | `16a61de1cd12cc59` |
| 172SnapPoleDrag | 321 | `02fc162cd230f126` |

## Journal

- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres · Esc aborts
- Circle1 · 38 ms
- Line on plane XY (or the body face under the first click): click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres · Esc aborts
- Lock tangent through the last point · the same key releases it · object snaps align along it
- Autosaved _ClaudeScratch/SolidArcWorkspace/SnapRecovery.solidarc
- Lock normal through the last point · the same key releases it · object snaps align along it
- Lock binormal leaves the XY drawing plane: refused
- Line1 · 36 ms
- Rectangle on plane XY (or the body face under the first click): click two corners · Alt centre mode · hold Ctrl to snap to ends / midpoints / centres · Esc aborts
- Rectangle: square (s releases)
- Rectangle1 · 36 ms
- Constrain Rectangle1 · 4 constraint(s) · DOF 4 · click points / curves (earlier sketches link), then D N H V O Q M Y L [ ] C · Shift+D reference · drag points · Esc ends
- Drag Line1.End of Rectangle1 · 40 ms
- Constrain ended
- Selected Block1 (11 faces)
- Move 1 body(ies) · X / Y / Z axis · Shift+X / Y / Z plane · type a value · click / Enter commits · Esc cancels
- Autosaved _ClaudeScratch/SolidArcWorkspace/SnapRecovery.solidarc
- Move (Block1) · 57 ms
- Undo
- Line on plane XY (or the body face under the first click): click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres · Esc aborts
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
