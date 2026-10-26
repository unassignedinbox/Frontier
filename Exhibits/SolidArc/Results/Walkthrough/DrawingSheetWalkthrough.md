# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 900 · 122 frames · 21.9196 s · slowest frame 1202.55 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/DrawingSheetWalkthrough.txt
- Expectations: 35 / 35 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 13 | `drawing-sheet == 0` | off | 🟢 |
| 16 | `drawing-sheet == 1` | A3 1:50 First | 🟢 |
| 17 | `drawing-sheet contains A3` | A3 1:50 First | 🟢 |
| 18 | `drawing-sheet contains First` | A3 1:50 First | 🟢 |
| 19 | `drawing-sheet-views == 4` | Front Top Right Isometric | 🟢 |
| 20 | `drawing-sheet-views == Front Top Right Isometric` | Front Top Right Isometric | 🟢 |
| 21 | `drawing-sheet-dimensions contains 4000` | 4000 1750 R250 1000 3000 1250 2500 ⌀600 ⌀1000 | 🟢 |
| 22 | `drawing-sheet-dimensions contains 2500` | 4000 1750 R250 1000 3000 1250 2500 ⌀600 ⌀1000 | 🟢 |
| 23 | `drawing-sheet-dimensions contains 1750` | 4000 1750 R250 1000 3000 1250 2500 ⌀600 ⌀1000 | 🟢 |
| 24 | `drawing-sheet-dimensions contains ⌀1000` | 4000 1750 R250 1000 3000 1250 2500 ⌀600 ⌀1000 | 🟢 |
| 25 | `drawing-sheet-dimensions contains ⌀600` | 4000 1750 R250 1000 3000 1250 2500 ⌀600 ⌀1000 | 🟢 |
| 26 | `drawing-sheet-texts contains Bracket` | TITLE / Bracket / DRAWING NO. / DWG-0001 / REV / A / DRAWN / DATE / MATERIAL / FIRST AN... | 🟢 |
| 27 | `drawing-sheet-refreshes == 1` | 1 | 🟢 |
| 28 | `drawing-sheet-saved == 0` | 0 | 🟢 |
| 34 | `drawing-sheet contains Third` | A3 1:50 Third | 🟢 |
| 35 | `drawing-sheet-refreshes == 2` | 2 | 🟢 |
| 36 | `drawing-sheet-saved == 1` | 1 | 🟢 |
| 43 | `drawing-sheet-views == Front Top Right Section A-A` | Front Top Right Section A-A | 🟢 |
| 44 | `drawing-sheet-section-area ~ 4.116587` | 4.116587385 | 🟢 |
| 58 | `drawing-sheet-texts contains Bracket B-1` | TITLE / Bracket B-1 / DRAWING NO. / DWG-0001 / REV / A / DRAWN / DATE / MATERIAL / AlSi... | 🟢 |
| 59 | `drawing-sheet-texts contains AlSi10Mg` | TITLE / Bracket B-1 / DRAWING NO. / DWG-0001 / REV / A / DRAWN / DATE / MATERIAL / AlSi... | 🟢 |
| 64 | `drawing-sheet-scale < 0.02` | 1:100 | 🟢 |
| 67 | `drawing-sheet-scale == 0.02` | 1:50 | 🟢 |
| 68 | `drawing-sheet contains 1:50` | A3 1:50 Third | 🟢 |
| 73 | `drawing-sheet-zoom ~ 1.728` | 1.728000164 | 🟢 |
| 77 | `drawing-sheet-zoom == 1` | 1 | 🟢 |
| 82 | `drawing-sheet-export contains Sheet.svg` | _ClaudeScratch/SolidArcWorkspace/Sheet.svg | 🟢 |
| 83 | `drawing-sheet-export > 20000` | _ClaudeScratch/SolidArcWorkspace/Sheet.svg | 🟢 |
| 86 | `drawing-sheet-export contains Sheet.dxf` | _ClaudeScratch/SolidArcWorkspace/Sheet.dxf | 🟢 |
| 87 | `drawing-sheet-export > 20000` | _ClaudeScratch/SolidArcWorkspace/Sheet.dxf | 🟢 |
| 93 | `drawing-sheet == 0` | off | 🟢 |
| 96 | `drawing-sheet contains Third` | A3 1:50 Third | 🟢 |
| 97 | `drawing-sheet-views contains Section A-A` | Front Top Right Section A-A | 🟢 |
| 98 | `drawing-sheet-texts contains AlSi10Mg` | TITLE / Bracket B-1 / DRAWING NO. / DWG-0001 / REV / A / DRAWN / DATE / MATERIAL / AlSi... | 🟢 |
| 101 | `drawing-sheet == 0` | off | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 190DrawingSheetFirstAngle | 10 | `720aeed8bdcaf5cb` |
| 191DrawingSheetThirdAngle | 22 | `977c67f867d8c050` |
| 192DrawingSheetSection | 31 | `0ac7f887d8d97810` |
| 193DrawingSheetZoom | 83 | `77005a06dd59229c` |
| 194DrawingSheetExport | 106 | `a598f1acb8371bf6` |

## Journal

- Drawing sheet A3 · 1:50 · first angle · 4 views · 9 dimensions
- Drawing sheet A3 · 1:50 · third angle · 4 views · 9 dimensions
- Drawing sheet A3 · 1:50 · third angle · 4 views · 9 dimensions
- Drawing sheet A3 · 1:50 · third angle · 4 views · 9 dimensions
- Drawing sheet A3 · 1:50 · third angle · 4 views · 9 dimensions
- Drawing sheet A3 · 1:100 · third angle · 4 views · 9 dimensions
- Drawing sheet A3 · 1:50 · third angle · 4 views · 9 dimensions
- Exported _ClaudeScratch/SolidArcWorkspace/Sheet.svg (120780 bytes)
- Exported _ClaudeScratch/SolidArcWorkspace/Sheet.dxf (322299 bytes)
- Drawing sheet off
- Drawing sheet A3 · 1:50 · third angle · 4 views · 9 dimensions
- Drawing sheet off
