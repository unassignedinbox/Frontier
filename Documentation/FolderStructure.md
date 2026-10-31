# Folder Structure

## Target ownership

```text
Frontier/
├── CMakeLists.txt             # one build: editor host, Project Everest, Texture Studio, validations
├── Automation/                # dependency setup, ImGui patches, packaging, base-tree sync
├── ExternalPackages/          # ignored downloaded dependency trees
├── README.md
├── AGENTS.md
├── CLAUDE.md                  # rules + Project Everest briefing
├── Documentation/
│   ├── Tracker.md             # single progress and violation tracker
│   ├── FolderStructure.md     # this map
│   ├── EnvironmentTodo.md
│   ├── Records/               # branch ledger, sync log, engine checkpoints, reference pins
│   ├── Rules/
│   └── Plans/{Ongoing,Completed,References}/
├── Engine/                    # shared UI-side production code
│   ├── Interface/             # kit, dockspace, notch, notifications, inspector canvas, overlays
│   ├── Lifecycle/             # shared activation sequence
│   ├── Scene/                 # record entry, store, token, directory, inspector profile
│   └── Render/                # UI texture work only; never the scene renderer
├── UI/
│   ├── Editor/                # lifecycle-owned production editor UI, inspector pages, viewport chrome
│   └── TextureStudio/         # texture, pattern and stamp workspaces
├── Frontier/                  # portable engine and applications
│   ├── Applications/Editor/   # Frontier host: project browser, workspace, studio host
│   ├── Engine/                # rendering, scene, physics, fluid, particles, visual effects, gameplay, …
│   └── Projects/              # Project-Everest, Project-Drive, Project-ProvingGrounds, Project-Zero, …
├── EngineContent/             # shared content archives (icons, fonts, materials, environments, celestial maps)
├── Exhibits/
│   ├── <ValidationName>/      # README, Code/, Images/, Results/
│   ├── EditorMirror/          # comparison mirror of the C++ editor UI
│   ├── EverestUi/             # UI parity captures, diff, checklist
│   ├── EverestInspector/      # record provisioning and page validation
│   └── EverestRender/         # tier A/B and per-element render evidence
├── SourceImages/              # owner-supplied design references
├── Experimental/              # prototypes and experimental HTML
├── EngineDocs/AgenticInstuctions/  # four retained authoritative skills
└── _ClaudeScratch/            # ignored logs/, build/, tmp/
```

Create meaningful folders as their contents are needed; do not add empty placeholder trees.
Use technical validation names under Exhibits. No new top-level ownership categories without approval.
Update this map after approved structural changes.

## Boundaries that changed with Project Everest

| Boundary | Before | Now |
|----------|--------|-----|
| Scene renderer | Excluded; `Engine/Render/` blank | Present, confined to `Frontier/Engine/Rendering/`; `Engine/Render/` still holds only UI texture work |
| Inspector | Absent | `UI/Editor/InspectorPanel` + `UI/Editor/InspectorPages/`, backed by `Engine/Scene/RecordInspectorProfile` |
| Records | Absent from `Documentation/` | `Documentation/Records/` holds the branch ledger, sync log, and checkpoint history |
| Comparison | `Exhibits/EditorMirror/` reserved | `Exhibits/EditorMirror/` plus the three `Everest*` evidence exhibits |
| Projects | Four | Five, with `Project-Everest` as the ported application and `Project-Drive` kept as its control build |

## Shared content and dependencies

```text
EngineContent/
├── Icons/                     # tracked original and compatibility SVGs
├── MaterialArchives/          # tracked Texture Studio material library (.material plus baked maps)
├── PatternArchives/           # tracked Texture Studio pattern presets
├── EnvironmentArchives/       # tracked baked equirect environments
├── CelestialMaps/             # lunar and sky maps with provenance notes
├── Defaults/                  # tracked blank project space
└── FontArchives/<font name>/  # ignored font downloads, OFL notices, TOML descriptors
ExternalPackages/
├── glfw/                      # platform windows and input
├── tomlpp/                    # native TOML formats
├── imgui/                     # pinned UI plus approved patches A–G
├── thorvg/                    # SVG rasterization
├── vulkan-headers/            # Vulkan and ray-query declarations
└── slang/                     # shader compiler and runtime
```

`ContentDirectory` resolves bundled content from the executable independently of the launch working directory.
The font catalogue has a native TOML form; JSON remains only automation and report interchange.

## Selective port boundary — what the import does not bring

- No inherited `Internal/`, `Executables/`, `Applications/`, or `memory/` trees. Their documents remain under
  `Documentation/Plans/References/InheritedEngine/`.
- No demo JavaScript or WGSL. Only `index.html` and the contract tables are retained, as the pinned reference.
- No third-party sources inside the repository. `ExternalPackages/` stays ignored and is restored by `Automation/`.
- No `Binaries/`, no build output, no captured frames outside `Exhibits/`.
