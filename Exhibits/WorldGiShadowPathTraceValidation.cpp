/*============================================================================================================================================
                                                   WORLDGISHADOWPATHTRACEVALIDATION.CPP
============================================================================================================================================*/
// 🧪 Real-Mesh Validation: 2 GTX GI Renderers (Stokes Contour + Surfel Cascade), 6 Shadow Modes & GTX/RTX Deterministic Path Tracer

#include "Engine/GeometricRaster/DirectLighting/SolarShadowSequence.h"
#include "Engine/GeometricRaster/DirectLighting/SurfelOccluderField.h"
#include "Engine/GeometricRaster/GlobalIllumination/DeterministicPathTraceSequence.h"
#include "Engine/GeometricRaster/GlobalIllumination/StokesContourGiSequence.h"
#include "Engine/GeometricRaster/GlobalIllumination/SurfelCascadeGiSequence.h"
#include "Engine/GeometricRaster/GlobalIllumination/TieredWorldRenderSequence.h"
#include "Engine/SpatialAcceleration/ClusterPyramid.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

using namespace Frontier;
using namespace Frontier::GeometricRaster;
using Mathematics::Axis3D;

int main()
{
    unsigned Passed = 0u;
    const auto Check = [&](bool Good, const char* Label)
    {
        std::printf("[%s] %s\n", Good ? "PASS" : "FAIL", Label);
        if (!Good) std::exit(1);
        ++Passed;
    };

    //--------------------------------------------------------------------------------------------------------------------
    // 1. STOKES CONTOUR GI MATHEMATICAL INVARIANTS
    //--------------------------------------------------------------------------------------------------------------------
    const Axis3D P {};
    const Axis3D N { 0.0f, 0.0f, 1.0f };
    const Axis3D A { -1.0f, -1.0f, 2.0f }, B { 1.0f, -1.0f, 2.0f }, C { 0.0f, 1.0f, 2.0f };
    const float Form = StokesContourGiSequence::EvaluateTriangleFormFactor(P, N, A, B, C);
    Check(Form > 0.0f && Form < 1.0f, "bounded positive triangle form factor");
    Check(std::abs(Form - StokesContourGiSequence::EvaluateTriangleFormFactor(P, N, C, B, A)) < 1.0e-6f,
        "winding-independent receiver integral");
    Check(StokesContourGiSequence::EvaluateTriangleFormFactor(P, { 0.0f, 0.0f, -1.0f }, A, B, C) == 0.0f,
        "triangle below receiver horizon rejected");
    const Axis3D Shift { 8.0f, -4.0f, 7.0f };
    Check(std::abs(Form - StokesContourGiSequence::EvaluateTriangleFormFactor(Shift, N, A + Shift, B + Shift, C + Shift)) < 1.0e-6f,
        "translation invariance");
    Check(std::abs(Form - StokesContourGiSequence::EvaluateTriangleFormFactor(P, N, A * 3.0f, B * 3.0f, C * 3.0f)) < 1.0e-6f,
        "solid angle scale invariance");

    double Integral = 0.0;
    constexpr unsigned Count = 400u;
    for (unsigned I = 0u; I < Count; ++I)
    {
        for (unsigned J = 0u; J < Count; ++J)
        {
            const float U = std::sqrt((I + 0.5f) / Count);
            const float V = (J + 0.5f) / Count;
            const auto  Q = A * (1.0f - U) + B * (U * (1.0f - V)) + C * (U * V);
            const float R2 = Mathematics::EvaluateLengthSquared(Q);
            Integral += 4.0 / (3.141592653589793 * R2 * R2) * 2.0 / (Count * Count);
        }
    }
    Check(std::abs(Form - Integral) < 0.0001, "Stokes versus independent area quadrature");
    const float Clipped = StokesContourGiSequence::EvaluateTriangleFormFactor(
        P, N, { -1.0f, -1.0f, -1.0f }, { 1.0f, -1.0f, 1.0f }, { 0.0f, 1.0f, 1.0f });
    Check(std::isfinite(Clipped) && Clipped > 0.0f && Clipped <= 1.0f, "horizon crossing including entering-edge denominator");
    Check(StokesContourGiSequence::EvaluateTriangleFormFactor(P, N, A, A, A) == 0.0f, "degenerate triangle finite black");

    //--------------------------------------------------------------------------------------------------------------------
    // 2. OCTAHEDRAL SURFEL CASCADE INVARIANTS
    //--------------------------------------------------------------------------------------------------------------------
    bool Roundtrip = true;
    for (unsigned I = 0u; I < 32u; ++I)
    {
        for (unsigned J = 0u; J < 32u; ++J)
        {
            const auto Direction = SurfelCascadeGiSequence::DecodeOctahedral((I + 0.5f) / 32.0f, (J + 0.5f) / 32.0f);
            const auto UV        = SurfelCascadeGiSequence::EncodeOctahedral(Direction);
            const auto Decoded   = SurfelCascadeGiSequence::DecodeOctahedral(UV.X, UV.Y);
            Roundtrip &= Mathematics::EvaluateDot(Direction, Decoded) > 0.99999f;
        }
    }
    Check(Roundtrip, "1024 octahedral direction round trips");

    SpatialAcceleration::WideBoundingTree SingleTree;
    SpatialAcceleration::RayTrianglePrimitive SingleTri;
    SingleTri.Corner0         = A;
    SingleTri.Edge01          = C - A;
    SingleTri.Edge02          = B - A;
    SingleTri.MaterialOrdinal = 0u;
    Check(SingleTree.BuildFromTrianglePrimitives({ &SingleTri, 1u }), "real triangle BVH build");

    const Mathematics::LinearRGB Emission { 2.0f, 1.0f, 0.5f }, Reflectance { 0.5f, 0.5f, 0.5f };
    StokesContourGiSequence Stokes;
    Stokes.Prepare(SingleTree, { &Emission, 1u }, { &Reflectance, 1u }, N, {});
    const auto Irradiance = Stokes.EvaluateIrradianceOverPi(P, N);
    Check(Stokes.Sources.size() == 1u && std::abs(Irradiance.R - Form * 2.0f) < 1.0e-5f,
        "emissive real triangle extraction and visible Stokes radiance");
    Check(std::abs(Irradiance.G - Form) < 1.0e-5f, "coloured source radiance");

    SurfelCascadeGiSequence Surfel;
    Surfel.Prepare(Stokes, SingleTree);
    Check(Surfel.Cascades[0].Probes.size() == 252u && Surfel.Cascades[1].Probes.size() == 140u,
        "two populated octahedral irradiance cascades");
    const auto Cached = Surfel.EvaluateIrradianceOverPi(P, N);
    Check(std::isfinite(Cached.R) && Cached.R > 0.0f, "surfel cascade interpolation yields finite irradiance");
    Check(Surfel.EvaluateIrradianceOverPi({ 1000.0f, 0.0f, 0.0f }, N).R == 0.0f, "outside cascade finite black");

    SurfelOccluderField Field;
    Field.BuildFromTriangles(SingleTree);
    Check(!Field.Disks.empty(), "occluder disks sampled from triangles");
    Check(Field.EvaluateSolarVisibility(SingleTree, { 4.0f, 0.0f, 0.0f }, N, N, 0.016f) > 0.99f, "open solar visibility");
    const auto Blocked = Field.EvaluateSolarVisibility(SingleTree, P, N, N, 0.016f);
    Check(Blocked < 0.1f, "triangle-derived distant solar blocker");
    Check(Field.EvaluateSolarVisibility(SingleTree, { 0.0f, 0.0f, 1.8f }, N, N, 0.016f) == 0.0f,
        "exact BVH triangle contact shadow");

    SpatialAcceleration::WideBoundingTree Empty;
    Stokes.Prepare(Empty, { &Emission, 1u }, { &Reflectance, 1u }, N, {});
    Check(Stokes.Sources.empty() && Stokes.EvaluateIrradianceOverPi(P, N).R == 0.0f, "rebuild removes stale triangle sources");

    //--------------------------------------------------------------------------------------------------------------------
    // 3. REAL TRIANGLE SCENE: 6 SHADOW ALGORITHMS + 2 GTX GI RENDERERS + UNIFIED GTX/RTX PATH TRACER
    //--------------------------------------------------------------------------------------------------------------------
    WorldSceneSpecification Scene {};
    MaterialSpace::SurfaceMaterial FloorMat {};
    FloorMat.MaterialName       = "StudioFloor";
    FloorMat.Diffuse.BaseColor  = { 0.65f, 0.63f, 0.60f };
    FloorMat.Specular.Roughness = 0.45f;
    const uint16_t SlotFloor = Scene.MaterialRegister.RegisterMaterial(FloorMat);

    MaterialSpace::SurfaceMaterial BlockerMat {};
    BlockerMat.MaterialName       = "BlockerPillar";
    BlockerMat.Diffuse.BaseColor  = { 0.78f, 0.18f, 0.14f };
    BlockerMat.Specular.Roughness = 0.35f;
    const uint16_t SlotBlocker = Scene.MaterialRegister.RegisterMaterial(BlockerMat);

    MaterialSpace::SurfaceMaterial EmissiveMat {};
    EmissiveMat.MaterialName                          = "StudioEmissivePanel";
    EmissiveMat.Diffuse.BaseColor                     = { 1.0f, 0.92f, 0.80f };
    EmissiveMat.PorosityLight.EmissionLuminanceNits   = 18000.0f;
    EmissiveMat.PorosityLight.EmissionTint            = { 1.0f, 0.85f, 0.65f };
    const uint16_t SlotEmissive = Scene.MaterialRegister.RegisterMaterial(EmissiveMat);

    const auto AppendBoxTriangles = [](
        std::vector<SpatialAcceleration::RayTrianglePrimitive>& OutTris,
        Axis3D Center,
        Axis3D HalfExtents,
        uint16_t MatSlot)
    {
        const float X0 = Center.X - HalfExtents.X, X1 = Center.X + HalfExtents.X;
        const float Y0 = Center.Y - HalfExtents.Y, Y1 = Center.Y + HalfExtents.Y;
        const float Z0 = Center.Z - HalfExtents.Z, Z1 = Center.Z + HalfExtents.Z;
        const Axis3D V[8] {
            { X0, Y0, Z0 }, { X1, Y0, Z0 }, { X1, Y1, Z0 }, { X0, Y1, Z0 },
            { X0, Y0, Z1 }, { X1, Y0, Z1 }, { X1, Y1, Z1 }, { X0, Y1, Z1 },
        };
        const uint32_t Quads[6][4] {
            { 0u, 3u, 2u, 1u }, { 4u, 5u, 6u, 7u }, { 0u, 1u, 5u, 4u },
            { 2u, 3u, 7u, 6u }, { 1u, 2u, 6u, 5u }, { 3u, 0u, 4u, 7u },
        };
        for (const auto& Q : Quads)
        {
            const uint32_t Idx[6] { Q[0], Q[1], Q[2], Q[0], Q[2], Q[3] };
            for (int T = 0; T < 2; ++T)
            {
                const Axis3D P0 = V[Idx[T * 3 + 0]];
                const Axis3D P1 = V[Idx[T * 3 + 1]];
                const Axis3D P2 = V[Idx[T * 3 + 2]];
                SpatialAcceleration::RayTrianglePrimitive Tri {};
                Tri.Corner0         = P0;
                Tri.Edge01          = P1 - P0;
                Tri.Edge02          = P2 - P0;
                Tri.MaterialOrdinal = MatSlot;
                Tri.SourceFaceIndex = static_cast<uint32_t>(OutTris.size());
                OutTris.push_back(Tri);
            }
        }
    };

    std::vector<SpatialAcceleration::RayTrianglePrimitive> SceneTris {};
    AppendBoxTriangles(SceneTris, { 0.0f, 0.0f, -0.10f }, { 4.5f, 4.5f, 0.10f }, SlotFloor);
    AppendBoxTriangles(SceneTris, { -0.8f, 0.2f, 0.85f }, { 0.40f, 0.40f, 0.85f }, SlotBlocker);
    AppendBoxTriangles(SceneTris, { 1.4f, 0.8f, 1.25f }, { 0.12f, 0.55f, 0.55f }, SlotEmissive);
    Check(Scene.SceneTree.BuildFromTrianglePrimitives(SceneTris), "real triangulated studio BVH4 build");

    Scene.Sun.DirectionToSun          = Mathematics::NormalizeAxis({ -0.45f, -0.52f, 0.725f });
    Scene.Sun.SolarIrradiance         = { 4.2f, 3.98f, 3.65f };
    Scene.Sun.SunAngularRadiusRadians = 0.028f;
    Scene.Sun.RtxSamplesPerPixel      = 8u;

    std::vector<Mathematics::LinearRGB> Emissions(Scene.MaterialRegister.QuerySlotCount());
    std::vector<Mathematics::LinearRGB> Reflectances(Scene.MaterialRegister.QuerySlotCount());
    for (uint32_t M = 0u; M < Scene.MaterialRegister.QuerySlotCount(); ++M)
    {
        const auto& Mat = Scene.MaterialRegister.InspectAuthoredMaterial(M);
        Emissions[M]    = MaterialSpace::Scattering::EvaluatePlanckianBlackbodyRadiance(Mat.PorosityLight);
        Reflectances[M] = Mat.Diffuse.BaseColor;
    }

    StokesContourGiSequence StokesScene {};
    StokesScene.Prepare(Scene.SceneTree, Emissions, Reflectances, Scene.Sun.DirectionToSun, Scene.Sun.SolarIrradiance);
    SurfelCascadeGiSequence SurfelScene {};
    SurfelScene.Prepare(StokesScene, Scene.SceneTree);
    SurfelOccluderField SurfelFieldScene {};
    SurfelFieldScene.BuildFromTriangles(Scene.SceneTree);

    OrthographicShadowCascade FineClip {}, CoarseClip {};
    SolarShadowSequence::BuildTwoCascadeContactClips(Scene.Sun, { 0.0f, 0.0f, 0.5f }, Scene.SceneTree, FineClip, CoarseClip);

    // 3a. Verify all 6 SolarShadowAlgorithm modes on real triangle surface samples
    constexpr uint32_t TestW = 48u;
    constexpr uint32_t TestH = 36u;
    std::vector<AnalyticResolvedSurfaceSample> Samples(TestW * TestH);
    for (uint32_t Y = 0u; Y < TestH; ++Y)
    {
        for (uint32_t X = 0u; X < TestW; ++X)
        {
            auto& S = Samples[Y * TestW + X];
            S.ValidHit             = true;
            S.MaterialSlot         = SlotFloor;
            S.WorldPosition        = { -2.2f + 4.4f * (float(X) / float(TestW - 1u)),
                                       -2.0f + 4.0f * (float(Y) / float(TestH - 1u)),
                                        0.0f };
            S.TangentFrame.NormalZ = { 0.0f, 0.0f, 1.0f };
        }
    }

    const std::array<SolarShadowAlgorithm, 6u> AllShadowModes {{
        SolarShadowAlgorithm::ContactPcss,
        SolarShadowAlgorithm::PcssPlaneAware,
        SolarShadowAlgorithm::ShadowRayMarch,
        SolarShadowAlgorithm::PcfBilinear,
        SolarShadowAlgorithm::WideTreeRayQuery,
        SolarShadowAlgorithm::SurfelTriangleField,
    }};

    for (std::size_t AlgIdx = 0u; AlgIdx < AllShadowModes.size(); ++AlgIdx)
    {
        const auto ShadowOut = SolarShadowSequence::EvaluateSolarShadowFieldWithAlgorithm(
            AllShadowModes[AlgIdx],
            Scene.Sun,
            TestW,
            TestH,
            Samples,
            Scene.SceneTree,
            &FineClip,
            &CoarseClip,
            &SurfelFieldScene);

        float MinVis = 1.0f;
        float MaxVis = 0.0f;
        bool  AllFinite = true;
        for (float V : ShadowOut.FilteredVisibility)
        {
            AllFinite &= std::isfinite(V);
            MinVis = std::min(MinVis, V);
            MaxVis = std::max(MaxVis, V);
        }
        Check(AllFinite && MinVis < 0.25f && MaxVis > 0.90f,
            "all 6 SolarShadowAlgorithm modes produce valid cast umbra & sunlit floor");
    }

    // 3b. Verify both GTX GI Renderers (StokesContourGi & SurfelCascadeGi) on shaded floor samples
    double SumStokesIrrad = 0.0;
    double SumSurfelIrrad = 0.0;
    bool   AllGiFinite    = true;
    for (const auto& S : Samples)
    {
        const auto E1 = StokesScene.EvaluateIrradianceOverPi(S.WorldPosition, S.TangentFrame.NormalZ);
        const auto E2 = SurfelScene.EvaluateIrradianceOverPi(S.WorldPosition, S.TangentFrame.NormalZ);
        AllGiFinite &= std::isfinite(E1.R) && std::isfinite(E2.R);
        SumStokesIrrad += E1.R + E1.G + E1.B;
        SumSurfelIrrad += E2.R + E2.G + E2.B;
    }
    Check(AllGiFinite && SumStokesIrrad > 10.0 && SumSurfelIrrad > 10.0,
        "GTX GI #1 (Stokes) and GTX GI #2 (Surfel) produce active finite indirect irradiance");

    // 3c. Verify DeterministicPathTraceSequence GTX vs RTX parity & progressive sample-splitting determinism
    PerspectiveCameraBasis Cam {};
    Cam.CameraOrigin       = { 2.8f, -3.2f, 2.0f };
    Cam.TargetPoint        = { 0.0f,  0.1f, 0.65f };
    Cam.WorldUpAxis        = { 0.0f,  0.0f, 1.0f };
    Cam.VerticalFovRadians = 0.78f;
    Cam.ViewportWidthPx    = 32u;
    Cam.ViewportHeightPx   = 24u;

    DeterministicPathTraceSequence PT {};
    PT.PrepareSceneSources(Scene.SceneTree, Scene.MaterialRegister);
    Check(!PT.EmissiveTriangles.empty() && PT.TotalEmissiveArea > 0.1f,
        "DeterministicPathTraceSequence extracted real emissive triangles for NEE/MIS");

    PathTraceConfiguration GtxCfg {};
    GtxCfg.WidthPx          = 32u;
    GtxCfg.HeightPx         = 24u;
    GtxCfg.SamplesPerPixel  = 8u;
    GtxCfg.MaxBounceCount   = 4u;
    GtxCfg.AccelerationMode = PathTraceAccelerationMode::GtxWideBoundingTree;

    PathTraceConfiguration RtxCfg = GtxCfg;
    RtxCfg.AccelerationMode = PathTraceAccelerationMode::RtxInlineRayQuery;

    PerspectiveCameraBasis PtCam = Cam;
    PtCam.ViewportWidthPx  = 32u;
    PtCam.ViewportHeightPx = 24u;

    const auto PtGtxA = PT.RenderViewport(GtxCfg, PtCam, Scene.SceneTree, Scene.MaterialRegister, Scene.Sun, {}, Scene.SkyRadiance);
    const auto PtRtx  = PT.RenderViewport(RtxCfg, PtCam, Scene.SceneTree, Scene.MaterialRegister, Scene.Sun, {}, Scene.SkyRadiance);

    // Progressive batch split: [0..3] + [4..7] == [0..7]
    PathTraceConfiguration Batch0 = GtxCfg;
    Batch0.SamplesPerPixel  = 4u;
    Batch0.StartSampleIndex = 0u;
    PathTraceConfiguration Batch1 = GtxCfg;
    Batch1.SamplesPerPixel  = 4u;
    Batch1.StartSampleIndex = 4u;
    const auto PtB0 = PT.RenderViewport(Batch0, PtCam, Scene.SceneTree, Scene.MaterialRegister, Scene.Sun, {}, Scene.SkyRadiance);
    const auto PtB1 = PT.RenderViewport(Batch1, PtCam, Scene.SceneTree, Scene.MaterialRegister, Scene.Sun, {}, Scene.SkyRadiance);

    float MaxGtxRtxDiff = 0.0f;
    float MaxBatchDiff  = 0.0f;
    for (std::size_t I = 0u; I < PtGtxA.LinearHdrRadiance.size(); ++I)
    {
        MaxGtxRtxDiff = std::max(MaxGtxRtxDiff, std::abs(PtGtxA.LinearHdrRadiance[I].R - PtRtx.LinearHdrRadiance[I].R));
        const float SplitMeanR = 0.5f * (PtB0.LinearHdrRadiance[I].R + PtB1.LinearHdrRadiance[I].R);
        MaxBatchDiff = std::max(MaxBatchDiff, std::abs(PtGtxA.LinearHdrRadiance[I].R - SplitMeanR));
    }
    Check(MaxGtxRtxDiff < 1.0e-6f, "DeterministicPathTraceSequence GTX BVH4 & RTX inline query bit-exact parity");
    Check(MaxBatchDiff < 1.0e-5f,  "DeterministicPathTraceSequence progressive batch split invariance (0..3 + 4..7 == 0..7)");

    std::printf("%u checks passed\n", Passed);
    return 0;
}
