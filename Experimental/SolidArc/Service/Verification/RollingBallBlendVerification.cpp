// RollingBallBlendVerification — specimen sweep for Kernel/RollingBallBlend.
//    Each specimen builds a body, blends a set of edges with the generic construction, and checks:
//      * the result is a closed, manifold, oriented solid with unchanged genus,
//      * the volume change has the sign implied by convexity and, where a closed form exists, its value,
//      * the legacy BlendSolver outcome on the same input is reported alongside for comparison.
//    Exit code 0 only if every specimen expected to pass does.

#include "Verification/QuadratureVolume.h"
#include "Kernel/RollingBallBlend.h"
#include "Kernel/BlendSolver.h"
#include "Kernel/IntersectionSolver.h"

#include <cmath>
#include <cstdio>
#include <functional>
#include <string>
#include <vector>

using namespace Frontier;

namespace
{

struct Specimen
{
    std::string Name;
    std::function<BrepBody()> Build;
    std::function<std::vector<int>(const BrepBody&)> Pick;
    double Size;
    BlendProfile Profile;
    double ExpectedDelta;                                                               // [m³] closed-form volume change, NaN when unknown
    bool   ExpectRefusal = false;                                                       // [-] the only correct answer is a refusal
};

std::vector<int> EdgesWhere(const BrepBody& B, const std::function<bool(Vec3, Vec3)>& Keep)
{
    std::vector<int> Out;
    for (size_t E = 0; E < B.Edges.size(); ++E)
        if (Keep(B.Edges[E].Curve.StartPoint(), B.Edges[E].Curve.EndPoint())) Out.push_back(static_cast<int>(E));
    return Out;
}

std::vector<int> ClosedEdgesAt(const BrepBody& B, double Z)
{
    std::vector<int> Out;
    for (size_t E = 0; E < B.Edges.size(); ++E)
        if (B.Edges[E].Closed() && std::fabs(B.Edges[E].Curve.StartPoint().Z - Z) < 1e-6) Out.push_back(static_cast<int>(E));
    return Out;
}

constexpr double Pi = 3.14159265358979323846;

const double Unknown = std::nan("");

// Removed volume of a convex fillet of radius r along a straight 90° edge of length L: (1 − π/4) r² L.
double StraightFilletLoss(double R, double L) { return (1.0 - Pi / 4.0) * R * R * L; }

}

