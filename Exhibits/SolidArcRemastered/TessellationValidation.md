# SolidArcRemastered — TessellationValidation

ε = 10⁻³ × body diagonal unless stated. Every quantity is recomputed by `AuditTessellation` from the PolygonComplex and the B-rep.

| | Corpus row | Bodies | Triangles | N1 unpaired · χ · unused | N2 peak / ε | N3 inverted · degenerate · folded | N4 max \|ΔV\|/(ε·A) | N5 Delaunay residual | N6 missing / edge sides | N7 min angle · < 20° | Repairs star · flip · split | ms |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 🟢 | Block | 1 | 142 | 0 · 0 · 0 | 0.000 | 0 · 0 · 0 | 0.000 | 0 | 0 / 40 | 32.4° · 0.0% | 0 · 0 · 0 | 1 |
| 🟢 | HexagonalPrism | 1 | 140 | 0 · 0 · 0 | 0.000 | 0 · 0 · 0 | 0.000 | 0 | 0 / 36 | 18.4° · 15.0% | 0 · 0 · 0 | 1 |
| 🟢 | Cylinder | 1 | 1880 | 0 · 0 · 0 | 0.997 | 0 · 0 · 0 | 0.214 | 0 | 0 / 144 | 20.9° · 0.0% | 0 · 0 · 0 | 12 |
| 🟢 | Cone | 1 | 1868 | 0 · 0 · 0 | 1.000 | 0 · 0 · 0 | 0.209 | 0 | 0 / 93 | 0.4° · 24.7% | 0 · 0 · 0 | 11 |
| 🟢 | Sphere | 1 | 5172 | 0 · 0 · 0 | 0.999 | 0 · 0 · 0 | 0.471 | 0 | 0 / 32 | 1.1° · 21.1% | 0 · 0 · 0 | 27 |
| 🟢 | Torus | 1 | 3548 | 0 · 0 · 0 | 0.999 | 0 · 0 · 0 | 0.335 | 0 | 0 / 160 | 15.3° · 1.6% | 0 · 0 · 0 | 17 |
| 🟢 | PerforatedBlock | 1 | 150 | 0 · 0 · 0 | 0.000 | 0 · 0 · 0 | 0.000 | 0 | 0 / 60 | 33.7° · 0.0% | 0 · 0 · 0 | 1 |
| 🟢 | HollowBlock | 1 | 214 | 0 · 0 · 0 | 0.000 | 0 · 0 · 0 | 0.000 | 0 | 0 / 56 | 14.0° · 3.7% | 0 · 0 · 0 | 1 |
| 🟢 | TolerantBlock | 1 | 142 | 0 · 0 · 0 | 0.000 | 0 · 0 · 0 | 0.000 | 0 | 0 / 40 | 32.4° · 0.0% | 0 · 0 · 0 | 0 |
| 🟢 | BlockCorner (4 ops) | 4 | 660 | 0 · 0 · 0 | 0.000 | 0 · 0 · 0 | 0.000 | 0 | 0 / 216 | 33.9° · 0.0% | 0 · 0 · 0 | 3 |
| 🟢 | BlockStacked (4 ops) | 3 (+1 empty) | 386 | 0 · 0 · 0 | 0.000 | 0 · 0 · 0 | 0.000 | 0 | 0 / 117 | 14.0° · 1.6% | 0 · 0 · 0 | 1 |
| 🟢 | BlockRotated (4 ops) | 4 | 596 | 0 · 0 · 0 | 0.000 | 0 · 0 · 0 | 0.000 | 0 | 0 / 240 | 15.7° · 4.5% | 0 · 0 · 0 | 4 |
| 🟢 | DrilledBlock (4 ops) | 4 | 5272 | 0 · 0 · 0 | 0.998 | 0 · 0 · 0 | 0.211 | 0 | 0 / 592 | 15.6° · 0.1% | 0 · 0 · 0 | 32 |
| 🟢 | SphereCrater (4 ops) | 4 | 6294 | 0 · 0 · 0 | 0.999 | 0 · 0 · 0 | 0.365 | 0 | 0 / 336 | 1.4° · 15.6% | 2 · 0 · 0 | 33 |
| 🟢 | SphereCraterTilted (4 ops) | 4 | 6366 | 0 · 0 · 0 | 1.000 | 0 · 0 · 0 | 0.361 | 0 | 0 / 332 | 1.4° · 16.7% | 1 · 0 · 0 | 31 |
| 🟢 | SphereLens (4 ops) | 4 | 16220 | 0 · 0 · 0 | 1.000 | 0 · 0 · 0 | 0.475 | 0 | 0 / 368 | 1.1° · 22.7% | 1 · 0 · 0 | 80 |
| 🟢 | SteppedShaft (4 ops) | 4 | 8924 | 0 · 0 · 0 | 1.000 | 0 · 0 · 0 | 0.227 | 0 | 0 / 728 | 20.8° · 0.0% | 0 · 0 · 0 | 56 |
| 🟢 | ConeInCylinder (4 ops) | 4 | 9142 | 0 · 0 · 0 | 0.999 | 0 · 0 · 0 | 0.316 | 0 | 0 / 664 | 0.4° · 6.7% | 0 · 0 · 0 | 50 |
| 🟢 | TorusSlab (4 ops) | 4 | 10952 | 0 · 0 · 0 | 1.000 | 0 · 0 · 0 | 0.224 | 0 | 0 / 1148 | 4.7° · 1.4% | 0 · 0 · 0 | 67 |
| 🟢 | TorusEquator (4 ops) | 4 | 10812 | 0 · 0 · 0 | 0.999 | 0 · 0 · 0 | 0.215 | 0 | 0 / 904 | 4.5° · 1.4% | 0 · 0 · 0 | 60 |
| 🟢 | DShaft (4 ops) | 4 | 4378 | 0 · 0 · 0 | 0.997 | 0 · 0 · 0 | 0.192 | 0 | 0 / 516 | 3.3° · 2.4% | 0 · 0 · 0 | 20 |
| 🟢 | BlockCavity (4 ops) | 3 (+1 empty) | 516 | 0 · 0 · 0 | 0.000 | 0 · 0 · 0 | 0.000 | 0 | 0 / 168 | 33.9° · 0.0% | 0 · 0 · 0 | 2 |
| 🟢 | BlockApart (4 ops) | 3 (+1 empty) | 420 | 0 · 0 · 0 | 0.000 | 0 · 0 · 0 | 0.000 | 0 | 0 / 136 | 23.9° · 0.0% | 0 · 0 · 0 | 1 |
| 🟢 | NapkinRing (4 ops) | 4 | 13656 | 0 · 0 · 0 | 1.000 | 0 · 0 · 0 | 0.404 | 0 | 0 / 576 | 2.9° · 3.4% | 0 · 0 · 0 | 55 |
| 🟢 | RandomSpheres (20 pairs × 4 ops) | 80 | 316362 | 0 · 0 · 0 | 1.000 | 0 · 0 · 0 | 0.479 | 0 | 0 / 7497 | 0.6° · 21.5% | 39 · 3 · 0 | 1549 |
| 🟢 | RandomBlockSphere (20 pairs × 4 ops) | 80 | 144252 | 0 · 0 · 0 | 1.000 | 0 · 0 · 0 | 0.452 | 0 | 0 / 7329 | 0.0° · 20.0% | 10 · 0 · 1 | 772 |
| 🟢 | RandomDrills (20 pairs × 4 ops) | 79 (+1 empty) | 100454 | 0 · 0 · 0 | 1.000 | 0 · 0 · 0 | 0.296 | 0 | 0 / 10267 | 0.1° · 1.8% | 0 · 0 · 0 | 522 |


