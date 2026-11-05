/*==========================================================================================================================================
                                                            GASPARAMETERS.CPP
==========================================================================================================================================*/

#include "Frontier/Engine/Effects/GasParameters.h"

#include <algorithm>
#include <cmath>

namespace Frontier
{

namespace
{

float GasParameters::* ResolveField(std::string_view Key)
{
    using F = float GasParameters::*;
    static constexpr std::array<std::pair<std::string_view, F>, 33> Fields = {{
        { "Emission", &GasParameters::Emission }, { "Heat", &GasParameters::Heat },
        { "Expansion", &GasParameters::Expansion }, { "Soot", &GasParameters::Soot },
        { "Momentum", &GasParameters::Momentum }, { "TreadEjection", &GasParameters::TreadEjection },
        { "Buoyancy", &GasParameters::Buoyancy }, { "Dissipation", &GasParameters::Dissipation },
        { "Cooling", &GasParameters::Cooling }, { "Vorticity", &GasParameters::Vorticity },
        { "Turbulence", &GasParameters::Turbulence }, { "TurbulenceScale", &GasParameters::TurbulenceScale },
        { "WindSpeed", &GasParameters::WindSpeed }, { "WindHeading", &GasParameters::WindHeading },
        { "TimeScale", &GasParameters::TimeScale }, { "PressureSweeps", &GasParameters::PressureSweeps },
        { "Density", &GasParameters::Density }, { "Albedo", &GasParameters::Albedo },
        { "Phase", &GasParameters::Phase }, { "MultiScatter", &GasParameters::MultiScatter },
        { "Ambient", &GasParameters::Ambient }, { "Detail", &GasParameters::Detail },
        { "DetailScale", &GasParameters::DetailScale }, { "Erosion", &GasParameters::Erosion },
        { "Warp", &GasParameters::Warp }, { "Powder", &GasParameters::Powder },
        { "FlameIntensity", &GasParameters::FlameIntensity }, { "FlameTemperature", &GasParameters::FlameTemperature },
        { "FlameCore", &GasParameters::FlameCore }, { "FlameTip", &GasParameters::FlameTip },
        { "FlameFlicker", &GasParameters::FlameFlicker }, { "FlameLoss", &GasParameters::FlameLoss },
        { "FireLight", &GasParameters::FireLight }
    }};
    for (const auto& [Name, Field] : Fields)
        if (Name == Key)
            return Field;
    return nullptr;
}

const GasParameterEntry* FindEntry(std::string_view Key)
{
    for (const GasParameterEntry& Entry : GasParameterCatalogue)
        if (Entry.Key == Key)
            return &Entry;
    return nullptr;
}

} // namespace

bool GasParameters::Assign(std::string_view Key, float Value)
{
    const GasParameterEntry* Entry = FindEntry(Key);
    float GasParameters::* Field = ResolveField(Key);
    if (!Entry || !Field)
        return false;
    const float Clamped = std::clamp(Value, Entry->Minimum, Entry->Maximum);
    const float Snapped = Entry->Step > 0.0f
        ? Entry->Minimum + std::round((Clamped - Entry->Minimum) / Entry->Step) * Entry->Step
        : Clamped;
    this->*Field = std::clamp(Snapped, Entry->Minimum, Entry->Maximum);
    return true;
}

float GasParameters::Query(std::string_view Key) const
{
    const float GasParameters::* Field = ResolveField(Key);
    return Field ? this->*Field : 0.0f;
}

bool GasParameters::ApplyPreset(std::string_view PresetName)
{
    // A refused preset changes nothing; an accepted one starts from the defaults, as in HTML.
    static constexpr std::array<std::string_view, 6> Known = {
        "Tyre smoke", "Thick burnout", "Light haze", "Inferno", "Crosswind", "Legacy billows" };
    if (std::find(Known.begin(), Known.end(), PresetName) == Known.end())
        return false;
    *this = GasParameters{};
    if (PresetName == "Tyre smoke")
        return true;
    if (PresetName == "Thick burnout")
    {
        Emission = 2.2f; Density = 1.6f; Dissipation = 0.12f; Albedo = 0.9f; Vorticity = 2.2f; Expansion = 1.4f; MultiScatter = 0.9f;
        return true;
    }
    if (PresetName == "Light haze")
    {
        Emission = 0.6f; Density = 0.55f; Dissipation = 0.35f; Detail = 0.75f; Erosion = 0.9f; Albedo = 0.92f;
        return true;
    }
    if (PresetName == "Inferno")
    {
        FlameIntensity = 2.4f; FlameTemperature = 1.18f; FlameFlicker = 0.85f; Soot = 2.2f; Heat = 1.8f; FireLight = 2.2f; Buoyancy = 3.4f;
        return true;
    }
    if (PresetName == "Crosswind")
    {
        WindSpeed = 5.5f; WindHeading = 95.0f; Turbulence = 0.7f; Dissipation = 0.28f;
        return true;
    }
    if (PresetName == "Legacy billows")
    {
        Density = 1.0f; Dissipation = 0.22f; Vorticity = 1.6f; Turbulence = 0.4f; TurbulenceScale = 1.1f;
        Albedo = 0.86f; Phase = 0.45f; MultiScatter = 0.45f; Detail = 0.65f; DetailScale = 0.85f; Erosion = 0.75f;
        Warp = 0.35f; Powder = 0.2f; WindSpeed = 0.0f;
        return true;
    }
    return false;
}

PlumeSettings ResolvePlumeSettings(const GasParameters& Gas, bool Enabled)
{
    PlumeSettings Settings{};
    Settings.Enabled = Enabled;
    Settings.EmissionRate = Gas.Emission;
    Settings.Density = 1.2f * Gas.Density;
    Settings.Heat = Gas.Heat;
    Settings.Soot = Gas.Soot;
    Settings.Buoyancy = Gas.Buoyancy;
    Settings.Dissipation = Gas.Dissipation;
    Settings.Cooling = Gas.Cooling;
    Settings.Vorticity = Gas.Vorticity;
    Settings.Turbulence = Gas.Turbulence;
    Settings.TurbulenceScale = Gas.TurbulenceScale;
    Settings.WindSpeed = Gas.WindSpeed;
    Settings.WindHeadingDegrees = Gas.WindHeading;
    Settings.Albedo = Gas.Albedo;
    Settings.Anisotropy = Gas.Phase;
    Settings.Scattering = Gas.MultiScatter;
    Settings.Ambient = Gas.Ambient;
    Settings.Detail = Gas.Detail;
    Settings.DetailScale = Gas.DetailScale;
    Settings.Fire = Gas.FlameIntensity;
    Settings.FlameCore = Gas.FlameCore * Gas.FlameTemperature;
    Settings.FlameTip = Gas.FlameTip * Gas.FlameTemperature;
    Settings.FireLight = Gas.FireLight;
    Settings.FlameDecay = -std::log(std::max(0.005f, 1.0f - Gas.FlameLoss));
    Settings.PressureSweeps = static_cast<int>(Gas.PressureSweeps + 0.5f);
    return Settings;
}

} // namespace Frontier
