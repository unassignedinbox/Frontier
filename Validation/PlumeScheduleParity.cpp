#include "Frontier/Engine/Effects/PlumeSchedule.h"
#include <iostream>
#include <limits>
int main()
{
    using namespace Frontier;
    for (const auto& Grade : PlumeGrades())
    {
        PlumeSchedule S(Grade);
        for (int I = 0; I < 11; ++I)
        {
            if (I == 9) S.Clear();
            auto Gas = ConstructGasParameters();
            if (I == 5) { Gas["WindSpeed"] = 5.5; Gas["FlameLoss"] = 0.995; }
            auto Settings = MapPlumeSettings(Gas, I != 8);
            const double Inf = std::numeric_limits<double>::infinity();
            PlumeSourceBounds Sources{};
            Sources.Count = (I == 1 || I == 2 || I == 3 || I == 5 || I == 8 || I == 10) ? 2 : 0;
            Sources.Minimum = Sources.Count ? std::array<double,3>{-3.4, -2.5, 0.1} : std::array<double,3>{Inf,Inf,Inf};
            Sources.Maximum = Sources.Count ? std::array<double,3>{4.8, 5.5, 6.6} : std::array<double,3>{-Inf,-Inf,-Inf};
            const double Time[] = {0.01, 0.008, 0.008, 0.008, 0.02, 0.1, 0.025, 0.04, 0.016, 0.01, 0.02};
            const std::array<double,3> Focus = I >= 5 ? std::array<double,3>{40,-20,0} : std::array<double,3>{-1,2,0};
            S.Schedule(Time[I], Time[I], Focus, Sources, Settings, {0.1,0.2,0.9}, I == 5 ? 10 : 0);
            std::cout << int(S.Resident()) << ' ' << int(S.StepPending()) << ' ' << S.State() << ' ' << S.StepsTaken() << ' ' << S.Origin()[0] << ' ' << S.Origin()[1];
            for (auto Word : S.Record()) std::cout << ' ' << Word;
            std::cout << '\n';
            S.TransportRecorded();
        }
    }
}
