# Sketch constraining validation

🟢 Constraints C1: SolveSpace constraint verbs on picked sketch points / curves, dimensions, diagnosis, point drag

## Gates

| | Gate | Measures | Limit | Worst | Checks | Failures |
|---|---|---|---|---|---|---|
| 🟢 | S1 | Verb table: D N H V O Q M Y L [ ] on lines, points, circles, arcs → the expected kind, references in order, measured value vs closed form; wrong picks refused by name | kind exact · value ≤ 10⁻¹² | 0.00e+00 | 32 | 0 |
| 🟢 | S2 | Measuring: length 5 of a 3-4-5 side, distance, signed horizontal / vertical distances, arc radius, angles +30° / −120° / 180° | ≤ 10⁻¹² | 0.00e+00 | 10 | 0 |
| 🟢 | S3 | Drawn rectangle (H / V, 4 DOF) → ] corner, D width typed 3, D height typed 2: DOF 4 → 2 → 1 → 0, corners exact; a second H is redundant (partner H1), D on the far side 3 redundant / 4 conflicting | ≤ 10⁻¹² | 4.44e-16 | 6 | 0 |
| 🟢 | S4 | Drag: free rectangle corner pinned at (4, 3) with the rest following minimally (P0 still), H / V held; the fully constrained one does not move; a point on a fixed circle lands on it nearest the target | ≤ 10⁻⁹ | 3.14e-16 | 4 | 0 |
| 🟢 | S5 | Names and descriptions: Line1.Start, Circle5.Centre, Arc6.End, Spline7.Point1, Point11; "Length 3 · Line1", "Angle 90° · Line1 · Line2" | exact text | 0.00e+00 | 15 | 0 |
| 🟢 | S6 | Solved verbs: tangent line–circle (distance = r), angle typed 30°, equal radius, midpoint, symmetric, concentric, perpendicular | ≤ 10⁻¹² | 4.48e-15 | 7 | 0 |

## Checks

