# DraftingDocumentValidation

Document layer of DraftingWorkspace over the SolidArcRemastered feature tree (14.84 s).

| Gate | Subject | Limit | Worst | Checks | Verdict |
|------|---------|-------|-------|--------|---------|
| W1 | Construct tiles (block, cylinder, sphere, torus, union, difference): |V − closed form| / V, sound audit | ≤ 10⁻⁹ | 4.14e-16 | 5 | 🟢 |
| W2 | CPU ray pick: face under the ray / edge within reach resolves to the expected persistent name | all equal | 0 | 4 | 🟢 |
| W3 | Fillet / chamfer on picked edges: |V − closed form| / V | ≤ 10⁻⁹ | 4.17e-16 | 2 | 🟢 |
| W4 | Face edit on a picked face (push): |V − closed form| / V | ≤ 10⁻⁹ | 5.49e-16 | 1 | 🟢 |
| W5 | Parameter edit regenerates downstream steps; selection / references survive by name | ≤ 10⁻⁹, names kept | 6.08e-16 | 3 | 🟢 |
| W6 | Refused command (oversize fillet, empty selection) leaves tree, body and history untouched | all equal | 0 | 2 | 🟢 |
| W7 | Undo / redo restore exact volumes; delete removes a whole tile | ≤ 10⁻¹² | 6.08e-16 | 4 | 🟢 |
| W8 | TOML save → load → save is byte-equal; reloaded body volume and names equal | equal | 0 | 4 | 🟢 |
| W9 | Variable blend (V / F): volume vs closed form; each size at its anchored corner through flip, parameter edit and reload | ≤ 10⁻⁹ | 3.57e-16 | 12 | 🟢 |
| W10 | Drawn sketches (line, rectangle, circle, arc, slot, 4 curve forms) → extrude / Boolean: |V − closed form| / V; refusals; reload; delete | ≤ 10⁻⁹ | 4.45e-16 | 20 | 🟢 |
| W11 | Vehicle curves: VehicleCurves.toml round trip (bit-equal) → SpaceCurve steps → loft sheet vs analytic bonnet and limit points; pole move + undo; reload | ≤ 0.5 mm, equal | 0.076 | 18 | 🟢 |
| W12 | Vehicle panels: loft + loft (G2) → Alt+X mirror → 4-panel sheet; pod loft → mirror → Close → solid vs ∫ πab dy; crossing curves → network; reload byte-equal | sound · rel. ≤ 2·10⁻³ | 0.0623 | 11 | 🟢 |
| W13 | Surface evaluation: deviation needles are true closest points (residual ∥ normal, ≤ 61² lattice) and ≤ 0.2 mm on the fitted bonnet; zebra phase jumps at a G0 crease and is continuous at G2; zebra density tessellation | ≤ 0.2 mm · G2 ≤ 10⁻⁶ stripe | 0.076 | 6 | 🟢 |
| W14 | Blueprint study: image planes (BMW calibration landmarks → world, picture decode, codec round trip, refusals, re-import by name) and x / y-axis Construct cylinders | ≤ 10⁻⁵ m · ≤ 10⁻⁹ | 2.16e-07 | 11 | 🟢 |
| W15 | Matcaps: built-in library optics (tinted metal / white plastic highlights, brushed anisotropy, roughness), picture fitting, per-face / body assignment without regeneration, undo, regeneration by persistent name, codec, refusals | exact · 1 / 255 | 0 | 13 | 🟢 |

## Walkthrough

