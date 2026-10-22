/*====================================================================================================================================
                                                  FREEFORMBLENDVALIDATION.CPP
====================================================================================================================================*/
// 🧩 Priority 10: exact offsets of NURBS surfaces (parallel carrier) and rolling-ball fillets / chamfers on NURBS supports

//------------------------------------------------------------------------------------------------------------------------
//                                                    INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Geometry/NurbsCarrier.h"
#include "Geometry/ParallelCarrier.h"
#include "Geometry/SurfaceGeometry.h"
#include "Intersection/ImplicitSurface.h"
#include "Intersection/SurfaceIntersection.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <functional>
#include <numbers>
#include <optional>
#include <string>
#include <vector>

using namespace Frontier::SolidArcRemastered;

//------------------------------------------------------------------------------------------------------------------------
//                                                    INTERNAL
//------------------------------------------------------------------------------------------------------------------------

namespace
{
    constexpr double Pi = std::numbers::pi;

    struct GateTally
    {
        const char* Code     = "";                                                           // [-] - P1 …
        const char* Subject  = "";                                                           // [-] - What it measures
        const char* Limit    = "";                                                           // [-] - Acceptance rule
        double      Worst    = 0.0;                                                          // [-] - Largest measured value
        long        Checks   = 0;                                                            // [-]
        long        Failures = 0;                                                            // [-]
    };

    GateTally CarrierGate  = { "P1", "Parallel carrier = closed form: offsets of NURBS cylinder / sphere / torus are the analytic offsets (position, O𝑢 = (1 − dκ)·S𝑢, normal), peak curvature κ/(1 − dκ)", "≤ 10⁻¹²" };
    GateTally FreeformGate = { "P2", "Freeform bicubic patch: O𝑢, O𝑣 vs central differences, |O𝑢 × O𝑣| = (1 − dκ₁)(1 − dκ₂)·|n|, the normal is the basis normal, inversion and signed distance of points off the offset", "≤ 10⁻⁷ (differences) · ≤ 10⁻¹⁰" };
    GateTally SectionGate  = { "P3", "Intersections with parallel carriers: offset NURBS sphere ∩ plane = the circle, offset cylinder ∩ offset cylinder (Steinmetz), offset freeform ∩ plane — every chart point on both exact carriers", "interior ≤ 10⁻⁹ · open ends ≤ 10⁻⁷" };
    GateTally FoldGate     = { "P4", "Fold margin min(1 − dκᵢ): inward offsets of a unit NURBS cylinder by 0.5 (margin ½) and 1.2 (folds); offset of an offset adds distances; d = 0 returns the basis", "exact" };

    std::vector<GateTally*> AllGates() { return { &CarrierGate, &FreeformGate, &SectionGate, &FoldGate }; }

    struct DetailRow
    {
        std::string Gate            = {};
        std::string Name            = {};
        std::string Detail          = {};
        bool        PassedCondition = false;
    };
    std::vector<DetailRow> Details;

    void Report(GateTally& Gate, const std::string& Name, const std::string& Detail, double Value, bool PassedCondition)
    {
        Gate.Worst = std::max(Gate.Worst, Value);
        ++Gate.Checks;
        if (!PassedCondition) ++Gate.Failures;
        Details.push_back({ Gate.Code, Name, Detail, PassedCondition });
        std::fprintf(stderr, "%s %s %s\n", PassedCondition ? "🟢" : "🔴", Gate.Code, Name.c_str());
    }

    [[nodiscard]] std::string Scientific(double Value)
    {
        char Text[32];
        std::snprintf(Text, sizeof(Text), "%.2e", Value);
        return Text;
    }

    [[nodiscard]] OrthonormalFrame Upright(Vector3 Origin) { return { Origin, { 1.0, 0.0, 0.0 }, { 0.0, 1.0, 0.0 }, { 0.0, 0.0, 1.0 } }; }

