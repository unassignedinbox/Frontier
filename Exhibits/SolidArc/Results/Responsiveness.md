# 🧩 SolidArc responsiveness — a cylinder joined on a cylinder's side

**TL;DR** — The reported scene is now drawn by `Scripts/ResponsivenessWalkthrough.txt` (no fixture): a cylinder r 1.7 h 0.6, a circle r 0.2 drawn on its curved side, extruded 0.3 and joined, a chamfer on the junction with the pointer sweeping, a chamfer on the boss rim, four gizmo moves and a G move. The sketch lies on the **exact tangent plane** (surface foot and normal, not the facet hit). The extrude **starts below that plane** by the measured profile depth, so the Union closes: one sound body with the closed-form volume, and the junction takes a chamfer. Slow bevel previews wait for the pointer to rest. A move reuses the volume its own assembly already integrated.

## 🔍 Curved-face sketches (Option C)

| Step | Before | Now |
|---|---|---|
| Sketch plane on a curved face | plane through the tessellation hit (≤ 0.5 mm inside), facet normal | tangent plane at the surface foot of the hit, surface normal (`plane-offset` = 1.7 exactly) |
| Extrude start for a Union | on the plane | `Start = −sink`: sink = deepest rim ray to the face + sag (≥ R − √(R² − r²)) |
| Union result | sliver contact (4 tiny junction edges), or a line contact with an exactly tangent plane | one closed body, 5 faces, V = π R² H + π r² L + cap, ≤ 10⁻¹⁴ |
| Chamfer 0.02 on the junction | refused (unequal mitre) | accepted, sound |
| Construct primitive on the tangent plane | same as the extrude | `Request.Start = −sink`, unites the same way |

| W51 check | Value | Verdict |
|---|---|---|
| Foot of a point 3 mm inside the side | 1.65e-16 | 🟢 |
| Measured sink 0.020799 ≥ rim depth 0.011806, over-reach ≤ 2 · sag + 1 mm | 0.009 | 🟢 |
| Tangent Union: line contact, gap left open · sunk Union V 5.485591014531 = closed form | 2.1e-15 | 🟢 |
| Junction chamfer 0.02 (826 ms) | accepted | 🟢 |
| Codec: `Start` written, byte-equal after reload | 0 | 🟢 |
| 4 moves (205 ms each): reused V = independent integral | 1.3e-13 | 🟢 |
| Construct cylinder on the −y tangent plane, started below it | 1.9e-15 | 🟢 |

⚠️ Limits:
- A profile that overhangs the face (it crosses a cap edge) gets sink 0. The Union then only touches and leaves the gap open. No lip is invented.
- A 0.05 junction blend on r 0.2–0.25 / R 1.7 is refused: neighbouring blends overlap (self-intersection removal is 🚧).
- A boss on the big cylinder's seam line (+x) splits the junction, so its blend is refused (🚧).

## 💡 Slow previews wait for the pointer to rest

| Bevel session, 30-step pointer sweep on the junction | Before | Now |
|---|---|---|
| Previews built during the sweep | 30 (one per step, 0.6 – 4 s each) | 1 |
| Previews after the pointer rests | — | 1 (size held ≥ 0.15 s and ≥ 6 frames) |
| Walkthrough wall time (SwiftShader) | 119 s | 56 s |

A preview that took more than 40 ms is rebuilt only when the size rests. The last preview stays up meanwhile. A typed (pinned) size, a new shape, verb or Boolean, and every fast preview still restage at once. A stale refusal does not block Enter: the kernel decides on the final size. `bevel-stages` counts the previews built in a session.

## 📝 Move commit on a blended body (lag harness, native)

| Body | Before | First move | Later moves |
|---|---|---|---|
| Junction chamfer 0.02 | 400 ms | 300 ms | 180 ms |
| Junction fillet 0.02 (rolling-ball face) | 2 500 ms | 1 780 ms | 800 ms |

- A move's assembly integrates every shell anyway (outer vs cavity). The sum is the body volume, so the summary reuses it. The next move of the same body hands it to its assembly as det M · V. One-shell bodies only.
- The remaining cost is the soundness audit, which still runs (≈ 0.8 s on a rolling-ball face).

## 🔍 Where a frame went (SwiftShader host, CPU interface time per frame)

| Phase | Before | After |
|---|---|---|
| Gizmo drag, frame that stages a new value | 26 – 45 ms | < 1 ms (`gizmo preview · rigid`) |
| Gizmo drag, all drag frames | median 1, max 45 ms | median 1, max 5 ms |
| G move preview | full regeneration per value | rigid, `transform-rigid == 1` |
| Bevel preview, mouse adjust | every new size | when the pointer rests (slow ones) |
| Idle frame (interface) | 0 – 2 ms | 0 – 2 ms |

## 📝 Right-click and Enter confirm

| Session | Enter | Right-click (no drag) | Esc | Walkthrough evidence |
|---|---|---|---|---|
| Move / Rotate / Scale (G / R / S, gizmo) | commits | commits | cancels | Responsiveness ⑧ / ⑨ (`transform-commits` 1 → 2, then held at 2; capture 266) |
| Trim (T) | ends | ends | ends | TrimSplit ⑩b (`trim` 1 → 0 both ways, 10 curves kept) |
| Fill | ends | ends | ends | Fill ③b (`fill` 1 → 0 both ways, steps held at 4) |
| Fillet / chamfer, extrude, inset, offset, pipe, joint, Boolean, pending dimension | applies | applies | cancels | existing walkthroughs (Enter) + the shared right-click dispatch |

- A right-drag still pans; only a press-and-release without movement confirms.

## 💡 Logging

- `SolidArcTrace.log` sits next to the exe; the previous run is kept as `SolidArcTrace.previous.log`. The status-bar **Log** button or `F12` opens the panel: path, Copy path, Slow frames / Refusals filters, Copy.
- A frame over 50 ms writes `Frame · slow frame N · T ms (CPU · interface · upload · GPU wait) · session · triangles · steps · costliest phases`. At most 4 lines a second are written; the rest are counted on the next line.
- `SolidArcCapture --trace <file> --slow-frame <ms>` writes the same trace from a script, with every frame over the threshold and no rate limit.
