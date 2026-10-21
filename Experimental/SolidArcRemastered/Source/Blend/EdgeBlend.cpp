/*====================================================================================================================================
                                                        EDGEBLEND.CPP
====================================================================================================================================*/
// 🧩 Rolling-ball fillets and chamfers on edge sets: vertex resolution (caps, mitres, ball corners) and direct rebuild

#include "Blend/EdgeBlend.h"

#include "Construction/BodyAssembly.h"
#include "Containment/FaceDomain.h"
#include "Geometry/ProjectedCurve.h"
#include "Intersection/CurveSurfaceIntersection.h"
#include "Intersection/IntersectionCurve.h"
#include "Intersection/SurfaceIntersection.h"
#include "Local/FaceChange.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <limits>
#include <map>
#include <numbers>
#include <set>
#include <tuple>

namespace Frontier::SolidArcRemastered
{

namespace
{
    //--------------------------------------------------------------------------------------------------------------------
    //                                                STRUCTS
    //--------------------------------------------------------------------------------------------------------------------

    // 📝 A blueprint edge traversed From → To (vertex indices); the use sense follows from the edge's Start.
    struct ChainLink
    {
        int Edge = -1;                                                                       // [-] - BlueprintEdge index
        int From = -1;                                                                       // [-] - BlueprintVertex index
        int To   = -1;                                                                       // [-] - BlueprintVertex index
    };

    struct CornerPatch
    {
        VertexToken            Vertex    = {};                                               // [-] - Source vertex
        SurfaceGeometry        Surface   = PlaneSurface{};                                   // [-] - Corner carrier
        Orientation            Sense     = Orientation::Forward;                             // [-] - Outward vs natural normal
        std::vector<ChainLink> Links     = {};                                               // [-] - Unordered boundary
        Vector2                Reference = {};                                               // [-] - Chart anchor reference
    };

    using EdgeEndKey    = std::pair<uint32_t, int>;                                          // (edge slot, 0 = Start end / 1 = End end)
    using ContactKey    = std::tuple<uint32_t, int, int>;                                    // (edge slot, side, end)
    using FaceVertexKey = std::pair<uint32_t, uint32_t>;                                     // (face slot, vertex slot)

    //--------------------------------------------------------------------------------------------------------------------
    //                                                REBUILD
    //--------------------------------------------------------------------------------------------------------------------

    class BlendRebuild
    {
    public:
        BlendRebuild(ModelPartition& Partition, BodyToken Body, const BlendSettings& Settings, BlendOutcome& Outcome)
            : Partition(Partition), Body(Body), Settings(Settings), Outcome(Outcome), Inventory(Partition.CollectInventory(Body))
        {
            for (VertexToken Vertex : Inventory.Vertices)
            {
                const Vector3 Position = Partition.Vertices.Resolve(Vertex).Position;
                Scale = std::max({ Scale, std::fabs(Position.X), std::fabs(Position.Y), std::fabs(Position.Z) });
            }
            Scale     = std::max(Scale, Settings.Size);
            Tolerance = 1.0e-9 * Scale;
            Agreement = 1.0e-8 * Scale;
        }

        [[nodiscard]] std::string Run(const std::vector<EdgeToken>& Edges)
        {
            // ① Sheets.
            for (EdgeToken Edge : Edges)
            {
                if (SheetIndex.count(Edge.Slot) != 0) continue;
                const bool Member = std::find(Inventory.Edges.begin(), Inventory.Edges.end(), Edge) != Inventory.Edges.end();
                if (!Member) return "edge " + LabelToken("E", Edge) + " is not in the body";
                EdgeBlendSheetOutcome Resolved = ResolveEdgeBlendSheet(Partition, Edge, Settings);
                if (!Resolved.Sheet) return Resolved.Refusal;
                SheetIndex[Edge.Slot] = static_cast<int>(Outcome.Sheets.size());
                Outcome.Statistics.MaxSheetResidual = std::max(Outcome.Statistics.MaxSheetResidual, Resolved.Sheet->MaxResidual);
                Outcome.Sheets.push_back(*Resolved.Sheet);
            }
            if (Outcome.Sheets.empty()) return "no edges selected";
            Outcome.Statistics.BlendedEdges = static_cast<int>(Outcome.Sheets.size());

            // ② Vertex resolution, ③ pieces, ④ faces.
            for (VertexToken Vertex : Inventory.Vertices)
            {
                const std::string Refusal = ResolveVertex(Vertex);
                if (!Refusal.empty()) return "vertex " + LabelToken("V", Vertex) + ": " + Refusal;
            }
            if (std::string Refusal = ResolvePieces(); !Refusal.empty()) return Refusal;
            if (std::string Refusal = ComposeFaces(); !Refusal.empty()) return Refusal;
            return {};
        }

        BodyBlueprint                 Blueprint;
        std::vector<BlendFaceHistory> Provenance;

    private:
        ModelPartition&      Partition;
        BodyToken            Body;
        const BlendSettings& Settings;
        BlendOutcome&        Outcome;
        BodyInventory        Inventory;
        double               Scale     = 1.0;                                                // [len] - Model size
        double               Tolerance = 1.0e-9;                                             // [len] - Coincidence
        double               Agreement = 1.0e-8;                                             // [len] - Corner / mitre agreement

        std::map<uint32_t, int>                    SheetIndex;                               // edge slot → sheet
        std::map<uint32_t, int>                    OriginalVertices;                         // vertex slot → blueprint vertex
        std::map<EdgeEndKey, int>                  TrimVertices;                             // sharp edge end → blueprint vertex
        std::map<ContactKey, int>                  ContactVertices;                          // contact end → blueprint vertex
        std::map<EdgeEndKey, std::vector<ChainLink>> Terminations;                           // blend end, side 0 → side 1
        std::map<FaceVertexKey, int>               CapEdges;                                 // third face corner → cap edge
        std::map<uint32_t, int>                    Pieces;                                   // sharp edge → blueprint edge
        std::map<std::pair<uint32_t, int>, int>    ContactPieces;                            // (edge, side) → blueprint edge
        std::vector<CornerPatch>                   Corners;
        std::set<EdgeEndKey>                       Extensions;                               // sharp edge ends a concave blend may lengthen

        //----------------------------------------------------------------------------------------------------------------
        //                                            BLUEPRINT RECORDS
        //----------------------------------------------------------------------------------------------------------------

        [[nodiscard]] Vector3 PositionOf(int Vertex) const { return Blueprint.Vertices[Vertex].Position; }

        int IssueVertex(Vector3 Position)
        {
            Blueprint.Vertices.push_back({ Position });
            return static_cast<int>(Blueprint.Vertices.size()) - 1;
        }

        int MapOriginalVertex(VertexToken Vertex)
        {
            const auto Found = OriginalVertices.find(Vertex.Slot);
            if (Found != OriginalVertices.end()) return Found->second;
            return OriginalVertices[Vertex.Slot] = IssueVertex(Partition.Vertices.Resolve(Vertex).Position);
        }

        int IssueEdge(const CurveGeometry& Curve, double Low, double High, int Start, int End)
        {
            Blueprint.Edges.push_back({ Curve, Low, High, Start, End });
            if (std::holds_alternative<EllipticalArc>(Curve)) ++Outcome.Statistics.EllipticalEdges;
            return static_cast<int>(Blueprint.Edges.size()) - 1;
        }

        // Straight edge between two blueprint vertices (−1 if they coincide).
        int IssueSegment(int From, int To)
        {
            const Vector3 Start = PositionOf(From), End = PositionOf(To);
            const double  Span  = Distance(Start, End);                                      // [len] - Segment length
            if (Span <= Tolerance) return -1;
            return IssueEdge(StraightLine{ Start, (End - Start) * (1.0 / Span) }, 0.0, Span, From, To);
        }

        // 📝 Shorter arc (≤ π of τ) of a conic between two of its points; −1 if an end is off the carrier.
        int IssueConicArc(const CurveGeometry& Curve, int From, int To)
        {
            const double Start = ProjectOntoCurve(Curve, PositionOf(From), 0.0);             // [-] - τ at From
            const double End   = ProjectOntoCurve(Curve, PositionOf(To), Start);             // [-] - τ at To, folded above Start
            if (Distance(EvaluateCurve(Curve, Start).Position, PositionOf(From)) > Agreement) return -1;
            if (Distance(EvaluateCurve(Curve, End).Position, PositionOf(To)) > Agreement) return -1;
            const double Sweep = End - Start;                                                // [rad] - In [0, 2π)
            if (Sweep <= 1.0e-12 || Sweep >= 2.0 * std::numbers::pi - 1.0e-12) return -1;
            if (Sweep <= std::numbers::pi) return IssueEdge(Curve, Start, End, From, To);
            return IssueEdge(Curve, End, Start + 2.0 * std::numbers::pi, To, From);
        }

