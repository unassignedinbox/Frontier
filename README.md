# Frontier 1 · ReSTIR PT Enhanced

**CandidateEvidenceFit render engine (HTML/WebGL2) + Project-Zero Editor UI + ControlVehicle physics.**

Open `index.html` in a WebGL2 browser (Chrome/Edge, Safari 26+, Firefox with WebGL2 enabled):

```sh
python3 -m http.server 8000 --bind 0.0.0.0
# → http://localhost:8000/
```

Drive: **W/S** throttle/brake · **A/D** steer · **Space** handbrake · **R** reset · **C** camera · **M** HMI/Garage · **P** panels · **H** HUD. Drag to orbit, scroll to zoom. Touch pedals appear on coarse pointers.

## What this is

- **Render engine — ReSTIR PT Enhanced.** Reservoir sampling (temporal + spatial reuse, visibility- + material-aware weights) for direct lighting, path-traced indirect bounces, progressive accumulation, glossy reflections (required). Authors report **2.08–3.05× speedup over their 2022 ReSTIR PT with improved quality** across their tests. Best starting research target for high-quality ray tracing + GI; still complex — closer to production-ready, not universally production-ready. Implemented here in a single WebGL2 fragment shader (MRT: accumulated HDR + per-pixel reservoir, ping-pong to avoid feedback).
- **UI — Project-Zero Editor.** Dark editor chrome (Outliner · Viewport · Inspector, DM Sans, card/metric/slider vocabulary) recreated from `Experimental/ProjectZeroEditor/` (`Editor.jsx`, `Inspectors.jsx`, `AtmosphereLab.jsx`, `GasPanel.jsx`, `FracturePanel.jsx`, `Notch.jsx`, `Editor.css`) on `SultanAladin/Frontier-` branch `arena/10e50e56-frontier` (commit `4010ac4`). Vanilla JS, no build — the 4.9 MB bundled `index.html` is not copied.
- **Vehicle — VehicleHMI ControlVehicle.** GT-R specs verbatim from `VehicleSpecification.js` / `VehicleSolver.js` / `MaterialSpecification.js` / `CourseStructure.js` on `SultanAladin/Frontier-OLD` branch `Frontier`, `Experimental/SlateReference/A/Exhibits/VehicleProvingGrounds/`: mass 1300 kg, CoM 0.53 m, wheelbase 3.396 m, track 2.095 m, tyre Ø 1.017 m, suspension 2.0 Hz / ζ 0.6 digressive, Pacejka MF6.1 sheet, GT-R torque curve (peak 520 N·m), 6-speed + turbo spool (0.55 s), brakes 2600 N·m/wheel, CdA 0.74 / ClA 1.10. Physics is a faithful arcade port (bicycle + friction-circle, strut springs, load transfer, aero, 32 knockable cones). The full 63 KB solver + Jolt WASM debris stack is summarised, not vendored, so the file stays self-contained.

## Scene (demonstrates the engine properly)

Proving grounds: asphalt pad + concrete apron + grass surround, start-straight paint/kerbs, ramp wedge, colour-bleed walls (crimson/viridian/chalk/cobalt) for GI, 5 containers, barriers, lamp poles, 32 slalom cones.

**Materials garden** (north of the start straight): mirror panel (rough 0.02), aluminium sphere (F0 0.91), gold sphere (F0 gold), glass sphere (transmission + IOR, Schlick vs Snell), red plastic (rough dielectric + clearcoat), rubber, ceramic. Plus full PBR set: asphalt, concrete, grass, paint, kerb, body coat (clearcoat paint), vehicle glass (transmission), vehicle plastic, tyre, blue-mag rim, brake, headlight/taillight (emissive), lamps, cones, steel. All linear Rec.709 from `MaterialSpecification.js`.

## Sliders (right Inspector)

- **Render:** resolution scale, max bounces, ReSTIR temporal/spatial reuse, M limit, exposure, gamma, reflection roughness scale, denoise. Modes: ReSTIR PT Enhanced / Direct only / Raster preview. Debug: Beauty / Albedo / Normal / Depth / Reservoir weight / Direct / Indirect.
- **Material:** base colour, metallic, roughness, IOR, transmission, clearcoat, emissive, specular + one-click presets (mirror/aluminium/gold/glass/plastic/rubber/ceramic/body coat). Select any Outliner row to edit.
- **Vehicle:** Garage paint swatches (gloss/matte, dark wheels, blue rims), headlight/taillight intensity, live telemetry.
- **Light:** sun azimuth/elevation, sun/sky intensity, time presets (Noon/Golden/Dusk/Night), glass IOR.
- **Physics:** grip, engine, brake, steering, FOV, chase distance/height.

## Provenance (searched 2026-11-10)

| Source | Branch / commit | Used |
|---|---|---|
| `SultanAladin/Frontier-` `Experimental/ProjectZeroEditor/` | `arena/10e50e56-frontier` (`4010ac4`) | Editor UI (layout, tokens, tabs, sliders) |
| `SultanAladin/Frontier-` `Experimental/FrontierEditor/` | `arena/f7e49d27-frontier` (`4111b04`) | Editor tokens / icon vocabulary (reference) |
| `SultanAladin/Frontier-OLD` `VehicleProvingGrounds/` | `Frontier` | ControlVehicle specs, materials, course, physics |
| ReSTIR PT Enhanced (CandidateEvidenceFit) | — | Reservoir + path-traced indirect + accumulation |

## Notes

- Path tracing is progressive: pause the car (or use Orbit/Free) to let SPP accumulate; driving resets accumulation (motion invalidates temporal reuse). Lower resolution scale / bounces for speed.
- Reflections are path-traced glossy (GGX) + transmission (glass). The mirror/aluminium/gold/glass spheres show the lobes at a glance; try Reflection roughness scale and Glass IOR.
- If WebGL2 is unavailable, the editor UI and physics still load; only rendering is blocked (message in viewport).
