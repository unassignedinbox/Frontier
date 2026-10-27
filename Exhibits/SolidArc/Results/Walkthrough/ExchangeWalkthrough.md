# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 900 · 212 frames · 29.0506 s · slowest frame 617.408 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/ExchangeWalkthrough.txt
- Expectations: 36 / 36 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 16 | `bodies == 1` | 1 | 🟢 |
| 17 | `volume ~ 10.2227737` | 10.22277375 | 🟢 |
| 22 | `exchange-export contains Exchange.step` | _ClaudeScratch/SolidArcWorkspace/Exchange.step | 🟢 |
| 23 | `exchange-export > 8000` | _ClaudeScratch/SolidArcWorkspace/Exchange.step | 🟢 |
| 24 | `exchange-export-bodies == 1` | Block1 | 🟢 |
| 25 | `exchange-export-bodies == Block1` | Block1 | 🟢 |
| 26 | `exchange-export-faces == 11` | 11 | 🟢 |
| 27 | `exchange-exports == 1` | 1 | 🟢 |
| 28 | `status contains Exported 1 body(ies) as STEP` | Exported 1 body(ies) as STEP → _ClaudeScratch/SolidArcWorkspace/Exchange.step · 11 f... | 🟢 |
| 48 | `exchange-export contains Exchange.stl` | _ClaudeScratch/SolidArcWorkspace/Exchange.stl | 🟢 |
| 49 | `exchange-export-facets > 1000` | 1906 | 🟢 |
| 50 | `exchange-exports == 2` | 2 | 🟢 |
| 51 | `status contains as STL` | Exported 1 body(ies) as STL → _ClaudeScratch/SolidArcWorkspace/Exchange.stl · 1906 f... | 🟢 |
| 58 | `bodies == 0` | 0 | 🟢 |
| 75 | `exchange-import contains Exchange.step` | _ClaudeScratch/SolidArcWorkspace/Exchange.step | 🟢 |
| 76 | `exchange-import == 1` | _ClaudeScratch/SolidArcWorkspace/Exchange.step | 🟢 |
| 77 | `exchange-import-faces == 11` | 11 | 🟢 |
| 78 | `exchange-imports == 1` | 1 | 🟢 |
| 79 | `bodies == 1` | 1 | 🟢 |
| 80 | `steps == 1` | 1 | 🟢 |
| 81 | `undo == 1` | 1 | 🟢 |
| 82 | `sound == 1` | 1 | 🟢 |
| 83 | `volume ~ 10.2227737` | 10.22277375 | 🟢 |
| 84 | `body-captions contains Exchange` | Exchange | 🟢 |
| 85 | `selected-bodies == Import#1` | Import#1 | 🟢 |
| 94 | `selected-edges contains Import#1.Face(` | Edge(Import#1.Face(1)/Import#1.Face(5)) | 🟢 |
| 97 | `bevel-active == 1` | 1 | 🟢 |
| 105 | `bevel-active == 0` | 0 | 🟢 |
| 106 | `steps == 2` | 2 | 🟢 |
| 107 | `sound == 1` | 1 | 🟢 |
| 108 | `volume ~ 10.2141897` | 10.21418967 | 🟢 |
| 114 | `focused-step == Import#1` | Import#1 | 🟢 |
| 130 | `exchange-import contains neither .step / .stp nor a VehicleCurves .toml` | Import: Experimental/VehicleSurfacing/Samples/SyntheticBonnet.obj is neither .step / .s... | 🟢 |
| 131 | `exchange-imports == 1` | 1 | 🟢 |
| 132 | `steps == 2` | 2 | 🟢 |
| 133 | `status contains Refused` | Refused: Import: Experimental/VehicleSurfacing/Samples/SyntheticBonnet.obj is neither .... | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 195ExchangeExportStep | 20 | `ab0a847263d8ae5a` |
| 196ExchangeSaveAsStl | 69 | `669900c534b9b71e` |
| 197ExchangeImported | 124 | `b54023872e01d7ca` |
| 198ExchangeFilletImported | 166 | `5e9c9fbf1276edae` |
| 199ExchangeImportStep | 175 | `e216ce7c2d508194` |

## Journal

- Exported 1 body(ies) as STEP → _ClaudeScratch/SolidArcWorkspace/Exchange.step · 11 face(s) · 12219 bytes · 2 ms
- Exported 1 body(ies) as STL → _ClaudeScratch/SolidArcWorkspace/Exchange.stl · 1906 facet(s) · 95384 bytes · 10 ms
- New document (previous recovery copy kept)
- Import Exchange · 1 solid(s) · 2 ms
- Framed the document (Space)
- Imported _ClaudeScratch/SolidArcWorkspace/Exchange.step · 1 solid(s) · 11 face(s) · file unit mm · 0 loop(s) reoriented · 0 tolerant edge(s) · 5 ms
- Selection cleared
- Selected Edge(Import#1.Face(1)|Import#1.Face(5))
- Fillet (B): drag anywhere or the ring handle to size (it stops at the largest that fits) · drag a corner diamond for a variable size · type in the card · D fillet · C chamfer · V taper · Alt+click an edge adds its ring · Enter / right-click applies · Esc aborts
- Bevel size 0.1 m (pinned)
- Fillet 0.1 on 1 edge(s) · 3 ms
- Autosaved _ClaudeScratch/SolidArcWorkspace/ExchangeRecovery.solidarc
- Refused: Import: Experimental/VehicleSurfacing/Samples/SyntheticBonnet.obj is neither .step / .stp nor a VehicleCurves .toml
