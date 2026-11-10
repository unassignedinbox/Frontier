# Folder Structure

**Status:** ratified 2026-11-10 · **Authority:** highest, alongside [`BannedWordsVocabulary.md`](BannedWordsVocabulary.md)
**Scope:** every file and folder in this repository — where it belongs, what it may hold, and which
names are legacy.

Two laws govern this repository. This document says **where a thing lives**. The vocabulary document
says **what a thing may be called**. Neither is advisory: a file that breaks either law is wrong, even
if it compiles.

---

## §1 Scope and authority

1. A path is chosen when the first file lands, not after the folder fills up. Moving a file later costs
   every include site; choosing the wrong folder costs one argument.
2. The tree in §4 is the **ratified layout**. Folders are created as their first file lands; an empty
   directory is not a commitment, it is a placeholder for a decision already made.
3. Names in §4 that carry barred vocabulary are **legacy identifiers**: they stay until their rename
   lands in the order given in §7. The tree records reality; §7 records the debt.
4. Only two documents may be edited to change the layout or the naming law: this one and
   `BannedWordsVocabulary.md`.

---

## §2 Notation

| Glyph | Meaning |
| --- | --- |
| `├──` `└──` `│` | ratified position in the tree |
| `//` | one-line statement of responsibility — what the folder *does*, not what it is called |
| `{h,cpp}` | a paired declaration and translation unit — both files exist or neither does |
| `.h` alone | a header with no translation unit (pure declarations, tokens, or specifications) |
| `[R]` | legacy name awaiting rename (§7) |

---

## §3 Top-level roots

Eleven roots. Nothing else is created at the repository top.

| Root | Holds | Must never hold |
| --- | --- | --- |
| `Engine/` | Engine source root: domains, hardware surface, authoring session, editor tooling, reusable widgets | project-specific gameplay; third-party code |
| `EngineContent/` | Stock engine assets: icons, fonts, PBR surface definitions, worlds, image and vector archives | project assets |
| `EngineDocs/` | Authoritative architectural guides | governance (that lives in `Documentation/`); code |
| `Documentation/` | Repository governance and engineering tracking | architecture guides |
| `Exhibits/` | Validation benches, comparison scenes, and `EditorMirror` | shipping code; anything a product target links |
| `Experimental/` | Sandboxed prototypes and scratch implementations | code any product target includes |
| `Automation/` | Build tooling, dependency pinning, patches, packaging | engine source |
| `ExternalPackages/` | Third-party dependencies, vendored as-is | local edits to vendored code (patch via `Automation/`) |
| `Projects/` | Game project workspaces, one folder per project | engine-shared code |
| `_ClaudeScratch/` | Ignored scratchpad and transient planning notes `[R]` | anything needed to build |
| `Engine/NetworkingSpace/` | Distributed simulation, state sync, remote directives | — |

`NetworkingSpace` was transcribed at the top level (§8, correction 6). It is engine source like every
other `…Space`, so it is nested under `Engine/`.

---

## §4 Ratified tree

