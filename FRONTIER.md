# FRONTIER — Standing Directives (save: do not make the user repeat these)

Source: user direction 2026-11-16. Every agent session on this repo MUST follow this file.
Repo live branch for viewer work: `arena/a99371c6-frontier` (base: `8fd0896a` on main).

## 1. What lives here
- `Experimental/ComponentViewer/` — Component Viewer prototype (cloned from
  `SultanAladin/Frontier @ arena/19265b6e-frontier`, path `Experimental/ComponentViewer`).
- `Experimental/ComponentViewer/AssetViewer.html` — Asset Previewer prototype (same source).
- Support code cloned alongside (required at runtime): `Experimental/Shared`,
  `Experimental/GeometryCompiler`, `Experimental/FractureEditor`,
  `Experimental/FrontierWorldMirror`, `Experimental/StudioIconSet`.
- `SourceImages/General UI/` — UI inspiration images (cloned from
  `streamlinkinbox/Frontier-OLD @ main`, path `SourceImages/General UI`).
  10 files. This is the ONLY approved visual reference for redesigns.

## 2. Servers (always run both so the user can see the work)
- Component Viewer: `python3 Experimental/ComponentViewer/Serve.py 8095`
  → `http://0.0.0.0:8095/` redirects to `/Experimental/ComponentViewer/`
- Asset Previewer: `python3 Experimental/ComponentViewer/ServeAssetViewer.py 8096`
  → `http://0.0.0.0:8096/` redirects to `/Experimental/ComponentViewer/AssetViewer.html`
- Prototype cards: served by the same servers, e.g.
  `http://0.0.0.0:8095/Experimental/ComponentViewer/Prototypes/P01-InfographicCards.html`
- Bind `0.0.0.0`, never `127.0.0.1` (Arena live preview).

## 3. Naming law (STRICT)
- This is NOT Unreal Engine. NEVER copy Unreal, Unity, Godot, CryEngine, Blender,
  or any other engine's names, terms, layouts, or concepts 1:1.
- BANNED words in any user-facing UI the agent creates or renames:
  `Blueprint, Actor, Pawn, Static Mesh, Skeletal Mesh, Content Browser, Outliner,
  Details, Viewport, Construction Script, Event Graph, Simulate, Compile (as action),
  GameObject, Prefab, Inspector, Hierarchy, Scene, Project, Node, SceneTree,
  Game View, Play Mode`.
- All replacement names must be UNIQUE and CREATIVE to Frontier, and the USER MUST
  APPROVE each name before it is wired into the Component Viewer / Asset Previewer.
- Until approved, new names may appear ONLY inside `Prototypes/Pxx-*` sandbox pages.

## 4. Prototype workflow (one at a time, approval-gated)
1. Each redesign is ONE standalone prototype page in
   `Experimental/ComponentViewer/Prototypes/Pxx-<Name>.html` (self-contained
   HTML+CSS+JS, no dependency on viewer code). Number sequentially (P01, P02, …).
2. Prototype, then RENDER it: serve it, open it via the server URL, and look at
   the actual rendered result (read the file back / screenshot path) before
   presenting — never present blind markup.
3. Present to the user with the live preview URL. WAIT for explicit approval.
4. ONLY after approval: integrate that ONE prototype's design + names into the
   real Component Viewer / Asset Previewer.
5. Log every prototype and its status in the table below.

## 5. Card design law
- Cards are INTERACTIVE INFOGRAPHICS first (telemetry numerals, rings, gauges,
  sparklines, stepped strata, live dials). Plain sliders are BANNED as the
  primary control — replace with dials, steppers, segmented pills, press-pads,
  draggable chart points, or another unique control.
- Visual language comes ONLY from `SourceImages/General UI/`:
  near-black ground `#0b0e0b`, floating charcoal cards `~#15191500`,
  radius 16–24px, thin 300–400 weight giant numerals, small muted captions,
  lime `#d6f26a` primary accent, amber `#ffb224` warn, coral `#ff5a4e` alert,
  teal `#46d39a` ok/telemetry, pill tabs + pill buttons, status dot + label,
  nested rows with left rules, red count badges on alert panels.

## 6. Prototype log
| ID | Title | Status | Approved names used | Integrated into |
|----|-------|--------|---------------------|-----------------|
| P01 | Infographic Cards (Hull / Strata / Hitch / Shakedown) | PENDING USER APPROVAL | none yet (proposed set P01-A…D, awaiting vote) | — |
