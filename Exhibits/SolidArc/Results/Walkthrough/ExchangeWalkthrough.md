# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0
- Frame: 1600 x 900 · 241 frames · 19.5578 s · slowest frame 343.412 ms
- Script: /home/user/Frontier/Exhibits/SolidArc/Code/Scripts/ExchangeWalkthrough.txt
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
| 50 | `exchange-export contains Exchange.stl` | _ClaudeScratch/SolidArcWorkspace/Exchange.stl | 🟢 |
| 51 | `exchange-export-facets > 1000` | 1906 | 🟢 |
| 52 | `exchange-exports == 2` | 2 | 🟢 |
| 53 | `status contains as STL` | Exported 1 body(ies) as STL → _ClaudeScratch/SolidArcWorkspace/Exchange.stl · 1906 f... | 🟢 |
| 60 | `bodies == 0` | 0 | 🟢 |
| 82 | `exchange-import contains Exchange.step` | _ClaudeScratch/SolidArcWorkspace/Exchange.step | 🟢 |
| 83 | `exchange-import == 1` | _ClaudeScratch/SolidArcWorkspace/Exchange.step | 🟢 |
| 84 | `exchange-import-faces == 11` | 11 | 🟢 |
| 85 | `exchange-imports == 1` | 1 | 🟢 |
| 86 | `bodies == 1` | 1 | 🟢 |
| 87 | `steps == 1` | 1 | 🟢 |
| 88 | `undo == 1` | 1 | 🟢 |
| 89 | `sound == 1` | 1 | 🟢 |
| 90 | `volume ~ 10.2227737` | 10.22277375 | 🟢 |
| 91 | `body-captions contains Exchange` | Exchange | 🟢 |
| 92 | `selected-bodies == Import#1` | Import#1 | 🟢 |
| 101 | `selected-edges contains Import#1.Face(` | Edge(Import#1.Face(1)/Import#1.Face(5)) | 🟢 |
| 104 | `bevel-active == 1` | 1 | 🟢 |
| 112 | `bevel-active == 0` | 0 | 🟢 |
| 113 | `steps == 2` | 2 | 🟢 |
| 114 | `sound == 1` | 1 | 🟢 |
| 115 | `volume ~ 10.2141897` | 10.21418967 | 🟢 |
| 121 | `focused-step == Import#1` | Import#1 | 🟢 |
| 137 | `exchange-import contains neither .step / .stp nor a VehicleCurves .toml` | Import: Experimental/VehicleSurfacing/Samples/SyntheticBonnet.obj is neither .step / .s... | 🟢 |
| 138 | `exchange-imports == 1` | 1 | 🟢 |
| 139 | `steps == 2` | 2 | 🟢 |
| 140 | `status contains Refused` | Refused: Import: Experimental/VehicleSurfacing/Samples/SyntheticBonnet.obj is neither .... | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 195ExchangeExportStep | 19 | `a3fb40416c73f06f` |
| 196ExchangeSaveAsStl | 83 | `ba33354676b068c5` |
| 197ExchangeImported | 153 | `46003a4a9301452d` |
| 198ExchangeFilletImported | 195 | `b1f23c8dad5db7b0` |
| 199ExchangeImportStep | 204 | `58d05a5e2d1fe736` |

## Journal

- Exported 1 body(ies) as STEP → _ClaudeScratch/SolidArcWorkspace/Exchange.step · 11 face(s) · 12219 bytes · 2 ms
- Exported 1 body(ies) as STL → _ClaudeScratch/SolidArcWorkspace/Exchange.stl · 1906 facet(s) · 95384 bytes · 8 ms
- New document (previous recovery copy kept)
- Import Exchange · 1 solid(s) · 73 ms
- Framed the document (Space)
- Imported _ClaudeScratch/SolidArcWorkspace/Exchange.step · 1 solid(s) · 11 face(s) · file unit mm · 0 loop(s) reoriented · 0 tolerant edge(s) · 111 ms
- Selection cleared
- Selected Edge(Import#1.Face(1)|Import#1.Face(5))
- Fillet (B): move horizontally · D fillet · C chamfer · V variable size · F flip its anchor · click edits the edges · Enter / right-click applies · Esc aborts
- Bevel size 0.1 m (pinned)
- Fillet 0.1 on 1 edge(s) · 124 ms
- Autosaved _ClaudeScratch/SolidArcWorkspace/ExchangeRecovery.solidarc
- Refused: Import: Experimental/VehicleSurfacing/Samples/SyntheticBonnet.obj is neither .step / .stp nor a VehicleCurves .toml
