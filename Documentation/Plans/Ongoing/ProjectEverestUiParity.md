# Project Everest — UI 1:1 port plan

## TL;DR

The demo's UI is a fixed three-column editor: Outliner, Viewport, Inspector, over a status bar, with a Log Stream
docked below. Frontier already owns that shape — docking, notch, Control Centre, Content Browser, Notifications,
Project Browser, Outliner, Log Stream. What is missing is the middle and the right: viewport chrome, the inspector
host and its sixteen page families, the status bar, and the dedicated studios. Those are the port. Everything is
transcribed from the demo CSS at its stated pixel geometry and proved by rendered capture against a browser mirror.

## 1. Inventory — what the demo actually contains

Source: `Frontier/Projects/Project-VehicleHmiDemo/Reference/index.html` (74 lines, the copied shell) plus the
contract tables in `.../Contract/` (473 files, 80 shaders, 81 entrypoints, 138 UI labels, 138 control-inventory
rows) and the 13 stylesheets named in the shell's `<link>` tags.

### 1.1 Shell elements (complete list from the copied HTML)

| # | Demo element (verbatim id / class) | Native owner | State | Notes |
|---|-----------------------------------|--------------|-------|-------|
| 1 | `#shell` root, window title "Frontier · Proving Grounds — distance-field GI, smoke + fire, fracture" | `WorkspaceHost` | partial | Title text becomes the Everest scene title |
| 2 | `aside#outliner` + `.column-tab` "Outliner" + hide `(P)` + `#outliner-body` | `SceneDirectoryPanel` | ✔️ shell, 🔴 body | Body is JS-generated in the demo; native rows follow record parentage |
| 3 | `#viewport-column` + "Viewport" tab + `.scene-pill` "Proving Grounds" | `ViewportPanel` | 🔴 | |
| 4 | `#viewport-bar` (filled by `EditorControls.js`) | `ViewportControls` | 🔴 | Per-control rows in §1.4 |
| 5 | `canvas#viewport` + `canvas#capture.capture` | Vulkan swapchain + capture target | 🔴 | Binds in phase 4 |
| 6 | `#hud` + close `(H)` + `pre#hud-text` "Starting…" | `ViewportOverlayControls` | 🔴 | Readout text formatting is part of the port |
| 7 | `#inset-frame` (hidden) + `(J)` + `#inset-legend` GDF-slice legend, verbatim | `ViewportOverlayControls` | 🔴 | Legend string is copied character for character |
| 8 | `#overlay-dock` (hidden) + `.overlay-close` | `ViewportOverlayControls` | 🔴 | |
| 9 | `#overlay-notice` (hidden), `#event-notice` (hidden) | `ViewportOverlayControls` | 🔴 | |
| 10 | `#bring-up` — `h2`, `.bring-up-caption` "Starting", `.bring-up-bar` | `ViewportOverlayControls` | 🔴 | |
| 11 | `#failure` (hidden) + `PreviewStartup.js` failure text pattern | `ViewportOverlayControls` | 🔴 | Same message pattern as the demo |
| 12 | `aside#inspector` + "Inspector" tab + hide `(P)` + `#inspector-top` + `#inspector-content` | `InspectorPanel` | 🔴 | |
| 13 | `footer#status` — `.status-item` label+value, `.status-fill` | `StatusBarPanel` | 🔴 | |
| 14 | 13 stylesheets + DM Sans 300/400/500 | tokens + `EngineContent/FontArchives/` | 🔴 | Decision D-E2 |
| 15 | Log Stream panel (owner-approved design of a later date, not in the copied shell) | `LogStreamPanel` | ✔️ | Reused unchanged |
| 16 | Control Centre notch drawer | `ControlCentrePanel` | ✔️ | Out of demo; reused unchanged |
| 17 | Content Browser bottom drawer | `ContentBrowserPanel` | ✔️ | Out of demo; reused unchanged |
| 18 | Notification cards and stream | `NotificationSequence` | ✔️ | Out of demo; reused unchanged |
| 19 | Project Browser card grid | `ProjectBrowserPanel` | ✔️ | Out of demo; gains the Everest card |
| 20 | Docking splitters, dock tabs, trapezoid tabs | `PanelDockspace` + patches A–G | ✔️ | Out of demo; the demo is a fixed grid |

### 1.2 Reusable controls the demo's stylesheet vocabulary demands

