/*====================================================================================================================================
                                                      DRAFTINGINTERFACE.H
====================================================================================================================================*/
// 🧩 DraftingWorkspace ImGui shell: header, Outliner, viewport, Inspector, drawing and bevel sessions, shortcuts, autosave / recovery

#pragma once

//------------------------------------------------------------------------------------------------------------------------
//                                                    INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Document/DraftingDocument.h"
#include "Document/DraftingDrawingSheet.h"
#include "Document/DraftingExchange.h"
#include "Document/DraftingMeasurement.h"
#include "Document/DraftingScene.h"
#include "Interface/DraftingOptions.h"
#include "Viewport/DraftingCamera.h"
#include "Viewport/DraftingRasterization.h"
#include "Viewport/DraftingSnapping.h"

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
    GeometryProcessing::BlendProfile      Shape     = GeometryProcessing::BlendProfile::Fillet;                  // [-]   - Blend
    double                      Size      = 0.0;                                             // [len] - Blend size
    GeometryProcessing::FaceOperationKind Operation = GeometryProcessing::FaceOperationKind::Extrude;            // [-]   - Face verb
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
    GeometryProcessing::BlendProfile            Shape           = GeometryProcessing::BlendProfile::Fillet;      // [-]   - Cross-section
    double                            Size            = 0.25;                                // [len] - Radius / setback
    std::array<char, 32>              SizeText        = {};                                  // [-]   - Field text ("250mm", "0.25")
    std::unique_ptr<DraftingDocument> Preview         = {};                                  // [-]   - Staged document
    DraftingScene                     PreviewScene    = {};                                  // [-]   - Its display arrays
    std::string                       Refusal         = {};                                  // [-]   - Why Apply is disabled
    double                            StagedSize      = -1.0;                                // [len] - Size the preview was built at
    GeometryProcessing::BlendProfile            StagedShape     = GeometryProcessing::BlendProfile::Fillet;      // [-]   - Shape the preview was built at
    bool                              VariableEnabled = false;                               // [-]   - V: size varies along each edge
    double                            EndSize         = 0.0;                                 // [len] - Size at the far corners (0 = not chosen yet)
    std::array<char, 32>              EndSizeText     = {};                                  // [-]   - Field text
    bool                              ReversedCondition = false;                             // [-]   - F: anchor at each edge's end vertex
    bool                              StagedVariable  = false;                               // [-]   - Variable state the preview was built at
    double                            StagedEndSize   = -1.0;                                // [len] - EndSize the preview was built at
    bool                              StagedReversed  = false;                               // [-]   - Flip the preview was built at
    std::string                       ProfileLabel    = {};                                  // [-]   - Profile: sketch step extruded
    GeometryProcessing::FeatureCombination      Combination     = GeometryProcessing::FeatureCombination::Union; // [-]   - Profile: Q union · W difference · E intersect · R new
    GeometryProcessing::FeatureCombination      StagedCombination = GeometryProcessing::FeatureCombination::Union; // [-] - Combination the preview was built at
};

// 📝 Drawing session (Shift+A line, Shift+Q rectangle, Shift+W circle, Shift+S curve; arc / slot from the Draw card):
//    clicks on the construction plane (snapped to 0.1 unless Ctrl is held) collect points while the shape previews
//    through the pointer. Rectangle and circle finish on their 2nd click, arc and slot on their 3rd; line and curve on
//    Enter / right-click, or closed by a click on their first point. Ctrl+Z drops the last point, Esc aborts.
struct DraftingDrawingSession
{
    bool                           ActiveCondition  = false;                                 // [-]   - Session open
    DraftingDrawingKind            Kind             = DraftingDrawingKind::Line;             // [-]   - Tool
    std::vector<GeometryProcessing::Vector2> Points           = {};                                    // [len] - Committed clicks (plane coordinates)
    GeometryProcessing::Vector2              Pointer          = {};                                    // [len] - Pointer on the plane (snapped)
    bool                           PointerCondition = false;                                 // [-]   - Pointer hits the plane
    int                            GridPlane        = 0;                                     // [-]   - 0 XY · 1 XZ · 2 YZ (fixed at open)
    bool                           CentreCondition  = false;                                 // [-]   - Rectangle: Alt centre mode · circle: V two-point mode off
    GeometryProcessing::SketchSplineForm     Form             = GeometryProcessing::SketchSplineForm::Hermite;   // [-]   - Curve: 1 Hermite · 2 Bezier · 3 NURBS · 4 cubic spline
    bool                           SquareCondition  = false;                                 // [-]   - Rectangle: s keeps the sides equal
    std::string                    Refusal          = {};                                    // [-]   - Why the preview cannot finish
};

