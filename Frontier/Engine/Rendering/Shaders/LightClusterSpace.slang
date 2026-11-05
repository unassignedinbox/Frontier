/*==========================================================================================================================================
                                                        LIGHT CLUSTERSPACE.SLANG
                                     ------------------------------------------------------------------
                   The 32 × 16 × 16 world-space light cluster grid and power-weighted alias table
                     over the scene's analytic and emissive lights — 1:1 Slang counterpart of
                     LightClusterSpace.wgsl and EmissiveLightSpace.
==========================================================================================================================================*/

struct LightClusterCell
{
    uint Offset;
    uint Count;
    float TotalWeight;
    uint Padding;
};

struct LightClusterAliasEntry
{
    float Probability;
    uint Alias;
    uint LightIndex;
    float Weight;
};

struct EmissiveTriangleEntry
{
    float4 CornerAAndArea;       // xyz = CornerA, w = Area
    float4 EdgeABAndPower;       // xyz = EdgeAB,  w = Power
    float4 EdgeACAndCumulative;  // xyz = EdgeAC,  w = Cumulative probability
    float4 NormalAndEmission;    // xyz = Normal,  w = MaxEmission
    float4 Radiance;             // rgb = Emissive radiance, w = TriangleId
};

static const float3 ClusterOrigin = float3(-40.0, -32.0, -2.0);
static const float3 ClusterScale  = float3(80.0, 64.0, 26.0);
static const uint3  ClusterGrid   = uint3(32u, 16u, 16u);

uint LocateLightClusterCell(float3 WorldPosition)
{
    float3 Normalized = saturate((WorldPosition - ClusterOrigin) / ClusterScale);
    uint3 Cell = min(uint3(Normalized * float3(ClusterGrid)), ClusterGrid - uint3(1u, 1u, 1u));
    return Cell.x + ClusterGrid.x * (Cell.y + ClusterGrid.y * Cell.z);
}

uint SampleLightClusterAlias(
    StructuredBuffer<LightClusterCell> Cells,
    StructuredBuffer<LightClusterAliasEntry> Entries,
    float3 WorldPosition,
    float2 UniformRandom,
    out float SelectionProbability)
{
    uint CellIndex = LocateLightClusterCell(WorldPosition);
    LightClusterCell Cell = Cells[CellIndex];
    if (Cell.Count == 0u || Cell.TotalWeight <= 0.0)
    {
        SelectionProbability = 0.0;
        return 0xFFFFFFFFu;
    }

    uint Slot = min(uint(UniformRandom.x * float(Cell.Count)), Cell.Count - 1u);
    LightClusterAliasEntry Candidate = Entries[Cell.Offset + Slot];
    uint PickedSlot = (UniformRandom.y < Candidate.Probability) ? Slot : Candidate.Alias;
    LightClusterAliasEntry Picked = Entries[Cell.Offset + PickedSlot];
    SelectionProbability = max(Picked.Weight / Cell.TotalWeight, 1.0e-7);
    return Picked.LightIndex;
}

float3 SampleEmissiveTrianglePoint(EmissiveTriangleEntry Triangle, float2 UniformRandom, out float AreaPdf)
{
    float RootU = sqrt(clamp(UniformRandom.x, 0.0, 1.0));
    float BarycentricU = 1.0 - RootU;
    float BarycentricV = UniformRandom.y * RootU;
    AreaPdf = 1.0 / max(Triangle.CornerAAndArea.w, 1.0e-7);
    return Triangle.CornerAAndArea.xyz
         + Triangle.EdgeABAndPower.xyz * BarycentricU
         + Triangle.EdgeACAndCumulative.xyz * BarycentricV;
}
