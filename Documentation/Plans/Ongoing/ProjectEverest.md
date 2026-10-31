# Project Everest — VehicleHMI to Vulkan port

## TL;DR

The VehicleHMI demo (ReSTIR GI + ray tracing, HTML/WebGPU) becomes a native C++/Vulkan/Slang Frontier project,
opened from the Project Browser as **Project Everest**. The UI is ported first and 1:1; nothing else starts until
it is accepted. No branch holds the whole answer: branch 0 holds the newest UI, branch 3 holds the engine and the
dual GTX/RTX renderer, branches 1 and 2 hold fog and atmosphere research. The plan merges branch 0 as the base,
absorbs branch 3's engine, and ports the UI on top. This turn delivers the plan, the folder map, and the evidence
contracts — no source, no binaries.

Status: plan only, awaiting owner approval. Written 2026-10-31 against the four tips in
`Documentation/Records/SourceBranchLedger.md`.

## 1. What exists today (measured, not assumed)

| Area | Branch 0 (base) | Branch 3 (donor) | Gap to Everest |
|------|-----------------|------------------|----------------|
| Docking, notch, Control Centre, Content Browser, Notifications | Complete, newest | Complete, older | None — reuse |
| Project Browser, Project Space | Complete | Complete | Add the Everest card |
| Outliner (`SceneDirectoryPanel`) | Complete; rows, fold, filter, spawn | Complete + `EnvironmentRecords` groups | Demo groups, counts, row metadata |
| Log Stream | Complete, 1:1 with its approved HTML | Complete | None — reuse |
| Inspector | Absent | Host + flat field description, environment slice only | 16 page families, 138 labels |
| Viewport chrome (bar, HUD, inset, notices, bring-up, failure) | Absent | Absent | Full port |
| Status bar | Absent | Absent | Full port |
| Dedicated studios | Absent | `DedicatedEntityEditors` (770 lines, zero-copy) | Rename + wire |
| Renderer | CPU reference cores, 2 Slang bodies | `SceneRenderer` 3,681 lines, 21 Slang units, GTX + RTX | Move into Everest, finish units |
| Physics, fluid, particles, fracture | Absent | Jolt 240 Hz, `PlumeFluid`, `SolidFracture`, `SparkParticles` | Move into Everest |
| Demo contract tables | 473 files / 80 shaders / 81 entrypoints / 138 labels | Same | The 1:1 worklist |

## 2. Import strategy

Base tree = branch 0. Engine and renderer = branch 3. Research = branches 1 and 2, read-only.

```text
git merge --allow-unrelated-histories <branch-0>     # rules + full UI, newest
git merge --allow-unrelated-histories <branch-3>     # engine, renderer, Project-Drive, exhibits
```

