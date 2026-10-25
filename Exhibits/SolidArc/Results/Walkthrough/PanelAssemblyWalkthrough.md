# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0
- Frame: 1600 x 900 · 387 frames · 35.094 s · slowest frame 734.089 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/PanelAssemblyWalkthrough.txt
- Expectations: 57 / 57 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 18 | `steps == 11` | 11 | 🟢 |
| 19 | `refused == 0` | 0 | 🟢 |
| 32 | `selected-curves == Section1,Section2,Section3` | Section1,Section2,Section3 | 🟢 |
| 33 | `loft-continuity == 0` | 0 | 🟢 |
| 36 | `steps == 12` | 12 | 🟢 |
| 37 | `refused == 0` | 0 | 🟢 |
| 38 | `panels == 1` | 1 | 🟢 |
| 39 | `sheet == 1` | 1 | 🟢 |
| 40 | `faces == 1` | 1 | 🟢 |
| 41 | `sound == 1` | 1 | 🟢 |
| 42 | `status contains Lofted 3 curve(s)` | Lofted 3 curve(s) into a panel (sound) · 1 panel(s) · open sections stay a sheet · 1 ms | 🟢 |
| 50 | `selected-curves == Section3,Section4,Section5,Section6` | Section3,Section4,Section5,Section6 | 🟢 |
| 53 | `loft-continuity == 2` | 2 | 🟢 |
| 57 | `steps == 13` | 13 | 🟢 |
| 58 | `refused == 0` | 0 | 🟢 |
| 59 | `panels == 2` | 2 | 🟢 |
| 60 | `sheet == 1` | 1 | 🟢 |
| 61 | `faces == 2` | 2 | 🟢 |
| 62 | `sound == 1` | 1 | 🟢 |
| 63 | `status contains · G2` | Lofted 4 curve(s) into a panel · G2 (sound) · 2 panel(s) · open sections stay a shee... | 🟢 |
| 69 | `steps == 14` | 14 | 🟢 |
| 70 | `refused == 0` | 0 | 🟢 |
| 71 | `panels == 4` | 4 | 🟢 |
| 72 | `sheet == 1` | 1 | 🟢 |
| 73 | `faces == 4` | 4 | 🟢 |
| 74 | `sound == 1` | 1 | 🟢 |
| 75 | `panel-names contains Mirror#14.Image(Loft#12.Side(Surface))` | Loft#12.Side(Surface),Loft#13.Side(Surface),Mirror#14.Image(Loft#12.Side(Surface)),Mirr... | 🟢 |
| 76 | `status contains Mirrored through x = 0` | Mirrored through x = 0 → 4 panel(s), sound · 6 ms | 🟢 |
| 84 | `steps == 14` | 14 | 🟢 |
| 85 | `panels == 4` | 4 | 🟢 |
| 86 | `status contains not planar` | Refused: Close#15: Close: rim loop 0 is not planar (off by 51.7 mm; only plane caps ) | 🟢 |
| 99 | `steps == 5` | 5 | 🟢 |
| 100 | `refused == 0` | 0 | 🟢 |
| 115 | `selected-curves == Hoop1,Hoop2,Hoop3,Hoop4,Hoop5` | Hoop1,Hoop2,Hoop3,Hoop4,Hoop5 | 🟢 |
| 118 | `loft-continuity == 0` | 0 | 🟢 |
| 121 | `panels == 1` | 1 | 🟢 |
| 122 | `sheet == 1` | 1 | 🟢 |
| 125 | `panels == 2` | 2 | 🟢 |
| 126 | `sheet == 1` | 1 | 🟢 |
| 127 | `faces == 2` | 2 | 🟢 |
| 131 | `steps == 8` | 8 | 🟢 |
| 132 | `refused == 0` | 0 | 🟢 |
| 133 | `sheet == 0` | 0 | 🟢 |
| 134 | `faces == 4` | 4 | 🟢 |
| 135 | `sound == 1` | 1 | 🟢 |
| 136 | `volume > 0.791` | 0.7928018098 | 🟢 |
| 137 | `volume < 0.794` | 0.7928018098 | 🟢 |
| 138 | `status contains Closed → solid` | Closed → solid · volume 0.79280 m³ · sound · 33 ms | 🟢 |
| 153 | `steps == 10` | 10 | 🟢 |
| 173 | `selected-curves == Spine2,Section1,Section4,Spine1,Section2,Spine3,Section6,Section3,Spine4,Section5` | Spine2,Section1,Section4,Spine1,Section2,Spine3,Section6,Section3,Spine4,Section5 | 🟢 |
| 179 | `steps == 11` | 11 | 🟢 |
| 180 | `refused == 0` | 0 | 🟢 |
| 181 | `panels == 1` | 1 | 🟢 |
| 182 | `sheet == 1` | 1 | 🟢 |
| 183 | `sound == 1` | 1 | 🟢 |
| 184 | `status contains Network of 10 curve(s)` | Network of 10 curve(s) into a panel (sound) · 1 panel(s) · open sections stay a sheet... | 🟢 |
| 190 | `dirty == 0` | 0 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 60RearSectionsG2 | 100 | `dcd45c7742c50f5e` |
| 61FrontRearG2 | 108 | `e060723c96199703` |
| 62BonnetMirrored | 122 | `f4e9169952cde370` |
| 63CloseRefused | 136 | `fb33d208e23b496e` |
| 64PodHalves | 238 | `9966790a4ca60595` |
| 65PodSolid | 258 | `ef0ebae2df543ffc` |
| 66NetworkCurves | 373 | `936593f4a507a9af` |
| 67NetworkPanel | 381 | `e89c81025a88dd43` |

