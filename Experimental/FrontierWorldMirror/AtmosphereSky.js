//==========================================================================================================================================
//                                                      ATMOSPHERESKY.JS
//==========================================================================================================================================
// 1:1 JavaScript port of Engine/GeometricRaster/Atmosphere/AtmosphericSkySequence.cpp
// (Bruneton/Hillaire Rayleigh + Mie + Ozone Transmittance LUT + infinite-order Multi-Scatter LUT +
//  192x108 geometric-limb SkyView LUT + Neckel & Labs solar photosphere + K/F corona + optical glare)
// plus the volumetric 3D cloud raymarcher (CPU parity), moon phase/albedos, and deterministic starfield.

export const SKY_VIEW_WIDTH = 192;
export const SKY_VIEW_HEIGHT = 108;
export const TRANS_W = 256;
export const TRANS_H = 64;
export const MS_W = 32;
export const MS_H = 32;

const EARTH_RADIUS_M = 6360000.0;
const DEFAULT_THICKNESS_M = 60000.0;
const OBSERVER_ALT_M = 100.0;

const TAU = Math.PI * 2.0;
const DEG_TO_RAD = Math.PI / 180.0;
const RAD_TO_DEG = 180.0 / Math.PI;
const PI_F = Math.PI;
const INV_FOUR_PI = 1.0 / (4.0 * Math.PI);

// AtmosphericSkySequence.cpp anon-namespace sea-level coefficients [m^-1]
const SeaLevelRayleighScatter = [5.802e-6, 13.558e-6, 33.1e-6];
const SeaLevelMieScatter = 3.996e-6;
const SeaLevelMieExtinction = 4.40e-6;
const PeakOzoneAbs = [0.650e-6, 1.881e-6, 0.085e-6];

const TRANS_STEPS = 64;
const MS_DIRS = 32;
const MS_STEPS = 24;
const GOLDEN_RATIO = 1.6180339887498948482;
const SKY_STEPS = 40;
const GND_STEPS = 28;
const HALF_H = SKY_VIEW_HEIGHT >> 1; // 54

export const MOON_PRESET_TINTS = [
  [0.94, 0.93, 0.90], // 0: Luna
  [1.00, 0.56, 0.34], // 1: Ember
  [0.76, 0.90, 1.00], // 2: Glacier
  [0.98, 0.88, 0.42], // 3: Sulfur
  [0.72, 0.74, 0.82], // 4: Shroud
  [0.84, 0.96, 0.94], // 5: Shard
];

function Clamp(v, lo, hi) {
  return v < lo ? lo : v > hi ? hi : v;
}

function ClampUnit(v) {
  return Clamp(v, -1.0, 1.0);
}

function IntersectSphereDist(originZ, dirZ, radius) {
  const b = originZ * dirZ;
  const c = originZ * originZ - radius * radius;
  const disc = b * b - c;
  if (disc < 0.0) return [-1.0, -1.0];
  const root = Math.sqrt(disc);
  return [-b - root, -b + root];
}

//==========================================================================================================================================
//                                        ATMOSPHERIC MEDIUM CONFIGURATION (C++ STRUCT PARITY)
//==========================================================================================================================================

export const DEFAULT_ATMOSPHERIC_MEDIUM = {
  AtmosphereEnabled: false,
  RayleighStrength: 1.0,
  MieStrength: 1.0,
  MieAnisotropy: 0.78,
  OzoneStrength: 1.2,
  RayleighScaleHeightMeters: 8000.0,
  MieScaleHeightMeters: 1200.0,
  AtmosphereThicknessMeters: DEFAULT_THICKNESS_M,
  PlanetRadiusMeters: EARTH_RADIUS_M,
  LimbGlowIntensity: 1.0,
  SkyBrightnessMultiplier: 1.0,
  SkyTint: [1.0, 1.0, 1.0],
  GroundAlbedo: [0.045, 0.088, 0.165],
  OrbitalViewEnabled: false,
  ObserverAltitudeMeters: OBSERVER_ALT_M,
  SolarIntensityMultiplier: 22.0,
};

// Maps the inspector atmosphere record (or an already-resolved C++ medium) onto AtmosphericMediumConfiguration
export function ResolveAtmosphericMedium(atmo = null) {
  const Src = atmo || {};
  const Num = (Raw, Fallback) => {
    const N = Number(Raw);
    return Number.isFinite(N) ? N : Fallback;
  };
  const Rgb = (Raw, Fallback) =>
    Array.isArray(Raw) && Raw.length >= 3
      ? [Number(Raw[0]) || 0, Number(Raw[1]) || 0, Number(Raw[2]) || 0]
      : Fallback.slice();
  const D = DEFAULT_ATMOSPHERIC_MEDIUM;
  return {
    AtmosphereEnabled: Src.AtmosphereEnabled ?? Src.enabled ?? D.AtmosphereEnabled,
    RayleighStrength: Math.max(0, Num(Src.RayleighStrength ?? Src.rayleigh, D.RayleighStrength)),
    MieStrength: Math.max(0, Num(Src.MieStrength ?? Src.mie, D.MieStrength)),
    MieAnisotropy: Math.max(0, Math.min(0.99, Num(Src.MieAnisotropy ?? Src.mieG, D.MieAnisotropy))),
    OzoneStrength: Math.max(0, Num(Src.OzoneStrength ?? Src.ozone, D.OzoneStrength)),
    RayleighScaleHeightMeters: Math.max(500, Num(Src.RayleighScaleHeightMeters ?? Src.rayleighH, D.RayleighScaleHeightMeters)),
    MieScaleHeightMeters: Math.max(200, Num(Src.MieScaleHeightMeters ?? Src.mieH, D.MieScaleHeightMeters)),
    AtmosphereThicknessMeters: Math.max(10, Num(Src.AtmosphereThicknessMeters ?? Src.atmoTop, D.AtmosphereThicknessMeters)),
    PlanetRadiusMeters: Math.max(500000, Num(Src.PlanetRadiusMeters ?? Src.planetRadius, D.PlanetRadiusMeters)),
    LimbGlowIntensity: Math.max(0, Num(Src.LimbGlowIntensity ?? Src.horizonGlow, D.LimbGlowIntensity)),
    SkyBrightnessMultiplier: Math.max(0, Num(Src.SkyBrightnessMultiplier ?? Src.skyBrightness, D.SkyBrightnessMultiplier)),
    SkyTint: Rgb(Src.SkyTint ?? Src.skyTint, D.SkyTint),
    GroundAlbedo: Rgb(Src.GroundAlbedo ?? Src.groundAlbedo, D.GroundAlbedo),
    OrbitalViewEnabled: Boolean(Src.OrbitalViewEnabled ?? Src.orbital),
    ObserverAltitudeMeters: Num(Src.ObserverAltitudeMeters ?? Src.observerAltitude, D.ObserverAltitudeMeters),
    SolarIntensityMultiplier: Math.max(0, Num(Src.SolarIntensityMultiplier ?? Src.solarIntensity ?? Src.solarIntensityScale, D.SolarIntensityMultiplier)),
  };
}

//==========================================================================================================================================
//                                        SPHERICAL GEOMETRY (AtmosphericSkySequence.cpp anon namespace)
//==========================================================================================================================================

function ComputeHorizonDistanceKm(BottomRadiusKm, TopRadiusKm) {
  return Math.sqrt(Math.max(0.0, (TopRadiusKm - BottomRadiusKm) * (TopRadiusKm + BottomRadiusKm)));
}

function QuerySphereBlockedKm(RadiusKm, CosinePolar, TargetRadiusKm) {
  if (CosinePolar >= 0.0) return false;
  const Disc =
    RadiusKm * RadiusKm * CosinePolar * CosinePolar -
    (RadiusKm - TargetRadiusKm) * (RadiusKm + TargetRadiusKm);
  return Disc >= 0.0 && RadiusKm > TargetRadiusKm + 1.0e-4;
}

function IntersectUpperSphereDistanceKm(RadiusKm, CosinePolar, TopRadiusKm) {
  const Disc = Math.max(
    0.0,
    RadiusKm * RadiusKm * CosinePolar * CosinePolar +
      (TopRadiusKm - RadiusKm) * (TopRadiusKm + RadiusKm),
  );
  return Math.max(0.0, -RadiusKm * CosinePolar + Math.sqrt(Disc));
}

function IntersectLowerSphereDistanceKm(RadiusKm, CosinePolar, BottomRadiusKm) {
  const Disc =
    RadiusKm * RadiusKm * CosinePolar * CosinePolar -
    (RadiusKm - BottomRadiusKm) * (RadiusKm + BottomRadiusKm);
  if (CosinePolar < 0.0 && Disc >= 0.0) {
    const Hit = -RadiusKm * CosinePolar - Math.sqrt(Disc);
    if (Hit > 0.0) return { Hits: true, DistanceKm: Hit };
  }
  return { Hits: false, DistanceKm: 0.0 };
}

function ComputeLimbPolarAngle(ViewerRadiusKm, BottomRadiusKm) {
  const SafeR = Math.max(BottomRadiusKm + 1.0e-4, ViewerRadiusKm);
  const TangentHeight = Math.sqrt(
    Math.max(0.0, (SafeR - BottomRadiusKm) * (SafeR + BottomRadiusKm)),
  );
  return Math.acos(ClampUnit(-TangentHeight / SafeR));
}

const MediumScratch = {
  RayScatterR: 0,
  RayScatterG: 0,
  RayScatterB: 0,
  MieScatter: 0,
  ExtinctionR: 0,
  ExtinctionG: 0,
  ExtinctionB: 0,
};

