# Exchange validation

🟢 Priority 4: STL export and read-back (5.98 s)

## Gates

| | Gate | Measures | Limit | Worst | Checks | Failures |
|---|---|---|---|---|---|---|
| 🟢 | EX1 | STL closure: every specimen (primitives, hole, cavity, NURBS sweep / loft / pipe, rolling-ball and variable fillets, chamfers) written binary and read back welds watertight — no open, non-manifold or misoriented side, no degenerate facet; facets = complex triangles; components = shells; Euler χ of the facets = the B-rep's V − E + 2F − L | 0 defects · χ exact | 0.00e+00 | 17 | 0 |
| 🟢 | EX2 | STL geometry: |V(facets) − V(exact)| ≤ sag · A(exact) for every specimen (each facet lies within sag of its face); the sphere's error shrinks at least 2× when sag shrinks 4×; ModelUnitMillimetres 1000 scales every corner by 1000 within float rounding | ≤ sag · A · ratio ≤ ½ | 4.70e-01 | 19 | 0 |
| 🟢 | EX3 | STL formats: binary size 84 + 50 n, header never "solid…"; ASCII (9 significant digits) reads back to the same welded corners and facets as binary; stored normals = unit facet normals; writing twice gives the same bytes | identical | 4.87e-08 | 17 | 0 |
| 🟢 | EX4 | STL refusals and defect detection: no bodies, a zero / negative unit, a stale body token, a truncated binary, ASCII without endsolid, a facet with two vertices; one facet removed → 3 open sides, one flipped → 3 misoriented, one repeated → 3 non-manifold | named · counted | 0.00e+00 | 8 | 0 |

## Checks

