/*====================================================================================================================================
                                                          SOLIDARCEDGE.SLANG
====================================================================================================================================*/
// 🧩 B-rep edges as antialiased pixel-width strokes (six corners per instanced segment) with the R32 identity output;
//    the x-ray program draws the occluded part of every edge as a thinner, faded stroke that stays pickable

#include "SolidArcProjection.slang"
#include "CadStroke.slang"                                                                   // Frontier stroke quad

[[vk::push_constant]] ConstantBuffer<SolidArcProjection> Projection;
[[vk::binding(1, 0)]] StructuredBuffer<uint> EdgeStates;
// 📝 Section (Alt+S): [0] = (n, w), the kept side n·P + w ≥ 0; [1] = (eye, enabled flag).
[[vk::binding(2, 0)]] StructuredBuffer<float4> Section;

//------------------------------------------------------------------------------------------------------------------------
//                                                    INPUT/OUTPUT STRUCTURES
//------------------------------------------------------------------------------------------------------------------------

struct EdgeSegment
{
    [[vk::location(0)]] float3 Start : POSITION0;                                            // [len] - World point (per instance)
    [[vk::location(1)]] float3 End   : POSITION1;                                            // [len] - World point (per instance)
    [[vk::location(2)]] uint   Edge  : BLENDINDICES0;                                        // [idx] - Scene edge (per instance)
};

struct EdgePixel
{
    float4                 Position : SV_Position;                                           // [-]  - Clip position
    noperspective float    Distance : TEXCOORD0;                                             // [px] - Signed distance from the centre line
    nointerpolation uint   Edge     : BLENDINDICES0;                                         // [idx] - Scene edge
};

struct EdgeTargets
{
    float4 Colour   : SV_Target0;                                                            // [0-1] - Display colour (blended)
    uint   Identity : SV_Target1;                                                            // [-]   - IdentityEdgeBit | edge + 1
};

//------------------------------------------------------------------------------------------------------------------------
//                                                    INTERNAL FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

