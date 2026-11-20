/*============================================================================================================================================
                                                        SURFELCASCADEGICOMPUTE.SLANG
============================================================================================================================================*/
// 🧩 GTX GI #2 Compute: Dual-Cascade Octahedral Surfel Irradiance + 16x16 Chebyshev Depth Moments (O(1) Lookup, Zero SDF, Zero Denoiser)

module SurfelCascadeGiCompute;

public struct GpuOctahedralCellHeader
{
    public float3 Position;
    public uint   CellOrdinal;
};

struct SurfelCascadeUniforms
{
    uint   ViewportWidth;
    uint   ViewportHeight;
    float3 InnerMinimum;
    float  InnerPadding;
    float3 InnerSpacing;
    uint   InnerCountX;
    uint   InnerCountY;
    uint   InnerCountZ;
    float3 OuterMinimum;
    float  OuterPadding;
    float3 OuterSpacing;
    uint   OuterCountX;
    uint   OuterCountY;
    uint   OuterCountZ;
};

[[vk::binding(0, 0)]] ConstantBuffer<SurfelCascadeUniforms> CascadeUniforms;
[[vk::binding(1, 0)]] StructuredBuffer<float4>              InnerIrradiance64;  // 64 texels per cell (8x8)
[[vk::binding(2, 0)]] StructuredBuffer<float2>              InnerDepthMoments;  // 256 texels per cell (16x16)
[[vk::binding(3, 0)]] StructuredBuffer<float4>              OuterIrradiance64;
[[vk::binding(4, 0)]] StructuredBuffer<float2>              OuterDepthMoments;
[[vk::binding(5, 0)]] Texture2D<float4>                     WorldPositionTex;
[[vk::binding(6, 0)]] Texture2D<float4>                     WorldNormalTex;
[[vk::binding(7, 0)]] RWTexture2D<float4>                   OutIrradianceOverPi;

float2 EncodeOctahedralDirection(float3 Direction)
{
    float L1 = abs(Direction.x) + abs(Direction.y) + abs(Direction.z);
    if (L1 < 1.0e-7)
    {
        return float2(0.5, 0.5);
    }
    float3 D = Direction / L1;
    if (D.z < 0.0)
    {
        float X = D.x;
        D.x = (1.0 - abs(D.y)) * ((X >= 0.0) ? 1.0 : -1.0);
        D.y = (1.0 - abs(X))   * ((D.y >= 0.0) ? 1.0 : -1.0);
    }
    return D.xy * 0.5 + 0.5;
}

