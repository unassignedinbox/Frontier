#pragma once
/*==========================================================================================================================================
                                                         SOFTWAREPRESENTATION.H
==========================================================================================================================================*/
// 🧩 A CPU rasteriser standing in for the GPU presentation: draws the Lobby (studio floor, softbox, the
//    vehicle from its facet archives) with a z-buffer and one overhead light, and writes a PPM. It exists
//    so the C++ Lobby can be *seen* and regression-checked anywhere; the GLFW/WebGPU front-end replaces
//    this class, nothing else.

#include "Frontier/Engine/Effects/PaintRipple.h"
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
    PaintRipple BodyRipple{};         // finish spreading over the body from its touch point (zip ripple)
    PaintRipple RimRipple{};          // the same mechanism over the rims (1.5 s, 4.6 m in the reference)
};

// 📝 Renders one frame of the studio: white cyclorama floor disc, the softbox overhead, and the vehicles.
//    Camera orbits the centre car at (YawDegrees, Pitch, DistanceMetres). Returns false if the file
//    cannot be written.
bool RenderLobbyPpm(const std::string& Path, const std::vector<PresentationVehicle>& Vehicles,
                    float CameraYawDegrees, float CameraPitchDegrees, float CameraDistanceMetres,
                    int Width = 640, int Height = 400, float FovDegrees = 55.0f);

// 📝 The Garage portrait, 1:1 with the reference configurator's stage (Scene.tsx + Truck.tsx): the Lobby
//    studio itself — cove floor, turntable, the one softbox — holding ONE vehicle, shot from the fixed
//    camera at (6.8, 2.9, 7.6) / fov 34° aimed at (0, 0.55, 0). Nothing rotates: no turntable spin, no
//    auto-rotate (the reference's autoRotate defaults off); the camera never moves between frames, so a
//    paint ripple is the only motion in the sequence.
bool RenderGaragePpm(const std::string& Path, const PresentationVehicle& Vehicle, int Width = 640, int Height = 400);

} // namespace Frontier
