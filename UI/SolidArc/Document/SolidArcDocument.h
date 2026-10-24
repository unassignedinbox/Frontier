/*====================================================================================================================================
                                                          SOLIDARCDOCUMENT.H
====================================================================================================================================*/
// 🧩 SolidArc document: feature tree, atomic commands with undo / redo, selection by persistent name, regeneration

#pragma once

//------------------------------------------------------------------------------------------------------------------------
//                                                    INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "UI/SolidArc/Document/SolidArcImagePlane.h"
#include "UI/SolidArc/Document/SolidArcMatcap.h"
#include "Engine/GeometryProcessing/Drawing/DrawingSheet.h"
#include "Engine/GeometryProcessing/Feature/FeatureTree.h"
#include "Engine/GeometryProcessing/Local/FaceOperations.h"
#include "Engine/GeometryProcessing/Sketch/SketchConstraining.h"
#include "Engine/GeometryProcessing/Sketch/SketchEditing.h"
#include "Engine/GeometryProcessing/Sketch/SketchTrim.h"
#include "Engine/GeometryProcessing/Topology/ModelPartition.h"

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace Frontier::SolidArc
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    ENUMS
//------------------------------------------------------------------------------------------------------------------------

// 📝 Construct catalogue tiles. Every tile authors an editable sketch + sweep pair, never a frozen primitive body.
enum class SolidArcConstructKind : uint8_t
{
    Block    = 0,                                                                            // Rectangle sketch on XY, extruded along +z
    Cylinder = 1,                                                                            // Circle sketch on XY, extruded along +z
    Sphere   = 2,                                                                            // Half disc on XZ, revolved about its diameter
    Torus    = 3,                                                                            // Circle on XZ, revolved about a construction axis
    Cone     = 4,                                                                            // Right triangle on XZ, revolved about its upright leg
    Wedge    = 5,                                                                            // Right triangle on XZ (high side at x = X), extruded along +y
    Prism    = 6,                                                                            // Regular polygon on XY, extruded along +z
    Tube     = 7,                                                                            // Two concentric circles on XY (annulus), extruded along +z
};

// 📝 Drawing tools on the construction plane (Plasticity: Shift+A line, Shift+Q corner rectangle, Shift+W centre circle,
//    Shift+S curve; arc and slot from the Draw card). Each finished drawing becomes one editable sketch step.
enum class SolidArcDrawingKind : uint8_t
{
    Line      = 0,                                                                           // Polyline through the clicks (closes on its first point)
    Rectangle = 1,                                                                           // Two corners, or centre + corner
    Circle    = 2,                                                                           // Centre + rim, or two diameter ends
    Arc       = 3,                                                                           // Start, end, a point on the arc
    Slot      = 4,                                                                           // Two cap centres + a point on the rim
    Curve     = 5,                                                                           // Spline through / on the clicks (form 1 … 4)
    Polygon   = 6,                                                                           // Regular polygon: centre + a vertex (Sides)
};

enum class SolidArcSelectionKind : uint8_t
{
    Face = 0,                                                                                // A face (persistent name)
    Edge = 1,                                                                                // An edge (persistent name)
    Body = 2,                                                                                // A whole body (key = creating step label)
};

//------------------------------------------------------------------------------------------------------------------------
//                                                    STRUCTS
//------------------------------------------------------------------------------------------------------------------------

// 📝 Only the fields of the chosen kind are read. Position: block / wedge corner (minimum x, y, z), cylinder / cone / prism /
//    tube base centre, sphere / torus centre. Cone: Radius at the base, apex Height above it. Wedge: Height at x = X falling to
//    0 at x = X + Width.
struct SolidArcConstructRequest
{
    SolidArcConstructKind        Kind        = SolidArcConstructKind::Block;                 // [-]   - Tile
    GeometryProcessing::FeatureCombination Combination = GeometryProcessing::FeatureCombination::Union;           // [-]   - With the target body (first body: New; Separate = new body)
    std::string                  Target      = {};                                           // [-]   - Body key the Boolean acts on (empty: the active body)
    GeometryProcessing::Vector3            Position    = { 0.0, 0.0, 0.0 };                             // [len] - Placement
    double                       Width       = 2.0;                                          // [len] - Block x extent
    double                       Depth       = 1.5;                                          // [len] - Block y extent
    double                       Height      = 1.0;                                          // [len] - Block / cylinder z extent
    double                       Radius      = 0.5;                                          // [len] - Cylinder / sphere radius
    double                       Major       = 1.5;                                          // [len] - Torus axis → tube centre
    double                       Minor       = 0.4;                                          // [len] - Torus tube radius
    int                          Axis        = 2;                                            // [-]   - Cylinder axis: 0 x · 1 y · 2 z (Height runs along +axis)
    double                       Inner       = 0.3;                                          // [len] - Tube bore radius (< Radius)
    int                          Sides       = 6;                                            // [-]   - Prism sides (3 … 64)
    double                       Angle       = 0.0;                                          // [rad] - Prism: direction of the first vertex from the centre
};

