// 📦 Engine/Modelling/SolidArc/Verification/FeatureSuitabilityVerification.cpp — is each modelling feature fit for CAD?
//
// Replays a feature corpus through ConsoleHost. Every command row reports, for the bodies it created or changed:
//    outcome (ran / refused + the printed reason), Validate().Solid(), GeometricAudit, exactness (freeform edges / all edges,
//    largest pole count) and the volume against a closed form when the row carries one. A second table measures the
//    persistent-naming question: after a command that keeps a body's identity, how many of the edges that survived
//    geometrically still carry their old index.
//
//    Corpus syntax (one command per line):
//      ## Section title
//      <command>                 {id:Name} → figure identity, {face:Name:+z} → face whose outward normal is +z at the
//                                extreme along it (axes ±x ±y ±z), {face:Name:+z:inner} → the least extreme such face
//                                {rim:Name:+z} → closed edge most extreme along the axis
//      <command>  #= 4.0         expected volume of the result body (closed form)
//      @naming <Name>            snapshot Name's edges; the next command row reports index stability for it
//
//      FeatureSuitabilityVerification <corpus.arc> > suitability.md
#include "Console/ConsoleHost.h"
#include "Kernel/GeometricAudit.h"
#include "Verification/QuadratureVolume.h"
#include <chrono>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <unistd.h>
#include <vector>

using namespace Frontier;

namespace
{

std::string Signature(const BrepBody& B)
{
    char Text[96]; std::snprintf(Text, sizeof Text, "%zu/%zu/%zu/%.9f", B.Vertices.size(), B.Edges.size(), B.Faces.size(), B.SignedVolume());
    return Text;
}

// Run one command with stdout captured, so a refusal's printed reason can be reported.
bool Run(ConsoleHost& Host, const std::string& Line, std::string& Printed)
{
    std::fflush(stdout);
    char Path[] = "/tmp/suitabilityXXXXXX";
    const int Capture = mkstemp(Path);
    const int Saved = dup(1);
    dup2(Capture, 1);
    const bool Ran = Host.Execute(Line);
    std::fflush(stdout);
    dup2(Saved, 1); close(Saved);
    std::ifstream In(Path); std::stringstream Text; Text << In.rdbuf(); Printed = Text.str();
    close(Capture); unlink(Path);
    return Ran;
}

Vec3 AxisOf(const std::string& Token)
{
    const double S = Token[0] == '-' ? -1.0 : 1.0;
    const char A = Token.back();
    return A == 'x' ? Vec3(S, 0, 0) : A == 'y' ? Vec3(0, S, 0) : Vec3(0, 0, S);
}

// Face of Body whose outward normal at its carrier's mid-parameter is Axis; the most (or least) extreme along Axis.
int FaceAlong(const BrepBody& Body, Vec3 Axis, bool Inner)
{
    int Best = -1; double Score = 0;
    for (size_t F = 0; F < Body.Faces.size(); ++F)
    {
        const NurbsSurface& S = Body.Faces[F].Surface;
        const double U = 0.5 * (S.DomainStartU() + S.DomainEndU()), V = 0.5 * (S.DomainStartV() + S.DomainEndV());
        Vec3 N = S.Normal(U, V); if (Body.Faces[F].Reversed) N = N * -1.0;
        if (N.Dot(Axis) < 0.999) continue;
        const double Along = S.Sample(U, V).Dot(Axis) * (Inner ? -1.0 : 1.0);
        if (Best < 0 || Along > Score) { Best = static_cast<int>(F); Score = Along; }
    }
    return Best;
}

std::string Substitute(ConsoleHost& Host, std::string Line)
{
    for (size_t Open; (Open = Line.find('{')) != std::string::npos;)
    {
        const size_t Close = Line.find('}', Open);
        if (Close == std::string::npos) break;
        std::vector<std::string> Part; std::stringstream Spec(Line.substr(Open + 1, Close - Open - 1)); std::string P;
        while (std::getline(Spec, P, ':')) Part.push_back(P);
        std::string Value = "?";
        const SceneFigure* F = Part.size() >= 2 ? Host.Document().Find(Part[1]) : nullptr;
        if (F && Part[0] == "id") Value = std::to_string(F->Identity);
        if (F && Part[0] == "rim" && Part.size() >= 3)                                 // closed edge most extreme along the axis
        {
            int Best = -1; double Score = 0; const Vec3 Axis = AxisOf(Part[2]);
            for (size_t E = 0; E < F->Body.Edges.size(); ++E)
            {
                const NurbsCurve& C = F->Body.Edges[E].Curve;
                if (!C.Closed()) continue;
                const double Along = C.Sample(0.5 * (C.DomainStart() + C.DomainEnd())).Dot(Axis);
                if (Best < 0 || Along > Score) { Best = static_cast<int>(E); Score = Along; }
            }
            Value = std::to_string(Best);
        }
        if (F && Part[0] == "face" && Part.size() >= 3) Value = std::to_string(FaceAlong(F->Body, AxisOf(Part[2]), Part.size() >= 4 && Part[3] == "inner"));
        Line.replace(Open, Close - Open + 1, Value);
    }
    return Line;
}

struct Exactness { int Edges = 0, FreeformEdges = 0, Faces = 0, FreeformFaces = 0; size_t MaxPoles = 0; };
Exactness Measure(const BrepBody& B)
{
    Exactness X;
    for (const BrepEdge& E : B.Edges) { ++X.Edges; X.FreeformEdges += E.Curve.Classification == CurveClassification::Freeform; X.MaxPoles = std::max(X.MaxPoles, E.Curve.Poles.size()); }
    for (const BrepFace& F : B.Faces) { ++X.Faces; X.FreeformFaces += F.Surface.Classification == SurfaceClassification::Freeform; }
    return X;
}

std::vector<Vec3> EdgeMidpoints(const BrepBody& B)
{
    std::vector<Vec3> M;
    for (const BrepEdge& E : B.Edges) M.push_back(E.Curve.Sample(0.5 * (E.Curve.DomainStart() + E.Curve.DomainEnd())));
    return M;
}

std::string Clip(std::string S, size_t N)
{
    for (char& C : S) if (C == '\n' || C == '|' || C == '`') C = ' ';
    while (!S.empty() && S.back() == ' ') S.pop_back();
    return S.size() > N ? S.substr(0, N - 3) + "..." : S;
}
} // namespace

