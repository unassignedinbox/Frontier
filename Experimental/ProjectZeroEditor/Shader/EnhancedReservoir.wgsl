//============================================================================================================================================
//                                                          ENHANCEDRESERVOIR.WGSL
//============================================================================================================================================
// 🧩 ReSTIR PT Enhanced (Lin et al. 2026, I3D Best Paper): the reservoir techniques that take the 2022 ReSTIR PT
//    baseline to the reported 2.08–3.05× speedup with better quality. This file holds the shared helpers; the two
//    reservoir programs include it and gate each technique on its own feature bit, so every enhancement can be
//    isolated from the inspector and the baseline path stays byte-identical when the master switch is off.
//
//    Techniques implemented here:
//    ① reciprocal neighbour selection — spatial proposals are mutual pairs, so non-mutual proposals leave before
//       the expensive Jacobian evaluation (halves shift-mapping cost, the paper's headline optimisation);
//    ② footprint-based reconnection — the Jacobian acceptance range adapts to the ray footprint at the receiver
//       instead of one fixed [1/8, 8] clamp, and incoherent samples (far beyond the receiver's depth) are refused;
//    ③ duplication map — a per-pixel count of how many neighbours adopted the same sample; heavily duplicated
//       samples are down-weighted, breaking the correlation spatial reuse would otherwise smear across the frame;
//    ④ lossy reservoir compression — the reconnection vertex is quantised toward the receiver before storage,
//       trading sub-texel position error for the smaller effective reservoir footprint the paper reports
//       (88 B → 64 B per reservoir in their implementation; this exhibit's record is already 32 B packed).

//! include "EnhancedFrame.wgsl"
//! include "ReservoirRecords.wgsl"

// 📐 World-space footprint of one resampling pixel at a given view depth: the frustum height at that depth divided
//    by the trace resolution. Reconnection coherence is judged against this, not against a fixed metric range.
fn ResolveRayFootprint(ViewDepth: f32) -> f32
{
    return max(ViewDepth, 0.05) * Frame.InverseTraceExtent.y * 2.0;
}

// 📐 Adaptive Jacobian acceptance range. A reconnection is coherent when the sample's footprint at the receiver is
//    within an order of magnitude of the receiver's own footprint; the range widens with the footprint scale slider
//    (EnhancedParams.x) and narrows for distant, finely-sampled surfaces where shifts go incoherent fastest.
fn ResolveFootprintJacobianRange(Footprint: f32) -> vec2f
{
    let Scale = clamp(Frame.EnhancedParams.x, 0.25, 4.0);
    // 📝 Floored at 1: the scale shrinks the tolerance toward the isotropic limit but never below it, or the range would
    //    invert (1/Spread > Spread) and reject every proposal.
    let Spread = max(clamp(2.0 + Footprint * 8.0, 2.0, 16.0) * Scale, 1.0);
    return vec2f(1.0 / Spread, Spread);
}

// 📐 A sample is incoherent for this receiver when it lies far beyond the receiver's depth: its footprint there is
//    many times the receiver's own, so the reconnection shift would smear light across a depth discontinuity.
fn IsFootprintCoherent(SampleDistance: f32, ViewDepth: f32, Footprint: f32) -> bool
{
    return SampleDistance < ViewDepth + Footprint * 24.0 * clamp(Frame.EnhancedParams.x, 0.25, 4.0);
}

// 📝 Reciprocal neighbour selection: each pixel derives a deterministic proposal offset from its own coordinates,
//    the proposal index and the frame. A donor is reciprocal when the donor's own deterministic offset for the same
//    proposal index lands back on the receiver (within one texel). Mutual pairs share one shift mapping; every
//    non-mutual proposal is refused before the reservoir unpack and Jacobian evaluation, which is where the cost
//    halves. The offset magnitude follows the spatial radius so the pair distance matches the reuse kernel.
fn ResolveReciprocalOffset(Pixel: vec2i, Proposal: u32) -> vec2i
{
    var Seed = ConstructPixelSeed(vec2u(max(Pixel, vec2i(0))), Frame.FrameIndex, 41u + Proposal * 7u);
    let Pair = vec2f(AdvanceUniform(&Seed), AdvanceUniform(&Seed)) * 2.0 - 1.0;
    let Radius = max(Frame.SpatialRadius * 0.5, 1.0);
    return vec2i(round(Pair * Radius));
}

fn IsReciprocalPair(Receiver: vec2i, Donor: vec2i, Proposal: u32) -> bool
{
    let Back = Donor + ResolveReciprocalOffset(Donor, Proposal);
    return max(abs(Back.x - Receiver.x), abs(Back.y - Receiver.y)) <= 1;
}

// 📐 Identity hash of a reservoir sample: quantised position + occupant. Two pixels that adopted the same sample
//    produce the same hash, which is what the duplication map counts.
fn HashSampleIdentity(Sample: ReservoirSample) -> u32
{
    let Cell = vec3i(round(Sample.Position * 4.0));
    var Hash = u32(Cell.x * 73856093) ^ u32(Cell.y * 19349663) ^ u32(Cell.z * 83492791);
    Hash ^= Sample.Identity * 2654435761u;
    return PermuteUnsigned(Hash);
}

// 📐 Duplication penalty: a sample adopted by N pixels carries 1/(1 + strength·(N−1)) of its weight. At strength 0
//    the map is inert; at 1 a sample shared by four pixels keeps a quarter of its weight, which decorrelates the
//    spatial reuse without biasing the 1/Z correction (the tally still counts every domain).
fn ResolveDuplicationPenalty(DuplicationCount: f32) -> f32
{
    let Strength = clamp(Frame.EnhancedParams.y, 0.0, 1.0);
    return 1.0 / (1.0 + Strength * max(DuplicationCount - 1.0, 0.0));
}

// 📐 Lossy position compression: quantise the reconnection vertex toward the receiver on a grid scaled by the
//    compression level (EnhancedParams2.y, 0 = off, 1 = full). Sub-texel error at the receiver's footprint, so the
//    stored vertex survives the round trip through the 32-byte record without visible shift.
fn CompressSamplePosition(Position: vec3f, Receiver: vec3f, Footprint: f32) -> vec3f
{
    let Level = clamp(Frame.EnhancedParams2.y, 0.0, 1.0);
    let Step = max(Footprint * 0.25, 1.0e-4) * (1.0 + Level * 3.0);
    return Receiver + round((Position - Receiver) / Step) * Step;
}
