# 🧩 SolidArcRemastered

**TL;DR**: This is a from-scratch, Parasolid-style B-rep modeller.
- Step 1 covers precision, topology, Euler edits and analytic geometry.
- Step 2 covers Bézier-clipping root solving, curve–surface intersection and hybrid surface–surface intersection.
- Step 3 adds generalized-winding-number containment and General Fuse Booleans (OCCT-style PaveFiller + Builder, all four operations from one fuse).
- Step 4 adds watertight shared-edge tessellation: one sampling per edge, used by every face that touches it, and a constrained Delaunay triangulation per face in its parameter chart.
- Step 5 adds rolling-ball fillets and chamfers. The spine is the intersection of the two offset surfaces (Choi–Ju). Exact Cylinder, Torus, Cone, Plane and corner Sphere carriers are trimmed at caps, mitres and ball corners. The body is rebuilt by assembly with face history, and every refusal is atomic.
- Step 6 adds the generic face change (research § K) and offsets (§ J). Each selected face gets a new carrier: offset, translated, scaled, rotated, drafted or replaced by a rim. Its edges and vertices are re-solved by intersecting the neighbouring carriers, and the body is rebuilt by assembly. Push, tweak, offset, scale, rotate, draft, extend, trim, offset-body, hollow, extrude, delete, heal and thicken are thin wrappers over that one algorithm. An edge/face clash probe refuses self-intersecting results.
- Step 7 adds the sketch solver (research § M), persistent names (§ L) and the feature tree. The solver runs Gauss–Newton and Levenberg–Marquardt on the full system, and a rank-revealing QR of J gives DOF, redundant constraints and conflict groups. Every face, edge and vertex gets a history name such as `Extrude#2.Side(Line1)`. A tree regenerates by replay; each reference resolves exact → same source → geometric rebind, and anything ambiguous is refused, never guessed. A bevel now follows its sketch, which closes G1.

A C++ library is inspected through a Python host and a single-page HTML viewer that rebuilds whenever a source file changes.

## Status: steps 1–7 of the build order

