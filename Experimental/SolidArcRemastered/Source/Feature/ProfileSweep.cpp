/*====================================================================================================================================
                                                        PROFILESWEEP.CPP
====================================================================================================================================*/
// 🧩 Solids from sketch profiles: linear extrude (planes, cylinders, holes) and full revolve (plane … torus, poles)

#include "Feature/ProfileSweep.h"

#include "Geometry/ProjectedCurve.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <optional>

namespace Frontier::SolidArcRemastered
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    INTERNAL FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

namespace
{
    constexpr double FullTurn = 2.0 * std::numbers::pi;                                      // [rad] - 2π

    [[nodiscard]] Orientation Opposite(Orientation Sense) noexcept { return Sense == Orientation::Forward ? Orientation::Reversed : Orientation::Forward; }

    struct LoopUse
    {
        int                    Edge   = -1;                                                  // [-] - Blueprint edge
        Orientation            Sense  = Orientation::Forward;                                // [-] - Along / against τ
        std::optional<Vector2> Anchor = std::nullopt;                                        // [-] - Chart point of the use midpoint (periodic charts)
    };

    struct LoopPlan
    {
        std::vector<LoopUse> Uses  = {};                                                     // [-] - Loop order (either direction)
        bool                 Outer = true;                                                   // [-] - Outer boundary vs hole
    };

    // 📝 Blueprint writer: faces get their sense from an outward probe and their loops are turned to agree with it
    //    (Σ∮𝑢 d𝑣 > 0 for outer loops of a Forward face, audit G5), so callers never reason about loop direction.
    class BlueprintWriter
    {
    public:
        int AddVertex(Vector3 Position)
        {
            Blueprint.Vertices.push_back({ Position });
            return static_cast<int>(Blueprint.Vertices.size()) - 1;
        }

        int AddEdge(CurveGeometry Curve, double Low, double High, int Start, int End)
        {
            BlueprintEdge Edge;
            Edge.Curve = std::move(Curve); Edge.Low = Low; Edge.High = High; Edge.Start = Start; Edge.End = End;
            Blueprint.Edges.push_back(std::move(Edge));
            return static_cast<int>(Blueprint.Edges.size()) - 1;
        }

        [[nodiscard]] Vector3 EdgeMidpoint(int Edge) const
        {
            const BlueprintEdge& Record = Blueprint.Edges[Edge];
            return EvaluateCurve(Record.Curve, 0.5 * (Record.Low + Record.High)).Position;
        }

        void AddFace(const SurfaceGeometry& Surface, Vector3 Probe, Vector3 Outward, std::vector<LoopPlan> Loops, SweepFace Role)
        {
            BlueprintFace Face;
            Face.Surface = Surface;
            const Vector2       Parameter = ProjectOntoSurface(Surface, Probe, { std::numbers::pi, 0.0 });
            const SurfaceSample Sample    = EvaluateSurface(Surface, Parameter);
            Face.Sense = Dot(Cross(Sample.DerivativeU, Sample.DerivativeV), Outward) > 0.0 ? Orientation::Forward : Orientation::Reversed;
            for (LoopPlan& Plan : Loops)
            {
                std::vector<BlueprintUse> Uses;
                for (const LoopUse& Use : Plan.Uses)
                {
                    const BlueprintEdge& Edge = Blueprint.Edges[Use.Edge];
                    const Vector2 Anchor = Use.Anchor.value_or(Vector2{ 0.0, 0.0 });
                    const ParameterWindow Window = { Anchor - Vector2{ 1.0, 1.0 }, Anchor + Vector2{ 1.0, 1.0 } };
                    BlueprintUse Record;
                    Record.Edge         = Use.Edge;
                    Record.Sense        = Use.Sense;
                    Record.SurfaceCurve = FitSurfaceCurve(Surface, Edge.Curve, Edge.Low, Edge.High, Window, Use.Anchor);
                    Uses.push_back(std::move(Record));
                }
                const double Expected = (Face.Sense == Orientation::Forward ? 1.0 : -1.0) * (Plan.Outer ? 1.0 : -1.0);
                if (MeasureChartArea(Uses) * Expected < 0.0)
                {
                    std::reverse(Uses.begin(), Uses.end());
                    for (BlueprintUse& Record : Uses) Record.Sense = Opposite(Record.Sense);
                }
                Face.Loops.push_back(std::move(Uses));
            }
            Blueprint.Faces.push_back(std::move(Face));
            Roles.push_back(std::move(Role));
        }

