# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 810 · 143 frames · 13.9724 s · slowest frame 494.951 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/ReshapeWalkthrough.txt
- Expectations: 17 / 17 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 27 | `bodies == 1` | 1 | 🟢 |
| 28 | `volume ~ 3` | 3 | 🟢 |
| 29 | `faces == 6` | 6 | 🟢 |
| 38 | `selected-edges == 1` | Edge(Extrude#2.Cap(End)/Extrude#2.Side(Line1)) | 🟢 |
| 39 | `gizmo-target == 5` | edges | 🟢 |
| 45 | `gizmo-commits == 1` | 1 | 🟢 |
| 46 | `refused == 0` | 0 | 🟢 |
| 47 | `faces == 6` | 6 | 🟢 |
| 48 | `volume > 3.05` | 3.45 | 🟢 |
| 49 | `selected-edges == 1` | Edge(Extrude#2.Cap(End)/Extrude#2.Side(Line1)) | 🟢 |
| 59 | `selected-vertices == 1` | Vertex(Extrude#2.Cap(End)/Extrude#2.Side(Line2)/Extrude#2.Side(Line3)) | 🟢 |
| 60 | `gizmo-target == 5` | vertices | 🟢 |
| 66 | `gizmo-commits == 2` | 2 | 🟢 |
| 67 | `refused == 0` | 0 | 🟢 |
| 68 | `faces == 6` | 6 | 🟢 |
| 76 | `volume ~ 3` | 3 | 🟢 |
| 77 | `faces == 6` | 6 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 332ReshapeEdgePicked | 68 | `d32df8da5f5cd514` |
| 333ReshapeEdgeRaised | 90 | `ef66ac0c887e2c31` |
| 334ReshapeVertexPicked | 108 | `823397eaf8bceb72` |
| 335ReshapeVertexRaised | 130 | `357d0ce88e38835b` |
| 336ReshapeUndone | 143 | `652d9cdcebcf8ed3` |

## Journal

- New document (previous recovery copy kept)
- Block: click the first corner · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts
- Block: click the opposite corner
- Block: move up, click the height
- Block1 · New body · 0 ms
- Selected Edge(Extrude#2.Cap(End)|Extrude#2.Side(Line1))
- Move 1 edge(s) · 1 ms
- Gizmo Z move 0.3 · reshape 1 edge · Move 1 edge(s) · 1 ms
- Vertex Vertex(Extrude#2.Cap(End)|Extrude#2.Side(Line2)|Extrude#2.Side(Line3)) · 1 picked (gizmo moves them; the faces at them follow)
- Move 1 vertex(es) · 10 ms
- Gizmo Z move 0.3 · reshape 1 vertex · Move 1 vertex(es) · 10 ms
- Undo
- Undo
