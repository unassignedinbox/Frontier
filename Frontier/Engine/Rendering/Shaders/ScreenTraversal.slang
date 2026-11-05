/*==========================================================================================================================================
                                                         SCREENTRAVERSAL.SLANG
                                     ------------------------------------------------------------------
                   Lumen-style stackless screen-space ray walk over the NearestDepth pyramid
                     ahead of the scene distance field — 1:1 Slang counterpart of ScreenTraversal.wgsl.
==========================================================================================================================================*/

static const float ScreenSinkTolerance     = 0.0015;  // [-] relative depth a grazing ray may sink below a tile and still pass over it
static const float ScreenRelativeThickness = 0.02;    // [-] relative depth behind a surface that still counts as striking it
static const float ScreenAbsoluteThickness = 0.03;    // [m] floor of that thickness near the eye
static const float ScreenExitanceAgreement = 0.02;    // [-] relative depth agreement that confirms a point in a depth lattice
static const float ScreenUnreachedTravel   = 1.0e30;  // [-] parameter of a boundary the walk can never reach

struct ScreenHit
{
    float3 Position;  // [m] on the ray, at the struck surface
    float  Travel;    // [m] ray parameter at the strike, or at the last point the screen proved unoccluded
    float3 Normal;    // [-] shading normal of the struck pixel
    uint   Identity;  // [-] occupant identity of the struck pixel
    bool   Hit;
};

float3 ProjectScreenPoint(float4x4 ViewProjection, float3 Position)
{
    float4 Clip = mul(ViewProjection, float4(Position, 1.0));
    float3 Ndc  = Clip.xyz / Clip.w;
    return float3(Ndc.x * 0.5 + 0.5, 0.5 - Ndc.y * 0.5, Ndc.z);
}

float3 ReconstructScreenPoint(float4x4 InverseViewProjection, float3 Point)
{
    float4 Clip = mul(InverseViewProjection, float4(Point.x * 2.0 - 1.0, 1.0 - Point.y * 2.0, Point.z, 1.0));
    return Clip.xyz / Clip.w;
}

float ResolveScreenBias(float ViewDepth)
{
    return 0.004 + 0.002 * ViewDepth;
}

float3 DecodeOctahedralScreenNormal(float2 Encoded)
{
    float2 Mapped = Encoded * 2.0 - 1.0;
    float3 Normal = float3(Mapped.xy, 1.0 - abs(Mapped.x) - abs(Mapped.y));
    float Fold = saturate(-Normal.z);
    Normal.x += (Normal.x >= 0.0) ? -Fold : Fold;
    Normal.y += (Normal.y >= 0.0) ? -Fold : Fold;
    return normalize(Normal);
}