// 📝 Points are construction-plane coordinates (GridPlane 0 XY, 1 XZ, 2 YZ; see SolidArcSketchPlane). Only the fields of
//    the kind are read; a Line or Curve is closed when ClosedCondition (its last point joins its first).
struct SolidArcDrawingRequest
{
    SolidArcDrawingKind            Kind            = SolidArcDrawingKind::Line;              // [-]   - Tool
    int                            GridPlane       = 0;                                      // [-]   - 0 XY · 1 XZ · 2 YZ
    std::vector<GeometryProcessing::Vector2> Points          = {};                                     // [len] - Clicks in plane coordinates
    bool                           ClosedCondition = false;                                  // [-]   - Line / curve: close the chain
    bool                           CentreCondition = false;                                  // [-]   - Rectangle: centre + corner · circle: centre + rim (else two diameter ends)
    GeometryProcessing::SketchSplineForm     Form            = GeometryProcessing::SketchSplineForm::CubicSpline; // [-]  - Curve form (Plasticity 1 … 4)
    int                            Sides           = 6;                                      // [-]   - Polygon sides (3 … 64)
};

// 📝 Selection is stored as persistent names, so it survives regeneration whenever the names still resolve.
struct SolidArcSelection
{
    std::vector<std::string> Faces = {};                                                     // [-] - Selected face names
    std::vector<std::string> Edges = {};                                                     // [-] - Selected edge names
    std::vector<std::string> Curves = {};                                                    // [-] - Selected SpaceCurve labels, in pick order (loft order)
    std::vector<std::string> Bodies = {};                                                    // [-] - Selected body keys, in pick order (first = Boolean target)
    SolidArcSelectionKind    LastKind = SolidArcSelectionKind::Face;                          // [-] - Kind of the most recent pick (active operand)
};

struct SolidArcCutRequest
{
    std::string       Target    = {};                                                        // [-] - Body key (empty = active body)
    std::string       Cutter    = {};                                                        // [-] - Sketch step or SpaceCurve label
    GeometryProcessing::Vector3 Direction = { 0.0, 0.0, -1.0 };                                        // [-] - SpaceCurve cutters: projection direction
};

// 📝 Pick is in the sketch's plane coordinates (the pointer met that plane).
struct SolidArcTrimRequest
{
    std::string       Sketch = {};                                                           // [-]   - Sketch step label
    int               Curve  = -1;                                                           // [-]   - Curve index in that sketch
    GeometryProcessing::Vector2 Pick   = {};                                                           // [len] - Where the pointer met the sketch plane
};

// Plasticity H family: H hides the selected bodies, Shift+H every other body, Alt+H shows all, Ctrl+H swaps shown and hidden.
enum class SolidArcVisibilityVerb : uint8_t
{
    HideSelected   = 0,
    HideUnselected = 1,
    UnhideAll      = 2,
    InvertHidden   = 3,
};

// 📝 Variable when EndSize is set: Size at each edge's anchor end, EndSize at its other end. The anchor is the edge's
//    start vertex on the current body, or its end vertex when ReversedCondition (Plasticity F flips it); it is stored as
//    a persistent vertex name, so a regeneration that reverses the edge keeps each size at its corner.
struct SolidArcBlendRequest
{
    GeometryProcessing::BlendProfile Shape             = GeometryProcessing::BlendProfile::Fillet;               // [-]   - Cross-section
    double                 Size              = 0.25;                                         // [len] - Radius / setback (at the anchors)
    std::optional<double>  EndSize           = {};                                           // [len] - Size at the far ends (empty = constant)
    bool                   ReversedCondition = false;                                        // [-]   - Anchor at each edge's end vertex
};

struct SolidArcCommandOutcome
{
    bool        AcceptedCondition = false;                                                   // [-]  - Document changed
    std::string Label             = {};                                                      // [-]  - History caption ("Block1", "Fillet r 0.2")
    std::string Refusal           = {};                                                      // [-]  - Why not, when refused (document untouched)
    double      Milliseconds      = 0.0;                                                     // [ms] - Command + regeneration time
};

// 📝 Figures of one body (Inspector body list, Outliner).
struct SolidArcBodyFigures
{
    std::string Key            = {};                                                         // [-]    - Creating step label ("Extrude#2")
    std::string Caption        = {};                                                         // [-]    - Shown name ("Block1")
    bool        SoundCondition = false;                                                      // [-]    - Audit found nothing
    bool        SheetCondition = false;                                                      // [-]    - Open sheet body (a lofted panel)
    int         Faces          = 0;                                                          // [-]    - Face count
    int         Edges          = 0;                                                          // [-]    - Edge count
    int         Vertices       = 0;                                                          // [-]    - Vertex count
    int         Genus          = -1;                                                         // [-]    - Euler–Poincaré genus
    double      Volume         = 0.0;                                                        // [len³] - Enclosed volume (0 for a sheet)
};

// 📝 Part figures shown by the Inspector (recomputed on every regeneration): totals over every body, plus each body.
struct SolidArcBodySummary
{
    bool   PresentCondition = false;                                                         // [-]     - At least one body exists
    bool   SoundCondition   = false;                                                         // [-]     - Every body audits sound
    int    Faces            = 0;                                                             // [-]     - Face count (all bodies)
    int    Edges            = 0;                                                             // [-]     - Edge count (all bodies)
    int    Vertices         = 0;                                                             // [-]     - Vertex count (all bodies)
    int    Genus            = -1;                                                            // [-]     - Sum of the bodies' genera
    double Volume           = 0.0;                                                           // [len³]  - Sum of the solid bodies' volumes
    bool   SheetCondition   = false;                                                         // [-]     - Some body is an open sheet (a lofted panel)
    double RegenerationMs   = 0.0;                                                           // [ms]    - Last replay
    std::vector<SolidArcBodyFigures> Bodies = {};                                            // [-]     - One per body, in creation order
};

