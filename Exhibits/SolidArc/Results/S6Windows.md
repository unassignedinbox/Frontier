# 📝 S6: SolidArc Windows build and package

**TL;DR**: SolidArc builds for Windows x64 from this Linux sandbox using Zig 0.17.0 (clang 22.1.8, `x86_64-windows-gnu`, static C++ runtime). A clean build of 243 steps finishes with **0 warnings in SolidArc code**. The two warnings it does report come from unmodified upstream UI code. `SolidArcWorkspace.exe` is a 64-bit GUI program that imports only Windows system DLLs and the UCRT, and it loads Vulkan at runtime. The preview package `SolidArc-Windows-x64.zip` is 3,555,963 B (8.4 MB unpacked). It holds the executable, its options file, the fonts and icons, a sample, a README and the licence notices.

On Linux, the same package layout finds its content beside the executable: the frame matches the development checkout everywhere except the frame-time text. With no content beside it, the program falls back to the ImGui default font and missing-icon markers. ctest passes 28/28. 🔴 **The exe has never been run on Windows** (there is no Windows host or wine here), and it was built with MinGW, not MSVC.

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
| Size | 6,981,120 B | 6,913,536 B |
| Machine | 0x8664 (x64) | 0x8664 (x64) |
| Subsystem | 2: Windows GUI (`-Wl,--subsystem,windows`; `WIN32_EXECUTABLE` alone gave 3, console) | 3: console (upstream setting, unchanged) |
| DLL imports | GDI32, KERNEL32, SHELL32, USER32, plus `api-ms-win-crt-{convert, environment, heap, locale, math, multibyte, private, runtime, stdio, string, time, utility}` | Same, plus comdlg32 and ole32 |
| C++ runtime | Static libc++; no libc++ / libstdc++ / libgcc DLL | Same |
| Vulkan | Loaded at runtime through volk (`LoadLibraryA`); no `vulkan-1.dll` import, so a missing driver produces a message box instead of a loader error | Same |
| Wide-character entry points used | `GetCommandLineW`, `CommandLineToArgvW`, `GetModuleFileNameW`, `MessageBoxW` | `CreateProcessW`, `GetModuleFileNameW`, `MessageBoxW` |

## 🧩 Package (`SolidArc-Windows-x64.zip`)

| Entry | Size | Purpose |
|---|---|---|
| `SolidArc/SolidArcWorkspace.exe` | 6,981,120 B | The application |
| `SolidArc/SolidArcWorkspace.toml` | 940 B | Package options: window, viewport, font, and file paths (autosave in `Autosave/`, documents in `Documents/`, both inside the package folder), plus Vulkan |
| `SolidArc/README.md` | 1,671 B | Requirements (a Vulkan 1.3 driver), start-up, file locations, keys, preview limits |
| `SolidArc/EngineContent/FontArchives/…` | | DM Sans (the upstream interface typeface) |
| `SolidArc/EngineContent/Icons/…` | | Interface icons |
| `SolidArc/Samples/SyntheticBonnet.toml` | 19,820 B | Curve import sample |
| `SolidArc/Notices/…` | | Licences: ImGui, GLFW, toml++, stb, volk, ThorVG, Vulkan-Headers (Apache-2.0, MIT) |

| Metric | Value |
|---|---|
| Zip size | 3,555,963 B |
| Zip sha256 | `1761cda8854e3dfb2c36160cd9c4cc80bab7b819a3ee3a8258a7d59fa4513ab0` |
| Unpacked | 8,445,666 B |
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

## 🏁 Linux regression

| Gate | Result |
|---|---|
| ctest after the S6 source changes (both upstream raster validations + every SolidArc test) | 🟢 28 / 28, 1303.74 s (`NurbsBlend` 725.98 s) |
| `SolidArcBrowserValidation` after the absolute document path | 🟢 15 / 15; the started workspace opened Bracket.solidarc |

## ⚠️ Limits

- 🔴 The exe has never run on Windows, so the Vulkan device, window, input, file dialogs and DPI behaviour on a real Windows desktop are unverified. The first run on Windows is the open acceptance step.
- It is built with MinGW and clang, not MSVC. MSVC portability is kept in the build files but was not compiled here.
- Only SwiftShader has been tested; there has been no hardware GPU run.
- The options file path in `LoadSolidArcOptions` is still opened through a narrow path, which is a problem only if the package sits in a folder whose name is outside the ANSI code page.
- The package is unsigned, so Windows SmartScreen will warn on first start.