// 📝 Point picker (Plasticity body[gizmo=point-picker]): object snaps under the pointer while drawing and moving, and
//    the direction lock keys n / b / t / x / y / z through the last placed point. Ctrl suspends snapping while held.
struct DraftingPointPicker
{
    bool                                   Enabled         = true;                           // [-]   - Object snaps on (Inspector)
    DraftingSnapFilter                     Filter          = {};                             // [-]   - Enabled kinds (Inspector)
    DraftingSnap                           Current         = {};                             // [-]   - Snap under the pointer this frame
    DraftingSnap                           Placed          = {};                             // [-]   - Snap of the last placed point (t / n / b)
    DraftingSnapLockKind                   Lock            = DraftingSnapLockKind::None;     // [-]   - Direction lock (cleared by the next point)
    GeometryProcessing::Vector3                      LockOrigin      = {};                             // [len] - Last placed point
    GeometryProcessing::Vector3                      LockDirection   = {};                             // [-]   - Unit lock direction
    std::unique_ptr<DraftingSnapCatalogue> Catalogue       = {};                             // [-]   - Exact carriers of the shown scene
    size_t                                 SceneFaces      = 0;                              // [-]   - Scene face count the catalogue indexes
    int                                    Snapped         = 0;                              // [-]   - Points placed on object snaps (script evidence)
};

// 📝 Mode 1 pole drag: pressing on a shown pole grabs it; the pole follows the pointer in the screen-parallel plane
//    through its start; the release commits one MoveDraftingPole (one undo entry), Esc / right-click drops it.
struct DraftingPoleDrag
{
    bool              ActiveCondition = false;                                               // [-]   - Pole grabbed
    std::string       Label           = {};                                                  // [-]   - SpaceCurve step
    int               Pole            = -1;                                                  // [-]   - Pole index
    GeometryProcessing::Vector3 Origin          = {};                                                  // [len] - Pole at the press
    GeometryProcessing::Vector3 Position        = {};                                                  // [len] - Pole under the pointer
    GeometryProcessing::Vector3 Facing          = {};                                                  // [-]   - Drag plane normal (view ray at the press)
    std::string       HoverLabel      = {};                                                  // [-]   - Pole under the pointer (mode 1)
    int               HoverPole       = -1;                                                  // [-]   - Its index
    int               Commits         = 0;                                                   // [-]   - Accepted drags (script evidence)
};

// 📝 Pictures behind the document's image planes as the frame last decoded them (one entry per distinct path).
struct DraftingPictureShelf
{
    std::string              Key      = {};                                                  // [-]  - Joined paths of the last upload
    std::vector<std::string> Paths    = {};                                                  // [-]  - Distinct resolved paths, upload order
    std::vector<uint32_t>    Widths   = {};                                                  // [px] - Per path (0: unreadable)
    std::vector<uint32_t>    Heights  = {};                                                  // [px] - Per path
    std::vector<std::string> Refusals = {};                                                  // [-]  - Per path (empty: read)
    int                      Drawn    = 0;                                                   // [-]  - Planes drawn last frame
};

// 📝 Matcap library as the interface last assembled it (Studio + built-ins + the document's pictures) and the Plasticity M
//    picker. Generation changes with every assembly so the frame re-uploads the texture array and thumbnails only then.
struct DraftingMatcapShelf
{
    DraftingMatcapLibrary    Library    = {};                                                // [-] - Layer order names / captions / pictures
    uint64_t                 Generation = 0;                                                 // [-] - Bumped per assembly
    std::vector<ImTextureID> Thumbnails = {};                                                // [-] - Per layer (frame-owned, may be empty)
    bool                     PickerOpen = false;                                             // [-] - M: Set material popup
    std::array<char, 260>    PicturePath = {};                                               // [-] - Add-picture path field
};

