#pragma once
/*==========================================================================================================================================
                                                             GASPARAMETERS.H
==========================================================================================================================================*/
// 🧩 The fluid/gas editor, as data: every control the HTML editor exposes (Source/SmokeSequence.js's
//    GasParameterCatalogue and InspectorPages' GasPresets), with the same names, ranges, steps and
//    defaults — a 1:1 port, so a value means the same thing in both editors. Sliders clamp and snap to
//    the catalogue exactly as the HTML ones do; presets are partial overrides of the defaults, exactly
//    as there. The solver (PlumeSolver) consumes ResolvePlumeSettings(), the same mapping the HTML
//    MapPlumeSettings performs.

#include <array>
#include <string_view>

namespace Frontier
{

struct GasParameterEntry
{
    std::string_view Key;
    float Default = 0.0f;
    float Minimum = 0.0f;
    float Maximum = 0.0f;
    float Step = 0.0f;
    std::string_view Unit;
    std::string_view Label;
};

inline constexpr std::array<GasParameterEntry, 32> GasParameterCatalogue = {{
    // source
    { "Emission",         1.0f,   0.0f,   4.0f,   0.05f,  "x",       "Emission rate" },
    { "Heat",             1.0f,   0.0f,   4.0f,   0.05f,  "x",       "Temperature" },
    { "Expansion",        1.0f,   0.0f,   3.0f,   0.05f,  "x",       "Volume expansion" },
    { "Soot",             1.0f,   0.0f,   4.0f,   0.05f,  "x",       "Soot yield" },
    { "Momentum",         9.0f,   0.0f,  24.0f,   0.5f,   "1/s",     "Carrier drag" },
    { "TreadEjection",    1.0f,   0.0f,   3.0f,   0.05f,  "x",       "Tread ejection" },
    // simulation
    { "Buoyancy",         2.4f,   0.0f,   8.0f,   0.1f,   "m/s2K",   "Buoyancy" },
    { "Dissipation",      0.10f,  0.0f,   0.9f,   0.01f,  "1/s",     "Dissipation" },
    { "Cooling",          0.55f,  0.0f,   0.95f,  0.01f,  "1/s",     "Cooling" },
    { "Vorticity",        3.0f,   0.0f,   6.0f,   0.1f,   "eps",     "Vorticity confinement" },
    { "Turbulence",       0.60f,  0.0f,   2.0f,   0.02f,  "m/s",     "Turbulence" },
    { "TurbulenceScale",  3.7f,   0.2f,   4.0f,   0.05f,  "1/m",     "Turbulence frequency" },
    { "WindSpeed",        0.8f,   0.0f,  12.0f,   0.1f,   "m/s",     "Wind speed" },
    { "WindHeading",     30.0f,   0.0f, 360.0f,   1.0f,   "deg",     "Wind heading" },
    { "TimeScale",        1.0f,   0.25f,  2.0f,   0.05f,  "x",       "Time scale" },
    { "PressureSweeps",  16.0f,   4.0f,  40.0f,   2.0f,   "",        "Pressure iterations" },
    // appearance
    { "Density",          1.0f,   0.2f,   3.0f,   0.05f,  "x",       "Density" },
    { "Albedo",           0.88f,  0.2f,   0.98f,  0.01f,  "",        "Albedo" },
    { "Phase",            0.38f,  0.0f,   0.9f,   0.01f,  "g",       "Forward scattering" },
    { "MultiScatter",     0.75f,  0.0f,   1.5f,   0.05f,  "",        "Multiple scattering" },
    { "Ambient",          1.0f,   0.0f,   3.0f,   0.05f,  "x",       "Ambient light" },
    { "Detail",           0.70f,  0.0f,   1.0f,   0.02f,  "",        "Detail" },
    { "DetailScale",      2.2f,   0.2f,   4.0f,   0.05f,  "1/m",     "Detail frequency" },
    { "Erosion",          1.0f,   0.0f,   1.5f,   0.05f,  "",        "Edge erosion" },
    { "Warp",             0.0f,   0.0f,   1.0f,   0.05f,  "",        "Billow warp" },
    { "Powder",           0.0f,   0.0f,   1.0f,   0.05f,  "",        "Powder" },
    // fire
    { "FlameIntensity",   1.0f,   0.0f,   4.0f,   0.05f,  "x",       "Flame intensity" },
    { "FlameTemperature", 1.0f,   0.7f,   1.6f,   0.01f,  "x",       "Flame temperature" },
    { "FlameCore",     1800.0f, 1300.0f, 2400.0f, 10.0f,   "K",       "Flame core" },
    { "FlameTip",      1150.0f,  900.0f, 1600.0f, 10.0f,   "K",       "Flame tip" },
    { "FlameFlicker",     0.6f,   0.0f,   1.0f,   0.02f,  "",        "Flicker" },
    { "FlameLoss",        0.96f,  0.5f,   0.995f, 0.005f, "",        "Flame decay" },
}};

struct GasParameters
{
    float Emission = 1.0f, Heat = 1.0f, Expansion = 1.0f, Soot = 1.0f, Momentum = 9.0f, TreadEjection = 1.0f;
    float Buoyancy = 2.4f, Dissipation = 0.10f, Cooling = 0.55f, Vorticity = 3.0f, Turbulence = 0.60f;
    float TurbulenceScale = 3.7f, WindSpeed = 0.8f, WindHeading = 30.0f, TimeScale = 1.0f, PressureSweeps = 16.0f;
    float Density = 1.0f, Albedo = 0.88f, Phase = 0.38f, MultiScatter = 0.75f, Ambient = 1.0f;
    float Detail = 0.70f, DetailScale = 2.2f, Erosion = 1.0f, Warp = 0.0f, Powder = 0.0f;
    float FlameIntensity = 1.0f, FlameTemperature = 1.0f, FlameCore = 1800.0f, FlameTip = 1150.0f;
    float FlameFlicker = 0.6f, FlameLoss = 0.96f, FireLight = 1.0f;

    // 📝 One slider move, as the HTML editor performs it: clamp to the catalogue, snap to its step.
    //    Returns false for a key the catalogue does not know.
    bool Assign(std::string_view Key, float Value);
    [[nodiscard]] float Query(std::string_view Key) const;

    // 📝 A preset: the named partial override over the defaults (InspectorPages' GasPresets, verbatim).
    bool ApplyPreset(std::string_view PresetName);
};

// 📝 What the solver consumes: the HTML MapPlumeSettings mapping, same formulas.
struct PlumeSettings
{
    bool Enabled = true;
    float EmissionRate = 1.0f, Density = 1.2f, Heat = 1.0f, Soot = 1.0f, Buoyancy = 2.4f;
    float Dissipation = 0.10f, Cooling = 0.55f, Vorticity = 3.0f, Turbulence = 0.6f, TurbulenceScale = 3.7f;
    float WindSpeed = 0.8f, WindHeadingDegrees = 30.0f;
    float Albedo = 0.88f, Anisotropy = 0.38f, Scattering = 0.75f, Ambient = 1.0f;
    float Detail = 0.70f, DetailScale = 2.2f;
    float Fire = 1.0f, FlameCore = 1800.0f, FlameTip = 1150.0f, FireLight = 1.0f, FlameDecay = 4.0f;
    int PressureSweeps = 16;
};

[[nodiscard]] PlumeSettings ResolvePlumeSettings(const GasParameters& Gas, bool Enabled = true);

} // namespace Frontier
