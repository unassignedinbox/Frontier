# Card prototypes

One folder per card. Each folder holds `index.html`, `README.md` (what it does and what is being asked),
and `renders/` (the accepted renders).

Serve them; do not open them from disk. The pages load the shared stylesheet and fonts by absolute path,
which is how they will resolve once integrated.

```sh
python3 Documentation/Prototypes/ServePrototypes.py 8097
```

| URL | What |
| -------------------------------------------------- | ------------------------------------------ |
| `http://localhost:8097/` | index of the cards |
| `http://localhost:8097/Documentation/Prototypes/PlacementFrame/` | Placement Frame |
| `http://localhost:8097/Documentation/Prototypes/LatticeReach/` | Lattice Reach |
| `http://localhost:8097/Experimental/ComponentViewer/AssetViewer.html` | the live Asset Viewer |
| `http://localhost:8097/Experimental/ComponentViewer/index.html` | the live Component Viewer |

Nothing here is wired into `Experimental/`. Integration is a separate, approved step.

---

## Built and awaiting approval

| Card | Folder | Replaces in the live dock | State |
| -------------------- | ---------------------- | ------------------------------------- | ------------------------- |
| Placement Frame | `PlacementFrame/` | `Transform` + `Physics` slabs | 🚩 render shown, awaiting the word |
| Lattice Reach | `LatticeReach/` | `Static Mesh` + `Geometry` + `Cluster LOD` | 🚩 render shown, awaiting the word |

## Backlog — one at a time, in this order

| # | Card | Replaces | The interactive idea | State |
| -- | ------------------ | ------------------------------------ | ------------------------------------------------------------------ | --------- |
| 1 | Placement Frame | `Transform`, `Physics` | ground pad with a turned footprint, three-ring facing dial, spread bands | 🚩 built |
| 2 | Lattice Reach | `Static Mesh`, `Geometry`, `Cluster LOD` | budget arc, two-edge size sweep | 🚩 built |
| 3 | Surface Finish | `Material channels` (≈ 1 900 px today) | channel fan cards, a colour well that reads one channel at a time | ☐ queued |
| 4 | Mark | `Icon & thumbnail` | crop frame with a scale ring and a plate that previews live | ☐ queued |
| 5 | Unwrap | `UV Layout` | a flat sheet the faces are dragged across, with real coverage | ☐ queued |
| 6 | Distance Haze | `SDF Volume` | concentric shells with a draggable reach and a cost meter | ☐ queued |
| 7 | Split Map | `Fracture` | pieced block that splits on drag, with a piece-count ring | ☐ queued |
| 8 | Hold | `Socket` × 8 types | one coupling row that changes behaviour by dragging its grip | ☐ queued |
| 9 | Staging Rack | `Asset Browser` drawer | fanned category folders, drag-to-place, no flat grid of cubes | ☐ queued |
| 10 | Add Rail | the Add menu | four grouped shelves, large hit targets, icons drawn in SVG | ☐ queued |

Each card is built only when the previous one is approved or dropped. A card's names go to
`Documentation/NAMING_LEDGER.md` first.

---

## What every card must hold to

- no engine vocabulary in any visible string
- one calm accent per group; the accent is a single hue, never a rainbow
- weight-300 tabular numerals with a muted decimal and a small unit
- at least one interactive infographic control, and no card whose only control is a slider
- no prose blocks inside a card body; one sentence of consequence at most, in a callout
- no external fonts, scripts, or images; everything comes from `_shared/` on the prototype server
- small on purpose: one or two controls per card, everything else read-only
- rendered and inspected in every reachable state before it is shown

---

## Shared base

`_shared/prototype.css` carries the three DM Sans faces and the page reset. The woff2 files are bundled
beside it (`_shared/fonts/`, OFL). Cards link it with a relative path so they work from disk.
