/*==========================================================================================================================================
                                                        VEHICLE EFFECT SPACE
                                     ------------------------------------------------------------------
                   Sparks (2048-particle pool with hull/rim scrape emission, swept SDF collision,
                     blackbody cooling, and 48-byte streak instances) and closed-form Derez/Rez
                     cube effects (1920 vehicle anchors, 216 box anchors, 8 emitter slots, 60 Hz
                     pose history, 1120-byte EffectRecord) — 1:1 C++ counterpart of
                     SparkSequence.js, SparkCollision.js, and EffectSequence.js.
==========================================================================================================================================*/

#pragma once

#include <array>
#include <cstdint>
#include <string_view>
#include <vector>

namespace Frontier::ParticleDynamics
{

inline constexpr uint32_t SparkCapacity        = 2048u;
inline constexpr uint32_t SparkInstanceFloats  = 12u;
inline constexpr float    SparkGravity         = 9.81f;
inline constexpr float    SparkAirDrag         = 1.6f;
inline constexpr float    SparkCoolingSeconds  = 0.42f;
inline constexpr float    SparkVisibleKelvin   = 950.0f;
inline constexpr float    SparksPerStrength    = 700.0f;
inline constexpr float    SparkBaseWidth       = 0.0022f;

inline constexpr uint32_t EffectEmitterSlots     = 8u;
inline constexpr uint32_t EffectEmitterStride    = 128u;
inline constexpr uint32_t EffectRecordHeader     = 96u;
inline constexpr uint32_t EffectRecordBytes      = EffectRecordHeader + EffectEmitterSlots * EffectEmitterStride;
inline constexpr uint32_t EffectPoseCapacity     = 360u;
inline constexpr float    EffectPoseSpacing      = 1.0f / 60.0f;
inline constexpr uint32_t VehicleBodyCellCount   = 1536u;
inline constexpr uint32_t VehicleWheelCellCount  = 96u;
inline constexpr uint32_t VehicleTotalCellCount  = VehicleBodyCellCount + 4u * VehicleWheelCellCount;
inline constexpr uint32_t BoxCellCount           = 216u;

struct SparkRimContact
{
    bool                 Active    = false;
    std::array<float, 3> Forward   = { 1.0f, 0.0f, 0.0f };
    float                SpinSpeed = 0.0f;
    float                Radius    = 0.34f;
};

struct SparkScrape
{
    std::array<float, 3> Point    = { 0.0f, 0.0f, 0.0f };
    std::array<float, 3> Velocity = { 0.0f, 0.0f, 0.0f };
    std::array<float, 3> Normal   = { 0.0f, 0.0f, 1.0f };
    float                Strength = 0.0f;
    int32_t              Wheel    = -1;
    SparkRimContact      Rim      = {};
};

struct SparkSettings
{
    bool  Enabled    = true;
    float Rate       = 1.0f;
    float Brightness = 1.0f;
    float Size       = 1.0f;
    float Variation  = 1.0f;
};

struct SparkParticle
{
    std::array<float, 3> Position    = { 0.0f, 0.0f, 0.0f };
    std::array<float, 3> Prior       = { 0.0f, 0.0f, 0.0f };
    std::array<float, 3> Velocity    = { 0.0f, 0.0f, 0.0f };
    uint32_t             Kind        = 0u;
    float                Kelvin      = 2450.0f;
    float                Drag        = SparkAirDrag;
    float                Cooling     = 1.0f;
    float                LifeSeconds = 2.5f;
    float                BounceScale = 1.0f;
    float                Energy      = 1.0f;
    float                Flicker     = 0.0f;
    float                Frequency   = 14.0f;
    float                Phase       = 0.0f;
    float                PixelWidth  = 1.0f;
    float                Shutter     = 1.0f;
    float                Age         = 0.0f;
    float                BornSeconds = 0.0f;
    bool                 HasBornStep = false;
    uint32_t             Impacts     = 0u;
    float                Size        = 1.0f;
};

struct alignas(16) SparkInstanceRecord
{
    std::array<float, 4> HeadAndWidth     = { 0.0f, 0.0f, 0.0f, SparkBaseWidth };
    std::array<float, 4> TailAndPixelSize = { 0.0f, 0.0f, 0.0f, 1.0f };
    std::array<float, 4> RadianceAndShut  = { 0.0f, 0.0f, 0.0f, 1.0f };
};
static_assert(sizeof(SparkInstanceRecord) == SparkInstanceFloats * sizeof(float), "SparkInstanceRecord must be 48 bytes");

[[nodiscard]] std::array<float, 3> ResolveSparkRadiance(float Kelvin) noexcept;
[[nodiscard]] float ResolveSparkGain(const SparkParticle& Spark) noexcept;
[[nodiscard]] std::array<float, 3> BounceSpark(const std::array<float, 3>& Velocity,
                                               const std::array<float, 3>& Normal) noexcept;

class SparkSequence
{
  public:
    void Clear() noexcept;
    void Emit(const std::vector<SparkScrape>& Scrapes, float Seconds);
    void Advance(float Seconds, float GroundHeight = 0.0f);

