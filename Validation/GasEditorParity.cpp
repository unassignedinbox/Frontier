// Machine-readable C++ output for comparison with the exhibit's JavaScript.
#include "Frontier/Engine/Effects/GasEditorModel.h"
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>

int main(int argc, char** argv)
{
    using namespace Frontier;
    std::cout << std::setprecision(17);
    if (argc < 2) return 1;
    const std::string Mode = argv[1];
    if (Mode == "catalogue")
    {
        for (const auto& S : GasParameterCatalogue())
            std::cout << S.Key << '\t' << S.Label << '\t' << S.Unit << '\t' << S.Default << '\t'
                      << S.Minimum << '\t' << S.Maximum << '\t' << S.Step << '\n';
        return 0;
    }
    if (Mode == "presets")
    {
        for (const auto& P : GasPresets()) std::cout << P.Name << '\n';
        return 0;
    }
    auto Gas = ConstructGasParameters();
    if (Mode == "preset" && argc == 3)
    {
        Gas["Unrelated"] = 42;
        if (!ApplyGasPreset(Gas, argv[2])) return 2;
        for (const auto& V : Gas) std::cout << V.first << '\t' << V.second << '\n';
        return 0;
    }
    if (Mode == "map" && argc == 5)
    {
        const std::string Key = argv[2];
        if (Key != "-") Gas.at(Key) = std::stod(argv[3]);
        const auto Mapped = MapPlumeSettings(Gas, std::string(argv[4]) == "1");
        std::cout << "PlumeEnabled\t" << (Mapped.Enabled ? 1 : 0) << '\n';
        for (const auto& V : Mapped.Values) std::cout << V.first << '\t' << V.second << '\n';
        return 0;
    }
    return 1;
}