    // 📝 Bicubic freeform test patch over [0, 2]² with two knot spans each way: a saddle-and-bump height field
    //    z = 0.3·sin(x)·cos(y) + 0.1·x·y sampled into the poles (non-rational, so every curvature is genuinely freeform).
    [[nodiscard]] NurbsSurface FreeformPatch()
    {
        const int Count = 5;
        std::vector<Vector3> Poles;
        std::vector<double>  Weights;
        for (int Row = 0; Row < Count; ++Row)
            for (int Column = 0; Column < Count; ++Column)
            {
                const double X = 2.0 * Row / (Count - 1), Y = 2.0 * Column / (Count - 1);
                Poles.push_back({ X, Y, 0.3 * std::sin(X) * std::cos(Y) + 0.1 * X * Y });
                Weights.push_back(1.0);
            }
        const std::vector<double> Knots = { 0.0, 0.0, 0.0, 0.0, 1.0, 2.0, 2.0, 2.0, 2.0 };
        return *ConstructNurbsSurface(3, 3, Count, Count, Knots, Knots, Poles, Weights);
    }

    // Exact residual of a point against an offset: |X − S(foot)| − |d| along N, plus the off-normal part.
    [[nodiscard]] double OffsetResidual(const ParallelSurface& Offset, Vector3 Position, Vector2 Reference)
    {
        const Vector2 Foot   = ProjectOntoNurbsSurface(Offset.Definition->Basis, Position, Reference);
        const Vector3 Basis  = EvaluateNurbsSurface(Offset.Definition->Basis, Foot).Position;
        const Vector3 Normal = EvaluateNurbsNormal(Offset.Definition->Basis, Foot);
        const Vector3 Gap    = Position - Basis;
        return std::max(std::fabs(Dot(Gap, Normal) - Offset.Definition->Distance), Length(Gap - Normal * Dot(Gap, Normal)));
    }

