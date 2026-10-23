# Exchange validation

🟢 Priority 4: STL export and read-back; STEP AP214 export (50.86 s)

## Gates

| | Gate | Measures | Limit | Worst | Checks | Failures |
|---|---|---|---|---|---|---|
| 🟢 | EX1 | STL closure: every specimen (primitives, hole, cavity, NURBS sweep / loft / pipe, rolling-ball and variable fillets, chamfers) written binary and read back welds watertight — no open, non-manifold or misoriented side, no degenerate facet; facets = complex triangles; components = shells; Euler χ of the facets = the B-rep's V − E + 2F − L | 0 defects · χ exact | 0.00e+00 | 17 | 0 |
| 🟢 | EX2 | STL geometry: |V(facets) − V(exact)| ≤ sag · A(exact) for every specimen (each facet lies within sag of its face); the sphere's error shrinks at least 2× when sag shrinks 4×; ModelUnitMillimetres 1000 scales every corner by 1000 within float rounding | ≤ sag · A · ratio ≤ ½ | 4.70e-01 | 19 | 0 |
| 🟢 | EX3 | STL formats: binary size 84 + 50 n, header never "solid…"; ASCII (9 significant digits) reads back to the same welded corners and facets as binary; stored normals = unit facet normals; writing twice gives the same bytes | identical | 4.87e-08 | 17 | 0 |
| 🟢 | EX4 | STL refusals and defect detection: no bodies, a zero / negative unit, a stale body token, a truncated binary, ASCII without endsolid, a facet with two vertices; one facet removed → 3 open sides, one flipped → 3 misoriented, one repeated → 3 non-manifold | named · counted | 0.00e+00 | 8 | 0 |
| 🟢 | EX5 | STEP structure, read by an independent Part 21 parser: AP214 schema, every #reference defined once, ADVANCED_FACE / EDGE_CURVE / VERTEX_POINT / CLOSED_SHELL / solid counts = the B-rep's; every EDGE_CURVE used exactly twice per closed shell in opposite effective directions; every EDGE_LOOP connected head to tail; writing twice gives the same bytes | exact | 0.00e+00 | 26 | 0 |
| 🟢 | EX6 | STEP geometry: every face's carrier entity matches its kind (PLANE, CYLINDRICAL / CONICAL / SPHERICAL / TOROIDAL_SURFACE; NURBS → B-spline, rational where weighted; rolling-ball → certified B-spline stand-in); every VERTEX_POINT reads back bit-identical to the model × unit; millimetre unit declared; metre model × 1000 exactly; stand-in deviation within tolerance | bit-exact · ≤ 10⁻⁶ | 3.29e-07 | 27 | 0 |
| 🟢 | EX7 | STEP refusals name the cause: no bodies, zero / negative unit, non-positive tolerance, a stale body token, a sheet body | named | 0.00e+00 | 6 | 0 |
| 🟢 | EX8 | STEP export time per body (stand-in fitting included) | ≤ 2 s | 1.04e-01 | 26 | 0 |

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
| 🟢 | EX5 | Block | 170 instances, 0 dangling, 0 repeated; faces 6/6, edges 12/12, vertices 8/8, shells 1/1, solids 1/1; edge uses not (1 forward + 1 reverse): 0; loop breaks 0; repeatable: yes |
| 🟢 | EX6 | Block | PLANE 6 (0 rational); wrong carrier kinds 0; vertices bit-identical: yes; stand-ins 0 surfaces / 0 curves, deviation 0.00e+00; exact V 24, A 52 |
| 🟢 | EX8 | Block | 0.6 ms, 170 instances, 6411 bytes |
| 🟢 | EX5 | Cylinder | 73 instances, 0 dangling, 0 repeated; faces 3/3, edges 3/3, vertices 2/2, shells 1/1, solids 1/1; edge uses not (1 forward + 1 reverse): 0; loop breaks 0; repeatable: yes |
| 🟢 | EX6 | Cylinder | CYLINDRICAL_SURFACE 1, PLANE 2 (0 rational); wrong carrier kinds 0; vertices bit-identical: yes; stand-ins 0 surfaces / 0 curves, deviation 0.00e+00; exact V 21.2057504117, A 42.4115008235 |
| 🟢 | EX8 | Cylinder | 1.9 ms, 73 instances, 3260 bytes |
| 🟢 | EX5 | Cone | 57 instances, 0 dangling, 0 repeated; faces 2/2, edges 2/2, vertices 2/2, shells 1/1, solids 1/1; edge uses not (1 forward + 1 reverse): 0; loop breaks 0; repeatable: yes |
| 🟢 | EX6 | Cone | CONICAL_SURFACE 1, PLANE 1 (0 rational); wrong carrier kinds 0; vertices bit-identical: yes; stand-ins 0 surfaces / 0 curves, deviation 0.00e+00; exact V 12.5663706144, A 35.2207174126 |
| 🟢 | EX8 | Cone | 1.7 ms, 57 instances, 2769 bytes |
| 🟢 | EX5 | Sphere | 42 instances, 0 dangling, 0 repeated; faces 1/1, edges 1/1, vertices 2/2, shells 1/1, solids 1/1; edge uses not (1 forward + 1 reverse): 0; loop breaks 0; repeatable: yes |
| 🟢 | EX6 | Sphere | SPHERICAL_SURFACE 1 (0 rational); wrong carrier kinds 0; vertices bit-identical: yes; stand-ins 0 surfaces / 0 curves, deviation 0.00e+00; exact V 33.5103216383, A 50.2654824574 |
| 🟢 | EX8 | Sphere | 2.3 ms, 42 instances, 2257 bytes |
| 🟢 | EX5 | Torus | 48 instances, 0 dangling, 0 repeated; faces 1/1, edges 2/2, vertices 1/1, shells 1/1, solids 1/1; edge uses not (1 forward + 1 reverse): 0; loop breaks 0; repeatable: yes |
| 🟢 | EX6 | Torus | TOROIDAL_SURFACE 1 (0 rational); wrong carrier kinds 0; vertices bit-identical: yes; stand-ins 0 surfaces / 0 curves, deviation 0.00e+00; exact V 59.2176264065, A 118.435252813 |
| 🟢 | EX8 | Torus | 1.3 ms, 48 instances, 2465 bytes |
| 🟢 | EX5 | PerforatedBlock | 306 instances, 0 dangling, 0 repeated; faces 10/10, edges 24/24, vertices 16/16, shells 1/1, solids 1/1; edge uses not (1 forward + 1 reverse): 0; loop breaks 0; repeatable: yes |
| 🟢 | EX6 | PerforatedBlock | PLANE 10 (0 rational); wrong carrier kinds 0; vertices bit-identical: yes; stand-ins 0 surfaces / 0 curves, deviation 0.00e+00; exact V 42, A 96 |
| 🟢 | EX8 | PerforatedBlock | 0.7 ms, 306 instances, 11081 bytes |
| 🟢 | EX5 | HollowBlock | 320 instances, 0 dangling, 0 repeated; faces 12/12, edges 24/24, vertices 16/16, shells 2/2, solids 1/1; edge uses not (1 forward + 1 reverse): 0; loop breaks 0; repeatable: yes |
| 🟢 | EX6 | HollowBlock | PLANE 12 (0 rational); wrong carrier kinds 0; vertices bit-identical: yes; stand-ins 0 surfaces / 0 curves, deviation 0.00e+00; exact V 114, A 170 |
| 🟢 | EX8 | HollowBlock | 0.8 ms, 320 instances, 11571 bytes |
| 🟢 | EX5 | PipeBend | 74 instances, 0 dangling, 0 repeated; faces 3/3, edges 3/3, vertices 2/2, shells 1/1, solids 1/1; edge uses not (1 forward + 1 reverse): 0; loop breaks 0; repeatable: yes |
| 🟢 | EX6 | PipeBend | PLANE 2, TOROIDAL_SURFACE 1 (0 rational); wrong carrier kinds 0; vertices bit-identical: yes; stand-ins 0 surfaces / 0 curves, deviation 0.00e+00; exact V 2.46740110027, A 11.4404007279 |
| 🟢 | EX8 | PipeBend | 1.0 ms, 74 instances, 3427 bytes |
| 🟢 | EX5 | LoftFrustum | 73 instances, 0 dangling, 0 repeated; faces 3/3, edges 3/3, vertices 2/2, shells 1/1, solids 1/1; edge uses not (1 forward + 1 reverse): 0; loop breaks 0; repeatable: yes |
| 🟢 | EX6 | LoftFrustum | CONICAL_SURFACE 1, PLANE 2 (0 rational); wrong carrier kinds 0; vertices bit-identical: yes; stand-ins 0 surfaces / 0 curves, deviation 0.00e+00; exact V 8.24668071567, A 23.9342302499 |
| 🟢 | EX8 | LoftFrustum | 1.5 ms, 73 instances, 3324 bytes |
| 🟢 | EX5 | SweptRing | 112 instances, 0 dangling, 0 repeated; faces 4/4, edges 6/6, vertices 4/4, shells 1/1, solids 1/1; edge uses not (1 forward + 1 reverse): 0; loop breaks 0; repeatable: yes |
| 🟢 | EX6 | SweptRing | CYLINDRICAL_SURFACE 2, PLANE 2 (0 rational); wrong carrier kinds 0; vertices bit-identical: yes; stand-ins 0 surfaces / 0 curves, deviation 0.00e+00; exact V 10.1159283446, A 39.0185807576 |
| 🟢 | EX8 | SweptRing | 2.3 ms, 112 instances, 4557 bytes |
| 🟢 | EX5 | MirroredCross | 553 instances, 0 dangling, 0 repeated; faces 7/7, edges 9/9, vertices 6/6, shells 1/1, solids 1/1; edge uses not (1 forward + 1 reverse): 0; loop breaks 0; repeatable: yes |
| 🟢 | EX6 | MirroredCross | CYLINDRICAL_SURFACE 3, PLANE 4 (0 rational); wrong carrier kinds 0; vertices bit-identical: yes; stand-ins 0 surfaces / 2 curves, deviation 1.26e-07; exact V 4.00319568135, A 19.9455739929 |
| 🟢 | EX8 | MirroredCross | 4.1 ms, 553 instances, 47043 bytes |
| 🟢 | EX5 | BoxAllFillet | 638 instances, 0 dangling, 0 repeated; faces 26/26, edges 48/48, vertices 24/24, shells 1/1, solids 1/1; edge uses not (1 forward + 1 reverse): 0; loop breaks 0; repeatable: yes |
| 🟢 | EX6 | BoxAllFillet | CYLINDRICAL_SURFACE 12, PLANE 6, SPHERICAL_SURFACE 8 (0 rational); wrong carrier kinds 0; vertices bit-identical: yes; stand-ins 0 surfaces / 0 curves, deviation 0.00e+00; exact V 23.3408493751, A 47.2626536188 |
| 🟢 | EX8 | BoxAllFillet | 4.5 ms, 638 instances, 25384 bytes |
| 🟢 | EX5 | BoxAllChamfer | 566 instances, 0 dangling, 0 repeated; faces 18/18, edges 48/48, vertices 32/32, shells 1/1, solids 1/1; edge uses not (1 forward + 1 reverse): 0; loop breaks 0; repeatable: yes |
| 🟢 | EX6 | BoxAllChamfer | PLANE 18 (0 rational); wrong carrier kinds 0; vertices bit-identical: yes; stand-ins 0 surfaces / 0 curves, deviation 0.00e+00; exact V 22.542, A 45.5424805026 |
| 🟢 | EX8 | BoxAllChamfer | 1.6 ms, 566 instances, 23172 bytes |
| 🟢 | EX5 | HoleRimFillet | 235 instances, 0 dangling, 0 repeated; faces 8/8, edges 17/17, vertices 11/11, shells 1/1, solids 1/1; edge uses not (1 forward + 1 reverse): 0; loop breaks 0; repeatable: yes |
| 🟢 | EX6 | HoleRimFillet | CYLINDRICAL_SURFACE 1, PLANE 6, TOROIDAL_SURFACE 1 (0 rational); wrong carrier kinds 0; vertices bit-identical: yes; stand-ins 0 surfaces / 0 curves, deviation 0.00e+00; exact V 21.7032826104, A 56.9720567701 |
| 🟢 | EX8 | HoleRimFillet | 1.8 ms, 235 instances, 8831 bytes |
| 🟢 | EX5 | BossRootFillet | 241 instances, 0 dangling, 0 repeated; faces 9/9, edges 17/17, vertices 11/11, shells 1/1, solids 1/1; edge uses not (1 forward + 1 reverse): 0; loop breaks 0; repeatable: yes |
| 🟢 | EX6 | BossRootFillet | CYLINDRICAL_SURFACE 1, PLANE 7, TOROIDAL_SURFACE 1 (0 rational); wrong carrier kinds 0; vertices bit-identical: yes; stand-ins 0 surfaces / 0 curves, deviation 0.00e+00; exact V 25.1657440343, A 55.4640922963 |
| 🟢 | EX8 | BossRootFillet | 2.4 ms, 241 instances, 8978 bytes |
| 🟢 | EX5 | VariableFillet | 299 instances, 0 dangling, 0 repeated; faces 7/7, edges 15/15, vertices 10/10, shells 1/1, solids 1/1; edge uses not (1 forward + 1 reverse): 0; loop breaks 0; repeatable: yes |
| 🟢 | EX6 | VariableFillet | CONICAL_SURFACE 1, PLANE 6 (0 rational); wrong carrier kinds 0; vertices bit-identical: yes; stand-ins 0 surfaces / 2 curves, deviation 1.09e-07; exact V 23.9400226677, A 51.5351756746 |
| 🟢 | EX8 | VariableFillet | 1.7 ms, 299 instances, 16972 bytes |
| 🟢 | EX5 | SmoothFillet | 207 instances, 0 dangling, 0 repeated; faces 7/7, edges 15/15, vertices 10/10, shells 1/1, solids 1/1; edge uses not (1 forward + 1 reverse): 0; loop breaks 0; repeatable: yes |
| 🟢 | EX6 | SmoothFillet | CYLINDRICAL_SURFACE 2, PLANE 5 (0 rational); wrong carrier kinds 0; vertices bit-identical: yes; stand-ins 0 surfaces / 0 curves, deviation 0.00e+00; exact V 7.27935257912, A 25.574241854 |
| 🟢 | EX8 | SmoothFillet | 0.9 ms, 207 instances, 8213 bytes |
| 🟢 | EX5 | TiltedBoreChamfer | 2120 instances, 0 dangling, 0 repeated; faces 8/8, edges 17/17, vertices 11/11, shells 1/1, solids 1/1; edge uses not (1 forward + 1 reverse): 0; loop breaks 0; repeatable: yes |
| 🟢 | EX6 | TiltedBoreChamfer | B_SPLINE_SURFACE 1, CYLINDRICAL_SURFACE 1, PLANE 6 (0 rational); wrong carrier kinds 0; vertices bit-identical: yes; stand-ins 1 surfaces / 3 curves, deviation 2.29e-07; exact V 15.1477357444, A 49.4921278086 |
| 🟢 | EX8 | TiltedBoreChamfer | 34.5 ms, 2120 instances, 193526 bytes |
| 🟢 | EX5 | TiltedBossChamfer | 1943 instances, 0 dangling, 0 repeated; faces 9/9, edges 17/17, vertices 11/11, shells 1/1, solids 1/1; edge uses not (1 forward + 1 reverse): 0; loop breaks 0; repeatable: yes |
| 🟢 | EX6 | TiltedBossChamfer | B_SPLINE_SURFACE 1, CYLINDRICAL_SURFACE 1, PLANE 7 (0 rational); wrong carrier kinds 0; vertices bit-identical: yes; stand-ins 1 surfaces / 2 curves, deviation 2.29e-07; exact V 16.801859151, A 50.9117088214 |
| 🟢 | EX8 | TiltedBossChamfer | 29.2 ms, 1943 instances, 174102 bytes |
| 🟢 | EX5 | VariableArcFillet | 993 instances, 0 dangling, 0 repeated; faces 5/5, edges 9/9, vertices 6/6, shells 1/1, solids 1/1; edge uses not (1 forward + 1 reverse): 0; loop breaks 0; repeatable: yes |
| 🟢 | EX6 | VariableArcFillet | B_SPLINE_SURFACE 1, CYLINDRICAL_SURFACE 1, PLANE 3 (0 rational); wrong carrier kinds 0; vertices bit-identical: yes; stand-ins 1 surfaces / 4 curves, deviation 3.29e-07; exact V 1.54313809616, A 7.98291566247 |
| 🟢 | EX8 | VariableArcFillet | 44.9 ms, 993 instances, 87025 bytes |
| 🟢 | EX5 | VariableArcChamfer | 493 instances, 0 dangling, 0 repeated; faces 5/5, edges 9/9, vertices 6/6, shells 1/1, solids 1/1; edge uses not (1 forward + 1 reverse): 0; loop breaks 0; repeatable: yes |
| 🟢 | EX6 | VariableArcChamfer | B_SPLINE_SURFACE 1, CYLINDRICAL_SURFACE 1, PLANE 3 (0 rational); wrong carrier kinds 0; vertices bit-identical: yes; stand-ins 1 surfaces / 4 curves, deviation 2.91e-07; exact V 1.50796447372, A 7.8379458189 |
| 🟢 | EX8 | VariableArcChamfer | 11.9 ms, 493 instances, 40754 bytes |
| 🟢 | EX5 | PromotedTorus | 135 instances, 0 dangling, 0 repeated; faces 1/1, edges 2/2, vertices 1/1, shells 1/1, solids 1/1; edge uses not (1 forward + 1 reverse): 0; loop breaks 0; repeatable: yes |
| 🟢 | EX6 | PromotedTorus | B_SPLINE_SURFACE 1 (1 rational); wrong carrier kinds 0; vertices bit-identical: yes; stand-ins 0 surfaces / 0 curves, deviation 0.00e+00; exact V 59.2176264065, A 118.435252813 |
| 🟢 | EX8 | PromotedTorus | 53.0 ms, 135 instances, 10845 bytes |
| 🟢 | EX5 | PromotedSphere | 84 instances, 0 dangling, 0 repeated; faces 1/1, edges 1/1, vertices 2/2, shells 1/1, solids 1/1; edge uses not (1 forward + 1 reverse): 0; loop breaks 0; repeatable: yes |
| 🟢 | EX6 | PromotedSphere | B_SPLINE_SURFACE 1 (1 rational); wrong carrier kinds 0; vertices bit-identical: yes; stand-ins 0 surfaces / 0 curves, deviation 0.00e+00; exact V 33.5103216383, A 50.2654824574 |
| 🟢 | EX8 | PromotedSphere | 103.6 ms, 84 instances, 6779 bytes |
| 🟢 | EX5 | PromotedCylinder | 96 instances, 0 dangling, 0 repeated; faces 3/3, edges 3/3, vertices 2/2, shells 1/1, solids 1/1; edge uses not (1 forward + 1 reverse): 0; loop breaks 0; repeatable: yes |
| 🟢 | EX6 | PromotedCylinder | B_SPLINE_SURFACE 3 (1 rational); wrong carrier kinds 0; vertices bit-identical: yes; stand-ins 0 surfaces / 0 curves, deviation 0.00e+00; exact V 21.2057504117, A 42.4115008235 |
| 🟢 | EX8 | PromotedCylinder | 18.8 ms, 96 instances, 6941 bytes |
| 🟢 | EX5 | PromotedBoxAllFillet | 1086 instances, 0 dangling, 0 repeated; faces 26/26, edges 48/48, vertices 24/24, shells 1/1, solids 1/1; edge uses not (1 forward + 1 reverse): 0; loop breaks 0; repeatable: yes |
| 🟢 | EX6 | PromotedBoxAllFillet | B_SPLINE_SURFACE 26 (20 rational); wrong carrier kinds 0; vertices bit-identical: yes; stand-ins 0 surfaces / 0 curves, deviation 0.00e+00; exact V 23.3408493751, A 47.2626536188 |
| 🟢 | EX8 | PromotedBoxAllFillet | 49.6 ms, 1086 instances, 91016 bytes |
| 🟢 | EX5 | ApproximatedHoleRimFillet | 327 instances, 0 dangling, 0 repeated; faces 8/8, edges 17/17, vertices 11/11, shells 1/1, solids 1/1; edge uses not (1 forward + 1 reverse): 0; loop breaks 0; repeatable: yes |
| 🟢 | EX6 | ApproximatedHoleRimFillet | B_SPLINE_SURFACE 8 (2 rational); wrong carrier kinds 0; vertices bit-identical: yes; stand-ins 0 surfaces / 0 curves, deviation 0.00e+00; exact V 21.7032826104, A 56.9720567701 |
| 🟢 | EX8 | ApproximatedHoleRimFillet | 27.8 ms, 327 instances, 22200 bytes |
| 🟢 | EX6 | Block 4 × 2.5 × 1 m at (0.25, 0.5, 0.125) → mm | length unit SI_UNIT(.MILLI.,.METRE.): yes; 8 corners = model × 1000 bit-identical: yes |
| 🟢 | EX7 | no bodies | STEP: no body to export |
| 🟢 | EX7 | zero unit | STEP: the model unit must be a positive length |
| 🟢 | EX7 | negative unit | STEP: the model unit must be a positive length |
| 🟢 | EX7 | zero tolerance | STEP: the approximation tolerance must be positive |
| 🟢 | EX7 | stale token | STEP: a body is not in the partition |
| 🟢 | EX7 | sheet body | STEP: only solid bodies are exported (a sheet or wire body was given) |
