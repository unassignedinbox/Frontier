# Blend validation (step 5, I)

**🟢 PASS** · rolling-ball harness 15/15 · BevelLab 6/6 · 50 specimens · 30906 ms

## Gates

| | Gate | Measures | Limit | Worst | Checks | Failures |
|---|---|---|---|---|---|---|
| 🟢 | B1 | Volume change vs closed form | \|ΔV − ΔV*\| ≤ 10⁻⁹·max(1, V) | 1.95e-12 | 40 | 0 |
| 🟢 | B2 | Audit sound · genus kept · face count · source untouched | all hold | 0.00e+00 | 40 | 0 |
| 🟢 | B3 | Fillet G1 across contact / corner edges · chamfer setback | ≤ 10⁻¹⁰ rad · ≤ 10⁻⁹·L | 2.24e-14 | 2250 | 0 |
| 🟢 | B4 | Rolling-ball law: spine ↔ supports = r, blend face ↔ spine = r, corner centre ↔ 3 supports = r | ≤ 10⁻⁹·L | 5.59e-14 | 91047 | 0 |
| 🟢 | B5 | Tessellation N1 · N3 · N4 · N6 on every blended body | 0 · 0 · ≤ 1 · 0 | 3.02e-01 | 40 | 0 |
| 🟢 | B6 | Generalized winding number vs analytic clearance oracle | 0 wrong | 0.00e+00 | 10800 | 0 |
| 🟢 | B7 | Booleans downstream of blends (volume, audit) | rel. ≤ 10⁻⁷, sound | 1.72e-16 | 6 | 0 |
| 🟢 | B8 | Refusals: atomic, prefixed, source intact | all refused | 0.00e+00 | 10 | 0 |
| 🟢 | B9 | Same specimen twice → identical tessellation digest | equal | 0.00e+00 | 17 | 0 |
| 🟢 | B10 | Scale 10⁻³ … 10³: relative ΔV error | ≤ 10⁻⁹ | 1.17e-14 | 4 | 0 |

## Specimens

Carriers: P plane · C cylinder · K cone · T torus · R procedural rolling ball (arc or chord) · S corner sphere. Vertices: cap · mitre · ball corner · continuation.

