/*============================================================================================================================================
                                                            BAKINGINSPECTORPANE.H
============================================================================================================================================*/
// 🧩 Right-docked Project Zero Channel Baking Inspector pane (study selector, bake progress bar, inspected target editor, output & cage fold, export)

#pragma once

//------------------------------------------------------------------------------------------------------------------------
//                                                        INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Engine/Components/Panes/WorkspaceAppearance.h"
#include "Engine/Components/PixelSpace.h"
#include "Engine/Workspaces/Texturing/BakingWorkspaceSession.h"

#include <cstdint>

namespace Frontier::Workspaces
{

class BakingInspectorPane
{
public:
    BakingInspectorPane() noexcept = default;
    ~BakingInspectorPane() noexcept;

    void AttachSession(BakingWorkspaceSession* TargetSession) noexcept;
    void PresentInImGuiPane(double DeltaSeconds, float ScaleFactor);
    void AssignScrollOffsetY(float OffsetY) noexcept { ScrollOffsetY = OffsetY; }

    void Render(
        PixelSpace&             Surface,
        Rectangle2D             Bounds,
        const PaneInputReading& Input,
        double                  DeltaSeconds);

private:
    BakingWorkspaceSession* Session            = nullptr;
    float                   ScrollOffsetY      = 0.0f;
    float                   MaxScrollY         = 0.0f;
    int32_t                 ActiveSliderId     = -1;
    bool                    StudyPopoverOpen   = false;
    Rectangle2D             StudyAnchorRect    {};
    bool                    OutputFoldExpanded = true;
    bool                    VolumeFoldExpanded = true;
};

} // namespace Frontier::Workspaces
