/*====================================================================================================================================
                                                       CURVEGEOMETRY.CPP
====================================================================================================================================*/
// 🧩 Analytic 3D curves (line, circle) and 2D parameter-space curves (SP-curves on faces) with exact inversion

#include "Geometry/CurveGeometry.h"

#include <cmath>
#include <numbers>

namespace Frontier::SolidArcRemastered
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    3D CURVES
//------------------------------------------------------------------------------------------------------------------------

CurveSample EvaluateCurve(const CurveGeometry& Curve, double τ) noexcept
{
    if (const auto* Line = std::get_if<StraightLine>(&Curve))
    {
        return { Line->Origin + Line->Direction * τ, Line->Direction };
    }
    const auto& Arc = std::get<CircularArc>(Curve);
    const double Cosine = std::cos(τ);                                                       // [-] - cos τ
    const double Sine   = std::sin(τ);                                                       // [-] - sin τ
    return {
        Arc.Frame.Origin + (Arc.Frame.XAxis * Cosine + Arc.Frame.YAxis * Sine) * Arc.Radius,
        (Arc.Frame.XAxis * -Sine + Arc.Frame.YAxis * Cosine) * Arc.Radius,
    };
}

double ProjectOntoCurve(const CurveGeometry& Curve, Vector3 Position, double IntervalLow) noexcept
{
    if (const auto* Line = std::get_if<StraightLine>(&Curve))
    {
        return Dot(Position - Line->Origin, Line->Direction);
    }
    const auto&   Arc    = std::get<CircularArc>(Curve);
    const Vector3 Offset = Position - Arc.Frame.Origin;                                      // [len] - From the centre
    double        τ      = std::atan2(Dot(Offset, Arc.Frame.YAxis), Dot(Offset, Arc.Frame.XAxis));
    constexpr double Period = 2.0 * std::numbers::pi;                                        // [rad] - Full turn
    // 📝 Fold into [IntervalLow, IntervalLow + 2π); a hair below the low end maps to the low end, not to the high end.
    τ = IntervalLow + std::fmod(std::fmod(τ - IntervalLow, Period) + Period, Period);
    if (τ - IntervalLow > Period - 1.0e-12) τ = IntervalLow;
    return τ;
}

bool PeriodicCondition(const CurveGeometry& Curve) noexcept
{
    return std::holds_alternative<CircularArc>(Curve);
}

const char* CurveCategoryLabel(const CurveGeometry& Curve) noexcept
{
    return std::holds_alternative<StraightLine>(Curve) ? "Line" : "Circle";
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    2D PARAMETER-SPACE CURVES
//------------------------------------------------------------------------------------------------------------------------

ParameterSample EvaluateParameterCurve(const ParameterCurve& Curve, double τ) noexcept
{
    if (const auto* Line = std::get_if<ParameterLine>(&Curve))
    {
        return { Line->Origin + Line->Direction * τ, Line->Direction };
    }
    if (const auto* Projected = std::get_if<ParameterProjection>(&Curve))
    {
        return EvaluateProjectedParameterCurve(*Projected->Projection, τ);
    }
    const auto&  Ellipse = std::get<ParameterEllipse>(Curve);
    const double Cosine  = std::cos(τ);                                                      // [-] - cos τ
    const double Sine    = std::sin(τ);                                                      // [-] - sin τ
    return {
        Ellipse.Centre + (Ellipse.FirstAxis * Cosine + Ellipse.SecondAxis * Sine) * Ellipse.Radius,
        (Ellipse.FirstAxis * -Sine + Ellipse.SecondAxis * Cosine) * Ellipse.Radius,
    };
}

} // namespace Frontier::SolidArcRemastered
