/*====================================================================================================================================
                                                         VEHICLECURVEIMPORT.H
====================================================================================================================================*/
// 🧩 Vehicle plan V1: VehicleCurves.toml (edge loops exported from Blender as ordered limit points) — decode, encode

#pragma once

//------------------------------------------------------------------------------------------------------------------------
//                                                    INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Document/DraftingDocument.h"

#include <string>
#include <vector>

namespace Frontier::DraftingWorkspace
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    TYPES
//------------------------------------------------------------------------------------------------------------------------

// 📝 One exported edge loop: world-space limit points in order (metres after decoding), closed when the loop wraps.
struct VehicleCurve
{
    std::string                    Name            = {};                                     // [-]   - Loop name from Blender ("ShoulderLine")
    bool                           ClosedCondition = false;                                  // [-]   - Last point joins the first
    std::vector<Modeller::Vector3> Points          = {};                                     // [len] - Ordered limit points (metres)
};

struct VehicleCurveFile
{
    std::string               Source  = {};                                                  // [-] - Exporter note ("Blender 4.2 · Body.001")
    std::vector<VehicleCurve> Curves  = {};                                                  // [-] - In file order
    std::string               Refusal = {};                                                  // [-] - Why the file was rejected (empty: fine)
};

//------------------------------------------------------------------------------------------------------------------------
//                                                    PUBLIC FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

// 💡 Format = "VehicleCurves", Units = "m" | "mm", [[Curves]] { Name, Closed, Points = [[x, y, z], …] }. Refuses a wrong
//    format tag, unknown units, a curve without a name or with fewer than 2 points, and any non-numeric coordinate —
//    never a partial import. Millimetre files are scaled to metres.
[[nodiscard]] VehicleCurveFile DecodeVehicleCurves(const std::string& Text);
[[nodiscard]] VehicleCurveFile ReadVehicleCurves(const std::string& Path);

// Metres, shortest round-trip doubles: decode(encode(file)) gives back bit-equal points (the V1 gate).
[[nodiscard]] std::string      EncodeVehicleCurves(const VehicleCurveFile& File);

}
