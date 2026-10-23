| # | Command | Outcome | Solid | Audit | Freeform edges | Max poles | Volume | ms |
|---|---|---|---|---|---|---|---|---|
| | ** A · Primitives** | | | | | | | |
| 2 | `box (0,0,0) (2,1.5,1) --name=Block` | ✔️ ran | ✔️ | ✔️ | 0 / 12 | 2 | 3.000000 ✔️ (err 7.2e-08) | 0 |
| 3 | `cylinder (5,1,0) 0.6 1 --name=Can` | ✔️ ran | ✔️ | ✔️ | 0 / 3 | 9 | 1.130973 ✔️ (err 9.2e-09) | 1 |
| 4 | `cone (8,1,0) 0.6 0.3 1 --name=Frustum` | ✔️ ran | ✔️ | ✔️ | 0 / 3 | 9 | 0.659734 ✔️ (err 2.3e-09) | 1 |
| 5 | `sphere (11,1,0.5) 0.5 --name=Ball` | ✔️ ran | ✔️ | ✔️ | 0 / 1 | 5 | 0.523426 🔴 (err 1.7e-04) | 8 |
| 6 | `torus (14,1,0.3) 0.8 0.25 --name=Ring` | ✔️ ran | ✔️ | ✔️ | 0 / 2 | 9 | 0.986960 ✔️ (err 9.0e-12) | 14 |
| | ** B · Sketch to solid** | | | | | | | |
| 8 | `extrude ProfileRect 1 --name=ExtrudedRect` | ✔️ ran | ✔️ | ✔️ | 0 / 12 | 2 | 3.000000 ✔️ (err 2.4e-08) | 0 |
| 10 | `extrude ProfileCircle 1 --name=ExtrudedCircle` | ✔️ ran | ✔️ | ✔️ | 0 / 3 | 9 | 1.130973 ✔️ (err 9.2e-09) | 2 |
| 12 | `revolve RevolveProfile 360 --origin=(7.5,4,0) --axis=(0,1,0) --name=RevolvedRing` | ✔️ ran | ✔️ | ✔️ | 0 / 8 | 9 | 2.356194 ✔️ (err 7.6e-12) | 3 |
| 14 | `revolve HalfRevolveProfile 180 --origin=(9.5,4,0) --axis=(0,1,0) --name=HalfRing` | ✔️ ran | ✔️ | ✔️ | 0 / 12 | 5 | 1.178097 ✔️ (err 3.8e-12) | 3 |
| 17 | `loft LoftFoot LoftHead --name=LoftedFrustum` | ✔️ ran | ✔️ | ✔️ | 0 / 3 | 9 | 0.659734 ✔️ (err 2.2e-09) | 7 |
| 19 | `pipe PipeLine 0.2 --name=StraightPipe` | ✔️ ran | ✔️ | ✔️ | 0 / 3 | 9 | 0.376991 ✔️ (err 3.2e-09) | 6 |
| 21 | `pipe PipeArc 0.2 --name=BentPipe` | ✔️ ran | ✔️ | ✔️ | 1 / 3 | 9 | 0.197395 🔴 (err 2.8e-06) | 24 |
| 24 | `sweep SweepProfile SweepPath --name=SweptRod` | ✔️ ran | ✔️ | ✔️ | 0 / 3 | 9 | 0.251327 ✔️ (err 2.2e-09) | 6 |
| | ** C · Booleans** | | | | | | | |
| 25 | `box (0,12,0) (4,15,2) --name=BoxA` | ✔️ ran | ✔️ | ✔️ | 0 / 12 | 2 | 24.000001 | 5 |
| 26 | `box (2,12,0) (6,15,2) --name=BoxB` | ✔️ ran | ✔️ | ✔️ | 0 / 12 | 2 | 24.000001 | 5 |
| 27 | `boolean union BoxA -- BoxB --keep --name=BoxUnion` | ✔️ ran | ✔️ | ✔️ | 0 / 12 | 2 | 36.000001 ✔️ (err 8.7e-07) | 5 |
| 28 | `boolean subtract BoxA -- BoxB --keep --name=BoxDifference` | ✔️ ran | ✔️ | ✔️ | 0 / 12 | 2 | 12.000000 ✔️ (err 2.9e-07) | 5 |
| 29 | `boolean intersect BoxA -- BoxB --keep --name=BoxCommon` | ✔️ ran | ✔️ | ✔️ | 0 / 12 | 2 | 12.000000 ✔️ (err 2.9e-07) | 5 |
| 30 | `box (6.0,40.0,0) (9.0,42.0,0.6) --name=SlabC` | ✔️ ran | ✔️ | ✔️ | 0 / 12 | 2 | 3.600000 | 5 |
| 31 | `cylinder (7.5,41.0,-0.5) 0.45 2 --name=BoreC` | ✔️ ran | ✔️ | ✔️ | 0 / 3 | 9 | 1.272345 | 6 |
| 32 | `boolean subtract SlabC -- BoreC --name=HoleSlabC` | ✔️ ran | ✔️ | ✔️ | 0 / 15 | 9 | 3.218297 ✔️ (err 7.9e-08) | 30 |
| 33 | `box (13,12,0) (16,14,0.6) --name=Plate` | ✔️ ran | ✔️ | ✔️ | 0 / 12 | 2 | 3.600000 | 9 |
| 34 | `cylinder (14.5,13,0.3) 0.5 1 --name=Boss` | ✔️ ran | ✔️ | ✔️ | 0 / 3 | 9 | 0.785398 | 7 |
| 35 | `boolean union Plate -- Boss --name=BossPlate` | ✔️ ran | ✔️ | ✔️ | 0 / 15 | 9 | 4.149779 ✔️ (err 9.3e-08) | 26 |
| 36 | `box (6,17,0) (8,19,1) --name=Pedestal` | ✔️ ran | ✔️ | ✔️ | 0 / 12 | 2 | 4.000000 | 5 |
| 37 | `sphere (7,18,1) 0.5 --name=Dome` | ✔️ ran | ✔️ | ✔️ | 0 / 1 | 5 | 0.523426 | 12 |
| 38 | `boolean union Pedestal -- Dome --name=DomedPedestal` | ✔️ ran | ✔️ | ✔️ | 0 / 14 | 9 | 4.261570 🔴 (err 2.3e-04) | 273 |
| 39 | `box (10,17,0) (12,19,1) --name=Socket` | ✔️ ran | ✔️ | ✔️ | 0 / 12 | 2 | 4.000000 | 6 |
| 40 | `sphere (11,18,1) 0.5 --name=Scoop` | ✔️ ran | ✔️ | ✔️ | 0 / 1 | 5 | 0.523426 | 12 |
| 41 | `boolean subtract Socket -- Scoop --name=ScoopedSocket` | ✔️ ran | ✔️ | ✔️ | 0 / 14 | 9 | 3.738090 🔴 (err 1.1e-04) | 272 |
| 42 | `cylinder (0,18,0) 0.5 4 --axis=(1,0,0) --name=CrossX` | ✔️ ran | ✔️ | ✔️ | 0 / 3 | 9 | 3.141593 | 8 |
| 43 | `cylinder (2,16,0) 0.3 4 --axis=(0,1,0) --name=CrossY` | ✔️ ran | ✔️ | ✔️ | 0 / 3 | 9 | 1.130973 | 7 |
| 44 | `boolean union CrossX -- CrossY --name=CrossUnequal` | 🔴 refused: > boolean union CrossX -- CrossY --name=CrossUnequal   ✗ boolean union: Unsupported — coincident faces along the ... | — | — | — | — | — | 15 |
| 45 | `cylinder (0,22,0.1) 0.5 4 --axis=(1,0,0) --name=OffsetCrossX` | ✔️ ran | ✔️ | ✔️ | 0 / 3 | 9 | 3.141593 | 7 |
| 46 | `cylinder (2,20,0) 0.3 4 --axis=(0,1,0) --name=OffsetCrossY` | ✔️ ran | ✔️ | ✔️ | 0 / 3 | 9 | 1.130973 | 6 |
| 47 | `boolean union OffsetCrossX -- OffsetCrossY --name=CrossOffset` | ✔️ ran | ✔️ | ✔️ | 4 / 12 | 122 | 4.009803 | 71 |
| 48 | `cylinder (6,22,0) 0.5 4 --axis=(1,0,0) --name=SteinmetzX` | ✔️ ran | ✔️ | ✔️ | 0 / 3 | 9 | 3.141593 | 7 |
| 49 | `cylinder (8,20,0) 0.5 4 --axis=(0,1,0) --name=SteinmetzY` | ✔️ ran | ✔️ | ✔️ | 0 / 3 | 9 | 3.141593 | 7 |
| 50 | `boolean union SteinmetzX -- SteinmetzY --name=SteinmetzUnion` | 🔴 refused: > boolean union SteinmetzX -- SteinmetzY --name=SteinmetzUnion   ✗ boolean union: Unsupported — intersection runs... | — | — | — | — | — | 12 |
| | ** D · Blends (general = beveladd; legacy = fillet / chamfer)** | | | | | | | |
| 51 | `box (0,26,0) (2,27.5,1) --name=BlendBlock` | ✔️ ran | ✔️ | ✔️ | 0 / 12 | 2 | 3.000000 | 6 |
| 52 | `beveladd 0.2 --mode=fillet --targets=48:e0,1,2,3,4,5,6,7,8,9,10,11` | ✔️ ran | ✔️ | ✔️ | 24 / 48 | 41 | 2.856196 | 458 |
| 53 | `box (3,26,0) (5,27.5,1) --name=LegacyFilletBlock` | ✔️ ran | ✔️ | ✔️ | 0 / 12 | 2 | 3.000000 | 6 |
| 54 | `fillet LegacyFilletBlock 0.2 --edges=0,1,2,3,4,5,6,7,8,9,10,11 --name=LegacyR...` | ✔️ ran | ✔️ | 🔴 trace-on-edge ×4 worst 2.83e-01 (edge 24 on face 18) | 0 / 48 | 3 | 2.856172 | 24 |
| 55 | `box (6,26,0) (8,27.5,1) --name=LegacyChamferBlock` | ✔️ ran | ✔️ | ✔️ | 0 / 12 | 2 | 3.000000 | 6 |
| 56 | `chamfer LegacyChamferBlock 0.15 --edges=0 --name=LegacyChamfered` | ✔️ ran | ✔️ | ✔️ | 0 / 15 | 2 | 2.977500 | 7 |
| 57 | `box (12.0,40.0,0) (15.0,42.0,0.6) --name=SlabD` | ✔️ ran | ✔️ | ✔️ | 0 / 12 | 2 | 3.600000 | 6 |
| 58 | `cylinder (13.5,41.0,-0.5) 0.45 2 --name=BoreD` | ✔️ ran | ✔️ | ✔️ | 0 / 3 | 9 | 1.272345 | 7 |
| 59 | `boolean subtract SlabD -- BoreD --name=HoleSlabD` | ✔️ ran | ✔️ | ✔️ | 0 / 15 | 9 | 3.218297 | 32 |
| 60 | `beveladd 0.1 --mode=fillet --targets=55:e9` | ✔️ ran | ✔️ | ✔️ | 2 / 17 | 131 | 3.211928 | 52 |
| | ** E · Face operations — Block / HoleSlab / Can** | | | | | | | |
| 61 | `box (18.0,40.0,0) (20.0,41.5,1) --name=BlockPush` | ✔️ ran | ✔️ | ✔️ | 0 / 12 | 2 | 3.000000 | 7 |
| 62 | `push BlockPush 0.5 --face=1` | ✔️ ran | ✔️ | ✔️ | 0 / 20 | 2 | 4.500000 ✔️ (err 8.7e-08) | 7 |
| 63 | `box (24.0,40.0,0) (27.0,42.0,0.6) --name=SlabPush` | ✔️ ran | ✔️ | ✔️ | 0 / 12 | 2 | 3.600000 | 7 |
| 64 | `cylinder (25.5,41.0,-0.5) 0.45 2 --name=BorePush` | ✔️ ran | ✔️ | ✔️ | 0 / 3 | 9 | 1.272345 | 7 |
| 65 | `boolean subtract SlabPush -- BorePush --name=HoleSlabPush` | ✔️ ran | ✔️ | ✔️ | 0 / 15 | 9 | 3.218297 | 34 |
| 66 | `push HoleSlabPush 0.2 --face=1` | ✔️ ran | ✔️ | ✔️ | 0 / 25 | 9 | 4.291062 ✔️ (err 7.8e-08) | 38 |
| 67 | `cylinder (31.0,41.0,0) 0.6 1 --name=CanPush` | ✔️ ran | ✔️ | ✔️ | 0 / 3 | 9 | 1.130973 | 7 |
| 68 | `push CanPush 0.5 --face=2` | ✔️ ran | ✔️ | ✔️ | 0 / 3 | 9 | 1.696460 ✔️ (err 1.5e-08) | 10 |
| 69 | `box (36.0,40.0,0) (38.0,41.5,1) --name=BlockExtrudefaces` | ✔️ ran | ✔️ | ✔️ | 0 / 12 | 2 | 3.000000 | 7 |
| 70 | `extrudefaces BlockExtrudefaces:f1 0.5` | ✔️ ran | ✔️ | ✔️ | 0 / 20 | 2 | 4.500000 ✔️ (err 9.1e-08) | 6 |
| 71 | `box (42.0,40.0,0) (45.0,42.0,0.6) --name=SlabExtrudefaces` | ✔️ ran | ✔️ | ✔️ | 0 / 12 | 2 | 3.600000 | 6 |
| 72 | `cylinder (43.5,41.0,-0.5) 0.45 2 --name=BoreExtrudefaces` | ✔️ ran | ✔️ | ✔️ | 0 / 3 | 9 | 1.272345 | 8 |
| 73 | `boolean subtract SlabExtrudefaces -- BoreExtrudefaces --name=HoleSlabExtrudef...` | ✔️ ran | ✔️ | ✔️ | 0 / 15 | 9 | 3.218297 | 31 |
| 74 | `extrudefaces HoleSlabExtrudefaces:f1 0.2` | ✔️ ran | ✔️ | ✔️ | 0 / 25 | 9 | 4.291062 ✔️ (err 7.6e-08) | 30 |
| 75 | `cylinder (49.0,41.0,0) 0.6 1 --name=CanExtrudefaces` | ✔️ ran | ✔️ | ✔️ | 0 / 3 | 9 | 1.130973 | 7 |
| 76 | `extrudefaces CanExtrudefaces:f2 0.5` | ✔️ ran | ✔️ | ✔️ | 0 / 3 | 9 | 1.696460 ✔️ (err 1.1e-08) | 9 |
| 77 | `box (54.0,40.0,0) (56.0,41.5,1) --name=BlockTweak` | ✔️ ran | ✔️ | ✔️ | 0 / 12 | 2 | 3.000000 | 10 |
| 78 | `tweak BlockTweak (0,0,0.5) --face=1` | ✔️ ran | ✔️ | ✔️ | 0 / 12 | 2 | 4.500000 ✔️ (err 1.1e-07) | 6 |
| 79 | `box (60.0,40.0,0) (63.0,42.0,0.6) --name=SlabTweak` | ✔️ ran | ✔️ | ✔️ | 0 / 12 | 2 | 3.600000 | 6 |
| 80 | `cylinder (61.5,41.0,-0.5) 0.45 2 --name=BoreTweak` | ✔️ ran | ✔️ | ✔️ | 0 / 3 | 9 | 1.272345 | 7 |
| 81 | `boolean subtract SlabTweak -- BoreTweak --name=HoleSlabTweak` | ✔️ ran | ✔️ | ✔️ | 0 / 15 | 9 | 3.218297 | 38 |
| 82 | `tweak HoleSlabTweak (0,0,0.2) --face=1` | 🔴 refused: > tweak HoleSlabTweak (0,0,0.2) --face=1   ✗ tweak HoleSlabTweak: an edge touching the moved vertices is curved; tw... | — | — | — | — | — | 18 |
| 83 | `cylinder (67.0,41.0,0) 0.6 1 --name=CanTweak` | ✔️ ran | ✔️ | ✔️ | 0 / 3 | 9 | 1.130973 | 7 |
| 84 | `tweak CanTweak (0,0,0.5) --face=2` | ✔️ ran | ✔️ | ✔️ | 0 / 3 | 9 | 1.696460 ✔️ (err 1.4e-08) | 9 |
| 85 | `box (0.0,46.0,0) (2.0,47.5,1) --name=BlockOffsetface` | ✔️ ran | ✔️ | ✔️ | 0 / 12 | 2 | 3.000000 | 6 |
| 86 | `offsetface BlockOffsetface 0.5 --face=1` | ✔️ ran | ✔️ | ✔️ | 0 / 12 | 2 | 4.500000 ✔️ (err 1.1e-07) | 6 |
| 87 | `box (6.0,46.0,0) (9.0,48.0,0.6) --name=SlabOffsetface` | ✔️ ran | ✔️ | ✔️ | 0 / 12 | 2 | 3.600000 | 6 |
| 88 | `cylinder (7.5,47.0,-0.5) 0.45 2 --name=BoreOffsetface` | ✔️ ran | ✔️ | ✔️ | 0 / 3 | 9 | 1.272345 | 10 |
| 89 | `boolean subtract SlabOffsetface -- BoreOffsetface --name=HoleSlabOffsetface` | ✔️ ran | ✔️ | ✔️ | 0 / 15 | 9 | 3.218297 | 36 |
| 90 | `offsetface HoleSlabOffsetface 0.2 --face=1` | 🔴 refused: > offsetface HoleSlabOffsetface 0.2 --face=1   ✗ OffsetFace HoleSlabOffsetface: Unsupported — oblique triangular-... | — | — | — | — | — | 189 |
| 91 | `cylinder (13.0,47.0,0) 0.6 1 --name=CanOffsetface` | ✔️ ran | ✔️ | ✔️ | 0 / 3 | 9 | 1.130973 | 8 |
| 92 | `offsetface CanOffsetface 0.5 --face=1` | 🔴 refused: > offsetface CanOffsetface 0.5 --face=1   ✗ OffsetFace CanOffsetface: Unsupported — oblique triangular-prism offs... | — | — | — | — | — | 33 |
| 93 | `box (18.0,46.0,0) (20.0,47.5,1) --name=BlockScale` | ✔️ ran | ✔️ | ✔️ | 0 / 12 | 2 | 3.000000 | 7 |
| 94 | `scale BlockScale 0.5 --face=1` | ✔️ ran | ✔️ | ✔️ | 0 / 12 | 2 | 1.750000 ✔️ (err 8.8e-08) | 6 |
| 95 | `box (24.0,46.0,0) (27.0,48.0,0.6) --name=SlabScale` | ✔️ ran | ✔️ | ✔️ | 0 / 12 | 2 | 3.600000 | 7 |
| 96 | `cylinder (25.5,47.0,-0.5) 0.45 2 --name=BoreScale` | ✔️ ran | ✔️ | ✔️ | 0 / 3 | 9 | 1.272345 | 8 |
| 97 | `boolean subtract SlabScale -- BoreScale --name=HoleSlabScale` | ✔️ ran | ✔️ | ✔️ | 0 / 15 | 9 | 3.218297 | 34 |
| 98 | `scale HoleSlabScale 0.5 --face=1` | 🔴 refused: > scale HoleSlabScale 0.5 --face=1   ✗ scale HoleSlabScale: transform tweak touches a curved edge | — | — | — | — | — | 11 |
| 99 | `cylinder (31.0,47.0,0) 0.6 1 --name=CanScale` | ✔️ ran | ✔️ | ✔️ | 0 / 3 | 9 | 1.130973 | 7 |
| 100 | `scale CanScale 0.5 --face=2` | 🔴 refused: > scale CanScale 0.5 --face=2   ✗ scale CanScale: transform tweak touches a curved edge | — | — | — | — | — | 6 |
| 101 | `box (36.0,46.0,0) (38.0,47.5,1) --name=BlockRotate` | ✔️ ran | ✔️ | ✔️ | 0 / 12 | 2 | 3.000000 | 6 |
| 102 | `rotate BlockRotate 5 --face=1 --axis=(1,0,0)` | ✔️ ran | ✔️ | ✔️ | 0 / 12 | 2 | 2.994292 | 6 |
| 103 | `box (42.0,46.0,0) (45.0,48.0,0.6) --name=SlabRotate` | ✔️ ran | ✔️ | ✔️ | 0 / 12 | 2 | 3.600000 | 6 |
| 104 | `cylinder (43.5,47.0,-0.5) 0.45 2 --name=BoreRotate` | ✔️ ran | ✔️ | ✔️ | 0 / 3 | 9 | 1.272345 | 8 |
| 105 | `boolean subtract SlabRotate -- BoreRotate --name=HoleSlabRotate` | ✔️ ran | ✔️ | ✔️ | 0 / 15 | 9 | 3.218297 | 30 |
| 106 | `rotate HoleSlabRotate 5 --face=1 --axis=(1,0,0)` | 🔴 refused: > rotate HoleSlabRotate 5 --face=1 --axis=(1,0,0)   ✗ rotate HoleSlabRotate: transform tweak touches a curved edge | — | — | — | — | — | 14 |
| 107 | `cylinder (49.0,47.0,0) 0.6 1 --name=CanRotate` | ✔️ ran | ✔️ | ✔️ | 0 / 3 | 9 | 1.130973 | 8 |
| 108 | `rotate CanRotate 5 --face=2 --axis=(1,0,0)` | 🔴 refused: > rotate CanRotate 5 --face=2 --axis=(1,0,0)   ✗ rotate CanRotate: transform tweak touches a curved edge | — | — | — | — | — | 7 |
| 109 | `box (54.0,46.0,0) (56.0,47.5,1) --name=BlockShell` | ✔️ ran | ✔️ | ✔️ | 0 / 12 | 2 | 3.000000 | 6 |
| 110 | `shell BlockShell 0.1 --face=1` | ✔️ ran | ✔️ | ✔️ | 0 / 24 | 2 | 0.894000 ✔️ (err 1.1e-08) | 9 |
| 111 | `box (60.0,46.0,0) (63.0,48.0,0.6) --name=SlabShell` | ✔️ ran | ✔️ | ✔️ | 0 / 12 | 2 | 3.600000 | 6 |
| 112 | `cylinder (61.5,47.0,-0.5) 0.45 2 --name=BoreShell` | ✔️ ran | ✔️ | ✔️ | 0 / 3 | 9 | 1.272345 | 8 |
| 113 | `boolean subtract SlabShell -- BoreShell --name=HoleSlabShell` | ✔️ ran | ✔️ | ✔️ | 0 / 15 | 9 | 3.218297 | 34 |
| 114 | `shell HoleSlabShell 0.1 --face=1` | 🔴 refused: > shell HoleSlabShell 0.1 --face=1   ✗ Shell HoleSlabShell: Unsupported — concave-prism shell requires a six-edge... | — | — | — | — | — | 22 |
| 115 | `cylinder (67.0,47.0,0) 0.6 1 --name=CanShell` | ✔️ ran | ✔️ | ✔️ | 0 / 3 | 9 | 1.130973 | 8 |
| 116 | `shell CanShell 0.1 --face=2` | 🔴 refused: > shell CanShell 0.1 --face=2   ✗ Shell CanShell: Unsupported — concave-prism shell requires a six-edge orthogona... | — | — | — | — | — | 8 |
| 117 | `box (0.0,52.0,0) (2.0,53.5,1) --name=BlockDraft` | ✔️ ran | ✔️ | ✔️ | 0 / 12 | 2 | 3.000000 | 6 |
| 118 | `draft BlockDraft 5 --face=5` | ✔️ ran | ✔️ | ✔️ | 0 / 12 | 2 | 3.065616 | 6 |
| 119 | `box (6.0,52.0,0) (9.0,54.0,0.6) --name=SlabDraft` | ✔️ ran | ✔️ | ✔️ | 0 / 12 | 2 | 3.600000 | 6 |
| 120 | `cylinder (7.5,53.0,-0.5) 0.45 2 --name=BoreDraft` | ✔️ ran | ✔️ | ✔️ | 0 / 3 | 9 | 1.272345 | 8 |
| 121 | `boolean subtract SlabDraft -- BoreDraft --name=HoleSlabDraft` | ✔️ ran | ✔️ | ✔️ | 0 / 15 | 9 | 3.218297 | 32 |
| 122 | `draft HoleSlabDraft 5 --face=5` | 🔴 refused: > draft HoleSlabDraft 5 --face=5   ✗ Draft HoleSlabDraft: Unsupported — exact draft currently requires a canonica... | — | — | — | — | — | 21 |
| 123 | `cylinder (13.0,53.0,0) 0.6 1 --name=CanDraft` | ✔️ ran | ✔️ | ✔️ | 0 / 3 | 9 | 1.130973 | 8 |
| 124 | `draft CanDraft 5 --face=-1` | 🔴 refused: > draft CanDraft 5 --face=-1   ✗ Draft CanDraft: Unsupported — exact draft currently requires a canonical box or ... | — | — | — | — | — | 7 |
| 125 | `box (18.0,52.0,0) (20.0,53.5,1) --name=BlockExtendface` | ✔️ ran | ✔️ | ✔️ | 0 / 12 | 2 | 3.000000 | 6 |
| 126 | `extendface BlockExtendface 0.2 --face=1` | ✔️ ran | ✔️ | ✔️ | 0 / 12 | 2 | 4.560000 | 6 |
| 127 | `box (24.0,52.0,0) (27.0,54.0,0.6) --name=SlabExtendface` | ✔️ ran | ✔️ | ✔️ | 0 / 12 | 2 | 3.600000 | 6 |
| 128 | `cylinder (25.5,53.0,-0.5) 0.45 2 --name=BoreExtendface` | ✔️ ran | ✔️ | ✔️ | 0 / 3 | 9 | 1.272345 | 8 |
| 129 | `boolean subtract SlabExtendface -- BoreExtendface --name=HoleSlabExtendface` | ✔️ ran | ✔️ | ✔️ | 0 / 15 | 9 | 3.218297 | 31 |
| 130 | `extendface HoleSlabExtendface 0.2 --face=1` | 🔴 refused: > extendface HoleSlabExtendface 0.2 --face=1   ✗ ExtendFace HoleSlabExtendface: Unsupported — exact face extensio... | — | — | — | — | — | 12 |
| 131 | `cylinder (31.0,53.0,0) 0.6 1 --name=CanExtendface` | ✔️ ran | ✔️ | ✔️ | 0 / 3 | 9 | 1.130973 | 8 |
| 132 | `extendface CanExtendface 0.2 --face=1` | 🔴 refused: > extendface CanExtendface 0.2 --face=1   ✗ ExtendFace CanExtendface: Unsupported — exact face extension currentl... | — | — | — | — | — | 6 |
| 133 | `box (36.0,52.0,0) (38.0,53.5,1) --name=BlockTrimface` | ✔️ ran | ✔️ | ✔️ | 0 / 12 | 2 | 3.000000 | 5 |
| 134 | `trimface BlockTrimface 0.2 --face=1` | ✔️ ran | ✔️ | ✔️ | 0 / 12 | 2 | 1.760000 | 6 |
| 135 | `box (42.0,52.0,0) (45.0,54.0,0.6) --name=SlabTrimface` | ✔️ ran | ✔️ | ✔️ | 0 / 12 | 2 | 3.600000 | 6 |
| 136 | `cylinder (43.5,53.0,-0.5) 0.45 2 --name=BoreTrimface` | ✔️ ran | ✔️ | ✔️ | 0 / 3 | 9 | 1.272345 | 8 |
| 137 | `boolean subtract SlabTrimface -- BoreTrimface --name=HoleSlabTrimface` | ✔️ ran | ✔️ | ✔️ | 0 / 15 | 9 | 3.218297 | 31 |
| 138 | `trimface HoleSlabTrimface 0.2 --face=1` | 🔴 refused: > trimface HoleSlabTrimface 0.2 --face=1   ✗ TrimFace HoleSlabTrimface: Unsupported — exact face trim currently r... | — | — | — | — | — | 11 |
| 139 | `cylinder (49.0,53.0,0) 0.6 1 --name=CanTrimface` | ✔️ ran | ✔️ | ✔️ | 0 / 3 | 9 | 1.130973 | 8 |
| 140 | `trimface CanTrimface 0.2 --face=2` | 🔴 refused: > trimface CanTrimface 0.2 --face=2   ✗ TrimFace CanTrimface: Unsupported — exact face trim currently requires a ... | — | — | — | — | — | 6 |
| 141 | `box (54.0,52.0,0) (56.0,53.5,1) --name=BlockDeleteface` | ✔️ ran | ✔️ | ✔️ | 0 / 12 | 2 | 3.000000 | 6 |
| 142 | `deleteface BlockDeleteface --face=1` | ✔️ ran | sheet | ✔️ | 0 / 12 | 2 | — | 6 |
| 143 | `box (60.0,52.0,0) (63.0,54.0,0.6) --name=SlabDeleteface` | ✔️ ran | ✔️ | ✔️ | 0 / 12 | 2 | 3.600000 | 6 |
| 144 | `cylinder (61.5,53.0,-0.5) 0.45 2 --name=BoreDeleteface` | ✔️ ran | ✔️ | ✔️ | 0 / 3 | 9 | 1.272345 | 8 |
| 145 | `boolean subtract SlabDeleteface -- BoreDeleteface --name=HoleSlabDeleteface` | ✔️ ran | ✔️ | ✔️ | 0 / 15 | 9 | 3.218297 | 32 |
| 146 | `deleteface HoleSlabDeleteface --face=1` | 🔴 refused: > deleteface HoleSlabDeleteface --face=1   ✗ DeleteFace HoleSlabDeleteface: Unsupported — face deletion currently... | — | — | — | — | — | 11 |
| 147 | `cylinder (67.0,53.0,0) 0.6 1 --name=CanDeleteface` | ✔️ ran | ✔️ | ✔️ | 0 / 3 | 9 | 1.130973 | 7 |
| 148 | `deleteface CanDeleteface --face=2` | 🔴 refused: > deleteface CanDeleteface --face=2   ✗ DeleteFace CanDeleteface: Unsupported — face deletion currently requires ... | — | — | — | — | — | 7 |
| 149 | `box (0.0,58.0,0) (2.0,59.5,1) --name=BlockHeal` | ✔️ ran | ✔️ | ✔️ | 0 / 12 | 2 | 3.000000 | 6 |
| 150 | `heal BlockHeal` | ✔️ ran | ✔️ | ✔️ | 0 / 12 | 2 | 3.000000 ✔️ (err 7.2e-08) | 8 |
| 151 | `box (6.0,58.0,0) (9.0,60.0,0.6) --name=SlabHeal` | ✔️ ran | ✔️ | ✔️ | 0 / 12 | 2 | 3.600000 | 7 |
| 152 | `cylinder (7.5,59.0,-0.5) 0.45 2 --name=BoreHeal` | ✔️ ran | ✔️ | ✔️ | 0 / 3 | 9 | 1.272345 | 8 |
| 153 | `boolean subtract SlabHeal -- BoreHeal --name=HoleSlabHeal` | ✔️ ran | ✔️ | ✔️ | 0 / 15 | 9 | 3.218297 | 30 |
| 154 | `heal HoleSlabHeal` | ✔️ ran | ✔️ | ✔️ | 0 / 15 | 9 | 3.218297 ✔️ (err 8.1e-08) | 30 |
| 155 | `cylinder (13.0,59.0,0) 0.6 1 --name=CanHeal` | ✔️ ran | ✔️ | ✔️ | 0 / 3 | 9 | 1.130973 | 8 |
| 156 | `heal CanHeal` | ✔️ ran | ✔️ | ✔️ | 0 / 3 | 9 | 1.130973 ✔️ (err 9.6e-09) | 9 |
| | ** F · Patterning** | | | | | | | |
| 157 | `box (18.0,58.0,0) (21.0,60.0,0.6) --name=SlabMirror` | ✔️ ran | ✔️ | ✔️ | 0 / 12 | 2 | 3.600000 | 6 |
| 158 | `cylinder (19.5,59.0,-0.5) 0.45 2 --name=BoreMirror` | ✔️ ran | ✔️ | ✔️ | 0 / 3 | 9 | 1.272345 | 8 |
| 159 | `boolean subtract SlabMirror -- BoreMirror --name=HoleSlabMirror` | ✔️ ran | ✔️ | ✔️ | 0 / 15 | 9 | 3.218297 | 33 |
| 160 | `mirror HoleSlabMirror --across=yz --name=MirroredSlab` | ✔️ ran | ✔️ | ✔️ | 0 / 15 | 9 | 3.218297 ✔️ (err 8.3e-08) | 7 |
| 161 | `box (24.0,58.0,0) (26.0,59.5,1) --name=BlockArray` | ✔️ ran | ✔️ | ✔️ | 0 / 12 | 2 | 3.000000 | 7 |
| 162 | `array BlockArray --count=3 --step=(0,0,2) --name=BlockRun` | ✔️ ran | ✔️ | ✔️ | 0 / 12 | 2 | 3.000000 ✔️ (err 6.7e-08) | 7 |
| | ** G · Persistent naming (edge index stability)** | | | | | | | |
| 163 | `box (30.0,58.0,0) (32.0,59.5,1) --name=BlockNamingFillet` | ✔️ ran | ✔️ | ✔️ | 0 / 12 | 2 | 3.000000 | 7 |
| 164 | `beveladd 0.2 --mode=fillet --targets=139:e0` | ✔️ ran | ✔️ | ✔️ | 4 / 15 | 97 | 2.982832 | 94 |
| 165 | `box (36.0,58.0,0) (39.0,60.0,0.6) --name=NamingSlab` | ✔️ ran | ✔️ | ✔️ | 0 / 12 | 2 | 3.600000 | 7 |
| 166 | `cylinder (37.5,59.0,-0.5) 0.45 2 --name=NamingBore` | ✔️ ran | ✔️ | ✔️ | 0 / 3 | 9 | 1.272345 | 8 |
| 167 | `boolean subtract NamingSlab -- NamingBore --name=NamingHoleSlab` | ✔️ ran | ✔️ | ✔️ | 0 / 15 | 9 | 3.218297 ✔️ (err 6.1e-08) | 33 |
| 168 | `box (42.0,58.0,0) (44.0,59.5,1) --name=BlockNamingTweak` | ✔️ ran | ✔️ | ✔️ | 0 / 12 | 2 | 3.000000 | 7 |
| 169 | `tweak BlockNamingTweak (0,0,0.5) --face=1` | ✔️ ran | ✔️ | ✔️ | 0 / 12 | 2 | 4.500000 ✔️ (err 1.1e-07) | 6 |
| 170 | `box (48.0,58.0,0) (50.0,59.5,1) --name=BlockNamingPush` | ✔️ ran | ✔️ | ✔️ | 0 / 12 | 2 | 3.000000 | 6 |
| 171 | `push BlockNamingPush 0.5 --face=1` | ✔️ ran | ✔️ | ✔️ | 0 / 20 | 2 | 4.500000 ✔️ (err 7.6e-08) | 6 |
| | ** H · Regeneration (edit a sketch under a derived, bevelled body)** | | | | | | | |
| 173 | `extrude DriverRect 1 --name=DrivenBlock` | ✔️ ran | ✔️ | ✔️ | 0 / 12 | 2 | 3.000000 ✔️ (err 2.7e-08) | 7 |
| 174 | `beveladd 0.2 --mode=fillet --targets=148:e0` | ✔️ ran | ✔️ | ✔️ | 4 / 15 | 97 | 2.982831 | 102 |

**Persistent naming** — edges that survived an operation unchanged (same midpoint ≤ 1e-9), and whether their index did:

| Operation | Body | Edges before → after | Survived | Same index | Renumbered |
|---|---|---|---|---|---|
| `beveladd 0.2 --mode=fillet --targets=139:e0` | BlockNamingFillet | 12 → 15 | 7 | 5 | 2 |
| `boolean subtract NamingSlab -- NamingBore --name=NamingHoleSlab` | NamingSlab | 12 → 15 | 12 | 4 | 8 |
| `tweak BlockNamingTweak (0,0,0.5) --face=1` | BlockNamingTweak | 12 → 12 | 4 | 4 | 0 |
| `push BlockNamingPush 0.5 --face=1` | BlockNamingPush | 12 → 20 | 12 | 12 | 0 |

176 command row(s), 19 refused, 1 closed-but-defective, 4 volume miss(es)
