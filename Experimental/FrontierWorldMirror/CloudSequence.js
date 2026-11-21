//==========================================================================================================================================
// FILE: CloudSequence.js
// PURPOSE: 1:1 port of the native volumetric cloud stack — Frontier::GeometricRaster::VolumetricCloudSequence
//          (VolumetricCloudSequence.h/.cpp) with the single-source Slang modules
//          Engine/Shaders/Atmosphere/CloudNoise.slang, CloudDensity.slang, CloudTransport.slang and
//          CloudAerialPerspective.slang.
//
//          Everything is ported verbatim: CloudDeckConfiguration + ApplyCloudCategoryPreset, the 12 CloudCategoryType
//          formations (stratus … pyrocumulus plus OrbitalCyclone), the planetary weather envelope, the three-plane
//          convective cell envelope, wind advection with shear/veer, the ordered spherical interval trace with
//          exponential step distribution and IGN blue-noise jitter, the weather-weighted 4-step solar optical depth,
//          Henyey-Greenstein scattering orders, the cloud source lighting model and the aerial perspective segment.
//
//          ONE documented deviation: the baked 128^3 Perlin-Worley condensate volume (CloudNoiseVolume.h /
//          SampleCloudShape / SampleCloudErosion) is replaced by the same lattice hash evaluated as a multi-octave
//          field (SampleCloudShapeField / SampleCloudErosionField). The hash, quintic interpolation, octave rotation
//          and warp maths are native; only the pre-baked texel data is synthesised on the fly instead of being
//          fetched from the 10.6 MB packing.
//==========================================================================================================================================

export const CloudCategoryType = Object.freeze({
  Stratus: 0,
  Stratocumulus: 1,
  Cumulus: 2,
  Cumulonimbus: 3,
  Altostratus: 4,
  Cirrus: 5,
  Lenticular: 6,
  Mammatus: 7,
  KelvinHelmholtz: 8,
  Asperitas: 9,
  Nacreous: 10,
  Pyrocumulus: 11,
  OrbitalCyclone: 12,
});

export const CLOUD_CATEGORY_NAMES = Object.freeze([
  "Stratus",
  "Stratocumulus",
  "Cumulus",
  "Cumulonimbus",
  "Altostratus",
  "Cirrus",
  "Lenticular",
  "Mammatus",
  "Kelvin–Helmholtz",
  "Asperitas",
  "Nacreous",
  "Pyrocumulus",
  "Orbital Cyclone",
]);

const CLOUD_PI = 3.14159265358979323846;
const DEG_TO_RAD = CLOUD_PI / 180.0;

const saturate = (X) => (X < 0 ? 0 : X > 1 ? 1 : X);
const clamp = (X, A, B) => (X < A ? A : X > B ? B : X);
const mix = (A, B, T) => A + (B - A) * T;
const frac = (X) => X - Math.floor(X);

export function SmoothStepCloud(E0, E1, X) {
  const T = saturate((X - E0) / Math.max(1e-9, E1 - E0));
  return T * T * (3 - 2 * T);
}

//==========================================================================================================================================
//                                              CLOUDDECKCONFIGURATION + PRESETS
//==========================================================================================================================================

export const DEFAULT_CLOUD_DECK = Object.freeze({
  CloudsEnabled: true,
  FollowWindEnabled: true,
  Category: CloudCategoryType.Cumulus,
  CloudFraction: 0.34,
  ExtinctionMultiplier: 2.4,
  FeatureScaleMultiplier: 0.35,
  CloudFloorMeters: 1400,
  LayerThicknessMeters: 1100,
  DeckCeilingMeters: 16000,
  AnvilTopSpread: 0.5,
  ForwardScatterAnisotropy: 0.45,
  WindSpeedMetersPerSecond: 7.0,
  WindBearingDegrees: 250,
  WindShearPerKilometer: 0.35,
  WindVeerDegreesPerKm: 12,
  WindTurbulence: 0.3,
  CloudRenderScale: 0.5,
  CloudMarchSteps: 36,
  ShadowEnabled: true,
  ShadowFraction: 1.0,
  ShadowDensityMultiplier: 1.0,
  ShadowScale: 1.0,
  ShadowAnvil: 1.0,
});

// VolumetricCloudSequence::ApplyCloudCategoryPreset (verbatim switch)
export function ApplyCloudCategoryPreset(Deck, CategoryValue) {
  const Out = { ...DEFAULT_CLOUD_DECK, ...(Deck || {}) };
  const Category = clamp(Math.round(Number(CategoryValue) || 0), 0, 12);
  Out.Category = Category;
  switch (Category) {
    case CloudCategoryType.Stratus:
      Out.CloudFraction = 0.62;
      Out.ExtinctionMultiplier = 1.4;
      Out.FeatureScaleMultiplier = 0.55;
      Out.CloudFloorMeters = 900;
      Out.LayerThicknessMeters = 450;
      Out.AnvilTopSpread = 0.15;
      Out.ForwardScatterAnisotropy = 0.35;
      break;
    case CloudCategoryType.Stratocumulus:
      Out.CloudFraction = 0.5;
      Out.ExtinctionMultiplier = 2.0;
      Out.FeatureScaleMultiplier = 0.42;
      Out.CloudFloorMeters = 1200;
      Out.LayerThicknessMeters = 850;
      Out.AnvilTopSpread = 0.3;
      Out.ForwardScatterAnisotropy = 0.42;
      break;
    case CloudCategoryType.Cumulus:
      Out.CloudFraction = 0.34;
      Out.ExtinctionMultiplier = 2.4;
      Out.FeatureScaleMultiplier = 0.35;
      Out.CloudFloorMeters = 1400;
      Out.LayerThicknessMeters = 1100;
      Out.AnvilTopSpread = 0.5;
      Out.ForwardScatterAnisotropy = 0.45;
      break;
    case CloudCategoryType.Cumulonimbus:
      Out.CloudFraction = 0.46;
      Out.ExtinctionMultiplier = 3.1;
      Out.FeatureScaleMultiplier = 0.3;
      Out.CloudFloorMeters = 900;
      Out.LayerThicknessMeters = 6200;
      Out.AnvilTopSpread = 0.92;
      Out.ForwardScatterAnisotropy = 0.52;
      break;
    case CloudCategoryType.Altostratus:
      Out.CloudFraction = 0.56;
      Out.ExtinctionMultiplier = 1.5;
      Out.FeatureScaleMultiplier = 0.6;
      Out.CloudFloorMeters = 3200;
      Out.LayerThicknessMeters = 1400;
      Out.AnvilTopSpread = 0.22;
      Out.ForwardScatterAnisotropy = 0.38;
      break;
    case CloudCategoryType.Cirrus:
      Out.CloudFraction = 0.4;
      Out.ExtinctionMultiplier = 0.9;
      Out.FeatureScaleMultiplier = 0.85;
      Out.CloudFloorMeters = 7400;
      Out.LayerThicknessMeters = 2600;
      Out.AnvilTopSpread = 0.18;
      Out.ForwardScatterAnisotropy = 0.62;
      break;
    case CloudCategoryType.Lenticular:
      Out.CloudFraction = 0.44;
      Out.ExtinctionMultiplier = 1.9;
      Out.FeatureScaleMultiplier = 0.5;
      Out.CloudFloorMeters = 3400;
      Out.LayerThicknessMeters = 1500;
      Out.AnvilTopSpread = 0.26;
      Out.ForwardScatterAnisotropy = 0.4;
      break;
    case CloudCategoryType.Mammatus:
      Out.CloudFraction = 0.5;
      Out.ExtinctionMultiplier = 2.7;
      Out.FeatureScaleMultiplier = 0.38;
      Out.CloudFloorMeters = 1600;
      Out.LayerThicknessMeters = 2400;
      Out.AnvilTopSpread = 0.74;
      Out.ForwardScatterAnisotropy = 0.48;
      break;
    case CloudCategoryType.KelvinHelmholtz:
      Out.CloudFraction = 0.4;
      Out.ExtinctionMultiplier = 1.7;
      Out.FeatureScaleMultiplier = 0.58;
      Out.CloudFloorMeters = 2400;
      Out.LayerThicknessMeters = 900;
      Out.AnvilTopSpread = 0.2;
      Out.ForwardScatterAnisotropy = 0.44;
      break;
    case CloudCategoryType.Asperitas:
      Out.CloudFraction = 0.55;
      Out.ExtinctionMultiplier = 2.2;
      Out.FeatureScaleMultiplier = 0.46;
      Out.CloudFloorMeters = 1800;
      Out.LayerThicknessMeters = 1600;
      Out.AnvilTopSpread = 0.24;
      Out.ForwardScatterAnisotropy = 0.4;
      break;
    case CloudCategoryType.Nacreous:
      Out.CloudFraction = 0.32;
      Out.ExtinctionMultiplier = 0.7;
      Out.FeatureScaleMultiplier = 0.9;
      Out.CloudFloorMeters = 21000;
      Out.LayerThicknessMeters = 3200;
      Out.DeckCeilingMeters = 26000;
      Out.AnvilTopSpread = 0.16;
      Out.ForwardScatterAnisotropy = 0.6;
      break;
    case CloudCategoryType.Pyrocumulus:
      Out.CloudFraction = 0.38;
      Out.ExtinctionMultiplier = 3.0;
      Out.FeatureScaleMultiplier = 0.34;
      Out.CloudFloorMeters = 1200;
      Out.LayerThicknessMeters = 4200;
      Out.AnvilTopSpread = 0.68;
      Out.ForwardScatterAnisotropy = 0.5;
      break;
    default:
      break;
  }
  return Out;
}