    // ---- P1 closed forms ---------------------------------------------------------------------------------------------
    void ValidateClosedForms()
    {
        struct Case { const char* Name; SurfaceGeometry Analytic; Vector2 Low, High; double Distance; std::function<double(Vector3)> Residual; };
        const OrthonormalFrame Frame = Upright({ 0.3, -0.2, 0.1 });
        const auto Radial = [Frame](Vector3 Point) { const Vector3 L = Point - Frame.Origin; return std::hypot(L.X, L.Y); };
        const std::vector<Case> Cases = {
            { "NURBS cylinder r 1 offset +0.25 → r 1.25", CylinderSurface{ Frame, 1.0 }, { 0.0, -1.0 }, { 2.0 * Pi, 1.0 }, 0.25,
              [&](Vector3 Point) { return std::fabs(Radial(Point) - 1.25); } },
            { "NURBS cylinder r 1 offset −0.4 → r 0.6", CylinderSurface{ Frame, 1.0 }, { 0.0, -1.0 }, { 2.0 * Pi, 1.0 }, -0.4,
              [&](Vector3 Point) { return std::fabs(Radial(Point) - 0.6); } },
            { "NURBS sphere R 2 offset +0.5 → R 2.5", SphereSurface{ Frame, 2.0 }, { 0.0, -1.2 }, { 2.0 * Pi, 1.2 }, 0.5,
              [&](Vector3 Point) { return std::fabs(Distance(Point, Frame.Origin) - 2.5); } },
            { "NURBS torus R 3 r 1 offset −0.3 → tube 0.7", TorusSurface{ Frame, 3.0, 1.0 }, { 0.0, 0.0 }, { 2.0 * Pi, 2.0 * Pi }, -0.3,
              [&](Vector3 Point) { const Vector3 L = Point - Frame.Origin; return std::fabs(std::hypot(std::hypot(L.X, L.Y) - 3.0, L.Z) - 0.7); } },
        };
        for (const Case& Each : Cases)
        {
            const std::optional<NurbsSurface> Basis = ConvertSurfaceToNurbs(Each.Analytic, Each.Low, Each.High);
            if (!Basis) { Report(CarrierGate, Each.Name, "conversion failed", 1.0, false); continue; }
            const std::optional<SurfaceGeometry> Offset = OffsetSurface(SurfaceGeometry{ *Basis }, Each.Distance);
            const auto* Parallel = Offset ? std::get_if<ParallelSurface>(&*Offset) : nullptr;
            if (Parallel == nullptr) { Report(CarrierGate, Each.Name, "no parallel carrier", 1.0, false); continue; }
            const NurbsDomain U = MeasureNurbsSurfaceDomain(*Basis->Definition, true), V = MeasureNurbsSurfaceDomain(*Basis->Definition, false);
            double PositionError = 0.0, DerivativeError = 0.0, NormalError = 0.0, CurvatureError = 0.0;
            for (int Row = 1; Row < 12; ++Row)
                for (int Column = 1; Column < 12; ++Column)
                {
                    const Vector2 At{ U.Low + (U.High - U.Low) * Column / 12.0, V.Low + (V.High - V.Low) * Row / 12.0 };
                    const SurfaceSample Sample = EvaluateSurface(*Offset, At);
                    const NurbsSurfaceDerivatives S = EvaluateNurbsSurfaceDerivatives(*Basis->Definition, At);
                    PositionError = std::max(PositionError, Each.Residual(Sample.Position));
                    // 📝 Principal directions of these carriers are the chart directions: O𝑢 = (1 − dκ₁)·S𝑢, O𝑣 = (1 − dκ₂)·S𝑣
                    //    (cylinder: κ = −1/r around, 0 along; sphere −1/R both; torus tube −1/r along 𝑣, ring along 𝑢).
                    std::array<double, 2> κ{};
                    (void)MeasureNurbsPrincipalCurvatures(*Basis->Definition, At, κ);
                    const Vector3 Normal = EvaluateNurbsNormal(*Basis, At);
                    // Which principal curvature belongs to S𝑢: the normal curvature of S𝑢 is L / E.
                    const double κU = Dot(S.UU, Normal) / Dot(S.U, S.U), κV = Dot(S.VV, Normal) / Dot(S.V, S.V);
                    DerivativeError = std::max({ DerivativeError, Length(Sample.DerivativeU - S.U * (1.0 - Each.Distance * κU)) / Length(S.U),
                                                 Length(Sample.DerivativeV - S.V * (1.0 - Each.Distance * κV)) / Length(S.V) });
                    NormalError = std::max(NormalError, Length(EvaluateSurfaceNormal(*Offset, At) - Normal));
                    const double Expected = std::max(std::fabs(κ[0] / (1.0 - Each.Distance * κ[0])), std::fabs(κ[1] / (1.0 - Each.Distance * κ[1])));
                    CurvatureError = std::max(CurvatureError, std::fabs(MeasureParallelCurvature(*Parallel, At) - Expected) / std::max(1.0, Expected));
                    if (std::holds_alternative<CylinderSurface>(Each.Analytic) || std::holds_alternative<SphereSurface>(Each.Analytic))
                    {
                        const double Analytic = 1.0 / (std::holds_alternative<CylinderSurface>(Each.Analytic) ? 1.0 + Each.Distance : 2.0 + Each.Distance);
                        CurvatureError = std::max(CurvatureError, std::fabs(MeasureParallelCurvature(*Parallel, At) - Analytic));
                    }
                }
            const double Worst = std::max({ PositionError, DerivativeError, NormalError, CurvatureError });
            Report(CarrierGate, Each.Name, "position " + Scientific(PositionError) + " · O𝑢 / O𝑣 " + Scientific(DerivativeError) + " · normal " + Scientific(NormalError) + " · curvature " + Scientific(CurvatureError) + " (121 points)",
                   Worst, Worst <= 1.0e-12);
        }
    }

