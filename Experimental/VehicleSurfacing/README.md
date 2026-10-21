# 🧩 VehicleSurfacing — Blender edge loops → fitted curves → CAD panels → mirrored sheet → solid (plan V1–V7)

**TL;DR** — Export the edge loops you already modelled in Blender as ordered limit points. In DraftingWorkspace, **Import curves…** fits each loop as a fair cubic B-spline within 0.2 mm. You then pick the sections in mode 1, press **L** to loft an exact NURBS sheet, and drag poles to reshape it; the loft regenerates. Tested on a synthetic bonnet: the panel lies within **0.076 mm** of all 286 limit points, including 5 loops the loft never saw, and within **0.053 mm** of the analytic surface.

**V5–V7:**
- Curves that cross (sections plus spines) build a Gordon **network** panel when you press **L**.
- The **G0 / G1 / G2** buttons make the next loft meet the panel it shares a section with. Measured: G1 1.9e-15 rad, G2 curvature jump 3e-12.
- **Alt+X** mirrors every panel through x = 0. SpaceCurves that end on that plane are refitted to leave it along x̂, so the halves meet G1 (measured: 0 rad).
- **Close → solid** puts plane caps on planar rims. The noisy pod sample closes into a sound solid within 4.8e-4 of its analytic volume.

| Milestone | State | Evidence |
|---|---|---|
| V1 Blender add-on + `VehicleCurves.toml` import | 🟢 import · ⚠️ add-on not run in Blender | W11: round trip bit-equal, 5 malformed files refused, mm scaled |
| V2 `FitSpaceCurve` | 🟢 | SurfacingValidation S1–S5 (worst 0.158 mm ≤ 0.2 mm, exact recovery 1e-14, corners split, refusal names the worst point) |
| V3 `SpaceCurve` steps: display, combs, mode 1 pole drag | 🟢 | L6 + W11 (pole move, undo exact, names kept) + walkthrough 53 / 53 with frames |
| V4 Loft through 3–8 sections | 🟢 | L1–L5 (sections to 7e-16, cylinder exact), W11 bonnet 0.076 mm ≤ 0.5 mm |
| V5 Gordon / Coons networks | 🟢 | PanelSurfacingValidation N1–N6 (bilinear 3.3e-16, bonnet net 0.065 mm vs H), W12, captures 66–67 |
| V6 G1 / G2 across shared sections | 🟢 loft · 🚧 network | C1–C6, T1, captures 60–61 |
| V7 Sew → mirror → close → solid | 🟢 · 🚧 trim | A1–A6 (cylinder and frustum 1.6e-15), T2–T4, W12, `PanelAssemblyWalkthrough.txt` 57 / 57, captures 62–65 |
| V8 Zebra + deviation map | 🚧 | — |

## Files

| Path | What |
|---|---|
| `Blender/ExportVehicleCurves.py` | Blender 4.x add-on: File ▸ Export ▸ Vehicle Curves (.toml) |
| `Samples/SyntheticBonnet.toml` | 11 curves × 26 points (6 sections + 5 check loops) on an analytic bonnet, ±0.05 mm noise |
| `Samples/SyntheticBonnetNetwork.toml` | V5: 6 sections + 4 spines that cross them (seed 0xB0AA38) |
| `Samples/SyntheticPod.toml` | V7: 5 half hoops with `Planar = true`, from the top of x = 0 round to the bottom (seed 0x90D5) |
| `Tooling/ComposeSyntheticBonnet.py` | Regenerates all three samples (SyntheticBonnet.toml stays bit-identical) |
| `../DraftingWorkspace/Source/Document/VehicleCurveImport.*` | `DecodeVehicleCurves` · `ReadVehicleCurves` · `EncodeVehicleCurves` |
| `../SolidArcRemastered/Source/Surfacing/` | `CurveFitting` (V2, plus end tangents and planar fits), `SectionLoft` (V4, V6), `CurveNetwork` (V5), `PanelAssembly` (V7), `SurfaceAlgebra`, `LinearSystem` |

## Workflow

```
Blender (quads, Subdivision)          DraftingWorkspace (Plasticity keys)
  select edge loops ──export──▶ VehicleCurves.toml ──Import curves…──▶ SpaceCurve steps (fit ≤ 0.2 mm)
                                                                         │ 1  control-point mode
                                                                         │ click / Shift+click sections in order
                                                                         ▼ L  loft
                                                                       Loft step → open NURBS sheet
                                                                         │ drag a pole (mode 1)
                                                                         ▼ the curve refits, the sheet regenerates
                                                                       G2 + L (next sections)  → panel 2, curvature-continuous
                                                                       Alt+X                   → mirror x = 0, one sewn sheet
                                                                       Close → solid           → plane caps (planar rims only)
```

