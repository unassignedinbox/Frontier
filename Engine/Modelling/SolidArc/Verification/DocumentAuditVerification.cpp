// 📦 Engine/Modelling/SolidArc/Verification/DocumentAuditVerification.cpp — replay .arc documents, audit every body
//
// Each document is replayed command by command through ConsoleHost. After every command, each body whose topology or
//    volume changed is passed through Validate() (combinatorial) and GeometricAudit::Inspect() (geometric). The first
//    command that produces a defective body is the one reported, so a finding always names its cause.
//
//      DocumentAuditVerification <file.arc>... > audit.md
#include "Console/ConsoleHost.h"
#include "Kernel/GeometricAudit.h"

#include <cstdio>
#include <fstream>
#include <map>
#include <string>

using namespace Frontier;

namespace {

std::string Signature(const BrepBody& B)
{
    char Text[96]; std::snprintf(Text, sizeof Text, "%zu/%zu/%zu/%.9f", B.Vertices.size(), B.Edges.size(), B.Faces.size(), B.SignedVolume());
    return Text;
}

} // namespace

int main(int Count, char** Values)
{
    int Defective = 0, Audited = 0;
    std::printf("| Document | Line | Command | Figure | V/E/F | Validate | Geometric audit |\n|---|---|---|---|---|---|---|\n");
    for (int A = 1; A < Count; ++A)
    {
        std::ifstream In(Values[A]);
        std::string Name = Values[A]; Name = Name.substr(Name.find_last_of('/') + 1);
        ConsoleHost Host("_ClaudeScratch/tmp/proofs", 64u, 64u);
        Host.StartEmptyDocument();
        std::map<uint32_t, std::string> Seen;                                           // figure → last audited signature
        std::map<uint32_t, bool> Reported;                                              // report a figure's first defect only
        std::string Line; int Number = 0;
        while (std::getline(In, Line))
        {
            ++Number;
            if (Line.empty() || Line[0] == '#') continue;
            std::fflush(stdout);
            Host.Execute(Line);
            for (const SceneFigure& F : Host.AllFigures())
            {
                if (F.Body.Faces.empty()) continue;
                const std::string Now = Signature(F.Body);
                if (Seen[F.Identity] == Now) continue;
                Seen[F.Identity] = Now;
                ++Audited;
                const BodyReport V = F.Body.Validate();
                const AuditReport G = GeometricAudit::Inspect(F.Body);
                const bool Topology = V.Closed && V.Manifold && V.Oriented;
                if (Topology && G.Sound()) { Reported[F.Identity] = false; continue; }
                if (Reported[F.Identity]) continue;
                Reported[F.Identity] = true; ++Defective;
                std::string Command = Line.size() > 70 ? Line.substr(0, 67) + "..." : Line;
                std::printf("| %s | %d | `%s` | #%u | %d/%d/%d | %s | %s |\n", Name.c_str(), Number, Command.c_str(), F.Identity, V.Vertices, V.Edges, V.Faces,
                            Topology ? "✔️" : ("🔴 open " + std::to_string(V.OpenEdges) + " nonmanifold " + std::to_string(V.NonManifoldEdges) + " misoriented " + std::to_string(V.MisorientedEdges)).c_str(),
                            G.Summary().c_str());
            }
        }
    }
    std::printf("\n%d body state(s) audited, %d defective first occurrence(s)\n", Audited, Defective);
    return Defective == 0 ? 0 : 1;
}
