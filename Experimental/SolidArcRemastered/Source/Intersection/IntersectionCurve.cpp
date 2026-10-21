/*====================================================================================================================================
                                                     INTERSECTIONCURVE.CPP
====================================================================================================================================*/
// 🧩 Exact intersection curves: Hermite guide, normal-plane Newton onto both surfaces, exact derivative of the map

//------------------------------------------------------------------------------------------------------------------------
//                                                    INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Intersection/IntersectionCurve.h"

#include "Intersection/ImplicitSurface.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

namespace Frontier::SolidArcRemastered
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    HELPERS
//------------------------------------------------------------------------------------------------------------------------

namespace
{
    constexpr double FullTurn     = 2.0 * std::numbers::pi;                                  // [rad] - Period of closed curves
    constexpr double TurnLimit    = 0.1;                                                     // [rad] - Guide tangent turn per segment
    constexpr int    RefineLimit  = 8;                                                       // [-]   - Midpoint refinement rounds

    struct GuideSample
    {
        Vector3 Position = {};                                                               // [len] - H(τ)
        Vector3 First    = {};                                                               // [len] - H′(τ)
        Vector3 Second   = {};                                                               // [len] - H″(τ)
    };

    // 📝 3D Newton on { d₁ = ℓ₁, d₂ = ℓ₂, Π·(X − Q) = 0 }: rows N₁, N₂, Π; inverse by the triple-product formula.
    //    Normals receives N₁, N₂ at the returned Position (reused by the caller, so no surface is measured twice there).
    //    Levels ℓᵢ are 0 for an intersection and σᵢ·r for a ball-law centre (the distance rows are shifted, nothing else).
    [[nodiscard]] bool CorrectOntoBoth(const SurfaceGeometry& First, const SurfaceGeometry& Second, Vector3& Position, Vector3 PlaneNormal, Vector3 PlanePoint, double Scale, std::array<Vector3, 2>* Normals = nullptr, std::array<double, 2> Levels = { 0.0, 0.0 }) noexcept
    {
        const double Tolerance = 1.0e-13 * (1.0 + Scale);                                    // [len] - Acceptance
        SurfaceDistance OnFirst, OnSecond;                                                   // [-]   - Measured at Position when Current
        bool Current = false;                                                                // [-]   - Position unmoved since measured
        for (int Iteration = 0; Iteration < 24; ++Iteration)
        {
            OnFirst  = MeasureSurfaceDistance(First, Position);
            OnSecond = MeasureSurfaceDistance(Second, Position);
            OnFirst.Distance  -= Levels[0];
            OnSecond.Distance -= Levels[1];
            Current  = true;
            const Vector3& A = OnFirst.Normal;
            const Vector3& B = OnSecond.Normal;
            const double  PlaneResidual = Dot(PlaneNormal, Position - PlanePoint);            // [len²] - Off the plane
            // 💡 Residual test before the step: where the surfaces are tangent (N₁ ∥ N₂, e.g. two fillets meeting on a
            //    face) the system is near-singular and one more step turns 1e-16 of rounding into a 1e-8 sideways drift
            //    that d₁ and d₂ only feel quadratically. A converged point is kept as it is.
            if (std::fabs(OnFirst.Distance) <= Tolerance && std::fabs(OnSecond.Distance) <= Tolerance && std::fabs(PlaneResidual) <= Tolerance * Length(PlaneNormal)) break;
            const double Determinant = Dot(A, Cross(B, PlaneNormal));                        // [len] - a·(b×c)
            if (std::fabs(Determinant) <= 1.0e-14 * Length(PlaneNormal)) return false;
            const Vector3 Step = (Cross(B, PlaneNormal) * OnFirst.Distance + Cross(PlaneNormal, A) * OnSecond.Distance + Cross(A, B) * PlaneResidual) * (1.0 / Determinant);
            Position = Position - Step;
            Current  = false;
            if (Length(Step) <= 4.0e-16 * (1.0 + Length(Position))) break;
        }
        if (!Current)
        {
            OnFirst  = MeasureSurfaceDistance(First, Position);
            OnSecond = MeasureSurfaceDistance(Second, Position);
            OnFirst.Distance  -= Levels[0];
            OnSecond.Distance -= Levels[1];
        }
        if (Normals) *Normals = { OnFirst.Normal, OnSecond.Normal };
        return std::fabs(OnFirst.Distance) <= Tolerance && std::fabs(OnSecond.Distance) <= Tolerance;
    }

