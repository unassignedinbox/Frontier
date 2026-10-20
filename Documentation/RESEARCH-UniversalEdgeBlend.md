# RESEARCH — Universal Edge Blend (Fillet · Chamfer · Bevel on Any Solid)

🧩 Why SolidArc's blend kernel cannot reach general coverage, and the finite general algorithm that can.
Date: 2026-10-19. Sources inspected: `SultanAladin/Frontier-@3382938` (SolidArc),
`SultanAladin/Frontier@ea2c748` (new project), OCCT `V7_8_1` source. Evidence: `Exhibits/RollingBallBlend/`.

## TL;DR

The old kernel matches each **pair of surface types** and builds an exact tool for that pair, so the
number of cases grows as types² × topology × mode. No list of special cases will ever be complete.
Every production CAD kernel uses the same three-layer approach instead: **one numeric cross-section solver**
that works on any two surfaces, a **fixed list of about 16 topological events**, and
**imprint-and-replace** to assemble the result. The cross-section layer is already proven here:
one solver handles 18/18 specimens (planes, cylinders, cone, sphere, freeform) with zero type branches.

## 1. Diagnosis of the current approach

| Evidence (inspected)                                                  | Value                                   |
|-----------------------------------------------------------------------|-----------------------------------------|
| `Kernel/BlendSolver.cpp` size                                         | 13,762 lines                            |
| `if (` statements in that one file                                    | 2,421                                   |
| Pair-specific routes (e.g. `FilletPlaneConeApexPartialBossRoot`)      | ~50 named recognisers/builders          |
| `FilletEdge` dispatch before the generic route                        | 8 chained pair-recognisers              |
| Broad sweep acceptance (`GEOMETRY_GAPS.md`)                           | 135 / 388 bevel requests (35%)          |
| OCCT bridge (`OptionalOcctBlend.h`)                                   | Sphere-gated, **Linux-only**, round-trips B-rep |
| New project (`Frontier@ea2c748`) solid blend code                     | None. Only the 2D sketch-corner fillet  |

Root causes:

| # | Cause                                                                                   | Effect                                     |
|---|-----------------------------------------------------------------------------------------|--------------------------------------------|
| 1 | Construction branches on `SurfaceClassification` (Plane/Cone/Cylinder…)                 | Combinatorial growth, never complete       |
| 2 | Blend = Boolean against a tool solid                                                     | Fillet tool is **tangent** to faces along the contact lines; the SSI marcher requires transversal intersections (`IntersectionSolver.h`) |
| 3 | Partial / apex / cap / root variants are each written as separate code              | Topology cases are multiplied by geometry cases |
| 4 | OCCT route converts the B-rep each call                                                | Loses identity, trims, analytic hints, and only runs on Linux |

💡 Cause 2 is the hidden one. Subtracting a rolling-ball tool asks the Boolean to resolve a curve where
the two surfaces touch tangentially along the whole contact line. That is the worst case for any marcher. The contact curves are
already **known** from the blend solve, so there is nothing left to intersect.

## 2. The general algorithm (what ChFi3d / Parasolid-class kernels do)

```text
 selected edges
      │
 ①  CLASSIFY + CHAIN        convex / concave / G1-smooth / seam / boundary   (normals + coedge sense)
      │
 ②  WALK  (per chain)       Newton per station: 4 equations, 4 unknowns (u₁,v₁,u₂,v₂)
      │                     events: support-hop · ball-too-big · fold · end-of-chain
 ③  FIT STRIP               stations → NURBS strip + exact pcurves (u,v) on both faces
      │
 ④  RESOLVE ENDS            vertex valence n of blended edges: 0 → trim on end face · 1 → continue · ≥2 → corner
      │
 ⑤  CORNER PATCH            n = 2 strip–strip trim · n = 3 vertex ball / 3-sided patch · n ≥ 4 N-sided fill
      │
 ⑥  IMPRINT + REPLACE       split faces along pcurves (existing UV arrangement code), drop the edge-side cells,
      │                     insert strips + corner patches, sew
 ⑦  VALIDATE (atomic)       closed · manifold · oriented · volume · tangency on rails; else refuse, keep source
```

### Station equations — identical for every surface type

| Profile          | Equations (e = spine point, T = spine tangent, cᵢ = pᵢ − σ·r·nᵢ, σ = ±1 convexity) |
|------------------|-------------------------------------------------------------------------------------|
| Fillet           | T·(c − e) = 0 · c₁ − c₂ = 0 (3 rows)                                                 |
| Chamfer (d, d)   | T·(p₁ − e) = 0 · T·(p₂ − e) = 0 · \|p₁ − e\|² = d² · \|p₂ − e\|² = d²                |
| Chamfer (d₁, d₂) | Same, with d₁ and d₂                                                                 |
| Chamfer (d, α)   | Same as d₁/d₂ with d₂ derived from α in the section plane                             |
| Variable radius  | Fillet with r = r(t)                                                                 |

A surface only has to supply `Evaluate(u, v) → S, Su, Sv` (plus second derivatives for an analytic Jacobian).
OCCT's `BlendFunc_ConstRad::ComputeValues` uses the same 4×4 structure: E(1) is the section-plane equation
and E(2..4) is centre agreement. `BRepBlend_Walking` marches it.

### Finite event taxonomy (replaces the infinite list of cases)

