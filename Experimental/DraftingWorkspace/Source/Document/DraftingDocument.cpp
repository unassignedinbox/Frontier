/*====================================================================================================================================
                                                      DRAFTINGDOCUMENT.CPP
====================================================================================================================================*/
// 🧩 DraftingWorkspace document: construct tiles as sketch + sweep pairs, atomic commands, undo / redo, selection upkeep

//------------------------------------------------------------------------------------------------------------------------
//                                                    INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Document/DraftingDocument.h"
#include "Document/VehicleCurveImport.h"

#include "Audit/EnclosedVolume.h"
#include "Audit/ModelAudit.h"
#include "Blend/RollingBallGeometry.h"
#include "Sketch/SketchCatalogue.h"
#include "Sketch/SketchSolver.h"
#include "Sketch/SketchSpline.h"
#include "Surfacing/CurveNetwork.h"

#include <algorithm>
#include <cctype>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <numbers>
#include <limits>
#include <set>

namespace Frontier::DraftingWorkspace
{

using namespace Frontier::SolidArcRemastered;

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
[[nodiscard]] std::string NextCaption(const DraftingDocument& Document, const std::string& Stem)
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
std::string AuthorTile(DraftingDocument& Document, const DraftingConstructRequest& Request, FeatureCombination Combination)
{
    FeatureTree&      Tree    = Document.Tree;
    const char*       Stem    = DraftingConstructLabel(Request.Kind);
    const std::string Caption = NextCaption(Document, Stem);
    const int         Ordinal = NextStepOrdinal(Tree);
    const std::string SketchLabel = "Sketch#" + std::to_string(Ordinal);
    TileAuthoring     Author{ Tree, Caption };
    const Vector3     Spot    = Request.Position;

    switch (Request.Kind)
    {
    case DraftingConstructKind::Block:
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
    case DraftingConstructKind::Cylinder:
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
        const Modeller::Vector2 Place = Axis == 0 ? Modeller::Vector2{ Spot.Y, Spot.Z } : Axis == 1 ? Modeller::Vector2{ Spot.X, Spot.Z } : Modeller::Vector2{ Spot.X, Spot.Y };
        FeatureStep& Section = Author.Step(FeatureStepKind::Sketch, SketchLabel);
        Section.Plane        = Axis == 0 ? DraftingSketchPlane(2) : Axis == 1 ? UprightPlane() : FloorPlane();
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
    case DraftingConstructKind::Sphere:
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
    case DraftingConstructKind::Torus:
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
    }
    Document.Captions[SketchLabel]            = Caption;
    Document.Captions[Tree.Steps.back().Label] = Caption;
    return Caption;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    COMMAND FRAME
//------------------------------------------------------------------------------------------------------------------------

[[nodiscard]] DraftingHistoryEntry Snapshot(const DraftingDocument& Document, const std::string& Label)
{
    return { Document.Tree, Document.Captions, Label, Document.Appearance, false };
}

// 📝 A matcap-only entry swaps the appearance back and keeps the body (and its Revision): no regeneration.
void Restore(DraftingDocument& Document, DraftingHistoryEntry Entry)
{
    Document.Appearance = std::move(Entry.Appearance);
    if (Entry.AppearanceCondition) return;
    Document.Tree     = std::move(Entry.Tree);
    Document.Captions = std::move(Entry.Captions);
    RegenerateDraftingDocument(Document);
}

// ① snapshot (flagged matcap-only) ② change the appearance (the callback returns a refusal or "") ③ keep or roll back.
template <typename Change>
DraftingCommandOutcome RunAppearanceCommand(DraftingDocument& Document, const std::string& Label, Change&& Apply)
{
    const auto           Start  = std::chrono::steady_clock::now();
    DraftingHistoryEntry Before = Snapshot(Document, Label);
    Before.AppearanceCondition  = true;
    DraftingCommandOutcome Outcome;
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
    Outcome.AcceptedCondition = true;
    Outcome.Milliseconds      = ElapsedMs(Start);
    return Outcome;
}

// 📝 First refusal among the steps at or after First that did not already refuse before the command (RefusedBefore:
//    labels; the steps a command appends are never in it).
[[nodiscard]] std::string RefusalFrom(const DraftingDocument& Document, size_t First, const std::set<std::string>& RefusedBefore)
{
    for (size_t Index = First; Index < Document.Regeneration.Steps.size(); ++Index)
    {
        const FeatureStepReport& Report = Document.Regeneration.Steps[Index];
        if (Report.Status == FeatureStepStatus::Refused && !RefusedBefore.contains(Report.Label)) return Report.Label + ": " + Report.Refusal;
    }
    return {};
}

[[nodiscard]] std::set<std::string> RefusedLabels(const DraftingDocument& Document)
{
    std::set<std::string> Labels;
    for (const FeatureStepReport& Report : Document.Regeneration.Steps) if (Report.Status == FeatureStepStatus::Refused) Labels.insert(Report.Label);
    return Labels;
}

// ① snapshot ② change (the callback returns a refusal or "") ③ regenerate ④ roll back when a new step refused, or (edits
//    of existing steps: WatchFrom) a step at or after WatchFrom that regenerated before now refuses.
template <typename Change>
DraftingCommandOutcome RunCommand(DraftingDocument& Document, const std::string& Label, bool CaptureCondition, Change&& Apply,
                                  size_t WatchFrom = std::numeric_limits<size_t>::max())
{
    const auto                  Start         = std::chrono::steady_clock::now();
    DraftingHistoryEntry        Before        = Snapshot(Document, Label);
    const size_t                FirstNew      = std::min(Document.Tree.Steps.size(), WatchFrom);
    const std::set<std::string> RefusedBefore = RefusedLabels(Document);
    DraftingCommandOutcome Outcome;
    Outcome.Label = Label;
    Outcome.Refusal = Apply();
    if (Outcome.Refusal.empty())
    {
        RegenerateDraftingDocument(Document, CaptureCondition);
        Outcome.Refusal = RefusalFrom(Document, FirstNew, RefusedBefore);
    }
    if (!Outcome.Refusal.empty())
    {
        Restore(Document, std::move(Before));
        Outcome.Milliseconds = ElapsedMs(Start);
        return Outcome;
    }
    Document.UndoEntries.push_back(std::move(Before));
    Document.RedoEntries.clear();
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

[[nodiscard]] const RegeneratedBody* FindRegeneratedBody(const DraftingDocument& Document, const std::string& Key)
{
    for (const RegeneratedBody& Body : Document.Regeneration.Bodies) if (Body.Key == Key) return &Body;
    return nullptr;
}

// 📝 The one body carrying every name (faces or edges); refusal text when they span bodies or none carries one.
[[nodiscard]] std::string CommonBody(const DraftingDocument& Document, DraftingSelectionKind Kind, const std::vector<std::string>& Names, std::string& Key)
{
    Key.clear();
    for (const std::string& Name : Names)
    {
        const std::string Owner = DraftingBodyOfName(Document, Kind, Name);
        if (Owner.empty()) return (Kind == DraftingSelectionKind::Edge ? "edge " : "face ") + Name + " is on no body";
        if (!Key.empty() && Owner != Key)
            return "the selection spans " + DraftingBodyCaption(Document, Key) + " and " + DraftingBodyCaption(Document, Owner) + " (one body at a time)";
        Key = Owner;
    }
    return {};
}

} // namespace

//------------------------------------------------------------------------------------------------------------------------
//                                                    PUBLIC FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

DraftingDocument CreateDraftingDocument(const std::string& Title)
{
    DraftingDocument Document;
    Document.Title       = Title;
    Document.Tree.Name   = Title;
    Document.Partition   = std::make_unique<ModelPartition>();
    return Document;
}

void RegenerateDraftingDocument(DraftingDocument& Document, bool CaptureCondition)
{
    Document.Partition    = std::make_unique<ModelPartition>();
    Document.Regeneration = RegenerateFeatureTree(*Document.Partition, Document.Tree, CaptureCondition);
    // 📝 One counter for every document in the process, so a staged preview never shares a scene key with its source.
    static std::atomic<uint64_t> RevisionCounter{ 0 };
    Document.Revision     = ++RevisionCounter;

    DraftingBodySummary& Summary = Document.Summary;
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
        DraftingBodyFigures Figures;
        Figures.Key            = Regenerated.Key;
        Figures.Caption        = DraftingBodyCaption(Document, Regenerated.Key);
        Figures.SoundCondition = Audit.SoundCondition();
        Figures.SheetCondition = Document.Partition->Bodies.Resolve(Body).Category == BodyCategory::Sheet;
        Figures.Faces          = static_cast<int>(Inventory.Faces.size());
        Figures.Edges          = static_cast<int>(Inventory.Edges.size());
        Figures.Vertices       = static_cast<int>(Inventory.Vertices.size());
        Figures.Genus          = Audit.Genus;
        Figures.Volume         = Figures.SheetCondition ? 0.0 : IntegrateBodyVolume(*Document.Partition, Body);
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
    KeepResolvedNames(Document.Selection.Faces, Document.Regeneration.Bodies, false);
    KeepResolvedNames(Document.Selection.Edges, Document.Regeneration.Bodies, true);
    std::erase_if(Document.Selection.Bodies, [&](const std::string& Key) { return !FindRegeneratedBody(Document, Key); });
    std::erase_if(Document.Selection.Curves, [&](const std::string& Label) { return !Document.Regeneration.Curves.contains(Label); });
}

DraftingDocument CloneDraftingDocument(const DraftingDocument& Source)
{
    DraftingDocument Copy = CreateDraftingDocument(Source.Title);
    Copy.Tree      = Source.Tree;
    Copy.Captions  = Source.Captions;
    Copy.Selection = Source.Selection;
    Copy.Appearance = Source.Appearance;
    RegenerateDraftingDocument(Copy);
    return Copy;
}

DraftingCommandOutcome ConstructDraftingPrimitive(DraftingDocument& Document, const DraftingConstructRequest& Request)
{
    const bool        FirstBodyCondition = !Document.Summary.PresentCondition;
    const std::string Label = NextCaption(Document, DraftingConstructLabel(Request.Kind)) + " · " +
                              (FirstBodyCondition && Request.Combination != FeatureCombination::Separate ? "New" : FeatureCombinationLabel(Request.Combination));
    return RunCommand(Document, Label, false, [&]() -> std::string
    {
        if (Request.Width <= 0.0 || Request.Depth <= 0.0 || Request.Height <= 0.0 || Request.Radius <= 0.0 ||
            Request.Minor <= 0.0 || Request.Major <= Request.Minor)
            return "every dimension must be positive (torus: Major > Minor)";
        if (!FirstBodyCondition && Request.Combination == FeatureCombination::New)
            return "a second body needs Union, Difference, Intersection or New body";
        if (!Request.Target.empty() && !FindRegeneratedBody(Document, Request.Target)) return "no body named " + Request.Target;
        AuthorTile(Document, Request, FirstBodyCondition && Request.Combination != FeatureCombination::Separate ? FeatureCombination::New : Request.Combination);
        return {};
    });
}

DraftingCommandOutcome BlendDraftingSelection(DraftingDocument& Document, BlendProfile Shape, double Size)
{
    DraftingBlendRequest Request;
    Request.Shape = Shape;
    Request.Size  = Size;
    return BlendDraftingSelection(Document, Request);
}

DraftingCommandOutcome BlendDraftingSelection(DraftingDocument& Document, const DraftingBlendRequest& Request)
{
    const bool        VariableCondition = Request.EndSize.has_value();
    const std::string Stem  = Request.Shape == BlendProfile::Fillet ? "Fillet" : "Chamfer";
    const std::string Sizes = FormatAmount(Request.Size) + (VariableCondition ? " → " + FormatAmount(*Request.EndSize) : std::string());
    const std::string Label = Stem + " " + Sizes + " on " + std::to_string(Document.Selection.Edges.size()) + " edge(s)";
    return RunCommand(Document, Label, true, [&]() -> std::string
    {
        if (Document.Selection.Edges.empty()) return "select one or more edges first";
        if (Request.Size <= 0.0 || (VariableCondition && *Request.EndSize <= 0.0)) return "the blend size must be positive";
        std::string Owner;
        if (const std::string Problem = CommonBody(Document, DraftingSelectionKind::Edge, Document.Selection.Edges, Owner); !Problem.empty()) return Problem;
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

        // ② Parameters and the step.
        const std::string Caption   = NextCaption(Document, Stem);
        const std::string StepLabel = "Blend#" + std::to_string(NextStepOrdinal(Document.Tree));
        const double      Larger    = std::max(Request.Size, Request.EndSize.value_or(Request.Size));
        const char*       Meaning   = Request.Shape == BlendProfile::Fillet ? "fillet radius" : "chamfer setback";
        Document.Tree.Parameters.push_back({ Caption + ".Size", Request.Size, 0.0, std::max(1.0, 4.0 * Larger),
                                             VariableCondition ? std::string(Meaning) + " at the anchor corners" : std::string(Meaning) });
        FeatureStep Step;
        Step.Kind  = FeatureStepKind::Blend;
        Step.Label = StepLabel;
        Step.Shape  = Request.Shape;
        Step.Size   = ParameterValue(Caption + ".Size");
        Step.Target = Owner;
        for (const std::string& Name : Document.Selection.Edges) Step.References.push_back({ ReferenceKind::Edge, Name, {} });
        if (VariableCondition)
        {
            Document.Tree.Parameters.push_back({ Caption + ".EndSize", *Request.EndSize, 0.0, std::max(1.0, 4.0 * Larger), std::string(Meaning) + " at the far corners" });
            Step.EndSize     = ParameterValue(Caption + ".EndSize");
            Step.SizeAnchors = std::move(Anchors);
        }
        Document.Tree.Steps.push_back(std::move(Step));
        Document.Captions[StepLabel] = Caption;
        return {};
    });
}

DraftingCommandOutcome ChangeDraftingSelection(DraftingDocument& Document, FaceOperationKind Operation, double Amount)
{
    const std::string Stem  = DraftingFaceOperationLabel(Operation);
    const std::string Label = Stem + " " + FormatAmount(Amount) + " on " + std::to_string(Document.Selection.Faces.size()) + " face(s)";
    return RunCommand(Document, Label, true, [&]() -> std::string
    {
        const bool WholeBodyCondition = Operation == FaceOperationKind::OffsetBody || Operation == FaceOperationKind::Heal;
        if (Document.Selection.Faces.empty() && !WholeBodyCondition) return "select one or more faces first";
        if (!Document.Summary.PresentCondition) return "there is no body to change";
        std::string Owner;
        if (const std::string Problem = CommonBody(Document, DraftingSelectionKind::Face, Document.Selection.Faces, Owner); !Problem.empty()) return Problem;
        if (Owner.empty() && !Document.Selection.Bodies.empty()) Owner = Document.Selection.Bodies.front();   // Whole-body verbs on a picked body
        const std::string Caption = NextCaption(Document, Stem);
        const std::string StepLabel = "Change#" + std::to_string(NextStepOrdinal(Document.Tree));
        const double      Span = std::max(1.0, 4.0 * std::abs(Amount));
        Document.Tree.Parameters.push_back({ Caption + ".Amount", Amount, std::min(-Span, Amount), std::max(Span, Amount),
                                             Operation == FaceOperationKind::Draft ? "draft angle [rad]" : "distance" });
        FeatureStep Step;
        Step.Kind      = FeatureStepKind::FaceOperation;
        Step.Label     = StepLabel;
        Step.Operation = Operation;
        Step.Amount    = ParameterValue(Caption + ".Amount");
        Step.Target    = Owner;
        for (const std::string& Name : Document.Selection.Faces) Step.References.push_back({ ReferenceKind::Face, Name, {} });
        Document.Tree.Steps.push_back(std::move(Step));
        Document.Captions[StepLabel] = Caption;
        return {};
    });
}

DraftingCommandOutcome AssignDraftingParameter(DraftingDocument& Document, const std::string& Name, double Value)
{
    return RunCommand(Document, Name + " = " + FormatAmount(Value), false, [&]() -> std::string
    {
        if (!AssignFeatureParameter(Document.Tree, Name, Value)) return "no parameter named " + Name;
        return {};
    });
}

DraftingCommandOutcome DeleteDraftingStep(DraftingDocument& Document, const std::string& Label)
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

DraftingCommandOutcome ReplaceDraftingTree(DraftingDocument& Document, FeatureTree Tree, std::map<std::string, std::string> Captions,
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

std::string ComposeDraftingSketch(const DraftingDrawingRequest& Request, SketchModel& Sketch)
{
    Sketch = {};
    const std::vector<Vector2>& Points = Request.Points;
    const char*  Tool    = DraftingDrawingLabel(Request.Kind);
    double       Extent  = 0.0;                                                                // [len] - Click spread
    for (const Vector2& First : Points) for (const Vector2& Second : Points) Extent = std::max(Extent, Distance(First, Second));
    const double Join    = 1.0e-9 * std::max(Extent, 1.0);                                     // [len] - Coincidence distance
    auto Needs = [&](size_t Count) -> std::string
    {
        return Points.size() < Count ? std::string(Tool) + " needs " + std::to_string(Count) + " points" : std::string{};
    };
    switch (Request.Kind)
    {
    case DraftingDrawingKind::Line:
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
    case DraftingDrawingKind::Rectangle:
    {
        if (std::string Refusal = Needs(2); !Refusal.empty()) return Refusal;
        const Vector2 First  = Request.CentreCondition ? Points[0] * 2.0 - Points[1] : Points[0];
        const Vector2 Second = Points[1];
        const Vector2 Low{ std::min(First.X, Second.X), std::min(First.Y, Second.Y) }, High{ std::max(First.X, Second.X), std::max(First.Y, Second.Y) };
        if (High.X - Low.X <= Join || High.Y - Low.Y <= Join) return "Rectangle has no width or no height";
        AppendSketchRectangle(Sketch, Low, High, 1);
        return {};
    }
    case DraftingDrawingKind::Circle:
    {
        if (std::string Refusal = Needs(2); !Refusal.empty()) return Refusal;
        const Vector2 Centre = Request.CentreCondition ? Points[0] : (Points[0] + Points[1]) * 0.5;
        const double  Radius = Request.CentreCondition ? Distance(Points[0], Points[1]) : 0.5 * Distance(Points[0], Points[1]);
        if (Radius <= Join) return "Circle has no radius";
        AppendSketchCircle(Sketch, AppendSketchPoint(Sketch, Centre), Radius, 1);
        return {};
    }
    case DraftingDrawingKind::Arc:
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
    case DraftingDrawingKind::Slot:
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
    case DraftingDrawingKind::Curve:
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

DraftingCommandOutcome DrawDraftingSketch(DraftingDocument& Document, const DraftingDrawingRequest& Request)
{
    const std::string Stem    = Request.Kind == DraftingDrawingKind::Curve ? std::string("Curve") : DraftingDrawingLabel(Request.Kind);
    const std::string Caption = NextCaption(Document, Stem);
    const char*       FormTag = Request.Kind == DraftingDrawingKind::Curve ? SketchSplineFormLabel(Request.Form) : nullptr;
    return RunCommand(Document, Caption + (FormTag ? std::string(" · ") + FormTag : std::string{}), false, [&]() -> std::string
    {
        if (Request.GridPlane < 0 || Request.GridPlane > 2) return "construction plane must be XY, XZ or YZ";
        SketchModel Sketch;
        if (std::string Refusal = ComposeDraftingSketch(Request, Sketch); !Refusal.empty()) return Refusal;
        FeatureStep Step;
        Step.Kind   = FeatureStepKind::Sketch;
        Step.Label  = "Sketch#" + std::to_string(NextStepOrdinal(Document.Tree));
        Step.Plane  = DraftingSketchPlane(Request.GridPlane);
        Step.Sketch = std::move(Sketch);
        Document.Captions[Step.Label] = Caption;
        Document.Tree.Steps.push_back(std::move(Step));
        return {};
    });
}

DraftingCommandOutcome ExtrudeDraftingSketch(DraftingDocument& Document, const std::string& SketchLabel, double Distance, FeatureCombination Combination,
                                             const std::string& Target)
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
        FeatureStep Step;
        Step.Kind        = FeatureStepKind::Extrude;
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

OrthonormalFrame DraftingSketchPlane(int GridPlane) noexcept
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

bool DraftingSketchConsumedCondition(const DraftingDocument& Document, const std::string& SketchLabel)
{
    return std::any_of(Document.Tree.Steps.begin(), Document.Tree.Steps.end(), [&](const FeatureStep& Step) { return Step.Profile == SketchLabel; });
}

DraftingCommandOutcome ImportDraftingCurves(DraftingDocument& Document, const VehicleCurveFile& File, double Tolerance)
{
    const std::string Label = "Import " + std::to_string(File.Curves.size()) + " curve(s)";
    DraftingCommandOutcome Outcome = RunCommand(Document, Label, false, [&]() -> std::string
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
        for (const DraftingImagePlane& Plane : File.ImagePlanes)
        {
            const auto Same = std::find_if(Document.ImagePlanes.begin(), Document.ImagePlanes.end(), [&](const DraftingImagePlane& Held) { return Held.Name == Plane.Name; });
            if (Same != Document.ImagePlanes.end()) *Same = Plane;
            else Document.ImagePlanes.push_back(Plane);
        }
    }
    return Outcome;
}

DraftingCommandOutcome LoftDraftingCurves(DraftingDocument& Document, const std::vector<std::string>& Labels, int Continuity)
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
                              + (Continuity > 0 ? " · G" + std::to_string(Continuity) : std::string());
    DraftingCommandOutcome Outcome = RunCommand(Document, Label, false, [&]() -> std::string
    {
        if (Labels.size() < 2) return "loft needs at least 2 curves (select them in section order)";
        for (size_t Index = 0; Index < Labels.size(); ++Index)
        {
            const auto Found = std::find_if(Document.Tree.Steps.begin(), Document.Tree.Steps.end(), [&](const FeatureStep& Step) { return Step.Label == Labels[Index]; });
            if (Found == Document.Tree.Steps.end() || Found->Kind != FeatureStepKind::SpaceCurve) return "no curve step labelled " + Labels[Index];
            if (std::find(Labels.begin(), Labels.begin() + Index, Labels[Index]) != Labels.begin() + Index) return "curve " + Labels[Index] + " is selected twice";
        }
        if (Continuity < 0 || Continuity > 2) return "continuity must be G0, G1 or G2";
        if (NetworkCondition && Continuity > 0) return "these curves cross (a network): G1 / G2 is available for section lofts only (🚧 networks)";
        FeatureStep Step;
        Step.Kind       = NetworkCondition ? FeatureStepKind::Network : FeatureStepKind::Loft;
        Step.Label      = (NetworkCondition ? "Network#" : "Loft#") + std::to_string(NextStepOrdinal(Document.Tree));
        Step.Sections   = Labels;
        Step.Continuity = Continuity;
        Document.Captions[Step.Label] = Caption;
        Document.Tree.Steps.push_back(std::move(Step));
        return {};
    });
    if (Outcome.AcceptedCondition) Document.Selection.Curves.clear();
    return Outcome;
}

DraftingCommandOutcome MirrorDraftingPanels(DraftingDocument& Document, int Axis)
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

DraftingCommandOutcome CloseDraftingPanels(DraftingDocument& Document)
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

DraftingCommandOutcome MoveDraftingPole(DraftingDocument& Document, const std::string& Label, int Pole, Vector3 Position)
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

void SelectDraftingCurve(DraftingDocument& Document, const std::string& Label, bool AdditiveCondition)
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

bool UndoDraftingCommand(DraftingDocument& Document)
{
    if (Document.UndoEntries.empty()) return false;
    DraftingHistoryEntry Entry = std::move(Document.UndoEntries.back());
    Document.UndoEntries.pop_back();
    Document.RedoEntries.push_back(Snapshot(Document, Entry.Label));
    Document.RedoEntries.back().AppearanceCondition = Entry.AppearanceCondition;
    Restore(Document, std::move(Entry));
    return true;
}

bool RedoDraftingCommand(DraftingDocument& Document)
{
    if (Document.RedoEntries.empty()) return false;
    DraftingHistoryEntry Entry = std::move(Document.RedoEntries.back());
    Document.RedoEntries.pop_back();
    Document.UndoEntries.push_back(Snapshot(Document, Entry.Label));
    Document.UndoEntries.back().AppearanceCondition = Entry.AppearanceCondition;
    Restore(Document, std::move(Entry));
    return true;
}

DraftingCommandOutcome AssignDraftingMatcap(DraftingDocument& Document, const std::string& Matcap)
{
    const std::vector<std::string> Faces  = Document.Selection.Faces;
    const std::vector<std::string> Bodies = Faces.empty() ? Document.Selection.Bodies : std::vector<std::string>{};
    const std::string Label = "Matcap " + Matcap + " → " + (!Faces.empty() ? std::to_string(Faces.size()) + " face(s)"
                                                         : !Bodies.empty() ? std::to_string(Bodies.size()) + " body(ies)" : std::string("body"));
    return RunAppearanceCommand(Document, Label, [&]() -> std::string
    {
        DraftingAppearance& Appearance = Document.Appearance;
        if (!DraftingMatcapKnownCondition(Appearance, Matcap)) return "no matcap named " + Matcap;
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

DraftingCommandOutcome RemoveDraftingMatcap(DraftingDocument& Document)
{
    const std::vector<std::string> Faces  = Document.Selection.Faces;
    const std::vector<std::string> Bodies = Faces.empty() ? Document.Selection.Bodies : std::vector<std::string>{};
    const std::string Label = !Faces.empty()  ? "Matcap removed → " + std::to_string(Faces.size()) + " face(s) show the body"
                            : !Bodies.empty() ? "Matcap removed → " + std::to_string(Bodies.size()) + " body(ies) show the default"
                                              : std::string("Matcap removed → body Studio");
    return RunAppearanceCommand(Document, Label, [&]() -> std::string
    {
        DraftingAppearance& Appearance = Document.Appearance;
        if (!Bodies.empty())
        {
            size_t Removed = 0;
            for (const std::string& Key : Bodies) Removed += Appearance.BodyMatcaps.erase(Key);
            return Removed ? std::string{} : "the selected bodies have no matcap of their own";
        }
        if (Faces.empty())
        {
            if (Appearance.BodyMatcap == DraftingStudioMatcap) return "the body already shows Studio";
            Appearance.BodyMatcap = DraftingStudioMatcap;
            return {};
        }
        size_t Removed = 0;
        for (const std::string& Face : Faces) Removed += Appearance.FaceMatcaps.erase(Face);
        return Removed ? std::string{} : "the selected faces have no matcap of their own";
    });
}

DraftingCommandOutcome AddDraftingMatcapPicture(DraftingDocument& Document, const std::string& Name, const std::string& Path)
{
    return RunAppearanceCommand(Document, "Matcap picture " + Name, [&]() -> std::string
    {
        DraftingAppearance& Appearance = Document.Appearance;
        if (Name.empty() || std::any_of(Name.begin(), Name.end(), [](char Character) { return std::isspace(static_cast<unsigned char>(Character)) != 0; }))
            return "a matcap name needs at least one character and no spaces";
        if (DraftingMatcapKnownCondition(Appearance, Name)) return "a matcap named " + Name + " exists already";
        if (Appearance.CustomMatcaps.size() + DraftingBuiltInMatcaps().size() >= DraftingMatcapLayerLimit) return "the matcap library is full";
        const DraftingPicture Picture = DecodeDraftingPicture(Path);
        if (!Picture.Refusal.empty()) return Picture.Refusal;
        Appearance.CustomMatcaps.push_back({ Name, Path });
        return {};
    });
}

void SelectDraftingName(DraftingDocument& Document, DraftingSelectionKind Kind, const std::string& Name, bool AdditiveCondition)
{
    std::vector<std::string>& Names = Kind == DraftingSelectionKind::Face ? Document.Selection.Faces
                                    : Kind == DraftingSelectionKind::Edge ? Document.Selection.Edges : Document.Selection.Bodies;
    if (!AdditiveCondition)
    {
        Document.Selection = {};
        Names.push_back(Name);
        Document.Selection.LastKind = Kind;
        return;
    }
    Document.Selection.LastKind = Kind;
    const auto Found = std::find(Names.begin(), Names.end(), Name);
    if (Found != Names.end()) Names.erase(Found);
    else Names.push_back(Name);
}

std::vector<std::string> DraftingTangentChain(const DraftingDocument& Document, const std::string& Name)
{
    const RegeneratedBody* Owner = FindRegeneratedBody(Document, DraftingBodyOfName(Document, DraftingSelectionKind::Edge, Name));
    if (!Document.Partition || !Owner) return { Name };
    const Modeller::BodyNaming&    Naming    = Owner->Naming;
    const Modeller::BodyInventory  Inventory = Document.Partition->CollectInventory(Owner->Body);
    std::vector<Modeller::EdgeToken> Seeds;
    for (const Modeller::EdgeToken Edge : Inventory.Edges)
    {
        const auto Found = Naming.Edges.find(Edge.Slot);
        if (Found != Naming.Edges.end() && Found->second == Name) Seeds.push_back(Edge);
    }
    if (Seeds.empty()) return { Name };
    std::vector<std::string> Names;
    for (const Modeller::EdgeToken Edge : Modeller::GrowTangentChain(*Document.Partition, Owner->Body, Seeds))
    {
        const auto Found = Naming.Edges.find(Edge.Slot);
        if (Found != Naming.Edges.end()) Names.push_back(Found->second);
    }
    return Names;
}

DraftingCommandOutcome CombineDraftingBodies(DraftingDocument& Document, const std::string& Target, const std::vector<std::string>& Tools, FeatureCombination Combination)
{
    std::string Operands = DraftingBodyCaption(Document, Target);
    const char* Sign = Combination == FeatureCombination::Difference ? " − " : Combination == FeatureCombination::Intersection ? " ∩ " : " ∪ ";
    for (const std::string& Tool : Tools) Operands += Sign + DraftingBodyCaption(Document, Tool);
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
            if (!Seen.insert(Tool).second) return DraftingBodyCaption(Document, Tool) + " is picked twice (the target cannot be a tool)";
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

DraftingCommandOutcome CutDraftingBody(DraftingDocument& Document, const DraftingCutRequest& Request)
{
    const std::string Target = Request.Target.empty() ? Document.Regeneration.Active : Request.Target;
    const std::string Label  = "Cut (" + DraftingBodyCaption(Document, Target) + " by " + DraftingBodyCaption(Document, Request.Cutter) + ")";
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

DraftingCommandOutcome RemoveDraftingBodies(DraftingDocument& Document, const std::vector<std::string>& Keys)
{
    std::string Operands;
    for (const std::string& Key : Keys) Operands += (Operands.empty() ? "" : ", ") + DraftingBodyCaption(Document, Key);
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

    [[nodiscard]] const SketchModel& SolvedSketch(const DraftingDocument& Document, const FeatureStep& Step)
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

SketchTrimOutcome PreviewDraftingTrim(const DraftingDocument& Document, const DraftingTrimRequest& Request)
{
    const auto Found = std::find_if(Document.Tree.Steps.begin(), Document.Tree.Steps.end(), [&](const FeatureStep& Step) { return Step.Label == Request.Sketch; });
    if (Found == Document.Tree.Steps.end() || Found->Kind != FeatureStepKind::Sketch)
    {
        SketchTrimOutcome Outcome;
        Outcome.Refusal = "no sketch step labelled " + Request.Sketch;
        return Outcome;
    }
    const OrthonormalFrame      Plane = ElevatedPlane(Document.Tree, *Found);
    std::vector<ProfileSegment> Foreign;
    for (const FeatureStep& Step : Document.Tree.Steps)
    {
        if (Step.Kind != FeatureStepKind::Sketch || Step.Label == Request.Sketch) continue;
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

DraftingCommandOutcome TrimDraftingSketch(DraftingDocument& Document, const DraftingTrimRequest& Request)
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
    return RunCommand(Document, "Trim " + CurveName + " of " + Owner, false, [&]() -> std::string
    {
        const SketchTrimOutcome Trim = PreviewDraftingTrim(Document, Request);
        if (!Trim.Refusal.empty()) return Trim.Refusal;
        const auto Found = std::find_if(Document.Tree.Steps.begin(), Document.Tree.Steps.end(), [&](const FeatureStep& Step) { return Step.Label == Request.Sketch; });
        if (!Trim.Sketch.Curves.empty())
        {
            Found->Sketch = Trim.Sketch;
            return {};
        }
        if (DraftingSketchConsumedCondition(Document, Request.Sketch)) return "the trim would leave " + Owner + " without curves, and a later step uses it";
        Document.Tree.Steps.erase(Found);
        Document.Captions.erase(Request.Sketch);
        return {};
    }, 0);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    EDITING (G / R / S · SHIFT+D · CTRL+V · H · J · O · P)
//------------------------------------------------------------------------------------------------------------------------

namespace
{
    [[nodiscard]] std::string ListCaptions(const DraftingDocument& Document, const std::vector<std::string>& Keys)
    {
        std::string Operands;
        for (const std::string& Key : Keys) Operands += (Operands.empty() ? "" : ", ") + DraftingBodyCaption(Document, Key);
        return Operands;
    }

    [[nodiscard]] std::vector<std::string> UniqueKeys(const std::vector<std::string>& Keys)
    {
        std::vector<std::string> Unique;
        for (const std::string& Key : Keys) if (std::find(Unique.begin(), Unique.end(), Key) == Unique.end()) Unique.push_back(Key);
        return Unique;
    }

    [[nodiscard]] const FeatureStep* FindStep(const DraftingDocument& Document, const std::string& Label)
    {
        for (const FeatureStep& Step : Document.Tree.Steps) if (Step.Label == Label) return &Step;
        return nullptr;
    }
}

DraftingCommandOutcome TransformDraftingBodies(DraftingDocument& Document, const std::vector<std::string>& Keys, const AffineMap& Map, const std::string& Verb)
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

DraftingCommandOutcome DuplicateDraftingBodies(DraftingDocument& Document, const std::vector<std::string>& Keys, const AffineMap& Map, const std::string& Source)
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
            std::string Stem = DraftingBodyCaption(Document, Key);
            const size_t Digits = Stem.find_last_not_of("0123456789");
            if (Digits != std::string::npos && Digits + 1 < Stem.size() && Digits >= 3 && Stem.compare(Digits - 3, 4, "Copy") == 0) Stem.erase(Digits - 3);
            Document.Captions[Step.Label] = NextCaption(Document, Stem + "Copy");
            Copies.push_back(Step.Label);
            Document.Tree.Steps.push_back(std::move(Step));
        }
        Document.Selection          = {};
        Document.Selection.Bodies   = Copies;
        Document.Selection.LastKind = DraftingSelectionKind::Body;
        return {};
    });
}

DraftingCommandOutcome ChangeDraftingVisibility(DraftingDocument& Document, DraftingVisibilityVerb Verb)
{
    // The picked bodies: body picks plus the owners of picked faces and edges.
    std::set<std::string> Picked(Document.Selection.Bodies.begin(), Document.Selection.Bodies.end());
    for (const std::string& Face : Document.Selection.Faces) if (const std::string Key = DraftingBodyOfName(Document, DraftingSelectionKind::Face, Face); !Key.empty()) Picked.insert(Key);
    for (const std::string& Edge : Document.Selection.Edges) if (const std::string Key = DraftingBodyOfName(Document, DraftingSelectionKind::Edge, Edge); !Key.empty()) Picked.insert(Key);
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
        case DraftingVisibilityVerb::HideSelected:
            if (Picked.empty()) return "pick the bodies to hide";
            Hidden.insert(Picked.begin(), Picked.end());
            break;
        case DraftingVisibilityVerb::HideUnselected:
            if (Picked.empty()) return "pick the bodies to keep in view";
            for (const std::string& Key : Present) if (!Picked.contains(Key)) Hidden.insert(Key);
            break;
        case DraftingVisibilityVerb::UnhideAll:
            Hidden.clear();
            break;
        case DraftingVisibilityVerb::InvertHidden:
        {
            std::set<std::string> Swapped;
            for (const std::string& Key : Present) if (!Before.contains(Key)) Swapped.insert(Key);
            Hidden = std::move(Swapped);
            break;
        }
        }
        if (Hidden == Before) return Verb == DraftingVisibilityVerb::UnhideAll ? "nothing is hidden" : "nothing changes";
        // Hidden bodies leave the selection.
        std::erase_if(Document.Selection.Bodies, [&](const std::string& Key) { return Hidden.contains(Key); });
        std::erase_if(Document.Selection.Faces, [&](const std::string& Name) { return Hidden.contains(DraftingBodyOfName(Document, DraftingSelectionKind::Face, Name)); });
        std::erase_if(Document.Selection.Edges, [&](const std::string& Name) { return Hidden.contains(DraftingBodyOfName(Document, DraftingSelectionKind::Edge, Name)); });
        return {};
    });
}

DraftingCommandOutcome JoinDraftingSketches(DraftingDocument& Document, const std::vector<std::string>& Sketches)
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
            if (Sketches[Index] == Sketches.front()) return DraftingBodyCaption(Document, Sketches[Index]) + " is picked twice";
            if (DraftingSketchConsumedCondition(Document, Other->Label)) return DraftingBodyCaption(Document, Other->Label) + " feeds a later step (join into it instead)";
            const OrthonormalFrame Plane = ElevatedPlane(Document.Tree, *Other);
            const Vector3          Apart = Plane.Origin - Base.Origin;
            if (std::fabs(Dot(Plane.ZAxis, Base.ZAxis)) < 1.0 - 1.0e-9 || std::fabs(Dot(Apart, Base.ZAxis)) > 1.0e-9 * std::max(1.0, Length(Apart)))
                return DraftingBodyCaption(Document, Other->Label) + " is not in the plane of " + DraftingBodyCaption(Document, First->Label);
            SketchPlacement Placement;
            Placement.Origin = { Dot(Apart, Base.XAxis), Dot(Apart, Base.YAxis) };
            Placement.XAxis  = { Dot(Plane.XAxis, Base.XAxis), Dot(Plane.XAxis, Base.YAxis) };
            Placement.YAxis  = { Dot(Plane.YAxis, Base.XAxis), Dot(Plane.YAxis, Base.YAxis) };
            const double Join = 1.0e-9 * std::max({ 1.0, MeasureSketchScale(Joined), MeasureSketchScale(SolvedSketch(Document, *Other)) });
            const SketchJoinOutcome Outcome = JoinSketchModels(Joined, SolvedSketch(Document, *Other), Placement, Join);
            if (!Outcome.Refusal.empty()) return Outcome.Refusal;
            Joined = Outcome.Sketch;
        }
        const auto Keep = std::find_if(Document.Tree.Steps.begin(), Document.Tree.Steps.end(), [&](const FeatureStep& Step) { return Step.Label == Sketches.front(); });
        Keep->Sketch = std::move(Joined);
        for (size_t Index = 1; Index < Sketches.size(); ++Index)
        {
            std::erase_if(Document.Tree.Steps, [&](const FeatureStep& Step) { return Step.Label == Sketches[Index]; });
            Document.Captions.erase(Sketches[Index]);
        }
        return {};
    }, 0);
}

std::vector<std::string> DraftingJoinCandidates(const DraftingDocument& Document, const std::string& Sketch)
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
        if (Step.Kind != FeatureStepKind::Sketch || Step.Label == Sketch || DraftingSketchConsumedCondition(Document, Step.Label)) continue;
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

SketchOffsetOutcome PreviewDraftingOffset(const DraftingDocument& Document, const std::string& Sketch, double Distance)
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

DraftingCommandOutcome OffsetDraftingSketch(DraftingDocument& Document, const std::string& Sketch, double Distance)
{
    return RunCommand(Document, "Offset " + DraftingBodyCaption(Document, Sketch) + " by " + FormatAmount(Distance), false, [&]() -> std::string
    {
        const SketchOffsetOutcome Outcome = PreviewDraftingOffset(Document, Sketch, Distance);
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

DraftingCommandOutcome PipeDraftingPath(DraftingDocument& Document, const std::string& Path, double Radius, FeatureCombination Combination, const std::string& Target)
{
    return RunCommand(Document, "Pipe r " + FormatAmount(Radius) + " along " + DraftingBodyCaption(Document, Path), false, [&]() -> std::string
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

std::string DraftingBodyOfName(const DraftingDocument& Document, DraftingSelectionKind Kind, const std::string& Name)
{
    for (const RegeneratedBody& Body : Document.Regeneration.Bodies)
    {
        if (Kind == DraftingSelectionKind::Body) { if (Body.Key == Name) return Body.Key; continue; }
        const std::map<uint32_t, std::string>& Table = Kind == DraftingSelectionKind::Edge ? Body.Naming.Edges : Body.Naming.Faces;
        for (const auto& Entry : Table) if (Entry.second == Name) return Body.Key;
    }
    return {};
}

std::string DraftingBodyCaption(const DraftingDocument& Document, const std::string& Key)
{
    const auto Found = Document.Captions.find(Key);
    return Found != Document.Captions.end() ? Found->second : Key;
}

void ClearDraftingSelection(DraftingDocument& Document)
{
    Document.Selection = {};
}

std::string DescribeDraftingStep(const DraftingDocument& Document, const FeatureStep& Step)
{
    const auto Found = Document.Captions.find(Step.Label);
    if (Found == Document.Captions.end()) return Step.Label;
    return Found->second + " · " + FeatureStepKindLabel(Step.Kind);
}

const char* DraftingConstructLabel(DraftingConstructKind Kind) noexcept
{
    switch (Kind)
    {
    case DraftingConstructKind::Block:    return "Block";
    case DraftingConstructKind::Cylinder: return "Cylinder";
    case DraftingConstructKind::Sphere:   return "Sphere";
    case DraftingConstructKind::Torus:    return "Torus";
    }
    return "Construct";
}

const char* DraftingDrawingLabel(DraftingDrawingKind Kind) noexcept
{
    switch (Kind)
    {
    case DraftingDrawingKind::Line:      return "Line";
    case DraftingDrawingKind::Rectangle: return "Rectangle";
    case DraftingDrawingKind::Circle:    return "Circle";
    case DraftingDrawingKind::Arc:       return "Arc";
    case DraftingDrawingKind::Slot:      return "Slot";
    case DraftingDrawingKind::Curve:     return "Curve";
    }
    return "Drawing";
}

const char* DraftingFaceOperationLabel(FaceOperationKind Operation) noexcept
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
    }
    return "Change";
}

} // namespace Frontier::DraftingWorkspace
