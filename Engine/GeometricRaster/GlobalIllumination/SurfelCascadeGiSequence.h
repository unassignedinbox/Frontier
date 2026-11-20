/*============================================================================================================================================
                                                       SURFELCASCADEGISEQUENCE.H
============================================================================================================================================*/
#pragma once
#include "Engine/GeometricRaster/GlobalIllumination/StokesContourGiSequence.h"
#include <array>

namespace Frontier::GeometricRaster
{

struct OctahedralIrradianceProbe
{
    Mathematics::Axis3D Position {};
    std::array<Mathematics::LinearRGB, 64> IrradianceOverPi {};
    std::array<Mathematics::Axis2D, 256> DepthMoments {};
};

struct OctahedralIrradianceCascade
{
    Mathematics::Axis3D Minimum {};
    Mathematics::Axis3D Spacing {};
    std::array<uint32_t, 3> Counts {};
    std::vector<OctahedralIrradianceProbe> Probes {};
};

class SurfelCascadeGiSequence
{
public:
    std::array<OctahedralIrradianceCascade, 2> Cascades {};
    std::vector<DiffuseSurfacePatch> RadiantSurfels {};

    void Prepare(const StokesContourGiSequence& Stokes, const SpatialAcceleration::WideBoundingTree& SceneTree);
    [[nodiscard]] Mathematics::LinearRGB EvaluateIrradianceOverPi(
        Mathematics::Axis3D Position, Mathematics::Axis3D Normal) const noexcept;
    [[nodiscard]] static Mathematics::Axis3D DecodeOctahedral(float U, float V) noexcept;
    [[nodiscard]] static Mathematics::Axis2D EncodeOctahedral(Mathematics::Axis3D Direction) noexcept;

private:
    [[nodiscard]] Mathematics::LinearRGB SampleCascade(
        const OctahedralIrradianceCascade& Cascade, Mathematics::Axis3D Position,
        Mathematics::Axis3D Normal, float& Confidence) const noexcept;
};

} // namespace Frontier::GeometricRaster
