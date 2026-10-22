# DraftingWorkspaceCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0
- Frame: 1600 x 900 · 316 frames · 33.8227 s · slowest frame 2250.89 ms
- Script: /home/user/Frontier/Experimental/DraftingWorkspace/Scripts/SurfaceEvaluationWalkthrough.txt
- Expectations: 28 / 28 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 20 | `steps == 11` | 11 | 🟢 |
| 35 | `pole-edits == 1` | 1 | 🟢 |
| 36 | `status contains Moved pole 2 of Section3` | Moved pole 2 of Section3 by 20.92 mm · 2 ms | 🟢 |
| 43 | `loft-continuity == 0` | 0 | 🟢 |
| 46 | `panels == 1` | 1 | 🟢 |
| 52 | `selected-curves == Section3,Section4,Section5,Section6` | Section3,Section4,Section5,Section6 | 🟢 |
| 55 | `steps == 13` | 13 | 🟢 |
| 56 | `refused == 0` | 0 | 🟢 |
| 57 | `panels == 2` | 2 | 🟢 |
| 58 | `sound == 1` | 1 | 🟢 |
| 63 | `zebra == 1` | 1 | 🟢 |
| 64 | `triangles > 1500` | 5766 | 🟢 |
| 65 | `status contains Zebra on` | Zebra on · 64 stripes · 5766 triangles | 🟢 |
| 73 | `panels == 1` | 1 | 🟢 |
| 81 | `loft-continuity == 2` | 2 | 🟢 |
| 84 | `steps == 13` | 13 | 🟢 |
| 85 | `refused == 0` | 0 | 🟢 |
| 86 | `panels == 2` | 2 | 🟢 |
| 87 | `sound == 1` | 1 | 🟢 |
| 88 | `status contains · G2` | Lofted 4 curve(s) → panel · G2 (sound) · 2 panel(s) · 2 ms | 🟢 |
| 91 | `zebra == 1` | 1 | 🟢 |
| 99 | `zebra == 0` | 0 | 🟢 |
| 102 | `deviation-points == 286` | 286 | 🟢 |
| 103 | `deviation-worst > 0.002` | 0.007021660845 | 🟢 |
| 124 | `selected-curves == Section1,Section2,Section3,Section4,Section5,Section6` | Section1,Section2,Section3,Section4,Section5,Section6 | 🟢 |
| 130 | `panels == 2` | 2 | 🟢 |
| 131 | `sound == 1` | 1 | 🟢 |
| 136 | `zebra == 1` | 1 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 70ZebraG0 | 134 | `a38ef55f960ab0d9` |
| 71ZebraG2 | 188 | `56fb153921efae61` |
| 72DeviationMap | 213 | `88056fe7c9580766` |
| 73ZebraMirrored | 316 | `1f1c54c7dea5e5b7` |

## Journal

- New document (previous recovery copy kept)
- Import 11 curve(s) · 2 ms
- Framed the document (Space)
- Imported 11 curve(s) from Experimental/VehicleSurfacing/Samples/SyntheticBonnet.toml · 44 poles · worst fit 0.056 mm
- View Isometric
- Framed the document (Space)
- Move pole 2 of Section3 · 2 ms
- Moved pole 2 of Section3 by 20.92 mm · 2 ms
- Selected curve Section1 · 4 poles · fit 0.045 mm · 1 selected
- Selected curve Section2 · 4 poles · fit 0.049 mm · 2 selected
- Selected curve Section3 · 4 poles · fit 0.042 mm · 3 selected
- Loft1 · 3 sections · 1 ms
- Lofted 3 curve(s) → panel (sound) · 1 panel(s) · 1 ms
- Selected curve Section3 · 4 poles · fit 0.042 mm · 1 selected
- Selected curve Section4 · 4 poles · fit 0.049 mm · 2 selected
- Selected curve Section5 · 4 poles · fit 0.045 mm · 3 selected
- Selected curve Section6 · 4 poles · fit 0.046 mm · 4 selected
- Loft2 · 4 sections · 2 ms
- Lofted 4 curve(s) → panel (sound) · 2 panel(s) · 2 ms
- Selection cleared
- Zebra on · 64 stripes · 5766 triangles
- View Top
- Undo
- Selected curve Section3 · 4 poles · fit 0.042 mm · 1 selected
- Selected curve Section4 · 4 poles · fit 0.049 mm · 2 selected
- Selected curve Section5 · 4 poles · fit 0.045 mm · 3 selected
- Selected curve Section6 · 4 poles · fit 0.046 mm · 4 selected
- Loft2 · 4 sections · G2 · 2 ms
- Lofted 4 curve(s) → panel · G2 (sound) · 2 panel(s) · 2 ms
- Selection cleared
- View Top
- Zebra off
- View Isometric
- Autosaved _ClaudeScratch/DraftingWorkspace/EvaluationRecovery.drafting.toml
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
- Lofted 6 curve(s) → panel (sound) · 1 panel(s) · 1 ms
- Mirror1 · plane x = 0 · 4 ms
- Mirrored through x = 0 → 2 panel(s), sound · 4 ms
- Selection cleared
- Zebra on · 64 stripes · 6068 triangles
- View Top
- Framed the document (Space)
