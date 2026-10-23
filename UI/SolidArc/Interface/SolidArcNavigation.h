/*====================================================================================================================================
                                                         SOLIDARCNAVIGATION.H
====================================================================================================================================*/
// 🧩 Plasticity viewport navigation on the shared Engine/Render/OrbitCamera: mouse gestures and numpad views

#pragma once

//------------------------------------------------------------------------------------------------------------------------
//                                                    INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Engine/Render/OrbitCamera.h"

#include <optional>

namespace Frontier::SolidArc
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    STRUCTS
//------------------------------------------------------------------------------------------------------------------------

// 📝 One frame of pointer input over the viewport, in viewport pixels (y down).
struct SolidArcNavigationPointer
{
    float DeltaX               = 0.0f;                                                        // [px]  - Pointer motion this frame
    float DeltaY               = 0.0f;                                                        // [px]  - Pointer motion this frame
    float WheelSteps           = 0.0f;                                                        // [-]   - + = away from the user (closer)
    bool  MiddleDragCondition  = false;                                                       // [-]   - Middle button dragging
    bool  RightDragCondition   = false;                                                       // [-]   - Right button dragging past the click tolerance
    bool  CapturedCondition    = false;                                                       // [-]   - Viewport hovered or holding the gesture
    bool  HoveredCondition     = false;                                                       // [-]   - Pointer over the viewport
};

// 📝 Numpad state this frame (Plasticity default-keymap.ts: "viewport" bindings stay live inside commands).
struct SolidArcNavigationKeypad
{
    bool FrontPressed        = false;                                                         // [-] - Numpad 1 (Ctrl: back)
    bool RightPressed        = false;                                                         // [-] - Numpad 3 (Ctrl: left)
    bool TopPressed          = false;                                                         // [-] - Numpad 7 (Ctrl: bottom)
    bool ProjectionPressed   = false;                                                         // [-] - Numpad 5: perspective ⇄ orthographic
    bool ControlCondition    = false;                                                         // [-] - Ctrl held: the opposite view
};

//------------------------------------------------------------------------------------------------------------------------
//                                                    PUBLIC FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

// Plasticity orbit-controls: middle drag orbits (azimuth about world z, elevation about camera right), right drag
// pans in the view plane, the wheel dollies. Returns true when the camera moved.
bool NavigateSolidArcPointer(OrbitCamera& Camera, const OrbitCameraFrame& Frame, const SolidArcNavigationPointer& Pointer);

// Numpad 5 toggles the projection on the camera; 1 / 3 / 7 (Ctrl: opposite) return the named view to present, so the
// caller can also move the drawing plane with it.
[[nodiscard]] std::optional<OrbitViewPreset> NavigateSolidArcKeypad(OrbitCamera& Camera, const SolidArcNavigationKeypad& Keypad);

} // namespace Frontier::SolidArc
