/*====================================================================================================================================
                                                       ENCLOSEDVOLUME.CPP
====================================================================================================================================*/
// 🧩 Exact enclosed volume and parametric face area by Green's theorem over SP-curves (no tessellation)

#include "Audit/EnclosedVolume.h"

#include "Geometry/NurbsCarrier.h"
#include "Geometry/RollingBallCarrier.h"

#include <array>
#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <vector>

namespace Frontier::SolidArcRemastered
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    INTERNAL FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

namespace
{
    // 📝 8-point Gauss–Legendre on [−1, 1]: exact for polynomials of degree ≤ 15 per panel.
    constexpr std::array<double, 8> GaussAbscissae = {
        -0.9602898564975363, -0.7966664774136267, -0.5255324099163290, -0.1834346424956498,
         0.1834346424956498,  0.5255324099163290,  0.7966664774136267,  0.9602898564975363 };
    constexpr std::array<double, 8> GaussWeights = {
         0.1012285362903763,  0.2223810344533745,  0.3137066458778873,  0.3626837833783620,
         0.3626837833783620,  0.3137066458778873,  0.2223810344533745,  0.1012285362903763 };
    constexpr double PanelSpan  = std::numbers::pi / 16.0;                                   // [-] - Max parameter width per panel
    // ⚠️ PanelSpan is an angle; plane charts and line parameters are lengths, so a body at 10³ cut them into ~10⁵ panels
    //    per sample. 64 × π/16 = 4π leaves every angular span untouched.
    constexpr int    PanelLimit = 64;                                                        // [-] - Initial panels per interval

    // Composite Gauss–Legendre of a scalar integrand over [Low, High].
    template <typename Integrand>
    [[nodiscard]] double IntegrateComposite(double Low, double High, double PanelWidth, const Integrand& Function)
    {
        const double Span       = High - Low;                                                // [-] - Signed interval width
        const int    PanelCount = std::clamp(static_cast<int>(std::ceil(std::fabs(Span) / PanelWidth)), 1, PanelLimit);
        const double Width      = Span / PanelCount;                                         // [-] - Signed panel width
        double       Total      = 0.0;                                                       // [-] - Accumulated integral
        for (int PanelIndex = 0; PanelIndex < PanelCount; ++PanelIndex)
        {
            const double Centre = Low + Width * (PanelIndex + 0.5);                          // [-] - Panel midpoint
            for (size_t AbscissaIndex = 0; AbscissaIndex < GaussAbscissae.size(); ++AbscissaIndex)
            {
                Total += GaussWeights[AbscissaIndex] * Function(Centre + GaussAbscissae[AbscissaIndex] * Width * 0.5);
            }
        }
        return Total * Width * 0.5;
    }

    // 📝 One panel's integral and ∫|f| (same nodes): the magnitude makes the acceptance test scale-free.
    struct PanelEstimate
    {
        double Value     = 0.0;                                                              // [-] - ∫ f
        double Magnitude = 0.0;                                                              // [-] - ∫ |f|
    };

    // 8-point Gauss–Legendre on one panel.
    template <typename Integrand>
    [[nodiscard]] PanelEstimate IntegratePanel(double Low, double High, const Integrand& Function)
    {
        const double  Centre = 0.5 * (Low + High), Half = 0.5 * (High - Low);                // [-] - Panel midpoint, half width
        PanelEstimate Total;
        for (size_t AbscissaIndex = 0; AbscissaIndex < GaussAbscissae.size(); ++AbscissaIndex)
        {
            const double Sample = Function(Centre + GaussAbscissae[AbscissaIndex] * Half);
            Total.Value     += GaussWeights[AbscissaIndex] * Sample;
            Total.Magnitude += GaussWeights[AbscissaIndex] * std::fabs(Sample);
        }
        return { Total.Value * Half, Total.Magnitude * std::fabs(Half) };
    }

