# 🌅 World Mirror — Sun & Sky 1:1 Native Parity Report

> **TL;DR**: The HTML `FrontierWorldMirror` no longer fakes the sun or the sky. Both are now direct ports of the two native authorities — `CelestialEphemerisSolver` (`Engine/Display/Celestial/…`, solar ephemeris + compass axes + Kelvin tint) and `AtmosphericSkySequence` (`Engine/GeometricRaster/Atmosphere/…`, transmittance / multi-scatter / sky-view LUTs, photosphere with Neckel & Labs limb darkening, K+F corona and the 30° glare veil). The same maths runs in the WebGL2 shader, the WebGPU WGSL shader and the zero-WebGL CPU kernel, and the Sun / Atmosphere inspector panels expose the native controls.

---

## 1. What was fake before (and is now gone)

| Area | Legacy HTML substitute | Native authority now used |
| :--- | :--- | :--- |
| Sky LUT | 96 × 48, `(0.5 − v) * 2` polar mapping, no limb geometry | 192 × 108, geometric-limb bank split (upper rows 0‑53 at 40 steps, lower rows 54‑107 at 28 steps, Schlick lower-limb Fresnel into row 0) |
| Transmittance | 128 × 32, 48-step march | 256 × 64 spherical-horizon LUT (`TransmittanceSteps = 64`) |
| Multi-scatter | absent | 32 × 32 infinite-order LUT (64 quadratic `Fraction²` steps, 32 Fibonacci directions × 24 steps) |
| Sun disc | `pow(cosSun, 160) * 0.25` aureole + `* 4.2` disc | `EvaluateSolarCoronaRadiance`, `DiscIntensity = SolarIntensity · SkyBrightness · 2.1`, `0.5 · (1 − erf((r − 0.997) / 0.012))` PSF, sub-pixel erfc mass gains 0.83276 / 0.77447 / 0.69061 |
| Limb darkening | none | Neckel & Labs 650 / 550 / 450 nm quintics |
| Corona | none | `0.042 / (1 + (r/1.18)²)^1.85 + 0.0085·S / (1 + (r/3.4)²)^1.38 + 0.0014·S / (1 + (r/11.5)²)^1.22`, tint 1.04 / 0.99 / 0.91 |
| Glare | none | 30° window, bloom weight 1.50, aureole 0.25 @ 6°, veil `0.045 · DiscIntensity` |
| Sun irradiance | hand-tuned 5.4 W/m² | `EffectiveSolarTint ⊙ SolarTransmittance · (Intensity · Direct · 0.044 · SunVis)` |
| Solar position | authored vectors only | `SolveSolarEphemeris` (Julian epoch, equation of time, hour angle, pole angle, GMST) + `ConvertAzimuthElevationToWorldAxis` |
| Kepler tint | ad-hoc warm ramp | `EvaluateKelvinToLinearRgb` (native blackbody fit) |

## 2. Files

| File | Role |
| :--- | :--- |
| `AtmosphereSky.js` | Full `AtmosphericSkySequence` port: `ResolveAtmosphericMedium`, `EnsureFoundationLuts` (transmittance + multi-scatter, cached on medium/ground-albedo hash), `BuildSkyViewLut`, `SampleSkyViewDirection`, `ComputeSolarTransmittance`, `EvaluateSolarCoronaRadiance`, `EvaluateNeckelLabsLimbDarkening`, `EvaluateSolarGlareSpread`, `SkyViewState` (shared `LimbAngle` + LUT bytes) |
| `Solar.mjs` | Full `CelestialEphemerisSolver` port: `SolveSolarEphemeris`, `ConvertAzimuthElevationToWorldAxis`, `ConvertWorldAxisToAzimuthElevation`, `EvaluateKelvinToLinearRgb`, `AdvanceDayCycleHours`, native default configuration, plus the legacy `Solar()` wrapper used by the light graph |
| `ShowcaseGalleryScene.js` | `SolarConfig`, `EffectiveSolarTint`, `SolarTransmittance`, `SolarSolved`, `Medium`, `IsOrbitalView` published on the compiled scene; moon axis through the shared compass conversion |
| `GtxWorldViewport.jsx` | GLSL port (uniforms `uSkyCfg`, `uMediumCfg0/1`, `uSolarCfgA/B/C`, `uSolarTint`, `uSolarTrans`) + JS uploads; an overhead LUT probe replaces the uploaded `uSkyRadiance` floor |
| `WebgpuGtxRenderer.js` | WGSL port; `SceneUniforms` grew eight `vec4f` fields (576 → 704 B) written from the same scene data |
| `CpuGtxFallbackRenderer.js` | Zero-WebGL kernel port: `SampleSkyViewInto` (bank split + smoothstep X + Catmull-Rom Y), `SolarTransmittanceDirection`, `SolarErf`, `NeckelLabsLimbDarkening`, `SolarGlareSpread`, `SolarCoronaRadiance`; `RenderBand` publishes `PixelAngularSizeRad` and `SkyLimbAngle` |
| `Inspectors.jsx` | Sun panel (Day cycle · Observer/date · Solved direction · Sun Disk · Sunlight gain · Lens Flare); the Animate loop and manual scrub write compass axes through `ConvertAzimuthElevationToWorldAxis`, readouts through `ConvertWorldAxisToAzimuthElevation` |

