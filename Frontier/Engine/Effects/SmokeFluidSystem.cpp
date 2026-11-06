/*==========================================================================================================================================
                                                         SMOKEFLUIDSYSTEM.CPP
==========================================================================================================================================*/
// 🧩 CPU reference port of the exhibit's plume passes: semi-Lagrangian transport, buoyancy, vorticity,
//    warm-start pressure projection, combustion, SDF-solid boundaries and scalar source injection. The
//    WGSL files remain the GPU reference; this path makes the native C++/GLFW editor self-contained.

#include "Frontier/Engine/Effects/SmokeFluidSystem.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace Frontier
{

namespace
{
constexpr float Pi = 3.14159265358979323846f;
constexpr float StepMinimum = 1.0f / 60.0f;
constexpr float FadeCeiling = 12.0f;
constexpr size_t MaximumSources = 24;
constexpr size_t GasIndex(GasParameter Id) { return static_cast<size_t>(Id); }

float Clamp01(float Value) { return std::clamp(Value, 0.0f, 1.0f); }
float Length(const SmokeVector3& V) { return std::sqrt(SmokeDot(V, V)); }
SmokeVector3 Cross(const SmokeVector3& A, const SmokeVector3& B)
{
    return { A.Y * B.Z - A.Z * B.Y, A.Z * B.X - A.X * B.Z, A.X * B.Y - A.Y * B.X };
}
SmokeVector3 Abs(const SmokeVector3& V) { return { std::fabs(V.X), std::fabs(V.Y), std::fabs(V.Z) }; }
SmokeVector3 Max(const SmokeVector3& V, float S) { return { std::max(V.X, S), std::max(V.Y, S), std::max(V.Z, S) }; }

const std::array<GasParameterSpec, static_cast<size_t>(GasParameter::Count)> Catalogue = {{
    { GasParameter::Emission, "Emission", "Emission rate", "×", 1.0f, 0.0f, 4.0f, 0.05f },
    { GasParameter::Heat, "Heat", "Temperature", "×", 1.0f, 0.0f, 4.0f, 0.05f },
    { GasParameter::Expansion, "Expansion", "Volume expansion", "×", 1.0f, 0.0f, 3.0f, 0.05f },
    { GasParameter::Soot, "Soot", "Soot yield", "×", 1.0f, 0.0f, 4.0f, 0.05f },
    { GasParameter::Momentum, "Momentum", "Carrier drag", "1/s", 9.0f, 0.0f, 24.0f, 0.5f },
    { GasParameter::TreadEjection, "TreadEjection", "Tread ejection", "×", 1.0f, 0.0f, 3.0f, 0.05f },
    { GasParameter::Buoyancy, "Buoyancy", "Buoyancy", "m/s²·K", 2.4f, 0.0f, 8.0f, 0.1f },
    { GasParameter::Dissipation, "Dissipation", "Dissipation", "1/s", 0.10f, 0.0f, 0.9f, 0.01f },
    { GasParameter::Cooling, "Cooling", "Cooling", "1/s", 0.55f, 0.0f, 0.95f, 0.01f },
    { GasParameter::Vorticity, "Vorticity", "Vorticity confinement", "ε", 3.0f, 0.0f, 6.0f, 0.1f },
    { GasParameter::Turbulence, "Turbulence", "Turbulence", "m/s", 0.60f, 0.0f, 2.0f, 0.02f },
    { GasParameter::TurbulenceScale, "TurbulenceScale", "Turbulence frequency", "1/m", 3.7f, 0.2f, 4.0f, 0.05f },
    { GasParameter::WindSpeed, "WindSpeed", "Wind speed", "m/s", 0.8f, 0.0f, 12.0f, 0.1f },
    { GasParameter::WindHeading, "WindHeading", "Wind heading", "°", 30.0f, 0.0f, 360.0f, 1.0f },
    { GasParameter::TimeScale, "TimeScale", "Time scale", "×", 1.0f, 0.25f, 2.0f, 0.05f },
    { GasParameter::PressureSweeps, "PressureSweeps", "Pressure iterations", "", 16.0f, 4.0f, 40.0f, 2.0f },
    { GasParameter::Density, "Density", "Density", "×", 1.0f, 0.2f, 3.0f, 0.05f },
    { GasParameter::Albedo, "Albedo", "Albedo", "", 0.88f, 0.2f, 0.98f, 0.01f },
    { GasParameter::Phase, "Phase", "Forward scattering", "g", 0.38f, 0.0f, 0.9f, 0.01f },
    { GasParameter::MultiScatter, "MultiScatter", "Multiple scattering", "", 0.75f, 0.0f, 1.5f, 0.05f },
    { GasParameter::Ambient, "Ambient", "Ambient light", "×", 1.0f, 0.0f, 3.0f, 0.05f },
    { GasParameter::Detail, "Detail", "Detail", "", 0.70f, 0.0f, 1.0f, 0.02f },
    { GasParameter::DetailScale, "DetailScale", "Detail frequency", "1/m", 2.2f, 0.2f, 4.0f, 0.05f },
    { GasParameter::Erosion, "Erosion", "Edge erosion", "", 1.0f, 0.0f, 1.5f, 0.05f },
    { GasParameter::Warp, "Warp", "Billow warp", "", 0.0f, 0.0f, 1.0f, 0.05f },
    { GasParameter::Powder, "Powder", "Powder", "", 0.0f, 0.0f, 1.0f, 0.05f },
    { GasParameter::FlameIntensity, "FlameIntensity", "Flame intensity", "×", 1.0f, 0.0f, 4.0f, 0.05f },
    { GasParameter::FlameTemperature, "FlameTemperature", "Flame temperature", "×", 1.0f, 0.7f, 1.6f, 0.01f },
    { GasParameter::FlameCore, "FlameCore", "Flame core", "K", 1800.0f, 1300.0f, 2400.0f, 10.0f },
    { GasParameter::FlameTip, "FlameTip", "Flame tip", "K", 1150.0f, 900.0f, 1600.0f, 10.0f },
    { GasParameter::FlameFlicker, "FlameFlicker", "Flicker", "", 0.6f, 0.0f, 1.0f, 0.02f },
    { GasParameter::FlameLoss, "FlameLoss", "Flame decay", "1/s", 0.96f, 0.5f, 0.995f, 0.005f },
    { GasParameter::FireLight, "FireLight", "Fire light", "×", 1.0f, 0.0f, 4.0f, 0.05f }
}};

const std::array<SmokeQualityGrade, 4> Grades = {{
    { "Low", { 64, 64, 20 }, 0.40f, 8, 32 },
    { "Medium", { 96, 96, 24 }, 0.30f, 12, 40 },
    { "High", { 128, 128, 32 }, 0.25f, 16, 48 },
    { "Ultra", { 160, 160, 48 }, 0.20f, 20, 64 }
}};

} // namespace

SmokeVector3 SmokeVector3::operator+(const SmokeVector3& Other) const noexcept { return { X + Other.X, Y + Other.Y, Z + Other.Z }; }
SmokeVector3 SmokeVector3::operator-(const SmokeVector3& Other) const noexcept { return { X - Other.X, Y - Other.Y, Z - Other.Z }; }
SmokeVector3 SmokeVector3::operator*(float Scale) const noexcept { return { X * Scale, Y * Scale, Z * Scale }; }
float SmokeDot(const SmokeVector3& A, const SmokeVector3& B) noexcept { return A.X * B.X + A.Y * B.Y + A.Z * B.Z; }
SmokeVector3 SmokeNormalize(const SmokeVector3& V) noexcept
{
    const float Size = Length(V);
    return Size > 1e-6f ? V * (1.0f / Size) : SmokeVector3{};
}

const std::array<GasParameterSpec, static_cast<size_t>(GasParameter::Count)>& QueryGasParameterCatalogue() noexcept { return Catalogue; }
const std::array<SmokeQualityGrade, 4>& QuerySmokeQualityGrades() noexcept { return Grades; }

SmokeFluidSettings::SmokeFluidSettings() noexcept { ResetDefaults(); }
float SmokeFluidSettings::Get(GasParameter Id) const noexcept { return Gas[GasIndex(Id)]; }
void SmokeFluidSettings::Set(GasParameter Id, float Value) noexcept
{
    const auto& Spec = Catalogue[GasIndex(Id)];
    Value = std::clamp(Value, Spec.Minimum, Spec.Maximum);
    if (Id == GasParameter::PressureSweeps)
        Value = 2.0f * std::round(Value * 0.5f);
    Gas[GasIndex(Id)] = Value;
}
void SmokeFluidSettings::ResetDefaults() noexcept
{
    Enabled = true;
    QualityGrade = 2;
    TestPlume = 0;
    IgnitionMode = 0;
    ResetGasDefaults();
}
void SmokeFluidSettings::ResetGasDefaults() noexcept
{
    for (const GasParameterSpec& Spec : Catalogue)
        Gas[GasIndex(Spec.Id)] = Spec.Default;
}
void SmokeFluidSettings::ApplyPreset(std::string_view Name) noexcept
{
    // Presets replace gas parameters only; the emitter, quality and ignition remain unchanged.
    ResetGasDefaults();
    if (Name == "Thick burnout")
    {
        Set(GasParameter::Emission, 2.2f); Set(GasParameter::Density, 1.6f); Set(GasParameter::Dissipation, 0.12f);
        Set(GasParameter::Albedo, 0.9f); Set(GasParameter::Vorticity, 2.2f); Set(GasParameter::Expansion, 1.4f);
        Set(GasParameter::MultiScatter, 0.9f);
    }
    else if (Name == "Light haze")
    {
        Set(GasParameter::Emission, 0.6f); Set(GasParameter::Density, 0.55f); Set(GasParameter::Dissipation, 0.35f);
        Set(GasParameter::Detail, 0.75f); Set(GasParameter::Erosion, 0.9f); Set(GasParameter::Albedo, 0.92f);
    }
    else if (Name == "Inferno")
    {
        Set(GasParameter::FlameIntensity, 2.4f); Set(GasParameter::FlameTemperature, 1.18f);
        Set(GasParameter::FlameFlicker, 0.85f); Set(GasParameter::Soot, 2.2f); Set(GasParameter::Heat, 1.8f);
        Set(GasParameter::FireLight, 2.2f); Set(GasParameter::Buoyancy, 3.4f);
    }
    else if (Name == "Crosswind")
    {
        Set(GasParameter::WindSpeed, 5.5f); Set(GasParameter::WindHeading, 95.0f);
        Set(GasParameter::Turbulence, 0.7f); Set(GasParameter::Dissipation, 0.28f);
    }
    else if (Name == "Legacy billows")
    {
        Set(GasParameter::Density, 1.0f); Set(GasParameter::Dissipation, 0.22f); Set(GasParameter::Vorticity, 1.6f);
        Set(GasParameter::Turbulence, 0.4f); Set(GasParameter::TurbulenceScale, 1.1f); Set(GasParameter::Albedo, 0.86f);
        Set(GasParameter::Phase, 0.45f); Set(GasParameter::MultiScatter, 0.45f); Set(GasParameter::Detail, 0.65f);
        Set(GasParameter::DetailScale, 0.85f); Set(GasParameter::Erosion, 0.75f); Set(GasParameter::Warp, 0.35f);
        Set(GasParameter::Powder, 0.2f); Set(GasParameter::WindSpeed, 0.0f);
    }
}

float SmokeSdfWorld::Distance(const SmokeVector3& P) const noexcept
{
    const float Ground = SceneDistance ? std::min(P.Z, SceneDistance(P, SceneContext)) : P.Z;
    if (!VehicleEnabled)
        return Ground;
    const float Yaw = VehicleYawDegrees * Pi / 180.0f;
    const SmokeVector3 Relative = P - (VehiclePosition + SmokeVector3{ 0.0f, 0.0f, 0.58f });
    const SmokeVector3 Local{ Relative.X * std::cos(Yaw) + Relative.Y * std::sin(Yaw),
                              -Relative.X * std::sin(Yaw) + Relative.Y * std::cos(Yaw), Relative.Z };
    const SmokeVector3 Q = Abs(Local) - SmokeVector3{ 2.10f, 0.95f, 0.58f };
    const SmokeVector3 Outside = Max(Q, 0.0f);
    const float Box = Length(Outside) + std::min(std::max(Q.X, std::max(Q.Y, Q.Z)), 0.0f);
    return std::min(Ground, Box);
}
SmokeVector3 SmokeSdfWorld::Normal(const SmokeVector3& P) const noexcept
{
    constexpr float E = 0.025f;
    return SmokeNormalize({ Distance(P + SmokeVector3{ E, 0, 0 }) - Distance(P - SmokeVector3{ E, 0, 0 }),
                            Distance(P + SmokeVector3{ 0, E, 0 }) - Distance(P - SmokeVector3{ 0, E, 0 }),
                            Distance(P + SmokeVector3{ 0, 0, E }) - Distance(P - SmokeVector3{ 0, 0, E }) });
}

SmokeFluidSystem::SmokeFluidSystem(const SmokeFluidSettings& InitialSettings) { Configure(InitialSettings); }
void SmokeFluidSystem::Configure(const SmokeFluidSettings& Updated)
{
    const bool GradeChanged = Settings.QualityGrade != Updated.QualityGrade || Cells.empty();
    Settings = Updated;
    Settings.QualityGrade = std::min<uint8_t>(Settings.QualityGrade, 3);
    if (GradeChanged) RebuildGrid();
}
void SmokeFluidSystem::RebuildGrid()
{
    const SmokeQualityGrade& Grade = Grades[Settings.QualityGrade];
    // 📝 GPU velocity lattice is half the fine thermodynamic resolution. The native CPU reference keeps
    //    a conservative 1/2-sized window in each horizontal dimension for interactive preview speed.
    Extent = { std::max(16, Grade.FineExtent[0] / 2), std::max(16, Grade.FineExtent[1] / 2), Grade.FineExtent[2] / 2 };
    CellSize = 2.0f * Grade.CellSizeMetres;
    const size_t Count = static_cast<size_t>(Extent[0]) * Extent[1] * Extent[2];
    Cells.assign(Count, Cell{});
    Scratch.assign(Count, Cell{});
    PressureScratch.assign(Count, 0.0f);
    WindowCentre = { 0.0f, 0.0f, Extent[2] * CellSize * 0.5f };
    WindowMinimum = { WindowCentre.X - Extent[0] * CellSize * 0.5f,
                      WindowCentre.Y - Extent[1] * CellSize * 0.5f, 0.0f };
    Metrics.Cells = static_cast<uint32_t>(Count);
    Metrics.Relocations++;
    Accumulator = 0.0f;
    Resident = !Sources.empty();
    QuietSeconds = 0.0f;
}

size_t SmokeFluidSystem::Index(int X, int Y, int Z) const noexcept
{
    X = std::clamp(X, 0, Extent[0] - 1); Y = std::clamp(Y, 0, Extent[1] - 1); Z = std::clamp(Z, 0, Extent[2] - 1);
    return (static_cast<size_t>(Z) * Extent[1] + Y) * Extent[0] + X;
}
SmokeVector3 SmokeFluidSystem::CellCentre(int X, int Y, int Z) const noexcept
{
    return { WindowMinimum.X + (X + 0.5f) * CellSize, WindowMinimum.Y + (Y + 0.5f) * CellSize,
             WindowMinimum.Z + (Z + 0.5f) * CellSize };
}
void SmokeFluidSystem::AddSource(const SmokeSource& Source)
{
    SmokeSource Copy = Source;
    Copy.RadiusMetres = std::max(Copy.RadiusMetres, CellSize * 0.5f);
    if (Copy.Persistent)
    {
        const auto Existing = std::find_if(Sources.begin(), Sources.end(), [](const SmokeSource& S) { return S.Persistent; });
        if (Existing != Sources.end()) { *Existing = Copy; return; }
    }
    if (Sources.size() >= MaximumSources)
        Sources.erase(Sources.begin());
    Sources.push_back(Copy);
    Resident = true;
    QuietSeconds = 0.0f;
}
void SmokeFluidSystem::ClearPersistentSources() noexcept
{
    Sources.erase(std::remove_if(Sources.begin(), Sources.end(), [](const SmokeSource& S) { return S.Persistent; }), Sources.end());
}
void SmokeFluidSystem::Clear() noexcept
{
    std::fill(Cells.begin(), Cells.end(), Cell{});
    std::fill(Scratch.begin(), Scratch.end(), Cell{});
    std::fill(PressureScratch.begin(), PressureScratch.end(), 0.0f);
    Sources.clear();
    Resident = false;
    QuietSeconds = 0.0f;
    Accumulator = 0.0f;
    Metrics.Sources = 0;
}
float SmokeFluidSystem::GaussianWeight(const SmokeSource& Source, const SmokeVector3& P) const noexcept
{
    const SmokeVector3 D = P - Source.Position;
    const float R = std::max(Source.RadiusMetres, CellSize * 0.5f);
    const float Q = SmokeDot(D, D) / (2.0f * R * R);
    return Q > 6.125f ? 0.0f : std::exp(-Q);
}

SmokeFieldSample SmokeFluidSystem::SampleGrid(const std::vector<Cell>& Field, const SmokeVector3& P) const noexcept
{
    SmokeFieldSample Out{};
    if (Field.empty()) return Out;
    const float X = (P.X - WindowMinimum.X) / CellSize - 0.5f;
    const float Y = (P.Y - WindowMinimum.Y) / CellSize - 0.5f;
    const float Z = (P.Z - WindowMinimum.Z) / CellSize - 0.5f;
    const int X0 = static_cast<int>(std::floor(X)), Y0 = static_cast<int>(std::floor(Y)), Z0 = static_cast<int>(std::floor(Z));
    const float Tx = X - X0, Ty = Y - Y0, Tz = Z - Z0;
    for (int Dz = 0; Dz < 2; ++Dz) for (int Dy = 0; Dy < 2; ++Dy) for (int Dx = 0; Dx < 2; ++Dx)
    {
        const float W = (Dx ? Tx : 1.0f - Tx) * (Dy ? Ty : 1.0f - Ty) * (Dz ? Tz : 1.0f - Tz);
        const Cell& C = Field[Index(X0 + Dx, Y0 + Dy, Z0 + Dz)];
        Out.Velocity = Out.Velocity + C.Velocity * W;
        Out.Density += C.Density * W; Out.Temperature += C.Temperature * W; Out.Flame += C.Flame * W; Out.Soot += C.Soot * W;
    }
    return Out;
}
SmokeVector3 SmokeFluidSystem::SampleVelocity(const std::vector<Cell>& Field, const SmokeVector3& P) const noexcept
{
    return SampleGrid(Field, P).Velocity;
}
SmokeFieldSample SmokeFluidSystem::Sample(const SmokeVector3& P) const noexcept
{
    if (P.X < WindowMinimum.X || P.Y < WindowMinimum.Y || P.Z < WindowMinimum.Z ||
        P.X >= WindowMinimum.X + Extent[0] * CellSize || P.Y >= WindowMinimum.Y + Extent[1] * CellSize ||
        P.Z >= WindowMinimum.Z + Extent[2] * CellSize)
        return {};
    return SampleGrid(Cells, P);
}

void SmokeFluidSystem::Advance(float FrameSeconds, const SmokeSdfWorld& World)
{
    const float Seconds = std::clamp(FrameSeconds, 0.0f, 0.1f);
    const SmokeVector3 Vehicle = World.VehiclePosition;
    const float SpanX = Extent[0] * CellSize, SpanY = Extent[1] * CellSize;
    if (std::fabs(Vehicle.X - WindowCentre.X) > SpanX * 0.25f || std::fabs(Vehicle.Y - WindowCentre.Y) > SpanY * 0.25f)
    {
        const SmokeVector3 NewCentre{ Vehicle.X, Vehicle.Y, WindowCentre.Z };
        const SmokeVector3 NewMin{ NewCentre.X - SpanX * 0.5f, NewCentre.Y - SpanY * 0.5f, 0.0f };
        const int ShiftX = static_cast<int>(std::lround((NewMin.X - WindowMinimum.X) / CellSize));
        const int ShiftY = static_cast<int>(std::lround((NewMin.Y - WindowMinimum.Y) / CellSize));
        std::vector<Cell> Shifted(Cells.size());
        for (int Z = 0; Z < Extent[2]; ++Z) for (int Y = 0; Y < Extent[1]; ++Y) for (int X = 0; X < Extent[0]; ++X)
        {
            const int OldX = X + ShiftX, OldY = Y + ShiftY;
            if (OldX >= 0 && OldX < Extent[0] && OldY >= 0 && OldY < Extent[1])
                Shifted[Index(X, Y, Z)] = Cells[Index(OldX, OldY, Z)];
        }
        Cells.swap(Shifted);
        std::fill(PressureScratch.begin(), PressureScratch.end(), 0.0f);
        WindowCentre = NewCentre; WindowMinimum = NewMin; ++Metrics.Relocations;
    }

    Accumulator += Seconds * Settings.Get(GasParameter::TimeScale);
    while (Accumulator >= StepMinimum)
    {
        Accumulator -= StepMinimum;
        if (Resident || !Sources.empty())
            Step(StepMinimum, World);
    }
    Metrics.Sources = static_cast<uint32_t>(std::count_if(Sources.begin(), Sources.end(), [](const SmokeSource& S) { return S.Active; }));
}

void SmokeFluidSystem::Step(float Seconds, const SmokeSdfWorld& World)
{
    const float WindHeading = Settings.Get(GasParameter::WindHeading) * Pi / 180.0f;
    const SmokeVector3 Wind{ std::cos(WindHeading) * Settings.Get(GasParameter::WindSpeed),
                             std::sin(WindHeading) * Settings.Get(GasParameter::WindSpeed), 0.0f };
    const float Drag = std::exp(-Settings.Get(GasParameter::Momentum) * Seconds);
    const float DensityLoss = std::exp(-Settings.Get(GasParameter::Dissipation) * Seconds);
    const float HeatLoss = std::exp(-Settings.Get(GasParameter::Cooling) * Seconds);
    const float FlameLoss = std::exp(-std::max(0.0f, -std::log(std::max(0.005f, 1.0f - Settings.Get(GasParameter::FlameLoss)))) * Seconds);
    const SmokeVector3 VehicleVelocity = World.VehiclePosition - WindowCentre;
    (void)VehicleVelocity;

    // ① mid-point semi-Lagrangian velocity advection + wind, buoyancy and a curl-noise force.
    for (int Z = 0; Z < Extent[2]; ++Z) for (int Y = 0; Y < Extent[1]; ++Y) for (int X = 0; X < Extent[0]; ++X)
    {
        const size_t I = Index(X, Y, Z);
        const SmokeVector3 P = CellCentre(X, Y, Z);
        Cell Out = Cells[I];
        Out.Solid = World.Distance(P) < CellSize * 0.35f ? 1 : 0;
        if (Out.Solid)
        {
            Out.Velocity = {};
            Out.Density = Out.Temperature = Out.Flame = Out.Soot = 0.0f;
            Scratch[I] = Out;
            continue;
        }
        const SmokeVector3 V0 = Cells[I].Velocity;
        const SmokeVector3 Mid = P - V0 * (0.5f * Seconds);
        SmokeVector3 V = SampleVelocity(Cells, P - SampleVelocity(Cells, Mid) * Seconds) * Drag + Wind * (1.0f - Drag);
        const float A = Settings.Get(GasParameter::TurbulenceScale);
        const SmokeVector3 CurlNoise{ std::sin(P.Y * A + ElapsedSeconds) - std::sin(P.Z * A),
                                      std::sin(P.Z * A + 0.7f) - std::sin(P.X * A + ElapsedSeconds),
                                      std::sin(P.X * A + 1.3f) - std::sin(P.Y * A + 0.5f) };
        V = V + CurlNoise * (Settings.Get(GasParameter::Turbulence) * 0.15f);
        V.Z += Settings.Get(GasParameter::Buoyancy) * std::min(Cells[I].Temperature / 1000.0f, 3.0f) * Seconds;
        if (Settings.Enabled)
            for (const SmokeSource& Source : Sources)
                if (Source.Active)
                {
                    const float Coupling = GaussianWeight(Source, P) * (1.0f - Drag);
                    V = V * (1.0f - Coupling) + Source.Velocity * Coupling;
                }
        Out.Velocity = V;
        Scratch[I] = Out;
    }
    Cells.swap(Scratch);

    // ② resolve curl, then confine it in a second pass (the neighbours' current curl is required).
    for (int Z = 0; Z < Extent[2]; ++Z) for (int Y = 0; Y < Extent[1]; ++Y) for (int X = 0; X < Extent[0]; ++X)
    {
        const size_t I = Index(X, Y, Z);
        Cell Out = Cells[I];
        if (!Out.Solid)
        {
            const SmokeVector3 Vxp = Cells[Index(X + 1, Y, Z)].Velocity, Vxm = Cells[Index(X - 1, Y, Z)].Velocity;
            const SmokeVector3 Vyp = Cells[Index(X, Y + 1, Z)].Velocity, Vym = Cells[Index(X, Y - 1, Z)].Velocity;
            const SmokeVector3 Vzp = Cells[Index(X, Y, Z + 1)].Velocity, Vzm = Cells[Index(X, Y, Z - 1)].Velocity;
            const float S = 0.5f / CellSize;
            Out.Curl = { (Vyp.Z - Vym.Z - Vzp.Y + Vzm.Y) * S,
                         (Vzp.X - Vzm.X - Vxp.Z + Vxm.Z) * S,
                         (Vxp.Y - Vxm.Y - Vyp.X + Vym.X) * S };
        }
        Scratch[I] = Out;
    }
    Cells.swap(Scratch);
    for (int Z = 0; Z < Extent[2]; ++Z) for (int Y = 0; Y < Extent[1]; ++Y) for (int X = 0; X < Extent[0]; ++X)
    {
        const size_t I = Index(X, Y, Z);
        Cell Out = Cells[I];
        if (!Out.Solid)
        {
            const SmokeVector3 Curl = Out.Curl;
            const float CurlMagnitude = Length(Curl);
            const SmokeVector3 Gradient{ Length(Cells[Index(X + 1, Y, Z)].Curl) - Length(Cells[Index(X - 1, Y, Z)].Curl),
                                         Length(Cells[Index(X, Y + 1, Z)].Curl) - Length(Cells[Index(X, Y - 1, Z)].Curl),
                                         Length(Cells[Index(X, Y, Z + 1)].Curl) - Length(Cells[Index(X, Y, Z - 1)].Curl) };
            const SmokeVector3 Force = Cross(SmokeNormalize(Gradient), Curl) * (Settings.Get(GasParameter::Vorticity) * CurlMagnitude * Seconds);
            Out.Velocity = Out.Velocity + Force;
            const SmokeVector3 P = CellCentre(X, Y, Z);
            if (World.Distance(P) < CellSize * 0.75f)
            {
                const SmokeVector3 Normal = World.Normal(P);
                const float IntoSolid = SmokeDot(Out.Velocity, Normal);
                if (IntoSolid < 0.0f) Out.Velocity = Out.Velocity - Normal * IntoSolid;
            }
        }
        Scratch[I] = Out;
    }
    Cells.swap(Scratch);

    // ③ divergence and warm-started Jacobi pressure solve. Solid neighbours use a Neumann boundary.
    for (int Z = 0; Z < Extent[2]; ++Z) for (int Y = 0; Y < Extent[1]; ++Y) for (int X = 0; X < Extent[0]; ++X)
    {
        Cell& C = Cells[Index(X, Y, Z)];
        if (C.Solid) { C.Divergence = 0.0f; C.Pressure = 0.0f; continue; }
        const auto V = [&](int Nx, int Ny, int Nz) { return Cells[Index(Nx, Ny, Nz)].Velocity; };
        C.Divergence = 0.5f / CellSize * (V(X + 1, Y, Z).X - V(X - 1, Y, Z).X
                         + V(X, Y + 1, Z).Y - V(X, Y - 1, Z).Y + V(X, Y, Z + 1).Z - V(X, Y, Z - 1).Z);
    }
    const int Sweeps = std::clamp(static_cast<int>(std::lround(Settings.Get(GasParameter::PressureSweeps))), 4, 40);
    for (int Sweep = 0; Sweep < Sweeps; ++Sweep)
    {
        for (int Z = 0; Z < Extent[2]; ++Z) for (int Y = 0; Y < Extent[1]; ++Y) for (int X = 0; X < Extent[0]; ++X)
        {
            const size_t I = Index(X, Y, Z);
            if (Cells[I].Solid) { PressureScratch[I] = Cells[I].Pressure; continue; }
            float Sum = 0.0f;
            const int Neighbours[6][3] = {{X+1,Y,Z},{X-1,Y,Z},{X,Y+1,Z},{X,Y-1,Z},{X,Y,Z+1},{X,Y,Z-1}};
            for (const auto& N : Neighbours)
            {
                if (N[0] < 0 || N[0] >= Extent[0] || N[1] < 0 || N[1] >= Extent[1] || N[2] < 0 || N[2] >= Extent[2]) continue;
                const Cell& Adjacent = Cells[Index(N[0], N[1], N[2])];
                Sum += Adjacent.Solid ? Cells[I].Pressure : Adjacent.Pressure;
            }
            PressureScratch[I] = (Sum - Cells[I].Divergence * CellSize * CellSize) / 6.0f;
        }
        for (size_t I = 0; I < Cells.size(); ++I) Cells[I].Pressure = PressureScratch[I];
    }
    for (int Z = 0; Z < Extent[2]; ++Z) for (int Y = 0; Y < Extent[1]; ++Y) for (int X = 0; X < Extent[0]; ++X)
    {
        const size_t I = Index(X, Y, Z);
        if (Cells[I].Solid) continue;
        const auto Pressure = [&](int Nx, int Ny, int Nz) -> float
        {
            if (Nz < 0) return Cells[I].Pressure;
            if (Nx < 0 || Nx >= Extent[0] || Ny < 0 || Ny >= Extent[1] || Nz >= Extent[2]) return 0.0f;
            const Cell& N = Cells[Index(Nx, Ny, Nz)];
            return N.Solid ? Cells[I].Pressure : N.Pressure;
        };
        const SmokeVector3 Gradient{ Pressure(X+1,Y,Z)-Pressure(X-1,Y,Z), Pressure(X,Y+1,Z)-Pressure(X,Y-1,Z), Pressure(X,Y,Z+1)-Pressure(X,Y,Z-1) };
        Cells[I].Velocity = Cells[I].Velocity - Gradient * (0.5f / CellSize);
        const float Speed = Length(Cells[I].Velocity);
        if (Speed > 40.0f) Cells[I].Velocity = Cells[I].Velocity * (40.0f / Speed);
    }

    // ④ second-order backtrace of smoke/heat/flame/soot, combustion and Gaussian sources.
    float MaximumActivity = 0.0f;
    for (int Z = 0; Z < Extent[2]; ++Z) for (int Y = 0; Y < Extent[1]; ++Y) for (int X = 0; X < Extent[0]; ++X)
    {
        const size_t I = Index(X, Y, Z);
        Cell Out = Cells[I];
        const SmokeVector3 P = CellCentre(X, Y, Z);
        if (!Out.Solid && World.Distance(P) >= CellSize * 0.35f)
        {
            const SmokeVector3 V = Out.Velocity;
            const SmokeVector3 Mid = P - V * (0.5f * Seconds);
            const SmokeFieldSample Prior = SampleGrid(Scratch, P - SampleVelocity(Scratch, Mid) * Seconds);
            Out.Density = Prior.Density * DensityLoss;
            Out.Temperature = Prior.Temperature * HeatLoss;
            Out.Flame = Prior.Flame * FlameLoss;
            Out.Soot = Prior.Soot * DensityLoss;
            const float Burn = Out.Flame * Seconds;
            Out.Temperature += Burn * 1200.0f;
            Out.Soot += Burn * 0.35f;
            if (Settings.Enabled)
                for (SmokeSource& Source : Sources)
                    if (Source.Active)
                    {
                        const float W = GaussianWeight(Source, P) * Seconds * Settings.Get(GasParameter::Emission);
                        Out.Density += Source.SmokeRate * W;
                        Out.Temperature += Source.HeatRate * W * Settings.Get(GasParameter::Heat) * 800.0f;
                        Out.Flame += Source.FlameRate * W * Settings.Get(GasParameter::FlameIntensity);
                        Out.Soot += Source.SootRate * W * Settings.Get(GasParameter::Soot);
                    }
            Out.Density = std::clamp(Out.Density, 0.0f, 64.0f);
            Out.Temperature = std::clamp(Out.Temperature, 0.0f, 4096.0f);
            Out.Flame = std::clamp(Out.Flame, 0.0f, 16.0f);
            Out.Soot = std::clamp(Out.Soot, 0.0f, 64.0f);
            MaximumActivity = std::max(MaximumActivity, std::max(Out.Density, std::max(Out.Flame, Out.Soot)));
        }
        else
            Out.Density = Out.Temperature = Out.Flame = Out.Soot = 0.0f;
        Scratch[I] = Out;
    }
    Cells.swap(Scratch);
    for (SmokeSource& Source : Sources)
    {
        if (!Source.Persistent)
        {
            Source.RemainingSeconds -= Seconds;
            if (Source.RemainingSeconds <= 0.0f) Source.Active = false;
        }
    }
    Sources.erase(std::remove_if(Sources.begin(), Sources.end(), [](const SmokeSource& S) { return !S.Active; }), Sources.end());
    ElapsedSeconds += Seconds;
    ++Metrics.Ticks;
    if (MaximumActivity < 0.002f && Sources.empty()) QuietSeconds += Seconds; else QuietSeconds = 0.0f;
    if (QuietSeconds >= FadeCeiling)
    {
        Resident = false;
        std::fill(Cells.begin(), Cells.end(), Cell{});
    }
    else
        Resident = MaximumActivity >= 0.002f || !Sources.empty();
}

} // namespace Frontier
