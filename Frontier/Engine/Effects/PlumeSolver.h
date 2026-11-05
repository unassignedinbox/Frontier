#pragma once
/*==========================================================================================================================================
                                                              PLUMESOLVER.H
==========================================================================================================================================*/
// 🧩 The fluid/gas simulation, CPU mirror of the HTML plume (PlumeSpace): an Eulerian grid carrying
//    velocity, density and heat through the same steps the WGSL performs — source injection, buoyancy,
//    vorticity confinement, semi-Lagrangian advection, dissipation/cooling, and an incompressible
//    pressure projection (divergence, Jacobi sweeps, subtract gradient). The grid presets are the HTML
//    PlumeGrades. Deterministic: the same settings and sources give the same plume, step for step,
//    which is what makes it validatable without a GPU.

#include "Frontier/Engine/Effects/GasParameters.h"

#include <array>
#include <cstdint>
#include <vector>

namespace Frontier
{

struct PlumeGrade
{
    std::string_view Name;
    std::array<int, 3> Extent;   // [cells]
    float CellExtent = 0.3f;     // [m]
    int PressureSweeps = 16;
    int MarchSamples = 40;
};

inline constexpr std::array<PlumeGrade, 4> PlumeGrades = {{
    { "Low",    { 64, 64, 20 },   0.40f, 8,  32 },
    { "Medium", { 96, 96, 24 },   0.30f, 12, 40 },
    { "High",   { 128, 128, 32 }, 0.25f, 16, 48 },
    { "Ultra",  { 160, 160, 48 }, 0.20f, 20, 64 }
}};

// 📝 A smoke/fire source: where it is, how fast the gas leaves, how much and how hot.
struct PlumeSource
{
    std::array<float, 3> PositionMetres = { 0.0f, 0.0f, 0.0f };
    std::array<float, 3> VelocityMps = { 0.0f, 0.0f, 0.0f };
    float RadiusMetres = 0.5f;
    float DensityRate = 1.0f;   // [1/s] scaled by the settings' EmissionRate/Density
    float HeatRate = 1.0f;      // [1/s] scaled by the settings' Heat
};

class PlumeSolver
{
  public:
    explicit PlumeSolver(const PlumeGrade& Grade = PlumeGrades[0]);

    [[nodiscard]] const std::array<int, 3>& QueryExtent() const noexcept { return Extent; }
    [[nodiscard]] float QueryCellExtent() const noexcept { return CellExtent; }
    [[nodiscard]] size_t QueryCellCount() const noexcept { return Density.size(); }

    void Clear() noexcept;

    // 📝 One step: sources, buoyancy, confinement, advection, decay, projection.
    void Step(float StepSeconds, const PlumeSettings& Settings, const std::vector<PlumeSource>& Sources);

    [[nodiscard]] float QueryDensity(int X, int Y, int Z) const noexcept;
    [[nodiscard]] float QueryDensityAt(const std::array<float, 3>& WorldMetres) const noexcept;
    [[nodiscard]] float QueryTotalDensity() const noexcept;
    [[nodiscard]] float QueryMeanHeat() const noexcept;
    // 📝 Mean |divergence| after projection — the incompressibility check (near zero when projected).
    [[nodiscard]] float QueryMeanDivergence() const noexcept;
    // 📝 The grid is centred on the origin in X/Y and sits on z=0.
    [[nodiscard]] std::array<float, 3> QueryCellCentre(int X, int Y, int Z) const noexcept;

  private:
    size_t Index(int X, int Y, int Z) const noexcept;
    float Sample(const std::vector<float>& Field, float X, float Y, float Z) const noexcept;

    std::array<int, 3> Extent = { 64, 64, 20 };
    float CellExtent = 0.40f;
    int GradeSweeps = 8;
    std::vector<float> VelocityX, VelocityY, VelocityZ;
    std::vector<float> Density, Heat;
    std::vector<float> Pressure, Divergence;
    std::vector<float> ScratchA, ScratchB, ScratchC;
    double ClockSeconds = 0.0;
};

} // namespace Frontier
