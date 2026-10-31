# Frontier

Frontier is a general-purpose, cross-platform editor with reusable engine code. It starts at a Project Browser;
choosing a project opens its workspace.

**Project Everest** is the project this branch is building: the VehicleHMI demo (ReSTIR GI and ray tracing, HTML and
WebGPU) ported to native C++ on Vulkan with Slang shaders, with two trace tiers — hardware ray query for RTX cards and
distance-field GI with software BVH reflections for GTX cards. The UI is ported first and 1:1.

## Start here

| Document | Purpose |
|----------|---------|
| `CLAUDE.md` | Rules, the Project Everest briefing, and the standing instructions |
| `Documentation/Plans/Ongoing/ProjectEverest.md` | Master plan, import strategy, folder map, phases, decisions |
| `Documentation/Plans/Ongoing/ProjectEverestUiParity.md` | The 1:1 UI port method and inventory |
| `Documentation/Plans/Ongoing/ProjectEverestInspector.md` | Modular inspector model |
| `Documentation/Plans/Ongoing/ProjectEverestRenderPort.md` | GTX and RTX renderer port |
| `Documentation/Records/SourceBranchLedger.md` | The four source branches and what each owns |
| `Documentation/Tracker.md` | Single progress and violation tracker |

## State

Plan only. No source has been imported and no binary exists on this branch yet. Phase 1 imports the base tree and the
engine donor; phase 3 is the UI port.

## Building

Not yet applicable on this branch. Once phase 1 lands, dependencies are restored with `Automation/PrepareUI.py`,
`Automation/StageThorVG.py`, and `Automation/PrepareFonts.py`, then configured with CMake. Windows binaries are the
default deliverable; none is produced until the owner asks.