```text
Frontier/
├── EngineContent/                          // Engine default assets and stock libraries
│   ├── Icons/                              // Vector SVGs and compiled icon catalogs
│   ├── FontArchives/                       // Primary UI and monospace debug typefaces
│   ├── MaterialArchives/                   // Stock PBR surface definitions
│   ├── WorldArchive/                       // Levels and worlds
│   └── GraphicArchives/                    // Image formats and SVG payloads
│
├── EngineDocs/                             // Authoritative architectural guides and primary skills
│
├── Documentation/                          // Repository governance and engineering tracking
│   ├── FolderStructure.md                  // This document — placement law
│   ├── BannedWordsVocabulary.md            // Naming law, gates, and dated rulings
│   └── Tracker.md                          // Project tracking: active work and the order territories land
│
├── Exhibits/                               // Validation benches, comparison scenes, and EditorMirror
│
├── Experimental/                           // Sandboxed R&D prototypes and scratch implementations
│
├── Automation/                             // Build tooling, dependency pinning, patches, and packaging
│
├── ExternalPackages/                       // Third-party external dependencies
│
├── _ClaudeScratch/                         // Ignored scratchpad and transient planning notes        [R]
│
├── Projects/                               // User project workspace container
│   └── SampleProject/                      // Concrete game project
│       ├── Content/                        // Project-specific assets (levels, textures, audio)
│       ├── Source/                         // Project-specific C++ logic
│       └── Saved/                          // Local project caches, logs, and saved sessions
│
└── Engine/                                 // Engine source root
    │
    ├── MicroUtils/                         // Foundational allocators and identity tokens                    [R]
    │   ├── RegionAllocator.{h,cpp}         // Monotonic non-freeing block allocator
    │   ├── TokenIssuer.h                   // Stale-detecting monotonic ID generator
    │   ├── TokenAuthentication.h           // Validation primitives for issued handles
    │   ├── BitSpan.h                       // Contiguous bit manipulation span
    │   ├── ObjectReservoir.h               // Preallocated indexed instance recycler                        [R]
    │   └── Assertion.h                     // Invariant enforcement macros and diagnostics
    │
    ├── Configuration/                      // Startup and authoring configurations
    │   ├── EngineConfiguration.{h,cpp}     // Space startup and thread preallocation
    │   ├── RenderConfiguration.{h,cpp}     // Resolution, swapchain, and multi-sample targets
    │   ├── ProjectConfiguration.{h,cpp}    // Project root, dependency manifests, and switches
    │   └── WorkspaceConfiguration.{h,cpp}  // Document session preferences and layout defaults
    │
    ├── MetricSpace/                        // Reference bases, projections, and spatial math
    │   ├── CoordinateSpace.h               // Reference-basis transformations and basis vectors
    │   ├── CoordinateProjection.h          // Orthographic, perspective, and screen projections
    │   ├── DistanceMetric.h                // Metric evaluation (Euclidean, Manhattan, Chebyshev)
    │   ├── SpatialExtent.h                 // Axis-aligned extents and volumetric bounds
    │   ├── LinearAlgebra.h                 // SIMD-aligned vector algebra definitions
    │   ├── VectorAlgebra.{h,cpp}
    │   ├── MatrixOperations.{h,cpp}                                                                    [R]
    │   ├── QuaternionRotation.{h,cpp}
    │   └── IntersectionSolver.{h,cpp}      // Ray-volume and volume-volume geometric solvers
    │
    ├── SpatialAcceleration/                // Broad-phase spatial acceleration structures
    │   ├── BoundingVolumeHierarchy.{h,cpp} // Surface-area heuristic split tree                        [R]
    │   ├── LooseOctree.{h,cpp}             // Expanded boundary volumetric spatial tree
    │   ├── DynamicAABBTree.{h,cpp}         // Self-balancing binary tree for dynamic colliders
    │   └── SpatialGridHash.{h,cpp}         // Sparse spatial hashing structure
    │
    ├── DistanceFieldSpace/                 // Volumetric SDFs, implicit geometry, and queries
    │   ├── SignedDistanceField.{h,cpp}     // Voxel-backed signed distance representations
    │   ├── DistanceTransform.{h,cpp}       // Fast sweeping and Maurer distance transforms
    │   ├── VolumetricDistance.{h,cpp}      // Trilinear volume evaluation and normal reconstruction
    │   ├── DistanceQuery.{h,cpp}           // Ray marching and closest-point queries
    │   └── SDFPrimitiveSolver.{h,cpp}      // Analytic primitive distance solvers
    │
    ├── GeometryProcessing/                 // Topological surface complexes and discrete operators
    │   ├── PolygonComplex.{h,cpp}          // Indexed polygon collection and topological groupings
    │   ├── HalfEdgeComplex.{h,cpp}         // Directional half-edge adjacency structure
    │   ├── DecimationSolver.{h,cpp}        // Quadric error metric (QEM) edge collapse simplifier
    │   ├── SubdivisionSurface.{h,cpp}      // Catmull-Clark and Loop refinement operators
    │   ├── SurfaceTopology.{h,cpp}         // Non-manifold detection and boundary loop traverser
    │   └── CurvatureAnalysis.{h,cpp}       // Discrete mean and Gaussian curvature calculation
    │
    ├── UVSpace/                            // Surface parameterization, chart routing, and packing
    │   ├── ChartParameterizer.{h,cpp}      // Least Squares Conformal (LSCM) / ARAP solver
    │   ├── UVAtlasPacker.{h,cpp}           // Horizon scanline chart packing
    │   ├── SeamDetector.{h,cpp}            // High-distortion edge loop finder
    │   ├── ConformalProjector.{h,cpp}      // Angle-preserving harmonic coordinate projector
    │   └── DistortionMetrics.{h,cpp}       // Area/angle deformation measurement
    │
    ├── GeometricRaster/                    // Hardware draw submission, commands, and GPU resources
    │   ├── RenderExtension.{h,cpp}         // Render lifecycle coordinator & device attachment point
    │   ├── RasterExecutionPath.{h,cpp}     // GPU execution sequence
    │   ├── RenderScheduler.{h,cpp}         // Transient dependency-sorted workload coordinator
    │   ├── DrawPacketDispatcher.{h,cpp}    // Draw-call grouping, instancing, and indirect dispatch    [R]
    │   ├── RenderDeviceSpecification.h     // GPU backend abstract specification
    │   ├── ShaderDescriptorRecord.{h,cpp}  // Descriptor and push-constant register                   [R]
    │   └── PostProcess/                    // Ordered image operations
    │       ├── PostProcessSequence.{h,cpp} // Ordered image operation execution sequence
    │       ├── ToneCurveComposite.{h,cpp}  // Color grading, ACES transform, and exposure             [R]
    │       ├── OpticalBloom.{h,cpp}        // Multi-scale downsample/upsample blur & threshold
    │       └── AmbientOcclusionFilter.{h,cpp} // Bilateral depth-aware spatial filtering
    │
    ├── Shaders/                            // GPU shader source library and compiler bridge
    │   ├── Compiler/
    │   │   ├── ShaderCompiler.{h,cpp}      // Offline and runtime HLSL/GLSL to SPIR-V compiler
    │   │   ├── ShaderReflection.{h,cpp}    // Vertex attribute and descriptor reflection
    │   │   └── ShaderPermutationIndex.{h,cpp} // Precompiled variant permutation register
    │   ├── Raster/                         // Raster kernels
    │   │   ├── BasePBR.vert.glsl                                                                       [R]
    │   │   ├── BasePBR.frag.glsl                                                                       [R]
    │   │   └── EarlyDepthOnly.vert.glsl
    │   ├── Compute/                        // General compute kernels
    │   │   ├── ParticleIntegration.comp.glsl
    │   │   ├── FluidAdvection.comp.glsl
    │   │   └── DistanceFieldGenerator.comp.glsl
    │   └── ImageOperations/                // Post-process image filters
    │       ├── ToneMapping.comp.glsl
    │       └── BloomDownsample.comp.glsl
    │
    ├── SimulationSpace/                    // Physics, dynamics, particles, and fluids
    │   ├── PhysicsSpace/                   // Rigid mechanics and constraints
    │   │   ├── PhysicsExtension.{h,cpp}    // Simulation loop coordinator and broad-phase attachment point
    │   │   ├── RigidBodyDynamics.{h,cpp}   // Velocity Verlet integration and inertia tensors
    │   │   ├── ConstraintSolver.{h,cpp}    // Projected Gauss-Seidel iterative velocity solver
    │   │   ├── BroadphaseSolver.{h,cpp}    // Sweep-and-prune dynamic bounding query
    │   │   └── CollisionDetection.{h,cpp}  // GJK narrow-phase & EPA contact extraction
    │   ├── ParticleSimulation/             // High-density particulate dynamics
    │   │   ├── ParticleEmitter.{h,cpp}     // Point, volume, and surface particle release            [R]
    │   │   ├── ParticleField.{h,cpp}       // Curl noise, vortex, and gravitational vector fields
    │   │   ├── ParticleIntegrator.{h,cpp}  // High-performance Euler and Verlet particle solvers
    │   │   └── ComputeParticleBuffer.{h,cpp} // GPU-side direct compute buffer synchronisation
    │   ├── FluidSimulation/                // Multiphase liquids and gases
    │   │   ├── LiquidSolver.{h,cpp}        // Incompressible SPH liquid & surface solver (water)
    │   │   ├── GasDynamics.{h,cpp}         // Eulerian grid smoke, fire, and atmosphere solver
    │   │   ├── VorticityConfinement.{h,cpp} // Turbulent detail and rotational energy preservation
    │   │   ├── AdvectionSolver.{h,cpp}     // Semi-Lagrangian and MacCormack advection schemes
    │   │   └── FluidBoundarySolver.{h,cpp} // Dynamic collision interaction with solid geometry
    │   └── Motion/                         // Procedural interpolation, dynamics, and timelines
    │       ├── MotionSolver.{h,cpp}        // Kinetic motion resolver
    │       ├── Interpolation/
    │       │   ├── ScalarInterpolation.{h,cpp}
    │       │   ├── VectorInterpolation.{h,cpp}
    │       │   └── ColorInterpolation.{h,cpp}
    │       ├── Easing/
    │       │   ├── EasingProfile.{h,cpp}
    │       │   ├── BezierEasing.{h,cpp}
    │       │   └── EaseCatalog.{h,cpp}     // Precomputed response curves
    │       ├── Dynamics/
    │       │   ├── SpringDynamics.{h,cpp}
    │       │   ├── InertiaDynamics.{h,cpp}
    │       │   └── DampingDynamics.{h,cpp}
    │       └── Sequencing/
    │           ├── TransitionDescriptor.h
    │           ├── SequenceAnimator.{h,cpp}
    │           └── TimelineAnimator.{h,cpp}
    │
    ├── Gameplay/                           // Generic engine-level gameplay foundations
    │   ├── EntityRegistry.{h,cpp}          // Actor lifecycle and component query storage              [R]
    │   ├── KinematicController.{h,cpp}     // Character movement and sweep-and-slide resolution        [R]
    │   ├── SpatialTriggerVolume.{h,cpp}    // Proximity-based collision trigger areas
    │   ├── PlayerIntentRouter.{h,cpp}      // Routes input commands to active pawn targets
    │   └── StateTransitionMachine.{h,cpp}  // Tiered mode transition machine for gameplay actors      [R]
    │
    ├── Input/                              // Peripheral hardware polling and routing
    │   ├── InputExtension.{h,cpp}          // Hardware event intake to command routing
    │   ├── DeviceRegistry.{h,cpp}          // Connected controllers, mice, and keyboards
    │   ├── CommandRouteIndex.{h,cpp}       // Direct command routing registry
    │   └── RawInputRouter.{h,cpp}          // OS event translation into engine input signals
    │
    ├── Windowing/                          // Native platform surface abstractions
    │   ├── PlatformWindow.h                // Shared window abstraction
    │   ├── PlatformWindowWin32.cpp         // Win32 / DXGI surface presentation
    │   ├── PlatformWindowX11.cpp           // Xlib / GLX presentation
    │   ├── PlatformWindowWayland.cpp       // Wayland XDG shell presentation
    │   ├── PlatformWindowStub.cpp          // Headless testing presentation
    │   └── PresentationSurface.{h,cpp}     // Surface capabilities, swap intervals, and DPI
    │
    ├── Interchange/                        // Native format asset readers and serializers
    │   ├── ModelInterchange.{h,cpp}        // Scene tree intake coordinator
    │   ├── InterchangeReaders.h            // Format recognition and reader registry
    │   ├── InterchangeWavefrontReader.{h,cpp} // OBJ decoder
    │   ├── InterchangeGltfReader.{h,cpp}   // glTF 2.0 binary and JSON reader
    │   └── InterchangeFilmboxReader.{h,cpp} // FBX interchange reader
    │
    ├── Annotation/                         // Visual annotations, callouts, and debug drawing
    │   ├── AnnotationHub.{h,cpp}           // Central router and lifetime coordinator                  [R]
    │   ├── VectorAnnotation.{h,cpp}        // 2D/3D vector line, arrow, Bezier, and curve markers
    │   ├── TextAnnotation.{h,cpp}          // Billboarded world-space and screen-space text callouts
    │   ├── SpatialAnnotation.{h,cpp}       // 3D debug bounding volumes, coordinate axes, and shapes
    │   └── DiagnosticPrimitives.{h,cpp}    // Immediate-mode wireframe boxes, spheres, rays, and cages
    │
    ├── Workspaces/                         // Multi-document session and authoring engine
    │   ├── WorkspaceExtension.{h,cpp}      // Workspace creation infrastructure and tab control
    │   ├── WorkspaceDocument.{h,cpp}       // Live editable document state
    │   ├── WorkspaceRegistry.{h,cpp}       // Active workspace tracker
    │   ├── WorkspaceToken.h                // Stale-detecting token with embedded WorkspaceKind
    │   ├── AuthoringWorkspace.{h,cpp}      // Domain binder (links document to target complex)
    │   └── Revision/                       // Branching per-document state history
    │       ├── RevisionExtension.{h,cpp}   // Revision coordinator
    │       ├── ContentRegion.{h,cpp}       // Monotonic document memory block
    │       ├── DeltaIntake.{h,cpp}         // Granular change intake
    │       ├── CursorResolution.{h,cpp}    // Active entry resolution in non-linear history
    │       └── NonLinearHistory.{h,cpp}    // Directed acyclic history tree
    │
    ├── NetworkingSpace/                    // Distributed simulation, state sync, and remote directives
    │   └── NetworkingExtension.{h,cpp}     // Lifecycle coordinator: polls sockets, ticks replication,
    │                                       // flushes buffers
    │
    ├── Editor/                             // Tooling and diagnostics ONLY (excluded from production builds)
    │   ├── Workspaces/                     // Concrete editor workspace implementations
    │   │   ├── ModelingWorkspace.{h,cpp}   // Geometry modeling, extrude, bevel, cut
    │   │   ├── UVWorkspace.{h,cpp}         // Seam marking, unwrapping, and relaxation
    │   │   ├── TexturingWorkspace.{h,cpp}  // Projection painting and volumetric stamp brushes
    │   │   └── BakingWorkspace.{h,cpp}     // Ambient occlusion and curvature texture baking
    │   ├── Explorer/                       // Project structure and browser tools
    │   │   ├── SceneOutliner.{h,cpp}       // Actor and spatial scene structure navigator
    │   │   ├── PropertyInspectorHost.{h,cpp} // Reflected attribute inspector
    │   │   ├── ProjectBrowser.{h,cpp}      // Multi-workspace and project document explorer
    │   │   ├── ContentBrowser.{h,cpp}      // Asset packages, materials, and runtime contents
    │   │   └── DebugConsole.{h,cpp}        // Interactive command interpreter and diagnostic log
    │   ├── Viewports/                      // General-purpose rendering and editing viewports
    │   │   ├── ProjectionViewport.{h,cpp}  // 3D general-purpose scene viewport and camera anchor
    │   │   ├── VectorCanvas.{h,cpp}        // 2D vector painting and graphics canvas viewport
    │   │   ├── TransformGizmo.{h,cpp}      // Translation, rotation, and scale manipulators
    │   │   └── NavigationOverlay.{h,cpp}   // Grid, axes, and orientation indicator
    │   └── Diagnostics/                    // Profiling and engine instrumentation
    │       ├── CycleProfiler.{h,cpp}       // CPU/GPU tick profiler
    │       ├── MemoryDebugger.{h,cpp}      // Allocation leak tracker and region visualizer
    │       └── GPUCaptureRecorder.{h,cpp}  // RenderDoc / PIX programmatic capture recorder
    │
    └── Components/                         // Reusable UI widgets and presentation surfaces
        ├── PropertyPanels/
        │   ├── PropertyPanelBase.{h,cpp}   // Foundation widget for attribute inspectors           [R]
        │   └── AttributeReflector.{h,cpp}  // Type-to-control generator
        ├── SectionBlocks/                  // Segmented content groupings
        │   ├── ToolRibbon.{h,cpp}          // Primary linear tool ribbons
        │   ├── SectionHeader.{h,cpp}       // Collapsible category titles
        │   ├── ContentSection.{h,cpp}      // Enclosed widget group
        │   └── ContentScroller.{h,cpp}     // Paginated content scroller
        ├── Controls/                       // Interactive numerical and scalar inputs
        │   ├── ValueSlider.{h,cpp}
        │   ├── VectorEntry.{h,cpp}
        │   ├── ScalarEntry.{h,cpp}
        │   ├── BooleanEntry.{h,cpp}
        │   ├── ColorEntry.{h,cpp}
        │   ├── SelectionEntry.{h,cpp}
        │   ├── PathEntry.{h,cpp}
        │   ├── Dropdown.{h,cpp}
        │   ├── ControlLayout.{h,cpp}
        │   └── EnumerationSelector.{h,cpp} // Strongly-typed discrete mode and enum selector
        ├── VectorIcons/                    // Unit-square vector shape icons and rasterization
        │   ├── VectorIcon.{h,cpp}          // Unit-square vector icon descriptor
        │   ├── IconGeometry.{h,cpp}        // Normalized [0, 1] path and stroke buffers
        │   ├── IconRasterizer.{h,cpp}      // Specialized unit-square rasterizer and atlas cache
        │   └── VectorRasterizer.{h,cpp}    // Generalized analytical vector tessellator/rasterizer
        └── Overlays/                       // Viewport and cursor interaction overlays
            ├── ToolShelf.{h,cpp}           // Viewport-anchored tool row (ToolCluster / UtilityStrip)
            ├── ToolCluster.{h,cpp}         // Compact group of related manipulation tools
            ├── UtilityStrip.{h,cpp}        // Lightweight secondary tool bar for viewport toggles   [R]
            ├── ContextDirectiveList.{h,cpp} // Cursor-anchored contextual directives                  [R]
            └── CommandLauncher.{h,cpp}     // Fuzzy-search directive runner
```

