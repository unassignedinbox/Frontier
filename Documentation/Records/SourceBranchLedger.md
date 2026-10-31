# Source branch ledger

Purpose: the four branches this tree is assembled from, what each one owns, the exact revision inspected, and the
sync log that must gain a row after every commit. Do not restate this from memory; read it.

Inspection date: 2026-10-31. Method: shallow clone of each branch (`--filter=blob:none --depth 1`), file-tree
enumeration, targeted reads of `CLAUDE.md`, `Docs/`, `Documentation/Plans/`, `Frontier/Engine/`, `UI/`, and
`Exhibits/`. Line and file counts below are measured from those clones, not estimated.

## The four branches

| # | Repository / branch | Tip | Files | Tree size | Owns | Decision |
|---|---------------------|-----|-------|-----------|------|----------|
| 0 | `SultanAladin/Frontier` `arena/11f31835-frontier` | `8c760a9` "Add native SDF cutting-plane previews" | 3,743 | 485 MB | Rules and skills; `PanelDockspace` **with** the dock-graph cycle guard and layout-restore ordering fix; `ControlCentrePanel` **with** drawer focus and pointer-ownership fixes; Content Browser; Notifications; Project Browser; Scene Directory Outliner; Log Stream; Texture Studio (baking, paint-stamp, texture-paint, panel chrome, stamp library, typeface catalogue) | **Base tree** |
| 1 | `streamlinkinbox/Frontier` `arena/08e7de48-frontier` | `7bc6025` "Capture exhibit-only ControlVehicle ReSTIR lighting studies" | 5,307 | 787 MB | Cloud-shadowed fog and light shafts, emissive-volume single scattering, facet-area sampling (CDF, proposals, reservoir, score, transport), fog-quadrature and cloud-budget evidence | Reference: fog and atmosphere research |
| 2 | `streamlinkinbox/Frontier` `arena/5ec53579-frontier` | `56059b3` "Add ControlVehicle environment render showcase" | 3,146 | 148 MB | Path-replay domain and submission sequence, environment showcase, `SceneInspectorPanel` | Reference: thin, superseded by 1 and 3 |
| 3 | `egoistm1inbox/Frontier` `arena/9ce908bc-frontier` | `4be51f2` "Scope MSVC /FIcstdint to CXX targets…" | 3,511 | 199 MB | `Project-Drive` unified showcase; `SceneRenderer` (3,681 lines) with dual GTX/RTX tiers; 21 Slang units; Jolt 240 Hz; GRIT tyres; `PlumeFluid`; `SolidFracture`; `SparkParticles`; `SkyPrecipitation`; `SnowSurface`; portals; mirrors; `VehicleCabinHmi`; 25 `RecordEntry` components; `RecordEditorDescription` + `SceneInspectorPanel` + `EnvironmentRecords`; 32 exhibit folders | **Engine and renderer donor** |

Raw locations, verbatim:

```text
https://github.com/SultanAladin/Frontier/tree/arena/11f31835-frontier
https://github.com/streamlinkinbox/Frontier/tree/arena/08e7de48-frontier
https://github.com/streamlinkinbox/Frontier/tree/arena/5ec53579-frontier
https://github.com/egoistm1inbox/Frontier/tree/arena/9ce908bc-frontier
```

## Why the base is branch 0 and not branch 3

Branch 3 is far ahead on the engine and only marginally behind on the UI. The reverse is decisive for the current
task, which is the UI:

| Compared path | Branch 0 (base) | Branch 3 (donor) | Verdict |
|---|---|---|---|
| `Engine/Interface/PanelDockspace.cpp` | Recursion guarded: depth cap plus visited-set refusal on the live dock graph every frame; `RestoreLayout` runs on a clean dockspace shell before defaults | Unbounded recursion over the live dock graph; layout restored after `DockBuilderFinish` | Branch 0 |
| `UI/Editor/ControlCentrePanel.cpp` | Drawer focus hand-off, underlying-item engagement test, notch pointer arbitration, FPS overlay clearance | Earlier variant without those | Branch 0 |
| `UI/TextureStudio/` | Adds `BakingWorkspacePanel`, `PaintStampEngine`, `PaintStampTools`, `PanelChrome`, `StampLibrary`, `TexturePaintWorkspacePanel`, `TypefaceCatalogue`, `WorkspaceViewportDrawList` | Subset | Branch 0 |
| `Frontier/Engine/Rendering/` | 44 files + 2 Slang units, CPU reference cores | 83 files + 21 Slang units, `SceneRenderer`, `PathReplayRayQuery`, `PathReplayVulkan` | Branch 3 |
| `Frontier/Projects/Project-Drive/` | Absent | 11,527 lines across 16 files | Branch 3 |
| `Frontier/Engine/{Audio,Fluid,Gameplay,Networking,Particles,Physics,GeometryProcessing,VisualEffects}` | Absent | Present | Branch 3 |
| `Frontier/Engine/Scene/` | 9 files | 13 files, adds `RecordEditorDescription`, `EnvironmentRecords` | Branch 3 |
| `Exhibits/` | 20 exhibit folders | 32 exhibit folders | Branch 3 |

Branch 3's engine content is additive to branch 0's tree; branch 0's UI fixes are small and would have to be
reverse-engineered if the merge ran the other way. Base 0 absorbs 3 with mostly new files.

## Naming violations found in the donor (fix on import, do not propagate)

| Donor path | Banned token | Approved replacement |
|---|---|---|
| `Frontier/Engine/Rendering/FrameGraphExecution.{h,cpp}` | `Frame`, `Graph` | `SubmissionOrderExecution` |
| `Frontier/Projects/Project-Drive/Source/DedicatedEntityEditors.{h,cpp}` | `Entity` | `DedicatedRecordStudios` |
| `RecordEditorDescription.h` comment "Pinned HTML entity id" | `Entity` | "Pinned HTML record id" |
| `DriveShowcaseRecords.h` comment "per-entity" | `Entity` | "per-record" |

Renames land with the same turn as the import so the tree never carries a fresh violation.

## Sync after every commit

🔴 Fetch and merge branch 0 after every commit, before reporting it delivered, and append the row to
`Documentation/Records/SyncLog.md`. A commit with no sync row has not been delivered.

```bash
Automation/SyncFromMain.sh --check   # is the base tree already contained in HEAD?
Automation/SyncFromMain.sh --apply   # fetch, merge, append the sync row
```

The script refuses to merge onto a dirty tree, so a sync never folds unrelated work into a merge commit.
Manual equivalent:

```bash
git fetch https://github.com/SultanAladin/Frontier.git arena/11f31835-frontier
git merge --no-edit FETCH_HEAD
```

⚠️ The first sync merges an unrelated history. Use `git merge --allow-unrelated-histories --no-edit FETCH_HEAD`
until the base tree is present.

## Reference pins carried forward

| Reference | Pin | Governs |
|---|---|---|
| VehicleHMI demo HTML | `SultanAladin/Frontier` @ `6afb6f00b5f25c45f7cda92c7da65b64845768f3`, `Experimental/VehicleHmiPrototype/` | The renderer and UI being ported; contract tables in `Frontier/Projects/Project-VehicleHmiDemo/Contract/` |
| Contract extraction | 473 files, 80 shaders, 81 entrypoints, 138 UI labels, 138 control-inventory rows | The 1:1 worklist; re-runnable by `Contract/ExtractContract.mjs` |
| Previous project (Windows) | `SultanAladin/Frontier-` @ `arena/9440f4d0-frontier`, inspected `3382938` | OpenPBR lobe library, automotive flake paint, physical dynamics |
| Materials and flakes | `SultanAladin/Frontier` @ `arena/5e581235-frontier`, audit pin `be1d0a8` | Material archives and paint shaders; glints/flakes to be rewritten |
| Physics reference | Previous project `Source/{TyreSlipSolver,VehicleSolver,VehicleSpecification}.js` | Pacejka MF6.1; port gate is the owner's call (donor already carries a native stack) |
| ImGui | Patched fork `3bae66c` plus `Automation/Patches` A–G, applied in order | Dock tab trapezoid behaviour; stock ImGui lacks it |