// 📝 One undo entry: the whole tree before the command (trees are small; replay rebuilds the body exactly). Recent entries
//    also keep the replay checkpoints of that moment (shared, immutable), so undo / redo / a rolled-back command restore
//    every step instead of replaying it.
struct SolidArcHistoryEntry
{
    GeometryProcessing::FeatureTree              Tree     = {};                                        // [-] - Tree before the command
    std::map<std::string, std::string> Captions = {};                                        // [-] - Step captions before the command
    std::string                        Label    = {};                                        // [-] - Command caption
    SolidArcAppearance                 Appearance = {};                                      // [-] - Matcaps before the command
    bool                               AppearanceCondition = false;                          // [-] - Matcap-only entry: restore without regenerating
    std::shared_ptr<const GeometryProcessing::RegenerationCache> Replay = {};                          // [-] - Checkpoints then (recent entries only)
    std::shared_ptr<const GeometryProcessing::ReplayCheckpoint>  SummaryCheckpoint = {};               // [-] - Checkpoint the summary below was measured on
    SolidArcBodySummary                                Summary = {};                         // [-] - Body figures then (reused when every step restores)
};

// 📝 Section plane choice (Section card, Alt+S). World planes pass through Offset on their axis; the Face plane is the
//    selected planar face moved Offset along its outward normal (negative = into the body). The kept side is the
//    plane's +normal side, as in the kernel; defaults remove the half that faces the isometric camera
//    (X: n = −x̂, Y: n = +ŷ, Z: n = −ẑ, Face: n = −outward). Flipped negates n.
enum class SolidArcSectionAxis : uint8_t
{
    X    = 0,                                                                                // Plane x = Offset
    Y    = 1,                                                                                // Plane y = Offset
    Z    = 2,                                                                                // Plane z = Offset
    Face = 3,                                                                                // Selected planar face, offset along its normal
};

struct SolidArcSectionRequest
{
    SolidArcSectionAxis Axis    = SolidArcSectionAxis::X;                                    // [-]   - Plane family
    double              Offset  = 0.0;                                                       // [len] - Coordinate / normal offset
    bool                Flipped = false;                                                     // [-]   - Keep the other half
    std::string         Face    = {};                                                        // [-]   - Face name (Axis = Face)
};

// 📝 Title block fields; an empty Title prints the document title.
struct SolidArcTitleFields
{
    std::string Title         = {};                                                          // [-] - Part name (empty: document title)
    std::string DrawingNumber = "DWG-0001";                                                  // [-] - Identifier
    std::string Author        = {};                                                          // [-] - Drawn by
    std::string Date          = {};                                                          // [-] - Issue date
    std::string Material      = {};                                                          // [-] - Material
    std::string Revision      = "A";                                                         // [-] - Revision letter
    std::string Company       = "Frontier";                                                  // [-] - Organisation
};

// 📝 Saved with the document ([DrawingSheet], only when not the defaults), not in the undo history — like image planes.
//    First angle by default (ISO 5456-2, SANS 10111); Scale 0 picks the largest standard scale that fits. Section A-A
//    cuts along a Section-card plane (Alt+S semantics: the +normal side is kept and seen from the removed side).
struct SolidArcDrawingSheetSettings
{
    GeometryProcessing::DrawingPaperSize       Paper               = GeometryProcessing::DrawingPaperSize::A3;    // [-] - ISO 216, landscape
    GeometryProcessing::DrawingProjectionAngle Angle               = GeometryProcessing::DrawingProjectionAngle::First; // [-] - Layout and symbol
    double                           Scale               = 0.0;                               // [-] - Paper : model, 0 = automatic
    bool                             IsometricEnabled    = true;                              // [-] - Isometric in the free quadrant
    bool                             SectionEnabled      = false;                             // [-] - Section A-A instead of the isometric
    SolidArcSectionRequest           Section             = { SolidArcSectionAxis::Y, 0.0, false, {} }; // [-] - Plane of section A-A
    bool                             HiddenLinesEnabled  = true;                              // [-] - Dashed hidden lines
    bool                             TangentEdgesEnabled = true;                              // [-] - Thin tangent edges
    bool                             DimensionsEnabled   = true;                              // [-] - Automatic dimensions + centre marks
    SolidArcTitleFields              Title               = {};                                // [-] - Title block
};

