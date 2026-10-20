# BevelLab: live SolidArc editor proof

Scene: `Experimental/SolidArc/public/examples/BevelLab.arc` (`?example=bevellab`), replayed through the live C++
GeometryHost (`npm run dev:native`, port 8081) over the same `/native` JSON protocol the browser uses.
Route is pinned per run with `SOLIDARC_BLEND=general|legacy` (default `auto` = general first, legacy fallback).

## Route A/B on the identical scene

| Bevel | Topology exercised | General (RollingBallBlend) | Legacy (BlendSolver) |
|---|---|---|---|
| RoundedBlock · fillet 0.2 · 12 edges | 8 sphere corners | ✔️ 12 strips · 376 stations · residual 8.6e-13 | ✔️ |
| ChamferBlock · chamfer 0.15 · 12 edges | 8 triple mitres (24 mitre curves) | ✔️ 512 stations · residual 6.8e-13 | 🔴 "non-planar edge selections must form one closed loop" |
| BossPlate · fillet 0.15 · 6 edges | concave root rim + boss rim + 4 plate edges with mitres | ✔️ 516 stations · deviation 8.7e-8 | 🔴 "multi-edge fillet chains share a vertex" |
| Puck · fillet 0.25 · 2 rims | closed seam edges on a cylinder | ✔️ 274 stations · deviation 8.5e-8 | 🔴 "edge is degenerate (zero length)" |
| HoleSlab · fillet 0.12 · 5 edges | hole rim + top edges with mitres | ✔️ 399 stations · deviation 7.7e-8 | 🔴 "multi-edge fillet chains share a vertex" |
| Frustum · chamfer 0.12 · 2 rims | cone rims | ✔️ 210 stations · deviation 6.3e-7 | 🔴 "selected edge requires two adjacent planar faces" |
| **Total** | | **6 / 6** | **1 / 6** |

Every general result passed `Validate().Solid()` with genus and hull count unchanged (atomic refusal otherwise).
Harness: [RollingBallBlendHarness.md](RollingBallBlendHarness.md) (15 / 15: 13 closed-form volumes + 2 required refusals).

## Kernel corrections found while proving (all in `Kernel/RollingBallBlend.cpp`)

| # | Defect | Symptom | Fix | Evidence |
|---|---|---|---|---|
| 1 | Oversized carriers (strips past the edge ends, sphere corners as full revolutions) | `Bounds` of a box 0..4 read low = 2.1 (1 edge) / −0.21 (12 edges) | Trim every new carrier to the (u,v) box of its boundary traces at assembly | Bounds exactly 0..4 for 1/4/12 edges, both modes; harness volumes identical |
| 2 | Blend of an edge running into an earlier tangent blend was refused (singular end-on-face) | `MixedCapCorner` second fillet refused | Tangent-chain propagation at selection: a sharp edge tangent to the selected one across a smooth (G1) edge joins the chain → ordinary continuation | Fixture replays 2 / 2 on the general route, deviation 3.9e-7 |
| 3 | Continuation sections compared at 1e-7 although fitted inputs carry ~1e-7 | Propagated chain refused with gaps < 1e-6 | Section gate = `JoinTolerance` (1e-6, the topological merge tolerance); refusal reports the gaps | Same fixture |
| 4 | Size beyond a support's curvature radius was accepted for chamfers | Chamfer 0.3 on a 0.193 arc rim validated "Solid" but the sweep folded through itself (topology checks cannot see it) | Refuse when a contact rail runs against its edge between consecutive stations | 2 new must-refuse specimens; rim refuses ≥ 0.2 in both modes |
| 5 | `BrepBody::Bounds()` used the full carrier hull of faces kept by a boolean | HoleSlab bounds z −0.5..1.5 vs true 0..0.6 (found by `GeometricAudit` A7) | `TrimmedCarrierBounds`: clip each carrier to its boundary-trace (u,v) box, except closed directions and collapsed sides | Audit A7 clean on all 26 body states |
| 6 | Boolean plane ∩ cylinder rims were fitted cubic splines (105–134 poles, radial 8.2e-7, tangent 3.2e-5 rad) where an exact circle exists | Blend stations rotate with the bad tangent → rail vertex 1.05e-6 off the bore seam → HoleSlab loop split (audit A2) | `IntersectionSolver::ExactPiece`: line / circle candidate from the march, accepted only if it lies on both surfaces within 1e-8 at 64 samples; else the interpolant stays | Rims now 9-pole rational circles (radial 8.9e-16); boss / hole volume error 2.7e-8 → 1.3e-9 and 5.6e-8 → 4.0e-9; audit 0 defects |

Design decision (user, option A): tangent propagation is the default, as in mainstream CAD. Old journals whose line
seeds ran into a tangent blend now replay with the propagated blend; `MixedCapCorner` expectations were updated
(V14/E22/F10 after the second fillet, rim = arc 10 + line 11, V18/E29/F13 after the third).

Known limit: downstream targets are bound geometrically (`ConsoleBevel.cpp` `Rebind`). Editing a feature whose output
*is* a later feature's target (feature 2's end arc = feature 3's rim) refuses atomically. Persistent topological
naming is an architecture item.

## Test suites (default route, after the corrections)

| Suite | Result |
|---|---|
| `npm run test:bevel` | 71 / 71 ✔️ (was 65 / 70) |
| `npm run test` | 50 / 50 ✔️ |
| `npm run test:transport` | 6 / 6 ✔️ |
| `npm run test:bevel-input` | 5 / 5 ✔️ |

## Editor latency (same scene, 960×600 frame)

| Request | Before | After fixes | Fix |
|---|---|---|---|
| Chamfer bevel command | 10,817 ms | 625 ms | bevel replaces the selection (no 2-body boolean preflight) |
| BossPlate / HoleSlab bevel | 6,308 / 6,392 ms | 1,472 / 1,760 ms | arc-length table instead of split + bisection |
| Idle frame (no change) | 1,007–1,088 ms · 10.2 MB | 8–9 ms · 0.086 MB | display tessellation cache + content-addressed streams |
| Re-select a blended body | 1,255 ms | 10–12 ms | cached `BodyReport` for describe / eligibility |
| Whole scene load (one batch) | ~32 s | 3.0 s | all of the above |

Full-transport baseline with caches only (`BevelLabLatency.py --full`): idle frame 667–699 ms · 10.2 MB.

## Reproduce

```sh
cd Experimental/SolidArc && npm run dev:native            # port 8081
python3 Exhibits/RollingBallBlend/Code/BevelLabRoutes.py  Experimental/SolidArc/public/examples/BevelLab.arc
python3 Exhibits/RollingBallBlend/Code/BevelLabLatency.py Experimental/SolidArc/public/examples/BevelLab.arc [--full]
SOLIDARC_BLEND=legacy npm run dev:native                  # A/B: legacy route
```
