//============================================================================================================================================
//                                                            FORWARDRASTER.WGSL
//============================================================================================================================================
// 🧩 A small lit forward pass over the scene's facets: the mirrors' reflected views and the HMI Garage's studio portrait of the car,
//    including live paint/rim waves and X-ray inspection (Ghost, Wireframe and sweeping Perception Scan).

//! include "FrameDescriptor.wgsl"
//! include "SurfaceReflectance.wgsl"
//! include "SkyProjection.wgsl"
//! include "EffectRecords.wgsl"
//! include "VehicleVisual.wgsl"
//! include "CabinVolume.wgsl"

struct ForwardView
{
    ViewProjection : mat4x4f,
    Camera         : vec4f,   // xyz [m] eye, w mode: 0 scene, 1 studio
    Key            : vec4f,   // xyz studio key light direction (toward the light), w its intensity
    Fill           : vec4f,   // xyz studio fill direction, w intensity
    Rim            : vec4f,   // xyz studio rim direction, w intensity
    RippleCentre   : vec4f,   // xyz [m] body-local drop centre, w [m] SweepFront (mode 1) or Radius (mode 2)
    RippleNext     : vec4f,   // rgb linear target paint albedo, w mode: 0 off, 1 side-to-side sweep, 2 radial sphere
    RippleFinish   : vec4f,   // x roughness, y metalness, z coat weight, w wheel-rim ripple active
    RimRipple      : vec4f,   // rgb linear target hub albedo, w wheel-rim ripple radius [m]
    RimCentre      : vec4f,   // xyz [m] body-local wheel drop centre, w target hub roughness
    Inspection     : vec4f,   // x view (0 Solid, 1 Ghost, 2 Wireframe, 3 Scan), y ScanX [m], z HoloAmount [0..1], w Clock [s]
}

struct CascadeReceiver
{
    LightViewProjection : array<mat4x4f, 3>,
    Splits              : vec4f,
    TexelWorld          : vec4f,
}

@group(0) @binding(1) var<uniform> View                 : ForwardView;
@group(0) @binding(2) var<storage, read> Occupants      : array<OccupantRecord>;
@group(0) @binding(3) var<storage, read> Materials      : array<MaterialRecord>;
@group(0) @binding(4) var<storage, read> VehicleState   : VehicleRecord;
@group(0) @binding(5) var<uniform> Effect               : EffectRecord;
@group(0) @binding(6) var ShadowDepth                   : texture_depth_2d_array;
@group(0) @binding(7) var ShadowSampler                 : sampler_comparison;
@group(0) @binding(8) var<uniform> Receiver             : CascadeReceiver;
@group(0) @binding(9) var SkyView                       : texture_2d<f32>;
@group(0) @binding(10) var SkySampler                   : sampler;

struct ForwardVarying
{
    @builtin(position) Position             : vec4f,
    @location(0) Normal                     : vec3f,
    @location(1) @interpolate(flat) Family  : u32,
    @location(2) WorldPosition              : vec3f,
    @location(3) @interpolate(flat) Vehicle : u32,
    @location(4) BodyLocal                  : vec3f,
    @location(5) Barycentric                : vec3f,
}

const StudioOccupantSentinel : u32 = 0xFFFFFFFFu;
const StudioSoftboxFamily    : u32 = 90u;
const HeadlightLensFamily    : u32 = 13u;
const StudioGroundOffsetZ    : f32 = 0.53;