struct SolidArcDocument
{
    std::string                                Title        = "Untitled";                    // [-] - Document caption
    GeometryProcessing::FeatureTree                      Tree         = {};                            // [-] - Authored feature tree
    std::map<std::string, std::string>         Captions     = {};                            // [-] - Step label → caption ("Sketch#1" → "Block1")
    std::unique_ptr<GeometryProcessing::ModelPartition>  Partition    = {};                            // [-] - Records of the last regeneration
    GeometryProcessing::RegenerationOutcome              Regeneration = {};                            // [-] - Last replay (body, names, step reports)
    SolidArcBodySummary                        Summary      = {};                            // [-] - Figures of the last body
    SolidArcSelection                          Selection    = {};                            // [-] - Current selection (names)
    std::vector<SolidArcHistoryEntry>          UndoEntries  = {};                            // [-] - Most recent last
    std::vector<SolidArcHistoryEntry>          RedoEntries  = {};                            // [-] - Most recent last
    uint64_t                                   Revision     = 0;                             // [-] - Bumped by every regeneration (scene cache key)
    std::vector<SolidArcImagePlane>            ImagePlanes  = {};                            // [-] - Blueprint references (saved, not in the history)
    SolidArcAppearance                         Appearance   = {};                            // [-] - Body / face matcaps (saved, undoable)
    SolidArcDrawingSheetSettings               DrawingSheet = {};                            // [-] - Alt+D sheet settings (saved, not in the history)
    GeometryProcessing::RegenerationCache                Replay       = {};                            // [-] - Step checkpoints of the last replay (incremental regeneration)
    std::shared_ptr<const GeometryProcessing::ReplayCheckpoint> SummaryCheckpoint = {};                // [-] - Final checkpoint Summary was measured on (figures reused when a
                                                                                             //       regeneration restores every step from it: undo / redo / clone)
};

//------------------------------------------------------------------------------------------------------------------------
//                                                    PUBLIC FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

// Empty document (no steps, no body).
struct VehicleCurveFile;

[[nodiscard]] SolidArcDocument CreateSolidArcDocument(const std::string& Title = "Untitled");

// Independent copy (tree, captions, selection; history not copied), regenerated into its own partition. The replay
// checkpoints are shared, so the copy restores every step instead of replaying the tree (preview staging is instant).
[[nodiscard]] SolidArcDocument CloneSolidArcDocument(const SolidArcDocument& Source);

// 💡 Commands are atomic: the tree is snapshotted, changed and regenerated; if the step the command added is refused by
//    the modeller (or the command cannot apply), the snapshot is restored and the refusal is returned. Parameter edits
//    that make a *later* step refuse are kept — the tree marks the step and keeps the previous body (plan § 3).
[[nodiscard]] SolidArcCommandOutcome ConstructSolidArcPrimitive(SolidArcDocument& Document, const SolidArcConstructRequest& Request);
[[nodiscard]] SolidArcCommandOutcome BlendSolidArcSelection(SolidArcDocument& Document, GeometryProcessing::BlendProfile Shape, double Size);
[[nodiscard]] SolidArcCommandOutcome BlendSolidArcSelection(SolidArcDocument& Document, const SolidArcBlendRequest& Request);
// Direction (gizmo): Tweak moves along it by Amount, Rotate turns about it (through each face centre) by Amount [rad].
[[nodiscard]] SolidArcCommandOutcome ChangeSolidArcSelection(SolidArcDocument& Document, GeometryProcessing::FaceOperationKind Operation, double Amount,
                                                             std::optional<GeometryProcessing::Vector3> Direction = std::nullopt);
// Inset (Plasticity O on faces · I): border ring Distance wide; the inner face moves Depth along the outward normal (− pocket).
[[nodiscard]] SolidArcCommandOutcome InsetSolidArcSelection(SolidArcDocument& Document, double Distance, double Depth);
[[nodiscard]] SolidArcCommandOutcome AssignSolidArcParameter(SolidArcDocument& Document, const std::string& Name, double Value);
[[nodiscard]] SolidArcCommandOutcome DeleteSolidArcStep(SolidArcDocument& Document, const std::string& Label);
[[nodiscard]] SolidArcCommandOutcome ReplaceSolidArcTree(SolidArcDocument& Document, GeometryProcessing::FeatureTree Tree,
                                                         std::map<std::string, std::string> Captions, const std::string& Label);

// 💡 Drawing → sketch: the request becomes one Sketch step on its construction plane (caption "Line1", "Curve2", …).
//    ComposeSolidArcSketch builds the same sketch without touching a document (live preview); a degenerate drawing
//    (zero size, collinear arc, coincident spline points) is refused with the reason.
[[nodiscard]] SolidArcCommandOutcome DrawSolidArcSketch(SolidArcDocument& Document, const SolidArcDrawingRequest& Request);
[[nodiscard]] std::string            ComposeSolidArcSketch(const SolidArcDrawingRequest& Request, GeometryProcessing::SketchModel& Sketch);

// Extrude a sketch step's profile by Distance along its plane normal; the first body is always New, a later one must
// combine (Union, Difference, Intersection). Refused (document untouched) when the profile or the Boolean refuses.
[[nodiscard]] SolidArcCommandOutcome ExtrudeSolidArcSketch(SolidArcDocument& Document, const std::string& SketchLabel, double Distance,
                                                           GeometryProcessing::FeatureCombination Combination, const std::string& Target = {});

// Plasticity Q (command:boolean): Combine steps merging each tool body into Target (Union / Difference / Intersection);
// the tools are consumed. One undo entry; refused (document untouched) unless every Boolean succeeds.
[[nodiscard]] SolidArcCommandOutcome CombineSolidArcBodies(SolidArcDocument& Document, const std::string& Target,
                                                           const std::vector<std::string>& Tools, GeometryProcessing::FeatureCombination Combination);

