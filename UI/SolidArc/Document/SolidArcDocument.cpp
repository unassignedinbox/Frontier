/*====================================================================================================================================
                                                         SOLIDARCDOCUMENT.CPP
====================================================================================================================================*/
// 🧩 SolidArc document: construct tiles as sketch + sweep pairs, atomic commands, undo / redo, selection upkeep

//------------------------------------------------------------------------------------------------------------------------
//                                                    INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "UI/SolidArc/Document/SolidArcDocument.h"

#include "Engine/GeometryProcessing/Diagnostics/OperationTrace.h"
#include "UI/SolidArc/Document/SolidArcDrawingSheet.h"
#include "UI/SolidArc/Document/SolidArcExchange.h"
#include "UI/SolidArc/Document/SolidArcScene.h"
#include "UI/SolidArc/Document/VehicleCurveImport.h"

#include "Engine/GeometryProcessing/Audit/EnclosedVolume.h"
#include "Engine/GeometryProcessing/Audit/ModelAudit.h"
#include "Engine/GeometryProcessing/Blend/RollingBallGeometry.h"
#include "Engine/GeometryProcessing/Local/FaceChange.h"
#include "Engine/GeometryProcessing/Sketch/SketchCatalogue.h"
#include "Engine/GeometryProcessing/Sketch/SketchSolver.h"
#include "Engine/GeometryProcessing/Sketch/SketchSpline.h"
#include "Engine/GeometryProcessing/Surfacing/CurveNetwork.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <numbers>
#include <limits>
#include <map>
#include <set>

namespace Frontier::SolidArc
{

using namespace Frontier::GeometryProcessing;

namespace
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    CONSTANTS
//------------------------------------------------------------------------------------------------------------------------

using Kind = SketchConstraintKind;
constexpr double Pi = std::numbers::pi;

//------------------------------------------------------------------------------------------------------------------------
//                                                    AUTHORING
//------------------------------------------------------------------------------------------------------------------------

[[nodiscard]] double ElapsedMs(std::chrono::steady_clock::time_point Start)
{
    return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - Start).count();
}

[[nodiscard]] double ParameterNamed(const FeatureTree& Tree, const std::string& Name)
{
    for (const FeatureParameter& Parameter : Tree.Parameters) if (Parameter.Name == Name) return Parameter.Value;
    return 0.0;
}

// 📝 Labels are unique for the life of the document: the next ordinal follows the largest "#n" already used.
[[nodiscard]] int NextStepOrdinal(const FeatureTree& Tree)
{
    int Largest = 0;                                                                         // [-] - Largest ordinal in use
    for (const FeatureStep& Step : Tree.Steps)
    {
        const size_t Mark = Step.Label.rfind('#');
        if (Mark == std::string::npos) continue;
        Largest = std::max(Largest, std::atoi(Step.Label.c_str() + Mark + 1));
    }
    return Largest + 1;
}

// Next free caption for a tile ("Block1", "Block2", …) over every caption in use.
[[nodiscard]] std::string NextCaption(const SolidArcDocument& Document, const std::string& Stem)
{
    for (int Ordinal = 1;; ++Ordinal)
    {
        const std::string Caption = Stem + std::to_string(Ordinal);
        const bool UsedCondition  = std::any_of(Document.Captions.begin(), Document.Captions.end(),
                                                [&](const auto& Entry) { return Entry.second == Caption; });
        if (!UsedCondition) return Caption;
    }
}

struct TileAuthoring
{
    FeatureTree&       Tree;
    const std::string& Caption;

    [[nodiscard]] std::string Named(const std::string& Suffix) const { return Caption + "." + Suffix; }

    void Parameter(const std::string& Suffix, double Value, double Low, double High, const std::string& Description)
    {
        Tree.Parameters.push_back({ Named(Suffix), Value, std::min(Low, Value), std::max(High, Value), Description });
    }

    // A sketch dimension bound to a tile parameter (literal = the parameter's current value).
    void Bound(SketchModel& Sketch, Kind Constraint, int First, int Second, const std::string& Suffix)
    {
        const int Index = AppendSketchConstraint(Sketch, Constraint, First, Second, -1, ParameterNamed(Tree, Named(Suffix)), Suffix);
        Sketch.Constraints[Index].Parameter = Named(Suffix);
    }

    FeatureStep& Step(FeatureStepKind StepKind, const std::string& Label)
    {
        FeatureStep Step;
        Step.Kind  = StepKind;
        Step.Label = Label;
        Tree.Steps.push_back(std::move(Step));
        return Tree.Steps.back();
    }
};

// 📝 Sketch planes: XY (normal +z, elevation = Z) for extrusions, XZ (x → x, sketch y → z, normal −y, elevation = −Y)
//    for revolutions about a vertical axis.
[[nodiscard]] OrthonormalFrame FloorPlane() { return {}; }

[[nodiscard]] OrthonormalFrame UprightPlane()
{
    OrthonormalFrame Plane;
    Plane.XAxis = { 1.0, 0.0, 0.0 };
    Plane.YAxis = { 0.0, 0.0, 1.0 };
    Plane.ZAxis = { 0.0, -1.0, 0.0 };
    return Plane;
}

// ① parameters ② sketch with a fixed origin and every placement bound ③ sweep step. Returns the caption.
std::string AuthorTile(SolidArcDocument& Document, const SolidArcConstructRequest& Request, FeatureCombination Combination)
{
    FeatureTree&      Tree    = Document.Tree;
    const char*       Stem    = SolidArcConstructLabel(Request.Kind);
    const std::string Caption = NextCaption(Document, Stem);
    const int         Ordinal = NextStepOrdinal(Tree);
    const std::string SketchLabel = "Sketch#" + std::to_string(Ordinal);
    TileAuthoring     Author{ Tree, Caption };
    const Vector3     Spot    = Request.Position;
    const size_t      FirstStep      = Tree.Steps.size();
    const size_t      FirstParameter = Tree.Parameters.size();

    switch (Request.Kind)
    {
    case SolidArcConstructKind::Block:
    {
        Author.Parameter("X", Spot.X, Spot.X - 5.0, Spot.X + 5.0, "corner x");
        Author.Parameter("Y", Spot.Y, Spot.Y - 5.0, Spot.Y + 5.0, "corner y");
        Author.Parameter("Z", Spot.Z, Spot.Z - 5.0, Spot.Z + 5.0, "corner z (sketch elevation)");
        Author.Parameter("Width", Request.Width, 0.1, 10.0, "extent along x");
        Author.Parameter("Depth", Request.Depth, 0.1, 10.0, "extent along y");
        Author.Parameter("Height", Request.Height, 0.1, 10.0, "extent along z (extrude)");
        FeatureStep& Section = Author.Step(FeatureStepKind::Sketch, SketchLabel);
        Section.Plane        = FloorPlane();
        Section.Elevation    = ParameterValue(Author.Named("Z"));
        SketchModel& Sketch  = Section.Sketch;
        const int Origin     = AppendSketchPoint(Sketch, { 0.0, 0.0 });
        const SketchRectangle Rectangle = AppendSketchRectangle(Sketch, { Spot.X, Spot.Y }, { Spot.X + Request.Width, Spot.Y + Request.Depth }, 1);
        AppendSketchConstraint(Sketch, Kind::Fixed, Origin, -1, -1, 0.0, "Origin", 0.0);
        Author.Bound(Sketch, Kind::HorizontalDistance, Origin, Rectangle.FirstPoint, "X");
        Author.Bound(Sketch, Kind::VerticalDistance, Origin, Rectangle.FirstPoint, "Y");
        Author.Bound(Sketch, Kind::HorizontalDistance, Rectangle.FirstPoint, Rectangle.FirstPoint + 1, "Width");
        Author.Bound(Sketch, Kind::VerticalDistance, Rectangle.FirstPoint, Rectangle.FirstPoint + 3, "Depth");
        FeatureStep& Sweep = Author.Step(FeatureStepKind::Extrude, "Extrude#" + std::to_string(Ordinal + 1));
        Sweep.Profile      = SketchLabel;
        Sweep.Extent       = ParameterValue(Author.Named("Height"));
        Sweep.Combination  = Combination;
        Sweep.Target       = Request.Target;
        break;
    }
    case SolidArcConstructKind::Cylinder:
    {
        // 📝 Axis z: floor sketch at elevation Z, circle at (X, Y). Axis x: YZ sketch (normal +x) at elevation X, circle at
        //    (Y, Z). Axis y: upright sketch (normal −y) at elevation −Y, circle at (X, Z), extent negated so Height runs +y.
        //    Position is the centre of the base circle in every case (wheel arches: Axis x at the wheel centre).
        const int   Axis      = std::clamp(Request.Axis, 0, 2);
        const char* Along[3]  = { "base x (sketch elevation)", "base y (sketch elevation)", "base z (sketch elevation)" };
        const char* Extents[3] = { "extent along +x (extrude)", "extent along +y (extrude)", "extent along +z (extrude)" };
        Author.Parameter("X", Spot.X, Spot.X - 5.0, Spot.X + 5.0, Axis == 0 ? Along[0] : "axis x");
        Author.Parameter("Y", Spot.Y, Spot.Y - 5.0, Spot.Y + 5.0, Axis == 1 ? Along[1] : "axis y");
        Author.Parameter("Z", Spot.Z, Spot.Z - 5.0, Spot.Z + 5.0, Axis == 2 ? Along[2] : "axis z");
        Author.Parameter("Radius", Request.Radius, 0.05, 5.0, "radius");
        Author.Parameter("Height", Request.Height, 0.1, 10.0, Extents[Axis]);
        const char* Elevation = Axis == 0 ? "X" : Axis == 1 ? "Y" : "Z";
        const char* First     = Axis == 0 ? "Y" : "X";
        const char* Second    = Axis == 2 ? "Y" : "Z";
        const GeometryProcessing::Vector2 Place = Axis == 0 ? GeometryProcessing::Vector2{ Spot.Y, Spot.Z } : Axis == 1 ? GeometryProcessing::Vector2{ Spot.X, Spot.Z } : GeometryProcessing::Vector2{ Spot.X, Spot.Y };
        FeatureStep& Section = Author.Step(FeatureStepKind::Sketch, SketchLabel);
        Section.Plane        = Axis == 0 ? SolidArcSketchPlane(2) : Axis == 1 ? UprightPlane() : FloorPlane();
        Section.Elevation    = ParameterValue(Author.Named(Elevation), Axis == 1 ? -1.0 : 1.0);
        SketchModel& Sketch  = Section.Sketch;
        const int Origin     = AppendSketchPoint(Sketch, { 0.0, 0.0 });
        const int Centre     = AppendSketchPoint(Sketch, Place);
        const int Circle     = AppendSketchCircle(Sketch, Centre, Request.Radius, 1);
        AppendSketchConstraint(Sketch, Kind::Fixed, Origin, -1, -1, 0.0, "Origin", 0.0);
        Author.Bound(Sketch, Kind::HorizontalDistance, Origin, Centre, First);
        Author.Bound(Sketch, Kind::VerticalDistance, Origin, Centre, Second);
        Author.Bound(Sketch, Kind::Radius, Circle, -1, "Radius");
        FeatureStep& Sweep = Author.Step(FeatureStepKind::Extrude, "Extrude#" + std::to_string(Ordinal + 1));
        Sweep.Profile      = SketchLabel;
        Sweep.Extent       = ParameterValue(Author.Named("Height"), Axis == 1 ? -1.0 : 1.0);
        Sweep.Combination  = Combination;
        Sweep.Target       = Request.Target;
        break;
    }
    case SolidArcConstructKind::Sphere:
    {
        // 📝 Half disc: closing line on the axis (a profile line, not construction) + arc through +x, revolved 2π.
        Author.Parameter("X", Spot.X, Spot.X - 5.0, Spot.X + 5.0, "centre x");
        Author.Parameter("Y", Spot.Y, Spot.Y - 5.0, Spot.Y + 5.0, "centre y (sketch elevation)");
        Author.Parameter("Z", Spot.Z, Spot.Z - 5.0, Spot.Z + 5.0, "centre z");
        Author.Parameter("Radius", Request.Radius, 0.05, 5.0, "radius");
        FeatureStep& Section = Author.Step(FeatureStepKind::Sketch, SketchLabel);
        Section.Plane        = UprightPlane();
        Section.Elevation    = ParameterValue(Author.Named("Y"), -1.0);
        SketchModel& Sketch  = Section.Sketch;
        const int Origin     = AppendSketchPoint(Sketch, { 0.0, 0.0 });
        const int Centre     = AppendSketchPoint(Sketch, { Spot.X, Spot.Z });
        const int South      = AppendSketchPoint(Sketch, { Spot.X, Spot.Z - Request.Radius });
        const int North      = AppendSketchPoint(Sketch, { Spot.X, Spot.Z + Request.Radius });
        AppendSketchLine(Sketch, South, North, 1);
        AppendSketchArc(Sketch, Centre, South, North, 2);
        AppendSketchConstraint(Sketch, Kind::Fixed, Origin, -1, -1, 0.0, "Origin", 0.0);
        Author.Bound(Sketch, Kind::HorizontalDistance, Origin, Centre, "X");
        Author.Bound(Sketch, Kind::VerticalDistance, Origin, Centre, "Z");
        AppendSketchConstraint(Sketch, Kind::HorizontalDistance, Centre, South, -1, 0.0, "SouthOnAxis");
        AppendSketchConstraint(Sketch, Kind::HorizontalDistance, Centre, North, -1, 0.0, "NorthOnAxis");
        Author.Bound(Sketch, Kind::VerticalDistance, Centre, North, "Radius");
        FeatureStep& Sweep    = Author.Step(FeatureStepKind::Revolve, "Revolve#" + std::to_string(Ordinal + 1));
        Sweep.Profile         = SketchLabel;
        Sweep.Combination     = Combination;
        Sweep.Target          = Request.Target;
        Sweep.AxisIdentifier  = 1;
        Sweep.Angle           = LiteralValue(2.0 * Pi);
        break;
    }
    case SolidArcConstructKind::Torus:
    {
        Author.Parameter("X", Spot.X, Spot.X - 5.0, Spot.X + 5.0, "axis x");
        Author.Parameter("Y", Spot.Y, Spot.Y - 5.0, Spot.Y + 5.0, "axis y (sketch elevation)");
        Author.Parameter("Z", Spot.Z, Spot.Z - 5.0, Spot.Z + 5.0, "centre z");
        Author.Parameter("Major", Request.Major, 0.2, 5.0, "axis to tube centre");
        Author.Parameter("Minor", Request.Minor, 0.05, 2.0, "tube radius");
        FeatureStep& Section = Author.Step(FeatureStepKind::Sketch, SketchLabel);
        Section.Plane        = UprightPlane();
        Section.Elevation    = ParameterValue(Author.Named("Y"), -1.0);
        SketchModel& Sketch  = Section.Sketch;
        const int Origin     = AppendSketchPoint(Sketch, { 0.0, 0.0 });
        const int AxisStart  = AppendSketchPoint(Sketch, { Spot.X, Spot.Z });
        const int AxisEnd    = AppendSketchPoint(Sketch, { Spot.X, Spot.Z + 1.0 });
        const int Centre     = AppendSketchPoint(Sketch, { Spot.X + Request.Major, Spot.Z });
        AppendSketchLine(Sketch, AxisStart, AxisEnd, 1, true);
        const int Circle     = AppendSketchCircle(Sketch, Centre, Request.Minor, 2);
        AppendSketchConstraint(Sketch, Kind::Fixed, Origin, -1, -1, 0.0, "Origin", 0.0);
        Author.Bound(Sketch, Kind::HorizontalDistance, Origin, AxisStart, "X");
        Author.Bound(Sketch, Kind::VerticalDistance, Origin, AxisStart, "Z");
        AppendSketchConstraint(Sketch, Kind::HorizontalDistance, AxisStart, AxisEnd, -1, 0.0, "AxisUpright");
        AppendSketchConstraint(Sketch, Kind::VerticalDistance, AxisStart, AxisEnd, -1, 1.0, "AxisLength");
        Author.Bound(Sketch, Kind::HorizontalDistance, AxisStart, Centre, "Major");
        AppendSketchConstraint(Sketch, Kind::VerticalDistance, AxisStart, Centre, -1, 0.0, "Level");
        Author.Bound(Sketch, Kind::Radius, Circle, -1, "Minor");
        FeatureStep& Sweep    = Author.Step(FeatureStepKind::Revolve, "Revolve#" + std::to_string(Ordinal + 1));
        Sweep.Profile         = SketchLabel;
        Sweep.Combination     = Combination;
        Sweep.Target          = Request.Target;
        Sweep.AxisIdentifier  = 1;
        Sweep.Angle           = LiteralValue(2.0 * Pi);
        break;
    }
    case SolidArcConstructKind::Cone:
    {
        // 📝 Right triangle on XZ: the leg on the axis is a profile line (as the sphere's diameter) and the revolution axis;
        //    the apex sits Height above the base centre (negative: the cone points down).
        Author.Parameter("X", Spot.X, Spot.X - 5.0, Spot.X + 5.0, "axis x");
        Author.Parameter("Y", Spot.Y, Spot.Y - 5.0, Spot.Y + 5.0, "axis y (sketch elevation)");
        Author.Parameter("Z", Spot.Z, Spot.Z - 5.0, Spot.Z + 5.0, "base z");
        Author.Parameter("Radius", Request.Radius, 0.05, 5.0, "base radius");
        Author.Parameter("Height", Request.Height, 0.1, 10.0, "base → apex along +z");
        FeatureStep& Section = Author.Step(FeatureStepKind::Sketch, SketchLabel);
        Section.Plane        = UprightPlane();
        Section.Elevation    = ParameterValue(Author.Named("Y"), -1.0);
        SketchModel& Sketch  = Section.Sketch;
        const int Origin     = AppendSketchPoint(Sketch, { 0.0, 0.0 });
        const int Base       = AppendSketchPoint(Sketch, { Spot.X, Spot.Z });
        const int Apex       = AppendSketchPoint(Sketch, { Spot.X, Spot.Z + Request.Height });
        const int Rim        = AppendSketchPoint(Sketch, { Spot.X + Request.Radius, Spot.Z });
        AppendSketchLine(Sketch, Apex, Base, 1);
        AppendSketchLine(Sketch, Base, Rim, 2);
        AppendSketchLine(Sketch, Rim, Apex, 3);
        AppendSketchConstraint(Sketch, Kind::Fixed, Origin, -1, -1, 0.0, "Origin", 0.0);
        Author.Bound(Sketch, Kind::HorizontalDistance, Origin, Base, "X");
        Author.Bound(Sketch, Kind::VerticalDistance, Origin, Base, "Z");
        AppendSketchConstraint(Sketch, Kind::HorizontalDistance, Base, Apex, -1, 0.0, "ApexOnAxis");
        Author.Bound(Sketch, Kind::VerticalDistance, Base, Apex, "Height");
        AppendSketchConstraint(Sketch, Kind::VerticalDistance, Base, Rim, -1, 0.0, "Level");
        Author.Bound(Sketch, Kind::HorizontalDistance, Base, Rim, "Radius");
        FeatureStep& Sweep    = Author.Step(FeatureStepKind::Revolve, "Revolve#" + std::to_string(Ordinal + 1));
        Sweep.Profile         = SketchLabel;
        Sweep.Combination     = Combination;
        Sweep.Target          = Request.Target;
        Sweep.AxisIdentifier  = 1;
        Sweep.Angle           = LiteralValue(2.0 * Pi);
        break;
    }
    case SolidArcConstructKind::Wedge:
    {
        // 📝 Right triangle on XZ (upright sketch, normal −y, elevation −Y) extruded by −Depth, so Depth runs along +y as
        //    the y-axis cylinder does. The high side stands at x = X; the slope falls to the floor at x = X + Width.
        Author.Parameter("X", Spot.X, Spot.X - 5.0, Spot.X + 5.0, "corner x (high side)");
        Author.Parameter("Y", Spot.Y, Spot.Y - 5.0, Spot.Y + 5.0, "corner y (sketch elevation)");
        Author.Parameter("Z", Spot.Z, Spot.Z - 5.0, Spot.Z + 5.0, "corner z");
        Author.Parameter("Width", Request.Width, 0.1, 10.0, "extent along x (slope run)");
        Author.Parameter("Depth", Request.Depth, 0.1, 10.0, "extent along y (extrude)");
        Author.Parameter("Height", Request.Height, 0.1, 10.0, "height of the high side");
        FeatureStep& Section = Author.Step(FeatureStepKind::Sketch, SketchLabel);
        Section.Plane        = UprightPlane();
        Section.Elevation    = ParameterValue(Author.Named("Y"), -1.0);
        SketchModel& Sketch  = Section.Sketch;
        const int Origin     = AppendSketchPoint(Sketch, { 0.0, 0.0 });
        const int Heel       = AppendSketchPoint(Sketch, { Spot.X, Spot.Z });
        const int Toe        = AppendSketchPoint(Sketch, { Spot.X + Request.Width, Spot.Z });
        const int Crest      = AppendSketchPoint(Sketch, { Spot.X, Spot.Z + Request.Height });
        AppendSketchLine(Sketch, Heel, Toe, 1);
        AppendSketchLine(Sketch, Toe, Crest, 2);
        AppendSketchLine(Sketch, Crest, Heel, 3);
        AppendSketchConstraint(Sketch, Kind::Fixed, Origin, -1, -1, 0.0, "Origin", 0.0);
        Author.Bound(Sketch, Kind::HorizontalDistance, Origin, Heel, "X");
        Author.Bound(Sketch, Kind::VerticalDistance, Origin, Heel, "Z");
        Author.Bound(Sketch, Kind::HorizontalDistance, Heel, Toe, "Width");
        AppendSketchConstraint(Sketch, Kind::VerticalDistance, Heel, Toe, -1, 0.0, "Level");
        AppendSketchConstraint(Sketch, Kind::HorizontalDistance, Heel, Crest, -1, 0.0, "Upright");
        Author.Bound(Sketch, Kind::VerticalDistance, Heel, Crest, "Height");
        FeatureStep& Sweep = Author.Step(FeatureStepKind::Extrude, "Extrude#" + std::to_string(Ordinal + 1));
        Sweep.Profile      = SketchLabel;
        Sweep.Extent       = ParameterValue(Author.Named("Depth"), -1.0);
        Sweep.Combination  = Combination;
        Sweep.Target       = Request.Target;
        break;
    }
    case SolidArcConstructKind::Prism:
    {
        // 📝 Regular n-gon on XY: every vertex on a construction circle of the bound Radius, every side equal to the first,
        //    the first vertex on a construction ray from the centre at Angle. 2n + 5 unknowns, 2n + 5 equations.
        const int    Sides  = std::clamp(Request.Sides, 3, 64);
        Author.Parameter("X", Spot.X, Spot.X - 5.0, Spot.X + 5.0, "centre x");
        Author.Parameter("Y", Spot.Y, Spot.Y - 5.0, Spot.Y + 5.0, "centre y");
        Author.Parameter("Z", Spot.Z, Spot.Z - 5.0, Spot.Z + 5.0, "base z (sketch elevation)");
        Author.Parameter("Radius", Request.Radius, 0.05, 5.0, "circumradius (centre → vertex)");
        Author.Parameter("Height", Request.Height, 0.1, 10.0, "extent along +z (extrude)");
        FeatureStep& Section = Author.Step(FeatureStepKind::Sketch, SketchLabel);
        Section.Plane        = FloorPlane();
        Section.Elevation    = ParameterValue(Author.Named("Z"));
        SketchModel& Sketch  = Section.Sketch;
        const int Origin     = AppendSketchPoint(Sketch, { 0.0, 0.0 });
        const int Centre     = AppendSketchPoint(Sketch, { Spot.X, Spot.Y });
        const int Ring       = AppendSketchCircle(Sketch, Centre, Request.Radius, 1, true);
        const GeometryProcessing::Vector2 Heading{ std::cos(Request.Angle), std::sin(Request.Angle) };
        const int Aim        = AppendSketchPoint(Sketch, { Spot.X + Heading.X, Spot.Y + Heading.Y });
        const int Ray        = AppendSketchLine(Sketch, Centre, Aim, 2, true);
        std::vector<int> Corners;
        for (int Index = 0; Index < Sides; ++Index)
        {
            const double θ = Request.Angle + 2.0 * Pi * Index / Sides;
            Corners.push_back(AppendSketchPoint(Sketch, { Spot.X + Request.Radius * std::cos(θ), Spot.Y + Request.Radius * std::sin(θ) }));
        }
        std::vector<int> Edges;
        for (int Index = 0; Index < Sides; ++Index) Edges.push_back(AppendSketchLine(Sketch, Corners[Index], Corners[(Index + 1) % Sides], 3 + Index));
        AppendSketchConstraint(Sketch, Kind::Fixed, Origin, -1, -1, 0.0, "Origin", 0.0);
        Author.Bound(Sketch, Kind::HorizontalDistance, Origin, Centre, "X");
        Author.Bound(Sketch, Kind::VerticalDistance, Origin, Centre, "Y");
        Author.Bound(Sketch, Kind::Radius, Ring, -1, "Radius");
        AppendSketchConstraint(Sketch, Kind::HorizontalDistance, Centre, Aim, -1, Heading.X, "AimX");
        AppendSketchConstraint(Sketch, Kind::VerticalDistance, Centre, Aim, -1, Heading.Y, "AimY");
        AppendSketchConstraint(Sketch, Kind::PointOnLine, Corners[0], Ray, -1, 0.0, "FirstOnRay");
        for (int Index = 0; Index < Sides; ++Index)
            AppendSketchConstraint(Sketch, Kind::PointOnCircle, Corners[Index], Ring, -1, 0.0, "OnRing" + std::to_string(Index + 1));
        for (int Index = 1; Index < Sides; ++Index)
            AppendSketchConstraint(Sketch, Kind::EqualLength, Edges[0], Edges[Index], -1, 0.0, "Equal" + std::to_string(Index + 1));
        FeatureStep& Sweep = Author.Step(FeatureStepKind::Extrude, "Extrude#" + std::to_string(Ordinal + 1));
        Sweep.Profile      = SketchLabel;
        Sweep.Extent       = ParameterValue(Author.Named("Height"));
        Sweep.Combination  = Combination;
        Sweep.Target       = Request.Target;
        break;
    }
    case SolidArcConstructKind::Tube:
    {
        // 📝 Annulus on XY: two circles on one centre point (concentric by construction), the inner one a hole of the profile.
        Author.Parameter("X", Spot.X, Spot.X - 5.0, Spot.X + 5.0, "axis x");
        Author.Parameter("Y", Spot.Y, Spot.Y - 5.0, Spot.Y + 5.0, "axis y");
        Author.Parameter("Z", Spot.Z, Spot.Z - 5.0, Spot.Z + 5.0, "base z (sketch elevation)");
        Author.Parameter("Radius", Request.Radius, 0.05, 5.0, "outer radius");
        Author.Parameter("Inner", Request.Inner, 0.02, 5.0, "bore radius");
        Author.Parameter("Height", Request.Height, 0.1, 10.0, "extent along +z (extrude)");
        FeatureStep& Section = Author.Step(FeatureStepKind::Sketch, SketchLabel);
        Section.Plane        = FloorPlane();
        Section.Elevation    = ParameterValue(Author.Named("Z"));
        SketchModel& Sketch  = Section.Sketch;
        const int Origin     = AppendSketchPoint(Sketch, { 0.0, 0.0 });
        const int Centre     = AppendSketchPoint(Sketch, { Spot.X, Spot.Y });
        const int Outer      = AppendSketchCircle(Sketch, Centre, Request.Radius, 1);
        const int Bore       = AppendSketchCircle(Sketch, Centre, Request.Inner, 2);
        AppendSketchConstraint(Sketch, Kind::Fixed, Origin, -1, -1, 0.0, "Origin", 0.0);
        Author.Bound(Sketch, Kind::HorizontalDistance, Origin, Centre, "X");
        Author.Bound(Sketch, Kind::VerticalDistance, Origin, Centre, "Y");
        Author.Bound(Sketch, Kind::Radius, Outer, -1, "Radius");
        Author.Bound(Sketch, Kind::Radius, Bore, -1, "Inner");
        FeatureStep& Sweep = Author.Step(FeatureStepKind::Extrude, "Extrude#" + std::to_string(Ordinal + 1));
        Sweep.Profile      = SketchLabel;
        Sweep.Extent       = ParameterValue(Author.Named("Height"));
        Sweep.Combination  = Combination;
        Sweep.Target       = Request.Target;
        break;
    }
    }
    if (Request.PlaneCondition)
    {
        // 📝 Oriented to a body face: the tile is authored in its own floor frame, then every sketch plane it made is carried
        //    rigidly into the face frame F (o ↦ F.o + F.x̂·oₓ + F.ŷ·o_y + F.n̂·o_z, axes rotated alike). Elevations run along
        //    the carried normals and extrusions / revolutions follow their sketches, so the whole solid turns with the face.
        const OrthonormalFrame& Face = Request.Plane;
        const auto Carry  = [&Face](Vector3 Point) { return Face.XAxis * Point.X + Face.YAxis * Point.Y + Face.ZAxis * Point.Z; };
        for (size_t Index = FirstStep; Index < Tree.Steps.size(); ++Index)
        {
            FeatureStep& Step = Tree.Steps[Index];
            if (Step.Kind != FeatureStepKind::Sketch) continue;
            Step.Plane = { Face.Origin + Carry(Step.Plane.Origin), Carry(Step.Plane.XAxis), Carry(Step.Plane.YAxis), Carry(Step.Plane.ZAxis) };
        }
        for (size_t Index = FirstParameter; Index < Tree.Parameters.size(); ++Index) Tree.Parameters[Index].Description += " · face frame";
    }
    Document.Captions[SketchLabel]            = Caption;
    Document.Captions[Tree.Steps.back().Label] = Caption;
    return Caption;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    COMMAND FRAME
//------------------------------------------------------------------------------------------------------------------------

[[nodiscard]] SolidArcHistoryEntry Snapshot(const SolidArcDocument& Document, const std::string& Label)
{
    return { Document.Tree, Document.Captions, Label, Document.Appearance, false, std::make_shared<const RegenerationCache>(Document.Replay),
             Document.SummaryCheckpoint, Document.Summary };
}

// 📝 Only the newest entries of each stack keep their checkpoints (memory bound); older ones replay when reached.
constexpr size_t HistoryReplayDepth = 8;                                                     // [-] - Entries per stack with checkpoints

void TrimHistoryReplay(SolidArcDocument& Document)
{
    for (std::vector<SolidArcHistoryEntry>* Entries : { &Document.UndoEntries, &Document.RedoEntries })
    {
        for (size_t Index = 0; Index + HistoryReplayDepth < Entries->size(); ++Index) (*Entries)[Index].Replay.reset();
    }
}

// 📝 A matcap-only entry swaps the appearance back and keeps the body (and its Revision): no regeneration.
void Restore(SolidArcDocument& Document, SolidArcHistoryEntry Entry)
{
    Document.Appearance = std::move(Entry.Appearance);
    if (Entry.AppearanceCondition) return;
    Document.Tree     = std::move(Entry.Tree);
    Document.Captions = std::move(Entry.Captions);
    if (Entry.Replay)
    {
        Document.Replay            = *Entry.Replay;
        Document.Summary           = std::move(Entry.Summary);
        Document.SummaryCheckpoint = std::move(Entry.SummaryCheckpoint);
    }
    RegenerateSolidArcDocument(Document);
}

// ① snapshot (flagged matcap-only) ② change the appearance (the callback returns a refusal or "") ③ keep or roll back.
template <typename Change>
SolidArcCommandOutcome RunAppearanceCommand(SolidArcDocument& Document, const std::string& Label, Change&& Apply)
{
    FRONTIER_TRACE_SCOPE("Appearance", Label);
    const auto           Start  = std::chrono::steady_clock::now();
    SolidArcHistoryEntry Before = Snapshot(Document, Label);
    Before.AppearanceCondition  = true;
    SolidArcCommandOutcome Outcome;
    Outcome.Label   = Label;
    Outcome.Refusal = Apply();
    if (!Outcome.Refusal.empty())
    {
        Document.Appearance  = std::move(Before.Appearance);
        Outcome.Milliseconds = ElapsedMs(Start);
        return Outcome;
    }
    Document.UndoEntries.push_back(std::move(Before));
    Document.RedoEntries.clear();
    TrimHistoryReplay(Document);
    Outcome.AcceptedCondition = true;
    Outcome.Milliseconds      = ElapsedMs(Start);
    return Outcome;
}

// 📝 First refusal among the steps at or after First that did not already refuse before the command (RefusedBefore:
//    labels; the steps a command appends are never in it).
[[nodiscard]] std::string RefusalFrom(const SolidArcDocument& Document, size_t First, const std::set<std::string>& RefusedBefore)
{
    for (size_t Index = First; Index < Document.Regeneration.Steps.size(); ++Index)
    {
        const FeatureStepReport& Report = Document.Regeneration.Steps[Index];
        if (Report.Status == FeatureStepStatus::Refused && !RefusedBefore.contains(Report.Label)) return Report.Label + ": " + Report.Refusal;
    }
    return {};
}

[[nodiscard]] std::set<std::string> RefusedLabels(const SolidArcDocument& Document)
{
    std::set<std::string> Labels;
    for (const FeatureStepReport& Report : Document.Regeneration.Steps) if (Report.Status == FeatureStepStatus::Refused) Labels.insert(Report.Label);
    return Labels;
}

// ① snapshot ② change (the callback returns a refusal or "") ③ regenerate ④ roll back when a new step refused, or (edits
//    of existing steps: WatchFrom) a step at or after WatchFrom that regenerated before now refuses.
template <typename Change>
SolidArcCommandOutcome RunCommand(SolidArcDocument& Document, const std::string& Label, bool CaptureCondition, Change&& Apply,
                                  size_t WatchFrom = std::numeric_limits<size_t>::max())
{
    FRONTIER_TRACE_SCOPE("Command", Label);
    const auto                  Start         = std::chrono::steady_clock::now();
    SolidArcHistoryEntry        Before        = Snapshot(Document, Label);
    const size_t                FirstNew      = std::min(Document.Tree.Steps.size(), WatchFrom);
    const std::set<std::string> RefusedBefore = RefusedLabels(Document);
    SolidArcCommandOutcome Outcome;
    Outcome.Label = Label;
    Outcome.Refusal = Apply();
    if (Outcome.Refusal.empty())
    {
        // 📝 The figures (audit, volume) are measured only once the command stands: a refused command rolls back to
        //    the snapshot, whose figures are reused, so measuring the refused body was wasted work.
        RegenerateSolidArcDocumentDeferred(Document, CaptureCondition);
        Outcome.Refusal = RefusalFrom(Document, FirstNew, RefusedBefore);
        if (Outcome.Refusal.empty()) SummarizeSolidArcDocument(Document);
    }
    if (!Outcome.Refusal.empty())
    {
        FRONTIER_TRACE("Command", "refused, rolled back: " + Outcome.Refusal);
        Restore(Document, std::move(Before));
        Outcome.Milliseconds = ElapsedMs(Start);
        return Outcome;
    }
    FRONTIER_TRACE("Command", "accepted · " + std::to_string(Document.Tree.Steps.size()) + " step(s) · " + std::to_string(Document.Regeneration.Bodies.size()) + " body(ies)");
    Document.UndoEntries.push_back(std::move(Before));
    Document.RedoEntries.clear();
    TrimHistoryReplay(Document);
    Outcome.AcceptedCondition = true;
    Outcome.Milliseconds      = ElapsedMs(Start);
    return Outcome;
}

[[nodiscard]] std::string FormatAmount(double Value)
{
    char Text[32];
    std::snprintf(Text, sizeof(Text), "%.4g", Value);
    return Text;
}

// Keeps the names some body still carries (Faces or Edges member of every body's naming).
void KeepResolvedNames(std::vector<std::string>& Names, const std::vector<RegeneratedBody>& Bodies, bool EdgeCondition)
{
    std::set<std::string> Present;
    for (const RegeneratedBody& Body : Bodies)
        for (const auto& Entry : EdgeCondition ? Body.Naming.Edges : Body.Naming.Faces) Present.insert(Entry.second);
    std::erase_if(Names, [&](const std::string& Name) { return !Present.contains(Name); });
}

[[nodiscard]] const RegeneratedBody* FindRegeneratedBody(const SolidArcDocument& Document, const std::string& Key)
{
    for (const RegeneratedBody& Body : Document.Regeneration.Bodies) if (Body.Key == Key) return &Body;
    return nullptr;
}

// 📝 The one body carrying every name (faces or edges); refusal text when they span bodies or none carries one.
[[nodiscard]] std::string CommonBody(const SolidArcDocument& Document, SolidArcSelectionKind Kind, const std::vector<std::string>& Names, std::string& Key)
{
    Key.clear();
    // ① The bodies carrying every name (a cut's pieces share names): the picked body, the active one, else the first.
    std::vector<std::string> Carriers;
    for (const RegeneratedBody& Body : Document.Regeneration.Bodies)
    {
        const std::map<uint32_t, std::string>& Table = Kind == SolidArcSelectionKind::Edge ? Body.Naming.Edges : Body.Naming.Faces;
        std::set<std::string> Present;
        for (const auto& Entry : Table) Present.insert(Entry.second);
        if (!Names.empty() && std::all_of(Names.begin(), Names.end(), [&](const std::string& Name) { return Present.contains(Name); })) Carriers.push_back(Body.Key);
    }
    for (const std::string& Preferred : { Document.Selection.PickedBody, Document.Regeneration.Active })
        if (!Preferred.empty() && std::find(Carriers.begin(), Carriers.end(), Preferred) != Carriers.end()) { Key = Preferred; return {}; }
    if (!Carriers.empty()) { Key = Carriers.front(); return {}; }
    // ② None carries them all: say why.
    for (const std::string& Name : Names)
    {
        const std::string Owner = SolidArcBodyOfName(Document, Kind, Name);
        if (Owner.empty()) return (Kind == SolidArcSelectionKind::Edge ? "edge " : "face ") + Name + " is on no body";
        if (!Key.empty() && Owner != Key)
            return "the selection spans " + SolidArcBodyCaption(Document, Key) + " and " + SolidArcBodyCaption(Document, Owner) + " (one body at a time)";
        Key = Owner;
    }
    return {};
}

} // namespace

