# Exchange cross-check

🟢 17 STL files written by ExchangeValidation, read back by trimesh 5.1.1 (vertices merged by trimesh itself, positive outward volume, closure and winding by its own edge test). Exact volumes are the kernel's MeasureBodyMass values from ExchangeValidation.md; the facet volume sits inside them by at most sag · area (EX2).

## STL

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
