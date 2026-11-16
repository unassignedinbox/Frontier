/*============================================================================================================================================
                                                         PATTERNSTAMPINSPECTORPANE.H
============================================================================================================================================*/
// 🧩 Right-docked Project Zero Inspector pane shared by Pattern Studio and Stamp Studio (identity thumb, meter trio, accordion rows, modifiers, document fold)

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
        AddChannelList,
        AddModifierList,
    };

    PatternStampWorkspaceSession* Session              = nullptr;
    int32_t                       OpenPropertyRow      = 1;     // 0:Shape 1:Fill 2:Outline 3:Placement 4:Blend 5:Surface 6:Mask (-1:closed)
    int32_t                       OpenModifierIndex    = -1;
    bool                          DocumentFoldExpanded = false;
    ActivePopoverKind             OpenPopover          = ActivePopoverKind::None;
    Rectangle2D                   PopoverAnchor        {};
    int32_t                       ActiveSliderId       = -1;
    float                         MaxScrollY           = 0.0f;
};

} // namespace Frontier::Workspaces
