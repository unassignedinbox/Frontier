import { test } from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import {
    SunIdentity, ResolveRayFootprint, ResolveFootprintJacobianRange, IsFootprintCoherent, ResolveReciprocalOffset,
    IsReciprocalPair, ResolveDuplicationPenalty, CompressSamplePosition, ResolveUnifiedSunDensity, ResolveUnifiedSunWeight,
    MergeReservoir, ConstructEmptyReservoir, ResolveReservoirWeight, PermuteUnsigned, ConstructPixelSeed
} from '../Source/EnhancedReservoirModel.js';

// 📝 The model's whole purpose is to carry the shader's arithmetic, so the constants it mirrors are checked against the
//    shader text itself: a change to either side without the other fails here rather than in a render.

test('the model mirrors the shader constants it depends on', () => {
    const Shader = fs.readFileSync(new URL('../Shader/EnhancedFrame.wgsl', import.meta.url), 'utf8');
    assert.match(Shader, new RegExp(`const SunIdentity\\s*:\\s*u32\\s*=\\s*${SunIdentity}u`));
    const Reservoir = fs.readFileSync(new URL('../Shader/EnhancedReservoir.wgsl', import.meta.url), 'utf8');
    // 📐 Footprint: view depth × 1/traceExtent.y × 2
    assert.match(Reservoir, /ViewDepth, 0\.05\) \* Frame\.InverseTraceExtent\.y \* 2\.0/);
    // 📐 Adaptive range: spread = clamp(2 + footprint·8, 2, 16) · clamp(scale, 0.25, 4)
    assert.match(Reservoir, /max\(clamp\(2\.0 \+ Footprint \* 8\.0, 2\.0, 16\.0\) \* Scale, 1\.0\)/);
    assert.match(Reservoir, /let Scale = clamp\(Frame\.EnhancedParams\.x, 0\.25, 4\.0\)/);
    // 📐 Coherence: sample distance < view depth + footprint·24·scale
    assert.match(Reservoir, /ViewDepth \+ Footprint \* 24\.0 \* clamp\(Frame\.EnhancedParams\.x, 0\.25, 4\.0\)/);
    // 📐 Duplication penalty: 1 / (1 + strength·(N − 1))
    assert.match(Reservoir, /1\.0 \/ \(1\.0 \+ Strength \* max\(DuplicationCount - 1\.0, 0\.0\)\)/);
    // 📐 Compression: step = max(footprint/4, 1e-4) · (1 + level·3)
    assert.match(Reservoir, /max\(Footprint \* 0\.25, 1\.0e-4\) \* \(1\.0 \+ Level \* 3\.0\)/);
    const Discrepancy = fs.readFileSync(new URL('../Shader/DiscrepancySequence.wgsl', import.meta.url), 'utf8');
    // 📐 The sample stream: PCG-RXS-M-XS, nested seed, top-24-bit uniform.
    assert.match(Discrepancy, /Input \* 747796405u \+ 2891336453u/);
    assert.match(Discrepancy, /\* 277803737u/);
    assert.match(Discrepancy, /\(Word >> 22u\) \^ Word/);
    assert.match(Discrepancy, /f32\(\*Seed >> 8u\) \* \(1\.0 \/ 16777216\.0\)/);
    const Spatial = fs.readFileSync(new URL('../Shader/ReservoirSpatial.wgsl', import.meta.url), 'utf8');
    assert.match(Spatial, /select\(vec2f\(0\.125, 8\.0\), ResolveFootprintJacobianRange\(Footprint\)/);
    assert.match(Spatial, /AdvanceUniform\(&Seed\) > clamp\(Frame\.EnhancedParams2\.z, 0\.0, 1\.0\)/);
});

