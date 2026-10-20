# Sketch validation (step 7, M)

**🟢 PASS** · corpus 18/18 · chain 122 unknowns DOF 0 in 5 steps 13.6 ms (conditioning 4e-05, not singular) · 54 ms total

## Gates

| | Gate | Measures | Limit | Worst | Checks | Failures |
|---|---|---|---|---|---|---|
| 🟢 | S1 | DOF (structural = measured unless singular) · redundant count · conflict groups (with the expected labels) vs hand count | all equal | 0.00e+00 | 18 | 0 |
| 🟢 | S2 | Solved positions vs closed form (law of cosines, tangency, mirror, …) | ≤ 10⁻¹²·L | 1.10e-15 | 10 | 0 |
| 🟢 | S3 | Random starts ±0.1·L (200 per specimen): converged with every constraint met | ≥ 99 % | 0.00e+00 | 5 | 0 |
| 🟢 | S4 | Minimum-norm motion: re-dimensioning a free rectangle splits the change evenly | ≤ 10⁻¹²·L | 4.39e-17 | 1 | 0 |
| 🟢 | S5 | Fully-constrained point flags (null space of J) vs hand analysis | all equal | 0.00e+00 | 14 | 0 |
| 🟢 | S6 | Scale 10⁻³ … 10³: same rank / DOF, relative position error | ≤ 10⁻¹² | 1.84e-16 | 21 | 0 |
| 🟢 | S7 | Positional singularity flagged exactly where expected (structural vs measured rank, or conditioning < 10⁻³ × generic) | all equal | 0.00e+00 | 18 | 0 |
| 🟢 | S8 | Profile loops: area vs closed form · refusals where expected | rel. ≤ 10⁻¹² | 1.28e-15 | 18 | 0 |
| 🟢 | S9 | Same sketch twice → bit-identical positions and diagnosis | equal | 0.00e+00 | 18 | 0 |

## Corpus

