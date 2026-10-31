# Project Everest — renderer port plan (GTX and RTX)

## TL;DR

The demo's renderer is 80 WGSL files (31 record-only headers, 49 files carrying 81 entrypoints) across twelve
families. The donor branch already carries a working dual-tier native renderer: RTX through hardware ray query and
GTX through signed distance fields plus oriented surface cards for ReSTIR GI, with a software binned-median BVH
handling reflections, mirrors, portals and glass on both tiers. The port is therefore not a green-field rewrite: it
is a completion, a structural split into two substrates behind one interface, and the per-element work in §7.

## 1. Demo inventory and native coverage

| Demo family | Files | Native counterpart today | Coverage |
|-------------|-------|--------------------------|----------|
| Illumination (ReSTIR, direct radiance, clustered lights, discrepancy) | 9 | `PathReplayReservoir.slang` (1,537), `LightInfluence.slang` (1,887), `ReservoirSampling.cpp`, `DiscrepancySequence.cpp`, `StaticDiffuseReservoir.slang` | High — chain exists, needs GPU validation per tier |
| Geometry (visibility, clusters, HiZ, material resolve, forward/surface/shadow raster) | 9 | `ClusterNesting.cpp`, `VisibilityCull.cpp`, `SurfaceRaster.slang`, `PathReplayRayQuery.slang`, `TriangleVisibilitySpace.h` | Partial — cull and visibility exist; HiZ and material resolve are new units |
| Effects (plume advect/march/pressure/vorticity, sparks, fire, lightning) | 9 | `PlumeFluid` + `PlumeFluidMarch.slang`, `PlumeFluidTransport.slang`, `GasPressureProjection.slang`, `SparkParticles`, `LightningStorm` | High |
| Display (AA, exposure, present, recombination, flare image/visibility) | 8 | `DisplayPresent.slang`, `ExposureAdaptation.cpp`, `PathReplayReconstruction.slang`, `SolarOptics.slang`, `SolarDiscFilter.h`, `LensFlareModel.h` | Partial — AA and full bloom are new |
| Atmosphere (medium, multiple, scattering, transmittance, celestial, twilight, stars, sky projection) | 8 | `SkyRadiance.slang` (349), `AtmosphereModel.cpp`, `CelestialModel.cpp` | Partial — LUT bakes and night stars are new GPU units |
| Weather (fog medium, precipitation compute/raster/surface, snowfall, deposit) | 7 | `FogMedium.cpp`, `PrecipitationOptics.slang`, `SkyPrecipitation`, `SnowSurface`, `ShallowWater.slang` | High for motion, partial for deposit |
| Clouds (field, present, records, shadow project/sample, trace) | 7 | `CloudField.cpp/.h`, GPU cloud cube and camera march (evidence in `Exhibits/LightCompute`) | Partial — present and shadow-projection units are new |
| Cards (capture, direct, final, irradiance, projection, radiosity, sampling) | 7 | `SurfaceCards.cpp` (CPU core), donor card bake in `SceneRenderer` | Partial — GPU units are new |
| Distance field (records, transfer, traversal, union, fragment distance) | 5 | `DistanceField.cpp`, `TriangleDistanceVolume.h`, `PathReplayDomain` | Partial — 3D transfer/union are new |
| Shared records (frame descriptor, packing, scene records, reflectance, material resolve) | 5 | `PathReplayFootprint.h`, `LightInfluenceSpace.h`, push constants | Partial — descriptor needs a deliberate, documented layout |
| Denoise (temporal, wavelet, upsample) | 3 | `VarianceFiltering.cpp`, `PathReplayVarianceFilter.slang` (450) | High |
| Cabin HMI (overlay, volume, vehicle visual) | 3 | `VehicleCabinHmi.cpp`, `CabinIconDepot` | Partial — overlay/volume GPU units are new |

📝 The donor's 21 Slang units are not one-to-one with the demo's 80 files; they are the units that were needed to
make the showcase run. The gap list above is the honest remainder, and §5 names the new files.

## 2. One interface, two substrates

```text
                        SceneRenderer (orchestration, unchanged role)
                                       │
                          TraceSubstrate  ⟨interface⟩
              ┌────────────────────────┴────────────────────────┐
     GtxDistanceSubstrate                              RtxQuerySubstrate
  SDF volume + surface cards + software BVH      BLAS/TLAS + VK_KHR_ray_query
              └────────────────────────┬────────────────────────┘
                                       │
        ReSTIR reservoirs · SVGF · materials · atmosphere · clouds · fog ·
        precipitation · plume · glass · exposure · display    (shared, tier-agnostic)
```