// Allocation-free variant used by every quadrature (millions of samples per bake)
function EvaluateMediumInto(AltitudeKm, Medium, Out) {
  const AltM = AltitudeKm > 0.0 ? AltitudeKm * 1000.0 : 0.0;
  const RayH = Medium.RayleighScaleHeightMeters > 500.0 ? Medium.RayleighScaleHeightMeters : 500.0;
  const MieH = Medium.MieScaleHeightMeters > 200.0 ? Medium.MieScaleHeightMeters : 200.0;
  const RayMul = Medium.RayleighStrength;
  const MieMul = Medium.MieStrength;
  const OzoneMul = Medium.OzoneStrength;

  const DenR = Math.exp(-AltM / RayH) * RayMul;
  const DenM = Math.exp(-AltM / MieH) * MieMul;
  const OzoneBand = 1.0 - Math.abs(AltM - 25000.0) / 15000.0;
  const DenO = (OzoneBand > 0.0 ? OzoneBand : 0.0) * OzoneMul;

  const RayScatterR = SeaLevelRayleighScatter[0] * 1000.0 * DenR;
  const RayScatterG = SeaLevelRayleighScatter[1] * 1000.0 * DenR;
  const RayScatterB = SeaLevelRayleighScatter[2] * 1000.0 * DenR;
  const MieScatter = SeaLevelMieScatter * 1000.0 * DenM;
  const MieExt = SeaLevelMieExtinction * 1000.0 * DenM;

  Out.RayScatterR = RayScatterR;
  Out.RayScatterG = RayScatterG;
  Out.RayScatterB = RayScatterB;
  Out.MieScatter = MieScatter;
  Out.ExtinctionR = RayScatterR + MieExt + PeakOzoneAbs[0] * 1000.0 * DenO;
  Out.ExtinctionG = RayScatterG + MieExt + PeakOzoneAbs[1] * 1000.0 * DenO;
  Out.ExtinctionB = RayScatterB + MieExt + PeakOzoneAbs[2] * 1000.0 * DenO;
  return Out;
}

function EvaluateMediumAtAltitudeKm(AltitudeKm, Medium) {
  const C = EvaluateMediumInto(AltitudeKm, Medium, {
    RayScatterR: 0,
    RayScatterG: 0,
    RayScatterB: 0,
    MieScatter: 0,
    ExtinctionR: 0,
    ExtinctionG: 0,
    ExtinctionB: 0,
  });
  return C;
}

function SampleLut2DBilinearRgb(Lut, Width, Height, U, V, Out) {
  const FX = Clamp(U, 0.0, 1.0) * (Width - 1);
  const FY = Clamp(V, 0.0, 1.0) * (Height - 1);
  const X0 = Math.min(Width - 2, Math.floor(FX));
  const Y0 = Math.min(Height - 2, Math.floor(FY));
  const TX = FX - X0;
  const TY = FY - Y0;

  const I00 = (Y0 * Width + X0) * 3;
  const I10 = I00 + 3;
  const I01 = I00 + Width * 3;
  const I11 = I01 + 3;
  const Mix = (A, B, T) => A + (B - A) * T;
  Out[0] = Mix(Mix(Lut[I00], Lut[I10], TX), Mix(Lut[I01], Lut[I11], TX), TY);
  Out[1] = Mix(Mix(Lut[I00 + 1], Lut[I10 + 1], TX), Mix(Lut[I01 + 1], Lut[I11 + 1], TX), TY);
  Out[2] = Mix(Mix(Lut[I00 + 2], Lut[I10 + 2], TX), Mix(Lut[I01 + 2], Lut[I11 + 2], TX), TY);
  return Out;
}

function SampleTransmittanceSphericalLut(Lut, RadiusKm, CosPolar, BottomRadiusKm, TopRadiusKm, Out) {
  if (QuerySphereBlockedKm(RadiusKm, CosPolar, BottomRadiusKm)) {
    Out[0] = 0.0;
    Out[1] = 0.0;
    Out[2] = 0.0;
    return Out;
  }
  const ClampedR = Clamp(RadiusKm, BottomRadiusKm, TopRadiusKm);
  const HorizonKm = ComputeHorizonDistanceKm(BottomRadiusKm, TopRadiusKm);
  const RhoKm = Math.sqrt(Math.max(0.0, (ClampedR - BottomRadiusKm) * (ClampedR + BottomRadiusKm)));
  const DistTopKm = IntersectUpperSphereDistanceKm(ClampedR, ClampUnit(CosPolar), TopRadiusKm);
  const DMinKm = Math.max(0.0, TopRadiusKm - ClampedR);
  const DMaxKm = RhoKm + HorizonKm;
  const U = Clamp((DistTopKm - DMinKm) / Math.max(1.0e-5, DMaxKm - DMinKm), 0.0, 1.0);
  const V = Clamp(RhoKm / Math.max(1.0e-5, HorizonKm), 0.0, 1.0);
  return SampleLut2DBilinearRgb(Lut, TRANS_W, TRANS_H, U, V, Out);
}

function SampleMultiScatterGridLut(Lut, AltitudeKm, SolarCosine, ThicknessKm, Out) {
  const U = Clamp(0.5 * (SolarCosine + 1.0), 0.0, 1.0);
  const V = Math.sqrt(Clamp(AltitudeKm / Math.max(1.0e-3, ThicknessKm), 0.0, 1.0));
  return SampleLut2DBilinearRgb(Lut, MS_W, MS_H, U, V, Out);
}

//==========================================================================================================================================
//                                     CACHED 256x64 TRANSMITTANCE + 32x32 MULTI-SCATTER FOUNDATION LUTS
//==========================================================================================================================================

const Scratch3 = [0, 0, 0];

const Foundation = {
  Trans: null,
  Multi: null,
  BottomKm: -1,
  TopKm: -1,
  RayStrength: -1,
  MieStrength: -1,
  OzoneStrength: -1,
  RayScaleH: -1,
  MieScaleH: -1,
  GroundAlbedoHash: -1,
};

export function GetTransmittanceLut() {
  return Foundation.Trans;
}

export function GetMultiScatterLut() {
  return Foundation.Multi;
}

