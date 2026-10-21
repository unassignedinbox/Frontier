/*====================================================================================================================================
                                                     DRAFTINGINTERFACE.CPP
====================================================================================================================================*/
// 🧩 DraftingWorkspace ImGui shell: header, Outliner, viewport, Inspector, bevel session, shortcuts, autosave / recovery

//------------------------------------------------------------------------------------------------------------------------
//                                                    INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Interface/DraftingInterface.h"

#include "Document/DraftingDocumentCodec.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <numbers>
#include <sstream>

namespace Frontier::DraftingWorkspace
{

using namespace Modeller;

namespace
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    CONSTANTS
//------------------------------------------------------------------------------------------------------------------------

// 📝 SolidArc WorkspacePanel.css: header + document bar, Outliner 300 px | viewport | Inspector 330 px, 10 px gaps.
constexpr float PanelGap        = 10.0f;                                                     // [px] - Between panels and at the frame edge
constexpr float HeaderHeight    = 48.0f;                                                     // [px] - Title row
constexpr float DocumentHeight  = 38.0f;                                                     // [px] - Document tab row
constexpr float OutlinerWidth   = 300.0f;                                                    // [px] - Left column
constexpr float InspectorWidth  = 330.0f;                                                    // [px] - Right column
constexpr float ViewportBar     = 44.0f;                                                     // [px] - Viewport toolbar
constexpr float PanelRounding   = 16.0f;                                                     // [px] - Panel corner radius
constexpr float CubeExtent      = 92.0f;                                                     // [px] - Orientation cube box
constexpr float ClickTolerance  = 4.0f;                                                      // [px] - Press → release travel still counted as a click

constexpr ImU32 PanelColour     = IM_COL32(0x1a, 0x1a, 0x1a, 0xff);                          // #1a1a1a
constexpr ImU32 CardColour      = IM_COL32(0x22, 0x22, 0x22, 0xff);                          // #222
constexpr ImU32 RaisedColour    = IM_COL32(0x29, 0x29, 0x29, 0xff);                          // #292929
constexpr ImU32 SelectedColour  = IM_COL32(0x40, 0x99, 0xff, 0xff);                          // #4099ff
constexpr ImU32 ActiveColour    = IM_COL32(0x33, 0xff, 0x73, 0xff);                          // #33ff73
constexpr ImU32 PillColour      = IM_COL32(0x16, 0x1a, 0x21, 0xae);                          // #161a21ae
constexpr ImU32 MutedColour     = IM_COL32(0xaa, 0xaa, 0xaa, 0xff);                          // #aaa
constexpr ImU32 RefusalColour   = IM_COL32(0xff, 0x6b, 0x6b, 0xff);                          // Refused step / reason
constexpr ImU32 AxisColours[3]  = { IM_COL32(0xe0, 0x4b, 0x4b, 0xff), IM_COL32(0x4b, 0xc8, 0x5a, 0xff), IM_COL32(0x4b, 0x7c, 0xe8, 0xff) };

constexpr FaceOperationKind FaceVerbs[] = { FaceOperationKind::Push, FaceOperationKind::Offset, FaceOperationKind::Extrude, FaceOperationKind::Draft,
                                            FaceOperationKind::Extend, FaceOperationKind::Trim, FaceOperationKind::Scale, FaceOperationKind::Hollow,
                                            FaceOperationKind::Delete };

//------------------------------------------------------------------------------------------------------------------------
//                                                    HELPERS
//------------------------------------------------------------------------------------------------------------------------

std::string Compose(const char* Pattern, ...)
{
    char    Buffer[512];
    va_list Arguments;
    va_start(Arguments, Pattern);
    std::vsnprintf(Buffer, sizeof(Buffer), Pattern, Arguments);
    va_end(Arguments);
    return Buffer;
}

ImVec4 Tint(ImU32 Colour)
{
    return ImGui::ColorConvertU32ToFloat4(Colour);
}

// Records the last widget's rectangle under its ID text (after "###" when present) for scripted pointer input.
void RegisterWidget(DraftingWorkspaceState& State, const char* Label)
{
    const char* Marker = std::strstr(Label, "###");
    const std::string Key = Marker ? std::string(Marker + 3) : std::string(Label);
    const ImVec2 Low  = ImGui::GetItemRectMin();
    const ImVec2 High = ImGui::GetItemRectMax();
    State.Widgets[Key] = ImVec4(Low.x, Low.y, High.x, High.y);
}

bool PressButton(DraftingWorkspaceState& State, const char* Label, ImVec2 Size = ImVec2(0, 0), bool EnabledCondition = true)
{
    ImGui::BeginDisabled(!EnabledCondition);
    const bool Pressed = ImGui::Button(Label, Size);
    ImGui::EndDisabled();
    RegisterWidget(State, Label);
    return Pressed && EnabledCondition;
}

bool ToggleButton(DraftingWorkspaceState& State, const char* Label, bool OnCondition, ImVec2 Size = ImVec2(0, 0))
{
    ImGui::PushStyleColor(ImGuiCol_Button, OnCondition ? Tint(SelectedColour) : Tint(RaisedColour));
    ImGui::PushStyleColor(ImGuiCol_Text, OnCondition ? ImVec4(1, 1, 1, 1) : Tint(MutedColour));
    const bool Pressed = ImGui::Button(Label, Size);
    ImGui::PopStyleColor(2);
    RegisterWidget(State, Label);
    return Pressed;
}

// 📝 The UI font has no pictographs: four-byte UTF-8 sequences (kernel status emoji) and their trailing space are dropped.
std::string Legible(const std::string& Text)
{
    std::string Result;
    for (size_t Index = 0; Index < Text.size();)
    {
        const unsigned char Lead = static_cast<unsigned char>(Text[Index]);
        if ((Lead & 0xF8) == 0xF0)
        {
            Index += 4;
            while (Index < Text.size() && (static_cast<unsigned char>(Text[Index]) == 0xEF)) Index += 3;     // variation selectors
            if (Index < Text.size() && Text[Index] == ' ') Index += 1;
            continue;
        }
        Result.push_back(Text[Index++]);
    }
    return Result;
}

void Announce(DraftingWorkspaceState& State, const std::string& RawMessage)
{
    const std::string Message = Legible(RawMessage);
    State.Status = Message;
    State.Journal.push_back(Message);
    if (State.Journal.size() > 200) State.Journal.erase(State.Journal.begin());
}

void Conclude(DraftingWorkspaceState& State, const DraftingCommandOutcome& Outcome)
{
    if (Outcome.AcceptedCondition)
    {
        RefreshDraftingScene(State, true);
        Announce(State, Compose("%s · %.0f ms", Outcome.Label.c_str(), Outcome.Milliseconds));
    }
    else
    {
        Announce(State, "Refused: " + Outcome.Refusal);
    }
}

std::string FormatLength(double Value)
{
    return Compose("%.4g", Value);
}

void BeginCard(const char* Title)
{
    ImGui::PushStyleColor(ImGuiCol_ChildBg, Tint(CardColour));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 10.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 10));
    ImGui::BeginChild(Title, ImVec2(0, 0), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysUseWindowPadding);
    ImGui::PushStyleColor(ImGuiCol_Text, Tint(MutedColour));
    ImGui::TextUnformatted(Title);
    ImGui::PopStyleColor();
    ImGui::Spacing();
}

void EndCard()
{
    ImGui::EndChild();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();
    ImGui::Spacing();
}

void BeginPanel(const char* Name, ImVec2 Position, ImVec2 Size, ImVec2 Padding = ImVec2(14, 12))
{
    ImGui::SetNextWindowPos(Position);
    ImGui::SetNextWindowSize(Size);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, Padding);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, PanelRounding);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, Tint(PanelColour));
    ImGui::Begin(Name, nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                                ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus);
}

void EndPanel()
{
    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
}

void PanelHeading(const char* Title)
{
    ImGui::TextUnformatted(Title);
    ImGui::Separator();
    ImGui::Spacing();
}

const FeatureStepReport* FindReport(const DraftingDocument& Document, const std::string& Label)
{
    for (const FeatureStepReport& Report : Document.Regeneration.Steps)
        if (Report.Label == Label) return &Report;
    return nullptr;
}

const char* FaceAmountUnit(FaceOperationKind Verb)
{
    switch (Verb)
    {
    case FaceOperationKind::Draft:
    case FaceOperationKind::Rotate: return "deg";
    case FaceOperationKind::Scale:  return "factor";
    case FaceOperationKind::Delete: return "-";
    default:                        return "m";
    }
}

