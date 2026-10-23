# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0
- Frame: 1600 x 900 · 349 frames · 1056.31 s · slowest frame 162139 ms
- Script: /home/user/Frontier/Exhibits/SolidArc/Code/Scripts/NurbsBlendWalkthrough.txt
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
| 88 | `selected-edges == 1` | Edge(Extrude#2.Cap(End)/Extrude#2.Side(Spline1)) | 🟢 |
| 91 | `bevel-active == 1` | 1 | 🟢 |
| 92 | `bevel-variable == 0` | 0 | 🟢 |
| 95 | `bevel-chamfer == 1` | 1 | 🟢 |
| 96 | `bevel-size ~ 0.25` | 0.25 | 🟢 |
| 97 | `bevel-refusal == 0` |  | 🟢 |
| 98 | `bevel-preview-volume ~ 2.050097333727` | 2.050097334 | 🟢 |
| 105 | `bevel-refusal == 0` |  | 🟢 |
| 106 | `bevel-preview-volume ~ 2.118687354516` | 2.118687355 | 🟢 |
| 109 | `bevel-active == 0` | 0 | 🟢 |
| 110 | `refused == 0` | 0 | 🟢 |
| 111 | `sound == 1` | 1 | 🟢 |
| 112 | `volume ~ 2.118687354516` | 2.118687355 | 🟢 |
| 124 | `selected-edges == 1` | Edge(Extrude#2.Cap(End)/Extrude#2.Side(Spline1)) | 🟢 |
| 129 | `bevel-chamfer == 0` | 0 | 🟢 |
| 135 | `bevel-refusal == 0` |  | 🟢 |
| 138 | `bevel-variable == 1` | 1 | 🟢 |
| 144 | `bevel-end-size ~ 0.25` | 0.25 | 🟢 |
| 145 | `bevel-refusal == 0` |  | 🟢 |
| 149 | `bevel-active == 0` | 0 | 🟢 |
| 150 | `refused == 0` | 0 | 🟢 |
| 151 | `sound == 1` | 1 | 🟢 |
| 152 | `status contains 0.1 → 0.25` | Fillet 0.1 → 0.25 on 1 edge(s) · 153853 ms | 🟢 |
| 153 | `volume ~ 2.133847116` | 2.133847116 | 🟢 |
| 167 | `bevel-variable == 0` | 0 | 🟢 |
| 175 | `bevel-refusal contains curvature radius` | Blend#6: Blend: E37: size exceeds the curvature radius of the NURBS support (fold margi... | 🟢 |
| 179 | `bevel-active == 0` | 0 | 🟢 |
| 180 | `sound == 1` | 1 | 🟢 |
| 181 | `refused == 0` | 0 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 173NurbsBlendCutBody | 149 | `07566569af7d4260` |
| 174NurbsChamfer025 | 184 | `689bd6dd4f65a61c` |
| 175NurbsChamferApplied | 208 | `149cc53f4749fbb3` |
| 176NurbsVariablePreview | 282 | `dfe78af561d13419` |
| 177NurbsVariableApplied | 290 | `3f2a61547877a0ab` |
| 178NurbsChamferRefused | 342 | `537b246710dcb288` |

## Journal

- New document (previous recovery copy kept)
- Curve on plane XY: click points · 1 Hermite · 2 Bezier · 3 NURBS · 4 cubic spline · Enter / right-click finishes · Ctrl+Z drops a point · Esc aborts
- Curve1 · Hermite · 0 ms
- Extrude Curve1 (E): move horizontally · Q union · W difference · E intersect · R new body · Enter / right-click applies · Esc aborts
- Bevel size 1 m (pinned)
- Extrude1 · Curve1 · New · 44 ms
- Selection cleared
- Line on plane XY: click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · Esc aborts
- Line1 · 34 ms
- Cut (Extrude1 by Line1) · 1345 ms
- Remove (Cut1) · 35 ms
- View Isometric
- Framed the document (Space)
- Selection cleared
- Autosaved _ClaudeScratch/SolidArcWorkspace/NurbsBlendRecovery.solidarc
- Selected Edge(Extrude#2.Cap(End)|Extrude#2.Side(Spline1))
- Fillet (B): move horizontally · D fillet · C chamfer · V variable size · F flip its anchor · click edits the edges · Enter / right-click applies · Esc aborts
- Bevel size 0.15 m (pinned)
- Chamfer 0.15 on 1 edge(s) · 70635 ms
- Undo
- Selected Edge(Extrude#2.Cap(End)|Extrude#2.Side(Spline1))
- Fillet (B): move horizontally · D fillet · C chamfer · V variable size · F flip its anchor · click edits the edges · Enter / right-click applies · Esc aborts
- Bevel size 0.1 m (pinned)
- Variable size: 0.1 m at the anchor corner → 0.05 m (F flips the anchor)
- Bevel end size 0.25 m
- Fillet 0.1 → 0.25 on 1 edge(s) · 153853 ms
- Undo
- Selected Edge(Extrude#2.Cap(End)|Extrude#2.Side(Spline1))
- Fillet (B): move horizontally · D fillet · C chamfer · V variable size · F flip its anchor · click edits the edges · Enter / right-click applies · Esc aborts
- Bevel size 0.9 m (pinned)
- Bevel cancelled (document untouched)
