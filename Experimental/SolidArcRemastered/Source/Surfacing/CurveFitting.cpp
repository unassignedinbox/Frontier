/*====================================================================================================================================
                                                           CURVEFITTING.CPP
====================================================================================================================================*/
// 🧩 Space-curve fitting: centripetal parameters, faired least squares, foot-point correction, knot refinement

//------------------------------------------------------------------------------------------------------------------------
//                                                    INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Surfacing/CurveFitting.h"
#include "Surfacing/LinearSystem.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <optional>
#include <set>

namespace Frontier::SolidArcRemastered
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    PRIVATE FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

namespace
{
    constexpr double DuplicateGap = 1.0e-12;                                                 // [len] - Consecutive points closer than this merge
    constexpr std::array<double, 4> GaussAbscissae = { -0.8611363115940526, -0.3399810435848563, 0.3399810435848563, 0.8611363115940526 };
    constexpr std::array<double, 4> GaussWeights   = {  0.3478548451374538,  0.6521451548625461, 0.6521451548625461, 0.3478548451374538 };

    // Consecutive duplicates removed; Source maps each kept point back to its input index.
    [[nodiscard]] std::vector<Vector3> MergeDuplicates(const std::vector<Vector3>& Points, std::vector<int>& Source)
    {
        std::vector<Vector3> Kept;
        for (size_t Index = 0; Index < Points.size(); ++Index)
        {
            if (!Kept.empty() && Length(Points[Index] - Kept.back()) <= DuplicateGap) continue;
            Kept.push_back(Points[Index]);
            Source.push_back(static_cast<int>(Index));
        }
        return Kept;
    }

    // 𝑡ₖ ∝ Σ √|Qᵢ − Qᵢ₋₁| (Lee 1989), normalised onto [0, 1].
    [[nodiscard]] std::vector<double> CentripetalParameters(const std::vector<Vector3>& Points)
    {
        std::vector<double> Parameters(Points.size(), 0.0);
        for (size_t Index = 1; Index < Points.size(); ++Index) Parameters[Index] = Parameters[Index - 1] + std::sqrt(Length(Points[Index] - Points[Index - 1]));
        const double Total = Parameters.back();
        for (double& Parameter : Parameters) Parameter /= Total;
        Parameters.back() = 1.0;
        return Parameters;
    }

    [[nodiscard]] std::vector<double> ClampedKnots(int Degree, const std::vector<double>& Interior)
    {
        std::vector<double> Knots(Degree + 1, 0.0);
        Knots.insert(Knots.end(), Interior.begin(), Interior.end());
        Knots.insert(Knots.end(), Degree + 1, 1.0);
        return Knots;
    }

