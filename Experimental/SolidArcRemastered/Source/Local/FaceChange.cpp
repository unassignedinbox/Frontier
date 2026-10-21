/*====================================================================================================================================
                                                         FACECHANGE.CPP
====================================================================================================================================*/
// 🧩 Generic face change: carriers replaced, edges re-intersected, vertices re-solved, body re-assembled and audited

//------------------------------------------------------------------------------------------------------------------------
//                                                    INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Local/FaceChange.h"

#include "Construction/BodyAssembly.h"
#include "Containment/FaceDomain.h"
#include "Geometry/ProjectedCurve.h"
#include "Intersection/CurveSurfaceIntersection.h"
#include "Intersection/IntersectionCurve.h"
#include "Geometry/NurbsCarrier.h"
#include "Geometry/RollingBallCarrier.h"
#include "Intersection/IsoParameterCurve.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <map>
#include <numbers>

namespace Frontier::SolidArcRemastered
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    PRIVATE HELPERS
//------------------------------------------------------------------------------------------------------------------------

namespace
{
    constexpr double τPeriod = 2.0 * std::numbers::pi;                                        // [-] - Period of conic carriers

    [[nodiscard]] double SignOf(Orientation Sense) noexcept { return Sense == Orientation::Forward ? 1.0 : -1.0; }

    // 📝 Same point set, opposite direction of travel (frames stay right-handed).
    [[nodiscard]] CurveGeometry ReverseCurve(const CurveGeometry& Curve)
    {
        if (const auto* Line = std::get_if<StraightLine>(&Curve)) return StraightLine{ Line->Origin, -Line->Direction };
        if (const auto* Traced = std::get_if<IntersectionCurve>(&Curve)) return ReverseIntersectionCurve(*Traced->Trace);
        if (const auto* Contact = std::get_if<ContactCurve>(&Curve)) return ReverseContactCurve(*Contact->Definition);
        if (const auto* Spline = std::get_if<NurbsCurve>(&Curve)) return ReverseNurbsCurve(*Spline->Definition);
        if (const auto* Circle = std::get_if<CircularArc>(&Curve))
        {
            CircularArc Reversed = *Circle;
            Reversed.Frame.YAxis = -Circle->Frame.YAxis;
            Reversed.Frame.ZAxis = -Circle->Frame.ZAxis;
            return Reversed;
        }
        EllipticalArc Reversed = std::get<EllipticalArc>(Curve);
        Reversed.Frame.YAxis = -Reversed.Frame.YAxis;
        Reversed.Frame.ZAxis = -Reversed.Frame.ZAxis;
        return Reversed;
    }

    [[nodiscard]] Vector3 ClosestCurvePoint(const CurveGeometry& Curve, Vector3 Position)
    {
        return EvaluateCurve(Curve, ProjectOntoCurve(Curve, Position, 0.0)).Position;
    }

    [[nodiscard]] Vector3 UnitTangentNear(const CurveGeometry& Curve, Vector3 Position)
    {
        return Normalize(EvaluateCurve(Curve, ProjectOntoCurve(Curve, Position, 0.0)).Derivative);
    }

    // 📝 Circle through three points; τ = 0 at the first, travelling towards the second.
    [[nodiscard]] std::optional<CurveGeometry> CircleThrough(Vector3 First, Vector3 Second, Vector3 Third)
    {
        const Vector3 Along  = Second - First;
        const Vector3 Across = Third - First;
        const Vector3 Normal = Cross(Along, Across);
        const double  Area   = Dot(Normal, Normal);
        if (!(Area > 1.0e-30)) return std::nullopt;
        const Vector3 Centre = First + (Cross(Normal, Along) * Dot(Across, Across) + Cross(Across, Normal) * Dot(Along, Along)) * (1.0 / (2.0 * Area));
        const Vector3 XAxis  = Normalize(First - Centre);
        const Vector3 ZAxis  = Normalize(Normal);
        return CircularArc{ { Centre, XAxis, Cross(ZAxis, XAxis), ZAxis }, Distance(First, Centre) };
    }

    [[nodiscard]] std::optional<CurveGeometry> IntersectPlanes(const PlaneSurface& First, const PlaneSurface& Second, Vector3 Near)
    {
        const Vector3 FirstNormal  = First.Frame.ZAxis;
        const Vector3 SecondNormal = Second.Frame.ZAxis;
        const Vector3 Direction    = Cross(FirstNormal, SecondNormal);
        const double  Sine         = Length(Direction);
        if (Sine < 1.0e-12) return std::nullopt;
        // X = Near + α·n₁ + β·n₂ on both planes: [1 c; c 1]·(α, β) = (r₁, r₂).
        const double Cosine      = Dot(FirstNormal, SecondNormal);
        const double FirstReach  = Dot(FirstNormal, First.Frame.Origin - Near);
        const double SecondReach = Dot(SecondNormal, Second.Frame.Origin - Near);
        const double Determinant = 1.0 - Cosine * Cosine;
        const double Alpha       = (FirstReach - Cosine * SecondReach) / Determinant;
        const double Beta        = (SecondReach - Cosine * FirstReach) / Determinant;
        return StraightLine{ Near + FirstNormal * Alpha + SecondNormal * Beta, Direction * (1.0 / Sine) };
    }

    // 📝 Two generator lines of a plane parallel to an axis, the one nearest Near (cylinder: both at radius r).
    [[nodiscard]] std::optional<CurveGeometry> NearestGenerator(Vector3 FirstPoint, Vector3 SecondPoint, Vector3 FirstDirection, Vector3 SecondDirection, Vector3 Near)
    {
        const StraightLine First{ FirstPoint, Normalize(FirstDirection) }, Second{ SecondPoint, Normalize(SecondDirection) };
        return Distance(ClosestCurvePoint(First, Near), Near) <= Distance(ClosestCurvePoint(Second, Near), Near) ? CurveGeometry{ First } : CurveGeometry{ Second };
    }