    // ---- P2 freeform ------------------------------------------------------------------------------------------------
    void ValidateFreeform()
    {
        const NurbsSurface Patch = FreeformPatch();
        for (const double d : { 0.15, -0.2 })
        {
            const ParallelSurface Offset = ConstructParallelSurface(Patch, d);
            double DifferenceError = 0.0, AreaError = 0.0, NormalError = 0.0, InversionError = 0.0, DistanceError = 0.0;
            constexpr double h = 1.0e-5;                                                         // [-] - Central-difference step
            for (int Row = 1; Row < 10; ++Row)
                for (int Column = 1; Column < 10; ++Column)
                {
                    const Vector2 At{ 2.0 * (Column + 0.37) / 10.5, 2.0 * (Row + 0.37) / 10.5 };              // Off the knot at 1
                    const SurfaceSample Sample = EvaluateParallelSurface(Offset, At);
                    const Vector3 DifferenceU = (EvaluateParallelSurface(Offset, { At.X + h, At.Y }).Position - EvaluateParallelSurface(Offset, { At.X - h, At.Y }).Position) * (0.5 / h);
                    const Vector3 DifferenceV = (EvaluateParallelSurface(Offset, { At.X, At.Y + h }).Position - EvaluateParallelSurface(Offset, { At.X, At.Y - h }).Position) * (0.5 / h);
                    DifferenceError = std::max({ DifferenceError, Length(DifferenceU - Sample.DerivativeU) / Length(Sample.DerivativeU), Length(DifferenceV - Sample.DerivativeV) / Length(Sample.DerivativeV) });
                    const NurbsSurfaceDerivatives S = EvaluateNurbsSurfaceDerivatives(*Patch.Definition, At);
                    std::array<double, 2> κ{};
                    (void)MeasureNurbsPrincipalCurvatures(*Patch.Definition, At, κ);
                    const double Expected = (1.0 - d * κ[0]) * (1.0 - d * κ[1]) * Length(Cross(S.U, S.V));
                    AreaError = std::max(AreaError, std::fabs(Length(Cross(Sample.DerivativeU, Sample.DerivativeV)) - Expected) / Expected);
                    NormalError = std::max(NormalError, Length(Normalize(Cross(Sample.DerivativeU, Sample.DerivativeV)) - EvaluateParallelNormal(Offset, At)));
                    // A point t = 0.03 off the offset along N: the foot is At and the signed distance is t.
                    const Vector3 Off = Sample.Position + EvaluateParallelNormal(Offset, At) * 0.03;
                    const Vector2 Foot = ProjectOntoSurface(SurfaceGeometry{ Offset }, Off, { 1.0, 1.0 });
                    InversionError = std::max(InversionError, Length(Foot - At));
                    DistanceError = std::max(DistanceError, std::fabs(MeasureSurfaceDistance(SurfaceGeometry{ Offset }, Off).Distance - 0.03));
                }
            const double Exact = std::max({ AreaError, NormalError, InversionError, DistanceError });
            Report(FreeformGate, "bicubic patch offset " + Scientific(d), "O𝑢 / O𝑣 vs differences " + Scientific(DifferenceError) + " · |O𝑢 × O𝑣| " + Scientific(AreaError) + " · normal " + Scientific(NormalError)
                   + " · foot " + Scientific(InversionError) + " · signed distance " + Scientific(DistanceError) + " (81 points)", std::max(DifferenceError * 1.0e-3, Exact), DifferenceError <= 1.0e-7 && Exact <= 1.0e-10);
        }
    }