| | Sketch | What it is | n | m | rank (structural) | DOF (hand) | Redundant | Conflict groups | Singular (conditioning) | Steps | max residual | position error | Profile | area rel. error | ms |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 🟢 | FreeRectangle | H, V, H, V only: position (2) + width + height free, DOF 4 | 8 | 4 | 4 (4) | 4 (4) | – | – | no (1e+00) | 1 | 5.2e-18 | – | 1 loop(s) | – | 0.05 |
| 🟢 | DimensionedRectangle | width and height dimensioned, position free, DOF 2 | 8 | 6 | 6 (6) | 2 (2) | – | – | no (7e-01) | 1 | 6.9e-18 | – | 1 loop(s) | 3.0e-16 | 0.01 |
| 🟢 | FixedRectangle | dimensioned and anchored, DOF 0 | 8 | 8 | 8 (8) | 0 (0) | – | – | no (4e-01) | 1 | 6.9e-18 | 1.4e-17 | 1 loop(s) | 1.5e-16 | 0.01 |
| 🟢 | HalfFixedRectangle | anchored with width only: bottom corners fixed, top edge free to slide up, DOF 1 | 8 | 7 | 7 (7) | 1 (1) | – | – | no (4e-01) | 1 | 6.9e-18 | – | 1 loop(s) | – | 0.01 |
| 🟢 | RedundantRectangle | fixed rectangle + parallel(bottom, top): implied by H1, H2, so redundant | 8 | 9 | 8 (8) | 0 (0) | Parallel | – | no (2e-01) | 1 | 8.5e-18 | 1.4e-17 | 1 loop(s) | 1.5e-16 | 0.01 |
| 🟢 | ConflictingRectangle | fixed 3-wide rectangle + top length 4: conflicts with Width | 8 | 9 | 8 (8) | 0 (0) | – | {V1, V2, Width, TopLength} | no (4e-01) | 2 | 1.0e+00 | – | 1 loop(s) | – | 0.01 |
| 🟢 | TriangleSides | sides 4, 3, 2 with A fixed and AB horizontal, DOF 0 | 6 | 6 | 6 (6) | 0 (0) | – | – | no (4e-01) | 3 | 4.4e-16 | 5.0e-16 | 1 loop(s) | 3.1e-16 | 0.01 |
| 🟢 | Equilateral | base 2 fixed, two equal-length constraints, DOF 0 | 6 | 6 | 6 (6) | 0 (0) | – | – | no (3e-01) | 3 | 2.7e-15 | 2.9e-15 | 1 loop(s) | 1.3e-15 | 0.01 |
| 🟢 | RedundantAngle | equilateral + exterior angle 120° (implied): redundant | 6 | 7 | 6 (6) | 0 (0) | Turn120 | – | no (2e-01) | 3 | 2.7e-15 | 2.9e-15 | 1 loop(s) | 1.3e-15 | 0.02 |
| 🟢 | Slot | stadium: 2 lines tangent to 2 arcs, r and centre distance dimensioned, DOF 0 | 12 | 12 | 12 (12) | 0 (0) | – | – | no (5e-02) | 4 | 6.3e-17 | 1.1e-17 | 1 loop(s) | 9.1e-16 | 0.05 |
| 🟢 | PlateWithHole | fixed rectangle with a dimensioned circular hole, DOF 0 | 11 | 11 | 11 (11) | 0 (0) | – | – | no (3e-01) | 1 | 6.9e-18 | 1.4e-17 | 2 loop(s) | 1.7e-16 | 0.04 |
| 🟢 | FreeCircle | circle with radius only: centre free, DOF 2 | 3 | 1 | 1 (1) | 2 (2) | – | – | no (1e+00) | 1 | 0.0e+00 | – | 1 loop(s) | 0.0e+00 | 0.00 |
| 🟢 | TangentCircles | r 1 fixed at origin, r 0.5 tangent outside on the same level, DOF 0 | 6 | 6 | 6 (6) | 0 (0) | – | – | no (4e-01) | 2 | 3.7e-18 | 3.7e-18 | 1 loop(s) | 2.8e-16 | 0.01 |
| 🟢 | RadiusConflict | concentric circles r 1 and r 2 declared equal radius: conflict | 6 | 7 | 6 (6) | 0 (0) | – | {R1, R2, Equal} | no (5e-01) | 2 | 1.0e+00 | – | 2 loop(s) | – | 0.01 |
| 🟢 | MidpointMirror | midpoint of a 4-long base, Q mirrors P across the vertical through it, DOF 0 | 12 | 12 | 12 (12) | 0 (0) | – | – | no (6e-02) | 2 | 1.1e-19 | 4.4e-16 | refused: sketch has no profile curves | – | 0.02 |
| 🟢 | PointOnCurves | point on the r 2 circle and on the line y = x (branch nearest the drawing), DOF 0 | 9 | 9 | 9 (9) | 0 (0) | – | – | no (4e-01) | 3 | 8.4e-16 | 5.0e-16 | refused: sketch has no profile curves | – | 0.01 |
| 🟢 | TouchingCircles | point on two r 1 circles 2 apart (they touch): double root, flagged singular (FPA #106) | 8 | 8 | 8 (8) | 0 (0) | – | – | ⚠️ yes (1e-06) | 18 | 5.5e-13 | – | refused: sketch has no profile curves | – | 0.03 |
| 🟢 | BowTie | 4 lines whose diagonals cross: solves (DOF 8) but is refused as a profile | 8 | 0 | 0 (0) | 8 (8) | – | – | no (1e+00) | 0 | 0.0e+00 | – | refused: profile crosses itself: Line1 × Line3 | – | 0.00 |

## Random starts (S3)

| | Sketch | spread / L | trials | converged | drawn branch | mean steps | mean ms |
|---|---|---|---|---|---|---|---|
| 🟢 | TriangleSides | 0.1 | 200 | 100.0 % | 100.0 % | 3.9 | 0.01 |
| 🔍 | TriangleSides | 0.3 | 200 | 100.0 % | 93.0 % | 4.8 | 0.01 |
| 🟢 | Slot | 0.1 | 200 | 100.0 % | 95.5 % | 5.7 | 0.04 |
| 🔍 | Slot | 0.3 | 200 | 100.0 % | 30.0 % | 9.0 | 0.07 |
| 🟢 | PlateWithHole | 0.1 | 200 | 100.0 % | 100.0 % | 1.0 | 0.01 |
| 🔍 | PlateWithHole | 0.3 | 200 | 100.0 % | 100.0 % | 1.0 | 0.01 |
| 🟢 | TangentCircles | 0.1 | 200 | 100.0 % | 100.0 % | 2.0 | 0.00 |
| 🔍 | TangentCircles | 0.3 | 200 | 100.0 % | 100.0 % | 2.0 | 0.00 |
| 🟢 | PointOnCurves | 0.1 | 200 | 100.0 % | 100.0 % | 3.0 | 0.01 |
| 🔍 | PointOnCurves | 0.3 | 200 | 100.0 % | 75.5 % | 3.2 | 0.01 |

> 💡 0.1·L is gated; 0.3·L is informational (a start that far from the drawing may legitimately reach another branch).

## Minimum-norm motion (S4)

Width 3 → 4 on a free rectangle: each vertical side moves 0.5, nothing else moves · error 1.8e-16

## Scale (S6)

| | s | FixedRectangle rel. error (rank) | TriangleSides rel. error (rank) | Slot rel. error (rank) |
|---|---|---|---|---|
| 🟢 | 1e-03 | 1.4e-18 (8) | 1.1e-16 (6) | 2.5e-18 (12) |
| 🟢 | 1e-02 | 5.3e-18 (8) | 1.3e-16 (6) | 2.0e-17 (12) |
| 🟢 | 1e-01 | 2.4e-18 (8) | 1.8e-16 (6) | 2.5e-18 (12) |
| 🟢 | 1e+00 | 3.8e-18 (8) | 1.2e-16 (6) | 2.5e-18 (12) |
| 🟢 | 1e+01 | 1.5e-18 (8) | 9.3e-17 (6) | 2.1e-17 (12) |
| 🟢 | 1e+02 | 7.9e-17 (8) | 0.0e+00 (6) | 2.5e-18 (12) |
| 🟢 | 1e+03 | 2.5e-19 (8) | 1.2e-16 (6) | 2.5e-18 (12) |
