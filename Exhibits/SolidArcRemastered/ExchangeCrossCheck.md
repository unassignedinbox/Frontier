# Exchange cross-check

## STL

🟢 17 STL files written by ExchangeValidation, read back by trimesh 5.1.1 (vertices merged by trimesh itself, positive outward volume, closure and winding by its own edge test). Exact volumes are the kernel's MeasureBodyMass values from ExchangeValidation.md; the facet volume sits inside them by at most sag · area (EX2).


| | Specimen | Facets | Vertices | Watertight | Winding | Euler | Components | V trimesh | V exact | ΔV / V |
|---|---|---|---|---|---|---|---|---|---|---|
| 🟢 | Block | 142 | 73 | True | True | 2 | 1 | 24 | 24 | 0.00e+00 |
| 🟢 | BossRootFillet | 1222 | 613 | True | True | 2 | 1 | 25.1632692 | 25.165744 | 9.83e-05 |
| 🟢 | BoxAllChamfer | 196 | 100 | True | True | 2 | 1 | 22.5420001 | 22.542 | 6.65e-09 |
| 🟢 | BoxAllFillet | 5616 | 2810 | True | True | 2 | 1 | 23.3137248 | 23.3408494 | 1.16e-03 |
| 🟢 | Cone | 1868 | 936 | True | True | 2 | 1 | 12.5192322 | 12.5663706 | 3.75e-03 |
| 🟢 | Cylinder | 1880 | 942 | True | True | 2 | 1 | 21.1586855 | 21.2057504 | 2.22e-03 |
| 🟢 | HoleRimFillet | 1884 | 942 | True | True | 0 | 1 | 21.7132012 | 21.7032826 | 4.57e-04 |
| 🟢 | HollowBlock | 214 | 111 | True | True | 4 | 2 | 114 | 114 | 0.00e+00 |
| 🟢 | LoftFrustum | 1308 | 656 | True | True | 2 | 1 | 8.22258985 | 8.24668072 | 2.92e-03 |
| 🟢 | MirroredCross | 3984 | 1994 | True | True | 2 | 1 | 3.97385897 | 4.00319568 | 7.33e-03 |
| 🟢 | PerforatedBlock | 150 | 75 | True | True | 0 | 1 | 42 | 42 | 0.00e+00 |
| 🟢 | PipeBend | 2044 | 1024 | True | True | 2 | 1 | 2.45538129 | 2.4674011 | 4.87e-03 |
| 🟢 | SmoothFillet | 1602 | 803 | True | True | 2 | 1 | 7.26934803 | 7.27935258 | 1.37e-03 |
| 🟢 | Sphere | 5172 | 2588 | True | True | 2 | 1 | 33.4045818 | 33.5103216 | 3.16e-03 |
| 🟢 | SweptRing | 2570 | 1285 | True | True | 0 | 1 | 10.103828 | 10.1159283 | 1.20e-03 |
| 🟢 | Torus | 3548 | 1774 | True | True | 0 | 1 | 58.762645 | 59.2176264 | 7.68e-03 |
| 🟢 | VariableFillet | 988 | 496 | True | True | 2 | 1 | 23.9372061 | 23.9400227 | 1.18e-04 |

## STEP

🟢 26 STEP files read by OpenCascade (cadquery-ocp, OCP): BRepCheck_Analyzer validity; unique solids, shells and faces against the B-rep; adaptive volume / area integration (ε 10⁻⁹) against the kernel's exact values; the largest distance from the sampled points of the exact model (tessellation vertices, which lie on the exact carriers) to the boundary OCCT built. OCCT counts the pole and apex degenerate edges it adds. Limits: ΔV, ΔA ≤ 5 × 10⁻⁵ (OCCT re-derives 2D trimming curves on B-spline faces by projection, ≈ 10⁻⁵ on promoted planar caps), distance ≤ 10⁻⁵.