// 💡 Plasticity C (command:cut): one Cut step splitting Target (empty = the active body) by Cutter, which is a sketch step
//    (its closed profile cuts out its interior, an open chain splits along itself) or a SpaceCurve projected along
//    Direction (the view axis). The outside piece keeps its key; the inside piece becomes body "Cut#n" (caption Cut1 …).
//    Refused (document untouched) when the cut misses or cannot be closed.
[[nodiscard]] SolidArcCommandOutcome CutSolidArcBody(SolidArcDocument& Document, const SolidArcCutRequest& Request);

// 💡 Plasticity T (command:trim): the piece of the curve under Pick, between its crossings with the sketch's other curves
//    and with every coplanar sketch, comes off (GeometryProcessing::TrimSketchCurve); one undo entry. A sketch left without
//    curves is deleted when nothing uses it. Refused (document untouched) when a step that regenerated before now refuses
//    (e.g. an extrude losing its closed profile), or when the trim itself refuses.
[[nodiscard]] SolidArcCommandOutcome TrimSolidArcSketch(SolidArcDocument& Document, const SolidArcTrimRequest& Request);
// The same trim on the solved sketch without touching the document (hover preview); Removed is in plane coordinates.
[[nodiscard]] GeometryProcessing::SketchTrimOutcome PreviewSolidArcTrim(const SolidArcDocument& Document, const SolidArcTrimRequest& Request);

// Plasticity X on bodies: one Remove step per key, one undo entry; refused (document untouched) for an unknown key.
[[nodiscard]] SolidArcCommandOutcome RemoveSolidArcBodies(SolidArcDocument& Document, const std::vector<std::string>& Keys);

// Plasticity G / R / S: one Transform step per body applying Map (x′ = X·x + Y·y + Z·z + Shift); Verb captions the steps
// ("Move", "Rotate", "Scale"). Names, selection and every later step that references the bodies follow them.
[[nodiscard]] SolidArcCommandOutcome TransformSolidArcBodies(SolidArcDocument& Document, const std::vector<std::string>& Keys,
                                                             const GeometryProcessing::AffineMap& Map, const std::string& Verb);

// 📝 Gizmo on sketches (loft sections): every listed Sketch step's plane is carried by the rigid Map (rotation +
//    translation; the elevation rides along its normal). Scale and mirror maps are refused (🚧), as are labels that are
//    not sketches. Later steps (lofts through the sketch, extrudes of it) regenerate from the new plane.
[[nodiscard]] SolidArcCommandOutcome PlaceSolidArcSketches(SolidArcDocument& Document, const std::vector<std::string>& Labels,
                                                           const GeometryProcessing::AffineMap& Map, const std::string& Verb);

// Shift+D / Ctrl+V: one Duplicate step per body, placed by Map; each copy is a new body (key = its step label), the last
// one active, the copies selected. Source (paste) copies each body as it stood right after that step — the clipboard.
[[nodiscard]] SolidArcCommandOutcome DuplicateSolidArcBodies(SolidArcDocument& Document, const std::vector<std::string>& Keys,
                                                             const GeometryProcessing::AffineMap& Map, const std::string& Source = {});

// 📝 Alt+J (Fusion Joint; Plasticity has none): place the body owning MovingName onto the body owning FixedName by a
//    joint between the two joint origins (an edge or a face each; Assembly/JointSolver.h reads the origin off it).
//    Angles in degrees; each value becomes a step parameter (caption.Angle, .Offset, .Slide, .SecondSlide, .Pitch, .Roll)
//    so the step card and the parameter fields drag it, replaying from the joint only.
struct SolidArcJointRequest
{
    SolidArcSelectionKind MovingKind  = SolidArcSelectionKind::Edge;                         // [-]   - Edge or face
    std::string           MovingName  = {};                                                  // [-]   - Joint origin on the moving body
    SolidArcSelectionKind FixedKind   = SolidArcSelectionKind::Edge;                         // [-]   - Edge or face
    std::string           FixedName   = {};                                                  // [-]   - Joint origin on the fixed body
    GeometryProcessing::JointKind   Kind        = GeometryProcessing::JointKind::Revolute;                       // [-]   - Fusion joint type
    double                Angle       = 0.0;                                                 // [deg] - Turn about the joint 𝑧 (ball: yaw)
    double                Offset      = 0.0;                                                 // [len] - Along the joint 𝑧
    double                Slide       = 0.0;                                                 // [len] - Slider / cylindrical 𝑧 · pin-slot / planar 𝑥
    double                SecondSlide = 0.0;                                                 // [len] - Planar 𝑦
    double                Pitch       = 0.0;                                                 // [deg] - Ball: about 𝑦
    double                Roll        = 0.0;                                                 // [deg] - Ball: about 𝑥
    bool                  Flipped     = false;                                               // [-]   - Origins aligned instead of facing
};

// 📝 The joint origin an edge or a face offers (hover glyph of the Joint card), with the body that owns it.
struct SolidArcJointOrigin
{
    std::string                Body        = {};                                             // [-] - Owning body key
    GeometryProcessing::OrthonormalFrame Frame       = {};                                             // [-] - Origin and axes (world)
    std::string                Description = {};                                             // [-] - "circle centre", "plane centroid", …
    std::string                Refusal     = {};                                             // [-] - Empty when the entity offers one
};

