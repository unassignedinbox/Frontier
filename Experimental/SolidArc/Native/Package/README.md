# 🧩 SolidArc — Windows x64 preview package

**TL;DR** — Unzip anywhere and start `SolidArcWorkspace.exe`. It needs a GPU driver with Vulkan 1.3 (current NVIDIA, AMD and Intel drivers install `vulkan-1.dll`). Nothing gets installed: the options, autosave and saved documents stay in this folder.

| Item | Where |
|---|---|
| Application | `SolidArcWorkspace.exe` |
| Options (window, viewport, font, file paths, Vulkan) | `SolidArcWorkspace.toml` |
| Autosave (written 0.6 s after the last edit) | `Autosave/SolidArcWorkspace.solidarc` |
| Default Save / Open document | `Documents/Bracket.solidarc` |
| Import curves sample | `Samples/SyntheticBonnet.toml` |
| UI font and icons | `EngineContent/` |
| Third-party licences | `Notices/` |

## 💡 Start

- Double-click `SolidArcWorkspace.exe`, or open a document: `SolidArcWorkspace.exe --open "C:\Parts\Bracket.solidarc"`.
- `--specimen` starts with the fillet / chamfer specimen; `--options <file>` uses another options file.
- Navigation and commands follow Plasticity (orbit, pan and zoom on the mouse; `B` fillet, `E` extrude, `1`–`5` modes, `Alt+Z` x-ray).

## ⚠️ Preview limits

- Built on Linux with Zig's clang for `x86_64-windows-gnu` (MinGW-w64, static C++ runtime). The MSVC build of the same sources is the release build.
- Not yet started on a Windows machine by the build: please report the first launch (window, viewport, opening a document).
- If the window does not open, start it from a terminal (`cmd`) to see the Vulkan or GLFW message.