//------------------------------------------------------------------------------------------------------------------------
//                                                    PUBLIC FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

SolidArcDocument CreateSolidArcDocument(const std::string& Title)
{
    SolidArcDocument Document;
    Document.Title       = Title;
    Document.Tree.Name   = Title;
    Document.Partition   = std::make_unique<ModelPartition>();
    return Document;
}

void RegenerateSolidArcDocument(SolidArcDocument& Document, bool CaptureCondition)
{
    RegenerateSolidArcDocumentDeferred(Document, CaptureCondition);
    SummarizeSolidArcDocument(Document);
}

void RegenerateSolidArcDocumentDeferred(SolidArcDocument& Document, bool CaptureCondition)
{
    FRONTIER_TRACE_SCOPE("Document", std::string("RegenerateSolidArcDocument") + (CaptureCondition ? " · capture" : ""));
    Document.Partition    = std::make_unique<ModelPartition>();
    Document.Regeneration = RegenerateFeatureTree(*Document.Partition, Document.Tree, CaptureCondition, &Document.Replay);
    // 📝 One counter for every document in the process, so a staged preview never shares a scene key with its source.
    static std::atomic<uint64_t> RevisionCounter{ 0 };
    Document.Revision     = ++RevisionCounter;
    KeepResolvedNames(Document.Selection.Faces, Document.Regeneration.Bodies, false);
    KeepResolvedNames(Document.Selection.Edges, Document.Regeneration.Bodies, true);
    std::erase_if(Document.Selection.Bodies, [&](const std::string& Key) { return !FindRegeneratedBody(Document, Key); });
    // Picked curves are SpaceCurves or sketches (loft sections, gizmo); either kind survives while its label still solves.
    std::erase_if(Document.Selection.Curves, [&](const std::string& Label) { return !Document.Regeneration.Curves.contains(Label) && !Document.Regeneration.Sketches.contains(Label); });
}

void SummarizeSolidArcDocument(SolidArcDocument& Document)
{
    FRONTIER_TRACE_SCOPE("Document", "SummarizeSolidArcDocument");
    // Every step restored from the checkpoint the summary was measured on: the bodies are those bodies (same records),
    // so the audit / volume figures still hold; only the captions are refreshed (renames do not touch the tree).
    const std::shared_ptr<const ReplayCheckpoint> Final = Document.Replay.Checkpoints.empty() ? nullptr : Document.Replay.Checkpoints.back();
    const bool ReusedCondition = Final && Document.SummaryCheckpoint == Final && Document.Regeneration.Resumed == static_cast<int>(Document.Tree.Steps.size());
    Document.SummaryCheckpoint = Final;
    SolidArcBodySummary& Summary = Document.Summary;
    if (ReusedCondition)
    {
        Summary.RegenerationMs = Document.Regeneration.Milliseconds;
        for (SolidArcBodyFigures& Figures : Summary.Bodies) Figures.Caption = SolidArcBodyCaption(Document, Figures.Key);
    }
    else
    {
        Summary                      = {};
        Summary.RegenerationMs       = Document.Regeneration.Milliseconds;
        Summary.SoundCondition       = true;
        for (const RegeneratedBody& Regenerated : Document.Regeneration.Bodies)
        {
            const BodyToken Body = Regenerated.Body;
            if (!Body.PresentCondition() || !Document.Partition->Bodies.Authenticate(Body)) continue;
            const BodyInventory Inventory = Document.Partition->CollectInventory(Body);
            if (Inventory.Faces.empty()) continue;
            const AuditReport   Audit     = AuditBody(*Document.Partition, Body);
            SolidArcBodyFigures Figures;
            Figures.Key            = Regenerated.Key;
            Figures.Caption        = SolidArcBodyCaption(Document, Regenerated.Key);
            Figures.SoundCondition = Audit.SoundCondition();
            Figures.SheetCondition = Document.Partition->Bodies.Resolve(Body).Category == BodyCategory::Sheet;
            Figures.Faces          = static_cast<int>(Inventory.Faces.size());
            Figures.Edges          = static_cast<int>(Inventory.Edges.size());
            Figures.Vertices       = static_cast<int>(Inventory.Vertices.size());
            Figures.Genus          = Audit.Genus;
            // A moved body's volume was integrated by the move's own assembly (same shells, same integral): reused.
            // The audit integrated every solid shell already (G6, same shells, same order): its total is the volume.
            const auto Known       = std::find_if(Document.Regeneration.Volumes.begin(), Document.Regeneration.Volumes.end(), [&](const auto& Entry) { return Entry.first == Body; });
            Figures.Volume         = Figures.SheetCondition ? 0.0 : Known != Document.Regeneration.Volumes.end() ? Known->second
                                   : Audit.VolumeCondition ? Audit.Volume : IntegrateBodyVolume(*Document.Partition, Body);
            Summary.PresentCondition = true;
            Summary.SoundCondition   = Summary.SoundCondition && Figures.SoundCondition;
            Summary.SheetCondition   = Summary.SheetCondition || Figures.SheetCondition;
            Summary.Faces           += Figures.Faces;
            Summary.Edges           += Figures.Edges;
            Summary.Vertices        += Figures.Vertices;
            Summary.Genus            = (Summary.Genus < 0 ? 0 : Summary.Genus) + Figures.Genus;
            Summary.Volume          += Figures.Volume;
            Summary.Bodies.push_back(std::move(Figures));
        }
        if (!Summary.PresentCondition) Summary.SoundCondition = false;
    }
}

SolidArcCommandOutcome AdoptSolidArcPreview(SolidArcDocument& Document, SolidArcDocument&& Preview, const SolidArcCommandOutcome& Staged)
{
    FRONTIER_TRACE_SCOPE("Command", Staged.Label + " · adopted from the preview");
    const auto           Start  = std::chrono::steady_clock::now();
    SolidArcHistoryEntry Before = Snapshot(Document, Staged.Label);
    Document.Tree              = std::move(Preview.Tree);
    Document.Captions          = std::move(Preview.Captions);
    Document.Partition         = std::move(Preview.Partition);
    Document.Regeneration      = std::move(Preview.Regeneration);
    Document.Summary           = std::move(Preview.Summary);
    Document.Selection         = std::move(Preview.Selection);
    Document.Revision          = Preview.Revision;
    Document.Appearance        = std::move(Preview.Appearance);
    Document.Replay            = std::move(Preview.Replay);
    Document.SummaryCheckpoint = std::move(Preview.SummaryCheckpoint);
    Document.UndoEntries.push_back(std::move(Before));
    Document.RedoEntries.clear();
    TrimHistoryReplay(Document);
    SolidArcCommandOutcome Outcome = Staged;
    Outcome.Milliseconds          += ElapsedMs(Start);
    return Outcome;
}

SolidArcDocument CloneSolidArcDocument(const SolidArcDocument& Source)
{
    SolidArcDocument Copy = CreateSolidArcDocument(Source.Title);
    Copy.Tree      = Source.Tree;
    Copy.Captions  = Source.Captions;
    Copy.Selection = Source.Selection;
    Copy.Appearance = Source.Appearance;
    Copy.DrawingSheet = Source.DrawingSheet;
    Copy.Replay     = Source.Replay;
    Copy.Summary    = Source.Summary;
    Copy.SummaryCheckpoint = Source.SummaryCheckpoint;
    RegenerateSolidArcDocument(Copy);
    return Copy;
}

SolidArcCommandOutcome ConstructSolidArcPrimitive(SolidArcDocument& Document, const SolidArcConstructRequest& Request)
{
    const bool        FirstBodyCondition = !Document.Summary.PresentCondition;
    const std::string Label = NextCaption(Document, SolidArcConstructLabel(Request.Kind)) + " · " +
                              (FirstBodyCondition && Request.Combination != FeatureCombination::Separate ? "New" : FeatureCombinationLabel(Request.Combination));
    return RunCommand(Document, Label, false, [&]() -> std::string
    {
        // 📝 Only the dimensions the tile reads are checked (a cone's Height may be negative: it points down).
        const SolidArcConstructKind Tile = Request.Kind;
        const bool BoxLike = Tile == SolidArcConstructKind::Block || Tile == SolidArcConstructKind::Wedge;
        const bool Round   = Tile == SolidArcConstructKind::Cylinder || Tile == SolidArcConstructKind::Sphere || Tile == SolidArcConstructKind::Cone ||
                             Tile == SolidArcConstructKind::Prism || Tile == SolidArcConstructKind::Tube;
        const bool Tall    = Tile != SolidArcConstructKind::Sphere && Tile != SolidArcConstructKind::Torus;
        if ((BoxLike && (Request.Width <= 0.0 || Request.Depth <= 0.0)) || (Round && Request.Radius <= 0.0) ||
            (Tall && (Tile == SolidArcConstructKind::Cone ? Request.Height == 0.0 : Request.Height <= 0.0)))
            return "every dimension must be positive";
        if (Tile == SolidArcConstructKind::Torus && (Request.Minor <= 0.0 || Request.Major <= Request.Minor)) return "torus: Major must exceed Minor > 0";
        if (Tile == SolidArcConstructKind::Tube && (Request.Inner <= 0.0 || Request.Inner >= Request.Radius)) return "tube: the bore must lie inside the outer radius";
        if (Tile == SolidArcConstructKind::Prism && (Request.Sides < 3 || Request.Sides > 64)) return "prism: 3 … 64 sides";
        if (!FirstBodyCondition && Request.Combination == FeatureCombination::New)
            return "a second body needs Union, Difference, Intersection or New body";
        if (!Request.Target.empty() && !FindRegeneratedBody(Document, Request.Target)) return "no body named " + Request.Target;
        if (Request.PlaneCondition)
        {
            const OrthonormalFrame& Face = Request.Plane;
            const bool Finite = std::isfinite(Face.Origin.X) && std::isfinite(Face.Origin.Y) && std::isfinite(Face.Origin.Z);
            const bool Square = std::abs(Length(Face.XAxis) - 1.0) < 1.0e-9 && std::abs(Length(Face.YAxis) - 1.0) < 1.0e-9 && std::abs(Length(Face.ZAxis) - 1.0) < 1.0e-9
                             && std::abs(Dot(Face.XAxis, Face.YAxis)) < 1.0e-9 && Length(Cross(Face.XAxis, Face.YAxis) - Face.ZAxis) < 1.0e-9;
            if (!Finite || !Square) return "the face plane is not an orthonormal right-handed frame";
        }
        if (Request.Start > 0.0) return "a construct start lies on or below its plane (start ≤ 0)";
        const size_t Authored = Document.Tree.Steps.size();
        AuthorTile(Document, Request, FirstBodyCondition && Request.Combination != FeatureCombination::Separate ? FeatureCombination::New : Request.Combination);
        // A footprint on the tangent plane of a curved face starts sunk into it (MeasureSolidArcProfileSink), so the
        // Union closes instead of touching along a line.
        if (Request.Start < 0.0)
            for (size_t Index = Authored; Index < Document.Tree.Steps.size(); ++Index)
            {
                FeatureStep& Step = Document.Tree.Steps[Index];
                if (Step.Kind == FeatureStepKind::Extrude && Step.Combination == FeatureCombination::Union && EvaluateFeatureValue(Document.Tree, Step.Extent) > 0.0)
                    Step.Start = FeatureValue{ Request.Start };
            }
        return {};
    });
}

SolidArcCommandOutcome BlendSolidArcSelection(SolidArcDocument& Document, BlendProfile Shape, double Size)
{
    SolidArcBlendRequest Request;
    Request.Shape = Shape;
    Request.Size  = Size;
    return BlendSolidArcSelection(Document, Request);
}

// PropagateCondition: the step takes the picked edges' tangent chains too; Grown says whether that added any edge.
static SolidArcCommandOutcome BlendSolidArcEdges(SolidArcDocument& Document, const SolidArcBlendRequest& Request, bool PropagateCondition, bool& Grown)
{
    const bool        VariableCondition = Request.EndSize.has_value() && Request.VertexSizes.empty();
    const bool        HandleCondition   = !Request.VertexSizes.empty();
    const std::string Stem  = Request.Shape == BlendProfile::Fillet ? "Fillet" : "Chamfer";
    const std::string Sizes = FormatAmount(Request.Size) + (VariableCondition ? " → " + FormatAmount(*Request.EndSize) : std::string())
                            + (HandleCondition ? " · " + std::to_string(Request.VertexSizes.size()) + " corner size(s)" : std::string());
    const std::string Label = Stem + " " + Sizes + " on " + std::to_string(Document.Selection.Edges.size()) + " edge(s)";
    return RunCommand(Document, Label, true, [&]() -> std::string
    {
        if (Document.Selection.Edges.empty()) return "select one or more edges first";
        if (Request.Size <= 0.0 || (VariableCondition && *Request.EndSize <= 0.0)) return "the blend size must be positive";
        for (const auto& [Vertex, Size] : Request.VertexSizes) if (!(Size > 0.0)) return "the corner size at " + Vertex + " must be positive";
        std::string Owner;
        if (const std::string Problem = CommonBody(Document, SolidArcSelectionKind::Edge, Document.Selection.Edges, Owner); !Problem.empty()) return Problem;
        const RegeneratedBody* Target = FindRegeneratedBody(Document, Owner);

        // ① Variable: name each edge's anchor vertex on the current body (start, or end when reversed).
        std::vector<TopologyReference> Anchors;
        if (VariableCondition)
        {
            if (!Document.Partition || !Target) return "there is no body to blend";
            const BodyNaming& Naming = Target->Naming;
            for (const std::string& EdgeName : Document.Selection.Edges)
            {
                const auto Edge = std::find_if(Naming.Edges.begin(), Naming.Edges.end(), [&](const auto& Entry) { return Entry.second == EdgeName; });
                if (Edge == Naming.Edges.end()) return "edge " + EdgeName + " is not on the current body";
                const EdgeProfile& Profile = Document.Partition->Edges.Resolve(Document.Partition->Edges.TokenAt(Edge->first));
                if (Profile.Start == Profile.End) return "variable size on a closed edge (the size would jump at its only vertex)";
                const VertexToken Anchor = Request.ReversedCondition ? Profile.End : Profile.Start;
                const auto Vertex = Naming.Vertices.find(Anchor.Slot);
                if (Vertex == Naming.Vertices.end()) return "the anchor vertex of " + EdgeName + " has no persistent name";
                Anchors.push_back({ ReferenceKind::Vertex, Vertex->second, {} });
            }
        }

        // ①′ Handles: every listed vertex must be an end of a picked edge on the current body.
        std::vector<TopologyReference> Corners;
        if (HandleCondition)
        {
            if (!Document.Partition || !Target) return "there is no body to blend";
            std::set<std::string> Ends;
            for (const std::string& EdgeName : Document.Selection.Edges)
            {
                const auto Edge = std::find_if(Target->Naming.Edges.begin(), Target->Naming.Edges.end(), [&](const auto& Entry) { return Entry.second == EdgeName; });
                if (Edge == Target->Naming.Edges.end()) return "edge " + EdgeName + " is not on the current body";
                const EdgeProfile& Profile = Document.Partition->Edges.Resolve(Document.Partition->Edges.TokenAt(Edge->first));
                for (VertexToken End : { Profile.Start, Profile.End })
                    if (const auto Name = Target->Naming.Vertices.find(End.Slot); Name != Target->Naming.Vertices.end()) Ends.insert(Name->second);
            }
            std::set<std::string> Seen;
            for (const auto& [Vertex, Size] : Request.VertexSizes)
            {
                if (!Ends.contains(Vertex)) return "corner " + Vertex + " is not an end of a picked edge";
                if (!Seen.insert(Vertex).second) return "corner " + Vertex + " has two sizes";
                Corners.push_back({ ReferenceKind::Vertex, Vertex, {} });
            }
        }

        // ② Parameters and the step.
        const std::string Caption   = NextCaption(Document, Stem);
        const std::string StepLabel = "Blend#" + std::to_string(NextStepOrdinal(Document.Tree));
        double            Larger    = std::max(Request.Size, VariableCondition ? *Request.EndSize : Request.Size);
        for (const auto& Entry : Request.VertexSizes) Larger = std::max(Larger, Entry.second);
        const char*       Meaning   = Request.Shape == BlendProfile::Fillet ? "fillet radius" : "chamfer setback";
        Document.Tree.Parameters.push_back({ Caption + ".Size", Request.Size, 0.0, std::max(1.0, 4.0 * Larger),
                                             VariableCondition ? std::string(Meaning) + " at the anchor corners" : std::string(Meaning) });
        FeatureStep Step;
        Step.Kind  = FeatureStepKind::Blend;
        Step.Label = StepLabel;
        Step.Shape  = Request.Shape;
        Step.Size   = ParameterValue(Caption + ".Size");
        Step.Target = Owner;
        // 💡 Tangent propagation (constant size, on refusal): an edge that runs on G1 into another sharp edge (the rim of a
        //    rounded end) takes the whole chain, as CAD fillets do — a blend cannot end where its support turns tangent.
        std::vector<std::string> Chain = Document.Selection.Edges;
        if (PropagateCondition && !VariableCondition && !HandleCondition && Document.Partition && Target)
        {
            std::vector<EdgeToken> Seeds;
            for (const EdgeToken Edge : Document.Partition->CollectInventory(Target->Body).Edges)
                if (const auto Found = Target->Naming.Edges.find(Edge.Slot); Found != Target->Naming.Edges.end() && std::find(Chain.begin(), Chain.end(), Found->second) != Chain.end()) Seeds.push_back(Edge);
            for (const EdgeToken Edge : GrowTangentChain(*Document.Partition, Target->Body, Seeds))
                if (const auto Found = Target->Naming.Edges.find(Edge.Slot); Found != Target->Naming.Edges.end() && std::find(Chain.begin(), Chain.end(), Found->second) == Chain.end()) Chain.push_back(Found->second);
        }
        Grown = Chain.size() > Document.Selection.Edges.size();
        if (PropagateCondition && !Grown) return "no tangent chain to propagate";
        for (const std::string& Name : Chain) Step.References.push_back({ ReferenceKind::Edge, Name, {} });
        if (VariableCondition)
        {
            Document.Tree.Parameters.push_back({ Caption + ".EndSize", *Request.EndSize, 0.0, std::max(1.0, 4.0 * Larger), std::string(Meaning) + " at the far corners" });
            Step.EndSize     = ParameterValue(Caption + ".EndSize");
            Step.SizeAnchors = std::move(Anchors);
        }
        for (size_t Index = 0; Index < Request.VertexSizes.size(); ++Index)
        {
            const std::string Name = Caption + ".Corner" + std::to_string(Index + 1);
            Document.Tree.Parameters.push_back({ Name, Request.VertexSizes[Index].second, 0.0, std::max(1.0, 4.0 * Larger), std::string(Meaning) + " at " + Request.VertexSizes[Index].first });
            Step.AnchorSizes.push_back(ParameterValue(Name));
        }
        if (HandleCondition) Step.SizeAnchors = std::move(Corners);
        Document.Tree.Steps.push_back(std::move(Step));
        Document.Captions[StepLabel] = Caption;
        return {};
    });
}

// The picked edges alone first; refused, their tangent chains (the blend could not end at a G1 joint) when they grow.
SolidArcCommandOutcome BlendSolidArcSelection(SolidArcDocument& Document, const SolidArcBlendRequest& Request)
{
    bool Grown = false;
    const SolidArcCommandOutcome Alone = BlendSolidArcEdges(Document, Request, false, Grown);
    if (Alone.AcceptedCondition || Request.EndSize.has_value() || !Request.VertexSizes.empty()) return Alone;
    const SolidArcCommandOutcome Chained = BlendSolidArcEdges(Document, Request, true, Grown);
    return Chained.AcceptedCondition ? Chained : Alone;
}

// Edge tokens of the picked edge names on the one body carrying them (empty when they span bodies or none resolves).
[[nodiscard]] std::vector<EdgeToken> ResolvePickedEdges(const SolidArcDocument& Document, const std::vector<std::string>& Edges, const RegeneratedBody*& Owner)
{
    Owner = nullptr;
    std::string Key;
    if (!Document.Partition || Edges.empty() || !CommonBody(Document, SolidArcSelectionKind::Edge, Edges, Key).empty()) return {};
    Owner = FindRegeneratedBody(Document, Key);
    if (!Owner) return {};
    std::vector<EdgeToken> Tokens;
    for (const std::string& Name : Edges)
    {
        const auto Entry = std::find_if(Owner->Naming.Edges.begin(), Owner->Naming.Edges.end(), [&](const auto& Pair) { return Pair.second == Name; });
        if (Entry == Owner->Naming.Edges.end()) return {};
        Tokens.push_back(Document.Partition->Edges.TokenAt(Entry->first));
    }
    return Tokens;
}

// The two faces of a manifold edge (the second equals the first on a lamina edge).
[[nodiscard]] std::array<FaceToken, 2> EdgeFaces(const ModelPartition& Partition, EdgeToken Edge)
{
    const FinToken Entry = Partition.Edges.Resolve(Edge).RadialFin;
    const FinToken Mate  = Partition.RadialMate(Entry);
    return { Partition.Loops.Resolve(Partition.Fins.Resolve(Entry).Loop).Face, Partition.Loops.Resolve(Partition.Fins.Resolve(Mate).Loop).Face };
}

