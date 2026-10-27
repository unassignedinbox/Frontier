# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 900 · 126 frames · 11.5517 s · slowest frame 527.205 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/AltSlideWalkthrough.txt
- Expectations: 25 / 25 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 24 | `sketches == 1` | Circle1 | 🟢 |
| 34 | `snap contains none` | none | 🟢 |
| 35 | `drawing-pointer-x ~ 0.6` | 0.6 | 🟢 |
| 36 | `drawing-pointer-y ~ 0.8` | 0.8 | 🟢 |
| 39 | `snap contains On curve` | On curve · Circle1 | 🟢 |
| 42 | `snap contains curve` | On curve · Circle1 | 🟢 |
| 43 | `snap-x > 0.600` | 0.6054546033 | 🟢 |
| 44 | `snap-x < 0.625` | 0.6054546033 | 🟢 |
| 45 | `snap-y > 0.780` | 0.7958798423 | 🟢 |
| 46 | `snap-y < 0.800` | 0.7958798423 | 🟢 |
| 50 | `drawing-points == 2` | 2 | 🟢 |
| 51 | `drawing-last-x > 0.600` | 0.6054546033 | 🟢 |
| 52 | `drawing-last-x < 0.625` | 0.6054546033 | 🟢 |
| 55 | `sketches == 2` | Circle1,Line1 | 🟢 |
| 62 | `drawing-centre == 1` | 1 | 🟢 |
| 65 | `snap contains curve` | On curve · Circle1 | 🟢 |
| 66 | `snap-x > 0.9999` | 0.9999884432 | 🟢 |
| 67 | `drawing-centre == 1` | 1 | 🟢 |
| 70 | `drawing-centre == 0` | 0 | 🟢 |
| 86 | `snap contains curve` | On curve · Circle1 | 🟢 |
| 90 | `point-commits == 1` | 1 | 🟢 |
| 91 | `picked-point-x > -0.500` | -0.4909466481 | 🟢 |
| 92 | `picked-point-x < -0.475` | -0.4909466481 | 🟢 |
| 93 | `picked-point-y > -0.885` | -0.8711896399 | 🟢 |
| 94 | `picked-point-y < -0.865` | -0.8711896399 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 243AltSlideCircle | 58 | `72f53c9d5f08b957` |
| 244RoundHandles | 103 | `813ed85156569a51` |
| 245AltSlidePoint | 120 | `7204580d03dbf617` |

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
