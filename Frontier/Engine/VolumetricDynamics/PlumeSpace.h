/*==========================================================================================================================================
                                                             PLUME SPACE
                                     ------------------------------------------------------------------
                   The toroidal smoke-and-fire cell lattices (velocity at 2h, thermo at h, light at 2h),
                     tyre burnout / tyre fire / exhaust afterfire source projection, 256-byte PlumeRecord,
                     and 144-byte FireLightRecord — 1:1 C++ counterpart of PlumeSpace.js,
                     PlumeSourceProjection.js, and SmokeSequence.js.
==========================================================================================================================================*/

#pragma once

#include <array>
#include <cstdint>
#include <string_view>
#include <vector>

namespace Frontier::VolumetricDynamics
{

inline constexpr uint32_t PlumeSourceStride   = 12u;
inline constexpr uint32_t PlumeSourceCapacity = 24u;
inline constexpr uint32_t PlumeRecordBytes    = 256u;
inline constexpr uint32_t FireLightRecordBytes = 144u;
inline constexpr uint32_t FireLightLimit      = 4u;
inline constexpr int32_t  PlumeRestartOrigin  = (1 << 24);
inline constexpr float    PlumeStepSeconds    = 1.0f / 60.0f;
inline constexpr float    PlumeStepCeiling    = 1.0f / 30.0f;
inline constexpr float    PlumeFadeCeiling    = 12.0f;
inline constexpr float    PlumeFloorHeight    = -0.5f;
inline constexpr float    PlumeFlameScale     = 80.0f;

struct PlumeGradeConfiguration
{
    std::string_view        Name           = "High";
    std::array<uint32_t, 3> Extent         = { 128u, 128u, 32u };
    float                   CellExtent     = 0.25f;
    uint32_t                PressureSweeps = 16u;
    uint32_t                MarchSamples   = 48u;
};

inline constexpr std::array<PlumeGradeConfiguration, 4> PlumeGrades = {{
    { "Low",    { 64u,  64u,  20u }, 0.40f, 8u,  32u },
    { "Medium", { 96u,  96u,  24u }, 0.30f, 12u, 40u },
    { "High",   { 128u, 128u, 32u }, 0.25f, 16u, 48u },
    { "Ultra",  { 160u, 160u, 48u }, 0.20f, 20u, 64u }
}};

struct PlumeSettings
{
    bool     PlumeEnabled         = true;
    uint32_t PlumeGrade           = 2u;
    uint32_t Inspection           = 0u;
    float    PlumeEmissionRate    = 1.0f;
    float    PlumeDensity         = 1.2f;
    float    PlumeHeat            = 1.0f;
    float    PlumeBuoyancy        = 2.4f;
    float    PlumeExtinction      = 3.6f;
    float    PlumeAlbedo          = 0.88f;
    float    PlumeFire            = 1.0f;
    float    PlumeSoot            = 1.0f;
    float    PlumeSootAlbedo      = 0.05f;
    float    PlumeVorticity       = 3.0f;
    float    PlumeTurbulence      = 0.6f;
    float    PlumeTurbulenceScale = 3.7f;
    float    PlumeDissipation     = 0.10f;
    float    PlumeCooling         = 0.55f;
    float    PlumeFlameDecay      = 4.0f;
    float    PlumeWindSpeed       = 0.8f;
    float    PlumeWindDirection   = 30.0f;
    float    PlumeScattering      = 0.75f;
    float    PlumeAnisotropy      = 0.38f;
    float    PlumeAmbient         = 1.0f;
    float    PlumeFireLight       = 1.0f;
    float    PlumeDetail          = 0.7f;
    float    PlumeDetailScale     = 2.2f;
    float    PlumeFlameCore       = 1800.0f;
    float    PlumeFlameTip        = 1150.0f;
    float    PlumeEmbers          = 1.0f;
};

struct alignas(16) PlumeRecord
{
    std::array<int32_t, 4>  VelocityOrigin      = { 0, 0, 0, 0 };
    std::array<int32_t, 4>  PriorVelocityOrigin = { PlumeRestartOrigin, PlumeRestartOrigin, 0, 0 };
    std::array<uint32_t, 4> VelocityExtent      = { 64u, 64u, 16u, 0u };
    std::array<uint32_t, 4> ThermoExtent        = { 128u, 128u, 32u, 0u };
    std::array<float, 4>    Lattice             = { 0.25f, 0.50f, PlumeFloorHeight, PlumeStepSeconds };
    std::array<float, 4>    Forces              = { 2.4f, 0.12f, 3.0f, 0.6f };
    std::array<float, 4>    Retention           = { 1.0f, 1.0f, 1.0f, 1.0f };
    std::array<float, 4>    Wind                = { 0.0f, 0.0f, 0.0f, 0.0f };
    std::array<float, 4>    SourceMinimum       = { 0.0f, 0.0f, 0.0f, PlumeStepSeconds };
    std::array<float, 4>    SourceMaximum       = { 0.0f, 0.0f, 0.0f, PlumeFlameScale };
    std::array<float, 4>    Optical             = { 3.6f, 0.88f, 10.8f, 48.0f };
    std::array<float, 4>    Sun                 = { 0.0f, 0.0f, 1.0f, 0.05f };
    std::array<float, 4>    Combustion          = { 2.4f, 25.0f, 1.0f, 0.0f };
    std::array<float, 4>    Scattering          = { 0.75f, 0.38f, 1.0f, 1.0f };
    std::array<float, 4>    Detail              = { 0.7f, 2.2f, 3.7f, 0.0f };
    std::array<float, 4>    Flame               = { 1800.0f, 1150.0f, 520.0f, 0.0f };
};
static_assert(sizeof(PlumeRecord) == PlumeRecordBytes, "PlumeRecord must be 256 bytes (16 x vec4)");

struct alignas(16) PlumeSourceRecord
{
    std::array<float, 4> PositionRadius = { 0.0f, 0.0f, 0.0f, 0.3f };
    std::array<float, 4> Velocity       = { 0.0f, 0.0f, 0.0f, 0.0f };
    std::array<float, 4> Rates          = { 0.0f, 0.0f, 0.0f, 0.0f };
};
static_assert(sizeof(PlumeSourceRecord) == PlumeSourceStride * sizeof(float), "PlumeSourceRecord must be 48 bytes");

struct alignas(16) FireLightRecord
{
    std::array<uint32_t, 4>               Count          = { 0u, 0u, 0u, 0u };
    std::array<std::array<float, 4>, 4>   PositionRadius = {};
    std::array<std::array<float, 4>, 4>   Radiance       = {};
};
static_assert(sizeof(FireLightRecord) == FireLightRecordBytes, "FireLightRecord must be 144 bytes");

struct PlumeWheelTelemetry
{
    bool                 InContact          = true;
    bool                 Burst              = false;
    float                SlidingPower       = 0.0f;
    float                SlipSpeed          = 0.0f;
    float                SurfaceSpeed       = 0.0f;
    float                Radius             = 0.34f;
    bool                 Ignited            = false;
    float                CombustionPower    = 0.0f;
    float                SurfaceTemperature = 333.15f;
    std::array<float, 3> ContactPoint       = { 0.0f, 0.0f, 0.0f };
    std::array<float, 3> HubPosition        = { 0.0f, 0.0f, 0.34f };
    std::array<float, 3> ContactVelocity    = { 0.0f, 0.0f, 0.0f };
    std::array<float, 3> Longitudinal       = { 1.0f, 0.0f, 0.0f };
};

struct PlumeExhaustTelemetry
{
    std::array<float, 3> Position  = { 0.0f, 0.0f, 0.0f };
    std::array<float, 3> Direction = { -1.0f, 0.0f, 0.0f };
};

struct PlumeVehicleTelemetry
{
    std::array<float, 3>                  Position         = { 0.0f, 0.0f, 0.0f };
    std::array<float, 3>                  Velocity         = { 0.0f, 0.0f, 0.0f };
    std::array<PlumeWheelTelemetry, 4>    Wheels           = {};
    std::vector<PlumeExhaustTelemetry>    Exhausts         = {};
    float                                 AfterfireSeconds = 0.0f;
};

struct PlumeSourceOptions
{
    bool  Afterfire    = false;
    float EmissionRate = 1.0f;
    float SmokeYield   = 1.2f;
    float HeatYield    = 1.0f;
    float SootYield    = 1.0f;
    float FireLight    = 1.0f;
    float Embers       = 1.0f;
};

struct PlumePointLight
{
    std::array<float, 3> Position = { 0.0f, 0.0f, 0.0f };
    float                Radius   = 0.4f;
    std::array<float, 3> Radiance = { 0.0f, 0.0f, 0.0f };
};

class PlumeSourceProjection
{
  public:
    struct OnsetState
    {
        float RequestSeconds = 0.0f;
        float QuietSeconds   = 0.0f;
        bool  Active         = false;
    };

