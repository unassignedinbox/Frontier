//============================================================================================================================================
//                                                           DIRECTRADIANCE.WGSL
//============================================================================================================================================
// 🧩 Direct sun radiance and clustered lights at every primary surface: cascaded shadow depth or BVH shadow rays, and sky background.

//! include "FrameDescriptor.wgsl"
//! include "SurfaceReflectance.wgsl"
//! include "PackingCodec.wgsl"
//! include "SkyProjection.wgsl"
//! include "FireIllumination.wgsl"
//! include "ClusteredLights.wgsl"
//! include "RayTraversal.wgsl"

struct CascadeReceiver
{
    LightViewProjection : array<mat4x4f, 3>,
    Splits              : vec4f,   // [m]   xyz far distance of each cascade, w cascade count
    TexelWorld          : vec4f,   // [m]   world size of one shadow texel per cascade
}

@group(0) @binding(1) var<storage, read> Materials     : array<MaterialRecord>;
@group(0) @binding(2) var SurfaceAttributes            : texture_2d<f32>;
@group(0) @binding(3) var SurfaceDepth                 : texture_depth_2d;
@group(0) @binding(4) var ShadowDepth                  : texture_depth_2d_array;
@group(0) @binding(5) var ShadowSampler                : sampler_comparison;
@group(0) @binding(6) var<uniform> Receiver            : CascadeReceiver;
@group(0) @binding(7) var DirectSkyView                : texture_2d<f32>;
@group(0) @binding(8) var DirectSkySampler             : sampler;
@group(0) @binding(9) var DirectTarget                 : texture_storage_2d<rgba16float, write>;
// 📝 A uniform of all 96 records: 32 real luminaires, then 64 emissive proxies. The direct pass is at the storage-binding
//    limit with the BVH slots bound, so the lights ride the uniform budget instead.
@group(0) @binding(10) var<uniform> SceneLights  : array<LightRecord, 96>;
@group(0) @binding(11) var<storage, read> ClusterCells : array<vec4u>;
@group(0) @binding(12) var<storage, read> ClusterIndices : array<u32>;

// 📐 3×3 percentage-closer taps, each a hardware bilinear comparison, after a normal offset of 1.5 texels.
fn ResolveCascadeVisibility(Position: vec3f, Normal: vec3f) -> f32
{
    let ViewDistance = ResolveViewDepth(Position);
    let Count = u32(Receiver.Splits.w);
    var Cascade = 0u;
    while (Cascade + 1u < Count && ViewDistance > Receiver.Splits[Cascade])
    {
        Cascade++;
    }
    if (ViewDistance > Receiver.Splits[Count - 1u])
    {
        return 1.0;
    }
    let Offset = Position + Normal * (1.5 * Receiver.TexelWorld[Cascade]);
    let Clip = Receiver.LightViewProjection[Cascade] * vec4f(Offset, 1.0);
    let Ndc = Clip.xyz / Clip.w;
    let Coordinates = vec2f(Ndc.x * 0.5 + 0.5, 0.5 - Ndc.y * 0.5);
    if (any(Coordinates < vec2f(0.0)) || any(Coordinates > vec2f(1.0)) || Ndc.z > 1.0)
    {
        return 1.0;
    }
    let Texel = 1.0 / vec2f(textureDimensions(ShadowDepth).xy);
    var Lit = 0.0;
    for (var Y = -1; Y <= 1; Y++)
    {
        for (var X = -1; X <= 1; X++)
        {
            Lit += textureSampleCompareLevel(ShadowDepth, ShadowSampler, Coordinates + vec2f(f32(X), f32(Y)) * Texel, Cascade, Ndc.z);
        }
    }
    return Lit / 9.0;
}

fn ResolveBackgroundRadiance(Direction: vec3f) -> vec3f
{
    var Radiance = textureSampleLevel(DirectSkyView, DirectSkySampler, ProjectSkyCoordinates(Direction), 0.0).rgb * Frame.SkyScale
                 + Frame.NightSkyRadiance;
    // 📝 The solar disc (0.27° radius) is added only here, never to escaped rays, so it is not counted twice.
    let Cosine = dot(Direction, Frame.SunDirection);
    if (Cosine > 0.99998)
    {
        Radiance += Frame.SunIlluminance * 2500.0;
    }
    return Radiance;
}

