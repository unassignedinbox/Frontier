# Frontier — agent instructions (mirror of CLAUDE.md)

The same standing instructions, kept in the file agents that are not Claude read first. If the two ever
disagree, treat `CLAUDE.md` as canonical and fix this file to match it.

## Read before anything else

- `EngineDocs/AgenticInstuctions/SKILL-Naming.md`
- `EngineDocs/AgenticInstuctions/SKILL-formatting.md`
- `EngineDocs/AgenticInstuctions/SKILL-Emoji.md`
- `EngineDocs/AgenticInstuctions/SKILL-CompactOutputProtocol.md`
- `SYSTEM_NOMENCLATURE.md`

## The rules that define this work

1. Not Unreal Engine, and not any other engine. No borrowed vocabulary, layout, or inspector pattern.
2. Every name is new, creative, and mechanism-derived, and **the user approves it** before use.
3. One prototype at a time, standalone in `Documentation/Prototypes/<Name>/`, integrated only after
   explicit approval in chat.
4. Cards are interactive infographics: polar rings, dot fields, budget arcs, draggable sweeps, dot rows,
   drag-to-reorder rows. A slider is a fallback, never the only control in a card.
5. Keep a card small: one or two controls, everything else read-only.
6. Preview through `Documentation/Prototypes/ServePrototypes.py` on port 8097, never from `file://`.
7. The visual language comes from `SourceImages/General UI/`: obsidian surface, 18–22 px bento tiles,
   hairline strokes, weight-300 tabular numerals with muted decimals, one calm accent per group.
8. Render every card and every viewer state to PNG and inspect it with `read_file` before calling it done.

## Ask first

Integrating a prototype into a viewer, changing any wording or icon under `Experimental/`, deleting or
restructuring `Experimental/`. Writing a new card prototype, rendering it, and writing docs do not need
permission.

## Everything else

Sources, commits, server commands, the render recipe, the naming ledger, and the card backlog all live in
`Documentation/` — start at `Documentation/UI_REDESIGN_WORKFLOW.md`.
