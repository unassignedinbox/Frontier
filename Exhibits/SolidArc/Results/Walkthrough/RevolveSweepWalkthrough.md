# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 900 · 445 frames · 42.397 s · slowest frame 1538.96 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/RevolveSweepWalkthrough.txt
- Expectations: 47 / 47 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 16 | `steps == 0` | 0 | 🟢 |
| 39 | `drawing-form == Bezier` | Bezier | 🟢 |
| 60 | `sketches == 3` | Line1,Curve1,Line2 | 🟢 |
| 63 | `sketches == 1` | Line2 | 🟢 |
| 64 | `sketch-curves == 5` | 5 | 🟢 |
| 70 | `sweep == 1` | 1 | 🟢 |
| 71 | `sweep-verb == Revolve` | Revolve | 🟢 |
| 72 | `sweep-axis contains y-axis` | sketch y-axis | 🟢 |
| 73 | `sweep-amount ~ 360` | 360 | 🟢 |
| 74 | `sweep-preview == 1` | 1 | 🟢 |
| 75 | `sweep-refusal == 0` |  | 🟢 |
| 84 | `sweep == 0` | 0 | 🟢 |
| 85 | `sweep-commits == 1` | 1 | 🟢 |
| 86 | `bodies == 1` | 1 | 🟢 |
| 87 | `volume ~ 4.101094890` | 4.10109489 | 🟢 |
| 88 | `sound == 1` | 1 | 🟢 |
| 100 | `sketches == 2` | Line2,Rectangle1 | 🟢 |
| 103 | `sweep-axis contains y-axis` | sketch y-axis | 🟢 |
| 106 | `sweep-axis contains x-axis` | sketch x-axis | 🟢 |
| 109 | `sweep-axis contains y-axis` | sketch y-axis | 🟢 |
| 112 | `sweep == 1` | 1 | 🟢 |
| 113 | `sweep-axis contains Line` | Line4 | 🟢 |
| 116 | `sweep-amount ~ 90` | 90 | 🟢 |
| 126 | `sweep-commits == 2` | 2 | 🟢 |
| 127 | `bodies == 2` | 2 | 🟢 |
| 128 | `body-volumes contains Revolve2 0.056549` | Revolve1 4.101095 · Revolve2 0.056549 | 🟢 |
| 129 | `sound == 1` | 1 | 🟢 |
| 143 | `grid-plane == 1` | 1 | 🟢 |
| 166 | `sketches == 5` | Line2,Rectangle1,Circle1,Curve1,Line1 | 🟢 |
| 173 | `sweep == 1` | 1 | 🟢 |
| 174 | `sweep-verb == Evolution` | Evolution | 🟢 |
| 175 | `sweep-profile contains Sketch` | Sketch#7 | 🟢 |
| 176 | `sweep-preview == 1` | 1 | 🟢 |
| 182 | `sweep == 1` | 1 | 🟢 |
| 183 | `sweep-preview == 1` | 1 | 🟢 |
| 184 | `sweep-refusal == 0` |  | 🟢 |
| 188 | `sweep-commits == 3` | 3 | 🟢 |
| 189 | `bodies == 3` | 3 | 🟢 |
| 190 | `body-volumes contains Evolution1 0.319080` | Revolve1 4.101095 · Revolve2 0.056549 · Evolution1 0.319080 | 🟢 |
| 191 | `sound == 1` | 1 | 🟢 |
| 199 | `sweep == 1` | 1 | 🟢 |
| 200 | `sweep-verb == Evolution` | Evolution | 🟢 |
| 203 | `sweep == 0` | 0 | 🟢 |
| 204 | `bodies == 3` | 3 | 🟢 |
| 207 | `bodies == 2` | 2 | 🟢 |
| 210 | `bodies == 3` | 3 | 🟢 |
| 211 | `body-volumes contains Evolution1 0.319080` | Revolve1 4.101095 · Revolve2 0.056549 · Evolution1 0.319080 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 134RevolvePreview | 171 | `90c70f4e9c9fe3c8` |
| 135RevolvePreviewIsometric | 188 | `055585ea78d3d36d` |
| 136RevolveEdgeAxis | 272 | `8d68c76f3a680766` |
| 137EvolutionProposed | 397 | `973cc530cd06fc1a` |
| 138EvolutionPath | 407 | `d853b2a636573238` |
| 139Evolution | 421 | `fc30ab3df817a748` |

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
- Revolve1 · Line2 · 360° about sketch y-axis · New · 292 ms
- Rectangle on plane XY (or the body face under the first click): click two corners · Alt centre mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Rectangle1 · 202 ms
- Revolve Rectangle1 about sketch y-axis · type degrees (360) · X / Y sketch axis · click one of its lines for the axis · Q union · W difference · E intersect · R new body · click / Enter commits · Esc cancels
- Revolve about the sketch x-axis
- Revolve about the sketch y-axis
- Revolve about Line4
- Revolve: New body
- Autosaved _ClaudeScratch/SolidArcWorkspace/RevolveSweepRecovery.solidarc
- View Isometric
- Revolve2 · Rectangle1 · 90° about Line4 · New body · 192 ms
- Circle on plane XY (or the body face under the first click): click centre and rim · V two-point mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Circle1 · 230 ms
- Curve on plane XZ (or the body face under the first click): click points · 1 Hermite · 2 Bezier · 3 NURBS · 4 cubic spline · Enter / right-click finishes · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Curve form: Bezier
- Autosaved _ClaudeScratch/SolidArcWorkspace/RevolveSweepRecovery.solidarc
- Curve1 · Bezier · 185 ms
- Line on plane XZ (or the body face under the first click): click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Line1 · 218 ms
- View Isometric
- Evolution Circle1 along Line1 · click another sketch's curve for the path · Q union · W difference · E intersect · R new body · click / Enter commits · Esc cancels
- Evolution along Curve1
- Evolution1 · Circle1 along Curve1 · New body · 275 ms
- Framed the document (Space)
- Evolution Circle1 along Line1 · click another sketch's curve for the path · Q union · W difference · E intersect · R new body · click / Enter commits · Esc cancels
- Evolution cancelled
- Undo
- Redo
