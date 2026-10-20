//============================================================================================================================================
//                                                         REFLECTIONTRAVERSAL.WGSL
//============================================================================================================================================
// 🧩 Glossy and mirror reflection rays: a GGX visible-normal sample per resampling pixel, traced across the screen first,
//    then through the world tracer (distance field, triangle hierarchy, or — in RTX mode — the multi-bounce BVH path).

// 📝 Three paths share one front end, selected per pixel:
//    • RTX (FeatureRayTracing + FeatureRayTracedReflections): the screen walk settles on-screen hits, then the chained
//      multi-bounce BVH path (TraceMultiBounceReflection) traces whatever the screen could not see.
//    • GTX: the screen walk, then the scene distance field — or the triangle hierarchy while the triangle toggle is on.
//    • Transmissive surfaces (glass) always take the Schlick-against-Snell split: one reflected leg plus one refracted leg
//      resumed past the far side. One refraction only; the exit bend is skipped and a trapped ray is absorbed.
//    Surfaces rougher than the cutoff are left to the resolve, which substitutes the diffuse irradiance there exactly as
//    Lumen reuses its diffuse gather for rough reflections.

//! include "CardSampling.wgsl"
//! include "ScreenTraversal.wgsl"
//! include "SurfaceReflectance.wgsl"
//! include "PackingCodec.wgsl"
//! include "DiscrepancySequence.wgsl"
//! include "RayTraversal.wgsl"

@group(0) @binding(1) var<storage, read> Materials : array<MaterialRecord>;
@group(0) @binding(2) var SurfaceAttributes        : texture_2d<f32>;
@group(0) @binding(3) var SurfaceDepth             : texture_depth_2d;
@group(0) @binding(4) var ReflectionTarget         : texture_storage_2d<rgba16float, write>;

const RoughnessCutoff : f32 = 0.5;

// 📝 How far past the entry point a transmitted segment resumes: the SDF march reports an instant hit from inside, so
//    the ray restarts past the far side instead. Must clear the thickest transmissive object — the garden's glass
//    sphere is 1.2 m across.
const TransmittedSlabOffset : f32 = 1.4;

// 📝 One world-tracer segment and its radiance. The BVH answers whenever ray tracing or the triangle toggle is on
//    (the scene's own surfaces are exact there); otherwise the distance field answers. An if, not a select — the
//    untaken tracer must not execute.
fn ResolveWorldSegment(Origin: vec3f, Direction: vec3f, MinimumTravel: f32, ConeSlope: f32) -> vec3f
{
    var Hit: TraversalHit;
    if (FeatureEnabled(Frame, FeatureRayTracing) || FeatureEnabled(Frame, FeatureTriangleReflection))
    {
        Hit = ResolveTraversalHit(TraceSceneRay(Origin, Direction, MinimumTravel, 300.0));
    }
    else
    {
        Hit = TraverseSceneDistance(Origin, Direction, MinimumTravel, 300.0, ConeSlope);
    }
    if (FeatureEnabled(Frame, FeatureScreenTraversal))
    {
        return ResolveContinuedRadiance(Hit, Direction);
    }
    return ResolveTraversalRadiance(Hit, Direction);
}

// 📝 Glass: a Schlick split at the entry interface. The reflected leg walks the same screen-first path as every
//    reflection; the transmitted leg refracts by Snell's law and resumes past the far side.
fn ResolveGlassRadiance(Position: vec3f, Normal: vec3f, View: vec3f, Direction: vec3f, Bias: f32, Roughness: f32,
                        F0: f32) -> vec3f
{
    let Complement = 1.0 - clamp(dot(View, Normal), 0.0, 1.0);
    var Share = F0 + (1.0 - F0) * Complement * Complement * Complement * Complement * Complement;
    var Transmitted = vec3f(0.0);
    let Bent = refract(-View, Normal, 1.0 / ResolveDielectricIor(F0));
    if (dot(Bent, Bent) > 0.0)
    {
        Transmitted = ResolveWorldSegment(Position, Bent, TransmittedSlabOffset, 0.0);
    }
    else
    {
        Share = 1.0;
    }
    var Reflected: vec3f;
    if (FeatureEnabled(Frame, FeatureScreenTraversal))
    {
        let Origin = Position + Normal * ResolveScreenBias(ResolveViewDepth(Position));
        let Strike = TraverseScreenDepth(Origin, Direction, 300.0, ScreenTraversalSteps);
        if (Strike.Hit)
        {
            Reflected = ResolveStrikeRadiance(Strike);
        }
        else
        {
            Reflected = ResolveWorldSegment(Position + Normal * Bias, Direction, max(0.5 * Bias, Strike.Travel - Bias),
                                            Roughness * 0.05);
        }
    }
    else
    {
        Reflected = ResolveWorldSegment(Position + Normal * Bias, Direction, 0.5 * Bias, Roughness * 0.05);
    }
    return Share * Reflected + (1.0 - Share) * Transmitted;
}

