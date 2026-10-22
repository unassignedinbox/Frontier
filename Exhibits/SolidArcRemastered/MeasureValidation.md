# Measure validation

🟢 Measure / section M1–M2: exact mass properties, face areas, edge lengths, minimum distances and plane sections (8.43 s)

## Gates

| | Gate | Measures | Limit | Worst | Checks | Failures |
|---|---|---|---|---|---|---|
| 🟢 | M1 | Body mass properties (volume, surface area, centroid, inertia tensor, principal moments) of block, translated / rotated block, cylinder (upright, tilted), cone, frustum, sphere, torus, hollow and perforated blocks, NURBS-promoted cylinder / torus vs closed forms | rel. ≤ 10⁻⁹ | 1.13e-15 | 13 | 0 |
| 🟢 | M2 | Face areas and centroids (cylinder side and caps, cone side, sphere, torus, holed plate face, NURBS cylinder side) and edge lengths (circles, lines) vs closed forms | rel. ≤ 10⁻⁹ | 1.32e-15 | 13 | 0 |
| 🟢 | M3 | Minimum distance: opposite faces, edge–edge (clamped end and interior skew), sphere–sphere, cylinder ∥ plane, vertex–sphere, vertex–torus, vertex–circle edge, vertex–cone, vertex–NURBS cylinder, adjacent faces = 0 | ≤ 10⁻⁹ | 1.78e-15 | 11 | 0 |
| 🟢 | M4 | Plane sections: rectangle, plate with a round hole, sphere disc, oblique cylinder ellipse (perimeter by AGM), torus meridian discs and annulus, cone disc, NURBS cylinder — area, perimeter, centroid, Iuu / Ivv / Iuv, loops | rel. ≤ 10⁻⁹ | 7.20e-14 | 8 | 0 |
| 🟢 | M5 | Refusals name the cause: section plane above the body, plane on a body face | named | 0.00e+00 | 2 | 0 |

## Checks

