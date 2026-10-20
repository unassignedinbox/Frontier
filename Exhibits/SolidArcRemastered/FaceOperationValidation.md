# Face-operation validation (step 6, J + K)

**🟢 PASS** · Chain 6/6 · CorpusE 36/36 · Extra 12/12 · Refusal 8/8 · corpus E refusals on HoleSlab / Can: 0 (old FaceEditSolver: 17) · 118911 ms

## Gates

| | Gate | Measures | Limit | Worst | Checks | Failures |
|---|---|---|---|---|---|---|
| 🟢 | E1 | Volume vs hand-derived closed form | \|V − V*\| ≤ 10⁻⁹·max(1, V*) | 4.97e-15 | 51 | 0 |
| 🟢 | E2 | Audit sound · category · genus · F/E/V counts · source untouched | all hold | 0.00e+00 | 54 | 0 |
| 🟢 | E3 | Every edge on both face carriers (9 samples) · vertices on every incident curve | ≤ 10⁻⁹·L | 2.57e-16 | 54 | 0 |
| 🟢 | E4 | Tessellation N1 · N3 · N4 · N6 (sheets: laminar sides single, no N4) | 0 · 0 · ≤ 1 · 0 | 2.31e-01 | 54 | 0 |
| 🟢 | E5 | Generalized winding number ±ε off every face agrees with its outward side (self-intersection probe) | 0 wrong | 0.00e+00 | 2210 | 0 |
| 🟢 | E6 | Chains ≡ independent reference construction (V, F/E/V) | rel. ≤ 10⁻⁹, equal | 3.78e-16 | 6 | 0 |
| 🟢 | E7 | Refusals: atomic, verb-prefixed, source intact | all refused | 0.00e+00 | 8 | 0 |
| 🟢 | E8 | Same specimen twice → identical tessellation digest | equal | 0.00e+00 | 4 | 0 |
| 🟢 | E9 | Scale 10⁻³ … 10³: relative volume error | ≤ 10⁻⁹ | 1.16e-14 | 12 | 0 |

## Corpus section E