    // 📝 Normal equations for the interior poles P₁ … Pₙ₋₁ (P₀ = Q₀, Pₙ = Qₘ fixed):
    //    (NᵀN + λ·G)·P = Nᵀ·R − λ·G_boundary·P_boundary, Gᵢⱼ = ∫ Nᵢ″ Nⱼ″ d𝑡 (4-point Gauss per span: exact to degree 5).
    [[nodiscard]] std::optional<std::vector<Vector3>> SolvePoles(const std::vector<Vector3>& Points, const std::vector<double>& Parameters, int Degree,
                                                                 const std::vector<double>& Knots, double Fairing)
    {
        const int Count = static_cast<int>(Knots.size()) - Degree - 1;
        std::vector<Vector3> Poles(Count);
        Poles.front() = Points.front();
        Poles.back()  = Points.back();
        const int Unknowns = Count - 2;
        if (Unknowns <= 0) return Poles;
        DenseMatrix Matrix(Unknowns, Unknowns), RightSides(Unknowns, 3);
        auto Interior = [&](int Pole) { return Pole >= 1 && Pole <= Count - 2; };
        for (size_t Point = 0; Point < Points.size(); ++Point)
        {
            const NurbsBasisSample Basis = EvaluateNurbsBasis(Knots, Degree, Count, Parameters[Point]);
            Vector3 Target = Points[Point];
            for (int Row = 0; Row <= Degree; ++Row)
            {
                const int Pole = Basis.First + Row;
                if (!Interior(Pole)) Target = Target - Poles[Pole] * Basis.Values[0][Row];
            }
            for (int Row = 0; Row <= Degree; ++Row)
            {
                const int Pole = Basis.First + Row;
                if (!Interior(Pole)) continue;
                const double Value = Basis.Values[0][Row];
                RightSides.At(Pole - 1, 0) += Value * Target.X;
                RightSides.At(Pole - 1, 1) += Value * Target.Y;
                RightSides.At(Pole - 1, 2) += Value * Target.Z;
                for (int Column = 0; Column <= Degree; ++Column)
                {
                    const int Other = Basis.First + Column;
                    if (Interior(Other)) Matrix.At(Pole - 1, Other - 1) += Value * Basis.Values[0][Column];
                }
            }
        }
        const double λ = Fairing * static_cast<double>(Points.size());
        if (λ > 0.0 && Degree >= 2)
        {
            for (int Span = Degree; Span < Count; ++Span)
            {
                const double Low = Knots[Span], High = Knots[Span + 1];
                if (!(High > Low)) continue;
                for (size_t Abscissa = 0; Abscissa < GaussAbscissae.size(); ++Abscissa)
                {
                    const double Parameter = 0.5 * (Low + High) + 0.5 * (High - Low) * GaussAbscissae[Abscissa];
                    const double Weight    = 0.5 * (High - Low) * GaussWeights[Abscissa] * λ;
                    const NurbsBasisSample Basis = EvaluateNurbsBasis(Knots, Degree, Count, Parameter);
                    for (int Row = 0; Row <= Degree; ++Row)
                    {
                        const int Pole = Basis.First + Row;
                        if (!Interior(Pole)) continue;
                        for (int Column = 0; Column <= Degree; ++Column)
                        {
                            const int    Other = Basis.First + Column;
                            const double Term  = Weight * Basis.Values[2][Row] * Basis.Values[2][Column];
                            if (Interior(Other)) { Matrix.At(Pole - 1, Other - 1) += Term; continue; }
                            RightSides.At(Pole - 1, 0) -= Term * Poles[Other].X;
                            RightSides.At(Pole - 1, 1) -= Term * Poles[Other].Y;
                            RightSides.At(Pole - 1, 2) -= Term * Poles[Other].Z;
                        }
                    }
                }
            }
        }
        const std::optional<DenseMatrix> Solution = SolveDenseSystem(std::move(Matrix), std::move(RightSides));
        if (!Solution) return std::nullopt;
        for (int Pole = 1; Pole <= Count - 2; ++Pole) Poles[Pole] = { Solution->At(Pole - 1, 0), Solution->At(Pole - 1, 1), Solution->At(Pole - 1, 2) };
        return Poles;
    }

    [[nodiscard]] NurbsCurveDefinition MakeDefinition(int Degree, const std::vector<double>& Knots, const std::vector<Vector3>& Poles)
    {
        NurbsCurveDefinition Definition;
        Definition.Degree  = Degree;
        Definition.Knots   = Knots;
        Definition.Poles   = Poles;
        Definition.Weights = std::vector<double>(Poles.size(), 1.0);
        return Definition;
    }

    // Hoschek correction: each interior 𝑡ₖ moves to the foot point of Qₖ (Newton on (C − Q)·C′ = 0), ends stay 0 and 1.
    void CorrectParameters(const NurbsCurveDefinition& Definition, const std::vector<Vector3>& Points, std::vector<double>& Parameters)
    {
        for (size_t Point = 1; Point + 1 < Points.size(); ++Point)
        {
            double Parameter = Parameters[Point];
            for (int Iteration = 0; Iteration < 4; ++Iteration)
            {
                const NurbsCurveDerivatives Sample = EvaluateNurbsCurveDerivatives(Definition, Parameter);
                const Vector3 Gap      = Sample.Position - Points[Point];
                const double  Slope    = Dot(Gap, Sample.First);
                const double  Steepness = Dot(Sample.First, Sample.First) + Dot(Gap, Sample.Second);
                if (!(Steepness > 0.0)) break;
                Parameter = std::clamp(Parameter - Slope / Steepness, 0.0, 1.0);
            }
            Parameters[Point] = Parameter;
        }
    }