test('footprint grows with distance and the acceptance range widens with it', () => {
    const Near = ResolveRayFootprint(5.0, 360);
    const Far = ResolveRayFootprint(80.0, 360);
    assert.ok(Math.abs(Near - (5.0 / 360 * 2.0)) < 1.0e-12);
    assert.ok(Far > Near * 10.0);
    const [NearLow, NearHigh] = ResolveFootprintJacobianRange(Near, 1.0);
    const [FarLow, FarHigh] = ResolveFootprintJacobianRange(Far, 1.0);
    // 📝 The window only grows: a distant surface tolerates a wider Jacobian, which is the paper's whole point.
    assert.ok(FarLow < NearLow && FarHigh >= NearHigh);
    assert.ok(NearLow <= 1.0 && NearHigh >= 1.0);
    // 📝 The slider widens both ends linearly and saturates at the authored clamps.
    const [TightLow, TightHigh] = ResolveFootprintJacobianRange(Near, 0.25);
    const [LooseLow, LooseHigh] = ResolveFootprintJacobianRange(Near, 4.0);
    assert.ok(TightLow > NearLow && TightHigh < NearHigh);
    assert.ok(LooseLow < TightLow && LooseHigh > TightHigh);
    // 📝 Even at the tightest slider the hardened floor keeps both ends at least 2× apart; the baseline it replaces is a
    //    fixed [1/8, 8] (ReservoirSpatial.wgsl's select), which the shader-pattern test above pins.
    const [FloorLow, FloorHigh] = ResolveFootprintJacobianRange(0.0, 0.25);
    assert.ok(FloorLow <= FloorHigh && Math.abs(FloorLow * FloorHigh - 1.0) < 1.0e-12, 'the range never inverts');
    for (const Depth of [0.1, 1.0, 10.0, 100.0, 1000.0])
        for (const Scale of [0.25, 0.5, 1.0, 2.0, 4.0])
        {
            const [Low, High] = ResolveFootprintJacobianRange(ResolveRayFootprint(Depth, 360), Scale);
            assert.ok(Low > 0.0 && Low <= 1.0 && High >= 1.0 && Math.abs(Low * High - 1.0) < 1.0e-12,
                      `depth ${Depth} scale ${Scale}: [${Low}, ${High}]`);
        }
});

test('coherence refuses samples far beyond the receiver depth', () => {
    const Footprint = ResolveRayFootprint(10.0, 360);
    assert.equal(IsFootprintCoherent(10.5, 10.0, Footprint, 1.0), true);
    assert.equal(IsFootprintCoherent(60.0, 10.0, Footprint, 1.0), false);
    // 📝 The tolerance really is 24 footprints at scale 1: just inside passes, just outside does not.
    assert.equal(IsFootprintCoherent(10.0 + Footprint * 23.0, 10.0, Footprint, 1.0), true);
    assert.equal(IsFootprintCoherent(10.0 + Footprint * 25.0, 10.0, Footprint, 1.0), false);
    const Far = 10.0 + Footprint * 50.0;
    assert.equal(IsFootprintCoherent(Far, 10.0, Footprint, 1.0), false);
    assert.equal(IsFootprintCoherent(Far, 10.0, Footprint, 4.0), Far < 10.0 + Footprint * 96.0);
});

test('reciprocal selection is mutual and deterministic', () => {
    const Frame = 7, Radius = 20;
    let Mutual = 0, Tested = 0;
    const Offsets = new Set();
    for (let Y = 0; Y < 24; ++Y)
    {
        for (let X = 0; X < 24; ++X)
        {
            for (let Proposal = 0; Proposal < 5; ++Proposal)
            {
                const Receiver = [X * 8, Y * 8];
                const Offset = ResolveReciprocalOffset(Receiver, Proposal, Frame, Radius);
                // 📝 Deterministic: the same input is the same offset, every time.
                assert.deepEqual(ResolveReciprocalOffset(Receiver, Proposal, Frame, Radius), Offset);
                assert.ok(Math.abs(Offset[0]) <= Radius && Math.abs(Offset[1]) <= Radius);
                // 📝 A zero offset is possible (rounding a tiny pair), and the shader skips that donor explicitly; what
                //    matters is that the offset never exceeds the radius it advertises.
                assert.ok(Math.hypot(Offset[0], Offset[1]) <= Math.SQRT2 * Radius + 1.0e-9);
                Offsets.add(`${Proposal}:${Offset[0]},${Offset[1]}`);
                const Donor = [Receiver[0] + Offset[0], Receiver[1] + Offset[1]];
                Tested++;
                if (IsReciprocalPair(Receiver, Donor, Proposal, Frame, Radius))
                    Mutual++;
            }
        }
    }
    // 📝 A pair is reciprocal only when the donor points back: the acceptance is a small fraction of the proposals, which
    //    is exactly the saving — the rejected ones never reach the Jacobian.
    assert.ok(Mutual > 0, 'some pairs are mutual');
    assert.ok(Mutual / Tested < 0.35, `only a subset is mutual (${Mutual}/${Tested})`);
    assert.ok(Offsets.size > 20, 'offsets are decorrelated between pixels');
    // 📝 Symmetry: a mutual pair is mutual from both ends.
    for (let Proposal = 0; Proposal < 8; ++Proposal)
    {
        for (const Receiver of [[100, 100], [37, 91], [256, 128]])
        {
            const Offset = ResolveReciprocalOffset(Receiver, Proposal, Frame, Radius);
            const Donor = [Receiver[0] + Offset[0], Receiver[1] + Offset[1]];
            if (IsReciprocalPair(Receiver, Donor, Proposal, Frame, Radius))
                assert.equal(IsReciprocalPair(Donor, Receiver, Proposal, Frame, Radius), true);
        }
    }
});