| Key / control | Does |
|---|---|
| Header **Import curves…** (or `--curves PATH` on the host or capture app) | Reads the file and adds one SpaceCurve step per curve. Everything is one undo entry; if any curve is refused, the whole import is refused |
| `1` | Control-point mode: every curve shows its poles and control polygon |
| click · Shift+click · Ctrl+click | Select · add (sets the loft order; the badges show 1, 2, …) · remove a curve. A curve within 6 px wins over the face under it |
| `L` / **Loft (L)** | Loft through the selected curves in pick order. Fewer than 2 curves is refused. If at least 4 selected curves cross each other, L builds a **Network** step instead (Gordon; order does not matter) |
| **G0 / G1 / G2** (Curves card) | Continuity of the next loft to the panel that already ends on its first or last section. The choice persists. On a network, G1 / G2 is refused (🚧) |
| `Alt+X` / **Mirror x** (Body card) | Mirror step through x = 0 (Plasticity mirror, gizmo x). Images are named `Mirror#n.Image(…)`, and the halves sew along the plane |
| **Close → solid** (Body card) | Close step: a plane cap on every planar rim loop (`Close#n.Cap(k)`). A non-planar rim is refused, with how far it is off its plane |
| drag a pole (mode 1) | The pole follows the pointer in the screen-parallel plane and the curve previews live in green. Release commits one undo entry; Esc drops the drag |
| **Combs** | Curvature combs on the selected curves (the longest tooth is 15 % of the chord) |
| Ctrl+Z / Ctrl+Shift+Z | Undo / redo. The undone panel is bit-identical to the one before the edit (W11) |

## File format (`VehicleCurves.toml`)

```toml
Format = "VehicleCurves"          # required
Units  = "m"                      # "m" or "mm" (mm is scaled by 1e-3)
Source = "free text"              # optional

[[Curves]]
Name   = "Section1"               # required; becomes the step caption (Name.1, Name.2 … if taken)
Closed = false                    # optional
Planar = true                     # optional: fit in the points' best plane (cap sections; refused when > tolerance off it)
Points = [[0.0, 0.0, 0.78], [0.03, 0.0, 0.7799], …]   # ≥ 2 finite [x, y, z], ordered along the loop
```

These files are refused, with a reason that names the problem: not TOML, `Format` other than `VehicleCurves`, units other than m / mm, no `[[Curves]]`, a curve without `Name`, fewer than 2 points, or a point that is not a finite `[x, y, z]`.

## Blender add-on (⚠️ untested in Blender)

The add-on has not been run in Blender. The sandbox has no Blender, so only its Python syntax has been checked. The workspace side is validated with the synthetic sample instead.

1. Edit ▸ Preferences ▸ Add-ons ▸ Install… → `ExportVehicleCurves.py`, then enable it.
2. In Edit Mode, select one or more edge loops on the active mesh. Branches, meaning a vertex with 3 or more selected edges, are refused.
3. File ▸ Export ▸ Vehicle Curves (.toml).

The add-on exports **limit points**: points on the evaluated Subdivision surface the designer sees, not on the cage. It assumes the cage vertices come first in the evaluated mesh. Each cage edge is followed along a breadth-first path of 2^level evaluated edges. Points within 0.5 mm of the mirror plane are snapped onto it (x = 0), and the scene unit scale is applied.

## Limits (honest)

- 🚧 V8 (zebra stripes, deviation map) is not built yet, and neither are trim, guide rails or N-sided corners.
- ⚠️ G1 / G2 to a neighbour and the guaranteed G1 mirror seam apply to **lofts** only. A network refits its curves, which loses the end tangent: the bonnet net's mirror seam is 2.75e-4 rad (reported in T3).
- ⚠️ Sewing needs boundaries that coincide point for point. Panels sharing one SpaceCurve meet that by construction; a differently parametrised boundary is refused (🚧 reparametrised sewing). Caps are planes only.
- ⚠️ Adding a Mirror step refits the SpaceCurves that end on its plane, and they may get more poles. A pole edit whose index then no longer exists refuses the step rather than moving the wrong pole.
- 📝 A lone network panel may face down (back-face tint). Close orients a closed body outward.
- ⚠️ A pole edit is an absolute position stored on the step, applied after the fit by pole index. A refit with fewer poles (different points or tolerance) refuses the step and names the edit, rather than moving the wrong pole. Re-importing makes new steps without the edits.
- ⚠️ Blends, face edits and offsets on the sheet are refused; NURBS offsets are not exact (plan decision D4).
- 📝 Overlay curves are drawn on top of the shaded body with no depth test, so a curve behind the sheet stays visible.
