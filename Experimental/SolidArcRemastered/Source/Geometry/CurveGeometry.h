/*====================================================================================================================================
                                                        CURVEGEOMETRY.H
====================================================================================================================================*/
// 🧩 Analytic 3D curves (line, circle) and 2D parameter-space curves (SP-curves on faces) with exact inversion

#pragma once

//------------------------------------------------------------------------------------------------------------------------
//                                                    INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Geometry/VectorAlgebra.h"

#include <cstdint>
#include <memory>
#include <variant>

namespace Frontier::SolidArcRemastered
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    3D CURVES
//------------------------------------------------------------------------------------------------------------------------

// 📝 P(τ) = Origin + τ·Direction, Direction unit length, so τ is arc length.
struct StraightLine
{
    Vector3 Origin    = {};                                                                  // [len] - Point at τ = 0
    Vector3 Direction = { 1.0, 0.0, 0.0 };                                                   // [-]   - Unit direction
};

// 📝 P(τ) = Centre + r·(cos τ·XAxis + sin τ·YAxis); the circle's axis is Frame.ZAxis (counter-clockwise about it).
struct CircularArc
{
    OrthonormalFrame Frame  = {};                                                            // [-]   - Origin is the centre
    double           Radius = 1.0;                                                           // [len] - Circle radius
};

using CurveGeometry = std::variant<StraightLine, CircularArc>;

struct CurveSample
{
    Vector3 Position   = {};                                                                 // [len] - P(τ)
    Vector3 Derivative = {};                                                                 // [len] - dP/dτ
};

[[nodiscard]] CurveSample EvaluateCurve(const CurveGeometry& Curve, double τ) noexcept;

// Closest-point inversion; the result is folded into [IntervalLow, IntervalLow + period) for periodic curves.
[[nodiscard]] double ProjectOntoCurve(const CurveGeometry& Curve, Vector3 Position, double IntervalLow) noexcept;

[[nodiscard]] bool PeriodicCondition(const CurveGeometry& Curve) noexcept;

[[nodiscard]] const char* CurveCategoryLabel(const CurveGeometry& Curve) noexcept;

//------------------------------------------------------------------------------------------------------------------------
//                                                    2D PARAMETER-SPACE CURVES
//------------------------------------------------------------------------------------------------------------------------

// 💡 SP-curves are parameterised by the SAME τ as their edge (same-parameter rule), so a fin can be sampled in 3D and
//    in (𝑢, 𝑣) at identical τ and the two compared directly — that comparison is the audit's p-curve check.

// 📝 q(τ) = Origin + τ·Direction (Direction need not be unit).
struct ParameterLine
{
    Vector2 Origin    = {};                                                                  // [-] - (𝑢, 𝑣) at τ = 0
    Vector2 Direction = { 1.0, 0.0 };                                                        // [-] - d(𝑢, 𝑣)/dτ
};

// 📝 q(τ) = Centre + r·(cos τ·FirstAxis + sin τ·SecondAxis) — the image of a circle on a plane's (𝑢, 𝑣) chart.
struct ParameterEllipse
{
    Vector2 Centre     = {};                                                                 // [-] - Chart centre
    Vector2 FirstAxis  = { 1.0, 0.0 };                                                       // [-] - Image of the circle XAxis
    Vector2 SecondAxis = { 0.0, 1.0 };                                                       // [-] - Image of the circle YAxis
    double  Radius     = 1.0;                                                                // [-] - Scale applied to both axes
};

// 📝 q(τ) = projection of a 3D curve onto a surface chart — exact for a curve lying on the surface (closed-form inversion,
//    periodic image chosen by an unwrap table). Used for section circles that are not iso-lines (plane ∩ sphere off-axis).
//    The body lives in Geometry/ProjectedCurve.h (it needs SurfaceGeometry; this header must not).
struct ProjectedParameterCurve;

struct ParameterProjection
{
    std::shared_ptr<const ProjectedParameterCurve> Projection = {};                         // [-] - Shared, immutable
};

using ParameterCurve = std::variant<ParameterLine, ParameterEllipse, ParameterProjection>;

struct ParameterSample
{
    Vector2 Position   = {};                                                                 // [-] - q(τ)
    Vector2 Derivative = {};                                                                 // [-] - dq/dτ
};

[[nodiscard]] ParameterSample EvaluateParameterCurve(const ParameterCurve& Curve, double τ) noexcept;

// Defined in Geometry/ProjectedCurve.cpp.
[[nodiscard]] ParameterSample EvaluateProjectedParameterCurve(const ProjectedParameterCurve& Curve, double τ) noexcept;

} // namespace Frontier::SolidArcRemastered
