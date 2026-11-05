/*==========================================================================================================================================
                                                             PLUMESOLVER.CPP
==========================================================================================================================================*/

#include "Frontier/Engine/Effects/PlumeSolver.h"

#include <algorithm>
#include <cmath>

namespace Frontier
{

namespace
{
constexpr float RadiansPerDegree = 0.017453292519943295f;

float ClampAxis(float Value, int Extent) noexcept
{
    return std::clamp(Value, 0.0f, static_cast<float>(Extent - 1));
}
}

PlumeSolver::PlumeSolver(const PlumeGrade& Grade) : Extent(Grade.Extent), CellExtent(Grade.CellExtent), GradeSweeps(Grade.PressureSweeps)
{
    const size_t Count = static_cast<size_t>(Extent[0]) * Extent[1] * Extent[2];
    VelocityX.assign(Count, 0.0f); VelocityY.assign(Count, 0.0f); VelocityZ.assign(Count, 0.0f);
    Density.assign(Count, 0.0f); Heat.assign(Count, 0.0f);
    Pressure.assign(Count, 0.0f); Divergence.assign(Count, 0.0f);
    ScratchA.assign(Count, 0.0f); ScratchB.assign(Count, 0.0f); ScratchC.assign(Count, 0.0f);
}

size_t PlumeSolver::Index(int X, int Y, int Z) const noexcept
{
    X = std::clamp(X, 0, Extent[0] - 1); Y = std::clamp(Y, 0, Extent[1] - 1); Z = std::clamp(Z, 0, Extent[2] - 1);
    return (static_cast<size_t>(Z) * Extent[1] + Y) * Extent[0] + X;
}

void PlumeSolver::Clear() noexcept
{
    for (auto* Field : { &VelocityX, &VelocityY, &VelocityZ, &Density, &Heat, &Pressure, &Divergence })
        std::fill(Field->begin(), Field->end(), 0.0f);
}

std::array<float, 3> PlumeSolver::QueryCellCentre(int X, int Y, int Z) const noexcept
{
    return { (static_cast<float>(X) + 0.5f - 0.5f * Extent[0]) * CellExtent,
             (static_cast<float>(Y) + 0.5f - 0.5f * Extent[1]) * CellExtent,
             (static_cast<float>(Z) + 0.5f) * CellExtent };
}

float PlumeSolver::Sample(const std::vector<float>& Field, float X, float Y, float Z) const noexcept
{
    const int X0 = static_cast<int>(std::floor(X)), Y0 = static_cast<int>(std::floor(Y)), Z0 = static_cast<int>(std::floor(Z));
    const float Fx = X - X0, Fy = Y - Y0, Fz = Z - Z0;
    float Total = 0.0f;
    for (int Dz = 0; Dz <= 1; ++Dz)
        for (int Dy = 0; Dy <= 1; ++Dy)
            for (int Dx = 0; Dx <= 1; ++Dx)
            {
                const float Weight = (Dx ? Fx : 1.0f - Fx) * (Dy ? Fy : 1.0f - Fy) * (Dz ? Fz : 1.0f - Fz);
                Total += Weight * Field[Index(X0 + Dx, Y0 + Dy, Z0 + Dz)];
            }
    return Total;
}

void PlumeSolver::Step(float StepSeconds, const PlumeSettings& Settings, const std::vector<PlumeSource>& Sources)
{
    if (!Settings.Enabled || StepSeconds <= 0.0f)
        return;
    ClockSeconds += StepSeconds;
    const float Dt = StepSeconds;
    const int Sweeps = std::max(1, Settings.PressureSweeps);

    // ① sources: splat density, heat and carrier velocity inside the source radius.
    for (const PlumeSource& Source : Sources)
    {
        const int Radius = std::max(1, static_cast<int>(std::ceil(Source.RadiusMetres / CellExtent)));
        const std::array<float, 3> Centre = {
            Source.PositionMetres[0] / CellExtent + 0.5f * Extent[0] - 0.5f,
            Source.PositionMetres[1] / CellExtent + 0.5f * Extent[1] - 0.5f,
            Source.PositionMetres[2] / CellExtent - 0.5f };
        const int Cx = static_cast<int>(std::round(Centre[0])), Cy = static_cast<int>(std::round(Centre[1])), Cz = static_cast<int>(std::round(Centre[2]));
        for (int Z = Cz - Radius; Z <= Cz + Radius; ++Z)
            for (int Y = Cy - Radius; Y <= Cy + Radius; ++Y)
                for (int X = Cx - Radius; X <= Cx + Radius; ++X)
                {
                    const float Distance2 = static_cast<float>((X - Cx) * (X - Cx) + (Y - Cy) * (Y - Cy) + (Z - Cz) * (Z - Cz));
                    if (Distance2 > static_cast<float>(Radius * Radius))
                        continue;
                    const float Weight = 1.0f - std::sqrt(Distance2) / static_cast<float>(Radius + 1);
                    const size_t At = Index(X, Y, Z);
                    Density[At] += Source.DensityRate * Settings.EmissionRate * Settings.Density * Weight * Dt;
                    Heat[At] += Source.HeatRate * Settings.Heat * Weight * Dt;
                    VelocityX[At] += Source.VelocityMps[0] * Weight * Dt;
                    VelocityY[At] += Source.VelocityMps[1] * Weight * Dt;
                    VelocityZ[At] += Source.VelocityMps[2] * Weight * Dt;
                }
    }

    // ② buoyancy + wind: hot gas rises; the wind carries everything.
    const float Heading = Settings.WindHeadingDegrees * RadiansPerDegree;
    const float WindX = std::cos(Heading) * Settings.WindSpeed, WindY = std::sin(Heading) * Settings.WindSpeed;
    for (size_t At = 0; At < Density.size(); ++At)
    {
        VelocityZ[At] += Settings.Buoyancy * Heat[At] * Dt;
        VelocityX[At] += WindX * 0.05f * Dt;
        VelocityY[At] += WindY * 0.05f * Dt;
    }

    // ③ vorticity confinement: curl, then push velocity along ∇|curl| × curl (the same ε rule as the WGSL).
    if (Settings.Vorticity > 0.0f)
    {
        auto& CurlX = ScratchA; auto& CurlY = ScratchB; auto& CurlZ = ScratchC;
        const float Inv = 0.5f / CellExtent;
        for (int Z = 0; Z < Extent[2]; ++Z)
            for (int Y = 0; Y < Extent[1]; ++Y)
                for (int X = 0; X < Extent[0]; ++X)
                {
                    const size_t At = Index(X, Y, Z);
                    CurlX[At] = (VelocityZ[Index(X, Y + 1, Z)] - VelocityZ[Index(X, Y - 1, Z)]
                                 - (VelocityY[Index(X, Y, Z + 1)] - VelocityY[Index(X, Y, Z - 1)])) * Inv;
                    CurlY[At] = (VelocityX[Index(X, Y, Z + 1)] - VelocityX[Index(X, Y, Z - 1)]
                                 - (VelocityZ[Index(X + 1, Y, Z)] - VelocityZ[Index(X - 1, Y, Z)])) * Inv;
                    CurlZ[At] = (VelocityY[Index(X + 1, Y, Z)] - VelocityY[Index(X - 1, Y, Z)]
                                 - (VelocityX[Index(X, Y + 1, Z)] - VelocityX[Index(X, Y - 1, Z)])) * Inv;
                }
        auto CurlMagnitude = [&](int X, int Y, int Z) {
            const size_t At = Index(X, Y, Z);
            return std::sqrt(CurlX[At] * CurlX[At] + CurlY[At] * CurlY[At] + CurlZ[At] * CurlZ[At]);
        };
        for (int Z = 0; Z < Extent[2]; ++Z)
            for (int Y = 0; Y < Extent[1]; ++Y)
                for (int X = 0; X < Extent[0]; ++X)
                {
                    const size_t At = Index(X, Y, Z);
                    const float Gx = (CurlMagnitude(X + 1, Y, Z) - CurlMagnitude(X - 1, Y, Z)) * Inv;
                    const float Gy = (CurlMagnitude(X, Y + 1, Z) - CurlMagnitude(X, Y - 1, Z)) * Inv;
                    const float Gz = (CurlMagnitude(X, Y, Z + 1) - CurlMagnitude(X, Y, Z - 1)) * Inv;
                    const float Gl = std::sqrt(Gx * Gx + Gy * Gy + Gz * Gz) + 1e-6f;
                    const float Nx = Gx / Gl, Ny = Gy / Gl, Nz = Gz / Gl;
                    VelocityX[At] += Settings.Vorticity * (Ny * CurlZ[At] - Nz * CurlY[At]) * CellExtent * Dt;
                    VelocityY[At] += Settings.Vorticity * (Nz * CurlX[At] - Nx * CurlZ[At]) * CellExtent * Dt;
                    VelocityZ[At] += Settings.Vorticity * (Nx * CurlY[At] - Ny * CurlX[At]) * CellExtent * Dt;
                }
    }

    // ④ semi-Lagrangian advection of velocity, density and heat (plus deterministic turbulence).
    const auto Advect = [&](const std::vector<float>& Field, std::vector<float>& Out) {
        for (int Z = 0; Z < Extent[2]; ++Z)
            for (int Y = 0; Y < Extent[1]; ++Y)
                for (int X = 0; X < Extent[0]; ++X)
                {
                    const size_t At = Index(X, Y, Z);
                    const std::array<float, 3> World = QueryCellCentre(X, Y, Z);
                    const float TurbX = Settings.Turbulence * std::sin(World[1] * Settings.TurbulenceScale + static_cast<float>(ClockSeconds) * 0.7f);
                    const float TurbY = Settings.Turbulence * std::cos(World[0] * Settings.TurbulenceScale - static_cast<float>(ClockSeconds) * 0.6f);
                    const float BackX = ClampAxis(X - (VelocityX[At] + TurbX * 0.1f) * Dt / CellExtent, Extent[0]);
                    const float BackY = ClampAxis(Y - (VelocityY[At] + TurbY * 0.1f) * Dt / CellExtent, Extent[1]);
                    const float BackZ = ClampAxis(Z - VelocityZ[At] * Dt / CellExtent, Extent[2]);
                    Out[At] = Sample(Field, BackX, BackY, BackZ);
                }
    };
    {
        std::vector<float> NextX = VelocityX, NextY = VelocityY, NextZ = VelocityZ;
        Advect(VelocityX, ScratchA); std::copy(ScratchA.begin(), ScratchA.end(), NextX.begin());
        Advect(VelocityY, ScratchA); std::copy(ScratchA.begin(), ScratchA.end(), NextY.begin());
        Advect(VelocityZ, ScratchA); std::copy(ScratchA.begin(), ScratchA.end(), NextZ.begin());
        VelocityX.swap(NextX); VelocityY.swap(NextY); VelocityZ.swap(NextZ);
        Advect(Density, ScratchA); Density.swap(ScratchA);
        Advect(Heat, ScratchA); Heat.swap(ScratchA);
    }

    // ⑤ decay: density dissipates, heat cools (exponential, so rates are per-second as catalogued).
    const float DensityKeep = std::exp(-Settings.Dissipation * Dt);
    const float HeatKeep = std::exp(-Settings.Cooling * Dt);
    for (size_t At = 0; At < Density.size(); ++At)
    {
        Density[At] *= DensityKeep;
        Heat[At] *= HeatKeep;
    }

    // ⑥ pressure projection: divergence, Jacobi sweeps, subtract the gradient.
    const float InvCell = 1.0f / CellExtent;
    for (int Z = 0; Z < Extent[2]; ++Z)
        for (int Y = 0; Y < Extent[1]; ++Y)
            for (int X = 0; X < Extent[0]; ++X)
            {
                const size_t At = Index(X, Y, Z);
                Divergence[At] = -0.5f * ((VelocityX[Index(X + 1, Y, Z)] - VelocityX[Index(X - 1, Y, Z)])
                                          + (VelocityY[Index(X, Y + 1, Z)] - VelocityY[Index(X, Y - 1, Z)])
                                          + (VelocityZ[Index(X, Y, Z + 1)] - VelocityZ[Index(X, Y, Z - 1)])) * InvCell;
                Pressure[At] = 0.0f;
            }
    auto& NextPressure = ScratchA;
    for (int Sweep = 0; Sweep < Sweeps; ++Sweep)
    {
        for (int Z = 0; Z < Extent[2]; ++Z)
            for (int Y = 0; Y < Extent[1]; ++Y)
                for (int X = 0; X < Extent[0]; ++X)
                {
                    const size_t At = Index(X, Y, Z);
                    // 📝 Jacobi for ∇²p = ∇·v on a grid of spacing h: the divergence term carries h².
                    //    Without it the gradient subtraction over-corrects by 1/h² and the field explodes.
                    NextPressure[At] = (Divergence[At] * CellExtent * CellExtent + Pressure[Index(X - 1, Y, Z)] + Pressure[Index(X + 1, Y, Z)]
                                        + Pressure[Index(X, Y - 1, Z)] + Pressure[Index(X, Y + 1, Z)]
                                        + Pressure[Index(X, Y, Z - 1)] + Pressure[Index(X, Y, Z + 1)]) / 6.0f;
                }
        Pressure.swap(NextPressure);
    }
    for (int Z = 0; Z < Extent[2]; ++Z)
        for (int Y = 0; Y < Extent[1]; ++Y)
            for (int X = 0; X < Extent[0]; ++X)
            {
                const size_t At = Index(X, Y, Z);
                VelocityX[At] -= 0.5f * (Pressure[Index(X + 1, Y, Z)] - Pressure[Index(X - 1, Y, Z)]) * InvCell;
                VelocityY[At] -= 0.5f * (Pressure[Index(X, Y + 1, Z)] - Pressure[Index(X, Y - 1, Z)]) * InvCell;
                VelocityZ[At] -= 0.5f * (Pressure[Index(X, Y, Z + 1)] - Pressure[Index(X, Y, Z - 1)]) * InvCell;
            }
}

float PlumeSolver::QueryMaximumSpeed() const noexcept
{
    float Maximum = 0.0f;
    for (size_t At = 0; At < Density.size(); ++At)
        Maximum = std::max(Maximum, VelocityX[At] * VelocityX[At] + VelocityY[At] * VelocityY[At] + VelocityZ[At] * VelocityZ[At]);
    return std::sqrt(Maximum);
}

float PlumeSolver::QueryDensity(int X, int Y, int Z) const noexcept
{
    return Density[Index(X, Y, Z)];
}

float PlumeSolver::QueryHeat(int X, int Y, int Z) const noexcept
{
    if (X < 0 || Y < 0 || Z < 0 || X >= Extent[0] || Y >= Extent[1] || Z >= Extent[2])
        return 0.0f;
    return Heat[Index(X, Y, Z)];
}

float PlumeSolver::QueryDensityAt(const std::array<float, 3>& WorldMetres) const noexcept
{
    return Sample(Density, WorldMetres[0] / CellExtent + 0.5f * Extent[0] - 0.5f,
                  WorldMetres[1] / CellExtent + 0.5f * Extent[1] - 0.5f,
                  WorldMetres[2] / CellExtent - 0.5f);
}

float PlumeSolver::QueryTotalDensity() const noexcept
{
    double Total = 0.0;
    for (const float Value : Density)
        Total += Value;
    return static_cast<float>(Total);
}

float PlumeSolver::QueryMeanHeat() const noexcept
{
    if (Heat.empty())
        return 0.0f;
    double Total = 0.0;
    for (const float Value : Heat)
        Total += Value;
    return static_cast<float>(Total / static_cast<double>(Heat.size()));
}

float PlumeSolver::QueryMeanDivergence() const noexcept
{
    // Divergence of the current (projected) velocity, recomputed — the projection's report card.
    double Total = 0.0;
    const float InvCell = 1.0f / CellExtent;
    for (int Z = 1; Z < Extent[2] - 1; ++Z)
        for (int Y = 1; Y < Extent[1] - 1; ++Y)
            for (int X = 1; X < Extent[0] - 1; ++X)
            {
                const float Div = 0.5f * ((VelocityX[Index(X + 1, Y, Z)] - VelocityX[Index(X - 1, Y, Z)])
                                          + (VelocityY[Index(X, Y + 1, Z)] - VelocityY[Index(X, Y - 1, Z)])
                                          + (VelocityZ[Index(X, Y, Z + 1)] - VelocityZ[Index(X, Y, Z - 1)])) * InvCell;
                Total += std::fabs(Div);
            }
    const size_t Interior = static_cast<size_t>(Extent[0] - 2) * (Extent[1] - 2) * (Extent[2] - 2);
    return Interior ? static_cast<float>(Total / static_cast<double>(Interior)) : 0.0f;
}

} // namespace Frontier
