/*==========================================================================================================================================
                                                     SPARK AND EFFECT RASTER.SLANG
                                     ------------------------------------------------------------------
                   Screen-aligned motion-blurred additive spark streaks (SparkRaster.wgsl) and
                     closed-form ballistic Derez / Rez / Glitch Dissolve cubes (EffectRecords.wgsl
                     + EffectRaster.wgsl).
==========================================================================================================================================*/

//------------------------------------------------------------------------------------------------------------------------
//                                                    1. SPARK STREAK RASTER
//------------------------------------------------------------------------------------------------------------------------

struct SparkFrameUniforms
{
    float4x4 UnjitteredViewProjection;
    float4x4 PreviousUnjitteredViewProjection;
    float4x4 View;
    float2   ScreenExtent;
    float    EmissionScale;
    float    Padding;
};

[[vk::binding(0, 0)]] ConstantBuffer<SparkFrameUniforms> SparkFrame;

struct SparkInstanceInput
{
    [[vk::location(0)]] float4 Head     : TEXCOORD0; // xyz world head, w width [m]
    [[vk::location(1)]] float4 Tail     : TEXCOORD1; // xyz world tail, w pixel-width factor
    [[vk::location(2)]] float4 Radiance : TEXCOORD2; // rgb blackbody radiance, w shutter fraction
};

struct SparkVarying
{
    float4 Position : SV_Position;
    float3 Radiance : TEXCOORD0;
    float2 Profile  : TEXCOORD1; // x across [-1, 1], y along [0 tail, 1 head]
    float2 Shape    : TEXCOORD2; // x pixel-width factor, y shutter fraction
};

float2 ProjectSparkPixels(float4 Clip, float2 ScreenExtent)
{
    return (Clip.xy / Clip.w) * 0.5 * ScreenExtent;
}

[shader("vertex")]
SparkVarying SparkVertex(uint VertexIndex : SV_VertexID, SparkInstanceInput Input)
{
    SparkVarying Varying;
    Varying.Radiance = float3(0.0, 0.0, 0.0);
    Varying.Profile  = float2(0.0, 0.0);
    Varying.Shape    = float2(1.0, 1.0);

    float4 HeadClip = mul(SparkFrame.UnjitteredViewProjection, float4(Input.Head.xyz, 1.0));
    float4 TailClip = mul(SparkFrame.PreviousUnjitteredViewProjection, float4(Input.Tail.xyz, 1.0));
    if (HeadClip.w < 0.05)
    {
        Varying.Position = float4(0.0, 0.0, -2.0, 1.0);
        return Varying;
    }
    if (TailClip.w < 0.05)
    {
        TailClip = HeadClip;
    }

    float3 Up = float3(SparkFrame.View[0][1], SparkFrame.View[1][1], SparkFrame.View[2][1]);
    float2 HeadPixels = ProjectSparkPixels(HeadClip, SparkFrame.ScreenExtent);
    float2 TailPixels = ProjectSparkPixels(TailClip, SparkFrame.ScreenExtent);

    float4 OffsetClip = mul(SparkFrame.UnjitteredViewProjection, float4(Input.Head.xyz + Up * Input.Head.w, 1.0));
    float Width       = clamp(length(ProjectSparkPixels(OffsetClip, SparkFrame.ScreenExtent) - HeadPixels), 0.65, 1.35);
    float PixelWidth  = (Input.Tail.w > 0.0) ? clamp(Input.Tail.w, 0.35, 1.0) : 1.0;
    float Shutter     = (Input.Radiance.w > 0.0) ? clamp(Input.Radiance.w, 0.15, 1.0) : 1.0;
    TailPixels        = lerp(HeadPixels, TailPixels, Shutter);

    float2 Along   = HeadPixels - TailPixels;
    float  Longest = 0.2 * SparkFrame.ScreenExtent.y;
    float  RawLen  = length(Along);
    if (RawLen > Longest)
    {
        TailPixels = HeadPixels - Along * (Longest / RawLen);
        Along      = HeadPixels - TailPixels;
        RawLen     = Longest;
    }
    Along = (RawLen > 1.0e-3) ? (Along / max(RawLen, 1.0e-4)) : float2(1.0, 0.0);
    float2 Across = float2(-Along.y, Along.x);

    static const float2 Corners[6] = {
        float2(0.0, -1.0), float2(1.0, -1.0), float2(1.0, 1.0),
        float2(0.0, -1.0), float2(1.0,  1.0), float2(0.0, 1.0)
    };
    float2 Corner     = Corners[VertexIndex % 6u];
    float2 BasePixels = (Corner.x > 0.5) ? HeadPixels : TailPixels;
    float2 Offset     = Across * Corner.y * Width * PixelWidth * lerp(0.22, 1.0, Corner.x) * 0.5
                      + Along  * (Corner.x - 0.5) * Width * PixelWidth;
    float2 Pixels     = BasePixels + Offset;

    Varying.Position = float4((Pixels / (0.5 * SparkFrame.ScreenExtent)) * HeadClip.w, HeadClip.z, HeadClip.w);
    float Spread     = Width / max(RawLen + Width, Width);
    Varying.Radiance = Input.Radiance.rgb * lerp(0.35, 1.0, Spread) * SparkFrame.EmissionScale;
    Varying.Profile  = float2(Corner.y, Corner.x);
    Varying.Shape    = float2(PixelWidth, Shutter);
    return Varying;
}

