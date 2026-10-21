/*====================================================================================================================================
                                                      DRAFTINGINTERFACE.H
====================================================================================================================================*/
// 🧩 DraftingWorkspace ImGui shell: header, Outliner, viewport, Inspector, drawing and bevel sessions, shortcuts, autosave / recovery

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

// 📝 Plasticity selection modes: 1 control points (curves and their poles), 2 edges, 3 faces, 4 solids, 5 everything.
enum class DraftingPickMode : uint8_t
{
    ControlPoint = 1,                                                                        // Plasticity 1: curves, pole drag
    Edge  = 2,                                                                               // Plasticity 2
    Face  = 3,                                                                               // Plasticity 3
    Solid = 4,                                                                               // Plasticity 4: a click selects the whole body
    Any   = 5,                                                                               // Plasticity 5 / Tab: edges and faces
};

// What a viewport click does to the selection (Plasticity: click, Shift+click, Ctrl+click, Alt+click).
enum class DraftingPickIntent : uint8_t
{
    Replace = 0,                                                                             // The selection becomes the hit
    Add     = 1,                                                                             // The hit joins the selection
    Remove  = 2,                                                                             // The hit leaves the selection
    Extend  = 3,                                                                             // Edges: the hit's tangent chain joins
};

// The command a session (B fillet, E extrude) stages and applies.
enum class DraftingSessionVerb : uint8_t
{
    Blend   = 0,                                                                             // Fillet / chamfer on the selected edges
    Extrude = 1,                                                                             // Extrude the selected faces
    Profile = 2,                                                                             // Extrude the focused sketch's profile (Q / W / E / R Boolean)
};

// Shift+R repeats the last applied command on the current selection.
enum class DraftingRepeatKind : uint8_t
{
    None      = 0,
    Blend     = 1,
    Face      = 2,
    Construct = 3,
};

struct DraftingRepeat
{
    DraftingRepeatKind          Kind      = DraftingRepeatKind::None;                        // [-]   - What to repeat
    Modeller::BlendProfile      Shape     = Modeller::BlendProfile::Fillet;                  // [-]   - Blend
    double                      Size      = 0.0;                                             // [len] - Blend size
    Modeller::FaceOperationKind Operation = Modeller::FaceOperationKind::Extrude;            // [-]   - Face verb
    double                      Amount    = 0.0;                                             // [-]   - Face amount (model units)
    DraftingConstructRequest    Construct = {};                                              // [-]   - Construct request
    DraftingBlendRequest        Blend     = {};                                              // [-]   - Blend request (variable sizes, flip)
};

//------------------------------------------------------------------------------------------------------------------------
//                                                    STRUCTS
//------------------------------------------------------------------------------------------------------------------------

// 📝 B / Shift+B session: a staged copy of the document carries the preview; the authored document is untouched
//    until Apply (one undo entry). Positive pointer-adjusted size = fillet radius, negative = chamfer setback.
//    V (Plasticity gizmo:fillet-solid:add) makes the size variable: Size at each edge's anchor corner, EndSize at its
//    other end; F flips which corner is the anchor. The pointer adjusts Size; EndSize is typed.
struct DraftingBevelSession
{
    bool                              ActiveCondition = false;                               // [-]   - Session open
    DraftingSessionVerb               Verb            = DraftingSessionVerb::Blend;          // [-]   - B (blend) / E (extrude)
    DraftingSessionVerb               PreviewVerb      = DraftingSessionVerb::Blend;          // [-]   - Verb the preview was built for
    bool                              PinnedCondition = false;                               // [-]   - Numeric entry pins the size
    Modeller::BlendProfile            Shape           = Modeller::BlendProfile::Fillet;      // [-]   - Cross-section
    double                            Size            = 0.25;                                // [len] - Radius / setback
    std::array<char, 32>              SizeText        = {};                                  // [-]   - Field text ("250mm", "0.25")
    std::unique_ptr<DraftingDocument> Preview         = {};                                  // [-]   - Staged document
    DraftingScene                     PreviewScene    = {};                                  // [-]   - Its display arrays
    std::string                       Refusal         = {};                                  // [-]   - Why Apply is disabled
    double                            StagedSize      = -1.0;                                // [len] - Size the preview was built at
    Modeller::BlendProfile            StagedShape     = Modeller::BlendProfile::Fillet;      // [-]   - Shape the preview was built at
    bool                              VariableEnabled = false;                               // [-]   - V: size varies along each edge
    double                            EndSize         = 0.0;                                 // [len] - Size at the far corners (0 = not chosen yet)
    std::array<char, 32>              EndSizeText     = {};                                  // [-]   - Field text
    bool                              ReversedCondition = false;                             // [-]   - F: anchor at each edge's end vertex
    bool                              StagedVariable  = false;                               // [-]   - Variable state the preview was built at
    double                            StagedEndSize   = -1.0;                                // [len] - EndSize the preview was built at
    bool                              StagedReversed  = false;                               // [-]   - Flip the preview was built at
    std::string                       ProfileLabel    = {};                                  // [-]   - Profile: sketch step extruded
    Modeller::FeatureCombination      Combination     = Modeller::FeatureCombination::Union; // [-]   - Profile: Q union · W difference · E intersect · R new
    Modeller::FeatureCombination      StagedCombination = Modeller::FeatureCombination::Union; // [-] - Combination the preview was built at
};

