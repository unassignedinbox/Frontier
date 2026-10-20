# 🔍 RESEARCH — Kernel algorithms: what a Parasolid-class kernel is made of, and the top 3 for every bone

**TL;DR** — You are right: SolidArc is a skeleton with missing and misplaced bones. Its *organs* are good and proven: exact NURBS evaluation, the rolling-ball station solve, GeometricAudit and the verification harnesses. Its *skeleton* is not what an industrial kernel stands on. It has:
- no shells or regions;
- no per-edge or per-vertex tolerance;
- no guaranteed start points for surface intersection;
- a boolean with box/cylinder shortcuts in front;
- local operations written as a per-shape catalogue;
- no history out of any operation.

**Recommendation**: rewrite the skeleton (topology, tolerance model, intersection, boolean, local operations, naming) on the Parasolid-class layout below, and transplant the organs. That is a rewrite, not a patch. It is also not a start from zero; the harnesses become the gate for every new bone.

Parasolid is proprietary. Everything here comes from its public documentation and the research literature it grew out of (the tolerant-modelling paper behind it is Jackson, 1995). That is enough to design the skeleton; the algorithms themselves are published.

---

## 1. Skeleton diagnosis — SolidArc vs a Parasolid-class kernel

| Bone | Parasolid-class kernel (public docs / literature) | SolidArc today (measured in this repository) | Verdict |
|---|---|---|---|
| Topology model | Body → Region → Shell → Face → Loop → Fin → Edge → Vertex; fins ordered radially around an edge, so non-manifold and general bodies are representable [P1][P3] | Vertex · Edge · Coedge · Loop · Face only. No shell or region (`Hulls` is only a count in the report), so cavities and multiple lumps have no home | 🔴 missing bones |
| Tolerance model | **Tolerant modelling**: exact by default (half the session precision); any edge or vertex may carry a local precision, i.e. edges are tubes and vertices are spheres, and every operation works on both [P1][P2][J95] | Global constants only (Kernel 1e-9, Join 1e-6). An inexact intersection is either forced exact or refused | 🔴 missing |
| Geometry types | Analytic surfaces (plane, cylinder, cone, sphere, torus) + B-surfaces + offset / swept / blend surfaces | Everything is a NURBS; `SurfaceClassification` is a hint. Closed-form quadric intersections are therefore unavailable | 🚧 half there |
| Surface–surface intersection | Start points guaranteed by boundary intersections + loop detection (parallel/collinear normals, normal cones), then marching [S1][S2][S3] | Seeds from tessellation triangle pairs, then marching (`TraceAll`). Small loops can be missed; seams and singular points refuse (G3, G4) | 🔴 wrong bone |
| Boolean | Interference graph first (vertex/vertex, vertex/edge, edge/edge, vertex/face, edge/face, face/face), then split, then build. Coincidence and tangency are handled as interferences, not refused [B1] | Face-pair trace → split → classify, preceded by shortcuts `AxisAlignedBoxBoolean`, `EquivalentRightCylinders`, `IdenticalBooleanResult`. Coplanar input falls back to a cell arrangement (G5) | 🔴 misplaced bones |
| Point containment | Ray / winding classification against every shell | Cut-adjacency propagation (good) + per-face 2D ring tests; no solid-level test against inner shells | 🚧 |
| Blending | Rolling-ball, variable, face–face, chamfer, cliff-edge, vertex blends with setbacks, overflow strategies (smooth, cliff, notch) [P4][P5] | `RollingBallBlend`: general station solve (good). No setback vertex blends or overflows. Plus a 13,762-line legacy catalogue | 🟢 organ, 🔴 limbs |
| Offset / hollow | Offset every face, intersect neighbours, remove self-intersections (trim → repair → extend) [P6][O1] | `shell` is box-only; `offsetface` is prism-only | 🔴 missing |
| Local operations | Generic face change: replace a face's surface, re-intersect the neighbours, re-make the blends [P7] | `FaceEditSolver`: 4,360 lines and 1,031 `if (` across per-shape routes | 🔴 wrong bone |
| History / naming | Every operation reports Generated / Modified / Deleted per sub-shape [B1]; CAD layers build persistent names on top [N3] | No operation reports history; edges referenced by index | 🔴 missing |
| Validity checking | Extensive checking functions | `GeometricAudit` A1–A7: found D7–D12 | 🟢 keep |

