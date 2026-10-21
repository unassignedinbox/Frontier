/*====================================================================================================================================
                                                      DRAFTINGDOCUMENT.CPP
====================================================================================================================================*/
// 🧩 DraftingWorkspace document: construct tiles as sketch + sweep pairs, atomic commands, undo / redo, selection upkeep

//------------------------------------------------------------------------------------------------------------------------
//                                                    INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Document/DraftingDocument.h"

#include "Audit/EnclosedVolume.h"
#include "Audit/ModelAudit.h"
#include "Blend/RollingBallGeometry.h"
#include "Sketch/SketchCatalogue.h"
#include "Sketch/SketchSolver.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <numbers>
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
        break;
    }
    case DraftingConstructKind::Cylinder:
    {
        Author.Parameter("X", Spot.X, Spot.X - 5.0, Spot.X + 5.0, "axis x");
        Author.Parameter("Y", Spot.Y, Spot.Y - 5.0, Spot.Y + 5.0, "axis y");
        Author.Parameter("Z", Spot.Z, Spot.Z - 5.0, Spot.Z + 5.0, "base z (sketch elevation)");
        Author.Parameter("Radius", Request.Radius, 0.05, 5.0, "radius");
        Author.Parameter("Height", Request.Height, 0.1, 10.0, "extent along z (extrude)");
        FeatureStep& Section = Author.Step(FeatureStepKind::Sketch, SketchLabel);
        Section.Plane        = FloorPlane();
        Section.Elevation    = ParameterValue(Author.Named("Z"));
        SketchModel& Sketch  = Section.Sketch;
        const int Origin     = AppendSketchPoint(Sketch, { 0.0, 0.0 });
        const int Centre     = AppendSketchPoint(Sketch, { Spot.X, Spot.Y });
        const int Circle     = AppendSketchCircle(Sketch, Centre, Request.Radius, 1);
        AppendSketchConstraint(Sketch, Kind::Fixed, Origin, -1, -1, 0.0, "Origin", 0.0);
        Author.Bound(Sketch, Kind::HorizontalDistance, Origin, Centre, "X");
        Author.Bound(Sketch, Kind::VerticalDistance, Origin, Centre, "Y");
        Author.Bound(Sketch, Kind::Radius, Circle, -1, "Radius");
        FeatureStep& Sweep = Author.Step(FeatureStepKind::Extrude, "Extrude#" + std::to_string(Ordinal + 1));
        Sweep.Profile      = SketchLabel;
        Sweep.Extent       = ParameterValue(Author.Named("Height"));
        Sweep.Combination  = Combination;
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
    return { Document.Tree, Document.Captions, Label };
}

void Restore(DraftingDocument& Document, DraftingHistoryEntry Entry)
{
    Document.Tree     = std::move(Entry.Tree);
    Document.Captions = std::move(Entry.Captions);
    RegenerateDraftingDocument(Document);
}

// 📝 First refusal among the steps at or after FirstNew (the steps this command appended).
[[nodiscard]] std::string RefusalFrom(const DraftingDocument& Document, size_t FirstNew)
{
    for (size_t Index = FirstNew; Index < Document.Regeneration.Steps.size(); ++Index)
    {
        const FeatureStepReport& Report = Document.Regeneration.Steps[Index];
        if (Report.Status == FeatureStepStatus::Refused) return Report.Label + ": " + Report.Refusal;
    }
    return {};
}

