# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 900 · 446 frames · 58.174 s · slowest frame 2010.02 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/RevolveSweepWalkthrough.txt
- Expectations: 47 / 47 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 19 | `steps == 0` | 0 | 🟢 |
| 42 | `drawing-form == Bezier` | Bezier | 🟢 |
| 63 | `sketches == 3` | Line1,Curve1,Line2 | 🟢 |
| 66 | `sketches == 1` | Line2 | 🟢 |
| 67 | `sketch-curves == 5` | 5 | 🟢 |
| 73 | `sweep == 1` | 1 | 🟢 |
| 74 | `sweep-verb == Revolve` | Revolve | 🟢 |
| 75 | `sweep-axis contains y-axis` | sketch y-axis | 🟢 |
| 76 | `sweep-amount ~ 360` | 360 | 🟢 |
| 77 | `sweep-preview == 1` | 1 | 🟢 |
| 78 | `sweep-refusal == 0` |  | 🟢 |
| 87 | `sweep == 0` | 0 | 🟢 |
| 88 | `sweep-commits == 1` | 1 | 🟢 |
| 89 | `bodies == 1` | 1 | 🟢 |
| 90 | `volume ~ 4.101094890` | 4.10109489 | 🟢 |
| 91 | `sound == 1` | 1 | 🟢 |
| 103 | `sketches == 2` | Line2,Rectangle1 | 🟢 |
| 106 | `sweep-axis contains y-axis` | sketch y-axis | 🟢 |
| 109 | `sweep-axis contains x-axis` | sketch x-axis | 🟢 |
| 112 | `sweep-axis contains y-axis` | sketch y-axis | 🟢 |
| 115 | `sweep == 1` | 1 | 🟢 |
| 116 | `sweep-axis contains Line` | Line4 | 🟢 |
| 119 | `sweep-amount ~ 90` | 90 | 🟢 |
| 129 | `sweep-commits == 2` | 2 | 🟢 |
| 130 | `bodies == 2` | 2 | 🟢 |
| 131 | `body-volumes contains Revolve2 0.056549` | Revolve1 4.101095 · Revolve2 0.056549 | 🟢 |
| 132 | `sound == 1` | 1 | 🟢 |
| 146 | `grid-plane == 1` | 1 | 🟢 |
| 169 | `sketches == 5` | Line2,Rectangle1,Circle1,Curve1,Line1 | 🟢 |
| 176 | `sweep == 1` | 1 | 🟢 |
| 177 | `sweep-verb == Evolution` | Evolution | 🟢 |
| 178 | `sweep-profile contains Sketch` | Sketch#7 | 🟢 |
| 179 | `sweep-preview == 1` | 1 | 🟢 |
| 185 | `sweep == 1` | 1 | 🟢 |
| 186 | `sweep-preview == 1` | 1 | 🟢 |
| 187 | `sweep-refusal == 0` |  | 🟢 |
| 191 | `sweep-commits == 3` | 3 | 🟢 |
| 192 | `bodies == 3` | 3 | 🟢 |
| 193 | `body-volumes contains Evolution1 0.319080` | Revolve1 4.101095 · Revolve2 0.056549 · Evolution1 0.319080 | 🟢 |
| 194 | `sound == 1` | 1 | 🟢 |
| 202 | `sweep == 1` | 1 | 🟢 |
| 203 | `sweep-verb == Evolution` | Evolution | 🟢 |
| 206 | `sweep == 0` | 0 | 🟢 |
| 207 | `bodies == 3` | 3 | 🟢 |
| 210 | `bodies == 2` | 2 | 🟢 |
| 213 | `bodies == 3` | 3 | 🟢 |
| 214 | `body-volumes contains Evolution1 0.319080` | Revolve1 4.101095 · Revolve2 0.056549 · Evolution1 0.319080 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 134RevolvePreview | 172 | `a9a030e320d052b6` |
| 135RevolvePreviewIsometric | 189 | `cb48e448efd26757` |
| 136RevolveEdgeAxis | 273 | `5b6a14c31d0c3498` |
| 137EvolutionProposed | 398 | `57d9cbda00a58941` |
| 138EvolutionPath | 408 | `ac48ba8f404eb321` |
| 139Evolution | 422 | `7672f1cb780fdd85` |

## Journal

- New document (previous recovery copy kept)
- Line on plane XY (or the body face under the first click): click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Line1 · 0 ms
- Curve on plane XY (or the body face under the first click): click points · 1 Hermite · 2 Bezier · 3 NURBS · 4 cubic spline · Enter / right-click finishes · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Curve form: Bezier
- Autosaved _ClaudeScratch/SolidArcWorkspace/RevolveSweepRecovery.solidarc
- Curve1 · Bezier · 0 ms
- Line on plane XY (or the body face under the first click): click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Line2 · 0 ms
- Join (Line2, Line1, Curve1) · 0 ms
- Revolve Line2 about sketch y-axis · type degrees (360) · X / Y sketch axis · click one of its lines for the axis · Q union · W difference · E intersect · R new body · click / Enter commits · Esc cancels
- View Isometric
- Revolve1 · Line2 · 360° about sketch y-axis · New · 304 ms
- Rectangle on plane XY (or the body face under the first click): click two corners · Alt centre mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Rectangle1 · 155 ms
- Revolve Rectangle1 about sketch y-axis · type degrees (360) · X / Y sketch axis · click one of its lines for the axis · Q union · W difference · E intersect · R new body · click / Enter commits · Esc cancels
- Revolve about the sketch x-axis
- Revolve about the sketch y-axis
- Revolve about Line4
- Revolve: New body
- Autosaved _ClaudeScratch/SolidArcWorkspace/RevolveSweepRecovery.solidarc
- View Isometric
- Revolve2 · Rectangle1 · 90° about Line4 · New body · 154 ms
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Circle1 · 153 ms
- Curve on plane XZ (or the body face under the first click): click points · 1 Hermite · 2 Bezier · 3 NURBS · 4 cubic spline · Enter / right-click finishes · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Curve form: Bezier
- Autosaved _ClaudeScratch/SolidArcWorkspace/RevolveSweepRecovery.solidarc
- Curve1 · Bezier · 157 ms
- Line on plane XZ (or the body face under the first click): click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Line1 · 155 ms
- View Isometric
- Evolution Circle1 along Line1 · click another sketch's curve for the path · Q union · W difference · E intersect · R new body · click / Enter commits · Esc cancels
- Evolution along Curve1
- Evolution1 · Circle1 along Curve1 · New body · 196 ms
- Framed the document (Space)
- Evolution Circle1 along Line1 · click another sketch's curve for the path · Q union · W difference · E intersect · R new body · click / Enter commits · Esc cancels
- Evolution cancelled
- Undo
- Redo
