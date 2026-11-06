# Project Everest — C++ two-level build

The standalone `Everest` executable starts in the C++ Lobby and has a small navigation menu for the Lobby and Proving Grounds. Selecting either route rebuilds the session with the destination's Everest policy and spawns a fresh vehicle for every carried player. This is the C++ session/presentation path; it does not use the browser HMI.

## Run locally

From the repository root:

```sh
cmake -S Frontier -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure

# Default: Lobby, with vehicle and studio render.
build/Everest --render lobby.ppm

# Navigate to Proving Grounds and render the same vehicle there.
build/Everest --navigate proving-grounds --render proving-grounds.ppm

# Exercise both menu transitions in one session, returning to the default Lobby.
build/Everest --navigate proving-grounds --navigate lobby --render return-lobby.ppm
```

On Windows, use `build/Release/Everest.exe`. Use `--guests N` (0–4) to spawn more cars and `--xray` to apply the C++ vehicle scan effect to the local vehicle. `--archive DIR` overrides the default vehicle facet archives.

## Download test binaries

Pushing to the session branch runs **Everest C++ binaries** in GitHub Actions for Linux, Windows, and macOS. Each workflow run uploads an `Everest-<OS>` artifact containing the executable, the three ControlVehicle facet archives, and a short run guide. Download the artifact from the Actions run's **Artifacts** section and follow its `README.txt`.

## Native ReSTIR Proving Grounds

The Vulkan ReSTIR illumination host, Slang sources, precompiled SPIR-V and
Proving Ground assets are taken from `unassignedinbox/Frontier` commit
`118dc3e40ff7ccc6a1ceb42d5b4eca7601e437c5` (the upstream parity exhibit is
`Exhibits/RestirGiParity`). The GLFW window uses that renderer for Proving
Grounds and updates the vehicle's body and wheels through the original scene
occupant relocation path. It copies the presented image back to the OpenGL
window; this is not a zero-copy or performance-optimized integration.

```sh
cmake -S Frontier -B build -DCMAKE_BUILD_TYPE=Release -DFRONTIER_ENABLE_GLFW=ON
cmake --build build --config Release
build/EverestWindow --content EngineContent
# An offscreen twelve-frame ReSTIR render, requiring no window:
build/EverestRestir proving-ground.ppm EngineContent
```

A Vulkan 1.3 loader and supported device are required. `FRONTIER_VULKAN_DRIVER`
can specify the loader library. macOS needs a compatible Vulkan/MoltenVK setup;
this adapter has not been tested on macOS. The default Low grade renders the
window at 480 × 312 and then scales it to the framebuffer. With a software
Vulkan driver, rendering can be very slow. Failure to initialize ReSTIR is
reported; the Proving Grounds does not silently fall back to the flat renderer.
`FRONTIER_ENABLE_RESTIR=OFF` explicitly builds the legacy software-only window.

**Not yet ported:** the native Lobby has no ReSTIR scene archive in this
checkout and still uses the software renderer (identified in the window title).
The original native renderer's cabin/HMI and smoke/fire are also not part of
this integration. The command-line `Everest --render` remains the legacy
software screenshot tool; use `EverestRestir` for ReSTIR output.