    void Reset() noexcept;
    bool AdvanceOnset(OnsetState& Onset, float Power, float SlipSpeed, bool InContact, float Seconds) noexcept;
    uint32_t Project(const PlumeVehicleTelemetry& Telemetry, float Seconds, const PlumeSourceOptions& Options = {});
    void ResolveBounds(std::array<float, 3>& OutMin, std::array<float, 3>& OutMax) const noexcept;

    std::array<PlumeSourceRecord, PlumeSourceCapacity> Records     = {};
    uint32_t                                           Count       = 0u;
    uint32_t                                           ActiveTyres = 0u;
    std::vector<PlumePointLight>                       Lights      = {};
    std::array<OnsetState, 4>                          Onsets      = {};

  private:
    float ProjectUniform() noexcept;
    uint32_t Generator = 0x517CC1B7u;
    float    Elapsed   = 0.0f;
    std::array<float, 2> PopSeconds  = { 0.0f, 0.0f };
    std::array<float, 2> PopStrength = { 0.0f, 0.0f };
};

class PlumeSpace
{
  public:
    explicit PlumeSpace(PlumeGradeConfiguration Configuration = PlumeGrades[2]) noexcept;

    void AssignConfiguration(PlumeGradeConfiguration Configuration) noexcept;
    void Clear() noexcept;
    void RecentreWindow(const std::array<float, 3>& Focus, bool Force) noexcept;
    void Schedule(float Seconds,
                  float FrameSeconds,
                  const std::array<float, 3>& Focus,
                  const PlumeSourceProjection& Sources,
                  const PlumeSettings& Settings,
                  const std::array<float, 3>& SunDirection = { 0.0f, 0.0f, 1.0f }) noexcept;
    void EncodeRecord(float StepDuration,
                      float FrameSeconds,
                      const PlumeSourceProjection& Sources,
                      const PlumeSettings& Settings) noexcept;
    void AssignFireLights(const std::vector<PlumePointLight>& Lights) noexcept;
    void CompleteStep() noexcept;

