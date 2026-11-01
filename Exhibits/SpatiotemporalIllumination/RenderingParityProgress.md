# WebGPU Demo → C++/Vulkan Renderer Parity (1:1 Port)

Reference: `Experimental/SlateReference/A/Exhibits/VehicleProvingGrounds/Shader/*.wgsl`
Target: `Frontier/Engine/Shaders/SpatiotemporalIllumination.slang` + `Frontier/Engine/Rendering/SpatiotemporalIllumination.{h,cpp}`

## Three renderers

| Mode | Name in UI | Pipeline |
|---|---|---|
| 1 | **Preview** (fast Blinn, debug) | Tile-binned compute raster + simple gradient sky + fast shadows (OrthoClusterDepth / SDF cone). **Fixed 10 FPS bug**: previously every frame did GPU→CPU readback + CPU ACES tonemap + CPU→GPU re-upload (≈6 full pipeline drains/frame). Now all dispatches + GPU ACES present (`PresentPreviewViewMain`) are recorded into the swapchain stream, ring-buffered ×4, zero readback. Shadow projection rows are striped across 64 lanes so one huge triangle no longer serializes ~500k atomic probes on one thread. |
| 2 | **Hybrid** (GTX SDF renderer) | Hardware raster G-buffer → CSM + analytic direct → **ReSTIR GI** (half-res 1 cosine ray/px in hardware 3D SDF + surface cards, temporal+spatial reservoir reuse) → CWBVH VNDF reflections → SVGF → **display TAA + lens flare**. |
| 3 | **Full RT** (RTX raytraced) | Same ReSTIR GI, but candidates + visibility traced against the two-level 8-wide CWBVH. Now also **half-res, 1 ray/px** (demo parity). Previously full-res with up to 4 rays/px — that workload mismatch (16× more rays) is why RT dropped to ~60 FPS where the web demo held 90–110 FPS. |

Presentation: swapchain now prefers `VK_PRESENT_MODE_MAILBOX_KHR` → `FIFO_RELAXED` → `FIFO`, 3 images (was FIFO/2 — the source of the 120 Hz 40↔60 FPS judder beat).

## Ported in this change (algorithm-level 1:1 with the WGSL)

- **ReSTIR GI** (`ReservoirRecords/Temporal/Spatial.wgsl` → `TraceIndirectDiffuseMain` + new `ResampleGiSpatialMain`):
  32 B reservoirs (position, octahedral normal, half radiance + identity, W, M); initial weight `lum·cos/pdf`,
  `pdf = cos/π`; temporal merge with M clamped to the history limit and visibility re-traced on a rotating
  quarter of pixels; spatial disk proposals with normal/depth similarity, reconnection-shift Jacobian refused
  outside `[1/8, 8]` (NaN included), adopted-sample visibility test with own-reservoir fallback, 1/Z bias
  correction, resolve `E = L·cos·W/π` (the `/π` matches this renderer's existing GI demodulation convention).
  This replaces the previous raw 1-spp GI → SVGF feed, which was the main remaining noise source.
- **Physical sky** (`AtmosphereScattering.wgsl`/`CelestialSky.wgsl` → `EvaluatePhysicalSky`): Rayleigh + Mie +
  ozone with Kasten–Young air mass, multi-scatter boost, limb-darkened solar disc, aureole, twilight band.
  Replaces the flat zenith/horizon gradient in the main renderer. (The simple gradient sky remains **only** in
  Fast Preview, as intended.) Config zenith luminance remains as an overall gain so day/dusk presets still work.
- **Display TAA** (`DisplayAntialias.wgsl` → new `IntegrateDisplayAntialiasMain`): Karis range compression,
  3×3 YCoCg mean±1.25σ clamp, motion-reprojected bilinear history, 0.10 blend fraction.
- **Lens flare** (`LensFlare.wgsl`/`OpticalLensFlare.slang` → `EvaluateOpticalLensFlare`): depth-occluded
  (5-tap G-buffer sun test) cool anamorphic streak + warm counter-streak, 5 chromatic ghosts on the
  sun↔antipode axis, antipode halo. Final-lit inspection mode only.

## Verified

- All 30 environment SPIR-V kernels compile (slangc, spirv_1_3). Reservoir half packing uses pure integer
  IEEE-754 bit ops (no Float16 SPIR-V capability) so kernels run on the SwiftShader CI device too.
- `SpatiotemporalIlluminationValidation` (Hybrid + Full RT end-to-end), `EverestInterfaceValidation`,
  `EverestShippingValidation`, `ProjectContentStagingValidation` pass on Linux + SwiftShader.
- Windows x64 cross-build (zig 0.13.0): `Everest.exe` / `Frontier.exe` PE32+ 0x8664, packaged in
  `Binaries/Everest-GTX-ReSTIR-Windows-x64.zip`. Native Windows GPU execution not yet run from this sandbox.

## Known remaining deviations from the WGSL demo (next round)

1. **Screen traversal** (`ScreenTraversal.wgsl` + nearest-depth pyramid) not yet ported — candidates go
   straight to SDF (GTX) / CWBVH (RTX). The demo treats this as a toggleable acceleration, not a look change.
2. **Radiance refresh / occupant carry** for temporal samples: radiance is stored, not re-resolved, and
   samples on moving occupants are not carried by their motion matrices (no occupant matrix buffer in the
   C++ path yet). The rotating-quarter visibility re-trace bounds the error.
3. **Clouds**: still the procedural slab inside the sky function (coverage/density/bottom wired);
   `VolumetricClouds.slang`/`CloudSynthesis.slang` exist but are not yet the Everest viewport cloud path.
   No wind drift (no time channel in `FrameHeader` yet).
4. **TAA sub-pixel jitter**: history reprojection is in, but Halton jitter of the raster primary is not yet
   applied, so TAA currently smooths temporal noise rather than supersampling static edges.
5. **Auto-exposure** (`DisplayExposure.wgsl`): manual `ExposureScale` only.
6. No UI changes were made (per request).
