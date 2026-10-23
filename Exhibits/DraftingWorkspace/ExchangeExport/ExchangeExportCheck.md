# Exchange walkthrough files read by other software

Bracket closed form V = 10222773746.755856 mm³

| File | Reader | Findings | ΔV / V | Verdict |
|---|---|---|---|---|
| `Exchange.step` | OpenCascade STEPControl_Reader + BRepCheck_Analyzer + GProp (ε 1e-12) | read, valid True, 1 solid, 11 faces, V 10222773746.755857 mm³ | 1.87e-16 | 🟢 |
| `Exchange.stl` | trimesh | 1906 facets, watertight True, winding consistent True, V 10220417977.0 mm³ (chordal sag) | 2.30e-04 | 🟢 |
