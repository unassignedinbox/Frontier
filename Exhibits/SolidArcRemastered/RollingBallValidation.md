# Rolling-ball carrier validation (procedural pipe on traced spines)

**🟢 PASS** · 3 bodies · 4 transforms · 83416 ms

## Gates

| | Gate | Measures | Limit | Worst | Checks | Failures |
|---|---|---|---|---|---|---|
| 🟢 | R1 | Ball: \|foot − C(τ)\| − r and \|P(𝑢,𝑣) − C(𝑢)\| − r, (P − C)·T, relative to r | ≤ 1e-9 | 1.23e-13 | 5 | 0 |
| 🟢 | R2 | Contact: distance of each rail point to its support | ≤ 1e-9 | 4.44e-16 | 5 | 0 |
| 🟢 | R3 | G1: angle between carrier and support normals at the rails [rad] | ≤ 1e-8 | 1.42e-14 | 5 | 0 |
| 🟢 | R4 | Projection round trip \|P(proj(P(𝑢,𝑣))) − P(𝑢,𝑣)\| | ≤ 1e-9 | 1.37e-15 | 5 | 0 |
| 🟢 | R5 | Chart normal: ∠(∂𝑢 × ∂𝑣, radial) [rad] | ≤ 1e-8 | 2.82e-10 | 5 | 0 |
| 🟢 | R5′ | Chart: ∂𝑢 vs a Richardson difference at 10⁻⁴·range (relative) | ≤ 1e-6 | 6.29e-07 | 5 | 0 |
| 🟢 | R6 | Analytic reproduction: line spine ≡ cylinder, circle spine ≡ torus (distance) | ≤ 1e-12 | 8.33e-16 | 1 | 0 |
| 🟢 | R7 | Carrier ∩ normal plane: a closed branch on the section circle (distance) | ≤ 1e-8 | 4.44e-16 | 5 | 0 |
| 🟢 | R8 | Bodies: audit sound, \|V − V_polygon\| ≤ peak deviation × area, fillet removes material | pass | 0.00e+00 | 3 | 0 |
| 🟢 | R9 | Transforms there and back: audit sound, V·s³ (relative), vertices home | ≤ 1e-9 | 8.08e-12 | 4 | 0 |

## Bodies (disc r 3 × 1 − one bore, fillet the traced mouth)

| | Specimen | Edges | Sheets | Pipes | Faces | Genus | V | Removed | \|V − V_polygon\| | Bound | ms | Outcome |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 🟢 | Radial bore through the axis (crosses both seams), r 0.03 | 2 | 2 | 2 | 7 | 0 | 28.022941899773 | 2.741e-04 | 8.7e-04 | 1.3e-02 | 9916 | blended |
| 🟢 | Radial bore through the axis, r 0.06 | 2 | 2 | 2 | 7 | 0 | 28.022072222397 | 1.144e-03 | 8.7e-04 | 1.3e-02 | 11082 | blended |
| 🟢 | Offset bore (y 0.8, asymmetric mouth), r 0.04 | 1 | 1 | 1 | 6 | 0 | 27.902839245929 | 6.976e-04 | 8.4e-04 | 1.3e-02 | 15029 | blended |

## Transforms

| | Journey | \|V − V*·s³\| / V* | Vertices home | Outcome |
|---|---|---|---|---|
| 🟢 | Rotation 37° about a skew axis + translation | 8.1e-12 | 1.5e-15 | round trip |
| 🟢 | Reflection in a skew plane | 2.7e-12 | 5.7e-16 | round trip |
| 🟢 | Uniform scale × 2.5 about (1, 1, 1) | 7.1e-14 | 4.7e-16 | round trip |
| 🟢 | Non-uniform scale (1, 1, 2) — must refuse | 0.0e+00 | 0.0e+00 | Transform: a non-similar map on a traced or contact edge (its chord-length τ does not stretch uniformly, 🚧) |
