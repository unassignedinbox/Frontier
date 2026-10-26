# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 900 · 329 frames · 44.7271 s · slowest frame 553.156 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/MultiBodyWalkthrough.txt
- Expectations: 32 / 32 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 10 | `bodies == 1` | 1 | 🟢 |
| 11 | `steps == 8` | 8 | 🟢 |
| 41 | `refused == 0` | 0 | 🟢 |
| 42 | `bodies == 2` | 2 | 🟢 |
| 43 | `steps == 10` | 10 | 🟢 |
| 44 | `body-volumes contains Block2 3.000000` | Block1 10.222774 · Block2 3.000000 | 🟢 |
| 45 | `status contains New body` | Block2 · New body · 73 ms | 🟢 |
| 77 | `refused == 0` | 0 | 🟢 |
| 78 | `bodies == 3` | 3 | 🟢 |
| 79 | `body-captions contains Cylinder3` | Block1,Block2,Cylinder3 | 🟢 |
| 80 | `sound == 1` | 1 | 🟢 |
| 88 | `selected-bodies == 1` | Extrude#2 | 🟢 |
| 89 | `selected-faces == 0` |  | 🟢 |
| 97 | `selected-bodies contains Extrude` | Extrude#10 | 🟢 |
| 109 | `body-matcaps == 3` | 3 | 🟢 |
| 110 | `selection-matcap contains Brass` | Brass | 🟢 |
| 111 | `body-matcap contains Studio` | Studio | 🟢 |
| 112 | `steps == 12` | 12 | 🟢 |
| 124 | `selected-bodies == 2` | Extrude#10,Extrude#12 | 🟢 |
| 127 | `boolean == 1` | 1 | 🟢 |
| 130 | `status contains Difference` | Boolean: Difference | 🟢 |
| 134 | `boolean == 0` | 0 | 🟢 |
| 135 | `boolean-commits == 1` | 1 | 🟢 |
| 136 | `bodies == 2` | 2 | 🟢 |
| 137 | `refused == 0` | 0 | 🟢 |
| 138 | `sound == 1` | 1 | 🟢 |
| 139 | `body-volumes contains Block2 2.497345` | Block1 10.222774 · Block2 2.497345 | 🟢 |
| 140 | `matcap-dangling == 1` | 1 | 🟢 |
| 148 | `bodies == 3` | 3 | 🟢 |
| 149 | `matcap-dangling == 0` | 0 | 🟢 |
| 152 | `bodies == 2` | 2 | 🟢 |
| 153 | `body-volumes contains Block2 2.497345` | Block1 10.222774 · Block2 2.497345 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 96MultiBodyNew | 110 | `f2d145ce5d1bf1ca` |
| 97MultiBodyThree | 199 | `da874848625076ae` |
| 98MultiBodyMatcaps | 272 | `ffa4d795351d1cf1` |
| 99MultiBodyBoolean | 302 | `cc4881b44e64ae6b` |
| 100MultiBodyDrilled | 315 | `5e51a9a00510978a` |
| 101MultiBodyUndo | 329 | `827b245f184e9db2` |

## Journal

- Block: click the first corner · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Block2 · New body · 73 ms
- Framed the document (Space)
- Cylinder: click the base centre · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Autosaved _ClaudeScratch/SolidArcWorkspace/MultiBodyRecovery.solidarc
- Cylinder3 · New body · 110 ms
- Selected Block1 (11 faces)
- Matcap BrushedSteel → 1 body(ies)
- Selected Block2 (6 faces)
- Matcap BluePaint → 1 body(ies)
- Selected Cylinder3 (3 faces)
- Matcap Brass → 1 body(ies)
- Selection cleared
- Selected Block2 (6 faces)
- Added Cylinder3 · 2 bodies picked
- Boolean: Block2 with 1 tool(s) · Q union · W difference · E intersect · Enter applies
- Boolean: Difference
- Autosaved _ClaudeScratch/SolidArcWorkspace/MultiBodyRecovery.solidarc
- Combine · Difference (Block2 − Cylinder3) · 124 ms
- Selection cleared
- Undo
- Redo
