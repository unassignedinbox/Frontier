//==========================================================================================================================================
//                                                          SOLAR.MJS
//==========================================================================================================================================
// 1:1 JavaScript port of Engine/GeometricRaster/Atmosphere/CelestialEphemerisSolver.cpp
// (astronomical solar ephemeris, Kelvin Planckian tint, Right-Handed +Z Up sky direction mapping, day-cycle advance).

const DEG_TO_RAD = 0.01745329251994329577;
const RAD_TO_DEG = 57.2957795130823208768;

export const SolarTintSource = { RgbTint: 0, KelvinTemperature: 1 };
export const SolarAppearanceMode = { Muted: 0, Radiant: 1 };
export const DayCycleSpeedPreset = {
  Multiplier1: 0,
  Multiplier8: 1,
  Multiplier30: 2,
  Multiplier100: 3,
  Custom: 4,
};

// SolarCelestialConfiguration defaults (CelestialEphemerisSolver.h)
export const DEFAULT_SOLAR_CELESTIAL_CONFIGURATION = {
  SolarEnabled: true,
  AngularDiameterDegrees: 0.533,
  TintSource: SolarTintSource.RgbTint,
  ManualRgbTint: [1.0, 1.0, 1.0],
  ColorTemperatureKelvin: 6500.0,
  SolarIntensityMultiplier: 22.0,
  DirectGainMultiplier: 2.5,
  LocalHours: 15.5,
  CycleAnimationEnabled: false,
  CycleSpeedPreset: DayCycleSpeedPreset.Multiplier8,
  DayDurationHours: 3.0,
  SiteLatitudeDegrees: -26.19,
  SiteLongitudeDegrees: 28.32,
  DayOfMonth: 10,
  Month: 9,
  Year: 2026,
  UtcOffsetHours: 2.0,
  ManualDirectionOverride: false,
  ManualAzimuthDegrees: 295.0,
  ManualElevationDegrees: 38.0,
  AppearanceMode: SolarAppearanceMode.Radiant,
  SunGlareStrength: 0.25,
};

function WrapDegrees360(AngleDegrees) {
  const Mod = AngleDegrees % 360.0;
  return Mod < 0.0 ? Mod + 360.0 : Mod;
}

function SrgbChannelToLinear(Channel) {
  const C = Math.max(0.0, Math.min(1.0, Channel));
  return C <= 0.04045 ? C / 12.92 : Math.pow((C + 0.055) / 1.055, 2.4);
}

// Converts compass Azimuth (0=North/+X, 90=East/-Y, 180=South/-X, 270=West/+Y) & Elevation to Right-Handed +Z Up unit axis
export function ConvertAzimuthElevationToWorldAxis(AzimuthDegrees, ElevationDegrees) {
  const AzRad = AzimuthDegrees * DEG_TO_RAD;
  const ElRad = ElevationDegrees * DEG_TO_RAD;
  const CosEl = Math.cos(ElRad);
  const X = CosEl * Math.cos(AzRad);
  const Y = -CosEl * Math.sin(AzRad);
  const Z = Math.sin(ElRad);
  const Len = Math.sqrt(X * X + Y * Y + Z * Z) || 1.0;
  return [X / Len, Y / Len, Z / Len];
}

// Inverse of ConvertAzimuthElevationToWorldAxis (compass bearing / altitude of a world axis)
export function ConvertWorldAxisToAzimuthElevation(Direction) {
  const Len = Math.sqrt(
    Direction[0] * Direction[0] + Direction[1] * Direction[1] + Direction[2] * Direction[2],
  ) || 1.0;
  const Z = Math.max(-1.0, Math.min(1.0, Direction[2] / Len));
  const Azimuth = WrapDegrees360(Math.atan2(-Direction[1], Direction[0]) * RAD_TO_DEG);
  return [Azimuth, Math.asin(Z) * RAD_TO_DEG];
}

// Evaluates Planckian color temperature [1000..40000 K] in LinearRGB
export function EvaluateKelvinToLinearRgb(Kelvin) {
  const T = Math.max(1000.0, Math.min(40000.0, Kelvin)) * 0.01;
  let R = 255.0;
  let G = 255.0;
  let B = 255.0;

  if (T <= 66.0) {
    R = 255.0;
    G = Math.max(
      0.0,
      Math.min(255.0, 99.4708025861 * Math.log(Math.max(1.0, T)) - 161.1195681661),
    );
    B =
      T <= 19.0
        ? 0.0
        : Math.max(
            0.0,
            Math.min(255.0, 138.5177312231 * Math.log(Math.max(1.0, T - 10.0)) - 305.0447927307),
          );
  } else {
    R = Math.max(0.0, Math.min(255.0, 329.698727446 * Math.pow(T - 60.0, -0.1332047592)));
    G = Math.max(0.0, Math.min(255.0, 288.1221695283 * Math.pow(T - 60.0, -0.0755148492)));
    B = 255.0;
  }

  return [
    SrgbChannelToLinear(R / 255.0),
    SrgbChannelToLinear(G / 255.0),
    SrgbChannelToLinear(B / 255.0),
  ];
}

