/*====================================================================================================================================
                                                           SOLIDARCKNIFE.H
====================================================================================================================================*/
// 🧩 Knife inserts (KitOps-style boolean library): built-in cutters authored procedurally and STEP files from a library
//    folder, each normalized to the insert frame and kept as canonical STEP text plus its edge polylines for the ghost

#pragma once

//------------------------------------------------------------------------------------------------------------------------
//                                                    INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "UI/SolidArc/Document/SolidArcDocument.h"

#include "Engine/GeometryProcessing/Construction/BodyTransform.h"

#include <array>
#include <string>
#include <vector>

namespace Frontier::SolidArc
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    CONSTANTS
//------------------------------------------------------------------------------------------------------------------------

inline constexpr double SolidArcInsertLift = 0.05;                                            // [-] - Insert top above its surface (insert widths)

//------------------------------------------------------------------------------------------------------------------------
//                                                    STRUCTS
//------------------------------------------------------------------------------------------------------------------------

// 📝 Insert frame: unit width across x / y, centred on the z axis, top at z = +SolidArcInsertLift, the cutter reaching
//    down −z. The surface it is placed on is z = 0 with outward normal +z, so a Difference cuts a pocket of Depth.
struct SolidArcInsert
{
    std::string                                             Name     = {};                    // [-] - Caption ("Round hole")
    std::string                                             Path     = {};                    // [-] - Library file (empty: built in)
    std::string                                             Exchange = {};                    // [-] - Canonical STEP text, insert frame
    double                                                  Depth    = 0.0;                   // [-] - Reach below the surface (insert widths)
    std::vector<std::array<GeometryProcessing::Vector3, 2>> Segments = {};                    // [-] - Edge polylines, insert frame (ghost)
};

struct SolidArcInsertLibrary
{
    std::string                 Folder       = {};                                            // [-]  - Folder scanned for *.step / *.stp
    std::vector<SolidArcInsert> Inserts      = {};                                            // [-]  - Built-ins first, then files by name
    std::vector<std::string>    Notes        = {};                                            // [-]  - Inserts that could not be made / read
    double                      Milliseconds = 0.0;                                           // [ms] - Assembly time
};

//------------------------------------------------------------------------------------------------------------------------
//                                                    PUBLIC FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

// 💡 Built-ins (square pocket, round hole, countersink, slot, hex pocket, triangle, cross) are drawn and swept in a
//    scratch document by the same commands a user runs; every STEP file in Folder is read by ReadSolidArcStepFile (first
//    solid). Each body is scaled to unit width, centred and lifted (NormalizeSolidArcInsert) before its text is written.
[[nodiscard]] SolidArcInsertLibrary AssembleSolidArcInserts(const std::string& Folder);

// Insert frame → world: Spot's origin on the surface, its x̂ turned by Turn [rad] about n̂, everything scaled by Size
// [len]. RaisedCondition flips the insert about x̂ so it stands out of the surface (Union: a boss instead of a pocket).
[[nodiscard]] GeometryProcessing::AffineMap PlaceSolidArcInsert(const GeometryProcessing::OrthonormalFrame& Spot, double Size, double Turn, bool RaisedCondition);

// Writes body Key as Folder/Name.step (the next free Name2, Name3 … when taken); the library normalizes it on load.
// Returns a refusal (empty on success); Path receives the file written.
[[nodiscard]] std::string SaveSolidArcInsert(const SolidArcDocument& Document, const std::string& Key, const std::string& Folder, const std::string& Name, std::string& Path);

} // namespace Frontier::SolidArc
