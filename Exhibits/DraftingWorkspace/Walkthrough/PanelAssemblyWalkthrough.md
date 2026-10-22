# DraftingWorkspaceCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0
- Frame: 1600 x 900 · 358 frames · 45.5826 s · slowest frame 768.3 ms
- Script: /home/user/Frontier/Experimental/DraftingWorkspace/Scripts/PanelAssemblyWalkthrough.txt
- Expectations: 57 / 57 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 18 | `steps == 11` | 11 | 🟢 |
| 19 | `refused == 0` | 0 | 🟢 |
| 30 | `selected-curves == Section1,Section2,Section3` | Section1,Section2,Section3 | 🟢 |
| 31 | `loft-continuity == 0` | 0 | 🟢 |
| 34 | `steps == 12` | 12 | 🟢 |
| 35 | `refused == 0` | 0 | 🟢 |
| 36 | `panels == 1` | 1 | 🟢 |
| 37 | `sheet == 1` | 1 | 🟢 |
| 38 | `faces == 1` | 1 | 🟢 |
| 39 | `sound == 1` | 1 | 🟢 |
| 40 | `status contains Lofted 3 curve(s)` | Lofted 3 curve(s) → panel (sound) · 1 panel(s) · 1 ms | 🟢 |
| 48 | `selected-curves == Section3,Section4,Section5,Section6` | Section3,Section4,Section5,Section6 | 🟢 |
| 51 | `loft-continuity == 2` | 2 | 🟢 |
| 55 | `steps == 13` | 13 | 🟢 |
| 56 | `refused == 0` | 0 | 🟢 |
| 57 | `panels == 2` | 2 | 🟢 |
| 58 | `sheet == 1` | 1 | 🟢 |
| 59 | `faces == 2` | 2 | 🟢 |
| 60 | `sound == 1` | 1 | 🟢 |
| 61 | `status contains · G2` | Lofted 4 curve(s) → panel · G2 (sound) · 2 panel(s) · 1 ms | 🟢 |
| 67 | `steps == 14` | 14 | 🟢 |
| 68 | `refused == 0` | 0 | 🟢 |
| 69 | `panels == 4` | 4 | 🟢 |
| 70 | `sheet == 1` | 1 | 🟢 |
| 71 | `faces == 4` | 4 | 🟢 |
| 72 | `sound == 1` | 1 | 🟢 |
| 73 | `panel-names contains Mirror#14.Image(Loft#12.Side(Surface))` | Loft#12.Side(Surface),Loft#13.Side(Surface),Mirror#14.Image(Loft#12.Side(Surface)),Mirr... | 🟢 |
| 74 | `status contains Mirrored through x = 0` | Mirrored through x = 0 → 4 panel(s), sound · 6 ms | 🟢 |
| 82 | `steps == 14` | 14 | 🟢 |
| 83 | `panels == 4` | 4 | 🟢 |
| 84 | `status contains not planar` | Refused: Close#15: Close: rim loop 0 is not planar (off by 51.7 mm; only plane caps ) | 🟢 |
| 97 | `steps == 5` | 5 | 🟢 |
| 98 | `refused == 0` | 0 | 🟢 |
| 111 | `selected-curves == Hoop1,Hoop2,Hoop3,Hoop4,Hoop5` | Hoop1,Hoop2,Hoop3,Hoop4,Hoop5 | 🟢 |
| 114 | `loft-continuity == 0` | 0 | 🟢 |
| 117 | `panels == 1` | 1 | 🟢 |
| 118 | `sheet == 1` | 1 | 🟢 |
| 121 | `panels == 2` | 2 | 🟢 |
| 122 | `sheet == 1` | 1 | 🟢 |
| 123 | `faces == 2` | 2 | 🟢 |
| 127 | `steps == 8` | 8 | 🟢 |
| 128 | `refused == 0` | 0 | 🟢 |
| 129 | `sheet == 0` | 0 | 🟢 |
| 130 | `faces == 4` | 4 | 🟢 |
| 131 | `sound == 1` | 1 | 🟢 |
| 132 | `volume > 0.791` | 0.7928018098 | 🟢 |
| 133 | `volume < 0.794` | 0.7928018098 | 🟢 |
| 134 | `status contains Closed → solid` | Closed → solid · volume 0.79280 m³ · sound · 35 ms | 🟢 |
| 149 | `steps == 10` | 10 | 🟢 |
| 167 | `selected-curves == Spine2,Section1,Section4,Spine1,Section2,Spine3,Section6,Section3,Spine4,Section5` | Spine2,Section1,Section4,Spine1,Section2,Spine3,Section6,Section3,Spine4,Section5 | 🟢 |
| 173 | `steps == 11` | 11 | 🟢 |
| 174 | `refused == 0` | 0 | 🟢 |
| 175 | `panels == 1` | 1 | 🟢 |
| 176 | `sheet == 1` | 1 | 🟢 |
| 177 | `sound == 1` | 1 | 🟢 |
| 178 | `status contains Network of 10 curve(s)` | Network of 10 curve(s) → panel (sound) · 1 panel(s) · 3 ms | 🟢 |
| 184 | `dirty == 0` | 0 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 60RearSectionsG2 | 93 | `3ae0684940dc799c` |
| 61FrontRearG2 | 101 | `ff35c7b4eeb2ada3` |
| 62BonnetMirrored | 115 | `1770d1eb30461962` |
| 63CloseRefused | 125 | `f1336448219ff3ab` |
| 64PodHalves | 220 | `0fd7236c94d63f13` |
| 65PodSolid | 236 | `88e7543d3c38da48` |
| 66NetworkCurves | 344 | `f2ab64ace7129a88` |
| 67NetworkPanel | 352 | `889e3484959916be` |