// 📝 Plasticity Q (command:boolean) on the bodies picked in mode 4: the first is the target, the rest are tools.
//    Q union · W difference · E intersect · Enter applies · Esc cancels.
struct DraftingBooleanSession
{
    bool                         ActiveCondition = false;                                    // [-] - Card open
    GeometryProcessing::FeatureCombination Combination     = GeometryProcessing::FeatureCombination::Union;      // [-] - Chosen operation
    int                          Commits         = 0;                                        // [-] - Booleans applied (script evidence)
};

// 📝 Plasticity T (command:trim): while open, the sketch curve under the pointer (≤ 8 px) previews in red the piece that
//    would come off; a click trims it and the session stays open for the next piece; Esc, T or right-click ends it.
struct DraftingTrimSession
{
    bool                           ActiveCondition = false;                                  // [-]   - T pressed, not ended
    bool                           HoverCondition  = false;                                  // [-]   - A curve is under the pointer
    std::string                    Sketch          = {};                                     // [-]   - Hovered sketch step
    int                            Curve           = -1;                                     // [-]   - Hovered curve index
    GeometryProcessing::Vector2              Pick            = {};                                     // [len] - Pointer on that sketch's plane
    std::vector<GeometryProcessing::Vector3> Preview         = {};                                     // [len] - Piece that would come off (world)
    std::string                    Refusal         = {};                                     // [-]   - Why the hovered piece cannot come off
    int                            Commits         = 0;                                      // [-]   - Trims applied (script evidence)
};

// 📝 K (constrain, SolveSpace's verbs on Plasticity's free K key) on one sketch step: click points / curves to pick them
//    (a click on a constraint glyph or dimension picks that constraint, a click on empty space clears), then D distance /
//    length / radius and N angle wait for a typed value (Enter; nothing typed keeps the measured one), H V O Q M Y L [ ]
//    apply at once. A press on a point that travels drags it (snapped 0.1 unless Ctrl; the rest of the sketch follows
//    its constraints), the release commits. A picked constraint: Delete / X / Backspace removes it, digits + Enter edit a
//    dimension. Esc drops a pending verb, then the picks, then ends; K or right-click ends.
//    Splines: O point on the curve, H / V with an end point (end direction), L (G1) and C (G2) with a curve meeting an end.
//    Shift+D adds a reference (driven, display-only) dimension, drawn grey in parentheses; E toggles a picked dimension
//    between driving and reference. Earlier sketches draw dimmed and are pickable: a constraint on their geometry links
//    it in (amber, held at its projection, following them on regeneration); Delete on a picked linked point / curve unlinks.
struct DraftingConstraintSession
{
    bool                                ActiveCondition      = false;                         // [-]   - K pressed, not ended
    std::string                         Sketch               = {};                            // [-]   - Sketch step label
    std::vector<DraftingSketchPick>     Picks                = {};                            // [-]   - Picked points / curves (click order; Sketch set: an earlier sketch's)
    bool                                HoverCondition       = false;                         // [-]   - A point / curve is under the pointer
    DraftingSketchPick                  Hover                = {};                            // [-]   - It
    int                                 HoverConstraint      = -1;                            // [-]   - Glyph under the pointer (−1: none)
    int                                 PickedConstraint     = -1;                            // [-]   - Picked glyph (−1: none)
    bool                                PendingCondition     = false;                         // [-]   - D / N waits for a value
    GeometryProcessing::SketchConstraint          Proposed             = {};                            // [-]   - The constraint D / N would add
    std::string                         Typed                = {};                            // [-]   - Typed value (pending verb or picked dimension)
    bool                                PressCondition       = false;                         // [-]   - Left button went down on a point
    int                                 DragPoint            = -1;                            // [-]   - That point
    bool                                DragCondition        = false;                         // [-]   - The press travelled: dragging
    GeometryProcessing::Vector2                   DragTarget           = {};                            // [len] - Pointer on the sketch plane (snapped)
    bool                                DragPreviewCondition = false;                         // [-]   - DragPreview is valid
    GeometryProcessing::SketchModel               DragPreview          = {};                            // [-]   - The sketch as the drag would leave it
    std::string                         Refusal              = {};                            // [-]   - Why the last verb / drag was refused
    int                                 Commits              = 0;                             // [-]   - Constraints added / removed / edited, drags (script evidence)
};

