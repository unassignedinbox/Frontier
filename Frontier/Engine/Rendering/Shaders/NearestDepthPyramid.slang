/*==========================================================================================================================================
                                                      NEAREST DEPTH PYRAMID.SLANG
                                     ------------------------------------------------------------------
                   Level 0 reversed-depth transfer and conservative 2×2 (plus unpartnered odd-border
                     3-wide / 3-tall) hierarchical reduction — 1:1 Slang counterpart of
                     NearestDepthTransfer.wgsl and NearestDepthReduction.wgsl.
==========================================================================================================================================*/

[[vk::binding(0, 0)]] Texture2D<float>   DepthSurface;
[[vk::binding(1, 0)]] RWTexture2D<float> PyramidLevel0;

[[vk::binding(2, 0)]] Texture2D<float>   SourcePyramidLevel;
[[vk::binding(3, 0)]] RWTexture2D<float> TargetPyramidLevel;

[shader("compute")]
[numthreads(8, 8, 1)]
void TransferNearestDepth(uint3 Pixel : SV_DispatchThreadID)
{
    uint Width = 0u;
    uint Height = 0u;
    PyramidLevel0.GetDimensions(Width, Height);
    if (Pixel.x >= Width || Pixel.y >= Height)
    {
        return;
    }

    float ReversedDepth = DepthSurface.Load(int3(int2(Pixel.xy), 0));
    PyramidLevel0[Pixel.xy] = ReversedDepth;
}

[shader("compute")]
[numthreads(8, 8, 1)]
void ReduceNearestDepth(uint3 Pixel : SV_DispatchThreadID)
{
    uint TargetWidth = 0u;
    uint TargetHeight = 0u;
    TargetPyramidLevel.GetDimensions(TargetWidth, TargetHeight);
    if (Pixel.x >= TargetWidth || Pixel.y >= TargetHeight)
    {
        return;
    }

    uint SourceWidth = 0u;
    uint SourceHeight = 0u;
    SourcePyramidLevel.GetDimensions(SourceWidth, SourceHeight);

    uint2 Base = Pixel.xy * 2u;
    uint2 Limit = uint2(SourceWidth - 1u, SourceHeight - 1u);

    // When the finer level has an odd width or height, the last column or row has no partner at the
    // coarser level — fold it into the border texel so the pyramid remains a conservative upper bound.
    bool UnpartneredX = ((SourceWidth & 1u) == 1u) && (Pixel.x + 1u == TargetWidth);
    bool UnpartneredY = ((SourceHeight & 1u) == 1u) && (Pixel.y + 1u == TargetHeight);
    uint SpanX = 2u + (UnpartneredX ? 1u : 0u);
    uint SpanY = 2u + (UnpartneredY ? 1u : 0u);

    float Nearest = 0.0;
    for (uint Row = 0u; Row < SpanY; ++Row)
    {
        for (uint Column = 0u; Column < SpanX; ++Column)
        {
            uint2 Texel = min(Base + uint2(Column, Row), Limit);
            Nearest = max(Nearest, SourcePyramidLevel.Load(int3(int2(Texel), 0)).x);
        }
    }

    TargetPyramidLevel[Pixel.xy] = Nearest;
}
