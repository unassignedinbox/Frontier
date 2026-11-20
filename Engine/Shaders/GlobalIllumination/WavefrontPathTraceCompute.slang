/*============================================================================================================================================
                                                      WAVEFRONTPATHTRACECOMPUTE.SLANG
============================================================================================================================================*/
// 🧩 Unified Deterministic Path Tracer Compute Shader:
//    GTX (128-byte 4-Wide SAH BVH4 via WideTreeRayQuery) & RTX (Inline VK_KHR_ray_query BLAS/TLAS) Sharing Sobol (0,2) + GGX VNDF

module WavefrontPathTraceCompute;

import WideTreeRayQuery;
import DeterministicQuadrature;

#ifndef FRONTIER_PT_USE_RAY_QUERY
#define FRONTIER_PT_USE_RAY_QUERY 0
#endif

struct PathTraceUniforms
{
    float4 CameraOriginTanX;
    float4 CameraRightTanY;
    float4 CameraUpWidth;
    float4 CameraForwardHeight;
    float4 SunDirConeTan;
    float4 SolarIrradiancePad;
    float4 SkyRadianceBvhCount;
    uint4  SampleBounceMask; // x=SampleIndex, y=MaxBounces, z=MaterialCount, w=OptionMask(bit0=SharedQuadrature, bit1=RtxRayQuery)
};

struct GpuCompactPathMaterial
{
    float4 DiffuseAlbedoRough;
    float4 SpecRoughMetalIor;
    float4 EmissiveRadiancePad;
};

[[vk::binding(0, 0)]] ConstantBuffer<PathTraceUniforms>          PtUniforms;
[[vk::binding(1, 0)]] StructuredBuffer<GpuWideTreeEntry128>      SceneWideTree;
[[vk::binding(2, 0)]] StructuredBuffer<GpuRayTrianglePrimitive48> ScenePrimitives;
[[vk::binding(3, 0)]] StructuredBuffer<GpuCompactPathMaterial>   MaterialEntries;
[[vk::binding(4, 0)]] RWStructuredBuffer<float4>                 AccumulationHdrBuffer;
[[vk::binding(5, 0)]] RWStructuredBuffer<uint>                   OutDisplayRgba8;
#if FRONTIER_PT_USE_RAY_QUERY
[[vk::binding(6, 0)]] RaytracingAccelerationStructure            SceneTlas;
#endif

float ApplyAcesFilmic(float V)
{
    float X = max(0.0, V * 0.85);
    return clamp((X * (2.51 * X + 0.03)) / (X * (2.43 * X + 0.59) + 0.14), 0.0, 1.0);
}

uint EncodeRgba8Srgb(float3 LinearColor)
{
    float3 Mapped = float3(ApplyAcesFilmic(LinearColor.x), ApplyAcesFilmic(LinearColor.y), ApplyAcesFilmic(LinearColor.z));
    float3 Srgb   = select(Mapped <= 0.0031308, Mapped * 12.92, 1.055 * pow(max(Mapped, 0.0), 1.0 / 2.4) - 0.055);
    uint   R      = uint(round(saturate(Srgb.x) * 255.0));
    uint   G      = uint(round(saturate(Srgb.y) * 255.0));
    uint   B      = uint(round(saturate(Srgb.z) * 255.0));
    return (255u << 24u) | (B << 16u) | (G << 8u) | R;
}