int main()
{
    const double R = 0.3;
    std::vector<Specimen> Specimens = {
        { "box one edge fillet", [] { return BrepBody::Box({ 0, 0, 0 }, { 4, 3, 2 }).Payload; },
          [](const BrepBody& B) { return EdgesWhere(B, [](Vec3 A, Vec3 C) { return A.Y < 1e-9 && C.Y < 1e-9 && A.Z > 1.99 && C.Z > 1.99; }); }, R, BlendProfile::Fillet, -StraightFilletLoss(R, 4) },
        { "box one edge chamfer", [] { return BrepBody::Box({ 0, 0, 0 }, { 4, 3, 2 }).Payload; },
          [](const BrepBody& B) { return EdgesWhere(B, [](Vec3 A, Vec3 C) { return A.Y < 1e-9 && C.Y < 1e-9 && A.Z > 1.99 && C.Z > 1.99; }); }, R, BlendProfile::Chamfer, -0.5 * R * R * 4 },
        { "box top four edges fillet (mitres)", [] { return BrepBody::Box({ 0, 0, 0 }, { 4, 3, 2 }).Payload; },
          [](const BrepBody& B) { return EdgesWhere(B, [](Vec3 A, Vec3 C) { return A.Z > 1.99 && C.Z > 1.99; }); }, R, BlendProfile::Fillet,
          // four prisms along the top perimeter (14 m) minus four pairwise overlaps ∫(r − √(r² − t²))² dt = r³(5/3 − π/2)
          -((1.0 - Pi / 4.0) * R * R * 14.0 - 4.0 * R * R * R * (5.0 / 3.0 - Pi / 2.0)) },
        { "box top four edges chamfer (mitres)", [] { return BrepBody::Box({ 0, 0, 0 }, { 4, 3, 2 }).Payload; },
          [](const BrepBody& B) { return EdgesWhere(B, [](Vec3 A, Vec3 C) { return A.Z > 1.99 && C.Z > 1.99; }); }, R, BlendProfile::Chamfer,
          -(0.5 * R * R * 14.0 - 4.0 * R * R * R / 3.0) },
        { "box three edges at a corner fillet (sphere corner)", [] { return BrepBody::Box({ 0, 0, 0 }, { 4, 3, 2 }).Payload; },
          [](const BrepBody& B) { return EdgesWhere(B, [](Vec3 A, Vec3 C) { Vec3 V{ 4, 3, 2 }; return A.Coincident(V, 1e-9) || C.Coincident(V, 1e-9); }); }, R, BlendProfile::Fillet,
          // three strips from the corner section (r from the vertex) to the far faces, plus the corner cube minus the ball octant
          -((1.0 - Pi / 4.0) * R * R * ((4 - R) + (3 - R) + (2 - R)) + R * R * R * (1.0 - Pi / 6.0)) },
        { "box all twelve edges fillet", [] { return BrepBody::Box({ 0, 0, 0 }, { 4, 3, 2 }).Payload; },
          [](const BrepBody& B) { std::vector<int> All; for (size_t E = 0; E < B.Edges.size(); ++E) All.push_back(static_cast<int>(E)); return All; }, R, BlendProfile::Fillet,
          // rounded box: V = abc − (4 − π) r² (a+b+c − 6r)... closed form for all-edges fillet with sphere corners
          -((4.0 - Pi) * R * R * ((4 - 2 * R) + (3 - 2 * R) + (2 - 2 * R)) + (8.0 - 4.0 / 3.0 * Pi) * R * R * R) },
        { "box all twelve edges chamfer (triple mitres)", [] { return BrepBody::Box({ 0, 0, 0 }, { 4, 3, 2 }).Payload; },
          [](const BrepBody& B) { std::vector<int> All; for (size_t E = 0; E < B.Edges.size(); ++E) All.push_back(static_cast<int>(E)); return All; }, R, BlendProfile::Chamfer,
          // twelve prisms (36 m of edge), minus 24 pairwise overlaps d³/3, plus 8 triple overlaps d³/4
          -(0.5 * R * R * 36.0 - 8.0 * R * R * R + 8.0 * R * R * R / 4.0) },
        { "cylinder top rim fillet", [] { return BrepBody::Cylinder({ 0, 0, 0 }, { 0, 0, 1 }, 1.0, 2.0).Payload; },
          [](const BrepBody& B) { return ClosedEdgesAt(B, 2.0); }, R, BlendProfile::Fillet,
          // Pappus: section area (1 − π/4) r², centroid k·r inside the rim, k = (10 − 3π)/(12 − 3π)
          -(2 * Pi * (1.0 - R * (10.0 - 3.0 * Pi) / (12.0 - 3.0 * Pi)) * (1.0 - Pi / 4.0) * R * R) },
        { "cylinder both rims chamfer", [] { return BrepBody::Cylinder({ 0, 0, 0 }, { 0, 0, 1 }, 1.0, 2.0).Payload; },
          [](const BrepBody& B) { auto A = ClosedEdgesAt(B, 2.0), C = ClosedEdgesAt(B, 0.0); A.insert(A.end(), C.begin(), C.end()); return A; }, R, BlendProfile::Chamfer,
          -2.0 * (2 * Pi * (1.0 - R / 3.0) * 0.5 * R * R) },
        { "boss root fillet (concave rim)", [] { auto A = BrepBody::Box({ 0, 0, 0 }, { 4, 3, 2 }).Payload; return IntersectionSolver::Combine(A, BrepBody::Cylinder({ 2, 1.5, 1 }, { 0, 0, 1 }, 0.6, 2).Payload, BodyOperation::Union).Payload; },
          [](const BrepBody& B) { return ClosedEdgesAt(B, 2.0); }, 0.2, BlendProfile::Fillet,
          // concave: adds (1 − π/4) r² with centroid k·r outside the boss radius
          (2 * Pi * (0.6 + 0.2 * (10.0 - 3.0 * Pi) / (12.0 - 3.0 * Pi)) * (1.0 - Pi / 4.0) * 0.04) },
        { "boss top rim fillet", [] { auto A = BrepBody::Box({ 0, 0, 0 }, { 4, 3, 2 }).Payload; return IntersectionSolver::Combine(A, BrepBody::Cylinder({ 2, 1.5, 1 }, { 0, 0, 1 }, 0.6, 2).Payload, BodyOperation::Union).Payload; },
          [](const BrepBody& B) { return ClosedEdgesAt(B, 3.0); }, 0.2, BlendProfile::Fillet,
          -(2 * Pi * (0.6 - 0.2 * (10.0 - 3.0 * Pi) / (12.0 - 3.0 * Pi)) * (1.0 - Pi / 4.0) * 0.04) },
        { "hole rim fillet (convex rim on a hole)", [] { auto A = BrepBody::Box({ 0, 0, 0 }, { 4, 3, 2 }).Payload; return IntersectionSolver::Combine(A, BrepBody::Cylinder({ 2, 1.5, -1 }, { 0, 0, 1 }, 0.6, 4).Payload, BodyOperation::Subtract).Payload; },
          [](const BrepBody& B) { return ClosedEdgesAt(B, 2.0); }, 0.2, BlendProfile::Fillet,
          -(2 * Pi * (0.6 + 0.2 * (10.0 - 3.0 * Pi) / (12.0 - 3.0 * Pi)) * (1.0 - Pi / 4.0) * 0.04) },
        { "cone top rim fillet", [] { return BrepBody::Cone({ 0, 0, 0 }, { 0, 0, 1 }, 1.2, 0.6, 1.5).Payload; },
          [](const BrepBody& B) { return ClosedEdgesAt(B, 1.5); }, 0.15, BlendProfile::Fillet, Unknown },
        // size beyond the rim's radius: the top-face rail would pass the axis and the section sweep would fold through itself
        { "cylinder rim chamfer larger than the radius (must refuse)", [] { return BrepBody::Cylinder({ 0, 0, 0 }, { 0, 0, 1 }, 0.5, 2.0).Payload; },
          [](const BrepBody& B) { return ClosedEdgesAt(B, 2.0); }, 0.6, BlendProfile::Chamfer, Unknown, true },
        { "cylinder rim fillet larger than the radius (must refuse)", [] { return BrepBody::Cylinder({ 0, 0, 0 }, { 0, 0, 1 }, 0.5, 2.0).Payload; },
          [](const BrepBody& B) { return ClosedEdgesAt(B, 2.0); }, 0.6, BlendProfile::Fillet, Unknown, true },
    };

    int Failures = 0;
    std::printf("| Specimen | Route | Faces | Stations | Max residual | Max fit deviation | ΔV measured | ΔV closed form | Legacy BlendSolver |\n");
    std::printf("|---|---|---|---|---|---|---|---|---|\n");
    for (const Specimen& S : Specimens)
    {
        BrepBody Body = S.Build();
        std::vector<int> Edges = S.Pick(Body);
        RollingBlendReport Report;
        Deliver<BrepBody> Result = RollingBallBlend::Construct(Body, Edges, S.Size, S.Profile, &Report);
        Deliver<BrepBody> Legacy = S.Profile == BlendProfile::Fillet ? BlendSolver::FilletEdges(Body, Edges, S.Size) : BlendSolver::ChamferEdges(Body, Edges, S.Size);
        std::string LegacyText = Legacy ? "solid" : std::string("refused: ") + (Legacy.Denial.Detail ? Legacy.Denial.Detail : "?");
        if (LegacyText.size() > 60) LegacyText = LegacyText.substr(0, 57) + "...";
        bool Pass = S.ExpectRefusal ? !Result : static_cast<bool>(Result);
        double Delta = Result ? QuadratureVolume(Result.Payload) - QuadratureVolume(Body) : 0.0;
        std::string Closed = "—";
        if (Result && !S.ExpectRefusal && !std::isnan(S.ExpectedDelta))
        {
            double Error = std::fabs(Delta - S.ExpectedDelta);
            // tessellated volume: allow a small relative tolerance on the change
            bool Close = Error <= 1e-4 * std::fabs(S.ExpectedDelta) + 1e-6;
            char Text[64]; std::snprintf(Text, sizeof Text, "%.6f (%s, err %.1e)", S.ExpectedDelta, Close ? "match" : "MISMATCH", Error);
            Closed = Text;
            Pass = Pass && Close;
        }
        if (!Pass) ++Failures;
        if (S.ExpectRefusal)
            std::printf("| %s | %s | — | — | — | — | — | — | %s |\n", S.Name.c_str(), Result ? "🔴 ACCEPTED (expected a refusal)" : ("✔️ refused as required: " + Report.Diagnostic).c_str(), LegacyText.c_str());
        else if (Result)
            std::printf("| %s | general ✔️ | %zu | %d | %.1e | %.1e | %.6f | %s | %s |\n", S.Name.c_str(), Result.Payload.Faces.size(), Report.Stations, Report.MaxResidual, Report.MaxDeviation, Delta, Closed.c_str(), LegacyText.c_str());
        else
            std::printf("| %s | 🔴 refused: %s | — | — | — | — | — | — | %s |\n", S.Name.c_str(), Report.Diagnostic.c_str(), LegacyText.c_str());
    }
    std::printf("\n%d specimen(s) failed\n", Failures);
    return Failures == 0 ? 0 : 1;
}
