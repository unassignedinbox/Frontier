#pragma once
/*==========================================================================================================================================
                                                         SMOKEFLUIDSYSTEM.H
==========================================================================================================================================*/
// 🧩 Native C++ counterpart to the Experimental/VehicleHmiPrototype SmokeSequence / PlumeSpace exhibit.
//    The gas-control catalogue, emitter modes, quality grades and solver-facing values are kept in one
//    portable contract; the interactive editor and software renderer both consume this system.

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace Frontier
{

struct SmokeVector3
{
    float X = 0.0f, Y = 0.0f, Z = 0.0f;
    SmokeVector3 operator+(const SmokeVector3& Other) const noexcept;
    SmokeVector3 operator-(const SmokeVector3& Other) const noexcept;
    SmokeVector3 operator*(float Scale) const noexcept;
};

[[nodiscard]] float SmokeDot(const SmokeVector3& A, const SmokeVector3& B) noexcept;
[[nodiscard]] SmokeVector3 SmokeNormalize(const SmokeVector3& V) noexcept;

// 📝 Parameters and exact UI ranges/defaults ported from Source/SmokeSequence.js GasParameterCatalogue.
enum class GasParameter : uint8_t
{
    Emission, Heat, Expansion, Soot, Momentum, TreadEjection,
    Buoyancy, Dissipation, Cooling, Vorticity, Turbulence, TurbulenceScale,
    WindSpeed, WindHeading, TimeScale, PressureSweeps,
    Density, Albedo, Phase, MultiScatter, Ambient, Detail, DetailScale, Erosion, Warp, Powder,
    FlameIntensity, FlameTemperature, FlameCore, FlameTip, FlameFlicker, FlameLoss, FireLight,
    Count
};

struct GasParameterSpec
{
    GasParameter Id{};
    const char* Key = "";
    const char* Label = "";
    const char* Unit = "";
    float Default = 0.0f;
    float Minimum = 0.0f;
    float Maximum = 1.0f;
    float Step = 0.01f;
};

[[nodiscard]] const std::array<GasParameterSpec, static_cast<size_t>(GasParameter::Count)>& QueryGasParameterCatalogue() noexcept;

struct SmokeFluidSettings
{
    bool Enabled = true;
    uint8_t QualityGrade = 2;      // Low, Medium, High, Ultra
    uint8_t TestPlume = 0;         // Off, Smoke plume, Fire
    uint8_t IgnitionMode = 0;      // authored, diagnostic, off
    std::array<float, static_cast<size_t>(GasParameter::Count)> Gas{};

    SmokeFluidSettings() noexcept;
    [[nodiscard]] float Get(GasParameter Id) const noexcept;
    void Set(GasParameter Id, float Value) noexcept;
    void ResetDefaults() noexcept;
    void ResetGasDefaults() noexcept;
    void ApplyPreset(std::string_view Name) noexcept;
};

struct SmokeQualityGrade
{
    const char* Name = "High";
    std::array<int, 3> FineExtent = { 128, 128, 32 };
    float CellSizeMetres = 0.25f;
    int PressureSweeps = 16;
    int MarchSamples = 48;
};

[[nodiscard]] const std::array<SmokeQualityGrade, 4>& QuerySmokeQualityGrades() noexcept;

struct SmokeSdfWorld
{
    SmokeVector3 VehiclePosition{};
    float VehicleYawDegrees = 0.0f;
    bool VehicleEnabled = true;
    // Optional scene SDF, unioned with the ground and vehicle. The caller owns Context.
    float (*SceneDistance)(const SmokeVector3&, const void* Context) = nullptr;
    const void* SceneContext = nullptr;

    // Signed-distance union of the ground, vehicle chassis and optional scene. Positive is free space.
    [[nodiscard]] float Distance(const SmokeVector3& WorldPosition) const noexcept;
    [[nodiscard]] SmokeVector3 Normal(const SmokeVector3& WorldPosition) const noexcept;
};

struct SmokeSource
{
    SmokeVector3 Position{};
    SmokeVector3 Velocity{};
    float RadiusMetres = 0.30f;
    float SmokeRate = 0.0f;
    float HeatRate = 0.0f;
    float FlameRate = 0.0f;
    float SootRate = 0.0f;
    float RemainingSeconds = 1.0f;
    bool Active = true;
    bool Persistent = false;   // editor-controlled emitter; refreshed in place until disabled
};

struct SmokeFieldSample
{
    SmokeVector3 Velocity{};
    float Density = 0.0f;
    float Temperature = 0.0f;
    float Flame = 0.0f;
    float Soot = 0.0f;
};

struct SmokeFluidMetrics
{
    uint32_t Sources = 0;
    uint64_t Ticks = 0;
    uint32_t Relocations = 0;
    uint32_t Cells = 0;
};

class SmokeFluidSystem
{
  public:
    explicit SmokeFluidSystem(const SmokeFluidSettings& InitialSettings = {});

    void Configure(const SmokeFluidSettings& Settings);
    void Advance(float FrameSeconds, const SmokeSdfWorld& World);
    void AddSource(const SmokeSource& Source);
    void ClearPersistentSources() noexcept;
    void Clear() noexcept;

    [[nodiscard]] SmokeFieldSample Sample(const SmokeVector3& WorldPosition) const noexcept;
    [[nodiscard]] const SmokeFluidSettings& QuerySettings() const noexcept { return Settings; }
    [[nodiscard]] const SmokeFluidMetrics& QueryMetrics() const noexcept { return Metrics; }
    [[nodiscard]] const std::array<int, 3>& QueryExtent() const noexcept { return Extent; }
    [[nodiscard]] bool IsResident() const noexcept { return Resident; }
    [[nodiscard]] SmokeVector3 QueryWindowMinimum() const noexcept { return WindowMinimum; }

  private:
    struct Cell
    {
        SmokeVector3 Velocity{};
        SmokeVector3 Curl{};
        float Density = 0.0f;
        float Temperature = 0.0f;
        float Flame = 0.0f;
        float Soot = 0.0f;
        float Pressure = 0.0f;
        float Divergence = 0.0f;
        uint8_t Solid = 0;
    };

    void RebuildGrid();
    void Step(float Seconds, const SmokeSdfWorld& World);
    [[nodiscard]] size_t Index(int X, int Y, int Z) const noexcept;
    [[nodiscard]] SmokeVector3 CellCentre(int X, int Y, int Z) const noexcept;
    [[nodiscard]] SmokeFieldSample SampleGrid(const std::vector<Cell>& Field, const SmokeVector3& Position) const noexcept;
    [[nodiscard]] SmokeVector3 SampleVelocity(const std::vector<Cell>& Field, const SmokeVector3& Position) const noexcept;
    [[nodiscard]] float GaussianWeight(const SmokeSource& Source, const SmokeVector3& Position) const noexcept;

    SmokeFluidSettings Settings{};
    SmokeFluidMetrics Metrics{};
    std::array<int, 3> Extent{ 32, 32, 16 };
    std::vector<Cell> Cells{};
    std::vector<Cell> Scratch{};
    std::vector<float> PressureScratch{};
    std::vector<SmokeSource> Sources{};
    SmokeVector3 WindowMinimum{ -8.0f, -8.0f, 0.0f };
    SmokeVector3 WindowCentre{};
    float CellSize = 0.5f;
    float Accumulator = 0.0f;
    float QuietSeconds = 0.0f;
    float ElapsedSeconds = 0.0f;
    bool Resident = false;
};

} // namespace Frontier
