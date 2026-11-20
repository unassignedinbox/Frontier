# Frontier — Agent Instructions

Read this file in full at the start of every session, before naming, formatting, editing, or presenting work.

This file replaces every earlier `CLAUDE.md` in this repository. There is one ruleset and this is it.
`AGENTS.md` is a pointer to this file and must never restate a rule.

**Verified against `SultanAladin/Frontier` `arena/8391e994-frontier` (`940a125`), 2026-11-20.**
Every path in this file resolves on that commit, and `python3 Automation/AuditCheck.py` proves it.
If a cited path stops resolving, the file is wrong. Fix the path; do not work around it.

---

## 0. What this repository is, right now

A cross-platform ISO C++20 engine with a Vulkan 1.3 renderer. The renderer is **direct lighting only**:
`WorldRenderEngine` has exactly two modes, `MatCap` and `OpenPbrDirect`.

Several subsystems were deliberately deleted because they shipped as simplifications rather than real
implementations. They are being rebuilt properly. **They do not exist, and nothing may pretend they do.**

| System | State | Archive |
| :--- | :--- | :--- |
| Fluid | deleted | none — rebuild from scratch |
| Particle (`Engine/ParticleSpace/`) | deleted | `Experimental/RemovedSdfGi/Source/Engine/ParticleSpace/` (FCT1 loader only) |
| Vector Field | deleted | none — rebuild from scratch |
| Force Field | deleted | none — rebuild from scratch |
| GI / SDF (`Engine/DistanceFieldSpace/`, GI render modes, GI shaders) | deleted by `63db120` | `Experimental/RemovedSdfGi/` — byte-exact source plus every proof image |

Read `Experimental/RemovedSdfGi/README.md` before any SDF or GI work. It records what was removed, by which
commit, why the removed exhibits do not compile against today's engine, and the two viable re-integration
routes.

**Nothing here authorises a rebuild.** The owner says when a system comes back. Until then a deleted system is
not designed, not scaffolded, not planned, and not named as if it existed.

---

## 1. Mandatory session reading

Read these seven before writing code, shaders, documentation, or a plan, in this order.

| # | Document | Scope |
| :--- | :--- | :--- |
| ① | `Documentation/AgenticInstructions/SKILL-CompactOutputProtocol.md` | Output shape: table first, prose last |
| ② | `Documentation/AgenticInstructions/SKILL-Emoji.md` | The only approved emojis |
| ③ | `Documentation/AgenticInstructions/SKILL-Naming.md` | Mechanism-first naming, 7-gate reject test |
| ④ | `Documentation/AgenticInstructions/SKILL-Formatting.md` | Banners, rulers, Allman braces, unit tags |
| ⑤ | `Documentation/BannedWordsVocabulary.md` | The blacklist |
| ⑥ | `Documentation/CodingStandards.md` | C++20 baseline, explicit prototypes, concepts |
| ⑦ | `Documentation/ImportantNotes.md` | Hard invariants |

`Documentation/FolderStructure.md` is a **reference to look up**, not a document to read end to end. Consult it
before creating a folder or a module. It is 68 KB; re-reading it every session buys nothing.

`Documentation/Tracker.md` and `Documentation/Backlog.md` are status files. Read the top of `Tracker.md`,
never the whole file.

**Reference index — open only when the task touches that area:**

| Area | Document |
| :--- | :--- |
| All host workspaces, UI controls, naming audit | `Engine/Workspaces/README.md` |
| Application lifecycles and desktop launchers | `Engine/Lifecycle/README.md` |
| Editor Explorer, `ProjectBrowser`, `ContentBrowser` | `Engine/Editor/README.md` |
| `UVWorkspace` | `Engine/Workspaces/UV/README.md` |
| `Pattern` · `Stamp` · `Baking` | `Engine/Workspaces/Texturing/README.md` |
| `FractureWorkspace` | `Engine/Workspaces/Fracture/README.md` |
| `ParametricSketcher` and the B-rep kernel | `Engine/Workspaces/ParametricSketcher/README.md` · `Engine/GeometrySpace/Parametric/README.md` |
| `MaterialWorkspace` | `Engine/Workspaces/Material/README.md` |
| `ComponentEditor` | `Engine/Workspaces/ComponentEditor/README.md` |
| Non-linear revision DAG | `Engine/Workspaces/Revision/README.md` |
| HTML reference studios | `Experimental/README.md` |
| UI component contract | `Experimental/UI_COMPONENT_GUIDELINES.md` |
| Retired SDF / GI work | `Experimental/RemovedSdfGi/README.md` |

