/*==========================================================================================================================================
                                                           PORTALPROJECTION.H
==========================================================================================================================================*/
// 🧩 Similarity mapping between proportional rectangular apertures. No rendering or physics ownership is implied.

#pragma once

#include <array>

namespace Frontier
{

struct PortalAperture
{
    std::array<double, 3> Position{};
    std::array<double, 4> Orientation{0.0, 0.0, 0.0, 1.0}; // local +z is the front normal
    std::array<double, 2> HalfExtent{1.0, 1.0};
};

struct PortalMotion
{
    std::array<double, 3> Position{};
    std::array<double, 4> Orientation{0.0, 0.0, 0.0, 1.0};
    std::array<double, 3> LinearVelocity{};
    std::array<double, 3> AngularVelocity{};
    double Scale = 1.0;
};

// 📝 A point crossing from the positive side to the negative side of the source plane, inside its rectangle.
[[nodiscard]] bool FindPortalCrossing(const PortalAperture& Aperture, const std::array<double, 3>& Previous,
                                      const std::array<double, 3>& Current, double& Fraction) noexcept;
// 📝 Applies the 180-degree local-y turn and uniform destination/source aperture ratio. Failure leaves Output unchanged.
[[nodiscard]] bool ProjectPortalMotion(const PortalAperture& Source, const PortalAperture& Destination,
                                       const PortalMotion& Input, PortalMotion& Output) noexcept;

} // namespace Frontier
