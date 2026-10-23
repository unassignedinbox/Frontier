/*====================================================================================================================================
                                                      TOPOLOGYREBUILD.CPP
====================================================================================================================================*/
// 🧩 Topology-changing face verbs by direct blueprint rebuild

//------------------------------------------------------------------------------------------------------------------------
//                                                    INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Engine/GeometryProcessing/Local/TopologyRebuild.h"

#include "Engine/GeometryProcessing/Construction/BodyAssembly.h"
#include "Engine/GeometryProcessing/Geometry/NurbsCarrier.h"
#include "Engine/GeometryProcessing/Geometry/ProjectedCurve.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <map>
#include <numbers>
#include <set>
#include <tuple>

namespace Frontier::GeometryProcessing
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    PRIVATE HELPERS
//------------------------------------------------------------------------------------------------------------------------

namespace
{
    constexpr double τPeriod = 2.0 * std::numbers::pi;                                        // [-] - Period of conic carriers

    // 📝 A body re-expressed as a blueprint in inventory order (vertex / edge / face indices = inventory positions), the
    //    common starting point of every rebuild verb.
    struct BodyDescription
    {
        BodyBlueprint           Blueprint   = {};                                            // [-] - Faithful copy
        BodyInventory           Inventory   = {};                                            // [-] - Source records
        std::map<uint32_t, int> VertexIndex = {};                                            // [-] - Vertex slot → index
        std::map<uint32_t, int> EdgeIndex   = {};                                            // [-] - Edge slot → index
        std::map<uint32_t, int> FaceIndex   = {};                                            // [-] - Face slot → index
        double                  Scale       = 1.0;                                           // [len] - Bounding diagonal
    };

    [[nodiscard]] std::string DescribeBody(const ModelPartition& Partition, BodyToken Body, BodyDescription& Description)
    {
        Description.Inventory = Partition.CollectInventory(Body);
        Vector3 Low{ 1e300, 1e300, 1e300 }, High{ -1e300, -1e300, -1e300 };
        for (size_t Index = 0; Index < Description.Inventory.Vertices.size(); ++Index)
        {
            const Vector3 Position = Partition.Vertices.Resolve(Description.Inventory.Vertices[Index]).Position;
            Description.VertexIndex[Description.Inventory.Vertices[Index].Slot] = static_cast<int>(Index);
            Description.Blueprint.Vertices.push_back({ Position });
            Low  = { std::min(Low.X, Position.X), std::min(Low.Y, Position.Y), std::min(Low.Z, Position.Z) };
            High = { std::max(High.X, Position.X), std::max(High.Y, Position.Y), std::max(High.Z, Position.Z) };
        }
        Description.Scale = std::max(1.0e-300, Distance(Low, High));
        for (size_t Index = 0; Index < Description.Inventory.Edges.size(); ++Index)
        {
            const EdgeProfile& Profile = Partition.Edges.Resolve(Description.Inventory.Edges[Index]);
            Description.EdgeIndex[Description.Inventory.Edges[Index].Slot] = static_cast<int>(Index);
            Description.Blueprint.Edges.push_back({ Partition.Curves.Resolve(Profile.Curve), Profile.IntervalLow, Profile.IntervalHigh,
                                                    Description.VertexIndex.at(Profile.Start.Slot), Description.VertexIndex.at(Profile.End.Slot) });
        }
        for (size_t Index = 0; Index < Description.Inventory.Faces.size(); ++Index)
        {
            const FaceProfile& Profile = Partition.Faces.Resolve(Description.Inventory.Faces[Index]);
            Description.FaceIndex[Description.Inventory.Faces[Index].Slot] = static_cast<int>(Index);
            BlueprintFace Planned{ Partition.Surfaces.Resolve(Profile.Surface), Profile.Sense, {} };
            std::string   Refusal;
            for (LoopToken Loop : Profile.Loops)
            {
                std::vector<BlueprintUse> Uses;
                Partition.EnumerateLoopFins(Loop, [&](FinToken Fin)
                {
                    const FinProfile& Use = Partition.Fins.Resolve(Fin);
                    if (!Use.SurfaceCurve) { Refusal = "fin without an SP-curve"; return; }
                    Uses.push_back({ Description.EdgeIndex.at(Use.Edge.Slot), Use.Sense, *Use.SurfaceCurve });
                });
                if (!Refusal.empty()) return Refusal;
                Planned.Loops.push_back(std::move(Uses));
            }
            Description.Blueprint.Faces.push_back(std::move(Planned));
        }
        Description.Blueprint.Category = Partition.Bodies.Resolve(Body).Category;
        return {};
    }

