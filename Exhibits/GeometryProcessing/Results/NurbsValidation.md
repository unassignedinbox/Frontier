# NURBS carrier validation (rational B-spline curves and surfaces as kernel carriers)

**🟢 PASS** · 27 reproduction probes · 5 surfaces · 4 curves · 17 bodies · 127684 ms

## Gates

| | Gate | Measures | Limit | Worst | Checks | Failures |
|---|---|---|---|---|---|---|
| 🟢 | N1 | Exact reproduction: NURBS circle / ellipse / line / plane / cylinder / cone / sphere / torus vs the analytic carrier (distance relative to scale / normal angle; the angle is conditioned by 1/cos 𝑣 beside a sphere pole) | ≤ 1e-14 / 1e-13 rad | 1.66e-14 | 54 | 0 |
| 🟢 | N2 | Rational derivatives C′, C″, S𝑢, S𝑣, S𝑢𝑢, S𝑢𝑣, S𝑣𝑣 vs Richardson differences inside a knot span (relative) | ≤ 1e-8 | 4.09e-09 | 9 | 0 |
| 🟢 | N3 | Inversion round trip: on-surface / on-curve points and offset points back to their feet (relative to extent) | ≤ 1e-12 | 1.20e-13 | 9 | 0 |
| 🟢 | N4 | Iso-curves ≡ surface rows, Bézier pieces ≡ curve, reversal ≡ curve (relative to extent) | ≤ 1e-14 | 6.68e-16 | 9 | 0 |
| 🟢 | N5 | Affine images: map(S)(𝑢,𝑣) vs M·S(𝑢,𝑣) (relative); bodies under non-uniform scale and shear: V = det·V*, there and back | ≤ 1e-12 / 1e-9 | 1.54e-15 | 9 | 0 |
| 🟢 | N6 | Promoted bodies: audit sound, closed-form volume (relative), watertight tessellation, containment oracle | ≤ 1e-9 | 9.57e-16 | 8 | 0 |
| 🟢 | N7 | Booleans with NURBS operands vs closed form or the all-analytic Boolean (relative volume) | ≤ 1e-9 | 1.68e-15 | 5 | 0 |

## N1 reproduction (worst over 41 × 41 or 401 samples per probe, 3 random frames)

| Carrier | Worst distance / scale | Worst normal angle [rad] |
|---|---|---|
| Circle (full turn) | 4.87e-16 | 0.00e+00 |
| Circle arc (−1, 2) | 5.87e-16 | 0.00e+00 |
| Ellipse (full turn) | 8.45e-16 | 0.00e+00 |
| Line (−3, 4) | 3.26e-16 | 0.00e+00 |
| Plane | 4.38e-16 | 6.89e-16 |
| Cylinder | 4.51e-16 | 1.97e-15 |
| Cone | 3.58e-16 | 1.71e-15 |
| Sphere | 4.06e-16 | 1.66e-14 |
| Torus | 7.44e-16 | 7.83e-15 |

## N5 affine bodies (V* = det M · V, then the inverse map back home)

| | Specimen | Faces | NURBS faces | V | V* | Relative error | Triangles | \|V − V_polygon\| | Bound | Oracle | ms | Outcome |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 🟢 | Cylinder r 1 × 2, scale (2, 1, 1) → elliptic cylinder | 3 | 1 | 12.566370614359 | 12.566370614359 | 0.0e+00 | 9086 | 6.2e-03 | 3.1e-02 | 0/0 | 4448 | there and back |
| 🟢 | Sphere r 1, general shear → ellipsoid | 1 | 1 | 6.136577650012 | 6.136577650012 | 4.2e-16 | 18324 | 6.3e-03 | 1.4e-02 | 0/0 | 7510 | there and back |
| 🟢 | Torus 2 / 0.5, scale (1, 1.5, 0.5) | 1 | 1 | 7.402203300817 | 7.402203300817 | 5.4e-16 | 32786 | 2.6e-02 | 7.9e-02 | 0/0 | 13695 | there and back |
| 🟢 | Cone r 1 × 1.5, general shear | 2 | 1 | 2.301216618755 | 2.301216618755 | 1.5e-15 | 9730 | 2.4e-03 | 1.0e-02 | 0/0 | 2784 | there and back |

