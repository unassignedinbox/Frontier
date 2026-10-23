| Defect | Specimen | Measure | Value | Bound | Before the fix | Verdict |
|---|---|---|---|---|---|---|
| D7 | DomedPedestal | hemisphere area rel. error | 0.000833 | ≤ 0.002 | 4.2e-2 | ✔️ |
| D7 | DomedPedestal | Validate volume rel. error | 0.000109 | ≤ 0.001 | 9.6e-2 | ✔️ |
| D5b | ScoopedSocket | audit findings | 0 | ≤ 0 | bounds-excess 1.67e-1 | ✔️ |
| D8 | Shelled box | volume rel. error vs 3 − 1.8·1.3·0.9 | 2.41e-08 | ≤ 1e-07 | 0.38 (volume 0.555 vs 0.894) | ✔️ |
| D8 | Shelled box | faces (5 outer + 5 inner + rim) | 11 | ≤ 11 | 10, no inner y walls | ✔️ |
| D9 | Pocket | faces | 11 | ≤ 11 | 46 | ✔️ |
| D9 | Pocket | volume rel. error | 2.41e-08 | ≤ 1e-07 | exact | ✔️ |
| D9 | SquareHole | faces | 10 | ≤ 10 | 32 | ✔️ |
| D9 | SquareHole | volume rel. error | 2.41e-08 | ≤ 1e-07 | exact | ✔️ |
| D9 | CornerNotch | faces | 10 | ≤ 10 | 34 | ✔️ |
| D9 | CornerNotch | volume rel. error | 2.42e-08 | ≤ 1e-07 | exact | ✔️ |
| D9 | CornerUnion | faces | 12 | ≤ 12 | 42 | ✔️ |
| D9 | CornerUnion | volume rel. error | 2.41e-08 | ≤ 1e-07 | exact | ✔️ |
| D9 | PlusUnion | faces | 22 | ≤ 22 | 22 (coplanar: arrangement fallback, unchanged) | ✔️ |
| D9 | PlusUnion | volume rel. error | 2.42e-08 | ≤ 1e-07 | exact | ✔️ |
| D10 | Oblique pipe | Bounds() excess / diagonal | 0.00131 | ≤ 0.005 | 3.3e-2 | ✔️ |
| D10 | Oblique pipe | Bounds() shortfall (must contain) | -0 | ≤ 1e-09 | — | ✔️ |
| D11 | Wheel–axle–wheel union | audit findings | 0 | ≤ 0 | trace-on-edge 5.70 m | ✔️ |
| D12 | ToySailboat Hull | audit findings | 0 | ≤ 0 | trace-on-edge 2.35e-2, bounds-excess 1.13e-1 | ✔️ |
| — | HoleSlab rim fillet | volume rel. error vs Pappus | 3.22e-08 | ≤ 1e-07 | 3.355 reported (tessellation) | ✔️ |
| — | HoleSlab rim fillet | audit findings | 0 | ≤ 0 | — | ✔️ |
21 specimen measure(s), 0 failed
