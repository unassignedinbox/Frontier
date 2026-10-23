# GeometryProcessing — PredicateValidation

| | Family | Cases | Oracle zero | Exact wrong | Naive wrong | Expansion fallback |
|---|---|---|---|---|---|---|
| 🟢 | Orient2D near-collinear | 200000 | 0 | 0 | 4564 (2.3%) | 37754 (18.88%) |
| 🟢 | Orient2D random | 200000 | 0 | 0 | 0 (0.0%) | 0 (0.00%) |
| 🟢 | Orient3D near-coplanar | 200000 | 0 | 0 | 5072 (2.5%) | 122215 (61.11%) |
| 🟢 | Orient3D exact-coplanar lattice | 200000 | 200000 | 0 | 91288 (45.6%) | 200000 (100.00%) |
| 🟢 | InCircle near-cocircular | 200000 | 0 | 0 | 27125 (13.6%) | 169898 (84.95%) |
| 🟢 | InCircle exact-cocircular lattice (108 points) | 200000 | 200000 | 0 | 0 (0.0%) | 200000 (100.00%) |

**PASS** — 0 failing row(s)
