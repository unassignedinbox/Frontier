# P01 Interaction Evidence

- Harness: `Exhibits/P01Cards/P01InteractionHarness.mjs` (jsdom, stubbed 2D context, bounded rAF).
- Date (UTC): 2026-11-16. Result: **9/9 checks passed**.
- Rendered-pixel capture: BLOCKED — no headless browser in sandbox. Visual check = live preview + user approval vote.

| Check | Verdict | Observed |
|---|---|---|
| page boots 6 cards | PASS | 6 cards present |
| StrataPlot tier select S4 | PASS | S4 · 52 tris · 1.90% · bar 72% |
| Surface-optics slot chips toggle | PASS | 4 lit, untoggle to 3 |
| PolarDial joint steppers ±2° | PASS | 0° -> +4° -> +2° |
| PolarDial sweep toggles drive label | PASS | sweep engaged + released |
| DiscRow gear select | PASS | gear 4 |
| RampButton throttle hold ramps | PASS | ramped to 28%, released to 0% |
| Compile assembly seals record | PASS | revision 25 sealed |
| Filter pills isolate families | PASS | eval 1 · assembly 2 · all 6 |