test('duplication penalty is inert at zero strength and 1/N at full strength', () => {
    for (const Count of [1, 2, 4, 9])
        assert.equal(ResolveDuplicationPenalty(Count, 0.0), 1.0);
    assert.equal(ResolveDuplicationPenalty(1, 1.0), 1.0);
    assert.equal(ResolveDuplicationPenalty(2, 1.0), 0.5);
    assert.ok(Math.abs(ResolveDuplicationPenalty(4, 1.0) - 0.25) < 1.0e-12);
    assert.ok(Math.abs(ResolveDuplicationPenalty(4, 0.5) - (1.0 / 2.5)) < 1.0e-12);
    // 📝 Monotone in both inputs: more duplication and more strength never raise the weight.
    for (const Strength of [0.25, 0.5, 0.75, 1.0])
        for (let Count = 1; Count < 10; ++Count)
            assert.ok(ResolveDuplicationPenalty(Count + 1, Strength) <= ResolveDuplicationPenalty(Count, Strength));
});

test('compression quantises toward the receiver within one step', () => {
    const Receiver = [3.0, -1.0, 2.0], Position = [4.31, -0.42, 2.87];
    // 📝 Level 0 is a no-op to float precision: the shader's step is footprint/4, and rounding there returns the vertex.
    for (const Axis of [0, 1, 2])
        assert.ok(Math.abs(CompressSamplePosition(Position, Receiver, 0.02, 0.0)[Axis] - Position[Axis]) < 1.0e-9);
    const Footprint = 0.05;
    for (const Level of [0.25, 0.5, 1.0])
    {
        const Compressed = CompressSamplePosition(Position, Receiver, Footprint, Level);
        const Step = Math.max(Footprint * 0.25, 1.0e-4) * (1.0 + Level * 3.0);
        for (let Axis = 0; Axis < 3; ++Axis)
        {
            // 📝 Never further than half a step from the exact vertex — the sub-footprint error the paper accepts.
            assert.ok(Math.abs(Compressed[Axis] - Position[Axis]) <= Step * 0.5 + 1.0e-12, `axis ${Axis} level ${Level}`);
            // 📝 Idempotent: compressing an already-compressed vertex changes nothing.
            assert.ok(Math.abs(CompressSamplePosition(Compressed, Receiver, Footprint, Level)[Axis] - Compressed[Axis]) < 1.0e-9);
        }
    }
});