    struct FitAttempt
    {
        std::vector<Vector3> Poles      = {};                                                // [len] - Solved poles
        std::vector<double>  Parameters = {};                                                // [-]   - Corrected 𝑡ₖ
        double               Worst      = 0.0;                                               // [len] - max |Qₖ − C(𝑡ₖ)|
    };

    [[nodiscard]] double MeasureWorst(const NurbsCurveDefinition& Definition, const std::vector<Vector3>& Points, const std::vector<double>& Parameters)
    {
        double Worst = 0.0;
        for (size_t Point = 0; Point < Points.size(); ++Point) Worst = std::max(Worst, Length(EvaluateNurbsCurveDerivatives(Definition, Parameters[Point]).Position - Points[Point]));
        return Worst;
    }

    // Solve, then alternate foot-point correction and re-solve (Rounds times) — one fit on fixed knots and λ.
    [[nodiscard]] std::optional<FitAttempt> SolveCorrected(const std::vector<Vector3>& Points, std::vector<double> Parameters, int Degree,
                                                           const std::vector<double>& Knots, double Fairing, int Rounds)
    {
        std::optional<std::vector<Vector3>> Poles = SolvePoles(Points, Parameters, Degree, Knots, Fairing);
        if (!Poles) return std::nullopt;
        for (int Round = 0; Round < Rounds; ++Round)
        {
            std::vector<double> Candidate = Parameters;
            CorrectParameters(MakeDefinition(Degree, Knots, *Poles), Points, Candidate);
            std::optional<std::vector<Vector3>> Corrected = SolvePoles(Points, Candidate, Degree, Knots, Fairing);
            if (!Corrected) break;
            Poles      = std::move(Corrected);
            Parameters = std::move(Candidate);
        }
        FitAttempt Attempt;
        Attempt.Poles      = std::move(*Poles);
        Attempt.Parameters = std::move(Parameters);
        // Final foot points on the final curve, so the reported deviation is a true distance.
        const NurbsCurveDefinition Definition = MakeDefinition(Degree, Knots, Attempt.Poles);
        CorrectParameters(Definition, Points, Attempt.Parameters);
        Attempt.Worst = MeasureWorst(Definition, Points, Attempt.Parameters);
        return Attempt;
    }

    void MeasureDeviation(const NurbsCurveDefinition& Definition, const std::vector<Vector3>& Points, const std::vector<double>& Parameters,
                          const std::vector<int>& Source, CurveFitOutcome& Outcome)
    {
        Outcome.WorstDeviation = 0.0;
        Outcome.MeanDeviation  = 0.0;
        for (size_t Point = 0; Point < Points.size(); ++Point)
        {
            const double Deviation = Length(EvaluateNurbsCurveDerivatives(Definition, Parameters[Point]).Position - Points[Point]);
            Outcome.MeanDeviation += Deviation;
            if (Deviation >= Outcome.WorstDeviation) { Outcome.WorstDeviation = Deviation; Outcome.WorstPoint = Source[Point]; }
        }
        Outcome.MeanDeviation /= static_cast<double>(Points.size());
    }

    // Interior maxima of κ = |C′ × C″| / |C′|³ on 400 samples, ignoring ripples below 10⁻⁶ of the largest κ.
    [[nodiscard]] int CountExtrema(const NurbsCurveDefinition& Definition)
    {
        constexpr int Samples = 400;
        std::vector<double> Curvature(Samples + 1, 0.0);
        double Largest = 0.0;
        for (int Index = 0; Index <= Samples; ++Index)
        {
            const NurbsCurveDerivatives Sample = EvaluateNurbsCurveDerivatives(Definition, static_cast<double>(Index) / Samples);
            const double Speed = Length(Sample.First);
            Curvature[Index] = Speed > 0.0 ? Length(Cross(Sample.First, Sample.Second)) / (Speed * Speed * Speed) : 0.0;
            Largest = std::max(Largest, Curvature[Index]);
        }
        int Extrema = 0;
        for (int Index = 1; Index < Samples; ++Index)
        {
            if (Curvature[Index] > Curvature[Index - 1] && Curvature[Index] >= Curvature[Index + 1] && Curvature[Index] > 1.0e-6 * Largest) ++Extrema;
        }
        return Extrema;
    }

