# Frontier

A component-driven world editor, ported into this repository from the legacy trees.

Nothing in `Experimental/` is borrowed from Unreal, Unity, Godot, or any other engine: the vocabulary and the
UI are the project's own, and every new name is proposed and approved before it is used.

---

## Start here

| File | Why |
| ------------------------------------------------------ | --------------------------------------------------------------- |
| `CLAUDE.md` | standing instructions for this checkout — read first |
| `AGENTS.md` | the same instructions for agents that look for this name |
| `Documentation/UI_REDESIGN_WORKFLOW.md` | sources, pinned commits, servers, render recipe, approval gates |
| `Documentation/NAMING_LEDGER.md` | proposed names, what they replace, and their approval state |
| `Documentation/Prototypes/README.md` | the card prototypes and the build order |
| `SYSTEM_NOMENCLATURE.md` | the project's own vocabulary registers |
| `EngineDocs/AgenticInstuctions/` | naming, formatting, emoji, and output skills — authoritative |

---

## Run the two viewers

```sh
python3 Experimental/ComponentViewer/Serve.py 8095              # Component Viewer
python3 Experimental/ComponentViewer/ServeAssetViewer.py 8096   # Asset Viewer
```

Both serve from the repository root and bind `0.0.0.0`. No dependencies.

---

## Look at a card prototype

Each folder under `Documentation/Prototypes/` opens on its own:

```sh
xdg-open Documentation/Prototypes/PlacementFrame/index.html
```

They use only bundled fonts and local scripts, so `file://` is enough.

---

## Layout

```
Experimental/            ported subsystems; ComponentViewer serves the two live viewers
EngineDocs/              folder map, backlog, and the agent skills
Documentation/
    Baseline/            rendered screenshots of the live viewers (the audit record)
    Prototypes/          one standalone HTML card per proposed replacement
    NAMING_LEDGER.md     name proposals and their approval state
    UI_REDESIGN_WORKFLOW.md
SourceImages/General UI/ the design reference set
```
