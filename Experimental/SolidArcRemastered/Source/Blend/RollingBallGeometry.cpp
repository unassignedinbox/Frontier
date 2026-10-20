/*====================================================================================================================================
                                                   ROLLINGBALLGEOMETRY.CPP
====================================================================================================================================*/
// 🧩 Rolling-ball geometry of one edge: spine = offset ∩ offset, contact curves, exact blend carrier, conic sections

#include "Blend/RollingBallGeometry.h"

#include "Audit/ModelAudit.h"

#include "Containment/FaceDomain.h"
#include "Intersection/SurfaceIntersection.h"
#include "Geometry/RollingBallCarrier.h"
#include "Intersection/IntersectionCurve.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <numbers>
#include <set>

namespace Frontier::SolidArcRemastered
{

namespace
{
    //--------------------------------------------------------------------------------------------------------------------
    //                                                CONSTANTS
    //--------------------------------------------------------------------------------------------------------------------

    constexpr double ResidualAllowance = 1.0e-9;                                             // [len/len] - Relative to max(1, size)
    constexpr int    ResidualSamples   = 24;                                                 // [-]       - Sections sampled per sheet

    //--------------------------------------------------------------------------------------------------------------------
    //                                                HELPERS
    //--------------------------------------------------------------------------------------------------------------------

    [[nodiscard]] double OrientationSign(Orientation Sense) noexcept
    {
        return Sense == Orientation::Forward ? 1.0 : -1.0;
    }

    // Outward unit normal of a face at a point on it, using a fin's SP-curve image as the periodic reference.
    [[nodiscard]] Vector3 ResolveOutwardNormal(const SurfaceGeometry& Surface, Orientation Sense, Vector3 Position, Vector2 Reference) noexcept
    {
        return EvaluateSurfaceNormal(Surface, ProjectOntoSurface(Surface, Position, Reference)) * OrientationSign(Sense);
    }

    // 📝 Parameter box for the SSI of an offset support: periodic directions span a full period (no boundary), the
    //    others grow by Margin (converted to an angle on the sphere's latitude).
    [[nodiscard]] ParameterBox ExpandWindow(const SurfaceGeometry& Surface, const ParameterWindow& Window, double Margin) noexcept
    {
        const SurfacePeriodicity Period = ResolvePeriodicity(Surface);
        ParameterBox Box{ Window.Low, Window.High };
        double MarginV = Margin;                                                             // [-] - 𝑣 growth
        if (const auto* Sphere = std::get_if<SphereSurface>(&Surface)) MarginV = Margin / Sphere->Radius;
        if (Period.PeriodU > 0.0) Box.High.X = Box.Low.X + Period.PeriodU;
        else { Box.Low.X -= Margin; Box.High.X += Margin; }
        if (Period.PeriodV > 0.0) Box.High.Y = Box.Low.Y + Period.PeriodV;
        else { Box.Low.Y -= MarginV; Box.High.Y += MarginV; }
        if (std::holds_alternative<SphereSurface>(Surface))
        {
            Box.Low.Y  = std::max(Box.Low.Y, -0.5 * std::numbers::pi);
            Box.High.Y = std::min(Box.High.Y, 0.5 * std::numbers::pi);
        }
        return Box;
    }

