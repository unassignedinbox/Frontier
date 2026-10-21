/*====================================================================================================================================
                                                    PANELSURFACINGVALIDATION.CPP
====================================================================================================================================*/
// 🧩 Vehicle plan V5–V7: curve networks (Gordon / Coons), G1 / G2 across shared sections, sewing → mirror → caps → solid

//------------------------------------------------------------------------------------------------------------------------
//                                                    INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Audit/EnclosedVolume.h"
#include "Audit/ModelAudit.h"
#include "Feature/FeatureTree.h"
#include "Surfacing/CurveFitting.h"
#include "Surfacing/CurveNetwork.h"
#include "Surfacing/PanelAssembly.h"
#include "Surfacing/SectionLoft.h"
#include "Surfacing/SurfaceAlgebra.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <functional>
#include <numbers>
#include <random>
#include <string>
#include <vector>

using namespace Frontier::SolidArcRemastered;

//------------------------------------------------------------------------------------------------------------------------
//                                                    INTERNAL
//------------------------------------------------------------------------------------------------------------------------

namespace
{
    constexpr double PanelTarget = 5.0e-4;                                                   // [len] - Plan deviation target (0.5 mm)

    struct GateTally
    {
        const char* Code     = "";                                                           // [-] - N1 …
        const char* Subject  = "";                                                           // [-] - What it measures
        const char* Limit    = "";                                                           // [-] - Acceptance rule
        double      Worst    = 0.0;                                                          // [-] - Largest measured value
        long        Checks   = 0;                                                            // [-]
        long        Failures = 0;                                                            // [-]
    };

    void Tally(GateTally& Gate, double Value, bool PassedCondition)
    {
        Gate.Worst = std::max(Gate.Worst, Value);
        ++Gate.Checks;
        if (!PassedCondition) ++Gate.Failures;
    }

    GateTally CoonsGate        = { "N1", "Coons: 2 + 2 lines → the bilinear patch (41 × 41); 4 curved boundaries interpolated", "≤ 10⁻¹² · ≤ 10⁻⁹" };
    GateTally GordonGate       = { "N2", "Gordon: iso-curve networks of B-spline surfaces → S(𝑢, 𝑣ⱼ) ≡ first family, S(𝑢ᵢ, 𝑣) ≡ second, vs refit and original curves", "≤ 10⁻⁹" };
    GateTally SortingGate      = { "N3", "Unsorted, reversed input sorts into the same surface (auto families, orientation)", "≤ 10⁻¹²" };
    GateTally BonnetNetGate    = { "N4", "Bonnet network: 6 sections + 4 spines fitted from noisy points → surface vs analytic H and vs every point", "≤ 0.5 mm" };
    GateTally NetworkRefuseGate= { "N5", "Refusals name the curves: no crossing, open network (overrun), odd cycle, isolated curve, too few curves", "all named" };
    GateTally NetworkSheetGate = { "N6", "Network sheet: audit sound · 1 face · 4 laminar edges; L5-style determinism (bit-identical twice)", "sound · equal" };

    GateTally TangentGate      = { "C1", "G1 loft against a neighbour: normal angle along the shared section (bonnet front 1–3 · rear 3–6, 4 side combinations)", "≤ 10⁻⁸ rad" };
    GateTally CurvatureGate    = { "C2", "G2 loft: normal angle and relative jump of the cross normal curvature along the shared section", "≤ 10⁻⁸ rad · ≤ 10⁻⁶" };
    GateTally DiscriminateGate = { "C3", "The gates see a difference: G0 shows a tangent break, G1 a curvature jump (so C1 / C2 are not vacuous)", "> 10⁻⁵ rad · > 10⁻³" };
    GateTally SharedRowGate    = { "C4", "Shared section unchanged: G1 / G2 panels still interpolate every section; shared rows bit-identical", "≤ 10⁻⁹ · equal" };
    GateTally ElevationGate    = { "C5", "Mixed-degree loft (line + quadratic + cubic): exact elevation, sections interpolated", "≤ 10⁻⁹" };
    GateTally ContinuityRefuseGate = { "C6", "Continuity refusals: reference without the section, rational reference, G3, closed sections", "all named" };

    std::vector<GateTally*> AllGates()
    {
        return { &CoonsGate, &GordonGate, &SortingGate, &BonnetNetGate, &NetworkRefuseGate, &NetworkSheetGate,
                 &TangentGate, &CurvatureGate, &DiscriminateGate, &SharedRowGate, &ElevationGate, &ContinuityRefuseGate };
    }

    struct DetailRow
    {
        std::string Gate     = {};
        std::string Name     = {};
        std::string Detail   = {};
        bool        PassedCondition = false;
    };
    std::vector<DetailRow> Details;

    void Report(GateTally& Gate, const std::string& Name, const std::string& Detail, double Value, bool PassedCondition)
    {
        Tally(Gate, Value, PassedCondition);
        Details.push_back({ Gate.Code, Name, Detail, PassedCondition });
    }

    [[nodiscard]] std::string Scientific(double Value)
    {
        char Text[32];
        std::snprintf(Text, sizeof Text, "%.2e", Value);
        return Text;
    }

    [[nodiscard]] std::string Millimetres(double Value)
    {
        char Text[32];
        std::snprintf(Text, sizeof Text, "%.4f mm", Value * 1000.0);
        return Text;
    }

    //--------------------------------------------------------------------------------------------------------------------
    //                                                SPECIMENS
    //--------------------------------------------------------------------------------------------------------------------

    [[nodiscard]] std::vector<double> IrregularStations(int Count)
    {
        std::vector<double> Stations(Count);
        for (int Index = 0; Index < Count; ++Index) Stations[Index] = (Index + 0.3 * std::sin(2.3 * Index)) / (Count - 1);
        Stations.front() = 0.0;
        Stations.back()  = 1.0;
        return Stations;
    }

    [[nodiscard]] Vector3 Jitter(std::mt19937_64& Generator, double Amplitude)
    {
        std::uniform_real_distribution<double> Uniform(-1.0, 1.0);
        const double Scale = Amplitude / std::sqrt(3.0);
        return { Scale * Uniform(Generator), Scale * Uniform(Generator), Scale * Uniform(Generator) };
    }