    [[nodiscard]] static inline std::array<int32_t, 3> WrapLattice(std::array<int32_t, 3> Cell,
                                                                   std::array<uint32_t, 3> Extent) noexcept
    {
        const int32_t EdgeX = static_cast<int32_t>(Extent[0]);
        const int32_t EdgeY = static_cast<int32_t>(Extent[1]);
        return {
            ((Cell[0] % EdgeX) + EdgeX) % EdgeX,
            ((Cell[1] % EdgeY) + EdgeY) % EdgeY,
            Cell[2]
        };
    }

    PlumeGradeConfiguration Configuration  = PlumeGrades[2];
    std::array<uint32_t, 3> ThermoExtent   = { 128u, 128u, 32u };
    std::array<uint32_t, 3> VelocityExtent = { 64u, 64u, 16u };
    float                   CoarseEdge     = 0.50f;
    std::array<int32_t, 2>  Origin         = { 0, 0 };
    std::array<int32_t, 2>  PriorOrigin    = { PlumeRestartOrigin, PlumeRestartOrigin };
    std::array<float, 3>    SunDirection   = { 0.0f, 0.0f, 1.0f };

    bool                    Resident       = false;
    bool                    StepPending    = false;
    uint32_t                State          = 0u;
    float                   Accumulator    = 0.0f;
    float                   QuietSeconds   = 0.0f;
    float                   QuietPrior     = 0.0f;
    int32_t                 StepOrdinal    = 0;
    float                   ElapsedSeconds = 0.0f;
    uint32_t                StepsTaken     = 0u;
    uint32_t                FireLightCount = 0u;

    PlumeRecord             Record         = {};
    FireLightRecord         FireLights     = {};
};

} // namespace Frontier::VolumetricDynamics
