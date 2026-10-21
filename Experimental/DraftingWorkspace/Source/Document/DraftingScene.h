/*====================================================================================================================================
                                                        DRAFTINGSCENE.H
====================================================================================================================================*/
// 🧩 Display arrays of the current body (per-face corners, edge segments, name tables) and the CPU ray pick

#pragma once

//------------------------------------------------------------------------------------------------------------------------
//                                                    INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Document/DraftingDocument.h"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace Frontier::DraftingWorkspace
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    CONSTANTS
//------------------------------------------------------------------------------------------------------------------------

// 📝 Identity target encoding (R32_UINT): 0 = background, face f → f + 1, edge e → IdentityEdgeBit | (e + 1).
constexpr uint32_t IdentityEdgeBit = 0x80000000u;

//------------------------------------------------------------------------------------------------------------------------
//                                                    STRUCTS
//------------------------------------------------------------------------------------------------------------------------

// 📝 GPU layouts: 32-byte corners and segments (std430-friendly, see Shaders/DraftingProjection.slang).
struct DraftingSceneCorner
{
    float    Position[3] = {};                                                               // [len] - World point
    float    Normal[3]   = {};                                                               // [-]   - Outward unit normal (exact surface)
    uint32_t Face        = 0;                                                                // [idx] - Scene face index
    uint32_t Padding     = 0;                                                                // [-]   - 32-byte stride
};

struct DraftingSceneSegment
{
    float    Start[3] = {};                                                                  // [len] - World point
    float    End[3]   = {};                                                                  // [len] - World point
    uint32_t Edge     = 0;                                                                   // [idx] - Scene edge index
    uint32_t Padding  = 0;                                                                   // [-]   - 32-byte stride
};

struct DraftingSceneFace
{
    std::string Name        = {};                                                            // [-]    - Persistent name
    std::string Surface     = {};                                                            // [-]    - Carrier category ("Plane", "Cylinder")
    uint32_t    Slot        = 0;                                                             // [idx]  - Partition face slot
    uint32_t    FirstCorner = 0;                                                             // [idx]  - Span start in Corners
    uint32_t    CornerCount = 0;                                                             // [-]    - Span length (3 per triangle)
    uint32_t    Body        = 0;                                                             // [idx]  - Scene body index
    double      Area        = 0.0;                                                           // [len²] - Facet area
};

struct DraftingSceneEdge
{
    std::string Name         = {};                                                           // [-]   - Persistent name
    std::string Curve        = {};                                                           // [-]   - Carrier category ("Line", "Circle")
    uint32_t    Slot         = 0;                                                            // [idx] - Partition edge slot
    uint32_t    FirstSegment = 0;                                                            // [idx] - Span start in Segments
    uint32_t    SegmentCount = 0;                                                            // [-]   - Span length
    uint32_t    Body         = 0;                                                            // [idx] - Scene body index
    double      Length       = 0.0;                                                          // [len] - Polyline length
};

// 📝 One body's spans in the scene tables (faces and edges are grouped by body, in creation order).
struct DraftingSceneBody
{
    std::string Key       = {};                                                              // [-]   - Creating step label ("Extrude#2")
    std::string Caption   = {};                                                              // [-]   - Shown name ("Block1")
    uint32_t    FirstFace = 0;                                                               // [idx] - Span start in Faces
    uint32_t    FaceCount = 0;                                                               // [-]   - Span length
    uint32_t    FirstEdge = 0;                                                               // [idx] - Span start in Edges
    uint32_t    EdgeCount = 0;                                                               // [-]   - Span length
};

struct DraftingScene
{
    std::vector<DraftingSceneCorner>  Corners     = {};                                      // [-]   - 3 per triangle, grouped by face
    std::vector<DraftingSceneSegment> Segments    = {};                                      // [-]   - Edge polylines as segments
    std::vector<DraftingSceneFace>    Faces       = {};                                      // [-]   - Scene face table
    std::vector<DraftingSceneEdge>    Edges       = {};                                      // [-]   - Scene edge table
    std::vector<DraftingSceneBody>    Bodies      = {};                                      // [-]   - Scene body table
    std::array<double, 3>             BoundsLow   = { -1.0, -1.0, -1.0 };                    // [len] - Box corner
    std::array<double, 3>             BoundsHigh  = { 1.0, 1.0, 1.0 };                       // [len] - Box corner
    uint64_t                          Revision    = 0;                                       // [-]   - Document revision it was built from
    uint64_t                          Generation  = 0;                                       // [-]   - Distinct per build (density changes re-upload)
    double                            Sag         = 0.0;                                     // [len] - Tessellation ε
    double                            Milliseconds = 0.0;                                    // [ms]  - Tessellation + packing
    std::string                       Refusal     = {};                                      // [-]   - Tessellation refusal (empty: fine)
};

// 📝 Per-face / per-edge state words for the viewport programs (bits: 1 selected, 2 active, 4 hovered).
struct DraftingSceneStates
{
    std::vector<uint32_t> Faces = {};                                                        // [-] - One word per scene face
    std::vector<uint32_t> Edges = {};                                                        // [-] - One word per scene edge
};