    [[nodiscard]] std::optional<CurveGeometry> IntersectPlaneCylinder(const PlaneSurface& Plane, const CylinderSurface& Cylinder, Vector3 Near)
    {
        const Vector3 Normal = Plane.Frame.ZAxis;
        const Vector3 Axis   = Cylinder.Frame.ZAxis;
        const double  Cosine = Dot(Normal, Axis);
        if (std::fabs(Cosine) >= 1.0e-12)
        {
            const Vector3 Centre = Cylinder.Frame.Origin + Axis * (Dot(Plane.Frame.Origin - Cylinder.Frame.Origin, Normal) / Cosine);
            if (std::fabs(Cosine) > 1.0 - 1.0e-15) return CircularArc{ ConstructFrame(Centre, Axis, Cylinder.Frame.XAxis), Cylinder.Radius };
            const Vector3 Minor = Normalize(Cross(Axis, Normal));
            const Vector3 Major = Normalize(Cross(Normal, Minor));
            return EllipticalArc{ { Centre, Major, Minor, Cross(Major, Minor) }, Cylinder.Radius / std::fabs(Cosine), Cylinder.Radius };
        }
        const double Reach = Dot(Normal, Cylinder.Frame.Origin - Plane.Frame.Origin);       // [len] - Axis to plane, signed
        if (std::fabs(Reach) >= Cylinder.Radius) return std::nullopt;
        const Vector3 Foot  = Cylinder.Frame.Origin - Normal * Reach;
        const Vector3 Width = Normalize(Cross(Axis, Normal)) * std::sqrt(Cylinder.Radius * Cylinder.Radius - Reach * Reach);
        return NearestGenerator(Foot + Width, Foot - Width, Axis, Axis, Near);
    }

    [[nodiscard]] std::optional<CurveGeometry> IntersectPlaneCone(const PlaneSurface& Plane, const ConeSurface& Cone, Vector3 Near)
    {
        const Vector3 Normal = Plane.Frame.ZAxis;
        const Vector3 Axis   = Cone.Frame.ZAxis;
        const double  Cosine = Dot(Normal, Axis);
        if (std::fabs(Cosine) > 1.0 - 1.0e-14)
        {
            const double ν      = Dot(Plane.Frame.Origin - Cone.Frame.Origin, Axis);
            const double Radius = Cone.OriginRadius + ν * Cone.RadialSlope;
            if (!(Radius > 0.0)) return std::nullopt;
            return CircularArc{ ConstructFrame(Cone.Frame.Origin + Axis * ν, Axis, Cone.Frame.XAxis), Radius };
        }
        if (std::fabs(Cosine) < 1.0e-14 && std::fabs(Dot(Normal, Cone.Frame.Origin - Plane.Frame.Origin)) < 1.0e-12 * (1.0 + std::fabs(Cone.OriginRadius)))
        {
            const Vector3 Radial = Normalize(Cross(Normal, Axis));
            return NearestGenerator(Cone.Frame.Origin + Radial * Cone.OriginRadius, Cone.Frame.Origin - Radial * Cone.OriginRadius,
                                    Axis + Radial * Cone.RadialSlope, Axis - Radial * Cone.RadialSlope, Near);
        }
        return std::nullopt;                                                                 // 🚧 Oblique conic sections
    }

    [[nodiscard]] std::optional<CurveGeometry> IntersectPlaneSphere(const PlaneSurface& Plane, const SphereSurface& Sphere)
    {
        const Vector3 Normal = Plane.Frame.ZAxis;
        const double  Reach  = Dot(Normal, Sphere.Frame.Origin - Plane.Frame.Origin);
        if (std::fabs(Reach) >= Sphere.Radius) return std::nullopt;
        const Vector3 Centre = Sphere.Frame.Origin - Normal * Reach;
        return CircularArc{ ConstructFrame(Centre, Normal, std::fabs(Dot(Sphere.Frame.XAxis, Normal)) < 0.9 ? Sphere.Frame.XAxis : Sphere.Frame.YAxis),
                            std::sqrt(Sphere.Radius * Sphere.Radius - Reach * Reach) };
    }

    //--------------------------------------------------------------------------------------------------------------------
    //                                                REBUILD
    //--------------------------------------------------------------------------------------------------------------------

    struct EdgeSolution
    {
        CurveGeometry      Curve    = StraightLine{};                                        // [-] - New carrier (aligned with the old edge)
        double             Low      = 0.0;                                                   // [-] - τ at the start vertex
        double             High     = 1.0;                                                   // [-] - τ at the end vertex
        EdgeChangeCategory Category = EdgeChangeCategory::Kept;                              // [-] - How it was solved
        double             Offset   = 0.0;                                                   // [len] - Offset distance (Offset category)
    };

    class FaceChangeRebuild
    {
    public:
        FaceChangeRebuild(ModelPartition& Partition, BodyToken Body, const std::vector<FaceReplacement>& Replacements, FaceChangeOutcome& Outcome)
            : Partition(Partition), Body(Body), Replacements(Replacements), Outcome(Outcome), Inventory(Partition.CollectInventory(Body))
        {
        }

        [[nodiscard]] std::string Run()
        {
            if (std::string Refusal = PrepareFaces(); !Refusal.empty()) return Refusal;
            if (std::string Refusal = SolveEdges(); !Refusal.empty()) return Refusal;
            if (std::string Refusal = SolveVertices(); !Refusal.empty()) return Refusal;
            if (std::string Refusal = SolveIntervals(); !Refusal.empty()) return Refusal;
            return ComposeBlueprint();
        }

        BodyBlueprint                  Blueprint;
        std::vector<FaceChangeHistory> Provenance;