| | Specimen | Corpus command | Old kernel | Result | F / E / V | Genus | V | V* (closed form) | \|error\| | E3 / L | Triangles | ms |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 🟢 | PushBlock | `push Block --face=+z 0.5` | ran | solid | 6 / 12 / 8 | 0 | 4.5000000000 | 4.5000000000 | 3.6e-15 | 0.0e+00 | 174 | 10.3 |
| 🟢 | PushHoleSlab | `push HoleSlab --face=+z 0.2` | ran | solid | 7 / 15 / 10 | 1 | 4.2910619901 | 4.2910619901 | 8.9e-16 | 3.0e-17 | 930 | 25.3 |
| 🟢 | PushCan | `push Can --face=+z 0.5` | ran | solid | 3 / 3 / 2 | 0 | 1.6964600329 | 1.6964600329 | 2.9e-15 | 1.5e-16 | 2096 | 11.0 |
| 🟢 | ExtrudeFacesBlock | `extrudefaces Block --face=+z 0.5` | ran | solid | 10 / 20 / 12 | 0 | 4.5000000000 | 4.5000000000 | 3.6e-15 | 0.0e+00 | 162 | 8.4 |
| 🟢 | ExtrudeFacesHoleSlab | `extrudefaces HoleSlab --face=+z 0.2` | ran | solid | 12 / 25 / 15 | 1 | 4.2910619901 | 4.2910619901 | 8.9e-16 | 3.0e-17 | 844 | 20.3 |
| 🟢 | ExtrudeFacesCan | `extrudefaces Can --face=+z 0.5` | ran | solid | 4 / 5 / 3 | 0 | 1.6964600329 | 1.6964600329 | 2.9e-15 | 1.5e-16 | 2176 | 9.0 |
| 🟢 | TweakBlock | `tweak Block --face=+z 0.5` | ran | solid | 6 / 12 / 8 | 0 | 4.5000000000 | 4.5000000000 | 3.6e-15 | 0.0e+00 | 174 | 9.3 |
| 🟢 | TweakHoleSlab | `tweak HoleSlab --face=+z 0.2` | refused | solid | 7 / 15 / 10 | 1 | 4.2910619901 | 4.2910619901 | 8.9e-16 | 3.0e-17 | 930 | 25.0 |
| 🟢 | TweakCan | `tweak Can --face=+z 0.5` | ran | solid | 3 / 3 / 2 | 0 | 1.6964600329 | 1.6964600329 | 2.9e-15 | 1.5e-16 | 2096 | 10.3 |
| 🟢 | OffsetFaceBlock | `offsetface Block --face=+z 0.5` | ran | solid | 6 / 12 / 8 | 0 | 4.5000000000 | 4.5000000000 | 3.6e-15 | 0.0e+00 | 174 | 10.1 |
| 🟢 | OffsetFaceHoleSlab | `offsetface HoleSlab --face=+z 0.2` | refused | solid | 7 / 15 / 10 | 1 | 4.2910619901 | 4.2910619901 | 8.9e-16 | 3.0e-17 | 930 | 24.4 |
| 🟢 | OffsetFaceCan | `offsetface Can --face=+z 0.5` | refused | solid | 3 / 3 / 2 | 0 | 1.6964600329 | 1.6964600329 | 2.9e-15 | 1.5e-16 | 2096 | 10.8 |
| 🟢 | ScaleBlock | `scale Block --face=+z 0.5` | ran | solid | 6 / 12 / 8 | 0 | 1.7500000000 | 1.7500000000 | 4.4e-16 | 2.3e-16 | 114 | 7.1 |
| 🟢 | ScaleHoleSlab | `scale HoleSlab --face=+z 0.5` | refused | solid | 7 / 15 / 10 | 1 | 1.8773396207 | 1.8773396207 | 9.3e-15 | 1.7e-16 | 704 | 23.5 |
| 🟢 | ScaleCan | `scale Can --face=+z 0.5` | refused | solid | 3 / 3 / 2 | 0 | 0.6597344573 | 0.6597344573 | 1.1e-16 | 2.3e-16 | 1546 | 9.0 |
| 🟢 | RotateBlock | `rotate Block --face=+z 5 --axis=(1,0,0)` | ran (2.994292) | solid | 6 / 12 / 8 | 0 | 3.0000000000 | 3.0000000000 | 4.4e-16 | 8.2e-17 | 146 | 8.8 |
| 🟢 | RotateHoleSlab | `rotate HoleSlab --face=+z 5 --axis=(1,0,0)` | refused | solid | 7 / 15 / 10 | 1 | 3.2182964926 | 3.2182964926 | 4.4e-15 | 9.1e-17 | 734 | 36.0 |
| 🟢 | RotateCan | `rotate Can --face=+z 5 --axis=(1,0,0)` | refused | solid | 3 / 3 / 2 | 0 | 1.1309733553 | 1.1309733553 | 2.4e-15 | 2.6e-16 | 1618 | 22.0 |
| 🟢 | HollowBlock | `shell Block --face=+z 0.1` | ran | solid | 11 / 24 / 16 | 0 | 0.8940000000 | 0.8940000000 | 0.0e+00 | 5.2e-17 | 234 | 46.1 |
| 🟢 | HollowHoleSlab | `shell HoleSlab --face=+z 0.1` | refused | solid | 14 / 30 / 20 | 1 | 1.1734623814 | 1.1734623814 | 8.9e-16 | 6.8e-17 | 1128 | 116.9 |
| 🟢 | HollowCan | `shell Can --face=+z 0.1` | refused | solid | 5 / 6 / 4 | 0 | 0.4241150082 | 0.4241150082 | 1.9e-15 | 1.5e-16 | 3098 | 34.9 |
| 🟢 | DraftBlock | `draft Block --face=+x 5` | ran | solid | 6 / 12 / 8 | 0 | 3.0656164976 | 3.0656164976 | 4.4e-16 | 1.6e-16 | 138 | 8.4 |
| 🟢 | DraftHoleSlab | `draft HoleSlab --face=+x 5` | refused | solid | 7 / 15 / 10 | 1 | 3.2497924115 | 3.2497924115 | 1.8e-15 | 1.2e-16 | 742 | 24.6 |
| 🟢 | DraftCan | `draft Can --face=+x 5` | refused | solid | 3 / 3 / 2 | 0 | 1.3039011297 | 1.3039011297 | 4.4e-16 | 1.7e-16 | 1578 | 9.6 |
| 🟢 | ExtendFaceBlock | `extendface Block --face=+z 0.2` | ran | solid | 6 / 12 / 8 | 0 | 4.5600000000 | 4.5600000000 | 2.7e-15 | 8.5e-17 | 134 | 9.9 |
| 🟢 | ExtendFaceHoleSlab | `extendface HoleSlab --face=+z 0.2` | refused | solid | 7 / 15 / 10 | 1 | 4.7781902755 | 4.7781902755 | 2.7e-15 | 7.6e-17 | 778 | 25.7 |
| 🟢 | ExtendFaceCan | `extendface Can --face=+z 0.2` | refused | solid | 3 / 3 / 2 | 0 | 2.0106192983 | 2.0106192983 | 3.6e-15 | 2.0e-16 | 1376 | 9.5 |
| 🟢 | TrimFaceBlock | `trimface Block --face=+z 0.2` | ran | solid | 6 / 12 / 8 | 0 | 1.7600000000 | 1.7600000000 | 2.2e-16 | 4.6e-17 | 172 | 7.7 |
| 🟢 | TrimFaceHoleSlab | `trimface HoleSlab --face=+z 0.2` | refused | solid | 7 / 15 / 10 | 1 | 1.6996062623 | 1.6996062623 | 1.1e-15 | 6.3e-17 | 1090 | 21.9 |
| 🟢 | TrimFaceCan | `trimface Can --face=+z 0.2` | refused | solid | 3 / 3 / 2 | 0 | 0.5026548246 | 0.5026548246 | 8.9e-16 | 9.8e-17 | 2088 | 8.4 |
| 🟢 | DeleteFaceBlock | `deleteface Block --face=+z` | ran (sheet) | sheet | 5 / 12 / 8 | 0 | – | – | – | 0.0e+00 | 106 | 0.2 |
| 🟢 | DeleteFaceHoleSlab | `deleteface HoleSlab --face=+z` | refused | sheet | 6 / 15 / 10 | 0 | – | – | – | 1.1e-17 | 612 | 0.3 |
| 🟢 | DeleteFaceCan | `deleteface Can --face=+z` | refused | sheet | 2 / 3 / 2 | 0 | – | – | – | 1.5e-16 | 1510 | 0.1 |
| 🟢 | HealBlock | `heal Block` | ran | solid | 6 / 12 / 8 | 0 | 3.0000000000 | 3.0000000000 | 1.3e-15 | 0.0e+00 | 142 | 6.2 |
| 🟢 | HealHoleSlab | `heal HoleSlab` | ran | solid | 7 / 15 / 10 | 1 | 3.2182964926 | 3.2182964926 | 4.4e-16 | 4.3e-17 | 748 | 17.4 |
| 🟢 | HealCan | `heal Can` | ran | solid | 3 / 3 / 2 | 0 | 1.1309733553 | 1.1309733553 | 2.0e-15 | 1.5e-16 | 1660 | 6.6 |