---

## §5 Placement rules

1. **One home per concern.** If a piece of logic could live in two folders, the domain owns it and the
   consumer includes it. Shared widgets never reach into `SimulationSpace/`; simulation never reaches
   into `Components/`.
2. **`Engine/` versus `EngineContent/` versus `Projects/`.** Source in `Engine/`; stock assets in
   `EngineContent/`; anything a specific game owns in `Projects/<name>/`.
3. **A `…Space` is a domain.** `MetricSpace`, `SpatialAcceleration`, `DistanceFieldSpace`,
   `GeometryProcessing`, `UVSpace`, `SimulationSpace`, `Gameplay`, `Input`, `Windowing`, `Interchange`,
   `Annotation`, `Workspaces`, `NetworkingSpace`. Spaces do not include each other's implementation
   headers — only their public declarations.
4. **`Extension` is a lifecycle coordinator slot.** Exactly one per space: startup order, per-tick
   polling, shutdown. See `BannedWordsVocabulary.md` §3.1.
5. **`Editor/` is excluded from production builds.** Nothing outside `Editor/` may include a header
   from `Editor/`. If a product build needs it, the logic belongs in the owning space and `Editor/`
   only presents it.
6. **`Experimental/` never ships.** No product target may include or link it.
7. **`Exhibits/` validates; it never ships.** A bench asserts behaviour of a ratified path; it does not
   become a production dependency. A validation bench is never called a harness.
