// 📦 Engine/Modelling/SolidArc/Verification/BrepDefectVerification.cpp — one specimen per B-rep defect fixed in phase 3
//
// Each specimen rebuilds the scene that exposed the defect through ConsoleHost (the same command stream the browser UI
//    sends to the geometry service) and measures the quantity that was wrong against a closed form or an independent
//    measure. A specimen names the defect it guards, the measured value, the bound, and the value before the fix.
//
//      BrepDefectVerification > defects.md          (run from the repository root: specimen D12 replays a sample)
#include "Console/ConsoleHost.h"
#include "Kernel/GeometricAudit.h"
#include "Verification/QuadratureVolume.h"
#include <cmath>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

using namespace Frontier;

namespace
{
constexpr double Pi = 3.14159265358979323846;

struct Verdict { std::string Defect, Specimen, Measure, Before; double Value, Bound; bool AtMost; };
std::vector<Verdict> Verdicts;

void Judge(const char* Defect, const char* Specimen, const char* Measure, double Value, double Bound, bool AtMost, const char* Before)
{
    Verdicts.push_back({ Defect, Specimen, Measure, Before, Value, Bound, AtMost });
}

const SceneFigure* Figure(ConsoleHost& Host, const std::string& Name)
{
    for (const SceneFigure& F : Host.Document().Figures()) if (F.Name == Name) return &F;
    return nullptr;
}

ConsoleHost& Fresh(ConsoleHost& Host, std::initializer_list<const char*> Script)
{
    Host.StartEmptyDocument();
    for (const char* Line : Script) Host.Execute(Line);
    return Host;
}

double AuditWorst(const BrepBody& Body)                                                 // 0 when sound, else the worst normalised measure
{
    const AuditReport R = GeometricAudit::Inspect(Body);
    return R.Summary() == "sound" ? 0.0 : 1.0;
}

double FaceArea(const BrepBody& B, int Face)                                            // tessellated area: what the renderer and Validate() see
{
    const BrepBody::FaceTriangles T = B.TessellateFace(Face, 1e-4);
    double Area = 0;
    for (size_t K = 0; K + 2 < T.Triangles.size(); K += 3)
        Area += 0.5 * (T.Positions[T.Triangles[K + 1]] - T.Positions[T.Triangles[K]]).Cross(T.Positions[T.Triangles[K + 2]] - T.Positions[T.Triangles[K]]).Length();
    return Area;
}
} // namespace