    // 📝 SP-curves for a whole loop on a (possibly periodic) carrier: the chart image is tracked continuously along the
    //    loop (each projection referenced to the previous one), then shifted by whole periods so the loop starts in the
    //    principal window. Seams get u = 0 on one use and u = 2π on the other by construction.
    void FitLoopSurfaceCurves(const SurfaceGeometry& Surface, const BodyBlueprint& Blueprint, std::vector<BlueprintUse>& Loop)
    {
        constexpr int        Steps   = 24;
        std::vector<Vector2> Anchors;
        Vector2              Cursor  = {};
        bool                 Started = false;
        Vector2              Least{ 1e300, 1e300 };
        for (const BlueprintUse& Use : Loop)
        {
            const BlueprintEdge& Edge  = Blueprint.Edges[Use.Edge];
            const double         From  = Use.Sense == Orientation::Forward ? Edge.Low : Edge.High;
            const double         Until = Use.Sense == Orientation::Forward ? Edge.High : Edge.Low;
            Vector2              Middle{};
            for (int Step = 0; Step <= Steps; ++Step)
            {
                const Vector3 Point = EvaluateCurve(Edge.Curve, From + (Until - From) * Step / Steps).Position;
                Cursor  = ProjectOntoSurface(Surface, Point, Started ? Cursor : Vector2{});
                Started = true;
                Least   = { std::min(Least.X, Cursor.X), std::min(Least.Y, Cursor.Y) };
                if (Step == Steps / 2) Middle = Cursor;
            }
            Anchors.push_back(Middle);
        }
        const SurfacePeriodicity Period = ResolvePeriodicity(Surface);
        Vector2 Shift{};
        if (Period.PeriodU > 0.0) Shift.X = -std::floor((Least.X + 1.0e-9) / Period.PeriodU) * Period.PeriodU;
        if (Period.PeriodV > 0.0) Shift.Y = -std::floor((Least.Y + 1.0e-9) / Period.PeriodV) * Period.PeriodV;
        for (size_t Index = 0; Index < Loop.size(); ++Index)
        {
            const BlueprintEdge& Edge   = Blueprint.Edges[Loop[Index].Edge];
            const Vector2        Anchor = Anchors[Index] + Shift;
            Loop[Index].SurfaceCurve = FitSurfaceCurve(Surface, Edge.Curve, Edge.Low, Edge.High, { Anchor - Vector2{ 1.0, 1.0 }, Anchor + Vector2{ 1.0, 1.0 } }, Anchor);
        }
    }

    [[nodiscard]] double SignOfSense(Orientation Sense) { return Sense == Orientation::Forward ? 1.0 : -1.0; }

    // Assemble, audit, histories; the shared tail of every rebuild verb.
    [[nodiscard]] std::string Conclude(ModelPartition& Partition, const BodyBlueprint& Blueprint, const std::string& Label, const std::vector<FaceChangeHistory>& Planned,
                                       BodyToken& Result, std::vector<FaceChangeHistory>& Histories, AuditReport& Audit)
    {
        const AssemblyOutcome Assembly = AssembleBody(Partition, Blueprint, Label);
        if (!Assembly.Refusal.empty()) return Assembly.Refusal;
        Audit = AuditBody(Partition, Assembly.Body);
        if (!Audit.SoundCondition()) return "result fails audit " + std::string(Audit.Findings.front().Code) + " on " + Audit.Findings.front().Subject + ": " + Audit.Findings.front().Detail;
        if (std::string Clash = DetectSelfClash(Partition, Assembly.Body); !Clash.empty()) return Clash;
        Histories = Planned;
        for (size_t Index = 0; Index < Histories.size(); ++Index) Histories[Index].Result = Assembly.Faces[Index];
        Result = Assembly.Body;
        return {};
    }

    //--------------------------------------------------------------------------------------------------------------------
    //                                                SURFACE EQUIVALENCE (HEAL)
    //--------------------------------------------------------------------------------------------------------------------

    // Same point set and same outward side: planes and coaxial cylinders (the kept face's chart is re-fitted over the
    // merged boundary by FitLoopSurfaceCurves); cones, spheres and tori are 🚧.
    [[nodiscard]] bool SameOutwardCarrier(const BlueprintFace& First, const BlueprintFace& Second, double Tolerance)
    {
        if (const auto* One = std::get_if<PlaneSurface>(&First.Surface))
        {
            const auto* Two = std::get_if<PlaneSurface>(&Second.Surface);
            if (!Two) return false;
            const Vector3 OneNormal = One->Frame.ZAxis * SignOfSense(First.Sense);
            const Vector3 TwoNormal = Two->Frame.ZAxis * SignOfSense(Second.Sense);
            return Dot(OneNormal, TwoNormal) > 1.0 - 1.0e-12 && std::fabs(Dot(Two->Frame.Origin - One->Frame.Origin, OneNormal)) <= Tolerance;
        }
        if (const auto* One = std::get_if<CylinderSurface>(&First.Surface))
        {
            const auto* Two = std::get_if<CylinderSurface>(&Second.Surface);
            if (!Two || First.Sense != Second.Sense || std::fabs(One->Radius - Two->Radius) > Tolerance) return false;
            const Vector3 Gap = Two->Frame.Origin - One->Frame.Origin;
            return std::fabs(Dot(One->Frame.ZAxis, Two->Frame.ZAxis)) > 1.0 - 1.0e-12 && Length(Gap - One->Frame.ZAxis * Dot(Gap, One->Frame.ZAxis)) <= Tolerance;
        }
        return false;
    }

