/*====================================================================================================================================
                                                 DRAFTINGDOCUMENTVALIDATION.CPP
====================================================================================================================================*/
// 🧩 Step 9: DraftingWorkspace document gates — construct tiles, picks, blends, face edits, parameters, undo, files, variable blends, drawn sketches

//------------------------------------------------------------------------------------------------------------------------
//                                                    INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Document/DraftingDocument.h"
#include "Document/DraftingDocumentCodec.h"
#include "Document/DraftingScene.h"
#include "Document/VehicleCurveImport.h"

#include "Blend/BlendCatalogue.h"
#include "Sketch/SketchProfile.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <numbers>
#include <string>
#include <vector>

using namespace Frontier::DraftingWorkspace;
using namespace Frontier::SolidArcRemastered;

//------------------------------------------------------------------------------------------------------------------------
//                                                    INTERNAL
//------------------------------------------------------------------------------------------------------------------------

namespace
{
    constexpr double Pi = std::numbers::pi;

    struct GateTally
    {
        const char* Code     = "";                                                           // [-] - W1 …
        const char* Subject  = "";                                                           // [-] - What is measured
        const char* Limit    = "";                                                           // [-] - Pass condition (text)
        double      Worst    = 0.0;                                                          // [-] - Worst value
        long        Checks   = 0;                                                            // [-] - Measurements
        long        Failures = 0;                                                            // [-] - Measurements over the limit
    };

    GateTally ConstructGate   = { "W1", "Construct tiles (block, cylinder, sphere, torus, union, difference): |V − closed form| / V, sound audit", "≤ 10⁻⁹" };
    GateTally PickGate        = { "W2", "CPU ray pick: face under the ray / edge within reach resolves to the expected persistent name", "all equal" };
    GateTally BlendGate       = { "W3", "Fillet / chamfer on picked edges: |V − closed form| / V", "≤ 10⁻⁹" };
    GateTally FaceGate        = { "W4", "Face edit on a picked face (push): |V − closed form| / V", "≤ 10⁻⁹" };
    GateTally ParameterGate   = { "W5", "Parameter edit regenerates downstream steps; selection / references survive by name", "≤ 10⁻⁹, names kept" };
    GateTally AtomicGate      = { "W6", "Refused command (oversize fillet, empty selection) leaves tree, body and history untouched", "all equal" };
    GateTally HistoryGate     = { "W7", "Undo / redo restore exact volumes; delete removes a whole tile", "≤ 10⁻¹²" };
    GateTally FileGate        = { "W8", "TOML save → load → save is byte-equal; reloaded body volume and names equal", "equal" };
    GateTally VariableGate    = { "W9", "Variable blend (V / F): volume vs closed form; each size at its anchored corner through flip, parameter edit and reload", "≤ 10⁻⁹" };
    GateTally SurfacingGate   = { "W11", "Vehicle curves: VehicleCurves.toml round trip (bit-equal) → SpaceCurve steps → loft sheet vs analytic bonnet and limit points; pole move + undo; reload", "≤ 0.5 mm, equal" };
    GateTally PanelGate       = { "W12", "Vehicle panels: loft + loft (G2) → Alt+X mirror → 4-panel sheet; pod loft → mirror → Close → solid vs ∫ πab dy; crossing curves → network; reload byte-equal", "sound · rel. ≤ 2·10⁻³" };
    GateTally DrawingGate     = { "W10", "Drawn sketches (line, rectangle, circle, arc, slot, 4 curve forms) → extrude / Boolean: |V − closed form| / V; refusals; reload; delete", "≤ 10⁻⁹" };

    std::vector<std::string> Journal;                                                        // [-] - Narrative rows for the exhibit

    void Tally(GateTally& Gate, double Value, bool PassedCondition, const std::string& Note)
    {
        Gate.Worst = std::max(Gate.Worst, Value);
        ++Gate.Checks;
        if (!PassedCondition) ++Gate.Failures;
        char Row[512];
        std::snprintf(Row, sizeof(Row), "| %s | %s | %.3g | %s |", Gate.Code, Note.c_str(), Value, PassedCondition ? "🟢" : "🔴");
        Journal.push_back(Row);
    }

    void TallyVolume(GateTally& Gate, const DraftingDocument& Document, double Expected, const std::string& Note)
    {
        const double Relative = std::abs(Document.Summary.Volume - Expected) / std::max(1.0e-12, std::abs(Expected));
        Tally(Gate, Relative, Relative <= 1.0e-9 && Document.Summary.SoundCondition, Note + " (V = " + std::to_string(Document.Summary.Volume) + ")");
    }

    [[nodiscard]] DraftingRay RayBetween(std::array<double, 3> From, std::array<double, 3> Towards)
    {
        std::array<double, 3> Direction = { Towards[0] - From[0], Towards[1] - From[1], Towards[2] - From[2] };
        const double Length = std::sqrt(Direction[0] * Direction[0] + Direction[1] * Direction[1] + Direction[2] * Direction[2]);
        for (double& Component : Direction) Component /= Length;
        return { From, Direction };
    }

    // Pick through the scene and select the hit (as the viewport does on a click).
    std::string PickAndSelect(DraftingDocument& Document, const DraftingRay& Ray, bool AdditiveCondition)
    {
        const DraftingScene               Scene = BuildDraftingScene(Document);
        const std::optional<DraftingPick> Hit   = PickDraftingScene(Scene, Ray, 0.02);
        if (!Hit) return {};
        const std::string Name = NameDraftingIdentity(Scene, Hit->Identity);
        SelectDraftingName(Document, DraftingIdentityKind(Hit->Identity), Name, AdditiveCondition);
        return Name;
    }

    // Position of an end of the named edge on the document's current body (start, or end when reversed).
    [[nodiscard]] Vector3 EdgeEndPosition(const DraftingDocument& Document, const std::string& EdgeName, bool ReversedCondition)
    {
        for (const auto& [Slot, Name] : Document.Regeneration.Naming.Edges)
        {
            if (Name != EdgeName) continue;
            const EdgeProfile& Profile = Document.Partition->Edges.Resolve(Document.Partition->Edges.TokenAt(Slot));
            return Document.Partition->Vertices.Resolve(ReversedCondition ? Profile.End : Profile.Start).Position;
        }
        return { std::nan(""), std::nan(""), std::nan("") };
    }

    // Worst distance from the expected cap vertices of a variable fillet on the front-top edge (y = 0, z = Top) to the
    // body's vertices: the rails end Size from the anchored corner (x = AnchorX) and EndSize from the far one.
    [[nodiscard]] double MeasureCapGap(const DraftingDocument& Document, double AnchorX, double FarX, double Top, double Size, double EndSize)
    {
        const Vector3 Expected[4] = { { AnchorX, Size, Top }, { AnchorX, 0.0, Top - Size }, { FarX, EndSize, Top }, { FarX, 0.0, Top - EndSize } };
        double Worst = 0.0;
        for (const Vector3& Point : Expected)
        {
            double Nearest = std::numeric_limits<double>::infinity();
            for (const auto& [Slot, Name] : Document.Regeneration.Naming.Vertices)
                Nearest = std::min(Nearest, Distance(Document.Partition->Vertices.Resolve(Document.Partition->Vertices.TokenAt(Slot)).Position, Point));
            Worst = std::max(Worst, Nearest);
        }
        return Worst;
    }

