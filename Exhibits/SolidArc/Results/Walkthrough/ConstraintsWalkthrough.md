# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 900 · 465 frames · 41.7776 s · slowest frame 539.395 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/ConstraintsWalkthrough.txt
- Expectations: 75 / 75 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 13 | `steps == 0` | 0 | 🟢 |
| 40 | `steps == 2` | 2 | 🟢 |
| 41 | `volume ~ 2` | 2 | 🟢 |
| 48 | `constrain == 1` | 1 | 🟢 |
| 49 | `constraint-count == 4` | 4 | 🟢 |
| 50 | `constraints == H1,V1,H2,V2` | H1,V1,H2,V2 | 🟢 |
| 51 | `sketch-dof == 4` | 4 | 🟢 |
| 52 | `sketch-status == under-constrained` | under-constrained | 🟢 |
| 53 | `status contains Constrain Rectangle1` | Constrain Rectangle1 · 4 constraint(s) · DOF 4 · click points / curves (earlier sket... | 🟢 |
| 59 | `constrain-picks == Line1.Start` | Line1.Start | 🟢 |
| 62 | `constraint-count == 5` | 5 | 🟢 |
| 63 | `sketch-dof == 2` | 2 | 🟢 |
| 64 | `constrain-picks == 0` |  | 🟢 |
| 65 | `constrain-commits == 1` | 1 | 🟢 |
| 70 | `constrain-picks == Line1` | Line1 | 🟢 |
| 73 | `status contains type a value` | Length 2 · Line1: type a value (now 2) · Enter applies · Esc drops | 🟢 |
| 78 | `constraint-count == 6` | 6 | 🟢 |
| 79 | `sketch-dof == 1` | 1 | 🟢 |
| 80 | `volume ~ 3` | 3 | 🟢 |
| 81 | `picked-constraint contains D1` | D1 Length 3 · Line1 | 🟢 |
| 88 | `constrain-picks == Line2` | Line2 | 🟢 |
| 94 | `sketch-dof == 0` | 0 | 🟢 |
| 95 | `sketch-status == fully constrained` | fully constrained | 🟢 |
| 96 | `constraints == H1,V1,H2,V2,F1,D1,D2` | H1,V1,H2,V2,F1,D1,D2 | 🟢 |
| 97 | `volume ~ 6` | 6 | 🟢 |
| 98 | `sketch-points == (0, 0) (3, 0) (3, 2) (0, 2)` | (0, 0) (3, 0) (3, 2) (0, 2) | 🟢 |
| 110 | `constraint-count == 7` | 7 | 🟢 |
| 111 | `constrain-refusal contains is already implied by` | Length 3 · Line3 is already implied by V1 (Vertical · Line2), V2 (Vertical · Line4),... | 🟢 |
| 112 | `constrain-refusal contains D1` | Length 3 · Line3 is already implied by V1 (Vertical · Line2), V2 (Vertical · Line4),... | 🟢 |
| 116 | `constraint-count == 7` | 7 | 🟢 |
| 117 | `constrain-refusal contains conflicts with` | Length 5 · Line3 conflicts with V1 (Vertical · Line2), V2 (Vertical · Line4), D1 (Le... | 🟢 |
| 118 | `volume ~ 6` | 6 | 🟢 |
| 124 | `constrain-picks == 0` |  | 🟢 |
| 125 | `constrain == 1` | 1 | 🟢 |
| 130 | `picked-constraint contains D1` | D1 Length 3 · Line1 | 🟢 |
| 134 | `volume ~ 8` | 8 | 🟢 |
| 135 | `sketch-points contains (4, 2)` | (0, 0) (4, 0) (4, 2) (0, 2) | 🟢 |
| 138 | `volume ~ 6` | 6 | 🟢 |
| 141 | `volume ~ 8` | 8 | 🟢 |
| 142 | `constrain == 1` | 1 | 🟢 |
| 150 | `picked-constraint contains D2` | D2 Length 2 · Line2 | 🟢 |
| 153 | `constraint-count == 6` | 6 | 🟢 |
| 154 | `constraints == H1,V1,H2,V2,F1,D1` | H1,V1,H2,V2,F1,D1 | 🟢 |
| 155 | `sketch-dof == 1` | 1 | 🟢 |
| 156 | `volume ~ 8` | 8 | 🟢 |
| 167 | `constrain == 1` | 1 | 🟢 |
| 168 | `volume ~ 8` | 8 | 🟢 |
| 172 | `sketch-points == (0, 0) (4, 0) (4, 3) (0, 3)` | (0, 0) (4, 0) (4, 3) (0, 3) | 🟢 |
| 173 | `volume ~ 12` | 12 | 🟢 |
| 174 | `constrain-commits == 6` | 6 | 🟢 |
| 187 | `constrain-refusal contains fully constrained` | Line1.Start is fully constrained (it cannot move) | 🟢 |
| 188 | `volume ~ 12` | 12 | 🟢 |
| 189 | `constrain-commits == 6` | 6 | 🟢 |
| 195 | `constrain == 0` | 0 | 🟢 |
| 208 | `constrain == 1` | 1 | 🟢 |
| 209 | `constraint-count == 0` | 0 | 🟢 |
| 210 | `sketch-dof == 6` | 6 | 🟢 |
| 213 | `constrain-picks == Line1.Start` | Line1.Start | 🟢 |
| 216 | `sketch-dof == 4` | 4 | 🟢 |
| 219 | `constrain-picks == Line1` | Line1 | 🟢 |
| 222 | `constraints == F1,H1` | F1,H1 | 🟢 |
| 223 | `sketch-dof == 3` | 3 | 🟢 |
| 231 | `constraints == F1,H1,D1` | F1,H1,D1 | 🟢 |
| 232 | `sketch-dof == 2` | 2 | 🟢 |
| 233 | `sketch-points == (0.5, 3.4) (2, 3.4) (3.5, 4.4)` | (0.5, 3.4) (2, 3.4) (3.5, 4.4) | 🟢 |
| 242 | `constrain-picks == Line1,Line2` | Line1,Line2 | 🟢 |
| 248 | `constraints == F1,H1,D1,A1` | F1,H1,D1,A1 | 🟢 |
| 249 | `sketch-dof == 1` | 1 | 🟢 |
| 256 | `constrain-picks == Line1,Line2` | Line1,Line2 | 🟢 |
| 259 | `constraints == F1,H1,D1,A1,Q1` | F1,H1,D1,A1,Q1 | 🟢 |
| 260 | `sketch-dof == 0` | 0 | 🟢 |
| 261 | `sketch-status == fully constrained` | fully constrained | 🟢 |
| 262 | `sketch-points == (0.5, 3.4) (2, 3.4) (3.06066, 4.46066)` | (0.5, 3.4) (2, 3.4) (3.06066, 4.46066) | 🟢 |
| 270 | `constrain == 0` | 0 | 🟢 |
| 271 | `refused == 0` | 0 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 122ConstrainOpen | 114 | `a4e92190b5fb4a94` |
| 123ConstrainFull | 179 | `3e27353824b0f88e` |
| 124ConstrainConflict | 207 | `d32d34acc15a933d` |
| 125ConstrainEdit | 245 | `64554797c90f7c17` |
| 126ConstrainDragging | 280 | `52f203d8fcba584c` |
| 127ConstrainAngle | 459 | `d29a1b6ab71be85f` |

## Journal

- New document (previous recovery copy kept)
- Rectangle on plane XY (or the body face under the first click): click two corners · Alt centre mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Rectangle1 · 0 ms
- Fill Rectangle1 · 0 ms
- Extrude Rectangle1 (E): move horizontally · Q union · W difference · E intersect · R new body · Enter / right-click applies · Esc aborts
- Bevel size 1 m (pinned)
- Extrude1 · Rectangle1 · New · 9 ms
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
- D1 = 4 in Rectangle1 · 18 ms
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