// Solves astronomical solar position, sidereal time, and effective tint without atmospheric refraction
export function SolveSolarEphemeris(Config = {}) {
  const Cfg = { ...DEFAULT_SOLAR_CELESTIAL_CONFIGURATION, ...(Config || {}) };
  const Outcome = {
    ElevationDegrees: 38.0,
    AzimuthDegrees: 295.0,
    DeclinationDegrees: 0.0,
    EquationOfTimeMinutes: 0.0,
    SiderealHours: 0.0,
    WorldDirectionToSun: [0.42, -0.55, 0.72],
    EffectiveSolarTint: [1.0, 1.0, 1.0],
  };

  Outcome.EffectiveSolarTint =
    Cfg.TintSource === SolarTintSource.KelvinTemperature
      ? EvaluateKelvinToLinearRgb(Cfg.ColorTemperatureKelvin)
      : [
          Number(Cfg.ManualRgbTint?.[0] ?? 1.0),
          Number(Cfg.ManualRgbTint?.[1] ?? 1.0),
          Number(Cfg.ManualRgbTint?.[2] ?? 1.0),
        ];

  let Y = Math.trunc(Number(Cfg.Year) || 2026);
  let M = Math.max(1, Math.min(12, Math.trunc(Number(Cfg.Month) || 1)));
  const D = Math.max(1, Math.min(31, Math.trunc(Number(Cfg.DayOfMonth) || 1)));
  if (M <= 2) {
    --Y;
    M += 12;
  }

  const A = Math.trunc(Y / 100);
  const B = 2 - A + Math.trunc(A / 4);
  const UtcHours = Number(Cfg.LocalHours) - Number(Cfg.UtcOffsetHours);
  const Jd =
    Math.floor(365.25 * (Y + 4716)) +
    Math.floor(30.6001 * (M + 1)) +
    D +
    B -
    1524.5 +
    UtcHours / 24.0;
  const T = (Jd - 2451545.0) / 36525.0;

  const MeanLon = WrapDegrees360(280.46646 + T * (36000.76983 + T * 0.0003032));
  const MeanAnom = 357.52911 + T * (35999.05029 - 0.0001537 * T);
  const Ecc = 0.016708634 - T * (0.000042037 + 0.0000001267 * T);
  const Mr = MeanAnom * DEG_TO_RAD;
  const Centre =
    Math.sin(Mr) * (1.914602 - T * (0.004817 + 0.000014 * T)) +
    Math.sin(2.0 * Mr) * (0.019993 - 0.000101 * T) +
    Math.sin(3.0 * Mr) * 0.000289;
  const Omega = 125.04 - 1934.136 * T;
  const ApparentLon = MeanLon + Centre - 0.00569 - 0.00478 * Math.sin(Omega * DEG_TO_RAD);
  const MeanObliq =
    23.0 + (26.0 + (21.448 - T * (46.815 + T * (0.00059 - T * 0.001813))) / 60.0) / 60.0;
  const Obliq = MeanObliq + 0.00256 * Math.cos(Omega * DEG_TO_RAD);

  const Declination =
    Math.asin(Math.sin(Obliq * DEG_TO_RAD) * Math.sin(ApparentLon * DEG_TO_RAD)) * RAD_TO_DEG;
  const TanHalf = Math.tan(Obliq * 0.5 * DEG_TO_RAD);
  const Y2 = TanHalf * TanHalf;
  const L0r = MeanLon * DEG_TO_RAD;
  const EqTime =
    4.0 *
    RAD_TO_DEG *
    (Y2 * Math.sin(2.0 * L0r) -
      2.0 * Ecc * Math.sin(Mr) +
      4.0 * Ecc * Y2 * Math.sin(Mr) * Math.cos(2.0 * L0r) -
      0.5 * Y2 * Y2 * Math.sin(4.0 * L0r) -
      1.25 * Ecc * Ecc * Math.sin(2.0 * Mr));

  const TrueSolarRaw =
    (UtcHours * 60.0 + EqTime + 4.0 * Number(Cfg.SiteLongitudeDegrees)) % 1440.0;
  const TrueSolarMin = TrueSolarRaw < 0.0 ? TrueSolarRaw + 1440.0 : TrueSolarRaw;
  const HourAngle = TrueSolarMin * 0.25 - 180.0;

  const LatR = Number(Cfg.SiteLatitudeDegrees) * DEG_TO_RAD;
  const DecR = Declination * DEG_TO_RAD;
  const HaR = HourAngle * DEG_TO_RAD;
  const CosAngle = Math.max(
    -1.0,
    Math.min(
      1.0,
      Math.sin(LatR) * Math.sin(DecR) + Math.cos(LatR) * Math.cos(DecR) * Math.cos(HaR),
    ),
  );
  const PoleAngle = Math.acos(CosAngle);
  const Elevation = 90.0 - PoleAngle * RAD_TO_DEG;
  const SinPole = Math.sin(PoleAngle);

  let Azimuth = 0.0;
  if (Math.abs(SinPole) > 1.0e-9) {
    const CosAz = Math.max(
      -1.0,
      Math.min(
        1.0,
        (Math.sin(LatR) * Math.cos(PoleAngle) - Math.sin(DecR)) / (Math.cos(LatR) * SinPole),
      ),
    );
    Azimuth = Math.acos(CosAz) * RAD_TO_DEG;
    Azimuth = HourAngle > 0.0 ? WrapDegrees360(Azimuth + 180.0) : WrapDegrees360(540.0 - Azimuth);
  }

  const GmstDeg = WrapDegrees360(
    280.46061837 + 360.98564736629 * (Jd - 2451545.0) + Number(Cfg.SiteLongitudeDegrees),
  );
  Outcome.SiderealHours = GmstDeg / 15.0;
  Outcome.DeclinationDegrees = Declination;
  Outcome.EquationOfTimeMinutes = EqTime;

  if (Cfg.ManualDirectionOverride) {
    Outcome.AzimuthDegrees = WrapDegrees360(Number(Cfg.ManualAzimuthDegrees) || 0.0);
    Outcome.ElevationDegrees = Math.max(
      -90.0,
      Math.min(90.0, Number(Cfg.ManualElevationDegrees) || 0.0),
    );
  } else {
    Outcome.AzimuthDegrees = Azimuth;
    Outcome.ElevationDegrees = Elevation;
  }

  Outcome.WorldDirectionToSun = ConvertAzimuthElevationToWorldAxis(
    Outcome.AzimuthDegrees,
    Outcome.ElevationDegrees,
  );
  return Outcome;
}