| | Specimen | Family | Edges | Faces | Genus | ΔV | ΔV* (closed form) | \|error\| | Carriers | Vertices | Triangles | ms |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 🟢 | BoxEdgeFillet | Harness | 1 | 7 | 0 | -0.077256661177 | -0.077256661177 | 7.5e-16 | 0P 1C 0K 0T 0R 0S | 2 · 0 · 0 · 0 | 1028 | 0.8 |
| 🟢 | BoxEdgeChamfer | Harness | 1 | 7 | 0 | -0.180000000000 | -0.180000000000 | 7.4e-15 | 1P 0C 0K 0T 0R 0S | 2 · 0 · 0 · 0 | 150 | 0.4 |
| 🟢 | BoxTopFillet | Harness | 4 | 10 | 0 | -0.260044317413 | -0.260044317413 | 1.3e-15 | 0P 4C 0K 0T 0R 0S | 0 · 4 · 0 · 0 | 2530 | 4.0 |
| 🟢 | BoxTopChamfer | Harness | 4 | 10 | 0 | -0.594000000000 | -0.594000000000 | 5.9e-15 | 4P 0C 0K 0T 0R 0S | 0 · 4 · 0 · 0 | 150 | 0.6 |
| 🟢 | BoxCornerFillet | Harness | 3 | 10 | 0 | -0.169307571942 | -0.169307571942 | 1.7e-16 | 0P 3C 0K 0T 0R 1S | 3 · 0 · 1 · 0 | 1806 | 2.5 |
| 🟢 | BoxAllFillet | Harness | 12 | 26 | 0 | -0.659150624945 | -0.659150624945 | 1.6e-15 | 0P 12C 0K 0T 0R 8S | 0 · 0 · 8 · 0 | 5616 | 13.2 |
| 🟢 | BoxAllChamfer | Harness | 12 | 18 | 0 | -1.458000000000 | -1.458000000000 | 5.1e-15 | 12P 0C 0K 0T 0R 0S | 0 · 0 · 8 · 0 | 196 | 1.3 |
| 🟢 | CylinderRimFillet | Harness | 1 | 4 | 0 | -0.113222469600 | -0.113222469600 | 1.1e-15 | 0P 0C 0K 1T 0R 0S | 0 · 0 · 0 · 1 | 3016 | 0.9 |
| 🟢 | CylinderRimsChamfer | Harness | 2 | 5 | 0 | -0.508938009882 | -0.508938009882 | 3.3e-16 | 0P 0C 2K 0T 0R 0S | 0 · 0 · 0 · 2 | 1690 | 0.8 |
| 🟢 | BossRootFillet | Harness | 1 | 9 | 0 | 0.034770679003 | 0.034770679003 | 4.8e-16 | 0P 0C 0K 1T 0R 0S | 0 · 0 · 0 · 1 | 1222 | 1.1 |
| 🟢 | BossRimFillet | Harness | 1 | 9 | 0 | -0.029951710116 | -0.029951710116 | 7.6e-16 | 0P 0C 0K 1T 0R 0S | 0 · 0 · 0 · 1 | 1196 | 1.1 |
| 🟢 | HoleRimFillet | Harness | 1 | 8 | 1 | -0.034770679003 | -0.034770679003 | 3.1e-15 | 0P 0C 0K 1T 0R 0S | 0 · 0 · 0 · 1 | 1884 | 1.1 |
| 🟢 | FrustumRimFillet | Harness | 1 | 4 | 0 | -0.006788683186 | -0.006788683186 | 3.9e-16 | 0P 0C 0K 1T 0R 0S | 0 · 0 · 0 · 1 | 1848 | 0.9 |
| 🟢 | CylinderChamferOversize | Harness | 1 | – | – | refused | – | – | – | – | – | 0.0 |
| 🟢 | CylinderFilletOversize | Harness | 1 | – | – | refused | – | – | – | – | – | 0.0 |
| 🟢 | RoundedBlock | BevelLab | 12 | 26 | 0 | -0.143799448088 | -0.143799448088 | 1.4e-15 | 0P 12C 0K 0T 0R 8S | 0 · 0 · 8 · 0 | 5408 | 12.4 |
| 🟢 | ChamferBlock | BevelLab | 12 | 18 | 0 | -0.182250000000 | -0.182250000000 | 2.5e-16 | 12P 0C 0K 0T 0R 0S | 0 · 0 · 8 · 0 | 196 | 1.2 |
| 🟢 | BossPlate | BevelLab | 6 | 14 | 0 | -0.044958161148 | -0.044958161148 | 2.6e-16 | 0P 4C 0K 2T 0R 0S | 0 · 4 · 0 · 2 | 4160 | 5.2 |
| 🟢 | Puck | BevelLab | 2 | 5 | 0 | -0.108571473223 | -0.108571473223 | 3.2e-16 | 0P 0C 0K 2T 0R 0S | 0 · 0 · 0 · 2 | 3970 | 1.4 |
| 🟢 | HoleSlab | BevelLab | 5 | 12 | 1 | -0.039497979852 | -0.039497979852 | 2.2e-15 | 0P 4C 0K 1T 0R 0S | 0 · 4 · 0 · 1 | 3974 | 4.7 |
| 🟢 | Frustum | BevelLab | 2 | 5 | 0 | -0.053175862484 | -0.053175862484 | 1.1e-15 | 0P 0C 2K 0T 0R 0S | 0 · 0 · 0 · 2 | 1294 | 0.8 |
| 🟢 | PrismAcuteFillet | Extra | 1 | 6 | 0 | -0.054788260510 | -0.054788260510 | 3.5e-16 | 0P 1C 0K 0T 0R 0S | 2 · 0 · 0 · 0 | 674 | 0.6 |
| 🟢 | HexagonPrismChamfer | Extra | 1 | 9 | 0 | -0.025980762114 | -0.025980762114 | 5.6e-16 | 1P 0C 0K 0T 0R 0S | 2 · 0 · 0 · 0 | 134 | 0.4 |
| 🟢 | SkewPrismFillet | Extra | 1 | 7 | 0 | -0.077256661177 | -0.077256661177 | 6.4e-15 | 0P 1C 0K 0T 0R 0S | 2 · 0 · 0 · 0 | 890 | 1.2 |
| 🟢 | CliffBoxFillet | Overflow | 1 | 6 | 0 | -5.331509667726 | -5.331509667726 | 7.1e-15 | 0P 1C 0K 0T 0R 0S | 2 · 0 · 0 · 0 | 604 | 0.8 |
| 🟢 | CliffPlateFillet | Overflow | 1 | 6 | 0 | -0.074995847374 | -0.074995847374 | 6.7e-16 | 0P 1C 0K 0T 0R 0S | 2 · 0 · 0 · 0 | 872 | 0.8 |
| 🟢 | CliffPlateChamfer | Overflow | 1 | 6 | 0 | -0.160000000000 | -0.160000000000 | 8.3e-17 | 1P 0C 0K 0T 0R 0S | 2 · 0 · 0 · 0 | 84 | 0.4 |
| 🟢 | CliffPlateFlush | Overflow | 1 | 6 | 0 | -0.034336293856 | -0.034336293856 | 1.8e-16 | 0P 1C 0K 0T 0R 0S | 2 · 0 · 0 · 0 | 1000 | 0.8 |
| 🟢 | CliffDiscFillet | Overflow | 1 | 3 | 0 | -0.109689377437 | -0.109689377437 | 2.5e-16 | 0P 0C 0K 1T 0R 0S | 0 · 0 · 0 · 1 | 1624 | 0.7 |
| 🟢 | CliffDiscChamfer | Overflow | 1 | 3 | 0 | -0.224100275956 | -0.224100275956 | 5.6e-17 | 0P 0C 1K 0T 0R 0S | 0 · 0 · 0 · 1 | 612 | 0.4 |
| 🟢 | CliffPartial | Refusal | 1 | – | – | refused | – | – | – | – | – | 0.1 |
| 🟢 | CliffOverlap | Refusal | 2 | – | – | refused | – | – | – | – | – | 0.1 |
| 🟢 | NotchStepFillet | Overflow | 1 | 8 | 0 | -0.028123442765 | -0.028123442765 | 1.4e-16 | 0P 1C 0K 0T 0R 0S | 2 · 0 · 0 · 0 | 612 | 0.7 |
| 🟢 | NotchStepChamfer | Overflow | 1 | 8 | 0 | -0.060000000000 | -0.060000000000 | 5.0e-16 | 1P 0C 0K 0T 0R 0S | 2 · 0 · 0 · 0 | 170 | 0.5 |
| 🟢 | NotchBossFillet | Overflow | 1 | 8 | 0 | -0.086128737121 | -0.086128737121 | 3.7e-16 | 0P 0C 0K 1T 0R 0S | 0 · 0 · 0 · 1 | 994 | 0.9 |
| 🟢 | SmoothFillet | Overflow | 1 | 7 | 0 | -0.100448113101 | -0.100448113101 | 5.6e-16 | 0P 1C 0K 0T 0R 0S | 2 · 0 · 0 · 0 | 1602 | 1.1 |
| 🟢 | SmoothChamfer | Overflow | 1 | 7 | 0 | -0.259735796010 | -0.259735796010 | 5.6e-17 | 1P 0C 0K 0T 0R 0S | 2 · 0 · 0 · 0 | 692 | 0.8 |
| 🟢 | SmoothTight | Refusal | 1 | – | – | refused | – | – | – | – | – | 0.1 |
| 🟢 | VariableFillet | Variable | 1 | 7 | 0 | -0.059977332307 | -0.059977332307 | 7.1e-15 | 0P 0C 1K 0T 0R 0S | 2 · 0 · 0 · 0 | 988 | 6.6 |
| 🟢 | VariableChamfer | Variable | 1 | 7 | 0 | -0.140000000000 | -0.140000000000 | 6.6e-15 | 1P 0C 0K 0T 0R 0S | 2 · 0 · 0 · 0 | 154 | 0.4 |
| 🟢 | VariableShared | Refusal | 4 | – | – | refused | – | – | – | – | – | 0.1 |
| 🟢 | VariableAnchored | Variable | 1 | 7 | 0 | -0.059977332307 | -0.059977332307 | 3.5e-15 | 0P 0C 1K 0T 0R 0S | 2 · 0 · 0 · 0 | 958 | 6.3 |
| 🟢 | VariableUnanchored | Refusal | 1 | – | – | refused | – | – | – | – | – | 0.0 |
| 🟢 | TiltedBoreChamfer | CurvedChamfer | 1 | 8 | 1 | -0.016460987608 | -0.016460987612 | 4.3e-12 | 0P 0C 0K 0T 1R 0S | 0 · 0 · 0 · 1 | 860 | 2602.0 |
| 🟢 | TiltedBossChamfer | CurvedChamfer | 1 | 9 | 0 | 0.016460987608 | 0.016460987612 | 4.4e-12 | 0P 0C 0K 0T 1R 0S | 0 · 0 · 0 · 1 | 736 | 2481.7 |
| 🟢 | VariableRim | Refusal | 1 | – | – | refused | – | – | – | – | – | 0.0 |
| 🟢 | VariableArcFillet | Variable | 1 | 5 | 0 | -0.027658230631 | -0.027658230629 | 2.4e-12 | 0P 0C 0K 0T 1R 0S | 2 · 0 · 0 · 0 | 1644 | 337.2 |
| 🟢 | VariableArcChamfer | Variable | 1 | 5 | 0 | -0.062831853075 | -0.062831853072 | 3.1e-12 | 0P 0C 0K 0T 1R 0S | 2 · 0 · 0 · 0 | 770 | 138.1 |
| 🟢 | FilletRailsCross | Refusal | 12 | – | – | refused | – | – | – | – | – | 0.7 |
| 🟢 | MixedConvexity | Refusal | 2 | – | – | refused | – | – | – | – | – | 0.1 |