Twenty-six control families, each one kit control, each built once and reused across pages. New ones are additions
to `Engine/Interface/ControlKit.*`, not a second kit.

| Kit control | Demo origin | First consumer | State |
|---|---|---|---|
| `Panel` | `.pane-head` / `.pane-body` / `.pane-foot` | every dock | ✔️ |
| `IconButton` | `.icon-button` (+`.active`) | Log Stream header | ✔️ |
| `ActionButton` | `.action-button` (+`.primary`, `.danger`), `.action-row` | inspector pages | 🔴 |
| `LinkButton` / `ResetButton` | `.link-button`, `.reset-button` | inspector pages | 🔴 |
| `SearchField` | `.search` | Log Stream | ✔️ |
| `PillDropdown` | `#ddChannels` pattern + glass menu | Log Stream | ✔️ |
| `ContextMenu` | `summon-menu`, `menu-track` | Log Stream overflow | ✔️ |
| `Segmented` | `.segmented` + `.active` | viewport bar | 🔴 |
| `Toggle` / `PropertySwitch` | `.toggle.on`, `.property-switch`, `.switch-row` | inspector pages | 🔴 |
| `NumberField` | `.number-field`, `.transform-fields` | inspector pages | 🔴 |
| `SliderControl` | `SliderControl.css`, `.range-labels` | everywhere | 🔴 |
| `SelectField` | `.select-field`, `.telemetry-select` | telemetry pages | 🔴 |
| `ColourField` | `.colour-input`, `.colour-pair`, `.colour-code` | material, sky | 🔴 |
| `MaterialChip` | `.material-grid`, `.material-chip` | material pages | 🔴 |
| `Card` | `.card`, `.card-heading`, `.card-subtitle`, `.cards` | Moon, clouds | 🔴 |
| `CountTile` | `.count-tiles`, `.count-tile`, `.count-icon`, `.count-caption` | inspector headers | 🔴 |
| `Metric` / `StatReadout` | `.metric`, `.metric-row`, `.stats`, `.readout` | Log Stream, diagnostics | ✔️ partial |
| `DeltaBadge` | `#mRate`, `#mWarnDelta` | Log Stream | ✔️ |
| `Sparkline` | `#spark`, dashed guide, head dot | Log Stream, speed/RPM | ✔️ 72 s; needs 48-sample variant |
| `Pills` | `.live-pill`, `.small-pill`, `.enabled-pill`, `.scene-pill` | shell, Log Stream | ✔️ partial |
| `TreeRow` | `.tree-row` (+`.selected`, `.nested`, `.is-off`), `.visibility` | Outliner | ✔️ |
| `CategoryTabs` | `InspectorCategories.js` tablist, roving tabindex, ←/→/Home/End | every inspector page | 🔴 |
| `DrillRow` / `DrillHud` | `.drill-row`, `#drill-hud` | drill pages | 🔴 |
| `OverlayDock` | `#overlay-dock`, `.overlay-close` | viewport | 🔴 |
| `StatusBar` | `#status .status-item`, `.footer-dot`, `.status-fill` | workspace footer | 🔴 |
| `BringUpBar` | `.bring-up-bar`, `.bring-up-caption`, `.bar-fill` | startup | 🔴 |
| `ColumnChrome` | `.column`, `.column-tabs`, `.column-tab`, `.tab-fill` | three columns | ✔️ partial |
| `Scrollbar` | webkit scrollbar rules on `.inspector-content`, `.outliner-body` | every scroll surface | 🔴 |

### 1.3 Inspector families — sixteen, from the demo's own module names

