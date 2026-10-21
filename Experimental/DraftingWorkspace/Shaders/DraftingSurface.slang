/*====================================================================================================================================
                                                     DRAFTINGSURFACE.SLANG
====================================================================================================================================*/
// 🧩 B-rep face shading (studio matcap computed analytically) with selection tints and the R32 identity output

#include "DraftingProjection.slang"

[[vk::push_constant]] ConstantBuffer<DraftingProjection> Projection;
[[vk::binding(0, 0)]] StructuredBuffer<uint> FaceStates;

//------------------------------------------------------------------------------------------------------------------------
//                                                    INPUT/OUTPUT STRUCTURES
//------------------------------------------------------------------------------------------------------------------------

struct SurfaceCorner
{
    [[vk::location(0)]] float3 Position : POSITION0;                                         // [len] - World point
    [[vk::location(1)]] float3 Normal   : NORMAL0;                                           // [-]   - Outward unit normal
    [[vk::location(2)]] uint   Face     : BLENDINDICES0;                                     // [idx] - Scene face
};

struct SurfacePixel
{
    float4                 Position : SV_Position;                                           // [-]   - Clip position
    float3                 Normal   : NORMAL0;                                               // [-]   - World normal
    nointerpolation uint   Face     : BLENDINDICES0;                                         // [idx] - Scene face
};

struct SurfaceTargets
{
    float4 Colour   : SV_Target0;                                                            // [0-1] - Display colour
    uint   Identity : SV_Target1;                                                            // [-]   - Face index + 1
};

//------------------------------------------------------------------------------------------------------------------------
//                                                    PROGRAMS
//------------------------------------------------------------------------------------------------------------------------

[shader("vertex")]
SurfacePixel DraftingSurfaceVertex(SurfaceCorner Corner)
{
    SurfacePixel Pixel;
    Pixel.Position = mul(Projection.ClipFromWorld, float4(Corner.Position, 1.0));
    Pixel.Normal   = Corner.Normal;
    Pixel.Face     = Corner.Face;
    return Pixel;
}

// 📝 View-space normal → studio lighting: soft key from the upper left, cool fill, hemispherical ambient and a rim term.
//    Back faces (open sheets, inside views) are shaded darker so the inside of a body never reads as its outside.
[shader("fragment")]
SurfaceTargets DraftingSurfaceFragment(SurfacePixel Pixel, bool FrontFacing : SV_IsFrontFace)
{
    float3 Normal = normalize(Pixel.Normal) * (FrontFacing ? 1.0 : -1.0);
    float3 View   = float3(dot(Normal, Projection.ViewRight.xyz), dot(Normal, Projection.ViewUp.xyz), -dot(Normal, Projection.ViewForward.xyz));
    float  Key    = saturate(dot(View, normalize(float3(-0.45, 0.55, 0.70))));
    float  Fill   = saturate(dot(View, normalize(float3(0.60, -0.10, 0.80))));
    float  Sky    = 0.5 + 0.5 * View.y;
    float  Rim    = pow(1.0 - saturate(View.z), 3.0);
    float3 Albedo = float3(0.70, 0.72, 0.76);
    float3 Colour = Albedo * (0.22 + 0.18 * Sky) + Albedo * Key * 0.62 + float3(0.62, 0.70, 0.82) * Fill * 0.16 + Rim * 0.10;
    Colour       += pow(saturate(dot(reflect(float3(0.0, 0.0, -1.0), View), normalize(float3(-0.45, 0.55, 0.70)))), 24.0) * 0.18;
    // 📝 Zebra (vehicle plan V8, ViewUp.w = stripe count): the view direction reflected in the normal, its angle to the
    //    camera up × count / π; stripe k covers phase [k, k + ½). Mirrors DraftingZebraPhase (DraftingScene.cpp).
    if (Projection.ViewUp.w > 0.0)
    {
        float3 Forward   = Projection.ViewForward.xyz;
        float3 Reflected = Forward - 2.0 * dot(Forward, Normal) * Normal;
        float  Phase     = Projection.ViewUp.w * acos(clamp(dot(Reflected, Projection.ViewUp.xyz), -1.0, 1.0)) / 3.14159265;
        float  Wave      = abs(frac(Phase) - 0.5) * 2.0;                                     // 1 at phase k, 0 at k + ½
        float  Blur      = max(fwidth(Phase) * 2.0, 1.0e-4);                                 // Pixel-wide anti-aliasing
        float  Light     = smoothstep(0.5 - Blur, 0.5 + Blur, Wave);
        Colour = lerp(float3(0.05, 0.05, 0.06), float3(0.96, 0.96, 0.97), Light) * (0.80 + 0.20 * Key);
    }

    uint State = FaceStates[Pixel.Face];
    if ((State & StateHovered) != 0u)  Colour = lerp(Colour, HoverTint, 0.28);
    if ((State & StateSelected) != 0u) Colour = lerp(Colour, SelectedTint * (0.55 + 0.45 * Key), 0.62);
    if ((State & StateActive) != 0u)   Colour = lerp(Colour, ActiveTint * (0.55 + 0.45 * Key), 0.62);
    if (!FrontFacing)                  Colour = Colour * float3(0.55, 0.42, 0.42);

    SurfaceTargets Targets;
    Targets.Colour   = float4(saturate(Colour), 1.0);
    Targets.Identity = Pixel.Face + 1u;
    return Targets;
}