| | Gate | Check | Detail |
|---|---|---|---|
| 🟢 | S1 | D line → length | Length 3 · Line1 · |Δvalue| 0.000e+00 |
| 🟢 | S1 | D 2 points → distance | Distance 3.60555 · Line1.Start · Line2.End · |Δvalue| 0.000e+00 |
| 🟢 | S1 | D circle → radius | Radius 0.5 · Circle5 · |Δvalue| 0.000e+00 |
| 🟢 | S1 | D arc → radius | Radius 1 · Arc6 · |Δvalue| 0.000e+00 |
| 🟢 | S1 | D point + line refused | refusal: Dimension: a point-to-line distance is not in the solver yet |
| 🟢 | S1 | D 2 lines refused | refusal: Dimension: takes one line (length), two points (distance) or one circle / arc (radius) |
| 🟢 | S1 | N 2 lines → angle +90° | Angle 90° · Line1 · Line2 · |Δvalue| 0.000e+00 |
| 🟢 | S1 | N reversed → angle −90° | Angle -90° · Line2 · Line1 · |Δvalue| 0.000e+00 |
| 🟢 | S1 | H line | Horizontal · Line1 · |Δvalue| 0.000e+00 |
| 🟢 | S1 | H 2 points → level | Vertical distance 0 · Line1.Start · Point11 · |Δvalue| 0.000e+00 |
| 🟢 | S1 | V 2 points → plumb | Horizontal distance 0 · Point11 · Line1.Start · |Δvalue| 0.000e+00 |
| 🟢 | S1 | V circle refused | refusal: Vertical: takes one line or two points |
| 🟢 | S1 | O 2 points → coincident | Coincident · Line1.Start · Point11 · |Δvalue| 0.000e+00 |
| 🟢 | S1 | O point + line (any order) | Point on line · Point11 · Line1 · |Δvalue| 0.000e+00 |
| 🟢 | S1 | O endpoint + its line refused | refusal: On point / curve: Line1.Start already ends Line1 |
| 🟢 | S1 | O point + circle | Point on circle · Point11 · Circle5 · |Δvalue| 0.000e+00 |
| 🟢 | S1 | O 2 circles → concentric | Concentric · Circle5 · Arc6 · |Δvalue| 0.000e+00 |
| 🟢 | S1 | Q 2 lines | Equal length · Line1 · Line3 · |Δvalue| 0.000e+00 |
| 🟢 | S1 | Q circle + arc | Equal radius · Arc6 · Circle5 · |Δvalue| 0.000e+00 |
| 🟢 | S1 | Q line + circle refused | refusal: Equal: takes two lines or two circles / arcs |
| 🟢 | S1 | M point + line | Midpoint · Point11 · Line1 · |Δvalue| 0.000e+00 |
| 🟢 | S1 | Y 2 points + line | Symmetric · Line1.Start · Point11 · Line2 · |Δvalue| 0.000e+00 |
| 🟢 | S1 | L 2 lines → parallel | Parallel · Line1 · Line3 · |Δvalue| 0.000e+00 |
| 🟢 | S1 | L circle + line → tangent (line first) | Tangent · Line1 · Circle5 · |Δvalue| 0.000e+00 |
| 🟢 | S1 | L 2 circles apart → external tangent | Tangent · Circle5 · Arc6 · |Δvalue| 0.000e+00 |
| 🟢 | S1 | [ 2 lines → perpendicular | Perpendicular · Line1 · Line2 · |Δvalue| 0.000e+00 |
| 🟢 | S1 | ] point → fixed where it is | Fixed (1.5, -1) · Point11 · |Δvalue| 0.000e+00 |
| 🟢 | S1 | spline curve refused | refusal: Horizontal: Spline7 is a spline: constrain its defining points instead |
| 🟢 | S1 | nothing picked refused | refusal: Lock point: pick points or curves of the sketch first |
| 🟢 | S1 | same point twice refused | refusal: On point / curve: the same point is picked twice |
| 🟢 | S1 | point out of range refused | refusal: Lock point: a picked point is not in the sketch |
| 🟢 | S1 | L nested circles → internal tangent | Tangent (inside) · Circle1 · Circle2 |
| 🟢 | S2 | length 3-4-5 side | measured 5.000e+00 vs 5.000e+00 |
| 🟢 | S2 | distance A → B | measured 5.000e+00 vs 5.000e+00 |
| 🟢 | S2 | horizontal distance B − A | measured 3.000e+00 vs 3.000e+00 |
| 🟢 | S2 | vertical distance A − B (signed) | measured -4.000e+00 vs -4.000e+00 |
| 🟢 | S2 | arc radius |CS| | measured 1.000e+00 vs 1.000e+00 |
| 🟢 | S2 | angle base → slant +30° | measured 5.236e-01 vs 5.236e-01 |
| 🟢 | S2 | angle slant → down −120° | measured -2.094e+00 vs -2.094e+00 |
| 🟢 | S2 | angle base → reversed base 180° | measured 3.142e+00 vs 3.142e+00 |
| 🟢 | S2 | non-dimension kind → 0 | measured 0.000e+00 vs 0.000e+00 |
| 🟢 | S2 | dimension kinds | Distance, H / V distance, Length, Angle, Radius carry values; Fixed, Parallel do not |
| 🟢 | S3 | DOF after each verb | start 4 → ] 2 → D 3 1 → D 2 0 |
| 🟢 | S3 | corners after D 3 / D 2 | (0.3, 0.2) (3.3, 0.2) (3.3, 2.2) (0.3, 2.2) · worst 4.441e-16 |
| 🟢 | S3 | every corner fully constrained | PointFixed ×4, CurveFixed ×4 |
| 🟢 | S3 | second H redundant | Redundant · partners include H1: yes |
| 🟢 | S3 | D Line3 = 3 redundant | Redundant · 0 conflict group(s) |
| 🟢 | S3 | D Line3 = 4 conflicting | Conflicting · 1 conflict group(s) |
| 🟢 | S4 | free corner pinned, rest follows minimally | P0 (0, 0) P1 (4, 0) P2 (4, 3) P3 (0, 3) · worst 3.925e-17 · DOF 4 |
| 🟢 | S4 | fully constrained corner stays | pinned no · travel 0.000e+00 |
| 🟢 | S4 | bead slides onto the circle | |P| − 2 = 2.220e-16 · |P − (2, 0)| = 3.140e-16 · pinned no |
| 🟢 | S4 | point out of range refused | Drag: the point is not in the sketch |
| 🟢 | S5 | Line1.Start | Line1.Start |
| 🟢 | S5 | Line1.End | Line1.End |
| 🟢 | S5 | Circle5.Centre | Circle5.Centre |
| 🟢 | S5 | Arc6.Centre | Arc6.Centre |
| 🟢 | S5 | Arc6.End | Arc6.End |
| 🟢 | S5 | Spline7.Point1 | Spline7.Point1 |
| 🟢 | S5 | Point11 | Point11 |
| 🟢 | S5 | Arc6 | Arc6 |
| 🟢 | S5 | Horizontal · Line1 | Horizontal · Line1 |
| 🟢 | S5 | Length 3 · Line1 | Length 3 · Line1 |
| 🟢 | S5 | Angle 90° · Line1 · Line2 | Angle 90° · Line1 · Line2 |
| 🟢 | S5 | Point on circle · Point11 · Circle5 | Point on circle · Point11 · Circle5 |
| 🟢 | S5 | Fixed (1.5, -1) · Point11 | Fixed (1.5, -1) · Point11 |
| 🟢 | S5 | Symmetric · Line1.Start · Point11 · Line2 | Symmetric · Line1.Start · Point11 · Line2 |
| 🟢 | S5 | SolveSpace keys | DNHVOQMYL[] |
| 🟢 | S6 | tangent line–circle | |dist(centre, line) − r| = 2.220e-16 |
| 🟢 | S6 | angle typed 30° | |θ − 30°| = 1.110e-16 rad |
| 🟢 | S6 | equal radius | |r₁ − r₂| = 4.441e-16 |
| 🟢 | S6 | midpoint | |P11 − mid(Line1)| = 1.110e-16 |
| 🟢 | S6 | concentric | |C₁ − C₂| = 0.000e+00 |
| 🟢 | S6 | symmetric | midpoint on the axis 7.720e-17 · chord ⟂ axis 5.204e-17 |
| 🟢 | S6 | perpendicular | |cos| = 4.478e-15 |
