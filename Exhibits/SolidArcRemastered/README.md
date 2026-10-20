# 🧩 SolidArcRemastered — evidence for steps 1–8 and the pattern stress test

> 📝 **Showcase:** [`Exhibits/Showcase.html`](../Showcase.html) is one self-contained page (4.9 MB, no external resources) with 16 interactive kernel models (now including P7 with its rolling-ball pipes), every table below, all 151 gallery panels and the DraftingWorkspace captures. Download it and open it in any browser.

**TL;DR (exact rolling-ball carrier, option C part B)**: Fillets on traced spines are now built on an exact procedural carrier. **P7 is 🟢**, so PatternStressValidation runs 64 rows: 🟢 63 · ⚠️ 1 · **🔴 0 wrong** (610 operations, 542 s). The only refusal left is P8, now refused honestly at a degree-8 singular vertex (setback, phase A). The new [`RollingBallValidation`](RollingBallValidation.md) passes all 10 gates, and ctest passes 12/12 (654 s).

| Gate | Measures | Worst | Limit |
|---|---|---|---|
| R1 | ball: \|foot − C\| − r, \|P − C\| − r, (P − C)·T (relative) | 1.2e-13 | 1e-9 |
| R2 · R3 | rails on their supports · G1 at the rails [rad] | 4.4e-16 · 1.4e-14 | 1e-9 · 1e-8 |
| R4 · R5 | projection round trip · chart normal ∥ radial [rad] | 1.4e-15 · 2.8e-10 | 1e-9 · 1e-8 |
| R6 | line spine ≡ cylinder, circle spine ≡ torus | 8.3e-16 | 1e-12 |
| R7 | pipe ∩ normal plane closes on the section circle | 4.4e-16 | 1e-8 |
| R8 | 3 bodies sound, \|V − V_polygon\| ≤ peak deviation × area | 3 / 3 | pass |
| R9 | rotation · reflection · uniform scale there and back (V, vertices) | 8.1e-12 | 1e-9 |

- 🧩 **Carrier:** P(𝑢, 𝑣) = C(𝑢) + r(cos 𝑣 E₁ + sin 𝑣 E₂). The spine C can be any curve (traced or ellipse). E₁ points to the foot on the guide support, and E₂ = h·(E₁ × T), where the handedness h flips under a mirror. Contact rails are the feet of the spine on each support (`ContactCurve`), because tangential SSI cannot trace them. Both are exact on their definition and evaluated on demand.
- 🐞 **6 defects found and fixed** (4 of them by the new validation):
  - **Mirror** (ΔV 2.3e-3): the image chart was M·P(𝑢, −𝑣), and `MapAnchor` flipped the spine parameter. Now: handedness plus an unchanged pipe anchor.
  - **Asymmetric mouth refused:** the continuation's section went through the old vertex, off the seam. Now it sits where the rail crosses the seam (secant).
  - **Knots:** the traced spine is only C¹, so the flux density jumps there. The inner Green integral is now split at the chart breakpoints (transforms improved from 6.8e-9 to 8.1e-12).
  - **SSI missed wrap-around loops** on a pipe (no 𝑣-lattice). The second surface's interior lattice now also seeds when the first surface is a pipe; the SmallLoop negative control still fails as designed.
  - **Hang:** adaptive quadrature chased round-off to depth 24. A gap inside the 10⁻⁹ noise band that shrinks by less than ¼ per level is now accepted.
  - **Seam crossing:** the P7 mouths cross both cylinder seams at one vertex (degree 4), giving a new shared-section continuation.
- Unchanged on closed-form carriers: BlendValidation B1 7.04e-16, B3 3.61e-15, B4 1.64e-15, B10 7.69e-14. The only stress-table side effect: B4 has 6 522 triangles instead of 6 518, with the same volume.
- ⚠️ **Cost:** every pipe evaluation re-solves the traced spine, so the P7 row takes 197 s. Caching spine samples is the follow-up.
- 🚧 **Still refused:** chamfers on traced or elliptical spines (needs a ruled procedural carrier) and setback vertices (P8, A: NURBS). Overflow comes after A.

**TL;DR (robust blends)**: 6 of the 8 pattern-stress refusals are now built and pass every gate. A new T family (tangent chains) adds 8 rows, all 🟢. PatternStressValidation now runs 64 rows: 🟢 62 · ⚠️ 2 · **🔴 0 wrong** (610 operations, 469 s). The 2 remaining refusals are P7 (traced spine) and P8 (elliptical spine).

| Row | Was refused | Construction now | Now |
|---|---|---|---|
| P1 slot cusps · P6 ring | mitre between non-cylindrical fillets | Section of fillet A by the bisector plane, accepted only if it lies on fillet B (5 samples) | 🟢 |
| P2 scallops (asymmetric) | same | General A ∩ B section (analytic or traced), with the endpoints projected onto both | 🟢 |
| P3 scallop chamfers | contacts do not meet (lines only) | Contact ∩ far face (any curve), confirmed on the other contact, then the A ∩ B section | 🟢 |
| P4 spoke–hub concave fillets | edge consumed by the blend | Concave ends extend the sharp edge's neighbour instead of consuming it | 🟢 |
| P5 spoke tops ending on the hub | blend ends on a curved face | The cap is the section of the blend surface by the end face (plane, analytic or traced) | 🟢 |
| T1–T3 stadium slot rims · T4–T5 rounded-block top loop · T6 boss foot loop | (new) | Smooth joint: the blends share their cross-section at the vertex (circle or chord) | 🟢 |
| T7 / T8 one seed edge, `TangentChainEnabled` | (new) | `GrowTangentChain` adds every sharp edge reached through a G1 vertex (1 → 4 slot-rim edges, 1 → 8 rounded-block edges); the result equals T1 / T5 | 🟢 |

