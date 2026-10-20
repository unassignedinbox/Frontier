/*====================================================================================================================================
                                                       SURFACEGEOMETRY.H
====================================================================================================================================*/
// 🧩 Analytic surfaces (plane, cylinder, cone, sphere, torus): evaluation, closed-form normals, closed-form inversion

#pragma once

//------------------------------------------------------------------------------------------------------------------------
//                                                    INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Geometry/VectorAlgebra.h"

#include <optional>
#include <variant>

namespace Frontier::SolidArcRemastered
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    SURFACE TYPES
//------------------------------------------------------------------------------------------------------------------------

// 💡 Analytic types first (research § D): every pair of them has closed-form or low-degree intersections, and their
//    normals and inversions are exact. B-spline surfaces join the variant when the salvaged NURBS organ is transplanted.
//    In every type the natural normal is ∂P/∂𝑢 × ∂P/∂𝑣 and points AWAY from the frame axis / centre.

// 📝 P(𝑢, 𝑣) = Origin + 𝑢·XAxis + 𝑣·YAxis; normal = ZAxis.
struct PlaneSurface
{
    OrthonormalFrame Frame = {};                                                             // [-] - Chart frame
};

// 📝 P(𝑢, 𝑣) = Origin + r·(cos 𝑢·X + sin 𝑢·Y) + 𝑣·Z; 𝑢 periodic 2π.
struct CylinderSurface
{
    OrthonormalFrame Frame  = {};                                                            // [-]   - Axis frame
    double           Radius = 1.0;                                                           // [len] - Radius
};

// 📝 P(𝑢, 𝑣) = Origin + (r₀ + 𝑣·s)·(cos 𝑢·X + sin 𝑢·Y) + 𝑣·Z; apex where r₀ + 𝑣·s = 0; 𝑢 periodic 2π.
struct ConeSurface
{
    OrthonormalFrame Frame       = {};                                                       // [-]   - Axis frame
    double           OriginRadius  = 1.0;                                                      // [len] - r₀, radius at 𝑣 = 0
    double           RadialSlope = -1.0;                                                     // [-]   - s = dr/d𝑣
};

// 📝 P(𝑢, 𝑣) = Origin + r·(cos 𝑣·(cos 𝑢·X + sin 𝑢·Y) + sin 𝑣·Z); 𝑣 ∈ [−π/2, π/2]; poles at 𝑣 = ±π/2.
struct SphereSurface
{
    OrthonormalFrame Frame  = {};                                                            // [-]   - Centre frame
    double           Radius = 1.0;                                                           // [len] - Radius
};

// 📝 P(𝑢, 𝑣) = Origin + (R + r·cos 𝑣)·(cos 𝑢·X + sin 𝑢·Y) + r·sin 𝑣·Z; 𝑢 and 𝑣 periodic 2π.
struct TorusSurface
{
    OrthonormalFrame Frame       = {};                                                       // [-]   - Centre frame
    double           MajorRadius = 2.0;                                                      // [len] - R, centre to tube axis
    double           MinorRadius = 0.5;                                                      // [len] - r, tube radius
};

using SurfaceGeometry = std::variant<PlaneSurface, CylinderSurface, ConeSurface, SphereSurface, TorusSurface>;

//------------------------------------------------------------------------------------------------------------------------
//                                                    STRUCTS
//------------------------------------------------------------------------------------------------------------------------

struct SurfaceSample
{
    Vector3 Position   = {};                                                                 // [len] - P(𝑢, 𝑣)
    Vector3 DerivativeU = {};                                                                // [len] - ∂P/∂𝑢
    Vector3 DerivativeV = {};                                                                // [len] - ∂P/∂𝑣
};

struct SurfacePeriodicity
{
    double PeriodU = 0.0;                                                                    // [-] - 0 when 𝑢 is not periodic
    double PeriodV = 0.0;                                                                    // [-] - 0 when 𝑣 is not periodic
};

//------------------------------------------------------------------------------------------------------------------------
//                                                    PUBLIC FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

[[nodiscard]] SurfaceSample      EvaluateSurface(const SurfaceGeometry& Surface, Vector2 Parameter) noexcept;

// Closed-form unit natural normal, defined at singular points too (poles, apex) where ∂P/∂𝑢 × ∂P/∂𝑣 vanishes.
[[nodiscard]] Vector3            EvaluateSurfaceNormal(const SurfaceGeometry& Surface, Vector2 Parameter) noexcept;

// Closed-form closest-point inversion; periodic parameters take the representative nearest to Reference.
[[nodiscard]] Vector2            ProjectOntoSurface(const SurfaceGeometry& Surface, Vector3 Position, Vector2 Reference) noexcept;

[[nodiscard]] SurfacePeriodicity ResolvePeriodicity(const SurfaceGeometry& Surface) noexcept;

[[nodiscard]] const char*        SurfaceCategoryLabel(const SurfaceGeometry& Surface) noexcept;

// 💡 Exact offset by Distance along the natural normal: every analytic type offsets to its own type (plane → plane,
//    cylinder r → r + δ, cone r₀ → r₀ + δ·√(1 + s²) with the same slope, sphere R → R + δ, torus r → r + δ). Empty when
//    the offset degenerates (radius ≤ 0, torus tube reaching its axis) — the curvature-radius limit of a rolling ball.
[[nodiscard]] std::optional<SurfaceGeometry> OffsetSurface(const SurfaceGeometry& Surface, double Distance) noexcept;

} // namespace Frontier::SolidArcRemastered