// 📝 Drawing session (Shift+A line, Shift+Q rectangle, Shift+W circle, Shift+S curve; arc / slot from the Draw card):
//    clicks on the construction plane (snapped to 0.1 unless Ctrl is held) collect points while the shape previews
//    through the pointer. Rectangle and circle finish on their 2nd click, arc and slot on their 3rd; line and curve on
//    Enter / right-click, or closed by a click on their first point. Ctrl+Z drops the last point, Esc aborts.
struct DraftingDrawingSession
{
    bool                           ActiveCondition  = false;                                 // [-]   - Session open
    DraftingDrawingKind            Kind             = DraftingDrawingKind::Line;             // [-]   - Tool
    std::vector<Modeller::Vector2> Points           = {};                                    // [len] - Committed clicks (plane coordinates)
    Modeller::Vector2              Pointer          = {};                                    // [len] - Pointer on the plane (snapped)
    bool                           PointerCondition = false;                                 // [-]   - Pointer hits the plane
    int                            GridPlane        = 0;                                     // [-]   - 0 XY · 1 XZ · 2 YZ (fixed at open)
    bool                           CentreCondition  = false;                                 // [-]   - Rectangle: Alt centre mode · circle: V two-point mode off
    Modeller::SketchSplineForm     Form             = Modeller::SketchSplineForm::Hermite;   // [-]   - Curve: 1 Hermite · 2 Bezier · 3 NURBS · 4 cubic spline
    std::string                    Refusal          = {};                                    // [-]   - Why the preview cannot finish
};

// 📝 Mode 1 pole drag: pressing on a shown pole grabs it; the pole follows the pointer in the screen-parallel plane
//    through its start; the release commits one MoveDraftingPole (one undo entry), Esc / right-click drops it.
struct DraftingPoleDrag
{
    bool              ActiveCondition = false;                                               // [-]   - Pole grabbed
    std::string       Label           = {};                                                  // [-]   - SpaceCurve step
    int               Pole            = -1;                                                  // [-]   - Pole index
    Modeller::Vector3 Origin          = {};                                                  // [len] - Pole at the press
    Modeller::Vector3 Position        = {};                                                  // [len] - Pole under the pointer
    Modeller::Vector3 Facing          = {};                                                  // [-]   - Drag plane normal (view ray at the press)
    std::string       HoverLabel      = {};                                                  // [-]   - Pole under the pointer (mode 1)
    int               HoverPole       = -1;                                                  // [-]   - Its index
    int               Commits         = 0;                                                   // [-]   - Accepted drags (script evidence)
};

struct DraftingWorkspaceState
{
    DraftingOptions                       Options        = {};                               // [-]   - Loaded options
    DraftingDocument                      Document       = {};                               // [-]   - Authored document
    DraftingScene                         Scene          = {};                               // [-]   - Display arrays of the document
    DraftingCamera                        Camera         = {};                               // [-]   - Viewport camera
    DraftingViewportStyle                 Style          = {};                               // [-]   - Viewport appearance
    DraftingPickMode                      PickMode       = DraftingPickMode::Any;            // [-]   - 2 / 3 / 4 / 5 (Plasticity)
    DraftingBevelSession                  Bevel          = {};                               // [-]   - B (blend) / E (extrude) session
    DraftingDrawingSession                Drawing        = {};                               // [-]   - Line / rectangle / circle / arc / slot / curve
    DraftingPoleDrag                      PoleDrag       = {};                               // [-]   - Mode 1 pole drag
    bool                                  CombsEnabled   = true;                             // [-]   - Curvature combs on selected curves
    DraftingRepeat                        LastCommand    = {};                               // [-]   - Shift+R
    std::string                           PopupRequest   = {};                               // [-]   - Header popup a shortcut asked for
    DraftingConstructRequest              ConstructForm  = {};                               // [-]   - Construct dialog values
    bool                                  ConstructOpen  = false;                            // [-]   - Construct dialog requested
    bool                                  ConstructShown = false;                            // [-]   - Construct dialog drawn this frame
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
