/*====================================================================================================================================
                                                      INTERSECTIONCURVE.H
====================================================================================================================================*/
// 🧩 Exact intersection curves of two surfaces: a marched chart as the guide, Newton onto both surfaces per evaluation

#pragma once

//------------------------------------------------------------------------------------------------------------------------
//                                                    INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Geometry/CurveGeometry.h"
#include "Geometry/SurfaceGeometry.h"

#include <vector>

namespace Frontier::SolidArcRemastered
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    STRUCTS
//------------------------------------------------------------------------------------------------------------------------

// 💡 Parasolid-style intersection curve (research § F): the curve IS the intersection of First and Second; the chart is
//    only a guide. C(τ) = the point of S₁ ∩ S₂ in the normal plane of the cubic Hermite guide H(τ):
//        d₁(X) = 0,  d₂(X) = 0,  H′(τ)·(X − H(τ)) = 0      (3D Newton, parameter-free, as the marching corrector)
//    so every evaluation lies on both surfaces to rounding, however coarse the chart. Differentiating the plane
//    condition gives the exact derivative of the evaluation map:
//        C′(τ) = σ·𝐭,  𝐭 = N₁ × N₂ / |N₁ × N₂| (oriented with H′),  σ = (|H′|² − (C − H)·H″) / (𝐭·H′)
//    τ is chord length along the chart for an open curve, and 2π·chord / perimeter for a closed one, so closed curves
//    share the 2π period of circles and ellipses everywhere a full turn is assumed.
struct TracedIntersection
{
    SurfaceGeometry      First    = PlaneSurface{};                                         // [-]   - S₁ (copy)
    SurfaceGeometry      Second   = PlaneSurface{};                                         // [-]   - S₂ (copy)
    std::vector<Vector3> Points   = {};                                                     // [len] - Guide knots (closed: last = first)
    std::vector<Vector3> Tangents = {};                                                     // [len] - dH/dτ at the knots
    std::vector<double>  Knots    = {};                                                     // [-]   - τ at the knots, ascending
    bool                 Closed   = false;                                                  // [-]   - Loop of period 2π
    double               Perimeter = 0.0;                                                   // [len] - Chord length of the guide
};

//------------------------------------------------------------------------------------------------------------------------
//                                                    PUBLIC FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

// 💡 Builds the carrier from ordered points on both surfaces (a marched branch): duplicates dropped, then the guide is
//    refined (corrected midpoints) until consecutive tangents turn by ≤ 0.1 rad, so the normal-plane solve is unique.
[[nodiscard]] CurveGeometry BuildIntersectionCurve(const SurfaceGeometry& First, const SurfaceGeometry& Second, const std::vector<Vector3>& Points, bool Closed);

// τ range of the carrier: [Knots.front(), Knots.back()] (closed: [0, 2π]).
[[nodiscard]] std::pair<double, double> MeasureIntersectionCurveRange(const TracedIntersection& Trace) noexcept;

// P_reversed(τ) = P(−τ), as ReverseCurve does for lines and conics.
[[nodiscard]] CurveGeometry ReverseIntersectionCurve(const TracedIntersection& Trace);

// Largest distance of either surface from C(τ) over Samples uniform τ in [Low, High] (validation of the carrier).
[[nodiscard]] double MeasureIntersectionCurveResidual(const TracedIntersection& Trace, double Low, double High, int Samples) noexcept;

} // namespace Frontier::SolidArcRemastered