| Family | Demo module(s) | Labels in contract | Page file |
|--------|----------------|--------------------|-----------|
| Sun | `SunInspector.js`, `SunInspectorDashboard.js` | 138 labels are shared across families; per-page counts are produced in phase 3 from `UiLabels.tsv` | `EnvironmentInspectorPages.cpp` |
| Sky / atmosphere | `AtmosphereInspector.js`, `AtmosphereSkyInspector.js`, `AtmosphereInspectorDashboard.js`, `AtmosphereColumnInstrument.js` | same | `EnvironmentInspectorPages.cpp` |
| Clouds | `CloudInspector.js`, `CloudsInspector.js`, `CloudLayerCard.js`, `NativeCloudPanel.js` | same | `EnvironmentInspectorPages.cpp` |
| Fog | `FogInspector.js`, `FogDashboard.css`, `NativeLocalFogPanel.js` | same | `EnvironmentInspectorPages.cpp` |
| Wind | `WindInspector.js`, `WindInstruments.css`, `NativeWindPanel.js` | same | `EnvironmentInspectorPages.cpp` |
| Moon | `MoonInspector.js`, `MoonCards.css`, `MoonInspectorInfographics.js` | same | `EnvironmentInspectorPages.cpp` |
| Stars / celestial | `StarsInspector.js`, `CelestialInspector.js`, `StarCatalogue.js` | same | `EnvironmentInspectorPages.cpp` |
| Lens optics | `LensFlareInspector.js`, `LensOptics.css`, `NativeFlarePanel.js` | same | `EnvironmentInspectorPages.cpp` |
| Environment root | `EnvironmentInspector.js`, `NativeEnvironment.css`, `NativeEnvironmentControls/Panels.js` | same | `EnvironmentInspectorPages.cpp` |
| Vehicle systems | `VehicleSystemsInspector.js`, `VehicleSystems.css` | same | `VehicleInspectorPages.cpp` |
| Tyres / drivetrain | `VehicleComponents`, `Tyre*` modules | same | `VehicleInspectorPages.cpp` |
| Cabin HMI | `Source/Cabin/*`, `Cabin/AppSpecification.js` (47 KB `ScreenSequence`) | same | `VehicleInspectorPages.cpp` |
| Plume / fire | `PyroPlume`, `ParticleWorkspace.js` | same | `EffectInspectorPages.cpp` |
| Weather effects | precipitation, lightning, snow, water modules | same | `EffectInspectorPages.cpp` |
| GI / reflection / quality / display | `InspectorPages.js` (77 KB) render and display sections | same | `RenderInspectorPages.cpp` |
| Diagnostics / telemetry | `DiagnosticsInspector.js`, `HardwareMetrics.js`, `SessionTelemetry.js` | same | `DiagnosticsInspectorPages.cpp` |

⚠️ `InspectorPages.js` is 77 KB and defines per-page control lists (labels, ranges, defaults). Its lists are
extracted into a checked-in contract table by the phase-3 parser, not hand-copied. The plan does not hand-list
hundreds of sliders from unread code.

### 1.4 Viewport bar controls

`EditorControls.js` fills `#viewport-bar`. Its rows are enumerated in phase 3 from the extracted table; the known
groups are display selector, camera/viewpoint, GI and reflection toggles, quality grade, trace resolution, exposure,
shadow and denoiser switches, the tier selector (GTX/RTX/Blinn), and the readout toggle. Each row binds to the real
`SceneRenderer` configuration in phase 4 — never to an inspector copy.

## 2. Method — how "pixel perfect" is proved rather than asserted

```text
CSS rule ──► transcribed token/metric (with the selector quoted in a comment)
         ──► native draw call through ControlKit at 1× logical pixels
         ──► CPU capture at 1.0 / 1.25 / 1.5 / 2.0 scale
         ──► diff against the browser mirror of the same DOM at the same size
         ──► row closes: ✔️ within tolerance · 🚩 approved delta · 🔴 open
```

| Step | Mechanism | Lives in |
|------|-----------|----------|
| Transcribe | One pass per stylesheet; every custom property and every fixed metric gets a named constant plus the CSS selector in its comment | `Engine/Interface/InspectorMetrics.h`, `ControlKitTokens`, `Appearance.toml` |
| Mirror | Static DOM fixtures built from the copied shell plus the 13 stylesheets, served and captured at fixed viewports and DPRs | `Exhibits/EditorMirror/` |
| Capture | Native ImGui command stream rasterised by the existing CPU fixture, same composition as the production workspace | `Exhibits/EverestUi/Code/` |
| Diff | Per-panel geometry bounds, colour sampling at named probes, and a pixel delta map | `Exhibits/EverestUi/Code/ParityDiff.*` |
| Record | One row per demo element and per control, with the three allowed end states | `Exhibits/EverestUi/PARITY-EVEREST.md` |

Tolerance policy: geometry must match exactly at 1× (0 px on every transcribed bound); colour must match at the
token level (identical component values, so the probe difference is 0); anti-aliased edge pixels are compared with a
stated per-panel structural-similarity floor rather than an exact byte match, because ImGui and the browser rasterise
rounded geometry differently. The floor is stated per panel and never loosened to make a run pass.

