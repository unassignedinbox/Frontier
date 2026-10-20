/*====================================================================================================================================
                                                       ENCLOSEDVOLUME.CPP
====================================================================================================================================*/
// 🧩 Exact enclosed volume and parametric face area by Green's theorem over SP-curves (no tessellation)

#include "Audit/EnclosedVolume.h"

#include <array>
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
    constexpr double PanelSpan = std::numbers::pi / 16.0;                                    // [-] - Max parameter width per panel

    // Composite Gauss–Legendre of a scalar integrand over [Low, High].
    template <typename Integrand>
    [[nodiscard]] double IntegrateComposite(double Low, double High, double PanelWidth, const Integrand& Function)
    {
        const double Span       = High - Low;                                                // [-] - Signed interval width
        const int    PanelCount = std::max(1, static_cast<int>(std::ceil(std::fabs(Span) / PanelWidth)));
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

    // 8-point Gauss–Legendre on one panel.
    template <typename Integrand>
    [[nodiscard]] double IntegratePanel(double Low, double High, const Integrand& Function)
    {
        const double Centre = 0.5 * (Low + High), Half = 0.5 * (High - Low);                 // [-] - Panel midpoint, half width
        double       Total  = 0.0;
        for (size_t AbscissaIndex = 0; AbscissaIndex < GaussAbscissae.size(); ++AbscissaIndex) Total += GaussWeights[AbscissaIndex] * Function(Centre + GaussAbscissae[AbscissaIndex] * Half);
        return Total * Half;
    }

    // Adaptive composite Gauss–Legendre: a panel is accepted when it agrees with its two halves.
    // ⚠️ Projected SP-curves passing near a chart pole swing 𝑢 by ~π over a tiny parameter range; fixed panels miss it.
    template <typename Integrand>
    [[nodiscard]] double IntegrateAdaptive(double Low, double High, double Coarse, const Integrand& Function, int Depth)
    {
        const double Middle = 0.5 * (Low + High);
        const double Left   = IntegratePanel(Low, Middle, Function);
        const double Right  = IntegratePanel(Middle, High, Function);
        if (Depth >= 24 || std::fabs(Left + Right - Coarse) <= 1.0e-15 * std::max(1.0, std::fabs(Coarse))) return Left + Right;
        return IntegrateAdaptive(Low, Middle, Left, Function, Depth + 1) + IntegrateAdaptive(Middle, High, Right, Function, Depth + 1);
    }

    template <typename Integrand>
    [[nodiscard]] double IntegrateRefined(double Low, double High, double PanelWidth, const Integrand& Function)
    {
        const int    PanelCount = std::max(1, static_cast<int>(std::ceil(std::fabs(High - Low) / PanelWidth)));
        const double Width      = (High - Low) / PanelCount;                                 // [-] - Signed panel width
        double       Total      = 0.0;
        for (int PanelIndex = 0; PanelIndex < PanelCount; ++PanelIndex)
        {
            const double PanelLow = Low + Width * PanelIndex, PanelHigh = PanelLow + Width;
            Total += IntegrateAdaptive(PanelLow, PanelHigh, IntegratePanel(PanelLow, PanelHigh, Function), Function, 0);
        }
        return Total;
    }

    // ⅓ P·(∂P/∂𝑢 × ∂P/∂𝑣) — the flux density of the position field over the surface chart.
    [[nodiscard]] double FluxDensity(const SurfaceGeometry& Surface, Vector2 Parameter)
    {
        const SurfaceSample Sample = EvaluateSurface(Surface, Parameter);
        return Dot(Sample.Position, Cross(Sample.DerivativeU, Sample.DerivativeV)) / 3.0;
    }

    // Line integral of G(q) dq_𝑣 along one fin, where G = ∫_0^𝑢 Density(s, 𝑣) ds (Density supplied by the caller).
    template <typename Density>
    [[nodiscard]] double IntegrateFin(const ModelPartition& Partition, FinToken Fin, const Density& Function)
    {
        const FinProfile& Use = Partition.Fins.Resolve(Fin);
        if (!Use.SurfaceCurve.has_value()) return std::nan("");
        const ParameterCurve& Curve = *Use.SurfaceCurve;
        return IntegrateRefined(Partition.FinParameterStart(Fin), Partition.FinParameterEnd(Fin), PanelSpan, [&](double τ)
        {
            const ParameterSample Sample = EvaluateParameterCurve(Curve, τ);
            if (Sample.Derivative.Y == 0.0) return 0.0;
            const double Primitive = IntegrateComposite(0.0, Sample.Position.X, PanelSpan, [&](double s)
            {
                return Function(Vector2{ s, Sample.Position.Y });
            });
            return Primitive * Sample.Derivative.Y;
        });
    }

    [[nodiscard]] double IntegrateFaceFlux(const ModelPartition& Partition, FaceToken Face)
    {
        const FaceProfile& Profile = Partition.Faces.Resolve(Face);
        if (!Profile.Surface.PresentCondition()) return std::nan("");
        const SurfaceGeometry& Surface = Partition.Surfaces.Resolve(Profile.Surface);
        double Total = 0.0;                                                                  // [len³] - Face flux
        for (LoopToken Loop : Profile.Loops)
        {
            Partition.EnumerateLoopFins(Loop, [&](FinToken Fin)
            {
                Total += IntegrateFin(Partition, Fin, [&](Vector2 Parameter) { return FluxDensity(Surface, Parameter); });
            });
        }
        return Total;
    }
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    PUBLIC FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

double IntegrateShellVolume(const ModelPartition& Partition, SolidShellToken Shell)
{
    double Total = 0.0;                                                                      // [len³] - Shell flux
    for (FaceToken Face : Partition.Shells.Resolve(Shell).Faces) Total += IntegrateFaceFlux(Partition, Face);
    return Total;
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
    double Total = 0.0;                                                                      // [-] - Σ ∮ 𝑢 d𝑣
    for (LoopToken Loop : Partition.Faces.Resolve(Face).Loops)
    {
        Partition.EnumerateLoopFins(Loop, [&](FinToken Fin)
        {
            Total += IntegrateFin(Partition, Fin, [](Vector2) { return 1.0; });
        });
    }
    return Total;
}

} // namespace Frontier::SolidArcRemastered