    // Adaptive composite Gauss–Legendre: a panel is accepted when it agrees with its two halves to 10⁻¹⁴ of ∫|f|.
    // ⚠️ Projected SP-curves passing near a chart pole swing 𝑢 by ~π over a tiny parameter range; fixed panels miss it.
    // ⚠️ The test is relative to ∫|f| (not max(1, |∫f|)): an absolute floor made bodies at 10³ recurse to depth 24
    //    everywhere (round-off never meets it) and bodies at 10⁻³ stop at ~10⁻⁶ relative accuracy.
    template <typename Integrand>
    // ⚠️ Floor (absolute, from the whole face) accepts panels whose integrand is analytically zero but numerically noise
    //    (𝑣′ ~ 10⁻¹⁷ along a projected SP-curve): their relative disagreement never shrinks.
    // ⚠️ Round-off stagnation: a procedural carrier (traced spine, projected rail) evaluates to ~10⁻¹² only, so its
    //    gaps stop shrinking (smooth panels shrink ~2⁻¹⁶ per level, kinked ones ~⅛) and the refinement ran to depth 24
    //    on every panel. A gap already inside the noise band (≤ 10⁻⁹ of ∫|f|) shrinking slower than ¼ per level (noise,
    //    or a sub-tolerance jump whose error is the gap itself) is accepted; larger
    //    gaps (an unresolved pole swing) always refine.
    [[nodiscard]] double IntegrateAdaptive(double Low, double High, double Coarse, const Integrand& Function, double Floor, int Depth, double ParentGap = std::numeric_limits<double>::infinity())
    {
        const double        Middle = 0.5 * (Low + High);
        const PanelEstimate Left   = IntegratePanel(Low, Middle, Function);
        const PanelEstimate Right  = IntegratePanel(Middle, High, Function);
        const double        Gap    = std::fabs(Left.Value + Right.Value - Coarse);           // [-] - Halves vs whole
        const double        Scale  = Left.Magnitude + Right.Magnitude;                       // [-] - ∫|f| over the panel
        if (Depth >= 24 || Gap <= 1.0e-14 * Scale || Gap <= Floor) return Left.Value + Right.Value;
        if (Gap <= 1.0e-9 * Scale && Gap > 0.25 * ParentGap) return Left.Value + Right.Value;
        return IntegrateAdaptive(Low, Middle, Left.Value, Function, Floor, Depth + 1, Gap) + IntegrateAdaptive(Middle, High, Right.Value, Function, Floor, Depth + 1, Gap);
    }

    template <typename Integrand>
    [[nodiscard]] double IntegrateRefined(double Low, double High, double PanelWidth, const Integrand& Function, double Floor)
    {
        const int    PanelCount = std::clamp(static_cast<int>(std::ceil(std::fabs(High - Low) / PanelWidth)), 1, PanelLimit);
        const double Width      = (High - Low) / PanelCount;                                 // [-] - Signed panel width
        double       Total      = 0.0;
        for (int PanelIndex = 0; PanelIndex < PanelCount; ++PanelIndex)
        {
            const double PanelLow = Low + Width * PanelIndex, PanelHigh = PanelLow + Width;
            Total += IntegrateAdaptive(PanelLow, PanelHigh, IntegratePanel(PanelLow, PanelHigh, Function).Value, Function, Floor, 0);
        }
        return Total;
    }

    // ∫|f| over the initial panels of IntegrateRefined (the floor's scale).
    template <typename Integrand>
    [[nodiscard]] double EstimateMagnitude(double Low, double High, double PanelWidth, const Integrand& Function)
    {
        const int    PanelCount = std::clamp(static_cast<int>(std::ceil(std::fabs(High - Low) / PanelWidth)), 1, PanelLimit);
        const double Width      = (High - Low) / PanelCount;                                 // [-] - Signed panel width
        double       Total      = 0.0;
        for (int PanelIndex = 0; PanelIndex < PanelCount; ++PanelIndex) Total += IntegratePanel(Low + Width * PanelIndex, Low + Width * (PanelIndex + 1), Function).Magnitude;
        return Total;
    }

    // ⅓ P·(∂P/∂𝑢 × ∂P/∂𝑣) — the flux density of the position field over the surface chart.
    [[nodiscard]] double FluxDensity(const SurfaceGeometry& Surface, Vector2 Parameter)
    {
        const SurfaceSample Sample = EvaluateSurface(Surface, Parameter);
        return Dot(Sample.Position, Cross(Sample.DerivativeU, Sample.DerivativeV)) / 3.0;
    }

