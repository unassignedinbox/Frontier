/*====================================================================================================================================
                                                     ROLLINGBALLCARRIER.CPP
====================================================================================================================================*/
// 🧩 Procedural rolling-ball carrier and its contact rails, evaluated from an exact spine (traced, elliptical or any curve)

//------------------------------------------------------------------------------------------------------------------------
//                                                    INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Geometry/RollingBallCarrier.h"
#include "Geometry/NurbsCarrier.h"

#include "Intersection/ImplicitSurface.h"
#include "Intersection/IntersectionCurve.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>
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
        Vector3 Velocity = {};                                                               // [len] - C′(𝑢)
    };

    // 📝 Section memo. Quadratures sweep 𝑣 at a fixed 𝑢 (and the ∂P/∂𝑢 difference revisits 𝑢 ± h), so the same section
    //    is resolved many times; on a traced spine each one is a Newton solve onto both supports. The frame depends only on
    //    Spine, Guide and range — copies sharing a Serial may differ only in Radius — and E₂ is rebuilt from Handedness on
    //    every call. A hit returns the bit-identical frame, so results are unchanged.
    struct SectionMemo
    {
        std::uint64_t Serial  = 0;                                                           // [-] - Definition identity (0 = empty)
        double        𝑢       = 0.0;                                                         // [-] - Exact key
        Vector3       Centre  = {};                                                          // [len]
        Vector3       Tangent = {};                                                          // [-]
        Vector3       First   = {};                                                          // [-]
        Vector3       Velocity = {};                                                         // [len]
    };

    constexpr std::size_t SectionMemoSlots = 256;                                            // [-] - Direct-mapped, per thread

    [[nodiscard]] std::size_t SectionMemoSlot(std::uint64_t Serial, double 𝑢) noexcept
    {
        std::uint64_t Bits = 0;
        std::memcpy(&Bits, &𝑢, sizeof Bits);
        std::uint64_t Mixed = (Bits ^ (Serial * 0x9E3779B97F4A7C15ull)) * 0xBF58476D1CE4E5B9ull;
        Mixed ^= Mixed >> 31;
        return static_cast<std::size_t>(Mixed % SectionMemoSlots);
    }

    std::atomic<std::uint64_t> NextDefinitionSerial{ 1 };                                    // [-] - Monotonic, never reused

    [[nodiscard]] SectionFrame ResolveSection(const RollingBallDefinition& Definition, double 𝑢) noexcept
    {
        thread_local std::array<SectionMemo, SectionMemoSlots> Memo = {};
        SectionMemo* Slot = nullptr;
        if (Definition.Serial != 0)
        {
            Slot = &Memo[SectionMemoSlot(Definition.Serial, 𝑢)];
            if (Slot->Serial == Definition.Serial && Slot->𝑢 == 𝑢)
                return { Slot->Centre, Slot->Tangent, Slot->First, Cross(Slot->First, Slot->Tangent) * Definition.Handedness, Slot->Velocity };
        }
        SectionFrame Frame;
        const CurveSample Sample = EvaluateCurve(Definition.Spine, Clamp(𝑢, Definition.SpineLow, Definition.SpineHigh, Definition.Period));
        Frame.Centre   = Sample.Position;
        Frame.Velocity = Sample.Derivative;
        Frame.Tangent  = Normalize(Sample.Derivative);
        const Vector3 Towards = FootOnSupport(Definition.Guide, Frame.Centre) - Frame.Centre;  // [len] - ≈ r·E₁
        Frame.First  = Normalize(Towards - Frame.Tangent * Dot(Towards, Frame.Tangent));
        Frame.Second = Cross(Frame.First, Frame.Tangent) * Definition.Handedness;
        if (Slot) *Slot = { Definition.Serial, 𝑢, Frame.Centre, Frame.Tangent, Frame.First, Frame.Velocity };
        return Frame;
    }

    // 📝 Section frame as a field of space for a traced spine C = S₁ ∩ S₂: T(X) = ±(N₁ × N₂)/|N₁ × N₂| from the two
    //    support normals at X (signed like Reference), E₁, E₂ from the guide foot exactly as ResolveSection does. On the
    //    spine it equals the section frame, so d/d𝑢 of the frame is its derivative along C′ — taken on the tangent line
    //    C ± h·C′, which needs no Newton solve onto the curve. The symmetric stencil cancels the O(h²) offset of the
    //    tangent line from the curve, so the quotient keeps the O(h²) truncation of the exact central difference.
    struct FieldFrame
    {
        Vector3 First  = {};                                                                 // [-] - E₁(X)
        Vector3 Second = {};                                                                 // [-] - E₂(X)
    };

    [[nodiscard]] FieldFrame ResolveFieldFrame(const RollingBallDefinition& Definition, const TracedIntersection& Trace, Vector3 Position, Vector3 Reference) noexcept
    {
        Vector3 Tangent = Normalize(Cross(MeasureSurfaceDistance(Trace.First, Position).Normal, MeasureSurfaceDistance(Trace.Second, Position).Normal));
        if (Dot(Tangent, Reference) < 0.0) Tangent = -Tangent;
        const Vector3 Towards = FootOnSupport(Definition.Guide, Position) - Position;      // [len] - ≈ r·E₁
        FieldFrame Frame;
        Frame.First  = Normalize(Towards - Tangent * Dot(Towards, Tangent));
        Frame.Second = Cross(Frame.First, Tangent) * Definition.Handedness;
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

    // ---- Chord section --------------------------------------------------------------------------------------------

    [[nodiscard]] bool ChordCondition(const RollingBallDefinition& Definition) noexcept { return Definition.Section == RollingBallSection::Chord; }

    // 📝 P = A + 𝑣(B − A) with A, B the feet of C(𝑢); ∂P/∂𝑢 from the foot fields on the tangent line C ± h·C′ (the
    //    symmetric stencil cancels the line's O(h²) offset from the spine, as for the traced frame field).
    [[nodiscard]] SurfaceSample EvaluateChord(const RollingBallDefinition& Definition, Vector2 Parameter) noexcept
    {
        const SectionFrame Frame = ResolveSection(Definition, Parameter.X);
        const Vector3 Near = FootOnSupport(Definition.Guide, Frame.Centre);                  // [len] - A
        const Vector3 Far  = FootOnSupport(Definition.Partner, Frame.Centre);                // [len] - B
        const double  Step = DifferenceStep(Definition.SpineLow, Definition.SpineHigh, Definition.Period);
        const Vector3 Reach = Frame.Velocity * Step;                                         // [len] - h·C′
        const Vector3 NearTurn = FootOnSupport(Definition.Guide, Frame.Centre + Reach) - FootOnSupport(Definition.Guide, Frame.Centre - Reach);
        const Vector3 FarTurn  = FootOnSupport(Definition.Partner, Frame.Centre + Reach) - FootOnSupport(Definition.Partner, Frame.Centre - Reach);
        SurfaceSample Sample;
        Sample.Position    = Near + (Far - Near) * Parameter.Y;
        Sample.DerivativeV = Far - Near;
        Sample.DerivativeU = (NearTurn * (1.0 - Parameter.Y) + FarTurn * Parameter.Y) * (1.0 / (2.0 * Step));
        return Sample;
    }

    // 📝 Spine inversion seeds 𝑢 (exact when r′ = 0), the chord fraction seeds 𝑣; Gauss–Newton on |P(𝑢, 𝑣) − X|² polishes.
    [[nodiscard]] Vector2 ProjectOntoChord(const RollingBallDefinition& Definition, Vector3 Position, Vector2 Reference) noexcept
    {
        const double SpineLow = Definition.Period > 0.0 ? Reference.X - 0.5 * Definition.Period : (std::isfinite(Definition.SpineLow) ? Definition.SpineLow : 0.0);
        double 𝑢 = Clamp(ProjectOntoCurve(Definition.Spine, Position, SpineLow), Definition.SpineLow, Definition.SpineHigh, Definition.Period);
        double 𝑣 = 0.5;
        {
            const SectionFrame Frame = ResolveSection(Definition, 𝑢);
            const Vector3 Near = FootOnSupport(Definition.Guide, Frame.Centre), Far = FootOnSupport(Definition.Partner, Frame.Centre);
            const double  Square = Dot(Far - Near, Far - Near);                              // [len²] - |B − A|²
            if (Square > 0.0) 𝑣 = Dot(Position - Near, Far - Near) / Square;
        }
        for (int Iteration = 0; Iteration < 30; ++Iteration)
        {
            const SurfaceSample Sample = EvaluateChord(Definition, { 𝑢, 𝑣 });
            const Vector3 Offset = Sample.Position - Position;                               // [len] - P − X
            const double  UU = Dot(Sample.DerivativeU, Sample.DerivativeU), UV = Dot(Sample.DerivativeU, Sample.DerivativeV), VV = Dot(Sample.DerivativeV, Sample.DerivativeV);
            const double  GU = Dot(Offset, Sample.DerivativeU), GV = Dot(Offset, Sample.DerivativeV);
            const double  Determinant = UU * VV - UV * UV;                                   // [len⁴]
            if (!(Determinant > 1.0e-300)) break;
            const double  StepU = (VV * GU - UV * GV) / Determinant, StepV = (UU * GV - UV * GU) / Determinant;
            𝑢 = Clamp(𝑢 - StepU, Definition.SpineLow, Definition.SpineHigh, Definition.Period);
            𝑣 -= StepV;
            if (std::fabs(StepU) <= 1.0e-15 * (1.0 + std::fabs(𝑢)) && std::fabs(StepV) <= 1.0e-15) break;
        }
        return { Fold(𝑢, Reference.X - 0.5 * Definition.Period, Definition.Period), 𝑣 };
    }
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
    if (const auto* Spline = std::get_if<NurbsCurve>(&Curve))
    {
        const NurbsDomain Domain = MeasureNurbsCurveDomain(*Spline->Definition);
        return { Domain.Low, Domain.High, Spline->Definition->Period };
    }
    return { 0.0, FullTurn, FullTurn };
}

