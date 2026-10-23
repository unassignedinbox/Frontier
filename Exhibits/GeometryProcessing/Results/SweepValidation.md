# Sweep validation

🟢 Revolve / sweep R1–R2: spline revolutions as exact NURBS, revolve about point + direction, Evolution (profile along a path)

## Gates

| | Gate | Measures | Limit | Worst | Checks | Failures |
|---|---|---|---|---|---|---|
| 🟢 | R1 | Spline revolve: a vase wall (NURBS spline) turned a full turn about a profile edge — every sampled point of the NURBS face sits at the spline's (r, z), volume = 2π·A·r̄ (Pappus, exact profile area / centroid), audit sound | ≤ 10⁻¹² · rel. ≤ 10⁻⁹ | 7.45e-16 | 1 | 0 |
| 🟢 | R2 | Partial spline revolve 120° about the sketch x-axis given as point + direction (no axis line): volume = θ·A·ȳ, two plane caps, far cap = the profile turned by θ | ≤ 10⁻¹² · rel. ≤ 10⁻⁹ | 9.49e-16 | 1 | 0 |
| 🟢 | R3 | Spline ending on the axis (dome): the pole collapses to one vertex, volume by Pappus, audit sound | rel. ≤ 10⁻⁹ | 1.71e-16 | 1 | 0 |
| 🟢 | R4 | Evolution along an oblique line: square with a round hole, every side a NURBS skin of the exact carried section, volume A·Δz, end cap = start cap + d | ≤ 10⁻¹² · rel. ≤ 10⁻⁹ | 9.82e-16 | 1 | 0 |
| 🟢 | R5 | Evolution along a quarter arc: rectangle ⟂ the path sweeps the revolve solid — volume A·R̄·θ, end vertices = start vertices turned 90° about the arc axis | ≤ 10⁻⁵·scale · rel. ≤ 10⁻⁶ | 7.39e-07 | 1 | 0 |
| 🟢 | R6 | Evolution along a non-planar spline: profile ⟂ the path with its centroid on it — volume A·L (rotation-minimising frames), circle profile = pipe volume, audit sound | ≤ 10⁻⁵·scale · rel. ≤ 10⁻⁵ | 8.70e-06 | 2 | 0 |
| 🟢 | R7 | Feature steps: Revolve with AxisOrigin / AxisDirection, Evolution from a profile sketch along a path sketch (spline), Evolution subtracted from a box; face names Side(curve) / Cap(Start) / Cap(End) | rel. ≤ 10⁻⁶ | 4.72e-08 | 4 | 0 |
| 🟢 | R8 | Refusals name the cause: spline crossing the axis, axis without direction, path along the profile plane, profile beyond the bend, closed path, path = profile, multi-curve path | all named | 0.00e+00 | 7 | 0 |

## Checks

| | Gate | Check | Detail |
|---|---|---|---|
| 🟢 | R1 | Vase (line · NURBS wall · line · axis edge), 360° | faces 3 · (r, z) gap 7.448e-16 · volume 5.438366163 vs 2π·A·r̄ 5.438366163 (rel. 3.266e-16) · sound |
| 🟢 | R2 | Rectangle with a NURBS top, 120° about the sketch x-axis | faces 6 (caps 2) · (r, z) gap 9.486e-16 · far cap gap 0.000e+00 · volume 1.327243110 vs θ·A·ȳ 1.327243110 (rel. 3.346e-16) · sound |
| 🟢 | R3 | Dome: NURBS wall from (1, 0) up to the axis at (0, 1.2) | faces 2 · vertices 2 · volume 2.591813939 vs 2π·A·r̄ 2.591813939 (rel. 1.713e-16) · sound |
| 🟢 | R4 | Square − hole r 0.25 along d = (0.6, 0.3, 2) | 9 stations · deviation 9.821e-16 · faces 7 · end corners gap 2.483e-16 · volume 1.607300918 vs A·Δz 1.607300918 (rel. 5.526e-16) · sound |
| 🟢 | R5 | 0.4 × 0.2 rectangle along a quarter arc R 2 | 33 stations · deviation 3.306e-07 (≤ 4.472e-06) · end corners gap 1.471e-16 · volume 0.251327411 vs A·R̄·θ 0.251327412 (rel. 5.276e-09) · sound |
| 🟢 | R6 | Rectangle 0.24 × 0.12, centroid on the path | 257 stations · deviation 2.335e-06 (≤ 2.683e-06) · reach·κmax 0.102 · faces 6 · volume 0.126739350 vs A·L 0.126739348 (rel. 1.443e-08) · sound |
| 🟢 | R6 | Circle r 0.1 (the pipe) | 513 stations · deviation 5.339e-07 (≤ 2.000e-06) · reach·κmax 0.076 · faces 3 · volume 0.138251184 vs A·L 0.138251182 (rel. 1.464e-08) · sound |
| 🟢 | R7 | Revolve#2 about AxisOrigin (0, 0) + t·(0, 1) | volume 3.926990817 vs π(R² − r²)h 3.926990817 (rel. 6.785e-16) |
| 🟢 | R7 | Evolution#3: Sketch#1 along Sketch#2 (spline) | faces Evolution#3.Cap(End), Evolution#3.Cap(Start), Evolution#3.Side(Line1), Evolution#3.Side(Line2), Evolution#3.Side(Line3), Evolution#3.Side(Line4) · volume 0.053872981 vs A·L 0.053872984 (rel. 4.723e-08) |
| 🟢 | R7 | Evolution#3 along a line drawn from its far end | volume 0.040000000 vs A·L 0.04 (rel. 5.204e-16) |
| 🟢 | R7 | Evolution#5 subtracted from a block | volume 3.633680744 < block 3.680000000 · sound |
| 🟢 | R8 | Spline crossing the axis | refused: Revolve: profile crosses the axis |
| 🟢 | R8 | Axis without direction | refused: Revolve#2: Revolve: the axis has no direction |
| 🟢 | R8 | Path along the profile plane | refused: Evolution: the path starts along the profile plane (it must leave the plane) |
| 🟢 | R8 | Profile beyond the bend | refused: Evolution: the profile reaches 1.414214 from the path start, beyond the tightest bend radius 0.500000 (the sweep would fold over itself) |
| 🟢 | R8 | Closed path sketch | refused: Evolution#3: Evolution: the path Circle1 is closed (🚧 closed sweeps) |
| 🟢 | R8 | Path = profile | refused: Evolution#2: Evolution: the path must be another sketch than the profile |
| 🟢 | R8 | Multi-curve path | refused: Evolution#3: Evolution: path sketch Sketch#2 has 2 curves (🚧 one line, arc or spline; J joins a chain into a spline first) |