    private:
        ModelPartition&                     Partition;
        BodyToken                           Body;
        const std::vector<FaceReplacement>& Replacements;
        FaceChangeOutcome&                  Outcome;
        BodyInventory                       Inventory;
        std::map<uint32_t, int>             FaceIndex;                                       // Face slot → inventory index
        std::map<uint32_t, int>             EdgeIndex;                                       // Edge slot → inventory index
        std::map<uint32_t, int>             VertexIndex;                                     // Vertex slot → inventory index
        std::vector<SurfaceGeometry>        Surfaces;                                        // Per face: carrier after the change
        std::vector<bool>                   Changed;                                         // Per face: carrier replaced
        std::vector<double>                 Offsets;                                         // Per face: outward offset (NaN)
        std::vector<EdgeSolution>           Solutions;                                       // Per edge
        std::vector<Vector3>                Positions;                                       // Per vertex
        std::vector<bool>                   Moved;                                           // Per vertex
        double                              Scale = 1.0;                                     // [len] - Body diagonal
        bool                                SheetCondition = false;                          // [-]   - Laminar edges allowed

        [[nodiscard]] FaceToken FaceOfFin(FinToken Fin) const { return Partition.Loops.Resolve(Partition.Fins.Resolve(Fin).Loop).Face; }
        [[nodiscard]] int       IndexOf(FaceToken Face) const { return FaceIndex.at(Face.Slot); }
        [[nodiscard]] double    Tolerance() const { return 1.0e-9 * Scale; }

        [[nodiscard]] Vector3 OutwardNormal(int Face, Vector3 Position) const
        {
            const SurfaceGeometry& Surface = Surfaces[Face];
            const Orientation      Sense   = Partition.Faces.Resolve(Inventory.Faces[Face]).Sense;
            return EvaluateSurfaceNormal(Surface, ProjectOntoSurface(Surface, Position, {})) * SignOf(Sense);
        }

        [[nodiscard]] double DistanceToSurface(int Face, Vector3 Position) const
        {
            const SurfaceGeometry& Surface = Surfaces[Face];
            return Distance(Position, EvaluateSurface(Surface, ProjectOntoSurface(Surface, Position, {})).Position);
        }

        std::string PrepareFaces()
        {
            Vector3 Low{ 1e300, 1e300, 1e300 }, High{ -1e300, -1e300, -1e300 };
            for (size_t Index = 0; Index < Inventory.Vertices.size(); ++Index)
            {
                const Vector3 Position = Partition.Vertices.Resolve(Inventory.Vertices[Index]).Position;
                VertexIndex[Inventory.Vertices[Index].Slot] = static_cast<int>(Index);
                Low  = { std::min(Low.X, Position.X), std::min(Low.Y, Position.Y), std::min(Low.Z, Position.Z) };
                High = { std::max(High.X, Position.X), std::max(High.Y, Position.Y), std::max(High.Z, Position.Z) };
                Positions.push_back(Position);
            }
            Moved.assign(Positions.size(), false);
            SheetCondition = Partition.Bodies.Resolve(Body).Category == BodyCategory::Sheet;
            Scale = std::max(1.0e-300, Distance(Low, High));
            for (size_t Index = 0; Index < Inventory.Faces.size(); ++Index)
            {
                FaceIndex[Inventory.Faces[Index].Slot] = static_cast<int>(Index);
                Surfaces.push_back(Partition.Surfaces.Resolve(Partition.Faces.Resolve(Inventory.Faces[Index]).Surface));
            }
            Changed.assign(Surfaces.size(), false);
            Offsets.assign(Surfaces.size(), std::numeric_limits<double>::quiet_NaN());
            for (const FaceReplacement& Replacement : Replacements)
            {
                const auto Found = FaceIndex.find(Replacement.Face.Slot);
                if (Found == FaceIndex.end() || !(Inventory.Faces[Found->second] == Replacement.Face)) return LabelToken("F", Replacement.Face) + " is not a face of the body";
                if (Changed[Found->second]) return LabelToken("F", Replacement.Face) + " is replaced twice";
                Surfaces[Found->second] = Replacement.Surface;
                Changed[Found->second]  = true;
                Offsets[Found->second]  = Replacement.OutwardOffset;
                ++Outcome.Statistics.ReplacedFaces;
            }
            for (size_t Index = 0; Index < Inventory.Edges.size(); ++Index) EdgeIndex[Inventory.Edges[Index].Slot] = static_cast<int>(Index);
            return {};
        }

        // ① Smooth edge: outward normals agree at both ends and the middle.
        [[nodiscard]] bool SmoothCondition(EdgeToken Edge, FaceToken First, FaceToken Second) const
        {
            const EdgeProfile&   Profile = Partition.Edges.Resolve(Edge);
            const CurveGeometry& Curve   = Partition.Curves.Resolve(Profile.Curve);
            for (double Fraction : { 0.0, 0.5, 1.0 })
            {
                const Vector3 Position = EvaluateCurve(Curve, Profile.IntervalLow + Fraction * (Profile.IntervalHigh - Profile.IntervalLow)).Position;
                if (Dot(EvaluateOutwardNormal(Partition, First, Position), EvaluateOutwardNormal(Partition, Second, Position)) < 1.0 - 1.0e-12) return false;
            }
            return true;
        }

