/*============================================================================================================================================
                                                       STOKESCONTOURGISEQUENCE.H
============================================================================================================================================*/
#pragma once

#include "Engine/SpatialAcceleration/WideBoundingTree.h"
#include "Engine/Mathematics/LinearColors.h"
#include <span>
#include <vector>

namespace Frontier::GeometricRaster
{

struct EmissiveTriangleSource
{
    Mathematics::Axis3D    VertexA {};
    Mathematics::Axis3D    VertexB {};
    Mathematics::Axis3D    VertexC {};
    Mathematics::Axis3D    UnitNormal {};
    Mathematics::LinearRGB Radiance {};
    float                  AreaM2 = 0.0f;
};

struct DiffuseSurfacePatch
{
    Mathematics::Axis3D    Center {};
    Mathematics::Axis3D    UnitNormal {};
    Mathematics::LinearRGB Reflectance {};
    Mathematics::LinearRGB Radiance {};
    float                  AreaM2 = 0.0f;
};

// Real triangle sources and area-weighted surface samples. No distance volume or character proxies.
class StokesContourGiSequence
{
public:
    std::vector<EmissiveTriangleSource> Sources {};
    std::vector<DiffuseSurfacePatch>   Patches {};

    void Prepare(
        const SpatialAcceleration::WideBoundingTree& SceneTree,
        std::span<const Mathematics::LinearRGB> EmissionBySlot,
        std::span<const Mathematics::LinearRGB> ReflectanceBySlot,
        Mathematics::Axis3D DirectionToSun,
        Mathematics::LinearRGB SolarIrradiance);

    [[nodiscard]] static float EvaluateTriangleFormFactor(
        Mathematics::Axis3D ReceiverP,
        Mathematics::Axis3D ReceiverN,
        Mathematics::Axis3D VertexA,
        Mathematics::Axis3D VertexB,
        Mathematics::Axis3D VertexC) noexcept;

    [[nodiscard]] Mathematics::LinearRGB EvaluateEmissiveOverPi(
        Mathematics::Axis3D Position, Mathematics::Axis3D Normal) const noexcept;
    [[nodiscard]] Mathematics::LinearRGB EvaluateBounceOverPi(
        Mathematics::Axis3D Position, Mathematics::Axis3D Normal) const noexcept;
    [[nodiscard]] Mathematics::LinearRGB EvaluateIrradianceOverPi(
        Mathematics::Axis3D Position, Mathematics::Axis3D Normal) const noexcept;
    [[nodiscard]] bool SegmentVisible(
        Mathematics::Axis3D Position, Mathematics::Axis3D Normal,
        Mathematics::Axis3D Target) const noexcept;

private:
    const SpatialAcceleration::WideBoundingTree* Tree = nullptr;
};

[[nodiscard]] float EvaluateSurfelDiskFormFactor(
    Mathematics::Axis3D Position, Mathematics::Axis3D Normal,
    const DiffuseSurfacePatch& Patch) noexcept;

} // namespace Frontier::GeometricRaster