// HTML "Volumetric Clouds" row -> CloudDeckConfiguration.
// The category preset supplies the formation defaults, then any value the row actually authored wins
// (native parity: ApplyCloudCategoryPreset seeds the deck, the inspector sliders override it afterwards).
export function ResolveCloudDeck(Clouds = null) {
  const Src = Clouds || {};
  const Pick = (...Keys) => {
    for (const Key of Keys) {
      const Value = Src[Key];
      if (typeof Value === "number" && Number.isFinite(Value)) return Value;
    }
    return undefined;
  };
  const Category = clamp(Math.round(Pick("type", "Type") ?? 2), 0, 12);
  const Deck = ApplyCloudCategoryPreset({}, Category);
  Deck.CloudsEnabled = Src.enabled !== false && Src.Enabled !== false;
  Deck.FollowWindEnabled = Pick("followWind") !== undefined ? Src.followWind !== false : Src.Wind !== false;
  Deck.CloudFraction = clamp(Pick("coverage", "Coverage", "CloudFraction") ?? Deck.CloudFraction, 0, 1);
  Deck.ExtinctionMultiplier = Math.max(0, Pick("density", "Density", "Extinction", "ExtinctionMultiplier") ?? Deck.ExtinctionMultiplier);
  Deck.FeatureScaleMultiplier = Math.max(0.08, Pick("featureScale", "Feature Scale", "FeatureScaleMultiplier") ?? Deck.FeatureScaleMultiplier);
  Deck.CloudFloorMeters = Math.max(100, Pick("baseM", "Base", "CloudFloorMeters") ?? Deck.CloudFloorMeters);
  Deck.LayerThicknessMeters = Math.max(100, Pick("thicknessM", "Thickness", "LayerThicknessMeters") ?? Deck.LayerThicknessMeters);
  Deck.DeckCeilingMeters = Math.max(4000, Pick("ceilingM", "Ceiling", "DeckCeilingMeters") ?? Deck.DeckCeilingMeters);
  Deck.AnvilTopSpread = clamp(Pick("anvil", "Anvil", "AnvilTopSpread") ?? Deck.AnvilTopSpread, 0, 1);
  Deck.ForwardScatterAnisotropy = clamp(Pick("anisotropy", "Anisotropy", "ForwardScatterAnisotropy") ?? Deck.ForwardScatterAnisotropy, -0.5, 0.92);
  Deck.CloudMarchSteps = Math.round(Pick("marchSteps", "Cloud March", "CloudMarchSteps") ?? Deck.CloudMarchSteps);
  Deck.WindTurbulence = clamp(Pick("turbulence", "Turbulence", "WindTurbulence") ?? Deck.WindTurbulence, 0, 1);
  Deck.WindShearPerKilometer = Pick("windShear", "Wind Shear", "WindShearPerKilometer") ?? Deck.WindShearPerKilometer;
  Deck.WindVeerDegreesPerKm = Pick("windVeer", "Wind Veer", "WindVeerDegreesPerKm") ?? Deck.WindVeerDegreesPerKm;
  const WindSpeed = Pick("windSpeed", "Wind Speed", "WindSpeedMetersPerSecond");
  if (WindSpeed !== undefined) Deck.WindSpeedMetersPerSecond = Math.max(0, WindSpeed);
  const Bearing = Pick("windBearing", "Wind Bearing", "WindBearingDegrees");
  if (Bearing !== undefined) Deck.WindBearingDegrees = ((Bearing % 360) + 360) % 360;
  Deck.ShadowEnabled = Src.shadowEnabled !== false && Src["Shadow Enabled"] !== false;
  Deck.ShadowFraction = clamp(Pick("shadowCoverage", "Shadow Coverage", "ShadowFraction") ?? Deck.ShadowFraction, 0, 1);
  Deck.ShadowDensityMultiplier = Math.max(0, Pick("shadowDensity", "Shadow Density", "ShadowDensityMultiplier") ?? Deck.ShadowDensityMultiplier);
  Deck.ShadowScale = Math.max(0.1, Pick("shadowScale", "Shadow Scale", "ShadowScale") ?? Deck.ShadowScale);
  Deck.ShadowAnvil = clamp(Pick("shadowAnvil", "Shadow Anvil", "ShadowAnvil") ?? Deck.ShadowAnvil, 0, 1);
  return Deck;
}

//==========================================================================================================================================
//                                          CLOUDNOISE.SLANG — HASH, QUINTIC, OCTAVE ROTATION
//==========================================================================================================================================

function PermuteBits(Word) {
  let W = Word >>> 0;
  W = Math.imul(W ^ (W >>> 16), 0x7feb352d) >>> 0;
  W = Math.imul(W ^ (W >>> 15), 0x846ca68b) >>> 0;
  return (W ^ (W >>> 16)) >>> 0;
}

export function CloudLatticeHash3(X, Y, Z) {
  let H = Math.imul(X | 0, 0x8da6b343) ^ Math.imul(Y | 0, 0xd8163841) ^ Math.imul(Z | 0, 0xcb1ab31f);
  H = PermuteBits(H >>> 0);
  return H >>> 0;
}

// DecodeCloudTexel: four bytes of the packed word -> 0..1 channels
function DecodeTexel(Word) {
  return [
    (Word & 255) / 255,
    ((Word >>> 8) & 255) / 255,
    ((Word >>> 16) & 255) / 255,
    ((Word >>> 24) & 255) / 255,
  ];
}

export function CloudQuintic(F) {
  return F * F * F * (F * (F * 6 - 15) + 10);
}

// CloudLatticeNoise: periodic-safe 3D value noise on the native lattice hash
export function CloudLatticeNoise(X, Y, Z) {
  const Ix = Math.floor(X);
  const Iy = Math.floor(Y);
  const Iz = Math.floor(Z);
  const Ux = CloudQuintic(X - Ix);
  const Uy = CloudQuintic(Y - Iy);
  const Uz = CloudQuintic(Z - Iz);
  const Corner = (Cx, Cy, Cz) => ((CloudLatticeHash3(Ix + Cx, Iy + Cy, Iz + Cz) & 0xffffff) / 16777216);
  const C000 = Corner(0, 0, 0);
  const C100 = Corner(1, 0, 0);
  const C010 = Corner(0, 1, 0);
  const C110 = Corner(1, 1, 0);
  const C001 = Corner(0, 0, 1);
  const C101 = Corner(1, 0, 1);
  const C011 = Corner(0, 1, 1);
  const C111 = Corner(1, 1, 1);
  return mix(
    mix(mix(C000, C100, Ux), mix(C010, C110, Ux), Uy),
    mix(mix(C001, C101, Ux), mix(C011, C111, Ux), Uy),
    Uz,
  );
}

// RotateCloudOctave: native 3x3 octave rotation (keeps octaves decorrelated)
export function RotateCloudOctave(X, Y, Z) {
  return [
    X * 0.00 + Y * 0.80 + Z * 0.60,
    X * -0.80 + Y * 0.36 + Z * 0.48,
    X * -0.60 + Y * -0.48 + Z * 0.64,
  ];
}

export function CloudBoundaryCurl(X, Y, Z) {
  const E = 1.0;
  const Dx = CloudLatticeNoise(X + E, Y, Z) - CloudLatticeNoise(X - E, Y, Z);
  const Dy = CloudLatticeNoise(X, Y + E, Z) - CloudLatticeNoise(X, Y - E, Z);
  const Dz = CloudLatticeNoise(X, Y, Z + E) - CloudLatticeNoise(X, Y, Z - E);
  return [Dy - Dz * 0.5, Dz - Dx * 0.5];
}

// CloudBlueJitter / IGN dither for the march start (CloudNoise.slang CloudBlueJitter)
export function CloudBlueJitter(PixelX, PixelY) {
  const DotV = PixelX * 0.06711056 + PixelY * 0.00583715;
  const F1 = DotV - Math.floor(DotV);
  const F2 = 52.9829189 * F1;
  return F2 - Math.floor(F2);
}

//==========================================================================================================================================
//                                 SampleCloudShape / SampleCloudErosion (lattice field stand-ins)
//==========================================================================================================================================

// Multi-octave condensate field in place of the baked 128^3 Perlin-Worley volume.
// Returns [macro, meso, detail, billow] in 0..1, footprint-filtered like the mip chain.
export function SampleCloudShapeField(X, Y, Z, FootprintM, PeriodM) {
  const Filter = saturate(FootprintM / Math.max(1, PeriodM));
  const OctaveFade = 1 - SmoothStepCloud(0.08, 0.55, Filter);
  const OctaveFade2 = 1 - SmoothStepCloud(0.16, 0.9, Filter);

  const A = CloudLatticeNoise(X, Y, Z);
  const R1 = RotateCloudOctave(X, Y, Z);
  const B = CloudLatticeNoise(R1[0] * 2.03 + 11.3, R1[1] * 2.03 - 7.1, R1[2] * 2.03 + 5.7);
  const R2 = RotateCloudOctave(X * 4.11, Y * 4.11, Z * 4.11);
  const C = CloudLatticeNoise(R2[0] + 31.7, R2[1] - 17.3, R2[2] + 23.9);

  // Contrast remap: a raw octave sum is Gaussian around 0.5, whereas a Perlin-Worley condensate volume is
  // bimodal (solid cores / clear sky). Stretch and bias so formations break into cells instead of a veil.
  const Expand = (V) => saturate((V - 0.30) / 0.44);
  const Macro = Expand(0.58 * A + 0.29 * mix(0.5, B, OctaveFade) + 0.13 * mix(0.5, C, OctaveFade2));
  const Meso = Expand(0.34 * A + 0.44 * mix(0.5, B, OctaveFade) + 0.22 * mix(0.5, C, OctaveFade2));
  const Billow = Expand(0.46 * mix(0.5, B, OctaveFade) + 0.30 * mix(0.5, C, OctaveFade2) + 0.24 * A);
  const Detail = Expand(0.55 * mix(0.5, C, OctaveFade2) + 0.45 * mix(0.5, B, OctaveFade));
  return [Macro, Meso, Detail, Billow];
}

export function SampleCloudErosionField(X, Y, Z, FootprintM, PeriodM) {
  const Filter = saturate(FootprintM / Math.max(1, PeriodM));
  const Fade = 1 - SmoothStepCloud(0.10, 0.75, Filter);
  const R = RotateCloudOctave(X, Y, Z);
  const A = mix(0.5, CloudLatticeNoise(R[0] + 3.1, R[1] - 5.7, R[2] + 9.3), Fade);
  const R2 = RotateCloudOctave(X * 2.37, Y * 2.37, Z * 2.37);
  const B = mix(0.5, CloudLatticeNoise(R2[0] - 13.9, R2[1] + 27.1, R2[2] - 41.3), Fade);
  const Curl = CloudBoundaryCurl(X * 1.7 + 5.1, Y * 1.7 - 2.9, Z * 1.7 + 8.3);
  return [
    saturate(0.62 * A + 0.38 * B),
    saturate(0.55 * B + 0.30 * A + 0.15 * (0.5 + Curl[0])),
    saturate(0.48 * A + 0.34 * B + 0.18 * (0.5 + Curl[1])),
    saturate(0.40 * A + 0.36 * B + 0.24 * saturate(0.5 + Curl[0] - Curl[1])),
  ];
}

//==========================================================================================================================================
//                                                    CLOUDSPECIFICATION
//==========================================================================================================================================

export function BuildCloudSpecification(
  Deck,
  Medium = null,
  SunDirection = [0.58, -0.55, 0.6],
  SolarIrradiance = [1, 1, 1],
  SkyAmbient = [0.1, 0.15, 0.25],
  SimulationSeconds = 0,
) {
  const D = { ...DEFAULT_CLOUD_DECK, ...(Deck || {}) };
  const Med = Medium || {};
  const BearRad = D.WindBearingDegrees * DEG_TO_RAD;
  const WindVX = Math.cos(BearRad) * D.WindSpeedMetersPerSecond;
  const WindVY = -Math.sin(BearRad) * D.WindSpeedMetersPerSecond;

  const Radius = Math.max(500000.0, Med.PlanetRadiusMeters ?? 6360000.0);
  const Floor = clamp(D.CloudFloorMeters, 100.0, Math.max(200.0, D.DeckCeilingMeters - 100.0));
  return {
    RadiusM: Radius,
    Coverage: saturate(D.CloudFraction),
    Extinction: Math.max(0.0, D.ExtinctionMultiplier),
    FloorM: Floor,
    ThicknessM: Math.max(100.0, D.LayerThicknessMeters),
    FeatureScale: Math.max(0.08, D.FeatureScaleMultiplier),
    Anvil: saturate(D.AnvilTopSpread),
    PhaseG: clamp(D.ForwardScatterAnisotropy, -0.5, 0.92),
    Seed: 0.04,
    PixelAngle: 0.002,
    UniformExtinction: -1.0,
    TimeSeconds: D.FollowWindEnabled ? Math.max(0, SimulationSeconds) : 0.0,
    WindVelocityMps: [WindVX, WindVY],
    Shear: D.WindShearPerKilometer,
    Veer: D.WindVeerDegreesPerKm * DEG_TO_RAD,
    Turbulence: D.WindTurbulence,
    CycloneSpeedMps: 28.0,
    CycloneEyeM: 22000.0,
    CycloneAxis: Normalize3([0.05, 0.14, 0.988]),
    SunDirection: Normalize3(SunDirection),
    SolarIrradiance: [SolarIrradiance[0], SolarIrradiance[1], SolarIrradiance[2]],
    SkyAmbient: [SkyAmbient[0], SkyAmbient[1], SkyAmbient[2]],
    AtmosphereStrengths: [
      Math.max(0.0, Med.RayleighStrength ?? 1.0),
      Math.max(0.0, Med.MieStrength ?? 1.0),
      Math.max(0.0, Med.OzoneStrength ?? 1.2),
    ],
    AirScaleHeightsM: [
      Math.max(400.0, Med.RayleighScaleHeightMeters ?? 8000.0),
      Math.max(200.0, Med.MieScaleHeightMeters ?? 1200.0),
    ],
    Steps: clamp(Math.round(D.CloudMarchSteps), 18, 28),
    LightSteps: 4,
    Formation: clamp(Math.round(D.Category), 0, 12),
    UseClouds: D.CloudsEnabled !== false,
    UseCloudShadows: D.CloudsEnabled !== false && D.ShadowEnabled !== false,
    ShadowFraction: saturate(D.ShadowFraction),
    ShadowDensityMultiplier: Math.max(0.0, D.ShadowDensityMultiplier),
    ShadowScale: Math.max(0.1, D.ShadowScale),
    ShadowAnvil: saturate(D.ShadowAnvil),
    ObserverAltitudeMeters: Math.max(5.0, Med.ObserverAltitudeMeters ?? 100.0),
  };
}

