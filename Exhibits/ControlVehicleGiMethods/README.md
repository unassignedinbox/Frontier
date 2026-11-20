# Real-triangle vehicle GI comparison

`00_comparison.png` shows matched front/rear views with four distinct columns:

| Column | Executed C++ algorithm |
| --- | --- |
| Direct solar only | OpenPBR direct sun with exact BVH4 solar-disc visibility; visible emission; no ambient fill or softbox NEE |
| Stokes contour GI | Clipped spherical-triangle edge contour integration of real emissive triangles; four-point visibility; triangle-derived diffuse patch secondary transport |
| Surfel cascade GI | Finite-disk radiosity from real emissive triangles and diffuse patches, baked into two octahedral irradiance cascades; visibility-aware interpolation at each hit |
| Path tracing | Existing deterministic 64-path, three-vertex CPU integrator, approximate diffuse/specular continuation |

All individual PNGs are 1280x720. The sheet uses 640x360 labelled copies. Vehicle
geometry is the FCT1 body, four tyres, four rims, and procedural rear-wing
triangles. The studio also consists of triangles. There are no implicit-solid
occluders, character capsules, voxel distance fields, denoisers, or skipped pixels.
No character asset has been introduced, so these pictures do not validate skinning.

The new implementations live in `Engine/GeometricRaster/GlobalIllumination/`
and are registered in CMake; they are invoked by the standalone C++ exhibit.
This is **not yet a world-editor/Vulkan integration** or a GTX/RTX timing test.
Visibility rays are used by both GI methods; neither uses the earlier stochastic
cosine-hemisphere GI integrator as its lighting solution.

## Known limits visible in the pictures

- Stokes computes the unoccluded contour integral in closed form; partial
  occlusion is only four triangle visibility locations. Bands are visible on
  the floor and should not be described as an exact shadow integral.
- Diffuse patches are bounded to the vehicle region and use finite-disk
  approximations. They can lose geometric detail and do not converge as a PT.
- Surfel probe spacing is coarse; depth moments reduce but do not eliminate
  leaking or interpolation artifacts. Probe relocation is not implemented.
- The path tracer is an approximate comparison, not unbiased ground truth.
  It has noisy 64-path specular continuation and different visibility sampling.
- The path tracer's explicit softbox radiance uses the original exhibit values;
  the new methods take emission from the actual triangle material. Therefore
  this sheet compares implemented appearance, not numerical parity to a reference.
- All methods disable the sky environment for this comparison. No additional
  ambient fill is added. Tone mapping and camera exposure are shared per view.
- Surface emission is treated as single-sided by Stokes and surfel transport.

## Run

```bash
bash Automation/RenderControlVehicleGi.sh --methods
```

This builds into `/tmp`, renders the actual C++ images, converts to PNG and
constructs the labelled sheet. Raw PPMs are ignored and removed after conversion.
See `Documentation/Plans/WorldGiShadowPathTracerPlan.md` for the full source
layout, historical recovery, remaining GPU/editor work and quality gates.