enum class DraftingTransformVerb : uint8_t
{
    Move   = 0,                                                                              // G
    Rotate = 1,                                                                              // R
    Scale  = 2,                                                                              // S
};

// 📝 Plasticity G / R / S on the picked bodies (Shift+D: duplicate, then move). The pointer drives the map from where
//    the session opened: move along the screen plane, an axis (X / Y / Z) or the plane normal to one (Shift+X / Y / Z);
//    rotate by the pointer's sweep about the pivot (screen axis or X / Y / Z); scale by its distance ratio to the pivot
//    (uniform or one axis). Snaps 0.1 / 5° / ×0.1 unless Ctrl is held; typed digits override the pointer (m, °, ×).
//    A staged document shows the result; click / Enter commits one command (one undo entry), Esc / right-click drops it.
struct DraftingTransformSession
{
    bool                              ActiveCondition    = false;                            // [-]   - Session open
    DraftingTransformVerb             Verb               = DraftingTransformVerb::Move;      // [-]   - G / R / S
    bool                              DuplicateCondition = false;                            // [-]   - Shift+D: commit copies
    std::vector<std::string>          Keys               = {};                               // [-]   - Bodies moved
    int                               Axis               = -1;                               // [-]   - −1 screen · 0 x · 1 y · 2 z
    bool                              PlaneCondition     = false;                            // [-]   - Move in the plane normal to Axis
    bool                              StartCondition     = false;                            // [-]   - Pointer anchor taken
    ImVec2                            Start              = {};                               // [px]  - Anchor (screen)
    ImVec2                            StartLocal         = {};                               // [px]  - Anchor (viewport)
    GeometryProcessing::Vector3                 Pivot              = {};                               // [len] - Bounds centre of the bodies
    bool                              AnchorCondition    = false;                            // [-]   - Move started on an object snap
    GeometryProcessing::Vector3                 Anchor             = {};                               // [len] - That snap point (moves onto the target snap)
    std::string                       AnchorSource       = {};                               // [-]   - "Endpoint · Edge …"
    std::string                       Typed              = {};                               // [-]   - Typed amount
    GeometryProcessing::AffineMap               Map                = {};                               // [-]   - Current map
    GeometryProcessing::AffineMap               StagedMap          = {};                               // [-]   - Map the preview shows
    bool                              StagedCondition    = false;                            // [-]   - A preview was built
    std::unique_ptr<DraftingDocument> Preview            = {};                               // [-]   - Staged document
    DraftingScene                     PreviewScene       = {};                               // [-]   - Its display arrays
    std::string                       Readout            = {};                               // [-]   - "Move x 0.3"
    std::string                       Refusal            = {};                               // [-]   - Why commit is refused
    int                               Commits            = 0;                                // [-]   - Committed (script evidence)
};

enum class DraftingSweepVerb : uint8_t
{
    Offset = 0,                                                                              // O: offset-curve
    Pipe      = 1,                                                                           // P: pipe
    Revolve   = 2,                                                                           // Alt+R: revolution (no Plasticity default key)
    Evolution = 3,                                                                           // Shift+P / Alt+E: evolution
};

