# 📝 PLAN — Vehicle surfacing: Blender edge loops → fitted curves → CAD surfaces (V1–V8 built)

**TL;DR** — The vehicle already exists as a quad model in Blender. Its edge loops already carry the design intent: character lines, wheel arches, window outlines and sections. The plan reuses those loops instead of remodelling. Each chosen loop is exported as an ordered point chain, fitted with a fair B-spline in the kernel, and the curves are lofted or network-surfaced into exact NURBS faces that sew into a sheet body. The car is then mirrored and closed into a solid, ready for the fillets and features the kernel already has.
- 🟢 **Already there:** NURBS curves / surfaces (construct, evaluate, project, iso-curves, Bezier decomposition), planar spline sketches (Hermite · Bezier · NURBS · cubic, Phase B), traced surface ∩ surface sections, Booleans with NURBS faces, `BodyCategory::Sheet`, the persistent-name feature tree, Plasticity keymap.
- 🚧 **Missing (this plan):** point import, 3D (non-planar) curves, curve fitting with a tolerance, loft, curve-network surfaces, continuity across patches, sewing sheets, mirror, evaluation tools (curvature combs, zebra, deviation from the Blender mesh).
- ⚠️ **Hard parts:** G2 across patch boundaries, N-sided corners (5 or more curves meeting), and blends / offsets on NURBS supports, which are refused today because an exact rational offset does not exist.
- 🏁 **First useful result (V1–V4):** import → fit → loft one panel (the bonnet / hood) and measure its deviation from the Blender surface.
- 🟢 **Status (2026-10-21): V1–V4 built.** Synthetic bonnet: 286 noisy limit points fit into 44 poles (worst 0.056 mm). The 6-section loft lies within 0.076 mm of every limit point, including 5 loops it never saw, and within 0.053 mm of the analytic surface. Mode 1 pole drag regenerates the panel, and undo restores it bit-identically. Gates: SurfacingValidation S1–S5, L1–L6 · DraftingDocumentValidation W11 · `VehicleSurfacingWalkthrough.txt` 53 / 53 with frames. ⚠️ The Blender add-on has not been run in Blender (no Blender in the sandbox). Usage: `Experimental/VehicleSurfacing/README.md`.
- 🟢 **Status (2026-10-21): V5–V7 built.** Kernel gates are PanelSurfacingValidation N1–N6, C1–C6, A1–A6 and T1–T4 (22/22); the workspace gate is W12 (11 checks); the walkthrough is `PanelAssemblyWalkthrough.txt` (57 checks, captures 60–67). Results:
  - Gordon network on the bonnet: 0.065 mm from the analytic surface.
  - G1 across a shared section: 1.9e-15 rad. G2: curvature jump 3e-12.
  - Mirror seam with the look-ahead tangent: 0 to 1e-15 rad.
  - Lofted half arcs, mirrored and capped, give a cylinder and a frustum to 1.6e-15 relative volume error.
  - The noisy pod becomes a solid within 4.8e-4 of ∫πab dy.
- 🟢 **Status (2026-10-21): V8 built.** The **Zebra** toolbar toggle draws 64 reflection stripes in the Slang surface shader and switches to a dense tessellation (sides ≤ 2 % of the diagonal, about 60× the triangles). The **Deviation** toggle draws a needle from every imported limit point to its closest point on the panels, coloured green → red over 0 … 0.2 mm, with a legend (worst, mean, magnification). Gate W13 (6 checks) and `SurfaceEvaluationWalkthrough.txt` (28 checks, captures 70–73). Results:
  - Deviation on the six-section bonnet: 286 needles, worst 0.076 mm, mean 0.018 mm — the same worst as W11. Needles are true closest points (|gap × N| / |gap| ≤ 5.5e-8) and never longer than the best of a 61 × 61 lattice.
  - Zebra across Section3 after a 20 mm pole lift: G0 phase jump 1.07 stripes (visibly broken, capture 70), G2 1.8e-13 stripes (continuous, capture 71).
  - Smooth data joins almost G1 by itself (8.5e-4 rad, C3), so the G0 / G2 comparison needs the pole lift to show anything.

---
- 🟢 **Status (2026-10-21): blueprint study (BMW M3 E90).** Built from an internet three-view to test the loft. Gaps hit and filled: image planes (B1), GIF pictures (B2), faired traced sections (B3: the fitter rightly refused a 31.8° kink), box selection (B4), Construct cylinder **Axis x / y** (B5), and a two-tier fragment winding in GeneralFuse (B6, 76 s → 12 s per freeform Boolean, same volume). Results: 24 sections → loft → mirror → close gives a sound 7.148 m³ solid; 4 wheel arches give a sound 6.790 m³ solid. Still open: incremental replay (B7), warm-started SSI marching (B8), tip / non-planar caps (B9), trim for details (B10). Gate W14; `BlueprintModellingWalkthrough.txt`; captures 80–88; table in `Experimental/VehicleSurfacing/README.md`.