std::vector<SolidArcBlendHandle> MeasureSolidArcBlendHandles(const SolidArcDocument& Document, const std::vector<std::string>& Edges)
{
    const RegeneratedBody*       Owner  = nullptr;
    const std::vector<EdgeToken> Tokens = ResolvePickedEdges(Document, Edges, Owner);
    if (Tokens.empty()) return {};
    const ModelPartition& Partition = *Document.Partition;
    const auto Bisector = [&](const std::vector<std::pair<FaceToken, Vector3>>& Feet)
    {
        Vector3 Sum{};
        std::set<uint32_t> Faces;
        for (const auto& [Face, Position] : Feet) if (Faces.insert(Face.Slot).second) Sum = Sum + EvaluateOutwardNormal(Partition, Face, Position);
        return Length(Sum) > 1.0e-9 ? Normalize(Sum) : Vector3{};
    };

    // ① The uniform handle at the middle of the first edge.
    std::vector<SolidArcBlendHandle> Handles;
    {
        const EdgeProfile& Profile = Partition.Edges.Resolve(Tokens.front());
        const Vector3      Middle  = EvaluateCurve(Partition.Curves.Resolve(Profile.Curve), 0.5 * (Profile.IntervalLow + Profile.IntervalHigh)).Position;
        const auto         Faces   = EdgeFaces(Partition, Tokens.front());
        Handles.push_back({ {}, Middle, Bisector({ { Faces[0], Middle }, { Faces[1], Middle } }), int(Tokens.size()) });
    }

    // ② One per distinct end vertex of the open picked edges (a closed edge's only vertex cannot take its own size).
    std::map<uint32_t, std::vector<std::pair<FaceToken, Vector3>>> Feet;
    std::map<uint32_t, int>  Counts;
    std::set<uint32_t>       Closed;
    std::vector<uint32_t>    Order;
    for (EdgeToken Edge : Tokens)
    {
        const EdgeProfile& Profile = Partition.Edges.Resolve(Edge);
        const auto         Faces   = EdgeFaces(Partition, Edge);
        if (Profile.Start == Profile.End) { Closed.insert(Profile.Start.Slot); continue; }
        for (VertexToken End : { Profile.Start, Profile.End })
        {
            const Vector3 Position = Partition.Vertices.Resolve(End).Position;
            if (!Counts.contains(End.Slot)) Order.push_back(End.Slot);
            Counts[End.Slot] += 1;
            Feet[End.Slot].push_back({ Faces[0], Position });
            Feet[End.Slot].push_back({ Faces[1], Position });
        }
    }
    for (uint32_t Slot : Order)
    {
        const auto Name = Owner->Naming.Vertices.find(Slot);
        if (Closed.contains(Slot) || Name == Owner->Naming.Vertices.end()) continue;
        const Vector3 Direction = Bisector(Feet[Slot]);
        if (Length(Direction) < 0.5) continue;
        Handles.push_back({ Name->second, Partition.Vertices.Resolve(Partition.Vertices.TokenAt(Slot)).Position, Direction, Counts[Slot] });
    }
    return Handles;
}

SolidArcBlendFit MeasureSolidArcBlendFit(const SolidArcDocument& Document, const std::vector<std::string>& Edges)
{
    SolidArcBlendFit             Fit;
    const RegeneratedBody*       Owner  = nullptr;
    const std::vector<EdgeToken> Tokens = ResolvePickedEdges(Document, Edges, Owner);
    if (Tokens.empty()) { Fit.Refusal = "no picked edges on one body"; return Fit; }
    const ModelPartition& Partition = *Document.Partition;
    double Span = std::numeric_limits<double>::infinity();
    for (EdgeToken Edge : Tokens)
    {
        const EdgeProfile& Profile = Partition.Edges.Resolve(Edge);
        const double       Along   = MeasureEdgeLength(Partition, Edge);
        Span = std::min(Span, Profile.Start == Profile.End ? Along / (2.0 * std::numbers::pi) : Along);   // Closed: its radius when it is a circle
        // The other edges of both faces that touch this edge's ends bound how far the blend can roll onto the faces.
        for (FaceToken Face : EdgeFaces(Partition, Edge))
        {
            for (LoopToken Loop : Partition.Faces.Resolve(Face).Loops)
            {
                const FinToken First = Partition.Loops.Resolve(Loop).FirstFin;
                if (!First.PresentCondition()) continue;
                FinToken Fin = First;
                for (int Guard = 0; Guard < 100000; ++Guard)
                {
                    const EdgeToken Other = Partition.Fins.Resolve(Fin).Edge;
                    if (Other != Edge)
                    {
                        const EdgeProfile& Neighbour = Partition.Edges.Resolve(Other);
                        const bool Touching = Neighbour.Start == Profile.Start || Neighbour.Start == Profile.End || Neighbour.End == Profile.Start || Neighbour.End == Profile.End;
                        if (Touching && Neighbour.Start != Neighbour.End) Span = std::min(Span, MeasureEdgeLength(Partition, Other));
                    }
                    Fin = Partition.Fins.Resolve(Fin).Successor;
                    if (Fin == First) break;
                }
            }
        }
    }
    if (!std::isfinite(Span) || !(Span > 0.0)) { Fit.Refusal = "the picked edges have no length"; return Fit; }
    Fit.Span = Span;
    const double Wanted = 0.15 * Span;                                                      // [len] - About 15 % of the bound
    const double Decade = std::pow(10.0, std::floor(std::log10(Wanted)));
    const double Steps[] = { 5.0, 2.0, 1.0 };
    Fit.Fitted = Decade;
    for (double Step : Steps) if (Step * Decade <= Wanted * (1.0 + 1.0e-9)) { Fit.Fitted = Step * Decade; break; }
    return Fit;
}

SolidArcCommandOutcome ChangeSolidArcSelection(SolidArcDocument& Document, FaceOperationKind Operation, double Amount, std::optional<Vector3> Direction)
{
    const std::string Stem  = SolidArcFaceOperationLabel(Operation);
    const std::string Label = Stem + " " + FormatAmount(Amount) + " on " + std::to_string(Document.Selection.Faces.size()) + " face(s)";
    return RunCommand(Document, Label, true, [&]() -> std::string
    {
        const bool WholeBodyCondition = Operation == FaceOperationKind::OffsetBody || Operation == FaceOperationKind::Heal;
        if (Document.Selection.Faces.empty() && !WholeBodyCondition) return "select one or more faces first";
        if (!Document.Summary.PresentCondition) return "there is no body to change";
        std::string Owner;
        if (const std::string Problem = CommonBody(Document, SolidArcSelectionKind::Face, Document.Selection.Faces, Owner); !Problem.empty()) return Problem;
        if (Owner.empty() && !Document.Selection.Bodies.empty()) Owner = Document.Selection.Bodies.front();   // Whole-body verbs on a picked body
        const std::string Caption = NextCaption(Document, Stem);
        const std::string StepLabel = "Change#" + std::to_string(NextStepOrdinal(Document.Tree));
        const double      Span = std::max(1.0, 4.0 * std::abs(Amount));
        Document.Tree.Parameters.push_back({ Caption + ".Amount", Amount, std::min(-Span, Amount), std::max(Span, Amount),
                                             Operation == FaceOperationKind::Draft ? "draft angle [rad]" : Operation == FaceOperationKind::Rotate ? "angle [rad]"
                                             : Operation == FaceOperationKind::Scale ? "factor" : "distance" });
        FeatureStep Step;
        Step.Kind      = FeatureStepKind::FaceOperation;
        Step.Label     = StepLabel;
        Step.Operation = Operation;
        Step.Amount    = ParameterValue(Caption + ".Amount");
        Step.Target    = Owner;
        if (Direction && (Operation == FaceOperationKind::Tweak || Operation == FaceOperationKind::Rotate))
        {
            if (!(Length(*Direction) > 1.0e-12)) return "the direction is zero";
            Step.Points = { Normalize(*Direction) };
        }
        for (const std::string& Name : Document.Selection.Faces) Step.References.push_back({ ReferenceKind::Face, Name, {} });
        Document.Tree.Steps.push_back(std::move(Step));
        Document.Captions[StepLabel] = Caption;
        return {};
    });
}

SolidArcCommandOutcome InsetSolidArcSelection(SolidArcDocument& Document, double Distance, double Depth)
{
    const std::string Label = "Inset " + FormatAmount(Distance) + (Depth != 0.0 ? " depth " + FormatAmount(Depth) : std::string()) + " on " +
                              std::to_string(Document.Selection.Faces.size()) + " face(s)";
    return RunCommand(Document, Label, true, [&]() -> std::string
    {
        if (Document.Selection.Faces.empty()) return "select one or more planar faces first";
        if (!Document.Summary.PresentCondition) return "there is no body to change";
        std::string Owner;
        if (const std::string Problem = CommonBody(Document, SolidArcSelectionKind::Face, Document.Selection.Faces, Owner); !Problem.empty()) return Problem;
        const std::string Caption   = NextCaption(Document, "Inset");
        const std::string StepLabel = "Change#" + std::to_string(NextStepOrdinal(Document.Tree));
        const double      Span      = std::max(1.0, 4.0 * std::max(std::abs(Distance), std::abs(Depth)));
        Document.Tree.Parameters.push_back({ Caption + ".Amount", Distance, 0.0, std::max(Span, Distance), "inset distance" });
        Document.Tree.Parameters.push_back({ Caption + ".Depth", Depth, std::min(-Span, Depth), std::max(Span, Depth), "depth (− pocket)" });
        FeatureStep Step;
        Step.Kind      = FeatureStepKind::FaceOperation;
        Step.Label     = StepLabel;
        Step.Operation = FaceOperationKind::Inset;
        Step.Amount    = ParameterValue(Caption + ".Amount");
        Step.Extent    = ParameterValue(Caption + ".Depth");
        Step.Target    = Owner;
        for (const std::string& Name : Document.Selection.Faces) Step.References.push_back({ ReferenceKind::Face, Name, {} });
        Document.Tree.Steps.push_back(std::move(Step));
        Document.Captions[StepLabel] = Caption;
        return {};
    });
}

SolidArcCommandOutcome AssignSolidArcParameter(SolidArcDocument& Document, const std::string& Name, double Value)
{
    return RunCommand(Document, Name + " = " + FormatAmount(Value), false, [&]() -> std::string
    {
        if (!AssignFeatureParameter(Document.Tree, Name, Value)) return "no parameter named " + Name;
        return {};
    });
}

SolidArcCommandOutcome DeleteSolidArcStep(SolidArcDocument& Document, const std::string& Label)
{
    // 📝 A construct tile is one sketch + one sweep: deleting either removes both (and the tile's parameters).
    const auto Found = Document.Captions.find(Label);
    const std::string Caption = Found != Document.Captions.end() ? Found->second : Label;
    return RunCommand(Document, "Delete " + Caption, false, [&]() -> std::string
    {
        std::set<std::string> Doomed;
        for (const FeatureStep& Step : Document.Tree.Steps)
        {
            const auto Entry = Document.Captions.find(Step.Label);
            if (Step.Label == Label || (Entry != Document.Captions.end() && Entry->second == Caption)) Doomed.insert(Step.Label);
        }
        if (Doomed.empty()) return "no step labelled " + Label;
        for (const FeatureStep& Step : Document.Tree.Steps) if (!Step.Profile.empty() && Doomed.contains(Step.Profile)) Doomed.insert(Step.Label);   // Sweeps of a deleted sketch go too
        std::erase_if(Document.Tree.Steps, [&](const FeatureStep& Step) { return Doomed.contains(Step.Label); });
        std::erase_if(Document.Tree.Parameters, [&](const FeatureParameter& Parameter) { return Parameter.Name.rfind(Caption + ".", 0) == 0; });
        for (const std::string& DoomedLabel : Doomed) Document.Captions.erase(DoomedLabel);
        return {};
    });
}

SolidArcCommandOutcome ReplaceSolidArcTree(SolidArcDocument& Document, FeatureTree Tree, std::map<std::string, std::string> Captions,
                                           const std::string& Label)
{
    return RunCommand(Document, Label, false, [&]() -> std::string
    {
        Document.Tree     = std::move(Tree);
        Document.Captions = std::move(Captions);
        Document.Selection = {};
        return {};
    });
}

std::string ComposeSolidArcSketch(const SolidArcDrawingRequest& Request, SketchModel& Sketch)
{
    Sketch = {};
    const std::vector<Vector2>& Points = Request.Points;
    const char*  Tool    = SolidArcDrawingLabel(Request.Kind);
    double       Extent  = 0.0;                                                                // [len] - Click spread
    for (const Vector2& First : Points) for (const Vector2& Second : Points) Extent = std::max(Extent, Distance(First, Second));
    const double Join    = 1.0e-9 * std::max(Extent, 1.0);                                     // [len] - Coincidence distance
    auto Needs = [&](size_t Count) -> std::string
    {
        return Points.size() < Count ? std::string(Tool) + " needs " + std::to_string(Count) + " points" : std::string{};
    };
    switch (Request.Kind)
    {
    case SolidArcDrawingKind::Line:
    {
        if (std::string Refusal = Needs(Request.ClosedCondition ? 3 : 2); !Refusal.empty()) return Refusal;
        std::vector<int> Indices;
        for (const Vector2& Point : Points) Indices.push_back(AppendSketchPoint(Sketch, Point));
        const size_t Count = Request.ClosedCondition ? Indices.size() : Indices.size() - 1;
        for (size_t Index = 0; Index < Count; ++Index)
        {
            const int Start = Indices[Index], End = Indices[(Index + 1) % Indices.size()];
            if (Distance(Points[Index], Points[(Index + 1) % Points.size()]) <= Join) return "Line points " + std::to_string(Index + 1) + " and " + std::to_string((Index + 1) % Points.size() + 1) + " coincide";
            AppendSketchLine(Sketch, Start, End, static_cast<int>(Index) + 1);
        }
        return {};
    }
    case SolidArcDrawingKind::Rectangle:
    {
        if (std::string Refusal = Needs(2); !Refusal.empty()) return Refusal;
        const Vector2 First  = Request.CentreCondition ? Points[0] * 2.0 - Points[1] : Points[0];
        const Vector2 Second = Points[1];
        const Vector2 Low{ std::min(First.X, Second.X), std::min(First.Y, Second.Y) }, High{ std::max(First.X, Second.X), std::max(First.Y, Second.Y) };
        if (High.X - Low.X <= Join || High.Y - Low.Y <= Join) return "Rectangle has no width or no height";
        AppendSketchRectangle(Sketch, Low, High, 1);
        return {};
    }
    case SolidArcDrawingKind::Circle:
    {
        if (std::string Refusal = Needs(2); !Refusal.empty()) return Refusal;
        const Vector2 Centre = Request.CentreCondition ? Points[0] : (Points[0] + Points[1]) * 0.5;
        const double  Radius = Request.CentreCondition ? Distance(Points[0], Points[1]) : 0.5 * Distance(Points[0], Points[1]);
        if (Radius <= Join) return "Circle has no radius";
        AppendSketchCircle(Sketch, AppendSketchPoint(Sketch, Centre), Radius, 1);
        return {};
    }
    case SolidArcDrawingKind::Arc:
    {
        // Circle through start, end and the third click; counter-clockwise from start to end when start → third → end
        // turns left (orientation > 0), else stored from end to start (the sketch arc is always counter-clockwise).
        if (std::string Refusal = Needs(3); !Refusal.empty()) return Refusal;
        const Vector2 Start = Points[0], End = Points[1], Through = Points[2];
        const double  Turn  = (Through.X - Start.X) * (End.Y - Start.Y) - (Through.Y - Start.Y) * (End.X - Start.X);   // [len²] - orient(Start, Through, End)
        if (std::abs(Turn) <= Join * std::max(Extent, 1.0)) return "Arc points are collinear";
        const double  Scale = 2.0 * (Start.X * (Through.Y - End.Y) + Through.X * (End.Y - Start.Y) + End.X * (Start.Y - Through.Y));
        const double  StartSquare = Start.X * Start.X + Start.Y * Start.Y, ThroughSquare = Through.X * Through.X + Through.Y * Through.Y, EndSquare = End.X * End.X + End.Y * End.Y;
        const Vector2 Centre{ (StartSquare * (Through.Y - End.Y) + ThroughSquare * (End.Y - Start.Y) + EndSquare * (Start.Y - Through.Y)) / Scale,
                              (StartSquare * (End.X - Through.X) + ThroughSquare * (Start.X - End.X) + EndSquare * (Through.X - Start.X)) / Scale };
        const int CentreIndex = AppendSketchPoint(Sketch, Centre);
        const int StartIndex  = AppendSketchPoint(Sketch, Start), EndIndex = AppendSketchPoint(Sketch, End);
        if (Turn > 0.0) AppendSketchArc(Sketch, CentreIndex, StartIndex, EndIndex, 1);
        else AppendSketchArc(Sketch, CentreIndex, EndIndex, StartIndex, 1);
        return {};
    }
    case SolidArcDrawingKind::Slot:
    {
        // Stadium: caps centred on the first two clicks, half-width = distance of the third click from their line.
        if (std::string Refusal = Needs(3); !Refusal.empty()) return Refusal;
        const Vector2 First = Points[0], Second = Points[1];
        const double  Length = Distance(First, Second);
        if (Length <= Join) return "Slot cap centres coincide";
        const Vector2 Along  = (Second - First) * (1.0 / Length);
        const Vector2 Left{ -Along.Y, Along.X };
        const double  Radius = std::abs((Points[2].X - First.X) * Left.X + (Points[2].Y - First.Y) * Left.Y);
        if (Radius <= Join) return "Slot has no width";
        const int FirstCentre = AppendSketchPoint(Sketch, First), SecondCentre = AppendSketchPoint(Sketch, Second);
        const int TopFirst    = AppendSketchPoint(Sketch, First + Left * Radius), TopSecond = AppendSketchPoint(Sketch, Second + Left * Radius);
        const int BottomFirst = AppendSketchPoint(Sketch, First - Left * Radius), BottomSecond = AppendSketchPoint(Sketch, Second - Left * Radius);
        const int Bottom = AppendSketchLine(Sketch, BottomFirst, BottomSecond, 1);
        const int Cap    = AppendSketchArc(Sketch, SecondCentre, BottomSecond, TopSecond, 2);
        const int Top    = AppendSketchLine(Sketch, TopSecond, TopFirst, 3);
        const int Back   = AppendSketchArc(Sketch, FirstCentre, TopFirst, BottomFirst, 4);
        AppendSketchConstraint(Sketch, Kind::Tangent, Bottom, Cap, -1, 0.0, "T1");
        AppendSketchConstraint(Sketch, Kind::Tangent, Top, Cap, -1, 0.0, "T2");
        AppendSketchConstraint(Sketch, Kind::Tangent, Top, Back, -1, 0.0, "T3");
        AppendSketchConstraint(Sketch, Kind::Tangent, Bottom, Back, -1, 0.0, "T4");
        return {};
    }
    case SolidArcDrawingKind::Polygon:
    {
        // Regular polygon: the first click is the centre, the second a vertex (size and turn); Sides closed lines.
        if (std::string Refusal = Needs(2); !Refusal.empty()) return Refusal;
        if (Request.Sides < 3 || Request.Sides > 64) return "Polygon needs 3 … 64 sides";
        const Vector2 Centre = Points[0];
        const double  Radius = Distance(Points[0], Points[1]);
        if (Radius <= Join) return "Polygon has no radius";
        const double  Turn = std::atan2(Points[1].Y - Centre.Y, Points[1].X - Centre.X);
        std::vector<int> Corners;
        for (int Index = 0; Index < Request.Sides; ++Index)
        {
            const double θ = Turn + 2.0 * Pi * Index / Request.Sides;
            Corners.push_back(AppendSketchPoint(Sketch, { Centre.X + Radius * std::cos(θ), Centre.Y + Radius * std::sin(θ) }));
        }
        for (int Index = 0; Index < Request.Sides; ++Index) AppendSketchLine(Sketch, Corners[Index], Corners[(Index + 1) % Request.Sides], Index + 1);
        return {};
    }
    case SolidArcDrawingKind::Curve:
    {
        if (std::string Refusal = Needs(Request.ClosedCondition ? 3 : 2); !Refusal.empty()) return Refusal;
        std::vector<int> Indices;
        for (const Vector2& Point : Points) Indices.push_back(AppendSketchPoint(Sketch, Point));
        if (Request.ClosedCondition) Indices.push_back(Indices.front());
        const int Curve = AppendSketchSpline(Sketch, Indices, Request.Form, 1);
        const SketchSplineOutcome Resolved = ResolveSketchCurveSpline(Sketch, Sketch.Curves[Curve]);
        return Resolved.Refusal;
    }
    }
    return "unknown drawing tool";
}

SolidArcCommandOutcome DrawSolidArcSketch(SolidArcDocument& Document, const SolidArcDrawingRequest& Request)
{
    const std::string Stem    = Request.Kind == SolidArcDrawingKind::Curve ? std::string("Curve") : SolidArcDrawingLabel(Request.Kind);
    const std::string Caption = NextCaption(Document, Stem);
    const char*       FormTag = Request.Kind == SolidArcDrawingKind::Curve ? SketchSplineFormLabel(Request.Form) : nullptr;
    return RunCommand(Document, Caption + (FormTag ? std::string(" · ") + FormTag : std::string{}), false, [&]() -> std::string
    {
        if (Request.GridPlane < 0 || Request.GridPlane > 2) return "construction plane must be XY, XZ or YZ";
        SketchModel Sketch;
        if (std::string Refusal = ComposeSolidArcSketch(Request, Sketch); !Refusal.empty()) return Refusal;
        FeatureStep Step;
        Step.Kind   = FeatureStepKind::Sketch;
        Step.Label  = "Sketch#" + std::to_string(NextStepOrdinal(Document.Tree));
        Step.Plane  = SolidArcSketchPlane(Request.GridPlane);
        if (Request.PlaneCondition)
        {
            // Drawn on a body face: the face plane (orthonormal, finite) carries the sketch.
            const OrthonormalFrame& Face = Request.Plane;
            const bool Finite = std::isfinite(Face.Origin.X) && std::isfinite(Face.Origin.Y) && std::isfinite(Face.Origin.Z);
            const bool Square = std::abs(Length(Face.XAxis) - 1.0) < 1.0e-9 && std::abs(Length(Face.YAxis) - 1.0) < 1.0e-9 && std::abs(Length(Face.ZAxis) - 1.0) < 1.0e-9
                             && std::abs(Dot(Face.XAxis, Face.YAxis)) < 1.0e-9 && Length(Cross(Face.XAxis, Face.YAxis) - Face.ZAxis) < 1.0e-9;
            if (!Finite || !Square) return "the face plane is not an orthonormal right-handed frame";
            Step.Plane = Face;
        }
        Step.Sketch        = std::move(Sketch);
        Step.FillCondition = Request.FillCondition;
        Document.Captions[Step.Label] = Caption;
        Document.Tree.Steps.push_back(std::move(Step));
        return {};
    });
}

SolidArcCommandOutcome FillSolidArcSketch(SolidArcDocument& Document, const std::string& Sketch, bool FillCondition)
{
    const auto Source = Document.Captions.find(Sketch);
    const std::string Caption = Source != Document.Captions.end() ? Source->second : Sketch;
    return RunCommand(Document, std::string(FillCondition ? "Fill " : "Unfill ") + Caption, false, [&]() -> std::string
    {
        const auto Found = std::find_if(Document.Tree.Steps.begin(), Document.Tree.Steps.end(), [&](const FeatureStep& Step) { return Step.Label == Sketch; });
        if (Found == Document.Tree.Steps.end() || Found->Kind != FeatureStepKind::Sketch) return "no sketch step labelled " + Sketch;
        if (Found->FillCondition == FillCondition) return Caption + (FillCondition ? " is already filled" : " is already unfilled");
        if (FillCondition)
        {
            const auto Solved = Document.Regeneration.Sketches.find(Sketch);
            if (Solved == Document.Regeneration.Sketches.end()) return Caption + " did not solve";
            const SketchProfile Profile = ExtractSketchProfile(Solved->second);
            if (!Profile.Refusal.empty()) return Caption + " cannot be filled: " + Profile.Refusal;
        }
        Found->FillCondition = FillCondition;
        return {};
    });
}

SolidArcCommandOutcome FlipSolidArcBodies(SolidArcDocument& Document, const std::vector<std::string>& Keys)
{
    std::vector<std::string> Unique;
    for (const std::string& Key : Keys) if (std::find(Unique.begin(), Unique.end(), Key) == Unique.end()) Unique.push_back(Key);
    std::string Operands;
    for (const std::string& Key : Unique) Operands += (Operands.empty() ? "" : ", ") + SolidArcBodyCaption(Document, Key);
    return RunCommand(Document, "Flip normals (" + Operands + ")", false, [&]() -> std::string
    {
        if (Unique.empty()) return "pick the sheets to flip (mode 4)";
        for (const std::string& Key : Unique)
        {
            const RegeneratedBody* Body = FindRegeneratedBody(Document, Key);
            if (!Body) return "no body named " + Key;
            if (Document.Partition && Document.Partition->Bodies.Resolve(Body->Body).Category != BodyCategory::Sheet)
                return SolidArcBodyCaption(Document, Key) + " is a solid: only sheets (lofts, walls) turn over";
        }
        for (const std::string& Key : Unique)
        {
            // ① a Flip that is the body's last step is undone instead of stacking a second one
            auto Last = Document.Tree.Steps.end();
            for (auto Step = Document.Tree.Steps.begin(); Step != Document.Tree.Steps.end(); ++Step)
                if (Step->Target == Key || Step->Label == Key) Last = Step;
            if (Last != Document.Tree.Steps.end() && Last->Kind == FeatureStepKind::Flip && Last->Target == Key)
            {
                Document.Captions.erase(Last->Label);
                Document.Tree.Steps.erase(Last);
                continue;
            }
            // ② else a new Flip step
            FeatureStep Step;
            Step.Kind   = FeatureStepKind::Flip;
            Step.Label  = "Flip#" + std::to_string(NextStepOrdinal(Document.Tree));
            Step.Target = Key;
            Document.Captions[Step.Label] = NextCaption(Document, "Flip");
            Document.Tree.Steps.push_back(std::move(Step));
        }
        return {};
    });
}

SolidArcCommandOutcome ExtrudeSolidArcSketch(SolidArcDocument& Document, const std::string& SketchLabel, double Distance, FeatureCombination Combination,
                                             const std::string& Target, double Start, bool FillCondition)
{
    const bool        FirstBodyCondition = !Document.Summary.PresentCondition && Combination != FeatureCombination::Separate;
    const std::string Caption = NextCaption(Document, "Extrude");
    const auto        Source  = Document.Captions.find(SketchLabel);
    const std::string Label   = Caption + " · " + (Source != Document.Captions.end() ? Source->second : SketchLabel) + " · " +
                                (FirstBodyCondition ? "New" : FeatureCombinationLabel(Combination));
    return RunCommand(Document, Label, false, [&]() -> std::string
    {
        const auto Found = std::find_if(Document.Tree.Steps.begin(), Document.Tree.Steps.end(), [&](const FeatureStep& Step) { return Step.Label == SketchLabel; });
        if (Found == Document.Tree.Steps.end() || Found->Kind != FeatureStepKind::Sketch) return "no sketch step labelled " + SketchLabel;
        if (!(std::abs(Distance) > 1.0e-9)) return "extrude distance is zero";
        if (FirstBodyCondition && (Combination == FeatureCombination::Difference || Combination == FeatureCombination::Intersection))
            return std::string("nothing to ") + (Combination == FeatureCombination::Difference ? "subtract from" : "intersect with") + " yet (no body)";
        if (!FirstBodyCondition && Combination == FeatureCombination::New) return "a second body needs Union, Difference, Intersection or New body";
        if (!Target.empty() && !FindRegeneratedBody(Document, Target)) return "no body named " + Target;
        if (Start * Distance > 0.0) return "an extrude start lies on or behind the sketch plane (opposite the distance)";
        // A cut of zero-thickness walls means nothing: the region it cuts is filled first (as the knife does).
        if (FillCondition && !FirstBodyCondition && (Combination == FeatureCombination::Difference || Combination == FeatureCombination::Intersection)) Found->FillCondition = true;
        FeatureStep Step;
        Step.Kind        = FeatureStepKind::Extrude;
        if (Start != 0.0) Step.Start = FeatureValue{ Start };
        Step.Label       = "Extrude#" + std::to_string(NextStepOrdinal(Document.Tree));
        Step.Profile     = SketchLabel;
        Step.Extent      = FeatureValue{ Distance };
        Step.Combination = FirstBodyCondition ? FeatureCombination::New : Combination;
        Step.Target      = Target;
        Document.Captions[Step.Label] = Caption;
        Document.Tree.Steps.push_back(std::move(Step));
        return {};
    });
}

OrthonormalFrame SolidArcSketchPlane(int GridPlane) noexcept
{
    OrthonormalFrame Plane;
    if (GridPlane == 1) return UprightPlane();
    if (GridPlane == 2)
    {
        Plane.XAxis = { 0.0, 1.0, 0.0 };
        Plane.YAxis = { 0.0, 0.0, 1.0 };
        Plane.ZAxis = { 1.0, 0.0, 0.0 };
    }
    return Plane;
}

bool SolidArcSketchConsumedCondition(const SolidArcDocument& Document, const std::string& SketchLabel)
{
    return std::any_of(Document.Tree.Steps.begin(), Document.Tree.Steps.end(), [&](const FeatureStep& Step) { return Step.Profile == SketchLabel; });
}

bool SolidArcSketchReferencedCondition(const SolidArcDocument& Document, const std::string& SketchLabel)
{
    return std::any_of(Document.Tree.Steps.begin(), Document.Tree.Steps.end(), [&](const FeatureStep& Step)
    {
        return Step.Label != SketchLabel && (Step.Profile == SketchLabel || Step.Path == SketchLabel ||
               std::find(Step.Sections.begin(), Step.Sections.end(), SketchLabel) != Step.Sections.end());
    });
}

SolidArcCommandOutcome ImportSolidArcCurves(SolidArcDocument& Document, const VehicleCurveFile& File, double Tolerance)
{
    const std::string Label = "Import " + std::to_string(File.Curves.size()) + " curve(s)";
    SolidArcCommandOutcome Outcome = RunCommand(Document, Label, false, [&]() -> std::string
    {
        if (!File.Refusal.empty()) return "import: " + File.Refusal;
        if (File.Curves.empty()) return "import: the file holds no curves";
        if (!(Tolerance > 0.0)) return "import: the fit tolerance must be positive";
        for (const VehicleCurve& Curve : File.Curves)
        {
            FeatureStep Step;
            Step.Kind            = FeatureStepKind::SpaceCurve;
            Step.Label           = "Curve#" + std::to_string(NextStepOrdinal(Document.Tree));
            Step.Points          = Curve.Points;
            Step.ClosedCondition = Curve.ClosedCondition;
            Step.PlanarCondition = Curve.PlanarCondition;
            Step.Tolerance       = FeatureValue{ Tolerance };
            const bool Taken = std::any_of(Document.Captions.begin(), Document.Captions.end(), [&](const auto& Entry) { return Entry.second == Curve.Name; });
            Document.Captions[Step.Label] = Taken ? NextCaption(Document, Curve.Name + ".") : Curve.Name;
            Document.Tree.Steps.push_back(std::move(Step));
        }
        return {};
    });
    // 📝 Image planes ride along with an accepted import (replaced by name); they are references, not history.
    if (Outcome.AcceptedCondition)
    {
        for (const SolidArcImagePlane& Plane : File.ImagePlanes)
        {
            const auto Same = std::find_if(Document.ImagePlanes.begin(), Document.ImagePlanes.end(), [&](const SolidArcImagePlane& Held) { return Held.Name == Plane.Name; });
            if (Same != Document.ImagePlanes.end()) *Same = Plane;
            else Document.ImagePlanes.push_back(Plane);
        }
    }
    return Outcome;
}

