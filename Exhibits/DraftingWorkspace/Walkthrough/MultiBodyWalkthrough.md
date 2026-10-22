# DraftingWorkspaceCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0
- Frame: 1600 x 900 · 305 frames · 31.6219 s · slowest frame 395.16 ms
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
| 41 | `status contains New body` | Block2 · New body · 77 ms | 🟢 |
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
| 96MultiBodyNew | 98 | `7a9596dbe496c69b` |
| 97MultiBodyThree | 175 | `643d7dca344c735d` |
| 98MultiBodyMatcaps | 248 | `271cca560a422fc3` |
| 99MultiBodyBoolean | 278 | `5f1410d1ece1bcc4` |
| 100MultiBodyDrilled | 291 | `ed36e661ef46e283` |
| 101MultiBodyUndo | 305 | `ffe7421fe36a6e4d` |

## Journal

- Block2 · New body · 77 ms
- Framed the document (Space)
- Autosaved _ClaudeScratch/DraftingWorkspace/MultiBodyRecovery.drafting.toml
- Cylinder3 · New body · 107 ms
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
- Combine · Difference (Block2 − Cylinder3) · 133 ms
- Selection cleared
- Undo
- Redo