@vertex
fn ForwardVertex(@location(0) Position: vec3f, @location(1) Family: u32, @location(2) Normal: vec3f,
                 @location(3) OccupantOffset: u32, @builtin(instance_index) OccupantIndex: u32, @builtin(vertex_index) VertexIndex: u32) -> ForwardVarying
{
    var Varying: ForwardVarying;
    Varying.Barycentric = vec3f(select(0.0, 1.0, VertexIndex % 3u == 0u), select(0.0, 1.0, VertexIndex % 3u == 1u), select(0.0, 1.0, VertexIndex % 3u == 2u));

    // 📝 The Garage's Lobby studio stage rides horizontally around Occupants[0] so both Lobby.html and index.html
    //    show the exact same seamless cyclorama cove, overhead softbox, ceiling trusses and hero turntable.
    if (OccupantOffset == StudioOccupantSentinel)
    {
        let Body = Occupants[0];
        let RawForward = vec2f(Body.LocalToWorld[0].x, Body.LocalToWorld[0].y);
        let ForwardLen = max(length(RawForward), 1.0e-5);
        let ForwardXY  = RawForward / ForwardLen;
        let LeftXY     = vec2f(-ForwardXY.y, ForwardXY.x);
        let Origin     = Body.LocalToWorld[3].xyz;
        let WorldXYZ   = Origin + vec3f(ForwardXY * Position.x + LeftXY * Position.y, Position.z);
        let WorldNorm  = vec3f(ForwardXY * Normal.x + LeftXY * Normal.y, Normal.z);
        Varying.Position      = View.ViewProjection * vec4f(WorldXYZ, 1.0);
        Varying.Normal        = WorldNorm;
        Varying.Family        = Family;
        Varying.WorldPosition = WorldXYZ;
        Varying.Vehicle       = 0u;
        Varying.BodyLocal     = Position;
        return Varying;
    }

    let Index = OccupantIndex + OccupantOffset;
    let Occupant = Occupants[Index];
    var Moved: DeformedVertex;
    Moved.Position = Position;
    Moved.Normal = Normal;
    if (Index >= 1u && Index < 5u)
    {
        Moved = ResolveWheelVertex(Index - 1u, Family, Position, Normal);
    }
    else if (Index == 0u)
    {
        Moved = DeformBodyVertex(Family, Position, Normal);
    }
    let World = Occupant.LocalToWorld * vec4f(Moved.Position, 1.0);
    Varying.Position = View.ViewProjection * World;
    Varying.Normal = (Occupant.LocalToWorld * vec4f(Moved.Normal, 0.0)).xyz;
    Varying.Family = select(Family, Occupant.MaterialOverride, Occupant.MaterialOverride != NoMaterialOverride);
    Varying.WorldPosition = World.xyz;
    Varying.Vehicle = select(select(0u, 1u, Index < 5u), 1u, View.Camera.w > 0.5);
    Varying.BodyLocal = (Occupants[0].WorldToLocal * World).xyz;
    return Varying;
}

fn ResolveSunVisibility(Position: vec3f, Normal: vec3f) -> f32
{
    let ViewDistance = ResolveViewDepth(Position);
    let Count = u32(Receiver.Splits.w);
    if (Count == 0u || ViewDistance < 0.0 || ViewDistance > Receiver.Splits[Count - 1u])
    {
        return 1.0;
    }
    var Cascade = 0u;
    while (Cascade + 1u < Count && ViewDistance > Receiver.Splits[Cascade])
    {
        Cascade++;
    }
    let Offset = Position + Normal * (1.5 * Receiver.TexelWorld[Cascade]);
    let Clip = Receiver.LightViewProjection[Cascade] * vec4f(Offset, 1.0);
    let Ndc = Clip.xyz / Clip.w;
    let Coordinates = vec2f(Ndc.x * 0.5 + 0.5, 0.5 - Ndc.y * 0.5);
    if (any(Coordinates < vec2f(0.0)) || any(Coordinates > vec2f(1.0)) || Ndc.z > 1.0)
    {
        return 1.0;
    }
    return textureSampleCompareLevel(ShadowDepth, ShadowSampler, Coordinates, Cascade, Ndc.z);
}

fn ClassifyMasked(BodyLocal: vec3f) -> bool
{
    if (Effect.MaskMode == 0u)
    {
        return false;
    }
    if (Effect.MaskMode == 3u)
    {
        return true;
    }
    let Unit = (BodyLocal - Effect.BodyMinimum.xyz) / max(Effect.BodyMaximum.xyz - Effect.BodyMinimum.xyz, vec3f(1.0e-3));
    let Elapsed = Effect.Clock - Effect.MaskStart;
    let Seed = u32(Effect.Emitters[0].Centre.w);
    if (Effect.MaskMode == 1u)
    {
        return Elapsed >= ResolveDerezRelease(Unit, Seed, Effect.Style, 1.0);
    }
    return Elapsed < ResolveRezArrival(Unit, Seed, Effect.Style, 1.0);
}