SolidArcCommandOutcome ImportSolidArcStep(SolidArcDocument& Document, const SolidArcStepReading& Reading)
{
    const std::string Label = "Import " + Reading.Stem + " · " + std::to_string(Reading.Texts.size()) + " solid(s)";
    return RunCommand(Document, Label, false, [&]() -> std::string
    {
        if (!Reading.Refusal.empty()) return Reading.Refusal;
        if (Reading.Texts.empty()) return "Import " + Reading.Stem + ": the file holds no solid";
        for (size_t Index = 0; Index < Reading.Texts.size(); ++Index)
        {
            FeatureStep Step;
            Step.Kind         = FeatureStepKind::Import;
            Step.Label        = "Import#" + std::to_string(NextStepOrdinal(Document.Tree));
            Step.Exchange     = Reading.Texts[Index];
            Step.ExchangeUnit = SolidArcModelUnitMillimetres;
            const std::string Wanted = Reading.Texts.size() == 1 ? Reading.Stem : Reading.Stem + "." + std::to_string(Index + 1);
            const bool Taken = std::any_of(Document.Captions.begin(), Document.Captions.end(), [&](const auto& Entry) { return Entry.second == Wanted; });
            Document.Captions[Step.Label] = Taken ? NextCaption(Document, Wanted + ".") : Wanted;
            Document.Tree.Steps.push_back(std::move(Step));
        }
        return {};
    });
}

SolidArcCommandOutcome LoftSolidArcCurves(SolidArcDocument& Document, const std::vector<std::string>& Labels, int Continuity, bool SolidCondition)
{
    // Curves that meet each other make a network; otherwise they are sections.
    std::vector<NurbsCurve> Fitted;
    for (const std::string& Name : Labels)
    {
        const auto Found = Document.Regeneration.Curves.find(Name);
        if (Found != Document.Regeneration.Curves.end()) Fitted.push_back(Found->second.Curve);
    }
    const bool        NetworkCondition = Fitted.size() == Labels.size() && Fitted.size() >= 4 && DetectCurveNetwork(Fitted, NetworkSettings{});
    const std::string Caption = NextCaption(Document, NetworkCondition ? "Network" : "Loft");
    const std::string Label   = Caption + " · " + std::to_string(Labels.size()) + (NetworkCondition ? " curves" : " sections")
                              + (Continuity > 0 ? " · G" + std::to_string(Continuity) : std::string()) + (SolidCondition && !NetworkCondition ? " · solid" : "");
    SolidArcCommandOutcome Outcome = RunCommand(Document, Label, false, [&]() -> std::string
    {
        if (Labels.size() < 2) return "loft needs at least 2 curves (select them in section order)";
        for (size_t Index = 0; Index < Labels.size(); ++Index)
        {
            const auto Found = std::find_if(Document.Tree.Steps.begin(), Document.Tree.Steps.end(), [&](const FeatureStep& Step) { return Step.Label == Labels[Index]; });
            if (Found == Document.Tree.Steps.end() || (Found->Kind != FeatureStepKind::SpaceCurve && Found->Kind != FeatureStepKind::Sketch)) return "no curve or sketch step labelled " + Labels[Index];
            if (std::find(Labels.begin(), Labels.begin() + Index, Labels[Index]) != Labels.begin() + Index) return "curve " + Labels[Index] + " is selected twice";
        }
        if (Continuity < 0 || Continuity > 2) return "continuity must be G0, G1 or G2";
        if (NetworkCondition && Continuity > 0) return "these curves cross (a network): G1 / G2 is available for section lofts only (🚧 networks)";
        if (NetworkCondition && SolidCondition) return "these curves cross (a network): a solid loft needs closed sections in order";
        if (SolidCondition && Continuity > 0) return "a solid loft is G0 (G1 / G2 join a loft to neighbouring panels)";
        FeatureStep Step;
        Step.Kind       = NetworkCondition ? FeatureStepKind::Network : FeatureStepKind::Loft;
        Step.Label      = (NetworkCondition ? "Network#" : "Loft#") + std::to_string(NextStepOrdinal(Document.Tree));
        Step.Sections   = Labels;
        Step.Continuity = Continuity;
        Step.ClosedCondition = SolidCondition;
        Document.Captions[Step.Label] = Caption;
        Document.Tree.Steps.push_back(std::move(Step));
        return {};
    });
    if (Outcome.AcceptedCondition) Document.Selection.Curves.clear();
    return Outcome;
}

SolidArcCommandOutcome MirrorSolidArcPanels(SolidArcDocument& Document, int Axis)
{
    const char*       AxisName = Axis == 0 ? "x" : Axis == 1 ? "y" : "z";
    const std::string Caption  = NextCaption(Document, "Mirror");
    return RunCommand(Document, Caption + " · plane " + AxisName + " = 0", false, [&]() -> std::string
    {
        if (Axis < 0 || Axis > 2) return "mirror axis must be x, y or z";
        FeatureStep Step;
        Step.Kind         = FeatureStepKind::Mirror;
        Step.Label        = "Mirror#" + std::to_string(NextStepOrdinal(Document.Tree));
        Step.Plane.Origin = { 0.0, 0.0, 0.0 };
        Step.Plane.ZAxis  = { Axis == 0 ? 1.0 : 0.0, Axis == 1 ? 1.0 : 0.0, Axis == 2 ? 1.0 : 0.0 };
        Step.Plane.XAxis  = { Axis == 0 ? 0.0 : 1.0, Axis == 0 ? 1.0 : 0.0, 0.0 };
        Step.Plane.YAxis  = Cross(Step.Plane.ZAxis, Step.Plane.XAxis);
        Document.Captions[Step.Label] = Caption;
        Document.Tree.Steps.push_back(std::move(Step));
        return {};
    });
}

SolidArcCommandOutcome CloseSolidArcPanels(SolidArcDocument& Document)
{
    const std::string Caption = NextCaption(Document, "Close");
    return RunCommand(Document, Caption + " · plane caps", false, [&]() -> std::string
    {
        FeatureStep Step;
        Step.Kind  = FeatureStepKind::Close;
        Step.Label = "Close#" + std::to_string(NextStepOrdinal(Document.Tree));
        Document.Captions[Step.Label] = Caption;
        Document.Tree.Steps.push_back(std::move(Step));
        return {};
    });
}

SolidArcCommandOutcome MoveSolidArcPole(SolidArcDocument& Document, const std::string& Label, int Pole, Vector3 Position)
{
    const auto        Caption = Document.Captions.find(Label);
    const std::string Command = "Move pole " + std::to_string(Pole) + " of " + (Caption != Document.Captions.end() ? Caption->second : Label);
    return RunCommand(Document, Command, false, [&]() -> std::string
    {
        const auto Found = std::find_if(Document.Tree.Steps.begin(), Document.Tree.Steps.end(), [&](const FeatureStep& Step) { return Step.Label == Label; });
        if (Found == Document.Tree.Steps.end() || Found->Kind != FeatureStepKind::SpaceCurve) return "no curve step labelled " + Label;
        const auto Fitted = Document.Regeneration.Curves.find(Label);
        if (Fitted == Document.Regeneration.Curves.end()) return "curve " + Label + " has no fitted poles (refused step)";
        if (Pole < 0 || Pole >= static_cast<int>(Fitted->second.Curve.Definition->Poles.size())) return "pole " + std::to_string(Pole) + " does not exist";
        if (!std::isfinite(Position.X) || !std::isfinite(Position.Y) || !std::isfinite(Position.Z)) return "pole position is not finite";
        const auto Edit = std::find_if(Found->PoleEdits.begin(), Found->PoleEdits.end(), [&](const FeaturePoleEdit& Entry) { return Entry.Index == Pole; });
        if (Edit != Found->PoleEdits.end()) Edit->Position = Position;
        else Found->PoleEdits.push_back({ Pole, Position });
        return {};
    });
}

void SelectSolidArcCurve(SolidArcDocument& Document, const std::string& Label, bool AdditiveCondition)
{
    std::vector<std::string>& Curves = Document.Selection.Curves;
    if (!AdditiveCondition)
    {
        Document.Selection = {};
        Document.Selection.Curves.push_back(Label);
        return;
    }
    const auto Found = std::find(Curves.begin(), Curves.end(), Label);
    if (Found != Curves.end()) Curves.erase(Found);
    else Curves.push_back(Label);
}

bool UndoSolidArcCommand(SolidArcDocument& Document)
{
    if (Document.UndoEntries.empty()) return false;
    FRONTIER_TRACE_SCOPE("History", "Undo · " + Document.UndoEntries.back().Label);
    SolidArcHistoryEntry Entry = std::move(Document.UndoEntries.back());
    Document.UndoEntries.pop_back();
    Document.RedoEntries.push_back(Snapshot(Document, Entry.Label));
    Document.RedoEntries.back().AppearanceCondition = Entry.AppearanceCondition;
    Restore(Document, std::move(Entry));
    TrimHistoryReplay(Document);
    return true;
}

bool RedoSolidArcCommand(SolidArcDocument& Document)
{
    if (Document.RedoEntries.empty()) return false;
    FRONTIER_TRACE_SCOPE("History", "Redo · " + Document.RedoEntries.back().Label);
    SolidArcHistoryEntry Entry = std::move(Document.RedoEntries.back());
    Document.RedoEntries.pop_back();
    Document.UndoEntries.push_back(Snapshot(Document, Entry.Label));
    Document.UndoEntries.back().AppearanceCondition = Entry.AppearanceCondition;
    Restore(Document, std::move(Entry));
    TrimHistoryReplay(Document);
    return true;
}

SolidArcCommandOutcome AssignSolidArcMatcap(SolidArcDocument& Document, const std::string& Matcap)
{
    const std::vector<std::string> Faces  = Document.Selection.Faces;
    const std::vector<std::string> Bodies = Faces.empty() ? Document.Selection.Bodies : std::vector<std::string>{};
    const std::string Label = "Matcap " + Matcap + " → " + (!Faces.empty() ? std::to_string(Faces.size()) + " face(s)"
                                                         : !Bodies.empty() ? std::to_string(Bodies.size()) + " body(ies)" : std::string("body"));
    return RunAppearanceCommand(Document, Label, [&]() -> std::string
    {
        SolidArcAppearance& Appearance = Document.Appearance;
        if (!SolidArcMatcapKnownCondition(Appearance, Matcap)) return "no matcap named " + Matcap;
        if (!Bodies.empty())
        {
            bool ChangedCondition = false;
            for (const std::string& Key : Bodies)
            {
                std::string& Assigned = Appearance.BodyMatcaps[Key];
                ChangedCondition = ChangedCondition || Assigned != Matcap;
                Assigned = Matcap;
            }
            return ChangedCondition ? std::string{} : "the selected bodies already show " + Matcap;
        }
        if (Faces.empty())
        {
            if (Appearance.BodyMatcap == Matcap) return "the body already shows " + Matcap;
            Appearance.BodyMatcap = Matcap;
            return {};
        }
        bool ChangedCondition = false;
        for (const std::string& Face : Faces)
        {
            std::string& Assigned = Appearance.FaceMatcaps[Face];
            ChangedCondition = ChangedCondition || Assigned != Matcap;
            Assigned = Matcap;
        }
        return ChangedCondition ? std::string{} : "the selected faces already show " + Matcap;
    });
}

SolidArcCommandOutcome RemoveSolidArcMatcap(SolidArcDocument& Document)
{
    const std::vector<std::string> Faces  = Document.Selection.Faces;
    const std::vector<std::string> Bodies = Faces.empty() ? Document.Selection.Bodies : std::vector<std::string>{};
    const std::string Label = !Faces.empty()  ? "Matcap removed → " + std::to_string(Faces.size()) + " face(s) show the body"
                            : !Bodies.empty() ? "Matcap removed → " + std::to_string(Bodies.size()) + " body(ies) show the default"
                                              : std::string("Matcap removed → body Studio");
    return RunAppearanceCommand(Document, Label, [&]() -> std::string
    {
        SolidArcAppearance& Appearance = Document.Appearance;
        if (!Bodies.empty())
        {
            size_t Removed = 0;
            for (const std::string& Key : Bodies) Removed += Appearance.BodyMatcaps.erase(Key);
            return Removed ? std::string{} : "the selected bodies have no matcap of their own";
        }
        if (Faces.empty())
        {
            if (Appearance.BodyMatcap == SolidArcStudioMatcap) return "the body already shows Studio";
            Appearance.BodyMatcap = SolidArcStudioMatcap;
            return {};
        }
        size_t Removed = 0;
        for (const std::string& Face : Faces) Removed += Appearance.FaceMatcaps.erase(Face);
        return Removed ? std::string{} : "the selected faces have no matcap of their own";
    });
}

SolidArcCommandOutcome AddSolidArcMatcapPicture(SolidArcDocument& Document, const std::string& Name, const std::string& Path)
{
    return RunAppearanceCommand(Document, "Matcap picture " + Name, [&]() -> std::string
    {
        SolidArcAppearance& Appearance = Document.Appearance;
        if (Name.empty() || std::any_of(Name.begin(), Name.end(), [](char Character) { return std::isspace(static_cast<unsigned char>(Character)) != 0; }))
            return "a matcap name needs at least one character and no spaces";
        if (SolidArcMatcapKnownCondition(Appearance, Name)) return "a matcap named " + Name + " exists already";
        if (Appearance.CustomMatcaps.size() + SolidArcBuiltInMatcaps().size() >= SolidArcMatcapLayerLimit) return "the matcap library is full";
        const SolidArcPicture Picture = DecodeSolidArcPicture(Path);
        if (!Picture.Refusal.empty()) return Picture.Refusal;
        Appearance.CustomMatcaps.push_back({ Name, Path });
        return {};
    });
}

void SelectSolidArcName(SolidArcDocument& Document, SolidArcSelectionKind Kind, const std::string& Name, bool AdditiveCondition, const std::string& Body)
{
    std::vector<std::string>& Names = Kind == SolidArcSelectionKind::Face ? Document.Selection.Faces
                                    : Kind == SolidArcSelectionKind::Edge ? Document.Selection.Edges : Document.Selection.Bodies;
    if (!AdditiveCondition)
    {
        Document.Selection = {};
        Names.push_back(Name);
        Document.Selection.LastKind = Kind;
        if (Kind != SolidArcSelectionKind::Body) Document.Selection.PickedBody = Body;
        return;
    }
    Document.Selection.LastKind = Kind;
    if (Kind != SolidArcSelectionKind::Body && !Body.empty()) Document.Selection.PickedBody = Body;
    const auto Found = std::find(Names.begin(), Names.end(), Name);
    if (Found != Names.end()) Names.erase(Found);
    else Names.push_back(Name);
}

std::vector<std::string> SolidArcTangentChain(const SolidArcDocument& Document, const std::string& Name)
{
    const RegeneratedBody* Owner = FindRegeneratedBody(Document, SolidArcBodyOfName(Document, SolidArcSelectionKind::Edge, Name));
    if (!Document.Partition || !Owner) return { Name };
    const GeometryProcessing::BodyNaming&    Naming    = Owner->Naming;
    const GeometryProcessing::BodyInventory  Inventory = Document.Partition->CollectInventory(Owner->Body);
    std::vector<GeometryProcessing::EdgeToken> Seeds;
    for (const GeometryProcessing::EdgeToken Edge : Inventory.Edges)
    {
        const auto Found = Naming.Edges.find(Edge.Slot);
        if (Found != Naming.Edges.end() && Found->second == Name) Seeds.push_back(Edge);
    }
    if (Seeds.empty()) return { Name };
    std::vector<std::string> Names;
    for (const GeometryProcessing::EdgeToken Edge : GeometryProcessing::GrowTangentChain(*Document.Partition, Owner->Body, Seeds))
    {
        const auto Found = Naming.Edges.find(Edge.Slot);
        if (Found != Naming.Edges.end()) Names.push_back(Found->second);
    }
    return Names;
}

namespace
{

constexpr int RadialLimit = 16;                                                              // [-] - Fins around one edge (non-manifold guard)
constexpr int CycleLimit  = 4096;                                                            // [-] - Fins round one loop (corrupt-cycle guard)

// One body's topology as the Blender walkers see it: edge names, fins round each edge, edge ends at each vertex.
struct SolidArcTopologyWalk
{
    const ModelPartition*                 Partition = nullptr;                               // [-] - Owning partition
    const GeometryProcessing::BodyNaming* Naming    = nullptr;                               // [-] - Persistent names
    std::vector<EdgeToken>                Edges     = {};                                    // [-] - Body edges
    std::map<uint32_t, int>               Valence   = {};                                    // [-] - Vertex slot → edge ends
    EdgeToken                             Start     = {};                                    // [-] - Edge the walk starts on
    bool                                  StartCondition = false;                            // [-] - The name resolved
};

SolidArcTopologyWalk PrepareTopologyWalk(const SolidArcDocument& Document, const std::string& Name, const std::string& Body)
{
    SolidArcTopologyWalk Walk;
    const RegeneratedBody* Owner = FindRegeneratedBody(Document, Body.empty() ? SolidArcBodyOfName(Document, SolidArcSelectionKind::Edge, Name) : Body);
    if (!Document.Partition || !Owner) return Walk;
    Walk.Partition = Document.Partition.get();
    Walk.Naming    = &Owner->Naming;
    Walk.Edges     = Walk.Partition->CollectInventory(Owner->Body).Edges;
    for (const EdgeToken Edge : Walk.Edges)
    {
        const EdgeProfile& Profile = Walk.Partition->Edges.Resolve(Edge);
        ++Walk.Valence[Profile.Start.Slot];
        ++Walk.Valence[Profile.End.Slot];
        const auto Found = Walk.Naming->Edges.find(Edge.Slot);
        if (!Walk.StartCondition && Found != Walk.Naming->Edges.end() && Found->second == Name) { Walk.Start = Edge; Walk.StartCondition = true; }
    }
    return Walk;
}

[[nodiscard]] int RadialCount(const ModelPartition& Partition, EdgeToken Edge)
{
    const FinToken First = Partition.Edges.Resolve(Edge).RadialFin;
    if (!First.PresentCondition()) return 0;
    int      Count = 0;
    FinToken Fin   = First;
    do
    {
        if (++Count > RadialLimit) return Count;
        Fin = Partition.Fins.Resolve(Fin).RadialSuccessor;
    } while (Fin.PresentCondition() && !(Fin == First));
    return Count;
}

// The other fin of a manifold edge (absent at a boundary or non-manifold edge).
[[nodiscard]] FinToken RadialMate(const ModelPartition& Partition, FinToken Fin)
{
    if (RadialCount(Partition, Partition.Fins.Resolve(Fin).Edge) != 2) return {};
    return Partition.Fins.Resolve(Fin).RadialSuccessor;
}

// The neighbouring fin of the same loop that shares Vertex with Fin (BM_loop_other_edge_loop).
[[nodiscard]] FinToken FanNeighbour(const ModelPartition& Partition, FinToken Fin, VertexToken Vertex)
{
    const FinProfile&  Use     = Partition.Fins.Resolve(Fin);
    const EdgeProfile& Edge    = Partition.Edges.Resolve(Use.Edge);
    const VertexToken  Arrival = Use.Sense == Orientation::Forward ? Edge.End : Edge.Start;
    return Arrival == Vertex ? Use.Successor : Use.Predecessor;
}

[[nodiscard]] int LoopFinCount(const ModelPartition& Partition, FinToken Fin)
{
    int      Count = 1;
    FinToken Cycle = Partition.Fins.Resolve(Fin).Successor;
    while (Cycle.PresentCondition() && !(Cycle == Fin) && Count <= CycleLimit) { ++Count; Cycle = Partition.Fins.Resolve(Cycle).Successor; }
    return Count;
}

[[nodiscard]] int FacesAtVertex(const ModelPartition& Partition, const std::vector<EdgeToken>& Edges, VertexToken Vertex)
{
    std::set<uint32_t> Faces;
    for (const EdgeToken Edge : Edges)
    {
        const EdgeProfile& Profile = Partition.Edges.Resolve(Edge);
        if (!(Profile.Start == Vertex) && !(Profile.End == Vertex)) continue;
        FinToken Fin = Profile.RadialFin;
        for (int Guard = 0; Fin.PresentCondition() && Guard < RadialLimit; ++Guard)
        {
            Faces.insert(Partition.Loops.Resolve(Partition.Fins.Resolve(Fin).Loop).Face.Slot);
            Fin = Partition.Fins.Resolve(Fin).RadialSuccessor;
            if (Fin == Profile.RadialFin) break;
        }
    }
    return int(Faces.size());
}

std::vector<std::string> NameWalkedEdges(const SolidArcTopologyWalk& Walk, const std::vector<EdgeToken>& Walked, const std::string& Name)
{
    std::vector<std::string> Names;
    for (const EdgeToken Edge : Walked)
    {
        const auto Found = Walk.Naming->Edges.find(Edge.Slot);
        if (Found != Walk.Naming->Edges.end() && std::find(Names.begin(), Names.end(), Found->second) == Names.end()) Names.push_back(Found->second);
    }
    return Names.empty() ? std::vector<std::string>{ Name } : Names;
}

// BMW_EDGERING: breadth-first across even faces (QuadOnly: four-sided only, the face-loop walker); Crossed collects the
// face slots stepped over.
std::vector<EdgeToken> WalkEdgeRing(const SolidArcTopologyWalk& Walk, bool QuadOnly, std::vector<FaceToken>* Crossed)
{
    const ModelPartition&  Partition = *Walk.Partition;
    std::vector<EdgeToken> Ring{ Walk.Start };
    std::set<uint32_t>     Chosen{ Walk.Start.Slot };
    std::set<uint32_t>     Faces;
    if (RadialCount(Partition, Walk.Start) > 2) return Ring;                                 // Non-manifold: that edge alone
    for (size_t Cursor = 0; Cursor < Ring.size(); ++Cursor)
    {
        const EdgeProfile& Profile = Partition.Edges.Resolve(Ring[Cursor]);
        if (!Profile.RadialFin.PresentCondition() || RadialCount(Partition, Ring[Cursor]) > 2) continue;
        FinToken Fin = Profile.RadialFin;
        for (int Guard = 0; Guard < RadialLimit; ++Guard)
        {
            const FinProfile&  Use  = Partition.Fins.Resolve(Fin);
            const LoopProfile& Loop = Partition.Loops.Resolve(Use.Loop);
            const int          Size = LoopFinCount(Partition, Fin);
            if (Partition.Faces.Resolve(Loop.Face).Loops.size() == 1 && Size >= 4 && Size % 2 == 0 && (!QuadOnly || Size == 4))
            {
                FinToken Across = Fin;
                for (int Step = 0; Step < Size / 2; ++Step) Across = Partition.Fins.Resolve(Across).Successor;
                const EdgeToken Opposite = Partition.Fins.Resolve(Across).Edge;
                if (Crossed && Faces.insert(Loop.Face.Slot).second) Crossed->push_back(Loop.Face);
                if (RadialCount(Partition, Opposite) <= 2 && Chosen.insert(Opposite.Slot).second) Ring.push_back(Opposite);
            }
            Fin = Use.RadialSuccessor;
            if (!Fin.PresentCondition() || Fin == Profile.RadialFin) break;
        }
    }
    return Ring;
}

} // namespace

std::vector<std::string> SolidArcEdgeRing(const SolidArcDocument& Document, const std::string& Name, const std::string& Body)
{
    const SolidArcTopologyWalk Walk = PrepareTopologyWalk(Document, Name, Body);
    if (!Walk.StartCondition) return { Name };
    return NameWalkedEdges(Walk, WalkEdgeRing(Walk, false, nullptr), Name);
}

std::vector<std::string> SolidArcFaceLoop(const SolidArcDocument& Document, const std::string& Edge, const std::string& Body)
{
    const SolidArcTopologyWalk Walk = PrepareTopologyWalk(Document, Edge, Body);
    if (!Walk.StartCondition) return {};
    const RegeneratedBody* Owner = FindRegeneratedBody(Document, Body.empty() ? SolidArcBodyOfName(Document, SolidArcSelectionKind::Edge, Edge) : Body);
    std::vector<FaceToken> Crossed;
    (void)WalkEdgeRing(Walk, true, &Crossed);
    std::vector<std::string> Names;
    for (const FaceToken Face : Crossed)
    {
        const auto Found = Owner->Naming.Faces.find(Face.Slot);
        if (Found != Owner->Naming.Faces.end() && std::find(Names.begin(), Names.end(), Found->second) == Names.end()) Names.push_back(Found->second);
    }
    return Names;
}

std::vector<std::string> SolidArcEdgeLoop(const SolidArcDocument& Document, const std::string& Name, const std::string& Body)
{
    const SolidArcTopologyWalk Walk = PrepareTopologyWalk(Document, Name, Body);
    if (!Walk.StartCondition) return { Name };
    const ModelPartition& Partition = *Walk.Partition;
    const auto ValenceOf = [&](VertexToken Vertex) { const auto Found = Walk.Valence.find(Vertex.Slot); return Found == Walk.Valence.end() ? 0 : Found->second; };
    const EdgeProfile& First = Partition.Edges.Resolve(Walk.Start);
    const int          Fins  = RadialCount(Partition, Walk.Start);
    if (Fins == 0 || Fins > 2 || First.Start == First.End) return { Name };                  // Wire, non-manifold or closed: itself
    const bool BoundaryCondition = Fins == 1;

    // f_hub: a three-edge, three-face end beside a face of more than four sides walks round that face.
    FaceToken Hub = {};
    if (!BoundaryCondition)
    {
        bool Pole = false;
        for (const VertexToken End : { First.Start, First.End })
            if (ValenceOf(End) == 3 && FacesAtVertex(Partition, Walk.Edges, End) == 3) Pole = true;
        if (Pole)
        {
            int      Largest = 0;
            FinToken Fin     = First.RadialFin;
            for (int Guard = 0; Guard < 2; ++Guard)
            {
                const int Size = LoopFinCount(Partition, Fin);
                if (Size > Largest) { Largest = Size; Hub = Partition.Loops.Resolve(Partition.Fins.Resolve(Fin).Loop).Face; }
                Fin = Partition.Fins.Resolve(Fin).RadialSuccessor;
            }
            if (Largest <= 4) Hub = {};
        }
    }

    // One step on from Edge through its end Vertex (absent when the walk stops there).
    const auto Advance = [&](EdgeToken Edge, VertexToken Vertex) -> EdgeToken
    {
        const EdgeProfile& Profile = Partition.Edges.Resolve(Edge);
        if (Profile.Start == Profile.End) return {};
        const int Valence = ValenceOf(Vertex);
        if (Hub.PresentCondition())
        {
            if (Valence != 3) return {};
            FinToken Fin = Profile.RadialFin;
            for (int Guard = 0; Guard < RadialLimit && Fin.PresentCondition(); ++Guard)
            {
                if (Partition.Loops.Resolve(Partition.Fins.Resolve(Fin).Loop).Face == Hub)
                {
                    const EdgeToken Next = Partition.Fins.Resolve(FanNeighbour(Partition, Fin, Vertex)).Edge;
                    return RadialCount(Partition, Next) == 2 ? Next : EdgeToken{};
                }
                Fin = Partition.Fins.Resolve(Fin).RadialSuccessor;
                if (Fin == Profile.RadialFin) break;
            }
            return {};
        }
        if (BoundaryCondition)
        {
            // Round the fan to the next boundary edge.
            FinToken Fin = Profile.RadialFin;
            for (int Guard = 0; Guard < 64; ++Guard)
            {
                Fin = FanNeighbour(Partition, Fin, Vertex);
                const EdgeToken Next  = Partition.Fins.Resolve(Fin).Edge;
                const int       Count = RadialCount(Partition, Next);
                if (Count == 1) return Next == Edge ? EdgeToken{} : Next;
                if (Count != 2) return {};
                Fin = Partition.Fins.Resolve(Fin).RadialSuccessor;
            }
            return {};
        }
        if (Valence != 4 && Valence != 2) return {};                                         // A pole ends the loop
        FinToken Fin = Profile.RadialFin;
        for (int Step = 0; Step < Valence / 2; ++Step)
        {
            Fin = FanNeighbour(Partition, Fin, Vertex);
            Fin = RadialMate(Partition, Fin);
            if (!Fin.PresentCondition()) return {};
        }
        const EdgeToken Next = Partition.Fins.Resolve(Fin).Edge;
        return Next == Edge ? EdgeToken{} : Next;
    };

    std::vector<EdgeToken> Loop{ Walk.Start };
    std::set<uint32_t>     Chosen{ Walk.Start.Slot };
    for (const VertexToken Toward : { First.End, First.Start })
    {
        EdgeToken   Edge   = Walk.Start;
        VertexToken Vertex = Toward;
        for (int Guard = 0; Guard < int(Walk.Edges.size()); ++Guard)
        {
            const EdgeToken Next = Advance(Edge, Vertex);
            if (!Next.PresentCondition() || !Chosen.insert(Next.Slot).second) break;
            Loop.push_back(Next);
            const EdgeProfile& Profile = Partition.Edges.Resolve(Next);
            Vertex = Profile.Start == Vertex ? Profile.End : Profile.Start;
            Edge   = Next;
        }
    }
    return NameWalkedEdges(Walk, Loop, Name);
}

SolidArcCommandOutcome CombineSolidArcBodies(SolidArcDocument& Document, const std::string& Target, const std::vector<std::string>& Tools, FeatureCombination Combination)
{
    std::string Operands = SolidArcBodyCaption(Document, Target);
    const char* Sign = Combination == FeatureCombination::Difference ? " − " : Combination == FeatureCombination::Intersection ? " ∩ " : " ∪ ";
    for (const std::string& Tool : Tools) Operands += Sign + SolidArcBodyCaption(Document, Tool);
    const std::string Label = std::string("Combine · ") + FeatureCombinationLabel(Combination) + " (" + Operands + ")";
    return RunCommand(Document, Label, false, [&]() -> std::string
    {
        if (Combination != FeatureCombination::Union && Combination != FeatureCombination::Difference && Combination != FeatureCombination::Intersection)
            return "choose Union, Difference or Intersection";
        if (Tools.empty()) return "select two or more bodies (the first is the target)";
        if (!FindRegeneratedBody(Document, Target)) return "no body named " + Target;
        std::set<std::string> Seen{ Target };
        for (const std::string& Tool : Tools)
        {
            if (!FindRegeneratedBody(Document, Tool)) return "no body named " + Tool;
            if (!Seen.insert(Tool).second) return SolidArcBodyCaption(Document, Tool) + " is picked twice (the target cannot be a tool)";
        }
        const std::string Caption = NextCaption(Document, "Combine");
        for (const std::string& Tool : Tools)
        {
            FeatureStep Step;
            Step.Kind        = FeatureStepKind::Combine;
            Step.Label       = "Combine#" + std::to_string(NextStepOrdinal(Document.Tree));
            Step.Combination = Combination;
            Step.Target      = Target;
            Step.Tool        = Tool;
            Document.Captions[Step.Label] = Caption;
            Document.Tree.Steps.push_back(std::move(Step));
        }
        Document.Selection = {};
        return {};
    });
}

