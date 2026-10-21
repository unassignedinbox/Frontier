/*====================================================================================================================================
                                                 DRAFTINGDOCUMENTVALIDATION.CPP
====================================================================================================================================*/
// 🧩 Step 9: DraftingWorkspace document gates — construct tiles, picks, blends, face edits, parameters, undo, files, variable blends

//------------------------------------------------------------------------------------------------------------------------
//                                                    INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Document/DraftingDocument.h"
#include "Document/DraftingDocumentCodec.h"
#include "Document/DraftingScene.h"

#include "Blend/BlendCatalogue.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
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

    // ---- report
    const double Seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - Start).count();
    std::printf("# DraftingDocumentValidation\n\n");
    std::printf("Document layer of DraftingWorkspace over the SolidArcRemastered feature tree (%.2f s).\n\n", Seconds);
    std::printf("| Gate | Subject | Limit | Worst | Checks | Verdict |\n|------|---------|-------|-------|--------|---------|\n");
    long Failures = 0;
    for (const GateTally* Gate : { &ConstructGate, &PickGate, &BlendGate, &FaceGate, &ParameterGate, &AtomicGate, &HistoryGate, &FileGate, &VariableGate })
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
