# DraftingWorkspaceCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0
- Frame: 1600 x 900 · 569 frames · 255.378 s · slowest frame 61106.1 ms
- Script: Experimental/DraftingWorkspace/Scripts/BlueprintModellingWalkthrough.txt
- Expectations: 48 / 48 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 22 | `steps == 24` | 24 | 🟢 |
| 23 | `refused == 0` | 0 | 🟢 |
| 24 | `undo == 1` | 1 | 🟢 |
| 25 | `image-planes == 3` | 3 | 🟢 |
| 26 | `curve-fit-worst < 0.0002` | 0.0001931273581 | 🟢 |
| 27 | `status contains Imported 24 curve(s)` | Imported 24 curve(s) from Experimental/VehicleSurfacing/Samples/BmwM3E90Sections.toml �... | 🟢 |
| 30 | `orthographic == 1` | 1 | 🟢 |
| 37 | `image-planes-drawn == 3` | 3 | 🟢 |
| 53 | `image-planes-drawn == 0` | 0 | 🟢 |
| 56 | `image-planes-drawn == 3` | 3 | 🟢 |
| 59 | `image-planes-drawn == 2` | 2 | 🟢 |
| 60 | `dirty == 1` | 1 | 🟢 |
| 63 | `image-planes-drawn == 3` | 3 | 🟢 |
| 74 | `box-selections == 1` | 1 | 🟢 |
| 75 | `selected-curves contains Station01,Station02,Station03` | Station01,Station02,Station03,Station04,Station05,Station06,Station07,Station08,Station... | 🟢 |
| 76 | `status contains Box selected 24 curve(s)` | Box selected 24 curve(s) · 24 selected | 🟢 |
| 82 | `steps == 25` | 25 | 🟢 |
| 83 | `sheet == 1` | 1 | 🟢 |
| 84 | `panels == 1` | 1 | 🟢 |
| 87 | `panels == 2` | 2 | 🟢 |
| 90 | `refused == 0` | 0 | 🟢 |
| 91 | `sheet == 0` | 0 | 🟢 |
| 92 | `sound == 1` | 1 | 🟢 |
| 93 | `faces == 4` | 4 | 🟢 |
| 94 | `volume > 7.14` | 7.14816437 | 🟢 |
| 95 | `volume < 7.16` | 7.14816437 | 🟢 |
| 128 | `refused == 0` | 0 | 🟢 |
| 129 | `sound == 1` | 1 | 🟢 |
| 130 | `faces == 6` | 6 | 🟢 |
| 131 | `volume > 7.04` | 7.054108837 | 🟢 |
| 132 | `volume < 7.07` | 7.054108837 | 🟢 |
| 157 | `refused == 0` | 0 | 🟢 |
| 158 | `sound == 1` | 1 | 🟢 |
| 159 | `faces == 8` | 8 | 🟢 |
| 160 | `volume > 6.95` | 6.960053304 | 🟢 |
| 161 | `volume < 6.97` | 6.960053304 | 🟢 |
| 186 | `refused == 0` | 0 | 🟢 |
| 187 | `sound == 1` | 1 | 🟢 |
| 188 | `faces == 10` | 10 | 🟢 |
| 189 | `volume > 6.86` | 6.875231305 | 🟢 |
| 190 | `volume < 6.89` | 6.875231305 | 🟢 |
| 215 | `refused == 0` | 0 | 🟢 |
| 216 | `sound == 1` | 1 | 🟢 |
| 217 | `faces == 12` | 12 | 🟢 |
| 218 | `volume > 6.78` | 6.790409307 | 🟢 |
| 219 | `volume < 6.8` | 6.790409307 | 🟢 |
| 223 | `pick-mode == 3` | 3 | 🟢 |
| 236 | `image-planes-drawn == 0` | 0 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 80BlueprintSide | 51 | `bbee353788ab23aa` |
| 81BlueprintTop | 66 | `0d02b1c75b80f0dd` |
| 82BlueprintFront | 81 | `766861fef42ab4e0` |
| 83SectionsBoxed | 147 | `349b59bd1d0b3048` |
| 84BodySolid | 182 | `0a11b647a50dcd4d` |
| 85ArchDialog | 258 | `f1445ea21fd62aab` |
| 86ArchesIso | 532 | `a2f9fc18ea9a8af6` |
| 87ArchesSide | 547 | `c5bc5d9d51818127` |
| 88ArchesSolid | 569 | `0fa2e9c2ceb52088` |

## Journal

- New document (previous recovery copy kept)
- Import 24 curve(s) · 95 ms
- Framed the document (Space)
- Imported 24 curve(s) from Experimental/VehicleSurfacing/Samples/BmwM3E90Sections.toml · 540 poles · worst fit 0.193 mm
- View Right
- Framed the document (Space)
- View Top
- Framed the document (Space)
- Autosaved _ClaudeScratch/DraftingWorkspace/BlueprintRecovery.drafting.toml
- View Front
- Framed the document (Space)
- View Right
- Framed the document (Space)
- Box selected 24 curve(s) · 24 selected
- Loft1 · 24 sections · 96 ms
- Lofted 24 curve(s) → panel (sound) · 1 panel(s) · 96 ms
- Mirror1 · plane x = 0 · 162 ms
- Mirrored through x = 0 → 2 panel(s), sound · 162 ms
- Close1 · plane caps · 1077 ms
- Closed → solid · volume 7.14816 m³ · sound · 1077 ms
- View Isometric
- Framed the document (Space)
- Autosaved _ClaudeScratch/DraftingWorkspace/BlueprintRecovery.drafting.toml
- Cylinder1 · Difference · 10773 ms
- Autosaved _ClaudeScratch/DraftingWorkspace/BlueprintRecovery.drafting.toml
- Cylinder2 · Difference · 22847 ms
- Autosaved _ClaudeScratch/DraftingWorkspace/BlueprintRecovery.drafting.toml
- Cylinder3 · Difference · 41952 ms
- Autosaved _ClaudeScratch/DraftingWorkspace/BlueprintRecovery.drafting.toml
- Cylinder4 · Difference · 59926 ms
- View Isometric
- Framed the document (Space)
- View Right
- Framed the document (Space)
- Autosaved _ClaudeScratch/DraftingWorkspace/BlueprintRecovery.drafting.toml
- View Isometric
- Framed the document (Space)
