# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 900 · 388 frames · 38.6665 s · slowest frame 854.597 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/PanelAssemblyWalkthrough.txt
- Expectations: 57 / 57 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 21 | `steps == 11` | 11 | 🟢 |
| 22 | `refused == 0` | 0 | 🟢 |
| 35 | `selected-curves == Section1,Section2,Section3` | Section1,Section2,Section3 | 🟢 |
| 36 | `loft-continuity == 0` | 0 | 🟢 |
| 39 | `steps == 12` | 12 | 🟢 |
| 40 | `refused == 0` | 0 | 🟢 |
| 41 | `panels == 1` | 1 | 🟢 |
| 42 | `sheet == 1` | 1 | 🟢 |
| 43 | `faces == 1` | 1 | 🟢 |
| 44 | `sound == 1` | 1 | 🟢 |
| 45 | `status contains Lofted 3 curve(s)` | Lofted 3 curve(s) into a panel (sound) · 1 panel(s) · open sections stay a sheet · 1 ms | 🟢 |
| 53 | `selected-curves == Section3,Section4,Section5,Section6` | Section3,Section4,Section5,Section6 | 🟢 |
| 56 | `loft-continuity == 2` | 2 | 🟢 |
| 60 | `steps == 13` | 13 | 🟢 |
| 61 | `refused == 0` | 0 | 🟢 |
| 62 | `panels == 2` | 2 | 🟢 |
| 63 | `sheet == 1` | 1 | 🟢 |
| 64 | `faces == 2` | 2 | 🟢 |
| 65 | `sound == 1` | 1 | 🟢 |
| 66 | `status contains · G2` | Lofted 4 curve(s) into a panel · G2 (sound) · 2 panel(s) · open sections stay a shee... | 🟢 |
| 72 | `steps == 14` | 14 | 🟢 |
| 73 | `refused == 0` | 0 | 🟢 |
| 74 | `panels == 4` | 4 | 🟢 |
| 75 | `sheet == 1` | 1 | 🟢 |
| 76 | `faces == 4` | 4 | 🟢 |
| 77 | `sound == 1` | 1 | 🟢 |
| 78 | `panel-names contains Mirror#14.Image(Loft#12.Side(Surface))` | Loft#12.Side(Surface),Loft#13.Side(Surface),Mirror#14.Image(Loft#12.Side(Surface)),Mirr... | 🟢 |
| 79 | `status contains Mirrored through x = 0` | Mirrored through x = 0 → 4 panel(s), sound · 6 ms | 🟢 |
| 87 | `steps == 14` | 14 | 🟢 |
| 88 | `panels == 4` | 4 | 🟢 |
| 89 | `status contains not planar` | Refused: Close#15: Close: rim loop 0 is not planar (off by 51.7 mm; only plane caps ) | 🟢 |
| 102 | `steps == 5` | 5 | 🟢 |
| 103 | `refused == 0` | 0 | 🟢 |
| 118 | `selected-curves == Hoop1,Hoop2,Hoop3,Hoop4,Hoop5` | Hoop1,Hoop2,Hoop3,Hoop4,Hoop5 | 🟢 |
| 121 | `loft-continuity == 0` | 0 | 🟢 |
| 124 | `panels == 1` | 1 | 🟢 |
| 125 | `sheet == 1` | 1 | 🟢 |
| 128 | `panels == 2` | 2 | 🟢 |
| 129 | `sheet == 1` | 1 | 🟢 |
| 130 | `faces == 2` | 2 | 🟢 |
| 134 | `steps == 8` | 8 | 🟢 |
| 135 | `refused == 0` | 0 | 🟢 |
| 136 | `sheet == 0` | 0 | 🟢 |
| 137 | `faces == 4` | 4 | 🟢 |
| 138 | `sound == 1` | 1 | 🟢 |
| 139 | `volume > 0.791` | 0.7928018098 | 🟢 |
| 140 | `volume < 0.794` | 0.7928018098 | 🟢 |
| 141 | `status contains Closed → solid` | Closed → solid · volume 0.79280 m³ · sound · 24 ms | 🟢 |
| 156 | `steps == 10` | 10 | 🟢 |
| 176 | `selected-curves == Spine2,Section1,Section4,Spine1,Section2,Spine3,Section6,Section3,Spine4,Section5` | Spine2,Section1,Section4,Spine1,Section2,Spine3,Section6,Section3,Spine4,Section5 | 🟢 |
| 182 | `steps == 11` | 11 | 🟢 |
| 183 | `refused == 0` | 0 | 🟢 |
| 184 | `panels == 1` | 1 | 🟢 |
| 185 | `sheet == 1` | 1 | 🟢 |
| 186 | `sound == 1` | 1 | 🟢 |
| 187 | `status contains Network of 10 curve(s)` | Network of 10 curve(s) into a panel (sound) · 1 panel(s) · open sections stay a sheet... | 🟢 |
| 193 | `dirty == 0` | 0 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 60RearSectionsG2 | 101 | `f4b8f1570a854113` |
| 61FrontRearG2 | 109 | `29c83426b516611a` |
| 62BonnetMirrored | 123 | `b3748e60b5c61267` |
| 63CloseRefused | 137 | `fe9677cba8b52045` |
| 64PodHalves | 239 | `bb48b81a1b05653c` |
| 65PodSolid | 259 | `fe5158704f4c90f9` |
| 66NetworkCurves | 374 | `665d56b3487508f9` |
| 67NetworkPanel | 382 | `82bcde946b02992d` |

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
- Import 5 curve(s) · 8 ms
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
- Close1 · plane caps · 24 ms
- Closed → solid · volume 0.79280 m³ · sound · 24 ms
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