| | Gate | Check | Detail |
|---|---|---|---|
| 🟢 | EX1 | Block | 142 facets, 73 corners, χ 2 = B-rep 2, 1 component(s) / 1 shell(s), defects 0 (open 0 · non-manifold 0 · misoriented 0 · degenerate 0) |
| 🟢 | EX2 | Block | V facets 24 vs exact 24: \|ΔV\| 1.776e-14 ≤ sag 5.39e-03 · A 52 = 2.800e-01 (ratio 0.000) |
| 🟢 | EX3 | Block | binary 7184 B = 84 + 50 · 142, ASCII 19176 B, same welded facets: yes, normals within 1.1e-16, repeatable: yes |
| 🟢 | EX1 | Cylinder | 1880 facets, 942 corners, χ 2 = B-rep 2, 1 component(s) / 1 shell(s), defects 0 (open 0 · non-manifold 0 · misoriented 0 · degenerate 0) |
| 🟢 | EX2 | Cylinder | V facets 21.1586855 vs exact 21.2057504: \|ΔV\| 4.706e-02 ≤ sag 5.20e-03 · A 42.4115 = 2.204e-01 (ratio 0.214) |
| 🟢 | EX3 | Cylinder | binary 94084 B = 84 + 50 · 1880, ASCII 419430 B, same welded facets: yes, normals within 4.1e-08, repeatable: yes |
| 🟢 | EX1 | Cone | 1868 facets, 936 corners, χ 2 = B-rep 2, 1 component(s) / 1 shell(s), defects 0 (open 0 · non-manifold 0 · misoriented 0 · degenerate 0) |
| 🟢 | EX2 | Cone | V facets 12.5192322 vs exact 12.5663706: \|ΔV\| 4.714e-02 ≤ sag 6.40e-03 · A 35.2207 = 2.255e-01 (ratio 0.209) |
| 🟢 | EX3 | Cone | binary 93484 B = 84 + 50 · 1868, ASCII 425552 B, same welded facets: yes, normals within 4.8e-08, repeatable: yes |
| 🟢 | EX1 | Sphere | 5172 facets, 2588 corners, χ 2 = B-rep 2, 1 component(s) / 1 shell(s), defects 0 (open 0 · non-manifold 0 · misoriented 0 · degenerate 0) |
| 🟢 | EX2 | Sphere | V facets 33.4045818 vs exact 33.5103216: \|ΔV\| 1.057e-01 ≤ sag 4.47e-03 · A 50.2655 = 2.248e-01 (ratio 0.470) |
| 🟢 | EX3 | Sphere | binary 258684 B = 84 + 50 · 5172, ASCII 1221791 B, same welded facets: yes, normals within 4.5e-08, repeatable: yes |
| 🟢 | EX1 | Torus | 3548 facets, 1774 corners, χ 0 = B-rep 0, 1 component(s) / 1 shell(s), defects 0 (open 0 · non-manifold 0 · misoriented 0 · degenerate 0) |
| 🟢 | EX2 | Torus | V facets 58.762645 vs exact 59.2176264: \|ΔV\| 4.550e-01 ≤ sag 1.15e-02 · A 118.435 = 1.361e+00 (ratio 0.334) |
| 🟢 | EX3 | Torus | binary 177484 B = 84 + 50 · 3548, ASCII 830326 B, same welded facets: yes, normals within 4.4e-08, repeatable: yes |
| 🟢 | EX1 | PerforatedBlock | 150 facets, 75 corners, χ 0 = B-rep 0, 1 component(s) / 1 shell(s), defects 0 (open 0 · non-manifold 0 · misoriented 0 · degenerate 0) |
| 🟢 | EX2 | PerforatedBlock | V facets 42 vs exact 42: \|ΔV\| 7.105e-15 ≤ sag 7.48e-03 · A 96 = 7.184e-01 (ratio 0.000) |
| 🟢 | EX3 | PerforatedBlock | binary 7584 B = 84 + 50 · 150, ASCII 19086 B, same welded facets: yes, normals within 1.1e-16, repeatable: yes |
| 🟢 | EX1 | HollowBlock | 214 facets, 111 corners, χ 4 = B-rep 4, 2 component(s) / 2 shell(s), defects 0 (open 0 · non-manifold 0 · misoriented 0 · degenerate 0) |
| 🟢 | EX2 | HollowBlock | V facets 114 vs exact 114: \|ΔV\| 2.132e-13 ≤ sag 8.77e-03 · A 170 = 1.492e+00 (ratio 0.000) |
| 🟢 | EX3 | HollowBlock | binary 10784 B = 84 + 50 · 214, ASCII 30648 B, same welded facets: yes, normals within 1.1e-16, repeatable: yes |
| 🟢 | EX1 | PipeBend | 2044 facets, 1024 corners, χ 2 = B-rep 2, 1 component(s) / 1 shell(s), defects 0 (open 0 · non-manifold 0 · misoriented 0 · degenerate 0) |
| 🟢 | EX2 | PipeBend | V facets 2.45538129 vs exact 2.4674011: \|ΔV\| 1.202e-02 ≤ sag 3.67e-03 · A 11.4404 = 4.203e-02 (ratio 0.286) |
| 🟢 | EX3 | PipeBend | binary 102284 B = 84 + 50 · 2044, ASCII 471962 B, same welded facets: yes, normals within 4.9e-08, repeatable: yes |
| 🟢 | EX1 | LoftFrustum | 1308 facets, 656 corners, χ 2 = B-rep 2, 1 component(s) / 1 shell(s), defects 0 (open 0 · non-manifold 0 · misoriented 0 · degenerate 0) |
| 🟢 | EX2 | LoftFrustum | V facets 8.22258985 vs exact 8.24668072: \|ΔV\| 2.409e-02 ≤ sag 4.69e-03 · A 23.9342 = 1.123e-01 (ratio 0.215) |
| 🟢 | EX3 | LoftFrustum | binary 65484 B = 84 + 50 · 1308, ASCII 291342 B, same welded facets: yes, normals within 4.2e-08, repeatable: yes |
| 🟢 | EX1 | SweptRing | 2570 facets, 1285 corners, χ 0 = B-rep 0, 1 component(s) / 1 shell(s), defects 0 (open 0 · non-manifold 0 · misoriented 0 · degenerate 0) |
| 🟢 | EX2 | SweptRing | V facets 10.103828 vs exact 10.1159283: \|ΔV\| 1.210e-02 ≤ sag 4.69e-03 · A 39.0186 = 1.830e-01 (ratio 0.066) |
| 🟢 | EX3 | SweptRing | binary 128584 B = 84 + 50 · 2570, ASCII 579230 B, same welded facets: yes, normals within 4.1e-08, repeatable: yes |
| 🟢 | EX1 | MirroredCross | 3984 facets, 1994 corners, χ 2 = B-rep 2, 1 component(s) / 1 shell(s), defects 0 (open 0 · non-manifold 0 · misoriented 0 · degenerate 0) |
| 🟢 | EX2 | MirroredCross | V facets 3.97385897 vs exact 4.00319568: \|ΔV\| 2.934e-02 ≤ sag 5.30e-03 · A 19.9456 = 1.058e-01 (ratio 0.277) |
| 🟢 | EX3 | MirroredCross | binary 199284 B = 84 + 50 · 3984, ASCII 939374 B, same welded facets: yes, normals within 4.6e-08, repeatable: yes |
| 🟢 | EX1 | BoxAllFillet | 5616 facets, 2810 corners, χ 2 = B-rep 2, 1 component(s) / 1 shell(s), defects 0 (open 0 · non-manifold 0 · misoriented 0 · degenerate 0) |
| 🟢 | EX2 | BoxAllFillet | V facets 23.3137248 vs exact 23.3408494: \|ΔV\| 2.712e-02 ≤ sag 5.39e-03 · A 47.2627 = 2.545e-01 (ratio 0.107) |
| 🟢 | EX3 | BoxAllFillet | binary 280884 B = 84 + 50 · 5616, ASCII 1187854 B, same welded facets: yes, normals within 4.1e-08, repeatable: yes |
| 🟢 | EX1 | BoxAllChamfer | 196 facets, 100 corners, χ 2 = B-rep 2, 1 component(s) / 1 shell(s), defects 0 (open 0 · non-manifold 0 · misoriented 0 · degenerate 0) |
| 🟢 | EX2 | BoxAllChamfer | V facets 22.5420001 vs exact 22.542: \|ΔV\| 1.498e-07 ≤ sag 5.39e-03 · A 45.5425 = 2.453e-01 (ratio 0.000) |
| 🟢 | EX3 | BoxAllChamfer | binary 9884 B = 84 + 50 · 196, ASCII 34247 B, same welded facets: yes, normals within 2.3e-08, repeatable: yes |
| 🟢 | EX1 | HoleRimFillet | 1884 facets, 942 corners, χ 0 = B-rep 0, 1 component(s) / 1 shell(s), defects 0 (open 0 · non-manifold 0 · misoriented 0 · degenerate 0) |
| 🟢 | EX2 | HoleRimFillet | V facets 21.7132012 vs exact 21.7032826: \|ΔV\| 9.919e-03 ≤ sag 5.39e-03 · A 56.9721 = 3.068e-01 (ratio 0.032) |
| 🟢 | EX3 | HoleRimFillet | binary 94284 B = 84 + 50 · 1884, ASCII 398651 B, same welded facets: yes, normals within 4.1e-08, repeatable: yes |
| 🟢 | EX1 | BossRootFillet | 1222 facets, 613 corners, χ 2 = B-rep 2, 1 component(s) / 1 shell(s), defects 0 (open 0 · non-manifold 0 · misoriented 0 · degenerate 0) |
| 🟢 | EX2 | BossRootFillet | V facets 25.1632692 vs exact 25.165744: \|ΔV\| 2.475e-03 ≤ sag 5.83e-03 · A 55.4641 = 3.234e-01 (ratio 0.008) |
| 🟢 | EX3 | BossRootFillet | binary 61184 B = 84 + 50 · 1222, ASCII 248979 B, same welded facets: yes, normals within 4.7e-08, repeatable: yes |
| 🟢 | EX1 | VariableFillet | 988 facets, 496 corners, χ 2 = B-rep 2, 1 component(s) / 1 shell(s), defects 0 (open 0 · non-manifold 0 · misoriented 0 · degenerate 0) |
| 🟢 | EX2 | VariableFillet | V facets 23.9372061 vs exact 23.9400227: \|ΔV\| 2.817e-03 ≤ sag 5.39e-03 · A 51.5352 = 2.775e-01 (ratio 0.010) |
| 🟢 | EX3 | VariableFillet | binary 49484 B = 84 + 50 · 988, ASCII 193095 B, same welded facets: yes, normals within 4.0e-08, repeatable: yes |
| 🟢 | EX1 | SmoothFillet | 1602 facets, 803 corners, χ 2 = B-rep 2, 1 component(s) / 1 shell(s), defects 0 (open 0 · non-manifold 0 · misoriented 0 · degenerate 0) |
| 🟢 | EX2 | SmoothFillet | V facets 7.26934803 vs exact 7.27935258: \|ΔV\| 1.000e-02 ≤ sag 4.57e-03 · A 25.5742 = 1.170e-01 (ratio 0.086) |
| 🟢 | EX3 | SmoothFillet | binary 80184 B = 84 + 50 · 1602, ASCII 341418 B, same welded facets: yes, normals within 4.0e-08, repeatable: yes |
| 🟢 | EX2 | Sphere convergence (sag ratio 4e-3 → 1e-3 → 2.5e-4) | 1340 facets \|ΔV\| 4.354e-01 → 5172 facets \|ΔV\| 1.057e-01 → 19944 facets \|ΔV\| 2.628e-02 · worst step ratio 0.248 |
| 🟢 | EX2 | Block 4 × 2.5 × 1 m → mm | V 1e+10 mm³ = 10¹⁰, corners × 1000 within 3.5e-08 relative |
| 🟢 | EX4 | no bodies | STL: no body to export |
| 🟢 | EX4 | zero unit | STL: the model unit must be a positive length |
| 🟢 | EX4 | negative unit | STL: the model unit must be a positive length |
| 🟢 | EX4 | stale token | STL: a body is not in the partition |
| 🟢 | EX4 | truncated binary | STL: neither a binary layout (84 + 50 · count bytes) nor ASCII "solid" |
| 🟢 | EX4 | ASCII without endsolid | STL: missing "endsolid" |
| 🟢 | EX4 | facet with two vertices | STL: a facet needs three vertices |
| 🟢 | EX4 | defects counted | one facet removed → 3 open; one flipped → 3 misoriented; one repeated → 3 non-manifold; none watertight |
