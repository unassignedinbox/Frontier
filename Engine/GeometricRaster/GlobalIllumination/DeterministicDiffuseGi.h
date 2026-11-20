#pragma once

#include "Engine/Mathematics/LinearColors.h"
#include "Engine/Mathematics/AxisCoordinates.h"
#include <cmath>
#include <cstdint>

namespace Frontier::GeometricRaster
{
// Cosine-weighted hemisphere quadrature. RadianceAtDirection must return incident
// radiance including visibility. No ambient constant or screen-space history.
// The cosine / pi BRDF cancels the cosine / pi sampling density.
inline float ReverseBinaryFraction(uint32_t Value) noexcept
{
    Value = (Value << 16u) | (Value >> 16u);
    Value = ((Value & 0x00ff00ffu) << 8u) | ((Value & 0xff00ff00u) >> 8u);
    Value = ((Value & 0x0f0f0f0fu) << 4u) | ((Value & 0xf0f0f0f0u) >> 4u);
    Value = ((Value & 0x33333333u) << 2u) | ((Value & 0xccccccccu) >> 2u);
    Value = ((Value & 0x55555555u) << 1u) | ((Value & 0xaaaaaaaau) >> 1u);
    return static_cast<float>(static_cast<double>(Value) / 4294967296.0);
}

template<class IncidentRadianceFunction>
Mathematics::LinearRGB IntegrateDeterministicDiffuseGi(
    Mathematics::LinearRGB Reflectance, uint32_t SampleCount, uint32_t RotationKey,
    IncidentRadianceFunction&& RadianceAtDirection)
{
    Mathematics::LinearRGB Sum {};
    if (SampleCount == 0u) return Sum;
    const float Rotation = ReverseBinaryFraction(RotationKey);
    for (uint32_t K = 0; K < SampleCount; ++K)
    {
        const float U = (static_cast<float>(K) + 0.5f) / SampleCount;
        const float Phi = 6.28318530718f * std::fmod(ReverseBinaryFraction(K) + Rotation, 1.0f);
        const float R = std::sqrt(U);
        const auto Li = RadianceAtDirection(Mathematics::Axis3D {
            R * std::cos(Phi), R * std::sin(Phi), std::sqrt(1.0f - U) });
        Sum = Sum + Li;
    }
    const float InvCount = 1.0f / SampleCount;
    return { Sum.R * Reflectance.R * InvCount,
             Sum.G * Reflectance.G * InvCount,
             Sum.B * Reflectance.B * InvCount };
}
}