8. **`ExternalPackages/` is never edited in place.** Behaviour changes are patches under `Automation/`.
9. **`Automation/` owns pins, patches, and packaging.** A version number for a dependency appears once,
   in `Automation/`.
10. **`_ClaudeScratch/` is ignored by version control.** Notes that matter are moved into
    `Documentation/`; notes that do not are deleted.
11. **`{h,cpp}` pairs are inseparable.** A header with no translation unit uses `.h` alone and holds
    only declarations, tokens, or specifications (e.g. `SpatialExtent.h`, `WorkspaceToken.h`).
12. **One concept per file.** A file named `X` declares the mechanism for `X`. If a second mechanism
    appears, it gets a second file in the same folder.

---

## §6 Where does new code go?

Answer in this order and stop at the first match.

1. Third-party, vendored? → `ExternalPackages/`
2. Build, pin, patch, package? → `Automation/`
3. Throwaway prototype? → `Experimental/`
4. Bench that asserts behaviour? → `Exhibits/`
5. Only meaningful inside the editor? → `Engine/Editor/<area>/`
6. Reusable widget or presentation surface? → `Engine/Components/<group>/`
7. Generic gameplay foundation usable by any project? → `Engine/Gameplay/`
8. A domain mechanism (math, geometry, SDF, UV, simulation, raster, shader, session, input, window,
   interchange, annotation, network)? → the owning `Engine/<Domain>Space/`
