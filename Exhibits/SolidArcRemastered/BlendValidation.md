# Blend validation (step 5, I)

**🟢 PASS** · rolling-ball harness 15/15 · BevelLab 6/6 · 44 specimens · 91914 ms

## Gates

| | Gate | Measures | Limit | Worst | Checks | Failures |
|---|---|---|---|---|---|---|
| 🟢 | B1 | Volume change vs closed form | \|ΔV − ΔV*\| ≤ 10⁻⁹·max(1, V) | 7.40e-16 | 35 | 0 |
| 🟢 | B2 | Audit sound · genus kept · face count · source untouched | all hold | 0.00e+00 | 35 | 0 |
| 🟢 | B3 | Fillet G1 across contact / corner edges · chamfer setback | ≤ 10⁻¹⁰ rad · ≤ 10⁻⁹·L | 3.61e-15 | 2160 | 0 |
| 🟢 | B4 | Rolling-ball law: spine ↔ supports = r, blend face ↔ spine = r, corner centre ↔ 3 supports = r | ≤ 10⁻⁹·L | 1.64e-15 | 86679 | 0 |
| 🟢 | B5 | Tessellation N1 · N3 · N4 · N6 on every blended body | 0 · 0 · ≤ 1 · 0 | 3.02e-01 | 35 | 0 |
| 🟢 | B6 | Generalized winding number vs analytic clearance oracle | 0 wrong | 0.00e+00 | 10200 | 0 |
| 🟢 | B7 | Booleans downstream of blends (volume, audit) | rel. ≤ 10⁻⁷, sound | 3.98e-16 | 6 | 0 |
| 🟢 | B8 | Refusals: atomic, prefixed, source intact | all refused | 0.00e+00 | 9 | 0 |
| 🟢 | B9 | Same specimen twice → identical tessellation digest | equal | 0.00e+00 | 17 | 0 |
| 🟢 | B10 | Scale 10⁻³ … 10³: relative ΔV error | ≤ 10⁻⁹ | 7.69e-14 | 4 | 0 |

## Specimens

Carriers: P plane · C cylinder · K cone · T torus · S corner sphere. Vertices: cap · mitre · ball corner · continuation.