    // 📝 Synthetic bonnet (half, mirror plane x = 0), as SurfacingValidation: ξ = x / 0.75, η = y / 1.1.
    [[nodiscard]] double BonnetHeight(double X, double Y)
    {
        const double ξ = X / 0.75, η = Y / 1.1;
        return 0.78 + 0.16 * η - 0.06 * η * η - 0.085 * ξ * ξ + 0.012 * std::sin(std::numbers::pi * η) * (1.0 - ξ * ξ);
    }

    [[nodiscard]] Vector3 BonnetGradient(double X, double Y)
    {
        const double ξ = X / 0.75, η = Y / 1.1;
        const double Across = (-0.17 * ξ - 0.024 * ξ * std::sin(std::numbers::pi * η)) / 0.75;
        const double Along  = (0.16 - 0.12 * η + 0.012 * std::numbers::pi * std::cos(std::numbers::pi * η) * (1.0 - ξ * ξ)) / 1.1;
        return { Across, Along, 0.0 };
    }

    // Distance from a point to the analytic bonnet (vertical gap scaled by the slope: first order in the gap).
    [[nodiscard]] double BonnetGap(Vector3 Point)
    {
        const Vector3 Gradient = BonnetGradient(Point.X, Point.Y);
        return std::abs(Point.Z - BonnetHeight(Point.X, Point.Y)) / std::sqrt(1.0 + Dot(Gradient, Gradient));
    }

    // Section loop at η (26 points across), spine at ξ (30 points along): quad spacing, 0.05 mm noise, x = 0 snapped.
    [[nodiscard]] std::vector<Vector3> BonnetSectionPoints(double η, std::mt19937_64& Generator)
    {
        std::vector<Vector3> Points;
        for (double Station : IrregularStations(26))
        {
            const double X = 0.75 * Station, Y = 1.1 * η;
            Vector3 Point = Vector3{ X, Y, BonnetHeight(X, Y) } + Jitter(Generator, 5.0e-5);
            if (Station == 0.0) Point.X = 0.0;
            Points.push_back(Point);
        }
        return Points;
    }

    [[nodiscard]] std::vector<Vector3> BonnetSpinePoints(double ξ, std::mt19937_64& Generator)
    {
        std::vector<Vector3> Points;
        for (double Station : IrregularStations(30))
        {
            const double X = 0.75 * ξ, Y = 1.1 * Station;
            Vector3 Point = Vector3{ X, Y, BonnetHeight(X, Y) } + Jitter(Generator, 5.0e-5);
            if (ξ == 0.0) Point.X = 0.0;
            Points.push_back(Point);
        }
        return Points;
    }

    constexpr double BonnetSectionStations[] = { 0.0, 0.17, 0.38, 0.6, 0.81, 1.0 };
    constexpr double BonnetSpineStations[]   = { 0.0, 0.35, 0.7, 1.0 };

    struct BonnetNetwork
    {
        std::vector<NurbsCurve>           Curves = {};                                       // Sections then spines
        std::vector<std::string>          Names  = {};
        std::vector<std::vector<Vector3>> Points = {};
    };

    [[nodiscard]] BonnetNetwork ComposeBonnetNetwork(uint64_t Seed)
    {
        BonnetNetwork Network;
        std::mt19937_64 Generator(Seed);
        int Index = 1;
        for (double η : BonnetSectionStations)
        {
            Network.Points.push_back(BonnetSectionPoints(η, Generator));
            Network.Curves.push_back(FitSpaceCurve(Network.Points.back(), CurveFitSettings{}).Curve);
            Network.Names.push_back("Section" + std::to_string(Index++));
        }
        Index = 1;
        for (double ξ : BonnetSpineStations)
        {
            Network.Points.push_back(BonnetSpinePoints(ξ, Generator));
            Network.Curves.push_back(FitSpaceCurve(Network.Points.back(), CurveFitSettings{}).Curve);
            Network.Names.push_back("Spine" + std::to_string(Index++));
        }
        return Network;
    }

    // Cubic B-spline surface with non-uniform knots and a smooth pole field (exact test carrier for N2).
    [[nodiscard]] NurbsSurface TestSurface(int CountU, int CountV, double Twist)
    {
        auto Knots = [](int Count)
        {
            std::vector<double> Values(4, 0.0);
            for (int Index = 1; Index < Count - 3; ++Index) Values.push_back((Index + 0.25 * std::sin(1.7 * Index)) / (Count - 3));
            Values.insert(Values.end(), 4, 1.0);
            return Values;
        };
        std::vector<Vector3> Poles;
        for (int IndexU = 0; IndexU < CountU; ++IndexU)
        {
            for (int IndexV = 0; IndexV < CountV; ++IndexV)
            {
                const double U = static_cast<double>(IndexU) / (CountU - 1), V = static_cast<double>(IndexV) / (CountV - 1);
                Poles.push_back({ 1.2 * U + 0.1 * V * V, 0.9 * V + Twist * U * V, 0.25 * std::sin(2.6 * U) * std::cos(1.9 * V) + 0.1 * U * V });
            }
        }
        return *ConstructNurbsSurface(3, 3, CountU, CountV, Knots(CountU), Knots(CountV), Poles, std::vector<double>(Poles.size(), 1.0));
    }

    [[nodiscard]] Vector3 SurfacePoint(const NurbsSurface& Surface, double U, double V) { return EvaluateNurbsSurfaceDerivatives(*Surface.Definition, { U, V }).Position; }
    [[nodiscard]] Vector3 CurvePoint(const NurbsCurve& Curve, double T) { return EvaluateNurbsCurveDerivatives(*Curve.Definition, T).Position; }

    [[nodiscard]] NurbsCurve LineCurve(Vector3 Start, Vector3 End)
    {
        return *ConstructNurbsCurve(1, { 0.0, 0.0, 1.0, 1.0 }, { Start, End }, { 1.0, 1.0 });
    }