        // 💡 Exact section of two carriers between two blueprint vertices lying on both (mitres, caps): two planes → segment;
        //    an analytic SSI branch through both → that line or conic; otherwise the marched branch through both becomes an
        //    exact intersection-curve carrier (Newton onto both surfaces per evaluation) trimmed to them. The SSI window
        //    and step are scaled to the chord, not to the model, so small blends on large parts still resolve. −1 if no
        //    branch passes through both points (e.g. the surfaces are tangent at an end and the march stops short).
        int IssueSection(const SurfaceGeometry& First, const SurfaceGeometry& Second, int From, int To)
        {
            const Vector3 Start = PositionOf(From), End = PositionOf(To);
            const double  Chord = Distance(Start, End);                                      // [len] - Endpoint separation
            if (Chord <= Tolerance) return -1;
            if (std::holds_alternative<PlaneSurface>(First) && std::holds_alternative<PlaneSurface>(Second)) return IssueSegment(From, To);
            const auto Window = [&](const SurfaceGeometry& Surface)
            {
                const Vector2            Near   = ProjectOntoSurface(Surface, Start, {});
                const Vector2            Far    = ProjectOntoSurface(Surface, End, Near);
                const SurfaceSample      Sample = EvaluateSurface(Surface, (Near + Far) * 0.5);
                const SurfacePeriodicity Period = ResolvePeriodicity(Surface);
                double MarginU = 1.5 * Chord / std::max(Length(Sample.DerivativeU), 1.0e-12);  // [-] - Chord in 𝑢 units
                double MarginV = 1.5 * Chord / std::max(Length(Sample.DerivativeV), 1.0e-12);  // [-] - Chord in 𝑣 units
                if (Period.PeriodU > 0.0) MarginU = std::min(MarginU, 0.25 * Period.PeriodU);
                if (Period.PeriodV > 0.0) MarginV = std::min(MarginV, 0.25 * Period.PeriodV);
                ParameterBox Box{ { std::min(Near.X, Far.X) - MarginU, std::min(Near.Y, Far.Y) - MarginV }, { std::max(Near.X, Far.X) + MarginU, std::max(Near.Y, Far.Y) + MarginV } };
                if (std::holds_alternative<SphereSurface>(Surface))
                {
                    Box.Low.Y  = std::max(Box.Low.Y, -0.5 * std::numbers::pi);
                    Box.High.Y = std::min(Box.High.Y, 0.5 * std::numbers::pi);
                }
                return Box;
            };
            SurfaceIntersectionSettings Settings;
            Settings.ChordTolerance = 1.0e-3 * Chord;
            Settings.MaximumStep    = 0.125 * Chord;
            const SurfaceIntersectionOutcome Section = IntersectSurfaces(First, Window(First), Second, Window(Second), Settings);
            if (Section.CoincidentCondition) return -1;
            for (const IntersectionBranch& Branch : Section.Branches)
            {
                if (Branch.Category != BranchCategory::Traced)
                {
                    if (DistanceToCurve(Branch.Curve, Start) > Agreement || DistanceToCurve(Branch.Curve, End) > Agreement) continue;
                    if (std::holds_alternative<StraightLine>(Branch.Curve)) return IssueSegment(From, To);
                    return IssueConicArc(Branch.Curve, From, To);
                }
                if (Branch.Chart.size() < 2 || Branch.StartTermination == BranchTermination::Limit || Branch.EndTermination == BranchTermination::Limit) continue;
                std::vector<Vector3> Guide;
                for (const IntersectionChartPoint& Point : Branch.Chart) Guide.push_back(Point.Position);
                // 📝 Where the carriers are tangent (two fillets touching the shared face at M) the march stops just short of
                //    the point with a singular termination. The endpoint is exact on both surfaces, so it closes the guide:
                //    the normal-plane Newton at that knot starts on a solution, and acceptance below re-checks every sample.
                if (!Branch.ClosedCondition)
                {
                    for (const Vector3& Endpoint : { Start, End })
                    {
                        const double Reach = 0.25 * Chord;                                   // [len] - How short a march may stop
                        if (Branch.StartTermination == BranchTermination::Singular && Distance(Guide.front(), Endpoint) > Tolerance && Distance(Guide.front(), Endpoint) < Reach) Guide.insert(Guide.begin(), Endpoint);
                        else if (Branch.EndTermination == BranchTermination::Singular && Distance(Guide.back(), Endpoint) > Tolerance && Distance(Guide.back(), Endpoint) < Reach) Guide.push_back(Endpoint);
                    }
                }
                const CurveGeometry Carrier = BuildIntersectionCurve(First, Second, Guide, Branch.ClosedCondition);
                if (Branch.ClosedCondition) { if (const int Arc = IssueConicArc(Carrier, From, To); Arc >= 0) return Arc; continue; }
                const auto [Low, High] = MeasureIntersectionCurveRange(*std::get<IntersectionCurve>(Carrier).Trace);
                const double Near = ProjectOntoCurve(Carrier, Start, Low);                   // [-] - τ at From
                const double Far  = ProjectOntoCurve(Carrier, End, Low);                     // [-] - τ at To
                if (Distance(EvaluateCurve(Carrier, Near).Position, Start) > Agreement || Distance(EvaluateCurve(Carrier, Far).Position, End) > Agreement) continue;
                if (std::fabs(Far - Near) <= 1.0e-12) continue;
                ++Outcome.Statistics.TracedSections;
                return Near < Far ? IssueEdge(Carrier, Near, Far, From, To) : IssueEdge(Carrier, Far, Near, To, From);
            }
            return -1;
        }

        // Largest distance of an issued edge from a surface over Samples interior points (mitre / cap acceptance).
        [[nodiscard]] double MeasureEdgeDeparture(int Edge, const SurfaceGeometry& Surface, int Samples) const
        {
            const BlueprintEdge& Block = Blueprint.Edges[Edge];
            double Worst = 0.0;                                                              // [len] - Largest departure
            for (int Sample = 1; Sample <= Samples; ++Sample)
            {
                const Vector3 Point = EvaluateCurve(Block.Curve, Block.Low + (Block.High - Block.Low) * Sample / (Samples + 1.0)).Position;
                Worst = std::max(Worst, Distance(ProjectPointOntoSurface(Surface, Point), Point));
            }
            return Worst;
        }

        [[nodiscard]] Orientation LinkSense(const ChainLink& Link) const
        {
            return Blueprint.Edges[Link.Edge].Start == Link.From ? Orientation::Forward : Orientation::Reversed;
        }

        [[nodiscard]] int UseStart(const BlueprintUse& Use) const
        {
            return Use.Sense == Orientation::Forward ? Blueprint.Edges[Use.Edge].Start : Blueprint.Edges[Use.Edge].End;
        }

        [[nodiscard]] int UseEnd(const BlueprintUse& Use) const
        {
            return Use.Sense == Orientation::Forward ? Blueprint.Edges[Use.Edge].End : Blueprint.Edges[Use.Edge].Start;
        }

        //----------------------------------------------------------------------------------------------------------------
        //                                            TOPOLOGY QUERIES
        //----------------------------------------------------------------------------------------------------------------

        [[nodiscard]] FaceToken FaceOfFin(FinToken Fin) const { return Partition.Loops.Resolve(Partition.Fins.Resolve(Fin).Loop).Face; }

        [[nodiscard]] std::pair<FaceToken, FaceToken> FacesOfEdge(EdgeToken Edge) const
        {
            const FinToken Fin = Partition.Edges.Resolve(Edge).RadialFin;
            return { FaceOfFin(Fin), FaceOfFin(Partition.RadialMate(Fin)) };
        }

        [[nodiscard]] const SurfaceGeometry& SurfaceOf(FaceToken Face) const { return Partition.Surfaces.Resolve(Partition.Faces.Resolve(Face).Surface); }

        [[nodiscard]] const EdgeBlendSheet* SheetOf(EdgeToken Edge) const
        {
            const auto Found = SheetIndex.find(Edge.Slot);
            return Found == SheetIndex.end() ? nullptr : &Outcome.Sheets[Found->second];
        }

        [[nodiscard]] static int SideOn(const EdgeBlendSheet& Sheet, FaceToken Face) { return Sheet.Faces[0] == Face ? 0 : (Sheet.Faces[1] == Face ? 1 : -1); }

        [[nodiscard]] int EndAt(EdgeToken Edge, VertexToken Vertex) const { return Partition.Edges.Resolve(Edge).Start == Vertex ? 0 : 1; }

        // Point where a curve meets a surface nearest a vertex (lines searched within Reach of it, conics over a turn).
        [[nodiscard]] std::optional<Vector3> CrossCurveSurface(const CurveGeometry& Curve, Vector3 Near, const SurfaceGeometry& Surface) const
        {
            double Low = 0.0, High = 2.0 * std::numbers::pi;                                 // [-] - Search window
            if (!PeriodicCondition(Curve))
            {
                const double Centre = ProjectOntoCurve(Curve, Near, 0.0);                     // [-] - τ nearest the vertex
                Low  = Centre - 4.0 * Scale;
                High = Centre + 4.0 * Scale;
            }
            const CurveSurfaceOutcome Crossing = IntersectCurveSurface(Curve, Low, High, Surface);
            std::optional<Vector3> Best;
            for (const CurveSurfaceCrossing& Point : Crossing.Crossings)
            {
                if (!Best || Distance(Point.Position, Near) < Distance(*Best, Near)) Best = Point.Position;
            }
            return Best;
        }

