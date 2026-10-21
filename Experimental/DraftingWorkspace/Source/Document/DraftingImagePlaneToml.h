/*====================================================================================================================================
                                                   DRAFTINGIMAGEPLANETOML.H
====================================================================================================================================*/
// 🧩 [[ImagePlanes]] rows shared by VehicleCurves.toml and the .drafting.toml document (one encoding, one set of refusals)

#pragma once

//------------------------------------------------------------------------------------------------------------------------
//                                                    INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Document/DraftingImagePlane.h"

#include <toml++/toml.hpp>

#include <optional>
#include <string>
#include <vector>

namespace Frontier::DraftingWorkspace
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    PUBLIC FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

// { Name, Path, Crop = [l, t, r, b], Corner / Right / Down = [x, y, z], Opacity, Visible (only when false) }.
[[nodiscard]] inline toml::array EncodeDraftingImagePlanes(const std::vector<DraftingImagePlane>& Planes)
{
    toml::array Rows;
    for (const DraftingImagePlane& Plane : Planes)
    {
        toml::table Row{ { "Name", Plane.Name }, { "Path", Plane.Path },
                         { "Crop", toml::array{ int64_t{ Plane.Crop[0] }, int64_t{ Plane.Crop[1] }, int64_t{ Plane.Crop[2] }, int64_t{ Plane.Crop[3] } } },
                         { "Corner", toml::array{ Plane.Corner.X, Plane.Corner.Y, Plane.Corner.Z } },
                         { "Right", toml::array{ Plane.Right.X, Plane.Right.Y, Plane.Right.Z } },
                         { "Down", toml::array{ Plane.Down.X, Plane.Down.Y, Plane.Down.Z } }, { "Opacity", Plane.Opacity } };
        if (!Plane.VisibleCondition) Row.insert("Visible", false);
        Rows.push_back(std::move(Row));
    }
    return Rows;
}

// Scale converts file units to metres (mm files). Returns the refusal; Planes is filled only when every row is valid.
[[nodiscard]] inline std::string DecodeDraftingImagePlanes(const toml::array* Rows, double Scale, std::vector<DraftingImagePlane>& Planes)
{
    if (!Rows) return {};
    std::vector<DraftingImagePlane> Decoded;
    for (size_t Index = 0; Index < Rows->size(); ++Index)
    {
        const toml::node_view<const toml::node> View{ (*Rows)[Index] };
        DraftingImagePlane Plane;
        Plane.Name             = View["Name"].value_or(std::string{});
        Plane.Path             = View["Path"].value_or(std::string{});
        Plane.Opacity          = View["Opacity"].value_or(0.6);
        Plane.VisibleCondition = View["Visible"].value_or(true);
        if (const toml::array* Crop = View["Crop"].as_array(); Crop && Crop->size() == 4)
            for (int Side = 0; Side < 4; ++Side) Plane.Crop[Side] = static_cast<int>((*Crop)[Side].value_or(int64_t{ -1 }));
        else if (View["Crop"]) return "image plane " + std::to_string(Index + 1) + ": Crop must be [left, top, right, bottom]";
        Modeller::Vector3* Targets[3] = { &Plane.Corner, &Plane.Right, &Plane.Down };
        const char*        Keys[3]    = { "Corner", "Right", "Down" };
        for (int Key = 0; Key < 3; ++Key)
        {
            const toml::array* Triple = View[Keys[Key]].as_array();
            std::optional<double> Values[3];
            if (Triple && Triple->size() == 3) for (int Axis = 0; Axis < 3; ++Axis) Values[Axis] = (*Triple)[Axis].value<double>();
            if (!Values[0] || !Values[1] || !Values[2]) return "image plane " + std::to_string(Index + 1) + ": " + Keys[Key] + " is not [x, y, z]";
            *Targets[Key] = Modeller::Vector3{ *Values[0], *Values[1], *Values[2] } * Scale;
        }
        if (const std::string Refusal = ExamineDraftingImagePlane(Plane); !Refusal.empty()) return Refusal;
        Decoded.push_back(std::move(Plane));
    }
    Planes = std::move(Decoded);
    return {};
}

} // namespace Frontier::DraftingWorkspace
