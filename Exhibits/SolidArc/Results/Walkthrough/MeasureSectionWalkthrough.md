# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 900 · 124 frames · 16.8187 s · slowest frame 524.737 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/MeasureSectionWalkthrough.txt
- Expectations: 32 / 32 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 14 | `steps == 8` | 8 | 🟢 |
| 15 | `measure == 0` | off | 🟢 |
| 18 | `measure == 1` | Body Block1 | 🟢 |
| 19 | `measure contains Body` | Body Block1 | 🟢 |
| 20 | `measure-volume ~ 10.22278` | 10.22277375 | 🟢 |
| 21 | `measure-refreshes == 1` | 1 | 🟢 |
| 28 | `measure contains Face` | Face Extrude#4.Cap(End) | 🟢 |
| 29 | `measure-area ~ 0.7853982` | 0.7853981634 | 🟢 |
| 30 | `measure-length ~ 3.141593` | 3.141592654 | 🟢 |
| 37 | `measure contains Distance` | Distance Extrude#4.Cap(End) → Extrude#2.Side(Line1) | 🟢 |
| 38 | `measure-distance ~ 1.060660` | 1.060660172 | 🟢 |
| 39 | `measure-angle ~ 90` | 90 | 🟢 |
| 47 | `measure contains Edge` | Edge Edge(Extrude#4.Cap(End)/Extrude#4.Side(Circle1)) | 🟢 |
| 48 | `measure-length ~ 3.141593` | 3.141592654 | 🟢 |
| 51 | `measure-distance ~ 1.415097` | 1.41509717 | 🟢 |
| 57 | `measure == 0` | off | 🟢 |
| 65 | `section == 1` | y 1.25 | 🟢 |
| 66 | `section contains y 1.25` | y 1.25 | 🟢 |
| 67 | `section-clip == 1` | 1 | 🟢 |
| 68 | `section-area ~ 4.116587` | 4.116587385 | 🟢 |
| 69 | `section-regions == 2` | 2 | 🟢 |
| 70 | `section-bodies == 1` | 1 | 🟢 |
| 77 | `hover == section cap` | section cap | 🟢 |
| 80 | `selected-faces == 0` |  | 🟢 |
| 86 | `section contains x 2` | x 2 | 🟢 |
| 92 | `section contains x 1` | x 1 | 🟢 |
| 93 | `section-area ~ 3.25` | 3.25 | 🟢 |
| 94 | `section-regions == 1` | 1 | 🟢 |
| 100 | `section contains flipped` | x 1 flipped | 🟢 |
| 101 | `section-area ~ 3.25` | 3.25 | 🟢 |
| 107 | `section == 0` | off | 🟢 |
| 108 | `section-clip == 0` | 0 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 152MeasureBody | 11 | `e47dbe057286a24a` |
| 153MeasureFace | 23 | `fe120a7ac3dc6121` |
| 154MeasureDistance | 32 | `6a32a0967fe33167` |
| 155MeasureEdges | 51 | `974699b5f021ea2b` |
| 156SectionY | 66 | `41cb668eea6ea536` |
| 157SectionCapHover | 82 | `c605b85521ebd13b` |
| 158SectionX | 109 | `2f2d4dc6f2444d8e` |
| 159SectionFlipped | 118 | `187ccd199c90c1c2` |

## Journal

- Measure · Body Block1 · Volume 10.2228 m³
- Selected Extrude#4.Cap(End)
- Added Extrude#2.Side(Line1)
- Selected Edge(Extrude#4.Cap(End)|Extrude#4.Side(Circle1))
- Added Edge(Extrude#2.Cap(End)|Extrude#6.Side(Circle1))
- Measure off
- Selection cleared
- Section y = 1.25 · A 4.11659 m²
- Section x = 2 · A 2.5 m²
- Section x = 1
- Section x = 1 (flipped)
- Section off
