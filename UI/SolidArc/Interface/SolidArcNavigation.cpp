/*====================================================================================================================================
                                                        SOLIDARCNAVIGATION.CPP
====================================================================================================================================*/
// 🧩 Plasticity viewport navigation on the shared Engine/Render/OrbitCamera (see header)

//------------------------------------------------------------------------------------------------------------------------
//                                                    INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "UI/SolidArc/Interface/SolidArcNavigation.h"

namespace Frontier::SolidArc
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    PUBLIC FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

bool NavigateSolidArcPointer(OrbitCamera& Camera, const OrbitCameraFrame& Frame, const SolidArcNavigationPointer& Pointer)
{
    bool MovedCondition = false;
    if (Pointer.CapturedCondition)
    {
        if (Pointer.MiddleDragCondition)
        {
            RevolveOrbitCamera(Camera, Pointer.DeltaX, Pointer.DeltaY);
            MovedCondition = true;
        }
        else if (Pointer.RightDragCondition)
        {
            PanOrbitCamera(Camera, Frame, Pointer.DeltaX, Pointer.DeltaY);
            MovedCondition = true;
        }
    }
    if (Pointer.HoveredCondition && Pointer.WheelSteps != 0.0f)
    {
        DollyOrbitCamera(Camera, Pointer.WheelSteps);
        MovedCondition = true;
    }
    return MovedCondition;
}

std::optional<OrbitViewPreset> NavigateSolidArcKeypad(OrbitCamera& Camera, const SolidArcNavigationKeypad& Keypad)
{
    if (Keypad.ProjectionPressed) Camera.OrthographicEnabled = !Camera.OrthographicEnabled;
    if (Keypad.FrontPressed) return Keypad.ControlCondition ? OrbitViewPreset::Back : OrbitViewPreset::Front;
    if (Keypad.RightPressed) return Keypad.ControlCondition ? OrbitViewPreset::Left : OrbitViewPreset::Right;
    if (Keypad.TopPressed) return Keypad.ControlCondition ? OrbitViewPreset::Bottom : OrbitViewPreset::Top;
    return std::nullopt;
}

} // namespace Frontier::SolidArc