- All 8 T rows match their closed form to ≤ 8.5e-14, with 0 of 2 400 oracle points wrong. T4 and T5 model the rounded block exactly, so the oracle tests 300 of 300 points.
- 🐞 Defects found and fixed along the way:
  - **Newton drift** (`IntersectionCurve.cpp` `CorrectOntoBoth`): a converged point was stepped again at a tangent contact, so the endpoint error went from 1.6e-16 to 7.1e-8. The residual test now runs before the step.
  - **Smooth joints located by tangent crossings** were only √ε accurate. 10⁻⁸ chart gaps made the tessellator insert false pole rows, so T4 was not watertight. The smooth-joint construction now uses only transversal operations.
  - **Periodic fold on short seam arcs:** the projection hint now folds at the middle of the gap, `IntervalLow − ½(2π − Span)`.

**TL;DR (pattern stress, first run at `a07b72e`; see robust blends above for the current 62 rows)**: PatternStressValidation ran 56 rows: 🟢 48 · ⚠️ 8 refused · **🔴 0 wrong**, with 573 modeller operations in 413 s. Every scene uses the modeller's own arrays (`ArrayBody`, the new `CircularArrayBody`) → Boolean chains → optional fillet or chamfer.
- **Booleans hold everywhere tested (25/25 🟢):** linear arrays up to 40 holes, circular arrays up to 24, overlapping slots and rings, coplanar ribs, and 6 touching blocks merged to 6 maximal faces. Mixed chains cover coaxial bores, coincident radii, a shuffled order, intersect → 4 lumps, and 3 × 3 crossing bores (genus 15, 18 singular points).
  - |ΔV| ≤ 1.6e-13 against hand-derived closed forms; 0 of 13 800 oracle points wrong; 207 176 triangles, all watertight.
  - The tree order and the shuffled order give the same F/E and volume as the sequential chain.
- **Blends on Boolean results (22/22 🟢):** fillets and chamfers on hole rims, boss rims and feet, rib edges, plate edges, a 40-rim blend in one operation, a blend after a blend, and Boolean-after-blend. All 20 built rows are exact to the closed form, and B20 / B22 refuse as required.
  - Overlap boundary pinned: rim fillet r 0.19 (footprints 0.02 apart) is exact; r 0.21 and r 0.5 are refused.
- **Where it stopped (8 ⚠️, all named limits; 6 are now built, see robust blends):**
  - mitres between non-cylindrical fillets (slot cusps, scallops, ring);
  - a chamfer mitre whose contacts do not meet;
  - a blend ending on a curved face (spokes on a hub);
  - an edge consumed by the blend;
  - traced spines (radial bores);
  - elliptical spines (equal-radius crossings).
- 🐞 **2 defects found and fixed:**
  - a crash: an `EllipticalArc` spine reached `std::get<CircularArc>` and aborted the process;
  - a lie: overlapping neighbouring blends built an audit-sound but self-intersecting body. `BlendEdges` now runs the step-6 `DetectSelfClash` gate.

**TL;DR (step 8)**: CorpusValidation passes 79/79 rows (5 gates, 6.2 s), and the plan gate is met: **0 corpus refusals**. The old kernel refused 19 of its 176 FeatureSuitability rows; all 19 now run and match the closed form.
- **Exact:** V vs the hand-derived closed form is ≤ 3.59e-14 over 76 bodies. Every result is audit-sound with its source untouched, has 0 redundant edges, and tessellates watertight.
- **G3, CrossUnequal (traced sections):** the union is 7 / 9 / 6 (F / E / V), V = 4.0031956814. The old kernel's CrossOffset value 4.009803 was **wrong**; the true value is 4.0099890528.
- **G4, Steinmetz (singular points):** union 5.6165186405, ∩ 0.6666666667 = 16r³/3, − 2.4749259869, and the 60° oblique ∩ 0.7698003589 = 16r³/(3 sin θ). The 2 branch points become fuse vertices.
- **G5, maximal faces:** BoxUnion drops from 14 faces (old kernel) to 6. StepUnion, FlushSlotDiff and CornerUnion give L- and U-shaped maximal faces.
- **G2, exact pipe:** the corpus pipe on an arc is a partial torus, V = 0.1973920880 (old kernel: freeform, V error 2.8e-06).
- **Corpus `rotate Block`:** the result is 3.0, versus the old 2.994292. The face rotates about its centre, and a 5° tilt about the face's centre line leaves a prism's volume unchanged.
- **Regression:** BooleanValidation now runs 23 scenes (8 new) × 4 operations, with ΔV ≤ 6.5e-15 and 0 of 26 400 oracle points wrong. TessellationValidation covers 342 bodies and 725 172 triangles. All 11 validations pass.

