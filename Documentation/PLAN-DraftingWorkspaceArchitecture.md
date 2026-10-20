# 📝 PLAN — DraftingWorkspace architecture (parametric CAD on the SolidArc kernel)

**TL;DR** — SolidArc is the only exact kernel in either repository. With the phase 3 fixes it builds every primitive, sketch-to-solid feature, box/cylinder boolean, rim blend and transform in the corpus exactly (≤ 1e-7 relative), and audits sound on every shipped document. What prevents it from being a parametric CAD kernel is not geometry but three structural gaps:

1. 🔴 **No feature history beyond the first operation.** Booleans, blends and face ops have no recipe. A bevel freezes its body.
2. 🔴 **No persistent naming.** A boolean renumbers 8 of 12 surviving edges.
3. 🔴 **Face operations are a shape catalogue.** `FaceEditSolver` has 4,360 lines and 1,031 `if (` branches, and refuses everything except canonical boxes.

The plan has four layers:
- a double-precision sketch solver;
- a feature tree keyed by topological names;
- SolidArc behind a GeometricAudit gate;
- tessellation for the viewport.

Migration is ordered so that each step is gated by the harnesses in `Exhibits/DraftingArchitecture`.

---

## 1. Evidence

| Source | Scope | Result |
|---|---|---|
| `Exhibits/DraftingArchitecture/Results/FeatureSuitability.md` | 176 corpus rows, sections A–H | 19 refused · 1 defective (legacy `fillet`) · exact everywhere a closed form exists, apart from the pole-quadrature artefact and G2 |
| `Exhibits/DraftingArchitecture/Results/BrepDefects.md` | 21 specimen measures | 21 / 21 ✔️ |
| `Exhibits/DraftingArchitecture/Results/DocumentAudit.md` | 129 body states, all shipped documents | 0 defective (18 before phase 3) |
| `Exhibits/RollingBallBlend/` | general rolling-ball blend | harness 15 / 15, BevelLab routes 6 / 6 (legacy 1 / 6) |
| `npm run test:bevel` · `npm test` (`Experimental/SolidArc`) | browser UI → `/native` → geometry service | 71 / 71 · 50 / 50 |
| Survey of `SultanAladin/Frontier` @ `ea2c748` | `Internal/Authoring/*` | see § 2 |

Both UI versions drive the C++ kernel through the same command stream: browser → `/native` → `ServeSequence.py` (port 8081) → `GeometryHost`. The corpus, specimens and audit replay go through that same `ConsoleHost` command stream.

## 2. Keep / Adapt / Replace / Delete

| Component | Where | Verdict | Evidence |
|---|---|---|---|
| Sketch UI (tools, snapping, viewport overlay) | `ref/new` ParametricSketcher | 🟢 **Keep** | interaction layer is independent of the solver |
| Sketch model `ParametricSketchShapeStore` | `ref/new` ParametricAuthoring | **Replace** | float 2D, whole-store snapshot history, constraints by vector index |
| Sketch solver `ParametricSketchConstraintSolver` | `ref/new` ParametricAuthoring | **Replace** | float Gauss–Seidel, heuristic DOF count (no rank) |
| Boundary B-rep (`BoundaryTopology.h`) | `ref/new` ParametricAuthoring/Boundary | **Adapt** → display/export only | polygonal radial-edge, no exact geometry |
| `Draughting/` fork | `ref/new` Authoring/Modeling | **Delete** ⚠️ pending your approval | dead fork of ParametricAuthoring (≈ 7,100 lines), no live callers in the survey |
| SolidArc kernel (curves, surfaces, topology, Skin, Intersection) | `Engine/Modelling/SolidArc/Kernel` | 🟢 **Keep** as the kernel | exact corpus rows; audit sound; booleans have gaps G3–G5 |
| `RollingBallBlend` (`beveladd`) | Kernel | 🟢 **Keep** | 1,609 lines · 180 `if (`; general for any face pair with a station solve |
| Legacy `BlendSolver` (`fillet`, `chamfer`) | Kernel | **Retire** | 13,762 lines · 2,085 `if (`; 12-edge fillet audits defective (trace-on-edge 0.283), `beveladd` on the same edges is sound |
| `FaceEditSolver` (draft, shell, offsetface, extend, trim, delete, scale, rotate) | Kernel | **Replace** | 4,360 lines · 1,031 `if (`; 16 of 19 corpus refusals ("canonical box", "six-edge orthogonal L-profile", "oblique triangular-prism") |
| `TweakSolver` (`push`, `extrudefaces`, `tweak`) | Kernel | **Adapt** | exact on Block, Can, HoleSlab (4.5 · 1.696460 · 4.291062); `tweak` refuses curved edges |
| `FigureRecipe` regeneration | `Document/` | **Adapt** → feature tree | Extrude, Revolve, Loft, Sweep, Pipe, Patch, FairPatch and SurfaceOffset only; no Boolean, blend or face-op recipe |
| Bevel stack (`BevelFeature`, `Rebind`) | `Console/ConsoleBevel.cpp` | **Adapt** | already a replayable step with geometric bindings; becomes the first feature-tree step type |
| `GeometricAudit` | Kernel | 🟢 **Keep** as the commit gate | A1–A7 invariants; found D7–D12 |
| Docs: `EngineDocs/FolderStructure.md:285` | `ref/new` | **Fix** | claims "`ParametricSketch/` — exact B-rep NURBS kernel"; no such kernel exists |

