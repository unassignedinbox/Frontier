# Planet Forge — decisions, and what comes next

This file records *why* the base is shaped the way it is, so the erosion studio can
build on it without re-litigating the foundations.

## Non-negotiables taken from the brief

| Requirement | Consequence in the architecture |
| --- | --- |
| Planet scale, not a 100 km plate | cube-sphere quadtree, no local tangent world |
| Not a heightmap | parametric SDF document is the only source of truth |
| Terrain is SDF so particles can erode it | field is analytic and resolution independent |
| Must convert to mesh | chunk extraction is volumetric (dual contouring) |
| Cracks must be possible | fracture operators subtract real volume |
| HDD, not SSD | batched, spatially keyed, single-blob mesh cache |
| Never kill RAM/VRAM | hard chunk and byte budgets with importance-weighted LRU |
| Realistic ocean, especially the shore | finite-volume SWE, wet/dry, nested rings, no normal map |
| Frontier editor look | dark desk, one blue accent, hairline panels, tool rail |

## Decisions that were made deliberately, with the trade-off

1. **Volume meshing everywhere, not heightfield meshing near the camera.**
   Cost: ~2× the per-chunk time of a pure heightfield mesher. Benefit: cracks,
   overhangs and caves are legal from day one, and mixed-LOD chunks close by
   construction because they share one lattice. Reversing this later would mean
   throwing away every cached mesh.

2. **Altitude packing of the lattice.**
   Cost: a non-uniform vertical metric inside the chunk. Benefit: a 900 m × 300 m
   chunk no longer needs 30× the cells to keep the dual mesh conditioned.

3. **Frontier LOD refinement instead of build-then-truncate.**
   Truncation is faster to write and leaves holes in the planet. The frontier is
   always a valid cut, so a tight budget degrades detail, never coverage.

4. **One-way nesting of ocean rings (coarse drives fine).**
   Cost: no feedback from the shore back into the far field. Benefit: bounded cost,
   no global solve, and it matches what regional ocean models actually do.

5. **Simulation stops at ~5 km; beyond that, dispersive swell.**
   Shallow-water theory does not apply at 24 km cells. The hand-off is at the point
   where the physics changes, not where it became expensive.

6. **Documents, not baked data, in the field.**
   An edit invalidates only the chunks whose bounds overlap the changed operator.

## Known sharp edges

- Coarse-chunk chord edges under-place by half²/2R against the true arc; sub-pixel
  at the distances where that LOD is chosen, but not zero.
- `worldNeedsVolume` is document-wide, so adding one fracture switches the whole
  planet to the 2× path. A per-chunk complexity test exists in spirit but was
  removed in favour of the stable rule; revisit when cracks ship.
- No Coriolis term. Large-scale gyres will not form, which matters once currents
  and erosion talk to each other.
- Meshing is CPU-only.

## Order of work for the erosion studio

Each step is chosen to make the next cheaper:

1. Sculpting polish: operator transform gizmos, painted masks, reusable region
   layers.
2. Hierarchical fracture: octave-mixed Worley, ridge-stress direction bias,
   hardness-driven crack width. All inside `fractureAmount`.
3. Strata as material: depositional dip, thickness variation, hardness. Bands
   already exist as datums.
4. Particle erosion on the SDF: thermal first (a local slope rule), then freeze/thaw,
   then chemical dissolution gated by hardness from step 3.
5. Region mesh baking: ship baked meshes, stream those instead of re-evaluating
   the field.
