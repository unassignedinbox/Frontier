/*====================================================================================================================================
                                                        DRAFTINGCAMERA.H
====================================================================================================================================*/
// 🧩 Z-up orbit camera: orbit / pan / dolly, view presets, perspective or orthographic, Vulkan clip matrices, pixel rays

#pragma once

//------------------------------------------------------------------------------------------------------------------------
//                                                    INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Document/DraftingScene.h"

#include <array>
#include <cstdint>
#include <vector>

namespace Frontier::DraftingWorkspace
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    TYPES
//------------------------------------------------------------------------------------------------------------------------

using DraftingMatrix = std::array<double, 16>;                                               // Column-major 4 × 4

// 📝 SolidArc NATIVE_UI_PARITY view table: Top / Bottom look along ∓z (XY), Front / Back along ±y (XZ), Right / Left
//    along ∓x (YZ); Isometric looks from (+x, −y, +z).
enum class DraftingViewPreset : uint8_t
{
    Isometric = 0,
    Top       = 1,
    Bottom    = 2,
    Front     = 3,
    Back      = 4,
    Right     = 5,
    Left      = 6,
};

//------------------------------------------------------------------------------------------------------------------------
//                                                    STRUCTS
//------------------------------------------------------------------------------------------------------------------------

struct DraftingCamera
{
    std::array<double, 3> Target              = { 0.0, 0.0, 0.0 };                           // [len] - Orbit centre
    double                Yaw                 = -0.7853981633974483;                         // [rad] - Eye azimuth from +x (CCW about +z)
    double                Pitch               = 0.6154797086703873;                          // [rad] - Eye elevation above the XY plane
    double                Distance            = 8.0;                                         // [len] - Eye → target
    double                FieldOfView         = 0.7;                                         // [rad] - Vertical, perspective
    bool                  OrthographicEnabled = false;                                       // [-]   - Parallel projection
    double                SceneRadius         = 2.0;                                         // [len] - Bounds radius (near / far planes)
};

// 📝 Everything a frame needs, derived once per frame from the camera and the viewport size.
struct DraftingCameraFrame
{
    DraftingMatrix        ClipFromWorld = {};                                                // [-]   - Projection × view (Vulkan clip: y down, z ∈ [0, 1])
    DraftingMatrix        WorldFromClip = {};                                                // [-]   - Inverse
    std::array<double, 3> Eye           = {};                                                // [len] - Eye point
    std::array<double, 3> Right         = {};                                                // [-]   - Camera right
    std::array<double, 3> Up            = {};                                                // [-]   - Camera up
    std::array<double, 3> Forward       = {};                                                // [-]   - Eye → target
    double                WorldPerPixel = 0.01;                                              // [len] - At the target depth
    double                Width         = 1.0;                                               // [px]  - Viewport width
    double                Height        = 1.0;                                               // [px]  - Viewport height
    bool                  OrthographicEnabled = false;                                       // [-]   - Parallel projection
};

//------------------------------------------------------------------------------------------------------------------------
//                                                    PUBLIC FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

[[nodiscard]] DraftingCameraFrame FrameDraftingCamera(const DraftingCamera& Camera, double Width, double Height);

// Pixel (x right, y down, from the top-left corner; centres at +0.5) → world ray through it.
[[nodiscard]] DraftingRay CastDraftingRay(const DraftingCameraFrame& Frame, double PixelX, double PixelY);

// 💡 Box selection of B-rep edges (Plasticity viewport-selector): the scene edges whose every polyline point projects
//    inside the pixel box Low … High (viewport pixels, y down). Without x-ray only visible edges qualify: at least one
//    segment midpoint is not behind the nearest face along its pixel ray (2 px + 10⁻⁴ of the bounds diagonal of slack,
//    so creases and silhouettes count as visible). XRayEnabled keeps hidden edges too. Indices in scene order.
[[nodiscard]] std::vector<uint32_t> EncloseDraftingEdges(const DraftingScene& Scene, const DraftingCameraFrame& Frame, std::array<double, 2> Low,
                                                         std::array<double, 2> High, bool XRayEnabled);

void OrbitDraftingCamera(DraftingCamera& Camera, double DeltaX, double DeltaY);                       // [px]
void PanDraftingCamera(DraftingCamera& Camera, const DraftingCameraFrame& Frame, double DeltaX, double DeltaY); // [px]
void DollyDraftingCamera(DraftingCamera& Camera, double WheelSteps);                                  // [-] + = closer
void PresentDraftingView(DraftingCamera& Camera, DraftingViewPreset Preset);
void FitDraftingCamera(DraftingCamera& Camera, const DraftingScene& Scene);

[[nodiscard]] const char* DraftingViewLabel(DraftingViewPreset Preset) noexcept;
[[nodiscard]] DraftingMatrix InvertDraftingMatrix(const DraftingMatrix& Matrix);

} // namespace Frontier::DraftingWorkspace