// Advances LocalHours monotonically when CycleAnimationEnabled is active
export function AdvanceDayCycleHours(Config, DeltaSeconds) {
  if (!Config.CycleAnimationEnabled || DeltaSeconds <= 0.0) return Config;
  let DurationH = Number(Config.DayDurationHours) || 3.0;
  switch (Config.CycleSpeedPreset) {
    case DayCycleSpeedPreset.Multiplier1:
      DurationH = 24.0;
      break;
    case DayCycleSpeedPreset.Multiplier8:
      DurationH = 3.0;
      break;
    case DayCycleSpeedPreset.Multiplier30:
      DurationH = 0.8;
      break;
    case DayCycleSpeedPreset.Multiplier100:
      DurationH = 0.24;
      break;
    default:
      break;
  }
  DurationH = Math.max(0.01, DurationH);
  let NextH = (Number(Config.LocalHours) + (24.0 / DurationH) * DeltaSeconds) % 24.0;
  if (NextH < 0.0) NextH += 24.0;
  Config.LocalHours = NextH;
  return Config;
}

// Legacy convenience wrapper (solar panel readouts / light-graph plots) returning the solved elevation & azimuth
export function Solar({
  Year = 2026,
  Month = 9,
  Day = 10,
  LocalHours = 15.5,
  UtcOffset = 2,
  Latitude = -26.19,
  Longitude = 28.32,
} = {}) {
  const Solved = SolveSolarEphemeris({
    Year,
    Month,
    DayOfMonth: Day,
    LocalHours,
    UtcOffsetHours: UtcOffset,
    SiteLatitudeDegrees: Latitude,
    SiteLongitudeDegrees: Longitude,
  });
  return {
    Elevation: Solved.ElevationDegrees,
    Azimuth: Solved.AzimuthDegrees,
    Declination: Solved.DeclinationDegrees,
    EquationOfTime: Solved.EquationOfTimeMinutes,
    SiderealHours: Solved.SiderealHours,
    WorldDirectionToSun: Solved.WorldDirectionToSun,
    EffectiveSolarTint: Solved.EffectiveSolarTint,
  };
}