// 📝 Plasticity O (offset the focused sketch: a new sketch) and P (a tube along the focused sketch or the selected curve):
//    horizontal pointer motion sets the distance / radius (Shift fine, snapped 0.01 unless Ctrl), typed digits override;
//    P takes Q / W / E / R like extrude. The offset previews as an outline, the pipe as a staged document.
//    Alt+R turns the focused sketch about an axis (typed degrees, default 360; X / Y sketch axes; a click on one of its
//    lines makes that the axis). Shift+P / Alt+E sweeps the focused sketch along a path (the selected curve or the newest
//    one-curve sketch; a click on another sketch's curve picks it). Both stage the result and take Q / W / E / R.
struct DraftingSweepSession
{
    bool                                        ActiveCondition   = false;                   // [-]   - Session open
    DraftingSweepVerb                           Verb              = DraftingSweepVerb::Offset; // [-] - O / P
    std::string                                 Path              = {};                      // [-]   - Sketch step / curve label (Revolve: profile; Evolution: path)
    std::string                                 Profile           = {};                      // [-]   - Evolution: profile sketch
    DraftingRevolveAxis                         Axis              = {};                      // [-]   - Revolve: axis
    std::string                                 StagedKey         = {};                      // [-]   - Revolve / Evolution: axis / path the preview shows
    std::string                                 HoverSketch       = {};                      // [-]   - Revolve / Evolution: sketch under the pointer
    int                                         HoverCurve        = -1;                      // [-]   - Its curve index (−1: none)
    double                                      Amount            = 0.1;                     // [len] - Offset distance / pipe radius ([°] revolve angle)
    double                                      Drift             = 0.0;                     // [len] - Unsnapped pointer amount
    std::string                                 Typed             = {};                      // [-]   - Typed amount
    GeometryProcessing::FeatureCombination                Combination       = GeometryProcessing::FeatureCombination::Separate; // [-] - Pipe: Q / W / E / R
    double                                      StagedAmount      = -1.0e300;                // [len] - Amount the preview shows
    GeometryProcessing::FeatureCombination                StagedCombination = GeometryProcessing::FeatureCombination::Separate; // [-] - Its combination
    std::unique_ptr<DraftingDocument>           Preview           = {};                      // [-]   - Pipe: staged document
    DraftingScene                               PreviewScene      = {};                      // [-]   - Its display arrays
    std::vector<std::vector<GeometryProcessing::Vector3>> Outline           = {};                      // [len] - Offset: preview curves (world)
    std::string                                 Refusal           = {};                      // [-]   - Why commit is refused
    int                                         Commits           = 0;                       // [-]   - Committed (script evidence)
};

// 📝 Ctrl+C: the picked bodies and the step they were copied after; Ctrl+V pastes them as they stood then (in place).
struct DraftingClipboard
{
    std::vector<std::string> Keys   = {};                                                    // [-] - Copied body keys
    std::string              Source = {};                                                    // [-] - Last step at copy time
};

// 📝 Box selection (Plasticity viewport-selector drag): a left drag from empty viewport space in modes 1 / 2 / 5 selects
//    every curve whose projection lies wholly inside the box, in feature-tree order (Shift adds, Ctrl removes); in modes
//    2 / 5 also every B-rep edge wholly inside — the visible ones, or hidden ones too in x-ray (EncloseDraftingEdges).
struct DraftingBoxSelection
{
    bool   ActiveCondition = false;                                                          // [-]  - Left button held after a press on empty space
    ImVec2 Start           = {};                                                             // [px] - Press position (screen)
    ImVec2 End             = {};                                                             // [px] - Pointer (screen)
    int    Commits         = 0;                                                              // [-]  - Box selections applied (script evidence)
    int    Edges           = 0;                                                              // [-]  - B-rep edges the last box enclosed
};

// 📝 Measure (I; Fusion Inspect → Measure — Plasticity has none): the Inspector card and viewport overlay (dimension line
//    between the closest points, centroid marker) of MeasureDraftingSelection, recomputed only when the selection or
//    the document revision changes.
struct DraftingMeasureSession
{
    bool                ActiveCondition = false;                                             // [-] - Card + overlay shown
    DraftingMeasurement Measurement     = {};                                                // [-] - Last exact measurement
    int                 Refreshes       = 0;                                                 // [-] - Recomputations (script evidence)
};

// 📝 Section (Alt+S; Fusion Section Analysis): the viewport clips everything on the removed side of the plane and caps
//    the cut solids on the GPU; the card shows the exact kernel section (SectionDraftingDocument), recomputed when the
//    request or the revision changes. The plane keeps its +normal side, as the kernel cut does.
struct DraftingSectionSession
{
    bool                   ActiveCondition = false;                                          // [-] - Clipping + card shown
    DraftingSectionRequest Request         = {};                                             // [-] - Plane choice
    DraftingSection        Section         = {};                                             // [-] - Last exact section
    std::array<char, 32>   OffsetText      = {};                                             // [-] - Offset field ("0.5", "250mm")
    DraftingSectionClip    Clip            = {};                                             // [-] - Plane + sheet faces for the CPU pick
    int                    Refreshes       = 0;                                              // [-] - Recomputations (script evidence)
};