@compute @workgroup_size(8, 8)
fn IntegrateDirectRadiance(@builtin(global_invocation_id) Invocation: vec3u)
{
    let Pixel = Invocation.xy;
    if (any(vec2f(Pixel) >= Frame.ScreenExtent))
    {
        return;
    }
    let Depth = textureLoad(SurfaceDepth, vec2i(Pixel), 0);
    let Centre = vec2f(Pixel) + 0.5;
    if (Depth <= 0.0)
    {
        let Far = ReconstructWorldPosition(Centre, 1.0e-6);
        textureStore(DirectTarget, vec2i(Pixel), vec4f(ResolveBackgroundRadiance(normalize(Far - Frame.CameraPosition)), 0.0));
        return;
    }
    let Position   = ReconstructWorldPosition(Centre, Depth);
    let Attributes = textureLoad(SurfaceAttributes, vec2i(Pixel), 0);
    let Normal     = DecodeOctahedral(Attributes.xy);
    let Surface    = ProjectSurfaceSample(Materials[u32(Attributes.z + 0.5)]);
    let View       = normalize(Frame.CameraPosition - Position);

    // 📝 Ray-traced shadows (RTX + the ray-traced shadow toggle) replace the cascades: the sun is sampled over its disc
    //    and every clustered light gets a BVH visibility ray. Otherwise the cascaded shadow depth answers.
    let UseRtShadows = FeatureEnabled(Frame, FeatureRayTracing) && FeatureEnabled(Frame, FeatureRayTracedShadows);
    let RayOrigin    = Position + Normal * RayEpsilon;
    var Visibility   = 1.0;
    var SunDir       = Frame.SunDirection;
    if (FeatureEnabled(Frame, FeatureDirectShadow))
    {
        if (UseRtShadows)
        {
            SunDir     = SampleSunConeDirection(Frame.SunDirection, ProjectTemporalPair(Pixel, Frame.FrameIndex, 17u));
            Visibility = TraceSceneVisibility(RayOrigin, SunDir, RayEpsilon, 400.0);
        }
        else
        {
            Visibility = ResolveCascadeVisibility(Position, Normal);
        }
    }
    var Radiance = Frame.SunIlluminance * Visibility * ResolveDirectReflectance(Surface, Normal, View, SunDir);
    let Fire = ResolveFireArrival(Position, Normal);
    if (any(Fire.Illuminance > vec3f(0.0)))
    {
        Radiance += Fire.Illuminance * ResolveDirectReflectance(Surface, Normal, View, Fire.Direction);
    }

    // 📝 Clustered light evaluations: only the position's cell, never the whole list. Each light's arrival carries its
    //    own profile (beam, spot, point or measured IES), and its shadow is a BVH ray when the toggle is on.
    // 📝 Two systems, one loop body: the real luminaires cast shadows (ray-traced or cascaded); emissive proxies are
    //    unshadowed glints, so they skip the visibility work.
    let Range = ResolveClusterRange(&ClusterCells, Position);
    for (var Li = 0u; Li < Range.y; Li++)
    {
        let Light = SceneLights[ClusterIndices[Range.x + Li]];
        let LightIllum = EvaluateLightArrival(Light, Position, Normal);
        if (any(LightIllum > vec3f(0.0)))
        {
            let Offset      = Light.PositionType.xyz - Position;
            let LightDist   = length(Offset);
            let TowardLight = Offset / max(LightDist, 1.0e-4);
            var LightVis    = 1.0;
            if (UseRtShadows)
            {
                LightVis = TraceSceneVisibility(RayOrigin, TowardLight, RayEpsilon, max(LightDist - 0.08, RayEpsilon));
            }
            Radiance += LightIllum * LightVis * ResolveDirectReflectance(Surface, Normal, View, TowardLight);
        }
    }
    let EmissiveRange = ResolveEmissiveClusterRange(&ClusterCells, Position);
    for (var Ei = 0u; Ei < EmissiveRange.y; Ei++)
    {
        let Emitter = SceneLights[ClusterIndices[EmissiveRange.x + Ei]];
        let EmitterIllum = EvaluateLightArrival(Emitter, Position, Normal);
        if (any(EmitterIllum > vec3f(0.0)))
        {
            let TowardEmitter = normalize(Emitter.PositionType.xyz - Position);
            Radiance += EmitterIllum * ResolveDirectReflectance(Surface, Normal, View, TowardEmitter);
        }
    }

    // 📝 w carries the sun visibility, so the recombination can rebuild the Lambertian exitance a surface card stores.
    textureStore(DirectTarget, vec2i(Pixel), vec4f(Radiance, Visibility));
}
