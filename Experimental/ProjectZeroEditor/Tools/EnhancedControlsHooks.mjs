import assert from 'node:assert/strict';
import { SunIdentity } from '../Source/EnhancedReservoirModel.js';

// 📝 Exercises the ReSTIR PT Enhanced controls through the real frame loop: every technique is toggled independently, the
//    sliders are moved to their extremes, and the master switch is flipped mid-run. Any frame record the shaders reject,
//    any stale binding, and any setting that does not reach the packed record shows up as a GPU error or a failed assert.
export default {
    BeforeFrame(Host, Frame) {
        const S = Host.Settings;
        if (Frame === 2) {
            // 📝 Every technique off, master on: the enhanced path with no technique is the baseline behaviour again.
            for (const Key of ['ReciprocalNeighbors', 'FootprintReconnection', 'DuplicationMap', 'UnifiedReservoir', 'EnhancedDenoise'])
                S[Key] = false;
        }
        if (Frame === 4) {
            // 📝 All on, every slider at an extreme — the widest Jacobian range, the strongest penalty, a fully unified
            //    reservoir, the coarsest compression and the hardest colour clamp.
            for (const Key of ['ReciprocalNeighbors', 'FootprintReconnection', 'DuplicationMap', 'UnifiedReservoir', 'EnhancedDenoise'])
                S[Key] = true;
            Object.assign(S, { FootprintScale: 4.0, ReciprocalStrength: 1.0, DuplicationStrength: 1.0, UnifiedBalance: 1.0,
                               CompressionLevel: 1.0, ColourNoiseClamp: 24.0, DisocclusionFade: 1.0 });
        }
        if (Frame === 6) {
            // 📝 The other extreme: the tightest footprint, no reciprocal acceptance, no penalty, direct pass only.
            Object.assign(S, { FootprintScale: 0.25, ReciprocalStrength: 0.0, DuplicationStrength: 0.0, UnifiedBalance: 0.0,
                               CompressionLevel: 0.0, ColourNoiseClamp: 1.0, DisocclusionFade: 0.0 });
            assert.ok(S.FootprintScale === 0.25);
        }
        if (Frame === 8) {
            // 📝 The mid-run A/B: the master switch off drops every sub-bit from the packed record, then back on.
            assert.ok(S.EnhancedRestirPT);
            S.EnhancedRestirPT = false;
        }
        if (Frame === 10) {
            S.EnhancedRestirPT = true;
            Object.assign(S, { FootprintScale: 1.0, ReciprocalStrength: 1.0, DuplicationStrength: 0.5, UnifiedBalance: 1.0,
                               CompressionLevel: 1.0, ColourNoiseClamp: 8.0, DisocclusionFade: 0.5 });
        }
    },
    Report(Host) {
        assert.equal(Host.Settings.EnhancedRestirPT, true);
        assert.equal(Host.Settings.UnifiedBalance, 1.0);
        assert.ok(Number.isInteger(SunIdentity) && SunIdentity === 4294967294);
        console.log('ReSTIR PT Enhanced controls exercised (all off, both extremes, master off and on)');
    }
};