**TL;DR (step 7)**: SketchValidation passes 18/18 sketches (9 gates, 56 ms) and FeatureValidation passes 33/33 regenerations of 7 trees (7 gates, 3.7 s).
- **Plan gate, sketches:** under-, well- and over-constrained sketches report the hand-counted DOF. The rank-revealing QR names the redundant constraint (`Parallel`, `Turn120`) and the conflict group ({V1, V2, Width, TopLength}). Touching circles are flagged ⚠️ singular, and a bow-tie profile is refused.
- **Plan gate, naming:** 1 164 survivors were tracked through every step. 74 changed their inventory index and 18 changed name (split pieces), yet every one resolves back to itself. Faces keep the exact name.
- **Plan gate, G1 closed:** `BlockFillet` (sketch → extrude → fillet) follows its sketch. Width, depth, height and radius edits regenerate with V vs closed form ≤ 2.1e-15; before step 7 the bevelled body stayed at 2.982806.
- **No silent guesses:**
  - references resolve Exact 30 · Geometric 2 · Refused 3;
  - a groove through the middle of a pushed face is refused as "ambiguous: 2 equally plausible candidates";
  - a chamfer whose hole left the plate is refused as "not found".
- **Refusals:** a refused step keeps the previous body, later steps still run, and the final body is audit-sound. Names and volume bits are deterministic.

**TL;DR (step 6)**: FaceOperationValidation passes: corpus section E 36/36, chains 6/6, extras 12/12 and refusals 8/8, with 9 gates and 0 failures in 120.6 s.
- **Gate met:** corpus section E has **0 refusals on HoleSlab and Can**; the old `FaceEditSolver` refused 17 of those rows. Closed forms are exact: V vs the hand-derived value ≤ 5.0e-15 over 51 bodies (prisms, frusta, cones, wedges, hollows).
- **One algorithm:** push, tweak, offset, scale, rotate, draft, extend, trim, offset-body, hollow, extrude, delete, heal and thicken all run through `PlanFaceChange` (new carriers → re-intersected edges and vertices → `AssembleBody`) or a topology rebuild on top of it.
- **Exact:** every edge lies on both face carriers to ≤ 2.6e-16·L. Six chains equal an independent construction to ≤ 3.8e-16.
- **Sound:**
  - every result is audit-sound, with the expected category, genus and F/E/V, and the source is untouched;
  - it tessellates watertight (open sheets by sheet rules);
  - 0 of 2 210 winding-number probes ±ε off the true faces are on the wrong side.
- **Refusals:** the 8 refusals are atomic, verb-prefixed and say why. That includes a 60° rotation that would make the Can pass through itself, caught by the new edge/face clash probe.
- **Robust:** digests are deterministic, and the relative V error is ≤ 1.2e-14 from s = 10⁻³ to 10³.

**TL;DR (step 5)**: BlendValidation passes: the rolling-ball harness 15/15, BevelLab 6/6 and all 27 specimens, with 10 gates and 0 failures in 20.2 s.
- **Exact:** ΔV matches the closed form to ≤ 7.0e-16. The worst rolling-ball law residual is 1.6e-15 over 71 037 samples (spine ↔ supports, blend face ↔ spine, corner centre ↔ 3 supports).
- **Smooth:** fillet G1 across every contact and corner edge is ≤ 3.6e-15 rad.
- **Sound:**
  - every blended body is audit-sound, keeps its genus and leaves the source untouched;
  - it tessellates watertight;
  - it classifies points like the analytic clearance oracle (0 of 2 400 wrong);
  - it Booleans downstream to ≤ 3.4e-16.
- **Refusals:** the 5 refusals are atomic and say why: oversize, wider than an edge, rails cross, convex meets concave.
- **Robust:** digests are deterministic, and ΔV ∝ s³ holds to 7.7e-14 from s = 10⁻³ to 10³.

The old kernel's `RollingBallBlend` stopped at ~1e-6 residuals; carriers here are exact Cylinder, Torus, Cone, Plane and Sphere surfaces built from the spine (the intersection of offset surfaces).

**TL;DR (step 4)**: TessellationValidation passes 0 failing rows over 305 bodies and 668 918 triangles: the 9 specimens, 15 Boolean scenes × 4 operations, and 240 random Boolean results. Every PolygonComplex:
- is closed, with 0 unpaired sides, χ equal to the B-rep's and 0 unused vertices;
- stays within ε of its surfaces (peak/ε ≤ 1.000);
- has 0 inverted, zero-area or folded triangles;
- matches the exact volume to ≤ 0.48 ε·A;
- uses every edge-sample side exactly once per fin.

Convergence slopes are ≈ 1 (volume) and ≈ −1 (triangle count). Digests are deterministic, and the triangle count is scale-free from 10⁻³ to 10³. The 201 CDT regression polygons all pass. PredicateValidation adds two InCircle families: exact makes 0 errors where naive gets 13.6% of near-cocircular cases wrong.

