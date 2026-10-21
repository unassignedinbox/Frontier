# Feature validation (step 7, L + feature tree)

**🟢 PASS** · regenerations 41/41 · survivors 1254: index renumbered 112, name changed 18 · references: Exact 42 Geometric 2 Refused 3 · 8370 ms

## Gates

| | Gate | Measures | Limit | Worst | Checks | Failures |
|---|---|---|---|---|---|---|
| 🟢 | F1 | Regenerated volume vs closed form, every sweep point | \|V − V*\| ≤ 10⁻⁹·V* | 2.07e-15 | 41 | 0 |
| 🟢 | F2 | Refused-step set equals the oracle; final body audit-sound | equal · sound | 0.00e+00 | 41 | 0 |
| 🟢 | F3 | Every face / edge / vertex named, names unique per kind | 0 missing · 0 duplicate | 0.00e+00 | 297 | 0 |
| 🟢 | F4 | Geometric survivors of every step: faces keep the exact name; every survivor resolves to itself | 0 wrong | 0.00e+00 | 58 | 0 |
| 🟢 | F5 | Refused step keeps the previous body; later steps still run | all hold | 0.00e+00 | 3 | 0 |
| 🟢 | F6 | Expected resolution methods (exact · geometric rebind · ambiguous refusal · sketch refusal) | all as expected | 0.00e+00 | 11 | 0 |
| 🟢 | F7 | Same tree twice in fresh partitions: identical names and volume bits | equal | 0.00e+00 | 8 | 0 |
| 🟢 | F8 | Variable blend sizes stay at their anchor vertices: expected cap vertices present | ≤ 10⁻⁹ | 3.73e-16 | 24 | 0 |

## Regenerations

