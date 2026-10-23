# Drawing validation

🟢 Priority 11 D1–D2: orthographic views, exact silhouettes, ray-cast hidden lines, section views with hatching; drawing sheets with automatic dimensions, title block and SVG / DXF export (4.68 s)

## Gates

| | Gate | Measures | Limit | Worst | Checks | Failures |
|---|---|---|---|---|---|---|
| 🟢 | DR1 | A block 4 × 2.5 × 1 in the six principal views and the isometric: visible length = both coincident outlines (2 × perimeter), nothing hidden; isometric: the three edges at the far corner (0, 2.5, 0) hidden, 9 visible, every edge foreshortened by √(2/3); view box = the projected extent | ≤ 10⁻¹² | 0.00e+00 | 7 | 0 |
| 🟢 | DR2 | The block minus a through hole r 0.3 at (3, 1.25): top view — both hole circles visible, every point at r from the centre, angular cover 2 × 2π; front / right view — exactly two hidden silhouettes of length 1 at x = 2.7, 3.3 / y = 0.95, 1.55, total hidden = 2 | ≤ 10⁻¹² | 7.11e-15 | 4 | 0 |
| 🟢 | DR3 | Silhouettes: a cylinder r 0.5 (front: two visible lines at x = ±0.5 of length 2; top: none), a sphere r 1 in front, top and isometric views (every point at radius 1, angular cover 2π), the cylinder in the isometric view (two lines at view x = ±0.5) | ≤ 10⁻¹² · cover ≤ 10⁻⁶ | 4.97e-14 | 6 | 0 |
| 🟢 | DR4 | Occlusion between bodies: block B [1, 4] × [3, 4] × [0.5, 1.5] behind block A [0, 2] × [0, 1] × [0, 1] in the front view — hidden length 3 (bottom x ∈ [1, 2], left z ∈ [0.5, 1], front and back), visible 25, every hidden stroke exactly (1, 0.5)→(2, 0.5) or (1, 0.5)→(1, 1); hidden lines switched off drops them all | ≤ 10⁻¹² | 0.00e+00 | 2 | 0 |
| 🟢 | DR5 | Section view through the hole axis (plane y = 1.25 toward the front viewer): cap area 4 × 1 − 0.6 × 1 = 3.4, every hatch end on the cap boundary, every hatch midpoint inside, hatch at 45° and the requested spacing, the section outline closed, the source partition untouched | ≤ 10⁻¹² | 1.33e-14 | 4 | 0 |
| 🟢 | DR6 | Tangent edges: a box edge filleted r 0.3 — exactly its two run-out edges flagged tangent (a chamfer: none), switched off they vanish; the right view along the fillet shows its quarter-circle caps at r 0.3 about (0.3, 1.7) and no silhouette on the fillet | ≤ 10⁻¹² · count exact | 1.78e-15 | 3 | 0 |
| 🟢 | DR7 | Refusals name the cause: no bodies, a view frame that is not orthonormal, a left-handed frame, a section plane that misses every body, a section plane with every body in front of it | named | 0.00e+00 | 5 | 0 |
| 🟢 | DR8 | Projection time of every view above (block, hole, sphere, occlusion, section, fillet) and composition time of every sheet below | ≤ 5 s per view / sheet | 7.73e-01 | 26 | 0 |
| 🟢 | DR9 | Sheet layout of the bracket on A4, third and first angle: Top shares Front's sheet x mapping and Right its y mapping exactly, placed per ISO 5456-2; every model corner at origin + 20 × view coordinates; footprints inside the drawing area and 12 mm apart; 20:1 is the largest standard scale that fits (50:1 refused); ISO 216 paper and ISO 5455 scale labels | ≤ 10⁻¹² mm | 1.42e-14 | 9 | 0 |
| 🟢 | DR10 | Automatic dimensions: bracket 4, 1 (front), 2.5, 3, 1.25, ⌀0.6 (top) — value, feature points, dimension line = 20 × value, arrow tips on the line ends; no value box touching another value, a view, the title block or the frame; centre marks r + 2 mm; a fillet's R0.3 in the right view; a cylinder's ⌀1 in the top view | ≤ 2 × 10⁻¹¹ mm | 5.86e-14 | 18 | 0 |
| 🟢 | DR11 | Frame (20 mm binding edge, 10 mm elsewhere), ISO 7200 title block 180 × 36 mm in the lower-right corner with title, number, revision, scale, paper, company; ISO 5456-2 projection symbol: circles r 2 / 4 left of the cone (third angle) or right of it (first angle), cone ends 4 / 8 mm, inside its cell | ≤ 10⁻¹² mm | 1.82e-13 | 4 | 0 |
| 🟢 | DR12 | Section A-A on the sheet: the cutting-plane chain across the top view at model y = 1.25, 5 mm past the outline, 7 mm arrows toward the kept material (+y), letters A, label SECTION A-A; hatching at 45° and 3 mm on paper; cap area 1360 mm² | ≤ 10⁻¹² mm · spacing ≤ 10⁻⁹ | 2.05e-12 | 4 | 0 |
| 🟢 | DR13 | SVG export read back by an independent parser: header (A4 in mm), ten layer groups in order with ISO 128 widths and dashes, every polyline, arrowhead and text (content, position, size, rotation) — XML escaping included | ≤ 5 × 10⁻⁷ mm (half the 6th decimal) | 4.99e-07 | 2 | 0 |
| 🟢 | DR14 | DXF R12 export read back by an independent group-code parser: AC1009 header, LTYPE / LAYER / STYLE tables, every LINE / POLYLINE / SOLID / TEXT in drawing order, ⌀ as %%c; ExportDrawingFile writes the same bytes | ≤ 10⁻¹² mm | 5.12e-13 | 3 | 0 |
| 🟢 | DR15 | Sheet refusals name the cause: no bodies, a zero model unit, a negative scale, a scale too large for the paper, a part too large even at 1:1000, a section plane that misses, exporting a refused sheet, an unknown file extension | named | 0.00e+00 | 8 | 0 |

