# World GI, 6-Mode Real-Mesh Shadows & Unified Deterministic Path Tracer Plan

**Status:** Active Implementation Plan (Real Triangle Geometry Only · Zero Voxel SDF · Zero Denoiser)  
**Target Modules:** `Engine/GeometricRaster/DirectLighting/`, `Engine/GeometricRaster/GlobalIllumination/`, `Engine/SpatialAcceleration/`, `Engine/Shaders/DirectLighting/`, `Engine/Shaders/GlobalIllumination/`, `Engine/Shaders/GeometricRaster/`, `Engine/Components/Panes/`, `Engine/Lifecycle/`, `Exhibits/`  
**Upstream Reference Baselines:**
- `SultanAladin/Frontier@arena/8391e994-frontier` (`daed305` — unified `MatCap` / `OpenPbrDirect`, Fluid/Particle retirement, PlanetaryClouds atmosphere chain)
- `SultanAladin/Frontier-1@arena/39c6910b-frontier-1` (`d5bbb81` — `DeterministicIntegrators.h`, `DeterministicPathTracerVulkanOptimizationPlan.md`, Shadow Lab algorithms)
- Empirical clean-sheet benchmarks (`2d6f15e`, `536cfe1`, `4edab07`)

---

## 1. Architectural Verdict: Should We Still Use SDF?

**Answer: NO — do NOT use 3D Voxel SDF (`DistanceFieldSpace` / `VoxelSdfClipmap`) for World GI or Direct Shadows.**  
*(Only `Engine/GeometricRaster/Baking/GpuSdfBaker.{h,cpp}` remains, strictly isolated to offline `BakingWorkspace` `.fvol` exports.)*

### Empirical Evidence (560×315 `ControlVehicle` + Studio Scene, Zero Denoiser)

| Subsystem | Candidate / Architecture | Metric vs 128-SPP BVH+NEE Reference | Why No SDF Is Needed |
| :--- | :--- | :--- | :--- |
| **Solar Shadows** | **Surfel Triangle Field (`SurfelOccluderField`) + 0.28 m Contact BVH4** | **0.2973** shadow RMSE | Beats SDF candidates (`0.2983`, `0.3068`) and legacy `VoxelSdfClipmap` (`0.4170`); zero voxel grid memory, zero AABB boundary seams, zero ~600 ms/frame rebake when meshes deform. |
| **GTX GI #1** | **Stokes Contour GI (`StokesContourGiSequence`)** | **0.2238** RMSE | Closed-form spherical-polygon contour integration on real emissive triangles + 2nd-bounce area-sampled `DiffuseSurfacePatch` cloud; beats `VoxelSdfClipmap` (`0.2402`) and `L1` voxel field (`0.2259`). |
| **GTX GI #2** | **Surfel Cascade GI (`SurfelCascadeGiSequence`)** | **0.2239** (normal) / **0.2269** (cascade) | Dual-cascade octahedral irradiance (8×8 irradiance + 16×16 Chebyshev depth moments) populated from real triangle surfels; O(1) runtime lookup without 3D SDF clipmaps or light leaking. |
| **Legacy SDF** | `VoxelSdfClipmap` / Legacy `DDGI` | **0.2402** (clipmap) / **0.5361** (DDGI) | Slower dynamic updates (~600 ms rebake for deforming characters/meshes), AABB clipmap discontinuities, and higher error than both selected real-mesh candidates. |

### Hard Invariant: 100% Real Triangle Geometry (Zero Analytical Proxies)

Characters, props, vehicles, and studio environments are represented **exclusively as real triangle meshes** (`PolygonNetwork`, `RigidClusterInstance`, `RayTrianglePrimitive` inside `WideBoundingTree` BVH4 / Vulkan BLAS+TLAS). There are **no** analytical bone capsules, sphere proxies, or box substitutes in lighting, shadowing, or GI.

---

## 2. Target Architecture: 2 GTX GI Renderers + 6 Real-Mesh Shadow Algorithms + 1 Deterministic Path Tracer (GTX & RTX)

### 2.1 Render Engine Matrix (`WorldRenderEngine` in `TieredWorldRenderSequence.h`)