// 📝 Drawing sheet (Alt+D; Fusion Drawing — Plasticity has none): the viewport shows the paper of the shown solid
//    bodies (kernel views, hidden lines, dimensions, title block) instead of the model; wheel zooms about the pointer,
//    middle / right drag pans, double-click fits. The card picks paper, angle, scale, views and the title fields (saved
//    with the document) and exports SVG / DXF R12 beside the document. Esc or Alt+D returns to the model.
struct DraftingDrawingSheetSession
{
    bool                  ActiveCondition = false;                                           // [-]  - Paper shown + card
    DraftingDrawingSheet  Sheet           = {};                                              // [-]  - Last composition
    float                 Zoom            = 1.0f;                                            // [-]  - × the fitted size
    std::array<float, 2>  Pan             = {};                                              // [px] - Paper centre offset
    std::array<char, 96>  TitleText       = {};                                              // [-]  - Title field
    std::array<char, 96>  NumberText      = {};                                              // [-]  - Drawing number field
    std::array<char, 96>  AuthorText      = {};                                              // [-]  - Drawn-by field
    std::array<char, 96>  MaterialText    = {};                                              // [-]  - Material field
    std::string           ExportPath      = {};                                              // [-]  - Last written file
    std::string           ExportRefusal   = {};                                              // [-]  - Last export refusal
    double                ExportBytes     = 0.0;                                             // [B]  - Size of the last file
    int                   Refreshes       = 0;                                               // [-]  - Compositions (script evidence)
    int                   Exports         = 0;                                               // [-]  - Files written
};

// 📝 STEP / STL exchange (priority 4 X4; Plasticity File → Import / Export, and Open / Save as by file type): Export…
//    writes the selected bodies (else every shown body) beside the document as STEP AP214 or binary STL in millimetres;
//    Import… reads .step / .stp (one Import step per solid, the file embedded) or VehicleCurves .toml.
struct DraftingExchangeSession
{
    DraftingExportOutcome Export             = {};                                           // [-]  - Last export (path, bodies, size, refusal)
    std::string           ImportPath         = {};                                           // [-]  - Last STEP file read
    std::string           ImportRefusal      = {};                                           // [-]  - Why the last STEP import was refused
    int                   ImportSolids       = 0;                                            // [-]  - Solids the last import added
    int                   ImportFaces        = 0;                                            // [-]  - Faces the last import read
    int                   ImportRepairs      = 0;                                            // [-]  - Loops reoriented + tolerant edges
    double                ImportMilliseconds = 0.0;                                          // [ms] - Read + canonical rewrite + regeneration
    int                   Exports            = 0;                                            // [-]  - Files written (script evidence)
    int                   Imports            = 0;                                            // [-]  - STEP files imported (script evidence)
};

// 📝 Joint (Alt+J; Fusion Joint — Plasticity has none): ① pick the joint origin on the moving body, ② on the fixed body
//    (the hovered edge / face shows its origin glyph: z blue, x red), ③ choose the kind (R rigid · V revolute · S slider ·
//    C cylindrical · P pin-slot · L planar · B ball), drag the values, F flips, Enter applies. Two entities picked beforehand
//    (on two bodies; the later pick is the fixed one) skip ① and ②. The preview is a staged document.
struct DraftingJointSession
{
    bool                              ActiveCondition = false;                               // [-] - Alt+J pressed, not ended
    int                               Phase           = 0;                                   // [-] - 0 moving origin · 1 fixed origin · 2 adjust
    DraftingJointRequest              Request         = {};                                  // [-] - Picks, kind and values
    DraftingSelectionKind             HoverKind       = DraftingSelectionKind::Edge;         // [-] - Entity under the pointer
    std::string                       HoverName       = {};                                  // [-] - Its name (empty: none)
    DraftingJointOrigin               Hover           = {};                                  // [-] - Its joint origin (glyph)
    DraftingJointOrigin               Moving          = {};                                  // [-] - Picked origin on the moving body
    DraftingJointOrigin               Fixed           = {};                                  // [-] - Picked origin on the fixed body
    std::string                       StagedKey       = {};                                  // [-] - Request the preview shows
    std::unique_ptr<DraftingDocument> Preview         = {};                                  // [-] - Staged document
    DraftingScene                     PreviewScene    = {};                                  // [-] - Its display arrays
    std::string                       Refusal         = {};                                  // [-] - Why the preview / commit is refused
    int                               Commits         = 0;                                   // [-] - Joints applied (script evidence)
};