## Journal

- New document (previous recovery copy kept)
- Import 11 curve(s) · 2 ms
- Framed the document (Space)
- Imported 11 curve(s) from Experimental/VehicleSurfacing/Samples/SyntheticBonnet.toml · 44 poles · worst fit 0.056 mm
- View Isometric
- Framed the document (Space)
- Selected curve Section1 · 4 poles · fit 0.045 mm · 1 selected
- Selected curve Section2 · 4 poles · fit 0.049 mm · 2 selected
- Autosaved _ClaudeScratch/SolidArcWorkspace/PanelRecovery.solidarc
- Selected curve Section3 · 4 poles · fit 0.042 mm · 3 selected
- Loft1 · 3 sections · 1 ms
- Lofted 3 curve(s) into a panel (sound) · 1 panel(s) · open sections stay a sheet · 1 ms
- Selected curve Section3 · 4 poles · fit 0.042 mm · 1 selected
- Selected curve Section4 · 4 poles · fit 0.049 mm · 2 selected
- Selected curve Section5 · 4 poles · fit 0.045 mm · 3 selected
- Selected curve Section6 · 4 poles · fit 0.046 mm · 4 selected
- Loft2 · 4 sections · G2 · 1 ms
- Lofted 4 curve(s) into a panel · G2 (sound) · 2 panel(s) · open sections stay a sheet · 1 ms
- Mirror1 · plane x = 0 · 6 ms
- Mirrored through x = 0 → 4 panel(s), sound · 6 ms
- Framed the document (Space)
- Refused: Close#15: Close: rim loop 0 is not planar (off by 51.7 mm; only plane caps )
- New document (previous recovery copy kept)
- Autosaved _ClaudeScratch/SolidArcWorkspace/PanelRecovery.solidarc
- Import 5 curve(s) · 7 ms
- Framed the document (Space)
- Imported 5 curve(s) from Experimental/VehicleSurfacing/Samples/SyntheticPod.toml · 46 poles · worst fit 0.066 mm
- View Isometric
- Framed the document (Space)
- Selected curve Hoop1 · 9 poles · fit 0.053 mm · 1 selected
- Selected curve Hoop2 · 9 poles · fit 0.054 mm · 2 selected
- Autosaved _ClaudeScratch/SolidArcWorkspace/PanelRecovery.solidarc
- Selected curve Hoop3 · 9 poles · fit 0.061 mm · 3 selected
- Selected curve Hoop4 · 9 poles · fit 0.060 mm · 4 selected
- Selected curve Hoop5 · 10 poles · fit 0.066 mm · 5 selected
- Loft1 · 5 sections · 1 ms
- Lofted 5 curve(s) into a panel (sound) · 1 panel(s) · open sections stay a sheet · 1 ms
- Mirror1 · plane x = 0 · 8 ms
- Mirrored through x = 0 → 2 panel(s), sound · 8 ms
- Close1 · plane caps · 33 ms
- Closed → solid · volume 0.79280 m³ · sound · 33 ms
- Framed the document (Space)
- New document (previous recovery copy kept)
- Import 10 curve(s) · 3 ms
- Framed the document (Space)
- Imported 10 curve(s) from Experimental/VehicleSurfacing/Samples/SyntheticBonnetNetwork.toml · 43 poles · worst fit 0.071 mm
- View Isometric
- Framed the document (Space)
- Selected curve Spine2 · 5 poles · fit 0.064 mm · 1 selected
- Selected curve Section1 · 4 poles · fit 0.036 mm · 2 selected
- Autosaved _ClaudeScratch/SolidArcWorkspace/PanelRecovery.solidarc
- Selected curve Section4 · 4 poles · fit 0.046 mm · 3 selected
- Selected curve Spine1 · 5 poles · fit 0.071 mm · 4 selected
- Selected curve Section2 · 4 poles · fit 0.042 mm · 5 selected
- Selected curve Spine3 · 5 poles · fit 0.050 mm · 6 selected
- Selected curve Section6 · 4 poles · fit 0.042 mm · 7 selected
- Selected curve Section3 · 4 poles · fit 0.044 mm · 8 selected
- Selected curve Spine4 · 4 poles · fit 0.049 mm · 9 selected
- Selected curve Section5 · 4 poles · fit 0.045 mm · 10 selected
- Network1 · 10 curves · 3 ms
- Network of 10 curve(s) into a panel (sound) · 1 panel(s) · open sections stay a sheet · 3 ms
- Saved _ClaudeScratch/SolidArcWorkspace/Panels.solidarc
