/*====================================================================================================================================
                                                          SOLIDARCGRID.SLANG
====================================================================================================================================*/
// 🧩 Analytic grid on the active drawing plane (adaptive decades, world-axis colours), depth-tested behind the model

#include "SolidArcProjection.slang"

[[vk::push_constant]] ConstantBuffer<SolidArcGridProjection> Projection;

//------------------------------------------------------------------------------------------------------------------------
//                                                    INPUT/OUTPUT STRUCTURES
//------------------------------------------------------------------------------------------------------------------------

struct GridPixel
{
    float4 Position : SV_Position;                                                           // [-] - Clip position
    float2 Device   : TEXCOORD0;                                                             // [-] - Normalised device xy
};

struct GridTargets
{
    float4 Colour : SV_Target0;                                                              // [0-1] - Blended over the scene
    float  Depth  : SV_Depth;                                                                // [0-1] - Depth of the plane point
};

//------------------------------------------------------------------------------------------------------------------------
//                                                    INTERNAL FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

// Line coverage of a periodic grid of spacing Spacing at Point (fwidth-scaled so lines stay ~1 px wide).
float GridCoverage(float2 Point, float Spacing)
{
    float2 Scaled = Point / Spacing;
    float2 Width  = fwidth(Scaled);
    float2 Line   = abs(frac(Scaled - 0.5) - 0.5) / max(Width, float2(1.0e-6, 1.0e-6));
    return 1.0 - saturate(min(Line.x, Line.y));
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    PROGRAMS
//------------------------------------------------------------------------------------------------------------------------

[shader("vertex")]
GridPixel SolidArcGridVertex(uint VertexIndex : SV_VulkanVertexID)
{
    // 📝 One triangle covering the screen: (−1, −1), (3, −1), (−1, 3).
    float2 Device = float2((VertexIndex == 1u) ? 3.0 : -1.0, (VertexIndex == 2u) ? 3.0 : -1.0);
    GridPixel Pixel;
    Pixel.Position = float4(Device, 0.5, 1.0);
    Pixel.Device   = Device;
    return Pixel;
}

[shader("fragment")]
GridTargets SolidArcGridFragment(GridPixel Pixel)
{
    float4 Near = mul(Projection.PlaneFromClip, float4(Pixel.Device, 0.0, 1.0));
    float4 Far  = mul(Projection.PlaneFromClip, float4(Pixel.Device, 1.0, 1.0));
    float3 From = Near.xyz / Near.w;
    float3 To   = Far.xyz / Far.w;
    float  Run  = To.z - From.z;
    // 📝 No discard before fwidth: derivatives need every lane of the 2 × 2 quad alive, so misses are dropped at the end.
    float  Reach    = abs(Run) < 1.0e-9 ? -1.0 : -From.z / Run;
    bool   HitCondition = Reach >= 0.0 && Reach <= 1.0;
    float3 Ground   = lerp(From, To, saturate(Reach));

    // Device depth where the homogeneous n coordinate vanishes (linear in device z).
    float DepthRun = Far.z - Near.z;
    float Depth    = abs(DepthRun) < 1.0e-12 ? 2.0 : -Near.z / DepthRun;
    float Weight   = lerp(Near.w, Far.w, saturate(Depth));

    // 📝 Adaptive decades: the minor spacing is the power of ten that keeps cells ≥ ~8 px; the next decade fades in.
    float2 Footprint = fwidth(Ground.xy);
    float  FootprintPeak = max(max(Footprint.x, Footprint.y), 1.0e-6);
    float  Decade    = log10(FootprintPeak * 8.0);
    float  Minor     = pow(10.0, floor(Decade));
    float  Blend     = frac(Decade);
    float  MinorLine = GridCoverage(Ground.xy, Minor) * (1.0 - Blend);
    float  MajorLine = GridCoverage(Ground.xy, Minor * 10.0);
    float  Line      = max(MinorLine * 0.45, MajorLine * 0.8);

    float3 Colour = float3(0.62, 0.66, 0.74);
    float  Alpha  = Line * 0.32;
    float2 AxisWidth = Footprint * 1.5;
    if (abs(Ground.y) < AxisWidth.y) { Colour = Projection.AlongU.rgb; Alpha = 0.85; }       // axis along u (v = 0)
    if (abs(Ground.x) < AxisWidth.x) { Colour = Projection.AlongV.rgb; Alpha = 0.85; }       // axis along v (u = 0)

    // 📝 Fade over the far part of the near → far range so the horizon never aliases.
    float Fade = saturate(1.0 - Reach * 2.5);
    if (!HitCondition || Depth < 0.0 || Depth > 1.0 || Weight <= 0.0 || Alpha * Fade < 0.004) discard;

    GridTargets Targets;
    Targets.Colour = float4(Colour, Alpha * Fade);
    Targets.Depth  = saturate(Depth);
    return Targets;
}