        // Closest approach of two straight contacts on one face (chamfer mitres and apexes).
        [[nodiscard]] std::optional<Vector3> MeetLines(const CurveGeometry& First, const CurveGeometry& Second)
        {
            const auto* LineA = std::get_if<StraightLine>(&First);
            const auto* LineB = std::get_if<StraightLine>(&Second);
            if (!LineA || !LineB) return std::nullopt;
            const Vector3 Offset      = LineA->Origin - LineB->Origin;                       // [len] - o₁ − o₂
            const double  Cosine      = Dot(LineA->Direction, LineB->Direction);             // [-]   - d₁·d₂
            const double  Denominator = 1.0 - Cosine * Cosine;                               // [-]   - sin²
            if (Denominator < 1.0e-20) return std::nullopt;
            const double  ParameterA = (Cosine * Dot(LineB->Direction, Offset) - Dot(LineA->Direction, Offset)) / Denominator;
            const double  ParameterB = Dot(LineB->Direction, Offset) + ParameterA * Cosine;
            const Vector3 PointA     = LineA->Origin + LineA->Direction * ParameterA;
            const Vector3 PointB     = LineB->Origin + LineB->Direction * ParameterB;
            Outcome.Statistics.MaxVertexResidual = std::max(Outcome.Statistics.MaxVertexResidual, Distance(PointA, PointB));
            if (Distance(PointA, PointB) > Agreement) return std::nullopt;
            return (PointA + PointB) * 0.5;
        }

        [[nodiscard]] double DistanceToCurve(const CurveGeometry& Curve, Vector3 Point) const
        {
            return Distance(Point, EvaluateCurve(Curve, ProjectOntoCurve(Curve, Point, 0.0)).Position);
        }

        bool Agree(double Gap)
        {
            Outcome.Statistics.MaxVertexResidual = std::max(Outcome.Statistics.MaxVertexResidual, Gap);
            return Gap <= Agreement;
        }

        //----------------------------------------------------------------------------------------------------------------
        //                                            VERTEX RESOLUTION
        //----------------------------------------------------------------------------------------------------------------

        [[nodiscard]] std::string ResolveVertex(VertexToken Vertex)
        {
            std::vector<std::pair<EdgeToken, int>> Ends, Blended, Sharp;
            for (EdgeToken Edge : Inventory.Edges)
            {
                const EdgeProfile& Profile = Partition.Edges.Resolve(Edge);
                if (Profile.Start == Vertex) Ends.push_back({ Edge, 0 });
                if (Profile.End == Vertex)   Ends.push_back({ Edge, 1 });
            }
            for (const auto& End : Ends) (SheetOf(End.first) ? Blended : Sharp).push_back(End);
            if (Blended.empty()) return {};
            const bool Convex = SheetOf(Blended.front().first)->ConvexCondition;
            for (const auto& End : Blended)
            {
                if (SheetOf(End.first)->ConvexCondition != Convex) return "convex and concave blends meet (setback vertex blend, 🚧)";
            }
            if (Blended.size() == 2 && Blended[0].first != Blended[1].first && !Sharp.empty())
            {
                bool SeamsOnly = true;                                                       // [-] - Every other edge is a seam
                for (const auto& End : Sharp) { const auto Faces = FacesOfEdge(End.first); SeamsOnly = SeamsOnly && Faces.first == Faces.second; }
                if (SeamsOnly) return ResolveSeamCrossing(Vertex, Blended[0], Blended[1], Sharp);
            }
            if (ResolvePinch(Vertex, Blended)) return {};
            if (Ends.size() != 3) return "degree " + std::to_string(Ends.size()) + " vertex with blended edges (setback vertex blend, 🚧)";
            if (Blended.size() == 2 && Blended[0].first == Blended[1].first) return ResolveContinuation(Vertex, Blended[0].first, Sharp[0].first);
            if (Blended.size() == 1) return ResolveCap(Vertex, Blended[0], Sharp[0].first, Sharp[1].first);
            if (Blended.size() == 2) return ResolveMitre(Vertex, Blended[0], Blended[1], Sharp[0].first);
            return ResolveBallCorner(Vertex, Blended);
        }

        // 📝 Pinch: where the two supports of a blended edge are tangent (a singular point of their intersection, e.g. equal
        //    crossing bores), the dihedral flattens, the setback r·tan(θ/2) vanishes and the ball touches both supports at
        //    the vertex itself. When that holds for every blended edge here, each blend simply ends on the vertex with a
        //    zero-width section: both rails close on it, no termination edge, sharp edges (seams) keep their ends.
        [[nodiscard]] bool ResolvePinch(VertexToken Vertex, const std::vector<std::pair<EdgeToken, int>>& Blended)
        {
            const Vector3 Position = Partition.Vertices.Resolve(Vertex).Position;
            double Worst = 0.0;                                                              // [len] - Largest rail miss
            for (const auto& End : Blended)
            {
                const EdgeBlendSheet& Sheet = *SheetOf(End.first);
                const double τ = ProjectOntoCurve(Sheet.Contacts[0], Position, 0.0);         // [-] - Spine τ of the pinch
                Worst = std::max({ Worst, Distance(EvaluateCurve(Sheet.Contacts[0], τ).Position, Position), Distance(EvaluateCurve(Sheet.Contacts[1], τ).Position, Position) });
                if (Worst > Agreement) return false;
            }
            Agree(Worst);
            const int Pinched = MapOriginalVertex(Vertex);
            for (const auto& End : Blended)
            {
                for (int Side = 0; Side < 2; ++Side) ContactVertices[{ End.first.Slot, Side, End.second }] = Pinched;
                Terminations[{ End.first.Slot, End.second }] = {};
            }
            ++Outcome.Statistics.Vertices[static_cast<int>(VertexBlendCategory::Pinch)];
            return true;
        }

        // 📝 Section where the rail on Side crosses a seam of its support (secant on the signed offset across the seam,
        //    from the vertex's own section). A seam is a chart artefact, so the section may move off the old vertex; the
        //    vertex projection alone left the foot off the seam on any mouth without a mirror plane through the seam.
        [[nodiscard]] double ResolveSeamSection(const EdgeBlendSheet& Sheet, int Side, EdgeToken Seam, double Start) const
        {
            const CurveGeometry&   SeamCurve = Partition.Curves.Resolve(Partition.Edges.Resolve(Seam).Curve);
            const SurfaceGeometry& Support   = Partition.Surfaces.Resolve(Partition.Faces.Resolve(Sheet.Faces[Side]).Surface);
            const auto Across = [&](double τ)
            {
                const Vector3     Rail = EvaluateCurve(Sheet.Contacts[Side], τ).Position;
                const CurveSample Foot = EvaluateCurve(SeamCurve, ProjectOntoCurve(SeamCurve, Rail, 0.0));
                const Vector3     Normal = EvaluateSurfaceNormal(Support, ProjectOntoSurface(Support, Foot.Position, { 0.0, 0.0 }));
                return Dot(Rail - Foot.Position, Normalize(Cross(Normal, Foot.Derivative)));
            };
            double Previous = Start, Current = Start + 1.0e-4 * std::max(1.0, std::fabs(Start));
            double PreviousValue = Across(Previous), CurrentValue = Across(Current);
            for (int Iteration = 0; Iteration < 60 && CurrentValue != PreviousValue; ++Iteration)
            {
                const double Next = Current - CurrentValue * (Current - Previous) / (CurrentValue - PreviousValue);
                Previous = Current; PreviousValue = CurrentValue;
                Current  = Next;    CurrentValue  = Across(Current);
                if (std::fabs(Current - Previous) <= 1.0e-15 * (1.0 + std::fabs(Current))) break;
            }
            return Current;
        }

        // 📝 Closed rim through its seam vertex: the cross-section at the vertex is the blend face's seam.
        [[nodiscard]] std::string ResolveContinuation(VertexToken Vertex, EdgeToken Edge, EdgeToken Seam)
        {
            const EdgeBlendSheet& Sheet = *SheetOf(Edge);
            const auto [SeamFace, SeamMate] = FacesOfEdge(Seam);
            if (SeamFace != SeamMate) return "closed blended edge meets a sharp edge (rim ends need a seam)";
            const int SeamSide = SideOn(Sheet, SeamFace);
            if (SeamSide < 0) return "seam is not on a support of the blended rim";
            const Vector3 Position = Partition.Vertices.Resolve(Vertex).Position;
            const double  τ        = ResolveSeamSection(Sheet, SeamSide, Seam, ProjectOntoCurve(Sheet.Spine, Position, 0.0));
            const int     FootA    = IssueVertex(EvaluateCurve(Sheet.Contacts[0], τ).Position);
            const int     FootB    = IssueVertex(EvaluateCurve(Sheet.Contacts[1], τ).Position);
            const CurveGeometry& SeamCurve = Partition.Curves.Resolve(Partition.Edges.Resolve(Seam).Curve);
            if (!Agree(DistanceToCurve(SeamCurve, PositionOf(SeamSide == 0 ? FootA : FootB)))) return "seam does not cross the contact rail";
            int Section = -1;
            if (Settings.Profile == BlendProfile::Fillet)
            {
                const Vector3 Ball = EvaluateCurve(Sheet.Spine, τ).Position;                 // [len] - Ball centre
                const Vector3 Out  = PositionOf(FootA) - Ball;                               // [len] - To contact A
                Section = IssueConicArc(CircularArc{ ConstructFrame(Ball, Cross(Out, PositionOf(FootB) - Ball), Out), Sheet.Radius }, FootA, FootB);
            }
            else Section = IssueSegment(FootA, FootB);
            if (Section < 0) return "degenerate cross-section at the seam";
            for (int End = 0; End < 2; ++End)
            {
                ContactVertices[{ Edge.Slot, 0, End }] = FootA;
                ContactVertices[{ Edge.Slot, 1, End }] = FootB;
                Terminations[{ Edge.Slot, End }]       = { { Section, FootA, FootB } };
            }
            TrimVertices[{ Seam.Slot, EndAt(Seam, Vertex) }] = SeamSide == 0 ? FootA : FootB;
            ++Outcome.Statistics.Vertices[static_cast<int>(VertexBlendCategory::Continuation)];
            return {};
        }