        // 📝 C(τ) + d·n(τ): a line stays a line, a circle on a surface of revolution about its axis stays a circle.
        [[nodiscard]] std::optional<CurveGeometry> OffsetAlongNormal(EdgeToken Edge, FaceToken Face, double Shift) const
        {
            const EdgeProfile&   Profile = Partition.Edges.Resolve(Edge);
            const CurveGeometry& Curve   = Partition.Curves.Resolve(Profile.Curve);
            const auto Displaced = [&](double τ) { const Vector3 Point = EvaluateCurve(Curve, τ).Position; return Point + EvaluateOutwardNormal(Partition, Face, Point) * Shift; };
            std::optional<CurveGeometry> Result;
            if (const auto* Line = std::get_if<StraightLine>(&Curve)) Result = StraightLine{ Displaced(0.0), Line->Direction };
            else if (std::holds_alternative<CircularArc>(Curve))
            {
                Result = CircleThrough(Displaced(0.0), Displaced(τPeriod / 3.0), Displaced(2.0 * τPeriod / 3.0));
                if (Result && Dot(std::get<CircularArc>(*Result).Frame.ZAxis, std::get<CircularArc>(Curve).Frame.ZAxis) < 0.0) Result = ReverseCurve(*Result);
            }
            if (!Result) return std::nullopt;
            for (double Fraction : { 0.0, 0.25, 0.5, 0.75, 1.0 })                            // The offset must really be that curve
            {
                const Vector3 Target = Displaced(Profile.IntervalLow + Fraction * (Profile.IntervalHigh - Profile.IntervalLow));
                if (Distance(ClosestCurvePoint(*Result, Target), Target) > Tolerance()) return std::nullopt;
            }
            return Result;
        }

        std::string SolveEdges()
        {
            Solutions.resize(Inventory.Edges.size());
            for (size_t Index = 0; Index < Inventory.Edges.size(); ++Index)
            {
                const EdgeToken      Edge    = Inventory.Edges[Index];
                const EdgeProfile&   Profile = Partition.Edges.Resolve(Edge);
                const CurveGeometry& Curve   = Partition.Curves.Resolve(Profile.Curve);
                const FinToken       FirstFin  = Profile.RadialFin;
                const FinToken       SecondFin = Partition.RadialMate(FirstFin);
                if (SecondFin == FirstFin && SheetCondition)
                {
                    // Laminar sheet edge: the face meets its rim surface, which no face change moves.
                    const int     Lone     = IndexOf(FaceOfFin(FirstFin));
                    EdgeSolution& Solution = Solutions[Index];
                    Solution.Curve = Curve;
                    if (!Changed[Lone]) { Solution.Category = EdgeChangeCategory::Kept; continue; }
                    const std::optional<SurfaceGeometry> Rim = ConstructRimSurface(Partition, Edge, Inventory.Faces[Lone]);
                    if (!Rim) return LabelToken("E", Edge) + ": laminar edge without a rim surface (🚧)";
                    const double  Middle   = 0.5 * (Profile.IntervalLow + Profile.IntervalHigh);
                    const Vector3 Midpoint = EvaluateCurve(Curve, Middle).Position;
                    std::optional<CurveGeometry> Solved = IntersectCarriersExactly(Surfaces[Lone], *Rim, Midpoint);
                    if (!Solved) return LabelToken("E", Edge) + ": new " + SurfaceCategoryLabel(Surfaces[Lone]) + " no longer meets the rim " + SurfaceCategoryLabel(*Rim);
                    if (Dot(UnitTangentNear(*Solved, Midpoint), Normalize(EvaluateCurve(Curve, Middle).Derivative)) < 0.0) Solved = ReverseCurve(*Solved);
                    Solution.Curve    = *Solved;
                    Solution.Category = EdgeChangeCategory::Intersected;
                    ++Outcome.Statistics.Edges[static_cast<int>(Solution.Category)];
                    continue;
                }
                if (SecondFin == FirstFin || !(Partition.RadialMate(SecondFin) == FirstFin)) return LabelToken("E", Edge) + ": not a manifold edge";
                const int First  = IndexOf(FaceOfFin(FirstFin));
                const int Second = IndexOf(FaceOfFin(SecondFin));
                EdgeSolution& Solution = Solutions[Index];
                Solution.Curve = Curve;
                if (!Changed[First] && !Changed[Second]) { Solution.Category = EdgeChangeCategory::Kept; continue; }
                const double  Middle   = 0.5 * (Profile.IntervalLow + Profile.IntervalHigh);
                const Vector3 Midpoint = EvaluateCurve(Curve, Middle).Position;
                const Vector3 Tangent  = Normalize(EvaluateCurve(Curve, Middle).Derivative);
                std::optional<CurveGeometry> Solved;
                if (First == Second)
                {
                    // Seam: the iso-line of the new carrier at the seam's parameter (frames are carried along by every change).
                    const FinProfile& Fin = Partition.Fins.Resolve(FirstFin);
                    if (!Fin.SurfaceCurve) return LabelToken("E", Edge) + ": seam without an SP-curve";
                    const ParameterSample Sample = EvaluateParameterCurve(*Fin.SurfaceCurve, Middle);
                    const bool FixedU = std::fabs(Sample.Derivative.X) <= 1.0e-12 * std::max(1.0, std::fabs(Sample.Derivative.Y));
                    const IsoParameterCurve Iso = ExtractIsoParameterCurve(Surfaces[First], FixedU ? IsoDirection::FixedU : IsoDirection::FixedV, FixedU ? Sample.Position.X : Sample.Position.Y);
                    if (Iso.DegenerateCondition) return LabelToken("E", Edge) + ": seam iso-line degenerates on the new carrier";
                    Solved = Iso.Curve;
                    Solution.Category = EdgeChangeCategory::Seam;
                }
                else if (!std::isnan(Offsets[First]) && Offsets[First] == Offsets[Second] && SmoothCondition(Edge, Inventory.Faces[First], Inventory.Faces[Second]))
                {
                    Solved = OffsetAlongNormal(Edge, Inventory.Faces[First], Offsets[First]);
                    if (!Solved) return LabelToken("E", Edge) + ": smooth edge whose offset is not a line or circle (🚧)";
                    Solution.Category = EdgeChangeCategory::Offset;
                    Solution.Offset   = Offsets[First];
                }
                else
                {
                    if (SmoothCondition(Edge, Inventory.Faces[First], Inventory.Faces[Second]))
                    {
                        return LabelToken("E", Edge) + ": smooth edge between faces that move differently (the blend must be re-made, 🚧)";
                    }
                    Solved = IntersectCarriersExactly(Surfaces[First], Surfaces[Second], Midpoint);
                    if (!Solved)
                    {
                        return LabelToken("E", Edge) + ": new " + SurfaceCategoryLabel(Surfaces[First]) + " and " + SurfaceCategoryLabel(Surfaces[Second]) +
                               " no longer meet (or not exactly, 🚧)";
                    }
                    Solution.Category = EdgeChangeCategory::Intersected;
                    if (std::holds_alternative<EllipticalArc>(*Solved)) ++Outcome.Statistics.EllipticalEdges;
                }
                if (Dot(UnitTangentNear(*Solved, Midpoint), Tangent) < 0.0) Solved = ReverseCurve(*Solved);
                Solution.Curve = *Solved;
                ++Outcome.Statistics.Edges[static_cast<int>(Solution.Category)];
            }
            Outcome.Statistics.Edges[0] = static_cast<int>(std::count_if(Solutions.begin(), Solutions.end(), [](const EdgeSolution& Solution) { return Solution.Category == EdgeChangeCategory::Kept; }));
            return {};
        }