| | Tree | Parameters | F / E / V | V | V* | rel. error | References | Refused steps | ms |
|---|---|---|---|---|---|---|---|---|---|
| 🟢 | BlockFillet | authoring | 7 / 15 / 10 | 5.9597621556 | 5.9597621556 | 4.5e-16 | Exact | – | 70.3 |
| 🟢 | BlockFillet | Width = 4.5 | 7 / 15 / 10 | 8.9396432335 | 8.9396432335 | 6.0e-16 | Exact | – | 81.4 |
| 🟢 | BlockFillet | Radius = 0.4, Width = 6 | 7 / 15 / 10 | 11.7939822369 | 11.7939822369 | 6.0e-16 | Exact | – | 96.8 |
| 🟢 | BlockFillet | Depth = 1.2, Height = 0.6, Radius = 0.1 | 7 / 15 / 10 | 2.1535619449 | 2.1535619449 | 2.1e-16 | Exact | – | 46.3 |
| 🟢 | BlockFillet | Depth = 3, Height = 2, Radius = 0.45, Width = 1.5 | 7 / 15 / 10 | 8.9348146921 | 8.9348146921 | 0.0e+00 | Exact | – | 62.3 |
| 🟢 | BlockVariableFillet | authoring | 7 / 15 / 10 | 5.9721425723 | 5.9721425723 | 1.5e-16 | Exact, Exact | – | 41.9 |
| 🟢 | BlockVariableFillet | Width = 4.5 | 7 / 15 / 10 | 8.9581798838 | 8.9581798838 | 5.9e-16 | Exact, Exact | – | 86.6 |
| 🟢 | BlockVariableFillet | EndRadius = 0.1, StartRadius = 0.4 | 7 / 15 / 10 | 5.9550815248 | 5.9550815248 | 1.5e-16 | Exact, Exact | – | 75.3 |
| 🟢 | BlockVariableFillet | EndRadius = 0.2, StartRadius = 0.2 | 7 / 15 / 10 | 5.9742477796 | 5.9742477796 | 4.5e-16 | Exact, Exact | – | 39.1 |
| 🟢 | BlockVariableFillet | Height = -1 | 7 / 15 / 10 | 5.9721425723 | 5.9721425723 | 3.0e-16 | Exact, Exact | – | 63.7 |
| 🟢 | BlockVariableFillet | Depth = 3, EndRadius = 0.05, Height = 2, StartRadius = 0.45, Width = 1.5 | 7 / 15 / 10 | 8.9761415980 | 8.9761415980 | 2.0e-16 | Exact, Exact | – | 84.4 |
| 🟢 | BossFillet | authoring | 9 / 17 / 11 | 3.6445043419 | 3.6445043419 | 1.2e-16 | Exact | – | 151.3 |
| 🟢 | BossFillet | BossRadius = 0.7 | 9 / 17 / 11 | 4.2537578554 | 4.2537578554 | 0.0e+00 | Exact | – | 147.4 |
| 🟢 | BossFillet | BossHeight = 1.5, BossX = 1 | 9 / 17 / 11 | 4.1942830563 | 4.1942830563 | 2.1e-16 | Exact | – | 145.0 |
| 🟢 | BossFillet | Radius = 0.25, Thickness = 1 | 9 / 17 / 11 | 6.6751615271 | 6.6751615271 | 1.3e-16 | Exact | – | 134.7 |
| 🟢 | BossFillet | BossX = 2.2, Radius = 0.05, Width = 5 | 9 / 17 / 11 | 5.6300416578 | 5.6300416578 | 3.2e-16 | Exact | – | 185.7 |
| 🟢 | SteppedShaft | authoring | 6 / 8 / 5 | 6.0517546484 | 6.0517546484 | 1.5e-16 | Exact | – | 62.2 |
| 🟢 | SteppedShaft | PinRadius = 0.4 | 6 / 8 / 5 | 5.3040555968 | 5.3040555968 | 5.0e-16 | Exact | – | 70.3 |
| 🟢 | SteppedShaft | MainLength = 2.5, PinLength = 0.5 | 6 / 8 / 5 | 8.4016659533 | 8.4016659533 | 2.1e-16 | Exact | – | 56.6 |
| 🟢 | SteppedShaft | Chamfer = 0.2, MainRadius = 1.4 | 6 / 8 / 5 | 10.5264297846 | 10.5264297846 | 5.1e-16 | Exact | – | 57.8 |
| 🟢 | SteppedShaft | Chamfer = 0.03, MainRadius = 0.7, PinRadius = 0.65 | 6 / 8 / 5 | 3.9000485184 | 3.9000485184 | 5.7e-16 | Exact | – | 46.5 |
| 🟢 | SlotPocket | authoring | 11 / 24 / 16 | 5.5589380702 | 5.5589380702 | 3.2e-16 | Exact | – | 236.5 |
| 🟢 | SlotPocket | SlotLength = 0.5 | 11 / 24 / 16 | 6.0389380702 | 6.0389380702 | 2.9e-16 | Exact | – | 201.8 |
| 🟢 | SlotPocket | Push = -0.1, SlotRadius = 0.2 | 11 / 24 / 16 | 5.8446017763 | 5.8446017763 | 3.0e-16 | Exact | – | 242.2 |
| 🟢 | SlotPocket | PocketDepth = 0.2, Push = 0.15 | 11 / 24 / 16 | 6.2948672588 | 6.2948672588 | 4.2e-16 | Exact | – | 261.2 |
| 🟢 | SlotPocket | SlotLength = 2.5, SlotX = 1.5, Width = 6 | 11 / 24 / 16 | 8.5989380702 | 8.5989380702 | 2.1e-15 | Exact | – | 340.4 |
| 🟢 | TorusRing | authoring | 1 / 2 / 1 | 9.8696044011 | 9.8696044011 | 5.4e-16 | – | – | 27.1 |
| 🟢 | TorusRing | Major = 0.8, Minor = 0.7 | 1 / 2 / 1 | 7.7377698505 | 7.7377698505 | 9.2e-16 | – | – | 19.1 |
| 🟢 | TorusRing | Major = 3, Minor = 0.2 | 1 / 2 / 1 | 2.3687050563 | 2.3687050563 | 1.3e-15 | – | – | 27.2 |
| 🟢 | TorusRing | Turn = 1.5708 | 3 / 3 / 2 | 2.4674011003 | 2.4674011003 | 7.2e-16 | – | – | 15.2 |
| 🟢 | TorusRing | Major = 0.8, Turn = 0.3 | 3 / 3 / 2 | 0.1884955592 | 0.1884955592 | 8.8e-16 | – | – | 3.0 |
| 🟢 | HoleLeavesPlate | authoring | 8 / 17 / 11 | 3.4431121281 | 3.4431121281 | 7.7e-16 | Exact, Exact | – | 199.3 |
| 🟢 | HoleLeavesPlate | HoleRadius = 0.55, HoleX = 0.8 | 8 / 17 / 11 | 3.2514749762 | 3.2514749762 | 6.8e-16 | Exact, Exact | – | 168.3 |
| 🟢 | HoleLeavesPlate | Chamfer = 0.15, Grow = -0.2 | 8 / 17 / 11 | 2.9429535752 | 2.9429535752 | 0.0e+00 | Exact, Exact | – | 197.5 |
| 🟢 | HoleLeavesPlate | HoleX = 5 | 6 / 12 / 8 | 3.8400000000 | 3.8400000000 | 5.8e-16 | Refused, Exact | Blend#5: Blend: reference Edge(Extrude#2.Cap(End)|Extrude#4.Side(Circle1)) not found: nothing with its sources or its shape remains | 85.3 |
| 🟢 | HoleLeavesPlate | Grow = 0.4, HoleX = 5 | 6 / 12 / 8 | 4.0800000000 | 4.0800000000 | 4.4e-16 | Refused, Exact | Blend#5: Blend: reference Edge(Extrude#2.Cap(End)|Extrude#4.Side(Circle1)) not found: nothing with its sources or its shape remains | 85.1 |
| 🟢 | SplitFacePush | authoring | 6 / 12 / 8 | 7.2000000000 | 7.2000000000 | 0.0e+00 | Exact | – | 89.6 |
| 🟢 | SplitFacePush | Push = -0.2 | 6 / 12 / 8 | 4.8000000000 | 4.8000000000 | 1.9e-16 | Exact | – | 85.0 |
| 🟢 | SplitFacePush | GrooveX = 0.5 | 10 / 24 / 16 | 6.6000000000 | 6.6000000000 | 1.3e-16 | Geometric | – | 180.8 |
| 🟢 | SplitFacePush | GrooveX = 1.5 | 10 / 24 / 16 | 5.6800000000 | 5.6800000000 | 0.0e+00 | Refused | FaceOperation#5: FaceOperation: reference Extrude#2.Cap(End) is ambiguous: 2 equally plausible candidates (Extrude#2.Cap(End)/1, Extrude#2.Cap(End)/2) | 138.6 |
| 🟢 | SplitFacePush | GrooveX = 2.5, Push = 0.4 | 10 / 24 / 16 | 7.5200000000 | 7.5200000000 | 3.5e-16 | Geometric | – | 202.1 |

