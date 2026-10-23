# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0
- Frame: 1600 x 900 · 120 frames · 20.4815 s · slowest frame 718.142 ms
- Script: /home/user/Frontier/Exhibits/SolidArc/Code/Scripts/MeasureSectionWalkthrough.txt
- Expectations: 32 / 32 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 11 | `steps == 8` | 8 | 🟢 |
| 12 | `measure == 0` | off | 🟢 |
| 15 | `measure == 1` | Body Block1 | 🟢 |
| 16 | `measure contains Body` | Body Block1 | 🟢 |
| 17 | `measure-volume ~ 10.22278` | 10.22277375 | 🟢 |
| 18 | `measure-refreshes == 1` | 1 | 🟢 |
| 25 | `measure contains Face` | Face Extrude#4.Cap(End) | 🟢 |
| 26 | `measure-area ~ 0.7853982` | 0.7853981634 | 🟢 |
| 27 | `measure-length ~ 3.141593` | 3.141592654 | 🟢 |
| 34 | `measure contains Distance` | Distance Extrude#4.Cap(End) → Extrude#2.Side(Line1) | 🟢 |
| 35 | `measure-distance ~ 1.060660` | 1.060660172 | 🟢 |
| 36 | `measure-angle ~ 90` | 90 | 🟢 |
| 44 | `measure contains Edge` | Edge Edge(Extrude#4.Cap(End)/Extrude#4.Side(Circle1)) | 🟢 |
| 45 | `measure-length ~ 3.141593` | 3.141592654 | 🟢 |
| 48 | `measure-distance ~ 1.415097` | 1.41509717 | 🟢 |
| 54 | `measure == 0` | off | 🟢 |
| 62 | `section == 1` | y 1.25 | 🟢 |
| 63 | `section contains y 1.25` | y 1.25 | 🟢 |
| 64 | `section-clip == 1` | 1 | 🟢 |
| 65 | `section-area ~ 4.116587` | 4.116587385 | 🟢 |
| 66 | `section-regions == 2` | 2 | 🟢 |
| 67 | `section-bodies == 1` | 1 | 🟢 |
| 74 | `hover == section cap` | section cap | 🟢 |
| 77 | `selected-faces == 0` |  | 🟢 |
| 83 | `section contains x 2` | x 2 | 🟢 |
| 89 | `section contains x 1` | x 1 | 🟢 |
| 90 | `section-area ~ 3.25` | 3.25 | 🟢 |
| 91 | `section-regions == 1` | 1 | 🟢 |
| 97 | `section contains flipped` | x 1 flipped | 🟢 |
| 98 | `section-area ~ 3.25` | 3.25 | 🟢 |
| 104 | `section == 0` | off | 🟢 |
| 105 | `section-clip == 0` | 0 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 152MeasureBody | 10 | `78d7c533bf7746d0` |
| 153MeasureFace | 22 | `539fa86dd698d519` |
| 154MeasureDistance | 31 | `a0c81f9a753d9bd3` |
| 155MeasureEdges | 50 | `48406e6d41ba0879` |
| 156SectionY | 65 | `baeeb48238a5af2f` |
| 157SectionCapHover | 81 | `409eb5e5066948ae` |
| 158SectionX | 105 | `605624fdd68a4d1b` |
| 159SectionFlipped | 114 | `b224225d48aeb3a9` |

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