💡 The tier changes exactly one thing: how a ray asks "what did I hit". Everything downstream — the estimator, the
denoiser, the shading, the display — is shared. That is what makes A/B comparison meaningful and what keeps the two
tiers from drifting.

| Rule | Reason |
|------|--------|
| A tier is a substrate, not a fork | One estimator; two visibility providers |
| The interface returns hit position, normal, material index, triangle index, distance | Enough for every consumer; no tier-specific struct escapes |
| Reservoirs stay imported, never atlas-aliased | Temporal reuse reads last frame's reservoir; aliasing corrupts it |
| Both tiers produce the same G-buffer layout | A/B without recompiling consumers |

## 3. Capability probe and tier selection

```text
probe ──► accelerationStructure + rayQuery present?
          ├── yes ──► RTX  (hardware ray query for GI, reflections, shadows, glass)
          └── no  ──► GTX  (SDF + cards for GI, software BVH for reflections/mirrors/portals/glass)
                       └── still no triangle path (very low end) ──► Blinn low-res reference tier
```

| Probe | Used for | Missing consequence |
|-------|----------|---------------------|
| `VK_KHR_acceleration_structure`, `VK_KHR_ray_query`, `VK_KHR_deferred_host_operations` | RTX tier | Falls to GTX, stated in the viewport bar |
| `VK_KHR_synchronization2` | Fine-grained barriers | Required by `BarrierDerivation`; absent is a hard stop |
| Descriptor indexing | Material resolve | Falls back to a bound table |
| Subgroup operations | Reservoir compaction | Scalar fallback, slower, same result |
| Timeline semaphores | Compute/graphics overlap | Serial submission |
| 16-bit storage | Optional bandwidth | FP32 path stays; never silently quantised |

🔴 The active tier is visible in the viewport bar and in the Log Stream at bring-up. A silent fallback is a defect.
The demo's own GTX/RTX/Blinn selector is a 1:1 UI control, so this is also a UI requirement, not only an engine one.

## 4. GTX substrate — distance fields and software BVH

| Piece | Mechanism | Source / precedent |
|-------|-----------|--------------------|
| Proxy field | Every record registers a proxy (box, sphere, capsule) into a distance scene; transmissive glass is exempt from SDF occlusion | `DistanceField.cpp`, donor `RegisterSolidBox` |
| Distance transfer | 3D volume of distances with a band limit; per-frame update of moving occupants only | `TriangleDistanceVolume.h`, `DistanceField` core |
| Surface cards | Six oriented faces per proxy, albedo plus cached exitant radiance, 3×3 face quadrature bake | `SurfaceCards.cpp`, donor `OrientedSurfaceCard` |
| ReSTIR GI | Reservoir candidates sphere-traced through the far field, cards sampled as emitters | `ReservoirSampling.cpp` + `PathReplayReservoir.slang` |
| Reflections, mirrors, portals, glass | Software binned-median triangle BVH, Möller–Trumbore, near-first ordering | Donor BVH; mirrors and rectangular/circular portals already implemented |
| Cost model | Field trace is memory-bandwidth bound; card bake is a per-change amortised cost | Measure, do not assume |

⚠️ Distance-field marching is still ray marching. It must never be described as "non-ray-traced", and it is not
hardware ray tracing. The UI label says distance field, not RTX.

## 5. RTX substrate — acceleration structures and ray query

| Piece | Mechanism | Notes |
|-------|-----------|-------|
| BLAS | One per distinct record geometry, `PREFER_FAST_TRACE`, compacted after build | Cached by geometry hash, not by record |
| TLAS | Instance list rebuilt when transforms or membership change; `PREFER_FAST_BUILD` while driving | Double-buffered, referenced by the frame's descriptor |
| GI rays | `RayQuery` in compute, one per candidate | Same reservoir format as GTX |
| Reflection, shadow, glass | `RayQuery` with cull mask per family | Glass uses the closed-dielectric transport unit the donor already carries |
| Compaction | Subgroup ballot to compact empty candidates | Falls back to serial compaction |
| Cost model | BVH traversal quality dominates; instance churn during driving is the risk | Measure per scene |

⚠️ Acceleration-structure builds must be synchronised with the compute reads; the frame's barrier set is derived, not
hand-written. A missing build barrier is the most likely source of a wrong-image bug on this tier.

## 6. Shared chain and frame shape

