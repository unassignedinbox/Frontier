# SolidArcRemastered — ContainmentValidation

| | Gate | Family | Cases | Failures | Measured | Allowed | ms |
|---|---|---|---|---|---|---|---|
| 🟢 | H1 | Block random | 1500 | 0 | residual 5.2e-14 · 111841 evals/pt | 0 wrong · residual ≤ 1e-6 | 4199 |
| 🟢 | H1 | RotatedBlock random | 1500 | 0 | residual 5.7e-15 · 38721 evals/pt | 0 wrong · residual ≤ 1e-6 | 1503 |
| 🟢 | H1 | Cylinder random | 1500 | 0 | residual 1.9e-13 · 110348 evals/pt | 0 wrong · residual ≤ 1e-6 | 4641 |
| 🟢 | H1 | Cone random | 1500 | 0 | residual 6.8e-15 · 63147 evals/pt | 0 wrong · residual ≤ 1e-6 | 2666 |
| 🟢 | H1 | Sphere random | 1500 | 0 | residual 1.4e-14 · 80066 evals/pt | 0 wrong · residual ≤ 1e-6 | 3955 |
| 🟢 | H1 | Torus random | 1500 | 0 | residual 3.2e-13 · 112485 evals/pt | 0 wrong · residual ≤ 1e-6 | 6168 |
| 🟢 | H1 | PerforatedBlock random | 1500 | 0 | residual 7.7e-14 · 178858 evals/pt | 0 wrong · residual ≤ 1e-6 | 6723 |
| 🟢 | H1 | HollowBlock random | 1500 | 0 | residual 4.3e-14 · 133531 evals/pt | 0 wrong · residual ≤ 1e-6 | 5005 |
| 🟢 | H2 | ± normal offsets 1e-3 / 1e-5 / 1e-7, all specimens | 1152 | 0 | residual 1.0e-13 · 384 winding / 768 nearest-normal | 0 wrong · residual ≤ 1e-6 | 14900 |
| 🟢 | H2w | ± offsets 1e-3 / 1e-5, pure winding number (shortcut off) | 192 | 0 | residual 4.7e-12 | 0 wrong · residual ≤ 1e-6 | 0 |
| 🟢 | H3 | on faces, edges, ±½ precision | 1636 | 0 | — | 0 wrong | 1 |
| 🟢 | H4 | GWN vs ray-cast parity (CSI + 2D face test) | 2000 | 0 | 0 undecided rays | 0 disagreements | 5730 |
| 🟢 | H5 | open box (top removed) vs 1 − Ω_top/4π (Van Oosterom–Strackee) | 298 | 0 | |Δw| 5.8e-16 | |Δw| ≤ 1e-8 | 305 |
| 🟢 | H6 | 2D winding: square face with a hole (top + bottom) | 8000 | 0 | — | 0 wrong | 2 |

**PASS** — 0 failing row(s)
