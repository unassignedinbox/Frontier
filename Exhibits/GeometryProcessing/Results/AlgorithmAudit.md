# GeometryProcessing — algorithm audit: Booleans, chamfers, fillets

🟢 6 bugs fixed · Booleans 2.3× faster · all-edge fillets 4.6–5.1× faster · all-edge chamfers on mixed corners now built (were refused) · every closed-form gate unchanged

## TL;DR

| | Area | Before | After |
|---|---|---|---|
| 🏆 | Boolean stress (78 steps, Release) | 2 334 ms · 4 failing | **1 017 ms · 0 failing** (64-point membership oracle per step) |
| 🏆 | Fillet, every edge at once (Pocket / TurnedCut / LBracket, 0.1) | 7 045 / 6 119 / 3 001 ms | **1 370 / 1 283 / 654 ms** |
| 🏆 | Chamfer, every edge at once (LBracket, Pocket, TurnedCut) | 🔴 refused "setback vertex blend of chamfers (🚧)" | **🟢 sound** (511 / 1 063 / 1 236 ms) |
| 🥇 | Size 0.3 on the pocketed block | 🔴 refused "contact rail runs backwards" | **🟢 sound** (fillet and chamfer) |
| 🥇 | BooleanValidation run | 4 min 23 s | **35 s** (same verdicts, same 0 / 480 oracle) |
| ✔️ | BooleanValidation · BlendValidation · document validation | PASS · PASS · 620 🟢 | PASS · PASS · 620 🟢 — every ΔV / residual identical to the last digit |

## Method

