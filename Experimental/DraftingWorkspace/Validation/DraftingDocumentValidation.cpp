/*====================================================================================================================================
                                                 DRAFTINGDOCUMENTVALIDATION.CPP
====================================================================================================================================*/
// 🧩 Step 9: DraftingWorkspace document gates — construct tiles, picks, blends, face edits, parameters, undo, files, variable blends, drawn sketches

//------------------------------------------------------------------------------------------------------------------------
//                                                    INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Document/DraftingDocument.h"
#include "Document/DraftingImagePlane.h"
#include "Document/DraftingDocumentCodec.h"
#include "Document/DraftingScene.h"
#include "Document/VehicleCurveImport.h"
#include "Viewport/DraftingCamera.h"

#include "Blend/BlendCatalogue.h"
#include "Geometry/NurbsCarrier.h"
#include "Sketch/SketchProfile.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <numbers>
#include <set>
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
    GateTally EvaluationGate  = { "W13", "Surface evaluation: deviation needles are true closest points (residual ∥ normal, ≤ 61² lattice) and ≤ 0.2 mm on the fitted bonnet; zebra phase jumps at a G0 crease and is continuous at G2; zebra density tessellation", "≤ 0.2 mm · G2 ≤ 10⁻⁶ stripe" };
    GateTally BlueprintGate   = { "W14", "Blueprint study: image planes (BMW calibration landmarks → world, picture decode, codec round trip, refusals, re-import by name) and x / y-axis Construct cylinders", "≤ 10⁻⁵ m · ≤ 10⁻⁹" };
    GateTally MatcapGate      = { "W15", "Matcaps: built-in library optics (tinted metal / white plastic highlights, brushed anisotropy, roughness), picture fitting, per-face / body assignment without regeneration, undo, regeneration by persistent name, codec, refusals", "exact · 1 / 255" };
    GateTally BodiesGate      = { "W16", "Multi-body: New body (R) tiles keep exact separate volumes; per-body matcaps resolve face → body → default without regeneration; Combine (Q) difference consumes the tool and its delete restores it; targeted Construct / blend touch one body; cross-body refusals; scene body spans; undo; codec", "≤ 10⁻⁹ · exact" };
    GateTally TrimGate        = { "W18", "Curve trim (Plasticity T): a pentagram drawn as one chain refuses to extrude, T on the five inner pieces leaves the star (area vs shoelace, extrude exact); preview leaves the document alone; cross-sketch trims (circle → arc, side split) with undo / redo; a trim that would break an extrude is refused; an emptied sketch goes; codec", "≤ 10⁻¹² · ≤ 10⁻⁹ · exact" };
    GateTally CutGate         = { "W17", "Trim / split: C cuts a block by a drawn line (2 + 6 exact) and a circle (π/4), the outside piece keeps key and matcap; X removes picked bodies (undo / redo); vehicle loft cut by a character line along −z → two sound sheets, X the piece; refusals; codec", "≤ 10⁻⁹ · exact" };
    GateTally MoveGate        = { "W19", "Move / rotate / scale (G / R / S): Transform steps keep volume (×8 for scale 2), the scene and picking follow the body, face selection survives by name, undo exact; refusals untouched", "≤ 10⁻⁹ · exact" };
    GateTally CopyGate        = { "W20", "Duplicate (Shift+D) and copy / paste (Ctrl+C / Ctrl+V): moved copy = new active selected body with its own caption; paste after X copies the body as it stood at copy time; refusals", "≤ 10⁻⁹ · exact" };
    GateTally HideGate        = { "W21", "Hide family (H, Shift+H, Alt+H, Ctrl+H): scene bodies and picking follow, selection drops hidden bodies, undo, codec round trip, refusals", "exact" };
    GateTally CurveGate       = { "W22", "Join (J) two open drawings into a closed square (extrudes to 1); offset (O) it by ¼ (2.25); pipe (P) along a drawn line (π r² L); refusals; codec byte-equal with kinds 13 / 14 / 15", "≤ 10⁻⁹ · exact" };
    GateTally RevolveGate     = { "W29", "Revolve (Alt+R): annulus 3π about the proposed y-axis (New); a plate about its own edge line united → 4.625π, undo / redo; x-axis fallback quarter turn 3π/2; refusals (angle, crossing axis, no sketch, nothing to subtract); codec writes the point axis only without a line", "≤ 10⁻⁹ · exact" };
    GateTally EvolutionGate   = { "W30", "Evolution (Shift+P / Alt+E): rectangle along a proposed line sketch = prism 1; circle along a quarter arc = Pappus π²/16; a square swept through a block as Difference = 1.875; refusals; codec Kind 16 + Path", "≤ 10⁻⁹ planar · ≤ 10⁻⁶ NURBS skin" };
    GateTally ReplayGate      = { "W31", "Incremental replay: on a 27-step studded plate and the vehicle bonnet every command / undo / redo / preview clone equals a full replay with the cache off (volumes, names, tokens, step reports, saved file); appends resume at the new step, a parameter edit at its first binding step, undo / redo / clone restore every step; only the 8 newest history entries keep checkpoints", "identical · undo / redo / clone ≥ 10× · tail edit ≥ 2×" };
    GateTally XRayGate        = { "W32", "X-ray (Alt+Z) pick-through and box edges on a 4 × 2 × 1 block in the isometric view: the ray at the hidden back-bottom edge picks the top face opaque and that edge in x-ray, √(1.25² + 1) = 1.6008 farther along the ray; a visible edge still picks in x-ray; the full-viewport box takes the 9 edges off the far corner (0, 2, 0) opaque and all 12 in x-ray; an empty box takes none", "all equal" };
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

    // ---- W13 surface evaluation (plan V8): deviation map and zebra phase through the document and scene functions.
    {
        auto Sections = [](DraftingDocument& Document)
        {
            std::vector<std::string> Labels;
            for (const FeatureStep& Step : Document.Tree.Steps) if (Document.Captions[Step.Label].rfind("Section", 0) == 0) Labels.push_back(Step.Label);
            return Labels;
        };
        auto Domain = [](const NurbsSurface& Surface, bool AlongU) { return MeasureNurbsSurfaceDomain(*Surface.Definition, AlongU); };
        auto Compact = [](double Value) { char Text[32]; std::snprintf(Text, sizeof(Text), "%.3g", Value); return std::string(Text); };

        // ① deviation map on the six-section bonnet: every imported point, closest-point residual, lattice bound.
        DraftingDocument Fitted = CreateDraftingDocument("Deviation");
        Expect(ImportDraftingCurves(Fitted, ReadVehicleCurves(DRAFTING_VEHICLE_SAMPLE)));
        Expect(LoftDraftingCurves(Fitted, Sections(Fitted)));
        const DraftingDeviationMap Map = MeasureDraftingDeviation(Fitted);
        size_t Imported = 0;
        for (const FeatureStep& Step : Fitted.Tree.Steps) if (Step.Kind == FeatureStepKind::SpaceCurve) Imported += Step.Points.size();
        const NurbsSurface& Sheet = Fitted.Regeneration.Panels.begin()->second;
        const NurbsDomain   AlongU = Domain(Sheet, true), AlongV = Domain(Sheet, false);
        double WorstResidual = 0.0, WorstLattice = 0.0;
        for (const DraftingDeviationNeedle& Needle : Map.Needles)
        {
            const Vector2 Foot   = ProjectOntoNurbsSurface(Sheet, Needle.Point, { 0.5 * (AlongU.Low + AlongU.High), 0.5 * (AlongV.Low + AlongV.High) });
            const bool    Inside = Foot.X > AlongU.Low + 1.0e-9 && Foot.X < AlongU.High - 1.0e-9 && Foot.Y > AlongV.Low + 1.0e-9 && Foot.Y < AlongV.High - 1.0e-9;
            const Vector3 Normal = EvaluateNurbsNormal(Sheet, Foot);
            const Vector3 Gap    = Needle.Point - Needle.Foot;
            if (Inside && Needle.Distance > 1.0e-12) WorstResidual = std::max(WorstResidual, Length(Cross(Gap, Normal)) / Needle.Distance);
            double Lattice = std::numeric_limits<double>::infinity();
            for (int First = 0; First <= 60; ++First)
                for (int Second = 0; Second <= 60; ++Second)
                    Lattice = std::min(Lattice, Distance(EvaluateNurbsSurface(Sheet, { AlongU.Low + (AlongU.High - AlongU.Low) * First / 60.0,
                                                                                         AlongV.Low + (AlongV.High - AlongV.Low) * Second / 60.0 }).Position, Needle.Point));
            WorstLattice = std::max(WorstLattice, Needle.Distance - Lattice);
        }
        Tally(EvaluationGate, Map.Worst * 1000.0, Map.Needles.size() == Imported && Imported == 286 && Map.Worst <= 2.0e-4 && Map.Revision == Fitted.Revision,
              "deviation map, six-section loft: " + std::to_string(Map.Needles.size()) + " needles (one per imported point), worst [mm] (value), mean "
              + std::to_string(Map.Mean * 1000.0) + " mm — W11 measures the same sheet");
        Tally(EvaluationGate, 0.0, WorstResidual <= 1.0e-6, "interior needles run along the foot normal: |gap × N| / |gap| = " + Compact(WorstResidual) + " (≤ 10⁻⁶)");
        Tally(EvaluationGate, 0.0, WorstLattice <= 1.0e-9, "no needle longer than the best of a 61 × 61 lattice: excess " + Compact(WorstLattice * 1000.0) + " mm");

        // ② zebra: lift pole 2 of Section3 by 20 mm, front + rear lofts; phase across the shared Section3 at G0 and G2.
        auto SeamJump = [&](const DraftingDocument& Document)
        {
            const Vector3 Forward = Normalize(Vector3{ -1.0, 1.0, -1.0 }), Up = Normalize(Vector3{ -1.0, 1.0, 2.0 });
            std::vector<const NurbsSurface*> Panels;
            for (const auto& [Name, Surface] : Document.Regeneration.Panels) Panels.push_back(&Surface);
            if (Panels.size() != 2) return std::numeric_limits<double>::infinity();
            double Worst = 0.0;
            for (int Edge = 0; Edge < 2; ++Edge)
            {
                const NurbsDomain Across = Domain(*Panels[0], false), Along = Domain(*Panels[0], true);
                const double      Fixed  = Edge ? Across.High : Across.Low;
                const Vector3     Probe  = EvaluateNurbsSurface(*Panels[0], { 0.5 * (Along.Low + Along.High), Fixed }).Position;
                const Vector2     Near   = ProjectOntoNurbsSurface(*Panels[1], Probe, { 0.5, 0.5 });
                if (Distance(EvaluateNurbsSurface(*Panels[1], Near).Position, Probe) > 1.0e-6) continue;  // not the shared edge
                for (int Index = 1; Index < 40; ++Index)
                {
                    const Vector2 Here  = { Along.Low + (Along.High - Along.Low) * Index / 40.0, Fixed };
                    const Vector3 Point = EvaluateNurbsSurface(*Panels[0], Here).Position;
                    const Vector2 There = ProjectOntoNurbsSurface(*Panels[1], Point, Near);
                    Worst = std::max(Worst, std::abs(DraftingZebraPhase(EvaluateNurbsNormal(*Panels[0], Here), Forward, Up, 64.0)
                                                   - DraftingZebraPhase(EvaluateNurbsNormal(*Panels[1], There), Forward, Up, 64.0)));
                }
            }
            return Worst;
        };
        DraftingDocument Creased = CreateDraftingDocument("Zebra");
        Expect(ImportDraftingCurves(Creased, ReadVehicleCurves(DRAFTING_VEHICLE_SAMPLE)));
        const std::vector<std::string> Labels = Sections(Creased);
        const Vector3 Pole = Creased.Regeneration.Curves.at(Labels[2]).Curve.Definition->Poles[2];
        Expect(MoveDraftingPole(Creased, Labels[2], 2, Pole + Vector3{ 0.0, 0.0, 0.02 }));
        Expect(LoftDraftingCurves(Creased, { Labels[0], Labels[1], Labels[2] }));
        const DraftingCommandOutcome Plain = LoftDraftingCurves(Creased, { Labels[2], Labels[3], Labels[4], Labels[5] }, 0);
        const double PlainJump = SeamJump(Creased);
        Tally(EvaluationGate, 0.0, Plain.AcceptedCondition && PlainJump > 0.05 && PlainJump < 1.0e3,
              "G0 rear loft on the lifted Section3: zebra phase jump across the seam " + Compact(PlainJump) + " stripes of 64 (> 0.05) — visibly broken stripes");
        (void)UndoDraftingCommand(Creased);
        const DraftingCommandOutcome Smooth = LoftDraftingCurves(Creased, { Labels[2], Labels[3], Labels[4], Labels[5] }, 2);
        const double SmoothJump = SeamJump(Creased);
        Tally(EvaluationGate, 0.0, Smooth.AcceptedCondition && SmoothJump <= 1.0e-6, "G2 rear loft: zebra phase jump " + Compact(SmoothJump) + " stripes of 64 (≤ 10⁻⁶) — continuous stripes");

        // ③ zebra density tessellation: interpolated normals need short sides on the sheet.
        const DraftingScene Coarse = BuildDraftingScene(Creased);
        const DraftingScene Fine   = BuildDraftingScene(Creased, 1.5e-3, true);
        const size_t CoarseCount = Coarse.Corners.size() / 3, FineCount = Fine.Corners.size() / 3;
        Tally(EvaluationGate, 0.0, FineCount >= 20 * CoarseCount && Fine.Generation != Coarse.Generation && Fine.Revision == Coarse.Revision,
              "zebra tessellation " + std::to_string(FineCount) + " vs " + std::to_string(CoarseCount) + " triangles (≥ 20×); distinct scene generation re-uploads the GPU buffers");
    }

    // ---- W14 blueprint study: image planes from the committed BMW trace, picture decode, codec, refusals, axis cylinders.
    {
        auto Compact = [](double Value) { char Text[32]; std::snprintf(Text, sizeof(Text), "%.3g", Value); return std::string(Text); };
        auto Find    = [](const std::vector<DraftingImagePlane>& Planes, const std::string& Name) -> const DraftingImagePlane*
        {
            for (const DraftingImagePlane& Plane : Planes) if (Plane.Name == Name) return &Plane;
            return nullptr;
        };
        // Crop pixel (column, row) → world through the plane's parallelogram.
        auto Pixel = [](const DraftingImagePlane& Plane, double Column, double Row)
        {
            return LocateDraftingImagePoint(Plane, (Column - Plane.Crop[0]) / double(Plane.Crop[2] - Plane.Crop[0]), (Row - Plane.Crop[1]) / double(Plane.Crop[3] - Plane.Crop[1]));
        };

        // ① landmarks of BmwM3E90Calibration.toml land on the car frame (nose y 0, tail y 4.58, ground z 0, mirror plane x 0).
        std::string Sample = DRAFTING_VEHICLE_SAMPLE;
        Sample = Sample.substr(0, Sample.find_last_of("/\\") + 1) + "BmwM3E90Sections.toml";
        const VehicleCurveFile Bmw = ReadVehicleCurves(Sample);
        const DraftingImagePlane* Side  = Find(Bmw.ImagePlanes, "Side");
        const DraftingImagePlane* Top   = Find(Bmw.ImagePlanes, "Top");
        const DraftingImagePlane* Front = Find(Bmw.ImagePlanes, "Front");
        // The side view sets the scale (4580 mm over its bumper lines), so its landmarks are exact; the top / front views of
        // the sheet agree with it to about one pixel (2.08 mm) — the drawing's own alignment, gated at 1.5 px.
        constexpr double Millimetre = 2.0771e-3;                                             // [m/px] - Calibration scale
        double Landmark = INFINITY, Alignment = INFINITY;
        if (Side && Top && Front)
        {
            Landmark  = std::max({ std::abs(Pixel(*Side, 30, 1642).Y - 0.0), std::abs(Pixel(*Side, 2235, 1642).Y - 4.58), std::abs(Pixel(*Side, 30, 1642).Z) });
            Alignment = std::max({ std::abs(Pixel(*Top, 25, 2388).Y), std::abs(Pixel(*Top, 2231, 2388).Y - 4.58), std::abs(Pixel(*Top, 25, 2388).X),
                                   std::abs(Pixel(*Front, 510, 727).X), std::abs(Pixel(*Front, 510, 727).Z) });
        }
        Tally(BlueprintGate, Landmark, Bmw.Refusal.empty() && Bmw.Curves.size() == 24 && Bmw.ImagePlanes.size() == 3 && Landmark <= 1.0e-5,
              "BMW trace: 24 sections + 3 planes; side bumper / ground pixels → y 0, y 4.58, z 0, worst [m] (value)");
        Tally(BlueprintGate, 0.0, Alignment <= 1.5 * Millimetre,
              "top nose / tail / centre line and front centre / ground pixels → world within " + Compact(Alignment * 1000.0) + " mm (sheet alignment, ≤ 1.5 px)");
        const bool Facing = Side && Top && Front && std::abs(Side->Corner.X + 1.2) <= 1.0e-12 && Side->Right.X == 0.0 && Side->Down.X == 0.0
                         && Top->Right.Z == 0.0 && Top->Down.Z == 0.0 && Front->Right.Y == 0.0 && Front->Down.Y == 0.0 && Top->Corner.Z < 0.0 && Front->Corner.Y > 4.58;
        Tally(BlueprintGate, 0.0, Facing, "Side ⟂ x at x = −1.2 (behind, seen from Right) · Top ⟂ z under the ground · Front ⟂ y behind the tail");

        // ② picture decode: a 3 × 2 24-bit BMP written here (bottom-up rows) → RGBA8, top row first; a missing file refuses.
        const std::string Bitmap = (std::filesystem::temp_directory_path() / "DraftingImagePlaneValidation.bmp").string();
        {
            const uint8_t Colours[2][3][3] = { { { 255, 0, 0 }, { 0, 255, 0 }, { 0, 0, 255 } }, { { 10, 20, 30 }, { 40, 50, 60 }, { 70, 80, 90 } } };   // [row][column] RGB, top row first
            std::vector<uint8_t> Bytes(54, 0);
            const uint32_t Stride = 12, Size = 54 + Stride * 2;
            auto Put = [&](size_t At, uint32_t Value, int Count) { for (int Index = 0; Index < Count; ++Index) Bytes[At + Index] = uint8_t(Value >> (8 * Index)); };
            Bytes[0] = 'B'; Bytes[1] = 'M'; Put(2, Size, 4); Put(10, 54, 4); Put(14, 40, 4); Put(18, 3, 4); Put(22, 2, 4); Put(26, 1, 2); Put(28, 24, 2); Put(34, Stride * 2, 4);
            for (int Row = 1; Row >= 0; --Row)
            {
                for (int Column = 0; Column < 3; ++Column) for (int Channel = 2; Channel >= 0; --Channel) Bytes.push_back(Colours[Row][Column][Channel]);
                for (uint32_t Pad = 9; Pad < Stride; ++Pad) Bytes.push_back(0);
            }
            std::ofstream(Bitmap, std::ios::binary).write(reinterpret_cast<const char*>(Bytes.data()), std::streamsize(Bytes.size()));
            const DraftingPicture Picture = DecodeDraftingPicture(Bitmap);
            bool Exact = Picture.Refusal.empty() && Picture.Width == 3 && Picture.Height == 2 && Picture.Pixels.size() == 24;
            for (int Row = 0; Exact && Row < 2; ++Row)
                for (int Column = 0; Column < 3; ++Column)
                    for (int Channel = 0; Channel < 3; ++Channel) Exact = Exact && Picture.Pixels[(Row * 3 + Column) * 4 + Channel] == Colours[Row][Column][Channel];
            Tally(BlueprintGate, 0.0, Exact && Picture.Pixels[3] == 255, "3 × 2 BMP → RGBA8, every pixel exact, top row first, alpha 255");
            std::filesystem::remove(Bitmap);
            const DraftingPicture Missing = DecodeDraftingPicture(Bitmap);
            Tally(BlueprintGate, 0.0, !Missing.Refusal.empty() && Missing.Pixels.empty(), "missing picture refused (no throw)");
        }

        // ③ import keeps the planes beside the curves; a second import replaces planes by Name; codec round trip byte-equal.
        DraftingDocument Study = CreateDraftingDocument("Blueprint");
        const DraftingCommandOutcome First  = ImportDraftingCurves(Study, Bmw);
        const size_t                 Steps  = Study.Tree.Steps.size();
        const DraftingCommandOutcome Second = ImportDraftingCurves(Study, Bmw);
        Tally(BlueprintGate, 0.0, First.AcceptedCondition && Second.AcceptedCondition && Steps == 24 && Study.Tree.Steps.size() == 48 && Study.ImagePlanes.size() == 3,
              "import → 24 curves + 3 planes; re-import adds curves, replaces planes by Name (still 3)");
        Study.ImagePlanes[1].VisibleCondition = false;
        Study.ImagePlanes[2].Opacity          = 0.3;
        const std::string            Encoded = EncodeDraftingDocument(Study);
        const DraftingDocumentFile   Decoded = DecodeDraftingDocument(Encoded);
        DraftingDocument             Again   = CreateDraftingDocument(Decoded.Title);
        const DraftingCommandOutcome Opened  = ReplaceDraftingTree(Again, Decoded.Tree, Decoded.Captions, "Open");
        Again.ImagePlanes = Decoded.ImagePlanes;
        Tally(BlueprintGate, 0.0, Decoded.Refusal.empty() && Opened.AcceptedCondition && Decoded.ImagePlanes.size() == 3 && !Decoded.ImagePlanes[1].VisibleCondition
              && Decoded.ImagePlanes[2].Opacity == 0.3 && EncodeDraftingDocument(Again) == Encoded, "document with planes (one hidden, one at ink 0.3): save → load → save byte-equal");
        const std::string Plain = EncodeDraftingDocument(CreateDraftingDocument("Plain"));
        Tally(BlueprintGate, 0.0, Plain.find("ImagePlanes") == std::string::npos, "documents without planes write no ImagePlanes key (older files unchanged)");

        // ④ refusals: a degenerate plane or an opacity outside [0, 1] rejects the whole curves file (no partial import).
        const std::string Curve = "Format = \"VehicleCurves\"\nUnits = \"m\"\n[[Curves]]\nName = \"Line\"\nPoints = [[0, 0, 0], [1, 0, 0], [2, 0, 0.5]]\n";
        auto PlaneRow = [](const std::string& Right, double Opacity)
        {
            return "[[ImagePlanes]]\nName = \"Side\"\nPath = \"Side.png\"\nCorner = [0, 0, 1]\nRight = " + Right + "\nDown = [0, 0, -1]\nOpacity = " + std::to_string(Opacity) + "\n";
        };
        const VehicleCurveFile Fine     = DecodeVehicleCurves(Curve + PlaneRow("[0, 4, 0]", 0.5));
        const VehicleCurveFile Flat     = DecodeVehicleCurves(Curve + PlaneRow("[0, 0, -2]", 0.5));
        const VehicleCurveFile Overdone = DecodeVehicleCurves(Curve + PlaneRow("[0, 4, 0]", 1.5));
        Tally(BlueprintGate, 0.0, Fine.Refusal.empty() && Fine.ImagePlanes.size() == 1 && !Flat.Refusal.empty() && Flat.Curves.empty() && !Overdone.Refusal.empty() && Overdone.Curves.empty(),
              "zero-area plane refused (" + Flat.Refusal + ") · opacity 1.5 refused — no curves kept");

        // ⑤ Construct · Cylinder · Axis x / y (wheel arches): exact volume πr²h and the extent along +axis from Position.
        for (int Axis = 0; Axis < 2; ++Axis)
        {
            DraftingDocument Wheel = CreateDraftingDocument("Wheel");
            DraftingConstructRequest Request;
            Request.Kind     = DraftingConstructKind::Cylinder;
            Request.Axis     = Axis;
            Request.Position = { 0.62, 0.805, 0.33 };
            Request.Radius   = 0.37;
            Request.Height   = 0.5;
            const DraftingCommandOutcome Built = ConstructDraftingPrimitive(Wheel, Request);
            const double Exact    = std::numbers::pi * 0.37 * 0.37 * 0.5;
            const double Relative = std::abs(Wheel.Summary.Volume - Exact) / Exact;
            const DraftingScene Scene = BuildDraftingScene(Wheel, 1.5e-3);
            const double Base   = Axis == 0 ? 0.62 : 0.805;
            const double Low    = Scene.BoundsLow[Axis], High = Scene.BoundsHigh[Axis];
            const double Across = Scene.BoundsHigh[2] - Scene.BoundsLow[2];
            const double Extent = std::max({ std::abs(Low - Base), std::abs(High - Base - 0.5), std::abs(Across - 0.74) });
            Tally(BlueprintGate, Relative, Built.AcceptedCondition && Wheel.Summary.SoundCondition && Relative <= 1.0e-9 && Extent <= 2.0e-3,
                  std::string(Axis == 0 ? "Axis x" : "Axis y") + " cylinder r 0.37 h 0.5: |V − πr²h| / V (value); " + Compact(Low) + " … " + Compact(High) + " along the axis, Ø " + Compact(Across) + " in z");
        }
    }

    // ---- W15 matcaps: built-in library optics, picture fitting, per-face / body assignment without regeneration, undo,
    //      persistence through regeneration, codec, refusals.
    {
        auto Compact = [](double Value) { char Text[32]; std::snprintf(Text, sizeof(Text), "%.3g", Value); return std::string(Text); };
        auto Luminance = [](const DraftingPicture& Picture, size_t Texel)
        {
            return 0.2126 * Picture.Pixels[Texel * 4] + 0.7152 * Picture.Pixels[Texel * 4 + 1] + 0.0722 * Picture.Pixels[Texel * 4 + 2];
        };
        // Texels inside 0.9 of the disc radius.
        auto Inside = [](const DraftingPicture& Picture, uint32_t Column, uint32_t Row)
        {
            const double X = 2.0 * (Column + 0.5) / Picture.Width - 1.0, Y = 2.0 * (Row + 0.5) / Picture.Height - 1.0;
            return X * X + Y * Y < 0.81;
        };
        // Mean |∂L/∂x| over mean |∂L/∂y| inside the disc (brushed metal blurs along x).
        auto Anisotropy = [&](const DraftingPicture& Picture)
        {
            double Across = 0.0, Down = 0.0;
            for (uint32_t Row = 1; Row < Picture.Height; ++Row)
                for (uint32_t Column = 1; Column < Picture.Width; ++Column)
                {
                    if (!Inside(Picture, Column, Row)) continue;
                    const size_t Texel = size_t(Row) * Picture.Width + Column;
                    Across += std::abs(Luminance(Picture, Texel) - Luminance(Picture, Texel - 1));
                    Down   += std::abs(Luminance(Picture, Texel) - Luminance(Picture, Texel - Picture.Width));
                }
            return Across / std::max(Down, 1e-9);
        };
        // Colour of the brightest texel inside the disc.
        auto Brightest = [&](const DraftingPicture& Picture)
        {
            size_t Best = 0; double Peak = -1.0;
            for (uint32_t Row = 0; Row < Picture.Height; ++Row)
                for (uint32_t Column = 0; Column < Picture.Width; ++Column)
                    if (Inside(Picture, Column, Row) && Luminance(Picture, size_t(Row) * Picture.Width + Column) > Peak)
                    {
                        Peak = Luminance(Picture, size_t(Row) * Picture.Width + Column);
                        Best = size_t(Row) * Picture.Width + Column;
                    }
            return std::array<double, 3>{ double(Picture.Pixels[Best * 4]), double(Picture.Pixels[Best * 4 + 1]), double(Picture.Pixels[Best * 4 + 2]) };
        };
        // Luminance spread (standard deviation) inside the disc: sharp reflections spread wide.
        auto Spread = [&](const DraftingPicture& Picture)
        {
            double Sum = 0.0, Squares = 0.0; size_t Count = 0;
            for (uint32_t Row = 0; Row < Picture.Height; ++Row)
                for (uint32_t Column = 0; Column < Picture.Width; ++Column)
                    if (Inside(Picture, Column, Row))
                    {
                        const double Value = Luminance(Picture, size_t(Row) * Picture.Width + Column);
                        Sum += Value; Squares += Value * Value; ++Count;
                    }
            const double Mean = Sum / double(Count);
            return std::sqrt(std::max(0.0, Squares / double(Count) - Mean * Mean));
        };

        // ① the library: 13 built-ins, square RGBA8, opaque, deterministic; unknown names refuse
        const std::vector<DraftingMatcapDescription>& BuiltIn = DraftingBuiltInMatcaps();
        bool Shaped = BuiltIn.size() == 13, Repeatable = true;
        std::map<std::string, DraftingPicture> Composed;
        for (const DraftingMatcapDescription& Description : BuiltIn)
        {
            const DraftingPicture Picture = ComposeDraftingMatcap(Description.Name);
            Shaped = Shaped && Picture.Refusal.empty() && Picture.Width == DraftingMatcapExtent && Picture.Height == DraftingMatcapExtent
                  && Picture.Pixels.size() == size_t(DraftingMatcapExtent) * DraftingMatcapExtent * 4;
            for (size_t Index = 3; Shaped && Index < Picture.Pixels.size(); Index += 4) Shaped = Picture.Pixels[Index] == 255;
            Repeatable = Repeatable && ComposeDraftingMatcap(Description.Name).Pixels == Picture.Pixels;
            Composed[Description.Name] = Picture;
        }
        const DraftingPicture Unknown = ComposeDraftingMatcap("Unobtainium");
        const DraftingMatcapLibrary Plain = AssembleDraftingMatcaps({});
        Tally(MatcapGate, 0.0, Shaped && Repeatable && !Unknown.Refusal.empty() && Plain.Names.size() == 14 && Plain.Names[0] == DraftingStudioMatcap && Plain.Pictures.size() == 13
                               && DraftingMatcapLayer(Plain, "Brass") == 5 && DraftingMatcapLayer(Plain, "Unobtainium") == 0,
              "13 built-ins 256 × 256 RGBA8, opaque, composed twice bit-equal; library Studio + 13 (Brass = layer 5); unknown name refused / layer 0");

        // ② optics: metal highlights carry the tint, plastic highlights stay white; brushed blurs along x; roughness spreads less
        const std::array<double, 3> BrassPeak = Brightest(Composed["Brass"]);
        const std::array<double, 3> RedPeak   = Brightest(Composed["RedPlastic"]);
        const double BrassTint = BrassPeak[2] / BrassPeak[0];
        const double RedWhite  = std::min({ RedPeak[0], RedPeak[1], RedPeak[2] }) / std::max({ RedPeak[0], RedPeak[1], RedPeak[2] });
        Tally(MatcapGate, 0.0, BrassTint < 0.75 && RedWhite > 0.8,
              "brightest texel: Brass blue / red " + Compact(BrassTint) + " (< 0.75, tinted metal) · Red plastic min / max " + Compact(RedWhite) + " (> 0.8, white Fresnel)");
        const double Brushed = Anisotropy(Composed["BrushedSteel"]), Mirror = Anisotropy(Composed["Chrome"]);
        Tally(MatcapGate, 0.0, Brushed < 0.6 * Mirror, "|∂L/∂x| / |∂L/∂y|: Brushed steel " + Compact(Brushed) + " < 0.6 × Chrome " + Compact(Mirror) + " (anisotropic)");
        const double Sharp = Spread(Composed["Chrome"]), Satin = Spread(Composed["SatinAluminium"]), Matte = Spread(Composed["Clay"]);
        Tally(MatcapGate, 0.0, Sharp > Satin && Satin > Matte,
              "luminance spread inside the disc: Chrome " + Compact(Sharp) + " > Satin aluminium " + Compact(Satin) + " > Clay " + Compact(Matte));

        // ③ user pictures: centre square crop + bilinear fit; a 4 × 2 picture keeps its middle two columns
        const std::string Bitmap = (std::filesystem::temp_directory_path() / "DraftingMatcapValidation.bmp").string();
        const uint8_t Columns[4][3] = { { 200, 0, 0 }, { 0, 200, 0 }, { 0, 0, 200 }, { 200, 200, 0 } };
        {
            std::vector<uint8_t> Bytes(54, 0);
            const uint32_t Stride = 12, Size = 54 + Stride * 2;
            auto Put = [&](size_t At, uint32_t Value, int Count) { for (int Index = 0; Index < Count; ++Index) Bytes[At + Index] = uint8_t(Value >> (8 * Index)); };
            Bytes[0] = 'B'; Bytes[1] = 'M'; Put(2, Size, 4); Put(10, 54, 4); Put(14, 40, 4); Put(18, 4, 4); Put(22, 2, 4); Put(26, 1, 2); Put(28, 24, 2); Put(34, Stride * 2, 4);
            for (int Row = 1; Row >= 0; --Row)
                for (int Column = 0; Column < 4; ++Column) for (int Channel = 2; Channel >= 0; --Channel) Bytes.push_back(Columns[Column][Channel]);
            std::ofstream(Bitmap, std::ios::binary).write(reinterpret_cast<const char*>(Bytes.data()), std::streamsize(Bytes.size()));
        }
        const DraftingPicture Fitted = FitDraftingMatcapPicture(DecodeDraftingPicture(Bitmap));
        auto Near = [&](uint32_t Column, uint32_t Row, const uint8_t* Expected)
        {
            const uint8_t* Pixel = &Fitted.Pixels[(size_t(Row) * Fitted.Width + Column) * 4];
            return std::abs(Pixel[0] - Expected[0]) <= 1 && std::abs(Pixel[1] - Expected[1]) <= 1 && std::abs(Pixel[2] - Expected[2]) <= 1 && Pixel[3] == 255;
        };
        Tally(MatcapGate, 0.0, Fitted.Refusal.empty() && Fitted.Width == 256 && Fitted.Height == 256 && Near(64, 10, Columns[1]) && Near(192, 200, Columns[2]),
              "4 × 2 BMP → 256 × 256: quarter columns show source columns 2 and 3 (centre crop), within 1 / 255, opaque");

        // ④ assignment on a block: two faces Brass, body Brushed steel — no regeneration, one undo entry each
        DraftingDocument Shop = CreateDraftingDocument("Matcaps");
        DraftingConstructRequest Stock;
        Stock.Kind = DraftingConstructKind::Block; Stock.Width = 2.0; Stock.Depth = 1.0; Stock.Height = 0.5;
        Expect(ConstructDraftingPrimitive(Shop, Stock));
        DraftingScene ShopScene = BuildDraftingScene(Shop);
        const std::string First = ShopScene.Faces.size() == 6 ? ShopScene.Faces[0].Name : "", Second = ShopScene.Faces.size() == 6 ? ShopScene.Faces[1].Name : "";
        const uint64_t Revision = Shop.Revision;
        const size_t   Entries  = Shop.UndoEntries.size();
        SelectDraftingName(Shop, DraftingSelectionKind::Face, First, false);
        SelectDraftingName(Shop, DraftingSelectionKind::Face, Second, true);
        const DraftingCommandOutcome FaceOutcome = AssignDraftingMatcap(Shop, "Brass");
        ClearDraftingSelection(Shop);
        const DraftingCommandOutcome BodyOutcome = AssignDraftingMatcap(Shop, "BrushedSteel");
        const DraftingMatcapLibrary Library = AssembleDraftingMatcaps(Shop.Appearance);
        auto LayerCounts = [&](const DraftingScene& Scene)
        {
            std::map<uint32_t, int> Counts;
            for (const uint32_t Word : ComposeDraftingMatcapLayers(Scene, Shop.Appearance, Library)) Counts[Word >> DraftingMatcapLayerShift] += 1;
            return Counts;
        };
        std::map<uint32_t, int> Counts = LayerCounts(ShopScene);
        const uint32_t BrassLayer = DraftingMatcapLayer(Library, "Brass"), SteelLayer = DraftingMatcapLayer(Library, "BrushedSteel");
        Tally(MatcapGate, 0.0, FaceOutcome.AcceptedCondition && BodyOutcome.AcceptedCondition && Shop.Revision == Revision && Shop.UndoEntries.size() == Entries + 2
                               && Shop.UndoEntries.back().AppearanceCondition && Counts[BrassLayer] == 2 && Counts[SteelLayer] == 4 && Counts.size() == 2,
              "2 faces → Brass, body → Brushed steel: state words 2 × layer " + std::to_string(BrassLayer) + " + 4 × layer " + std::to_string(SteelLayer)
              + "; revision unchanged (no regeneration); 2 matcap-only undo entries");

        // ⑤ refusals leave the appearance and the history alone
        const DraftingAppearance Kept = Shop.Appearance;
        const DraftingCommandOutcome Unnamed = AssignDraftingMatcap(Shop, "Unobtainium");
        const DraftingCommandOutcome Again   = AssignDraftingMatcap(Shop, "BrushedSteel");
        const bool Untouched = Shop.Appearance.BodyMatcap == Kept.BodyMatcap && Shop.Appearance.FaceMatcaps == Kept.FaceMatcaps && Shop.UndoEntries.size() == Entries + 2;
        Tally(MatcapGate, 0.0, !Unnamed.AcceptedCondition && !Again.AcceptedCondition && Untouched,
              "unknown matcap refused (" + Unnamed.Refusal + ") · repeat refused (" + Again.Refusal + ") · appearance and history unchanged");

        // ⑥ undo / redo restore the appearance exactly and never regenerate
        UndoDraftingCommand(Shop);
        const bool BodyUndone = Shop.Appearance.BodyMatcap == DraftingStudioMatcap && Shop.Appearance.FaceMatcaps.size() == 2;
        UndoDraftingCommand(Shop);
        const bool FacesUndone = Shop.Appearance.FaceMatcaps.empty() && Shop.Revision == Revision;
        RedoDraftingCommand(Shop);
        RedoDraftingCommand(Shop);
        Tally(MatcapGate, 0.0, BodyUndone && FacesUndone && Shop.Appearance.BodyMatcap == Kept.BodyMatcap && Shop.Appearance.FaceMatcaps == Kept.FaceMatcaps && Shop.Revision == Revision,
              "undo → body Studio, undo → no face matcaps, redo × 2 → identical appearance; revision unchanged throughout");

        // ⑦ a parameter edit regenerates: assignments follow the persistent names (none dangling)
        const DraftingCommandOutcome Deeper = AssignDraftingParameter(Shop, "Block1.Depth", 1.5);
        ShopScene = BuildDraftingScene(Shop);
        Counts    = LayerCounts(ShopScene);
        Tally(MatcapGate, 0.0, Deeper.AcceptedCondition && Shop.Revision != Revision && Counts[BrassLayer] == 2 && Counts[SteelLayer] == 4 && CountDanglingMatcaps(ShopScene, Shop.Appearance) == 0,
              "Block1.Depth 1 → 1.5 regenerates; still 2 Brass + 4 Brushed steel faces, 0 dangling assignments");

        // ⑧ Alt+M: the selected face returns to the body matcap; with nothing selected the body returns to Studio
        SelectDraftingName(Shop, DraftingSelectionKind::Face, First, false);
        const DraftingCommandOutcome FaceRemoval = RemoveDraftingMatcap(Shop);
        ClearDraftingSelection(Shop);
        const DraftingCommandOutcome BodyRemoval = RemoveDraftingMatcap(Shop);
        const DraftingCommandOutcome Nothing     = RemoveDraftingMatcap(Shop);
        Tally(MatcapGate, 0.0, FaceRemoval.AcceptedCondition && BodyRemoval.AcceptedCondition && !Nothing.AcceptedCondition && Shop.Appearance.FaceMatcaps.size() == 1
                               && Shop.Appearance.FaceMatcaps.count(Second) == 1 && Shop.Appearance.BodyMatcap == DraftingStudioMatcap,
              "remove on 1 face → 1 override left; remove on the body → Studio; a third remove refused (" + Nothing.Refusal + ")");

        // ⑨ user picture: added by name (layer 15), refused when missing / named twice; saved → loaded → saved byte-equal
        const DraftingCommandOutcome Added     = AddDraftingMatcapPicture(Shop, "ShopStripes", Bitmap);
        const DraftingCommandOutcome Twice     = AddDraftingMatcapPicture(Shop, "ShopStripes", Bitmap);
        const DraftingCommandOutcome BuiltName = AddDraftingMatcapPicture(Shop, "Chrome", Bitmap);
        const DraftingCommandOutcome Absent    = AddDraftingMatcapPicture(Shop, "Absent", Bitmap + ".missing");
        ClearDraftingSelection(Shop);
        const DraftingCommandOutcome Striped   = AssignDraftingMatcap(Shop, "ShopStripes");
        const DraftingMatcapLibrary  Custom    = AssembleDraftingMatcaps(Shop.Appearance);
        Tally(MatcapGate, 0.0, Added.AcceptedCondition && !Twice.AcceptedCondition && !BuiltName.AcceptedCondition && !Absent.AcceptedCondition && Striped.AcceptedCondition
                               && Custom.Names.size() == 15 && DraftingMatcapLayer(Custom, "ShopStripes") == 14 && Custom.Refusals.back().empty() && Custom.Pictures.back().Pixels == Fitted.Pixels,
              "picture ShopStripes → layer 14 of 15, fitted pixels equal; twice / built-in name / missing file refused (" + Absent.Refusal + ")");
        const std::string Written = EncodeDraftingDocument(Shop);
        const DraftingDocumentFile Read = DecodeDraftingDocument(Written);
        DraftingDocument Copy = CreateDraftingDocument(Read.Title);
        Copy.Tree = Read.Tree; Copy.Captions = Read.Captions; Copy.Appearance = Read.Appearance;
        RegenerateDraftingDocument(Copy, true);
        DraftingDocument Bare = CreateDraftingDocument("Bare");
        Expect(ConstructDraftingPrimitive(Bare, Stock));
        Tally(MatcapGate, 0.0, Read.Refusal.empty() && EncodeDraftingDocument(Copy) == Written && Written.find("[Appearance]") != std::string::npos
                               && Copy.Appearance.FaceMatcaps == Shop.Appearance.FaceMatcaps && EncodeDraftingDocument(Bare).find("Appearance") == std::string::npos,
              "save → load → save byte-equal with [Appearance]; documents without matcaps write no Appearance key");
        std::string Corrupt = Written;
        const size_t Brass = Corrupt.find("Matcap = 'Brass'");
        if (Brass != std::string::npos) Corrupt.replace(Brass, 16, "Matcap = 'Unobtainium'");
        const DraftingDocumentFile Rejected = DecodeDraftingDocument(Corrupt);
        Tally(MatcapGate, 0.0, Brass != std::string::npos && !Rejected.Refusal.empty(), "a file naming an unknown matcap is refused: " + Rejected.Refusal);
        std::filesystem::remove(Bitmap);
    }

    // ---- W16 multi-body: Plasticity R (new body) tiles, per-body matcaps, Q Boolean, targeted commands, refusals, codec.
    {
        auto Relative = [](double Value, double Expected) { return std::abs(Value - Expected) / std::max(1.0e-12, std::abs(Expected)); };
        auto Figures  = [](const DraftingDocument& Target, const std::string& Key) -> const DraftingBodyFigures*
        {
            for (const DraftingBodyFigures& Body : Target.Summary.Bodies) if (Body.Key == Key) return &Body;
            return nullptr;
        };
        auto VolumeOf = [&](const DraftingDocument& Target, const std::string& Key) { const DraftingBodyFigures* Body = Figures(Target, Key); return Body ? Body->Volume : -1.0; };

        // ① three bodies: Block1 4 × 2 × 1, Cylinder1 r 0.5 h 2 through it (New body), Block2 unit cube at x = 6 (New body)
        DraftingDocument Parts = CreateDraftingDocument("Parts");
        DraftingConstructRequest Request;
        Request.Kind = DraftingConstructKind::Block; Request.Width = 4.0; Request.Depth = 2.0; Request.Height = 1.0;
        Expect(ConstructDraftingPrimitive(Parts, Request));
        Request = {};
        Request.Kind = DraftingConstructKind::Cylinder; Request.Position = { 1.0, 1.0, -0.5 }; Request.Radius = 0.5; Request.Height = 2.0;
        Request.Combination = FeatureCombination::Separate;
        Expect(ConstructDraftingPrimitive(Parts, Request));
        Request = {};
        Request.Kind = DraftingConstructKind::Block; Request.Position = { 6.0, 0.0, 0.0 }; Request.Width = 1.0; Request.Depth = 1.0; Request.Height = 1.0;
        Request.Combination = FeatureCombination::Separate;
        Expect(ConstructDraftingPrimitive(Parts, Request));
        const bool Three = Parts.Summary.Bodies.size() == 3;
        const std::string BlockKey    = Three ? Parts.Summary.Bodies[0].Key : std::string{};
        const std::string CylinderKey = Three ? Parts.Summary.Bodies[1].Key : std::string{};
        const std::string CubeKey     = Three ? Parts.Summary.Bodies[2].Key : std::string{};
        const double CylinderVolume = Pi * 0.25 * 2.0;
        const double Worst = std::max({ Relative(VolumeOf(Parts, BlockKey), 8.0), Relative(VolumeOf(Parts, CylinderKey), CylinderVolume), Relative(VolumeOf(Parts, CubeKey), 1.0),
                                        Relative(Parts.Summary.Volume, 9.0 + CylinderVolume) });
        std::string Captions;
        for (const DraftingBodyFigures& Body : Parts.Summary.Bodies) Captions += (Captions.empty() ? "" : " · ") + Body.Caption + " (" + Body.Key + ")";
        Tally(BodiesGate, Worst, Three && Worst <= 1.0e-9 && Parts.Summary.SoundCondition && Parts.Regeneration.Active == CubeKey,
              "3 bodies " + Captions + ": 8 · π/2 · 1 exact, overlapping solids stay separate, all sound, active = the last created");

        // ② scene: one span per body, faces and edges tagged; picking the cylinder body marks exactly its faces
        DraftingScene PartsScene = BuildDraftingScene(Parts);
        bool Spans = PartsScene.Bodies.size() == 3;
        uint32_t FaceSum = 0;
        for (size_t Index = 0; Spans && Index < PartsScene.Bodies.size(); ++Index)
        {
            const DraftingSceneBody& Body = PartsScene.Bodies[Index];
            Spans = Spans && Body.Key == Parts.Summary.Bodies[Index].Key && int(Body.FaceCount) == Parts.Summary.Bodies[Index].Faces && Body.FirstFace == FaceSum;
            for (uint32_t Face = Body.FirstFace; Face < Body.FirstFace + Body.FaceCount; ++Face) Spans = Spans && PartsScene.Faces[Face].Body == Index;
            FaceSum += Body.FaceCount;
        }
        SelectDraftingName(Parts, DraftingSelectionKind::Body, CylinderKey, false);
        const DraftingSceneStates Picked = ComposeDraftingStates(PartsScene, Parts.Selection, 0u);
        const long Marked = std::count_if(Picked.Faces.begin(), Picked.Faces.end(), [](uint32_t Word) { return (Word & 1u) != 0; });
        Tally(BodiesGate, 0.0, Spans && FaceSum == PartsScene.Faces.size() && Three && Marked == Parts.Summary.Bodies[1].Faces,
              "scene: 3 body spans in creation order, face counts equal the audit, every face tagged; body pick marks its " + std::to_string(Marked) + " faces only");

        // ③ per-body matcaps: Block1 → Brushed steel, Cylinder1 → Brass, one Block1 face → Red plastic; Block2 keeps the default
        const uint64_t Revision = Parts.Revision;
        SelectDraftingName(Parts, DraftingSelectionKind::Body, BlockKey, false);
        const DraftingCommandOutcome Steel = AssignDraftingMatcap(Parts, "BrushedSteel");
        SelectDraftingName(Parts, DraftingSelectionKind::Body, CylinderKey, false);
        const DraftingCommandOutcome Brass = AssignDraftingMatcap(Parts, "Brass");
        const std::string RedFace = PartsScene.Faces[PartsScene.Bodies[0].FirstFace].Name;
        SelectDraftingName(Parts, DraftingSelectionKind::Face, RedFace, false);
        const DraftingCommandOutcome Red = AssignDraftingMatcap(Parts, "RedPlastic");
        ClearDraftingSelection(Parts);
        const DraftingMatcapLibrary Library = AssembleDraftingMatcaps(Parts.Appearance);
        auto LayerCounts = [&](const DraftingScene& Scene)
        {
            std::map<std::string, int> Counts;
            for (const uint32_t Word : ComposeDraftingMatcapLayers(Scene, Parts.Appearance, Library)) Counts[Library.Names[Word >> DraftingMatcapLayerShift]] += 1;
            return Counts;
        };
        std::map<std::string, int> Counts = LayerCounts(PartsScene);
        const int CylinderFaces = Parts.Summary.Bodies[1].Faces, CubeFaces = Parts.Summary.Bodies[2].Faces;
        Tally(BodiesGate, 0.0, Steel.AcceptedCondition && Brass.AcceptedCondition && Red.AcceptedCondition && Parts.Revision == Revision && Counts["BrushedSteel"] == 5
                               && Counts["RedPlastic"] == 1 && Counts["Brass"] == CylinderFaces && Counts[DraftingStudioMatcap] == CubeFaces && Parts.Appearance.BodyMatcaps.size() == 2
                               && DraftingFaceMatcap(Parts.Appearance, BlockKey, RedFace) == "RedPlastic" && DraftingBodyMatcap(Parts.Appearance, CubeKey) == DraftingStudioMatcap,
              Steel.Label + " · " + Brass.Label + " · " + Red.Label + ": 5 steel + 1 red + " + std::to_string(CylinderFaces) + " brass + " + std::to_string(CubeFaces)
              + " Studio faces; revision unchanged");

        // ④ Q Boolean: Block1 − Cylinder1. The tool is consumed (its Brass override is kept for undo / delete, so it dangles);
        //    the hole faces belong to Block1 and show its matcap.
        const DraftingCommandOutcome Drill = CombineDraftingBodies(Parts, BlockKey, { CylinderKey }, FeatureCombination::Difference);
        PartsScene = BuildDraftingScene(Parts);
        Counts = LayerCounts(PartsScene);
        const double Drilled = 8.0 - Pi * 0.25;
        const double DrillError = std::max(Relative(VolumeOf(Parts, BlockKey), Drilled), Relative(Parts.Summary.Volume, Drilled + 1.0));
        Tally(BodiesGate, DrillError, Drill.AcceptedCondition && DrillError <= 1.0e-9 && Parts.Summary.Bodies.size() == 2 && !Figures(Parts, CylinderKey) && Parts.Summary.SoundCondition
                                      && Parts.Tree.Steps.back().Kind == FeatureStepKind::Combine && Parts.Tree.Steps.back().Tool == CylinderKey && Parts.Regeneration.Active == BlockKey
                                      && CountDanglingMatcaps(PartsScene, Parts.Appearance) == 1 && Counts["Brass"] == 0 && Counts["BrushedSteel"] == Parts.Summary.Bodies[0].Faces - 1,
              Drill.Label + ": Block1 = 8 − π/4 exact, 2 bodies, the hole wears Block1's steel; the tool's Brass override dangles (1)");

        // ⑤ deleting the Combine step brings Cylinder1 back with its Brass; undo re-applies the Boolean
        const std::string CombineLabel = Parts.Tree.Steps.back().Label;
        const DraftingCommandOutcome Restored = DeleteDraftingStep(Parts, CombineLabel);
        PartsScene = BuildDraftingScene(Parts);
        Counts = LayerCounts(PartsScene);
        const bool Back = Restored.AcceptedCondition && Parts.Summary.Bodies.size() == 3 && Relative(VolumeOf(Parts, CylinderKey), CylinderVolume) <= 1.0e-9 && Counts["Brass"] == CylinderFaces && CountDanglingMatcaps(PartsScene, Parts.Appearance) == 0;
        UndoDraftingCommand(Parts);
        Tally(BodiesGate, Relative(VolumeOf(Parts, BlockKey), Drilled), Back && Parts.Summary.Bodies.size() == 2 && Relative(VolumeOf(Parts, BlockKey), Drilled) <= 1.0e-9,
              "delete " + CombineLabel + " → Cylinder1 returns (π/2, Brass, 0 dangling); undo → drilled again");

        // ⑥ targeted commands while Block1 is active: fillet one vertical Block2 edge (r 0.2, closed form), then a boss on
        //    Block2 (Target). Vertical edges join two sides, so their names carry no "Cap".
        std::string CubeEdge;
        for (const RegeneratedBody& Body : Parts.Regeneration.Bodies)
            if (Body.Key == CubeKey)
                for (const auto& Entry : Body.Naming.Edges)
                    if (CubeEdge.empty() && Entry.second.find("Cap") == std::string::npos) CubeEdge = Entry.second;
        SelectDraftingName(Parts, DraftingSelectionKind::Edge, CubeEdge, false);
        const DraftingCommandOutcome Rounded = BlendDraftingSelection(Parts, BlendProfile::Fillet, 0.2);
        const double RoundedCube = 1.0 - 0.04 * (1.0 - Pi / 4.0);
        Request = {};
        Request.Kind = DraftingConstructKind::Cylinder; Request.Position = { 6.5, 0.5, 1.0 }; Request.Radius = 0.25; Request.Height = 0.5;
        Request.Combination = FeatureCombination::Union; Request.Target = CubeKey;
        const DraftingCommandOutcome Bossed = ConstructDraftingPrimitive(Parts, Request);
        const double BossedCube = RoundedCube + Pi * 0.0625 * 0.5;
        const double TargetError = std::max({ Relative(VolumeOf(Parts, CubeKey), BossedCube), Relative(VolumeOf(Parts, BlockKey), Drilled) });
        const FeatureStep* Blend = nullptr;
        for (const FeatureStep& Step : Parts.Tree.Steps) if (Step.Kind == FeatureStepKind::Blend) Blend = &Step;
        Tally(BodiesGate, TargetError, Rounded.AcceptedCondition && Bossed.AcceptedCondition && TargetError <= 1.0e-9 && Blend && Blend->Target == CubeKey && Parts.Summary.SoundCondition,
              "fillet r 0.2 on " + CubeEdge + " (Target " + CubeKey + ") + Cylinder boss with Target = Block2: 1 − 0.04(1 − π/4) + π/32 exact; Block1 untouched");

        // ⑦ refusals leave tree, bodies and history untouched
        const size_t Steps = Parts.Tree.Steps.size(), Entries = Parts.UndoEntries.size();
        const double Volume = Parts.Summary.Volume;
        PartsScene = BuildDraftingScene(Parts);
        SelectDraftingName(Parts, DraftingSelectionKind::Edge, PartsScene.Edges[PartsScene.Bodies.front().FirstEdge].Name, false);
        SelectDraftingName(Parts, DraftingSelectionKind::Edge, PartsScene.Edges[PartsScene.Bodies.back().FirstEdge].Name, true);
        const DraftingCommandOutcome Spanning = BlendDraftingSelection(Parts, BlendProfile::Fillet, 0.1);
        ClearDraftingSelection(Parts);
        const DraftingCommandOutcome Self     = CombineDraftingBodies(Parts, BlockKey, { BlockKey }, FeatureCombination::Union);
        const DraftingCommandOutcome Lonely   = CombineDraftingBodies(Parts, BlockKey, {}, FeatureCombination::Union);
        const DraftingCommandOutcome Ghost    = CombineDraftingBodies(Parts, BlockKey, { "Extrude#99" }, FeatureCombination::Union);
        const DraftingCommandOutcome Separate = CombineDraftingBodies(Parts, BlockKey, { CubeKey }, FeatureCombination::Separate);
        Request = {};
        Request.Kind = DraftingConstructKind::Sphere; Request.Radius = 0.3; Request.Combination = FeatureCombination::New;
        const DraftingCommandOutcome Second = ConstructDraftingPrimitive(Parts, Request);
        const bool Refused = !Spanning.AcceptedCondition && !Self.AcceptedCondition && !Lonely.AcceptedCondition && !Ghost.AcceptedCondition && !Separate.AcceptedCondition && !Second.AcceptedCondition;
        Tally(BodiesGate, 0.0, Refused && Parts.Tree.Steps.size() == Steps && Parts.UndoEntries.size() == Entries && Parts.Summary.Volume == Volume,
              "refused: blend across bodies (" + Spanning.Refusal + ") · self-combine (" + Self.Refusal + ") · no tool · unknown body (" + Ghost.Refusal + ") · Separate as Q · second New; untouched");

        // ⑧ intersection keeps only the common part: Block2 ∩ a fresh New-body block overlapping its lower half
        Request = {};
        Request.Kind = DraftingConstructKind::Block; Request.Position = { 5.5, -0.5, -0.5 }; Request.Width = 2.0; Request.Depth = 2.0; Request.Height = 1.0;
        Request.Combination = FeatureCombination::Separate;
        Expect(ConstructDraftingPrimitive(Parts, Request));
        const std::string SlabKey = Parts.Summary.Bodies.back().Key;
        const DraftingCommandOutcome Common = CombineDraftingBodies(Parts, CubeKey, { SlabKey }, FeatureCombination::Intersection);
        const double CommonVolume = 0.5 - 0.04 * (1.0 - Pi / 4.0) * 0.5;                    // Lower half of the cube with half the vertical fillet
        const double CommonError  = Relative(VolumeOf(Parts, CubeKey), CommonVolume);
        Tally(BodiesGate, CommonError, Common.AcceptedCondition && CommonError <= 1.0e-9 && Parts.Summary.Bodies.size() == 2 && Parts.Summary.SoundCondition,
              Common.Label + ": Block2 = its lower half (V = " + std::to_string(VolumeOf(Parts, CubeKey)) + "), the slab consumed");
        UndoDraftingCommand(Parts);
        UndoDraftingCommand(Parts);

        // ⑨ save → load → save byte-equal (Target / Tool / Combine / [[Appearance.Bodies]]); same body volumes after reload
        const std::string Written = EncodeDraftingDocument(Parts);
        const DraftingDocumentFile Read = DecodeDraftingDocument(Written);
        DraftingDocument Copy = CreateDraftingDocument(Read.Title);
        Copy.Tree = Read.Tree; Copy.Captions = Read.Captions; Copy.Appearance = Read.Appearance;
        RegenerateDraftingDocument(Copy, true);
        bool SameBodies = Copy.Summary.Bodies.size() == Parts.Summary.Bodies.size();
        for (size_t Index = 0; SameBodies && Index < Copy.Summary.Bodies.size(); ++Index)
            SameBodies = Copy.Summary.Bodies[Index].Key == Parts.Summary.Bodies[Index].Key && Copy.Summary.Bodies[Index].Volume == Parts.Summary.Bodies[Index].Volume;
        Tally(BodiesGate, 0.0, Read.Refusal.empty() && EncodeDraftingDocument(Copy) == Written && SameBodies && Copy.Appearance.BodyMatcaps == Parts.Appearance.BodyMatcaps
                               && Written.find("Tool = '") != std::string::npos && Written.find("Target = '") != std::string::npos && Written.find("[[Appearance.Bodies]]") != std::string::npos,
              "save → load → save byte-equal with Combine Tool / Target and [[Appearance.Bodies]]; " + std::to_string(Copy.Summary.Bodies.size()) + " bodies, volumes bit-equal");
        std::string Future = Written;
        const size_t KindAt = Future.find("Kind = 10");
        if (KindAt != std::string::npos) Future.replace(KindAt, 9, "Kind = 25");
        const DraftingDocumentFile Newer = DecodeDraftingDocument(Future);
        Tally(BodiesGate, 0.0, KindAt != std::string::npos && !Newer.Refusal.empty(), "a future step kind (25, beyond Evolution 16) is refused: " + Newer.Refusal);
    }

    // ---- W17 trim / split: Plasticity C (command:cut) and X on picked bodies.
    {
        auto Relative = [](double Value, double Expected) { return std::abs(Value - Expected) / std::max(1.0e-12, std::abs(Expected)); };
        auto Figures  = [](const DraftingDocument& Target, const std::string& Key) -> const DraftingBodyFigures*
        {
            for (const DraftingBodyFigures& Body : Target.Summary.Bodies) if (Body.Key == Key) return &Body;
            return nullptr;
        };
        auto VolumeOf = [&](const DraftingDocument& Target, const std::string& Key) { const DraftingBodyFigures* Body = Figures(Target, Key); return Body ? Body->Volume : -1.0; };
        auto Draw = [](DraftingDocument& Target, DraftingDrawingKind Kind, std::vector<Vector2> Points, bool CentreCondition = false)
        {
            DraftingDrawingRequest Drawing;
            Drawing.Kind = Kind; Drawing.Points = std::move(Points); Drawing.CentreCondition = CentreCondition;
            const DraftingCommandOutcome Outcome = DrawDraftingSketch(Target, Drawing);
            return Outcome.AcceptedCondition ? Target.Tree.Steps.back().Label : std::string{};
        };

        // ① Block1 4 × 2 × 1 (Brass) cut by a drawn line x = 1 drawn upward: the inside piece (left, x < 1) is 2, the rest 6
        DraftingDocument Part = CreateDraftingDocument("Cut");
        DraftingConstructRequest Request;
        Request.Kind = DraftingConstructKind::Block; Request.Width = 4.0; Request.Depth = 2.0; Request.Height = 1.0;
        Expect(ConstructDraftingPrimitive(Part, Request));
        const std::string BlockKey = Part.Regeneration.Active;
        SelectDraftingName(Part, DraftingSelectionKind::Body, BlockKey, false);
        Expect(AssignDraftingMatcap(Part, "Brass"));
        const std::string LineSketch = Draw(Part, DraftingDrawingKind::Line, { { 1.0, -1.0 }, { 1.0, 3.0 } });
        DraftingCutRequest Cut;
        Cut.Target = BlockKey; Cut.Cutter = LineSketch;
        const DraftingCommandOutcome Split = CutDraftingBody(Part, Cut);
        const std::string PieceKey = Split.AcceptedCondition ? Part.Tree.Steps.back().Label : std::string{};
        const double SplitError = std::max(Relative(VolumeOf(Part, PieceKey), 2.0), Relative(VolumeOf(Part, BlockKey), 6.0));
        Tally(CutGate, SplitError, Split.AcceptedCondition && SplitError <= 1.0e-9 && Part.Summary.Bodies.size() == 2 && Part.Summary.SoundCondition && Part.Regeneration.Active == PieceKey
                                   && DraftingBodyCaption(Part, PieceKey) == "Cut1" && DraftingBodyMatcap(Part.Appearance, BlockKey) == "Brass" && DraftingBodyMatcap(Part.Appearance, PieceKey) == DraftingStudioMatcap,
              Split.Label + ": " + PieceKey + " (Cut1) = 2, Block1 = 6 exact, both sound; Block1 keeps its key and Brass, the piece wears Studio");

        // ② X on the picked piece removes it; undo brings it back, redo removes it again
        const DraftingCommandOutcome Removed = RemoveDraftingBodies(Part, { PieceKey });
        const bool Gone = Removed.AcceptedCondition && Part.Summary.Bodies.size() == 1 && !Figures(Part, PieceKey) && Relative(VolumeOf(Part, BlockKey), 6.0) <= 1.0e-9
                       && Part.Tree.Steps.back().Kind == FeatureStepKind::Remove && Part.Regeneration.Active == BlockKey;
        UndoDraftingCommand(Part);
        const bool Undone = Part.Summary.Bodies.size() == 2 && Relative(VolumeOf(Part, PieceKey), 2.0) <= 1.0e-9;
        RedoDraftingCommand(Part);
        Tally(CutGate, 0.0, Gone && Undone && Part.Summary.Bodies.size() == 1, Removed.Label + ": 1 body left (6), Block1 active; undo → 2 bodies, redo → 1");

        // ③ a drawn circle r ½ at (2, 1) cuts a cylinder out of what is left: π/4 inside, 6 − π/4 outside
        const std::string CircleSketch = Draw(Part, DraftingDrawingKind::Circle, { { 2.0, 1.0 }, { 2.5, 1.0 } }, true);
        Cut = {};
        Cut.Cutter = CircleSketch;                                                           // Target empty → the active body
        const DraftingCommandOutcome Core = CutDraftingBody(Part, Cut);
        const std::string CoreKey = Core.AcceptedCondition ? Part.Tree.Steps.back().Label : std::string{};
        const double CoreError = std::max(Relative(VolumeOf(Part, CoreKey), Pi / 4.0), Relative(VolumeOf(Part, BlockKey), 6.0 - Pi / 4.0));
        Tally(CutGate, CoreError, Core.AcceptedCondition && CoreError <= 1.0e-9 && Part.Summary.Bodies.size() == 2 && Part.Summary.SoundCondition && Figures(Part, BlockKey) && Figures(Part, BlockKey)->Genus == 1,
              Core.Label + ": core π/4 and the holed block 6 − π/4 exact (genus 1), cut from the active body");

        // ④ vehicle flow: 3 parabolic sections lofted into a sheet, a wavy character line seen from the Top view (−z)
        //    splits it into two sound sheets, X removes one (the BMW workflow: panel → C → X)
        DraftingDocument Bonnet = CreateDraftingDocument("Bonnet");
        VehicleCurveFile Curves;
        for (int Section = 0; Section < 3; ++Section)
        {
            VehicleCurve Curve;
            Curve.Name = "Section" + std::to_string(Section + 1);
            for (int Index = 0; Index <= 10; ++Index)
            {
                const double X = -1.0 + 0.2 * Index, Y = 0.5 * Section;
                Curve.Points.push_back({ X, Y, 0.25 * (1.0 - X * X) + 0.08 * Y * Y });
            }
            Curves.Curves.push_back(Curve);
        }
        VehicleCurve Character;
        Character.Name = "CharacterLine";
        for (int Index = 0; Index <= 12; ++Index)
        {
            const double X = -1.6 + 3.2 * Index / 12.0;
            Character.Points.push_back({ X, 0.5 + 0.18 * std::sin(2.2 * X / 1.6), 1.0 });
        }
        Curves.Curves.push_back(Character);
        Expect(ImportDraftingCurves(Bonnet, Curves));
        std::vector<std::string> Labels;
        for (const FeatureStep& Step : Bonnet.Tree.Steps) if (Step.Kind == FeatureStepKind::SpaceCurve) Labels.push_back(Step.Label);
        const bool Imported = Labels.size() == 4;
        if (Imported) Expect(LoftDraftingCurves(Bonnet, { Labels[0], Labels[1], Labels[2] }));
        const std::string PanelKey = Bonnet.Regeneration.Active;
        Cut = {};
        Cut.Target = PanelKey; Cut.Cutter = Imported ? Labels[3] : std::string{}; Cut.Direction = { 0.0, 0.0, -1.0 };
        const DraftingCommandOutcome Trim = CutDraftingBody(Bonnet, Cut);
        const std::string TrimKey = Trim.AcceptedCondition ? Bonnet.Tree.Steps.back().Label : std::string{};
        const DraftingBodyFigures* Kept  = Figures(Bonnet, PanelKey);
        const DraftingBodyFigures* Piece = Figures(Bonnet, TrimKey);
        const bool Sheets = Trim.AcceptedCondition && Kept && Piece && Kept->SheetCondition && Piece->SheetCondition && Kept->SoundCondition && Piece->SoundCondition && Bonnet.Summary.Bodies.size() == 2;
        const DraftingCommandOutcome Discard = RemoveDraftingBodies(Bonnet, { TrimKey });
        Tally(CutGate, 0.0, Imported && Sheets && Discard.AcceptedCondition && Bonnet.Summary.Bodies.size() == 1 && Figures(Bonnet, PanelKey) && Figures(Bonnet, PanelKey)->SheetCondition,
              Trim.Label + " seen along −z: 2 sound sheets (" + std::to_string(Kept ? Kept->Faces : 0) + " + " + std::to_string(Piece ? Piece->Faces : 0) + " faces); " + Discard.Label + " keeps the trimmed panel");

        // ⑤ refusals leave tree, bodies and history untouched
        const size_t Steps = Part.Tree.Steps.size(), Entries = Part.UndoEntries.size();
        const double Volume = Part.Summary.Volume;
        Cut = {};
        const DraftingCommandOutcome NoCutter = CutDraftingBody(Part, Cut);
        const std::string Beside = Draw(Part, DraftingDrawingKind::Line, { { 9.0, -1.0 }, { 9.0, 3.0 } });
        const size_t StepsWithBeside = Part.Tree.Steps.size(), EntriesWithBeside = Part.UndoEntries.size();
        Cut.Target = BlockKey; Cut.Cutter = Beside;
        const DraftingCommandOutcome Misses = CutDraftingBody(Part, Cut);
        Cut.Target = "Extrude#99";
        const DraftingCommandOutcome Ghost = CutDraftingBody(Part, Cut);
        Cut.Target = BlockKey; Cut.Cutter = BlockKey;                                         // An extrude step is no cutter
        const DraftingCommandOutcome Wrong = CutDraftingBody(Part, Cut);
        const DraftingCommandOutcome Phantom = RemoveDraftingBodies(Part, { "Ghost#1" });
        const bool Refused = !NoCutter.AcceptedCondition && !Misses.AcceptedCondition && !Ghost.AcceptedCondition && !Wrong.AcceptedCondition && !Phantom.AcceptedCondition;
        Tally(CutGate, 0.0, Refused && Part.Tree.Steps.size() == StepsWithBeside && Part.UndoEntries.size() == EntriesWithBeside && Part.Summary.Volume == Volume && StepsWithBeside == Steps + 1 && EntriesWithBeside == Entries + 1,
              "refused: no cutter (" + NoCutter.Refusal + ") · misses (" + Misses.Refusal + ") · unknown body (" + Ghost.Refusal + ") · extrude as cutter · Remove unknown (" + Phantom.Refusal + "); untouched");
        Expect(DeleteDraftingStep(Part, Beside));

        // ⑥ save → load → save byte-equal with Cut (Profile) / Remove and a curve Cut (Sections + plane); volumes bit-equal
        bool RoundTrips = true;
        std::string Notes;
        for (DraftingDocument* Source : { &Part, &Bonnet })
        {
            const std::string Written = EncodeDraftingDocument(*Source);
            const DraftingDocumentFile Read = DecodeDraftingDocument(Written);
            DraftingDocument Copy = CreateDraftingDocument(Read.Title);
            Copy.Tree = Read.Tree; Copy.Captions = Read.Captions; Copy.Appearance = Read.Appearance;
            RegenerateDraftingDocument(Copy, true);
            bool Same = Read.Refusal.empty() && EncodeDraftingDocument(Copy) == Written && Copy.Summary.Bodies.size() == Source->Summary.Bodies.size();
            for (size_t Index = 0; Same && Index < Copy.Summary.Bodies.size(); ++Index)
                Same = Copy.Summary.Bodies[Index].Key == Source->Summary.Bodies[Index].Key && Copy.Summary.Bodies[Index].Volume == Source->Summary.Bodies[Index].Volume;
            RoundTrips = RoundTrips && Same && Written.find("Kind = 11") != std::string::npos && Written.find("Kind = 12") != std::string::npos;
            Notes += (Notes.empty() ? "" : " · ") + Source->Title + " " + std::to_string(Copy.Summary.Bodies.size()) + " bodies";
        }
        Tally(CutGate, 0.0, RoundTrips, "save → load → save byte-equal with Cut (11) and Remove (12): " + Notes + ", volumes bit-equal");
    }

    // ---- W18: curve trim (Plasticity T) on sketches
    {
        auto Format = [](double Value) { char Text[32]; std::snprintf(Text, sizeof(Text), "%.12g", Value); return std::string(Text); };
        auto Draw = [](DraftingDocument& Target, DraftingDrawingKind Kind, std::vector<Vector2> Points, bool ClosedCondition = false, bool CentreCondition = false)
        {
            DraftingDrawingRequest Drawing;
            Drawing.Kind = Kind; Drawing.Points = std::move(Points); Drawing.ClosedCondition = ClosedCondition; Drawing.CentreCondition = CentreCondition;
            const DraftingCommandOutcome Outcome = DrawDraftingSketch(Target, Drawing);
            return Outcome.AcceptedCondition ? Target.Tree.Steps.back().Label : std::string{};
        };
        auto CurvesOf = [](const DraftingDocument& Target, const std::string& Label)
        {
            const auto Solved = Target.Regeneration.Sketches.find(Label);
            return Solved != Target.Regeneration.Sketches.end() ? Solved->second.Curves.size() : size_t(0);
        };
        auto Meet = [](Vector2 First, Vector2 FirstEnd, Vector2 Second, Vector2 SecondEnd)
        {
            const Vector2 Along = FirstEnd - First, Across = SecondEnd - Second;
            const double  Share = Cross(Second - First, Across) / Cross(Along, Across);
            return First + Along * Share;
        };

        // ① pentagram A → C → E → B → D → A as one closed Line chain: the crossing chain cannot be extruded
        const std::vector<Vector2> Tips = { { 1.0, 0.0 }, { 1.6, 1.9 }, { 0.0, 0.7 }, { 2.0, 0.7 }, { 0.4, 1.9 } };
        DraftingDocument Star = CreateDraftingDocument("Star");
        const std::string StarSketch = Draw(Star, DraftingDrawingKind::Line, Tips, true);
        const DraftingCommandOutcome Early = ExtrudeDraftingSketch(Star, StarSketch, 1.0, FeatureCombination::New);
        std::vector<Vector2> Outline = Tips;                                                 // Independent star: 5 tips + 5 inner crossings
        for (int Chord = 0; Chord < 5; ++Chord)
        {
            const Vector2 From = Tips[Chord], To = Tips[(Chord + 1) % 5];
            const Vector2 Next = Tips[(Chord + 2) % 5], NextEnd = Tips[(Chord + 3) % 5];
            Outline.push_back(Meet(From, To, Next, NextEnd));
        }
        Vector2 Middle = {};
        for (const Vector2& Tip : Tips) Middle = Middle + Tip * 0.2;
        std::sort(Outline.begin(), Outline.end(), [&](Vector2 Left, Vector2 Right) { return std::atan2(Left.Y - Middle.Y, Left.X - Middle.X) < std::atan2(Right.Y - Middle.Y, Right.X - Middle.X); });
        double StarArea = 0.0;
        for (size_t Index = 0; Index < Outline.size(); ++Index) StarArea += 0.5 * Cross(Outline[Index], Outline[(Index + 1) % Outline.size()]);

        // ② preview of the first trim: removed ends on the two crossings of chord A → C, the document untouched
        const std::string BeforePreview = EncodeDraftingDocument(Star);
        const SketchTrimOutcome Preview = PreviewDraftingTrim(Star, { StarSketch, 0, (Tips[0] + Tips[1]) * 0.5 });
        Vector2 FirstCrossing  = Meet(Tips[0], Tips[1], Tips[3], Tips[4]);                   // A → C meets B → D and E → B,
        Vector2 SecondCrossing = Meet(Tips[0], Tips[1], Tips[2], Tips[3]);                   // ordered along A → C
        if (Dot(FirstCrossing - Tips[0], Tips[1] - Tips[0]) > Dot(SecondCrossing - Tips[0], Tips[1] - Tips[0])) std::swap(FirstCrossing, SecondCrossing);
        const double  PreviewError   = Preview.Removed.size() >= 2 ? std::max(Length(Preview.Removed.front() - FirstCrossing), Length(Preview.Removed.back() - SecondCrossing)) : 1.0;
        Tally(TrimGate, PreviewError, Preview.Refusal.empty() && PreviewError <= 1.0e-12 && EncodeDraftingDocument(Star) == BeforePreview && Preview.Pieces == 2,
              "preview T on Line1 of the pentagram: removed piece between its two crossings (off " + Format(PreviewError) + "), 2 pieces left; document unchanged");

        // ③ T on the five inner pieces: the star outline (10 lines) is one loop of the shoelace area; extrude 1 is exact
        bool Trimmed = true;
        for (int Chord = 0; Chord < 5; ++Chord)
            Trimmed = Trimmed && TrimDraftingSketch(Star, { StarSketch, Chord, (Tips[Chord] + Tips[(Chord + 1) % 5]) * 0.5 }).AcceptedCondition;
        const auto   Solved      = Star.Regeneration.Sketches.find(StarSketch);
        const SketchProfile Loop = Solved != Star.Regeneration.Sketches.end() ? ExtractSketchProfile(Solved->second) : SketchProfile{};
        const double AreaError   = std::abs(Loop.Area - StarArea);
        const DraftingCommandOutcome Raised = ExtrudeDraftingSketch(Star, StarSketch, 1.0, FeatureCombination::New);
        const double VolumeError = std::abs(Star.Summary.Volume - StarArea) / StarArea;
        Tally(TrimGate, std::max(AreaError, VolumeError), !Early.AcceptedCondition && Trimmed && CurvesOf(Star, StarSketch) == 10 && Loop.Refusal.empty() && Loop.Loops.size() == 1
              && AreaError <= 1.0e-12 && Raised.AcceptedCondition && VolumeError <= 1.0e-9 && Star.Summary.SoundCondition && Star.UndoEntries.size() == 7,
              "pentagram: extrude refused first (" + Early.Refusal.substr(0, 60) + "…); 5 × T → 10 lines, area " + Format(Loop.Area) + " vs shoelace " + Format(StarArea) + ", extrude 1 → V " + Format(Star.Summary.Volume) + ", sound");

        // ④ a trim that would open the extruded star is refused, document untouched
        const std::string BeforeBreak = EncodeDraftingDocument(Star);
        const size_t      Entries     = Star.UndoEntries.size();
        const DraftingCommandOutcome Break = TrimDraftingSketch(Star, { StarSketch, 0, Tips[0] + (Tips[1] - Tips[0]) * 0.1 });
        Tally(TrimGate, 0.0, !Break.AcceptedCondition && Break.Refusal.find("Extrude") != std::string::npos && EncodeDraftingDocument(Star) == BeforeBreak && Star.UndoEntries.size() == Entries,
              "T on a star tip after the extrude: refused (" + Break.Refusal + "); untouched");

        // ⑤ cross-sketch: circle r 0.3 at (2, 0.5) over the right side of a 2 × 1 rectangle (two sketches)
        DraftingDocument Plan = CreateDraftingDocument("Plan");
        const std::string Box    = Draw(Plan, DraftingDrawingKind::Rectangle, { { 0.0, 0.0 }, { 2.0, 1.0 } });
        const std::string Circle = Draw(Plan, DraftingDrawingKind::Circle, { { 2.0, 0.5 }, { 2.3, 0.5 } }, false, true);
        const DraftingCommandOutcome Outer = TrimDraftingSketch(Plan, { Circle, 0, { 2.3, 0.5 } });
        const auto   Arc      = Plan.Regeneration.Sketches.find(Circle);
        const bool   ArcCondition = Arc != Plan.Regeneration.Sketches.end() && Arc->second.Curves.size() == 1 && Arc->second.Curves[0].Kind == SketchCurveKind::Arc;
        double ArcError = 1.0;
        if (ArcCondition)
        {
            const SketchModel& Model = Arc->second;
            ArcError = std::max(Length(Model.Points[Model.Curves[0].Start].Position - Vector2{ 2.0, 0.8 }), Length(Model.Points[Model.Curves[0].End].Position - Vector2{ 2.0, 0.2 }));
        }
        UndoDraftingCommand(Plan);
        const bool Restored = Plan.Regeneration.Sketches.at(Circle).Curves[0].Kind == SketchCurveKind::Circle;
        RedoDraftingCommand(Plan);
        const DraftingCommandOutcome Side = TrimDraftingSketch(Plan, { Box, 1, { 2.0, 0.5 } });
        Tally(TrimGate, ArcError, Outer.AcceptedCondition && ArcCondition && ArcError <= 1.0e-12 && Restored && Plan.Regeneration.Sketches.at(Circle).Curves[0].Kind == SketchCurveKind::Arc
              && Side.AcceptedCondition && CurvesOf(Plan, Box) == 5,
              Outer.Label + ": the circle (another sketch than the rectangle) becomes the arc (2, 0.8) → (2, 0.2) inside it (off " + Format(ArcError) + "); undo → circle, redo → arc; " + Side.Label + " splits the right side (5 curves)");

        // ⑥ a lone unused line with nothing crossing it: T erases it and its sketch step goes; undo brings it back
        const std::string Lone  = Draw(Plan, DraftingDrawingKind::Line, { { 5.0, 5.0 }, { 6.0, 5.0 } });
        const size_t      Steps = Plan.Tree.Steps.size();
        const DraftingCommandOutcome Erase = TrimDraftingSketch(Plan, { Lone, 0, { 5.5, 5.0 } });
        const bool Gone = Erase.AcceptedCondition && Plan.Tree.Steps.size() == Steps - 1 && !Plan.Captions.contains(Lone);
        UndoDraftingCommand(Plan);
        const bool Back = Plan.Tree.Steps.size() == Steps && CurvesOf(Plan, Lone) == 1;
        Tally(TrimGate, 0.0, Gone && Back, Erase.Label + ": nothing crosses it → the curve and its emptied sketch step go; undo restores");

        // ⑦ save → load → save byte-equal with the trimmed sketches (new points, PointOnLine pins = constraint kind 14)
        bool RoundTrips = true;
        for (DraftingDocument* Source : { &Star, &Plan })
        {
            const std::string Written = EncodeDraftingDocument(*Source);
            const DraftingDocumentFile Read = DecodeDraftingDocument(Written);
            DraftingDocument Copy = CreateDraftingDocument(Read.Title);
            Copy.Tree = Read.Tree; Copy.Captions = Read.Captions; Copy.Appearance = Read.Appearance;
            RegenerateDraftingDocument(Copy, true);
            RoundTrips = RoundTrips && Read.Refusal.empty() && EncodeDraftingDocument(Copy) == Written && Copy.Summary.Volume == Source->Summary.Volume;
        }
        RoundTrips = RoundTrips && EncodeDraftingDocument(Star).find("Kind = 14") != std::string::npos;
        Tally(TrimGate, 0.0, RoundTrips, "save → load → save byte-equal for the star (pins kept) and the plan; volumes bit-equal");
    }

    GateTally ConstrainGate   = { "W23", "Sketch constraints (K): a drawn rectangle goes DOF 4 → Lock 2 → D 1 → D 0, each dimension drives the extrude volume; literal and parameter-bound dimension edits regenerate downstream; undo / redo; remove frees a degree", "≤ 10⁻⁹ · exact" };
    GateTally ConflictGate    = { "W24", "Constraint refusals: redundant (named partner) and conflicting (named partners) constraints, non-dimension / non-positive edits, unknown sketch — the document, history and file stay untouched", "exact" };
    GateTally DragGate        = { "W25", "Point drag: a free corner follows the cursor and the extrude follows it; a locked point refuses; the preview is pure; save → load → save byte-equal with constraint labels", "≤ 10⁻⁹ · exact" };
    GateTally ReferenceGate   = { "W26", "Reference dimensions (Shift+D / E): a driven diagonal reads √13 then 5 without moving anything (DOF, positions, volume bit-equal); editing it or making it driving while implied is refused; E makes it driving at its reading and back; undo / redo; codec", "≤ 10⁻¹² · exact" };
    GateTally SplineGate      = { "W27", "Spline constraints: a line and a NURBS curve joined → L (G1) → C (G2, replaces the G1) → H on the far end, each solve ≤ 10⁻¹² with DOF −1 / −1 / −1; undo restores the G1; G2 on a natural cubic refused; codec", "≤ 10⁻¹² · exact" };
    GateTally LinkGate        = { "W28", "Cross-sketch links: a later sketch's line linked to an earlier plate's corner and top line (one undo entry, construction curve on linked points) follows the plate's width edit; backward links, linked drags, bad indices refused; codec; unlink frees", "≤ 10⁻¹² · exact" };
    // ---- W19 … W22: the editing keys (G / R / S, Shift+D, Ctrl+C / Ctrl+V, H family, J / O / P)
    {
        auto Relative = [](double Value, double Expected) { return std::abs(Value - Expected) / std::max(1.0e-12, std::abs(Expected)); };
        auto VolumeOf = [](const DraftingDocument& Target, const std::string& Key)
        {
            for (const DraftingBodyFigures& Body : Target.Summary.Bodies) if (Body.Key == Key) return Body.Volume;
            return -1.0;
        };
        auto RoundTrip = [](const DraftingDocument& Source)
        {
            const std::string Written = EncodeDraftingDocument(Source);
            const DraftingDocumentFile Read = DecodeDraftingDocument(Written);
            DraftingDocument Copy = CreateDraftingDocument(Read.Title);
            Copy.Tree = Read.Tree; Copy.Captions = Read.Captions; Copy.Appearance = Read.Appearance;
            RegenerateDraftingDocument(Copy, true);
            return Read.Refusal.empty() && EncodeDraftingDocument(Copy) == Written && Copy.Summary.Volume == Source.Summary.Volume && Copy.Appearance.HiddenBodies == Source.Appearance.HiddenBodies;
        };
        auto Draw = [](DraftingDocument& Target, DraftingDrawingKind Kind, std::vector<Vector2> Points)
        {
            DraftingDrawingRequest Drawing;
            Drawing.Kind = Kind; Drawing.Points = std::move(Points);
            const DraftingCommandOutcome Outcome = DrawDraftingSketch(Target, Drawing);
            return Outcome.AcceptedCondition ? Target.Tree.Steps.back().Label : std::string{};
        };
        auto Hits = [](const DraftingDocument& Target, std::array<double, 3> At)
        {
            const DraftingScene Scene = BuildDraftingScene(Target);
            const std::optional<DraftingPick> Hit = PickDraftingScene(Scene, RayBetween({ At[0], At[1], 20.0 }, At), 0.02);
            if (!Hit) return std::string{};
            const uint32_t Index = (Hit->Identity & ~IdentityEdgeBit) - 1u;
            return Scene.Bodies[(Hit->Identity & IdentityEdgeBit) ? Scene.Edges[Index].Body : Scene.Faces[Index].Body].Key;
        };

        // W19 ① G: Block1 2 × 2 × 1 moved by (10, 0, 0): volume 4, the old spot is empty and the new one picks Block1
        DraftingDocument Part = CreateDraftingDocument("Editing");
        DraftingConstructRequest Request;
        Request.Kind = DraftingConstructKind::Block; Request.Width = 2.0; Request.Depth = 2.0; Request.Height = 1.0;
        Expect(ConstructDraftingPrimitive(Part, Request));
        const std::string BlockKey = Part.Regeneration.Active;
        const std::string TopFace  = PickAndSelect(Part, RayBetween({ 1.0, 1.0, 20.0 }, { 1.0, 1.0, 0.0 }), false);
        const DraftingCommandOutcome Moved = TransformDraftingBodies(Part, { BlockKey }, TranslationMap({ 10.0, 0.0, 0.0 }), "Move");
        const bool Followed = Hits(Part, { 1.0, 1.0, 0.0 }).empty() && Hits(Part, { 11.0, 1.0, 0.0 }) == BlockKey;
        const bool Kept = Part.Selection.Faces.size() == 1 && Part.Selection.Faces.front() == TopFace;
        Tally(MoveGate, Relative(Part.Summary.Volume, 4.0), Moved.AcceptedCondition && Relative(Part.Summary.Volume, 4.0) <= 1.0e-9 && Part.Summary.SoundCondition && Followed && Kept
                                                            && Part.Tree.Steps.back().Kind == FeatureStepKind::Transform && DraftingBodyCaption(Part, Part.Tree.Steps.back().Label) == "Move1",
              Moved.Label + ": V 4 kept, sound; (1, 1) now empty, (11, 1) picks Block1; the picked top face " + TopFace + " stays selected");

        // W19 ② R 90° about z through (11, 1, 0), then S ×2 about the same point: V 4 → 32; undo → 4
        const DraftingCommandOutcome Turned = TransformDraftingBodies(Part, { BlockKey }, RotationMap({ 11.0, 1.0, 0.0 }, { 0.0, 0.0, 1.0 }, Pi / 2.0), "Rotate");
        const DraftingCommandOutcome Grown  = TransformDraftingBodies(Part, { BlockKey }, ScalingMap({ 11.0, 1.0, 0.0 }, { 2.0, 2.0, 2.0 }), "Scale");
        const double GrownVolume = Part.Summary.Volume;
        UndoDraftingCommand(Part);
        const double UndoneVolume = Part.Summary.Volume;
        RedoDraftingCommand(Part);
        Tally(MoveGate, std::max(Relative(GrownVolume, 32.0), Relative(UndoneVolume, 4.0)), Turned.AcceptedCondition && Grown.AcceptedCondition && Relative(GrownVolume, 32.0) <= 1.0e-9 && Relative(UndoneVolume, 4.0) <= 1.0e-9
                                                                                         && Part.Summary.SoundCondition && Hits(Part, { 11.0, 1.0, 0.0 }) == BlockKey,
              Turned.Label + " + " + Grown.Label + ": V 32 = 8 × 4, sound; undo → 4, redo → 32");

        // W19 ③ refusals leave the document alone
        const std::string Before = EncodeDraftingDocument(Part);
        const DraftingCommandOutcome Empty = TransformDraftingBodies(Part, {}, TranslationMap({ 1.0, 0.0, 0.0 }), "Move");
        const DraftingCommandOutcome Ghost = TransformDraftingBodies(Part, { "Ghost#9" }, TranslationMap({ 1.0, 0.0, 0.0 }), "Move");
        AffineMap Flat;
        Flat.ColumnZ = {};
        const DraftingCommandOutcome Singular = TransformDraftingBodies(Part, { BlockKey }, Flat, "Scale");
        Tally(MoveGate, 0.0, !Empty.AcceptedCondition && !Ghost.AcceptedCondition && !Singular.AcceptedCondition && Singular.Refusal.find("singular") != std::string::npos && EncodeDraftingDocument(Part) == Before,
              "refused: nothing picked · " + Ghost.Refusal + " · " + Singular.Refusal + " — document unchanged");

        // W20 ① Shift+D: a copy moved by (0, 6, 0) is a new body, active and selected, captioned Block1Copy1
        const DraftingCommandOutcome Copied = DuplicateDraftingBodies(Part, { BlockKey }, TranslationMap({ 0.0, 6.0, 0.0 }));
        const std::string CopyKey = Copied.AcceptedCondition ? Part.Tree.Steps.back().Label : std::string{};
        Tally(CopyGate, Relative(VolumeOf(Part, CopyKey), 32.0), Copied.AcceptedCondition && Part.Summary.Bodies.size() == 2 && Relative(VolumeOf(Part, CopyKey), 32.0) <= 1.0e-9 && Relative(VolumeOf(Part, BlockKey), 32.0) <= 1.0e-9
                                                                 && Part.Regeneration.Active == CopyKey && Part.Selection.Bodies == std::vector<std::string>{ CopyKey } && DraftingBodyCaption(Part, CopyKey) == "Block1Copy1"
                                                                 && Hits(Part, { 11.0, 7.0, 0.0 }) == CopyKey,
              Copied.Label + ": 2 bodies of 32; " + CopyKey + " (Block1Copy1) active, selected and picked at (11, 7)");

        // W20 ② Ctrl+C on Block1 (clipboard = key + the last step), X on Block1, Ctrl+V: the pasted copy is Block1 as copied
        const std::string Clipboard = Part.Tree.Steps.back().Label;
        Expect(RemoveDraftingBodies(Part, { BlockKey }));
        const DraftingCommandOutcome Pasted = DuplicateDraftingBodies(Part, { BlockKey }, TranslationMap({}), Clipboard);
        const std::string PasteKey = Pasted.AcceptedCondition ? Part.Tree.Steps.back().Label : std::string{};
        Tally(CopyGate, Relative(VolumeOf(Part, PasteKey), 32.0), Pasted.AcceptedCondition && Part.Summary.Bodies.size() == 2 && Relative(VolumeOf(Part, PasteKey), 32.0) <= 1.0e-9 && Hits(Part, { 11.0, 1.0, 0.0 }) == PasteKey,
              Pasted.Label + ": after X on Block1 the paste brings it back as " + PasteKey + " (V 32 at the copied place)");

        // W20 ③ refusals
        const DraftingCommandOutcome Stale = DuplicateDraftingBodies(Part, { BlockKey }, TranslationMap({}), "Nowhere#99");
        const DraftingCommandOutcome None  = DuplicateDraftingBodies(Part, {}, TranslationMap({}));
        Tally(CopyGate, 0.0, !Stale.AcceptedCondition && !None.AcceptedCondition && Stale.Refusal.find("gone") != std::string::npos, "refused: " + Stale.Refusal + " · " + None.Refusal);

        // W21 H family on three bodies: Block1Copy1 (copy), the pasted body, and a new cylinder
        DraftingConstructRequest Post;
        Post.Kind = DraftingConstructKind::Cylinder; Post.Position = { -5.0, 0.0, 0.0 }; Post.Radius = 0.5; Post.Height = 1.0; Post.Combination = FeatureCombination::Separate;
        Expect(ConstructDraftingPrimitive(Part, Post));
        const std::string PostKey = Part.Regeneration.Active;
        auto Shown = [](const DraftingDocument& Target) { std::set<std::string> Keys; for (const DraftingSceneBody& Body : BuildDraftingScene(Target).Bodies) Keys.insert(Body.Key); return Keys; };
        ClearDraftingSelection(Part);
        const DraftingCommandOutcome NoPick = ChangeDraftingVisibility(Part, DraftingVisibilityVerb::HideSelected);
        SelectDraftingName(Part, DraftingSelectionKind::Body, CopyKey, false);
        const DraftingCommandOutcome Hide = ChangeDraftingVisibility(Part, DraftingVisibilityVerb::HideSelected);
        const bool HideOk = Hide.AcceptedCondition && Shown(Part) == std::set<std::string>{ PasteKey, PostKey } && Hits(Part, { 11.0, 7.0, 0.0 }).empty() && Part.Selection.Bodies.empty();
        SelectDraftingName(Part, DraftingSelectionKind::Body, PostKey, false);
        const DraftingCommandOutcome Isolate = ChangeDraftingVisibility(Part, DraftingVisibilityVerb::HideUnselected);
        const bool IsolateOk = Isolate.AcceptedCondition && Shown(Part) == std::set<std::string>{ PostKey };
        const DraftingCommandOutcome Invert = ChangeDraftingVisibility(Part, DraftingVisibilityVerb::InvertHidden);
        const bool InvertOk = Invert.AcceptedCondition && Shown(Part) == std::set<std::string>{ CopyKey, PasteKey } && Part.Selection.Bodies.empty();
        const bool Saved = RoundTrip(Part);
        const DraftingCommandOutcome Reveal = ChangeDraftingVisibility(Part, DraftingVisibilityVerb::UnhideAll);
        const bool RevealOk = Reveal.AcceptedCondition && Shown(Part).size() == 3;
        UndoDraftingCommand(Part);
        const bool UndoOk = Shown(Part) == std::set<std::string>{ CopyKey, PasteKey } && Part.Summary.Bodies.size() == 3;
        RedoDraftingCommand(Part);
        const DraftingCommandOutcome Again = ChangeDraftingVisibility(Part, DraftingVisibilityVerb::UnhideAll);
        Tally(HideGate, 0.0, HideOk && IsolateOk && InvertOk && RevealOk && UndoOk && Saved && !NoPick.AcceptedCondition && !Again.AcceptedCondition,
              "H hides Block1Copy1 (scene 2, unpickable, deselected) · Shift+H on the cylinder leaves it alone · Ctrl+H swaps → 2 shown · saved with Hidden, reload equal · Alt+H shows 3; undo → 2 · refused: " + NoPick.Refusal + " · " + Again.Refusal);
        Tally(HideGate, 0.0, Part.Summary.Bodies.size() == 3 && Relative(Part.Summary.Volume, 64.0 + Pi * 0.25) <= 1.0e-9, "hiding never touches the model: 3 bodies, V 64 + π/4 throughout");
        Tally(MoveGate, 0.0, RoundTrip(Part) && EncodeDraftingDocument(Part).find("Kind = 13") != std::string::npos && EncodeDraftingDocument(Part).find("Kind = 14") != std::string::npos,
              "save → load → save byte-equal with Transform (13) and Duplicate (14) steps (placements and the paste source kept)");

        // W22 ① J: an L and its complement drawn as two sketches; the L alone cannot extrude, joined they make a unit square
        DraftingDocument Plan = CreateDraftingDocument("Curves");
        const std::string Lower = Draw(Plan, DraftingDrawingKind::Line, { { 0.0, 0.0 }, { 1.0, 0.0 }, { 1.0, 1.0 } });
        const std::string Upper = Draw(Plan, DraftingDrawingKind::Line, { { 1.0, 1.0 }, { 0.0, 1.0 }, { 0.0, 0.0 } });
        const DraftingCommandOutcome Open   = ExtrudeDraftingSketch(Plan, Lower, 1.0, FeatureCombination::Separate);
        const size_t                 Steps  = Plan.Tree.Steps.size();
        const DraftingCommandOutcome Joined = JoinDraftingSketches(Plan, { Lower, Upper });
        const bool Merged = Joined.AcceptedCondition && Plan.Tree.Steps.size() == Steps - 1 && !Plan.Captions.contains(Upper);
        const DraftingCommandOutcome Square = ExtrudeDraftingSketch(Plan, Lower, 1.0, FeatureCombination::Separate);
        const std::string SquareKey = Square.AcceptedCondition ? Plan.Tree.Steps.back().Label : std::string{};
        Tally(CurveGate, Relative(VolumeOf(Plan, SquareKey), 1.0), !Open.AcceptedCondition && Merged && Square.AcceptedCondition && Relative(VolumeOf(Plan, SquareKey), 1.0) <= 1.0e-9,
              Joined.Label + ": the open L refuses to extrude; joined (one sketch fewer) the square extrudes to V 1");

        // W22 ② O: the joined square offset by ¼ outward → 1.5 × 1.5 (mitred corners), extruded 1 → 2.25; preview is pure
        const std::string BeforePreview = EncodeDraftingDocument(Plan);
        const SketchOffsetOutcome Preview = PreviewDraftingOffset(Plan, Lower, 0.25);
        const bool Pure = EncodeDraftingDocument(Plan) == BeforePreview && Preview.Refusal.empty() && Preview.Chains == 1;
        const DraftingCommandOutcome Offset = OffsetDraftingSketch(Plan, Lower, 0.25);
        const std::string OffsetSketch = Offset.AcceptedCondition ? Plan.Tree.Steps.back().Label : std::string{};
        const DraftingCommandOutcome Grow = ExtrudeDraftingSketch(Plan, OffsetSketch, 1.0, FeatureCombination::Separate);
        const std::string GrowKey = Grow.AcceptedCondition ? Plan.Tree.Steps.back().Label : std::string{};
        Tally(CurveGate, Relative(VolumeOf(Plan, GrowKey), 2.25), Pure && Offset.AcceptedCondition && Grow.AcceptedCondition && Relative(VolumeOf(Plan, GrowKey), 2.25) <= 1.0e-9 && DraftingBodyCaption(Plan, OffsetSketch) == "Offset1",
              Offset.Label + ": new sketch Offset1, extruded V 2.25 = 1.5²; the preview left the document alone");

        // W22 ③ P: a drawn line (0, 3) → (2, 3) piped with r ¼ → new body π r² L
        const std::string Rail = Draw(Plan, DraftingDrawingKind::Line, { { 0.0, 3.0 }, { 2.0, 3.0 } });
        const DraftingCommandOutcome Piped = PipeDraftingPath(Plan, Rail, 0.25);
        const std::string PipeKey = Piped.AcceptedCondition ? Plan.Tree.Steps.back().Label : std::string{};
        Tally(CurveGate, Relative(VolumeOf(Plan, PipeKey), Pi * 0.0625 * 2.0), Piped.AcceptedCondition && Relative(VolumeOf(Plan, PipeKey), Pi * 0.0625 * 2.0) <= 1.0e-9 && Plan.Summary.SoundCondition && Plan.Regeneration.Active == PipeKey,
              Piped.Label + ": tube V π r² L = π/8 exact, sound, active");

        // W22 ④ refusals and codec
        const DraftingCommandOutcome Lone     = JoinDraftingSketches(Plan, { Lower });
        const DraftingCommandOutcome Consumed = JoinDraftingSketches(Plan, { Rail, Lower });
        const DraftingCommandOutcome Vanish   = OffsetDraftingSketch(Plan, Lower, -0.6);
        const DraftingCommandOutcome Fat      = PipeDraftingPath(Plan, Rail, 0.0);
        const DraftingCommandOutcome NoPath   = PipeDraftingPath(Plan, PipeKey, 0.1);
        Tally(CurveGate, 0.0, !Lone.AcceptedCondition && !Consumed.AcceptedCondition && !Vanish.AcceptedCondition && !Fat.AcceptedCondition && !NoPath.AcceptedCondition,
              "refused: " + Lone.Refusal + " · " + Consumed.Refusal + " · " + Vanish.Refusal + " · " + Fat.Refusal + " · " + NoPath.Refusal);
        Tally(CurveGate, 0.0, RoundTrip(Plan) && EncodeDraftingDocument(Plan).find("Kind = 15") != std::string::npos, "save → load → save byte-equal with the joined / offset sketches and the Pipe (15) step");
    }

    // ---- W23 … W25: sketch constraints (K)
    {
        auto Relative = [](double Value, double Expected) { return std::abs(Value - Expected) / std::max(1.0e-12, std::abs(Expected)); };
        auto Propose = [](const DraftingDocument& Target, const std::string& Sketch, std::vector<SketchPick> Picks, SketchConstraintVerb Verb, double Value = std::numeric_limits<double>::quiet_NaN())
        {
            const DraftingSketchDiagnosis Diagnosis = DiagnoseDraftingSketch(Target, Sketch);
            SketchConstraintProposal Proposal = ProposeSketchConstraint(Diagnosis.Sketch, Picks, Verb);
            if (!std::isnan(Value)) Proposal.Constraint.Value = Value;
            return Proposal.Constraint;
        };
        auto Freedom = [](const DraftingDocument& Target, const std::string& Sketch) { return DiagnoseDraftingSketch(Target, Sketch).Solve.DegreesOfFreedom; };
        const SketchPick Bottom = { SketchPickKind::Curve, 0 };
        const SketchPick Side   = { SketchPickKind::Curve, 1 };
        const SketchPick Top    = { SketchPickKind::Curve, 2 };

        // W23 ① a drawn 2 × 1 rectangle extruded by 1: DOF 4; ] locks its first corner (2), D on the bottom = 3 (1), D on the side = 2 (0)
        DraftingDocument Plate = CreateDraftingDocument("Constraints");
        DraftingDrawingRequest Drawing;
        Drawing.Kind = DraftingDrawingKind::Rectangle; Drawing.Points = { { 0.0, 0.0 }, { 2.0, 1.0 } };
        const DraftingCommandOutcome Drawn = DrawDraftingSketch(Plate, Drawing);
        const std::string Sketch = Drawn.AcceptedCondition ? Plate.Tree.Steps.back().Label : std::string{};
        const DraftingCommandOutcome Raised = ExtrudeDraftingSketch(Plate, Sketch, 1.0, FeatureCombination::Separate);
        const std::string Key = Raised.AcceptedCondition ? Plate.Tree.Steps.back().Label : std::string{};
        const int Free = Freedom(Plate, Sketch);
        const DraftingCommandOutcome Locked = ConstrainDraftingSketch(Plate, Sketch, Propose(Plate, Sketch, { { SketchPickKind::Point, 0 } }, SketchConstraintVerb::Lock));
        const int AfterLock = Freedom(Plate, Sketch);
        const DraftingCommandOutcome Wide = ConstrainDraftingSketch(Plate, Sketch, Propose(Plate, Sketch, { Bottom }, SketchConstraintVerb::Dimension, 3.0));
        const int AfterWide = Freedom(Plate, Sketch);
        const double WideVolume = Plate.Summary.Volume;
        const DraftingCommandOutcome Tall = ConstrainDraftingSketch(Plate, Sketch, Propose(Plate, Sketch, { Side }, SketchConstraintVerb::Dimension, 2.0));
        const int AfterTall = Freedom(Plate, Sketch);
        const std::vector<SketchConstraint>& Authored = Plate.Tree.Steps[0].Sketch.Constraints;
        const bool Labelled = Authored.size() == 7 && Authored[4].Label == "F1" && Authored[5].Label == "D1" && Authored[6].Label == "D2";
        Tally(ConstrainGate, Relative(Plate.Summary.Volume, 6.0) + Relative(WideVolume, 3.0),
              Free == 4 && Locked.AcceptedCondition && AfterLock == 2 && Wide.AcceptedCondition && AfterWide == 1 && Tall.AcceptedCondition && AfterTall == 0 && Labelled
                  && Relative(WideVolume, 3.0) <= 1.0e-9 && Relative(Plate.Summary.Volume, 6.0) <= 1.0e-9 && Plate.Regeneration.Active == Key,
              "DOF " + std::to_string(Free) + " → " + Locked.Label + " " + std::to_string(AfterLock) + " → " + Wide.Label + " " + std::to_string(AfterWide) + " (V 3) → " + Tall.Label + " " + std::to_string(AfterTall) + " (V 6); labels F1 D1 D2");

        // W23 ② D1 = 4 → V 8 (the solved corners are written back: the step's own points move); undo → 6, redo → 8
        const DraftingCommandOutcome Edit = ChangeDraftingDimension(Plate, Sketch, 5, 4.0);
        const double Edited = Plate.Summary.Volume;
        const Vector2 Corner = Plate.Tree.Steps[0].Sketch.Points[2].Position;
        const bool Undone = UndoDraftingCommand(Plate);
        const double Restored = Plate.Summary.Volume;
        const bool Redone = RedoDraftingCommand(Plate);
        Tally(ConstrainGate, Relative(Edited, 8.0) + Relative(Restored, 6.0) + std::abs(Corner.X - 4.0) + std::abs(Corner.Y - 2.0),
              Edit.AcceptedCondition && Relative(Edited, 8.0) <= 1.0e-9 && std::abs(Corner.X - 4.0) <= 1.0e-9 && std::abs(Corner.Y - 2.0) <= 1.0e-9 && Undone && Relative(Restored, 6.0) <= 1.0e-12 && Redone && Relative(Plate.Summary.Volume, 8.0) <= 1.0e-9,
              Edit.Label + ": V 8, far corner written back at (4, 2); undo V 6, redo V 8");

        // W23 ③ a Block tile is fully dimensioned by parameters: its Width dimension changes the parameter (and the volume)
        DraftingDocument Tile = CreateDraftingDocument("Tile");
        Expect(ConstructDraftingPrimitive(Tile, DraftingConstructRequest{}));
        const std::string TileSketch = Tile.Tree.Steps[0].Label;
        int WidthIndex = -1;
        for (size_t Index = 0; Index < Tile.Tree.Steps[0].Sketch.Constraints.size(); ++Index)
            if (Tile.Tree.Steps[0].Sketch.Constraints[Index].Parameter.ends_with("Width")) WidthIndex = static_cast<int>(Index);
        const int TileFree = Freedom(Tile, TileSketch);
        const DraftingCommandOutcome Widen = ChangeDraftingDimension(Tile, TileSketch, WidthIndex, 3.0);
        double WidthValue = 0.0;
        for (const FeatureParameter& Parameter : Tile.Tree.Parameters) if (Parameter.Name.ends_with("Width")) WidthValue = Parameter.Value;
        Tally(ConstrainGate, Relative(Tile.Summary.Volume, 4.5), TileFree == 0 && WidthIndex >= 0 && Widen.AcceptedCondition && WidthValue == 3.0 && Relative(Tile.Summary.Volume, 4.5) <= 1.0e-9,
              "Block tile DOF 0; " + Widen.Label + " sets its Width parameter to 3 → V 3 × 1.5 × 1 = 4.5");

        // W23 ④ removing D2 frees the height (DOF 1); the body stays (positions unchanged)
        const DraftingCommandOutcome Freed = RemoveDraftingConstraint(Plate, Sketch, 6);
        Tally(ConstrainGate, Relative(Plate.Summary.Volume, 8.0), Freed.AcceptedCondition && Freedom(Plate, Sketch) == 1 && Relative(Plate.Summary.Volume, 8.0) <= 1.0e-12 && Plate.Tree.Steps[0].Sketch.Constraints.size() == 6,
              Freed.Label + ": DOF 1, V 8 kept");

        // W24 refusals (with D2 back): each leaves the file and the history untouched
        Expect(ConstrainDraftingSketch(Plate, Sketch, Propose(Plate, Sketch, { Side }, SketchConstraintVerb::Dimension, 2.0)));
        const std::string File = EncodeDraftingDocument(Plate);
        const size_t History = Plate.UndoEntries.size();
        const DraftingCommandOutcome Again     = ConstrainDraftingSketch(Plate, Sketch, Propose(Plate, Sketch, { Bottom }, SketchConstraintVerb::Horizontal));
        const DraftingCommandOutcome Implied   = ConstrainDraftingSketch(Plate, Sketch, Propose(Plate, Sketch, { Top }, SketchConstraintVerb::Dimension));
        const DraftingCommandOutcome Clash     = ConstrainDraftingSketch(Plate, Sketch, Propose(Plate, Sketch, { Top }, SketchConstraintVerb::Dimension, 5.0));
        const DraftingCommandOutcome Valueless = ChangeDraftingDimension(Plate, Sketch, 0, 1.0);
        const DraftingCommandOutcome Negative  = ChangeDraftingDimension(Plate, Sketch, 5, -1.0);
        const DraftingCommandOutcome Missing   = ConstrainDraftingSketch(Plate, Key, Propose(Plate, Sketch, { Bottom }, SketchConstraintVerb::Horizontal));
        const DraftingCommandOutcome Beyond    = RemoveDraftingConstraint(Plate, Sketch, 40);
        const bool Untouched = EncodeDraftingDocument(Plate) == File && Plate.UndoEntries.size() == History;
        Tally(ConflictGate, 0.0, !Again.AcceptedCondition && Again.Refusal.find("already implied by H1") != std::string::npos,
              "second H on Line1 refused: " + Again.Refusal);
        Tally(ConflictGate, 0.0, !Implied.AcceptedCondition && Implied.Refusal.find("already implied by") != std::string::npos && Implied.Refusal.find("D1") != std::string::npos,
              "Length 4 on the top refused: " + Implied.Refusal);
        Tally(ConflictGate, 0.0, !Clash.AcceptedCondition && Clash.Refusal.find("conflicts with") != std::string::npos && Clash.Refusal.find("D1") != std::string::npos,
              "Length 5 on the top refused: " + Clash.Refusal);
        Tally(ConflictGate, 0.0, !Valueless.AcceptedCondition && !Negative.AcceptedCondition && !Missing.AcceptedCondition && !Beyond.AcceptedCondition && Untouched,
              "refused: " + Valueless.Refusal + " · " + Negative.Refusal + " · " + Missing.Refusal + " · " + Beyond.Refusal + "; file and history untouched");

        // W25 ① D2 removed again: the free far corner dragged to (4, 3) carries the top up → V 12; the preview is pure
        Expect(RemoveDraftingConstraint(Plate, Sketch, 6));
        const std::string BeforePreview = EncodeDraftingDocument(Plate);
        const SketchDragOutcome Preview = PreviewDraftingDrag(Plate, Sketch, 2, { 4.0, 3.0 });
        const bool Pure = EncodeDraftingDocument(Plate) == BeforePreview && Preview.Refusal.empty() && Preview.PinnedCondition;
        const DraftingCommandOutcome Dragged = DragDraftingSketchPoint(Plate, Sketch, 2, { 4.0, 3.0 });
        const Vector2 Far = Plate.Tree.Steps[0].Sketch.Points[2].Position;
        const Vector2 Near = Plate.Tree.Steps[0].Sketch.Points[3].Position;
        const double Miss = std::abs(Far.X - 4.0) + std::abs(Far.Y - 3.0) + std::abs(Near.X) + std::abs(Near.Y - 3.0);
        Tally(DragGate, Miss + Relative(Plate.Summary.Volume, 12.0), Pure && Dragged.AcceptedCondition && Miss <= 1.0e-9 && Relative(Plate.Summary.Volume, 12.0) <= 1.0e-9,
              Dragged.Label + ": corner at (4, 3), Line3.Start follows to (0, 3), V 12; the preview left the document alone");

        // W25 ② dragging the locked corner refuses; ③ codec byte-equal with F1 D1 and the dragged positions
        const DraftingCommandOutcome Stuck = DragDraftingSketchPoint(Plate, Sketch, 0, { -1.0, -1.0 });
        Tally(DragGate, 0.0, !Stuck.AcceptedCondition && Stuck.Refusal.find("fully constrained") != std::string::npos, "refused: " + Stuck.Refusal);
        const std::string Written = EncodeDraftingDocument(Plate);
        const DraftingDocumentFile Read = DecodeDraftingDocument(Written);
        DraftingDocument Copy = CreateDraftingDocument(Read.Title);
        Copy.Tree = Read.Tree; Copy.Captions = Read.Captions; Copy.Appearance = Read.Appearance;
        RegenerateDraftingDocument(Copy, true);
        Tally(DragGate, 0.0, Read.Refusal.empty() && EncodeDraftingDocument(Copy) == Written && Copy.Summary.Volume == Plate.Summary.Volume && Written.find("'F1'") != std::string::npos && Freedom(Copy, Sketch) == 1,
              "save → load → save byte-equal; F1 / D1 labels and DOF 1 survive the reload");
    }

    // ---- W26 … W28: reference dimensions (Shift+D / E), spline constraints (L / C / H), cross-sketch links
    {
        auto Relative = [](double Value, double Expected) { return std::abs(Value - Expected) / std::max(1.0e-12, std::abs(Expected)); };
        auto Propose = [](const DraftingDocument& Target, const std::string& Sketch, std::vector<SketchPick> Picks, SketchConstraintVerb Verb, double Value = std::numeric_limits<double>::quiet_NaN())
        {
            SketchConstraintProposal Proposal = ProposeSketchConstraint(DiagnoseDraftingSketch(Target, Sketch).Sketch, Picks, Verb);
            if (!std::isnan(Value)) Proposal.Constraint.Value = Value;
            return Proposal.Constraint;
        };
        auto Freedom = [](const DraftingDocument& Target, const std::string& Sketch) { return DiagnoseDraftingSketch(Target, Sketch).Solve.DegreesOfFreedom; };
        auto RoundTrip = [](const DraftingDocument& Source)
        {
            const std::string Written = EncodeDraftingDocument(Source);
            const DraftingDocumentFile Read = DecodeDraftingDocument(Written);
            DraftingDocument Copy = CreateDraftingDocument(Read.Title);
            Copy.Tree = Read.Tree; Copy.Captions = Read.Captions; Copy.Appearance = Read.Appearance;
            RegenerateDraftingDocument(Copy, true);
            return Read.Refusal.empty() && EncodeDraftingDocument(Copy) == Written && Copy.Summary.Volume == Source.Summary.Volume;
        };
        auto DrawRectangle = [](DraftingDocument& Target, Vector2 Low, Vector2 High)
        {
            DraftingDrawingRequest Drawing;
            Drawing.Kind = DraftingDrawingKind::Rectangle; Drawing.Points = { Low, High };
            return DrawDraftingSketch(Target, Drawing).AcceptedCondition ? Target.Tree.Steps.back().Label : std::string{};
        };
        auto Scientific = [](double Value) { char Text[32]; std::snprintf(Text, sizeof(Text), "%.3e", Value); return std::string(Text); };
        const SketchPick Bottom = { SketchPickKind::Curve, 0 };
        const SketchPick Side   = { SketchPickKind::Curve, 1 };

        // W26 ① a fully dimensioned 2 × 1 → 3 × 2 plate (DOF 0, V 6); Shift+D on its diagonal: D3 (reference) reads √13,
        //        DOF stays 0, the positions and the volume are bit-equal
        DraftingDocument Plate = CreateDraftingDocument("References");
        const std::string Sketch = DrawRectangle(Plate, { 0.0, 0.0 }, { 2.0, 1.0 });
        Expect(ExtrudeDraftingSketch(Plate, Sketch, 1.0, FeatureCombination::Separate));
        Expect(ConstrainDraftingSketch(Plate, Sketch, Propose(Plate, Sketch, { { SketchPickKind::Point, 0 } }, SketchConstraintVerb::Lock)));
        Expect(ConstrainDraftingSketch(Plate, Sketch, Propose(Plate, Sketch, { Bottom }, SketchConstraintVerb::Dimension, 3.0)));
        Expect(ConstrainDraftingSketch(Plate, Sketch, Propose(Plate, Sketch, { Side }, SketchConstraintVerb::Dimension, 2.0)));
        const std::vector<Vector2> Corners = [&] { std::vector<Vector2> Positions; for (const SketchPoint& Point : Plate.Tree.Steps[0].Sketch.Points) Positions.push_back(Point.Position); return Positions; }();
        const double Before = Plate.Summary.Volume;
        const SketchConstraintProposal Diagonal = ProposeSketchReference(DiagnoseDraftingSketch(Plate, Sketch).Sketch, { { SketchPickKind::Point, 0 }, { SketchPickKind::Point, 2 } });
        const DraftingCommandOutcome Referenced = ConstrainDraftingSketch(Plate, Sketch, Diagonal.Constraint);
        const SketchConstraint& Reading = Plate.Tree.Steps[0].Sketch.Constraints.back();
        bool Unmoved = Plate.Summary.Volume == Before;
        for (size_t Point = 0; Point < Corners.size(); ++Point) Unmoved = Unmoved && Plate.Tree.Steps[0].Sketch.Points[Point].Position.X == Corners[Point].X && Plate.Tree.Steps[0].Sketch.Points[Point].Position.Y == Corners[Point].Y;
        const DraftingSketchDiagnosis Diagnosed = DiagnoseDraftingSketch(Plate, Sketch);
        const int ReadingIndex = static_cast<int>(Plate.Tree.Steps[0].Sketch.Constraints.size()) - 1;
        Tally(ReferenceGate, std::abs(Reading.Value - std::sqrt(13.0)),
              Diagonal.Refusal.empty() && Referenced.AcceptedCondition && Reading.DrivenCondition && Reading.Label == "D3" && std::abs(Reading.Value - std::sqrt(13.0)) <= 1.0e-12 && Diagnosed.Solve.DegreesOfFreedom == 0
                  && Diagnosed.Solve.Constraints[ReadingIndex].Status == SketchConstraintStatus::Driven && Unmoved,
              Referenced.Label + ": " + DescribeSketchConstraint(Plate.Tree.Steps[0].Sketch, Reading) + " · DOF 0 · positions and V bit-equal");

        // W26 ② D2 = 4 → the reading follows to 5 (written back); editing the reading and making it driving are refused
        const DraftingCommandOutcome Taller = ChangeDraftingDimension(Plate, Sketch, 6, 4.0);
        const double Followed = Plate.Tree.Steps[0].Sketch.Constraints[ReadingIndex].Value;
        const std::string File = EncodeDraftingDocument(Plate);
        const size_t History = Plate.UndoEntries.size();
        const DraftingCommandOutcome Typed   = ChangeDraftingDimension(Plate, Sketch, ReadingIndex, 6.0);
        const DraftingCommandOutcome Driving = ToggleDraftingReference(Plate, Sketch, ReadingIndex);
        const DraftingCommandOutcome Plain   = ToggleDraftingReference(Plate, Sketch, 0);
        const bool Untouched = EncodeDraftingDocument(Plate) == File && Plate.UndoEntries.size() == History;
        Tally(ReferenceGate, std::abs(Followed - 5.0) + Relative(Plate.Summary.Volume, 12.0),
              Taller.AcceptedCondition && std::abs(Followed - 5.0) <= 1.0e-12 && Relative(Plate.Summary.Volume, 12.0) <= 1.0e-9 && !Typed.AcceptedCondition && Typed.Refusal.find("reference dimension") != std::string::npos
                  && !Driving.AcceptedCondition && Driving.Refusal.find("stays a reference") != std::string::npos && !Plain.AcceptedCondition && Untouched,
              Taller.Label + " → D3 reads 5 · refused: " + Typed.Refusal + " · " + Driving.Refusal + " · " + Plain.Refusal);

        // W26 ③ D2 removed (DOF 1): E makes D3 driving at its reading √(9 + 16) = 5 (DOF 0, nothing moves), E again frees it;
        //        undo / redo; save → load → save byte-equal with Reference = true
        Expect(RemoveDraftingConstraint(Plate, Sketch, 6));
        const int Loose = Freedom(Plate, Sketch);
        const double LooseVolume = Plate.Summary.Volume;
        const int Index = static_cast<int>(Plate.Tree.Steps[0].Sketch.Constraints.size()) - 1;
        const DraftingCommandOutcome Drive = ToggleDraftingReference(Plate, Sketch, Index);
        const int Driven = Freedom(Plate, Sketch);
        const double DrivenValue = Plate.Tree.Steps[0].Sketch.Constraints[Index].Value;
        const bool DrivenStill = Plate.Summary.Volume == LooseVolume;
        const DraftingCommandOutcome Release = ToggleDraftingReference(Plate, Sketch, Index);
        const int Released = Freedom(Plate, Sketch);
        const bool Undone = UndoDraftingCommand(Plate) && Freedom(Plate, Sketch) == 0 && !Plate.Tree.Steps[0].Sketch.Constraints[Index].DrivenCondition;
        const bool Redone = RedoDraftingCommand(Plate) && Freedom(Plate, Sketch) == 1 && Plate.Tree.Steps[0].Sketch.Constraints[Index].DrivenCondition;
        Tally(ReferenceGate, std::abs(DrivenValue - 5.0),
              Loose == 1 && Drive.AcceptedCondition && Driven == 0 && std::abs(DrivenValue - 5.0) <= 1.0e-12 && DrivenStill && Release.AcceptedCondition && Released == 1 && Undone && Redone,
              "DOF " + std::to_string(Loose) + " → " + Drive.Label + " " + std::to_string(Driven) + " (V bit-equal) → " + Release.Label + " " + std::to_string(Released) + "; undo / redo");
        const std::string Written = EncodeDraftingDocument(Plate);
        Tally(ReferenceGate, 0.0, RoundTrip(Plate) && Written.find("Reference = true") != std::string::npos, "save → load → save byte-equal; Reference = true written for D3 only");

        // W27 ① a NURBS curve drawn from (0, 0) and a line (−2, 0.5) → (0, 0), joined (ends welded): L gives T1 (G1), C gives
        //        K1 that replaces it (G2), H on the far end; each solve converges ≤ 10⁻¹², undo brings T1 back
        DraftingDocument Joint = CreateDraftingDocument("Spline joint");
        DraftingDrawingRequest Lead;
        Lead.Kind = DraftingDrawingKind::Line; Lead.Points = { { -2.0, 0.5 }, { 0.0, 0.0 } };
        Expect(DrawDraftingSketch(Joint, Lead));
        const std::string LineSketch = Joint.Tree.Steps.back().Label;
        DraftingDrawingRequest Sweep;
        Sweep.Kind = DraftingDrawingKind::Curve; Sweep.Form = SketchSplineForm::Nurbs; Sweep.Points = { { 0.0, 0.0 }, { 1.0, 1.0 }, { 2.0, 1.2 }, { 3.0, 0.4 } };
        Expect(DrawDraftingSketch(Joint, Sweep));
        const std::string CurveSketch = Joint.Tree.Steps.back().Label;
        const DraftingCommandOutcome Joined = JoinDraftingSketches(Joint, { LineSketch, CurveSketch });
        int LineCurve = -1, SplineCurve = -1;
        const SketchModel& Merged = Joint.Tree.Steps[0].Sketch;
        for (size_t Curve = 0; Curve < Merged.Curves.size(); ++Curve)
            (Merged.Curves[Curve].Kind == SketchCurveKind::Spline ? SplineCurve : LineCurve) = static_cast<int>(Curve);
        const bool Welded = SplineCurve >= 0 && LineCurve >= 0 && (Merged.Curves[SplineCurve].Points.front() == Merged.Curves[LineCurve].End || Merged.Curves[SplineCurve].Points.front() == Merged.Curves[LineCurve].Start);
        const int FarEnd = SplineCurve >= 0 ? Merged.Curves[SplineCurve].Points.back() : -1;
        const int Open = Freedom(Joint, LineSketch);
        const DraftingCommandOutcome Tangent = ConstrainDraftingSketch(Joint, LineSketch, Propose(Joint, LineSketch, { { SketchPickKind::Curve, SplineCurve }, { SketchPickKind::Curve, LineCurve } }, SketchConstraintVerb::Parallel));
        const DraftingSketchDiagnosis AfterTangent = DiagnoseDraftingSketch(Joint, LineSketch);
        const DraftingCommandOutcome Curvature = ConstrainDraftingSketch(Joint, LineSketch, Propose(Joint, LineSketch, { { SketchPickKind::Curve, SplineCurve }, { SketchPickKind::Curve, LineCurve } }, SketchConstraintVerb::Curvature));
        const DraftingSketchDiagnosis AfterCurvature = DiagnoseDraftingSketch(Joint, LineSketch);
        const std::vector<SketchConstraint>& Joints = Joint.Tree.Steps[0].Sketch.Constraints;
        const bool Replaced = !Joints.empty() && Joints.back().Kind == SketchConstraintKind::SplineCurvature && Joints.back().Label == "K1"
                           && std::none_of(Joints.begin(), Joints.end(), [](const SketchConstraint& Constraint) { return Constraint.Kind == SketchConstraintKind::SplineTangent; });
        Tally(SplineGate, std::max(AfterTangent.Solve.MaxResidual, AfterCurvature.Solve.MaxResidual),
              Joined.AcceptedCondition && Welded && Tangent.AcceptedCondition && AfterTangent.Solve.Converged && AfterTangent.Solve.MaxResidual <= 1.0e-12 && AfterTangent.Solve.DegreesOfFreedom == Open - 1
                  && Curvature.AcceptedCondition && Curvature.Label.find("replaces T1") != std::string::npos && Replaced && AfterCurvature.Solve.Converged && AfterCurvature.Solve.MaxResidual <= 1.0e-12
                  && AfterCurvature.Solve.DegreesOfFreedom == Open - 2,
              "DOF " + std::to_string(Open) + " → " + Tangent.Label + " " + std::to_string(AfterTangent.Solve.DegreesOfFreedom) + " → " + Curvature.Label + " " + std::to_string(AfterCurvature.Solve.DegreesOfFreedom));
        const DraftingCommandOutcome Level = ConstrainDraftingSketch(Joint, LineSketch, Propose(Joint, LineSketch, { { SketchPickKind::Curve, SplineCurve }, { SketchPickKind::Point, FarEnd } }, SketchConstraintVerb::Horizontal));
        const DraftingSketchDiagnosis AfterLevel = DiagnoseDraftingSketch(Joint, LineSketch);
        const std::string LevelLabel = Joint.Tree.Steps[0].Sketch.Constraints.back().Label;
        const bool Back = UndoDraftingCommand(Joint) && UndoDraftingCommand(Joint)
                       && std::any_of(Joint.Tree.Steps[0].Sketch.Constraints.begin(), Joint.Tree.Steps[0].Sketch.Constraints.end(), [](const SketchConstraint& Constraint) { return Constraint.Kind == SketchConstraintKind::SplineTangent && Constraint.Label == "T1"; });
        const bool Forward = RedoDraftingCommand(Joint) && RedoDraftingCommand(Joint) && Joint.Tree.Steps[0].Sketch.Constraints.back().Label == LevelLabel;
        Tally(SplineGate, AfterLevel.Solve.MaxResidual,
              Level.AcceptedCondition && LevelLabel == "H1" && AfterLevel.Solve.Converged && AfterLevel.Solve.MaxResidual <= 1.0e-12 && AfterLevel.Solve.DegreesOfFreedom == Open - 3 && Back && Forward,
              Level.Label + " · DOF " + std::to_string(AfterLevel.Solve.DegreesOfFreedom) + "; undo × 2 brings T1 back, redo × 2 restores K1 + H1");

        // W27 ② refusals: G2 on a natural cubic spline, L on a spline and a curve that do not meet; save → load → save byte-equal
        DraftingDocument Natural = CreateDraftingDocument("Natural");
        Expect(DrawDraftingSketch(Natural, Lead));
        const std::string NaturalSketch = Natural.Tree.Steps.back().Label;
        DraftingDrawingRequest Cubic = Sweep;
        Cubic.Form = SketchSplineForm::CubicSpline;
        Expect(DrawDraftingSketch(Natural, Cubic));
        Expect(JoinDraftingSketches(Natural, { NaturalSketch, Natural.Tree.Steps.back().Label }));
        const SketchConstraintProposal NaturalProposal = ProposeSketchConstraint(DiagnoseDraftingSketch(Natural, NaturalSketch).Sketch, { { SketchPickKind::Curve, SplineCurve }, { SketchPickKind::Curve, LineCurve } }, SketchConstraintVerb::Curvature);
        Tally(SplineGate, 0.0, !NaturalProposal.Refusal.empty() && NaturalProposal.Refusal.find("zero curvature") != std::string::npos, "refused: " + NaturalProposal.Refusal);
        Tally(SplineGate, 0.0, RoundTrip(Joint) && EncodeDraftingDocument(Joint).find("Kind = 22") != std::string::npos && EncodeDraftingDocument(Joint).find("Kind = 24") != std::string::npos,
              "save → load → save byte-equal with SplineCurvature (22) and SplineDirection (24)");

        // W28 ① Sketch A: a 3 × 2 plate (DOF 0, V 6); Sketch B (later, same plane): a free line (5, 3) → (6, 4). O on B's start
        //        and A's far corner links the corner into B (one undo entry); O on B's end and A's top line links the line
        //        (construction, on linked points); B's start sits on (3, 2), its end on y = 2
        DraftingDocument Linked = CreateDraftingDocument("Links");
        const std::string Source = DrawRectangle(Linked, { 0.0, 0.0 }, { 2.0, 1.0 });
        Expect(ExtrudeDraftingSketch(Linked, Source, 1.0, FeatureCombination::Separate));
        Expect(ConstrainDraftingSketch(Linked, Source, Propose(Linked, Source, { { SketchPickKind::Point, 0 } }, SketchConstraintVerb::Lock)));
        Expect(ConstrainDraftingSketch(Linked, Source, Propose(Linked, Source, { Bottom }, SketchConstraintVerb::Dimension, 3.0)));
        Expect(ConstrainDraftingSketch(Linked, Source, Propose(Linked, Source, { Side }, SketchConstraintVerb::Dimension, 2.0)));
        DraftingDrawingRequest Spoke;
        Spoke.Kind = DraftingDrawingKind::Line; Spoke.Points = { { 5.0, 3.0 }, { 6.0, 4.0 } };
        Expect(DrawDraftingSketch(Linked, Spoke));
        const std::string Follower = Linked.Tree.Steps.back().Label;
        const std::vector<std::string> Sources = DraftingLinkSources(Linked, Follower);
        const int Unlinked = Freedom(Linked, Follower);
        const std::vector<DraftingSketchPick> CornerPicks = { { {}, { SketchPickKind::Point, 0 } }, { Source, { SketchPickKind::Point, 2 } } };
        const DraftingLinkedPicks CornerLinked = LinkDraftingPicks(Linked, Follower, CornerPicks);
        const SketchConstraintProposal CornerProposal = ProposeSketchConstraint(CornerLinked.Sketch, CornerLinked.Picks, SketchConstraintVerb::On);
        const size_t BeforeCorner = Linked.UndoEntries.size();
        const DraftingCommandOutcome Cornered = ConstrainDraftingSketch(Linked, Follower, CornerProposal.Constraint, CornerPicks);
        const bool OneEntry = Linked.UndoEntries.size() == BeforeCorner + 1;
        const int AfterCorner = Freedom(Linked, Follower);
        const std::vector<DraftingSketchPick> EdgePicks = { { {}, { SketchPickKind::Point, 1 } }, { Source, { SketchPickKind::Curve, 2 } } };
        const DraftingLinkedPicks EdgeLinked = LinkDraftingPicks(Linked, Follower, EdgePicks);
        const SketchConstraintProposal EdgeProposal = ProposeSketchConstraint(EdgeLinked.Sketch, EdgeLinked.Picks, SketchConstraintVerb::On);
        const DraftingCommandOutcome Edged = ConstrainDraftingSketch(Linked, Follower, EdgeProposal.Constraint, EdgePicks);
        const int AfterEdge = Freedom(Linked, Follower);
        const SketchModel& Solved = Linked.Regeneration.Sketches.at(Follower);
        const double CornerGap = std::abs(Solved.Points[0].Position.X - 3.0) + std::abs(Solved.Points[0].Position.Y - 2.0);
        const double EdgeGap   = std::abs(Solved.Points[1].Position.Y - 2.0);
        const SketchModel& Authored = Linked.Tree.Steps.back().Sketch;
        const bool Shapes = Authored.Points.size() == 4 && Authored.Points[2].Link.Sketch == Source && Authored.Points[2].Link.Index == 2 && Authored.Curves.size() == 2 && Authored.Curves[1].Construction
                         && Authored.Curves[1].Link.Sketch == Source && Authored.Curves[1].Link.Index == 2 && CornerLinked.Appended == 1 && EdgeLinked.Appended == 2;
        Tally(LinkGate, CornerGap + EdgeGap,
              Sources.size() == 1 && Sources[0] == Source && CornerLinked.Refusal.empty() && CornerProposal.Refusal.empty() && Cornered.AcceptedCondition && OneEntry && Edged.AcceptedCondition && Shapes
                  && Unlinked == 4 && AfterCorner == 2 && AfterEdge == 1 && CornerGap <= 1.0e-12 && EdgeGap <= 1.0e-12,
              "DOF " + std::to_string(Unlinked) + " → " + Cornered.Label + " " + std::to_string(AfterCorner) + " → " + Edged.Label + " " + std::to_string(AfterEdge) + "; corner gap " + Scientific(CornerGap) + ", edge gap " + Scientific(EdgeGap));

        // W28 ② A's D1 = 4: B regenerates with its start on (4, 2) and its end still on the top line; the K session's view
        //        (DiagnoseDraftingSketch) shows the same before B is touched
        const DraftingCommandOutcome Widened = ChangeDraftingDimension(Linked, Source, 5, 4.0);
        const SketchModel& Moved = Linked.Regeneration.Sketches.at(Follower);
        const DraftingSketchDiagnosis Viewed = DiagnoseDraftingSketch(Linked, Follower);
        const double MovedGap = std::abs(Moved.Points[0].Position.X - 4.0) + std::abs(Moved.Points[0].Position.Y - 2.0) + std::abs(Moved.Points[1].Position.Y - 2.0);
        const double ViewGap  = std::abs(Viewed.Sketch.Points[0].Position.X - 4.0) + std::abs(Viewed.Sketch.Points[0].Position.Y - 2.0);
        bool Quiet = true;
        for (const FeatureStepReport& Report : Linked.Regeneration.Steps) Quiet = Quiet && Report.Notes.empty();
        Tally(LinkGate, MovedGap + ViewGap, Widened.AcceptedCondition && MovedGap <= 1.0e-12 && ViewGap <= 1.0e-12 && Quiet && Relative(Linked.Summary.Volume, 8.0) <= 1.0e-9,
              Widened.Label + ": B's start follows to (4, 2), end on y = 2 (gap " + Scientific(MovedGap) + "), K view gap " + Scientific(ViewGap) + ", no notes, V 8");

        // W28 ③ refusals: A linking to the later B, dragging the linked point, a non-existent source point — untouched
        const std::string LinkedFile = EncodeDraftingDocument(Linked);
        const size_t LinkedHistory = Linked.UndoEntries.size();
        const DraftingLinkedPicks Later = LinkDraftingPicks(Linked, Source, { { {}, { SketchPickKind::Point, 0 } }, { Follower, { SketchPickKind::Point, 0 } } });
        const DraftingCommandOutcome Backward = ConstrainDraftingSketch(Linked, Source, CornerProposal.Constraint, { { {}, { SketchPickKind::Point, 0 } }, { Follower, { SketchPickKind::Point, 0 } } });
        const DraftingCommandOutcome Held = DragDraftingSketchPoint(Linked, Follower, 2, { 9.0, 9.0 });
        const DraftingLinkedPicks Beyond = LinkDraftingPicks(Linked, Follower, { { Source, { SketchPickKind::Point, 40 } } });
        const bool LinkedUntouched = EncodeDraftingDocument(Linked) == LinkedFile && Linked.UndoEntries.size() == LinkedHistory;
        Tally(LinkGate, 0.0, Later.Refusal.find("comes after") != std::string::npos && !Backward.AcceptedCondition && !Held.AcceptedCondition && Held.Refusal.find("is linked to") != std::string::npos
                  && Beyond.Refusal.find("has no point 40") != std::string::npos && LinkedUntouched,
              "refused: " + Later.Refusal + " · " + Held.Refusal + " · " + Beyond.Refusal + "; file and history untouched");

        // W28 ④ save → load → save byte-equal (Links, LinkSketch); Delete unlinks the corner: B's start stays at (4, 2), free,
        //        and no longer follows A (D1 = 5 leaves it); undo relinks it
        Tally(LinkGate, 0.0, RoundTrip(Linked) && LinkedFile.find("[[Steps.Sketch.Links]]") != std::string::npos && LinkedFile.find("LinkSketch = ") != std::string::npos,
              "save → load → save byte-equal with [[Steps.Sketch.Links]] { Point, Sketch, Index } and the construction line's LinkSketch / LinkIndex");
        const DraftingCommandOutcome Unlink = UnlinkDraftingGeometry(Linked, Follower, { SketchPickKind::Point, 2 });
        const int Freed = Freedom(Linked, Follower);
        Expect(ChangeDraftingDimension(Linked, Source, 5, 5.0));
        const SketchModel& Stayed = Linked.Regeneration.Sketches.at(Follower);
        const double StayGap = std::abs(Stayed.Points[0].Position.X - 4.0) + std::abs(Stayed.Points[0].Position.Y - 2.0);
        const bool Relinked = UndoDraftingCommand(Linked) && UndoDraftingCommand(Linked) && !Linked.Tree.Steps.back().Sketch.Points[2].Link.Sketch.empty();
        Tally(LinkGate, StayGap, Unlink.AcceptedCondition && Freed == AfterEdge + 2 && StayGap <= 1.0e-12 && Relinked,
              Unlink.Label + ": DOF " + std::to_string(Freed) + ", start stays at (4, 2) when D1 = 5 (gap " + Scientific(StayGap) + "); undo relinks");
    }

    // ---- W29 … W30: Revolve (Alt+R) and Evolution (Shift+P / Alt+E)
    {
        auto Relative = [](double Value, double Expected) { return std::abs(Value - Expected) / std::max(1.0e-12, std::abs(Expected)); };
        auto Figures  = [](const DraftingDocument& Target, const std::string& Key) -> const DraftingBodyFigures*
        {
            for (const DraftingBodyFigures& Body : Target.Summary.Bodies) if (Body.Key == Key) return &Body;
            return nullptr;
        };
        auto VolumeOf = [&](const DraftingDocument& Target, const std::string& Key) { const DraftingBodyFigures* Body = Figures(Target, Key); return Body ? Body->Volume : -1.0; };
        auto Draw = [](DraftingDocument& Target, DraftingDrawingKind Kind, std::vector<Vector2> Points, int GridPlane = 0)
        {
            DraftingDrawingRequest Drawing;
            Drawing.Kind = Kind; Drawing.Points = std::move(Points); Drawing.GridPlane = GridPlane;
            return DrawDraftingSketch(Target, Drawing).AcceptedCondition ? Target.Tree.Steps.back().Label : std::string{};
        };
        auto RoundTrip = [](const DraftingDocument& Source)
        {
            const std::string Written = EncodeDraftingDocument(Source);
            const DraftingDocumentFile Read = DecodeDraftingDocument(Written);
            DraftingDocument Copy = CreateDraftingDocument(Read.Title);
            Copy.Tree = Read.Tree; Copy.Captions = Read.Captions; Copy.Appearance = Read.Appearance;
            RegenerateDraftingDocument(Copy, true);
            return Read.Refusal.empty() && EncodeDraftingDocument(Copy) == Written && Copy.Summary.Volume == Source.Summary.Volume;
        };
        auto Scientific = [](double Value) { char Text[32]; std::snprintf(Text, sizeof(Text), "%.3e", Value); return std::string(Text); };
        auto Verdict = [](const DraftingCommandOutcome& Outcome) { return Outcome.AcceptedCondition ? Outcome.Label : Outcome.Label + " REFUSED (" + Outcome.Refusal + ")"; };
        auto Occurrences = [](const std::string& Text, const std::string& Word) { int Count = 0; for (size_t At = Text.find(Word); At != std::string::npos; At = Text.find(Word, At + 1)) ++Count; return Count; };

        // W29 ① a 1 × 1 square at x ∈ [1, 2] on XY: the proposed axis is the sketch y-axis (the profile lies right of it);
        //        a full turn → the first body (New) is an annulus π(R² − r²)h = 3π
        DraftingDocument Turned = CreateDraftingDocument("Revolve");
        const std::string Ring = Draw(Turned, DraftingDrawingKind::Rectangle, { { 1.0, 0.0 }, { 2.0, 1.0 } });
        const DraftingRevolveAxis Proposed = ProposeDraftingRevolveAxis(Turned, Ring);
        const DraftingCommandOutcome Annulus = RevolveDraftingSketch(Turned, Ring, Proposed, 2.0 * Pi, FeatureCombination::New);
        const std::string RingKey = Annulus.AcceptedCondition ? Turned.Tree.Steps.back().Label : std::string{};
        const double RingVolume = VolumeOf(Turned, RingKey);
        Tally(RevolveGate, Relative(RingVolume, 3.0 * Pi),
              Annulus.AcceptedCondition && Proposed.Line < 0 && Proposed.Direction.X == 0.0 && Proposed.Direction.Y == 1.0 && Turned.Tree.Steps.back().Combination == FeatureCombination::New
                  && Relative(RingVolume, 3.0 * Pi) <= 1.0e-9 && Turned.Summary.SoundCondition,
              Verdict(Annulus) + ": proposed axis " + DescribeDraftingRevolveAxis(Turned, Ring, Proposed) + ", V 3π (rel. " + Scientific(Relative(RingVolume, 3.0 * Pi)) + "), New, sound");

        // W29 ② a 1.5 × 1 plate on the y-axis (x ∈ [0, 1.5], y ∈ [−½, ½]) turned about its own left edge (a line of the
        //        sketch, by identifier) and united with the annulus: 3π + 2.25π − overlap 0.625π = 4.625π; undo / redo exact
        const std::string Plate = Draw(Turned, DraftingDrawingKind::Rectangle, { { 0.0, -0.5 }, { 1.5, 0.5 } });
        DraftingRevolveAxis Edge;
        for (const SketchCurve& Curve : Turned.Regeneration.Sketches.at(Plate).Curves)
        {
            const SketchModel& Model = Turned.Regeneration.Sketches.at(Plate);
            if (Curve.Kind == SketchCurveKind::Line && Model.Points[Curve.Start].Position.X == 0.0 && Model.Points[Curve.End].Position.X == 0.0) Edge.Line = Curve.Identifier;
        }
        const DraftingCommandOutcome United = RevolveDraftingSketch(Turned, Plate, Edge, 2.0 * Pi, FeatureCombination::Union, RingKey);
        const double UnionVolume = VolumeOf(Turned, RingKey);
        const bool   Rewound = UndoDraftingCommand(Turned) && VolumeOf(Turned, RingKey) == RingVolume && RedoDraftingCommand(Turned) && VolumeOf(Turned, RingKey) == UnionVolume;
        Tally(RevolveGate, Relative(UnionVolume, 4.625 * Pi),
              United.AcceptedCondition && Edge.Line >= 0 && Relative(UnionVolume, 4.625 * Pi) <= 1.0e-9 && Turned.Summary.Bodies.size() == 1 && Turned.Summary.SoundCondition && Rewound,
              Verdict(United) + ": axis by line identifier " + std::to_string(Edge.Line) + ", V 4.625π (rel. " + Scientific(Relative(UnionVolume, 4.625 * Pi)) + "), one sound body; undo / redo bit-equal");

        // W29 ③ a 2 × 1 plate across the y-axis (x ∈ [−1, 1], y ∈ [1, 2]): the proposal falls back to the sketch x-axis;
        //        a quarter turn → (θ/2)(R² − r²)w = 3π/2
        DraftingDocument Quarter = CreateDraftingDocument("Quarter");
        const std::string Bar = Draw(Quarter, DraftingDrawingKind::Rectangle, { { -1.0, 1.0 }, { 1.0, 2.0 } });
        const DraftingRevolveAxis Across = ProposeDraftingRevolveAxis(Quarter, Bar);
        const DraftingCommandOutcome Bent = RevolveDraftingSketch(Quarter, Bar, Across, 0.5 * Pi, FeatureCombination::Separate);
        const double BentVolume = VolumeOf(Quarter, Bent.AcceptedCondition ? Quarter.Tree.Steps.back().Label : std::string{});
        Tally(RevolveGate, Relative(BentVolume, 1.5 * Pi),
              Bent.AcceptedCondition && Across.Line < 0 && Across.Direction.X == 1.0 && Across.Direction.Y == 0.0 && Relative(BentVolume, 1.5 * Pi) <= 1.0e-9 && Quarter.Summary.SoundCondition,
              Verdict(Bent) + ": proposed " + DescribeDraftingRevolveAxis(Quarter, Bar, Across) + ", V 3π/2 (rel. " + Scientific(Relative(BentVolume, 1.5 * Pi)) + "), sound");

        // W29 ④ refusals leave file and history alone: no angle, more than a turn, a profile across the axis, no sketch,
        //        Difference with nothing to subtract from
        DraftingDocument Empty = CreateDraftingDocument("Refusals");
        const std::string Across2 = Draw(Empty, DraftingDrawingKind::Rectangle, { { -1.0, 0.0 }, { 1.0, 1.0 } });
        const std::string Before  = EncodeDraftingDocument(Empty);
        const DraftingRevolveAxis YAxis{ -1, { 0.0, 0.0 }, { 0.0, 1.0 } };
        const DraftingCommandOutcome Nothing  = RevolveDraftingSketch(Empty, Across2, Across, 0.0);
        const DraftingCommandOutcome Overturn = RevolveDraftingSketch(Empty, Across2, Across, 2.5 * Pi);
        const DraftingCommandOutcome Crossing = RevolveDraftingSketch(Empty, Across2, YAxis, Pi);
        const DraftingCommandOutcome Missing  = RevolveDraftingSketch(Empty, "Sketch#99", YAxis, Pi);
        const DraftingCommandOutcome Subtract = RevolveDraftingSketch(Empty, Across2, Across, Pi, FeatureCombination::Difference);
        Tally(RevolveGate, 0.0, !Nothing.AcceptedCondition && !Overturn.AcceptedCondition && !Crossing.AcceptedCondition && !Missing.AcceptedCondition && !Subtract.AcceptedCondition
                                && EncodeDraftingDocument(Empty) == Before && Crossing.Refusal.find("crosses the axis") != std::string::npos,
              "refused: " + Nothing.Refusal + " · " + Overturn.Refusal + " · " + Crossing.Refusal + " · " + Missing.Refusal + " · " + Subtract.Refusal + "; file untouched");

        // W29 ⑤ codec: the point + direction axis is written only without a line (older files stay byte-equal)
        const std::string TurnedFile = EncodeDraftingDocument(Turned), QuarterFile = EncodeDraftingDocument(Quarter);
        Tally(RevolveGate, 0.0, RoundTrip(Turned) && RoundTrip(Quarter) && Occurrences(TurnedFile, "AxisDirection") == 1 && QuarterFile.find("AxisDirection = [ 1.0, 0.0 ]") != std::string::npos,
              "save → load → save byte-equal; AxisOrigin / AxisDirection only on the y- / x-axis revolves (" + std::to_string(Occurrences(TurnedFile, "AxisDirection")) + " in the union file)");

        // W30 ① a 1 × ½ rectangle on XY and a line (0, 0) → (0, 2) on XZ (world +z): the proposal takes the line sketch;
        //        the rigid sweep is a prism A·L = 1
        DraftingDocument Swept = CreateDraftingDocument("Evolution");
        const std::string Slab = Draw(Swept, DraftingDrawingKind::Rectangle, { { 0.0, 0.0 }, { 1.0, 0.5 } });
        const std::string Rail = Draw(Swept, DraftingDrawingKind::Line, { { 0.0, 0.0 }, { 0.0, 2.0 } }, 1);
        const std::string Chosen = ProposeDraftingEvolutionPath(Swept, Slab);
        const DraftingCommandOutcome Prism = EvolveDraftingProfile(Swept, Slab, Chosen, FeatureCombination::New);
        const std::string PrismKey = Prism.AcceptedCondition ? Swept.Tree.Steps.back().Label : std::string{};
        Tally(EvolutionGate, Relative(VolumeOf(Swept, PrismKey), 1.0),
              Prism.AcceptedCondition && Chosen == Rail && Relative(VolumeOf(Swept, PrismKey), 1.0) <= 1.0e-9 && Swept.Summary.SoundCondition && Swept.Tree.Steps.back().Combination == FeatureCombination::New,
              Verdict(Prism) + ": path proposed " + Chosen + ", V 1 (rel. " + Scientific(Relative(VolumeOf(Swept, PrismKey), 1.0)) + "), New, sound");

        // W30 ② a circle r ¼ about the origin swept along a quarter arc of radius 2 on XZ (start ⟂ the profile): Pappus
        //        πr² · Rπ/2 = π²/16 (the side is a NURBS skin within 10⁻⁶ of the canal)
        DraftingDocument Bend = CreateDraftingDocument("Bend");
        const std::string Disc = Draw(Bend, DraftingDrawingKind::Circle, { { -0.25, 0.0 }, { 0.25, 0.0 } });
        const std::string Arc  = Draw(Bend, DraftingDrawingKind::Arc, { { 0.0, 0.0 }, { 2.0, 2.0 }, { 2.0 - std::sqrt(2.0), std::sqrt(2.0) } }, 1);
        const DraftingCommandOutcome Elbow = EvolveDraftingProfile(Bend, Disc, Arc, FeatureCombination::Separate);
        const double ElbowVolume = VolumeOf(Bend, Elbow.AcceptedCondition ? Bend.Tree.Steps.back().Label : std::string{});
        Tally(EvolutionGate, Relative(ElbowVolume, Pi * Pi / 16.0),
              Elbow.AcceptedCondition && Relative(ElbowVolume, Pi * Pi / 16.0) <= 1.0e-6 && Bend.Summary.SoundCondition,
              Verdict(Elbow) + ": the arc is stored from (2, 2) (counter-clockwise), the sweep starts at its end nearer the profile; V π²/16 (rel. " + Scientific(Relative(ElbowVolume, Pi * Pi / 16.0)) + "), sound");

        // W30 ③ groove: a 2 × 1 × 1 block (XZ rectangle x ∈ [−1, 1], z ∈ [½, 1½] extruded 1 along −y) minus a ½ × ½
        //        square swept along +z through it: the block keeps 2 − ½ · ¼ · 1 = 1.875 (the square straddles y = 0)
        DraftingDocument Groove = CreateDraftingDocument("Groove");
        const std::string Face = Draw(Groove, DraftingDrawingKind::Rectangle, { { -1.0, 0.5 }, { 1.0, 1.5 } }, 1);
        Expect(ExtrudeDraftingSketch(Groove, Face, 1.0, FeatureCombination::Separate));
        const std::string BlockKey = Groove.Tree.Steps.back().Label;
        const double BlockVolume = VolumeOf(Groove, BlockKey);
        const std::string Square = Draw(Groove, DraftingDrawingKind::Rectangle, { { -0.25, -0.25 }, { 0.25, 0.25 } });
        const std::string Spine  = Draw(Groove, DraftingDrawingKind::Line, { { 0.0, 0.0 }, { 0.0, 2.0 } }, 1);
        const DraftingCommandOutcome Cut = EvolveDraftingProfile(Groove, Square, Spine, FeatureCombination::Difference, BlockKey);
        const double GrooveVolume = VolumeOf(Groove, BlockKey);
        const double GrooveExpected = BlockVolume - 0.125;
        Tally(EvolutionGate, Relative(GrooveVolume, GrooveExpected),
              Cut.AcceptedCondition && Relative(BlockVolume, 2.0) <= 1.0e-12 && Relative(GrooveVolume, GrooveExpected) <= 1.0e-6 && Groove.Summary.Bodies.size() == 1 && Groove.Summary.SoundCondition,
              Verdict(Cut) + ": block 2 → " + std::to_string(GrooveVolume) + " vs 1.875 (rel. " + Scientific(Relative(GrooveVolume, GrooveExpected)) + "), one sound body");

        // W30 ④ refusals (path = profile, no path, a 4-curve path, a path leaving along the profile plane, Difference with
        //        no body) leave the file alone; codec byte-equal with Kind 16 and its Path
        const std::string Flat = Draw(Swept, DraftingDrawingKind::Line, { { 3.0, 0.0 }, { 5.0, 0.0 } });
        const std::string SweptBefore = EncodeDraftingDocument(Swept);
        const DraftingCommandOutcome Self   = EvolveDraftingProfile(Swept, Slab, Slab);
        const DraftingCommandOutcome NoPath = EvolveDraftingProfile(Swept, Slab, "");
        const DraftingCommandOutcome Many   = EvolveDraftingProfile(Swept, Rail, Slab);
        const DraftingCommandOutcome InPlane = EvolveDraftingProfile(Swept, Slab, Flat);
        DraftingDocument Bare = CreateDraftingDocument("Bare");
        const std::string BareSlab = Draw(Bare, DraftingDrawingKind::Rectangle, { { 0.0, 0.0 }, { 1.0, 0.5 } });
        const std::string BareRail = Draw(Bare, DraftingDrawingKind::Line, { { 0.0, 0.0 }, { 0.0, 2.0 } }, 1);
        const DraftingCommandOutcome Hollow = EvolveDraftingProfile(Bare, BareSlab, BareRail, FeatureCombination::Difference);
        const std::string SweptAfter = EncodeDraftingDocument(Swept);
        Tally(EvolutionGate, 0.0, !Self.AcceptedCondition && !NoPath.AcceptedCondition && !Many.AcceptedCondition && !InPlane.AcceptedCondition && !Hollow.AcceptedCondition && SweptAfter == SweptBefore,
              "refused: " + Self.Refusal + " · " + NoPath.Refusal + " · " + Many.Refusal + " · " + InPlane.Refusal + " · " + Hollow.Refusal + "; file untouched");
        Tally(EvolutionGate, 0.0, RoundTrip(Swept) && RoundTrip(Bend) && RoundTrip(Groove) && SweptAfter.find("Kind = 16") != std::string::npos && SweptAfter.find("Path = '" + Rail + "'") != std::string::npos,
              "save → load → save byte-equal with the Evolution (16) steps and Path = '" + Rail + "'");
    }

    // ---- W31 incremental replay: identical to full replays, resume points, speed
    {
        // A copy regenerated from scratch (cache off): the reference every incremental state must equal.
        auto FullReplay = [](const DraftingDocument& Source)
        {
            DraftingDocument Copy = CreateDraftingDocument(Source.Title);
            Copy.Tree      = Source.Tree;
            Copy.Captions  = Source.Captions;
            Copy.Selection = Source.Selection;
            Copy.Replay.EnabledCondition = false;
            RegenerateDraftingDocument(Copy);
            return Copy;
        };
        auto Identical = [](const DraftingDocument& First, const DraftingDocument& Second)
        {
            const DraftingBodySummary& A = First.Summary;
            const DraftingBodySummary& B = Second.Summary;
            if (A.Volume != B.Volume || A.Faces != B.Faces || A.Edges != B.Edges || A.Vertices != B.Vertices || A.Genus != B.Genus || A.Bodies.size() != B.Bodies.size()) return false;
            for (size_t Index = 0; Index < A.Bodies.size(); ++Index) if (A.Bodies[Index].Key != B.Bodies[Index].Key || A.Bodies[Index].Volume != B.Bodies[Index].Volume) return false;
            const RegenerationOutcome& P = First.Regeneration;
            const RegenerationOutcome& Q = Second.Regeneration;
            if (P.Bodies.size() != Q.Bodies.size() || P.Steps.size() != Q.Steps.size() || P.Active != Q.Active || P.Refused != Q.Refused || P.Curves.size() != Q.Curves.size()) return false;
            for (size_t Index = 0; Index < P.Bodies.size(); ++Index)
            {
                const RegeneratedBody& X = P.Bodies[Index];
                const RegeneratedBody& Y = Q.Bodies[Index];
                if (X.Key != Y.Key || X.Body.Slot != Y.Body.Slot || X.Body.Generation != Y.Body.Generation) return false;
                if (X.Naming.Faces != Y.Naming.Faces || X.Naming.Edges != Y.Naming.Edges || X.Naming.Vertices != Y.Naming.Vertices) return false;
            }
            for (size_t Index = 0; Index < P.Steps.size(); ++Index)
            {
                if (P.Steps[Index].Status != Q.Steps[Index].Status || P.Steps[Index].Refusal != Q.Steps[Index].Refusal || P.Steps[Index].Body.Slot != Q.Steps[Index].Body.Slot) return false;
            }
            return EncodeDraftingDocument(First) == EncodeDraftingDocument(Second);
        };
        auto Milliseconds = [](auto&& Action)
        {
            const auto Started = std::chrono::steady_clock::now();
            Action();
            return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - Started).count();
        };
        auto Ms = [](double Value) { char Text[32]; std::snprintf(Text, sizeof Text, "%.1f ms", Value); return std::string(Text); };
        auto Times = [](double Value) { char Text[32]; std::snprintf(Text, sizeof Text, "%.1f×", Value); return std::string(Text); };

        // Build: block + 8 bosses (union) + 4 holes (difference) + a front fillet, every command checked against a full replay.
        DraftingDocument Studded = CreateDraftingDocument("Studded");
        int  Commands = 0, IdenticalCount = 0, AppendResumes = 0;
        auto Command = [&](DraftingCommandOutcome Outcome)
        {
            const size_t Before = Studded.UndoEntries.empty() ? 0 : Studded.UndoEntries.back().Tree.Steps.size();
            Expect(Outcome);
            ++Commands;
            IdenticalCount += Identical(Studded, FullReplay(Studded)) ? 1 : 0;
            AppendResumes  += static_cast<size_t>(Studded.Regeneration.Resumed) == Before ? 1 : 0;
        };
        DraftingConstructRequest Plate;
        Plate.Kind = DraftingConstructKind::Block; Plate.Width = 8.0; Plate.Depth = 3.0; Plate.Height = 0.6;
        Command(ConstructDraftingPrimitive(Studded, Plate));
        for (int Boss = 0; Boss < 8; ++Boss)
        {
            DraftingConstructRequest Stud;
            Stud.Kind = DraftingConstructKind::Cylinder; Stud.Position = { 0.75 + 0.9 * Boss, 2.1, 0.3 }; Stud.Radius = 0.25 + 0.01 * Boss; Stud.Height = 0.5 + 0.05 * Boss;
            Stud.Combination = FeatureCombination::Union;
            Command(ConstructDraftingPrimitive(Studded, Stud));
        }
        for (int Hole = 0; Hole < 4; ++Hole)
        {
            DraftingConstructRequest Drill;
            Drill.Kind = DraftingConstructKind::Cylinder; Drill.Position = { 1.3 + 1.8 * Hole, 0.9, -0.5 }; Drill.Radius = 0.2; Drill.Height = 2.0;
            Drill.Combination = FeatureCombination::Difference;
            Command(ConstructDraftingPrimitive(Studded, Drill));
        }
        const std::string FrontEdge = PickAndSelect(Studded, RayBetween({ 4.0, -1.0, 1.6 }, { 4.0, 0.0, 0.6 }), false);
        SelectDraftingName(Studded, DraftingSelectionKind::Edge, FrontEdge, false);
        Command(BlendDraftingSelection(Studded, BlendProfile::Fillet, 0.15));
        const int Steps = static_cast<int>(Studded.Tree.Steps.size());
        Tally(ReplayGate, 0.0, IdenticalCount == Commands && AppendResumes == Commands && Steps == 27 && Studded.Regeneration.Refused == 0,
              std::to_string(Commands) + " commands → " + std::to_string(Steps) + " steps: " + std::to_string(IdenticalCount) + " identical to a full replay, " + std::to_string(AppendResumes) + " resumed at the new step");

        // Speed: full replay vs the incremental paths (each result identical to a full replay of the same tree).
        double Full = 1.0e300;
        for (int Run = 0; Run < 3; ++Run) Full = std::min(Full, Milliseconds([&]() { (void)FullReplay(Studded); }));
        auto Timed = [&](const std::string& Caption, auto&& Action, int ExpectedResume, double Least)
        {
            const double Time = Milliseconds(Action);
            const bool Same = Identical(Studded, FullReplay(Studded));
            const int Resumed = Studded.Regeneration.Resumed;
            Tally(ReplayGate, Full / Time, Same && Resumed == ExpectedResume && Full / Time >= Least,
                  Caption + ": resumed at " + std::to_string(Resumed) + "/" + std::to_string(Studded.Tree.Steps.size()) + " · " + Ms(Time) + " vs full " + Ms(Full) + " · " + Times(Full / Time) + " · identical");
        };
        int BindingStep = -1;
        for (int Index = 0; Index < Steps && BindingStep < 0; ++Index)
        {
            for (const SketchConstraint& Constraint : Studded.Tree.Steps[Index].Sketch.Constraints) if (Constraint.Parameter == "Cylinder6.Radius") BindingStep = Index;
        }
        Timed("tail edit Fillet1.Size 0.15 → 0.12", [&]() { Expect(AssignDraftingParameter(Studded, "Fillet1.Size", 0.12)); }, Steps - 1, 2.0);
        Timed("middle edit Cylinder6.Radius 0.30 → 0.32", [&]() { Expect(AssignDraftingParameter(Studded, "Cylinder6.Radius", 0.32)); }, BindingStep, 0.0);
        Timed("undo (Cylinder6.Radius back)", [&]() { (void)UndoDraftingCommand(Studded); }, Steps, 10.0);
        Timed("redo", [&]() { (void)RedoDraftingCommand(Studded); }, Steps, 10.0);
        Timed("undo again", [&]() { (void)UndoDraftingCommand(Studded); }, Steps, 10.0);
        {
            std::unique_ptr<DraftingDocument> Preview;
            const double Time = Milliseconds([&]() { Preview = std::make_unique<DraftingDocument>(CloneDraftingDocument(Studded)); });
            Tally(ReplayGate, Full / Time, Identical(*Preview, Studded) && Preview->Regeneration.Resumed == Steps && Full / Time >= 10.0,
                  "preview clone (bevel / move / sweep staging): resumed at " + std::to_string(Preview->Regeneration.Resumed) + "/" + std::to_string(Steps) + " · " + Ms(Time) + " · " + Times(Full / Time));
            DraftingBlendRequest Wider;
            Wider.Size = 0.2;
            const std::string BackEdge = PickAndSelect(*Preview, RayBetween({ 4.0, 4.0, 1.6 }, { 4.0, 3.0, 0.6 }), false);
            SelectDraftingName(*Preview, DraftingSelectionKind::Edge, BackEdge, false);
            const double Staged = Milliseconds([&]() { Expect(BlendDraftingSelection(*Preview, Wider)); });
            // Fillet1 was last replayed by the (non-capturing) parameter edit: this authoring command re-captures its edge
            // signature in place from the checkpoint before it, so only the new fillet replays (27).
            Tally(ReplayGate, 0.0, Identical(*Preview, FullReplay(*Preview)) && Preview->Regeneration.Resumed == Steps && Identical(Studded, FullReplay(Studded)),
                  "preview fillets the back edge " + BackEdge + ": resumed at " + std::to_string(Preview->Regeneration.Resumed) + " (Fillet1 re-captured in place) · " + Ms(Staged) + " · source untouched");
        }

        // History depth: 10 more edits; only the 8 newest undo entries keep checkpoints; undoing all 10 stays identical.
        for (int Edit = 0; Edit < 10; ++Edit) Expect(AssignDraftingParameter(Studded, "Fillet1.Size", 0.1 + 0.005 * Edit));
        int Kept = 0;
        for (const DraftingHistoryEntry& Entry : Studded.UndoEntries) Kept += Entry.Replay ? 1 : 0;
        int UndoIdentical = 0, UndoRestored = 0;
        for (int Edit = 0; Edit < 10; ++Edit)
        {
            (void)UndoDraftingCommand(Studded);
            UndoIdentical += Identical(Studded, FullReplay(Studded)) ? 1 : 0;
            UndoRestored  += Studded.Regeneration.Resumed == Steps ? 1 : 0;
        }
        Tally(ReplayGate, 0.0, Kept == 8 && Studded.UndoEntries.size() > 8 && UndoIdentical == 10 && UndoRestored >= 8,
              std::to_string(Studded.UndoEntries.size() + 10) + " undo entries, " + std::to_string(Kept) + " with checkpoints; 10 undos identical, " + std::to_string(UndoRestored) + " restored every step (older ones resume at the edited step)");

        // Vehicle bonnet: 11 SpaceCurves + loft; a pole move on one section resumes at that curve.
        DraftingDocument Bonnet = CreateDraftingDocument("Bonnet");
        Expect(ImportDraftingCurves(Bonnet, ReadVehicleCurves(DRAFTING_VEHICLE_SAMPLE)));
        std::vector<std::string> Sections;
        for (const FeatureStep& Step : Bonnet.Tree.Steps) if (Bonnet.Captions[Step.Label].rfind("Section", 0) == 0) Sections.push_back(Step.Label);
        Expect(LoftDraftingCurves(Bonnet, Sections));
        int SectionStep = -1;
        for (size_t Index = 0; Index < Bonnet.Tree.Steps.size(); ++Index) if (Bonnet.Tree.Steps[Index].Label == Sections.back()) SectionStep = static_cast<int>(Index);
        const Vector3 Pole = Bonnet.Regeneration.Curves.at(Sections.back()).Curve.Definition->Poles[2];
        double BonnetFull = 1.0e300;
        for (int Run = 0; Run < 3; ++Run) BonnetFull = std::min(BonnetFull, Milliseconds([&]() { (void)FullReplay(Bonnet); }));
        const double Moved = Milliseconds([&]() { Expect(MoveDraftingPole(Bonnet, Sections.back(), 2, Pole + Vector3{ 0.0, 0.0, 0.01 })); });
        const bool MovedSame = Identical(Bonnet, FullReplay(Bonnet));
        const int MovedResume = Bonnet.Regeneration.Resumed;
        const double Undone = Milliseconds([&]() { (void)UndoDraftingCommand(Bonnet); });
        Tally(ReplayGate, BonnetFull / Undone, MovedSame && MovedResume == SectionStep && Identical(Bonnet, FullReplay(Bonnet)) && Bonnet.Regeneration.Resumed == static_cast<int>(Bonnet.Tree.Steps.size()),
              "bonnet (" + std::to_string(Bonnet.Tree.Steps.size()) + " steps, full " + Ms(BonnetFull) + "): pole move resumed at " + std::to_string(MovedResume) + " · " + Ms(Moved) + "; undo restored all · " + Ms(Undone) + " · identical");
    }

    // ---- W32 x-ray: pick-through and box edges (closed form: the isometric eye sits at +x, −y, +z, so exactly the 3 edges
    //      meeting the far corner (0, 2, 0) are hidden)
    {
        DraftingDocument Ghost = CreateDraftingDocument("Ghost");
        DraftingConstructRequest Brick;
        Brick.Kind = DraftingConstructKind::Block; Brick.Width = 4.0; Brick.Depth = 2.0; Brick.Height = 1.0;
        Expect(ConstructDraftingPrimitive(Ghost, Brick));
        const DraftingScene Scene = BuildDraftingScene(Ghost);
        auto AtCorner = [](const Vector3& Point, double X, double Y, double Z)
        {
            return std::abs(Point.X - X) + std::abs(Point.Y - Y) + std::abs(Point.Z - Z) < 1.0e-9;
        };
        auto TouchesFarCorner = [&](const std::string& Name)
        {
            return AtCorner(EdgeEndPosition(Ghost, Name, false), 0.0, 2.0, 0.0) || AtCorner(EdgeEndPosition(Ghost, Name, true), 0.0, 2.0, 0.0);
        };
        auto LiesOn = [&](const std::string& Name, double Y, double Z)                       // Edge along x at (y, z)
        {
            const Vector3 Start = EdgeEndPosition(Ghost, Name, false);
            const Vector3 End   = EdgeEndPosition(Ghost, Name, true);
            return std::abs(Start.Y - Y) + std::abs(End.Y - Y) + std::abs(Start.Z - Z) + std::abs(End.Z - Z) < 1.0e-9 && std::abs(Start.X - End.X) > 1.0;
        };
        auto NameOf = [&](const std::optional<DraftingPick>& Hit) { return Hit ? NameDraftingIdentity(Scene, Hit->Identity) : std::string("(miss)"); };

        // Pick-through: from the front-top towards the back-bottom edge (2, 2, 0)
        const DraftingRay Behind = RayBetween({ 2.0, -3.0, 4.0 }, { 2.0, 2.0, 0.0 });
        const std::optional<DraftingPick> Opaque = PickDraftingScene(Scene, Behind, 0.02, false);
        const std::optional<DraftingPick> Through = PickDraftingScene(Scene, Behind, 0.02, true);
        const bool OpaqueFace = Opaque && (Opaque->Identity & IdentityEdgeBit) == 0;
        const bool ThroughEdge = Through && (Through->Identity & IdentityEdgeBit) != 0 && LiesOn(NameOf(Through), 2.0, 0.0);
        const double Farther = (Opaque && Through) ? Through->Distance - Opaque->Distance : 0.0;
        Tally(XRayGate, 0.0, OpaqueFace && ThroughEdge && Farther > 0.5,
              "ray at the hidden back-bottom edge: opaque → " + NameOf(Opaque) + " · x-ray → " + NameOf(Through) + " (" + std::to_string(Farther) + " farther)");

        // A visible edge (front-top, y = 0, z = 1) still picks in x-ray, and the same with x-ray off
        const DraftingRay Front = RayBetween({ 2.0, -3.0, 4.0 }, { 2.0, 0.0, 1.0 });
        const std::optional<DraftingPick> FrontOpaque  = PickDraftingScene(Scene, Front, 0.02, false);
        const std::optional<DraftingPick> FrontThrough = PickDraftingScene(Scene, Front, 0.02, true);
        Tally(XRayGate, 0.0, FrontOpaque && FrontThrough && FrontOpaque->Identity == FrontThrough->Identity && (FrontThrough->Identity & IdentityEdgeBit) != 0 && LiesOn(NameOf(FrontThrough), 0.0, 1.0),
              "ray at the visible front-top edge: opaque = x-ray → " + NameOf(FrontThrough));

        // Box edges over the whole viewport in the isometric view
        DraftingCamera Camera;
        Camera.Target = { 2.0, 1.0, 0.5 }; Camera.Distance = 10.0; Camera.SceneRadius = 2.5;
        const DraftingCameraFrame Frame = FrameDraftingCamera(Camera, 800.0, 600.0);
        const std::vector<uint32_t> Visible = EncloseDraftingEdges(Scene, Frame, { 0.0, 0.0 }, { 800.0, 600.0 }, false);
        const std::vector<uint32_t> All     = EncloseDraftingEdges(Scene, Frame, { 0.0, 0.0 }, { 800.0, 600.0 }, true);
        long OffCorner = 0;
        for (const DraftingSceneEdge& Edge : Scene.Edges) OffCorner += TouchesFarCorner(Edge.Name) ? 0 : 1;
        bool VisibleOffCorner = true;
        for (const uint32_t Index : Visible) VisibleOffCorner = VisibleOffCorner && !TouchesFarCorner(Scene.Edges[Index].Name);
        const bool Subset = std::includes(All.begin(), All.end(), Visible.begin(), Visible.end());
        Tally(XRayGate, 0.0, Scene.Edges.size() == 12 && OffCorner == 9 && Visible.size() == 9 && VisibleOffCorner,
              "full box without x-ray → " + std::to_string(Visible.size()) + " edges, none at the far corner (closed form 9 of " + std::to_string(Scene.Edges.size()) + ")");
        Tally(XRayGate, 0.0, All.size() == 12 && Subset, "full box with x-ray → " + std::to_string(All.size()) + " edges (the opaque 9 among them)");

        // An empty corner of the viewport (the block projects around the centre)
        const std::vector<uint32_t> NoneOpaque  = EncloseDraftingEdges(Scene, Frame, { 0.0, 0.0 }, { 40.0, 40.0 }, false);
        const std::vector<uint32_t> NoneThrough = EncloseDraftingEdges(Scene, Frame, { 0.0, 0.0 }, { 40.0, 40.0 }, true);
        Tally(XRayGate, 0.0, NoneOpaque.empty() && NoneThrough.empty(), "empty 40 × 40 px corner box → " + std::to_string(NoneOpaque.size()) + " / " + std::to_string(NoneThrough.size()) + " edges");
    }

    // ---- report
    const double Seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - Start).count();
    std::printf("# DraftingDocumentValidation\n\n");
    std::printf("Document layer of DraftingWorkspace over the SolidArcRemastered feature tree (%.2f s).\n\n", Seconds);
    std::printf("| Gate | Subject | Limit | Worst | Checks | Verdict |\n|------|---------|-------|-------|--------|---------|\n");
    long Failures = 0;
    for (const GateTally* Gate : { &ConstructGate, &PickGate, &BlendGate, &FaceGate, &ParameterGate, &AtomicGate, &HistoryGate, &FileGate, &VariableGate, &DrawingGate, &SurfacingGate, &PanelGate, &EvaluationGate, &BlueprintGate, &MatcapGate, &BodiesGate, &CutGate, &TrimGate, &MoveGate, &CopyGate, &HideGate, &CurveGate, &ConstrainGate, &ConflictGate, &DragGate, &ReferenceGate, &SplineGate, &LinkGate, &RevolveGate, &EvolutionGate, &ReplayGate, &XRayGate })
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