9. This game's logic? → `Projects/<name>/Source/`
10. This game's assets? → `Projects/<name>/Content/`

If none match, the concern is not yet understood — decide in `Documentation/Tracker.md` before creating
the folder.

---

## §7 Legacy renames awaiting ruling

The tree in §4 records the layout as it stands. The paths below carry vocabulary barred by
`BannedWordsVocabulary.md` §1 or §1.9. All but one are replaced, in the order given; #17 records an
exemption so the argument is never reopened. Each proposal needs a dated entry in the vocabulary
document (§7 there) before the rename lands; the folder keeps its legacy name until then, so the
repository is never half-renamed.

| # | Legacy path | Barred token | Ruling | Proposed | Notes |
| --- | --- | --- | --- | --- | --- |
| 1 | `Engine/MicroUtils/` | `Utility` | §1.4 | `Engine/MicroFoundation/` | folder names its primitives, not its convenience |
| 2 | `Engine/MicroUtils/ObjectReservoir.h` | `Object`, `reservoir` | §1.4, §1.9 | `InstanceShelf.h` | "preallocated indexed instance recycler" is the mechanism |
| 3 | `Engine/Gameplay/EntityRegistry.{h,cpp}` | `Entity` | 2026-07-25 | `ActorRegistry.{h,cpp}` | `Actor` is the proposed gameplay noun |
| 4 | `Engine/Gameplay/KinematicController.{h,cpp}` | `Controller` | §1.4 | `KinematicResolver.{h,cpp}` | it resolves sweeps, it does not control |
| 5 | `Engine/Gameplay/StateTransitionMachine.{h,cpp}` | `state` | §1.8 | `ModeTransitionMachine.{h,cpp}` | name what it holds: the mode |
| 6 | `Engine/SpatialAcceleration/BoundingVolumeHierarchy.{h,cpp}` | `Hierarchy` | 2026-07-25 | `BoundingVolumeTree.{h,cpp}` | also updates the §6 replacement chart example |
| 7 | `Engine/Annotation/AnnotationHub.{h,cpp}` | `Hub` | §1.2 | `AnnotationRouter.{h,cpp}` | matches `RawInputRouter`, `PlayerIntentRouter` |
| 8 | `Engine/GeometricRaster/DrawPacketDispatcher.{h,cpp}` | `Dispatcher` | §1.4 | `DrawPacketSubmitter.{h,cpp}` | "dispatch" stays as a GPU verb in comments |
| 9 | `Engine/GeometricRaster/ShaderDescriptorRecord.{h,cpp}` | bare `Record` | §1.4† | `ShaderDescriptorRegister.{h,cpp}` | §4 composites do not fit a descriptor block |
| 10 | `Engine/GeometricRaster/PostProcess/ToneCurveComposite.{h,cpp}` | `Compose` | §1.7 | `ToneCurveRasterization.{h,cpp}` | 2026-08-02 ruling: composite-into-target ⇒ `…Rasterization` |
| 11 | `Engine/Shaders/Raster/BasePBR.{vert,frag}.glsl` | `Base` | §1.4 | `SurfacePBR.{vert,frag}.glsl` | stock surface kernel, not a base class |
| 12 | `Engine/SimulationSpace/ParticleSimulation/ParticleEmitter.{h,cpp}` | `Emit` / `Emission` | 2026-07-26 | `ParticleSource.{h,cpp}` | comment becomes "particle release" |
| 13 | `Engine/Components/PropertyPanels/PropertyPanelBase.{h,cpp}` | `Base` | §1.4 | `PropertyPanelFoundation.{h,cpp}` | |
| 14 | `Engine/Components/Overlays/UtilityStrip.{h,cpp}` | `Utility` | §1.4 | `SecondaryStrip.{h,cpp}` | its own comment already says "secondary tool bar" |
| 15 | `Engine/Components/Overlays/ContextDirectiveList.{h,cpp}` | `context`, `list` | §1.9 | `CursorDirectiveRoster.{h,cpp}` | cursor-anchored is the anchor, not "context" |
| 16 | `_ClaudeScratch/` | AI trope branding | §1.4 spirit | `_Scratch/` | ignored either way; cheap to fix |
| 17 | `Engine/MetricSpace/MatrixOperations.{h,cpp}` | `Matrix` | §1.2 | *(no rename proposed)* | `Matrix` is barred as a center-of-universe trophy, not as linear algebra. Keep; record the exemption here rather than losing the argument twice. |
| 18 | `Engine/Workspaces/WorkspaceToken.h` | `Kind` | 2026-06-20 | in-file: `WorkspaceDomain` enum | path unchanged; only the embedded enum is renamed |

