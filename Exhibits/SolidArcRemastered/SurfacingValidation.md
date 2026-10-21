# Surfacing validation

🟢 Vehicle plan V2 (curve fitting) and V4 (loft): points → fair B-splines within 0.2 mm → exact skinned NURBS sheet

## Gates

| | Gate | Measures | Limit | Worst | Checks | Failures |
|---|---|---|---|---|---|---|
| 🟢 | S1 | Fit: every input point within the tolerance of the curve (noise 0 / 0.05 / 0.1 mm) | ≤ 0.2 mm | 1.58e-04 | 12 | 0 |
| 🟢 | S2 | Fit: fitted curve vs the true curve stays within tolerance + noise amplitude; poles ≤ points / 2 | ≤ 0.2 mm + a | 1.62e-04 | 12 | 0 |
| 🟢 | S3 | Exact recovery: B-spline samples at their own parameters → poles; Bézier samples with parameter correction → curve | ≤ 10⁻¹²·L · ≤ 10⁻⁹ | 1.02e-14 | 3 | 0 |
| 🟢 | S4 | Corners split exactly where seeded (90°, 45°); runs share their joint points bit-exactly | exact | 0.00e+00 | 1 | 0 |
| 🟢 | S5 | Refusals name the reason (and the worst point for a span-limit refusal) | all named | 0.00e+00 | 8 | 0 |
| 🟢 | L1 | Loft interpolates every section: S(𝑢, 𝑣ₖ) vs the merged section, merged vs the original (knot insertion) | ≤ 10⁻⁹ | 7.07e-16 | 5 | 0 |
| 🟢 | L2 | Lofted circles reproduce a cylinder and a cone, two lines a bilinear patch (41 × 41 samples) | ≤ 10⁻¹² | 3.40e-16 | 3 | 0 |
| 🟢 | L3 | Sheet body: audit sound · 1 face · every unpaired side on the laminar rim · tessellation conforming · area (quadrature) vs closed form | rel. ≤ 10⁻⁹ | 9.30e-12 | 5 | 0 |
| 🟢 | L4 | Synthetic bonnet: 6 fitted sections lofted; deviation from all limit points and from the analytic surface | ≤ 0.5 mm | 5.45e-05 | 3 | 0 |
| 🟢 | L5 | Same input twice → bit-identical fitted poles and loft poles | equal | 0.00e+00 | 1 | 0 |

## Fitting (S1 S2) · tolerance 0.2 mm · fairing λ ≤ 10⁻⁷ (lowered by decades only when it would cost tolerance)

| | Curve | Noise a [mm] | Points | Poles | Spans | λ used | worst [mm] | mean [mm] | vs true curve [mm] | κ maxima | Note |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 🟢 | Shoulder line (3D) | 0.00 | 60 | 7 | 4 | 1e-07 | 0.0369 | 0.0143 | 0.0377 | 1 |  |
| 🟢 | Shoulder line (3D) | 0.05 | 60 | 7 | 4 | 1e-07 | 0.0628 | 0.0254 | 0.0362 | 1 |  |
| 🟢 | Shoulder line (3D) | 0.10 | 60 | 7 | 4 | 1e-07 | 0.0993 | 0.0472 | 0.0592 | 1 |  |
| 🟢 | Wheel arch (semicircle r 0.36) | 0.00 | 40 | 9 | 6 | 1e-08 | 0.0601 | 0.0215 | 0.0611 | 7 |  |
| 🟢 | Wheel arch (semicircle r 0.36) | 0.05 | 40 | 9 | 6 | 1e-08 | 0.0883 | 0.0279 | 0.0719 | 7 |  |
| 🟢 | Wheel arch (semicircle r 0.36) | 0.10 | 40 | 10 | 7 | 1e-08 | 0.0974 | 0.0420 | 0.0994 | 8 |  |
| 🟢 | Helix (1 turn, r 0.3) | 0.00 | 120 | 13 | 10 | 1e-09 | 0.1337 | 0.0346 | 0.1343 | 11 |  |
| 🟢 | Helix (1 turn, r 0.3) | 0.05 | 120 | 13 | 10 | 1e-09 | 0.1573 | 0.0399 | 0.1400 | 11 |  |
| 🟢 | Helix (1 turn, r 0.3) | 0.10 | 120 | 15 | 12 | 1e-09 | 0.1583 | 0.0507 | 0.1624 | 11 |  |
| 🟢 | Roof S-curve (planar) | 0.00 | 80 | 19 | 16 | 1e-07 | 0.1281 | 0.0212 | 0.1287 | 2 |  |
| 🟢 | Roof S-curve (planar) | 0.05 | 80 | 19 | 16 | 1e-07 | 0.1367 | 0.0331 | 0.1368 | 3 |  |
| 🟢 | Roof S-curve (planar) | 0.10 | 80 | 19 | 16 | 1e-07 | 0.1337 | 0.0488 | 0.1422 | 4 |  |