**TL;DR (step 3)**: Both new gates pass.
- **BooleanValidation** passes 21/21 rows with 2 841 checks:
  - 15 closed-form scenes × 4 operations, plus 160 random pairs;
  - every result is audit-sound;
  - volume error ≤ 3.6e-15 and identity residual ≤ 5.7e-14;
  - 0 of 22 560 membership-oracle points wrong;
  - an open argument is refused.
- **ContainmentValidation** passes 14/14 rows: 0 points misclassified, from random points down to normal offsets of 1e-7, with integer residual ≤ 3.2e-13.

**TL;DR (step 2)**: IntersectionValidation passes 22/22 rows. Root finding stays within the condition bound on 60 000 polynomials, curve–surface intersection misses 0 of 3 183 sign changes, analytic SSI is exact to ≤ 5.4e-15, marching stays on the exact curves, and all 11 scenes are sound under both policies. The audit catches the deliberately blinded small-loop run.

**TL;DR (step 1)**: Both gates pass. 9 specimens are exact to ≤ 1.4e-15 relative volume error, every one tears down to an empty partition, all 6 deliberate corruptions are caught, and the exact predicates make 0 errors where the naive determinant gets 45.6% of exactly coplanar cases wrong.

| File | Content | Verdict |
|---|---|---|
| `TopologyValidation.md` | A specimens, B teardown, C tokens, D shell reversal, E tolerant modelling, F audit sensitivity | 🟢 PASS |
| `PredicateValidation.md` | 6 × 200 000 cases (Orient2D, Orient3D, InCircle) against an independent 256-bit integer oracle | 🟢 PASS |
| `SpecimenTimings.md` | Counts, volume, error, audit verdict and timings per specimen | 🟢 all sound |
| `IntersectionValidation.md` | E1 Bézier clipping · E2 curve–surface (exact table + random sign scan) · F1 analytic SSI · F2 marching vs exact · F3 gallery, negative control, random oblique pairs | 🟢 PASS |
| `IntersectionScenes.md` | 11 scenes × {Hybrid, MarchingOnly}: counts, length vs exact, residual, reference crossings, timings | 🟢 22/22 sound |
| `IntersectionGallery/*.png` | Renders of the viewer JSON for all 11 scenes | — |
| `ContainmentValidation.md` | H1 random points (8 specimens) · H2 ± normal offsets 1e-3…1e-7 (with and without the nearest-normal shortcut) · H3 on-boundary · H4 ray-cast parity · H5 open box vs Van Oosterom–Strackee · H6 2D holes | 🟢 PASS |
| `BooleanValidation.md` | B1 audit · B2 closed-form volume · B3 volume identities · B4 membership oracle · B5 shell/solid counts; 23 scenes, 5 random families, open-argument negative control; per-operation table | 🟢 PASS |
| `BooleanGallery/*.png` | One render per scene (result hatch by origin, section edges, argument ghosts) + 3 winding-number slices. The 8 step-8 scenes (CrossUnequal, CrossOffset, SteinmetzSolid, SteinmetzOblique, BlockFlushHalf, BlockStep, BlockFlushSlot, BlockFlushCorner) are shaded 2 × 2 sheets of ∪ ∩ − −′: faces tinted by argument (A blue, B orange), section edges gold; a merged maximal face keeps one source tint | — |
| `CorpusValidation.md` | R1 refusals · R2 closed-form V · R3 audit, category, sources untouched · R4 redundant edges (maximal faces) · R5 tessellation; one row per FeatureSuitability corpus command with the old kernel's verdict, plus section X (G4 / G5 scenes, mirror, full-turn pipe) | 🟢 79/79, 0 refused |
| `PatternStressValidation.md` (64 rows) | Arrays → Boolean chains → blends: audit, genus, lumps, closed-form V (10⁻⁹·max(1, V*)), 300-point CSG oracle, watertight tessellation, redundant edges (L/C/M); families L linear · C circular · M mixed chains · B blends with closed forms · T tangent chains · P probes; a refusal is ⚠️, a built wrong body 🔴 | 🟢 62 · ⚠️ 2 · 🔴 0 |
| `ModelGallery/*.png` | Shaded step-8 specimens (PipeBend, LoftFrustum, SweptRing, MirroredCross, ArrayedPins), one hue per face, with V, error, audit and tessellation verdicts | — |
| `TessellationValidation.md` | N1 closure · N2 deviation · N3 orientation · N4 volume · N5 Delaunay · N6 edge conformity · N7 quality, per corpus row with repair counts; N8 convergence (4 specimens × 5 ε); N9 determinism; N10 scale; N11 CDT regressions | 🟢 PASS |
| `TessellationGallery/*.png` | 13 shaded PolygonComplex renders (6 specimens, 7 Boolean results) with gate figures; `SphereQuality.png` is coloured by minimum angle | — |
| `BlendValidation.md` | B1 closed-form ΔV · B2 audit, genus, face count, source untouched · B3 G1 and chamfer setback · B4 rolling-ball law · B5 tessellation · B6 winding vs clearance oracle · B7 Booleans downstream · B8 refusals · B9 determinism · B10 scale; per-specimen carriers and vertex classes | 🟢 PASS |
| `FaceOperationValidation.md` | E1 closed-form V · E2 audit, category, genus, F/E/V, source untouched · E3 edges on both carriers · E4 tessellation · E5 winding probe off every face · E6 chains vs reference · E7 refusals · E8 determinism · E9 scale; corpus-E table with the old-kernel column, chains, extras, refusals | 🟢 PASS |
| `FaceOperationGallery/*.png` | 62 Before \| After panels. Before: source with the selected faces outlined. After: replaced carriers orange, trimmed faces teal, generated faces violet, with V vs closed form, F/E/V and N gates in the caption. The 8 refusals show their reason | — |
| `SketchValidation.md` | S1 DOF, redundant set, conflict groups vs hand count · S2 positions vs closed form · S3 random starts · S4 minimum-norm motion · S5 fixed-point flags · S6 scale · S7 singularity · S8 profile area and refusals · S9 determinism; corpus table with n, m, rank (structural), conditioning, blame | 🟢 PASS |
| `FeatureValidation.md` | F1 V vs closed form · F2 refused set = oracle, audit · F3 names complete and unique · F4 survivors resolve to themselves · F5 refused step keeps the body · F6 resolution methods · F7 determinism; regeneration table, oracles, survivor naming table, name samples | 🟢 PASS |
| `FeatureGallery/*.png` | 7 Authored \| Regenerated panels: faces hued by the step that created them, solved sketches dotted violet, refused steps listed with their reason; V vs closed form and resolution methods in the caption | — |
| `BlendGallery/*.png` | 27 Before \| After panels. Before: source and selection. After: blend faces teal, corner spheres gold, spines cyan, contact rails green, with ΔV vs closed form, carriers and N gates in the caption. The 5 refusals show their reason | — |

