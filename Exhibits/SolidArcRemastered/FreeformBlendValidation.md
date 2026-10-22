# Freeform blend validation

🟢 Priority 10: exact offsets of NURBS surfaces, rolling-ball blends on NURBS supports, offsets / hollows of NURBS walls and certified NURBS stand-ins (578.65 s)

## Gates

| | Gate | Measures | Limit | Worst | Checks | Failures |
|---|---|---|---|---|---|---|
| 🟢 | P1 | Parallel carrier = closed form: offsets of NURBS cylinder / sphere / torus are the analytic offsets (position, O𝑢 = (1 − dκ)·S𝑢, normal), peak curvature κ/(1 − dκ) | ≤ 10⁻¹² | 4.00e-15 | 4 | 0 |
| 🟢 | P2 | Freeform bicubic patch: O𝑢, O𝑣 vs central differences, |O𝑢 × O𝑣| = (1 − dκ₁)(1 − dκ₂)·|n|, the normal is the basis normal, inversion and signed distance of points off the offset | ≤ 10⁻⁷ (differences) · ≤ 10⁻¹⁰ | 8.67e-14 | 2 | 0 |
| 🟢 | P3 | Intersections with parallel carriers: offset NURBS sphere ∩ plane = the circle, offset cylinder ∩ offset cylinder (Steinmetz), offset freeform ∩ plane — every chart point on both exact carriers | interior ≤ 10⁻⁹ · open ends ≤ 10⁻⁷ | 6.10e-16 | 3 | 0 |
| 🟢 | P4 | Fold margin min(1 − dκᵢ): inward offsets of a unit NURBS cylinder by 0.5 (margin ½) and 1.2 (folds); offset of an offset adds distances; d = 0 returns the basis | exact | 8.33e-16 | 4 | 0 |
| 🟢 | F1 | Rolling-ball fillets on NURBS supports = closed forms: promoted block edge (3 − (1 − π/4)·r²·L), promoted cylinder rim (Pappus), freeform D-extrusion spline rim (A·(L − c·Θ), curved Pappus) — and equal to the analytic-body fillet | ≤ 10⁻⁹ relative | 2.63e-11 | 3 | 0 |
| 🟢 | F2 | Rolling-ball law on every NURBS sheet: |ball − support| = r, feet on the supports along their normals, contact on the carrier, G1 between blend and supports | ≤ 10⁻⁹ (len · rad) | 8.35e-15 | 3 | 0 |
| 🟢 | F3 | Honest refusals: a ball past a support curvature radius (fold), and the 🚧 chamfer / variable on NURBS supports — atomic, input untouched | exact | 0.00e+00 | 3 | 0 |
| 🟢 | O1 | Face offsets of a NURBS wall = closed form: the D-extrusion's spline wall ±0.1 → ΔV = H·(±d·L + π·d²/2) (Steiner: the wall leaves the base perpendicular, so the offset ends on the base plane untrimmed) | ≤ 10⁻⁹ relative | 1.41e-13 | 2 | 0 |
| 🟢 | O2 | Hollow with a NURBS wall: D-extrusion, wall 0.1, top open → V = A·H − A_in·(H − t); A_in by Green's theorem on the inward offset curve trimmed at y = t (root-found crossings, Simpson per knot span) | ≤ 10⁻⁹ relative | 2.21e-13 | 1 | 0 |
| 🟢 | O3 | Traced meetings (FaceChange N4): new edges between a parallel carrier and planes are exact traced intersections — on both carriers ≤ 10⁻⁹·scale, audit sound, the wall is a parallel carrier | exact | 8.54e-16 | 2 | 0 |
| 🟢 | A1 | NURBS stand-ins of rolling-ball / parallel carriers, measured off the certification lattice: 2000 random chart points — parametric |S − A|, geometric distance of A to the exact carrier, normal drift | ≤ tolerance (len) · drift ≤ angular tolerance 10⁻³ rad | 4.11e-01 | 12 | 0 |
| 🟢 | A2 | All-NURBS bodies (ApproximateBodyAsNurbs): no procedural carrier left, audit sound, |ΔV| ≤ tolerance × area, every edge on its stand-in faces, G1 drift across blend boundaries | ≤ tolerance · 2 × tolerance (edges) · drift ≤ 10⁻³ rad | 2.26e-01 | 8 | 0 |
| 🟢 | A3 | Honest refusals: a tolerance the span cap cannot reach, a non-positive tolerance, an empty window — atomic, no body created | exact | 0.00e+00 | 4 | 0 |
| 🟢 | A4 | Convergence: tolerance 10⁻⁴ → 10⁻¹⁰ on a parallel carrier (position only); spans per direction grow as ε^(−1/4) for cubic interpolation, every step certified | 3.5 ≤ order ≤ 5 | 3.69e+00 | 1 | 0 |

