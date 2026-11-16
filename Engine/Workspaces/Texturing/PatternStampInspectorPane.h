/*============================================================================================================================================
                                                         PATTERNSTAMPINSPECTORPANE.H
============================================================================================================================================*/
// 🧩 Right-docked TextureStudio Inspector pane shared by Pattern Studio and Stamp Studio (KPI tiles, categorized section groups, TextureStudio sliders)

#pragma once

//------------------------------------------------------------------------------------------------------------------------
//                                                        INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Engine/Components/Controls/TextInputField.h"
#include "Engine/Components/Panes/WorkspaceAppearance.h"
#include "Engine/Workspaces/Texturing/PatternStampStackSession.h"

#include <cstdint>

namespace Frontier::Workspaces
{

//------------------------------------------------------------------------------------------------------------------------
//                                                  CLASS DECLARATION
//------------------------------------------------------------------------------------------------------------------------

class PatternStampInspectorPane
{
public:
    ~PatternStampInspectorPane() noexcept;

    void AttachSession(PatternStampWorkspaceSession* TargetSession) noexcept;

    void Render(
        PixelSpace&             Surface,
        Rectangle2D             Bounds,
        const PaneInputReading& Input,
        double                  DeltaSeconds);

    void PresentInImGuiPane(double DeltaSeconds, float ScaleFactor = 1.0f);

public:
    float ScrollOffsetY = 0.0f;

private:
    enum class ActivePopoverKind : uint8_t
    {
        None = 0,
        StarterLibrary,
        MotifShapeList,
        FinishPresetList,
        AddModifierList,
    };

    PatternStampWorkspaceSession* Session          = nullptr;
    bool                          LayerGroupOpen   = true;
    bool                          SurfaceGroupOpen = true;
    bool                          ModifierGroupOpen = true;
    bool                          DocumentGroupOpen = false;
    ActivePopoverKind             OpenPopover      = ActivePopoverKind::None;
    Rectangle2D                   PopoverAnchor    {};
    int32_t                       ActiveSliderId   = -1;
    float                         MaxScrollY       = 0.0f;
};

} // namespace Frontier::Workspaces