Do not cite a document that is not on this list or otherwise verified to exist.
`Engine/Workspaces/Fluid/README.md`, `Engine/Workspaces/Particle/README.md`, and
`Exhibits/DeterministicPathTracer/README.md` were removed with their systems. They are not references.

---

## 2. HTML is the ground truth for UI

The HTML studios in `Experimental/` are the design. The C++ is a conversion of them. **The C++ must look like
the HTML, not like what an agent assumes the HTML looked like.**

The failure this rule exists to stop: a workspace was converted from HTML to C++ and came back with a different
UI *and* simplified sprite particles instead of the HTML's particle output. The HTML was available the whole
time and was never opened.

### 2.1 The procedure, in order

1. **Locate the HTML ground truth first.** Name the exact directory under `Experimental/` in your first
   message. If it is not in this repository, say which repository and branch carries it, and stop before
   writing any C++.
2. **Capture it before writing any C++.** Serve it with `python3 Experimental/Serve.py`. Capture every state
   you intend to reproduce into `Exhibits/Images/<Domain>/HtmlGroundTruth/`. These are committed evidence.
3. **Read the HTML's own numbers.** Panel widths, tile radii, gaps, font sizes, colours, and animation timings
   come from its CSS. Never estimate them.
4. **Convert using the existing kit.** Compose out of `PixelSpace`, `WorkspaceDockContainer`,
   `ControlCentrePanel`, `ContentBrowser`, `OutlinerPanel`, `ViewportPanel`, `InspectorPanel`, `CapsuleButton`,
   `ExclusiveSelector`, `DropdownSelector`, `ScalarSlider`, `TextInputField`, `ConfirmationDialogue`,
   `ThemeRepository`, `TypographyRepository`. Never invent an ad-hoc `ImDrawList` layout. Never ship unstyled
   `ImGui::*` substitutes.
5. **Port the behaviour, not a description of it.** If the HTML renders its presets from a large `presets.js`,
   the C++ implements those presets from that file. A simplified sprite particle is not a port. Fewer presets,
   simpler shading, or fewer controls is a failed conversion, not a first pass.
6. **Capture the C++ result and place it beside the HTML capture.** Same state, same viewport size, written
   into `Exhibits/Images/<Domain>/` and linked as a pair in the report.
7. **State the delta honestly.** Unmatched pixels, unmatched controls, unmatched frame timing. A conversion
   with a documented delta is reviewable. A conversion reported as "1:1" with no capture is a lie.

### 2.2 Where screenshots come from — stated once, so it stops being an excuse

Two environments exist. Know which one you are in before claiming a capture.

| Environment | HTML capture | C++ capture |
| :--- | :--- | :--- |
| **Owner's Windows machine**, Chrome present, executable built | `msedge --headless=new --screenshot=out.png --window-size=1600,1000 file:///<absolute path>/index.html`, or serve and use a real browser | Run the executable with `--frames=4 --capture-png=<path>` |
| **Sandboxed agent environment**, no browser, no GPU | **BLOCKED.** No browser binary is preinstalled and the two download hosts are unreachable | **BLOCKED** unless an offscreen Vulkan or software rasterizer path exists |

What the blockage actually prints, so it is quoted rather than invented around:

```
$ npx puppeteer browsers install chrome
Error: All providers failed for chrome-headless-shell 157.0.8103.78:
  - DefaultProvider: Client network socket disconnected before secure TLS connection was established

$ curl -o /dev/null -w "%{http_code}" https://storage.googleapis.com/   -> 000
$ curl -o /dev/null -w "%{http_code}" https://edgedl.me.gvt1.com/      -> 000
```

`npm install puppeteer` succeeds. The browser binary it needs does not download. Therefore:

- **Never write a jsdom or stubbed-canvas harness and present it as a visual proof.** It is a DOM test.
  Label it a DOM test.
- **Never claim a screenshot you did not take.** If capture was impossible, the delivery is
  **BLOCKED — capture unavailable**, with the command and the output above.