int main(int Count, char** Values)
{
    if (Count < 2) { std::fprintf(stderr, "usage: FeatureSuitabilityVerification <corpus.arc>\n"); return 2; }
    std::ifstream In(Values[1]);
    ConsoleHost Host("_ClaudeScratch/tmp/proofs", 64u, 64u);
    Host.StartEmptyDocument();
    std::map<uint32_t, std::string> Seen;
    std::string NamingFigure; std::vector<Vec3> NamingBefore;
    std::vector<std::string> NamingRows;
    int Rows = 0, Refused = 0, Defective = 0, VolumeMisses = 0;

    std::printf("| # | Command | Outcome | Solid | Audit | Freeform edges | Max poles | Volume | ms |\n|---|---|---|---|---|---|---|---|---|\n");
    std::string Line;
    while (std::getline(In, Line))
    {
        if (Line.empty() || (Line[0] == '#' && Line.rfind("##", 0) != 0)) continue;
        if (Line.rfind("##", 0) == 0) { std::printf("| | **%s** | | | | | | | |\n", Clip(Line.substr(2), 90).c_str()); continue; }
        if (Line.rfind("@naming ", 0) == 0)
        {
            NamingFigure = Line.substr(8);
            const SceneFigure* F = Host.Document().Find(NamingFigure);
            NamingBefore = F ? EdgeMidpoints(F->Body) : std::vector<Vec3>{};
            continue;
        }
        double Expected = std::nan("");
        if (const size_t Mark = Line.find("#="); Mark != std::string::npos) { Expected = std::stod(Line.substr(Mark + 2)); Line = Line.substr(0, Mark); }
        const std::string Command = Substitute(Host, Line);
        std::string Printed;
        const auto Start = std::chrono::steady_clock::now();
        const bool Ran = Run(Host, Command, Printed);
        const double Ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - Start).count();
        ++Rows;

        // bodies this command created or changed
        std::vector<const SceneFigure*> Changed;
        for (const SceneFigure& F : Host.Document().Figures())
        {
            if (F.Body.Faces.empty()) continue;
            const std::string Now = Signature(F.Body);
            if (Seen[F.Identity] == Now) continue;
            Seen[F.Identity] = Now; Changed.push_back(&F);
        }
        std::string Outcome = Ran ? "✔️ ran" : "🔴 refused: " + Clip(Printed, 120);
        if (!Ran) ++Refused;
        if (Changed.empty())
        {
            if (Ran && std::isnan(Expected)) continue;                                  // sketch / setup rows stay quiet
            std::printf("| %d | `%s` | %s | — | — | — | — | %s | %.0f |\n", Rows, Clip(Command, 80).c_str(), Outcome.c_str(), Ran ? "no body" : "—", Ms);
            continue;
        }
        const SceneFigure& F = *Changed.back();
        const BodyReport V = F.Body.Validate();
        const AuditReport G = GeometricAudit::Inspect(F.Body);
        const Exactness X = Measure(F.Body);
        const bool Sound = V.Solid() && G.Sound();
        if (!Sound && V.Closed) ++Defective;
        std::string Volume = "—";
        if (!std::isnan(Expected))
        {
            const double Exact = V.Closed ? QuadratureVolume(F.Body) : 0.0;
            const double Error = std::fabs(Exact - Expected);
            const bool Match = Error <= 1e-6 * std::max(1.0, std::fabs(Expected));
            if (!Match) ++VolumeMisses;
            char T[96]; std::snprintf(T, sizeof T, "%.6f %s (err %.1e)", Exact, Match ? "✔️" : "🔴", Error); Volume = T;
        }
        else if (V.Closed) { char T[32]; std::snprintf(T, sizeof T, "%.6f", QuadratureVolume(F.Body)); Volume = T; }
        char Edges[48]; std::snprintf(Edges, sizeof Edges, "%d / %d", X.FreeformEdges, X.Edges);
        std::printf("| %d | `%s` | %s | %s | %s | %s | %zu | %s | %.0f |\n", Rows, Clip(Command, 80).c_str(), Outcome.c_str(),
                    V.Solid() ? "✔️" : (!V.Closed && V.Manifold ? "sheet" : "🔴"), G.Sound() ? "✔️" : ("🔴 " + Clip(G.Summary(), 60)).c_str(),
                    Edges, X.MaxPoles, Volume.c_str(), Ms);

        if (!NamingFigure.empty())
        {
            {
                const std::vector<Vec3> After = EdgeMidpoints(F.Body);
                int Survived = 0, SameIndex = 0;
                for (size_t I = 0; I < NamingBefore.size(); ++I)
                    for (size_t J = 0; J < After.size(); ++J)
                        if (After[J].Distance(NamingBefore[I]) <= 1e-9) { ++Survived; SameIndex += I == J; break; }
                char T[256]; std::snprintf(T, sizeof T, "| `%s` | %s | %zu → %zu | %d | %d | %d |", Clip(Command, 70).c_str(), NamingFigure.c_str(), NamingBefore.size(), After.size(), Survived, SameIndex, Survived - SameIndex);
                NamingRows.push_back(T);
            }
            NamingFigure.clear();
        }
    }
    if (!NamingRows.empty())
    {
        std::printf("\n**Persistent naming** — edges that survived an operation unchanged (same midpoint ≤ 1e-9), and whether their index did:\n\n");
        std::printf("| Operation | Body | Edges before → after | Survived | Same index | Renumbered |\n|---|---|---|---|---|---|\n");
        for (const std::string& R : NamingRows) std::printf("%s\n", R.c_str());
    }
    std::printf("\n%d command row(s), %d refused, %d closed-but-defective, %d volume miss(es)\n", Rows, Refused, Defective, VolumeMisses);
    return 0;
}