// ---- Carrier ----------------------------------------------------------------------------------------------------------

RollingBallSurface ConstructRollingBallSurface(const CurveGeometry& Spine, double Radius, const SurfaceGeometry& Guide, double Handedness, RollingBallSection Section, const SurfaceGeometry& Partner)
{
    auto Definition = std::make_shared<RollingBallDefinition>();
    const CarrierRange Range = MeasureCarrierRange(Spine);
    Definition->Spine     = Spine;
    Definition->Radius    = Radius;
    Definition->Guide     = Guide;
    Definition->SpineLow  = Range.Low;
    Definition->SpineHigh = Range.High;
    Definition->Period    = Range.Period;
    Definition->Handedness = Handedness < 0.0 ? -1.0 : 1.0;
    Definition->Serial    = NextDefinitionSerial.fetch_add(1, std::memory_order_relaxed);
    Definition->Section   = Section;
    Definition->Partner   = Partner;
    return RollingBallSurface{ std::move(Definition) };
}

SurfaceSample EvaluateRollingBall(const RollingBallSurface& Surface, Vector2 Parameter) noexcept
{
    const RollingBallDefinition& Definition = *Surface.Definition;
    if (ChordCondition(Definition)) return EvaluateChord(Definition, Parameter);
    const SectionFrame Frame  = ResolveSection(Definition, Parameter.X);
    const double       Cosine = std::cos(Parameter.Y), Sine = std::sin(Parameter.Y);           // [-] - cos 𝑣, sin 𝑣
    SurfaceSample Sample;
    Sample.Position    = Frame.Centre + (Frame.First * Cosine + Frame.Second * Sine) * Definition.Radius;
    Sample.DerivativeV = (Frame.Second * Cosine - Frame.First * Sine) * Definition.Radius;
    // 📝 ∂P/∂𝑢 by central difference (E₁′ would need the guide's curvature); h = 10⁻⁶·range, truncation O(h²).
    const double Step = DifferenceStep(Definition.SpineLow, Definition.SpineHigh, Definition.Period);
    if (const auto* Traced = std::get_if<IntersectionCurve>(&Definition.Spine); Traced && Traced->Trace)
    {
        // 💡 Traced spine: C′ is exact and E₁′, E₂′ come from the frame field on the tangent line (one Newton solve per
        //    sample instead of three).
        const Vector3    Reach = Frame.Velocity * Step;                                      // [len] - h·C′
        const FieldFrame Ahead = ResolveFieldFrame(Definition, *Traced->Trace, Frame.Centre + Reach, Frame.Tangent);
        const FieldFrame Back  = ResolveFieldFrame(Definition, *Traced->Trace, Frame.Centre - Reach, Frame.Tangent);
        const Vector3    Turn  = ((Ahead.First - Back.First) * Cosine + (Ahead.Second - Back.Second) * Sine) * (1.0 / (2.0 * Step));
        Sample.DerivativeU = Frame.Velocity + Turn * Definition.Radius;
        return Sample;
    }
    const double Low  = Clamp(Parameter.X - Step, Definition.SpineLow, Definition.SpineHigh, Definition.Period);
    const double High = Clamp(Parameter.X + Step, Definition.SpineLow, Definition.SpineHigh, Definition.Period);
    Sample.DerivativeU = (EvaluatePosition(Definition, { High, Parameter.Y }) - EvaluatePosition(Definition, { Low, Parameter.Y })) * (1.0 / (High - Low));
    return Sample;
}

