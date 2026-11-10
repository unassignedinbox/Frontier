//============================================================================================================================================
//                                                          ENHANCEDSUN.WGSL
//============================================================================================================================================
// 🧩 The unified reservoir's sun proposal. Split from EnhancedReservoir.wgsl because its visibility trace needs the scene
//    distance field, which only the reservoir programs include — the display programs take EnhancedFrame.wgsl instead.

//! include "EnhancedReservoir.wgsl"
//! include "DistanceTraversal.wgsl"

// 📐 The sun candidate for a receiver: a directional sample at the sun's direction, pushed out to the sky so the
//    reconnection vertex is a real point and the shared reconnection machinery needs no special case. Its radiance is the
//    solar irradiance over the cosine, i.e. exactly the exitance the direct pass computes.
fn ConstructSunCandidate(Position: vec3f) -> ReservoirSample
{
    var Candidate: ReservoirSample;
    Candidate.Position = Position + Frame.SunDirection * SkyReach;
    Candidate.Normal   = -Frame.SunDirection;
    Candidate.Identity = SunIdentity;
    Candidate.Radiance = Frame.SunIlluminance;
    return Candidate;
}

// 📐 Balance-heuristic proposal density of the sun: a cosine lobe of exponent UnifiedBalance about the sun direction.
//    The slider at 0 makes it uniform (the sun is proposed as often as any direction) and at 1 it is the cosine the
//    temporal pass already samples, so both analytic densities the balance heuristic needs are available exactly.
fn ResolveUnifiedSunDensity(Normal: vec3f) -> f32
{
    let Balance = clamp(Frame.EnhancedParams.z, 0.0, 4.0);
    let Cosine = clamp(dot(Normal, Frame.SunDirection), 0.0, 1.0);
    return pow(Cosine, Balance);
}

// 📐 Sun visibility for the unified candidate: the scene shadow trace from the receiver, the same test the card lighting
//    runs. Hardness 6 spreads a penumbra the way the direct pass's own trace does.
fn ResolveUnifiedSunVisibility(Position: vec3f, Normal: vec3f) -> f32
{
    if (dot(Normal, Frame.SunDirection) <= 0.0)
    {
        return 0.0;
    }
    let Bias = ResolveSurfaceBias(Position);
    return TraverseSceneShadow(Position + Normal * Bias, Frame.SunDirection, 0.5 * Bias, 80.0, 6.0);
}

// 📐 Merges the sun into an existing reservoir. Called after the indirect candidate and before temporal reuse so the
//    temporal pass carries the combined estimate forward; the tally carries the sun's one proposal too, so the spatial
//    pass's 1/Z correction counts the unified domain correctly.
fn MergeUnifiedSun(Target: ptr<function, Reservoir>, Position: vec3f, Normal: vec3f, Random: f32) -> bool
{
    let Density = ResolveUnifiedSunDensity(Normal);
    let Visibility = ResolveUnifiedSunVisibility(Position, Normal);
    let Weight = ResolveLuminance(Frame.SunIlluminance) * max(dot(Normal, Frame.SunDirection), 0.0) * Visibility
               / max(Density, 1.0e-4);
    return MergeReservoir(Target, ConstructSunCandidate(Position), select(0.0, Weight, Visibility > 0.0), 1.0, Random);
}
