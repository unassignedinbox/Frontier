// 📦 Engine/Modelling/SolidArc/Kernel/GeometricAudit.cpp — geometric consistency of a B-rep body (see header)
#include "Kernel/GeometricAudit.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <set>

namespace Frontier {

const char* Describe(AuditInvariant Invariant) noexcept
{
    switch (Invariant)
    {
        case AuditInvariant::VertexOnEdge:   return "vertex-on-edge";
        case AuditInvariant::LoopContinuity: return "loop-continuity";
        case AuditInvariant::EdgeOnSurface:  return "edge-on-surface";
        case AuditInvariant::TraceOnEdge:    return "trace-on-edge";
        case AuditInvariant::Orientation:    return "orientation";
        case AuditInvariant::FaceCrossing:   return "face-crossing";
        case AuditInvariant::BoundsExcess:   return "bounds-excess";
        default:                             return "?";
    }
}

bool AuditReport::Sound() const noexcept
{
    for (const AuditMeasure& M : Measures) if (M.Violations) return false;
    return true;
}

std::string AuditReport::Summary() const
{
    std::string Out;
    for (int I = 0; I < static_cast<int>(AuditInvariant::Count); ++I)
    {
        const AuditMeasure& M = Measures[I];
        if (!M.Violations) continue;
        char Text[256];
        std::snprintf(Text, sizeof Text, "%s%s ×%d worst %.2e (%s)", Out.empty() ? "" : " · ", Describe(static_cast<AuditInvariant>(I)), M.Violations, M.Worst, M.FirstOffender.c_str());
        Out += Text;
    }
    return Out.empty() ? "sound" : Out;
}

namespace {

void Record(AuditMeasure& M, double Value, double Tolerance, const std::string& Who)
{
    if (Value > M.Worst) M.Worst = Value;
    if (Value > Tolerance) { if (!M.Violations) M.FirstOffender = Who; ++M.Violations; }
}

int TailOf(const BrepBody& B, int Coedge) { const BrepCoedge& C = B.Coedges[Coedge]; const BrepEdge& E = B.Edges[C.Edge]; return C.Reversed ? E.VertexEnd : E.VertexStart; }
int HeadOf(const BrepBody& B, int Coedge) { const BrepCoedge& C = B.Coedges[Coedge]; const BrepEdge& E = B.Edges[C.Edge]; return C.Reversed ? E.VertexStart : E.VertexEnd; }

// Segment PQ against triangle ABC, interior hits only (touching within Epsilon is not a crossing).
bool SegmentCrossesTriangle(Vec3 P, Vec3 Q, Vec3 A, Vec3 B, Vec3 C, double Epsilon)
{
    const Vec3 D = Q - P, E1 = B - A, E2 = C - A, H = D.Cross(E2);
    const double Det = E1.Dot(H);
    if (std::fabs(Det) < 1e-300) return false;                                          // parallel (coplanar overlap is not reported)
    const double F = 1.0 / Det; const Vec3 S = P - A;
    const double U = F * S.Dot(H); if (U <= Epsilon || U >= 1.0 - Epsilon) return false;
    const Vec3 R = S.Cross(E1);
    const double V = F * D.Dot(R); if (V <= Epsilon || U + V >= 1.0 - Epsilon) return false;
    const double T = F * E2.Dot(R);
    return T > Epsilon && T < 1.0 - Epsilon;
}

bool TrianglesCross(const Vec3* T1, const Vec3* T2, double Epsilon)
{
    for (int K = 0; K < 3; ++K)
    {
        if (SegmentCrossesTriangle(T1[K], T1[(K + 1) % 3], T2[0], T2[1], T2[2], Epsilon)) return true;
        if (SegmentCrossesTriangle(T2[K], T2[(K + 1) % 3], T1[0], T1[1], T1[2], Epsilon)) return true;
    }
    return false;
}

bool Overlap(const Box3& A, const Box3& B, double Margin)
{
    return A.Low.X <= B.High.X + Margin && B.Low.X <= A.High.X + Margin && A.Low.Y <= B.High.Y + Margin && B.Low.Y <= A.High.Y + Margin
        && A.Low.Z <= B.High.Z + Margin && B.Low.Z <= A.High.Z + Margin;
}

} // namespace

AuditReport GeometricAudit::Inspect(const BrepBody& Body, bool FaceCrossings) noexcept
{
    AuditReport R;
    auto& VertexOnEdge = R.Measures[static_cast<int>(AuditInvariant::VertexOnEdge)];
    auto& Continuity   = R.Measures[static_cast<int>(AuditInvariant::LoopContinuity)];
    auto& OnSurface    = R.Measures[static_cast<int>(AuditInvariant::EdgeOnSurface)];
    auto& TraceOnEdge  = R.Measures[static_cast<int>(AuditInvariant::TraceOnEdge)];
    auto& Orientation  = R.Measures[static_cast<int>(AuditInvariant::Orientation)];
    auto& Crossing     = R.Measures[static_cast<int>(AuditInvariant::FaceCrossing)];
    auto& Excess       = R.Measures[static_cast<int>(AuditInvariant::BoundsExcess)];
    const int VertexCount = static_cast<int>(Body.Vertices.size());

    // A1: edge ends sit on their vertices
    for (size_t I = 0; I < Body.Edges.size(); ++I)
    {
        const BrepEdge& E = Body.Edges[I];
        if (E.VertexStart < 0 || E.VertexEnd < 0 || E.VertexStart >= VertexCount || E.VertexEnd >= VertexCount) { Record(VertexOnEdge, 1.0, 0.0, "edge " + std::to_string(I) + " has no vertex"); continue; }
        const double D = std::max(E.Curve.StartPoint().Distance(Body.Vertices[E.VertexStart].Point), E.Curve.EndPoint().Distance(Body.Vertices[E.VertexEnd].Point));
        Record(VertexOnEdge, D, ScalarCriteria::MergeTolerance, "edge " + std::to_string(I));
    }

    // A2: loops are closed chains of coedges, head to tail
    for (size_t L = 0; L < Body.Loops.size(); ++L)
    {
        const auto& Cs = Body.Loops[L].Coedges;
        for (size_t K = 0; K < Cs.size(); ++K)
            if (HeadOf(Body, Cs[K]) != TailOf(Body, Cs[(K + 1) % Cs.size()]))
                Record(Continuity, 1.0, 0.0, "loop " + std::to_string(L) + " of face " + std::to_string(Body.Loops[L].Face) + " after coedge " + std::to_string(Cs[K]));
    }

    // A3 + A4: every coedge's edge lies on its face, and the face's (u,v) trace maps back onto the edge
    for (size_t C = 0; C < Body.Coedges.size(); ++C)
    {
        const BrepCoedge& Ce = Body.Coedges[C];
        const BrepEdge& E = Body.Edges[Ce.Edge];
        const NurbsSurface& S = Body.Faces[Ce.Face].Surface;
        const std::string Who = "edge " + std::to_string(Ce.Edge) + " on face " + std::to_string(Ce.Face);
        double Worst = 0;
        for (int K = 0; K <= 8; ++K)
        {
            const double T = E.Curve.DomainStart() + (E.Curve.DomainEnd() - E.Curve.DomainStart()) * K / 8.0;
            double U = 0, V = 0, D = 0; S.ClosestParameter(E.Curve.Sample(T), U, V, &D);
            Worst = std::max(Worst, D);
        }
        Record(OnSurface, Worst, EdgeTolerance, Who);
        const std::vector<Vec2> Trace = Body.CoedgeTrace(static_cast<int>(C));
        double TraceWorst = 0;
        for (const Vec2& At : Trace) { double D = 0; (void)E.Curve.ClosestParameter(S.Sample(At.X, At.Y), &D); TraceWorst = std::max(TraceWorst, D); }
        if (Trace.size() >= 2)                                                          // the trace also runs tail → head
            TraceWorst = std::max({ TraceWorst, S.Sample(Trace.front().X, Trace.front().Y).Distance(Body.Vertices[TailOf(Body, static_cast<int>(C))].Point),
                                                S.Sample(Trace.back().X, Trace.back().Y).Distance(Body.Vertices[HeadOf(Body, static_cast<int>(C))].Point) });
        Record(TraceOnEdge, Trace.empty() ? 1.0 : TraceWorst, EdgeTolerance, Who);
    }

    // A5: the closed body encloses positive volume; on non-periodic faces the outer loop winds CCW about the face normal
    const BodyReport Combinatorial = Body.Validate();
    if (Combinatorial.Closed && Combinatorial.Volume <= 0.0) Record(Orientation, -Combinatorial.Volume + 1e-300, 0.0, "body is inside out (signed volume " + std::to_string(Combinatorial.Volume) + ")");
    for (size_t F = 0; F < Body.Faces.size(); ++F)
    {
        const BrepFace& Face = Body.Faces[F];
        if (Face.Surface.ClosedU() || Face.Surface.ClosedV()) continue;                 // non-contractible loops have no winding
        for (int L : Face.Loops)
        {
            double Area = 0; std::vector<Vec2> Ring;
            for (int C : Body.Loops[L].Coedges) { auto T = Body.CoedgeTrace(C); Ring.insert(Ring.end(), T.begin(), T.end()); }
            for (size_t K = 0; K < Ring.size(); ++K) { const Vec2 A = Ring[K], B = Ring[(K + 1) % Ring.size()]; Area += A.X * B.Y - B.X * A.Y; }
            const double Expected = (Body.Loops[L].Outer ? 1.0 : -1.0) * (Face.Reversed ? -1.0 : 1.0);
            const double Span = (Face.Surface.DomainEndU() - Face.Surface.DomainStartU()) * (Face.Surface.DomainEndV() - Face.Surface.DomainStartV());
            if (Area * Expected < 0.0) Record(Orientation, std::fabs(0.5 * Area) / std::max(Span, 1e-300), 1e-9, std::string(Body.Loops[L].Outer ? "outer" : "inner") + " loop of face " + std::to_string(F) + " winds the wrong way");
        }
    }

    // A6 + A7: tessellate once; faces sharing no vertex never meet; Bounds() hugs the tessellated body
    Box3 Whole;
    std::vector<BrepBody::FaceTriangles> Meshes(Body.Faces.size());
    std::vector<Box3> FaceBox(Body.Faces.size());
    std::vector<std::set<int>> FaceVertices(Body.Faces.size());
    for (size_t F = 0; F < Body.Faces.size(); ++F)
    {
        Meshes[F] = Body.TessellateFace(static_cast<int>(F), 1e-3);
        for (const Vec3& P : Meshes[F].Positions) { FaceBox[F].Include(P); Whole.Include(P); }
        for (int L : Body.Faces[F].Loops) for (int C : Body.Loops[L].Coedges) { FaceVertices[F].insert(TailOf(Body, C)); FaceVertices[F].insert(HeadOf(Body, C)); }
    }
    const double Diagonal = Whole.Empty() ? 1.0 : (Whole.High - Whole.Low).Length();
    if (!Whole.Empty())
    {
        const Box3 Reported = Body.Bounds();
        const Vec3 Below = Whole.Low - Reported.Low, Above = Reported.High - Whole.High;
        const double Gaps[6] = { Below.X, Below.Y, Below.Z, Above.X, Above.Y, Above.Z };
        const char* Sides[6] = { "low x", "low y", "low z", "high x", "high y", "high z" };
        int Worst = 0; for (int K = 1; K < 6; ++K) if (Gaps[K] > Gaps[Worst]) Worst = K;
        char Who[96]; std::snprintf(Who, sizeof Who, "%s overshoots by %.4g m", Sides[Worst], Gaps[Worst]);
        Record(Excess, std::max(Gaps[Worst], 0.0) / Diagonal, BoundsFraction, Who);
    }
    if (FaceCrossings)
    {
        const double Epsilon = 1e-9;
        for (size_t F = 0; F < Body.Faces.size(); ++F)
            for (size_t G = F + 1; G < Body.Faces.size(); ++G)
            {
                if (!Overlap(FaceBox[F], FaceBox[G], 0.0)) continue;
                bool Shared = false; for (int V : FaceVertices[F]) if (FaceVertices[G].count(V)) { Shared = true; break; }
                if (Shared) continue;
                const auto& A = Meshes[F]; const auto& B = Meshes[G];
                int Hits = 0;
                for (size_t I = 0; I + 2 < A.Triangles.size() && !Hits; I += 3)
                {
                    const Vec3 TA[3] = { A.Positions[A.Triangles[I]], A.Positions[A.Triangles[I + 1]], A.Positions[A.Triangles[I + 2]] };
                    Box3 BA; for (const Vec3& P : TA) BA.Include(P);
                    if (!Overlap(BA, FaceBox[G], 0.0)) continue;
                    for (size_t J = 0; J + 2 < B.Triangles.size(); J += 3)
                    {
                        const Vec3 TB[3] = { B.Positions[B.Triangles[J]], B.Positions[B.Triangles[J + 1]], B.Positions[B.Triangles[J + 2]] };
                        Box3 BB; for (const Vec3& P : TB) BB.Include(P);
                        if (Overlap(BA, BB, 0.0) && TrianglesCross(TA, TB, Epsilon)) { ++Hits; break; }
                    }
                }
                if (Hits) Record(Crossing, 1.0, 0.0, "faces " + std::to_string(F) + " and " + std::to_string(G));
            }
    }
    return R;
}

} // namespace Frontier