    [[nodiscard]] NurbsCurve CubicCurve(Vector3 A, Vector3 B, Vector3 C, Vector3 D)
    {
        return *ConstructNurbsCurve(3, { 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0 }, { A, B, C, D }, { 1.0, 1.0, 1.0, 1.0 });
    }

    // Largest |S(𝑢, 𝑣ⱼ) − Fⱼ(𝑢)| and |S(𝑢ᵢ, 𝑣) − Gᵢ(𝑣)| over 201 samples per curve (refit families).
    [[nodiscard]] double MeasureNetworkInterpolation(const NetworkSurfaceOutcome& Network)
    {
        double Worst = 0.0;
        for (size_t Row = 0; Row < Network.First.size(); ++Row)
        {
            for (int Sample = 0; Sample <= 200; ++Sample)
            {
                const double U = Sample / 200.0;
                Worst = std::max(Worst, Length(SurfacePoint(Network.Surface, U, Network.FirstParameters[Row]) - CurvePoint(Network.First[Row], U)));
            }
        }
        for (size_t Column = 0; Column < Network.Second.size(); ++Column)
        {
            for (int Sample = 0; Sample <= 200; ++Sample)
            {
                const double V = Sample / 200.0;
                Worst = std::max(Worst, Length(SurfacePoint(Network.Surface, Network.SecondParameters[Column], V) - CurvePoint(Network.Second[Column], V)));
            }
        }
        return Worst;
    }

    // Largest distance from the original curves to the surface (each sample projected): the network honours its input.
    [[nodiscard]] double MeasureOriginalGap(const NetworkSurfaceOutcome& Network, const std::vector<NurbsCurve>& Originals)
    {
        double Worst = 0.0;
        for (const NurbsCurve& Curve : Originals)
        {
            for (int Sample = 0; Sample <= 100; ++Sample)
            {
                const Vector3 Point = CurvePoint(Curve, MeasureNurbsCurveDomain(*Curve.Definition).Low + (MeasureNurbsCurveDomain(*Curve.Definition).High - MeasureNurbsCurveDomain(*Curve.Definition).Low) * Sample / 100.0);
                const Vector2 Foot  = ProjectOntoNurbsSurface(Network.Surface, Point, { 0.5, 0.5 });
                Worst = std::max(Worst, Length(SurfacePoint(Network.Surface, Foot.X, Foot.Y) - Point));
            }
        }
        return Worst;
    }

    //--------------------------------------------------------------------------------------------------------------------
    //                                                V5 · NETWORKS
    //--------------------------------------------------------------------------------------------------------------------