**Order.** 1–3 first (they are the ones the vocabulary chart cites as examples), then 4–12, then 13–16,
then 18 (an enum inside an otherwise correctly named header). #17 is never moved. A rename is one
change: move the files, update every include site, and update §4 and this chart in the same edit.

### Reviewed and cleared

Recorded so the same argument is not reopened:

| Path | Near-miss | Why it stands |
| --- | --- | --- |
| `Engine/GeometricRaster/RenderExtension`, `PhysicsExtension`, `InputExtension`, `WorkspaceExtension`, `RevisionExtension`, `NetworkingExtension` | `Extension` is restricted | each is the single lifecycle coordinator for its space (§3.1) |
| `Engine/Shaders/Compiler/ShaderCompiler` | "compiler" is framework furniture | it is literally an HLSL/GLSL → SPIR-V compiler |
| `Engine/Editor/Explorer/PropertyInspectorHost` | job-title family (§1.5) | `Inspector` is not on the barred list and the mechanism is reflection |
| `Engine/Editor/Diagnostics/GPUCaptureRecorder` | bare `Record` barred | `Recorder` is the agent noun for a capture, not the barred bare token |
| `Engine/Components/VectorIcons/IconRasterizer` | render-unit suffix | CPU tessellator, not a GPU composite-into-target unit; §2 does not reach it |
| `Engine/GeometryProcessing/SurfaceTopology` | `Graph` banned | topology is the domain; no graph vocabulary used |
| `Engine/Workspaces/Revision/NonLinearHistory` | kinship terms barred | links are up-link / down-link / same-tier link per §5 |