## Chains and extras

| | Specimen | Family | Result | F / E / V | Genus | V | V* (closed form) | \|error\| | E3 / L | Method (last step) | ms |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 🟢 | HealExtrudeBlock | Chain | solid | 6 / 12 / 8 | 0 | 4.5000000000 | 4.5000000000 | 3.6e-15 | 0.0e+00 | merged 4 face(s), removed 0 slit(s), joined 4 edge pair(s) | 15.1 |
| 🟢 | ThickenDeleteBlock | Chain | solid | 11 / 24 / 16 | 0 | 0.8940000000 | 0.8940000000 | 2.2e-16 | 4.3e-17 | merged 3 face(s), removed 0 slit(s), joined 0 edge pair(s) | 20.9 |
| 🟢 | OffsetBodyOutBlock | Extra | solid | 6 / 12 / 8 | 0 | 4.4880000000 | 4.4880000000 | 5.3e-15 | 5.2e-17 | face change: 6 carrier(s) replaced, edges and vertices re-solved | 9.7 |
| 🟢 | OffsetBodyInBlock | Extra | solid | 6 / 12 / 8 | 0 | 1.8720000000 | 1.8720000000 | 6.7e-16 | 5.2e-17 | face change: 6 carrier(s) replaced, edges and vertices re-solved | 7.5 |
| 🟢 | HealExtrudeHoleSlab | Chain | solid | 7 / 15 / 10 | 1 | 5.9002102364 | 5.9002102364 | 0.0e+00 | 3.0e-17 | merged 5 face(s), removed 0 slit(s), joined 5 edge pair(s) | 42.0 |
| 🟢 | ThickenDeleteHoleSlab | Chain | solid | 14 / 30 / 20 | 1 | 1.1734623814 | 1.1734623814 | 1.3e-15 | 6.1e-17 | merged 3 face(s), removed 1 slit(s), joined 0 edge pair(s) | 54.0 |
| 🟢 | OffsetBodyOutHoleSlab | Extra | solid | 7 / 15 / 10 | 1 | 5.3241239199 | 5.3241239199 | 1.8e-15 | 6.5e-17 | face change: 7 carrier(s) replaced, edges and vertices re-solved | 24.6 |
| 🟢 | OffsetBodyInHoleSlab | Extra | solid | 7 / 15 / 10 | 1 | 1.6358672889 | 1.6358672889 | 6.7e-16 | 6.5e-17 | face change: 7 carrier(s) replaced, edges and vertices re-solved | 20.5 |
| 🟢 | HealExtrudeCan | Chain | solid | 3 / 3 / 2 | 0 | 1.6964600329 | 1.6964600329 | 2.9e-15 | 1.5e-16 | merged 1 face(s), removed 0 slit(s), joined 1 edge pair(s) | 17.0 |
| 🟢 | ThickenDeleteCan | Chain | solid | 5 / 6 / 4 | 0 | 0.4241150082 | 0.4241150082 | 1.8e-15 | 1.5e-16 | merged 0 face(s), removed 1 slit(s), joined 0 edge pair(s) | 25.6 |
| 🟢 | OffsetBodyOutCan | Extra | solid | 3 / 3 / 2 | 0 | 1.8472564803 | 1.8472564803 | 1.6e-15 | 1.8e-16 | face change: 3 carrier(s) replaced, edges and vertices re-solved | 10.5 |
| 🟢 | OffsetBodyInCan | Extra | solid | 3 / 3 / 2 | 0 | 0.6283185307 | 0.6283185307 | 1.1e-16 | 1.2e-16 | face change: 3 carrier(s) replaced, edges and vertices re-solved | 8.4 |
| 🟢 | DeleteBore | Extra | solid | 6 / 12 / 8 | 0 | 3.6000000000 | 3.6000000000 | 4.4e-16 | 1.1e-17 | hole heal: 1 face(s) and 2 hole loop(s) removed, body stays solid | 8.2 |
| 🟢 | DraftAllSides | Extra | solid | 6 / 12 / 8 | 0 | 3.3164160107 | 3.3164160107 | 1.3e-15 | 8.0e-17 | face change: 4 carrier(s) replaced, edges and vertices re-solved | 8.5 |
| 🟢 | DraftBore | Extra | solid | 7 / 15 / 10 | 1 | 3.2610914447 | 3.2610914447 | 4.4e-16 | 3.0e-17 | face change: 1 carrier(s) replaced, edges and vertices re-solved | 23.7 |
| 🟢 | OffsetBoreWall | Extra | solid | 7 / 15 / 10 | 1 | 3.0298009334 | 3.0298009334 | 8.9e-16 | 6.8e-17 | face change: 1 carrier(s) replaced, edges and vertices re-solved | 22.7 |
| 🟢 | ScaleGrowBlock | Extra | solid | 6 / 12 / 8 | 0 | 7.0000000000 | 7.0000000000 | 3.6e-15 | 2.5e-16 | face change: 4 carrier(s) replaced, edges and vertices re-solved | 20.3 |
| 🟢 | ExtrudeSlabBottom | Extra | solid | 12 / 25 / 15 | 1 | 4.8274447389 | 4.8274447389 | 1.8e-15 | 4.3e-17 | direct rebuild: 1 face(s) raised, 5 side face(s) | 20.5 |