    // Foot of a spine point on a support as a contact curve sharing the spine parameter.
    [[nodiscard]] std::optional<CurveGeometry> ResolveContactCurve(const CurveGeometry& Spine, const SurfaceGeometry& Support, double Scale)
    {
        if (const auto* Line = std::get_if<StraightLine>(&Spine))
        {
            const Vector3 Foot = ProjectPointOntoSurface(Support, Line->Origin);             // [len] - Foot at τ = 0
            const Vector3 Next = ProjectPointOntoSurface(Support, Line->Origin + Line->Direction); // [len] - Foot at τ = 1
            if (Distance(Next, Foot + Line->Direction) > 1.0e-9 * Scale) return std::nullopt;
            return StraightLine{ Foot, Line->Direction };
        }
        const auto&   Arc     = std::get<CircularArc>(Spine);
        const Vector3 Foot    = ProjectPointOntoSurface(Support, EvaluateCurve(Spine, 0.0).Position);
        const Vector3 Offset  = Foot - Arc.Frame.Origin;                                     // [len] - From the spine centre
        const double  Height  = Dot(Offset, Arc.Frame.ZAxis);                                // [len] - Axial position
        const Vector3 Radial  = Offset - Arc.Frame.ZAxis * Height;                           // [len] - Radial part
        const double  Reach   = Dot(Radial, Arc.Frame.XAxis);                                // [len] - Contact radius ρ
        if (!(Reach > 1.0e-9 * Scale) || Length(Radial - Arc.Frame.XAxis * Reach) > 1.0e-9 * Scale) return std::nullopt;
        OrthonormalFrame Frame = Arc.Frame;
        Frame.Origin = Arc.Frame.Origin + Arc.Frame.ZAxis * Height;
        return CircularArc{ Frame, Reach };
    }
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    PUBLIC FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

Vector3 ProjectPointOntoSurface(const SurfaceGeometry& Surface, Vector3 Position) noexcept
{
    return EvaluateSurface(Surface, ProjectOntoSurface(Surface, Position, { 0.0, 0.0 })).Position;
}

Vector2 ResolveSheetChart(const EdgeBlendSheet& Sheet, double τ) noexcept
{
    if (Sheet.SpineAxis == 0) return { τ, Sheet.CrossCentre };
    if (Sheet.SpineAxis == 1) return { Sheet.CrossCentre, τ };
    return { 0.0, 0.0 };
}

std::optional<CurveGeometry> SectionCylinderByPlane(const CylinderSurface& Cylinder, Vector3 PlanePoint, Vector3 PlaneNormal) noexcept
{
    const Vector3 Normal = Normalize(PlaneNormal);                                           // [-] - Unit plane normal
    const Vector3 Axis   = Cylinder.Frame.ZAxis;                                             // [-] - Unit cylinder axis
    const double  Cosine = Dot(Normal, Axis);                                                // [-] - cos of the tilt
    if (std::fabs(Cosine) < 1.0e-12) return std::nullopt;
    const Vector3 Centre = Cylinder.Frame.Origin + Axis * (Dot(PlanePoint - Cylinder.Frame.Origin, Normal) / Cosine);
    if (std::fabs(Cosine) > 1.0 - 1.0e-15)
    {
        return CircularArc{ ConstructFrame(Centre, Normal, Cylinder.Frame.XAxis), Cylinder.Radius };
    }
    const Vector3 Minor = Normalize(Cross(Axis, Normal));                                    // [-] - In the plane, ⟂ axis
    const Vector3 Major = Normalize(Cross(Normal, Minor));                                   // [-] - In the plane, along the tilt
    return EllipticalArc{ { Centre, Major, Minor, Cross(Major, Minor) }, Cylinder.Radius / std::fabs(Cosine), Cylinder.Radius };
}

EdgeBlendSheetOutcome ResolveEdgeBlendSheet(const ModelPartition& Partition, EdgeToken Edge, const BlendSettings& Settings)
{
    EdgeBlendSheetOutcome Outcome;
    const auto Refuse = [&](const std::string& Reason) { Outcome.Refusal = LabelToken("E", Edge) + ": " + Reason; return Outcome; };
    if (!(Settings.Size > 0.0)) return Refuse("blend size must be positive");

    // ① Supports, convexity and the wedge angle at mid-edge.
    const EdgeProfile& Profile = Partition.Edges.Resolve(Edge);
    EdgeBlendSheet Sheet;
    Sheet.Edge    = Edge;
    Sheet.Fins[0] = Profile.RadialFin;
    Sheet.Fins[1] = Partition.RadialMate(Profile.RadialFin);
    if (Sheet.Fins[1] == Sheet.Fins[0] || Partition.RadialMate(Sheet.Fins[1]) != Sheet.Fins[0]) return Refuse("not a manifold edge with two faces");
    SurfaceGeometry Supports[2];
    Orientation     Senses[2];
    for (int Side = 0; Side < 2; ++Side)
    {
        Sheet.Faces[Side] = Partition.Loops.Resolve(Partition.Fins.Resolve(Sheet.Fins[Side]).Loop).Face;
        const FaceProfile& Face = Partition.Faces.Resolve(Sheet.Faces[Side]);
        Supports[Side] = Partition.Surfaces.Resolve(Face.Surface);
        Senses[Side]   = Face.Sense;
    }
    if (Sheet.Faces[0] == Sheet.Faces[1]) return Refuse("seam edge (both sides on one face)");
    const CurveGeometry& Carrier = Partition.Curves.Resolve(Profile.Curve);
    const double Middle  = 0.5 * (Profile.IntervalLow + Profile.IntervalHigh);              // [-]   - Mid-edge τ
    const double Extent  = Distance(EvaluateCurve(Carrier, Profile.IntervalLow).Position, EvaluateCurve(Carrier, Middle).Position)
                         + Distance(EvaluateCurve(Carrier, Middle).Position, EvaluateCurve(Carrier, Profile.IntervalHigh).Position);
    const double Scale   = std::max({ 1.0, Settings.Size, Extent });                         // [len] - Tolerance scale
    const auto WedgeAt = [&](double τ, Vector3* Normals)
    {
        const Vector3 Position = EvaluateCurve(Carrier, τ).Position;
        for (int Side = 0; Side < 2; ++Side)
        {
            const FinProfile& Fin = Partition.Fins.Resolve(Sheet.Fins[Side]);
            const Vector2 Reference = Fin.SurfaceCurve ? EvaluateParameterCurve(*Fin.SurfaceCurve, τ).Position : Vector2{ 0.0, 0.0 };
            Normals[Side] = ResolveOutwardNormal(Supports[Side], Senses[Side], Position, Reference);
        }
        return std::acos(std::clamp(-Dot(Normals[0], Normals[1]), -1.0, 1.0));
    };
    Vector3 Normals[2];
    Sheet.Dihedral = WedgeAt(Middle, Normals);
    const Vector3 Position = EvaluateCurve(Carrier, Middle).Position;                        // [len] - Mid-edge point
    const Vector3 Tangent  = Normalize(EvaluateCurve(Carrier, Middle).Derivative);           // [-]   - Edge direction
    const double  Turn     = Dot(Cross(Normals[0], Normals[1]), Tangent * OrientationSign(Partition.Fins.Resolve(Sheet.Fins[0]).Sense));
    if (Length(Cross(Normals[0], Normals[1])) < 1.0e-9) return Refuse("smooth (tangent-continuous) edge: nothing to blend");
    Sheet.ConvexCondition = Turn > 0.0;

    // ② Ball radius; a chamfer needs a constant wedge angle along the edge.
    Sheet.Radius = Settings.Size;
    if (Settings.Profile == BlendProfile::Chamfer)
    {
        for (int Sample = 0; Sample <= 8; ++Sample)
        {
            Vector3 Probe[2];
            const double Angle = WedgeAt(Profile.IntervalLow + (Profile.IntervalHigh - Profile.IntervalLow) * Sample / 8.0, Probe);
            if (std::fabs(Angle - Sheet.Dihedral) > 1.0e-9) return Refuse("chamfer on a variable-angle edge (setback-to-ball mapping not constant, 🚧)");
        }
        Sheet.Radius = Settings.Size * std::tan(0.5 * Sheet.Dihedral);
    }

    // ③ Offsets towards the ball: inside the material for a convex edge, outside for a concave one.
    for (int Side = 0; Side < 2; ++Side)
    {
        const double Distance = (Sheet.ConvexCondition ? -1.0 : 1.0) * OrientationSign(Senses[Side]) * Sheet.Radius;
        const std::optional<SurfaceGeometry> Offset = OffsetSurface(Supports[Side], Distance);
        if (!Offset) return Refuse(std::string("size exceeds the curvature radius of the ") + SurfaceCategoryLabel(Supports[Side]) + " support");
        Sheet.Offsets[Side] = *Offset;
    }

    // ④ Spine: the exact SSI branch of the offsets nearest the edge.
    const double Margin = 4.0 * Sheet.Radius / std::max(std::sin(0.5 * Sheet.Dihedral), 0.1); // [len] - Box growth
    const ParameterBox BoxA = ExpandWindow(Sheet.Offsets[0], DescribeFaceDomain(Partition, Sheet.Faces[0]).Window, Margin);
    const ParameterBox BoxB = ExpandWindow(Sheet.Offsets[1], DescribeFaceDomain(Partition, Sheet.Faces[1]).Window, Margin);
    const SurfaceIntersectionOutcome Section = IntersectSurfaces(Sheet.Offsets[0], BoxA, Sheet.Offsets[1], BoxB);
    if (Section.CoincidentCondition) return Refuse("offset supports coincide");
    // 💡 Lines and circles keep their analytic carriers (cylinder, torus, cone, plane); ellipses and traced branches get
    //    the procedural rolling-ball carrier, whose spine is the exact branch itself (traced: Newton onto both offsets).
    double Nearest = std::numeric_limits<double>::max();                                     // [len] - Spine-to-edge distance
    for (const IntersectionBranch& Branch : Section.Branches)
    {
        CurveGeometry Candidate = Branch.Curve;
        if (Branch.Category == BranchCategory::Traced)
        {
            if (Branch.Chart.size() < 2 || Branch.TangentialCondition) continue;
            std::vector<Vector3> Guide;
            for (const IntersectionChartPoint& Point : Branch.Chart) Guide.push_back(Point.Position);
            Candidate = BuildIntersectionCurve(Sheet.Offsets[0], Sheet.Offsets[1], Guide, Branch.ClosedCondition);
        }
        const double Gap = Distance(Position, EvaluateCurve(Candidate, ProjectOntoCurve(Candidate, Position, 0.0)).Position);
        if (Gap < Nearest) { Nearest = Gap; Sheet.Spine = Candidate; }
    }
    if (Nearest == std::numeric_limits<double>::max()) return Refuse("offset supports do not meet (no spine)");
    const bool AnalyticCondition = std::holds_alternative<StraightLine>(Sheet.Spine) || std::holds_alternative<CircularArc>(Sheet.Spine);   // [-] - Analytic carrier
    if (!AnalyticCondition && Settings.Profile == BlendProfile::Chamfer) return Refuse("chamfer on a traced or elliptical spine (ruled procedural carrier, 🚧)");

    // ⑤ Contact curves (same τ as the spine).
    for (int Side = 0; Side < 2; ++Side)
    {
        const std::optional<CurveGeometry> Contact = AnalyticCondition ? ResolveContactCurve(Sheet.Spine, Supports[Side], Scale)
                                                                       : std::optional<CurveGeometry>{ ConstructContactCurve(Sheet.Spine, Supports[Side]) };
        if (!Contact) return Refuse("contact curve degenerates (ball reaches a support axis)");
        Sheet.Contacts[Side] = *Contact;
    }
    const double SpineMiddle = ProjectOntoCurve(Sheet.Spine, Position, 0.0);                 // [-] - Spine τ of the mid-section
    Sheet.Alignment = Dot(EvaluateCurve(Sheet.Spine, SpineMiddle).Derivative, Tangent) >= 0.0 ? 1.0 : -1.0;

    // ⑥ Exact carrier.
    const Vector3 Centre   = EvaluateCurve(Sheet.Spine, 0.0).Position;                       // [len] - Spine at τ = 0
    const Vector3 FootA    = EvaluateCurve(Sheet.Contacts[0], 0.0).Position;                 // [len] - Contact A at τ = 0
    const Vector3 FootB    = EvaluateCurve(Sheet.Contacts[1], 0.0).Position;                 // [len] - Contact B at τ = 0
    const Vector3 Bisector = Normalize((FootA - Centre) + (FootB - Centre));                 // [-]   - Towards the edge side
    if (!AnalyticCondition)
    {
        // 📝 Procedural carrier: 𝑢 is the spine τ (SpineAxis 0), 𝑣 = 0 on contact A; the cross-section centre is the polar
        //    angle of the bisector at the mid-section.
        const RollingBallSurface Carrier = ConstructRollingBallSurface(Sheet.Spine, Sheet.Radius, Supports[0]);
        const Vector3 BallMiddle  = EvaluateCurve(Sheet.Spine, SpineMiddle).Position;          // [len] - Mid-section ball centre
        const Vector3 MiddleA     = EvaluateCurve(Sheet.Contacts[0], SpineMiddle).Position;    // [len] - Mid-section contact A
        const Vector3 MiddleB     = EvaluateCurve(Sheet.Contacts[1], SpineMiddle).Position;    // [len] - Mid-section contact B
        const Vector3 Towards     = BallMiddle + Normalize(MiddleA + MiddleB - BallMiddle * 2.0) * Sheet.Radius;
        Sheet.Surface     = Carrier;
        Sheet.SpineAxis   = 0;
        Sheet.CrossCentre = ProjectOntoRollingBall(Carrier, Towards, { SpineMiddle, 0.0 }).Y;
    }
    else if (const auto* Line = std::get_if<StraightLine>(&Sheet.Spine))
    {
        if (Settings.Profile == BlendProfile::Fillet)
        {
            Sheet.Surface     = CylinderSurface{ ConstructFrame(Line->Origin, Line->Direction, -Bisector), Sheet.Radius };
            Sheet.SpineAxis   = 1;
            Sheet.CrossCentre = std::numbers::pi;
        }
        else
        {
            const PlaneSurface Plane{ ConstructFrame(FootA, Cross(Line->Direction, FootB - FootA), Line->Direction) };
            Sheet.Surface     = Plane;
            Sheet.SpineAxis   = 0;
            Sheet.CrossCentre = 0.5 * Dot(FootB - FootA, Plane.Frame.YAxis);
        }
    }
    else
    {
        const auto& Arc = std::get<CircularArc>(Sheet.Spine);
        if (Settings.Profile == BlendProfile::Fillet)
        {
            if (!(Arc.Radius > Sheet.Radius * (1.0 + 1.0e-9))) return Refuse("blend tube would cross its own axis (spine radius ≤ ball radius)");
            Sheet.Surface     = TorusSurface{ Arc.Frame, Arc.Radius, Sheet.Radius };
            Sheet.SpineAxis   = 0;
            Sheet.CrossCentre = std::atan2(Dot(Bisector, Arc.Frame.ZAxis), Dot(Bisector, Arc.Frame.XAxis));
        }
        else
        {
            const auto&  ArcA = std::get<CircularArc>(Sheet.Contacts[0]);
            const auto&  ArcB = std::get<CircularArc>(Sheet.Contacts[1]);
            const double HeightA = Dot(ArcA.Frame.Origin - Arc.Frame.Origin, Arc.Frame.ZAxis);  // [len] - Axial position of A
            const double HeightB = Dot(ArcB.Frame.Origin - Arc.Frame.Origin, Arc.Frame.ZAxis);  // [len] - Axial position of B
            OrthonormalFrame Frame = Arc.Frame;
            Frame.Origin = ArcA.Frame.Origin;
            if (std::fabs(HeightB - HeightA) <= 1.0e-12 * Scale)
            {
                Sheet.Surface   = PlaneSurface{ Frame };
                Sheet.SpineAxis = -1;
            }
            else if (std::fabs(ArcB.Radius - ArcA.Radius) <= 1.0e-12 * Scale)
            {
                Sheet.Surface     = CylinderSurface{ Frame, ArcA.Radius };
                Sheet.SpineAxis   = 0;
                Sheet.CrossCentre = 0.5 * (HeightB - HeightA);
            }
            else
            {
                Sheet.Surface     = ConeSurface{ Frame, ArcA.Radius, (ArcB.Radius - ArcA.Radius) / (HeightB - HeightA) };
                Sheet.SpineAxis   = 0;
                Sheet.CrossCentre = 0.5 * (HeightB - HeightA);
            }
        }
    }

    // ⑦ Sampled residuals: ball distance |C_i(τ) − S(τ)| = r, feet on the supports, contacts on the carrier.
    const bool   Periodic = PeriodicCondition(Sheet.Spine);
    const double SpineLow = Periodic ? 0.0 : ProjectOntoCurve(Sheet.Spine, EvaluateCurve(Carrier, Profile.IntervalLow).Position, 0.0);
    const double SpineHigh = Periodic ? 2.0 * std::numbers::pi : ProjectOntoCurve(Sheet.Spine, EvaluateCurve(Carrier, Profile.IntervalHigh).Position, 0.0);
    for (int Sample = 0; Sample <= ResidualSamples; ++Sample)
    {
        const double  τ    = SpineLow + (SpineHigh - SpineLow) * Sample / ResidualSamples;
        const Vector3 Ball = EvaluateCurve(Sheet.Spine, τ).Position;                         // [len] - Ball centre
        for (int Side = 0; Side < 2; ++Side)
        {
            const Vector3 Foot = EvaluateCurve(Sheet.Contacts[Side], τ).Position;            // [len] - Contact point
            Sheet.MaxResidual = std::max({ Sheet.MaxResidual,
                                           std::fabs(Distance(Foot, Ball) - Sheet.Radius),
                                           Distance(ProjectPointOntoSurface(Supports[Side], Ball), Foot),
                                           Distance(ProjectPointOntoSurface(Sheet.Surface, Foot), Foot) });
        }
    }
    if (Sheet.MaxResidual > ResidualAllowance * Scale) return Refuse("rolling-ball residual " + std::to_string(Sheet.MaxResidual) + " exceeds the allowance");

    // ⑧ Orientation: outward points towards the old edge for a convex blend (material removed), away for a concave one.
    const Vector3 BallMiddle = EvaluateCurve(Sheet.Spine, SpineMiddle).Position;             // [len] - Mid-section ball centre
    const Vector3 FootMiddleA = EvaluateCurve(Sheet.Contacts[0], SpineMiddle).Position;      // [len] - Mid-section contact A
    const Vector3 FootMiddleB = EvaluateCurve(Sheet.Contacts[1], SpineMiddle).Position;      // [len] - Mid-section contact B
    const Vector3 Probe = Settings.Profile == BlendProfile::Fillet
                        ? BallMiddle + Normalize(FootMiddleA + FootMiddleB - BallMiddle * 2.0) * Sheet.Radius
                        : (FootMiddleA + FootMiddleB) * 0.5;
    const Vector3 Natural = EvaluateSurfaceNormal(Sheet.Surface, ProjectOntoSurface(Sheet.Surface, Probe, ResolveSheetChart(Sheet, SpineMiddle)));
    const double  Facing  = Dot(Natural, Position - Probe);                                  // [len] - > 0: natural faces the edge
    Sheet.Sense = ((Facing > 0.0) == Sheet.ConvexCondition) ? Orientation::Forward : Orientation::Reversed;
    Outcome.Sheet = Sheet;
    return Outcome;
}

bool SmoothEdgeCondition(const ModelPartition& Partition, EdgeToken Edge)
{
    const EdgeProfile& Profile = Partition.Edges.Resolve(Edge);
    const FinToken     First   = Profile.RadialFin;
    const FinToken     Second  = Partition.RadialMate(First);
    if (Second == First || Partition.RadialMate(Second) != First) return false;
    const CurveGeometry& Carrier = Partition.Curves.Resolve(Profile.Curve);
    const double  Middle   = 0.5 * (Profile.IntervalLow + Profile.IntervalHigh);              // [-] - Mid-edge τ
    const Vector3 Position = EvaluateCurve(Carrier, Middle).Position;
    Vector3 Normals[2];
    int     Side = 0;
    for (FinToken Fin : { First, Second })
    {
        const FinProfile&  Use  = Partition.Fins.Resolve(Fin);
        const FaceProfile& Face = Partition.Faces.Resolve(Partition.Loops.Resolve(Use.Loop).Face);
        const Vector2 Reference = Use.SurfaceCurve ? EvaluateParameterCurve(*Use.SurfaceCurve, Middle).Position : Vector2{ 0.0, 0.0 };
        Normals[Side++] = ResolveOutwardNormal(Partition.Surfaces.Resolve(Face.Surface), Face.Sense, Position, Reference);
    }
    return Length(Cross(Normals[0], Normals[1])) < 1.0e-9 && Dot(Normals[0], Normals[1]) > 0.0;
}

std::vector<EdgeToken> GrowTangentChain(const ModelPartition& Partition, BodyToken Body, const std::vector<EdgeToken>& Seeds)
{
    const BodyInventory Inventory = Partition.CollectInventory(Body);
    std::map<uint32_t, std::vector<std::pair<EdgeToken, int>>> Star;                         // vertex slot → (edge, end)
    for (EdgeToken Edge : Inventory.Edges)
    {
        const EdgeProfile& Profile = Partition.Edges.Resolve(Edge);
        Star[Profile.Start.Slot].push_back({ Edge, 0 });
        Star[Profile.End.Slot].push_back({ Edge, 1 });
    }
    // Unit tangent at an edge end, pointing from the vertex into the edge.
    const auto Inward = [&](EdgeToken Edge, int End)
    {
        const EdgeProfile&   Profile = Partition.Edges.Resolve(Edge);
        const CurveGeometry& Carrier = Partition.Curves.Resolve(Profile.Curve);
        const Vector3 Along = Normalize(EvaluateCurve(Carrier, End == 0 ? Profile.IntervalLow : Profile.IntervalHigh).Derivative);
        return End == 0 ? Along : Along * -1.0;
    };
    std::vector<EdgeToken> Chain;
    std::set<uint32_t>     Chosen;
    for (EdgeToken Seed : Seeds)
    {
        if (Chosen.insert(Seed.Slot).second) Chain.push_back(Seed);
    }
    for (size_t Cursor = 0; Cursor < Chain.size(); ++Cursor)
    {
        const EdgeToken    Edge    = Chain[Cursor];
        const EdgeProfile& Profile = Partition.Edges.Resolve(Edge);
        if (Profile.Start == Profile.End) continue;                                          // Closed rim: no ends to cross
        for (int End = 0; End < 2; ++End)
        {
            const VertexToken Vertex = End == 0 ? Profile.Start : Profile.End;
            const Vector3     Leaving = Inward(Edge, End) * -1.0;                            // [-] - Direction through the vertex
            for (const auto& [Other, OtherEnd] : Star[Vertex.Slot])
            {
                if (Chosen.count(Other.Slot) != 0) continue;
                if (Dot(Leaving, Inward(Other, OtherEnd)) < 1.0 - 1.0e-9) continue;
                if (SmoothEdgeCondition(Partition, Other)) continue;
                Chosen.insert(Other.Slot);
                Chain.push_back(Other);
            }
        }
    }
    return Chain;
}

const char* BlendProfileLabel(BlendProfile Profile) noexcept
{
    return Profile == BlendProfile::Fillet ? "Fillet" : "Chamfer";
}

} // namespace Frontier::SolidArcRemastered
