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

// 📝 Selection is stored as persistent names, so it survives regeneration whenever the names still resolve.
struct DraftingSelection
{
    std::vector<std::string> Faces = {};                                                     // [-] - Selected face names
    std::vector<std::string> Edges = {};                                                     // [-] - Selected edge names
    DraftingSelectionKind    LastKind = DraftingSelectionKind::Face;                          // [-] - Kind of the most recent pick (active operand)
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
    double Volume           = 0.0;                                                           // [len³]  - Enclosed volume
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
[[nodiscard]] DraftingDocument CreateDraftingDocument(const std::string& Title = "Untitled");

// Independent copy (tree, captions, selection; history not copied), regenerated into its own partition.
[[nodiscard]] DraftingDocument CloneDraftingDocument(const DraftingDocument& Source);

// 💡 Commands are atomic: the tree is snapshotted, changed and regenerated; if the step the command added is refused by
//    the modeller (or the command cannot apply), the snapshot is restored and the refusal is returned. Parameter edits
//    that make a *later* step refuse are kept — the tree marks the step and keeps the previous body (plan § 3).
[[nodiscard]] DraftingCommandOutcome ConstructDraftingPrimitive(DraftingDocument& Document, const DraftingConstructRequest& Request);
[[nodiscard]] DraftingCommandOutcome BlendDraftingSelection(DraftingDocument& Document, Modeller::BlendProfile Shape, double Size);
[[nodiscard]] DraftingCommandOutcome ChangeDraftingSelection(DraftingDocument& Document, Modeller::FaceOperationKind Operation, double Amount);
[[nodiscard]] DraftingCommandOutcome AssignDraftingParameter(DraftingDocument& Document, const std::string& Name, double Value);
[[nodiscard]] DraftingCommandOutcome DeleteDraftingStep(DraftingDocument& Document, const std::string& Label);
[[nodiscard]] DraftingCommandOutcome ReplaceDraftingTree(DraftingDocument& Document, Modeller::FeatureTree Tree,
                                                         std::map<std::string, std::string> Captions, const std::string& Label);

// Undo / redo restore the tree snapshot and regenerate; false when the stack is empty.
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

} // namespace Frontier::DraftingWorkspace