    [[nodiscard]] Vector3 CurveDirection(const SurfaceGeometry& First, const SurfaceGeometry& Second, Vector3 Position) noexcept
    {
        const Vector3 Direction = Cross(MeasureSurfaceDistance(First, Position).Normal, MeasureSurfaceDistance(Second, Position).Normal);
        const double  Magnitude = Length(Direction);                                         // [-] - |N₁ × N₂|
        return Magnitude > 1.0e-300 ? Direction * (1.0 / Magnitude) : Vector3{};
    }

    // Folds τ of a closed curve into [Knots.front(), Knots.front() + 2π).
    [[nodiscard]] double FoldParameter(const TracedIntersection& Trace, double τ) noexcept
    {
        if (!Trace.Closed) return τ;
        const double Low = Trace.Knots.front();                                              // [-] - Period start
        double Folded = Low + std::fmod(std::fmod(τ - Low, FullTurn) + FullTurn, FullTurn);
        if (Folded - Low >= FullTurn) Folded = Low;
        return Folded;
    }

    [[nodiscard]] size_t LocateSegment(const TracedIntersection& Trace, double τ) noexcept
    {
        const auto Upper = std::upper_bound(Trace.Knots.begin(), Trace.Knots.end(), τ);
        const size_t Index = Upper == Trace.Knots.begin() ? 0 : static_cast<size_t>(Upper - Trace.Knots.begin()) - 1;
        return std::min(Index, Trace.Knots.size() - 2);
    }

    // 📝 Cubic Hermite on segment k with u = (τ − τₖ)/h (u outside [0, 1] extrapolates the end cubic).
    [[nodiscard]] GuideSample EvaluateGuide(const TracedIntersection& Trace, double τ) noexcept
    {
        const size_t  Index = LocateSegment(Trace, τ);
        const double  Span  = Trace.Knots[Index + 1] - Trace.Knots[Index];                   // [-] - h
        const double  u     = (τ - Trace.Knots[Index]) / Span;                               // [-] - Local parameter
        const Vector3 P0 = Trace.Points[Index], P1 = Trace.Points[Index + 1];
        const Vector3 T0 = Trace.Tangents[Index] * Span, T1 = Trace.Tangents[Index + 1] * Span;
        const double  uu = u * u, uuu = uu * u;
        GuideSample Sample;
        Sample.Position = P0 * (2.0 * uuu - 3.0 * uu + 1.0) + T0 * (uuu - 2.0 * uu + u) + P1 * (-2.0 * uuu + 3.0 * uu) + T1 * (uuu - uu);
        Sample.First    = (P0 * (6.0 * uu - 6.0 * u) + T0 * (3.0 * uu - 4.0 * u + 1.0) + P1 * (-6.0 * uu + 6.0 * u) + T1 * (3.0 * uu - 2.0 * u)) * (1.0 / Span);
        Sample.Second   = (P0 * (12.0 * u - 6.0) + T0 * (6.0 * u - 4.0) + P1 * (-12.0 * u + 6.0) + T1 * (6.0 * u - 2.0)) * (1.0 / (Span * Span));
        return Sample;
    }

    // ---- Ball law ---------------------------------------------------------------------------------------------------