function EnsureFoundationLuts(Medium) {
  const GroundKm = Math.max(500.0, Medium.PlanetRadiusMeters * 0.001);
  const ThicknessKm = Math.max(10.0, Medium.AtmosphereThicknessMeters * 0.001);
  const TopKm = GroundKm + ThicknessKm;
  const RayMul = Math.max(0.0, Medium.RayleighStrength);
  const MieMul = Math.max(0.0, Medium.MieStrength);
  const OzoneMul = Math.max(0.0, Medium.OzoneStrength);
  const RayH = Math.max(500.0, Medium.RayleighScaleHeightMeters);
  const MieH = Math.max(200.0, Medium.MieScaleHeightMeters);
  const AlbHash =
    Medium.GroundAlbedo[0] + 7.1 * Medium.GroundAlbedo[1] + 31.3 * Medium.GroundAlbedo[2];

  const MediumChanged =
    !Foundation.Trans ||
    !Foundation.Multi ||
    Math.abs(Foundation.BottomKm - GroundKm) > 1.0e-4 ||
    Math.abs(Foundation.TopKm - TopKm) > 1.0e-4 ||
    Math.abs(Foundation.RayStrength - RayMul) > 1.0e-5 ||
    Math.abs(Foundation.MieStrength - MieMul) > 1.0e-5 ||
    Math.abs(Foundation.OzoneStrength - OzoneMul) > 1.0e-5 ||
    Math.abs(Foundation.RayScaleH - RayH) > 1.0e-2 ||
    Math.abs(Foundation.MieScaleH - MieH) > 1.0e-2 ||
    Math.abs(Foundation.GroundAlbedoHash - AlbHash) > 1.0e-5;

  if (!MediumChanged) {
    return { GroundKm, TopKm, ThicknessKm };
  }

  Foundation.BottomKm = GroundKm;
  Foundation.TopKm = TopKm;
  Foundation.RayStrength = RayMul;
  Foundation.MieStrength = MieMul;
  Foundation.OzoneStrength = OzoneMul;
  Foundation.RayScaleH = RayH;
  Foundation.MieScaleH = MieH;
  Foundation.GroundAlbedoHash = AlbHash;

  // Pass 1: 256x64 Bruneton/Hillaire spherical-horizon Transmittance LUT (64 quadratic Fraction^2 steps)
  const Trans = new Float32Array(TRANS_W * TRANS_H * 3);
  const HorizonKm = ComputeHorizonDistanceKm(GroundKm, TopKm);
  for (let Y = 0; Y < TRANS_H; ++Y) {
    const V = Y / (TRANS_H - 1);
    const RhoKm = V * HorizonKm;
    const RadiusKm = Math.sqrt(RhoKm * RhoKm + GroundKm * GroundKm);
    const DMinKm = Math.max(0.0, TopKm - RadiusKm);
    const DMaxKm = RhoKm + HorizonKm;
    for (let X = 0; X < TRANS_W; ++X) {
      const U = X / (TRANS_W - 1);
      const DistTopKm = DMinKm + U * (DMaxKm - DMinKm);
      const CosPolar =
        DistTopKm <= 1.0e-5
          ? 1.0
          : ClampUnit(
              (HorizonKm * HorizonKm - RhoKm * RhoKm - DistTopKm * DistTopKm) /
                (2.0 * RadiusKm * DistTopKm),
            );

      let OptR = 0.0;
      let OptG = 0.0;
      let OptB = 0.0;
      for (let S = 0; S < TRANS_STEPS; ++S) {
        const F0 = S / TRANS_STEPS;
        const F1 = (S + 1) / TRANS_STEPS;
        const T0 = DistTopKm * F0 * F0;
        const T1 = DistTopKm * F1 * F1;
        const T = 0.5 * (T0 + T1);
        const Step = Math.max(1.0e-5, T1 - T0);
        const RK = Math.sqrt(
          Math.max(
            GroundKm * GroundKm,
            RadiusKm * RadiusKm + 2.0 * RadiusKm * CosPolar * T + T * T,
          ),
        );
        const C = EvaluateMediumInto(RK - GroundKm, Medium, MediumScratch);
        OptR += C.ExtinctionR * Step;
        OptG += C.ExtinctionG * Step;
        OptB += C.ExtinctionB * Step;
      }
      const Idx = (Y * TRANS_W + X) * 3;
      Trans[Idx] = Math.exp(-OptR);
      Trans[Idx + 1] = Math.exp(-OptG);
      Trans[Idx + 2] = Math.exp(-OptB);
    }
  }
  Foundation.Trans = Trans;

  // Pass 2: 32x32 isotropic infinite-order Multi-Scatter LUT (Hillaire 2020 Fibonacci 32 dirs x 24 steps)
  const Multi = new Float32Array(MS_W * MS_H * 3);
  const InvDirs = 1.0 / MS_DIRS;
  for (let Y = 0; Y < MS_H; ++Y) {
    const V = Y / (MS_H - 1);
    const AltKm = Clamp(V * V * ThicknessKm, 0.01, Math.max(0.02, ThicknessKm - 0.01));
    const RadiusKm = GroundKm + AltKm;

    for (let X = 0; X < MS_W; ++X) {
      const U = X / (MS_W - 1);
      const SunCos = ClampUnit(2.0 * U - 1.0);
      const SunSin = Math.sqrt(Math.max(0.0, 1.0 - SunCos * SunCos));

      let Lum2R = 0.0;
      let Lum2G = 0.0;
      let Lum2B = 0.0;
      let FmsR = 0.0;
      let FmsG = 0.0;
      let FmsB = 0.0;

      for (let D = 0; D < MS_DIRS; ++D) {
        const CosPhi = 1.0 - 2.0 * (D + 0.5) * InvDirs;
        const SinPhi = Math.sqrt(Math.max(0.0, 1.0 - CosPhi * CosPhi));
        const Theta = (TAU * (D + 0.5)) / GOLDEN_RATIO;
        const DirX = SinPhi * Math.cos(Theta);
        const DirZ = CosPhi;
        const RaySunCos = ClampUnit(DirX * SunSin + DirZ * SunCos);

        const GroundHit = IntersectLowerSphereDistanceKm(RadiusKm, DirZ, GroundKm);
        const MaxD = GroundHit.Hits
          ? GroundHit.DistanceKm
          : IntersectUpperSphereDistanceKm(RadiusKm, DirZ, TopKm);
        if (MaxD <= 1.0e-5) {
          continue;
        }
        const Step = MaxD / MS_STEPS;

        let TR = 1.0;
        let TG = 1.0;
        let TB = 1.0;
        let RayL2R = 0.0;
        let RayL2G = 0.0;
        let RayL2B = 0.0;
        let RayFmsR = 0.0;
        let RayFmsG = 0.0;
        let RayFmsB = 0.0;

        for (let S = 0; S < MS_STEPS; ++S) {
          const T = (S + 0.5) * Step;
          const RK = Math.sqrt(
            Math.max(GroundKm * GroundKm, RadiusKm * RadiusKm + 2.0 * RadiusKm * DirZ * T + T * T),
          );
          const C = EvaluateMediumInto(RK - GroundKm, Medium, MediumScratch);

          const SafeER = Math.max(1.0e-7, C.ExtinctionR);
          const SafeEG = Math.max(1.0e-7, C.ExtinctionG);
          const SafeEB = Math.max(1.0e-7, C.ExtinctionB);
          const SegTR = Math.exp(-SafeER * Step);
          const SegTG = Math.exp(-SafeEG * Step);
          const SegTB = Math.exp(-SafeEB * Step);
          const IntR = (1.0 - SegTR) / SafeER;
          const IntG = (1.0 - SegTG) / SafeEG;
          const IntB = (1.0 - SegTB) / SafeEB;

          const ScatR = C.RayScatterR + C.MieScatter;
          const ScatG = C.RayScatterG + C.MieScatter;
          const ScatB = C.RayScatterB + C.MieScatter;

          const SampleSunCos = ClampUnit((RadiusKm * SunCos + T * RaySunCos) / RK);
          const SunT = SampleTransmittanceSphericalLut(Trans, RK, SampleSunCos, GroundKm, TopKm, Scratch3);

          RayL2R += TR * ScatR * INV_FOUR_PI * SunT[0] * IntR;
          RayL2G += TG * ScatG * INV_FOUR_PI * SunT[1] * IntG;
          RayL2B += TB * ScatB * INV_FOUR_PI * SunT[2] * IntB;

          RayFmsR += TR * ScatR * IntR;
          RayFmsG += TG * ScatG * IntG;
          RayFmsB += TB * ScatB * IntB;

          TR *= SegTR;
          TG *= SegTG;
          TB *= SegTB;
        }

        if (GroundHit.Hits) {
          const HitSunCos = ClampUnit((RadiusKm * SunCos + MaxD * RaySunCos) / GroundKm);
          if (HitSunCos > 0.0) {
            const GroundSunT = SampleTransmittanceSphericalLut(
              Trans,
              GroundKm,
              HitSunCos,
              GroundKm,
              TopKm,
              Scratch3,
            );
            const DiffFactor = HitSunCos / PI_F;
            RayL2R += TR * Medium.GroundAlbedo[0] * GroundSunT[0] * DiffFactor;
            RayL2G += TG * Medium.GroundAlbedo[1] * GroundSunT[1] * DiffFactor;
            RayL2B += TB * Medium.GroundAlbedo[2] * GroundSunT[2] * DiffFactor;
          }
        }

        // 4*pi / MsDirs * (1/(4*pi)) == 1 / MsDirs for isotropic multi-scatter spherical integration
        Lum2R += RayL2R * InvDirs;
        Lum2G += RayL2G * InvDirs;
        Lum2B += RayL2B * InvDirs;
        FmsR += RayFmsR * InvDirs;
        FmsG += RayFmsG * InvDirs;
        FmsB += RayFmsB * InvDirs;
      }

      const Idx = (Y * MS_W + X) * 3;
      Multi[Idx] = Lum2R / Math.max(1.0e-3, 1.0 - Math.min(0.95, FmsR));
      Multi[Idx + 1] = Lum2G / Math.max(1.0e-3, 1.0 - Math.min(0.95, FmsG));
      Multi[Idx + 2] = Lum2B / Math.max(1.0e-3, 1.0 - Math.min(0.95, FmsB));
    }
  }
  Foundation.Multi = Multi;
  return { GroundKm, TopKm, ThicknessKm };
}

//==========================================================================================================================================
//                                        ComputeSolarTransmittance (C++ public static)
//==========================================================================================================================================

export function ComputeSolarTransmittance(SunDirection, atmo = {}, ObserverAltitudeMeters = null) {
  const Medium = ResolveAtmosphericMedium(atmo);
  const AltObs = ObserverAltitudeMeters === null || ObserverAltitudeMeters === undefined
    ? Medium.ObserverAltitudeMeters
    : Number(ObserverAltitudeMeters);

  const GroundKm = Math.max(500.0, Medium.PlanetRadiusMeters * 0.001);
  const ThicknessKm = Math.max(10.0, Medium.AtmosphereThicknessMeters * 0.001);
  const TopKm = GroundKm + ThicknessKm;
  const AltKm = Math.max(0.001, AltObs * 0.001);
  if (AltKm >= ThicknessKm) {
    return [1.0, 1.0, 1.0];
  }

  const SunMag = Math.sqrt(SunDirection[0] * SunDirection[0] + SunDirection[1] * SunDirection[1] + SunDirection[2] * SunDirection[2]);
  const CosPolar = SunMag > 1.0e-6 ? ClampUnit(SunDirection[2] / SunMag) : 1.0;
  const RadiusKm = GroundKm + AltKm;

  if (QuerySphereBlockedKm(RadiusKm, CosPolar, GroundKm)) {
    return [0.0, 0.0, 0.0];
  }
  const DistTopKm = IntersectUpperSphereDistanceKm(RadiusKm, CosPolar, TopKm);
  if (DistTopKm <= 1.0e-5) {
    return [1.0, 1.0, 1.0];
  }

  let OptR = 0.0;
  let OptG = 0.0;
  let OptB = 0.0;
  for (let S = 0; S < TRANS_STEPS; ++S) {
    const F0 = S / TRANS_STEPS;
    const F1 = (S + 1) / TRANS_STEPS;
    const T0 = DistTopKm * F0 * F0;
    const T1 = DistTopKm * F1 * F1;
    const T = 0.5 * (T0 + T1);
    const Step = Math.max(1.0e-5, T1 - T0);
    const RK = Math.sqrt(
      Math.max(GroundKm * GroundKm, RadiusKm * RadiusKm + 2.0 * RadiusKm * CosPolar * T + T * T),
    );
    const C = EvaluateMediumInto(RK - GroundKm, Medium, MediumScratch);
    OptR += C.ExtinctionR * Step;
    OptG += C.ExtinctionG * Step;
    OptB += C.ExtinctionB * Step;
  }
  return [Math.exp(-OptR), Math.exp(-OptG), Math.exp(-OptB)];
}

//==========================================================================================================================================
//                                        Transmittance / Multi-Scatter LUT sampling (C++ public)
//==========================================================================================================================================

export function SampleTransmittanceLutDirection(atmo, AltitudeMeters, PolarCosine) {
  const Medium = ResolveAtmosphericMedium(atmo);
  if (!Foundation.Trans || !Foundation.Multi) {
    const Sin = Math.sqrt(Math.max(0.0, 1.0 - PolarCosine * PolarCosine));
    return ComputeSolarTransmittance([Sin, 0.0, PolarCosine], Medium, AltitudeMeters);
  }
  const RadiusKm = Foundation.BottomKm + Math.max(0.0, AltitudeMeters * 0.001);
  const Out = SampleTransmittanceSphericalLut(
    Foundation.Trans,
    RadiusKm,
    PolarCosine,
    Foundation.BottomKm,
    Foundation.TopKm,
    [0, 0, 0],
  );
  return [Out[0], Out[1], Out[2]];
}

