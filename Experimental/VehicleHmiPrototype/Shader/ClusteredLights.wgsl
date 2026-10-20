//============================================================================================================================================
//                                                          CLUSTEREDLIGHTS.WGSL
//============================================================================================================================================
// 🧩 Two clustered-light systems over one world-space grid: REAL luminaires (point, spot and measured IES profiles) and
//    EMISSIVE proxies (the glowing sparks of hot surfaces). Each shading point reads only its own cell in each system.

// 📝 Kinds (PositionType.w): 0 point · 1 street lamp (IES) · 2 low beam (IES) · 3 high beam (spot) · 4 fog (spot) ·
//    5 roof flood (spot) · 6 generic spot · 7 generic IES. Emissive proxies are points.
//    Params: x = inner cone cosine, y = outer cone cosine (spots), w = active.
//    Profile: the IES candela-against-angle table for the light's axis, 32 samples over 0…180° normalised to the peak.

const IesSampleCount : u32 = 32u;

struct LightRecord {
    PositionType    : vec4f,            // xyz = world position, w = kind
    DirectionReach  : vec4f,            // xyz = forward axis, w = reach radius [m]
    ColourIntensity : vec4f,            // rgb = linear colour, w = intensity [cd]
    Params          : vec4f,            // x = inner cone cos, y = outer cone cos, z = unused, w = active
    Profile         : array<vec4f, 8>,  // IES samples 0…31 (4 per vec4f)
}

// 📝 Cluster assignment (written every frame by LightingSystem.AssignClusters). Header: element 0 carries the grid
//    resolution and the real per-cell cap, 1–2 the bitcast minimum and inverse extent. Then REAL cells from element 3 and
//    EMISSIVE cells from element 3 + CellCount; each element is an absolute (offset, count) pair into ClusterIndices.
//    Positions outside the grid clamp to its edge; the reach window in EvaluateLightArrival still attenuates distant lights.
const RealCellBase     : u32 = 3u;
const CellCount        : u32 = 1024u;
const EmissiveCellBase : u32 = 3u + 1024u;

// 📐 The light-index range of the cell holding a world position, in the cell table starting at CellBase.
fn ResolveClusterRangeAt(Cells: ptr<storage, array<vec4u>, read>, Position: vec3f, CellBase: u32) -> vec2u
{
    let Resolution = (*Cells)[0].xyz;
    let Minimum = vec3f(bitcast<f32>((*Cells)[1].x), bitcast<f32>((*Cells)[1].y), bitcast<f32>((*Cells)[1].z));
    let InverseExtent = vec3f(bitcast<f32>((*Cells)[2].x), bitcast<f32>((*Cells)[2].y), bitcast<f32>((*Cells)[2].z));
    let Scaled = clamp((Position - Minimum) * InverseExtent, vec3f(0.0), vec3f(1.0));
    let Cell = min(vec3u(Scaled * vec3f(Resolution)), Resolution - vec3u(1u));
    let Index = (Cell.z * Resolution.y + Cell.y) * Resolution.x + Cell.x;
    return (*Cells)[CellBase + Index].xy;
}

// 📐 Real luminaires: the cell range of the position.
fn ResolveClusterRange(Cells: ptr<storage, array<vec4u>, read>, Position: vec3f) -> vec2u
{
    return ResolveClusterRangeAt(Cells, Position, RealCellBase);
}

// 📐 Emissive proxies: the cell range of the position in the second system.
fn ResolveEmissiveClusterRange(Cells: ptr<storage, array<vec4u>, read>, Position: vec3f) -> vec2u
{
    return ResolveClusterRangeAt(Cells, Position, EmissiveCellBase);
}

// 📐 Measured photometry: linear interpolation of the light's IES candela table at an angle from its axis (0…π).
fn ResolveIesCandela(Light: LightRecord, Angle: f32) -> f32
{
    let T = clamp(Angle * f32(IesSampleCount - 1u) / 3.14159265, 0.0, f32(IesSampleCount - 1u));
    let Low  = u32(floor(T));
    let High = min(Low + 1u, IesSampleCount - 1u);
    let Fraction = T - f32(Low);
    let A = Light.Profile[Low / 4u][Low % 4u];
    let B = Light.Profile[High / 4u][High % 4u];
    return mix(A, B, Fraction);
}

// 📐 Angular distribution of a light, as a fraction of its peak, for a unit direction from the light to the point.
fn EvaluateLightProfile(Light: LightRecord, FromLight: vec3f) -> f32
{
    let Kind = u32(Light.PositionType.w + 0.5);
    if (Kind == 0u)
    {
        return 1.0;
    }
    let CosTheta = dot(FromLight, normalize(Light.DirectionReach.xyz));
    if (Kind == 1u || Kind == 2u || Kind == 7u)
    {
        return ResolveIesCandela(Light, acos(clamp(CosTheta, -1.0, 1.0)));
    }
    // 📝 Spots (3, 4, 5, 6): a smooth cone between the outer and inner cosines.
    return smoothstep(Light.Params.y, Light.Params.x, CosTheta);
}

// 📐 Smooth, physical inverse-square illuminance with windowed reach, shaped by the light's distribution.
fn EvaluateLightArrival(Light: LightRecord, Position: vec3f, Normal: vec3f) -> vec3f {
    if (Light.Params.w < 0.5) {
        return vec3f(0.0);
    }

    let LightPos = Light.PositionType.xyz;
    let Reach = Light.DirectionReach.w;
    let Toward = LightPos - Position;
    let D2 = dot(Toward, Toward);

    if (D2 >= Reach * Reach || Reach <= 0.0) {
        return vec3f(0.0);
    }

    let Distance = sqrt(D2);
    let LightDir = Toward / max(Distance, 1.0e-4);
    let NdotL = max(dot(Normal, LightDir), 0.0);

    if (NdotL <= 0.0) {
        return vec3f(0.0);
    }

    // Window function: C2 continuous polynomial smoothly dropping to 0 at Reach
    let Ratio = D2 / (Reach * Reach);
    let Window = (1.0 - Ratio * Ratio) * (1.0 - Ratio * Ratio);

    let Profile = EvaluateLightProfile(Light, -LightDir);
    if (Profile <= 0.0) {
        return vec3f(0.0);
    }

    let Intensity = Light.ColourIntensity.w;
    let Colour = Light.ColourIntensity.rgb;

    // Standard physical inverse-square falloff with radius margin to avoid singularity
    let Falloff = Window / max(D2, 0.36);
    return Colour * (Intensity * Falloff * Profile * NdotL);
}