    SparkSettings                    Settings  = {};
    std::vector<SparkParticle>       Particles = {};
    std::vector<SparkInstanceRecord> Instances = {};
    uint32_t                         Live      = 0u;
    uint32_t                         Emitted   = 0u;
    uint32_t                         Scrapes   = 0u;

  private:
    float Random() noexcept;
    uint32_t Seed  = 0x5EEDu;
    float    Clock = 0.0f;
};

enum class EffectKind : uint32_t
{
    VehicleDerez = 0u,
    VehicleRez   = 1u,
    BoxDerez     = 2u,
    BoxRez       = 3u,
    Debris       = 4u,
    Inactive     = 255u
};

enum class EffectMaskMode : uint32_t
{
    Off    = 0u,
    Derez  = 1u,
    Rez    = 2u,
    Hidden = 3u
};

struct DerezPresetSpecification
{
    std::string_view Name         = "Derez cube";
    uint32_t         Style        = 0u;
    float            HoldOffset   = 0.30f;
    float            Wave         = 1.00f;
    float            Burst        = 0.50f;
    float            SpeedMinimum = 0.60f;
    float            SpeedMaximum = 2.60f;
    float            Drag         = 0.40f;
    float            Life         = 3.50f;
    float            Size         = 0.075f;
    float            Flight       = 1.20f;
    std::string_view Glow         = "#35d6ff";
    std::string_view Hot          = "#ffffff";
    std::string_view Glitch       = "#ff2d6f";
};

inline constexpr std::array<DerezPresetSpecification, 3> DerezPresets = {{
    { "Derez cube",      0u, 0.30f, 1.0f, 0.5f, 0.6f, 2.6f, 0.4f, 3.5f, 0.075f, 1.2f, "#35d6ff", "#ffffff", "#ff2d6f" },
    { "Reverse derez",   0u, 0.15f, 0.8f, 0.3f, 0.4f, 1.8f, 0.6f, 2.8f, 0.070f, 1.3f, "#35d6ff", "#ffffff", "#ff2d6f" },
    { "Glitch dissolve", 1u, 0.10f, 0.9f, 0.2f, 0.2f, 1.2f, 1.2f, 2.0f, 0.060f, 1.0f, "#00ffd0", "#ffffff", "#ff2d6f" }
}};

struct alignas(16) EffectEmitterRecord
{
    std::array<float, 16> BodyToWorld = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    std::array<float, 4> VelocityAndStart = { 0.0f, 0.0f, 0.0f, 0.0f };
    std::array<float, 4> AngularAndKind   = { 0.0f, 0.0f, 0.0f, 255.0f };
    std::array<float, 4> ExtentAndCount   = { 0.0f, 0.0f, 0.0f, 0.0f };
    std::array<float, 4> CentreAndSeed    = { 0.0f, 0.0f, 0.0f, 0.0f };
};
static_assert(sizeof(EffectEmitterRecord) == EffectEmitterStride, "EffectEmitterRecord must be 128 bytes");

struct alignas(16) EffectUniformRecord
{
    float                                                Clock        = 0.0f;
    uint32_t                                             MaskMode     = 0u;
    float                                                MaskStart    = 0.0f;
    uint32_t                                             Style        = 0u;
    std::array<float, 4>                                 MinimumAndHold = { -2.5f, -1.0f, -0.6f, 0.30f };
    std::array<float, 4>                                 MaximumAndDt   = {  2.5f,  1.0f,  0.9f, 1.0f / 60.0f };
    std::array<float, 4>                                 Kinematics     = { 0.50f, 0.60f, 2.60f, 0.40f };
    std::array<float, 4>                                 Timing         = { 3.50f, 0.075f, 1.00f, 1.20f };
    std::array<float, 4>                                 Colour         = { 1.0f, 6.0f, 0.0f, 0.0f };
    std::array<EffectEmitterRecord, EffectEmitterSlots>  Emitters       = {};
};
static_assert(sizeof(EffectUniformRecord) == EffectRecordBytes, "EffectUniformRecord must be 1120 bytes");

[[nodiscard]] std::array<float, 3> ParseEffectHexColour(std::string_view Hex) noexcept;
[[nodiscard]] float HashEffectScalar(uint32_t Seed) noexcept;
[[nodiscard]] std::array<float, 3> EvaluateBallisticPosition(const std::array<float, 3>& Origin,
                                                             const std::array<float, 3>& Velocity,
                                                             float Drag,
                                                             float Time) noexcept;

} // namespace Frontier::ParticleDynamics