double FaceAmountToModel(FaceOperationKind Verb, double Amount)
{
    return (Verb == FaceOperationKind::Draft || Verb == FaceOperationKind::Rotate) ? Amount * std::numbers::pi / 180.0 : Amount;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    DOCUMENT LIFECYCLE
//------------------------------------------------------------------------------------------------------------------------

std::string PreviousRecoveryPath(const DraftingWorkspaceState& State)
{
    return DraftingPreviousRecoveryPath(State.Options);
}

bool WriteDocument(const DraftingDocument& Document, const std::string& Path)
{
    std::error_code Failure;
    const std::filesystem::path Folder = std::filesystem::path(Path).parent_path();
    if (!Folder.empty()) std::filesystem::create_directories(Folder, Failure);
    return SaveDraftingDocument(Document, Path);
}

void AdoptDocument(DraftingWorkspaceState& State, DraftingDocument Document, bool FitCondition)
{
    State.Bevel       = {};
    State.Document    = std::move(Document);
    State.FocusedStep = {};
    RefreshDraftingScene(State, false);
    if (FitCondition && !State.Scene.Faces.empty()) FitDraftingCamera(State.Camera, State.Scene);
}

bool OpenDocument(DraftingWorkspaceState& State, const std::string& Path, bool AdoptPathCondition)
{
    const DraftingDocumentFile File = LoadDraftingDocumentFile(Path);
    if (!File.Refusal.empty())
    {
        Announce(State, "Open refused: " + File.Refusal);
        return false;
    }
    DraftingDocument Document = CreateDraftingDocument(File.Title);
    Document.Tree     = File.Tree;
    Document.Captions = File.Captions;
    RegenerateDraftingDocument(Document, true);
    AdoptDocument(State, std::move(Document), true);
    if (AdoptPathCondition) State.DocumentPath = Path;
    Announce(State, Compose("Opened %s · %d steps", Path.c_str(), int(State.Document.Tree.Steps.size())));
    return true;
}

void SaveDocument(DraftingWorkspaceState& State, const std::string& Path)
{
    if (WriteDocument(State.Document, Path))
    {
        State.DocumentPath   = Path;
        State.DirtyCondition = false;
        Announce(State, "Saved " + Path);
    }
    else
    {
        Announce(State, "Save failed: " + Path);
    }
}

void StartNewDocument(DraftingWorkspaceState& State)
{
    std::error_code Failure;
    if (std::filesystem::exists(State.Options.RecoveryPath))
        std::filesystem::copy_file(State.Options.RecoveryPath, PreviousRecoveryPath(State), std::filesystem::copy_options::overwrite_existing, Failure);
    else if (!State.Document.Tree.Steps.empty())
        WriteDocument(State.Document, PreviousRecoveryPath(State));
    AdoptDocument(State, CreateDraftingDocument("Untitled"), false);
    State.DocumentPath = {};
    State.Camera       = DraftingCamera{ .FieldOfView = State.Options.FieldOfView, .OrthographicEnabled = State.Camera.OrthographicEnabled };
    Announce(State, "New document (previous recovery copy kept)");
}

void Undo(DraftingWorkspaceState& State)
{
    if (State.Bevel.ActiveCondition) return;
    if (UndoDraftingCommand(State.Document))
    {
        RefreshDraftingScene(State, true);
        Announce(State, "Undo");
    }
}

void Redo(DraftingWorkspaceState& State)
{
    if (State.Bevel.ActiveCondition) return;
    if (RedoDraftingCommand(State.Document))
    {
        RefreshDraftingScene(State, true);
        Announce(State, "Redo");
    }
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    BEVEL SESSION
//------------------------------------------------------------------------------------------------------------------------

void WriteBevelText(DraftingBevelSession& Bevel)
{
    std::snprintf(Bevel.SizeText.data(), Bevel.SizeText.size(), "%.4g", Bevel.Size);
    std::snprintf(Bevel.EndSizeText.data(), Bevel.EndSizeText.size(), "%.4g", Bevel.EndSize);
}

// The blend the session would apply (variable sizes and the flip only when V is on).
[[nodiscard]] DraftingBlendRequest ComposeBlendRequest(const DraftingBevelSession& Bevel)
{
    DraftingBlendRequest Request;
    Request.Shape = Bevel.Shape;
    Request.Size  = Bevel.Size;
    if (Bevel.VariableEnabled)
    {
        Request.EndSize           = Bevel.EndSize;
        Request.ReversedCondition = Bevel.ReversedCondition;
    }
    return Request;
}

// V: variable size on / off; the far size starts at half the current size so the taper is visible at once (a typed far
//    size is kept when V is pressed again).
void ToggleBevelVariable(DraftingWorkspaceState& State)
{
    DraftingBevelSession& Bevel = State.Bevel;
    if (!Bevel.ActiveCondition || Bevel.Verb != DraftingSessionVerb::Blend) return;
    Bevel.VariableEnabled = !Bevel.VariableEnabled;
    if (Bevel.VariableEnabled && !(Bevel.EndSize > 0.0)) Bevel.EndSize = std::round(0.5 * Bevel.Size * 1.0e4) * 1.0e-4;
    WriteBevelText(Bevel);
    Announce(State, Bevel.VariableEnabled ? Compose("Variable size: %s m at the anchor corner → %s m (F flips the anchor)", FormatLength(Bevel.Size).c_str(), FormatLength(Bevel.EndSize).c_str())
                                          : std::string("Constant size"));
}

// F: anchor each edge at its other end (the start size moves to the opposite corner).
void FlipBevelAnchor(DraftingWorkspaceState& State)
{
    DraftingBevelSession& Bevel = State.Bevel;
    if (!Bevel.ActiveCondition || Bevel.Verb != DraftingSessionVerb::Blend || !Bevel.VariableEnabled) return;
    Bevel.ReversedCondition = !Bevel.ReversedCondition;
    Announce(State, Bevel.ReversedCondition ? "Anchor flipped: the start size is at each edge's end corner" : "Anchor: the start size is at each edge's start corner");
}

void OpenBevelSession(DraftingWorkspaceState& State, BlendProfile Shape)
{
    if (State.Document.Selection.Edges.empty())
    {
        Announce(State, "Bevel needs selected edges (mode 2)");
        return;
    }
    const double Previous = State.Bevel.Verb == DraftingSessionVerb::Blend ? State.Bevel.Size : 0.0;
    State.Bevel                 = {};
    State.Bevel.ActiveCondition = true;
    State.Bevel.Verb            = DraftingSessionVerb::Blend;
    State.Bevel.Shape           = Shape;
    State.Bevel.Size            = Previous > 0.0 ? Previous : 0.25;
    WriteBevelText(State.Bevel);
    Announce(State, "Fillet (B): move horizontally · D fillet · C chamfer · V variable size · F flip its anchor · click edits the edges · Enter / right-click applies · Esc aborts");
}

// Plasticity E: extrude the selected faces; the pointer sets the signed distance like the fillet size.
void OpenExtrudeSession(DraftingWorkspaceState& State)
{
    if (State.Document.Selection.Faces.empty())
    {
        Announce(State, "Extrude needs selected faces (mode 3)");
        return;
    }
    const double Remembered = State.LastCommand.Kind == DraftingRepeatKind::Face && State.LastCommand.Operation == FaceOperationKind::Extrude ? State.LastCommand.Amount : 0.5;
    State.Bevel                 = {};
    State.Bevel.ActiveCondition = true;
    State.Bevel.Verb            = DraftingSessionVerb::Extrude;
    State.Bevel.Size            = Remembered;
    WriteBevelText(State.Bevel);
    Announce(State, "Extrude (E): move horizontally · click edits the faces · Enter / right-click applies · Esc aborts");
}

void StageBevelPreview(DraftingWorkspaceState& State)
{
    DraftingBevelSession& Bevel = State.Bevel;
    if (!Bevel.ActiveCondition) return;
    if (Bevel.StagedSize == Bevel.Size && Bevel.StagedShape == Bevel.Shape && Bevel.PreviewVerb == Bevel.Verb &&
        Bevel.StagedVariable == Bevel.VariableEnabled && Bevel.StagedEndSize == Bevel.EndSize && Bevel.StagedReversed == Bevel.ReversedCondition) return;
    Bevel.StagedSize     = Bevel.Size;
    Bevel.StagedShape    = Bevel.Shape;
    Bevel.PreviewVerb    = Bevel.Verb;
    Bevel.StagedVariable = Bevel.VariableEnabled;
    Bevel.StagedEndSize  = Bevel.EndSize;
    Bevel.StagedReversed = Bevel.ReversedCondition;
    Bevel.Preview     = std::make_unique<DraftingDocument>(CloneDraftingDocument(State.Document));
    const DraftingCommandOutcome Outcome = Bevel.Verb == DraftingSessionVerb::Extrude ? ChangeDraftingSelection(*Bevel.Preview, FaceOperationKind::Extrude, Bevel.Size)
                                                                                      : BlendDraftingSelection(*Bevel.Preview, ComposeBlendRequest(Bevel));
    if (Outcome.AcceptedCondition)
    {
        Bevel.PreviewScene = BuildDraftingScene(*Bevel.Preview);
        Bevel.Refusal      = Legible(Bevel.PreviewScene.Refusal);
    }
    else
    {
        Bevel.Refusal = Legible(Outcome.Refusal);
    }
}

void ApplyBevelSession(DraftingWorkspaceState& State)
{
    if (!State.Bevel.ActiveCondition || !State.Bevel.Refusal.empty()) return;
    const BlendProfile         Shape   = State.Bevel.Shape;
    const double               Size    = State.Bevel.Size;
    const DraftingBlendRequest Request = ComposeBlendRequest(State.Bevel);
    State.Bevel.ActiveCondition = false;
    State.Bevel.Preview.reset();
    if (State.Bevel.Verb == DraftingSessionVerb::Extrude)
    {
        const DraftingCommandOutcome Outcome = ChangeDraftingSelection(State.Document, FaceOperationKind::Extrude, Size);
        if (Outcome.AcceptedCondition) State.LastCommand = { DraftingRepeatKind::Face, Shape, 0.0, FaceOperationKind::Extrude, Size, {} };
        Conclude(State, Outcome);
        return;
    }
    const DraftingCommandOutcome Outcome = BlendDraftingSelection(State.Document, Request);
    if (Outcome.AcceptedCondition) State.LastCommand = { DraftingRepeatKind::Blend, Shape, Size, FaceOperationKind::Extrude, 0.0, {}, Request };
    Conclude(State, Outcome);
}

void CancelBevelSession(DraftingWorkspaceState& State)
{
    if (!State.Bevel.ActiveCondition) return;
    State.Bevel.ActiveCondition = false;
    State.Bevel.Preview.reset();
    Announce(State, "Bevel cancelled (document untouched)");
}

// Positive signed size = fillet radius, negative = chamfer setback (SolidArc pointer convention).
void AdjustBevelSize(DraftingWorkspaceState& State, double Delta)
{
    DraftingBevelSession& Bevel = State.Bevel;
    if (Bevel.Verb == DraftingSessionVerb::Extrude)
    {
        double Distance = Bevel.Size + Delta;                                                // [len] - Signed extrusion
        if (std::abs(Distance) < 1.0e-3) Distance = Distance < 0.0 ? -1.0e-3 : 1.0e-3;
        Bevel.Size = std::round(Distance * 1.0e4) * 1.0e-4;
        WriteBevelText(Bevel);
        return;
    }
    double Signed = (Bevel.Shape == BlendProfile::Fillet ? Bevel.Size : -Bevel.Size) + Delta;
    if (std::abs(Signed) < 1.0e-3) Signed = Signed < 0.0 ? -1.0e-3 : 1.0e-3;
    Bevel.Shape = Signed > 0.0 ? BlendProfile::Fillet : BlendProfile::Chamfer;
    Bevel.Size  = std::round(std::abs(Signed) * 1.0e4) * 1.0e-4;
    WriteBevelText(Bevel);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    VIEWPORT PICKING
//------------------------------------------------------------------------------------------------------------------------

// Named views also choose the drawing plane (and its grid); Isometric / free orbit keep the active one.
void PresentView(DraftingWorkspaceState& State, DraftingViewPreset Preset)
{
    PresentDraftingView(State.Camera, Preset);
    switch (Preset)
    {
    case DraftingViewPreset::Top:
    case DraftingViewPreset::Bottom: State.Style.GridPlane = 0; break;
    case DraftingViewPreset::Front:
    case DraftingViewPreset::Back:   State.Style.GridPlane = 1; break;
    case DraftingViewPreset::Right:
    case DraftingViewPreset::Left:   State.Style.GridPlane = 2; break;
    case DraftingViewPreset::Isometric: break;
    }
}

DraftingCameraFrame ViewportFrame(const DraftingWorkspaceState& State)
{
    return FrameDraftingCamera(State.Camera, std::max(1.0f, State.ViewportRect[2]), std::max(1.0f, State.ViewportRect[3]));
}

// 💡 The GPU identity under the pointer is primary (identity target readback); modes 2 / 3 retarget it with the
//    exact CPU ray pick so an edge stroke never blocks a face click and a face never blocks a nearby edge.
void PickAtPointer(DraftingWorkspaceState& State, ImVec2 Local, DraftingPickIntent Intent)
{
    const DraftingCameraFrame Frame = ViewportFrame(State);
    const DraftingRay         Ray   = CastDraftingRay(Frame, Local.x, Local.y);
    uint32_t                  Identity = State.HoverIdentity;
    if (State.PickMode == DraftingPickMode::Edge && (Identity == 0 || !(Identity & IdentityEdgeBit)))
    {
        const std::optional<DraftingPick> Nearby = PickDraftingScene(State.Scene, Ray, 8.0 * Frame.WorldPerPixel);
        Identity = Nearby && (Nearby->Identity & IdentityEdgeBit) ? Nearby->Identity : 0;
    }
    if (State.PickMode == DraftingPickMode::Face && (Identity & IdentityEdgeBit))
    {
        const std::optional<DraftingPick> Behind = PickDraftingScene(State.Scene, Ray, 0.0);
        Identity = Behind ? Behind->Identity : 0;
    }
    if (Identity == 0)
    {
        if (Intent == DraftingPickIntent::Replace && (!State.Document.Selection.Faces.empty() || !State.Document.Selection.Edges.empty()))
        {
            ClearDraftingSelection(State.Document);
            Announce(State, "Selection cleared");
        }
        return;
    }
    if (State.PickMode == DraftingPickMode::Solid)
    {
        if (Intent == DraftingPickIntent::Remove)
        {
            ClearDraftingSelection(State.Document);
            Announce(State, "Body deselected");
            return;
        }
        ClearDraftingSelection(State.Document);
        for (const DraftingSceneFace& Face : State.Scene.Faces) SelectDraftingName(State.Document, DraftingSelectionKind::Face, Face.Name, true);
        Announce(State, Compose("Selected the body (%d faces)", int(State.Scene.Faces.size())));
        return;
    }
    const std::string           Name = NameDraftingIdentity(State.Scene, Identity);
    const DraftingSelectionKind Kind = DraftingIdentityKind(Identity);
    const auto Member = [&](const std::string& Candidate)
    {
        const std::vector<std::string>& Names = Kind == DraftingSelectionKind::Edge ? State.Document.Selection.Edges : State.Document.Selection.Faces;
        return std::find(Names.begin(), Names.end(), Candidate) != Names.end();
    };
    switch (Intent)
    {
    case DraftingPickIntent::Replace:
        SelectDraftingName(State.Document, Kind, Name, false);
        Announce(State, "Selected " + Name);
        break;
    case DraftingPickIntent::Add:
        if (!Member(Name)) SelectDraftingName(State.Document, Kind, Name, true);
        Announce(State, "Added " + Name);
        break;
    case DraftingPickIntent::Remove:
        if (Member(Name)) SelectDraftingName(State.Document, Kind, Name, true);
        Announce(State, "Removed " + Name);
        break;
    case DraftingPickIntent::Extend:
    {
        const std::vector<std::string> Chain = Kind == DraftingSelectionKind::Edge ? DraftingTangentChain(State.Document, Name) : std::vector<std::string>{ Name };
        for (const std::string& Link : Chain)
        {
            if (!Member(Link)) SelectDraftingName(State.Document, Kind, Link, true);
        }
        Announce(State, Compose("Extended along the tangent chain: %d %s", int(Chain.size()), Kind == DraftingSelectionKind::Edge ? "edge(s)" : "face(s)"));
        break;
    }
    }
}

DraftingPickIntent PickIntentOf(const ImGuiIO& Io)
{
    if (Io.KeyAlt) return DraftingPickIntent::Extend;
    if (Io.KeyCtrl) return DraftingPickIntent::Remove;
    if (Io.KeyShift) return DraftingPickIntent::Add;
    return DraftingPickIntent::Replace;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    HEADER + DOCUMENT BAR
//------------------------------------------------------------------------------------------------------------------------

void DrawHeader(DraftingWorkspaceState& State, const ImVec2 Frame)
{
    BeginPanel("Header", ImVec2(PanelGap, PanelGap), ImVec2(Frame.x - 2 * PanelGap, HeaderHeight), ImVec2(16, 10));
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("DraftingWorkspace");
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, Tint(MutedColour));
    ImGui::TextUnformatted("SolidArcRemastered · Vulkan · Slang");
    ImGui::PopStyleColor();
    ImGui::SameLine(0, 28);
    if (PressButton(State, "New###HeaderNew")) StartNewDocument(State);
    ImGui::SameLine();
    if (PressButton(State, "Open…###HeaderOpen")) ImGui::OpenPopup("Open document");
    ImGui::SameLine();
    if (PressButton(State, "Save###HeaderSave"))
    {
        if (State.DocumentPath.empty()) ImGui::OpenPopup("Save document");
        else SaveDocument(State, State.DocumentPath);
    }
    ImGui::SameLine();
    if (PressButton(State, "Save as…###HeaderSaveAs")) ImGui::OpenPopup("Save document");
    if (!State.PopupRequest.empty())
    {
        ImGui::OpenPopup(State.PopupRequest.c_str());                                        // Ctrl+O / Ctrl+Shift+S (same ID stack as the buttons)
        State.PopupRequest.clear();
    }
    ImGui::SameLine(0, 18);
    if (PressButton(State, "Undo###HeaderUndo", ImVec2(0, 0), !State.Document.UndoEntries.empty() && !State.Bevel.ActiveCondition)) Undo(State);
    ImGui::SameLine();
    if (PressButton(State, "Redo###HeaderRedo", ImVec2(0, 0), !State.Document.RedoEntries.empty() && !State.Bevel.ActiveCondition)) Redo(State);

    const std::string Device = Compose("%s · %.1f ms", State.DeviceLabel.c_str(), State.FrameMs);
    ImGui::SameLine(std::max(ImGui::GetCursorPosX() + 20, ImGui::GetWindowWidth() - ImGui::CalcTextSize(Device.c_str()).x - 18));
    ImGui::PushStyleColor(ImGuiCol_Text, Tint(MutedColour));
    ImGui::TextUnformatted(Device.c_str());
    ImGui::PopStyleColor();

    for (const char* Title : { "Open document", "Save document" })
    {
        const bool OpenCondition = std::strcmp(Title, "Open document") == 0;
        if (ImGui::IsPopupOpen(Title) && State.PathText[0] == 0)
            std::snprintf(State.PathText.data(), State.PathText.size(), "%s", (State.DocumentPath.empty() ? State.Options.DocumentPath : State.DocumentPath).c_str());
        if (ImGui::BeginPopupModal(Title, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::TextUnformatted(OpenCondition ? "Open a .drafting.toml document" : "Save the document as .drafting.toml");
            ImGui::SetNextItemWidth(420);
            const bool Entered = ImGui::InputText("##Path", State.PathText.data(), State.PathText.size(), ImGuiInputTextFlags_EnterReturnsTrue);
            RegisterWidget(State, "PathField");
            if (PressButton(State, OpenCondition ? "Open###PathOpen" : "Save###PathSave", ImVec2(120, 0)) || Entered)
            {
                if (OpenCondition) OpenDocument(State, State.PathText.data(), true);
                else SaveDocument(State, State.PathText.data());
                State.PathText[0] = 0;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (PressButton(State, "Cancel###PathCancel", ImVec2(120, 0)))
            {
                State.PathText[0] = 0;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }
    EndPanel();
}

void DrawDocumentBar(DraftingWorkspaceState& State, const ImVec2 Frame)
{
    const float Top = PanelGap + HeaderHeight + PanelGap;
    BeginPanel("Documents", ImVec2(PanelGap, Top), ImVec2(Frame.x - 2 * PanelGap, DocumentHeight), ImVec2(12, 6));
    ImDrawList* Canvas = ImGui::GetWindowDrawList();
    const std::string Tab = State.Document.Title + (State.DocumentPath.empty() ? "" : "  ·  " + State.DocumentPath);
    const ImVec2 Origin = ImGui::GetCursorScreenPos();
    const ImVec2 Extent(ImGui::CalcTextSize(Tab.c_str()).x + 44, DocumentHeight - 12);
    Canvas->AddRectFilled(Origin, ImVec2(Origin.x + Extent.x, Origin.y + Extent.y), RaisedColour, 9.0f);
    Canvas->AddRect(Origin, ImVec2(Origin.x + Extent.x, Origin.y + Extent.y), SelectedColour, 9.0f);
    Canvas->AddCircleFilled(ImVec2(Origin.x + 14, Origin.y + Extent.y * 0.5f), 4.0f, State.DirtyCondition ? ActiveColour : MutedColour);
    Canvas->AddText(ImVec2(Origin.x + 26, Origin.y + (Extent.y - ImGui::GetFontSize()) * 0.5f), IM_COL32_WHITE, Tab.c_str());
    ImGui::Dummy(Extent);
    ImGui::SameLine(0, 18);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (Extent.y - ImGui::GetFontSize()) * 0.5f);
    ImGui::PushStyleColor(ImGuiCol_Text, Tint(MutedColour));
    ImGui::TextUnformatted(State.Status.c_str());
    ImGui::PopStyleColor();
    EndPanel();
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    OUTLINER
//------------------------------------------------------------------------------------------------------------------------

void DrawStatTile(const char* Label, const std::string& Value, float Width)
{
    ImDrawList*  Canvas = ImGui::GetWindowDrawList();
    const ImVec2 Origin = ImGui::GetCursorScreenPos();
    const ImVec2 Extent(Width, 50);
    Canvas->AddRectFilled(Origin, ImVec2(Origin.x + Extent.x, Origin.y + Extent.y), CardColour, 10.0f);
    Canvas->AddText(ImVec2(Origin.x + 10, Origin.y + 6), MutedColour, Label);
    Canvas->AddText(ImVec2(Origin.x + 10, Origin.y + 25), IM_COL32_WHITE, Value.c_str());
    ImGui::Dummy(Extent);
}

void DrawConstructDialog(DraftingWorkspaceState& State)
{
    if (State.ConstructOpen)
    {
        ImGui::OpenPopup("Construct");
        State.ConstructOpen = false;
    }
    State.ConstructShown = false;
    if (!ImGui::BeginPopupModal("Construct", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) return;
    State.ConstructShown = true;
    DraftingConstructRequest& Form = State.ConstructForm;
    ImGui::Text("%s", DraftingConstructLabel(Form.Kind));
    ImGui::Separator();
    const bool FirstBody = !State.Document.Summary.PresentCondition;
    if (FirstBody)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, Tint(MutedColour));
        ImGui::TextUnformatted("First body: combination is New");
        ImGui::PopStyleColor();
    }
    else
    {
        const std::pair<FeatureCombination, const char*> Combinations[] = { { FeatureCombination::Union, "Union###CombinationUnion" },
                                                                            { FeatureCombination::Difference, "Difference###CombinationDifference" },
                                                                            { FeatureCombination::Intersection, "Intersection###CombinationIntersection" } };
        for (const auto& [Combination, Label] : Combinations)
        {
            if (ToggleButton(State, Label, Form.Combination == Combination)) Form.Combination = Combination;
            ImGui::SameLine(0, 4);
        }
        ImGui::NewLine();
    }
    auto Field = [&](const char* Label, double& Value)
    {
        ImGui::SetNextItemWidth(160);
        ImGui::InputDouble(Label, &Value, 0.0, 0.0, "%.3f");
        RegisterWidget(State, Label);
    };
    Field("X [m]###ConstructX", Form.Position.X);
    Field("Y [m]###ConstructY", Form.Position.Y);
    Field("Z [m]###ConstructZ", Form.Position.Z);
    switch (Form.Kind)
    {
    case DraftingConstructKind::Block:
        Field("Width [m]###ConstructWidth", Form.Width);
        Field("Depth [m]###ConstructDepth", Form.Depth);
        Field("Height [m]###ConstructHeight", Form.Height);
        break;
    case DraftingConstructKind::Cylinder:
        Field("Radius [m]###ConstructRadius", Form.Radius);
        Field("Height [m]###ConstructHeight", Form.Height);
        break;
    case DraftingConstructKind::Sphere:
        Field("Radius [m]###ConstructRadius", Form.Radius);
        break;
    case DraftingConstructKind::Torus:
        Field("Major [m]###ConstructMajor", Form.Major);
        Field("Minor [m]###ConstructMinor", Form.Minor);
        break;
    }
    // Plasticity box / cylinder / sphere command keys: Q union · W difference · E intersect · R new body; Enter creates.
    bool CreateCondition = false;
    if (!ImGui::GetIO().WantTextInput)
    {
        if (!FirstBody && ImGui::IsKeyPressed(ImGuiKey_Q, false)) Form.Combination = FeatureCombination::Union;
        if (!FirstBody && ImGui::IsKeyPressed(ImGuiKey_W, false)) Form.Combination = FeatureCombination::Difference;
        if (!FirstBody && ImGui::IsKeyPressed(ImGuiKey_E, false)) Form.Combination = FeatureCombination::Intersection;
        if (ImGui::IsKeyPressed(ImGuiKey_R, false)) Announce(State, "New body (R): documents hold one body; use Q / W / E");
        CreateCondition = ImGui::IsKeyPressed(ImGuiKey_Enter, false) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, false);
        if (ImGui::IsKeyPressed(ImGuiKey_Escape, false))
        {
            ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
            return;
        }
    }
    ImGui::PushStyleColor(ImGuiCol_Text, Tint(MutedColour));
    ImGui::TextUnformatted("Q union · W difference · E intersect · Enter create · Esc cancel");
    ImGui::PopStyleColor();
    ImGui::Spacing();
    if (PressButton(State, "Create###ConstructCreate", ImVec2(130, 0)) || CreateCondition)
    {
        DraftingConstructRequest Request = Form;
        if (FirstBody) Request.Combination = FeatureCombination::New;
        const bool FitCondition = FirstBody;
        State.LastCommand = { DraftingRepeatKind::Construct, BlendProfile::Fillet, 0.0, FaceOperationKind::Extrude, 0.0, Request };
        Conclude(State, ConstructDraftingPrimitive(State.Document, Request));
        if (FitCondition && !State.Scene.Faces.empty()) FitDraftingCamera(State.Camera, State.Scene);
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (PressButton(State, "Cancel###ConstructCancel", ImVec2(130, 0))) ImGui::CloseCurrentPopup();
    ImGui::EndPopup();
}

void DrawOutliner(DraftingWorkspaceState& State, ImVec2 Position, ImVec2 Size)
{
    BeginPanel("Outliner", Position, Size);
    PanelHeading("Outliner");
    const DraftingBodySummary& Summary = State.Document.Summary;
    const float Tile = (ImGui::GetContentRegionAvail().x - 8) * 0.5f;
    DrawStatTile("Faces", std::to_string(Summary.Faces), Tile);
    ImGui::SameLine(0, 8);
    DrawStatTile("Edges", std::to_string(Summary.Edges), Tile);
    DrawStatTile("Steps", Compose("%d  (%d refused)", int(State.Document.Tree.Steps.size()), State.Document.Regeneration.Refused), Tile);
    ImGui::SameLine(0, 8);
    DrawStatTile("Volume", Summary.PresentCondition ? Compose("%.4f m³", Summary.Volume) : std::string("-"), Tile);
    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_Text, Tint(MutedColour));
    ImGui::TextUnformatted("Feature tree");
    ImGui::PopStyleColor();
    const float TreeHeight = std::max(120.0f, ImGui::GetContentRegionAvail().y - 190.0f);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, Tint(CardColour));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 10.0f);
    ImGui::BeginChild("FeatureTree", ImVec2(0, TreeHeight), ImGuiChildFlags_AlwaysUseWindowPadding);
    if (State.Document.Tree.Steps.empty())
    {
        ImGui::PushStyleColor(ImGuiCol_Text, Tint(MutedColour));
        ImGui::TextWrapped("Empty document. Construct a body below, or Open a .drafting.toml.");
        ImGui::PopStyleColor();
    }
    for (const FeatureStep& Step : State.Document.Tree.Steps)
    {
        const FeatureStepReport* Report  = FindReport(State.Document, Step.Label);
        const bool               Refused = Report && Report->Status == FeatureStepStatus::Refused;
        const bool               Focused = State.FocusedStep == Step.Label;
        ImGui::PushStyleColor(ImGuiCol_Header, Tint(SelectedColour));
        if (Refused) ImGui::PushStyleColor(ImGuiCol_Text, Tint(RefusalColour));
        const std::string Row = DescribeDraftingStep(State.Document, Step) + (Refused ? "  · refused" : "") + "###Step:" + Step.Label;
        if (ImGui::Selectable(Row.c_str(), Focused)) State.FocusedStep = Focused ? std::string() : Step.Label;
        RegisterWidget(State, Row.c_str());
        if (Refused && ImGui::IsItemHovered()) ImGui::SetTooltip("%s", Legible(Report->Refusal).c_str());
        if (Refused) ImGui::PopStyleColor();
        ImGui::PopStyleColor();
    }
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_Text, Tint(MutedColour));
    ImGui::TextUnformatted("Construct");
    ImGui::PopStyleColor();
    const float ConstructTile = (ImGui::GetContentRegionAvail().x - 8) * 0.5f;
    const DraftingConstructKind Kinds[] = { DraftingConstructKind::Block, DraftingConstructKind::Cylinder, DraftingConstructKind::Sphere, DraftingConstructKind::Torus };
    for (int Index = 0; Index < 4; ++Index)
    {
        if (Index % 2 == 1) ImGui::SameLine(0, 8);
        const std::string Label = std::string(DraftingConstructLabel(Kinds[Index])) + "###Construct" + DraftingConstructLabel(Kinds[Index]);
        if (PressButton(State, Label.c_str(), ImVec2(ConstructTile, 40)))
        {
            State.ConstructForm.Kind        = Kinds[Index];
            State.ConstructForm.Combination = State.Document.Summary.PresentCondition ? FeatureCombination::Union : FeatureCombination::New;
            State.ConstructOpen             = true;
        }
    }
    if (std::filesystem::exists(PreviousRecoveryPath(State)))
    {
        if (PressButton(State, "Restore previous document###RestorePrevious", ImVec2(-1, 0))) OpenDocument(State, PreviousRecoveryPath(State), false);
    }
    DrawConstructDialog(State);
    EndPanel();
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    VIEWPORT
//------------------------------------------------------------------------------------------------------------------------

struct CubeFace
{
    std::array<double, 3> Normal = {};                                                       // [-] - Outward world normal
    DraftingViewPreset    Preset = DraftingViewPreset::Top;                                  // [-] - View it presents
    const char*           Label  = "";                                                       // [-] - Face caption
};

// 💡 Orientation cube: the six faces projected with the camera basis, back faces culled, painter-sorted; a click
//    presents that view. Returns true when the pointer is over the cube (viewport gestures are then ignored).
bool DrawOrientationCube(DraftingWorkspaceState& State, ImDrawList* Canvas, ImVec2 Corner, bool ClickCondition)
{
    static const CubeFace Faces[] = {
        { { 0, 0, 1 }, DraftingViewPreset::Top, "Top" },     { { 0, 0, -1 }, DraftingViewPreset::Bottom, "Bottom" },
        { { 0, -1, 0 }, DraftingViewPreset::Front, "Front" }, { { 0, 1, 0 }, DraftingViewPreset::Back, "Back" },
        { { 1, 0, 0 }, DraftingViewPreset::Right, "Right" },  { { -1, 0, 0 }, DraftingViewPreset::Left, "Left" },
    };
    const DraftingCameraFrame Frame  = ViewportFrame(State);
    const ImVec2              Centre(Corner.x + CubeExtent * 0.5f, Corner.y + CubeExtent * 0.5f);
    const float               Scale  = CubeExtent * 0.29f;
    auto Project = [&](const std::array<double, 3>& Point)
    {
        const double Across = Point[0] * Frame.Right[0] + Point[1] * Frame.Right[1] + Point[2] * Frame.Right[2];
        const double Upward = Point[0] * Frame.Up[0] + Point[1] * Frame.Up[1] + Point[2] * Frame.Up[2];
        return ImVec2(Centre.x + float(Across) * Scale, Centre.y - float(Upward) * Scale);
    };
    struct Visible
    {
        const CubeFace* Face  = nullptr;
        double          Depth = 0.0;
        ImVec2          Corners[4];
    };
    std::vector<Visible> Shown;
    for (const CubeFace& Face : Faces)
    {
        const double Facing = -(Face.Normal[0] * Frame.Forward[0] + Face.Normal[1] * Frame.Forward[1] + Face.Normal[2] * Frame.Forward[2]);
        if (Facing <= 1.0e-3) continue;
        int Axis = 0;
        while (Face.Normal[Axis] == 0) ++Axis;
        const int FirstAxis = (Axis + 1) % 3, SecondAxis = (Axis + 2) % 3;
        Visible Entry{ &Face, Facing, {} };
        const double Signs[4][2] = { { -1, -1 }, { 1, -1 }, { 1, 1 }, { -1, 1 } };
        for (int Corner = 0; Corner < 4; ++Corner)
        {
            std::array<double, 3> Point = Face.Normal;
            Point[FirstAxis]  = Signs[Corner][0];
            Point[SecondAxis] = Signs[Corner][1];
            Entry.Corners[Corner] = Project(Point);
        }
        Shown.push_back(Entry);
    }
    std::sort(Shown.begin(), Shown.end(), [](const Visible& Left, const Visible& Right) { return Left.Depth < Right.Depth; });

    const ImVec2 Mouse    = ImGui::GetIO().MousePos;
    const bool   Inside   = Mouse.x >= Corner.x && Mouse.x <= Corner.x + CubeExtent && Mouse.y >= Corner.y && Mouse.y <= Corner.y + CubeExtent;
    const CubeFace* Hovered = nullptr;
    for (const Visible& Entry : Shown)
    {
        bool Contained = Inside;
        for (int Corner = 0; Corner < 4 && Contained; ++Corner)
        {
            const ImVec2 From = Entry.Corners[Corner], To = Entry.Corners[(Corner + 1) % 4];
            const float  Cross = (To.x - From.x) * (Mouse.y - From.y) - (To.y - From.y) * (Mouse.x - From.x);
            Contained = Cross <= 0.0f;
        }
        if (!Contained)
        {
            bool Reversed = Inside;
            for (int Corner = 0; Corner < 4 && Reversed; ++Corner)
            {
                const ImVec2 From = Entry.Corners[Corner], To = Entry.Corners[(Corner + 1) % 4];
                Reversed = (To.x - From.x) * (Mouse.y - From.y) - (To.y - From.y) * (Mouse.x - From.x) >= 0.0f;
            }
            Contained = Reversed;
        }
        if (Contained) Hovered = Entry.Face;
    }
    Canvas->AddRectFilled(Corner, ImVec2(Corner.x + CubeExtent, Corner.y + CubeExtent), PillColour, 12.0f);
    for (const Visible& Entry : Shown)
    {
        const bool  Lit    = Entry.Face == Hovered;
        const float Shade  = 0.35f + 0.45f * float(Entry.Depth);
        const ImU32 Fill   = Lit ? SelectedColour : ImGui::ColorConvertFloat4ToU32(ImVec4(Shade * 0.42f, Shade * 0.46f, Shade * 0.54f, 1.0f));
        Canvas->AddConvexPolyFilled(Entry.Corners, 4, Fill);
        Canvas->AddPolyline(Entry.Corners, 4, IM_COL32(0x10, 0x12, 0x18, 0xff), 1.2f, ImDrawFlags_Closed);
        if (Entry.Depth > 0.3)
        {
            ImVec2 Middle(0, 0);
            for (const ImVec2& Point : Entry.Corners) Middle = ImVec2(Middle.x + Point.x * 0.25f, Middle.y + Point.y * 0.25f);
            const ImVec2 Text = ImGui::CalcTextSize(Entry.Face->Label);
            Canvas->AddText(ImGui::GetFont(), ImGui::GetFontSize() * 0.8f, ImVec2(Middle.x - Text.x * 0.4f, Middle.y - Text.y * 0.4f), IM_COL32_WHITE, Entry.Face->Label);
        }
    }
    for (int Axis = 0; Axis < 3; ++Axis)
    {
        std::array<double, 3> Tip{ -1.25, -1.25, -1.25 };
        std::array<double, 3> Root{ -1.25, -1.25, -1.25 };
        Tip[Axis] += 0.9;
        Canvas->AddLine(Project(Root), Project(Tip), AxisColours[Axis], 2.0f);
    }
    for (const CubeFace& Face : Faces)
    {
        const std::string Key = std::string("Cube") + Face.Label;
        const ImVec2      Point = Project({ Face.Normal[0] * 1.0, Face.Normal[1] * 1.0, Face.Normal[2] * 1.0 });
        State.Widgets[Key] = ImVec4(Point.x - 2, Point.y - 2, Point.x + 2, Point.y + 2);
    }
    if (Hovered && ClickCondition)
    {
        PresentView(State, Hovered->Preset);
        Announce(State, std::string("View ") + Hovered->Label + " (orientation cube)");
    }
    return Inside;
}

void DrawStatusPill(ImDrawList* Canvas, ImVec2& Cursor, const std::string& Text, ImU32 Accent)
{
    const ImVec2 Extent(ImGui::CalcTextSize(Text.c_str()).x + 26, 26);
    Canvas->AddRectFilled(Cursor, ImVec2(Cursor.x + Extent.x, Cursor.y + Extent.y), PillColour, 13.0f);
    Canvas->AddCircleFilled(ImVec2(Cursor.x + 11, Cursor.y + 13), 3.5f, Accent);
    Canvas->AddText(ImVec2(Cursor.x + 19, Cursor.y + (Extent.y - ImGui::GetFontSize()) * 0.5f), IM_COL32(0xdd, 0xdd, 0xdd, 0xff), Text.c_str());
    Cursor.x += Extent.x + 6;
}

void DrawViewportBar(DraftingWorkspaceState& State)
{
    ImGui::SetCursorPos(ImVec2(12, 8));
    for (DraftingViewPreset Preset : { DraftingViewPreset::Isometric, DraftingViewPreset::Top, DraftingViewPreset::Front, DraftingViewPreset::Right })
    {
        const std::string Label = std::string(DraftingViewLabel(Preset)) + "###View" + DraftingViewLabel(Preset);
        if (PressButton(State, Label.c_str()))
        {
            PresentView(State, Preset);
            Announce(State, std::string("View ") + DraftingViewLabel(Preset));
        }
        ImGui::SameLine(0, 4);
    }
    if (PressButton(State, "More…###ViewMore")) ImGui::OpenPopup("MoreViews");
    if (ImGui::BeginPopup("MoreViews"))
    {
        for (DraftingViewPreset Preset : { DraftingViewPreset::Bottom, DraftingViewPreset::Back, DraftingViewPreset::Left })
        {
            const std::string Label = std::string(DraftingViewLabel(Preset)) + "###View" + DraftingViewLabel(Preset);
            if (ImGui::Selectable(Label.c_str())) PresentView(State, Preset);
            RegisterWidget(State, Label.c_str());
        }
        ImGui::EndPopup();
    }
    ImGui::SameLine(0, 16);
    if (ToggleButton(State, State.Camera.OrthographicEnabled ? "Parallel###Projection" : "Perspective###Projection", State.Camera.OrthographicEnabled))
        State.Camera.OrthographicEnabled = !State.Camera.OrthographicEnabled;
    ImGui::SameLine(0, 4);
    if (ToggleButton(State, "Grid###GridToggle", State.Style.GridEnabled)) State.Style.GridEnabled = !State.Style.GridEnabled;
    ImGui::SameLine(0, 4);
    if (ToggleButton(State, "Edges###EdgeToggle", State.Style.EdgesEnabled)) State.Style.EdgesEnabled = !State.Style.EdgesEnabled;
    ImGui::SameLine(0, 4);
    if (PressButton(State, "Fit###FitView") && !State.Scene.Faces.empty()) FitDraftingCamera(State.Camera, State.Scene);
    ImGui::SameLine(0, 16);
    const std::pair<DraftingPickMode, const char*> Modes[] = { { DraftingPickMode::Edge, "2 Edge###Mode2" }, { DraftingPickMode::Face, "3 Face###Mode3" },
                                                               { DraftingPickMode::Solid, "4 Solid###Mode4" }, { DraftingPickMode::Any, "5 All###Mode5" } };
    for (const auto& [Mode, Label] : Modes)
    {
        if (ToggleButton(State, Label, State.PickMode == Mode)) State.PickMode = Mode;
        ImGui::SameLine(0, 4);
    }
    ImGui::NewLine();
}

void DrawViewport(DraftingWorkspaceState& State, ImTextureID ViewportTexture, ImVec2 Position, ImVec2 Size)
{
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.090f, 0.106f, 0.137f, 1.0f));
    ImGui::SetNextWindowPos(Position);
    ImGui::SetNextWindowSize(Size);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, PanelRounding);
    ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                                      ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoScrollbar |
                                      ImGuiWindowFlags_NoScrollWithMouse);
    ImDrawList* Canvas = ImGui::GetWindowDrawList();
    Canvas->AddRectFilled(Position, ImVec2(Position.x + Size.x, Position.y + ViewportBar), IM_COL32(0x1e, 0x22, 0x2b, 0xff), PanelRounding, ImDrawFlags_RoundCornersTop);
    DrawViewportBar(State);

    const ImVec2 Image(Position.x, Position.y + ViewportBar);
    const ImVec2 Extent(Size.x, std::max(8.0f, Size.y - ViewportBar));
    State.ViewportRect = { Image.x, Image.y, Extent.x, Extent.y };
    ImGui::SetCursorScreenPos(Image);
    ImGui::InvisibleButton("ViewportSurface", Extent, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);
    RegisterWidget(State, "ViewportSurface");
    const bool Hovered = ImGui::IsItemHovered();
    State.ViewportHovered = Hovered;
    if (ViewportTexture)
        Canvas->AddImageRounded(ViewportTexture, Image, ImVec2(Image.x + Extent.x, Image.y + Extent.y), ImVec2(0, 0), ImVec2(1, 1), IM_COL32_WHITE, PanelRounding,
                                ImDrawFlags_RoundCornersBottom);

    // 📝 Gestures (Plasticity orbit-controls / viewport-selector): middle drag orbit (yaw about world z, elevation about
    //    camera right), right drag pan, wheel dolly; LMB click replaces the selection, Shift+click adds, Ctrl+click
    //    removes, Alt+click adds the edge's tangent chain. In a command: click edits the selection, right-click finishes.
    ImGuiIO&     Io        = ImGui::GetIO();
    const ImVec2 Local(Io.MousePos.x - Image.x, Io.MousePos.y - Image.y);
    const ImVec2 CubeCorner(Image.x + Extent.x - CubeExtent - 12, Image.y + 12);
    const bool   LeftClick  = Hovered && ImGui::IsMouseReleased(ImGuiMouseButton_Left) && Io.MouseDragMaxDistanceSqr[ImGuiMouseButton_Left] <= ClickTolerance * ClickTolerance;
    const bool   RightClick = Hovered && ImGui::IsMouseReleased(ImGuiMouseButton_Right) && Io.MouseDragMaxDistanceSqr[ImGuiMouseButton_Right] <= ClickTolerance * ClickTolerance;
    const bool   OverCube   = DrawOrientationCube(State, Canvas, CubeCorner, LeftClick);
    const DraftingCameraFrame Frame = ViewportFrame(State);
    if (ImGui::IsItemActive() || Hovered)
    {
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 0.0f))
            OrbitDraftingCamera(State.Camera, Io.MouseDelta.x, Io.MouseDelta.y);
        else if (ImGui::IsMouseDragging(ImGuiMouseButton_Right, ClickTolerance))
            PanDraftingCamera(State.Camera, Frame, Io.MouseDelta.x, Io.MouseDelta.y);
    }
    if (Hovered && Io.MouseWheel != 0.0f) DollyDraftingCamera(State.Camera, Io.MouseWheel);

    if (State.Bevel.ActiveCondition)
    {
        const bool Idle = !ImGui::IsAnyMouseDown();
        if (Hovered && Idle && !State.Bevel.PinnedCondition && Io.MouseDelta.x != 0.0f)
            AdjustBevelSize(State, Io.MouseDelta.x * Frame.WorldPerPixel * (Io.KeyShift ? 0.1 : 1.0) * 0.5);
        if (LeftClick && !OverCube)
        {
            PickAtPointer(State, Local, PickIntentOf(Io));
            State.Bevel.StagedSize = -1.0;                                                   // Selection changed: rebuild the preview
        }
        if (RightClick && !OverCube) ApplyBevelSession(State);
    }
    else if (LeftClick && !OverCube)
    {
        PickAtPointer(State, Local, PickIntentOf(Io));
    }

    // Overlays: status pills (top-left), interaction hint (bottom-left).
    ImVec2 Pill(Image.x + 12, Image.y + 12);
    static const char* ModeNames[] = { "", "", "Mode 2 · edges", "Mode 3 · faces", "Mode 4 · solid", "Mode 5 · all" };
    DrawStatusPill(Canvas, Pill, ModeNames[int(State.PickMode)], SelectedColour);
    DrawStatusPill(Canvas, Pill, State.Camera.OrthographicEnabled ? "Parallel" : "Perspective", MutedColour);
    static const char* PlaneNames[] = { "Plane XY", "Plane XZ", "Plane YZ" };
    DrawStatusPill(Canvas, Pill, PlaneNames[std::clamp(State.Style.GridPlane, 0, 2)], AxisColours[State.Style.GridPlane == 2 ? 1 : 0]);
    const DraftingBodySummary& Summary = State.Document.Summary;
    if (Summary.PresentCondition)
        DrawStatusPill(Canvas, Pill, Compose("%s · genus %d", Summary.SoundCondition ? "Sound solid" : "Audit findings", Summary.Genus),
                       Summary.SoundCondition ? ActiveColour : RefusalColour);
    if (State.HoverIdentity != 0 && Hovered)
        DrawStatusPill(Canvas, Pill, NameDraftingIdentity(DisplayedDraftingScene(State), State.HoverIdentity), IM_COL32(0xff, 0xd2, 0x4d, 0xff));
    if (State.Bevel.ActiveCondition)
    {
        ImVec2 Below(Image.x + 12, Image.y + 44);
        const bool Refused = !State.Bevel.Refusal.empty();
        const char* Caption = State.Bevel.Verb == DraftingSessionVerb::Extrude ? "Extrude d" : State.Bevel.Shape == BlendProfile::Fillet ? "Fillet r" : "Chamfer d";
        const std::string Sizes = State.Bevel.VariableEnabled && State.Bevel.Verb == DraftingSessionVerb::Blend
                                ? FormatLength(State.Bevel.Size) + " → " + FormatLength(State.Bevel.EndSize) + (State.Bevel.ReversedCondition ? " (flipped)" : "")
                                : FormatLength(State.Bevel.Size);
        DrawStatusPill(Canvas, Below, Compose("%s %s m%s", Caption, Sizes.c_str(),
                                              State.Bevel.PinnedCondition ? " · pinned" : ""), Refused ? RefusalColour : ActiveColour);
        if (Refused) DrawStatusPill(Canvas, Below, State.Bevel.Refusal, RefusalColour);
    }
    const char* Hint = State.Bevel.ActiveCondition
                           ? "Move: size (Shift fine) · D fillet · C chamfer · V variable · F flip anchor · click edits selection · Enter / right-click finish · Esc abort"
                           : "Middle orbit · right pan · wheel zoom · click select (Shift add, Ctrl remove, Alt chain) · 2/3/4/5 modes · B fillet · E extrude · Space frame";
    Canvas->AddText(ImVec2(Image.x + 14, Image.y + Extent.y - 26), MutedColour, Hint);

    ImGui::End();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    INSPECTOR
