/*============================================================================================================================================
                                                             BAKINGTARGETSPANE.H
============================================================================================================================================*/
// 🧩 Left-docked Baking Queue Outliner pane (pane-direct surface, sharp entries, procedural target previews, green tick + eye + lock, TextureStudio footer)

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
    BakingWorkspaceSession* Session           = nullptr;
    TextEditBuffer          SearchBuffer      {};
    float                   ScrollOffsetY     = 0.0f;
    float                   MaxScrollY        = 0.0f;
    bool                    FilterQueuedOnly  = false;
    bool                    FilterReadyOnly   = false;
    bool                    PresetPopoverOpen = false;
    Rectangle2D             AddButtonAnchor   {};
    uint32_t                ExpandedGroupMask = 0x0Fu; // All 4 groups expanded initially
    uint32_t                LockedTargetMask  = 0u;
};

} // namespace Frontier::Workspaces
