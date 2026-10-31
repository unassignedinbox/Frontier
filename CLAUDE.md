# Frontier — Agent Instructions

Frontier is a general-purpose, cross-platform editor with reusable engine code.
The legacy migration is complete. Do not treat this as an unfinished 1:1 migration target.

## Mandatory reading — every session

Read all four documents at session start, before naming, formatting, editing, or presenting work.
They are authoritative and override agent defaults within their scope:

- `EngineDocs/AgenticInstuctions/SKILL-Naming.md` — all naming, banned words, and name construction.
- `EngineDocs/AgenticInstuctions/SKILL-formatting.md` — source and document formatting.
- `EngineDocs/AgenticInstuctions/SKILL-Emoji.md` — the only approved emojis and permitted usage.
- `EngineDocs/AgenticInstuctions/SKILL-CompactOutputProtocol.md` — compact output; tables and diagrams over prose.

Report missing rules before affected work. Do not silently skip them or invent exceptions.
These paths remain authoritative until an approved relocation updates every reference.

## Project Everest — sources, branch ledger, standing instructions

🔴 This section is the standing brief for the VehicleHMI → Vulkan port. Read it before any work and do not
require the owner to restate it. Full detail: `Documentation/Records/SourceBranchLedger.md`.

The ported application is **Project Everest**. It is a Frontier project opened from the Project Browser, not a
separate executable. Frontier starts at the Project Browser; choosing **Project Everest** opens the workspace
(Outliner · Viewport · Inspector · Log Stream · status bar) with the ported VehicleHMI UI.

### The four source branches

| # | Repository / branch | Tip inspected | What it owns | Use as |
|---|---|---|---|---|
| 0 | `SultanAladin/Frontier` `arena/11f31835-frontier` | `8c760a9` | Rules, docking, Control Centre, Content Browser, Notifications, Project Browser, Outliner, Log Stream, Texture Studio | **Base tree (main)** |
| 1 | `streamlinkinbox/Frontier` `arena/08e7de48-frontier` | `7bc6025` | Cloud-shadowed fog, emissive-volume single scattering, facet-area reservoirs, cloud budget evidence | Reference for fog / atmosphere research |
| 2 | `streamlinkinbox/Frontier` `arena/5ec53579-frontier` | `56059b3` | Path-replay domain and submission sequence, environment showcase | Reference (thin) |
| 3 | `egoistm1inbox/Frontier` `arena/9ce908bc-frontier` | `4be51f2` | `Project-Drive`, `SceneRenderer` dual GTX/RTX tiers, 21 Slang units, Jolt physics, fluid, fracture, particles, cabin HMI, inspector seam | **Engine and renderer donor** |

- Raw locations, do not retype from memory:
  `https://github.com/SultanAladin/Frontier/tree/arena/11f31835-frontier`,
  `https://github.com/streamlinkinbox/Frontier/tree/arena/08e7de48-frontier`,
  `https://github.com/streamlinkinbox/Frontier/tree/arena/5ec53579-frontier`,
  `https://github.com/egoistm1inbox/Frontier/tree/arena/9ce908bc-frontier`.
- The port target is the VehicleHMI demo HTML/WebGPU build (pinned `SultanAladin/Frontier` @ `6afb6f0`,
  `Experimental/VehicleHmiPrototype/`). Its contract tables live in
  `Frontier/Projects/Project-VehicleHmiDemo/Contract/` — 473 files, 80 shaders, 81 entrypoints, 138 UI labels,
  138 control-inventory rows. Those tables are the binding worklist.
- 🔴 The demo's JavaScript and WGSL were never copied, only `index.html`. Never claim golden-image parity with the
  demo unless a demo capture exists. Analytic oracles and native captures are the evidence until then.

### Standing instructions

1. **UI before everything else.** No renderer, physics, or content work lands before the UI is 1:1 accepted.
2. **1:1 or nothing.** Every demo element is ported at its transcribed pixel geometry, colour, and type. No
   redesign, merge, or silent drop. A deviation is either fixed or entered in the parity checklist as an approved
   delta with the owner's reason. Frontier-only chrome (dock splitters, dock tabs, the Control Centre notch) is
   outside the demo and is reused from the base tree unchanged.
3. **Reuse, do not reinvent.** The docking system, Control Centre, Content Browser, Notifications, Project
   Browser, Outliner, Log Stream, ControlKit, and Theme structure come from the base tree as they are. Extend
   them; never fork a parallel kit.
4. **Modular inspectors.** A record registers itself in the Outliner and supplies its own inspector page from its
   constructor; its destructor withdraws both. Engine-reusable records (Sun, sky, atmosphere, clouds, fog, wind,
   Moon, stars, lens optics) ship their pages with the engine and are never re-authored by a project. Authored
   records (a vehicle, a fracture target, a plume emitter) are created and described by the project. See
   `Documentation/Plans/Ongoing/ProjectEverestInspector.md`.