@compute @workgroup_size(8, 8)
fn TraverseReflection(@builtin(global_invocation_id) Invocation: vec3u)
{
    let TracePixel = Invocation.xy;
    if (any(vec2f(TracePixel) >= Frame.TraceExtent))
    {
        return;
    }
    let ScreenPixel = min(vec2i(TracePixel * Frame.TraceScale), vec2i(Frame.ScreenExtent) - vec2i(1));
    let Depth = textureLoad(SurfaceDepth, ScreenPixel, 0);
    if (Depth <= 0.0)
    {
        textureStore(ReflectionTarget, vec2i(TracePixel), vec4f(0.0));
        return;
    }
    let Attributes = textureLoad(SurfaceAttributes, ScreenPixel, 0);
    let Surface = ProjectSurfaceSample(Materials[u32(Attributes.z + 0.5)]);
    let Roughness = select(Surface.Roughness, Surface.CoatRoughness, Surface.CoatWeight > 0.0);
    let UseRtReflections = FeatureEnabled(Frame, FeatureRayTracing) && FeatureEnabled(Frame, FeatureRayTracedReflections);
    let Cutoff = select(RoughnessCutoff, ResolveReflectionRoughness(Frame), UseRtReflections);
    if (Roughness > Cutoff)
    {
        textureStore(ReflectionTarget, vec2i(TracePixel), vec4f(0.0));
        return;
    }
    let Position = ReconstructWorldPosition(vec2f(ScreenPixel) + 0.5, Depth);
    let Normal = DecodeOctahedral(Attributes.xy);
    let View = normalize(Frame.CameraPosition - Position);
    let Frame3 = ConstructTangentFrame(Normal);
    let TangentView = transpose(Frame3) * View;
    let Pair = ProjectTemporalPair(TracePixel, Frame.FrameIndex, 29u);
    let Half = Frame3 * ProjectVisibleNormal(TangentView, Roughness * Roughness, Pair);
    var Direction = reflect(-View, Half);
    if (dot(Direction, Normal) <= 0.0)
    {
        Direction = reflect(-View, Normal);
    }
    let Bias = ResolveSurfaceBias(Position);

    var Radiance = vec3f(0.0);
    if (Surface.Transmission > 0.5)
    {
        Radiance = ResolveGlassRadiance(Position, Normal, View, Direction, Bias, Roughness, Surface.SpecularLevel);
    }
    else if (UseRtReflections)
    {
        var Seed = ConstructPixelSeed(TracePixel, Frame.FrameIndex, 41u);
        if (FeatureEnabled(Frame, FeatureScreenTraversal))
        {
            let Origin = Position + Normal * ResolveScreenBias(ResolveViewDepth(Position));
            let Strike = TraverseScreenDepth(Origin, Direction, 300.0, ScreenTraversalSteps);
            if (Strike.Hit)
            {
                Radiance = ResolveStrikeRadiance(Strike);
            }
            else
            {
                // 📝 The screen walked the ray this far unoccluded; the BVH path continues from there.
                let Resume = max(0.5 * Bias, Strike.Travel - Bias);
                Radiance = TraceMultiBounceReflection(Position + Normal * Bias + Direction * Resume, Direction, Roughness, &Seed);
            }
        }
        else
        {
            Radiance = TraceMultiBounceReflection(Position + Normal * RayEpsilon, Direction, Roughness, &Seed);
        }
    }
    else if (FeatureEnabled(Frame, FeatureScreenTraversal))
    {
        let Origin = Position + Normal * ResolveScreenBias(ResolveViewDepth(Position));
        let Strike = TraverseScreenDepth(Origin, Direction, 300.0, ScreenTraversalSteps);
        if (Strike.Hit)
        {
            Radiance = ResolveStrikeRadiance(Strike);
        }
        else
        {
            let Resume = max(0.5 * Bias, Strike.Travel - Bias);
            Radiance = ResolveWorldSegment(Position + Normal * Bias, Direction, Resume, Roughness * 0.05);
        }
    }
    else
    {
        Radiance = ResolveWorldSegment(Position + Normal * Bias, Direction, 0.5 * Bias, Roughness * 0.05);
    }
    textureStore(ReflectionTarget, vec2i(TracePixel), vec4f(min(Radiance, vec3f(200.0)), 1.0));
}
