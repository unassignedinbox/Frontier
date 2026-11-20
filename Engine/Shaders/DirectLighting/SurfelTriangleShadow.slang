/*============================================================================================================================================
                                                          SURFELTRIANGLESHADOW.SLANG
============================================================================================================================================*/
// 🧩 Shadow Lab Mode 5: Real-Triangle SurfelOccluderField + 0.28 m WideBoundingTree Contact Ray (Zero SDF, Zero Capsules)

module SurfelTriangleShadow;

import WideTreeRayQuery;

public struct GpuSurfelOccluderDisk32
{
    public float3 Position;
    public float  RadiusMeters;
    public float3 Normal;
    public float  Padding;
};

struct SurfelShadowUniforms
{
    float3 SunDirection;
    float  TanSunAngularRadius;
    uint   ViewportWidth;
    uint   ViewportHeight;
    uint   WideEntryCount;
    uint   SurfelDiskCount;
};

[[vk::binding(0, 0)]] ConstantBuffer<SurfelShadowUniforms>        ShadowUniforms;
[[vk::binding(1, 0)]] StructuredBuffer<GpuWideTreeEntry128>       SceneWideTree;
[[vk::binding(2, 0)]] StructuredBuffer<GpuRayTrianglePrimitive48> ScenePrimitives;
[[vk::binding(3, 0)]] StructuredBuffer<GpuSurfelOccluderDisk32>   SurfelDisks;
[[vk::binding(4, 0)]] Texture2D<float4>                           WorldPositionTex;
[[vk::binding(5, 0)]] Texture2D<float4>                           WorldNormalTex;
[[vk::binding(6, 0)]] RWTexture2D<float4>                         OutShadowBlocker; // R=RawVis, G=BlockerDist, B=1, A=1

[shader("compute")]
[numthreads(8, 8, 1)]
void SurfelShadowComputeEntry(uint3 DispatchThreadID : SV_DispatchThreadID)
{
    if (DispatchThreadID.x >= ShadowUniforms.ViewportWidth || DispatchThreadID.y >= ShadowUniforms.ViewportHeight)
    {
        return;
    }

    float4 PosSample = WorldPositionTex.Load(int3(DispatchThreadID.xy, 0));
    float3 Nrm       = normalize(WorldNormalTex.Load(int3(DispatchThreadID.xy, 0)).xyz);
    if (PosSample.w <= 0.0)
    {
        OutShadowBlocker[DispatchThreadID.xy] = float4(1.0, 0.0, 1.0, 1.0);
        return;
    }

    float3 SunDir = normalize(ShadowUniforms.SunDirection);
    if (dot(Nrm, SunDir) <= 0.0)
    {
        OutShadowBlocker[DispatchThreadID.xy] = float4(0.0, 0.0, 1.0, 1.0);
        return;
    }

    float3 Origin = PosSample.xyz + Nrm * 0.006;

    // 1. Exact 0.28 m WideBoundingTree contact ray (preserves crisp grounded contact on real triangles)
    GpuRayHitSummary ContactHit = TraceWideBoundingTreeClosest(
        SceneWideTree, ScenePrimitives, ShadowUniforms.WideEntryCount, Origin, SunDir, 0.002, 0.28);
    if (ContactHit.HitConfirmed)
    {
        OutShadowBlocker[DispatchThreadID.xy] = float4(0.0, ContactHit.HitDistanceMeters, 1.0, 1.0);
        return;
    }

    // 2. Stratified real-triangle SurfelOccluderDisk soft penumbra evaluation
    float Visibility     = 1.0;
    float ClosestBlocker = 0.0;
    for (uint DiskIdx = 0u; DiskIdx < ShadowUniforms.SurfelDiskCount; ++DiskIdx)
    {
        GpuSurfelOccluderDisk32 Disk = SurfelDisks[DiskIdx];
        float3 Delta    = Disk.Position - Origin;
        float  Distance = dot(Delta, SunDir);
        if (Distance < 0.14 || Distance > 15.0)
        {
            continue;
        }
        if (dot(Delta, Nrm) < 0.02)
        {
            continue;
        }
        float Perp           = length(Delta - SunDir * Distance);
        float ConeRadius     = max(0.005, Distance * ShadowUniforms.TanSunAngularRadius);
        float ProjectedRad   = Disk.RadiusMeters * sqrt(max(0.02, abs(dot(Disk.Normal, SunDir))));
        float SmoothRatio    = saturate((Perp - ProjectedRad + ConeRadius) / (2.0 * ConeRadius));
        float DiskVisibility = SmoothRatio * SmoothRatio * (3.0 - 2.0 * SmoothRatio);
        if (DiskVisibility < Visibility)
        {
            Visibility = DiskVisibility;
            if (DiskVisibility < 0.995 && (ClosestBlocker <= 0.0 || Distance < ClosestBlocker))
            {
                ClosestBlocker = Distance;
            }
        }
        if (Visibility <= 0.0)
        {
            break;
        }
    }

    OutShadowBlocker[DispatchThreadID.xy] = float4(Visibility, ClosestBlocker, 1.0, 1.0);
}