    // r = d·tan(θ/2) at the edge parameter t, θ the wedge angle between the outward normals at E(t).
    [[nodiscard]] double LawRadiusAt(const TracedIntersection& Trace, double EdgeParameter) noexcept
    {
        const BallLaw& Law    = *Trace.Law;
        const Vector3  Point  = EvaluateCurve(Law.Edge, EdgeParameter).Position;
        const Vector3  First  = MeasureSurfaceDistance(Trace.First, Point).Normal * Law.Senses[0];
        const Vector3  Second = MeasureSurfaceDistance(Trace.Second, Point).Normal * Law.Senses[1];
        const double   Wedge  = std::atan2(Length(Cross(First, Second)), -Dot(First, Second));   // [rad] - θ
        return Law.Setback * std::tan(0.5 * Wedge);
    }

    // 📝 Centre C(t) and dC/dt at the edge parameter t: Newton from E(t) (for planar supports the first step is exact),
    //    then the 3×3 derivative rows; r′ and E″ by central differences (h = 5e-6, error ~1e-10, positions unaffected).
    [[nodiscard]] bool SolveBallCentre(const TracedIntersection& Trace, double EdgeParameter, Vector3& Centre, Vector3& Derivative) noexcept
    {
        const BallLaw&    Law    = *Trace.Law;
        const CurveSample Edge   = EvaluateCurve(Law.Edge, EdgeParameter);
        const double      Radius = LawRadiusAt(Trace, EdgeParameter);                       // [len] - r(t)
        if (!(Radius > 0.0) || !std::isfinite(Radius)) return false;
        Centre = Edge.Position;
        std::array<Vector3, 2> Normals;
        if (!CorrectOntoBoth(Trace.First, Trace.Second, Centre, Edge.Derivative, Edge.Position, Trace.Perimeter, &Normals, { Law.Signs[0] * Radius, Law.Signs[1] * Radius })) return false;
        const double  Step   = 5.0e-6 * (1.0 + std::fabs(EdgeParameter));                  // [-]   - h
        const double  Rate   = (LawRadiusAt(Trace, EdgeParameter + Step) - LawRadiusAt(Trace, EdgeParameter - Step)) / (2.0 * Step);
        const Vector3 Bend   = (EvaluateCurve(Law.Edge, EdgeParameter + Step).Derivative - EvaluateCurve(Law.Edge, EdgeParameter - Step).Derivative) * (0.5 / Step);
        const Vector3 A = Normals[0], B = Normals[1], C = Edge.Derivative;
        const double  Determinant = Dot(A, Cross(B, C));                                     // [len] - a·(b×c)
        if (std::fabs(Determinant) <= 1.0e-14 * Length(C)) return false;
        const double  Third = Dot(C, C) - Dot(Bend, Centre - Edge.Position);                 // [len²] - E′·C′
        Derivative = (Cross(B, C) * (Law.Signs[0] * Rate) + Cross(C, A) * (Law.Signs[1] * Rate) + Cross(A, B) * Third) * (1.0 / Determinant);
        return true;
    }