| Layer      | Event                                   | Generic handling                                         |
|------------|-----------------------------------------|----------------------------------------------------------|
| Classify   | Convex / concave                        | σ = sign from into-face direction · other normal         |
|            | G1-smooth vertex                        | Chain continues                                          |
|            | Periodic seam / open boundary / non-manifold | Not blendable: specific refusal                    |
| Walk       | Converged                               | Next station (adaptive step)                             |
|            | Contact leaves its face domain          | **Support hop**: re-seed on adjacent face, continue      |
|            | Jacobian singular / r > curvature radius| Refuse: "ball does not fit" + station location           |
|            | Cross-sections fold (spine curvature)   | Refuse, or clamp at the fold                             |
| Ends       | 0 other blended edges at vertex         | Extend strip, intersect with end face (transversal SSI)  |
|            | Open boundary                           | Flush end                                                |
| Corner     | n = 2                                   | Strip–strip intersection trim                            |
|            | n = 3                                   | Vertex blend (sphere if equal r on planes), else 3-sided patch |
|            | n ≥ 4 or mixed convex/concave           | N-sided fill (Gregory / energy patch)                    |
| Assembly   | Imprint, replace, sew                   | Reuse `IntersectionSolver` UV-arrangement + `BrepBody::Sew` |
|            | Validation failure                      | Atomic refusal, source preserved                         |

About 16 events, all **independent of surface type**. Adding torus, NURBS or offset surfaces adds zero events.
Analytic recognition (strip ≈ cylinder/torus/sphere) is an optional **post-step** that stores exact classification.
It never gates construction.

## 3. Proof: the walk layer is type-agnostic

![Fillet](../Exhibits/RollingBallBlend/Images/RollingBallFilletSheet.png)

| Specimen                    | Edge    | Fillet residual | Fillet vs closed form | Chamfer vs closed form |
|-----------------------------|---------|-----------------|-----------------------|------------------------|
| Plane–plane 90° / 60° / 135°| convex  | 7.4e-14         | 7.4e-14               | 0                      |
| Plane–cylinder cap rim      | convex  | 4.9e-16         | 1.1e-12               | 1.1e-12                |
| Plane–cylinder boss root    | concave | 3.7e-16         | 9.1e-13               | 9.1e-13                |
| Plane–sphere truncation rim | convex  | 5.0e-16         | 9.7e-13               | 3.0e-14                |
| Plane–cone frustum foot     | convex  | 5.0e-16         | 1.2e-12               | 7.6e-13                |
| Cylinder–cylinder T         | concave | 2.2e-15         | n/a (indep. 1.2e-15)  | n/a (indep. 1.1e-16)   |
| Freeform bicubic–plane      | convex  | 1.0e-15         | n/a (indep. 1.5e-15)  | n/a (indep. 5.0e-16)   |

Each specimen has 161 stations; all 2,898 converge. The old kernel needed separate hand-written routes for
cap rim, boss root, cone foot and sphere rim. Here all of them use the same ~120 lines.
⚠️ Steps ③–⑦ are **not** implemented by this exhibit.

## 4. Options

Effort/Risk: more = worse. Coverage: 🔴 = best.

| Option                                                      | Coverage | Effort | Risk | Windows | Own kernel | Quality    | Rank  |
|-------------------------------------------------------------|----------|--------|------|---------|------------|------------|-------|
| D · Generic pipeline in our kernel + OCCT as test oracle only| 🔴       | 🔴     | 🚩   | 🟢      | 🟢         | ⭐⭐⭐⭐⭐ | 🥇 🏆 |
| B · OCCT owns all solids (no round-trip)                     | 🔴       | 🚩     | 🚩   | 🟢 vcpkg| 🔴 LGPL    | ⭐⭐⭐⭐   | 🥈    |
| C · Current OCCT worker bridge, ungated                      | 🚩       | ✔️     | 🔴   | 🔴      | 🔴         | ⭐⭐       | 🥉    |
| A · Keep extending the analytic catalogue                    | ✔️       | 🔴     | 🔴   | 🟢      | 🟢         | ⭐         | 🏴    |

⭐ **Recommendation: D.** It keeps one owning B-rep, persistent edge identity and exact pcurves, and works on Windows.
Existing analytic routes become recognised fast paths and regression references rather than gates.
OCCT `BRepFilletAPI_MakeFillet` runs **only in tests** as a comparison oracle on shared fixtures.

## 5. Phased plan for D

| Phase | Deliverable                                                              | Exit evidence                                      |
|-------|--------------------------------------------------------------------------|----------------------------------------------------|
| 1     | Production walker: analytic Jacobian, adaptive step, walk events         | Exhibit grows to radius sweeps + refusals          |
| 2     | Strip NURBS fit + pcurves within tolerance                               | Rail-on-face deviation, G1 angle on rails          |
| 3     | Imprint + replace, single edge, ends trimmed on end faces                | Box edge, cylinder rims, sphere rim: closed solids |
| 4     | Tangent chains + support hop                                             | Slot, rounded-rectangle, Boolean-derived edges     |
| 5     | Corners n = 2, 3 (vertex ball), then N-sided fill                        | Box corner, mixed convex/concave corners           |
| 6     | Chamfer variants (d₁d₂, dα), variable radius                             | Same matrix, chamfer mode                          |
| 7     | Rerun old 388-candidate sweep + `Fixtures/*.json`; OCCT oracle comparison| Acceptance vs 135/388 baseline, zero wrong accepts |

## 6. Known limits of this report

- 🔍 Claims about proprietary kernels are not made. OCCT/FreeCAD/CadQuery structure is from inspected source.
- The 135/388 baseline is the old repository's own measurement. It was not re-run here.
- No Windows run was performed; the exhibit is dependency-free C++20.
