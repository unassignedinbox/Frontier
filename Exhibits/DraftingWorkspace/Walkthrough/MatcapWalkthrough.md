# DraftingWorkspaceCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0
- Frame: 1600 x 900 · 151 frames · 13.5606 s · slowest frame 361.998 ms
- Script: /home/user/Frontier/Experimental/DraftingWorkspace/Scripts/MatcapWalkthrough.txt
- Expectations: 30 / 30 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 11 | `faces == 11` | 11 | 🟢 |
| 12 | `steps == 8` | 8 | 🟢 |
| 13 | `body-matcap contains Studio` | Studio | 🟢 |
| 14 | `face-matcaps == 0` | 0 | 🟢 |
| 15 | `matcap-layers == 14` | 14 | 🟢 |
| 16 | `matcap-thumbnails == 14` | 14 | 🟢 |
| 22 | `body-matcap contains BrushedSteel` | BrushedSteel | 🟢 |
| 23 | `selection-matcap contains BrushedSteel` | BrushedSteel | 🟢 |
| 24 | `status contains Matcap BrushedSteel → body` | Matcap BrushedSteel → body | 🟢 |
| 25 | `steps == 8` | 8 | 🟢 |
| 35 | `selected-faces == 2` | Extrude#2.Cap(End),Extrude#4.Cap(End) | 🟢 |
| 38 | `matcap-picker == 1` | 1 | 🟢 |
| 42 | `matcap-picker == 0` | 0 | 🟢 |
| 43 | `face-matcaps == 2` | 2 | 🟢 |
| 44 | `selection-matcap contains Brass` | Brass | 🟢 |
| 45 | `status contains Matcap Brass → 2 face(s)` | Matcap Brass → 2 face(s) | 🟢 |
| 48 | `selected-faces == 0` |  | 🟢 |
| 55 | `selected-faces contains Side(Line1)` | Extrude#2.Side(Line1) | 🟢 |
| 63 | `selected-faces == 1` | Extrude#4.Side(Circle1) | 🟢 |
| 68 | `face-matcaps == 4` | 4 | 🟢 |
| 71 | `selected-faces == 3` | Extrude#4.Side(Circle1),Extrude#2.Cap(End),Extrude#2.Side(Line1) | 🟢 |
| 72 | `selection-matcap contains mixed` | mixed | 🟢 |
| 75 | `matcap-dangling == 0` | 0 | 🟢 |
| 84 | `face-matcaps == 3` | 3 | 🟢 |
| 85 | `selection-matcap contains BrushedSteel` | BrushedSteel | 🟢 |
| 89 | `body-matcap contains Studio` | Studio | 🟢 |
| 92 | `body-matcap contains BrushedSteel` | BrushedSteel | 🟢 |
| 95 | `face-matcaps == 4` | 4 | 🟢 |
| 98 | `face-matcaps == 3` | 3 | 🟢 |
| 99 | `steps == 8` | 8 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 90MatcapStudio | 5 | `a1dd20b6998a9b89` |
| 91MatcapBody | 14 | `d4d5f8da5e6ceb46` |
| 92MatcapPicker | 39 | `da28f5a35be3f811` |
| 93MatcapFaces | 53 | `9d7f6da6a9911488` |
| 94MatcapMixed | 112 | `b0304f02bc6d6a5c` |
| 95MatcapUndo | 151 | `135f93edb5f43de3` |

## Journal

- Matcap BrushedSteel → body
- Selected Extrude#2.Cap(End)
- Added Extrude#4.Cap(End)
- Matcap Brass → 2 face(s)
- Selection cleared
- Selected Extrude#2.Side(Line1)
- Matcap RedPlastic → 1 face(s)
- Selected Extrude#4.Side(Circle1)
- Matcap Chrome → 1 face(s)
- Added Extrude#2.Cap(End)
- Added Extrude#2.Side(Line1)
- Selection cleared
- Selected Extrude#2.Side(Line1)
- Matcap removed → 1 face(s) show the body
- Selection cleared
- Matcap removed → body Studio
- Undo
- Undo
- Redo
