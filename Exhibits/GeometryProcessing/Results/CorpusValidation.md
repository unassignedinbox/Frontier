# Corpus validation (step 8 — kernel gaps G2–G5)

**🟢 PASS** · 79 / 79 rows pass · corpus result rows 71 (sections A–H) · old-kernel refusals among them 19 → now passing 19 · refused now 0 · 6196 ms

> 📝 One row per corpus command that produces a body (primitive arguments of booleans and face operations are built inside their row). Section E rows are the `CorpusE` face-operation specimens (bodies at the origin, same sizes and picks). Section X adds the G4 / G5 scenes the corpus could not express.

## Gates

| | Gate | Measures | Limit | Worst | Checks | Failures |
|---|---|---|---|---|---|---|
| 🟢 | R1 | Corpus rows refused (old kernel: 19 of 176) | 0 | 0.00e+00 | 79 | 0 |
| 🟢 | R2 | Volume vs hand-derived closed form | \|V − V*\| ≤ 10⁻⁹·max(1, V*) | 3.59e-14 | 76 | 0 |
| 🟢 | R3 | Audit sound · expected category · source bodies untouched | all hold | 0.00e+00 | 79 | 0 |
| 🟢 | R4 | Edges between faces of one oriented carrier (G5: maximal faces) | 0 (ExtrudeFaces keeps its ring by definition) | 0.00e+00 | 76 | 0 |
| 🟢 | R5 | Tessellation watertight · Euler equal · no inversions · \|ΔV\| ≤ sag·area | all hold | 4.71e-01 | 81 | 0 |

## Rows