## 3. Clouds (`VolumetricCloudSequence` 1:1)

The volumetric deck is no longer a hand-rolled FBM. `CloudSequence.js` is a line-for-line port of
`Engine/GeometricRaster/Atmosphere/VolumetricCloudSequence.{h,cpp}` plus `Engine/Shaders/Atmosphere/CloudNoise.slang`,
`CloudDensity.slang`, `CloudTransport.slang` and `CloudAerialPerspective.slang`, and the same trace now runs in all three
renderers.

| Stage | Native function | Port |
| :--- | :--- | :--- |
| Wind advection | `CloudWindCoordinates` | `CloudAdvect` / `cloudAdvect` (tangent-plane rotation about the wind axis, shear + veer per km, followed radius) |
| Weather | `CloudWeatherAtPoint` / `CloudWeatherFromAdvectedNormal` | `CloudWeather`, moisture = `0.48 + 0.57·(broad − 0.5) + 0.43·(meso − 0.5)` |
| Cell envelope | `CloudCellEnvelope` + `CloudConvectiveEnvelope` | 3×3 randomised cell discs with buds, tilt and tower height, averaged over the three axis projections |
| Density | `SamplePlanetCloudDensityAdvected` | all 13 formations (stratus, stratocumulus, cumulus, cumulonimbus, altostratus, cirrus, lenticular, mammatus/KH, asperitas, nacreous, pyrocumulus, cyclone) incl. anvil discs, pouch lobes, wave curtains and lens masks |
| Shadow density | `SamplePlanetCloudShadowDensity` | `sampleCloudShadowDensity` — the cheap shape-only field the solar march taps |
| View march | `TracePlanetaryClouds` | two ordered spans, `Growth = clamp(log(1 + L/1900), 0.1, 4.8)` (flat above 19 km), `Fraction²`/exponential step spacing, jittered 0.40–0.60 sample, lit-sample caching at `clamp(footprint·0.75 + 120, 100, 280) m` |
| Light transport | `PlanetCloudSource` + `CloudScatteringOrderPhases` | three dual-lobe scattering orders (scales 1.0 / 0.49 / 0.19) through `exp(−τ·[1, 0.35, 0.12])`, `CloudAirColumn` (upward **and** downward) air transmittance, sky ambient + bounce, nacreous iridescence and pyrocumulus ember glow |
| Aerial perspective | `CloudAerialPerspective.slang` | optical-centroid two-sided 8-step Rayleigh/Mie/ozone in-scatter, `radiance = T_air·cloud + opacity·air` |

**One documented deviation** (unchanged from the previous revision of this report): the baked 128³ Perlin–Worley condensate
volume (`CloudNoiseVolume.h`, 8 MiB) is not shipped to the browser; the same lattice hash is sampled as a footprint-filtered
multi-octave field instead (`SampleCloudShapeField` / `cloudShapeField`). Because that field is Gaussian around 0.5 rather than
bimodal, two calibration constants sit on top of the native thresholds:

* `Expand(v) = saturate((v − 0.30) / 0.44)` on the macro / meso / billow channels, so formations break into cells instead of a veil.
* Stratiform threshold `mix(0.64, 0.25, Amount) → mix(0.77, 0.38, Amount)` (measured sky cover at Coverage 0.25 / 0.42 / 0.70 is
  29 / 38 / 62 %, versus 47 / 68 / 85 % before).

| Renderer | Where | Uniforms |
| :--- | :--- | :--- |
| CPU kernel | `CpuGtxFallbackRenderer.js` → `CreateGtxBandKernel` plus the stringified worker, fed `BuildCloudKernelPrelude()` (52 kB of JSON-embedded deck defaults) | `CloudLib` (the whole of `CloudSequence.js`) |
| WebGL2 | `GtxWorldViewport.jsx` GLSL (`cloudPermute` … `evalVolumetricClouds`, 26 functions) | `uCloudCfg0..6` |
| WebGPU | `WebgpuGtxRenderer.js` WGSL, same 26 functions translated from the GLSL block | `uCloudCfg0..6`; `SceneUniforms` grew four `vec4f` fields (704 → 768 B, `uCloudCfg3 @704 … uCloudCfg6 @752`) |