[[nodiscard]] SolidArcJointOrigin ResolveSolidArcJointOrigin(const SolidArcDocument& Document, SolidArcSelectionKind Kind, const std::string& Name);

// One Joint step: the moving body (and every body jointed to it) moves; refused (document untouched) when both origins
// are on one body, the moving body is already jointed, or the joint would close a loop.
[[nodiscard]] SolidArcCommandOutcome JointSolidArcBodies(SolidArcDocument& Document, const SolidArcJointRequest& Request);

// Plasticity H / Shift+H / Alt+H / Ctrl+H (appearance: undoable, saved, no regeneration).
[[nodiscard]] SolidArcCommandOutcome ChangeSolidArcVisibility(SolidArcDocument& Document, SolidArcVisibilityVerb Verb);

// Outliner eye: hides one body, or shows it again (appearance: undoable, saved, no regeneration).
[[nodiscard]] SolidArcCommandOutcome ToggleSolidArcBodyVisibility(SolidArcDocument& Document, const std::string& Key);

// Plasticity J: the coplanar sketches merge into the first (ends welded, frame constraints carried); the others are
// removed. Refused when a merged sketch feeds a later step.
[[nodiscard]] SolidArcCommandOutcome JoinSolidArcSketches(SolidArcDocument& Document, const std::vector<std::string>& Sketches);

// J from one sketch: Sketch first, then every coplanar sketch no later step uses that touches the group (end to end or
// crossing), transitively, in tree order — what Plasticity's join-curves would weld.
[[nodiscard]] std::vector<std::string> SolidArcJoinCandidates(const SolidArcDocument& Document, const std::string& Sketch);

// Plasticity O: a new sketch step holding the exact offset of every chain of Sketch (Distance > 0: outward / left).
[[nodiscard]] SolidArcCommandOutcome OffsetSolidArcSketch(SolidArcDocument& Document, const std::string& Sketch, double Distance);
[[nodiscard]] GeometryProcessing::SketchOffsetOutcome PreviewSolidArcOffset(const SolidArcDocument& Document, const std::string& Sketch, double Distance);

// Plasticity P: a tube of Radius along a one-curve sketch or a SpaceCurve, combined with Target (Separate: a new body).
[[nodiscard]] SolidArcCommandOutcome PipeSolidArcPath(SolidArcDocument& Document, const std::string& Path, double Radius,
                                                      GeometryProcessing::FeatureCombination Combination = GeometryProcessing::FeatureCombination::Separate,
                                                      const std::string& Target = {});

// 📝 Revolve axis (Alt+R): a line of the sketch by identifier (construction or profile edge), else a point + direction
//    in sketch coordinates (the sketch x- / y-axis).
struct SolidArcRevolveAxis
{
    int               Line      = -1;                                                        // [-] - Sketch line identifier (−1: Origin + t·Direction)
    GeometryProcessing::Vector2 Origin    = {};                                                        // [len] - A point of the axis
    GeometryProcessing::Vector2 Direction = { 0.0, 1.0 };                                              // [-]   - Axis direction
};

// The axis Alt+R starts with: the sketch's first construction line, else the sketch y-axis when the profile lies on one
// side of it (touching allowed), else the x-axis when that one is clear, else the y-axis (the revolve then refuses).
[[nodiscard]] SolidArcRevolveAxis ProposeSolidArcRevolveAxis(const SolidArcDocument& Document, const std::string& Sketch);
// "Line3", "sketch y-axis", "sketch x-axis" or "axis (x, y) + t·(dx, dy)".
[[nodiscard]] std::string DescribeSolidArcRevolveAxis(const SolidArcDocument& Document, const std::string& Sketch, const SolidArcRevolveAxis& Axis);

// Alt+R (Plasticity revolution, no default key): the profile of Sketch turned by Angle (0, 2π] about Axis, combined like
// an extrude (the first body is New; later ones Union / Difference / Intersection with Target, or New body).
[[nodiscard]] SolidArcCommandOutcome RevolveSolidArcSketch(SolidArcDocument& Document, const std::string& Sketch, const SolidArcRevolveAxis& Axis, double Angle,
                                                           GeometryProcessing::FeatureCombination Combination = GeometryProcessing::FeatureCombination::Separate,
                                                           const std::string& Target = {});

// Plasticity Evolution (Shift+P / Alt+E): the profile of sketch Profile swept along Path (a one-curve sketch or a
// SpaceCurve), combined like an extrude.
[[nodiscard]] SolidArcCommandOutcome EvolveSolidArcProfile(SolidArcDocument& Document, const std::string& Profile, const std::string& Path,
                                                           GeometryProcessing::FeatureCombination Combination = GeometryProcessing::FeatureCombination::Separate,
                                                           const std::string& Target = {});
// The path Evolution starts with: the first selected curve, else the newest other sketch holding exactly one drawn open
// curve (empty: none).
[[nodiscard]] std::string ProposeSolidArcEvolutionPath(const SolidArcDocument& Document, const std::string& Profile);