    // ---- P3 intersections ---------------------------------------------------------------------------------------------
    void ValidateSections()
    {
        const auto Box = [](const SurfaceGeometry& Surface) -> ParameterBox
        {
            const NurbsSurface* Chart = ResolveNurbsChart(Surface);
            if (Chart == nullptr) return { { -6.0, -6.0 }, { 6.0, 6.0 } };
            const NurbsDomain U = MeasureNurbsSurfaceDomain(*Chart->Definition, true), V = MeasureNurbsSurfaceDomain(*Chart->Definition, false);
            return { { U.Low, V.Low }, { U.High, V.High } };
        };
        const auto Validate = [&](const char* Name, const SurfaceGeometry& First, const SurfaceGeometry& Second, const std::function<double(Vector3)>& Residual, int Branches)
        {
            const SurfaceIntersectionOutcome Outcome = IntersectSurfaces(First, Box(First), Second, Box(Second));
            // 📝 Interior points must sit on both exact carriers; an open branch's end on a patch boundary may be marched up
            //    to the SSI's foot gate (10 × precision) past it, as on any NURBS face.
            double Worst = 0.0, EndWorst = 0.0;
            long   Points = 0;
            for (const IntersectionBranch& Branch : Outcome.Branches)
                for (size_t Index = 0; Index < Branch.Chart.size(); ++Index)
                {
                    const bool End = !Branch.ClosedCondition && (Index == 0 || Index + 1 == Branch.Chart.size());
                    (End ? EndWorst : Worst) = std::max(End ? EndWorst : Worst, Residual(Branch.Chart[Index].Position));
                    ++Points;
                }
            const bool Passed = static_cast<int>(Outcome.Branches.size()) >= Branches && Points >= 12 && Worst <= 1.0e-9 && EndWorst <= 1.0e-7;
            Report(SectionGate, Name, std::to_string(Outcome.Branches.size()) + " branch(es), " + std::to_string(Points) + " chart points, worst interior residual " + Scientific(Worst) + " · open ends " + Scientific(EndWorst) + " · " + Outcome.Method + " · lattice " + std::to_string(Outcome.Statistics.LatticeCurves) + " / " + std::to_string(Outcome.Statistics.LatticeCrossings) + " · traced " + std::to_string(Outcome.Statistics.SeedsTraced) + " skipped " + std::to_string(Outcome.Statistics.SeedsSkipped), Worst, Passed);
        };

        // Offset NURBS sphere R 1 by +0.5 against the plane z = 0.3: the circle ρ = √(1.5² − 0.3²)
        const OrthonormalFrame Frame = Upright({});
        const NurbsSurface Sphere = *ConvertSurfaceToNurbs(SphereSurface{ Frame, 1.0 }, { 0.0, -1.5 }, { 2.0 * Pi, 1.5 });
        const SurfaceGeometry Swollen = *OffsetSurface(SurfaceGeometry{ Sphere }, 0.5);
        Validate("offset sphere (R 1 + 0.5) ∩ plane z = 0.3 → circle ρ = √2.16", Swollen, PlaneSurface{ Upright({ 0.0, 0.0, 0.3 }) },
                 [](Vector3 Point) { return std::max(std::fabs(std::hypot(Point.X, Point.Y) - std::sqrt(2.16)), std::fabs(Point.Z - 0.3)); }, 1);

        // Two crossing NURBS cylinders r 1 (axes z and x), each offset by +0.25: equal cylinders r 1.25 (Steinmetz)
        const NurbsSurface Upward  = *ConvertSurfaceToNurbs(CylinderSurface{ Frame, 1.0 }, { 0.0, -3.0 }, { 2.0 * Pi, 3.0 });
        const OrthonormalFrame Sideways{ {}, { 0.0, 1.0, 0.0 }, { 0.0, 0.0, 1.0 }, { 1.0, 0.0, 0.0 } };
        const NurbsSurface Across  = *ConvertSurfaceToNurbs(CylinderSurface{ Sideways, 1.0 }, { 0.0, -3.0 }, { 2.0 * Pi, 3.0 });
        Validate("offset cylinder z ∩ offset cylinder x (r 1 + 0.25) → Steinmetz", *OffsetSurface(SurfaceGeometry{ Upward }, 0.25), *OffsetSurface(SurfaceGeometry{ Across }, 0.25),
                 [](Vector3 Point) { return std::max(std::fabs(std::hypot(Point.X, Point.Y) - 1.25), std::fabs(std::hypot(Point.Y, Point.Z) - 1.25)); }, 2);

        // Offset freeform patch by 0.15 against the plane x = 1.1
        const ParallelSurface Raised = ConstructParallelSurface(FreeformPatch(), 0.15);
        const OrthonormalFrame Wall{ { 1.1, 0.0, 0.0 }, { 0.0, 1.0, 0.0 }, { 0.0, 0.0, 1.0 }, { 1.0, 0.0, 0.0 } };
        Validate("offset freeform (+0.15) ∩ plane x = 1.1", Raised, PlaneSurface{ Wall },
                 [&](Vector3 Point) { return std::max(std::fabs(Point.X - 1.1), OffsetResidual(Raised, Point, { 1.0, 1.0 })); }, 1);
    }

