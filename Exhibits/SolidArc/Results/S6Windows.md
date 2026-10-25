# 📝 S6: SolidArc Windows build and package

**TL;DR**: SolidArc builds for Windows x64 from this Linux sandbox using Zig 0.17.0 (clang 22.1.8, `x86_64-windows-gnu`, static C++ runtime). A clean build of 243 steps finishes with **0 warnings in SolidArc code**. The two warnings it does report come from unmodified upstream UI code. `SolidArcWorkspace.exe` is a 64-bit GUI program that imports only Windows system DLLs and the UCRT, and it loads Vulkan at runtime. The preview package `SolidArc-Windows-x64.zip` is 3,719,715 B (8.8 MB unpacked). It holds the executable, its options file, the fonts and icons, a sample, a README and the licence notices.

On Linux, the same package layout finds its content beside the executable: the frame matches the development checkout everywhere except the frame-time text. With no content beside it, the program falls back to the ImGui default font and missing-icon markers. ctest passes 28/28. It was built with MinGW, not MSVC. **First Windows run (user, AMD Radeon RX 9060 XT, Vulkan 1.4.359):** the interface, fonts and icons come up clean, but the viewport showed displaced tiles and picking missed. The cause was a stale viewport texture, now fixed in the current zip, which awaits a second run (see below).

## 🏷️ Toolchain

| Item | Value |
|---|---|
| Compiler | Zig 0.17.0 (pip `ziglang`) → clang 22.1.8, target `x86_64-windows-gnu` (MinGW-w64 headers, UCRT) |
| Toolchain file | `Experimental/SolidArc/Native/Tooling/WindowsZigToolchain.cmake` (wrappers from `$ENV{SOLIDARC_ZIG_TOOLS}`, forwarded to try_compile) |
| Driver script | `Experimental/SolidArc/Native/Tooling/CrossBuildWindows.sh <Frontier checkout> [build] [targets…]`: configure → build → `cmake --install --component SolidArc` → zip |
| Targets | `SolidArc` (`SolidArcWorkspace.exe`) and `Frontier` (`Frontier.exe` with the SolidArc browser link) |
| Clean build | 243 / 243 steps, exit 0 (`_ClaudeScratch/logs/win-clean.log`) |
| Warnings | 0 in SolidArc and GeometryProcessing sources. There are 2 in upstream UI: `ProjectBrowserPanel.cpp:1220` dangling else (the same code is at upstream line 1152), and `ContentBrowserPanel.cpp:154` comparing a 16-bit `ImWchar` with 0x10000 (always true). GCC on Linux reports neither |
| Why not MSVC | The MSVC / Windows SDK downloads (xwin) are on blocked Microsoft hosts. MSVC is still the delivery compiler; the build files keep `/utf-8` and `/ENTRY:mainCRTStartup` for it |

## 🔍 Executables (pefile)

| Property | SolidArcWorkspace.exe | Frontier.exe |
|---|---|---|
| Size | 7,320,576 B | 6,917,632 B |
| Machine | 0x8664 (x64) | 0x8664 (x64) |
| Subsystem | 2: Windows GUI (`-Wl,--subsystem,windows`; `WIN32_EXECUTABLE` alone gave 3, console) | 3: console (upstream setting, unchanged) |
| DLL imports | GDI32, KERNEL32, SHELL32, USER32, plus `api-ms-win-crt-{convert, environment, heap, locale, math, multibyte, private, runtime, stdio, string, time, utility}` | Same, plus comdlg32 and ole32 |
| C++ runtime | Static libc++; no libc++ / libstdc++ / libgcc DLL | Same |
| Vulkan | Loaded at runtime through volk (`LoadLibraryA`); no `vulkan-1.dll` import, so a missing driver produces a message box instead of a loader error | Same |
| Wide-character entry points used | `GetCommandLineW`, `CommandLineToArgvW`, `GetModuleFileNameW`, `MessageBoxW` | `CreateProcessW`, `GetModuleFileNameW`, `MessageBoxW` |

## 🧩 Package (`SolidArc-Windows-x64.zip`)