        [[nodiscard]] double MeasureChartArea(const std::vector<BlueprintUse>& Uses) const
        {
            std::vector<Vector2> Polygon;
            constexpr int Samples = 24;
            for (const BlueprintUse& Use : Uses)
            {
                const BlueprintEdge& Edge = Blueprint.Edges[Use.Edge];
                for (int Index = 0; Index < Samples; ++Index)
                {
                    const double Fraction = static_cast<double>(Index) / Samples;
                    const double Tau = Use.Sense == Orientation::Forward ? Edge.Low + Fraction * (Edge.High - Edge.Low) : Edge.High - Fraction * (Edge.High - Edge.Low);
                    Polygon.push_back(EvaluateParameterCurve(Use.SurfaceCurve, Tau).Position);
                }
            }
            double Area = 0.0;
            for (size_t Index = 0; Index < Polygon.size(); ++Index)
            {
                const Vector2 First = Polygon[Index], Second = Polygon[(Index + 1) % Polygon.size()];
                Area += 0.5 * (First.X * Second.Y - Second.X * First.Y);
            }
            return Area;
        }

        BodyBlueprint          Blueprint = {};
        std::vector<SweepFace> Roles     = {};
    };

    [[nodiscard]] SweepOutcome Assemble(ModelPartition& Partition, BlueprintWriter& Writer, const char* Label)
    {
        SweepOutcome Outcome;
        const AssemblyOutcome Assembly = AssembleBody(Partition, Writer.Blueprint, Label);
        if (!Assembly.Refusal.empty()) { Outcome.Refusal = Assembly.Refusal; return Outcome; }
        Outcome.Body  = Assembly.Body;
        Outcome.Faces = Assembly.Faces;
        Outcome.Roles = Writer.Roles;
        return Outcome;
    }

