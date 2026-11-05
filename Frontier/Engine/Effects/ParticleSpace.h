#pragma once
/*==========================================================================================================================================
                                                             PARTICLESPACE.H
==========================================================================================================================================*/
// 🧩 The particle runtime: a pooled CPU particle space carrying the Particle Editor's presets (the three
//    derez looks, in this engine's terms — see EffectSequence.js's DerezPresets) plus spark bursts. Its
//    editor already exists in C++ in the editor tree (Frontier/Engine/Interface/ParticleEditor in the
//    reference branch); this is the runtime half it edits: emitters write particles, gravity and drag
//    move them, presentations draw what is alive. Deterministic with a fixed seed.

#include <array>
#include <cstdint>
#include <string_view>
#include <vector>

namespace Frontier
{

struct ParticlePreset
{
    std::string_view Name;
    int Style = 0;              // 0 cube, 1 glitch (the editor's two families)
    float HoldOffset = 0.3f;    // [-] release-wave offset across the emitter
    float Wave = 1.0f;          // [-] release-wave scale
    float Burst = 0.5f;         // [-] share of speed along the surface normal
    float SpeedMinimum = 0.6f;  // [m/s]
    float SpeedMaximum = 2.6f;  // [m/s]
    float Drag = 0.4f;          // [1/s]
    float LifeSeconds = 3.5f;   // [s]
    float SizeMetres = 0.075f;  // [m]
};

// 📝 The Particle Editor's three presets, verbatim (HTML EffectSequence's DerezPresets).
inline constexpr std::array<ParticlePreset, 3> ParticlePresets = {{
    { "Derez cube",      0, 0.30f, 1.0f, 0.5f, 0.6f, 2.6f, 0.4f, 3.5f, 0.075f },
    { "Reverse derez",   0, 0.15f, 0.8f, 0.3f, 0.4f, 1.8f, 0.6f, 2.8f, 0.070f },
    { "Glitch dissolve", 1, 0.10f, 0.9f, 0.2f, 0.2f, 1.2f, 1.2f, 2.0f, 0.060f }
}};

[[nodiscard]] const ParticlePreset* FindParticlePreset(std::string_view Name) noexcept;

struct Particle
{
    std::array<float, 3> PositionMetres = { 0.0f, 0.0f, 0.0f };
    std::array<float, 3> VelocityMps = { 0.0f, 0.0f, 0.0f };
    float AgeSeconds = 0.0f;
    float LifeSeconds = 1.0f;
    float SizeMetres = 0.07f;
    std::array<float, 3> Colour = { 0.2f, 0.84f, 1.0f };   // the derez glow (#35d6ff) by default

    [[nodiscard]] bool IsAlive() const noexcept { return AgeSeconds < LifeSeconds; }
    [[nodiscard]] float LifeFraction() const noexcept { return LifeSeconds > 0.0f ? AgeSeconds / LifeSeconds : 1.0f; }
};

class ParticleSpace
{
  public:
    explicit ParticleSpace(size_t Capacity = 8192) : Pool(Capacity) {}

    // 📝 Emits Count particles of the preset from a point, with the emitter's own velocity (momentum:
    //    a moving car's cubes leave with the car). Deterministic per (seed, emission index).
    void Emit(const ParticlePreset& Preset, const std::array<float, 3>& OriginMetres,
              const std::array<float, 3>& EmitterVelocityMps, int Count, uint32_t Seed = 1);

    void Step(float StepSeconds, float GravityMps2 = -9.81f);
    void Clear() noexcept;

    [[nodiscard]] size_t QueryAliveCount() const noexcept;
    [[nodiscard]] const std::vector<Particle>& QueryParticles() const noexcept { return Pool; }

  private:
    std::vector<Particle> Pool;
    size_t Cursor = 0;
    uint64_t EmissionCounter = 0;
};

} // namespace Frontier
