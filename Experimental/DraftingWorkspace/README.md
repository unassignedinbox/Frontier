# 🧩 DraftingWorkspace — Vulkan + Slang + ImGui CAD workspace on the SolidArcRemastered kernel

**TL;DR (step 9)**: The workspace runs the SolidArcRemastered feature tree behind an ImGui shell that mirrors the SolidArc browser editor. It uses Vulkan 1.3 (volk), SPIR-V compiled from Slang, a GLFW host without a client API, and TOML options. ctest passes **5 / 5** (90.6 s on SwiftShader).
- **Document gates W1–W8 🟢:** construct, pick, fillet / chamfer, push and parameter edit all match closed forms to ≤ 6.1e-16. Refusals are atomic, undo / redo is exact, and TOML save → load → save is byte-equal.
- **Viewport gates V1–V7 🟢:** the GPU identity buffer equals the CPU ray pick (0 mismatched pixels over 14 views). 100 % of edge pixels are explained, hover probe = readback, selection colours hold, all 7 view presets are checked, and digests are deterministic.
- **Walkthroughs:** **131 / 131** expectations pass (main 78, shortcuts 41, recovery 12), driven by real ImGui pointer and keyboard events on the offscreen Vulkan frame. They produce 21 captures.
- **Parity gate:** `PARITY-SolidArcBrowserEditor.md` has 58 rows: 🟢 48 · ⚠️ 4 · 🚧 6 (sketch drawing, multi-document, workplane offset).
- **View-aligned grid:** Top / Bottom → XY, Front / Back → XZ, Right / Left → YZ, with X red, Y green, Z blue. Isometric keeps the active plane.

## Layout

| Path | Contents |
|---|---|
| `Source/Document/` | `DraftingDocument` (commands → `DraftingCommandOutcome`, undo / redo, selection by persistent name), `DraftingScene` (tessellated faces / edges with identities), `DraftingDocumentCodec` (TOML), `DraftingSpecimen` (the Bracket) |
| `Source/Viewport/` | `DraftingDevice` (Vulkan 1.3 through volk, system loader or an explicit ICD), `DraftingRasterization` (surface, edge and grid pipelines; colour + R32 identity + depth), `DraftingCamera` (upright orbit, pan, dolly, presets, fit), `DraftingPortableImage` (PNG, digests) |
| `Source/Viewport/Generated/` | `DraftingShaderArchive.h` (SPIR-V words) + reflection JSON, generated from `Shaders/*.slang` |
| `Source/Interface/` | `DraftingInterface` (header, Outliner, viewport, Inspector, bevel session, shortcuts, autosave / recovery), `DraftingFrame` (ImGui → Vulkan), `DraftingOptions` (toml++), `DraftingScript` (walkthrough language) |
| `Source/Applications/` | `DraftingWorkspaceHost` (GLFW window + swapchain), `DraftingWorkspaceCapture` (offscreen frames driven by a script) |
| `Validation/` | `DraftingDocumentValidation` (W1–W8), `DraftingViewportValidation` (V1–V7) |
| `Scripts/` | `WorkspaceWalkthrough.txt`, `ShortcutWalkthrough.txt`, `RecoveryWalkthrough.txt` + `RecoveryOptions.toml` |
| `Tooling/` | `PrepareDraftingDependencies.py` (pinned, sha256-checked sources), `CompileDraftingShaders.mjs` (Slang → SPIR-V archive) |
| `DraftingWorkspace.toml` | Every option with its default |

## Build and test (Linux, from the repository root)

```sh
# ① pinned third-party sources → ExternalPackages/ (imgui, tomlpp, glfw, vulkan-headers, stb, volk)
python3 Experimental/DraftingWorkspace/Tooling/PrepareDraftingDependencies.py

# ② a Vulkan driver for headless runs: SwiftShader (google/swiftshader @ cddb5c91b23ff70a1f607ea31610aa0d7f4b5ad5)
git clone https://github.com/google/swiftshader.git _ClaudeScratch/build/swiftshader
git -C _ClaudeScratch/build/swiftshader checkout cddb5c91b23ff70a1f607ea31610aa0d7f4b5ad5
cmake -S _ClaudeScratch/build/swiftshader -B _ClaudeScratch/build/swiftshader/out -G Ninja -DCMAKE_BUILD_TYPE=Release \
      -DREACTOR_BACKEND=Subzero -DSWIFTSHADER_BUILD_TESTS=OFF -DSWIFTSHADER_BUILD_BENCHMARKS=OFF -DSWIFTSHADER_BUILD_PVR=OFF \
      -DSWIFTSHADER_BUILD_WSI_XCB=OFF -DSWIFTSHADER_BUILD_WSI_WAYLAND=OFF -DSWIFTSHADER_WARNINGS_AS_ERRORS=OFF
ninja -C _ClaudeScratch/build/swiftshader/out vk_swiftshader
mkdir -p _ClaudeScratch/SwiftShaderDriver && cp _ClaudeScratch/build/swiftshader/out/Linux/libvk_swiftshader.so _ClaudeScratch/SwiftShaderDriver/

# ③ configure + build (the windowed host is compile coverage only on a headless Linux GLFW null platform)
cmake -S Experimental/DraftingWorkspace -B Experimental/DraftingWorkspace/build -G Ninja -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DDRAFTING_WINDOWED_HOST=OFF
ninja -C Experimental/DraftingWorkspace/build

# ④ the 5 tests (walkthroughs run from the repository root; without a Vulkan driver the GPU tests skip with code 77)
cd Experimental/DraftingWorkspace/build && \
  DRAFTING_VULKAN_DRIVER=$PWD/../../../_ClaudeScratch/SwiftShaderDriver/libvk_swiftshader.so ctest --output-on-failure
```