    [[nodiscard]] DraftingDrawingRequest DrawingOf(DraftingDrawingKind Kind, int GridPlane, std::vector<Vector2> Points, bool ClosedCondition = false,
                                                   bool CentreCondition = false, SketchSplineForm Form = SketchSplineForm::Hermite)
    {
        DraftingDrawingRequest Request;
        Request.Kind = Kind; Request.GridPlane = GridPlane; Request.Points = std::move(Points);
        Request.ClosedCondition = ClosedCondition; Request.CentreCondition = CentreCondition; Request.Form = Form;
        return Request;
    }

    // Exact profile area of a drawing (Green's theorem on the solved sketch; independent of the solid's volume integral).
    [[nodiscard]] double MeasureDrawingArea(const DraftingDrawingRequest& Request)
    {
        SketchModel Sketch;
        if (!ComposeDraftingSketch(Request, Sketch).empty()) return std::nan("");
        (void)SolveSketch(Sketch);
        return ExtractSketchProfile(Sketch).Area;
    }

    void Expect(DraftingCommandOutcome Outcome)
    {
        if (!Outcome.AcceptedCondition) std::printf("⚠️ command \"%s\" refused: %s\n", Outcome.Label.c_str(), Outcome.Refusal.c_str());
    }
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    MAIN
//------------------------------------------------------------------------------------------------------------------------

int main()
{
    const auto Start = std::chrono::steady_clock::now();

    // ---- W1 single tiles against closed forms
    {
        DraftingDocument Document = CreateDraftingDocument("Tiles");
        DraftingConstructRequest Request;
        Request.Kind = DraftingConstructKind::Sphere; Request.Position = { 0.3, -0.2, 1.0 }; Request.Radius = 0.8;
        Expect(ConstructDraftingPrimitive(Document, Request));
        TallyVolume(ConstructGate, Document, 4.0 / 3.0 * Pi * 0.8 * 0.8 * 0.8, "Sphere r 0.8 at (0.3, −0.2, 1)");
        DraftingDocument Ring = CreateDraftingDocument("Ring");
        Request.Kind = DraftingConstructKind::Torus; Request.Position = { 1.0, 2.0, 0.5 }; Request.Major = 1.5; Request.Minor = 0.4;
        Expect(ConstructDraftingPrimitive(Ring, Request));
        TallyVolume(ConstructGate, Ring, 2.0 * Pi * Pi * 1.5 * 0.4 * 0.4, "Torus R 1.5 r 0.4 at (1, 2, 0.5)");
        DraftingDocument Drilled = CreateDraftingDocument("Drilled");
        Request = {};
        Request.Kind = DraftingConstructKind::Block; Request.Position = { 0.0, 0.0, 0.0 }; Request.Width = 3.0; Request.Depth = 2.0; Request.Height = 1.0;
        Expect(ConstructDraftingPrimitive(Drilled, Request));
        Request.Kind = DraftingConstructKind::Cylinder; Request.Position = { 1.0, 1.0, -0.5 }; Request.Radius = 0.4; Request.Height = 2.0;
        Request.Combination = FeatureCombination::Difference;
        Expect(ConstructDraftingPrimitive(Drilled, Request));
        TallyVolume(ConstructGate, Drilled, 6.0 - Pi * 0.16, "Block 3 × 2 × 1 − through cylinder r 0.4");
    }

    // ---- W1 … W7 on one modelling session (the parity walkthrough)
    DraftingDocument Document = CreateDraftingDocument("Bracket");
    DraftingConstructRequest Block;
    Block.Kind = DraftingConstructKind::Block; Block.Width = 4.0; Block.Depth = 2.0; Block.Height = 1.0;
    Expect(ConstructDraftingPrimitive(Document, Block));
    TallyVolume(ConstructGate, Document, 8.0, "Block1 4 × 2 × 1");
    DraftingConstructRequest Boss;
    Boss.Kind = DraftingConstructKind::Cylinder; Boss.Position = { 1.0, 1.0, 1.0 }; Boss.Radius = 0.5; Boss.Height = 0.75;
    Boss.Combination = FeatureCombination::Union;
    Expect(ConstructDraftingPrimitive(Document, Boss));
    const double BossVolume = Pi * 0.25 * 0.75;
    TallyVolume(ConstructGate, Document, 8.0 + BossVolume, "+ Cylinder1 boss r 0.5 h 0.75 (union on the top face)");

    // W2: pick the top face beside the boss, then the right vertical-face / top-face edge (x = 4, z = 1)
    const std::string TopFace = PickAndSelect(Document, RayBetween({ 3.0, 1.0, 5.0 }, { 3.0, 1.0, 0.0 }), false);
    Tally(PickGate, 0.0, TopFace.find("Cap(End)") != std::string::npos, "ray ↓ at (3, 1) → " + TopFace);
    const std::string RightEdge = PickAndSelect(Document, RayBetween({ 5.0, 1.0, 2.0 }, { 4.0, 1.0, 1.0 }), false);
    const bool RightEdgeCondition = RightEdge.rfind("Edge(", 0) == 0 && RightEdge.find("Cap(End)") != std::string::npos;
    Tally(PickGate, 0.0, RightEdgeCondition, "ray at the x = 4 top edge → " + RightEdge);
    const std::string Missed = PickAndSelect(Document, RayBetween({ 9.0, 9.0, 9.0 }, { 9.0, 9.0, 20.0 }), false);
    Tally(PickGate, 0.0, Missed.empty(), "ray away from the body → no pick");

    // W3: fillet r 0.25 on the picked edge (length 2): removed (1 − π/4)·r²·L
    SelectDraftingName(Document, DraftingSelectionKind::Edge, RightEdge, false);
    Expect(BlendDraftingSelection(Document, BlendProfile::Fillet, 0.25));
    const double FilletLoss = (1.0 - Pi / 4.0) * 0.0625 * 2.0;
    TallyVolume(BlendGate, Document, 8.0 + BossVolume - FilletLoss, "Fillet1 r 0.25 on the picked edge");
    // chamfer setback 0.2 on the opposite (x = 0) top edge: removed c²/2·L
    const std::string LeftEdge = PickAndSelect(Document, RayBetween({ -1.0, 1.0, 2.0 }, { 0.0, 1.0, 1.0 }), false);
    Expect(BlendDraftingSelection(Document, BlendProfile::Chamfer, 0.2));
    TallyVolume(BlendGate, Document, 8.0 + BossVolume - FilletLoss - 0.02 * 2.0, "Chamfer1 0.2 on the x = 0 top edge");

    // W4: push the front face (y = 0) out by 0.3: + 0.3 × front area (rectangle minus the blended corners)
    const std::string FrontFace = PickAndSelect(Document, RayBetween({ 2.0, -5.0, 0.5 }, { 2.0, 0.0, 0.5 }), false);
    Tally(PickGate, 0.0, !FrontFace.empty() && FrontFace.rfind("Edge(", 0) != 0, "ray → front face → " + FrontFace);
    const double FrontArea = 4.0 * 1.0 - (1.0 - Pi / 4.0) * 0.0625 - 0.02;
    Expect(ChangeDraftingSelection(Document, FaceOperationKind::Push, 0.3));
    const double PushedVolume = 8.0 + BossVolume - FilletLoss - 0.04 + 0.3 * FrontArea;
    TallyVolume(FaceGate, Document, PushedVolume, "Push1 0.3 on the picked front face");

    // W5: Block1.Depth 2 → 2.5: every blend follows (edge length 2.5), the push still applies to the renamed-same face
    const double BeforeEdit = Document.Summary.Volume;
    Expect(AssignDraftingParameter(Document, "Block1.Depth", 2.5));
    const double EditedVolume = 4.0 * 2.5 + BossVolume - (1.0 - Pi / 4.0) * 0.0625 * 2.5 - 0.02 * 2.5 + 0.3 * FrontArea;
    TallyVolume(ParameterGate, Document, EditedVolume, "Block1.Depth = 2.5 → fillet, chamfer, push regenerate");
    Tally(ParameterGate, 0.0, Document.Regeneration.Refused == 0, "no step refused after the edit");
    Tally(ParameterGate, 0.0, Document.Selection.Faces.size() == 1 && Document.Selection.Faces[0] == FrontFace, "selection still names " + FrontFace);

    // W6: atomic refusals
    const size_t StepsBefore = Document.Tree.Steps.size();
    const size_t UndoBefore  = Document.UndoEntries.size();
    const double VolumeBefore = Document.Summary.Volume;
    const std::string BackEdge = PickAndSelect(Document, RayBetween({ 2.0, 3.5, 2.0 }, { 2.0, 2.5, 1.0 }), false);
    const DraftingCommandOutcome Oversize = BlendDraftingSelection(Document, BlendProfile::Fillet, 5.0);
    Tally(AtomicGate, 0.0, !Oversize.AcceptedCondition && Document.Tree.Steps.size() == StepsBefore && Document.UndoEntries.size() == UndoBefore &&
                           Document.Summary.Volume == VolumeBefore, "fillet r 5 on " + BackEdge + " refused: " + Oversize.Refusal.substr(0, 90));
    ClearDraftingSelection(Document);
    const DraftingCommandOutcome Empty = ChangeDraftingSelection(Document, FaceOperationKind::Push, 0.1);
    Tally(AtomicGate, 0.0, !Empty.AcceptedCondition && Document.Tree.Steps.size() == StepsBefore, "push with no faces refused: " + Empty.Refusal);

    // W7: undo the edit, redo it, delete the boss tile
    UndoDraftingCommand(Document);
    Tally(HistoryGate, std::abs(Document.Summary.Volume - BeforeEdit), Document.Summary.Volume == BeforeEdit, "undo Block1.Depth → exact earlier volume");
    RedoDraftingCommand(Document);
    Tally(HistoryGate, std::abs(Document.Summary.Volume - EditedVolume) / EditedVolume, std::abs(Document.Summary.Volume - EditedVolume) <= 1.0e-12 * EditedVolume, "redo → edited volume");
    const std::string BossLabel = Document.Tree.Steps[2].Label;
    Expect(DeleteDraftingStep(Document, BossLabel));
    const bool WholeTileCondition = std::none_of(Document.Tree.Steps.begin(), Document.Tree.Steps.end(), [](const FeatureStep& Step)
                                                 { return Step.Label == "Sketch#3" || Step.Label == "Extrude#4"; });
    TallyVolume(HistoryGate, Document, EditedVolume - BossVolume, "delete Cylinder1 (sketch + extrude) → volume without the boss");
    Tally(HistoryGate, 0.0, WholeTileCondition && Document.Tree.Parameters.end() == std::find_if(Document.Tree.Parameters.begin(), Document.Tree.Parameters.end(),
          [](const FeatureParameter& Parameter) { return Parameter.Name.rfind("Cylinder1.", 0) == 0; }), "both steps and every Cylinder1.* parameter removed");

    // W8: file round trip
    const std::string Saved = EncodeDraftingDocument(Document);
    DraftingDocumentFile Loaded = DecodeDraftingDocument(Saved);
    DraftingDocument Reloaded = CreateDraftingDocument(Loaded.Title);
    const DraftingCommandOutcome Opened = ReplaceDraftingTree(Reloaded, Loaded.Tree, Loaded.Captions, "Open");
    Tally(FileGate, std::abs(Reloaded.Summary.Volume - Document.Summary.Volume), Loaded.Refusal.empty() && Opened.AcceptedCondition &&
          Reloaded.Summary.Volume == Document.Summary.Volume, "reloaded volume equal (" + std::to_string(Saved.size()) + " bytes of TOML)");
    Tally(FileGate, 0.0, Reloaded.Regeneration.Naming.Faces == Document.Regeneration.Naming.Faces &&
          Reloaded.Regeneration.Naming.Edges == Document.Regeneration.Naming.Edges, "reloaded persistent names equal");
    Tally(FileGate, 0.0, EncodeDraftingDocument(Reloaded) == Saved, "save → load → save byte-equal");
    const DraftingDocumentFile Broken = DecodeDraftingDocument("Format = \"DraftingWorkspace\"\nSteps = [ { Kind = 0 } ]\n");
    Tally(FileGate, 0.0, !Broken.Refusal.empty(), "malformed file refused: " + Broken.Refusal);

    // ---- W9 variable blend: pick the front-top edge (y = 0, z = 1) of a 4 × 2 × 1 block, fillet 0.1 → 0.3, flip, edit, reload
    for (int Flip = 0; Flip < 2; ++Flip)
    {
        const bool       ReversedCondition = Flip == 1;
        DraftingDocument Variable = CreateDraftingDocument("Variable");
        Expect(ConstructDraftingPrimitive(Variable, Block));
        const std::string FrontEdge = PickAndSelect(Variable, RayBetween({ 2.0, -1.0, 2.0 }, { 2.0, 0.0, 1.0 }), false);
        const double AnchorX = EdgeEndPosition(Variable, FrontEdge, ReversedCondition).X;      // [len] - Corner that takes Size
        const double FarX    = 4.0 - AnchorX;
        DraftingBlendRequest Request;
        Request.Shape = BlendProfile::Fillet; Request.Size = 0.1; Request.EndSize = 0.3; Request.ReversedCondition = ReversedCondition;
        const DraftingCommandOutcome Applied = BlendDraftingSelection(Variable, Request);
        Expect(Applied);
        const std::string Case = std::string(ReversedCondition ? "flipped" : "unflipped") + ", anchor x = " + std::to_string(AnchorX).substr(0, 3);
        TallyVolume(VariableGate, Variable, 8.0 - MeasureVariableFilletLoss(4.0, 0.1, 0.3), "Fillet 0.1 → 0.3 on " + FrontEdge + " (" + Case + ")");
        const double Gap = MeasureCapGap(Variable, AnchorX, FarX, 1.0, 0.1, 0.3);
        Tally(VariableGate, Gap, Gap <= 1.0e-9, "rails 0.1 from the anchored corner, 0.3 from the far one (" + Case + ")");
        Tally(VariableGate, 0.0, Applied.Label.find("0.1 → 0.3") != std::string::npos, "history caption: " + Applied.Label);

        // Width 4 → 5: the anchor is a named vertex, so 0.1 stays at its corner (x = 0 stays, x = 4 moves to 5).
        Expect(AssignDraftingParameter(Variable, "Block1.Width", 5.0));
        const double MovedAnchor = AnchorX < 2.0 ? 0.0 : 5.0, MovedFar = 5.0 - MovedAnchor;
        TallyVolume(VariableGate, Variable, 10.0 - MeasureVariableFilletLoss(5.0, 0.1, 0.3), "Block1.Width = 5 regenerates the variable fillet (" + Case + ")");
        const double MovedGap = MeasureCapGap(Variable, MovedAnchor, MovedFar, 1.0, 0.1, 0.3);
        Tally(VariableGate, MovedGap, MovedGap <= 1.0e-9 && Variable.Regeneration.Refused == 0, "after the edit 0.1 is still at its corner, x = " + std::to_string(MovedAnchor).substr(0, 3) + " (" + Case + ")");

        // Save → load: the end size and the named anchor survive the TOML round trip.
        const std::string Saved = EncodeDraftingDocument(Variable);
        DraftingDocumentFile Loaded = DecodeDraftingDocument(Saved);
        DraftingDocument Reloaded = CreateDraftingDocument(Loaded.Title);
        const DraftingCommandOutcome Opened = ReplaceDraftingTree(Reloaded, Loaded.Tree, Loaded.Captions, "Open");
        const double ReloadGap = Opened.AcceptedCondition ? MeasureCapGap(Reloaded, MovedAnchor, MovedFar, 1.0, 0.1, 0.3) : 1.0;
        Tally(VariableGate, ReloadGap, Opened.AcceptedCondition && Reloaded.Summary.Volume == Variable.Summary.Volume && ReloadGap <= 1.0e-9 &&
              Saved.find("SizeAnchors") != std::string::npos && EncodeDraftingDocument(Reloaded) == Saved, "reload: same volume bits, anchored rails, byte-equal re-save (" + Case + ")");
    }

    // ---- W10 drawing: the workspace tools as the viewport commits them, extruded with the Plasticity Boolean choices
    {
        using Kind = DraftingDrawingKind;
        const FeatureCombination Union = FeatureCombination::Union, Difference = FeatureCombination::Difference, New = FeatureCombination::New;
        auto DrawAndExtrude = [&](DraftingDocument& Target, const DraftingDrawingRequest& Request, double Distance, FeatureCombination Combination)
        {
            Expect(DrawDraftingSketch(Target, Request));
            const DraftingCommandOutcome Outcome = ExtrudeDraftingSketch(Target, Target.Tree.Steps.back().Label, Distance, Combination);
            Expect(Outcome);
            return Outcome;
        };
        const DraftingDrawingRequest Box = DrawingOf(Kind::Rectangle, 0, { { 0.0, 0.0 }, { 2.0, 1.0 } });
        const std::vector<Vector2> Square = { { 0.2, 0.2 }, { 0.5, 0.2 }, { 0.5, 0.8 }, { 0.2, 0.8 } };
        {
            DraftingDocument Drawn = CreateDraftingDocument("Drawn");
            DrawAndExtrude(Drawn, Box, 1.5, New);
            TallyVolume(DrawingGate, Drawn, 3.0, "Rectangle (0,0)–(2,1) on XY, extrude 1.5 (New)");
            DrawAndExtrude(Drawn, DrawingOf(Kind::Circle, 0, { { 1.0, 0.5 }, { 1.4, 0.5 } }, false, true), 1.5, Difference);
            TallyVolume(DrawingGate, Drawn, 3.0 - 0.24 * Pi, "− centre circle r 0.4 through (Difference)");
            DrawAndExtrude(Drawn, DrawingOf(Kind::Line, 0, { { 1.5, 0.0 }, { 3.0, 0.0 }, { 1.5, 1.0 } }, true), 1.0, Union);
            TallyVolume(DrawingGate, Drawn, 3.0 - 0.24 * Pi + 1.0 / 3.0, "+ closed line triangle, height 1 (Union; 1/3 outside the box)");
            DrawAndExtrude(Drawn, DrawingOf(Kind::Curve, 0, Square, true), 0.5, Difference);
            TallyVolume(DrawingGate, Drawn, 3.0 - 0.24 * Pi + 1.0 / 3.0 - 0.123, "− closed Hermite pocket 0.5 (area 0.246, coplanar floor: closed spline section)");

            // Reload: spline points and forms round-trip; same volume bits, byte-equal re-save.
            const std::string Saved = EncodeDraftingDocument(Drawn);
            DraftingDocumentFile Loaded = DecodeDraftingDocument(Saved);
            DraftingDocument Reloaded = CreateDraftingDocument(Loaded.Title);
            const DraftingCommandOutcome Opened = ReplaceDraftingTree(Reloaded, Loaded.Tree, Loaded.Captions, "Open");
            Tally(DrawingGate, std::abs(Reloaded.Summary.Volume - Drawn.Summary.Volume), Opened.AcceptedCondition && Reloaded.Summary.Volume == Drawn.Summary.Volume &&
                  EncodeDraftingDocument(Reloaded) == Saved, "reload of the drawn document: same volume bits, byte-equal re-save");

            // Refusals leave the document untouched.
            const size_t Steps = Drawn.Tree.Steps.size();
            const double Volume = Drawn.Summary.Volume;
            const DraftingCommandOutcome Collinear = DrawDraftingSketch(Drawn, DrawingOf(Kind::Arc, 0, { { 0.0, 2.0 }, { 1.0, 2.0 }, { 2.0, 2.0 } }));
            const DraftingCommandOutcome Coincident = DrawDraftingSketch(Drawn, DrawingOf(Kind::Curve, 0, { { 0.0, 2.0 }, { 0.0, 2.0 }, { 1.0, 2.0 } }));
            const DraftingCommandOutcome SecondBody = ExtrudeDraftingSketch(Drawn, Drawn.Tree.Steps.front().Label, 1.0, New);
            Tally(DrawingGate, 0.0, !Collinear.AcceptedCondition && !Coincident.AcceptedCondition && !SecondBody.AcceptedCondition && Drawn.Tree.Steps.size() == Steps &&
                  Drawn.Summary.Volume == Volume, "refused untouched: \"" + Collinear.Refusal + "\" · \"" + Coincident.Refusal + "\" · \"" + SecondBody.Refusal + "\"");
            Expect(DrawDraftingSketch(Drawn, DrawingOf(Kind::Arc, 0, { { 0.0, 2.0 }, { 2.0, 2.0 }, { 1.0, 2.5 } })));
            const DraftingCommandOutcome OpenArc = ExtrudeDraftingSketch(Drawn, Drawn.Tree.Steps.back().Label, 1.0, Union);
            Tally(DrawingGate, 0.0, !OpenArc.AcceptedCondition && Drawn.Summary.Volume == Volume, "open arc profile refused: \"" + OpenArc.Refusal + "\"");

            // Delete the pocket's sketch: its extrude goes with it.
            std::string CurveLabel;
            for (const FeatureStep& Step : Drawn.Tree.Steps) if (Drawn.Captions[Step.Label] == "Curve1") CurveLabel = Step.Label;
            Expect(DeleteDraftingStep(Drawn, CurveLabel));
            TallyVolume(DrawingGate, Drawn, 3.0 - 0.24 * Pi + 1.0 / 3.0, "delete Curve1: its extrude goes with it");
        }
        {
            DraftingDocument Slotted = CreateDraftingDocument("Slotted");
            DrawAndExtrude(Slotted, DrawingOf(Kind::Rectangle, 0, { { 1.0, 0.5 }, { 2.0, 1.0 } }, false, true), 1.5, New);
            TallyVolume(DrawingGate, Slotted, 3.0, "centre rectangle about (1, 0.5) to (2, 1), extrude 1.5");
            DrawAndExtrude(Slotted, DrawingOf(Kind::Slot, 0, { { 0.4, 0.5 }, { 1.2, 0.5 }, { 0.8, 0.7 } }), 1.5, Difference);
            TallyVolume(DrawingGate, Slotted, 3.0 - (0.8 * 0.4 + 0.04 * Pi) * 1.5, "− slot (0.4,0.5)–(1.2,0.5) r 0.2 through");
        }
        {
            // Upright plane: rectangle on XZ extrudes along −y.
            DraftingDocument Upright = CreateDraftingDocument("Upright");
            DrawAndExtrude(Upright, DrawingOf(Kind::Rectangle, 1, { { 0.0, 0.0 }, { 2.0, 1.0 } }), 0.5, New);
            TallyVolume(DrawingGate, Upright, 1.0, "rectangle on XZ, extrude 0.5");
            DrawAndExtrude(Upright, DrawingOf(Kind::Circle, 1, { { 0.6, 0.5 }, { 1.4, 0.5 } }), 0.5, Difference);
            TallyVolume(DrawingGate, Upright, 1.0 - 0.08 * Pi, "− two-point circle (0.6,0.5)–(1.4,0.5) on XZ");
        }
        // Each curve form alone: volume = exact profile area × height.
        const std::vector<Vector2> Wave = { { 0.0, 0.0 }, { 1.0, -0.3 }, { 2.0, 0.4 }, { 2.2, 1.5 }, { 0.8, 1.9 }, { -0.4, 1.0 } };
        for (const SketchSplineForm Form : { SketchSplineForm::Hermite, SketchSplineForm::Bezier, SketchSplineForm::Nurbs, SketchSplineForm::CubicSpline })
        {
            DraftingDocument Freeform = CreateDraftingDocument("Freeform");
            const DraftingDrawingRequest Request = DrawingOf(Kind::Curve, 0, Wave, true, false, Form);
            const double Area = MeasureDrawingArea(Request);
            DrawAndExtrude(Freeform, Request, 0.7, New);
            TallyVolume(DrawingGate, Freeform, Area * 0.7, std::string("closed ") + SketchSplineFormLabel(Form) + " through 6 clicks, extrude 0.7 = area " + std::to_string(Area) + " × 0.7");
            DrawAndExtrude(Freeform, DrawingOf(Kind::Circle, 0, { { 1.0, 0.8 }, { 1.3, 0.8 } }, false, true), 0.7, Difference);
            TallyVolume(DrawingGate, Freeform, (Area - 0.09 * Pi) * 0.7, std::string("  − circle r 0.3 inside the ") + SketchSplineFormLabel(Form));
        }
    }

    // ---- W11 vehicle surfacing (plan V1 import, V3 curve steps, V4 loft) on the synthetic bonnet sample.
    {
        const VehicleCurveFile Sample = ReadVehicleCurves(DRAFTING_VEHICLE_SAMPLE);
        size_t PointCount = 0;
        for (const VehicleCurve& Curve : Sample.Curves) PointCount += Curve.Points.size();
        Tally(SurfacingGate, 0.0, Sample.Refusal.empty() && Sample.Curves.size() == 11, "SyntheticBonnet.toml decoded: " + std::to_string(Sample.Curves.size()) + " curves · "
              + std::to_string(PointCount) + " points " + Sample.Refusal);
        const VehicleCurveFile Again = DecodeVehicleCurves(EncodeVehicleCurves(Sample));
        bool Equal = Again.Refusal.empty() && Again.Curves.size() == Sample.Curves.size();
        for (size_t Index = 0; Equal && Index < Sample.Curves.size(); ++Index)
        {
            const VehicleCurve& First = Sample.Curves[Index];
            const VehicleCurve& Second = Again.Curves[Index];
            Equal = First.Name == Second.Name && First.ClosedCondition == Second.ClosedCondition && First.Points.size() == Second.Points.size()
                 && std::memcmp(First.Points.data(), Second.Points.data(), First.Points.size() * sizeof(Vector3)) == 0;
        }
        Tally(SurfacingGate, 0.0, Equal, "V1 round trip decode → encode → decode: names, closed flags, every coordinate bit-equal");
        const VehicleCurveFile Millimetres = DecodeVehicleCurves("Format = \"VehicleCurves\"\nUnits = \"mm\"\n[[Curves]]\nName = \"A\"\nPoints = [[1000.0, 0.0, 0.0], [0.0, 500.0, 0.0]]\n");
        Tally(SurfacingGate, 0.0, Millimetres.Refusal.empty() && Millimetres.Curves.size() == 1 && Millimetres.Curves[0].Points[0].X == 1.0 && Millimetres.Curves[0].Points[1].Y == 0.5,
              "Units = \"mm\" scaled to metres");
        const std::pair<const char*, const char*> Malformed[] = {
            { "Format = \"Mesh\"\n", "Format is not" },
            { "Format = \"VehicleCurves\"\nUnits = \"in\"\n[[Curves]]\nName = \"A\"\nPoints = [[0.0, 0.0, 0.0], [1.0, 0.0, 0.0]]\n", "unknown Units" },
            { "Format = \"VehicleCurves\"\n[[Curves]]\nName = \"A\"\nPoints = [[0.0, 0.0, 0.0]]\n", "needs at least 2 points" },
            { "Format = \"VehicleCurves\"\n[[Curves]]\nName = \"A\"\nPoints = [[0.0, 0.0, 0.0], [1.0, \"x\", 0.0]]\n", "point 1 is not [x, y, z]" },
            { "Format = \"VehicleCurves\"\n[[Curves]]\nPoints = [[0.0, 0.0, 0.0], [1.0, 0.0, 0.0]]\n", "has no Name" },
        };
        for (const auto& [Text, Expected] : Malformed)
        {
            const VehicleCurveFile Refused = DecodeVehicleCurves(Text);
            Tally(SurfacingGate, 0.0, Refused.Refusal.find(Expected) != std::string::npos && Refused.Curves.empty(), "malformed file refused: " + Refused.Refusal);
        }

        // Import: 11 SpaceCurve steps in one undo entry, all within the 0.2 mm tolerance.
        DraftingDocument Vehicle = CreateDraftingDocument("Bonnet");
        const DraftingCommandOutcome Imported = ImportDraftingCurves(Vehicle, Sample);
        double WorstFit = 0.0;
        int    Poles = 0;
        for (const auto& [Label, Fit] : Vehicle.Regeneration.Curves) { WorstFit = std::max(WorstFit, Fit.WorstDeviation); Poles += static_cast<int>(Fit.Curve.Definition->Poles.size()); }
        Tally(SurfacingGate, WorstFit * 1000.0, Imported.AcceptedCondition && Vehicle.Tree.Steps.size() == 11 && Vehicle.Regeneration.Refused == 0 && Vehicle.UndoEntries.size() == 1
              && Vehicle.Regeneration.Curves.size() == 11 && WorstFit <= 2.0e-4, "import → 11 SpaceCurve steps, 1 undo entry, " + std::to_string(PointCount) + " points → " + std::to_string(Poles)
              + " poles, worst fit [mm] (value)");
        const DraftingCommandOutcome Refused = ImportDraftingCurves(Vehicle, DecodeVehicleCurves("Format = \"Mesh\"\n"));
        Tally(SurfacingGate, 0.0, !Refused.AcceptedCondition && Vehicle.Tree.Steps.size() == 11 && Vehicle.UndoEntries.size() == 1, "refused file leaves the document untouched: " + Refused.Refusal);

        // Loft through the 6 sections (by caption) → one open NURBS sheet.
        std::vector<std::string> Sections;
        for (const FeatureStep& Step : Vehicle.Tree.Steps) if (Vehicle.Captions[Step.Label].rfind("Section", 0) == 0) Sections.push_back(Step.Label);
        const DraftingCommandOutcome Single = LoftDraftingCurves(Vehicle, { Sections.front() });
        Tally(SurfacingGate, 0.0, !Single.AcceptedCondition && Single.Refusal.find("at least 2") != std::string::npos, "loft through one curve refused: " + Single.Refusal);
        const DraftingCommandOutcome Lofted = LoftDraftingCurves(Vehicle, Sections);
        const DraftingBodySummary& Sheet = Vehicle.Summary;
        Tally(SurfacingGate, 0.0, Lofted.AcceptedCondition && Sheet.PresentCondition && Sheet.SheetCondition && Sheet.SoundCondition && Sheet.Faces == 1 && Sheet.Edges == 4 && Sheet.Vertices == 4,
              "loft 6 sections → sheet: " + std::to_string(Sheet.Faces) + " face · " + std::to_string(Sheet.Edges) + " edges · audit " + (Sheet.SoundCondition ? "sound" : "findings") + " " + Lofted.Refusal);
        auto SheetSurface = [](const DraftingDocument& Document)
        {
            const FaceToken Face = Document.Partition->Faces.TokenAt(Document.Regeneration.Naming.Faces.begin()->first);
            return Document.Partition->Surfaces.Resolve(Document.Partition->Faces.Resolve(Face).Surface);
        };
        if (Lofted.AcceptedCondition)
        {
            const SurfaceGeometry Surface = SheetSurface(Vehicle);
            double WorstPoint = 0.0, WorstAnalytic = 0.0;
            for (const VehicleCurve& Curve : Sample.Curves)
            {
                for (const Vector3& Point : Curve.Points)
                {
                    const Vector2 Foot = ProjectOntoSurface(Surface, Point, { Point.X / 0.75, Point.Y / 1.1 });
                    WorstPoint = std::max(WorstPoint, Length(EvaluateSurface(Surface, Foot).Position - Point));
                }
            }
            for (int First = 0; First <= 60; ++First)
            {
                for (int Second = 0; Second <= 60; ++Second)
                {
                    const Vector3 Point = EvaluateSurface(Surface, { First / 60.0, Second / 60.0 }).Position;
                    const double  Xi = Point.X / 0.75, Eta = Point.Y / 1.1;
                    const double  Height = 0.78 + 0.16 * Eta - 0.06 * Eta * Eta - 0.085 * Xi * Xi + 0.012 * std::sin(Pi * Eta) * (1.0 - Xi * Xi);
                    const double  Across = (-0.17 * Xi - 0.024 * Xi * std::sin(Pi * Eta)) / 0.75;
                    const double  Along  = (0.16 - 0.12 * Eta + 0.012 * Pi * std::cos(Pi * Eta) * (1.0 - Xi * Xi)) / 1.1;
                    WorstAnalytic = std::max(WorstAnalytic, std::abs(Point.Z - Height) / std::sqrt(1.0 + Across * Across + Along * Along));
                }
            }
            Tally(SurfacingGate, WorstPoint * 1000.0, WorstPoint <= 5.0e-4, "sheet vs all " + std::to_string(PointCount) + " limit points (6 sections + 5 loops never lofted), worst [mm] (value)");
            Tally(SurfacingGate, WorstAnalytic * 1000.0, WorstAnalytic <= 5.0e-4, "sheet vs the analytic bonnet H(x, y), 61 × 61 normal distance, worst [mm] (value)");

            // Reload: points and pole edits bit-equal, save → load → save byte-equal, same face name.
            const std::string Text = EncodeDraftingDocument(Vehicle);
            const DraftingDocumentFile File = DecodeDraftingDocument(Text);
            DraftingDocument Reopened = CreateDraftingDocument(File.Title);
            const DraftingCommandOutcome Opened = ReplaceDraftingTree(Reopened, File.Tree, File.Captions, "Open");
            bool SameSteps = File.Tree.Steps.size() == Vehicle.Tree.Steps.size();
            for (size_t Index = 0; SameSteps && Index < File.Tree.Steps.size(); ++Index)
            {
                const FeatureStep& First = Vehicle.Tree.Steps[Index];
                const FeatureStep& Second = File.Tree.Steps[Index];
                SameSteps = First.Kind == Second.Kind && First.Sections == Second.Sections && First.Points.size() == Second.Points.size()
                         && std::memcmp(First.Points.data(), Second.Points.data(), First.Points.size() * sizeof(Vector3)) == 0;
            }
            Tally(SurfacingGate, 0.0, Opened.AcceptedCondition && SameSteps && EncodeDraftingDocument(Reopened) == Text && Reopened.Regeneration.Naming.Faces == Vehicle.Regeneration.Naming.Faces,
                  "reload: points bit-equal, sections equal, save → load → save byte-equal, face name " + Vehicle.Regeneration.Naming.Faces.begin()->second);

            // Pole move (mode 1 drag): +10 mm on the middle pole of Section3, then undo / redo.
            const std::string Third = Sections[2];
            const NurbsCurveDefinition Before = *Vehicle.Regeneration.Curves.at(Third).Curve.Definition;
            const int     Pole   = static_cast<int>(Before.Poles.size()) / 2;
            const Vector3 Middle = EvaluateSurface(Surface, { 0.5, 0.38 }).Position;
            const DraftingCommandOutcome Moved = MoveDraftingPole(Vehicle, Third, Pole, Before.Poles[Pole] + Vector3{ 0.0, 0.0, 0.01 });
            const double Shift = Moved.AcceptedCondition ? Length(EvaluateSurface(SheetSurface(Vehicle), { 0.5, 0.38 }).Position - Middle) : 0.0;
            Tally(SurfacingGate, 0.0, Moved.AcceptedCondition && Vehicle.Summary.SheetCondition && Shift > 1.0e-3 && Vehicle.Regeneration.Naming.Faces == Reopened.Regeneration.Naming.Faces,
                  "move pole " + std::to_string(Pole) + " of Section3 by +10 mm → loft regenerates, panel at (0.5, 0.38) moved " + std::to_string(Shift * 1000.0).substr(0, 5) + " mm, face name kept");
            const DraftingCommandOutcome Missing = MoveDraftingPole(Vehicle, Third, 99, Vector3{});
            Tally(SurfacingGate, 0.0, !Missing.AcceptedCondition && Missing.Refusal.find("does not exist") != std::string::npos, "pole 99 refused: " + Missing.Refusal);
            const bool Undone = UndoDraftingCommand(Vehicle);
            const double Back = Length(EvaluateSurface(SheetSurface(Vehicle), { 0.5, 0.38 }).Position - Middle);
            const bool Redone = RedoDraftingCommand(Vehicle);
            const double Reapplied = Length(EvaluateSurface(SheetSurface(Vehicle), { 0.5, 0.38 }).Position - Middle);
            Tally(SurfacingGate, Back, Undone && Redone && Back == 0.0 && std::abs(Reapplied - Shift) <= 1.0e-15, "undo → panel exactly back · redo → moved again");
        }
    }

    // ---- W12 vehicle panels (plan V5–V7): continuity, mirror, close, network through the document commands.
    {
        auto SamplePath = [](const char* FileName)
        {
            std::string Path = DRAFTING_VEHICLE_SAMPLE;
            return Path.substr(0, Path.rfind('/') + 1) + FileName;
        };
        auto ByCaption = [](DraftingDocument& Document, const std::string& Stem)
        {
            std::vector<std::string> Labels;
            for (const FeatureStep& Step : Document.Tree.Steps) if (Document.Captions[Step.Label].rfind(Stem, 0) == 0) Labels.push_back(Step.Label);
            return Labels;
        };
        auto Reloads = [](const DraftingDocument& Document)
        {
            const std::string Text = EncodeDraftingDocument(Document);
            const DraftingDocumentFile File = DecodeDraftingDocument(Text);
            DraftingDocument Reopened = CreateDraftingDocument(File.Title);
            const DraftingCommandOutcome Opened = ReplaceDraftingTree(Reopened, File.Tree, File.Captions, "Open");
            return Opened.AcceptedCondition && EncodeDraftingDocument(Reopened) == Text && Reopened.Regeneration.Naming.Faces == Document.Regeneration.Naming.Faces
                && Reopened.Summary.Volume == Document.Summary.Volume;
        };

        // Bonnet: front loft, rear loft G2 to it, mirror through x = 0.
        DraftingDocument Bonnet = CreateDraftingDocument("Bonnet panels");
        Expect(ImportDraftingCurves(Bonnet, ReadVehicleCurves(DRAFTING_VEHICLE_SAMPLE)));
        const std::vector<std::string> Sections = ByCaption(Bonnet, "Section");
        const DraftingCommandOutcome Front = LoftDraftingCurves(Bonnet, { Sections[0], Sections[1], Sections[2] });
        const DraftingCommandOutcome Rear  = LoftDraftingCurves(Bonnet, { Sections[2], Sections[3], Sections[4], Sections[5] }, 2);
        Tally(PanelGate, 0.0, Front.AcceptedCondition && Rear.AcceptedCondition && Bonnet.Regeneration.Panels.size() == 2 && Bonnet.Summary.SheetCondition && Bonnet.Summary.SoundCondition
              && Bonnet.Summary.Faces == 2, "front loft + rear loft G2 → 2 panels sewn on Section3, sheet sound " + Front.Refusal + Rear.Refusal);
        const DraftingCommandOutcome Mirrored = MirrorDraftingPanels(Bonnet, 0);
        const auto& Names = Bonnet.Regeneration.Panels;
        const std::string FrontName = Bonnet.Tree.Steps[11].Label + ".Side(Surface)";
        const std::string ImageName = Bonnet.Tree.Steps.back().Label + ".Image(" + FrontName + ")";
        Tally(PanelGate, 0.0, Mirrored.AcceptedCondition && Names.size() == 4 && Names.contains(ImageName) && Bonnet.Summary.SheetCondition && Bonnet.Summary.SoundCondition && Bonnet.Summary.Faces == 4,
              "Alt+X → 4 panels, one sound sheet, face " + ImageName + " " + Mirrored.Refusal);
        Tally(PanelGate, 0.0, Reloads(Bonnet), "bonnet reload: continuity + mirror steps survive, save → load → save byte-equal, names equal");
        const DraftingCommandOutcome Capped = CloseDraftingPanels(Bonnet);
        Tally(PanelGate, 0.0, !Capped.AcceptedCondition && Capped.Refusal.find("not planar") != std::string::npos && Names.size() == 4, "Close on the open bonnet rim refused, document kept: " + Capped.Refusal);
        const bool Undone = UndoDraftingCommand(Bonnet);
        Tally(PanelGate, 0.0, Undone && Bonnet.Regeneration.Panels.size() == 2 && Bonnet.Summary.Faces == 2, "undo mirror → 2 panels");

        // Pod: planar hoops, loft, mirror, close → solid.
        DraftingDocument Pod = CreateDraftingDocument("Pod");
        const VehicleCurveFile Hoops = ReadVehicleCurves(SamplePath("SyntheticPod.toml"));
        const bool Planar = !Hoops.Curves.empty() && std::all_of(Hoops.Curves.begin(), Hoops.Curves.end(), [](const VehicleCurve& Curve) { return Curve.PlanarCondition; })
                         && DecodeVehicleCurves(EncodeVehicleCurves(Hoops)).Curves.front().PlanarCondition;
        Expect(ImportDraftingCurves(Pod, Hoops));
        const DraftingCommandOutcome Loft = LoftDraftingCurves(Pod, ByCaption(Pod, "Hoop"));
        const DraftingCommandOutcome Image = MirrorDraftingPanels(Pod, 0);
        const DraftingCommandOutcome Closed = CloseDraftingPanels(Pod);
        double Exact = 0.0;
        for (int Index = 0; Index <= 2000; ++Index)
        {
            const double Y = 1.6 * Index / 2000.0, Weight = (Index == 0 || Index == 2000) ? 1.0 : (Index % 2 ? 4.0 : 2.0);
            Exact += Weight * Pi * (0.4 + 0.1 * std::sin(Pi * Y / 1.6)) * (0.3 + 0.05 * Y) * 1.6 / 6000.0;
        }
        const double Relative = std::abs(Pod.Summary.Volume - Exact) / Exact;
        Tally(PanelGate, Relative, Planar && Loft.AcceptedCondition && Image.AcceptedCondition && Closed.AcceptedCondition && !Pod.Summary.SheetCondition && Pod.Summary.SoundCondition && Relative <= 2.0e-3
              && Pod.Summary.Faces == 4, "pod: Planar = true round trip · loft → mirror → Close → solid V = " + std::to_string(Pod.Summary.Volume).substr(0, 8) + " vs " + std::to_string(Exact).substr(0, 8)
              + ", rel. (value) " + Loft.Refusal + Image.Refusal + Closed.Refusal);
        const DraftingCommandOutcome Again = CloseDraftingPanels(Pod);
        Tally(PanelGate, 0.0, !Again.AcceptedCondition && Again.Refusal.find("already closed") != std::string::npos, "second Close refused: " + Again.Refusal);
        Tally(PanelGate, 0.0, Reloads(Pod), "pod reload: planar flags + close step, byte-equal, same volume");

        // Network: 6 sections + 4 spines cross → L makes a Gordon panel; G1 on a network refuses.
        DraftingDocument Network = CreateDraftingDocument("Network");
        Expect(ImportDraftingCurves(Network, ReadVehicleCurves(SamplePath("SyntheticBonnetNetwork.toml"))));
        std::vector<std::string> Curves;
        for (const FeatureStep& Step : Network.Tree.Steps) Curves.push_back(Step.Label);
        const DraftingCommandOutcome Refused = LoftDraftingCurves(Network, Curves, 1);
        Tally(PanelGate, 0.0, !Refused.AcceptedCondition && Refused.Refusal.find("network") != std::string::npos, "G1 on crossing curves refused: " + Refused.Refusal);
        const DraftingCommandOutcome Gordon = LoftDraftingCurves(Network, Curves);
        double WorstAnalytic = 0.0;
        if (Gordon.AcceptedCondition)
        {
            const NurbsSurface& Surface = Network.Regeneration.Panels.begin()->second;
            for (int First = 0; First <= 60; ++First)
            {
                for (int Second = 0; Second <= 60; ++Second)
                {
                    const Vector3 Point = EvaluateNurbsSurface(Surface, { First / 60.0, Second / 60.0 }).Position;
                    const double  Xi = Point.X / 0.75, Eta = Point.Y / 1.1;
                    const double  Height = 0.78 + 0.16 * Eta - 0.06 * Eta * Eta - 0.085 * Xi * Xi + 0.012 * std::sin(Pi * Eta) * (1.0 - Xi * Xi);
                    const double  Across = (-0.17 * Xi - 0.024 * Xi * std::sin(Pi * Eta)) / 0.75;
                    const double  Along  = (0.16 - 0.12 * Eta + 0.012 * Pi * std::cos(Pi * Eta) * (1.0 - Xi * Xi)) / 1.1;
                    WorstAnalytic = std::max(WorstAnalytic, std::abs(Point.Z - Height) / std::sqrt(1.0 + Across * Across + Along * Along));
                }
            }
        }
        const bool NetworkStep = !Network.Tree.Steps.empty() && Network.Tree.Steps.back().Kind == FeatureStepKind::Network;
        Tally(PanelGate, WorstAnalytic * 1000.0, Gordon.AcceptedCondition && NetworkStep && Network.Summary.SheetCondition && Network.Summary.SoundCondition && Network.Summary.Faces == 1 && WorstAnalytic <= 5.0e-4,
              "10 crossing curves → Network step (Gordon), sheet sound, vs analytic H 61 × 61, worst [mm] (value) " + Gordon.Refusal);
        Tally(PanelGate, 0.0, Reloads(Network), "network reload byte-equal");
    }

    // ---- report
    const double Seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - Start).count();
    std::printf("# DraftingDocumentValidation\n\n");
    std::printf("Document layer of DraftingWorkspace over the SolidArcRemastered feature tree (%.2f s).\n\n", Seconds);
    std::printf("| Gate | Subject | Limit | Worst | Checks | Verdict |\n|------|---------|-------|-------|--------|---------|\n");
    long Failures = 0;
    for (const GateTally* Gate : { &ConstructGate, &PickGate, &BlendGate, &FaceGate, &ParameterGate, &AtomicGate, &HistoryGate, &FileGate, &VariableGate, &DrawingGate, &SurfacingGate, &PanelGate })
    {
        std::printf("| %s | %s | %s | %.3g | %ld | %s |\n", Gate->Code, Gate->Subject, Gate->Limit, Gate->Worst, Gate->Checks, Gate->Failures ? "🔴" : "🟢");
        Failures += Gate->Failures;
    }
    std::printf("\n## Walkthrough\n\n| Gate | Check | Value | Verdict |\n|------|-------|-------|---------|\n");
    for (const std::string& Row : Journal) std::printf("%s\n", Row.c_str());
    std::printf("\n## Saved document (TOML, first 40 lines)\n\n```toml\n");
    size_t Cursor = 0;
    for (int Line = 0; Line < 40 && Cursor < Saved.size(); ++Line)
    {
        const size_t Next = Saved.find('\n', Cursor);
        std::printf("%s\n", Saved.substr(Cursor, Next - Cursor).c_str());
        Cursor = Next == std::string::npos ? Saved.size() : Next + 1;
    }
    std::printf("```\n");
    return Failures == 0 ? 0 : 1;
}
