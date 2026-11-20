/*============================================================================================================================================
                                                         SURFELOCCLUDERFIELD.H
============================================================================================================================================*/
#pragma once
#include "Engine/SpatialAcceleration/WideBoundingTree.h"
#include <vector>

namespace Frontier::GeometricRaster
{
struct OccluderSurfelDisk
{
    Mathematics::Axis3D Position {};
    Mathematics::Axis3D Normal {};
    float RadiusMeters = 0.0f;
};

// Approximate distant solar blockers sampled from the supplied real triangles.
// Exact short triangle queries preserve contact; no bone or implicit-solid substitutes.
class SurfelOccluderField
{
public:
    std::vector<OccluderSurfelDisk> Disks {};
    void BuildFromTriangles(const SpatialAcceleration::WideBoundingTree& SceneTree);
    [[nodiscard]] float EvaluateSolarVisibility(
        const SpatialAcceleration::WideBoundingTree& SceneTree,
        Mathematics::Axis3D Position, Mathematics::Axis3D Normal,
        Mathematics::Axis3D DirectionToSun, float TanSunRadius) const noexcept;
};
} // namespace Frontier::GeometricRaster
