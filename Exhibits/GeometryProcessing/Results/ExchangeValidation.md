# Exchange validation

🟢 Priority 4: STL export and read-back; STEP AP214 export and import (153.45 s)

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
| 🟢 | EX8 | STEP export time per body (stand-in fitting included) | ≤ 2 s | 1.01e-01 | 26 | 0 |
| 🟢 | EX9 | STEP import of every exported body (ReadStep → AssembleBody → AuditBody sound): faces, loops, edges, vertices, shells and solid regions = the original's; carrier kinds kept (stand-ins arrive as NURBS); every vertex bit-identical; exported again, read again, exported again → the same bytes (fixed point after one generation); two bodies in one file → two bodies; a metre model read in metres and in millimetres | exact | 0.00e+00 | 28 | 0 |
| 🟢 | EX10 | STEP import geometry: |ΔV| and |ΔA| against the original's exact mass properties — ≤ 10⁻⁹ relative where every carrier was written exactly, ≤ 2 × stand-in deviation × area otherwise (tolerant edges absorb the stand-in gap, counted) | ≤ 10⁻⁹ rel · ≤ 2δA | 6.60e-08 | 26 | 0 |
| 🟢 | EX11 | OpenCascade-authored STEP (StepFixtures.py: fillet spheres, chamfer cones, torus with two seams, edge-less sphere, apex cone, B-spline loft, surface of revolution / linear extrusion, void, inch units, a boss fillet whose torus loop OpenCascade wrote backwards): read sound; solids, shells, faces, edges, vertices = OpenCascade's (degenerated edges excluded); |ΔV|, |ΔA| against GProp (ε 10⁻¹²; Gauss–Legendre of the exact profile where GProp's quadrature is off); OpenCascade's own triangulation points on the imported carriers | ≤ 10⁻⁶ rel · ≤ 10⁻⁶ mm | 2.55e-12 | 11 | 0 |
| 🟢 | EX12 | STEP import refusals name the cause and add nothing to the partition: empty input, no DATA, unterminated DATA, a missing instance, an unsupported surface, an open shell, no solid, a non-metre length unit, a zero model unit, a missing file, a two-solid file whose second solid is broken | named · atomic | 0.00e+00 | 11 | 0 |
| 🟢 | EX13 | STEP import time per file (parse, conversion, SP-curves, assembly, audit twice) | ≤ 2 s | 1.46e+00 | 37 | 0 |

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
| 🟢 | EX8 | Block | 0.4 ms, 170 instances, 6411 bytes |
| 🟢 | EX5 | Cylinder | 73 instances, 0 dangling, 0 repeated; faces 3/3, edges 3/3, vertices 2/2, shells 1/1, solids 1/1; edge uses not (1 forward + 1 reverse): 0; loop breaks 0; repeatable: yes |
| 🟢 | EX6 | Cylinder | CYLINDRICAL_SURFACE 1, PLANE 2 (0 rational); wrong carrier kinds 0; vertices bit-identical: yes; stand-ins 0 surfaces / 0 curves, deviation 0.00e+00; exact V 21.2057504117, A 42.4115008235 |
| 🟢 | EX8 | Cylinder | 2.4 ms, 73 instances, 3260 bytes |
| 🟢 | EX5 | Cone | 57 instances, 0 dangling, 0 repeated; faces 2/2, edges 2/2, vertices 2/2, shells 1/1, solids 1/1; edge uses not (1 forward + 1 reverse): 0; loop breaks 0; repeatable: yes |
| 🟢 | EX6 | Cone | CONICAL_SURFACE 1, PLANE 1 (0 rational); wrong carrier kinds 0; vertices bit-identical: yes; stand-ins 0 surfaces / 0 curves, deviation 0.00e+00; exact V 12.5663706144, A 35.2207174126 |
| 🟢 | EX8 | Cone | 1.6 ms, 57 instances, 2769 bytes |
| 🟢 | EX5 | Sphere | 42 instances, 0 dangling, 0 repeated; faces 1/1, edges 1/1, vertices 2/2, shells 1/1, solids 1/1; edge uses not (1 forward + 1 reverse): 0; loop breaks 0; repeatable: yes |
| 🟢 | EX6 | Sphere | SPHERICAL_SURFACE 1 (0 rational); wrong carrier kinds 0; vertices bit-identical: yes; stand-ins 0 surfaces / 0 curves, deviation 0.00e+00; exact V 33.5103216383, A 50.2654824574 |
| 🟢 | EX8 | Sphere | 2.2 ms, 42 instances, 2257 bytes |
| 🟢 | EX5 | Torus | 48 instances, 0 dangling, 0 repeated; faces 1/1, edges 2/2, vertices 1/1, shells 1/1, solids 1/1; edge uses not (1 forward + 1 reverse): 0; loop breaks 0; repeatable: yes |
| 🟢 | EX6 | Torus | TOROIDAL_SURFACE 1 (0 rational); wrong carrier kinds 0; vertices bit-identical: yes; stand-ins 0 surfaces / 0 curves, deviation 0.00e+00; exact V 59.2176264065, A 118.435252813 |
| 🟢 | EX8 | Torus | 1.2 ms, 48 instances, 2465 bytes |
| 🟢 | EX5 | PerforatedBlock | 306 instances, 0 dangling, 0 repeated; faces 10/10, edges 24/24, vertices 16/16, shells 1/1, solids 1/1; edge uses not (1 forward + 1 reverse): 0; loop breaks 0; repeatable: yes |
| 🟢 | EX6 | PerforatedBlock | PLANE 10 (0 rational); wrong carrier kinds 0; vertices bit-identical: yes; stand-ins 0 surfaces / 0 curves, deviation 0.00e+00; exact V 42, A 96 |
| 🟢 | EX8 | PerforatedBlock | 0.6 ms, 306 instances, 11081 bytes |
| 🟢 | EX5 | HollowBlock | 320 instances, 0 dangling, 0 repeated; faces 12/12, edges 24/24, vertices 16/16, shells 2/2, solids 1/1; edge uses not (1 forward + 1 reverse): 0; loop breaks 0; repeatable: yes |
| 🟢 | EX6 | HollowBlock | PLANE 12 (0 rational); wrong carrier kinds 0; vertices bit-identical: yes; stand-ins 0 surfaces / 0 curves, deviation 0.00e+00; exact V 114, A 170 |
| 🟢 | EX8 | HollowBlock | 0.7 ms, 320 instances, 11571 bytes |
| 🟢 | EX5 | PipeBend | 74 instances, 0 dangling, 0 repeated; faces 3/3, edges 3/3, vertices 2/2, shells 1/1, solids 1/1; edge uses not (1 forward + 1 reverse): 0; loop breaks 0; repeatable: yes |
| 🟢 | EX6 | PipeBend | PLANE 2, TOROIDAL_SURFACE 1 (0 rational); wrong carrier kinds 0; vertices bit-identical: yes; stand-ins 0 surfaces / 0 curves, deviation 0.00e+00; exact V 2.46740110027, A 11.4404007279 |
| 🟢 | EX8 | PipeBend | 1.0 ms, 74 instances, 3427 bytes |
| 🟢 | EX5 | LoftFrustum | 73 instances, 0 dangling, 0 repeated; faces 3/3, edges 3/3, vertices 2/2, shells 1/1, solids 1/1; edge uses not (1 forward + 1 reverse): 0; loop breaks 0; repeatable: yes |
| 🟢 | EX6 | LoftFrustum | CONICAL_SURFACE 1, PLANE 2 (0 rational); wrong carrier kinds 0; vertices bit-identical: yes; stand-ins 0 surfaces / 0 curves, deviation 0.00e+00; exact V 8.24668071567, A 23.9342302499 |
| 🟢 | EX8 | LoftFrustum | 1.5 ms, 73 instances, 3324 bytes |
| 🟢 | EX5 | SweptRing | 112 instances, 0 dangling, 0 repeated; faces 4/4, edges 6/6, vertices 4/4, shells 1/1, solids 1/1; edge uses not (1 forward + 1 reverse): 0; loop breaks 0; repeatable: yes |
| 🟢 | EX6 | SweptRing | CYLINDRICAL_SURFACE 2, PLANE 2 (0 rational); wrong carrier kinds 0; vertices bit-identical: yes; stand-ins 0 surfaces / 0 curves, deviation 0.00e+00; exact V 10.1159283446, A 39.0185807576 |
| 🟢 | EX8 | SweptRing | 2.5 ms, 112 instances, 4557 bytes |
| 🟢 | EX5 | MirroredCross | 553 instances, 0 dangling, 0 repeated; faces 7/7, edges 9/9, vertices 6/6, shells 1/1, solids 1/1; edge uses not (1 forward + 1 reverse): 0; loop breaks 0; repeatable: yes |
| 🟢 | EX6 | MirroredCross | CYLINDRICAL_SURFACE 3, PLANE 4 (0 rational); wrong carrier kinds 0; vertices bit-identical: yes; stand-ins 0 surfaces / 2 curves, deviation 1.26e-07; exact V 4.00319568135, A 19.9455739929 |
| 🟢 | EX8 | MirroredCross | 3.9 ms, 553 instances, 47133 bytes |
| 🟢 | EX5 | BoxAllFillet | 638 instances, 0 dangling, 0 repeated; faces 26/26, edges 48/48, vertices 24/24, shells 1/1, solids 1/1; edge uses not (1 forward + 1 reverse): 0; loop breaks 0; repeatable: yes |
| 🟢 | EX6 | BoxAllFillet | CYLINDRICAL_SURFACE 12, PLANE 6, SPHERICAL_SURFACE 8 (0 rational); wrong carrier kinds 0; vertices bit-identical: yes; stand-ins 0 surfaces / 0 curves, deviation 0.00e+00; exact V 23.3408493751, A 47.2626536188 |
| 🟢 | EX8 | BoxAllFillet | 4.1 ms, 638 instances, 25480 bytes |
| 🟢 | EX5 | BoxAllChamfer | 566 instances, 0 dangling, 0 repeated; faces 18/18, edges 48/48, vertices 32/32, shells 1/1, solids 1/1; edge uses not (1 forward + 1 reverse): 0; loop breaks 0; repeatable: yes |
| 🟢 | EX6 | BoxAllChamfer | PLANE 18 (0 rational); wrong carrier kinds 0; vertices bit-identical: yes; stand-ins 0 surfaces / 0 curves, deviation 0.00e+00; exact V 22.542, A 45.5424805026 |
| 🟢 | EX8 | BoxAllChamfer | 1.4 ms, 566 instances, 23172 bytes |
| 🟢 | EX5 | HoleRimFillet | 235 instances, 0 dangling, 0 repeated; faces 8/8, edges 17/17, vertices 11/11, shells 1/1, solids 1/1; edge uses not (1 forward + 1 reverse): 0; loop breaks 0; repeatable: yes |
| 🟢 | EX6 | HoleRimFillet | CYLINDRICAL_SURFACE 1, PLANE 6, TOROIDAL_SURFACE 1 (0 rational); wrong carrier kinds 0; vertices bit-identical: yes; stand-ins 0 surfaces / 0 curves, deviation 0.00e+00; exact V 21.7032826104, A 56.9720567701 |
| 🟢 | EX8 | HoleRimFillet | 1.4 ms, 235 instances, 8831 bytes |
| 🟢 | EX5 | BossRootFillet | 241 instances, 0 dangling, 0 repeated; faces 9/9, edges 17/17, vertices 11/11, shells 1/1, solids 1/1; edge uses not (1 forward + 1 reverse): 0; loop breaks 0; repeatable: yes |
| 🟢 | EX6 | BossRootFillet | CYLINDRICAL_SURFACE 1, PLANE 7, TOROIDAL_SURFACE 1 (0 rational); wrong carrier kinds 0; vertices bit-identical: yes; stand-ins 0 surfaces / 0 curves, deviation 0.00e+00; exact V 25.1657440343, A 55.4640922963 |
| 🟢 | EX8 | BossRootFillet | 1.6 ms, 241 instances, 8978 bytes |
| 🟢 | EX5 | VariableFillet | 299 instances, 0 dangling, 0 repeated; faces 7/7, edges 15/15, vertices 10/10, shells 1/1, solids 1/1; edge uses not (1 forward + 1 reverse): 0; loop breaks 0; repeatable: yes |
| 🟢 | EX6 | VariableFillet | CONICAL_SURFACE 1, PLANE 6 (0 rational); wrong carrier kinds 0; vertices bit-identical: yes; stand-ins 0 surfaces / 2 curves, deviation 1.09e-07; exact V 23.9400226677, A 51.5351756746 |
| 🟢 | EX8 | VariableFillet | 1.3 ms, 299 instances, 16972 bytes |
| 🟢 | EX5 | SmoothFillet | 207 instances, 0 dangling, 0 repeated; faces 7/7, edges 15/15, vertices 10/10, shells 1/1, solids 1/1; edge uses not (1 forward + 1 reverse): 0; loop breaks 0; repeatable: yes |
| 🟢 | EX6 | SmoothFillet | CYLINDRICAL_SURFACE 2, PLANE 5 (0 rational); wrong carrier kinds 0; vertices bit-identical: yes; stand-ins 0 surfaces / 0 curves, deviation 0.00e+00; exact V 7.27935257912, A 25.574241854 |
| 🟢 | EX8 | SmoothFillet | 1.2 ms, 207 instances, 8213 bytes |
| 🟢 | EX5 | TiltedBoreChamfer | 2120 instances, 0 dangling, 0 repeated; faces 8/8, edges 17/17, vertices 11/11, shells 1/1, solids 1/1; edge uses not (1 forward + 1 reverse): 0; loop breaks 0; repeatable: yes |
| 🟢 | EX6 | TiltedBoreChamfer | B_SPLINE_SURFACE 1, CYLINDRICAL_SURFACE 1, PLANE 6 (0 rational); wrong carrier kinds 0; vertices bit-identical: yes; stand-ins 1 surfaces / 3 curves, deviation 2.29e-07; exact V 15.1477357444, A 49.4921278086 |
| 🟢 | EX8 | TiltedBoreChamfer | 31.9 ms, 2120 instances, 193526 bytes |
| 🟢 | EX5 | TiltedBossChamfer | 1943 instances, 0 dangling, 0 repeated; faces 9/9, edges 17/17, vertices 11/11, shells 1/1, solids 1/1; edge uses not (1 forward + 1 reverse): 0; loop breaks 0; repeatable: yes |
| 🟢 | EX6 | TiltedBossChamfer | B_SPLINE_SURFACE 1, CYLINDRICAL_SURFACE 1, PLANE 7 (0 rational); wrong carrier kinds 0; vertices bit-identical: yes; stand-ins 1 surfaces / 2 curves, deviation 2.29e-07; exact V 16.801859151, A 50.9117088214 |
| 🟢 | EX8 | TiltedBossChamfer | 34.9 ms, 1943 instances, 174102 bytes |
| 🟢 | EX5 | VariableArcFillet | 993 instances, 0 dangling, 0 repeated; faces 5/5, edges 9/9, vertices 6/6, shells 1/1, solids 1/1; edge uses not (1 forward + 1 reverse): 0; loop breaks 0; repeatable: yes |
| 🟢 | EX6 | VariableArcFillet | B_SPLINE_SURFACE 1, CYLINDRICAL_SURFACE 1, PLANE 3 (0 rational); wrong carrier kinds 0; vertices bit-identical: yes; stand-ins 1 surfaces / 4 curves, deviation 3.29e-07; exact V 1.54313809616, A 7.98291566247 |
| 🟢 | EX8 | VariableArcFillet | 41.1 ms, 993 instances, 87025 bytes |
| 🟢 | EX5 | VariableArcChamfer | 493 instances, 0 dangling, 0 repeated; faces 5/5, edges 9/9, vertices 6/6, shells 1/1, solids 1/1; edge uses not (1 forward + 1 reverse): 0; loop breaks 0; repeatable: yes |
| 🟢 | EX6 | VariableArcChamfer | B_SPLINE_SURFACE 1, CYLINDRICAL_SURFACE 1, PLANE 3 (0 rational); wrong carrier kinds 0; vertices bit-identical: yes; stand-ins 1 surfaces / 4 curves, deviation 2.91e-07; exact V 1.50796447372, A 7.8379458189 |
| 🟢 | EX8 | VariableArcChamfer | 13.8 ms, 493 instances, 40754 bytes |
| 🟢 | EX5 | PromotedTorus | 135 instances, 0 dangling, 0 repeated; faces 1/1, edges 2/2, vertices 1/1, shells 1/1, solids 1/1; edge uses not (1 forward + 1 reverse): 0; loop breaks 0; repeatable: yes |
| 🟢 | EX6 | PromotedTorus | B_SPLINE_SURFACE 1 (1 rational); wrong carrier kinds 0; vertices bit-identical: yes; stand-ins 0 surfaces / 0 curves, deviation 0.00e+00; exact V 59.2176264065, A 118.435252813 |
| 🟢 | EX8 | PromotedTorus | 49.4 ms, 135 instances, 10845 bytes |
| 🟢 | EX5 | PromotedSphere | 84 instances, 0 dangling, 0 repeated; faces 1/1, edges 1/1, vertices 2/2, shells 1/1, solids 1/1; edge uses not (1 forward + 1 reverse): 0; loop breaks 0; repeatable: yes |
| 🟢 | EX6 | PromotedSphere | B_SPLINE_SURFACE 1 (1 rational); wrong carrier kinds 0; vertices bit-identical: yes; stand-ins 0 surfaces / 0 curves, deviation 0.00e+00; exact V 33.5103216383, A 50.2654824574 |
| 🟢 | EX8 | PromotedSphere | 101.2 ms, 84 instances, 6779 bytes |
| 🟢 | EX5 | PromotedCylinder | 96 instances, 0 dangling, 0 repeated; faces 3/3, edges 3/3, vertices 2/2, shells 1/1, solids 1/1; edge uses not (1 forward + 1 reverse): 0; loop breaks 0; repeatable: yes |
| 🟢 | EX6 | PromotedCylinder | B_SPLINE_SURFACE 3 (1 rational); wrong carrier kinds 0; vertices bit-identical: yes; stand-ins 0 surfaces / 0 curves, deviation 0.00e+00; exact V 21.2057504117, A 42.4115008235 |
| 🟢 | EX8 | PromotedCylinder | 22.1 ms, 96 instances, 6941 bytes |
| 🟢 | EX5 | PromotedBoxAllFillet | 1086 instances, 0 dangling, 0 repeated; faces 26/26, edges 48/48, vertices 24/24, shells 1/1, solids 1/1; edge uses not (1 forward + 1 reverse): 0; loop breaks 0; repeatable: yes |
| 🟢 | EX6 | PromotedBoxAllFillet | B_SPLINE_SURFACE 26 (20 rational); wrong carrier kinds 0; vertices bit-identical: yes; stand-ins 0 surfaces / 0 curves, deviation 0.00e+00; exact V 23.3408493751, A 47.2626536188 |
| 🟢 | EX8 | PromotedBoxAllFillet | 61.4 ms, 1086 instances, 91016 bytes |
| 🟢 | EX5 | ApproximatedHoleRimFillet | 327 instances, 0 dangling, 0 repeated; faces 8/8, edges 17/17, vertices 11/11, shells 1/1, solids 1/1; edge uses not (1 forward + 1 reverse): 0; loop breaks 0; repeatable: yes |
| 🟢 | EX6 | ApproximatedHoleRimFillet | B_SPLINE_SURFACE 8 (2 rational); wrong carrier kinds 0; vertices bit-identical: yes; stand-ins 0 surfaces / 0 curves, deviation 0.00e+00; exact V 21.7032826104, A 56.9720567701 |
| 🟢 | EX8 | ApproximatedHoleRimFillet | 15.8 ms, 327 instances, 22200 bytes |
| 🟢 | EX6 | Block 4 × 2.5 × 1 m at (0.25, 0.5, 0.125) → mm | length unit SI_UNIT(.MILLI.,.METRE.): yes; 8 corners = model × 1000 bit-identical: yes |
| 🟢 | EX7 | no bodies | STEP: no body to export |
| 🟢 | EX7 | zero unit | STEP: the model unit must be a positive length |
| 🟢 | EX7 | negative unit | STEP: the model unit must be a positive length |
| 🟢 | EX7 | zero tolerance | STEP: the approximation tolerance must be positive |
| 🟢 | EX7 | stale token | STEP: a body is not in the partition |
| 🟢 | EX7 | sheet body | STEP: only solid bodies are exported (a sheet or wire body was given) |
| 🟢 | EX13 | Block | 13.5 ms, 170 instances |
| 🟢 | EX9 | Block | F 6 L 6 E 12 V 8 shells 1 solids 1; kinds kept: yes; vertices bit-identical: yes; tolerant edges 0 / vertices 0 (largest gap 0.0e+00); generation 2 = 3: yes |
| 🟢 | EX10 | Block | exact carriers; ΔV 0.00e+00 (limit 2.40e-08), ΔA 0.00e+00 (limit 5.20e-08); V 24 |
| 🟢 | EX13 | Cylinder | 8.6 ms, 73 instances |
| 🟢 | EX9 | Cylinder | F 3 L 3 E 3 V 2 shells 1 solids 1; kinds kept: yes; vertices bit-identical: yes; tolerant edges 0 / vertices 0 (largest gap 3.7e-16); generation 2 = 3: yes |
| 🟢 | EX10 | Cylinder | exact carriers; ΔV 0.00e+00 (limit 2.12e-08), ΔA 0.00e+00 (limit 4.24e-08); V 21.2057504117 |
| 🟢 | EX13 | Cone | 8.8 ms, 57 instances |
| 🟢 | EX9 | Cone | F 2 L 2 E 2 V 2 shells 1 solids 1; kinds kept: yes; vertices bit-identical: yes; tolerant edges 0 / vertices 0 (largest gap 9.2e-16); generation 2 = 3: yes |
| 🟢 | EX10 | Cone | exact carriers; ΔV 1.78e-15 (limit 1.26e-08), ΔA 0.00e+00 (limit 3.52e-08); V 12.5663706144 |
| 🟢 | EX13 | Sphere | 8.0 ms, 42 instances |
| 🟢 | EX9 | Sphere | F 1 L 1 E 1 V 2 shells 1 solids 1; kinds kept: yes; vertices bit-identical: yes; tolerant edges 0 / vertices 0 (largest gap 9.3e-16); generation 2 = 3: yes |
| 🟢 | EX10 | Sphere | exact carriers; ΔV 3.55e-14 (limit 3.35e-08), ΔA 4.97e-14 (limit 5.03e-08); V 33.5103216383 |
| 🟢 | EX13 | Torus | 15.1 ms, 48 instances |
| 🟢 | EX9 | Torus | F 1 L 1 E 2 V 1 shells 1 solids 1; kinds kept: yes; vertices bit-identical: yes; tolerant edges 0 / vertices 0 (largest gap 1.8e-15); generation 2 = 3: yes |
| 🟢 | EX10 | Torus | exact carriers; ΔV 0.00e+00 (limit 5.92e-08), ΔA 0.00e+00 (limit 1.18e-07); V 59.2176264065 |
| 🟢 | EX13 | PerforatedBlock | 29.3 ms, 306 instances |
| 🟢 | EX9 | PerforatedBlock | F 10 L 12 E 24 V 16 shells 1 solids 1; kinds kept: yes; vertices bit-identical: yes; tolerant edges 0 / vertices 0 (largest gap 0.0e+00); generation 2 = 3: yes |
| 🟢 | EX10 | PerforatedBlock | exact carriers; ΔV 0.00e+00 (limit 4.20e-08), ΔA 0.00e+00 (limit 9.60e-08); V 42 |
| 🟢 | EX13 | HollowBlock | 43.9 ms, 320 instances |
| 🟢 | EX9 | HollowBlock | F 12 L 12 E 24 V 16 shells 2 solids 1; kinds kept: yes; vertices bit-identical: yes; tolerant edges 0 / vertices 0 (largest gap 0.0e+00); generation 2 = 3: yes |
| 🟢 | EX10 | HollowBlock | exact carriers; ΔV 0.00e+00 (limit 1.14e-07), ΔA 0.00e+00 (limit 1.70e-07); V 114 |
| 🟢 | EX13 | PipeBend | 10.6 ms, 74 instances |
| 🟢 | EX9 | PipeBend | F 3 L 3 E 3 V 2 shells 1 solids 1; kinds kept: yes; vertices bit-identical: yes; tolerant edges 0 / vertices 0 (largest gap 3.3e-16); generation 2 = 3: yes |
| 🟢 | EX10 | PipeBend | exact carriers; ΔV 0.00e+00 (limit 2.47e-09), ΔA 0.00e+00 (limit 1.14e-08); V 2.46740110027 |
| 🟢 | EX13 | LoftFrustum | 6.5 ms, 73 instances |
| 🟢 | EX9 | LoftFrustum | F 3 L 3 E 3 V 2 shells 1 solids 1; kinds kept: yes; vertices bit-identical: yes; tolerant edges 0 / vertices 0 (largest gap 4.6e-16); generation 2 = 3: yes |
| 🟢 | EX10 | LoftFrustum | exact carriers; ΔV 1.78e-15 (limit 8.25e-09), ΔA 7.11e-15 (limit 2.39e-08); V 8.24668071567 |
| 🟢 | EX13 | SweptRing | 12.8 ms, 112 instances |
| 🟢 | EX9 | SweptRing | F 4 L 6 E 6 V 4 shells 1 solids 1; kinds kept: yes; vertices bit-identical: yes; tolerant edges 0 / vertices 0 (largest gap 3.7e-16); generation 2 = 3: yes |
| 🟢 | EX10 | SweptRing | exact carriers; ΔV 0.00e+00 (limit 1.01e-08), ΔA 0.00e+00 (limit 3.90e-08); V 10.1159283446 |
| 🟢 | EX13 | MirroredCross | 107.7 ms, 553 instances |
| 🟢 | EX9 | MirroredCross | F 7 L 9 E 9 V 6 shells 1 solids 1; kinds kept: yes; vertices bit-identical: yes; tolerant edges 2 / vertices 2 (largest gap 5.2e-08); generation 2 = 3: yes |
| 🟢 | EX10 | MirroredCross | stand-ins; ΔV 9.61e-09 (limit 5.05e-06), ΔA 5.99e-08 (limit 5.06e-06); V 4.00319567174 |
| 🟢 | EX13 | BoxAllFillet | 113.4 ms, 638 instances |
| 🟢 | EX9 | BoxAllFillet | F 26 L 26 E 48 V 24 shells 1 solids 1; kinds kept: yes; vertices bit-identical: yes; tolerant edges 0 / vertices 0 (largest gap 3.0e-16); generation 2 = 3: yes |
| 🟢 | EX10 | BoxAllFillet | exact carriers; ΔV 0.00e+00 (limit 2.33e-08), ΔA 0.00e+00 (limit 4.73e-08); V 23.3408493751 |
| 🟢 | EX13 | BoxAllChamfer | 20.6 ms, 566 instances |
| 🟢 | EX9 | BoxAllChamfer | F 18 L 18 E 48 V 32 shells 1 solids 1; kinds kept: yes; vertices bit-identical: yes; tolerant edges 0 / vertices 0 (largest gap 6.3e-16); generation 2 = 3: yes |
| 🟢 | EX10 | BoxAllChamfer | exact carriers; ΔV 0.00e+00 (limit 2.25e-08), ΔA 0.00e+00 (limit 4.55e-08); V 22.542 |
| 🟢 | EX13 | HoleRimFillet | 32.9 ms, 235 instances |
| 🟢 | EX9 | HoleRimFillet | F 8 L 10 E 17 V 11 shells 1 solids 1; kinds kept: yes; vertices bit-identical: yes; tolerant edges 0 / vertices 0 (largest gap 5.4e-16); generation 2 = 3: yes |
| 🟢 | EX10 | HoleRimFillet | exact carriers; ΔV 0.00e+00 (limit 2.17e-08), ΔA 0.00e+00 (limit 5.70e-08); V 21.7032826104 |
| 🟢 | EX13 | BossRootFillet | 28.8 ms, 241 instances |
| 🟢 | EX9 | BossRootFillet | F 9 L 10 E 17 V 11 shells 1 solids 1; kinds kept: yes; vertices bit-identical: yes; tolerant edges 0 / vertices 0 (largest gap 6.3e-16); generation 2 = 3: yes |
| 🟢 | EX10 | BossRootFillet | exact carriers; ΔV 0.00e+00 (limit 2.52e-08), ΔA 0.00e+00 (limit 5.55e-08); V 25.1657440343 |
| 🟢 | EX13 | VariableFillet | 55.8 ms, 299 instances |
| 🟢 | EX9 | VariableFillet | F 7 L 7 E 15 V 10 shells 1 solids 1; kinds kept: yes; vertices bit-identical: yes; tolerant edges 2 / vertices 4 (largest gap 7.6e-08); generation 2 = 3: yes |
| 🟢 | EX10 | VariableFillet | stand-ins; ΔV 8.39e-09 (limit 1.12e-05), ΔA 1.25e-08 (limit 1.12e-05); V 23.9400226761 |
| 🟢 | EX13 | SmoothFillet | 23.3 ms, 207 instances |
| 🟢 | EX9 | SmoothFillet | F 7 L 7 E 15 V 10 shells 1 solids 1; kinds kept: yes; vertices bit-identical: yes; tolerant edges 0 / vertices 0 (largest gap 7.8e-16); generation 2 = 3: yes |
| 🟢 | EX10 | SmoothFillet | exact carriers; ΔV 0.00e+00 (limit 7.28e-09), ΔA 3.55e-15 (limit 2.56e-08); V 7.27935257912 |
| 🟢 | EX13 | TiltedBoreChamfer | 1431.8 ms, 2120 instances |
| 🟢 | EX9 | TiltedBoreChamfer | F 8 L 10 E 17 V 11 shells 1 solids 1; kinds kept: yes; vertices bit-identical: yes; tolerant edges 3 / vertices 3 (largest gap 1.5e-07); generation 2 = 3: yes |
| 🟢 | EX10 | TiltedBoreChamfer | stand-ins; ΔV 3.85e-08 (limit 2.27e-05), ΔA 1.91e-07 (limit 2.27e-05); V 15.1477357058 |
| 🟢 | EX13 | TiltedBossChamfer | 1304.7 ms, 1943 instances |
| 🟢 | EX9 | TiltedBossChamfer | F 9 L 10 E 17 V 11 shells 1 solids 1; kinds kept: yes; vertices bit-identical: yes; tolerant edges 2 / vertices 2 (largest gap 1.0e-07); generation 2 = 3: yes |
| 🟢 | EX10 | TiltedBossChamfer | stand-ins; ΔV 1.71e-11 (limit 2.33e-05), ΔA 8.94e-08 (limit 2.34e-05); V 16.801859151 |
| 🟢 | EX13 | VariableArcFillet | 1458.7 ms, 993 instances |
| 🟢 | EX9 | VariableArcFillet | F 5 L 5 E 9 V 6 shells 1 solids 1; kinds kept: yes; vertices bit-identical: yes; tolerant edges 4 / vertices 4 (largest gap 1.2e-07); generation 2 = 3: yes |
| 🟢 | EX10 | VariableArcFillet | stand-ins; ΔV 1.02e-07 (limit 5.26e-06), ΔA 4.60e-07 (limit 5.27e-06); V 1.54313799427 |
| 🟢 | EX13 | VariableArcChamfer | 1166.6 ms, 493 instances |
| 🟢 | EX9 | VariableArcChamfer | F 5 L 5 E 9 V 6 shells 1 solids 1; kinds kept: yes; vertices bit-identical: yes; tolerant edges 4 / vertices 4 (largest gap 7.9e-08); generation 2 = 3: yes |
| 🟢 | EX10 | VariableArcChamfer | stand-ins; ΔV 8.62e-08 (limit 4.56e-06), ΔA 3.96e-07 (limit 4.57e-06); V 1.50796438749 |
| 🟢 | EX13 | PromotedTorus | 111.4 ms, 135 instances |
| 🟢 | EX9 | PromotedTorus | F 1 L 1 E 2 V 1 shells 1 solids 1; kinds kept: yes; vertices bit-identical: yes; tolerant edges 0 / vertices 0 (largest gap 4.4e-16); generation 2 = 3: yes |
| 🟢 | EX10 | PromotedTorus | exact carriers; ΔV 0.00e+00 (limit 5.92e-08), ΔA 0.00e+00 (limit 1.18e-07); V 59.2176264065 |
| 🟢 | EX13 | PromotedSphere | 39.7 ms, 84 instances |
| 🟢 | EX9 | PromotedSphere | F 1 L 1 E 1 V 2 shells 1 solids 1; kinds kept: yes; vertices bit-identical: yes; tolerant edges 0 / vertices 0 (largest gap 2.2e-16); generation 2 = 3: yes |
| 🟢 | EX10 | PromotedSphere | exact carriers; ΔV 0.00e+00 (limit 3.35e-08), ΔA 0.00e+00 (limit 5.03e-08); V 33.5103216383 |
| 🟢 | EX13 | PromotedCylinder | 136.3 ms, 96 instances |
| 🟢 | EX9 | PromotedCylinder | F 3 L 3 E 3 V 2 shells 1 solids 1; kinds kept: yes; vertices bit-identical: yes; tolerant edges 0 / vertices 0 (largest gap 9.4e-16); generation 2 = 3: yes |
| 🟢 | EX10 | PromotedCylinder | exact carriers; ΔV 0.00e+00 (limit 2.12e-08), ΔA 0.00e+00 (limit 4.24e-08); V 21.2057504117 |
| 🟢 | EX13 | PromotedBoxAllFillet | 796.4 ms, 1086 instances |
| 🟢 | EX9 | PromotedBoxAllFillet | F 26 L 26 E 48 V 24 shells 1 solids 1; kinds kept: yes; vertices bit-identical: yes; tolerant edges 0 / vertices 0 (largest gap 1.8e-15); generation 2 = 3: yes |
| 🟢 | EX10 | PromotedBoxAllFillet | exact carriers; ΔV 3.55e-15 (limit 2.33e-08), ΔA 0.00e+00 (limit 4.73e-08); V 23.3408493751 |
| 🟢 | EX13 | ApproximatedHoleRimFillet | 284.2 ms, 327 instances |
| 🟢 | EX9 | ApproximatedHoleRimFillet | F 8 L 10 E 17 V 11 shells 1 solids 1; kinds kept: yes; vertices bit-identical: yes; tolerant edges 0 / vertices 0 (largest gap 1.3e-15); generation 2 = 3: yes |
| 🟢 | EX10 | ApproximatedHoleRimFillet | exact carriers; ΔV 0.00e+00 (limit 2.17e-08), ΔA 0.00e+00 (limit 5.70e-08); V 21.7032826104 |
| 🟢 | EX9 | Block + Torus in one file | 2 bodies, 2 solids; vertices bit-identical in order: yes |
| 🟢 | EX9 | Block in metres → mm file → metres and mm | file unit 1 mm; worst corner error 0.0e+00 m (both readings) |
| 🟢 | EX13 | FilletedBox (OpenCascade) | 618.4 ms, 1326 instances |
| 🟢 | EX11 | FilletedBox | MM file: solids 1 shells 1 F 26 E 48 V 24 (OpenCascade 1 1 26 48 24); ΔV/V 1.3e-13, ΔA/A 1.1e-12; 272 points, worst 5.7e-11 mm; seams added 0, degenerate edges skipped 0, converted surfaces 0, reoriented faces 0, tolerant edges 0 (largest gap 3.6e-15 mm) |
| 🟢 | EX13 | ChamferedBore (OpenCascade) | 274.2 ms, 540 instances |
| 🟢 | EX11 | ChamferedBore | MM file: solids 1 shells 1 F 9 E 19 V 12 (OpenCascade 1 1 9 19 12); ΔV/V 2.2e-12, ΔA/A 7.0e-14; 87 points, worst 3.2e-12 mm; seams added 0, degenerate edges skipped 0, converted surfaces 0, reoriented faces 0, tolerant edges 0 (largest gap 9.5e-13 mm) |
| 🟢 | EX13 | TiltedTorus (OpenCascade) | 19.7 ms, 78 instances |
| 🟢 | EX11 | TiltedTorus | MM file: solids 1 shells 1 F 1 E 2 V 1 (OpenCascade 1 1 1 2 1); ΔV/V 2.2e-12, ΔA/A 6.1e-13; 13 points, worst 5.9e-11 mm; seams added 0, degenerate edges skipped 0, converted surfaces 0, reoriented faces 0, tolerant edges 0 (largest gap 1.3e-14 mm) |
| 🟢 | EX13 | OffsetSphere (OpenCascade) | 10.8 ms, 32 instances |
| 🟢 | EX11 | OffsetSphere | MM file: solids 1 shells 1 F 1 E 1 V 2 (OpenCascade 1 1 1 1 2); ΔV/V 4.2e-13, ΔA/A 1.8e-12; 13 points, worst 2.6e-11 mm; seams added 1, degenerate edges skipped 0, converted surfaces 0, reoriented faces 0, tolerant edges 0 (largest gap 1.8e-15 mm) |
| 🟢 | EX13 | ApexCone (OpenCascade) | 41.1 ms, 87 instances |
| 🟢 | EX11 | ApexCone | MM file: solids 1 shells 1 F 2 E 2 V 2 (OpenCascade 1 1 2 2 2); ΔV/V 1.6e-12, ΔA/A 2.5e-12; 27 points, worst 2.2e-11 mm; seams added 0, degenerate edges skipped 0, converted surfaces 0, reoriented faces 0, tolerant edges 0 (largest gap 9.8e-12 mm) |
| 🟢 | EX13 | SplineLoft (OpenCascade) | 71.1 ms, 344 instances |
| 🟢 | EX11 | SplineLoft | MM file: solids 1 shells 1 F 4 E 6 V 4 (OpenCascade 1 1 4 6 4); ΔV/V 2.5e-12, ΔA/A 2.0e-12; 56 points, worst 4.7e-11 mm; seams added 0, degenerate edges skipped 0, converted surfaces 0, reoriented faces 0, tolerant edges 0 (largest gap 2.2e-12 mm) |
| 🟢 | EX13 | RevolvedSpline (OpenCascade) | 227.5 ms, 125 instances |
| 🟢 | EX11 | RevolvedSpline | MM file: solids 1 shells 1 F 3 E 3 V 2 (OpenCascade 1 1 3 3 2); ΔV/V 2.2e-12, ΔA/A 5.8e-13; 41 points, worst 4.2e-11 mm; seams added 0, degenerate edges skipped 0, converted surfaces 1, reoriented faces 0, tolerant edges 0 (largest gap 4.1e-15 mm) |
| 🟢 | EX13 | ExtrudedSpline (OpenCascade) | 233.0 ms, 357 instances |
| 🟢 | EX11 | ExtrudedSpline | MM file: solids 1 shells 1 F 6 E 12 V 8 (OpenCascade 1 1 6 12 8); ΔV/V 2.2e-13, ΔA/A 1.9e-12; 48 points, worst 1.3e-12 mm; seams added 0, degenerate edges skipped 0, converted surfaces 1, reoriented faces 0, tolerant edges 0 (largest gap 6.0e-12 mm) |
| 🟢 | EX13 | HollowBlock (OpenCascade) | 227.3 ms, 362 instances |
| 🟢 | EX11 | HollowBlock | MM file: solids 1 shells 2 F 7 E 13 V 10 (OpenCascade 1 2 7 13 10); ΔV/V 1.9e-13, ΔA/A 3.3e-13; 37 points, worst 4.9e-11 mm; seams added 1, degenerate edges skipped 0, converted surfaces 0, reoriented faces 0, tolerant edges 0 (largest gap 1.8e-15 mm) |
| 🟢 | EX13 | FilletedBoss (OpenCascade) | 224.4 ms, 494 instances |
| 🟢 | EX11 | FilletedBoss | MM file: solids 1 shells 1 F 9 E 17 V 11 (OpenCascade 1 1 9 17 11); ΔV/V 1.5e-12, ΔA/A 3.6e-13; 75 points, worst 6.6e-12 mm; seams added 0, degenerate edges skipped 0, converted surfaces 0, reoriented faces 1, tolerant edges 0 (largest gap 4.0e-15 mm) |
| 🟢 | EX13 | InchBlock (OpenCascade) | 250.8 ms, 353 instances |
| 🟢 | EX11 | InchBlock | INCH file: solids 1 shells 1 F 6 E 12 V 8 (OpenCascade 1 1 6 12 8); ΔV/V 2.7e-15, ΔA/A 2.6e-15; 24 points, worst 0.0e+00 mm; seams added 0, degenerate edges skipped 0, converted surfaces 0, reoriented faces 0, tolerant edges 0 (largest gap 0.0e+00 mm) |
| 🟢 | EX12 | empty input | STEP: not an ISO 10303-21 exchange structure (no ISO-10303-21 header); partition untouched |
| 🟢 | EX12 | no DATA section | STEP: no DATA section; partition untouched |
| 🟢 | EX12 | unterminated DATA | STEP: the DATA section is not closed by ENDSEC; partition untouched |
| 🟢 | EX12 | missing instance | STEP: reference to the missing instance #19; partition untouched |
| 🟢 | EX12 | unsupported surface | STEP: unsupported face surface OFFSET_SURFACE (#99); supported: PLANE, CYLINDRICAL / CONICAL / SPHERICAL / TOROIDAL_SURFACE, B-spline, SURFACE_OF_REVOLUTION, SURFACE_OF_LINEAR_EXTRUSION, RECTANGULAR_TRIMMED_SURFACE; partition untouched |
| 🟢 | EX12 | open shell | STEP: solid 1 (#168): non-manifold or open result (pave block 4 used 1 times); partition untouched |
| 🟢 | EX12 | no solid | STEP: the file holds no MANIFOLD_SOLID_BREP, BREP_WITH_VOIDS or SHELL_BASED_SURFACE_MODEL; partition untouched |
| 🟢 | EX12 | gram length unit | STEP: the length unit is not based on the metre; partition untouched |
| 🟢 | EX12 | zero model unit | STEP: the model unit must be a positive length; partition untouched |
| 🟢 | EX12 | missing file | STEP: cannot open /nonexistent/Frontier.step |
| 🟢 | EX12 | second of two solids open | STEP: solid 2 (#318): non-manifold or open result (pave block 4 used 1 times); partition untouched |
