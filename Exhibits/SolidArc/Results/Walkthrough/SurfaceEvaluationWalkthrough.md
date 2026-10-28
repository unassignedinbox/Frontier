# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 810 · 386 frames · 33.5279 s · slowest frame 1634.27 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/SurfaceEvaluationWalkthrough.txt
- Expectations: 28 / 28 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 23 | `steps == 11` | 11 | 🟢 |
| 40 | `pole-edits == 1` | 1 | 🟢 |
| 41 | `status contains Moved pole 2 of Section3` | Moved pole 2 of Section3 by 21.27 mm · 1 ms | 🟢 |
| 48 | `loft-continuity == 0` | 0 | 🟢 |
| 51 | `panels == 1` | 1 | 🟢 |
| 57 | `selected-curves == Section3,Section4,Section5,Section6` | Section3,Section4,Section5,Section6 | 🟢 |
| 60 | `steps == 13` | 13 | 🟢 |
| 61 | `refused == 0` | 0 | 🟢 |
| 62 | `panels == 2` | 2 | 🟢 |
| 63 | `sound == 1` | 1 | 🟢 |
| 70 | `zebra == 1` | 1 | 🟢 |
| 71 | `triangles > 1500` | 5782 | 🟢 |
| 72 | `status contains Zebra on` | Zebra on · 64 stripes · 5782 triangles | 🟢 |
| 82 | `panels == 1` | 1 | 🟢 |
| 90 | `loft-continuity == 2` | 2 | 🟢 |
| 93 | `steps == 13` | 13 | 🟢 |
| 94 | `refused == 0` | 0 | 🟢 |
| 95 | `panels == 2` | 2 | 🟢 |
| 96 | `sound == 1` | 1 | 🟢 |
| 97 | `status contains · G2` | Lofted 4 curve(s) into a panel · G2 (sound) · 2 panel(s) · open sections stay a shee... | 🟢 |
| 100 | `zebra == 1` | 1 | 🟢 |
| 112 | `zebra == 0` | 0 | 🟢 |
| 117 | `deviation-points == 286` | 286 | 🟢 |
| 118 | `deviation-worst > 0.002` | 0.007135962802 | 🟢 |
| 143 | `selected-curves == Section1,Section2,Section3,Section4,Section5,Section6` | Section1,Section2,Section3,Section4,Section5,Section6 | 🟢 |
| 149 | `panels == 2` | 2 | 🟢 |
| 150 | `sound == 1` | 1 | 🟢 |
| 157 | `zebra == 1` | 1 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 70ZebraG0 | 154 | `1325032e5ca2ff45` |
| 71ZebraG2 | 218 | `dee0e7d93984770e` |
| 72DeviationMap | 264 | `4e96b8f0e269e9c1` |
| 73ZebraMirrored | 386 | `090f5bbbc2c3f110` |

## Journal

- New document (previous recovery copy kept)
- Import 11 curve(s) · 2 ms
- Framed the document (Space)
- Imported 11 curve(s) from Experimental/VehicleSurfacing/Samples/SyntheticBonnet.toml · 44 poles · worst fit 0.056 mm
- View Isometric
- Framed the document (Space)
- Move pole 2 of Section3 · 1 ms
- Moved pole 2 of Section3 by 21.27 mm · 1 ms
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
- Zebra on · 64 stripes · 5782 triangles
- View Top
- Autosaved _ClaudeScratch/SolidArcWorkspace/EvaluationRecovery.solidarc
- Undo
- Selected curve Section3 · 4 poles · fit 0.042 mm · 1 selected
- Selected curve Section4 · 4 poles · fit 0.049 mm · 2 selected
- Selected curve Section5 · 4 poles · fit 0.045 mm · 3 selected
- Selected curve Section6 · 4 poles · fit 0.046 mm · 4 selected
- Loft2 · 4 sections · G2 · 1 ms
- Lofted 4 curve(s) into a panel · G2 (sound) · 2 panel(s) · open sections stay a sheet · 1 ms
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
- Mirror1 · plane x = 0 · 3 ms
- Mirrored through x = 0 → 2 panel(s), sound · 3 ms
- Selection cleared
- Zebra on · 64 stripes · 6068 triangles
- View Top
- Framed the document (Space)
- Autosaved _ClaudeScratch/SolidArcWorkspace/EvaluationRecovery.solidarc
