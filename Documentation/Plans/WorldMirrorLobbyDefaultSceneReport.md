# Frontier World Mirror — Default Lobby Scene & C++ Parity Pass Report

**Branch:** `arena/3bdb09ab-frontier` (continued from `arena/01ea1b85-frontier`)
**Date:** 2026-11-21
**Status:** ✅ Complete — Frontier Lobby is the default scene; all eight requested items closed

---

## 1. Request Recap

The eight directives given for `Experimental/FrontierWorldMirror/`:

| # | Directive | State |
| :---: | :--- | :--- |
| 1 | Sun & sky must use the C++ `SunInspectorPanel` / `AtmosphereSkyInspectorPanel` control set, not the AI-composed layout | 🟢 Done — panels generated from `NativePanels.json` (`SourceCommit 9864969`) |
| 2 | Volumetric cloud inspector must use the C++ `CloudsInspectorPanel` controls and the shipped cloud/atmosphere artwork | 🟢 Done — C++ Shape / Altitude / Tier Budget / Cloud Shadows / Shadow Clock / Wind binding groups |
| 3 | Delete the redundant cloud-coverage infographic card from the clouds settings | 🟢 Done — `CloudCoverageCards` removed (card + plumbing) |
| 4 | `Add notes` must be OFF by default | 🟢 Done — notes textarea only appears after the user opts in |
| 5 | Objects need Physics, plus an `Overview` tab, with quick controls only across `Overview · Transform · Material · Physics · UV · Geometry` | 🟢 Done — A–F pages, quick sliders/switches |
| 6 | Add an `Open Component Viewer` button | 🟢 Done — full-width button in the Overview page opens `Experimental/ComponentViewer/` |
| 7 | Editor Camera must carry `Lens Flare` + `Tonemapping` as sub-components | 🟢 Done — Blueprint-style sub-rows with C++ parity defaults |
| 8 | Replace the cone scene with a default **lobby** containing the vehicle and a plane | 🟢 Done — `Frontier Lobby` level, `ControlVehicle` + `Atrium Display Aircraft` |

GI was explicitly left untouched.

---

## 2. Frontier Lobby (default scene)

`BuildLobbyLevel()` in `Experimental/FrontierWorldMirror/LevelSystem.js` builds a 32-row level.
`BuildDefaultLevels()` now returns **Lobby → Cornell-Studio Gallery → Look-Dev Studio → Empty
Proving Ground**, and the editor seeds its first frame from the active level, so a fresh profile
opens the lobby.

| Row | Mechanism |
| :--- | :--- |
| `Lobby Travertine Floor` | 12.00 × 10.00 m honed travertine slab (top at z = −0.60 m) |
| `Lobby Display Rug` | 10.40 × 6.40 m charcoal fabric rug under both exhibits |
| `Lobby Plaster Wall West / East` + `Lobby Portal Header` | warm lime plaster back wall split around the portal |
| `Lobby Basalt West / East Wall` | dark basalt side walls (cool colour bleed) |
| `Lobby Glazed Portal` | 4.40 × 3.43 m full-height structural glazing (Glass, IOR 1.52, Abbe 26) — opens the physical sky + volumetric clouds into the atrium |
| `Lobby Roof Coffer` | suspended ceiling coffer over the reception bay |
| `Lobby West / East Column` | brushed stainless Ø 0.48 × 4.00 m columns flanking the portal |
| `Lobby Reception Desk` + `Reception Quartz Counter` | walnut counter with honed quartz worktop |
| `Frontier Sign Pylon` + `Frontier Sign Light Panel` | patinated bronze monolith with a 3.4 nit emissive face |
| `ControlVehicle` | `PrimitiveShape: "vehicle"` — 9-instance FVB1 BVH (body + 4 tyres + 4 rims) |
| `Atrium Display Aircraft` | `PropId: "lobby_aircraft"` — 38-part monoplane |
| `Editor Camera` (+ `Lens Flare`, `Tonemapping`, `Render Settings`) | 28 mm, 39.6° vertical FOV, three-quarter establishing shot |
| `Sun` · `Atmosphere` · `Volumetric Clouds` · `Moon` · `Stars` | physical environment; late-afternoon sun (15:12 local), Mie haze, 0.34 coverage cumulus |
| `Lobby Ceiling Lightfield` · `Reception Downlight` | LTC clipped-horizon area quads recessed in the coffer |

### 2.1 `lobby_aircraft` world prop

New entry in `WORLD_PROP_CATALOG` (`WorldPropLibrary.js`, category **Aircraft**): a 6.16 × 4.68 ×
1.84 m high-wing single-engine monoplane — cylinder fuselage barrel and cowl, analytical cone
spinner, 3-blade propeller, dihedral wing panels with cobalt tips and lift struts, tailplane,
fin + rudder accent, smoked canopy (raked windshield / cabin glazing / rear window), tricycle
landing gear and port/starboard navigation lamps. Authored nose-down (−Y), +Z up, wheels on
Z = −0.98 so `HalfHeightZ = 0.98` seats it flush on any floor. The level displays it at 0.72
showroom scale (≈ 3.5 m span).

---

## 3. Storage & Persistence

| Key | Change |
| :--- | :--- |
| `Frontier.WorldMirrorGtx.v6` → `v7` | bumped; `v1…v6` removed on load |
| `Frontier.WorldMirrorLevels.v6` → `v7` | bumped; `v1…v6` removed on load |

The one-time reset is what makes the lobby the default scene for existing browser profiles;
`Clear cache & restart baseline` still returns to the lobby.

---

## 4. Verification

| Gate | Command | Result |
| :--- | :--- | :--- |
| Mirror parity | `node Experimental/FrontierWorldMirror/WorldMirrorValidation.mjs` | **ALL 20 SUITES PASSED** (suite 20 is the new lobby suite: rows, aircraft parts, wheel contact, obstacle clearance, camera framing, CPU GTX render, thumbnail) |
| Live-bundle smoke | jsdom boot of the built `index.html` | Editor mounts on `Frontier Lobby`; ControlVehicle / Atrium Display Aircraft / floor / portal / Sun / Volumetric Clouds / Lens Flare / Tonemapping / Render Settings all present; zero console errors |
| Bundle | `node Experimental/FrontierWorldMirror/Build.mjs` | 5.29 MiB standalone `index.html` |
| Offline visual proof | `CpuGtxRenderer` frame of the compiled lobby | Car left, aircraft right, full-height glazing with sky, rug, coffer and area-light shadows all present |

---

## 5. Honest Notes

- The C++ shadow/GI/reflection pipeline is unchanged — only the default level and its environment
  values are new.
- `CpuGtxRenderer` is the offline proof renderer; the WebGL2 GTX viewport in
  `GtxWorldViewport.jsx` remains the authoritative real-time path (glass refraction, sky LUT and
  volumetric clouds resolve in the browser, not in the CPU fallback).
- The retired `Automotive Cone Slalom` template is gone; `Traffic Cone` / `Cylinder Cone`
  primitives, materials and thumbnails are untouched and remain available in Construct.
