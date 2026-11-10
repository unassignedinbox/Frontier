# Frontier

**A real-time engine and authoring environment written from first principles in C++.**

Frontier is built rather than assembled. Surface topology, conformal parameterization, signed-distance
volumes, spatial acceleration, rigid and multiphase simulation, raster execution, a shader compiler
bridge, the multi-document authoring session, and the editor that sits on top of all of it live in this
repository and answer to one placement law and one naming law.

There is no scripting layer, no plugin marketplace, and no borrowed scene graph. If a mechanism is
missing, it gets written here, in the space that owns it, under a name that states what it does.

---

## The two laws

Code is subordinate to these two documents. Where a file contradicts them, the file is wrong.

| Document | Governs |
| --- | --- |
| [`Documentation/FolderStructure.md`](Documentation/FolderStructure.md) | Where every file and folder belongs, what each root may hold, and the order in which legacy names are replaced |
| [`Documentation/BannedWordsVocabulary.md`](Documentation/BannedWordsVocabulary.md) | What a thing may **not** be called, the gates a new name must pass, and the dated rulings that settle it |

`Documentation/Tracker.md` is reserved for engineering tracking — what is being built now, and the
order in which territories are laid down. Architecture guides (the reasoning behind a space, not the
rules for placing a file) live in `EngineDocs/`.

---

## Repository roots

| Root | What lives there |
| --- | --- |
| `Engine/` | Engine source: the domains, the hardware surface, the authoring session, editor tooling, reusable widgets |
| `EngineContent/` | Stock engine assets — icons, fonts, PBR surface definitions, world and graphic archives |
| `EngineDocs/` | Authoritative architectural guides |
| `Documentation/` | Repository governance: structure, naming law, engineering tracking |
| `Exhibits/` | Validation benches, comparison scenes, and `EditorMirror` |
| `Experimental/` | Sandboxed prototypes — never included by a product target |
| `Automation/` | Build tooling, dependency pinning, patches, packaging |
| `ExternalPackages/` | Third-party dependencies, vendored as-is |
| `Projects/` | Game project workspaces — `Projects/SampleProject/{Content,Source,Saved}` |
| `_ClaudeScratch/` | Ignored scratchpad for transient planning notes |

The full annotated tree, down to individual headers, is in
[`Documentation/FolderStructure.md`](Documentation/FolderStructure.md) §4.

---

## What the engine does

Each `…Space` under `Engine/` is a domain with its own vocabulary, its own solvers, and a single
lifecycle coordinator (`…Extension`) that owns its startup, per-tick polling, and shutdown.

**MetricSpace** — reference bases, orthographic and perspective projection, distance metrics, SIMD
vector and quaternion algebra, axis-aligned extents, and ray/volume intersection solvers that every
other space builds on.

**SpatialAcceleration** — broad-phase structure for whichever query shape a domain needs: a
surface-area-heuristic bounding-volume tree, a loose octree, a self-balancing AABB tree for moving
colliders, and sparse spatial hashing.

**DistanceFieldSpace** — voxel-backed signed distance, fast-sweeping and Maurer transforms, trilinear
evaluation, analytic primitive solvers, and ray-march queries for closest-point work.

**GeometryProcessing** — indexed polygon and half-edge complexes, quadric-error-metric decimation,
Catmull-Clark and Loop subdivision, non-manifold detection with boundary-loop traversal, and discrete
mean/Gaussian curvature.

**UVSpace** — LSCM/ARAP chart parameterization, horizon-scanline atlas packing, high-distortion seam
detection, conformal projection, and area/angle distortion measurement.

**GeometricRaster** — render lifecycle coordination, GPU execution sequencing, dependency-sorted
workload scheduling, draw-packet grouping with instancing and indirect submission, descriptor and
push-constant registers, and an ordered post-process sequence (grading and tone curves, bloom, and a
depth-aware bilateral occlusion filter).

**Shaders** — offline and runtime HLSL/GLSL → SPIR-V compilation, attribute and descriptor reflection,
permutation indexing, and the GLSL kernels themselves (raster, compute, image operations).

**SimulationSpace** — velocity-Verlet rigid dynamics with inertia tensors, a projected Gauss-Seidel
constraint solver, sweep-and-prune broad phase, GJK/EPA contact extraction, particle fields and
integrators with GPU buffer synchronisation, SPH liquids, Eulerian smoke and fire with vorticity
confinement, semi-Lagrangian and MacCormack advection, and a motion stack covering interpolation,
easing, spring/inertia/damping dynamics, and timeline sequencing.

**Gameplay** — actor lifecycle and component query storage, sweep-and-slide character movement,
proximity trigger volumes, intent routing from input to the active pawn, and a mode transition machine.

**Input**, **Windowing**, **Interchange**, **Annotation** — hardware polling and command routing; Win32,
X11, Wayland, and headless presentation surfaces; OBJ, glTF 2.0, and FBX intake; world-space and
screen-space debug annotation.

**Workspaces** — the multi-document authoring session: live documents, a workspace registry,
stale-detecting workspace tokens, domain binding, and a branching non-linear revision history with
granular delta intake and cursor resolution.

**NetworkingSpace** — distributed simulation, state sync, and remote directives.

**Components** — the reusable widget vocabulary the editor is assembled from: attribute panels and
reflection-driven controls, section blocks, ribbon and cluster tool rows, a unit-square vector icon
rasterizer, and viewport overlays.

---

## Editor and product are separate builds

`Engine/Editor/` holds the concrete workspaces (modeling, UV, texturing, baking), the explorer
(outliner, attribute inspector, project and content browsers, debug console), the viewports
(projection viewport, vector canvas, transform gizmo, navigation overlay), and diagnostics (tick
profiler, allocation leak tracker, GPU capture recorder).

It is **excluded from production builds**, and nothing outside it may include a header from it. When a
product build needs a behaviour that currently lives there, the behaviour moves into the owning space
and the editor merely presents it.

`Engine/Components/` is the only editor-facing code a product build is allowed to link, and only when
the product is itself shipping a widget surface.

---

## Repository status

Frontier is at its founding stage. The two laws and this index are ratified; the layout in
`FolderStructure.md` is the agreed target, and directories are created as their first file lands rather
than as an empty skeleton.

Two consequences worth stating plainly:

- **Some ratified names are already legacy.** `Engine/MicroUtils/`, `Gameplay/EntityRegistry`,
  `GeometricRaster/DrawPacketDispatcher`, and a dozen others carry vocabulary barred by the naming law.
  They keep their names until their rename lands, and the renames are ordered in
  `FolderStructure.md` §7 so the repository is never half-renamed.
- **The build entry point is not documented yet.** Toolchain, dependency pins, and packaging belong to
  `Automation/`; build instructions land in this file when the toolchain is pinned, not before.

---

## Working in this repository

1. **Read the naming law before writing the first line.** A name that needs a ruling blocks the
   change. Rulings are dated and written down; they are not settled in review comments.
2. **Choose the folder at creation time** using the decision order in `FolderStructure.md` §6.
3. **A `{h,cpp}` pair lands together.** Headers with no translation unit are `.h` and hold declarations,
   tokens, or specifications only.
4. **One concept per file**, one responsibility per space.
5. **A `…Extension` is a lifecycle coordinator**, one per space, and nothing else gets that suffix.
6. **Validation benches assert; they never ship**, and they are not called test harnesses.

---

## Licence

No licence has been declared for this repository. Until one is added, treat the contents as
all-rights-reserved and do not redistribute them.
