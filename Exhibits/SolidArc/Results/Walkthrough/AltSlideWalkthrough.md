# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 900 · 125 frames · 11.1515 s · slowest frame 528.719 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/AltSlideWalkthrough.txt
- Expectations: 25 / 25 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 21 | `sketches == 1` | Circle1 | 🟢 |
| 31 | `snap contains none` | none | 🟢 |
| 32 | `drawing-pointer-x ~ 0.6` | 0.6 | 🟢 |
| 33 | `drawing-pointer-y ~ 0.8` | 0.8 | 🟢 |
| 36 | `snap contains none` | none | 🟢 |
| 39 | `snap contains curve` | On curve · Circle1 | 🟢 |
| 40 | `snap-x > 0.600` | 0.6054546033 | 🟢 |
| 41 | `snap-x < 0.625` | 0.6054546033 | 🟢 |
| 42 | `snap-y > 0.780` | 0.7958798423 | 🟢 |
| 43 | `snap-y < 0.800` | 0.7958798423 | 🟢 |
| 47 | `drawing-points == 2` | 2 | 🟢 |
| 48 | `drawing-last-x > 0.600` | 0.6054546033 | 🟢 |
| 49 | `drawing-last-x < 0.625` | 0.6054546033 | 🟢 |
| 52 | `sketches == 2` | Circle1,Line1 | 🟢 |
| 59 | `drawing-centre == 1` | 1 | 🟢 |
| 62 | `snap contains curve` | On curve · Circle1 | 🟢 |
| 63 | `snap-x > 0.9999` | 0.9999884432 | 🟢 |
| 64 | `drawing-centre == 1` | 1 | 🟢 |
| 67 | `drawing-centre == 0` | 0 | 🟢 |
| 83 | `snap contains curve` | On curve · Circle1 | 🟢 |
| 87 | `point-commits == 1` | 1 | 🟢 |
| 88 | `picked-point-x > -0.500` | -0.4909466481 | 🟢 |
| 89 | `picked-point-x < -0.475` | -0.4909466481 | 🟢 |
| 90 | `picked-point-y > -0.885` | -0.8711896399 | 🟢 |
| 91 | `picked-point-y < -0.865` | -0.8711896399 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 243AltSlideCircle | 57 | `f622ff0955d3dc2b` |
| 244RoundHandles | 102 | `ed4549b3a9048c53` |
| 245AltSlidePoint | 119 | `9659abd979b6c3d8` |

## Journal

- New document (previous recovery copy kept)
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Circle1 · 0 ms
- Line on plane XY (or the body face under the first click): click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Autosaved _ClaudeScratch/SolidArcWorkspace/AltSlideRecovery.solidarc
- Line1 · 0 ms
- Rectangle on plane XY (or the body face under the first click): click two corners · Alt centre mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Rectangle: centre mode (first click is the centre)
- Rectangle: corner mode
- Rectangle aborted (document untouched)
- Autosaved _ClaudeScratch/SolidArcWorkspace/AltSlideRecovery.solidarc
- Drag Line1.Start of Line1 · 0 ms
- Drag Line1.Start of Line1 · 0 ms