**Misplaced bones** (things that exist but sit in the wrong layer):
- **Shortcut layer in front of the boolean.** Special cases decide before the general algorithm runs. D9 (46 faces for an 11-face pocket) came from this layer.
- **Feature semantics in the console.** The bevel stack and `Rebind` live in `Console/ConsoleBevel.cpp`, not in a feature layer.
- **Shell inside the face editor**, as a box-specific route (D8: the wrong solid, passed by every check).
- **Tessellation inside kernel decisions.** `Validate().Volume` and the fallback bounds come from triangles. Fine for display, wrong as kernel truth.
- **`Validate().Solid()` is topological only.** A geometric check exists (`GeometricAudit`) but is not part of every operation's commit.

## 2. Pipeline map — where every algorithm is needed

```
 Sketch ──► SketchSolver ──► Profile (exact curves)
                                   │
 Feature (extrude/revolve/sweep/loft/boolean/blend/offset/face change)
                                   │  inputs by persistent name ── NameHistory
                                   ▼
 ┌──────────────────────────── KERNEL ────────────────────────────────────┐
 │ Geometry library: analytic + NURBS · evaluation · point inversion  [D,E]│
 │ Root isolation (curve/curve, curve/surface)                        [E]  │
 │ Surface/surface intersection: start points + loops + marching      [F]  │
 │ Interference graph → split → build (boolean, imprint, sew)         [G]  │
 │ Containment (point, face, shell)                                   [H]  │
 │ Blends: spine · cross-section · vertex blend · overflow            [I]  │
 │ Offset / hollow / thicken · self-intersection removal              [J]  │
 │ Local face change (push/pull, taper, replace, delete)              [K]  │
 │ Topology: Body/Region/Shell/Face/Loop/Fin/Edge/Vertex              [B]  │
 │   mutated only through Euler edits                                 [C]  │
 │ Robustness: tolerant modelling + exact predicates                  [A]  │
 │ Audit gate (GeometricAudit) on every result                        [O]  │
 └──────────────────────────────────┬──────────────────────────────────────┘
                                    ▼
 Tessellation (watertight, parameter-space CDT)  [N] ──► SketchModelViewport
```

Every operation needs A, B, C, D and O. Booleans need E, F, G and H. Blends need E, F, I, and G for imprinting the blend. Offset needs F, G and J. Local operations need F, I and K. The feature layer needs L and M.

## 3. Top 3 per bone

Ratings: ⭐ = low effort and low risk; ⭐⭐⭐⭐⭐ = high robustness, high generality and a high overall rating. 🥇 is the pick.

### A · Robustness model

| Rank | Approach | Where used | Robustness | Effort | Rating |
|---|---|---|---|---|---|
| 🥇 | **Tolerant modelling**: exact by default, local precision on edges and vertices, all tests done to the sum of tolerances [J95][P1] | Parasolid; ACIS and OCCT carry per-shape tolerances too [B1] | ⭐⭐⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐⭐⭐ |
| 🥈 | **Adaptive exact predicates** with floating-point filters (orientation, incircle) [R1][R2] | Delaunay, CDT and 2D parameter-space decisions | ⭐⭐⭐⭐⭐ (for predicates) | ⭐⭐ | ⭐⭐⭐⭐ |
| 🥉 | **Rounded interval arithmetic** to bound roots and surfaces [E2] | IPP solver, conservative bounds | ⭐⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐ |

**Pick**: 🥇 for 3D geometry plus 🥈 for every combinatorial decision in parameter space. They are complementary. Exact arithmetic alone cannot represent constructed intersection points, which are irrational [R1].

### B · Topology data structure

| Rank | Structure | Supports | Rating |
|---|---|---|---|
| 🥇 | **Body/Region/Shell/Face/Loop/Fin/Edge/Vertex** with radial fin order [P1][P3] | manifold, sheet, wire, general (non-manifold), cavities | ⭐⭐⭐⭐⭐ |
| 🥈 | Radial edge (Weiler) [T1] | non-manifold; more storage | ⭐⭐⭐⭐ |
| 🥉 | Half-edge (Mäntylä) [T1] | manifold only | ⭐⭐⭐ |

**Pick**: 🥇. It is radial-edge-class, so it covers 🥈. The partial-entity structure halves radial-edge storage [T2] and can come later as an optimisation.

### C · Topology mutation

