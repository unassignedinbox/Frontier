/*====================================================================================================================================
                                                         SOLIDARCOPTIONS.CPP
====================================================================================================================================*/
// 🧩 SolidArc options (SolidArcWorkspace.toml): window, viewport, interface, files, Vulkan driver

//------------------------------------------------------------------------------------------------------------------------
//                                                    INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "UI/SolidArc/Interface/SolidArcOptions.h"

#include <toml++/toml.hpp>

#include <filesystem>
#include <sstream>

namespace Frontier::SolidArc
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    PUBLIC FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

SolidArcOptions LoadSolidArcOptions(const std::string& Path)
{
    SolidArcOptions Options;
    if (Path.empty() || !std::filesystem::exists(Path)) return Options;
    toml::table Root;
    try
    {
        Root = toml::parse_file(Path);
    }
    catch (const toml::parse_error& Failure)
    {
        Options.Refusal = Path + ": " + std::string(Failure.description());
        return Options;
    }
    Options.WindowWidth         = static_cast<int>(Root["Window"]["Width"].value_or(int64_t{ Options.WindowWidth }));
    Options.WindowHeight        = static_cast<int>(Root["Window"]["Height"].value_or(int64_t{ Options.WindowHeight }));
    Options.EdgeStroke          = static_cast<float>(Root["Viewport"]["EdgeStroke"].value_or(double{ Options.EdgeStroke }));
    Options.GridEnabled         = Root["Viewport"]["Grid"].value_or(Options.GridEnabled);
    Options.OrthographicEnabled = Root["Viewport"]["Orthographic"].value_or(Options.OrthographicEnabled);
    Options.FieldOfView         = Root["Viewport"]["FieldOfView"].value_or(Options.FieldOfView);
    Options.FontPath            = Root["Interface"]["FontPath"].value_or(Options.FontPath);
    Options.FontSize            = static_cast<float>(Root["Interface"]["FontSize"].value_or(double{ Options.FontSize }));
    Options.RecoveryPath        = Root["Files"]["RecoveryPath"].value_or(Options.RecoveryPath);
    Options.AutosaveDelay       = Root["Files"]["AutosaveDelay"].value_or(Options.AutosaveDelay);
    Options.DocumentPath        = Root["Files"]["DocumentPath"].value_or(Options.DocumentPath);
    Options.CurvesPath          = Root["Files"]["CurvesPath"].value_or(Options.CurvesPath);
    Options.DriverPath          = Root["Vulkan"]["DriverPath"].value_or(Options.DriverPath);
    Options.ValidationEnabled   = Root["Vulkan"]["Validation"].value_or(Options.ValidationEnabled);
    return Options;
}

std::string EncodeSolidArcOptions(const SolidArcOptions& Options)
{
    const toml::table Root{
        { "Window", toml::table{ { "Width", Options.WindowWidth }, { "Height", Options.WindowHeight } } },
        { "Viewport", toml::table{ { "EdgeStroke", double(Options.EdgeStroke) }, { "Grid", Options.GridEnabled },
                                   { "Orthographic", Options.OrthographicEnabled }, { "FieldOfView", Options.FieldOfView } } },
        { "Interface", toml::table{ { "FontPath", Options.FontPath }, { "FontSize", double(Options.FontSize) } } },
        { "Files", toml::table{ { "RecoveryPath", Options.RecoveryPath }, { "AutosaveDelay", Options.AutosaveDelay }, { "DocumentPath", Options.DocumentPath }, { "CurvesPath", Options.CurvesPath } } },
        { "Vulkan", toml::table{ { "DriverPath", Options.DriverPath }, { "Validation", Options.ValidationEnabled } } },
    };
    std::ostringstream Stream;
    Stream << "# SolidArc options — every key is optional\n" << Root << "\n";
    return Stream.str();
}

} // namespace Frontier::SolidArc