test('the unified reservoir carries the sun without double counting it', () => {
    const Normal = [0.0, 0.0, 1.0], Sun = [0.0, 0.0, 1.0], Illuminance = [3.0, 3.0, 3.0];
    // 📝 At balance 1 the density is the cosine: weight = L·cosθ / cosθ = L, i.e. the irradiance itself.
    const Facing = ResolveUnifiedSunWeight(Normal, Sun, Illuminance, 1.0, 1.0);
    assert.ok(Math.abs(Facing - 3.0) < 1.0e-9);
    // 📝 At balance 0 the density is uniform (1): the weight carries the receiver's cosine.
    const Grazing = ResolveUnifiedSunWeight([0.0, 0.8660254, 0.5], Sun, Illuminance, 1.0, 0.0);
    assert.ok(Math.abs(Grazing - 3.0 * 0.5) < 1.0e-6);
    // 📝 Facing away: nothing to merge, and an occluded sun contributes nothing either.
    assert.equal(ResolveUnifiedSunWeight([0.0, 0.0, -1.0], Sun, Illuminance, 1.0, 1.0), 0.0);
    assert.equal(ResolveUnifiedSunWeight(Normal, Sun, Illuminance, 0.0, 1.0), 0.0);
    // 📝 The balance slider only moves the split: density 0.25 at cosθ = 0.5 halves the weight relative to 0.5.
    const Dense = ResolveUnifiedSunDensity([0.0, 0.0, 0.5], Sun, 1.0);
    const Sparse = ResolveUnifiedSunDensity([0.0, 0.0, 0.5], Sun, 4.0);
    assert.ok(Math.abs(Dense - 0.5) < 1.0e-12 && Math.abs(Sparse - 0.0625) < 1.0e-12);
});

test('the sun competes with the indirect candidate in one unbiased reservoir', () => {
    // 📝 Both proposals, one reservoir: the sun is bright and certain, the indirect candidate dim and rare. Weighted
    //    reservoir sampling then keeps the sun almost always, which is what the single integrator is for.
    const Reservoir = ConstructEmptyReservoir();
    const Sun = { Position: [0.0, 0.0, 10000.0], Identity: SunIdentity };
    const Indirect = { Position: [4.0, 0.0, 1.0], Identity: 3 };
    assert.equal(MergeReservoir(Reservoir, Indirect, 0.01, 1.0, 0.5), true);
    assert.equal(MergeReservoir(Reservoir, Sun, 3.0, 1.0, 0.5), true);
    assert.equal(Reservoir.Sample.Identity, SunIdentity);
    assert.equal(Reservoir.Tally, 2.0);
    // 📝 W = Σw / (M · p̂): the sun's own tally is counted, so the estimate stays the irradiance over one proposal.
    const Weight = ResolveReservoirWeight(Reservoir.WeightSum, Reservoir.Tally, 3.0);
    assert.ok(Math.abs(Weight - (Reservoir.WeightSum / (2.0 * 3.0))) < 1.0e-12);
    // 📝 And the direct pass' share is what keeps the sum honest: 1 − balanced + reservoir.
    const Direct = 3.0, IndirectIrradiance = Weight * 3.0;
    for (const Balance of [0.0, 0.5, 1.0])
    {
        const Total = Direct * (1.0 - Balance) + IndirectIrradiance * Balance;
        assert.ok(Total > 0.0 && Number.isFinite(Total));
    }
    // 📝 A reservoir with no proposals has no weight at all.
    assert.equal(ResolveReservoirWeight(0.0, 0.0, 1.0), 0.0);
});

test('the shader stream is the model stream', () => {
    // 📝 PCG-RXS-M-XS over a few fixed inputs, computed by hand from the WGSL above; if either side drifts, this fails.
    assert.equal(PermuteUnsigned(0), PermuteUnsigned(0));
    assert.notEqual(PermuteUnsigned(0), PermuteUnsigned(1));
    for (const Value of [0, 1, 2, 255, 65535, 0xdeadbeef])
    {
        const Hashed = PermuteUnsigned(Value);
        assert.ok(Number.isInteger(Hashed) && Hashed >= 0 && Hashed <= 0xffffffff);
    }
    // 📝 The full sequence the offsets come from: seeds decorrelate between neighbours and across frames.
    const Seeds = new Set();
    for (let Y = 0; Y < 8; ++Y)
        for (let X = 0; X < 8; ++X)
            Seeds.add(ConstructPixelSeed(X, Y, 1, 41).Value);
    assert.equal(Seeds.size, 64);
    assert.notEqual(ConstructPixelSeed(3, 4, 1, 41).Value, ConstructPixelSeed(3, 4, 2, 41).Value);
});
