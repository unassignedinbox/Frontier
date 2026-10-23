# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0
- Frame: 1600 x 900 · 457 frames · 52.747 s · slowest frame 452.578 ms
- Script: /home/user/Frontier/Exhibits/SolidArc/Code/Scripts/ConstraintsWalkthrough.txt
- Expectations: 75 / 75 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 13 | `steps == 0` | 0 | 🟢 |
| 37 | `steps == 2` | 2 | 🟢 |
| 38 | `volume ~ 2` | 2 | 🟢 |
| 45 | `constrain == 1` | 1 | 🟢 |
| 46 | `constraint-count == 4` | 4 | 🟢 |
| 47 | `constraints == H1,V1,H2,V2` | H1,V1,H2,V2 | 🟢 |
| 48 | `sketch-dof == 4` | 4 | 🟢 |
| 49 | `sketch-status == under-constrained` | under-constrained | 🟢 |
| 50 | `status contains Constrain Rectangle1` | Constrain Rectangle1 · 4 constraint(s) · DOF 4 · click points / curves (earlier sket... | 🟢 |
| 56 | `constrain-picks == Line1.Start` | Line1.Start | 🟢 |
| 59 | `constraint-count == 5` | 5 | 🟢 |
| 60 | `sketch-dof == 2` | 2 | 🟢 |
| 61 | `constrain-picks == 0` |  | 🟢 |
| 62 | `constrain-commits == 1` | 1 | 🟢 |
| 67 | `constrain-picks == Line1` | Line1 | 🟢 |
| 70 | `status contains type a value` | Length 2 · Line1: type a value (now 2) · Enter applies · Esc drops | 🟢 |
| 75 | `constraint-count == 6` | 6 | 🟢 |
| 76 | `sketch-dof == 1` | 1 | 🟢 |
| 77 | `volume ~ 3` | 3 | 🟢 |
| 78 | `picked-constraint contains D1` | D1 Length 3 · Line1 | 🟢 |
| 85 | `constrain-picks == Line2` | Line2 | 🟢 |
| 91 | `sketch-dof == 0` | 0 | 🟢 |
| 92 | `sketch-status == fully constrained` | fully constrained | 🟢 |
| 93 | `constraints == H1,V1,H2,V2,F1,D1,D2` | H1,V1,H2,V2,F1,D1,D2 | 🟢 |
| 94 | `volume ~ 6` | 6 | 🟢 |
| 95 | `sketch-points == (0, 0) (3, 0) (3, 2) (0, 2)` | (0, 0) (3, 0) (3, 2) (0, 2) | 🟢 |
| 107 | `constraint-count == 7` | 7 | 🟢 |
| 108 | `constrain-refusal contains is already implied by` | Length 3 · Line3 is already implied by V1 (Vertical · Line2), V2 (Vertical · Line4),... | 🟢 |
| 109 | `constrain-refusal contains D1` | Length 3 · Line3 is already implied by V1 (Vertical · Line2), V2 (Vertical · Line4),... | 🟢 |
| 113 | `constraint-count == 7` | 7 | 🟢 |
| 114 | `constrain-refusal contains conflicts with` | Length 5 · Line3 conflicts with V1 (Vertical · Line2), V2 (Vertical · Line4), D1 (Le... | 🟢 |
| 115 | `volume ~ 6` | 6 | 🟢 |
| 121 | `constrain-picks == 0` |  | 🟢 |
| 122 | `constrain == 1` | 1 | 🟢 |
| 127 | `picked-constraint contains D1` | D1 Length 3 · Line1 | 🟢 |
| 131 | `volume ~ 8` | 8 | 🟢 |
| 132 | `sketch-points contains (4, 2)` | (0, 0) (4, 0) (4, 2) (0, 2) | 🟢 |
| 135 | `volume ~ 6` | 6 | 🟢 |
| 138 | `volume ~ 8` | 8 | 🟢 |
| 139 | `constrain == 1` | 1 | 🟢 |
| 147 | `picked-constraint contains D2` | D2 Length 2 · Line2 | 🟢 |
| 150 | `constraint-count == 6` | 6 | 🟢 |
| 151 | `constraints == H1,V1,H2,V2,F1,D1` | H1,V1,H2,V2,F1,D1 | 🟢 |
| 152 | `sketch-dof == 1` | 1 | 🟢 |
| 153 | `volume ~ 8` | 8 | 🟢 |
| 164 | `constrain == 1` | 1 | 🟢 |
| 165 | `volume ~ 8` | 8 | 🟢 |
| 169 | `sketch-points == (0, 0) (4, 0) (4, 3) (0, 3)` | (0, 0) (4, 0) (4, 3) (0, 3) | 🟢 |
| 170 | `volume ~ 12` | 12 | 🟢 |
| 171 | `constrain-commits == 6` | 6 | 🟢 |
| 184 | `constrain-refusal contains fully constrained` | Line1.Start is fully constrained (it cannot move) | 🟢 |
| 185 | `volume ~ 12` | 12 | 🟢 |
| 186 | `constrain-commits == 6` | 6 | 🟢 |
| 192 | `constrain == 0` | 0 | 🟢 |
| 205 | `constrain == 1` | 1 | 🟢 |
| 206 | `constraint-count == 0` | 0 | 🟢 |
| 207 | `sketch-dof == 6` | 6 | 🟢 |
| 210 | `constrain-picks == Line1.Start` | Line1.Start | 🟢 |
| 213 | `sketch-dof == 4` | 4 | 🟢 |
| 216 | `constrain-picks == Line1` | Line1 | 🟢 |
| 219 | `constraints == F1,H1` | F1,H1 | 🟢 |
| 220 | `sketch-dof == 3` | 3 | 🟢 |
| 228 | `constraints == F1,H1,D1` | F1,H1,D1 | 🟢 |
| 229 | `sketch-dof == 2` | 2 | 🟢 |
| 230 | `sketch-points == (0.5, 3.4) (2, 3.4) (3.5, 4.4)` | (0.5, 3.4) (2, 3.4) (3.5, 4.4) | 🟢 |
| 239 | `constrain-picks == Line1,Line2` | Line1,Line2 | 🟢 |
| 245 | `constraints == F1,H1,D1,A1` | F1,H1,D1,A1 | 🟢 |
| 246 | `sketch-dof == 1` | 1 | 🟢 |
| 253 | `constrain-picks == Line1,Line2` | Line1,Line2 | 🟢 |
| 256 | `constraints == F1,H1,D1,A1,Q1` | F1,H1,D1,A1,Q1 | 🟢 |
| 257 | `sketch-dof == 0` | 0 | 🟢 |
| 258 | `sketch-status == fully constrained` | fully constrained | 🟢 |
| 259 | `sketch-points == (0.5, 3.4) (2, 3.4) (3.06066, 4.46066)` | (0.5, 3.4) (2, 3.4) (3.06066, 4.46066) | 🟢 |
| 267 | `constrain == 0` | 0 | 🟢 |
| 268 | `refused == 0` | 0 | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 122ConstrainOpen | 109 | `db600726249c417e` |
| 123ConstrainFull | 174 | `c8d57a506352f693` |
| 124ConstrainConflict | 202 | `c866b661dacdf063` |
| 125ConstrainEdit | 240 | `587a7cf84cefe7b7` |
| 126ConstrainDragging | 275 | `5ac91343c32508c8` |
| 127ConstrainAngle | 451 | `50be84ad9678559f` |

## Journal

- New document (previous recovery copy kept)
- Rectangle on plane XY: click two corners · Alt centre mode · Esc aborts
- Rectangle1 · 0 ms
- Extrude Rectangle1 (E): move horizontally · Q union · W difference · E intersect · R new body · Enter / right-click applies · Esc aborts
- Bevel size 1 m (pinned)
- Extrude1 · Rectangle1 · New · 7 ms
- Selection cleared
- Constrain Rectangle1 · 4 constraint(s) · DOF 4 · click points / curves (earlier sketches link), then D N H V O Q M Y L [ ] C · Shift+D reference · drag points · Esc ends
- Picked Line1.Start
- Constrain Rectangle1: Fixed (0, 0) · Line1.Start · 13 ms
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
- Line on plane XY: click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · Esc aborts
- Autosaved _ClaudeScratch/SolidArcWorkspace/ConstraintsRecovery.solidarc
- Line1 · 15 ms
- Constrain Line1 · 0 constraint(s) · DOF 6 · click points / curves (earlier sketches link), then D N H V O Q M Y L [ ] C · Shift+D reference · drag points · Esc ends
- Picked Line1.Start
- Constrain Line1: Fixed (0.5, 3.4) · Line1.Start · 15 ms
- Picked Line1
- Constrain Line1: Horizontal · Line1 · 15 ms
- Picked Line1
- Length 2 · Line1: type a value (now 2) · Enter applies · Esc drops
- Constrain Line1: Length 1.5 · Line1 · 16 ms
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
