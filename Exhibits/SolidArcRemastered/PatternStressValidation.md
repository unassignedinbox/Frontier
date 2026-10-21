# Pattern stress validation (arrays → Boolean chains → blends)

**🟢 NO WRONG ANSWERS** · 68 rows · 🟢 68 correct · ⚠️ 0 refused (honest limit) · 🔴 0 wrong · 628 modeller operations · 716.6 s

> 📝 Every row: modeller array (ArrayBody / CircularArrayBody) → Boolean chain → optional BlendEdges. Built rows must be audit-sound with the expected genus and lump count, match the hand-derived closed form to 10⁻⁹·max(1, V*), classify 300 random points like the analytic CSG oracle (points nearer a primitive boundary than max(10⁻³, 1.5 × blend size) are skipped) and tessellate watertight. A refusal is an honest limit (⚠️); a built wrong body is 🔴.

| | Code | Scene | Ops | F / E / V | Genus | Lumps | V | V* | \|ΔV\| | Oracle wrong | Triangles | ms | Finding |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 🟢 | L1a | Plate − 1 hole (r 0.3) | 2 | 7 / 15 / 10 | 1 | 1 | 71.7172566612 | 71.7172566612 | 8.5e-14 | 0 / 300 | 358 | 1473 | – |
| 🟢 | L1b | Plate − linear array 4 holes | 5 | 10 / 24 / 16 | 4 | 1 | 70.8690266447 | 70.8690266447 | 8.5e-14 | 0 / 300 | 1240 | 3300 | – |
| 🟢 | L1c | Plate − linear array 10 holes | 11 | 16 / 42 / 28 | 10 | 1 | 69.1725666118 | 69.1725666118 | 1.1e-13 | 0 / 300 | 2978 | 7288 | – |
| 🟢 | L2 | Plate − 10 × 4 grid array (40 holes r 0.25, 40 Booleans) | 41 | 46 / 132 / 88 | 40 | 1 | 64.1460183660 | 64.1460183660 | 8.5e-14 | 0 / 300 | 12544 | 44249 | – |
| 🟢 | L3 | Tree order: ∪ 10 disjoint tools (multi-lump) then one −; must equal L1c | 11 | 16 / 42 / 28 | 10 | 1 | 69.1725666118 | 69.1725666118 | 1.1e-13 | 0 / 300 | 2978 | 9011 | cluster lumps 10 · F 16 vs 16 · |ΔV| vs sequential 0.000000 |
| 🟢 | L4 | Plate − 5 overlapping holes r 0.4 at pitch 0.5 (slot with cusps) | 6 | 14 / 36 / 24 | 1 | 1 | 70.0086770134 | 70.0086770134 | 1.1e-13 | 0 / 300 | 1104 | 3238 | – |
| 🟢 | L5a | Plate ∪ linear array 6 bosses (embedded) | 7 | 18 / 30 / 20 | 0 | 1 | 74.4127431580 | 74.4127431580 | 2.8e-14 | 0 / 300 | 1460 | 3590 | – |
| 🟢 | L5b | Plate ∪ 6 bosses touching the top face (coplanar contact) | 7 | 18 / 30 / 20 | 0 | 1 | 74.4127431580 | 74.4127431580 | 2.8e-14 | 0 / 300 | 1460 | 3339 | – |
| 🟢 | L6 | Plate ∪ 5 ribs flush with both sides (coplanar merges) | 6 | 26 / 72 / 48 | 0 | 1 | 78.3000000000 | 78.3000000000 | 4.3e-14 | 0 / 300 | 196 | 3486 | – |
| 🟢 | L7 | 6 unit blocks at pitch 1 (face-to-face) ∪ → one 6 × 1 × 1 block | 6 | 6 / 12 / 8 | 0 | 1 | 6.0000000000 | 6.0000000000 | 0.0e+00 | 0 / 300 | 68 | 615 | 6 faces (6 expected) |
| 🟢 | C1a | Disc − circular array 6 holes (r 0.2) | 7 | 9 / 21 / 14 | 6 | 1 | 16.5122109873 | 16.5122109873 | 3.6e-15 | 0 / 300 | 2534 | 2943 | – |
| 🟢 | C1b | Disc − circular array 12 holes | 13 | 15 / 39 / 26 | 12 | 1 | 16.0598216452 | 16.0598216452 | 0.0e+00 | 0 / 300 | 4388 | 5472 | – |
| 🟢 | C1c | Disc − circular array 24 holes | 25 | 27 / 75 / 50 | 24 | 1 | 15.1550429609 | 15.1550429609 | 8.9e-15 | 0 / 300 | 7098 | 12744 | – |
| 🟢 | C2 | Disc ∪ circular array 8 bosses r 0.35 | 9 | 19 / 27 / 18 | 0 | 1 | 18.8118568097 | 18.8118568097 | 0.0e+00 | 0 / 300 | 2346 | 3036 | – |
| 🟢 | C3a | Hub ∪ circular array 6 spokes (oblique planes) | 7 | 33 / 78 / 52 | 0 | 1 | 5.3056564334 | 5.3056564334 | 2.7e-15 | 0 / 300 | 586 | 2331 | – |
| 🟢 | C3b | Hub ∪ circular array 12 spokes (overlapping inside the hub) | 13 | 63 / 150 / 100 | 0 | 1 | 7.4697202132 | 7.4697202132 | 1.8e-15 | 0 / 300 | 566 | 4869 | – |
| 🟢 | C4 | Disc − 12 rim scallops r 0.5 | 13 | 26 / 72 / 48 | 0 | 1 | 14.2372365411 | 14.2372365411 | 3.6e-15 | 0 / 300 | 2604 | 3875 | – |
| 🟢 | C5 | Ring of 12 overlapping cylinders (closes on itself) | 12 | 26 / 72 / 48 | 1 | 1 | 6.3801919574 | 6.3801919574 | 1.5e-14 | 0 / 300 | 2978 | 5269 | – |
| 🟢 | C6 | Disc − 8 radial blind bores (traced cylinder ∩ cylinder) | 9 | 19 / 29 / 19 | 0 | 1 | 26.2653910322 | 26.2653910322 | 2.1e-14 | 0 / 300 | 6602 | 38325 | – |
| 🟢 | M1 | Plate ∪ 6 bosses − 6 coaxial holes r 0.2 (12 Booleans) | 14 | 24 / 48 / 32 | 6 | 1 | 73.0555751316 | 73.0555751316 | 2.8e-14 | 0 / 300 | 4520 | 6675 | – |
| 🟢 | M2 | Plate ∪ 6 bosses − 6 holes of the SAME radius (coincident cylinders) | 14 | 12 / 30 / 20 | 6 | 1 | 68.9840710526 | 68.9840710526 | 7.1e-14 | 0 / 300 | 2146 | 5278 | – |
| 🟢 | M3 | Order independence: C1b in shuffled order = in order | 13 | 15 / 39 / 26 | 12 | 1 | 16.0598216452 | 16.0598216452 | 0.0e+00 | 0 / 300 | 4394 | 7336 | F/E 15/39 vs 15/39 · |ΔV| 0.000000 |
| 🟢 | M4 | Plate ∩ (∪ of 4 disjoint pins) → 4 lumps | 5 | 12 / 12 / 8 | 0 | 4 | 3.1415926536 | 3.1415926536 | 1.8e-15 | 0 / 300 | 1080 | 1077 | – |
| 🟢 | M5 | Block − 3 × 3 crossing bores, equal r 0.3 (9 Steinmetz crossings, 18 singular points) | 8 | 30 / 84 / 38 | 15 | 1 | 26.5101598682 | 26.5101598682 | 6.0e-14 | 0 / 300 | 8778 | 9276 | – |
| 🟢 | M6 | Block − 3 × 3 crossing bores, r 0.3 × r 0.2 (traced crossings) | 8 | 21 / 57 / 38 | 15 | 1 | 27.7375404479 | 27.7375404479 | 4.6e-14 | 0 / 300 | 8848 | 22166 | – |
| 🟢 | B1 | L1b + fillet 4 hole rims (top) r 0.1 | 6 | 14 / 32 / 20 | 4 | 1 | 70.8516413052 | 70.8516413052 | 5.7e-14 | 0 / 300 | 1940 | 3768 | – |
| 🟢 | B2 | L1b + chamfer 4 hole rims c 0.1 | 6 | 14 / 32 / 20 | 4 | 1 | 70.8271387427 | 70.8271387427 | 4.3e-14 | 0 / 300 | 1320 | 3678 | – |
| 🟢 | B3 | L1b + fillet all 8 rims (top + bottom) r 0.1 | 6 | 18 / 40 / 24 | 4 | 1 | 70.8342559657 | 70.8342559657 | 2.8e-14 | 0 / 300 | 2468 | 3933 | – |
| 🟢 | B4 | C1b + fillet 12 hole rims r 0.05 | 14 | 27 / 63 / 38 | 12 | 1 | 16.0512795682 | 16.0512795682 | 7.1e-15 | 0 / 300 | 6522 | 6552 | – |
| 🟢 | B5 | L2 (40 holes) + fillet 40 rims r 0.05 in one operation | 42 | 86 / 212 / 128 | 40 | 1 | 64.1108028606 | 64.1108028606 | 1.1e-13 | 0 / 300 | 19850 | 48301 | – |
| 🟢 | B6 | L5a + fillet 6 boss top rims r 0.1 (convex) | 8 | 24 / 42 / 26 | 0 | 1 | 74.3821890767 | 74.3821890767 | 7.1e-14 | 0 / 300 | 2430 | 5959 | – |
| 🟢 | B7 | L5a + fillet 6 boss feet r 0.1 (concave, adds material) | 8 | 24 / 42 / 26 | 0 | 1 | 74.4469114658 | 74.4469114658 | 7.1e-14 | 0 / 300 | 2382 | 5382 | – |
| 🟢 | B8 | L5a + chamfer 6 boss top rims c 0.1 | 8 | 24 / 42 / 26 | 0 | 1 | 74.3436281196 | 74.3436281196 | 1.1e-13 | 0 / 300 | 1508 | 3977 | – |
| 🟢 | B9 | L6 + fillet 10 rib top edges r 0.1 | 7 | 36 / 102 / 68 | 0 | 1 | 78.1712388980 | 78.1712388980 | 9.9e-14 | 0 / 300 | 4196 | 5465 | – |
| 🟢 | B10 | L6 + fillet 10 rib foot edges r 0.1 (concave) | 7 | 36 / 102 / 68 | 0 | 1 | 78.4287611020 | 78.4287611020 | 9.9e-14 | 0 / 300 | 5020 | 5487 | – |
| 🟢 | B11 | L6 + chamfer 10 rib top edges c 0.1 | 7 | 36 / 102 / 68 | 0 | 1 | 78.0000000000 | 78.0000000000 | 7.1e-14 | 0 / 300 | 256 | 4872 | – |
| 🟢 | B12 | L6 + fillet 10 top AND 10 foot rib edges at once | 7 | 46 / 132 / 88 | 0 | 1 | 78.3000000000 | 78.3000000000 | 1.6e-13 | 0 / 300 | 9898 | 6967 | – |
| 🟢 | B13 | L7 merged block + fillet all edges r 0.2 (needs maximal faces) | 7 | 26 / 48 / 24 | 0 | 1 | 5.7360235234 | 5.7360235234 | 5.3e-15 | 0 / 300 | 4548 | 1708 | – |
| 🟢 | B14 | M1 + fillet boss rims AND hole rims on the boss tops r 0.05 | 15 | 36 / 72 / 44 | 6 | 1 | 73.0434396836 | 73.0434396836 | 5.7e-14 | 0 / 300 | 6944 | 7531 | – |
| 🟢 | B15 | L1c + fillet the 12 outer plate edges r 0.2 | 12 | 36 / 78 / 44 | 10 | 1 | 68.5308909028 | 68.5308909028 | 7.1e-14 | 0 / 300 | 11436 | 8937 | – |
| 🟢 | B16 | B1 then a second blend: outer plate edges r 0.2 (blend after blend) | 7 | 34 / 68 / 36 | 4 | 1 | 70.2099655962 | 70.2099655962 | 1.1e-13 | 0 / 300 | 10360 | 5574 | – |
| 🟢 | B17 | Blend then Boolean: rounded plate (r 0.2) − linear array 4 holes | 6 | 30 / 60 / 32 | 4 | 1 | 70.2273509357 | 70.2273509357 | 1.3e-13 | 0 / 300 | 9702 | 5586 | – |
| 🟢 | B18 | M5 cross-bores + fillet the 12 bore entry rims r 0.05 | 9 | 42 / 108 / 50 | 15 | 1 | 26.4975726420 | 26.4975726420 | 4.3e-14 | 0 / 300 | 13608 | 10664 | – |
| 🟢 | B19 | C2 + fillet 8 boss feet r 0.08 (concave rims on a disc) | 10 | 27 / 43 / 26 | 0 | 1 | 18.8372534910 | 18.8372534910 | 1.8e-14 | 0 / 300 | 4772 | 3448 | – |
| 🟢 | B20 | L1b + fillet 4 rims r 0.5 — neighbouring blends would overlap (must refuse) | 6 | – | – | – | – | 70.313919 | – | – | – | 1236 | Fillet on 4 edges: Blend: neighbouring blends overlap: E118 passes through F69 (self-intersection; removal is 🚧) |
| 🟢 | B21 | L1b + fillet 4 rims r 0.19 — footprints 0.02 apart (legal, closed form) | 6 | 14 / 32 / 20 | 4 | 1 | 70.8023513616 | 70.8023513616 | 8.5e-14 | 0 / 300 | 1792 | 3546 | – |
| 🟢 | B22 | L1b + fillet 4 rims r 0.21 — footprints overlap by 0.02 (must refuse) | 6 | – | – | – | – | 70.786513 | – | – | – | 1252 | Fillet on 4 edges: Blend: neighbouring blends overlap: E118 passes through F69 (self-intersection; removal is 🚧) |
| 🟢 | T1 | Plate − stadium slot (box + 2 tangent cylinders) + fillet the top rim r 0.1 | 7 | 14 / 32 / 20 | 1 | 1 | 68.2744823105 | 68.2744823105 | 7.1e-14 | 0 / 300 | 1554 | 2076 | 4 edges blended |
| 🟢 | T2 | Stadium slot + chamfer the top rim c 0.1 (planes and cones, tangent joints) | 7 | 14 / 32 / 20 | 1 | 1 | 68.2437316073 | 68.2437316073 | 8.5e-14 | 0 / 300 | 444 | 1950 | 4 edges blended |
| 🟢 | T3 | Stadium slot + fillet top AND bottom rims r 0.1 in one operation | 7 | 18 / 40 / 24 | 1 | 1 | 68.2516194456 | 68.2516194456 | 7.1e-14 | 0 / 300 | 2870 | 2342 | 8 edges blended |
| 🟢 | T4 | Block, fillet 4 vertical edges r 0.5, then fillet the top loop r 0.2 (lines + quarter tori) | 2 | 18 / 40 / 24 | 0 | 1 | 7.6921673980 | 7.6921673980 | 3.6e-15 | 0 / 300 | 3244 | 1067 | 8 edges blended |
| 🟢 | T5 | Block, fillet 4 vertical edges r 0.5, then chamfer the top loop c 0.1 (planes + cones) | 2 | 18 / 40 / 24 | 0 | 1 | 7.7307373977 | 7.7307373977 | 4.4e-15 | 0 / 300 | 916 | 968 | 8 edges blended |
| 🟢 | T7 | Stadium slot + fillet r 0.1 seeded with ONE rim line, tangent chain on (must equal T1) | 7 | 14 / 32 / 20 | 1 | 1 | 68.2744823105 | 68.2744823105 | 7.1e-14 | 0 / 300 | 1554 | 2042 | 1 seed(s) → 4 edges along the tangent chain |
| 🟢 | T8 | Rounded block + chamfer c 0.1 seeded with ONE top line, tangent chain on (must equal T5) | 2 | 18 / 40 / 24 | 0 | 1 | 7.7307373977 | 7.7307373977 | 5.3e-15 | 0 / 300 | 916 | 966 | 1 seed(s) → 8 edges along the tangent chain |
| 🟢 | T6 | Plate ∪ boss 2 × 1, round the boss verticals r 0.3, then fillet the concave foot loop r 0.1 | 3 | 23 / 52 / 32 | 0 | 1 | 72.9734436704 | 72.9734436704 | 7.1e-14 | 0 / 300 | 802 | 1879 | 8 edges blended |
| 🟢 | P1 | L4 slot + fillet every top rim arc r 0.05 (arcs meet at cusps) | 7 | 22 / 52 / 32 | 1 | 1 | 70.0049838159 | – | – | 0 / 300 | 1840 | 4112 | 8 edges blended |
| 🟢 | P2 | C4 scallops + fillet every top edge r 0.05 | 14 | 50 / 120 / 72 | 0 | 1 | 14.2240362149 | – | – | 0 / 300 | 6184 | 25627 | 24 edges blended |
| 🟢 | P3 | C4 scallops + chamfer every top edge c 0.05 | 14 | 50 / 120 / 72 | 0 | 1 | 14.2067600052 | – | – | 0 / 300 | 2682 | 5476 | 24 edges blended |
| 🟢 | P4 | C3a spokes + fillet the 12 vertical spoke–hub edges r 0.05 (concave plane ∩ cylinder) | 8 | 45 / 114 / 76 | 0 | 1 | 5.3081709032 | – | – | 0 / 300 | 1736 | 3134 | – |
| 🟢 | P5 | C3a spokes + fillet the spoke top edges r 0.05 (lines ending on the hub) | 8 | 45 / 114 / 76 | 0 | 1 | 5.2927176861 | – | – | 0 / 300 | 5528 | 4197 | – |
| 🟢 | P6 | C5 ring + fillet every top edge r 0.05 (convex outer, concave inner cusps) | 13 | 50 / 120 / 72 | 1 | 1 | 6.3638934369 | – | – | 0 / 300 | 8220 | 8707 | 24 edges blended |
| 🟢 | P7 | C6 radial bores + fillet the 8 traced bore mouths r 0.03 | 10 | 28 / 47 / 28 | 0 | 1 | 26.2631980227 | – | – | 0 / 300 | 8678 | 176094 | 9 edges blended |
| 🟢 | P8 | M5 equal cross-bores + fillet the crossing edges r 0.03 (elliptical sections) | 9 | 66 / 120 / 38 | 15 | 1 | 26.5075467943 | – | – | 0 / 300 | 18196 | 48642 | 36 edges blended |
| 🟢 | P9 | L5b touching bosses + fillet 6 foot circles r 0.1 | 8 | 24 / 42 / 26 | 0 | 1 | 74.4469114658 | 74.4469114658 | 7.1e-14 | 0 / 300 | 2382 | 3835 | – |
| 🟢 | V1 | L block (prism) + fillet all 18 edges r 0.1 (2 setback vertices: 2 convex + 1 concave blend) | 2 | 48 / 102 / 56 | 0 | 1 | 4.4561897029 | – | – | 0 / 300 | 8488 | 38988 | – |
| 🟢 | V2 | Square pyramid (prism ∩ prism) + fillet all 8 edges r 0.1 (degree-4 apex → setback) | 4 | 25 / 52 / 29 | 0 | 1 | 1.2416687741 | – | – | 0 / 300 | 5936 | 16427 | – |
| 🟢 | O1 | Thin plate t 0.2 − 4 arrayed drills + fillet the front top edge r 0.3 (cliff overflow onto the bottom face) | 6 | 10 / 24 / 16 | 4 | 1 | 1.4244731877 | 1.4244731877 | 1.1e-15 | 0 / 300 | 1534 | 1968 | – |
| 🟢 | O2 | Thin plate t 0.2 − 4 arrayed drills + chamfer the front top edge c 0.3 (cliff overflow, plane trimmed by the bottom) | 6 | 10 / 24 / 16 | 4 | 1 | 1.3394690351 | 1.3394690351 | 8.9e-16 | 0 / 300 | 742 | 1984 | – |
