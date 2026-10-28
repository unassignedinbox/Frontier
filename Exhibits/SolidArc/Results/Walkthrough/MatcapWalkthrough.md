# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 810 · 155 frames · 14.3751 s · slowest frame 370.134 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/MatcapWalkthrough.txt
- Expectations: 30 / 30 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 14 | `faces == 11` | 11 | 🟢 |
| 15 | `steps == 8` | 8 | 🟢 |
| 16 | `body-matcap contains Studio` | Studio | 🟢 |
| 17 | `face-matcaps == 0` | 0 | 🟢 |
| 18 | `matcap-layers == 14` | 14 | 🟢 |
| 19 | `matcap-thumbnails == 14` | 14 | 🟢 |
| 25 | `body-matcap contains BrushedSteel` | BrushedSteel | 🟢 |
| 26 | `selection-matcap contains BrushedSteel` | BrushedSteel | 🟢 |
| 27 | `status contains Matcap BrushedSteel → body` | Matcap BrushedSteel → body | 🟢 |
| 28 | `steps == 8` | 8 | 🟢 |
| 38 | `selected-faces == 2` | Extrude#2.Cap(End),Extrude#4.Cap(End) | 🟢 |
| 41 | `matcap-picker == 1` | 1 | 🟢 |
| 45 | `matcap-picker == 0` | 0 | 🟢 |
| 46 | `face-matcaps == 2` | 2 | 🟢 |
| 47 | `selection-matcap contains Brass` | Brass | 🟢 |
| 48 | `status contains Matcap Brass → 2 face(s)` | Matcap Brass → 2 face(s) | 🟢 |
| 51 | `selected-faces == 0` |  | 🟢 |
| 58 | `selected-faces contains Side(Line1)` | Extrude#2.Side(Line1) | 🟢 |
| 66 | `selected-faces == 1` | Extrude#4.Side(Circle1) | 🟢 |
| 71 | `face-matcaps == 4` | 4 | 🟢 |
| 74 | `selected-faces == 3` | Extrude#4.Side(Circle1),Extrude#2.Cap(End),Extrude#2.Side(Line1) | 🟢 |
| 75 | `selection-matcap contains mixed` | mixed | 🟢 |
| 78 | `matcap-dangling == 0` | 0 | 🟢 |
| 87 | `face-matcaps == 3` | 3 | 🟢 |
| 88 | `selection-matcap contains BrushedSteel` | BrushedSteel | 🟢 |
| 92 | `body-matcap contains Studio` | Studio | 🟢 |
| 95 | `body-matcap contains BrushedSteel` | BrushedSteel | 🟢 |
| 98 | `face-matcaps == 4` | 4 | 🟢 |
| 101 | `face-matcaps == 3` | 3 | 🟢 |
| 102 | `steps == 8` | 8 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 90MatcapStudio | 6 | `352c09078fb66477` |
| 91MatcapBody | 18 | `913cea550bdbec86` |
| 92MatcapPicker | 43 | `cc7cc1218adf83f6` |
| 93MatcapFaces | 57 | `167ce5616d6e73ac` |
| 94MatcapMixed | 116 | `0f98dfc65c36545d` |
| 95MatcapUndo | 155 | `56b448ab9efbad00` |

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