- **Do not let a blocked capture stop the C++ work.** Write the conversion, build it, run its tests, and state
  that the visual comparison is owed by the owner's environment. That is a legitimate delivery.
- Hand the owner the exact command that will produce the capture where a browser exists.

### 2.3 What counts as evidence

| Evidence | Counts | Does not count |
| :--- | :--- | :--- |
| HTML capture from a real browser (§2.2) | ✔ | A description of the HTML |
| C++ capture from the executable | ✔ | A compile succeeding |
| Side-by-side composite of the pair | ✔ | A hand-drawn mockup |
| A jsdom / stubbed-canvas harness, labelled a DOM test | ✔ as a DOM test only | Presenting it as a visual proof |
| Frame timings with the capture conditions stated | ✔ | An estimate |
| "It looks right", "the layout matches" | ✘ | Anything you did not open and look at |

---

## 3. Work and verification

Complete the work, run the verification, then deliver with evidence.

| Work | Required before you may call it done |
| :--- | :--- |
| C++ or `.slang` | Relevant build plus tests. Runtime validation where the change is observable. |
| UI | HTML ground-truth capture (§2) plus a C++ capture plus the comparison |
| Behaviour repair | Reproduce the defect, then verify the repair. Both captured. |
| Rendering claim | A captured image, plus the command and settings that produced it |
| Performance claim | Reproducible measurement, stated conditions, comparison |
| Documentation | Every path resolves, every link opens |
| Plan or analysis | Every factual claim verified against an inspected source, with the source stated |

- Exercise the changed behaviour. Passing unrelated tests is not verification.
- Compilation is not correctness. A green build proves the code parses.
- Progress messages must never imply that unverified work passed.
- When blocked, report the command, its output, the missing prerequisite, and what remains unrun.
- A blocked report is a legitimate delivery. An invented pass is not.

---

## 4. Vocabulary — never reintroduce a deleted name

`Documentation/BannedWordsVocabulary.md` is the authority. This is the part that gets violated.

- **Never name a folder, file, type, or plan after a deleted system as if it exists.** No `ParticleSpace`,
  no `FluidSpace`, no `VectorFieldSpace`, no `ForceFieldSpace`, no `DistanceFieldSpace` directory anywhere
  under `Engine/`, and no fluid or particle workspace beside the registered workspaces. When a system is
  rebuilt the owner names it first.
- **Never cite a superseded plan as live work.** The four plans listed below describe deleted systems and
  reference HTML prototypes that are not in this repository. Read them for history only:
  `Documentation/Plans/ParticleSpaceModularPortPlan.md`,
  `Documentation/Plans/ParticleRendererDerezMaterialTexturePlan.md`,
  `Documentation/Plans/VectorFieldArchitecturePlan.md`,
  `Documentation/Plans/VectorFieldVulkanConversionPlan.md`.
- **Hard-banned words** include `Cache`, `Widget`, `Kit`, `Hierarchy`, `Node`, `Mesh`, `Actor`, `Beacon`,
  `Pipeline`, `Subsystem`, `Manager`, `Handler`, `Coordinator`, `Utility`, `Data`, `Info`, `Element`, `Item`,
  `System`. `Model` is permitted only inside the Force/Turbulence exemption that
  `BannedWordsVocabulary.md` names, and nowhere else. The conjunction `And` is barred inside every identifier,
  filename, and plan title.
- Use `Space` for a domain's world. Use `…Extension` only for the lifecycles `SKILL-Naming.md` lists. Use
  `<Domain>Sequence` for an ordered GPU chain and `…Rasterization` for a GPU overlay unit.

---

## 5. Architectural mandates

### 5.1 GPU overlays
Every viewport overlay — grids, gizmos, debug wireframes, callouts — is drawn on the GPU by a `.slang`
`…Rasterization` unit. Never on the CPU. Never through ImGui `ImDrawList`. Route GPU execution through
`Engine/GeometricRaster/` (`RenderDeviceInterface`, `IndirectDrawEncoder`).

ImGui is for docked panels, inspectors, tool ribbons, and chrome. That is all.

### 5.2 Shaders
All shaders in `Engine/Shaders/` are `.slang`, compiled by `slangc` via `Automation/PrepareShaderToolchain.py`.
No `.glsl`, `.hlsl`, or `.wgsl` under `Engine/Shaders/`.