struct DraftingRay
{
    std::array<double, 3> Origin    = {};                                                    // [len] - Start point
    std::array<double, 3> Direction = { 0.0, 0.0, -1.0 };                                    // [-]   - Unit direction
};

struct DraftingPick
{
    uint32_t Identity = 0;                                                                   // [-]   - Identity code (see IdentityEdgeBit)
    double   Distance = 0.0;                                                                 // [len] - Along the ray
};

//------------------------------------------------------------------------------------------------------------------------
//                                                    PUBLIC FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

// Tessellates the document's body (SagRatio of the body diagonal) and packs the display arrays; empty without a body.
// FineCondition (zebra on): sides no longer than FineEdgeRatio of the diagonal, so interpolated normals follow the surface.
[[nodiscard]] DraftingScene BuildDraftingScene(const DraftingDocument& Document, double SagRatio = 1.5e-3, bool FineCondition = false);

constexpr double FineEdgeRatio = 0.02;                                                       // [-] - Zebra tessellation: longest side / diagonal

// 📝 Vehicle plan V8: one imported limit point and its foot on the nearest surface panel.
struct DraftingDeviationNeedle
{
    Modeller::Vector3 Point    = {};                                                         // [len] - Imported point (SpaceCurve step)
    Modeller::Vector3 Foot     = {};                                                         // [len] - Closest point on the panels
    double            Distance = 0.0;                                                        // [len] - |Point − Foot|
};

struct DraftingDeviationMap
{
    uint64_t                             Revision = 0;                                       // [-]   - Document revision measured
    std::vector<DraftingDeviationNeedle> Needles  = {};                                      // [-]   - One per imported point
    double                               Worst    = 0.0;                                     // [len] - Largest distance
    double                               Mean     = 0.0;                                     // [len] - Mean distance
};

// 💡 Distance from every SpaceCurve point (lofted or not — check loops count) to the nearest surface panel: best of a
//    9 × 9 parameter grid per panel, then Newton projection (ProjectOntoNurbsSurface, clamped to the panel). Empty
//    without panels.
[[nodiscard]] DraftingDeviationMap MeasureDraftingDeviation(const DraftingDocument& Document);

// 💡 Zebra phase (mirrors DraftingSurface.slang): the view direction reflected in the unit normal, its angle to the
//    camera up axis × Stripes / π — stripe k covers phase [k, k + ½). A function of the normal alone (view fixed), so
//    a G0 joint breaks the stripes, G1 kinks them and G2 keeps them smooth.
[[nodiscard]] double DraftingZebraPhase(Modeller::Vector3 Normal, Modeller::Vector3 Forward, Modeller::Vector3 Up, double Stripes) noexcept;

// 💡 CPU pick: nearest triangle hit (Möller–Trumbore); an edge wins over the face behind it when the ray passes within
//    EdgeReach of the segment and no closer than the face hit minus EdgeReach (so silhouette and crease edges are
//    pickable but hidden edges are not). Returns nothing on a miss.
[[nodiscard]] std::optional<DraftingPick> PickDraftingScene(const DraftingScene& Scene, const DraftingRay& Ray, double EdgeReach);

// Selection names → state words; the last name of the most recently picked kind is the active operand.
[[nodiscard]] DraftingSceneStates ComposeDraftingStates(const DraftingScene& Scene, const DraftingSelection& Selection, uint32_t HoverIdentity);
// 📝 Matcap layer of every scene face (its own assignment, else the body matcap; unknown names → 0 = Studio), already
//    shifted into bits 8 … 15 so the caller ORs it into the face state words.
[[nodiscard]] std::vector<uint32_t> ComposeDraftingMatcapLayers(const DraftingScene& Scene, const DraftingAppearance& Appearance,
                                                                const DraftingMatcapLibrary& Library);
// Face assignments whose persistent name the current scene no longer has (kept in the document, reported in the Inspector).
[[nodiscard]] size_t CountDanglingMatcaps(const DraftingScene& Scene, const DraftingAppearance& Appearance);

// 📝 Closest approach of the ray to one scene edge: ray parameter and separation at the nearest of its segments.
struct DraftingEdgeApproach
{
    double Reach      = 0.0;                                                                 // [len] - Along the ray
    double Separation = 1.0e300;                                                             // [len] - Ray ↔ edge distance
};
[[nodiscard]] DraftingEdgeApproach ApproachDraftingEdge(const DraftingScene& Scene, const DraftingRay& Ray, uint32_t EdgeIndex);

// Identity → persistent name ("" for the background or an out-of-range code).
[[nodiscard]] std::string NameDraftingIdentity(const DraftingScene& Scene, uint32_t Identity);
[[nodiscard]] DraftingSelectionKind DraftingIdentityKind(uint32_t Identity) noexcept;

} // namespace Frontier::DraftingWorkspace