ScreenHit TraverseScreenDepth(
    Texture2D<float>  NearestDepth,
    Texture2D<float4> ScreenAttributes,
    float4x4          View,
    float4x4          ViewProjection,
    float4x4          InverseViewProjection,
    float             CameraNear,
    uint              SkyIdentity,
    float3            Origin,
    float3            Direction,
    float             MaximumTravel,
    uint              StepLimit)
{
    ScreenHit Result;
    Result.Hit      = false;
    Result.Travel   = 0.0;
    Result.Position = Origin;
    Result.Normal   = -Direction;
    Result.Identity = SkyIdentity;

    float OriginDepth = -mul(View, float4(Origin, 1.0)).z;
    float DepthRate   = -mul(View, float4(Direction, 0.0)).z;
    float Reach       = MaximumTravel;
    if (DepthRate < 0.0)
    {
        Reach = min(Reach, (OriginDepth - 2.0 * CameraNear) / -DepthRate);
    }
    if (OriginDepth <= 2.0 * CameraNear || Reach <= 0.0)
    {
        return Result;
    }

    float3 Start = ProjectScreenPoint(ViewProjection, Origin);
    if (any(Start.xy <= float2(0.0, 0.0)) || any(Start.xy >= float2(1.0, 1.0)))
    {
        return Result;
    }

    float3 Projected = ProjectScreenPoint(ViewProjection, Origin + Direction * Reach) - Start;
    float Exit = 1.0;
    [unroll]
    for (uint Axis = 0u; Axis < 2u; ++Axis)
    {
        if (Projected[Axis] > 0.0)
        {
            Exit = min(Exit, (1.0 - Start[Axis]) / Projected[Axis]);
        }
        else if (Projected[Axis] < 0.0)
        {
            Exit = min(Exit, -Start[Axis] / Projected[Axis]);
        }
    }

    float3 Delta = Projected * Exit;

    uint BaseW = 0u;
    uint BaseH = 0u;
    uint MipLevels = 1u;
    NearestDepth.GetDimensions(0, BaseW, BaseH, MipLevels);
    float2 BaseExtent = float2(float(BaseW), float(BaseH));
    int LevelCount    = int(MipLevels);

    bool3 Moving   = abs(Delta) > float3(1.0e-12, 1.0e-12, 1.0e-12);
    float3 SafeD   = float3(
        Moving.x ? Delta.x : 1.0,
        Moving.y ? Delta.y : 1.0,
        Moving.z ? Delta.z : 1.0);
    float3 Inverse = float3(
        Moving.x ? (1.0 / SafeD.x) : ScreenUnreachedTravel,
        Moving.y ? (1.0 / SafeD.y) : ScreenUnreachedTravel,
        Moving.z ? (1.0 / SafeD.z) : ScreenUnreachedTravel);

    bool2 Receding  = bool2(Delta.x < 0.0 && Moving.x, Delta.y < 0.0 && Moving.y);
    float2 ExitSide = float2(Receding.x ? 0.0 : 1.0, Receding.y ? 0.0 : 1.0);
    float2 Nudge    = float2(Receding.x ? -0.005 : 0.005, Receding.y ? -0.005 : 0.005) / BaseExtent;

    float2 StartPixel    = floor(Start.xy * BaseExtent);
    float2 FirstCrossing = ((StartPixel + ExitSide) / BaseExtent + Nudge - Start.xy) * Inverse.xy;
    float Travel         = min(FirstCrossing.x, FirstCrossing.y);

    int Level      = 0;
    bool Descended = false;
    for (uint Iteration = 0u; Iteration < StepLimit; ++Iteration)
    {
        if (Travel > 1.0)
        {
            break;
        }
        float3 Point      = Start + Delta * Travel;
        float2 Resolution = BaseExtent * exp2(-float(Level));
        float2 Cell       = floor(Point.xy * Resolution);

        uint LevelW = max(1u, BaseW >> uint(Level));
        uint LevelH = max(1u, BaseH >> uint(Level));
        int2 Texel  = clamp(int2(Cell), int2(0, 0), int2(int(LevelW) - 1, int(LevelH) - 1));
        float Tolerated = NearestDepth.Load(int3(Texel, Level)).r * (1.0 - ScreenSinkTolerance);

        float2 Crossing = ((Cell + ExitSide) / Resolution + Nudge - Start.xy) * Inverse.xy;
        float Boundary  = min(Crossing.x, Crossing.y);
        float Sinking   = ScreenUnreachedTravel;
        if (Delta.z < 0.0 && Moving.z)
        {
            Sinking = (Tolerated - Start.z) * Inverse.z;
        }
        bool InFront = Point.z > Tolerated;
        if (InFront)
        {
            Travel = min(Boundary, Sinking);
        }
        if (InFront && Boundary < Sinking)
        {
            Level = min(Level + 1, LevelCount - 1);
        }
        else
        {
            --Level;
            if (Level < 0)
            {
                Descended = true;
                break;
            }
        }
    }

    float3 Settled = Start + Delta * saturate(Travel);
    Result.Travel  = max(dot(ReconstructScreenPoint(InverseViewProjection, Settled) - Origin, Direction), 0.0);
    if (!Descended)
    {
        return Result;
    }

    int2 Pixel = int2(floor(Settled.xy * BaseExtent));
    if (any(Pixel < int2(0, 0)) || any(Pixel >= int2(BaseExtent)) || all(abs(Pixel - int2(StartPixel)) <= int2(1, 1)))
    {
        return Result;
    }

    float SurfaceDepth = NearestDepth.Load(int3(Pixel, 0)).r;
    if (SurfaceDepth <= 0.0)
    {
        return Result;
    }

    float4 Attributes = ScreenAttributes.Load(int3(Pixel, 0));
    float3 Normal     = DecodeOctahedralScreenNormal(Attributes.xy);
    if (dot(Normal, Direction) >= 0.0)
    {
        return Result;
    }

    float SurfaceView = CameraNear / SurfaceDepth;
    float RayView     = CameraNear / max(Settled.z, 1.0e-12);
    if (RayView - SurfaceView > max(ScreenRelativeThickness * SurfaceView, ScreenAbsoluteThickness))
    {
        return Result;
    }

    Result.Hit      = true;
    Result.Position = Origin + Direction * Result.Travel;
    Result.Normal   = Normal;
    Result.Identity = uint(Attributes.w + 0.5);
    return Result;
}
