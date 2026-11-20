/*============================================================================================================================================
                                                        STOKESCONTOURGICOMPUTE.SLANG
============================================================================================================================================*/
// 🧩 GTX GI #1 Compute: Closed-Form Spherical-Polygon Stokes Contour Emissive GI + 2nd-Bounce Triangle Surface Patches (Zero SDF, Zero Denoiser)

module StokesContourGiCompute;

import WideTreeRayQuery;

public struct GpuEmissiveTriangleSource64
{
    public float3 VertexA;
    public float  AreaM2;
    public float3 VertexB;
    public float  PaddingA;
    public float3 VertexC;
    public float  PaddingB;
    public float3 UnitNormal;
    public float  PaddingC;
    public float3 Radiance;
    public float  PaddingD;
};

public struct GpuDiffuseSurfacePatch48
{
    public float3 Center;
    public float  AreaM2;
    public float3 UnitNormal;
    public float  PaddingA;
    public float3 Radiance;
    public float  PaddingB;
};

struct StokesGiUniforms
{
    uint ViewportWidth;
    uint ViewportHeight;
    uint EmissiveSourceCount;
    uint DiffusePatchCount;
    uint WideEntryCount;
    uint Padding0;
    uint Padding1;
    uint Padding2;
};

[[vk::binding(0, 0)]] ConstantBuffer<StokesGiUniforms>              GiUniforms;
[[vk::binding(1, 0)]] StructuredBuffer<GpuWideTreeEntry128>         SceneWideTree;
[[vk::binding(2, 0)]] StructuredBuffer<GpuRayTrianglePrimitive48>   ScenePrimitives;
[[vk::binding(3, 0)]] StructuredBuffer<GpuEmissiveTriangleSource64> EmissiveSources;
[[vk::binding(4, 0)]] StructuredBuffer<GpuDiffuseSurfacePatch48>    DiffusePatches;
[[vk::binding(5, 0)]] Texture2D<float4>                             WorldPositionTex;
[[vk::binding(6, 0)]] Texture2D<float4>                             WorldNormalTex;
[[vk::binding(7, 0)]] RWTexture2D<float4>                           OutIrradianceOverPi;

float EvaluateTriangleFormFactor(float3 ReceiverP, float3 ReceiverN, float3 VertexA, float3 VertexB, float3 VertexC)
{
    float3 Input[3];
    Input[0] = VertexA - ReceiverP;
    Input[1] = VertexB - ReceiverP;
    Input[2] = VertexC - ReceiverP;

    float3 Poly[4];
    uint   Count = 0u;
    [unroll]
    for (uint Idx = 0u; Idx < 3u; ++Idx)
    {
        float3 A  = Input[Idx];
        float3 B  = Input[(Idx + 1u) % 3u];
        float  DA = dot(A, ReceiverN);
        float  DB = dot(B, ReceiverN);
        if (DA >= 0.0)
        {
            Poly[Count++] = A;
        }
        if ((DA < 0.0) != (DB < 0.0))
        {
            Poly[Count++] = A + (B - A) * (DA / (DA - DB));
        }
    }
    if (Count < 3u)
    {
        return 0.0;
    }

    for (uint I = 0u; I < Count; ++I)
    {
        float Len = length(Poly[I]);
        if (Len < 1.0e-7)
        {
            return 0.0;
        }
        Poly[I] = Poly[I] / Len;
    }

    float Sum = 0.0;
    for (uint J = 0u; J < Count; ++J)
    {
        float3 A        = Poly[J];
        float3 B        = Poly[(J + 1u) % Count];
        float3 CrossVec = cross(A, B);
        float  CrossLen = length(CrossVec);
        if (CrossLen > 1.0e-7)
        {
            Sum += atan2(CrossLen, dot(A, B)) * (dot(CrossVec, ReceiverN) / CrossLen);
        }
    }
    return clamp(abs(Sum) / 6.28318530718, 0.0, 1.0);
}

