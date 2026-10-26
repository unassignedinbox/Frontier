# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 900 · 386 frames · 46.4581 s · slowest frame 2466.27 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/SurfaceEvaluationWalkthrough.txt
- Expectations: 28 / 28 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 20 | `steps == 11` | 11 | 🟢 |
| 37 | `pole-edits == 1` | 1 | 🟢 |
| 38 | `status contains Moved pole 2 of Section3` | Moved pole 2 of Section3 by 25.86 mm · 3 ms | 🟢 |
| 45 | `loft-continuity == 0` | 0 | 🟢 |
| 48 | `panels == 1` | 1 | 🟢 |
| 54 | `selected-curves == Section3,Section4,Section5,Section6` | Section3,Section4,Section5,Section6 | 🟢 |
| 57 | `steps == 13` | 13 | 🟢 |
| 58 | `refused == 0` | 0 | 🟢 |
| 59 | `panels == 2` | 2 | 🟢 |
| 60 | `sound == 1` | 1 | 🟢 |
| 67 | `zebra == 1` | 1 | 🟢 |
| 68 | `triangles > 1500` | 5818 | 🟢 |
| 69 | `status contains Zebra on` | Zebra on · 64 stripes · 5818 triangles | 🟢 |
| 79 | `panels == 1` | 1 | 🟢 |
| 87 | `loft-continuity == 2` | 2 | 🟢 |
| 90 | `steps == 13` | 13 | 🟢 |
| 91 | `refused == 0` | 0 | 🟢 |
| 92 | `panels == 2` | 2 | 🟢 |
| 93 | `sound == 1` | 1 | 🟢 |
| 94 | `status contains · G2` | Lofted 4 curve(s) into a panel · G2 (sound) · 2 panel(s) · open sections stay a shee... | 🟢 |
| 97 | `zebra == 1` | 1 | 🟢 |
| 109 | `zebra == 0` | 0 | 🟢 |
| 114 | `deviation-points == 286` | 286 | 🟢 |
| 115 | `deviation-worst > 0.002` | 0.008634900031 | 🟢 |
| 140 | `selected-curves == Section1,Section2,Section3,Section4,Section5,Section6` | Section1,Section2,Section3,Section4,Section5,Section6 | 🟢 |
| 146 | `panels == 2` | 2 | 🟢 |
| 147 | `sound == 1` | 1 | 🟢 |
| 154 | `zebra == 1` | 1 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 70ZebraG0 | 155 | `9408de4b5eb6dea7` |
| 71ZebraG2 | 216 | `752430af874c78b8` |
| 72DeviationMap | 262 | `f06555e42b1a8ba2` |
| 73ZebraMirrored | 386 | `0c50a1ceac3e0e46` |

## Journal

- New document (previous recovery copy kept)
- Import 11 curve(s) · 2 ms
- Framed the document (Space)
- Imported 11 curve(s) from Experimental/VehicleSurfacing/Samples/SyntheticBonnet.toml · 44 poles · worst fit 0.056 mm
- View Isometric
- Framed the document (Space)
- Move pole 2 of Section3 · 3 ms
- Moved pole 2 of Section3 by 25.86 mm · 3 ms
- Selected curve Section1 · 4 poles · fit 0.045 mm · 1 selected
- Selected curve Section2 · 4 poles · fit 0.049 mm · 2 selected
- Selected curve Section3 · 4 poles · fit 0.042 mm · 3 selected
- Loft1 · 3 sections · 1 ms
- Lofted 3 curve(s) into a panel (sound) · 1 panel(s) · open sections stay a sheet · 1 ms
- Selected curve Section3 · 4 poles · fit 0.042 mm · 1 selected
- Selected curve Section4 · 4 poles · fit 0.049 mm · 2 selected
- Selected curve Section5 · 4 poles · fit 0.045 mm · 3 selected
- Selected curve Section6 · 4 poles · fit 0.046 mm · 4 selected
- Loft2 · 4 sections · 1 ms
- Lofted 4 curve(s) into a panel (sound) · 2 panel(s) · open sections stay a sheet · 1 ms
- Selection cleared
- Zebra on · 64 stripes · 5818 triangles
- View Top
- Autosaved _ClaudeScratch/SolidArcWorkspace/EvaluationRecovery.solidarc
- Undo
- Selected curve Section3 · 4 poles · fit 0.042 mm · 1 selected
- Selected curve Section4 · 4 poles · fit 0.049 mm · 2 selected
- Selected curve Section5 · 4 poles · fit 0.045 mm · 3 selected
- Selected curve Section6 · 4 poles · fit 0.046 mm · 4 selected
- Loft2 · 4 sections · G2 · 2 ms
- Lofted 4 curve(s) into a panel · G2 (sound) · 2 panel(s) · open sections stay a sheet · 2 ms
- Selection cleared
- View Top
- Zebra off
- Autosaved _ClaudeScratch/SolidArcWorkspace/EvaluationRecovery.solidarc
- View Isometric
- New document (previous recovery copy kept)
- Import 11 curve(s) · 2 ms
- Framed the document (Space)
- Imported 11 curve(s) from Experimental/VehicleSurfacing/Samples/SyntheticBonnet.toml · 44 poles · worst fit 0.056 mm
- Selected curve Section1 · 4 poles · fit 0.045 mm · 1 selected
- Selected curve Section2 · 4 poles · fit 0.049 mm · 2 selected
- Selected curve Section3 · 4 poles · fit 0.042 mm · 3 selected
- Selected curve Section4 · 4 poles · fit 0.049 mm · 4 selected
- Selected curve Section5 · 4 poles · fit 0.045 mm · 5 selected
- Selected curve Section6 · 4 poles · fit 0.046 mm · 6 selected
- Loft1 · 6 sections · 1 ms
- Lofted 6 curve(s) into a panel (sound) · 1 panel(s) · open sections stay a sheet · 1 ms
- Mirror1 · plane x = 0 · 7 ms
- Mirrored through x = 0 → 2 panel(s), sound · 7 ms
- Selection cleared
- Zebra on · 64 stripes · 6068 triangles
- View Top
- Framed the document (Space)
- Autosaved _ClaudeScratch/SolidArcWorkspace/EvaluationRecovery.solidarc
