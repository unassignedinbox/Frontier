#include "Frontier/Engine/Effects/GasEditorModel.h"

#include <algorithm>
#include <cmath>

namespace Frontier
{
namespace
{
#include "Frontier/Engine/Effects/GasEditorData.inl"
}

const std::vector<GasParameterSpec>& GasParameterCatalogue() noexcept { return Catalogue; }
const std::vector<GasPreset>& GasPresets() noexcept { return Presets; }

GasValues ConstructGasParameters()
{
    GasValues Gas;
    for (const auto& Spec : Catalogue) Gas.emplace(Spec.Key, Spec.Default);
    return Gas;
}

bool ApplyGasPreset(GasValues& Gas, std::string_view Name)
{
    const auto It = std::find_if(Presets.begin(), Presets.end(),
        [Name](const GasPreset& Preset) { return Preset.Name == Name; });
    if (It == Presets.end()) return false;
    // Object.assign(Gas, ConstructGasParameters(), Values) preserves unrelated keys.
    for (const auto& Pair : ConstructGasParameters()) Gas[Pair.first] = Pair.second;
    for (const auto& Pair : It->Overrides) Gas[Pair.first] = Pair.second;
    return true;
}

PlumeSettings MapPlumeSettings(const GasValues& Gas, bool Enabled)
{
    PlumeSettings Out{ Enabled, DefaultPlumeValues };
    const auto Set = [&](const char* Target, const char* Source) { Out.Values[Target] = Gas.at(Source); };
    Set("PlumeEmissionRate", "Emission");
    Out.Values["PlumeDensity"] = 1.2 * Gas.at("Density");
    Set("PlumeHeat", "Heat"); Set("PlumeSoot", "Soot"); Set("PlumeBuoyancy", "Buoyancy");
    Set("PlumeDissipation", "Dissipation"); Set("PlumeCooling", "Cooling");
    Set("PlumeVorticity", "Vorticity"); Set("PlumeTurbulence", "Turbulence");
    Set("PlumeTurbulenceScale", "TurbulenceScale"); Set("PlumeWindSpeed", "WindSpeed");
    Set("PlumeWindDirection", "WindHeading"); Set("PlumeAlbedo", "Albedo");
    Set("PlumeAnisotropy", "Phase"); Set("PlumeScattering", "MultiScatter");
    Set("PlumeAmbient", "Ambient"); Set("PlumeDetail", "Detail"); Set("PlumeDetailScale", "DetailScale");
    Set("PlumeFire", "FlameIntensity");
    Out.Values["PlumeFlameCore"] = Gas.at("FlameCore") * Gas.at("FlameTemperature");
    Out.Values["PlumeFlameTip"] = Gas.at("FlameTip") * Gas.at("FlameTemperature");
    Set("PlumeFireLight", "FireLight");
    Out.Values["PlumeFlameDecay"] = -std::log(std::max(0.005, 1.0 - Gas.at("FlameLoss")));
    return Out;
}
} // namespace Frontier
