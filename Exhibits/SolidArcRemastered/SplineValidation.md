# Spline validation

🟢 Phase B freeform sketch curves: Hermite · Bezier · NURBS · cubic spline (Plasticity curve keys 1 … 4)

## Gates

| | Gate | Measures | Limit | Worst | Checks | Failures |
|---|---|---|---|---|---|---|
| 🟢 | C1 | Interpolating forms pass through every point; control forms start and end on the first and last point | ≤ 10⁻¹²·L | 0.00e+00 | 16 | 0 |
| 🟢 | C2 | Joint continuity from both sides (left limit = reversed curve): Hermite C¹, cubic spline and cubic NURBS C², closed seams too | ≤ 10⁻¹¹·L | 1.65e-15 | 9 | 0 |
| 🟢 | C3 | Kernel B-spline vs independent oracle (dense tangent solve, de Casteljau, recursive Cox–de Boor): C, C′, C″ at 64 samples per span | ≤ 10⁻¹¹·L | 7.21e-14 | 16 | 0 |
| 🟢 | C4 | Profile area and centroid vs exact power-basis integration of the oracle pieces | rel. ≤ 10⁻¹² | 9.11e-16 | 9 | 0 |
| 🟢 | C5 | Extrude: audit sound · volume = A·H · one NURBS side per spline · watertight, conforming, outward tessellation | rel. ≤ 10⁻⁹ | 1.21e-15 | 9 | 0 |
| 🟢 | C6 | Refusals with the expected reason (unknown form, too few points, coincident, degree, open, self-crossing, revolve) | all named | 0.00e+00 | 9 | 0 |
| 🟢 | C7 | Spline points solve as ordinary unknowns; curve constraints naming a spline are Unsupported and left out | all equal | 0.00e+00 | 3 | 0 |
| 🟢 | C8 | Same input twice → bit-identical poles, area and volume | equal | 0.00e+00 | 1 | 0 |

## Curves (C1 C2 C3)

| | Specimen | Form | Degree | Poles | Spans | through points / L | joint jump / L | vs oracle / L |
|---|---|---|---|---|---|---|---|---|
| 🟢 | Wave | Hermite | 3 | 16 | 5 | 0.0e+00 | 1.8e-16 | 1.5e-15 |
| 🟢 | Pebble (closed) | Hermite | 3 | 19 | 6 | 0.0e+00 | 1.1e-15 | 1.8e-15 |
| 🟢 | Chord | Hermite | 3 | 4 | 1 | 0.0e+00 | 0.0e+00 | 4.2e-16 |
| 🟢 | Hook | Hermite | 3 | 7 | 2 | 0.0e+00 | 0.0e+00 | 1.3e-15 |
| 🟢 | Wave | Bezier | 5 | 6 | 1 | 0.0e+00 | 0.0e+00 | 1.1e-14 |
| 🟢 | Pebble (closed) | Bezier | 6 | 7 | 1 | 0.0e+00 | 0.0e+00 | 7.2e-14 |
| 🟢 | Chord | Bezier | 1 | 2 | 1 | 0.0e+00 | 0.0e+00 | 0.0e+00 |
| 🟢 | Hook | Bezier | 2 | 3 | 1 | 0.0e+00 | 0.0e+00 | 2.4e-16 |
| 🟢 | Wave | NURBS | 3 | 6 | 3 | 0.0e+00 | 7.9e-17 | 3.9e-15 |
| 🟢 | Pebble (closed) | NURBS | 3 | 7 | 4 | 0.0e+00 | 1.8e-16 | 1.3e-14 |
| 🟢 | Chord | NURBS | 1 | 2 | 1 | 0.0e+00 | 0.0e+00 | 3.5e-17 |
| 🟢 | Hook | NURBS | 2 | 3 | 1 | 0.0e+00 | 0.0e+00 | 6.0e-16 |
| 🟢 | Wave | Cubic spline | 3 | 16 | 5 | 0.0e+00 | 1.6e-15 | 1.9e-15 |
| 🟢 | Pebble (closed) | Cubic spline | 3 | 19 | 6 | 0.0e+00 | 1.3e-15 | 2.1e-15 |
| 🟢 | Chord | Cubic spline | 3 | 4 | 1 | 0.0e+00 | 0.0e+00 | 4.2e-16 |
| 🟢 | Hook | Cubic spline | 3 | 7 | 2 | 0.0e+00 | 6.7e-16 | 1.7e-15 |