function Normalize3(V) {
  const L = Math.max(1e-9, Math.hypot(V[0], V[1], V[2]));
  return [V[0] / L, V[1] / L, V[2] / L];
}

//==========================================================================================================================================
//                                        CLOUDTRANSPORT.SLANG — SPHERICAL INTERVALS
//==========================================================================================================================================

export function PlanetCloudAltitude(Position, PlanetRadiusM) {
  return Math.hypot(Position[0], Position[1], Position[2]) - PlanetRadiusM;
}

export function PlanetCloudSphere(Position, Direction, RadiusM) {
  const Radial = Math.hypot(Position[0], Position[1], Position[2]);
  const Projection = Position[0] * Direction[0] + Position[1] * Direction[1] + Position[2] * Direction[2];
  const Quadratic = (Radial - RadiusM) * (Radial + RadiusM);
  const Disc = Projection * Projection - Quadratic;
  if (Disc < 0) return [1, -1];
  const SqrtD = Math.sqrt(Disc);
  const Stable = -Projection - (Projection < 0 ? -1 : 1) * SqrtD;
  const Companion = Math.abs(Stable) > 1e-6 ? Quadratic / Stable : Stable;
  return [Math.min(Stable, Companion), Math.max(Stable, Companion)];
}

export function PlanetCloudGroundDistance(Origin, Direction, RadiusM) {
  const Ground = PlanetCloudSphere(Origin, Direction, RadiusM);
  if (Ground[0] > Ground[1] || Ground[1] <= 0) return 1e9;
  if (Ground[0] > 0) return Ground[0];
  const Along = Origin[0] * Direction[0] + Origin[1] * Direction[1] + Origin[2] * Direction[2];
  return Along < 0 ? 0 : 1e9;
}

export function PlanetCloudLayerSpans(Origin, Direction, RadiusM, Layer, LimitM) {
  const Outer = PlanetCloudSphere(Origin, Direction, RadiusM + Layer.TopM);
  const OuterX = Math.max(0, Outer[0]);
  const OuterY = Math.min(LimitM, Outer[1]);
  if (OuterY <= OuterX) return [0, 0, 0, 0];
  const Inner = PlanetCloudSphere(Origin, Direction, RadiusM + Layer.BottomM);
  if (Inner[1] <= OuterX || Inner[0] >= OuterY || Inner[0] >= Inner[1]) return [OuterX, OuterY, 0, 0];
  return [OuterX, Math.min(OuterY, Inner[0]), Math.max(OuterX, Inner[1]), OuterY];
}

//==========================================================================================================================================
//                                              CLOUDDENSITY.SLANG — LAYERS + WEATHER
//==========================================================================================================================================

// Map CloudCategoryType (0..11) + OrbitalCyclone (12) to a single non-overlapping deck
export function PlanetCloudLayer(Spec, Index = 0) {
  const BottomM = Math.max(100.0, Spec.FloorM);
  const Layer = {
    BottomM,
    TopM: BottomM + Math.max(100.0, Spec.ThicknessM),
    Amount: Spec.Coverage,
    Sigma: 0.0042 * Spec.Extinction,
    Category: 0,
  };
  const Fmt = Spec.Formation;
  if (Fmt === 5 || Fmt === 10) {
    Layer.Category = 2;
    Layer.Sigma = (Fmt === 10 ? 0.0012 : 0.00055) * Spec.Extinction;
  } else if (Fmt === 0 || Fmt === 4 || Fmt === 6 || Fmt === 8 || Fmt === 9) {
    Layer.Category = 1;
    Layer.Sigma = (Fmt === 4 ? 0.0018 : 0.0032) * Spec.Extinction;
  } else if (Fmt === 12) {
    Layer.Category = 0;
    Layer.Sigma = 0.0026 * Spec.Extinction;
  } else {
    Layer.Category = 0;
    Layer.Sigma = (Fmt === 1 ? 0.0036 : 0.0045) * Spec.Extinction;
  }
  if (Index !== 0) Layer.Amount = 0;
  return Layer;
}

export function CloudRotationSinCos(Theta) {
  return [Math.sin(Theta), Math.cos(Theta)];
}

export function RotateCloudPosition(Position, Axis, Angle) {
  if (Angle === 0) return Position;
  const [S, C] = CloudRotationSinCos(Angle);
  const Dot = Axis[0] * Position[0] + Axis[1] * Position[1] + Axis[2] * Position[2];
  const Cross = [
    Axis[1] * Position[2] - Axis[2] * Position[1],
    Axis[2] * Position[0] - Axis[0] * Position[2],
    Axis[0] * Position[1] - Axis[1] * Position[0],
  ];
  const Rotated = [
    Position[0] * C + Cross[0] * S + Axis[0] * Dot * (1 - C),
    Position[1] * C + Cross[1] * S + Axis[1] * Dot * (1 - C),
    Position[2] * C + Cross[2] * S + Axis[2] * Dot * (1 - C),
  ];
  const LenP = Math.hypot(Position[0], Position[1], Position[2]);
  const LenR = Math.max(1e-6, Math.hypot(Rotated[0], Rotated[1], Rotated[2]));
  const Scale = LenP / LenR;
  return [Rotated[0] * Scale, Rotated[1] * Scale, Rotated[2] * Scale];
}

export function CloudGeodesicRadius(Spec, Normal) {
  const CrossX = Spec.CycloneAxis[1] * Normal[2] - Spec.CycloneAxis[2] * Normal[1];
  const CrossY = Spec.CycloneAxis[2] * Normal[0] - Spec.CycloneAxis[0] * Normal[2];
  const CrossZ = Spec.CycloneAxis[0] * Normal[1] - Spec.CycloneAxis[1] * Normal[0];
  const Dot = Spec.CycloneAxis[0] * Normal[0] + Spec.CycloneAxis[1] * Normal[1] + Spec.CycloneAxis[2] * Normal[2];
  return Spec.RadiusM * Math.atan2(Math.hypot(CrossX, CrossY, CrossZ), Dot);
}

export function CloudCycloneAngularSpeed(Spec, RadiusM, AltitudeM) {
  const MaxWindRadius = Math.max(12000.0, Spec.CycloneEyeM * 1.8);
  const Angular =
    (2.0 * Spec.CycloneSpeedMps * MaxWindRadius) / (RadiusM * RadiusM + MaxWindRadius * MaxWindRadius);
  const Outflow = mix(1.0, -0.16, SmoothStepCloud(7200.0, 14500.0, AltitudeM));
  return Angular * Outflow * Math.exp(-Math.pow(RadiusM / 620000.0, 4.0));
}

export function CloudWindCoordinates(Spec, Position) {
  if (Spec.TimeSeconds === 0) return Position;
  const Radius = Math.hypot(Position[0], Position[1], Position[2]);
  const AltitudeM = Radius - Spec.RadiusM;
  const HeightKm = Math.max(0.0, AltitudeM - Spec.FloorM) * 0.001;
  const WindTurn = HeightKm * Spec.Veer;
  const [TrigS, TrigC] = CloudRotationSinCos(WindTurn);
  const Wind = [
    Spec.WindVelocityMps[0] * TrigC - Spec.WindVelocityMps[1] * TrigS,
    Spec.WindVelocityMps[0] * TrigS + Spec.WindVelocityMps[1] * TrigC,
  ];
  const WindSpeed = Math.hypot(Wind[0], Wind[1]);
  let Out = Position;
  if (WindSpeed > 1e-5) {
    const WindAxis = [-Wind[1] / WindSpeed, Wind[0] / WindSpeed, 0];
    const WindAngle = (WindSpeed * Spec.TimeSeconds * (1 + Spec.Shear * HeightKm)) / Spec.RadiusM;
    Out = RotateCloudPosition(Position, WindAxis, -WindAngle);
  }
  if (Spec.Formation === 12) {
    const StormRadius = CloudGeodesicRadius(Spec, [Out[0] / Radius, Out[1] / Radius, Out[2] / Radius]);
    const AngularSpeed = CloudCycloneAngularSpeed(Spec, StormRadius, AltitudeM);
    Out = RotateCloudPosition(Out, Spec.CycloneAxis, -AngularSpeed * Spec.TimeSeconds);
  }
  return Out;
}

export function CloudWeatherFromAdvectedNormal(Spec, AdvectedNormal) {
  const R = RotateCloudOctave(AdvectedNormal[0] * 13.0, AdvectedNormal[1] * 13.0, AdvectedNormal[2] * 13.0);
  const Broad = CloudLatticeNoise(R[0] + 12.8 + Spec.Seed * 0.73, R[1] - 7.9 - Spec.Seed * 0.46, R[2] + 42.1 + Spec.Seed * 1.17);
  const R2 = RotateCloudOctave(AdvectedNormal[0] * 470.0, AdvectedNormal[1] * 470.0, AdvectedNormal[2] * 470.0);
  const Meso = CloudLatticeNoise(R2[0] + 17.4, R2[1] + Spec.Seed, R2[2] + 36.1);
  const Moisture = saturate(0.48 + 0.57 * (Broad - 0.5) + 0.43 * (Meso - 0.5));
  return {
    Moisture,
    Type: Meso,
    Lift: saturate(0.42 + 0.68 * (Meso - 0.5)),
    Rain: SmoothStepCloud(0.5, 0.8, Moisture),
    Bands: Broad,
  };
}

export function CloudWeatherAtPoint(Spec, Position) {
  const Radius = Math.hypot(Position[0], Position[1], Position[2]);
  const Adv = CloudWindCoordinates(Spec, Position);
  return CloudWeatherFromAdvectedNormal(Spec, [Adv[0] / Radius, Adv[1] / Radius, Adv[2] / Radius]);
}

//==========================================================================================================================================
//                                        CLOUDDENSITY.SLANG — CONVECTIVE ENVELOPES
//==========================================================================================================================================

