/*============================================================================================================================================
                                                    DETERMINISTICPATHTRACESEQUENCE.H
============================================================================================================================================*/
// 🧩 Unified Deterministic Path Tracer (GTX 4-Wide BVH4 + RTX VK_KHR_ray_query Parity, Sobol (0,2) Quadrature, Dupuy-Benyoub GGX VNDF)

#pragma once

//------------------------------------------------------------------------------------------------------------------------
//                                                        INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Engine/GeometricRaster/DirectLighting/LinearCosineLightIntegration.h"
#include "Engine/GeometricRaster/GlobalIllumination/DeterministicIntegrators.h"
#include "Engine/GeometricRaster/GlobalIllumination/StokesContourGiSequence.h"
#include "Engine/MaterialSpace/CompactMaterialRegister.h"
#include "Engine/SpatialAcceleration/WideBoundingTree.h"

#include <cstdint>
#include <span>
#include <vector>

namespace Frontier::GeometricRaster
{

//------------------------------------------------------------------------------------------------------------------------
//                                                      ENUMERATIONS
//------------------------------------------------------------------------------------------------------------------------

enum class PathTraceAccelerationMode : uint8_t
{
    GtxWideBoundingTree = 0u, // [-] - 128-byte 4-Wide SAH BVH4 traversal for GTX compute & CPU reference
    RtxInlineRayQuery   = 1u, // [-] - Inline VK_KHR_ray_query BLAS/TLAS with deterministic sorted thin-glass accumulation
};

//------------------------------------------------------------------------------------------------------------------------
//                                                       STRUCTURES
//------------------------------------------------------------------------------------------------------------------------

struct PathTraceConfiguration
{
    uint32_t                  WidthPx                 = 0u;   // [px]    - Viewport width
    uint32_t                  HeightPx                = 0u;   // [px]    - Viewport height
    uint32_t                  SamplesPerPixel         = 16u;  // [count] - Deterministic Sobol (0,2) samples per pixel
    uint32_t                  MaxBounceCount          = 4u;   // [count] - Maximum path scatter bounces
    uint32_t                  StartSampleIndex        = 0u;   // [idx]   - First Sobol sequence index for progressive refinement
    PathTraceAccelerationMode AccelerationMode        = PathTraceAccelerationMode::GtxWideBoundingTree;
    bool                      SharedPixelQuadrature   = false; // [-]    - True shares Sobol (0,2) net across pixels; false uses Owen pixel key
    float                     SolarAngularRadiusRad   = 0.028f; // [rad] - Solar disc angular radius for NEE cone sampling
};

struct PathTraceOutput
{
    uint32_t                            WidthPx             = 0u;   // [px]    - Viewport width
    uint32_t                            HeightPx            = 0u;   // [px]    - Viewport height
    uint32_t                            CompletedSamples    = 0u;   // [count] - Total accumulated samples per pixel
    PathTraceAccelerationMode           ActiveMode          = PathTraceAccelerationMode::GtxWideBoundingTree;
    std::vector<Mathematics::LinearRGB> LinearHdrRadiance   {};     // [cd/m2] - Mean linear HDR radiance
    std::vector<uint8_t>                DisplayRgba8        {};     // [rgba8] - ACES filmic + sRGB8 output
};

//------------------------------------------------------------------------------------------------------------------------
//                                             DETERMINISTIC PATH TRACE SEQUENCE
//------------------------------------------------------------------------------------------------------------------------

class DeterministicPathTraceSequence
{
public:
    std::vector<EmissiveTriangleSource> EmissiveTriangles {}; // [-]  - Real emissive triangles extracted from SceneTree
    std::vector<float>                  EmissiveCdf       {}; // [0-1]- Area-weighted CDF for deterministic emissive NEE
    float                               TotalEmissiveArea = 0.0f; // [m2] - Sum of emissive triangle areas

    // Builds the emissive triangle NEE distribution from the supplied real triangle BVH and material register
    void PrepareSceneSources(
        const SpatialAcceleration::WideBoundingTree&  SceneTree,
        const MaterialSpace::CompactMaterialRegister& MaterialRegister);

    // Evaluates one pixel deterministically (bit-exact across batch partitions and GTX/RTX acceleration modes)
    [[nodiscard]] Mathematics::LinearRGB TracePixelRadiance(
        const PathTraceConfiguration&                 Config,
        uint32_t                                      PixelX,
        uint32_t                                      PixelY,
        Mathematics::Axis3D                           RayOrigin,
        Mathematics::Axis3D                           RayDirection,
        const SpatialAcceleration::WideBoundingTree&  SceneTree,
        const MaterialSpace::CompactMaterialRegister& MaterialRegister,
        const SolarDirectionalSource&                 Sun,
        std::span<const PolygonalAreaQuadLight>       AreaLights,
        Mathematics::LinearRGB                        SkyRadiance) const noexcept;

    // Renders the complete viewport deterministically (zero RNG, zero Russian roulette, zero denoiser)
    [[nodiscard]] PathTraceOutput RenderViewport(
        const PathTraceConfiguration&                 Config,
        const PerspectiveCameraBasis&                 Camera,
        const SpatialAcceleration::WideBoundingTree&  SceneTree,
        const MaterialSpace::CompactMaterialRegister& MaterialRegister,
        const SolarDirectionalSource&                 Sun,
        std::span<const PolygonalAreaQuadLight>       AreaLights,
        Mathematics::LinearRGB                        SkyRadiance) const;
};

} // namespace Frontier::GeometricRaster
