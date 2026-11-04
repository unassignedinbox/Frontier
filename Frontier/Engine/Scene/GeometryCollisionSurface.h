/*==========================================================================================================================================
                                                      GEOMETRYCOLLISIONSURFACE.H
==========================================================================================================================================*/
// 🧩 Immutable placed scene facets for building fixed collision bodies at simulation start.

#pragma once

#include <array>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace Frontier
{

// 📝 Three world-space points per triangle, already transformed and wound for the placed geometry.
struct GeometryCollisionSurface
{
    std::vector<std::array<float, 3>> Corners;
};
using GeometryCollisionSurfaces = std::unordered_map<uint64_t, GeometryCollisionSurface>;

} // namespace Frontier
