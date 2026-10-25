# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0
- Frame: 1600 x 900 · 356 frames · 987.204 s · slowest frame 174725 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/NurbsBlendWalkthrough.txt
- Expectations: 43 / 43 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 13 | `steps == 0` | 0 | 🟢 |
| 18 | `drawing-form == Hermite` | Hermite | 🟢 |
| 35 | `drawing == 0` |  | 🟢 |
| 36 | `sketch-curves == 1` | 1 | 🟢 |
| 46 | `steps == 2` | 2 | 🟢 |
| 47 | `bodies == 1` | 1 | 🟢 |
| 48 | `sound == 1` | 1 | 🟢 |
| 49 | `refused == 0` | 0 | 🟢 |
| 62 | `focused-caption == Line1` | Line1 | 🟢 |
| 65 | `refused == 0` | 0 | 🟢 |
| 66 | `bodies == 2` | 2 | 🟢 |
| 67 | `selected-bodies contains Cut#` | Cut#4 | 🟢 |
| 70 | `bodies == 1` | 1 | 🟢 |
| 71 | `sound == 1` | 1 | 🟢 |
| 90 | `selected-edges == 1` | Edge(Extrude#2.Cap(End)/Extrude#2.Side(Spline1)) | 🟢 |
| 93 | `bevel-active == 1` | 1 | 🟢 |
| 94 | `bevel-variable == 0` | 0 | 🟢 |
| 97 | `bevel-chamfer == 1` | 1 | 🟢 |
| 98 | `bevel-size ~ 0.25` | 0.25 | 🟢 |
| 99 | `bevel-refusal == 0` |  | 🟢 |
| 100 | `bevel-preview-volume ~ 2.050097333727` | 2.050097334 | 🟢 |
| 107 | `bevel-refusal == 0` |  | 🟢 |
| 108 | `bevel-preview-volume ~ 2.118687354516` | 2.118687355 | 🟢 |
| 111 | `bevel-active == 0` | 0 | 🟢 |
| 112 | `refused == 0` | 0 | 🟢 |
| 113 | `sound == 1` | 1 | 🟢 |
| 114 | `volume ~ 2.118687354516` | 2.118687355 | 🟢 |
| 126 | `selected-edges == 1` | Edge(Extrude#2.Cap(End)/Extrude#2.Side(Spline1)) | 🟢 |
| 131 | `bevel-chamfer == 0` | 0 | 🟢 |
| 137 | `bevel-refusal == 0` |  | 🟢 |
| 140 | `bevel-variable == 1` | 1 | 🟢 |
| 146 | `bevel-end-size ~ 0.25` | 0.25 | 🟢 |
| 147 | `bevel-refusal == 0` |  | 🟢 |
| 151 | `bevel-active == 0` | 0 | 🟢 |
| 152 | `refused == 0` | 0 | 🟢 |
| 153 | `sound == 1` | 1 | 🟢 |
| 154 | `status contains 0.1 → 0.25` | Fillet 0.1 → 0.25 on 1 edge(s) · 158518 ms | 🟢 |
| 155 | `volume ~ 2.133847116` | 2.133847116 | 🟢 |
| 169 | `bevel-variable == 0` | 0 | 🟢 |
| 177 | `bevel-refusal contains curvature radius` | Blend#6: Blend: E37: size exceeds the curvature radius of the NURBS support (fold margi... | 🟢 |
| 181 | `bevel-active == 0` | 0 | 🟢 |
| 182 | `sound == 1` | 1 | 🟢 |
| 183 | `refused == 0` | 0 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 173NurbsBlendCutBody | 156 | `13611ace2e6b0bd0` |
| 174NurbsChamfer025 | 191 | `10cd32d94c11301d` |
| 175NurbsChamferApplied | 215 | `9d9f45878d3761e1` |
| 176NurbsVariablePreview | 289 | `b5fe6131f97d8e78` |
| 177NurbsVariableApplied | 297 | `0ddd39d8d2843d33` |
| 178NurbsChamferRefused | 349 | `473ca855e06d88f7` |

## Journal

- New document (previous recovery copy kept)
- Curve on plane XY (or the body face under the first click): click points · 1 Hermite · 2 Bezier · 3 NURBS · 4 cubic spline · Enter / right-click finishes · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres · Esc aborts
- Curve1 · Hermite · 0 ms
- Extrude Curve1 (E): move horizontally · Q union · W difference · E intersect · R new body · Enter / right-click applies · Esc aborts
- Bevel size 1 m (pinned)
- Extrude1 · Curve1 · New · 44 ms
- Selection cleared
- Line on plane XY (or the body face under the first click): click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres · Esc aborts
- Line1 · 30 ms
- Cut (Extrude1 by Line1) · 1388 ms
- Remove (Cut1) · 36 ms
- View Isometric
- Framed the document (Space)
- Selection cleared
- Autosaved _ClaudeScratch/SolidArcWorkspace/NurbsBlendRecovery.solidarc
- Selected Edge(Extrude#2.Cap(End)|Extrude#2.Side(Spline1))
- Fillet (B): move horizontally · D fillet · C chamfer · V variable size · F flip its anchor · click edits the edges · Enter / right-click applies · Esc aborts
- Bevel size 0.15 m (pinned)
- Chamfer 0.15 on 1 edge(s) · 76140 ms
- Undo
- Selected Edge(Extrude#2.Cap(End)|Extrude#2.Side(Spline1))
- Fillet (B): move horizontally · D fillet · C chamfer · V variable size · F flip its anchor · click edits the edges · Enter / right-click applies · Esc aborts
- Bevel size 0.1 m (pinned)
- Variable size: 0.1 m at the anchor corner → 0.05 m (F flips the anchor)
- Bevel end size 0.25 m
- Fillet 0.1 → 0.25 on 1 edge(s) · 158518 ms
- Undo
- Selected Edge(Extrude#2.Cap(End)|Extrude#2.Side(Spline1))
- Fillet (B): move horizontally · D fillet · C chamfer · V variable size · F flip its anchor · click edits the edges · Enter / right-click applies · Esc aborts
- Bevel size 0.9 m (pinned)
- Bevel cancelled (document untouched)
