# SolidArcViewportValidation

Viewport of SolidArc on SwiftShader Device (Subzero) · Vulkan 1.3.0 — specimen: 11 faces, 24 edges, 1332 triangles; 960 × 640; slowest frame 104.8 ms (16.3 s total).

| Gate | Subject | Limit | Worst | Checks | Verdict |
|------|---------|-------|-------|--------|---------|
| V1 | Vulkan 1.3 device, SPIR-V modules from the Slang archive, seven pipelines (opaque, x-ray depth / ghost / occluded strokes, edges, grid, image planes) | created | 0 | 1 | 🟢 |
| V2 | Face pixels away from edges: GPU identity = CPU ray pick (7 views × perspective / orthographic) | mismatch ≤ 0.1 % | 0 | 14 | 🟢 |
| V3 | Edge-stroke pixels: the GPU edge passes within the stroke reach of the pixel ray and is not hidden behind the CPU face hit (orthographic) | explained ≥ 99.5 % | 0 | 1 | 🟢 |
| V4 | Single-texel identity probe (hover path) = full readback = CPU pick | all equal | 0 | 5 | 🟢 |
| V5 | Selected face blue (#4099ff), active edge green (#33ff73), hovered face lighter; others unchanged | all hold | 0.7207 | 4 | 🟢 |
| V6 | View presets: centre pixel names the expected face (Top → top cap, Front → front side, …) | all equal | 0 | 8 | 🟢 |
| V7 | Same frame twice → identical colour and identity digests | equal | 0 | 1 | 🟢 |
| V8 | Matcap layers: planar faces = the bilinear texel at their view-space normal (body + 2 face overrides); Studio shader = CPU mirror; clearing restores the Studio frame bit-exact | ≤ 2 / 255 | 0.5139 | 7 | 🟢 |
| V9 | X-ray: ghost faces = α · opaque + (1 − α) · background; every revealed stroke pixel is a CPU-hidden edge; no other identity changes; rim spill; probe and CPU pick through; box visibility = GPU edges; off → bit-identical | ≤ 2 / 255 · ≥ 99.5 % · 0 · ≤ 2 % · equal | 1.347 | 11 | 🟢 |
| V10 | Section (Alt+S): interior pixels = CPU pick with the clip (removed side gone, caps where the material is cut); every cap pixel within 1 px of a CPU cap; probe; grid never through a cap; off → bit-identical | ≤ 0.1 % · all · equal | 0 | 18 | 🟢 |

## Walkthrough

| Gate | Check | Value | Verdict |
|------|-------|-------|---------|
| V1 | SwiftShader Device (Subzero) · Vulkan 1.3.0 | 0 | 🟢 |
| V2 | Isometric persp: 0 / 65116 sampled pixels differ (%) | 0 | 🟢 |
| V2 | Top persp: 0 / 65512 sampled pixels differ (%) | 0 | 🟢 |
| V6 | Top persp centre → Extrude#2.Cap(End) | 0 | 🟢 |
| V2 | Bottom persp: 0 / 66068 sampled pixels differ (%) | 0 | 🟢 |
| V6 | Bottom persp centre → Extrude#2.Cap(Start) | 0 | 🟢 |
| V2 | Front persp: 0 / 66386 sampled pixels differ (%) | 0 | 🟢 |
| V6 | Front persp centre → Extrude#2.Side(Line1) | 0 | 🟢 |
| V2 | Back persp: 0 / 66350 sampled pixels differ (%) | 0 | 🟢 |
| V6 | Back persp centre → Extrude#2.Side(Line3) | 0 | 🟢 |
| V2 | Right persp: 0 / 66301 sampled pixels differ (%) | 0 | 🟢 |
| V2 | Left persp: 0 / 66327 sampled pixels differ (%) | 0 | 🟢 |
| V2 | Isometric ortho: 0 / 65330 sampled pixels differ (%) | 0 | 🟢 |
| V2 | Top ortho: 0 / 65740 sampled pixels differ (%) | 0 | 🟢 |
| V6 | Top ortho centre → Extrude#2.Cap(End) | 0 | 🟢 |
| V2 | Bottom ortho: 0 / 66292 sampled pixels differ (%) | 0 | 🟢 |
| V6 | Bottom ortho centre → Extrude#2.Cap(Start) | 0 | 🟢 |
| V2 | Front ortho: 0 / 66492 sampled pixels differ (%) | 0 | 🟢 |
| V6 | Front ortho centre → Extrude#2.Side(Line1) | 0 | 🟢 |
| V2 | Back ortho: 0 / 66544 sampled pixels differ (%) | 0 | 🟢 |
| V6 | Back ortho centre → Extrude#2.Side(Line3) | 0 | 🟢 |
| V2 | Right ortho: 0 / 66501 sampled pixels differ (%) | 0 | 🟢 |
| V2 | Left ortho: 0 / 66553 sampled pixels differ (%) | 0 | 🟢 |
| V3 | isometric ortho: 4091 / 4091 edge pixels explained (unexplained %) | 0 | 🟢 |
| V4 | (480, 320) → Extrude#2.Cap(End) | 0 | 🟢 |
| V4 | (300, 400) → background | 0 | 🟢 |
| V4 | (700, 260) → background | 0 | 🟢 |
| V4 | (10, 10) → background | 0 | 🟢 |
| V4 | (520, 360) → Extrude#2.Cap(End) | 0 | 🟢 |
| V5 | selected top cap: B − R 0.066667 → 0.462745 | 0.4627 | 🟢 |
| V5 | active fillet edge: mean RGB (0.231759, 0.954158, 0.466995) | 0.7207 | 🟢 |
| V5 | hovered front face brightened by 0.376471 | 0.3765 | 🟢 |
| V5 | unselected right side unchanged (Σ|ΔRGB|) | 5.551e-17 | 🟢 |
| V8 | Extrude#2.Cap(End) → Brass (layer 5): view normal (0.000, 0.816), 54214 pixels within 0.42 / 255 of the texel (RGB 241 202 118) | 0.4248 | 🟢 |
| V8 | Extrude#2.Cap(End) without matcap: Studio shader = CPU Studio mirror within 0.506 / 255 | 0.5058 | 🟢 |
| V8 | Extrude#2.Side(Line2) → RedPlastic (layer 9): view normal (0.707, -0.408), 10315 pixels within 0.00 / 255 of the texel (RGB 79 11 9) | 0 | 🟢 |
| V8 | Extrude#2.Side(Line2) without matcap: Studio shader = CPU Studio mirror within 0.244 / 255 | 0.2442 | 🟢 |
| V8 | Extrude#2.Side(Line1) → BrushedSteel (layer 2): view normal (-0.707, -0.408), 25351 pixels within 0.51 / 255 of the texel (RGB 58 58 61) | 0.5139 | 🟢 |
| V8 | Extrude#2.Side(Line1) without matcap: Studio shader = CPU Studio mirror within 0.244 / 255 | 0.2442 | 🟢 |
| V8 | layers cleared → colour bit-identical to the Studio frame; identity unchanged by matcaps | 0 | 🟢 |
| V9 | isometric ortho: 95461 interior face pixels = α · opaque + (1 − α) · background within 0.736 / 255 (α 0.45) | 0.7358 | 🟢 |
| V9 | isometric ortho: 1708 / 1708 revealed stroke pixels are hidden edges (unexplained %) | 0 | 🟢 |
| V9 | isometric ortho: 1708 pixels changed identity to a hidden edge, 0 changed otherwise | 0 | 🟢 |
| V9 | isometric ortho: 23 / 1708 revealed pixels lie within 2 px of the same edge's visible stroke (%) | 1.347 | 🟢 |
| V9 | isometric ortho: probe (538, 319) → Edge(Extrude#2.Cap(Start)|Extrude#2.Side(Line3)); CPU x-ray pick → Edge(Extrude#2.Cap(Start)|Extrude#2.Side(Line3)); opaque pick → Extrude#2.Cap(End) | 0 | 🟢 |
| V9 | orbited ortho: box encloses 19 visible edges (GPU drew 19, 4 touching by end caps) and 24 / 24 in x-ray; disagreements none | 0 | 🟢 |
| V9 | isometric ortho: x-ray frame repeatable; off → colour and identity bit-identical to the opaque frame | 0 | 🟢 |
| V9 | isometric persp: 95358 interior face pixels = α · opaque + (1 − α) · background within 0.736 / 255 (α 0.45) | 0.7358 | 🟢 |
| V9 | isometric persp: 2013 pixels changed identity to a hidden edge, 0 changed otherwise | 0 | 🟢 |
| V9 | isometric persp: 21 / 2013 revealed pixels lie within 2 px of the same edge's visible stroke (%) | 1.043 | 🟢 |
| V9 | isometric persp: x-ray frame repeatable; off → colour and identity bit-identical to the opaque frame | 0 | 🟢 |
| V10 | ortho y = 1.25 (keep y ≥ 1.25): 0 / 65880 interior pixels differ (2787 on the cap) (%) | 0 | 🟢 |
| V10 | ortho y = 1.25 (keep y ≥ 1.25): 28547 cap pixels, mean colour (0.533, 0.235, 0.179); without a CPU cap within 1 px | 0 | 🟢 |
| V10 | ortho y = 1.25 (keep y ≥ 1.25): probe at (363, 181) → cap | 0 | 🟢 |
| V10 | ortho y = 1.25 (keep y ≥ 1.25): cap pixels changed by switching the grid on | 0 | 🟢 |
| V10 | ortho x = 1 (keep x ≤ 1): 0 / 66391 interior pixels differ (2204 on the cap) (%) | 0 | 🟢 |
| V10 | ortho x = 1 (keep x ≤ 1): 22681 cap pixels, mean colour (0.407, 0.179, 0.137); without a CPU cap within 1 px | 0 | 🟢 |
| V10 | ortho x = 1 (keep x ≤ 1): probe at (437, 180) → cap | 0 | 🟢 |
| V10 | ortho x = 1 (keep x ≤ 1): cap pixels changed by switching the grid on | 0 | 🟢 |
| V10 | ortho: section off → colour and identity bit-identical to the unsectioned frame | 0 | 🟢 |
| V10 | persp y = 1.25 (keep y ≥ 1.25): 0 / 65792 interior pixels differ (2533 on the cap) (%) | 0 | 🟢 |
| V10 | persp y = 1.25 (keep y ≥ 1.25): 26288 cap pixels, mean colour (0.533, 0.235, 0.179); without a CPU cap within 1 px | 0 | 🟢 |
| V10 | persp y = 1.25 (keep y ≥ 1.25): probe at (376, 186) → cap | 0 | 🟢 |
| V10 | persp y = 1.25 (keep y ≥ 1.25): cap pixels changed by switching the grid on | 0 | 🟢 |
| V10 | persp x = 1 (keep x ≤ 1): 0 / 66317 interior pixels differ (2017 on the cap) (%) | 0 | 🟢 |
| V10 | persp x = 1 (keep x ≤ 1): 21592 cap pixels, mean colour (0.408, 0.179, 0.137); without a CPU cap within 1 px | 0 | 🟢 |
| V10 | persp x = 1 (keep x ≤ 1): probe at (428, 186) → cap | 0 | 🟢 |
| V10 | persp x = 1 (keep x ≤ 1): cap pixels changed by switching the grid on | 0 | 🟢 |
| V10 | persp: section off → colour and identity bit-identical to the unsectioned frame | 0 | 🟢 |
| V7 | colour 98dff767c0c745d2 · identity edc87526f403b936 | 0 | 🟢 |