    // Line integral of G(q) dq_𝑣 along one fin, where G = ∫_0^𝑢 Density(s, 𝑣) ds (Density supplied by the caller).
    // Magnitude mode returns ∫|G 𝑣′| dτ over the initial panels instead (no adaptive refinement).
    // ⚠️ Knots: a traced rolling-ball spine is only C¹ at its knots, so the flux density (through T′) jumps there; the
    //    inner 𝑢 integral is split at the chart's breakpoints (Chart may be null: density without a surface).
    template <typename Density>
    [[nodiscard]] double IntegrateFin(const ModelPartition& Partition, FinToken Fin, const Density& Function, double Floor, bool MagnitudeMode = false, const SurfaceGeometry* Chart = nullptr)
    {
        const FinProfile& Use = Partition.Fins.Resolve(Fin);
        if (!Use.SurfaceCurve.has_value()) return std::nan("");
        const ParameterCurve& Curve = *Use.SurfaceCurve;
        // 📝 Any constant base works (∮ H(𝑣) d𝑣 = 0 on a closed chart loop); a NURBS patch starts at its 𝑢-domain so the
        //    inner integral never extrapolates the patch.
        double LowerLimit = 0.0;                                                             // [-] - Inner lower limit
        if (Chart != nullptr) if (const auto* Spline = std::get_if<NurbsSurface>(Chart)) LowerLimit = MeasureNurbsSurfaceDomain(*Spline->Definition, true).Low;
        // ⚠️ An open rolling-ball spine (a ball-law spine extended past its edge) exists only over its range: below it the
        //    chart clamps 𝑢, so the inner integral starts at the spine's low end.
        if (Chart != nullptr) if (const auto* Ball = std::get_if<RollingBallSurface>(Chart); Ball && Ball->Definition->Period == 0.0 && std::isfinite(Ball->Definition->SpineLow))
            LowerLimit = Ball->Definition->SpineLow;
        const auto Integrand = [&](double τ)
        {
            const ParameterSample Sample = EvaluateParameterCurve(Curve, τ);
            if (Sample.Derivative.Y == 0.0) return 0.0;
            const auto   Row   = [&](double s) { return Function(Vector2{ s, Sample.Position.Y }); };
            const double Reach = Sample.Position.X;                                          // [-] - Inner upper limit
            std::vector<double> Knots = Chart != nullptr ? CollectSurfaceBreakpoints(*Chart, std::min(LowerLimit, Reach), std::max(LowerLimit, Reach)) : std::vector<double>{};
            if (Reach < LowerLimit) std::reverse(Knots.begin(), Knots.end());
            double Primitive = 0.0, From = LowerLimit;                                             // [-] - ∫_lower^𝑢 density, running piece start
            for (double Knot : Knots) { Primitive += IntegrateComposite(From, Knot, PanelSpan, Row); From = Knot; }
            Primitive += IntegrateComposite(From, Reach, PanelSpan, Row);
            return Primitive * Sample.Derivative.Y;
        };
        // 📝 Split at the carrier's breakpoints (intersection-curve knots, where it is only C¹) so every panel sees an
        //    analytic integrand: Gauss–Legendre then converges spectrally instead of O(h²) across a kink.
        const double Start = Partition.FinParameterStart(Fin), End = Partition.FinParameterEnd(Fin);
        std::vector<double> Stops = { Start };
        const EdgeProfile& Edge = Partition.Edges.Resolve(Use.Edge);
        if (Edge.Curve.PresentCondition())
        {
            std::vector<double> Breakpoints = CollectCurveBreakpoints(Partition.Curves.Resolve(Edge.Curve), Start, End);
            if (Start > End) std::reverse(Breakpoints.begin(), Breakpoints.end());
            Stops.insert(Stops.end(), Breakpoints.begin(), Breakpoints.end());
        }
        if (Chart != nullptr && std::holds_alternative<NurbsSurface>(*Chart))
        {
            // ⚠️ The density is only C⁰ across a 𝑣-knot: stop where the SP-curve crosses one (64 samples, bisection).
            constexpr int Samples = 64;
            const auto Height = [&](double τ) { return EvaluateParameterCurve(Curve, τ).Position.Y; };
            double LowV = INFINITY, HighV = -INFINITY;
            for (int Index = 0; Index <= Samples; ++Index) { const double 𝑣 = Height(Start + (End - Start) * Index / Samples); LowV = std::min(LowV, 𝑣); HighV = std::max(HighV, 𝑣); }
            for (double Knot : CollectSurfaceBreakpointsAlongV(*Chart, LowV, HighV))
            {
                for (int Index = 0; Index < Samples; ++Index)
                {
                    double Left = Start + (End - Start) * Index / Samples, Right = Start + (End - Start) * (Index + 1) / Samples;
                    double LeftGap = Height(Left) - Knot;
                    const double RightGap = Height(Right) - Knot;
                    if (LeftGap == 0.0 || LeftGap * RightGap >= 0.0) continue;
                    for (int Halving = 0; Halving < 60; ++Halving)
                    {
                        const double Middle = 0.5 * (Left + Right), MiddleGap = Height(Middle) - Knot;
                        if (MiddleGap * LeftGap > 0.0) { Left = Middle; LeftGap = MiddleGap; } else Right = Middle;
                    }
                    Stops.push_back(0.5 * (Left + Right));
                }
            }
            std::sort(Stops.begin() + 1, Stops.end(), [&](double First, double Second) { return Start <= End ? First < Second : First > Second; });
        }
        Stops.push_back(End);
        double Total = 0.0;
        for (size_t Index = 0; Index + 1 < Stops.size(); ++Index)
        {
            Total += MagnitudeMode ? EstimateMagnitude(Stops[Index], Stops[Index + 1], PanelSpan, Integrand) : IntegrateRefined(Stops[Index], Stops[Index + 1], PanelSpan, Integrand, Floor);
        }
        return Total;
    }

