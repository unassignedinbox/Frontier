# 🧩 SolidArc — drawn Construct primitives and the development operation trace

**TL;DR**: The Construct menu now has two sections, **Solids** (Box, Cylinder, Cone, Sphere, Torus, Wedge, Prism, Tube) and **Sketch** (Line, Rectangle, Centre rect, Circle, Arc, Slot, Polygon, Curve). You pick a tile and draw the shape in the viewport, as on the reference site. Live dimension lines (W / D / H / R / Ø, lengths, angles) follow the pointer. Every solid is added as its **own body in the Outliner**: the default is R = new body, so nothing is Booleaned unless you press Q / W / E. A development trace (`FRONTIER_DEVELOPMENT_TRACE`) writes each operation to `SolidArcTrace.log`, with nesting and timings, and shows it live in an F12 panel.

## 🏁 Evidence

| Check | Result |
|---|---|
| `ConstructDrawingWalkthrough.txt` (ctest `SolidArcConstructDrawingWalkthrough`) | 🟢 45 / 45, real Vulkan frames 200–211 |
| `SolidArcDocumentValidation` **W40** (new primitives) | 🟢 14 checks, worst 3.97e-15 |
| W1–W39 after the change | 🟢 no regressions |
| Trace smoke run (`_ClaudeScratch/tmp/TraceSmoke.cpp`) | 🟢 Command → Document → Replay → Sketch / Sweep → Volume, nested with ms, the refusal logged |

## 🏁 Drawing a solid

| Solid | Clicks | Live dimensions |
|---|---|---|
| Box · Wedge | corner (Alt: centre), opposite corner, height | W, D, H |
| Cylinder | centre, radius, height | R, Ø, H |
| Cone | centre, radius, apex height (below the plane points down) | R, Ø, H |
| Sphere | centre, radius | R |
| Torus | centre, major radius, tube radius | R, r |
| Prism | centre, vertex (+ / − sides), height | R, `n × side`, H |
| Tube | centre, outer radius, bore radius, height | R, r, H |

- **Plane:** the ground (XY) at the height of the first click. Starting on a body's upward face or a point on it raises the plane there (capture 211: a box drawn on the first box's top, `drawing-elevation = 1`).
- **Snapping:** plane points snap to the 0.1 grid and to object snaps. Heights move in 0.1 steps or level with a point snap. Hold Ctrl to place freely.
- **Keys:** R new body (the default) · Q union · W difference · E intersect · Tab opens the exact-size form, filled with what you have drawn · Ctrl+Z drops the last click · Esc aborts · Enter or right-click finishes.
- **Sketch tools:** dimensions show while you draw. Line shows length and angle (to +x, then to the previous segment). Rectangle shows W and H, Circle R and Ø, Arc the chord and R, Slot L and R. Polygon shows R, the side length, the angle per side and the interior angle.

| Capture | Shows |
|---|---|
| `Walkthrough/200ConstructSolids.png` · `201ConstructSketch.png` | The menu: Solids 8 tiles · Sketch 13 tiles (every primitive and curve form) |
| `202BoxBase.png` · `203BoxHeight.png` | Box base (W, D), then the height (H 1.000) |
| `204CylinderHeight.png` · `205ConeHeight.png` · `206PrismHeight.png` · `207TorusTube.png` | Wireframe preview with R / Ø / H / r |
| `208PolygonDimensions.png` · `209LineDimensions.png` | 2-D dimensions (R, side, 128.6° interior; length 0.943, 148.0°) |
| `210ConstructExact.png` | Tab: the form filled from the first click, New body selected |
| `211ConstructBodies.png` | 6 separate bodies in the Outliner |

## 🏁 Operation trace (development builds)

| Item | Value |
|---|---|
| Switch | CMake option `FRONTIER_DEVELOPMENT_TRACE` (default ON, PUBLIC define). OFF compiles every macro to nothing |
| File | `SolidArcTrace.log` beside the executable. The previous session's log is kept as `SolidArcTrace.previous.log` |
| Line | `   12.345 s  T0  Blend      │ ▶ BlendEdges · 3 edge(s)` … `◀ … · 41.208 ms`; `⚠️ slow` at ≥ 250 ms |
| Categories | Session · Command · Document · History · Replay · Sketch · Sweep · Boolean · Blend · FaceChange · Cut · Volume · Tessellate · Exchange · File · Scene · Preview · Construct · Status |
| Live view | F12: a trace panel with a filter. It keeps the last 4000 lines |

## ⚠️ Notes

- The document API (`SolidArcConstructRequest`) still defaults to Union for scripted callers. Only the interface defaults to New body.
- The exact-size form is still the old ImGui modal (unchanged styling).