5. **Two trace tiers, one library.** RTX hardware ray query and GTX distance-field/SDF with software BVH
   reflections are both first-class. Never present one tier as the other, and never silently fall back without
   saying so in the UI. See `Documentation/Plans/Ongoing/ProjectEverestRenderPort.md`.
6. **Sync after every commit.** Before reporting a commit complete, fetch the base tree
   (`arena/11f31835-frontier`) and merge it, then record the merged tip in
   `Documentation/Records/SyncLog.md`. `Automation/SyncFromMain.sh --apply` performs the merge, verifies it, and writes
   the row; `--check` only reports. A commit whose sync entry is missing is not delivered.
7. **Evidence or it did not happen.** Every delivery carries an execute-able validation and a real capture under
   `Exhibits/EverestUi/`, `Exhibits/EverestInspector/`, or `Exhibits/EverestRender/`. Plans and analysis verify
   their factual claims against inspected sources and show the report.
8. **Binaries only when asked.** The current phase delivers rendered evidence, not packaged Windows builds.

## Content, formats, and rendering

- Use **Content Directory**, `ContentDirectory`, and `EngineContent/`.
  Do not introduce `AssetDirectory` or `UI/Assets/`.
- Shared fonts belong in `EngineContent/FontArchives/<font name>/`, with the original licence beside each
  collection.
- Shared SVG content belongs in `EngineContent/Icons/`. Resolve bundled content relative to the executable first;
  launching from another working directory must not hide fonts or pick an unrelated partial collection.
- All engine-authored C++ configuration and serialization formats use **TOML**, decoded/encoded with **toml++**.
  Preserve established schemas and extensions; inspect existing codecs before creating another format.
  `Appearance.toml` stores applied theme/typography; `FontCatalogue.toml` and per-font archive TOMLs describe fonts.
  JSON lock manifests and exhibit reports are automation interchange, not new C++ runtime serialization formats.
  External file formats such as OpenType, SVG, and shaders are not converted into TOML.
- **Vulkan is the primary rendering API. Slang (`.slang`) is the primary shader language.**
  DX11 is permitted for the isolated ImGui display exhibit; it is not a replacement engine-renderer architecture.
  The rule "keep the scene renderer excluded" governed the earlier UI-only port; Project Everest supersedes it for
  its own scope, because porting the VehicleHMI renderer is the task. The imported renderer is confined to
  `Frontier/Engine/Rendering/` and never enters `Engine/`.
- **GLFW owns platform windows and input.** Use a no-OpenGL-context window for Vulkan or the DX11 ImGui exhibit.
- Third-party sources belong under the existing ignored `ExternalPackages/`, restored from pinned dependencies.
  Reuse the existing toml++, GLFW, ImGui, and ThorVG dependency setup; do not vendor duplicate copies elsewhere.
- The Control Centre is a top-edge drawer, not a separate floating settings window. Retain the reference's
  trapezoid, travelling sheet, bottom grip, axis-locked drag, and upward swipe/flick dismissal.
  Typography sliders and scroll content retain their pointer ownership; drag the header, quick-card blank space,
  notch, or bottom grip to carry the sheet. GI and renderer settings remain excluded from it.

## Work and verification

Complete the requested work, run relevant local verification, then deliver results with evidence.
Local builds, tests, focused validation programs, and actual application captures need no separate approval.
Never deliver an implementation as complete without testing it.

| Work                  | Required verification                                             |
|-----------------------|-------------------------------------------------------------------|
| C++ or shaders        | Relevant build and tests; runtime validation where applicable.     |
| UI                    | Relevant tests and actual rendered or interaction evidence.        |
| Behaviour repairs     | Reproduce the defect and verify the repaired behaviour.            |
| Performance claims    | Reproducible measurements, conditions, and comparison results.     |
| Documentation         | Paths, links, structure, consistency, and applicable rule checks.   |
| Plans and analysis    | Verify factual claims against inspected sources; show a report.     |

- Exercise the changed behaviour, not merely unrelated passing tests.
- Add focused validation when existing coverage is insufficient.
- Compilation alone does not establish runtime correctness.
- Progress messages must not imply unfinished work passed verification.
- When blocked, report the command, observed result, missing prerequisites, and remaining checks.
- Mark unavailable checks blocked or not run, never passing. A blocked report is not completed implementation.

## Visible evidence — required with delivery

Every delivery must include an actual image, graph, results table, or concise verification report.
UI appearance claims require actual rendered evidence when capture is supported.
Mockups, generated illustrations, and invented measurements are not validation evidence.
A browser prototype does not prove that the production C++ editor renders the same result.

Retain evidence and its generating code together under `Exhibits/`.
Include reproduction commands, prerequisites, platform, compiler, configuration, tested revision,
and any relevant uncommitted changes. Explain limitations and present the evidence, not just its location.

## Folder ownership and documentation

Follow `Documentation/FolderStructure.md`. Do not invent alternative top-level folders.

