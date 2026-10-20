# DraftingViewportValidation

Viewport of DraftingWorkspace on SwiftShader Device (Subzero) · Vulkan 1.3.0 — specimen: 11 faces, 24 edges, 1332 triangles; 960 × 640; slowest frame 94.5 ms (14.2 s total).

| Gate | Subject | Limit | Worst | Checks | Verdict |
|------|---------|-------|-------|--------|---------|
| V1 | Vulkan 1.3 device, SPIR-V modules from the Slang archive, three pipelines | created | 0 | 1 | 🟢 |
| V2 | Face pixels away from edges: GPU identity = CPU ray pick (7 views × perspective / orthographic) | mismatch ≤ 0.1 % | 0 | 14 | 🟢 |
| V3 | Edge-stroke pixels: the GPU edge passes within the stroke reach of the pixel ray and is not hidden behind the CPU face hit (orthographic) | explained ≥ 99.5 % | 0 | 1 | 🟢 |
| V4 | Single-texel identity probe (hover path) = full readback = CPU pick | all equal | 0 | 5 | 🟢 |
| V5 | Selected face blue (#4099ff), active edge green (#33ff73), hovered face lighter; others unchanged | all hold | 0.7207 | 4 | 🟢 |
| V6 | View presets: centre pixel names the expected face (Top → top cap, Front → front side, …) | all equal | 0 | 8 | 🟢 |
| V7 | Same frame twice → identical colour and identity digests | equal | 0 | 1 | 🟢 |

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
| V7 | colour 98dff767c0c745d2 · identity edc87526f403b936 | 0 | 🟢 |
