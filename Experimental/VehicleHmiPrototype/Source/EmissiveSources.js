//============================================================================================================================================
//                                                          EMISSIVESOURCES.JS
//============================================================================================================================================
// 🧩 Emissive proxies: each hot particle (a spark ground off the rims and body) becomes a point emitter for the emissive
//    cluster system. The light it casts follows its blackbody colour and temperature, so it cools as the spark does.

import { LightType } from "./LightingSystem.js";

const EmissiveReach      = 1.5;     // [m] influence radius of one spark's light
const EmissiveFloorK     = 1100.0;  // [K] below this a spark is too cool to register as a light
const EmissiveGain       = 0.25;    // [cd per 1000 K above ambient] calibrated against the 45 cd street lamps

// 📐 Tanner Helland's fit of the Planckian locus, normalised to the brightest channel (0…1 per channel).
export function BlackbodyColour(Kelvin)
{
    const T = Math.min(40000.0, Math.max(1000.0, Kelvin)) / 100.0;
    const Red   = T <= 66.0 ? 255.0 : 329.698727446 * Math.pow(T - 60.0, -0.1332047592);
    const Green = T <= 66.0 ? 99.4708025861 * Math.log(T) - 161.1195681661 : 288.1221695283 * Math.pow(T - 60.0, -0.0755148492);
    const Blue  = T >= 66.0 ? 255.0 : (T <= 19.0 ? 0.0 : 138.5177312231 * Math.log(T - 10.0) - 305.0447927307);
    const Clamp = (Value) => Math.min(255.0, Math.max(0.0, Value)) / 255.0;
    return [Clamp(Red), Clamp(Green), Clamp(Blue)];
}

// 📝 Brightest first, capped at the emissive slot count. Particles that have not yet started to glow are skipped.
export function GatherEmissiveLights(Particles, Limit = 64)
{
    const Emitters = [];
    for (const Particle of Particles)
    {
        if (!Particle || !Particle.Position || !(Particle.Kelvin > EmissiveFloorK))
            continue;
        const Heat = Particle.Kelvin - 293.0;
        Emitters.push({
            Type: LightType.Omni,
            Position: Particle.Position.slice(),
            Direction: [0.0, 0.0, -1.0],
            Colour: BlackbodyColour(Particle.Kelvin),
            Intensity: EmissiveGain * Heat / 1000.0 * (Particle.Size ?? 1.0),
            Reach: EmissiveReach,
            Params: [0.0, 0.0, 0.0, 1.0],
            Profile: null,
            Heat
        });
    }
    Emitters.sort((A, B) => B.Heat - A.Heat);
    return Emitters.slice(0, Limit).map(({ Heat, ...Rest }) => Rest);
}