| Gate | Check | Value | Verdict |
|------|-------|-------|---------|
| W1 | Sphere r 0.8 at (0.3, −0.2, 1) (V = 2.144661) | 4.14e-16 | 🟢 |
| W1 | Torus R 1.5 r 0.4 at (1, 2, 0.5) (V = 4.737410) | 3.75e-16 | 🟢 |
| W1 | Block 3 × 2 × 1 − through cylinder r 0.4 (V = 5.497345) | 1.62e-16 | 🟢 |
| W1 | Block1 4 × 2 × 1 (V = 8.000000) | 2.22e-16 | 🟢 |
| W1 | + Cylinder1 boss r 0.5 h 0.75 (union on the top face) (V = 8.589049) | 4.14e-16 | 🟢 |
| W2 | ray ↓ at (3, 1) → Extrude#2.Cap(End) | 0 | 🟢 |
| W2 | ray at the x = 4 top edge → Edge(Extrude#2.Cap(End)|Extrude#2.Side(Line2)) | 0 | 🟢 |
| W2 | ray away from the body → no pick | 0 | 🟢 |
| W3 | Fillet1 r 0.25 on the picked edge (V = 8.562223) | 4.15e-16 | 🟢 |
| W3 | Chamfer1 0.2 on the x = 0 top edge (V = 8.522223) | 4.17e-16 | 🟢 |
| W2 | ray → front face → Extrude#2.Side(Line1) | 0 | 🟢 |
| W4 | Push1 0.3 on the picked front face (V = 9.712200) | 5.49e-16 | 🟢 |
| W5 | Block1.Depth = 2.5 → fillet, chamfer, push regenerate (V = 11.695493) | 6.08e-16 | 🟢 |
| W5 | no step refused after the edit | 0 | 🟢 |
| W5 | selection still names Extrude#2.Side(Line1) | 0 | 🟢 |
| W6 | fillet r 5 on Edge(Extrude#2.Cap(End)|Extrude#2.Side(Line3)) refused: Blend#8: Blend: vertex V57: cap: contact curve misses the third face | 0 | 🟢 |
| W6 | push with no faces refused: select one or more faces first | 0 | 🟢 |
| W7 | undo Block1.Depth → exact earlier volume | 0 | 🟢 |
| W7 | redo → edited volume | 6.08e-16 | 🟢 |
| W7 | delete Cylinder1 (sketch + extrude) → volume without the boss (V = 11.106445) | 4.8e-16 | 🟢 |
| W7 | both steps and every Cylinder1.* parameter removed | 0 | 🟢 |
| W8 | reloaded volume equal (5548 bytes of TOML) | 0 | 🟢 |
| W8 | reloaded persistent names equal | 0 | 🟢 |
| W8 | save → load → save byte-equal | 0 | 🟢 |
| W8 | malformed file refused: a step has no label | 0 | 🟢 |
| W9 | Fillet 0.1 → 0.3 on Edge(Extrude#2.Cap(End)|Extrude#2.Side(Line1)) (unflipped, anchor x = 0.0) (V = 7.962833) | 2.23e-16 | 🟢 |
| W9 | rails 0.1 from the anchored corner, 0.3 from the far one (unflipped, anchor x = 0.0) | 0 | 🟢 |
| W9 | history caption: Fillet 0.1 → 0.3 on 1 edge(s) | 0 | 🟢 |
| W9 | Block1.Width = 5 regenerates the variable fillet (unflipped, anchor x = 0.0) (V = 9.953527) | 3.57e-16 | 🟢 |
| W9 | after the edit 0.1 is still at its corner, x = 0.0 (unflipped, anchor x = 0.0) | 1.36e-16 | 🟢 |
| W9 | reload: same volume bits, anchored rails, byte-equal re-save (unflipped, anchor x = 0.0) | 1.36e-16 | 🟢 |
| W9 | Fillet 0.1 → 0.3 on Edge(Extrude#2.Cap(End)|Extrude#2.Side(Line1)) (flipped, anchor x = 4.0) (V = 7.962833) | 3.35e-16 | 🟢 |
| W9 | rails 0.1 from the anchored corner, 0.3 from the far one (flipped, anchor x = 4.0) | 0 | 🟢 |
| W9 | history caption: Fillet 0.1 → 0.3 on 1 edge(s) | 0 | 🟢 |
| W9 | Block1.Width = 5 regenerates the variable fillet (flipped, anchor x = 4.0) (V = 9.953527) | 3.57e-16 | 🟢 |
| W9 | after the edit 0.1 is still at its corner, x = 5.0 (flipped, anchor x = 4.0) | 1.36e-16 | 🟢 |
| W9 | reload: same volume bits, anchored rails, byte-equal re-save (flipped, anchor x = 4.0) | 1.36e-16 | 🟢 |
| W10 | Rectangle (0,0)–(2,1) on XY, extrude 1.5 (New) (V = 3.000000) | 2.96e-16 | 🟢 |
| W10 | − centre circle r 0.4 through (Difference) (V = 2.246018) | 3.95e-16 | 🟢 |
| W10 | + closed line triangle, height 1 (Union; 1/3 outside the box) (V = 2.579351) | 1.72e-16 | 🟢 |
| W10 | − closed Hermite pocket 0.5 (area 0.246, coplanar floor: closed spline section) (V = 2.456351) | 1.81e-16 | 🟢 |
| W10 | reload of the drawn document: same volume bits, byte-equal re-save | 0 | 🟢 |
| W10 | refused untouched: "Arc points are collinear" · "Spline1: Hermite points 1 and 2 coincide" · "a second body needs Union, Difference or Intersection" | 0 | 🟢 |
| W10 | open arc profile refused: "Extrude#10: Extrude: open profile: Arc1 ends at a point joined to 0 other curves" | 0 | 🟢 |
| W10 | delete Curve1: its extrude goes with it (V = 2.579351) | 1.72e-16 | 🟢 |
| W10 | centre rectangle about (1, 0.5) to (2, 1), extrude 1.5 (V = 3.000000) | 2.96e-16 | 🟢 |
| W10 | − slot (0.4,0.5)–(1.2,0.5) r 0.2 through (V = 2.331504) | 1.9e-16 | 🟢 |
| W10 | rectangle on XZ, extrude 0.5 (V = 1.000000) | 2.22e-16 | 🟢 |
| W10 | − two-point circle (0.6,0.5)–(1.4,0.5) on XZ (V = 0.748673) | 4.45e-16 | 🟢 |
| W10 | closed Hermite through 6 clicks, extrude 0.7 = area 4.510250 × 0.7 (V = 3.157175) | 2.81e-16 | 🟢 |
| W10 |   − circle r 0.3 inside the Hermite (V = 2.959255) | 3e-16 | 🟢 |
| W10 | closed Bezier through 6 clicks, extrude 0.7 = area 1.571429 × 0.7 (V = 1.100000) | 4.04e-16 | 🟢 |
| W10 |   − circle r 0.3 inside the Bezier (V = 0.902080) | 1.23e-16 | 🟢 |
| W10 | closed NURBS through 6 clicks, extrude 0.7 = area 3.065625 × 0.7 (V = 2.145938) | 0 | 🟢 |
| W10 |   − circle r 0.3 inside the NURBS (V = 1.948017) | 0 | 🟢 |
| W10 | closed Cubic spline through 6 clicks, extrude 0.7 = area 4.633720 × 0.7 (V = 3.243604) | 4.11e-16 | 🟢 |
| W10 |   − circle r 0.3 inside the Cubic spline (V = 3.045684) | 4.37e-16 | 🟢 |
| W11 | SyntheticBonnet.toml decoded: 11 curves · 286 points  | 0 | 🟢 |
| W11 | V1 round trip decode → encode → decode: names, closed flags, every coordinate bit-equal | 0 | 🟢 |
| W11 | Units = "mm" scaled to metres | 0 | 🟢 |
| W11 | malformed file refused: Format is not "VehicleCurves" | 0 | 🟢 |
| W11 | malformed file refused: unknown Units "in" (m or mm) | 0 | 🟢 |
| W11 | malformed file refused: curve 1 (A) needs at least 2 points | 0 | 🟢 |
| W11 | malformed file refused: curve 1 (A): point 1 is not [x, y, z] | 0 | 🟢 |
| W11 | malformed file refused: curve 1 has no Name | 0 | 🟢 |
| W11 | import → 11 SpaceCurve steps, 1 undo entry, 286 points → 44 poles, worst fit [mm] (value) | 0.0558 | 🟢 |
| W11 | refused file leaves the document untouched: import: Format is not "VehicleCurves" | 0 | 🟢 |
| W11 | loft through one curve refused: loft needs at least 2 curves (select them in section order) | 0 | 🟢 |
| W11 | loft 6 sections → sheet: 1 face · 4 edges · audit sound  | 0 | 🟢 |
| W11 | sheet vs all 286 limit points (6 sections + 5 loops never lofted), worst [mm] (value) | 0.076 | 🟢 |
| W11 | sheet vs the analytic bonnet H(x, y), 61 × 61 normal distance, worst [mm] (value) | 0.0525 | 🟢 |
| W11 | reload: points bit-equal, sections equal, save → load → save byte-equal, face name Loft#12.Side(Surface) | 0 | 🟢 |
| W11 | move pole 2 of Section3 by +10 mm → loft regenerates, panel at (0.5, 0.38) moved 3.728 mm, face name kept | 0 | 🟢 |
| W11 | pole 99 refused: pole 99 does not exist | 0 | 🟢 |
| W11 | undo → panel exactly back · redo → moved again | 0 | 🟢 |
| W12 | front loft + rear loft G2 → 2 panels sewn on Section3, sheet sound  | 0 | 🟢 |
| W12 | Alt+X → 4 panels, one sound sheet, face Mirror#14.Image(Loft#12.Side(Surface))  | 0 | 🟢 |
| W12 | bonnet reload: continuity + mirror steps survive, save → load → save byte-equal, names equal | 0 | 🟢 |
| W12 | Close on the open bonnet rim refused, document kept: Close#15: Close: rim loop 0 is not planar (off by 51.7 mm; only plane caps 🚧) | 0 | 🟢 |
| W12 | undo mirror → 2 panels | 0 | 🟢 |
| W12 | pod: Planar = true round trip · loft → mirror → Close → solid V = 0.792802 vs 0.792411, rel. (value)  | 0.000494 | 🟢 |
| W12 | second Close refused: Close#9: Close: the panels are already closed by Close#8 | 0 | 🟢 |
| W12 | pod reload: planar flags + close step, byte-equal, same volume | 0 | 🟢 |
| W12 | G1 on crossing curves refused: these curves cross (a network): G1 / G2 is available for section lofts only (🚧 networks) | 0 | 🟢 |
| W12 | 10 crossing curves → Network step (Gordon), sheet sound, vs analytic H 61 × 61, worst [mm] (value)  | 0.0623 | 🟢 |
| W12 | network reload byte-equal | 0 | 🟢 |
| W13 | deviation map, six-section loft: 286 needles (one per imported point), worst [mm] (value), mean 0.017541 mm — W11 measures the same sheet | 0.076 | 🟢 |
| W13 | interior needles run along the foot normal: |gap × N| / |gap| = 5.53e-08 (≤ 10⁻⁶) | 0 | 🟢 |
| W13 | no needle longer than the best of a 61 × 61 lattice: excess 0 mm | 0 | 🟢 |
| W13 | G0 rear loft on the lifted Section3: zebra phase jump across the seam 1.07 stripes of 64 (> 0.05) — visibly broken stripes | 0 | 🟢 |
| W13 | G2 rear loft: zebra phase jump 1.76e-13 stripes of 64 (≤ 10⁻⁶) — continuous stripes | 0 | 🟢 |
| W13 | zebra tessellation 5816 vs 94 triangles (≥ 20×); distinct scene generation re-uploads the GPU buffers | 0 | 🟢 |
| W14 | BMW trace: 24 sections + 3 planes; side bumper / ground pixels → y 0, y 4.58, z 0, worst [m] (value) | 2.16e-07 | 🟢 |
| W14 | top nose / tail / centre line and front centre / ground pixels → world within 2.08 mm (sheet alignment, ≤ 1.5 px) | 0 | 🟢 |
| W14 | Side ⟂ x at x = −1.2 (behind, seen from Right) · Top ⟂ z under the ground · Front ⟂ y behind the tail | 0 | 🟢 |
| W14 | 3 × 2 BMP → RGBA8, every pixel exact, top row first, alpha 255 | 0 | 🟢 |
| W14 | missing picture refused (no throw) | 0 | 🟢 |
| W14 | import → 24 curves + 3 planes; re-import adds curves, replaces planes by Name (still 3) | 0 | 🟢 |
| W14 | document with planes (one hidden, one at ink 0.3): save → load → save byte-equal | 0 | 🟢 |
| W14 | documents without planes write no ImagePlanes key (older files unchanged) | 0 | 🟢 |
| W14 | zero-area plane refused (image plane Side: Right and Down span no area) · opacity 1.5 refused — no curves kept | 0 | 🟢 |
| W14 | Axis x cylinder r 0.37 h 0.5: |V − πr²h| / V (value); 0.62 … 1.12 along the axis, Ø 0.74 in z | 1.29e-15 | 🟢 |
| W14 | Axis y cylinder r 0.37 h 0.5: |V − πr²h| / V (value); 0.805 … 1.31 along the axis, Ø 0.74 in z | 0 | 🟢 |
| W15 | 13 built-ins 256 × 256 RGBA8, opaque, composed twice bit-equal; library Studio + 13 (Brass = layer 5); unknown name refused / layer 0 | 0 | 🟢 |
| W15 | brightest texel: Brass blue / red 0.514 (< 0.75, tinted metal) · Red plastic min / max 0.914 (> 0.8, white Fresnel) | 0 | 🟢 |
| W15 | |∂L/∂x| / |∂L/∂y|: Brushed steel 0.347 < 0.6 × Chrome 0.745 (anisotropic) | 0 | 🟢 |
| W15 | luminance spread inside the disc: Chrome 63.1 > Satin aluminium 47.8 > Clay 45.6 | 0 | 🟢 |
| W15 | 4 × 2 BMP → 256 × 256: quarter columns show source columns 2 and 3 (centre crop), within 1 / 255, opaque | 0 | 🟢 |
| W15 | 2 faces → Brass, body → Brushed steel: state words 2 × layer 5 + 4 × layer 2; revision unchanged (no regeneration); 2 matcap-only undo entries | 0 | 🟢 |
| W15 | unknown matcap refused (no matcap named Unobtainium) · repeat refused (the body already shows BrushedSteel) · appearance and history unchanged | 0 | 🟢 |
| W15 | undo → body Studio, undo → no face matcaps, redo × 2 → identical appearance; revision unchanged throughout | 0 | 🟢 |
| W15 | Block1.Depth 1 → 1.5 regenerates; still 2 Brass + 4 Brushed steel faces, 0 dangling assignments | 0 | 🟢 |
| W15 | remove on 1 face → 1 override left; remove on the body → Studio; a third remove refused (the body already shows Studio) | 0 | 🟢 |
| W15 | picture ShopStripes → layer 14 of 15, fitted pixels equal; twice / built-in name / missing file refused (cannot read picture /tmp/DraftingMatcapValidation.bmp.missing (can't fopen)) | 0 | 🟢 |
| W15 | save → load → save byte-equal with [Appearance]; documents without matcaps write no Appearance key | 0 | 🟢 |
| W15 | a file naming an unknown matcap is refused: Appearance: no matcap named Unobtainium (face Extrude#2.Side(Line2)) | 0 | 🟢 |

## Saved document (TOML, first 40 lines)

```toml
# DraftingWorkspace document — feature tree of SolidArcRemastered (regenerated on load)
Description = ''
Format = 'DraftingWorkspace'
FormatVersion = 1
Title = 'Bracket'

[[Parameters]]
Description = 'corner x'
High = 5.0
Low = -5.0
Name = 'Block1.X'
Value = 0.0

[[Parameters]]
Description = 'corner y'
High = 5.0
Low = -5.0
Name = 'Block1.Y'
Value = 0.0

[[Parameters]]
Description = 'corner z (sketch elevation)'
High = 5.0
Low = -5.0
Name = 'Block1.Z'
Value = 0.0

[[Parameters]]
Description = 'extent along x'
High = 10.0
Low = 0.10000000000000001
Name = 'Block1.Width'
Value = 4.0

[[Parameters]]
Description = 'extent along y'
High = 10.0
Low = 0.10000000000000001
Name = 'Block1.Depth'
Value = 2.5
```