---

## §8 Corrections applied to the ratified transcription

The layout was handed over as prose plus an ASCII sketch. Eight transcription defects were repaired;
none of them changes a decision.

1. `EngineContent/WorldsArchives (contins levels worlds )/GraphicArchives(contain image formats +SVG)`
   was one run-on line. It is two sibling folders: `WorldArchive/` (levels and worlds) and
   `GraphicArchives/` (image formats and SVG payloads).
2. `FontsArchives/` → `FontArchives/` (a modifier is singular in a compound).
3. `MaterialArchives/` kept; comment de-jargoned from "Base PBR surface definitions" to "Stock PBR
   surface definitions".
4. A dangling `└──` with no entry closed the `EngineContent` group; removed.
5. `Documentation/Tracker.md //Project racking` → `// Project tracking`.
6. `NetworkingSpace/` appeared after `Engine/` closed, at the same indentation as the roots. Nested
   under `Engine/` (§3), since it holds engine source.
7. Indentation was regenerated with consistent glyphs; long comments were wrapped rather than truncated.
8. The barred-vocabulary list that followed the tree was moved into
   `Documentation/BannedWordsVocabulary.md`, where rulings are dated.

Fifteen comment concepts were rewritten to satisfy the naming law while keeping the mechanism:

| Was | Now | Barred token |
| --- | --- | --- |
| "Validation test harnesses, benches" | "Validation benches, comparison scenes" | `harness` |
| "core skills" | "primary skills" | `Core` |
| "frame-of-reference" | "reference-basis" | `Frame` |
| "System startup" | "Startup" | `system` |
| "Subsystem startup" | "Space startup" | `Subsystem` |
| "thread reservoir allocation" | "thread preallocation" | `reservoir` |
| "Common window abstraction" | "Shared window abstraction" | `Common` |
| "Kinetic state and motion resolver" | "Kinetic motion resolver" | `state` |
| "Paginated element scroller" | "Paginated content scroller" | `element` |
| "Coordinate frames" | "Reference bases" | `Frame` |
| "Native operating system surface" | "Native platform surface" | `system` |
| "Core Engine Source Root" | "Engine source root" | `Core` |
| "save data" | "saved sessions" | `Data` |
| "Hierarchical mode transition machine" | "Tiered mode transition machine" | `Hierarchy` |
| "presentation elements" | "presentation surfaces" | `element` |

Comments are part of the naming law: a barred word in a comment teaches the next author to use it in
an identifier.

---

## §9 Document map

| Question | Document |
| --- | --- |
| Where does this file go? | this document, §5–§6 |
| What may I call it? | [`BannedWordsVocabulary.md`](BannedWordsVocabulary.md), §0–§4 |
| What is being renamed, and in what order? | this document, §7 |
| What is being worked on now? | `Documentation/Tracker.md` |
| How is this area architected? | `EngineDocs/` |
| What does the engine do overall? | [`../README.md`](../README.md) |