fn ResolveForwardSurface(Family: u32, BodyLocal: vec3f) -> SurfaceSample
{
    var Surface = ProjectSurfaceSample(Materials[Family]);
    if (Family == BodyCoatFamilyIndex && View.RippleNext.w > 0.5)
    {
        let Coverage = ResolvePaintWaveCoverage(BodyLocal, View.RippleCentre, View.RippleNext.w);
        if (Coverage > 0.0)
        {
            var Target = Surface;
            Target.Albedo        = View.RippleNext.rgb;
            Target.Roughness     = clamp(View.RippleFinish.x, 0.03, 1.0);
            Target.Metalness     = View.RippleFinish.y;
            Target.CoatWeight    = View.RippleFinish.z;
            Target.CoatRoughness = 0.03;
            Surface = BlendSurfaceSample(Surface, Target, Coverage);
        }
    }
    else if (Family == HubFamilyIndex && View.RippleFinish.w > 0.5)
    {
        let Radius = View.RimRipple.w;
        if (Radius > 0.001)
        {
            let Coverage = 1.0 - smoothstep(Radius - 0.55, Radius, distance(BodyLocal, View.RimCentre.xyz));
            Surface.Albedo    = mix(Surface.Albedo, View.RimRipple.rgb, Coverage);
            Surface.Roughness = mix(Surface.Roughness, clamp(View.RimCentre.w, 0.03, 1.0), Coverage);
            Surface.Metalness = mix(Surface.Metalness, 0.9, Coverage);
            Surface.Emission  = Surface.Emission * (1.0 - Coverage);
        }
    }
    return Surface;
}

// 📝 Procedural automotive photography softbox + studio horizon environment for glossy & metallic coats in the Garage stage.
fn SampleStudioEnvironment(Direction: vec3f, Roughness: f32) -> vec3f
{
    let Up = clamp(Direction.z, -1.0, 1.0);
    let Soft = mix(0.06, 0.42, Roughness);
    let Projected = Direction.xy / max(abs(Direction.z) + 0.22, 0.22);
    let Softbox = smoothstep(1.35 + Soft, 0.85 - Soft * 0.4, abs(Projected.x))
                * smoothstep(0.95 + Soft, 0.48 - Soft * 0.3, abs(Projected.y))
                * smoothstep(0.05, 0.45, Up);
    let Horizon = exp(-pow((Up - 0.08) * 3.8, 2.0)) * 0.34;
    let SkyDome = mix(vec3f(0.03, 0.035, 0.045), vec3f(0.22, 0.26, 0.33), clamp(Up * 0.5 + 0.5, 0.0, 1.0));
    return SkyDome + vec3f(1.55, 1.62, 1.72) * Softbox + vec3f(0.32, 0.42, 0.58) * Horizon;
}

