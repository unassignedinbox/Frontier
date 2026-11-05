/*==========================================================================================================================================
                                                             PLUME SPACE
                                     ------------------------------------------------------------------
                   1:1 C++ implementation of PlumeSpace.js, PlumeSourceProjection.js, and SmokeSequence.js.
==========================================================================================================================================*/

#include "PlumeSpace.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace Frontier::VolumetricDynamics
{

namespace
{
constexpr float SourcePowerOnset      = 2200.0f;
constexpr float SourcePowerFull       = 30000.0f;
constexpr float SourceOnsetSeconds    = 0.08f;
constexpr float SourceReleaseRatio    = 0.55f;
constexpr float SourceReleaseSeconds  = 0.14f;
constexpr float MinimumSlipSpeed      = 1.2f;
constexpr float CarrierVelocityShare  = 0.42f;
constexpr float DensityRate           = 16.0f;
constexpr float SmokeHeatRate         = 140.0f;
constexpr float FireHeatPowerScale    = 0.022f;
constexpr float PatchRadius           = 0.24f;
constexpr float FlingShare            = 0.35f;
constexpr float FlingCeiling          = 14.0f;
constexpr float ArchShare             = 0.35f;
constexpr float WarmBase              = 333.15f;
constexpr float WarmSpan              = 80.0f;
constexpr float FlameSidewallReach    = 0.32f;
constexpr float FlameRise             = 3.4f;
constexpr float FireLightGain         = 0.00028f;
constexpr std::array<float, 3> FireColour = { 1.0f, 0.46f, 0.14f };
constexpr float Pi                    = 3.14159265358979323846f;
} // namespace

void PlumeSourceProjection::Reset() noexcept
{
    Count       = 0u;
    ActiveTyres = 0u;
    Lights.clear();
    Onsets      = {};
    Elapsed     = 0.0f;
    PopSeconds  = { 0.0f, 0.0f };
    PopStrength = { 0.0f, 0.0f };
}

float PlumeSourceProjection::ProjectUniform() noexcept
{
    Generator ^= Generator << 13u;
    Generator ^= Generator >> 17u;
    Generator ^= Generator << 5u;
    return static_cast<float>(Generator) / 4294967296.0f;
}

bool PlumeSourceProjection::AdvanceOnset(OnsetState& Onset,
                                         float Power,
                                         float SlipSpeed,
                                         bool InContact,
                                         float Seconds) noexcept
{
    if (!InContact)
    {
        Onset.RequestSeconds = 0.0f;
        Onset.QuietSeconds += Seconds;
        if (Onset.QuietSeconds >= SourceReleaseSeconds)
        {
            Onset.Active = false;
        }
        return Onset.Active;
    }

    const bool AboveEntry   = Power > SourcePowerOnset && SlipSpeed >= MinimumSlipSpeed;
    const bool AboveRelease = Power > SourcePowerOnset * SourceReleaseRatio && SlipSpeed >= MinimumSlipSpeed;
    if (AboveEntry)
    {
        Onset.RequestSeconds += Seconds;
        Onset.QuietSeconds = 0.0f;
        if (Onset.RequestSeconds + 1.0e-6f >= SourceOnsetSeconds)
        {
            Onset.Active = true;
        }
    }
    else if (Onset.Active && AboveRelease)
    {
        Onset.QuietSeconds = 0.0f;
    }
    else
    {
        Onset.RequestSeconds = 0.0f;
        Onset.QuietSeconds += Seconds;
        if (Onset.QuietSeconds >= SourceReleaseSeconds)
        {
            Onset.Active = false;
        }
    }
    return Onset.Active;
}

uint32_t PlumeSourceProjection::Project(const PlumeVehicleTelemetry& Telemetry,
                                        float Seconds,
                                        const PlumeSourceOptions& Options)
{
    const float Rate       = Options.EmissionRate;
    const float SmokeYield = Options.SmokeYield * Rate;
    const float HeatYield  = Options.HeatYield * Rate;
    const float SootYield  = Options.SootYield * Rate;

    Count       = 0u;
    ActiveTyres = 0u;
    Lights.clear();
    Elapsed += Seconds;

    auto Emit = [this](const std::array<float, 3>& Position,
                       float Radius,
                       const std::array<float, 3>& Velocity,
                       float Smoke,
                       float Heat,
                       float Flame,
                       float Soot) noexcept
    {
        if (Count >= PlumeSourceCapacity || (Smoke <= 0.0f && Heat <= 0.0f && Flame <= 0.0f && Soot <= 0.0f))
        {
            return;
        }
        auto& Rec = Records[Count++];
        Rec.PositionRadius = { Position[0], Position[1], Position[2], Radius };
        Rec.Velocity       = { Velocity[0], Velocity[1], Velocity[2], 0.0f };
        Rec.Rates          = { Smoke, Heat, Flame, Soot };
    };

    for (uint32_t Index = 0u; Index < 4u; ++Index)
    {
        const auto& Wheel   = Telemetry.Wheels[Index];
        const bool Burning  = Wheel.Ignited && Wheel.CombustionPower > 0.0f;
        const float Power   = Wheel.SlidingPower;
        const bool Sliding  = AdvanceOnset(Onsets[Index], Power, Wheel.SlipSpeed, Wheel.InContact, Seconds);
        if (!Sliding && !Burning)
        {
            continue;
        }

        ++ActiveTyres;
        const auto& Anchor = (Burning && !Wheel.InContact) ? Wheel.HubPosition : Wheel.ContactPoint;
        const std::array<float, 3> Position = { Anchor[0], Anchor[1], Anchor[2] + 0.04f };
        const float Strength = Sliding
            ? std::clamp((Power - SourcePowerOnset) / std::max(1.0f, SourcePowerFull - SourcePowerOnset), 0.0f, 4.0f)
            : 0.0f;
        const float Warm  = std::clamp((Wheel.SurfaceTemperature - WarmBase) / WarmSpan, 0.0f, 1.0f);
        const float Smoke = DensityRate * Strength * (0.35f + 0.65f * Warm) * (Wheel.Burst ? 0.25f : 1.0f);

        const float GroundAlong = Wheel.ContactVelocity[0] * Wheel.Longitudinal[0]
                                + Wheel.ContactVelocity[1] * Wheel.Longitudinal[1]
                                + Wheel.ContactVelocity[2] * Wheel.Longitudinal[2];
        const float SurfaceSlip = Wheel.SurfaceSpeed - GroundAlong;
        const float Fling       = std::clamp(FlingShare * SurfaceSlip, -FlingCeiling, FlingCeiling);
        const auto& Heading     = Wheel.Longitudinal;
        const std::array<float, 3> Thrown = {
            Wheel.ContactVelocity[0] * CarrierVelocityShare - Heading[0] * Fling,
            Wheel.ContactVelocity[1] * CarrierVelocityShare - Heading[1] * Fling,
            Wheel.ContactVelocity[2] * CarrierVelocityShare - Heading[2] * Fling + 0.35f
        };

        Emit(Position, PatchRadius * 1.4f, Thrown, Smoke * SmokeYield, SmokeHeatRate * Strength * HeatYield, 0.0f, 0.0f);

        if (Sliding && std::fabs(Fling) > 1.0f && Wheel.Radius > 0.0f)
        {
            const float Sign = (Fling >= 0.0f) ? -1.0f : 1.0f;
            const std::array<float, 3> Back = { Heading[0] * Sign, Heading[1] * Sign, Heading[2] * Sign };
            const std::array<float, 3> Arch = {
                Wheel.HubPosition[0] + Back[0] * Wheel.Radius * 0.85f,
                Wheel.HubPosition[1] + Back[1] * Wheel.Radius * 0.85f,
                Wheel.HubPosition[2] + Back[2] * Wheel.Radius * 0.85f + Wheel.Radius * 0.35f
            };
            const std::array<float, 3> ArchVel = {
                Back[0] * 0.45f * std::fabs(Fling),
                Back[1] * 0.45f * std::fabs(Fling),
                Back[2] * 0.45f * std::fabs(Fling) + 1.0f
            };
            Emit(Arch, 0.28f, ArchVel, ArchShare * Smoke * SmokeYield, 0.0f, 0.0f, 0.0f);
        }

        if (Burning)
        {
            const float SideSign = (Index & 1u) ? -1.0f : 1.0f;
            const std::array<float, 3> Outward = { -Heading[1] * SideSign, Heading[0] * SideSign, 0.0f };
            const float Radius = (Wheel.Radius > 0.0f) ? Wheel.Radius : 0.5f;
            const std::array<float, 3> Sidewall = {
                Wheel.HubPosition[0] + Outward[0] * FlameSidewallReach,
                Wheel.HubPosition[1] + Outward[1] * FlameSidewallReach,
                Wheel.HubPosition[2] - 0.3f * Radius
            };
            const std::array<float, 3> Trailing = {
                Wheel.HubPosition[0] - Heading[0] * (Radius + 0.12f),
                Wheel.HubPosition[1] - Heading[1] * (Radius + 0.12f),
                Wheel.HubPosition[2] - 0.15f * Radius
            };
            const std::array<float, 3> Carried = {
                Wheel.ContactVelocity[0] * CarrierVelocityShare,
                Wheel.ContactVelocity[1] * CarrierVelocityShare,
                Wheel.ContactVelocity[2] * CarrierVelocityShare
            };
            const float Heat = Wheel.CombustionPower * FireHeatPowerScale * HeatYield;
            Emit(Sidewall,
                 0.30f,
                 { Carried[0] + Outward[0] * 0.4f, Carried[1] + Outward[1] * 0.4f, Carried[2] + FlameRise },
                 0.6f * SmokeYield,
                 0.6f * Heat,
                 7.0f * Rate,
                 8.0f * SootYield);
            Emit(Trailing,
                 0.28f,
                 { Carried[0], Carried[1], Carried[2] + FlameRise },
                 0.4f * SmokeYield,
                 0.4f * Heat,
                 5.0f * Rate,
                 6.0f * SootYield);

            const float Flicker = 0.78f + 0.22f * std::sin(Elapsed * 17.0f + static_cast<float>(Index) * 2.1f)
                                                * std::sin(Elapsed * 6.3f + static_cast<float>(Index));
            const float Intensity = Wheel.CombustionPower * FireLightGain * Options.FireLight * Flicker;
            Lights.push_back(PlumePointLight{
                { Sidewall[0], Sidewall[1], Sidewall[2] + 0.45f },
                0.40f,
                { FireColour[0] * Intensity, FireColour[1] * Intensity, FireColour[2] * Intensity }
            });
        }
    }

    for (size_t Tip = 0u; Tip < Telemetry.Exhausts.size() && Tip < 2u; ++Tip)
    {
        PopSeconds[Tip] = std::max(0.0f, PopSeconds[Tip] - Seconds);
        if (Options.Afterfire && Telemetry.AfterfireSeconds > 0.0f && PopSeconds[Tip] <= 0.0f && ProjectUniform() < Seconds * 14.0f)
        {
            PopSeconds[Tip]  = 0.035f + 0.055f * ProjectUniform();
            PopStrength[Tip] = 0.6f + 0.8f * ProjectUniform();
        }
        if (PopSeconds[Tip] <= 0.0f)
        {
            continue;
        }
        const auto& Exhaust  = Telemetry.Exhausts[Tip];
        const float Strength = PopStrength[Tip];
        const float Speed    = 8.0f + 6.0f * Strength;
        const std::array<float, 3> Velocity = {
            Telemetry.Velocity[0] * 0.7f + Exhaust.Direction[0] * Speed,
            Telemetry.Velocity[1] * 0.7f + Exhaust.Direction[1] * Speed,
            Telemetry.Velocity[2] * 0.7f + Exhaust.Direction[2] * Speed
        };
        const std::array<float, 3> Nozzle = {
            Exhaust.Position[0] + Exhaust.Direction[0] * 0.15f,
            Exhaust.Position[1] + Exhaust.Direction[1] * 0.15f,
            Exhaust.Position[2] + Exhaust.Direction[2] * 0.15f
        };
        Emit(Nozzle, 0.10f, Velocity, 1.0f * Strength * SmokeYield, 260.0f * Strength * HeatYield, 40.0f * Strength * Rate, 1.2f * Strength * SootYield);
        const float Intensity = 9.0f * Strength * Options.FireLight;
        Lights.push_back(PlumePointLight{
            { Nozzle[0] + Exhaust.Direction[0] * 0.3f, Nozzle[1] + Exhaust.Direction[1] * 0.3f, Nozzle[2] + Exhaust.Direction[2] * 0.3f },
            0.25f,
            { Intensity, 0.55f * Intensity, 0.20f * Intensity }
        });
    }

    std::sort(Lights.begin(), Lights.end(), [](const PlumePointLight& Left, const PlumePointLight& Right) {
        return (Right.Radiance[0] + Right.Radiance[1]) < (Left.Radiance[0] + Left.Radiance[1]);
    });
    return Count;
}

void PlumeSourceProjection::ResolveBounds(std::array<float, 3>& OutMin, std::array<float, 3>& OutMax) const noexcept
{
    if (Count == 0u)
    {
        OutMin = { 0.0f, 0.0f, 0.0f };
        OutMax = { 0.0f, 0.0f, 0.0f };
        return;
    }

    const float Inf = std::numeric_limits<float>::infinity();
    OutMin = { Inf, Inf, Inf };
    OutMax = { -Inf, -Inf, -Inf };
    for (uint32_t Index = 0u; Index < Count; ++Index)
    {
        const auto& Rec = Records[Index];
        const float Reach = std::max(Rec.PositionRadius[3], 0.3f) * 3.5f;
        for (uint32_t Axis = 0u; Axis < 3u; ++Axis)
        {
            OutMin[Axis] = std::min(OutMin[Axis], Rec.PositionRadius[Axis] - Reach);
            OutMax[Axis] = std::max(OutMax[Axis], Rec.PositionRadius[Axis] + Reach);
        }
    }
}

PlumeSpace::PlumeSpace(PlumeGradeConfiguration InConfiguration) noexcept
{
    AssignConfiguration(InConfiguration);
}

void PlumeSpace::AssignConfiguration(PlumeGradeConfiguration InConfiguration) noexcept
{
    Configuration  = InConfiguration;
    ThermoExtent   = Configuration.Extent;
    VelocityExtent = {
        Configuration.Extent[0] / 2u,
        Configuration.Extent[1] / 2u,
        Configuration.Extent[2] / 2u
    };
    CoarseEdge  = 2.0f * Configuration.CellExtent;
    Origin      = { 0, 0 };
    PriorOrigin = { PlumeRestartOrigin, PlumeRestartOrigin };
}

void PlumeSpace::Clear() noexcept
{
    Resident       = false;
    StepPending    = false;
    State          = 0u;
    Accumulator    = 0.0f;
    QuietSeconds   = 0.0f;
    QuietPrior     = 0.0f;
    StepOrdinal    = 0;
    ElapsedSeconds = 0.0f;
    StepsTaken     = 0u;
    FireLightCount = 0u;
    PriorOrigin    = { PlumeRestartOrigin, PlumeRestartOrigin };
    FireLights     = {};
}

void PlumeSpace::RecentreWindow(const std::array<float, 3>& Focus, bool Force) noexcept
{
    const int32_t CellX   = static_cast<int32_t>(std::floor(Focus[0] / CoarseEdge));
    const int32_t CellY   = static_cast<int32_t>(std::floor(Focus[1] / CoarseEdge));
    const int32_t HalfX   = static_cast<int32_t>(VelocityExtent[0] >> 1u);
    const int32_t HalfY   = static_cast<int32_t>(VelocityExtent[1] >> 1u);
    const int32_t CentreX = Origin[0] + HalfX;
    const int32_t CentreY = Origin[1] + HalfY;
    if (Force || std::abs(CellX - CentreX) > HalfX / 2 || std::abs(CellY - CentreY) > HalfY / 2)
    {
        Origin = { CellX - HalfX, CellY - HalfY };
    }
}

void PlumeSpace::Schedule(float Seconds,
                          float FrameSeconds,
                          const std::array<float, 3>& Focus,
                          const PlumeSourceProjection& Sources,
                          const PlumeSettings& Settings,
                          const std::array<float, 3>& InSunDirection) noexcept
{
    SunDirection = InSunDirection;
    StepPending  = false;
    const bool Active = Settings.PlumeEnabled && Sources.Count > 0u;
    if (Active)
    {
        QuietSeconds = 0.0f;
        if (!Resident)
        {
            Resident    = true;
            PriorOrigin = { PlumeRestartOrigin, PlumeRestartOrigin };
            Accumulator = PlumeStepSeconds;
            QuietPrior  = 0.0f;
            RecentreWindow(Focus, true);
        }
    }
    else if (Resident)
    {
        QuietSeconds += Seconds;
        if (QuietSeconds >= PlumeFadeCeiling || !Settings.PlumeEnabled)
        {
            Resident = false;
        }
    }

    if (!Resident)
    {
        Accumulator = 0.0f;
        return;
    }

    Accumulator += Seconds;
    ElapsedSeconds += Seconds;
    if (Accumulator < PlumeStepSeconds)
    {
        return;
    }

    const float StepDuration = std::min(Accumulator, PlumeStepCeiling);
    Accumulator = 0.0f;
    RecentreWindow(Focus, false);
    EncodeRecord(StepDuration, FrameSeconds, Sources, Settings);
    StepPending = true;
}

void PlumeSpace::EncodeRecord(float StepDuration,
                              float FrameSeconds,
                              const PlumeSourceProjection& Sources,
                              const PlumeSettings& Settings) noexcept
{
    const float Edge = Configuration.CellExtent;
    float Fade = 1.0f;
    if (QuietSeconds > 0.0f)
    {
        const float Remaining = std::max(0.0f, PlumeFadeCeiling - QuietSeconds);
        Fade = Remaining / std::max(StepDuration, PlumeFadeCeiling - QuietPrior);
    }
    QuietPrior = QuietSeconds;

    const float Dissipation = std::min(0.95f, Settings.PlumeDissipation);
    const float Cooling     = std::min(0.95f, Settings.PlumeCooling);
    const float WindAngle   = Settings.PlumeWindDirection * Pi / 180.0f;
    const std::array<float, 3> Wind = {
        std::cos(WindAngle) * Settings.PlumeWindSpeed,
        std::sin(WindAngle) * Settings.PlumeWindSpeed,
        0.0f
    };

    std::array<float, 3> BoundsMin = {};
    std::array<float, 3> BoundsMax = {};
    Sources.ResolveBounds(BoundsMin, BoundsMax);

    Record.VelocityOrigin      = { Origin[0], Origin[1], 0, StepOrdinal++ };
    Record.PriorVelocityOrigin = { PriorOrigin[0], PriorOrigin[1], 0, static_cast<int32_t>(Sources.Count) };
    Record.VelocityExtent      = { VelocityExtent[0], VelocityExtent[1], VelocityExtent[2], 0u };
    Record.ThermoExtent        = { ThermoExtent[0], ThermoExtent[1], ThermoExtent[2], Settings.Inspection == 10u ? 1u : 0u };
    Record.Lattice             = { Edge, 2.0f * Edge, PlumeFloorHeight, StepDuration };
    Record.Forces              = { Settings.PlumeBuoyancy, 0.12f, Settings.PlumeVorticity, Settings.PlumeTurbulence };
    Record.Retention           = {
        std::pow(1.0f - Dissipation, StepDuration) * Fade,
        std::pow(1.0f - Cooling, StepDuration) * Fade,
        std::exp(-Settings.PlumeFlameDecay * StepDuration) * Fade,
        std::pow(1.0f - 0.08f, StepDuration)
    };
    Record.Wind                = { Wind[0], Wind[1], Wind[2], ElapsedSeconds };
    Record.SourceMinimum       = { BoundsMin[0], BoundsMin[1], BoundsMin[2], std::max(FrameSeconds, 1.0e-3f) };
    Record.SourceMaximum       = { BoundsMax[0], BoundsMax[1], BoundsMax[2], PlumeFlameScale * Settings.PlumeFire };
    Record.Optical             = {
        Settings.PlumeExtinction,
        Settings.PlumeAlbedo,
        3.0f * Settings.PlumeExtinction,
        static_cast<float>(Configuration.MarchSamples)
    };
    Record.Sun                 = { SunDirection[0], SunDirection[1], SunDirection[2], Settings.PlumeSootAlbedo };
    Record.Combustion          = {
        2.4f * Settings.PlumeSoot,
        25.0f * Settings.PlumeHeat,
        std::pow(1.0f - 0.8f * Dissipation, StepDuration) * Fade,
        0.0f
    };
    Record.Scattering          = {
        Settings.PlumeScattering,
        Settings.PlumeAnisotropy,
        Settings.PlumeAmbient,
        Settings.PlumeFireLight
    };
    Record.Detail              = {
        Settings.PlumeDetail,
        Settings.PlumeDetailScale,
        Settings.PlumeTurbulenceScale,
        0.0f
    };
    Record.Flame               = {
        Settings.PlumeFlameCore,
        Settings.PlumeFlameTip,
        520.0f,
        0.0f
    };
}

void PlumeSpace::AssignFireLights(const std::vector<PlumePointLight>& Lights) noexcept
{
    FireLights = {};
    FireLightCount = static_cast<uint32_t>(std::min<size_t>(Lights.size(), FireLightLimit));
    FireLights.Count[0] = FireLightCount;
    for (uint32_t Index = 0u; Index < FireLightCount; ++Index)
    {
        const auto& Light = Lights[Index];
        FireLights.PositionRadius[Index] = { Light.Position[0], Light.Position[1], Light.Position[2], Light.Radius };
        FireLights.Radiance[Index]       = { Light.Radiance[0], Light.Radiance[1], Light.Radiance[2], 0.0f };
    }
}

void PlumeSpace::CompleteStep() noexcept
{
    if (!StepPending)
    {
        return;
    }
    State       = 1u - State;
    PriorOrigin = Origin;
    StepPending = false;
    ++StepsTaken;
}

} // namespace Frontier::VolumetricDynamics
