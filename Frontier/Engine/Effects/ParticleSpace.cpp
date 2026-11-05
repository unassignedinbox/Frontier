/*==========================================================================================================================================
                                                            PARTICLESPACE.CPP
==========================================================================================================================================*/

#include "Frontier/Engine/Effects/ParticleSpace.h"

#include <algorithm>
#include <cmath>

namespace Frontier
{

namespace
{

// Small deterministic hash RNG (splitmix-style): same seed, same burst, on every machine.
struct HashRandom
{
    uint64_t State;
    float Next() noexcept
    {
        State += 0x9E3779B97F4A7C15ull;
        uint64_t Value = State;
        Value = (Value ^ (Value >> 30)) * 0xBF58476D1CE4E5B9ull;
        Value = (Value ^ (Value >> 27)) * 0x94D049BB133111EBull;
        Value = Value ^ (Value >> 31);
        return static_cast<float>(Value >> 40) / 16777216.0f;   // [0,1)
    }
};

} // namespace

const ParticlePreset* FindParticlePreset(std::string_view Name) noexcept
{
    for (const ParticlePreset& Preset : ParticlePresets)
        if (Preset.Name == Name)
            return &Preset;
    return nullptr;
}

void ParticleSpace::Emit(const ParticlePreset& Preset, const std::array<float, 3>& OriginMetres,
                         const std::array<float, 3>& EmitterVelocityMps, int Count, uint32_t Seed)
{
    if (Pool.empty() || Count <= 0)
        return;
    HashRandom Random{ (static_cast<uint64_t>(Seed) << 32) | (++EmissionCounter * 2654435761ull) };
    for (int Index = 0; Index < Count; ++Index)
    {
        Particle& Particle = Pool[Cursor];
        Cursor = (Cursor + 1) % Pool.size();

        // A burst in a sphere, biased upward/outward by the preset's Burst share, at a preset speed.
        const float A = Random.Next() * 6.28318530718f;
        const float CosB = Random.Next() * 2.0f - 1.0f;
        const float SinB = std::sqrt(std::max(0.0f, 1.0f - CosB * CosB));
        const float Speed = Preset.SpeedMinimum + Random.Next() * (Preset.SpeedMaximum - Preset.SpeedMinimum);
        const std::array<float, 3> Direction = { SinB * std::cos(A), SinB * std::sin(A), CosB };

        Particle.PositionMetres = OriginMetres;
        Particle.VelocityMps = {
            EmitterVelocityMps[0] + Direction[0] * Speed,
            EmitterVelocityMps[1] + Direction[1] * Speed,
            EmitterVelocityMps[2] + std::fabs(Direction[2]) * Speed * Preset.Burst + Speed * 0.25f };
        Particle.AgeSeconds = -Random.Next() * Preset.HoldOffset * Preset.Wave;   // the release wave
        Particle.LifeSeconds = Preset.LifeSeconds * (0.7f + 0.6f * Random.Next());
        Particle.SizeMetres = Preset.SizeMetres;
        Particle.Colour = Preset.Style == 1 && (Index & 3) == 0
            ? std::array<float, 3>{ 1.0f, 0.18f, 0.44f }    // glitch magenta accent
            : std::array<float, 3>{ 0.2f, 0.84f, 1.0f };    // derez glow
        (void)Index;
    }
}

void ParticleSpace::Step(float StepSeconds, float GravityMps2)
{
    if (StepSeconds <= 0.0f)
        return;
    for (Particle& Particle : Pool)
    {
        if (Particle.AgeSeconds < 0.0f)
        {
            Particle.AgeSeconds += StepSeconds;   // waiting for its release in the wave
            continue;
        }
        if (!Particle.IsAlive())
            continue;
        Particle.AgeSeconds += StepSeconds;
        const float Drag = std::exp(-0.4f * StepSeconds);
        Particle.VelocityMps[0] *= Drag;
        Particle.VelocityMps[1] *= Drag;
        Particle.VelocityMps[2] = Particle.VelocityMps[2] * Drag + GravityMps2 * StepSeconds;
        Particle.PositionMetres[0] += Particle.VelocityMps[0] * StepSeconds;
        Particle.PositionMetres[1] += Particle.VelocityMps[1] * StepSeconds;
        Particle.PositionMetres[2] += Particle.VelocityMps[2] * StepSeconds;
        if (Particle.PositionMetres[2] < Particle.SizeMetres && Particle.VelocityMps[2] < 0.0f)
        {
            Particle.PositionMetres[2] = Particle.SizeMetres;
            Particle.VelocityMps[2] *= -0.35f;   // a dead bounce on the floor
        }
    }
}

void ParticleSpace::Clear() noexcept
{
    for (Particle& Particle : Pool)
        Particle.AgeSeconds = Particle.LifeSeconds = 0.0f;
    Cursor = 0;
}

size_t ParticleSpace::QueryAliveCount() const noexcept
{
    return static_cast<size_t>(std::count_if(Pool.begin(), Pool.end(),
                                             [](const Particle& Particle) { return Particle.AgeSeconds >= 0.0f && Particle.IsAlive(); }));
}

} // namespace Frontier
