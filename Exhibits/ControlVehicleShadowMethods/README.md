# Triangle solar shadow comparison

`00_comparison.png` holds cached surfel GI fixed, then compares:

1. Sixteen-tap BVH4 solar-disc triangle visibility.
2. Triangle-sampled surfel cone visibility plus an exact 0.28 m triangle contact ray.
3. BVH4 scalar visibility (not tone-mapped).
4. Surfel scalar visibility (not tone-mapped).

Front and rear views are 1280x720 each. The comparison sheet uses labelled
640x360 copies. No character proxies, capsules or voxel SDF are used.

`SurfelOccluderField` samples supplied triangles into 0.35 m cells separated by
normal direction. Each disk's radius comes from accumulated triangle area,
dilated by 20% to reduce gaps. Projected disks smooth against the solar cone.
This is an approximate field and visibly over-occludes or loses detail; **the
images do not establish that it is superior to exact BVH4 shadows**. It also
bounds its sampled region around the vehicle. A dynamic triangle update must
rebuild the field; there is no automatic editor/skinning invalidation yet.

The contact test remains an actual triangle query, not an analytic vehicle
or skeleton solid. The five existing engine shadow modes are untouched; this
new CPU algorithm is not yet a sixth option in the world editor selector.

```bash
bash Automation/RenderControlVehicleGi.sh --shadow-methods
```

See the GI method README and `Documentation/Plans/WorldGiShadowPathTracerPlan.md`
for implementation status. All output is from the standalone C++ renderer.