// 📝 Shade the Lobby studio architecture (cyclorama cove, turntable, polished floor, trusses & overhead softbox)
//    matching LobbyPreview.png: crisp luminous overhead softbox band, moody mid-grey cyclorama cove wall, bright
//    illuminated white/light-grey turntable & floor pool, crisp turntable seam, and deep under-vehicle shadow.
fn ShadeLobbyStudioSurface(Family: u32, BodyLocal: vec3f, WorldPosition: vec3f, WorldNormal: vec3f, ViewDirection: vec3f, HoloAmt: f32) -> vec4f
{
    let Dim = mix(0.66, 0.44, HoloAmt);
    if (Family == StudioSoftboxFamily)
    {
        return vec4f(vec3f(0.98, 0.99, 1.0) * mix(0.96, 0.78, HoloAmt), 1.0);
    }

    let Surface = ProjectSurfaceSample(Materials[Family]);
    let Albedo  = ResolveDiffuseAlbedo(Surface);
    let Body    = Occupants[0];
    let RawFwd  = vec2f(Body.LocalToWorld[0].x, Body.LocalToWorld[0].y);
    let FwdXY   = RawFwd / max(length(RawFwd), 1.0e-5);
    let LeftXY  = vec2f(-FwdXY.y, FwdXY.x);

    let StudioPos = BodyLocal + vec3f(0.0, 0.0, StudioGroundOffsetZ);
    let LocalN    = normalize(vec3f(dot(WorldNormal.xy, FwdXY), dot(WorldNormal.xy, LeftXY), WorldNormal.z));
    let LocalV    = normalize(vec3f(dot(ViewDirection.xy, FwdXY), dot(ViewDirection.xy, LeftXY), ViewDirection.z));

    // Downward pool from the overhead softbox: bright studio floor & turntable, falling off toward the 20 m cove wall
    var SoftboxPool = 0.0;
    for (var Tap = -1; Tap <= 1; Tap++)
    {
        let Emitter = vec3f(f32(Tap) * 5.0, 0.0, 5.45);
        let Delta   = Emitter - StudioPos;
        let DistSq  = dot(Delta, Delta);
        let InvDist = inverseSqrt(max(DistSq, 1.0e-4));
        let L       = Delta * InvDist;
        let CosDown = clamp(-L.z, 0.0, 1.0);
        let CosSurf = clamp(dot(LocalN, L), 0.0, 1.0);
        SoftboxPool += (32.0 * pow(CosDown, 2.15) * CosSurf) / (10.0 + DistSq);
    }

    // Moody studio cyclorama wall illumination (LobbyPreview.png: mid-grey #5c6066..#787c82 gradient rising toward softbox)
    let FloorUp    = clamp(LocalN.z, 0.0, 1.0);
    let WallWeight = 1.0 - FloorUp;
    let WallGrad   = (0.095 + 0.095 * smoothstep(0.2, 5.3, StudioPos.z)) * WallWeight;
    let FloorBase  = 0.14 * FloorUp;

    // Deep under-vehicle chassis & four-tyre contact shadow on the turntable floor
    var Occlusion = 1.0;
    if (StudioPos.z < 0.06 && FloorUp > 0.5)
    {
        let Box      = abs(StudioPos.xy) - vec2f(2.55, 1.00);
        let BoxDist  = length(max(Box, vec2f(0.0))) + min(max(Box.x, Box.y), 0.0);
        let UnderCar = 0.78 * (1.0 - smoothstep(-0.20, 0.45, BoxDist)) + 0.18 * (1.0 - smoothstep(0.0, 1.55, BoxDist));
        let WheelA   = min(length(StudioPos.xy - vec2f( 1.73,  1.05)), length(StudioPos.xy - vec2f( 1.73, -1.05)));
        let WheelB   = min(length(StudioPos.xy - vec2f(-1.67,  1.05)), length(StudioPos.xy - vec2f(-1.67, -1.05)));
        let TyrePad  = 0.65 * (1.0 - smoothstep(0.16, 0.48, min(WheelA, WheelB)));
        let ShadowStrength = select(1.0, 0.38, View.Inspection.x > 0.5 && View.Inspection.x < 2.5);
        Occlusion = 1.0 - clamp(max(UnderCar, TyrePad) * ShadowStrength, 0.0, 0.90);
    }

    // Accentuate the hairline turntable seam ring (Family == 2u Surround at r = 4.65 m) so it reads crisply at any resolution
    let SeamBoost = select(1.0, 0.52, Family == 2u);
    var Radiance  = Albedo * SeamBoost * ((SoftboxPool + FloorBase) * Occlusion + WallGrad) * vec3f(0.97, 0.985, 1.0);

    // Polished StudioFloor & StudioTurntable reflection of the overhead softbox + subtle live body-coat bounce
    if (StudioPos.z < 0.06 && FloorUp > 0.5 && Surface.Roughness < 0.25)
    {
        let Refl = reflect(-LocalV, LocalN);
        if (Refl.z > 0.02)
        {
            let T      = (5.45 - StudioPos.z) / Refl.z;
            let HitXY  = StudioPos.xy + Refl.xy * T;
            let InSoft = smoothstep(12.0, 8.5, abs(HitXY.x)) * smoothstep(12.0, 8.5, abs(HitXY.y));
            let Fres   = 0.06 + 0.48 * pow(1.0 - clamp(LocalV.z, 0.0, 1.0), 3.6);
            Radiance  += vec3f(0.96, 0.98, 1.0) * InSoft * Fres * (1.0 - Surface.Roughness * 2.4) * Occlusion * 0.62;
        }
        let CoatSurface = ResolveForwardSurface(BodyCoatFamilyIndex, vec3f(clamp(StudioPos.x, -2.5, 2.5), clamp(StudioPos.y, -1.0, 1.0), 0.2));
        let FlankBox    = abs(StudioPos.xy) - vec2f(2.65, 1.12);
        let FlankDist   = length(max(FlankBox, vec2f(0.0)));
        let PaintBounce = exp(-FlankDist * 1.35) * smoothstep(-0.3, 0.25, max(FlankBox.x, FlankBox.y));
        Radiance       += CoatSurface.Albedo * PaintBounce * 0.12;
    }

    Radiance *= Dim;
    let Mapped = Radiance * 1.55 / (vec3f(1.0) + Radiance * 1.55);
    return vec4f(Mapped, 1.0);
}