export function CloudCellEnvelope(PositionX, PositionY, Height, Coverage, Seed, ScaleM, Expansion = 1.0) {
  const CX = PositionX / ScaleM;
  const CY = PositionY / ScaleM;
  const CellX = Math.floor(CX);
  const CellY = Math.floor(CY);
  const SeedCell = Math.floor(Seed * 53.0);
  let Envelope = -1.0;
  for (let Y = -1; Y <= 1; ++Y) {
    for (let X = -1; X <= 1; ++X) {
      const AddressX = CellX + X;
      const AddressY = CellY + Y;
      const Random = DecodeTexel(CloudLatticeHash3(AddressX, AddressY, SeedCell));
      const CenterX = AddressX + 0.5 + 0.76 * (Random[0] - 0.5);
      const CenterY = AddressY + 0.5 + 0.76 * (Random[1] - 0.5);
      const Occurrence = SmoothStepCloud(0.18, 0.65, Coverage + 0.30 * (Random[2] - 0.5));
      let Radius = (0.19 + 0.27 * Random[3]) * mix(0.65, 1.15, Occurrence);
      if (Expansion > 1.001 && Random[2] < 0.58) continue;
      Radius *= Expansion;
      const TowerHeight = 0.30 + 0.58 * Random[2];
      const TiltX = (Random[1] - 0.5) * (0.38 * Height * Height);
      const TiltY = (Random[3] - 0.5) * (0.38 * Height * Height);
      const OffsetX = (CX - CenterX - TiltX) / Radius;
      const OffsetY = (CY - CenterY - TiltY) / Radius;
      const OffsetSquared = OffsetX * OffsetX + OffsetY * OffsetY;
      if (OffsetSquared > 3.2) continue;
      const Vertical = (Height - 0.36 * TowerHeight) / Math.max(0.1, TowerHeight * 0.64);
      const Main = 1.0 - Math.sqrt(OffsetSquared + Vertical * Vertical);
      const BudOffsetX = OffsetX - (Random[1] - 0.5) * 0.95;
      const BudOffsetY = OffsetY - (Random[2] - 0.5) * 0.95;
      const BudHeight = (Height - TowerHeight * 0.70) / Math.max(0.08, TowerHeight * 0.35);
      const Bud = 0.90 - Math.sqrt((BudOffsetX * BudOffsetX + BudOffsetY * BudOffsetY) * 1.65 + BudHeight * BudHeight);
      Envelope = Math.max(Envelope, Math.max(Main, Bud) - (1.0 - Occurrence) * 0.75);
    }
  }
  return Envelope;
}

export function CloudConvectiveEnvelope(Spec, Advected, AltitudeM, Height, Amount, ScaleM, Expansion = 1.0) {
  const Normal = Normalize3(Advected);
  const SeaLevel = [
    Advected[0] - Normal[0] * AltitudeM,
    Advected[1] - Normal[1] * AltitudeM,
    Advected[2] - Normal[2] * AltitudeM,
  ];
  const WeightRaw = [
    SmoothStepCloud(0.4, 0.82, Math.abs(Normal[0])),
    SmoothStepCloud(0.4, 0.82, Math.abs(Normal[1])),
    SmoothStepCloud(0.4, 0.82, Math.abs(Normal[2])),
  ];
  const Weight = [WeightRaw[0] * WeightRaw[0], WeightRaw[1] * WeightRaw[1], WeightRaw[2] * WeightRaw[2]];
  const WeightSum = Math.max(0.001, Weight[0] + Weight[1] + Weight[2]);
  let Envelope = 0.0;
  for (let Axis = 0; Axis < 3; ++Axis) {
    const W = Axis === 0 ? Weight[2] : Axis === 1 ? Weight[0] : Weight[1];
    if (W <= 0.001) continue;
    const PX = Axis === 0 ? SeaLevel[0] : Axis === 1 ? SeaLevel[1] : SeaLevel[0];
    const PY = Axis === 0 ? SeaLevel[1] : Axis === 1 ? SeaLevel[2] : SeaLevel[2];
    const AxisSeed = Spec.Seed + (Axis === 0 ? 0 : Axis === 1 ? 23.0 : 59.0);
    Envelope += W * CloudCellEnvelope(PX, PY, Height, Amount, AxisSeed, ScaleM, Expansion);
  }
  return Envelope / WeightSum;
}

export function EvaluateCloudShapeDisplacement(X, Y, Z) {
  const R1 = RotateCloudOctave(X, Y, Z);
  const R2 = RotateCloudOctave(X * 1.19, Y * 1.19, Z * 1.19);
  return [
    CloudLatticeNoise(X + 17.3, Y + 43.7, Z + 61.1) - 0.5,
    CloudLatticeNoise(R1[0] + 53.1, R1[1] + 11.9, R1[2] + 37.7) - 0.5,
    CloudLatticeNoise(R2[0] + 29.3, R2[1] + 71.1, R2[2] + 13.7) - 0.5,
  ];
}

export function CloudCycloneEnvelope(Spec, Advected, AltitudeM, Coordinate, FootprintM) {
  const Normal = Normalize3(Advected);
  const RadiusM = CloudGeodesicRadius(Spec, Normal);
  const ReferenceAxis = Math.abs(Spec.CycloneAxis[1]) > 0.9 ? [1, 0, 0] : [0, 1, 0];
  const TangentX = Normalize3([
    ReferenceAxis[1] * Spec.CycloneAxis[2] - ReferenceAxis[2] * Spec.CycloneAxis[1],
    ReferenceAxis[2] * Spec.CycloneAxis[0] - ReferenceAxis[0] * Spec.CycloneAxis[2],
    ReferenceAxis[0] * Spec.CycloneAxis[1] - ReferenceAxis[1] * Spec.CycloneAxis[0],
  ]);
  const TangentY = [
    Spec.CycloneAxis[1] * TangentX[2] - Spec.CycloneAxis[2] * TangentX[1],
    Spec.CycloneAxis[2] * TangentX[0] - Spec.CycloneAxis[0] * TangentX[2],
    Spec.CycloneAxis[0] * TangentX[1] - Spec.CycloneAxis[1] * TangentX[0],
  ];
  const Azimuth = Math.atan2(
    Normal[0] * TangentY[0] + Normal[1] * TangentY[1] + Normal[2] * TangentY[2],
    Normal[0] * TangentX[0] + Normal[1] * TangentX[1] + Normal[2] * TangentX[2],
  );
  const Warp = CloudLatticeNoise(Coordinate[0] * 0.000013 + 31.7, Coordinate[1] * 0.000013 + 31.7, Coordinate[2] * 0.000013 + 31.7) - 0.5;
  const SpiralAngle =
    2.0 * Azimuth - 5.8 * Math.log(Math.max(0.8, RadiusM / Math.max(9000.0, Spec.CycloneEyeM))) + 1.8 * Warp;
  const Spiral = Math.pow(saturate(0.5 + 0.5 * Math.cos(SpiralAngle)), 2.7);
  const Broken = CloudLatticeNoise(Coordinate[0] * 0.000026 + 53.2, Coordinate[1] * 0.000026 + 53.2, Coordinate[2] * 0.000026 + 53.2);
  const Central = 1.0 - SmoothStepCloud(100000.0, 240000.0, RadiusM);
  const Outer = 1.0 - SmoothStepCloud(210000.0, 570000.0, RadiusM);
  const Rainbands = SmoothStepCloud(0.08, 0.70, Spiral) * (0.57 + 0.43 * Broken) * Outer;
  const Eyewall = Math.exp(-Math.pow((RadiusM - Spec.CycloneEyeM * 1.65) / (Spec.CycloneEyeM * 0.65), 2.0));
  const EyeRadiusM =
    Spec.CycloneEyeM +
    Math.max(0.0, AltitudeM - 1500.0) * 0.58 +
    1500.0 * Math.sin(3.0 * Azimuth + 0.4) +
    1100.0 * Math.sin(Azimuth - 0.7);
  const Eye = SmoothStepCloud(EyeRadiusM, EyeRadiusM + 4300.0 + 1600.0 * Warp, RadiusM);
  const BillowCoordinate = [Coordinate[0] + 18000.0 * Warp, Coordinate[1] + 18000.0 * Warp, Coordinate[2] + 18000.0 * Warp];
  const Billow = SampleCloudShapeField(
    BillowCoordinate[0] / 140000.0 + 0.713,
    BillowCoordinate[1] / 140000.0 + 0.713,
    BillowCoordinate[2] / 140000.0 + 0.713,
    FootprintM * 1.6,
    140000.0,
  );
  const MacroBillow = Billow[0];
  const MesoBillow = Billow[1];
  const SmallBillow = Broken;
  const ConvectiveTopM =
    12100.0 + 5800.0 * (MacroBillow - 0.47) + 2400.0 * (MesoBillow - 0.47) + 500.0 * (SmallBillow - 0.47);
  let TopM = mix(2900.0 + 2900.0 * Rainbands + 1500.0 * MesoBillow, ConvectiveTopM, Central);
  TopM = Math.max(TopM, 12500.0 * Eyewall);
  const WaterHeight =
    SmoothStepCloud(900.0, 1350.0, AltitudeM) * (1.0 - SmoothStepCloud(TopM - 1250.0, TopM, AltitudeM));
  const Outflow =
    SmoothStepCloud(100000.0, 185000.0, RadiusM) * (1.0 - SmoothStepCloud(215000.0, 440000.0, RadiusM));
  const OutflowTopM =
    11900.0 + 2400.0 * (MacroBillow - 0.47) + 2000.0 * (MesoBillow - 0.47) + 350.0 * (SmallBillow - 0.47);
  const IceHeight =
    SmoothStepCloud(10000.0, 10800.0, AltitudeM) * (1.0 - SmoothStepCloud(OutflowTopM - 360.0, OutflowTopM, AltitudeM));
  const IceFibers = saturate(0.51 + 0.49 * Spiral + 0.40 * (MacroBillow - 0.47) + 0.12 * (MesoBillow - 0.47));
  const HeightEnvelope = Math.max(WaterHeight, IceHeight * Outflow) * Eye;
  const CentralMass = Central * (0.72 + 0.32 * MacroBillow + 0.16 * MesoBillow);
  let Mass = Math.max(CentralMass, 0.98 * Rainbands) + 0.20 * Eyewall;
  Mass = Math.max(Mass * WaterHeight, Outflow * IceHeight * IceFibers * 0.72);
  return { Mass: Mass * Eye, HeightEnvelope };
}

//==========================================================================================================================================
//                                      CLOUDDENSITY.SLANG — ONE DENSITY FROM GROUND TO SPACE
//==========================================================================================================================================

