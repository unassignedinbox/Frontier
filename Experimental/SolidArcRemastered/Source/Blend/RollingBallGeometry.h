/*====================================================================================================================================
                                                    ROLLINGBALLGEOMETRY.H
====================================================================================================================================*/
// 🧩 Rolling-ball geometry of one edge: spine = offset ∩ offset, contact curves, exact blend carrier, conic sections

#pragma once

//------------------------------------------------------------------------------------------------------------------------
//                                                    INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Geometry/CurveGeometry.h"
#include "Geometry/SurfaceGeometry.h"
#include "Topology/ModelPartition.h"

#include <cstdint>
#include <optional>
#include <string>

namespace Frontier::SolidArcRemastered
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    TYPES
//------------------------------------------------------------------------------------------------------------------------

enum class BlendProfile : uint8_t
{
    Fillet  = 0,                                                                             // Circular cross-section (rolling ball)
    Chamfer = 1,                                                                             // Chord of the same ball (ruled)
};

// 📝 Size is the ball radius for a fillet and the equal setback along each face for a chamfer.
struct BlendSettings
{
    BlendProfile Profile = BlendProfile::Fillet;                                             // [-]   - Cross-section
    double       Size    = 0.1;                                                              // [len] - Radius / setback
};

// 💡 Everything about one blended edge that does not depend on its neighbours. Spine and both contact curves share
//    one parameter τ: Contacts[i](τ) is the foot of Spine(τ) on support i, so a cross-section is a single τ value.
struct EdgeBlendSheet
{
    EdgeToken       Edge            = {};                                                    // [-]   - Blended edge
    FinToken        Fins[2]         = {};                                                    // [-]   - [0] = radial entry, [1] = mate
    FaceToken       Faces[2]        = {};                                                    // [-]   - Supports (faces of Fins)
    bool            ConvexCondition = true;                                                  // [-]   - Ball inside the material
    double          Radius          = 0.0;                                                   // [len] - Ball radius
    double          Dihedral        = 0.0;                                                   // [rad] - Material-free wedge angle θ
    SurfaceGeometry Offsets[2]      = {};                                                    // [-]   - Ball-centre loci
    CurveGeometry   Spine           = StraightLine{};                                        // [-]   - Ball-centre path
    CurveGeometry   Contacts[2]     = {};                                                    // [-]   - Feet on the supports
    SurfaceGeometry Surface         = PlaneSurface{};                                        // [-]   - Exact blend carrier
    Orientation     Sense           = Orientation::Forward;                                  // [-]   - Outward vs natural normal
    int             SpineAxis       = 0;                                                     // [-]   - Chart coordinate equal to τ (0 = 𝑢, 1 = 𝑣, −1 none)
    double          CrossCentre     = 0.0;                                                   // [-]   - Other chart coordinate at mid-section
    double          Alignment       = 1.0;                                                   // [-]   - +1 if τ runs along the edge, −1 against
    double          MaxResidual     = 0.0;                                                   // [len] - Worst ball / foot residual sampled
};

struct EdgeBlendSheetOutcome
{
    std::optional<EdgeBlendSheet> Sheet   = {};                                              // [-] - Present on success
    std::string                   Refusal = {};                                              // [-] - Reason otherwise
};

//------------------------------------------------------------------------------------------------------------------------
//                                                    PUBLIC FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

// 💡 Research § I (Choi–Ju): ① convexity and wedge angle θ from the outward normals at mid-edge; ② ball radius (chamfer:
//    r = d·tan(θ/2), exact equal setback where the cross-section of the supports is straight); ③ each support offset by
//    r towards the ball (exact, OffsetSurface — refused past a curvature radius); ④ spine = exact hybrid SSI branch of
//    the two offsets nearest the edge (Line / Circle only — traced spines need procedural carriers, 🚧); ⑤ contact
//    curves from the feet of the spine; ⑥ carrier: line spine → cylinder (fillet) / plane (chamfer); circle spine →
//    torus (fillet) / cone, cylinder or annulus plane (chamfer); ⑦ sampled residual |C(τ) − S(τ)| − r and foot distance.
[[nodiscard]] EdgeBlendSheetOutcome ResolveEdgeBlendSheet(const ModelPartition& Partition, EdgeToken Edge, const BlendSettings& Settings);

// Unwrapped chart reference of a sheet at spine parameter τ (anchor for SP-curve fitting on the blend face).
[[nodiscard]] Vector2 ResolveSheetChart(const EdgeBlendSheet& Sheet, double τ) noexcept;

// Exact section of a cylinder by a plane: a circle when the plane is perpendicular to the axis, else an ellipse whose
// minor semi-axis is the radius. Empty when the plane is parallel to the axis.
[[nodiscard]] std::optional<CurveGeometry> SectionCylinderByPlane(const CylinderSurface& Cylinder, Vector3 PlanePoint, Vector3 PlaneNormal) noexcept;

// Closest point of a surface to a position (exact inversion).
[[nodiscard]] Vector3 ProjectPointOntoSurface(const SurfaceGeometry& Surface, Vector3 Position) noexcept;

[[nodiscard]] const char* BlendProfileLabel(BlendProfile Profile) noexcept;

} // namespace Frontier::SolidArcRemastered