| Entry | Size | Purpose |
|---|---|---|
| `SolidArc/SolidArcWorkspace.exe` | 7,320,576 B | The application |
| `SolidArc/SolidArcWorkspace.toml` | 940 B | Package options: window, viewport, font, and file paths (autosave in `Autosave/`, documents in `Documents/`, both inside the package folder), plus Vulkan |
| `SolidArc/README.md` | 1,671 B | Requirements (a Vulkan 1.3 driver), start-up, file locations, keys, preview limits |
| `SolidArc/EngineContent/FontArchives/…` | | DM Sans (the upstream interface typeface) |
| `SolidArc/EngineContent/Icons/…` | | Interface icons |
| `SolidArc/Samples/SyntheticBonnet.toml` | 19,820 B | Curve import sample |
| `SolidArc/Notices/…` | | Licences: ImGui, GLFW, toml++, stb, volk, ThorVG, Vulkan-Headers (Apache-2.0, MIT) |

| Metric | Value |
|---|---|
| Zip size | 3,719,715 B |
| Zip sha256 | `d23b1948e8564d6fdbf856fbafc66f06611f0590e7c3e4f0c86014166239c840` |
| Unpacked | 8,771,274 B (342 entries) |
| Location | `Exhibits/SolidArc/Package/SolidArc-Windows-x64.zip` (committed on this branch for the first Windows run; 🚩 remove before the S7 PR) |
| Excluded | `Frontier.exe`: the package is SolidArc only (install component `SolidArc`) |

The zip records file times, so its sha256 changes every time it is repackaged. The value above belongs to the zip that sits at the location shown.

## 🏁 Package layout at runtime (Linux, SwiftShader, `ShellLook.txt`, 1600 × 960)

| Run | Setup | Compared with A | Result |
|---|---|---|---|
| A | Development checkout; content from `SOLIDARC_CONTENT_ROOT` | Reference | 🟢 |
| B | Executable copied into a package-style folder with `EngineContent/` beside it. The checkout's DM Sans folder was renamed away during the run, and the working directory was elsewhere | Equal except the frame-time text in the status bar (box x 1101–1344, y 940–950: "61.0 ms frame" against "73.9 ms frame") | 🟢 |
| D | Bare executable in `/tmp` with no `EngineContent` above it (control) | 77,553 px differ across the whole frame: ImGui default font and ☒ missing-icon markers | 🟢 the fallback is visible, not silent |

Evidence: `S6Windows/PackageContentRoot.png` (top 260 px of A, B and D stacked).

An earlier control run C, from a folder inside this repository, walked up to the repository's own `EngineContent/` and so was not a true "no content" case. Run D replaces it.

## 🧩 Start-up and paths

| Topic | Behaviour | Where |
|---|---|---|
| Content root | Use `SOLIDARC_CONTENT_ROOT` if it holds DM Sans. Otherwise use the parent of the nearest `EngineContent/` from the executable folder, then fall back to `.` | `UI/SolidArc/Interface/SolidArcInterface.cpp` |
| Working folder | If the working directory has no `SolidArcWorkspace.toml`, the app changes to the executable folder. Command-line paths are made absolute first | `Experimental/SolidArc/Native/SolidArcApplication.cpp` |
| Arguments | Read as UTF-16 through `CommandLineToArgvW`, then converted to UTF-8. The narrow `argv` is in the ANSI code page | same |
| File streams | `ResolveUtf8Path` builds `std::filesystem::path` from UTF-8 (`char8_t`), so non-ASCII folder names open on Windows. This covers STEP, STL, drawings, curve import and the document codec, and Save creates missing folders | `Engine/GeometryProcessing/Exchange/ExchangePath.h` and its callers |
| Images | `STBI_WINDOWS_UTF8` / `STBIW_WINDOWS_UTF8` | `SolidArcImagePlane.cpp`, `SolidArcPortableImage.cpp` |
| Start-up failures | The GUI build has no console, so a missing Vulkan driver, missing content or an unreadable options file is reported with `MessageBoxW` (stderr on Linux) | `ReportStartupFailure` |
| Browser hand-off | `LaunchSolidArcWorkspace` passes an absolute, normalised document path. The workspace may change to its own folder, which would break a relative path | `UI/SolidArc/Browser/SolidArcBrowserLink.cpp` |
| Preview cache key | Microseconds as `long long`. libc++ on Windows uses a 128-bit `file_time_type::rep`, which makes `std::to_string` ambiguous | `SolidArcBrowserPreview.cpp` |

