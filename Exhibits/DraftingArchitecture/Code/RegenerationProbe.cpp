// 📦 Exhibits/DraftingArchitecture/Code/RegenerationProbe.cpp — section H: does editing the driving sketch regenerate the extrusion, with and without a downstream bevel?
#include "Console/ConsoleHost.h"
#include <cstdio>
#include <string>
using namespace Frontier;
static void Show(ConsoleHost& Host, const char* Step)
{
    for (const SceneFigure& F : Host.Document().Figures())
        if (!F.Body.Faces.empty())
        {
            const Box3 X = F.Body.Bounds(); const BodyReport V = F.Body.Validate();
            std::printf("  %-26s %-12s F %2d  volume %.6f  x %.3f..%.3f  z %.3f..%.3f\n", Step, F.Name.c_str(), V.Faces, V.Volume, X.Low.X, X.High.X, X.Low.Z, X.High.Z);
        }
}
static void Run(bool Bevel)
{
    ConsoleHost Host("_ClaudeScratch/tmp/proofs", 64u, 64u);
    Host.StartEmptyDocument();
    std::printf("%s\n", Bevel ? "== with beveladd" : "== without beveladd");
    Host.Execute("rect (0,100) (2,101.5) --name=DriverRect");
    Host.Execute("extrude DriverRect 1 --name=DrivenBlock"); Show(Host, "extrude");
    if (Bevel)
    {
        uint32_t Id = 0; for (const SceneFigure& F : Host.Document().Figures()) if (F.Name == "DrivenBlock") Id = F.Identity;
        Host.Execute(("beveladd 0.2 --mode=fillet --targets=" + std::to_string(Id) + ":e0").c_str()); Show(Host, "beveladd");
    }
    Host.Execute("transform DriverRect --scale=(2,1,1) --pivot=(0,100,0)"); Show(Host, "transform x2");
    Host.Execute("move DriverRect (0,0,0.5)"); Show(Host, "move +z0.5");
}
int main() { Run(false); Run(true); }