| | Specimen | BRepCheck | Solids | Shells | Faces | Edges (OCCT) | Surface types read | V OCCT | ΔV / V | ΔA / A | Points | Max distance |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 🟢 | ApproximatedHoleRimFillet | valid | 1 | 1 | 8 / 8 | 17 | BSplineSurface 8 | 21.70324928 | 1.5e-06 | 1.8e-06 | 244 | 2.4e-15 |
| 🟢 | Block | valid | 1 | 1 | 6 / 6 | 12 | Plane 6 | 24 | 0.0e+00 | 0.0e+00 | 73 | 0.0e+00 |
| 🟢 | BossRootFillet | valid | 1 | 1 | 9 / 9 | 17 | Cylinder 1, Plane 7, Torus 1 | 25.16574403 | 2.0e-13 | 5.1e-13 | 307 | 6.8e-16 |
| 🟢 | BoxAllChamfer | valid | 1 | 1 | 18 / 18 | 48 | Plane 18 | 22.542 | 1.6e-16 | 3.3e-13 | 100 | 0.0e+00 |
| 🟢 | BoxAllFillet | valid | 1 | 1 | 26 / 26 | 48 | Cylinder 12, Plane 6, Sphere 8 | 23.34084943 | 2.2e-09 | 2.8e-09 | 256 | 2.2e-16 |
| 🟢 | Cone | valid | 1 | 1 | 2 / 2 | 3 | Cone 1, Plane 1 | 12.56637061 | 3.2e-12 | 1.1e-12 | 312 | 1.6e-15 |
| 🟢 | Cylinder | valid | 1 | 1 | 3 / 3 | 3 | Cylinder 1, Plane 2 | 21.20575041 | 1.5e-12 | 8.9e-13 | 314 | 1.3e-15 |
| 🟢 | HoleRimFillet | valid | 1 | 1 | 8 / 8 | 17 | Cylinder 1, Plane 6, Torus 1 | 21.70328261 | 5.8e-13 | 8.6e-13 | 314 | 5.4e-16 |
| 🟢 | HollowBlock | valid | 1 | 2 | 12 / 12 | 24 | Plane 12 | 114 | 0.0e+00 | 0.0e+00 | 111 | 0.0e+00 |
| 🟢 | LoftFrustum | valid | 1 | 1 | 3 / 3 | 3 | Cone 1, Plane 2 | 8.246680716 | 3.9e-13 | 1.6e-13 | 328 | 1.1e-15 |
| 🟢 | MirroredCross | valid | 1 | 1 | 7 / 7 | 9 | Cylinder 3, Plane 4 | 4.003195672 | 2.4e-09 | 3.0e-09 | 250 | 9.0e-08 |
| 🟢 | PerforatedBlock | valid | 1 | 1 | 10 / 10 | 24 | Plane 10 | 42 | 0.0e+00 | 0.0e+00 | 75 | 0.0e+00 |
| 🟢 | PipeBend | valid | 1 | 1 | 3 / 3 | 3 | Plane 2, Torus 1 | 2.4674011 | 9.6e-13 | 1.4e-12 | 256 | 5.4e-16 |
| 🟢 | PromotedBoxAllFillet | valid | 1 | 1 | 26 / 26 | 48 | BSplineSurface 26 | 23.34084882 | 2.4e-08 | 4.8e-10 | 249 | 1.6e-15 |
| 🟢 | PromotedCylinder | valid | 1 | 1 | 3 / 3 | 3 | BSplineSurface 3 | 21.20598144 | 1.1e-05 | 1.1e-05 | 261 | 1.2e-15 |
| 🟢 | PromotedSphere | valid | 1 | 1 | 1 / 1 | 3 | BSplineSurface 1 | 33.51032144 | 5.9e-09 | 5.9e-09 | 248 | 1.2e-15 |
| 🟢 | PromotedTorus | valid | 1 | 1 | 1 / 1 | 2 | BSplineSurface 1 | 59.21762606 | 5.9e-09 | 5.9e-09 | 255 | 2.0e-15 |
| 🟢 | SmoothFillet | valid | 1 | 1 | 7 / 7 | 15 | Cylinder 2, Plane 5 | 7.279352579 | 3.8e-13 | 5.1e-13 | 268 | 9.1e-16 |
| 🟢 | Sphere | valid | 1 | 1 | 1 / 1 | 3 | Sphere 1 | 33.51032164 | 2.6e-13 | 7.3e-13 | 259 | 1.8e-15 |
| 🟢 | SweptRing | valid | 1 | 1 | 4 / 4 | 6 | Cylinder 2, Plane 2 | 10.11592834 | 4.0e-12 | 3.8e-13 | 257 | 7.1e-16 |
| 🟢 | TiltedBoreChamfer | valid | 1 | 1 | 8 / 8 | 17 | BSplineSurface 1, Cylinder 1, Plane 6 | 15.14773571 | 2.5e-09 | 3.9e-09 | 430 | 1.6e-07 |
| 🟢 | TiltedBossChamfer | valid | 1 | 1 | 9 / 9 | 17 | BSplineSurface 1, Cylinder 1, Plane 7 | 16.80185915 | 1.8e-11 | 1.8e-09 | 370 | 1.5e-07 |
| 🟢 | Torus | valid | 1 | 1 | 1 / 1 | 2 | Torus 1 | 59.21762641 | 6.1e-13 | 6.1e-13 | 254 | 3.5e-15 |
| 🟢 | VariableArcChamfer | valid | 1 | 1 | 5 / 5 | 9 | BSplineSurface 1, Cylinder 1, Plane 3 | 1.507964383 | 6.0e-08 | 5.3e-08 | 387 | 1.7e-07 |
| 🟢 | VariableArcFillet | valid | 1 | 1 | 5 / 5 | 9 | BSplineSurface 1, Cylinder 1, Plane 3 | 1.543138003 | 6.1e-08 | 4.9e-08 | 275 | 2.9e-07 |
| 🟢 | VariableFillet | valid | 1 | 1 | 7 / 7 | 15 | Cone 1, Plane 6 | 23.94002268 | 3.5e-10 | 2.4e-10 | 248 | 3.7e-08 |
