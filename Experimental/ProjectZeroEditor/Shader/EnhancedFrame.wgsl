//============================================================================================================================================
//                                                          ENHANCEDFRAME.WGSL
//============================================================================================================================================
// 🧩 Frame-level helpers of ReSTIR PT Enhanced that more than one program needs. Kept apart from EnhancedReservoir.wgsl so
//    the display programs can take the resolve-side helpers without dragging the reservoir record's traversal includes
//    into a fragment shader that already owns those binding slots.

//! include "SceneRecords.wgsl"

// 📝 The unified reservoir's sun sample carries its own identity, above every occupant id and the sky, so the shift
//    mappings treat it as directional exactly like an escaped ray and it never collides with a real surface.
const SunIdentity : u32 = 4294967294u;

// 📐 How much of the direct pass the unified reservoir has taken over: the balance slider when both the master switch
//    and the unified bit are set, zero otherwise, so the baseline two-integrator sum is untouched with the feature off.
fn ResolveUnifiedDirectShare() -> f32
{
    if (!FeatureEnabled(Frame, FeatureEnhancedRestirPT) || !FeatureEnabled(Frame, FeatureUnifiedReservoir))
    {
        return 0.0;
    }
    return clamp(Frame.EnhancedParams.z, 0.0, 1.0);
}
