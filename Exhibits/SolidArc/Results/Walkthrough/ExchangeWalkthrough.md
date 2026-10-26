# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 900 · 211 frames · 26.9081 s · slowest frame 533.672 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/ExchangeWalkthrough.txt
- Expectations: 36 / 36 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 13 | `bodies == 1` | 1 | 🟢 |
| 14 | `volume ~ 10.2227737` | 10.22277375 | 🟢 |
| 19 | `exchange-export contains Exchange.step` | _ClaudeScratch/SolidArcWorkspace/Exchange.step | 🟢 |
| 20 | `exchange-export > 8000` | _ClaudeScratch/SolidArcWorkspace/Exchange.step | 🟢 |
| 21 | `exchange-export-bodies == 1` | Block1 | 🟢 |
| 22 | `exchange-export-bodies == Block1` | Block1 | 🟢 |
| 23 | `exchange-export-faces == 11` | 11 | 🟢 |
| 24 | `exchange-exports == 1` | 1 | 🟢 |
| 25 | `status contains Exported 1 body(ies) as STEP` | Exported 1 body(ies) as STEP → _ClaudeScratch/SolidArcWorkspace/Exchange.step · 11 f... | 🟢 |
| 45 | `exchange-export contains Exchange.stl` | _ClaudeScratch/SolidArcWorkspace/Exchange.stl | 🟢 |
| 46 | `exchange-export-facets > 1000` | 1906 | 🟢 |
| 47 | `exchange-exports == 2` | 2 | 🟢 |
| 48 | `status contains as STL` | Exported 1 body(ies) as STL → _ClaudeScratch/SolidArcWorkspace/Exchange.stl · 1906 f... | 🟢 |
| 55 | `bodies == 0` | 0 | 🟢 |
| 72 | `exchange-import contains Exchange.step` | _ClaudeScratch/SolidArcWorkspace/Exchange.step | 🟢 |
| 73 | `exchange-import == 1` | _ClaudeScratch/SolidArcWorkspace/Exchange.step | 🟢 |
| 74 | `exchange-import-faces == 11` | 11 | 🟢 |
| 75 | `exchange-imports == 1` | 1 | 🟢 |
| 76 | `bodies == 1` | 1 | 🟢 |
| 77 | `steps == 1` | 1 | 🟢 |
| 78 | `undo == 1` | 1 | 🟢 |
| 79 | `sound == 1` | 1 | 🟢 |
| 80 | `volume ~ 10.2227737` | 10.22277375 | 🟢 |
| 81 | `body-captions contains Exchange` | Exchange | 🟢 |
| 82 | `selected-bodies == Import#1` | Import#1 | 🟢 |
| 91 | `selected-edges contains Import#1.Face(` | Edge(Import#1.Face(1)/Import#1.Face(5)) | 🟢 |
| 94 | `bevel-active == 1` | 1 | 🟢 |
| 102 | `bevel-active == 0` | 0 | 🟢 |
| 103 | `steps == 2` | 2 | 🟢 |
| 104 | `sound == 1` | 1 | 🟢 |
| 105 | `volume ~ 10.2141897` | 10.21418967 | 🟢 |
| 111 | `focused-step == Import#1` | Import#1 | 🟢 |
| 127 | `exchange-import contains neither .step / .stp nor a VehicleCurves .toml` | Import: Experimental/VehicleSurfacing/Samples/SyntheticBonnet.obj is neither .step / .s... | 🟢 |
| 128 | `exchange-imports == 1` | 1 | 🟢 |
| 129 | `steps == 2` | 2 | 🟢 |
| 130 | `status contains Refused` | Refused: Import: Experimental/VehicleSurfacing/Samples/SyntheticBonnet.obj is neither .... | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 195ExchangeExportStep | 19 | `d25e408ec1d2e3b0` |
| 196ExchangeSaveAsStl | 68 | `673d2452dbba9bd9` |
| 197ExchangeImported | 123 | `6d972c4c5fa98099` |
| 198ExchangeFilletImported | 165 | `a12ac863d1e752eb` |
| 199ExchangeImportStep | 174 | `0427a66e5f5ea244` |

## Journal

- Exported 1 body(ies) as STEP → _ClaudeScratch/SolidArcWorkspace/Exchange.step · 11 face(s) · 12219 bytes · 2 ms
- Exported 1 body(ies) as STL → _ClaudeScratch/SolidArcWorkspace/Exchange.stl · 1906 facet(s) · 95384 bytes · 11 ms
- New document (previous recovery copy kept)
- Import Exchange · 1 solid(s) · 100 ms
- Framed the document (Space)
- Imported _ClaudeScratch/SolidArcWorkspace/Exchange.step · 1 solid(s) · 11 face(s) · file unit mm · 0 loop(s) reoriented · 0 tolerant edge(s) · 148 ms
- Selection cleared
- Selected Edge(Import#1.Face(1)|Import#1.Face(5))
- Fillet (B): drag the handles (corner handles make it variable) or drag / move anywhere · D fillet · C chamfer · V variable size · click an edge to add / remove it · Enter / right-click applies · Esc aborts
- Bevel size 0.1 m (pinned)
- Fillet 0.1 on 1 edge(s) · 141 ms
- Autosaved _ClaudeScratch/SolidArcWorkspace/ExchangeRecovery.solidarc
- Refused: Import: Experimental/VehicleSurfacing/Samples/SyntheticBonnet.obj is neither .step / .stp nor a VehicleCurves .toml