## 1. Goal and constraints

| # | Requirement (from the request) | Consequence |
|---|---|---|
| 1 | Do not remodel; reuse the existing lines / curves | Input = edge loops picked in Blender, not new sketches |
| 2 | CAD-level control and precision; tweaking must be quick | Few control points per curve (fitted, not one pole per vertex); surfaces driven by those curves, so editing a curve regenerates the panel |
| 3 | B-spline / Hermite curves from the points | Kernel fitting with a tolerance; Hermite and cubic interpolation already exist for clean data |
| 4 | Loft to recreate the vehicle | Loft (skinning) first, then curve networks where a panel has curves in two directions |
| 5 | Same quality bar as the kernel | Every step has a validation gate; refuse rather than build a wrong surface |

---

## 2. Pipeline

```text
 Blender quad model                                     DraftingWorkspace / SolidArcRemastered
 ──────────────────                                     ────────────────────────────────────────
 pick edge loops ──► VehicleCurves.toml ──► ① import ──► ② fit (tolerance, fairing, corners)
 (limit positions)   ordered points,         point        │
                     closed flag, names      chains       ▼
                                                     ③ 3D curves in the feature tree (editable poles)
                                                          │
                                         ┌────────────────┼──────────────────┐
                                         ▼                ▼                  ▼
                                    ④ loft (sections)  ⑤ network (2 families)  sweep (existing PathSweep)
                                         └────────────────┼──────────────────┘
                                                          ▼
                                     ⑥ continuity: G1 / G2 across shared curves
                                                          ▼
                                     ⑦ trim + sew → sheet body → mirror → close → solid
                                                          ▼
                                     ⑧ evaluate: combs · zebra · deviation map vs the Blender mesh
```

---

## 3. Export from Blender

- A small Blender Python add-on (kept in `Experimental/VehicleSurfacing/Blender/`, not run in this sandbox) exports the **selected edge loops** as ordered vertex chains: object name, loop name, closed flag, world-space points in metres.
- **Use limit positions, not cage vertices.** A subdivision-surface cage does not lie on its own surface. The add-on evaluates the mesh with the Subdivision modifier (level 2–3) and walks the matching loop. This gives points on the surface the user actually sees.
- **Symmetry:** export one half. Points within a tolerance of the mirror plane (x = 0) are snapped onto it, and the curve end tangent is later constrained perpendicular to it (G1 across the mirror).
- **Format:** TOML (the repository convention, toml++ already linked):

```toml
Format = "VehicleCurves"
Units  = "m"
[[Curves]]
Name   = "ShoulderLine"
Closed = false
Points = [ [0.000, 0.812, 0.905], [0.150, 0.815, 0.907], … ]
```

| Format option | Readable | Already parsed | Keeps names / closed flag | Rating |
|---|---|---|---|---|
| TOML (above) | ⭐⭐⭐ | ✔️ toml++ | ✔️ | 🥇 |
| JSON | ⭐⭐ | needs a parser | ✔️ | 🥈 |
| OBJ polylines (`l` records) | ⭐ | needs a parser | names only through groups | 🥉 |

---

## 4. Curve fitting

Mesh loops are noisy at the millimetre level and have one vertex per quad: interpolating them exactly copies the faceting into the curve. Fitting turns 40–200 points into 6–20 poles that a designer can drag.

| Method | Through every point | Fair on noisy data | Poles to edit | Rating |
|---|---|---|---|---|
| Cubic interpolation (exists: cubic spline form) | ✔️ | 🔴 wiggles | one per point | 🥉 for clean data only |
| Hermite / Catmull–Rom (exists) | ✔️ | ⚠️ local, still copies noise | one per point | 🥉 |
| Least-squares B-spline with a tolerance and a fairing term | within tolerance | ✔️ | few | 🥇 |

Planned algorithm (Piegl & Tiller, *The NURBS Book*, ch. 9):
1. **Parametrise** by centripetal chord length (Lee 1989), so tight turns do not overshoot or loop.
2. **Split at corners**: a turning angle above a threshold (default 30°) marks a crease. Each side is fitted separately and the joint stays G0.
3. **Fit** cubic, clamped, starting with a few spans: least squares on the points plus λ·∫|C″|² for fairness, with the ends interpolated.
4. **Refine**: insert a knot in the span with the worst deviation until max |P − C(τ)| ≤ tolerance (default 0.2 mm), or refuse with the worst point named when a span limit is hit.
5. **Report**: worst deviation, pole count, curvature-sign changes (a fairness score).

