# Freeform blend validation

🟢 Priority 10: exact offsets of NURBS surfaces and rolling-ball blends on NURBS supports (129.36 s)

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
| 🟢 | F1 | Promoted block 2 × 1.5 × 1 · fillet r 0.2 on the top front edge (NURBS plane ∩ NURBS plane) | V 2.982832 vs closed form 2.982832 → 2.98e-16 · analytic twin 1.49e-16 · procedural carriers 1 · traced sections 2 · audit sound · 2369 ms |
| 🟢 | F2 | Promoted block 2 × 1.5 × 1 · fillet r 0.2 on the top front edge (NURBS plane ∩ NURBS plane) | ball 5.55e-17 · feet 7.02e-16 · G1 2.29e-15 rad · sheet residual 6.94e-16 (39 sections × 2 supports) |
| 🟢 | F1 | Promoted cylinder r 1 × 1.5 · fillet r 0.2 on the top rim (NURBS cylinder ∩ NURBS plane, closed) | V 4.660863 vs closed form 4.660863 → 2.67e-12 · analytic twin 2.67e-12 · procedural carriers 1 · traced sections 0 · audit sound · 57403 ms |
| 🟢 | F2 | Promoted cylinder r 1 × 1.5 · fillet r 0.2 on the top rim (NURBS cylinder ∩ NURBS plane, closed) | ball 7.22e-16 · feet 6.67e-16 · G1 8.35e-15 rad · sheet residual 1.91e-15 (39 sections × 2 supports) |
| 🟢 | F1 | Freeform D-extrusion · fillet r 0.25 on the spline rim (bicubic-ruled NURBS ∩ plane, curved Pappus) | V 2.700361 vs closed form 2.700361 → 2.63e-11 · procedural carriers 1 · traced sections 2 · audit sound · 37676 ms |
| 🟢 | F2 | Freeform D-extrusion · fillet r 0.25 on the spline rim (bicubic-ruled NURBS ∩ plane, curved Pappus) | ball 8.88e-16 · feet 9.16e-16 · G1 4.02e-15 rad · sheet residual 1.30e-15 (39 sections × 2 supports) |
| 🟢 | F3 | D-extrusion rim, r 0.9 (past the arch's curvature radius) | "Blend: E5: size exceeds the curvature radius of the NURBS support (fold margin -0.132860)" |
| 🟢 | F3 | D-extrusion rim, chamfer 0.2 | "Blend: E5: chamfer on a NURBS support (closed-form foot needed for the chord carrier, 🚧)" |
| 🟢 | F3 | D-extrusion rim, variable fillet 0.15 → 0.3 | "Blend: E5: variable size on a NURBS support (closed-form foot needed for the ball law, 🚧)" |