| Enum Value | Engine | Target Hardware | Core Mechanism (100% Real Triangles, Zero Denoiser) |
| :--- | :--- | :--- | :--- |
| `0` (`MatCap`) | Studio MatCap | GTX / RTX / CPU | View-space normal/roughness MatCap evaluation + direct ray contact occlusion. |
| `1` (`OpenPbrDirect`) | OpenPBR Direct (No GI) | GTX / RTX / CPU | 10-lobe OpenPBR direct + LTC polygonal area lights + 6-mode Shadow Lab + BVH4 specular & Abbe glass. |
| `2` (`StokesContourGi`) | **GTX GI #1: Stokes Contour GI** | **GTX (Compute) / CPU** | Exact closed-form spherical-polygon Stokes contour integral over real emissive triangles (1st bounce) + area-weighted `DiffuseSurfacePatch` 2nd-bounce radiosity + `WideBoundingTree` visibility. |
| `3` (`SurfelCascadeGi`) | **GTX GI #2: Surfel Cascade GI** | **GTX (Compute) / CPU** | Two-cascade `OctahedralIrradianceCascade` (9×7×4 inner + 7×5×4 outer, 8×8 irradiance + 16×16 Chebyshev depth moments) filled from real triangle surfels for O(1) trilinear+octahedral runtime lookup. |
| `4` (`DeterministicPathTrace`) | **Unified Deterministic Path Tracer** | **GTX (BVH4) & RTX (`VK_KHR_ray_query`)** | Zero-RNG Owen-scrambled Sobol `(0,2)` 5-stage bit-parallel quadrature + Dupuy–Benyoub (2023) Spherical-Cap GGX VNDF + NEE (Sun, Area Quads, Emissive Triangles) with Power-Heuristic MIS + progressive HDR accumulation. |

### 2.2 Six Real-Mesh Solar Shadow Algorithms (`SolarShadowAlgorithm` in `SolarShadowSequence.h`)

| # | Algorithm Enum | Mechanism (100% Real Triangles, Zero SDF) |
| :--- | :--- | :--- |
| `0` | `ContactPcss` | 2-Cascade `1024×1024` Depth32f PCSS (3.91 mm/tx fine + 15.6 mm/tx coarse) + 0.24 m `WideBoundingTree` contact gate + 3-step plane-aware À-trous filter. |
| `1` | `PcssPlaneAware` | 2-Cascade Vogel blocker search + continuous penumbra + 3-step plane-aware À-trous filter. |
| `2` | `ShadowRayMarch` | EEVEE-Next quadratic shadow depth raymarch (SMRT) over real-mesh depth cascades. |
| `3` | `PcfBilinear` | Fast 3×3 tent-weighted orthographic bilinear PCF over real-mesh depth cascades. |
| `4` | `WideTreeRayQuery` | Deterministic Vogel solar disc `WideBoundingTree` BVH4 / `VK_KHR_ray_query` traversal + plane-aware filter. |
| `5` | `SurfelTriangleField` | `SurfelOccluderField` stratified `OccluderSurfelDisk` field sampled directly from real triangle meshes + 0.28 m `WideBoundingTree` exact contact ray. |

### 2.3 Unified Deterministic Path Tracer (GTX + RTX)

- **Shared Deterministic Quadrature (`DeterministicIntegrators.h` + `DeterministicQuadrature.slang`)**:
  - Bit-exact Owen-scrambled Sobol `(0,2)` (`ReverseBits32`, `AvalancheKey32`, `NestedDigitPermutation`, `SobolSecondDimension`, `Sobol02Sample`).
  - Dupuy & Benyoub (2023) Spherical-Cap GGX VNDF (`SampleSphericalCapGgxVndfLocal`, `EvaluateSphericalCapGgxVndfPdf`) + `PowerHeuristicMis` + `BuildDuffBasis`.
  - Zero runtime RNG, zero Russian roulette, batch-size-independent sample indexing.
- **GTX Acceleration Path (`PathTraceAccelerationMode::GtxWideBoundingTree`)**:
  - Traverses 128-byte aligned 4-wide SAH `WideTreeEntry` BVH4 (`WideBoundingTree` / `WideTreeRayQuery.slang`) in compute shaders (`WavefrontPathTraceCompute.slang`).
- **RTX Acceleration Path (`PathTraceAccelerationMode::RtxInlineRayQuery`)**:
  - Uses inline `VK_KHR_ray_query` (`rayQueryEXT`) over `VK_KHR_acceleration_structure` two-level BLAS/TLAS (opaque geometry + non-opaque `ThinDielectric` glass sorted deterministically by `t`), while sharing the exact same quadrature, MIS/NEE, and BSDF shader core (`WavefrontPathTraceCompute.slang`). Deforming character meshes update BLAS via `VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR` refits.

---

## 3. Repository Folder Structure (Compliant with `FolderStructure.md` & `BannedWordsVocabulary.md`)

