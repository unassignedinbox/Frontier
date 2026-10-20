# SolidArcRemastered — TopologyValidation

## A. Specimens (audit, counts, genus, exact volume)

| | Specimen | V E F L S G (measured / expected) | Volume | Relative error | Audit |
|---|---|---|---|---|---|
| 🟢 | Block | 8 12 6 6 1 0 / 8 12 6 6 1 0 | 24 | 1.48e-16 | sound |
| 🟢 | HexagonalPrism | 12 18 8 8 1 0 / 12 18 8 8 1 0 | 31.1769145362 | 7.98e-16 | sound |
| 🟢 | Cylinder | 2 3 3 3 1 0 / 2 3 3 3 1 0 | 21.2057504117 | 1.68e-16 | sound |
| 🟢 | Cone | 2 2 2 2 1 0 / 2 2 2 2 1 0 | 12.5663706144 | 1.41e-16 | sound |
| 🟢 | Sphere | 2 1 1 1 1 0 / 2 1 1 1 1 0 | 33.5103216383 | 0.00e+00 | sound |
| 🟢 | Torus | 1 2 1 1 1 1 / 1 2 1 1 1 1 | 59.2176264065 | 7.20e-16 | sound |
| 🟢 | PerforatedBlock | 16 24 10 12 1 1 / 16 24 10 12 1 1 | 42 | 6.77e-16 | sound |
| 🟢 | HollowBlock | 16 24 12 12 2 0 / 16 24 12 12 2 0 | 114 | 1.37e-15 | sound |
| 🟢 | TolerantBlock | 8 12 6 6 1 0 / 8 12 6 6 1 0 | 24 | 1.48e-16 | sound |

## B. Teardown to an empty partition (topology audit after every edit)

| | Specimen | Edits | Unsound after edit | Records left | Edit sequence (runs condensed) |
|---|---|---|---|---|---|
| 🟢 | Block | 13 | 0 | 0 | KEF KEV KVFS |
| 🟢 | HexagonalPrism | 19 | 0 | 0 | KEF KEV KVFS |
| 🟢 | Cylinder | 4 | 0 | 0 | KEF KEV KVFS |
| 🟢 | Cone | 3 | 0 | 0 | KEF KEV KVFS |
| 🟢 | Sphere | 2 | 0 | 0 | KEV KVFS |
| 🟢 | Torus | 4 | 0 | 0 | KEMR MFKRH KEF KVFS |
| 🟢 | PerforatedBlock | 28 | 0 | 0 | KEF KEV MFKRH KEF KEV MEKR KEV KVFS |
| 🟢 | TolerantBlock | 13 | 0 | 0 | KEF KEV KVFS |
| 🟢 | HollowBlock (DetachShell, then both) | 26 | 0 | 0 | KEF KEV KVFS / KEF KEV KVFS |

## C. Token authentication

| | Check | Result |
|---|---|---|
| 🟢 | KEF reclaims the edge; the old token no longer authenticates | rejected |
| 🟢 | Recycled slot 3 gets generation 1; old token still rejected | E3·1 |

## D. Shell reversal

| | Check | Result |
|---|---|---|
| 🟢 | Once: volume 6.28e+00 becomes -6.28e+00; audit flags the inside-out solid region (G6), topology stays valid | G6 R1: solid region volume is not positive |
| 🟢 | Twice: identity, audit sound | sound |

## E. Tolerant modelling

| | Check | Result |
|---|---|---|
| 🟢 | Vertex displaced 4e-6 with NO local precision: audit refuses (G1) | 3 findings, e.g. G1 E11: curve end misses V7 |
| 🟢 | Same vertex with local precision 1e-5: audit sound (sphere covers the gap) | sound |
| 🟢 | Edge tube 1e-4 wider than its exact vertices: precision order violated (T5) | T5 V7: vertex precision below edge E11 |

## F. Audit sensitivity (each corruption must be caught by the named check)

| | Corruption | Expected | First finding |
|---|---|---|---|
| 🟢 | Successor link of one fin skips a fin | T2 | T2 Fn1: successor's predecessor is not this fin |
| 🟢 | Both fins of one edge forced to the same sense | T3 | T2 Fn41: fin end vertex differs from successor start vertex |
| 🟢 | One face sense flipped | G5 | G5 F3: parametric area has the wrong sign for the face sense |
| 🟢 | One plane moved 1e-6 along its normal | G2 | G2 Fn17: edge E8 leaves the face surface |
| 🟢 | One SP-curve shifted 1e-6 in (u, v) | G3 | G3 Fn44: SP-curve disagrees with edge E22 |
| 🟢 | One vertex moved 1e-6 (exact model) | G1 | G1 E20: curve end misses V13 |

**PASS** — 0 failing row(s)
