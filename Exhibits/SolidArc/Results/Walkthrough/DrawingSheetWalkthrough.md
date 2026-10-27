# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 900 · 123 frames · 18.2886 s · slowest frame 547.689 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/DrawingSheetWalkthrough.txt
- Expectations: 35 / 35 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 16 | `drawing-sheet == 0` | off | 🟢 |
| 19 | `drawing-sheet == 1` | A3 1:50 First | 🟢 |
| 20 | `drawing-sheet contains A3` | A3 1:50 First | 🟢 |
| 21 | `drawing-sheet contains First` | A3 1:50 First | 🟢 |
| 22 | `drawing-sheet-views == 4` | Front Top Right Isometric | 🟢 |
| 23 | `drawing-sheet-views == Front Top Right Isometric` | Front Top Right Isometric | 🟢 |
| 24 | `drawing-sheet-dimensions contains 4000` | 4000 1750 R250 1000 3000 1250 2500 ⌀600 ⌀1000 | 🟢 |
| 25 | `drawing-sheet-dimensions contains 2500` | 4000 1750 R250 1000 3000 1250 2500 ⌀600 ⌀1000 | 🟢 |
| 26 | `drawing-sheet-dimensions contains 1750` | 4000 1750 R250 1000 3000 1250 2500 ⌀600 ⌀1000 | 🟢 |
| 27 | `drawing-sheet-dimensions contains ⌀1000` | 4000 1750 R250 1000 3000 1250 2500 ⌀600 ⌀1000 | 🟢 |
| 28 | `drawing-sheet-dimensions contains ⌀600` | 4000 1750 R250 1000 3000 1250 2500 ⌀600 ⌀1000 | 🟢 |
| 29 | `drawing-sheet-texts contains Bracket` | TITLE / Bracket / DRAWING NO. / DWG-0001 / REV / A / DRAWN / DATE / MATERIAL / FIRST AN... | 🟢 |
| 30 | `drawing-sheet-refreshes == 1` | 1 | 🟢 |
| 31 | `drawing-sheet-saved == 0` | 0 | 🟢 |
| 37 | `drawing-sheet contains Third` | A3 1:50 Third | 🟢 |
| 38 | `drawing-sheet-refreshes == 2` | 2 | 🟢 |
| 39 | `drawing-sheet-saved == 1` | 1 | 🟢 |
| 46 | `drawing-sheet-views == Front Top Right Section A-A` | Front Top Right Section A-A | 🟢 |
| 47 | `drawing-sheet-section-area ~ 4.116587` | 4.116587385 | 🟢 |
| 61 | `drawing-sheet-texts contains Bracket B-1` | TITLE / Bracket B-1 / DRAWING NO. / DWG-0001 / REV / A / DRAWN / DATE / MATERIAL / AlSi... | 🟢 |
| 62 | `drawing-sheet-texts contains AlSi10Mg` | TITLE / Bracket B-1 / DRAWING NO. / DWG-0001 / REV / A / DRAWN / DATE / MATERIAL / AlSi... | 🟢 |
| 67 | `drawing-sheet-scale < 0.02` | 1:100 | 🟢 |
| 70 | `drawing-sheet-scale == 0.02` | 1:50 | 🟢 |
| 71 | `drawing-sheet contains 1:50` | A3 1:50 Third | 🟢 |
| 76 | `drawing-sheet-zoom ~ 1.728` | 1.728000164 | 🟢 |
| 80 | `drawing-sheet-zoom == 1` | 1 | 🟢 |
| 85 | `drawing-sheet-export contains Sheet.svg` | _ClaudeScratch/SolidArcWorkspace/Sheet.svg | 🟢 |
| 86 | `drawing-sheet-export > 20000` | _ClaudeScratch/SolidArcWorkspace/Sheet.svg | 🟢 |
| 89 | `drawing-sheet-export contains Sheet.dxf` | _ClaudeScratch/SolidArcWorkspace/Sheet.dxf | 🟢 |
| 90 | `drawing-sheet-export > 20000` | _ClaudeScratch/SolidArcWorkspace/Sheet.dxf | 🟢 |
| 96 | `drawing-sheet == 0` | off | 🟢 |
| 99 | `drawing-sheet contains Third` | A3 1:50 Third | 🟢 |
| 100 | `drawing-sheet-views contains Section A-A` | Front Top Right Section A-A | 🟢 |
| 101 | `drawing-sheet-texts contains AlSi10Mg` | TITLE / Bracket B-1 / DRAWING NO. / DWG-0001 / REV / A / DRAWN / DATE / MATERIAL / AlSi... | 🟢 |
| 104 | `drawing-sheet == 0` | off | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 190DrawingSheetFirstAngle | 11 | `3fecf9bda500c9a8` |
| 191DrawingSheetThirdAngle | 23 | `6105b11d6af723ed` |
| 192DrawingSheetSection | 32 | `32d9321816886bf4` |
| 193DrawingSheetZoom | 84 | `dc8207a01335e23f` |
| 194DrawingSheetExport | 107 | `acd560ae5bf8ad17` |

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