float3 EvaluateSingleCascade(
    StructuredBuffer<float4> IrradianceTexels,
    StructuredBuffer<float2> DepthMomentsTexels,
    float3                   MinBounds,
    float3                   Spacing,
    uint3                    Counts,
    float3                   Position,
    float3                   Normal,
    out float                OutConfidence)
{
    OutConfidence = 0.0;
    if (Counts.x < 2u || Counts.y < 2u || Counts.z < 2u)
    {
        return float3(0.0);
    }

    float3 Grid = (Position - MinBounds) / Spacing;
    if (Grid.x < -0.5 || Grid.x > float(Counts.x) - 0.5 ||
        Grid.y < -0.5 || Grid.y > float(Counts.y) - 0.5 ||
        Grid.z < -0.5 || Grid.z > float(Counts.z) - 0.5)
    {
        return float3(0.0);
    }

    float3 Clamped = clamp(Grid, float3(0.0), float3(Counts) - 1.0);
    int3   BaseIdx = min(int3(floor(Clamped)), int3(Counts) - 2);
    float3 Frac    = Clamped - float3(BaseIdx);

    float2 Oct = EncodeOctahedralDirection(Normal);
    float  U   = Oct.x * 8.0 - 0.5;
    float  V   = Oct.y * 8.0 - 0.5;
    float  FX  = U - floor(U);
    float  FY  = V - floor(V);

    float3 SumIrrad = float3(0.0);
    [unroll]
    for (uint Corner = 0u; Corner < 8u; ++Corner)
    {
        int3 CellCoord = BaseIdx + int3((Corner & 1u) != 0u ? 1 : 0, (Corner & 2u) != 0u ? 1 : 0, (Corner & 4u) != 0u ? 1 : 0);
        float Weight = (((Corner & 1u) != 0u) ? Frac.x : (1.0 - Frac.x)) *
                       (((Corner & 2u) != 0u) ? Frac.y : (1.0 - Frac.y)) *
                       (((Corner & 4u) != 0u) ? Frac.z : (1.0 - Frac.z));

        uint   CellIdx = (uint(CellCoord.z) * Counts.y + uint(CellCoord.y)) * Counts.x + uint(CellCoord.x);
        float3 CellPos = MinBounds + float3(CellCoord) * Spacing;

        float3 Delta    = Position + Normal * 0.04 - CellPos;
        float  Distance = length(Delta);
        float3 DirToPos = (Distance > 1.0e-6) ? (Delta / Distance) : float3(0.0, 0.0, 1.0);
        float2 DepthOct = EncodeOctahedralDirection(DirToPos);
        int    DX       = clamp(int(DepthOct.x * 16.0), 0, 15);
        int    DY       = clamp(int(DepthOct.y * 16.0), 0, 15);
        float2 Moments  = DepthMomentsTexels[CellIdx * 256u + uint(DY * 16 + DX)];

        float Variance   = max(0.0001, Moments.y - Moments.x * Moments.x);
        float Excess     = max(0.0, Distance - Moments.x - 0.05);
        float Visibility = Variance / (Variance + Excess * Excess);
        Weight *= Visibility * Visibility * Visibility;

        float Wrap = max(0.05, (1.0 - dot(Normal, DirToPos)) * 0.5);
        Weight *= Wrap * Wrap;

        float3 CellIrrad = float3(0.0);
        [unroll]
        for (int J = 0; J < 2; ++J)
        {
            [unroll]
            for (int I = 0; I < 2; ++I)
            {
                int TX = int(floor(U)) + I;
                int TY = int(floor(V)) + J;
                float Bilinear = ((I != 0) ? FX : (1.0 - FX)) * ((J != 0) ? FY : (1.0 - FY));
                if (TX < 0) { TX = -TX - 1; TY = 7 - TY; }
                if (TX > 7) { TX = 15 - TX; TY = 7 - TY; }
                if (TY < 0) { TY = -TY - 1; TX = 7 - TX; }
                if (TY > 7) { TY = 15 - TY; TX = 7 - TX; }
                CellIrrad += IrradianceTexels[CellIdx * 64u + uint(TY * 8 + TX)].xyz * Bilinear;
            }
        }
        SumIrrad += CellIrrad * Weight;
        OutConfidence += Weight;
    }
    return (OutConfidence > 1.0e-7) ? (SumIrrad / OutConfidence) : float3(0.0);
}

[shader("compute")]
[numthreads(8, 8, 1)]
void SurfelCascadeGiComputeEntry(uint3 DispatchThreadID : SV_DispatchThreadID)
{
    if (DispatchThreadID.x >= CascadeUniforms.ViewportWidth || DispatchThreadID.y >= CascadeUniforms.ViewportHeight)
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

    float InnerConf = 0.0;
    float OuterConf = 0.0;
    float3 InnerIrrad = EvaluateSingleCascade(
        InnerIrradiance64, InnerDepthMoments,
        CascadeUniforms.InnerMinimum, CascadeUniforms.InnerSpacing,
        uint3(CascadeUniforms.InnerCountX, CascadeUniforms.InnerCountY, CascadeUniforms.InnerCountZ),
        Position, Normal, InnerConf);
    float3 OuterIrrad = EvaluateSingleCascade(
        OuterIrradiance64, OuterDepthMoments,
        CascadeUniforms.OuterMinimum, CascadeUniforms.OuterSpacing,
        uint3(CascadeUniforms.OuterCountX, CascadeUniforms.OuterCountY, CascadeUniforms.OuterCountZ),
        Position, Normal, OuterConf);

    float3 Result = float3(0.0);
    if (InnerConf < 1.0e-6)
    {
        Result = OuterIrrad;
    }
    else if (OuterConf < 1.0e-6)
    {
        Result = InnerIrrad;
    }
    else
    {
        float Edge = min(
            min((Position.x - CascadeUniforms.InnerMinimum.x) / CascadeUniforms.InnerSpacing.x,
                (CascadeUniforms.InnerMinimum.x + float(CascadeUniforms.InnerCountX - 1u) * CascadeUniforms.InnerSpacing.x - Position.x) / CascadeUniforms.InnerSpacing.x),
            min((Position.y - CascadeUniforms.InnerMinimum.y) / CascadeUniforms.InnerSpacing.y,
                (CascadeUniforms.InnerMinimum.y + float(CascadeUniforms.InnerCountY - 1u) * CascadeUniforms.InnerSpacing.y - Position.y) / CascadeUniforms.InnerSpacing.y));
        float Blend = saturate(Edge);
        Result = lerp(OuterIrrad, InnerIrrad, Blend);
    }

    OutIrradianceOverPi[DispatchThreadID.xy] = float4(Result, 1.0);
}