// 📝 Dragging a joint value in the step card: the staged document shows the motion (replayed from the joint), the
//    document takes the value once on release (one undo entry).
struct DraftingParameterDrag
{
    std::string                       Parameter    = {};                                     // [-] - Parameter being dragged (empty: none)
    double                            Value        = 0.0;                                    // [-] - Value the preview shows
    std::unique_ptr<DraftingDocument> Preview      = {};                                     // [-] - Staged document
    DraftingScene                     PreviewScene = {};                                     // [-] - Its display arrays
    int                               Stagings     = 0;                                      // [-] - Previews built (script evidence)
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
    DraftingPointPicker                   Picker         = {};                               // [-]   - Object snaps and direction locks
    DraftingBoxSelection                  Box            = {};                               // [-]   - Left-drag box selection of curves
    DraftingBooleanSession                Boolean        = {};                               // [-]   - Q: Boolean card on picked bodies
    DraftingTrimSession                   Trim           = {};                               // [-]   - T: curve trim session
    DraftingConstraintSession             Constrain      = {};                               // [-]   - K: sketch constraint session
    DraftingTransformSession              Transform      = {};                               // [-]   - G / R / S / Shift+D session
    DraftingSweepSession                  Sweep          = {};                               // [-]   - O / P session
    DraftingClipboard                     Clipboard      = {};                               // [-]   - Ctrl+C / Ctrl+V
    DraftingMeasureSession                Measure        = {};                               // [-]   - I: Measure card + overlay
    DraftingSectionSession                Section        = {};                               // [-]   - Alt+S: section plane, caps, exact figures
    DraftingDrawingSheetSession           DrawingSheet   = {};                               // [-]   - Alt+D: paper, views, dimensions, export
    DraftingJointSession                  Joint          = {};                               // [-]   - Alt+J: joint card, origin glyphs, preview
    DraftingExchangeSession               Exchange       = {};                               // [-]   - Import… / Export…: STEP and STL files
    DraftingParameterDrag                 ParameterDrag  = {};                               // [-]   - Step card: dragged joint value
    bool                                  CombsEnabled   = true;                             // [-]   - Curvature combs on selected curves
    int                                   LoftContinuity = 0;                                // [-]   - Next loft: 0 G0 · 1 G1 · 2 G2 to the panel it meets
    bool                                  ZebraEnabled   = false;                            // [-]   - Zebra stripes on the faces (fine tessellation, V8)
    bool                                  DeviationEnabled = false;                          // [-]   - Needles from imported points to the panels (V8)
    DraftingDeviationMap                  Deviation      = {};                               // [-]   - Cached per document revision
    bool                                  ImagePlanesEnabled = true;                         // [-]   - Blueprint image planes drawn (toolbar Images)
    DraftingPictureShelf                  Pictures       = {};                               // [-]   - Decoded image-plane pictures (frame-owned)
    DraftingMatcapShelf                   Matcaps        = {};                               // [-]   - Matcap library + thumbnails + M picker
    DraftingRepeat                        LastCommand    = {};                               // [-]   - Shift+R
    std::string                           PopupRequest   = {};                               // [-]   - Header popup a shortcut asked for
    DraftingConstructRequest              ConstructForm  = {};                               // [-]   - Construct dialog values
    bool                                  ConstructOpen  = false;                            // [-]   - Construct dialog requested
    bool                                  ConstructShown = false;                            // [-]   - Construct dialog drawn this frame
    GeometryProcessing::FaceOperationKind           FaceVerb       = GeometryProcessing::FaceOperationKind::Push; // [-]  - Face-edit verb
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

// Reassembles the matcap library when the document's custom pictures changed (or on first use).
void RefreshDraftingMatcaps(DraftingWorkspaceState& State);

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

// Sketch step the constraint card / script fields describe: the K session's, else the focused sketch step, else "".
[[nodiscard]] std::string DraftingConstraintSketch(const DraftingWorkspaceState& State);

// JSON summary of the workspace (document, selection, camera, bevel session) for capture exhibits.
[[nodiscard]] std::string DescribeDraftingWorkspace(const DraftingWorkspaceState& State);

} // namespace Frontier::DraftingWorkspace
