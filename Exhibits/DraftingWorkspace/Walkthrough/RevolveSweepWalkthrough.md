# DraftingWorkspaceCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0
- Frame: 1600 x 900 · 424 frames · 49.2908 s · slowest frame 1959.51 ms
- Script: Experimental/DraftingWorkspace/Scripts/RevolveSweepWalkthrough.txt
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
| 82 | `sweep == 0` | 0 | 🟢 |
| 83 | `sweep-commits == 1` | 1 | 🟢 |
| 84 | `bodies == 1` | 1 | 🟢 |
| 85 | `volume ~ 4.101094890` | 4.10109489 | 🟢 |
| 86 | `sound == 1` | 1 | 🟢 |
| 98 | `sketches == 2` | Line2,Rectangle1 | 🟢 |
| 101 | `sweep-axis contains y-axis` | sketch y-axis | 🟢 |
| 104 | `sweep-axis contains x-axis` | sketch x-axis | 🟢 |
| 107 | `sweep-axis contains y-axis` | sketch y-axis | 🟢 |
| 110 | `sweep == 1` | 1 | 🟢 |
| 111 | `sweep-axis contains Line` | Line4 | 🟢 |
| 114 | `sweep-amount ~ 90` | 90 | 🟢 |
| 122 | `sweep-commits == 2` | 2 | 🟢 |
| 123 | `bodies == 2` | 2 | 🟢 |
| 124 | `body-volumes contains Revolve2 0.056549` | Revolve1 4.101095 · Revolve2 0.056549 | 🟢 |
| 125 | `sound == 1` | 1 | 🟢 |
| 139 | `grid-plane == 1` | 1 | 🟢 |
| 162 | `sketches == 5` | Line2,Rectangle1,Circle1,Curve1,Line1 | 🟢 |
| 167 | `sweep == 1` | 1 | 🟢 |
| 168 | `sweep-verb == Evolution` | Evolution | 🟢 |
| 169 | `sweep-profile contains Sketch` | Sketch#7 | 🟢 |
| 170 | `sweep-preview == 1` | 1 | 🟢 |
| 176 | `sweep == 1` | 1 | 🟢 |
| 177 | `sweep-preview == 1` | 1 | 🟢 |
| 178 | `sweep-refusal == 0` |  | 🟢 |
| 182 | `sweep-commits == 3` | 3 | 🟢 |
| 183 | `bodies == 3` | 3 | 🟢 |
| 184 | `body-volumes contains Evolution1 0.319080` | Revolve1 4.101095 · Revolve2 0.056549 · Evolution1 0.319080 | 🟢 |
| 185 | `sound == 1` | 1 | 🟢 |
| 193 | `sweep == 1` | 1 | 🟢 |
| 194 | `sweep-verb == Evolution` | Evolution | 🟢 |
| 197 | `sweep == 0` | 0 | 🟢 |
| 198 | `bodies == 3` | 3 | 🟢 |
| 201 | `bodies == 2` | 2 | 🟢 |
| 204 | `bodies == 3` | 3 | 🟢 |
| 205 | `body-volumes contains Evolution1 0.319080` | Revolve1 4.101095 · Revolve2 0.056549 · Evolution1 0.319080 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 134RevolvePreview | 171 | `abceaedb1792e83c` |
| 135RevolvePreviewIsometric | 181 | `483628637bd2a305` |
| 136RevolveEdgeAxis | 258 | `7a681db76d6e338b` |
| 137EvolutionProposed | 376 | `e05117afaf89389d` |
| 138EvolutionPath | 386 | `0be61d0ea00ea69f` |
| 139Evolution | 400 | `5a29f4f5e5a1247e` |

## Journal

- New document (previous recovery copy kept)
- Line on plane XY: click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · Esc aborts
- Line1 · 0 ms
- Curve on plane XY: click points · 1 Hermite · 2 Bezier · 3 NURBS · 4 cubic spline · Enter / right-click finishes · Ctrl+Z drops a point · Esc aborts
- Curve form: Bezier
- Autosaved _ClaudeScratch/DraftingWorkspace/RevolveSweepRecovery.drafting.toml
- Curve1 · Bezier · 0 ms
- Line on plane XY: click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · Esc aborts
- Line2 · 0 ms
- Join (Line2, Line1, Curve1) · 0 ms
- Revolve Line2 about sketch y-axis · type degrees (360) · X / Y sketch axis · click one of its lines for the axis · Q union · W difference · E intersect · R new body · click / Enter commits · Esc cancels
- View Isometric
- Revolve1 · Line2 · 360° about sketch y-axis · New · 260 ms
- Rectangle on plane XY: click two corners · Alt centre mode · Esc aborts
- Rectangle1 · 264 ms
- Revolve Rectangle1 about sketch y-axis · type degrees (360) · X / Y sketch axis · click one of its lines for the axis · Q union · W difference · E intersect · R new body · click / Enter commits · Esc cancels
- Revolve about the sketch x-axis
- Revolve about the sketch y-axis
- Revolve about Line4
- Revolve: New body
- Autosaved _ClaudeScratch/DraftingWorkspace/RevolveSweepRecovery.drafting.toml
- View Isometric
- Revolve2 · Rectangle1 · 90° about Line4 · New body · 274 ms
- Circle on plane XY: click centre and rim · V two-point mode · Esc aborts
- Circle1 · 266 ms
- Curve on plane XZ: click points · 1 Hermite · 2 Bezier · 3 NURBS · 4 cubic spline · Enter / right-click finishes · Ctrl+Z drops a point · Esc aborts
- Curve form: Bezier
- Autosaved _ClaudeScratch/DraftingWorkspace/RevolveSweepRecovery.drafting.toml
- Curve1 · Bezier · 264 ms
- Line on plane XZ: click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · Esc aborts
- Line1 · 271 ms
- View Isometric
- Evolution Circle1 along Line1 · click another sketch's curve for the path · Q union · W difference · E intersect · R new body · click / Enter commits · Esc cancels
- Evolution along Curve1
- Evolution1 · Circle1 along Curve1 · New body · 318 ms
- Framed the document (Space)
- Evolution Circle1 along Line1 · click another sketch's curve for the path · Q union · W difference · E intersect · R new body · click / Enter commits · Esc cancels
- Evolution cancelled
- Undo
- Redo