// 📝 Constraint commands on a sketch step (the K session). Each one solves the edited sketch first, with its parameters
//    bound as regeneration binds them: a redundant or conflicting constraint is refused naming the constraints it
//    duplicates or contradicts, and on success the solved positions are written back into the step, so the sketch stays
//    where it is shown and every later step (extrudes of it) regenerates. One undo entry each.
struct SolidArcSketchDiagnosis
{
    bool                         PresentCondition = false;                                   // [-] - The label is a sketch step
    GeometryProcessing::SketchModel        Sketch           = {};                                      // [-] - As solved (parameters bound)
    GeometryProcessing::SketchSolveOutcome Solve            = {};                                      // [-] - DOF, statuses, fixed points
};

[[nodiscard]] SolidArcSketchDiagnosis DiagnoseSolidArcSketch(const SolidArcDocument& Document, const std::string& Sketch);

// 📝 Geometry picked in an earlier sketch step while constraining this one. Linking appends it as linked points (held
//    at their projection into this plane) and construction curves, reusing whatever is linked already, so the kernel
//    proposals run on one model; the constraint command links the same picks again inside its own undo entry.
struct SolidArcSketchPick
{
    std::string          Sketch = {};                                                        // [-] - Sketch step label (empty: the edited sketch)
    GeometryProcessing::SketchPick Pick   = {};                                                        // [-] - Point / curve in that sketch
};

struct SolidArcLinkedPicks
{
    GeometryProcessing::SketchModel             Sketch   = {};                                         // [-] - As solved, the linked geometry appended
    std::vector<GeometryProcessing::SketchPick> Picks    = {};                                         // [-] - The picks in that model
    int                               Appended = 0;                                          // [-] - Points + curves it gained
    std::string                       Refusal  = {};                                         // [-] - Why a pick cannot be linked
};

[[nodiscard]] SolidArcLinkedPicks LinkSolidArcPicks(const SolidArcDocument& Document, const std::string& Sketch, const std::vector<SolidArcSketchPick>& Picks);

// Sketch steps Sketch may link to (the ones before it), in tree order.
[[nodiscard]] std::vector<std::string> SolidArcLinkSources(const SolidArcDocument& Document, const std::string& Sketch);

// Appends Constraint (an unlabelled one gets the next "H3" / "D2" … label of its kind). With Links, its indices refer to
// LinkSolidArcPicks(Links)'s model and the linked geometry is added with it. G2 replaces the G1 on the same joint.
[[nodiscard]] SolidArcCommandOutcome ConstrainSolidArcSketch(SolidArcDocument& Document, const std::string& Sketch, const GeometryProcessing::SketchConstraint& Constraint,
                                                             const std::vector<SolidArcSketchPick>& Links = {});

// SolveSpace E: a dimension toggled between driving and reference (driven, display-only). Becoming driving keeps the
// measured value and is refused when it would be redundant or conflicting; a parameter-bound one stays driving.
[[nodiscard]] SolidArcCommandOutcome ToggleSolidArcReference(SolidArcDocument& Document, const std::string& Sketch, int Constraint);

// Delete on linked geometry: the link dropped (a circle's radius too), the geometry stays where it is, now free.
[[nodiscard]] SolidArcCommandOutcome UnlinkSolidArcGeometry(SolidArcDocument& Document, const std::string& Sketch, GeometryProcessing::SketchPick Pick);

// True when the point / curve is held by a link to another sketch.
[[nodiscard]] bool SolidArcLinkedCondition(const GeometryProcessing::SketchModel& Sketch, GeometryProcessing::SketchPick Pick) noexcept;

[[nodiscard]] SolidArcCommandOutcome RemoveSolidArcConstraint(SolidArcDocument& Document, const std::string& Sketch, int Constraint);

// A dimension's new value ([len], angles [rad]); one bound to a document parameter changes the parameter.
[[nodiscard]] SolidArcCommandOutcome ChangeSolidArcDimension(SolidArcDocument& Document, const std::string& Sketch, int Constraint, double Value);

// SolveSpace drag: the point pinned at Target (sketch-plane coordinates) with the rest following, else as near as its
// constraints let it; refused when it cannot move at all.
[[nodiscard]] SolidArcCommandOutcome DragSolidArcSketchPoint(SolidArcDocument& Document, const std::string& Sketch, int Point, GeometryProcessing::Vector2 Target);
[[nodiscard]] GeometryProcessing::SketchDragOutcome PreviewSolidArcDrag(const SolidArcDocument& Document, const std::string& Sketch, int Point, GeometryProcessing::Vector2 Target);

// "H3", "D2": the label the next constraint of that kind gets in Sketch.
[[nodiscard]] std::string NextSolidArcConstraintLabel(const GeometryProcessing::SketchModel& Sketch, GeometryProcessing::SketchConstraintKind Kind);

// Key of the body carrying a face / edge name, or the key itself for Body; empty when no body has it.
[[nodiscard]] std::string SolidArcBodyOfName(const SolidArcDocument& Document, SolidArcSelectionKind Kind, const std::string& Name);

// Shown name of a body: the caption of its creating step ("Block1"), else the key.
[[nodiscard]] std::string SolidArcBodyCaption(const SolidArcDocument& Document, const std::string& Key);

// Construction plane frame: 0 XY (normal +z), 1 XZ (x → x, sketch y → z, normal −y), 2 YZ (y → x, z → y, normal +x).
[[nodiscard]] GeometryProcessing::OrthonormalFrame SolidArcSketchPlane(int GridPlane) noexcept;

