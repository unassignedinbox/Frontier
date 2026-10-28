# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 810 · 330 frames · 30.6939 s · slowest frame 334.93 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/MultiBodyWalkthrough.txt
- Expectations: 32 / 32 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 13 | `bodies == 1` | 1 | 🟢 |
| 14 | `steps == 8` | 8 | 🟢 |
| 44 | `refused == 0` | 0 | 🟢 |
| 45 | `bodies == 2` | 2 | 🟢 |
| 46 | `steps == 10` | 10 | 🟢 |
| 47 | `body-volumes contains Block2 3.000000` | Block1 10.222774 · Block2 3.000000 | 🟢 |
| 48 | `status contains New body` | Block2 · New body · 1 ms | 🟢 |
| 80 | `refused == 0` | 0 | 🟢 |
| 81 | `bodies == 3` | 3 | 🟢 |
| 82 | `body-captions contains Cylinder3` | Block1,Block2,Cylinder3 | 🟢 |
| 83 | `sound == 1` | 1 | 🟢 |
| 91 | `selected-bodies == 1` | Extrude#2 | 🟢 |
| 92 | `selected-faces == 0` |  | 🟢 |
| 100 | `selected-bodies contains Extrude` | Extrude#10 | 🟢 |
| 112 | `body-matcaps == 3` | 3 | 🟢 |
| 113 | `selection-matcap contains Brass` | Brass | 🟢 |
| 114 | `body-matcap contains Studio` | Studio | 🟢 |
| 115 | `steps == 12` | 12 | 🟢 |
| 127 | `selected-bodies == 2` | Extrude#10,Extrude#12 | 🟢 |
| 130 | `boolean == 1` | 1 | 🟢 |
| 133 | `status contains Difference` | Boolean: Difference | 🟢 |
| 137 | `boolean == 0` | 0 | 🟢 |
| 138 | `boolean-commits == 1` | 1 | 🟢 |
| 139 | `bodies == 2` | 2 | 🟢 |
| 140 | `refused == 0` | 0 | 🟢 |
| 141 | `sound == 1` | 1 | 🟢 |
| 142 | `body-volumes contains Block2 2.497345` | Block1 10.222774 · Block2 2.497345 | 🟢 |
| 143 | `matcap-dangling == 1` | 1 | 🟢 |
| 151 | `bodies == 3` | 3 | 🟢 |
| 152 | `matcap-dangling == 0` | 0 | 🟢 |
| 155 | `bodies == 2` | 2 | 🟢 |
| 156 | `body-volumes contains Block2 2.497345` | Block1 10.222774 · Block2 2.497345 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 96MultiBodyNew | 111 | `da8170777f13b498` |
| 97MultiBodyThree | 200 | `8df304277f586c12` |
| 98MultiBodyMatcaps | 273 | `122a734a5238f828` |
| 99MultiBodyBoolean | 303 | `84a7be5624512881` |
| 100MultiBodyDrilled | 316 | `8362d7df92ff697d` |
| 101MultiBodyUndo | 330 | `4bc206e86c24fa2d` |

## Journal

- Block: click the first corner · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Block2 · New body · 1 ms
- Framed the document (Space)
- Cylinder: click the base centre · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Autosaved _ClaudeScratch/SolidArcWorkspace/MultiBodyRecovery.solidarc
- Cylinder3 · New body · 1 ms
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
- Combine · Difference (Block2 − Cylinder3) · 2 ms
- Selection cleared
- Undo
- Redo
