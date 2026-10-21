# DraftingWorkspaceCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0
- Frame: 1600 x 900 · 305 frames · 47.0333 s · slowest frame 533.509 ms
- Script: /home/user/Frontier/Experimental/DraftingWorkspace/Scripts/MultiBodyWalkthrough.txt
- Expectations: 32 / 32 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 10 | `bodies == 1` | 1 | 🟢 |
| 11 | `steps == 8` | 8 | 🟢 |
| 37 | `refused == 0` | 0 | 🟢 |
| 38 | `bodies == 2` | 2 | 🟢 |
| 39 | `steps == 10` | 10 | 🟢 |
| 40 | `body-volumes contains Block2 3.000000` | Block1 10.222774 · Block2 3.000000 | 🟢 |
| 41 | `status contains New body` | Block2 · New body · 270 ms | 🟢 |
| 69 | `refused == 0` | 0 | 🟢 |
| 70 | `bodies == 3` | 3 | 🟢 |
| 71 | `body-captions contains Cylinder3` | Block1,Block2,Cylinder3 | 🟢 |
| 72 | `sound == 1` | 1 | 🟢 |
| 80 | `selected-bodies == 1` | Extrude#2 | 🟢 |
| 81 | `selected-faces == 0` |  | 🟢 |
| 89 | `selected-bodies contains Extrude` | Extrude#10 | 🟢 |
| 101 | `body-matcaps == 3` | 3 | 🟢 |
| 102 | `selection-matcap contains Brass` | Brass | 🟢 |
| 103 | `body-matcap contains Studio` | Studio | 🟢 |
| 104 | `steps == 12` | 12 | 🟢 |
| 116 | `selected-bodies == 2` | Extrude#10,Extrude#12 | 🟢 |
| 119 | `boolean == 1` | 1 | 🟢 |
| 122 | `status contains Difference` | Boolean: Difference | 🟢 |
| 126 | `boolean == 0` | 0 | 🟢 |
| 127 | `boolean-commits == 1` | 1 | 🟢 |
| 128 | `bodies == 2` | 2 | 🟢 |
| 129 | `refused == 0` | 0 | 🟢 |
| 130 | `sound == 1` | 1 | 🟢 |
| 131 | `body-volumes contains Block2 2.497345` | Block1 10.222774 · Block2 2.497345 | 🟢 |
| 132 | `matcap-dangling == 1` | 1 | 🟢 |
| 140 | `bodies == 3` | 3 | 🟢 |
| 141 | `matcap-dangling == 0` | 0 | 🟢 |
| 144 | `bodies == 2` | 2 | 🟢 |
| 145 | `body-volumes contains Block2 2.497345` | Block1 10.222774 · Block2 2.497345 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 96MultiBodyNew | 98 | `de469e4c03247287` |
| 97MultiBodyThree | 175 | `c832009bf6251d96` |
| 98MultiBodyMatcaps | 248 | `131298969a13141a` |
| 99MultiBodyBoolean | 278 | `3a9a1284ac8a5899` |
| 100MultiBodyDrilled | 291 | `7efea92fb8cff2e1` |
| 101MultiBodyUndo | 305 | `53dd7274dcef23d5` |

## Journal

- Block2 · New body · 270 ms
- Framed the document (Space)
- Autosaved _ClaudeScratch/DraftingWorkspace/MultiBodyRecovery.drafting.toml
- Cylinder3 · New body · 342 ms
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
- Autosaved _ClaudeScratch/DraftingWorkspace/MultiBodyRecovery.drafting.toml
- Combine · Difference (Block2 − Cylinder3) · 369 ms
- Selection cleared
- Undo
- Redo