        // 📝 A blended loop split into two edges at a vertex where only seams meet it (a radial bore mouth crossing the seams of
        //    both cylinders): the two sheets continue smoothly, so the joint is their shared cross-section at the vertex,
        //    and every seam is trimmed at the foot on its own support. The seams must pass through those feet; seams that
        //    cross the rails at different sections would need extra rail vertices and are refused.
        [[nodiscard]] std::string ResolveSeamCrossing(VertexToken Vertex, std::pair<EdgeToken, int> EndA, std::pair<EdgeToken, int> EndB, const std::vector<std::pair<EdgeToken, int>>& Seams)
        {
            const EdgeBlendSheet& SheetA = *SheetOf(EndA.first);
            const EdgeBlendSheet& SheetB = *SheetOf(EndB.first);
            const int SideB[2] = { SideOn(SheetB, SheetA.Faces[0]), SideOn(SheetB, SheetA.Faces[1]) };   // Side of B on A's faces
            if (SideB[0] < 0 || SideB[1] < 0) return "seam crossing: the blended edges do not share both supports";
            const Vector3 Position = Partition.Vertices.Resolve(Vertex).Position;
            const int     FirstSide = SideOn(SheetA, FacesOfEdge(Seams[0].first).first);
            if (FirstSide < 0) return "seam crossing: a seam is not on a support of the blend";
            const double  SpineA   = ResolveSeamSection(SheetA, FirstSide, Seams[0].first, ProjectOntoCurve(SheetA.Spine, Position, 0.0));
            const Vector3 Ball     = EvaluateCurve(SheetA.Spine, SpineA).Position;          // [len] - Ball centre
            if (!Agree(DistanceToCurve(SheetB.Spine, Ball))) return "seam crossing: spines do not share the section ball";
            const double SpineB = ProjectOntoCurve(SheetB.Spine, Ball, 0.0);                // [-] - Same section on B
            int Feet[2] = { -1, -1 };
            for (int Side = 0; Side < 2; ++Side)
            {
                const Vector3 FootA = EvaluateCurve(SheetA.Contacts[Side], SpineA).Position;
                const Vector3 FootB = EvaluateCurve(SheetB.Contacts[SideB[Side]], SpineB).Position;
                if (!Agree(Distance(FootA, FootB))) return "seam crossing: the blends' sections disagree (unequal sizes, setback 🚧)";
                Feet[Side] = IssueVertex((FootA + FootB) * 0.5);
            }
            for (const auto& Seam : Seams)
            {
                const int Side = SideOn(SheetA, FacesOfEdge(Seam.first).first);
                if (Side < 0) return "seam crossing: a seam is not on a support of the blend";
                if (!Agree(DistanceToCurve(Partition.Curves.Resolve(Partition.Edges.Resolve(Seam.first).Curve), PositionOf(Feet[Side]))))
                    return "seam crossing: a seam meets the rail away from the shared section (extra rail vertex, 🚧)";
                TrimVertices[{ Seam.first.Slot, Seam.second }] = Feet[Side];
            }
            int Section = -1;
            if (Settings.Profile == BlendProfile::Fillet)
            {
                const Vector3 Out = PositionOf(Feet[0]) - Ball;                              // [len] - To contact A
                Section = IssueConicArc(CircularArc{ ConstructFrame(Ball, Cross(Out, PositionOf(Feet[1]) - Ball), Out), SheetA.Radius }, Feet[0], Feet[1]);
            }
            else Section = IssueSegment(Feet[0], Feet[1]);
            if (Section < 0) return "seam crossing: degenerate cross-section";
            ContactVertices[{ EndA.first.Slot, 0, EndA.second }] = Feet[0];
            ContactVertices[{ EndA.first.Slot, 1, EndA.second }] = Feet[1];
            Terminations[{ EndA.first.Slot, EndA.second }]       = { { Section, Feet[0], Feet[1] } };
            ContactVertices[{ EndB.first.Slot, SideB[0], EndB.second }] = Feet[0];
            ContactVertices[{ EndB.first.Slot, SideB[1], EndB.second }] = Feet[1];
            Terminations[{ EndB.first.Slot, EndB.second }]       = { { Section, Feet[SideB[0] == 0 ? 0 : 1], Feet[SideB[0] == 0 ? 1 : 0] } };
            ++Outcome.Statistics.Vertices[static_cast<int>(VertexBlendCategory::Continuation)];
            return {};
        }

        // 📝 One blended edge: the blend ends on the third face; cap = carrier ∩ that plane, from contact A to contact B.
        [[nodiscard]] std::string ResolveCap(VertexToken Vertex, std::pair<EdgeToken, int> End, EdgeToken First, EdgeToken Second)
        {
            const EdgeBlendSheet& Sheet = *SheetOf(End.first);
            const auto FacesFirst  = FacesOfEdge(First);
            const auto FacesSecond = FacesOfEdge(Second);
            FaceToken Third = {};
            for (FaceToken Face : { FacesFirst.first, FacesFirst.second })
            {
                if (Face != Sheet.Faces[0] && Face != Sheet.Faces[1]) Third = Face;
            }
            if (!Third.PresentCondition() || (FacesSecond.first != Third && FacesSecond.second != Third)) return "cap: no common third face";
            const auto*   Plane    = std::get_if<PlaneSurface>(&SurfaceOf(Third));
            const Vector3 Position = Partition.Vertices.Resolve(Vertex).Position;
            int Feet[2] = { -1, -1 };
            for (int Side = 0; Side < 2; ++Side)
            {
                const std::optional<Vector3> Foot = CrossCurveSurface(Sheet.Contacts[Side], Position, SurfaceOf(Third));
                if (!Foot) return "cap: contact curve misses the third face";
                Feet[Side] = IssueVertex(*Foot);
            }
            // 📝 Cap = blend carrier ∩ third face between the feet. A cylinder on a plane is a closed-form conic; every
            //    other pair (torus or cone blends, curved third faces) goes through the exact section primitive.
            int Cap = -1;
            const auto* Cylinder = std::get_if<CylinderSurface>(&Sheet.Surface);
            if (Cylinder && Plane)
            {
                const std::optional<CurveGeometry> Conic = SectionCylinderByPlane(*Cylinder, Plane->Frame.Origin, Plane->Frame.ZAxis);
                if (Conic) Cap = IssueConicArc(*Conic, Feet[0], Feet[1]);
            }
            else Cap = IssueSection(Sheet.Surface, SurfaceOf(Third), Feet[0], Feet[1]);
            if (Cap < 0) return std::string("cap: no exact section of the ") + SurfaceCategoryLabel(Sheet.Surface) + " blend and the " + SurfaceCategoryLabel(SurfaceOf(Third)) + " face through both feet";
            if (!Agree(MeasureEdgeDeparture(Cap, Sheet.Surface, 5)) || !Agree(MeasureEdgeDeparture(Cap, SurfaceOf(Third), 5))) return "cap curve leaves the blend or the third face";
            for (EdgeToken Sharp : { First, Second })
            {
                const auto Faces = FacesOfEdge(Sharp);
                const FaceToken Support = Faces.first == Third ? Faces.second : Faces.first;
                const int Side = SideOn(Sheet, Support);
                if (Side < 0) return "cap: sharp edge not on a support";
                TrimVertices[{ Sharp.Slot, EndAt(Sharp, Vertex) }] = Feet[Side];
                if (!Sheet.ConvexCondition) Extensions.insert({ Sharp.Slot, EndAt(Sharp, Vertex) });   // added material may run past the edge's end
            }
            ContactVertices[{ End.first.Slot, 0, End.second }] = Feet[0];
            ContactVertices[{ End.first.Slot, 1, End.second }] = Feet[1];
            Terminations[{ End.first.Slot, End.second }]       = { { Cap, Feet[0], Feet[1] } };
            CapEdges[{ Third.Slot, Vertex.Slot }]              = Cap;
            ++Outcome.Statistics.Vertices[static_cast<int>(VertexBlendCategory::Cap)];
            return {};
        }