export function SampleMultiScatterLut(atmo, AltitudeMeters, SolarCosine) {
  const Medium = ResolveAtmosphericMedium(atmo);
  if (!Foundation.Trans || !Foundation.Multi) {
    return [0.0, 0.0, 0.0];
  }
  const ThicknessKm = Math.max(10.0, Medium.AtmosphereThicknessMeters * 0.001);
  const Out = SampleMultiScatterGridLut(
    Foundation.Multi,
    Math.max(0.0, AltitudeMeters * 0.001),
    SolarCosine,
    ThicknessKm,
    [0, 0, 0],
  );
  return [Out[0], Out[1], Out[2]];
}

//==========================================================================================================================================
//                                        SkyView LUT direction reconstruction / projection
//==========================================================================================================================================

// Reconstructs the SkyView LUT ray for texel-space coordinates (u in [0,1), v in [0,1] over rows 0..107)
export function ReconstructSkyViewDirection(u, v, LimbAngle = SkyViewState.LimbAngle) {
  const Limb = Math.max(1.0e-5, LimbAngle || 0.0);
  const Y = v * (SKY_VIEW_HEIGHT - 1);
  const LowerBank = Y >= HALF_H;
  const LocalV = LowerBank
    ? (Y - HALF_H) / (HALF_H - 1)
    : Y / (HALF_H - 1);
  const PolarAngle = LowerBank
    ? Math.min(PI_F - 1.0e-5, Limb + (PI_F - Limb) * LocalV * LocalV + 1.5e-4)
    : Math.max(0.0, Limb * (1.0 - LocalV * LocalV) - 1.5e-4);
  const CosPolar = Math.cos(PolarAngle);
  const SinPolar = Math.sqrt(Math.max(0.0, 1.0 - CosPolar * CosPolar));
  const Azimuth = (u - 0.5) * TAU;
  return [SinPolar * Math.cos(Azimuth), SinPolar * Math.sin(Azimuth), CosPolar];
}

// Projects a world direction onto SkyView LUT [u,v] using the C++ geometric-limb split
export function ProjectSkyViewCoordinates(dx, dy, dz, LimbAngle = SkyViewState.LimbAngle) {
  const Limb = Math.max(1.0e-5, LimbAngle || 0.0);
  const Azimuth = Math.atan2(dy, dx);
  const PolarAngle = Math.acos(ClampUnit(dz));

  let U = Azimuth / TAU + 0.5;
  U -= Math.floor(U);

  const LowerBank = PolarAngle > Limb;
  let CoordY;
  if (!LowerBank) {
    const Q = Math.sqrt(Clamp(1.0 - PolarAngle / Math.max(1.0e-4, Limb), 0.0, 1.0));
    CoordY = Q * (HALF_H - 1);
  } else {
    const Q = Math.sqrt(
      Clamp((PolarAngle - Limb) / Math.max(1.0e-4, PI_F - Limb), 0.0, 1.0),
    );
    CoordY = HALF_H + Q * (HALF_H - 1);
  }
  return [U, CoordY / (SKY_VIEW_HEIGHT - 1)];
}

//==========================================================================================================================================
//                                        BuildSkyViewLut (192x108 geometric-limb-centered)
//==========================================================================================================================================

export const SkyViewState = {
  Data: null,
  LimbAngle: 0.0,
  StartRadiusKm: EARTH_RADIUS_M * 0.001 + OBSERVER_ALT_M * 0.001,
  BottomKm: EARTH_RADIUS_M * 0.001,
  TopKm: EARTH_RADIUS_M * 0.001 + DEFAULT_THICKNESS_M * 0.001,
};

export function BuildSkyViewLut(sunDir, atmo = {}, outBuf = null, ObserverAltitudeMeters = null) {
  const Medium = ResolveAtmosphericMedium(atmo);
  const W = SKY_VIEW_WIDTH;
  const H = SKY_VIEW_HEIGHT;
  const data = outBuf && outBuf.length === W * H * 4 ? outBuf : new Float32Array(W * H * 4);

  const SunMag = Math.sqrt(sunDir[0] * sunDir[0] + sunDir[1] * sunDir[1] + sunDir[2] * sunDir[2]);
  const UnitSun = SunMag > 1.0e-6
    ? [sunDir[0] / SunMag, sunDir[1] / SunMag, sunDir[2] / SunMag]
    : [0.0, 0.0, 1.0];

  const GroundKm = Math.max(500.0, Medium.PlanetRadiusMeters * 0.001);
  const ThicknessKm = Math.max(10.0, Medium.AtmosphereThicknessMeters * 0.001);
  const TopKm = GroundKm + ThicknessKm;
  const MieG = Clamp(Medium.MieAnisotropy, 0.0, 0.99);

  const ObsAlt = ObserverAltitudeMeters === null || ObserverAltitudeMeters === undefined
    ? Medium.ObserverAltitudeMeters
    : Number(ObserverAltitudeMeters);
  const EffectiveAltM = Clamp(
    ObsAlt > 0.0 ? ObsAlt : OBSERVER_ALT_M,
    5.0,
    Math.max(100.0, (ThicknessKm - 0.05) * 1000.0),
  );
  const StartRadiusKm = GroundKm + EffectiveAltM * 0.001;

  // Poisson-free deterministic bake: foundation LUTs are rebuilt only when the medium changes
  EnsureFoundationLuts(Medium);
  const Trans = Foundation.Trans;
  const Multi = Foundation.Multi;

  if (!Medium.AtmosphereEnabled) {
    // Engine fallback: EvaluateSkyDome() shades the plain SkyRadiance dome when atmosphere is disabled
    const Base = Array.isArray(atmo?.fallbackSkyRadiance) ? atmo.fallbackSkyRadiance : [0.16, 0.23, 0.36];
    for (let Y = 0; Y < H; ++Y) {
      const [Dx, Dy, Dz] = ReconstructSkyViewDirection(0.5, Y / (H - 1), Math.PI * 0.5);
      const Up = Clamp(Dz * 0.5 + 0.5, 0.0, 1.0);
      const Scale = 0.65 + 0.70 * Up;
      for (let X = 0; X < W; ++X) {
        const Idx = (Y * W + X) * 4;
        data[Idx] = Math.max(0.0, Base[0] * Scale);
        data[Idx + 1] = Math.max(0.0, Base[1] * Scale);
        data[Idx + 2] = Math.max(0.0, Base[2] * Scale);
        data[Idx + 3] = 1.0;
      }
    }
    SkyViewState.Data = data;
    SkyViewState.LimbAngle = ComputeLimbPolarAngle(StartRadiusKm, GroundKm);
    SkyViewState.StartRadiusKm = StartRadiusKm;
    SkyViewState.BottomKm = GroundKm;
    SkyViewState.TopKm = TopKm;
    return data;
  }

  const LimbAngle = ComputeLimbPolarAngle(StartRadiusKm, GroundKm);
  const SunElevRad = Math.asin(ClampUnit(UnitSun[2]));
  const DayVis = Clamp((SunElevRad * RAD_TO_DEG + 6.0) / 14.0, 0.0, 1.0);
  const SolarScale = Math.max(0.0, Medium.SolarIntensityMultiplier) * 0.24;
  const G2 = MieG * MieG;
  const SkyTint = Medium.SkyTint;
  const SkyBrightness = Medium.SkyBrightnessMultiplier;
  const SunT = [0, 0, 0];
  const MultiSample = [0, 0, 0];

  for (let Y = 0; Y < H; ++Y) {
    const LowerBank = Y >= HALF_H;
    const LocalV = LowerBank
      ? (Y - HALF_H) / (HALF_H - 1)
      : Y / (HALF_H - 1);
    const PolarAngle = LowerBank
      ? Math.min(PI_F - 1.0e-5, LimbAngle + (PI_F - LimbAngle) * LocalV * LocalV + 1.5e-4)
      : Math.max(0.0, LimbAngle * (1.0 - LocalV * LocalV) - 1.5e-4);
    const CosPolar = Math.cos(PolarAngle);
    const SinPolar = Math.sqrt(Math.max(0.0, 1.0 - CosPolar * CosPolar));
    const LimbFactor = Math.exp(-Math.max(0.0, CosPolar) * 7.5);
    const GlowScale = 1.0 + (Medium.LimbGlowIntensity - 1.0) * LimbFactor * 0.55;

    const GroundHit = LowerBank
      ? IntersectLowerSphereDistanceKm(StartRadiusKm, CosPolar, GroundKm)
      : { Hits: false, DistanceKm: 0.0 };
    const MaxDistKm =
      LowerBank && GroundHit.Hits
        ? Math.max(0.005, GroundHit.DistanceKm)
        : IntersectUpperSphereDistanceKm(StartRadiusKm, CosPolar, TopKm);

    const Steps = LowerBank ? GND_STEPS : SKY_STEPS;
    const RowBase = Y * W * 4;

    for (let X = 0; X < W; ++X) {
      const U = (X + 0.5) / W;
      const Azimuth = (U - 0.5) * TAU;
      const RdX = SinPolar * Math.cos(Azimuth);
      const RdY = SinPolar * Math.sin(Azimuth);
      const RdZ = CosPolar;

      const CosTheta = RdX * UnitSun[0] + RdY * UnitSun[1] + RdZ * UnitSun[2];
      const PhaseR = (3.0 / (16.0 * PI_F)) * (1.0 + CosTheta * CosTheta);
      const Denom = Math.max(1.0e-3, 1.0 + G2 - 2.0 * MieG * CosTheta);
      const PhaseM =
        (3.0 / (8.0 * PI_F)) *
        (((1.0 - G2) * (1.0 + CosTheta * CosTheta)) / ((2.0 + G2) * Math.pow(Denom, 1.5)));

      let TR = 1.0;
      let TG = 1.0;
      let TB = 1.0;
      let RadR = 0.0;
      let RadG = 0.0;
      let RadB = 0.0;

      for (let S = 0; S < Steps; ++S) {
        const F0 = S / Steps;
        const F1 = (S + 1) / Steps;
        const T0 = MaxDistKm * F0 * F0;
        const T1 = MaxDistKm * F1 * F1;
        const T = 0.5 * (T0 + T1);
        const Step = Math.max(1.0e-5, T1 - T0);

        const RK = Math.sqrt(
          Math.max(
            GroundKm * GroundKm,
            StartRadiusKm * StartRadiusKm + 2.0 * StartRadiusKm * RdZ * T + T * T,
          ),
        );
        const AltKm = Math.max(0.0, RK - GroundKm);
        const C = EvaluateMediumInto(AltKm, Medium, MediumScratch);

        const SafeER = Math.max(1.0e-7, C.ExtinctionR);
        const SafeEG = Math.max(1.0e-7, C.ExtinctionG);
        const SafeEB = Math.max(1.0e-7, C.ExtinctionB);
        const SegTR = Math.exp(-SafeER * Step);
        const SegTG = Math.exp(-SafeEG * Step);
        const SegTB = Math.exp(-SafeEB * Step);
        const IntR = (1.0 - SegTR) / SafeER;
        const IntG = (1.0 - SegTG) / SafeEG;
        const IntB = (1.0 - SegTB) / SafeEB;

        const SampleSunCos = ClampUnit((StartRadiusKm * UnitSun[2] + T * CosTheta) / RK);
        SampleTransmittanceSphericalLut(Trans, RK, SampleSunCos, GroundKm, TopKm, SunT);
        SampleMultiScatterGridLut(Multi, AltKm, SampleSunCos, ThicknessKm, MultiSample);

        const ScR = C.RayScatterR * PhaseR + C.MieScatter * PhaseM;
        const ScG = C.RayScatterG * PhaseR + C.MieScatter * PhaseM;
        const ScB = C.RayScatterB * PhaseR + C.MieScatter * PhaseM;
        const MsR = C.RayScatterR + C.MieScatter;
        const MsG = C.RayScatterG + C.MieScatter;
        const MsB = C.RayScatterB + C.MieScatter;

        RadR += TR * (ScR * SunT[0] + MsR * MultiSample[0]) * IntR;
        RadG += TG * (ScG * SunT[1] + MsG * MultiSample[1]) * IntG;
        RadB += TB * (ScB * SunT[2] + MsB * MultiSample[2]) * IntB;

        TR *= SegTR;
        TG *= SegTG;
        TB *= SegTB;
      }

      if (LowerBank && GroundHit.Hits) {
        const HitSunCos = ClampUnit((StartRadiusKm * UnitSun[2] + MaxDistKm * CosTheta) / GroundKm);
        if (HitSunCos > 0.0) {
          SampleTransmittanceSphericalLut(Trans, GroundKm, HitSunCos, GroundKm, TopKm, SunT);
        } else {
          SunT[0] = 0.0;
          SunT[1] = 0.0;
          SunT[2] = 0.0;
        }
        SampleMultiScatterGridLut(Multi, 0.0, HitSunCos, ThicknessKm, MultiSample);
        const SunTerm = Math.max(0.0, HitSunCos) / PI_F;
        RadR += TR * Medium.GroundAlbedo[0] * (SunT[0] * SunTerm + MultiSample[0] * 0.70);
        RadG += TG * Medium.GroundAlbedo[1] * (SunT[1] * SunTerm + MultiSample[1] * 0.70);
        RadB += TB * Medium.GroundAlbedo[2] * (SunT[2] * SunTerm + MultiSample[2] * 0.70);
      }

      const NightR = 0.00018 + 0.00022 * LimbFactor;
      const NightG = 0.00028 + 0.00032 * LimbFactor;
      const NightB = 0.00062 + 0.00058 * LimbFactor;

      let OutR = (RadR * SolarScale * GlowScale + NightR * (1.0 - DayVis * 0.9)) * SkyTint[0] * SkyBrightness;
      let OutG = (RadG * SolarScale * GlowScale + NightG * (1.0 - DayVis * 0.9)) * SkyTint[1] * SkyBrightness;
      let OutB = (RadB * SolarScale * GlowScale + NightB * (1.0 - DayVis * 0.9)) * SkyTint[2] * SkyBrightness;

      if (LowerBank) {
        // True grazing Schlick Fresnel reflection near the lower limb blended cleanly against upper-limb row 0
        const LimbIdx = RowBase + X * 4;
        const Grazing = Math.pow(1.0 - Clamp(-CosPolar, 0.0, 1.0), 5.0);
        const Fresnel = 0.02 + 0.98 * Grazing;
        OutR = OutR * (1.0 - Fresnel) + data[LimbIdx] * Fresnel;
        OutG = OutG * (1.0 - Fresnel) + data[LimbIdx + 1] * Fresnel;
        OutB = OutB * (1.0 - Fresnel) + data[LimbIdx + 2] * Fresnel;
      }

      const Idx = RowBase + X * 4;
      data[Idx] = Math.max(0.0, OutR);
      data[Idx + 1] = Math.max(0.0, OutG);
      data[Idx + 2] = Math.max(0.0, OutB);
      data[Idx + 3] = 1.0;
    }
  }

  SkyViewState.Data = data;
  SkyViewState.LimbAngle = LimbAngle;
  SkyViewState.StartRadiusKm = StartRadiusKm;
  SkyViewState.BottomKm = GroundKm;
  SkyViewState.TopKm = TopKm;
  return data;
}