    // Σ over a face's fins: integral (absolute acceptance floor supplied) or, in magnitude mode, Σ∫|G 𝑣′| dτ.
    template <typename Density>
    [[nodiscard]] double IntegrateFaceLoops(const ModelPartition& Partition, FaceToken Face, const Density& Function, double Floor, bool MagnitudeMode, const SurfaceGeometry* Chart = nullptr)
    {
        double Total = 0.0;
        for (LoopToken Loop : Partition.Faces.Resolve(Face).Loops)
        {
            Partition.EnumerateLoopFins(Loop, [&](FinToken Fin) { Total += IntegrateFin(Partition, Fin, Function, Floor, MagnitudeMode, Chart); });
        }
        return Total;
    }

    [[nodiscard]] double IntegrateFaceFlux(const ModelPartition& Partition, FaceToken Face, double Floor, bool MagnitudeMode)
    {
        const FaceProfile& Profile = Partition.Faces.Resolve(Face);
        if (!Profile.Surface.PresentCondition()) return std::nan("");
        const SurfaceGeometry& Surface = Partition.Surfaces.Resolve(Profile.Surface);
        return IntegrateFaceLoops(Partition, Face, [&](Vector2 Parameter) { return FluxDensity(Surface, Parameter); }, Floor, MagnitudeMode, &Surface);
    }
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    PUBLIC FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

ShellVolumeMeasure MeasureShellVolume(const ModelPartition& Partition, SolidShellToken SolidShell)
{
    // ⚠️ The noise floor 10⁻¹⁵ · Σ∫|G 𝑣′| is taken over the whole shell, never one face: a plane through the origin
    //    has flux density P·n ≡ 0, so its own magnitude is round-off and a per-face floor never stops the refinement.
    //    A procedural carrier's face is refined only down to its own derivative noise (× its magnitude): below that the
    //    panel gaps are round-off and the refinement would run to the depth cap.
    const std::vector<FaceToken>& Faces = Partition.Shells.Resolve(SolidShell).Faces;
    std::vector<double> FaceMagnitudes(Faces.size(), 0.0);                                   // [len³] - Per face ∫|G 𝑣′| dτ
    double Magnitude = 0.0;                                                                  // [len³] - Σ∫|G 𝑣′| dτ
    for (size_t Index = 0; Index < Faces.size(); ++Index) Magnitude += FaceMagnitudes[Index] = IntegrateFaceFlux(Partition, Faces[Index], 0.0, true);
    ShellVolumeMeasure Measure;
    for (size_t Index = 0; Index < Faces.size(); ++Index)
    {
        const FaceToken Face  = Faces[Index];
        const FaceProfile& Profile = Partition.Faces.Resolve(Face);
        const double Noise = Profile.Surface.PresentCondition() ? MeasureDerivativeNoise(Partition.Surfaces.Resolve(Profile.Surface)) : 0.0;
        const double Flux  = IntegrateFaceFlux(Partition, Face, std::max(1.0e-15 * Magnitude, Noise * FaceMagnitudes[Index]), false);  // [len³] - Face flux
        Measure.Volume        += Flux;
        Measure.FluxMagnitude += std::fabs(Flux);
    }
    return Measure;
}

double IntegrateShellVolume(const ModelPartition& Partition, SolidShellToken SolidShell)
{
    return MeasureShellVolume(Partition, SolidShell).Volume;
}

double IntegrateBodyVolume(const ModelPartition& Partition, BodyToken Body)
{
    double Total = 0.0;                                                                      // [len³] - Body volume
    for (RegionToken Region : Partition.Bodies.Resolve(Body).Regions)
    {
        if (Partition.Regions.Resolve(Region).Material != RegionMaterial::Solid) continue;
        for (SolidShellToken Shell : Partition.Regions.Resolve(Region).Shells) Total += IntegrateShellVolume(Partition, Shell);
    }
    return Total;
}

double IntegrateShellFlux(const ModelPartition& Partition, SolidShellToken SolidShell, const std::function<Vector3(Vector3)>& Field)
{
    // 📝 MeasureShellVolume with a general field: the floor is again taken over the whole shell (a field may vanish on
    //    a whole face, as 𝑥² does on 𝑥 = 0).
    // ⚠️ The floor's scale integrates |F·N|, never F·N: a moment field cancels over a full turn (∫ cos³ = 0 on a
    //    cylinder), so its signed inner integral along the seam is round-off, a floor scaled by it is round-off too,
    //    and every panel recursed to the depth cap (2²⁴ panels per fin).
    const std::vector<FaceToken>& Faces = Partition.Shells.Resolve(SolidShell).Faces;
    const auto Density = [&](const SurfaceGeometry& Surface)
    {
        return [&Surface, &Field](Vector2 Parameter)
        {
            const SurfaceSample Sample = EvaluateSurface(Surface, Parameter);
            return Dot(Field(Sample.Position), Cross(Sample.DerivativeU, Sample.DerivativeV));
        };
    };
    const auto Absolute = [&](const SurfaceGeometry& Surface)
    {
        return [&Surface, &Field](Vector2 Parameter)
        {
            const SurfaceSample Sample = EvaluateSurface(Surface, Parameter);
            return std::fabs(Dot(Field(Sample.Position), Cross(Sample.DerivativeU, Sample.DerivativeV)));
        };
    };
    std::vector<double> FaceMagnitudes(Faces.size(), 0.0);                                   // [-] - Per face ∫|G 𝑣′| dτ
    double Magnitude = 0.0;                                                                  // [-] - Σ∫|G 𝑣′| dτ
    for (size_t Index = 0; Index < Faces.size(); ++Index)
    {
        const FaceProfile& Profile = Partition.Faces.Resolve(Faces[Index]);
        if (!Profile.Surface.PresentCondition()) return std::nan("");
        const SurfaceGeometry& Surface = Partition.Surfaces.Resolve(Profile.Surface);
        Magnitude += FaceMagnitudes[Index] = IntegrateFaceLoops(Partition, Faces[Index], Absolute(Surface), 0.0, true, &Surface);
    }
    double Total = 0.0;
    for (size_t Index = 0; Index < Faces.size(); ++Index)
    {
        const SurfaceGeometry& Surface = Partition.Surfaces.Resolve(Partition.Faces.Resolve(Faces[Index]).Surface);
        const double Floor = std::max(1.0e-15 * Magnitude, MeasureDerivativeNoise(Surface) * FaceMagnitudes[Index]);
        Total += IntegrateFaceLoops(Partition, Faces[Index], Density(Surface), Floor, false, &Surface);
    }
    return Total;
}

double IntegrateFaceSurface(const ModelPartition& Partition, FaceToken Face, const std::function<double(Vector3)>& Weight)
{
    // ⚠️ The sense comes from the parametric area (density 1), never from the weighted total: a weight that changes
    //    sign over the face (a centred coordinate) may integrate to either sign on a correctly oriented face.
    const FaceProfile& Profile = Partition.Faces.Resolve(Face);
    if (!Profile.Surface.PresentCondition()) return std::nan("");
    const SurfaceGeometry& Surface = Partition.Surfaces.Resolve(Profile.Surface);
    const auto Density = [&](Vector2 Parameter)
    {
        const SurfaceSample Sample = EvaluateSurface(Surface, Parameter);
        return Weight(Sample.Position) * Length(Cross(Sample.DerivativeU, Sample.DerivativeV));
    };
    // ⚠️ The floor's scale integrates |w| (see IntegrateShellFlux): a centred weight cancels over a full turn.
    const auto Absolute = [&](Vector2 Parameter)
    {
        const SurfaceSample Sample = EvaluateSurface(Surface, Parameter);
        return std::fabs(Weight(Sample.Position)) * Length(Cross(Sample.DerivativeU, Sample.DerivativeV));
    };
    const double Sense     = IntegrateParametricArea(Partition, Face) < 0.0 ? -1.0 : 1.0;    // [-] - Loop sense in (𝑢, 𝑣)
    const double Magnitude = IntegrateFaceLoops(Partition, Face, Absolute, 0.0, true, &Surface);
    const double Floor     = std::max(1.0e-15, MeasureDerivativeNoise(Surface)) * Magnitude;
    return Sense * IntegrateFaceLoops(Partition, Face, Density, Floor, false, &Surface);
}

double MeasureEdgeLength(const ModelPartition& Partition, EdgeToken Edge)
{
    const EdgeProfile& Profile = Partition.Edges.Resolve(Edge);
    if (!Profile.Curve.PresentCondition()) return std::nan("");
    const CurveGeometry& Curve = Partition.Curves.Resolve(Profile.Curve);
    const auto Speed = [&](double τ) { return Length(EvaluateCurve(Curve, τ).Derivative); };
    std::vector<double> Stops = { Profile.IntervalLow };
    for (double Knot : CollectCurveBreakpoints(Curve, Profile.IntervalLow, Profile.IntervalHigh)) Stops.push_back(Knot);
    Stops.push_back(Profile.IntervalHigh);
    double Total = 0.0;
    for (size_t Index = 0; Index + 1 < Stops.size(); ++Index)
    {
        const double Magnitude = EstimateMagnitude(Stops[Index], Stops[Index + 1], PanelSpan, Speed);
        Total += IntegrateRefined(Stops[Index], Stops[Index + 1], PanelSpan, Speed, 1.0e-15 * Magnitude);
    }
    return std::fabs(Total);
}

double IntegrateParametricArea(const ModelPartition& Partition, FaceToken Face)
{
    // Density 1 never cancels, so the face's own magnitude (× its carrier's derivative noise) is a valid floor here.
    const auto Unit = [](Vector2) { return 1.0; };
    const FaceProfile& Profile = Partition.Faces.Resolve(Face);
    const double Noise = std::max(1.0e-15, Profile.Surface.PresentCondition() ? MeasureDerivativeNoise(Partition.Surfaces.Resolve(Profile.Surface)) : 0.0);
    return IntegrateFaceLoops(Partition, Face, Unit, Noise * IntegrateFaceLoops(Partition, Face, Unit, 0.0, true), false);  // Σ ∮ 𝑢 d𝑣
}

} // namespace Frontier::SolidArcRemastered
