# DraftingWorkspaceCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0
- Frame: 1600 x 900 · 609 frames · 44.0456 s · slowest frame 401.442 ms
- Script: /home/user/Frontier/Experimental/DraftingWorkspace/Scripts/LinkedConstraintsWalkthrough.txt
- Expectations: 55 / 55 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 15 | `steps == 0` | 0 | 🟢 |
| 36 | `drawing-form == NURBS` | NURBS | 🟢 |
| 47 | `sketches == 2` | Line1,Curve1 | 🟢 |
| 50 | `sketches == 1` | Curve1 | 🟢 |
| 51 | `sketch-curves == 2` | 2 | 🟢 |
| 56 | `constrain == 1` | 1 | 🟢 |
| 57 | `sketch-dof == 10` | 10 | 🟢 |
| 66 | `constraints == F1,F2` | F1,F2 | 🟢 |
| 67 | `sketch-dof == 6` | 6 | 🟢 |
| 74 | `constrain-picks == Spline1,Line2` | Spline1,Line2 | 🟢 |
| 77 | `constraints == F1,F2,T1` | F1,F2,T1 | 🟢 |
| 78 | `sketch-dof == 5` | 5 | 🟢 |
| 79 | `constrain-refusal == 0` |  | 🟢 |
| 88 | `constrain-picks == Spline1,Line2` | Spline1,Line2 | 🟢 |
| 91 | `constraints == F1,F2,K1` | F1,F2,K1 | 🟢 |
| 92 | `sketch-dof == 4` | 4 | 🟢 |
| 93 | `status contains replaces T1` | Constrain Curve1: Curvature (G2) · Spline1 · Line2 (replaces T1 (Tangent (G1) · Spli... | 🟢 |
| 98 | `constrain-picks == Spline1,Spline1.Point3` | Spline1,Spline1.Point3 | 🟢 |
| 101 | `constraints == F1,F2,K1,H1` | F1,F2,K1,H1 | 🟢 |
| 102 | `sketch-dof == 3` | 3 | 🟢 |
| 103 | `sketch-status == under-constrained` | under-constrained | 🟢 |
| 134 | `steps == 2` | 2 | 🟢 |
| 161 | `sketch-dof == 0` | 0 | 🟢 |
| 162 | `volume ~ 1.5` | 1.5 | 🟢 |
| 172 | `constraints == H1,V1,H2,V2,F1,D1,D2,D3` | H1,V1,H2,V2,F1,D1,D2,D3 | 🟢 |
| 173 | `picked-constraint contains (reference)` | D3 Distance 1.80278 (reference) · Line1.Start · Line2.End | 🟢 |
| 174 | `picked-constraint contains 1.80278` | D3 Distance 1.80278 (reference) · Line1.Start · Line2.End | 🟢 |
| 175 | `sketch-dof == 0` | 0 | 🟢 |
| 176 | `sketch-status == fully constrained` | fully constrained | 🟢 |
| 177 | `volume ~ 1.5` | 1.5 | 🟢 |
| 185 | `picked-constraint contains D2` | D2 Length 1 · Line2 | 🟢 |
| 189 | `volume ~ 3` | 3 | 🟢 |
| 192 | `picked-constraint contains Distance 2.5 (reference)` | D3 Distance 2.5 (reference) · Line1.Start · Line2.End | 🟢 |
| 195 | `constrain-refusal contains stays a reference` | Distance 2.5 · Line1.Start · Line2.End is already implied by H1 (Horizontal · Line1)... | 🟢 |
| 196 | `constraint-count == 8` | 8 | 🟢 |
| 207 | `constrain == 0` | 0 | 🟢 |
| 216 | `sketches == 2` | Rectangle1,Line1 | 🟢 |
| 219 | `constrain == 1` | 1 | 🟢 |
| 220 | `sketch-dof == 4` | 4 | 🟢 |
| 225 | `constrain-picks == Line1.Start,Line2.End of Rectangle1` | Line1.Start,Line2.End of Rectangle1 | 🟢 |
| 232 | `sketch-dof == 2` | 2 | 🟢 |
| 233 | `sketch-links contains Point` | Point2 | 🟢 |
| 234 | `sketch-points contains (1.5, 2)` | (1.5, 2) (3.2, 3) (1.5, 2) | 🟢 |
| 239 | `constrain-picks == Line1.End,Line3 of Rectangle1` | Line1.End,Line3 of Rectangle1 | 🟢 |
| 242 | `sketch-dof == 1` | 1 | 🟢 |
| 243 | `sketch-links contains Line2` | Line2.Start,Line2.End,Line2 | 🟢 |
| 244 | `sketch-points contains (3.2, 2)` | (1.5, 2) (3.2, 2) (1.5, 2) (0, 2) | 🟢 |
| 249 | `constrain == 0` | 0 | 🟢 |
| 256 | `picked-constraint contains D1` | D1 Length 1.5 · Line1 | 🟢 |
| 260 | `volume ~ 4` | 4 | 🟢 |
| 269 | `sketch-points contains (2, 2)` | (2, 2) (3.2, 2) (2, 2) (0, 2) | 🟢 |
| 270 | `sketch-dof == 1` | 1 | 🟢 |
| 280 | `status contains Unlink` | Unlink Line2.Start of Line1 · 7 ms | 🟢 |
| 281 | `sketch-dof == 3` | 3 | 🟢 |
| 282 | `sketch-points contains (2, 2)` | (2, 2) (3.2, 2) (2, 2) (0, 2) | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 128ConstrainSplineG1 | 179 | `37a2d3d88dad63f7` |
| 129ConstrainSplineG2 | 221 | `bc4c418ff5de707d` |
| 130ConstrainReference | 415 | `3875f545f682b6a7` |
| 131ConstrainReferenceFollows | 449 | `1e98a8a7855bb011` |
| 132ConstrainLinkPick | 508 | `9148cae22ec4b20a` |
| 133ConstrainLinked | 595 | `bacd6fa93d6b96b3` |

## Journal

- New document (previous recovery copy kept)
- Line on plane XY: click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · Esc aborts
- Line1 · 0 ms
- Curve on plane XY: click points · 1 Hermite · 2 Bezier · 3 NURBS · 4 cubic spline · Enter / right-click finishes · Ctrl+Z drops a point · Esc aborts
- Curve form: NURBS
- Autosaved _ClaudeScratch/DraftingWorkspace/LinkedConstraintsRecovery.drafting.toml
- Curve1 · NURBS · 0 ms
- Join (Curve1, Line1) · 0 ms
- Constrain Curve1 · 0 constraint(s) · DOF 10 · click points / curves (earlier sketches link), then D N H V O Q M Y L [ ] C · Shift+D reference · drag points · Esc ends
- Picked Line2.Start
- Constrain Curve1: Fixed (0, 1.5) · Line2.Start · 0 ms
- Picked Spline1.Point0
- Constrain Curve1: Fixed (1, 1) · Spline1.Point0 · 0 ms
- Picked Spline1
- Picked Spline1, Line2
- Constrain Curve1: Tangent (G1) · Spline1 · Line2 · 0 ms
- Picked Spline1
- Picked Spline1, Line2
- Constrain Curve1: Curvature (G2) · Spline1 · Line2 (replaces T1 (Tangent (G1) · Spline1 · Line2)) · 0 ms
- Picked Spline1
- Picked Spline1, Spline1.Point3
- Constrain Curve1: Horizontal end · Spline1.Point3 · 0 ms
- Constrain ended
- Selection cleared
- New document (previous recovery copy kept)
- Autosaved _ClaudeScratch/DraftingWorkspace/LinkedConstraintsRecovery.drafting.toml
- Rectangle on plane XY: click two corners · Alt centre mode · Esc aborts
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
- Constrain Rectangle1: Length 1.5 · Line1 · 6 ms
- Picks cleared
- Picked Line2
- Length 1 · Line2: type a value (now 1) · Enter applies · Esc drops
- Constrain Rectangle1: Length 1 · Line2 · 6 ms
- Picks cleared
- Picked Line1.Start
- Picked Line1.Start, Line2.End
- Constrain Rectangle1: Distance 1.80278 (reference) · Line1.Start · Line2.End · 6 ms
- Picks cleared
- D2 = 2 in Rectangle1 · 9 ms
- Refused: Distance 2.5 · Line1.Start · Line2.End is already implied by H1 (Horizontal · Line1), V1 (Vertical · Line2), D1 (Length 1.5 · Line1), D2 (Length 2 · Line2) (it stays a reference)
- Picks cleared
- Constrain ended
- Selection cleared
- Line on plane XY: click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · Esc aborts
- Autosaved _ClaudeScratch/DraftingWorkspace/LinkedConstraintsRecovery.drafting.toml
- Line1 · 6 ms
- Constrain Line1 · 0 constraint(s) · DOF 4 · click points / curves (earlier sketches link), then D N H V O Q M Y L [ ] C · Shift+D reference · drag points · Esc ends
- Picked Line1.Start
- Picked Line1.Start, Line2.End of Rectangle1
- Constrain Line1: Coincident · Line1.Start · Point2 · 6 ms
- Picked Line1.End
- Picked Line1.End, Line3 of Rectangle1
- Constrain Line1: Point on line · Line1.End · Line2 · 6 ms
- Constrain ended
- Constrain Rectangle1 · 8 constraint(s) · DOF 0 · click points / curves (earlier sketches link), then D N H V O Q M Y L [ ] C · Shift+D reference · drag points · Esc ends
- D1 = 2 in Rectangle1 · 11 ms
- Picks cleared
- Constrain ended
- Constrain Line1 · 2 constraint(s) · DOF 1 · click points / curves (earlier sketches link), then D N H V O Q M Y L [ ] C · Shift+D reference · drag points · Esc ends
- Picked Line2.Start
- Autosaved _ClaudeScratch/DraftingWorkspace/LinkedConstraintsRecovery.drafting.toml
- Unlink Line2.Start of Line1 · 7 ms
