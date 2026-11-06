#pragma once

// C++ data/mapping port of VehicleHmiPrototype's SmokeSequence.js and the
// GasPresets in InspectorPages.js. This is not a UI or a fluid renderer.
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace Frontier
{
struct GasParameterSpec
{
    std::string Key, Label, Unit;
    double Default, Minimum, Maximum, Step;
};
struct GasPreset
{
    std::string Name;
    std::map<std::string, double> Overrides;
};
using GasValues = std::map<std::string, double>;
struct PlumeSettings
{
    bool Enabled;
    GasValues Values;
};

const std::vector<GasParameterSpec>& GasParameterCatalogue() noexcept;
const std::vector<GasPreset>& GasPresets() noexcept;
GasValues ConstructGasParameters();
// As in the exhibit, applying a preset resets gas values only. Returns false for an unknown preset.
bool ApplyGasPreset(GasValues& Gas, std::string_view Name);
// Equivalent to MapPlumeSettings. The input must contain all keys from ConstructGasParameters().
PlumeSettings MapPlumeSettings(const GasValues& Gas, bool Enabled = true);
} // namespace Frontier