| Area | Content | Gate |
|---|---|---|
| A. Precision | Session/angular precision, local (tolerant) precision per edge and vertex, Shewchuk adaptive `Orient2D` / `Orient3D` | 🟢 PredicateValidation: 0 wrong out of 800 000 |
| B. Topology | Body → Region → SolidShell → Face → Loop → Fin → Edge → Vertex, generation-checked tokens | 🟢 T1–T5 audit after every edit |
| C. Euler edits | MVFS/KVFS, MEV/KEV, MEF/KEF, MEKR/KEMR, KFMRH/MFKRH, ReverseShellOrientation, TransferShell, DetachShell | 🟢 9 specimens torn down to an empty partition |
| D. Geometry | Line, circle, ellipse (`ConicForm`); plane, cylinder, cone, sphere, torus, offset surface; SP-curves (line, ellipse) | 🟢 Volume by quadrature, relative error ≤ 1.4e-15 |
| E. Root solving + CSI | Bernstein algebra, Bézier clipping (Sederberg–Nishita), exact rational Bézier arcs composed into implicit surfaces (degree ≤ 8), roots of F and F′, Newton polish on the exact distance, tangent and coincident detection | 🟢 IntersectionValidation E1/E2: 60 000 polynomials, all within the condition bound; 2 000 random curve–surface pairs, 0 of 3 183 sign changes missed |
| F. Hybrid SSI | Analytic dispatch (plane–plane, coaxial meridians, parallel sections, plane through an axis), then lattice + boundary seeds, normal-cone loop detection, 3D-corrector marching, singular-point splitting | 🟢 F1: 3 500 random analytic cases ≤ 5.4e-15. F2: marching on the exact curves (≤ 7.6e-15). F3: 22 gallery runs + 300 random oblique pairs sound, and the negative control is caught |
| G. General Fuse Booleans | PaveFiller: EE, EF, FF interferences (exact SSI; marched sections that touch both faces are refused), paves, pave blocks, common blocks, section splitting, imprint. Builder: face splitting by smallest-turn cycle tracing in the chart, outer/hole assignment, fragment classification (GWN, plus normals for on-boundary fragments). Selection and assembly for ∪ ∩ − with history (Modified / Generated) | 🟢 BooleanValidation: 15 closed-form scenes × 4 operations + 160 random pairs; audit sound, ΔV ≤ 3.6e-15, identities ≤ 5.7e-14, 0 of 22 560 oracle points wrong; an open argument is refused |
| H. Containment | Projected SP-curves (`ParameterProjection`), face domains in the chart, 2D parameter winding, 3D generalized winding number Σ Ω_F / 4π by Green in the chart (adaptive Gauss–Kronrod). Proximity first, then the nearest-normal shortcut, then winding | 🟢 ContainmentValidation H1–H6: 12 000 random points, normal offsets down to 1e-7, on-boundary points, ray-cast parity, open-box Van Oosterom–Strackee, 2D holes; 0 wrong, integer residual ≤ 3.2e-13 |
| N. Shared-edge CDT tessellation | Exact `InCircle`; Bowyer–Watson CDT with exact cavity repair, Sloan constraint recovery, parity classification (`ChartTriangulation`). Per edge: adaptive τ bisection to sag ε, a √(8ε/κ) chord bound and chart fidelity on every fin. Per face: linear chart scaled by \|S_u\|, \|S_v\|; Ruppert-style refinement on 3D sag (distance to the surface) and side length, with a quality floor; pole and apex stars rebuilt from the chart boundary of the star; 3D side flips of folded slivers; split of interior sides another face already used | 🟢 TessellationValidation: 305 bodies (9 specimens, 15 scenes × 4, 240 random Boolean results), 668 918 triangles. 0 unpaired sides, χ equal, peak ≤ ε, 0 inverted / zero-area / folded, \|ΔV\| ≤ 0.48 ε·A, 0 missing edge sides. Convergence slopes ≈ 1, deterministic digests, scale-free 10⁻³…10³, 201 CDT regressions |
| J + K. Face change and offsets | `PlanFaceChange`: new carrier per selected face (offset, similarity motion, draft hinge at the neutral plane: plane → plane, cylinder → cone), every edge re-solved as kept / intersected / seam / laminar (exact plane–plane, plane–cylinder circle or ellipse, coaxial and cone sections), every vertex re-solved from three carriers, intervals re-derived, then `AssembleBody`. `ApplyFaceOperation`: 14 verbs over it. Topology rebuilds (`ExtrudePlanarFaces` side ring, `DeleteBodyFaces` hole heal or open sheet, `HealBody` carrier merge plus fin splicing, `ThickenSheet` inner skin plus rim faces). `HollowBody` = cavity by face change, then General Fuse difference. `DetectSelfClash`: every edge against every non-incident face | 🟢 FaceOperationValidation E1–E9: corpus section E 36/36 with **0 refusals on HoleSlab / Can (old FaceEditSolver: 17)**, chains 6/6, extras 12/12, refusals 8/8. V vs closed form ≤ 5.0e-15, edges on both carriers ≤ 2.6e-16·L, tessellation N gates (sheets by sheet rules), 0 of 2 210 winding probes wrong, chains ≡ reference ≤ 3.8e-16, deterministic, scale 10⁻³…10³ ≤ 1.2e-14 |
| M. Sketch solver | `SolveSketch`: points and curves (line, circle, arc, construction flag) with 21 constraint kinds; residuals scaled to length; minimum-norm Gauss–Newton with an LM fallback; column-pivoted QR in constraint order for rank, DOF, the redundant set (the later member of a dependent set is blamed) and conflict groups (dependent rows whose residual stays); structural rank = the rank of the same system at a deterministically perturbed generic configuration; singular when structural rank > rank or conditioning < 10⁻³ × the generic configuration; fixed-point flags from the null space. `ExtractSketchProfile`: closed loops, area, centroid; open, branching or crossing loops are refused | 🟢 SketchValidation S1–S9: 18/18 sketches, DOF / redundant / conflicts equal to the hand count, positions vs closed form ≤ 1.1e-15·L, random starts 100 %, minimum-norm motion, fixed flags, scale 10⁻³…10³ ≤ 1.8e-16, singularity flagged exactly where expected, profile area ≤ 1.3e-15, deterministic; a 122-unknown chain at DOF 0 |
| L. Persistent names + feature tree | `ExtrudeProfile`, `RevolveProfile` (`ProfileSweep`). `TopologicalNaming`: face names from operation history (`Extrude#2.Side(Line1)`, `Cap(End)`, `Blend#3.Strip(Edge(…))`, `Corner(Vertex(…))`, Boolean keeps source names, split pieces `/1`, `/2`); edge and vertex names from their faces (`Edge(A\|B)#k`); a signature (category, centroid, size) per name. `ResolveReference`: exact, then unique after stripping disambiguators, then geometric rebind (same category, size ratio ½…2, best clearly ahead), otherwise refused with the candidates. `RegenerateFeatureTree`: Sketch, Extrude, Revolve, Boolean, Blend and FaceOperation steps replayed from scratch; a refused step keeps the previous body and later steps still run | 🟢 FeatureValidation F1–F7: 7 trees, 33 regenerations, V vs closed form ≤ 2.1e-15, refused set = oracle and audit sound, 255 namings complete and unique, 1 164 survivors (74 renumbered by index, 18 renamed) all resolve to themselves, refused steps keep the previous body, expected methods (Exact 30 · Geometric 2 · Refused 3), deterministic |
| I. Rolling-ball blends | `ResolveEdgeBlendSheet`: convexity from (n₀ × n₁)·(t·σ); fillet radius r or chamfer setback d (r = d·tan(θ/2)); spine = SSI of the two offset surfaces; carrier Cylinder / Torus (fillet) or Plane / Cone / Cylinder (chamfer). `BlendEdges`: vertex classes Cap (planar end face), Mitre (two blends, elliptical seam), BallCorner (three convex fillets, sphere), Continuation; contact rails trimmed against each other; rebuilt with `AssembleBody` (shared with General Fuse), then audit, then the genus is compared. The source body is never touched | 🟢 BlendValidation B1–B10: rolling-ball harness 15/15, BevelLab 6/6, 27 specimens. ΔV vs closed form ≤ 7.0e-16, G1 ≤ 3.6e-15 rad, rolling-ball law ≤ 1.6e-15 over 71 037 samples, tessellation N gates, 0 of 2 400 winding points wrong, 4 Booleans downstream ≤ 3.4e-16, 5 atomic refusals, deterministic, ΔV ∝ s³ to 7.7e-14 |