        // 📝 Two blended edges and one sharp edge: P where both outer contacts meet the sharp edge, M where both blends
        //    touch the shared face; the mitre M–P lies on the plane through the corner ball centre O, M and P.
        [[nodiscard]] std::string ResolveMitre(VertexToken Vertex, std::pair<EdgeToken, int> EndA, std::pair<EdgeToken, int> EndB, EdgeToken Sharp)
        {
            const EdgeBlendSheet& SheetA = *SheetOf(EndA.first);
            const EdgeBlendSheet& SheetB = *SheetOf(EndB.first);
            FaceToken Shared = {};
            for (FaceToken Face : SheetA.Faces) if (SideOn(SheetB, Face) >= 0) Shared = Face;
            if (!Shared.PresentCondition()) return "mitre: blended edges share no face";
            const int SharedA = SideOn(SheetA, Shared), SharedB = SideOn(SheetB, Shared);
            const int OuterA  = 1 - SharedA, OuterB = 1 - SharedB;
            const FaceToken FaceA = SheetA.Faces[OuterA], FaceB = SheetB.Faces[OuterB];
            const Vector3 Position = Partition.Vertices.Resolve(Vertex).Position;
            {
                const Vector3 NormalA = EvaluateSurfaceNormal(SurfaceOf(FaceA), ProjectOntoSurface(SurfaceOf(FaceA), Position, {}));
                const Vector3 NormalB = EvaluateSurfaceNormal(SurfaceOf(FaceB), ProjectOntoSurface(SurfaceOf(FaceB), Position, {}));
                if (Length(Cross(NormalA, NormalB)) < 1.0e-9) return ResolveSmoothJoint(Vertex, EndA, EndB, Sharp, SharedA, SharedB);
            }
            const std::optional<Vector3> PointA = CrossCurveSurface(SheetA.Contacts[OuterA], Position, SurfaceOf(FaceB));
            const std::optional<Vector3> PointB = CrossCurveSurface(SheetB.Contacts[OuterB], Position, SurfaceOf(FaceA));
            if (!PointA || !PointB) return "mitre: outer contacts miss the far faces";
            if (!Agree(Distance(*PointA, *PointB))) return "unequal mitre: outer contacts meet the sharp edge apart (setback, 🚧)";
            const int Outer = IssueVertex((*PointA + *PointB) * 0.5);
            int Inner = -1, Mitre = -1;
            if (Settings.Profile == BlendProfile::Fillet)
            {
                const std::optional<Vector3> Ball = CrossCurveSurface(SheetA.Spine, Position, SheetB.Offsets[OuterB]);
                if (!Ball || !Agree(DistanceToCurve(SheetB.Spine, *Ball))) return "mitre: spines do not meet at a corner ball";
                const Vector3 FootA = EvaluateCurve(SheetA.Contacts[SharedA], ProjectOntoCurve(SheetA.Spine, *Ball, 0.0)).Position;
                const Vector3 FootB = EvaluateCurve(SheetB.Contacts[SharedB], ProjectOntoCurve(SheetB.Spine, *Ball, 0.0)).Position;
                if (!Agree(Distance(FootA, FootB))) return "mitre: blends touch the shared face apart";
                Inner = IssueVertex((FootA + FootB) * 0.5);
                // 📝 Both fillets touch the shared face at M, so they are tangent there and A ∩ B is singular at that end.
                //    Mirror-image blends (equal radii, symmetric supports — patterned cuts) meet on the plane through the
                //    corner ball O, M and P, which cuts A transversally: try that section first (two cylinders give the
                //    closed-form ellipse), then the general A ∩ B; the acceptance test below decides which one is true.
                const Vector3 Normal = Cross(PositionOf(Inner) - *Ball, PositionOf(Outer) - *Ball);
                const auto*   CylinderA = std::get_if<CylinderSurface>(&SheetA.Surface);
                if (CylinderA && std::holds_alternative<CylinderSurface>(SheetB.Surface))
                {
                    if (const std::optional<CurveGeometry> Conic = SectionCylinderByPlane(*CylinderA, *Ball, Normal)) Mitre = IssueConicArc(*Conic, Inner, Outer);
                }
                else if (Length(Normal) > Tolerance * Tolerance)
                {
                    const PlaneSurface Bisector{ ConstructFrame(*Ball, Normal, PositionOf(Inner) - *Ball) };
                    Mitre = IssueSection(SheetA.Surface, Bisector, Inner, Outer);
                    if (Mitre >= 0 && MeasureEdgeDeparture(Mitre, SheetB.Surface, 5) > Agreement) Mitre = -1;
                }
                if (Mitre < 0 && !(CylinderA && std::holds_alternative<CylinderSurface>(SheetB.Surface))) Mitre = IssueSection(SheetA.Surface, SheetB.Surface, Inner, Outer);
                if (Mitre < 0) return std::string("mitre: no exact section of the ") + SurfaceCategoryLabel(SheetA.Surface) + " and " + SurfaceCategoryLabel(SheetB.Surface)
                                    + " fillets from the shared face to the sharp edge (asymmetric fillets are tangent at the shared face, 🚧)";
            }
            else
            {
                // 📝 M = contact A on the shared face ∩ chamfer B (lines meet in closed form; circles and other contacts through
                //    the curve–surface solver, confirmed on contact B). Chamfers cut the shared face at an angle, so A ∩ B is
                //    transversal at M and the exact section primitive applies to every carrier pair.
                std::optional<Vector3> Meet = MeetLines(SheetA.Contacts[SharedA], SheetB.Contacts[SharedB]);
                if (!Meet)
                {
                    Meet = CrossCurveSurface(SheetA.Contacts[SharedA], Position, SheetB.Surface);
                    if (Meet && !Agree(DistanceToCurve(SheetB.Contacts[SharedB], *Meet))) Meet.reset();
                }
                if (!Meet) return "mitre: chamfer contacts on the shared face do not meet";
                Inner = IssueVertex(*Meet);
                Mitre = IssueSection(SheetA.Surface, SheetB.Surface, Inner, Outer);
                if (Mitre < 0) return std::string("mitre: no exact section of the ") + SurfaceCategoryLabel(SheetA.Surface) + " and " + SurfaceCategoryLabel(SheetB.Surface) + " chamfers";
            }
            if (Mitre < 0) return "mitre curve is degenerate";
            if (!Agree(MeasureEdgeDeparture(Mitre, SheetA.Surface, 5)) || !Agree(MeasureEdgeDeparture(Mitre, SheetB.Surface, 5))) return "mitre curve leaves a blend";
            TrimVertices[{ Sharp.Slot, EndAt(Sharp, Vertex) }] = Outer;
            if (!SheetA.ConvexCondition) Extensions.insert({ Sharp.Slot, EndAt(Sharp, Vertex) });
            for (const auto& [End, SharedSide] : { std::pair{ EndA, SharedA }, std::pair{ EndB, SharedB } })
            {
                ContactVertices[{ End.first.Slot, SharedSide, End.second }]     = Inner;
                ContactVertices[{ End.first.Slot, 1 - SharedSide, End.second }] = Outer;
                const int From = SharedSide == 0 ? Inner : Outer;
                Terminations[{ End.first.Slot, End.second }] = { { Mitre, From, From == Inner ? Outer : Inner } };
            }
            ++Outcome.Statistics.Vertices[static_cast<int>(VertexBlendCategory::Mitre)];
            return {};
        }

        // 📝 Tangent-continuous chain (line ↔ arc across a smooth sharp edge, e.g. a slot rim or a rounded-rectangle loop):
        //    the two blends share their cross-section at the vertex, so the joint is that section — circle (fillet) or chord
        //    (chamfer). Every point comes from a transversal operation: the ball is spine A ∩ the edge's normal plane at
        //    the vertex, the feet are the contacts at that τ. (The mitre construction would use contact ∩ far face and
        //    spine ∩ offset, which are tangent here and only √ε accurate — 1e-8 gaps that break the SP-curves.)
        [[nodiscard]] std::string ResolveSmoothJoint(VertexToken Vertex, std::pair<EdgeToken, int> EndA, std::pair<EdgeToken, int> EndB, EdgeToken Sharp, int SharedA, int SharedB)
        {
            const EdgeBlendSheet& SheetA = *SheetOf(EndA.first);
            const EdgeBlendSheet& SheetB = *SheetOf(EndB.first);
            const int OuterA = 1 - SharedA, OuterB = 1 - SharedB;
            const Vector3        Position = Partition.Vertices.Resolve(Vertex).Position;
            const EdgeProfile&   Profile  = Partition.Edges.Resolve(EndA.first);
            const CurveGeometry& Carrier  = Partition.Curves.Resolve(Profile.Curve);
            const Vector3 Tangent = Normalize(EvaluateCurve(Carrier, ProjectOntoCurve(Carrier, Position, Profile.IntervalLow)).Derivative);
            const Vector3 Pick    = std::fabs(Tangent.X) < 0.6 ? Vector3{ 1.0, 0.0, 0.0 } : Vector3{ 0.0, 1.0, 0.0 };
            const PlaneSurface Section{ ConstructFrame(Position, Tangent, Normalize(Cross(Tangent, Pick))) };
            const std::optional<Vector3> Ball = CrossCurveSurface(SheetA.Spine, Position, Section);
            if (!Ball || !Agree(DistanceToCurve(SheetB.Spine, *Ball))) return "smooth joint: spines do not share the section ball";
            const double  SpineA = ProjectOntoCurve(SheetA.Spine, *Ball, 0.0);               // [-] - Section τ on A
            const double  SpineB = ProjectOntoCurve(SheetB.Spine, *Ball, 0.0);               // [-] - Section τ on B
            const Vector3 InnerA = EvaluateCurve(SheetA.Contacts[SharedA], SpineA).Position, InnerB = EvaluateCurve(SheetB.Contacts[SharedB], SpineB).Position;
            const Vector3 OuterPointA = EvaluateCurve(SheetA.Contacts[OuterA], SpineA).Position, OuterPointB = EvaluateCurve(SheetB.Contacts[OuterB], SpineB).Position;
            if (!Agree(Distance(InnerA, InnerB)) || !Agree(Distance(OuterPointA, OuterPointB))) return "smooth joint: the blends' sections disagree (unequal sizes, setback 🚧)";
            const int Inner = IssueVertex((InnerA + InnerB) * 0.5);
            const int Outer = IssueVertex((OuterPointA + OuterPointB) * 0.5);
            if (!Agree(DistanceToCurve(Partition.Curves.Resolve(Partition.Edges.Resolve(Sharp).Curve), PositionOf(Outer)))) return "smooth joint: section misses the smooth edge";
            int Joint = -1;
            if (Settings.Profile == BlendProfile::Fillet)
            {
                const Vector3 Out = PositionOf(Inner) - *Ball;                               // [len] - To the shared-face foot
                Joint = IssueConicArc(CircularArc{ ConstructFrame(*Ball, Cross(Out, PositionOf(Outer) - *Ball), Out), SheetA.Radius }, Inner, Outer);
            }
            else Joint = IssueSegment(Inner, Outer);
            if (Joint < 0) return "smooth joint: degenerate section";
            TrimVertices[{ Sharp.Slot, EndAt(Sharp, Vertex) }] = Outer;
            if (!SheetA.ConvexCondition) Extensions.insert({ Sharp.Slot, EndAt(Sharp, Vertex) });
            for (const auto& [End, SharedSide] : { std::pair{ EndA, SharedA }, std::pair{ EndB, SharedB } })
            {
                ContactVertices[{ End.first.Slot, SharedSide, End.second }]     = Inner;
                ContactVertices[{ End.first.Slot, 1 - SharedSide, End.second }] = Outer;
                const int From = SharedSide == 0 ? Inner : Outer;
                Terminations[{ End.first.Slot, End.second }] = { { Joint, From, From == Inner ? Outer : Inner } };
            }
            ++Outcome.Statistics.Vertices[static_cast<int>(VertexBlendCategory::Continuation)];
            return {};
        }