float StrokeRadius(uint State)
{
    float Width = Projection.DisplayStroke.z;
    if ((State & (StateSelected | StateActive)) != 0u) Width *= 2.0;
    else if ((State & StateHovered) != 0u)             Width *= 1.6;
    return Width * 0.5;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    PROGRAMS
//------------------------------------------------------------------------------------------------------------------------

[shader("vertex")]
// 📝 Direct Vulkan indexing avoids the optional shader-draw-parameters feature implied by SV_VertexID.
EdgePixel SolidArcEdgeVertex(EdgeSegment Segment, uint VertexIndex : SV_VulkanVertexID)
{
    EdgePixel Pixel;
    Pixel.Position = float4(-2.0, -2.0, 0.0, 1.0);
    Pixel.Distance = 1000.0;
    Pixel.Edge     = Segment.Edge;
    // 📝 Section (Alt+S): the world segment is cut at the plane before projection (exact, unlike a per-pixel test).
    float3 Head = Segment.Start;
    float3 Tail = Segment.End;
    if (Section[1].w > 0.5)
    {
        const float HeadSide = dot(Section[0].xyz, Head) + Section[0].w;
        const float TailSide = dot(Section[0].xyz, Tail) + Section[0].w;
        if (HeadSide < 0.0 && TailSide < 0.0) return Pixel;
        const float3 Crossing = lerp(Head, Tail, HeadSide / (HeadSide - TailSide));
        if (HeadSide < 0.0) Head = Crossing;
        if (TailSide < 0.0) Tail = Crossing;
    }
    float4 Start = mul(Projection.ClipFromWorld, float4(Head, 1.0));
    float4 End   = mul(Projection.ClipFromWorld, float4(Tail, 1.0));
    // 📝 DisplayStroke.w < 0 (x-ray occluded draw): a world margin M = −w. The stroke takes its depth from the endpoint
    //    moved M toward the eye (along −Forward), so a stroke passes the GREATER test only when it lies more than M
    //    behind the face in front — the same world margin in orthographic and perspective views.
    const float  Margin    = max(-Projection.DisplayStroke.w, 0.0);
    const float4 StartNear = mul(Projection.ClipFromWorld, float4(Head - Projection.ViewForward.xyz * Margin, 1.0));
    const float4 EndNear   = mul(Projection.ClipFromWorld, float4(Tail - Projection.ViewForward.xyz * Margin, 1.0));
    // 📝 Frontier CadStroke: the same clipping and quad as upstream CadEdge, stretched by Reach along the segment too, so
    //    polyline joints overlap instead of leaving notches.
    if (!ClipCadSegment(Start, End)) return Pixel;
    const float     Reach = StrokeRadius(EdgeStates[Segment.Edge]) + 1.0;
    CadStrokeCorner Corner;
    if (!ExpandCadStroke(Start, End, VertexIndex, Projection.DisplayStroke.xy, Reach, Reach, Corner)) return Pixel;
    Pixel.Position = Corner.Position;
    // 📝 Edges win depth ties against their own faces: pull toward the eye by a fraction of the depth range.
    if (Projection.DisplayStroke.w >= 0.0)
        Pixel.Position.z = max(0.0, Pixel.Position.z - Projection.DisplayStroke.w * Pixel.Position.w);
    else
    {
        const float4 Near = Corner.EndCondition ? EndNear : StartNear;
        Pixel.Position.z  = clamp(Near.z / max(Near.w, 1.0e-6) * Pixel.Position.w, 0.0, Pixel.Position.w);
    }
    Pixel.Distance = Corner.Distance;
    return Pixel;
}

[shader("fragment")]
EdgeTargets SolidArcEdgeFragment(EdgePixel Pixel)
{
    uint  State    = EdgeStates[Pixel.Edge];
    float Radius   = StrokeRadius(State);
    float Coverage = CadStrokeCoverage(Pixel.Distance, Radius - 0.5, Radius + 0.75);
    if (Coverage < 0.35) discard;
    float3 Colour = float3(0.075, 0.085, 0.105);
    if ((State & StateHovered) != 0u)  Colour = HoverTint * 0.85;
    if ((State & StateSelected) != 0u) Colour = SelectedTint;
    if ((State & StateActive) != 0u)   Colour = ActiveTint;
    EdgeTargets Targets;
    Targets.Colour   = float4(Colour, Coverage);
    Targets.Identity = IdentityEdgeBit | (Pixel.Edge + 1u);
    return Targets;
}

// 📝 X-ray (Plasticity CurveEdge_XRay layer): drawn with depth test GREATER after the surface depth, so only the stroke
//    pixels hidden behind a face arrive here. Unselected strokes are a pale steel at 55 % and 0.7 × the width; selected,
//    active and hovered ones keep their tints at 85 %. The identity is written, so a hidden edge is hovered and picked
//    through the faces in front of it.
[shader("fragment")]
EdgeTargets SolidArcEdgeOccludedFragment(EdgePixel Pixel)
{
    uint  State    = EdgeStates[Pixel.Edge];
    float Radius   = StrokeRadius(State) * ((State & (StateSelected | StateActive | StateHovered)) != 0u ? 1.0 : 0.7);
    float Coverage = CadStrokeCoverage(Pixel.Distance, Radius - 0.5, Radius + 0.75);
    if (Coverage < 0.35) discard;
    float3 Colour  = float3(0.780, 0.830, 0.910);
    float  Opacity = 0.55;
    if ((State & StateHovered) != 0u)  { Colour = HoverTint;    Opacity = 0.85; }
    if ((State & StateSelected) != 0u) { Colour = SelectedTint; Opacity = 0.85; }
    if ((State & StateActive) != 0u)   { Colour = ActiveTint;   Opacity = 0.85; }
    EdgeTargets Targets;
    Targets.Colour   = float4(Colour, Coverage * Opacity);
    Targets.Identity = IdentityEdgeBit | (Pixel.Edge + 1u);
    return Targets;
}