    void ValidateNetworks()
    {
        // ---- N1 Coons: four lines of a skew quadrilateral → the bilinear patch.
        {
            const Vector3 A = { 0.0, 0.0, 0.0 }, B = { 1.3, 0.1, 0.2 }, C = { 1.1, 0.9, 0.7 }, D = { -0.2, 1.0, -0.1 };
            const NetworkSurfaceOutcome Coons = ConstructGordonSurface({ LineCurve(A, B), LineCurve(D, C) }, { LineCurve(A, D), LineCurve(B, C) }, NetworkSettings{});
            double Worst = Coons.Refusal.empty() ? 0.0 : 1.0;
            if (Coons.Refusal.empty())
            {
                for (int IndexU = 0; IndexU <= 40; ++IndexU)
                {
                    for (int IndexV = 0; IndexV <= 40; ++IndexV)
                    {
                        const double U = IndexU / 40.0, V = IndexV / 40.0;
                        const Vector3 Bilinear = A * ((1 - U) * (1 - V)) + B * (U * (1 - V)) + C * (U * V) + D * ((1 - U) * V);
                        Worst = std::max(Worst, Length(SurfacePoint(Coons.Surface, U, V) - Bilinear));
                    }
                }
            }
            Report(CoonsGate, "Coons of 4 lines (skew quad) vs bilinear, 41 × 41", Coons.Refusal.empty() ? "max " + Scientific(Worst) + " · degree " + std::to_string(Coons.Surface.Definition->DegreeU) + " × "
                   + std::to_string(Coons.Surface.Definition->DegreeV) : Coons.Refusal, Worst, Worst <= 1.0e-12);

            // Curved boundary loop: four cubics meeting at the corners.
            const Vector3 P = { 0.0, 0.0, 0.0 }, Q = { 1.0, 0.0, 0.1 }, R = { 1.0, 1.0, 0.0 }, S = { 0.0, 1.0, 0.2 };
            const std::vector<NurbsCurve> Bottom = { CubicCurve(P, { 0.3, -0.1, 0.2 }, { 0.7, 0.1, 0.25 }, Q), CubicCurve(S, { 0.3, 1.1, 0.1 }, { 0.6, 0.9, -0.1 }, R) };
            const std::vector<NurbsCurve> Sides  = { CubicCurve(P, { -0.1, 0.4, 0.1 }, { 0.1, 0.7, 0.3 }, S), CubicCurve(Q, { 1.1, 0.3, 0.2 }, { 0.9, 0.6, 0.15 }, R) };
            const NetworkSurfaceOutcome Curved = ConstructGordonSurface(Bottom, Sides, NetworkSettings{});
            const double Boundary = Curved.Refusal.empty() ? std::max(MeasureNetworkInterpolation(Curved), MeasureOriginalGap(Curved, { Bottom[0], Bottom[1], Sides[0], Sides[1] })) : 1.0;
            Report(CoonsGate, "Coons of 4 cubic boundaries: boundaries interpolated", Curved.Refusal.empty() ? "max " + Scientific(Boundary) : Curved.Refusal, Boundary, Boundary <= 1.0e-9);
        }

        // ---- N2 Gordon on iso-curve networks of B-spline surfaces (3 grids).
        struct GridCase { const char* Name; int CountU; int CountV; double Twist; std::vector<double> AtU; std::vector<double> AtV; };
        const std::vector<GridCase> Grids = {
            { "7 × 6 poles, 4 × 4 iso-curves",  7, 6, 0.15, { 0.0, 0.31, 0.68, 1.0 }, { 0.0, 0.27, 0.71, 1.0 } },
            { "9 × 8 poles, 5 × 6 iso-curves",  9, 8, 0.30, { 0.0, 0.2, 0.45, 0.8, 1.0 }, { 0.0, 0.15, 0.4, 0.55, 0.83, 1.0 } },
            { "6 × 6 poles, 3 × 3 iso-curves",  6, 6, -0.2, { 0.0, 0.5, 1.0 }, { 0.0, 0.42, 1.0 } },
        };
        for (const GridCase& Grid : Grids)
        {
            const NurbsSurface Carrier = TestSurface(Grid.CountU, Grid.CountV, Grid.Twist);
            std::vector<NurbsCurve> First, Second;
            for (double V : Grid.AtV) First.push_back(ExtractNurbsIsoCurve(Carrier, false, V));
            for (double U : Grid.AtU) Second.push_back(ExtractNurbsIsoCurve(Carrier, true, U));
            const NetworkSurfaceOutcome Gordon = ConstructGordonSurface(First, Second, NetworkSettings{});
            if (!Gordon.Refusal.empty()) { Report(GordonGate, Grid.Name, Gordon.Refusal, 1.0, false); continue; }
            const double Refit = MeasureNetworkInterpolation(Gordon);
            std::vector<NurbsCurve> Originals = First;
            Originals.insert(Originals.end(), Second.begin(), Second.end());
            const double Original = MeasureOriginalGap(Gordon, Originals);
            Report(GordonGate, Grid.Name, "vs refit " + Scientific(Refit) + " · vs original " + Scientific(Original) + " · reapprox " + Scientific(Gordon.WorstReapproximation) + " · poles "
                   + std::to_string(Gordon.Surface.Definition->CountU) + " × " + std::to_string(Gordon.Surface.Definition->CountV), std::max(Refit, Original), Refit <= 1.0e-9 && Original <= 1.0e-9);

            // ---- N3 the same curves shuffled, half of them reversed, families mixed: auto sorting reproduces the surface.
            std::vector<NurbsCurve> Mixed;
            for (size_t Index = 0; Index < std::max(First.size(), Second.size()); ++Index)
            {
                if (Index < Second.size()) Mixed.push_back(Index % 2 ? ReverseNurbsCurve(*Second[Second.size() - 1 - Index].Definition) : Second[Second.size() - 1 - Index]);
                if (Index < First.size())  Mixed.push_back(Index % 2 ? First[Index] : ReverseNurbsCurve(*First[Index].Definition));
            }
            // Curve 0 of Mixed is a second-family curve: the auto first family is then the 𝑣-curves (transposed surface).
            const NetworkSurfaceOutcome Sorted = ConstructNetworkSurface(Mixed, NetworkSettings{});
            if (!Sorted.Refusal.empty()) { Report(SortingGate, Grid.Name, Sorted.Refusal, 1.0, false); continue; }
            double Difference = 0.0;
            for (int IndexU = 0; IndexU <= 20; ++IndexU)
            {
                for (int IndexV = 0; IndexV <= 20; ++IndexV)
                {
                    const double U = IndexU / 20.0, V = IndexV / 20.0;
                    const Vector3 Reference = SurfacePoint(Gordon.Surface, U, V);
                    // Map (U, V) of the reference into the sorted surface: it may be transposed and / or reversed.
                    double Best = 1.0e300;
                    for (int Variant = 0; Variant < 8; ++Variant)
                    {
                        double A = (Variant & 1) ? 1.0 - U : U, B = (Variant & 2) ? 1.0 - V : V;
                        if (Variant & 4) std::swap(A, B);
                        Best = std::min(Best, Length(SurfacePoint(Sorted.Surface, A, B) - Reference));
                    }
                    Difference = std::max(Difference, Best);
                }
            }
            Report(SortingGate, Grid.Name, "max " + Scientific(Difference) + " · reversed " + std::to_string(Sorted.Reversed) + " · families " + std::to_string(Sorted.FirstOrder.size()) + " + "
                   + std::to_string(Sorted.SecondOrder.size()), Difference, Difference <= 1.0e-12);
        }

        // ---- N4 bonnet network from noisy points.
        {
            const BonnetNetwork Network = ComposeBonnetNetwork(0xB0AA37ULL);
            const NetworkSurfaceOutcome Bonnet = ConstructNetworkSurface(Network.Curves, NetworkSettings{}, Network.Names);
            if (!Bonnet.Refusal.empty()) Report(BonnetNetGate, "Bonnet network", Bonnet.Refusal, 1.0, false);
            else
            {
                double Analytic = 0.0, Points = 0.0;
                for (int IndexU = 0; IndexU <= 60; ++IndexU)
                {
                    for (int IndexV = 0; IndexV <= 60; ++IndexV) Analytic = std::max(Analytic, BonnetGap(SurfacePoint(Bonnet.Surface, IndexU / 60.0, IndexV / 60.0)));
                }
                int Count = 0;
                for (const std::vector<Vector3>& Loop : Network.Points)
                {
                    for (const Vector3& Point : Loop)
                    {
                        const Vector2 Foot = ProjectOntoNurbsSurface(Bonnet.Surface, Point, { 0.5, 0.5 });
                        Points = std::max(Points, Length(SurfacePoint(Bonnet.Surface, Foot.X, Foot.Y) - Point));
                        ++Count;
                    }
                }
                std::string FirstFamily;
                for (int Index : Bonnet.FirstOrder) FirstFamily += (FirstFamily.empty() ? "" : ", ") + Network.Names[Index];
                Report(BonnetNetGate, "vs analytic H (61 × 61)", "max " + Millimetres(Analytic) + " · first family " + FirstFamily, Analytic, Analytic <= PanelTarget);
                Report(BonnetNetGate, "vs all " + std::to_string(Count) + " input points", "max " + Millimetres(Points) + " · crossing gaps ≤ " + Millimetres(Bonnet.WorstGap) + " · curve shift ≤ "
                       + Millimetres(Bonnet.WorstShift) + " · poles " + std::to_string(Bonnet.Surface.Definition->CountU) + " × " + std::to_string(Bonnet.Surface.Definition->CountV), Points, Points <= PanelTarget);
                Report(BonnetNetGate, "Surface interpolates the refit families", "max " + Scientific(MeasureNetworkInterpolation(Bonnet)), MeasureNetworkInterpolation(Bonnet) * 1.0e-3,
                       MeasureNetworkInterpolation(Bonnet) <= 1.0e-9);
            }

            // ---- N5 refusals.
            struct RefusalCase { std::string Name; std::vector<NurbsCurve> Curves; std::vector<std::string> Names; std::string Expected; };
            std::vector<RefusalCase> Cases;
            {
                // Section3 bent up towards its outer end (10 mm at x = 0.75): it still meets Spine1, not the outer spines.
                RefusalCase Case{ "Section3 bent 10 mm at its outer end", Network.Curves, Network.Names, "Section3 and Spine" };
                Case.Curves[2] = MapNurbsCurve(*Network.Curves[2].Definition, [](Vector3 Point)
                {
                    const double Bend = std::max(0.0, (Point.X - 0.4) / 0.35);
                    return Point + Vector3{ 0.0, 0.0, 0.01 * Bend * Bend };
                });
                Cases.push_back(Case);
            }
            {
                // Spine2 shortened: it stops 60 mm before the last section (open network).
                RefusalCase Case{ "Spine2 stops short of Section6", Network.Curves, Network.Names, "do not meet" };
                std::mt19937_64 Generator(0x5B1E2ULL);
                std::vector<Vector3> Points;
                for (double Station : IrregularStations(28)) { const double X = 0.75 * 0.35, Y = 1.04 * Station; Points.push_back(Vector3{ X, Y, BonnetHeight(X, Y) }); }
                Case.Curves[7] = FitSpaceCurve(Points, CurveFitSettings{}).Curve;
                Cases.push_back(Case);
            }
            {
                // Spine3 runs 50 mm past Section6.
                RefusalCase Case{ "Spine3 runs past Section6", Network.Curves, Network.Names, "past the boundary curve" };
                std::vector<Vector3> Points;
                for (double Station : IrregularStations(30)) { const double X = 0.75 * 0.7, Y = 1.15 * Station; Points.push_back(Vector3{ X, Y, BonnetHeight(X, Y) }); }
                Case.Curves[8] = FitSpaceCurve(Points, CurveFitSettings{}).Curve;
                Cases.push_back(Case);
            }
            {
                // A diagonal curve meeting both families: odd cycle.
                RefusalCase Case{ "Diagonal curve (meets both families)", Network.Curves, Network.Names, "same family" };
                std::vector<Vector3> Points;
                for (int Index = 0; Index <= 30; ++Index) { const double S = Index / 30.0, X = 0.75 * S, Y = 1.1 * S; Points.push_back(Vector3{ X, Y, BonnetHeight(X, Y) }); }
                Case.Curves.push_back(FitSpaceCurve(Points, CurveFitSettings{}).Curve);
                Case.Names.push_back("Diagonal");
                Cases.push_back(Case);
            }
            {
                RefusalCase Case{ "A stray curve meeting nothing", Network.Curves, Network.Names, "meets no other curve" };
                Case.Curves.push_back(LineCurve({ 2.0, 0.0, 0.0 }, { 2.0, 1.0, 0.0 }));
                Case.Names.push_back("Stray");
                Cases.push_back(Case);
            }
            Cases.push_back({ "Three curves", { Network.Curves[0], Network.Curves[6], Network.Curves[7] }, { "Section1", "Spine1", "Spine2" }, "at least 4 curves" });
            for (const RefusalCase& Case : Cases)
            {
                const NetworkSurfaceOutcome Outcome = ConstructNetworkSurface(Case.Curves, NetworkSettings{}, Case.Names);
                const bool Named = Outcome.Refusal.find(Case.Expected) != std::string::npos;
                Report(NetworkRefuseGate, Case.Name, Outcome.Refusal.empty() ? "(built — expected a refusal)" : Outcome.Refusal, Named ? 0.0 : 1.0, Named);
            }

            // ---- N6 sheet body and determinism.
            ModelPartition Partition;
            const NetworkSheetOutcome Sheet = ConstructNetworkSheet(Partition, Network.Curves, NetworkSettings{}, Network.Names);
            if (!Sheet.Sheet.Refusal.empty()) Report(NetworkSheetGate, "Network sheet", Sheet.Sheet.Refusal, 1.0, false);
            else
            {
                const AuditReport Audit = AuditBody(Partition, Sheet.Sheet.Body);
                const BodyInventory Inventory = Partition.CollectInventory(Sheet.Sheet.Body);
                const bool Sound = Audit.SoundCondition() && Inventory.Faces.size() == 1 && Inventory.Edges.size() == 4;
                Report(NetworkSheetGate, "Network sheet body", std::string(Audit.SoundCondition() ? "sound" : "UNSOUND: " + std::string(Audit.Findings.front().Code) + " " + Audit.Findings.front().Detail) + " · " + std::to_string(Inventory.Faces.size()) + " face · "
                       + std::to_string(Inventory.Edges.size()) + " edges", Sound ? 0.0 : 1.0, Sound);
            }
            const BonnetNetwork Again = ComposeBonnetNetwork(0xB0AA37ULL);
            const NetworkSurfaceOutcome Repeat = ConstructNetworkSurface(Again.Curves, NetworkSettings{}, Again.Names);
            const bool Equal = Bonnet.Refusal.empty() && Repeat.Refusal.empty() && Bonnet.Surface.Definition->Poles.size() == Repeat.Surface.Definition->Poles.size()
                            && std::equal(Bonnet.Surface.Definition->Poles.begin(), Bonnet.Surface.Definition->Poles.end(), Repeat.Surface.Definition->Poles.begin(),
                                          [](Vector3 A, Vector3 B) { return A.X == B.X && A.Y == B.Y && A.Z == B.Z; });
            Report(NetworkSheetGate, "Same input twice → bit-identical network poles", Equal ? "equal" : "DIFFERENT", Equal ? 0.0 : 1.0, Equal);
        }
    }
}

