# SolidArcRemastered — Intersection scenes

Each scene is solved by `IntersectSurfaces` and checked by `AuditIntersection` (I1–I6) against independent expectations:
closed-form or quadrature lengths, hand-derived counts, and a dense 48 × 48 reference lattice on **both** surfaces (offset a quarter cell from the solver's 8 × 8 lattice).
Timings are the best of 3 runs on the sandbox (Linux, g++ -O2 Release).

## Hybrid policy (default)

| | Scene | Method | Branches (closed) | Special | Length | Exact | Rel. error | Residual | Ref. crossings / misses | Solve ms |
|---|---|---|---|---|---|---|---|---|---|---|
| 🟢 | PlaneSphereSection | Analytic | 1 (1) | 0 | 10.8807 | 10.8828 | 1.9e-04 | 4.4e-16 | 101 / 0 | 0.1 |
| 🟢 | SphereSphereLens | Analytic | 1 (1) | 0 | 7.53773 | 7.53982 | 2.8e-04 | 4.4e-16 | 146 / 0 | 0.1 |
| 🟢 | TorusPlaneSection | Analytic | 2 (2) | 0 | 37.695 | 37.6991 | 1.1e-04 | 6.7e-16 | 214 / 0 | 0.1 |
| 🟢 | VillarceauCircles | Hybrid | 4 (0) | 2 | 37.6967 | 37.6991 | 6.4e-05 | 4.4e-16 | 315 / 0 | 9.1 |
| 🟢 | ObliqueCylinderSection | Hybrid | 1 (1) | 0 | 6.96273 | 6.96395 | 1.8e-04 | 2.2e-16 | 87 / 0 | 0.4 |
| 🟢 | Bicylinder | Hybrid | 2 (2) | 0 | 7.74244 | 7.74509 | 3.4e-04 | 2.2e-16 | 190 / 0 | 3.3 |
| 🟢 | SteinmetzCrossing | Hybrid | 4 (0) | 2 | 15.2784 | 15.2808 | 1.6e-04 | 2.2e-16 | 323 / 0 | 7.9 |
| 🟢 | VivianiCurve | Hybrid | 2 (0) | 1 | 15.2787 | 15.2808 | 1.4e-04 | 4.4e-16 | 390 / 0 | 23.8 |
| 🟢 | ConeTorusRing | Hybrid | 2 (2) | 0 | 37.9748 | — | — | 6.4e-16 | 236 / 0 | 67.6 |
| 🟢 | SmallLoop | Hybrid | 1 (1) | 0 | 0.279273 | — | — | 1.4e-14 | 10 / 0 | 13.5 |
| 🟢 | TangentSpheres | Analytic | 0 (0) | 1 | 0 | 0 | 0.0e+00 | 0.0e+00 | 0 / 0 | 0.0 |

## MarchingOnly policy

| | Scene | Method | Branches (closed) | Special | Length | Exact | Rel. error | Residual | Ref. crossings / misses | Solve ms |
|---|---|---|---|---|---|---|---|---|---|---|
| 🟢 | PlaneSphereSection | Marching | 1 (1) | 0 | 10.8816 | 10.8828 | 1.1e-04 | 4.4e-16 | 101 / 0 | 1.4 |
| 🟢 | SphereSphereLens | Marching | 1 (1) | 0 | 7.53859 | 7.53982 | 1.6e-04 | 4.4e-16 | 146 / 0 | 5.0 |
| 🟢 | TorusPlaneSection | Marching | 2 (2) | 0 | 37.6967 | 37.6991 | 6.4e-05 | 6.7e-16 | 214 / 0 | 16.0 |
| 🟢 | VillarceauCircles | Marching | 4 (0) | 2 | 37.6967 | 37.6991 | 6.4e-05 | 4.4e-16 | 315 / 0 | 8.3 |
| 🟢 | ObliqueCylinderSection | Marching | 1 (1) | 0 | 6.96273 | 6.96395 | 1.8e-04 | 2.2e-16 | 87 / 0 | 0.4 |
| 🟢 | Bicylinder | Marching | 2 (2) | 0 | 7.74244 | 7.74509 | 3.4e-04 | 2.2e-16 | 190 / 0 | 3.2 |
| 🟢 | SteinmetzCrossing | Marching | 4 (0) | 2 | 15.2784 | 15.2808 | 1.6e-04 | 2.2e-16 | 323 / 0 | 7.3 |
| 🟢 | VivianiCurve | Marching | 2 (0) | 1 | 15.2787 | 15.2808 | 1.4e-04 | 4.4e-16 | 390 / 0 | 23.5 |
| 🟢 | ConeTorusRing | Marching | 2 (2) | 0 | 37.9748 | — | — | 6.4e-16 | 236 / 0 | 71.0 |
| 🟢 | SmallLoop | Marching | 1 (1) | 0 | 0.279273 | — | — | 1.4e-14 | 10 / 0 | 14.5 |
| 🟢 | TangentSpheres | Marching | 0 (0) | 1 | 0 | 0 | 0.0e+00 | 0.0e+00 | 0 / 0 | 5.6 |

## Negative control

`SmallLoop LoopDetection=0` → 🔴 unsound, as required: 0 branches, 10 of 10 reference crossings missed — the audit catches a loop the solver was told not to look for.

## Gallery

Renders of the viewer JSON (orthographic, exact branches green, marched branches orange, special points red rings): `IntersectionGallery/*.png`.