//------------------------------------------------------------------------------------------------------------------------

void DrawBevelCard(DraftingWorkspaceState& State)
{
    DraftingBevelSession& Bevel   = State.Bevel;
    const bool            Extrude = Bevel.Verb == DraftingSessionVerb::Extrude;
    BeginCard(Extrude ? "Extrude" : "Bevel");
    if (!Extrude)
    {
        if (ToggleButton(State, "Fillet  (D)###BevelFillet", Bevel.Shape == BlendProfile::Fillet, ImVec2(130, 0))) Bevel.Shape = BlendProfile::Fillet;
        ImGui::SameLine();
        if (ToggleButton(State, "Chamfer  (C)###BevelChamfer", Bevel.Shape == BlendProfile::Chamfer, ImVec2(130, 0))) Bevel.Shape = BlendProfile::Chamfer;
    }
    ImGui::SetNextItemWidth(110);
    if (ImGui::InputText(Extrude ? "Distance###BevelSize" : Bevel.Shape == BlendProfile::Fillet ? "Radius###BevelSize" : "Setback###BevelSize", Bevel.SizeText.data(),
                         Bevel.SizeText.size(), ImGuiInputTextFlags_EnterReturnsTrue))
    {
        double Parsed = 0.0;
        if (ParseDraftingLength(Bevel.SizeText.data(), Parsed) && Parsed != 0.0)
        {
            if (Parsed < 0.0 && !Extrude) Bevel.Shape = BlendProfile::Chamfer;
            Bevel.Size            = Extrude ? Parsed : std::abs(Parsed);
            Bevel.PinnedCondition = true;
            Announce(State, Compose("Bevel size %s m (pinned)", FormatLength(Bevel.Size).c_str()));
        }
        else
        {
            Announce(State, std::string("Unreadable size: ") + Bevel.SizeText.data());
        }
        WriteBevelText(Bevel);
    }
    RegisterWidget(State, "BevelSize");
    ImGui::SameLine();
    if (PressButton(State, "Mouse adjust###BevelMouse", ImVec2(0, 0), Bevel.PinnedCondition)) Bevel.PinnedCondition = false;
    if (!Extrude)
    {
        // Variable size (V): the end size field and the anchor flip (F).
        if (ToggleButton(State, "Variable  (V)###BevelVariable", Bevel.VariableEnabled, ImVec2(130, 0))) ToggleBevelVariable(State);
        if (Bevel.VariableEnabled)
        {
            ImGui::SameLine();
            if (PressButton(State, "Flip anchor  (F)###BevelFlip", ImVec2(130, 0))) FlipBevelAnchor(State);
            ImGui::SetNextItemWidth(110);
            if (ImGui::InputText("End size###BevelEndSize", Bevel.EndSizeText.data(), Bevel.EndSizeText.size(), ImGuiInputTextFlags_EnterReturnsTrue))
            {
                double Parsed = 0.0;
                if (ParseDraftingLength(Bevel.EndSizeText.data(), Parsed) && Parsed > 0.0)
                {
                    Bevel.EndSize = Parsed;
                    Announce(State, Compose("Bevel end size %s m", FormatLength(Bevel.EndSize).c_str()));
                }
                else
                {
                    Announce(State, std::string("Unreadable end size: ") + Bevel.EndSizeText.data());
                }
                WriteBevelText(Bevel);
            }
            RegisterWidget(State, "BevelEndSize");
            ImGui::PushStyleColor(ImGuiCol_Text, Tint(MutedColour));
            ImGui::TextWrapped("%s m at each edge's %s corner → %s m at the other", FormatLength(Bevel.Size).c_str(), Bevel.ReversedCondition ? "end" : "start",
                               FormatLength(Bevel.EndSize).c_str());
            ImGui::PopStyleColor();
        }
    }
    ImGui::PushStyleColor(ImGuiCol_Text, Tint(MutedColour));
    const std::vector<std::string>& Operands = Extrude ? State.Document.Selection.Faces : State.Document.Selection.Edges;
    ImGui::TextWrapped("%d %s: %s", int(Operands.size()), Extrude ? "face(s)" : "edge(s)", Operands.empty() ? "-" : Operands.front().c_str());
    ImGui::TextUnformatted("Units: 250mm, -25cm, 0.25m, 1in, 1ft");
    ImGui::PopStyleColor();
    if (!Bevel.Refusal.empty())
    {
        ImGui::PushStyleColor(ImGuiCol_Text, Tint(RefusalColour));
        ImGui::TextWrapped("Refused: %s", Bevel.Refusal.c_str());
        ImGui::PopStyleColor();
    }
    else if (Bevel.Preview)
    {
        ImGui::Text("Preview: %d faces · %.4f m³", Bevel.Preview->Summary.Faces, Bevel.Preview->Summary.Volume);
    }
    if (PressButton(State, "Apply###BevelApply", ImVec2(130, 0), Bevel.Refusal.empty())) ApplyBevelSession(State);
    ImGui::SameLine();
    if (PressButton(State, "Cancel###BevelCancel", ImVec2(130, 0))) CancelBevelSession(State);
    EndCard();
}

