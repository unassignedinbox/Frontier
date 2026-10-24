/*==========================================================================================================================================
                                                               CADEDGE.SLANG
==========================================================================================================================================*/
// 🧩 Expand authored topology edges into antialiased pixel-width strokes without optional wide lines or geometry shaders.

#include "SurfaceProjection.slang"
#include "CadStroke.slang"

[[vk::push_constant]] ConstantBuffer<SurfaceProjection> Projection;

//----------------------------------------------------------------------------------------------------------------------
//                                                EDGE INPUT AND OUTPUT
//----------------------------------------------------------------------------------------------------------------------

struct EdgeInput
{
    float3 Start : POSITION0;
    float3 End : POSITION1;
};
struct EdgePixel
{
    float4 Position : SV_Position;
    noperspective float Distance : TEXCOORD0;
};

[shader("vertex")]
// 📝 Direct Vulkan indexing avoids the optional shader-draw-parameters feature implied by SV_VertexID.
EdgePixel CadEdgeVertex(EdgeInput Edge, uint VertexIndex : SV_VulkanVertexID)
{
    EdgePixel Pixel;
    Pixel.Position = float4(-2, -2, 0, 1);
    Pixel.Distance = 1000;
    float4 Start = mul(Projection.ClipFromSurface, float4(Edge.Start, 1));
    float4 End = mul(Projection.ClipFromSurface, float4(Edge.End, 1));
    if (!ClipCadSegment(Start, End)) return Pixel;
    CadStrokeCorner Corner;
    if (!ExpandCadStroke(Start, End, VertexIndex, Projection.DisplayStroke.xy, Projection.DisplayStroke.z * 0.5 + 1.0, 0.0, Corner)) return Pixel;
    Pixel.Position = Corner.Position;
    Pixel.Distance = Corner.Distance;
    return Pixel;
}

[shader("fragment")]
float4 CadEdgeFragment(EdgePixel Pixel) : SV_Target
{
    float Radius = Projection.DisplayStroke.z * 0.5;
    float Coverage = CadStrokeCoverage(Pixel.Distance, Radius, Radius + 1.0);
    return float4(Projection.EdgeColour.rgb, Projection.EdgeColour.a * Coverage);
}
