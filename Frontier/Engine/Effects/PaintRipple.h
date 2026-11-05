#pragma once
/*==========================================================================================================================================
                                                          PAINTRIPPLE.H
============================================================================================================================================
 🧩 Paint application exactly as the reference configurator performs it (Truck.tsx, "circular material
 ripple runtime"): choosing a finish does not swap the car's colour at once. An expanding sphere grows
 from the touch point — the raycast hit of a drag-and-drop, or the nose default (2.75, 1.25, 0) on a tap —
 and every surface point inside it takes the next finish while the soft 0.55 m edge of the sphere is still
 crossing it:

     coverage = (1 - smoothstep(R - 0.55, R, distance(point, centre))) * active
     R        = easeOutCubic(min(t / duration, 1)) * maximumRadius

 Paint runs 1.35 s out to 6.4 m; the rim ripple runs 1.5 s out to 4.6 m. When the sphere completes, the
 target finish (colour, roughness, metalness — and clearcoat, via the completion hook the caller wires)
 is committed, exactly as finishRun() commits the Three.js material. This class is the mechanism only:
 presentations ask it for the per-point coverage, so the CPU mirror and any GPU front-end agree. */

#include <algorithm>
#include <array>
#include <cmath>

namespace Frontier
{

struct PaintRipple
{
    std::array<float, 3> CentreMetres = { 2.75f, 0.0f, 1.25f };  // [m] world-space touch point (nose default, mapped to Frontier axes)
    std::array<float, 3> NextColour = { 1.0f, 1.0f, 1.0f };      // linear albedo the sphere carries
    float NextRoughness = 0.26f;                                 // [-] committed with the colour on completion
    float NextMetalness = 0.9f;                                  // [-] committed with the colour on completion
    float DurationSeconds = 1.35f;                               // [s] paint ripple duration (rims: 1.5 s)
    float MaximumRadiusMetres = 6.4f;                            // [m] paint reach (rims: 4.6 m)
    float ElapsedSeconds = 0.0f;                                 // [s]
    float RadiusMetres = 0.0f;                                   // [m] current sphere radius
    bool Active = false;

    // 📝 Starts the sphere. A ripple already in flight is committed by the caller first (the reference
    //    calls finishRun() before beginRun()); this simply re-arms from the new touch point.
    void Begin(const std::array<float, 3>& Centre, const std::array<float, 3>& Colour, float Roughness, float Metalness,
               float Duration = 1.35f, float MaximumRadius = 6.4f) noexcept
    {
        CentreMetres = Centre;
        NextColour = Colour;
        NextRoughness = Roughness;
        NextMetalness = Metalness;
        DurationSeconds = Duration;
        MaximumRadiusMetres = MaximumRadius;
        ElapsedSeconds = 0.0f;
        RadiusMetres = 0.0f;
        Active = true;
    }

    // 📝 Advances the sphere; returns true on the step that completes it (the caller then commits
    //    NextColour/NextRoughness/NextMetalness as the vehicle's finish, as finishRun() does).
    bool Step(float StepSeconds) noexcept
    {
        if (!Active)
            return false;
        ElapsedSeconds += StepSeconds;
        const float K = DurationSeconds > 0.0f ? std::min(ElapsedSeconds / DurationSeconds, 1.0f) : 1.0f;
        const float Eased = 1.0f - (1.0f - K) * (1.0f - K) * (1.0f - K);   // easeOutCubic
        RadiusMetres = Eased * MaximumRadiusMetres;
        if (K >= 1.0f)
        {
            Active = false;
            return true;
        }
        return false;
    }

    // 📝 The reference shader's mix factor for one surface point: 1 well inside the sphere, 0 outside,
    //    smoothstepped across the trailing 0.55 m of the radius. 0 when no ripple is running.
    [[nodiscard]] float ResolveCoverage(const std::array<float, 3>& PointMetres) const noexcept
    {
        if (!Active)
            return 0.0f;
        const float Dx = PointMetres[0] - CentreMetres[0];
        const float Dy = PointMetres[1] - CentreMetres[1];
        const float Dz = PointMetres[2] - CentreMetres[2];
        const float Distance = std::sqrt(Dx * Dx + Dy * Dy + Dz * Dz);
        const float Edge0 = RadiusMetres - 0.55f, Edge1 = RadiusMetres;
        float T = Edge1 > Edge0 ? (Distance - Edge0) / (Edge1 - Edge0) : 1.0f;
        T = std::clamp(T, 0.0f, 1.0f);
        const float Smooth = T * T * (3.0f - 2.0f * T);   // smoothstep
        return 1.0f - Smooth;
    }
};

} // namespace Frontier