void DrawSelectionCard(DraftingWorkspaceState& State)
{
    const DraftingSelection& Selection = State.Document.Selection;
    BeginCard("Selection");
    if (Selection.Faces.empty() && Selection.Edges.empty())
    {
        ImGui::PushStyleColor(ImGuiCol_Text, Tint(MutedColour));
        ImGui::TextWrapped("Nothing selected. Click a face or an edge (1 any · 2 edges · 3 faces).");
        ImGui::PopStyleColor();
    }
    int Shown = 0;
    for (const DraftingSceneFace& Face : State.Scene.Faces)
    {
        if (std::find(Selection.Faces.begin(), Selection.Faces.end(), Face.Name) == Selection.Faces.end() || ++Shown > 8) continue;
        const bool Active = Selection.LastKind == DraftingSelectionKind::Face && Selection.Faces.back() == Face.Name;
        ImGui::TextColored(Tint(Active ? ActiveColour : SelectedColour), "Face");
        ImGui::SameLine();
        ImGui::TextWrapped("%s · %s · %.4f m²", Face.Name.c_str(), Face.Surface.c_str(), Face.Area);
    }
    for (const DraftingSceneEdge& Edge : State.Scene.Edges)
    {
        if (std::find(Selection.Edges.begin(), Selection.Edges.end(), Edge.Name) == Selection.Edges.end() || ++Shown > 8) continue;
        const bool Active = Selection.LastKind == DraftingSelectionKind::Edge && Selection.Edges.back() == Edge.Name;
        ImGui::TextColored(Tint(Active ? ActiveColour : SelectedColour), "Edge");
        ImGui::SameLine();
        ImGui::TextWrapped("%s · %s · %.4f m", Edge.Name.c_str(), Edge.Curve.c_str(), Edge.Length);
    }
    if (Shown > 8) ImGui::Text("… %d more", Shown - 8);
    if (!Selection.Faces.empty() || !Selection.Edges.empty())
    {
        if (PressButton(State, "Clear selection###SelectionClear"))
        {
            ClearDraftingSelection(State.Document);
            Announce(State, "Selection cleared");
        }
    }
    EndCard();

    if (!Selection.Edges.empty())
    {
        BeginCard("Edges");
        if (PressButton(State, "Fillet  (B)###EdgeFillet", ImVec2(140, 0))) OpenBevelSession(State, BlendProfile::Fillet);
        ImGui::SameLine();
        if (PressButton(State, "Chamfer  (B, C)###EdgeChamfer", ImVec2(140, 0))) OpenBevelSession(State, BlendProfile::Chamfer);
        EndCard();
    }
    if (!Selection.Faces.empty())
    {
        BeginCard("Face edit");
        int Current = 0;
        for (int Index = 0; Index < int(std::size(FaceVerbs)); ++Index)
            if (FaceVerbs[Index] == State.FaceVerb) Current = Index;
        ImGui::SetNextItemWidth(150);
        if (ImGui::BeginCombo("Verb###FaceVerb", DraftingFaceOperationLabel(State.FaceVerb)))
        {
            for (int Index = 0; Index < int(std::size(FaceVerbs)); ++Index)
            {
                const std::string Label = std::string(DraftingFaceOperationLabel(FaceVerbs[Index])) + "###FaceVerb:" + DraftingFaceOperationLabel(FaceVerbs[Index]);
                if (ImGui::Selectable(Label.c_str(), Index == Current)) State.FaceVerb = FaceVerbs[Index];
                RegisterWidget(State, Label.c_str());
            }
            ImGui::EndCombo();
        }
        RegisterWidget(State, "FaceVerb");
        if (State.FaceVerb != FaceOperationKind::Delete)
        {
            ImGui::SetNextItemWidth(150);
            const std::string Label = Compose("Amount [%s]###FaceAmount", FaceAmountUnit(State.FaceVerb));
            ImGui::InputDouble(Label.c_str(), &State.FaceAmount, 0.05, 0.25, "%.4f");
            RegisterWidget(State, "FaceAmount");
        }
        if (PressButton(State, "Apply  (E: extrude)###FaceApply", ImVec2(-1, 0)))
        {
            const double                 Amount  = FaceAmountToModel(State.FaceVerb, State.FaceAmount);
            const DraftingCommandOutcome Outcome = ChangeDraftingSelection(State.Document, State.FaceVerb, Amount);
            if (Outcome.AcceptedCondition) State.LastCommand = { DraftingRepeatKind::Face, BlendProfile::Fillet, 0.0, State.FaceVerb, Amount, {} };
            Conclude(State, Outcome);
        }
        EndCard();
    }
}