## Profiles and extrusion (C4 C5) · H = 1.5 on a tilted plane

| | Profile | Loops | Splines | Area | area rel. | centroid / L | Volume | volume rel. | Faces | NURBS faces | Triangles | Note |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 🟢 | Arch · cubic spline | 1 | 1 | 7.831428571 | 1.1e-16 | 2.8e-16 | 11.747142857 | 4.5e-16 | 4 | 1 | 462 |  |
| 🟢 | Arch · Hermite | 1 | 1 | 7.722000000 | 3.5e-16 | 2.1e-16 | 11.583000000 | 6.1e-16 | 4 | 1 | 556 |  |
| 🟢 | Arch · Bezier | 1 | 1 | 5.396571429 | 6.6e-16 | 1.7e-16 | 8.094857143 | 4.4e-16 | 4 | 1 | 510 |  |
| 🟢 | Arch · NURBS | 1 | 1 | 8.767291667 | 0.0e+00 | 2.8e-16 | 13.150937500 | 2.7e-16 | 4 | 1 | 772 |  |
| 🟢 | Pebble · cubic spline (closed) | 1 | 1 | 9.180500000 | 1.9e-16 | 2.0e-16 | 13.770750000 | 6.4e-16 | 3 | 1 | 1002 |  |
| 🟢 | Pebble · Hermite (closed) | 1 | 1 | 8.959375000 | 7.9e-16 | 4.3e-16 | 13.439062500 | 1.3e-16 | 3 | 1 | 1102 |  |
| 🟢 | Pebble · NURBS (closed, G⁰ seam) | 1 | 1 | 6.214000000 | 2.9e-16 | 1.5e-16 | 9.321000000 | 5.7e-16 | 3 | 1 | 1098 |  |
| 🟢 | Plate with pebble hole | 2 | 1 | 46.819500000 | 9.1e-16 | 2.1e-16 | 70.229250000 | 1.2e-15 | 7 | 1 | 524 |  |
| 🟢 | Two splines and two lines | 1 | 2 | 17.795375000 | 2.0e-16 | 1.4e-16 | 26.693062500 | 2.7e-16 | 6 | 2 | 544 |  |

## Refusals (C6)

| | Case | Expected | Measured |
|---|---|---|---|
| 🟢 | Unknown form 7 (hand-edited document) | unknown spline form 7 | unknown spline form 7 |
| 🟢 | One point | needs at least 2 points | Cubic spline needs at least 2 points |
| 🟢 | Repeated point | points 2 and 3 coincide | Hermite points 2 and 3 coincide |
| 🟢 | Closed on 2 distinct points | needs at least 3 distinct points | closed Cubic spline needs at least 3 distinct points |
| 🟢 | Bezier on 17 points | exceeds degree 15 | Bezier with 17 control points exceeds degree 15 (use NURBS) |
| 🟢 | Open spline profile | open profile | open profile: Spline21 ends at a point joined to 0 other curves |
| 🟢 | Self-crossing closed spline | crosses itself: Spline | profile crosses itself: Spline22 × Spline22 |
| 🟢 | Spline crossing a line | crosses itself | profile crosses itself: Spline23 × Line24 |
| 🟢 | Revolve a spline profile | Revolve: 🚧 Spline | Revolve: 🚧 Spline25 is a spline (rational surface of revolution not built yet) |

## Solver (C7)

| | Check | Detail |
|---|---|---|
| 🟢 | Fixed spline points reach their targets | error 0.000000 |
| 🟢 | DOF 0 · spline and line flagged fixed | DOF 0 |
| 🟢 | Tangent(line, spline) and Horizontal(spline) → Unsupported, nothing else disturbed | Unsupported / Unsupported |