⚠️ The mirror is a comparison tool, not a substitute for the demo. It cannot prove behaviour where the demo's
JavaScript generates the DOM; those rows are marked as reconstructed and cite the contract table instead.

## 3. Build order inside phase 3

| Step | Work | Depends on | Evidence |
|------|------|------------|----------|
| 3.1 | Tokens, metrics, DM Sans, scale matrix | D-E2 | token table, font metrics capture |
| 3.2 | Column chrome: three columns, tabs, hide-`P`, scene pill | 3.1 | capture |
| 3.3 | Outliner: demo groups (Render, Environment, Forcefield, Objects, Vehicle, Effects, Destruction, Level), enabled/disabled counts, row metadata, spawn menu | 3.2 | capture + interaction checks |
| 3.4 | Inspector host: crumb bar, identity header, category tabs with roving focus | 3.3 | capture + keyboard checks |
| 3.5 | Inspector pages: environment family, then vehicle, then effects, then diagnostics | 3.4 + inspector model | per-page captures |
| 3.6 | Render pages: GI, ReSTIR, reflection, quality, display | 3.5 + D-E4 | captures with named blocked controls |
| 3.7 | Viewport chrome: bar, HUD, inset + legend, overlay dock, notices, bring-up, failure | 3.2 | captures per overlay state |
| 3.8 | Status bar | 3.2 | capture |
| 3.9 | Dedicated studios: fracture, plume, particles, cabin, vehicle, material | 3.5 + 5 | studio captures |
| 3.10 | Parity closure: every row ✔️ or 🚩 | all | `PARITY-EVEREST.md` |

## 4. Interaction and input parity

| Behaviour | Source | Check |
|-----------|--------|-------|
| Hide editor columns (`P`), readout (`H`), slice inset (`J`) | shell dataset attributes | key presses toggle exactly those overlays |
| Category tabs | `InspectorCategories.js` — roving tabindex, ←/→, Home/End | ImGui key injection sequence |
| Slider drag vs scroll ownership | `SliderControl.js` | drag changes the value; wheel over the drawer moves the sheet |
| Drawer and notch arbitration | `NotchMotion` + Control Centre | pointer over an underlying item never starts a sheet drag |
| Selection rebuilds the inspector | `EditorShell.js` | deleting the selected record clears the page without a dangling field pointer |
| Column hide state persists | `.column-tab` dataset | session and project space restore it |

## 5. Files this phase creates or changes

| Path | Action | Purpose |
|------|--------|---------|
| `Engine/Interface/InspectorMetrics.h` | create | Transcribed CSS geometry |
| `Engine/Interface/InspectorCanvas.{h,cpp}` | create | Record-facing drawing surface |
| `Engine/Interface/InspectorPageRegistry.{h,cpp}` | create | Classification to default page |
| `Engine/Interface/StatusBarControls.{h,cpp}` | create | Status items and fills |
| `Engine/Interface/ViewportOverlayControls.{h,cpp}` | create | HUD, inset, notices, bring-up, failure |
| `Engine/Interface/ControlKit.{h,cpp}` | extend | The 🔴 controls of §1.2 |
| `UI/Editor/InspectorPanel.{h,cpp}` | create | Inspector host |
| `UI/Editor/InspectorPages/*.cpp` | create | Five page files |
| `UI/Editor/ViewportPanel.{h,cpp}` | create | Viewport chrome |
| `UI/Editor/StatusBarPanel.{h,cpp}` | create | Footer |
| `UI/Editor/DedicatedRecordStudios.{h,cpp}` | create | Studios, renamed from donor |
| `UI/Editor/EditorWorkspace.{h,cpp}` | extend | Compose the five regions |
| `Frontier/Applications/Editor/FrontierMain.cpp` | extend | Register the new panels in the dockspace |
| `EngineContent/FontArchives/DM Sans/` | create | Fonts plus OFL licence |
| `Exhibits/EverestUi/` | create | Captures, diff, parity checklist |
| `Exhibits/EditorMirror/` | create | Browser mirror fixtures |

## 6. Acceptance

- Every row of §1.1 and §1.2 in a closed state, with a capture link.
- Geometry exact at 1×; capture set at 1.00, 1.25, 1.50, 2.00 scale.
- No literal colour or metric in a draw call; a scan proves it.
- Shipping profile contains no inspector page, canvas, or studio symbol.
- The production composition, not a mock, is what the captures show.