        // 📝 Three blended edges: fillet → sphere of the ball touching all three faces, bounded by the three end
        //    cross-sections (great circles); chamfer → three lines from the contact corners to the apex of the planes.
        [[nodiscard]] std::string ResolveBallCorner(VertexToken Vertex, const std::vector<std::pair<EdgeToken, int>>& Ends)
        {
            std::vector<FaceToken> Faces;
            for (const auto& End : Ends)
            {
                for (FaceToken Face : SheetOf(End.first)->Faces)
                {
                    if (std::find(Faces.begin(), Faces.end(), Face) == Faces.end()) Faces.push_back(Face);
                }
            }
            if (Faces.size() != 3) return "ball corner needs three distinct faces";
            const Vector3 Position = Partition.Vertices.Resolve(Vertex).Position;
            std::map<uint32_t, int> Feet;                                                    // face slot → blueprint vertex
            CornerPatch Patch;
            Patch.Vertex = Vertex;
            if (Settings.Profile == BlendProfile::Fillet)
            {
                // Corner ball centre: spine of the first edge ∩ the offset of its third face (from a sheet supporting it).
                const EdgeBlendSheet& First = *SheetOf(Ends[0].first);
                FaceToken Third = {};
                for (FaceToken Face : Faces) if (SideOn(First, Face) < 0) Third = Face;
                const SurfaceGeometry* ThirdOffset = nullptr;
                for (const auto& End : Ends)
                {
                    const EdgeBlendSheet& Sheet = *SheetOf(End.first);
                    if (SideOn(Sheet, Third) >= 0) ThirdOffset = &Sheet.Offsets[SideOn(Sheet, Third)];
                }
                if (!ThirdOffset) return "ball corner: no offset of the third face";
                const std::optional<Vector3> Ball = CrossCurveSurface(First.Spine, Position, *ThirdOffset);
                if (!Ball) return "ball corner: spine misses the third offset";
                for (const auto& End : Ends) if (!Agree(DistanceToCurve(SheetOf(End.first)->Spine, *Ball))) return "ball corner: spines do not meet";
                for (FaceToken Face : Faces) Feet[Face.Slot] = IssueVertex(ProjectPointOntoSurface(SurfaceOf(Face), *Ball));
                for (const auto& End : Ends)
                {
                    const EdgeBlendSheet& Sheet = *SheetOf(End.first);
                    const double τ = ProjectOntoCurve(Sheet.Spine, *Ball, 0.0);
                    for (int Side = 0; Side < 2; ++Side)
                    {
                        if (!Agree(Distance(EvaluateCurve(Sheet.Contacts[Side], τ).Position, PositionOf(Feet[Sheet.Faces[Side].Slot])))) return "ball corner: contacts disagree with the corner feet";
                    }
                    const int     From = Feet[Sheet.Faces[0].Slot], To = Feet[Sheet.Faces[1].Slot];
                    const Vector3 Out  = PositionOf(From) - *Ball;                           // [len] - To the first foot
                    const int     Arc  = IssueConicArc(CircularArc{ ConstructFrame(*Ball, Cross(Out, PositionOf(To) - *Ball), Out), Sheet.Radius }, From, To);
                    if (Arc < 0) return "ball corner: degenerate end section";
                    Terminations[{ End.first.Slot, End.second }] = { { Arc, From, To } };
                    Patch.Links.push_back(AgainstBlend(End, { Arc, From, To }));
                }
                const Vector3 Diagonal = Normalize(Position - *Ball);                        // [-] - Towards the old vertex
                const Vector3 Pick     = std::fabs(Diagonal.X) < 0.6 ? Vector3{ 1.0, 0.0, 0.0 } : Vector3{ 0.0, 1.0, 0.0 };
                Patch.Surface   = SphereSurface{ ConstructFrame(*Ball, Cross(Diagonal, Pick), -Diagonal), SheetOf(Ends[0].first)->Radius };
                Patch.Sense     = SheetOf(Ends[0].first)->ConvexCondition ? Orientation::Forward : Orientation::Reversed;
                Patch.Reference = { std::numbers::pi, 0.0 };
                Corners.push_back(Patch);
                ++Outcome.Statistics.CornerSpheres;
            }
            else
            {
                Vector3 Normals[3];
                double  Offsets[3];
                for (int Index = 0; Index < 3; ++Index)
                {
                    const auto* Plane = std::get_if<PlaneSurface>(&SheetOf(Ends[Index].first)->Surface);
                    if (!Plane) return "chamfer corner of non-planar chamfers (🚧)";
                    Normals[Index] = Plane->Frame.ZAxis;
                    Offsets[Index] = Dot(Plane->Frame.ZAxis, Plane->Frame.Origin);
                }
                const double Volume = Dot(Normals[0], Cross(Normals[1], Normals[2]));        // [-] - Triple product
                if (std::fabs(Volume) < 1.0e-12) return "chamfer planes meet in no single apex";
                const Vector3 Apex = (Cross(Normals[1], Normals[2]) * Offsets[0] + Cross(Normals[2], Normals[0]) * Offsets[1] + Cross(Normals[0], Normals[1]) * Offsets[2]) * (1.0 / Volume);
                const int ApexVertex = IssueVertex(Apex);
                for (FaceToken Face : Faces)
                {
                    std::vector<const CurveGeometry*> Contacts;
                    for (const auto& End : Ends)
                    {
                        const EdgeBlendSheet& Sheet = *SheetOf(End.first);
                        if (SideOn(Sheet, Face) >= 0) Contacts.push_back(&Sheet.Contacts[SideOn(Sheet, Face)]);
                    }
                    const std::optional<Vector3> Meet = Contacts.size() == 2 ? MeetLines(*Contacts[0], *Contacts[1]) : std::nullopt;
                    if (!Meet) return "chamfer corner: contacts do not meet on a face";
                    Feet[Face.Slot] = IssueVertex(*Meet);
                }
                std::map<uint32_t, int> Spokes;                                              // face slot → apex segment
                for (FaceToken Face : Faces)
                {
                    Spokes[Face.Slot] = IssueSegment(Feet[Face.Slot], ApexVertex);
                    if (Spokes[Face.Slot] < 0) return "chamfer corner: apex coincides with a contact corner";
                }
                for (const auto& End : Ends)
                {
                    const EdgeBlendSheet& Sheet = *SheetOf(End.first);
                    const int From = Feet[Sheet.Faces[0].Slot], To = Feet[Sheet.Faces[1].Slot];
                    Terminations[{ End.first.Slot, End.second }] = { { Spokes[Sheet.Faces[0].Slot], From, ApexVertex }, { Spokes[Sheet.Faces[1].Slot], ApexVertex, To } };
                }
            }
            for (const auto& End : Ends)
            {
                const EdgeBlendSheet& Sheet = *SheetOf(End.first);
                for (int Side = 0; Side < 2; ++Side) ContactVertices[{ End.first.Slot, Side, End.second }] = Feet[Sheet.Faces[Side].Slot];
            }
            ++Outcome.Statistics.Vertices[static_cast<int>(VertexBlendCategory::BallCorner)];
            return {};
        }

        // 📝 The blend face uses its termination at the X end (start of the side-0 fin) side 0 → side 1 and at the Y end
        //    side 1 → side 0; a corner face uses the same link the other way.
        [[nodiscard]] ChainLink AgainstBlend(std::pair<EdgeToken, int> End, ChainLink Link) const
        {
            const EdgeBlendSheet& Sheet = *SheetOf(End.first);
            const bool XEnd = Partition.FinStart(Sheet.Fins[0]) == (End.second == 0 ? Partition.Edges.Resolve(End.first).Start : Partition.Edges.Resolve(End.first).End);
            if (XEnd) std::swap(Link.From, Link.To);
            return Link;
        }

        //----------------------------------------------------------------------------------------------------------------
        //                                            PIECES
        //----------------------------------------------------------------------------------------------------------------

