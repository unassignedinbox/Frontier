# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 810 · 264 frames · 18.213 s · slowest frame 343.368 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/VehicleSurfacingWalkthrough.txt
- Expectations: 53 / 53 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 16 | `steps == 0` | 0 | 🟢 |
| 21 | `steps == 11` | 11 | 🟢 |
| 22 | `refused == 0` | 0 | 🟢 |
| 23 | `undo == 1` | 1 | 🟢 |
| 24 | `curves == 11` | Section1,Between4,Between5,Section2,Section3,Section4,Section5,Section6,Between1,Betwee... | 🟢 |
| 25 | `curves contains Section3` | Section1,Between4,Between5,Section2,Section3,Section4,Section5,Section6,Between1,Betwee... | 🟢 |
| 26 | `curves contains Between5` | Section1,Between4,Between5,Section2,Section3,Section4,Section5,Section6,Between1,Betwee... | 🟢 |
| 27 | `curve-fit-worst < 0.0002` | 5.582644671e-05 | 🟢 |
| 28 | `sheet == 0` | 0 | 🟢 |
| 29 | `status contains Imported 11 curve(s)` | Imported 11 curve(s) from Experimental/VehicleSurfacing/Samples/SyntheticBonnet.toml ·... | 🟢 |
| 41 | `pick-mode == 1` | 1 | 🟢 |
| 45 | `selected-curves == Section1` | Section1 | 🟢 |
| 46 | `focused-caption == Section1` | Section1 | 🟢 |
| 49 | `status contains at least 2` | Refused: loft needs at least 2 curves (select them in section order) | 🟢 |
| 50 | `steps == 11` | 11 | 🟢 |
| 59 | `selected-curves == Section1,Section2,Section3,Section4,Section5,Section6` | Section1,Section2,Section3,Section4,Section5,Section6 | 🟢 |
| 62 | `selected-curves == Section1,Section2,Section4,Section5,Section6` | Section1,Section2,Section4,Section5,Section6 | 🟢 |
| 65 | `selected-curves == Section1,Section2,Section4,Section5,Section6,Section3` | Section1,Section2,Section4,Section5,Section6,Section3 | 🟢 |
| 75 | `selected-curves == Section1,Section2,Section3,Section4,Section5,Section6` | Section1,Section2,Section3,Section4,Section5,Section6 | 🟢 |
| 81 | `steps == 12` | 12 | 🟢 |
| 82 | `refused == 0` | 0 | 🟢 |
| 83 | `sheet == 1` | 1 | 🟢 |
| 84 | `faces == 1` | 1 | 🟢 |
| 85 | `edges == 4` | 4 | 🟢 |
| 86 | `sound == 1` | 1 | 🟢 |
| 87 | `volume == 0` | 0 | 🟢 |
| 88 | `undo == 2` | 2 | 🟢 |
| 89 | `selected-curves == 0` |  | 🟢 |
| 90 | `status contains Lofted 6 curve(s)` | Lofted 6 curve(s) into a panel (sound) · 1 panel(s) · open sections stay a sheet · 1 ms | 🟢 |
| 99 | `pole-drag-active == 1` | 1 | 🟢 |
| 104 | `pole-drag-active == 0` | 0 | 🟢 |
| 105 | `pole-drags == 0` | 0 | 🟢 |
| 106 | `pole-edits == 0` | 0 | 🟢 |
| 107 | `undo == 2` | 2 | 🟢 |
| 120 | `pole-drag-active == 1` | 1 | 🟢 |
| 121 | `undo == 2` | 2 | 🟢 |
| 125 | `pole-drag-active == 0` | 0 | 🟢 |
| 126 | `pole-drags == 1` | 1 | 🟢 |
| 127 | `pole-edits == 1` | 1 | 🟢 |
| 128 | `undo == 3` | 3 | 🟢 |
| 129 | `refused == 0` | 0 | 🟢 |
| 130 | `sheet == 1` | 1 | 🟢 |
| 131 | `sound == 1` | 1 | 🟢 |
| 132 | `status contains Moved pole 2 of Section3` | Moved pole 2 of Section3 by 33.10 mm · 2 ms | 🟢 |
| 135 | `selected-curves == Section3` | Section3 | 🟢 |
| 139 | `pole-edits == 0` | 0 | 🟢 |
| 140 | `sheet == 1` | 1 | 🟢 |
| 141 | `redo == 1` | 1 | 🟢 |
| 144 | `pole-edits == 1` | 1 | 🟢 |
| 145 | `redo == 0` | 0 | 🟢 |
| 152 | `selected-faces contains Loft#12.Side` | Loft#12.Side(Surface) | 🟢 |
| 168 | `file:_ClaudeScratch/SolidArcWorkspace/Vehicle.solidarc == 1` | 1 | 🟢 |
| 169 | `dirty == 0` | 0 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 50CurvesImported | 42 | `07d07a13a95bc176` |
| 51ControlPoints | 49 | `818a5b72e1fb6582` |
| 52SectionsSelected | 146 | `5efb57088139ce97` |
| 53BonnetLoft | 154 | `26366ffd62fd5983` |
| 54PoleDragLive | 191 | `5ed5154f20dfe292` |
| 55PoleMoved | 205 | `2df35a41e69fad06` |
| 56SheetUnderside | 240 | `cdc424b093f952ae` |

