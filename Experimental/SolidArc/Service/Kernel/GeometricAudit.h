// 📦 Experimental/SolidArc/Service/Kernel/GeometricAudit.h — geometric consistency of a B-rep body
//
// BrepBody::Validate() answers combinatorial questions only (open / non-manifold / misoriented edges, Euler, genus). A body
//    can pass it and still be wrong: an edge that does not lie on its faces, a (u,v) trace that wanders off its edge (a
//    tessellation crack or flap), an inside-out loop, a face that passes through another face (a folded blend), or a
//    carrier far larger than its trimmed face leaking into Bounds(). GeometricAudit measures each of those invariants and
//    reports the worst value with the first offender, so a modelling operation can be gated on geometry, not only topology.
//
//      A1 VertexOnEdge      |edge end − vertex|                         ≤ MergeTolerance
//      A2 LoopContinuity    head of coedge i = tail of coedge i+1        (vertex identity)
//      A3 EdgeOnSurface     edge samples → each adjacent surface         ≤ EdgeTolerance
//      A4 TraceOnEdge       S(trace (u,v)) → edge curve                  ≤ EdgeTolerance
//      A5 Orientation       signed volume > 0 (closed), loop winding in (u,v) matches face sense (non-periodic faces)
//      A6 FaceCrossing      triangles of faces sharing no vertex do not intersect
//      A7 BoundsExcess      BrepBody::Bounds() vs tessellated extent     ≤ 2e-2 · body diagonal (legal oversized carriers must not leak)
#pragma once

#include "Kernel/TopologySpecification.h"

#include <string>
#include <vector>

namespace Frontier {

enum class AuditInvariant : uint8_t { VertexOnEdge, LoopContinuity, EdgeOnSurface, TraceOnEdge, Orientation, FaceCrossing, BoundsExcess, Count };
[[nodiscard]] const char* Describe(AuditInvariant Invariant) noexcept;

struct AuditMeasure
{
    double      Worst = 0.0;                                                            // [m] or [-]: largest violation measured
    int         Violations = 0;                                                         // [-] entities beyond tolerance
    std::string FirstOffender;                                                          // [-] e.g. "edge 7 on face 3"
};

struct AuditReport
{
    AuditMeasure Measures[static_cast<int>(AuditInvariant::Count)];
    [[nodiscard]] const AuditMeasure& Of(AuditInvariant I) const noexcept { return Measures[static_cast<int>(I)]; }
    [[nodiscard]] bool Sound() const noexcept;                                          // no violations at all
    [[nodiscard]] std::string Summary() const;                                          // one line, violations only
};

class GeometricAudit
{
public:
    static constexpr double EdgeTolerance   = 1e-5;                                     // [m] fitted blends carry ≤ 1e-6
    static constexpr double BoundsFraction  = 2e-2;                                     // [-] of the body diagonal (rational arc hulls overshoot ≤ 0.41 r)

    [[nodiscard]] static AuditReport Inspect(const BrepBody& Body, bool FaceCrossings = true) noexcept;
};

} // namespace Frontier