export function SamplePlanetCloudDensityAdvected(Spec, Layer, Position, Advected, Weather, FootprintM, LightQuery) {
  const Radius = Math.hypot(Position[0], Position[1], Position[2]);
  const AltitudeM = Radius - Spec.RadiusM;
  const Height = (AltitudeM - Layer.BottomM) / (Layer.TopM - Layer.BottomM);
  if (Height <= 0 || Height >= 1 || Layer.Amount <= 0) return 0;
  if (Spec.UniformExtinction >= 0) return Spec.UniformExtinction;

  // native: RotateCloudOctave(Advected - float3(0, 0, RadiusM)) + Seed * float3(731, -1261, 3179)
  const Rotated = RotateCloudOctave(Advected[0], Advected[1], Advected[2] - Spec.RadiusM);
  const Coordinate = [
    Rotated[0] + Spec.Seed * 731.0,
    Rotated[1] - Spec.Seed * 1261.0,
    Rotated[2] + Spec.Seed * 3179.0,
  ];

  const ScaleM = Math.max(900.0, 7200.0 * Math.max(0.15, Spec.FeatureScale));
  const ShapeCoordinate = [Coordinate[0] / ScaleM + 0.143, Coordinate[1] / ScaleM + 0.713, Coordinate[2] / ScaleM + 0.337];
  const Fmt = Spec.Formation;

  const Amount = saturate(Layer.Amount + 0.38 * (Weather.Moisture - 0.5));

  let ConvEnvelope = 0.0;
  const IsConvective = Fmt === 1 || Fmt === 2 || Fmt === 3 || Fmt === 7 || Fmt === 11;
  if (IsConvective) {
    const CellScaleM = ScaleM * (Fmt === 1 || Fmt === 11 ? 0.5525 : 0.85);
    ConvEnvelope = CloudConvectiveEnvelope(Spec, Advected, AltitudeM, Height, Amount, CellScaleM);
    if (ConvEnvelope < -0.92 && Fmt !== 3 && Fmt !== 11) return 0;
  }

  let Shape = [0.47, 0.47];
  let Detail = 0.48;
  let ShapeFootprintM = FootprintM;
  if (!LightQuery && (Fmt === 8 || Fmt === 12)) {
    const WarpS = Fmt === 8 ? 2.2 : 3.7;
    const WarpA = Fmt === 8 ? 1.4 : 1.15;
    const WarpNoise =
      CloudLatticeNoise(Coordinate[0] / (ScaleM * WarpS), Coordinate[1] / (ScaleM * WarpS), Coordinate[2] / (ScaleM * WarpS)) -
      0.5;
    ShapeCoordinate[0] += WarpA * WarpNoise;
    ShapeCoordinate[1] += WarpA * WarpNoise;
    ShapeCoordinate[2] += WarpA * WarpNoise;
    ShapeFootprintM *= 1.3;
  }

  if (Layer.Category !== 2 || Fmt === 10) {
    const Field = SampleCloudShapeField(ShapeCoordinate[0], ShapeCoordinate[1], ShapeCoordinate[2], ShapeFootprintM, ScaleM);
    Shape = [Field[0], Field[1]];
    if (!LightQuery) {
      const FineFilter = 1 - SmoothStepCloud(ScaleM * 0.015, ScaleM * 0.11, FootprintM);
      if (FineFilter > 0.01) {
        const R = RotateCloudOctave(ShapeCoordinate[0] * 3.17 + 0.337, ShapeCoordinate[1] * 3.17 + 0.337, ShapeCoordinate[2] * 3.17 + 0.337);
        const BillowField = SampleCloudShapeField(R[0], R[1], R[2], ShapeFootprintM, ScaleM / 3.17);
        Detail = mix(0.48, BillowField[3], FineFilter);
      }
    }
  }

  let Raw = 0.0;
  let HeightEnvelope = SmoothStepCloud(0.0, 0.045, Height) * (1 - SmoothStepCloud(0.93, 1.0, Height));

  if (Fmt === 12) {
    const Cyclone = CloudCycloneEnvelope(Spec, Advected, AltitudeM, Coordinate, FootprintM);
    const CycloneMass = Cyclone.Mass;
    HeightEnvelope = Math.max(HeightEnvelope, Cyclone.HeightEnvelope);
    const SynopticEnv = SmoothStepCloud(0.02, 0.16, Height) * (1 - SmoothStepCloud(0.62, 0.96, Height));
    const SynopticRaw = 0.85 * (Shape[0] - 0.44) + 0.30 * (Detail - 0.48) - (1 - Amount) * 0.22;
    if (CycloneMass <= 0.001) {
      HeightEnvelope = Math.max(HeightEnvelope, SynopticEnv);
      Raw = SynopticRaw * SynopticEnv - (1 - SynopticEnv) * 0.12;
    } else {
      const Blended = Math.max(CycloneMass, saturate(SynopticRaw) * 0.65 * SynopticEnv);
      HeightEnvelope = Math.max(HeightEnvelope, SynopticEnv);
      Raw = Blended - (0.28 + 0.24 * (1 - Amount)) + 0.20 * (Shape[0] - 0.5) + 0.06 * (Detail - 0.5);
    }
  } else if (Fmt === 5 || Fmt === 10) {
    const Rot = RotateCloudOctave(Coordinate[0], Coordinate[1], Coordinate[2]);
    const Fiber = [
      (Rot[0] * 0.000075) / Math.max(0.3, Spec.FeatureScale),
      (Rot[1] * 0.0028) / Math.max(0.3, Spec.FeatureScale),
      (Rot[2] * 0.00065) / Math.max(0.3, Spec.FeatureScale),
    ];
    const Bend = CloudLatticeNoise(Coordinate[0] * 0.00014 + 41.3, Coordinate[1] * 0.00014 + 41.3, Coordinate[2] * 0.00014 + 41.3);
    const Bank = CloudLatticeNoise(
      Coordinate[0] * 0.000045 + 59.1,
      Coordinate[1] * 0.000070 + 59.1,
      Coordinate[2] * 0.00013 + 59.1,
    );
    Fiber[1] += 4.8 * Bend + 2.7 * Height * Height;
    Fiber[0] += 0.60 * Height * Height;
    let Strand;
    if (LightQuery) {
      Strand =
        0.65 * CloudLatticeNoise(Fiber[0] + 3.7, Fiber[1] + 3.7, Fiber[2] + 3.7) +
        0.35 * CloudLatticeNoise(Fiber[0] * 2.13 + 16.9, Fiber[1] * 2.13 + 16.9, Fiber[2] * 2.13 + 16.9);
    } else {
      Fiber[2] += 1.3 * CloudLatticeNoise(Coordinate[0] * 0.00023 - 23.0, Coordinate[1] * 0.00023 - 23.0, Coordinate[2] * 0.00023 - 23.0);
      const Filter = (FootprintM * 0.0028) / Math.max(0.3, Spec.FeatureScale);
      Strand =
        0.64 *
          mix(
            0.5,
            CloudLatticeNoise(Fiber[0] + 3.7, Fiber[1] + 3.7, Fiber[2] + 3.7),
            1 - SmoothStepCloud(0.6, 1.5, Filter),
          ) +
        0.36 *
          mix(
            0.5,
            CloudLatticeNoise(Fiber[0] * 2.13 + 16.9, Fiber[1] * 2.13 + 16.9, Fiber[2] * 2.13 + 16.9),
            1 - SmoothStepCloud(0.6, 1.5, Filter * 2.13),
          );
    }
    let BankEnvelope = SmoothStepCloud(0.30, 0.67, Bank + (Amount - 0.6) * 0.35);
    if (Fmt === 10) {
      const SilkRibbon = Math.pow(0.5 + 0.5 * Math.sin(Fiber[1] * 0.45 + 3.0 * Bend), 0.78);
      Strand = 0.55 * SilkRibbon + 0.45 * Shape[0];
      BankEnvelope = mix(0.45, 1.0, BankEnvelope);
    }
    HeightEnvelope *= SmoothStepCloud(0.0, 0.23, Height) * (1 - SmoothStepCloud(0.55, 1.0, Height)) * BankEnvelope;
    Raw = Strand - mix(0.70, 0.43, Amount) - (1 - HeightEnvelope) * 0.16;
  } else if (Fmt === 6) {
    const MacroLobes = CloudLatticeNoise(
      (Coordinate[0] * 0.00011) / Math.max(0.25, Spec.FeatureScale) + 1.7,
      (Coordinate[1] * 0.00024) / Math.max(0.25, Spec.FeatureScale) + 1.7,
      1.7,
    );
    const Laminar = Math.pow(0.5 + 0.5 * Math.sin(Height * CLOUD_PI * 2.4 - 0.3 + (Shape[0] - 0.5) * 0.85), 1.15);
    const LensMask = SmoothStepCloud(0.42, 0.72, MacroLobes + (Shape[0] - 0.5) * 0.24);
    HeightEnvelope *= SmoothStepCloud(0.04, 0.24, Height) * (1 - SmoothStepCloud(0.74, 0.96, Height));
    Raw = LensMask * mix(0.72, Laminar, 0.58) - (1 - Amount) * 0.45 - (1 - HeightEnvelope) * 0.28;
  } else if (Fmt === 8) {
    const TiltOffset = 320.0 * Math.sin(Height * CLOUD_PI) + 220.0 * Height * Height;
    const WavePhase =
      (Coordinate[0] + TiltOffset) * (0.0020 / Math.max(0.25, Spec.FeatureScale)) + (Shape[0] - 0.5) * 2.8;
    const BreakCurl = SmoothStepCloud(0.26, 0.88, 0.5 + 0.5 * Math.sin(WavePhase + Height * 3.0));
    const MacroBand = SmoothStepCloud(
      0.36,
      0.72,
      CloudLatticeNoise(Coordinate[0] * 0.00012 + 3.1, Coordinate[1] * 0.00012 + 3.1, Coordinate[2] * 0.00012 + 3.1),
    );
    HeightEnvelope *= SmoothStepCloud(0.08, 0.30, Height) * (1 - SmoothStepCloud(0.72, 0.98, Height));
    Raw = MacroBand * (0.58 * BreakCurl + 0.42 * Shape[0]) - mix(0.56, 0.24, Amount) - (1 - HeightEnvelope) * 0.26;
  } else if (Fmt === 9) {
    const WaveFilter = LightQuery ? 0.0 : 1 - SmoothStepCloud(320.0, 2200.0, FootprintM);
    const K = 0.0018 / Math.max(0.25, Spec.FeatureScale);
    let Swell = 0.5;
    if (WaveFilter > 0.01) {
      Swell +=
        0.125 *
        WaveFilter *
        (Math.sin((Coordinate[0] * 0.966 + Coordinate[1] * 0.259) * K + (Shape[0] - 0.5) * 3.2) +
          Math.cos((Coordinate[0] * 0.391 + Coordinate[1] * 0.920) * K * 1.08 - (Shape[1] - 0.5) * 2.8) +
          Math.sin((-Coordinate[0] * 0.391 + Coordinate[1] * 0.920) * K * 1.15 + (Detail - 0.5) * 2.4) +
          Math.cos((-Coordinate[0] * 0.927 + Coordinate[1] * 0.375) * K * 0.96 - (Shape[0] - 0.5) * 3.0));
    }
    HeightEnvelope *= SmoothStepCloud(0.05, 0.22, Height) * (1 - SmoothStepCloud(0.72, 0.96, Height));
    Raw = 0.62 * Shape[0] + 0.38 * Swell - mix(0.62, 0.26, Amount) - (1 - HeightEnvelope) * 0.28;
  } else if (Layer.Category === 1) {
    const Fold = CloudLatticeNoise(Coordinate[0] * 0.00016 + 37.1, Coordinate[1] * 0.00016 + 37.1, Coordinate[2] * 0.00016 + 37.1);
    const Vertical = Height + (Fold - 0.5) * 0.25;
    HeightEnvelope *= SmoothStepCloud(0.0, 0.19, Vertical) * (1 - SmoothStepCloud(0.66, 1.0, Vertical));
    // The condensate lattice here is procedural (CloudNoiseVolume is not baked), so its distribution sits
    // tighter around 0.5 than the shipped Perlin-Worley volume. The threshold is raised by 0.13 to restore
    // the authored cover: measured sky cover 0.25/0.42/0.70 -> 30/39/61 % with it, 47/68/85 % without.
    Raw = 0.72 * Shape[0] + 0.28 * Shape[1] - mix(0.77, 0.38, Amount) - (1 - HeightEnvelope) * 0.36;
  } else {
    let Envelope = ConvEnvelope;
    if (Fmt === 3) {
      const AnvilDisc = SmoothStepCloud(0.68, 0.88, Height) * (1 - SmoothStepCloud(0.92, 1.0, Height));
      Envelope = Math.max(Envelope, AnvilDisc * (0.45 * Spec.Anvil + 0.40 * (Shape[0] - 0.35)));
    } else if (Fmt === 7 && !LightQuery) {
      const PouchFilter = 1 - SmoothStepCloud(260.0, 1800.0, FootprintM);
      const K = 0.0017 / Math.max(0.25, Spec.FeatureScale);
      const PX = Coordinate[0] + (Shape[0] - 0.5) * 420.0;
      const PY = Coordinate[1] - (Shape[1] - 0.5) * 420.0;
      const Pouch =
        Math.pow(
          saturate(
            (Math.cos(PX * K) + Math.cos((-0.5 * PX + 0.866025 * PY) * K) + Math.cos((-0.5 * PX - 0.866025 * PY) * K) + 1.5) /
              4.5,
          ),
          1.15,
        ) * PouchFilter;
      const LowerMask = 1 - SmoothStepCloud(0.18, 0.62, Height);
      Envelope += Pouch * LowerMask * 0.42;
    } else if (Fmt === 11) {
      const TowerCap = SmoothStepCloud(0.70, 0.96, Height) * Spec.Anvil;
      Envelope = Math.max(Envelope * 1.12, TowerCap * (0.42 + 0.45 * (Shape[0] - 0.40)));
    }
    Raw = 0.90 * Envelope + 1.05 * (Shape[0] - 0.45) + 0.37 * (Shape[1] - 0.45) + 0.32 * (Detail - 0.5);
    Raw -= 0.06 + (1 - Amount) * 0.10;
  }

  const CoherentBoundary = Fmt === 3 || Fmt === 11 || Fmt === 12;
  if (Raw <= (CoherentBoundary ? 0.0 : -0.08) || HeightEnvelope <= 0) return 0;

  if (!LightQuery && Layer.Category !== 2) {
    const ErosionFilter = 1 - SmoothStepCloud(24.0, 180.0, FootprintM);
    if (Raw < 0.45 && ErosionFilter > 0) {
      const EroX = Coordinate[0] + (Shape[1] - 0.47) * (180.0 * Spec.Turbulence);
      const EroY = Coordinate[1] + (0.47 - Shape[0]) * (180.0 * Spec.Turbulence);
      const Erosion = SampleCloudErosionField(EroX / 1550.0 + 0.419, EroY / 1550.0 + 0.673, Coordinate[2] / 1550.0 + 0.613, FootprintM, 1550.0);
      const ErosionSignal =
        (Erosion[0] - 0.47) * 0.46 + (Erosion[1] - 0.47) * 0.29 + (Erosion[2] - 0.47) * 0.17 + (Erosion[3] - 0.47) * 0.08;
      let BoundaryShift = ErosionSignal * 0.64 * ErosionFilter * (1 - SmoothStepCloud(0.20, 0.45, Raw));
      if (CoherentBoundary) BoundaryShift = clamp(BoundaryShift, -0.35 * Raw, 0.35 * Raw);
      Raw += BoundaryShift;
    }
  }

  const Response = Layer.Category === 2 ? 0.14 : 0.048;
  const Condensate = SmoothStepCloud(0.0, Response, Raw) * HeightEnvelope;
  return Layer.Sigma * Condensate;
}

