# UI redesign — workflow, sources, and gates

The single record of how the UI work is done in this checkout. Written once; read instead of re-explaining.

---

## 1. Sources, pinned

| Repo | Branch | Commit | What was taken |
| ---------------------------------------- | ------------------------- | -------------------------------------------- | ---------------------------------------------------- |
| `SultanAladin/Frontier` | `arena/19265b6e-frontier` | `1fef98c8dc1131d9f38637bd6f300843ff05d807` | `Experimental/`, `EngineDocs/AgenticInstuctions/`, `SYSTEM_NOMENCLATURE.md`, `.gitignore` |
| `streamlinkinbox/Frontier-OLD` | `main` | `4cab9133ef01022019f0bf4c71b9b328791306bd` | `SourceImages/General UI/` (9 unique JPGs) |

Nothing from either source has been edited. The viewer files are exactly as they were cloned.

Re-fetch a source without touching this checkout:

```sh
git clone --filter=blob:none --sparse --depth 1 \
    -b arena/19265b6e-frontier https://github.com/SultanAladin/Frontier /tmp/src-sultan
git -C /tmp/src-sultan sparse-checkout set Experimental EngineDocs SYSTEM_NOMENCLATURE.md CLAUDE.md
```

---

## 2. Servers

```sh
python3 Documentation/Prototypes/ServePrototypes.py 8097        # the cards, plus both viewers
python3 Experimental/ComponentViewer/Serve.py 8095              # Component Viewer
python3 Experimental/ComponentViewer/ServeAssetViewer.py 8096   # Asset Viewer
```

All three bind `0.0.0.0` and serve from the repository root. Check with
`ss -ltnp | grep -E '8095|8096|8097'` before assuming they are up.

**Preview every prototype through the server (8097), never from `file://`.** Cards load
`/Documentation/Prototypes/_shared/…` by absolute path; that is how they resolve once integrated, and it
is how the user looks at them. `8097` also serves the two viewers, so one port shows everything.
`_shared/capture.mjs` points at `http://127.0.0.1:8097` for exactly this reason.

---

## 3. The render recipe (headless Chromium, no display)

Chromium is unpacked outside the workspace, so nothing large enters Git.

```sh
mkdir -p /tmp/renderer && cd /tmp/renderer
npm install --no-save playwright-core @sparticuz/chromium
# unpack the three brotli payloads the package ships with
for p in al2023 swiftshader fonts; do
    tar -xf node_modules/@sparticuz/chromium/bin/$p.tar.br -C libs 2>/dev/null || true
done
```

The binary lands at `/tmp/chromium`; its shared libraries live in `/tmp/renderer/libs` and
`/tmp/renderer/libs/lib`. Every run needs both variables:

```sh
LD_LIBRARY_PATH=/tmp/renderer/libs:/tmp/renderer/libs/lib \
FONTCONFIG_PATH=/tmp/renderer/libs node capture.mjs
```

The capture script is committed, not scratch: `Documentation/Prototypes/_shared/capture.mjs`. It needs the
sandbox install, so point it at it if `playwright-core` is not on the path:

```sh
LD_LIBRARY_PATH=/tmp/renderer/libs:/tmp/renderer/libs/lib FONTCONFIG_PATH=/tmp/renderer/libs \
PLAYWRIGHT_HOME=/tmp/renderer/node_modules CAPTURE_OUT=/tmp/renderer/proto \
node Documentation/Prototypes/_shared/capture.mjs
```

It renders:

- `placement-frame-01..03` — the Placement Frame card, default, engaged, and warning states
- `lattice-reach-01..02` — the Lattice Reach card, default and swept states
- `viewer-01..02` — the live Asset Viewer at `8096`, home and open Add menu

Pitfalls that cost time before:

