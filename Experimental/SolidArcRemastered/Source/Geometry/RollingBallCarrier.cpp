/*====================================================================================================================================
                                                     ROLLINGBALLCARRIER.CPP
====================================================================================================================================*/
// 🧩 Procedural rolling-ball carrier and its contact rails, evaluated from an exact spine (traced, elliptical or any curve)

//------------------------------------------------------------------------------------------------------------------------
//                                                    INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Geometry/RollingBallCarrier.h"

#include "Intersection/IntersectionCurve.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>

namespace Frontier::SolidArcRemastered
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    PRIVATE HELPERS
//------------------------------------------------------------------------------------------------------------------------

namespace
{
    constexpr double FullTurn = 2.0 * std::numbers::pi;                                      // [rad] - 𝑣 period

    // Folds Value into [Low, Low + Period); identity when Period is 0.
    [[nodiscard]] double Fold(double Value, double Low, double Period) noexcept
    {
        if (Period <= 0.0) return Value;
        double Folded = Low + std::fmod(std::fmod(Value - Low, Period) + Period, Period);
        if (Folded - Low > Period - 1.0e-12) Folded = Low;
        return Folded;
    }

    // Keeps an open carrier inside its range (traced guides do not extrapolate).
    [[nodiscard]] double Clamp(double Value, double Low, double High, double Period) noexcept
    {
        if (Period > 0.0 || !(High > Low)) return Value;
        return std::clamp(Value, Low, High);
    }

    // Foot of Position on an analytic support (closed-form inversion), exact.
    [[nodiscard]] Vector3 FootOnSupport(const SurfaceGeometry& Support, Vector3 Position) noexcept
    {
        return EvaluateSurface(Support, ProjectOntoSurface(Support, Position, { 0.0, 0.0 })).Position;
    }

    // Spine point, unit tangent and the cross-section frame E₁ (towards the guide foot), E₂ = E₁ × T.
    struct SectionFrame
    {
        Vector3 Centre  = {};                                                                // [len] - C(𝑢)
        Vector3 Tangent = {};                                                                // [-]   - T(𝑢)
        Vector3 First   = {};                                                                // [-]   - E₁(𝑢)
        Vector3 Second  = {};                                                                // [-]   - E₂(𝑢)
    };

    [[nodiscard]] SectionFrame ResolveSection(const RollingBallDefinition& Definition, double 𝑢) noexcept
    {
        SectionFrame Frame;
        const CurveSample Sample = EvaluateCurve(Definition.Spine, Clamp(𝑢, Definition.SpineLow, Definition.SpineHigh, Definition.Period));
        Frame.Centre  = Sample.Position;
        Frame.Tangent = Normalize(Sample.Derivative);
        const Vector3 Towards = FootOnSupport(Definition.Guide, Frame.Centre) - Frame.Centre;  // [len] - ≈ r·E₁
        Frame.First  = Normalize(Towards - Frame.Tangent * Dot(Towards, Frame.Tangent));
        Frame.Second = Cross(Frame.First, Frame.Tangent);
        return Frame;
    }

    [[nodiscard]] Vector3 EvaluatePosition(const RollingBallDefinition& Definition, Vector2 Parameter) noexcept
    {
        const SectionFrame Frame = ResolveSection(Definition, Parameter.X);
        return Frame.Centre + (Frame.First * std::cos(Parameter.Y) + Frame.Second * std::sin(Parameter.Y)) * Definition.Radius;
    }

    // Step for central differences in τ: 10⁻⁶ of the range (or of 2π for unbounded carriers).
    [[nodiscard]] double DifferenceStep(double Low, double High, double Period) noexcept
    {
        const double Span = Period > 0.0 ? Period : (std::isfinite(High - Low) && High > Low ? High - Low : FullTurn);
        return 1.0e-6 * Span;
    }

