#pragma once
/*==========================================================================================================================================
                                                         SOFTWAREPRESENTATION.H
==========================================================================================================================================*/
// 🧩 A CPU rasteriser standing in for the GPU presentation: draws either the Lobby studio or the Proving
//    Grounds lane with the vehicles from their facet archives, a z-buffer and simple lighting, then writes
//    a PPM. It exists so both C++ levels can be seen and regression-checked anywhere; a GLFW/WebGPU
//    front-end replaces this class, nothing else.

#include "Frontier/Engine/Effects/VehicleInspection.h"
#include "Frontier/Engine/Scene/FacetArchive.h"

#include <array>
#include <string>
#include <vector>

namespace Frontier
{

struct PresentationVehicle
{
    const FacetArchiveData* Body = nullptr;
    const FacetArchiveData* Tyre = nullptr;
    const FacetArchiveData* Rim = nullptr;
    std::array<float, 3> PositionMetres = { 0.0f, 0.0f, 0.53f };
    float YawDegrees = 0.0f;
    std::array<float, 3> Paint = { 0.20f, 0.21f, 0.23f };   // linear-ish albedo tint
    VehicleInspection Inspection{};   // visual effect for this presentation of the vehicle
};

// 📝 Renders one frame of either C++ level using the same vehicle facet archives. Camera orbits the
//    centre car at (YawDegrees, Pitch, DistanceMetres). Returns false if the file cannot be written.
bool RenderLobbyPpm(const std::string& Path, const std::vector<PresentationVehicle>& Vehicles,
                    float CameraYawDegrees, float CameraPitchDegrees, float CameraDistanceMetres,
                    int Width = 640, int Height = 400);
bool RenderProvingGroundsPpm(const std::string& Path, const std::vector<PresentationVehicle>& Vehicles,
                             float CameraYawDegrees, float CameraPitchDegrees, float CameraDistanceMetres,
                             int Width = 640, int Height = 400);

} // namespace Frontier