A shader is not wired until its `frontier_compile_slang_shader(...)` line exists in `CMakeLists.txt` and the
build produces its SPIR-V.

### 5.3 Portability
Windows (Win32 x64 and ARM64), Linux (X11 and Wayland), Android (NDK / `ANativeWindow`), and headless
(`PlatformWindowStub`). OS headers stay inside the `PlatformWindow*` and platform device `.cpp` files.

Provide a portable scalar path plus ARM64 NEON and x86_64 SSE4.2. Never lock math or physics to x86-only
intrinsics. State plainly which platform you actually built on.

### 5.4 Build profiles
`-DFRONTIER_PROFILE=Editor` sets `FRONTIER_PROFILE_EDITOR=1` and `FRONTIER_EDITOR_ENABLED=1`: the full editor.
`Development`: assert-live runtime, `Engine/Editor/` excluded by profile macro.
`Shipping`: release, assertions stripped, `Engine/Editor/` excluded, packaged binary.

Never widen a profile gate to make a build pass.

### 5.5 Dependencies
Third-party packages live in git-ignored `ExternalPackages/`, staged and SHA-256 verified by
`python3 Automation/PrepareDependencies.py`. Never vendor a second copy anywhere.

`ExternalPackages/imgui` requires all eight patches `PatchA` to `PatchH` applied in order via
`Automation/ApplyImGuiPatches.py`, compiled with `ImDrawIdx=unsigned int`.

### 5.6 Reuse before creating
Before creating any file, class, shader, panel, or helper, search `Engine/`, `EngineContent/`, `Exhibits/`,
`Automation/`, and `Documentation/FolderStructure.md` for the capability. Extend what exists. Create only when
the capability is genuinely absent and the new responsibility does not overlap an existing unit.

### 5.7 Serialization
Engine-authored C++ configuration and serialization use TOML via `toml++`. Container files use the `*space`
family (`.projectspace`, `.levelspace`, `.worldspace`, `.sdfspace`) and never a compound `.toml` suffix.
Preserve established schemas and extensions. External formats — OpenType, SVG, shaders, glTF, OBJ, FCT1 — are
not converted to TOML.

---

## 6. Where work lives

| Location | Responsibility |
| :--- | :--- |
| `Engine/` | Reusable production code |
| `Engine/Editor/` | Editor-only code, excluded from `Development` and `Shipping` |
| `EngineContent/` | Fonts, icons, MatCaps, skies, geometry, shader inputs |
| `Exhibits/` | Validation executables and their retained results |
| `Exhibits/Images/<Domain>/` | Proof images, including HTML ground-truth captures |
| `Experimental/` | HTML/JS reference studios and prototypes. HTML/JS only — no engine C++ |
| `Experimental/RemovedSdfGi/` | Byte-exact archive of the retired SDF/GI work. Read-only |
| `Documentation/AgenticInstructions/` | The four skill documents |
| `Documentation/Plans/` | Plans and reports |
| `Automation/` | Dependency staging, shader toolchain, ImGui patches, compliance check, Markdown Studio build |

- No engine C++ in `Experimental/`. No prototypes, captures, or validation-only code in `Engine/`.
  A dependency-free standalone demo that links nothing into the engine is the one exception, and it must be
  listed in this file before it is created.
- Exhibits reuse production engine code. They do not keep private copies of it.
- Never delete the retired SDF/GI archive under `Experimental/`, its evidence folder, or its restore script.
  That archive is the only record of the removed work.
- One tracker: `Documentation/Tracker.md`. No per-session progress files, no competing backlogs.
- Do not delete an inherited folder without the owner saying so in chat, and do not restore deleted code from
  an upstream snapshot without asking first (§8).
- Update `Documentation/FolderStructure.md` in the same commit as an approved move that changes the tree.

---

## 7. Output and delivery

Follow `SKILL-CompactOutputProtocol.md`. Table first, prose last.

- Write plans, architecture decisions, and turn reports into `Documentation/Plans/*.md`, then rebuild the
  viewer with `python3 Automation/BuildMarkdownViewer.py`.
- Keep a chat reply to a few lines naming the deliverable path and its verification status. Never paste a plan
  into chat.
- Before asking the owner to choose, put the comparison table first. Never ask in prose a question a table
  answers.