## Oracles

| Tree | Closed form | Steps |
|---|---|---|
| BlockFillet | W·D·H − (1 − π/4)·r²·W | Sketch#1 → Extrude#2 → Blend#3 |
| BlockVariableFillet | W·D·|H| − ΔV(cone; W, r₀, r₁), cap rails r₀ / r₁ from their corners | Sketch#1 → Extrude#2 → Blend#3 |
| BossFillet | W·D·T + π·R²·H + 2π(R + x̄)·r²(1 − π/4), x̄ = r(10 − 3π)/(3(4 − π)) | Sketch#1 → Extrude#2 → Sketch#3 → Extrude#4 → Blend#5 |
| SteppedShaft | π·R₁²·L₁ + π·R₂²·L₂ − π·c²(R₂ − c/3) | Sketch#1 → Revolve#2 → Blend#3 |
| SlotPocket | W·D·T − (2rL + πr²)(depth − push) | Sketch#1 → Extrude#2 → Sketch#3 → Extrude#4 → FaceOperation#5 |
| TorusRing | Φ·π·R·r² (Pappus; 2π²·R·r² at a full turn) | Sketch#1 → Revolve#2 |
| HoleLeavesPlate | ((W + g)·D − π·r²)·T − π·c²(r + c/3); hole off the plate: (W + g)·D·T with Blend#5 refused | Sketch#1 → Extrude#2 → Sketch#3 → Extrude#4 → Blend#5 → FaceOperation#6 |
| SplitFacePush | groove misses: W·D·(H + p); off-centre: W·D·H − 2h·D·s + p·max(piece)·D; centred: push refused | Sketch#1 → Extrude#2 → Sketch#3 → Extrude#4 → FaceOperation#5 |