| | Specimen | Family | Edges | Faces | Genus | ΔV | ΔV* (closed form) | \|error\| | Carriers | Vertices | Triangles | ms |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 🟢 | BoxEdgeFillet | Harness | 1 | 7 | 0 | -0.077256661177 | -0.077256661177 | 2.8e-15 | 0P 1C 0K 0T 0S | 2 · 0 · 0 · 0 | 1028 | 61.3 |
| 🟢 | BoxEdgeChamfer | Harness | 1 | 7 | 0 | -0.180000000000 | -0.180000000000 | 6.8e-15 | 1P 0C 0K 0T 0S | 2 · 0 · 0 · 0 | 150 | 39.8 |
| 🟢 | BoxTopFillet | Harness | 4 | 10 | 0 | -0.260044317413 | -0.260044317413 | 4.9e-15 | 0P 4C 0K 0T 0S | 0 · 4 · 0 · 0 | 2530 | 118.7 |
| 🟢 | BoxTopChamfer | Harness | 4 | 10 | 0 | -0.594000000000 | -0.594000000000 | 8.3e-15 | 4P 0C 0K 0T 0S | 0 · 4 · 0 · 0 | 150 | 42.2 |
| 🟢 | BoxCornerFillet | Harness | 3 | 10 | 0 | -0.169307571942 | -0.169307571942 | 3.4e-15 | 0P 3C 0K 0T 1S | 3 · 0 · 1 · 0 | 1806 | 105.4 |
| 🟢 | BoxAllFillet | Harness | 12 | 26 | 0 | -0.659150624945 | -0.659150624945 | 2.0e-15 | 0P 12C 0K 0T 8S | 0 · 0 · 8 · 0 | 5616 | 252.5 |
| 🟢 | BoxAllChamfer | Harness | 12 | 18 | 0 | -1.458000000000 | -1.458000000000 | 1.6e-14 | 12P 0C 0K 0T 0S | 0 · 0 · 8 · 0 | 196 | 48.6 |
| 🟢 | CylinderRimFillet | Harness | 1 | 4 | 0 | -0.113222469600 | -0.113222469600 | 1.7e-16 | 0P 0C 0K 1T 0S | 0 · 0 · 0 · 1 | 3016 | 22.9 |
| 🟢 | CylinderRimsChamfer | Harness | 2 | 5 | 0 | -0.508938009882 | -0.508938009882 | 3.3e-16 | 0P 0C 2K 0T 0S | 0 · 0 · 0 · 2 | 1690 | 18.1 |
| 🟢 | BossRootFillet | Harness | 1 | 9 | 0 | 0.034770679003 | 0.034770679003 | 3.1e-15 | 0P 0C 0K 1T 0S | 0 · 0 · 0 · 1 | 1222 | 64.4 |
| 🟢 | BossRimFillet | Harness | 1 | 9 | 0 | -0.029951710116 | -0.029951710116 | 2.8e-15 | 0P 0C 0K 1T 0S | 0 · 0 · 0 · 1 | 1196 | 61.6 |
| 🟢 | HoleRimFillet | Harness | 1 | 8 | 1 | -0.034770679003 | -0.034770679003 | 4.8e-16 | 0P 0C 0K 1T 0S | 0 · 0 · 0 · 1 | 1884 | 74.2 |
| 🟢 | FrustumRimFillet | Harness | 1 | 4 | 0 | -0.006788683186 | -0.006788683186 | 1.7e-15 | 0P 0C 0K 1T 0S | 0 · 0 · 0 · 1 | 1848 | 21.4 |
| 🟢 | CylinderChamferOversize | Harness | 1 | – | – | refused | – | – | – | – | – | 0.0 |
| 🟢 | CylinderFilletOversize | Harness | 1 | – | – | refused | – | – | – | – | – | 0.0 |
| 🟢 | RoundedBlock | BevelLab | 12 | 26 | 0 | -0.143799448088 | -0.143799448088 | 1.8e-15 | 0P 12C 0K 0T 8S | 0 · 0 · 8 · 0 | 5408 | 170.4 |
| 🟢 | ChamferBlock | BevelLab | 12 | 18 | 0 | -0.182250000000 | -0.182250000000 | 2.5e-16 | 12P 0C 0K 0T 0S | 0 · 0 · 8 · 0 | 196 | 18.6 |
| 🟢 | BossPlate | BevelLab | 6 | 14 | 0 | -0.044958161148 | -0.044958161148 | 2.9e-15 | 0P 4C 0K 2T 0S | 0 · 4 · 0 · 2 | 4160 | 99.2 |
| 🟢 | Puck | BevelLab | 2 | 5 | 0 | -0.108571473223 | -0.108571473223 | 5.4e-16 | 0P 0C 0K 2T 0S | 0 · 0 · 0 · 2 | 3970 | 25.9 |
| 🟢 | HoleSlab | BevelLab | 5 | 12 | 1 | -0.039497979852 | -0.039497979852 | 8.6e-16 | 0P 4C 0K 1T 0S | 0 · 4 · 0 · 1 | 3974 | 97.3 |
| 🟢 | Frustum | BevelLab | 2 | 5 | 0 | -0.053175862484 | -0.053175862484 | 6.7e-16 | 0P 0C 2K 0T 0S | 0 · 0 · 0 · 2 | 1294 | 12.6 |
| 🟢 | PrismAcuteFillet | Extra | 1 | 6 | 0 | -0.054788260510 | -0.054788260510 | 5.4e-16 | 0P 1C 0K 0T 0S | 2 · 0 · 0 · 0 | 674 | 35.8 |
| 🟢 | HexagonPrismChamfer | Extra | 1 | 9 | 0 | -0.025980762114 | -0.025980762114 | 5.6e-16 | 1P 0C 0K 0T 0S | 2 · 0 · 0 · 0 | 134 | 22.3 |
| 🟢 | SkewPrismFillet | Extra | 1 | 7 | 0 | -0.077256661177 | -0.077256661177 | 2.8e-15 | 0P 1C 0K 0T 0S | 2 · 0 · 0 · 0 | 890 | 63.5 |
| 🟢 | CliffBoxFillet | Overflow | 1 | 6 | 0 | -5.331509667726 | -5.331509667726 | 1.8e-14 | 0P 1C 0K 0T 0S | 2 · 0 · 0 · 0 | 604 | 40.8 |
| 🟢 | CliffPlateFillet | Overflow | 1 | 6 | 0 | -0.074995847374 | -0.074995847374 | 6.7e-16 | 0P 1C 0K 0T 0S | 2 · 0 · 0 · 0 | 872 | 28.5 |
| 🟢 | CliffPlateChamfer | Overflow | 1 | 6 | 0 | -0.160000000000 | -0.160000000000 | 8.3e-17 | 1P 0C 0K 0T 0S | 2 · 0 · 0 · 0 | 84 | 19.1 |
| 🟢 | CliffPlateFlush | Overflow | 1 | 6 | 0 | -0.034336293856 | -0.034336293856 | 4.9e-16 | 0P 1C 0K 0T 0S | 2 · 0 · 0 · 0 | 1000 | 31.3 |
| 🟢 | CliffDiscFillet | Overflow | 1 | 3 | 0 | -0.109689377437 | -0.109689377437 | 1.9e-16 | 0P 0C 0K 1T 0S | 0 · 0 · 0 · 1 | 1624 | 13.3 |
| 🟢 | CliffDiscChamfer | Overflow | 1 | 3 | 0 | -0.224100275956 | -0.224100275956 | 5.6e-17 | 0P 0C 1K 0T 0S | 0 · 0 · 0 · 1 | 612 | 7.6 |
| 🟢 | CliffPartial | Refusal | 1 | – | – | refused | – | – | – | – | – | 0.1 |
| 🟢 | CliffOverlap | Refusal | 2 | – | – | refused | – | – | – | – | – | 0.1 |
| 🟢 | NotchStepFillet | Overflow | 1 | 8 | 0 | -0.028123442765 | -0.028123442765 | 3.1e-16 | 0P 1C 0K 0T 0S | 2 · 0 · 0 · 0 | 612 | 19.2 |
| 🟢 | NotchStepChamfer | Overflow | 1 | 8 | 0 | -0.060000000000 | -0.060000000000 | 5.6e-17 | 1P 0C 0K 0T 0S | 2 · 0 · 0 · 0 | 170 | 13.3 |
| 🟢 | NotchBossFillet | Overflow | 1 | 8 | 0 | -0.086128737121 | -0.086128737121 | 2.2e-15 | 0P 0C 0K 1T 0S | 0 · 0 · 0 · 1 | 994 | 36.3 |
| 🟢 | SmoothFillet | Overflow | 1 | 7 | 0 | -0.100448113101 | -0.100448113101 | 3.0e-15 | 0P 1C 0K 0T 0S | 2 · 0 · 0 · 0 | 1602 | 60.7 |
| 🟢 | SmoothChamfer | Overflow | 1 | 7 | 0 | -0.259735796010 | -0.259735796010 | 3.6e-15 | 1P 0C 0K 0T 0S | 2 · 0 · 0 · 0 | 692 | 44.8 |
| 🟢 | SmoothTight | Refusal | 1 | – | – | refused | – | – | – | – | – | 0.1 |
| 🟢 | VariableFillet | Variable | 1 | 7 | 0 | -0.059977332307 | -0.059977332307 | 2.1e-17 | 0P 0C 1K 0T 0S | 2 · 0 · 0 · 0 | 988 | 66.3 |
| 🟢 | VariableChamfer | Variable | 1 | 7 | 0 | -0.140000000000 | -0.140000000000 | 5.3e-16 | 1P 0C 0K 0T 0S | 2 · 0 · 0 · 0 | 154 | 51.7 |
| 🟢 | VariableShared | Refusal | 4 | – | – | refused | – | – | – | – | – | 0.1 |
| 🟢 | VariableRim | Refusal | 1 | – | – | refused | – | – | – | – | – | 0.0 |
| 🟢 | FilletRailsCross | Refusal | 12 | – | – | refused | – | – | – | – | – | 0.6 |
| 🟢 | MixedConvexity | Refusal | 2 | – | – | refused | – | – | – | – | – | 0.1 |

