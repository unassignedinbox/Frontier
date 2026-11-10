# Frontier — ReSTIR PT Enhanced (HTML Render Engine)

Open `index.html` (or serve: `python3 -m http.server 8000`) in a WebGL2 browser.

**Engine:** progressive WebGL2 path tracer — reflections, refraction (glass/IOR/transmission),
metal/plastic/ceramic/velvet/car-paint/clearcoat materials, emissive area light + NEE,
temporal accumulation, and a ReSTIR-PT-Enhanced mode (temporal + spatial reservoir reuse)
benchmarked against a Classic ReSTIR PT 2022 baseline mode (reported 2.08–3.05× in the
EnhancedAuthors report; HUD shows live SPP / effective SPP / reservoir M / variance).

**Scene:** material showroom (glass/chrome/plastic/ceramic/velvet spheres, mirror/rubber/
car-paint cubes, polished checker floor, coloured GI walls) + the ControlVehicle from the
VehicleHMI demo (paint body, glass cabin, wheels, headlights) with drive physics.

**Controls:** WASD/arrows drive, Space brake, C camera, drag orbit, wheel zoom.
UI follows the Project Zero Editor layout: Outliner (left) · Viewport · Inspector (right),
Construct menu bar, console + status bar. Select any object, apply any material preset,
then tweak colour / roughness / metalness / IOR / transmission / clearcoat / emission.
Renderer sliders: SPP, bounces, temporal reuse, spatial reuse radius, resolution, exposure,
sun angle, light intensity, environment; toggles for reflections/refraction and a reservoir
heatmap debug view.
