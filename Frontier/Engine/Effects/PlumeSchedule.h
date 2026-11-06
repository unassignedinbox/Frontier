#pragma once
// Host-side schedule and uniform encoding port of PlumeSpace.js. Does not submit GPU work.
#include "Frontier/Engine/Effects/GasEditorModel.h"
#include <array>
#include <cstdint>

namespace Frontier
{
struct PlumeGrade
{
    const char* Name;
    std::array<int, 3> Extent;
    double CellExtent;
    int PressureSweeps, MarchSamples;
};
const std::array<PlumeGrade, 4>& PlumeGrades() noexcept;
struct PlumeSourceBounds
{
    int Count = 0;
    std::array<double, 3> Minimum{}, Maximum{};
};
class PlumeSchedule
{
  public:
    explicit PlumeSchedule(PlumeGrade Grade);
    void Schedule(double Seconds, double FrameSeconds, const std::array<double, 3>& Focus,
                  const PlumeSourceBounds& Sources, const PlumeSettings& Settings,
                  const std::array<double, 3>& SunDirection = { 0, 0, 1 }, int Inspection = 0);
    void Clear() noexcept;
    // Called only after the transport passes have actually been submitted.
    void TransportRecorded() noexcept;
    bool Resident() const noexcept { return IsResident; }
    bool StepPending() const noexcept { return Pending; }
    int State() const noexcept { return CurrentState; }
    int StepsTaken() const noexcept { return Steps; }
    std::array<int, 2> Origin() const noexcept { return WindowOrigin; }
    const std::array<uint32_t, 64>& Record() const noexcept { return RecordWords; }
  private:
    void Recentre(const std::array<double, 3>& Focus, bool Force);
    void Encode(double Step, double FrameSeconds, const PlumeSourceBounds& Sources,
                const PlumeSettings& Settings, const std::array<double, 3>& Sun, int Inspection);
    PlumeGrade Configuration;
    std::array<int, 3> CoarseExtent{};
    std::array<int, 2> WindowOrigin{0, 0}, PriorOrigin{1 << 24, 1 << 24};
    std::array<uint32_t, 64> RecordWords{};
    bool IsResident = false, Pending = false;
    int CurrentState = 0, Steps = 0, StepOrdinal = 0;
    double Accumulator = 0, QuietSeconds = 0, QuietPrior = 0, ElapsedSeconds = 0;
};
} // namespace Frontier