void DrawStepCard(DraftingWorkspaceState& State)
{
    const FeatureStep* Step = nullptr;
    for (const FeatureStep& Candidate : State.Document.Tree.Steps)
        if (Candidate.Label == State.FocusedStep) Step = &Candidate;
    if (!Step) return;
    const FeatureStepReport* Report = FindReport(State.Document, Step->Label);
    const std::string        Label  = Step->Label;
    BeginCard("Feature");
    ImGui::TextWrapped("%s", DescribeDraftingStep(State.Document, *Step).c_str());
    ImGui::PushStyleColor(ImGuiCol_Text, Tint(MutedColour));
    ImGui::Text("Tag %s · %.1f ms", Label.c_str(), Report ? Report->Milliseconds : 0.0);
    ImGui::PopStyleColor();
    if (Report && Report->Status == FeatureStepStatus::Refused)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, Tint(RefusalColour));
        ImGui::TextWrapped("Refused: %s", Legible(Report->Refusal).c_str());
        ImGui::PopStyleColor();
    }
    const auto Caption = State.Document.Captions.find(Label);
    const std::string Prefix = Caption == State.Document.Captions.end() ? Label + "." : Caption->second + ".";
    std::vector<FeatureParameter> Parameters;
    for (const FeatureParameter& Parameter : State.Document.Tree.Parameters)
        if (Parameter.Name.rfind(Prefix, 0) == 0) Parameters.push_back(Parameter);
    for (FeatureParameter& Parameter : Parameters)
    {
        double Value = Parameter.Value;
        ImGui::SetNextItemWidth(150);
        const std::string Field = Parameter.Name.substr(Prefix.size()) + "###Parameter:" + Parameter.Name;
        ImGui::InputDouble(Field.c_str(), &Value, 0.0, 0.0, "%.4f");
        RegisterWidget(State, Field.c_str());
        if (ImGui::IsItemDeactivatedAfterEdit() && Value != Parameter.Value)
        {
            Conclude(State, AssignDraftingParameter(State.Document, Parameter.Name, Value));
            break;
        }
    }
    if (PressButton(State, "Delete step  (Del)###StepDelete"))
    {
        Conclude(State, DeleteDraftingStep(State.Document, Label));
        State.FocusedStep = {};
    }
    EndCard();
}