## 3. Layering

```
 SketchModelViewport (Vulkan + Slang)            ← tessellation only, never topology
          ▲ PolygonComplex per face (cached by face name + revision)
 ┌────────┴─────────────────────────────────────────────────────────────────┐
 │ SolidArc kernel  — exact NURBS B-rep                                     │
 │   every feature result → GeometricAudit::Inspect → commit | refuse       │
 └────────▲─────────────────────────────────────────────────────────────────┘
          │ BrepBody + provenance (which input face/edge each output came from)
 ┌────────┴─────────────────────────────────────────────────────────────────┐
 │ FeatureTree — ordered steps, each = { operation, parameters,             │
 │   inputs as TopologicalName, result names }; regenerate = replay         │
 └────────▲─────────────────────────────────────────────────────────────────┘
          │ closed profiles (exact curves) + named sketch entities
 ┌────────┴─────────────────────────────────────────────────────────────────┐
 │ SketchSolver — double precision, stable entity IDs, Newton / Levenberg–  │
 │   Marquardt, DOF = unknowns − rank(J) (rank-revealing QR)                │
 └──────────────────────────────────────────────────────────────────────────┘
```

- **Tolerances** (unchanged): Kernel 1e-9 · Accept 1e-8 · Join 1e-6 · audit edge 1e-5. Do not loosen them to admit a result.
- **Refusal is a result.** A step that fails the audit or refuses keeps the previous body, and the tree marks the step. This is already the behaviour of `beveladd`.

## 4. Persistent naming

