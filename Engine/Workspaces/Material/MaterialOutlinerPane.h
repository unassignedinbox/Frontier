/*============================================================================================================================================
                                                          MATERIALOUTLINERPANE.H
============================================================================================================================================*/
// 🧩 Left-docked Material Outliner pane (pane-direct surface, sharp entries, live material/layer previews, green tick + eye + lock, TextureStudio + & footer)

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

class MaterialOutlinerPane
{
public:
    ~MaterialOutlinerPane() noexcept;

    void AttachSession(MaterialWorkspaceSession* TargetSession) noexcept;
    void AssignScrollOffsetY(float OffsetY) noexcept { ScrollOffsetY = OffsetY; }

    void Render(
        PixelSpace&             Surface,
        Rectangle2D             Bounds,
        const PaneInputReading& Input,
        double                  DeltaSeconds);

    void PresentInImGuiPane(double DeltaSeconds, float ScaleFactor = 1.0f);

private:
    MaterialWorkspaceSession* Session           = nullptr;
    TextEditBuffer            SearchBuffer      {};
    float                     ScrollOffsetY     = 0.0f;
    float                     MaxScrollY        = 0.0f;
    bool                      RootExpanded      = true;
    bool                      AddLayerPopoverOpen  = false;
    Rectangle2D               AddButtonAnchor   {};
    bool                      FilterEnabledOnly = false;
    bool                      FilterDrivenOnly  = false;
    uint32_t                  ExpandedGroupMask = 0x05u; // Primary + Dynamic open initially
    uint32_t                  LockedLayerMask   = 0u;
};

} // namespace Frontier::Workspaces