- Approved emoji only, per `SKILL-Emoji.md`.
- GitHub Actions: **ask first, every time.** Do not infer approval to commit or push. If the owner has already
  instructed committing, commit and state exactly what was committed.

### Delivery checklist

| # | Item |
| :--- | :--- |
| 1 | The requested work is complete — not a subset, not a simplification |
| 2 | Relevant build and tests pass, or the failure is reported verbatim |
| 3 | Each changed behaviour was exercised |
| 4 | UI work has an HTML capture, a C++ capture, and the comparison — or a stated BLOCKED capture |
| 5 | Evidence is committed under `Exhibits/` beside the code that produced it |
| 6 | Reproduction command, platform, profile, and tested commit are stated |
| 7 | Unrun checks are marked BLOCKED, never PASS |
| 8 | No path in this file or in the report is broken |
| 9 | `python3 Automation/AuditCheck.py` passes, or every FAIL is explained in the report |

---

## 8. Rules that exist because they were broken

Each of these cost days. They are not suggestions.

| Rule | What happened, and how to prove compliance |
| :--- | :--- |
| **A `Remove` commit must leave nothing behind, and must be proven.** | Commits titled *"Remove GI and SDF subsystems"* and *"Retire Fluid/Particle/VectorField/ForceField"* left surviving paths. Before writing "removed", run `git ls-tree -r --name-only HEAD \| grep -i <name>` and paste the empty result. |
| **Never restore deleted code from an upstream snapshot without asking.** | A *"Restore Frontier source snapshot"* commit resurrected a directory the owner had ordered deleted, because the snapshot predated the deletion. If upstream and this branch disagree about a deletion, this branch wins. Ask first. |
| **Never report a stub harness as a passing visual test.** | A jsdom harness with a stubbed 2D canvas scored itself 13/13 PASS in the same report that said pixel capture was blocked. A stubbed canvas proves nothing visual. |
| **Never describe an implementation you did not port.** | A 152 KB HTML vehicle solver became a 7.9 KB C++ scalar force model with no tyre force, no slip ratio, and no angular velocity. It was reported as the port. It is not. |
| **Never simplify.** | Sprite particles instead of the HTML particle output. Fewer presets than the source. A copy is not a port. If the full conversion cannot be done, deliver the part that is exact and say precisely what is missing. |
| **Never let a rule cite a path that does not exist.** | The previous ruleset cited a fluid workspace README, a particle workspace README, a path-tracer exhibit, and six GI shaders that had already been deleted. It answered questions nobody could act on. `AuditCheck.py` now fails on this. |

---

## 9. Current state — 2026-11-20

| Item | State |
| :--- | :--- |
| Renderer | Direct lighting only: `MatCap`, `OpenPbrDirect`. No GI modes |
| GI / SDF | Deleted by `63db120`, archived under `Experimental/`. Awaiting the owner's rebuild decision |
| Fluid, Particle, Vector Field, Force Field | Deleted. Their plans are SUPERSEDED |
| Particle rebuild | Not started. The HTML ground truth is **not in this repository**; it exists on `arena/6eeea758-frontier` in this repository's history. Do not begin until the owner says where the ground truth lives |
| Vehicle dynamics | The physics solver under `Engine/SimulationSpace/` is a scalar force model with no wheel, no slip ratio, no angular velocity. It cannot reach a dead stop. It is not a port |
| Workspaces registered | `ComponentEditor`, `Fracture`, `Material`, `ParametricSketcher`, `Revision`, `Texturing`, `UV` |
| Desktop executables | 9, per `Engine/Lifecycle/README.md` |
| Shaders | 77 `.slang`, 14 not yet named in `CMakeLists.txt` |

---

## 10. Bringup order for new work

1. Dependencies and build profiles — `Automation/`, `ExternalPackages/`, `Engine/Configuration/BuildConfiguration.h`, `CMakeLists.txt`
2. Input and windowing — `Engine/Input/`, `Engine/Windowing/`
3. Editor and workspace groundwork — `Engine/Workspaces/`, `Engine/Editor/` behind `FRONTIER_EDITOR_ENABLED`, the docked ImGui host, GPU overlay hooks

Within any feature: HTML ground truth captured, then the C++ conversion, then build and tests, then the
capture, then the comparison, then the report. In that order. Do not reorder it to show progress.
