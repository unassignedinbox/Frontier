# 🧩 SolidArcRemastered — evidence for steps 1–6

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
| `BooleanValidation.md` | B1 audit · B2 closed-form volume · B3 volume identities · B4 membership oracle · B5 shell/solid counts; 15 scenes, 5 random families, open-argument negative control; per-operation table | 🟢 PASS |
| `BooleanGallery/*.png` | One render per scene (result hatch by origin, section edges, argument ghosts) + 3 winding-number slices | — |
| `TessellationValidation.md` | N1 closure · N2 deviation · N3 orientation · N4 volume · N5 Delaunay · N6 edge conformity · N7 quality, per corpus row with repair counts; N8 convergence (4 specimens × 5 ε); N9 determinism; N10 scale; N11 CDT regressions | 🟢 PASS |
| `TessellationGallery/*.png` | 13 shaded PolygonComplex renders (6 specimens, 7 Boolean results) with gate figures; `SphereQuality.png` is coloured by minimum angle | — |
| `BlendValidation.md` | B1 closed-form ΔV · B2 audit, genus, face count, source untouched · B3 G1 and chamfer setback · B4 rolling-ball law · B5 tessellation · B6 winding vs clearance oracle · B7 Booleans downstream · B8 refusals · B9 determinism · B10 scale; per-specimen carriers and vertex classes | 🟢 PASS |
| `FaceOperationValidation.md` | E1 closed-form V · E2 audit, category, genus, F/E/V, source untouched · E3 edges on both carriers · E4 tessellation · E5 winding probe off every face · E6 chains vs reference · E7 refusals · E8 determinism · E9 scale; corpus-E table with the old-kernel column, chains, extras, refusals | 🟢 PASS |
| `FaceOperationGallery/*.png` | 62 Before \| After panels. Before: source with the selected faces outlined. After: replaced carriers orange, trimmed faces teal, generated faces violet, with V vs closed form, F/E/V and N gates in the caption. The 8 refusals show their reason | — |
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
python Experimental/SolidArcRemastered/Tooling/GalleryRasterizer.py      # BlendGallery (needs Pillow + numpy)
python Experimental/SolidArcRemastered/Tooling/GalleryRasterizer.py --kind FaceOperation   # FaceOperationGallery
# single specimen or prefix: FaceOperationValidation --only Push   (Scale selects E9)
# single specimen or prefix: BlendValidation --only Rounded   (Scale selects B10)
# single row or section: TessellationValidation --only RandomSpheres  (also Scale, Regressions, …)
# single scene or family: BooleanValidation --only SphereLens
```

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

🐞 Defects the gates found and fixed during step 6:

| Found by | Defect | Fix |
|---|---|---|
| E7, `RotateCanSteep` | Rotating the Can top by 60° made the tilted plane dip below the base; every edge still solved, intervals stayed positive and the audit was sound, so a self-intersecting body was returned | `DetectSelfClash`: every edge against every face that does not use it. A crossing inside the face, or on its boundary away from the edge's own vertices (edge–edge contact), refuses the change |
| E5, first run | 2 994 of 7 001 probes wrong, on curved bodies only. The probe started from triangle centroids, which sit inside the true surface by the chord sag (> ε). The meter was wrong, not the bodies | Probe from the chart centroid evaluated on the true carrier, along the true outward normal; 40 probes per body (347 s → 120 s) |
| Viewer / gallery, open sheets | Tessellation gates N1 and N4 failed on every sheet: the laminar rim sides are legitimately single and an open body encloses no volume | `TessellationAuditReport.LaminarSides`; N1 allows exactly those, N4 is n/a for open bodies |
| `ExtrudeCurved` refusal text | A cylinder's own seam edge was counted as "two selected faces share an edge" | Planarity is checked first; shared edges count distinct faces |

📝 `TessellationValidation.md` was refreshed. Rows, bodies and verdicts are unchanged, but triangle counts in the three random-Boolean rows and one digest differ from the old file. A scratch build of the previous commit reproduces today's numbers exactly, so the old file was stale from an earlier step; step 6 did not change it.

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
| Setback vertex (convex meets concave, `MixedConvexity`) | Refused at the vertex | Várady–Rockwood 2n-sided setback patch |
| Tangent-chain propagation | Edges are blended only as selected | Grow the selection across G1 edges (old-kernel option A) |
| Overflow (blend wider than its face, `FilletRailsCross`) | Refused: "contact rail runs backwards" | Smooth / cliff / notch overflow strategies |
| Caps on curved faces, traced spines | Caps only on planar ends; spines only where SSI is analytic | Marched spines and SP-curve caps (needs G traced sections on both faces) |

🚧 Known limitation (step 4): next to poles and apexes, triangle shape follows the chart's anisotropy. Sphere and Cone reach a minimum angle of 0.4–1.1°, with ≈ 21–25% of their triangles below 20° (`SphereQuality.png`). Specimens without singular points stay ≥ 14° (Torus 15.3°, Cylinder 20.9°, HollowBlock 14.0°). Boolean results also inherit their smallest *input* angle, where a section edge meets a boundary almost tangentially (DShaft 3.3°, RandomDrills 0.1°); no Delaunay refinement goes below the input angle. Every gate holds, so this is a shading-quality issue, not a correctness one. Rebuilding every star does not help. The cure is a pole-local chart (an azimuthal projection around the axis) for the singular rows.