| | § | # | Corpus command | Old kernel | Remastered route | Result | F / E / V | R4 | V | V* (closed form) | \|error\| | Triangles | ms |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 🟢 | A | 2 | `box (0,0,0) (2,1.5,1)` | ran | ConstructBlock | solid | 6 / 12 / 8 | 0 | 3.0000000000 | 3.0000000000 | 1.3e-15 | 142 | 0.0 |
| 🟢 | A | 3 | `cylinder (5,1,0) 0.6 1` | ran | ConstructCylinder | solid | 3 / 3 / 2 | 0 | 1.1309733553 | 1.1309733553 | 2.2e-16 | 1660 | 0.0 |
| 🟢 | A | 4 | `cone (8,1,0) 0.6 0.3 1` | ran | ConstructFrustum | solid | 3 / 3 / 2 | 0 | 0.6597344573 | 0.6597344573 | 1.1e-16 | 1546 | 0.0 |
| 🟢 | A | 5 | `sphere (11,1,0.5) 0.5` | ran (V err 1.7e-04) | ConstructSphere | solid | 1 / 1 / 2 | 0 | 0.5235987756 | 0.5235987756 | 3.3e-16 | 5172 | 0.0 |
| 🟢 | A | 6 | `torus (14,1,0.3) 0.8 0.25` | ran | ConstructTorus | solid | 1 / 2 / 1 | 0 | 0.9869604401 | 0.9869604401 | 4.4e-16 | 3562 | 0.0 |
| 🟢 | B | 8 | `extrude ProfileRect 1` | ran | ExtrudeProfile | solid | 6 / 12 / 8 | 0 | 3.0000000000 | 3.0000000000 | 3.6e-15 | 140 | 4.3 |
| 🟢 | B | 10 | `extrude ProfileCircle 1` | ran | ExtrudeProfile | solid | 3 / 3 / 2 | 0 | 1.1309733553 | 1.1309733553 | 1.3e-15 | 1656 | 18.1 |
| 🟢 | B | 12 | `revolve RevolveProfile 360 --origin=(7.5,4,0) --axis=(0,1,0)` | ran | RevolveProfile(2π) | solid | 4 / 6 / 4 | 0 | 2.3561944902 | 2.3561944902 | 1.8e-15 | 2154 | 9.5 |
| 🟢 | B | 14 | `revolve HalfRevolveProfile 180 --origin=(9.5,4,0) --axis=(0,1,0)` | ran | RevolveProfile(π) — G2 partial | solid | 6 / 12 / 8 | 0 | 1.1780972451 | 1.1780972451 | 6.7e-16 | 1344 | 5.9 |
| 🟢 | B | 17 | `loft LoftFoot LoftHead` | ran | LoftCircles → frustum | solid | 3 / 3 / 2 | 0 | 0.6597344573 | 0.6597344573 | 1.1e-16 | 1546 | 0.0 |
| 🟢 | B | 19 | `pipe PipeLine 0.2` | ran | PipeAlongPath(line) → cylinder | solid | 3 / 3 / 2 | 0 | 0.3769911184 | 0.3769911184 | 3.9e-16 | 2802 | 0.0 |
| 🟢 | B | 21 | `pipe PipeArc 0.2 (arc (6,8) 1 0 90)` | ran, freeform (V err 2.8e-06) | PipeAlongPath(arc) → partial torus (G2) | solid | 3 / 3 / 2 | 0 | 0.1973920880 | 0.1973920880 | 5.8e-16 | 2150 | 8.8 |
| 🟢 | B | 24 | `sweep SweepProfile SweepPath` | ran | SweepProfileAlongLine → extrude | solid | 3 / 3 / 2 | 0 | 0.2513274123 | 0.2513274123 | 2.2e-16 | 2772 | 6.2 |
| 🟢 | C | 27 | `boolean union BoxA -- BoxB` | ran (14 faces) | ComputeBoolean(Union) | solid | 6 / 12 / 8 | 0 | 36.0000000000 | 36.0000000000 | 1.6e-13 | 140 | 39.0 |
| 🟢 | C | 28 | `boolean subtract BoxA -- BoxB` | ran | ComputeBoolean(Difference) | solid | 6 / 12 / 8 | 0 | 12.0000000000 | 12.0000000000 | 4.6e-14 | 172 | 23.2 |
| 🟢 | C | 29 | `boolean intersect BoxA -- BoxB` | ran | ComputeBoolean(Intersection) | solid | 6 / 12 / 8 | 0 | 12.0000000000 | 12.0000000000 | 7.8e-14 | 172 | 20.6 |
| 🟢 | C | 32 | `boolean subtract SlabC -- BoreC` | ran | ComputeBoolean(Difference) | solid | 7 / 15 / 10 | 0 | 3.2182964926 | 3.2182964926 | 4.6e-14 | 748 | 28.4 |
| 🟢 | C | 35 | `boolean union Plate -- Boss` | ran | ComputeBoolean(Union) | solid | 8 / 15 / 10 | 0 | 4.1497787144 | 4.1497787144 | 3.6e-15 | 712 | 26.5 |
| 🟢 | C | 38 | `boolean union Pedestal -- Dome` | ran (V err 2.3e-04) | ComputeBoolean(Union) | solid | 7 / 14 / 10 | 0 | 4.2617993878 | 4.2617993878 | 6.2e-15 | 1028 | 17.9 |
| 🟢 | C | 41 | `boolean subtract Socket -- Scoop` | ran (V err 1.1e-04) | ComputeBoolean(Difference) | solid | 7 / 14 / 10 | 0 | 3.7382006122 | 3.7382006122 | 4.9e-15 | 1116 | 18.2 |
| 🟢 | C | 44 | `boolean union CrossX -- CrossY (CrossUnequal)` | refused | ComputeBoolean(Union) | solid | 7 / 9 / 6 | 0 | 4.0031956814 | 4.0031956814 | 3.6e-15 | 3540 | 137.8 |
| 🟢 | C | 47 | `boolean union OffsetCrossX -- OffsetCrossY (CrossOffset)` | ran (4.009803 — wrong) | ComputeBoolean(Union) | solid | 7 / 9 / 6 | 0 | 4.0099890528 | 4.0099890528 | 8.0e-15 | 3590 | 141.3 |
| 🟢 | C | 50 | `boolean union SteinmetzX -- SteinmetzY` | refused | ComputeBoolean(Union) | solid | 8 / 12 / 6 | 0 | 5.6165186405 | 5.6165186405 | 4.4e-15 | 3306 | 70.8 |
| 🟢 | D | 52 | `beveladd 0.2 --mode=fillet --targets=BlendBlock:e0…e11` | ran | BlendEdges(Fillet, 12 edges) | solid | 26 / 48 / 24 | 0 | 2.8562005519 | 2.8562005519 | 3.1e-15 | 5420 | 146.0 |
| 🟢 | D | 54 | `fillet LegacyFilletBlock 0.2 --edges=0…11` | ran, audit 🔴 trace-on-edge ×4 | BlendEdges(Fillet, 12 edges) | solid | 26 / 48 / 24 | 0 | 2.8562005519 | 2.8562005519 | 4.4e-16 | 5404 | 139.8 |
| 🟢 | D | 56 | `chamfer LegacyChamferBlock 0.15 --edges=0` | ran | BlendEdges(Chamfer, 1 edge) | solid | 7 / 15 / 10 | 0 | 2.9775000000 | 2.9775000000 | 2.4e-14 | 150 | 10.3 |
| 🟢 | D | 60 | `beveladd 0.1 --mode=fillet --targets=HoleSlabD:e{rim +z}` | ran, freeform | BlendEdges(Fillet, 1 edge) | solid | 8 / 17 / 11 | 0 | 3.2119275831 | 3.2119275831 | 5.0e-14 | 1340 | 56.7 |
| 🟢 | E | – | `push Block --face=+z 0.5` | ran | ApplyFaceOperation(Push) | solid | 6 / 12 / 8 | 0 | 4.5000000000 | 4.5000000000 | 3.6e-15 | 174 | 9.5 |
| 🟢 | E | – | `push HoleSlab --face=+z 0.2` | ran | ApplyFaceOperation(Push) | solid | 7 / 15 / 10 | 0 | 4.2910619901 | 4.2910619901 | 8.9e-16 | 930 | 50.7 |
| 🟢 | E | – | `push Can --face=+z 0.5` | ran | ApplyFaceOperation(Push) | solid | 3 / 3 / 2 | 0 | 1.6964600329 | 1.6964600329 | 2.9e-15 | 2096 | 10.6 |
| 🟢 | E | – | `extrudefaces Block --face=+z 0.5` | ran | ApplyFaceOperation(ExtrudeFaces) | solid | 10 / 20 / 12 | 4 | 4.5000000000 | 4.5000000000 | 3.6e-15 | 162 | 8.3 |
| 🟢 | E | – | `extrudefaces HoleSlab --face=+z 0.2` | ran | ApplyFaceOperation(ExtrudeFaces) | solid | 12 / 25 / 15 | 4 | 4.2910619901 | 4.2910619901 | 8.9e-16 | 844 | 48.2 |
| 🟢 | E | – | `extrudefaces Can --face=+z 0.5` | ran | ApplyFaceOperation(ExtrudeFaces) | solid | 4 / 5 / 3 | 0 | 1.6964600329 | 1.6964600329 | 2.9e-15 | 2176 | 9.2 |
| 🟢 | E | – | `tweak Block --face=+z 0.5` | ran | ApplyFaceOperation(Tweak) | solid | 6 / 12 / 8 | 0 | 4.5000000000 | 4.5000000000 | 3.6e-15 | 174 | 9.7 |
| 🟢 | E | – | `tweak HoleSlab --face=+z 0.2` | refused | ApplyFaceOperation(Tweak) | solid | 7 / 15 / 10 | 0 | 4.2910619901 | 4.2910619901 | 8.9e-16 | 930 | 50.6 |
| 🟢 | E | – | `tweak Can --face=+z 0.5` | ran | ApplyFaceOperation(Tweak) | solid | 3 / 3 / 2 | 0 | 1.6964600329 | 1.6964600329 | 2.9e-15 | 2096 | 10.3 |
| 🟢 | E | – | `offsetface Block --face=+z 0.5` | ran | ApplyFaceOperation(OffsetFace) | solid | 6 / 12 / 8 | 0 | 4.5000000000 | 4.5000000000 | 3.6e-15 | 174 | 9.5 |
| 🟢 | E | – | `offsetface HoleSlab --face=+z 0.2` | refused | ApplyFaceOperation(OffsetFace) | solid | 7 / 15 / 10 | 0 | 4.2910619901 | 4.2910619901 | 8.9e-16 | 930 | 51.1 |
| 🟢 | E | – | `offsetface Can --face=+z 0.5` | refused | ApplyFaceOperation(OffsetFace) | solid | 3 / 3 / 2 | 0 | 1.6964600329 | 1.6964600329 | 2.9e-15 | 2096 | 10.7 |
| 🟢 | E | – | `scale Block --face=+z 0.5` | ran | ApplyFaceOperation(Scale) | solid | 6 / 12 / 8 | 0 | 1.7500000000 | 1.7500000000 | 4.4e-16 | 114 | 7.6 |
| 🟢 | E | – | `scale HoleSlab --face=+z 0.5` | refused | ApplyFaceOperation(Scale) | solid | 7 / 15 / 10 | 0 | 1.8773396207 | 1.8773396207 | 9.3e-15 | 704 | 47.8 |
| 🟢 | E | – | `scale Can --face=+z 0.5` | refused | ApplyFaceOperation(Scale) | solid | 3 / 3 / 2 | 0 | 0.6597344573 | 0.6597344573 | 1.1e-16 | 1546 | 8.6 |
| 🟢 | E | – | `rotate Block --face=+z 5 --axis=(1,0,0)` | ran (2.994292) | ApplyFaceOperation(Rotate) | solid | 6 / 12 / 8 | 0 | 3.0000000000 | 3.0000000000 | 4.4e-16 | 146 | 9.0 |
| 🟢 | E | – | `rotate HoleSlab --face=+z 5 --axis=(1,0,0)` | refused | ApplyFaceOperation(Rotate) | solid | 7 / 15 / 10 | 0 | 3.2182964926 | 3.2182964926 | 4.4e-15 | 734 | 61.4 |
| 🟢 | E | – | `rotate Can --face=+z 5 --axis=(1,0,0)` | refused | ApplyFaceOperation(Rotate) | solid | 3 / 3 / 2 | 0 | 1.1309733553 | 1.1309733553 | 2.4e-15 | 1618 | 20.9 |
| 🟢 | E | – | `shell Block --face=+z 0.1` | ran | ApplyFaceOperation(Hollow) | solid | 11 / 24 / 16 | 0 | 0.8940000000 | 0.8940000000 | 0.0e+00 | 234 | 46.5 |
| 🟢 | E | – | `shell HoleSlab --face=+z 0.1` | refused | ApplyFaceOperation(Hollow) | solid | 14 / 30 / 20 | 0 | 1.1734623814 | 1.1734623814 | 8.9e-16 | 1128 | 140.7 |
| 🟢 | E | – | `shell Can --face=+z 0.1` | refused | ApplyFaceOperation(Hollow) | solid | 5 / 6 / 4 | 0 | 0.4241150082 | 0.4241150082 | 1.9e-15 | 3098 | 33.3 |
| 🟢 | E | – | `draft Block --face=+x 5` | ran | ApplyFaceOperation(Draft) | solid | 6 / 12 / 8 | 0 | 3.0656164976 | 3.0656164976 | 4.4e-16 | 138 | 8.4 |
| 🟢 | E | – | `draft HoleSlab --face=+x 5` | refused | ApplyFaceOperation(Draft) | solid | 7 / 15 / 10 | 0 | 3.2497924115 | 3.2497924115 | 1.8e-15 | 742 | 49.4 |
| 🟢 | E | – | `draft Can --face=+x 5` | refused | ApplyFaceOperation(Draft) | solid | 3 / 3 / 2 | 0 | 1.3039011297 | 1.3039011297 | 4.4e-16 | 1578 | 9.2 |
| 🟢 | E | – | `extendface Block --face=+z 0.2` | ran | ApplyFaceOperation(ExtendFace) | solid | 6 / 12 / 8 | 0 | 4.5600000000 | 4.5600000000 | 2.7e-15 | 134 | 9.9 |
| 🟢 | E | – | `extendface HoleSlab --face=+z 0.2` | refused | ApplyFaceOperation(ExtendFace) | solid | 7 / 15 / 10 | 0 | 4.7781902755 | 4.7781902755 | 2.7e-15 | 778 | 50.7 |
| 🟢 | E | – | `extendface Can --face=+z 0.2` | refused | ApplyFaceOperation(ExtendFace) | solid | 3 / 3 / 2 | 0 | 2.0106192983 | 2.0106192983 | 3.6e-15 | 1376 | 9.4 |
| 🟢 | E | – | `trimface Block --face=+z 0.2` | ran | ApplyFaceOperation(TrimFace) | solid | 6 / 12 / 8 | 0 | 1.7600000000 | 1.7600000000 | 2.2e-16 | 172 | 7.9 |
| 🟢 | E | – | `trimface HoleSlab --face=+z 0.2` | refused | ApplyFaceOperation(TrimFace) | solid | 7 / 15 / 10 | 0 | 1.6996062623 | 1.6996062623 | 1.1e-15 | 1090 | 49.4 |
| 🟢 | E | – | `trimface Can --face=+z 0.2` | refused | ApplyFaceOperation(TrimFace) | solid | 3 / 3 / 2 | 0 | 0.5026548246 | 0.5026548246 | 8.9e-16 | 2088 | 8.8 |
| 🟢 | E | – | `deleteface Block --face=+z` | ran (sheet) | ApplyFaceOperation(DeleteFace) | sheet | 5 / 12 / 8 | 0 | – | – | – | 106 | 0.3 |
| 🟢 | E | – | `deleteface HoleSlab --face=+z` | refused | ApplyFaceOperation(DeleteFace) | sheet | 6 / 15 / 10 | 0 | – | – | – | 612 | 30.8 |
| 🟢 | E | – | `deleteface Can --face=+z` | refused | ApplyFaceOperation(DeleteFace) | sheet | 2 / 3 / 2 | 0 | – | – | – | 1510 | 0.2 |
| 🟢 | E | – | `heal Block` | ran | ApplyFaceOperation(Heal) | solid | 6 / 12 / 8 | 0 | 3.0000000000 | 3.0000000000 | 1.3e-15 | 142 | 6.7 |
| 🟢 | E | – | `heal HoleSlab` | ran | ApplyFaceOperation(Heal) | solid | 7 / 15 / 10 | 0 | 3.2182964926 | 3.2182964926 | 4.4e-16 | 748 | 44.7 |
| 🟢 | E | – | `heal Can` | ran | ApplyFaceOperation(Heal) | solid | 3 / 3 / 2 | 0 | 1.1309733553 | 1.1309733553 | 2.0e-15 | 1660 | 7.0 |
| 🟢 | F | 160 | `mirror HoleSlabMirror --across=yz` | ran | MirrorBody | solid | 7 / 15 / 10 | 0 | 3.2182964926 | 3.2182964926 | 4.4e-14 | 738 | 38.5 |
| 🟢 | F | 162 | `array BlockArray --count=3 --step=(0,0,2)` | ran | ArrayBody (3 bodies, V summed) | solid | 18 / 36 / 24 | 0 | 9.0000000000 | 9.0000000000 | 1.4e-14 | 426 | 8.3 |
| 🟢 | G | 164 | `beveladd 0.2 --mode=fillet --targets=BlockNamingFillet:e0` | ran, freeform | BlendEdges(Fillet, 1 edge) | solid | 7 / 15 / 10 | 0 | 2.9828318531 | 2.9828318531 | 1.1e-13 | 784 | 17.1 |
| 🟢 | G | 167 | `boolean subtract NamingSlab -- NamingBore` | ran | ComputeBoolean(Difference) | solid | 7 / 15 / 10 | 0 | 3.2182964926 | 3.2182964926 | 3.0e-14 | 748 | 29.7 |
| 🟢 | G | 169 | `tweak BlockNamingTweak (0,0,0.5) --face=+z` | ran | ApplyFaceOperation(Tweak) | solid | 6 / 12 / 8 | 0 | 4.5000000000 | 4.5000000000 | 3.6e-14 | 174 | 9.6 |
| 🟢 | G | 171 | `push BlockNamingPush 0.5 --face=+z` | ran | ApplyFaceOperation(Push) | solid | 6 / 12 / 8 | 0 | 4.5000000000 | 4.5000000000 | 7.1e-15 | 174 | 9.6 |
| 🟢 | H | 173 | `extrude DriverRect 1 → beveladd 0.2 e0` | ran, freeform | RegenerateFeatureTree(BlockFillet) | solid | 7 / 15 / 10 | 0 | 2.9828318531 | 2.9828318531 | 6.5e-14 | 780 | 25.7 |
| 🟢 | H | 175 | `transform DriverRect --scale=(2,1,1) · move (0,0,0.5) → regenerate` | ran | RegenerateFeatureTree(BlockFillet) | solid | 7 / 15 / 10 | 0 | 5.9656637061 | 5.9656637061 | 7.2e-14 | 1052 | 41.1 |
| 🟢 | X | – | `Steinmetz ∩ (G4: 2 singular vertices)` | refused | ComputeBoolean(Intersection) | solid | 4 / 4 / 2 | 0 | 0.6666666667 | 0.6666666667 | 1.0e-15 | 2014 | 46.2 |
| 🟢 | X | – | `Steinmetz − (G4: two shells)` | refused | ComputeBoolean(Difference) | solid | 6 / 8 / 4 | 0 | 2.4749259869 | 2.4749259869 | 2.2e-15 | 2732 | 58.4 |
| 🟢 | X | – | `Steinmetz 60° ∩ (oblique, 16r³ / 3 sin θ)` | refused | ComputeBoolean(Intersection) | solid | 4 / 4 / 2 | 0 | 0.7698003589 | 0.7698003589 | 5.3e-15 | 1888 | 47.0 |
| 🟢 | X | – | `StepUnion (G5: coplanar back / sides / bottom)` | – | ComputeBoolean(Union) | solid | 8 / 18 / 12 | 0 | 30.0000000000 | 30.0000000000 | 1.4e-14 | 140 | 37.6 |
| 🟢 | X | – | `FlushSlotDiff (G5: slot through flush faces)` | – | ComputeBoolean(Difference) | solid | 10 / 24 / 16 | 0 | 15.0000000000 | 15.0000000000 | 1.1e-14 | 200 | 48.3 |
| 🟢 | X | – | `CornerUnion (G5: L-shaped maximal faces)` | – | ComputeBoolean(Union) | solid | 10 / 24 / 16 | 0 | 14.0000000000 | 14.0000000000 | 5.3e-15 | 152 | 34.4 |
| 🟢 | X | – | `MirrorCross (traced edges under a reflection)` | – | MirrorBody | solid | 7 / 9 / 6 | 0 | 4.0031956814 | 4.0031956814 | 5.3e-15 | 3616 | 306.8 |
| 🟢 | X | – | `PipeArc full turn (closed torus)` | – | PipeAlongPath(arc, 2π) → torus | solid | 1 / 2 / 1 | 0 | 0.7895683521 | 0.7895683521 | 2.2e-16 | 4184 | 19.3 |