Reproduce (from the repository root):

```bash
cmake -S Experimental/SolidArcRemastered -B Experimental/SolidArcRemastered/build && cmake --build Experimental/SolidArcRemastered/build
Experimental/SolidArcRemastered/build/TopologyValidation  > Exhibits/SolidArcRemastered/TopologyValidation.md
Experimental/SolidArcRemastered/build/PredicateValidation > Exhibits/SolidArcRemastered/PredicateValidation.md
Experimental/SolidArcRemastered/build/IntersectionValidation > Exhibits/SolidArcRemastered/IntersectionValidation.md
Experimental/SolidArcRemastered/build/ContainmentValidation > Exhibits/SolidArcRemastered/ContainmentValidation.md
Experimental/SolidArcRemastered/build/BooleanValidation --detail > Exhibits/SolidArcRemastered/BooleanValidation.md
Experimental/SolidArcRemastered/build/TessellationValidation > Exhibits/SolidArcRemastered/TessellationValidation.md
Experimental/SolidArcRemastered/build/BlendValidation > Exhibits/SolidArcRemastered/BlendValidation.md
Experimental/SolidArcRemastered/build/FaceOperationValidation > Exhibits/SolidArcRemastered/FaceOperationValidation.md
Experimental/SolidArcRemastered/build/SketchValidation > Exhibits/SolidArcRemastered/SketchValidation.md
Experimental/SolidArcRemastered/build/FeatureValidation > Exhibits/SolidArcRemastered/FeatureValidation.md
Experimental/SolidArcRemastered/build/CorpusValidation > Exhibits/SolidArcRemastered/CorpusValidation.md
Experimental/SolidArcRemastered/build/PatternStressValidation > Exhibits/SolidArcRemastered/PatternStressValidation.md   # ~8 min; --only L|C|M|B|T|P or a row code, --verbose for phase timings
python Experimental/SolidArcRemastered/Tooling/GalleryRasterizer.py      # BlendGallery (needs Pillow + numpy)
python Experimental/SolidArcRemastered/Tooling/GalleryRasterizer.py --kind FaceOperation   # FaceOperationGallery
python Experimental/SolidArcRemastered/Tooling/GalleryRasterizer.py --kind Feature   # FeatureGallery
python Experimental/SolidArcRemastered/Tooling/GalleryRasterizer.py --kind Model --output Exhibits/SolidArcRemastered/ModelGallery --only PipeBend LoftFrustum SweptRing MirroredCross ArrayedPins
python Experimental/SolidArcRemastered/Tooling/GalleryRasterizer.py --kind Boolean --output Exhibits/SolidArcRemastered/BooleanGallery --only CrossUnequal CrossOffset SteinmetzSolid SteinmetzOblique BlockFlushHalf BlockStep BlockFlushSlot BlockFlushCorner
# single corpus section: CorpusValidation --only C   (X = step-8 extensions)
# single tree: FeatureValidation --only Boss
# single specimen or prefix: FaceOperationValidation --only Push   (Scale selects E9)
# single specimen or prefix: BlendValidation --only Rounded   (Scale selects B10)
# single row or section: TessellationValidation --only RandomSpheres  (also Scale, Regressions, …)
# single scene or family: BooleanValidation --only SphereLens
```

🐞 Defects the pattern stress test found and fixed:

| Row | Before | Cause | Fix | After |
|---|---|---|---|---|
| P8 equal cross-bores + fillet | 🔴 process aborted: `std::bad_variant_access` | Offsets of two equal cylinders meet in exact ellipses; step ④ accepted any analytic branch as the spine, and steps ⑤–⑥ assume a line or circle | `RollingBallGeometry.cpp` ④ keeps line and circle branches only; an ellipse is refused as "spine is an ellipse (… 🚧)" | ⚠️ honest refusal |
| B20 / B22 rim fillets r 0.5 / 0.21 on holes at pitch 1 | 🔴 built: V equals the naive sum of 4 disjoint tori (double-counted overlap), tessellation not watertight, audit sound | No global clash test after blending; the per-face audit cannot see two blend faces crossing | `EdgeBlend.cpp` ⑥ runs `DetectSelfClash` (step 6) on the assembled body and refuses "neighbouring blends overlap: E… passes through F…" | 🟢 refused as expected; r 0.19 still exact |

Harness fixes (no modeller change): edge criteria that also caught rib-top end edges, bore seam lines and the hub seam (count mismatches on B9, B11, B12, B15, B16 and P4). The membership oracle now uses relaxed quadrature (samples sit ≥ 10⁻³ from every primitive) because traced faces took over 150 s per row.

🐞 Defects the gates found and fixed during step 1:

| Found by | Defect | Fix |
|---|---|---|
| B teardown (Torus, PerforatedBlock) | Faces created by MFKRH / MEF had no surface, so T6 fired mid-edit | New face inherits a copy of the carrier surface. T6 runs only in Complete scope |
| B teardown (PerforatedBlock) | Stalled at two isolated-vertex loops in one face | Teardown uses MEKR, then KEV (and KFMRH for edge-less faces) |
| HollowBlock teardown | Cavity shell could not be separated without hand-editing stores | New `DetachShell` edit |
| PredicateValidation (first draft) | 3D families snapped to a 2⁻³⁰ grid were well-conditioned (naive never wrong), so the row proved nothing | Full 52-bit coordinates, 256-bit oracle |

🐞 Defects the gates found and fixed during step 2:

| Found by | Defect | Fix |
|---|---|---|
| Smoke (Steinmetz, Viviani) | Marching stepped straight over singular crossings (the sine only dips), so branches did not split | `SplitAtSingularPoints`: local sine minima → Gauss–Newton refinement → split with Singular terminations |
| I3 completeness (VivianiCurve) | A segment next to the inserted singular point bulged 1.8e-3 (allowance 1.5e-3); it was never chord-checked | `RefineSingularChord` bisects those segments with the corrector |
| E1 first draft | A fixed 1e-11 allowance failed clustered roots whose location is limited by coefficient rounding, so the oracle was at fault, not the solver | Per-root Wilkinson condition bound. Worst observed is 0.05 × bound for simple roots |
| Gallery render (SmallLoop) | The radius-50 cylinder patch swamped the view | Hatch is cut to a world window around the curve; spans up to a full turn are kept whole |

🐞 Defects the gates found and fixed during step 3:

| Found by | Defect | Fix |
|---|---|---|
| H2 (offsets 1e-7) | Pure winding near a surface: panel values reach ~2/d, so an absolute tolerance is unreachable (recursion to depth 40, ~1e8 evaluations) | Relative panel floor, plus the nearest-normal shortcut. It is exact because the segment to the nearest face-interior point crosses no boundary |
| Box–box union probe | Interior probes on the v = ½ scanline ran along section edges of split faces, so 3 fragments were classified OnOpposite and the union was refused | `LocateFaceInterior` scans irrational fractions and requires an integral 2D winding |
| SphereCraterTilted | A hole cycle was given to its own twin outer cycle: its sample lay exactly on a bisection node, where the swept angle is 0, not ½. The result was a degenerate fragment | The witness is a hole point offset slightly to the left of its traversal (the material side) |
| SteppedShaft (`bad_alloc`) | A line parallel to a cylinder axis makes F constant; root isolation of F′ (zero or rounding noise) reported roots in every subinterval. This was a latent step-2 CSI defect | The zero polynomial has no isolated roots, and the extremum search is skipped when F′ is negligible against L^d |
| RandomSpheres (ΔV 1.3e-6, identities 5e-15) | Fixed Gauss panels under-resolved projected SP-curves passing near a sphere chart pole, where 𝑢 swings by ~π. The Boolean was right and the meter was wrong | Adaptive panel-vs-halves Gauss in `EnclosedVolume`: ΔV ≤ 3.6e-15 |
| README review (OpenArgument) | Fragment classification thresholded the winding at ½ even when it was not integral | General Fuse refuses residuals > 1e-3; the negative control now refuses with w = 0.075 |

🐞 Defects the gates found and fixed during step 4:

| Found by | Defect | Fix |
|---|---|---|
| Probe (sphere, cone) | Chart triangles in a fan around a pole kept their chart orientation but flipped in 3D. Fold-driven centroid insertion ran away, even when capped | Singular rows get 4× density, and a star with a folded triangle is rebuilt by an exact CDT of its link projected along the axis |
| RandomSpheres (99 zero-area triangles) | A section circle passing 0.17 rad from a pole bends sharply in the chart. Quality refinement gated on the *shortest* boundary segment cascaded into one point | Quality floor at ¼ of the median boundary segment; edges are also sampled for chart fidelity (the chart chord must map within ε of the curve on every fin) |
| RandomSpheres #12 (spiral into a segment) | Sag was measured against S(chart lerp). Next to a constrained side this includes the boundary chord's parametric distortion (≈ 1.2 ε), which no split removes | Sag is the distance to the surface (as audited), with a ¼·√(8ε/κ) termination guard |
| N6 = star repairs in every failing row | A rebuilt pole star lost P's seam-edge side | P → X sides that are edge samples are constrained in the star CDT |
| RandomSpheres #2, #11 | The vertex-level link of a folded star can branch, and the chart star can pinch or enclose a hole | The link comes from the chart boundary of the star region, which grows over pinch and hole points; enclosed vertices become Steiner points |
| RandomSpheres #2 (slivers off the pole) | Chart-CCW slivers near a pole flipped in 3D but touched no singular vertex | 3D side flips of folded facets across non-edge sides, when both new facets are unfolded |
| RandomBlockSphere #4 (unpaired side) | Micro-lens: a sphere cap poking less than ε through a block face shares its whole 8-point boundary with the plane face, and both CDTs chose the same diagonal | Interior sides are claimed body-wide, and a later face splits a claimed side at its surface midpoint |
| RandomBlockSphere #8 (refused) | `FindSide` rotated around an enclosing-triangle corner in one direction only; that fan is open, so a valid crossed side "vanished" during constraint recovery | Rotate both ways; a crossing side queued twice is skipped. Both are pinned by N11 |
| Gallery render (DrilledBlock) | The viewer's default pitch put the eye *below* the model (Right × Up = −Ahead); a wireframe cannot show this, a shaded render can | Default pitch −0.45 (eye above); back faces drawn first in red |

🐞 Defects the gates found and fixed during step 5:

| Found by | Defect | Fix |
|---|---|---|
| B10 scale, mm-scale probe | Volume quadrature was not scale-free: an absolute tolerance, a G6 audit floor of `SessionPrecision` that refused mm-sized parts, and a π/16 panel span applied to *length* parameters | Relative panel criterion plus a shell-level floor of 10⁻¹⁵·Σ\|f\| (`MeasureShellVolume` returns Volume and FluxMagnitude); G6 fails only when \|V\| ≤ 10⁻¹⁰·FluxMagnitude. ΔV is now exactly ∝ s³ |
| Quadrature fix, first draft | A relative-only criterion never terminates on rounding-noise integrands; a *per-face* floor fails on a plane through the origin, whose flux density is identically 0 | The floor is shell-level, not per face. `PanelLimit` is 64 |
| B3 G1, first run | The measured angle bottomed out at 2.1e-8 rad: `acos(dot)` cannot resolve angles below √(2ε). The meter was wrong, not the surfaces | `atan2(\|a × b\|, a · b)`: worst 3.6e-15 rad |
| Gallery render (RoundedBlock) | Spines and contact rails were drawn from the *untrimmed* sheet curves and overshot every corner by the setback | The encoder emits the result's real contact edges (blend face ∩ support) and traces the spine alongside them |

🐞 Defects the gates found and fixed during step 7:

| Found by | Defect | Fix |
|---|---|---|
| S1, conflict groups | LM minimised the merit over all rows, so the conflict was smeared over every constraint and none stayed clearly unmet | The merit runs over the independent rows only; dependent rows are diagnosed afterwards (redundant if met, conflicting if not) |
| S2, tangent arc | Tangency posed as "line at distance r" plus "endpoint on the arc" is a double root, so J loses rank at the solution and the tangency looks redundant | Endpoint-angle form: the line direction is perpendicular to the radius at the shared endpoint |
| S1, redundant blame | Max-norm column pivoting blamed an arbitrary member of a dependent set, not the constraint the user added last | Ordered pivoting: the latest constraint in a dependent set is blamed |
| S6 / S7, singularity | An absolute conditioning floor is not scale-free and cannot separate a near-singular sketch from a merely skewed one | Conditioning ratio vs the same sketch at a generic configuration (< 10⁻³ = singular) |
| Viewer, ConflictingRectangle | The area row said "dimension edited" for sketches with no closed form at all | The encoder writes `Summary.Edited`; the viewer says "no closed form" otherwise |

🐞 Defects the gates found and fixed during step 6:

| Found by | Defect | Fix |
|---|---|---|
| E7, `RotateCanSteep` | Rotating the Can top by 60° made the tilted plane dip below the base; every edge still solved, intervals stayed positive and the audit was sound, so a self-intersecting body was returned | `DetectSelfClash`: every edge against every face that does not use it. A crossing inside the face, or on its boundary away from the edge's own vertices (edge–edge contact), refuses the change |
| E5, first run | 2 994 of 7 001 probes wrong, on curved bodies only. The probe started from triangle centroids, which sit inside the true surface by the chord sag (> ε). The meter was wrong, not the bodies | Probe from the chart centroid evaluated on the true carrier, along the true outward normal; 40 probes per body (347 s → 120 s) |
| Viewer / gallery, open sheets | Tessellation gates N1 and N4 failed on every sheet: the laminar rim sides are legitimately single and an open body encloses no volume | `TessellationAuditReport.LaminarSides`; N1 allows exactly those, N4 is n/a for open bodies |
| `ExtrudeCurved` refusal text | A cylinder's own seam edge was counted as "two selected faces share an edge" | Planarity is checked first; shared edges count distinct faces |