//==========================================================================================================================================
//                                        SampleSkyViewDirection (bank split + Catmull-Rom, zero horizon bleed)
//==========================================================================================================================================

export function SampleSkyViewLutCpu(lut, dx, dy, dz, LimbAngle = SkyViewState.LimbAngle) {
  if (!lut || lut.length < SKY_VIEW_WIDTH * SKY_VIEW_HEIGHT * 4) return [0.35, 0.45, 0.65];
  const W = SKY_VIEW_WIDTH;
  const Limb = Math.max(1.0e-5, LimbAngle || 0.0);

  const Azimuth = Math.atan2(dy, dx);
  const PolarAngle = Math.acos(ClampUnit(dz));

  let U = Azimuth / TAU + 0.5;
  U -= Math.floor(U);

  const LowerBank = PolarAngle > Limb;
  let CoordY;
  if (!LowerBank) {
    const Q = Math.sqrt(Clamp(1.0 - PolarAngle / Math.max(1.0e-4, Limb), 0.0, 1.0));
    CoordY = Q * (HALF_H - 1);
  } else {
    const Q = Math.sqrt(Clamp((PolarAngle - Limb) / Math.max(1.0e-4, PI_F - Limb), 0.0, 1.0));
    CoordY = HALF_H + Q * (HALF_H - 1);
  }

  const BankMin = LowerBank ? HALF_H : 0;
  const BankMax = LowerBank ? HALF_H * 2 - 1 : HALF_H - 1;

  const FX = U * W - 0.5;
  const FloorX = Math.floor(FX);
  const IX0 = ((FloorX % W) + W) % W;
  const IX1 = (IX0 + 1) % W;
  const TX = FX - FloorX;
  const SX = TX * TX * (3.0 - 2.0 * TX);

  const IY1 = Clamp(Math.floor(CoordY), BankMin, BankMax);
  const IY0 = Clamp(IY1 - 1, BankMin, BankMax);
  const IY2 = Clamp(IY1 + 1, BankMin, BankMax);
  const IY3 = Clamp(IY1 + 2, BankMin, BankMax);
  const TY = Clamp(CoordY - IY1, 0.0, 1.0);

  const Fetch = (X, Y) => (Y * W + X) * 4;
  const MixC = (A, B, T) => A + (B - A) * T;
  const CatmullRom = (P0, P1, P2, P3, T) => {
    const T2 = T * T;
    const T3 = T2 * T;
    return 0.5 * ((2.0 * P1) + (-P0 + P2) * T + (2.0 * P0 - 5.0 * P1 + 4.0 * P2 - P3) * T2 +
      (-P0 + 3.0 * P1 - 3.0 * P2 + P3) * T3);
  };

  const Out = [0, 0, 0];
  for (let C = 0; C < 3; ++C) {
    const R0 = MixC(lut[Fetch(IX0, IY0) + C], lut[Fetch(IX1, IY0) + C], SX);
    const R1 = MixC(lut[Fetch(IX0, IY1) + C], lut[Fetch(IX1, IY1) + C], SX);
    const R2 = MixC(lut[Fetch(IX0, IY2) + C], lut[Fetch(IX1, IY2) + C], SX);
    const R3 = MixC(lut[Fetch(IX0, IY3) + C], lut[Fetch(IX1, IY3) + C], SX);
    Out[C] = Math.max(0.0, CatmullRom(R0, R1, R2, R3, TY));
  }
  return Out;
}

export function SampleSkyViewDirection(dx, dy, dz) {
  return SampleSkyViewLutCpu(SkyViewState.Data, dx, dy, dz, SkyViewState.LimbAngle);
}