@fragment
fn ForwardFragment(Varying: ForwardVarying) -> @location(0) vec4f
{
    let Studio = View.Camera.w > 0.5;
    let HoloAmt = clamp(View.Inspection.z, 0.0, 1.0);
    let ViewDirection = normalize(View.Camera.xyz - Varying.WorldPosition);
    var Normal = normalize(Varying.Normal);
    if (dot(Normal, ViewDirection) < 0.0)
    {
        Normal = -Normal;
    }

    if (Studio && Varying.Vehicle == 0u)
    {
        return ShadeLobbyStudioSurface(Varying.Family, Varying.BodyLocal, Varying.WorldPosition, Normal, ViewDirection, HoloAmt);
    }

    if (Varying.Vehicle == 1u)
    {
        if (Varying.Family == MirrorGlassFamily)
        {
            discard;
        }
        if (!Studio && Varying.Family == VehicleGlassFamily && VehicleState.Cabin.y > 0.5)
        {
            discard;
        }
        if (ClassifyMasked(Varying.BodyLocal))
        {
            discard;
        }
    }

    var SolidAlpha = 1.0;
    if (Studio)
    {
        let ViewMode = View.Inspection.x;
        let ScanX    = View.Inspection.y;
        if (ViewMode > 0.5 && ViewMode < 2.5)
        {
            SolidAlpha = 1.0 - HoloAmt;
        }
        else if (ViewMode > 2.5)
        {
            let ScanD      = Varying.BodyLocal.x - ScanX;
            let ScanRegion = smoothstep(-0.9, -0.05, ScanD);
            SolidAlpha     = 1.0 - ScanRegion * HoloAmt * 0.98;
        }
        else
        {
            SolidAlpha = 1.0 - HoloAmt * 0.85;
        }
        if (SolidAlpha < 0.06)
        {
            discard;
        }
    }

    var Surface = ResolveForwardSurface(Varying.Family, Varying.BodyLocal);
    if (Studio && Varying.Family == HeadlightLensFamily)
    {
        // Match Lobby.html's smoked studio headlight lenses (IlluminationHost.js) so the nose doesn't blow out.
        Surface.Albedo     = vec3f(0.05, 0.055, 0.06);
        Surface.Emission   = vec3f(0.0);
        Surface.Roughness  = 0.08;
        Surface.CoatWeight = 1.0;
    }

    if (Studio)
    {
        // Smoked studio canopy glass: subtle sky-softbox Fresnel reflection while letting the cabin interior show through.
        if (Varying.Family == VehicleGlassFamily && VehicleState.Cabin.y > 0.5)
        {
            let NdotV = clamp(dot(Normal, ViewDirection), 0.0, 1.0);
            let Fresnel = 0.08 + 0.72 * pow(1.0 - NdotV, 4.2);
            let GlassEnv = SampleStudioEnvironment(reflect(-ViewDirection, Normal), 0.04);
            let GlassCol = (vec3f(0.02, 0.03, 0.04) + GlassEnv * Fresnel) * 1.35;
            let MappedGlass = GlassCol / (vec3f(1.0) + GlassCol);
            let GlassAlpha = clamp(0.42 + Fresnel * 0.45, 0.0, 0.88) * SolidAlpha;
            return vec4f(MappedGlass * GlassAlpha, GlassAlpha);
        }

        let Albedo = ResolveDiffuseAlbedo(Surface);
        var Radiance = Surface.Emission * 0.38;
        Radiance += ResolveDirectReflectance(Surface, Normal, ViewDirection, normalize(View.Key.xyz)) * View.Key.w;
        Radiance += ResolveDirectReflectance(Surface, Normal, ViewDirection, normalize(View.Fill.xyz)) * View.Fill.w;
        Radiance += ResolveDirectReflectance(Surface, Normal, ViewDirection, normalize(View.Rim.xyz)) * View.Rim.w;
        Radiance += Albedo * (vec3f(0.18, 0.21, 0.25) * (0.62 + 0.38 * Normal.z) + vec3f(0.06, 0.055, 0.05) * max(-Normal.z, 0.0));

        let NdotV = max(dot(Normal, ViewDirection), 1.0e-3);
        let Reflected = reflect(-ViewDirection, Normal);
        let BaseEnv = SampleStudioEnvironment(Reflected, Surface.Roughness);
        let BaseSplit = ResolveEnvironmentResponse(NdotV, Surface.Roughness);
        var EnvSpec = BaseEnv * (ResolveSpecularColour(Surface) * BaseSplit.x + BaseSplit.y) * 0.55;
        if (Surface.CoatWeight > 0.0)
        {
            let CoatEnv = SampleStudioEnvironment(Reflected, Surface.CoatRoughness);
            let CoatSplit = ResolveEnvironmentResponse(NdotV, Surface.CoatRoughness);
            let CoatRefl = (0.04 * CoatSplit.x + CoatSplit.y) * Surface.CoatWeight;
            EnvSpec = EnvSpec * (1.0 - CoatRefl) + CoatEnv * CoatRefl * 0.65;
        }
        Radiance += EnvSpec;

        // Subtle luminous sheen riding the active paint wavefront as it sweeps or expands radially across the body.
        if (Varying.Family == BodyCoatFamilyIndex && View.RippleNext.w > 0.5)
        {
            let Band = ResolvePaintWaveBand(Varying.BodyLocal, View.RippleCentre, View.RippleNext.w);
            Radiance += mix(View.RippleNext.rgb, vec3f(0.65, 0.88, 1.0), 0.55) * Band * 0.42;
        }

        let Mapped = Radiance * 1.55 / (vec3f(1.0) + Radiance * 1.55);
        return vec4f(Mapped * SolidAlpha, SolidAlpha);
    }

    // 📝 Scene (mirrors): the sun with its cascaded shadow, the sky over the normal's hemisphere, the ground's bounce.
    let Albedo = ResolveDiffuseAlbedo(Surface);
    var Radiance = Surface.Emission * Frame.EmissionScale;
    let Visibility = ResolveSunVisibility(Varying.WorldPosition, Normal);
    Radiance += Frame.SunIlluminance * Visibility * ResolveDirectReflectance(Surface, Normal, ViewDirection, Frame.SunDirection);
    let SkyDirection = normalize(vec3f(Normal.xy, max(Normal.z, 0.0) + 0.35));
    let Sky = textureSampleLevel(SkyView, SkySampler, ProjectSkyCoordinates(SkyDirection), 0.0).rgb * Frame.SkyScale + Frame.NightSkyRadiance;
    let Bounce = Frame.SunIlluminance * (max(Frame.SunDirection.z, 0.0) * 0.18 * InversePi);
    let Enclosure = select(1.0, mix(1.0, 0.2, ResolveCabinShare(Varying.BodyLocal)), Varying.Vehicle == 1u);
    Radiance += Albedo * (Sky * (0.55 + 0.45 * Normal.z) + Bounce * (0.5 - 0.5 * Normal.z)) * Enclosure;
    let Reflected = reflect(-ViewDirection, Normal);
    let Glossy = textureSampleLevel(SkyView, SkySampler, ProjectSkyCoordinates(normalize(vec3f(Reflected.xy, max(Reflected.z, 0.02)))), 0.0).rgb * Frame.SkyScale;
    Radiance += ResolveSpecularColour(Surface) * Glossy * (1.0 - Surface.Roughness) * 0.6;
    return vec4f(Radiance, 1.0);
}

