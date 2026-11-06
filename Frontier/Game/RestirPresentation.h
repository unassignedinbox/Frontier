#pragma once
// Adapter to the upstream native Vulkan/ReSTIR illumination host. No software-renderer fallback.
#include "Frontier/Game/SoftwarePresentation.h"
#include <memory>
#include <string>

namespace Frontier
{
class RestirPresentation
{
  public:
    RestirPresentation();
    ~RestirPresentation();
    RestirPresentation(const RestirPresentation&) = delete;
    RestirPresentation& operator=(const RestirPresentation&) = delete;
    bool Initialize(const std::string& ContentRoot, int Width, int Height, std::string& Diagnostic, bool Lobby = false);
    bool Render(const PresentationCamera& Camera, double DeltaSeconds, PresentationFrame& Out, std::string& Diagnostic);
    std::array<float, 3> VehiclePosition() const;
    // The session spawn at (0,0,0.53) maps to the vehicle's archived proving-ground spawn.
    void SetVehiclePose(const std::array<float, 3>& Position, float YawDegrees);
  private:
    struct Impl;
    std::unique_ptr<Impl> State;
};
}