## Refusals

| | Specimen | Refusal |
|---|---|---|
| 🟢 | CylinderChamferOversize | Blend: E2: size exceeds the curvature radius of the Cylinder support |
| 🟢 | CylinderFilletOversize | Blend: E2: size exceeds the curvature radius of the Cylinder support |
| 🟢 | CliffPartial | Blend: edge E0: blend wider than its face at one end only (partial overflow, 🚧) |
| 🟢 | CliffOverlap | Blend: vertex V5: blend wider than its face: it runs onto another blended edge at the far corner (🚧) |
| 🟢 | SmoothTight | Blend: smooth overflow: E14: size exceeds the curvature radius of the Cylinder support |
| 🟢 | VariableShared | Blend: vertex V7: unequal mitre: outer contacts meet the sharp edge apart (setback, 🚧) |
| 🟢 | VariableUnanchored | Blend: E9: variable size with neither end of the edge anchored (which end takes the start size?) |
| 🟢 | VariableRim | Blend: E2: variable size on a closed edge (the size would jump at its only vertex) |
| 🟢 | FilletRailsCross | Blend: edge E4: contact rail runs backwards (blend wider than its face) |
| 🟢 | MixedConvexity | Blend: vertex V10: convex and concave blends meet beside a sharp edge (🚧) |