Windows (MSVC, the target platform): run ① and then `cmake -S Experimental/DraftingWorkspace -B Experimental/DraftingWorkspace/build -G "Visual Studio 17 2022"`. The host then opens a real window and uses the system Vulkan loader. CMake sets `/utf-8` for MSVC. ⚠️ This sandbox has no MSVC, so the Windows build itself is unverified here.

## Regenerate the exhibits

```sh
B=Experimental/DraftingWorkspace/build; D=_ClaudeScratch/SwiftShaderDriver/libvk_swiftshader.so; E=Exhibits/DraftingWorkspace
mkdir -p $E/Viewport $E/Walkthrough
$B/DraftingDocumentValidation > $E/DocumentValidation.md
$B/DraftingViewportValidation --vulkan-driver $D --exhibits $E/Viewport > $E/ViewportValidation.md
$B/DraftingWorkspaceCapture --vulkan-driver $D --specimen --reset-files \
    --recovery _ClaudeScratch/DraftingWorkspace/Recovery.drafting.toml --document _ClaudeScratch/DraftingWorkspace/Bracket.drafting.toml \
    --script Experimental/DraftingWorkspace/Scripts/WorkspaceWalkthrough.txt --exhibits $E/Walkthrough > /dev/null
$B/DraftingWorkspaceCapture --vulkan-driver $D --options Experimental/DraftingWorkspace/Scripts/RecoveryOptions.toml \
    --script Experimental/DraftingWorkspace/Scripts/RecoveryWalkthrough.txt --exhibits $E/Walkthrough > /dev/null
$B/DraftingWorkspaceCapture --vulkan-driver $D --specimen --reset-files \
    --recovery _ClaudeScratch/DraftingWorkspace/ShortcutRecovery.drafting.toml --document _ClaudeScratch/DraftingWorkspace/Shortcut.drafting.toml \
    --script Experimental/DraftingWorkspace/Scripts/ShortcutWalkthrough.txt --exhibits $E/Walkthrough > /dev/null
```

The recovery run consumes the autosave the main walkthrough leaves behind, so keep that order. `DRAFTING_VULKAN_DRIVER` can replace `--vulkan-driver`.

## Regenerate the shader archive (after editing `Shaders/*.slang`)

```sh
npm install --prefix _ClaudeScratch/tmp/SlangCompiler --ignore-scripts slang-loader@0.0.7
node Experimental/DraftingWorkspace/Tooling/CompileDraftingShaders.mjs \
     --compiler _ClaudeScratch/tmp/SlangCompiler/node_modules/slang-loader/vendor/slang-wasm.js
```

Alternatively, configure with `-DDRAFTING_SLANG_COMPILER=<…>/slang-wasm.js` and the build regenerates the archive whenever a `.slang` file changes. Without it, the committed archive is used as is.

## Capture tool

| Flag | Meaning |
|---|---|
| `--vulkan-driver PATH` | ICD library to dlopen (else `DRAFTING_VULKAN_DRIVER`, else the system loader) |
| `--options PATH` | TOML options (see `DraftingWorkspace.toml`) |
| `--script PATH` / `--exhibits DIR` | Walkthrough to run, and where to write `NAME.png` + `NAME.json` per `capture` plus the `<Script>.md` report |
| `--specimen` · `--open PATH` | Start from the Bracket specimen, or open a document |
| `--recovery PATH` · `--document PATH` · `--reset-files` | Autosave and Ctrl+S paths; delete both (and the previous copy) first |
| `--width` · `--height` | Frame size (default 1600 × 900) |

Exit codes: 0 all expectations pass · 1 an expectation failed · 2 bad arguments or script · 77 no Vulkan device.

## Walkthrough language (one micro-step per frame; whole-line `#` comments)

| Command | Effect |
|---|---|
| `frames N` · `move T` · `click T [Left\|Right\|Middle] [Ctrl] [Shift]` · `press` / `release` | Pointer input |
| `drag BUTTON T by DX DY STEPS [mods]` · `wheel T STEPS` | Drags and wheel |
| `key NAME [Ctrl] [Shift]` · `type TEXT` | Keyboard |
| `capture NAME` | Write the frame and a JSON state snapshot |
| `expect FIELD OP VALUE` | OP one of `== != < > ~ contains lacks`; fields such as `faces`, `steps`, `volume`, `selected-edges`, `bevel-chamfer`, `camera-yaw`, `grid-plane`, `welcome`, `file:PATH` |

Targets: `@Widget` (a registered widget's centre, e.g. `@BevelSize`, `@Step:Blend#9`), `world X Y Z` (projected), `viewport U V`, or `X Y` pixels, each with an optional `offset DX DY`.

## Honest limits

- The captures come from a software Vulkan driver (SwiftShader, Subzero). They prove the rendering, picking and interface paths. They are **not** evidence of physical-GPU performance or of desktop window input.
- The GLFW host compiles and links on Linux (`-DDRAFTING_WINDOWED_HOST=ON`). On the headless null platform it exits 77 with a message. It has not been run in a real window here.
- Not built yet: interactive sketch drawing tools, a workplane offset, multi-document state and docking (see 🚧 rows in the parity checklist).
- ⚠️ Known artefact: dotted depth fighting where the boss-top circle edge is seen exactly edge-on in Front parallel (capture 09FrontParallel).
