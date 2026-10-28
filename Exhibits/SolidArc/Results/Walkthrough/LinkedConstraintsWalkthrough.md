# SolidArcCapture

- Device: SwiftShader Device (Subzero) · Vulkan 1.3.0 · 4× MSAA
- Frame: 1600 x 810 · 664 frames · 38.8402 s · slowest frame 343.178 ms
- Script: /home/user/Frontier/_ClaudeScratch/Upstream/Exhibits/SolidArc/Code/Scripts/LinkedConstraintsWalkthrough.txt
- Expectations: 55 / 55 passed

| Line | Expectation | Observed | Result |
|-----:|-------------|----------|--------|
| 18 | `steps == 0` | 0 | 🟢 |
| 39 | `drawing-form == NURBS` | NURBS | 🟢 |
| 50 | `sketches == 2` | Line1,Curve1 | 🟢 |
| 53 | `sketches == 1` | Curve1 | 🟢 |
| 54 | `sketch-curves == 2` | 2 | 🟢 |
| 59 | `constrain == 1` | 1 | 🟢 |
| 60 | `sketch-dof == 10` | 10 | 🟢 |
| 69 | `constraints == F1,F2` | F1,F2 | 🟢 |
| 70 | `sketch-dof == 6` | 6 | 🟢 |
| 77 | `constrain-picks == Spline1,Line2` | Spline1,Line2 | 🟢 |
| 80 | `constraints == F1,F2,T1` | F1,F2,T1 | 🟢 |
| 81 | `sketch-dof == 5` | 5 | 🟢 |
| 82 | `constrain-refusal == 0` |  | 🟢 |
| 91 | `constrain-picks == Spline1,Line2` | Spline1,Line2 | 🟢 |
| 94 | `constraints == F1,F2,K1` | F1,F2,K1 | 🟢 |
| 95 | `sketch-dof == 4` | 4 | 🟢 |
| 96 | `status contains replaces T1` | Constrain Curve1: Curvature (G2) · Spline1 · Line2 (replaces T1 (Tangent (G1) · Spli... | 🟢 |
| 101 | `constrain-picks == Spline1,Spline1.Point3` | Spline1,Spline1.Point3 | 🟢 |
| 104 | `constraints == F1,F2,K1,H1` | F1,F2,K1,H1 | 🟢 |
| 105 | `sketch-dof == 3` | 3 | 🟢 |
| 106 | `sketch-status == under-constrained` | under-constrained | 🟢 |
| 139 | `steps == 2` | 2 | 🟢 |
| 166 | `sketch-dof == 0` | 0 | 🟢 |
| 167 | `volume ~ 1.5` | 1.5 | 🟢 |
| 177 | `constraints == H1,V1,H2,V2,F1,D1,D2,D3` | H1,V1,H2,V2,F1,D1,D2,D3 | 🟢 |
| 178 | `picked-constraint contains (reference)` | D3 Distance 1.80278 (reference) · Line1.Start · Line2.End | 🟢 |
| 179 | `picked-constraint contains 1.80278` | D3 Distance 1.80278 (reference) · Line1.Start · Line2.End | 🟢 |
| 180 | `sketch-dof == 0` | 0 | 🟢 |
| 181 | `sketch-status == fully constrained` | fully constrained | 🟢 |
| 182 | `volume ~ 1.5` | 1.5 | 🟢 |
| 190 | `picked-constraint contains D2` | D2 Length 1 · Line2 | 🟢 |
| 194 | `volume ~ 3` | 3 | 🟢 |
| 197 | `picked-constraint contains Distance 2.5 (reference)` | D3 Distance 2.5 (reference) · Line1.Start · Line2.End | 🟢 |
| 200 | `constrain-refusal contains stays a reference` | Distance 2.5 · Line1.Start · Line2.End is already implied by H1 (Horizontal · Line1)... | 🟢 |
| 201 | `constraint-count == 8` | 8 | 🟢 |
| 212 | `constrain == 0` | 0 | 🟢 |
| 221 | `sketches == 2` | Rectangle1,Line1 | 🟢 |
| 224 | `constrain == 1` | 1 | 🟢 |
| 225 | `sketch-dof == 4` | 4 | 🟢 |
| 230 | `constrain-picks == Line1.Start,Line2.End of Rectangle1` | Line1.Start,Line2.End of Rectangle1 | 🟢 |
| 237 | `sketch-dof == 2` | 2 | 🟢 |
| 238 | `sketch-links contains Point` | Point2 | 🟢 |
| 239 | `sketch-points contains (1.5, 2)` | (1.5, 2) (3.2, 3) (1.5, 2) | 🟢 |
| 244 | `constrain-picks == Line1.End,Line3 of Rectangle1` | Line1.End,Line3 of Rectangle1 | 🟢 |
| 247 | `sketch-dof == 1` | 1 | 🟢 |
| 248 | `sketch-links contains Line2` | Line2.Start,Line2.End,Line2 | 🟢 |
| 249 | `sketch-points contains (3.2, 2)` | (1.5, 2) (3.2, 2) (1.5, 2) (0, 2) | 🟢 |
| 254 | `constrain == 0` | 0 | 🟢 |
| 266 | `picked-constraint contains D1` | D1 Length 1.5 · Line1 | 🟢 |
| 270 | `volume ~ 4` | 4 | 🟢 |
| 284 | `sketch-points contains (2, 2)` | (2, 2) (3.2, 2) (2, 2) (0, 2) | 🟢 |
| 285 | `sketch-dof == 1` | 1 | 🟢 |
| 295 | `status contains Unlink` | Unlink Line2.Start of Line1 · 0 ms | 🟢 |
| 296 | `sketch-dof == 3` | 3 | 🟢 |
| 297 | `sketch-points contains (2, 2)` | (2, 2) (3.2, 2) (2, 2) (0, 2) | 🟢 |

| Capture | Frame | Digest |
|---------|------:|--------|
| 128ConstrainSplineG1 | 180 | `d93d199fa684bd23` |
| 129ConstrainSplineG2 | 222 | `eb22df01bd0d2364` |
| 130ConstrainReference | 421 | `0d90554d3fc6b22a` |
| 131ConstrainReferenceFollows | 459 | `9da15c6797e95fbd` |
| 132ConstrainLinkPick | 518 | `d1d720418e177a0e` |
| 133ConstrainLinked | 650 | `fbaa79526d7e281b` |

## Journal

- New document (previous recovery copy kept)
- Line on plane XY (or the body face under the first click): click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Line1 · 0 ms
- Curve on plane XY (or the body face under the first click): click points · 1 Hermite · 2 Bezier · 3 NURBS · 4 cubic spline · Enter / right-click finishes · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Curve form: NURBS
- Autosaved _ClaudeScratch/SolidArcWorkspace/LinkedConstraintsRecovery.solidarc
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
- Autosaved _ClaudeScratch/SolidArcWorkspace/LinkedConstraintsRecovery.solidarc
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
- Constrain Rectangle1: Length 1.5 · Line1 · 0 ms
- Picks cleared
- Picked Line2
- Length 1 · Line2: type a value (now 1) · Enter applies · Esc drops
- Constrain Rectangle1: Length 1 · Line2 · 0 ms
- Picks cleared
- Picked Line1.Start
- Picked Line1.Start, Line2.End
- Constrain Rectangle1: Distance 1.80278 (reference) · Line1.Start · Line2.End · 0 ms
- Picks cleared
- D2 = 2 in Rectangle1 · 0 ms
- Refused: Distance 2.5 · Line1.Start · Line2.End is already implied by H1 (Horizontal · Line1), V1 (Vertical · Line2), D1 (Length 1.5 · Line1), D2 (Length 2 · Line2) (it stays a reference)
- Picks cleared
- Constrain ended
- Selection cleared
- Line on plane XY (or the body face under the first click): click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts
- Autosaved _ClaudeScratch/SolidArcWorkspace/LinkedConstraintsRecovery.solidarc
- Line1 · 0 ms
- Constrain Line1 · 0 constraint(s) · DOF 4 · click points / curves (earlier sketches link), then D N H V O Q M Y L [ ] C · Shift+D reference · drag points · Esc ends
- Picked Line1.Start
- Picked Line1.Start, Line2.End of Rectangle1
- Constrain Line1: Coincident · Line1.Start · Point2 · 0 ms
- Picked Line1.End
- Picked Line1.End, Line3 of Rectangle1
- Constrain Line1: Point on line · Line1.End · Line2 · 0 ms
- Constrain ended
- Autosaved _ClaudeScratch/SolidArcWorkspace/LinkedConstraintsRecovery.solidarc
- Constrain Rectangle1 · 8 constraint(s) · DOF 0 · click points / curves (earlier sketches link), then D N H V O Q M Y L [ ] C · Shift+D reference · drag points · Esc ends
- D1 = 2 in Rectangle1 · 0 ms
- Picks cleared
- Constrain ended
- Autosaved _ClaudeScratch/SolidArcWorkspace/LinkedConstraintsRecovery.solidarc
- Constrain Line1 · 2 constraint(s) · DOF 1 · click points / curves (earlier sketches link), then D N H V O Q M Y L [ ] C · Shift+D reference · drag points · Esc ends
- Picked Line2.Start
- Unlink Line2.Start of Line1 · 0 ms
