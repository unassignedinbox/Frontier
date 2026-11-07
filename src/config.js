/**
 * FRONTIER — tuning data.
 *
 * Every "feel" number lives here on purpose. When this prototype is ported to
 * Unreal/Unity these become the designer-facing data asset (a DataTable /
 * ScriptableObject), and the sim code below does not change.
 * Units are metres, seconds, kilograms-ish. Speeds shown to the player are
 * km/h = m/s * 3.6.
 */

export const CFG = {
  // ---- vehicle ------------------------------------------------------------
  craft: {
    hoverHeight: 6.0,        // ride height above terrain
    thrust: 26,              // m/s^2 at full throttle -> ~6s to top speed, so acceleration is felt
    brakeForce: 42,          // m/s^2 of air-brake deceleration
    topSpeed: 128,           // m/s cruise ceiling (~460 km/h)
    topSpeedSurge: 172,      // during Tachyon Surge (~620 km/h)
    linearDrag: 0.055,       // exponential drag coefficient
    quadDrag: 0.00042,       // speed^2 drag -> creates a natural terminal speed
    grip: 5.2,               // how fast lateral slip is scrubbed off (higher = more grip)
    gripDrift: 2.6,          // grip while holding drift -> ~38 deg slip angle, a real slide
    gripFloor: 0.35,         // grip multiplier when scraping terrain
    yawRate: 1.85,           // rad/s at reference speed
    yawRateHighSpeed: 1.15,  // rad/s at top speed (steering tightens up as you slow)
    yawRateSpeedRef: 42,     // speed at which we are fully in yawRate
    pitchRate: 1.15,
    pitchLimit: 0.42,        // radians of climb/dive authority
    bankAmount: 0.95,        // visual roll into a turn
    bankResponse: 7.0,
    driftFlowRate: 26,      // flow gained per second of sustained drift
    nearMissFlow: 9,         // flow gained per skimming pass
    nearMissRadius: 22,      // how close counts as a skim
    crashBleed: 0.35,        // speed retained after hitting something solid
    impactShake: 1.0
  },

  // ---- resources ----------------------------------------------------------
  boost: {
    max: 100,
    thrustMul: 1.85,
    capMul: 1.22,            // boost lifts the speed ceiling while it burns
    drain: 34,               // per second while burning
    gateReward: 26,
    draftPerSecond: 14,      // regen while tucked behind a rival
    draftRange: 55,
    draftCone: 0.55          // radians half angle
  },

  flow: {
    max: 100,
    decay: 3.2,              // per second when not earning
    gateBase: 12,            // scaled by combo + speed
    surgeDuration: 3.4,
    surgeTimeDilation: 0.55, // world slows during surge (player stays fast)
    comboWindow: 5.5,        // seconds to chain gates before combo resets
    comboMax: 12
  },

  // ---- rivals -------------------------------------------------------------
  rivals: [
    { name: 'VANTA',    tag: 'CLEAN LINE',  color: 0xff4d6d, skill: 0.965, aggression: 0.15, boostUse: 0.55, lineOffset: 0.0 },
    { name: 'KESTREL',  tag: 'AGGRO',       color: 0xffb020, skill: 0.935, aggression: 0.80, boostUse: 0.80, lineOffset: 0.0 },
    { name: 'ORPHEUS',  tag: 'FLOW HUNTER', color: 0x7cffb2, skill: 0.905, aggression: 0.35, boostUse: 0.95, lineOffset: 0.0 }
  ],
  rivalTopSpeedMul: 1.0,

  // ---- course -------------------------------------------------------------
  course: {
    seed: 20261107,
    nodeSpacing: 95,         // metres between spline control nodes
    nodes: 140,              // total control nodes (~13.5 km of corridor, a ~2 min run)
    wander: 1.0,             // lateral wander multiplier
    climb: 1.0,              // vertical wander multiplier
    halfWidth: 120,          // canyon floor half width at the racing line
    gateEvery: 4,            // place a gate every N nodes -> ~34 rings per run
    gateRadiusMin: 24,       // rings must be readable and fair at 460 km/h
    gateRadiusMax: 40,
    gateLateral: 0.62,       // how far off the centreline gates may sit (0..1 of halfWidth)
    gateVertical: 46,        // metres of vertical offset either side of the spline
    pylonEvery: 2,
    scenerySeedOffset: 991
  },

  // ---- camera -------------------------------------------------------------
  camera: {
    distance: 26,
    height: 9.5,
    lookAhead: 26,
    posDamp: 7.5,
    lookDamp: 11.0,
    fovBase: 74,
    fovSpeedGain: 0.30,      // extra degrees per (speed / topSpeed) * 20
    fovSurge: 16,
    fovBoost: 11,
    shakeDecay: 5.5
  },

  // ---- quality ------------------------------------------------------------
  quality: {
    bloom: true,
    bloomStrength: 0.85,
    bloomRadius: 0.72,
    bloomThreshold: 0.62,
    streaks: 260,
    pixelRatioCap: 1.6,
    shadows: false
  }
};

export const MS_TO_KMH = 3.6;