| Step | Action | Conflict expectation | Resolution rule |
|------|--------|----------------------|-----------------|
| 1 | Merge branch 0 into `arena/580ab89f-frontier` | `README.md`, `.gitignore` | Branch 0 wins; this branch's README replaced by the project README |
| 2 | Merge branch 3 | `CMakeLists.txt`, `Frontier/Applications/Editor/FrontierMain.cpp`, `Documentation/Tracker.md`, `CLAUDE.md`, `Engine/` identifier files | Take branch 0 for UI files, branch 3 for engine files, then hand-merge the two `CMakeLists` and `FrontierMain` into one composition |
| 3 | Rename donor violations | None | `FrameGraphExecution` → `SubmissionOrderExecution`; `DedicatedEntityEditors` → `DedicatedRecordStudios` |
| 4 | Move renderer | None | `Frontier/Engine/Rendering/*` stays where it is; `Engine/Render/` (base 0's shader-adjacent folder) keeps only UI textures and is not given engine code |
| 5 | Verify | — | Imported validation targets build and pass on the host; GPU-only translation units are compile-proved and reported as not executed |

⚠️ `ExternalPackages/` is ignored in every branch. The import does not carry ImGui, GLFW, toml++, ThorVG, or the
Vulkan headers. `Automation/PrepareUI.py`, `StageThorVG.py`, and `PrepareFonts.py` restore them; no build runs
before that step succeeds.

## 3. Target folder structure

```text
Frontier/
├── CLAUDE.md                          # rules + Everest briefing + branch ledger pointer
├── AGENTS.md
├── README.md
├── CMakeLists.txt                     # FrontierUI · Frontier · ProjectEverest · TextureStudio · validations
├── Automation/
│   ├── SyncFromMain.sh                # NEW: fetch + merge branch 0, append sync-log row, verify
│   ├── PrepareUI.py · PrepareFonts.py · StageThorVG.py · Patches/ (A–G)
│   └── UIDependencies.json · FontSources.json
├── EngineDocs/AgenticInstuctions/     # four retained skills, unedited
├── Documentation/
│   ├── Tracker.md                     # single tracker
│   ├── FolderStructure.md             # updated by this plan's §3
│   ├── EnvironmentTodo.md
│   ├── Records/
│   │   ├── SourceBranchLedger.md      # NEW: four branches, triage, reference pins
│   │   ├── SyncLog.md                 # NEW: append-only base-tree merge record
│   │   └── EngineCheckpointLedger.md  # NEW: donor CLAUDE.md history, verbatim, linked not merged
│   ├── Rules/
│   └── Plans/{Ongoing,Completed,References}/
│       └── Ongoing/
│           ├── ProjectEverest.md            # this file
│           ├── ProjectEverestUiParity.md
│           ├── ProjectEverestInspector.md
│           ├── ProjectEverestRenderPort.md
│           ├── EditorSceneInspection.md     # imported, still active
│           └── RendererIntegration.md       # imported, still active
├── Engine/                            # shared UI-side production code
│   ├── Interface/                     # kit, dockspace, notch, notifications, content browser, log stream
│   │   ├── InspectorCanvas.{h,cpp}          # NEW: record-facing drawing surface
│   │   ├── InspectorMetrics.h               # NEW: transcribed CSS geometry
│   │   ├── InspectorPageRegistry.{h,cpp}    # NEW: classification → default page
│   │   ├── StatusBarControls.{h,cpp}        # NEW
│   │   └── ViewportOverlayControls.{h,cpp}  # NEW: HUD, inset, notices, bring-up, failure
│   ├── Lifecycle/
│   ├── Scene/                         # RecordEntry · RecordStore · RecordToken · SceneDirectory
│   │   └── RecordInspectorProfile.{h,cpp}   # NEW: provisioning, page token, studio token, Build()
│   └── Render/                        # UI texture work only; no scene renderer
├── UI/
│   ├── Editor/
│   │   ├── ProjectBrowserPanel.*  ControlCentrePanel.*  ContentBrowserPanel.*  FoundationPanel.*
│   │   ├── EditorWorkspace.*                    # EXTEND: add viewport, inspector, status bar
│   │   ├── InspectorPanel.{h,cpp}               # NEW: crumb, identity header, category tabs
│   │   ├── InspectorPages/                      # NEW: one file per demo family
│   │   │   ├── EnvironmentInspectorPages.cpp    # Sun · sky · atmosphere · clouds · fog · wind · Moon · stars · lens
│   │   │   ├── VehicleInspectorPages.cpp        # vehicle systems · tyres · drivetrain · cabin
│   │   │   ├── EffectInspectorPages.cpp         # plume · sparks · precipitation · snow · water · fracture
│   │   │   ├── RenderInspectorPages.cpp         # GI · ReSTIR · reflection · quality · display
│   │   │   └── DiagnosticsInspectorPages.cpp    # counters · telemetry · hardware
│   │   ├── ViewportPanel.{h,cpp}                # NEW: chrome + presentation binding
│   │   ├── StatusBarPanel.{h,cpp}               # NEW
│   │   └── DedicatedRecordStudios.{h,cpp}       # NEW (from donor, renamed)
│   └── TextureStudio/                           # unchanged from branch 0
├── Frontier/                          # portable engine and projects
│   ├── Applications/Editor/{FrontierMain,ProjectBrowserHost,CommandLine,DedicatedStudioHost}
│   ├── Engine/
│   │   ├── Rendering/                 # donor renderer + the substrate split of RenderPort §2
│   │   │   ├── SceneRenderer.{h,cpp}
│   │   │   ├── GpuTraceCapability.{h,cpp}       # NEW: feature probe + tier decision
│   │   │   ├── TraceSubstrate.h                 # NEW: the interface both tiers implement
│   │   │   ├── GtxDistanceSubstrate.{h,cpp}     # NEW: SDF + surface cards + software BVH
│   │   │   ├── RtxQuerySubstrate.{h,cpp}        # NEW: BLAS/TLAS + ray query
│   │   │   └── Shaders/                         # 21 donor units + the gap units of RenderPort §1/§10
│   │   ├── Scene/  Physics/  Fluid/  Particles/  VisualEffects/  Gameplay/
│   │   ├── Networking/  GeometryProcessing/  Threading/  Foundation/  Audio/  Diagnostics/  Interface/
│   └── Projects/
│       ├── Project-Everest/                     # NEW: the ported application
│       │   ├── ProjectEverest.frontier          # TOML descriptor
│       │   ├── Content/{Scenes,Icons,Environments}/
│       │   └── Source/{EverestMain.cpp,EverestScene.cpp,VehicleSpecification.cpp,…}
│       ├── Project-Drive/                       # imported donor, kept as the reference build
│       ├── Project-VehicleHmiDemo/Contract/     # the binding worklist, unchanged
│       └── Project-ProvingGrounds/  Project-Networking/  Project-Zero/
├── EngineContent/{Icons,Defaults,MaterialArchives,PatternArchives,EnvironmentArchives,CelestialMaps}/
│   └── FontArchives/DM Sans/                    # NEW, pending decision D-E2
├── Exhibits/
│   ├── EditorMirror/                            # browser-side comparison mirror
│   ├── EverestUi/                               # NEW: parity captures + diff + results
│   ├── EverestInspector/                        # NEW: provisioning and page tests
│   ├── EverestRender/                           # NEW: tier A/B and element captures
│   └── 32 imported exhibit folders (Docking, ControlCentre, LogStream, SceneDirectory, LightCompute, …)
├── SourceImages/                                # design references (General UI, HMI, Project Browser, …)
├── Experimental/                                # prototypes; demo HTML copy at Project-VehicleHmiDemo/Reference
└── _ClaudeScratch/                              # ignored
```

📝 Folder rule from `Documentation/FolderStructure.md`: create a folder when its contents are needed, not as an
empty placeholder. This tree is the approved target, not 40 empty directories committed up front.

## 4. Phases and exit gates

| Phase | Deliverable | Exit gate (evidence) |
|-------|-------------|----------------------|
| 0 ✔️ | This plan, folder map, evidence contracts, branch ledger | Owner approves §7 decisions |
| 1 | Import branch 0 then branch 3; reconcile `CMakeLists` and `FrontierMain`; rename violations; restore dependencies | Host build of every imported validation target; per-target pass counts; imported GPU units reported compile-proved only |
| 2 | Project Everest exists: descriptor, content, scene, browser card | Project Browser enumerates five projects and opens Everest; Log Stream captures the bring-up sequence |
| 3 | **UI 1:1** — tokens, shell, Outliner, Inspector + 16 pages, viewport chrome, status bar, studios | `Exhibits/EverestUi` parity checklist with per-panel captures, diff metrics, DPI matrix; zero unexplained deltas |
| 4 | Renderer port — tier probe, GTX and RTX substrates, ReSTIR + SVGF on GPU, elements | `Exhibits/EverestRender`: tier A/B on one scene, per-element captures, barrier dumps, measured rates |
| 5 | Vehicle, tyre, fluid, fracture, particles wired into Everest | Donor validations green inside Everest; driving capture |
| 6 | Shipping audit | Shipping binary has zero editor symbols (`nm` report); every 1:1 row Ported or Approved-removal |

Phase 3 is the whole of the next working block. Phases 4–6 are planned here so their file names and seams exist
before the UI freezes them.

## 5. UI port law (applies to every panel in phase 3)

1. **Geometry is transcribed, never chosen.** Every value comes from a named CSS rule, quoted in the code comment.
   Rounding to a pleasing number is a defect.
2. **Colours and type come from tokens.** CSS custom properties become `ControlKitTokens`/`Appearance.toml`
   entries. No literal colour in a draw call.
3. **Frontier-only chrome is out of scope of the demo.** Dock splitters, dock tabs, and the Control Centre notch
   come from the base tree unchanged and are listed as *out-of-demo* rows, not deltas.
4. **A deviation is recorded or fixed.** Every row of the parity checklist ends as ✔️ ported, 🚩 approved delta
   with the owner's reason, or 🔴 open. No fourth state.
5. **Evidence is a rendered capture**, not a description, and it is regenerated by a checked-in program.

Full method and the per-panel table: `Documentation/Plans/Ongoing/ProjectEverestUiParity.md`.

## 6. Scope boundary and honest limits

| Limit | Consequence |
|-------|-------------|
| The demo's JS and WGSL were never copied, only `index.html` | Golden-image parity with the demo is impossible until demo captures exist. Native captures and analytic oracles are the evidence. |
| No GPU, no Vulkan driver, no display in the agent sandbox | Tier work is compile-proved plus CPU-reference. Real GPU and Windows claims require the owner's machine; they are reported blocked, never passing. |
| DM Sans is absent from `EngineContent/FontArchives` in every branch | Type metrics cannot be 1:1 until it is bundled. Decision D-E2. |
| WebGPU to Vulkan floating-point differences in ReSTIR/SVGF | Tolerances must be derived from the demo's own variance data, or from tier A/B, and stated per element. |
| `ProvingGrounds.gltf` does not exist | Everest starts on its own authored scene; the demo level is authored procedurally from `LevelStructure`/`CourseStructure` names only. |
| Docking has no demo counterpart | The demo is a fixed three-column grid. Everest keeps real docking; the grid geometry is preserved inside the docked layout. |

## 7. Decisions required from the owner

| # | Decision | Options (rating = risk) | Recommendation |
|---|----------|--------------------------|----------------|
| D-E1 | Does Project Everest replace Project-Drive in the browser, or sit beside it? | Sit beside ✔️ / replace 🚩 | Beside — keeps the donor buildable as a control |
| D-E2 | Bundle DM Sans 300/400/500 (OFL) into `EngineContent/FontArchives/DM Sans/`? | Bundle ✔️ / accept host-font fallback 🚩 | Bundle; the demo's whole type scale is DM Sans |
| D-E3 | Inspector substrate: retained kit canvas, or raw ImGui per page? | `InspectorCanvas` ✔️ / raw ImGui 🚩 | Canvas; keeps Shipping free of UI types and geometry in one place |
| D-E4 | Do the 16 inspector families all land before the renderer, or do renderer-bound pages follow the renderer? | UI-only pages first, renderer-bound pages bind in phase 4 ✔️ / block the UI on the renderer 🚩 | Bind-later; pages ship with lifecycle-valid controls and a named blocked state |
| D-E5 | GTX tier default when both are available? | Probe and prefer RTX ✔️ / always GTX 🚩 | Probe; show the active tier in the viewport bar |
| D-E6 | Keep Project-Drive's `Binaries/` and captures out of Git? | Ignore ✔️ / commit 🚩 | Ignore; captures belong in exhibits |

## 8. Tracker hooks

`Documentation/Tracker.md` gains one row per phase with its exhibit link. Per-agent progress reports are not
created; detailed numbers live in the exhibit `Results/` files and are linked.

| Tracker row | Exhibit | State |
|-------------|---------|-------|
| Phase 1 import | per-target host results | not started |
| Phase 2 Everest opens | `Exhibits/ProjectBrowser` + `Exhibits/LogStream` | not started |
| Phase 3 UI 1:1 | `Exhibits/EverestUi` | not started |
| Phase 4 renderer | `Exhibits/EverestRender` | not started |
| Phase 5 simulation | `Exhibits/VehiclePhysics` + `Exhibits/FluidReference` | not started |
| Phase 6 shipping audit | symbol report | not started |
