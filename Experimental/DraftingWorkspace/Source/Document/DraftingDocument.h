/*====================================================================================================================================
                                                       DRAFTINGDOCUMENT.H
====================================================================================================================================*/
// 🧩 DraftingWorkspace document: feature tree, atomic commands with undo / redo, selection by persistent name, regeneration

#pragma once

//------------------------------------------------------------------------------------------------------------------------
//                                                    INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Feature/FeatureTree.h"
#include "Local/FaceOperations.h"
#include "Topology/ModelPartition.h"

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace Frontier::DraftingWorkspace
{

namespace Modeller = Frontier::SolidArcRemastered;

//------------------------------------------------------------------------------------------------------------------------
//                                                    ENUMS
//------------------------------------------------------------------------------------------------------------------------

// 📝 Construct catalogue tiles. Every tile authors an editable sketch + sweep pair, never a frozen primitive body.
enum class DraftingConstructKind : uint8_t
{
    Block    = 0,                                                                            // Rectangle sketch on XY, extruded along +z
    Cylinder = 1,                                                                            // Circle sketch on XY, extruded along +z
    Sphere   = 2,                                                                            // Half disc on XZ, revolved about its diameter
    Torus    = 3,                                                                            // Circle on XZ, revolved about a construction axis
};

// 📝 Drawing tools on the construction plane (Plasticity: Shift+A line, Shift+Q corner rectangle, Shift+W centre circle,
//    Shift+S curve; arc and slot from the Draw card). Each finished drawing becomes one editable sketch step.
enum class DraftingDrawingKind : uint8_t
{
    Line      = 0,                                                                           // Polyline through the clicks (closes on its first point)
    Rectangle = 1,                                                                           // Two corners, or centre + corner
    Circle    = 2,                                                                           // Centre + rim, or two diameter ends
    Arc       = 3,                                                                           // Start, end, a point on the arc
    Slot      = 4,                                                                           // Two cap centres + a point on the rim
    Curve     = 5,                                                                           // Spline through / on the clicks (form 1 … 4)
};

enum class DraftingSelectionKind : uint8_t
{
    Face = 0,                                                                                // A face of the current body
    Edge = 1,                                                                                // An edge of the current body
};

//------------------------------------------------------------------------------------------------------------------------
//                                                    STRUCTS
//------------------------------------------------------------------------------------------------------------------------

// 📝 Only the fields of the chosen kind are read. Position: block corner, cylinder base centre, sphere / torus centre.
struct DraftingConstructRequest
{
    DraftingConstructKind        Kind        = DraftingConstructKind::Block;                 // [-]   - Tile
    Modeller::FeatureCombination Combination = Modeller::FeatureCombination::Union;           // [-]   - With the current body (first body: New)
    Modeller::Vector3            Position    = { 0.0, 0.0, 0.0 };                             // [len] - Placement
    double                       Width       = 2.0;                                          // [len] - Block x extent
    double                       Depth       = 1.5;                                          // [len] - Block y extent
    double                       Height      = 1.0;                                          // [len] - Block / cylinder z extent
    double                       Radius      = 0.5;                                          // [len] - Cylinder / sphere radius
    double                       Major       = 1.5;                                          // [len] - Torus axis → tube centre
    double                       Minor       = 0.4;                                          // [len] - Torus tube radius
};

// 📝 Points are construction-plane coordinates (GridPlane 0 XY, 1 XZ, 2 YZ; see DraftingSketchPlane). Only the fields of
//    the kind are read; a Line or Curve is closed when ClosedCondition (its last point joins its first).
struct DraftingDrawingRequest
{
    DraftingDrawingKind            Kind            = DraftingDrawingKind::Line;              // [-]   - Tool
    int                            GridPlane       = 0;                                      // [-]   - 0 XY · 1 XZ · 2 YZ
    std::vector<Modeller::Vector2> Points          = {};                                     // [len] - Clicks in plane coordinates
    bool                           ClosedCondition = false;                                  // [-]   - Line / curve: close the chain
    bool                           CentreCondition = false;                                  // [-]   - Rectangle: centre + corner · circle: centre + rim (else two diameter ends)
    Modeller::SketchSplineForm     Form            = Modeller::SketchSplineForm::CubicSpline; // [-]  - Curve form (Plasticity 1 … 4)
};

// 📝 Selection is stored as persistent names, so it survives regeneration whenever the names still resolve.
struct DraftingSelection
{
    std::vector<std::string> Faces = {};                                                     // [-] - Selected face names
    std::vector<std::string> Edges = {};                                                     // [-] - Selected edge names
    std::vector<std::string> Curves = {};                                                    // [-] - Selected SpaceCurve labels, in pick order (loft order)
    DraftingSelectionKind    LastKind = DraftingSelectionKind::Face;                          // [-] - Kind of the most recent pick (active operand)
};

// 📝 Variable when EndSize is set: Size at each edge's anchor end, EndSize at its other end. The anchor is the edge's
//    start vertex on the current body, or its end vertex when ReversedCondition (Plasticity F flips it); it is stored as
//    a persistent vertex name, so a regeneration that reverses the edge keeps each size at its corner.
struct DraftingBlendRequest
{
    Modeller::BlendProfile Shape             = Modeller::BlendProfile::Fillet;               // [-]   - Cross-section
    double                 Size              = 0.25;                                         // [len] - Radius / setback (at the anchors)
    std::optional<double>  EndSize           = {};                                           // [len] - Size at the far ends (empty = constant)
    bool                   ReversedCondition = false;                                        // [-]   - Anchor at each edge's end vertex
};

struct DraftingCommandOutcome
{
    bool        AcceptedCondition = false;                                                   // [-]  - Document changed
    std::string Label             = {};                                                      // [-]  - History caption ("Block1", "Fillet r 0.2")
    std::string Refusal           = {};                                                      // [-]  - Why not, when refused (document untouched)
    double      Milliseconds      = 0.0;                                                     // [ms] - Command + regeneration time
};

// 📝 One undo entry: the whole tree before the command (trees are small; replay rebuilds the body exactly).
struct DraftingHistoryEntry
{
    Modeller::FeatureTree              Tree     = {};                                        // [-] - Tree before the command
    std::map<std::string, std::string> Captions = {};                                        // [-] - Step captions before the command
    std::string                        Label    = {};                                        // [-] - Command caption
};

// 📝 Body figures shown by the Inspector (recomputed on every regeneration).
struct DraftingBodySummary
{
    bool   PresentCondition = false;                                                         // [-]     - A body exists
    bool   SoundCondition   = false;                                                         // [-]     - Audit found nothing
    int    Faces            = 0;                                                             // [-]     - Face count
    int    Edges            = 0;                                                             // [-]     - Edge count
    int    Vertices         = 0;                                                             // [-]     - Vertex count
    int    Genus            = -1;                                                            // [-]     - Euler–Poincaré genus
    double Volume           = 0.0;                                                           // [len³]  - Enclosed volume (0 for a sheet)
    bool   SheetCondition   = false;                                                         // [-]     - Open sheet body (a lofted panel)
    double RegenerationMs   = 0.0;                                                           // [ms]    - Last replay
};

struct DraftingDocument
{
    std::string                                Title        = "Untitled";                    // [-] - Document caption
    Modeller::FeatureTree                      Tree         = {};                            // [-] - Authored feature tree
    std::map<std::string, std::string>         Captions     = {};                            // [-] - Step label → caption ("Sketch#1" → "Block1")
    std::unique_ptr<Modeller::ModelPartition>  Partition    = {};                            // [-] - Records of the last regeneration
    Modeller::RegenerationOutcome              Regeneration = {};                            // [-] - Last replay (body, names, step reports)
    DraftingBodySummary                        Summary      = {};                            // [-] - Figures of the last body
    DraftingSelection                          Selection    = {};                            // [-] - Current selection (names)
    std::vector<DraftingHistoryEntry>          UndoEntries  = {};                            // [-] - Most recent last
    std::vector<DraftingHistoryEntry>          RedoEntries  = {};                            // [-] - Most recent last
    uint64_t                                   Revision     = 0;                             // [-] - Bumped by every regeneration (scene cache key)
};

//------------------------------------------------------------------------------------------------------------------------
//                                                    PUBLIC FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

// Empty document (no steps, no body).
struct VehicleCurveFile;

[[nodiscard]] DraftingDocument CreateDraftingDocument(const std::string& Title = "Untitled");

// Independent copy (tree, captions, selection; history not copied), regenerated into its own partition.
[[nodiscard]] DraftingDocument CloneDraftingDocument(const DraftingDocument& Source);

// 💡 Commands are atomic: the tree is snapshotted, changed and regenerated; if the step the command added is refused by
//    the modeller (or the command cannot apply), the snapshot is restored and the refusal is returned. Parameter edits
//    that make a *later* step refuse are kept — the tree marks the step and keeps the previous body (plan § 3).
[[nodiscard]] DraftingCommandOutcome ConstructDraftingPrimitive(DraftingDocument& Document, const DraftingConstructRequest& Request);
[[nodiscard]] DraftingCommandOutcome BlendDraftingSelection(DraftingDocument& Document, Modeller::BlendProfile Shape, double Size);
[[nodiscard]] DraftingCommandOutcome BlendDraftingSelection(DraftingDocument& Document, const DraftingBlendRequest& Request);
[[nodiscard]] DraftingCommandOutcome ChangeDraftingSelection(DraftingDocument& Document, Modeller::FaceOperationKind Operation, double Amount);
[[nodiscard]] DraftingCommandOutcome AssignDraftingParameter(DraftingDocument& Document, const std::string& Name, double Value);
[[nodiscard]] DraftingCommandOutcome DeleteDraftingStep(DraftingDocument& Document, const std::string& Label);
[[nodiscard]] DraftingCommandOutcome ReplaceDraftingTree(DraftingDocument& Document, Modeller::FeatureTree Tree,
                                                         std::map<std::string, std::string> Captions, const std::string& Label);

// 💡 Drawing → sketch: the request becomes one Sketch step on its construction plane (caption "Line1", "Curve2", …).
//    ComposeDraftingSketch builds the same sketch without touching a document (live preview); a degenerate drawing
//    (zero size, collinear arc, coincident spline points) is refused with the reason.
[[nodiscard]] DraftingCommandOutcome DrawDraftingSketch(DraftingDocument& Document, const DraftingDrawingRequest& Request);
[[nodiscard]] std::string            ComposeDraftingSketch(const DraftingDrawingRequest& Request, Modeller::SketchModel& Sketch);

// Extrude a sketch step's profile by Distance along its plane normal; the first body is always New, a later one must
// combine (Union, Difference, Intersection). Refused (document untouched) when the profile or the Boolean refuses.
[[nodiscard]] DraftingCommandOutcome ExtrudeDraftingSketch(DraftingDocument& Document, const std::string& SketchLabel, double Distance,
                                                           Modeller::FeatureCombination Combination);

// Construction plane frame: 0 XY (normal +z), 1 XZ (x → x, sketch y → z, normal −y), 2 YZ (y → x, z → y, normal +x).
[[nodiscard]] Modeller::OrthonormalFrame DraftingSketchPlane(int GridPlane) noexcept;

// A sketch step some extrude / revolve already uses as its profile.
[[nodiscard]] bool DraftingSketchConsumedCondition(const DraftingDocument& Document, const std::string& SketchLabel);

// Undo / redo restore the tree snapshot and regenerate; false when the stack is empty.
// 💡 Vehicle plan V1: every curve of the file becomes a SpaceCurve step ("Curve#n", captioned with the loop name) fitted
//    at Tolerance — one undo entry, and all or nothing (a curve that cannot be fitted refuses the import).
[[nodiscard]] DraftingCommandOutcome ImportDraftingCurves(DraftingDocument& Document, const VehicleCurveFile& File, double Tolerance = 2.0e-4);

// 💡 V4: a Loft step through the SpaceCurve steps in the given order (Plasticity L); the result is an open sheet.
[[nodiscard]] DraftingCommandOutcome LoftDraftingCurves(DraftingDocument& Document, const std::vector<std::string>& Labels);

// 💡 V3 (Plasticity mode 1): pole Pole of a SpaceCurve moves to Position; kept as a pole edit re-applied after every re-fit.
[[nodiscard]] DraftingCommandOutcome MoveDraftingPole(DraftingDocument& Document, const std::string& Label, int Pole, Modeller::Vector3 Position);

// Toggles a SpaceCurve in the curve selection (AdditiveCondition false: it becomes the only selected curve).
void SelectDraftingCurve(DraftingDocument& Document, const std::string& Label, bool AdditiveCondition);

bool UndoDraftingCommand(DraftingDocument& Document);
bool RedoDraftingCommand(DraftingDocument& Document);

// Replays the tree into a fresh partition, refreshes the summary and drops selected names that no longer resolve.
void RegenerateDraftingDocument(DraftingDocument& Document, bool CaptureCondition = false);

// Selection by persistent name; Additive toggles membership, otherwise the selection becomes exactly this name.
void SelectDraftingName(DraftingDocument& Document, DraftingSelectionKind Kind, const std::string& Name, bool AdditiveCondition);
void ClearDraftingSelection(DraftingDocument& Document);

// Names of the edge's tangent chain on the current body (the edge itself first); just the name when it does not resolve.
[[nodiscard]] std::vector<std::string> DraftingTangentChain(const DraftingDocument& Document, const std::string& Name);

// Step caption for the outliner ("Block1 · Sketch", "Fillet3"), falling back to the label.
[[nodiscard]] std::string DescribeDraftingStep(const DraftingDocument& Document, const Modeller::FeatureStep& Step);

[[nodiscard]] const char* DraftingConstructLabel(DraftingConstructKind Kind) noexcept;
[[nodiscard]] const char* DraftingFaceOperationLabel(Modeller::FaceOperationKind Operation) noexcept;
[[nodiscard]] const char* DraftingDrawingLabel(DraftingDrawingKind Kind) noexcept;

} // namespace Frontier::DraftingWorkspace
