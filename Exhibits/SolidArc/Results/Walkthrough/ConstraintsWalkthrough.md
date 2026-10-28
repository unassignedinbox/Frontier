# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 810 · 466 frames · 29.3044 s · slowest frame 352.18 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/ConstraintsWalkthrough.txt
- Expectations: 75 / 75 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 16 | `steps == 0` | 0 | 🟢 |
| 43 | `steps == 2` | 2 | 🟢 |
| 44 | `volume ~ 2` | 2 | 🟢 |
| 51 | `constrain == 1` | 1 | 🟢 |
| 52 | `constraint-count == 4` | 4 | 🟢 |
| 53 | `constraints == H1,V1,H2,V2` | H1,V1,H2,V2 | 🟢 |
| 54 | `sketch-dof == 4` | 4 | 🟢 |
| 55 | `sketch-status == under-constrained` | under-constrained | 🟢 |
| 56 | `status contains Constrain Rectangle1` | Constrain Rectangle1 · 4 constraint(s) · DOF 4 · click points / curves (earlier sket... | 🟢 |
| 62 | `constrain-picks == Line1.Start` | Line1.Start | 🟢 |
| 65 | `constraint-count == 5` | 5 | 🟢 |
| 66 | `sketch-dof == 2` | 2 | 🟢 |
| 67 | `constrain-picks == 0` |  | 🟢 |
| 68 | `constrain-commits == 1` | 1 | 🟢 |
| 73 | `constrain-picks == Line1` | Line1 | 🟢 |
| 76 | `status contains type a value` | Length 2 · Line1: type a value (now 2) · Enter applies · Esc drops | 🟢 |
| 81 | `constraint-count == 6` | 6 | 🟢 |
| 82 | `sketch-dof == 1` | 1 | 🟢 |
| 83 | `volume ~ 3` | 3 | 🟢 |
| 84 | `picked-constraint contains D1` | D1 Length 3 · Line1 | 🟢 |
| 91 | `constrain-picks == Line2` | Line2 | 🟢 |
| 97 | `sketch-dof == 0` | 0 | 🟢 |
| 98 | `sketch-status == fully constrained` | fully constrained | 🟢 |
| 99 | `constraints == H1,V1,H2,V2,F1,D1,D2` | H1,V1,H2,V2,F1,D1,D2 | 🟢 |
| 100 | `volume ~ 6` | 6 | 🟢 |
| 101 | `sketch-points == (0, 0) (3, 0) (3, 2) (0, 2)` | (0, 0) (3, 0) (3, 2) (0, 2) | 🟢 |
| 113 | `constraint-count == 7` | 7 | 🟢 |
| 114 | `constrain-refusal contains is already implied by` | Length 3 · Line3 is already implied by V1 (Vertical · Line2), V2 (Vertical · Line4),... | 🟢 |
| 115 | `constrain-refusal contains D1` | Length 3 · Line3 is already implied by V1 (Vertical · Line2), V2 (Vertical · Line4),... | 🟢 |
| 119 | `constraint-count == 7` | 7 | 🟢 |
| 120 | `constrain-refusal contains conflicts with` | Length 5 · Line3 conflicts with V1 (Vertical · Line2), V2 (Vertical · Line4), D1 (Le... | 🟢 |
| 121 | `volume ~ 6` | 6 | 🟢 |
| 127 | `constrain-picks == 0` |  | 🟢 |
| 128 | `constrain == 1` | 1 | 🟢 |
| 133 | `picked-constraint contains D1` | D1 Length 3 · Line1 | 🟢 |
| 137 | `volume ~ 8` | 8 | 🟢 |
| 138 | `sketch-points contains (4, 2)` | (0, 0) (4, 0) (4, 2) (0, 2) | 🟢 |
| 141 | `volume ~ 6` | 6 | 🟢 |
| 144 | `volume ~ 8` | 8 | 🟢 |
| 145 | `constrain == 1` | 1 | 🟢 |
| 153 | `picked-constraint contains D2` | D2 Length 2 · Line2 | 🟢 |
| 156 | `constraint-count == 6` | 6 | 🟢 |
| 157 | `constraints == H1,V1,H2,V2,F1,D1` | H1,V1,H2,V2,F1,D1 | 🟢 |
| 158 | `sketch-dof == 1` | 1 | 🟢 |
| 159 | `volume ~ 8` | 8 | 🟢 |
| 170 | `constrain == 1` | 1 | 🟢 |
| 171 | `volume ~ 8` | 8 | 🟢 |
| 175 | `sketch-points == (0, 0) (4, 0) (4, 3) (0, 3)` | (0, 0) (4, 0) (4, 3) (0, 3) | 🟢 |
| 176 | `volume ~ 12` | 12 | 🟢 |
| 177 | `constrain-commits == 6` | 6 | 🟢 |
| 190 | `constrain-refusal contains fully constrained` | Line1.Start is fully constrained (it cannot move) | 🟢 |
| 191 | `volume ~ 12` | 12 | 🟢 |
| 192 | `constrain-commits == 6` | 6 | 🟢 |
| 198 | `constrain == 0` | 0 | 🟢 |
| 211 | `constrain == 1` | 1 | 🟢 |
| 212 | `constraint-count == 0` | 0 | 🟢 |
| 213 | `sketch-dof == 6` | 6 | 🟢 |
| 216 | `constrain-picks == Line1.Start` | Line1.Start | 🟢 |
| 219 | `sketch-dof == 4` | 4 | 🟢 |
| 222 | `constrain-picks == Line1` | Line1 | 🟢 |
| 225 | `constraints == F1,H1` | F1,H1 | 🟢 |
| 226 | `sketch-dof == 3` | 3 | 🟢 |
| 234 | `constraints == F1,H1,D1` | F1,H1,D1 | 🟢 |
| 235 | `sketch-dof == 2` | 2 | 🟢 |
| 236 | `sketch-points == (0.5, 3.4) (2, 3.4) (3.5, 4.4)` | (0.5, 3.4) (2, 3.4) (3.5, 4.4) | 🟢 |
| 245 | `constrain-picks == Line1,Line2` | Line1,Line2 | 🟢 |
| 251 | `constraints == F1,H1,D1,A1` | F1,H1,D1,A1 | 🟢 |
| 252 | `sketch-dof == 1` | 1 | 🟢 |
| 259 | `constrain-picks == Line1,Line2` | Line1,Line2 | 🟢 |
| 262 | `constraints == F1,H1,D1,A1,Q1` | F1,H1,D1,A1,Q1 | 🟢 |
| 263 | `sketch-dof == 0` | 0 | 🟢 |
| 264 | `sketch-status == fully constrained` | fully constrained | 🟢 |
| 265 | `sketch-points == (0.5, 3.4) (2, 3.4) (3.06066, 4.46066)` | (0.5, 3.4) (2, 3.4) (3.06066, 4.46066) | 🟢 |
| 273 | `constrain == 0` | 0 | 🟢 |
| 274 | `refused == 0` | 0 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 122ConstrainOpen | 115 | `d5788f2e2bf8f75e` |
| 123ConstrainFull | 180 | `26bb99e2b349a75f` |
| 124ConstrainConflict | 208 | `62352135ce5e5ce9` |
| 125ConstrainEdit | 246 | `0a16c5edb613dd9e` |
| 126ConstrainDragging | 281 | `37d72de2f4421498` |
| 127ConstrainAngle | 460 | `bc02654770f3faf9` |

## Journal

- New document (previous recovery copy kept)
- Rectangle on plane XY (or the body face under the first click): click two corners · Alt centre mode · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Rectangle1 · 0 ms
- Fill Rectangle1 · 0 ms
- Extrude Rectangle1 (E): the profile follows the pointer along its normal · Q union · W difference · E intersect · R new body · A auto · Enter / right-click applies · Esc aborts
- Bevel size 1 m (pinned)
- Extrude1 · Rectangle1 · New · 0 ms
- Selection cleared
- Constrain Rectangle1 · 4 constraint(s) · DOF 4 · click points / curves (earlier sketches link), then D N H V O Q M Y L [ ] C · Shift+D reference · drag points · Esc ends
- Picked Line1.Start
- Constrain Rectangle1: Fixed (0, 0) · Line1.Start · 0 ms
- Picked Line1
- Length 2 · Line1: type a value (now 2) · Enter applies · Esc drops
- Constrain Rectangle1: Length 3 · Line1 · 0 ms
- Picks cleared
- Picked Line2
- Length 1 · Line2: type a value (now 1) · Enter applies · Esc drops
- Constrain Rectangle1: Length 2 · Line2 · 0 ms
- Picks cleared
- Picked Line3
- Length 3 · Line3: type a value (now 3) · Enter applies · Esc drops
- Refused: Length 3 · Line3 is already implied by V1 (Vertical · Line2), V2 (Vertical · Line4), D1 (Length 3 · Line1)
- Refused: Length 5 · Line3 conflicts with V1 (Vertical · Line2), V2 (Vertical · Line4), D1 (Length 3 · Line1)
- Autosaved _ClaudeScratch/SolidArcWorkspace/ConstraintsRecovery.solidarc
- Constraint dropped
- Picks cleared
- D1 Length 3 · Line1 · type a value + Enter · E makes it a reference · Delete removes
- D1 = 4 in Rectangle1 · 0 ms
- Undo
- Redo
- Picks cleared
- D2 Length 2 · Line2 · type a value + Enter · E makes it a reference · Delete removes
- Remove D2 (Length 2 · Line2) from Rectangle1 · 0 ms
- Drag Line2.End of Rectangle1 · 0 ms
- Refused: Line1.Start is fully constrained (it cannot move)
- Constrain ended
- Line on plane XY (or the body face under the first click): click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Autosaved _ClaudeScratch/SolidArcWorkspace/ConstraintsRecovery.solidarc
- Line1 · 0 ms
- Constrain Line1 · 0 constraint(s) · DOF 6 · click points / curves (earlier sketches link), then D N H V O Q M Y L [ ] C · Shift+D reference · drag points · Esc ends
- Picked Line1.Start
- Constrain Line1: Fixed (0.5, 3.4) · Line1.Start · 0 ms
- Picked Line1
- Constrain Line1: Horizontal · Line1 · 0 ms
- Picked Line1
- Length 2 · Line1: type a value (now 2) · Enter applies · Esc drops
- Constrain Line1: Length 1.5 · Line1 · 0 ms
- Picks cleared
- Picked Line1
- Picked Line1, Line2
- Angle 33.6901° · Line1 · Line2: type a value (now 33.69°) · Enter applies · Esc drops
- Constrain Line1: Angle 45° · Line1 · Line2 · 0 ms
- Picks cleared
- Picked Line1
- Picked Line1, Line2
- Constrain Line1: Equal length · Line1 · Line2 · 0 ms
- Constrain ended