export function GetSkyViewCacheKey(sunDir, atmo = {}) {
  const M = ResolveAtmosphericMedium(atmo);
  return [
    sunDir[0].toFixed(4),
    sunDir[1].toFixed(4),
    sunDir[2].toFixed(4),
    M.RayleighStrength.toFixed(3),
    M.MieStrength.toFixed(3),
    M.MieAnisotropy.toFixed(3),
    M.OzoneStrength.toFixed(3),
    Math.round(M.RayleighScaleHeightMeters),
    Math.round(M.MieScaleHeightMeters),
    Math.round(M.AtmosphereThicknessMeters),
    Math.round(M.PlanetRadiusMeters),
    M.LimbGlowIntensity.toFixed(2),
    M.SkyBrightnessMultiplier.toFixed(2),
    M.SolarIntensityMultiplier.toFixed(2),
    Math.round(M.ObserverAltitudeMeters),
    M.SkyTint.map((c) => c.toFixed(2)).join(","),
    M.GroundAlbedo.map((c) => c.toFixed(3)).join(","),
    M.AtmosphereEnabled ? "1" : "0",
  ].join("|");
}

//==========================================================================================================================================
//                                        Solar photosphere: erf PSF, Neckel & Labs limb darkening, K/F corona, glare
//==========================================================================================================================================

export function EvaluateSolarErf(Argument) {
  const Variable = 1.0 / (1.0 + 0.3275911 * Math.abs(Argument));
  const Polynomial =
    ((((1.061405429 * Variable - 1.453152027) * Variable + 1.421413741) * Variable -
      0.284496736) *
      Variable +
      0.254829592) *
    Variable;
  const Result = 1.0 - Polynomial * Math.exp(-Argument * Argument);
  return Argument < 0.0 ? -Result : Result;
}

export function EvaluateNeckelLabsLimbDarkening(MuPhotosphere) {
  const Mu = Clamp(MuPhotosphere, 0.0, 1.0);
  const Mu2 = Mu * Mu;
  const Mu3 = Mu2 * Mu;
  const Mu4 = Mu2 * Mu2;
  const Mu5 = Mu4 * Mu;

  // Exact published Neckel & Labs (1994, Solar Physics 153) polynomial coefficients at 650/550/450 nm
  const LimbRed = 0.34685 + 1.37539 * Mu - 2.04425 * Mu2 + 2.70493 * Mu3 - 1.9429 * Mu4 + 0.55999 * Mu5;
  const LimbGreen = 0.23698 + 1.37916 * Mu - 1.83226 * Mu2 + 2.3454 * Mu3 - 1.65319 * Mu4 + 0.52391 * Mu5;
  const LimbBlue = 0.11903 + 1.24089 * Mu - 1.19977 * Mu2 + 1.33936 * Mu3 - 0.9869 * Mu4 + 0.48739 * Mu5;

  return [
    Clamp(LimbRed, 0.0, 1.05),
    Clamp(LimbGreen, 0.0, 1.05),
    Clamp(LimbBlue, 0.0, 1.05),
  ];
}

export function EvaluateSolarGlareSpread(SeparationRadians, DiscRadiusRadians) {
  const SolarGlareReachRadians = 30.0 * DEG_TO_RAD;
  const SolarGlareBloomWeight = 1.5;
  const SolarGlareAureoleWeight = 0.25;
  const SolarGlareAureoleDegrees = 6.0;

  if (SeparationRadians >= SolarGlareReachRadians) return 0.0;

  const SepDegrees = SeparationRadians * RAD_TO_DEG;
  const DiscDegrees = DiscRadiusRadians * RAD_TO_DEG;
  const Effective = Math.sqrt(SepDegrees * SepDegrees + DiscDegrees * DiscDegrees);
  const Window = 1.0 - SeparationRadians / SolarGlareReachRadians;
  const Bloom = SolarGlareBloomWeight / ((Effective + 0.02) * (Effective + 0.02));
  const AureoleRatio = Effective / SolarGlareAureoleDegrees;
  const Aureole = SolarGlareAureoleWeight / (1.0 + AureoleRatio * AureoleRatio);
  return (Bloom + Aureole) * Window * Window;
}

export const DEFAULT_SOLAR_CONFIG = {
  SolarEnabled: true,
  AngularDiameterDegrees: 0.533,
  SolarIntensityMultiplier: 22.0,
  AppearanceMode: 1, // 0 = Muted, 1 = Radiant
  SunGlareStrength: 0.25,
};