| | Gate | Check | Detail |
|---|---|---|---|
| 🟢 | M1 | Block 4 × 2 × 1 at (1, 2, 3) | V 8 · A 28 · C (3, 3, 3.5) · I₁ I₂ I₃ 3.33333333333 11.3333333333 13.3333333333 · worst 2.54e-16 · 65.251355 ms |
| 🟢 | M1 | Same block moved by (1000, −500, 300) | V 8 · A 28 · C (1003, -497, 303.5) · I₁ I₂ I₃ 3.33333333333 11.3333333333 13.3333333333 · worst 2.54e-16 · 65.126816 ms |
| 🟢 | M1 | Same block turned 30° about (1, 1, 1): R · I · Rᵀ, products of inertia ≠ 0 | V 8 · A 28 · C (3.16666666667, 2.87799153207, 3.45534180126) · I₁ I₂ I₃ 3.33333333333 11.3333333333 13.3333333333 · worst 2.22e-16 · 72.298423 ms |
| 🟢 | M1 | Cylinder r 0.75 h 2 | V 3.53429173529 · A 12.9590696961 · C (0.5, -0.25, 1.1) · I₁ I₂ I₃ 0.99401955055 1.67510702037 1.67510702037 · worst 8.80e-16 · 46.128257 ms |
| 🟢 | M1 | Cylinder on the axis (0, 1, 1) / √2 | V 3.53429173529 · A 12.9590696961 · C (6.66133814775e-16, 0.707106781187, 0.707106781187) · I₁ I₂ I₃ 0.99401955055 1.67510702037 1.67510702037 · worst 1.13e-15 · 45.074826 ms |
| 🟢 | M1 | NURBS-promoted cylinder (rational faces) | V 3.53429173529 · A 12.9590696961 · C (0.5, -0.25, 1.1) · I₁ I₂ I₃ 0.99401955055 1.67510702037 1.67510702037 · worst 3.77e-16 · 578.334857 ms |
| 🟢 | M1 | Cone r 1 h 3 | V 3.14159265359 · A 13.0761809194 · C (-3, 1.8103703898e-17, 0.75) · I₁ I₂ I₃ 0.942477796077 1.53152641863 1.53152641863 · worst 5.65e-16 · 62.641526 ms |
| 🟢 | M1 | Frustum r 1 → 0.5 h 2 | V 3.66519142919 · A 13.6418295745 · C (2.22044604925e-16, 4, 0.785714285714) · I₁ I₂ I₃ 1.21736715327 1.69702103163 1.69702103163 · worst 6.06e-16 · 48.203214 ms |
| 🟢 | M1 | Sphere R 1.3 | V 9.20277207992 · A 21.2371663383 · C (0.2, 0.3, -0.4) · I₁ I₂ I₃ 6.22107392602 6.22107392602 6.22107392602 · worst 5.79e-16 · 67.701675 ms |
| 🟢 | M1 | Torus R 2 r 0.5 | V 9.86960440109 · A 39.4784176044 · C (0, -6, -6.6010455837e-17) · I₁ I₂ I₃ 21.2813344898 21.2813344898 41.3289684296 · worst 7.20e-16 · 129.549238 ms |
| 🟢 | M1 | NURBS-promoted torus (rational faces) | V 9.86960440109 · A 39.4784176044 · C (0, -6, -1.12489110962e-16) · I₁ I₂ I₃ 21.2813344898 21.2813344898 41.3289684296 · worst 1.08e-15 · 866.711508 ms |
| 🟢 | M1 | Hollow block (cavity shell inside the solid region) | V 22 · A 62 · C (2, 1.5, 1) · I₁ I₂ I₃ 25.6666666667 39.1666666667 49.1666666667 · worst 8.07e-16 · 142.919088 ms |
| 🟢 | M1 | Perforated block (genus 1) | V 20 · A 60 · C (2, 1.5, 1) · I₁ I₂ I₃ 24.3333333333 37.3333333333 48.3333333333 · worst 3.55e-16 · 169.75354 ms |
| 🟢 | M2 | Cylinder side 2πrh | A 9.42477796077 vs 9.42477796077 · centroid (-5.55111512313e-16, 2.76240031645e-17, 1) · 3.77e-16 |
| 🟢 | M2 | Cylinder top cap πr² | A 1.76714586764 vs 1.76714586764 · centroid (-6.66133814775e-16, -1.87495661778e-16, 2) · 3.77e-16 |
| 🟢 | M2 | Cylinder rim 2πr | L 4.71238898038 vs 4.71238898038 · 1.32e-15 |
| 🟢 | M2 | Cone side πr√(r² + h²), centroid h/3 | A 9.9345882658 vs 9.9345882658 · centroid (5, 1.62006508341e-17, 1) · 5.36e-16 |
| 🟢 | M2 | Cone base circle 2π | L 6.28318530718 vs 6.28318530718 · 7.07e-16 |
| 🟢 | M2 | Sphere 4πR² (poles, seam) | A 21.2371663383 vs 21.2371663383 · centroid (-5, 1.98091882874e-17, 2.22044604925e-16) · 5.02e-16 |
| 🟢 | M2 | Torus 4π²Rr | A 39.4784176044 vs 39.4784176044 · centroid (-4.4408920985e-16, 8, -1.73303536576e-16) · 7.20e-16 |
| 🟢 | M2 | Torus outer seam circle 2π(R + r) | L 15.7079632679 vs 15.7079632679 · 3.39e-16 |
| 🟢 | M2 | Torus meridian seam circle 2πr | L 3.14159265359 vs 3.14159265359 · 7.07e-16 |
| 🟢 | M2 | Holed top face 12 − 2 (inner loop) | A 10 vs 10 · centroid (2, 1.5, 2) · 1.78e-16 |
| 🟢 | M2 | Block edge 4 | L 4 vs 4 · 4.44e-16 |
| 🟢 | M2 | NURBS cylinder side 2πrh | A 9.42477796077 vs 9.42477796077 · centroid (2.22044604925e-16, 2.06104264013e-17, 1) · 1.88e-16 |
| 🟢 | M2 | NURBS rim 2πr (rational quadratic) | L 4.71238898038 vs 4.71238898038 · 7.54e-16 |
| 🟢 | M3 | Opposite faces of a unit block | d 1 vs 1 · error 0.00e+00 · (0, 0.41999999984, 0.94000000088) → (1, 0.41999999984, 0.94000000088) · 8 seeds, 0 rounds · 54.777719 ms |
| 🟢 | M3 | Adjacent faces (shared edge) | d 0 vs 0 · error 0.00e+00 · (0, 0, 0.229166666667) → (0, 0, 0.229166666667) · 8 seeds, 0 rounds · 60.04981 ms |
| 🟢 | M3 | Edge–edge, clamped at an end: √(0.5² + 1) | d 1.11803398875 vs 1.11803398875 · error 0.00e+00 · (0.5, 1, 1) → (0.5, 1.5, 2) · 8 seeds, 0 rounds · 0.135811 ms |
| 🟢 | M3 | Edge–edge, interior skew lines | d 1 vs 1 · error 0.00e+00 · (0.2, 1, 1) → (0.2, 1, 2) · 8 seeds, 1 rounds · 0.127172 ms |
| 🟢 | M3 | Sphere–sphere: dist(C₁, C₂) − R₁ − R₂ | d 1.70156211872 vs 1.70156211872 · error 2.22e-16 · (4.9370425713, 0.31234752391, 0.156173761813) → (6.53147871433, 0.843826238142, 0.421913119038) · 8 seeds, 7 rounds · 46.198117 ms |
| 🟢 | M3 | Vertex–sphere: √11 − 1 | d 2.31662479036 vs 2.31662479036 · error 0.00e+00 · (1, 1, 1) → (3.09546596627, 0.301511344578, 0.301511344578) · 8 seeds, 0 rounds · 0.216989 ms |
| 🟢 | M3 | Cylinder side ∥ plane (a line of minima) | d 0.5 vs 0.5 · error 0.00e+00 · (0, 5, 1.5) → (0, 5, 1) · 8 seeds, 0 rounds · 63.36475 ms |
| 🟢 | M3 | Vertex on the torus axis: √(R² + 9) − r | d 3.10555127546 vs 3.10555127546 · error 0.00e+00 · (0, -6, 3) → (1.70906629288, -6.2159052825, 0.416025147169) · 8 seeds, 0 rounds · 0.203064 ms |
| 🟢 | M3 | Vertex on the axis – rim circle: √(1 + 2²) | d 2.2360679775 vs 2.2360679775 · error 0.00e+00 · (-5, 0, 3) → (-4, 0, 1) · 8 seeds, 0 rounds · 0.00831 ms |
| 🟢 | M3 | Vertex – cone side: 3 / √5 | d 1.3416407865 vs 1.3416407865 · error 0.00e+00 · (8, -6, 1) → (6.8, -6, 0.4) · 8 seeds, 0 rounds · 0.171498 ms |
| 🟢 | M3 | Vertex – NURBS cylinder side: 2.5 − 1 | d 1.5 vs 1.5 · error 1.78e-15 · (12.5, 0, 0.5) → (11, 1.59928304583e-15, 0.5) · 8 seeds, 0 rounds · 5.476235 ms |
| 🟢 | M4 | Block 4 × 2 × 1 at x = 1: rectangle 2 × 1 | A 2 · P 6 · C (1, 1, 0.5) · Iuu Ivv Iuv 0.166666666667 0.666666666667 -3.5527136788e-15 · 1 region(s), 1 loop(s), 4 outline edges · worst 7.20e-14 · 421.134698 ms |
| 🟢 | M4 | Plate − round hole r 0.4 at z = 0.5: 8 − πr², hole loop | A 7.49734517543 · P 14.5132741229 · C (1.93295562458, 1, 0.5) · Iuu Ivv Iuv 2.64656047368 10.1102054703 2.15383266777e-14 · 1 region(s), 2 loop(s), 5 outline edges · worst 2.10e-14 · 610.953647 ms |
| 🟢 | M4 | Sphere R 1.3 at d = 0.5: disc π(R² − d²) | A 4.52389342117 · P 7.53982236862 · C (0.2, 0.3, 0.1) · Iuu Ivv Iuv 1.62860163162 1.62860163162 -3.21964677141e-15 · 1 region(s), 1 loop(s), 1 outline edges · worst 1.00e-14 · 253.802664 ms |
| 🟢 | M4 | Cylinder cut at 35°: ellipse πr²/cos θ, perimeter 4a·E(sin θ) | A 2.15728676985 · P 5.24551325713 · C (-1.59497757214e-15, 4.48969125796e-15, 2) · Iuu Ivv Iuv 0.452107151326 0.303368452011 -3.64291929955e-16 · 1 region(s), 1 loop(s), 1 outline edges · worst 6.79e-15 · 404.79616 ms |
| 🟢 | M4 | NURBS cylinder at z = 1: disc πr² | A 1.76714586764 · P 4.71238898038 · C (3.31798605659e-16, 1.0020710552e-14, 1) · Iuu Ivv Iuv 0.248504887637 0.248504887637 -3.20576898361e-15 · 1 region(s), 1 loop(s), 1 outline edges · worst 5.01e-15 · 1919.509614 ms |
| 🟢 | M4 | Torus meridian plane x = 0: two discs 2πr² | A 1.57079632679 · P 6.28318530718 · C (0, 3.67530763191e-15, 2.39071693557e-14) · Iuu Ivv Iuv 0.0981747704245 6.3813600776 -2.34257058196e-14 · 2 region(s), 2 loop(s), 2 outline edges · worst 6.08e-15 · 412.081415 ms |
| 🟢 | M4 | Torus at z = 0.3: annulus π(R₊² − R₋²) | A 10.0530964915 · P 25.1327412287 · C (-1.67310428556e-15, -2.63941864191e-15, 0.3) · Iuu Ivv Iuv 20.9104407023 20.9104407023 3.64153152077e-14 · 1 region(s), 2 loop(s), 2 outline edges · worst 2.47e-15 · 398.64232 ms |
| 🟢 | M4 | Cone r 1 h 2 at z = 1: disc r/2 | A 0.785398163397 · P 3.14159265359 · C (-5.03587824565e-16, 6.03421902067e-15, 1) · Iuu Ivv Iuv 0.0490873852123 0.0490873852123 8.84708972748e-16 · 1 region(s), 1 loop(s), 1 outline edges · worst 4.42e-15 · 222.335014 ms |
| 🟢 | M5 | Plane z = 5 above the block | Section: the plane misses the body |
| 🟢 | M5 | Plane z = 1 on the top face | Section: the plane misses the body |
