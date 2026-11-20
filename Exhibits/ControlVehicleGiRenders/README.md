# ControlVehicle diffuse GI proof

## Implementation status

This is a new, standalone CPU diffuse-GI implementation, not restoration of every
module described in the earlier GI plan. Stokes contour GI, surfel cascades, GPU
wavefront tracing, editor controls, and Vulkan integration are **not implemented
by this change**. Do not treat the images as validation of those methods.

`Engine/GeometricRaster/GlobalIllumination/DeterministicDiffuseGi.h` integrates
incident radiance on a cosine-weighted hemisphere using deterministic binary
radical-inverse quadrature. The vehicle exhibit supplies exact BVH4 visibility,
interpolated surface normals, OpenPBR direct lighting on secondary surfaces,
emission, and environment escape radiance. Diffuse reflectance weights the
result. This transports light from the yellow body onto neighbouring surfaces.
There is no constant ambient fill or cavity-AO substitute in the new GI modes.
The secondary surface uses single-sample solar and softbox visibility; the
primary uses sixteen solar taps and four softbox taps.

## Images

All images are 1280 by 720. Front and rear each have four matched variants:

- `*_direct_only.png`: solar plus softbox illumination and visible emission.
  Ambient fill and approximate reflection rays are disabled.
- `*_diffuse_gi_32spp.png`: the identical direct image plus 32 cosine-weighted
  visibility rays per primary hit, collecting environment and one surface bounce.
- `*_indirect_only_32spp.png`: the added GI term alone, with identical exposure.
- `*_path_trace_64spp.png`: existing three-vertex diffuse/specular path tracer
  with 64 deterministic paths per pixel. It uses approximate lobe weighting,
  not a converged or unbiased reference. Its environment/specular paths differ
  from the one-bounce diffuse integrator; do not interpret it as numerical parity.

The camera, exposure, light sources, geometry and materials match within each
view. PNGs are ACES tone-mapped and sRGB encoded, so adding their pixel values
will not reproduce the combined image. No denoising is applied. Sampling noise
is intentionally retained. Studio diffuser strips are visible emission geometry;
they contribute through secondary hits rather than explicit area-light sampling.

## Reproduce

From the repository root, run:

```sh
bash Automation/RenderControlVehicleGi.sh
```

Requires C++20, pthread, and ImageMagick `convert`; no Vulkan SDK or downloaded
third-party libraries. The script builds into `/tmp`, renders actual `.face`
vehicle geometry and studio triangles, converts PPMs to PNGs and removes PPMs.

The original ten studio images remain unchanged in `ControlVehicleRenders`.

## Validation

```sh
g++ -std=c++20 -O2 -I. Exhibits/DeterministicDiffuseGiValidation.cpp -o /tmp/validate_diffuse_gi
/tmp/validate_diffuse_gi
```

Forty checks: hemisphere direction norms, deterministic repeatability, ray count,
zero samples, blocked hemisphere, coloured constant radiance, and analytic
cosine-radiance integral. These validate the quadrature, not GPU/editor parity.
