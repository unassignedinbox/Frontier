/*====================================================================================================================================
                                                       DRAFTINGSNAPPING.H
====================================================================================================================================*/
// 🧩 Object snapping (Fusion / Plasticity point picker): endpoints, midpoints, centres, intersections, tangent and
//    perpendicular points, nearest points on curves / edges / faces — exact on the kernel carriers — and the direction
//    locks of Plasticity's point picker (x / y / z, tangent, normal, binormal)

#pragma once

//------------------------------------------------------------------------------------------------------------------------
//                                                    INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Viewport/DraftingCamera.h"

#include "Geometry/CurveGeometry.h"
#include "Geometry/SurfaceGeometry.h"
#include "Sketch/SketchProfile.h"

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace Frontier::DraftingWorkspace
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    TYPES
//------------------------------------------------------------------------------------------------------------------------

// 📝 What the pointer snapped to. Point snaps (Endpoint … Perpendicular) win over Curve, Curve over Face; Grid and Axis
//    are set by the callers (construction grid, direction lock).
enum class DraftingSnapKind : uint8_t
{
    None          = 0,                                                                       // Nothing in reach
    Grid          = 1,                                                                       // Construction grid step
    Endpoint      = 2,                                                                       // Vertex / curve end / placed point
    Midpoint      = 3,                                                                       // Half the arc length
    Centre        = 4,                                                                       // Circle / arc / ellipse / planar face centre
    Intersection  = 5,                                                                       // Two curves meet
    Tangent       = 6,                                                                       // Line from the last point touches the curve
    Perpendicular = 7,                                                                       // Line from the last point meets the curve square
    Curve         = 8,                                                                       // Nearest point on a curve / edge
    Face          = 9,                                                                       // Exact ray hit on a face
    Axis          = 10,                                                                      // On a direction lock line
};

// 📝 Plasticity point-picker choices (body[gizmo=point-picker]: x y z n b t) — the next point runs along that line
//    through the last placed point; Square is the rectangle's s (snaps:set-square).
enum class DraftingSnapLockKind : uint8_t
{
    None     = 0,
    X        = 1,                                                                            // World x̂
    Y        = 2,                                                                            // World ŷ
    Z        = 3,                                                                            // World ẑ
    Tangent  = 4,                                                                            // Tangent of the snapped curve (t)
    Normal   = 5,                                                                            // Curve normal / face normal (n)
    Binormal = 6,                                                                            // Tangent × normal (b)
};

struct DraftingSnapFilter
{
    bool EndpointEnabled      = true;                                                        // [-] - Vertices, curve ends
    bool MidpointEnabled      = true;                                                        // [-] - Curve midpoints
    bool CentreEnabled        = true;                                                        // [-] - Circle / arc / face centres
    bool IntersectionEnabled  = true;                                                        // [-] - Curve crossings
    bool TangentEnabled       = true;                                                        // [-] - Tangent from the last point
    bool PerpendicularEnabled = true;                                                        // [-] - Perpendicular from the last point
    bool CurveEnabled         = true;                                                        // [-] - On curve / edge
    bool FaceEnabled          = true;                                                        // [-] - On face
};

// 📝 One snap result. Tangent / Normal describe the snapped object at Position (zero when it has none): a curve's unit
//    tangent and its normal (in-plane for planar curves, principal otherwise), a face's unit normal, a circle's axis.
struct DraftingSnap
{
    DraftingSnapKind  Kind     = DraftingSnapKind::None;                                     // [-]   - What it is
    Modeller::Vector3 Position = {};                                                         // [len] - Snapped world point (exact)
    Modeller::Vector3 Tangent  = {};                                                         // [-]   - Unit tangent (curves)
    Modeller::Vector3 Normal   = {};                                                         // [-]   - Unit normal (curves, faces, circle axis)
    std::string       Source   = {};                                                         // [-]   - "Edge E12", "Line3 · Sketch1", "Face F4"
    double            Pixels   = 0.0;                                                        // [px]  - Pointer → snapped point on screen
};

// 📝 One snappable curve with its exact carrier: a kernel curve over [Low, High] (edges, SpaceCurves) or a sketch profile
//    segment over the fraction [0, 1] placed in Plane. Samples hold parameters and their world points (screen search).
struct DraftingSnapCurve
{
    std::string                               Source         = {};                           // [-]   - Shown name
    std::string                               Body           = {};                           // [-]   - Owning body key (edges; empty otherwise)
    std::optional<Modeller::CurveGeometry>    Carrier        = {};                           // [-]   - Kernel carrier (edges, SpaceCurves)
    double                                    Low            = 0.0;                          // [-]   - Parameter range
    double                                    High           = 1.0;                          // [-]   - Parameter range
    std::optional<Modeller::ProfileSegment>   Segment        = {};                           // [-]   - Sketch segment (fraction 0 … 1)
    Modeller::OrthonormalFrame                Plane          = {};                           // [-]   - Sketch plane (Segment)
    bool                                      PlanarCondition = false;                       // [-]   - Lies in the plane ⊥ PlaneNormal
    Modeller::Vector3                         PlaneNormal    = {};                           // [-]   - Unit (planar curves)
    std::optional<Modeller::Vector3>          Centre         = {};                           // [len] - Circle / arc centre
    double                                    Radius         = 0.0;                          // [len] - Circle / arc radius (0: not circular)
    std::vector<double>                       Parameters     = {};                           // [-]   - Sample parameters (ascending)
    std::vector<Modeller::Vector3>            Points         = {};                           // [len] - Sample points
};

