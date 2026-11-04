/*==========================================================================================================================================
                                                            PORTALCLIPPING.H
==========================================================================================================================================*/
// 🧩 Splits a world-space triangle at an aperture plane, retaining interpolated shading attributes on each side.

#pragma once

#include "Frontier/Extensions/Portal/PortalProjection.h"

#include <array>
#include <vector>

namespace Frontier
{

struct PortalClipVertex
{
    std::array<double, 3> Position{};
    std::array<double, 3> Normal{};   // [-] interpolated, caller normalizes for shading
    std::array<double, 2> Texture{};
};
using PortalClipTriangle = std::array<PortalClipVertex, 3>;

struct PortalClipResult
{
    std::vector<PortalClipTriangle> Front;   // local positive z, including triangles wholly on the plane
    std::vector<PortalClipTriangle> Back;    // local negative z
};

// 📝 Clips against the infinite plane of the aperture; the caller must separately limit work to the actual portal opening.
//    No cap faces are generated: the result is not a closed physics collider. Failure leaves Output unchanged.
[[nodiscard]] bool ClipPortalTriangle(const PortalAperture& Aperture, const PortalClipTriangle& Triangle,
                                      PortalClipResult& Output) noexcept;

} // namespace Frontier