        // 📝 Gauss–Newton on |C₁(τ₁) − C₂(τ₂)|² from the old vertex: quadratic for transversal curves.
        [[nodiscard]] Vector3 MeetCurves(const CurveGeometry& First, const CurveGeometry& Second, Vector3 Start) const
        {
            double FirstParameter  = ProjectOntoCurve(First, Start, 0.0);
            double SecondParameter = ProjectOntoCurve(Second, Start, 0.0);
            for (int Iteration = 0; Iteration < 60; ++Iteration)
            {
                const CurveSample A = EvaluateCurve(First, FirstParameter);
                const CurveSample B = EvaluateCurve(Second, SecondParameter);
                const Vector3 Residual = A.Position - B.Position;
                const double  M11 = Dot(A.Derivative, A.Derivative), M12 = -Dot(A.Derivative, B.Derivative), M22 = Dot(B.Derivative, B.Derivative);
                const double  G1  = Dot(A.Derivative, Residual), G2 = -Dot(B.Derivative, Residual);
                const double  Determinant = M11 * M22 - M12 * M12;
                if (!(std::fabs(Determinant) > 1.0e-300)) break;
                const double  StepFirst  = -(M22 * G1 - M12 * G2) / Determinant;
                const double  StepSecond = -(M11 * G2 - M12 * G1) / Determinant;
                FirstParameter  += StepFirst;
                SecondParameter += StepSecond;
                if (std::fabs(StepFirst) + std::fabs(StepSecond) < 1.0e-16 * (1.0 + std::fabs(FirstParameter) + std::fabs(SecondParameter))) break;
            }
            return (EvaluateCurve(First, FirstParameter).Position + EvaluateCurve(Second, SecondParameter).Position) * 0.5;
        }

        std::string SolveVertices()
        {
            std::vector<std::vector<int>> Incident(Positions.size());
            for (size_t Index = 0; Index < Inventory.Edges.size(); ++Index)
            {
                const EdgeProfile& Profile = Partition.Edges.Resolve(Inventory.Edges[Index]);
                const int Start = VertexIndex.at(Profile.Start.Slot), End = VertexIndex.at(Profile.End.Slot);
                Incident[Start].push_back(static_cast<int>(Index));
                if (End != Start) Incident[End].push_back(static_cast<int>(Index));
            }
            for (size_t Vertex = 0; Vertex < Positions.size(); ++Vertex)
            {
                const std::vector<int>& Edges = Incident[Vertex];
                if (Edges.empty()) return LabelToken("V", Inventory.Vertices[Vertex]) + ": isolated vertex";
                const bool AllKept = std::all_of(Edges.begin(), Edges.end(), [&](int Edge) { return Solutions[Edge].Category == EdgeChangeCategory::Kept; });
                if (AllKept) continue;
                const Vector3 Old = Positions[Vertex];
                Vector3 Solved = Old;
                const bool AllOffset = std::all_of(Edges.begin(), Edges.end(), [&](int Edge) { return Solutions[Edge].Category == EdgeChangeCategory::Offset && Solutions[Edge].Offset == Solutions[Edges[0]].Offset; });
                if (AllOffset)
                {
                    const FinToken Fin = Partition.Edges.Resolve(Inventory.Edges[Edges[0]]).RadialFin;
                    Solved = Old + EvaluateOutwardNormal(Partition, FaceOfFin(Fin), Old) * Solutions[Edges[0]].Offset;
                }
                else
                {
                    // The two most transversal incident curves define the vertex; the others must agree.
                    double BestSine = -1.0;
                    int    BestFirst = Edges[0], BestSecond = Edges[0];
                    for (size_t A = 0; A < Edges.size(); ++A)
                    {
                        for (size_t B = A + 1; B < Edges.size(); ++B)
                        {
                            const double Sine = Length(Cross(UnitTangentNear(Solutions[Edges[A]].Curve, Old), UnitTangentNear(Solutions[Edges[B]].Curve, Old)));
                            if (Sine > BestSine) { BestSine = Sine; BestFirst = Edges[A]; BestSecond = Edges[B]; }
                        }
                    }
                    if (BestSine < 1.0e-6) return LabelToken("V", Inventory.Vertices[Vertex]) + ": incident edges are tangent, the vertex is not determined (🚧)";
                    Solved = MeetCurves(Solutions[BestFirst].Curve, Solutions[BestSecond].Curve, Old);
                }
                for (int Edge : Edges)
                {
                    const double Gap = Distance(ClosestCurvePoint(Solutions[Edge].Curve, Solved), Solved);
                    Outcome.Statistics.MaxVertexResidual = std::max(Outcome.Statistics.MaxVertexResidual, Gap);
                    if (Gap > Tolerance())
                    {
                        return LabelToken("V", Inventory.Vertices[Vertex]) + ": incident edges no longer meet in one point (" + LabelToken("E", Inventory.Edges[Edge]) + " misses by " +
                               std::to_string(Gap) + ")";
                    }
                }
                Positions[Vertex] = Solved;
                Moved[Vertex]     = Distance(Solved, Old) > 0.0;
                if (Moved[Vertex]) ++Outcome.Statistics.MovedVertices;
            }
            return {};
        }

