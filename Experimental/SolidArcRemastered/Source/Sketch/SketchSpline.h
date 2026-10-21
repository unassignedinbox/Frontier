/*====================================================================================================================================
                                                        SKETCHSPLINE.H
====================================================================================================================================*/
// 🧩 Freeform sketch curves (Hermite, Bezier, NURBS, cubic spline) resolved to exact polynomial clamped B-splines

#pragma once

//------------------------------------------------------------------------------------------------------------------------
//                                                    INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Sketch/SketchSolver.h"

#include <string>
#include <vector>

namespace Frontier::SolidArcRemastered
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    STRUCTS
//------------------------------------------------------------------------------------------------------------------------

// 📝 C(τ) = Σ Nᵢ,ₚ(τ)·Pᵢ on a clamped knot vector (all weights 1, so the 3D lift is an exact NURBS curve).
struct SketchSplineCurve
{
    int                  Degree = 3;                                                         // [-]   - p
    std::vector<double>  Knots  = {};                                                        // [-]   - Clamped, Poles + p + 1 values
    std::vector<Vector2> Poles  = {};                                                        // [len] - Control points
};

struct SketchSplineDerivatives
{
    Vector2 Position = {};                                                                   // [len] - C(τ)
    Vector2 First    = {};                                                                   // [len] - C′(τ)
    Vector2 Second   = {};                                                                   // [len] - C″(τ)
};

struct SketchSplineOutcome
{
    SketchSplineCurve Curve   = {};                                                          // [-] - Valid when Refusal is empty
    bool              Closed  = false;                                                       // [-] - First point = last point
    std::string       Refusal = {};                                                          // [-] - Why no curve
};

//------------------------------------------------------------------------------------------------------------------------
//                                                    PUBLIC FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

// 💡 Points → exact B-spline, uniform parameter (span i ↦ τ ∈ [i, i + 1] for the interpolating forms):
//    Hermite     — through every point, tangent Tᵢ = (Qᵢ₊₁ − Qᵢ₋₁)/2 (ends: one-sided; closed: wrapped) · C¹;
//    CubicSpline — through every point, tangents from Tᵢ₋₁ + 4Tᵢ + Tᵢ₊₁ = 3(Qᵢ₊₁ − Qᵢ₋₁) with natural ends
//                  (2T₀ + T₁ = 3(Q₁ − Q₀)) or the cyclic system when closed · C²;
//    both as Bézier spans (Qᵢ, Qᵢ + Tᵢ/3, Qᵢ₊₁ − Tᵢ₊₁/3, Qᵢ₊₁) with triple interior knots;
//    Bezier      — one Bézier of degree n on the n + 1 control points (≤ 16 points);
//    Nurbs       — clamped uniform B-spline of degree min(3, n) on the control points.
//    Refused: fewer than 2 points, coincident consecutive points, a closed curve with fewer than 3 distinct points.
[[nodiscard]] SketchSplineOutcome ResolveSketchSpline(SketchSplineForm Form, const std::vector<Vector2>& Points, double Scale);

// The same for a Spline curve of a solved sketch (its Points, at their current positions).
[[nodiscard]] SketchSplineOutcome ResolveSketchCurveSpline(const SketchModel& Sketch, const SketchCurve& Curve);

[[nodiscard]] SketchSplineDerivatives EvaluateSketchSpline(const SketchSplineCurve& Curve, double Tau) noexcept;

// Same point set traversed backwards: poles reversed, knots mirrored (τ′ = a + b − τ).
void ReverseSketchSpline(SketchSplineCurve& Curve) noexcept;

// Distinct knots from first to last (span boundaries, where the polynomial pieces meet).
[[nodiscard]] std::vector<double> CollectSketchSplineBreaks(const SketchSplineCurve& Curve);

// Display polyline of any sketch curve at its current point positions: lines 2 points, arcs and circles ~64 per turn,
// splines 16 per knot span (a spline that cannot resolve falls back to its control polygon).
[[nodiscard]] std::vector<Vector2> SampleSketchCurve(const SketchModel& Sketch, const SketchCurve& Curve);

} // namespace Frontier::SolidArcRemastered