SolidArcCommandOutcome CutSolidArcBody(SolidArcDocument& Document, const SolidArcCutRequest& Request)
{
    const std::string Target = Request.Target.empty() ? Document.Regeneration.Active : Request.Target;
    const std::string Label  = "Cut (" + SolidArcBodyCaption(Document, Target) + " by " + SolidArcBodyCaption(Document, Request.Cutter) + ")";
    return RunCommand(Document, Label, false, [&]() -> std::string
    {
        if (Target.empty()) return "pick the body to cut (mode 4), or build one first";
        if (!FindRegeneratedBody(Document, Target)) return "no body named " + Target;
        const FeatureStep* Cutter = nullptr;
        for (const FeatureStep& Step : Document.Tree.Steps) if (Step.Label == Request.Cutter) Cutter = &Step;
        if (!Cutter || (Cutter->Kind != FeatureStepKind::Sketch && Cutter->Kind != FeatureStepKind::SpaceCurve))
            return "choose the cutter: focus a sketch step or select a curve";
        FeatureStep Step;
        Step.Kind   = FeatureStepKind::Cut;
        Step.Label  = "Cut#" + std::to_string(NextStepOrdinal(Document.Tree));
        Step.Target = Target;
        if (Cutter->Kind == FeatureStepKind::Sketch) Step.Profile = Cutter->Label;
        else
        {
            Step.Sections    = { Cutter->Label };
            Step.Plane.ZAxis = Request.Direction;
            Step.Plane.XAxis = std::fabs(Request.Direction.X) < 0.9 ? Vector3{ 1.0, 0.0, 0.0 } : Vector3{ 0.0, 1.0, 0.0 };
        }
        Document.Captions[Step.Label] = NextCaption(Document, "Cut");
        Document.Tree.Steps.push_back(std::move(Step));
        Document.Selection = {};
        return {};
    });
}

SolidArcCommandOutcome RemoveSolidArcBodies(SolidArcDocument& Document, const std::vector<std::string>& Keys)
{
    std::string Operands;
    for (const std::string& Key : Keys) Operands += (Operands.empty() ? "" : ", ") + SolidArcBodyCaption(Document, Key);
    return RunCommand(Document, "Remove (" + Operands + ")", false, [&]() -> std::string
    {
        if (Keys.empty()) return "pick the bodies to remove (mode 4)";
        std::set<std::string> Seen;
        for (const std::string& Key : Keys)
        {
            if (!FindRegeneratedBody(Document, Key)) return "no body named " + Key;
            if (!Seen.insert(Key).second) continue;
            FeatureStep Step;
            Step.Kind   = FeatureStepKind::Remove;
            Step.Label  = "Remove#" + std::to_string(NextStepOrdinal(Document.Tree));
            Step.Target = Key;
            Document.Captions[Step.Label] = NextCaption(Document, "Remove");
            Document.Tree.Steps.push_back(std::move(Step));
        }
        Document.Selection = {};
        return {};
    });
}

namespace
{
    [[nodiscard]] OrthonormalFrame ElevatedPlane(const FeatureTree& Tree, const FeatureStep& Step)
    {
        OrthonormalFrame Plane = Step.Plane;
        Plane.Origin = Plane.Origin + Plane.ZAxis * EvaluateFeatureValue(Tree, Step.Elevation);
        return Plane;
    }

    [[nodiscard]] const SketchModel& SolvedSketch(const SolidArcDocument& Document, const FeatureStep& Step)
    {
        const auto Found = Document.Regeneration.Sketches.find(Step.Label);
        return Found != Document.Regeneration.Sketches.end() ? Found->second : Step.Sketch;
    }

    // 📝 A segment of a sketch on plane From, re-expressed in the coordinates of the coplanar plane To (rotation, offset,
    //    and a mirror when the normals oppose: arcs then run the other way). Exact for lines, arcs, circles and B-splines.
    [[nodiscard]] ProfileSegment MapSegment(ProfileSegment Segment, const OrthonormalFrame& From, const OrthonormalFrame& To)
    {
        auto Map = [&](Vector2 Point)
        {
            const Vector3 Offset = From.Origin + From.XAxis * Point.X + From.YAxis * Point.Y - To.Origin;
            return Vector2{ Dot(Offset, To.XAxis), Dot(Offset, To.YAxis) };
        };
        const double Turn = Dot(From.XAxis, To.XAxis) * Dot(From.YAxis, To.YAxis) - Dot(From.YAxis, To.XAxis) * Dot(From.XAxis, To.YAxis);
        Segment.Start = Map(Segment.Start);
        Segment.End   = Map(Segment.End);
        if (Segment.Kind == ProfileSegmentKind::Line) return Segment;
        if (Segment.Kind == ProfileSegmentKind::Spline)
        {
            for (Vector2& Pole : Segment.Spline.Poles) Pole = Map(Pole);
            return Segment;
        }
        const Vector2 Origin = Map(Segment.Centre);
        const Vector2 Rim    = Map(Segment.Centre + Vector2{ std::cos(Segment.StartAngle), std::sin(Segment.StartAngle) } * Segment.Radius);
        Segment.Centre     = Origin;
        Segment.StartAngle = std::atan2(Rim.Y - Origin.Y, Rim.X - Origin.X);
        if (Turn < 0.0) Segment.Sweep = -Segment.Sweep;
        return Segment;
    }
}

SketchTrimOutcome PreviewSolidArcTrim(const SolidArcDocument& Document, const SolidArcTrimRequest& Request)
{
    const auto Found = std::find_if(Document.Tree.Steps.begin(), Document.Tree.Steps.end(), [&](const FeatureStep& Step) { return Step.Label == Request.Sketch; });
    if (Found == Document.Tree.Steps.end() || Found->Kind != FeatureStepKind::Sketch)
    {
        SketchTrimOutcome Outcome;
        Outcome.Refusal = "no sketch step labelled " + Request.Sketch;
        return Outcome;
    }
    if (Request.InsertCondition) return InsertSketchCurvePoint(SolvedSketch(Document, *Found), Request.Curve, Request.Pick);
    if (Request.SplitCondition) return SplitSketchCurve(SolvedSketch(Document, *Found), Request.Curve, Request.Pick);
    const OrthonormalFrame      Plane = ElevatedPlane(Document.Tree, *Found);
    std::vector<ProfileSegment> Foreign;
    for (const FeatureStep& Step : Document.Tree.Steps)
    {
        if (Step.Kind != FeatureStepKind::Sketch || Step.Label == Request.Sketch || Document.Appearance.HiddenCurves.contains(Step.Label)) continue;
        const OrthonormalFrame Other = ElevatedPlane(Document.Tree, Step);
        const Vector3          Apart = Other.Origin - Plane.Origin;
        if (std::fabs(Dot(Other.ZAxis, Plane.ZAxis)) < 1.0 - 1.0e-9 || std::fabs(Dot(Apart, Plane.ZAxis)) > 1.0e-9 * std::max(1.0, Length(Apart))) continue;
        const SketchModel& Sketch = SolvedSketch(Document, Step);
        for (int Curve = 0; Curve < static_cast<int>(Sketch.Curves.size()); ++Curve)
        {
            std::string    Refusal;
            ProfileSegment Segment = ResolveSketchCurveSegment(Sketch, Curve, 1.0e-9, Refusal);
            if (Refusal.empty()) Foreign.push_back(MapSegment(std::move(Segment), Other, Plane));
        }
    }
    return TrimSketchCurve(SolvedSketch(Document, *Found), Request.Curve, Request.Pick, Foreign);
}

SolidArcRegionPreview PreviewSolidArcRegion(const SolidArcDocument& Document, const SolidArcRegionRequest& Request)
{
    SolidArcRegionPreview Preview;
    const auto Found = std::find_if(Document.Tree.Steps.begin(), Document.Tree.Steps.end(), [&](const FeatureStep& Step) { return Step.Label == Request.Sketch; });
    if (Found == Document.Tree.Steps.end() || Found->Kind != FeatureStepKind::Sketch) { Preview.Refusal = "no sketch step labelled " + Request.Sketch; return Preview; }
    Preview.Plane = ElevatedPlane(Document.Tree, *Found);
    // ① every drawn curve of this sketch and of the shown sketches in its plane, in its coordinates
    std::vector<ProfileSegment> Segments;
    std::map<std::string, int>  Drawn;                                                       // Sketch label → curves it gave
    double Scale = 1.0;
    for (const FeatureStep& Step : Document.Tree.Steps)
    {
        if (Step.Kind != FeatureStepKind::Sketch) continue;
        if (Step.Label != Request.Sketch && Document.Appearance.HiddenCurves.contains(Step.Label)) continue;
        const OrthonormalFrame Other = ElevatedPlane(Document.Tree, Step);
        const Vector3          Apart = Other.Origin - Preview.Plane.Origin;
        if (std::fabs(Dot(Other.ZAxis, Preview.Plane.ZAxis)) < 1.0 - 1.0e-9 || std::fabs(Dot(Apart, Preview.Plane.ZAxis)) > 1.0e-9 * std::max(1.0, Length(Apart))) continue;
        const SketchModel& Sketch = SolvedSketch(Document, Step);
        Scale = std::max(Scale, MeasureSketchScale(Sketch));
        for (int Curve = 0; Curve < static_cast<int>(Sketch.Curves.size()); ++Curve)
        {
            if (Sketch.Curves[Curve].Construction) continue;
            std::string    Refusal;
            ProfileSegment Segment = ResolveSketchCurveSegment(Sketch, Curve, 1.0e-9, Refusal);
            if (!Refusal.empty()) continue;
            Segments.push_back(Step.Label == Request.Sketch ? std::move(Segment) : MapSegment(std::move(Segment), Other, Preview.Plane));
            Preview.Sources.push_back(Step.Label);
            ++Drawn[Step.Label];
        }
    }
    // ② the region, and whether it is one sketch's own loops (every piece a whole curve of it, all its curves used)
    Preview.Region = LocateSketchRegion(Segments, Request.Pick, Scale);
    if (!Preview.Region.Refusal.empty()) { Preview.Refusal = Preview.Region.Refusal; return Preview; }
    std::set<int> Used;
    std::string   Owner;
    bool          WholeCondition = true;
    for (const std::vector<RegionPiece>& Loop : Preview.Region.Loops)
        for (const RegionPiece& Piece : Loop)
        {
            const std::string& Source = Preview.Sources[Piece.Source];
            if (Owner.empty()) Owner = Source;
            WholeCondition = WholeCondition && Source == Owner && Piece.FromFraction <= 1.0e-9 && Piece.ToFraction >= 1.0 - 1.0e-9;
            Used.insert(Piece.Source);
        }
    if (WholeCondition && static_cast<int>(Used.size()) == Drawn[Owner])
    {
        const auto Solved = Document.Regeneration.Sketches.find(Owner);
        if (Solved != Document.Regeneration.Sketches.end() && ExtractSketchProfile(Solved->second).Refusal.empty()) Preview.WholeSketch = Owner;
    }
    return Preview;
}

SolidArcCommandOutcome FillSolidArcRegion(SolidArcDocument& Document, const SolidArcRegionRequest& Request)
{
    const SolidArcRegionPreview Preview = PreviewSolidArcRegion(Document, Request);
    if (Preview.Refusal.empty() && !Preview.WholeSketch.empty()) return FillSolidArcSketch(Document, Preview.WholeSketch, true);
    const std::string Caption = NextCaption(Document, "Fill");
    return RunCommand(Document, "Fill region into " + Caption, false, [&]() -> std::string
    {
        if (!Preview.Refusal.empty()) return "Fill: " + Preview.Refusal;
        FeatureStep Step;
        Step.Kind          = FeatureStepKind::Sketch;
        Step.Label         = "Sketch#" + std::to_string(NextStepOrdinal(Document.Tree));
        Step.Plane         = Preview.Plane;
        Step.Sketch        = ComposeRegionSketch(Preview.Region);
        Step.FillCondition = true;
        if (Step.Sketch.Curves.empty()) return "Fill: the region has no boundary";
        Document.Captions[Step.Label] = Caption;
        Document.Tree.Steps.push_back(std::move(Step));
        return {};
    });
}

SolidArcCommandOutcome TrimSolidArcSketch(SolidArcDocument& Document, const SolidArcTrimRequest& Request)
{
    const auto  Caption = Document.Captions.find(Request.Sketch);
    const std::string Owner = Caption != Document.Captions.end() ? Caption->second : Request.Sketch;
    std::string CurveName = "curve " + std::to_string(Request.Curve);
    for (const FeatureStep& Step : Document.Tree.Steps)
    {
        if (Step.Label != Request.Sketch || Step.Kind != FeatureStepKind::Sketch) continue;
        const SketchModel& Sketch = SolvedSketch(Document, Step);
        if (Request.Curve >= 0 && Request.Curve < static_cast<int>(Sketch.Curves.size())) CurveName = SketchCurveName(Sketch.Curves[Request.Curve]);
    }
    const bool Referenced = SolidArcSketchReferencedCondition(Document, Request.Sketch);

    // ① in place: the sketch itself changes (later steps follow when they still regenerate).
    const std::string Verb = Request.InsertCondition ? "Insert point on " : Request.SplitCondition ? "Cut " : "Trim ";
    SolidArcCommandOutcome InPlace = RunCommand(Document, Verb + CurveName + " of " + Owner, false, [&]() -> std::string
    {
        const SketchTrimOutcome Trim = PreviewSolidArcTrim(Document, Request);
        if (!Trim.Refusal.empty()) return Trim.Refusal;
        const auto Found = std::find_if(Document.Tree.Steps.begin(), Document.Tree.Steps.end(), [&](const FeatureStep& Step) { return Step.Label == Request.Sketch; });
        if (!Trim.Sketch.Curves.empty())
        {
            Found->Sketch = Trim.Sketch;
            return {};
        }
        if (Referenced) return "the trim would leave " + Owner + " without curves, and a later step uses it";
        Document.Tree.Steps.erase(Found);
        Document.Captions.erase(Request.Sketch);
        return {};
    }, 0);
    if (InPlace.AcceptedCondition || !Referenced || !PreviewSolidArcTrim(Document, Request).Refusal.empty()) return InPlace;

    // ② detached: the original stays (hidden) for the steps that read it; the trimmed curves become a free sketch.
    std::string Stem = Owner;
    while (!Stem.empty() && std::isdigit(static_cast<unsigned char>(Stem.back()))) Stem.pop_back();
    if (Stem.empty()) Stem = "Sketch";
    const std::string Copy = NextCaption(Document, Stem);
    return RunCommand(Document, Verb + CurveName + " of " + Owner + " into " + Copy + " (" + Owner + " kept hidden for the steps that read it)", false,
                      [&]() -> std::string
    {
        const SketchTrimOutcome Trim = PreviewSolidArcTrim(Document, Request);
        if (!Trim.Refusal.empty()) return Trim.Refusal;
        const auto Found = std::find_if(Document.Tree.Steps.begin(), Document.Tree.Steps.end(), [&](const FeatureStep& Step) { return Step.Label == Request.Sketch; });
        if (Found == Document.Tree.Steps.end()) return "no sketch step labelled " + Request.Sketch;
        Document.Appearance.HiddenCurves.insert(Request.Sketch);
        std::erase(Document.Selection.Curves, Request.Sketch);
        if (Trim.Sketch.Curves.empty()) return {};
        FeatureStep Detached = *Found;
        Detached.Label  = "Sketch#" + std::to_string(NextStepOrdinal(Document.Tree));
        Detached.Sketch = Trim.Sketch;
        Document.Captions[Detached.Label] = Copy;
        Document.Tree.Steps.push_back(std::move(Detached));
        return {};
    });
}

namespace
{
    // 📝 The cut at Point on every shown sketch: Edited holds each changed sketch (solved model with its curves split),
    //    Outcome the counts and the preview geometry.
    void CutSketchesAtPoint(const SolidArcDocument& Document, const Vector3& Point, std::map<std::string, SketchModel>& Edited, SolidArcPointCut& Outcome)
    {
        constexpr double EndGuard = 1.0e-3;                                                  // [-]   - As SplitSketchCurve: no break at an open end
        const double     Tolerance = 1.0e-6 * std::max(1.0, Length(Point));                   // [len] - Point on a plane / curve
        const auto Shown = [&](const FeatureStep& Step) { return Step.Kind == FeatureStepKind::Sketch && !Document.Appearance.HiddenCurves.contains(Step.Label); };
        for (const FeatureStep& Step : Document.Tree.Steps)
        {
            if (!Shown(Step)) continue;
            const OrthonormalFrame Plane = ElevatedPlane(Document.Tree, Step);
            if (std::fabs(Dot(Point - Plane.Origin, Plane.ZAxis)) > Tolerance) continue;
            const Vector2 Pick{ Dot(Point - Plane.Origin, Plane.XAxis), Dot(Point - Plane.Origin, Plane.YAxis) };
            // Curves of the other shown sketches in this plane (a circle's second break may come from them).
            std::vector<ProfileSegment> Foreign;
            for (const FeatureStep& Other : Document.Tree.Steps)
            {
                if (!Shown(Other) || Other.Label == Step.Label) continue;
                const OrthonormalFrame Frame = ElevatedPlane(Document.Tree, Other);
                const Vector3          Apart = Frame.Origin - Plane.Origin;
                if (std::fabs(Dot(Frame.ZAxis, Plane.ZAxis)) < 1.0 - 1.0e-9 || std::fabs(Dot(Apart, Plane.ZAxis)) > 1.0e-9 * std::max(1.0, Length(Apart))) continue;
                const SketchModel& Sketch = SolvedSketch(Document, Other);
                for (int Curve = 0; Curve < static_cast<int>(Sketch.Curves.size()); ++Curve)
                {
                    std::string    Refusal;
                    ProfileSegment Segment = ResolveSketchCurveSegment(Sketch, Curve, 1.0e-9, Refusal);
                    if (Refusal.empty()) Foreign.push_back(MapSegment(std::move(Segment), Frame, Plane));
                }
            }
            SketchModel Model   = SolvedSketch(Document, Step);
            int         Changed = 0;
            for (int Round = 0; Round < 64; ++Round)
            {
                bool Split = false;
                for (int Curve = 0; Curve < static_cast<int>(Model.Curves.size()) && !Split; ++Curve)
                {
                    if (Model.Curves[Curve].Construction) continue;
                    std::string          Refusal;
                    const ProfileSegment Target = ResolveSketchCurveSegment(Model, Curve, 1.0e-9, Refusal);
                    if (!Refusal.empty() || Target.Kind == ProfileSegmentKind::Spline) continue;
                    const double Fraction = LocateProfileFraction(Target, Pick);
                    if (Distance(EvaluateProfileSegment(Target, Fraction), Pick) > Tolerance) continue;
                    const bool Closed = Target.Kind == ProfileSegmentKind::Circle;
                    if (!Closed && (Fraction <= EndGuard || Fraction >= 1.0 - EndGuard)) continue;   // Touches Point at an end
                    // A circle's partner: its next crossing counter-clockwise with another curve of the plane.
                    std::optional<Vector2> Partner;
                    if (Closed)
                    {
                        double Nearest = 2.0;                                                 // [-] - Fraction gap past the pick
                        const auto Consider = [&](const ProfileSegment& Cutter)
                        {
                            for (const SegmentCrossing& Crossing : IntersectProfileSegments(Target, Cutter, 1.0e-9 * std::max(1.0, Target.Radius)))
                            {
                                const double Gap = Crossing.TargetFraction - Fraction - std::floor(Crossing.TargetFraction - Fraction);
                                if (Gap > EndGuard && Gap < 1.0 - EndGuard && Gap < Nearest) { Nearest = Gap; Partner = Crossing.Position; }
                            }
                        };
                        for (int Other = 0; Other < static_cast<int>(Model.Curves.size()); ++Other)
                        {
                            if (Other == Curve) continue;
                            std::string          Ignored;
                            const ProfileSegment Cutter = ResolveSketchCurveSegment(Model, Other, 1.0e-9, Ignored);
                            if (Ignored.empty()) Consider(Cutter);
                        }
                        for (const ProfileSegment& Cutter : Foreign) Consider(Cutter);
                    }
                    const SketchTrimOutcome Cut = SplitSketchCurve(Model, Curve, Pick, Partner);
                    if (!Cut.Refusal.empty()) continue;
                    std::vector<Vector3> Samples;
                    for (int Index = 0; Index <= 48; ++Index)
                    {
                        const Vector2 Sample = EvaluateProfileSegment(Target, Index / 48.0);
                        Samples.push_back(Plane.Origin + Plane.XAxis * Sample.X + Plane.YAxis * Sample.Y);
                    }
                    Outcome.Lit.push_back(std::move(Samples));
                    for (const Vector2& Break : Cut.SplitPositions) Outcome.Breaks.push_back(Plane.Origin + Plane.XAxis * Break.X + Plane.YAxis * Break.Y);
                    Model = Cut.Sketch;
                    ++Changed;
                    Split = true;
                }
                if (!Split) break;
            }
            if (Changed == 0) continue;
            Outcome.Curves += Changed;
            Outcome.Sketches.push_back(Step.Label);
            Edited[Step.Label] = std::move(Model);
        }
    }
}

SolidArcPointCut PreviewSolidArcPointCut(const SolidArcDocument& Document, const Vector3& Point)
{
    SolidArcPointCut                   Outcome;
    std::map<std::string, SketchModel> Edited;
    CutSketchesAtPoint(Document, Point, Edited, Outcome);
    return Outcome;
}

SolidArcCommandOutcome CutSolidArcSketchesAt(SolidArcDocument& Document, const Vector3& Point)
{
    SolidArcPointCut                   Preview;
    std::map<std::string, SketchModel> Edited;
    CutSketchesAtPoint(Document, Point, Edited, Preview);
    std::string Owners;
    for (const std::string& Label : Preview.Sketches)
    {
        const auto Caption = Document.Captions.find(Label);
        Owners += (Owners.empty() ? "" : ", ") + (Caption != Document.Captions.end() ? Caption->second : Label);
    }
    char Where[96];
    std::snprintf(Where, sizeof Where, "(%.4g, %.4g, %.4g)", Point.X, Point.Y, Point.Z);
    return RunCommand(Document, "Cut " + std::to_string(Preview.Curves) + " curve(s) at " + Where + (Owners.empty() ? std::string{} : " · " + Owners), false, [&]() -> std::string
    {
        if (Preview.Curves == 0) return "no curve passes through that point (hold Ctrl to snap onto a crossing, an end or a midpoint)";
        for (auto& [Label, Model] : Edited)
        {
            const auto Found = std::find_if(Document.Tree.Steps.begin(), Document.Tree.Steps.end(), [&](const FeatureStep& Step) { return Step.Label == Label; });
            if (Found == Document.Tree.Steps.end()) return "no sketch step labelled " + Label;
            Found->Sketch = Model;
        }
        return {};
    }, 0);
}