🚧 Step-5 plan-gate gaps (refused with a 🚧 reason, never built wrong): setback vertex blends where convex meets concave (Várady–Rockwood 2n-sided), tangent-chain propagation, overflow strategies (smooth / cliff / notch), caps on curved end faces and traced (marched) spines.

🚧 Step-6 gaps (refused with a reason, never built wrong): self-intersection *removal* (clashes are detected and refused), joint extrusion of faces that share an edge, extrusion of curved faces, thickening of closed sheets and outward thickening, re-making blends across a changed face, and carriers beyond plane / cylinder / cone / sphere / torus.

🚧 Step-7 gaps (refused with a reason, never built wrong): graph-constructive decomposition of sketches (§ M 🥈; today one numerical system), sketch-on-face (sketch planes are fixed placements), references into sketch geometry other than lines, circles and arcs, feature-semantic roles beyond sweeps (§ L 🥉), and incremental regeneration (every edit replays the whole tree).

Next in the build order: step 8, kernel gaps G2–G5. See `Documentation/RESEARCH-KernelAlgorithms.md`.

## Layout

```
SolidArcRemastered/
├── CMakeLists.txt            static lib + RemasteredInspection + 10 validations (ctest)
├── Source/
│   ├── Precision/            PrecisionSpecification.h, ExactPredicates
│   ├── Geometry/             VectorAlgebra.h, CurveGeometry, SurfaceGeometry, ProjectedCurve (exact or projected SP-curves)
│   ├── Intersection/         BernsteinPolynomial, ImplicitSurface, CurveSurfaceIntersection, IsoParameterCurve, SurfaceIntersection
│   ├── Containment/          FaceDomain (chart loops, 2D winding, interior points), WindingNumber (3D GWN, ClassifySolidPoint)
│   ├── Tessellation/         ChartTriangulation (exact CDT), BodyTessellation (shared-edge sampling, per-face refinement, repairs)
│   ├── Boolean/              GeneralFuse (PaveFiller + Builder + selection/assembly), BooleanCatalogue (15 closed-form scenes)
│   ├── Topology/             RecordStore.h (tokens), ModelPartition, EulerEdits
│   ├── Construction/         PrimitiveConstruction (prism, block, cylinder, cone, frustum, sphere, torus, perforated, hollow), BodyAssembly (faces + shared edges → body)
│   ├── Blend/                RollingBallGeometry (sheets, spines, carriers), EdgeBlend (BlendEdges: vertex classes, trimming, assembly), BlendCatalogue (27 specimens)
│   ├── Local/                FaceChange (PlanFaceChange, DetectSelfClash), FaceOperations (14 verbs, HollowBody), TopologyRebuild (extrude, delete, heal, thicken), FaceOperationCatalogue (62 specimens)
│   ├── Sketch/               SketchSolver (SolveSketch, rank-revealing QR diagnosis), SketchProfile (loops, area), SketchCatalogue (18 sketches)
│   ├── Feature/              ProfileSweep (extrude, revolve), TopologicalNaming (history names, ResolveReference), FeatureTree (RegenerateFeatureTree), FeatureCatalogue (7 trees)
│   ├── Audit/                ModelAudit (T1–T6, G1–G6), EnclosedVolume (divergence-theorem quadrature), IntersectionAudit (I1–I6), TessellationAudit (N1–N7)
│   ├── Inspection/           SpecimenCatalogue, IntersectionCatalogue, InspectionEncoding, IntersectionEncoding, BooleanEncoding, TessellationEncoding, BlendEncoding, FaceOperationEncoding, FeatureEncoding, JsonWriter.h
│   └── Applications/         RemasteredInspection.cpp (stdin line protocol)
├── Validation/               Topology-, Predicate-, Intersection-, Containment-, Boolean-, Tessellation-, Blend-, FaceOperation-, Sketch-, FeatureValidation.cpp
├── Tooling/InspectionHost.py stdlib-only host: static viewer, API, SSE, rebuild on change
│   GalleryRasterizer.py      Before | After exhibit PNGs (--kind Blend | FaceOperation | Feature) straight from the C++ process (Pillow + numpy, z-buffer)
└── Viewer/index.html         self-contained canvas viewer
```

