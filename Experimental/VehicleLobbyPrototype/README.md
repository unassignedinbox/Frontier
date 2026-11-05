# Car configurator lobby — HTML prototype

🧩 A browser prototype of the car configurator lobby, running on the pinned VehicleHMI WebGPU renderer. The lobby uses the
studio set (round cyclorama, overhead softbox, glossy floor) and a roster of members who join, leave and press Ready.

📝 Status: prototype for user review. No C++, GLFW, vehicle-input or native lobby-level work is included. That waits for feedback.

## Run it

Run from the repository root.

```bash
# 1. Restore the renderer (git-ignored ExternalPackages/VehicleHmiRenderer), apply the patches, install npm dependencies.
python3 Automation/RestoreVehicleHmiRenderer.py

# 2. Serve the lobby on 0.0.0.0:8080. "/" redirects to /lobby/index.html?scene=studio.
python3 Experimental/VehicleLobbyPrototype/Tools/Serve.py --port 8080

# 3. Open http://127.0.0.1:8080/ in a WebGPU browser. Open a second tab on /lobby/peer.html to act as another member.
#    /lobby/peer.html?ready=1 presses Ready on join.
```

Roster unit tests (no browser needed):

```bash
node --test Experimental/VehicleLobbyPrototype/Lobby/Tests/RosterRegistry.test.mjs
```

Note: `node --test <directory>` fails on Node 22, so name the file explicitly.

Headless capture (Playwright on SwiftShader). It needs the browser dependencies in the git-ignored `ExternalPackages/CaptureTools`:

```bash
cd ExternalPackages/CaptureTools && npm install && cd -   # playwright-core@1.49.1, @sparticuz/chromium@131
node Exhibits/VehicleLobbyPrototype/CaptureLobbyProof.mjs
```

The capture expects the Chromium runtime libraries at `/tmp/al2023/lib` and the SwiftShader ICD at `/tmp/vk_swiftshader_icd.json`
(override with `LD_LIBRARY_PATH` and `VK_ICD_FILENAMES`). Outputs and the result log are in `Exhibits/VehicleLobbyPrototype/`.

## What it does

| Area | Behaviour |
|------|-----------|
| Studio set | `Renderer/StudioCyclorama.js`: floor disc r 10 m, quarter-circle cove r 8 m, wall to 20 m, centred on the spawn (-40, 0). One 6 × 4 m softbox at 7 m, facing down. Sky, sun, clouds, fog and aerial perspective are off through the renderer's environment keys. The proving ground, course lamps, cones and breakables are not built when `?scene=studio` is set. |
| Lobby roster | `Lobby/Source/RosterRegistry.js` (pure data). Members announce over `BroadcastChannel` (`frontier-lobby-v1`), heartbeat every second, and are dropped after 4 s of silence. A closed tab leaves on its own. |
| Ready | Ready marks the member. The room travels to the next level once every listed member is ready. Leaving the lobby removes a member, so a member who leaves no longer blocks travel. |
| Next level | `/index.html`: the upstream Proving Grounds editor page. Travel restores the environment the editor had saved before the lobby overwrote it. |
| Configurator | Four paint swatches, matte, dark wheels and blue rims. `Lobby/Source/AppearanceChoices.js` maps the choice to `window.IlluminationExhibit.Cabin.Appearance.Apply`. |
| Driving keys | WASD, arrows, space and shift are stopped in the capture phase, so the car stays still in the lobby. |

## Renderer changes (patches)

The upstream renderer is pinned in `Automation/VehicleHmiRendererPin.json` (commit `6afb6f0`). The restore step applies two patches
from `Renderer/Patches/`, plus an overlay for the studio module:

| Patch | Change | Reason |
|-------|--------|--------|
| `0001-chrome-smoothstep-edge.patch` | `Shader/ClusteredLights.wgsl` line 100: `smoothstep(0.06, -0.04, z)` → `1.0 - smoothstep(-0.04, 0.06, z)` | Chromium 131 (Tint) rejects `smoothstep` with `low ≥ high` when both are constants. The new form has the same falloff. |
| `0002-studio-scene.patch` | `Source/FacetStructure.js`: the studio ground and softbox replace the proving ground; no placements, cones, breakables or stress set in studio mode. `Source/IlluminationHost.js`: course lamp poles do not light the studio. | Studio mode only; the default course is unchanged. |
| Overlay | `Renderer/StudioCyclorama.js` → `Source/StudioCyclorama.js` | New module. |

## Evidence

- `Exhibits/VehicleLobbyPrototype/capture-result.json`: 6 of 6 checks passed, 0 page errors, 916 s run.
  - studio renders frames (FrameIndex ≥ 3)
  - roster starts with one member
  - a peer join adds a member (2 players)
  - the peer leaves and the roster drops to 1
  - Ready waits for the other member (status text checked)
  - all ready moves the room to `/index.html`
- Screenshots: `lobby-one-member.png`, `lobby-two-members.png`, `lobby-waiting-for-ready.png`, `next-level-after-ready.png`.

## Limitations

- **SwiftShader only.** Frames run at about 10 fps at `quality=low` (the lobby renders at 864 × 540). These are not Windows GPU numbers.
- **Look versus the references.** The studio is a first pass. The wall top shows a blotchy grey haze, and the floor reflection is weak.
  The camera is the renderer's chase view from behind the car, not the three-quarter studio angle in the reference images.
- **Appearance is local.** Each browser renders its own car, so a finish chosen in one tab is not shown in another.
- **Peers are browser tabs.** Members share one browser profile over `BroadcastChannel`. There is no network transport, no server
  authority, and no cross-machine lobby.
- **Clicks in the capture use `element.click()`.** Playwright's actionability checks wait for animation frames, which the SwiftShader render starves.
- **Names.** Lobby members get generated names (`Player XXXX`). The 100-name pool for character replacement is not implemented yet.
- **Inherited names.** `Source/`, `Exhibits/`, the `?scene=` parameter, `window.IlluminationExhibit`, `ControlVehicle` (the HUD label) and
  `MaterialSpecification.js` come from the renderer or the repository and keep their names. They are not renamed here.