void DrawBodyCard(DraftingWorkspaceState& State)
{
    const DraftingBodySummary& Summary = State.Document.Summary;
    BeginCard("Body");
    if (!Summary.PresentCondition)
    {
        ImGui::TextDisabled("No body");
    }
    else
    {
        ImGui::Text("Faces %d · Edges %d · Vertices %d", Summary.Faces, Summary.Edges, Summary.Vertices);
        ImGui::Text("Genus %d · Volume %.5f m³", Summary.Genus, Summary.Volume);
        ImGui::TextColored(Tint(Summary.SoundCondition ? ActiveColour : RefusalColour), "%s", Summary.SoundCondition ? "Audit: sound" : "Audit: findings");
        ImGui::SameLine();
        ImGui::TextDisabled("· replay %.1f ms · %d triangles", Summary.RegenerationMs, int(State.Scene.Corners.size() / 3));
    }
    EndCard();
}

void DrawHistoryCard(DraftingWorkspaceState& State)
{
    BeginCard("History");
    const auto& Undo = State.Document.UndoEntries;
    const auto& Redo = State.Document.RedoEntries;
    for (size_t Index = Undo.size() > 6 ? Undo.size() - 6 : 0; Index < Undo.size(); ++Index) ImGui::BulletText("%s", Undo[Index].Label.c_str());
    for (auto Entry = Redo.rbegin(); Entry != Redo.rend(); ++Entry) ImGui::TextDisabled("   %s (redo)", Entry->Label.c_str());
    if (Undo.empty() && Redo.empty()) ImGui::TextDisabled("No commands yet");
    EndCard();
}