Corpus: 305 bodies, 668918 triangles.

## N8 — Convergence (ε = 10⁻², 3·10⁻³, 10⁻³, 3·10⁻⁴, 10⁻⁴ × diagonal)

| | Specimen | Triangles | \|ΔV\| | Peak / ε | Slope log\|ΔV\| / log ε | Slope log N / log ε |
|---|---|---|---|---|---|---|
| 🟢 | Sphere | 538 → 1736 → 5172 → 16850 → 50198 | 1.0e+00 → 3.1e-01 → 1.1e-01 → 3.1e-02 → 1.0e-02 | 0.995 · 0.998 · 0.999 · 1.000 · 1.000 | 0.997 | -0.985 |
| 🟢 | Torus | 344 → 1164 → 3548 → 11562 → 34704 | 4.5e+00 → 1.4e+00 → 4.5e-01 → 1.4e-01 → 4.7e-02 | 0.973 · 0.998 · 0.999 · 1.000 · 1.000 | 0.993 | -1.001 |
| 🟢 | Cone | 256 → 688 → 1868 → 5530 → 14224 | 4.5e-01 → 1.3e-01 → 4.7e-02 → 1.4e-02 → 4.9e-03 | 0.995 · 0.997 · 1.000 · 1.000 · 1.000 | 0.983 | -0.879 |
| 🟢 | Cylinder | 192 → 660 → 1880 → 6052 → 15994 | 4.7e-01 → 1.4e-01 → 4.7e-02 → 1.4e-02 → 5.0e-03 | 0.935 · 0.996 · 0.997 · 1.000 · 1.000 | 0.985 | -0.961 |