## 🐞 First Windows run

| Item | Observed |
|---|---|
| Machine | AMD Radeon RX 9060 XT, Vulkan 1.4.359 (status bar) |
| Start-up | 🟢 Window, interface, DM Sans, icons, Outliner, Inspector and status bar as on Linux; frame time 0.6–3.6 ms |
| Viewport | 🔴 Displaced rectangular tiles of the grid; a new cylinder appeared in pieces away from its hover outline, and clicking it selected nothing (`S6Windows/WindowsFirstRunTiled.png`, `S6Windows/WindowsFirstRunCylinder.png`) |

| Step | Finding |
|---|---|
| Cause | `PrepareViewport` registered the ImGui texture again only when `Raster.ColourView()` differed from the last registered handle. After a resize, the destroyed view's handle value is reused for the new view (SwiftShader does it too: V11 reports "this driver recycled the colour view handle"). The descriptor therefore kept the old image: freed memory read with the old size and tiling. SwiftShader stores the view object itself, so it drew correctly by chance. AMD stores the image address and layout, so it drew tiles. Picking reads the live identity target, so it disagreed with the stale picture |
| Fix | `SolidArcRasterization::TargetGeneration()` counts target rebuilds, and the frame keys the texture on it (`SolidArcFrame.cpp`) |
| Guard | `SolidArcViewportValidation` V11: the generation holds on an equal size and advances on each real resize, and the frame after a rebuild equals the frame before (3 / 3 🟢) |
| Linux recheck | SolidArcViewportValidation, BrowserValidation, Workspace, Matcap and XRay walkthroughs 🟢. A maximise → restore capture is identical, old code against new, on SwiftShader (the bug cannot show there) |
| Open | 🔴 Second Windows run with the fixed zip |

## 🐞 Bevel freeze (second Windows report)

**TL;DR**: Beveling an upright edge beside the earlier 0.4 bottom fillet froze the workspace: the window stopped responding but kept using CPU. The bevel preview runs on the interface thread, and one preview took **334 s** at r 0.1. Four kernel defects were behind it. All four are fixed, every radius now builds exactly in about 0.05 s, and a fillet or chamfer as wide as its neighbouring face is refused instead of crashing.

