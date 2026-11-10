//============================================================================================================================================
//                                                    ENHANCEDRESERVOIRMODEL.JS
//============================================================================================================================================
// 🧩 A CPU-side model of the ReSTIR PT Enhanced reservoir decisions, sharing the shader's arithmetic exactly.
//
// 📝 The headless harness runs Dawn's null backend, which validates every WGSL module but executes nothing, so nothing in
//    CI can otherwise observe what a technique actually does. These functions mirror the WGSL one for one — the same
//    formulas, the same constants, the same gates — so the behaviour of each enhancement is testable: a reciprocal pair
//    really is mutual, the footprint range really widens with distance, the duplication penalty really is 1/N at full
//    strength, and the unified reservoir really does hold the sun's radiance without counting it twice. When a shader
//    constant changes, this file changes with it and its tests are the tripwire.

export const SunIdentity = 4294967294;   // EnhancedFrame.wgsl

// 📐 ResolveRayFootprint: frustum height at the view depth over the trace resolution.
export function ResolveRayFootprint(ViewDepth, TraceExtentY)
{
    return Math.max(ViewDepth, 0.05) * (1.0 / TraceExtentY) * 2.0;
}

// 📐 ResolveFootprintJacobianRange: the adaptive acceptance range, widened by the footprint-scale slider.
export function ResolveFootprintJacobianRange(Footprint, FootprintScale)
{
    const Scale = Math.min(4.0, Math.max(0.25, FootprintScale));
    const Spread = Math.max(Math.min(16.0, Math.max(2.0, 2.0 + Footprint * 8.0)) * Scale, 1.0);
    return [1.0 / Spread, Spread];
}

// 📐 IsFootprintCoherent: a sample far beyond the receiver's depth is refused before the Jacobian.
export function IsFootprintCoherent(SampleDistance, ViewDepth, Footprint, FootprintScale)
{
    return SampleDistance < ViewDepth + Footprint * 24.0 * Math.min(4.0, Math.max(0.25, FootprintScale));
}

// 📐 DiscrepancySequence.wgsl's sample stream, reproduced bit for bit: the PCG-RXS-M-XS permutation, the nested
//    per-pixel seed and the top-24-bit uniform, so the model's offsets are the shader's offsets.
export function PermuteUnsigned(Input)
{
    const Advanced = (Math.imul(Input >>> 0, 747796405) + 2891336453) >>> 0;
    const Word = (Math.imul(((Advanced >>> ((Advanced >>> 28) + 4)) ^ Advanced) >>> 0, 277803737)) >>> 0;
    return ((Word >>> 22) ^ Word) >>> 0;
}

function AdvanceUniform(Seed)
{
    Seed.Value = PermuteUnsigned(Seed.Value);
    return (Seed.Value >>> 8) * (1.0 / 16777216.0);
}

export function ConstructPixelSeed(X, Y, Frame, Stream)
{
    const Nested = PermuteUnsigned((Frame + PermuteUnsigned(Stream)) >>> 0);
    const Rows = PermuteUnsigned((Y + Nested) >>> 0);
    return { Value: PermuteUnsigned((X + Rows) >>> 0) };
}

// 📐 ResolveReciprocalOffset: a deterministic offset whose magnitude follows the spatial radius.
export function ResolveReciprocalOffset(Pixel, Proposal, Frame, SpatialRadius)
{
    const Seed = ConstructPixelSeed(Math.max(Pixel[0], 0), Math.max(Pixel[1], 0), Frame, 41 + Proposal * 7);
    const X = AdvanceUniform(Seed) * 2.0 - 1.0;
    const Y = AdvanceUniform(Seed) * 2.0 - 1.0;
    const Radius = Math.max(SpatialRadius * 0.5, 1.0);
    return [Math.round(X * Radius), Math.round(Y * Radius)];
}

// 📐 IsReciprocalPair: the donor's own offset for the same proposal must land back within one texel.
export function IsReciprocalPair(Receiver, Donor, Proposal, Frame, SpatialRadius)
{
    const Back = ResolveReciprocalOffset(Donor, Proposal, Frame, SpatialRadius);
    return Math.max(Math.abs(Back[0] + Donor[0] - Receiver[0]), Math.abs(Back[1] + Donor[1] - Receiver[1])) <= 1;
}

// 📐 ResolveDuplicationPenalty: 1 / (1 + strength · (N − 1)).
export function ResolveDuplicationPenalty(DuplicationCount, Strength)
{
    return 1.0 / (1.0 + Math.min(1.0, Math.max(0.0, Strength)) * Math.max(DuplicationCount - 1.0, 0.0));
}

// 📐 CompressSamplePosition: quantise toward the receiver on a grid set by the compression level.
export function CompressSamplePosition(Position, Receiver, Footprint, Level)
{
    const Clamped = Math.min(1.0, Math.max(0.0, Level));
    const Step = Math.max(Footprint * 0.25, 1.0e-4) * (1.0 + Clamped * 3.0);
    return Position.map((Axis, Index) => Receiver[Index] + Math.round((Axis - Receiver[Index]) / Step) * Step);
}

// 📐 ResolveUnifiedSunDensity: cosine lobe of exponent UnifiedBalance about the sun direction.
export function ResolveUnifiedSunDensity(Normal, SunDirection, UnifiedBalance)
{
    const Dot = Normal[0] * SunDirection[0] + Normal[1] * SunDirection[1] + Normal[2] * SunDirection[2];
    return Math.pow(Math.min(1.0, Math.max(0.0, Dot)), Math.min(4.0, Math.max(0.0, UnifiedBalance)));
}

// 📐 MergeUnifiedSun's weight: the solar radiance over the proposal density, times the receiver's cosine and visibility.
export function ResolveUnifiedSunWeight(Normal, SunDirection, SunIlluminance, Visibility, UnifiedBalance)
{
    const Dot = Normal[0] * SunDirection[0] + Normal[1] * SunDirection[1] + Normal[2] * SunDirection[2];
    if (Dot <= 0.0 || Visibility <= 0.0)
        return 0.0;
    const Luminance = 0.2126 * SunIlluminance[0] + 0.7152 * SunIlluminance[1] + 0.0722 * SunIlluminance[2];
    return Luminance * Dot * Visibility / Math.max(ResolveUnifiedSunDensity(Normal, SunDirection, UnifiedBalance), 1.0e-4);
}

// 📐 Weighted reservoir sampling, as MergeReservoir writes it: accumulate, then keep the candidate with probability
//    w / Σw. Returns the reservoir after the merge so the caller can chain proposals.
export function MergeReservoir(Target, Candidate, Weight, Tally, Random)
{
    Target.WeightSum += Weight;
    Target.Tally += Tally;
    if (Weight > 0.0 && Random * Target.WeightSum < Weight)
    {
        Target.Sample = Candidate;
        return true;
    }
    return false;
}

export function ConstructEmptyReservoir()
{
    return { Sample: null, WeightSum: 0.0, Tally: 0.0, Adopted: false };
}

// 📐 W = Σw / (M · p̂(y)): the unbiased contribution weight the resolve divides by.
export function ResolveReservoirWeight(WeightSum, Tally, Target)
{
    return Target > 0.0 && Tally > 0.0 ? WeightSum / (Tally * Target) : 0.0;
}