## Exact recovery (S3)

| | Case | Measured | Limit |
|---|---|---|---|
| 🟢 | B-spline (8 poles, knots 0.15 · 0.4 · 0.55 · 0.8) from 50 samples at its parameters: max pole error / L | 8.29e-16 | ≤ 10⁻¹² |
| 🟢 | Bézier from 40 samples (unknown parameters, 1 span): worst point deviation | 1.17e-15 | ≤ 10⁻⁹ |
| 🟢 | … and its poles vs the true poles | 1.02e-14 | ≤ 10⁻⁷ |

## Corners (S4)

🟢 corners at 19 34 (seeded 19 34) · 3 runs · joints bit-exact

## Refusals (S5)

| | Case | Expected | Measured |
|---|---|---|---|
| 🟢 | One point | fewer than 2 distinct points | Fit: fewer than 2 distinct points |
| 🟢 | Three equal points | fewer than 2 distinct points | Fit: fewer than 2 distinct points |
| 🟢 | Tolerance 0 | tolerance must be positive | Fit: tolerance must be positive |
| 🟢 | 20 mm outlier at point 37, span limit 4 | point 37 stays | Fit: point 37 stays 18.7 mm away at 4 spans (limit 4, tolerance 0.2 mm) |
| 🟢 | Loft of one section | needs at least 2 sections | Loft: needs at least 2 sections |
| 🟢 | Cubic + circle (degree 2) | degree elevation | Loft: sections of degree 3 and 2 (degree elevation 🚧) |
| 🟢 | Closed circle + open arc | open and closed sections mixed | Loft: open and closed sections mixed (section 1) |
| 🟢 | Same section twice | coincide | Loft: all sections coincide |

## Loft (L1 L2 L3)

| | Specimen | Sections | Poles 𝑢 × 𝑣 | Rim edges | S(𝑢, 𝑣ₖ) vs section | merged vs original | vs exact surface | Area | area rel. | Note |
|---|---|---|---|---|---|---|---|---|---|---|
| 🟢 | Cylinder r 0.4 (4 circles, uneven heights 0 · 0.3 · 0.45 · 1) | 4 | 9 × 4 | 3 | 2.8e-16 | 2.2e-16 | 2.2e-16 | 2.513274123 | 9.3e-12 |  |
| 🟢 | Cone r = 0.5 − 0.2 z (3 circles at 0 · 0.5 · 1.2) | 3 | 9 × 3 | 3 | 2.8e-16 | 4.6e-16 | 2.2e-16 | 2.921873305 | 9.3e-12 |  |
| 🟢 | Twisted lines → bilinear patch | 2 | 2 × 2 | 4 | 0.0e+00 | 2.4e-16 | 3.4e-16 | 1.071167812 | — |  |
| 🟢 | Three cubics, different knots (middle one reversed) | 3 | 9 × 3 | 4 | 4.7e-16 | 5.6e-16 | — | 0.861647778 | — |  |
| 🟢 | Synthetic bonnet (6 fitted sections) | 6 | 4 × 6 | 4 | 7.1e-16 | 0.0e+00 | — | 0.837643672 | — |  |

## Bonnet panel (L4) · 156 limit points → 24 section poles → surface 4 × 6 poles

| | Measured against | Samples | worst [mm] | mean [mm] |
|---|---|---|---|---|
| 🟢 | Limit points of the 6 section loops (fitted, 0.05 mm noise) | 156 | 0.0382 | 0.0157 |
| 🟢 | Limit points of 5 loops between the sections (never seen by the loft) | 130 | 0.0545 | 0.0184 |
| 🟢 | Analytic surface H(x, y), normal distance at 61 × 61 (𝑢, 𝑣) | 3721 | 0.0334 | 0.0106 |
