# Setback vertex blend validation (fair NURBS quads)

**🟢 PASS** · 4 bodies · 2 refusals · 94040 ms

## Gates

| | Gate | Measures | Limit | Worst | Checks | Failures |
|---|---|---|---|---|---|---|
| 🟢 | S1 | Exact rims: every edge of the result on both face carriers (16 samples, relative to the model size) | ≤ 1e-9 | 2.35e-16 | 4 | 0 |
| 🟢 | S2 | G1 setback quad ↔ neighbouring blend / face: ∠ of outward normals across the shared edge [rad] | ≤ 1e-2 | 2.89e-03 | 4 | 0 |
| 🟢 | S3 | G1 setback quad ↔ quad across a spoke: ∠ of outward normals [rad] | ≤ 5e-2 | 2.14e-02 | 4 | 0 |
| 🟢 | S4 | Bodies: audit sound, genus kept, watertight polygon complex, \|V − V_polygon\| ≤ peak deviation × area, expected vertex categories | pass | 0.00e+00 | 4 | 0 |
| 🟢 | S5 | Determinism: rebuilt twice → identical volume bits and polygon digest | identical | 0.00e+00 | 4 | 0 |
| 🟢 | S6 | Refusals: atomic, prefixed, source intact | all refused | 0.00e+00 | 2 | 0 |

## Bodies

| | Specimen | Edges | Setback | Ball | Cap | Quads | Faces | Genus | V₀ | V | Rim (S1) | Solver rim | G1 (S2) | Seam (S3) | \|V − V_polygon\| | Bound | ms | Outcome |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 🟢 | L block, every edge r 0.1 (2 mixed vertices: 2 convex + 1 concave) | 18 | 2 | 10 | 0 | 12 | 48 | 0 | 4.500000000000 | 4.456189702861 | 2.4e-16 | 1.3e-15 | 2.89e-03 | 2.14e-02 | 6.8e-05 | 1.1e-03 | 32950 | blended |
| 🟢 | L block, every edge r 0.05 | 18 | 2 | 10 | 0 | 12 | 48 | 0 | 4.500000000000 | 4.488641061565 | 2.2e-16 | 1.0e-15 | 2.89e-03 | 2.14e-02 | 3.5e-05 | 1.3e-03 | 30018 | blended |
| 🟢 | Square pyramid, every edge r 0.1 (apex of degree 4) | 8 | 1 | 4 | 0 | 8 | 25 | 0 | 1.333333333333 | 1.241668774072 | 1.7e-16 | 6.7e-16 | 7.69e-05 | 9.38e-03 | 3.7e-05 | 4.0e-04 | 17072 | blended |
| 🟢 | Square pyramid, the 4 apex edges r 0.1 (setback + 4 caps) | 4 | 1 | 0 | 4 | 8 | 17 | 0 | 1.333333333333 | 1.330394628342 | 1.5e-16 | 6.7e-16 | 7.69e-05 | 9.38e-03 | 1.4e-05 | 5.7e-04 | 13983 | blended |

## Refusals

| | Specimen | Refusal |
|---|---|---|
| 🟢 | L block, every edge chamfer 0.1 — must refuse (chamfer setback 🚧) | Blend: vertex V8: setback vertex blend of chamfers (🚧) |
| 🟢 | L block, concave edge + its riser edge only (mixed beside a sharp edge) — must refuse | Blend: vertex V8: convex and concave blends meet beside a sharp edge (🚧) |