namespace
{
    //--------------------------------------------------------------------------------------------------------------------
    //                                                V6 · CONTINUITY
    //--------------------------------------------------------------------------------------------------------------------

    // 📝 Normal curvature of a surface at (𝑢, 𝑣) in tangent direction W: II(W) / I(W), with W written in (S𝑢, S𝑣) and the
    //    normal oriented along Up.
    [[nodiscard]] double NormalCurvature(const NurbsSurface& Surface, Vector2 Parameter, Vector3 W, Vector3 Up)
    {
        const NurbsSurfaceDerivatives D = EvaluateNurbsSurfaceDerivatives(*Surface.Definition, Parameter);
        Vector3 Normal = Cross(D.U, D.V);
        Normal = Normal * ((Dot(Normal, Up) < 0.0 ? -1.0 : 1.0) / Length(Normal));
        const double E = Dot(D.U, D.U), F = Dot(D.U, D.V), G = Dot(D.V, D.V);
        const double Determinant = E * G - F * F;
        const double WU = Dot(W, D.U), WV = Dot(W, D.V);
        const double A = (G * WU - F * WV) / Determinant, B = (E * WV - F * WU) / Determinant;
        const double Second = A * A * Dot(D.UU, Normal) + 2.0 * A * B * Dot(D.UV, Normal) + B * B * Dot(D.VV, Normal);
        const double First  = A * A * E + 2.0 * A * B * F + B * B * G;
        return Second / First;
    }