// A sketch step some extrude / revolve already uses as its profile.
[[nodiscard]] bool SolidArcSketchConsumedCondition(const SolidArcDocument& Document, const std::string& SketchLabel);

// Undo / redo restore the tree snapshot and regenerate; false when the stack is empty.
// 💡 Vehicle plan V1: every curve of the file becomes a SpaceCurve step ("Curve#n", captioned with the loop name) fitted
//    at Tolerance — one undo entry, and all or nothing (a curve that cannot be fitted refuses the import).
[[nodiscard]] SolidArcCommandOutcome ImportSolidArcCurves(SolidArcDocument& Document, const VehicleCurveFile& File, double Tolerance = 2.0e-4);

// 💡 Import STEP (priority 4 X4): one Import step per solid of a ReadSolidArcStepFile reading (one undo entry), each a
//    body of its own captioned by the file name ("Boss", "Boss.2" …), its canonical STEP text embedded in the document.
struct SolidArcStepReading;
[[nodiscard]] SolidArcCommandOutcome ImportSolidArcStep(SolidArcDocument& Document, const SolidArcStepReading& Reading);

// 💡 V4–V6: a Loft step through the SpaceCurve steps and / or one-chain sketches (line, polyline, arc, slot, rectangle,
//    circle, spline: Sketch/SketchSection.h) in the given order (Plasticity L), G0 / G1 / G2 (Continuity 0–2) to a
//    panel already ending on its first or last section; when the picked curves cross each other (DetectCurveNetwork)
//    a Network step (Gordon / Coons, V5) is made instead — order does not matter there, and G1 / G2 refuses (🚧).
[[nodiscard]] SolidArcCommandOutcome LoftSolidArcCurves(SolidArcDocument& Document, const std::vector<std::string>& Labels, int Continuity = 0);

// 💡 V7 (Plasticity Alt+X, gizmo x / y / z): Mirror step through the world plane normal to Axis (0 x · 1 y · 2 z) at the
//    origin; SpaceCurves ending on that plane are re-fitted to leave it along its normal, so lofted halves meet G1.
[[nodiscard]] SolidArcCommandOutcome MirrorSolidArcPanels(SolidArcDocument& Document, int Axis = 0);

// 💡 V7: Close step — plane caps on the planar rim loops of the sewn panels; the result is a solid (or a named refusal).
[[nodiscard]] SolidArcCommandOutcome CloseSolidArcPanels(SolidArcDocument& Document);

// 💡 V3 (Plasticity mode 1): pole Pole of a SpaceCurve moves to Position; kept as a pole edit re-applied after every re-fit.
[[nodiscard]] SolidArcCommandOutcome MoveSolidArcPole(SolidArcDocument& Document, const std::string& Label, int Pole, GeometryProcessing::Vector3 Position);

// Toggles a SpaceCurve in the curve selection (AdditiveCondition false: it becomes the only selected curve).
void SelectSolidArcCurve(SolidArcDocument& Document, const std::string& Label, bool AdditiveCondition);

// 💡 Matcaps (Plasticity M / Alt+M). Assign: the selected faces take Matcap, or the body when no face is selected. Remove:
//    the selected faces fall back to the body matcap, or the body returns to Studio. Neither regenerates the body (the
//    Revision stays); both are undo entries. Unknown matcap names refuse.
[[nodiscard]] SolidArcCommandOutcome AssignSolidArcMatcap(SolidArcDocument& Document, const std::string& Matcap);
[[nodiscard]] SolidArcCommandOutcome RemoveSolidArcMatcap(SolidArcDocument& Document);
// A user picture under a new name (not Studio, a built-in or an existing custom name); refused when it does not decode.
[[nodiscard]] SolidArcCommandOutcome AddSolidArcMatcapPicture(SolidArcDocument& Document, const std::string& Name, const std::string& Path);
bool UndoSolidArcCommand(SolidArcDocument& Document);
bool RedoSolidArcCommand(SolidArcDocument& Document);

// Replays the tree into a fresh partition, refreshes the summary and drops selected names that no longer resolve.
void RegenerateSolidArcDocument(SolidArcDocument& Document, bool CaptureCondition = false);

// Selection by persistent name; Additive toggles membership, otherwise the selection becomes exactly this name.
void SelectSolidArcName(SolidArcDocument& Document, SolidArcSelectionKind Kind, const std::string& Name, bool AdditiveCondition);
void ClearSolidArcSelection(SolidArcDocument& Document);

// Names of the edge's tangent chain on the current body (the edge itself first); just the name when it does not resolve.
[[nodiscard]] std::vector<std::string> SolidArcTangentChain(const SolidArcDocument& Document, const std::string& Name);

// Step caption for the outliner ("Block1 · Sketch", "Fillet3"), falling back to the label.
[[nodiscard]] std::string DescribeSolidArcStep(const SolidArcDocument& Document, const GeometryProcessing::FeatureStep& Step);

[[nodiscard]] const char* SolidArcConstructLabel(SolidArcConstructKind Kind) noexcept;
[[nodiscard]] const char* SolidArcFaceOperationLabel(GeometryProcessing::FaceOperationKind Operation) noexcept;
[[nodiscard]] const char* SolidArcDrawingLabel(SolidArcDrawingKind Kind) noexcept;

} // namespace Frontier::SolidArc
