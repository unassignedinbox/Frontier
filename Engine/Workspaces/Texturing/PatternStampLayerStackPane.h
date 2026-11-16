/*============================================================================================================================================
                                                         PATTERNSTAMPLAYERSTACKPANE.H
============================================================================================================================================*/
// 🧩 Left-docked Project Zero Layers Outliner pane shared by Pattern Studio and Stamp Studio (stat tiles, quick chips, tree, starters, footer)

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

class PatternStampLayerStackPane
{
public:
    ~PatternStampLayerStackPane() noexcept;

    void AttachSession(PatternStampWorkspaceSession* TargetSession) noexcept;

    void Render(
        PixelSpace&             Surface,
        Rectangle2D             Bounds,
        const PaneInputReading& Input,
        double                  DeltaSeconds);

    void PresentInImGuiPane(double DeltaSeconds, float ScaleFactor = 1.0f);

public:
    bool  AddLayerSheetOpen = false;
    float ScrollOffsetY     = 0.0f;

private:
    PatternStampWorkspaceSession* Session            = nullptr;
    TextEditBuffer                SearchBuffer       {};
    TextEditBuffer                InlineRenameBuffer {};
    uint32_t                      RenamingLayerId    = 0u;
    float                         MaxScrollY         = 0.0f;
    Rectangle2D                   AddButtonAnchor    {};
    bool                          RootExpanded       = true;
    bool                          StartersExpanded   = false;
    bool                          FilterModsOnly     = false;
};

} // namespace Frontier::Workspaces