    [[nodiscard]] Vector3 UnitNormal(const NurbsSurface& Surface, Vector2 Parameter)
    {
        const NurbsSurfaceDerivatives D = EvaluateNurbsSurfaceDerivatives(*Surface.Definition, Parameter);
        const Vector3 Normal = Cross(D.U, D.V);
        return Normal * (1.0 / Length(Normal));
    }

    struct SeamMeasure
    {
        double Position  = 0.0;                                                              // [len] - Largest gap between the two sides
        double Angle     = 0.0;                                                              // [rad] - Largest normal angle
        double Curvature = 0.0;                                                              // [-]   - Largest relative cross-curvature jump
    };

    // 📝 Along a shared boundary: A at (𝑢, VA) and B at (𝑢 or 1 − 𝑢, VB). Normals compared up to orientation; the cross
    //    direction is N × T (T the edge tangent), the same tangent direction on both sides.
    [[nodiscard]] SeamMeasure MeasureSeam(const NurbsSurface& A, double VA, const NurbsSurface& B, double VB, bool ReversedCondition)
    {
        SeamMeasure Measure;
        for (int Sample = 0; Sample <= 100; ++Sample)
        {
            const double U = Sample / 100.0, UB = ReversedCondition ? 1.0 - U : U;
            const NurbsSurfaceDerivatives DA = EvaluateNurbsSurfaceDerivatives(*A.Definition, { U, VA });
            Measure.Position = std::max(Measure.Position, Length(DA.Position - SurfacePoint(B, UB, VB)));
            const Vector3 NormalA = UnitNormal(A, { U, VA });
            Vector3 NormalB = UnitNormal(B, { UB, VB });
            if (Dot(NormalA, NormalB) < 0.0) NormalB = NormalB * -1.0;
            Measure.Angle = std::max(Measure.Angle, std::atan2(Length(Cross(NormalA, NormalB)), Dot(NormalA, NormalB)));
            const Vector3 Tangent = DA.U * (1.0 / Length(DA.U));
            const Vector3 Across  = Cross(NormalA, Tangent);
            const double  KappaA  = NormalCurvature(A, { U, VA }, Across, NormalA), KappaB = NormalCurvature(B, { UB, VB }, Across, NormalA);
            Measure.Curvature = std::max(Measure.Curvature, std::abs(KappaA - KappaB) / std::max({ std::abs(KappaA), std::abs(KappaB), 1.0e-2 }));
        }
        return Measure;
    }

    [[nodiscard]] std::vector<NurbsCurve> BonnetSections(uint64_t Seed)
    {
        std::mt19937_64 Generator(Seed);
        std::vector<NurbsCurve> Sections;
        for (double η : BonnetSectionStations) Sections.push_back(FitSpaceCurve(BonnetSectionPoints(η, Generator), CurveFitSettings{}).Curve);
        return Sections;
    }

    [[nodiscard]] double MeasureSectionInterpolation(const LoftSurfaceOutcome& Loft, const std::vector<NurbsCurve>& Originals)
    {
        double Worst = 0.0;
        for (size_t Section = 0; Section < Originals.size(); ++Section)
        {
            for (int Sample = 0; Sample <= 100; ++Sample)
            {
                const double U = Sample / 100.0;
                const Vector3 Point = CurvePoint(Originals[Section], U);
                const double  V = Loft.SectionParameters[Section];
                // Sections may run reversed in the loft (alignment): compare against the nearer end.
                Worst = std::max(Worst, std::min(Length(SurfacePoint(Loft.Surface, U, V) - Point), Length(SurfacePoint(Loft.Surface, 1.0 - U, V) - Point)));
            }
        }
        return Worst;
    }

