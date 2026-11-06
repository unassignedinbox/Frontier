#include "Frontier/Engine/Effects/PlumeSchedule.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace Frontier
{
namespace
{
constexpr double StepSeconds = 1.0 / 60.0, StepCeiling = 1.0 / 30.0, FadeCeiling = 12.0;
constexpr int RestartOrigin = 1 << 24;
const std::array<PlumeGrade, 4> Grades = {{
    {"Low", {64, 64, 20}, 0.40, 8, 32}, {"Medium", {96, 96, 24}, 0.30, 12, 40},
    {"High", {128, 128, 32}, 0.25, 16, 48}, {"Ultra", {160, 160, 48}, 0.20, 20, 64}
}};
}
const std::array<PlumeGrade, 4>& PlumeGrades() noexcept { return Grades; }
PlumeSchedule::PlumeSchedule(PlumeGrade Grade) : Configuration(Grade)
{
    for (int I = 0; I < 3; ++I) CoarseExtent[I] = Grade.Extent[I] / 2;
}
void PlumeSchedule::Recentre(const std::array<double, 3>& Focus, bool Force)
{
    const double Edge = 2.0 * Configuration.CellExtent;
    std::array<int, 2> Cell{static_cast<int>(std::floor(Focus[0] / Edge)), static_cast<int>(std::floor(Focus[1] / Edge))};
    const std::array<int, 2> Half{CoarseExtent[0] >> 1, CoarseExtent[1] >> 1};
    if (Force || std::abs(Cell[0] - (WindowOrigin[0] + Half[0])) > Half[0] / 2.0 ||
                 std::abs(Cell[1] - (WindowOrigin[1] + Half[1])) > Half[1] / 2.0)
        WindowOrigin = {Cell[0] - Half[0], Cell[1] - Half[1]};
}
void PlumeSchedule::Schedule(double Seconds, double FrameSeconds, const std::array<double, 3>& Focus,
                             const PlumeSourceBounds& Sources, const PlumeSettings& Settings,
                             const std::array<double, 3>& Sun, int Inspection)
{
    Pending = false;
    const bool Active = Settings.Enabled && Sources.Count > 0;
    if (Active)
    {
        QuietSeconds = 0;
        if (!IsResident)
        {
            IsResident = true; PriorOrigin = {RestartOrigin, RestartOrigin};
            Accumulator = StepSeconds; QuietPrior = 0; Recentre(Focus, true);
        }
    }
    else if (IsResident)
    {
        QuietSeconds += Seconds;
        if (QuietSeconds >= FadeCeiling || !Settings.Enabled) IsResident = false;
    }
    if (!IsResident) { Accumulator = 0; return; }
    Accumulator += Seconds;
    ElapsedSeconds += Seconds;
    if (Accumulator < StepSeconds) return;
    const double Step = std::min(Accumulator, StepCeiling);
    Accumulator = 0;
    Recentre(Focus, false);
    Encode(Step, FrameSeconds, Sources, Settings, Sun, Inspection);
    Pending = true;
}
void PlumeSchedule::Encode(double Step, double FrameSeconds, const PlumeSourceBounds& Sources,
                           const PlumeSettings& Settings, const std::array<double, 3>& Sun, int Inspection)
{
    const auto Read = [&](const char* Key) { return Settings.Values.at(Key); };
    const auto Integer = [&](int I, int Value) { RecordWords[I] = static_cast<uint32_t>(Value); };
    const auto Scalar = [&](int I, double Value) { float F = static_cast<float>(Value); std::memcpy(&RecordWords[I], &F, sizeof(F)); };
    double Fade = 1.0;
    if (QuietSeconds > 0.0)
        Fade = std::max(0.0, FadeCeiling - QuietSeconds) / std::max(Step, FadeCeiling - QuietPrior);
    QuietPrior = QuietSeconds;
    const double Dissipation = std::min(0.95, Read("PlumeDissipation"));
    const double WindAngle = Read("PlumeWindDirection") * 3.14159265358979323846 / 180.0;
    Integer(0, WindowOrigin[0]); Integer(1, WindowOrigin[1]); Integer(2, 0); Integer(3, StepOrdinal++);
    Integer(4, PriorOrigin[0]); Integer(5, PriorOrigin[1]); Integer(6, 0); Integer(7, Sources.Count);
    for (int I = 0; I < 3; ++I) { Integer(8+I, CoarseExtent[I]); Integer(12+I, Configuration.Extent[I]); }
    Integer(11, 0); Integer(15, Inspection == 10 ? 1 : 0);
    Scalar(16, Configuration.CellExtent); Scalar(17, 2 * Configuration.CellExtent); Scalar(18, -0.5); Scalar(19, Step);
    Scalar(20, Read("PlumeBuoyancy")); Scalar(21, 0.12); Scalar(22, Read("PlumeVorticity")); Scalar(23, Read("PlumeTurbulence"));
    Scalar(24, std::pow(1-Dissipation, Step)*Fade); Scalar(25, std::pow(1-std::min(0.95,Read("PlumeCooling")),Step)*Fade);
    Scalar(26, std::exp(-Read("PlumeFlameDecay")*Step)*Fade); Scalar(27, std::pow(1-0.08, Step));
    Scalar(28, std::cos(WindAngle)*Read("PlumeWindSpeed")); Scalar(29, std::sin(WindAngle)*Read("PlumeWindSpeed")); Scalar(30, 0); Scalar(31, ElapsedSeconds);
    for (int I=0; I<3; ++I) { Scalar(32+I, std::isfinite(Sources.Minimum[I]) ? Sources.Minimum[I] : 0); Scalar(36+I, std::isfinite(Sources.Maximum[I]) ? Sources.Maximum[I] : 0); }
    Scalar(35, std::max(FrameSeconds, 1.0e-3)); Scalar(39, 80*Read("PlumeFire"));
    Scalar(40, Read("PlumeExtinction")); Scalar(41, Read("PlumeAlbedo")); Scalar(42, 3*Read("PlumeExtinction")); Scalar(43, Configuration.MarchSamples);
    for (int I=0; I<3; ++I) Scalar(44+I, Sun[I]);
    Scalar(47, Read("PlumeSootAlbedo")); Scalar(48, 2.4*Read("PlumeSoot")); Scalar(49, 25*Read("PlumeHeat"));
    Scalar(50, std::pow(1-0.8*Dissipation, Step)*Fade); Scalar(51, 0);
    Scalar(52, Read("PlumeScattering")); Scalar(53, Read("PlumeAnisotropy")); Scalar(54, Read("PlumeAmbient")); Scalar(55, Read("PlumeFireLight"));
    Scalar(56, Read("PlumeDetail")); Scalar(57, Read("PlumeDetailScale")); Scalar(58, Read("PlumeTurbulenceScale")); Scalar(59, 0);
    Scalar(60, Read("PlumeFlameCore")); Scalar(61, Read("PlumeFlameTip")); Scalar(62, 30); Scalar(63, 0);
    PriorOrigin = WindowOrigin;
}
void PlumeSchedule::Clear() noexcept
{
    IsResident = false; Pending = false; QuietSeconds = 0; PriorOrigin = {RestartOrigin, RestartOrigin};
}
void PlumeSchedule::TransportRecorded() noexcept
{
    if (!Pending) return;
    CurrentState = 1-CurrentState; Pending = false; ++Steps;
}
} // namespace Frontier
