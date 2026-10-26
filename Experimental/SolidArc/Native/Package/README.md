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
| **Log** (every command, preview, kernel step, refusal and slow frame) | `SolidArcTrace.log` (the run before: `SolidArcTrace.previous.log`) |

## 💡 Start

- Double-click `SolidArcWorkspace.exe`, or open a document: `SolidArcWorkspace.exe --open "C:\Parts\Bracket.solidarc"`.
- `--specimen` starts with the fillet / chamfer specimen; `--options <file>` uses another options file.
- Navigation and commands follow Plasticity (orbit, pan and zoom on the mouse; `B` fillet, `E` extrude, `1`–`5` modes, `Alt+Z` x-ray).
- On a curved face, a sketch lies on the exact tangent plane under the pointer. `E` → Union starts the extrude slightly inside the face (the Inspector shows how far), so the joined body closes and its junction edge takes a chamfer.
- A slow fillet / chamfer preview is rebuilt when the pointer rests, not on every pixel. A typed size applies at once.
- **Right-click or `Enter` confirms** every command (fillet / chamfer, extrude, inset, move / rotate / scale, offset, pipe, joint, boolean, a pending dimension) and ends Trim / Fill; `Esc` cancels. A right-drag still pans. A size typed in the field and not yet entered is the one a right-click applies.
- Edges are soft-edged (antialiased) strokes held a few pixels in front of their faces at every zoom, so curved edges no longer break into dashes. The viewport renders with 4× multisampling (the status bar shows `4× MSAA` next to the GPU name), so face silhouettes are smooth too.

## 🔍 Log

- `SolidArcTrace.log` sits next to `SolidArcWorkspace.exe` and is rewritten on each start (the previous run is kept as `SolidArcTrace.previous.log`).
- In the app: the **Log** button at the bottom right of the status bar (or `F12`) opens the log panel. It shows the file path (Copy path), filters (Slow frames, Refusals) and Copy.
- A frame over 50 ms writes a `Frame` line: the total, the interface / upload / GPU wait split, the open session and the costliest phases (for example `bevel preview · stage 39 ms`). The button reads `Log · N slow` once that has happened.
- When something is slow or refused, send `SolidArcTrace.log` (or the panel's Copy).

## ⚠️ Preview limits

- Built on Linux with Zig's clang for `x86_64-windows-gnu` (MinGW-w64, static C++ runtime). The MSVC build of the same sources is the release build.
- Not yet started on a Windows machine by the build: please report the first launch (window, viewport, opening a document).
- If the window does not open, a message box names the cause (no Vulkan 1.3 driver, missing `EngineContent/`, unreadable options file).
- Unsigned preview: Windows SmartScreen may ask for confirmation on the first start (More info → Run anyway).