## Journal

- New document (previous recovery copy kept)
- Import 11 curve(s) · 1 ms
- Framed the document (Space)
- Imported 11 curve(s) from Experimental/VehicleSurfacing/Samples/SyntheticBonnet.toml · 44 poles · worst fit 0.056 mm
- View Isometric
- Framed the document (Space)
- Selected curve Section1 · 4 poles · fit 0.045 mm · 1 selected
- Autosaved _ClaudeScratch/SolidArcWorkspace/VehicleRecovery.solidarc
- Refused: loft needs at least 2 curves (select them in section order)
- Selected curve Section2 · 4 poles · fit 0.049 mm · 2 selected
- Selected curve Section3 · 4 poles · fit 0.042 mm · 3 selected
- Selected curve Section4 · 4 poles · fit 0.049 mm · 4 selected
- Selected curve Section5 · 4 poles · fit 0.045 mm · 5 selected
- Selected curve Section6 · 4 poles · fit 0.046 mm · 6 selected
- Removed curve Section3 · 4 poles · fit 0.042 mm · 5 selected
- Selected curve Section3 · 4 poles · fit 0.042 mm · 6 selected
- Removed curve Section3 · 4 poles · fit 0.042 mm · 5 selected
- Removed curve Section4 · 4 poles · fit 0.049 mm · 4 selected
- Removed curve Section5 · 4 poles · fit 0.045 mm · 3 selected
- Removed curve Section6 · 4 poles · fit 0.046 mm · 2 selected
- Selected curve Section3 · 4 poles · fit 0.042 mm · 3 selected
- Selected curve Section4 · 4 poles · fit 0.049 mm · 4 selected
- Selected curve Section5 · 4 poles · fit 0.045 mm · 5 selected
- Selected curve Section6 · 4 poles · fit 0.046 mm · 6 selected
- Loft1 · 6 sections · 1 ms
- Lofted 6 curve(s) into a panel (sound) · 1 panel(s) · open sections stay a sheet · 1 ms
- Pole drag dropped
- Move pole 2 of Section3 · 2 ms
- Moved pole 2 of Section3 by 33.10 mm · 2 ms
- Selected curve Section3 · 4 poles · fit 0.042 mm · 1 selected
- Undo
- Redo
- Selected Loft#12.Side(Surface)
- Selection cleared
- Autosaved _ClaudeScratch/SolidArcWorkspace/VehicleRecovery.solidarc
- View Isometric
- Saved _ClaudeScratch/SolidArcWorkspace/Vehicle.solidarc