    void Finish(CurveFitOutcome& Outcome, const NurbsCurveDefinition& Definition)
    {
        Outcome.Curve            = NurbsCurve{ std::make_shared<NurbsCurveDefinition>(Definition) };
        Outcome.Spans            = static_cast<int>(Definition.Poles.size()) - Definition.Degree;
        Outcome.Extrema = CountExtrema(Definition);
    }

    [[nodiscard]] std::string FormatLength(double Value)
    {
        char Text[48];
        std::snprintf(Text, sizeof Text, "%.3g mm", Value * 1000.0);
        return Text;
    }
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    PUBLIC FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

CurveFitOutcome FitSpaceCurve(const std::vector<Vector3>& Points, const CurveFitSettings& Settings)
{
    CurveFitOutcome Outcome;
    std::vector<int> Source;
    const std::vector<Vector3> Kept = MergeDuplicates(Points, Source);
    if (Kept.size() < 2) { Outcome.Refusal = "Fit: fewer than 2 distinct points"; return Outcome; }
    if (!(Settings.Tolerance > 0.0)) { Outcome.Refusal = "Fit: tolerance must be positive"; return Outcome; }
    const int Degree = std::clamp(Settings.Degree, 1, std::min(NurbsDegreeLimit, static_cast<int>(Kept.size()) - 1));
    std::vector<double> Parameters = CentripetalParameters(Kept);
    std::vector<double> Interior;
    for (;;)
    {
        // ① Faired solve on the current knots (with foot-point correction); accept when every point is within tolerance.
        const std::vector<double> Knots = ClampedKnots(Degree, Interior);
        std::optional<FitAttempt> Faired = SolveCorrected(Kept, Parameters, Degree, Knots, Settings.Fairing, Settings.CorrectionRounds);
        if (!Faired) { Outcome.Refusal = "Fit: singular system at " + std::to_string(Interior.size() + 1) + " spans (too few points per span)"; return Outcome; }
        auto Accept = [&](const FitAttempt& Attempt, double Fairing)
        {
            const NurbsCurveDefinition Definition = MakeDefinition(Degree, Knots, Attempt.Poles);
            MeasureDeviation(Definition, Kept, Attempt.Parameters, Source, Outcome);
            Outcome.Parameters.assign(Points.size(), 0.0);
            size_t KeptIndex = 0;                                                            // Merged duplicates share their survivor's 𝑡
            for (size_t Index = 0; Index < Points.size(); ++Index)
            {
                if (KeptIndex + 1 < Kept.size() && Source[KeptIndex + 1] == static_cast<int>(Index)) ++KeptIndex;
                Outcome.Parameters[Index] = Attempt.Parameters[KeptIndex];
            }
            Outcome.Fairing = Fairing;
            Finish(Outcome, Definition);
            return Outcome;
        };
        if (Faired->Worst <= Settings.Tolerance) return Accept(*Faired, Settings.Fairing);

        // ② Fairing must never cost the tolerance: when the unfaired fit passes on these knots, λ drops by decades until
        //    it passes too (the fairest curve that still honours the tolerance); knots are added only when λ = 0 fails.
        std::optional<FitAttempt> Plain = Settings.Fairing > 0.0 ? SolveCorrected(Kept, Parameters, Degree, Knots, 0.0, Settings.CorrectionRounds) : Faired;
        if (Plain && Plain->Worst <= Settings.Tolerance)
        {
            for (double Fairing = 0.1 * Settings.Fairing; Fairing >= 1.0e-8 * Settings.Fairing; Fairing *= 0.1)
            {
                const std::optional<FitAttempt> Lighter = SolveCorrected(Kept, Parameters, Degree, Knots, Fairing, Settings.CorrectionRounds);
                if (Lighter && Lighter->Worst <= Settings.Tolerance) return Accept(*Lighter, Fairing);
            }
            return Accept(*Plain, 0.0);
        }
        const FitAttempt& Guide = Plain ? *Plain : *Faired;
        const NurbsCurveDefinition Definition = MakeDefinition(Degree, Knots, Guide.Poles);
        MeasureDeviation(Definition, Kept, Guide.Parameters, Source, Outcome);
        Parameters = Guide.Parameters;

        // ③ Split every span holding a point beyond tolerance at its middle.
        std::set<int> Offending;
        for (size_t Point = 0; Point < Kept.size(); ++Point)
        {
            if (Length(EvaluateNurbsCurveDerivatives(Definition, Parameters[Point]).Position - Kept[Point]) <= Settings.Tolerance) continue;
            const int Span = static_cast<int>(std::upper_bound(Knots.begin() + Degree, Knots.end() - Degree - 1, Parameters[Point]) - Knots.begin()) - 1;
            Offending.insert(std::clamp(Span, Degree, static_cast<int>(Knots.size()) - Degree - 2));
        }
        const int Spans = static_cast<int>(Interior.size()) + 1;
        if (Spans + static_cast<int>(Offending.size()) > Settings.SpanLimit || Degree + Spans + static_cast<int>(Offending.size()) > static_cast<int>(Kept.size()))
        {
            Outcome.Refusal = "Fit: point " + std::to_string(Outcome.WorstPoint) + " stays " + FormatLength(Outcome.WorstDeviation) + " away at "
                            + std::to_string(Spans) + " spans (limit " + std::to_string(Settings.SpanLimit) + ", tolerance " + FormatLength(Settings.Tolerance) + ")";
            return Outcome;
        }
        for (int Span : Offending) Interior.push_back(0.5 * (Knots[Span] + Knots[Span + 1]));
        std::sort(Interior.begin(), Interior.end());
    }
}

CurveFitOutcome FitSpaceCurveOnKnots(const std::vector<Vector3>& Points, const std::vector<double>& Parameters, int Degree,
                                     const std::vector<double>& Knots, double Fairing)
{
    CurveFitOutcome Outcome;
    if (Points.size() < 2 || Parameters.size() != Points.size()) { Outcome.Refusal = "Fit: points and parameters differ in count"; return Outcome; }
    const std::optional<std::vector<Vector3>> Poles = SolvePoles(Points, Parameters, Degree, Knots, Fairing);
    if (!Poles) { Outcome.Refusal = "Fit: singular system"; return Outcome; }
    const NurbsCurveDefinition Definition = MakeDefinition(Degree, Knots, *Poles);
    std::vector<int> Source(Points.size());
    for (size_t Index = 0; Index < Points.size(); ++Index) Source[Index] = static_cast<int>(Index);
    MeasureDeviation(Definition, Points, Parameters, Source, Outcome);
    Outcome.Parameters = Parameters;
    Finish(Outcome, Definition);
    return Outcome;
}

std::vector<int> FindCurveCorners(const std::vector<Vector3>& Points, double Angle)
{
    std::vector<int> Corners;
    for (size_t Index = 1; Index + 1 < Points.size(); ++Index)
    {
        const Vector3 Before = Points[Index] - Points[Index - 1], After = Points[Index + 1] - Points[Index];
        const double  Lengths = Length(Before) * Length(After);
        if (!(Lengths > 0.0)) continue;
        const double Turning = std::acos(std::clamp(Dot(Before, After) / Lengths, -1.0, 1.0));
        if (Turning > Angle) Corners.push_back(static_cast<int>(Index));
    }
    return Corners;
}

std::vector<CurveFitOutcome> FitSpaceCurveRuns(const std::vector<Vector3>& Points, const CurveFitSettings& Settings)
{
    std::vector<int> Cuts = FindCurveCorners(Points, Settings.CornerAngle);
    Cuts.insert(Cuts.begin(), 0);
    Cuts.push_back(static_cast<int>(Points.size()) - 1);
    std::vector<CurveFitOutcome> Runs;
    for (size_t Index = 0; Index + 1 < Cuts.size(); ++Index)
    {
        const std::vector<Vector3> Run(Points.begin() + Cuts[Index], Points.begin() + Cuts[Index + 1] + 1);
        CurveFitOutcome Outcome = FitSpaceCurve(Run, Settings);
        if (Outcome.WorstPoint >= 0) Outcome.WorstPoint += Cuts[Index];
        Runs.push_back(std::move(Outcome));
    }
    return Runs;
}

}
