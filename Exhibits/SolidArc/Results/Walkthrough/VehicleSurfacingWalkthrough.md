# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 900 · 265 frames · 26.9012 s · slowest frame 512.754 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/VehicleSurfacingWalkthrough.txt
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
| 38 | `pick-mode == 1` | 1 | 🟢 |
| 42 | `selected-curves == Section1` | Section1 | 🟢 |
| 43 | `focused-caption == Section1` | Section1 | 🟢 |
| 46 | `status contains at least 2` | Refused: loft needs at least 2 curves (select them in section order) | 🟢 |
| 47 | `steps == 11` | 11 | 🟢 |
| 56 | `selected-curves == Section1,Section2,Section3,Section4,Section5,Section6` | Section1,Section2,Section3,Section4,Section5,Section6 | 🟢 |
| 59 | `selected-curves == Section1,Section2,Section4,Section5,Section6` | Section1,Section2,Section4,Section5,Section6 | 🟢 |
| 62 | `selected-curves == Section1,Section2,Section4,Section5,Section6,Section3` | Section1,Section2,Section4,Section5,Section6,Section3 | 🟢 |
| 72 | `selected-curves == Section1,Section2,Section3,Section4,Section5,Section6` | Section1,Section2,Section3,Section4,Section5,Section6 | 🟢 |
| 78 | `steps == 12` | 12 | 🟢 |
| 79 | `refused == 0` | 0 | 🟢 |
| 80 | `sheet == 1` | 1 | 🟢 |
| 81 | `faces == 1` | 1 | 🟢 |
| 82 | `edges == 4` | 4 | 🟢 |
| 83 | `sound == 1` | 1 | 🟢 |
| 84 | `volume == 0` | 0 | 🟢 |
| 85 | `undo == 2` | 2 | 🟢 |
| 86 | `selected-curves == 0` |  | 🟢 |
| 87 | `status contains Lofted 6 curve(s)` | Lofted 6 curve(s) into a panel (sound) · 1 panel(s) · open sections stay a sheet · 1 ms | 🟢 |
| 96 | `pole-drag-active == 1` | 1 | 🟢 |
| 101 | `pole-drag-active == 0` | 0 | 🟢 |
| 102 | `pole-drags == 0` | 0 | 🟢 |
| 103 | `pole-edits == 0` | 0 | 🟢 |
| 104 | `undo == 2` | 2 | 🟢 |
| 117 | `pole-drag-active == 1` | 1 | 🟢 |
| 118 | `undo == 2` | 2 | 🟢 |
| 122 | `pole-drag-active == 0` | 0 | 🟢 |
| 123 | `pole-drags == 1` | 1 | 🟢 |
| 124 | `pole-edits == 1` | 1 | 🟢 |
| 125 | `undo == 3` | 3 | 🟢 |
| 126 | `refused == 0` | 0 | 🟢 |
| 127 | `sheet == 1` | 1 | 🟢 |
| 128 | `sound == 1` | 1 | 🟢 |
| 129 | `status contains Moved pole 2 of Section3` | Moved pole 2 of Section3 by 40.32 mm · 3 ms | 🟢 |
| 132 | `selected-curves == Section3` | Section3 | 🟢 |
| 136 | `pole-edits == 0` | 0 | 🟢 |
| 137 | `sheet == 1` | 1 | 🟢 |
| 138 | `redo == 1` | 1 | 🟢 |
| 141 | `pole-edits == 1` | 1 | 🟢 |
| 142 | `redo == 0` | 0 | 🟢 |
| 149 | `selected-faces contains Loft#12.Side` | Loft#12.Side(Surface) | 🟢 |
| 165 | `file:_ClaudeScratch/SolidArcWorkspace/Vehicle.solidarc == 1` | 1 | 🟢 |
| 166 | `dirty == 0` | 0 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 50CurvesImported | 43 | `169e5ef7902d3c31` |
| 51ControlPoints | 50 | `1d582ea9dc87a04d` |
| 52SectionsSelected | 147 | `b33d94081968ce2d` |
| 53BonnetLoft | 155 | `bef0f1bf24f8ad4a` |
| 54PoleDragLive | 192 | `a36e00eb88d1d7c8` |
| 55PoleMoved | 206 | `1718f32435025f5a` |
| 56SheetUnderside | 241 | `ba17071402288217` |

## Journal

- New document (previous recovery copy kept)
- Import 11 curve(s) · 2 ms
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
- Move pole 2 of Section3 · 3 ms
- Moved pole 2 of Section3 by 40.32 mm · 3 ms
- Selected curve Section3 · 4 poles · fit 0.042 mm · 1 selected
- Undo
- Redo
- Selected Loft#12.Side(Surface)
- Selection cleared
- Autosaved _ClaudeScratch/SolidArcWorkspace/VehicleRecovery.solidarc
- View Isometric
- Saved _ClaudeScratch/SolidArcWorkspace/Vehicle.solidarc