| Defect | Where | Effect before | Fix |
|---|---|---|---|
| Cap foot took the wrong root | `EdgeBlend.cpp` `ResolveCap` | The contact line meets the bottom fillet's cylinder twice, at the same distance from the vertex. Rounding picked the root on the far side of the axis, so r 0.01 / 0.03 / 0.05 / 0.12 / 0.3 were refused as "consumed by the blend" while the radii next to them built | `CrossCurveFace` keeps only crossings that lie on the face (`ClassifyFacePosition`), nearest first |
| Touch picked by residual | `CurveSurfaceIntersection.cpp` merge | At a tangential touch \|d\| is flat to second order, so the duplicate kept was chosen by rounding. The foot sat 4.9·10⁻⁹ off the tangency | The duplicate with the smaller \|d′\| / \|C′\| is kept, so the foot lands exactly on the tangency |
| Random tangent at a singular knot | `IntersectionCurve.cpp` `CurveDirection` | Where the two fillets touch, N₁ × N₂ is rounding noise. Normalising it gave the guide a random tangent: speed 21× and C′ off by 80 % | No direction below \|N₁ × N₂\| 10⁻⁶. The chord stands in, and the segment is refined until the regular end agrees with it |
| C′ blew up near the tangency | `EvaluateIntersectionCurve` | \|C′\| reached 168 against a true 1.0 within 10⁻⁶ of the end. The volume quadrature integrated this as 1.7·10⁻⁵ of spurious volume and kept bisecting it | Below \|N₁ × N₂\| 10⁻³, C′ is the central difference of the evaluation map, so it stays consistent with C |
| Unbounded bisection | `EnclosedVolume.cpp` | A noisy integrand was halved without limit | At most 2048 halvings per interval, and a stagnation floor of 10³ × the acceptance floor (a safety net; it no longer triggers here) |
| Blend as wide as the face | `EdgeBlend.cpp` `ComposeFaces` | Size 1.5 (the cap's depth) read `Uses.back()` of an empty vector: SIGSEGV | Refused: "blend consumes the face next to the corner … (size reaches the face width)" |

| Upright r beside the 0.4 fillet | Before | After | \|V − closed form\| / V after |
|---|---|---|---|
| 0.01 / 0.03 / 0.05 / 0.12 / 0.3 | refused | 0.05–0.08 s | ≤ 1.3·10⁻¹² |
| 0.02 | 4.5 s, 3.5·10⁻⁶ off | 0.05 s | 2.1·10⁻¹² |
| 0.1 | **334 s** | 0.06 s | 8.5·10⁻¹³ |
| 0.1137 (the screenshot) | 4.3 s, 5.6·10⁻⁶ off | 0.06 s | 6.4·10⁻¹³ |

- 📝 **Closed form:** V = 3 − (1 − π/4)(R²·2 + r²) + ∫₀ʳ 𝑢max(𝑣)·h(𝑣) d𝑣. The integral is the overlap of the two notch prisms, where 𝑢max is the upright notch's reach and h is the bottom fillet's height. It is evaluated with Simpson's rule in 𝑠, with 𝑣 = r𝑠².
- 🏁 **Gate:** `DocumentValidation.md` W39 (10 checks): 8 radii ≤ 2.1·10⁻¹² and ≤ 20 s each, plus the fillet and chamfer of size 1.5 refused with the document byte-equal.
- 🔍 **Sweep:** a block gets a first fillet or chamfer (12 edges × 4 sizes), then a second fillet or chamfer on every edge at 12 sizes. Before the fixes, 4,488 cases found the six size-1.5 crashes and slow cases up to 2.2 s. After the fixes, the full run of **52,128 cases** completed: 34,444 built, 17,684 refused, **0 crashes, 0 stalls, slowest 103 ms**.
- ⚠️ **Still open:** the preview is still synchronous. A kernel case that is slow for another reason would still pause the window while it runs.

## 🏁 Linux regression

| Gate | Result |
|---|---|
| ctest after the S6 source changes (both upstream raster validations + every SolidArc test) | 🟢 28 / 28, 1303.74 s (`NurbsBlend` 725.98 s) |
| `SolidArcBrowserValidation` after the absolute document path | 🟢 15 / 15; the started workspace opened Bracket.solidarc |
| ctest after the bevel-freeze fixes (kernel + W39) | 🟢 28 / 28, 1289.04 s (`NurbsBlend` 646.91 s) |
| Kernel ctest (`Exhibits/GeometryProcessing`, PatternStress separately) | 🟢 29 / 29, 1350.31 s; PatternStress 69 / 69 🟢 in 495.9 s (was 754.4 s). Only P2's volume changed, and it is now the tessellation limit (see the GeometryProcessing README) |
| Solid loft shells, face sketches, Ctrl snaps, hide / lock (package rebuilt, 0 warnings) | 🟢 DocumentValidation W1–W45; ctest 13 / 13 re-run walkthroughs (incl. SolidArcShellsWalkthrough 34 / 34); 1316 / 1316 expectations across 29 reports |

## ⚠️ Limits

- One Windows run so far (AMD, before the viewport fix). Viewport, picking, file dialogs and DPI behaviour on Windows still need the second run.
- It is built with MinGW and clang, not MSVC. MSVC portability is kept in the build files but was not compiled here.
- Only SwiftShader has been tested; there has been no hardware GPU run.
- The options file path in `LoadSolidArcOptions` is still opened through a narrow path, which is a problem only if the package sits in a folder whose name is outside the ANSI code page.
- The package is unsigned, so Windows SmartScreen will warn on first start.
- The package now includes the drawn Construct primitives and the development trace (`FRONTIER_DEVELOPMENT_TRACE` ON). `SolidArcTrace.log` is written next to `SolidArcWorkspace.exe`, the previous session's log is kept as `SolidArcTrace.previous.log`, and F12 shows the live tail (`ConstructAndTrace.md`).
- The package now includes Inset on faces: O (Plasticity offset-curve on a face) or I opens it; D sets the inset distance, H the depth (− pocket, + boss).