    [[nodiscard]] bool SameCarrierCurve(const CurveGeometry& First, const CurveGeometry& Second, double Tolerance)
    {
        if (const auto* One = std::get_if<StraightLine>(&First))
        {
            const auto* Two = std::get_if<StraightLine>(&Second);
            if (!Two || Length(Cross(One->Direction, Two->Direction)) > 1.0e-12) return false;
            const Vector3 Gap = Two->Origin - One->Origin;
            return Length(Gap - One->Direction * Dot(Gap, One->Direction)) <= Tolerance;
        }
        if (const auto* One = std::get_if<CircularArc>(&First))
        {
            const auto* Two = std::get_if<CircularArc>(&Second);
            return Two && Distance(One->Frame.Origin, Two->Frame.Origin) <= Tolerance && std::fabs(One->Radius - Two->Radius) <= Tolerance &&
                   std::fabs(Dot(One->Frame.ZAxis, Two->Frame.ZAxis)) > 1.0 - 1.0e-12;
        }
        return false;
    }
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    EXTRUDE
//------------------------------------------------------------------------------------------------------------------------

// 📝 Corpus E "extrudefaces": each selected planar face moves by Distance along its outward normal n (carrier translated,
//    same chart, so its SP-curves carry over) and a ring of side faces closes the gap: per boundary fin A → B (direction t)
//    a side face on the plane / cylinder swept by the edge along n, outward t × n, loop [edge, rise at B, top edge
//    reversed, rise at A reversed]. A closed circle reuses its single rise as the cylinder seam. The old neighbours are
//    untouched, so side faces meet them coplanar (Heal merges them).
std::string ExtrudePlanarFaces(ModelPartition& Partition, BodyToken Body, const std::vector<FaceToken>& Faces, double Distance, BodyToken& Result,
                               std::vector<FaceChangeHistory>& Histories, AuditReport& Audit, std::string& Method)
{
    if (Faces.empty()) return "no faces selected";
    if (!(Distance > 0.0)) return "extrusion distance must be positive (use push / offset to cut in)";
    BodyDescription Description;
    if (std::string Refusal = DescribeBody(Partition, Body, Description); !Refusal.empty()) return Refusal;
    BodyBlueprint&                 Blueprint = Description.Blueprint;
    std::vector<FaceChangeHistory> Planned;
    for (size_t Index = 0; Index < Blueprint.Faces.size(); ++Index) Planned.push_back({ {}, FaceChangeOrigin::Unchanged, Description.Inventory.Faces[Index], {} });
    std::set<int> Selected;
    for (FaceToken Face : Faces) Selected.insert(Description.FaceIndex.at(Face.Slot));
    std::map<int, std::set<int>> EdgeUsers;                                                  // Edge → selected faces using it
    for (int Face : Selected)
    {
        if (!std::holds_alternative<PlaneSurface>(Blueprint.Faces[Face].Surface)) return LabelToken("F", Description.Inventory.Faces[Face]) + " is not planar";
        for (const auto& Loop : Blueprint.Faces[Face].Loops) for (const BlueprintUse& Use : Loop) EdgeUsers[Use.Edge].insert(Face);
        for (const auto& Loop : Blueprint.Faces[Face].Loops)
        {
            for (const BlueprintUse& Use : Loop)
            {
                if (std::holds_alternative<ContactCurve>(Blueprint.Edges[Use.Edge].Curve)) return LabelToken("F", Description.Inventory.Faces[Face]) + " is bounded by a rolling-ball contact rail (🚧 ruled sides over procedural edges)";
                if (std::holds_alternative<IntersectionCurve>(Blueprint.Edges[Use.Edge].Curve)) return LabelToken("F", Description.Inventory.Faces[Face]) + " is bounded by an intersection curve (🚧 ruled sides over traced edges)";
            }
        }
    }
    for (const auto& [Edge, Users] : EdgeUsers) if (Users.size() > 1) return "two selected faces share an edge (🚧 joint extrusion)";

    int SideFaces = 0;
    for (int Face : Selected)
    {
        BlueprintFace& Top   = Blueprint.Faces[Face];
        const auto*    Plane = std::get_if<PlaneSurface>(&Top.Surface);
        if (!Plane) return LabelToken("F", Description.Inventory.Faces[Face]) + " is not planar";
        const Vector3 Normal = Plane->Frame.ZAxis * SignOfSense(Top.Sense);
        const Vector3 Lift   = Normal * Distance;
        PlaneSurface  Raised = *Plane;
        Raised.Frame.Origin  = Plane->Frame.Origin + Lift;
        Top.Surface          = Raised;
        Planned[Face].Origin = FaceChangeOrigin::Replaced;
        std::map<int, int> Raise, Rise;                                                      // Vertex → raised vertex, rise edge
        const auto RaisedVertex = [&](int Vertex)
        {
            if (!Raise.count(Vertex))
            {
                Raise[Vertex] = static_cast<int>(Blueprint.Vertices.size());
                Blueprint.Vertices.push_back({ Blueprint.Vertices[Vertex].Position + Lift });
                Rise[Vertex] = static_cast<int>(Blueprint.Edges.size());
                Blueprint.Edges.push_back({ StraightLine{ Blueprint.Vertices[Vertex].Position, Normal }, 0.0, Distance, Vertex, Raise[Vertex] });
            }
            return Raise[Vertex];
        };
        for (auto& Loop : Top.Loops)
        {
            for (BlueprintUse& Use : Loop)
            {
                const BlueprintEdge Lower = Blueprint.Edges[Use.Edge];
                CurveGeometry Lifted = Lower.Curve;
                std::visit([&](auto& Carrier)
                {
                    using Type = std::decay_t<decltype(Carrier)>;
                    if constexpr (std::is_same_v<Type, StraightLine>) Carrier.Origin = Carrier.Origin + Lift;
                    else if constexpr (std::is_same_v<Type, IntersectionCurve> || std::is_same_v<Type, ContactCurve>) {}   // Refused above
                    else if constexpr (std::is_same_v<Type, NurbsCurve>) Carrier = MapNurbsCurve(*Carrier.Definition, [&](Vector3 Point) { return Point + Lift; });
                    else Carrier.Frame.Origin = Carrier.Frame.Origin + Lift;
                }, Lifted);
                const int UpperStart = RaisedVertex(Lower.Start);                             // Issues rise edges first
                const int UpperEnd   = RaisedVertex(Lower.End);
                const int Upper      = static_cast<int>(Blueprint.Edges.size());
                Blueprint.Edges.push_back({ Lifted, Lower.Low, Lower.High, UpperStart, UpperEnd });

                // Side face: plane through a line, or the cylinder over a circle whose axis is the normal.
                const int     From      = Use.Sense == Orientation::Forward ? Lower.Start : Lower.End;
                const int     Until     = Use.Sense == Orientation::Forward ? Lower.End : Lower.Start;
                const double  Middle    = 0.5 * (Lower.Low + Lower.High);
                const Vector3 Tangent   = Normalize(EvaluateCurve(Lower.Curve, Middle).Derivative) * SignOfSense(Use.Sense);
                const Vector3 Outward   = Cross(Tangent, Normal);
                BlueprintFace Side;
                if (const auto* Line = std::get_if<StraightLine>(&Lower.Curve))
                {
                    Side.Surface = PlaneSurface{ ConstructFrame(Line->Origin, Outward, Tangent) };
                    Side.Sense   = Orientation::Forward;
                }
                else if (const auto* Circle = std::get_if<CircularArc>(&Lower.Curve))
                {
                    OrthonormalFrame Axis = ConstructFrame(Circle->Frame.Origin, Normal, Circle->Frame.XAxis);
                    Side.Surface = CylinderSurface{ Axis, Circle->Radius };
                    const Vector3 Point  = EvaluateCurve(Lower.Curve, Middle).Position;
                    const Vector3 Radial = Point - Circle->Frame.Origin;
                    Side.Sense   = Dot(Radial, Outward) > 0.0 ? Orientation::Forward : Orientation::Reversed;
                }
                else
                {
                    return "boundary edge of " + LabelToken("F", Description.Inventory.Faces[Face]) + " is an ellipse (side carrier 🚧)";
                }
                const Orientation Back = Use.Sense == Orientation::Forward ? Orientation::Reversed : Orientation::Forward;
                Side.Loops.push_back({ { Use.Edge, Use.Sense, ParameterLine{} }, { Rise[Until], Orientation::Forward, ParameterLine{} },
                                       { Upper, Back, ParameterLine{} }, { Rise[From], Orientation::Reversed, ParameterLine{} } });
                FitLoopSurfaceCurves(Side.Surface, Blueprint, Side.Loops.back());
                Blueprint.Faces.push_back(std::move(Side));
                Planned.push_back({ {}, FaceChangeOrigin::Generated, Description.Inventory.Faces[Face], Description.Inventory.Edges[Use.Edge] });
                ++SideFaces;
                Use.Edge = Upper;                                                            // Same τ, translated chart: SP-curve unchanged
            }
        }
    }
    Method = "direct rebuild: " + std::to_string(Selected.size()) + " face(s) raised, " + std::to_string(SideFaces) + " side face(s)";
    return Conclude(Partition, Blueprint, "ExtrudeFaces", Planned, Result, Histories, Audit);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    DELETE
//------------------------------------------------------------------------------------------------------------------------

// 📝 Corpus E "deleteface": ① hole heal — when every loop of a neighbour that touches the deleted faces is a hole loop made
//    only of their edges (a bore wall), those loops are dropped and the body stays solid; ② otherwise the faces are
//    removed and the remainder is an open sheet body (laminar edges where the faces were). Neighbour growth to close a
//    gap (Parasolid "heal by growing") is 🚧.
std::string DeleteBodyFaces(ModelPartition& Partition, BodyToken Body, const std::vector<FaceToken>& Faces, BodyToken& Result, BodyCategory& Category,
                            std::vector<FaceChangeHistory>& Histories, AuditReport& Audit, std::string& Method)
{
    if (Faces.empty()) return "no faces selected";
    BodyDescription Description;
    if (std::string Refusal = DescribeBody(Partition, Body, Description); !Refusal.empty()) return Refusal;
    const BodyBlueprint& Source = Description.Blueprint;
    std::set<int> Deleted, DeletedEdges;
    for (FaceToken Face : Faces) Deleted.insert(Description.FaceIndex.at(Face.Slot));
    if (Deleted.size() == Source.Faces.size()) return "every face would be deleted";
    for (int Face : Deleted) for (const auto& Loop : Source.Faces[Face].Loops) for (const BlueprintUse& Use : Loop) DeletedEdges.insert(Use.Edge);

    bool Healable = true;
    for (size_t Face = 0; Face < Source.Faces.size() && Healable; ++Face)
    {
        if (Deleted.count(static_cast<int>(Face))) continue;
        for (size_t Loop = 0; Loop < Source.Faces[Face].Loops.size(); ++Loop)
        {
            const auto& Uses     = Source.Faces[Face].Loops[Loop];
            const auto  Touching = std::count_if(Uses.begin(), Uses.end(), [&](const BlueprintUse& Use) { return DeletedEdges.count(Use.Edge) > 0; });
            if (Touching > 0 && (Loop == 0 || Touching != static_cast<long>(Uses.size()))) Healable = false;
        }
    }

    BodyBlueprint                  Blueprint;
    std::vector<FaceChangeHistory> Planned;
    Blueprint.Vertices = Source.Vertices;
    Blueprint.Edges    = Source.Edges;
    Blueprint.Category = Healable ? BodyCategory::Solid : BodyCategory::Sheet;
    int DroppedLoops = 0;
    for (size_t Face = 0; Face < Source.Faces.size(); ++Face)
    {
        if (Deleted.count(static_cast<int>(Face))) continue;
        BlueprintFace Kept = Source.Faces[Face];
        bool Trimmed = false;
        if (Healable)
        {
            const auto Before = Kept.Loops.size();
            std::erase_if(Kept.Loops, [&](const std::vector<BlueprintUse>& Uses) { return !Uses.empty() && DeletedEdges.count(Uses.front().Edge) > 0; });
            Trimmed       = Kept.Loops.size() != Before;
            DroppedLoops += static_cast<int>(Before - Kept.Loops.size());
        }
        Blueprint.Faces.push_back(std::move(Kept));
        Planned.push_back({ {}, Trimmed ? FaceChangeOrigin::Trimmed : FaceChangeOrigin::Unchanged, Description.Inventory.Faces[Face], {} });
    }
    Category = Blueprint.Category;
    Method   = Healable ? "hole heal: " + std::to_string(Deleted.size()) + " face(s) and " + std::to_string(DroppedLoops) + " hole loop(s) removed, body stays solid"
                        : "open sheet: " + std::to_string(Deleted.size()) + " face(s) removed, their edges become laminar";
    return Conclude(Partition, Blueprint, "DeleteFace", Planned, Result, Histories, Audit);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    HEAL
//------------------------------------------------------------------------------------------------------------------------

// 📝 Corpus E "heal": ① faces on one carrier (plane, coaxial cylinder) with the same outward side, adjacent through an
//    edge, merge (union-find); their shared edges vanish and the loops are spliced (next = successor; across a vanished
//    edge, continue from its mate's successor), outer loop = largest |area| in the chart;
//    an edge used twice by one planar face (a slit) vanishes the same way; ② at a vertex left with exactly two edges on
//    one carrier (collinear lines, one circle) the edges join. A body with nothing to merge is returned as an equal copy.
std::string HealBody(ModelPartition& Partition, BodyToken Body, BodyToken& Result, std::vector<FaceChangeHistory>& Histories, AuditReport& Audit, std::string& Method)
{
    BodyDescription Description;
    if (std::string Refusal = DescribeBody(Partition, Body, Description); !Refusal.empty()) return Refusal;
    BodyBlueprint& Source    = Description.Blueprint;
    const double   Tolerance = 1.0e-9 * Description.Scale;
    const int      FaceCount = static_cast<int>(Source.Faces.size());

    // ① Union-find over faces through edges whose two uses lie on one outward plane.
    std::vector<int> Root(FaceCount);
    for (int Face = 0; Face < FaceCount; ++Face) Root[Face] = Face;
    const std::function<int(int)> Find = [&](int Face) { return Root[Face] == Face ? Face : Root[Face] = Find(Root[Face]); };
    std::map<int, std::vector<int>> EdgeFaces;
    for (int Face = 0; Face < FaceCount; ++Face) for (const auto& Loop : Source.Faces[Face].Loops) for (const BlueprintUse& Use : Loop) EdgeFaces[Use.Edge].push_back(Face);
    std::set<int> Internal;
    for (const auto& [Edge, Users] : EdgeFaces)
    {
        if (Users.size() == 2 && Users[0] != Users[1] && SameOutwardCarrier(Source.Faces[Users[0]], Source.Faces[Users[1]], Tolerance))
        {
            Root[Find(Users[0])] = Find(Users[1]);
            Internal.insert(Edge);
        }
    }
    for (auto Edge = Internal.begin(); Edge != Internal.end();)                              // Only edges inside one merged group vanish
    {
        const auto& Users = EdgeFaces[*Edge];
        Edge = Find(Users[0]) == Find(Users[1]) ? std::next(Edge) : Internal.erase(Edge);
    }
    std::set<int> Slit;                                                                      // Faces with an edge used twice by themselves
    for (const auto& [Edge, Users] : EdgeFaces)
    {
        if (Users.size() == 2 && Users[0] == Users[1] && std::holds_alternative<PlaneSurface>(Source.Faces[Users[0]].Surface))
        {
            Internal.insert(Edge);                                                           // A slit on a plane is not a seam
            Slit.insert(Users[0]);
        }
    }

    // ② Merged faces: surviving uses re-traced into loops by vertex chaining.
    BodyBlueprint                  Blueprint;
    std::vector<FaceChangeHistory> Planned;
    Blueprint.Vertices = Source.Vertices;
    Blueprint.Edges    = Source.Edges;
    Blueprint.Category = Source.Category;
    int Merged = 0, Slits = 0;
    for (int Face = 0; Face < FaceCount; ++Face)
    {
        if (Find(Face) != Face) continue;
        std::vector<int> Members;
        for (int Other = 0; Other < FaceCount; ++Other) if (Find(Other) == Face) Members.push_back(Other);
        if (Members.size() == 1 && !Slit.count(Face))
        {
            Blueprint.Faces.push_back(Source.Faces[Face]);
            Planned.push_back({ {}, FaceChangeOrigin::Unchanged, Description.Inventory.Faces[Face], {} });
            continue;
        }
        Merged += static_cast<int>(Members.size()) - 1;
        Slits  += Slit.count(Face) ? 1 : 0;
        // Fin splicing over (member, loop, use) positions.
        struct UsePlace { int Member; size_t Loop; size_t Use; };
        std::vector<UsePlace>              Places;
        std::map<int, std::vector<size_t>> EdgePlaces;                                      // Edge → places using it
        std::map<std::tuple<int, size_t, size_t>, size_t> PlaceIndex;
        for (int Member : Members)
        {
            for (size_t Loop = 0; Loop < Source.Faces[Member].Loops.size(); ++Loop)
            {
                for (size_t Use = 0; Use < Source.Faces[Member].Loops[Loop].size(); ++Use)
                {
                    PlaceIndex[{ Member, Loop, Use }] = Places.size();
                    EdgePlaces[Source.Faces[Member].Loops[Loop][Use].Edge].push_back(Places.size());
                    Places.push_back({ Member, Loop, Use });
                }
            }
        }
        const auto UseAt     = [&](size_t Place) -> const BlueprintUse& { return Source.Faces[Places[Place].Member].Loops[Places[Place].Loop][Places[Place].Use]; };
        const auto Successor = [&](size_t Place)
        {
            const UsePlace& Here = Places[Place];
            return PlaceIndex.at({ Here.Member, Here.Loop, (Here.Use + 1) % Source.Faces[Here.Member].Loops[Here.Loop].size() });
        };
        const auto Mate = [&](size_t Place) { const auto& Both = EdgePlaces.at(UseAt(Place).Edge); return Both[0] == Place ? Both[1] : Both[0]; };
        BlueprintFace Combined{ Source.Faces[Face].Surface, Source.Faces[Face].Sense, {} };
        std::vector<bool> Taken(Places.size(), false);
        for (size_t Seed = 0; Seed < Places.size(); ++Seed)
        {
            if (Taken[Seed] || Internal.count(UseAt(Seed).Edge)) continue;
            std::vector<BlueprintUse> Loop;
            size_t Cursor = Seed;
            for (size_t Guard = 0; Guard <= Places.size(); ++Guard)
            {
                Taken[Cursor] = true;
                Loop.push_back(UseAt(Cursor));
                Cursor = Successor(Cursor);
                for (size_t Hop = 0; Internal.count(UseAt(Cursor).Edge) && Hop <= Places.size(); ++Hop) Cursor = Successor(Mate(Cursor));
                if (Cursor == Seed) break;
                if (Taken[Cursor]) return "merged face boundary does not close";
            }
            if (Cursor != Seed) return "merged face boundary does not close";
            FitLoopSurfaceCurves(Combined.Surface, Blueprint, Loop);
            Combined.Loops.push_back(std::move(Loop));
        }
        // Outer loop first: largest |signed area| of the chart polygon.
        const auto Area = [&](const std::vector<BlueprintUse>& Loop)
        {
            double Sum = 0.0;
            for (const BlueprintUse& Use : Loop)
            {
                const BlueprintEdge& Edge = Blueprint.Edges[Use.Edge];
                for (int Step = 0; Step < 16; ++Step)
                {
                    const double  From = Use.Sense == Orientation::Forward ? Edge.Low : Edge.High;
                    const double  Span = (Use.Sense == Orientation::Forward ? 1.0 : -1.0) * (Edge.High - Edge.Low);
                    const Vector2 One  = EvaluateParameterCurve(Use.SurfaceCurve, From + Span * Step / 16.0).Position;
                    const Vector2 Two  = EvaluateParameterCurve(Use.SurfaceCurve, From + Span * (Step + 1) / 16.0).Position;
                    Sum += One.X * Two.Y - Two.X * One.Y;
                }
            }
            return std::fabs(Sum);
        };
        std::stable_sort(Combined.Loops.begin(), Combined.Loops.end(), [&](const auto& One, const auto& Two) { return Area(One) > Area(Two); });
        Blueprint.Faces.push_back(std::move(Combined));
        Planned.push_back({ {}, FaceChangeOrigin::Trimmed, Description.Inventory.Faces[Face], {} });
    }

    // ③ Edge joins at vertices of degree two whose edges share a carrier.
    int Joined = 0;
    for (bool Progress = true; Progress;)
    {
        Progress = false;
        std::map<int, std::vector<int>> VertexEdges;
        std::set<int> Used;
        for (const BlueprintFace& Target : Blueprint.Faces) for (const auto& Loop : Target.Loops) for (const BlueprintUse& Use : Loop) Used.insert(Use.Edge);
        for (int Edge : Used)
        {
            VertexEdges[Blueprint.Edges[Edge].Start].push_back(Edge);
            if (Blueprint.Edges[Edge].End != Blueprint.Edges[Edge].Start) VertexEdges[Blueprint.Edges[Edge].End].push_back(Edge);
        }
        for (const auto& [Vertex, Edges] : VertexEdges)
        {
            if (Edges.size() != 2 || Edges[0] == Edges[1]) continue;
            const BlueprintEdge& One = Blueprint.Edges[Edges[0]];
            const BlueprintEdge& Two = Blueprint.Edges[Edges[1]];
            if (!SameCarrierCurve(One.Curve, Two.Curve, Tolerance)) continue;
            // Orient the join First → Vertex → Last along the first edge's direction.
            const bool OneEndsHere = One.End == Vertex;
            const int  First       = OneEndsHere ? One.Start : One.End;
            const int  Last        = Two.Start == Vertex ? Two.End : Two.Start;
            if (First == Vertex || Last == Vertex) continue;
            BlueprintEdge Join;
            if (std::holds_alternative<StraightLine>(One.Curve))
            {
                const Vector3 Begin = Blueprint.Vertices[First].Position, Finish = Blueprint.Vertices[Last].Position;
                Join = { StraightLine{ Begin, Normalize(Finish - Begin) }, 0.0, Distance(Begin, Finish), First, Last };
            }
            else
            {
                const Vector3 Begin = Blueprint.Vertices[First].Position, Finish = Blueprint.Vertices[Last].Position, Through = Blueprint.Vertices[Vertex].Position;
                const double  Low   = ProjectOntoCurve(One.Curve, Begin, 0.0);
                double        Pivot = ProjectOntoCurve(One.Curve, Through, Low);
                double        High  = First == Last ? Low + τPeriod : ProjectOntoCurve(One.Curve, Finish, Low);
                if (Pivot <= Low) Pivot += τPeriod;
                if (High <= Low) High += τPeriod;
                if (!(Pivot < High)) continue;                                               // Circle runs the other way: leave it
                Join = { One.Curve, Low, High, First, Last };
            }
            const int Fresh = static_cast<int>(Blueprint.Edges.size());
            Blueprint.Edges.push_back(Join);
            // Each loop that traverses Edges[0] and Edges[1] consecutively (either order) gets one use of the join.
            for (BlueprintFace& Target : Blueprint.Faces)
            {
                for (auto& Loop : Target.Loops)
                {
                    for (size_t Index = 0; Index < Loop.size() && Loop.size() >= 2; ++Index)  // A seam pair occurs twice
                    {
                        const size_t Next = (Index + 1) % Loop.size();
                        const bool   Pair = (Loop[Index].Edge == Edges[0] && Loop[Next].Edge == Edges[1]) || (Loop[Index].Edge == Edges[1] && Loop[Next].Edge == Edges[0]);
                        if (!Pair) continue;
                        const BlueprintEdge& Here  = Blueprint.Edges[Loop[Index].Edge];
                        const int            Begin = Loop[Index].Sense == Orientation::Forward ? Here.Start : Here.End;
                        Loop[Index] = { Fresh, Begin == First ? Orientation::Forward : Orientation::Reversed, ParameterLine{} };
                        Loop.erase(Loop.begin() + static_cast<long>(Next));
                        if (Next < Index) --Index;
                    }
                }
            }
            ++Joined;
            Progress = true;
            break;
        }
    }
    if (Joined > 0) for (BlueprintFace& Target : Blueprint.Faces) for (auto& Loop : Target.Loops) FitLoopSurfaceCurves(Target.Surface, Blueprint, Loop);
    Method = Merged + Joined + Slits == 0 ? "nothing to merge: equal copy"
                                          : "merged " + std::to_string(Merged) + " face(s), removed " + std::to_string(Slits) + " slit(s), joined " + std::to_string(Joined) + " edge pair(s)";
    return Conclude(Partition, Blueprint, "Heal", Planned, Result, Histories, Audit);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    THICKEN
//------------------------------------------------------------------------------------------------------------------------

// 📝 Sheet → solid of thickness Distance on the side opposite the face normals (Parasolid thicken): the inner skin is the
//    face change "every face offset by −Distance" of the sheet (laminar edges re-solved on their rims), stitched to the
//    outer skin by one rim face per laminar edge — loop [edge reversed, rise at X, inner edge, rise at Y reversed] for a
//    fin X → Y, outward t × n. Thicken(DeleteFace(F), t) is the closed-form twin of Hollow(t, F).
std::string ThickenSheet(ModelPartition& Partition, BodyToken Body, double Thickness, BodyToken& Result, std::vector<FaceChangeHistory>& Histories, AuditReport& Audit, std::string& Method)
{
    if (Partition.Bodies.Resolve(Body).Category != BodyCategory::Sheet) return "body is not a sheet (delete a face first)";
    if (!(Thickness > 0.0)) return "thickness must be positive";
    BodyDescription Description;
    if (std::string Refusal = DescribeBody(Partition, Body, Description); !Refusal.empty()) return Refusal;
    const BodyBlueprint& Outer = Description.Blueprint;
    std::vector<FaceReplacement> Replacements;
    for (FaceToken Face : Description.Inventory.Faces)
    {
        const FaceProfile& Profile = Partition.Faces.Resolve(Face);
        const std::optional<SurfaceGeometry> Offset = OffsetSurface(Partition.Surfaces.Resolve(Profile.Surface), -Thickness * SignOfSense(Profile.Sense));
        if (!Offset) return LabelToken("F", Face) + ": thickness exceeds the curvature radius";
        Replacements.push_back({ Face, *Offset, -Thickness });
    }
    BodyBlueprint                  Inner;
    std::vector<FaceChangeHistory> Provenance;
    FaceChangeStatistics           Statistics;
    if (std::string Refusal = PlanFaceChange(Partition, Body, Replacements, Inner, Provenance, Statistics); !Refusal.empty()) return "inner skin: " + Refusal;

    const int     VertexShift = static_cast<int>(Outer.Vertices.size());
    const int     EdgeShift   = static_cast<int>(Outer.Edges.size());
    BodyBlueprint Blueprint   = Outer;
    Blueprint.Category        = BodyCategory::Solid;
    std::vector<FaceChangeHistory> Planned;
    for (FaceToken Face : Description.Inventory.Faces) Planned.push_back({ {}, FaceChangeOrigin::Unchanged, Face, {} });
    for (const BlueprintVertex& Vertex : Inner.Vertices) Blueprint.Vertices.push_back(Vertex);
    for (BlueprintEdge Edge : Inner.Edges)
    {
        Edge.Start += VertexShift;
        Edge.End   += VertexShift;
        Blueprint.Edges.push_back(Edge);
    }
    for (size_t Face = 0; Face < Inner.Faces.size(); ++Face)
    {
        BlueprintFace Flipped{ Inner.Faces[Face].Surface, Inner.Faces[Face].Sense == Orientation::Forward ? Orientation::Reversed : Orientation::Forward, {} };
        for (const auto& Loop : Inner.Faces[Face].Loops)
        {
            std::vector<BlueprintUse> Reversed;
            for (auto Use = Loop.rbegin(); Use != Loop.rend(); ++Use)
            {
                Reversed.push_back({ Use->Edge + EdgeShift, Use->Sense == Orientation::Forward ? Orientation::Reversed : Orientation::Forward, Use->SurfaceCurve });
            }
            Flipped.Loops.push_back(std::move(Reversed));
        }
        Blueprint.Faces.push_back(std::move(Flipped));
        Planned.push_back({ {}, FaceChangeOrigin::Generated, Description.Inventory.Faces[Face], {} });
    }

    // Rim faces over the laminar edges.
    std::map<int, std::vector<std::pair<int, BlueprintUse>>> Uses;                            // Edge → (face, use)
    for (size_t Face = 0; Face < Outer.Faces.size(); ++Face) for (const auto& Loop : Outer.Faces[Face].Loops) for (const BlueprintUse& Use : Loop) Uses[Use.Edge].push_back({ static_cast<int>(Face), Use });
    std::map<int, int> Rise;                                                                 // Vertex → rise edge
    const auto RiseEdge = [&](int Vertex) -> int
    {
        if (Rise.count(Vertex)) return Rise[Vertex];
        const Vector3 From = Blueprint.Vertices[Vertex].Position, Until = Blueprint.Vertices[Vertex + VertexShift].Position;
        if (Distance(From, Until) <= 1.0e-12 * Description.Scale) return -1;
        Blueprint.Edges.push_back({ StraightLine{ From, Normalize(Until - From) }, 0.0, Distance(From, Until), Vertex, Vertex + VertexShift });
        return Rise[Vertex] = static_cast<int>(Blueprint.Edges.size()) - 1;
    };
    int RimFaces = 0;
    for (const auto& [Edge, Users] : Uses)
    {
        if (Users.size() != 1) continue;
        const auto& [Face, Use] = Users.front();
        const BlueprintEdge& SkinEdge = Outer.Edges[Edge];
        const std::optional<SurfaceGeometry> Rim = ConstructRimSurface(Partition, Description.Inventory.Edges[Edge], Description.Inventory.Faces[Face]);
        if (!Rim) return "laminar edge " + LabelToken("E", Description.Inventory.Edges[Edge]) + " has no rim surface (🚧)";
        const int From  = Use.Sense == Orientation::Forward ? SkinEdge.Start : SkinEdge.End;
        const int Until = Use.Sense == Orientation::Forward ? SkinEdge.End : SkinEdge.Start;
        const int AtFrom = RiseEdge(From), AtUntil = RiseEdge(Until);
        if (AtFrom < 0 || AtUntil < 0) return "a boundary vertex does not move (degenerate rim)";
        const Orientation Back = Use.Sense == Orientation::Forward ? Orientation::Reversed : Orientation::Forward;
        BlueprintFace Side{ *Rim, Orientation::Forward, {} };
        Side.Loops.push_back({ { Edge, Back, ParameterLine{} }, { AtFrom, Orientation::Forward, ParameterLine{} },
                               { Edge + EdgeShift, Use.Sense, ParameterLine{} }, { AtUntil, Orientation::Reversed, ParameterLine{} } });
        const double  Middle  = 0.5 * (SkinEdge.Low + SkinEdge.High);
        const CurveSample Sample = EvaluateCurve(SkinEdge.Curve, Middle);
        const Vector3 Normal  = EvaluateOutwardNormal(Partition, Description.Inventory.Faces[Face], Sample.Position);
        const Vector3 Outward = Cross(Normalize(Sample.Derivative) * SignOfSense(Use.Sense), Normal);
        if (Dot(EvaluateSurfaceNormal(*Rim, ProjectOntoSurface(*Rim, Sample.Position, {})), Outward) < 0.0) Side.Sense = Orientation::Reversed;
        FitLoopSurfaceCurves(Side.Surface, Blueprint, Side.Loops.back());
        Blueprint.Faces.push_back(std::move(Side));
        Planned.push_back({ {}, FaceChangeOrigin::Generated, Description.Inventory.Faces[Face], Description.Inventory.Edges[Edge] });
        ++RimFaces;
    }
    if (RimFaces == 0) return "sheet has no boundary (closed sheet 🚧)";
    Method = "outer skin + inner skin (" + std::to_string(Inner.Faces.size()) + " offsets) + " + std::to_string(RimFaces) + " rim face(s)";
    return Conclude(Partition, Blueprint, "Thicken", Planned, Result, Histories, Audit);
}

} // namespace Frontier::GeometryProcessing
