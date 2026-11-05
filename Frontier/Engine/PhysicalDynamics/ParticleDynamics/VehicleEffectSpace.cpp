/*==========================================================================================================================================
                                                        VEHICLE EFFECT SPACE
                                     ------------------------------------------------------------------
                   1:1 C++ implementation of SparkSequence.js, SparkCollision.js, and EffectSequence.js.
==========================================================================================================================================*/

#include "VehicleEffectSpace.h"

#include <algorithm>
#include <cmath>

namespace Frontier::ParticleDynamics
{

namespace
{
constexpr float Pi = 3.14159265358979323846f;
} // namespace

std::array<float, 3> ResolveSparkRadiance(float Kelvin) noexcept
{
    const float Hecto = std::clamp(Kelvin, 1000.0f, 6600.0f) / 100.0f;
    const float Green = std::clamp((99.4708025861f * std::log(Hecto) - 161.1195681661f) / 255.0f, 0.0f, 1.0f);
    const float Blue  = (Hecto <= 19.0f)
        ? 0.0f
        : std::clamp((138.5177312231f * std::log(std::max(Hecto - 10.0f, 1.0f)) - 305.0447927307f) / 255.0f, 0.0f, 1.0f);
    const float Ratio = Kelvin / 1500.0f;
    const float Brightness = 8.0f * Ratio * Ratio * Ratio * Ratio * std::clamp((Kelvin - SparkVisibleKelvin) / 500.0f, 0.0f, 1.0f);
    return {
        Brightness,
        std::pow(Green, 2.2f) * Brightness,
        std::pow(Blue, 2.2f) * Brightness
    };
}

float ResolveSparkGain(const SparkParticle& Spark) noexcept
{
    const float Fade   = std::clamp((Spark.LifeSeconds - Spark.Age) / 0.10f, 0.0f, 1.0f);
    const float Ripple = 1.0f - Spark.Flicker * (0.5f + 0.5f * std::sin(Spark.Age * Spark.Frequency * 2.0f * Pi + Spark.Phase));
    return Spark.Energy * Fade * Ripple;
}

std::array<float, 3> BounceSpark(const std::array<float, 3>& Velocity,
                                 const std::array<float, 3>& Normal) noexcept
{
    const float Approach = std::min(0.0f,
                                    Velocity[0] * Normal[0] + Velocity[1] * Normal[1] + Velocity[2] * Normal[2]);
    return {
        (Velocity[0] - Approach * Normal[0]) * 0.55f - Approach * Normal[0] * 0.28f,
        (Velocity[1] - Approach * Normal[1]) * 0.55f - Approach * Normal[1] * 0.28f,
        (Velocity[2] - Approach * Normal[2]) * 0.55f - Approach * Normal[2] * 0.28f
    };
}

void SparkSequence::Clear() noexcept
{
    Particles.clear();
    Instances.clear();
    Clock = 0.0f;
    Live  = 0u;
}

float SparkSequence::Random() noexcept
{
    Seed = Seed * 1664525u + 1013904223u;
    return static_cast<float>(Seed) / 4294967296.0f;
}

void SparkSequence::Emit(const std::vector<SparkScrape>& InScrapes, float Seconds)
{
    if (!Settings.Enabled || !(Seconds > 0.0f))
    {
        return;
    }

    Clock += std::min(Seconds, 0.1f);
    Scrapes = static_cast<uint32_t>(InScrapes.size());
    const float Variation = std::clamp(Settings.Variation, 0.0f, 1.0f);
    auto Mix = [Variation](float Neutral, float Varied) noexcept {
        return Neutral + (Varied - Neutral) * Variation;
    };

    for (const auto& Scrape : InScrapes)
    {
        const float Expected = SparksPerStrength * Settings.Rate * Scrape.Strength * Seconds;
        uint32_t Count = static_cast<uint32_t>(std::floor(Expected));
        if (Random() < Expected - static_cast<float>(Count))
        {
            ++Count;
        }
        Count = std::min(Count, 96u);

        const auto& Normal = Scrape.Normal;
        const auto& Slide  = Scrape.Velocity;
        const float SlideSpeed = std::sqrt(Slide[0] * Slide[0] + Slide[1] * Slide[1] + Slide[2] * Slide[2]);

        for (uint32_t SparkIdx = 0u; SparkIdx < Count && Particles.size() < SparkCapacity; ++SparkIdx)
        {
            const float Roll = Random();
            const uint32_t Kind = (Roll < 0.72f) ? 0u : ((Roll < 0.94f) ? 1u : 2u);
            const float Size = Mix(
                1.0f,
                Kind == 0u ? (0.25f + 0.50f * Random()) : (Kind == 1u ? (0.70f + 0.60f * Random()) : (1.30f + 0.70f * Random())));
            const float SpeedShare = Mix(
                1.0f,
                Kind == 2u ? (0.35f + 0.40f * Random()) : (0.65f + 0.55f * Random()));
            const std::array<float, 3> Spread = {
                Random() * 2.0f - 1.0f,
                Random() * 2.0f - 1.0f,
                Random() * 2.0f - 1.0f
            };

            std::array<float, 3> Origin   = {};
            std::array<float, 3> Velocity = {};
            if (Scrape.Rim.Active)
            {
                const auto& Forward = Scrape.Rim.Forward;
                const float SpinSpeed = Scrape.Rim.SpinSpeed;
                const float Radius = Scrape.Rim.Radius;
                const float Sense = (SpinSpeed < 0.0f) ? -1.0f : 1.0f;
                const float Rnd = Random();
                const float Detach = 0.02f + 0.45f * Rnd * Rnd;
                const float Cosine = std::cos(Detach);
                const float Sine = std::sin(Detach);

                for (uint32_t Axis = 0u; Axis < 3u; ++Axis)
                {
                    Origin[Axis] = Scrape.Point[Axis] + Radius * ((1.0f - Cosine) * Normal[Axis] - Sense * Sine * Forward[Axis]);
                }

                std::array<float, 3> Metal = {};
                for (uint32_t Axis = 0u; Axis < 3u; ++Axis)
                {
                    Metal[Axis] = Slide[Axis] + SpinSpeed * (1.0f - Cosine) * Forward[Axis] + std::fabs(SpinSpeed) * Sine * Normal[Axis];
                }
                const float Speed = std::sqrt(Metal[0] * Metal[0] + Metal[1] * Metal[1] + Metal[2] * Metal[2]);
                const float Share = 0.75f + 0.25f * Random();
                const float Scatter = 0.12f * Speed + 0.4f;
                const float Bounce = 0.3f + 1.2f * Random();
                for (uint32_t Axis = 0u; Axis < 3u; ++Axis)
                {
                    Velocity[Axis] = (Metal[Axis] * Share + Spread[Axis] * Scatter + Normal[Axis] * Bounce) * SpeedShare;
                }
            }
            else
            {
                const float Share = 0.3f + 0.45f * Random();
                const float Fan = 0.6f * std::max(2.0f, SlideSpeed * 0.35f);
                const float Kick = 1.0f + 3.0f * Random();
                Origin = {
                    Scrape.Point[0] + (Random() - 0.5f) * 0.04f,
                    Scrape.Point[1] + (Random() - 0.5f) * 0.04f,
                    Scrape.Point[2] + 0.01f + Random() * 0.02f
                };
                for (uint32_t Axis = 0u; Axis < 3u; ++Axis)
                {
                    Velocity[Axis] = (Slide[Axis] * Share + Spread[Axis] * Fan + Normal[Axis] * Kick) * SpeedShare;
                }
            }

            SparkParticle Particle;
            Particle.Position    = Origin;
            Particle.Prior       = Origin;
            Particle.Velocity    = Velocity;
            Particle.Kind        = Kind;
            Particle.Kelvin      = Mix(2450.0f, Kind == 2u ? (1800.0f + 800.0f * Random()) : (2100.0f + 900.0f * Random()));
            Particle.Drag        = Mix(SparkAirDrag, Kind == 0u ? (2.0f + 2.5f * Random()) : (0.65f + 1.2f * Random()));
            Particle.Cooling     = Mix(1.0f, Kind == 0u ? (0.55f + 0.35f * Random()) : (Kind == 1u ? (0.85f + 0.40f * Random()) : (1.2f + 0.5f * Random())));
            Particle.LifeSeconds = Mix(2.5f, Kind == 0u ? (0.20f + 0.40f * Random()) : (Kind == 1u ? (0.45f + 0.75f * Random()) : (0.90f + Random())));
            Particle.BounceScale = Mix(1.0f, 0.70f + 0.30f * Random());
            Particle.Energy      = Mix(1.0f, 0.35f + 0.85f * Random());
            Particle.Flicker     = Mix(0.0f, 0.04f + 0.12f * Random());
            Particle.Frequency   = 10.0f + 18.0f * Random();
            Particle.Phase       = Random() * 2.0f * Pi;
            Particle.PixelWidth  = Mix(1.0f, Kind == 0u ? (0.40f + 0.35f * Random()) : (Kind == 1u ? (0.70f + 0.25f * Random()) : (0.90f + 0.10f * Random())));
            Particle.Shutter     = Mix(1.0f, Kind == 0u ? (0.18f + 0.45f * Random()) : (0.55f + 0.45f * Random()));
            Particle.Age         = 0.0f;
            Particle.BornSeconds = std::min(Seconds, 0.1f) * Random();
            Particle.HasBornStep = true;
            Particle.Impacts     = 0u;
            Particle.Size        = Size * Settings.Size;

            Particles.push_back(Particle);
            ++Emitted;
        }
    }
}

void SparkSequence::Advance(float Seconds, float GroundHeight)
{
    if (!(Seconds > 0.0f))
    {
        return;
    }
    if (!Settings.Enabled)
    {
        Clear();
        return;
    }

    std::vector<SparkParticle> Survivors;
    Survivors.reserve(Particles.size());

    for (auto& Spark : Particles)
    {
        Spark.Prior = Spark.Position;
        const float Duration = Spark.HasBornStep ? Spark.BornSeconds : std::min(Seconds, 0.1f);
        Spark.HasBornStep = false;
        const uint32_t Steps = std::max(1u, static_cast<uint32_t>(std::ceil(Duration * 120.0f)));
        const float Dt = Duration / static_cast<float>(Steps);

        for (uint32_t Step = 0u; Step < Steps; ++Step)
        {
            Spark.Age += Dt;
            const float Damping = std::exp(-Spark.Drag * Dt);
            Spark.Velocity[0] *= Damping;
            Spark.Velocity[1] *= Damping;
            Spark.Velocity[2]  = Spark.Velocity[2] * Damping - SparkGravity * Dt;

            const std::array<float, 3> From = Spark.Position;
            std::array<float, 3> To = {
                From[0] + Spark.Velocity[0] * Dt,
                From[1] + Spark.Velocity[1] * Dt,
                From[2] + Spark.Velocity[2] * Dt
            };

            if (To[2] <= GroundHeight)
            {
                const float Denom = From[2] - To[2];
                const float HitTime = (Denom > 1.0e-6f) ? std::clamp((From[2] - GroundHeight) / Denom, 0.0f, 1.0f) : 0.0f;
                Spark.Position = {
                    From[0] + (To[0] - From[0]) * HitTime,
                    From[1] + (To[1] - From[1]) * HitTime,
                    GroundHeight + 0.004f
                };
                const auto Bounced = BounceSpark(Spark.Velocity, { 0.0f, 0.0f, 1.0f });
                Spark.Velocity = {
                    Bounced[0] * Spark.BounceScale,
                    Bounced[1] * Spark.BounceScale,
                    Bounced[2] * Spark.BounceScale
                };
                ++Spark.Impacts;
                Spark.Kelvin = 293.0f + (Spark.Kelvin - 293.0f) * 0.92f;
                Spark.Prior  = Spark.Position;
            }
            else
            {
                Spark.Position = To;
            }

            Spark.Kelvin = 293.0f + (Spark.Kelvin - 293.0f) * std::exp(-Dt / (SparkCoolingSeconds * Spark.Cooling * std::max(0.5f, Spark.Size)));
        }

        const float Speed = std::sqrt(
            Spark.Velocity[0] * Spark.Velocity[0] +
            Spark.Velocity[1] * Spark.Velocity[1] +
            Spark.Velocity[2] * Spark.Velocity[2]);
        if (Spark.Kelvin > SparkVisibleKelvin && Spark.Age < Spark.LifeSeconds && (Spark.Impacts < 8u || Speed > 0.6f))
        {
            Survivors.push_back(Spark);
        }
    }

    Particles = std::move(Survivors);
    Live = static_cast<uint32_t>(Particles.size());
    Instances.resize(Live);
    for (uint32_t Index = 0u; Index < Live; ++Index)
    {
        const auto& Spark = Particles[Index];
        const auto BaseRad = ResolveSparkRadiance(Spark.Kelvin);
        const float Gain = Settings.Brightness * ResolveSparkGain(Spark);
        Instances[Index].HeadAndWidth     = { Spark.Position[0], Spark.Position[1], Spark.Position[2], SparkBaseWidth * Spark.Size };
        Instances[Index].TailAndPixelSize = { Spark.Prior[0], Spark.Prior[1], Spark.Prior[2], Spark.PixelWidth };
        Instances[Index].RadianceAndShut  = { BaseRad[0] * Gain, BaseRad[1] * Gain, BaseRad[2] * Gain, Spark.Shutter };
    }
}

std::array<float, 3> ParseEffectHexColour(std::string_view Hex) noexcept
{
    if (!Hex.empty() && Hex.front() == '#')
    {
        Hex.remove_prefix(1);
    }
    uint32_t Value = 0u;
    for (char Ch : Hex)
    {
        uint32_t Digit = 0u;
        if (Ch >= '0' && Ch <= '9') Digit = static_cast<uint32_t>(Ch - '0');
        else if (Ch >= 'a' && Ch <= 'f') Digit = static_cast<uint32_t>(Ch - 'a' + 10);
        else if (Ch >= 'A' && Ch <= 'F') Digit = static_cast<uint32_t>(Ch - 'A' + 10);
        Value = (Value << 4u) | Digit;
    }

    auto ToLinear = [](uint32_t Byte) noexcept -> float {
        const float Channel = static_cast<float>(Byte & 0xFFu) / 255.0f;
        return (Channel <= 0.04045f) ? (Channel / 12.92f) : std::pow((Channel + 0.055f) / 1.055f, 2.4f);
    };
    return {
        ToLinear((Value >> 16u) & 0xFFu),
        ToLinear((Value >> 8u) & 0xFFu),
        ToLinear(Value & 0xFFu)
    };
}

float HashEffectScalar(uint32_t Seed) noexcept
{
    uint32_t State = Seed * 747796405u + 2891336453u;
    uint32_t Word  = ((State >> ((State >> 28u) + 4u)) ^ State) * 277803737u;
    return static_cast<float>((Word >> 22u) ^ Word) / 4294967296.0f;
}

std::array<float, 3> EvaluateBallisticPosition(const std::array<float, 3>& Origin,
                                               const std::array<float, 3>& Velocity,
                                               float Drag,
                                               float Time) noexcept
{
    const float SafeDrag = std::max(Drag, 1.0e-4f);
    const std::array<float, 3> Terminal = { 0.0f, 0.0f, -SparkGravity / SafeDrag };
    const float Decay = (1.0f - std::exp(-SafeDrag * Time)) / SafeDrag;
    return {
        Origin[0] + Terminal[0] * Time + (Velocity[0] - Terminal[0]) * Decay,
        Origin[1] + Terminal[1] * Time + (Velocity[1] - Terminal[1]) * Decay,
        Origin[2] + Terminal[2] * Time + (Velocity[2] - Terminal[2]) * Decay
    };
}

} // namespace Frontier::ParticleDynamics
