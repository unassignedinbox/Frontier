# 🧩 SolidArcRemastered — evidence for steps 1–4

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

Reproduce (from the repository root):

```bash
cmake -S Experimental/SolidArcRemastered -B Experimental/SolidArcRemastered/build && cmake --build Experimental/SolidArcRemastered/build
Experimental/SolidArcRemastered/build/TopologyValidation  > Exhibits/SolidArcRemastered/TopologyValidation.md
Experimental/SolidArcRemastered/build/PredicateValidation > Exhibits/SolidArcRemastered/PredicateValidation.md
Experimental/SolidArcRemastered/build/IntersectionValidation > Exhibits/SolidArcRemastered/IntersectionValidation.md
Experimental/SolidArcRemastered/build/ContainmentValidation > Exhibits/SolidArcRemastered/ContainmentValidation.md
Experimental/SolidArcRemastered/build/BooleanValidation --detail > Exhibits/SolidArcRemastered/BooleanValidation.md
Experimental/SolidArcRemastered/build/TessellationValidation > Exhibits/SolidArcRemastered/TessellationValidation.md
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

🚧 Known limitation (step 4): next to poles and apexes, triangle shape follows the chart's anisotropy. Sphere and Cone reach a minimum angle of 0.4–1.1°, with ≈ 21–25% of their triangles below 20° (`SphereQuality.png`). Specimens without singular points stay ≥ 14° (Torus 15.3°, Cylinder 20.9°, HollowBlock 14.0°). Boolean results also inherit their smallest *input* angle, where a section edge meets a boundary almost tangentially (DShaft 3.3°, RandomDrills 0.1°); no Delaunay refinement goes below the input angle. Every gate holds, so this is a shading-quality issue, not a correctness one. Rebuilding every star does not help. The cure is a pole-local chart (an azimuthal projection around the axis) for the singular rows.