        [[nodiscard]] std::string ResolvePieces()
        {
            for (EdgeToken Edge : Inventory.Edges)
            {
                const EdgeProfile&   Profile = Partition.Edges.Resolve(Edge);
                const CurveGeometry& Curve   = Partition.Curves.Resolve(Profile.Curve);
                const bool           Closed  = Profile.Start == Profile.End;
                if (const EdgeBlendSheet* Sheet = SheetOf(Edge))
                {
                    for (int Side = 0; Side < 2; ++Side)
                    {
                        const auto StartFound = ContactVertices.find({ Edge.Slot, Side, 0 });
                        const auto EndFound   = ContactVertices.find({ Edge.Slot, Side, 1 });
                        if (StartFound == ContactVertices.end() || EndFound == ContactVertices.end()) return "edge " + LabelToken("E", Edge) + ": unresolved blend end";
                        const CurveGeometry& Contact = Sheet->Contacts[Side];
                        const int            From    = StartFound->second, To = EndFound->second;
                        if (Closed)
                        {
                            const double Low = ProjectOntoCurve(Contact, PositionOf(From), 0.0);
                            ContactPieces[{ Edge.Slot, Side }] = IssueEdge(Contact, Low, Low + 2.0 * std::numbers::pi, From, From);
                            continue;
                        }
                        // 📝 Contacts share the spine τ; along the edge τ must advance in the sheet's alignment, otherwise the
                        //    rail runs backwards (the blend is wider than the face or edge it rests on).
                        const bool   Along = Sheet->Alignment > 0.0;
                        const double Low   = ProjectOntoCurve(Contact, PositionOf(Along ? From : To), 0.0);
                        const double High  = ProjectOntoCurve(Contact, PositionOf(Along ? To : From), Low);
                        if (!(High - Low > Tolerance)) return "edge " + LabelToken("E", Edge) + ": contact rail runs backwards (blend wider than its face)";
                        ContactPieces[{ Edge.Slot, Side }] = IssueEdge(Contact, Low, High, Along ? From : To, Along ? To : From);
                    }
                    continue;
                }
                const auto StartFound = TrimVertices.find({ Edge.Slot, 0 });
                const auto EndFound   = TrimVertices.find({ Edge.Slot, 1 });
                const bool Trimmed    = StartFound != TrimVertices.end() || EndFound != TrimVertices.end();
                const int  From       = StartFound != TrimVertices.end() ? StartFound->second : MapOriginalVertex(Profile.Start);
                const int  To         = EndFound != TrimVertices.end() ? EndFound->second : MapOriginalVertex(Profile.End);
                if (!Trimmed) { Pieces[Edge.Slot] = IssueEdge(Curve, Profile.IntervalLow, Profile.IntervalHigh, From, To); continue; }
                if (Closed) return "edge " + LabelToken("E", Edge) + ": closed sharp edge at a blended vertex";
                // 📝 A convex blend only shortens a sharp edge. A concave blend adds material flush with the third face, so its
                //    end may lie past the old vertex: the edge is lengthened along its own carrier (the third face grows by the
                //    cap region); the self-clash gate after assembly rejects an extension that runs into the body.
                const double Span = Profile.IntervalHigh - Profile.IntervalLow;              // [-] - Original τ span
                // Fold periodic carriers at the middle of the gap the edge does not cover, so a short arc extended at its
                // start is not wrapped a full turn.
                const double Fold = PeriodicCondition(Curve) ? Profile.IntervalLow - 0.5 * (2.0 * std::numbers::pi - Span) : Profile.IntervalLow - 0.25 * Span;
                const double Low  = StartFound != TrimVertices.end() ? ProjectOntoCurve(Curve, PositionOf(From), Fold) : Profile.IntervalLow;
                const double High = EndFound != TrimVertices.end() ? ProjectOntoCurve(Curve, PositionOf(To), Low) : Profile.IntervalHigh;
                const bool   LowExtended  = Extensions.count({ Edge.Slot, 0 }) != 0;
                const bool   HighExtended = Extensions.count({ Edge.Slot, 1 }) != 0;
                if (!(High - Low > Tolerance) || (High > Profile.IntervalHigh + Agreement && !HighExtended) || (Low < Profile.IntervalLow - Agreement && !LowExtended))
                {
                    const double Overshoot = std::max({ Profile.IntervalLow - Low, High - Profile.IntervalHigh, Low - High }) * MeasureParameterScale(Curve);
                    char Detail[96];
                    std::snprintf(Detail, sizeof Detail, " (%s, overshoot %.3g)", CurveCategoryLabel(Curve), Overshoot);
                    return "edge " + LabelToken("E", Edge) + ": consumed by the blend (blend wider than the edge)" + Detail;
                }
                if (Distance(EvaluateCurve(Curve, Low).Position, PositionOf(From)) > Agreement || Distance(EvaluateCurve(Curve, High).Position, PositionOf(To)) > Agreement)
                {
                    return "edge " + LabelToken("E", Edge) + ": blend end is off the sharp edge's carrier";
                }
                Pieces[Edge.Slot] = IssueEdge(Curve, Low, High, From, To);
            }
            return {};
        }

        //----------------------------------------------------------------------------------------------------------------
        //                                            FACES
        //----------------------------------------------------------------------------------------------------------------

        [[nodiscard]] ParameterCurve FitUse(const SurfaceGeometry& Surface, int Edge, Vector2 Reference) const
        {
            const BlueprintEdge& Block  = Blueprint.Edges[Edge];
            const Vector3        Middle = EvaluateCurve(Block.Curve, 0.5 * (Block.Low + Block.High)).Position;
            const Vector2        Anchor = ProjectOntoSurface(Surface, Middle, Reference);
            return FitSurfaceCurve(Surface, Block.Curve, Block.Low, Block.High, { Anchor - Vector2{ 1.0, 1.0 }, Anchor + Vector2{ 1.0, 1.0 } }, Anchor);
        }

        [[nodiscard]] std::string DiagnoseLoop(const std::vector<BlueprintUse>& Loop, const std::string& Subject) const
        {
            for (size_t Index = 0; Index < Loop.size(); ++Index)
            {
                if (UseEnd(Loop[Index]) != UseStart(Loop[(Index + 1) % Loop.size()])) return Subject + ": loop does not close at use " + std::to_string(Index);
            }
            return {};
        }

