# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0
- Frame: 1600 x 900 · 361 frames · 912.591 s · slowest frame 158235 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/NurbsBlendWalkthrough.txt
- Expectations: 44 / 44 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 13 | `steps == 0` | 0 | 🟢 |
| 18 | `drawing-form == Hermite` | Hermite | 🟢 |
| 35 | `drawing == 0` |  | 🟢 |
| 36 | `sketch-curves == 1` | 1 | 🟢 |
| 39 | `sketch-filled == 1` | 1 | 🟢 |
| 49 | `steps == 2` | 2 | 🟢 |
| 50 | `bodies == 1` | 1 | 🟢 |
| 51 | `sound == 1` | 1 | 🟢 |
| 52 | `refused == 0` | 0 | 🟢 |
| 65 | `focused-caption == Line1` | Line1 | 🟢 |
| 68 | `refused == 0` | 0 | 🟢 |
| 69 | `bodies == 2` | 2 | 🟢 |
| 70 | `selected-bodies contains Cut#` | Cut#4 | 🟢 |
| 73 | `bodies == 1` | 1 | 🟢 |
| 74 | `sound == 1` | 1 | 🟢 |
| 93 | `selected-edges == 1` | Edge(Extrude#2.Cap(End)/Extrude#2.Side(Spline1)) | 🟢 |
| 96 | `bevel-active == 1` | 1 | 🟢 |
| 97 | `bevel-variable == 0` | 0 | 🟢 |
| 100 | `bevel-chamfer == 1` | 1 | 🟢 |
| 101 | `bevel-size ~ 0.25` | 0.25 | 🟢 |
| 102 | `bevel-refusal == 0` |  | 🟢 |
| 103 | `bevel-preview-volume ~ 2.050097333727` | 2.050097334 | 🟢 |
| 110 | `bevel-refusal == 0` |  | 🟢 |
| 111 | `bevel-preview-volume ~ 2.118687354516` | 2.118687355 | 🟢 |
| 114 | `bevel-active == 0` | 0 | 🟢 |
| 115 | `refused == 0` | 0 | 🟢 |
| 116 | `sound == 1` | 1 | 🟢 |
| 117 | `volume ~ 2.118687354516` | 2.118687355 | 🟢 |
| 129 | `selected-edges == 1` | Edge(Extrude#2.Cap(End)/Extrude#2.Side(Spline1)) | 🟢 |
| 134 | `bevel-chamfer == 0` | 0 | 🟢 |
| 140 | `bevel-refusal == 0` |  | 🟢 |
| 143 | `bevel-variable == 1` | 1 | 🟢 |
| 149 | `bevel-end-size ~ 0.25` | 0.25 | 🟢 |
| 150 | `bevel-refusal == 0` |  | 🟢 |
| 154 | `bevel-active == 0` | 0 | 🟢 |
| 155 | `refused == 0` | 0 | 🟢 |
| 156 | `sound == 1` | 1 | 🟢 |
| 157 | `status contains 0.1 → 0.25` | Fillet 0.1 → 0.25 on 1 edge(s) · 149876 ms | 🟢 |
| 158 | `volume ~ 2.133847116` | 2.133847116 | 🟢 |
| 172 | `bevel-variable == 0` | 0 | 🟢 |
| 180 | `bevel-refusal contains curvature radius` | Blend#6: Blend: E37: size exceeds the curvature radius of the NURBS support (fold margi... | 🟢 |
| 184 | `bevel-active == 0` | 0 | 🟢 |
| 185 | `sound == 1` | 1 | 🟢 |
| 186 | `refused == 0` | 0 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 173NurbsBlendCutBody | 161 | `342aef6d4477fc41` |
| 174NurbsChamfer025 | 196 | `8f8e8762cb442e0e` |
| 175NurbsChamferApplied | 220 | `589362deb4e7f35a` |
| 176NurbsVariablePreview | 294 | `2283b821c1935157` |
| 177NurbsVariableApplied | 302 | `9c03eb79ab604f85` |
| 178NurbsChamferRefused | 354 | `5778c44641869546` |

## Journal

- New document (previous recovery copy kept)
- Curve on plane XY (or the body face under the first click): click points · 1 Hermite · 2 Bezier · 3 NURBS · 4 cubic spline · Enter / right-click finishes · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Curve1 · Hermite · 0 ms
- Fill Curve1 · 0 ms
- Extrude Curve1 (E): move horizontally · Q union · W difference · E intersect · R new body · Enter / right-click applies · Esc aborts
- Bevel size 1 m (pinned)
- Extrude1 · Curve1 · New · 45 ms
- Selection cleared
- Line on plane XY (or the body face under the first click): click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Line1 · 27 ms
- Cut (Extrude1 by Line1) · 1293 ms
- Remove (Cut1) · 35 ms
- View Isometric
- Framed the document (Space)
- Selection cleared
- Autosaved _ClaudeScratch/SolidArcWorkspace/NurbsBlendRecovery.solidarc
- Selected Edge(Extrude#2.Cap(End)|Extrude#2.Side(Spline1))
- Fillet (B): move horizontally · D fillet · C chamfer · V variable size · F flip its anchor · click edits the edges · Enter / right-click applies · Esc aborts
- Bevel size 0.15 m (pinned)
- Chamfer 0.15 on 1 edge(s) · 68442 ms
- Undo
- Selected Edge(Extrude#2.Cap(End)|Extrude#2.Side(Spline1))
- Fillet (B): move horizontally · D fillet · C chamfer · V variable size · F flip its anchor · click edits the edges · Enter / right-click applies · Esc aborts
- Bevel size 0.1 m (pinned)
- Variable size: 0.1 m at the anchor corner → 0.05 m (F flips the anchor)
- Bevel end size 0.25 m
- Fillet 0.1 → 0.25 on 1 edge(s) · 149876 ms
- Undo
- Selected Edge(Extrude#2.Cap(End)|Extrude#2.Side(Spline1))
- Fillet (B): move horizontally · D fillet · C chamfer · V variable size · F flip its anchor · click edits the edges · Enter / right-click applies · Esc aborts
- Bevel size 0.9 m (pinned)
- Bevel cancelled (document untouched)