## Survivor naming table (F4)

A survivor is found geometrically (same category, position, size, end points), never by name. *Renumbered* = its inventory index moved (what a raw-index reference would break on); *renamed* = its persistent name changed.

| | Tree · parameters | Step | Faces | Edges | Vertices | Renumbered | Renamed | Exact | Source | Geometric | Wrong |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 🟢 | BlockFillet · authoring | Blend#3 | 2 | 7 | 6 | 6 | 0 | 15 | 0 | 0 | 0 |
| 🟢 | BlockFillet · Width = 4.5 | Blend#3 | 2 | 7 | 6 | 6 | 0 | 15 | 0 | 0 | 0 |
| 🟢 | BlockFillet · Radius = 0.4, Width = 6 | Blend#3 | 2 | 7 | 6 | 6 | 0 | 15 | 0 | 0 | 0 |
| 🟢 | BlockFillet · Depth = 1.2, Height = 0.6, Radius = 0.1 | Blend#3 | 2 | 7 | 6 | 6 | 0 | 15 | 0 | 0 | 0 |
| 🟢 | BlockFillet · Depth = 3, Height = 2, Radius = 0.45, Width = 1.5 | Blend#3 | 2 | 7 | 6 | 6 | 0 | 15 | 0 | 0 | 0 |
| 🟢 | BlockVariableFillet · authoring | Blend#3 | 2 | 7 | 6 | 6 | 0 | 15 | 0 | 0 | 0 |
| 🟢 | BlockVariableFillet · Width = 4.5 | Blend#3 | 2 | 7 | 6 | 6 | 0 | 15 | 0 | 0 | 0 |
| 🟢 | BlockVariableFillet · EndRadius = 0.1, StartRadius = 0.4 | Blend#3 | 2 | 7 | 6 | 6 | 0 | 15 | 0 | 0 | 0 |
| 🟢 | BlockVariableFillet · EndRadius = 0.2, StartRadius = 0.2 | Blend#3 | 2 | 7 | 6 | 6 | 0 | 15 | 0 | 0 | 0 |
| 🟢 | BlockVariableFillet · Height = -1 | Blend#3 | 2 | 7 | 6 | 8 | 0 | 15 | 0 | 0 | 0 |
| 🟢 | BlockVariableFillet · Depth = 3, EndRadius = 0.05, Height = 2, StartRadius = 0.45, Width = 1.5 | Blend#3 | 2 | 7 | 6 | 6 | 0 | 15 | 0 | 0 | 0 |
| 🟢 | BossFillet · authoring | Extrude#4 | 5 | 12 | 8 | 0 | 0 | 25 | 0 | 0 | 0 |
| 🟢 | BossFillet · authoring | Blend#5 | 6 | 13 | 9 | 1 | 0 | 28 | 0 | 0 | 0 |
| 🟢 | BossFillet · BossRadius = 0.7 | Extrude#4 | 5 | 12 | 8 | 0 | 0 | 25 | 0 | 0 | 0 |
| 🟢 | BossFillet · BossRadius = 0.7 | Blend#5 | 6 | 13 | 9 | 1 | 0 | 28 | 0 | 0 | 0 |
| 🟢 | BossFillet · BossHeight = 1.5, BossX = 1 | Extrude#4 | 5 | 12 | 8 | 0 | 0 | 25 | 0 | 0 | 0 |
| 🟢 | BossFillet · BossHeight = 1.5, BossX = 1 | Blend#5 | 6 | 13 | 9 | 1 | 0 | 28 | 0 | 0 | 0 |
| 🟢 | BossFillet · Radius = 0.25, Thickness = 1 | Extrude#4 | 5 | 12 | 8 | 0 | 0 | 25 | 0 | 0 | 0 |
| 🟢 | BossFillet · Radius = 0.25, Thickness = 1 | Blend#5 | 6 | 13 | 9 | 1 | 0 | 28 | 0 | 0 | 0 |
| 🟢 | BossFillet · BossX = 2.2, Radius = 0.05, Width = 5 | Extrude#4 | 5 | 12 | 8 | 0 | 0 | 25 | 0 | 0 | 0 |
| 🟢 | BossFillet · BossX = 2.2, Radius = 0.05, Width = 5 | Blend#5 | 6 | 13 | 9 | 1 | 0 | 28 | 0 | 0 | 0 |
| 🟢 | SteppedShaft · authoring | Blend#3 | 3 | 4 | 3 | 0 | 0 | 10 | 0 | 0 | 0 |
| 🟢 | SteppedShaft · PinRadius = 0.4 | Blend#3 | 3 | 4 | 3 | 0 | 0 | 10 | 0 | 0 | 0 |
| 🟢 | SteppedShaft · MainLength = 2.5, PinLength = 0.5 | Blend#3 | 3 | 4 | 3 | 0 | 0 | 10 | 0 | 0 | 0 |
| 🟢 | SteppedShaft · Chamfer = 0.2, MainRadius = 1.4 | Blend#3 | 3 | 4 | 3 | 0 | 0 | 10 | 0 | 0 | 0 |
| 🟢 | SteppedShaft · Chamfer = 0.03, MainRadius = 0.7, PinRadius = 0.65 | Blend#3 | 3 | 4 | 3 | 0 | 0 | 10 | 0 | 0 | 0 |
| 🟢 | SlotPocket · authoring | Extrude#4 | 5 | 12 | 8 | 0 | 0 | 25 | 0 | 0 | 0 |
| 🟢 | SlotPocket · authoring | FaceOperation#5 | 6 | 16 | 12 | 0 | 0 | 34 | 0 | 0 | 0 |
| 🟢 | SlotPocket · SlotLength = 0.5 | Extrude#4 | 5 | 12 | 8 | 0 | 0 | 25 | 0 | 0 | 0 |
| 🟢 | SlotPocket · SlotLength = 0.5 | FaceOperation#5 | 6 | 16 | 12 | 0 | 0 | 34 | 0 | 0 | 0 |
| 🟢 | SlotPocket · Push = -0.1, SlotRadius = 0.2 | Extrude#4 | 5 | 12 | 8 | 0 | 0 | 25 | 0 | 0 | 0 |
| 🟢 | SlotPocket · Push = -0.1, SlotRadius = 0.2 | FaceOperation#5 | 6 | 16 | 12 | 0 | 0 | 34 | 0 | 0 | 0 |
| 🟢 | SlotPocket · PocketDepth = 0.2, Push = 0.15 | Extrude#4 | 5 | 12 | 8 | 0 | 0 | 25 | 0 | 0 | 0 |
| 🟢 | SlotPocket · PocketDepth = 0.2, Push = 0.15 | FaceOperation#5 | 6 | 16 | 12 | 0 | 0 | 34 | 0 | 0 | 0 |
| 🟢 | SlotPocket · SlotLength = 2.5, SlotX = 1.5, Width = 6 | Extrude#4 | 5 | 12 | 8 | 0 | 0 | 25 | 0 | 0 | 0 |
| 🟢 | SlotPocket · SlotLength = 2.5, SlotX = 1.5, Width = 6 | FaceOperation#5 | 6 | 16 | 12 | 0 | 0 | 34 | 0 | 0 | 0 |
| 🟢 | HoleLeavesPlate · authoring | Extrude#4 | 4 | 12 | 8 | 0 | 0 | 24 | 0 | 0 | 0 |
| 🟢 | HoleLeavesPlate · authoring | Blend#5 | 5 | 13 | 9 | 0 | 0 | 27 | 0 | 0 | 0 |
| 🟢 | HoleLeavesPlate · authoring | FaceOperation#6 | 3 | 9 | 7 | 0 | 0 | 19 | 0 | 0 | 0 |
| 🟢 | HoleLeavesPlate · HoleRadius = 0.55, HoleX = 0.8 | Extrude#4 | 4 | 12 | 8 | 0 | 0 | 24 | 0 | 0 | 0 |
| 🟢 | HoleLeavesPlate · HoleRadius = 0.55, HoleX = 0.8 | Blend#5 | 5 | 13 | 9 | 0 | 0 | 27 | 0 | 0 | 0 |
| 🟢 | HoleLeavesPlate · HoleRadius = 0.55, HoleX = 0.8 | FaceOperation#6 | 3 | 9 | 7 | 0 | 0 | 19 | 0 | 0 | 0 |
| 🟢 | HoleLeavesPlate · Chamfer = 0.15, Grow = -0.2 | Extrude#4 | 4 | 12 | 8 | 0 | 0 | 24 | 0 | 0 | 0 |
| 🟢 | HoleLeavesPlate · Chamfer = 0.15, Grow = -0.2 | Blend#5 | 5 | 13 | 9 | 0 | 0 | 27 | 0 | 0 | 0 |
| 🟢 | HoleLeavesPlate · Chamfer = 0.15, Grow = -0.2 | FaceOperation#6 | 3 | 9 | 7 | 0 | 0 | 19 | 0 | 0 | 0 |
| 🟢 | HoleLeavesPlate · HoleX = 5 | Extrude#4 | 6 | 12 | 8 | 0 | 0 | 26 | 0 | 0 | 0 |
| 🟢 | HoleLeavesPlate · HoleX = 5 | FaceOperation#6 | 1 | 4 | 4 | 0 | 0 | 9 | 0 | 0 | 0 |
| 🟢 | HoleLeavesPlate · Grow = 0.4, HoleX = 5 | Extrude#4 | 6 | 12 | 8 | 0 | 0 | 26 | 0 | 0 | 0 |
| 🟢 | HoleLeavesPlate · Grow = 0.4, HoleX = 5 | FaceOperation#6 | 1 | 4 | 4 | 0 | 0 | 9 | 0 | 0 | 0 |
| 🟢 | SplitFacePush · authoring | Extrude#4 | 6 | 12 | 8 | 0 | 0 | 26 | 0 | 0 | 0 |
| 🟢 | SplitFacePush · authoring | FaceOperation#5 | 1 | 4 | 4 | 0 | 0 | 9 | 0 | 0 | 0 |
| 🟢 | SplitFacePush · Push = -0.2 | Extrude#4 | 6 | 12 | 8 | 0 | 0 | 26 | 0 | 0 | 0 |
| 🟢 | SplitFacePush · Push = -0.2 | FaceOperation#5 | 1 | 4 | 4 | 0 | 0 | 9 | 0 | 0 | 0 |
| 🟢 | SplitFacePush · GrooveX = 0.5 | Extrude#4 | 3 | 10 | 8 | 13 | 6 | 15 | 6 | 0 | 0 |
| 🟢 | SplitFacePush · GrooveX = 0.5 | FaceOperation#5 | 5 | 16 | 12 | 0 | 0 | 33 | 0 | 0 | 0 |
| 🟢 | SplitFacePush · GrooveX = 1.5 | Extrude#4 | 3 | 10 | 8 | 13 | 6 | 15 | 6 | 0 | 0 |
| 🟢 | SplitFacePush · GrooveX = 2.5, Push = 0.4 | Extrude#4 | 3 | 10 | 8 | 13 | 6 | 15 | 6 | 0 | 0 |
| 🟢 | SplitFacePush · GrooveX = 2.5, Push = 0.4 | FaceOperation#5 | 5 | 16 | 12 | 0 | 0 | 33 | 0 | 0 | 0 |