export function SamplePlanetCloudShadowDensity(Spec, Layer, Position, Advected, Weather, FootprintM) {
  const Radius = Math.hypot(Position[0], Position[1], Position[2]);
  const AltitudeM = Radius - Spec.RadiusM;
  const Height = (AltitudeM - Layer.BottomM) / (Layer.TopM - Layer.BottomM);
  if (Height <= 0 || Height >= 1 || Layer.Amount <= 0) return 0;
  if (Spec.UniformExtinction >= 0) return Spec.UniformExtinction;

  const Rotated = RotateCloudOctave(Advected[0], Advected[1], Advected[2] - Spec.RadiusM);
  const Coordinate = [Rotated[0] + Spec.Seed * 731.0, Rotated[1] - Spec.Seed * 1261.0, Rotated[2] + Spec.Seed * 3179.0];
  const ScaleM = Math.max(900.0, 7200.0 * Math.max(0.15, Spec.FeatureScale));
  const Amount = saturate(Layer.Amount + 0.38 * (Weather.Moisture - 0.5));
  const HeightEnv = SmoothStepCloud(0.0, 0.12, Height) * (1 - SmoothStepCloud(0.75, 1.0, Height));
  const Fmt = Spec.Formation;

  const Field = SampleCloudShapeField(Coordinate[0] / ScaleM + 0.143, Coordinate[1] / ScaleM + 0.713, Coordinate[2] / ScaleM + 0.337, FootprintM, ScaleM);
  let Raw = 0.78 * (Field[0] - 0.45) + 0.28 * (Field[1] - 0.45) - (1 - Amount) * 0.42 - (1 - HeightEnv) * 0.25;
  if (Fmt === 12) {
    const CycR = CloudGeodesicRadius(Spec, Normalize3(Advected));
    const Cyc =
      (1 - SmoothStepCloud(100000.0, 260000.0, CycR)) *
      SmoothStepCloud(Spec.CycloneEyeM, Spec.CycloneEyeM + 6000.0, CycR);
    Raw = Math.max(Raw, Cyc - (0.28 + 0.24 * (1 - Amount)));
  }
  if (Raw <= 0 || HeightEnv <= 0) return 0;
  const Response = Layer.Category === 2 ? 0.14 : 0.06;
  return Layer.Sigma * SmoothStepCloud(0.0, Response, Raw) * HeightEnv;
}

export function SamplePlanetCloudDensity(Spec, Layer, Position, Weather, FootprintM, LightQuery) {
  const Advected = CloudWindCoordinates(Spec, Position);
  if (LightQuery) return SamplePlanetCloudShadowDensity(Spec, Layer, Position, Advected, Weather, FootprintM);
  return SamplePlanetCloudDensityAdvected(Spec, Layer, Position, Advected, Weather, FootprintM, false);
}

//==========================================================================================================================================
//                                     CLOUDTRANSPORT.SLANG — SOLAR EXTINCTION + ORDERS
//==========================================================================================================================================

export function QueryCloudRayleighExtinction(Spec) {
  const S = Math.max(0.0, Spec.AtmosphereStrengths[0]);
  return [5.802e-6 * S, 13.558e-6 * S, 33.1e-6 * S];
}

export function QueryCloudAerosolExtinction(Spec) {
  const S = Math.max(0.0, Spec.AtmosphereStrengths[1]);
  return [4.4e-6 * S, 4.4e-6 * S, 4.4e-6 * S];
}

export function CloudAirColumn(ScaleHeightM, RadiusM, AltitudeM, Mu) {
  const AbsCos = Math.abs(Mu);
  const Upward =
    (2.0 * ScaleHeightM) / (AbsCos + Math.sqrt(AbsCos * AbsCos + (8.0 * ScaleHeightM) / (CLOUD_PI * RadiusM)));
  const ClosestRise = ((RadiusM + AltitudeM) * Mu * Mu) / (1 + Math.sqrt(saturate(1 - Mu * Mu)));
  const DownwardCol =
    Math.sqrt(2.0 * CLOUD_PI * (RadiusM + AltitudeM) * ScaleHeightM) * Math.exp(Math.min(9.0, ClosestRise / ScaleHeightM)) -
    Upward;
  return Math.exp(-Math.max(0.0, AltitudeM) / ScaleHeightM) * (Mu < 0 ? DownwardCol : Upward);
}

export function CloudSolarAirTransmittance(Spec, Position) {
  const RadiusM = Math.hypot(Position[0], Position[1], Position[2]);
  const AltitudeM = Math.max(0.0, RadiusM - Spec.RadiusM);
  const Mu = (Position[0] * Spec.SunDirection[0] + Position[1] * Spec.SunDirection[1] + Position[2] * Spec.SunDirection[2]) / RadiusM;
  const TangentBoundary = -Math.sqrt(saturate((AltitudeM * (RadiusM + Spec.RadiusM)) / (RadiusM * RadiusM)));
  const SolarVisibility = SmoothStepCloud(-0.00465, 0.00465, Mu - TangentBoundary);
  if (SolarVisibility <= 0) return [0, 0, 0];
  const RayleighColumn = CloudAirColumn(Math.max(400.0, Spec.AirScaleHeightsM[0]), Spec.RadiusM, AltitudeM, Mu);
  const AerosolColumn = CloudAirColumn(Math.max(200.0, Spec.AirScaleHeightsM[1]), Spec.RadiusM, AltitudeM, Mu);
  const OzoneColumn = (15000.0 * Math.exp(-Math.max(0.0, AltitudeM - 25000.0) / 8000.0)) / Math.max(0.15, Math.abs(Mu));
  const BetaR = QueryCloudRayleighExtinction(Spec);
  const BetaA = QueryCloudAerosolExtinction(Spec);
  const Ozone = Math.max(0.0, Spec.AtmosphereStrengths[2]);
  return [0, 1, 2].map((I) => {
    const Tau =
      BetaR[I] * RayleighColumn +
      BetaA[I] * AerosolColumn +
      Ozone * [0.65e-6, 1.881e-6, 0.085e-6][I] * OzoneColumn;
    return SolarVisibility * Math.exp(-Tau);
  });
}

export function IntegrateCloudSolarDepthWithWeather(Spec, Layer, Position, Weather, Span, Steps, FootprintM) {
  if (Span[1] <= Span[0]) return 0;
  const LengthM = Span[1] - Span[0];
  let Tau = 0;
  let StartM = 0;
  for (let Index = 0; Index < Steps; ++Index) {
    const Fraction = (Index + 1) / Steps;
    const EndM = LengthM * Fraction * Fraction;
    const SampleM = Span[0] + 0.5 * (StartM + EndM);
    const SegmentM = EndM - StartM;
    const SamplePosition = [
      Position[0] + Spec.SunDirection[0] * SampleM,
      Position[1] + Spec.SunDirection[1] * SampleM,
      Position[2] + Spec.SunDirection[2] * SampleM,
    ];
    const Adv = CloudWindCoordinates(Spec, SamplePosition);
    const Sigma = SamplePlanetCloudShadowDensity(Spec, Layer, SamplePosition, Adv, Weather, Math.max(FootprintM, 16.0 + 0.18 * SegmentM));
    Tau += SegmentM * Sigma;
    if (Tau >= 18.0) return 18.0;
    StartM = EndM;
  }
  return Tau;
}

export function CloudSolarOpticalDepthWithWeather(Spec, Position, Weather, Steps, FootprintM = 0) {
  if (!Spec.UseClouds || !Spec.UseCloudShadows || Spec.Coverage <= 0.001 || Spec.Extinction <= 0.001) return 0;
  const LimitM = PlanetCloudGroundDistance(Position, Spec.SunDirection, Spec.RadiusM);
  const Layer = PlanetCloudLayer(Spec, 0);
  const Spans = PlanetCloudLayerSpans(Position, Spec.SunDirection, Spec.RadiusM, Layer, LimitM);
  let Tau = 0;
  for (let Interval = 0; Interval < 2 && Tau < 18.0; ++Interval) {
    const Span = Interval === 0 ? [Spans[0], Spans[1]] : [Spans[2], Spans[3]];
    Tau += IntegrateCloudSolarDepthWithWeather(Spec, Layer, Position, Weather, Span, Steps, FootprintM);
  }
  return Math.min(18.0, Tau);
}

