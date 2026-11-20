/*============================================================================================================================================
                                                      SURFELCASCADEGISEQUENCE.CPP
============================================================================================================================================*/
#include "Engine/GeometricRaster/GlobalIllumination/SurfelCascadeGiSequence.h"
#include <algorithm>
#include <cmath>
#include <thread>

namespace Frontier::GeometricRaster
{
using Mathematics::Axis3D;
using Mathematics::Axis2D;
using Mathematics::LinearRGB;
using Mathematics::EvaluateDot;
using Mathematics::NormalizeAxis;

Axis3D SurfelCascadeGiSequence::DecodeOctahedral(float U, float V) noexcept
{
    Axis3D Direction { U * 2.0f - 1.0f, V * 2.0f - 1.0f, 0 };
    Direction.Z = 1.0f - std::abs(Direction.X) - std::abs(Direction.Y);
    if (Direction.Z < 0)
    {
        const float X = Direction.X;
        Direction.X = (1.0f - std::abs(Direction.Y)) * (X >= 0 ? 1.0f : -1.0f);
        Direction.Y = (1.0f - std::abs(X)) * (Direction.Y >= 0 ? 1.0f : -1.0f);
    }
    return NormalizeAxis(Direction);
}

Axis2D SurfelCascadeGiSequence::EncodeOctahedral(Axis3D Direction) noexcept
{
    const float Length = std::abs(Direction.X) + std::abs(Direction.Y) + std::abs(Direction.Z);
    if (Length < 1.0e-7f) return { 0.5f, 0.5f };
    Direction = Direction * (1.0f / Length);
    if (Direction.Z < 0)
    {
        const float X = Direction.X;
        Direction.X = (1.0f - std::abs(Direction.Y)) * (X >= 0 ? 1.0f : -1.0f);
        Direction.Y = (1.0f - std::abs(X)) * (Direction.Y >= 0 ? 1.0f : -1.0f);
    }
    return { Direction.X * 0.5f + 0.5f, Direction.Y * 0.5f + 0.5f };
}

void SurfelCascadeGiSequence::Prepare(
    const StokesContourGiSequence& Stokes, const SpatialAcceleration::WideBoundingTree& SceneTree)
{
    RadiantSurfels = Stokes.Patches;
    // Near field uses each emissive triangle as its finite disk; distant sources
    // are coalesced by the probe integral, not substituted with a hemisphere trace.
    for (const auto& Source : Stokes.Sources)
    {
        RadiantSurfels.push_back({ (Source.VertexA + Source.VertexB + Source.VertexC) * (1.0f / 3.0f),
            Source.UnitNormal, {}, Source.Radiance, Source.AreaM2 });
    }
    Cascades[0] = { { -4.5f, -3.6f, 0.15f }, { 1.125f, 1.2f, 1.1f }, { 9, 7, 4 }, {} };
    Cascades[1] = { { -9.0f, -7.5f, 0.2f }, { 3.0f, 3.75f, 2.0f }, { 7, 5, 4 }, {} };
    std::array<Axis3D, 64> IrradianceDirections {};
    for (uint32_t K = 0; K < 64; ++K)
        IrradianceDirections[K] = DecodeOctahedral((K % 8 + 0.5f) / 8, (K / 8 + 0.5f) / 8);
    for (auto& Cascade : Cascades)
    {
        const auto NX = Cascade.Counts[0];
        const auto NY = Cascade.Counts[1];
        Cascade.Probes.resize(NX * NY * Cascade.Counts[2]);
        const unsigned ThreadCount = std::clamp(std::thread::hardware_concurrency(), 1u, 16u);
        std::vector<std::thread> Workers;
        for (unsigned Worker = 0; Worker < ThreadCount; ++Worker)
        {
            Workers.emplace_back([&, Worker]()
            {
                for (uint32_t I = Worker; I < Cascade.Probes.size(); I += ThreadCount)
                {
                    auto& Probe = Cascade.Probes[I];
                    Probe.Position = Cascade.Minimum + Axis3D {
                        (I % NX) * Cascade.Spacing.X, ((I / NX) % NY) * Cascade.Spacing.Y,
                        (I / (NX * NY)) * Cascade.Spacing.Z };
                    for (const auto& Surfel : RadiantSurfels)
                    {
                        const auto Delta = Surfel.Center - Probe.Position;
                        const float DistanceSquared = Mathematics::EvaluateLengthSquared(Delta);
                        if (DistanceSquared < 0.0025f) continue;
                        const float Distance = std::sqrt(DistanceSquared);
                        const auto Direction = Delta * (1.0f / Distance);
                        const float SourceCos = std::max(0.0f, -EvaluateDot(Surfel.UnitNormal, Direction));
                        if (SourceCos < 1.0e-5f) continue;
                        const auto Target = Surfel.Center + Surfel.UnitNormal * 0.008f;
                        const auto Segment = Target - Probe.Position;
                        const float SegmentLength = Mathematics::EvaluateLength(Segment);
                        if (SceneTree.QueryRayOccluded({ Probe.Position, 0.004f,
                            Segment * (1.0f / SegmentLength), SegmentLength - 0.012f })) continue;
                        const float Form = SourceCos * Surfel.AreaM2 /
                            (3.14159265359f * DistanceSquared + Surfel.AreaM2);
                        for (uint32_t K = 0; K < 64; ++K)
                        {
                            const float Cosine = std::max(0.0f, EvaluateDot(IrradianceDirections[K], Direction));
                            Probe.IrradianceOverPi[K] = Probe.IrradianceOverPi[K] + Surfel.Radiance * (Form * Cosine);
                        }
                    }
                    // Depth moments use actual scene triangles, four directions per bin.
                    // No spatial filtering or denoiser. These bound interpolation leakage.
                    for (uint32_t K = 0; K < 256; ++K)
                    {
                        Axis2D Moments {};
                        for (uint32_t S = 0; S < 4; ++S)
                        {
                            const auto Direction = DecodeOctahedral(
                                (K % 16 + ((S & 1) ? 0.75f : 0.25f)) / 16,
                                (K / 16 + ((S & 2) ? 0.75f : 0.25f)) / 16);
                            const auto Hit = SceneTree.TraceClosestRay({ Probe.Position, 0.004f, Direction, 40.0f });
                            const float Distance = Hit.HitConfirmed ? Hit.HitDistanceMeters : 40.0f;
                            Moments.X += Distance * 0.25f;
                            Moments.Y += Distance * Distance * 0.25f;
                        }
                        Probe.DepthMoments[K] = Moments;
                    }
                }
            });
        }
        for (auto& Worker : Workers) Worker.join();
    }
}

LinearRGB SurfelCascadeGiSequence::SampleCascade(
    const OctahedralIrradianceCascade& Cascade, Axis3D Position, Axis3D Normal, float& Confidence) const noexcept
{
    Confidence = 0;
    if (Cascade.Probes.empty()) return {};
    const Axis3D Grid { (Position.X - Cascade.Minimum.X) / Cascade.Spacing.X,
        (Position.Y - Cascade.Minimum.Y) / Cascade.Spacing.Y, (Position.Z - Cascade.Minimum.Z) / Cascade.Spacing.Z };
    const std::array<float, 3> Coordinate { Grid.X, Grid.Y, Grid.Z };
    std::array<int, 3> Base {};
    std::array<float, 3> Fraction {};
    for (uint32_t K = 0; K < 3; ++K)
    {
        if (Coordinate[K] < -0.5f || Coordinate[K] > Cascade.Counts[K] - 0.5f) return {};
        const float C = std::clamp(Coordinate[K], 0.0f, static_cast<float>(Cascade.Counts[K] - 1));
        Base[K] = std::min(static_cast<int>(std::floor(C)), static_cast<int>(Cascade.Counts[K]) - 2);
        Fraction[K] = C - Base[K];
    }
    const auto Oct = EncodeOctahedral(Normal);
    const float U = Oct.X * 8.0f - 0.5f;
    const float V = Oct.Y * 8.0f - 0.5f;
    LinearRGB Sum {};
    for (uint32_t Corner = 0; Corner < 8; ++Corner)
    {
        const int X = Base[0] + ((Corner & 1) ? 1 : 0);
        const int Y = Base[1] + ((Corner & 2) ? 1 : 0);
        const int Z = Base[2] + ((Corner & 4) ? 1 : 0);
        float Weight = ((Corner & 1) ? Fraction[0] : 1 - Fraction[0]) *
            ((Corner & 2) ? Fraction[1] : 1 - Fraction[1]) * ((Corner & 4) ? Fraction[2] : 1 - Fraction[2]);
        const auto& Probe = Cascade.Probes[(Z * Cascade.Counts[1] + Y) * Cascade.Counts[0] + X];
        const auto Delta = Position + Normal * 0.04f - Probe.Position;
        const float Distance = Mathematics::EvaluateLength(Delta);
        const auto Direction = NormalizeAxis(Delta);
        const auto DepthOct = EncodeOctahedral(Direction);
        const int DX = std::clamp(static_cast<int>(DepthOct.X * 16), 0, 15);
        const int DY = std::clamp(static_cast<int>(DepthOct.Y * 16), 0, 15);
        const auto Moments = Probe.DepthMoments[DY * 16 + DX];
        const float Variance = std::max(0.0001f, Moments.Y - Moments.X * Moments.X);
        const float Excess = std::max(0.0f, Distance - Moments.X - 0.05f);
        const float Visibility = Variance / (Variance + Excess * Excess);
        Weight *= Visibility * Visibility * Visibility;
        const float Wrap = std::max(0.05f, (1.0f - EvaluateDot(Normal, Direction)) * 0.5f);
        Weight *= Wrap * Wrap;
        // Reflect octahedral texture coordinates at the folded square boundary.
        LinearRGB Irradiance {};
        for (int J = 0; J < 2; ++J)
        {
            for (int I = 0; I < 2; ++I)
            {
                int TX = static_cast<int>(std::floor(U)) + I;
                int TY = static_cast<int>(std::floor(V)) + J;
                const float FX = U - std::floor(U);
                const float FY = V - std::floor(V);
                const float Bilinear = (I ? FX : 1-FX) * (J ? FY : 1-FY);
                if (TX < 0) { TX = -TX-1; TY = 7-TY; }
                if (TX > 7) { TX = 15-TX; TY = 7-TY; }
                if (TY < 0) { TY = -TY-1; TX = 7-TX; }
                if (TY > 7) { TY = 15-TY; TX = 7-TX; }
                Irradiance = Irradiance + Probe.IrradianceOverPi[TY * 8 + TX] * Bilinear;
            }
        }
        Sum = Sum + Irradiance * Weight;
        Confidence += Weight;
    }
    return Confidence > 1.0e-7f ? Sum * (1.0f / Confidence) : LinearRGB {};
}

LinearRGB SurfelCascadeGiSequence::EvaluateIrradianceOverPi(Axis3D Position, Axis3D Normal) const noexcept
{
    float InnerWeight = 0, OuterWeight = 0;
    const auto Inner = SampleCascade(Cascades[0], Position, Normal, InnerWeight);
    const auto Outer = SampleCascade(Cascades[1], Position, Normal, OuterWeight);
    if (InnerWeight < 1.0e-6f) return Outer;
    if (OuterWeight < 1.0e-6f) return Inner;
    const float Edge = std::min({ (Position.X + 4.5f) / 1.125f, (4.5f - Position.X) / 1.125f,
        (Position.Y + 3.6f) / 1.2f, (3.6f - Position.Y) / 1.2f });
    const float Blend = std::clamp(Edge, 0.0f, 1.0f);
    return Inner * Blend + Outer * (1.0f - Blend);
}

} // namespace Frontier::GeometricRaster