## Refusals

| | Specimen | Refusal |
|---|---|---|
| 🟢 | PushThrough | Push: E4: runs backwards (the change exceeds the edge; topology would change, 🚧) |
| 🟢 | TrimCanAway | TrimFace: F2: offset -0.700000 degenerates the Cylinder |
| 🟢 | TrimSlabAcross | TrimFace: E16: runs backwards (the change exceeds the edge; topology would change, 🚧) |
| 🟢 | DraftTop | Draft: F0 is perpendicular to the pull direction; nothing to draft |
| 🟢 | HollowCanThick | Hollow: F2: the wall is thicker than the curvature radius of the Cylinder |
| 🟢 | ThickenSolid | Thicken: body is not a sheet (delete a face first) |
| 🟢 | ExtrudeCurved | ExtrudeFaces: F2 is not planar |
| 🟢 | RotateCanSteep | Rotate: E3 passes through F4 (self-intersection; removal is 🚧) |

## Scale (E9)

| | s | PushHoleSlab rel. error | DraftCan rel. error | HollowHoleSlab rel. error |
|---|---|---|---|---|
| 🟢 | 1e-03 | 0.0e+00 | 4.8e-16 | 5.3e-16 |
| 🟢 | 1e-01 | 2.0e-16 | 1.7e-16 | 1.3e-15 |
| 🟢 | 1e+01 | 6.4e-16 | 1.4e-15 | 1.2e-14 |
| 🟢 | 1e+03 | 1.8e-15 | 1.3e-15 | 2.6e-15 |