bool SegmentVisible(float3 Origin, float3 Target)
{
    float3 Delta    = Target - Origin;
    float  Distance = length(Delta);
    if (Distance < 0.012)
    {
        return false;
    }
    GpuRayHitSummary Hit = TraceWideBoundingTreeClosest(
        SceneWideTree, ScenePrimitives, GiUniforms.WideEntryCount, Origin, Delta / Distance, 0.002, Distance - 0.008);
    return !Hit.HitConfirmed;
}

[shader("compute")]
[numthreads(8, 8, 1)]
void StokesContourGiComputeEntry(uint3 DispatchThreadID : SV_DispatchThreadID)
{
    if (DispatchThreadID.x >= GiUniforms.ViewportWidth || DispatchThreadID.y >= GiUniforms.ViewportHeight)
    {
        return;
    }

    float4 PosSample = WorldPositionTex.Load(int3(DispatchThreadID.xy, 0));
    if (PosSample.w <= 0.0)
    {
        OutIrradianceOverPi[DispatchThreadID.xy] = float4(0.0);
        return;
    }

    float3 Position = PosSample.xyz;
    float3 Normal   = normalize(WorldNormalTex.Load(int3(DispatchThreadID.xy, 0)).xyz);
    float3 Origin   = Position + Normal * 0.006;

    float3 SumIrrad = float3(0.0);

    // 1. Direct Emissive Triangle Stokes Contour Integration
    for (uint SIdx = 0u; SIdx < GiUniforms.EmissiveSourceCount; ++SIdx)
    {
        GpuEmissiveTriangleSource64 Src = EmissiveSources[SIdx];
        float3 Center = (Src.VertexA + Src.VertexB + Src.VertexC) * (1.0 / 3.0);
        if (dot(Src.UnitNormal, Position - Center) <= 0.0)
        {
            continue;
        }
        float Form = EvaluateTriangleFormFactor(Position, Normal, Src.VertexA, Src.VertexB, Src.VertexC);
        if (Form <= 1.0e-7)
        {
            continue;
        }
        float3 Targets[4];
        Targets[0] = Center;
        Targets[1] = Src.VertexA * 0.6 + Src.VertexB * 0.2 + Src.VertexC * 0.2;
        Targets[2] = Src.VertexA * 0.2 + Src.VertexB * 0.6 + Src.VertexC * 0.2;
        Targets[3] = Src.VertexA * 0.2 + Src.VertexB * 0.2 + Src.VertexC * 0.6;
        float Vis = 0.0;
        [unroll]
        for (uint TIdx = 0u; TIdx < 4u; ++TIdx)
        {
            if (SegmentVisible(Origin, Targets[TIdx]))
            {
                Vis += 0.25;
            }
        }
        SumIrrad += Src.Radiance * (Form * Vis);
    }

    // 2. Second-Bounce Area-Sampled Diffuse Triangle Patches
    for (uint PIdx = 0u; PIdx < GiUniforms.DiffusePatchCount; ++PIdx)
    {
        GpuDiffuseSurfacePatch48 Patch = DiffusePatches[PIdx];
        float3 Delta  = Patch.Center - Position;
        float  DistSq = dot(Delta, Delta);
        if (DistSq < 0.0025)
        {
            continue;
        }
        float3 Dir    = Delta * rsqrt(DistSq);
        float  RecCos = max(0.0, dot(Normal, Dir));
        float  SrcCos = max(0.0, -dot(Patch.UnitNormal, Dir));
        float  Form   = (RecCos * SrcCos * Patch.AreaM2) / (3.14159265359 * DistSq + Patch.AreaM2);
        if (Form < 1.0e-6)
        {
            continue;
        }
        if (SegmentVisible(Origin, Patch.Center + Patch.UnitNormal * 0.008))
        {
            SumIrrad += Patch.Radiance * Form;
        }
    }

    OutIrradianceOverPi[DispatchThreadID.xy] = float4(SumIrrad, 1.0);
}
