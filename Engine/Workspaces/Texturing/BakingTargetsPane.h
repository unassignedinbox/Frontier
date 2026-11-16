/*============================================================================================================================================
                                                             BAKINGTARGETSPANE.H
============================================================================================================================================*/
// 🧩 Left-docked Project Zero Queue Outliner pane for Baking Studio (preset chips, categorized 18-target queue tree, studies library, footer)

#pragma once

//------------------------------------------------------------------------------------------------------------------------
//                                                        INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Engine/Components/Controls/TextInputField.h"
#include "Engine/Components/Panes/WorkspaceAppearance.h"
#include "Engine/Components/PixelSpace.h"
#include "Engine/Workspaces/Texturing/BakingWorkspaceSession.h"

#include <cstdint>

namespace Frontier::Workspaces
{

class BakingTargetsPane
{
public:
    BakingTargetsPane() noexcept = default;
    ~BakingTargetsPane() noexcept;

    void AttachSession(BakingWorkspaceSession* TargetSession) noexcept;
    void PresentInImGuiPane(double DeltaSeconds, float ScaleFactor);
    void AssignScrollOffsetY(float OffsetY) noexcept { ScrollOffsetY = OffsetY; }

    void Render(
        PixelSpace&             Surface,
        Rectangle2D             Bounds,
        const PaneInputReading& Input,
        double                  DeltaSeconds);

    static void ConstructTargetPreviewSwatch(
        PixelSpace&   Surface,
        Rectangle2D   SwatchBounds,
        BakerTargetId Target,
        bool          Ready) noexcept;

private:
    BakingWorkspaceSession* Session            = nullptr;
    TextEditBuffer          SearchBuffer       {};
    float                   ScrollOffsetY      = 0.0f;
    float                   MaxScrollY         = 0.0f;
    bool                    FilterQueuedOnly   = false;
    bool                    FilterReadyOnly    = false;
    bool                    StudiesExpanded    = false;
    uint32_t                ExpandedGroupMask  = 0x0Fu; // All 4 groups expanded initially
};

} // namespace Frontier::Workspaces
