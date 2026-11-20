# Real-Triangle GI, Solar Shadows and Deterministic Path Tracing

## Decision and scope

| Subsystem | Selected methods | Distance volume needed? |
| --- | --- | --- |
| GTX-oriented GI | Stokes contour integration; two octahedral surfel irradiance cascades | No |
| Solar shadows | Exact BVH4 solar disc; triangle-derived surfel field with exact contact | No |
| Path tracing | One deterministic integrator, software triangle traversal on GTX, hardware queries on RTX | No |
| Deforming characters | Updated skinned triangles, acceleration refit/rebuild and refreshed surface samples | No capsules |

These are architectural selections, **not a new measured claim that they are the
fastest or highest-quality methods on GTX hardware**. Historical candidate RMSE
values are not transferable to the changed triangle visibility implementation.
No new SDF lighting system, analytic character solid, or capsule proxy is allowed.
The existing offline SDF baker is outside this change.

## Historical recovery

The current working branch was reset locally to `8fd0896`; its pushed history was
recovered from `origin/arena/035c342e-frontier` through `27e4b59` before editing.
The following historical source objects were fetched from SultanAladin/Frontier:

- `2d6f15ec29d13099c7e845d0ff03b3743d7d6dd9`:
  `Exhibits/CleanEmissiveGiCandidates.h` (2,472 lines).
- `536cfe11c2f73bf20fcbcf6fda6480053260f334`:
  `Exhibits/CleanShadowAndGiCandidates.h` and
  `Exhibits/CleanEmissiveShadowValidation.cpp`.

Their SDF bridge and capsule rig are **not restored into active code**. The
spherical contour integration and surfel approach inform separate C++ engine
units below. The contour horizon clip also fixes the historical entering-edge
error: retain the signed `DA - DB` denominator rather than clamping it positive.

## Implemented CPU proof

| Unit | Implemented mechanism | Important limit |
| --- | --- | --- |
| `StokesContourGiSequence` | Real emissive triangle extraction; horizon clipping; closed-form spherical edge contour; diffuse area samples from actual triangles, clustered by position, normal and material | Partial source visibility uses four deterministic triangle locations, not an exact occlusion integral |
| `DiffuseSurfacePatch` | Area-weighted triangle samples with coloured reflected emissive and solar radiance | Finite-disk secondary transport, one diffuse surface bounce; bounded sampling region |
| `SurfelCascadeGiSequence` | Triangle and diffuse-patch radiant disks; 252 inner and 140 outer probes; 8x8 octahedral irradiance; 16x16 four-ray depth moments | Coarse probe interpolation can leak or lose detail; no probe relocation yet |
| `SurfelOccluderField` | Surface samples from real triangles, normal-separated area clustering, projected disks and smooth solar cone visibility | Approximate distant blockers; disks dilated 20% to close sampling gaps; can over-occlude |
| Contact visibility | Exact 0.28 m BVH4 triangle ray | No deforming proxy |
| Existing deterministic path tracer | 64 paths, three vertices, deterministic quadrature, diffuse and approximate GGX lobe weights | CPU comparison only; not unbiased ground truth or a completed wavefront GPU port |

The contour and cascade GI algorithms are **not** the earlier ray-traced cosine
hemisphere integrator. Primary visibility and segment occlusion still use the
triangle BVH: using visibility rays does not turn the contour integral or cached
surfel radiosity into a path tracer.

### Current source layout

```text
Engine/GeometricRaster/
  DirectLighting/
    SolarShadowSequence.{h,cpp}       # Existing five shadow modes
    SurfelOccluderField.{h,cpp}       # New triangle sampled solar approximation
  GlobalIllumination/
    StokesContourGiSequence.{h,cpp}   # Emissive triangles and diffuse patches
    SurfelCascadeGiSequence.{h,cpp}   # Octahedral irradiance and depth moments
    DeterministicDiffuseGi.h          # Earlier hemisphere comparison, distinct
    TieredWorldRenderSequence.{h,cpp} # Existing world renderer, not GI-wired yet
Exhibits/
  ControlVehicleRenderExhibit.cpp
  WorldGiShadowPathTraceValidation.cpp
  ControlVehicleGiMethods/            # Direct / Stokes / surfel / PT, front + rear
  ControlVehicleShadowMethods/        # Two solar methods + scalar visibility
Automation/
  RenderControlVehicleGi.sh
  BuildVehicleGiComparison.py
```

The branch still uses `GlobalIllumination/`; moving all world-rendering sources
to `WorldRendering/` is a separate migration, not silently marked complete.

## Rendering acceptance

- Actual FCT1 body, four tyres, four rims and rear-wing triangles; no vehicle box proxy.
- Same camera, materials, sun, exposure and emitter geometry for each GI view.
- GI comparison: direct solar only, Stokes emissive + diffuse bounce, surfel
  cascades, and the existing path tracer. Environment radiance is disabled.
- The contour and cascade outputs include direct area emission as their first
  lighting term. The explicit softbox NEE is disabled in their direct shader to
  avoid counting the softbox twice.
- Shadow comparison holds surfel GI fixed while changing solar visibility;
  grayscale visibility is also saved without ACES tone mapping.
- No denoiser, temporal accumulation or pixel-skipping shortcut.
- Individual images are 1280x720. Labelled comparison sheets use resized copies.
- Numeric validation includes independent triangle area quadrature, horizon
  clipping, octahedral round trips, source extraction, probe lookup, triangle
  contact and open/blocked surfel visibility.
- Visual defects remain visible rather than being claimed away: source-visibility
  bands, patch approximation, coarse probe leakage, and path sampling noise.

## Remaining implementation stages (not completed by CPU images)

1. **World/editor integration:** GI method enum and selector; per-scene instances;
   upload geometry/material changes; invalidate sources, patches and probes on
   material edits or deformed triangle updates. Add the sixth shadow enum without
   breaking existing five modes. No scene-specific geometry in the engine units.
2. **GPU parity:** `.slang` contour integration, surfel radiosity/probe update,
   octahedral irradiance/depth storage and shader-to-C++ layout assertions; bind
   and dispatch in `VulkanWorldRenderSequence`. Compile with `slangc` and compare
   linear HDR output to CPU. No GPU execution is claimed in this sandbox.
3. **GTX/RTX path tracing:** extract the current exhibit integrator into an engine
   sequence; replace approximate lobe weights with evaluated mixture PDF and MIS;
   add persistent accumulation and deterministic dimension allocation. Then
   software BVH wavefront traversal and capability-gated hardware ray queries.
   SBVH/CWBVH8, six-stage GPU queues and lock-free scheduling remain unimplemented.
4. **Quality/performance gates:** real deforming character asset (none introduced
   as a stand-in), moving emissive geometry, thin walls, dark rooms, multi-source
   occlusion, probe relocation, cascade transitions and CPU/GPU parity. Measure
   GTX and RTX timing separately; do not repeat old historical metrics as new ones.
5. Commit and push each verified stage and its images on the current branch.

## Reproduce

```bash
bash Automation/RenderControlVehicleGi.sh --methods
bash Automation/RenderControlVehicleGi.sh --shadow-methods
```

The shell script builds into `/tmp` and converts raw PPMs with ImageMagick. The
CPU proof requires no Vulkan device or downloaded third-party SDK. Full editor,
Slang compilation and GTX/RTX testing require their actual toolchains/hardware.