---

## 5. Kernel and workspace gaps

| # | Gap | Today | Needed | Effort |
|---|---|---|---|---|
| G1 | 3D curve feature | 🟢 built: `FeatureStepKind::SpaceCurve` (points + tolerance + pole edits in the document) | `SpaceCurve` feature step: a NURBS curve in world space, poles stored in the document | ⭐⭐ |
| G2 | Point import + fitting | 🟢 built: `ReadVehicleCurves` / `ImportDraftingCurves`, `FitSpaceCurve` | `ImportVehicleCurves`, `FitSpaceCurve(points, tolerance)` | ⭐⭐ |
| G3 | Loft (skinning) | 🟢 built: `LoftSectionSheet`, `FeatureStepKind::Loft` (guide rails open) | make sections compatible (common degree, merged knots), interpolate across them (P&T §10.3); optional guide rails | ⭐⭐⭐ |
| G4 | Curve-network surface | 🟢 built: `ConstructNetworkSurface` (Gordon; 2 + 2 → Coons), `FeatureStepKind::Network` | Gordon surface (P&T §10.5) for a grid of two curve families; Coons patch for 4-sided holes | ⭐⭐⭐ |
| G5 | Continuity across patches | 🟢 loft G1 / G2 to a neighbour panel (`LoftContinuity`); 🚧 networks | G1 by sharing tangent ribbons (aligned pole rows); G2 adds the second row; a gate measuring the normal angle and curvature jump along each shared edge | ⭐⭐⭐⭐ |
| G6 | N-sided corners | — | 5- or 6-sided holes (headlight corners, A-pillar roots): split into 4-sided patches, or Gregory-style patches | ⭐⭐⭐⭐ |
| G7 | Sew into a sheet / solid | 🟢 built: `AssemblePanels` (boundaries must match point for point), `MirrorPanel`, plane caps; `Mirror` / `Close` steps; 🚧 trim, reparametrised sewing, non-planar caps | stitch faces whose boundary curves coincide (tolerant edges) → sheet body → mirror → close with planar caps → solid | ⭐⭐⭐ |
| G8 | Blends / offsets on NURBS faces | refused (no exact rational offset) | needed for panel gaps, shut lines, thickness — see decision D4 | ⭐⭐⭐⭐ |
| G9 | Pole editing in the viewport | 🟢 built: mode 1 pole drag → `MoveDraftingPole` (V3) | Plasticity mode 1 (control points): pick and drag poles; the surfaces regenerate | ⭐⭐ |
| G10 | Evaluation | 🟢 curvature combs, limit-point deviation (W11), zebra stripes and the coloured deviation map (V8, W13) | curvature combs on curves; zebra / isophote Slang shader; deviation map (distance from each Blender limit point to the surface, coloured) | ⭐⭐ |

---

## 6. Milestones (each gated, in this order)

| # | Milestone | Gate (validation) | Status |
|---|---|---|---|
| V1 | Blender add-on + `VehicleCurves.toml` import | round trip: counts, names and closed flags equal; points bit-equal | 🟢 import gate W11 (round trip bit-equal, 5 refusals) · ⚠️ add-on untested in Blender |
| V2 | `FitSpaceCurve` | synthetic curves with known noise: max deviation ≤ tolerance; exact recovery of a B-spline sampled without noise; corners split where seeded; refusal names the worst point | 🟢 S1–S5 (worst 0.158 mm ≤ 0.2 mm; exact recovery 1.0e-14) |
| V3 | `SpaceCurve` steps in the workspace: display, combs, mode 1 pole drag | regeneration after a pole edit; persistent names survive; walkthrough with rendered evidence | 🟢 L6 + W11 + walkthrough 53 / 53 (C 50–56) |
| V4 | Loft through 3–8 section curves (bonnet first) | surface interpolates every section to 10⁻⁹; analytic check: lofting circles reproduces a cone or cylinder exactly; **deviation from the Blender limit points** reported (target ≤ 0.5 mm) | 🟢 L1–L5 (sections 7e-16, cylinder exact) · W11 bonnet 0.076 mm ≤ 0.5 mm |
| V5 | Gordon / Coons network surfaces | interpolates both curve families to 10⁻⁹; Coons of 4 lines reproduces a bilinear patch exactly | 🟢 N1–N6 (bilinear 3.3e-16 · Gordon 1.3e-15 · bonnet net 0.065 mm vs H) · W12 · captures 66–67 |
| V6 | G1 / G2 across shared curves | normal angle ≤ 10⁻⁸ rad (G1) and relative curvature jump ≤ 10⁻⁶ (G2) sampled along every shared edge | 🟢 C1–C6 (1.9e-15 rad · 3e-12; the G0 control shows 8.5e-4 rad) · T1 · captures 60–61 |
| V7 | Sew → sheet → mirror → solid | audit sound; volume of a closed lofted test body vs closed form; mirrored halves meet G1 at x = 0 | 🟢 A1–A6 (cylinder / frustum 1.6e-15 · seam ≤ 1.3e-15 rad · pod 4.8e-4) · T2–T4 · W12 · captures 62–65 · 🚧 trim |
| V8 | Zebra shader + deviation map | rendered evidence on the bonnet: stripes continuous across a G2 joint, kinked across a G0 joint (both shown) | 🟢 W13 (deviation 0.076 mm · G0 jump 1.07 stripes · G2 1.8e-13) · captures 70–73 |