    // Right of travel (outward for an outer CCW / hole CW profile) at a segment fraction, in sketch coordinates.
    [[nodiscard]] Vector2 OutwardAt(const ProfileSegment& Segment, double Fraction)
    {
        Vector2 Tangent;
        if (Segment.Kind == ProfileSegmentKind::Line) Tangent = Segment.End - Segment.Start;
        else
        {
            const double Phi = Segment.StartAngle + Fraction * Segment.Sweep;
            Tangent = Vector2{ -std::sin(Phi), std::cos(Phi) } * (Segment.Sweep > 0.0 ? 1.0 : -1.0);
        }
        return Normalize(Vector2{ Tangent.Y, -Tangent.X });
    }
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    EXTRUDE
//------------------------------------------------------------------------------------------------------------------------

SweepOutcome ExtrudeProfile(ModelPartition& Partition, const SketchProfile& Profile, const OrthonormalFrame& Plane, double Extent)
{
    SweepOutcome Refused;
    if (!Profile.Refusal.empty()) { Refused.Refusal = "Extrude: " + Profile.Refusal; return Refused; }
    const double Height = std::fabs(Extent);                                               // [len] - Extent along the plane normal
    if (!(Height > 1.0e-9 * Profile.Scale)) { Refused.Refusal = "Extrude: distance is zero"; return Refused; }
    const double  Floor = std::min(0.0, Extent);                                           // [len] - Low cap height
    const Vector3 Axis  = Plane.ZAxis;
    auto Place = [&](Vector2 Point, double Elevation) { return Plane.Origin + Plane.XAxis * Point.X + Plane.YAxis * Point.Y + Axis * Elevation; };
    auto Lift  = [&](Vector2 Direction) { return Plane.XAxis * Direction.X + Plane.YAxis * Direction.Y; };

    BlueprintWriter Writer;
    struct LoopEdges { std::vector<int> Bottom, Top; std::vector<Orientation> Senses; };
    std::vector<LoopEdges> Rims(Profile.Loops.size());
    for (size_t LoopIndex = 0; LoopIndex < Profile.Loops.size(); ++LoopIndex)
    {
        const ProfileLoop& Loop = Profile.Loops[LoopIndex];
        const int Count = static_cast<int>(Loop.Segments.size());
        std::vector<int> Low(Count), High(Count), Rise(Count);
        for (int Index = 0; Index < Count; ++Index)
        {
            Low[Index]  = Writer.AddVertex(Place(Loop.Segments[Index].Start, Floor));
            High[Index] = Writer.AddVertex(Place(Loop.Segments[Index].Start, Floor + Height));
            Rise[Index] = Writer.AddEdge(StraightLine{ Place(Loop.Segments[Index].Start, Floor), Axis }, 0.0, Height, Low[Index], High[Index]);
        }
        LoopEdges& Rim = Rims[LoopIndex];
        for (int Index = 0; Index < Count; ++Index)
        {
            const ProfileSegment& Segment = Loop.Segments[Index];
            const int Next = (Index + 1) % Count;
            int Bottom = -1, Top = -1;
            Orientation Sense = Orientation::Forward;
            if (Segment.Kind == ProfileSegmentKind::Line)
            {
                const double  Length    = Distance(Segment.Start, Segment.End);
                const Vector3 Direction = Lift(Normalize(Segment.End - Segment.Start));
                Bottom = Writer.AddEdge(StraightLine{ Place(Segment.Start, Floor), Direction }, 0.0, Length, Low[Index], Low[Next]);
                Top    = Writer.AddEdge(StraightLine{ Place(Segment.Start, Floor + Height), Direction }, 0.0, Length, High[Index], High[Next]);
            }
            else
            {
                const bool   Counter = Segment.Sweep > 0.0;
                const double LowTau  = Counter ? Segment.StartAngle : Segment.StartAngle + Segment.Sweep;
                const double HighTau = LowTau + std::fabs(Segment.Sweep);
                Sense = Counter ? Orientation::Forward : Orientation::Reversed;
                const int First = Counter ? Index : Next, Last = Counter ? Next : Index;
                Bottom = Writer.AddEdge(CircularArc{ { Place(Segment.Centre, Floor), Plane.XAxis, Plane.YAxis, Axis }, Segment.Radius }, LowTau, HighTau, Low[First], Low[Last]);
                Top    = Writer.AddEdge(CircularArc{ { Place(Segment.Centre, Floor + Height), Plane.XAxis, Plane.YAxis, Axis }, Segment.Radius }, LowTau, HighTau, High[First], High[Last]);
            }
            Rim.Bottom.push_back(Bottom);
            Rim.Top.push_back(Top);
            Rim.Senses.push_back(Sense);

            // Side face.
            const Vector2 Middle  = EvaluateProfileSegment(Segment, 0.5);
            const Vector3 Probe   = Place(Middle, Floor + 0.5 * Height);
            const Vector3 Outward = Lift(OutwardAt(Segment, 0.5));
            LoopPlan Plan;
            const SweepFace Role{ SweepFaceRole::Side, Segment.Name, static_cast<int>(LoopIndex) };
            if (Segment.Kind == ProfileSegmentKind::Line)
            {
                const Vector3 Direction = Lift(Normalize(Segment.End - Segment.Start));
                const PlaneSurface Surface{ ConstructFrame(Place(Segment.Start, Floor), Cross(Direction, Axis), Direction) };
                Plan.Uses = { { Bottom, Sense }, { Rise[Next], Orientation::Forward }, { Top, Opposite(Sense) }, { Rise[Index], Orientation::Reversed } };
                Writer.AddFace(Surface, Probe, Outward, { Plan }, Role);
                continue;
            }
            if (Segment.Kind == ProfileSegmentKind::Circle)
            {
                const CylinderSurface Surface{ { Place(Segment.Centre, Floor), Plane.XAxis, Plane.YAxis, Axis }, Segment.Radius };
                const double Up = Sense == Orientation::Forward ? FullTurn : 0.0;            // [rad] - 𝑢 of the upward seam use
                Plan.Uses = { { Bottom, Sense, Vector2{ std::numbers::pi, 0.0 } }, { Rise[Index], Orientation::Forward, Vector2{ Up, 0.5 * Height } },
                              { Top, Opposite(Sense), Vector2{ std::numbers::pi, Height } }, { Rise[Index], Orientation::Reversed, Vector2{ FullTurn - Up, 0.5 * Height } } };
                Writer.AddFace(Surface, Probe, Outward, { Plan }, Role);
                continue;
            }
            // Arc: chart seam opposite the arc midpoint so the whole side lies inside (0, 2π).
            const Vector3 Away = Lift(Normalize(Segment.Centre - Middle));
            const CylinderSurface Surface{ ConstructFrame(Place(Segment.Centre, Floor), Axis, Away), Segment.Radius };
            auto AnchorOf = [&](int Edge) { return ProjectOntoSurface(Surface, Writer.EdgeMidpoint(Edge), { std::numbers::pi, 0.5 * Height }); };
            Plan.Uses = { { Bottom, Sense, AnchorOf(Bottom) }, { Rise[Next], Orientation::Forward, AnchorOf(Rise[Next]) },
                          { Top, Opposite(Sense), AnchorOf(Top) }, { Rise[Index], Orientation::Reversed, AnchorOf(Rise[Index]) } };
            Writer.AddFace(Surface, Probe, Outward, { Plan }, Role);
        }
    }

    // Caps: one pair per outer loop with the holes it directly encloses.
    for (size_t Outer = 0; Outer < Profile.Loops.size(); ++Outer)
    {
        if (Profile.Loops[Outer].Depth % 2 != 0) continue;
        for (int End = 0; End < 2; ++End)
        {
            const double Elevation = Floor + (End == 1 ? Height : 0.0);
            std::vector<LoopPlan> Plans;
            for (size_t LoopIndex = 0; LoopIndex < Profile.Loops.size(); ++LoopIndex)
            {
                if (LoopIndex != Outer && Profile.Loops[LoopIndex].Enclosure != static_cast<int>(Outer)) continue;
                LoopPlan Plan;
                Plan.Outer = LoopIndex == Outer;
                const LoopEdges& Rim = Rims[LoopIndex];
                for (size_t Index = 0; Index < Rim.Bottom.size(); ++Index) Plan.Uses.push_back({ End == 1 ? Rim.Top[Index] : Rim.Bottom[Index], Rim.Senses[Index] });
                Plans.push_back(Plan);
            }
            const PlaneSurface Surface{ { Place({ 0.0, 0.0 }, Elevation), Plane.XAxis, Plane.YAxis, Axis } };
            const bool OnSketch = std::fabs(Elevation) <= 1.0e-12 * Profile.Scale;           // The cap lying in the sketch plane is Start
            const SweepFace Role{ OnSketch ? SweepFaceRole::StartCap : SweepFaceRole::EndCap, {}, static_cast<int>(Outer) };
            Writer.AddFace(Surface, Place(Profile.Loops[Outer].Segments[0].Start, Elevation), Axis * (End == 1 ? 1.0 : -1.0), Plans, Role);
        }
    }
    return Assemble(Partition, Writer, "Extrude");
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    REVOLVE
//------------------------------------------------------------------------------------------------------------------------

SweepOutcome RevolveProfile(ModelPartition& Partition, const SketchProfile& Profile, const OrthonormalFrame& Plane, Vector2 AxisPoint, Vector2 AxisDirection)
{
    SweepOutcome Refused;
    if (!Profile.Refusal.empty()) { Refused.Refusal = "Revolve: " + Profile.Refusal; return Refused; }
    const double Tolerance = 1.0e-9 * Profile.Scale;                                         // [len] - On-axis distance
    const Vector2 Along = Normalize(AxisDirection);
    Vector2 Radial = { -Along.Y, Along.X };                                                  // [-] - Sketch direction away from the axis

    // ① The profile must lie on one side of the axis (touching allowed).
    double Lowest = 1e300, Highest = -1e300;
    for (const ProfileLoop& Loop : Profile.Loops)
    {
        for (const ProfileSegment& Segment : Loop.Segments)
        {
            for (int Index = 0; Index <= 32; ++Index)
            {
                const double Offset = Dot(EvaluateProfileSegment(Segment, Index / 32.0) - AxisPoint, Radial);
                Lowest = std::min(Lowest, Offset); Highest = std::max(Highest, Offset);
            }
        }
    }
    if (Lowest < -Tolerance && Highest > Tolerance) { Refused.Refusal = "Revolve: profile crosses the axis"; return Refused; }
    if (Highest <= Tolerance) Radial = Radial * -1.0;
    const double Handedness = Along.X * Radial.Y - Along.Y * Radial.X > 0.0 ? -1.0 : 1.0;    // [-] - (r, z) vs sketch (x, y) orientation

    // ② Revolution frame: Z along the axis, X toward the profile (𝑢 = 0 holds the profile itself).
    auto Lift = [&](Vector2 Direction) { return Plane.XAxis * Direction.X + Plane.YAxis * Direction.Y; };
    const Vector3 Origin = Plane.Origin + Lift(AxisPoint);
    const Vector3 AxisZ  = Lift(Along);
    const Vector3 AxisX  = Lift(Radial);
    const Vector3 AxisY  = Cross(AxisZ, AxisX);
    auto Meridian = [&](Vector2 Point) { return Vector2{ Dot(Point - AxisPoint, Radial), Dot(Point - AxisPoint, Along) }; };   // (r, z)
    auto Place    = [&](Vector2 Point) { const Vector2 RZ = Meridian(Point); return Origin + AxisX * RZ.X + AxisZ * RZ.Y; };
    auto Circle   = [&](double Radius, double Elevation) { return CircularArc{ { Origin + AxisZ * Elevation, AxisX, AxisY, AxisZ }, Radius }; };

    BlueprintWriter Writer;
    for (size_t LoopIndex = 0; LoopIndex < Profile.Loops.size(); ++LoopIndex)
    {
        const ProfileLoop& Loop = Profile.Loops[LoopIndex];
        const int Count = static_cast<int>(Loop.Segments.size());

        // ③ Full circle off the axis → closed torus (equator + meridian circles through one vertex, chart [0, 2π]²).
        if (Count == 1 && Loop.Segments[0].Kind == ProfileSegmentKind::Circle)
        {
            const ProfileSegment& Segment = Loop.Segments[0];
            const Vector2 Centre = Meridian(Segment.Centre);
            if (Centre.X - Segment.Radius <= Tolerance) { Refused.Refusal = "Revolve: circle " + Segment.Name + " touches or crosses the axis"; return Refused; }
            const TorusSurface Surface{ { Origin + AxisZ * Centre.Y, AxisX, AxisY, AxisZ }, Centre.X, Segment.Radius };
            const int Vertex = Writer.AddVertex(Origin + AxisZ * Centre.Y + AxisX * (Centre.X + Segment.Radius));
            const int Equator = Writer.AddEdge(Circle(Centre.X + Segment.Radius, Centre.Y), 0.0, FullTurn, Vertex, Vertex);
            const int Ring = Writer.AddEdge(CircularArc{ { Origin + AxisZ * Centre.Y + AxisX * Centre.X, AxisX, AxisZ, Cross(AxisX, AxisZ) }, Segment.Radius }, 0.0, FullTurn, Vertex, Vertex);
            LoopPlan Plan;
            const double Pi = std::numbers::pi;
            Plan.Uses = { { Equator, Orientation::Forward, Vector2{ Pi, 0.0 } }, { Ring, Orientation::Forward, Vector2{ FullTurn, Pi } },
                          { Equator, Orientation::Reversed, Vector2{ Pi, FullTurn } }, { Ring, Orientation::Reversed, Vector2{ 0.0, Pi } } };
            const Vector2 Outward = OutwardAt(Segment, 0.0);
            Writer.AddFace(Surface, Place(Segment.Start), Lift(Outward), { Plan }, { SweepFaceRole::Side, Segment.Name, static_cast<int>(LoopIndex) });
            continue;
        }

        // ④ Vertices (every profile vertex; on-axis ones are poles / apexes) and their circles.
        std::vector<int> Vertex(Count), Ring(Count, -1);
        std::vector<Vector2> RZ(Count);
        for (int Index = 0; Index < Count; ++Index)
        {
            RZ[Index] = Meridian(Loop.Segments[Index].Start);
            const bool OnAxis = RZ[Index].X <= Tolerance;
            Vertex[Index] = Writer.AddVertex(OnAxis ? Origin + AxisZ * RZ[Index].Y : Place(Loop.Segments[Index].Start));
            if (!OnAxis) Ring[Index] = Writer.AddEdge(Circle(RZ[Index].X, RZ[Index].Y), 0.0, FullTurn, Vertex[Index], Vertex[Index]);
        }

        // ⑤ One face per segment not lying on the axis.
        for (int Index = 0; Index < Count; ++Index)
        {
            const ProfileSegment& Segment = Loop.Segments[Index];
            const int     Next  = (Index + 1) % Count;
            const Vector2 Start = RZ[Index], End = RZ[Next];
            const SweepFace Role{ SweepFaceRole::Side, Segment.Name, static_cast<int>(LoopIndex) };
            const Vector2 Middle  = EvaluateProfileSegment(Segment, 0.5);
            const Vector3 Probe   = Place(Middle);
            const Vector3 Outward = Lift(OutwardAt(Segment, 0.5));
            if (Segment.Kind == ProfileSegmentKind::Line && Start.X <= Tolerance && End.X <= Tolerance) continue;   // On the axis: sweeps nothing

            // Plane ⟂ axis: disc or annulus bounded by the two circles.
            if (Segment.Kind == ProfileSegmentKind::Line && std::fabs(Start.Y - End.Y) <= Tolerance)
            {
                const PlaneSurface Surface{ { Origin + AxisZ * Start.Y, AxisX, AxisY, AxisZ } };
                std::vector<LoopPlan> Plans;
                const int Larger = Start.X > End.X ? Index : Next, Smaller = Start.X > End.X ? Next : Index;
                Plans.push_back({ { { Ring[Larger], Orientation::Forward } }, true });
                if (Ring[Smaller] >= 0) Plans.push_back({ { { Ring[Smaller], Orientation::Forward } }, false });
                Writer.AddFace(Surface, Probe, Outward, Plans, Role);
                continue;
            }

            // Curved face: seam = the profile segment itself at 𝑢 = 0.
            SurfaceGeometry Surface;
            CurveGeometry   SeamCurve;
            double SeamLow = 0.0, SeamHigh = 0.0, StartV = 0.0, EndV = 0.0;
            Orientation SeamSense = Orientation::Forward;                                    // Seam use running Start → End
            if (Segment.Kind == ProfileSegmentKind::Line)
            {
                const double Length = Distance(Segment.Start, Segment.End);
                SeamCurve = StraightLine{ Place(Segment.Start), Lift(Normalize(Segment.End - Segment.Start)) };
                SeamLow = 0.0; SeamHigh = Length;
                if (std::fabs(Start.X - End.X) <= Tolerance)
                {
                    Surface = CylinderSurface{ { Origin, AxisX, AxisY, AxisZ }, Start.X };
                    StartV = Start.Y; EndV = End.Y;
                }
                else
                {
                    // Chart origin at the wide end, so an apex (r = 0) is never the frame origin.
                    const double Wide = Start.X >= End.X ? Start.Y : End.Y;                  // [len] - Axial height of the wide end
                    Surface = ConeSurface{ { Origin + AxisZ * Wide, AxisX, AxisY, AxisZ }, std::max(Start.X, End.X), (End.X - Start.X) / (End.Y - Start.Y) };
                    StartV = Start.Y - Wide; EndV = End.Y - Wide;
                }
            }
            else
            {
                const Vector2 Centre = Meridian(Segment.Centre);
                const double  First  = std::atan2(Start.Y - Centre.Y, Start.X - Centre.X);   // [rad] - (r, z) angle at Start
                const double  Sweep  = Segment.Sweep * Handedness;                            // [rad] - Signed (r, z) sweep
                if (Centre.X <= Tolerance)
                {
                    Surface = SphereSurface{ { Origin + AxisZ * Centre.Y, AxisX, AxisY, AxisZ }, Segment.Radius };
                }
                else
                {
                    if (Centre.X - Segment.Radius < -Tolerance) { Refused.Refusal = "Revolve: arc " + Segment.Name + " would sweep a self-intersecting torus"; return Refused; }
                    Surface = TorusSurface{ { Origin + AxisZ * Centre.Y, AxisX, AxisY, AxisZ }, Centre.X, Segment.Radius };
                }
                SeamCurve = CircularArc{ { Origin + AxisX * Centre.X + AxisZ * Centre.Y, AxisX, AxisZ, Cross(AxisX, AxisZ) }, Segment.Radius };
                SeamLow   = Sweep > 0.0 ? First : First + Sweep;
                SeamHigh  = SeamLow + std::fabs(Sweep);
                SeamSense = Sweep > 0.0 ? Orientation::Forward : Orientation::Reversed;
                StartV = First; EndV = First + Sweep;
                if (Centre.X <= Tolerance && (std::max(StartV, EndV) > 0.5 * std::numbers::pi + 1.0e-9 || std::min(StartV, EndV) < -0.5 * std::numbers::pi - 1.0e-9))
                {
                    Refused.Refusal = "Revolve: arc " + Segment.Name + " crosses the axis"; return Refused;
                }
            }
            const int SeamStart = SeamSense == Orientation::Forward ? Vertex[Index] : Vertex[Next];
            const int SeamEnd   = SeamSense == Orientation::Forward ? Vertex[Next] : Vertex[Index];
            const int Seam      = Writer.AddEdge(SeamCurve, SeamLow, SeamHigh, SeamStart, SeamEnd);
            const double MiddleV = 0.5 * (StartV + EndV);
            LoopPlan Plan;
            if (Ring[Index] >= 0) Plan.Uses.push_back({ Ring[Index], Orientation::Forward, Vector2{ std::numbers::pi, StartV } });
            Plan.Uses.push_back({ Seam, SeamSense, Vector2{ FullTurn, MiddleV } });
            if (Ring[Next] >= 0) Plan.Uses.push_back({ Ring[Next], Orientation::Reversed, Vector2{ std::numbers::pi, EndV } });
            Plan.Uses.push_back({ Seam, Opposite(SeamSense), Vector2{ 0.0, MiddleV } });
            Writer.AddFace(Surface, Probe, Outward, { Plan }, Role);
        }
    }
    return Assemble(Partition, Writer, "Revolve");
}

std::string SweepRoleName(const SweepFace& Role)
{
    switch (Role.Role)
    {
    case SweepFaceRole::Side:     return "Side(" + Role.Curve + ")";
    case SweepFaceRole::StartCap: return "Cap(Start)";
    case SweepFaceRole::EndCap:   return "Cap(End)";
    }
    return "?";
}

} // namespace Frontier::SolidArcRemastered