## Run

```bash
# Windows (Developer PowerShell, cmake on PATH). The host configures and builds into ./build on first start.
python Experimental/SolidArcRemastered/Tooling/InspectionHost.py
# open http://localhost:8740

# Linux sandbox (cmake and ninja from the scratch venv)
V=_ClaudeScratch/.venv/bin
python3 Experimental/SolidArcRemastered/Tooling/InspectionHost.py --cmake $V/cmake --generator Ninja --make-program $V/ninja

# Gates without the viewer
cmake -S Experimental/SolidArcRemastered -B Experimental/SolidArcRemastered/build
cmake --build Experimental/SolidArcRemastered/build --config RelWithDebInfo
ctest --test-dir Experimental/SolidArcRemastered/build -C RelWithDebInfo
```

| Host endpoint | Purpose |
|---|---|
| `GET /api/catalogue` | Specimen and scene names, `Kind` (Model / Intersection / Boolean / Blend / FaceOperation / Sketch / Feature), descriptions, default parameters |
| `GET /api/specimen?name=Torus&MajorRadius=4` | Construct + audit + encode (one line to the persistent C++ process) |
| `GET /api/specimen?name=SteinmetzCrossing&MarchingOnly=1` | Solve + audit an intersection scene (common keys: MarchingOnly, LoopDetection, ChordTolerance, Lattice) |
| `GET /api/specimen?name=NapkinRing&Operation=2` | Boolean scene: General Fuse once, all four operations assembled, the chosen one (0 ∪, 1 ∩, 2 A − B, 3 B − A) audited and encoded with history |
| `GET /api/blend?name=RoundedBlock&Size=0.2&Source=0` | Blend specimen (same verb as `/api/specimen`): blend applied (Size clamped to 10⁻⁴…10), audited and encoded with selection, sheets (carrier, radius, dihedral, residual, spine, contact edges), face provenance and statistics; `Source=1` shows the untouched source. A refusal still returns the source and the reason |
| `GET /api/faceop?name=PushHoleSlab&Amount=1&Source=0` | Face-operation specimen (same verb as `/api/specimen`): the steps are applied in order (Amount multiplies the last step's distance, angle, vector and scale change; closed forms only at 1), audited and encoded with the steps, method, edge categories, residuals, outlines of the selected faces and face provenance (Unchanged / Trimmed / Replaced / Generated); `Source=1` shows the body before. A refusal still returns the source and the reason |
| `GET /api/specimen?name=Slot&Radius=0.8` | Sketch: dimensions edited by label, solved, diagnosed (DOF, rank, structural rank, conditioning, per-constraint status and residual, redundant set, conflict groups) and the profile extracted. The expected area is withheld once a dimension is edited |
| `GET /api/specimen?name=BlockFillet&Width=5&Upto=2` | Feature tree: parameters assigned, regenerated by replay up to step `Upto`, audited and encoded with the steps (status, refusal, reference resolutions), solved sketches, and the persistent name of every face, edge and vertex |
| `GET /api/slice?name=DrilledBlock&Operation=2&Axis=2&Offset=0.5&Resolution=29` | Winding-number slice of a model or Boolean result: N × N grid, membership and winding per point |
| `GET /api/tessellation?name=NapkinRing&Operation=2&SagRatio=0.001` | Shared-edge CDT PolygonComplex of a model or Boolean result (ε = SagRatio × diagonal, clamped to 10⁻⁴…5·10⁻²): positions, outward triangles, face per triangle, edge polylines, per-face records, N1–N7 verdicts |
| `GET /api/validation?name=BooleanValidation` | Run a gate, returns markdown |
| `GET /api/status`, `GET /api/events` | Build state (Building / Ready / Failed, stale flag, log) as JSON and SSE |

⚠️ When a rebuild fails, the previous binary keeps serving, but the banner says **STALE** and shows the compiler log.

## Viewer

| Panel | Shows |
|---|---|
| Stage | Orbit wireframe with trimmed iso-hatch per face, face normals (sense × surface normal), vertices, dashed tolerance spheres with ε |
| Measured vs expected | V E F L S genus against closed-form expectations, volume error (gate 1e-12), timings |
| Audit | 12 checks with 🟢/🔴, findings table (click to locate the subject) |
| Topology | Body → … → Fin tree; hovering a row highlights that token's geometry |
| Winding-number slice | Toggle plus axis, offset and resolution: an axis-aligned grid through the body box, coloured inside / outside / boundary; table of method counts, max integer residual, evaluations, ms |
| Tessellation (N) | Toggle plus ε / diagonal and colouring (by face, or by minimum angle red 0° → green ≥ 40°). Shaded triangles with a headlight; back faces are drawn first in red, so any red that shows through marks a gap or an inverted triangle. Tables: N1–N7 gates, volume and area, then per face triangles, insertions, star · flip · split repairs and ms (hover a row to highlight the face) |
| Gates | Runs any of the 10 validations and renders the markdown |

Intersection scenes (the second group in the selector) reuse the same panels:

| Panel | Shows |
|---|---|
| Stage | Both surface patches as hatching, cut to a window around the curve; exact branches green, marched branches orange; singular points as red rings, touch points as violet rings |
| Measured vs expected | Branches, closed branches and special points against hand-derived counts |
| Metrics | Length vs exact (gate 1e-3), max residual (1e-9), reference crossings / misses, worst reference gap, solve and audit ms |
| Audit | I1 residual · I2 boxes · I3 dense-lattice completeness · I4 terminations · I5 counts · I6 length |
| Tree | Surfaces, branches (category, points, length, terminations), special points, solver statistics |

Boolean scenes (the third group) show:

| Panel | Shows |
|---|---|
| Controls | Segmented operation selector A ∪ B · A ∩ B · A − B · B − A |
| Stage | Dashed ghosts of both arguments; result hatch blue when it came from A, orange from B; section edges green |
| Measured vs expected | Faces / edges / vertices, shells and solid regions against expectations, fragments per argument and per state |
| Volume, identities and timing | Volume vs closed form (gate 1e-9), the three volume identities, fuse phases (intersect · split · classify), assembly and audit ms |
| All four operations | Volume, closed form, faces and shells for every operation; click a row to show it. Expected-empty results get a green banner |
| General Fuse | Face pairs tested, coincident pairs, section branches, EE / EF tests, paves, split edges, common blocks, section and imprinted edges, dropped cycles |
| Face history | Result face ← argument face, the state that kept it, and whether it was flipped; hovering a row highlights the face |

Blend specimens (the fourth group) show:

| Panel | Shows |
|---|---|
| Controls | Size (radius or setback), After · Before segmented control, blend-guide toggle; tessellation colouring "by blend origin" |
| Stage | Selected edges amber dashed, spines (ball centres) cyan dotted, contact rails green; hatch and shading teal for blend faces, gold for corner faces, slate for modified faces |
| Measured vs expected | Faces / edges / vertices / genus against the expected counts; V before, V shown, ΔV vs closed form (gate 10⁻⁹·max(1, V)); blend, construction and audit times |
| Blend | Profile and size, edges blended, exact carriers by type, vertex classes (cap · mitre · ball · continuation); sheets (edge, wedge θ, spine, carrier, rolling-ball residual); face provenance (face, origin, source face, surface) (hover a row to highlight the face). A refusal shows the source with a banner: green when the refusal is the expected outcome |

Face-operation specimens (the fifth group) show:

| Panel | Shows |
|---|---|
| Controls | Amount (× last step), After · Before segmented control, face-operation guide toggle |
| Stage | Outlines of the selected source faces amber dashed (the "before" ghost); hatch orange for replaced carriers, teal for trimmed faces, violet for generated faces, slate for unchanged |
| Measured vs expected | Faces / edges / vertices / genus against the catalogue; V before, V shown vs the hand-derived closed form (gate 10⁻⁹·max(1, V*)), or "open sheet"; operation, construction and audit times |
| Face operation | Steps (the refused one marked 🔴), corpus row and the old FaceEditSolver result, method, carriers replaced, vertices re-solved, edge categories, max vertex and edge residual; face provenance (hover a row to highlight the face). A refusal shows the source with a banner |

Sketches (the sixth group) show:

| Panel | Shows |
|---|---|
| Controls | One slider plus number box per dimension |
| Stage | Solved curves blue, fully fixed curves green, construction curves grey dashed |
| Measured vs expected | DOF, unknowns, equations, rank and structural rank against the hand count; profile loops and area vs closed form, or why there is none (edited dimension, refused profile) |
| Diagnosis | Converged, steps, conditioning vs generic, max residual, ms; per constraint: kind, value, status (met · redundant · conflicting) and residual; conflict groups; ⚠️ banner when singular |

Feature trees (the seventh group) show:

| Panel | Shows |
|---|---|
| Controls | One slider plus number box per parameter, and `Upto` for partial replay |
| Stage | The regenerated body with faces hued by the step that created them, solved sketches dashed violet; hovering shows the persistent name |
| Measured vs expected | Steps built, V vs the closed form (only for a full replay), F / E / V, genus |
| Feature tree | Steps with status, ms and detail; each reference with its method (🟢 exact · 🔍 geometric · 🔴 refused) and the token it resolved to; a banner for refused steps, green when the refusal is expected; the name table for faces, edges and vertices |

## Conventions

| Topic | Rule |
|---|---|
| Orientation | MEF: the old loop keeps the forward fin, the new face gets the reversed fin. KEMR on First·X·Second·Y: X stays, Y becomes the ring |
| Tokens | `{Slot, Generation}`. Reclaim increments the generation, so stale tokens fail `Authenticate` |
| Precision | `ExactPrecision` (NaN) means session precision applies. Vertex ε ≥ incident edge ε (T5). Curves meet vertices within εV + εE (G1) |
| Face splits | MEF / MFKRH give the new face a copy of the carrier surface and sense |
| Intersection output | Branches carry an exact carrier (Line / Circle) or a marched chart: points on both surfaces with (u, v) on each. Closed branches do not repeat the first point. Singular points split branches, and both pieces end there |
| Boolean selection | ∪ = A-Outside + B-Outside + A-OnSame · ∩ = A-Inside + B-Inside + A-OnSame · A − B = A-Outside + A-OnOpposite + flipped B-Inside · B − A is symmetric |
| Boolean assembly | Every pave block needs exactly two balanced uses, otherwise the result is refused as non-manifold or open. Shells come from union-find over shared edges; negative-volume shells are cavities placed in the solid that contains them. An empty selection is refused ("Boolean: empty result") |
| Tessellation | ε = SagRatio × body diagonal (default 10⁻³). An edge's samples are shared by every fin, and the face polygon uses them in fin order, so sides on edges match by construction. Triangles are outward CCW. A pole or apex is one vertex: singular-row triangles collapse onto it. The default camera looks from above (pitch < 0; the projection frame is Right × Up = −Ahead) |
| Blends | `BlendEdges` never edits its input: it builds a new body by assembly, and a refusal returns no body (the message starts with "Blend: "). Fillet Size is the ball radius r; chamfer Size is the setback d along both faces. History: each result face is Edge (blend sheet), Vertex (corner sphere) or Modified (source face, trimmed). Contact rails drawn in the viewer are the result's real edges, not the untrimmed sheet curves |
| Face changes | `ApplyFaceOperation` never edits its input; a refusal returns no body and starts with the verb ("Push: …"). Offset and push move along the outward normal (sense × surface normal); scale hinges about the boundary centre of the face; rotate turns the carrier about the face's boundary centroid (Parasolid `PK_FACE_transform` semantics); draft hinges at the neutral plane through the body's lowest point along the pull. Hollow keeps the selected faces open; thicken grows inwards. Delete heals a face that sits in a hole loop, otherwise the result is an open sheet (laminar rim edges: one fin each) |
| Sketch constraints | Residuals are lengths (angles × the sketch scale). Redundant means implied by earlier constraints and met; conflicting means dependent and unmet. The blamed member of a dependent set is the latest in constraint order. Tangency uses the endpoint-angle form, never "distance = r" (a double root) |
| Persistent names | `Operation#k` is the step label. Faces: `Side(LineN)`, `Cap(Start\|End)`, `Strip(edge)`, `Corner(vertex)`, `Generated(cause)`; a face change and a Boolean keep the source name; split pieces `/1`, `/2` ordered by position (x, then y, then z, quantised at 10⁻⁶·L); edges `Edge(faces sorted, joined by \|)`, vertices `Vertex(faces)`, with `#k` when names repeat. Resolution never picks a best guess: a tie or a weak geometric match refuses the step |
| Containment | ClassifySolidPoint: Boundary within Precision; then the nearest-normal shortcut when the nearest point is a face interior point closer than NearDistance; otherwise Σ Ω_F / 4π with |w| > ½ inside. Every result carries IntegerResidual = |w − round(w)|, and General Fuse refuses a fragment whose residual exceeds 1e-3 (argument not closed) rather than rounding it |
