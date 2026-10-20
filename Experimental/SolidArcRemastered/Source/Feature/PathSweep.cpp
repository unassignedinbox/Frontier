/*====================================================================================================================================
                                                         PATHSWEEP.CPP
====================================================================================================================================*/
// 🧩 Path verbs — each recognised case is rewritten as the exact primitive or sweep that produces the same solid

#include "Feature/PathSweep.h"

#include "Construction/PrimitiveConstruction.h"

#include <cmath>
#include <numbers>

namespace Frontier::SolidArcRemastered
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    INTERNAL FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

namespace
{
    constexpr double AxisSlack = 1.0e-12;                                                    // [-] - Parallel test

    [[nodiscard]] SweepOutcome FromPrimitive(const EditOutcome<BodyToken>& Primitive, const char* Label)
    {
        SweepOutcome Outcome;
        if (!Primitive.SucceededCondition()) { Outcome.Refusal = std::string(Label) + ": " + Primitive.Refusal; return Outcome; }
        Outcome.Body = Primitive.Value;
        return Outcome;
    }

    [[nodiscard]] Vector3 AnyPerpendicular(Vector3 Axis) noexcept { return std::fabs(Axis.Z) > 0.9 ? Vector3{ 1.0, 0.0, 0.0 } : Vector3{ 0.0, 0.0, 1.0 }; }

    [[nodiscard]] SweepOutcome Refuse(std::string Reason)
    {
        SweepOutcome Outcome;
        Outcome.Refusal = std::move(Reason);
        return Outcome;
    }
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    PUBLIC FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

SweepOutcome PipeAlongPath(ModelPartition& Partition, const CurveGeometry& Path, double Low, double High, double Radius)
{
    if (!(Radius > 0.0)) return Refuse("Pipe: radius must be positive");
    if (!(High > Low)) return Refuse("Pipe: empty path range");
    if (const auto* Line = std::get_if<StraightLine>(&Path))
    {
        const Vector3 Start = Line->Origin + Line->Direction * Low;                          // [len] - Path start
        return FromPrimitive(ConstructCylinder(Partition, ConstructFrame(Start, Line->Direction, AnyPerpendicular(Line->Direction)), Radius, High - Low), "Pipe");
    }
    if (const auto* Arc = std::get_if<CircularArc>(&Path))
    {
        if (Radius >= Arc->Radius) return Refuse("Pipe: section radius reaches the arc axis (self-intersecting torus)");
        if (High - Low > 2.0 * std::numbers::pi + 1.0e-12) return Refuse("Pipe: arc range exceeds a full turn");
        // 📝 Sketch plane through the arc centre spanned by the radial direction at Low (x) and the arc axis (y): the
        //    section is the circle at (R, 0), revolved about the sketch y-axis — the revolve turns toward Z × radial,
        //    which is the arc's direction of increasing τ.
        const Vector3 Radial = Arc->Frame.XAxis * std::cos(Low) + Arc->Frame.YAxis * std::sin(Low);   // [-] - Centre → path start
        const OrthonormalFrame Plane{ Arc->Frame.Origin, Radial, Arc->Frame.ZAxis, Cross(Radial, Arc->Frame.ZAxis) };
        SketchProfile Section;
        ProfileSegment Circle;
        Circle.Kind   = ProfileSegmentKind::Circle;
        Circle.Centre = { Arc->Radius, 0.0 };
        Circle.Radius = Radius;
        Circle.Sweep  = 2.0 * std::numbers::pi;
        Circle.Start  = Circle.End = { Arc->Radius + Radius, 0.0 };
        Circle.Name   = "Section";
        ProfileLoop Loop;
        Loop.Segments   = { Circle };
        Loop.SignedArea = std::numbers::pi * Radius * Radius;
        Section.Loops    = { Loop };
        Section.Area     = Loop.SignedArea;
        Section.Centroid = Circle.Centre;
        Section.Scale    = Arc->Radius + Radius;
        SweepOutcome Outcome = RevolveProfile(Partition, Section, Plane, { 0.0, 0.0 }, { 0.0, 1.0 }, High - Low);
        if (!Outcome.Refusal.empty()) Outcome.Refusal = "Pipe: " + Outcome.Refusal;
        return Outcome;
    }
    return Refuse(std::string("Pipe: 🚧 path kind ") + CurveCategoryLabel(Path) + " (lines and arcs are exact)");
}

SweepOutcome SweepProfileAlongLine(ModelPartition& Partition, const SketchProfile& Profile, const OrthonormalFrame& Plane, Vector3 Displacement)
{
    const double Distance = Length(Displacement);                                            // [len] - |d|
    if (!(Distance > 0.0)) return Refuse("Sweep: zero displacement");
    if (Length(Cross(Displacement * (1.0 / Distance), Plane.ZAxis)) > AxisSlack) return Refuse("Sweep: 🚧 oblique sweep (path not ⟂ the profile plane)");
    SweepOutcome Outcome = ExtrudeProfile(Partition, Profile, Plane, Dot(Displacement, Plane.ZAxis));
    if (!Outcome.Refusal.empty()) Outcome.Refusal = "Sweep: " + Outcome.Refusal;
    return Outcome;
}

SweepOutcome LoftCircles(ModelPartition& Partition, Vector3 FirstCentre, Vector3 FirstNormal, double FirstRadius, Vector3 SecondCentre, Vector3 SecondNormal, double SecondRadius)
{
    const Vector3 Join = SecondCentre - FirstCentre;                                         // [len] - Centre line
    const double  Height = Length(Join);                                                     // [len] - Loft length
    if (!(Height > 0.0)) return Refuse("Loft: coincident section centres");
    const Vector3 Axis = Join * (1.0 / Height);
    if (Length(Cross(Normalize(FirstNormal), Axis)) > AxisSlack || Length(Cross(Normalize(SecondNormal), Axis)) > AxisSlack)
    {
        return Refuse("Loft: 🚧 non-coaxial sections need a skinned surface");
    }
    const OrthonormalFrame Frame = ConstructFrame(FirstCentre, Axis, AnyPerpendicular(Axis));
    if (std::fabs(FirstRadius - SecondRadius) <= 1.0e-12 * std::max(FirstRadius, SecondRadius)) return FromPrimitive(ConstructCylinder(Partition, Frame, FirstRadius, Height), "Loft");
    return FromPrimitive(ConstructFrustum(Partition, Frame, FirstRadius, SecondRadius, Height), "Loft");
}

} // namespace Frontier::SolidArcRemastered