| Location                         | Responsibility                                        |
|----------------------------------|-------------------------------------------------------|
| `Engine/`                        | Reusable production code shared across applications.  |
| `UI/Editor/`                     | Production editor UI.                                 |
| `Exhibits/`                      | Validation code, retained results, and proof images.   |
| `Exhibits/EditorMirror/`          | Comparison mirror of the C++ editor UI.                |
| `Experimental/`                  | Prototypes, experimental HTML, and related assets.     |
| `Documentation/Tracker.md`       | Single project progress and violation tracker.        |
| `Documentation/Records/`         | Source branch ledger, engine checkpoint history, pins. |
| `Documentation/Rules/`           | Project rules; four retained skills remain above.     |
| `Documentation/Plans/Ongoing/`   | Approved active plans.                                |
| `Documentation/Plans/Completed/` | Completed plans, outcomes, and evidence links.         |
| `_ClaudeScratch/`                | Disposable local work; ignored and never committed.    |

Do not put prototypes, captures, reports, or validation-only code in `Engine/`.
Exhibits must reuse production engine code, not independent copies of it.
Keep requested prototype source, assets, and launchers together in `Experimental/`.
Do not delete inherited build, dependency, content, or application folders without an approved migration.
Update the folder map and affected references whenever approved moves change paths.
Maintain one tracker, not per-agent progress reports or competing backlogs.
Keep detailed results in exhibits and link them from the tracker.
Do not silently weaken rules to make a scan pass.

## Scratchpad

Use `_ClaudeScratch/logs/`, `_ClaudeScratch/build/`, and `_ClaudeScratch/tmp/` for disposable material.
Never commit scratch contents or leave the only copy of a deliverable or retained evidence there.
Permanent prototypes go in `Experimental/`; retained validation code and results go in `Exhibits/`.
Respect existing ignored build-output conventions when tools require their own directories.
Do not leave generated output next to source or loose at the root.
Incomplete HTML fragments use a non-`.html` extension such as `.part`.

## Platforms, binaries, and tooling

The user is on Windows. Deliver Windows binaries by default when a binary is requested or part of the task.
Do not build, package, or deliver Linux binaries unless requested.
Never substitute a Linux executable for a Windows deliverable.
If Windows compilation is unavailable, report the blocker; do not claim a Windows binary was verified.

Preserve Windows, Linux, and macOS portability. Android is a future consideration; no Android app is in scope.
Do not download macOS binaries or set up macOS during current work.
MSVC is the user's current compiler, not the only permitted compiler.
GCC/g++, MinGW-w64, or Clang may be used where the repository supports them.
Distinguish locally tested configurations from untested portability expectations.

Use actual repository build instructions and available tooling; do not hard-code developer installation paths.
Isolate platform-specific code from reusable code. Avoid unnecessary Windows-only assumptions.
Use the appropriate host interpreter: Windows command tooling for `.bat`, PowerShell for scripts requiring it,
and Bash for appropriate Linux/macOS commands. Python need not live at a fixed path.

## GitHub Actions — explicit approval required

Always ask before causing a GitHub Actions run unless the user explicitly authorised that run.
This includes dispatches, reruns, and remote operations such as pushes or pull requests that trigger workflows.
Inspect triggers before an authorised remote operation. Never use Actions as an automatic local-test fallback.
Permission to edit or commit locally is not permission to run Actions.
Follow the user's current commit and push instructions; do not infer permission to commit or push.
The owner's standing direction for this branch is to commit and push to
`arena/580ab89f-frontier` on `unassignedinbox/Frontier` after each delivery.

## Delivery

Follow the compact output skill: short outcome, verification table, visible evidence, exhibit paths,
blockers, untested configurations, and remaining work. Do not substitute confidence or compilation for testing.

### Recovered native Float32 surface maps (2026-10-19)

`VectorSurfaceCompute` writes typed RGBA32F channel maps and separate per-channel contribution masks; coverage/selection
stay RGBA8. `VectorCoverageRasterization` derives the actual storage format from SPIR-V and refuses unsupported Float32
capabilities instead of silently quantizing. The shared PFM writer preserves float bits and emits little-endian,
bottom-first scalar/RGB output. Seven GPU channel assertions pass strict and sanitizer builds; the archive contains
14 actual 32x32 GPU fixture maps/masks with separately authored gold thread and fabric metalness. See
`Exhibits/TextureStudio/NativeSurfaceEvidence.html` and `NativeMaterialApplication.md`.

The retired-slot cache path dispatches and samples the 512×512 Float32 surface output. Texture Workspace now toggles
to the native Pattern panel, persists vector TOML, and submits GPU coverage/selection. Its C++ runner passes 606 checks;
the recorded evidence explicitly does not represent a Windows desktop session. Pattern Float32 outputs remain
unconnected to the PBR material renderer; native 3D application, physical height calibration, user-document export and
Pattern 1:1 parity remain open. GIF previews are 8-bit evidence, not precision checks. Scope remains approximately 68%.