void DrawInspector(DraftingWorkspaceState& State, ImVec2 Position, ImVec2 Size)
{
    BeginPanel("Inspector", Position, Size);
    PanelHeading("Inspector");
    if (State.Bevel.ActiveCondition) DrawBevelCard(State);
    else DrawSelectionCard(State);
    DrawStepCard(State);
    DrawBodyCard(State);
    DrawHistoryCard(State);
    EndPanel();
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    SHORTCUTS + RECOVERY
//------------------------------------------------------------------------------------------------------------------------

// Plasticity Space (viewport:navigate:selection): frame the selected faces and edges, or the whole body.
void FrameSelection(DraftingWorkspaceState& State)
{
    if (State.Scene.Faces.empty()) return;
    const DraftingSelection& Selection = State.Document.Selection;
    DraftingScene Bounds;
    Bounds.BoundsLow  = { 1.0e300, 1.0e300, 1.0e300 };
    Bounds.BoundsHigh = { -1.0e300, -1.0e300, -1.0e300 };
    bool Found = false;
    const auto Include = [&](const float* Point)
    {
        for (int Axis = 0; Axis < 3; ++Axis)
        {
            Bounds.BoundsLow[Axis]  = std::min(Bounds.BoundsLow[Axis], double(Point[Axis]));
            Bounds.BoundsHigh[Axis] = std::max(Bounds.BoundsHigh[Axis], double(Point[Axis]));
        }
        Found = true;
    };
    for (const DraftingSceneFace& Face : State.Scene.Faces)
    {
        if (std::find(Selection.Faces.begin(), Selection.Faces.end(), Face.Name) == Selection.Faces.end()) continue;
        for (uint32_t Corner = Face.FirstCorner; Corner < Face.FirstCorner + Face.CornerCount; ++Corner) Include(State.Scene.Corners[Corner].Position);
    }
    for (const DraftingSceneEdge& Edge : State.Scene.Edges)
    {
        if (std::find(Selection.Edges.begin(), Selection.Edges.end(), Edge.Name) == Selection.Edges.end()) continue;
        for (uint32_t Segment = Edge.FirstSegment; Segment < Edge.FirstSegment + Edge.SegmentCount; ++Segment)
        {
            Include(State.Scene.Segments[Segment].Start);
            Include(State.Scene.Segments[Segment].End);
        }
    }
    FitDraftingCamera(State.Camera, Found ? Bounds : State.Scene);
    Announce(State, Found ? "Framed the selection (Space)" : "Framed the body (Space)");
}

// Plasticity Shift+R (edit:repeat-last-command) on the current selection.
void RepeatLastCommand(DraftingWorkspaceState& State)
{
    const DraftingRepeat& Last = State.LastCommand;
    switch (Last.Kind)
    {
    case DraftingRepeatKind::None:      Announce(State, "Nothing to repeat yet"); return;
    case DraftingRepeatKind::Blend:     Conclude(State, BlendDraftingSelection(State.Document, Last.Blend)); return;
    case DraftingRepeatKind::Face:      Conclude(State, ChangeDraftingSelection(State.Document, Last.Operation, Last.Amount)); return;
    case DraftingRepeatKind::Construct: Conclude(State, ConstructDraftingPrimitive(State.Document, Last.Construct)); return;
    }
}

void OpenConstruct(DraftingWorkspaceState& State, DraftingConstructKind Kind)
{
    State.ConstructForm.Kind = Kind;
    State.ConstructOpen      = true;
}

// 📝 Plasticity default keymap (nkallen/plasticity src/startup/default-keymap.ts). Commands this modeller does not have
//    (control points, curves, hide, materials, groups, clipboard, booleans between bodies) announce that instead of
//    silently doing something else; no non-Plasticity shortcut remains.
void ApplyShortcuts(DraftingWorkspaceState& State)
{
    ImGuiIO& Io = ImGui::GetIO();
    if (Io.WantTextInput || ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel)) return;
    const bool Control   = Io.KeyCtrl;
    const bool Shift     = Io.KeyShift;
    const bool Alternate = Io.KeyAlt;
    auto Pressed = [](ImGuiKey Key) { return ImGui::IsKeyPressed(Key, false); };

    // body[command] — a running command (fillet-solid, extrude).
    if (State.Bevel.ActiveCondition)
    {
        if (Pressed(ImGuiKey_Enter) || Pressed(ImGuiKey_KeypadEnter)) ApplyBevelSession(State);
        if (Pressed(ImGuiKey_Escape)) CancelBevelSession(State);
        if (State.Bevel.Verb == DraftingSessionVerb::Blend && !Control && !Alternate)
        {
            if (Pressed(ImGuiKey_D)) State.Bevel.Shape = BlendProfile::Fillet;                // gizmo:fillet-solid:fillet
            if (Pressed(ImGuiKey_C)) State.Bevel.Shape = BlendProfile::Chamfer;               // gizmo:fillet-solid:chamfer
            if (Pressed(ImGuiKey_V)) ToggleBevelVariable(State);                              // gizmo:fillet-solid:add (variable size)
            if (Pressed(ImGuiKey_F)) FlipBevelAnchor(State);                                  // as gizmo:place:flip
        }
        return;
    }

    // Viewport navigation (any modifier state the keymap names).
    if (Pressed(ImGuiKey_Keypad1)) PresentView(State, Control ? DraftingViewPreset::Back : DraftingViewPreset::Front);
    if (Pressed(ImGuiKey_Keypad3)) PresentView(State, Control ? DraftingViewPreset::Left : DraftingViewPreset::Right);
    if (Pressed(ImGuiKey_Keypad7)) PresentView(State, Control ? DraftingViewPreset::Bottom : DraftingViewPreset::Top);
    if (Pressed(ImGuiKey_Keypad5)) State.Camera.OrthographicEnabled = !State.Camera.OrthographicEnabled;

    if (Control)
    {
        if (Pressed(ImGuiKey_Z)) { if (Shift) Redo(State); else Undo(State); }
        if (Pressed(ImGuiKey_S))
        {
            if (Shift) State.PopupRequest = "Save document";                                // file:save-as
            else SaveDocument(State, State.DocumentPath.empty() ? State.Options.DocumentPath : State.DocumentPath);
        }
        if (Pressed(ImGuiKey_O)) State.PopupRequest = "Open document";
        if (Pressed(ImGuiKey_N)) StartNewDocument(State);
        if (Pressed(ImGuiKey_C) || Pressed(ImGuiKey_V)) Announce(State, "Copy / paste: not available (one body per document)");
        if (Pressed(ImGuiKey_H)) Announce(State, "Invert hidden: not available (no per-face visibility)");
        if (Pressed(ImGuiKey_G)) Announce(State, "Group: not available");
        if (Pressed(ImGuiKey_D)) Announce(State, "Place: not available");
        if (Pressed(ImGuiKey_1) || Pressed(ImGuiKey_2) || Pressed(ImGuiKey_3) || Pressed(ImGuiKey_4)) Announce(State, "Selection convert: not available yet");
        return;
    }
    if (Alternate)
    {
        if (Pressed(ImGuiKey_A)) { ClearDraftingSelection(State.Document); State.FocusedStep = {}; Announce(State, "Selection cleared"); }
        if (Pressed(ImGuiKey_Z))
        {
            if (Shift) { State.Style.GridEnabled = !State.Style.GridEnabled; Announce(State, State.Style.GridEnabled ? "Overlays on" : "Overlays off"); }
            else { State.Style.SurfacesEnabled = !State.Style.SurfacesEnabled; Announce(State, State.Style.SurfacesEnabled ? "X-ray off" : "X-ray on (edges only)"); }
        }
        if (Pressed(ImGuiKey_E)) Announce(State, "Evolution: not available");
        if (Pressed(ImGuiKey_X)) Announce(State, "Mirror: not available");
        if (Pressed(ImGuiKey_H) || Pressed(ImGuiKey_M) || Pressed(ImGuiKey_G) || Pressed(ImGuiKey_Q)) Announce(State, "Not available in this modeller");
        return;
    }
    if (Shift)
    {
        if (Pressed(ImGuiKey_C) || Pressed(ImGuiKey_V)) OpenConstruct(State, DraftingConstructKind::Block);     // corner / centre box
        if (Pressed(ImGuiKey_X)) OpenConstruct(State, DraftingConstructKind::Cylinder);
        if (Pressed(ImGuiKey_Z)) OpenConstruct(State, DraftingConstructKind::Sphere);
        if (Pressed(ImGuiKey_R)) RepeatLastCommand(State);
        if (Pressed(ImGuiKey_D)) Announce(State, "Duplicate: not available (one body per document)");
        if (Pressed(ImGuiKey_A) || Pressed(ImGuiKey_S) || Pressed(ImGuiKey_Q) || Pressed(ImGuiKey_W)) Announce(State, "Curves and sketches: use the outliner's sketch steps");
        if (Pressed(ImGuiKey_H) || Pressed(ImGuiKey_P)) Announce(State, "Not available in this modeller");
        if (Pressed(ImGuiKey_1) || Pressed(ImGuiKey_2) || Pressed(ImGuiKey_3) || Pressed(ImGuiKey_4)) Announce(State, "Mode toggles: one mode at a time (2 / 3 / 4 / 5)");
        return;
    }

    // body:not([gizmo]) — no command running.
    if (Pressed(ImGuiKey_1)) Announce(State, "Control-point mode: not available (exact B-rep, no control points)");
    if (Pressed(ImGuiKey_2)) State.PickMode = DraftingPickMode::Edge;
    if (Pressed(ImGuiKey_3)) State.PickMode = DraftingPickMode::Face;
    if (Pressed(ImGuiKey_4)) State.PickMode = DraftingPickMode::Solid;
    if (Pressed(ImGuiKey_5) || Pressed(ImGuiKey_Tab)) State.PickMode = DraftingPickMode::Any;
    if (Pressed(ImGuiKey_B)) OpenBevelSession(State, BlendProfile::Fillet);
    if (Pressed(ImGuiKey_E)) OpenExtrudeSession(State);
    if (Pressed(ImGuiKey_Space)) FrameSelection(State);
    if (Pressed(ImGuiKey_Q)) Announce(State, "Boolean (Q) needs two bodies: construct with Shift+C / V / X / Z, then Q / W / E");
    if (Pressed(ImGuiKey_G) || Pressed(ImGuiKey_R) || Pressed(ImGuiKey_S)) Announce(State, "Move / rotate / scale bodies: not available (face edits: Face edit card)");
    if (Pressed(ImGuiKey_H)) Announce(State, "Hide: not available (no per-face visibility)");
    if (Pressed(ImGuiKey_C) || Pressed(ImGuiKey_J) || Pressed(ImGuiKey_P) || Pressed(ImGuiKey_T) || Pressed(ImGuiKey_O) || Pressed(ImGuiKey_L) || Pressed(ImGuiKey_M))
        Announce(State, "Not available in this modeller");
    if (Pressed(ImGuiKey_Escape))
    {
        ClearDraftingSelection(State.Document);
        State.FocusedStep = {};
        Announce(State, "Selection cleared");
    }
    if (Pressed(ImGuiKey_X) || Pressed(ImGuiKey_Delete) || Pressed(ImGuiKey_Backspace))
    {
        if (!State.FocusedStep.empty())
        {
            Conclude(State, DeleteDraftingStep(State.Document, State.FocusedStep));
            State.FocusedStep = {};
        }
        else if (!State.Document.Selection.Faces.empty())
        {
            const DraftingCommandOutcome Outcome = ChangeDraftingSelection(State.Document, FaceOperationKind::Delete, 0.0);
            if (Outcome.AcceptedCondition) State.LastCommand = { DraftingRepeatKind::Face, BlendProfile::Fillet, 0.0, FaceOperationKind::Delete, 0.0, {} };
            Conclude(State, Outcome);
        }
        else Announce(State, "Delete needs selected faces or a focused feature step");
    }
}