Uniform semantics (identical in both shaders): `0` = (enabled, coverage, extinction, feature scale), `1` = (floor, thickness,
anvil, anisotropy), `2` = (category, shadows, wind X, wind Y), `3` = (shear/km, veer rad/km, turbulence, simulation seconds),
`4` = (planet radius, seed, pixel angle, march steps), `5` = (overhead sky radiance, Rayleigh strength), `6` = (Rayleigh scale
height, Mie scale height, ozone strength, Mie strength) — all derived from `ActiveScene.CloudDeck` (`ResolveCloudDeck`) and
`ActiveScene.Medium`, i.e. the same `CloudDeckConfiguration` the inspector edits.

## 4. Ordering / scale decisions

* `SolarScale = SolarIntensityMultiplier · 0.24` inside the sky LUT (Rayleigh phase scaled by `0.0596·SolarScale`) — unchanged from native.
* `SunIrradiance = tint · transmittance · (Intensity · Direct · 0.044 · SunVis)`; with the native defaults (22 · 2.5) the noon irradiance is ≈ 2.2–2.4 W/m², i.e. the native scale, not the previous authored 5.4 W/m².
* `SkyRadiance` keeps the `SynchronizeWorldAtmosphere` night floors (`0.015, 0.020, 0.032`), then the shaders relight it from the LUT (an overhead probe clamps it to `0.001 / 0.002` in orbital views).
* Authored `Direction` remains an explicit override (`ManualDirectionOverride` in native terms) — the shipped levels keep their art-directed sun vectors, and the inspector shows the true solved bearing/altitude of whatever axis is in use.

## 5. Measured behaviour (Node, 2026‑11‑21)

| Quantity | Value |
| :--- | :--- |
| Sky-view bake | 462 ms cold / ~155 ms rebake (192 × 108, 40 / 28 steps) |
| Transmittance LUT | 829 k samples, 132 ms |
| Multi-scatter LUT | 829 k samples, 179 ms |
| Transmittance LUT bytes | 49 152 (`256 · 64 · 3`) |
| Multi-scatter LUT bytes | 3 072 (`32 · 32 · 3`) |
| Zenith sky radiance | `0.0218, 0.0491, 0.1297` |
| Horizon (sun-ward) | `0.4153, 0.5376, 0.6043` |
| Solar transmittance @ zenith | `0.9396, 0.8644, 0.7652` |
| Disc centre radiance | ≈ 70.8 (R), limb ≈ 15.2 — Neckel & Labs falloff |
| 5° off-axis (corona + veil) | `0.0734, 0.0556, 0.0346` |
| 30° off-axis | `0, 0, 0` (hard window) |
| Lobby frame (800 × 450, CPU kernel) | mean luma 136 vs 173 before the port — the native scale is ~2.9× dimmer than the old authored irradiance, and the glazed portal now resolves a real horizon + ground bank instead of a hazy dome |

## 6. Gates

* `node WorldMirrorValidation.mjs` → **ALL 22 SUITES PASSED** (suite 21 covers LUT dimensions, compass axes, Kelvin tint, limb darkening, corona falloff,
  transmittance, ephemeris noon/wrap, panel schema, the cloud trace in all three renderers and a sky-cover probe through `EvaluateVolumetricClouds`;
  suite 9 now also pins the Unreal camera modifiers — Shift 2.5x, Ctrl 0.25x, Ctrl+Shift 0.625x, wheel fly-speed scalar with its 0.15x/16x clamps).
  Because no WGSL compiler exists in the sandbox, the suite also resolves every WGSL call site against the 110 declared functions and
  every `U.*` read against `SceneUniforms` — that gate caught the moon/starlight block being clipped by the cloud splice.
* `node Build.mjs` → standalone `index.html` (5.41 MiB).
* `JSDOM_DIR=… node Smoke.mjs` → **ALL CHECKS PASSED** (14/14).
* Shader syntax checked out-of-band: GLSL through `@webgpu/glslang` (Vulkan 450 profile) — the full vertex + fragment source compiles to 102 197 B of SPIR-V after the cloud port; WGSL mirrored field-for-field from it (uniform block now 768 B, `uSkyCfg @576 … uCloudCfg6 @752`).

## 7. Known characteristics

* The day-cycle animation rebuilds the 192 × 108 LUT whenever the quantised sun direction or medium changes (native does the same every frame). In JS that is ~150 ms per bake, so a scrubbed day cycle is CPU-bound rather than frame-rate bound.
* The solar corona march (`ComputeSolarTransmittance`, 64 steps) is evaluated per sky pixel within 12.8 disc radii of the sun; the glare veil covers 30°. Costs scale with the sun's on-screen footprint.
* Glass transmission, mirror reflections and the sky fill all sample the baked SkyView LUT (`_ActiveSkyLut`), matching `EvaluateBackgroundSkyRadiance`; before that fix they fell back to the flat overhead constant and the lobby portal rendered near-black.
* Shipped level `Direction` vectors are art-directed overrides and do not necessarily agree with their `Local Hours`; the inspector readout follows the override so the panel stays self-consistent.
