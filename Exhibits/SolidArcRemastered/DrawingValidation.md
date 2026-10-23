# Drawing validation

🟢 Priority 11 D1: orthographic views, exact silhouettes, ray-cast hidden lines and section views with hatching (1.34 s)

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
| 🟢 | DR8 | Projection time of every view above (block, hole, sphere, occlusion, section, fillet) | ≤ 5 s per view | 3.58e-01 | 20 | 0 |

## Checks

| | Gate | Check | Detail |
|---|---|---|---|
| 🟢 | DR1 | Block · Front | visible 20 (2 × 10), hidden 0, view box error 0.00e+00 |
| 🟢 | DR8 | Block · Front | 0.5 ms, 8 strokes, 424 rays |
| 🟢 | DR1 | Block · Back | visible 20 (2 × 10), hidden 0, view box error 0.00e+00 |
| 🟢 | DR8 | Block · Back | 0.3 ms, 8 strokes, 424 rays |
| 🟢 | DR1 | Block · Top | visible 26 (2 × 13), hidden 0, view box error 0.00e+00 |
| 🟢 | DR8 | Block · Top | 0.4 ms, 8 strokes, 548 rays |
| 🟢 | DR1 | Block · Bottom | visible 26 (2 × 13), hidden 0, view box error 0.00e+00 |
| 🟢 | DR8 | Block · Bottom | 0.4 ms, 8 strokes, 548 rays |
| 🟢 | DR1 | Block · Right | visible 14 (2 × 7), hidden 0, view box error 0.00e+00 |
| 🟢 | DR8 | Block · Right | 0.2 ms, 8 strokes, 300 rays |
| 🟢 | DR1 | Block · Left | visible 14 (2 × 7), hidden 0, view box error 0.00e+00 |
| 🟢 | DR8 | Block · Left | 0.3 ms, 8 strokes, 300 rays |
| 🟢 | DR1 | Block · Isometric | visible 18.3711730708738 (22.5·√⅔), hidden 6.12372435695795 (7.5·√⅔), hidden edges 3 |
| 🟢 | DR8 | Block · Isometric | 1.9 ms, 13 strokes, 716 rays |
| 🟢 | DR2 | Top view · hole circles | radius error 2.78e-16, cover 12.5663706143592 (4π = 12.5663706143592), hidden 0, silhouettes 0 |
| 🟢 | DR8 | Bracket · Top | 0.9 ms, 10 strokes, 862 rays |
| 🟢 | DR2 | Front view · hidden hole silhouettes | 2 hidden silhouettes, total 2, worst offset from x = {2.7, 3.3} 2.22e-16 |
| 🟢 | DR8 | Bracket · Front | 5.2 ms, 12 strokes, 1108 rays |
| 🟢 | DR2 | Right view · hidden hole silhouettes | 2 hidden silhouettes, total 2, worst offset from x = {0.95, 1.55} 2.22e-16 |
| 🟢 | DR8 | Bracket · Right | 5.4 ms, 12 strokes, 984 rays |
| 🟢 | DR2 | Isometric view · projects | 18 strokes, 8 hidden |
| 🟢 | DR8 | Bracket · Isometric | 6.1 ms, 18 strokes, 1396 rays |
| 🟢 | DR3 | Cylinder r 0.5 · front | 2 silhouettes, worst \|x\| − 0.5 / length − 2 = 0.00e+00 |
| 🟢 | DR8 | Cylinder · Front | 10.6 ms, 4 strokes, 1362 rays |
| 🟢 | DR3 | Cylinder r 0.5 · top (seen along its axis) | 0 silhouettes, rims cover 12.5663706143592 (4π) |
| 🟢 | DR3 | Cylinder r 0.5 · isometric | 2 silhouettes, worst \| \|x\| − 0.5 \| = 5.55e-17 |
| 🟢 | DR8 | Cylinder · Isometric | 12.1 ms, 6 strokes, 1442 rays |
| 🟢 | DR3 | Sphere r 1 · Front | 1 silhouette runs, radius error 4.44e-16, cover 6.28318530737 (2π) |
| 🟢 | DR8 | Sphere · Front | 2.5 ms, 1 strokes, 353 rays |
| 🟢 | DR3 | Sphere r 1 · Top | 1 silhouette runs, radius error 4.44e-16, cover 6.28318530718 (2π) |
| 🟢 | DR8 | Sphere · Top | 1.6 ms, 1 strokes, 229 rays |
| 🟢 | DR3 | Sphere r 1 · Isometric | 1 silhouette runs, radius error 8.88e-16, cover 6.28318530718 (2π) |
| 🟢 | DR8 | Sphere · Isometric | 2.5 ms, 1 strokes, 307 rays |
| 🟢 | DR4 | Block behind block · front | visible 25 (25), hidden 3 (3), worst hidden piece off (1, 0.5)→(2, 0.5) / (1, 0.5)→(1, 1) by 0.00e+00 |
| 🟢 | DR8 | Occlusion · Front | 0.6 ms, 20 strokes, 664 rays |
| 🟢 | DR4 | Hidden lines off | hidden 0, visible 25 (25) |
| 🟢 | DR5 | Cap area | 3.4 (3.4) over 2 caps |
| 🟢 | DR5 | Hatching | 38 hatch lines, worst end off the cap boundary 4.44e-16, worst slope error 1.33e-14, spacing error 1.42e-15, midpoints inside |
| 🟢 | DR5 | Section outline | 8 strokes, length 10.8 (10.8), worst point off the boundary 0.00e+00 |
| 🟢 | DR5 | Source partition untouched | bodies 3 → 3, faces 16 → 16 |
| 🟢 | DR8 | Bracket · Section A-A | 358.0 ms, 54 strokes, 866 rays |
| 🟢 | DR6 | BoxEdgeFillet · isometric | 2 tangent edges (2 expected), run-out ends off y = 0.3 / z = 1.7 by 1.67e-16, 0 left when switched off |
| 🟢 | DR8 | BoxEdgeFillet · Isometric | 3.7 ms, 16 strokes, 850 rays |
| 🟢 | DR6 | BoxEdgeFillet · right view caps | 2 quarter arcs, radius error 2.22e-16, mean cover 1.57079632679489 (π/2), 0 silhouettes |
| 🟢 | DR6 | BoxEdgeChamfer · isometric | 0 tangent edges (0 expected), run-out ends off y = 0.3 / z = 1.7 by 0.00e+00, 0 left when switched off |
| 🟢 | DR8 | BoxEdgeChamfer · Isometric | 1.4 ms, 15 strokes, 739 rays |
| 🟢 | DR7 | No bodies | Drawing: no bodies to project |
| 🟢 | DR7 | Skewed frame | Drawing: the view frame is not a right-handed orthonormal frame |
| 🟢 | DR7 | Left-handed frame | Drawing: the view frame is not a right-handed orthonormal frame |
| 🟢 | DR7 | Plane beyond the body | Drawing: the section plane misses every body |
| 🟢 | DR7 | Plane in front of the body | Drawing: the section plane misses every body |