## Refusals

| | Specimen | Refusal |
|---|---|---|
| 🟢 | CylinderChamferOversize | Blend: E2: size exceeds the curvature radius of the Cylinder support |
| 🟢 | CylinderFilletOversize | Blend: E2: size exceeds the curvature radius of the Cylinder support |
| 🟢 | CliffPartial | Blend: edge E0: blend wider than its face at one end only (partial overflow, 🚧) |
| 🟢 | CliffOverlap | Blend: vertex V5: blend wider than its face: it runs onto another blended edge at the far corner (🚧) |
| 🟢 | SmoothTight | Blend: smooth overflow: E14: size exceeds the curvature radius of the Cylinder support |
| 🟢 | VariableShared | Blend: vertex V7: variable size where blends meet or at a vertex of degree ≠ 3 (🚧) |
| 🟢 | VariableRim | Blend: E2: variable size on a curved edge or a curved support (🚧) |
| 🟢 | FilletRailsCross | Blend: edge E4: contact rail runs backwards (blend wider than its face) |
| 🟢 | MixedConvexity | Blend: vertex V10: convex and concave blends meet beside a sharp edge (🚧) |

## Booleans downstream (B7)

| | Operation | V | V* | rel. error |
|---|---|---|---|---|
| 🟢 | RoundedBlock − drill (flat-to-flat) | 2.573457213089 | 2.573457213089 | 1.7e-16 |
| 🟢 | RoundedBlock ∩ half-space x ≤ 1 (cuts 4 cylinders, 0 spheres) | 1.428100275956 | 1.428100275956 | 3.1e-16 |
| 🟢 | CylinderRimFillet − coaxial bore r 0.4 | 5.164653188431 | 5.164653188431 | 3.4e-16 |
| 🟢 | BoxAllChamfer ∪ block on the top flat | 23.042000000000 | 23.042000000000 | 0.0e+00 |
| 🟢 | CliffPlateFillet − drill r 0.3 (through the kept flat) | 1.468455484862 | 1.468455484862 | 1.5e-16 |
| 🟢 | CliffDiscFillet − coaxial bore r 0.4 | 0.418098188366 | 0.418098188366 | 4.0e-16 |

## Scale (B10)

| | s | Rounded box rel. error | Rim fillet rel. error | Cliff plate rel. error |
|---|---|---|---|---|
| 🟢 | 1e-03 | 9.3e-15 | 1.3e-14 | 1.2e-14 |
| 🟢 | 1e-01 | 1.4e-14 | 5.3e-15 | 4.5e-15 |
| 🟢 | 1e+01 | 5.6e-14 | 7.7e-14 | 1.4e-14 |
| 🟢 | 1e+03 | 2.2e-15 | 1.5e-14 | 1.1e-14 |
