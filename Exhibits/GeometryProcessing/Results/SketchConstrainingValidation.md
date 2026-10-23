# Sketch constraining validation

🟢 Constraints C1 + L1: SolveSpace constraint verbs on picked sketch points / curves, dimensions, diagnosis, point drag; spline G1 / G2 / on / end direction, reference dimensions, linked geometry

## Gates

| | Gate | Measures | Limit | Worst | Checks | Failures |
|---|---|---|---|---|---|---|
| 🟢 | S1 | Verb table: D N H V O Q M Y L [ ] on lines, points, circles, arcs → the expected kind, references in order, measured value vs closed form; wrong picks refused by name | kind exact · value ≤ 10⁻¹² | 0.00e+00 | 32 | 0 |
| 🟢 | S2 | Measuring: length 5 of a 3-4-5 side, distance, signed horizontal / vertical distances, arc radius, angles +30° / −120° / 180° | ≤ 10⁻¹² | 0.00e+00 | 10 | 0 |
| 🟢 | S3 | Drawn rectangle (H / V, 4 DOF) → ] corner, D width typed 3, D height typed 2: DOF 4 → 2 → 1 → 0, corners exact; a second H is redundant (partner H1), D on the far side 3 redundant / 4 conflicting | ≤ 10⁻¹² | 4.44e-16 | 6 | 0 |
| 🟢 | S4 | Drag: free rectangle corner pinned at (4, 3) with the rest following minimally (P0 still), H / V held; the fully constrained one does not move; a point on a fixed circle lands on it nearest the target | ≤ 10⁻⁹ | 3.14e-16 | 4 | 0 |
| 🟢 | S5 | Names and descriptions: Line1.Start, Circle5.Centre, Arc6.End, Spline7.Point1, Point11; "Length 3 · Line1", "Angle 90° · Line1 · Line2" | exact text | 0.00e+00 | 15 | 0 |
| 🟢 | S6 | Solved verbs: tangent line–circle (distance = r), angle typed 30°, equal radius, midpoint, symmetric, concentric, perpendicular | ≤ 10⁻¹² | 4.48e-15 | 7 | 0 |
| 🟢 | S7 | Spline verbs: O point → on spline, H / V spline + end → end direction, L / C spline + line / arc / spline meeting at an end (shared or coincident) → G1 / G2; defining point, interior point, apart, closed, cubic-spline G2 and D refused by name | kind exact | 0.00e+00 | 16 | 0 |
| 🟢 | S8 | Spline solves: G1 to a fixed line in all 4 forms, G2 Hermite → NURBS, G2 NURBS → arc (κ = 1 / r), G2 Bezier → line (κ = 0), point on spline, H / V end directions; DOF 6 → 5 (G1) → 4 (G2); G1 + G2 on one joint → redundant (partner named) | sin, |Δκ|·L, distance ≤ 10⁻¹² | 1.09e-14 | 11 | 0 |
| 🟢 | S9 | Reference (driven) dimensions: on a fully constrained rectangle the diagonal reads √13, then 5 after the height becomes 4; status Driven, DOF unchanged, positions bit-equal with and without it; reference angle; text "(reference)" | ≤ 10⁻¹² | 0.00e+00 | 7 | 0 |
| 🟢 | S10 | Linked geometry: a projected point is held bit-exact (2 unknowns out), a line end coincident with it lands on it, Fixed on it redundant / conflicting, a linked circle keeps its radius, drag refused; feature tree: Sketch#2 (z = 1) follows Sketch#1 when Width 3 → 4, XZ sketch gets (x, z) = (4, 0), dangling link noted and held | ≤ 10⁻¹² | 4.09e-13 | 10 | 0 |

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
| 🟢 | S1 | spline curve refused | refusal: Equal: Spline7 is a spline: it takes O, L, C and H / V (constrain its defining points for the rest) |
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
| 🟢 | S7 | O point + spline → on spline | Point on spline · Point14 · Spline2 |
| 🟢 | S7 | H spline + start → horizontal end | Horizontal end · Line1.End |
| 🟢 | S7 | V spline + end → vertical end | Vertical end · Spline2.Point3 |
| 🟢 | S7 | L spline + line (shared point) → G1 | Tangent (G1) · Spline2 · Line1 |
| 🟢 | S7 | C spline + line → G2 | Curvature (G2) · Spline2 · Line1 |
| 🟢 | S7 | L spline + arc (coincident ends) → G1 | Tangent (G1) · Spline2 · Arc3 |
| 🟢 | S7 | C spline + spline (coincident ends) → G2 | Curvature (G2) · Spline2 · Spline4 |
| 🟢 | S7 | O defining point refused | refusal: On point / curve: Spline2.Point1 defines Spline2 |
| 🟢 | S7 | H interior point refused | refusal: Horizontal: takes a spline and one of its end points (the end tangent turns level / plumb) |
| 🟢 | S7 | L apart refused | refusal: Parallel / tangent: Spline2 and Line6 do not meet at an end (O their ends together first) |
| 🟢 | S7 | L closed spline refused | refusal: Parallel / tangent: Spline5 is closed: it has no end |
| 🟢 | S7 | C closed spline refused | refusal: Curvature (G2): Spline5 is closed: it has no end |
| 🟢 | S7 | D on a spline refused | refusal: Dimension: Spline2 is a spline: it takes O, L, C and H / V (constrain its defining points for the rest) |
| 🟢 | S7 | C two lines refused | refusal: Curvature (G2): takes a spline and the curve it meets at an end |
| 🟢 | S7 | C natural cubic spline refused | refusal: Curvature (G2): Spline2 is a cubic spline: its ends have zero curvature by construction (use Hermite, Bezier or NURBS for G2) |
| 🟢 | S7 | key C, label | C · Curvature (G2) |
| 🟢 | S8 | G1 to a line · Hermite | |sin| = 9.100e-17 · DOF 5 |
| 🟢 | S8 | G1 to a line · Bezier | |sin| = 0.000e+00 · DOF 5 |
| 🟢 | S8 | G1 to a line · NURBS | |sin| = 0.000e+00 · DOF 5 |
| 🟢 | S8 | G1 to a line · Cubic spline | |sin| = 4.928e-16 · DOF 5 |
| 🟢 | S8 | G2 Hermite → NURBS | |sin| 7.220e-17 · |Δκ|·L 5.079e-16 · κ 1.431e-01 |
| 🟢 | S8 | G2 NURBS → arc | |sin| 2.646e-16 · |κ ∓ 1/r|·L 1.092e-14 |
| 🟢 | S8 | G2 Bezier → line | |sin| 7.398e-19 · |κ|·L 2.930e-17 · DOF 4 |
| 🟢 | S8 | point on spline | distance 0.000e+00 · moved 3.845e-01 |
| 🟢 | S8 | H start · V end | |sin| 9.100e-17 · 7.837e-16 · Horizontal end · Line1.End |
| 🟢 | S8 | DOF 6 → 5 → 4 | 6 → 5 → 4 |
| 🟢 | S8 | G1 + G2 on one joint → redundant | Redundant · partner G1 named |
| 🟢 | S9 | diagonal reads √13 | Distance 3.60555 (reference) · Line1.Start · Line2.End · |Δ| 0.000e+00 |
| 🟢 | S9 | DOF unchanged, no conflict | DOF 0 → 0 · conflicts 0 |
| 🟢 | S9 | positions bit-equal with / without | bit-equal |
| 🟢 | S9 | height 4 → diagonal 5 | Distance 5 (reference) · Line1.Start · Line2.End · |Δ| 0.000e+00 |
| 🟢 | S9 | text | Distance 5 (reference) · Line1.Start · Line2.End |
| 🟢 | S9 | reference angle 90° | Angle 90° (reference) · Line1 · Line2 · |Δ| 0.000e+00 |
| 🟢 | S9 | one point refused | refusal: Reference dimension: takes one line, two points, one circle / arc or two lines |
| 🟢 | S10 | linked point held, line end lands on it | held bit-exact · gap 0.000e+00 · DOF 2 · linked unknowns 2 |
| 🟢 | S10 | linked point counts as fixed | fixed |
| 🟢 | S10 | Fixed at its place → redundant | Redundant |
| 🟢 | S10 | Fixed elsewhere → conflicting | Conflicting |
| 🟢 | S10 | drag refused | refusal: Drag: Point0 is linked to Sketch#1 (move it there) |
| 🟢 | S10 | linked circle keeps r = 1 | r 0.000e+00 · tangent gap 4.091e-13 |
| 🟢 | S10 | Sketch#2 line ends on the corner (3, 2) | gap 0.000e+00 · built yes |
| 🟢 | S10 | Width 3 → 4: Sketch#2 follows to (4, 2) | gap 0.000e+00 |
| 🟢 | S10 | XZ sketch gets (x, z) = (4, 0) | (4.000e+00, 0.000e+00) |
| 🟢 | S10 | dangling link noted and held | Point0: link to Sketch#9 is dangling (held where it was) |