GpuRayHitSummary TraceSceneClosest(float3 Origin, float3 Dir, float MinT, float MaxT)
{
#if FRONTIER_PT_USE_RAY_QUERY
    GpuRayHitSummary Hit;
    Hit.HitConfirmed      = false;
    Hit.HitDistanceMeters = MaxT;
    Hit.BarycentricsUV    = float2(0.0);
    Hit.MaterialOrdinal   = 0u;
    Hit.GeometricNormal   = float3(0.0, 0.0, 1.0);
    Hit.SourceFaceIndex   = 0xFFFFFFFFu;
    Hit.HitFrontFace      = true;

    RayDesc Desc;
    Desc.Origin    = Origin;
    Desc.Direction = Dir;
    Desc.TMin      = MinT;
    Desc.TMax      = MaxT;

    RayQuery<RAY_FLAG_NONE> Query;
    Query.TraceRayInline(SceneTlas, RAY_FLAG_NONE, 0xFFu, Desc);
    while (Query.Proceed())
    {
        if (Query.CandidateType() == CANDIDATE_NON_OPAQUE_TRIANGLE)
        {
            Query.CommitNonOpaqueTriangleHit();
        }
    }
    if (Query.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
    {
        uint PrimIdx = Query.CommittedPrimitiveIndex();
        GpuRayTrianglePrimitive48 Prim = ScenePrimitives[PrimIdx];
        float3 RawCross = cross(Prim.Edge01, Prim.Edge02);
        float  CrossLen = length(RawCross);
        float3 GeomN    = (CrossLen > 1.0e-12) ? (RawCross / CrossLen) : float3(0.0, 0.0, 1.0);
        Hit.HitConfirmed      = true;
        Hit.HitDistanceMeters = Query.CommittedRayT();
        Hit.BarycentricsUV    = Query.CommittedTriangleBarycentrics();
        Hit.MaterialOrdinal   = Prim.PackedMaterialOpaque & 0xFFFFu;
        Hit.GeometricNormal   = GeomN;
        Hit.SourceFaceIndex   = Prim.SourceFaceIndex;
        Hit.HitFrontFace      = (dot(Dir, GeomN) < 0.0);
    }
    return Hit;
#else
    return TraceWideBoundingTreeClosest(
        SceneWideTree, ScenePrimitives, uint(PtUniforms.SkyRadianceBvhCount.w), Origin, Dir, MinT, MaxT);
#endif
}

bool TraceSceneOccluded(float3 Origin, float3 Dir, float MinT, float MaxT)
{
#if FRONTIER_PT_USE_RAY_QUERY
    RayDesc Desc;
    Desc.Origin    = Origin;
    Desc.Direction = Dir;
    Desc.TMin      = MinT;
    Desc.TMax      = MaxT;

    RayQuery<RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH> Query;
    Query.TraceRayInline(SceneTlas, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH, 0xFFu, Desc);
    while (Query.Proceed())
    {
        if (Query.CandidateType() == CANDIDATE_NON_OPAQUE_TRIANGLE)
        {
            Query.CommitNonOpaqueTriangleHit();
        }
    }
    return Query.CommittedStatus() == COMMITTED_TRIANGLE_HIT;
#else
    GpuRayHitSummary Hit = TraceWideBoundingTreeClosest(
        SceneWideTree, ScenePrimitives, uint(PtUniforms.SkyRadianceBvhCount.w), Origin, Dir, MinT, MaxT);
    return Hit.HitConfirmed;
#endif
}

float3 EvaluateSimpleBrdf(GpuCompactPathMaterial Mat, float3 LocalWo, float3 LocalWi)
{
    if (LocalWo.z <= 1.0e-5 || LocalWi.z <= 1.0e-5)
    {
        return float3(0.0);
    }
    float3 HalfV   = normalize(LocalWo + LocalWi);
    float  NdotH   = max(1.0e-4, HalfV.z);
    float  VdotH   = max(1.0e-4, dot(LocalWo, HalfV));
    float  Rough   = max(0.025, Mat.SpecRoughMetalIor.x);
    float  Alpha   = Rough * Rough;
    float  Alpha2  = Alpha * Alpha;
    float  Denom   = NdotH * NdotH * (Alpha2 - 1.0) + 1.0;
    float  Ndf     = Alpha2 / max(1.0e-6, 3.14159265359 * Denom * Denom);
    float  K       = (Rough + 1.0) * (Rough + 1.0) * 0.125;
    float  G1V     = LocalWo.z / (LocalWo.z * (1.0 - K) + K);
    float  G1L     = LocalWi.z / (LocalWi.z * (1.0 - K) + K);
    float  F0Base  = pow((Mat.SpecRoughMetalIor.z - 1.0) / (Mat.SpecRoughMetalIor.z + 1.0), 2.0);
    float3 F0      = lerp(float3(F0Base), Mat.DiffuseAlbedoRough.xyz, Mat.SpecRoughMetalIor.y);
    float3 Fresnel = F0 + (1.0 - F0) * pow(1.0 - VdotH, 5.0);
    float3 Spec    = (Ndf * G1V * G1L * Fresnel) / max(1.0e-5, 4.0 * LocalWo.z * LocalWi.z);
    float3 Diff    = Mat.DiffuseAlbedoRough.xyz * ((1.0 - Mat.SpecRoughMetalIor.y) / 3.14159265359);
    return Diff + Spec;
}

[shader("compute")]
[numthreads(8, 8, 1)]
void WavefrontPathTraceComputeEntry(uint3 DispatchThreadID : SV_DispatchThreadID)
{
    uint Width  = uint(PtUniforms.CameraUpWidth.w);
    uint Height = uint(PtUniforms.CameraForwardHeight.w);
    if (DispatchThreadID.x >= Width || DispatchThreadID.y >= Height)
    {
        return;
    }

    float NdcX = ((float(DispatchThreadID.x) + 0.5) / float(Width)) * 2.0 - 1.0;
    float NdcY = 1.0 - ((float(DispatchThreadID.y) + 0.5) / float(Height)) * 2.0;

    float3 Origin = PtUniforms.CameraOriginTanX.xyz;
    float3 Dir = normalize(
        PtUniforms.CameraForwardHeight.xyz +
        PtUniforms.CameraRightTanY.xyz * (NdcX * PtUniforms.CameraOriginTanX.w) +
        PtUniforms.CameraUpWidth.xyz   * (NdcY * PtUniforms.CameraRightTanY.w));

    uint SampleIdx = PtUniforms.SampleBounceMask.x;
    uint MaxBounce = max(1u, PtUniforms.SampleBounceMask.y);
    uint PixelKey  = ((PtUniforms.SampleBounceMask.w & 1u) != 0u)
        ? 0u
        : AvalancheKey32(DispatchThreadID.x * 747796405u + DispatchThreadID.y * 2891336453u + 1u);

    float3 SunDir      = normalize(PtUniforms.SunDirConeTan.xyz);
    float  TanSunCone  = PtUniforms.SunDirConeTan.w;
    float  SunConePdf  = 1.0 / max(1.0e-7, 3.14159265359 * TanSunCone * TanSunCone);

    float3 Throughput = float3(1.0);
    float3 PathRad    = float3(0.0);

    for (uint Bounce = 0u; Bounce < MaxBounce; ++Bounce)
    {
        GpuRayHitSummary Hit = TraceSceneClosest(Origin, Dir, (Bounce == 0u) ? 1.0e-3 : 2.0e-3, 150.0);
        if (!Hit.HitConfirmed)
        {
            float UpFactor = saturate(Dir.z * 0.5 + 0.5);
            float3 SkyVal  = PtUniforms.SkyRadianceBvhCount.xyz * (0.65 + 0.70 * UpFactor);
            float  SunCos  = max(0.0, dot(Dir, SunDir));
            SkyVal += PtUniforms.SolarIrradiancePad.xyz * (pow(SunCos, 160.0) * 0.15);
            PathRad += Throughput * SkyVal;
            break;
        }

        float3 HitPos = Origin + Dir * Hit.HitDistanceMeters;
        float3 Nrm    = (dot(Hit.GeometricNormal, Dir) < 0.0) ? Hit.GeometricNormal : -Hit.GeometricNormal;
        uint   Slot   = min(Hit.MaterialOrdinal, max(1u, PtUniforms.SampleBounceMask.z) - 1u);
        GpuCompactPathMaterial Mat = MaterialEntries[Slot];

        PathRad += Throughput * Mat.EmissiveRadiancePad.xyz;

        float3 Tangent, Bitangent;
        BuildDuffBasis(Nrm, Tangent, Bitangent);
        float3 WoWorld = -Dir;
        float3 LocalWo = float3(dot(WoWorld, Tangent), dot(WoWorld, Bitangent), dot(WoWorld, Nrm));
        float3 ShadowOrigin = HitPos + Nrm * 3.0e-3;

        // Solar Disc NEE
        float2 USun = Sobol02Sample(SampleIdx, PixelKey, Bounce * 5u + 0u);
        float3 SunT, SunB;
        BuildDuffBasis(SunDir, SunT, SunB);
        float  ConeR    = sqrt(USun.x) * TanSunCone;
        float  ConeA    = 6.28318530718 * USun.y;
        float3 LightDir = normalize(SunDir + SunT * (ConeR * cos(ConeA)) + SunB * (ConeR * sin(ConeA)));
        float  NdotL    = dot(Nrm, LightDir);
        if (NdotL > 1.0e-4 && !TraceSceneOccluded(ShadowOrigin, LightDir, 1.5e-3, 120.0))
        {
            float3 LocalWi  = float3(dot(LightDir, Tangent), dot(LightDir, Bitangent), dot(LightDir, Nrm));
            float3 Bsdf     = EvaluateSimpleBrdf(Mat, LocalWo, LocalWi);
            float  Rough    = max(0.025, Mat.SpecRoughMetalIor.x);
            float  Alpha    = Rough * Rough;
            float  SpecProb = clamp(0.20 + 0.65 * Mat.SpecRoughMetalIor.y, 0.15, 0.85);
            float  BsdfPdf  = SpecProb * EvaluateSphericalCapGgxVndfPdf(LocalWo, LocalWi, Alpha, Alpha) +
                              (1.0 - SpecProb) * (max(0.0, LocalWi.z) / 3.14159265359);
            float  MisW     = PowerHeuristicMis(SunConePdf, BsdfPdf);
            PathRad += Throughput * Bsdf * PtUniforms.SolarIrradiancePad.xyz * (NdotL * MisW);
        }

        if (Bounce + 1u >= MaxBounce)
        {
            break;
        }

        float2 UBsdf    = Sobol02Sample(SampleIdx, PixelKey, Bounce * 5u + 4u);
        float2 ULobe    = Sobol02Sample(SampleIdx, PixelKey, Bounce * 5u + 5u);
        float  Rough    = max(0.025, Mat.SpecRoughMetalIor.x);
        float  Alpha    = Rough * Rough;
        float  SpecProb = clamp(0.20 + 0.65 * Mat.SpecRoughMetalIor.y, 0.15, 0.85);

        float3 LocalWi;
        if (ULobe.x < SpecProb)
        {
            LocalWi = SampleSphericalCapGgxVndfLocal(LocalWo, Alpha, Alpha, UBsdf.x, UBsdf.y);
        }
        else
        {
            float Rad = sqrt(UBsdf.x);
            float Phi = 6.28318530718 * UBsdf.y;
            LocalWi = float3(Rad * cos(Phi), Rad * sin(Phi), sqrt(max(0.0, 1.0 - UBsdf.x)));
        }

        if (LocalWi.z <= 1.0e-4)
        {
            break;
        }

        float MixPdf = SpecProb * EvaluateSphericalCapGgxVndfPdf(LocalWo, LocalWi, Alpha, Alpha) +
                       (1.0 - SpecProb) * (LocalWi.z / 3.14159265359);
        if (MixPdf <= 1.0e-7)
        {
            break;
        }

        float3 Bsdf = EvaluateSimpleBrdf(Mat, LocalWo, LocalWi);
        Throughput *= Bsdf * (LocalWi.z / MixPdf);
        if (max(Throughput.x, max(Throughput.y, Throughput.z)) <= 1.0e-5)
        {
            break;
        }

        Origin = ShadowOrigin;
        Dir    = normalize(Tangent * LocalWi.x + Bitangent * LocalWi.y + Nrm * LocalWi.z);
    }

    uint PixelIdx = DispatchThreadID.y * Width + DispatchThreadID.x;
    float3 PrevMean = (SampleIdx == 0u) ? float3(0.0) : AccumulationHdrBuffer[PixelIdx].xyz;
    float3 NextMean = PrevMean + (PathRad - PrevMean) / float(SampleIdx + 1u);
    AccumulationHdrBuffer[PixelIdx] = float4(NextMean, 1.0);
    OutDisplayRgba8[PixelIdx]       = EncodeRgba8Srgb(NextMean);
}