    // ---- P4 fold margin and composition --------------------------------------------------------------------------------
    void ValidateFolds()
    {
        const NurbsSurface Cylinder = *ConvertSurfaceToNurbs(CylinderSurface{ Upright({}), 1.0 }, { 0.0, -1.0 }, { 2.0 * Pi, 1.0 });
        const NurbsDomain U = MeasureNurbsSurfaceDomain(*Cylinder.Definition, true), V = MeasureNurbsSurfaceDomain(*Cylinder.Definition, false);
        for (const auto& [Inward, Expected] : { std::pair{ -0.5, 0.5 }, std::pair{ -1.2, -0.2 } })
        {
            const ParallelFold Fold = MeasureParallelFold(ConstructParallelSurface(Cylinder, Inward), { U.Low, V.Low }, { U.High, V.High });
            const double Error = std::fabs(Fold.Margin - Expected);
            Report(FoldGate, "unit NURBS cylinder offset " + Scientific(Inward), "margin " + Scientific(Fold.Margin) + " (closed form 1 + d/r = " + Scientific(Expected) + ")" + (Fold.Margin <= 0.0 ? " → folds" : " → regular"),
                   Error, Error <= 1.0e-12);
        }
        const SurfaceGeometry Once  = *OffsetSurface(SurfaceGeometry{ Cylinder }, 0.2);
        const SurfaceGeometry Twice = *OffsetSurface(Once, 0.3);
        const auto* Collapsed = std::get_if<ParallelSurface>(&Twice);
        const bool  Composed  = Collapsed && Collapsed->Definition->Basis.Definition == Cylinder.Definition && std::fabs(Collapsed->Definition->Distance - 0.5) <= 1.0e-15;
        Report(FoldGate, "offset 0.2 then 0.3", Composed ? "one parallel carrier on the original basis, d = 0.5" : "not collapsed", 0.0, Composed);
        const SurfaceGeometry Back = *OffsetSurface(Twice, -0.5);
        const auto* Basis = std::get_if<NurbsSurface>(&Back);
        Report(FoldGate, "offset back by −0.5", Basis && Basis->Definition == Cylinder.Definition ? "the basis itself" : "not the basis", 0.0, Basis && Basis->Definition == Cylinder.Definition);
    }
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    ENTRY
//------------------------------------------------------------------------------------------------------------------------

int main(int Count, char** Arguments)
{
    const std::string Only = Count > 2 && std::string(Arguments[1]) == "--only" ? Arguments[2] : "";
    const auto Start = std::chrono::steady_clock::now();
    if (Only.empty() || Only == "carrier") ValidateClosedForms();
    if (Only.empty() || Only == "freeform") ValidateFreeform();
    if (Only.empty() || Only == "section") ValidateSections();
    if (Only.empty() || Only == "fold") ValidateFolds();
    const double Seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - Start).count();

    bool PassedCondition = true;
    for (const GateTally* Gate : AllGates()) PassedCondition = PassedCondition && Gate->Failures == 0 && (Gate->Checks > 0 || !Only.empty());
    std::printf("# Freeform blend validation\n\n%s Priority 10: exact offsets of NURBS surfaces and rolling-ball blends on NURBS supports (%.2f s)\n\n", PassedCondition ? "🟢" : "🔴", Seconds);
    std::printf("## Gates\n\n| | Gate | Measures | Limit | Worst | Checks | Failures |\n|---|---|---|---|---|---|---|\n");
    for (const GateTally* Gate : AllGates())
    {
        std::printf("| %s | %s | %s | %s | %.2e | %ld | %ld |\n", Gate->Failures == 0 && Gate->Checks > 0 ? "🟢" : "🔴", Gate->Code, Gate->Subject, Gate->Limit, Gate->Worst, Gate->Checks, Gate->Failures);
    }
    std::printf("\n## Checks\n\n| | Gate | Check | Detail |\n|---|---|---|---|\n");
    for (const DetailRow& Row : Details) std::printf("| %s | %s | %s | %s |\n", Row.PassedCondition ? "🟢" : "🔴", Row.Gate.c_str(), Row.Name.c_str(), Row.Detail.c_str());
    return PassedCondition ? 0 : 1;
}
