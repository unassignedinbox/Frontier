# 📝 PLAN — Vehicle surfacing: Blender edge loops → fitted curves → CAD surfaces (document only, not started)

**TL;DR** — The vehicle already exists as a quad model in Blender. Its edge loops already carry the design intent: character lines, wheel arches, window outlines and sections. The plan reuses those loops instead of remodelling. Each chosen loop is exported as an ordered point chain, fitted with a fair B-spline in the kernel, and the curves are lofted or network-surfaced into exact NURBS faces that sew into a sheet body. The car is then mirrored and closed into a solid, ready for the fillets and features the kernel already has.
- 🟢 **Already there:** NURBS curves / surfaces (construct, evaluate, project, iso-curves, Bezier decomposition), planar spline sketches (Hermite · Bezier · NURBS · cubic, Phase B), traced surface ∩ surface sections, Booleans with NURBS faces, `BodyCategory::Sheet`, the persistent-name feature tree, Plasticity keymap.
- 🚧 **Missing (this plan):** point import, 3D (non-planar) curves, curve fitting with a tolerance, loft, curve-network surfaces, continuity across patches, sewing sheets, mirror, evaluation tools (curvature combs, zebra, deviation from the Blender mesh).
- ⚠️ **Hard parts:** G2 across patch boundaries, N-sided corners (5 or more curves meeting), and blends / offsets on NURBS supports, which are refused today because an exact rational offset does not exist.
- 🏁 **First useful result (V1–V4):** import → fit → loft one panel (the bonnet / hood) and measure its deviation from the Blender surface.

---

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
| G1 | 3D curve feature | sketches are planar (2D on a frame) | `SpaceCurve` feature step: a NURBS curve in world space, poles stored in the document | ⭐⭐ |
| G2 | Point import + fitting | — | `ImportVehicleCurves`, `FitSpaceCurve(points, tolerance)` | ⭐⭐ |
| G3 | Loft (skinning) | only `LoftCircles` | make sections compatible (common degree, merged knots), interpolate across them (P&T §10.3); optional guide rails | ⭐⭐⭐ |
| G4 | Curve-network surface | — | Gordon surface (P&T §10.5) for a grid of two curve families; Coons patch for 4-sided holes | ⭐⭐⭐ |
| G5 | Continuity across patches | — | G1 by sharing tangent ribbons (aligned pole rows); G2 adds the second row; a gate measuring the normal angle and curvature jump along each shared edge | ⭐⭐⭐⭐ |
| G6 | N-sided corners | — | 5- or 6-sided holes (headlight corners, A-pillar roots): split into 4-sided patches, or Gregory-style patches | ⭐⭐⭐⭐ |
| G7 | Sew into a sheet / solid | Booleans only; `Sheet` category exists | stitch faces whose boundary curves coincide (tolerant edges) → sheet body → mirror → close with planar caps → solid | ⭐⭐⭐ |
| G8 | Blends / offsets on NURBS faces | refused (no exact rational offset) | needed for panel gaps, shut lines, thickness — see decision D4 | ⭐⭐⭐⭐ |
| G9 | Pole editing in the viewport | mode 1 announced as unavailable | Plasticity mode 1 (control points): pick and drag poles; the surfaces regenerate | ⭐⭐ |
| G10 | Evaluation | — | curvature combs on curves; zebra / isophote Slang shader; deviation map (distance from each Blender limit point to the surface, coloured) | ⭐⭐ |

---

## 6. Milestones (each gated, in this order)

| # | Milestone | Gate (validation) |
|---|---|---|
| V1 | Blender add-on + `VehicleCurves.toml` import | round trip: counts, names and closed flags equal; points bit-equal |
| V2 | `FitSpaceCurve` | synthetic curves with known noise: max deviation ≤ tolerance; exact recovery of a B-spline sampled without noise; corners split where seeded; refusal names the worst point |
| V3 | `SpaceCurve` steps in the workspace: display, combs, mode 1 pole drag | regeneration after a pole edit; persistent names survive; walkthrough with rendered evidence |
| V4 | Loft through 3–8 section curves (bonnet first) | surface interpolates every section to 10⁻⁹; analytic check: lofting circles reproduces a cone or cylinder exactly; **deviation from the Blender limit points** reported (target ≤ 0.5 mm) |
| V5 | Gordon / Coons network surfaces | interpolates both curve families to 10⁻⁹; Coons of 4 lines reproduces a bilinear patch exactly |
| V6 | G1 / G2 across shared curves | normal angle ≤ 10⁻⁸ rad (G1) and relative curvature jump ≤ 10⁻⁶ (G2) sampled along every shared edge |
| V7 | Sew → sheet → mirror → solid | audit sound; volume of a closed lofted test body vs closed form; mirrored halves meet G1 at x = 0 |
| V8 | Zebra shader + deviation map | rendered evidence on the bonnet: stripes continuous across a G2 joint, kinked across a G0 joint (both shown) |

---

## 7. Plasticity mapping (keymap stays Plasticity)

| Plasticity key / command | Use in this workflow |
|---|---|
| `l` loft | ④ loft through the selected curves |
| `j` join-curves | join fitted pieces split at corners |
| `o` offset-curve | offset a character line (shut lines) |
| `t` trim | trim surfaces by curves |
| `p` pipe | exists as `PathSweep` |
| `1` control-point mode | G9 pole editing |
| Shift+S curve (Phase B) | hand-drawn planar guide curves next to the imported ones |

---

## 8. Risks and honest limits

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