void DrawWelcome(DraftingWorkspaceState& State)
{
    if (State.WelcomeOpen && !ImGui::IsPopupOpen("Welcome back")) ImGui::OpenPopup("Welcome back");
    if (!ImGui::BeginPopupModal("Welcome back", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) return;
    ImGui::TextUnformatted("An autosaved document is available.");
    ImGui::TextDisabled("%s", State.Options.RecoveryPath.c_str());
    ImGui::Spacing();
    if (PressButton(State, "Continue previous###WelcomeContinue", ImVec2(170, 0)))
    {
        OpenDocument(State, State.Options.RecoveryPath, false);
        State.WelcomeOpen = false;
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (PressButton(State, "New document###WelcomeNew", ImVec2(170, 0)))
    {
        StartNewDocument(State);
        State.WelcomeOpen = false;
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}

void AdvanceAutosave(DraftingWorkspaceState& State, double DeltaSeconds)
{
    if (State.AutosaveTimer < 0.0) return;
    if (ImGui::IsAnyMouseDown() || State.Bevel.ActiveCondition) return;
    State.AutosaveTimer -= DeltaSeconds;
    if (State.AutosaveTimer > 0.0) return;
    State.AutosaveTimer = -1.0;
    if (WriteDocument(State.Document, State.Options.RecoveryPath)) State.Journal.push_back("Autosaved " + State.Options.RecoveryPath);
    else Announce(State, "Autosave failed: " + State.Options.RecoveryPath);
}

void AppendJsonText(std::ostringstream& Stream, const std::string& Text)
{
    Stream << '"';
    for (const char Character : Text)
    {
        if (Character == '"' || Character == '\\') Stream << '\\' << Character;
        else if (Character == '\n') Stream << "\\n";
        else if (static_cast<unsigned char>(Character) < 0x20) Stream << ' ';
        else Stream << Character;
    }
    Stream << '"';
}

} // namespace

//------------------------------------------------------------------------------------------------------------------------
//                                                    PUBLIC FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

void ConfigureDraftingInterface(const DraftingOptions& Options)
{
    ImGuiIO& Io = ImGui::GetIO();
    Io.IniFilename = nullptr;
    Io.LogFilename = nullptr;
    std::string FontPath = Options.FontPath;
    if (FontPath.empty())
    {
        for (const char* Candidate : { "EngineContent/FontArchives/DM Sans/DMSans-Regular.ttf", "../../EngineContent/FontArchives/DM Sans/DMSans-Regular.ttf",
                                       "../../../EngineContent/FontArchives/DM Sans/DMSans-Regular.ttf" })
            if (std::filesystem::exists(Candidate)) { FontPath = Candidate; break; }
#ifdef DRAFTING_FONT_PATH
        if (FontPath.empty() && std::filesystem::exists(DRAFTING_FONT_PATH)) FontPath = DRAFTING_FONT_PATH;
#endif
    }
    if (!FontPath.empty() && std::filesystem::exists(FontPath)) Io.Fonts->AddFontFromFileTTF(FontPath.c_str(), Options.FontSize);
    else Io.Fonts->AddFontDefault();

    ImGuiStyle& Style = ImGui::GetStyle();
    ImGui::StyleColorsDark(&Style);
    Style.WindowBorderSize  = 0.0f;
    Style.ChildBorderSize   = 0.0f;
    Style.PopupBorderSize   = 1.0f;
    Style.FrameRounding     = 8.0f;
    Style.GrabRounding      = 8.0f;
    Style.PopupRounding     = 12.0f;
    Style.FramePadding      = ImVec2(10, 6);
    Style.ItemSpacing       = ImVec2(8, 7);
    Style.WindowPadding     = ImVec2(16, 14);
    Style.Colors[ImGuiCol_Text]             = ImVec4(0.93f, 0.93f, 0.93f, 1.0f);
    Style.Colors[ImGuiCol_PopupBg]          = Tint(CardColour);
    Style.Colors[ImGuiCol_Border]           = Tint(RaisedColour);
    Style.Colors[ImGuiCol_FrameBg]          = Tint(RaisedColour);
    Style.Colors[ImGuiCol_FrameBgHovered]   = ImVec4(0.21f, 0.21f, 0.21f, 1.0f);
    Style.Colors[ImGuiCol_FrameBgActive]    = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);
    Style.Colors[ImGuiCol_Button]           = Tint(RaisedColour);
    Style.Colors[ImGuiCol_ButtonHovered]    = ImVec4(0.25f, 0.40f, 0.62f, 1.0f);
    Style.Colors[ImGuiCol_ButtonActive]     = Tint(SelectedColour);
    Style.Colors[ImGuiCol_Header]           = Tint(SelectedColour);
    Style.Colors[ImGuiCol_HeaderHovered]    = ImVec4(0.25f, 0.40f, 0.62f, 1.0f);
    Style.Colors[ImGuiCol_HeaderActive]     = Tint(SelectedColour);
    Style.Colors[ImGuiCol_Separator]        = Tint(RaisedColour);
    Style.Colors[ImGuiCol_CheckMark]        = Tint(SelectedColour);
    Style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.45f);
}

void InitialiseDraftingWorkspace(DraftingWorkspaceState& State, const DraftingOptions& Options)
{
    State.Options                    = Options;
    State.Document                   = CreateDraftingDocument("Untitled");
    State.Camera                     = DraftingCamera{};
    State.Camera.FieldOfView         = Options.FieldOfView;
    State.Camera.OrthographicEnabled = Options.OrthographicEnabled;
    State.Style.EdgeStroke           = Options.EdgeStroke;
    State.Style.GridEnabled          = Options.GridEnabled;
    State.WelcomeOpen                = !Options.RecoveryPath.empty() && std::filesystem::exists(Options.RecoveryPath);
    RefreshDraftingScene(State, false);
    if (!Options.Refusal.empty()) Announce(State, "Options: " + Options.Refusal);
}

void DrawDraftingInterface(DraftingWorkspaceState& State, ImTextureID ViewportTexture, double DeltaSeconds)
{
    State.Widgets.clear();
    const ImVec2 Frame = ImGui::GetIO().DisplaySize;
    StageBevelPreview(State);
    DrawHeader(State, Frame);
    DrawDocumentBar(State, Frame);
    const float Top    = PanelGap + HeaderHeight + PanelGap + DocumentHeight + PanelGap;
    const float Height = std::max(100.0f, Frame.y - Top - PanelGap);
    const float Middle = std::max(100.0f, Frame.x - 4 * PanelGap - OutlinerWidth - InspectorWidth);
    DrawOutliner(State, ImVec2(PanelGap, Top), ImVec2(OutlinerWidth, Height));
    DrawViewport(State, ViewportTexture, ImVec2(2 * PanelGap + OutlinerWidth, Top), ImVec2(Middle, Height));
    DrawInspector(State, ImVec2(3 * PanelGap + OutlinerWidth + Middle, Top), ImVec2(InspectorWidth, Height));
    DrawWelcome(State);
    ApplyShortcuts(State);
    AdvanceAutosave(State, DeltaSeconds);
    StageBevelPreview(State);
    State.FrameNumber += 1;
}

const DraftingScene& DisplayedDraftingScene(const DraftingWorkspaceState& State)
{
    if (State.Bevel.ActiveCondition && State.Bevel.Preview && State.Bevel.Refusal.empty()) return State.Bevel.PreviewScene;
    return State.Scene;
}

DraftingSceneStates DisplayedDraftingStates(const DraftingWorkspaceState& State)
{
    const DraftingScene& Scene = DisplayedDraftingScene(State);
    if (&Scene == &State.Bevel.PreviewScene) return ComposeDraftingStates(Scene, State.Bevel.Preview->Selection, State.HoverIdentity);
    return ComposeDraftingStates(Scene, State.Document.Selection, State.ViewportHovered ? State.HoverIdentity : 0);
}

void RefreshDraftingScene(DraftingWorkspaceState& State, bool DirtyCondition)
{
    State.Scene = BuildDraftingScene(State.Document);
    State.Camera.SceneRadius = std::max(1.0, 0.5 * std::hypot(State.Scene.BoundsHigh[0] - State.Scene.BoundsLow[0], State.Scene.BoundsHigh[1] - State.Scene.BoundsLow[1],
                                                              State.Scene.BoundsHigh[2] - State.Scene.BoundsLow[2]));
    State.HoverIdentity = 0;
    if (!State.Scene.Refusal.empty()) Announce(State, "Tessellation refused: " + State.Scene.Refusal);
    if (DirtyCondition)
    {
        State.DirtyCondition = true;
        State.AutosaveTimer  = State.Options.AutosaveDelay;
    }
}

std::string DraftingPreviousRecoveryPath(const DraftingOptions& Options)
{
    const std::string& Path   = Options.RecoveryPath;
    const std::string  Suffix = ".drafting.toml";
    if (Path.size() > Suffix.size() && Path.compare(Path.size() - Suffix.size(), Suffix.size(), Suffix) == 0)
        return Path.substr(0, Path.size() - Suffix.size()) + ".previous" + Suffix;
    return Path + ".previous";
}

bool ParseDraftingLength(const std::string& Text, double& Value)
{
    std::string Compact;
    for (const char Character : Text)
        if (!std::isspace(static_cast<unsigned char>(Character))) Compact.push_back(char(std::tolower(static_cast<unsigned char>(Character))));
    if (Compact.empty()) return false;
    size_t Consumed = 0;
    double Number   = 0.0;
    try
    {
        Number = std::stod(Compact, &Consumed);
    }
    catch (...)
    {
        return false;
    }
    const std::string Unit = Compact.substr(Consumed);
    static const std::pair<const char*, double> Units[] = { { "", 1.0 }, { "m", 1.0 }, { "mm", 1.0e-3 }, { "cm", 1.0e-2 }, { "in", 0.0254 }, { "ft", 0.3048 } };
    for (const auto& [Suffix, Factor] : Units)
    {
        if (Unit == Suffix)
        {
            Value = Number * Factor;
            return std::isfinite(Value);
        }
    }
    return false;
}

std::string DescribeDraftingWorkspace(const DraftingWorkspaceState& State)
{
    const DraftingDocument&    Document = State.Document;
    const DraftingBodySummary& Summary  = Document.Summary;
    std::ostringstream Stream;
    Stream.precision(10);
    Stream << "{\n  \"Frame\": " << State.FrameNumber << ",\n  \"Title\": ";
    AppendJsonText(Stream, Document.Title);
    Stream << ",\n  \"DocumentPath\": ";
    AppendJsonText(Stream, State.DocumentPath);
    Stream << ",\n  \"Dirty\": " << (State.DirtyCondition ? "true" : "false") << ",\n  \"Steps\": [";
    for (size_t Index = 0; Index < Document.Tree.Steps.size(); ++Index)
    {
        const FeatureStep&       Step   = Document.Tree.Steps[Index];
        const FeatureStepReport* Report = FindReport(Document, Step.Label);
        Stream << (Index ? ", " : "") << "{\"Label\": ";
        AppendJsonText(Stream, Step.Label);
        Stream << ", \"Caption\": ";
        AppendJsonText(Stream, DescribeDraftingStep(Document, Step));
        Stream << ", \"Refused\": " << (Report && Report->Status == FeatureStepStatus::Refused ? "true" : "false") << "}";
    }
    Stream << "],\n  \"Body\": {\"Faces\": " << Summary.Faces << ", \"Edges\": " << Summary.Edges << ", \"Vertices\": " << Summary.Vertices
           << ", \"Genus\": " << Summary.Genus << ", \"Volume\": " << Summary.Volume << ", \"Sound\": " << (Summary.SoundCondition ? "true" : "false") << "},\n";
    Stream << "  \"Selection\": {\"Faces\": [";
    for (size_t Index = 0; Index < Document.Selection.Faces.size(); ++Index)
    {
        Stream << (Index ? ", " : "");
        AppendJsonText(Stream, Document.Selection.Faces[Index]);
    }
    Stream << "], \"Edges\": [";
    for (size_t Index = 0; Index < Document.Selection.Edges.size(); ++Index)
    {
        Stream << (Index ? ", " : "");
        AppendJsonText(Stream, Document.Selection.Edges[Index]);
    }
    Stream << "]},\n  \"PickMode\": " << int(State.PickMode) << ",\n  \"Hover\": ";
    AppendJsonText(Stream, State.HoverIdentity ? NameDraftingIdentity(DisplayedDraftingScene(State), State.HoverIdentity) : std::string());
    Stream << ",\n  \"Camera\": {\"Yaw\": " << State.Camera.Yaw << ", \"Pitch\": " << State.Camera.Pitch << ", \"Distance\": " << State.Camera.Distance
           << ", \"Target\": [" << State.Camera.Target[0] << ", " << State.Camera.Target[1] << ", " << State.Camera.Target[2] << "], \"Orthographic\": "
           << (State.Camera.OrthographicEnabled ? "true" : "false") << "},\n";
    Stream << "  \"Bevel\": {\"Active\": " << (State.Bevel.ActiveCondition ? "true" : "false") << ", \"Shape\": \""
           << (State.Bevel.Shape == BlendProfile::Fillet ? "Fillet" : "Chamfer") << "\", \"Size\": " << State.Bevel.Size << ", \"Pinned\": "
           << (State.Bevel.PinnedCondition ? "true" : "false") << ", \"Variable\": " << (State.Bevel.VariableEnabled ? "true" : "false")
           << ", \"EndSize\": " << State.Bevel.EndSize << ", \"Flipped\": " << (State.Bevel.ReversedCondition ? "true" : "false") << ", \"PreviewFaces\": " << (State.Bevel.Preview ? State.Bevel.Preview->Summary.Faces : 0)
           << ", \"Refusal\": ";
    AppendJsonText(Stream, State.Bevel.Refusal);
    Stream << "},\n  \"Undo\": " << Document.UndoEntries.size() << ",\n  \"Redo\": " << Document.RedoEntries.size() << ",\n  \"Welcome\": "
           << (State.WelcomeOpen ? "true" : "false") << ",\n  \"Status\": ";
    AppendJsonText(Stream, State.Status);
    Stream << ",\n  \"Viewport\": [" << State.ViewportRect[0] << ", " << State.ViewportRect[1] << ", " << State.ViewportRect[2] << ", " << State.ViewportRect[3] << "]\n}\n";
    return Stream.str();
}

} // namespace Frontier::DraftingWorkspace