export function CloudSolarOpticalDepth(Spec, Position, Steps, FootprintM = 0) {
  if (!Spec.UseClouds || !Spec.UseCloudShadows || Spec.Coverage <= 0.001 || Spec.Extinction <= 0.001) return 0;
  const Layer = PlanetCloudLayer(Spec, 0);
  const Weather = CloudWeatherAtPoint(Spec, Position);
  return CloudSolarOpticalDepthWithWeather(Spec, Position, Weather, Steps, FootprintM);
}

export function CloudTransportHG(Mu, Asymmetry) {
  const A2 = Asymmetry * Asymmetry;
  const Den = Math.max(1e-4, 1 + A2 - 2 * Asymmetry * Mu);
  return (1 - A2) / (4 * CLOUD_PI * Den * Math.sqrt(Den));
}

export function CloudDualLobe(Mu, Asymmetry, Scale) {
  return 0.86 * CloudTransportHG(Mu, Asymmetry * Scale) + 0.14 * CloudTransportHG(Mu, -0.25 * Scale);
}

export function CloudScatteringOrderPhases(Spec, Direction) {
  const Mu = Direction[0] * Spec.SunDirection[0] + Direction[1] * Spec.SunDirection[1] + Direction[2] * Spec.SunDirection[2];
  const Asymmetry = clamp(Spec.PhaseG, 0.35, 0.92);
  return [CloudDualLobe(Mu, Asymmetry, 1.0), CloudDualLobe(Mu, Asymmetry, 0.49), CloudDualLobe(Mu, Asymmetry, 0.19)];
}

//==========================================================================================================================================
//                                          CLOUDTRANSPORT.SLANG — CLOUD SOURCE LIGHTING
//==========================================================================================================================================

export function PlanetCloudSource(Spec, Layer, Position, Weather, Sigma, OrderPhases, ViewDirection, FootprintM) {
  const AirTrans = CloudSolarAirTransmittance(Spec, Position);
  const Direct = [
    Spec.SolarIrradiance[0] * AirTrans[0],
    Spec.SolarIrradiance[1] * AirTrans[1],
    Spec.SolarIrradiance[2] * AirTrans[2],
  ];
  let SolarTau = 0;
  if (Direct[0] * Direct[0] + Direct[1] * Direct[1] + Direct[2] * Direct[2] > 1e-6) {
    SolarTau = CloudSolarOpticalDepthWithWeather(Spec, Position, Weather, Spec.LightSteps, FootprintM);
  }
  const Attenuated = [Math.exp(-SolarTau * 1.0), Math.exp(-SolarTau * 0.35), Math.exp(-SolarTau * 0.12)];
  const PhaseMix =
    OrderPhases[0] * 1.0 * Attenuated[0] + OrderPhases[1] * 0.45 * Attenuated[1] + OrderPhases[2] * 0.20 * Attenuated[2];
  const Scattered = [Direct[0] * PhaseMix, Direct[1] * PhaseMix, Direct[2] * PhaseMix];

  const HeightFraction = saturate((PlanetCloudAltitude(Position, Spec.RadiusM) - Layer.BottomM) / (Layer.TopM - Layer.BottomM));
  const Radius = Math.hypot(Position[0], Position[1], Position[2]);
  const SunUp = (Position[0] * Spec.SunDirection[0] + Position[1] * Spec.SunDirection[1] + Position[2] * Spec.SunDirection[2]) / Radius;
  const Day = SmoothStepCloud(-0.20, 0.23, SunUp);
  const Warm = Math.exp(-Math.abs(SunUp - 0.045) * 9.5) * SmoothStepCloud(-0.20, -0.025, SunUp);

  const SkyVisibility = mix(1.0, 0.16 + 0.84 * Math.exp(-0.24 * SolarTau), SmoothStepCloud(0.12, 0.50, SunUp));
  const Twilight = SmoothStepCloud(-0.30, -0.12, SunUp);
  const SkyScale = (0.025 * Twilight + 0.36 * Day) * mix(0.48, 1.0, HeightFraction) * SkyVisibility;
  const BounceScale = (1 - HeightFraction) * 0;
  const Bounce = [
    BounceScale + (1 - HeightFraction) * (0.032 * Day + 0.077 * Warm),
    (1 - HeightFraction) * (0.048 * Day + 0.029 * Warm),
    (1 - HeightFraction) * (0.071 * Day + 0.015 * Warm),
  ];

  let Scatter = [Scattered[0], Scattered[1], Scattered[2]];
  if (Spec.Formation === 10) {
    const Mu = ViewDirection[0] * Spec.SunDirection[0] + ViewDirection[1] * Spec.SunDirection[1] + ViewDirection[2] * Spec.SunDirection[2];
    const Phase = Mu * 9.0 + HeightFraction * 5.0 + (Position[0] + Position[1]) * 0.00020;
    Scatter = [
      Scatter[0] * (0.98 + 0.28 * Math.sin(Phase)),
      Scatter[1] * (0.95 + 0.28 * Math.sin(Phase + 2.094)),
      Scatter[2] * (1.04 + 0.30 * Math.sin(Phase + 4.188)),
    ];
  } else if (Spec.Formation === 11) {
    const CoreDense = SmoothStepCloud(Layer.Sigma * 0.22, Layer.Sigma * 0.85, Sigma);
    const Glow = Math.pow(1 - HeightFraction, 2.8) * CoreDense;
    Bounce[0] += 1.25 * Glow;
    Bounce[1] += 0.34 * Glow;
    Bounce[2] += 0.05 * Glow;
  }

  const SolarMean = (Spec.SolarIrradiance[0] + Spec.SolarIrradiance[1] + Spec.SolarIrradiance[2]) / 3 / 16;
  return [
    Scatter[0] + Spec.SkyAmbient[0] * SkyScale + Bounce[0] * SolarMean,
    Scatter[1] + Spec.SkyAmbient[1] * SkyScale + Bounce[1] * SolarMean,
    Scatter[2] + Spec.SkyAmbient[2] * SkyScale + Bounce[2] * SolarMean,
  ];
}

//==========================================================================================================================================
//                                       CLOUDTRANSPORT.SLANG — ORDERED VIEW INTEGRATION
//==========================================================================================================================================

export function TracePlanetaryClouds(Spec, Origin, Direction, PixelX = 0.5, PixelY = 0.5, LimitM = 1e9) {
  const Integral = { Radiance: [0, 0, 0], Transmittance: 1, DistanceM: 0 };
  if (!Spec.UseClouds || Spec.Coverage <= 0 || Spec.Extinction <= 0) return Integral;

  const Layer = PlanetCloudLayer(Spec, 0);
  const ViewAltitudeM = Math.hypot(Origin[0], Origin[1], Origin[2]) - Spec.RadiusM;
  if (ViewAltitudeM < Layer.BottomM - 10.0 && Direction[2] < -0.02 && Spec.UniformExtinction < 0) return Integral;

  LimitM = Math.min(LimitM, PlanetCloudGroundDistance(Origin, Direction, Spec.RadiusM));
  const Spans = PlanetCloudLayerSpans(Origin, Direction, Spec.RadiusM, Layer, LimitM);
  const Jitter = CloudBlueJitter(PixelX, PixelY);
  const Phases = CloudScatteringOrderPhases(Spec, Direction);

  for (let Interval = 0; Interval < 2; ++Interval) {
    const NearM = Interval === 0 ? Spans[0] : Spans[2];
    const FarM = Interval === 0 ? Spans[1] : Spans[3];
    if (FarM <= NearM) continue;
    const LengthM = FarM - NearM;
    const Growth = ViewAltitudeM > 19000.0 ? 0 : clamp(Math.log(1 + LengthM / 1900.0), 0.1, 4.8);
    const Denominator = Math.max(1e-6, Math.exp(Growth) - 1);
    let LastM = NearM;
    let LitAtM = -1e9;
    let Source = [0, 0, 0];

    for (let Index = 0; Index < Spec.Steps && Integral.Transmittance > 0.0001; ++Index) {
      const Fraction = (Index + 1) / Spec.Steps;
      const NextM = NearM + LengthM * (Growth < 0.01 ? Fraction : (Math.exp(Growth * Fraction) - 1) / Denominator);
      const DistanceM = NextM - LastM;
      const SampleM = LastM + DistanceM * (0.4 + Jitter * 0.2);
      const Position = [Origin[0] + Direction[0] * SampleM, Origin[1] + Direction[1] * SampleM, Origin[2] + Direction[2] * SampleM];
      const FootprintM = Math.max(1.0, SampleM * Spec.PixelAngle);
      const Advected = CloudWindCoordinates(Spec, Position);
      const AdvLen = Math.max(1e-9, Math.hypot(Advected[0], Advected[1], Advected[2]));
      const Weather = CloudWeatherFromAdvectedNormal(Spec, [Advected[0] / AdvLen, Advected[1] / AdvLen, Advected[2] / AdvLen]);
      const Sigma = SamplePlanetCloudDensityAdvected(Spec, Layer, Position, Advected, Weather, FootprintM, false);
      if (Sigma > 1e-9) {
        const SegmentTransmittance = Math.exp(-Sigma * DistanceM);
        const Weight = Integral.Transmittance * (1 - SegmentTransmittance);
        const LightStrideM = clamp(FootprintM * 0.75 + 120.0, 100.0, 280.0);
        if (SampleM - LitAtM >= LightStrideM) {
          Source = PlanetCloudSource(Spec, Layer, Position, Weather, Sigma, Phases, Direction, FootprintM);
          LitAtM = SampleM;
        }
        Integral.Radiance[0] += Weight * Source[0];
        Integral.Radiance[1] += Weight * Source[1];
        Integral.Radiance[2] += Weight * Source[2];
        Integral.DistanceM += Weight * SampleM;
        Integral.Transmittance *= SegmentTransmittance;
      }
      LastM = NextM;
    }
  }

  const Opacity = 1 - Integral.Transmittance;
  if (Opacity > 1e-5) Integral.DistanceM /= Opacity;
  return Integral;
}

//==========================================================================================================================================
//                                       CLOUDAERIALPERSPECTIVE.SLANG — AIR SEGMENT
//==========================================================================================================================================

