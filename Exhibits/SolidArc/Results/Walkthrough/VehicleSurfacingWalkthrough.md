# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0
- Frame: 1600 x 900 · 251 frames · 23.1112 s · slowest frame 468.984 ms
- Script: /home/user/Frontier/Exhibits/SolidArc/Code/Scripts/VehicleSurfacingWalkthrough.txt
- Expectations: 53 / 53 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 13 | `steps == 0` | 0 | 🟢 |
| 18 | `steps == 11` | 11 | 🟢 |
| 19 | `refused == 0` | 0 | 🟢 |
| 20 | `undo == 1` | 1 | 🟢 |
| 21 | `curves == 11` | Section1,Between4,Between5,Section2,Section3,Section4,Section5,Section6,Between1,Betwee... | 🟢 |
| 22 | `curves contains Section3` | Section1,Between4,Between5,Section2,Section3,Section4,Section5,Section6,Between1,Betwee... | 🟢 |
| 23 | `curves contains Between5` | Section1,Between4,Between5,Section2,Section3,Section4,Section5,Section6,Between1,Betwee... | 🟢 |
| 24 | `curve-fit-worst < 0.0002` | 5.582644671e-05 | 🟢 |
| 25 | `sheet == 0` | 0 | 🟢 |
| 26 | `status contains Imported 11 curve(s)` | Imported 11 curve(s) from Experimental/VehicleSurfacing/Samples/SyntheticBonnet.toml ·... | 🟢 |
| 36 | `pick-mode == 1` | 1 | 🟢 |
| 40 | `selected-curves == Section1` | Section1 | 🟢 |
| 41 | `focused-caption == Section1` | Section1 | 🟢 |
| 44 | `status contains at least 2` | Refused: loft needs at least 2 curves (select them in section order) | 🟢 |
| 45 | `steps == 11` | 11 | 🟢 |
| 54 | `selected-curves == Section1,Section2,Section3,Section4,Section5,Section6` | Section1,Section2,Section3,Section4,Section5,Section6 | 🟢 |
| 57 | `selected-curves == Section1,Section2,Section4,Section5,Section6` | Section1,Section2,Section4,Section5,Section6 | 🟢 |
| 60 | `selected-curves == Section1,Section2,Section4,Section5,Section6,Section3` | Section1,Section2,Section4,Section5,Section6,Section3 | 🟢 |
| 70 | `selected-curves == Section1,Section2,Section3,Section4,Section5,Section6` | Section1,Section2,Section3,Section4,Section5,Section6 | 🟢 |
| 76 | `steps == 12` | 12 | 🟢 |
| 77 | `refused == 0` | 0 | 🟢 |
| 78 | `sheet == 1` | 1 | 🟢 |
| 79 | `faces == 1` | 1 | 🟢 |
| 80 | `edges == 4` | 4 | 🟢 |
| 81 | `sound == 1` | 1 | 🟢 |
| 82 | `volume == 0` | 0 | 🟢 |
| 83 | `undo == 2` | 2 | 🟢 |
| 84 | `selected-curves == 0` |  | 🟢 |
| 85 | `status contains Lofted 6 curve(s)` | Lofted 6 curve(s) → panel (sound) · 1 panel(s) · 1 ms | 🟢 |
| 94 | `pole-drag-active == 1` | 1 | 🟢 |
| 99 | `pole-drag-active == 0` | 0 | 🟢 |
| 100 | `pole-drags == 0` | 0 | 🟢 |
| 101 | `pole-edits == 0` | 0 | 🟢 |
| 102 | `undo == 2` | 2 | 🟢 |
| 115 | `pole-drag-active == 1` | 1 | 🟢 |
| 116 | `undo == 2` | 2 | 🟢 |
| 120 | `pole-drag-active == 0` | 0 | 🟢 |
| 121 | `pole-drags == 1` | 1 | 🟢 |
| 122 | `pole-edits == 1` | 1 | 🟢 |
| 123 | `undo == 3` | 3 | 🟢 |
| 124 | `refused == 0` | 0 | 🟢 |
| 125 | `sheet == 1` | 1 | 🟢 |
| 126 | `sound == 1` | 1 | 🟢 |
| 127 | `status contains Moved pole 2 of Section3` | Moved pole 2 of Section3 by 32.64 mm · 3 ms | 🟢 |
| 130 | `selected-curves == Section3` | Section3 | 🟢 |
| 134 | `pole-edits == 0` | 0 | 🟢 |
| 135 | `sheet == 1` | 1 | 🟢 |
| 136 | `redo == 1` | 1 | 🟢 |
| 139 | `pole-edits == 1` | 1 | 🟢 |
| 140 | `redo == 0` | 0 | 🟢 |
| 147 | `selected-faces contains Loft#12.Side` | Loft#12.Side(Surface) | 🟢 |
| 161 | `file:_ClaudeScratch/SolidArcWorkspace/Vehicle.solidarc == 1` | 1 | 🟢 |
| 162 | `dirty == 0` | 0 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 50CurvesImported | 36 | `b53df43d528aa941` |
| 51ControlPoints | 43 | `754147dc36f87eed` |
| 52SectionsSelected | 140 | `a971b0c3e72b1cd6` |
| 53BonnetLoft | 148 | `2d0ee143df99cbcc` |
| 54PoleDragLive | 185 | `fad5860ddb91c89e` |
| 55PoleMoved | 199 | `9133438454239286` |
| 56SheetUnderside | 234 | `76ad6f3add456d81` |

## Journal

- New document (previous recovery copy kept)
- Import 11 curve(s) · 3 ms
- Framed the document (Space)
- Imported 11 curve(s) from Experimental/VehicleSurfacing/Samples/SyntheticBonnet.toml · 44 poles · worst fit 0.056 mm
- View Isometric
- Framed the document (Space)
- Selected curve Section1 · 4 poles · fit 0.045 mm · 1 selected
- Refused: loft needs at least 2 curves (select them in section order)
- Autosaved _ClaudeScratch/SolidArcWorkspace/VehicleRecovery.solidarc
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
- Lofted 6 curve(s) → panel (sound) · 1 panel(s) · 1 ms
- Pole drag dropped
- Move pole 2 of Section3 · 3 ms
- Moved pole 2 of Section3 by 32.64 mm · 3 ms
- Selected curve Section3 · 4 poles · fit 0.042 mm · 1 selected
- Undo
- Redo
- Selected Loft#12.Side(Surface)
- Selection cleared
- View Isometric
- Autosaved _ClaudeScratch/SolidArcWorkspace/VehicleRecovery.solidarc
- Saved _ClaudeScratch/SolidArcWorkspace/Vehicle.solidarc