---

## 7. Plasticity mapping (keymap stays Plasticity)

| Plasticity key / command | Use in this workflow |
|---|---|
| `l` loft | ④ loft through the selected curves; crossing curves → network (V5); G0 / G1 / G2 buttons (V6) |
| `alt-x` mirror (gizmo x) | V7 mirror every panel through x = 0 |
| `j` join-curves | join fitted pieces split at corners |
| `o` offset-curve | offset a character line (shut lines) |
| `t` trim | trim surfaces by curves |
| `p` pipe | exists as `PathSweep` |
| `1` control-point mode | G9 pole editing |
| Shift+S curve (Phase B) | hand-drawn planar guide curves next to the imported ones |

---

## 8. Risks and honest limits

- ⚠️ **Honest V5–V7 limits:**
  - Only lofts get G1 / G2 to a neighbour and a guaranteed G1 mirror seam. A network refits its curves, which drops the end tangent: on the bonnet net the mirror seam measures 2.75e-4 rad, reported in T3 but not gated.
  - Sewing needs boundaries that coincide point for point at equal τ. Panels built from the same SpaceCurve meet that; a reparametrised boundary is refused.
  - Caps are plane faces only. Trim is not built.
  - A planar fit reports its deviation against the points after projection onto the plane. The true distance is at most `WorstDeviation + PlanarOffset`.
  - A lone network panel can face down (it renders in the back-face tint). Orientation is fixed only when Close closes the body.
- ⚠️ **Honest V8 limits:**
  - Zebra stripes come from the interpolated vertex normals of the dense tessellation, not a per-pixel surface evaluation. Each vertex carries the exact surface normal, so the error is the interpolation across a triangle side ≤ 2 % of the diagonal.
  - Stripes are drawn against the view's up axis (they move with the camera, like Plasticity / Alias), not a fixed environment.
  - The deviation map measures the imported points against the panels, not against the Blender mesh faces. Mirrored images are included as panels, so a point is measured against the nearest panel.
  - The deviation map is recomputed on every toggle and regeneration (286 points × panels × 81 seeds + Newton, a few ms on the bonnet). A full car will need caching.
- ⚠️ **G2 networks are research-grade.** Commercial modellers solve them with heavy machinery (Alias / ICEM class). Expect G1 everywhere first; G2 only on the main panels.
- ⚠️ **N-sided corners** are where quad models hide their problems. Some corners will need a designer's choice of patch layout, not an algorithm.
- ⚠️ **Exactness policy vs offsets.** The kernel refuses anything it cannot build exactly. Offsets of NURBS faces are not rational, so panel thickness and fillets between freeform panels need a tolerance-bounded approximation (decision D4).
- ⚠️ **Data quality.** Loops that zig-zag across the flow lines produce poor curves. The add-on can warn, but picking the right loops stays a design task.
- 📝 **Scale.** A full body has hundreds of curves and 50–150 patches. Regeneration has to stay interactive, so per-step caching in the feature tree will probably be needed.

---

## 9. Decisions for later (not needed now)

| # | Decision | Options (rating) |
|---|---|---|
| D1 | Exchange format | TOML 🥇 · JSON 🥈 · OBJ 🥉 |
| D2 | Default fitting | least squares + fairing 🥇 · Hermite on clean loops 🥈 · cubic interpolation 🥉 |
| D3 | Surface strategy | loft per panel, then network where needed 🥇 · network everywhere 🥈 · patch by patch (Coons) 🥉 |
| D4 | Offsets / blends on NURBS | tolerance-bounded NURBS approximation with a reported error 🥇 · keep refusing (no shut lines or thickness) 🥈 · polygon offsets 🥉 (breaks the exact B-rep) |
| D5 | Where curves live | world-space `SpaceCurve` steps 🥇 · planar sketches on many section planes 🥈 (no 3D character lines) |