    [[nodiscard]] double SegmentDistance(Vector3 Position, Vector3 Start, Vector3 End, double& Fraction) noexcept
    {
        const Vector3 Along  = End - Start;                                                  // [len] - Chord
        const double  Square = Dot(Along, Along);                                            // [len²]
        Fraction = Square > 0.0 ? std::clamp(Dot(Position - Start, Along) / Square, 0.0, 1.0) : 0.0;
        return Distance(Position, Start + Along * Fraction);
    }
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    CONSTRUCTION
//------------------------------------------------------------------------------------------------------------------------

CurveGeometry BuildIntersectionCurve(const SurfaceGeometry& First, const SurfaceGeometry& Second, const std::vector<Vector3>& Points, bool Closed)
{
    auto Trace = std::make_shared<TracedIntersection>();
    Trace->First  = First;
    Trace->Second = Second;
    Trace->Closed = Closed;

    double Scale = 0.0;                                                                      // [len] - Extent of the chart
    for (const Vector3& Point : Points) Scale = std::max(Scale, Distance(Point, Points.front()));
    std::vector<Vector3> Guide;
    for (const Vector3& Point : Points)
    {
        if (Guide.empty() || Distance(Guide.back(), Point) > 1.0e-12 * (1.0 + Scale)) Guide.push_back(Point);
    }
    if (Closed && Guide.size() > 2 && Distance(Guide.back(), Guide.front()) <= 1.0e-12 * (1.0 + Scale)) Guide.pop_back();

    // ---- Refine: corrected midpoints wherever the curve turns by more than TurnLimit across a segment.
    for (int Round = 0; Round < RefineLimit; ++Round)
    {
        bool Inserted = false;
        std::vector<Vector3> Refined;
        const size_t Segments = Closed ? Guide.size() : Guide.size() - 1;
        for (size_t Index = 0; Index < Guide.size(); ++Index)
        {
            Refined.push_back(Guide[Index]);
            if (Index >= Segments) continue;
            const Vector3 Start = Guide[Index], End = Guide[(Index + 1) % Guide.size()];
            const Vector3 StartDirection = CurveDirection(First, Second, Start), EndDirection = CurveDirection(First, Second, End);
            const double  Turn = std::atan2(Length(Cross(StartDirection, EndDirection)), std::fabs(Dot(StartDirection, EndDirection)));
            if (Turn <= TurnLimit) continue;
            Vector3 Middle = (Start + End) * 0.5;
            if (CorrectOntoBoth(First, Second, Middle, End - Start, Middle, Scale) && Distance(Middle, Start) < Distance(Start, End) && Distance(Middle, End) < Distance(Start, End))
            {
                Refined.push_back(Middle);
                Inserted = true;
            }
        }
        Guide.swap(Refined);
        if (!Inserted) break;
    }
    if (Closed) Guide.push_back(Guide.front());

    // ---- Knots by chord length; tangents N₁ × N₂ oriented along the chart, scaled by ds/dτ.
    std::vector<double> Chord(Guide.size(), 0.0);
    for (size_t Index = 1; Index < Guide.size(); ++Index) Chord[Index] = Chord[Index - 1] + Distance(Guide[Index - 1], Guide[Index]);
    Trace->Perimeter = Chord.back();
    const double Speed = Closed ? Trace->Perimeter / FullTurn : 1.0;                        // [len] - ds/dτ
    for (size_t Index = 0; Index < Guide.size(); ++Index)
    {
        const size_t  Before = Index > 0 ? Index - 1 : (Closed ? Guide.size() - 2 : 0);
        const size_t  After  = Index + 1 < Guide.size() ? Index + 1 : (Closed ? 1 : Index);
        Vector3 Direction = CurveDirection(First, Second, Guide[Index]);
        const Vector3 Along = Guide[After] - Guide[Before];                                  // [len] - Chart direction
        if (Length(Direction) == 0.0) Direction = Normalize(Along);
        if (Dot(Direction, Along) < 0.0) Direction = -Direction;
        Trace->Points.push_back(Guide[Index]);
        Trace->Tangents.push_back(Direction * Speed);
        Trace->Knots.push_back(Chord[Index] / Speed);
    }
    return IntersectionCurve{ std::move(Trace) };
}

std::pair<double, double> MeasureIntersectionCurveRange(const TracedIntersection& Trace) noexcept
{
    return { Trace.Knots.front(), Trace.Knots.back() };
}

CurveGeometry ReverseIntersectionCurve(const TracedIntersection& Trace)
{
    auto Reversed = std::make_shared<TracedIntersection>(Trace);
    if (Trace.Law)
    {
        auto Law = std::make_shared<BallLaw>(*Trace.Law);
        Law->Direction = -Law->Direction;
        Reversed->Law  = std::move(Law);
    }
    std::reverse(Reversed->Points.begin(), Reversed->Points.end());
    std::reverse(Reversed->Tangents.begin(), Reversed->Tangents.end());
    for (Vector3& Tangent : Reversed->Tangents) Tangent = -Tangent;
    for (size_t Index = 0; Index < Trace.Knots.size(); ++Index) Reversed->Knots[Index] = -Trace.Knots[Trace.Knots.size() - 1 - Index];
    return IntersectionCurve{ std::move(Reversed) };
}

std::optional<CurveGeometry> BuildBallLawCurve(const SurfaceGeometry& First, const SurfaceGeometry& Second, const BallLaw& Law, double Low, double High, bool Closed)
{
    auto Trace = std::make_shared<TracedIntersection>();
    Trace->First  = First;
    Trace->Second = Second;
    Trace->Closed = Closed;
    auto Owned = std::make_shared<BallLaw>(Law);
    Owned->Direction = 1.0;
    Trace->Law = Owned;
    if (Closed) High = Low + FullTurn;
    if (!(High > Low)) return std::nullopt;

    // ---- Scale for the Newton acceptance: chord length of the edge.
    constexpr int Coarse = 32;                                                               // [-] - Initial samples
    for (int Index = 0; Index < Coarse; ++Index) Trace->Perimeter += Distance(EvaluateCurve(Law.Edge, Low + (High - Low) * Index / Coarse).Position, EvaluateCurve(Law.Edge, Low + (High - Low) * (Index + 1) / Coarse).Position);

    // ---- Exact samples, bisected in τ wherever the spine tangent turns by more than TurnLimit.
    struct Sample { double τ; Vector3 Position; Vector3 Derivative; };
    std::vector<Sample> Samples;
    for (int Index = 0; Index <= Coarse; ++Index)
    {
        Sample Entry{ Low + (High - Low) * Index / Coarse, {}, {} };
        if (!SolveBallCentre(*Trace, Entry.τ, Entry.Position, Entry.Derivative)) return std::nullopt;
        Samples.push_back(Entry);
    }
    for (int Round = 0; Round < RefineLimit; ++Round)
    {
        bool Inserted = false;
        std::vector<Sample> Refined;
        for (size_t Index = 0; Index < Samples.size(); ++Index)
        {
            Refined.push_back(Samples[Index]);
            if (Index + 1 == Samples.size()) continue;
            const Vector3 Start = Samples[Index].Derivative, End = Samples[Index + 1].Derivative;
            if (std::atan2(Length(Cross(Start, End)), Dot(Start, End)) <= TurnLimit) continue;
            Sample Middle{ 0.5 * (Samples[Index].τ + Samples[Index + 1].τ), {}, {} };
            if (!SolveBallCentre(*Trace, Middle.τ, Middle.Position, Middle.Derivative)) return std::nullopt;
            Refined.push_back(Middle);
            Inserted = true;
        }
        Samples.swap(Refined);
        if (!Inserted) break;
    }
    if (Closed) Samples.back().Position = Samples.front().Position, Samples.back().Derivative = Samples.front().Derivative;
    Trace->Perimeter = 0.0;
    for (size_t Index = 0; Index < Samples.size(); ++Index)
    {
        if (Index > 0) Trace->Perimeter += Distance(Samples[Index - 1].Position, Samples[Index].Position);
        Trace->Points.push_back(Samples[Index].Position);
        Trace->Tangents.push_back(Samples[Index].Derivative);
        Trace->Knots.push_back(Samples[Index].τ);
    }
    return IntersectionCurve{ std::move(Trace) };
}

double MeasureBallLawRadius(const TracedIntersection& Trace, double τ) noexcept
{
    return Trace.Law ? LawRadiusAt(Trace, Trace.Law->Direction * FoldParameter(Trace, τ)) : 0.0;
}

double MeasureIntersectionCurveResidual(const TracedIntersection& Trace, double Low, double High, int Samples) noexcept
{
    double Worst = 0.0;                                                                      // [len] - max |dᵢ(C(τ))|
    for (int Index = 0; Index <= Samples; ++Index)
    {
        const double  τ        = Low + (High - Low) * Index / std::max(Samples, 1);
        const Vector3 Position = EvaluateIntersectionCurve(Trace, τ).Position;
        const double  Radius   = MeasureBallLawRadius(Trace, τ);                            // [len] - 0 for a plain trace
        const double  First    = Trace.Law ? Trace.Law->Signs[0] * Radius : 0.0;
        const double  Second   = Trace.Law ? Trace.Law->Signs[1] * Radius : 0.0;
        Worst = std::max({ Worst, std::fabs(MeasureSurfaceDistance(Trace.First, Position).Distance - First), std::fabs(MeasureSurfaceDistance(Trace.Second, Position).Distance - Second) });
    }
    return Worst;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    CURVEGEOMETRY DISPATCH
//------------------------------------------------------------------------------------------------------------------------

CurveSample EvaluateIntersectionCurve(const TracedIntersection& Trace, double τ) noexcept
{
    if (Trace.Law)
    {
        const double Folded = FoldParameter(Trace, τ);                                       // [-] - Spine τ
        Vector3 Centre, Derivative;
        if (SolveBallCentre(Trace, Trace.Law->Direction * Folded, Centre, Derivative)) return { Centre, Derivative * Trace.Law->Direction };
        const GuideSample Guide = EvaluateGuide(Trace, Folded);
        return { Guide.Position, Guide.First };
    }
    const GuideSample Guide = EvaluateGuide(Trace, FoldParameter(Trace, τ));
    Vector3 Position = Guide.Position;
    std::array<Vector3, 2> Normals;
    if (!CorrectOntoBoth(Trace.First, Trace.Second, Position, Guide.First, Guide.Position, Trace.Perimeter, &Normals)) return { Guide.Position, Guide.First };
    const Vector3 Direction = Cross(Normals[0], Normals[1]);
    const double  Sine      = Length(Direction);                                             // [-] - |N₁ × N₂|
    if (Sine <= 1.0e-10) return { Position, Guide.First };
    Vector3 Unit = Direction * (1.0 / Sine);                                                 // [-] - 𝐭
    if (Dot(Unit, Guide.First) < 0.0) Unit = -Unit;
    const double Speed = (Dot(Guide.First, Guide.First) - Dot(Position - Guide.Position, Guide.Second)) / Dot(Unit, Guide.First);
    return { Position, Unit * Speed };
}

double ProjectOntoIntersectionCurve(const TracedIntersection& Trace, Vector3 Position, double IntervalLow) noexcept
{
    // ① Nearest guide segment; ② Gauss–Newton on g(τ) = (C(τ) − X)·C′(τ), steps capped at one segment.
    double τ = Trace.Knots.front(), Nearest = INFINITY, Widest = 0.0;
    for (size_t Index = 0; Index + 1 < Trace.Points.size(); ++Index)
    {
        double Fraction = 0.0;
        const double Gap = SegmentDistance(Position, Trace.Points[Index], Trace.Points[Index + 1], Fraction);
        Widest = std::max(Widest, Trace.Knots[Index + 1] - Trace.Knots[Index]);
        if (Gap < Nearest) { Nearest = Gap; τ = Trace.Knots[Index] + Fraction * (Trace.Knots[Index + 1] - Trace.Knots[Index]); }
    }
    for (int Iteration = 0; Iteration < 40; ++Iteration)
    {
        const CurveSample Sample = EvaluateIntersectionCurve(Trace, τ);
        const double Square = Dot(Sample.Derivative, Sample.Derivative);                     // [len²] - |C′|²
        if (Square <= 1.0e-300) break;
        const double Step = std::clamp(Dot(Sample.Position - Position, Sample.Derivative) / Square, -Widest, Widest);
        τ -= Step;
        if (std::fabs(Step) <= 1.0e-15 * (1.0 + std::fabs(τ))) break;
    }
    if (!Trace.Closed) return τ;
    // 📝 Fold into [IntervalLow, IntervalLow + 2π); a hair below the low end maps to the low end.
    τ = IntervalLow + std::fmod(std::fmod(τ - IntervalLow, FullTurn) + FullTurn, FullTurn);
    if (τ - IntervalLow > FullTurn - 1.0e-12) τ = IntervalLow;
    return τ;
}

bool IntersectionCurveClosedCondition(const TracedIntersection& Trace) noexcept
{
    return Trace.Closed;
}

double MeasureIntersectionCurveBulge(const TracedIntersection& Trace, double Low, double High, int Pieces) noexcept
{
    // 📝 Sampled sagitta per piece (7 interior samples), inflated by ¼ — the guide turns ≤ 0.1 rad per segment, so the
    //    true sagitta between samples is a small fraction of the sampled one.
    const int Count = std::max(Pieces, 1);
    double Worst = 0.0;                                                                      // [len] - Largest sampled sagitta
    for (int Piece = 0; Piece < Count; ++Piece)
    {
        const double  From  = Low + (High - Low) * Piece / Count, To = Low + (High - Low) * (Piece + 1) / Count;
        const Vector3 Start = EvaluateIntersectionCurve(Trace, From).Position, End = EvaluateIntersectionCurve(Trace, To).Position;
        for (int Sample = 1; Sample < 8; ++Sample)
        {
            double Fraction = 0.0;
            Worst = std::max(Worst, SegmentDistance(EvaluateIntersectionCurve(Trace, From + (To - From) * Sample / 8.0).Position, Start, End, Fraction));
        }
    }
    return 1.25 * Worst + 1.0e-12 * (1.0 + Trace.Perimeter);
}

std::vector<double> CollectIntersectionCurveKnots(const TracedIntersection& Trace, double Low, double High)
{
    if (Trace.Law)
    {
        // ⚠️ A ball-law spine is as smooth as its edge's parametrisation: on a traced edge E(τ) is only C¹ at the edge's
        //    knots (H″ jumps), so C′ (through E″) jumps there too. Its own guide knots are smooth points; report the edge's.
        const double Direction = Trace.Law->Direction;                                       // [-] - Spine τ = Direction · edge τ
        std::vector<double> Breakpoints;
        for (double Knot : CollectCurveBreakpoints(Trace.Law->Edge, std::min(Direction * Low, Direction * High), std::max(Direction * Low, Direction * High)))
            Breakpoints.push_back(Direction * Knot);
        std::sort(Breakpoints.begin(), Breakpoints.end());
        return Breakpoints;
    }
    std::vector<double> Breakpoints;
    const double Slack = 1.0e-12 * (1.0 + std::fabs(High - Low));                            // [-] - Ends are not breakpoints
    const double Period = Trace.Closed ? FullTurn : 0.0;                                     // [-] - Knot repetition
    const int    First  = Trace.Closed ? static_cast<int>(std::floor((Low - Trace.Knots.back()) / FullTurn)) : 0;
    const int    Last   = Trace.Closed ? static_cast<int>(std::ceil((High - Trace.Knots.front()) / FullTurn)) : 0;
    for (int Turn = First; Turn <= Last; ++Turn)
    {
        for (double Knot : Trace.Knots)
        {
            const double τ = Knot + Period * Turn;
            if (τ > Low + Slack && τ < High - Slack && (Breakpoints.empty() || τ > Breakpoints.back() + Slack)) Breakpoints.push_back(τ);
        }
    }
    return Breakpoints;
}

double MeasureIntersectionCurveScale(const TracedIntersection& Trace) noexcept
{
    return Trace.Closed ? Trace.Perimeter / FullTurn : 1.0;
}

} // namespace Frontier::SolidArcRemastered
