/*====================================================================================================================================
                                                      DRAFTINGEDGE.SLANG
====================================================================================================================================*/
// 🧩 B-rep edges as antialiased pixel-width strokes (six corners per instanced segment) with the R32 identity output;
//    the x-ray program draws the occluded part of every edge as a thinner, faded stroke that stays pickable

#include "DraftingProjection.slang"

[[vk::push_constant]] ConstantBuffer<DraftingProjection> Projection;
[[vk::binding(1, 0)]] StructuredBuffer<uint> EdgeStates;

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

// Clips the homogeneous segment against one plane (distance ≥ 0 kept); false when both ends are outside.
bool ClipSegment(inout float4 Start, inout float4 End, float StartDistance, float EndDistance)
{
    if (StartDistance < 0.0 && EndDistance < 0.0) return false;
    if (StartDistance < 0.0 || EndDistance < 0.0)
    {
        float4 Crossing = lerp(Start, End, StartDistance / (StartDistance - EndDistance));
        if (StartDistance < 0.0) Start = Crossing;
        else End = Crossing;
    }
    return true;
}

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
EdgePixel DraftingEdgeVertex(EdgeSegment Segment, uint VertexIndex : SV_VulkanVertexID)
{
    EdgePixel Pixel;
    Pixel.Position = float4(-2.0, -2.0, 0.0, 1.0);
    Pixel.Distance = 1000.0;
    Pixel.Edge     = Segment.Edge;
    float4 Start = mul(Projection.ClipFromWorld, float4(Segment.Start, 1.0));
    float4 End   = mul(Projection.ClipFromWorld, float4(Segment.End, 1.0));
    // 📝 DisplayStroke.w < 0 (x-ray occluded draw): a world margin M = −w. The stroke takes its depth from the endpoint
    //    moved M toward the eye (along −Forward), so a stroke passes the GREATER test only when it lies more than M
    //    behind the face in front — the same world margin in orthographic and perspective views.
    const float  Margin    = max(-Projection.DisplayStroke.w, 0.0);
    const float4 StartNear = mul(Projection.ClipFromWorld, float4(Segment.Start - Projection.ViewForward.xyz * Margin, 1.0));
    const float4 EndNear   = mul(Projection.ClipFromWorld, float4(Segment.End - Projection.ViewForward.xyz * Margin, 1.0));
    // 📝 Clip before the perspective division, including the near-zero homogeneous denominator.
    if (!ClipSegment(Start, End, Start.w - 1.0e-5, End.w - 1.0e-5)) return Pixel;
    if (!ClipSegment(Start, End, Start.z, End.z)) return Pixel;
    static const uint Corners[6] = { 0u, 1u, 2u, 2u, 1u, 3u };
    uint   Corner    = Corners[VertexIndex];
    float2 Direction = (End.xy / End.w - Start.xy / Start.w) * Projection.DisplayStroke.xy * 0.5;
    float  Length    = length(Direction);
    if (Length < 1.0e-5) return Pixel;
    float2 Across = float2(-Direction.y, Direction.x) / Length;
    float  Side   = (Corner == 1u || Corner == 3u) ? 1.0 : -1.0;
    float  Reach  = StrokeRadius(EdgeStates[Segment.Edge]) + 1.0;
    // 📝 Stretch the quad by Reach along the segment too, so polyline joints overlap instead of leaving notches.
    float2 Along  = Direction / Length * (Corner < 2u ? -Reach : Reach);
    Pixel.Position     = Corner < 2u ? Start : End;
    Pixel.Position.xy += (Across * Side * Reach + Along) * 2.0 / Projection.DisplayStroke.xy * Pixel.Position.w;
    // 📝 Edges win depth ties against their own faces: pull toward the eye by a fraction of the depth range.
    if (Projection.DisplayStroke.w >= 0.0)
        Pixel.Position.z = max(0.0, Pixel.Position.z - Projection.DisplayStroke.w * Pixel.Position.w);
    else
    {
        const float4 Near = Corner < 2u ? StartNear : EndNear;
        Pixel.Position.z  = clamp(Near.z / max(Near.w, 1.0e-6) * Pixel.Position.w, 0.0, Pixel.Position.w);
    }
    Pixel.Distance     = Side * Reach;
    return Pixel;
}

[shader("fragment")]
EdgeTargets DraftingEdgeFragment(EdgePixel Pixel)
{
    uint  State    = EdgeStates[Pixel.Edge];
    float Radius   = StrokeRadius(State);
    float Coverage = 1.0 - smoothstep(Radius - 0.5, Radius + 0.75, abs(Pixel.Distance));
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
EdgeTargets DraftingEdgeOccludedFragment(EdgePixel Pixel)
{
    uint  State    = EdgeStates[Pixel.Edge];
    float Radius   = StrokeRadius(State) * ((State & (StateSelected | StateActive | StateHovered)) != 0u ? 1.0 : 0.7);
    float Coverage = 1.0 - smoothstep(Radius - 0.5, Radius + 0.75, abs(Pixel.Distance));
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
