# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0
- Frame: 1600 x 900 · 460 frames · 35.2971 s · slowest frame 489.563 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/ConstraintsWalkthrough.txt
- Expectations: 75 / 75 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 13 | `steps == 0` | 0 | 🟢 |
| 38 | `steps == 2` | 2 | 🟢 |
| 39 | `volume ~ 2` | 2 | 🟢 |
| 46 | `constrain == 1` | 1 | 🟢 |
| 47 | `constraint-count == 4` | 4 | 🟢 |
| 48 | `constraints == H1,V1,H2,V2` | H1,V1,H2,V2 | 🟢 |
| 49 | `sketch-dof == 4` | 4 | 🟢 |
| 50 | `sketch-status == under-constrained` | under-constrained | 🟢 |
| 51 | `status contains Constrain Rectangle1` | Constrain Rectangle1 · 4 constraint(s) · DOF 4 · click points / curves (earlier sket... | 🟢 |
| 57 | `constrain-picks == Line1.Start` | Line1.Start | 🟢 |
| 60 | `constraint-count == 5` | 5 | 🟢 |
| 61 | `sketch-dof == 2` | 2 | 🟢 |
| 62 | `constrain-picks == 0` |  | 🟢 |
| 63 | `constrain-commits == 1` | 1 | 🟢 |
| 68 | `constrain-picks == Line1` | Line1 | 🟢 |
| 71 | `status contains type a value` | Length 2 · Line1: type a value (now 2) · Enter applies · Esc drops | 🟢 |
| 76 | `constraint-count == 6` | 6 | 🟢 |
| 77 | `sketch-dof == 1` | 1 | 🟢 |
| 78 | `volume ~ 3` | 3 | 🟢 |
| 79 | `picked-constraint contains D1` | D1 Length 3 · Line1 | 🟢 |
| 86 | `constrain-picks == Line2` | Line2 | 🟢 |
| 92 | `sketch-dof == 0` | 0 | 🟢 |
| 93 | `sketch-status == fully constrained` | fully constrained | 🟢 |
| 94 | `constraints == H1,V1,H2,V2,F1,D1,D2` | H1,V1,H2,V2,F1,D1,D2 | 🟢 |
| 95 | `volume ~ 6` | 6 | 🟢 |
| 96 | `sketch-points == (0, 0) (3, 0) (3, 2) (0, 2)` | (0, 0) (3, 0) (3, 2) (0, 2) | 🟢 |
| 108 | `constraint-count == 7` | 7 | 🟢 |
| 109 | `constrain-refusal contains is already implied by` | Length 3 · Line3 is already implied by V1 (Vertical · Line2), V2 (Vertical · Line4),... | 🟢 |
| 110 | `constrain-refusal contains D1` | Length 3 · Line3 is already implied by V1 (Vertical · Line2), V2 (Vertical · Line4),... | 🟢 |
| 114 | `constraint-count == 7` | 7 | 🟢 |
| 115 | `constrain-refusal contains conflicts with` | Length 5 · Line3 conflicts with V1 (Vertical · Line2), V2 (Vertical · Line4), D1 (Le... | 🟢 |
| 116 | `volume ~ 6` | 6 | 🟢 |
| 122 | `constrain-picks == 0` |  | 🟢 |
| 123 | `constrain == 1` | 1 | 🟢 |
| 128 | `picked-constraint contains D1` | D1 Length 3 · Line1 | 🟢 |
| 132 | `volume ~ 8` | 8 | 🟢 |
| 133 | `sketch-points contains (4, 2)` | (0, 0) (4, 0) (4, 2) (0, 2) | 🟢 |
| 136 | `volume ~ 6` | 6 | 🟢 |
| 139 | `volume ~ 8` | 8 | 🟢 |
| 140 | `constrain == 1` | 1 | 🟢 |
| 148 | `picked-constraint contains D2` | D2 Length 2 · Line2 | 🟢 |
| 151 | `constraint-count == 6` | 6 | 🟢 |
| 152 | `constraints == H1,V1,H2,V2,F1,D1` | H1,V1,H2,V2,F1,D1 | 🟢 |
| 153 | `sketch-dof == 1` | 1 | 🟢 |
| 154 | `volume ~ 8` | 8 | 🟢 |
| 165 | `constrain == 1` | 1 | 🟢 |
| 166 | `volume ~ 8` | 8 | 🟢 |
| 170 | `sketch-points == (0, 0) (4, 0) (4, 3) (0, 3)` | (0, 0) (4, 0) (4, 3) (0, 3) | 🟢 |
| 171 | `volume ~ 12` | 12 | 🟢 |
| 172 | `constrain-commits == 6` | 6 | 🟢 |
| 185 | `constrain-refusal contains fully constrained` | Line1.Start is fully constrained (it cannot move) | 🟢 |
| 186 | `volume ~ 12` | 12 | 🟢 |
| 187 | `constrain-commits == 6` | 6 | 🟢 |
| 193 | `constrain == 0` | 0 | 🟢 |
| 206 | `constrain == 1` | 1 | 🟢 |
| 207 | `constraint-count == 0` | 0 | 🟢 |
| 208 | `sketch-dof == 6` | 6 | 🟢 |
| 211 | `constrain-picks == Line1.Start` | Line1.Start | 🟢 |
| 214 | `sketch-dof == 4` | 4 | 🟢 |
| 217 | `constrain-picks == Line1` | Line1 | 🟢 |
| 220 | `constraints == F1,H1` | F1,H1 | 🟢 |
| 221 | `sketch-dof == 3` | 3 | 🟢 |
| 229 | `constraints == F1,H1,D1` | F1,H1,D1 | 🟢 |
| 230 | `sketch-dof == 2` | 2 | 🟢 |
| 231 | `sketch-points == (0.5, 3.4) (2, 3.4) (3.5, 4.4)` | (0.5, 3.4) (2, 3.4) (3.5, 4.4) | 🟢 |
| 240 | `constrain-picks == Line1,Line2` | Line1,Line2 | 🟢 |
| 246 | `constraints == F1,H1,D1,A1` | F1,H1,D1,A1 | 🟢 |
| 247 | `sketch-dof == 1` | 1 | 🟢 |
| 254 | `constrain-picks == Line1,Line2` | Line1,Line2 | 🟢 |
| 257 | `constraints == F1,H1,D1,A1,Q1` | F1,H1,D1,A1,Q1 | 🟢 |
| 258 | `sketch-dof == 0` | 0 | 🟢 |
| 259 | `sketch-status == fully constrained` | fully constrained | 🟢 |
| 260 | `sketch-points == (0.5, 3.4) (2, 3.4) (3.06066, 4.46066)` | (0.5, 3.4) (2, 3.4) (3.06066, 4.46066) | 🟢 |
| 268 | `constrain == 0` | 0 | 🟢 |
| 269 | `refused == 0` | 0 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 122ConstrainOpen | 109 | `e99498d95fed458a` |
| 123ConstrainFull | 174 | `7d250cb2dd979b6d` |
| 124ConstrainConflict | 202 | `f5199192c731fc62` |
| 125ConstrainEdit | 240 | `d890ed5634073bc9` |
| 126ConstrainDragging | 275 | `fd3c97066519f491` |
| 127ConstrainAngle | 454 | `20c796eef86dc4cc` |

## Journal

- New document (previous recovery copy kept)
- Rectangle on plane XY (or the body face under the first click): click two corners · Alt centre mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Rectangle1 · 0 ms
- Extrude Rectangle1 (E): move horizontally · Q union · W difference · E intersect · R new body · Enter / right-click applies · Esc aborts
- Bevel size 1 m (pinned)
- Extrude1 · Rectangle1 · New · 7 ms
- Selection cleared
- Constrain Rectangle1 · 4 constraint(s) · DOF 4 · click points / curves (earlier sketches link), then D N H V O Q M Y L [ ] C · Shift+D reference · drag points · Esc ends
- Picked Line1.Start
- Constrain Rectangle1: Fixed (0, 0) · Line1.Start · 7 ms
- Picked Line1
- Length 2 · Line1: type a value (now 2) · Enter applies · Esc drops
- Constrain Rectangle1: Length 3 · Line1 · 10 ms
- Picks cleared
- Picked Line2
- Length 1 · Line2: type a value (now 1) · Enter applies · Esc drops
- Constrain Rectangle1: Length 2 · Line2 · 14 ms
- Picks cleared
- Picked Line3
- Length 3 · Line3: type a value (now 3) · Enter applies · Esc drops
- Refused: Length 3 · Line3 is already implied by V1 (Vertical · Line2), V2 (Vertical · Line4), D1 (Length 3 · Line1)
- Refused: Length 5 · Line3 conflicts with V1 (Vertical · Line2), V2 (Vertical · Line4), D1 (Length 3 · Line1)
- Autosaved _ClaudeScratch/SolidArcWorkspace/ConstraintsRecovery.solidarc
- Constraint dropped
- Picks cleared
- D1 Length 3 · Line1 · type a value + Enter · E makes it a reference · Delete removes
- D1 = 4 in Rectangle1 · 17 ms
- Undo
- Redo
- Picks cleared
- D2 Length 2 · Line2 · type a value + Enter · E makes it a reference · Delete removes
- Remove D2 (Length 2 · Line2) from Rectangle1 · 17 ms
- Drag Line2.End of Rectangle1 · 22 ms
- Refused: Line1.Start is fully constrained (it cannot move)
- Constrain ended
- Line on plane XY (or the body face under the first click): click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Autosaved _ClaudeScratch/SolidArcWorkspace/ConstraintsRecovery.solidarc
- Line1 · 15 ms
- Constrain Line1 · 0 constraint(s) · DOF 6 · click points / curves (earlier sketches link), then D N H V O Q M Y L [ ] C · Shift+D reference · drag points · Esc ends
- Picked Line1.Start
- Constrain Line1: Fixed (0.5, 3.4) · Line1.Start · 15 ms
- Picked Line1
- Constrain Line1: Horizontal · Line1 · 15 ms
- Picked Line1
- Length 2 · Line1: type a value (now 2) · Enter applies · Esc drops
- Constrain Line1: Length 1.5 · Line1 · 15 ms
- Picks cleared
- Picked Line1
- Picked Line1, Line2
- Angle 33.6901° · Line1 · Line2: type a value (now 33.69°) · Enter applies · Esc drops
- Constrain Line1: Angle 45° · Line1 · Line2 · 15 ms
- Picks cleared
- Picked Line1
- Picked Line1, Line2
- Constrain Line1: Equal length · Line1 · Line2 · 15 ms
- Constrain ended