[shader("fragment")]
float4 SparkFragment(SparkVarying Varying) : SV_Target0
{
    float Across = pow(max(0.0, 1.0 - Varying.Profile.x * Varying.Profile.x), 2.0);
    float Head   = lerp(lerp(0.08, 0.35, Varying.Shape.y), 1.0, pow(Varying.Profile.y, lerp(0.7, 1.8, Varying.Shape.x)));
    return float4(Varying.Radiance * Across * Head, 0.0);
}

//------------------------------------------------------------------------------------------------------------------------
//                                             2. DEREZ / REZ CLOSED-FORM CUBES
//------------------------------------------------------------------------------------------------------------------------

static const uint  EffectMaskOff     = 0u;
static const uint  EffectMaskDerez   = 1u;
static const uint  EffectMaskRez     = 2u;
static const uint  EffectMaskHidden  = 3u;
static const float EffectGravity     = 9.81;

struct EffectEmitter
{
    float4x4 BodyToWorld;
    float4   Velocity; // xyz [m/s], w start clock [s]
    float4   Angular;  // xyz [rad/s], w kind (0..4, 255 inactive)
    float4   Extent;   // xyz half-extents [m], w particle count
    float4   Centre;   // xyz local centre [m], w seed
};

struct EffectRecord
{
    float         Clock;
    uint          MaskMode;
    float         MaskStart;
    uint          Style;
    float4        Minimum;    // xyz vehicle local min [m], w hold offset [s]
    float4        Maximum;    // xyz vehicle local max [m], w frame duration [s]
    float4        Kinematics; // x burst [m/s], y speed min, z speed max, w drag [1/s]
    float4        Timing;     // x life [s], y size [m], z wave [s], w flight [s]
    float4        Colour;     // x face-colour flag, y face glow, z pose count, w pose start clock [s]
    EffectEmitter Emitters[8];
};

float HashEffect(uint Seed)
{
    uint State = Seed * 747796405u + 2891336453u;
    uint Word  = ((State >> ((State >> 28u) + 4u)) ^ State) * 277803737u;
    return float((Word >> 22u) ^ Word) / 4294967296.0;
}

float3 EvaluateEffectBallistic(float3 Origin, float3 Velocity, float Drag, float Time)
{
    float SafeDrag  = max(Drag, 1.0e-4);
    float3 Terminal = float3(0.0, 0.0, -EffectGravity) / SafeDrag;
    return Origin + Terminal * Time + (Velocity - Terminal) * ((1.0 - exp(-SafeDrag * Time)) / SafeDrag);
}