export function EvaluateSolarCoronaRadiance(
  RayDirection,
  SunDirection,
  SolarConfig = DEFAULT_SOLAR_CONFIG,
  SolarTransmittance = [1, 1, 1],
  EffectiveSolarTint = [1, 1, 1],
  AllowBelowHorizon = false,
  Medium = null,
  PixelAngularSizeRad = 0.0,
  ForegroundOcclusion = [1, 1, 1],
  GlareEclipseVis = 1.0,
) {
  const Cfg = { ...DEFAULT_SOLAR_CONFIG, ...(SolarConfig || {}) };
  if (!Cfg.SolarEnabled || Cfg.SolarIntensityMultiplier <= 0.0) return [0.0, 0.0, 0.0];

  const RayMag = Math.hypot(RayDirection[0], RayDirection[1], RayDirection[2]) || 1.0;
  const UnitRay = [RayDirection[0] / RayMag, RayDirection[1] / RayMag, RayDirection[2] / RayMag];
  const SunMag = Math.hypot(SunDirection[0], SunDirection[1], SunDirection[2]) || 1.0;
  const UnitSun = [SunDirection[0] / SunMag, SunDirection[1] / SunMag, SunDirection[2] / SunMag];

  const HalfChord = 0.5 * Math.hypot(UnitRay[0] - UnitSun[0], UnitRay[1] - UnitSun[1], UnitRay[2] - UnitSun[2]);
  const Separation = 2.0 * Math.asin(Clamp(HalfChord, 0.0, 1.0));
  const HalfRad = Math.max(0.0008, Cfg.AngularDiameterDegrees * 0.5 * DEG_TO_RAD);

  const SkyBrightness = Medium ? Math.max(0.0, Medium.SkyBrightnessMultiplier ?? 1.0) : 1.0;
  const DiscIntensity = Math.max(0.0, Cfg.SolarIntensityMultiplier) * SkyBrightness * 2.1;
  if (DiscIntensity <= 0.0) return [0.0, 0.0, 0.0];

  const OrbitalView =
    AllowBelowHorizon ||
    (Medium !== null &&
      (Medium.OrbitalViewEnabled ||
        Medium.ObserverAltitudeMeters > Medium.AtmosphereThicknessMeters));
  const PlanetR = Medium ? Math.max(500000.0, Medium.PlanetRadiusMeters) : 6360000.0;
  const ObsAlt = Medium ? Math.max(5.0, Medium.ObserverAltitudeMeters) : 100.0;
  const ViewerR = PlanetR + ObsAlt;
  const Ratio = Clamp(PlanetR / ViewerR, 0.0, 1.0);
  const LimbZ = -Math.sqrt(Math.max(0.0, 1.0 - Ratio * Ratio));
  const SmoothStepSky = (E0, E1, X) => {
    const T = Clamp((X - E0) / Math.max(1.0e-6, E1 - E0), 0.0, 1.0);
    return T * T * (3.0 - 2.0 * T);
  };

  const PixelSizeRad = Math.max(0.0, PixelAngularSizeRad);
  const MaxDiscCoronaR = Math.max(HalfRad * 48.0, PixelSizeRad * 8.0);
  const HorizonBand = Math.max(0.00035, 0.65 * Math.max(PixelSizeRad, 1.0e-4));
  const HorizonVis = OrbitalView ? 1.0 : SmoothStepSky(LimbZ - HorizonBand, LimbZ + HorizonBand, UnitRay[2]);

  const Out = [0.0, 0.0, 0.0];
  const OcclusionSum = ForegroundOcclusion[0] + ForegroundOcclusion[1] + ForegroundOcclusion[2];
  if (Separation <= MaxDiscCoronaR && HorizonVis > 1.0e-5 && OcclusionSum > 1.0e-5) {
    const LineTransmittance =
      !OrbitalView && Medium
        ? ComputeSolarTransmittance(
            [UnitRay[0], UnitRay[1], Math.max(UnitRay[2], LimbZ + 1.0e-4)],
            Medium,
            ObsAlt,
          )
        : SolarTransmittance;

    if (LineTransmittance[0] + LineTransmittance[1] + LineTransmittance[2] > 1.0e-6) {
      const NormalizedRadius = Separation / HalfRad;

      let SmallWeight = 0.0;
      if (PixelSizeRad > 1.0e-6) {
        const RadiusPixels = HalfRad / PixelSizeRad;
        const Blend = Clamp((6.0 - RadiusPixels) / 3.0, 0.0, 1.0);
        SmallWeight = Blend * Blend * (3.0 - 2.0 * Blend);
      }

      let ResolvedR = 0.0;
      let ResolvedG = 0.0;
      let ResolvedB = 0.0;
      if (SmallWeight < 1.0 && NormalizedRadius < 1.32) {
        const ClampedR = Math.min(1.0, NormalizedRadius);
        const MuSun = Math.sqrt(Math.max(0.0, 1.0 - ClampedR * ClampedR));
        const LimbDark = EvaluateNeckelLabsLimbDarkening(MuSun);
        const EdgeErfc = 0.5 * (1.0 - EvaluateSolarErf((NormalizedRadius - 0.997) / 0.012));
        const PhotoMag = EdgeErfc * DiscIntensity;
        ResolvedR = LimbDark[0] * PhotoMag;
        ResolvedG = LimbDark[1] * PhotoMag;
        ResolvedB = LimbDark[2] * PhotoMag;
      }

      let SubR = 0.0;
      let SubG = 0.0;
      let SubB = 0.0;
      const SunDotRay = UnitRay[0] * UnitSun[0] + UnitRay[1] * UnitSun[1] + UnitRay[2] * UnitSun[2];
      if (SmallWeight > 0.0 && SunDotRay > 0.0) {
        const RefAxis = Math.abs(UnitSun[2]) < 0.98 ? [0.0, 0.0, 1.0] : [1.0, 0.0, 0.0];
        let TanT = [
          RefAxis[1] * UnitSun[2] - RefAxis[2] * UnitSun[1],
          RefAxis[2] * UnitSun[0] - RefAxis[0] * UnitSun[2],
          RefAxis[0] * UnitSun[1] - RefAxis[1] * UnitSun[0],
        ];
        let TanTLen = Math.hypot(TanT[0], TanT[1], TanT[2]) || 1.0;
        TanT = [TanT[0] / TanTLen, TanT[1] / TanTLen, TanT[2] / TanTLen];
        let TanB = [
          UnitSun[1] * TanT[2] - UnitSun[2] * TanT[1],
          UnitSun[2] * TanT[0] - UnitSun[0] * TanT[2],
          UnitSun[0] * TanT[1] - UnitSun[1] * TanT[0],
        ];
        const TanBLen = Math.hypot(TanB[0], TanB[1], TanB[2]) || 1.0;
        TanB = [TanB[0] / TanBLen, TanB[1] / TanBLen, TanB[2] / TanBLen];

        const DeltaX = UnitRay[0] * TanT[0] + UnitRay[1] * TanT[1] + UnitRay[2] * TanT[2];
        const DeltaY = UnitRay[0] * TanB[0] + UnitRay[1] * TanB[1] + UnitRay[2] * TanB[2];
        const SmallSigma = Math.sqrt(0.25 * HalfRad * HalfRad + 0.2 * PixelSizeRad * PixelSizeRad);
        const InvSqrt2Sigma = 1.0 / (1.4142135623730951 * Math.max(1.0e-6, SmallSigma));
        const LowX = (DeltaX - 0.5 * PixelSizeRad) * InvSqrt2Sigma;
        const HighX = (DeltaX + 0.5 * PixelSizeRad) * InvSqrt2Sigma;
        const LowY = (DeltaY - 0.5 * PixelSizeRad) * InvSqrt2Sigma;
        const HighY = (DeltaY + 0.5 * PixelSizeRad) * InvSqrt2Sigma;
        const XMass = Math.max(0.0, 0.5 * (EvaluateSolarErf(HighX) - EvaluateSolarErf(LowX)));
        const YMass = Math.max(0.0, 0.5 * (EvaluateSolarErf(HighY) - EvaluateSolarErf(LowY)));
        const SinHalf = Math.sin(0.5 * HalfRad);
        const SolidAngle = 4.0 * PI_F * SinHalf * SinHalf;
        const PixelArea = Math.max(1.0e-12, PixelSizeRad * PixelSizeRad);
        const SubGain = (SolidAngle / PixelArea) * XMass * YMass * DiscIntensity;
        SubR = 0.83276 * SubGain;
        SubG = 0.77447 * SubGain;
        SubB = 0.69061 * SubGain;
      }

      const PhotoR = ResolvedR + (SubR - ResolvedR) * SmallWeight;
      const PhotoG = ResolvedG + (SubG - ResolvedG) * SmallWeight;
      const PhotoB = ResolvedB + (SubB - ResolvedB) * SmallWeight;

      let CoronaR = 0.0;
      let CoronaG = 0.0;
      let CoronaB = 0.0;
      const CoronaBlurAngle = 0.3 * PixelSizeRad * SmallWeight;
      const CoronaNormR =
        Math.sqrt(Separation * Separation + CoronaBlurAngle * CoronaBlurAngle) / HalfRad;
      if (CoronaNormR < 48.0) {
        const R1 = CoronaNormR / 1.18;
        const R2 = CoronaNormR / 3.4;
        const R3 = CoronaNormR / 11.5;
        const InnerTerm = 1.0 / Math.pow(1.0 + R1 * R1, 1.85);
        const MiddleTerm = 1.0 / Math.pow(1.0 + R2 * R2, 1.38);
        const OuterTerm = 1.0 / Math.pow(1.0 + R3 * R3, 1.22);

        const ScatterFactor = OrbitalView
          ? 0.38
          : Clamp(0.55 + 0.85 * (1.0 - LineTransmittance[2]), 0.55, 1.35);
        const CoronaProfile =
          DiscIntensity *
          (0.042 * InnerTerm + 0.0085 * ScatterFactor * MiddleTerm + 0.0014 * ScatterFactor * OuterTerm);
        const WindowFade = Clamp(1.0 - CoronaNormR / 48.0, 0.0, 1.0);
        const SmoothWindow = WindowFade * WindowFade;
        CoronaR = CoronaProfile * SmoothWindow * 1.04;
        CoronaG = CoronaProfile * SmoothWindow * 0.99;
        CoronaB = CoronaProfile * SmoothWindow * 0.91;
      }

      Out[0] += ForegroundOcclusion[0] * (PhotoR + CoronaR) * LineTransmittance[0] * EffectiveSolarTint[0] * HorizonVis;
      Out[1] += ForegroundOcclusion[1] * (PhotoG + CoronaG) * LineTransmittance[1] * EffectiveSolarTint[1] * HorizonVis;
      Out[2] += ForegroundOcclusion[2] * (PhotoB + CoronaB) * LineTransmittance[2] * EffectiveSolarTint[2] * HorizonVis;
    }
  }

  // Optical glare veil (Radiant vs Muted): bounded 30 deg spread tinted by solar-axis transmittance + eclipse visibility
  if (Cfg.AppearanceMode === 1 && Cfg.SunGlareStrength > 0.0 && GlareEclipseVis > 1.0e-4) {
    const Spread = EvaluateSolarGlareSpread(Separation, HalfRad);
    if (Spread > 0.0) {
      const SunAxisHorizonVis = OrbitalView
        ? 1.0
        : SmoothStepSky(LimbZ - Math.max(0.001, HalfRad), LimbZ + Math.max(0.001, HalfRad), UnitSun[2]);
      if (SunAxisHorizonVis > 1.0e-4) {
        const GlareScale =
          Math.max(0.0, Cfg.SunGlareStrength) * Spread * (0.045 * DiscIntensity) * SunAxisHorizonVis * GlareEclipseVis;
        Out[0] += EffectiveSolarTint[0] * SolarTransmittance[0] * GlareScale * 1.02;
        Out[1] += EffectiveSolarTint[1] * SolarTransmittance[1] * GlareScale * 1.0;
        Out[2] += EffectiveSolarTint[2] * SolarTransmittance[2] * GlareScale * 0.96;
      }
    }
  }

  return Out;
}

// CPU 3D Value-Noise FBM for volumetric cloud parity in CpuGtxFallbackRenderer
function Hash3(ix, iy, iz) {
  let n = (ix * 374761393 + iy * 668265263 + iz * 2147483647) | 0;
  n = (n ^ (n >>> 13)) * 1274126177;
  return ((n ^ (n >>> 16)) >>> 0) / 4294967296.0;
}

function Noise3(x, y, z) {
  const ix = Math.floor(x), iy = Math.floor(y), iz = Math.floor(z);
  const fx = x - ix, fy = y - iy, fz = z - iz;
  const ux = fx * fx * (3.0 - 2.0 * fx);
  const uy = fy * fy * (3.0 - 2.0 * fy);
  const uz = fz * fz * (3.0 - 2.0 * fz);
  const lerp = (a, b, t) => a + (b - a) * t;
  const n000 = Hash3(ix, iy, iz), n100 = Hash3(ix + 1, iy, iz);
  const n010 = Hash3(ix, iy + 1, iz), n110 = Hash3(ix + 1, iy + 1, iz);
  const n001 = Hash3(ix, iy, iz + 1), n101 = Hash3(ix + 1, iy, iz + 1);
  const n011 = Hash3(ix, iy + 1, iz + 1), n111 = Hash3(ix + 1, iy + 1, iz + 1);
  return lerp(
    lerp(lerp(n000, n100, ux), lerp(n010, n110, ux), uy),
    lerp(lerp(n001, n101, ux), lerp(n011, n111, ux), uy),
    uz,
  );
}

function CloudFbm3(x, y, z) {
  let sum = 0.0, amp = 0.52, freq = 1.0;
  for (let o = 0; o < 4; ++o) {
    sum += amp * Noise3(x * freq, y * freq, z * freq);
    freq *= 2.07;
    amp *= 0.48;
  }
  return sum;
}

