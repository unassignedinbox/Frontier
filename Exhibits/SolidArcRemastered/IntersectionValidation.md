# SolidArcRemastered — IntersectionValidation

| | Gate | Family | Cases | Failures | Measured | Allowed | ms |
|---|---|---|---|---|---|---|---|
| 🟢 | E1 | Simple roots, degree 1–8, gap ≥ 1e-3 | 20000 | 0 | worst err 5.9e-11 (0.05 × bound) · missed 0 · spurious 0 · 16.4 clips + 2.53 halvings / poly | err ≤ condition bound | 83 |
| 🟢 | E1 | Close pair (gap 1e-5 … 1e-4) + simple, degree ≤ 8 | 20000 | 0 | worst err 2.0e-09 (0.05 × bound) · missed 0 · spurious 0 · 27.2 clips + 2.91 halvings / poly | err ≤ condition bound | 106 |
| 🟢 | E1 | Double root + simple, degree ≤ 8, 𝑓 ∪ 𝑓′ roots | 20000 | 0 | worst err 8.0e-08 (0.17 × bound) · missed 0 · spurious 0 · 32.6 clips + 2.08 halvings / poly | err ≤ condition bound | 145 |
| 🟢 | E2 | Exact table: line/circle × plane, sphere, cylinder, cone, torus (incl. tangent, coincident) | 12 | 0 | worst transversal τ error 8.9e-16 | τ ≤ 1e-12 (tangent 1e-4) | 0 |
| 🟢 | E2 | Random line/circle × 5 surface kinds vs 20000-sample sign scan | 2000 | 0 | 3183 sign changes, 0 missed · 3183 crossings, worst |d| 1.6e-15 | 0 missed · |d| ≤ 1e-8 | 1484 |
| 🟢 | F1 | Analytic sphere × plane (random frames) | 500 | 0 | worst centre/radius/axis error 1.6e-15 · 0 not analytic | ≤ 1e-11 | 36 |
| 🟢 | F1 | Analytic sphere × sphere (random frames) | 500 | 0 | worst centre/radius/axis error 5.4e-15 · 0 not analytic | ≤ 1e-11 | 48 |
| 🟢 | F1 | Analytic torus × plane ⟂ axis (random frames) | 500 | 0 | worst centre/radius/axis error 1.8e-15 · 0 not analytic | ≤ 1e-11 | 76 |
| 🟢 | F1 | Analytic cylinder × plane ⟂ axis (random frames) | 500 | 0 | worst centre/radius/axis error 2.0e-15 · 0 not analytic | ≤ 1e-11 | 31 |
| 🟢 | F1 | Analytic cone × plane ⟂ axis (random frames) | 500 | 0 | worst centre/radius/axis error 5.2e-16 · 0 not analytic | ≤ 1e-11 | 30 |
| 🟢 | F1 | Analytic cylinder × coaxial sphere (random frames) | 500 | 0 | worst centre/radius/axis error 9.0e-16 · 0 not analytic | ≤ 1e-11 | 86 |
| 🟢 | F1 | Analytic plane × plane (random) | 500 | 0 | worst direction/offset error 1.1e-15 | ≤ 1e-12 | 10 |
| 🟢 | F2 | MarchingOnly sphere × plane vs exact circles | 100 | 0 | on-curve 2.9e-15 · coverage gap 1.0e-03 · length rel 6.8e-04 | on ≤ 1e-9 · gap ≤ 1.5e-3 | 151 |
| 🟢 | F2 | MarchingOnly sphere × sphere vs exact circles | 100 | 0 | on-curve 8.0e-15 · coverage gap 1.0e-03 · length rel 7.9e-04 | on ≤ 1e-9 · gap ≤ 1.5e-3 | 2222 |
| 🟢 | F2 | MarchingOnly torus × plane ⟂ axis vs exact circles | 100 | 0 | on-curve 3.2e-15 · coverage gap 1.0e-03 · length rel 1.3e-04 | on ≤ 1e-9 · gap ≤ 1.5e-3 | 1135 |
| 🟢 | F2 | MarchingOnly cylinder × plane ⟂ axis vs exact circles | 100 | 0 | on-curve 4.7e-15 · coverage gap 1.0e-03 · length rel 6.3e-04 | on ≤ 1e-9 · gap ≤ 1.5e-3 | 62 |
| 🟢 | F2 | MarchingOnly cone × plane ⟂ axis vs exact circles | 100 | 0 | on-curve 1.8e-15 · coverage gap 1.0e-03 · length rel 2.9e-04 | on ≤ 1e-9 · gap ≤ 1.5e-3 | 66 |
| 🟢 | F2 | MarchingOnly cylinder × coaxial sphere vs exact circles | 100 | 0 | on-curve 3.7e-15 · coverage gap 1.0e-03 · length rel 6.3e-04 | on ≤ 1e-9 · gap ≤ 1.5e-3 | 2955 |
| 🟢 | F3 | Scene gallery through the audit (I1–I6), Hybrid | 11 | 0 | worst length rel 3.4e-04 | all sound | 143 |
| 🟢 | F3 | Scene gallery through the audit (I1–I6), MarchingOnly | 11 | 0 | worst length rel 3.4e-04 | all sound | 170 |
| 🟢 | F3 | Negative control: SmallLoop with LoopDetection = 0 must be flagged | 1 | 0 | 10 of 10 reference crossings missed | audit unsound | 4 |
| 🟢 | F3 | Random oblique cylinder×cylinder, sphere×cone, torus×cylinder through the audit | 300 | 0 | 454 branches · 62264 reference crossings · residual 2.9e-15 · worst gap 1.0e-03 | I1–I4 sound | 8531 |

**PASS** — 0 failing row(s)