| Rank | Approach | Rating |
|---|---|---|
| 🥇 | **Euler edits** as the only mutation API: every edit preserves the Euler–Poincaré relation, so invalid topology cannot be built [T3] | ⭐⭐⭐⭐⭐ |
| 🥈 | Free construction + validation afterwards (today's `AddEdge` / `AddCoedge` + `Validate`) | ⭐⭐ |
| 🥉 | Rebuild by sewing faces (`Sew`) | ⭐⭐ |

### D · Geometry representation

| Rank | Approach | Rating |
|---|---|---|
| 🥇 | **Analytic first, NURBS second**: true plane, cylinder, cone, sphere and torus types; B-splines for freeform; procedural offset and blend surfaces where exact | ⭐⭐⭐⭐⭐ |
| 🥈 | NURBS-only with classification hints (today) | ⭐⭐⭐ |
| 🥉 | T-splines for watertight trims [N4] | ⭐⭐ (specialist) |

Analytic types turn most CAD intersections (plane∩cylinder, cylinder∩cylinder, plane∩sphere…) into closed forms. That is where G2, G3 and part of G4 disappear.

### E · Root isolation (curve/curve, curve/surface, point inversion)

| Rank | Algorithm | Property | Rating |
|---|---|---|---|
| 🥇 | **Bézier clipping** (Sederberg–Nishita): convex-hull clipping, needs no initial guess, finds all roots in range, quadratic convergence [E1] | robust + fast | ⭐⭐⭐⭐⭐ |
| 🥈 | **Interval Projected Polyhedron** (Patrikalakis–Maekawa): the n-dimensional generalisation in rounded interval arithmetic [E2] | certified | ⭐⭐⭐⭐ |
| 🥉 | Subdivision + Newton refinement | simple | ⭐⭐⭐ |

**Pick**: 🥇 for the 1D and 2D cases; 🥈 for systems (critical points, collinear normals).

### F · Surface–surface intersection

| Rank | Algorithm | Rating |
|---|---|---|
| 🥇 | **Hybrid** [S1][S2][S3][S4]:<br>1. closed forms for analytic pairs;<br>2. start points from boundary curve/surface intersections (E);<br>3. loop detection from collinear-normal points, which lie inside every closed loop and at every singular point (solved with E 🥈), with normal cones to prune;<br>4. marching with adaptive step;<br>5. branch splitting at singular points | ⭐⭐⭐⭐⭐ |
| 🥈 | Lattice/tessellation seeds + marching (today) | ⭐⭐ (misses small loops) |
| 🥉 | Algebraic (implicitisation) | ⭐ (two bicubic patches meet in a curve of algebraic degree 324 [S5]) |

Patrikalakis classifies SSI into analytic, lattice, subdivision and marching methods [S5]. Recent work adds precision control to NURBS SSI [S6], and combines winding numbers with subdivision to find small loops and tangent curves robustly [S4]. Both are candidate refinements for steps 3–4.

### G · Boolean / imprint / sew

| Rank | Algorithm | Rating |
|---|---|---|
| 🥇 | **General Fuse** [B1][B2]:<br>1. interferences in order of dimension (vertex/vertex, vertex/edge, edge/edge, vertex/face, edge/face, face/face), each to the sum of tolerances;<br>2. split edges into pave blocks, with common blocks for coincident edges;<br>3. section edges with p-curves;<br>4. build faces, then shells, then solids;<br>5. pick the parts by operation (fuse / common / cut);<br>6. report history. One algorithm also gives imprint, split and sew | ⭐⭐⭐⭐⭐ |
| 🥈 | Classic split-and-classify (Requicha–Voelcker / Mäntylä set membership; today's traced route) | ⭐⭐⭐ (coincident and tangent cases become refusals) |
| 🥉 | Polygonal exact-arithmetic booleans | ⭐ for exact B-rep (output is a polygon complex) |

**Why 🥇 fixes our refusals**: coplanar faces are a face/face interference with a common part, and a seam crossing is a vertex/edge interference. Both get computed *before* any face is split, instead of surfacing later as "coincident faces" or "seam corner".

### H · Containment

| Rank | Algorithm | Rating |
|---|---|---|
| 🥇 | **Adjacency propagation** across split edges (exact and local; SolidArc already has it) + **winding number** fallback for detached pieces | ⭐⭐⭐⭐⭐ |
| 🥈 | Generalized winding number on trimmed NURBS (2025): tolerant of gaps and overlaps, needs no ray tie-breaking [H1][H2] | ⭐⭐⭐⭐ |
| 🥉 | Ray casting with perturbation (fails on grazing rays, coincident boundaries, open boundaries) [H2] | ⭐⭐ |

### I · Blending

| Rank | Algorithm | Rating |
|---|---|---|
| 🥇 | **Rolling-ball spine** = intersection of the offset surfaces, with a circular or conic cross-section (Choi–Ju constant radius) [I1][I2]. This is our `RollingBallBlend`, extended with:<br>• **setback vertex blends** (2n-sided patches, Várady–Rockwood) [I3];<br>• **overflow strategies** (smooth, cliff, notch) [P4][P5] | ⭐⭐⭐⭐⭐ |
| 🥈 | Face–face blends (blend between arbitrary face sets, hold lines, cliff edges) [P5] | ⭐⭐⭐⭐ |
| 🥉 | Per-configuration catalogue (legacy `BlendSolver`) | ⭐ |

The Vida–Martin–Várady survey is the map of the whole field: edge, vertex and region blends, trimlines, profile curves [I1].

### J · Offset / hollow / thicken

| Rank | Algorithm | Rating |
|---|---|---|
| 🥇 | **Offset faces → intersect neighbouring offset surfaces for edges and vertices → remove self-intersections (trim, then repair with a patch, then extend neighbours)** (Forsyth; Parasolid) [O1][P6] | ⭐⭐⭐⭐⭐ |
| 🥈 | Initial offset solid → make it manifold → regularised boolean [O2] | ⭐⭐⭐⭐ (good fallback) |
| 🥉 | Distance-field / voxel offset | ⭐⭐ (approximate) |

### K · Local face operations

| Rank | Approach | Rating |
|---|---|---|
| 🥇 | **Generic face change**: give each selected face a new surface (offset, transformed, tapered or replaced), re-intersect with the neighbours via F and G, re-make the blends via I [P7] | ⭐⭐⭐⭐⭐ |
| 🥈 | Rebuild through the feature tree (replay with new parameters) | ⭐⭐⭐ |
| 🥉 | Per-shape catalogue (today's `FaceEditSolver`) | ⭐ |

Push, pull, taper, offset-face, extend, delete-face and shell all become *one* algorithm with different surface replacements.

### L · Persistent naming

| Rank | Scheme | Rating |
|---|---|---|
| 🥇 | **History-mapper names**: each operation reports Generated / Modified / Deleted; a name = source element + operation tag + disambiguator. Unnamed elements get names from their named neighbours (FreeCAD 1.0, realthunder's algorithm on OCCT history) [N3] | ⭐⭐⭐⭐⭐ |
| 🥈 | Topological ID system (Kripac): face ID graph + matching [N1][N2] | ⭐⭐⭐ (graph matching is costly) |
| 🥉 | Generic naming (Capoyleas–Chen–Hoffmann): names from feature semantics (profile, path) [N1][N2] | ⭐⭐⭐⭐ (good disambiguation, use inside 🥇) |

**Pick**: 🥇, with 🥉's feature-semantic roles for faces born by sweeps, and a geometric search fallback (realthunder's algorithm also falls back to geometric search [N3]).

### M · Sketch constraint solving

| Rank | Approach | Rating |
|---|---|---|
| 🥇 | **Numerical**: Newton / Levenberg–Marquardt / DogLeg on the full system; DOF and redundancy from a rank-revealing QR of the Jacobian; per-subsystem diagnosis with a scale-aware rank tolerance (SolveSpace; FreeCAD planegcs) [M1][M2][M4] | ⭐⭐⭐⭐ |
| 🥈 | **Graph-constructive decomposition** (Owen 1991 → D-Cubed DCM; Hoffmann–Lomonosov–Sitharam DR-plans) feeding a numerical solve per cluster [M5][M6][M7] | ⭐⭐⭐⭐⭐ (industrial) |
| 🥉 | Iterative relaxation (today's float Gauss–Seidel in `ref/new`) | ⭐ |

**Pick**: build 🥇 first (smaller, and it gives correct DOF), then add 🥈's decomposition for speed and stable solution selection. D-Cubed DCM descends from Owen's graph-constructive work [M5].

### N · Tessellation

| Rank | Algorithm | Rating |
|---|---|---|
| 🥇 | **Shared edge discretisation + parameter-space constrained Delaunay** per face → watertight by construction [N7][N8] | ⭐⭐⭐⭐⭐ |
| 🥈 | Lattice clipped by trims + ear clipping (today; the source of D7) | ⭐⭐ |
| 🥉 | GPU adaptive tessellation for display only | ⭐⭐⭐ (viewport add-on) |

### O · Audit gate

`GeometricAudit` (A1–A7) stays, and becomes mandatory on every operation's commit: a result is either sound or refused.

## 4. The rewrite — what is kept, what is new

| Kept (transplanted organs) | Lines today | New skeleton | Replaces |
|---|---|---|---|
| NURBS curve/surface evaluation, split, knot work (`CurveSpecification`, `SurfaceSpecification`, `VectorSpecification`) | ≈ 2,350 | Topology model (B) + Euler edits (C) + tolerance fields (A) | `TopologySpecification` topology part |
| Rolling-ball station solve and march (`RollingBallBlend`) | 1,609 | Analytic surface types (D) | classification hints |
| Skin / profile constructors (extrude, revolve, loft, sweep) | ≈ 2,000 | Root isolation (E) + hybrid SSI (F) | `TraceAll` seeding |
| `GeometricAudit` + all verification harnesses and corpora | ≈ 1,500 | General Fuse (G) + containment (H) | `IntersectionSolver` + shortcuts |
| Constraint solver experiments (`ConstraintSolver`) | 498 | Vertex blends + overflow (I) | legacy `BlendSolver` (13,762) |
| | | Offset / hollow (J) + generic face change (K) | `FaceEditSolver` (4,360), `TweakSolver` |
| | | History reporting (L) from every operation | — |
| | | Parameter-space CDT tessellation (N) | lattice tessellator |

Build order, where each step is gated by the existing harnesses, run against the *new* kernel:
1. **A + B + C + D**: topology, tolerances, analytic types. Gate: primitives corpus rows exact; audit sound.
2. **E + F**: root isolation and SSI. Gate: new SSI corpus covering seam crossings (CrossUnequal), Steinmetz, tangent pairs and small loops.
3. **G + H**: General Fuse. Gate: corpus section C with 0 refusals, maximal faces (Pocket 11, PlusUnion 10), every D9 specimen.
4. **N**: tessellation. Gate: D7 specimens, watertight check.
5. **I**: blends on the new topology. Gate: rolling-ball harness 15/15, BevelLab 6/6, plus setback corners.
6. **J + K**: offset and generic face change. Gate: corpus section E with 0 refusals on HoleSlab and Can.
7. **L + M**: history names and the sketch solver, then the feature tree from the plan.

## 5. Decision

| Option | Effort | Risk | Ceiling | Rating |
|---|---|---|---|---|
| **Rewrite the skeleton, transplant the organs (§ 4)** | ⭐⭐⭐⭐ | ⭐⭐ (harnesses gate every step) | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ |
| Full rewrite from zero | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ (re-solves what already works: NURBS, station solve) | ⭐⭐⭐⭐⭐ | ⭐⭐⭐ |
| Keep patching SolidArc | ⭐⭐ per fix | ⭐⭐⭐⭐ (D8 shows wrong solids pass every check) | ⭐⭐ | ⭐⭐ |
| Adopt OCCT as the kernel | ⭐⭐ | ⭐⭐⭐ (LGPL, size; we would inherit its algorithms instead of owning them) | ⭐⭐⭐⭐ | ⭐⭐⭐ |

⭐ = low for effort and risk, high for ceiling and rating.

## References

Links are given only for pages retrieved during this research. Classic papers are cited bibliographically.

- [P1] Parasolid V35 Overview — Model structure, tolerant modelling: http://www.q-solid.com/Parasolid_Docs_V35/chapters/ov_chap.04.html
- [P2] Parasolid V35 Functional Description — session and local precision: http://www.q-solid.com/Parasolid_Docs_V35/chapters/fd_chap.017.html
- [P3] Parasolid XT Format Reference (entities, fins, tolerances): http://www.q-solid.com/Parasolid_Docs_V35/pdf/xt.pdf
- [P4] Parasolid V35 documentation — blending (rolling-ball, variable, face-face, cliff-edge, conic; overflows smooth / cliff / notch), same documentation set as [P1]
- [P5] Parasolid V35 documentation — face–face blending, same set
- [P6] Parasolid V35 documentation — offsetting and hollowing (offset faces, pierce faces, local boolean; self-intersection removal by trim / repair / extend), same set
- [P7] Parasolid V35 documentation — local operations / generic face change, same set
- [J95] D. J. Jackson, "Boundary representation modelling with local tolerances", Proc. 3rd ACM Symposium on Solid Modeling, 1995: https://dl.acm.org/doi/10.1145/218013.218067
- [B1] OCCT Boolean Operations specification (General Fuse, interferences, pave blocks, history, fuzzy mode): https://dev.opencascade.org/doc/overview/html/specification__boolean_operations.html
- [B2] Same specification, Builder / Splitter / Cells builder sections
- [S1] T. W. Sederberg, R. J. Meyers, "Loop detection in surface patch intersections", CAGD 5(2), 1988
- [S2] R. E. Barnhill, S. N. Kersey, "A marching method for parametric surface/surface intersection", CAGD 7, 1990
- [S3] N. M. Patrikalakis, T. Maekawa, W. Cho, *Shape Interrogation for Computer Aided Design and Manufacturing* (MIT hyperbook), ch. 4–5 (IPP solver, SSI, collinear-normal points)
- [S4] Robust NURBS intersection — small loops and tangent intersections, ACM 2026: https://dl.acm.org/doi/10.1145/3807948
- [S5] N. M. Patrikalakis, "Surface-to-surface intersections", IEEE CG&A 13(1), 1993
- [S6] Precision-controlled surface–surface intersection for NURBS, ACM 2026: https://dl.acm.org/doi/10.1145/3806045
- [E1] T. W. Sederberg, T. Nishita, "Curve intersection using Bézier clipping", CAD 22(9), 1990
- [E2] Patrikalakis–Maekawa–Cho [S3], Interval Projected Polyhedron in rounded interval arithmetic
- [R1] J. R. Shewchuk, "Adaptive precision floating-point arithmetic and fast robust geometric predicates", Discrete & Computational Geometry 18, 1997
- [R2] Same author, floating-point filters for orientation / incircle
- [T1] K. Weiler, "The radial edge structure", in *Geometric Modeling for CAD Applications*, 1988; M. Mäntylä, *An Introduction to Solid Modeling*, 1988 (half-edge)
- [T2] S. H. Lee, K. Lee, "Partial entity structure: a compact boundary representation for non-manifold geometric modeling", JCISE 1(4), 2001
- [T3] Mäntylä 1988 [T1], Euler operators
- [H1] Generalized winding numbers for trimmed NURBS containment: https://arxiv.org/abs/2504.11435
- [H2] Point containment on trimmed surfaces: https://arxiv.org/abs/2510.25159
- [I1] J. Vida, R. R. Martin, T. Várady, "A survey of blending methods that use parametric surfaces", CAD 26(5), 1994
- [I2] B. K. Choi, S. Y. Ju, "Constant-radius blending in surface modelling", CAD 21(4), 1989
- [I3] T. Várady, A. Rockwood, "Geometric construction for setback vertex blending", CAD 29(6), 1997
- [O1] M. Forsyth, "Shelling and offsetting bodies", Proc. 3rd ACM Symposium on Solid Modeling, 1995
- [O2] Shelling via initial offset solid → self-intersection repair → regularised boolean (2004 study)
- [N1] Mechanisms of persistent identification of topological entities: a review: https://www.sciencedirect.com/science/article/pii/S1110016818300814
- [N2] J. Kripac, "A mechanism for persistently naming topological entities in history-based parametric solid models", CAD 29(2), 1997; V. Capoyleas, X. Chen, C. M. Hoffmann, "Generic naming in generative, constraint-based design", CAD 28(1), 1996
- [N3] realthunder, Topological Naming Algorithm (basis of FreeCAD 1.0): https://github.com/realthunder/FreeCAD_assembly3/wiki/Topological-Naming-Algorithm
- [N4] T. W. Sederberg et al., "Watertight trimmed NURBS", ACM TOG 27(3), 2008
- [N7] Robust watertight CAD tessellation via parameter-space constrained Delaunay, JCDE 2026: https://doi.org/10.1093/jcde/qwaf134
- [N8] L. Piegl, A. Richard, "Tessellating trimmed NURBS surfaces", CAD 27(1), 1995
- [M1] SolveSpace solver (modified Newton + least squares), SolveSpace technical notes
- [M2] FreeCAD planegcs (DogLeg default, LM, BFGS, SQP; dense/sparse QR redundancy diagnosis)
- [M4] FreeCAD FPA grant proposals, issue #106 (structural DOF vs numerical rank)
- [M5] J. C. Owen, "Algebraic solution for geometry from dimensional constraints", Proc. ACM Symposium on Solid Modeling, 1991
- [M6] C. M. Hoffmann, A. Lomonosov, M. Sitharam, "Decomposition plans for geometric constraint systems", J. Symbolic Computation 31, 2001
- [M7] I. Fudos, C. M. Hoffmann, "A graph-constructive approach to solving systems of geometric constraints", ACM TOG 16(2), 1997