## Resolution expectations (F6)

- SplitFacePush GrooveX = -1.0: expected Exact, got Exact
- SplitFacePush GrooveX = -1.0: expected Exact, got Exact
- SplitFacePush GrooveX = 0.50: expected Geometric, got Geometric
- SplitFacePush GrooveX = 1.50: expected Refused, got Refused
- SplitFacePush GrooveX = 2.50: expected Geometric, got Geometric
- ConflictingSketch Sketch#1: Sketch: conflicting constraints {Width, TopLength}
- ConflictingSketch Extrude#2: Extrude: profile sketch Sketch#1 is unavailable
- ConflictingSketch Blend#3: Blend: no body yet

## Name samples

| Where | Name |
|---|---|
| BlockFillet face | `Extrude#2.Side(Line1)` |
| BlockFillet face | `Extrude#2.Side(Line2)` |
| BlockFillet face | `Extrude#2.Side(Line3)` |
| BlockFillet face | `Extrude#2.Side(Line4)` |
| BlockFillet face | `Extrude#2.Cap(Start)` |
| BlockFillet face | `Extrude#2.Cap(End)` |
| BlockFillet face | `Blend#3.Strip(Edge(Extrude#2.Cap(End)|Extrude#2.Side(Line1)))` |
| BlockFillet edge | `Edge(Extrude#2.Cap(Start)|Extrude#2.Side(Line1))` |
| BlockFillet edge | `Edge(Extrude#2.Side(Line1)|Extrude#2.Side(Line2))` |
| BlockFillet edge | `Edge(Blend#3.Strip(Edge(Extrude#2.Cap(End)|Extrude#2.Side(Line1)))|Extrude#2.Side(Line1))` |
| BlockFillet edge | `Edge(Extrude#2.Side(Line1)|Extrude#2.Side(Line4))` |
| BossFillet face | `Extrude#2.Side(Line1)` |
| BossFillet face | `Extrude#2.Side(Line2)` |
| BossFillet face | `Extrude#2.Side(Line3)` |
| BossFillet face | `Extrude#2.Side(Line4)` |
| BossFillet face | `Extrude#2.Cap(Start)` |
| BossFillet face | `Extrude#2.Cap(End)` |
| BossFillet face | `Extrude#4.Side(Circle1)` |
| BossFillet face | `Extrude#4.Cap(End)` |
| BossFillet face | `Blend#5.Strip(Edge(Extrude#2.Cap(End)|Extrude#4.Side(Circle1)))` |
| BossFillet edge | `Edge(Extrude#2.Cap(Start)|Extrude#2.Side(Line1))` |
| BossFillet edge | `Edge(Extrude#2.Side(Line1)|Extrude#2.Side(Line2))` |
| BossFillet edge | `Edge(Extrude#2.Cap(End)|Extrude#2.Side(Line1))` |
| BossFillet edge | `Edge(Extrude#2.Side(Line1)|Extrude#2.Side(Line4))` |
