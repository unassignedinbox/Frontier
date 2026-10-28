# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 810 · 238 frames · 20.1074 s · slowest frame 340.467 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/ExchangeWalkthrough.txt
- Expectations: 36 / 36 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 16 | `bodies == 1` | 1 | 🟢 |
| 17 | `volume ~ 10.2227737` | 10.22277375 | 🟢 |
| 24 | `exchange-export contains Exchange.step` | _ClaudeScratch/SolidArcWorkspace/Exchange.step | 🟢 |
| 25 | `exchange-export > 8000` | _ClaudeScratch/SolidArcWorkspace/Exchange.step | 🟢 |
| 26 | `exchange-export-bodies == 1` | Block1 | 🟢 |
| 27 | `exchange-export-bodies == Block1` | Block1 | 🟢 |
| 28 | `exchange-export-faces == 11` | 11 | 🟢 |
| 29 | `exchange-exports == 1` | 1 | 🟢 |
| 30 | `status contains Exported 1 body(ies) as STEP` | Exported 1 body(ies) as STEP → _ClaudeScratch/SolidArcWorkspace/Exchange.step · 11 f... | 🟢 |
| 50 | `exchange-export contains Exchange.stl` | _ClaudeScratch/SolidArcWorkspace/Exchange.stl | 🟢 |
| 51 | `exchange-export-facets > 1000` | 1906 | 🟢 |
| 52 | `exchange-exports == 2` | 2 | 🟢 |
| 53 | `status contains as STL` | Exported 1 body(ies) as STL → _ClaudeScratch/SolidArcWorkspace/Exchange.stl · 1906 f... | 🟢 |
| 60 | `bodies == 0` | 0 | 🟢 |
| 77 | `exchange-import contains Exchange.step` | _ClaudeScratch/SolidArcWorkspace/Exchange.step | 🟢 |
| 78 | `exchange-import == 1` | _ClaudeScratch/SolidArcWorkspace/Exchange.step | 🟢 |
| 79 | `exchange-import-faces == 11` | 11 | 🟢 |
| 80 | `exchange-imports == 1` | 1 | 🟢 |
| 81 | `bodies == 1` | 1 | 🟢 |
| 82 | `steps == 1` | 1 | 🟢 |
| 83 | `undo == 1` | 1 | 🟢 |
| 84 | `sound == 1` | 1 | 🟢 |
| 85 | `volume ~ 10.2227737` | 10.22277375 | 🟢 |
| 86 | `body-captions contains Exchange` | Exchange | 🟢 |
| 87 | `selected-bodies == Import#1` | Import#1 | 🟢 |
| 96 | `selected-edges contains Import#1.Face(` | Edge(Import#1.Face(1)/Import#1.Face(5)) | 🟢 |
| 99 | `bevel-active == 1` | 1 | 🟢 |
| 107 | `bevel-active == 0` | 0 | 🟢 |
| 108 | `steps == 2` | 2 | 🟢 |
| 109 | `sound == 1` | 1 | 🟢 |
| 110 | `volume ~ 10.2141897` | 10.21418967 | 🟢 |
| 121 | `focused-step == Import#1` | Import#1 | 🟢 |
| 137 | `exchange-import contains neither .step / .stp nor a VehicleCurves .toml` | Import: Experimental/VehicleSurfacing/Samples/SyntheticBonnet.obj is neither .step / .s... | 🟢 |
| 138 | `exchange-imports == 1` | 1 | 🟢 |
| 139 | `steps == 2` | 2 | 🟢 |
| 140 | `status contains Refused` | Refused: Import: Experimental/VehicleSurfacing/Samples/SyntheticBonnet.obj is neither .... | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 195ExchangeExportStep | 27 | `d45f66b6d0850162` |
| 196ExchangeSaveAsStl | 76 | `38e2246695b2ca89` |
| 197ExchangeImported | 131 | `2dbd3688ae374578` |
| 198ExchangeFilletImported | 173 | `7fc12274f04a8dfc` |
| 199ExchangeImportStep | 203 | `03794f1dd99376af` |

## Journal

- Exported 1 body(ies) as STEP → _ClaudeScratch/SolidArcWorkspace/Exchange.step · 11 face(s) · 12219 bytes · 1 ms
- Exported 1 body(ies) as STL → _ClaudeScratch/SolidArcWorkspace/Exchange.stl · 1906 facet(s) · 95384 bytes · 8 ms
- New document (previous recovery copy kept)
- Import Exchange · 1 solid(s) · 1 ms
- Framed the document (Space)
- Imported _ClaudeScratch/SolidArcWorkspace/Exchange.step · 1 solid(s) · 11 face(s) · file unit mm · 0 loop(s) reoriented · 0 tolerant edge(s) · 3 ms
- Selection cleared
- Selected Edge(Import#1.Face(1)|Import#1.Face(5))
- Fillet (B): drag anywhere or the ring handle to size (it stops at the largest that fits) · drag a corner diamond for a variable size · type in the card · D fillet · C chamfer · V taper · Alt+click an edge loop, Ctrl+Alt+click a ring (Shift adds) · Enter / right-click applies · Esc aborts
- Bevel size 0.1 m (pinned)
- Fillet 0.1 on 1 edge(s) · 2 ms
- Autosaved _ClaudeScratch/SolidArcWorkspace/ExchangeRecovery.solidarc
- Refused: Import: Experimental/VehicleSurfacing/Samples/SyntheticBonnet.obj is neither .step / .stp nor a VehicleCurves .toml
