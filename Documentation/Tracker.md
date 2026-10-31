# Tracker

One tracker for the whole project. Detailed numbers live in the linked exhibit `Results/` files, never here.
Rows are added or advanced only with a matching exhibit entry.

Legend: ✔️ done · 🚧 in progress · 🔴 blocked · `—` not started

## Project Everest phases

| Phase | Deliverable | Exhibit | State |
|-------|-------------|---------|-------|
| 0 | Plan, folder map, evidence contracts, branch ledger | `Documentation/Plans/Ongoing/ProjectEverest*.md` | ✔️ awaiting owner approval of §7 decisions |
| 1 | Import base tree then donor; reconcile build; rename violations | per-target host results | — |
| 2 | Project Everest opens from the Project Browser | `Exhibits/ProjectBrowser`, `Exhibits/LogStream` | — |
| 3 | UI 1:1 — tokens, shell, Outliner, Inspector and pages, viewport chrome, status bar, studios | `Exhibits/EverestUi` | — |
| 4 | Renderer — tier probe, GTX and RTX substrates, elements | `Exhibits/EverestRender` | — |
| 5 | Vehicle, tyre, fluid, fracture, particles inside Everest | `Exhibits/VehiclePhysics`, `Exhibits/FluidReference` | — |
| 6 | Shipping audit — zero editor symbols | symbol report | — |

## Standing registers

| Register | Contents | State |
|----------|----------|-------|
| `Documentation/Records/SourceBranchLedger.md` | Four source branches, triage, reference pins | ✔️ written |
| `Documentation/Records/SyncLog.md` | Base-tree merge record, one row per commit | ✔️ standing |
| `Documentation/Records/EngineCheckpointLedger.md` | Donor engine checkpoints, verbatim | ✔️ written |
| `Docs/PortingFindings.md` (imported in phase 1) | Parity limits, product deltas, bugs found, process lessons | — import |

## Violations to clear on import

| Violation | Location | Action |
|-----------|----------|--------|
| Banned stems `Frame` and `Graph` | donor `FrameGraphExecution.{h,cpp}` | Rename to `SubmissionOrderExecution` |
| Banned stem `Entity` | donor `DedicatedEntityEditors.{h,cpp}` and two comments | Rename to `DedicatedRecordStudios`, reword comments |
| `CLAUDE.md` history grew to 48 KB | donor root `CLAUDE.md` | Split: rules here, history in `Documentation/Records/EngineCheckpointLedger.md` |
| `Binaries/` and capture frames tracked in places | donor project folders | Confirm ignored; captures belong in exhibits |