## Checks

| | Gate | Check | Detail |
|---|---|---|---|
| 🟢 | DR1 | Block · Front | visible 20 (2 × 10), hidden 0, view box error 0.00e+00 |
| 🟢 | DR8 | Block · Front | 0.3 ms, 8 strokes, 424 rays |
| 🟢 | DR1 | Block · Back | visible 20 (2 × 10), hidden 0, view box error 0.00e+00 |
| 🟢 | DR8 | Block · Back | 0.3 ms, 8 strokes, 424 rays |
| 🟢 | DR1 | Block · Top | visible 26 (2 × 13), hidden 0, view box error 0.00e+00 |
| 🟢 | DR8 | Block · Top | 0.4 ms, 8 strokes, 548 rays |
| 🟢 | DR1 | Block · Bottom | visible 26 (2 × 13), hidden 0, view box error 0.00e+00 |
| 🟢 | DR8 | Block · Bottom | 0.4 ms, 8 strokes, 548 rays |
| 🟢 | DR1 | Block · Right | visible 14 (2 × 7), hidden 0, view box error 0.00e+00 |
| 🟢 | DR8 | Block · Right | 0.2 ms, 8 strokes, 300 rays |
| 🟢 | DR1 | Block · Left | visible 14 (2 × 7), hidden 0, view box error 0.00e+00 |
| 🟢 | DR8 | Block · Left | 0.2 ms, 8 strokes, 300 rays |
| 🟢 | DR1 | Block · Isometric | visible 18.3711730708738 (22.5·√⅔), hidden 6.12372435695795 (7.5·√⅔), hidden edges 3 |
| 🟢 | DR8 | Block · Isometric | 1.4 ms, 13 strokes, 716 rays |
| 🟢 | DR2 | Top view · hole circles | radius error 2.78e-16, cover 12.5663706143592 (4π = 12.5663706143592), hidden 0, silhouettes 0 |
| 🟢 | DR8 | Bracket · Top | 1.0 ms, 10 strokes, 862 rays |
| 🟢 | DR2 | Front view · hidden hole silhouettes | 2 hidden silhouettes, total 2, worst offset from x = {2.7, 3.3} 2.22e-16 |
| 🟢 | DR8 | Bracket · Front | 5.3 ms, 12 strokes, 1108 rays |
| 🟢 | DR2 | Right view · hidden hole silhouettes | 2 hidden silhouettes, total 2, worst offset from x = {0.95, 1.55} 2.22e-16 |
| 🟢 | DR8 | Bracket · Right | 5.5 ms, 12 strokes, 984 rays |
| 🟢 | DR2 | Isometric view · projects | 18 strokes, 8 hidden |
| 🟢 | DR8 | Bracket · Isometric | 6.6 ms, 18 strokes, 1396 rays |
| 🟢 | DR3 | Cylinder r 0.5 · front | 2 silhouettes, worst \|x\| − 0.5 / length − 2 = 0.00e+00 |
| 🟢 | DR8 | Cylinder · Front | 11.0 ms, 4 strokes, 1362 rays |
| 🟢 | DR3 | Cylinder r 0.5 · top (seen along its axis) | 0 silhouettes, rims cover 12.5663706143592 (4π) |
| 🟢 | DR3 | Cylinder r 0.5 · isometric | 2 silhouettes, worst \| \|x\| − 0.5 \| = 5.55e-17 |
| 🟢 | DR8 | Cylinder · Isometric | 13.3 ms, 6 strokes, 1442 rays |
| 🟢 | DR3 | Sphere r 1 · Front | 1 silhouette runs, radius error 4.44e-16, cover 6.28318530737 (2π) |
| 🟢 | DR8 | Sphere · Front | 3.2 ms, 1 strokes, 353 rays |
| 🟢 | DR3 | Sphere r 1 · Top | 1 silhouette runs, radius error 4.44e-16, cover 6.28318530718 (2π) |
| 🟢 | DR8 | Sphere · Top | 2.7 ms, 1 strokes, 229 rays |
| 🟢 | DR3 | Sphere r 1 · Isometric | 1 silhouette runs, radius error 8.88e-16, cover 6.28318530718 (2π) |
| 🟢 | DR8 | Sphere · Isometric | 2.9 ms, 1 strokes, 307 rays |
| 🟢 | DR4 | Block behind block · front | visible 25 (25), hidden 3 (3), worst hidden piece off (1, 0.5)→(2, 0.5) / (1, 0.5)→(1, 1) by 0.00e+00 |
| 🟢 | DR8 | Occlusion · Front | 0.8 ms, 20 strokes, 664 rays |
| 🟢 | DR4 | Hidden lines off | hidden 0, visible 25 (25) |
| 🟢 | DR5 | Cap area | 3.4 (3.4) over 2 caps |
| 🟢 | DR5 | Hatching | 38 hatch lines, worst end off the cap boundary 4.44e-16, worst slope error 1.33e-14, spacing error 1.42e-15, midpoints inside |
| 🟢 | DR5 | Section outline | 8 strokes, length 10.8 (10.8), worst point off the boundary 0.00e+00 |
| 🟢 | DR5 | Source partition untouched | bodies 3 → 3, faces 16 → 16 |
| 🟢 | DR8 | Bracket · Section A-A | 343.0 ms, 54 strokes, 866 rays |
| 🟢 | DR6 | BoxEdgeFillet · isometric | 2 tangent edges (2 expected), run-out ends off y = 0.3 / z = 1.7 by 1.67e-16, 0 left when switched off |
| 🟢 | DR8 | BoxEdgeFillet · Isometric | 3.4 ms, 16 strokes, 850 rays |
| 🟢 | DR6 | BoxEdgeFillet · right view caps | 2 quarter arcs, radius error 2.22e-16, mean cover 1.57079632679489 (π/2), 0 silhouettes |
| 🟢 | DR6 | BoxEdgeChamfer · isometric | 0 tangent edges (0 expected), run-out ends off y = 0.3 / z = 1.7 by 0.00e+00, 0 left when switched off |
| 🟢 | DR8 | BoxEdgeChamfer · Isometric | 1.3 ms, 15 strokes, 739 rays |
| 🟢 | DR7 | No bodies | Drawing: no bodies to project |
| 🟢 | DR7 | Skewed frame | Drawing: the view frame is not a right-handed orthonormal frame |
| 🟢 | DR7 | Left-handed frame | Drawing: the view frame is not a right-handed orthonormal frame |
| 🟢 | DR7 | Plane beyond the body | Drawing: the section plane misses every body |
| 🟢 | DR7 | Plane in front of the body | Drawing: the section plane misses every body |
| 🟢 | DR8 | Sheet · Third angle bracket A4 | 16.2 ms, 77 lines, 23 texts |
| 🟢 | DR9 | Third angle · projection alignment | Top x origin − Front x origin = 0, Right y origin − Front y origin = 0, Top above Front, Right on its right |
| 🟢 | DR9 | Third angle · sheet mapping | scale 20:1, worst corner / box error 1.42e-14 mm |
| 🟢 | DR9 | Third angle · footprints | nearest footprint to the area edge 11.3 mm (≥ 0), nearest pair 12 mm (≥ 12) |
| 🟢 | DR9 | Third angle · largest standard scale | 20:1 chosen; 50:1 requested → Drawing: the views do not fit on A4 at 50:1 |
| 🟢 | DR8 | Sheet · First angle bracket A4 | 14.3 ms, 77 lines, 23 texts |
| 🟢 | DR9 | First angle · projection alignment | Top x origin − Front x origin = 0, Right y origin − Front y origin = 0, Top below Front, Right on its left |
| 🟢 | DR9 | First angle · sheet mapping | scale 20:1, worst corner / box error 0.00e+00 mm |
| 🟢 | DR9 | First angle · footprints | nearest footprint to the area edge 10.05 mm (≥ 0), nearest pair 12 mm (≥ 12) |
| 🟢 | DR9 | First angle · largest standard scale | 20:1 chosen; 50:1 requested → Drawing: the views do not fit on A4 at 50:1 |
| 🟢 | DR9 | Paper sizes and scale labels | ISO 216 landscape A4 A3 A2 A1 A0, error 0 mm; 50:1 1:1 1:5 1:1000 |
| 🟢 | DR10 | Third angle · Front 4 | value 4 (error 0.00e+00), feature points off by 0.00e+00 mm, line − 20 × value 0.00e+00 mm, arrow tips off by 0.00e+00 mm |
| 🟢 | DR10 | Third angle · Front 1 | value 1 (error 0.00e+00), feature points off by 0.00e+00 mm, line − 20 × value 0.00e+00 mm, arrow tips off by 0.00e+00 mm |
| 🟢 | DR10 | Third angle · Top 2.5 | value 2.5 (error 0.00e+00), feature points off by 0.00e+00 mm, line − 20 × value 1.42e-14 mm, arrow tips off by 0.00e+00 mm |
| 🟢 | DR10 | Third angle · Top 3 | value 2.9999999999999978 (error 2.22e-15), feature points off by 4.26e-14 mm, line − 20 × value 4.26e-14 mm, arrow tips off by 0.00e+00 mm |
| 🟢 | DR10 | Third angle · Top 1.25 | value 1.2499999999999993 (error 6.66e-16), feature points off by 2.84e-14 mm, line − 20 × value 1.42e-14 mm, arrow tips off by 0.00e+00 mm |
| 🟢 | DR10 | Third angle · Top ⌀0.6 | value 0.60000000000000409 (error 4.11e-15), feature points off by 5.12e-14 mm, line − 20 × value 5.86e-14 mm, arrow tips off by 0.00e+00 mm |
| 🟢 | DR10 | Third angle · no collisions | 6 dimensions, nearest value box to another value / view / title block / frame 3.828 mm (> 0), lines past the frame 0.00e+00 mm |
| 🟢 | DR10 | Third angle · centre mark | 2 centre-mark arms of 2 × (r + 2 mm) = 16 mm |
| 🟢 | DR10 | First angle · Front 4 | value 4 (error 0.00e+00), feature points off by 0.00e+00 mm, line − 20 × value 0.00e+00 mm, arrow tips off by 0.00e+00 mm |
| 🟢 | DR10 | First angle · Front 1 | value 1 (error 0.00e+00), feature points off by 0.00e+00 mm, line − 20 × value 0.00e+00 mm, arrow tips off by 0.00e+00 mm |
| 🟢 | DR10 | First angle · Top 2.5 | value 2.5 (error 0.00e+00), feature points off by 0.00e+00 mm, line − 20 × value 0.00e+00 mm, arrow tips off by 0.00e+00 mm |
| 🟢 | DR10 | First angle · Top 3 | value 2.9999999999999978 (error 2.22e-15), feature points off by 2.84e-14 mm, line − 20 × value 2.84e-14 mm, arrow tips off by 0.00e+00 mm |
| 🟢 | DR10 | First angle · Top 1.25 | value 1.2499999999999993 (error 6.66e-16), feature points off by 1.42e-14 mm, line − 20 × value 1.42e-14 mm, arrow tips off by 0.00e+00 mm |
| 🟢 | DR10 | First angle · Top ⌀0.6 | value 0.60000000000000409 (error 4.11e-15), feature points off by 3.18e-14 mm, line − 20 × value 3.82e-14 mm, arrow tips off by 0.00e+00 mm |
| 🟢 | DR10 | First angle · no collisions | 6 dimensions, nearest value box to another value / view / title block / frame 3.828 mm (> 0), lines past the frame 0.00e+00 mm |
| 🟢 | DR10 | First angle · centre mark | 2 centre-mark arms of 2 × (r + 2 mm) = 16 mm |
| 🟢 | DR8 | Sheet · Fillet A4 | 7.8 ms, 69 lines, 21 texts |
| 🟢 | DR10 | BoxEdgeFillet · R | R0.3 in the Right view, scale 10:1, value / centre (0.3, 1.7) / tip error 2.84e-15 |
| 🟢 | DR8 | Sheet · Cylinder A4 | 28.6 ms, 39 lines, 20 texts |
| 🟢 | DR10 | Cylinder · ⌀ alone sizes the end view | Front 1 Front 2 Top ⌀1 (no repeated size or position), scale 20:1, ⌀ error 0.00e+00 |
| 🟢 | DR11 | Third angle · frame and title block | frame 20 / 10 mm, title block 180 × 36 mm in the lower-right corner, error 0.00e+00 mm; title, number, revision, scale 20:1, A4 mm, company, THIRD ANGLE |
| 🟢 | DR11 | Third angle · projection symbol | circles r 2 / 4 (error 1.78e-14), cone ends 4 / 8 mm, circles left of the cone, inside its cell |
| 🟢 | DR11 | First angle · frame and title block | frame 20 / 10 mm, title block 180 × 36 mm in the lower-right corner, error 0.00e+00 mm; title, number, revision, scale 20:1, A4 mm, company, FIRST ANGLE |
| 🟢 | DR11 | First angle · projection symbol | circles r 2 / 4 (error 1.82e-13), cone ends 4 / 8 mm, circles right of the cone, inside its cell |
| 🟢 | DR8 | Sheet · Third angle bracket section A4 | 732.1 ms, 112 lines, 25 texts |
| 🟢 | DR12 | Third angle · cutting line | chain at y = Top origin + 20 × 1.25, 5 mm past the outline: error 0.00e+00 mm; 2 arrows of 7 mm toward +y (error 0.00e+00), 2 heads up, 2 letters A, label SECTION A-A |
| 🟢 | DR12 | Third angle · hatching | 26 hatch lines, slope error 1.04e-14, spacing off a 3 mm multiple 1.77e-14 mm, cap area 1360 mm² (1360) |
| 🟢 | DR8 | Sheet · First angle bracket section A4 | 773.1 ms, 112 lines, 25 texts |
| 🟢 | DR12 | First angle · cutting line | chain at y = Top origin + 20 × 1.25, 5 mm past the outline: error 0.00e+00 mm; 2 arrows of 7 mm toward +y (error 0.00e+00), 2 heads up, 2 letters A, label SECTION A-A |
| 🟢 | DR12 | First angle · hatching | 26 hatch lines, slope error 1.32e-14, spacing off a 3 mm multiple 1.60e-14 mm, cap area 1360 mm² (1360) |
| 🟢 | DR13 | Bracket sheet | 48486 bytes, 77 polylines, 11 arrowheads, 23 texts read back; worst coordinate / size / angle error 4.99e-07, A4 mm header, 10 layer groups in order, hidden dashed 3 1.5 |
| 🟢 | DR13 | Bracket section sheet (with an XML-escaped note) | 55054 bytes, 112 polylines, 13 arrowheads, 26 texts read back; worst coordinate / size / angle error 4.99e-07, A4 mm header, 10 layer groups in order, hidden dashed 3 1.5 |
| 🟢 | DR14 | Bracket sheet | 60 LINE, 17 POLYLINE, 11 SOLID, 23 TEXT; worst coordinate / height / angle error 5.12e-13, AC1009 header, LTYPE / LAYER / STYLE tables, EOF, ⌀ as %%c |
| 🟢 | DR14 | Bracket section sheet | 92 LINE, 20 POLYLINE, 13 SOLID, 25 TEXT; worst coordinate / height / angle error 4.97e-13, AC1009 header, LTYPE / LAYER / STYLE tables, EOF, ⌀ as %%c |
| 🟢 | DR14 | ExportDrawingFile | the .svg and .dxf files equal the writers' strings byte for byte |
| 🟢 | DR15 | No bodies | Drawing: no bodies to project |
| 🟢 | DR15 | Model unit 0 | Drawing: the model unit must be a positive length |
| 🟢 | DR15 | Scale −2 | Drawing: the scale must be positive (0 picks one) |
| 🟢 | DR15 | Bracket at 50:1 on A4 | Drawing: the views do not fit on A4 at 50:1 |
| 🟢 | DR15 | A 2000 km slab | Drawing: the views do not fit on A4 even at 1:1000 |
| 🟢 | DR15 | Section plane beyond the body | Drawing: the section plane misses every body |
| 🟢 | DR15 | Export of a refused sheet | Export: the sheet was refused (Drawing: no bodies to project) |
| 🟢 | DR15 | Unknown extension | Export: unknown drawing format (use .svg or .dxf) |