```text
Frontier/
├── CMakeLists.txt
├── Automation/
│   └── RenderControlVehicleGi.sh
├── Documentation/
│   └── Plans/
│       ├── README.md
│       ├── MarkdownStudioViewer.html
│       ├── WorldRenderEngineUnificationPlan.md
│       └── WorldGiShadowPathTracerPlan.md             <-- [NEW] This architecture plan
├── Engine/
│   ├── SpatialAcceleration/
│   │   ├── WideBoundingTree.{h,cpp}                   <-- 128 B 4-wide SAH BVH4 (GTX compute + CPU)
│   │   ├── ClusterPyramid.{h,cpp}
│   │   └── TriangleVisibilitySpace.h
│   ├── GeometricRaster/
│   │   ├── DirectLighting/
│   │   │   ├── LinearCosineLightIntegration.{h,cpp}
│   │   │   ├── SolarShadowSequence.{h,cpp}            <-- [UPDATED] 6 Shadow Algorithms (0..5)
│   │   │   └── SurfelOccluderField.{h,cpp}            <-- [WIRED] Mode 5 real-triangle surfel shadow
│   │   └── GlobalIllumination/
│   │       ├── DeterministicIntegrators.h             <-- [NEW] Shared Sobol (0,2) + Dupuy-Benyoub GGX VNDF
│   │       ├── DeterministicDiffuseGi.h               <-- Deterministic hemisphere diffuse reference
│   │       ├── DeterministicPathTraceSequence.{h,cpp} <-- [NEW] Unified GTX & RTX deterministic path tracer
│   │       ├── StokesContourGiSequence.{h,cpp}        <-- [WIRED] GTX GI #1 (Stokes contour + 2-bounce patches)
│   │       ├── SurfelCascadeGiSequence.{h,cpp}        <-- [WIRED] GTX GI #2 (Octahedral cascades + Chebyshev)
│   │       ├── SpecularReflectionSequence.{h,cpp}
│   │       ├── TieredWorldRenderSequence.{h,cpp}      <-- [UPDATED] 5 WorldRenderEngine modes
│   │       └── VulkanWorldRenderSequence.{h,cpp}      <-- [UPDATED] GPU dispatch for all 5 engines & 6 shadows
│   ├── Shaders/
│   │   ├── DirectLighting/
│   │   │   ├── SolarFrustumShadow.slang
│   │   │   ├── PlaneAwareShadowFilter.slang
│   │   │   ├── WideTreeRayQuery.slang
│   │   │   └── SurfelTriangleShadow.slang             <-- [NEW] Mode 5 GPU surfel shadow evaluation
│   │   ├── GlobalIllumination/
│   │   │   ├── DeterministicQuadrature.slang          <-- [NEW] Slang Sobol (0,2) + GGX VNDF
│   │   │   ├── StokesContourGiCompute.slang           <-- [NEW] GTX GI #1 GPU compute
│   │   │   ├── SurfelCascadeGiCompute.slang           <-- [NEW] GTX GI #2 GPU compute
│   │   │   └── WavefrontPathTraceCompute.slang        <-- [NEW] GTX BVH4 + RTX VK_KHR_ray_query path tracer
│   │   └── GeometricRaster/
│   │       └── WorldViewportRadiometricCompute.slang  <-- [UPDATED] Unified 5-engine & 6-shadow dispatcher
│   ├── Components/Panes/
│   │   └── ViewportPanel.{h,cpp}                      <-- [UPDATED] 5 Engines & 6 Shadows UI selectors
│   └── Lifecycle/
│       └── EditorLifecycle.cpp                        <-- [UPDATED] Progressive accumulation & mode routing
└── Exhibits/
    ├── ControlVehicleRenderExhibit.cpp                <-- Multi-engine ControlVehicle renders
    ├── DeterministicDiffuseGiValidation.cpp           <-- Analytic diffuse GI proof
    ├── SolarShadowValidation.cpp                      <-- 6-mode Shadow Lab validation
    ├── TieredWorldRenderValidation.cpp                <-- 5-engine world render validation
    └── WorldGiShadowPathTraceValidation.cpp           <-- [UPDATED] End-to-end 6-shadow + 2-GI + GTX/RTX PT suite
```

---

## 4. Verification Gates

1. **Naming & Governance Gate**: Zero banned words (`and`, `mesh`, `pass`, `stage`, `frame`, `node`, `pipeline`, `table`, `pool`, `probe`, `core`, `base`, `common`, `data`, `info`, `object`, `context`, `state`, `value`, `flag`, etc.) in all newly introduced files and identifiers.
2. **Real-Mesh & Zero-SDF Gate**: Zero `SignedDistanceVolume` / `VoxelSdfClipmap` dependencies in `DirectLighting/` or `GlobalIllumination/`, and zero analytical capsule/sphere/box shadow proxies.
3. **C++20 Validation Gate**: Compile and execute `Exhibits/WorldGiShadowPathTraceValidation.cpp` verifying:
   - Stokes contour closed-form invariance & independent quadrature parity
   - Octahedral cascade encoding/decoding & Chebyshev leak suppression
   - All 6 `SolarShadowAlgorithm` modes (`ContactPcss`, `PcssPlaneAware`, `ShadowRayMarch`, `PcfBilinear`, `WideTreeRayQuery`, `SurfelTriangleField`) on real meshes + deformed triangle updates
   - Both GTX GI renderers (`StokesContourGi`, `SurfelCascadeGi`) through `TieredWorldRenderSequence`
   - `DeterministicPathTraceSequence` in both `GtxWideBoundingTree` and `RtxInlineRayQuery` modes (bit-exact determinism, NEE+MIS energy conservation, zero NaN/Inf)
4. **Shader Compilation Gate**: Validate `.slang` modules (`slangc` SPIR-V 1.6 where toolchain is installed; compile-clean structure for CI).