| Born by | Name | Example |
|---|---|---|
| Sketch → extrude/revolve | step · role · sketch entity | `Extrude#3.Side(Line#7)`, `Extrude#3.Cap(End)` |
| Boolean | step · `Section` · the two parent face names · ordinal along the curve | `Boolean#5.Section(Extrude#3.Side(Line#7), Cylinder#4.Side)#0` |
| Split survivor | parent name · ordinal by a stable key (centroid projected on the parent's u) | `Extrude#3.Cap(End)/1` |
| Blend | step · `Strip` / `Corner` · bound edge names | `Blend#7.Strip(Boolean#5.Section(…)#0)` |
| Face op | step · moved face name | `Push#8.Moved(Extrude#3.Cap(End))` |

- The kernel already carries most provenance: boolean `FacePieces` know `{Body, Face}`, and blend `Bindings` know their source edges. The new piece is emitting it into the result instead of discarding it.
- Resolution order: exact name match → same parent with one candidate → geometric `Rebind`, which already exists. If none of these yields exactly one candidate, the step refuses with the ambiguity. No silent best guess.
- Measured need: `boolean subtract` renumbered 8 of 12 surviving edges; `beveladd` renumbered 2 of 7; `push` and `tweak` renumbered 0.

## 5. Kernel gaps (after phase 3)

| # | Gap | Evidence | Fix direction |
|---|---|---|---|
| G1 | 🔴 Bevel/boolean/face-op bodies do not regenerate | `Results/Regeneration.txt`: plain extrusion follows its sketch (3 → 6, z + 0.5); bevelled one stays 2.982806. Now announced by a ⚠️ row | feature tree (§ 3) with Bevel as the first step type: regenerate base → replay `BevelFeature` via names / `Rebind` |
| G2 | Circle swept along an arc is fitted, not exact | BentPipe volume error 2.8e-6 (1.4e-5 relative), 1 freeform edge | `Pipe` on an arc path = `Revolve` of the circle about the arc axis |
| G3 | Seam on an intersection point is misdiagnosed | CrossUnequal: "coincident faces along the intersection (not transversal)"; same pair shifted by 0.1 z unions fine (4.009803) | seed SSI marching off-seam; classify by surface normals, not by the trace collision |
| G4 | Singular intersections (curves crossing) refuse | Steinmetz: "intersection runs into a surface singularity or seam corner" | branch-point detection + split at the singular point (legitimate hard case) |
| G5 | Coplanar booleans fall back to a cell arrangement | PlusUnion 22 faces, face-contact union 34 faces (exact, but fragmented) | coplanar face overlay in the traced route, then remove the arrangement |
| G6 | Face ops are box-only | 16 refusals | generic rule: offset the selected faces' carriers → re-intersect with their neighbours (the `TweakSolver` approach) → audit. `shell` = offset all faces + boolean (as D8 now does for boxes) |
| G7 | No persistent naming | § 4 | § 4 |

## 6. Migration order

| Step | Deliverable | Gate |
|---|---|---|
| 1 | Approve the decisions in § 8; delete `Draughting/`; fix `FolderStructure.md:285` | none (documentation) |
| 2 | Port SolidArc into `ref/new` as `Internal/Authoring/Modeling/Cad/` (folder name kept), and add the harnesses to its verification build | `BrepDefectVerification` 21 / 21 · `RollingBallBlendVerification` 15 / 15 · audit 0 defective |
| 3 | `SketchSolver` (double precision, IDs, Newton/LM, rank DOF) under the existing sketch UI | new sketch corpus: under-, well- and over-constrained cases report the correct DOF |
| 4 | Provenance out of boolean, blend and face ops → `TopologicalName` | naming table: Same index → **Same name** for every survivor |
| 5 | `FeatureTree` with Extrude, Revolve, Boolean and Bevel steps; regeneration by replay | `RegenerationProbe` with bevel: body follows the sketch |
| 6 | Generic face ops (G6) replace `FaceEditSolver` routes one verb at a time | corpus section E: 0 refusals on HoleSlab and Can, closed forms exact |
| 7 | Retire `BlendSolver` `fillet` and `chamfer` (aliases to `beveladd`) | BevelLab 6 / 6; corpus section D unchanged |
| 8 | G2–G5 kernel work | corpus refusals → 0 |
| 9 | `DraftingWorkspace` in `ref/new` on SketchModelViewport (Vulkan + Slang, GLFW, TOML options) | UI parity checklist against the SolidArc browser editor |

## 7. Fixed in phase 3

| Defect | Was | Fix | Specimen |
|---|---|---|---|
| D7 | trimmed tessellation counted a lattice row twice where a trim ran along an iso line (hemisphere area +4.2%) | reject lattice sub-segments lying on a ring; uv bucket grid keeps it faster than before (277 vs 299 ms) | area 8.3e-4 · volume 1.1e-4 |
| D5b | trimmed sphere/torus bounds = whole carrier | tessellation ⊕ chord where trace clipping is not provable | audit sound |
| D8 | `shell` on a box extruded a U profile (no y walls, 45° rim mitres: 0.555 vs 0.894) | outer box − inward offset box carried through the opening | 2e-8 relative, 11 faces |
| D9 | box ⊖ box emitted one patch per arrangement cell (Pocket 46 faces) | traced route first, arrangement only as fallback | 11 / 10 / 10 / 12 faces, exact |
| D10 | caps and natural curved faces bounded by control hulls (oblique pipe 3.3e-2) | hull refinement inside every knot span; closed directions ∩ tessellated box | 1.3e-3 |
| D11 | stored trace seeded the seam exit from its first point; the seam guard kept the far end (trace error 5.70 m) | nearest-sample seed + wrap-aware guard | audit sound |
| D12 | closed boolean edge stored its ring trace from an arbitrary start (and judged its sense there) | anchor the ring at the edge vertex before computing sense | audit sound |

The earlier phases fixed D1–D6 and the chamfer/fillet work (`afc3da2`, `f345606`).

## 8. Decisions for you

| Decision | Option | Effort | Risk | Payoff | Rating |
|---|---|---|---|---|---|
| Delete `Draughting/` fork | **Delete** | ⭐ | ⭐ | ⭐⭐⭐ | ⭐⭐⭐⭐⭐ |
| | Keep as reference | — | ⭐⭐ (drift) | — | ⭐⭐ |
| Kernel for DraftingWorkspace | **Port SolidArc into `ref/new`** | ⭐⭐⭐ | ⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ |
| | Extend `BoundaryTopology` to exact | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐ | ⭐ |
| | OCCT dependency | ⭐⭐ | ⭐⭐⭐ (licence, size) | ⭐⭐⭐⭐ | ⭐⭐⭐ |
| Persistent naming | **Provenance names + Rebind fallback (§ 4)** | ⭐⭐⭐ | ⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ |
| | Geometric matching only | ⭐⭐ | ⭐⭐⭐⭐ (ambiguity) | ⭐⭐ | ⭐⭐ |
| Face operations | **Generic offset + re-intersect (G6)** | ⭐⭐⭐⭐ | ⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ |
| | Keep growing the catalogue | ⭐ per shape | ⭐⭐⭐⭐⭐ | ⭐ | ⭐ |
| Legacy `fillet` / `chamfer` | **Retire, alias to `beveladd`** | ⭐ | ⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ |
| | Keep both | — | ⭐⭐⭐ (defective output) | — | ⭐ |

⭐ = low for effort and risk, high for payoff and rating.
