/*====================================================================================================================================
                                                       ENCLOSEDVOLUME.CPP
====================================================================================================================================*/
// 🧩 Exact enclosed volume and parametric face area by Green's theorem over SP-curves (no tessellation)

#include "Audit/EnclosedVolume.h"

#include <array>
#include <algorithm>
#include <cmath>
#include <numbers>

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
    [[nodiscard]] double IntegrateAdaptive(double Low, double High, double Coarse, const Integrand& Function, double Floor, int Depth)
    {
        const double        Middle = 0.5 * (Low + High);
        const PanelEstimate Left   = IntegratePanel(Low, Middle, Function);
        const PanelEstimate Right  = IntegratePanel(Middle, High, Function);
        const double        Gap    = std::fabs(Left.Value + Right.Value - Coarse);           // [-] - Halves vs whole
        if (Depth >= 24 || Gap <= 1.0e-14 * (Left.Magnitude + Right.Magnitude) || Gap <= Floor) return Left.Value + Right.Value;
        return IntegrateAdaptive(Low, Middle, Left.Value, Function, Floor, Depth + 1) + IntegrateAdaptive(Middle, High, Right.Value, Function, Floor, Depth + 1);
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
    template <typename Density>
    [[nodiscard]] double IntegrateFin(const ModelPartition& Partition, FinToken Fin, const Density& Function, double Floor, bool MagnitudeMode = false)
    {
        const FinProfile& Use = Partition.Fins.Resolve(Fin);
        if (!Use.SurfaceCurve.has_value()) return std::nan("");
        const ParameterCurve& Curve = *Use.SurfaceCurve;
        const auto Integrand = [&](double τ)
        {
            const ParameterSample Sample = EvaluateParameterCurve(Curve, τ);
            if (Sample.Derivative.Y == 0.0) return 0.0;
            const double Primitive = IntegrateComposite(0.0, Sample.Position.X, PanelSpan, [&](double s)
            {
                return Function(Vector2{ s, Sample.Position.Y });
            });
            return Primitive * Sample.Derivative.Y;
        };
        if (MagnitudeMode) return EstimateMagnitude(Partition.FinParameterStart(Fin), Partition.FinParameterEnd(Fin), PanelSpan, Integrand);
        return IntegrateRefined(Partition.FinParameterStart(Fin), Partition.FinParameterEnd(Fin), PanelSpan, Integrand, Floor);
    }

    // Σ over a face's fins: integral (absolute acceptance floor supplied) or, in magnitude mode, Σ∫|G 𝑣′| dτ.
    template <typename Density>
    [[nodiscard]] double IntegrateFaceLoops(const ModelPartition& Partition, FaceToken Face, const Density& Function, double Floor, bool MagnitudeMode)
    {
        double Total = 0.0;
        for (LoopToken Loop : Partition.Faces.Resolve(Face).Loops)
        {
            Partition.EnumerateLoopFins(Loop, [&](FinToken Fin) { Total += IntegrateFin(Partition, Fin, Function, Floor, MagnitudeMode); });
        }
        return Total;
    }

    [[nodiscard]] double IntegrateFaceFlux(const ModelPartition& Partition, FaceToken Face, double Floor, bool MagnitudeMode)
    {
        const FaceProfile& Profile = Partition.Faces.Resolve(Face);
        if (!Profile.Surface.PresentCondition()) return std::nan("");
        const SurfaceGeometry& Surface = Partition.Surfaces.Resolve(Profile.Surface);
        return IntegrateFaceLoops(Partition, Face, [&](Vector2 Parameter) { return FluxDensity(Surface, Parameter); }, Floor, MagnitudeMode);
    }
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    PUBLIC FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

ShellVolumeMeasure MeasureShellVolume(const ModelPartition& Partition, SolidShellToken SolidShell)
{
    // ⚠️ The noise floor 10⁻¹⁵ · Σ∫|G 𝑣′| is taken over the whole shell, never one face: a plane through the origin
    //    has flux density P·n ≡ 0, so its own magnitude is round-off and a per-face floor never stops the refinement.
    double Magnitude = 0.0;                                                                  // [len³] - Σ∫|G 𝑣′| dτ
    for (FaceToken Face : Partition.Shells.Resolve(SolidShell).Faces) Magnitude += IntegrateFaceFlux(Partition, Face, 0.0, true);
    ShellVolumeMeasure Measure;
    for (FaceToken Face : Partition.Shells.Resolve(SolidShell).Faces)
    {
        const double Flux = IntegrateFaceFlux(Partition, Face, 1.0e-15 * Magnitude, false);  // [len³] - Face flux
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

double IntegrateParametricArea(const ModelPartition& Partition, FaceToken Face)
{
    // Density 1 never cancels, so the face's own magnitude is a valid floor scale here.
    const auto Unit = [](Vector2) { return 1.0; };
    return IntegrateFaceLoops(Partition, Face, Unit, 1.0e-15 * IntegrateFaceLoops(Partition, Face, Unit, 0.0, true), false);  // Σ ∮ 𝑢 d𝑣
}

} // namespace Frontier::SolidArcRemastered