## N6 promoted bodies (oracle: wrong / samples, closed-form membership)

| | Specimen | Faces | NURBS faces | V | V* | Relative error | Triangles | \|V − V_polygon\| | Bound | Oracle | ms | Outcome |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 🟢 | Block 2 × 1 × 0.5 | 6 | 6 | 1.000000000000 | 1.000000000000 | 3.3e-16 | 110 | 1.0e-15 | 2.5e-15 | 0/100 | 675 | promoted |
| 🟢 | Cylinder r 0.8 × 1.5 | 3 | 3 | 3.015928947446 | 3.015928947446 | 5.9e-16 | 9004 | 1.3e-03 | 6.3e-03 | 0/100 | 5771 | promoted |
| 🟢 | Cone r 0.8 × 1.2 | 2 | 2 | 0.804247719319 | 0.804247719319 | 5.5e-16 | 8220 | 6.1e-04 | 2.9e-03 | 0/100 | 4556 | promoted |
| 🟢 | Frustum r 0.9 → 0.4 × 1 | 3 | 3 | 1.392772743091 | 1.392772743091 | 9.6e-16 | 6022 | 8.2e-04 | 4.2e-03 | 0/0 | 1657 | promoted |
| 🟢 | Sphere r 0.9 | 1 | 1 | 3.053628059289 | 3.053628059289 | 7.3e-16 | 24658 | 1.9e-03 | 4.1e-03 | 0/100 | 11235 | promoted |
| 🟢 | Torus 1.2 / 0.35 | 1 | 1 | 2.901663693920 | 2.901663693920 | 7.7e-16 | 17802 | 5.1e-03 | 1.5e-02 | 0/100 | 10106 | promoted |
| 🟢 | Filleted block 2 × 1.5 × 1, r 0.2 on every edge | 26 | 26 | 2.856200551912 | 2.856200551912 | 3.1e-16 | 22582 | 8.7e-04 | 6.1e-03 | 0/0 | 7744 | promoted |
| 🟢 | Disc r 2 × 1 − radial bore r 0.3 (traced edges on NURBS faces) | 5 | 5 | 12.285222211670 | 12.285222211670 | 2.9e-16 | 6148 | 3.8e-03 | 4.5e-02 | 0/0 | 4220 | promoted (reference: analytic body) |

## N7 Booleans with NURBS operands

| | Specimen | Faces | NURBS faces | V | V* | Relative error | Triangles | \|V − V_polygon\| | Bound | Oracle | ms | Outcome |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 🟢 | NURBS sphere r 1 − analytic cylinder r 0.4 through the centre | 2 | 1 | 3.224835294865 | 3.224835294865 | 1.4e-16 | 27198 | 1.9e-03 | 7.9e-03 | 0/0 | 11615 | closed form |
| 🟢 | Analytic block 2 × 2 × 1 − NURBS cylinder r 0.5 through | 7 | 1 | 3.214601836603 | 3.214601836603 | 8.3e-16 | 6064 | 6.1e-04 | 1.1e-02 | 0/0 | 5221 | closed form |
| 🟢 | NURBS cylinder r 0.6 ∪ analytic block (partial overlap) | 8 | 3 | 1.318783289344 | 1.318783289344 | 1.7e-15 | 6570 | 5.4e-04 | 3.5e-03 | 0/0 | 5292 | reference: analytic Boolean |
| 🟢 | NURBS torus 1.2 / 0.4 ∩ analytic block (half) | 3 | 1 | 2.200112270156 | 2.200112270156 | 2.0e-16 | 12246 | 2.9e-03 | 9.2e-03 | 0/0 | 13325 | reference: analytic Boolean |
| 🟢 | NURBS sphere r 1 − NURBS cylinder r 0.4 (both NURBS) | 2 | 2 | 3.224835294866 | 3.224835294865 | 5.5e-16 | 28438 | 1.9e-03 | 7.9e-03 | 0/0 | 17792 | closed form |