📝 `TessellationValidation.md` was refreshed. Rows, bodies and verdicts are unchanged, but triangle counts in the three random-Boolean rows and one digest differ from the old file. A scratch build of the previous commit reproduces today's numbers exactly, so the old file was stale from an earlier step; step 6 did not change it.

🐞 Defects the gates found and fixed during step 8:

| Found by | Defect | Fix |
|---|---|---|
| Closed-form volume, traced sections | Quadrature over an intersection curve lost digits at the guide's knots (the Newton-projected curve has a kink in its derivative there) | Volume integration splits at the knots; the acceptance was not loosened |
| F3, SteinmetzOblique | Special points were classified against the chart polyline, which misses a crossing of an oblique ellipse by more than the acceptance | Exact branches are measured on their carrier |
| ctest, sphere Boolean results | A sphere's pole-to-pole seam looked like a spur to slit removal | Slit removal requires chart-curve coincidence and only touches merged faces |
| Gallery (difference panels) | Faces were tinted by token, but `specimen` and `tessellate` are separate host runs with different tokens | Faces are matched by geometry: each face's on-face anchor goes to the nearest tessellated triangle |

🚧 Plan-gate gaps (step 8): these cases are refused cleanly, with a reason:

| Case | Today | Planned |
|---|---|---|
| Pipe along a general path | Lines and arcs only (exact cylinder / torus) | Swept surface with a rotation-minimising frame |
| Loft of non-coaxial sections, oblique sweep | Refused | Skinned surface (needs a spline carrier) |
| Non-similar transforms of curved bodies | Refused: the carriers would leave the kinds | Spline carriers |
| Analytic singular points | Equal crossing cylinders; other cases rely on marching, which refuses when it runs out of points | Collinear-normal point solve and branch splitting for general quadric pairs |
| Face merge across closed loops (KEMR) | Planes only | Periodic carriers with a seam anchor |

🚧 Plan-gate gaps (step 7): these cases are refused cleanly, never guessed:

| Case | Today | Planned |
|---|---|---|
| Large sketches | One numerical system (fine to 122 unknowns, 15 ms) | Graph-constructive decomposition (§ M 🥈, Owen / Fudos–Hoffmann) |
| Sketch on a face | Sketch planes are fixed placements | Sketch placement as a resolved face reference |
| Ambiguous references | Refused, listing ≤ 4 candidates | User repair dialogue in DraftingWorkspace |
| Incremental regeneration | Full replay on every edit | Replay from the first changed step |

🚧 Plan-gate gaps (step 6): these cases are refused cleanly, with the source intact:

| Case | Today | Planned |
|---|---|---|
| Self-intersection removal | Detected and refused | Trim / repair / extend (Parasolid), or an offset solid + regularised Boolean (§ J fallback) |
| Topology change during a face change | An edge that would run backwards is refused | Edge collapse and face split (local Boolean) |
| Joint extrusion, curved extrusion | Refused | Shared side faces; a ruled side ring on curved boundaries |
| Thicken outward or both ways, closed sheets | Inward only; open sheets only | Two-sided skins, closed sheets via hollow |
| Blends across a changed face | Not re-made | Re-run `BlendEdges` on the history of blend faces (§ K step 3) |

🚧 Plan-gate gaps (step 5): these cases are refused cleanly, with a 🚧 reason and the source intact. They are not built yet:

| Case | Today | Planned |
|---|---|---|
| Setback vertex (convex meets concave, `MixedConvexity`; P8 degree-8 singular crossings) | Refused at the vertex | Várady–Rockwood 2n-sided setback patch (A: NURBS) |
| Tangent-chain propagation | ✔️ Built: `GrowTangentChain` / `BlendSettings.TangentChainEnabled`; DraftingWorkspace Alt+click | — |
| Overflow (blend wider than its face, `FilletRailsCross`) | Refused: "contact rail runs backwards" | Smooth / cliff / notch overflow strategies |
| Traced spines | ✔️ Fillets built on the exact procedural rolling-ball carrier (P7, `RollingBallValidation`); chamfers on traced or elliptical spines refused | Ruled procedural carrier for chamfers |

🚧 Known limitation (step 4): next to poles and apexes, triangle shape follows the chart's anisotropy. Sphere and Cone reach a minimum angle of 0.4–1.1°, with ≈ 21–25% of their triangles below 20° (`SphereQuality.png`). Specimens without singular points stay ≥ 14° (Torus 15.3°, Cylinder 20.9°, HollowBlock 14.0°). Boolean results also inherit their smallest *input* angle, where a section edge meets a boundary almost tangentially (DShaft 3.3°, RandomDrills 0.1°); no Delaunay refinement goes below the input angle. Every gate holds, so this is a shading-quality issue, not a correctness one. Rebuilding every star does not help. The cure is a pole-local chart (an azimuthal projection around the axis) for the singular rows.