```text
Tier probe ─► visibility (raster or query) ─► material resolve ─► candidate generation
      ─► ReservoirTemporal ─► ReservoirSpatial ─► shade winner
      ─► VarianceTemporal ─► VarianceWavelet ─► VarianceUpsample
      ─► atmosphere · clouds · fog · precipitation · plume · glass · flare
      ─► exposure ─► bloom ─► antialias ─► present
```

Declared through `SubmissionOrder`, allocated through `TransientTargetAtlas`, barriered by `BarrierDerivation`, with
reservoirs and history as imported ping-pong pairs. The donor already resolves a complete chain; the port keeps the
existing declaration order and adds the new units into it instead of inventing a second order.

## 7. Rendering elements — what is ported, in what order

| # | Element | Demo sources | Donor state | Work | Evidence gate |
|---|---------|--------------|-------------|------|---------------|
| 1 | **Sun disc, sky and atmosphere** | `AtmosphereMedium/Multiple/Scattering/Transmittance`, `CelestialSky`, `NativeTwilight`, `SkyProjection` | `SkyRadiance.slang`, `SolarOptics.slang`, analytic models, measured solar position | Finish transmittance and multiple-scattering LUT bakes on GPU; keep the analytic CPU oracle as the test | Sky-ladder against the analytic optical depth; noon/sunset/night captures |
| 2 | **Night sky and stars** | `NightStars`, `CelestialSky`, `StarCatalogue` | `CelestialModel` deterministic stars, `BrightStars` catalogue asset | GPU star raster plus catalogue binding | Star-count and position assertions; night capture |
| 3 | **Moon** | `MoonCards`, `NativeMoonPreview`, `CelestialAtlas` | Lunar phase and bounce reservoir in donor, Moon presentation open | Disc projection, phase, earthshine | Phase-angle assertions plus capture |
| 4 | **Clouds** | `CloudField`, `CloudTrace`, `CloudPresent`, `CloudShadowProject`, `CloudShadowSample`, `CloudShadowRecords` | Density field, 64-step camera march, six-step self-shadow, radiance cube, shadowed fog (branches 1 and 2), working budget preset | Present and shadow-projection units; keep the low budget as the iteration preset and the high budget as a reference tool | Cube and camera probes, disable/re-enable cold equality, budget RMSE |
| 5 | **Fog and light shafts** | `FogMedium` | Height-fog single scattering, cloud-shadowed arrivals, emissive-volume scattering (branch 1) | Bring in branch 1's facet-area emissive-volume work; keep the 64-step preset | Native probes plus ASan/UBSan, quadrature RMSE |
| 6 | **Lens flare and solar optics** | `LensFlare`, `FlareImage`, `FlareVisibility`, `LensOptics.css` | `LensFlareModel.h`, `SolarOptics.slang`, `SolarDiscFilter.h`, `FlareVisibility` compute precedent | Image-space flare kernel, occlusion-driven visibility, ghost chain | Kernel energy check plus capture against the flare reference |
| 7 | **Exposure, bloom, antialias** | `DisplayExposure`, `DisplayRecombination`, `DisplayAntialias`, `DisplayPresent` | `ExposureAdaptation.cpp` with dark-cabin guard, `DisplayPresent.slang`, `PathReplayReconstruction.slang` | Bloom, temporal AA, final recombination | Stable exposure under cabin/ground contrast; AA edge measure |
| 8 | **ReSTIR GI and reflections** | `DirectRadiance`, `ReservoirTemporal`, `ReservoirSpatial`, `RayTraversal`, `ReflectionTraversal`, `DiscrepancySequence` | Both tiers, spatial reuse, edge-stopping filter | Per-tier unbiasedness checks; Jacobian parity at solid angle | Tier A/B plus analytic oracles |
| 9 | **Shadows** | `ShadowRaster`, cloud shadow sampling | Native sun shadow path, cloud extinction per receiver | Tier-aware shadow queries; keep native analytic lighting untouched | Bitwise equality when clouds are disabled |
| 10 | **Glass and closed dielectrics** | `MaterialResolve` refraction gather, `CabinVolume` | Closed-dielectric transport unit, thin pane then closed glass, multi-stratum mixtures | Keep, retest per tier | Transmittance and energy checks |
| 11 | **Mirrors and portals** | Not in the demo shell; donor extension | Planar mirror, circular and rectangular portals, SDF and RTX portal extension | Keep as a named extension, documented as beyond-demo | Transit geometry assertions |
| 12 | **Precipitation, snow, water** | `PrecipitationCompute/Raster/Surface`, `SnowfallRaster`, `SurfaceDepositRaster` | `SkyPrecipitation`, `SnowSurface` single mesh, `ShallowWater` 2.5D | Deposit and accumulation units | Component-count and depth invariants |
| 13 | **Plume, fire, sparks, lightning** | `PlumeAdvection/March/Pressure/Vorticity/Records`, `FireIllumination`, `SparkRaster`, `LightningRaster` | `PlumeFluid` 5.13× pyro, `SparkParticles`, `LightningStorm` | Wire fire illumination into the GI candidate set | Plume step time, fire contribution probe |
| 14 | **Vehicle paint, glints, flakes** | Material modules, `AutomotiveFlakePaint` lineage | OpenPBR lobes, flake paint, glint research | Replace cell-search flakes with the approved constant-time NDF; new formulation needs its own energy and sampling checks | Sampling/PDF consistency, temporal stability, capture |
| 15 | **Materials and layered PBR** | `SurfaceReflectance`, `MaterialResolve`, archives | Layered material archives, one-stratum GGX/Smith/Schlick in the RIS target | Full layered evaluation inside the GI path | Reciprocity and energy audit |
| 16 | **Cabin HMI overlay and volume** | `CabinOverlay`, `CabinVolume`, `VehicleVisual` | `VehicleCabinHmi` in 3D, `CabinIconDepot`, dark-cabin exposure guard | GPU overlay and volumetric cabin units | Cabin contrast guard assertions |

