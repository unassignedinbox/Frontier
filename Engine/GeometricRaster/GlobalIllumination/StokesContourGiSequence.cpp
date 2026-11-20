/*============================================================================================================================================
                                                      STOKESCONTOURGISEQUENCE.CPP
============================================================================================================================================*/
#include "Engine/GeometricRaster/GlobalIllumination/StokesContourGiSequence.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <tuple>

namespace Frontier::GeometricRaster
{
using Mathematics::Axis3D;
using Mathematics::LinearRGB;
using Mathematics::EvaluateDot;
using Mathematics::EvaluateCross;
using Mathematics::EvaluateLength;
using Mathematics::NormalizeAxis;

float StokesContourGiSequence::EvaluateTriangleFormFactor(
    Axis3D ReceiverP, Axis3D ReceiverN, Axis3D VertexA, Axis3D VertexB, Axis3D VertexC) noexcept
{
    const std::array<Axis3D, 3> Input { VertexA - ReceiverP, VertexB - ReceiverP, VertexC - ReceiverP };
    std::array<Axis3D, 4> Polygon {};
    uint32_t Count = 0;
    for (uint32_t I = 0; I < 3; ++I)
    {
        const auto A = Input[I];
        const auto B = Input[(I + 1) % 3];
        const float DA = EvaluateDot(A, ReceiverN);
        const float DB = EvaluateDot(B, ReceiverN);
        if (DA >= 0.0f) Polygon[Count++] = A;
        if ((DA < 0.0f) != (DB < 0.0f))
        {
            // Keep the denominator sign: clamping it positive breaks entering edges.
            Polygon[Count++] = A + (B - A) * (DA / (DA - DB));
        }
    }
    if (Count < 3) return 0.0f;
    for (uint32_t I = 0; I < Count; ++I)
    {
        if (EvaluateLength(Polygon[I]) < 1.0e-7f) return 0.0f;
        Polygon[I] = NormalizeAxis(Polygon[I]);
    }
    double Sum = 0.0;
    for (uint32_t I = 0; I < Count; ++I)
    {
        const auto A = Polygon[I];
        const auto B = Polygon[(I + 1) % Count];
        const auto Cross = EvaluateCross(A, B);
        const float Length = EvaluateLength(Cross);
        if (Length > 1.0e-7f)
        {
            Sum += std::atan2(Length, EvaluateDot(A, B)) * EvaluateDot(Cross, ReceiverN) / Length;
        }
    }
    return std::clamp(static_cast<float>(std::abs(Sum) / 6.283185307179586), 0.0f, 1.0f);
}

bool StokesContourGiSequence::SegmentVisible(Axis3D Position, Axis3D Normal, Axis3D Target) const noexcept
{
    if (Tree == nullptr) return false;
    const auto Origin = Position + Normal * 0.006f;
    const auto Delta = Target - Origin;
    const float Distance = EvaluateLength(Delta);
    if (Distance < 0.012f) return false;
    return !Tree->QueryRayOccluded({ Origin, 0.002f, Delta * (1.0f / Distance), Distance - 0.008f });
}

LinearRGB StokesContourGiSequence::EvaluateEmissiveOverPi(Axis3D Position, Axis3D Normal) const noexcept
{
    LinearRGB Sum {};
    for (const auto& Source : Sources)
    {
        const auto Center = (Source.VertexA + Source.VertexB + Source.VertexC) * (1.0f / 3.0f);
        if (EvaluateDot(Source.UnitNormal, Position - Center) <= 0.0f) continue;
        const float Form = EvaluateTriangleFormFactor(Position, Normal, Source.VertexA, Source.VertexB, Source.VertexC);
        if (Form <= 1.0e-7f) continue;
        // Four deterministic triangle locations approximate partial visibility only.
        // The unoccluded contour integral itself is closed form, not hemisphere ray tracing.
        float Visibility = 0.0f;
        const std::array<Axis3D, 4> Targets {
            Center, Source.VertexA * 0.6f + Source.VertexB * 0.2f + Source.VertexC * 0.2f,
            Source.VertexA * 0.2f + Source.VertexB * 0.6f + Source.VertexC * 0.2f,
            Source.VertexA * 0.2f + Source.VertexB * 0.2f + Source.VertexC * 0.6f };
        for (const auto& Target : Targets)
        {
            if (SegmentVisible(Position, Normal, Target)) Visibility += 0.25f;
        }
        Sum = Sum + Source.Radiance * (Form * Visibility);
    }
    return Sum;
}

float EvaluateSurfelDiskFormFactor(Axis3D Position, Axis3D Normal, const DiffuseSurfacePatch& Patch) noexcept
{
    const auto Delta = Patch.Center - Position;
    const float DistanceSquared = Mathematics::EvaluateLengthSquared(Delta);
    if (DistanceSquared < 0.0025f) return 0.0f;
    const auto Direction = Delta * (1.0f / std::sqrt(DistanceSquared));
    const float ReceiverCos = std::max(0.0f, EvaluateDot(Normal, Direction));
    const float SourceCos = std::max(0.0f, -EvaluateDot(Patch.UnitNormal, Direction));
    return ReceiverCos * SourceCos * Patch.AreaM2 / (3.14159265359f * DistanceSquared + Patch.AreaM2);
}

LinearRGB StokesContourGiSequence::EvaluateBounceOverPi(Axis3D Position, Axis3D Normal) const noexcept
{
    LinearRGB Sum {};
    for (const auto& Patch : Patches)
    {
        const float Form = EvaluateSurfelDiskFormFactor(Position, Normal, Patch);
        if (Form < 1.0e-6f) continue;
        if (SegmentVisible(Position, Normal, Patch.Center + Patch.UnitNormal * 0.008f))
        {
            Sum = Sum + Patch.Radiance * Form;
        }
    }
    return Sum;
}

LinearRGB StokesContourGiSequence::EvaluateIrradianceOverPi(Axis3D Position, Axis3D Normal) const noexcept
{
    return EvaluateEmissiveOverPi(Position, Normal) + EvaluateBounceOverPi(Position, Normal);
}

void StokesContourGiSequence::Prepare(
    const SpatialAcceleration::WideBoundingTree& SceneTree,
    std::span<const LinearRGB> EmissionBySlot,
    std::span<const LinearRGB> ReflectanceBySlot,
    Axis3D DirectionToSun, LinearRGB SolarIrradiance)
{
    Tree = &SceneTree;
    Sources.clear();
    Patches.clear();
    struct PatchSum
    {
        Axis3D Center {};
        Axis3D Normal {};
        float Area = 0.0f;
        uint16_t Slot = 0;
    };
    std::map<std::tuple<int, int, int, int, int, int, uint16_t>, PatchSum> Cells;
    for (const auto& Triangle : Tree->Primitives)
    {
        const auto A = Triangle.Corner0;
        const auto B = A + Triangle.Edge01;
        const auto C = A + Triangle.Edge02;
        const auto Cross = EvaluateCross(Triangle.Edge01, Triangle.Edge02);
        const float Area = EvaluateLength(Cross) * 0.5f;
        if (Area < 1.0e-7f) continue;
        const auto Normal = NormalizeAxis(Cross);
        const auto Slot = Triangle.MaterialOrdinal;
        const auto Emission = Slot < EmissionBySlot.size() ? EmissionBySlot[Slot] : LinearRGB {};
        if (std::max({Emission.R, Emission.G, Emission.B}) > 0.0f)
        {
            Sources.push_back({ A, B, C, Normal, Emission, Area });
            continue;
        }
        // Subdivide only to acquire area samples of the supplied triangles.
        // Large studio triangles must not collapse to a single remote centroid.
        const uint32_t Divisions = std::clamp(static_cast<uint32_t>(std::ceil(std::sqrt(Area / 0.5f))), 1u, 64u);
        for (uint32_t I = 0; I < Divisions; ++I)
        {
            for (uint32_t J = 0; J < Divisions; ++J)
            {
                const float U = std::sqrt((I + 0.5f) / Divisions);
                const float V = (J + 0.5f) / Divisions;
                const auto Center = A * (1.0f - U) + B * (U * (1.0f - V)) + C * (U * V);
                if (std::abs(Center.X) > 7.0f || std::abs(Center.Y) > 6.0f || Center.Z < -0.01f || Center.Z > 4.0f)
                    continue;
                const auto Key = std::make_tuple(
                    static_cast<int>(std::floor(Center.X / 0.9f)), static_cast<int>(std::floor(Center.Y / 0.9f)),
                    static_cast<int>(std::floor(Center.Z / 0.9f)), static_cast<int>(std::round(Normal.X * 2)),
                    static_cast<int>(std::round(Normal.Y * 2)), static_cast<int>(std::round(Normal.Z * 2)), Slot);
                auto& Cell = Cells[Key];
                const float Weight = Area / (Divisions * Divisions);
                Cell.Center = Cell.Center + Center * Weight;
                Cell.Normal = Cell.Normal + Normal * Weight;
                Cell.Area += Weight;
                Cell.Slot = Slot;
            }
        }
    }
    for (const auto& [Key, Cell] : Cells)
    {
        const auto Reflectance = Cell.Slot < ReflectanceBySlot.size() ? ReflectanceBySlot[Cell.Slot] : LinearRGB {};
        Patches.push_back({ Cell.Center * (1.0f / Cell.Area), NormalizeAxis(Cell.Normal), Reflectance, {}, Cell.Area });
    }
    for (auto& Patch : Patches)
    {
        auto Incoming = EvaluateEmissiveOverPi(Patch.Center, Patch.UnitNormal);
        const float Cosine = std::max(0.0f, EvaluateDot(Patch.UnitNormal, DirectionToSun));
        if (Cosine > 0 && !Tree->QueryRayOccluded({ Patch.Center + Patch.UnitNormal * 0.006f,
            0.002f, DirectionToSun, 100.0f }))
        {
            Incoming = Incoming + SolarIrradiance * (Cosine / 3.14159265359f);
        }
        Patch.Radiance = { Incoming.R * Patch.Reflectance.R, Incoming.G * Patch.Reflectance.G,
            Incoming.B * Patch.Reflectance.B };
    }
}

} // namespace Frontier::GeometricRaster