Gate: volume slope ∈ (0.85, 1.15), triangle slope ∈ (−1.15, −0.85), peak ≤ ε at every level.

## N9 — Determinism (fresh partition per run)

| | Body | Triangles | Digest run 1 | Digest run 2 |
|---|---|---|---|---|
| 🟢 | Sphere | 5172 | 6e2f2e10980e225c | 6e2f2e10980e225c |
| 🟢 | Torus | 3548 | 68b0e276748c17ff | 68b0e276748c17ff |
| 🟢 | DrilledBlock Difference | 1128 | 47f6f56302dc2c2a | 47f6f56302dc2c2a |
| 🟢 | SphereCraterTilted Difference | 1344 | fba13665aa8f787f | fba13665aa8f787f |
| 🟢 | NapkinRing Difference | 5470 | cb398b5f3f5b33c7 | cb398b5f3f5b33c7 |

## N10 — Scale invariance (tilted torus R = 2s, r = 0.5s; triangle count within 1 %)

| | Scale s | Triangles | Peak / ε | \|ΔV\| / V |
|---|---|---|---|---|
| 🟢 | 1e-03 | 3844 | 1.000 | 9.78e-03 |
| 🟢 | 1e+00 | 3848 | 1.000 | 9.78e-03 |
| 🟢 | 1e+03 | 3848 | 1.000 | 9.77e-03 |

## N11 — CDT regressions (every segment recovered, n − 2 interior triangles)

| | Case | Polygons | Vertices | Failures |
|---|---|---|---|---|
| 🟢 | Hull-fan side lookup (RandomBlockSphere #8 face 13) | 1 | 32 | 0 |
| 🟢 | Dipped rectangles with collinear runs (δ = 10⁻¹…10⁻⁶) | 200 | 6402 | 0 |

**PASS** — 0 failing row(s); 131.6 s

Allowances: N1 0 unpaired sides, χ(complex) = V − E + F − (L − F) of the B-rep, 0 unused vertices · N2 peak deviation ≤ ε (7 barycentric samples per triangle, closed-form projection) · N3 0 inverted, 0 zero-area, 0 folded · N4 |ΔV| ≤ ε·A · N5 0 · N6 every edge-sample side used once per fin, opposite directions · N7 reported only.
