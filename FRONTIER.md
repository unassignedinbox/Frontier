# FRONTIER — Standing Directives (save: do not make the user repeat these)

Source: user direction 2026-11-16 + Frontier-OLD agent skills. Every agent session on this repo
MUST follow this file. Repo live branch for viewer work: `arena/a99371c6-frontier`.

## 1. What lives here
- `Experimental/ComponentViewer/` — Component Viewer prototype (cloned from
  `SultanAladin/Frontier @ arena/19265b6e-frontier`, path `Experimental/ComponentViewer`).
- `Experimental/ComponentViewer/AssetViewer.html` — Asset Previewer prototype (same source).
- Support code cloned alongside (required at runtime): `Experimental/Shared`,
  `Experimental/GeometryCompiler`, `Experimental/FractureEditor`,
  `Experimental/FrontierWorldMirror`, `Experimental/StudioIconSet`.
- `SourceImages/General UI/` — UI inspiration images (cloned from
  `streamlinkinbox/Frontier-OLD @ main`, path `SourceImages/General UI`). 10 files.
  This is the ONLY approved visual reference for redesigns.
- `EngineDocs/AgenticInstuctions/SKILL-*.md`, `CLAUDE.md`, `AGENTS.md`,
  `Documentation/Rules/Naming.md`, `Documentation/FolderStructure.md` — agent skills
  cloned from Frontier-OLD @ main. Authoritative; see §3.

## 2. Servers (always run both so the user can see the work)
- Component Viewer: `python3 Experimental/ComponentViewer/Serve.py 8095`
  → `/` redirects to `/Experimental/ComponentViewer/`
- Asset Previewer: `python3 Experimental/ComponentViewer/ServeAssetViewer.py 8096`
  → `/` redirects to `/Experimental/ComponentViewer/AssetViewer.html`
- Prototype cards: same servers, e.g.
  `/Experimental/Prototypes/P01-InfographicCards.html` on port 8095.
- Bind `0.0.0.0`, never `127.0.0.1` (Arena live preview).

## 3. Naming law (SKILL-Naming is authoritative)
- `EngineDocs/AgenticInstuctions/SKILL-Naming.md` is the SINGLE naming authority.
  Run its §0 construction procedure (mechanism sentence → verb+substrate fusion →
  domain register → 6-gate reject test) BEFORE proposing any name. Never hand a
  naming problem back to the user without running it first.
- This is NOT Unreal Engine. Gate 3 + user direction bar Unreal/Unity/Godot/other
  borrowings outright: `Blueprint, Actor, Pawn, Static Mesh, Skeletal Mesh, Content
  Browser, Outliner, Details, Construction Script, Event Graph, GameObject, Prefab,
  Inspector (Unity sense — see below), Hierarchy, Scene, Node, SceneTree`.
- Frontier-OLD rulings that OVERRIDE first instincts (do not "creatively" rename these):
  `Compile` (approved verb, §6) · `ViewportPanel` (approved panel, §10) · `Origin`
  (§2) · `ContentDirectory` (mandated by CLAUDE.md) · `SceneDirectoryPanel` (the
  outliner, §10) · `RecordEntry` + modular `*Configuration` + matching `*Inspector`
  classes (§1) · `mesh` → `PolygonComplex`/`PolygonSurface`/`DisplayPolygons` (§2) ·
  `Cache` BANNED → `Atlas`/`Store`/`Table` (§1) · controls use authorized nouns
  (`ArcPlot, PolarDial, StrataPlot, CurvePlot, SpectrumBar, DiscRow, TimeSeriesPlot`).
- All user-approved replacement names are recorded in `Documentation/Tracker.md`.
  New names may appear ONLY inside `Experimental/Prototypes/Pxx-*` sandbox pages until the user
  approves them; only then wire ONE approved prototype into the real viewers.
- Conflicts between user taste and SKILL-Naming are REPORTED (table + options), never
  silently resolved.

## 4. Prototype workflow (one at a time, approval-gated)
1. Each redesign is ONE standalone page in `Experimental/Prototypes/Pxx-<Name>.html` (self-contained HTML+CSS+JS). Number sequentially.
2. Verify before presenting: JS syntax check + serve check + interaction evidence
   harness under `Exhibits/Pxx*/` (generating code + report together, per CLAUDE.md).
   Rendered-pixel capture is BLOCKED in this sandbox (no headless browser) — mark it
   blocked, never passing; the live preview + user vote is the visual check.
3. Present with the live preview URL. WAIT for explicit approval.
4. ONLY after approval: integrate that ONE prototype into the viewers.
5. Log every prototype and its status in `Documentation/Tracker.md` (single tracker).

## 5. Card design law
- Cards are INTERACTIVE INFOGRAPHICS first (telemetry numerals, rings, gauges,
  sparklines, stepped plots, live dials). Plain sliders are BANNED as the primary
  control — use authorized controls (`ArcPlot, PolarDial, StrataPlot, CurvePlot,
  DiscRow, TimeSeriesPlot`) or a constructed equivalent (mechanism-named).
- Visual language comes ONLY from `SourceImages/General UI/`: near-black ground
  `#0b0e0b`, floating charcoal cards, radius 16–24px, thin giant numerals, small
  muted captions, lime `#d6f26a` primary, amber `#ffb224` warn, coral `#ff5a4e`
  alert, teal `#46d39a` ok/telemetry, pill tabs + pill buttons, status dot + label,
  nested rows with left rules, red count badges on alert panels.

## 6. Output law (SKILL-CompactOutputProtocol + SKILL-Emoji)
- Order: TL;DR (1–3 sentences) → table → diagram → bullets → short paragraph.
- Comparing options or asking a choice: comparison table FIRST, then the question
  directly beneath. Prose-only choices are a rule violation.
- Emoji: ONLY the SKILL-Emoji set (✔️ 🚩 🔴 ⭐ 🟢 🏷️ 📝 💡 ⚠️ 🔍 🧩 🐞🐛 🚧 🏁 …).
  Never 🌟 ✨, never any other dot colour, never unlisted emoji.

## 7. Standing bans for this branch
- Never switch/create/push any branch except `arena/a99371c6-frontier`.
- Same-file edits: NEVER batch parallel `edit_file` calls on one file (they clobber —
  last write wins). Use one atomic patch (python via bash) or full `write_file`.
