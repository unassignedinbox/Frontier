#pragma once

// RollingBallBlend — one generic constant-radius fillet / constant-distance chamfer for any pair of
//    parametric faces. It replaces per-surface-pair recipes with four generic numeric solves:
//
//      1. Station solve (4×4 Newton)      — ball centre on both offset surfaces inside one section plane.
//      2. Curve × surface solve (3×3)     — where a strip ends on an unblended end face.
//      3. Surface × surface solve (3×3)   — mitre between two strips meeting at a sharp corner.
//      4. Three-face ball solve (6×6)     — vertex ball when three equal fillets meet (sphere corner).
//
//    The strip surface is an exact rational quadratic (fillet) or ruled (chamfer) section at every station,
//    interpolated along the spine. Faces are rebuilt by imprint-and-replace: each blended edge is replaced by
//    its two rails, neighbouring edges are trimmed, end connectors are inserted, and the body is re-assembled
//    through BrepBody::AddEdge (which merges shared edges). The result is accepted only if Validate() reports a
//    closed, manifold, oriented solid. Everything outside the supported vertex events refuses with a specific
//    diagnostic so the caller can fall back to BlendSolver.
//
//    Supported vertex events: closed rims (seam vertices), one blended edge ending on a face, two tangent
//    blended edges (continuation), two non-tangent blended edges with an unblended third edge (mitre), three
//    blended edges (sphere corner for equal fillets, triple mitre otherwise).

#include "TopologySpecification.h"
#include <string>
#include <vector>

namespace Frontier
{

enum class BlendProfile : uint8_t
{
    Fillet,                                                                             // circular section, radius = Size
    Chamfer,                                                                            // straight section, distance = Size from the edge
};

struct RollingBlendReport
{
    std::string Diagnostic;                                                             // refusal reason or "ok"
    int    Strips          = 0;                                                         // [-] blend faces created
    int    Corners         = 0;                                                         // [-] sphere corner patches
    int    Mitres          = 0;                                                         // [-] strip-strip intersection curves
    int    EndCurves       = 0;                                                         // [-] strip-face end curves
    int    Stations        = 0;                                                         // [-] stations solved
    double MaxResidual     = 0.0;                                                       // [m] worst station residual
    double MaxDeviation    = 0.0;                                                       // [m] worst mid-station surface deviation
    double VolumeBefore    = 0.0;                                                       // [m³]
    double VolumeAfter     = 0.0;                                                       // [m³]
};

class RollingBallBlend
{
public:
    // Blends every listed edge of Body. Returns a validated solid or refuses atomically.
    [[nodiscard]] static Deliver<BrepBody> Construct(const BrepBody& Body, const std::vector<int>& Edges, double Size,
                                                     BlendProfile Profile, RollingBlendReport* Report = nullptr) noexcept;
};

}
