# DraftingWorkspaceCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0
- Frame: 1600 x 900 · 224 frames · 23.5689 s · slowest frame 413.382 ms
- Script: /home/user/Frontier/Experimental/DraftingWorkspace/Scripts/SnappingWalkthrough.txt
- Expectations: 52 / 52 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 18 | `steps == 9` | 9 | 🟢 |
| 25 | `snap contains Endpoint` | Endpoint · Edge Edge(Extrude#2.Cap(Start)/Extrude#2.Side(Line1)) | 🟢 |
| 26 | `snap-x == 0` | 0 | 🟢 |
| 27 | `snap-y == 0` | 0 | 🟢 |
| 28 | `snap-z == 0` | 0 | 🟢 |
| 32 | `drawing-points == 1` | 1 | 🟢 |
| 33 | `drawing-last-x == 0` | 0 | 🟢 |
| 34 | `drawing-last-y == 0` | 0 | 🟢 |
| 39 | `snap contains Tangent` | Tangent · Circle1 | 🟢 |
| 40 | `snap-x ~ -1.569338679` | -1.569338679 | 🟢 |
| 41 | `snap-y ~ 1.617768868` | 1.617768868 | 🟢 |
| 45 | `drawing-points == 2` | 2 | 🟢 |
| 46 | `drawing-last-x ~ -1.569338679` | -1.569338679 | 🟢 |
| 51 | `snap-lock contains tangent` | tangent | 🟢 |
| 54 | `snap contains tangent` | Axis · Lock tangent | 🟢 |
| 55 | `drawing-pointer-x ~ -2.265620126` | -2.265620126 | 🟢 |
| 56 | `drawing-pointer-y ~ 2.335537736` | 2.335537736 | 🟢 |
| 60 | `drawing-points == 3` | 3 | 🟢 |
| 61 | `snap-lock contains none` | none | 🟢 |
| 66 | `snap-lock contains normal` | normal | 🟢 |
| 69 | `drawing-pointer-x ~ -2.624504560` | -2.62450456 | 🟢 |
| 70 | `drawing-pointer-y ~ 1.987397012` | 1.987397012 | 🟢 |
| 74 | `drawing-points == 4` | 4 | 🟢 |
| 79 | `snap-lock contains none` | none | 🟢 |
| 85 | `snap contains Centre` | Centre · Circle1 | 🟢 |
| 86 | `snap-x == -2` | -2 | 🟢 |
| 87 | `snap-y ~ 1.2` | 1.2 | 🟢 |
| 91 | `snap contains Midpoint` | Midpoint · Edge Edge(Extrude#2.Cap(Start)/Extrude#2.Side(Line1)) | 🟢 |
| 92 | `snap-x == 2` | 2 | 🟢 |
| 93 | `snap-y == 0` | 0 | 🟢 |
| 97 | `drawing-points == 5` | 5 | 🟢 |
| 102 | `snap contains Perpendicular` | Perpendicular · Circle1 | 🟢 |
| 103 | `snap-x ~ -1.425304229` | -1.425304229 | 🟢 |
| 104 | `snap-y ~ 1.027591269` | 1.027591269 | 🟢 |
| 108 | `drawing-points == 6` | 6 | 🟢 |
| 109 | `snapped-points == 4` | 4 | 🟢 |
| 112 | `steps == 10` | 10 | 🟢 |
| 121 | `drawing-square == 1` | 1 | 🟢 |
| 124 | `drawing-pointer-x ~ -3.8` | -3.8 | 🟢 |
| 125 | `drawing-pointer-y ~ 2.8` | 2.8 | 🟢 |
| 129 | `steps == 11` | 11 | 🟢 |
| 142 | `snap contains Centre` | Centre · Circle1 | 🟢 |
| 143 | `transform-shift-x == -2` | -2 | 🟢 |
| 144 | `transform-shift-y ~ 1.2` | 1.2 | 🟢 |
| 145 | `transform-shift-z == 0` | 0 | 🟢 |
| 146 | `transform-readout contains Endpoint` | Move screen (-2, 1.2, 0) · Endpoint Edge Edge(Extrude#2.Cap(Start)/Extrude#2.Side(Line... | 🟢 |
| 150 | `transform-commits == 1` | 1 | 🟢 |
| 151 | `volume ~ 10.22278` | 10.22277375 | 🟢 |
| 159 | `snap lacks Endpoint` | none | 🟢 |
| 164 | `snap contains Endpoint` | Endpoint · Edge Edge(Extrude#2.Cap(Start)/Extrude#2.Side(Line3)) | 🟢 |
| 165 | `snap-x == 0` | 0 | 🟢 |
| 166 | `snap-y == 2.5` | 2.5 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 160SnapEndpoint | 29 | `eae7291abc15fffb` |
| 161SnapTangent | 42 | `64b45798c0e965d1` |
| 162SnapLockTangent | 60 | `2392a25d92d46d3f` |
| 163SnapLockNormal | 78 | `9142c78c686746be` |
| 164SnapBinormalRefused | 92 | `e46be79b5dce29f7` |
| 165SnapCentre | 98 | `83d4dbd39731b9cd` |
| 166SnapMidpoint | 104 | `231acadfb2ca9d07` |
| 167SnapPerpendicular | 117 | `20ada2005d42bd1e` |
| 168SnapSquare | 150 | `eff10681e4ede74d` |
| 169SnapMove | 183 | `d86bc671c64119df` |
| 170SnapXRay | 216 | `8b4722a160bfde36` |

## Journal

- Circle on plane XY: click centre and rim · V two-point mode · Esc aborts
- Circle1 · 56 ms
- Line on plane XY: click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · Esc aborts
- Lock tangent through the last point · the same key releases it · object snaps align along it
- Autosaved _ClaudeScratch/DraftingWorkspace/SnapRecovery.drafting.toml
- Lock normal through the last point · the same key releases it · object snaps align along it
- Lock binormal leaves the XY drawing plane: refused
- Line1 · 46 ms
- Rectangle on plane XY: click two corners · Alt centre mode · Esc aborts
- Rectangle: square (s releases)
- Rectangle1 · 44 ms
- Selected Block1 (11 faces)
- Move 1 body(ies) · X / Y / Z axis · Shift+X / Y / Z plane · type a value · click / Enter commits · Esc cancels
- Move (Block1) · 66 ms
- Undo
- Line on plane XY: click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · Esc aborts
- X-ray on · faces 45 % · hidden edges pickable
- Line aborted (document untouched)
- X-ray off