// 📝 Second studio pass (depth read-only, premultiplied alpha): true translucent Hologram Ghost, crisp structural
//    Wireframe, and the sweeping X-Ray Perception Scan (hologram + wireframe behind the travelling LiDAR front).
@fragment
fn ForwardXrayFragment(Varying: ForwardVarying) -> @location(0) vec4f
{
    // Evaluate screen-space derivatives in uniform control flow first.
    let Width = max(fwidth(Varying.Barycentric), vec3f(1.0e-5));
    let EdgeDistance = Varying.Barycentric / Width;
    let RawEdge = 1.0 - smoothstep(0.35, 1.35, min(min(EdgeDistance.x, EdgeDistance.y), EdgeDistance.z));
    // Attenuate sub-pixel dense mesh clusters (e.g. cylindrical tyre/rim tessellation) so multi-layer wheels stay crisp, never blown out.
    let DensityScale = clamp(1.0 / (1.0 + (Width.x + Width.y + Width.z) * 1.35), 0.16, 1.0);
    let Edge = RawEdge * DensityScale;

    if (Varying.Family == MirrorGlassFamily || (Varying.Vehicle == 1u && ClassifyMasked(Varying.BodyLocal)))
    {
        discard;
    }

    let ViewMode = View.Inspection.x;
    let ScanX    = View.Inspection.y;
    let HoloAmt  = clamp(View.Inspection.z, 0.0, 1.0);
    if (HoloAmt < 0.004)
    {
        discard;
    }

    let ViewDirection = normalize(View.Camera.xyz - Varying.WorldPosition);
    var Normal = normalize(Varying.Normal);
    if (dot(Normal, ViewDirection) < 0.0)
    {
        Normal = -Normal;
    }
    let Fresnel = pow(1.0 - clamp(abs(dot(Normal, ViewDirection)), 0.0, 1.0), 2.4);
    let Lines   = smoothstep(0.72, 1.0, sin(Varying.BodyLocal.z * 85.0) * 0.5 + 0.5);
    let Cyan    = vec3f(0.28, 0.76, 0.96);
    let Ice     = vec3f(0.68, 0.93, 1.00);
    let ShellScale = mix(0.38, 1.0, DensityScale);

    // ① Wireframe view: crisp anti-aliased structural edges across body, cabin, steering wheel, tyres, rims & brakes.
    if (ViewMode > 1.5 && ViewMode < 2.5)
    {
        let Alpha = clamp((Edge * 0.78 + Fresnel * 0.025 * ShellScale) * HoloAmt, 0.0, 0.90);
        if (Alpha < 0.008)
        {
            discard;
        }
        let Colour = mix(Cyan, Ice, clamp(Edge * 0.65 + Fresnel * 0.35, 0.0, 1.0));
        return vec4f(Colour * Alpha, Alpha);
    }

    // ② Ghost view: translucent holographic X-ray shell carrying a subtle hint of the live paint colour + internal anatomy.
    if (ViewMode > 0.5 && ViewMode < 1.5)
    {
        let Surface = ResolveForwardSurface(Varying.Family, Varying.BodyLocal);
        let PaintHint = mix(Surface.Albedo, Cyan, 0.58);
        let Alpha = clamp((0.038 + Fresnel * 0.24 + Lines * 0.026 + Edge * 0.16) * ShellScale * HoloAmt, 0.0, 0.48);
        let Colour = clamp(PaintHint * (0.48 + 0.52 * Fresnel) + Ice * (Fresnel * 0.42 + Edge * 0.28 + Lines * 0.05), vec3f(0.0), vec3f(1.0));
        return vec4f(Colour * Alpha, Alpha);
    }

    // ③ X-Ray Scan (Truck.tsx): front-to-back LiDAR pulse sweep — solid PBR ahead, glowing wavefront curtain,
    //    Hologram Ghost + Wireframe mesh behind the wavefront.
    let ScanD    = Varying.BodyLocal.x - ScanX;
    let Region   = smoothstep(-0.9, -0.05, ScanD);
    let WaveEdge = exp(-pow((ScanD + 0.16) * 4.4, 2.0));
    let Vis      = clamp(max(Region, WaveEdge * 0.9) * HoloAmt, 0.0, 1.0);
    if (Vis < 0.004)
    {
        discard;
    }
    let Wire  = Edge * smoothstep(-0.20, 0.06, ScanD) * 0.62;
    let Alpha = clamp(((0.026 + Fresnel * 0.26 + Lines * 0.028) * ShellScale + WaveEdge * 0.55 * ShellScale + Wire) * Vis, 0.0, 0.88);
    let Colour = clamp(Cyan * (0.50 + 0.50 * Fresnel + Wire * 1.15) + Ice * (WaveEdge * 1.25 + Wire * 0.35), vec3f(0.0), vec3f(1.0));
    return vec4f(Colour * Alpha, Alpha);
}