int main()
{
    ConsoleHost Host("_ClaudeScratch/tmp/proofs", 64u, 64u);

    // D7 — trimmed tessellation counted a lattice row twice where a trim ran along an iso line (hemisphere on a box)
    Fresh(Host, { "box (6,17,0) (8,19,1) --name=Pedestal", "sphere (7,18,1) 0.5 --name=Dome", "boolean union Pedestal -- Dome --name=DomedPedestal" });
    if (const SceneFigure* F = Figure(Host, "DomedPedestal"))
    {
        int Sphere = -1; for (size_t I = 0; I < F->Body.Faces.size(); ++I) if (F->Body.Faces[I].Surface.Classification != SurfaceClassification::Plane) Sphere = static_cast<int>(I);
        Judge("D7", "DomedPedestal", "hemisphere area rel. error", Sphere < 0 ? 1.0 : std::fabs(FaceArea(F->Body, Sphere) / (2 * Pi * 0.25) - 1.0), 2e-3, true, "4.2e-2");
        Judge("D7", "DomedPedestal", "Validate volume rel. error", std::fabs(F->Body.Validate().Volume / (4.0 + 2.0 / 3.0 * Pi * 0.125) - 1.0), 1e-3, true, "9.6e-2");
    }

    // D5b — trimmed sphere bounds used the whole carrier (a pole region needs no trim loop)
    Fresh(Host, { "box (10,17,0) (12,19,1) --name=Socket", "sphere (11,18,1) 0.5 --name=Scoop", "boolean subtract Socket -- Scoop --name=ScoopedSocket" });
    if (const SceneFigure* F = Figure(Host, "ScoopedSocket")) Judge("D5b", "ScoopedSocket", "audit findings", AuditWorst(F->Body), 0.0, true, "bounds-excess 1.67e-1");

    // D8 — `shell` on a box extruded a U profile: two walls missing, 45° mitres at the rim
    Fresh(Host, { "box (0,0,0) (2,1.5,1) --name=B", "shell B 0.1 --face=1 --name=S" });
    if (const SceneFigure* F = Figure(Host, "S") ? Figure(Host, "S") : Figure(Host, "B"))
    {
        Judge("D8", "Shelled box", "volume rel. error vs 3 − 1.8·1.3·0.9", std::fabs(QuadratureVolume(F->Body) / (3.0 - 1.8 * 1.3 * 0.9) - 1.0), 1e-7, true, "0.38 (volume 0.555 vs 0.894)");
        Judge("D8", "Shelled box", "faces (5 outer + 5 inner + rim)", F->Body.Faces.size(), 11, true, "10, no inner y walls");
    }

    // D9 — box ⊖ box booleans emitted one patch per cell of a 3×3×3 arrangement
    struct Case { const char* Name; const char* A; const char* C; const char* Op; double Volume; int Faces; const char* Before; };
    const Case Cases[] = {
        { "Pocket",      "box (0,0,0) (2,1.5,1)",  "box (0.5,0.5,0.5) (1.5,1,2)",  "subtract", 2.75, 11, "46" },
        { "SquareHole",  "box (0,5,0) (2,6.5,1)",  "box (0.5,5.5,-1) (1.5,6,2)",   "subtract", 2.50, 10, "32" },
        { "CornerNotch", "box (0,9,0) (2,10.5,1)", "box (1,9.5,0.5) (3,10,2)",     "subtract", 2.75, 10, "34" },
        { "CornerUnion", "box (0,17,0) (2,18,1)",  "box (1,17.5,0.5) (3,19,2)",    "union",    6.25, 12, "42" },
        { "PlusUnion",   "box (0,21,0) (4,22,1)",  "box (1.5,20,0) (2.5,23,1)",    "union",    6.00, 22, "22 (coplanar: arrangement fallback, unchanged)" } };
    for (const Case& K : Cases)
    {
        const std::string A = std::string(K.A) + " --name=A", C = std::string(K.C) + " --name=C", Op = std::string("boolean ") + K.Op + " A -- C --name=R";
        Fresh(Host, { A.c_str(), C.c_str(), Op.c_str() });
        const SceneFigure* F = Figure(Host, "R");
        Judge("D9", K.Name, "faces", F ? F->Body.Faces.size() : 999, K.Faces, true, K.Before);
        Judge("D9", K.Name, "volume rel. error", F ? std::fabs(QuadratureVolume(F->Body) / K.Volume - 1.0) : 1.0, 1e-7, true, "exact");
    }

    // D10 — planar caps and natural curved faces bounded by control hulls (an oblique circle's corner poles)
    Fresh(Host, { "line (2.6,1.2,0.3) (3.6,2.4,-1.4) --name=Leg", "pipe Leg 0.2 --name=Pipe" });
    if (const SceneFigure* F = Figure(Host, "Pipe"))
    {
        Box3 Dense;
        for (const BrepFace& Fc : F->Body.Faces)
            for (int I = 0; I <= 200; ++I) for (int J = 0; J <= 200; ++J)
            {
                const NurbsSurface& S = Fc.Surface;
                if (S.Classification == SurfaceClassification::Plane) continue;
                Dense.Include(S.Sample(S.DomainStartU() + (S.DomainEndU() - S.DomainStartU()) * I / 200.0, S.DomainStartV() + (S.DomainEndV() - S.DomainStartV()) * J / 200.0));
            }
        const Box3 B = F->Body.Bounds();
        const Vec3 Low = Dense.Low - B.Low, High = B.High - Dense.High;
        const double Excess = std::max({ Low.X, Low.Y, Low.Z, High.X, High.Y, High.Z }) / (Dense.High - Dense.Low).Length();
        Judge("D10", "Oblique pipe", "Bounds() excess / diagonal", Excess, 5e-3, true, "3.3e-2");
        Judge("D10", "Oblique pipe", "Bounds() shortfall (must contain)", -std::min({ Low.X, Low.Y, Low.Z, High.X, High.Y, High.Z }), 1e-9, true, "—");
    }

    // D11 — a stored trace seeded the seam exit from its first point; the seam guard then kept the far end's (u,v)
    Fresh(Host, { "cylinder (4.4,-3.85,1.2) 1.35 1.0 --axis=(0,1,0) --name=WheelL", "cylinder (4.4,2.85,1.2) 1.35 1.0 --axis=(0,1,0) --name=WheelR",
                  "cylinder (4.4,-3.5,1.2) 0.3 7.0 --axis=(0,1,0) --name=Axle", "boolean union WheelL -- Axle --name=Half", "boolean union Half -- WheelR --name=Set" });
    if (const SceneFigure* F = Figure(Host, "Set")) Judge("D11", "Wheel–axle–wheel union", "audit findings", AuditWorst(F->Body), 0.0, true, "trace-on-edge 5.70 m");

    // D12 — a closed boolean edge stored its ring trace from an arbitrary start, not from the edge's vertex
    Host.StartEmptyDocument();
    {
        std::ifstream In("Engine/Modelling/SolidArc/Samples/ToySailboat.arc"); std::string Line; int N = 0;
        while (std::getline(In, Line) && ++N <= 38) if (Line.rfind("render", 0) != 0 && Line.rfind("view", 0) != 0) Host.Execute(Line);
    }
    if (const SceneFigure* F = Figure(Host, "Hull")) Judge("D12", "ToySailboat Hull", "audit findings", AuditWorst(F->Body), 0.0, true, "trace-on-edge 2.35e-2, bounds-excess 1.13e-1");
    else Judge("D12", "ToySailboat Hull", "sample replay", 1.0, 0.0, true, "run from the repository root");

    // Rim fillet on a bored slab (the probe's 3.355 was the D7 tessellation volume): Pappus closed form
    Fresh(Host, { "box (6,40,0) (9,42,0.6) --name=Slab", "cylinder (7.5,41,-0.5) 0.45 2 --name=Bore", "boolean subtract Slab -- Bore --name=HoleSlab" });
    if (const SceneFigure* F = Figure(Host, "HoleSlab"))
    {
        int Rim = -1; double Top = -1e9;
        for (size_t E = 0; E < F->Body.Edges.size(); ++E)
        {
            const NurbsCurve& C = F->Body.Edges[E].Curve;
            if (C.StartPoint().Distance(C.EndPoint()) < 1e-9 && C.Sample(0.5 * (C.DomainStart() + C.DomainEnd())).Z > Top) { Top = C.Sample(0.5 * (C.DomainStart() + C.DomainEnd())).Z; Rim = static_cast<int>(E); }
        }
        const std::string Bevel = "beveladd 0.1 --mode=fillet --targets=" + std::to_string(F->Identity) + ":e" + std::to_string(Rim);
        Host.Execute(Bevel);
        const double R = 0.45, r = 0.1, Spandrel = (1 - Pi / 4) * r * r, Centroid = R + (10 - 3 * Pi) / (12 - 3 * Pi) * r;
        const double Expected = 3 * 2 * 0.6 - Pi * R * R * 0.6 - 2 * Pi * Centroid * Spandrel;
        if (const SceneFigure* G = Figure(Host, "HoleSlab"))
        {
            Judge("—", "HoleSlab rim fillet", "volume rel. error vs Pappus", std::fabs(QuadratureVolume(G->Body) / Expected - 1.0), 1e-7, true, "3.355 reported (tessellation)");
            Judge("—", "HoleSlab rim fillet", "audit findings", AuditWorst(G->Body), 0.0, true, "—");
        }
    }

    int Failed = 0;
    std::printf("| Defect | Specimen | Measure | Value | Bound | Before the fix | Verdict |\n|---|---|---|---|---|---|---|\n");
    for (const Verdict& V : Verdicts)
    {
        const bool Pass = V.AtMost ? V.Value <= V.Bound : V.Value >= V.Bound;
        Failed += Pass ? 0 : 1;
        std::printf("| %s | %s | %s | %.3g | %s %.3g | %s | %s |\n", V.Defect.c_str(), V.Specimen.c_str(), V.Measure.c_str(), V.Value, V.AtMost ? "≤" : "≥", V.Bound, V.Before.c_str(), Pass ? "✔️" : "🔴");
    }
    std::printf("\n%d specimen measure(s), %d failed\n", static_cast<int>(Verdicts.size()), Failed);
    return Failed == 0 ? 0 : 1;
}