Vector3 EvaluateRollingBallNormal(const RollingBallSurface& Surface, Vector2 Parameter) noexcept
{
    if (ChordCondition(*Surface.Definition))
    {
        const SurfaceSample Sample = EvaluateChord(*Surface.Definition, Parameter);
        return Normalize(Cross(Sample.DerivativeU, Sample.DerivativeV)) * Surface.Definition->Handedness;
    }
    const SectionFrame Frame = ResolveSection(*Surface.Definition, Parameter.X);
    // Exact radial direction; ∂𝑢 × ∂𝑣 points away from the spine for handedness +1, towards it for the mirror image.
    return (Frame.First * std::cos(Parameter.Y) + Frame.Second * std::sin(Parameter.Y)) * Surface.Definition->Handedness;
}

Vector2 ProjectOntoRollingBall(const RollingBallSurface& Surface, Vector3 Position, Vector2 Reference) noexcept
{
    const RollingBallDefinition& Definition = *Surface.Definition;
    if (ChordCondition(Definition)) return ProjectOntoChord(Definition, Position, Reference);
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
    return { Surface.Definition->Period, ChordCondition(*Surface.Definition) ? 0.0 : FullTurn };
}

std::optional<SurfaceGeometry> OffsetRollingBall(const RollingBallSurface& Surface, double Distance)
{
    const RollingBallDefinition& Definition = *Surface.Definition;
    if (ChordCondition(Definition)) return std::nullopt;                                     // Offset of a ruled chord: not this carrier
    const double Radius = Definition.Radius + Definition.Handedness * Distance;              // [len] - Along the natural normal
    if (!(Radius > 0.0)) return std::nullopt;
    auto Offset = std::make_shared<RollingBallDefinition>(Definition);
    Offset->Radius = Radius;
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
    if (ChordCondition(Definition)) return Along > 1.0e-12 ? κ / Along : INFINITY;           // Straight rulings: only the rails bend
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
    if (ChordCondition(Definition))
    {
        const Vector2 Parameter = ProjectOntoChord(Definition, Position, { std::isfinite(Definition.SpineLow) ? Definition.SpineLow + 0.5 * Definition.Period : 0.0, 0.5 });
        const SurfaceSample Sample = EvaluateChord(Definition, Parameter);
        RollingBallDistance Outcome;
        Outcome.Normal   = Normalize(Cross(Sample.DerivativeU, Sample.DerivativeV)) * Definition.Handedness;
        Outcome.Distance = Dot(Position - Sample.Position, Outcome.Normal);
        Outcome.Foot     = Sample.Position;
        return Outcome;
    }
    const double  SpineLow = Definition.Period > 0.0 ? 0.0 : (std::isfinite(Definition.SpineLow) ? Definition.SpineLow : 0.0);
    const double  𝑢        = Clamp(ProjectOntoCurve(Definition.Spine, Position, SpineLow), Definition.SpineLow, Definition.SpineHigh, Definition.Period);
    const Vector3 Centre   = EvaluateCurve(Definition.Spine, 𝑢).Position;                    // [len] - Nearest ball centre
    const Vector3 Offset   = Position - Centre;
    const double  Reach    = Length(Offset);                                                 // [len] - Distance to the spine
    RollingBallDistance Outcome;
    const Vector3 Radial = Reach > 0.0 ? Offset * (1.0 / Reach) : ResolveSection(Definition, 𝑢).First;
    Outcome.Normal   = Radial * Definition.Handedness;                                       // Signed like the natural normal
    Outcome.Distance = (Reach - Definition.Radius) * Definition.Handedness;
    Outcome.Foot     = Centre + Radial * Definition.Radius;
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
