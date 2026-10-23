# DraftingDocumentValidation

Document layer of DraftingWorkspace over the SolidArcRemastered feature tree (89.59 s).

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
| W16 | Multi-body: New body (R) tiles keep exact separate volumes; per-body matcaps resolve face → body → default without regeneration; Combine (Q) difference consumes the tool and its delete restores it; targeted Construct / blend touch one body; cross-body refusals; scene body spans; undo; codec | ≤ 10⁻⁹ · exact | 1.83e-15 | 10 | 🟢 |
| W17 | Trim / split: C cuts a block by a drawn line (2 + 6 exact) and a circle (π/4), the outside piece keeps key and matcap; X removes picked bodies (undo / redo); vehicle loft cut by a character line along −z → two sound sheets, X the piece; refusals; codec | ≤ 10⁻⁹ · exact | 3.25e-15 | 6 | 🟢 |
| W18 | Curve trim (Plasticity T): a pentagram drawn as one chain refuses to extrude, T on the five inner pieces leaves the star (area vs shoelace, extrude exact); preview leaves the document alone; cross-sketch trims (circle → arc, side split) with undo / redo; a trim that would break an extrude is refused; an emptied sketch goes; codec | ≤ 10⁻¹² · ≤ 10⁻⁹ · exact | 1.11e-15 | 6 | 🟢 |
| W19 | Move / rotate / scale (G / R / S): Transform steps keep volume (×8 for scale 2), the scene and picking follow the body, face selection survives by name, undo exact; refusals untouched | ≤ 10⁻⁹ · exact | 3.22e-15 | 4 | 🟢 |
| W20 | Duplicate (Shift+D) and copy / paste (Ctrl+C / Ctrl+V): moved copy = new active selected body with its own caption; paste after X copies the body as it stood at copy time; refusals | ≤ 10⁻⁹ · exact | 1.89e-15 | 3 | 🟢 |
| W21 | Hide family (H, Shift+H, Alt+H, Ctrl+H): scene bodies and picking follow, selection drops hidden bodies, undo, codec round trip, refusals | exact | 0 | 2 | 🟢 |
| W22 | Join (J) two open drawings into a closed square (extrudes to 1); offset (O) it by ¼ (2.25); pipe (P) along a drawn line (π r² L); refusals; codec byte-equal with kinds 13 / 14 / 15 | ≤ 10⁻⁹ · exact | 5.65e-16 | 5 | 🟢 |
| W23 | Sketch constraints (K): a drawn rectangle goes DOF 4 → Lock 2 → D 1 → D 0, each dimension drives the extrude volume; literal and parameter-bound dimension edits regenerate downstream; undo / redo; remove frees a degree | ≤ 10⁻⁹ · exact | 5.92e-16 | 4 | 🟢 |
| W24 | Constraint refusals: redundant (named partner) and conflicting (named partners) constraints, non-dimension / non-positive edits, unknown sketch — the document, history and file stay untouched | exact | 0 | 4 | 🟢 |
| W25 | Point drag: a free corner follows the cursor and the extrude follows it; a locked point refuses; the preview is pure; save → load → save byte-equal with constraint labels | ≤ 10⁻⁹ · exact | 2.96e-16 | 3 | 🟢 |
| W26 | Reference dimensions (Shift+D / E): a driven diagonal reads √13 then 5 without moving anything (DOF, positions, volume bit-equal); editing it or making it driving while implied is refused; E makes it driving at its reading and back; undo / redo; codec | ≤ 10⁻¹² · exact | 2.96e-16 | 4 | 🟢 |
| W27 | Spline constraints: a line and a NURBS curve joined → L (G1) → C (G2, replaces the G1) → H on the far end, each solve ≤ 10⁻¹² with DOF −1 / −1 / −1; undo restores the G1; G2 on a natural cubic refused; codec | ≤ 10⁻¹² · exact | 5.14e-16 | 4 | 🟢 |
| W28 | Cross-sketch links: a later sketch's line linked to an earlier plate's corner and top line (one undo entry, construction curve on linked points) follows the plate's width edit; backward links, linked drags, bad indices refused; codec; unlink frees | ≤ 10⁻¹² · exact | 2.22e-16 | 5 | 🟢 |
| W29 | Revolve (Alt+R): annulus 3π about the proposed y-axis (New); a plate about its own edge line united → 4.625π, undo / redo; x-axis fallback quarter turn 3π/2; refusals (angle, crossing axis, no sketch, nothing to subtract); codec writes the point axis only without a line | ≤ 10⁻⁹ · exact | 1.88e-16 | 5 | 🟢 |
| W30 | Evolution (Shift+P / Alt+E): rectangle along a proposed line sketch = prism 1; circle along a quarter arc = Pappus π²/16; a square swept through a block as Difference = 1.875; refusals; codec Kind 16 + Path | ≤ 10⁻⁹ planar · ≤ 10⁻⁶ NURBS skin | 5.28e-09 | 5 | 🟢 |
| W31 | Incremental replay: on a 27-step studded plate and the vehicle bonnet every command / undo / redo / preview clone equals a full replay with the cache off (volumes, names, tokens, step reports, saved file); appends resume at the new step, a parameter edit at its first binding step, undo / redo / clone restore every step; only the 8 newest history entries keep checkpoints | identical · undo / redo / clone ≥ 10× · tail edit ≥ 2× | 3.18e+03 | 10 | 🟢 |
| W32 | X-ray (Alt+Z) pick-through and box edges on a 4 × 2 × 1 block in the isometric view: the ray at the hidden back-bottom edge picks the top face opaque and that edge in x-ray, √(1.25² + 1) = 1.6008 farther along the ray; a visible edge still picks in x-ray; the full-viewport box takes the 9 edges off the far corner (0, 2, 0) opaque and all 12 in x-ray; an empty box takes none | all equal | 0 | 5 | 🟢 |
| W33 | Measure (I) and Section (Alt+S) on a 4 × 2 × 1 block and a separate 2 × 2 × 1 body: body mass properties, face area / perimeter, edge length, minimum distances and angles between faces and edges, totals, X / Z / face-plane sections with second moments, two-body sections by the parallel-axis theorem, hidden bodies skipped, refusals | ≤ 1e-9 relative | 3.8e-12 | 15 | 🟢 |
| W34 | Section clip (Alt+S) on a 4 × 2 × 1 block cut at x = 1 (kept x ≤ 1): rays wholly on the removed side miss; a ray through the opening meets the cap at the back face x = 0 (distance 5, not the plane's 4); a kept face in front of the plane crossing still wins; removed edges are skipped, kept edges pick; the cap has no name; sheet faces show their back instead of a cap; the box takes only edges reaching the kept side; ComposeDraftingSectionClip marks sheet bodies | exact | 0 | 9 | 🟢 |
| W35 | Object snapping on a 4 × 2 × 1 block, a separate cylinder r 0.5 and coplanar sketches (isometric camera, pointer at projected pixels): endpoint, midpoint, edge / face / circle centres, on edge, on face, on circle, sketch crossing, tangent and perpendicular points from a reference, arc-length midpoint of a spline; hidden corners need x-ray, the section and excluded bodies drop candidates, the filter disables kinds, a dragged sketch excludes itself, direction lock lines | ≤ 1e-12 (on carrier) | 3.8e-14 | 22 | 🟢 |
| W36 | Joints (Alt+J) + interference on the bracket specimen: hover origins (rim centres, face centroids); a pin r 0.25 jointed revolute, flipped, offset 0.25 into the hole r 0.3 → centroid (3, 1.25, 0.5); a lever on the pin cap at 90° → exact bounds; measure lever + bracket clear 0.25, bracket + pin clear 0.05; pin offset −0.1 → lever overlaps (0.8 − 2 (0.2 √0.05 + 0.09 asin ⅔)) · 0.1; moving the bracket carries both; refusals; codec (joints, geometry, fixed point); undo | ≤ 1e-9 | 3.56e-14 | 15 | 🟢 |

## Walkthrough

| Gate | Check | Value | Verdict |
|------|-------|-------|---------|
| W1 | Sphere r 0.8 at (0.3, −0.2, 1) (V = 2.144661) | 4.14e-16 | 🟢 |
| W1 | Torus R 1.5 r 0.4 at (1, 2, 0.5) (V = 4.737410) | 3.75e-16 | 🟢 |
| W1 | Block 3 × 2 × 1 − through cylinder r 0.4 (V = 5.497345) | 1.62e-16 | 🟢 |
| W1 | Block1 4 × 2 × 1 (V = 8.000000) | 2.22e-16 | 🟢 |
| W1 | + Cylinder1 boss r 0.5 h 0.75 (union on the top face) (V = 8.589049) | 4.14e-16 | 🟢 |
| W2 | ray ↓ at (3, 1) → Extrude#2.Cap(End) | 0 | 🟢 |
| W2 | ray at the x = 4 top edge → Edge(Extrude#2.Cap(End)\|Extrude#2.Side(Line2)) | 0 | 🟢 |
| W2 | ray away from the body → no pick | 0 | 🟢 |
| W3 | Fillet1 r 0.25 on the picked edge (V = 8.562223) | 4.15e-16 | 🟢 |
| W3 | Chamfer1 0.2 on the x = 0 top edge (V = 8.522223) | 4.17e-16 | 🟢 |
| W2 | ray → front face → Extrude#2.Side(Line1) | 0 | 🟢 |
| W4 | Push1 0.3 on the picked front face (V = 9.712200) | 5.49e-16 | 🟢 |
| W5 | Block1.Depth = 2.5 → fillet, chamfer, push regenerate (V = 11.695493) | 6.08e-16 | 🟢 |
| W5 | no step refused after the edit | 0 | 🟢 |
| W5 | selection still names Extrude#2.Side(Line1) | 0 | 🟢 |
| W6 | fillet r 5 on Edge(Extrude#2.Cap(End)\|Extrude#2.Side(Line3)) refused: Blend#8: Blend: vertex V57: cap: contact curve misses the third face | 0 | 🟢 |
| W6 | push with no faces refused: select one or more faces first | 0 | 🟢 |
| W7 | undo Block1.Depth → exact earlier volume | 0 | 🟢 |
| W7 | redo → edited volume | 6.08e-16 | 🟢 |
| W7 | delete Cylinder1 (sketch + extrude) → volume without the boss (V = 11.106445) | 4.8e-16 | 🟢 |
| W7 | both steps and every Cylinder1.* parameter removed | 0 | 🟢 |
| W8 | reloaded volume equal (5611 bytes of TOML) | 0 | 🟢 |
| W8 | reloaded persistent names equal | 0 | 🟢 |
| W8 | save → load → save byte-equal | 0 | 🟢 |
| W8 | malformed file refused: a step has no label | 0 | 🟢 |
| W9 | Fillet 0.1 → 0.3 on Edge(Extrude#2.Cap(End)\|Extrude#2.Side(Line1)) (unflipped, anchor x = 0.0) (V = 7.962833) | 2.23e-16 | 🟢 |
| W9 | rails 0.1 from the anchored corner, 0.3 from the far one (unflipped, anchor x = 0.0) | 0 | 🟢 |
| W9 | history caption: Fillet 0.1 → 0.3 on 1 edge(s) | 0 | 🟢 |
| W9 | Block1.Width = 5 regenerates the variable fillet (unflipped, anchor x = 0.0) (V = 9.953527) | 3.57e-16 | 🟢 |
| W9 | after the edit 0.1 is still at its corner, x = 0.0 (unflipped, anchor x = 0.0) | 1.36e-16 | 🟢 |
| W9 | reload: same volume bits, anchored rails, byte-equal re-save (unflipped, anchor x = 0.0) | 1.36e-16 | 🟢 |
| W9 | Fillet 0.1 → 0.3 on Edge(Extrude#2.Cap(End)\|Extrude#2.Side(Line1)) (flipped, anchor x = 4.0) (V = 7.962833) | 3.35e-16 | 🟢 |
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
| W10 | refused untouched: "Arc points are collinear" · "Spline1: Hermite points 1 and 2 coincide" · "a second body needs Union, Difference, Intersection or New body" | 0 | 🟢 |
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
| W13 | interior needles run along the foot normal: \|gap × N\| / \|gap\| = 5.53e-08 (≤ 10⁻⁶) | 0 | 🟢 |
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
| W14 | Axis x cylinder r 0.37 h 0.5: \|V − πr²h\| / V (value); 0.62 … 1.12 along the axis, Ø 0.74 in z | 1.29e-15 | 🟢 |
| W14 | Axis y cylinder r 0.37 h 0.5: \|V − πr²h\| / V (value); 0.805 … 1.31 along the axis, Ø 0.74 in z | 0 | 🟢 |
| W15 | 13 built-ins 256 × 256 RGBA8, opaque, composed twice bit-equal; library Studio + 13 (Brass = layer 5); unknown name refused / layer 0 | 0 | 🟢 |
| W15 | brightest texel: Brass blue / red 0.514 (< 0.75, tinted metal) · Red plastic min / max 0.914 (> 0.8, white Fresnel) | 0 | 🟢 |
| W15 | \|∂L/∂x\| / \|∂L/∂y\|: Brushed steel 0.347 < 0.6 × Chrome 0.745 (anisotropic) | 0 | 🟢 |
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
| W16 | 3 bodies Block1 (Extrude#2) · Cylinder1 (Extrude#4) · Block2 (Extrude#6): 8 · π/2 · 1 exact, overlapping solids stay separate, all sound, active = the last created | 1.33e-15 | 🟢 |
| W16 | scene: 3 body spans in creation order, face counts equal the audit, every face tagged; body pick marks its 3 faces only | 0 | 🟢 |
| W16 | Matcap BrushedSteel → 1 body(ies) · Matcap Brass → 1 body(ies) · Matcap RedPlastic → 1 face(s): 5 steel + 1 red + 3 brass + 6 Studio faces; revision unchanged | 0 | 🟢 |
| W16 | Combine · Difference (Block1 − Cylinder1): Block1 = 8 − π/4 exact, 2 bodies, the hole wears Block1's steel; the tool's Brass override dangles (1) | 4.32e-16 | 🟢 |
| W16 | delete Combine#7 → Cylinder1 returns (π/2, Brass, 0 dangling); undo → drilled again | 2.46e-16 | 🟢 |
| W16 | fillet r 0.2 on Edge(Extrude#6.Side(Line1)\|Extrude#6.Side(Line2)) (Target Extrude#6) + Cylinder boss with Target = Block2: 1 − 0.04(1 − π/4) + π/32 exact; Block1 untouched | 1.83e-15 | 🟢 |
| W16 | refused: blend across bodies (the selection spans Block1 and Block2 (one body at a time)) · self-combine (Block1 is picked twice (the target cannot be a tool)) · no tool · unknown body (no body named Extrude#99) · Separate as Q · second New; untouched | 0 | 🟢 |
| W16 | Combine · Intersection (Block2 ∩ Block3): Block2 = its lower half (V = 0.495708), the slab consumed | 1.12e-15 | 🟢 |
| W16 | save → load → save byte-equal with Combine Tool / Target and [[Appearance.Bodies]]; 2 bodies, volumes bit-equal | 0 | 🟢 |
| W16 | a future step kind (25, beyond Evolution 16) is refused: a step has an unknown kind 25 (newer DraftingWorkspace?) | 0 | 🟢 |
| W17 | Cut (Block1 by Line1): Cut#4 (Cut1) = 2, Block1 = 6 exact, both sound; Block1 keeps its key and Brass, the piece wears Studio | 6.66e-16 | 🟢 |
| W17 | Remove (Cut1): 1 body left (6), Block1 active; undo → 2 bodies, redo → 1 | 0 | 🟢 |
| W17 | Cut (Block1 by Circle1): core π/4 and the holed block 6 − π/4 exact (genus 1), cut from the active body | 3.25e-15 | 🟢 |
| W17 | Cut (Loft1 by CharacterLine) seen along −z: 2 sound sheets (0 + 0 faces); Remove (Cut1) keeps the trimmed panel | 0 | 🟢 |
| W17 | refused: no cutter (choose the cutter: focus a sketch step or select a curve) · misses (Cut#9: Cut: the cut misses the body (nothing on the outside)) · unknown body (no body named Extrude#99) · extrude as cutter · Remove unknown (no body named Ghost#1); untouched | 0 | 🟢 |
| W17 | save → load → save byte-equal with Cut (11) and Remove (12): Cut 2 bodies · Bonnet 1 bodies, volumes bit-equal | 0 | 🟢 |
| W18 | preview T on Line1 of the pentagram: removed piece between its two crossings (off 2.22044604925e-16), 2 pieces left; document unchanged | 2.22e-16 | 🟢 |
| W18 | pentagram: extrude refused first (Extrude#2: Extrude: profile crosses itself: Line1 × Line3…); 5 × T → 10 lines, area 1.23792833147 vs shoelace 1.23792833147, extrude 1 → V 1.23792833147, sound | 1.11e-15 | 🟢 |
| W18 | T on a star tip after the extrude: refused (Extrude#2: Extrude: open profile: Line10 ends at a point joined to 0 other curves); untouched | 0 | 🟢 |
| W18 | Trim Circle1 of Circle1: the circle (another sketch than the rectangle) becomes the arc (2, 0.8) → (2, 0.2) inside it (off 2.22044604925e-16); undo → circle, redo → arc; Trim Line2 of Rectangle1 splits the right side (5 curves) | 2.22e-16 | 🟢 |
| W18 | Trim Line1 of Line1: nothing crosses it → the curve and its emptied sketch step go; undo restores | 0 | 🟢 |
| W18 | save → load → save byte-equal for the star (pins kept) and the plan; volumes bit-equal | 0 | 🟢 |
| W19 | Move (Block1): V 4 kept, sound; (1, 1) now empty, (11, 1) picks Block1; the picked top face Extrude#2.Cap(End) stays selected | 3.11e-15 | 🟢 |
| W19 | Rotate (Block1) + Scale (Block1): V 32 = 8 × 4, sound; undo → 4, redo → 32 | 3.22e-15 | 🟢 |
| W19 | refused: nothing picked · no body named Ghost#9 · Transform#6: Transform: the map is singular — document unchanged | 0 | 🟢 |
| W20 | Duplicate (Block1): 2 bodies of 32; Duplicate#6 (Block1Copy1) active, selected and picked at (11, 7) | 1.89e-15 | 🟢 |
| W20 | Paste (Block1): after X on Block1 the paste brings it back as Duplicate#8 (V 32 at the copied place) | 1.33e-15 | 🟢 |
| W20 | refused: the copied state is gone (step Nowhere#99 was deleted) · pick the bodies to duplicate (mode 4) | 0 | 🟢 |
| W21 | H hides Block1Copy1 (scene 2, unpickable, deselected) · Shift+H on the cylinder leaves it alone · Ctrl+H swaps → 2 shown · saved with Hidden, reload equal · Alt+H shows 3; undo → 2 · refused: pick the bodies to hide · nothing is hidden | 0 | 🟢 |
| W21 | hiding never touches the model: 3 bodies, V 64 + π/4 throughout | 0 | 🟢 |
| W19 | save → load → save byte-equal with Transform (13) and Duplicate (14) steps (placements and the paste source kept) | 0 | 🟢 |
| W22 | Join (Line1, Line2): the open L refuses to extrude; joined (one sketch fewer) the square extrudes to V 1 | 0 | 🟢 |
| W22 | Offset Line1 by 0.25: new sketch Offset1, extruded V 2.25 = 1.5²; the preview left the document alone | 1.97e-16 | 🟢 |
| W22 | Pipe r 0.25 along Line2: tube V π r² L = π/8 exact, sound, active | 5.65e-16 | 🟢 |
| W22 | refused: pick two or more sketches to join · Line1 feeds a later step (join into it instead) · Offset: the offset turns Line1 inside out (🚧 no loop clean-up) · Pipe#7: Pipe: radius must be positive · choose the path: focus a sketch step or select a curve | 0 | 🟢 |
| W22 | save → load → save byte-equal with the joined / offset sketches and the Pipe (15) step | 0 | 🟢 |
| W23 | DOF 4 → Constrain Rectangle1: Fixed (0, 0) · Line1.Start 2 → Constrain Rectangle1: Length 3 · Line1 1 (V 3) → Constrain Rectangle1: Length 2 · Line2 0 (V 6); labels F1 D1 D2 | 2.96e-16 | 🟢 |
| W23 | D1 = 4 in Rectangle1: V 8, far corner written back at (4, 2); undo V 6, redo V 8 | 5.18e-16 | 🟢 |
| W23 | Block tile DOF 0; Width = 3 in Block1 sets its Width parameter to 3 → V 3 × 1.5 × 1 = 4.5 | 5.92e-16 | 🟢 |
| W23 | Remove D2 (Length 2 · Line2) from Rectangle1: DOF 1, V 8 kept | 2.22e-16 | 🟢 |
| W24 | second H on Line1 refused: Horizontal · Line1 is already implied by H1 (Horizontal · Line1) | 0 | 🟢 |
| W24 | Length 4 on the top refused: Length 4 · Line3 is already implied by V1 (Vertical · Line2), V2 (Vertical · Line4), D1 (Length 4 · Line1) | 0 | 🟢 |
| W24 | Length 5 on the top refused: Length 5 · Line3 conflicts with V1 (Vertical · Line2), V2 (Vertical · Line4), D1 (Length 4 · Line1) | 0 | 🟢 |
| W24 | refused: Horizontal · Line1 has no value to change · Length must be positive · no sketch step Extrude#2 · the sketch has no constraint 40; file and history untouched | 0 | 🟢 |
| W25 | Drag Line2.End of Rectangle1: corner at (4, 3), Line3.Start follows to (0, 3), V 12; the preview left the document alone | 2.96e-16 | 🟢 |
| W25 | refused: Line1.Start is fully constrained (it cannot move) | 0 | 🟢 |
| W25 | save → load → save byte-equal; F1 / D1 labels and DOF 1 survive the reload | 0 | 🟢 |
| W26 | Constrain Rectangle1: Distance 3.60555 (reference) · Line1.Start · Line2.End: Distance 3.60555 (reference) · Line1.Start · Line2.End · DOF 0 · positions and V bit-equal | 0 | 🟢 |
| W26 | D2 = 4 in Rectangle1 → D3 reads 5 · refused: D3 is a reference dimension (E makes it driving) · Distance 5 · Line1.Start · Line2.End is already implied by H1 (Horizontal · Line1), V1 (Vertical · Line2), D1 (Length 3 · Line1), D2 (Length 4 · Line2) (it stays a reference) · H1 (Horizontal · Line1) has no value (only dimensions can be references) | 2.96e-16 | 🟢 |
| W26 | DOF 1 → Drive D3 (Distance 5 (reference) · Line1.Start · Line2.End) in Rectangle1 0 (V bit-equal) → Reference D3 (Distance 5 · Line1.Start · Line2.End) in Rectangle1 1; undo / redo | 0 | 🟢 |
| W26 | save → load → save byte-equal; Reference = true written for D3 only | 0 | 🟢 |
| W27 | DOF 10 → Constrain Line1: Tangent (G1) · Spline2 · Line1 9 → Constrain Line1: Curvature (G2) · Spline2 · Line1 (replaces T1 (Tangent (G1) · Spline2 · Line1)) 8 | 2.4e-16 | 🟢 |
| W27 | Constrain Line1: Horizontal end · Spline2.Point3 · DOF 7; undo × 2 brings T1 back, redo × 2 restores K1 + H1 | 5.14e-16 | 🟢 |
| W27 | refused: Curvature (G2): Spline2 is a cubic spline: its ends have zero curvature by construction (use Hermite, Bezier or NURBS for G2) | 0 | 🟢 |
| W27 | save → load → save byte-equal with SplineCurvature (22) and SplineDirection (24) | 0 | 🟢 |
| W28 | DOF 4 → Constrain Line1: Coincident · Line1.Start · Point2 2 → Constrain Line1: Point on line · Line1.End · Line2 1; corner gap 2.220e-16, edge gap 0.000e+00 | 2.22e-16 | 🟢 |
| W28 | D1 = 4 in Rectangle1: B's start follows to (4, 2), end on y = 2 (gap 0.000e+00), K view gap 0.000e+00, no notes, V 8 | 0 | 🟢 |
| W28 | refused: Sketch#3 comes after Sketch#1 (link to earlier sketches only) · Drag: Line2.Start is linked to Sketch#1 (move it there) · Sketch#1 has no point 40; file and history untouched | 0 | 🟢 |
| W28 | save → load → save byte-equal with [[Steps.Sketch.Links]] { Point, Sketch, Index } and the construction line's LinkSketch / LinkIndex | 0 | 🟢 |
| W28 | Unlink Line2.Start of Line1: DOF 3, start stays at (4, 2) when D1 = 5 (gap 0.000e+00); undo relinks | 0 | 🟢 |
| W29 | Revolve1 · Rectangle1 · 360° about sketch y-axis · New: proposed axis sketch y-axis, V 3π (rel. 0.000e+00), New, sound | 0 | 🟢 |
| W29 | Revolve2 · Rectangle2 · 360° about Line4 · Union: axis by line identifier 4, V 4.625π (rel. 1.223e-16), one sound body; undo / redo bit-equal | 1.22e-16 | 🟢 |
| W29 | Revolve1 · Rectangle1 · 90° about sketch x-axis · New body: proposed sketch x-axis, V 3π/2 (rel. 1.885e-16), sound | 1.88e-16 | 🟢 |
| W29 | refused: the angle must lie in (0°, 360°] · the angle must lie in (0°, 360°] · Revolve#2: Revolve: profile crosses the axis · choose the profile: focus a sketch step · nothing to subtract from yet (no body); file untouched | 0 | 🟢 |
| W29 | save → load → save byte-equal; AxisOrigin / AxisDirection only on the y- / x-axis revolves (1 in the union file) | 0 | 🟢 |
| W30 | Evolution1 · Rectangle1 along Line1 · New: path proposed Sketch#2, V 1 (rel. 0.000e+00), New, sound | 0 | 🟢 |
| W30 | Evolution1 · Circle1 along Arc1 · New body: the arc is stored from (2, 2) (counter-clockwise), the sweep starts at its end nearer the profile; V π²/16 (rel. 5.276e-09), sound | 5.28e-09 | 🟢 |
| W30 | Evolution1 · Rectangle2 along Line1 · Difference: block 2 → 1.875000 vs 1.875 (rel. 1.184e-16), one sound body | 1.18e-16 | 🟢 |
| W30 | refused: the path must be another sketch than the profile · choose the path: click a one-curve sketch or select a curve · Evolution#5: Evolution: path sketch Sketch#1 has 4 curves (🚧 one line, arc or spline; J joins a chain into a spline first) · Evolution#5: Evolution: the path starts along the profile plane (it must leave the plane) · nothing to subtract from yet (no body); file untouched | 0 | 🟢 |
| W30 | save → load → save byte-equal with the Evolution (16) steps and Path = 'Sketch#2' | 0 | 🟢 |
| W31 | 14 commands → 27 steps: 14 identical to a full replay, 14 resumed at the new step | 0 | 🟢 |
| W31 | tail edit Fillet1.Size 0.15 → 0.12: resumed at 26/27 · 659.3 ms vs full 1880.9 ms · 2.9× · identical | 2.85 | 🟢 |
| W31 | middle edit Cylinder6.Radius 0.30 → 0.32: resumed at 12/27 · 1609.0 ms vs full 1880.9 ms · 1.2× · identical | 1.17 | 🟢 |
| W31 | undo (Cylinder6.Radius back): resumed at 27/27 · 1.1 ms vs full 1880.9 ms · 1649.4× · identical | 1.65e+03 | 🟢 |
| W31 | redo: resumed at 27/27 · 1.2 ms vs full 1880.9 ms · 1581.8× · identical | 1.58e+03 | 🟢 |
| W31 | undo again: resumed at 27/27 · 1.1 ms vs full 1880.9 ms · 1755.5× · identical | 1.76e+03 | 🟢 |
| W31 | preview clone (bevel / move / sweep staging): resumed at 27/27 · 0.6 ms · 3179.6× | 3.18e+03 | 🟢 |
| W31 | preview fillets the back edge Edge(Extrude#2.Cap(End)\|Extrude#2.Side(Line3)): resumed at 27 (Fillet1 re-captured in place) · 725.9 ms · source untouched | 0 | 🟢 |
| W31 | 25 undo entries, 8 with checkpoints; 10 undos identical, 8 restored every step (older ones resume at the edited step) | 0 | 🟢 |
| W31 | bonnet (12 steps, full 3.0 ms): pole move resumed at 5 · 2.0 ms; undo restored all · 0.1 ms · identical | 25.6 | 🟢 |
| W32 | ray at the hidden back-bottom edge: opaque → Extrude#2.Cap(End) · x-ray → Edge(Extrude#2.Cap(Start)\|Extrude#2.Side(Line3)) (1.600781 farther) | 0 | 🟢 |
| W32 | ray at the visible front-top edge: opaque = x-ray → Edge(Extrude#2.Cap(End)\|Extrude#2.Side(Line1)) | 0 | 🟢 |
| W32 | full box without x-ray → 9 edges, none at the far corner (closed form 9 of 12) | 0 | 🟢 |
| W32 | full box with x-ray → 12 edges (the opaque 9 among them) | 0 | 🟢 |
| W32 | empty 40 × 40 px corner box → 0 / 0 edges | 0 | 🟢 |
| W33 | nothing selected → Body Block1 · V 8.000000 · A 28.000000 · I₁ 3.333333 | 4.44e-16 | 🟢 |
| W33 | face Extrude#2.Cap(End) → area 8.000000 · perimeter 12.000000 | 4.44e-16 | 🟢 |
| W33 | edge Edge(Extrude#2.Cap(End)\|Extrude#2.Side(Line1)) → length 4.000000 | 4.44e-16 | 🟢 |
| W33 | Extrude#2.Cap(End) + Extrude#2.Cap(Start) → distance 1.000000 · angle 180.000000° | 0 | 🟢 |
| W33 | Extrude#2.Cap(End) + Extrude#2.Side(Line1) → distance 0.000000 · angle 90.000000° | 0 | 🟢 |
| W33 | Extrude#2.Cap(Start) + Edge(Extrude#2.Cap(End)\|Extrude#2.Side(Line1)) → distance 1.000000 · angle 0.000000° | 0 | 🟢 |
| W33 | Edge(Extrude#2.Cap(End)\|Extrude#2.Side(Line1)) + Edge(Extrude#2.Cap(Start)\|Extrude#2.Side(Line3)) → distance 2.236068 (√5) · angle 0.000000° | 0 | 🟢 |
| W33 | 3 face(s) + 0 edge(s) → total area 20.000000 | 3.55e-16 | 🟢 |
| W33 | section x = 1.5 → A 2.000000 · P 6.000000 · Iuu 0.166667 · Ivv 0.666667 | 3.6e-13 | 🟢 |
| W33 | section z = 0.25 → A 8.000000 · Ivv 10.666667 | 2.27e-13 | 🟢 |
| W33 | section on Extrude#2.Cap(End) at −0.25 → plane z 0.750000 · A 8.000000 | 2.27e-13 | 🟢 |
| W33 | x = 10 → "Section: the plane misses every shown body" · face F-none → "Section: F-none is not a planar face of a shown body" | 0 | 🟢 |
| W33 | two bodies, z = ½ → A 12.000000 · x̄ 3.666667 · Ivv 78.666667 (parallel axis 78.666667) | 3.8e-12 | 🟢 |
| W33 | body Extrude#4 picked → Body Block2 · V 4.000000 | 1.33e-15 | 🟢 |
| W33 | second body hidden → section A 8.000000 over 1 body | 8.88e-16 | 🟢 |
| W34 | clip of a solid block: plane copied, 6 faces, none marked sheet | 0 | 🟢 |
| W34 | down at x = 3: unsectioned → Extrude#2.Cap(End) · sectioned → (miss) | 0 | 🟢 |
| W34 | along −x through the opening → (cap) at 5.000000 (back face x = 0: 5; plane crossing: 4) · cap name "" | 0 | 🟢 |
| W34 | oblique ray crossing the plane outside the material → Extrude#2.Cap(End) (the kept top, not the cap) at 3.201562 | 0 | 🟢 |
| W34 | x = 4 top edge: unsectioned → Edge(Extrude#2.Cap(End)\|Extrude#2.Side(Line2)) · sectioned → (miss) | 0 | 🟢 |
| W34 | x = 0 top edge: sectioned = unsectioned → Edge(Extrude#2.Cap(End)\|Extrude#2.Side(Line4)) | 0 | 🟢 |
| W34 | same ray with sheet faces → Extrude#2.Side(Line4) (its back, no cap) | 0 | 🟢 |
| W34 | the body summarised as a sheet → 6 / 6 faces marked | 0 | 🟢 |
| W34 | full box, x-ray, sectioned → 8 edges, all reaching x ≤ 1 (closed form 8 of 12) | 0 | 🟢 |
| W35 | block corner (4, 0, 1), pointer 3.6 px off → Endpoint (4, 0, 1) · Edge Edge(Extrude#2.Side(Line1)\|Extrude#2.Side(Line2)) · 3.6 px | 0 | 🟢 |
| W35 | front top edge midpoint (2, 0, 1) → Midpoint (2, 0, 1) · Edge Edge(Extrude#2.Cap(End)\|Extrude#2.Side(Line1)) · 3.6 px | 0 | 🟢 |
| W35 | top face centre (2, 1, 1) (planar face centroid) → Centre (2, 1, 1) · Face Extrude#2.Cap(End) centre · 3.6 px | 2.22e-16 | 🟢 |
| W35 | cylinder top rim centre (6, 1, 1) → Centre (6, 1, 1) · Edge Edge(Extrude#4.Cap(End)\|Extrude#4.Side(Circle1)) · 3.6 px | 0 | 🟢 |
| W35 | sketch lines cross at (−2, −3, 0) → Intersection (-2, -3, 0) · Line1 × Line1 · Line2 · 3.6 px | 0 | 🟢 |
| W35 | sketch line end (−1, −4, 0) → Endpoint (-1, -4, 0) · Line1 · 3.6 px | 0 | 🟢 |
| W35 | sketch circle centre (−5, −3, 0) → Centre (-5, -3, 0) · Circle1 · 3.6 px | 0 | 🟢 |
| W35 | on edge near (1.3, 0, 1): \|y\| + \|z − 1\| → On curve (1.32778686671, 0, 1) · Edge Edge(Extrude#2.Cap(End)\|Extrude#2.Side(Line1)) · 0.4 px | 0 | 🟢 |
| W35 | on top face near (2.9, 1.4, 1): \|z − 1\|, outward normal ẑ → On face (2.9, 1.4, 1) · Face Extrude#2.Cap(End) · 0.0 px | 0 | 🟢 |
| W35 | on top rim near angle −0.6: \|\|ρ\| − 0.5\| + \|z − 1\| → On curve (6.42322840595, 0.733771308088, 1) · Edge Edge(Extrude#4.Cap(End)\|Extrude#4.Side(Circle1)) · 0.9 px | 2.22e-16 | 🟢 |
| W35 | on cylinder side near angle −0.9, z 0.45: \|\|ρ\| − 0.5\| (exact ray hit, not the facets) → On face (6.31080498414, 0.608336545186, 0.45) · Face Extrude#4.Side(Circle1) · 0.0 px | 5.55e-17 | 🟢 |
| W35 | tangent from (−5, −0.5, 0) to circle r 0.8: \|\|T − C\| − r\| + \|(T − P)·(T − C)\| → Tangent (-4.24206596593, -2.744, 0) · Circle1 · 2.2 px | 5.55e-16 | 🟢 |
| W35 | perpendicular from (−4.5, −4.5, 0) to Line (−3, −2) → (−1, −4): foot (−2.5, −2.5, 0) → Perpendicular (-2.5, -2.5, 0) · Line1 · 2.8 px | 0 | 🟢 |
| W35 | Hermite curve midpoint: \|L₁ − L₂\| / L (L₁ 2.570515, L₂ 2.570515) | 1.45e-14 | 🟢 |
| W35 | hidden corner (0, 2, 0): plain → On face (0.996318810024, 0.798538383255, 1) · Face Extrude#2.Cap(End) · 0.0 px · x-ray → Endpoint (0, 2, 0) · Edge Edge(Extrude#2.Cap(Start)\|Extrude#2.Side(Line3)) · 1.4 px | 0 | 🟢 |
| W35 | corner (4, 0, 1) behind the section x ≤ 3 → none (0, 0, 0) ·  · 0.0 px | 0 | 🟢 |
| W35 | block excluded (Move) at its corner → none (0, 0, 0) ·  · 0.0 px | 0 | 🟢 |
| W35 | sketch Sketch#7 excluded (its point being dragged): centre → none (0, 0, 0) ·  · 0.0 px · corner → Endpoint (4, 0, 1) · Edge Edge(Extrude#2.Side(Line1)\|Extrude#2.Side(Line2)) · 1.4 px | 0 | 🟢 |
| W35 | endpoints disabled at the corner → Intersection (4, 0, 1) · Edge Edge(Extrude#2.Side(Line1)\|Extrude#2.Side(Line2)) × Edge Edge(Extrude#2.Cap(End)\|Extrude#2.Side(Line2)) · 1.4 px | 0 | 🟢 |
| W35 | placed drawing point (−6, −6, 0) → Endpoint (-6, -6, 0) · Drawing point 1 · 2.8 px | 0 | 🟢 |
| W35 | x lock through (4, 0, 1), pointer over (5.5, 0, 1) → t 1.500000 | 3.8e-14 | 🟢 |
| W35 | catalogue: 24 curves, 39 fixed points | 0 | 🟢 |
| W36 | bracket + pin + lever: 3 bodies | 0 | 🟢 |
| W36 | origins: pin top rim → circle centre (6, 1.25, 1.5); hole top rim → (3, 1.25, 1); lever top → plane centroid (7, 4.2, 0.2); pin cap → (6, 1.25, 1.5) | 0 | 🟢 |
| W36 | hole rim z axis leaves the bracket top (+z): (0, 0, 1) on Extrude#2 | 0 | 🟢 |
| W36 | pin into the hole (revolute, flipped, offset 0.25): centroid (3, 1.25, 0.5) = (3, 1.25, 0.5) | 7.11e-16 | 🟢 |
| W36 | 1 regenerated joint, origins 0.25 apart (the offset), 1 undo entry; step Joint#13 captioned Revolute1 | 0 | 🟢 |
| W36 | revolute parameters: Revolute1.Angle Revolute1.Offset  | 0 | 🟢 |
| W36 | lever on the pin cap at 90°: bounds (2.8, 0.25, 1.25) … (3.2, 2.25, 1.45) = (2.8, 0.25, 1.25) … (3.2, 2.25, 1.45) | 3.56e-14 | 🟢 |
| W36 | lever + bracket: Clear, clearance 0.250000 | 0 | 🟢 |
| W36 | bracket + pin: Clear, clearance 0.050000 (0.3 − 0.25) | 2.57e-16 | 🟢 |
| W36 | pin offset −0.1 → lever + bracket: Overlap, overlap 0.057921 = (0.8 − 2 (0.2 √0.05 + 0.09 asin ⅔)) · 0.1, 24 overlap edges drawn | 1.55e-15 | 🟢 |
| W36 | the lever followed the pin: bed at z 0.900000 = 0.9 | 1.11e-16 | 🟢 |
| W36 | bracket moved +1 x: pin centroid (4, 1.25, 0.15) = (4, 1.25, 0.15), lever x from 3.800000 = 3.8, joints still assembled | 1.79e-14 | 🟢 |
| W36 | refused: "both joint origins are on Cylinder3 (pick one on each body)" · "Joint#16: Joint: Extrude#10 is already jointed to Extrude#2 (🚧 one joint per placed body)"; no undo entry | 0 | 🟢 |
| W36 | save → load: 2 joints with equal kind / flip / values / bodies, pin and lever centroids within 0.000000; the second round trip is byte-equal (JointFlipped = true for the pin) | 0 | 🟢 |
| W36 | 2 × undo → pin centroid (3, 1.25, 0.5) again | 7.11e-16 | 🟢 |

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
