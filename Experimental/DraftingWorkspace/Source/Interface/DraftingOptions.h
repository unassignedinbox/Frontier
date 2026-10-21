/*====================================================================================================================================
                                                       DRAFTINGOPTIONS.H
====================================================================================================================================*/
// 🧩 DraftingWorkspace options (DraftingWorkspace.toml): window, viewport, interface, files, Vulkan driver

#pragma once

//------------------------------------------------------------------------------------------------------------------------
//                                                    INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include <string>

namespace Frontier::DraftingWorkspace
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    STRUCTS
//------------------------------------------------------------------------------------------------------------------------

// 📝 Every field has a default, so a missing file or key is never an error; unknown keys are ignored.
struct DraftingOptions
{
    int         WindowWidth         = 1600;                                                  // [px]  - Initial window / capture width
    int         WindowHeight        = 900;                                                   // [px]  - Initial window / capture height
    float       EdgeStroke          = 1.5f;                                                  // [px]  - B-rep edge stroke
    bool        GridEnabled         = true;                                                  // [-]   - Ground grid
    bool        OrthographicEnabled = false;                                                 // [-]   - Start in parallel projection
    double      FieldOfView         = 0.7;                                                   // [rad] - Vertical perspective angle
    std::string FontPath            = {};                                                    // [-]   - UI font (empty: DM Sans from EngineContent)
    float       FontSize            = 15.0f;                                                 // [px]  - UI font size
    std::string RecoveryPath        = "_ClaudeScratch/autosave/DraftingWorkspace.drafting.toml"; // [-] - Autosave target
    double      AutosaveDelay       = 0.6;                                                   // [s]   - Quiet time before an autosave
    std::string DocumentPath        = "Bracket.drafting.toml";                               // [-]   - Default Save / Open path
    std::string CurvesPath          = "Experimental/VehicleSurfacing/Samples/SyntheticBonnet.toml"; // [-] - Default Import curves path
    std::string DriverPath          = {};                                                    // [-]   - Vulkan ICD loaded directly (empty: loader)
    bool        ValidationEnabled   = false;                                                 // [-]   - Khronos validation layer when present
    std::string Refusal             = {};                                                    // [-]   - Parse failure of the file (defaults kept)
};

//------------------------------------------------------------------------------------------------------------------------
//                                                    PUBLIC FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

[[nodiscard]] DraftingOptions LoadDraftingOptions(const std::string& Path);
[[nodiscard]] std::string     EncodeDraftingOptions(const DraftingOptions& Options);

} // namespace Frontier::DraftingWorkspace
