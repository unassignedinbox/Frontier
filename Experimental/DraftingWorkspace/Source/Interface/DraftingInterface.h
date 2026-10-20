/*====================================================================================================================================
                                                      DRAFTINGINTERFACE.H
====================================================================================================================================*/
// 🧩 DraftingWorkspace ImGui shell: header, Outliner, viewport, Inspector, bevel session, shortcuts, autosave / recovery

#pragma once

//------------------------------------------------------------------------------------------------------------------------
//                                                    INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Document/DraftingDocument.h"
#include "Document/DraftingScene.h"
#include "Interface/DraftingOptions.h"
#include "Viewport/DraftingCamera.h"
#include "Viewport/DraftingRasterization.h"

#include <imgui.h>

#include <array>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace Frontier::DraftingWorkspace
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    ENUMS
//------------------------------------------------------------------------------------------------------------------------

// 📝 SolidArc selection modes: 1 picks whatever is under the pointer, 2 edges only, 3 faces only.
enum class DraftingPickMode : uint8_t
{
    Any  = 1,
    Edge = 2,
    Face = 3,
};

//------------------------------------------------------------------------------------------------------------------------
//                                                    STRUCTS
//------------------------------------------------------------------------------------------------------------------------

// 📝 B / Shift+B session: a staged copy of the document carries the preview; the authored document is untouched
//    until Apply (one undo entry). Positive pointer-adjusted size = fillet radius, negative = chamfer setback.
struct DraftingBevelSession
{
    bool                              ActiveCondition = false;                               // [-]   - Session open
    bool                              PinnedCondition = false;                               // [-]   - Numeric entry pins the size
    Modeller::BlendProfile            Shape           = Modeller::BlendProfile::Fillet;      // [-]   - Cross-section
    double                            Size            = 0.25;                                // [len] - Radius / setback
    std::array<char, 32>              SizeText        = {};                                  // [-]   - Field text ("250mm", "0.25")
    std::unique_ptr<DraftingDocument> Preview         = {};                                  // [-]   - Staged document
    DraftingScene                     PreviewScene    = {};                                  // [-]   - Its display arrays
    std::string                       Refusal         = {};                                  // [-]   - Why Apply is disabled
    double                            StagedSize      = -1.0;                                // [len] - Size the preview was built at
    Modeller::BlendProfile            StagedShape     = Modeller::BlendProfile::Fillet;      // [-]   - Shape the preview was built at
};

struct DraftingWorkspaceState
{
    DraftingOptions                       Options        = {};                               // [-]   - Loaded options
    DraftingDocument                      Document       = {};                               // [-]   - Authored document
    DraftingScene                         Scene          = {};                               // [-]   - Display arrays of the document
    DraftingCamera                        Camera         = {};                               // [-]   - Viewport camera
    DraftingViewportStyle                 Style          = {};                               // [-]   - Viewport appearance
    DraftingPickMode                      PickMode       = DraftingPickMode::Any;            // [-]   - 1 / 2 / 3
    DraftingBevelSession                  Bevel          = {};                               // [-]   - B / Shift+B session
    DraftingConstructRequest              ConstructForm  = {};                               // [-]   - Construct dialog values
    bool                                  ConstructOpen  = false;                            // [-]   - Construct dialog requested
    Modeller::FaceOperationKind           FaceVerb       = Modeller::FaceOperationKind::Push; // [-]  - Face-edit verb
    double                                FaceAmount     = 0.25;                             // [-]   - Face-edit amount (by verb)
    std::string                           FocusedStep    = {};                               // [-]   - Outliner row (step label)
    std::string                           DocumentPath   = {};                               // [-]   - Save / Open path
    std::array<char, 260>                 PathText       = {};                               // [-]   - Path field
    bool                                  DirtyCondition = false;                            // [-]   - Unsaved edits
    double                                AutosaveTimer  = -1.0;                             // [s]   - Countdown (< 0: idle)
    bool                                  WelcomeOpen    = false;                            // [-]   - Recovery prompt pending
    uint32_t                              HoverIdentity  = 0;                                // [-]   - GPU identity under the pointer
    std::array<float, 4>                  ViewportRect   = { 0, 0, 1, 1 };                   // [px]  - x, y, width, height in the frame
    bool                                  ViewportHovered = false;                           // [-]   - Pointer over the viewport
    std::string                           Status         = "Ready";                          // [-]   - Status line
    std::vector<std::string>              Journal        = {};                               // [-]   - Command log (newest last)
    std::map<std::string, ImVec4>         Widgets        = {};                               // [px]  - Named widget rectangles (x0, y0, x1, y1) of the last frame
    uint64_t                              FrameNumber    = 0;                                // [-]   - Frames drawn
    double                                FrameMs        = 0.0;                              // [ms]  - Last frame's CPU + GPU time
    std::string                           DeviceLabel    = {};                               // [-]   - Vulkan device description
};

//------------------------------------------------------------------------------------------------------------------------
//                                                    PUBLIC FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

// Loads the font, applies the SolidArc dark palette and spacing to the current ImGui context.
void ConfigureDraftingInterface(const DraftingOptions& Options);

// Empty document + defaults; offers recovery when Options.RecoveryPath holds an autosave.
void InitialiseDraftingWorkspace(DraftingWorkspaceState& State, const DraftingOptions& Options);

// 💡 Builds the whole frame (between ImGui::NewFrame and ImGui::Render). ViewportTexture is the colour target of the
//    previous viewport render. Serves every shortcut and viewport gesture; DeltaSeconds drives the autosave timer.
void DrawDraftingInterface(DraftingWorkspaceState& State, ImTextureID ViewportTexture, double DeltaSeconds);

// The scene the viewport must show (the staged preview during a bevel session).
[[nodiscard]] const DraftingScene& DisplayedDraftingScene(const DraftingWorkspaceState& State);

// Per-face / per-edge state words for the displayed scene (selection, active operand, hover).
[[nodiscard]] DraftingSceneStates DisplayedDraftingStates(const DraftingWorkspaceState& State);

// Rebuilds the scene after a document change and marks the document dirty (autosave pending).
void RefreshDraftingScene(DraftingWorkspaceState& State, bool DirtyCondition);

// Where New document keeps the previous recovery copy ("X.drafting.toml" → "X.previous.drafting.toml").
[[nodiscard]] std::string DraftingPreviousRecoveryPath(const DraftingOptions& Options);

// "250mm", "-25cm", "0.25m", "1in", "1ft", "2.5e-1" → model units (metres); false when unparsable.
bool ParseDraftingLength(const std::string& Text, double& Value);

// JSON summary of the workspace (document, selection, camera, bevel session) for capture exhibits.
[[nodiscard]] std::string DescribeDraftingWorkspace(const DraftingWorkspaceState& State);

} // namespace Frontier::DraftingWorkspace