## 8. Verification strategy

| Level | Method | Fails when |
|-------|--------|------------|
| Analytic | Closed-form oracles for optical depth, Beer–Lambert, RIS variance, BRDF reciprocity | Any drift beyond the stated epsilon |
| Tier A/B | One scene, one camera, one seed; capture both tiers; diff | Difference beyond the tier tolerance stated per element |
| Regression | Existing donor exhibits keep their pass counts after the split | A count drops |
| Barrier | Retained barrier dumps per declared frame | A transition disappears or a usage flips silently |
| Cold/warm | Disable/re-enable an element; cold reconstruction must match the warm path bitwise or within epsilon | Stale history enters the image |
| Sanitizer | ASan/UBSan on every native validation | Any report |

🔴 Tolerances are stated per element before the run, from the tier's own variance data, never chosen after seeing the
result.

## 9. Performance claims

No performance number is delivered without a reproducible measurement: scene, camera, resolution, tier, driver,
sample counts, and the exact command. The demo's own budget evidence (branch 1 `Results/CloudBudget.json`) is the
precedent for how these are recorded. Nothing in this plan asserts a frame rate.

## 10. Files

| Path | Action | Purpose |
|------|--------|---------|
| `Frontier/Engine/Rendering/SceneRenderer.{h,cpp}` | keep, split | Orchestration; delegates visibility to a substrate |
| `Frontier/Engine/Rendering/TraceSubstrate.h` | create | The one interface both tiers implement |
| `Frontier/Engine/Rendering/GtxDistanceSubstrate.{h,cpp}` | create | SDF, cards, software BVH |
| `Frontier/Engine/Rendering/RtxQuerySubstrate.{h,cpp}` | create | BLAS, TLAS, ray query |
| `Frontier/Engine/Rendering/GpuTraceCapability.{h,cpp}` | create | Probe and tier decision |
| `Frontier/Engine/Rendering/Shaders/{AtmosphereTransmittance,AtmosphereMultiple,CloudPresent,CloudShadowProject,CardDirect,CardRadiosity,DistanceTransfer,HiZPyramid,MaterialResolve,DisplayAntialias,FlareImage}.slang` | create | The gap units of §1 |
| `Frontier/Engine/Rendering/Shaders/*` | keep | 21 donor units |
| `Exhibits/EverestRender/` | create | Tier A/B, element captures, results |

## 11. Risks

| Risk | Impact | Mitigation |
|------|--------|------------|
| No GPU in the agent sandbox | Nothing about RTX performance or hardware correctness can be claimed here | Compile-prove, run CPU-reference oracles, and mark hardware verification blocked on the owner's machine |
| Acceleration-structure synchronisation | Wrong images that look plausible | Derived barriers plus retained dumps, and a deliberately broken build in validation to prove the check |
| Tier drift over time | Two estimators quietly diverging | Shared chain, A/B in CI, one reservoir format |
| Cloud and fog budget creep | Iteration slows to unusable | Keep the working low budget as default; high budgets stay explicit reference tools |
| Distance-field banding at grazing angles | Visible artefacts in reflections | Card bake density plus the documented band limit; measure, do not hide |
| WebGPU to Vulkan float differences | Golden-image mismatch | FP32 path kept; tolerance per element from measured variance |