| Tool | What it does |
|---|---|
| `Code/AlgorithmStressValidation.cpp` (new) | 78 Boolean steps (drilled plate, pocket row, staircase, crossing cylinders, Booleans on blended bodies, 6 random 6-step sequences) and 32 blend rows (8 bodies × fillet / chamfer × 0.1 / 0.3): every non-seam edge alone, then all edges at once. Each Boolean is timed as General Fuse / assembly / maximal faces, audited (`AuditScope::Complete`) and checked by a **64-point membership oracle** (result winding = A ∘ B from the arguments' windings). `--only Prefix`, `--oracle N` (0 for timing runs). |
| Timing builds | Two identical Release trees: `38286c0` (before, a git worktree + the new harness) and this commit (after), run one after the other on an idle host. |
| Profiles | `-pg` build + gprof, and temporary phase clocks inside `AssembleBody`, `BlendEdges`, `DetectSelfClash`, `IntersectCurveSurface` (removed after use). |

## Bugs found and fixed

| | Symptom | Root cause | Fix | Evidence |
|---|---|---|---|---|
| 🐞 B1 | Random sequence 3 · 5 refused "pave block 48 used 1 times" | One fragment's own interior probe misclassified it, leaving the result open | **State propagation** (below): fragments joined by a clean pave block take one state | Step now sound, oracle 0 / 64 |
| 🐞 B2 | Random 6 · 5 and 6 · 6 refused "pave block 49 used 4 times" | Two steps earlier (6 · 4) a marched section ended 2·10⁻⁸ off the exact vertex where a block edge pierces a cylinder; `SplitSections` registered a fresh end vertex and a 2·10⁻⁸ piece, which later joins collapsed to a **zero-length closed edge** in the body | `GeneralFuse.cpp` `SplitSections`: a piece from an unregistered open end shorter than the existing degenerate-piece bound (10·precision) is dropped — the open end belongs to the exact vertex | 36 / 36 random steps sound, oracle 0 |
| 🐞 B3 | Crossing cylinders r 0.6 × 0.5, offset 0.1, difference returned **unsound** (T4: V − E + 2F − L odd) | Internal tangency: the tunnel's two mouths meet at one point — a pinched, non-manifold solid passed the edge-use check | `AssembleBoolean`: topology-scope audit gate (T1–T5, no geometry) refuses "non-manifold result (…; tangent contact?)" | Refused instead of an invalid solid |
| 🐞 B4 | Chamfer of all edges refused at every mixed-convexity / degree ≠ 3 corner | `ResolveSetback` was fillet-only (🚧) | Chamfer sheets are ball-based too (r = d·tan(θ/2)), so ring, cuts, face conics, spokes and fair quads are shared; only the section is the straight chord of the two feet across the chamfer plane | LBracket, Pocket, TurnedCut (0.1) and LBracket, Pocket (0.3) sound |
| 🐞 B5 | Size 0.3 on the pocketed block refused "contact rail runs backwards" | A 1.0 rim edge with setback corners at both ends: 2 × (crossing + 1 radius) > 1.0 | Margin past the crossings capped at 45 % of the spine span (floor ¼ radius); edges with room keep the full radius, so their geometry is unchanged | Pocket fillet / chamfer 0.3 sound |
| 🐞 B6 | Harness: "fillet ∪ boss" refused | The boss (y 0.5 ± 0.3) was accidentally **tangent** to the slot wall y = 0.8 | Boss r 0.28 crosses the plane / fillet G1 edge instead (sound); the tangent case stays as its own 🚧 row | See open items |

## Performance fixes

| | Hotspot (share of time) | Cause | Fix | Exactness |
|---|---|---|---|---|
| ⭐ P1 | NURBS foot polishing, 78 % of an all-edge fillet | `PolishNurbsFootLocally` rejected feet whose residual is pure rounding (points ON the surface: SP-curves on fair / loft patches) and went global (5 polished starts) | A residual within 64 ε of the coordinates means the point is on the surface: no stationarity / domain-edge test on noise | Rounding bound, not a tolerance; all counts, refusals and volumes unchanged |
| ⭐ P2 | Fragment classification, 55 % of a Boolean | One generalized-winding quadrature per fragment | **State propagation** (OCCT builder): union-find over pave blocks that are argument edges used only by that argument's fragments (not a section, common block or imprint); one winding per group. Only Inside / Outside propagate; a group with an ON member classifies each member | Classify 807 → 262 ms; fixes B1 |
| ⭐ P3 | Assembly 57–214 ms per Boolean once a body has a void | Cavity ownership in `AssembleBody` ran the strict winding quadrature (the shell-volume integral was only 4–21 ms) | New `ClassifySolidPointScreened` (coarse quadrature first, strict only when the coarse winding is not within 10⁻⁴ of an integer — the screen GeneralFuse already used), now shared by GeneralFuse, `AssembleBody` and assembly analysis | Same membership; assembly 1 121 → 356 ms |
| 🥇 P4 | `DetectSelfClash`, 80 % of an all-edge chamfer | Edges near a setback corner were scanned against every corner quad: one box around all poles of a quad held them all (line × NURBS: 41 ms per scan) | One box per non-empty span from its own (p+1)(q+1) poles (local convex hull); the scan is clipped to the stretch near any span box | Conservative; same scan spacing |
| 🥇 P5 | Curve–surface polishing | A Newton step pinned at a clip bound repeated 40 times (3 global projections each) | Stop when the clamped τ stops moving — every later iteration is an exact repeat | Bit-identical |
| 🥈 P6 | `BlendEdges` / face changes | A full geometric audit of the **input** body just to read its genus | Genus from `AuditScope::Topology` (computed before the scope return) | Same integer |
| 🥈 P7 | BooleanValidation, 97 % in its oracle | Strict quadrature at 480 points × 5 bodies per scene (95 s per crossing-cylinder scene vs 0.6 s for its General Fuse) | Oracle uses `ClassifySolidPointScreened` | Same verdicts, 0 / 480 everywhere |

## Boolean phases (Release, sum over 78 steps)

| Phase | Before ms | After ms | Speed-up |
|---|---|---|---|
| Total | 2 334 | 1 017 | 2.3× |
| Interference / SSI | 371 | 364 | 1.0× |
| Face splitting | 31 | 31 | 1.0× |
| Classification | 807 | 262 | 3.1× |
| Assembly | 1 121 | 356 | 3.1× |
| Maximal faces | 2 | 3 | — |

| Scene | Steps | Before ms | After ms | Speed-up | Failing before → after |
|---|---|---|---|---|---|
| DrilledPlate | 12 | 104 | 58 | 1.8× | 0 → 0 |
| PocketRow | 8 | 20 | 10 | 1.9× | 0 → 0 |
| Staircase | 8 | 14 | 10 | 1.3× | 0 → 0 |
| CylinderCross | 6 | 126 | 107 | 1.2× | 1 → 0 |
| BlendedBody | 8 | 207 | 76 | 2.7× | 0 → 0 |
| RandomSequence | 36 | 1 863 | 755 | 2.5× | 3 → 0 |

## Blends, every edge at once (Release)

| Body | Profile | Before ms | After ms | Before → after |
|---|---|---|---|---|
| LBracket | fillet 0.10 | 3 001 | 654 | sound → sound |
| Pocket | fillet 0.10 | 7 045 | 1 370 | sound → sound |
| TurnedCut | fillet 0.10 | 6 119 | 1 283 | sound → sound |
| LBracket | fillet 0.30 | 2 796 | 504 | sound → sound |
| LBracket | chamfer 0.10 / 0.30 | — | 511 / 456 | 🔴 refused → 🟢 sound |
| Pocket | chamfer 0.10 / 0.30 | — | 1 063 / 839 | 🔴 refused → 🟢 sound |
| TurnedCut | chamfer 0.10 | — | 1 236 | 🔴 refused → 🟢 sound |
| Pocket | fillet 0.30 | — | 1 126 | 🔴 refused → 🟢 sound |
| Block, Cylinder, Wedge, DrilledBlock, Boss | both, both sizes | 1–19 | 1–19 | sound → sound |

Single-edge blends: every edge of every body accepted, sound, ΔV of the right sign, closed form on the block — before and after (now 10–20 % faster).

## Verification

| Suite | Result |
|---|---|
| AlgorithmStressValidation | 🟢 78 Boolean steps · 32 blend rows · 0 failing (`AlgorithmStressValidation.md`) |
| BooleanValidation | 🟢 PASS — identical B2 ΔV, B3 identities, B4 0 / 480 oracle per scene |
| BlendValidation | 🟢 PASS 15 / 15 · 6 / 6 · 50 specimens — every ΔV identical |
| SolidArc document validation | 🟢 620 / 0 |
| SolidArc walkthroughs (SwiftShader) | 🟢 41 / 41 |
| Windows cross-build | 🟢 0 errors |

## Open items

| | Item | Detail |
|---|---|---|
| 🚧 | Tangent contacts | A boss exactly tangent to a slot wall, and cylinders tangent inside each other: refused cleanly (no wrong solid). Needs tangent-aware interference (a line touching a cylinder gives two crossings √(2r·10⁻⁹) apart today). |
| 🚧 | TurnedCut, size 0.3, all edges | The notch floor wall is 1.0 long with acute corners: the rail crossings alone pass half the edge, so setbacks at both ends overlap ("contact rail runs backwards"). Needs merged corner patches. |
| 💡 | NURBS distance probes | ~100 µs each (5 Newton polishes from the lattice); the remaining all-edge blend time is mostly `DetectSelfClash` probes near setback corners. Changing the start set could change tie-breaks, so it was left alone. |
