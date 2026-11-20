/*============================================================================================================================================
                                                        SURFELOCCLUDERFIELD.CPP
============================================================================================================================================*/
#include "Engine/GeometricRaster/DirectLighting/SurfelOccluderField.h"
#include <algorithm>
#include <cmath>
#include <map>
#include <tuple>

namespace Frontier::GeometricRaster
{
void SurfelOccluderField::BuildFromTriangles(const SpatialAcceleration::WideBoundingTree& SceneTree)
{
    Disks.clear();
    struct AreaSum
    {
        Mathematics::Axis3D Position {};
        Mathematics::Axis3D Normal {};
        float Area = 0;
    };
    std::map<std::tuple<int, int, int, int, int, int>, AreaSum> Cells;
    for (const auto& Triangle : SceneTree.Primitives)
    {
        if (Triangle.OpaqueMask == 0) continue;
        const auto Cross = Mathematics::EvaluateCross(Triangle.Edge01, Triangle.Edge02);
        const float Area = Mathematics::EvaluateLength(Cross) * 0.5f;
        if (Area < 1.0e-8f) continue;
        const auto Normal = Mathematics::NormalizeAxis(Cross);
        // Sample large triangles as well: room geometry is not an analytic proxy.
        const uint32_t Divisions = std::clamp(static_cast<uint32_t>(std::ceil(std::sqrt(Area / 0.04f))), 1u, 128u);
        for (uint32_t I = 0; I < Divisions; ++I)
        {
            for (uint32_t J = 0; J < Divisions; ++J)
            {
                const float U = std::sqrt((I + 0.5f) / Divisions);
                const float V = (J + 0.5f) / Divisions;
                const auto P = Triangle.Corner0 + Triangle.Edge01 * (U * (1-V)) + Triangle.Edge02 * (U*V);
                if (std::abs(P.X) > 7 || std::abs(P.Y) > 6 || P.Z < 0.02f || P.Z > 3.5f) continue;
                const auto Key = std::make_tuple(static_cast<int>(std::floor(P.X / 0.35f)),
                    static_cast<int>(std::floor(P.Y / 0.35f)), static_cast<int>(std::floor(P.Z / 0.35f)),
                    static_cast<int>(std::round(Normal.X*2)), static_cast<int>(std::round(Normal.Y*2)),
                    static_cast<int>(std::round(Normal.Z*2)));
                auto& Cell = Cells[Key];
                const float Weight = Area / (Divisions * Divisions);
                Cell.Position = Cell.Position + P * Weight;
                Cell.Normal = Cell.Normal + Normal * Weight;
                Cell.Area += Weight;
            }
        }
    }
    for (const auto& [Key, Cell] : Cells)
    {
        Disks.push_back({ Cell.Position * (1.0f / Cell.Area), Mathematics::NormalizeAxis(Cell.Normal),
            std::sqrt(Cell.Area / 3.14159265359f) * 1.2f });
    }
}

float SurfelOccluderField::EvaluateSolarVisibility(
    const SpatialAcceleration::WideBoundingTree& SceneTree,
    Mathematics::Axis3D Position, Mathematics::Axis3D Normal,
    Mathematics::Axis3D DirectionToSun, float TanSunRadius) const noexcept
{
    if (Mathematics::EvaluateDot(Normal, DirectionToSun) <= 0) return 0;
    const auto Origin = Position + Normal * 0.006f;
    if (SceneTree.QueryRayOccluded({ Origin, 0.002f, DirectionToSun, 0.28f })) return 0;
    float Visibility = 1;
    for (const auto& Disk : Disks)
    {
        const auto Delta = Disk.Position - Origin;
        const float Distance = Mathematics::EvaluateDot(Delta, DirectionToSun);
        if (Distance < 0.14f || Distance > 15) continue;
        if (Mathematics::EvaluateDot(Delta, Normal) < 0.02f) continue;
        const float Perpendicular = Mathematics::EvaluateLength(Delta - DirectionToSun * Distance);
        const float ConeRadius = std::max(0.005f, Distance * TanSunRadius);
        // Project disk area onto the solar plane, then smooth over the solar cone.
        const float ProjectedRadius = Disk.RadiusMeters *
            std::sqrt(std::max(0.02f, std::abs(Mathematics::EvaluateDot(Disk.Normal, DirectionToSun))));
        const float T = std::clamp((Perpendicular - ProjectedRadius + ConeRadius) / (2*ConeRadius), 0.0f, 1.0f);
        Visibility = std::min(Visibility, T*T*(3-2*T));
        if (Visibility == 0) break;
    }
    return Visibility;
}
} // namespace Frontier::GeometricRaster