        std::string SolveIntervals()
        {
            for (size_t Index = 0; Index < Inventory.Edges.size(); ++Index)
            {
                const EdgeToken    Edge    = Inventory.Edges[Index];
                const EdgeProfile& Profile = Partition.Edges.Resolve(Edge);
                EdgeSolution&      Solution = Solutions[Index];
                const int Start = VertexIndex.at(Profile.Start.Slot), End = VertexIndex.at(Profile.End.Slot);
                if (Solution.Category == EdgeChangeCategory::Kept && !Moved[Start] && !Moved[End])
                {
                    Solution.Low = Profile.IntervalLow; Solution.High = Profile.IntervalHigh;
                    continue;
                }
                const double OldSweep = Profile.IntervalHigh - Profile.IntervalLow;
                if (PeriodicCondition(Solution.Curve))
                {
                    // Kept curves keep their parameter branch; new ones start at the projection of the start vertex.
                    double Low = ProjectOntoCurve(Solution.Curve, Positions[Start], Solution.Category == EdgeChangeCategory::Kept ? Profile.IntervalLow - 0.5 : 0.0);
                    if (Solution.Category == EdgeChangeCategory::Kept && std::fabs(Low - Profile.IntervalLow) > std::numbers::pi) Low += τPeriod * std::round((Profile.IntervalLow - Low) / τPeriod);
                    double High = Start == End ? Low + τPeriod : ProjectOntoCurve(Solution.Curve, Positions[End], Low);
                    if (Start != End && High <= Low) High += τPeriod;
                    if (std::fabs((High - Low) - OldSweep) > std::numbers::pi) return LabelToken("E", Edge) + ": arc runs backwards (the change exceeds the edge, 🚧)";
                    Solution.Low = Low; Solution.High = High;
                }
                else
                {
                    Solution.Low  = ProjectOntoCurve(Solution.Curve, Positions[Start], 0.0);
                    Solution.High = ProjectOntoCurve(Solution.Curve, Positions[End], 0.0);
                    const double Span = (Solution.High - Solution.Low) * MeasureParameterScale(Solution.Curve);
                    if (Span <= Tolerance()) return LabelToken("E", Edge) + (Span < -Tolerance() ? ": runs backwards" : ": collapses") + " (the change exceeds the edge; topology would change, 🚧)";
                }
                // Every edge must lie on both new carriers.
                const FinToken FirstFin = Profile.RadialFin;
                const int First  = IndexOf(FaceOfFin(FirstFin));
                const int Second = IndexOf(FaceOfFin(Partition.RadialMate(FirstFin)));
                for (double Fraction : { 0.0, 0.25, 0.5, 0.75, 1.0 })
                {
                    const Vector3 Point = EvaluateCurve(Solution.Curve, Solution.Low + Fraction * (Solution.High - Solution.Low)).Position;
                    const double  Gap   = std::max(DistanceToSurface(First, Point), DistanceToSurface(Second, Point));
                    Outcome.Statistics.MaxEdgeResidual = std::max(Outcome.Statistics.MaxEdgeResidual, Gap);
                    if (Gap > Tolerance()) return LabelToken("E", Edge) + ": new curve leaves its carriers by " + std::to_string(Gap);
                }
            }
            return {};
        }