## Checks

| | Gate | Check | Detail |
|---|---|---|---|
| 🟢 | P1 | NURBS cylinder r 1 offset +0.25 → r 1.25 | position 4.44e-16 · O𝑢 / O𝑣 2.52e-16 · normal 0.00e+00 · curvature 8.88e-16 (121 points) |
| 🟢 | P1 | NURBS cylinder r 1 offset −0.4 → r 0.6 | position 4.44e-16 · O𝑢 / O𝑣 2.40e-16 · normal 0.00e+00 · curvature 4.00e-15 (121 points) |
| 🟢 | P1 | NURBS sphere R 2 offset +0.5 → R 2.5 | position 8.88e-16 · O𝑢 / O𝑣 8.85e-16 · normal 0.00e+00 · curvature 1.50e-15 (121 points) |
| 🟢 | P1 | NURBS torus R 3 r 1 offset −0.3 → tube 0.7 | position 1.44e-15 · O𝑢 / O𝑣 2.76e-16 · normal 0.00e+00 · curvature 0.00e+00 (121 points) |
| 🟢 | P2 | bicubic patch offset 1.50e-01 | O𝑢 / O𝑣 vs differences 8.48e-11 · |O𝑢 × O𝑣| 4.58e-16 · normal 2.36e-16 · foot 9.42e-16 · signed distance 1.94e-16 (81 points) |
| 🟢 | P2 | bicubic patch offset -2.00e-01 | O𝑢 / O𝑣 vs differences 8.67e-11 · |O𝑢 × O𝑣| 4.24e-16 · normal 2.96e-16 · foot 8.01e-16 · signed distance 1.39e-16 (81 points) |
| 🟢 | P3 | offset sphere (R 1 + 0.5) ∩ plane z = 0.3 → circle ρ = √2.16 | 1 branch(es), 119 chart points, worst interior residual 4.44e-16 · open ends 0.00e+00 · Hybrid · lattice + normal-cone loops + marching · lattice 21 / 8 · traced 1 skipped 7 |
| 🟢 | P3 | offset cylinder z ∩ offset cylinder x (r 1 + 0.25) → Steinmetz | 4 branch(es), 244 chart points, worst interior residual 4.44e-16 · open ends 0.00e+00 · Hybrid · lattice + normal-cone loops + marching · lattice 19 / 20 · traced 2 skipped 90 |
| 🟢 | P3 | offset freeform (+0.15) ∩ plane x = 1.1 | 1 branch(es), 15 chart points, worst interior residual 6.10e-16 · open ends 1.00e-07 · Hybrid · lattice + normal-cone loops + marching · lattice 22 / 11 · traced 1 skipped 7 |
| 🟢 | P4 | unit NURBS cylinder offset -5.00e-01 | margin 5.00e-01 (closed form 1 + d/r = 5.00e-01) → regular |
| 🟢 | P4 | unit NURBS cylinder offset -1.20e+00 | margin -2.00e-01 (closed form 1 + d/r = -2.00e-01) → folds |
| 🟢 | P4 | offset 0.2 then 0.3 | one parallel carrier on the original basis, d = 0.5 |
| 🟢 | P4 | offset back by −0.5 | the basis itself |
| 🟢 | F1 | Promoted block 2 × 1.5 × 1 · fillet r 0.2 on the top front edge (NURBS plane ∩ NURBS plane) | V 2.982832 vs closed form 2.982832 → 2.98e-16 · analytic twin 1.49e-16 · procedural carriers 1 · traced sections 2 · audit sound · 2670 ms |
| 🟢 | F2 | Promoted block 2 × 1.5 × 1 · fillet r 0.2 on the top front edge (NURBS plane ∩ NURBS plane) | ball 5.55e-17 · feet 7.02e-16 · G1 2.29e-15 rad · sheet residual 6.94e-16 (39 sections × 2 supports) |
| 🟢 | F1 | Promoted cylinder r 1 × 1.5 · fillet r 0.2 on the top rim (NURBS cylinder ∩ NURBS plane, closed) | V 4.660863 vs closed form 4.660863 → 2.67e-12 · analytic twin 2.67e-12 · procedural carriers 1 · traced sections 0 · audit sound · 71515 ms |
| 🟢 | F2 | Promoted cylinder r 1 × 1.5 · fillet r 0.2 on the top rim (NURBS cylinder ∩ NURBS plane, closed) | ball 7.22e-16 · feet 6.67e-16 · G1 8.35e-15 rad · sheet residual 1.91e-15 (39 sections × 2 supports) |
| 🟢 | F1 | Freeform D-extrusion · fillet r 0.25 on the spline rim (bicubic-ruled NURBS ∩ plane, curved Pappus) | V 2.700361 vs closed form 2.700361 → 2.63e-11 · procedural carriers 1 · traced sections 2 · audit sound · 42956 ms |
| 🟢 | F2 | Freeform D-extrusion · fillet r 0.25 on the spline rim (bicubic-ruled NURBS ∩ plane, curved Pappus) | ball 8.88e-16 · feet 9.16e-16 · G1 4.02e-15 rad · sheet residual 1.30e-15 (39 sections × 2 supports) |
| 🟢 | F3 | D-extrusion rim, r 0.9 (past the arch's curvature radius) | "Blend: E5: size exceeds the curvature radius of the NURBS support (fold margin -0.132860)" |
| 🟢 | F3 | D-extrusion rim, chamfer 0.2 | "Blend: E5: chamfer on a NURBS support (closed-form foot needed for the chord carrier, 🚧)" |
| 🟢 | F3 | D-extrusion rim, variable fillet 0.15 → 0.3 | "Blend: E5: variable size on a NURBS support (closed-form foot needed for the ball law, 🚧)" |
| 🟢 | O1 | D-extrusion · spline wall offset +0.1 | V 3.207426 vs closed form 3.207426 (arch length 4.353219) → 1.04e-13 |
| 🟢 | O3 | D-extrusion · spline wall offset +0.1 | 4 traced edge(s) · edge residual 8.32e-16 · vertex residual 7.64e-16 · 1 parallel face(s) · audit sound · 1947 ms |
| 🟢 | O1 | D-extrusion · spline wall offset −0.1 | V 2.336782 vs closed form 2.336782 (arch length 4.353219) → 1.41e-13 |
| 🟢 | O3 | D-extrusion · spline wall offset −0.1 | 4 traced edge(s) · edge residual 8.54e-16 · vertex residual 7.92e-16 · 1 parallel face(s) · audit sound · 1849 ms |
| 🟢 | O2 | D-extrusion · hollow, wall 0.1, top open | V 0.815243 vs closed form 0.815243 (A_in 2.156836 by Green; the O(t²) corner formula would give 2.156782) → 2.21e-13 · audit sound |
| 🟢 | A1 | Freeform bicubic patch offset +0.1 (parallel carrier) · tolerance 1.00e-06 | parametric 3.39e-07 · geometric 3.28e-07 · normal drift 6.98e-06 rad · 21 × 19 poles · 12 ms |
| 🟢 | A1 | Freeform bicubic patch offset +0.1 (parallel carrier) · tolerance 1.00e-08 | parametric 3.49e-09 · geometric 3.43e-09 · normal drift 1.64e-07 rad · 64 × 59 poles · 92 ms |
| 🟢 | A1 | Freeform bicubic patch offset −0.25 (parallel carrier) · tolerance 1.00e-06 | parametric 2.27e-07 · geometric 1.47e-07 · normal drift 4.64e-06 rad · 30 × 29 poles · 23 ms |
| 🟢 | A1 | Freeform bicubic patch offset −0.25 (parallel carrier) · tolerance 1.00e-08 | parametric 3.03e-09 · geometric 2.70e-09 · normal drift 1.86e-07 rad · 75 × 69 poles · 137 ms |
| 🟢 | A1 | Promoted block · fillet r 0.2 (rolling ball on NURBS planes) · tolerance 1.00e-06 · RollingBall face | parametric 2.39e-07 · geometric 2.38e-07 · normal drift 7.56e-05 rad · 25 × 22 poles |
| 🟢 | A2 | Promoted block · fillet r 0.2 (rolling ball on NURBS planes) · tolerance 1.00e-06 | 1 face(s) · 4 edge(s) approximated · 550 poles · certified 2.25e-07 / 5.39e-08 · procedural left 0 · audit sound · |ΔV| 1.37e-08 ≤ 6.28e-07 · edges off faces 1.80e-07 · G1 drift 8.06e-05 rad · 853 ms (audit 963 · volumes 4157 · edges 10) |
| 🟢 | A1 | Promoted block · fillet r 0.2 (rolling ball on NURBS planes) · tolerance 1.00e-08 · RollingBall face | parametric 1.05e-09 · geometric 1.05e-09 · normal drift 1.61e-06 rad · 25 × 65 poles |
| 🟢 | A2 | Promoted block · fillet r 0.2 (rolling ball on NURBS planes) · tolerance 1.00e-08 | 1 face(s) · 4 edge(s) approximated · 1625 poles · certified 1.02e-09 / 5.04e-09 · procedural left 0 · audit sound · |ΔV| 3.49e-11 ≤ 6.28e-09 · edges off faces 4.53e-09 · G1 drift 1.61e-06 rad · 2343 ms (audit 2280 · volumes 5189 · edges 10) |
| 🟢 | A1 | Promoted cylinder rim · fillet r 0.2 (closed rolling ball) · tolerance 1.00e-06 · RollingBall face | parametric 4.11e-07 · geometric 4.11e-07 · normal drift 7.57e-05 rad · 205 × 22 poles |
| 🟢 | A2 | Promoted cylinder rim · fillet r 0.2 (closed rolling ball) · tolerance 1.00e-06 | 1 face(s) · 2 edge(s) approximated · 4510 poles · certified 4.73e-07 / 3.62e-07 · procedural left 0 · audit sound · |ΔV| 1.79e-07 ≤ 1.83e-06 · edges off faces 2.88e-07 · G1 drift 8.13e-05 rad · 2958 ms (audit 2521 · volumes 11868 · edges 9) |
| 🟢 | A1 | Promoted cylinder rim · fillet r 0.2 (closed rolling ball) · tolerance 1.00e-08 · RollingBall face | parametric 2.70e-09 · geometric 2.69e-09 · normal drift 1.61e-06 rad · 473 × 65 poles |
| 🟢 | A2 | Promoted cylinder rim · fillet r 0.2 (closed rolling ball) · tolerance 1.00e-08 | 1 face(s) · 2 edge(s) approximated · 30745 poles · certified 3.34e-09 / 4.57e-09 · procedural left 0 · audit sound · |ΔV| 5.00e-10 ≤ 1.83e-08 · edges off faces 3.06e-09 · G1 drift 1.70e-06 rad · 18118 ms (audit 10578 · volumes 19912 · edges 7) |
| 🟢 | A1 | Freeform D-extrusion · fillet r 0.25 on the spline rim (traced spine, knots at ⅓ and ⅔) · tolerance 1.00e-06 · RollingBall face | parametric 1.83e-07 · geometric 1.72e-07 · normal drift 2.05e-05 rad · 153 × 19 poles |
| 🟢 | A2 | Freeform D-extrusion · fillet r 0.25 on the spline rim (traced spine, knots at ⅓ and ⅔) · tolerance 1.00e-06 | 1 face(s) · 4 edge(s) approximated · 2907 poles · certified 2.38e-07 / 5.02e-07 · procedural left 0 · audit sound · |ΔV| 8.04e-08 ≤ 1.60e-06 · edges off faces 2.23e-07 · G1 drift 2.27e-05 rad · 2391 ms (audit 1918 · volumes 13863 · edges 6) |
| 🟢 | A1 | Freeform D-extrusion · fillet r 0.25 on the spline rim (traced spine, knots at ⅓ and ⅔) · tolerance 1.00e-08 · RollingBall face | parametric 3.03e-09 · geometric 2.73e-09 · normal drift 1.61e-06 rad · 356 × 65 poles |
| 🟢 | A2 | Freeform D-extrusion · fillet r 0.25 on the spline rim (traced spine, knots at ⅓ and ⅔) · tolerance 1.00e-08 | 1 face(s) · 4 edge(s) approximated · 23140 poles · certified 3.55e-09 / 5.00e-09 · procedural left 0 · audit sound · |ΔV| 2.25e-10 ≤ 1.60e-08 · edges off faces 4.02e-09 · G1 drift 1.66e-06 rad · 18836 ms (audit 11755 · volumes 23544 · edges 6) |
| 🟢 | A1 | Freeform D-extrusion · spline wall offset outward 0.1 (parallel carrier, traced edges) · tolerance 1.00e-06 · Parallel face | parametric 2.24e-07 · geometric 2.03e-07 · normal drift 5.21e-06 rad · 63 × 7 poles |
| 🟢 | A2 | Freeform D-extrusion · spline wall offset outward 0.1 (parallel carrier, traced edges) · tolerance 1.00e-06 | 1 face(s) · 4 edge(s) approximated · 441 poles · certified 2.23e-07 / 4.89e-07 · procedural left 0 · audit sound · |ΔV| 5.98e-08 ≤ 4.67e-06 · edges off faces 3.22e-07 · G1 drift 0.00e+00 rad · 677 ms (audit 635 · volumes 2253 · edges 4) |
| 🟢 | A1 | Freeform D-extrusion · spline wall offset outward 0.1 (parallel carrier, traced edges) · tolerance 1.00e-08 · Parallel face | parametric 2.41e-09 · geometric 1.60e-09 · normal drift 1.36e-07 rad · 179 × 7 poles |
| 🟢 | A2 | Freeform D-extrusion · spline wall offset outward 0.1 (parallel carrier, traced edges) · tolerance 1.00e-08 | 1 face(s) · 4 edge(s) approximated · 1253 poles · certified 2.39e-09 / 4.91e-09 · procedural left 0 · audit sound · |ΔV| 4.09e-10 ≤ 4.67e-08 · edges off faces 2.91e-09 · G1 drift 0.00e+00 rad · 4171 ms (audit 3980 · volumes 5416 · edges 4) |
| 🟢 | A3 | Tolerance 10⁻¹² with at most 4 spans per direction | "Approximate: edge 8 — approximation: more than 4 spans needed for the tolerance" |
| 🟢 | A3 | Tolerance 0 | "Approximate: edge 8 — approximation: the tolerance must be positive and finite" |
| 🟢 | A3 | Tolerance NaN | "Approximate: edge 8 — approximation: the tolerance must be positive and finite" |
| 🟢 | A3 | Empty window [0, 0] × [0, 1] | "approximation: empty or non-finite parameter window" |
| 🟢 | A4 | Parallel offset +0.1 of the freeform patch | 1.00e-04: 5 × 4 spans, certified 2.50e-05 · 1.00e-06: 17 × 15 spans, certified 3.26e-07 · 1.00e-08: 60 × 55 spans, certified 3.88e-09 · 1.00e-10: 208 × 172 spans, certified 3.09e-11 · order p = 3.68 (cubic interpolation: 4) |