    void ValidateContinuity()
    {
        const std::vector<NurbsCurve> Sections = BonnetSections(0xB0AA37ULL);
        const std::vector<NurbsCurve> FrontSections(Sections.begin(), Sections.begin() + 3), RearSections(Sections.begin() + 2, Sections.end());

        // Four side combinations: the front panel built forward or backward, the rear panel's shared section first or last.
        struct Combination { const char* Name; bool FrontBackward; bool RearBackward; };
        const Combination Combinations[] = { { "front 1→3 · rear 3→6", false, false }, { "front 3→1 · rear 3→6", true, false },
                                             { "front 1→3 · rear 6→3", false, true }, { "front 3→1 · rear 6→3", true, true } };
        for (const Combination& Case : Combinations)
        {
            std::vector<NurbsCurve> Front = FrontSections, Rear = RearSections;
            if (Case.FrontBackward) std::reverse(Front.begin(), Front.end());
            if (Case.RearBackward)  std::reverse(Rear.begin(), Rear.end());
            const LoftSurfaceOutcome FrontLoft = LoftSectionSurface(Front, LoftSettings{});
            if (!FrontLoft.Refusal.empty()) { Report(TangentGate, Case.Name, FrontLoft.Refusal, 1.0, false); continue; }
            const double FrontRow = Case.FrontBackward ? 0.0 : 1.0;
            std::array<SeamMeasure, 3> Measures;
            std::array<LoftSurfaceOutcome, 3> RearLofts;
            for (int Order = 0; Order <= 2; ++Order)
            {
                LoftSettings Settings;
                LoftContinuity& Continuity = Case.RearBackward ? Settings.End : Settings.Start;
                Continuity.Order     = Order;
                Continuity.Reference = FrontLoft.Surface;
                RearLofts[Order] = LoftSectionSurface(Rear, Settings);
                if (!RearLofts[Order].Refusal.empty()) { Report(Order == 2 ? CurvatureGate : TangentGate, std::string(Case.Name) + " G" + std::to_string(Order), RearLofts[Order].Refusal, 1.0, false); continue; }
                // The front's shared row and the rear's: same 𝑢 direction (the rear aligns its sections with section 3 or 6).
                const double RearRow = Case.RearBackward ? 1.0 : 0.0;
                const bool Reversed = Dot(SurfacePoint(FrontLoft.Surface, 1.0, FrontRow) - SurfacePoint(FrontLoft.Surface, 0.0, FrontRow),
                                          SurfacePoint(RearLofts[Order].Surface, 1.0, RearRow) - SurfacePoint(RearLofts[Order].Surface, 0.0, RearRow)) < 0.0;
                Measures[Order] = MeasureSeam(FrontLoft.Surface, FrontRow, RearLofts[Order].Surface, RearRow, Reversed);
            }
            if (!RearLofts[1].Refusal.empty() || !RearLofts[2].Refusal.empty() || !RearLofts[0].Refusal.empty()) continue;
            Report(TangentGate, std::string(Case.Name) + " · G1", "angle " + Scientific(Measures[1].Angle) + " rad · gap " + Scientific(Measures[1].Position) + " · α " + std::to_string(RearLofts[1].StartScale + RearLofts[1].EndScale).substr(0, 5),
                   Measures[1].Angle, Measures[1].Angle <= 1.0e-8 && Measures[1].Position <= 1.0e-12);
            Report(CurvatureGate, std::string(Case.Name) + " · G2", "angle " + Scientific(Measures[2].Angle) + " rad · curvature jump " + Scientific(Measures[2].Curvature) + " · gap " + Scientific(Measures[2].Position),
                   std::max(Measures[2].Angle, Measures[2].Curvature), Measures[2].Angle <= 1.0e-8 && Measures[2].Curvature <= 1.0e-6 && Measures[2].Position <= 1.0e-12);
            Report(DiscriminateGate, std::string(Case.Name) + " · G0 / G1 differ", "G0 angle " + Scientific(Measures[0].Angle) + " rad · G1 curvature jump " + Scientific(Measures[1].Curvature),
                   0.0, Measures[0].Angle > 1.0e-5 && Measures[1].Curvature > 1.0e-3);
            double Interpolation = 0.0;
            for (int Order = 1; Order <= 2; ++Order) Interpolation = std::max(Interpolation, MeasureSectionInterpolation(RearLofts[Order], Rear));
            Report(SharedRowGate, std::string(Case.Name) + " · sections kept", "G1 / G2 vs every rear section " + Scientific(Interpolation), Interpolation, Interpolation <= 1.0e-9);
        }

        // Shared rows bit-identical: the rear's first pole row is the front's last (refined to the same knots).
        {
            const LoftSurfaceOutcome FrontLoft = LoftSectionSurface(FrontSections, LoftSettings{});
            LoftSettings Settings;
            Settings.Start = { 2, FrontLoft.Surface };
            const LoftSurfaceOutcome RearLoft = LoftSectionSurface(RearSections, Settings);
            const NurbsSurfaceDefinition Refined = RefineNurbsSurfaceKnots(*FrontLoft.Surface.Definition,
                                                                           MissingInteriorKnots(FrontLoft.Surface.Definition->KnotsU, RearLoft.Surface.Definition->KnotsU, 3), {});
            const NurbsCurveDefinition FrontRow = ExtractPoleRow(Refined, Refined.CountV - 1), RearRow = ExtractPoleRow(*RearLoft.Surface.Definition, 0);
            bool Equal = FrontRow.Poles.size() == RearRow.Poles.size();
            for (size_t Index = 0; Equal && Index < FrontRow.Poles.size(); ++Index) Equal = FrontRow.Poles[Index].X == RearRow.Poles[Index].X && FrontRow.Poles[Index].Y == RearRow.Poles[Index].Y && FrontRow.Poles[Index].Z == RearRow.Poles[Index].Z;
            Report(SharedRowGate, "Rear first pole row ≡ front last row (refined)", Equal ? "bit-identical (" + std::to_string(RearRow.Poles.size()) + " poles)" : "DIFFERENT", Equal ? 0.0 : 1.0, Equal);
        }

        // ---- C5 mixed degrees.
        {
            const NurbsCurve Line      = LineCurve({ 0.0, 0.0, 0.0 }, { 1.0, 0.0, 0.1 });
            const NurbsCurve Quadratic = *ConstructNurbsCurve(2, { 0.0, 0.0, 0.0, 1.0, 1.0, 1.0 }, { { 0.0, 0.5, 0.1 }, { 0.5, 0.45, 0.3 }, { 1.0, 0.5, 0.0 } }, { 1.0, 1.0, 1.0 });
            const NurbsCurve Cubic     = CubicCurve({ 0.0, 1.0, 0.0 }, { 0.3, 1.1, 0.2 }, { 0.7, 0.95, 0.25 }, { 1.0, 1.0, 0.05 });
            const LoftSurfaceOutcome Mixed = LoftSectionSurface({ Line, Quadratic, Cubic }, LoftSettings{});
            const double Interpolation = Mixed.Refusal.empty() ? MeasureSectionInterpolation(Mixed, { Line, Quadratic, Cubic }) : 1.0;
            Report(ElevationGate, "Line + quadratic + cubic", Mixed.Refusal.empty() ? "sections " + Scientific(Interpolation) + " · elevated " + std::to_string(Mixed.Elevated) + " · degree "
                   + std::to_string(Mixed.Surface.Definition->DegreeU) : Mixed.Refusal, Interpolation, Interpolation <= 1.0e-9 && Mixed.Elevated == 2);
            // Elevation itself is exact: a cubic raised to 5 matches at 201 points.
            const std::optional<NurbsCurveDefinition> Raised = ElevateNurbsCurveDegree(*Sections[2].Definition, 5);
            double Worst = Raised ? 0.0 : 1.0;
            for (int Sample = 0; Raised && Sample <= 200; ++Sample) Worst = std::max(Worst, Length(EvaluateNurbsCurveDerivatives(*Raised, Sample / 200.0).Position - CurvePoint(Sections[2], Sample / 200.0)));
            Report(ElevationGate, "Fitted section raised 3 → 5", "max " + Scientific(Worst), Worst, Worst <= 1.0e-12);
        }

        // ---- C6 refusals.
        {
            const LoftSurfaceOutcome FrontLoft = LoftSectionSurface(FrontSections, LoftSettings{});
            auto Expect = [&](const std::string& Name, const std::string& Refusal, const std::string& Expected)
            {
                const bool Named = Refusal.find(Expected) != std::string::npos;
                Report(ContinuityRefuseGate, Name, Refusal.empty() ? "(built — expected a refusal)" : Refusal, Named ? 0.0 : 1.0, Named);
            };
            LoftSettings Away;
            Away.Start = { 1, FrontLoft.Surface };
            Expect("Reference that does not carry the section", LoftSectionSurface({ Sections[3], Sections[4], Sections[5] }, Away).Refusal, "is not a boundary");
            const NurbsCurve Circle = ConvertConicToNurbs({ { 0.0, 0.0, 0.0 }, { 1.0, 0.0, 0.0 }, { 0.0, 1.0, 0.0 }, { 0.0, 0.0, 1.0 } }, 0.0, std::numbers::pi);
            const NurbsCurve Higher = ConvertConicToNurbs({ { 0.0, 0.0, 1.0 }, { 1.0, 0.0, 0.0 }, { 0.0, 1.0, 0.0 }, { 0.0, 0.0, 1.0 } }, 0.0, std::numbers::pi);
            const NurbsCurve Top    = ConvertConicToNurbs({ { 0.0, 0.0, 2.0 }, { 1.0, 0.0, 0.0 }, { 0.0, 1.0, 0.0 }, { 0.0, 0.0, 1.0 } }, 0.0, std::numbers::pi);
            const LoftSurfaceOutcome HalfTube = LoftSectionSurface({ Circle, Higher }, LoftSettings{});
            LoftSettings Rational;
            Rational.Start = { 1, HalfTube.Surface };
            Expect("Rational reference (half cylinder)", LoftSectionSurface({ Higher, Top }, Rational).Refusal, "rational reference");
            LoftSettings Third;
            Third.Start = { 3, FrontLoft.Surface };
            Expect("G3 requested", LoftSectionSurface(RearSections, Third).Refusal, "only G0 / G1 / G2");
            const NurbsCurve Ring = ConvertConicToNurbs({ { 0.0, 0.0, 0.0 }, { 1.0, 0.0, 0.0 }, { 0.0, 1.0, 0.0 }, { 0.0, 0.0, 1.0 } }, 0.0, 2.0 * std::numbers::pi);
            const NurbsCurve Ring2 = ConvertConicToNurbs({ { 0.0, 0.0, 1.0 }, { 1.0, 0.0, 0.0 }, { 0.0, 1.0, 0.0 }, { 0.0, 0.0, 1.0 } }, 0.0, 2.0 * std::numbers::pi);
            LoftSettings Closed;
            Closed.Start = { 1, FrontLoft.Surface };
            Expect("Continuity on closed sections", LoftSectionSurface({ Ring, Ring2 }, Closed).Refusal, "closed sections");
        }
    }
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    ENTRY
//------------------------------------------------------------------------------------------------------------------------

int main(int Count, char** Arguments)
{
    std::string Only = Count > 2 && std::string(Arguments[1]) == "--only" ? Arguments[2] : "";
    if (Only.empty() || Only == "networks")   ValidateNetworks();
    if (Only.empty() || Only == "continuity") ValidateContinuity();

    bool PassedCondition = true;
    for (const GateTally* Gate : AllGates()) PassedCondition = PassedCondition && Gate->Failures == 0 && (Gate->Checks > 0 || !Only.empty());
    std::printf("# Panel surfacing validation\n\n%s Vehicle plan V5–V7: curve networks, G1 / G2 across shared sections, sewing → mirror → caps → solid\n\n", PassedCondition ? "🟢" : "🔴");
    std::printf("## Gates\n\n| | Gate | Measures | Limit | Worst | Checks | Failures |\n|---|---|---|---|---|---|---|\n");
    for (const GateTally* Gate : AllGates())
    {
        std::printf("| %s | %s | %s | %s | %.2e | %ld | %ld |\n", Gate->Failures == 0 && Gate->Checks > 0 ? "🟢" : "🔴", Gate->Code, Gate->Subject, Gate->Limit, Gate->Worst, Gate->Checks, Gate->Failures);
    }
    std::printf("\n## Checks\n\n| | Gate | Check | Detail |\n|---|---|---|---|\n");
    for (const DetailRow& Row : Details) std::printf("| %s | %s | %s | %s |\n", Row.PassedCondition ? "🟢" : "🔴", Row.Gate.c_str(), Row.Name.c_str(), Row.Detail.c_str());
    return PassedCondition ? 0 : 1;
}
