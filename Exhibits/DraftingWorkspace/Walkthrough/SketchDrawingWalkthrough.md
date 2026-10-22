# DraftingWorkspaceCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0
- Frame: 1600 x 900 · 339 frames · 30.7642 s · slowest frame 1001.44 ms
- Script: /home/user/Frontier/Experimental/DraftingWorkspace/Scripts/SketchDrawingWalkthrough.txt
- Expectations: 64 / 64 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 12 | `steps == 0` | 0 | 🟢 |
| 15 | `grid-plane == 0` | 0 | 🟢 |
| 20 | `drawing == Rectangle` | Rectangle | 🟢 |
| 21 | `drawing-plane == 0` | 0 | 🟢 |
| 22 | `drawing-centre == 0` | 0 | 🟢 |
| 27 | `drawing-points == 1` | 1 | 🟢 |
| 33 | `drawing == 0` |  | 🟢 |
| 34 | `steps == 1` | 1 | 🟢 |
| 35 | `sketches == Rectangle1` | Rectangle1 | 🟢 |
| 36 | `focused-caption == Rectangle1` | Rectangle1 | 🟢 |
| 37 | `sketch-curves == 4` | 4 | 🟢 |
| 42 | `bevel-active == 1` | 1 | 🟢 |
| 43 | `bevel-verb == 2` | 2 | 🟢 |
| 44 | `bevel-combination == New` | New | 🟢 |
| 50 | `bevel-size ~ 1.5` | 1.5 | 🟢 |
| 51 | `bevel-refusal == 0` |  | 🟢 |
| 52 | `bevel-preview-volume ~ 3` | 3 | 🟢 |
| 55 | `bevel-active == 0` | 0 | 🟢 |
| 56 | `steps == 2` | 2 | 🟢 |
| 57 | `volume ~ 3` | 3 | 🟢 |
| 58 | `faces == 6` | 6 | 🟢 |
| 59 | `sound == 1` | 1 | 🟢 |
| 64 | `drawing == Circle` | Circle | 🟢 |
| 65 | `drawing-centre == 1` | 1 | 🟢 |
| 70 | `steps == 3` | 3 | 🟢 |
| 71 | `focused-caption == Circle1` | Circle1 | 🟢 |
| 74 | `bevel-combination == Union` | Union | 🟢 |
| 77 | `bevel-combination == Difference` | Difference | 🟢 |
| 83 | `bevel-preview-volume ~ 2.2460177631` | 2.246017763 | 🟢 |
| 86 | `volume ~ 2.2460177631` | 2.246017763 | 🟢 |
| 87 | `genus == 1` | 1 | 🟢 |
| 88 | `sound == 1` | 1 | 🟢 |
| 93 | `drawing == Line` | Line | 🟢 |
| 100 | `drawing-points == 3` | 3 | 🟢 |
| 103 | `drawing == 0` |  | 🟢 |
| 104 | `sketches == Rectangle1,Circle1,Line1` | Rectangle1,Circle1,Line1 | 🟢 |
| 105 | `sketch-curves == 3` | 3 | 🟢 |
| 110 | `bevel-combination == Intersection` | Intersection | 🟢 |
| 113 | `bevel-combination == Union` | Union | 🟢 |
| 119 | `bevel-preview-volume ~ 2.5793510965` | 2.579351096 | 🟢 |
| 122 | `volume ~ 2.5793510965` | 2.579351096 | 🟢 |
| 123 | `sound == 1` | 1 | 🟢 |
| 129 | `drawing == Curve` | Curve | 🟢 |
| 130 | `drawing-form == Hermite` | Hermite | 🟢 |
| 137 | `drawing-points == 3` | 3 | 🟢 |
| 140 | `drawing-points == 2` | 2 | 🟢 |
| 147 | `drawing-form == Bezier` | Bezier | 🟢 |
| 150 | `drawing-form == NURBS` | NURBS | 🟢 |
| 153 | `drawing-form == Cubic spline` | Cubic spline | 🟢 |
| 157 | `drawing-form == Hermite` | Hermite | 🟢 |
| 163 | `drawing == 0` |  | 🟢 |
| 164 | `focused-caption == Curve1` | Curve1 | 🟢 |
| 165 | `sketch-curves == 1` | 1 | 🟢 |
| 175 | `bevel-refusal == 0` |  | 🟢 |
| 176 | `bevel-preview-volume ~ 2.4563510965` | 2.456351096 | 🟢 |
| 179 | `volume ~ 2.4563510965` | 2.456351096 | 🟢 |
| 180 | `sound == 1` | 1 | 🟢 |
| 181 | `refused == 0` | 0 | 🟢 |
| 186 | `drawing == Arc` | Arc | 🟢 |
| 193 | `drawing == Arc` | Arc | 🟢 |
| 194 | `drawing-refusal contains collinear` | Arc points are collinear | 🟢 |
| 197 | `drawing-points == 2` | 2 | 🟢 |
| 200 | `drawing == 0` |  | 🟢 |
| 201 | `steps == 8` | 8 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 30RectanglePreview | 32 | `548d570484437c40` |
| 31CurveForms | 243 | `13245b30d0522cf0` |
| 32HermiteClosing | 254 | `fcc71da8e1f3f5ac` |
| 33DrawnSolid | 339 | `26c15b3ddefcfde4` |

## Journal

- New document (previous recovery copy kept)
- Rectangle on plane XY: click two corners · Alt centre mode · Esc aborts
- Rectangle1 · 0 ms
- Extrude Rectangle1 (E): move horizontally · Q union · W difference · E intersect · R new body · Enter / right-click applies · Esc aborts
- Bevel size 1.5 m (pinned)
- Extrude1 · Rectangle1 · New · 9 ms
- Circle on plane XY: click centre and rim · V two-point mode · Esc aborts
- Circle1 · 6 ms
- Extrude Circle1 (E): move horizontally · Q union · W difference · E intersect · R new body · Enter / right-click applies · Esc aborts
- Extrude Boolean: Difference
- Bevel size 1.5 m (pinned)
- Extrude2 · Circle1 · Difference · 34 ms
- Line on plane XY: click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · Esc aborts
- Line1 · 15 ms
- Extrude Line1 (E): move horizontally · Q union · W difference · E intersect · R new body · Enter / right-click applies · Esc aborts
- Extrude Boolean: Intersection
- Extrude Boolean: Union
- Bevel size 1 m (pinned)
- Extrude3 · Line1 · Union · 51 ms
- Curve on plane XY: click points · 1 Hermite · 2 Bezier · 3 NURBS · 4 cubic spline · Enter / right-click finishes · Ctrl+Z drops a point · Esc aborts
- Point dropped · 2 left
- Autosaved _ClaudeScratch/DraftingWorkspace/SketchRecovery.drafting.toml
- Curve form: Bezier
- Curve form: NURBS
- Curve form: Cubic spline
- Curve form: Hermite
- Curve1 · Hermite · 19 ms
- Extrude Curve1 (E): move horizontally · Q union · W difference · E intersect · R new body · Enter / right-click applies · Esc aborts
- Extrude Boolean: Difference
- Bevel size 0.5 m (pinned)
- Extrude4 · Curve1 · Difference · 763 ms
- Arc on plane XY: click start, end, then a point on the arc · Esc aborts
- Arc points are collinear
- Point dropped · 2 left
- Arc aborted (document untouched)
- Autosaved _ClaudeScratch/DraftingWorkspace/SketchRecovery.drafting.toml
- View Isometric