    [[nodiscard]] double SpineParameter(const ContactDefinition& Definition, double τ) noexcept { return Definition.Direction * τ; }
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    PUBLIC FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

CarrierRange MeasureCarrierRange(const CurveGeometry& Curve) noexcept
{
    if (std::holds_alternative<StraightLine>(Curve)) return { -std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity(), 0.0 };
    if (const auto* Traced = std::get_if<IntersectionCurve>(&Curve))
    {
        const auto [Low, High] = MeasureIntersectionCurveRange(*Traced->Trace);
        return { Low, High, IntersectionCurveClosedCondition(*Traced->Trace) ? High - Low : 0.0 };
    }
    if (const auto* Contact = std::get_if<ContactCurve>(&Curve))
    {
        const ContactDefinition& Definition = *Contact->Definition;
        return { Definition.SpineLow, Definition.SpineHigh, Definition.Period };
    }
    return { 0.0, FullTurn, FullTurn };
}

// ---- Carrier ----------------------------------------------------------------------------------------------------------

RollingBallSurface ConstructRollingBallSurface(const CurveGeometry& Spine, double Radius, const SurfaceGeometry& Guide)
{
    auto Definition = std::make_shared<RollingBallDefinition>();
    const CarrierRange Range = MeasureCarrierRange(Spine);
    Definition->Spine     = Spine;
    Definition->Radius    = Radius;
    Definition->Guide     = Guide;
    Definition->SpineLow  = Range.Low;
    Definition->SpineHigh = Range.High;
    Definition->Period    = Range.Period;
    return RollingBallSurface{ std::move(Definition) };
}

SurfaceSample EvaluateRollingBall(const RollingBallSurface& Surface, Vector2 Parameter) noexcept
{
    const RollingBallDefinition& Definition = *Surface.Definition;
    const SectionFrame Frame  = ResolveSection(Definition, Parameter.X);
    const double       Cosine = std::cos(Parameter.Y), Sine = std::sin(Parameter.Y);           // [-] - cos 𝑣, sin 𝑣
    SurfaceSample Sample;
    Sample.Position    = Frame.Centre + (Frame.First * Cosine + Frame.Second * Sine) * Definition.Radius;
    Sample.DerivativeV = (Frame.Second * Cosine - Frame.First * Sine) * Definition.Radius;
    // 📝 ∂P/∂𝑢 by central difference (E₁′ would need the guide's curvature); h = 10⁻⁶·range, truncation O(h²).
    const double Step = DifferenceStep(Definition.SpineLow, Definition.SpineHigh, Definition.Period);
    const double Low  = Clamp(Parameter.X - Step, Definition.SpineLow, Definition.SpineHigh, Definition.Period);
    const double High = Clamp(Parameter.X + Step, Definition.SpineLow, Definition.SpineHigh, Definition.Period);
    Sample.DerivativeU = (EvaluatePosition(Definition, { High, Parameter.Y }) - EvaluatePosition(Definition, { Low, Parameter.Y })) * (1.0 / (High - Low));
    return Sample;
}

Vector3 EvaluateRollingBallNormal(const RollingBallSurface& Surface, Vector2 Parameter) noexcept
{
    const SectionFrame Frame = ResolveSection(*Surface.Definition, Parameter.X);
    return Frame.First * std::cos(Parameter.Y) + Frame.Second * std::sin(Parameter.Y);      // Exact: away from the spine
}

Vector2 ProjectOntoRollingBall(const RollingBallSurface& Surface, Vector3 Position, Vector2 Reference) noexcept
{
    const RollingBallDefinition& Definition = *Surface.Definition;
    // 📝 For a carrier point C(𝑢) + r·e with e ⟂ T, C(𝑢) is the nearest spine point (r below the spine's curvature
    //    radius), so the spine inversion gives 𝑢 exactly; 𝑣 is then the polar angle in (E₁, E₂).
    const double SpineLow = Definition.Period > 0.0 ? Reference.X - 0.5 * Definition.Period : (std::isfinite(Definition.SpineLow) ? Definition.SpineLow : 0.0);
    double       𝑢        = ProjectOntoCurve(Definition.Spine, Position, SpineLow);
    𝑢 = Fold(Clamp(𝑢, Definition.SpineLow, Definition.SpineHigh, Definition.Period), Reference.X - 0.5 * Definition.Period, Definition.Period);
    const SectionFrame Frame  = ResolveSection(Definition, 𝑢);
    const Vector3      Offset = Position - Frame.Centre;                                     // [len] - From the ball centre
    const double       𝑣      = std::atan2(Dot(Offset, Frame.Second), Dot(Offset, Frame.First));
    return { 𝑢, Fold(𝑣, Reference.Y - std::numbers::pi, FullTurn) };
}

SurfacePeriodicity ResolveRollingBallPeriodicity(const RollingBallSurface& Surface) noexcept
{
    return { Surface.Definition->Period, FullTurn };
}

std::optional<SurfaceGeometry> OffsetRollingBall(const RollingBallSurface& Surface, double Distance)
{
    const RollingBallDefinition& Definition = *Surface.Definition;
    if (!(Definition.Radius + Distance > 0.0)) return std::nullopt;
    auto Offset = std::make_shared<RollingBallDefinition>(Definition);
    Offset->Radius = Definition.Radius + Distance;
    return SurfaceGeometry{ RollingBallSurface{ std::move(Offset) } };
}

double MeasureRollingBallCurvature(const RollingBallSurface& Surface, double 𝑢) noexcept
{
    const RollingBallDefinition& Definition = *Surface.Definition;
    const double Step = 1.0e3 * DifferenceStep(Definition.SpineLow, Definition.SpineHigh, Definition.Period);   // [-] - 10⁻³·range
    const double Low  = Clamp(𝑢 - Step, Definition.SpineLow, Definition.SpineHigh, Definition.Period);
    const double High = Clamp(𝑢 + Step, Definition.SpineLow, Definition.SpineHigh, Definition.Period);
    const CurveSample Middle = EvaluateCurve(Definition.Spine, Clamp(𝑢, Definition.SpineLow, Definition.SpineHigh, Definition.Period));
    const Vector3 Turn  = Normalize(EvaluateCurve(Definition.Spine, High).Derivative) - Normalize(EvaluateCurve(Definition.Spine, Low).Derivative);
    const double  Speed = Length(Middle.Derivative) * (High - Low);                         // [len] - Arc length of the stencil
    const double  κ     = Speed > 0.0 ? Length(Turn) / Speed : 0.0;                          // [1/len] - Spine curvature
    const double  Along = 1.0 - Definition.Radius * κ;                                       // [-] - Inner-side stretch
    return std::max(1.0 / Definition.Radius, Along > 1.0e-12 ? κ / Along : INFINITY);
}

CircularArc ExtractRollingBallSection(const RollingBallSurface& Surface, double 𝑢) noexcept
{
    const SectionFrame Frame = ResolveSection(*Surface.Definition, 𝑢);
    return CircularArc{ OrthonormalFrame{ Frame.Centre, Frame.First, Frame.Second, Cross(Frame.First, Frame.Second) }, Surface.Definition->Radius };
}

RollingBallDistance MeasureRollingBallDistance(const RollingBallSurface& Surface, Vector3 Position) noexcept
{
    const RollingBallDefinition& Definition = *Surface.Definition;
    const double  SpineLow = Definition.Period > 0.0 ? 0.0 : (std::isfinite(Definition.SpineLow) ? Definition.SpineLow : 0.0);
    const double  𝑢        = Clamp(ProjectOntoCurve(Definition.Spine, Position, SpineLow), Definition.SpineLow, Definition.SpineHigh, Definition.Period);
    const Vector3 Centre   = EvaluateCurve(Definition.Spine, 𝑢).Position;                    // [len] - Nearest ball centre
    const Vector3 Offset   = Position - Centre;
    const double  Reach    = Length(Offset);                                                 // [len] - Distance to the spine
    RollingBallDistance Outcome;
    Outcome.Normal   = Reach > 0.0 ? Offset * (1.0 / Reach) : ResolveSection(Definition, 𝑢).First;
    Outcome.Distance = Reach - Definition.Radius;
    Outcome.Foot     = Centre + Outcome.Normal * Definition.Radius;
    return Outcome;
}

// ---- Contact rails ----------------------------------------------------------------------------------------------------

ContactCurve ConstructContactCurve(const CurveGeometry& Spine, const SurfaceGeometry& Support)
{
    auto Definition = std::make_shared<ContactDefinition>();
    const CarrierRange Range = MeasureCarrierRange(Spine);
    Definition->Spine     = Spine;
    Definition->Support   = Support;
    Definition->Direction = 1.0;
    Definition->SpineLow  = Range.Low;
    Definition->SpineHigh = Range.High;
    Definition->Period    = Range.Period;
    return ContactCurve{ std::move(Definition) };
}

CurveSample EvaluateContactCurve(const ContactDefinition& Definition, double τ) noexcept
{
    const auto Foot = [&](double Value)
    {
        const double Inside = Clamp(Value, Definition.SpineLow, Definition.SpineHigh, Definition.Period);
        return FootOnSupport(Definition.Support, EvaluateCurve(Definition.Spine, SpineParameter(Definition, Inside)).Position);
    };
    const double Step = DifferenceStep(Definition.SpineLow, Definition.SpineHigh, Definition.Period);
    const double Low  = Clamp(τ - Step, Definition.SpineLow, Definition.SpineHigh, Definition.Period);
    const double High = Clamp(τ + Step, Definition.SpineLow, Definition.SpineHigh, Definition.Period);
    return { Foot(τ), (Foot(High) - Foot(Low)) * (1.0 / (High - Low)) };
}

double ProjectOntoContactCurve(const ContactDefinition& Definition, Vector3 Position, double IntervalLow) noexcept
{
    // 📝 The rail point F(τ) = C(τ) + r·e with e ⟂ T has C(τ) as its nearest spine point, so the spine inversion is exact
    //    on the rail; off the rail, Gauss–Newton on (F(τ) − X)·F′(τ) = 0 polishes it.
    const double SpineFold = Definition.Period > 0.0 ? 0.0 : (Definition.Direction > 0.0 ? Definition.SpineLow : -Definition.SpineHigh);
    double τ = Definition.Direction * ProjectOntoCurve(Definition.Spine, Position, std::isfinite(SpineFold) ? SpineFold : 0.0);
    for (int Iteration = 0; Iteration < 12; ++Iteration)
    {
        const CurveSample Sample = EvaluateContactCurve(Definition, τ);
        const double      Slope  = Dot(Sample.Derivative, Sample.Derivative);
        if (!(Slope > 0.0)) break;
        const double Step = Dot(Sample.Position - Position, Sample.Derivative) / Slope;
        τ = Clamp(τ - Step, Definition.SpineLow, Definition.SpineHigh, Definition.Period);
        if (std::fabs(Step) <= 1.0e-15 * (1.0 + std::fabs(τ))) break;
    }
    return Fold(τ, IntervalLow, Definition.Period);
}

CurveGeometry ReverseContactCurve(const ContactDefinition& Definition)
{
    auto Reversed = std::make_shared<ContactDefinition>(Definition);
    Reversed->Direction = -Definition.Direction;
    Reversed->SpineLow  = -Definition.SpineHigh;
    Reversed->SpineHigh = -Definition.SpineLow;
    return ContactCurve{ std::move(Reversed) };
}

double MeasureContactCurveBulge(const ContactDefinition& Definition, double Low, double High, int Pieces) noexcept
{
    // 📝 Largest midpoint-to-chord distance over the pieces, refined 4× inside each piece (sampled sagitta).
    double Largest = 0.0;                                                                    // [len]
    const int Count = std::max(Pieces, 1);
    for (int Piece = 0; Piece < Count; ++Piece)
    {
        const double  Start  = Low + (High - Low) * Piece / Count;
        const double  Finish = Low + (High - Low) * (Piece + 1) / Count;
        const Vector3 From   = EvaluateContactCurve(Definition, Start).Position;
        const Vector3 To     = EvaluateContactCurve(Definition, Finish).Position;
        for (int Probe = 1; Probe < 4; ++Probe)
        {
            const double  Fraction = Probe / 4.0;
            const Vector3 Point    = EvaluateContactCurve(Definition, Start + (Finish - Start) * Fraction).Position;
            Largest = std::max(Largest, Length(Point - (From + (To - From) * Fraction)));
        }
    }
    return Largest;
}

std::vector<double> CollectContactCurveBreakpoints(const ContactDefinition& Definition, double Low, double High)
{
    std::vector<double> Breakpoints;
    for (double Knot : CollectCurveBreakpoints(Definition.Spine, std::min(Definition.Direction * Low, Definition.Direction * High), std::max(Definition.Direction * Low, Definition.Direction * High)))
        Breakpoints.push_back(Definition.Direction * Knot);
    std::sort(Breakpoints.begin(), Breakpoints.end());
    return Breakpoints;
}

} // namespace Frontier::SolidArcRemastered