        [[nodiscard]] std::string ComposeFaces()
        {
            // ① Original faces: contacts replace blended fins, sharp fins become their trimmed pieces, caps fill corners.
            for (FaceToken Face : Inventory.Faces)
            {
                const FaceProfile&     Profile = Partition.Faces.Resolve(Face);
                const SurfaceGeometry& Surface = Partition.Surfaces.Resolve(Profile.Surface);
                BlueprintFace Blue{ Surface, Profile.Sense, {} };
                for (LoopToken Loop : Profile.Loops)
                {
                    std::vector<FinToken> Fins;
                    Partition.EnumerateLoopFins(Loop, [&](FinToken Fin) { Fins.push_back(Fin); });
                    std::vector<BlueprintUse> Uses;
                    for (FinToken Fin : Fins)
                    {
                        const FinProfile& FinRecord = Partition.Fins.Resolve(Fin);
                        if (!FinRecord.SurfaceCurve) return "fin without an SP-curve on " + LabelToken("F", Face);
                        if (const EdgeBlendSheet* Sheet = SheetOf(FinRecord.Edge))
                        {
                            const int Side  = Sheet->Fins[0] == Fin ? 0 : 1;
                            const int Piece = ContactPieces[{ FinRecord.Edge.Slot, Side }];
                            const BlueprintEdge& Block = Blueprint.Edges[Piece];
                            const Orientation Sense = Sheet->Alignment > 0.0 ? FinRecord.Sense : InvertOrientation(FinRecord.Sense);
                            const CurveGeometry& Original = Partition.Curves.Resolve(Partition.Edges.Resolve(FinRecord.Edge).Curve);
                            const EdgeProfile&   Range    = Partition.Edges.Resolve(FinRecord.Edge);
                            const Vector3 Middle = EvaluateCurve(Block.Curve, 0.5 * (Block.Low + Block.High)).Position;
                            const double  Along  = std::clamp(ProjectOntoCurve(Original, Middle, Range.IntervalLow), Range.IntervalLow, Range.IntervalHigh);
                            Uses.push_back({ Piece, Sense, FitUse(Surface, Piece, EvaluateParameterCurve(*FinRecord.SurfaceCurve, Along).Position) });
                        }
                        else Uses.push_back({ Pieces[FinRecord.Edge.Slot], FinRecord.Sense, *FinRecord.SurfaceCurve });
                        const auto Cap = CapEdges.find({ Face.Slot, Partition.FinEnd(Fin).Slot });
                        if (Cap != CapEdges.end())
                        {
                            const int From = UseEnd(Uses.back());
                            const ChainLink Link{ Cap->second, From, Blueprint.Edges[Cap->second].Start == From ? Blueprint.Edges[Cap->second].End : Blueprint.Edges[Cap->second].Start };
                            const Vector2 Reference = EvaluateParameterCurve(*FinRecord.SurfaceCurve, Partition.FinParameterEnd(Fin)).Position;
                            Uses.push_back({ Link.Edge, LinkSense(Link), FitUse(Surface, Link.Edge, Reference) });
                        }
                    }
                    if (std::string Refusal = DiagnoseLoop(Uses, LabelToken("F", Face)); !Refusal.empty()) return Refusal;
                    Blue.Loops.push_back(std::move(Uses));
                }
                Blueprint.Faces.push_back(std::move(Blue));
                Provenance.push_back({ {}, BlendFaceOrigin::Modified, Face, {}, {} });
            }

            // ② Blend faces: contact A reversed, termination at X, contact B, termination at Y reversed.
            for (const EdgeBlendSheet& Sheet : Outcome.Sheets)
            {
                const EdgeProfile& Profile = Partition.Edges.Resolve(Sheet.Edge);
                const int XEnd = Partition.FinStart(Sheet.Fins[0]) == Profile.Start ? 0 : 1;
                const int YEnd = 1 - XEnd;
                const int PieceA = ContactPieces[{ Sheet.Edge.Slot, 0 }], PieceB = ContactPieces[{ Sheet.Edge.Slot, 1 }];
                const Orientation SenseA = Sheet.Alignment > 0.0 ? Partition.Fins.Resolve(Sheet.Fins[0]).Sense : InvertOrientation(Partition.Fins.Resolve(Sheet.Fins[0]).Sense);
                const Orientation SenseB = Sheet.Alignment > 0.0 ? Partition.Fins.Resolve(Sheet.Fins[1]).Sense : InvertOrientation(Partition.Fins.Resolve(Sheet.Fins[1]).Sense);
                const BlueprintEdge& BlockA = Blueprint.Edges[PieceA];
                const double SpineX = SenseA == Orientation::Forward ? BlockA.Low : BlockA.High;   // [-] - τ of the X end
                const double SpineY = SenseA == Orientation::Forward ? BlockA.High : BlockA.Low;   // [-] - τ of the Y end
                const auto Unwrap = [&](Vector3 Point, double Expected)
                {
                    double τ = ProjectOntoCurve(Sheet.Spine, Point, 0.0);
                    if (PeriodicCondition(Sheet.Spine)) τ += 2.0 * std::numbers::pi * std::round((Expected - τ) / (2.0 * std::numbers::pi));
                    return τ;
                };
                const auto Reference = [&](int Edge, double Expected)
                {
                    const BlueprintEdge& Block = Blueprint.Edges[Edge];
                    return ResolveSheetChart(Sheet, Unwrap(EvaluateCurve(Block.Curve, 0.5 * (Block.Low + Block.High)).Position, Expected));
                };
                BlueprintFace Blue{ Sheet.Surface, Sheet.Sense, {} };
                std::vector<BlueprintUse> Uses;
                Uses.push_back({ PieceA, InvertOrientation(SenseA), FitUse(Sheet.Surface, PieceA, ResolveSheetChart(Sheet, 0.5 * (BlockA.Low + BlockA.High))) });
                for (const ChainLink& Link : Terminations[{ Sheet.Edge.Slot, XEnd }])
                {
                    Uses.push_back({ Link.Edge, LinkSense(Link), FitUse(Sheet.Surface, Link.Edge, Reference(Link.Edge, SpineX)) });
                }
                const BlueprintEdge& BlockB = Blueprint.Edges[PieceB];
                Uses.push_back({ PieceB, InvertOrientation(SenseB), FitUse(Sheet.Surface, PieceB, ResolveSheetChart(Sheet, 0.5 * (BlockB.Low + BlockB.High))) });
                const std::vector<ChainLink>& Back = Terminations[{ Sheet.Edge.Slot, YEnd }];
                for (auto Link = Back.rbegin(); Link != Back.rend(); ++Link)
                {
                    const ChainLink Reversed{ Link->Edge, Link->To, Link->From };
                    Uses.push_back({ Reversed.Edge, LinkSense(Reversed), FitUse(Sheet.Surface, Reversed.Edge, Reference(Reversed.Edge, SpineY)) });
                }
                if (std::string Refusal = DiagnoseLoop(Uses, "blend of " + LabelToken("E", Sheet.Edge)); !Refusal.empty()) return Refusal;
                Blue.Loops.push_back(std::move(Uses));
                Blueprint.Faces.push_back(std::move(Blue));
                Provenance.push_back({ {}, BlendFaceOrigin::Edge, {}, Sheet.Edge, {} });
                if (std::holds_alternative<PlaneSurface>(Sheet.Surface))         ++Outcome.Statistics.CarrierPlanes;
                else if (std::holds_alternative<CylinderSurface>(Sheet.Surface)) ++Outcome.Statistics.CarrierCylinders;
                else if (std::holds_alternative<ConeSurface>(Sheet.Surface))     ++Outcome.Statistics.CarrierCones;
                else if (std::holds_alternative<TorusSurface>(Sheet.Surface))    ++Outcome.Statistics.CarrierTori;
            }

            // ③ Corner faces: the end sections chained into one loop.
            for (CornerPatch& Patch : Corners)
            {
                std::vector<ChainLink> Order = { Patch.Links.front() };
                std::vector<bool>      Taken(Patch.Links.size(), false);
                Taken[0] = true;
                while (Order.size() < Patch.Links.size())
                {
                    bool Found = false;
                    for (size_t Index = 0; Index < Patch.Links.size() && !Found; ++Index)
                    {
                        if (!Taken[Index] && Patch.Links[Index].From == Order.back().To) { Order.push_back(Patch.Links[Index]); Taken[Index] = true; Found = true; }
                    }
                    if (!Found) return "corner at " + LabelToken("V", Patch.Vertex) + ": end sections do not chain";
                }
                BlueprintFace Blue{ Patch.Surface, Patch.Sense, {} };
                std::vector<BlueprintUse> Uses;
                for (const ChainLink& Link : Order) Uses.push_back({ Link.Edge, LinkSense(Link), FitUse(Patch.Surface, Link.Edge, Patch.Reference) });
                if (std::string Refusal = DiagnoseLoop(Uses, "corner at " + LabelToken("V", Patch.Vertex)); !Refusal.empty()) return Refusal;
                Blue.Loops.push_back(std::move(Uses));
                Blueprint.Faces.push_back(std::move(Blue));
                Provenance.push_back({ {}, BlendFaceOrigin::Vertex, {}, {}, Patch.Vertex });
            }
            return {};
        }
    };
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    PUBLIC FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

BlendOutcome BlendEdges(ModelPartition& Partition, BodyToken Body, const std::vector<EdgeToken>& Edges, const BlendSettings& Settings)
{
    const auto   Started = std::chrono::steady_clock::now();
    BlendOutcome Outcome;
    const auto   Finish  = [&](std::string Refusal)
    {
        Outcome.Refusal = Refusal.empty() ? std::string{} : "Blend: " + Refusal;
        Outcome.Statistics.Milliseconds = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - Started).count();
        return Outcome;
    };
    BlendRebuild Rebuild(Partition, Body, Settings, Outcome);
    const std::vector<EdgeToken> Selection = Settings.TangentChainEnabled ? GrowTangentChain(Partition, Body, Edges) : Edges;
    if (std::string Refusal = Rebuild.Run(Selection); !Refusal.empty()) return Finish(Refusal);

    // ⑤ Assembly and the atomic soundness gate (topology + geometry audit, genus unchanged).
    const AssemblyOutcome Assembly = AssembleBody(Partition, Rebuild.Blueprint, "Blend");
    if (!Assembly.Refusal.empty()) return Finish(Assembly.Refusal);
    Outcome.Audit = AuditBody(Partition, Assembly.Body);
    if (!Outcome.Audit.SoundCondition())
    {
        const AuditFinding& First = Outcome.Audit.Findings.front();
        return Finish("result fails audit " + std::string(First.Code) + " on " + First.Subject + " (" + First.Detail + ")");
    }
    const int Genus = AuditBody(Partition, Body).Genus;
    if (Outcome.Audit.Genus != Genus) return Finish("genus changed from " + std::to_string(Genus) + " to " + std::to_string(Outcome.Audit.Genus));
    // ⑥ Blends that reach each other (footprints overlapping on a shared face, tubes cutting through a neighbour) pass
    //    the local audit but leave the body passing through itself: refuse instead of returning it.
    if (std::string Clash = DetectSelfClash(Partition, Assembly.Body); !Clash.empty()) return Finish("neighbouring blends overlap: " + Clash);
    for (size_t Index = 0; Index < Assembly.Faces.size(); ++Index)
    {
        BlendFaceHistory History = Rebuild.Provenance[Index];
        History.Result = Assembly.Faces[Index];
        Outcome.FaceHistories.push_back(History);
    }
    Outcome.Body = Assembly.Body;
    return Finish({});
}

const char* VertexBlendCategoryLabel(VertexBlendCategory Category) noexcept
{
    switch (Category)
    {
        case VertexBlendCategory::Cap:          return "Cap";
        case VertexBlendCategory::Mitre:        return "Mitre";
        case VertexBlendCategory::BallCorner:   return "BallCorner";
        case VertexBlendCategory::Continuation: return "Continuation";
        case VertexBlendCategory::Pinch:        return "Pinch";
    }
    return "?";
}

const char* BlendFaceOriginLabel(BlendFaceOrigin Origin) noexcept
{
    switch (Origin)
    {
        case BlendFaceOrigin::Modified: return "Modified";
        case BlendFaceOrigin::Edge:     return "Edge";
        case BlendFaceOrigin::Vertex:   return "Vertex";
    }
    return "?";
}

} // namespace Frontier::SolidArcRemastered