export function EvalVolumetricCloudsCpu(dx, dy, dz, sunDir, sunIrr, skyCol, clouds, timeSec = 0.0) {
  if (!clouds?.enabled || dz <= 0.008 || clouds.coverage <= 0.01 || clouds.density <= 0.01) {
    return { rad: [0, 0, 0], trans: 1.0 };
  }
  const baseM = Math.max(200.0, clouds.baseM ?? 1400.0);
  const thickM = Math.max(150.0, clouds.thicknessM ?? 1100.0);
  const rInner = EARTH_RADIUS_M + baseM;
  const rOuter = EARTH_RADIUS_M + baseM + thickM;
  const origZ = EARTH_RADIUS_M + OBSERVER_ALT_M;

  const [, tEnter] = IntersectSphereDist(origZ, dz, rInner);
  const [, tExit] = IntersectSphereDist(origZ, dz, rOuter);
  const maxDist = Math.min(tExit, 48000.0);
  if (maxDist <= tEnter || tEnter < 0.0) return { rad: [0, 0, 0], trans: 1.0 };

  const steps = 16;
  const dt = (maxDist - tEnter) / steps;
  const fScale = 0.00065 / Math.max(0.08, clouds.featureScale ?? 0.35);
  const wx = (clouds.windX ?? 6.0) * timeSec;
  const wy = (clouds.windY ?? 2.0) * timeSec;
  const cov = Clamp(clouds.coverage ?? 0.34, 0.0, 1.0);
  const densMul = Math.max(0.0, clouds.density ?? 2.4) * 0.008;
  const anvil = Clamp(clouds.anvil ?? 0.5, 0.0, 1.0);
  const g = Clamp(clouds.anisotropy ?? 0.45, -0.5, 0.9);
  const cosTheta = dx * sunDir[0] + dy * sunDir[1] + dz * sunDir[2];
  const hg = (gg) => (1.0 - gg * gg) / (4.0 * Math.PI * Math.pow(Math.max(1e-3, 1.0 + gg * gg - 2.0 * gg * cosTheta), 1.5));
  const phase = 0.78 * hg(g) + 0.22 * hg(-0.25);

  const sampleDensity = (px, py, pz) => {
    const alt = Math.hypot(px, py, pz) - EARTH_RADIUS_M;
    const h = Clamp((alt - baseM) / thickM, 0.0, 1.0);
    const bot = Clamp(h / 0.16, 0.0, 1.0);
    const top = Clamp((1.0 - h) / (0.28 - 0.16 * anvil), 0.0, 1.0);
    const profile = bot * top;
    if (profile <= 0.001) return 0.0;
    const shapeCov = Clamp(cov + anvil * h * h * 0.25, 0.0, 1.0);
    const n = CloudFbm3((px + wx) * fScale, (py + wy) * fScale, (alt - baseM) * fScale * 1.4);
    const nNorm = Clamp((n - 0.28) / 0.44, 0.0, 1.0);
    const thr = 1.0 - shapeCov;
    return Math.max(0.0, (nNorm - thr) / Math.max(0.18, shapeCov)) * profile * densMul;
  };

  let trans = 1.0;
  let rR = 0.0, rG = 0.0, rB = 0.0;
  for (let i = 0; i < steps; ++i) {
    const t = tEnter + (i + 0.5) * dt;
    const px = dx * t, py = dy * t, pz = origZ + dz * t;
    const d = sampleDensity(px, py, pz);
    if (d > 1e-6) {
      let sunTau = 0.0;
      if (clouds.shadowEnabled !== false) {
        const sStep = (thickM * 0.55) / 3.0;
        for (let s = 1; s <= 3; ++s) {
          sunTau += sampleDensity(px + sunDir[0] * s * sStep, py + sunDir[1] * s * sStep, pz + sunDir[2] * s * sStep) * sStep;
        }
      }
      const sunAtt = Math.exp(-sunTau * 1.15) * (0.35 + 0.65 * (1.0 - Math.exp(-sunTau * 2.2 + 0.25)));
      const segT = Math.exp(-d * dt);
      const scatter = (1.0 - segT);
      const alt = Math.hypot(px, py, pz) - EARTH_RADIUS_M;
      const h = Clamp((alt - baseM) / thickM, 0.0, 1.0);
      const amb = 0.28 + 0.52 * h;
      const litR = sunIrr[0] * sunAtt * phase * 2.4 + skyCol[0] * amb * 0.55;
      const litG = sunIrr[1] * sunAtt * phase * 2.4 + skyCol[1] * amb * 0.55;
      const litB = sunIrr[2] * sunAtt * phase * 2.4 + skyCol[2] * amb * 0.55;
      rR += trans * litR * scatter;
      rG += trans * litG * scatter;
      rB += trans * litB * scatter;
      trans *= segT;
      if (trans < 0.015) break;
    }
  }
  const haze = 1.0 - Math.exp(-tEnter * 0.00004);
  rR = rR * (1.0 - haze) + skyCol[0] * (1.0 - trans) * haze;
  rG = rG * (1.0 - haze) + skyCol[1] * (1.0 - trans) * haze;
  rB = rB * (1.0 - haze) + skyCol[2] * (1.0 - trans) * haze;
  return { rad: [rR, rG, rB], trans };
}

export function EvalMoonAndStarsCpu(dx, dy, dz, sunDir, moon, stars, timeSec = 0.0) {
  let outR = 0.0, outG = 0.0, outB = 0.0;
  // Moon disc with 3D spherical phase & mare/crater detail
  if (moon?.visible && moon.dir && dz > -0.05) {
    const mx = moon.dir[0], my = moon.dir[1], mz = moon.dir[2];
    const cosM = dx * mx + dy * my + dz * mz;
    const angR = moon.sizeRad ?? 0.012;
    const cosEdge = Math.cos(angR);
    const glow = Math.pow(Math.max(0.0, cosM), 420.0) * (moon.glow ?? 0.8) * 0.28 * (moon.bright ?? 1.6);
    const tint = moon.tint || [0.94, 0.93, 0.90];
    outR += tint[0] * glow;
    outG += tint[1] * glow;
    outB += tint[2] * glow;

    if (cosM > cosEdge) {
      // Tangent basis on moon sphere
      const upRef = Math.abs(mz) < 0.98 ? [0, 0, 1] : [1, 0, 0];
      let tx = upRef[1] * mz - upRef[2] * my;
      let ty = upRef[2] * mx - upRef[0] * mz;
      let tz = upRef[0] * my - upRef[1] * mx;
      const tl = Math.hypot(tx, ty, tz) || 1.0;
      tx /= tl; ty /= tl; tz /= tl;
      const bx = my * tz - mz * ty, by = mz * tx - mx * tz, bz = mx * ty - my * tx;
      const px = (dx * tx + dy * ty + dz * tz) / Math.max(1e-4, angR);
      const py = (dx * bx + dy * by + dz * bz) / Math.max(1e-4, angR);
      const r2 = px * px + py * py;
      if (r2 < 1.0) {
        const pz = Math.sqrt(Math.max(0.0, 1.0 - r2));
        // Lunar phase angle from 0 (new) -> 0.5 (full) -> 1.0 (new)
        const phAng = ((moon.phase ?? 0.62) - 0.5) * Math.PI * 2.0;
        const lx = Math.sin(phAng), ly = 0.18, lz = Math.cos(phAng);
        const ll = Math.hypot(lx, ly, lz);
        const ndl = Math.max(0.0, (px * lx + py * ly + pz * lz) / ll);
        const mare = 0.68 + 0.32 * Noise3(px * 3.8 + 4.1, py * 3.8 - 2.3, pz * 3.8);
        const edgeAA = Clamp((1.0 - Math.sqrt(r2)) * 14.0, 0.0, 1.0);
        const lum = (0.015 + ndl * 1.45) * mare * (moon.bright ?? 1.6) * edgeAA;
        outR += tint[0] * lum;
        outG += tint[1] * lum;
        outB += tint[2] * lum;
      }
    }
  }

  // Deterministic procedural starfield
  if (stars?.enabled && dz > 0.005) {
    const dayFade = Clamp(1.0 - (sunDir[2] + 0.08) * 5.0, 0.0, 1.0);
    if (dayFade > 0.01) {
      const rot = stars.rotationRad ?? 0.0;
      const cr = Math.cos(rot), sr = Math.sin(rot);
      const rx = dx * cr - dy * sr;
      const ry = dx * sr + dy * cr;
      const rz = dz;
      const scale = 175.0;
      const gx = rx * scale, gy = ry * scale, gz = rz * scale;
      const ix = Math.floor(gx), iy = Math.floor(gy), iz = Math.floor(gz);
      const h1 = Hash3(ix, iy, iz);
      const h2 = Hash3(ix + 19, iy - 7, iz + 31);
      const h3 = Hash3(ix - 13, iy + 29, iz - 5);
      const magThresh = Clamp((stars.limMag ?? 5.5) / 8.5, 0.05, 0.98);
      if (h1 > 1.0 - 0.18 * magThresh) {
        const cx = ix + 0.2 + 0.6 * h2;
        const cy = iy + 0.2 + 0.6 * h3;
        const cz = iz + 0.2 + 0.6 * ((h1 * 7.13) % 1.0);
        const d2 = (gx - cx) * (gx - cx) + (gy - cy) * (gy - cy) + (gz - cz) * (gz - cz);
        const rad = 0.08 * Clamp(stars.pointSize ?? 1.0, 0.4, 3.0);
        if (d2 < rad * rad) {
          const falloff = Math.exp(-d2 / (rad * rad * 0.35));
          const tw = stars.twinkle
            ? 1.0 - ((stars.twinkleDepth ?? 30) * 0.008) * (0.5 + 0.5 * Math.sin(timeSec * (stars.twinkleRate ?? 1.2) * 5.0 + h2 * 40.0))
            : 1.0;
          const intensity = (0.45 + 1.55 * h3 * h3) * (stars.brightness ?? 1.0) * dayFade * tw * falloff * Clamp(dz * 4.0, 0.0, 1.0);
          const warm = h2;
          outR += intensity * (0.78 + 0.42 * warm);
          outG += intensity * (0.88 + 0.12 * warm);
          outB += intensity * (1.20 - 0.35 * warm);
        }
      }
    }
  }
  return [outR, outG, outB];
}

const _f32View = new Float32Array(1);
const _u32View = new Uint32Array(_f32View.buffer);

function FloatToHalf(val) {
  _f32View[0] = val;
  const x = _u32View[0];
  const sign = (x >>> 16) & 0x8000;
  let exp = ((x >>> 23) & 0xff) - 127 + 15;
  let mant = x & 0x7fffff;
  if (exp <= 0) {
    if (exp < -10) return sign;
    mant = (mant | 0x800000) >>> (1 - exp);
    return sign | ((mant + 0x1000) >>> 13);
  }
  if (exp >= 31) return sign | 0x7c00;
  return sign | (exp << 10) | ((mant + 0x1000) >>> 13);
}

export function PackSkyViewRgba16f(floatData, outU16 = null) {
  const len = floatData.length;
  const u16 = outU16 && outU16.length === len ? outU16 : new Uint16Array(len);
  for (let i = 0; i < len; ++i) {
    u16[i] = FloatToHalf(floatData[i]);
  }
  return u16;
}


