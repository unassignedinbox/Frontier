# 🧩 SolidArc responsiveness — the side-cylinder union scene

**TL;DR** — The reported lag scene (a cylinder r 1.7 h 0.6, a cylinder r 0.35 h 0.3 drawn on its side and joined, a rim chamfer, four moves) reproduces as `Scripts/ResponsivenessWalkthrough.txt` on `Code/Documents/SideCylinderUnion.solidarc`. The frame profiler names the cost: every snapped step of a move drag cloned the document, replayed the Transform step, integrated the volume and tessellated again (≈ 45 ms on one frame). Move / rotate / scale previews of whole bodies are now rigid (the shown triangles carried by the map, < 1 ms); the kernel runs once, on release.

## 🔍 Where a frame went (SwiftShader host, CPU interface time per frame)

| Phase | Before | After |
|---|---|---|
| Gizmo drag, frame that stages a new value | 26 – 45 ms | < 1 ms (`gizmo preview · rigid`) |
| Gizmo drag, all 70 drag frames | median 1, max 45 ms | median 1, max 5 ms |
| G move preview | full regeneration per value | rigid, `transform-rigid == 1` |
| Bevel / chamfer preview per new size | 11 – 39 ms | unchanged (🚧 async preview) |
| Idle frame (interface) | 0 – 2 ms | 0 – 2 ms |

| Old staged move preview (one value) | ms |
|---|---|
| Transform step replay | 7.8 |
| Volume integration | 8.4 |
| Rest of the document regeneration | 9.1 |
| Tessellation (728 triangles) | 18.9 |
| **Total** | **44.8** |

## 📝 Kernel figures (lag harness, same scene)

- Extrude ≈ 11 ms, Union ≈ 77 ms, rim chamfer 28 – 35 ms, scene build ≈ 9 ms, move commit ≈ 16 ms: no command is slow.
- The face sketch on the curved side sits on the tessellation hit (≤ 0.5 mm inside the exact cylinder): the Union gives a sliver contact (4 tiny junction edges); a chamfer on the junction edge is refused (unequal mitre). An exactly tangent plane would make the outward Union a line contact, refused as non-manifold. ⚠️ Open decision.

## 💡 Logging

- `SolidArcTrace.log` next to the exe (previous run: `SolidArcTrace.previous.log`); the status-bar **Log** button or `F12` opens the panel (path, Copy path, Slow frames / Refusals filters, Copy).
- A frame over 50 ms writes `Frame · slow frame N · T ms (CPU · interface · upload · GPU wait) · session · triangles · steps · costliest phases`, at most 4 lines a second (the rest counted on the next line).
- `SolidArcCapture --trace <file> --slow-frame <ms>` writes the same trace from a script (every frame over the threshold, no rate limit).
