/*============================================================================================================================================
                                                          MATERIALINSPECTORPANE.H
============================================================================================================================================*/
// 🧩 Right-docked Project Zero Inspector pane for Material Workspace (meter stat trio, inline channel editors, surface finish fold, export bar)

#pragma once

//------------------------------------------------------------------------------------------------------------------------
//                                                        INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Engine/Components/Controls/TextInputField.h"
#include "Engine/Components/Panes/WorkspaceAppearance.h"
#include "Engine/Components/PixelSpace.h"
#include "Engine/Workspaces/Material/MaterialWorkspaceSession.h"

#include <cstdint>

namespace Frontier::Workspaces
{

//------------------------------------------------------------------------------------------------------------------------
//                                                  CLASS DECLARATION
//------------------------------------------------------------------------------------------------------------------------

class MaterialInspectorPane
{
public:
    ~MaterialInspectorPane() noexcept;

    void AttachSession(MaterialWorkspaceSession* TargetSession) noexcept;
    void AssignScrollOffsetY(float OffsetY) noexcept { ScrollOffsetY = OffsetY; }

    void Render(
        PixelSpace&             Surface,
        Rectangle2D             Bounds,
        const PaneInputReading& Input,
        double                  DeltaSeconds);

    void PresentInImGuiPane(double DeltaSeconds, float ScaleFactor = 1.0f);

private:
    MaterialWorkspaceSession* Session             = nullptr;
    TextEditBuffer            ChannelSearchBuffer {};
    float                     ScrollOffsetY       = 0.0f;
    float                     MaxScrollY          = 0.0f;
    int32_t                   ActiveSliderId      = -1;
    bool                      PresetPopoverOpen   = false;
    Rectangle2D               PresetAnchorRect    {};
    bool                      FinishFoldExpanded  = false;
    uint32_t                  OpenGroupMask       = 0x05u; // Primary + Dynamic open initially
};

} // namespace Frontier::Workspaces