// ① snapshot ② change (the callback returns a refusal or "") ③ regenerate ④ roll back when a new step refused.
template <typename Change>
DraftingCommandOutcome RunCommand(DraftingDocument& Document, const std::string& Label, bool CaptureCondition, Change&& Apply)
{
    const auto           Start      = std::chrono::steady_clock::now();
    DraftingHistoryEntry Before     = Snapshot(Document, Label);
    const size_t         FirstNew   = Document.Tree.Steps.size();
    DraftingCommandOutcome Outcome;
    Outcome.Label = Label;
    Outcome.Refusal = Apply();
    if (Outcome.Refusal.empty())
    {
        RegenerateDraftingDocument(Document, CaptureCondition);
        Outcome.Refusal = RefusalFrom(Document, FirstNew);
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

void KeepResolvedNames(std::vector<std::string>& Names, const std::map<uint32_t, std::string>& Table)
{
    std::set<std::string> Present;
    for (const auto& Entry : Table) Present.insert(Entry.second);
    std::erase_if(Names, [&](const std::string& Name) { return !Present.contains(Name); });
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
    const BodyToken Body         = Document.Regeneration.Body;
    if (Body.PresentCondition() && Document.Partition->Bodies.Authenticate(Body))
    {
        const BodyInventory Inventory = Document.Partition->CollectInventory(Body);
        const AuditReport   Audit     = AuditBody(*Document.Partition, Body);
        Summary.PresentCondition = !Inventory.Faces.empty();
        Summary.SoundCondition   = Audit.SoundCondition();
        Summary.Faces            = static_cast<int>(Inventory.Faces.size());
        Summary.Edges            = static_cast<int>(Inventory.Edges.size());
        Summary.Vertices         = static_cast<int>(Inventory.Vertices.size());
        Summary.Genus            = Audit.Genus;
        Summary.Volume           = Summary.PresentCondition ? IntegrateBodyVolume(*Document.Partition, Body) : 0.0;
    }
    KeepResolvedNames(Document.Selection.Faces, Document.Regeneration.Naming.Faces);
    KeepResolvedNames(Document.Selection.Edges, Document.Regeneration.Naming.Edges);
}

DraftingDocument CloneDraftingDocument(const DraftingDocument& Source)
{
    DraftingDocument Copy = CreateDraftingDocument(Source.Title);
    Copy.Tree      = Source.Tree;
    Copy.Captions  = Source.Captions;
    Copy.Selection = Source.Selection;
    RegenerateDraftingDocument(Copy);
    return Copy;
}

DraftingCommandOutcome ConstructDraftingPrimitive(DraftingDocument& Document, const DraftingConstructRequest& Request)
{
    const bool        FirstBodyCondition = !Document.Summary.PresentCondition;
    const std::string Label = NextCaption(Document, DraftingConstructLabel(Request.Kind)) + " · " +
                              (FirstBodyCondition ? "New" : FeatureCombinationLabel(Request.Combination));
    return RunCommand(Document, Label, false, [&]() -> std::string
    {
        if (Request.Width <= 0.0 || Request.Depth <= 0.0 || Request.Height <= 0.0 || Request.Radius <= 0.0 ||
            Request.Minor <= 0.0 || Request.Major <= Request.Minor)
            return "every dimension must be positive (torus: Major > Minor)";
        if (!FirstBodyCondition && Request.Combination == FeatureCombination::New)
            return "a second body needs Union, Difference or Intersection";
        AuthorTile(Document, Request, FirstBodyCondition ? FeatureCombination::New : Request.Combination);
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

        // ① Variable: name each edge's anchor vertex on the current body (start, or end when reversed).
        std::vector<TopologyReference> Anchors;
        if (VariableCondition)
        {
            if (!Document.Partition || !Document.Regeneration.Body.PresentCondition()) return "there is no body to blend";
            const BodyNaming& Naming = Document.Regeneration.Naming;
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
        Step.Shape = Request.Shape;
        Step.Size  = ParameterValue(Caption + ".Size");
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

bool UndoDraftingCommand(DraftingDocument& Document)
{
    if (Document.UndoEntries.empty()) return false;
    DraftingHistoryEntry Entry = std::move(Document.UndoEntries.back());
    Document.UndoEntries.pop_back();
    Document.RedoEntries.push_back(Snapshot(Document, Entry.Label));
    Restore(Document, std::move(Entry));
    return true;
}

bool RedoDraftingCommand(DraftingDocument& Document)
{
    if (Document.RedoEntries.empty()) return false;
    DraftingHistoryEntry Entry = std::move(Document.RedoEntries.back());
    Document.RedoEntries.pop_back();
    Document.UndoEntries.push_back(Snapshot(Document, Entry.Label));
    Restore(Document, std::move(Entry));
    return true;
}

void SelectDraftingName(DraftingDocument& Document, DraftingSelectionKind Kind, const std::string& Name, bool AdditiveCondition)
{
    std::vector<std::string>& Names = Kind == DraftingSelectionKind::Face ? Document.Selection.Faces : Document.Selection.Edges;
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
    if (!Document.Partition || !Document.Regeneration.Body.PresentCondition()) return { Name };
    const Modeller::BodyNaming&    Naming    = Document.Regeneration.Naming;
    const Modeller::BodyInventory  Inventory = Document.Partition->CollectInventory(Document.Regeneration.Body);
    std::vector<Modeller::EdgeToken> Seeds;
    for (const Modeller::EdgeToken Edge : Inventory.Edges)
    {
        const auto Found = Naming.Edges.find(Edge.Slot);
        if (Found != Naming.Edges.end() && Found->second == Name) Seeds.push_back(Edge);
    }
    if (Seeds.empty()) return { Name };
    std::vector<std::string> Names;
    for (const Modeller::EdgeToken Edge : Modeller::GrowTangentChain(*Document.Partition, Document.Regeneration.Body, Seeds))
    {
        const auto Found = Naming.Edges.find(Edge.Slot);
        if (Found != Naming.Edges.end()) Names.push_back(Found->second);
    }
    return Names;
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