## Booleans downstream (B7)

| | Operation | V | V* | rel. error |
|---|---|---|---|---|
| 🟢 | RoundedBlock − drill (flat-to-flat) | 2.573457213089 | 2.573457213089 | 0.0e+00 |
| 🟢 | RoundedBlock ∩ half-space x ≤ 1 (cuts 4 cylinders, 0 spheres) | 1.428100275956 | 1.428100275956 | 1.6e-16 |
| 🟢 | CylinderRimFillet − coaxial bore r 0.4 | 5.164653188431 | 5.164653188431 | 1.7e-16 |
| 🟢 | BoxAllChamfer ∪ block on the top flat | 23.042000000000 | 23.042000000000 | 0.0e+00 |
| 🟢 | CliffPlateFillet − drill r 0.3 (through the kept flat) | 1.468455484862 | 1.468455484862 | 0.0e+00 |
| 🟢 | CliffDiscFillet − coaxial bore r 0.4 | 0.418098188366 | 0.418098188366 | 1.3e-16 |

## Scale (B10)

| | s | Rounded box rel. error | Rim fillet rel. error | Cliff plate rel. error |
|---|---|---|---|---|
| 🟢 | 1e-03 | 9.3e-15 | 1.1e-15 | 1.6e-15 |
| 🟢 | 1e-01 | 8.6e-15 | 5.3e-15 | 4.5e-15 |
| 🟢 | 1e+01 | 1.2e-14 | 3.4e-15 | 2.3e-15 |
| 🟢 | 1e+03 | 8.0e-15 | 1.0e-14 | 1.8e-15 |
