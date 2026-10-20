# RollingBallBlend harness: B-rep specimens vs closed-form volumes

Output of `Engine/Modelling/SolidArc/Verification/RollingBallBlendVerification.cpp` (copy in `../Code/`). ΔV is measured by
quadrature on the result; "must refuse" rows pass only when the general route refuses.

| Specimen | Route | Faces | Stations | Max residual | Max fit deviation | ΔV measured | ΔV closed form | Legacy BlendSolver |
|---|---|---|---|---|---|---|---|---|
| box one edge fillet | general ✔️ | 7 | 55 | 1.6e-14 | 2.7e-15 | -0.077257 | -0.077257 (match, err 1.9e-08) | solid |
| box one edge chamfer | general ✔️ | 7 | 55 | 5.2e-13 | 1.3e-13 | -0.180000 | -0.180000 (match, err 2.1e-08) | solid |
| box top four edges fillet (mitres) | general ✔️ | 10 | 196 | 1.6e-14 | 3.1e-15 | -0.260056 | -0.260044 (match, err 1.1e-05) | refused: multi-edge fillet chains share a vertex (corner ... |
| box top four edges chamfer (mitres) | general ✔️ | 10 | 196 | 5.2e-13 | 1.3e-13 | -0.594000 | -0.594000 (match, err 1.3e-07) | solid |
| box three edges at a corner fillet (sphere corner) | general ✔️ | 10 | 128 | 1.2e-14 | 1.8e-15 | -0.169313 | -0.169308 (match, err 5.7e-06) | solid |
| box all twelve edges fillet | general ✔️ | 26 | 500 | 2.4e-14 | 6.7e-15 | -0.659174 | -0.659151 (match, err 2.3e-05) | solid |
| box all twelve edges chamfer (triple mitres) | general ✔️ | 18 | 508 | 5.2e-13 | 1.3e-13 | -1.458000 | -1.458000 (match, err 2.3e-07) | refused: non-planar edge selections must form one closed ... |
| cylinder top rim fillet | general ✔️ | 4 | 137 | 9.8e-13 | 1.2e-07 | -0.113223 | -0.113222 (match, err 7.0e-08) | solid |
| cylinder both rims chamfer | general ✔️ | 5 | 274 | 8.3e-16 | 8.6e-08 | -0.508938 | -0.508938 (match, err 7.4e-08) | refused: selected edge requires two adjacent planar faces |
| boss root fillet (concave rim) | general ✔️ | 9 | 137 | 5.6e-13 | 1.1e-07 | 0.034771 | 0.034771 (match, err 2.7e-08) | refused: edge is degenerate (zero length) |
| boss top rim fillet | general ✔️ | 9 | 137 | 8.8e-13 | 7.3e-08 | -0.029952 | -0.029952 (match, err 3.6e-08) | refused: edge is degenerate (zero length) |
| hole rim fillet (convex rim on a hole) | general ✔️ | 8 | 137 | 1.2e-14 | 1.7e-07 | -0.034771 | -0.034771 (match, err 5.6e-08) | refused: edge is degenerate (zero length) |
| cone top rim fillet | general ✔️ | 4 | 137 | 7.8e-13 | 7.2e-08 | -0.006789 | — | refused: edge is degenerate (zero length) |
| cylinder rim chamfer larger than the radius (must refuse) | ✔️ refused as required: size exceeds the curvature radius of face 1 along edge 2 (the blend would fold through itself) | — | — | — | — | — | — | refused: set-back reaches the cylinder axis |
| cylinder rim fillet larger than the radius (must refuse) | ✔️ refused as required: size exceeds the curvature radius of face 1 along edge 2 (the blend would fold through itself) | — | — | — | — | — | — | refused: fillet radius reaches the cylinder axis |

0 specimen(s) failed
