# Garage configurator — zip-exact port evidence

✔️ Evidence for the 1:1 C++ port of `interactive-vehicle-hmi-configurator.zip` behaviours, per `CLAUDE.md`
(evidence + generating code retained together; the generating code is production code, not a copy).

## The three ported mechanisms

| Zip source | C++ | What matches |
|---|---|---|
| `Truck.tsx` ripple runtime (`beginRun`/`stepRun`/`finishRun`) | `Frontier/Engine/Effects/PaintRipple.h` | Expanding sphere from the touch point; coverage `1 - smoothstep(R-0.55, R, dist)` per pixel; `easeOutCubic`; paint 1.35 s / 6.4 m, rims 1.5 s / 4.6 m; finish commits colour+roughness+metalness on completion |
| `Truck.tsx` apply path (`applyReq`, raycast else nose `(2.75, 1.25, 0)`) | `LobbySession --garage [--paint-ndc X Y]` | Drop point raycast onto the body becomes the centre; a tap uses the nose default |
| `Scene.tsx` stage | `SoftwarePresentation::RenderGaragePpm` | The Lobby studio (cove floor, turntable, one softbox), ONE vehicle, fixed camera `(6.8, 2.9, 7.6)` fov 34° → target `(0, 0.55, 0)`; no rotation anywhere — no turntable spin, `autoRotate` off |
| `data/config.ts` COLORS | `LobbySessionMain` paint table | All 8 finishes, hex + metallic flag verbatim; roughness/metalness targets as in `Truck.tsx` (`matte 0.72`, metallic `0.26/0.9`, non-metallic `0.45/0.35`) |
| `PlumeSpace.js` + `SmokeSequence.js` (fluid/gas editor) | `Frontier/Engine/Effects/{GasParameters,PlumeSolver}` | 32-control catalogue, 6 presets, `ResolvePlumeSettings`; CPU solver steps: splat → buoyancy/wind → vorticity confinement → semi-Lagrangian advection → decay → pressure projection |

## Bug found by demanding this proof

🐞 The plume solver's Jacobi pressure iteration omitted the `h²` cell-size factor, so the gradient
subtraction over-corrected by `1/h²`: velocity exploded (`inf` within 120 steps) and density smeared
to a uniform fog — a plume that could never be photographed. Fixed in `PlumeSolver.cpp`; guarded by
the new validation check "the projected field stays stable". After the fix, a 120-step Thick-burnout
run: peak density 15.3 at the source, maximum speed 6.5 m/s, mean divergence 0.041.

## Reproduction

```bash
g++ -std=c++17 -I. Frontier/Game/LobbySessionMain.cpp Frontier/Engine/Input/*.cpp \
  Frontier/Engine/Scene/*.cpp Frontier/Engine/Session/*.cpp Frontier/Engine/Effects/*.cpp \
  Frontier/Game/SoftwarePresentation.cpp Projects/Everest/Source/Session/*.cpp -o /tmp/LobbySession

/tmp/LobbySession --garage /tmp/g.ppm --paint red --ripple 0.6            # tap: spreads from the nose
/tmp/LobbySession --garage /tmp/g.ppm --paint blue --ripple 0.6 --paint-ndc 0.3 0.2   # drop: spreads from the hit
/tmp/LobbySession --plume /tmp/p.ppm --plume-frames /tmp/f               # Thick burnout, 120 steps @ 30 Hz
```

- Platform: Linux sandbox, g++ 12.2, C++17 (user platform Windows/MSVC — not run here, marked untested)
- Tested at revision `0abef14` + this change set (uncommitted at capture time)
- Validation: `SessionValidation` — 86 checks, 0 failures (incl. 8 paint-ripple checks: point start,
  easeOutCubic radius 5.6 m at half-time, full cover at the touch, zero beyond the sphere, smoothstep
  edge, 1.35 s commit, rim 1.5 s / 4.6 m course)
- HTML prototype: `CabinSequence` preview spin disabled (`Spin: false`); `npm test` passes (0 failures)

## Evidence in this folder

| File | Shows |
|---|---|
| `PaintRipple.gif` | Nebula Silver → Command Red spreading from the nose over 1.5 s, studio scene, fixed camera |
| `GarageRippleMid.png` | The same at t = 0.6 s: nose covered, tail still silver |
| `GarageRippleDone.png` | t = 1.5 s: ripple committed, whole car red |
| `GarageDropPoint.png` | Tactical Blue dropped at NDC (0.3, 0.2): the raycast rear-quarter hit is the centre — the nose is still silver |
| `PlumePreview.png` / `.gif` | The ported plume solver's own density/heat (side elevation), Thick burnout preset |

## Limitations

- The CPU rasteriser shades flat facets with one overhead light; it is the mirror of record for
  behaviour (coverage, scene, camera), not for PBR clearcoat/metalness appearance.
- The rim ripple mechanism is ported and validated; the Garage CLI drives the body ripple (the
  configurator's primary path). Rim wiring through the CLI is remaining work.

## HTML demo phase (fixed first, for approval before any further C++)

The VehicleHMI demo's Garage now behaves as the reference configurator:

| Requirement | Where | Evidence |
|---|---|---|
| Preview renders the scene, not free space | `CabinSequence.RecordPreview` draws the Lobby studio occupants (cyclorama, ceiling, trusses, softbox, floor) around the hero | `GarageHmiRippleHtml.png`: the car sits in the lit studio well |
| Only 1 vehicle | Guest fleet + guest plates excluded from the preview pass; hero body/wheels/plates only | Same capture: one car in the well |
| Rotation disabled | `Preview.Spin = false`; the turntable no longer turns (manual drag-orbit remains, as the reference's OrbitControls) | `CabinSequence.js` |
| Paint spreads from the touch point | `VehicleAppearance.BeginPaintRipple/AdvancePaintRipple` (1.35 s, 6.4 m, `1-smoothstep(R-0.55, R, d)`, easeOutCubic, commit on completion) + the same blend per-pixel in `ForwardRaster.wgsl` studio mode; tap = nose default, drop = the raycast `BodyPoint` | Capture: Command Red has covered the nose half, the tail is still graphite; unit tests pin radius 5.6 m at half-time and the smooth edge |
| Plume 1:1 with the C++ port | `MapPlumeSettings` (HTML) and `ResolvePlumeSettings` (C++) locked to the same numbers by tests on both sides (density ×1.2, flame core/tip × temperature, `-ln(max(0.005, 1-loss))` decay, the 2124 K check) | `Tools/SlatePort.test.mjs`, `Validation/SessionValidation.cpp` |

Capture method: `Tools/RunOnSoftwareGpu.mjs` + `HeadlessHarness.mjs` with
`Exhibits/GarageConfigurator/GarageCaptureHooks.mjs` (the harness has no 2D canvas, so the hooks supply
a stub canvas/Image and skip the panel-texture upload; the speckle around the tablet in the capture is
that stub, not the app — in a browser the panel draws normally). 4 frames, 0 errors, SwiftShader.
`npm test`: 103 pass, 0 fail.
