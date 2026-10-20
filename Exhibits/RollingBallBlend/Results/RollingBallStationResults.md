# Rolling-ball station validation results

## Fillet (constant-radius rolling ball)

| Specimen | Edge | Stations | Converged | Max residual [m] | Ball dev [m] | Normal dev [rad] | Independent dev [m] | Closed-form dev [m] | Mean Newton its |
|---|---|---|---|---|---|---|---|---|---|
| Plane-plane 90 deg (box edge) | convex | 161 | 161 PASS | 7.4e-14 | 0.0e+00 | 0.0e+00 | 0.0e+00 | 7.4e-14 | 2.0 |
| Plane-plane 60 deg | convex | 161 | 161 PASS | 7.4e-14 | 0.0e+00 | 0.0e+00 | n/a | 7.4e-14 | 2.0 |
| Plane-plane 135 deg | convex | 161 | 161 PASS | 7.4e-14 | 0.0e+00 | 0.0e+00 | n/a | 7.4e-14 | 2.0 |
| Plane-cylinder cap rim (convex) | convex | 161 | 161 PASS | 4.9e-16 | 1.7e-16 | 2.1e-08 | 1.4e-16 | 1.1e-12 | 3.0 |
| Plane-cylinder boss root (concave) | concave | 161 | 161 PASS | 3.7e-16 | 9.7e-17 | 2.1e-08 | 1.1e-16 | 9.1e-13 | 3.0 |
| Plane-sphere truncation rim | convex | 161 | 161 PASS | 5.0e-16 | 1.4e-16 | 2.6e-08 | 1.4e-16 | 9.7e-13 | 3.0 |
| Plane-cone frustum foot rim | convex | 161 | 161 PASS | 5.0e-16 | 1.2e-16 | 2.1e-08 | 1.4e-17 | 1.2e-12 | 3.0 |
| Cylinder-cylinder T junction | concave | 161 | 161 PASS | 2.2e-15 | 1.1e-15 | 2.1e-08 | 1.2e-15 | n/a | 3.0 |
| Freeform bicubic - plane | convex | 161 | 161 PASS | 1.0e-15 | 5.0e-16 | 2.1e-08 | 1.5e-15 | n/a | 3.0 |

## Chamfer (equal chord setback)

| Specimen | Edge | Stations | Converged | Max residual [m] | Setback dev [m] | Normal dev [rad] | Independent dev [m] | Closed-form dev [m] | Mean Newton its |
|---|---|---|---|---|---|---|---|---|---|
| Plane-plane 90 deg (box edge) | convex | 161 | 161 PASS | 0.0e+00 | 0.0e+00 | n/a | 0.0e+00 | 0.0e+00 | 4.0 |
| Plane-plane 60 deg | convex | 161 | 161 PASS | 0.0e+00 | 0.0e+00 | n/a | n/a | 0.0e+00 | 4.0 |
| Plane-plane 135 deg | convex | 161 | 161 PASS | 3.5e-18 | 0.0e+00 | n/a | n/a | 0.0e+00 | 4.0 |
| Plane-cylinder cap rim (convex) | convex | 161 | 161 PASS | 5.8e-16 | 1.9e-15 | n/a | 1.1e-16 | 1.1e-12 | 4.0 |
| Plane-cylinder boss root (concave) | concave | 161 | 161 PASS | 4.9e-17 | 8.3e-17 | n/a | 5.6e-17 | 9.1e-13 | 4.0 |
| Plane-sphere truncation rim | convex | 161 | 161 PASS | 5.1e-14 | 2.5e-13 | n/a | 1.1e-16 | 3.0e-14 | 4.0 |
| Plane-cone frustum foot rim | convex | 161 | 161 PASS | 4.4e-16 | 6.9e-17 | n/a | 0.0e+00 | 7.6e-13 | 5.0 |
| Cylinder-cylinder T junction | concave | 161 | 161 PASS | 9.7e-16 | 5.9e-15 | n/a | 1.1e-16 | n/a | 4.0 |
| Freeform bicubic - plane | convex | 161 | 161 PASS | 6.9e-15 | 4.3e-14 | n/a | 5.0e-16 | n/a | 4.0 |

