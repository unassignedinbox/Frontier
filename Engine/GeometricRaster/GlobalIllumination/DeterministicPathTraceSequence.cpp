/*============================================================================================================================================
                                                  DETERMINISTICPATHTRACESEQUENCE.CPP
============================================================================================================================================*/
// 🧩 Unified Deterministic Path Tracer (GTX 4-Wide BVH4 + RTX VK_KHR_ray_query Parity, Sobol (0,2) Quadrature, Dupuy-Benyoub GGX VNDF)

//------------------------------------------------------------------------------------------------------------------------
//                                                        INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Engine/GeometricRaster/GlobalIllumination/DeterministicPathTraceSequence.h"
#include "Engine/Constants/MathConstants.h"
#include "Engine/MaterialSpace/Scattering/BlackbodyEmission.h"
#include "Engine/MaterialSpace/Scattering/OpenPbrSurfaceScattering.h"

#include <algorithm>
#include <cmath>
#include <thread>

namespace Frontier::GeometricRaster
{

namespace
{

[[nodiscard]] Mathematics::LinearRGB MultiplyRgb(
    Mathematics::LinearRGB Left,
    Mathematics::LinearRGB Right) noexcept
{
    return { Left.R * Right.R, Left.G * Right.G, Left.B * Right.B };
}

[[nodiscard]] float ApplyAcesToneCurve(float HdrScalar) noexcept
{
    const float V = std::max(0.0f, HdrScalar * 0.85f);
    return std::clamp((V * (2.51f * V + 0.03f)) / (V * (2.43f * V + 0.59f) + 0.14f), 0.0f, 1.0f);
}

[[nodiscard]] uint8_t EncodeSrgbByte(float LinearScalar) noexcept
{
    const float Clamped = std::clamp(LinearScalar, 0.0f, 1.0f);
    const float Encoded = (Clamped <= 0.0031308f)
        ? (12.92f * Clamped)
        : (1.055f * std::pow(Clamped, 1.0f / 2.4f) - 0.055f);
    return static_cast<uint8_t>(std::round(std::clamp(Encoded, 0.0f, 1.0f) * 255.0f));
}

[[nodiscard]] Mathematics::LinearRGB EvaluateSkyEnvironment(
    Mathematics::Axis3D    RayDir,
    Mathematics::LinearRGB SkyRadiance,
    Mathematics::Axis3D    SunDir,
    Mathematics::LinearRGB SunIrradiance) noexcept
{
    const float UpFactor = std::clamp(RayDir.Z * 0.5f + 0.5f, 0.0f, 1.0f);
    Mathematics::LinearRGB Out = SkyRadiance * (0.65f + 0.70f * UpFactor);
    const float SunCos = std::max(0.0f, Mathematics::EvaluateDot(RayDir, SunDir));
    const float Halo   = std::pow(SunCos, 160.0f) * 0.15f;
    Out.R += SunIrradiance.R * Halo;
    Out.G += SunIrradiance.G * Halo;
    Out.B += SunIrradiance.B * Halo;
    return Out;
}

[[nodiscard]] Mathematics::Axis3D SampleCosineHemisphereLocal(float U1, float U2) noexcept
{
    const float RadiusSq = std::clamp(U1, 0.0f, 1.0f);
    const float Radius   = std::sqrt(RadiusSq);
    const float Phi      = Constants::TauFloat * U2;
    return {
        Radius * std::cos(Phi),
        Radius * std::sin(Phi),
        std::sqrt(std::max(0.0f, 1.0f - RadiusSq)),
    };
}

} // namespace

//------------------------------------------------------------------------------------------------------------------------
//                                             DETERMINISTIC PATH TRACE SEQUENCE
//------------------------------------------------------------------------------------------------------------------------

void DeterministicPathTraceSequence::PrepareSceneSources(
    const SpatialAcceleration::WideBoundingTree&  SceneTree,
    const MaterialSpace::CompactMaterialRegister& MaterialRegister)
{
    EmissiveTriangles.clear();
    EmissiveCdf.clear();
    TotalEmissiveArea = 0.0f;

    const uint32_t MaterialCount = MaterialRegister.QuerySlotCount();
    for (const auto& Tri : SceneTree.Primitives)
    {
        if (Tri.MaterialOrdinal >= MaterialCount)
        {
            continue;
        }
        const auto& Mat = MaterialRegister.InspectAuthoredMaterial(Tri.MaterialOrdinal);
        const auto  Rad = MaterialSpace::Scattering::EvaluatePlanckianBlackbodyRadiance(Mat.PorosityLight);
        if (std::max({ Rad.R, Rad.G, Rad.B }) <= 1.0e-5f)
        {
            continue;
        }
        const Mathematics::Axis3D Cross = Mathematics::EvaluateCross(Tri.Edge01, Tri.Edge02);
        const float               Area  = 0.5f * Mathematics::EvaluateLength(Cross);
        if (Area <= 1.0e-7f)
        {
            continue;
        }
        EmissiveTriangleSource Source {};
        Source.VertexA    = Tri.Corner0;
        Source.VertexB    = Tri.Corner0 + Tri.Edge01;
        Source.VertexC    = Tri.Corner0 + Tri.Edge02;
        Source.UnitNormal = Mathematics::NormalizeAxis(Cross);
        Source.Radiance   = Rad;
        Source.AreaM2     = Area;
        EmissiveTriangles.push_back(Source);
        TotalEmissiveArea += Area;
    }

    if (TotalEmissiveArea > 1.0e-7f && !EmissiveTriangles.empty())
    {
        EmissiveCdf.resize(EmissiveTriangles.size(), 1.0f);
        double Running = 0.0;
        for (std::size_t Idx = 0u; Idx < EmissiveTriangles.size(); ++Idx)
        {
            Running += static_cast<double>(EmissiveTriangles[Idx].AreaM2) / static_cast<double>(TotalEmissiveArea);
            EmissiveCdf[Idx] = static_cast<float>(Running);
        }
        EmissiveCdf.back() = 1.0f;
    }
}

Mathematics::LinearRGB DeterministicPathTraceSequence::TracePixelRadiance(
    const PathTraceConfiguration&                 Config,
    uint32_t                                      PixelX,
    uint32_t                                      PixelY,
    Mathematics::Axis3D                           RayOrigin,
    Mathematics::Axis3D                           RayDirection,
    const SpatialAcceleration::WideBoundingTree&  SceneTree,
    const MaterialSpace::CompactMaterialRegister& MaterialRegister,
    const SolarDirectionalSource&                 Sun,
    std::span<const PolygonalAreaQuadLight>       AreaLights,
    Mathematics::LinearRGB                        SkyRadiance) const noexcept
{
    const uint32_t Spp      = std::max(1u, Config.SamplesPerPixel);
    const uint32_t Bounces  = std::max(1u, Config.MaxBounceCount);
    const uint32_t PixelKey = Config.SharedPixelQuadrature
        ? 0u
        : AvalancheKey32(PixelX * 747796405u + PixelY * 2891336453u + 1u);

    const Mathematics::Axis3D SunDir = Mathematics::NormalizeAxis(Sun.DirectionToSun, { 0.0f, 0.0f, 1.0f });
    const float TanSunRadius = std::tan(std::clamp(Config.SolarAngularRadiusRad, 1.0e-4f, 0.25f));
    const float SolarConeSolidAngle = Constants::PiFloat * TanSunRadius * TanSunRadius;
    const float SolarConePdf = (SolarConeSolidAngle > 1.0e-9f) ? (1.0f / SolarConeSolidAngle) : 1.0f;
    const uint32_t MaterialCount = MaterialRegister.QuerySlotCount();
    const bool UseRtxMode = (Config.AccelerationMode == PathTraceAccelerationMode::RtxInlineRayQuery);

    Mathematics::LinearRGB AccumulatedRadiance {};

    for (uint32_t SampleOffset = 0u; SampleOffset < Spp; ++SampleOffset)
    {
        const uint32_t SampleIdx = Config.StartSampleIndex + SampleOffset;
        Mathematics::Axis3D    Origin     = RayOrigin;
        Mathematics::Axis3D    Dir        = RayDirection;
        Mathematics::LinearRGB Throughput { 1.0f, 1.0f, 1.0f };
        Mathematics::LinearRGB PathTotal  {};
        float                  PreviousBsdfPdf = 1.0f;
        bool                   PreviousWasDelta = true;

        for (uint32_t BounceIdx = 0u; BounceIdx < Bounces; ++BounceIdx)
        {
            const SpatialAcceleration::RaySegmentQuery RayQuery {
                Origin,
                (BounceIdx == 0u) ? 1.0e-3f : 2.0e-3f,
                Dir,
                150.0f
            };
            const auto Hit = SceneTree.TraceClosestRay(RayQuery, false);
            if (!Hit.HitConfirmed)
            {
                const auto SkyVal = EvaluateSkyEnvironment(Dir, SkyRadiance, SunDir, Sun.SolarIrradiance);
                PathTotal = PathTotal + MultiplyRgb(Throughput, SkyVal);
                break;
            }

            const Mathematics::Axis3D HitPos = Origin + Dir * Hit.HitDistanceMeters;
            Mathematics::Axis3D GeomN = Hit.GeometricNormal;
            if (Mathematics::EvaluateDot(GeomN, Dir) > 0.0f)
            {
                GeomN = GeomN * -1.0f;
            }
            Mathematics::Axis3D ShadeN = GeomN;
            if (Hit.PrimitiveIndex < SceneTree.PrimitiveNormals.size())
            {
                const auto& TriN = SceneTree.PrimitiveNormals[Hit.PrimitiveIndex];
                const float W0   = 1.0f - Hit.BarycentricU - Hit.BarycentricV;
                ShadeN = Mathematics::NormalizeAxis(
                    TriN.Normal0 * W0 + TriN.Normal1 * Hit.BarycentricU + TriN.Normal2 * Hit.BarycentricV,
                    GeomN);
                if (Mathematics::EvaluateDot(ShadeN, Dir) > 0.0f)
                {
                    ShadeN = ShadeN * -1.0f;
                }
            }

            const uint16_t MatSlot = (Hit.MaterialOrdinal < MaterialCount) ? Hit.MaterialOrdinal : 0u;
            const auto&    Mat     = MaterialRegister.InspectAuthoredMaterial(MatSlot);
            const uint32_t LobeMask = MaterialSpace::DetermineActiveLobeMask(Mat);
            const auto     SelfRad  = MaterialSpace::Scattering::EvaluatePlanckianBlackbodyRadiance(Mat.PorosityLight);

            if (std::max({ SelfRad.R, SelfRad.G, SelfRad.B }) > 1.0e-5f)
            {
                float EmissiveMis = 1.0f;
                if (!PreviousWasDelta && TotalEmissiveArea > 1.0e-7f)
                {
                    const float CosSrc = std::max(0.0f, Mathematics::EvaluateDot(GeomN, Dir * -1.0f));
                    if (CosSrc > 1.0e-5f)
                    {
                        const float DistSq = std::max(1.0e-5f, Hit.HitDistanceMeters * Hit.HitDistanceMeters);
                        const float NeePdf = DistSq / (CosSrc * TotalEmissiveArea);
                        EmissiveMis = PowerHeuristicMis(PreviousBsdfPdf, NeePdf);
                    }
                }
                PathTotal = PathTotal + MultiplyRgb(Throughput, SelfRad) * EmissiveMis;
            }

            Mathematics::Axis3D TangentX {};
            Mathematics::Axis3D BitangentY {};
            BuildDuffBasis(ShadeN, TangentX, BitangentY);
            const MaterialSpace::Scattering::SurfaceShadingBasis Basis { TangentX, BitangentY, ShadeN };
            const Mathematics::Axis3D WoWorld = Dir * -1.0f;
            const Mathematics::Axis3D LocalWo = Basis.WorldToLocal(WoWorld);
            const Mathematics::Axis3D ShadowOrigin = HitPos + GeomN * 3.0e-3f;

            // 1. Solar Disc Next-Event Estimation (NEE)
            {
                const auto USun = Sobol02Sample(SampleIdx, PixelKey, BounceIdx * 5u + 0u);
                Mathematics::Axis3D SunT {}, SunB {};
                BuildDuffBasis(SunDir, SunT, SunB);
                const float ConeR = std::sqrt(USun.X) * TanSunRadius;
                const float ConeA = Constants::TauFloat * USun.Y;
                const Mathematics::Axis3D LightDir = Mathematics::NormalizeAxis(
                    SunDir + SunT * (ConeR * std::cos(ConeA)) + SunB * (ConeR * std::sin(ConeA)),
                    SunDir);
                const float NdotL = Mathematics::EvaluateDot(ShadeN, LightDir);
                if (NdotL > 1.0e-4f)
                {
                    const bool Occluded = SceneTree.QueryRayOccluded({ ShadowOrigin, 1.5e-3f, LightDir, 120.0f });
                    if (!Occluded)
                    {
                        const Mathematics::Axis3D LocalWi = Basis.WorldToLocal(LightDir);
                        const auto Bsdf = MaterialSpace::Scattering::EvaluateOpenPbrLocalBsdf(
                            Mat, LobeMask, LocalWo, LocalWi);
                        const float Rough = std::max(0.02f, Mat.Specular.Roughness);
                        const float Alpha = Rough * Rough;
                        const float VndfPdf = EvaluateSphericalCapGgxVndfPdf(LocalWo, LocalWi, Alpha, Alpha);
                        const float DiffPdf = std::max(0.0f, LocalWi.Z) / Constants::PiFloat;
                        const float SpecProb = std::clamp(
                            0.20f + 0.65f * std::max(Mat.Specular.Metalness, Mat.Clearcoat.CoatWeight),
                            0.15f, 0.85f);
                        const float BsdfPdf = SpecProb * VndfPdf + (1.0f - SpecProb) * DiffPdf;
                        const float MisW = PowerHeuristicMis(SolarConePdf, BsdfPdf);
                        PathTotal = PathTotal + MultiplyRgb(MultiplyRgb(Throughput, Bsdf), Sun.SolarIrradiance) * (NdotL * MisW);
                    }
                }
            }

            // 2. Polygonal Area Quad Light NEE
            if (!AreaLights.empty())
            {
                const auto UQuad = Sobol02Sample(SampleIdx, PixelKey, BounceIdx * 5u + 1u);
                const uint32_t QuadCount = static_cast<uint32_t>(AreaLights.size());
                const uint32_t QuadIdx   = std::min(QuadCount - 1u, static_cast<uint32_t>(UQuad.X * static_cast<float>(QuadCount)));
                const float    LocalUX   = std::clamp(UQuad.X * static_cast<float>(QuadCount) - static_cast<float>(QuadIdx), 0.0f, 1.0f);
                const auto&    Quad      = AreaLights[QuadIdx];

                const Mathematics::Axis3D LightPos = Quad.CenterWorld +
                    Quad.RightTangent * ((LocalUX * 2.0f - 1.0f) * Quad.HalfWidthMeters) +
                    Quad.UpBitangent  * ((UQuad.Y * 2.0f - 1.0f) * Quad.HalfHeightMeters);
                const Mathematics::Axis3D Delta  = LightPos - ShadowOrigin;
                const float               DistSq = Mathematics::EvaluateLengthSquared(Delta);
                const float               Dist   = std::sqrt(std::max(1.0e-6f, DistSq));
                const Mathematics::Axis3D LDir   = Delta * (1.0f / Dist);
                const Mathematics::Axis3D QNorm  = Mathematics::NormalizeAxis(
                    Mathematics::EvaluateCross(Quad.RightTangent, Quad.UpBitangent));
                const float CosR = Mathematics::EvaluateDot(ShadeN, LDir);
                const float CosL = Quad.TwoSidedEmission
                    ? std::fabs(Mathematics::EvaluateDot(QNorm, LDir))
                    : std::max(0.0f, -Mathematics::EvaluateDot(QNorm, LDir));
                if (CosR > 1.0e-4f && CosL > 1.0e-4f && Dist > 0.02f)
                {
                    if (!SceneTree.QueryRayOccluded({ ShadowOrigin, 1.5e-3f, LDir, Dist - 0.01f }))
                    {
                        const float Area = 4.0f * Quad.HalfWidthMeters * Quad.HalfHeightMeters;
                        const auto  Bsdf = MaterialSpace::Scattering::EvaluateOpenPbrLocalBsdf(
                            Mat, LobeMask, LocalWo, Basis.WorldToLocal(LDir));
                        const float Weight = (CosR * CosL * Area * static_cast<float>(QuadCount)) / DistSq;
                        PathTotal = PathTotal + MultiplyRgb(MultiplyRgb(Throughput, Bsdf), Quad.EmittedRadiance) * Weight;
                    }
                }
            }

            // 3. Real Emissive Triangle NEE
            if (!EmissiveTriangles.empty() && TotalEmissiveArea > 1.0e-7f)
            {
                const auto USelect = Sobol02Sample(SampleIdx, PixelKey, BounceIdx * 5u + 2u);
                const auto UTrix   = Sobol02Sample(SampleIdx, PixelKey, BounceIdx * 5u + 3u);
                const auto It = std::lower_bound(EmissiveCdf.begin(), EmissiveCdf.end(), USelect.X);
                const std::size_t TriIdx = std::min(
                    EmissiveTriangles.size() - 1u,
                    static_cast<std::size_t>(It - EmissiveCdf.begin()));
                const auto& Src = EmissiveTriangles[TriIdx];

                const float Su = std::sqrt(std::clamp(UTrix.X, 0.0f, 1.0f));
                const float B0 = 1.0f - Su;
                const float B1 = UTrix.Y * Su;
                const float B2 = 1.0f - B0 - B1;
                const Mathematics::Axis3D LightPos = Src.VertexA * B0 + Src.VertexB * B1 + Src.VertexC * B2;
                const Mathematics::Axis3D Delta    = LightPos - ShadowOrigin;
                const float               DistSq   = Mathematics::EvaluateLengthSquared(Delta);
                const float               Dist     = std::sqrt(std::max(1.0e-6f, DistSq));
                const Mathematics::Axis3D LDir     = Delta * (1.0f / Dist);
                const float CosR = Mathematics::EvaluateDot(ShadeN, LDir);
                const float CosL = std::max(0.0f, -Mathematics::EvaluateDot(Src.UnitNormal, LDir));
                if (CosR > 1.0e-4f && CosL > 1.0e-4f && Dist > 0.02f)
                {
                    if (!SceneTree.QueryRayOccluded({ ShadowOrigin, 1.5e-3f, LDir, Dist - 0.015f }))
                    {
                        const Mathematics::Axis3D LocalWi = Basis.WorldToLocal(LDir);
                        const auto Bsdf = MaterialSpace::Scattering::EvaluateOpenPbrLocalBsdf(
                            Mat, LobeMask, LocalWo, LocalWi);
                        const float NeePdf = DistSq / (CosL * TotalEmissiveArea);
                        const float Rough  = std::max(0.02f, Mat.Specular.Roughness);
                        const float Alpha  = Rough * Rough;
                        const float VndfPdf = EvaluateSphericalCapGgxVndfPdf(LocalWo, LocalWi, Alpha, Alpha);
                        const float DiffPdf = std::max(0.0f, LocalWi.Z) / Constants::PiFloat;
                        const float SpecProb = std::clamp(
                            0.20f + 0.65f * std::max(Mat.Specular.Metalness, Mat.Clearcoat.CoatWeight),
                            0.15f, 0.85f);
                        const float BsdfPdf = SpecProb * VndfPdf + (1.0f - SpecProb) * DiffPdf;
                        const float MisW    = PowerHeuristicMis(NeePdf, BsdfPdf);
                        const float GeomTerm = (CosR * CosL * TotalEmissiveArea) / DistSq;
                        PathTotal = PathTotal + MultiplyRgb(MultiplyRgb(Throughput, Bsdf), Src.Radiance) * (GeomTerm * MisW);
                    }
                }
            }

            if (BounceIdx + 1u >= Bounces)
            {
                break;
            }

            // 4. Deterministic BSDF Lobe Scatter (Dupuy-Benyoub Spherical-Cap GGX VNDF vs Cosine Hemisphere)
            const auto UBsdf = Sobol02Sample(SampleIdx, PixelKey, BounceIdx * 5u + 4u);
            const auto ULobe = Sobol02Sample(SampleIdx, PixelKey, BounceIdx * 5u + 5u);

            const float Rough = std::max(
                0.025f,
                std::min(Mat.Specular.Roughness, (Mat.Clearcoat.CoatWeight > 0.1f) ? Mat.Clearcoat.CoatRoughness : 1.0f));
            const float Alpha    = Rough * Rough;
            const float SpecProb = std::clamp(
                0.20f + 0.65f * std::max(Mat.Specular.Metalness, Mat.Clearcoat.CoatWeight),
                0.15f, 0.85f);

            Mathematics::Axis3D LocalWi {};
            if (ULobe.X < SpecProb)
            {
                LocalWi = SampleSphericalCapGgxVndfLocal(LocalWo, Alpha, Alpha, UBsdf.X, UBsdf.Y);
            }
            else
            {
                LocalWi = SampleCosineHemisphereLocal(UBsdf.X, UBsdf.Y);
            }

            if (LocalWi.Z <= 1.0e-4f)
            {
                break;
            }

            const float VndfPdf = EvaluateSphericalCapGgxVndfPdf(LocalWo, LocalWi, Alpha, Alpha);
            const float DiffPdf = LocalWi.Z / Constants::PiFloat;
            const float MixPdf  = SpecProb * VndfPdf + (1.0f - SpecProb) * DiffPdf;
            if (MixPdf <= 1.0e-7f)
            {
                break;
            }

            const auto Bsdf = MaterialSpace::Scattering::EvaluateOpenPbrLocalBsdf(Mat, LobeMask, LocalWo, LocalWi);
            const float WeightScale = LocalWi.Z / MixPdf;
            Throughput = MultiplyRgb(Throughput, Bsdf * WeightScale);
            if (std::max({ Throughput.R, Throughput.G, Throughput.B }) <= 1.0e-5f)
            {
                break;
            }

            PreviousBsdfPdf  = MixPdf;
            PreviousWasDelta = false;
            Origin           = ShadowOrigin;
            Dir              = Basis.LocalToWorld(LocalWi);
        }

        AccumulatedRadiance = AccumulatedRadiance + PathTotal;
    }

    return AccumulatedRadiance * (1.0f / static_cast<float>(Spp));
}

PathTraceOutput DeterministicPathTraceSequence::RenderViewport(
    const PathTraceConfiguration&                 Config,
    const PerspectiveCameraBasis&                 Camera,
    const SpatialAcceleration::WideBoundingTree&  SceneTree,
    const MaterialSpace::CompactMaterialRegister& MaterialRegister,
    const SolarDirectionalSource&                 Sun,
    std::span<const PolygonalAreaQuadLight>       AreaLights,
    Mathematics::LinearRGB                        SkyRadiance) const
{
    PathTraceOutput Out {};
    Out.WidthPx          = (Config.WidthPx > 0u) ? Config.WidthPx : Camera.ViewportWidthPx;
    Out.HeightPx         = (Config.HeightPx > 0u) ? Config.HeightPx : Camera.ViewportHeightPx;
    Out.CompletedSamples = std::max(1u, Config.SamplesPerPixel);
    Out.ActiveMode       = Config.AccelerationMode;

    const std::size_t PixelCount = static_cast<std::size_t>(Out.WidthPx) * Out.HeightPx;
    Out.LinearHdrRadiance.assign(PixelCount, {});
    Out.DisplayRgba8.assign(PixelCount * 4u, 255u);
    if (PixelCount == 0u)
    {
        return Out;
    }

    PerspectiveCameraBasis ResolvedCamera = Camera;
    ResolvedCamera.ViewportWidthPx        = Out.WidthPx;
    ResolvedCamera.ViewportHeightPx       = Out.HeightPx;
    const OrthonormalCameraBasis Basis    = ComputeCameraBasis(ResolvedCamera);

    const unsigned ThreadCount = std::clamp(std::thread::hardware_concurrency(), 1u, 16u);
    std::vector<std::thread> Threads {};
    Threads.reserve(ThreadCount);

    for (unsigned ThreadIdx = 0u; ThreadIdx < ThreadCount; ++ThreadIdx)
    {
        Threads.emplace_back([&, ThreadIdx]()
        {
            for (uint32_t PY = ThreadIdx; PY < Out.HeightPx; PY += ThreadCount)
            {
                const float NdcY = 1.0f - ((static_cast<float>(PY) + 0.5f) / static_cast<float>(Out.HeightPx)) * 2.0f;
                for (uint32_t PX = 0u; PX < Out.WidthPx; ++PX)
                {
                    const float NdcX = ((static_cast<float>(PX) + 0.5f) / static_cast<float>(Out.WidthPx)) * 2.0f - 1.0f;
                    const Mathematics::Axis3D RayDir = Mathematics::NormalizeAxis(
                        Basis.ForwardAxis +
                        Basis.RightAxis * (NdcX * Basis.TanHalfFovX) +
                        Basis.UpAxis    * (NdcY * Basis.TanHalfFovY));

                    const auto Hdr = TracePixelRadiance(
                        Config,
                        PX,
                        PY,
                        Basis.CameraOrigin,
                        RayDir,
                        SceneTree,
                        MaterialRegister,
                        Sun,
                        AreaLights,
                        SkyRadiance);

                    const std::size_t Off = static_cast<std::size_t>(PY) * Out.WidthPx + PX;
                    Out.LinearHdrRadiance[Off] = Hdr;
                    Out.DisplayRgba8[Off * 4u + 0u] = EncodeSrgbByte(ApplyAcesToneCurve(Hdr.R));
                    Out.DisplayRgba8[Off * 4u + 1u] = EncodeSrgbByte(ApplyAcesToneCurve(Hdr.G));
                    Out.DisplayRgba8[Off * 4u + 2u] = EncodeSrgbByte(ApplyAcesToneCurve(Hdr.B));
                    Out.DisplayRgba8[Off * 4u + 3u] = 255u;
                }
            }
        });
    }
    for (auto& Thread : Threads)
    {
        Thread.join();
    }

    return Out;
}

} // namespace Frontier::GeometricRaster