SolidArcCommandOutcome CutSolidArcSketchCorners(SolidArcDocument& Document, const SolidArcCornerRequest& Request)
{
    const bool        FilletCondition = Request.Shape == SketchCornerShape::Fillet;
    std::string       Owners;
    for (const std::string& Sketch : Request.Sketches)
    {
        const auto Caption = Document.Captions.find(Sketch);
        Owners += (Owners.empty() ? "" : ", ") + (Caption != Document.Captions.end() ? Caption->second : Sketch);
    }
    char Amount[48];
    std::snprintf(Amount, sizeof Amount, "%.4g", Request.Size);
    const std::string Corners = Request.Points.empty() ? std::string("every corner") : std::to_string(Request.Points.size()) + (Request.Points.size() == 1 ? " corner" : " corners");
    const std::string Label   = std::string(FilletCondition ? "Fillet r " : "Chamfer ") + Amount + " · " + Corners + " of " + Owners;
    return RunCommand(Document, Label, false, [&]() -> std::string
    {
        if (Request.Sketches.empty()) return std::string("pick a sketch, or corner points of one (mode 1), to ") + (FilletCondition ? "fillet" : "chamfer");
        if (!Request.Points.empty() && Request.Sketches.size() != 1) return "corner points belong to one sketch";
        for (const std::string& Sketch : Request.Sketches)
        {
            const auto Found = std::find_if(Document.Tree.Steps.begin(), Document.Tree.Steps.end(), [&](const FeatureStep& Step) { return Step.Label == Sketch; });
            if (Found == Document.Tree.Steps.end() || Found->Kind != FeatureStepKind::Sketch) return "no sketch step labelled " + Sketch;
            const SketchCornerOutcome Cut = CutSketchCorners(SolvedSketch(Document, *Found), Request.Points, Request.Size, Request.Shape);
            const auto Caption = Document.Captions.find(Sketch);
            if (!Cut.Refusal.empty()) return (Caption != Document.Captions.end() ? Caption->second : Sketch) + ": " + Cut.Refusal;
            Found->Sketch = Cut.Sketch;
        }
        return {};
    });
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    EDITING (G / R / S · SHIFT+D · CTRL+V · H · J · O · P)
//------------------------------------------------------------------------------------------------------------------------

namespace
{
    [[nodiscard]] std::string ListCaptions(const SolidArcDocument& Document, const std::vector<std::string>& Keys)
    {
        std::string Operands;
        for (const std::string& Key : Keys) Operands += (Operands.empty() ? "" : ", ") + SolidArcBodyCaption(Document, Key);
        return Operands;
    }

    [[nodiscard]] std::vector<std::string> UniqueKeys(const std::vector<std::string>& Keys)
    {
        std::vector<std::string> Unique;
        for (const std::string& Key : Keys) if (std::find(Unique.begin(), Unique.end(), Key) == Unique.end()) Unique.push_back(Key);
        return Unique;
    }

    [[nodiscard]] const FeatureStep* FindStep(const SolidArcDocument& Document, const std::string& Label)
    {
        for (const FeatureStep& Step : Document.Tree.Steps) if (Step.Label == Label) return &Step;
        return nullptr;
    }
}

SolidArcCommandOutcome TransformSolidArcBodies(SolidArcDocument& Document, const std::vector<std::string>& Keys, const AffineMap& Map, const std::string& Verb)
{
    const std::vector<std::string> Unique = UniqueKeys(Keys);
    return RunCommand(Document, Verb + " (" + ListCaptions(Document, Unique) + ")", false, [&]() -> std::string
    {
        if (Unique.empty()) return "pick the bodies to " + Verb + " (mode 4)";
        for (const std::string& Key : Unique) if (!FindRegeneratedBody(Document, Key)) return "no body named " + Key;
        const std::string Caption = NextCaption(Document, Verb);
        for (const std::string& Key : Unique)
        {
            FeatureStep Step;
            Step.Kind      = FeatureStepKind::Transform;
            Step.Label     = "Transform#" + std::to_string(NextStepOrdinal(Document.Tree));
            Step.Target    = Key;
            Step.Placement = Map;
            Document.Captions[Step.Label] = Caption;
            Document.Tree.Steps.push_back(std::move(Step));
        }
        return {};
    });
}

SolidArcCommandOutcome PlaceSolidArcSketches(SolidArcDocument& Document, const std::vector<std::string>& Labels, const AffineMap& Map, const std::string& Verb)
{
    const std::vector<std::string> Unique = UniqueKeys(Labels);
    return RunCommand(Document, Verb + " (" + ListCaptions(Document, Unique) + ")", false, [&]() -> std::string
    {
        if (Unique.empty()) return "pick the sketches to " + Verb;
        // ① rigid only: orthonormal columns, right-handed
        const Vector3 Columns[3] = { Map.ColumnX, Map.ColumnY, Map.ColumnZ };
        for (int First = 0; First < 3; ++First)
            for (int Second = 0; Second < 3; ++Second)
                if (std::abs(Dot(Columns[First], Columns[Second]) - (First == Second ? 1.0 : 0.0)) > 1.0e-9) return "a sketch moves and turns only (🚧 scaling a sketch)";
        if (Dot(Cross(Map.ColumnX, Map.ColumnY), Map.ColumnZ) < 0.0) return "a sketch cannot be mirrored (🚧)";
        const auto Turn = [&](Vector3 Vector) { return Map.ColumnX * Vector.X + Map.ColumnY * Vector.Y + Map.ColumnZ * Vector.Z; };
        // ② carry each plane
        for (const std::string& Label : Unique)
        {
            const auto Found = std::find_if(Document.Tree.Steps.begin(), Document.Tree.Steps.end(), [&](const FeatureStep& Step) { return Step.Label == Label; });
            if (Found == Document.Tree.Steps.end() || Found->Kind != FeatureStepKind::Sketch) return "no sketch step labelled " + Label;
            OrthonormalFrame& Plane = Found->Plane;
            Plane.Origin = Turn(Plane.Origin) + Map.Shift;
            Plane.XAxis  = Normalize(Turn(Plane.XAxis));
            Plane.YAxis  = Normalize(Turn(Plane.YAxis));
            Plane.ZAxis  = Normalize(Turn(Plane.ZAxis));
        }
        return {};
    });
}

SolidArcCommandOutcome DuplicateSolidArcSketches(SolidArcDocument& Document, const std::vector<std::string>& Labels, const AffineMap& Map)
{
    const std::vector<std::string> Unique = UniqueKeys(Labels);
    std::string Operands;
    for (const std::string& Label : Unique)
    {
        const auto Caption = Document.Captions.find(Label);
        Operands += (Operands.empty() ? "" : ", ") + (Caption != Document.Captions.end() ? Caption->second : Label);
    }
    return RunCommand(Document, "Duplicate (" + Operands + ")", false, [&]() -> std::string
    {
        if (Unique.empty()) return "pick the sketches to duplicate";
        const Vector3 Columns[3] = { Map.ColumnX, Map.ColumnY, Map.ColumnZ };
        for (int First = 0; First < 3; ++First)
            for (int Second = 0; Second < 3; ++Second)
                if (std::abs(Dot(Columns[First], Columns[Second]) - (First == Second ? 1.0 : 0.0)) > 1.0e-9) return "a sketch copy moves and turns only (🚧 scaling a sketch)";
        if (Dot(Cross(Map.ColumnX, Map.ColumnY), Map.ColumnZ) < 0.0) return "a sketch copy cannot be mirrored (🚧)";
        const auto Turn = [&](Vector3 Vector) { return Map.ColumnX * Vector.X + Map.ColumnY * Vector.Y + Map.ColumnZ * Vector.Z; };
        std::vector<std::string> Copies;
        for (const std::string& Label : Unique)
        {
            const auto Found = std::find_if(Document.Tree.Steps.begin(), Document.Tree.Steps.end(), [&](const FeatureStep& Step) { return Step.Label == Label; });
            if (Found == Document.Tree.Steps.end() || Found->Kind != FeatureStepKind::Sketch) return "no sketch step labelled " + Label;
            FeatureStep Copy = *Found;
            Copy.Label   = "Sketch#" + std::to_string(NextStepOrdinal(Document.Tree));
            Copy.Sketch  = SolvedSketch(Document, *Found);                                   // As shown (solved positions)
            Copy.Plane.Origin = Turn(Copy.Plane.Origin) + Map.Shift;
            Copy.Plane.XAxis  = Normalize(Turn(Copy.Plane.XAxis));
            Copy.Plane.YAxis  = Normalize(Turn(Copy.Plane.YAxis));
            Copy.Plane.ZAxis  = Normalize(Turn(Copy.Plane.ZAxis));
            const auto  Caption = Document.Captions.find(Label);
            std::string Stem    = Caption != Document.Captions.end() ? Caption->second : std::string("Sketch");
            while (!Stem.empty() && std::isdigit(static_cast<unsigned char>(Stem.back()))) Stem.pop_back();
            if (Stem.empty()) Stem = "Sketch";
            Document.Captions[Copy.Label] = NextCaption(Document, Stem);
            Copies.push_back(Copy.Label);
            Document.Tree.Steps.push_back(std::move(Copy));
        }
        Document.Selection        = {};
        Document.Selection.Curves = Copies;
        return {};
    });
}

SolidArcJointOrigin ResolveSolidArcJointOrigin(const SolidArcDocument& Document, SolidArcSelectionKind Kind, const std::string& Name)
{
    SolidArcJointOrigin Origin;
    if (!Document.Partition) { Origin.Refusal = "there is no body"; return Origin; }
    if (Kind != SolidArcSelectionKind::Edge && Kind != SolidArcSelectionKind::Face) { Origin.Refusal = "a joint origin is an edge or a face"; return Origin; }
    for (const RegeneratedBody& Body : Document.Regeneration.Bodies)
    {
        const std::map<uint32_t, std::string>& Names = Kind == SolidArcSelectionKind::Edge ? Body.Naming.Edges : Body.Naming.Faces;
        for (const auto& [Slot, Candidate] : Names)
        {
            if (Candidate != Name) continue;
            const JointConnector Connector = ResolveJointConnector(*Document.Partition, Kind == SolidArcSelectionKind::Edge ? ReferenceKind::Edge : ReferenceKind::Face, Slot);
            Origin.Body        = Body.Key;
            Origin.Frame       = Connector.Frame;
            Origin.Description = Connector.Description;
            Origin.Refusal     = Connector.Refusal;
            return Origin;
        }
    }
    Origin.Refusal = "no edge or face named " + Name;
    return Origin;
}

SolidArcFaceFoot ProjectOntoSolidArcFace(const SolidArcDocument& Document, const std::string& Name, Vector3 Position)
{
    SolidArcFaceFoot Foot;
    if (!Document.Partition) return Foot;
    for (const RegeneratedBody& Body : Document.Regeneration.Bodies)
        for (const auto& [Slot, Candidate] : Body.Naming.Faces)
        {
            if (Candidate != Name) continue;
            const ModelPartition& Partition = *Document.Partition;
            const FaceToken       Face      = Partition.Faces.TokenAt(Slot);
            if (!Partition.Faces.Authenticate(Face)) return Foot;
            const FaceProfile&     Profile   = Partition.Faces.Resolve(Face);
            const SurfaceGeometry& Surface   = Partition.Surfaces.Resolve(Profile.Surface);
            const Vector2          Parameter = ProjectOntoSurface(Surface, Position, Vector2{});
            const Vector3          Point     = EvaluateSurface(Surface, Parameter).Position;
            const Vector3          Natural   = EvaluateSurfaceNormal(Surface, Parameter);
            if (!std::isfinite(Point.X + Point.Y + Point.Z) || !(Length(Natural) > 0.5)) return Foot;
            Foot.FoundCondition = true;
            Foot.Position       = Point;
            Foot.Normal         = Profile.Sense == Orientation::Forward ? Natural : Natural * -1.0;
            return Foot;
        }
    return Foot;
}

SolidArcCommandOutcome JointSolidArcBodies(SolidArcDocument& Document, const SolidArcJointRequest& Request)
{
    const SolidArcJointOrigin Moving = ResolveSolidArcJointOrigin(Document, Request.MovingKind, Request.MovingName);
    const SolidArcJointOrigin Fixed  = ResolveSolidArcJointOrigin(Document, Request.FixedKind, Request.FixedName);
    const std::string Label = std::string(JointKindLabel(Request.Kind)) + " joint (" + (Moving.Body.empty() ? std::string("?") : SolidArcBodyCaption(Document, Moving.Body)) + " → "
                            + (Fixed.Body.empty() ? std::string("?") : SolidArcBodyCaption(Document, Fixed.Body)) + ")";
    return RunCommand(Document, Label, true, [&]() -> std::string
    {
        if (!Moving.Refusal.empty()) return "moving origin: " + Moving.Refusal;
        if (!Fixed.Refusal.empty()) return "fixed origin: " + Fixed.Refusal;
        if (Moving.Body == Fixed.Body) return "both joint origins are on " + SolidArcBodyCaption(Document, Moving.Body) + " (pick one on each body)";
        const std::string Caption   = NextCaption(Document, JointKindLabel(Request.Kind));
        const std::string StepLabel = "Joint#" + std::to_string(NextStepOrdinal(Document.Tree));
        const double      Degree    = std::numbers::pi / 180.0;
        const auto Bind = [&](const char* Field, double Value, double Low, double High, const char* Meaning, double Factor)
        {
            Document.Tree.Parameters.push_back({ Caption + "." + Field, Value, std::min(Low, Value), std::max(High, Value), Meaning });
            return ParameterValue(Caption + "." + Field, Factor);
        };
        const JointKind Kind   = Request.Kind;
        const bool      Slides = Kind == JointKind::Slider || Kind == JointKind::Cylindrical || Kind == JointKind::PinSlot || Kind == JointKind::Planar;
        const double    Span   = std::max({ 1.0, 4.0 * std::abs(Request.Slide), 4.0 * std::abs(Request.SecondSlide), 4.0 * std::abs(Request.Offset) });
        FeatureStep Step;
        Step.Kind   = FeatureStepKind::Joint;
        Step.Label  = StepLabel;
        Step.Target = Moving.Body;
        Step.Tool   = Fixed.Body;
        Step.References.push_back({ Request.MovingKind == SolidArcSelectionKind::Edge ? ReferenceKind::Edge : ReferenceKind::Face, Request.MovingName, {} });
        Step.References.push_back({ Request.FixedKind == SolidArcSelectionKind::Edge ? ReferenceKind::Edge : ReferenceKind::Face, Request.FixedName, {} });
        FeatureJoint& Joint = Step.Joint;
        Joint.Kind             = Kind;
        Joint.FlippedCondition = Request.Flipped;
        Joint.Angle            = Bind("Angle", Request.Angle, -180.0, 180.0, Kind == JointKind::Ball ? "yaw about the joint z [deg]" : "turn about the joint z [deg]", Degree);
        Joint.Offset           = Bind("Offset", Request.Offset, -Span, Span, "offset along the joint z", 1.0);
        if (Slides) Joint.Slide = Bind("Slide", Request.Slide, -Span, Span, Kind == JointKind::Slider || Kind == JointKind::Cylindrical ? "slide along the joint z" : "slide along the joint x", 1.0);
        if (Kind == JointKind::Planar) Joint.SecondSlide = Bind("SecondSlide", Request.SecondSlide, -Span, Span, "slide along the joint y", 1.0);
        if (Kind == JointKind::Ball)
        {
            Joint.Pitch = Bind("Pitch", Request.Pitch, -180.0, 180.0, "pitch about the joint y [deg]", Degree);
            Joint.Roll  = Bind("Roll", Request.Roll, -180.0, 180.0, "roll about the joint x [deg]", Degree);
        }
        Document.Tree.Steps.push_back(std::move(Step));
        Document.Captions[StepLabel] = Caption;
        return {};
    });
}

SolidArcCommandOutcome DuplicateSolidArcBodies(SolidArcDocument& Document, const std::vector<std::string>& Keys, const AffineMap& Map, const std::string& Source)
{
    const std::vector<std::string> Unique = UniqueKeys(Keys);
    return RunCommand(Document, (Source.empty() ? "Duplicate (" : "Paste (") + ListCaptions(Document, Unique) + ")", false, [&]() -> std::string
    {
        if (Unique.empty()) return Source.empty() ? "pick the bodies to duplicate (mode 4)" : "the clipboard is empty (Ctrl+C copies the picked bodies)";
        if (!Source.empty() && !FindStep(Document, Source)) return "the copied state is gone (step " + Source + " was deleted)";
        if (Source.empty()) for (const std::string& Key : Unique) if (!FindRegeneratedBody(Document, Key)) return "no body named " + Key;
        std::vector<std::string> Copies;
        for (const std::string& Key : Unique)
        {
            FeatureStep Step;
            Step.Kind      = FeatureStepKind::Duplicate;
            Step.Label     = "Duplicate#" + std::to_string(NextStepOrdinal(Document.Tree));
            Step.Target    = Key;
            Step.Placement = Map;
            Step.Source    = Source;
            // "Block1" → "Block1Copy1"; a copy of a copy stays in the family ("Block1Copy1" → "Block1Copy2").
            std::string Stem = SolidArcBodyCaption(Document, Key);
            const size_t Digits = Stem.find_last_not_of("0123456789");
            if (Digits != std::string::npos && Digits + 1 < Stem.size() && Digits >= 3 && Stem.compare(Digits - 3, 4, "Copy") == 0) Stem.erase(Digits - 3);
            Document.Captions[Step.Label] = NextCaption(Document, Stem + "Copy");
            Copies.push_back(Step.Label);
            Document.Tree.Steps.push_back(std::move(Step));
        }
        Document.Selection          = {};
        Document.Selection.Bodies   = Copies;
        Document.Selection.LastKind = SolidArcSelectionKind::Body;
        return {};
    });
}

// 📝 A sketch cutter's extrude: the kernel moves the profile plane to Top along the normal and sweeps Bottom − Top, so a
//    Start above the plane is fine there (only the authored E refuses one: its start sinks into a tangent body).
SolidArcCommandOutcome KnifeSolidArcSketch(SolidArcDocument& Document, const std::string& SketchLabel, double Top, double Bottom, FeatureCombination Combination,
                                           const std::string& Target)
{
    const std::string Caption = NextCaption(Document, "Knife");
    const auto        Source  = Document.Captions.find(SketchLabel);
    const std::string Label   = Caption + " · " + (Source != Document.Captions.end() ? Source->second : SketchLabel) + " · " + FeatureCombinationLabel(Combination) +
                                " · depth " + FormatAmount(-Bottom);
    return RunCommand(Document, Label, false, [&]() -> std::string
    {
        const auto Found = std::find_if(Document.Tree.Steps.begin(), Document.Tree.Steps.end(), [&](const FeatureStep& Step) { return Step.Label == SketchLabel; });
        if (Found == Document.Tree.Steps.end() || Found->Kind != FeatureStepKind::Sketch) return "no sketch step labelled " + SketchLabel;
        if (!(Top >= 0.0) || !(Bottom < -1.0e-9) || !(Top - Bottom > 1.0e-9)) return "the knife depth must be positive";
        if (Combination == FeatureCombination::New) return "choose Difference, Union, Intersection or Separate";
        if (Combination != FeatureCombination::Separate)
        {
            if (!Document.Summary.PresentCondition) return "nothing to cut yet (no body)";
            if (Target.empty() || !FindRegeneratedBody(Document, Target)) return Target.empty() ? std::string("no body under the knife") : "no body named " + Target;
        }
        Found->FillCondition = true;                                                         // A cutter is solid
        FeatureStep Step;
        Step.Kind        = FeatureStepKind::Extrude;
        Step.Label       = "Extrude#" + std::to_string(NextStepOrdinal(Document.Tree));
        Step.Profile     = SketchLabel;
        if (Top > 0.0) Step.Start = FeatureValue{ Top };
        Step.Extent      = FeatureValue{ Bottom };
        Step.Combination = Combination;
        Step.Target      = Combination == FeatureCombination::Separate ? std::string{} : Target;
        Document.Captions[Step.Label] = Caption;
        const std::string Made = Step.Label;
        Document.Tree.Steps.push_back(std::move(Step));
        Document.Selection = {};
        if (Combination == FeatureCombination::Separate)
        {
            Document.Selection.Bodies   = { Made };
            Document.Selection.LastKind = SolidArcSelectionKind::Body;
        }
        return {};
    });
}

SolidArcCommandOutcome ImprintSolidArcSketch(SolidArcDocument& Document, const std::string& SketchLabel, const std::string& Target)
{
    const std::string Caption = NextCaption(Document, "Imprint");
    const auto        Source  = Document.Captions.find(SketchLabel);
    const std::string Key     = Target.empty() ? Document.Regeneration.Active : Target;
    const std::string Label   = Caption + " · " + (Source != Document.Captions.end() ? Source->second : SketchLabel) + " on " + SolidArcBodyCaption(Document, Key);
    // The tool stands on the side away from the material (a closed outline on a face); open chains and undecided outlines
    // take the sketch normal (orient-to-face sketches: outward). Either side splits the same faces.
    const int Material = MeasureSolidArcProfileMaterial(Document, SketchLabel, Key);
    return RunCommand(Document, Label, false, [&]() -> std::string
    {
        const auto Found = std::find_if(Document.Tree.Steps.begin(), Document.Tree.Steps.end(), [&](const FeatureStep& Step) { return Step.Label == SketchLabel; });
        if (Found == Document.Tree.Steps.end() || Found->Kind != FeatureStepKind::Sketch) return "no sketch step labelled " + SketchLabel;
        if (!Document.Summary.PresentCondition) return "nothing to imprint on yet (no body)";
        if (Key.empty() || !FindRegeneratedBody(Document, Key)) return Key.empty() ? std::string("no body under the outline") : "no body named " + Key;
        FeatureStep Step;
        Step.Kind    = FeatureStepKind::Imprint;
        Step.Label   = "Imprint#" + std::to_string(NextStepOrdinal(Document.Tree));
        Step.Profile = SketchLabel;
        Step.Extent  = FeatureValue{ Material > 0 ? -1.0 : 1.0 };
        Step.Target  = Key;
        Document.Captions[Step.Label] = Caption;
        Document.Tree.Steps.push_back(std::move(Step));
        Document.Selection = {};
        return {};
    });
}

SolidArcCommandOutcome KnifeSolidArcInsert(SolidArcDocument& Document, const SolidArcInsertPlacement& Placement)
{
    const std::string Caption = NextCaption(Document, "Knife");
    const std::string Label   = Caption + " · " + Placement.Name + " · " + FeatureCombinationLabel(Placement.Combination);
    return RunCommand(Document, Label, false, [&]() -> std::string
    {
        if (Placement.Exchange.empty()) return "the insert " + Placement.Name + " holds no solid";
        const AffineMap& Map = Placement.Map;
        const double Volume = Dot(Map.ColumnX, Cross(Map.ColumnY, Map.ColumnZ));
        if (!(Volume > 1.0e-15)) return "the insert placement is degenerate (zero size or mirrored)";
        if (Placement.Combination != FeatureCombination::Separate)
        {
            if (Placement.Combination != FeatureCombination::Union && Placement.Combination != FeatureCombination::Difference &&
                Placement.Combination != FeatureCombination::Intersection)
                return "choose Difference, Union, Intersection or Separate";
            if (Placement.Target.empty() || !FindRegeneratedBody(Document, Placement.Target))
                return Placement.Target.empty() ? std::string("place the insert on a body's face") : "no body named " + Placement.Target;
        }
        FeatureStep Import;
        Import.Kind         = FeatureStepKind::Import;
        Import.Label        = "Import#" + std::to_string(NextStepOrdinal(Document.Tree));
        Import.Exchange     = Placement.Exchange;
        Import.ExchangeUnit = SolidArcModelUnitMillimetres;
        const std::string Insert = Import.Label;
        Document.Captions[Insert] = NextCaption(Document, Placement.Name.empty() ? std::string("Insert") : Placement.Name);
        Document.Tree.Steps.push_back(std::move(Import));
        FeatureStep Place;
        Place.Kind      = FeatureStepKind::Transform;
        Place.Label     = "Transform#" + std::to_string(NextStepOrdinal(Document.Tree));
        Place.Target    = Insert;
        Place.Placement = Map;
        Document.Captions[Place.Label] = NextCaption(Document, "Place");
        Document.Tree.Steps.push_back(std::move(Place));
        Document.Selection = {};
        if (Placement.Combination == FeatureCombination::Separate)
        {
            Document.Selection.Bodies   = { Insert };
            Document.Selection.LastKind = SolidArcSelectionKind::Body;
            return {};
        }
        FeatureStep Combine;
        Combine.Kind        = FeatureStepKind::Combine;
        Combine.Label       = "Combine#" + std::to_string(NextStepOrdinal(Document.Tree));
        Combine.Combination = Placement.Combination;
        Combine.Target      = Placement.Target;
        Combine.Tool        = Insert;
        Document.Captions[Combine.Label] = Caption;
        Document.Tree.Steps.push_back(std::move(Combine));
        return {};
    });
}

SolidArcCommandOutcome KnifeSolidArcBodies(SolidArcDocument& Document, const std::string& Target, const std::vector<std::string>& Tools, FeatureCombination Combination,
                                           bool KeepCondition)
{
    const std::string Cut = Target;                                                          // Target may live in the selection this command clears
    const std::string Caption = NextCaption(Document, "Knife");
    std::string Operands = SolidArcBodyCaption(Document, Cut);
    const char* Sign = Combination == FeatureCombination::Difference ? " − " : Combination == FeatureCombination::Intersection ? " ∩ " : " ∪ ";
    for (const std::string& Tool : Tools) Operands += Sign + SolidArcBodyCaption(Document, Tool);
    const std::string Label = Caption + " · " + FeatureCombinationLabel(Combination) + " (" + Operands + ")" + (KeepCondition ? " · cutters kept" : "");
    return RunCommand(Document, Label, false, [&]() -> std::string
    {
        if (Combination != FeatureCombination::Union && Combination != FeatureCombination::Difference && Combination != FeatureCombination::Intersection)
            return "choose Difference, Union or Intersection";
        if (Tools.empty()) return "select the target body, then the cutter bodies";
        if (!FindRegeneratedBody(Document, Cut)) return "no body named " + Cut;
        std::set<std::string> Seen{ Cut };
        for (const std::string& Tool : Tools)
        {
            if (!FindRegeneratedBody(Document, Tool)) return "no body named " + Tool;
            if (!Seen.insert(Tool).second) return SolidArcBodyCaption(Document, Tool) + " is picked twice (the target cannot be a cutter)";
        }
        const AffineMap Identity;                                                            // In place
        for (const std::string& Tool : Tools)
        {
            std::string Cutter = Tool;
            if (KeepCondition)
            {
                FeatureStep Copy;
                Copy.Kind      = FeatureStepKind::Duplicate;
                Copy.Label     = "Duplicate#" + std::to_string(NextStepOrdinal(Document.Tree));
                Copy.Target    = Tool;
                Copy.Placement = Identity;
                Document.Captions[Copy.Label] = NextCaption(Document, SolidArcBodyCaption(Document, Tool) + "Cutter");
                Cutter = Copy.Label;
                Document.Tree.Steps.push_back(std::move(Copy));
            }
            FeatureStep Step;
            Step.Kind        = FeatureStepKind::Combine;
            Step.Label       = "Combine#" + std::to_string(NextStepOrdinal(Document.Tree));
            Step.Combination = Combination;
            Step.Target      = Cut;
            Step.Tool        = Cutter;
            Document.Captions[Step.Label] = Caption;
            Document.Tree.Steps.push_back(std::move(Step));
        }
        Document.Selection          = {};
        Document.Selection.Bodies   = { Cut };
        Document.Selection.LastKind = SolidArcSelectionKind::Body;
        return {};
    });
}

SolidArcCommandOutcome KnifeSolidArcPlacedBody(SolidArcDocument& Document, const std::string& Target, const std::string& Tool, const AffineMap& Map,
                                               FeatureCombination Combination, bool KeepCondition)
{
    const std::string Cut     = Target;                                                      // Both may live in the selection this command clears
    const std::string Moved   = Tool;
    const std::string Caption = NextCaption(Document, "Knife");
    const char*       Sign    = Combination == FeatureCombination::Difference ? " − " : Combination == FeatureCombination::Intersection ? " ∩ " : " ∪ ";
    const std::string Label   = Caption + " · " + FeatureCombinationLabel(Combination) + " (" + SolidArcBodyCaption(Document, Cut) + Sign + SolidArcBodyCaption(Document, Moved) +
                                " placed)" + (KeepCondition ? " · cutter kept" : "");
    return RunCommand(Document, Label, false, [&]() -> std::string
    {
        if (Combination != FeatureCombination::Union && Combination != FeatureCombination::Difference && Combination != FeatureCombination::Intersection)
            return "choose Difference, Union or Intersection";
        if (Cut.empty()) return "point at a face of the body to cut";
        if (!FindRegeneratedBody(Document, Cut)) return "no body named " + Cut;
        if (!FindRegeneratedBody(Document, Moved)) return "no body named " + Moved;
        if (Cut == Moved) return "the cutter cannot cut itself: point at another body";
        const double Volume = Dot(Map.ColumnX, Cross(Map.ColumnY, Map.ColumnZ));
        if (!(Volume > 1.0e-15)) return "the cutter placement is degenerate (zero size or mirrored)";
        FeatureStep Place;
        Place.Kind      = KeepCondition ? FeatureStepKind::Duplicate : FeatureStepKind::Transform;
        Place.Label     = (KeepCondition ? "Duplicate#" : "Transform#") + std::to_string(NextStepOrdinal(Document.Tree));
        Place.Target    = Moved;
        Place.Placement = Map;
        const std::string Cutter = KeepCondition ? Place.Label : Moved;
        Document.Captions[Place.Label] = NextCaption(Document, KeepCondition ? SolidArcBodyCaption(Document, Moved) + "Cutter" : std::string("Place"));
        Document.Tree.Steps.push_back(std::move(Place));
        FeatureStep Combine;
        Combine.Kind        = FeatureStepKind::Combine;
        Combine.Label       = "Combine#" + std::to_string(NextStepOrdinal(Document.Tree));
        Combine.Combination = Combination;
        Combine.Target      = Cut;
        Combine.Tool        = Cutter;
        Document.Captions[Combine.Label] = Caption;
        Document.Tree.Steps.push_back(std::move(Combine));
        Document.Selection = {};
        return {};
    });
}

SolidArcCommandOutcome KnifeSolidArcPrimitive(SolidArcDocument& Document, const SolidArcConstructRequest& Request, const std::string& Target,
                                              FeatureCombination Combination, bool KeepCondition)
{
    const std::string Cut     = Target;
    const std::string Caption = NextCaption(Document, "Knife");
    const std::string Label   = Caption + " · " + SolidArcConstructLabel(Request.Kind) + " · " + FeatureCombinationLabel(Combination) +
                                (Combination != FeatureCombination::Separate && !Cut.empty() ? " (" + SolidArcBodyCaption(Document, Cut) + ")" : std::string()) +
                                (KeepCondition && Combination != FeatureCombination::Separate ? " · cutter kept" : "");
    return RunCommand(Document, Label, false, [&]() -> std::string
    {
        if (Combination != FeatureCombination::Union && Combination != FeatureCombination::Difference && Combination != FeatureCombination::Intersection &&
            Combination != FeatureCombination::Separate)
            return "choose Difference, Union, Intersection or Separate";
        if (Combination != FeatureCombination::Separate)
        {
            if (!Document.Summary.PresentCondition) return "nothing to cut yet (no body)";
            if (Cut.empty()) return "no body under the knife: draw the cutter on a face of the body to cut";
            if (!FindRegeneratedBody(Document, Cut)) return "no body named " + Cut;
        }
        // The same checks as a Construct tile (a cone's Height may be negative: it points down).
        const SolidArcConstructKind Tile = Request.Kind;
        const bool BoxLike = Tile == SolidArcConstructKind::Block || Tile == SolidArcConstructKind::Wedge;
        const bool Round   = Tile == SolidArcConstructKind::Cylinder || Tile == SolidArcConstructKind::Sphere || Tile == SolidArcConstructKind::Cone ||
                             Tile == SolidArcConstructKind::Prism || Tile == SolidArcConstructKind::Tube;
        const bool Tall    = Tile != SolidArcConstructKind::Sphere && Tile != SolidArcConstructKind::Torus;
        if ((BoxLike && (Request.Width <= 0.0 || Request.Depth <= 0.0)) || (Round && Request.Radius <= 0.0) ||
            (Tall && (Tile == SolidArcConstructKind::Cone ? Request.Height == 0.0 : Request.Height <= 0.0)))
            return "every dimension must be positive";
        if (Tile == SolidArcConstructKind::Torus && (Request.Minor <= 0.0 || Request.Major <= Request.Minor)) return "torus: Major must exceed Minor > 0";
        if (Tile == SolidArcConstructKind::Tube && (Request.Inner <= 0.0 || Request.Inner >= Request.Radius)) return "tube: the bore must lie inside the outer radius";
        if (Tile == SolidArcConstructKind::Prism && (Request.Sides < 3 || Request.Sides > 64)) return "prism: 3 … 64 sides";
        if (Request.PlaneCondition)
        {
            const OrthonormalFrame& Face = Request.Plane;
            const bool Finite = std::isfinite(Face.Origin.X) && std::isfinite(Face.Origin.Y) && std::isfinite(Face.Origin.Z);
            const bool Square = std::abs(Length(Face.XAxis) - 1.0) < 1.0e-9 && std::abs(Length(Face.YAxis) - 1.0) < 1.0e-9 && std::abs(Length(Face.ZAxis) - 1.0) < 1.0e-9
                             && std::abs(Dot(Face.XAxis, Face.YAxis)) < 1.0e-9 && Length(Cross(Face.XAxis, Face.YAxis) - Face.ZAxis) < 1.0e-9;
            if (!Finite || !Square) return "the face plane is not an orthonormal right-handed frame";
        }
        SolidArcConstructRequest Cutter = Request;
        const bool Direct = Combination != FeatureCombination::Separate && !KeepCondition;
        Cutter.Target      = Direct ? Cut : std::string{};
        Cutter.Combination = Direct ? Combination : FeatureCombination::Separate;
        AuthorTile(Document, Cutter, Cutter.Combination);
        const std::string Made = Document.Tree.Steps.back().Label;                           // The sweep: the new body when Separate
        if (Direct) Document.Captions[Made] = Caption;                                       // The sweep is the cut itself
        Document.Selection = {};
        if (Combination == FeatureCombination::Separate)
        {
            Document.Selection.Bodies   = { Made };
            Document.Selection.LastKind = SolidArcSelectionKind::Body;
            return {};
        }
        if (Direct) return {};
        FeatureStep Copy;
        Copy.Kind      = FeatureStepKind::Duplicate;
        Copy.Label     = "Duplicate#" + std::to_string(NextStepOrdinal(Document.Tree));
        Copy.Target    = Made;
        Copy.Placement = AffineMap{};
        const std::string Tool = Copy.Label;
        Document.Captions[Tool] = NextCaption(Document, Document.Captions[Made] + "Cutter");
        Document.Tree.Steps.push_back(std::move(Copy));
        FeatureStep Combine;
        Combine.Kind        = FeatureStepKind::Combine;
        Combine.Label       = "Combine#" + std::to_string(NextStepOrdinal(Document.Tree));
        Combine.Combination = Combination;
        Combine.Target      = Cut;
        Combine.Tool        = Tool;
        Document.Captions[Combine.Label] = Caption;
        Document.Tree.Steps.push_back(std::move(Combine));
        return {};
    });
}

SolidArcCommandOutcome ChangeSolidArcVisibility(SolidArcDocument& Document, SolidArcVisibilityVerb Verb)
{
    // The picked bodies: body picks plus the owners of picked faces and edges.
    std::set<std::string> Picked(Document.Selection.Bodies.begin(), Document.Selection.Bodies.end());
    for (const std::string& Face : Document.Selection.Faces) if (const std::string Key = SolidArcBodyOfName(Document, SolidArcSelectionKind::Face, Face); !Key.empty()) Picked.insert(Key);
    for (const std::string& Edge : Document.Selection.Edges) if (const std::string Key = SolidArcBodyOfName(Document, SolidArcSelectionKind::Edge, Edge); !Key.empty()) Picked.insert(Key);
    static const char* const Labels[] = { "Hide selected", "Hide unselected", "Unhide all", "Invert hidden" };
    return RunAppearanceCommand(Document, Labels[static_cast<int>(Verb)], [&]() -> std::string
    {
        std::set<std::string>& Hidden = Document.Appearance.HiddenBodies;
        std::set<std::string>  Present;
        for (const RegeneratedBody& Body : Document.Regeneration.Bodies) Present.insert(Body.Key);
        std::erase_if(Hidden, [&](const std::string& Key) { return !Present.contains(Key); });       // Keys of removed bodies
        const std::set<std::string> Before = Hidden;
        switch (Verb)
        {
        case SolidArcVisibilityVerb::HideSelected:
            if (Picked.empty()) return "pick the bodies to hide";
            Hidden.insert(Picked.begin(), Picked.end());
            break;
        case SolidArcVisibilityVerb::HideUnselected:
            if (Picked.empty()) return "pick the bodies to keep in view";
            for (const std::string& Key : Present) if (!Picked.contains(Key)) Hidden.insert(Key);
            break;
        case SolidArcVisibilityVerb::UnhideAll:
            Hidden.clear();
            break;
        case SolidArcVisibilityVerb::InvertHidden:
        {
            std::set<std::string> Swapped;
            for (const std::string& Key : Present) if (!Before.contains(Key)) Swapped.insert(Key);
            Hidden = std::move(Swapped);
            break;
        }
        }
        if (Hidden == Before) return Verb == SolidArcVisibilityVerb::UnhideAll ? "nothing is hidden" : "nothing changes";
        // Hidden bodies leave the selection.
        std::erase_if(Document.Selection.Bodies, [&](const std::string& Key) { return Hidden.contains(Key); });
        std::erase_if(Document.Selection.Faces, [&](const std::string& Name) { return Hidden.contains(SolidArcBodyOfName(Document, SolidArcSelectionKind::Face, Name)); });
        std::erase_if(Document.Selection.Edges, [&](const std::string& Name) { return Hidden.contains(SolidArcBodyOfName(Document, SolidArcSelectionKind::Edge, Name)); });
        return {};
    });
}

SolidArcCommandOutcome ToggleSolidArcBodyVisibility(SolidArcDocument& Document, const std::string& Key)
{
    const bool HiddenCondition = Document.Appearance.HiddenBodies.contains(Key);
    return RunAppearanceCommand(Document, HiddenCondition ? "Show body" : "Hide body", [&]() -> std::string
    {
        const bool Present = std::any_of(Document.Regeneration.Bodies.begin(), Document.Regeneration.Bodies.end(),
                                         [&](const RegeneratedBody& Body) { return Body.Key == Key; });
        if (!Present) return "no body " + Key;
        std::set<std::string>& Hidden = Document.Appearance.HiddenBodies;
        if (HiddenCondition)
        {
            Hidden.erase(Key);
            return {};
        }
        Hidden.insert(Key);
        // A hidden body leaves the selection.
        std::erase(Document.Selection.Bodies, Key);
        std::erase_if(Document.Selection.Faces, [&](const std::string& Name) { return SolidArcBodyOfName(Document, SolidArcSelectionKind::Face, Name) == Key; });
        std::erase_if(Document.Selection.Edges, [&](const std::string& Name) { return SolidArcBodyOfName(Document, SolidArcSelectionKind::Edge, Name) == Key; });
        return {};
    });
}

bool SolidArcCurveHiddenCondition(const SolidArcDocument& Document, const std::string& Label) noexcept
{
    return Document.Appearance.HiddenCurves.contains(Label);
}

bool SolidArcLockedCondition(const SolidArcDocument& Document, const std::string& Key) noexcept
{
    return Document.Appearance.LockedFigures.contains(Key);
}

SolidArcCommandOutcome ToggleSolidArcCurveVisibility(SolidArcDocument& Document, const std::string& Label)
{
    const bool HiddenCondition = Document.Appearance.HiddenCurves.contains(Label);
    return RunAppearanceCommand(Document, HiddenCondition ? "Show curve" : "Hide curve", [&]() -> std::string
    {
        const bool Present = std::any_of(Document.Tree.Steps.begin(), Document.Tree.Steps.end(), [&](const FeatureStep& Step)
                                         { return Step.Label == Label && (Step.Kind == FeatureStepKind::Sketch || Step.Kind == FeatureStepKind::SpaceCurve); });
        if (!Present) return "no sketch or curve " + Label;
        std::set<std::string>& Hidden = Document.Appearance.HiddenCurves;
        if (HiddenCondition)
        {
            Hidden.erase(Label);
            return {};
        }
        Hidden.insert(Label);
        std::erase(Document.Selection.Curves, Label);                                       // A hidden curve leaves the selection
        return {};
    });
}

SolidArcCommandOutcome ToggleSolidArcLock(SolidArcDocument& Document, const std::string& Key)
{
    const bool LockedCondition = Document.Appearance.LockedFigures.contains(Key);
    return RunAppearanceCommand(Document, LockedCondition ? "Unlock" : "Lock", [&]() -> std::string
    {
        const bool Body  = std::any_of(Document.Regeneration.Bodies.begin(), Document.Regeneration.Bodies.end(), [&](const RegeneratedBody& Entry) { return Entry.Key == Key; });
        const bool Curve = std::any_of(Document.Tree.Steps.begin(), Document.Tree.Steps.end(), [&](const FeatureStep& Step)
                                       { return Step.Label == Key && (Step.Kind == FeatureStepKind::Sketch || Step.Kind == FeatureStepKind::SpaceCurve); });
        if (!Body && !Curve) return "no body, sketch or curve " + Key;
        std::set<std::string>& Locked = Document.Appearance.LockedFigures;
        if (LockedCondition)
        {
            Locked.erase(Key);
            return {};
        }
        Locked.insert(Key);
        // A locked figure leaves the selection (it cannot be edited until unlocked).
        std::erase(Document.Selection.Curves, Key);
        std::erase(Document.Selection.Bodies, Key);
        std::erase_if(Document.Selection.Faces, [&](const std::string& Name) { return SolidArcBodyOfName(Document, SolidArcSelectionKind::Face, Name) == Key; });
        std::erase_if(Document.Selection.Edges, [&](const std::string& Name) { return SolidArcBodyOfName(Document, SolidArcSelectionKind::Edge, Name) == Key; });
        return {};
    });
}

SolidArcCommandOutcome JoinSolidArcSketches(SolidArcDocument& Document, const std::vector<std::string>& Sketches)
{
    std::vector<std::string> Captions;
    for (const std::string& Label : Sketches) Captions.push_back(Label);
    return RunCommand(Document, "Join (" + ListCaptions(Document, Captions) + ")", false, [&]() -> std::string
    {
        if (Sketches.size() < 2) return "pick two or more sketches to join";
        const FeatureStep* First = FindStep(Document, Sketches.front());
        if (!First || First->Kind != FeatureStepKind::Sketch) return "no sketch step labelled " + Sketches.front();
        const OrthonormalFrame Base   = ElevatedPlane(Document.Tree, *First);
        SketchModel            Joined = SolvedSketch(Document, *First);
        for (size_t Index = 1; Index < Sketches.size(); ++Index)
        {
            const FeatureStep* Other = FindStep(Document, Sketches[Index]);
            if (!Other || Other->Kind != FeatureStepKind::Sketch) return "no sketch step labelled " + Sketches[Index];
            if (Sketches[Index] == Sketches.front()) return SolidArcBodyCaption(Document, Sketches[Index]) + " is picked twice";
            const OrthonormalFrame Plane = ElevatedPlane(Document.Tree, *Other);
            const Vector3          Apart = Plane.Origin - Base.Origin;
            if (std::fabs(Dot(Plane.ZAxis, Base.ZAxis)) < 1.0 - 1.0e-9 || std::fabs(Dot(Apart, Base.ZAxis)) > 1.0e-9 * std::max(1.0, Length(Apart)))
                return SolidArcBodyCaption(Document, Other->Label) + " is not in the plane of " + SolidArcBodyCaption(Document, First->Label);
            SketchPlacement Placement;
            Placement.Origin = { Dot(Apart, Base.XAxis), Dot(Apart, Base.YAxis) };
            Placement.XAxis  = { Dot(Plane.XAxis, Base.XAxis), Dot(Plane.XAxis, Base.YAxis) };
            Placement.YAxis  = { Dot(Plane.YAxis, Base.XAxis), Dot(Plane.YAxis, Base.YAxis) };
            const double Join = 1.0e-9 * std::max({ 1.0, MeasureSketchScale(Joined), MeasureSketchScale(SolvedSketch(Document, *Other)) });
            const SketchJoinOutcome Outcome = JoinSketchModels(Joined, SolvedSketch(Document, *Other), Placement, Join);
            if (!Outcome.Refusal.empty()) return Outcome.Refusal;
            Joined = Outcome.Sketch;
        }
        // A sketch a later step reads stays (hidden) so that step keeps its input; the others merge away.
        std::set<std::string>& Hidden = Document.Appearance.HiddenCurves;
        const auto Keep = std::find_if(Document.Tree.Steps.begin(), Document.Tree.Steps.end(), [&](const FeatureStep& Step) { return Step.Label == Sketches.front(); });
        if (SolidArcSketchReferencedCondition(Document, Sketches.front()))
        {
            std::string Stem = SolidArcBodyCaption(Document, Sketches.front());
            while (!Stem.empty() && std::isdigit(static_cast<unsigned char>(Stem.back()))) Stem.pop_back();
            FeatureStep Copy = *Keep;
            Copy.Label  = "Sketch#" + std::to_string(NextStepOrdinal(Document.Tree));
            Copy.Sketch = std::move(Joined);
            Document.Captions[Copy.Label] = NextCaption(Document, Stem.empty() ? std::string("Sketch") : Stem);
            Hidden.insert(Sketches.front());
            Document.Tree.Steps.push_back(std::move(Copy));
        }
        else Keep->Sketch = std::move(Joined);
        for (size_t Index = 1; Index < Sketches.size(); ++Index)
        {
            if (SolidArcSketchReferencedCondition(Document, Sketches[Index]))
            {
                Hidden.insert(Sketches[Index]);
                continue;
            }
            std::erase_if(Document.Tree.Steps, [&](const FeatureStep& Step) { return Step.Label == Sketches[Index]; });
            Document.Captions.erase(Sketches[Index]);
        }
        for (const std::string& Label : Sketches)
            if (Hidden.contains(Label) || !FindStep(Document, Label)) std::erase(Document.Selection.Curves, Label);
        return {};
    }, 0);
}

SolidArcCommandOutcome FlowSolidArcSketches(SolidArcDocument& Document, const SolidArcFlowRequest& Request)
{
    const std::string Caption = NextCaption(Document, "Flow");
    std::vector<std::string> Shapes;
    for (const std::string& Label : Request.Shapes) Shapes.push_back(Label);
    const char* Fit = Request.Settings.StretchEnabled ? "stretched" : "true length";
    return RunCommand(Document, "Flow " + ListCaptions(Document, Shapes) + " along " + SolidArcBodyCaption(Document, Request.Rail) + " into " + Caption + " (" + Fit + ")", false,
                      [&]() -> std::string
    {
        const FeatureStep* Rail = FindStep(Document, Request.Rail);
        if (!Rail || Rail->Kind != FeatureStepKind::Sketch) return "Flow: pick the rail sketch first, then the shapes";
        if (Request.Shapes.empty()) return "Flow: pick the rail curve, then one or more shapes to bend along it";
        const FeatureStep* First = FindStep(Document, Request.Shapes.front());
        if (!First || First->Kind != FeatureStepKind::Sketch) return "Flow: no sketch step labelled " + Request.Shapes.front();
        // ① the shapes as one model in the first shape's plane (coplanar, as Join).
        const OrthonormalFrame Base  = ElevatedPlane(Document.Tree, *First);
        SketchModel            Shape = SolvedSketch(Document, *First);
        for (const std::string& Label : Request.Shapes)
        {
            if (Label == Request.Rail) return "Flow: " + SolidArcBodyCaption(Document, Label) + " is the rail and a shape";
            if (Label == Request.Shapes.front()) continue;
            const FeatureStep* Other = FindStep(Document, Label);
            if (!Other || Other->Kind != FeatureStepKind::Sketch) return "Flow: no sketch step labelled " + Label;
            const OrthonormalFrame Plane = ElevatedPlane(Document.Tree, *Other);
            const Vector3          Apart = Plane.Origin - Base.Origin;
            if (std::fabs(Dot(Plane.ZAxis, Base.ZAxis)) < 1.0 - 1.0e-9 || std::fabs(Dot(Apart, Base.ZAxis)) > 1.0e-9 * std::max(1.0, Length(Apart)))
                return "Flow: " + SolidArcBodyCaption(Document, Label) + " is not in the plane of " + SolidArcBodyCaption(Document, First->Label);
            SketchPlacement Placement;
            Placement.Origin = { Dot(Apart, Base.XAxis), Dot(Apart, Base.YAxis) };
            Placement.XAxis  = { Dot(Plane.XAxis, Base.XAxis), Dot(Plane.XAxis, Base.YAxis) };
            Placement.YAxis  = { Dot(Plane.YAxis, Base.XAxis), Dot(Plane.YAxis, Base.YAxis) };
            const double Join = 1.0e-9 * std::max({ 1.0, MeasureSketchScale(Shape), MeasureSketchScale(SolvedSketch(Document, *Other)) });
            const SketchJoinOutcome Joined = JoinSketchModels(Shape, SolvedSketch(Document, *Other), Placement, Join);
            if (!Joined.Refusal.empty()) return "Flow: " + Joined.Refusal;
            Shape = Joined.Sketch;
        }
        // ② bend, then a new sketch step on the rail's plane.
        const SketchFlowOutcome Flowed = FlowSketchModel(Shape, SolvedSketch(Document, *Rail), Request.Settings);
        if (!Flowed.Refusal.empty()) return "Flow: " + Flowed.Refusal;
        FeatureStep Step   = *Rail;
        Step.Label         = "Sketch#" + std::to_string(NextStepOrdinal(Document.Tree));
        Step.Sketch        = Flowed.Sketch;
        Step.FillCondition = First->FillCondition && ExtractSketchProfile(Flowed.Sketch).Refusal.empty();
        Document.Captions[Step.Label] = Caption;
        Document.Selection.Curves     = { Step.Label };
        Document.Tree.Steps.push_back(std::move(Step));
        return {};
    });
}

std::vector<std::string> SolidArcJoinCandidates(const SolidArcDocument& Document, const std::string& Sketch)
{
    std::vector<std::string> Group;
    const FeatureStep* First = FindStep(Document, Sketch);
    if (!First || First->Kind != FeatureStepKind::Sketch) return Group;
    const OrthonormalFrame   Base = ElevatedPlane(Document.Tree, *First);
    std::vector<SketchModel> Members{ SolvedSketch(Document, *First) };
    Group.push_back(Sketch);
    // Every other candidate, placed in the base plane's coordinates.
    std::vector<std::pair<std::string, SketchModel>> Pending;
    for (const FeatureStep& Step : Document.Tree.Steps)
    {
        if (Step.Kind != FeatureStepKind::Sketch || Step.Label == Sketch || SolidArcSketchConsumedCondition(Document, Step.Label)) continue;
        const OrthonormalFrame Plane = ElevatedPlane(Document.Tree, Step);
        const Vector3          Apart = Plane.Origin - Base.Origin;
        if (std::fabs(Dot(Plane.ZAxis, Base.ZAxis)) < 1.0 - 1.0e-9 || std::fabs(Dot(Apart, Base.ZAxis)) > 1.0e-9 * std::max(1.0, Length(Apart))) continue;
        SketchPlacement Placement;
        Placement.Origin = { Dot(Apart, Base.XAxis), Dot(Apart, Base.YAxis) };
        Placement.XAxis  = { Dot(Plane.XAxis, Base.XAxis), Dot(Plane.XAxis, Base.YAxis) };
        Placement.YAxis  = { Dot(Plane.YAxis, Base.XAxis), Dot(Plane.YAxis, Base.YAxis) };
        Pending.emplace_back(Step.Label, PlaceSketchModel(SolvedSketch(Document, Step), Placement));
    }
    for (bool GrownCondition = true; GrownCondition;)
    {
        GrownCondition = false;
        for (auto Candidate = Pending.begin(); Candidate != Pending.end(); ++Candidate)
        {
            const double Join = 1.0e-9 * std::max(1.0, MeasureSketchScale(Candidate->second));
            const bool Touching = std::any_of(Members.begin(), Members.end(), [&](const SketchModel& Member) { return SketchContactCondition(Member, Candidate->second, std::max(Join, 1.0e-9 * MeasureSketchScale(Member))); });
            if (!Touching) continue;
            Group.push_back(Candidate->first);
            Members.push_back(Candidate->second);
            Pending.erase(Candidate);
            GrownCondition = true;
            break;
        }
    }
    // Tree order after the first (the join appends in that order).
    std::stable_sort(Group.begin() + 1, Group.end(), [&](const std::string& A, const std::string& B)
    {
        const auto Position = [&](const std::string& Label) { return std::find_if(Document.Tree.Steps.begin(), Document.Tree.Steps.end(), [&](const FeatureStep& Step) { return Step.Label == Label; }); };
        return Position(A) < Position(B);
    });
    return Group;
}

SketchOffsetOutcome PreviewSolidArcOffset(const SolidArcDocument& Document, const std::string& Sketch, double Distance)
{
    const FeatureStep* Found = FindStep(Document, Sketch);
    if (!Found || Found->Kind != FeatureStepKind::Sketch)
    {
        SketchOffsetOutcome Outcome;
        Outcome.Refusal = "no sketch step labelled " + Sketch;
        return Outcome;
    }
    return OffsetSketchCurves(SolvedSketch(Document, *Found), Distance);
}

SolidArcCommandOutcome OffsetSolidArcSketch(SolidArcDocument& Document, const std::string& Sketch, double Distance)
{
    return RunCommand(Document, "Offset " + SolidArcBodyCaption(Document, Sketch) + " by " + FormatAmount(Distance), false, [&]() -> std::string
    {
        const SketchOffsetOutcome Outcome = PreviewSolidArcOffset(Document, Sketch, Distance);
        if (!Outcome.Refusal.empty()) return Outcome.Refusal;
        const FeatureStep* Found = FindStep(Document, Sketch);
        FeatureStep Step;
        Step.Kind      = FeatureStepKind::Sketch;
        Step.Label     = "Sketch#" + std::to_string(NextStepOrdinal(Document.Tree));
        Step.Plane     = Found->Plane;
        Step.Elevation = Found->Elevation;
        Step.Sketch    = Outcome.Sketch;
        Document.Captions[Step.Label] = NextCaption(Document, "Offset");
        Document.Tree.Steps.push_back(std::move(Step));
        return {};
    });
}

SolidArcCommandOutcome PipeSolidArcPath(SolidArcDocument& Document, const std::string& Path, double Radius, FeatureCombination Combination, const std::string& Target)
{
    return RunCommand(Document, "Pipe r " + FormatAmount(Radius) + " along " + SolidArcBodyCaption(Document, Path), false, [&]() -> std::string
    {
        const FeatureStep* Found = FindStep(Document, Path);
        if (!Found || (Found->Kind != FeatureStepKind::Sketch && Found->Kind != FeatureStepKind::SpaceCurve)) return "choose the path: focus a sketch step or select a curve";
        FeatureStep Step;
        Step.Kind        = FeatureStepKind::Pipe;
        Step.Label       = "Pipe#" + std::to_string(NextStepOrdinal(Document.Tree));
        Step.Size        = FeatureValue{ Radius };
        Step.Combination = Combination;
        Step.Target      = Combination == FeatureCombination::Separate || Combination == FeatureCombination::New ? std::string{} : Target;
        if (Found->Kind == FeatureStepKind::Sketch) Step.Profile = Path;
        else Step.Sections = { Path };
        Document.Captions[Step.Label] = NextCaption(Document, "Pipe");
        Document.Tree.Steps.push_back(std::move(Step));
        Document.Selection = {};
        return {};
    });
}

SolidArcRevolveAxis ProposeSolidArcRevolveAxis(const SolidArcDocument& Document, const std::string& Sketch)
{
    SolidArcRevolveAxis Axis;
    const FeatureStep* Found = FindStep(Document, Sketch);
    if (!Found || Found->Kind != FeatureStepKind::Sketch) return Axis;
    const SketchModel& Model = SolvedSketch(Document, *Found);
    for (const SketchCurve& Curve : Model.Curves)
    {
        if (Curve.Kind == SketchCurveKind::Line && Curve.Construction) { Axis.Line = Curve.Identifier; return Axis; }
    }
    // Side test on the drawn geometry (curve samples, so bulging arcs and splines count).
    double LowX = 1e300, HighX = -1e300, LowY = 1e300, HighY = -1e300, Scale = 0.0;
    for (const SketchCurve& Curve : Model.Curves)
    {
        if (Curve.Construction) continue;
        for (const Vector2& Sample : SampleSketchCurve(Model, Curve))
        {
            LowX = std::min(LowX, Sample.X); HighX = std::max(HighX, Sample.X);
            LowY = std::min(LowY, Sample.Y); HighY = std::max(HighY, Sample.Y);
            Scale = std::max({ Scale, std::fabs(Sample.X), std::fabs(Sample.Y) });
        }
    }
    const double Slack = 1.0e-9 * std::max(Scale, 1.0);
    const bool ClearOfY = LowX >= -Slack || HighX <= Slack, ClearOfX = LowY >= -Slack || HighY <= Slack;
    if (!ClearOfY && ClearOfX) Axis.Direction = { 1.0, 0.0 };
    return Axis;
}

std::string DescribeSolidArcRevolveAxis(const SolidArcDocument& Document, const std::string& Sketch, const SolidArcRevolveAxis& Axis)
{
    if (Axis.Line >= 0)
    {
        const FeatureStep* Found = FindStep(Document, Sketch);
        if (Found)
        {
            const SketchModel& Model = SolvedSketch(Document, *Found);
            for (const SketchCurve& Curve : Model.Curves) if (Curve.Identifier == Axis.Line) return SketchCurveName(Curve);
        }
        return "line " + std::to_string(Axis.Line);
    }
    if (Axis.Origin.X == 0.0 && Axis.Origin.Y == 0.0)
    {
        if (Axis.Direction.X == 0.0 && Axis.Direction.Y != 0.0) return "sketch y-axis";
        if (Axis.Direction.Y == 0.0 && Axis.Direction.X != 0.0) return "sketch x-axis";
    }
    return "axis (" + FormatAmount(Axis.Origin.X) + ", " + FormatAmount(Axis.Origin.Y) + ") + t·(" + FormatAmount(Axis.Direction.X) + ", " + FormatAmount(Axis.Direction.Y) + ")";
}

namespace
{
    // Extrude's combination rules for the profile verbs: the first body is New, a second one needs a Boolean or New body.
    [[nodiscard]] std::string CombinationRefusal(const SolidArcDocument& Document, FeatureCombination Combination, bool FirstBodyCondition, const std::string& Target)
    {
        if (FirstBodyCondition && (Combination == FeatureCombination::Difference || Combination == FeatureCombination::Intersection))
            return std::string("nothing to ") + (Combination == FeatureCombination::Difference ? "subtract from" : "intersect with") + " yet (no body)";
        if (!FirstBodyCondition && Combination == FeatureCombination::New) return "a second body needs Union, Difference, Intersection or New body";
        if (!Target.empty() && !FindRegeneratedBody(Document, Target)) return "no body named " + Target;
        return {};
    }
}

SolidArcCommandOutcome RevolveSolidArcSketch(SolidArcDocument& Document, const std::string& Sketch, const SolidArcRevolveAxis& Axis, double Angle, FeatureCombination Combination,
                                             const std::string& Target)
{
    const bool        FirstBodyCondition = !Document.Summary.PresentCondition && Combination != FeatureCombination::Separate;
    const std::string Caption = NextCaption(Document, "Revolve");
    const std::string Label   = Caption + " · " + SolidArcBodyCaption(Document, Sketch) + " · " + FormatAmount(Angle * 180.0 / std::numbers::pi) + "° about " +
                                DescribeSolidArcRevolveAxis(Document, Sketch, Axis) + " · " + (FirstBodyCondition ? "New" : FeatureCombinationLabel(Combination));
    return RunCommand(Document, Label, false, [&]() -> std::string
    {
        const FeatureStep* Found = FindStep(Document, Sketch);
        if (!Found || Found->Kind != FeatureStepKind::Sketch) return "choose the profile: focus a sketch step";
        if (!(Angle > 1.0e-9) || Angle > 2.0 * std::numbers::pi + 1.0e-12) return "the angle must lie in (0°, 360°]";
        if (const std::string Refusal = CombinationRefusal(Document, Combination, FirstBodyCondition, Target); !Refusal.empty()) return Refusal;
        FeatureStep Step;
        Step.Kind           = FeatureStepKind::Revolve;
        Step.Label          = "Revolve#" + std::to_string(NextStepOrdinal(Document.Tree));
        Step.Profile        = Sketch;
        Step.AxisIdentifier = Axis.Line;
        if (Axis.Line < 0) { Step.AxisOrigin = Axis.Origin; Step.AxisDirection = Axis.Direction; }
        Step.Angle          = FeatureValue{ Angle };
        Step.Combination    = FirstBodyCondition ? FeatureCombination::New : Combination;
        Step.Target         = Target;
        Document.Captions[Step.Label] = Caption;
        Document.Tree.Steps.push_back(std::move(Step));
        return {};
    });
}

SolidArcCommandOutcome EvolveSolidArcProfile(SolidArcDocument& Document, const std::string& Profile, const std::string& Path, FeatureCombination Combination, const std::string& Target)
{
    const bool        FirstBodyCondition = !Document.Summary.PresentCondition && Combination != FeatureCombination::Separate;
    const std::string Caption = NextCaption(Document, "Evolution");
    const std::string Label   = Caption + " · " + SolidArcBodyCaption(Document, Profile) + " along " + (Path.empty() ? std::string("?") : SolidArcBodyCaption(Document, Path)) + " · " +
                                (FirstBodyCondition ? "New" : FeatureCombinationLabel(Combination));
    return RunCommand(Document, Label, false, [&]() -> std::string
    {
        const FeatureStep* Found = FindStep(Document, Profile);
        if (!Found || Found->Kind != FeatureStepKind::Sketch) return "choose the profile: focus a sketch step";
        const FeatureStep* Spine = FindStep(Document, Path);
        if (!Spine || (Spine->Kind != FeatureStepKind::Sketch && Spine->Kind != FeatureStepKind::SpaceCurve)) return "choose the path: click a one-curve sketch or select a curve";
        if (Path == Profile) return "the path must be another sketch than the profile";
        if (const std::string Refusal = CombinationRefusal(Document, Combination, FirstBodyCondition, Target); !Refusal.empty()) return Refusal;
        FeatureStep Step;
        Step.Kind        = FeatureStepKind::Evolution;
        Step.Label       = "Evolution#" + std::to_string(NextStepOrdinal(Document.Tree));
        Step.Profile     = Profile;
        Step.Path        = Path;
        Step.Combination = FirstBodyCondition ? FeatureCombination::New : Combination;
        Step.Target      = Target;
        Document.Captions[Step.Label] = Caption;
        Document.Tree.Steps.push_back(std::move(Step));
        Document.Selection = {};
        return {};
    });
}

std::string ProposeSolidArcEvolutionPath(const SolidArcDocument& Document, const std::string& Profile)
{
    if (!Document.Selection.Curves.empty()) return Document.Selection.Curves.front();
    for (auto Step = Document.Tree.Steps.rbegin(); Step != Document.Tree.Steps.rend(); ++Step)
    {
        if (Step->Kind != FeatureStepKind::Sketch || Step->Label == Profile) continue;
        const SketchModel& Model = SolvedSketch(Document, *Step);
        int Drawn = 0;
        bool Open = true;
        for (const SketchCurve& Curve : Model.Curves)
        {
            if (Curve.Construction) continue;
            ++Drawn;
            Open = Open && Curve.Kind != SketchCurveKind::Circle && !(Curve.Kind == SketchCurveKind::Spline && Curve.Points.size() > 1 && Curve.Points.front() == Curve.Points.back());
        }
        if (Drawn == 1 && Open) return Step->Label;
    }
    return {};
}

std::string SolidArcBodyOfName(const SolidArcDocument& Document, SolidArcSelectionKind Kind, const std::string& Name)
{
    // 💡 The two pieces of a cut carry the same names: the body the user picked on wins, then the active body.
    if (Kind != SolidArcSelectionKind::Body)
    {
        for (const std::string& Preferred : { Document.Selection.PickedBody, Document.Regeneration.Active })
        {
            if (Preferred.empty()) continue;
            for (const RegeneratedBody& Body : Document.Regeneration.Bodies)
            {
                if (Body.Key != Preferred) continue;
                const std::map<uint32_t, std::string>& Table = Kind == SolidArcSelectionKind::Edge ? Body.Naming.Edges : Body.Naming.Faces;
                for (const auto& Entry : Table) if (Entry.second == Name) return Body.Key;
            }
        }
    }
    for (const RegeneratedBody& Body : Document.Regeneration.Bodies)
    {
        if (Kind == SolidArcSelectionKind::Body) { if (Body.Key == Name) return Body.Key; continue; }
        const std::map<uint32_t, std::string>& Table = Kind == SolidArcSelectionKind::Edge ? Body.Naming.Edges : Body.Naming.Faces;
        for (const auto& Entry : Table) if (Entry.second == Name) return Body.Key;
    }
    return {};
}

std::string SolidArcBodyCaption(const SolidArcDocument& Document, const std::string& Key)
{
    const auto Found = Document.Captions.find(Key);
    return Found != Document.Captions.end() ? Found->second : Key;
}

void ClearSolidArcSelection(SolidArcDocument& Document)
{
    Document.Selection = {};
}

std::string DescribeSolidArcStep(const SolidArcDocument& Document, const FeatureStep& Step)
{
    const auto Found = Document.Captions.find(Step.Label);
    if (Found == Document.Captions.end()) return Step.Label;
    return Found->second + " · " + FeatureStepKindLabel(Step.Kind);
}

const char* SolidArcConstructLabel(SolidArcConstructKind Kind) noexcept
{
    switch (Kind)
    {
    case SolidArcConstructKind::Block:    return "Block";
    case SolidArcConstructKind::Cylinder: return "Cylinder";
    case SolidArcConstructKind::Sphere:   return "Sphere";
    case SolidArcConstructKind::Torus:    return "Torus";
    case SolidArcConstructKind::Cone:     return "Cone";
    case SolidArcConstructKind::Wedge:    return "Wedge";
    case SolidArcConstructKind::Prism:    return "Prism";
    case SolidArcConstructKind::Tube:     return "Tube";
    }
    return "Construct";
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    CONSTRAINTS (K)
//------------------------------------------------------------------------------------------------------------------------

namespace
{
    // 📝 Label prefix per kind; the rectangle tool's own H1 V1 H2 V2 share the scheme.
    [[nodiscard]] const char* ConstraintPrefix(SketchConstraintKind Constraint) noexcept
    {
        switch (Constraint)
        {
        case Kind::Coincident:         return "C";
        case Kind::Fixed:              return "F";
        case Kind::Horizontal:         return "H";
        case Kind::Vertical:           return "V";
        case Kind::Distance:
        case Kind::Length:             return "D";
        case Kind::HorizontalDistance: return "DX";
        case Kind::VerticalDistance:   return "DY";
        case Kind::Parallel:           return "L";
        case Kind::Perpendicular:      return "P";
        case Kind::Angle:              return "A";
        case Kind::EqualLength:
        case Kind::EqualRadius:        return "Q";
        case Kind::Radius:             return "R";
        case Kind::PointOnLine:
        case Kind::PointOnCircle:      return "O";
        case Kind::Tangent:            return "T";
        case Kind::Midpoint:           return "M";
        case Kind::Symmetric:          return "Y";
        case Kind::Concentric:         return "CC";
        case Kind::ArcRule:            return "AR";
        case Kind::SplineTangent:      return "T";
        case Kind::SplineCurvature:    return "K";                                          // κ: "G1" would read as tangency
        case Kind::PointOnSpline:      return "O";
        case Kind::SplineDirection:    return "E";
        }
        return "X";
    }

    // A spline end direction is labelled by its sense (H / V), the rest by kind.
    [[nodiscard]] std::string LabelPrefix(const SketchConstraint& Constraint)
    {
        if (Constraint.Kind == Kind::SplineDirection && std::fabs(Constraint.Value) < 1.0e-12) return "H";
        if (Constraint.Kind == Kind::SplineDirection && std::fabs(Constraint.Value - std::numbers::pi / 2.0) < 1.0e-12) return "V";
        return ConstraintPrefix(Constraint.Kind);
    }

    [[nodiscard]] std::string NextLabel(const SketchModel& Sketch, const std::string& Prefix)
    {
        int Highest = 0;
        for (const SketchConstraint& Existing : Sketch.Constraints)
        {
            if (Existing.Label.size() <= Prefix.size() || Existing.Label.compare(0, Prefix.size(), Prefix) != 0) continue;
            const std::string Digits = Existing.Label.substr(Prefix.size());
            if (std::all_of(Digits.begin(), Digits.end(), [](unsigned char Character) { return std::isdigit(Character) != 0; })) Highest = std::max(Highest, std::stoi(Digits));
        }
        return Prefix + std::to_string(Highest + 1);
    }

    // Index of the sketch step labelled Label (Steps.size(): none).
    [[nodiscard]] size_t SketchStepIndex(const SolidArcDocument& Document, const std::string& Label)
    {
        for (size_t Index = 0; Index < Document.Tree.Steps.size(); ++Index)
            if (Document.Tree.Steps[Index].Label == Label && Document.Tree.Steps[Index].Kind == FeatureStepKind::Sketch) return Index;
        return Document.Tree.Steps.size();
    }

    // The last replay's solve of sketch step Label when it comes before step Before (null Model otherwise).
    [[nodiscard]] LinkedSketchSource EarlierSketch(const SolidArcDocument& Document, size_t Before, const std::string& Label)
    {
        const size_t Index = SketchStepIndex(Document, Label);
        if (Index >= Before) return {};
        const auto Solved = Document.Regeneration.Sketches.find(Label);
        if (Solved == Document.Regeneration.Sketches.end()) return {};
        return { &Solved->second, SketchStepFrame(Document.Tree, Document.Tree.Steps[Index]) };
    }

    // 📝 Picks into Model (the sketch of step Target): the edited sketch's own pass through, picks in earlier sketches are
    //    linked in. A point becomes a linked point (one already linked to it is reused); a curve becomes a construction
    //    curve on linked points, carrying the link (a circle's radius is then held too). Circles and arcs need parallel
    //    planes (anything else projects to an ellipse); an arc seen from behind runs the other way. Empty: all linked.
    [[nodiscard]] std::string LinkPicks(const SolidArcDocument& Document, size_t Target, SketchModel& Model, const std::vector<SolidArcSketchPick>& Picks,
                                        std::vector<SketchPick>& Local, int& Appended)
    {
        const std::string&     Own   = Document.Tree.Steps[Target].Label;
        const OrthonormalFrame Plane = SketchStepFrame(Document.Tree, Document.Tree.Steps[Target]);
        for (const SolidArcSketchPick& Pick : Picks)
        {
            if (Pick.Sketch.empty() || Pick.Sketch == Own) { Local.push_back(Pick.Pick); continue; }
            const LinkedSketchSource Source = EarlierSketch(Document, Target, Pick.Sketch);
            if (!Source.Model)
            {
                const size_t Index = SketchStepIndex(Document, Pick.Sketch);
                if (Index == Document.Tree.Steps.size()) return "no sketch step " + Pick.Sketch;
                if (Index > Target) return Pick.Sketch + " comes after " + Own + " (link to earlier sketches only)";
                return Pick.Sketch + " has no solved geometry to link";
            }
            const SketchModel& From      = *Source.Model;
            const double       Alignment = Dot(Source.Plane.ZAxis, Plane.ZAxis);
            const auto Project = [&](int Index) -> Vector2
            {
                const Vector2 Position = From.Points[Index].Position;
                const Vector3 World    = Source.Plane.Origin + Source.Plane.XAxis * Position.X + Source.Plane.YAxis * Position.Y;
                return { Dot(World - Plane.Origin, Plane.XAxis), Dot(World - Plane.Origin, Plane.YAxis) };
            };
            const auto LinkPoint = [&](int Index) -> int
            {
                for (size_t Point = 0; Point < Model.Points.size(); ++Point)
                    if (Model.Points[Point].Link.Sketch == Pick.Sketch && Model.Points[Point].Link.Index == Index) return static_cast<int>(Point);
                const int Added = AppendSketchPoint(Model, Project(Index));
                Model.Points[Added].Link = { Pick.Sketch, Index };
                ++Appended;
                return Added;
            };
            const int Count = Pick.Pick.Kind == SketchPickKind::Point ? static_cast<int>(From.Points.size()) : static_cast<int>(From.Curves.size());
            if (Pick.Pick.Index < 0 || Pick.Pick.Index >= Count) return Pick.Sketch + " has no " + (Pick.Pick.Kind == SketchPickKind::Point ? "point " : "curve ") + std::to_string(Pick.Pick.Index);
            if (Pick.Pick.Kind == SketchPickKind::Point) { Local.push_back({ SketchPickKind::Point, LinkPoint(Pick.Pick.Index) }); continue; }
            int Reused = -1;
            for (size_t Index = 0; Index < Model.Curves.size(); ++Index)
                if (Model.Curves[Index].Link.Sketch == Pick.Sketch && Model.Curves[Index].Link.Index == Pick.Pick.Index) Reused = static_cast<int>(Index);
            if (Reused >= 0) { Local.push_back({ SketchPickKind::Curve, Reused }); continue; }
            const SketchCurve& Curve = From.Curves[Pick.Pick.Index];
            if ((Curve.Kind == SketchCurveKind::Circle || Curve.Kind == SketchCurveKind::Arc) && std::fabs(Alignment) < 1.0 - 1.0e-12)
                return SketchCurveName(Curve) + " of " + Pick.Sketch + " is not parallel to " + Own + " (it would project to an ellipse)";
            int Identifier = 1;
            for (const SketchCurve& Existing : Model.Curves) Identifier = std::max(Identifier, Existing.Identifier + 1);
            int Added = -1;
            switch (Curve.Kind)
            {
            case SketchCurveKind::Line:
            {
                const int Start = LinkPoint(Curve.Start);
                Added = AppendSketchLine(Model, Start, LinkPoint(Curve.End), Identifier, true);
                break;
            }
            case SketchCurveKind::Circle:
                Added = AppendSketchCircle(Model, LinkPoint(Curve.Centre), MeasureSketchRadius(From, Curve), Identifier, true);
                break;
            case SketchCurveKind::Arc:
            {
                const int Centre = LinkPoint(Curve.Centre);
                const int Start  = LinkPoint(Curve.Start);
                const int End    = LinkPoint(Curve.End);
                Added = Alignment > 0.0 ? AppendSketchArc(Model, Centre, Start, End, Identifier, true) : AppendSketchArc(Model, Centre, End, Start, Identifier, true);
                break;
            }
            case SketchCurveKind::Spline:
            {
                std::vector<int> Points;
                for (int Point : Curve.Points) Points.push_back(LinkPoint(Point));
                Added = AppendSketchSpline(Model, Points, Curve.Form, Identifier, true);
                Model.Curves[Added].Knots = Curve.Knots;
                break;
            }
            }
            Model.Curves[Added].Link = { Pick.Sketch, Pick.Pick.Index };
            ++Appended;
            Local.push_back({ SketchPickKind::Curve, Added });
        }
        return {};
    }

    [[nodiscard]] FeatureStep* FindSketchStep(SolidArcDocument& Document, const std::string& Label, size_t& Index)
    {
        for (Index = 0; Index < Document.Tree.Steps.size(); ++Index)
            if (Document.Tree.Steps[Index].Label == Label) return Document.Tree.Steps[Index].Kind == FeatureStepKind::Sketch ? &Document.Tree.Steps[Index] : nullptr;
        return nullptr;
    }

    // The step's sketch with every parameter-bound dimension set to its parameter's value and its links re-projected from
    // the last replay, as regeneration solves it.
    [[nodiscard]] SketchModel BindSketch(const SolidArcDocument& Document, const FeatureStep& Step)
    {
        SketchModel Bound = Step.Sketch;
        const size_t Target = SketchStepIndex(Document, Step.Label);
        if (Target < Document.Tree.Steps.size())
        {
            const std::vector<std::string> Notes = ProjectSketchLinks(Bound, SketchStepFrame(Document.Tree, Step), [&](const std::string& Label) { return EarlierSketch(Document, Target, Label); });
            static_cast<void>(Notes);                                                        // Dangling links stay held; the replay reports them
        }
        for (SketchConstraint& Constraint : Bound.Constraints)
        {
            if (Constraint.Parameter.empty()) continue;
            const double Value = EvaluateFeatureValue(Document.Tree, ParameterValue(Constraint.Parameter));
            if (!std::isnan(Value)) Constraint.Value = Value;
        }
        return Bound;
    }

    // Solved positions (points and circle radii) back into the authored sketch, with the reference dimensions' readings;
    // constraints stay as authored.
    void WriteBackPositions(FeatureStep& Step, const SketchModel& Solved)
    {
        for (size_t Point = 0; Point < Step.Sketch.Points.size() && Point < Solved.Points.size(); ++Point) Step.Sketch.Points[Point].Position = Solved.Points[Point].Position;
        for (size_t Curve = 0; Curve < Step.Sketch.Curves.size() && Curve < Solved.Curves.size(); ++Curve) Step.Sketch.Curves[Curve].Radius = Solved.Curves[Curve].Radius;
        for (size_t Constraint = 0; Constraint < Step.Sketch.Constraints.size() && Constraint < Solved.Constraints.size(); ++Constraint)
            if (Step.Sketch.Constraints[Constraint].DrivenCondition) Step.Sketch.Constraints[Constraint].Value = Solved.Constraints[Constraint].Value;
    }

    // Index of the G1 a G2 on the same spline joint supersedes (-1: none).
    [[nodiscard]] int SupersededTangent(const SketchModel& Sketch, const SketchConstraint& Curvature)
    {
        if (Curvature.Kind != Kind::SplineCurvature) return -1;
        for (size_t Index = 0; Index < Sketch.Constraints.size(); ++Index)
        {
            const SketchConstraint& Existing = Sketch.Constraints[Index];
            if (Existing.Kind != Kind::SplineTangent || Existing.Third != Curvature.Third) continue;
            const bool Same    = Existing.First == Curvature.First && Existing.Second == Curvature.Second;
            const bool Swapped = Existing.First == Curvature.Second && Existing.Second == Curvature.First;
            if (Same || Swapped) return static_cast<int>(Index);
        }
        return -1;
    }

    [[nodiscard]] std::string NameConstraint(const SketchModel& Sketch, int Index)
    {
        const SketchConstraint& Constraint = Sketch.Constraints[Index];
        const std::string Text = DescribeSketchConstraint(Sketch, Constraint);
        return Constraint.Label.empty() ? Text : Constraint.Label + " (" + Text + ")";
    }

    // 📝 Why a solve of Sketch is not acceptable, naming constraints; empty when it converged without conflicts. Watch:
    //    the index of the constraint just added (-1: none), refused also when merely redundant.
    [[nodiscard]] std::string SolveRefusal(const SketchModel& Sketch, const SketchSolveOutcome& Solve, int Watch)
    {
        if (Watch >= 0 && Watch < static_cast<int>(Solve.Constraints.size()))
        {
            const SketchConstraintReport& Report = Solve.Constraints[Watch];
            std::string Partners;
            for (int Partner : Report.Partners) if (Partner != Watch) Partners += (Partners.empty() ? "" : ", ") + NameConstraint(Sketch, Partner);
            const std::string Subject = DescribeSketchConstraint(Sketch, Sketch.Constraints[Watch]);
            if (Report.Status == SketchConstraintStatus::Unsupported) return Subject + ": its references do not fit the constraint";
            if (Report.Status == SketchConstraintStatus::Redundant) return Subject + " is already implied by " + (Partners.empty() ? std::string("the others") : Partners);
            if (Report.Status == SketchConstraintStatus::Conflicting) return Subject + " conflicts with " + (Partners.empty() ? std::string("the others") : Partners);
        }
        if (!Solve.ConflictGroups.empty())
        {
            std::string Group;
            for (int Member : Solve.ConflictGroups.front()) Group += (Group.empty() ? "" : ", ") + NameConstraint(Sketch, Member);
            return "conflicting constraints: " + Group;
        }
        if (!Solve.Converged) return "the sketch has no solution there (largest residual " + std::to_string(Solve.MaxResidual) + ")";
        return {};
    }

    [[nodiscard]] std::string SketchOwner(const SolidArcDocument& Document, const std::string& Label)
    {
        const auto Caption = Document.Captions.find(Label);
        return Caption != Document.Captions.end() ? Caption->second : Label;
    }
}

std::string NextSolidArcConstraintLabel(const SketchModel& Sketch, SketchConstraintKind Constraint)
{
    return NextLabel(Sketch, ConstraintPrefix(Constraint));
}

SolidArcLinkedPicks LinkSolidArcPicks(const SolidArcDocument& Document, const std::string& Sketch, const std::vector<SolidArcSketchPick>& Picks)
{
    SolidArcLinkedPicks Linked;
    const size_t Target = SketchStepIndex(Document, Sketch);
    if (Target == Document.Tree.Steps.size()) { Linked.Refusal = "no sketch step " + Sketch; return Linked; }
    Linked.Sketch  = BindSketch(Document, Document.Tree.Steps[Target]);
    static_cast<void>(SolveSketch(Linked.Sketch));                                           // Proposals measure what is shown
    Linked.Refusal = LinkPicks(Document, Target, Linked.Sketch, Picks, Linked.Picks, Linked.Appended);
    return Linked;
}

std::vector<std::string> SolidArcLinkSources(const SolidArcDocument& Document, const std::string& Sketch)
{
    std::vector<std::string> Sources;
    const size_t Target = SketchStepIndex(Document, Sketch);
    for (size_t Index = 0; Index < Target && Index < Document.Tree.Steps.size(); ++Index)
        if (Document.Tree.Steps[Index].Kind == FeatureStepKind::Sketch) Sources.push_back(Document.Tree.Steps[Index].Label);
    return Sources;
}

bool SolidArcLinkedCondition(const SketchModel& Sketch, SketchPick Pick) noexcept
{
    if (Pick.Kind == SketchPickKind::Point) return Pick.Index >= 0 && Pick.Index < static_cast<int>(Sketch.Points.size()) && !Sketch.Points[Pick.Index].Link.Sketch.empty();
    return Pick.Index >= 0 && Pick.Index < static_cast<int>(Sketch.Curves.size()) && !Sketch.Curves[Pick.Index].Link.Sketch.empty();
}

SolidArcSketchDiagnosis DiagnoseSolidArcSketch(const SolidArcDocument& Document, const std::string& Sketch)
{
    SolidArcSketchDiagnosis Diagnosis;
    for (const FeatureStep& Step : Document.Tree.Steps)
    {
        if (Step.Label != Sketch || Step.Kind != FeatureStepKind::Sketch) continue;
        Diagnosis.PresentCondition = true;
        Diagnosis.Sketch           = BindSketch(Document, Step);
        Diagnosis.Solve            = SolveSketch(Diagnosis.Sketch);
    }
    return Diagnosis;
}

SolidArcCommandOutcome ConstrainSolidArcSketch(SolidArcDocument& Document, const std::string& Sketch, const SketchConstraint& Constraint, const std::vector<SolidArcSketchPick>& Links)
{
    std::string Subject = SketchConstraintTitle(Constraint.Kind);
    const SolidArcLinkedPicks Preview = LinkSolidArcPicks(Document, Sketch, Links);
    const int Superseded = Preview.Refusal.empty() ? SupersededTangent(Preview.Sketch, Constraint) : -1;
    if (Preview.Refusal.empty()) Subject = DescribeSketchConstraint(Preview.Sketch, Constraint);
    if (Superseded >= 0) Subject += " (replaces " + NameConstraint(Preview.Sketch, Superseded) + ")";
    size_t Index = 0;
    FeatureStep* Found = FindSketchStep(Document, Sketch, Index);
    return RunCommand(Document, "Constrain " + SketchOwner(Document, Sketch) + ": " + Subject, false, [&]() -> std::string
    {
        if (!Found) return "no sketch step " + Sketch;
        if (!Preview.Refusal.empty()) return Preview.Refusal;
        std::vector<SketchPick> Local;
        int Appended = 0;
        if (std::string Refusal = LinkPicks(Document, Index, Found->Sketch, Links, Local, Appended); !Refusal.empty()) return Refusal;
        if (Superseded >= 0) Found->Sketch.Constraints.erase(Found->Sketch.Constraints.begin() + Superseded);
        SketchConstraint Added = Constraint;
        if (Added.Label.empty()) Added.Label = NextLabel(Found->Sketch, LabelPrefix(Added));
        Found->Sketch.Constraints.push_back(Added);
        SketchModel Solved = BindSketch(Document, *Found);
        const SketchSolveOutcome Solve = SolveSketch(Solved);
        if (std::string Refusal = SolveRefusal(Solved, Solve, static_cast<int>(Solved.Constraints.size()) - 1); !Refusal.empty()) return Refusal;
        WriteBackPositions(*Found, Solved);
        return {};
    }, Index);
}

SolidArcCommandOutcome RemoveSolidArcConstraint(SolidArcDocument& Document, const std::string& Sketch, int Constraint)
{
    size_t Index = 0;
    FeatureStep* Found = FindSketchStep(Document, Sketch, Index);
    const std::string Named = Found && Constraint >= 0 && Constraint < static_cast<int>(Found->Sketch.Constraints.size()) ? NameConstraint(Found->Sketch, Constraint) : std::string("constraint");
    return RunCommand(Document, "Remove " + Named + " from " + SketchOwner(Document, Sketch), false, [&]() -> std::string
    {
        if (!Found) return "no sketch step " + Sketch;
        if (Constraint < 0 || Constraint >= static_cast<int>(Found->Sketch.Constraints.size())) return "the sketch has no constraint " + std::to_string(Constraint);
        Found->Sketch.Constraints.erase(Found->Sketch.Constraints.begin() + Constraint);
        return {};
    }, Index);
}

SolidArcCommandOutcome ChangeSolidArcDimension(SolidArcDocument& Document, const std::string& Sketch, int Constraint, double Value)
{
    size_t Index = 0;
    FeatureStep* Found = FindSketchStep(Document, Sketch, Index);
    const bool  Valid = Found && Constraint >= 0 && Constraint < static_cast<int>(Found->Sketch.Constraints.size());
    const bool  Angular = Valid && Found->Sketch.Constraints[Constraint].Kind == Kind::Angle;
    char Shown[48];
    std::snprintf(Shown, sizeof(Shown), Angular ? "%.6g°" : "%.6g", Angular ? Value * 180.0 / std::numbers::pi : Value);
    const std::string Named = Valid ? (Found->Sketch.Constraints[Constraint].Label.empty() ? std::string(SketchConstraintTitle(Found->Sketch.Constraints[Constraint].Kind)) : Found->Sketch.Constraints[Constraint].Label) : std::string("dimension");
    return RunCommand(Document, Named + " = " + Shown + " in " + SketchOwner(Document, Sketch), false, [&]() -> std::string
    {
        if (!Found) return "no sketch step " + Sketch;
        if (!Valid) return "the sketch has no constraint " + std::to_string(Constraint);
        SketchConstraint& Dimension = Found->Sketch.Constraints[Constraint];
        if (!SketchDimensionCondition(Dimension.Kind)) return DescribeSketchConstraint(Found->Sketch, Dimension) + " has no value to change";
        if (Dimension.DrivenCondition) return Named + " is a reference dimension (E makes it driving)";
        if (!std::isfinite(Value)) return "the value is not a number";
        const bool Positive = Dimension.Kind == Kind::Distance || Dimension.Kind == Kind::Length || Dimension.Kind == Kind::Radius;
        if (Positive && !(Value > 0.0)) return std::string(SketchConstraintTitle(Dimension.Kind)) + " must be positive";
        if (!Dimension.Parameter.empty())
        {
            if (!AssignFeatureParameter(Document.Tree, Dimension.Parameter, Value)) return "unknown parameter " + Dimension.Parameter;
        }
        else Dimension.Value = Value;
        SketchModel Solved = BindSketch(Document, *Found);
        const SketchSolveOutcome Solve = SolveSketch(Solved);
        if (std::string Refusal = SolveRefusal(Solved, Solve, -1); !Refusal.empty()) return Refusal;
        WriteBackPositions(*Found, Solved);
        return {};
    }, Index);
}

SolidArcCommandOutcome ToggleSolidArcReference(SolidArcDocument& Document, const std::string& Sketch, int Constraint)
{
    size_t Index = 0;
    FeatureStep* Found = FindSketchStep(Document, Sketch, Index);
    const bool Valid    = Found && Constraint >= 0 && Constraint < static_cast<int>(Found->Sketch.Constraints.size());
    const bool ToDriven = Valid && !Found->Sketch.Constraints[Constraint].DrivenCondition;
    const std::string Named = Valid ? NameConstraint(Found->Sketch, Constraint) : std::string("dimension");
    return RunCommand(Document, std::string(ToDriven ? "Reference " : "Drive ") + Named + " in " + SketchOwner(Document, Sketch), false, [&]() -> std::string
    {
        if (!Found) return "no sketch step " + Sketch;
        if (!Valid) return "the sketch has no constraint " + std::to_string(Constraint);
        SketchConstraint& Dimension = Found->Sketch.Constraints[Constraint];
        if (!SketchDimensionCondition(Dimension.Kind)) return Named + " has no value (only dimensions can be references)";
        if (ToDriven && !Dimension.Parameter.empty()) return Named + " is bound to " + Dimension.Parameter + " (it stays driving)";
        if (ToDriven)
        {
            Dimension.DrivenCondition = true;
            SketchModel Solved = BindSketch(Document, *Found);
            static_cast<void>(SolveSketch(Solved));
            WriteBackPositions(*Found, Solved);
            return {};
        }
        SketchModel Reading = BindSketch(Document, *Found);                                  // The current reading, so nothing moves
        static_cast<void>(SolveSketch(Reading));
        Dimension.DrivenCondition = false;
        Dimension.Value           = Reading.Constraints[Constraint].Value;
        SketchModel Solved = BindSketch(Document, *Found);
        const SketchSolveOutcome Solve = SolveSketch(Solved);
        if (std::string Refusal = SolveRefusal(Solved, Solve, Constraint); !Refusal.empty()) return Refusal + " (it stays a reference)";
        WriteBackPositions(*Found, Solved);
        return {};
    }, Index);
}

SolidArcCommandOutcome UnlinkSolidArcGeometry(SolidArcDocument& Document, const std::string& Sketch, SketchPick Pick)
{
    size_t Index = 0;
    FeatureStep* Found = FindSketchStep(Document, Sketch, Index);
    const std::string Named = Found ? NameSketchPick(Found->Sketch, Pick) : std::string("geometry");
    return RunCommand(Document, "Unlink " + Named + " of " + SketchOwner(Document, Sketch), false, [&]() -> std::string
    {
        if (!Found) return "no sketch step " + Sketch;
        if (!SolidArcLinkedCondition(Found->Sketch, Pick)) return Named + " is not linked";
        SketchModel Projected = BindSketch(Document, *Found);                                // Where it is shown now, so it stays there
        static_cast<void>(SolveSketch(Projected));
        WriteBackPositions(*Found, Projected);
        SketchModel& Model = Found->Sketch;
        const auto Free = [&](int Point) { if (Point >= 0 && Point < static_cast<int>(Model.Points.size())) Model.Points[Point].Link = {}; };
        if (Pick.Kind == SketchPickKind::Point)
        {
            Free(Pick.Index);
            for (SketchCurve& Curve : Model.Curves) if (Curve.Kind == SketchCurveKind::Circle && Curve.Centre == Pick.Index) Curve.Link = {};
            return {};
        }
        SketchCurve& Curve = Model.Curves[Pick.Index];
        Curve.Link = {};
        Free(Curve.Start);
        Free(Curve.End);
        Free(Curve.Centre);
        for (int Point : Curve.Points) Free(Point);
        return {};
    }, Index);
}

SketchDragOutcome PreviewSolidArcDrag(const SolidArcDocument& Document, const std::string& Sketch, int Point, Vector2 Target)
{
    for (const FeatureStep& Step : Document.Tree.Steps)
        if (Step.Label == Sketch && Step.Kind == FeatureStepKind::Sketch) return DragSketchPoint(BindSketch(Document, Step), Point, Target);
    SketchDragOutcome Missing;
    Missing.Refusal = "Drag: no sketch step " + Sketch;
    return Missing;
}

SolidArcCommandOutcome DragSolidArcSketchPoint(SolidArcDocument& Document, const std::string& Sketch, int Point, Vector2 Target)
{
    size_t Index = 0;
    FeatureStep* Found = FindSketchStep(Document, Sketch, Index);
    const std::string Named = Found ? NameSketchPick(Found->Sketch, { SketchPickKind::Point, Point }) : std::string("point");
    return RunCommand(Document, "Drag " + Named + " of " + SketchOwner(Document, Sketch), false, [&]() -> std::string
    {
        if (!Found) return "no sketch step " + Sketch;
        const SketchDragOutcome Drag = PreviewSolidArcDrag(Document, Sketch, Point, Target);
        if (!Drag.Refusal.empty()) return Drag.Refusal;
        if (!(Drag.Travel > 1.0e-12 * std::max(1.0, Drag.Solve.Scale))) return Named + " is fully constrained (it cannot move)";
        WriteBackPositions(*Found, Drag.Sketch);
        return {};
    }, Index);
}

SolidArcCommandOutcome DragSolidArcSketchPoints(SolidArcDocument& Document, const std::string& Sketch, const std::vector<int>& Points, const std::vector<Vector2>& Targets)
{
    if (Points.size() == 1 && Targets.size() == 1) return DragSolidArcSketchPoint(Document, Sketch, Points[0], Targets[0]);
    size_t Index = 0;
    FeatureStep* Found = FindSketchStep(Document, Sketch, Index);
    return RunCommand(Document, "Drag " + std::to_string(Points.size()) + " points of " + SketchOwner(Document, Sketch), false, [&]() -> std::string
    {
        if (!Found) return "no sketch step " + Sketch;
        if (Points.empty() || Points.size() != Targets.size()) return "Drag: points and targets do not pair up";
        SketchModel Working = {};
        for (const FeatureStep& Step : Document.Tree.Steps)
            if (Step.Label == Sketch && Step.Kind == FeatureStepKind::Sketch) Working = BindSketch(Document, Step);
        for (int Point : Points)
            if (Point < 0 || Point >= int(Working.Points.size())) return "Drag: no point " + std::to_string(Point) + " in " + Sketch;
        const std::vector<SketchPoint> Start = Working.Points;
        double Travel = 0.0;
        for (int Sweep = 0; Sweep < 6; ++Sweep)
        {
            double Miss = 0.0;
            for (size_t Pick = 0; Pick < Points.size(); ++Pick)
            {
                const SketchDragOutcome Drag = DragSketchPoint(Working, Points[Pick], Targets[Pick]);
                if (!Drag.Refusal.empty()) return Drag.Refusal;
                Working = Drag.Sketch;
            }
            for (size_t Pick = 0; Pick < Points.size(); ++Pick) Miss = std::max(Miss, Distance(Working.Points[Points[Pick]].Position, Targets[Pick]));
            if (Miss <= 1.0e-9) break;
        }
        for (size_t Point = 0; Point < Start.size(); ++Point) Travel = std::max(Travel, Distance(Start[Point].Position, Working.Points[Point].Position));
        if (!(Travel > 1.0e-12)) return "the points are fully constrained (they cannot move)";
        WriteBackPositions(*Found, Working);
        return {};
    }, Index);
}

const char* SolidArcDrawingLabel(SolidArcDrawingKind Kind) noexcept
{
    switch (Kind)
    {
    case SolidArcDrawingKind::Line:      return "Line";
    case SolidArcDrawingKind::Rectangle: return "Rectangle";
    case SolidArcDrawingKind::Circle:    return "Circle";
    case SolidArcDrawingKind::Arc:       return "Arc";
    case SolidArcDrawingKind::Slot:      return "Slot";
    case SolidArcDrawingKind::Curve:     return "Curve";
    case SolidArcDrawingKind::Polygon:   return "Polygon";
    }
    return "Drawing";
}

const char* SolidArcFaceOperationLabel(FaceOperationKind Operation) noexcept
{
    switch (Operation)
    {
    case FaceOperationKind::Offset:     return "Offset";
    case FaceOperationKind::Push:       return "Push";
    case FaceOperationKind::Tweak:      return "Tweak";
    case FaceOperationKind::Scale:      return "Scale";
    case FaceOperationKind::Rotate:     return "Rotate";
    case FaceOperationKind::Draft:      return "Draft";
    case FaceOperationKind::Extend:     return "Extend";
    case FaceOperationKind::Trim:       return "Trim";
    case FaceOperationKind::OffsetBody: return "OffsetBody";
    case FaceOperationKind::Hollow:     return "Hollow";
    case FaceOperationKind::Extrude:    return "ExtrudeFaces";
    case FaceOperationKind::Delete:     return "DeleteFaces";
    case FaceOperationKind::Heal:       return "Heal";
    case FaceOperationKind::Thicken:    return "Thicken";
    case FaceOperationKind::Inset:      return "Inset";
    }
    return "Change";
}

} // namespace Frontier::SolidArc