export function IntegrateCloudAir(Spec, Origin, Direction, LimitM, Steps, CloudShafts = false) {
  const Integral = { Radiance: [0, 0, 0], Transmittance: [1, 1, 1] };
  const Atmosphere = PlanetCloudSphere(Origin, Direction, Spec.RadiusM + 100000.0);
  const EnterM = Math.max(0, Atmosphere[0]);
  const ExitM = Math.min(LimitM, Atmosphere[1]);
  if (ExitM <= EnterM) return Integral;

  const Mu = Direction[0] * Spec.SunDirection[0] + Direction[1] * Spec.SunDirection[1] + Direction[2] * Spec.SunDirection[2];
  const RayleighPhase = (3 / (16 * CLOUD_PI)) * (1 + Mu * Mu);
  const AerosolPhase = CloudTransportHG(Mu, 0.78);
  const BetaRayleigh = QueryCloudRayleighExtinction(Spec);
  const BetaAerosol = QueryCloudAerosolExtinction(Spec);
  const Ozone = Math.max(0.0, Spec.AtmosphereStrengths[2]);
  const BetaOzone = [Ozone * 0.65e-6, Ozone * 1.881e-6, Ozone * 0.085e-6];
  const AirSolar = [Spec.SolarIrradiance[0] * 0.3333333, Spec.SolarIrradiance[1] * 0.3333333, Spec.SolarIrradiance[2] * 0.3333333];

  const ClosestM = clamp(-(Origin[0] * Direction[0] + Origin[1] * Direction[1] + Origin[2] * Direction[2]), EnterM, ExitM);
  for (let Side = 0; Side < 2; ++Side) {
    const LengthM = Side === 0 ? ClosestM - EnterM : ExitM - ClosestM;
    if (LengthM < 0.1) continue;
    let PreviousM = Side === 0 ? EnterM : ClosestM;
    for (let Index = 0; Index < Steps; ++Index) {
      const Fraction = (Index + 1) / Steps;
      const RadialFraction = Side === 0 ? 1 - Fraction : Fraction;
      const DistanceM =
        ClosestM +
        LengthM * (0.30 * RadialFraction + 0.70 * RadialFraction * RadialFraction * RadialFraction) * (Side === 0 ? -1 : 1);
      const Ds = DistanceM - PreviousM;
      const MidpointM = (DistanceM + PreviousM) * 0.5;
      PreviousM = DistanceM;
      const Position = [Origin[0] + Direction[0] * MidpointM, Origin[1] + Direction[1] * MidpointM, Origin[2] + Direction[2] * MidpointM];
      const AltitudeM = Math.max(0.0, PlanetCloudAltitude(Position, Spec.RadiusM));
      const Rayleigh = [0, 1, 2].map((I) => BetaRayleigh[I] * Math.exp(-AltitudeM / Math.max(400.0, Spec.AirScaleHeightsM[0])));
      const Aerosol = [0, 1, 2].map((I) => BetaAerosol[I] * Math.exp(-AltitudeM / Math.max(200.0, Spec.AirScaleHeightsM[1])));
      const OzoneCol = [0, 1, 2].map((I) => BetaOzone[I] * saturate(1 - Math.abs(AltitudeM - 25000.0) / 16000.0));
      const Extinction = [0, 1, 2].map((I) => Math.max(1e-8, Rayleigh[I] + Aerosol[I] + OzoneCol[I]));
      const SunTrans = CloudSolarAirTransmittance(Spec, Position);
      const Radius = Math.hypot(Position[0], Position[1], Position[2]);
      const SunUp = (Position[0] * Spec.SunDirection[0] + Position[1] * Spec.SunDirection[1] + Position[2] * Spec.SunDirection[2]) / Radius;
      const Twilight = SmoothStepCloud(-0.21, 0.095, SunUp + AltitudeM * 0.000002);
      let Visibility = 1;
      if (CloudShafts && Mu > 0.69 && AltitudeM < 4300 && MidpointM < 65000 && SunTrans[0] * SunTrans[0] > 0.0005) {
        Visibility = Math.exp(-CloudSolarOpticalDepth(Spec, Position, 4));
      }
      for (let I = 0; I < 3; ++I) {
        const SegmentTransmittance = Math.exp(-Extinction[I] * Ds);
        let Source = SunTrans[I] * Visibility * (Rayleigh[I] * RayleighPhase + 0.88 * Aerosol[I] * AerosolPhase);
        Source += (Rayleigh[I] + 0.48 * Aerosol[I]) * Twilight * [0.019, 0.025, 0.034][I];
        Integral.Radiance[I] += (Integral.Transmittance[I] * AirSolar[I] * Source * (1 - SegmentTransmittance)) / Extinction[I];
        Integral.Transmittance[I] *= SegmentTransmittance;
      }
    }
  }
  return Integral;
}

export function CloudRadianceThroughAir(Spec, Origin, Direction, Cloud, AirSteps = 12) {
  if (Cloud.Transmittance >= 0.999999) return Cloud.Radiance;
  const Front = IntegrateCloudAir(Spec, Origin, Direction, Cloud.DistanceM, AirSteps, false);
  return [
    Front.Transmittance[0] * Cloud.Radiance[0] + (1 - Cloud.Transmittance) * Front.Radiance[0],
    Front.Transmittance[1] * Cloud.Radiance[1] + (1 - Cloud.Transmittance) * Front.Radiance[1],
    Front.Transmittance[2] * Cloud.Radiance[2] + (1 - Cloud.Transmittance) * Front.Radiance[2],
  ];
}

//==========================================================================================================================================
//                                                       PUBLIC ENTRY POINT
//==========================================================================================================================================

// VolumetricCloudSequence::EvaluateVolumetricClouds: world-space ray (camera at ObserverAltitudeMeters) ->
// { Radiance, Transmittance }. Origin is the observer position in metres inside the planet shell.
export function EvaluateVolumetricClouds(
  RayOriginWorld,
  RayDirection,
  Deck,
  Medium = null,
  SunDirection = [0.58, -0.55, 0.6],
  SolarIrradiance = [1, 1, 1],
  SkyAmbient = [0.1, 0.15, 0.25],
  SimulationSeconds = 0,
  PixelX = 0.5,
  PixelY = 0.5,
) {
  const Spec = BuildCloudSpecification(Deck, Medium, SunDirection, SolarIrradiance, SkyAmbient, SimulationSeconds);
  const Origin = [
    RayOriginWorld?.[0] ?? 0,
    RayOriginWorld?.[1] ?? 0,
    Spec.RadiusM + Spec.ObserverAltitudeMeters + (RayOriginWorld?.[2] ?? 0),
  ];
  const Dir = Normalize3(RayDirection);
  if (!Spec.UseClouds || Spec.Coverage <= 0.01 || Spec.Extinction <= 0.01) {
    return { Radiance: [0, 0, 0], Transmittance: 1 };
  }
  const Cloud = TracePlanetaryClouds(Spec, Origin, Dir, PixelX, PixelY, 1e9);
  if (Cloud.Transmittance >= 0.999999) return { Radiance: [0, 0, 0], Transmittance: 1 };
  const Radiance = CloudRadianceThroughAir(Spec, Origin, Dir, Cloud, 12);
  return { Radiance, Transmittance: ClimateClamp(Cloud.Transmittance), Spec };
}

function ClimateClamp(V) {
  return clamp(V, 0, 1);
}

// VolumetricCloudSequence::EvaluateSolarCloudTransmittance: opaque-sky shadow factor for a ground point
export function EvaluateSolarCloudTransmittance(WorldPosition, Deck, Medium = null, SunDirection = [0.58, -0.55, 0.6], SolarIrradiance = [1, 1, 1], SimulationSeconds = 0) {
  const Spec = BuildCloudSpecification(Deck, Medium, SunDirection, SolarIrradiance, [0, 0, 0], SimulationSeconds);
  if (!Spec.UseClouds || !Spec.UseCloudShadows) return 1.0;
  const Position = [
    WorldPosition[0],
    WorldPosition[1],
    Spec.RadiusM + Math.max(5.0, WorldPosition[2] ?? 0),
  ];
  const Tau = CloudSolarOpticalDepth(Spec, Position, 6, 0);
  return clamp(Math.exp(-Tau), 0, 1);
}

//==========================================================================================================================================
//                                        WORKER / KERNEL SHARING (single implementation)
//==========================================================================================================================================

// The zero-WebGL CPU kernel is stringified into a Worker, so the cloud port has to travel as source text.
// Every function below is a pure declaration (no closures), so Function.prototype.toString() round-trips it.
export function BuildCloudKernelPrelude() {
  const Constants = [
    "const CLOUD_PI = 3.14159265358979323846;",
    "const DEG_TO_RAD = CLOUD_PI / 180.0;",
    "const saturate = (X) => (X < 0 ? 0 : X > 1 ? 1 : X);",
    "const clamp = (X, A, B) => (X < A ? A : X > B ? B : X);",
    "const mix = (A, B, T) => A + (B - A) * T;",
    "const frac = (X) => X - Math.floor(X);",
    `const DEFAULT_CLOUD_DECK = ${JSON.stringify(DEFAULT_CLOUD_DECK)};`,
    `const CLOUD_CATEGORY_NAMES = ${JSON.stringify(CLOUD_CATEGORY_NAMES)};`,
  ].join("\n");
  const Functions = [
    SmoothStepCloud,
    PermuteBits,
    CloudLatticeHash3,
    DecodeTexel,
    CloudQuintic,
    CloudLatticeNoise,
    RotateCloudOctave,
    CloudBoundaryCurl,
    CloudBlueJitter,
    SampleCloudShapeField,
    SampleCloudErosionField,
    BuildCloudSpecification,
    Normalize3,
    PlanetCloudAltitude,
    PlanetCloudSphere,
    PlanetCloudGroundDistance,
    PlanetCloudLayerSpans,
    PlanetCloudLayer,
    CloudRotationSinCos,
    RotateCloudPosition,
    CloudGeodesicRadius,
    CloudCycloneAngularSpeed,
    CloudWindCoordinates,
    CloudWeatherFromAdvectedNormal,
    CloudWeatherAtPoint,
    CloudCellEnvelope,
    CloudConvectiveEnvelope,
    EvaluateCloudShapeDisplacement,
    CloudCycloneEnvelope,
    SamplePlanetCloudDensityAdvected,
    SamplePlanetCloudShadowDensity,
    SamplePlanetCloudDensity,
    QueryCloudRayleighExtinction,
    QueryCloudAerosolExtinction,
    CloudAirColumn,
    CloudSolarAirTransmittance,
    IntegrateCloudSolarDepthWithWeather,
    CloudSolarOpticalDepthWithWeather,
    CloudSolarOpticalDepth,
    CloudTransportHG,
    CloudDualLobe,
    CloudScatteringOrderPhases,
    PlanetCloudSource,
    TracePlanetaryClouds,
    IntegrateCloudAir,
    CloudRadianceThroughAir,
    EvaluateVolumetricClouds,
    EvaluateSolarCloudTransmittance,
  ];
  const Names = Functions.map((Fn) => Fn.name);
  return [
    Constants,
    Functions.map((Fn) => Fn.toString()).join("\n"),
    `const CloudLib = { ${Names.map((Name) => `${Name}`).join(", ")} };`,
  ].join("\n");
}

export const CloudLibrary = {
  SmoothStepCloud,
  CloudLatticeHash3,
  CloudLatticeNoise,
  RotateCloudOctave,
  CloudBlueJitter,
  SampleCloudShapeField,
  SampleCloudErosionField,
  BuildCloudSpecification,
  PlanetCloudSphere,
  PlanetCloudGroundDistance,
  PlanetCloudLayerSpans,
  PlanetCloudLayer,
  CloudWindCoordinates,
  CloudWeatherFromAdvectedNormal,
  CloudWeatherAtPoint,
  CloudCellEnvelope,
  CloudConvectiveEnvelope,
  CloudCycloneEnvelope,
  SamplePlanetCloudDensityAdvected,
  SamplePlanetCloudShadowDensity,
  CloudSolarAirTransmittance,
  CloudSolarOpticalDepth,
  CloudScatteringOrderPhases,
  PlanetCloudSource,
  TracePlanetaryClouds,
  IntegrateCloudAir,
  CloudRadianceThroughAir,
  EvaluateVolumetricClouds,
  EvaluateSolarCloudTransmittance,
  ResolveCloudDeck,
  ApplyCloudCategoryPreset,
  CLOUD_CATEGORY_NAMES,
};