// 📝 A fixed snap point (endpoint, midpoint, centre, coplanar sketch crossing), found once per document revision.
struct DraftingSnapPoint
{
    DraftingSnapKind  Kind     = DraftingSnapKind::Endpoint;                                 // [-]   - Endpoint, Midpoint, Centre, Intersection
    Modeller::Vector3 Position = {};                                                         // [len] - Exact point
    Modeller::Vector3 Tangent  = {};                                                         // [-]   - Curve tangent there (zero: none)
    Modeller::Vector3 Normal   = {};                                                         // [-]   - Curve normal / circle axis (zero: none)
    std::string       Source   = {};                                                         // [-]   - Shown name
    std::string       Body     = {};                                                         // [-]   - Owning body key (empty: sketch / curve)
};

// 📝 Everything snappable in one document revision. Excluded names bodies (Move snaps onto other bodies) and sketch or
//    SpaceCurve step labels (a dragged sketch point or pole never snaps onto its own geometry).
//    Surfaces / FaceTokens follow the scene face table; face centres are measured lazily (planar faces only).
struct DraftingSnapCatalogue
{
    uint64_t                                          Revision     = 0;                      // [-]   - Document revision
    std::string                                       ExclusionKey = {};                     // [-]   - Excluded body keys / step labels joined
    std::vector<DraftingSnapCurve>                    Curves       = {};                     // [-]   - Edges, sketch segments, SpaceCurves
    std::vector<DraftingSnapPoint>                    Points       = {};                     // [-]   - Fixed snap points
    std::vector<std::optional<Modeller::SurfaceGeometry>> Surfaces = {};                     // [-]   - Per scene face (empty: excluded / none)
    std::vector<Modeller::FaceToken>                  FaceTokens   = {};                     // [-]   - Per scene face
    std::vector<std::string>                          FaceNames    = {};                     // [-]   - Per scene face
    std::map<uint32_t, std::optional<Modeller::Vector3>> FaceCentres = {};                   // [len] - Planar face centroids (lazy)
    double                                            Scale        = 1.0;                    // [len] - Scene diagonal
};

struct DraftingSnapQuery
{
    DraftingCameraFrame              Frame       = {};                                       // [-]   - Camera of the viewport
    double                           PixelX      = 0.0;                                      // [px]  - Pointer, viewport-local
    double                           PixelY      = 0.0;                                      // [px]  - Pointer, viewport-local
    double                           PointReach  = 12.0;                                     // [px]  - Point snaps
    double                           CurveReach  = 8.0;                                      // [px]  - Curve snaps
    std::optional<Modeller::Vector3> Reference   = {};                                       // [len] - Last placed point (tangent, perpendicular)
    std::vector<DraftingSnapPoint>   Transient   = {};                                       // [-]   - Extra endpoints (drawing chain)
    bool                             XRayEnabled = false;                                    // [-]   - Hidden geometry snaps too
    const DraftingSectionClip*       Section     = nullptr;                                  // [-]   - Removed side does not snap
    DraftingSnapFilter               Filter      = {};                                       // [-]   - Enabled kinds
};

//------------------------------------------------------------------------------------------------------------------------
//                                                    PUBLIC FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

// 💡 Collects the snappable geometry of the shown bodies (exact edge carriers and vertices, arc-length midpoints, conic
//    centres, face carriers), every sketch (solved, on its elevated plane: segment ends, midpoints, arc / circle
//    centres, crossings of segments sharing one plane frame) and every fitted SpaceCurve.
[[nodiscard]] DraftingSnapCatalogue CatalogueDraftingSnaps(const DraftingDocument& Document, const DraftingScene& Scene, const std::vector<std::string>& Excluded = {});

// 💡 The snap under the pointer. Point candidates within PointReach (fixed points, transient points, crossings of the
//    curves near the pointer by Gauss–Newton on both carriers, tangent / perpendicular points from Reference by
//    bracketed roots of (C − P) × C′ · n and (C − P) · C′) compete on pixel distance plus a small kind rank; else the
//    nearest point of a curve within CurveReach (golden section on the ray distance); else the exact ray hit on the face
//    under the pointer (Newton on the signed distance). Hidden candidates (behind a face, or on the removed side of the
//    section) are skipped unless x-ray. Every returned Position lies on its exact carrier.
[[nodiscard]] DraftingSnap FindDraftingSnap(DraftingSnapCatalogue& Catalogue, const DraftingDocument& Document, const DraftingScene& Scene, const DraftingSnapQuery& Query);

// Line parameter of the point of Origin + t·Direction nearest to the ray (nothing when they run parallel).
[[nodiscard]] std::optional<double> ApproachDraftingLine(const DraftingRay& Ray, Modeller::Vector3 Origin, Modeller::Vector3 Direction) noexcept;

// World point → viewport-local pixel (false behind the eye).
[[nodiscard]] bool ProjectDraftingPoint(const DraftingCameraFrame& Frame, Modeller::Vector3 Point, double& PixelX, double& PixelY) noexcept;

[[nodiscard]] const char* DraftingSnapLabel(DraftingSnapKind Kind) noexcept;
[[nodiscard]] const char* DraftingSnapLockLabel(DraftingSnapLockKind Kind) noexcept;

} // namespace Frontier::DraftingWorkspace
