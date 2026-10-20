# SolidArcRemastered — TessellationValidation

ε = 10⁻³ × body diagonal unless stated. Every quantity is recomputed by `AuditTessellation` from the PolygonComplex and the B-rep.

| | Corpus row | Bodies | Triangles | N1 unpaired · χ · unused | N2 peak / ε | N3 inverted · degenerate · folded | N4 max \|ΔV\|/(ε·A) | N5 Delaunay residual | N6 missing / edge sides | N7 min angle · < 20° | Repairs star · flip · split | ms |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 🟢 | Block | 1 | 142 | 0 · 0 · 0 | 0.000 | 0 · 0 · 0 | 0.000 | 0 | 0 / 40 | 32.4° · 0.0% | 0 · 0 · 0 | 1 |
| 🟢 | HexagonalPrism | 1 | 140 | 0 · 0 · 0 | 0.000 | 0 · 0 · 0 | 0.000 | 0 | 0 / 36 | 18.4° · 15.0% | 0 · 0 · 0 | 0 |
| 🟢 | Cylinder | 1 | 1880 | 0 · 0 · 0 | 0.997 | 0 · 0 · 0 | 0.214 | 0 | 0 / 144 | 20.9° · 0.0% | 0 · 0 · 0 | 9 |
| 🟢 | Cone | 1 | 1868 | 0 · 0 · 0 | 1.000 | 0 · 0 · 0 | 0.209 | 0 | 0 / 93 | 0.4° · 24.7% | 0 · 0 · 0 | 8 |
| 🟢 | Sphere | 1 | 5172 | 0 · 0 · 0 | 0.999 | 0 · 0 · 0 | 0.471 | 0 | 0 / 32 | 1.1° · 21.1% | 0 · 0 · 0 | 18 |
| 🟢 | Torus | 1 | 3548 | 0 · 0 · 0 | 0.999 | 0 · 0 · 0 | 0.335 | 0 | 0 / 160 | 15.3° · 1.6% | 0 · 0 · 0 | 11 |
| 🟢 | PerforatedBlock | 1 | 150 | 0 · 0 · 0 | 0.000 | 0 · 0 · 0 | 0.000 | 0 | 0 / 60 | 33.7° · 0.0% | 0 · 0 · 0 | 0 |
| 🟢 | HollowBlock | 1 | 214 | 0 · 0 · 0 | 0.000 | 0 · 0 · 0 | 0.000 | 0 | 0 / 56 | 14.0° · 3.7% | 0 · 0 · 0 | 0 |
| 🟢 | TolerantBlock | 1 | 142 | 0 · 0 · 0 | 0.000 | 0 · 0 · 0 | 0.000 | 0 | 0 / 40 | 32.4° · 0.0% | 0 · 0 · 0 | 0 |
| 🟢 | PipeBend | 1 | 2044 | 0 · 0 · 0 | 0.999 | 0 · 0 · 0 | 0.287 | 0 | 0 / 128 | 16.9° · 0.7% | 0 · 0 · 0 | 8 |
| 🟢 | LoftFrustum | 1 | 1308 | 0 · 0 · 0 | 0.999 | 0 · 0 · 0 | 0.215 | 0 | 0 / 112 | 18.5° · 0.2% | 0 · 0 · 0 | 6 |
| 🟢 | SweptRing | 1 | 2570 | 0 · 0 · 0 | 0.999 | 0 · 0 · 0 | 0.066 | 0 | 0 / 224 | 21.0° · 0.0% | 0 · 0 · 0 | 11 |
| 🟢 | MirroredCross | 1 | 3910 | 0 · 0 · 0 | 1.000 | 0 · 0 · 0 | 0.288 | 0 | 0 / 256 | 21.3° · 0.0% | 0 · 0 · 0 | 15 |
| 🟢 | ArrayedPins | 1 | 788 | 0 · 0 · 0 | 0.998 | 0 · 0 · 0 | 0.032 | 0 | 0 / 148 | 4.8° · 7.7% | 0 · 0 · 0 | 4 |
| 🟢 | BlockCorner (4 ops) | 4 | 660 | 0 · 0 · 0 | 0.000 | 0 · 0 · 0 | 0.000 | 0 | 0 / 216 | 33.9° · 0.0% | 0 · 0 · 0 | 2 |
| 🟢 | BlockStacked (4 ops) | 3 (+1 empty) | 388 | 0 · 0 · 0 | 0.000 | 0 · 0 · 0 | 0.000 | 0 | 0 / 109 | 14.0° · 1.5% | 0 · 0 · 0 | 1 |
| 🟢 | BlockRotated (4 ops) | 4 | 596 | 0 · 0 · 0 | 0.000 | 0 · 0 · 0 | 0.000 | 0 | 0 / 240 | 15.7° · 4.5% | 0 · 0 · 0 | 3 |
| 🟢 | DrilledBlock (4 ops) | 4 | 5272 | 0 · 0 · 0 | 0.998 | 0 · 0 · 0 | 0.211 | 0 | 0 / 592 | 15.6° · 0.1% | 0 · 0 · 0 | 24 |
| 🟢 | SphereCrater (4 ops) | 4 | 6294 | 0 · 0 · 0 | 0.999 | 0 · 0 · 0 | 0.365 | 0 | 0 / 336 | 1.4° · 15.6% | 2 · 0 · 0 | 25 |
| 🟢 | SphereCraterTilted (4 ops) | 4 | 6366 | 0 · 0 · 0 | 1.000 | 0 · 0 · 0 | 0.361 | 0 | 0 / 332 | 1.4° · 16.7% | 1 · 0 · 0 | 23 |
| 🟢 | SphereLens (4 ops) | 4 | 16210 | 0 · 0 · 0 | 1.000 | 0 · 0 · 0 | 0.478 | 0 | 0 / 336 | 1.2° · 22.7% | 1 · 0 · 0 | 56 |
| 🟢 | SteppedShaft (4 ops) | 4 | 8924 | 0 · 0 · 0 | 1.000 | 0 · 0 · 0 | 0.227 | 0 | 0 / 728 | 20.8° · 0.0% | 0 · 0 · 0 | 41 |
| 🟢 | ConeInCylinder (4 ops) | 4 | 9142 | 0 · 0 · 0 | 0.999 | 0 · 0 · 0 | 0.316 | 0 | 0 / 664 | 0.4° · 6.7% | 0 · 0 · 0 | 35 |
| 🟢 | TorusSlab (4 ops) | 4 | 10750 | 0 · 0 · 0 | 1.000 | 0 · 0 · 0 | 0.240 | 0 | 0 / 888 | 4.7° · 1.4% | 0 · 0 · 0 | 43 |
| 🟢 | TorusEquator (4 ops) | 4 | 10812 | 0 · 0 · 0 | 0.999 | 0 · 0 · 0 | 0.215 | 0 | 0 / 904 | 4.5° · 1.4% | 0 · 0 · 0 | 44 |
| 🟢 | DShaft (4 ops) | 4 | 4388 | 0 · 0 · 0 | 0.997 | 0 · 0 · 0 | 0.192 | 0 | 0 / 468 | 3.3° · 2.4% | 0 · 0 · 0 | 15 |
| 🟢 | BlockCavity (4 ops) | 3 (+1 empty) | 516 | 0 · 0 · 0 | 0.000 | 0 · 0 · 0 | 0.000 | 0 | 0 / 168 | 33.9° · 0.0% | 0 · 0 · 0 | 1 |
| 🟢 | BlockApart (4 ops) | 3 (+1 empty) | 420 | 0 · 0 · 0 | 0.000 | 0 · 0 · 0 | 0.000 | 0 | 0 / 136 | 23.9° · 0.0% | 0 · 0 · 0 | 1 |
| 🟢 | NapkinRing (4 ops) | 4 | 13656 | 0 · 0 · 0 | 1.000 | 0 · 0 · 0 | 0.404 | 0 | 0 / 576 | 2.9° · 3.4% | 0 · 0 · 0 | 42 |
| 🟢 | CrossUnequal (4 ops) | 4 | 11480 | 0 · 0 · 0 | 1.000 | 0 · 0 · 0 | 0.326 | 0 | 0 / 760 | 20.9° · 0.0% | 0 · 0 · 0 | 39 |
| 🟢 | CrossOffset (4 ops) | 4 | 11436 | 0 · 0 · 0 | 1.000 | 0 · 0 · 0 | 0.312 | 0 | 0 / 752 | 20.8° · 0.0% | 0 · 0 · 0 | 40 |
| 🟢 | SteinmetzSolid (4 ops) | 4 | 10752 | 0 · 0 · 0 | 0.999 | 0 · 0 · 0 | 0.323 | 0 | 0 / 752 | 17.4° · 0.0% | 0 · 0 · 4 | 40 |
| 🟢 | SteinmetzOblique (4 ops) | 4 | 10046 | 0 · 0 · 0 | 1.000 | 0 · 0 · 0 | 0.329 | 0 | 0 / 780 | 20.7° · 0.0% | 0 · 0 · 0 | 42 |
| 🟢 | BlockFlushHalf (4 ops) | 4 | 654 | 0 · 0 · 0 | 0.000 | 0 · 0 · 0 | 0.000 | 0 | 0 / 128 | 18.4° · 2.4% | 0 · 0 · 0 | 1 |
| 🟢 | BlockStep (4 ops) | 4 | 546 | 0 · 0 · 0 | 0.000 | 0 · 0 · 0 | 0.000 | 0 | 0 / 168 | 18.4° · 3.7% | 0 · 0 · 0 | 1 |
| 🟢 | BlockFlushSlot (4 ops) | 4 | 476 | 0 · 0 · 0 | 0.000 | 0 · 0 · 0 | 0.000 | 0 | 0 / 160 | 18.4° · 5.9% | 0 · 0 · 0 | 2 |
| 🟢 | BlockFlushCorner (4 ops) | 4 | 612 | 0 · 0 · 0 | 0.000 | 0 · 0 · 0 | 0.000 | 0 | 0 / 184 | 23.9° · 0.0% | 0 · 0 · 0 | 2 |
| 🟢 | RandomSpheres (20 pairs × 4 ops) | 80 | 316492 | 0 · 0 · 0 | 1.000 | 0 · 0 · 0 | 0.479 | 0 | 0 / 7312 | 0.6° · 21.4% | 41 · 2 · 0 | 1123 |
| 🟢 | RandomBlockSphere (20 pairs × 4 ops) | 80 | 144234 | 0 · 0 · 0 | 1.000 | 0 · 0 · 0 | 0.452 | 0 | 0 / 7181 | 0.0° · 20.0% | 10 · 0 · 1 | 548 |
| 🟢 | RandomDrills (20 pairs × 4 ops) | 79 (+1 empty) | 100174 | 0 · 0 · 0 | 1.000 | 0 · 0 · 0 | 0.296 | 0 | 0 / 9831 | 0.1° · 1.6% | 0 · 0 · 0 | 377 |


Corpus: 342 bodies, 725172 triangles.

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
| 🟢 | SphereCraterTilted Difference | 1344 | 06c530e2d7197c5c | 06c530e2d7197c5c |
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

**PASS** — 0 failing row(s); 20.9 s

Allowances: N1 0 unpaired sides, χ(complex) = V − E + F − (L − F) of the B-rep, 0 unused vertices · N2 peak deviation ≤ ε (7 barycentric samples per triangle, closed-form projection) · N3 0 inverted, 0 zero-area, 0 folded · N4 |ΔV| ≤ ε·A · N5 0 · N6 every edge-sample side used once per fin, opposite directions · N7 reported only.
