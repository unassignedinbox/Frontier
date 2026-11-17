# P02 Interaction Evidence

- Harness: `Exhibits/P02Cards/P02InteractionHarness.mjs` (jsdom, stubbed 2D context, bounded rAF).
- Date (UTC): 2026-11-16. Result: **13/13 checks passed**.
- Rendered-pixel capture: BLOCKED — no headless browser in sandbox. Visual check = live preview + user approval vote.

| Check | Verdict | Observed |
|---|---|---|
| page boots 4 cards + compile strip | PASS | 4 cards + strip present |
| tier rail S4 retunes both cards | PASS | S4 · 52 tris · wrong 1.90% · complex follows |
| tier rail arrow key steps back | PASS | ArrowUp S4 -> S3 |
| history scrubber drags | PASS | T-98 · 26.3k tris |
| dial needle drags to limit | PASS | drag -> 32.0° · 1.2 kN |
| dial double-tap centres | PASS | 0.0° |
| sweep toggles drive label | PASS | sweep on/off |
| throttle strip pull sets value | PASS | pull -> 80% |
| gear DiscRow selects | PASS | gear 4 |
| card headers fold/unfold | PASS | fold/unfold ok |
| optics chips retune count | PASS | 4 of 4 lit |
| compile strip seals revision 25 | PASS | revision 25 |
| filter pills isolate families | PASS | eval 1 · asset 2 · all 4 |