- `@sparticuz/chromium` is CommonJS: `const { default: pkg } = require('@sparticuz/chromium')`.
- Never `pkill -f /tmp/chromium` (it kills its own shell); use `pkill -x chromium`.
- In the live viewers, scope to `#Studio > .dock.right`; a bare `.dock.right` matches twice.
- Loading the Component Viewer's 25-part vehicle under software GL takes minutes. Use
  `waitUntil: 'domcontentloaded'`, short selector timeouts, and test single cards in the Asset Viewer
  or in standalone HTML instead.
- `deviceScaleFactor: 2` gives legible text for inspection; 1.5 keeps renders small enough to commit.

---

## 4. The visual-audit rule

A card is done when its PNG has been opened and looked at. The audit covers, at minimum:

| Check | Failing sign |
| ------------------------------------ | ------------------------------------------------------------------ |
| Numerals | clipped digit, clipped unit, `.0` on a whole number, `NaN` |
| Glyphs | tofu boxes, missing icon, a glyph that reads as a different object |
| Accent discipline | two competing accents inside one group, an orange field |
| Rhythm | two controls at different heights in one row, dead space in a tile |
| Contrast | caption lost on the grid, hairline invisible, label under a handle |
| Focus | default browser outline on a custom control |
| Overflow | anything past the card edge, horizontal scrollbar |

Record the findings — fixed and outstanding — in the turn reply, and keep the accepted renders under
`Documentation/Prototypes/<Name>/renders/`.

---

## 5. The gates

| Gate | Meaning | Who opens it |
| --------------------------- | ---------------------------------------------------------------- | ------------ |
| Naming | a proposed name may be used in a prototype | the user, in chat |
| Card | a built card may be integrated into a viewer | the user, after seeing the render |
| Integration | which viewer, which slot, which order | the user |

A prototype may be built and rendered freely at any time. Only the second and third gates need waiting.

---

## 6. Composing a card

Borrowed from the reference set, not from any engine:

- **Hero tiles** — two or three at the top, one large numeral each, split integer and muted decimal, a
  quiet caption underneath, a single accent per tile.
- **Polar controls** — concentric rings, each with its own zero tick and its own colour value; dragging
  near a ring edits that ring only. Good for facing, tilt, roll, or any angular triple.
- **Field controls** — a dot grid or a dense square where hovering a cell reads one tile. Good for
  cluster density, coverage, or heat.
- **Sweep controls** — a histogram or sparkline with two draggable edges, the selected span reading as
  count plus share. Good for ranges, cuts, and budgets.
- **Dot rows** — a row of round cells that fill up to a level; the label names the level in words.
- **Drag rows** — a list where the grip itself is the affordance, with a dashed slot to add one more.
- **Callouts** — a tinted strip with a 2 px vertical bar, one sentence of consequence, and an inline
  action that fixes it on the spot.

Never: a bare slider as the only control, a row of five identical steppers, a card whose body is prose,
a footnote that disclaims the card.

### Keep a card small

The first two cards were built too heavy and were cut back. The rule that came out of it:

| Keep | Drop |
| ----------------------------------------------- | ------------------------------------------------------ |
| One or two controls, everything else read-only | a control that repeats a readout |
| One accent per section | a second hue inside the same group |
| A single sentence of consequence | a paragraph explaining the card |
| Symmetric captions on a ranged control | a label that implies the axis starts or ends at the mark |

If a card's height passes roughly 900 px in one column, something in it is decoration.

---

## 7. Naming

Proposals are collected in `Documentation/NAMING_LEDGER.md` before they touch a prototype. Each entry
names the mechanism, not the engine concept it replaces, and passes `SKILL-Naming.md`: no banned words,
no `Manager`/`Handler`/`Controller`/`Service`, no `Is`/`Has`/`Can` prefixes, PascalCase on code names.

---

## 8. What is still in the viewers today

The live viewers still carry the old surface: a single long right dock, engine vocabulary throughout,
32 px capsule sliders, five hundred pixels of empty preview tiles, and tofu glyphs in the Add menu.
`Documentation/Baseline/AssetViewer/` holds the renders that record it. Each card replaces one slab of
that dock, one approved card at a time.
