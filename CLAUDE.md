# Frontier — workspace standing instructions

Persistent context for every session in this checkout. Read this first, then the three references it points at.
Written once so the directions never have to be repeated in chat.

---

## 🔴 Read these every session, before naming, formatting, or output

| File | What it governs |
| ------------------------------------------------ | ---------------------------------------------------------- |
| `EngineDocs/AgenticInstuctions/SKILL-Naming.md` | ALL naming: banned words + how a name is built |
| `EngineDocs/AgenticInstuctions/SKILL-formatting.md` | file and source formatting |
| `EngineDocs/AgenticInstuctions/SKILL-Emoji.md` | the only approved emojis and where each is used |
| `EngineDocs/AgenticInstuctions/SKILL-CompactOutputProtocol.md` | output shape: tables and diagrams over prose |
| `SYSTEM_NOMENCLATURE.md` | the project's own vocabulary registers |

These are authoritative and override any default habit.

---

## 🔴 The UI redesign rules — non-negotiable

1. **This is not Unreal Engine.** No borrowed engine vocabulary, no borrowed engine layouts, no copied
   inspector patterns. Not Unreal, not Unity, not Godot, not any other engine.
2. **Every name must be new, creative, and derived from what the mechanism actually does.** The user
   approves names **before** any of them lands in a viewer. Proposals go in `Documentation/NAMING_LEDGER.md`.
3. **One prototype at a time, standalone.** A card is built as its own HTML page under
   `Documentation/Prototypes/<Name>/index.html`, rendered, visually inspected, then shown to the user.
   Integration into `Experimental/ComponentViewer/` happens **only after explicit approval** in chat.
4. **Cards are interactive infographics**, not rows of sliders. Allowed: polar rings, dot fields, budget
   arcs, draggable sweeps, hatch-versus-solid deltas, dot rows, drag-to-reorder rows, fan cards.
   A plain slider is the fallback, never the starting point. No slider may be the only control.
5. **Small cards.** One or two controls per card; everything else read-only. If a card needs a paragraph
   to explain itself, or two hues in one section, it is too heavy.
6. **Design language comes from `SourceImages/General UI/`** — matte obsidian surface, 18–22 px bento
   tiles, hairline inner strokes, weight-300 tabular numerals with a muted decimal and a small unit,
   one calm accent per group, tinted diagnostic callouts with a vertical bar.
7. **Always render, always look.** Every prototype and every viewer state must be rendered to PNG with
   the headless recipe and inspected with `read_file` before it is called done.

---

## 🔴 Autonomy limits — ask, do not act

| Action | Rule |
| -------------------------------------------------- | ---------------------------------------------------------- |
| Integrate a prototype into a viewer | 🔴 ASK first. Approval in chat is the only trigger. |
| Change wording, names, or icons in `Experimental/` | 🔴 ASK first. Names need approval on their own. |
| Delete or restructure anything under `Experimental/` | 🔴 ASK first. |
| Add a new prototype card | 🟢 allowed in `Documentation/Prototypes/`, one per turn, reported after. |
| Render a page to PNG for inspection | 🟢 allowed; this is the required visual audit. |
| Write docs, ledgers, or workflow notes | 🟢 allowed under `Documentation/` only. |

`EngineDocs/` stays as it is: `FolderStructure.md`, `Backlog.md`, `AgenticInstuctions/`. Nothing else goes in.

---

## Where the work lives

| Path | Contents |
| ------------------------------------------------- | ---------------------------------------------------------- |
| `Experimental/ComponentViewer/` | the two prototypes served live: Component Viewer and Asset Viewer |
| `Experimental/` (rest) | untouched ported subsystems, not modified without approval |
| `Documentation/Prototypes/` | one folder per standalone card prototype plus its renders |
| `Documentation/Prototypes/_shared/` | shared base stylesheet and bundled DM Sans faces (no network) |
| `Documentation/Baseline/` | rendered screenshots of the live viewers, the visual audit record |
| `Documentation/UI_REDESIGN_WORKFLOW.md` | sources, commits, server commands, render recipe, gates |
| `Documentation/NAMING_LEDGER.md` | every proposed name, its status, and the approved replacement |
| `SourceImages/General UI/` | the inspiration set the design language is read from |

---

## Repositories, pinned

| Repo | Branch | Commit | Role |
| ---------------------------------------- | ------------------------- | -------------------------------------------- | ----------------------- |
| `SultanAladin/Frontier` | `arena/19265b6e-frontier` | `1fef98c8dc1131d9f38637bd6f300843ff05d807` | prototype source |
| `streamlinkinbox/Frontier-OLD` | `main` | `4cab9133ef01022019f0bf4c71b9b328791306bd` | UI inspiration images |

---

## Servers

```sh
python3 Documentation/Prototypes/ServePrototypes.py 8097    # the cards, plus both viewers
python3 Experimental/ComponentViewer/Serve.py 8095          # Component Viewer
python3 Experimental/ComponentViewer/ServeAssetViewer.py 8096   # Asset Viewer
```

All bind `0.0.0.0`. Re-run them from the repository root whenever they are not listed by `ss -ltnp`.

**Look at prototypes through the server, never from `file://`.** `8097` is the one to hand the user: it
lists the cards and also serves both viewers.

---

## The loop, in order

1. Read the four skills and the nomenclature file.
2. Look at the current viewer state (render it; never describe from memory).
3. Pick the next card from `Documentation/Prototypes/README.md`.
4. Propose names in `Documentation/NAMING_LEDGER.md`, and ask before using them.
5. Build one standalone card, render it in every state, fix what the render exposes.
6. Show the render and the mechanics. Wait for approval.
7. Integrate only that approved card. Re-render the viewer, and record the new baseline.