## Journal

- New document (previous recovery copy kept)
- Import 11 curve(s) · 3 ms
- Framed the document (Space)
- Imported 11 curve(s) from Experimental/VehicleSurfacing/Samples/SyntheticBonnet.toml · 44 poles · worst fit 0.056 mm
- View Isometric
- Framed the document (Space)
- Selected curve Section1 · 4 poles · fit 0.045 mm · 1 selected
- Selected curve Section2 · 4 poles · fit 0.049 mm · 2 selected
- Selected curve Section3 · 4 poles · fit 0.042 mm · 3 selected
- Autosaved _ClaudeScratch/DraftingWorkspace/PanelRecovery.drafting.toml
- Loft1 · 3 sections · 1 ms
- Lofted 3 curve(s) → panel (sound) · 1 panel(s) · 1 ms
- Selected curve Section3 · 4 poles · fit 0.042 mm · 1 selected
- Selected curve Section4 · 4 poles · fit 0.049 mm · 2 selected
- Selected curve Section5 · 4 poles · fit 0.045 mm · 3 selected
- Selected curve Section6 · 4 poles · fit 0.046 mm · 4 selected
- Loft2 · 4 sections · G2 · 1 ms
- Lofted 4 curve(s) → panel · G2 (sound) · 2 panel(s) · 1 ms
- Mirror1 · plane x = 0 · 6 ms
- Mirrored through x = 0 → 4 panel(s), sound · 6 ms
- Framed the document (Space)
- Refused: Close#15: Close: rim loop 0 is not planar (off by 51.7 mm; only plane caps )
- New document (previous recovery copy kept)
- Autosaved _ClaudeScratch/DraftingWorkspace/PanelRecovery.drafting.toml
- Import 5 curve(s) · 7 ms
- Framed the document (Space)
- Imported 5 curve(s) from Experimental/VehicleSurfacing/Samples/SyntheticPod.toml · 46 poles · worst fit 0.066 mm
- View Isometric
- Framed the document (Space)
- Selected curve Hoop1 · 9 poles · fit 0.053 mm · 1 selected
- Selected curve Hoop2 · 9 poles · fit 0.054 mm · 2 selected
- Selected curve Hoop3 · 9 poles · fit 0.061 mm · 3 selected
- Autosaved _ClaudeScratch/DraftingWorkspace/PanelRecovery.drafting.toml
- Selected curve Hoop4 · 9 poles · fit 0.060 mm · 4 selected
- Selected curve Hoop5 · 10 poles · fit 0.066 mm · 5 selected
- Loft1 · 5 sections · 1 ms
- Lofted 5 curve(s) → panel (sound) · 1 panel(s) · 1 ms
- Mirror1 · plane x = 0 · 8 ms
- Mirrored through x = 0 → 2 panel(s), sound · 8 ms
- Close1 · plane caps · 35 ms
- Closed → solid · volume 0.79280 m³ · sound · 35 ms
- Framed the document (Space)
- New document (previous recovery copy kept)
- Import 10 curve(s) · 3 ms
- Framed the document (Space)
- Imported 10 curve(s) from Experimental/VehicleSurfacing/Samples/SyntheticBonnetNetwork.toml · 43 poles · worst fit 0.071 mm
- View Isometric
- Framed the document (Space)
- Selected curve Spine2 · 5 poles · fit 0.064 mm · 1 selected
- Selected curve Section1 · 4 poles · fit 0.036 mm · 2 selected
- Selected curve Section4 · 4 poles · fit 0.046 mm · 3 selected
- Autosaved _ClaudeScratch/DraftingWorkspace/PanelRecovery.drafting.toml
- Selected curve Spine1 · 5 poles · fit 0.071 mm · 4 selected
- Selected curve Section2 · 4 poles · fit 0.042 mm · 5 selected
- Selected curve Spine3 · 5 poles · fit 0.050 mm · 6 selected
- Selected curve Section6 · 4 poles · fit 0.042 mm · 7 selected
- Selected curve Section3 · 4 poles · fit 0.044 mm · 8 selected
- Selected curve Spine4 · 4 poles · fit 0.049 mm · 9 selected
- Selected curve Section5 · 4 poles · fit 0.045 mm · 10 selected
- Network1 · 10 curves · 3 ms
- Network of 10 curve(s) → panel (sound) · 1 panel(s) · 3 ms
- Saved _ClaudeScratch/DraftingWorkspace/Panels.drafting.toml