        std::string ComposeBlueprint()
        {
            Blueprint.Category = Partition.Bodies.Resolve(Body).Category;
            for (const Vector3& Position : Positions) Blueprint.Vertices.push_back({ Position });
            for (size_t Index = 0; Index < Inventory.Edges.size(); ++Index)
            {
                const EdgeProfile&  Profile  = Partition.Edges.Resolve(Inventory.Edges[Index]);
                const EdgeSolution& Solution = Solutions[Index];
                Blueprint.Edges.push_back({ Solution.Curve, Solution.Low, Solution.High, VertexIndex.at(Profile.Start.Slot), VertexIndex.at(Profile.End.Slot) });
            }
            for (size_t Face = 0; Face < Inventory.Faces.size(); ++Face)
            {
                const FaceProfile& Profile = Partition.Faces.Resolve(Inventory.Faces[Face]);
                BlueprintFace Planned{ Surfaces[Face], Profile.Sense, {} };
                bool BoundaryMoved = false;
                for (LoopToken Loop : Profile.Loops)
                {
                    std::vector<BlueprintUse> Uses;
                    std::string Refusal;
                    Partition.EnumerateLoopFins(Loop, [&](FinToken Fin)
                    {
                        const FinProfile& FinRecord = Partition.Fins.Resolve(Fin);
                        const int Edge = EdgeIndex.at(FinRecord.Edge.Slot);
                        const EdgeSolution& Solution = Solutions[Edge];
                        const EdgeProfile&  Old      = Partition.Edges.Resolve(FinRecord.Edge);
                        if (!FinRecord.SurfaceCurve) { Refusal = "fin without an SP-curve on " + LabelToken("F", Inventory.Faces[Face]); return; }
                        const bool Same = Solution.Category == EdgeChangeCategory::Kept && Solution.Low == Old.IntervalLow && Solution.High == Old.IntervalHigh;
                        BoundaryMoved = BoundaryMoved || !Same;
                        if (Same || (Solution.Category == EdgeChangeCategory::Kept && !Changed[Face]))
                        {
                            Uses.push_back({ Edge, FinRecord.Sense, *FinRecord.SurfaceCurve });       // Same τ, same chart
                            return;
                        }
                        const Vector2 Reference = EvaluateParameterCurve(*FinRecord.SurfaceCurve, 0.5 * (Old.IntervalLow + Old.IntervalHigh)).Position;
                        const Vector3 Middle    = EvaluateCurve(Solution.Curve, 0.5 * (Solution.Low + Solution.High)).Position;
                        const Vector2 Anchor    = ProjectOntoSurface(Surfaces[Face], Middle, Reference);
                        Uses.push_back({ Edge, FinRecord.Sense, FitSurfaceCurve(Surfaces[Face], Solution.Curve, Solution.Low, Solution.High,
                                                                                { Anchor - Vector2{ 1.0, 1.0 }, Anchor + Vector2{ 1.0, 1.0 } }, Anchor) });
                    });
                    if (!Refusal.empty()) return Refusal;
                    Planned.Loops.push_back(std::move(Uses));
                }
                Blueprint.Faces.push_back(std::move(Planned));
                const FaceChangeOrigin Origin = Changed[Face] ? FaceChangeOrigin::Replaced : BoundaryMoved ? FaceChangeOrigin::Trimmed : FaceChangeOrigin::Unchanged;
                Provenance.push_back({ {}, Origin, Inventory.Faces[Face], {} });
            }
            return {};
        }
    };
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    PUBLIC FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

std::optional<CurveGeometry> IntersectCarriersExactly(const SurfaceGeometry& First, const SurfaceGeometry& Second, Vector3 Near)
{
    if (!std::holds_alternative<PlaneSurface>(First) && std::holds_alternative<PlaneSurface>(Second)) return IntersectCarriersExactly(Second, First, Near);
    const auto* Plane = std::get_if<PlaneSurface>(&First);
    if (Plane == nullptr) return std::nullopt;                                               // 🚧 Two curved carriers
    if (const auto* Other = std::get_if<PlaneSurface>(&Second))    return IntersectPlanes(*Plane, *Other, Near);
    if (const auto* Other = std::get_if<CylinderSurface>(&Second)) return IntersectPlaneCylinder(*Plane, *Other, Near);
    if (const auto* Other = std::get_if<ConeSurface>(&Second))     return IntersectPlaneCone(*Plane, *Other, Near);
    if (const auto* Other = std::get_if<SphereSurface>(&Second))   return IntersectPlaneSphere(*Plane, *Other);
    return std::nullopt;
}

std::optional<SurfaceGeometry> ConstructRimSurface(const ModelPartition& Partition, EdgeToken Edge, FaceToken Face)
{
    const EdgeProfile&     Profile = Partition.Edges.Resolve(Edge);
    const CurveGeometry&   Curve   = Partition.Curves.Resolve(Profile.Curve);
    const SurfaceGeometry& Surface = Partition.Surfaces.Resolve(Partition.Faces.Resolve(Face).Surface);
    const CurveSample      Middle  = EvaluateCurve(Curve, 0.5 * (Profile.IntervalLow + Profile.IntervalHigh));
    const Vector3          Normal  = EvaluateOutwardNormal(Partition, Face, Middle.Position);
    if (const auto* Line = std::get_if<StraightLine>(&Curve))
    {
        const Vector3 Across = Cross(Line->Direction, Normal);
        if (Length(Across) < 1.0e-12) return std::nullopt;
        // Plane face: the normal is constant. Cylinder generator: the normal is radial and constant along the line.
        if (std::holds_alternative<PlaneSurface>(Surface) || std::holds_alternative<CylinderSurface>(Surface)) return PlaneSurface{ ConstructFrame(Line->Origin, Normalize(Across), Line->Direction) };
        return std::nullopt;
    }
    if (const auto* Circle = std::get_if<CircularArc>(&Curve))
    {
        if (const auto* Plane = std::get_if<PlaneSurface>(&Surface))
        {
            if (std::fabs(std::fabs(Dot(Plane->Frame.ZAxis, Circle->Frame.ZAxis)) - 1.0) > 1.0e-12) return std::nullopt;
            return CylinderSurface{ Circle->Frame, Circle->Radius };
        }
        if (const auto* Cylinder = std::get_if<CylinderSurface>(&Surface))
        {
            const Vector3 Gap = Circle->Frame.Origin - Cylinder->Frame.Origin;
            if (std::fabs(std::fabs(Dot(Cylinder->Frame.ZAxis, Circle->Frame.ZAxis)) - 1.0) > 1.0e-12 || Length(Gap - Cylinder->Frame.ZAxis * Dot(Gap, Cylinder->Frame.ZAxis)) > 1.0e-9 * Circle->Radius) return std::nullopt;
            return PlaneSurface{ Circle->Frame };
        }
    }
    return std::nullopt;
}

std::string PlanFaceChange(ModelPartition& Partition, BodyToken Body, const std::vector<FaceReplacement>& Replacements, BodyBlueprint& Blueprint,
                           std::vector<FaceChangeHistory>& Provenance, FaceChangeStatistics& Statistics)
{
    FaceChangeOutcome Outcome;
    FaceChangeRebuild Rebuild(Partition, Body, Replacements, Outcome);
    std::string       Refusal = Rebuild.Run();
    Statistics = Outcome.Statistics;
    if (!Refusal.empty()) return Refusal;
    Blueprint  = Rebuild.Blueprint;
    Provenance = Rebuild.Provenance;
    return {};
}

std::string DetectSelfClash(const ModelPartition& Partition, BodyToken Body)
{
    const BodyInventory Inventory = Partition.CollectInventory(Body);
    Vector3 Low{ 1e300, 1e300, 1e300 }, High{ -1e300, -1e300, -1e300 };
    for (VertexToken Vertex : Inventory.Vertices)
    {
        const Vector3 Position = Partition.Vertices.Resolve(Vertex).Position;
        Low  = { std::min(Low.X, Position.X), std::min(Low.Y, Position.Y), std::min(Low.Z, Position.Z) };
        High = { std::max(High.X, Position.X), std::max(High.Y, Position.Y), std::max(High.Z, Position.Z) };
    }
    const double Precision = 1.0e-9 * std::max(1.0e-300, Distance(Low, High));
    std::vector<FaceDomain> Domains;
    for (FaceToken Face : Inventory.Faces) Domains.push_back(DescribeFaceDomain(Partition, Face));
    CurveSurfaceSettings Settings;
    Settings.Precision = Precision;
    for (EdgeToken Edge : Inventory.Edges)
    {
        const EdgeProfile&   Profile = Partition.Edges.Resolve(Edge);
        const CurveGeometry& Curve   = Partition.Curves.Resolve(Profile.Curve);
        const Vector3        Ends[2] = { Partition.Vertices.Resolve(Profile.Start).Position, Partition.Vertices.Resolve(Profile.End).Position };
        std::vector<uint32_t> Users;
        FinToken Fin = Profile.RadialFin;
        for (int Guard = 0; Fin.PresentCondition() && Guard < 8; ++Guard)
        {
            Users.push_back(Partition.Loops.Resolve(Partition.Fins.Resolve(Fin).Loop).Face.Slot);
            Fin = Partition.Fins.Resolve(Fin).RadialSuccessor;
            if (Fin == Profile.RadialFin) break;
        }
        for (size_t Index = 0; Index < Inventory.Faces.size(); ++Index)
        {
            if (std::find(Users.begin(), Users.end(), Inventory.Faces[Index].Slot) != Users.end()) continue;
            const FaceDomain& Domain = Domains[Index];
            const CurveSurfaceOutcome Outcome = IntersectCurveSurface(Curve, Profile.IntervalLow, Profile.IntervalHigh, Domain.Surface, Settings);
            std::vector<Vector3> Probes;
            for (const CurveSurfaceCrossing& Crossing : Outcome.Crossings) Probes.push_back(Crossing.Position);
            for (const CoincidentInterval& Stretch : Outcome.Coincidences) Probes.push_back(EvaluateCurve(Curve, 0.5 * (Stretch.Low + Stretch.High)).Position);
            for (const Vector3& Probe : Probes)
            {
                if (Distance(Probe, Ends[0]) <= 1.0e3 * Precision || Distance(Probe, Ends[1]) <= 1.0e3 * Precision) continue;
                // Inside: the edge pierces the face. Boundary: it crosses one of the face's edges away from any shared
                // vertex (an edge–edge crossing, e.g. a tilted rim dipping through the base rim) — both are clashes.
                if (ClassifyFacePosition(Domain, Probe, 1.0e3 * Precision).Membership != PointMembership::Outside)
                {
                    return LabelToken("E", Edge) + " passes through " + LabelToken("F", Inventory.Faces[Index]) + " (self-intersection; removal is 🚧)";
                }
            }
        }
    }
    return {};
}

Vector3 EvaluateOutwardNormal(const ModelPartition& Partition, FaceToken Face, Vector3 Position)
{
    const FaceProfile&     Profile = Partition.Faces.Resolve(Face);
    const SurfaceGeometry& Surface = Partition.Surfaces.Resolve(Profile.Surface);
    return EvaluateSurfaceNormal(Surface, ProjectOntoSurface(Surface, Position, {})) * SignOf(Profile.Sense);
}

FaceChangeOutcome ChangeFaces(ModelPartition& Partition, BodyToken Body, const std::vector<FaceReplacement>& Replacements, const std::string& Label)
{
    const auto        Started = std::chrono::steady_clock::now();
    FaceChangeOutcome Outcome;
    const auto        Finish  = [&](std::string Refusal)
    {
        Outcome.Refusal = Refusal.empty() ? std::string{} : Label + ": " + Refusal;
        Outcome.Statistics.Milliseconds = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - Started).count();
        return Outcome;
    };
    FaceChangeRebuild Rebuild(Partition, Body, Replacements, Outcome);
    if (std::string Refusal = Rebuild.Run(); !Refusal.empty()) return Finish(Refusal);
    const AssemblyOutcome Assembly = AssembleBody(Partition, Rebuild.Blueprint, Label);
    if (!Assembly.Refusal.empty()) return Finish(Assembly.Refusal);
    Outcome.Audit = AuditBody(Partition, Assembly.Body);
    if (!Outcome.Audit.SoundCondition())
    {
        const AuditFinding& First = Outcome.Audit.Findings.front();
        return Finish("result fails audit " + std::string(First.Code) + " on " + First.Subject + " (" + First.Detail + ")");
    }
    if (std::string Clash = DetectSelfClash(Partition, Assembly.Body); !Clash.empty()) return Finish(Clash);
    const int Genus = AuditBody(Partition, Body).Genus;
    if (Outcome.Audit.Genus != Genus) return Finish("genus changed from " + std::to_string(Genus) + " to " + std::to_string(Outcome.Audit.Genus));
    for (size_t Index = 0; Index < Assembly.Faces.size(); ++Index)
    {
        FaceChangeHistory History = Rebuild.Provenance[Index];
        History.Result = Assembly.Faces[Index];
        Outcome.FaceHistories.push_back(History);
    }
    Outcome.Body = Assembly.Body;
    return Finish({});
}

const char* FaceChangeOriginLabel(FaceChangeOrigin Origin) noexcept
{
    switch (Origin)
    {
        case FaceChangeOrigin::Unchanged: return "Unchanged";
        case FaceChangeOrigin::Trimmed:   return "Trimmed";
        case FaceChangeOrigin::Replaced:  return "Replaced";
        case FaceChangeOrigin::Generated: return "Generated";
    }
    return "?";
}

const char* EdgeChangeCategoryLabel(EdgeChangeCategory Category) noexcept
{
    switch (Category)
    {
        case EdgeChangeCategory::Kept:        return "Kept";
        case EdgeChangeCategory::Seam:        return "Seam";
        case EdgeChangeCategory::Offset:      return "Offset";
        case EdgeChangeCategory::Intersected: return "Intersected";
    }
    return "?";
}

} // namespace Frontier::SolidArcRemastered
