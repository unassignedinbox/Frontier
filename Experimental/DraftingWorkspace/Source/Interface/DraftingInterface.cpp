/*====================================================================================================================================
                                                     DRAFTINGINTERFACE.CPP
====================================================================================================================================*/
// 🧩 DraftingWorkspace ImGui shell: header, Outliner, viewport, Inspector, bevel session, shortcuts, autosave / recovery

//------------------------------------------------------------------------------------------------------------------------
//                                                    INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Interface/DraftingInterface.h"

#include "Document/DraftingDocumentCodec.h"
#include "Document/VehicleCurveImport.h"

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
constexpr float  ZebraStripeCount = 64.0f;                                                  // [-]   - Zebra stripes over the reflection angle (V8)
constexpr double DeviationLimit   = 2.0e-4;                                                 // [len] - Red end of the deviation scale (fit tolerance)
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
    Document.ImagePlanes = File.ImagePlanes;
    Document.Appearance  = File.Appearance;
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

void MarkDirty(DraftingWorkspaceState& State)
{
    State.DirtyCondition = true;
    State.AutosaveTimer  = State.Options.AutosaveDelay;
}

// 📝 A matcap entry leaves the body (and its scene) as it is: only the state words change, so no re-tessellation.
void Undo(DraftingWorkspaceState& State)
{
    if (State.Bevel.ActiveCondition) return;
    const bool AppearanceCondition = !State.Document.UndoEntries.empty() && State.Document.UndoEntries.back().AppearanceCondition;
    if (UndoDraftingCommand(State.Document))
    {
        if (AppearanceCondition) MarkDirty(State);
        else RefreshDraftingScene(State, true);
        Announce(State, "Undo");
    }
}

void Redo(DraftingWorkspaceState& State)
{
    if (State.Bevel.ActiveCondition) return;
    const bool AppearanceCondition = !State.Document.RedoEntries.empty() && State.Document.RedoEntries.back().AppearanceCondition;
    if (RedoDraftingCommand(State.Document))
    {
        if (AppearanceCondition) MarkDirty(State);
        else RefreshDraftingScene(State, true);
        Announce(State, "Redo");
    }
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    MATCAPS
//------------------------------------------------------------------------------------------------------------------------

void ConcludeAppearance(DraftingWorkspaceState& State, const DraftingCommandOutcome& Outcome)
{
    if (Outcome.AcceptedCondition)
    {
        MarkDirty(State);
        Announce(State, Outcome.Label);
    }
    else Announce(State, "Refused: " + Outcome.Refusal);
}

// M picker / swatch click: the selected faces (or the body) take the matcap.
void AssignMatcap(DraftingWorkspaceState& State, const std::string& Name)
{
    ConcludeAppearance(State, AssignDraftingMatcap(State.Document, Name));
}

// Alt+M (Plasticity remove-material).
void RemoveMatcap(DraftingWorkspaceState& State)
{
    ConcludeAppearance(State, RemoveDraftingMatcap(State.Document));
}

// A user picture: the file stem becomes the matcap name ("Shop steel.png" → "Shopsteel").
void AddMatcapPicture(DraftingWorkspaceState& State, const std::string& Path)
{
    std::string Stem = Path.substr(Path.find_last_of("/\\") == std::string::npos ? 0 : Path.find_last_of("/\\") + 1);
    Stem = Stem.substr(0, Stem.find_last_of('.'));
    std::erase_if(Stem, [](char Character) { return std::isspace(static_cast<unsigned char>(Character)) != 0; });
    ConcludeAppearance(State, AddDraftingMatcapPicture(State.Document, Stem, Path));
    RefreshDraftingMatcaps(State);
}

// 📝 The matcap the selection shows: faces first, else picked bodies, else the default; one name when all agree, else "mixed".
std::string SelectionMatcap(const DraftingWorkspaceState& State)
{
    const DraftingDocument&   Document   = State.Document;
    const DraftingAppearance& Appearance = Document.Appearance;
    const auto FaceShows = [&](const std::string& Face)
    { return DraftingFaceMatcap(Appearance, DraftingBodyOfName(Document, DraftingSelectionKind::Face, Face), Face); };
    if (!Document.Selection.Faces.empty())
    {
        const std::string Shown = FaceShows(Document.Selection.Faces.front());
        for (const std::string& Face : Document.Selection.Faces) if (FaceShows(Face) != Shown) return "mixed";
        return Shown;
    }
    if (Document.Selection.Bodies.empty()) return Appearance.BodyMatcap;
    const std::string Shown = DraftingBodyMatcap(Appearance, Document.Selection.Bodies.front());
    for (const std::string& Key : Document.Selection.Bodies) if (DraftingBodyMatcap(Appearance, Key) != Shown) return "mixed";
    return Shown;
}

// The body a Union / Difference / Intersection acts on: the first body picked in mode 4, else the active body (empty).
std::string BooleanTarget(const DraftingWorkspaceState& State)
{
    return State.Document.Selection.Bodies.empty() ? std::string{} : State.Document.Selection.Bodies.front();
}

// 💡 Swatch grid (thumbnails when the frame registered them, else a flat colour from the sphere's lit side) in category
//    order Studio · Metal · Plastic · Other · Pictures. TitledCondition writes the category above each group (the picker);
//    otherwise one compact flow fills the available width (the Inspector). Columns = 0 fits as many as the width allows.
//    Returns the clicked matcap name or "". Widget keys: Prefix + name ("MatcapBrass", "PickMatcapBrass").
std::string DrawMatcapSwatches(DraftingWorkspaceState& State, const char* Prefix, int Columns, float Side, bool TitledCondition)
{
    const DraftingMatcapLibrary& Library = State.Matcaps.Library;
    const std::string Current = SelectionMatcap(State);
    std::string Chosen;
    std::vector<std::pair<std::string, std::vector<size_t>>> Groups = { { "Studio", { 0 } }, { "Metal", {} }, { "Plastic", {} }, { "Other", {} }, { "Pictures", {} } };
    const std::vector<DraftingMatcapDescription>& BuiltIn = DraftingBuiltInMatcaps();
    for (size_t Layer = 1; Layer < Library.Names.size(); ++Layer)
    {
        const size_t Group = Layer <= BuiltIn.size() ? (BuiltIn[Layer - 1].Category == "Metal" ? 1 : BuiltIn[Layer - 1].Category == "Plastic" ? 2 : 3) : 4;
        Groups[Group].second.push_back(Layer);
    }
    const ImGuiStyle& Style = ImGui::GetStyle();
    if (Columns <= 0)
        Columns = std::max(1, int((ImGui::GetContentRegionAvail().x + Style.ItemSpacing.x) / (Side + 2.0f * Style.FramePadding.x + Style.ItemSpacing.x)));
    int Column = 0;
    for (const auto& [Title, Layers] : Groups)
    {
        if (Layers.empty()) continue;
        if (TitledCondition)
        {
            ImGui::TextDisabled("%s", Title.c_str());
            Column = 0;
        }
        for (const size_t Layer : Layers)
        {
            if (Column++ % Columns != 0) ImGui::SameLine();
            const std::string& Name  = Library.Names[Layer];
            const std::string  Label = std::string("###") + Prefix + Name;
            const bool CurrentCondition = Name == Current;
            ImGui::PushStyleColor(ImGuiCol_Button, CurrentCondition ? Tint(SelectedColour) : ImVec4(0, 0, 0, 0));
            bool Pressed = false;
            if (Layer < State.Matcaps.Thumbnails.size() && State.Matcaps.Thumbnails[Layer])
                Pressed = ImGui::ImageButton(Label.c_str(), State.Matcaps.Thumbnails[Layer], ImVec2(Side, Side));
            else
            {
                ImVec4 Flat(0.7f, 0.72f, 0.76f, 1.0f);
                if (Layer >= 1 && Layer - 1 < Library.Pictures.size() && !Library.Pictures[Layer - 1].Pixels.empty())
                {
                    const DraftingPicture& Picture = Library.Pictures[Layer - 1];
                    const size_t Texel = (size_t(Picture.Height * 0.35) * Picture.Width + size_t(Picture.Width * 0.4)) * 4;
                    Flat = ImVec4(Picture.Pixels[Texel] / 255.0f, Picture.Pixels[Texel + 1] / 255.0f, Picture.Pixels[Texel + 2] / 255.0f, 1.0f);
                }
                Pressed = ImGui::ColorButton(Label.c_str(), Flat, ImGuiColorEditFlags_NoTooltip, ImVec2(Side + 8, Side + 8));
            }
            ImGui::PopStyleColor();
            RegisterWidget(State, Label.c_str());
            if (ImGui::IsItemHovered())
            {
                const bool Refused = Layer < Library.Refusals.size() && !Library.Refusals[Layer].empty();
                ImGui::SetTooltip("%s%s%s", Library.Captions[Layer].c_str(), Refused ? " — " : "", Refused ? Library.Refusals[Layer].c_str() : "");
            }
            if (Pressed) Chosen = Name;
        }
    }
    return Chosen;
}

// 📝 Inspector card: what the selection shows, the library, Remove (Alt+M) and Add picture (a sphere image, e.g. a Blender
//    matcap). Face assignments whose names left the body are counted, never re-targeted.
void DrawAppearanceCard(DraftingWorkspaceState& State)
{
    if (!State.Document.Summary.PresentCondition) return;
    BeginCard("Appearance");
    const size_t Faces   = State.Document.Selection.Faces.size();
    const std::string Shown = SelectionMatcap(State);
    const size_t Layer   = DraftingMatcapLayer(State.Matcaps.Library, Shown);
    const std::string Caption = Shown == "mixed" ? std::string("mixed") : Layer < State.Matcaps.Library.Captions.size() ? State.Matcaps.Library.Captions[Layer] : Shown;
    const size_t Bodies  = State.Document.Selection.Bodies.size();
    const DraftingAppearance& Appearance = State.Document.Appearance;
    if (Faces) ImGui::Text("%zu face(s) · %s", Faces, Caption.c_str());
    else if (Bodies) ImGui::Text("%zu body(ies) · %s", Bodies, Caption.c_str());
    else ImGui::Text("%s · %s", State.Document.Summary.Bodies.size() > 1 ? "Default" : "Body", Caption.c_str());
    ImGui::SameLine();
    if (Appearance.BodyMatcaps.empty()) ImGui::TextDisabled("· %zu override(s)", Appearance.FaceMatcaps.size());
    else ImGui::TextDisabled("· %zu body + %zu face override(s)", Appearance.BodyMatcaps.size(), Appearance.FaceMatcaps.size());
    const std::string Chosen = DrawMatcapSwatches(State, "Matcap", 0, 26.0f, false);
    if (!Chosen.empty()) AssignMatcap(State, Chosen);
    const float Spacing = ImGui::GetStyle().ItemSpacing.x;
    const float Half    = std::floor(0.5f * (ImGui::GetContentRegionAvail().x - Spacing));
    if (PressButton(State, "Remove (Alt+M)###MatcapRemove", ImVec2(Half, 0))) RemoveMatcap(State);
    ImGui::SameLine();
    if (PressButton(State, "Picker (M)###MatcapPickerOpen", ImVec2(Half, 0))) State.Matcaps.PickerOpen = true;
    std::array<char, 260>& PicturePath = State.Matcaps.PicturePath;
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 60.0f - Spacing);
    ImGui::InputTextWithHint("###MatcapPath", "matcap picture (.png / .jpg)", PicturePath.data(), PicturePath.size());
    RegisterWidget(State, "###MatcapPath");
    ImGui::SameLine();
    if (PressButton(State, "Add###MatcapAdd", ImVec2(60, 0), PicturePath[0] != 0)) AddMatcapPicture(State, PicturePath.data());
    if (const size_t Dangling = CountDanglingMatcaps(State.Scene, State.Document.Appearance))
        ImGui::TextColored(Tint(RefusalColour), "%zu assignment(s) name faces / bodies no longer present (kept for undo)", Dangling);
    EndCard();
}

// 📝 Plasticity M (command:set-material): a floating picker over the viewport; a click applies and closes, Esc closes.
void DrawMatcapPicker(DraftingWorkspaceState& State)
{
    if (!State.Matcaps.PickerOpen) return;
    ImGui::SetNextWindowPos(ImVec2(State.ViewportRect[0] + 0.5f * State.ViewportRect[2], State.ViewportRect[1] + 56.0f), ImGuiCond_Always, ImVec2(0.5f, 0.0f));
    ImGui::SetNextWindowBgAlpha(0.96f);
    bool Open = true;
    const size_t Faces = State.Document.Selection.Faces.size();
    const std::string Title = (Faces ? "Set material · " + std::to_string(Faces) + " face(s)" : std::string("Set material · body")) + "###MatcapPicker";
    if (ImGui::Begin(Title.c_str(), &Open, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove))
    {
        RegisterWidget(State, "###MatcapPicker");
        const std::string Chosen = DrawMatcapSwatches(State, "PickMatcap", 7, 52.0f, true);
        ImGui::TextDisabled("Click applies · Esc closes · Alt+M removes");
        if (!Chosen.empty())
        {
            AssignMatcap(State, Chosen);
            Open = false;
        }
    }
    ImGui::End();
    State.Matcaps.PickerOpen = Open;
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
        Bevel.StagedVariable == Bevel.VariableEnabled && Bevel.StagedEndSize == Bevel.EndSize && Bevel.StagedReversed == Bevel.ReversedCondition &&
        Bevel.StagedCombination == Bevel.Combination) return;
    Bevel.StagedCombination = Bevel.Combination;
    Bevel.StagedSize     = Bevel.Size;
    Bevel.StagedShape    = Bevel.Shape;
    Bevel.PreviewVerb    = Bevel.Verb;
    Bevel.StagedVariable = Bevel.VariableEnabled;
    Bevel.StagedEndSize  = Bevel.EndSize;
    Bevel.StagedReversed = Bevel.ReversedCondition;
    Bevel.Preview     = std::make_unique<DraftingDocument>(CloneDraftingDocument(State.Document));
    const DraftingCommandOutcome Outcome = Bevel.Verb == DraftingSessionVerb::Profile ? ExtrudeDraftingSketch(*Bevel.Preview, Bevel.ProfileLabel, Bevel.Size, Bevel.Combination, BooleanTarget(State))
                                         : Bevel.Verb == DraftingSessionVerb::Extrude ? ChangeDraftingSelection(*Bevel.Preview, FaceOperationKind::Extrude, Bevel.Size)
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
    if (State.Bevel.Verb == DraftingSessionVerb::Profile)
    {
        const DraftingCommandOutcome Outcome = ExtrudeDraftingSketch(State.Document, State.Bevel.ProfileLabel, Size, State.Bevel.Combination, BooleanTarget(State));
        if (Outcome.AcceptedCondition) State.FocusedStep = {};
        Conclude(State, Outcome);
        return;
    }
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
    if (Bevel.Verb == DraftingSessionVerb::Extrude || Bevel.Verb == DraftingSessionVerb::Profile)
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
//                                                    DRAWING
//------------------------------------------------------------------------------------------------------------------------

constexpr double DrawingSnap      = 0.1;                                                     // [len] - Construction grid step
constexpr float  ClosingTolerance = 10.0f;                                                   // [px]  - Click on the first point closes

DraftingCameraFrame ViewportFrame(const DraftingWorkspaceState& State);

[[nodiscard]] Vector3 PlacePlanePoint(const OrthonormalFrame& Plane, Vector2 Point)
{
    return Plane.Origin + Plane.XAxis * Point.X + Plane.YAxis * Point.Y;
}

// World point → frame pixel (absolute); false behind the eye.
bool ProjectWorldPoint(const DraftingWorkspaceState& State, const DraftingCameraFrame& Frame, Vector3 Point, ImVec2& Pixel)
{
    const auto& Rect = State.ViewportRect;
    double Clip[4] = {};
    for (int Row = 0; Row < 4; ++Row)
        Clip[Row] = Frame.ClipFromWorld[0 * 4 + Row] * Point.X + Frame.ClipFromWorld[1 * 4 + Row] * Point.Y + Frame.ClipFromWorld[2 * 4 + Row] * Point.Z + Frame.ClipFromWorld[3 * 4 + Row];
    if (Clip[3] <= 1.0e-12) return false;
    Pixel = ImVec2(Rect[0] + float((Clip[0] / Clip[3] * 0.5 + 0.5) * Rect[2]), Rect[1] + float((Clip[1] / Clip[3] * 0.5 + 0.5) * Rect[3]));
    return true;
}

// Viewport pixel (local) → construction-plane coordinates; false when the ray runs parallel to the plane.
bool IntersectConstructionPlane(const DraftingCameraFrame& Frame, ImVec2 Local, int GridPlane, Vector2& Point)
{
    const OrthonormalFrame Plane  = DraftingSketchPlane(GridPlane);
    const DraftingRay      Ray    = CastDraftingRay(Frame, Local.x, Local.y);
    const Vector3          Origin{ Ray.Origin[0], Ray.Origin[1], Ray.Origin[2] }, Direction{ Ray.Direction[0], Ray.Direction[1], Ray.Direction[2] };
    const double           Facing = Dot(Direction, Plane.ZAxis);
    if (std::abs(Facing) < 1.0e-9) return false;
    const Vector3 Hit = Origin + Direction * (Dot(Plane.Origin - Origin, Plane.ZAxis) / Facing);
    Point = { Dot(Hit - Plane.Origin, Plane.XAxis), Dot(Hit - Plane.Origin, Plane.YAxis) };
    return true;
}

[[nodiscard]] DraftingDrawingRequest ComposeDrawingRequest(const DraftingDrawingSession& Drawing, bool PointerCondition, bool ClosedCondition)
{
    DraftingDrawingRequest Request;
    Request.Kind            = Drawing.Kind;
    Request.GridPlane       = Drawing.GridPlane;
    Request.Points          = Drawing.Points;
    Request.CentreCondition = Drawing.CentreCondition;
    Request.Form            = Drawing.Form;
    Request.ClosedCondition = ClosedCondition;
    if (PointerCondition && Drawing.PointerCondition) Request.Points.push_back(Drawing.Pointer);
    return Request;
}

// Points that finish the tool on their last click (0: open-ended chain).
[[nodiscard]] size_t DrawingClicks(DraftingDrawingKind Kind)
{
    switch (Kind)
    {
    case DraftingDrawingKind::Rectangle:
    case DraftingDrawingKind::Circle: return 2;
    case DraftingDrawingKind::Arc:
    case DraftingDrawingKind::Slot:   return 3;
    default:                          return 0;
    }
}

[[nodiscard]] std::string DrawingCaption(const DraftingDrawingSession& Drawing)
{
    std::string Caption = DraftingDrawingLabel(Drawing.Kind);
    if (Drawing.Kind == DraftingDrawingKind::Curve) Caption += std::string(" · ") + SketchSplineFormLabel(Drawing.Form);
    if (Drawing.Kind == DraftingDrawingKind::Rectangle) Caption += Drawing.CentreCondition ? " · centre" : " · corner";
    if (Drawing.Kind == DraftingDrawingKind::Circle) Caption += Drawing.CentreCondition ? " · centre" : " · two-point";
    return Caption;
}

void OpenDrawingSession(DraftingWorkspaceState& State, DraftingDrawingKind Kind)
{
    if (State.Bevel.ActiveCondition) return;
    const SketchSplineForm Form = State.Drawing.Form;
    State.Drawing                 = {};
    State.Drawing.ActiveCondition = true;
    State.Drawing.Kind            = Kind;
    State.Drawing.GridPlane       = std::clamp(State.Style.GridPlane, 0, 2);
    State.Drawing.CentreCondition = Kind == DraftingDrawingKind::Circle;                     // Plasticity center-circle; V → two-point
    State.Drawing.Form            = Form;
    static const char* Hints[] = { "click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point",
                                   "click two corners · Alt centre mode", "click centre and rim · V two-point mode", "click start, end, then a point on the arc",
                                   "click both cap centres, then the rim", "click points · 1 Hermite · 2 Bezier · 3 NURBS · 4 cubic spline · Enter / right-click finishes · Ctrl+Z drops a point" };
    static const char* Planes[] = { "XY", "XZ", "YZ" };
    Announce(State, std::string(DraftingDrawingLabel(Kind)) + " on plane " + Planes[State.Drawing.GridPlane] + ": " + Hints[int(Kind)] + " · Esc aborts");
}

void CancelDrawingSession(DraftingWorkspaceState& State)
{
    if (!State.Drawing.ActiveCondition) return;
    State.Drawing.ActiveCondition = false;
    Announce(State, std::string(DraftingDrawingLabel(State.Drawing.Kind)) + " aborted (document untouched)");
}

// Commits the collected points as a sketch step; the new sketch becomes the focused step (E extrudes it).
void FinishDrawingSession(DraftingWorkspaceState& State, bool ClosedCondition)
{
    DraftingDrawingSession& Drawing = State.Drawing;
    if (!Drawing.ActiveCondition) return;
    const DraftingDrawingRequest Request = ComposeDrawingRequest(Drawing, false, ClosedCondition);
    SketchModel Probe;
    const std::string Refusal = ComposeDraftingSketch(Request, Probe);
    if (!Refusal.empty())
    {
        Drawing.Refusal = Legible(Refusal);
        Announce(State, Drawing.Refusal);
        return;
    }
    Drawing.ActiveCondition = false;
    const size_t Before = State.Document.Tree.Steps.size();
    const DraftingCommandOutcome Outcome = DrawDraftingSketch(State.Document, Request);
    if (Outcome.AcceptedCondition && State.Document.Tree.Steps.size() > Before) State.FocusedStep = State.Document.Tree.Steps.back().Label;
    Conclude(State, Outcome);
}

void AppendDrawingPoint(DraftingWorkspaceState& State, Vector2 Point, bool ClosingCondition)
{
    DraftingDrawingSession& Drawing = State.Drawing;
    const bool Chain = DrawingClicks(Drawing.Kind) == 0;
    if (Chain && ClosingCondition) { FinishDrawingSession(State, true); return; }
    if (!Drawing.Points.empty() && Distance(Drawing.Points.back(), Point) <= 1.0e-12)
    {
        Announce(State, "Same point twice: ignored");
        return;
    }
    Drawing.Points.push_back(Point);
    Drawing.Refusal.clear();
    if (!Chain && Drawing.Points.size() >= DrawingClicks(Drawing.Kind)) FinishDrawingSession(State, false);
}

void RetractDrawingPoint(DraftingWorkspaceState& State)
{
    DraftingDrawingSession& Drawing = State.Drawing;
    if (Drawing.Points.empty()) { Announce(State, "No point to drop"); return; }
    Drawing.Points.pop_back();
    Drawing.Refusal.clear();
    Announce(State, Compose("Point dropped · %d left", int(Drawing.Points.size())));
}

// Plasticity E on a sketch region: the focused sketch step's profile, pointer-adjusted distance, Q/W/E/R Boolean.
void OpenSketchExtrudeSession(DraftingWorkspaceState& State)
{
    const std::string Label = State.FocusedStep;
    const double Remembered = State.Bevel.Verb == DraftingSessionVerb::Profile && State.Bevel.Size != 0.0 ? State.Bevel.Size : 0.5;
    State.Bevel                 = {};
    State.Bevel.ActiveCondition = true;
    State.Bevel.Verb            = DraftingSessionVerb::Profile;
    State.Bevel.ProfileLabel    = Label;
    State.Bevel.Combination     = State.Document.Summary.PresentCondition ? FeatureCombination::Union : FeatureCombination::New;
    State.Bevel.Size            = Remembered;
    WriteBevelText(State.Bevel);
    const auto Caption = State.Document.Captions.find(Label);
    Announce(State, "Extrude " + (Caption != State.Document.Captions.end() ? Caption->second : Label) +
                    " (E): move horizontally · Q union · W difference · E intersect · R new body · Enter / right-click applies · Esc aborts");
}

[[nodiscard]] bool FocusedSketchCondition(const DraftingWorkspaceState& State)
{
    if (State.FocusedStep.empty()) return false;
    for (const FeatureStep& Step : State.Document.Tree.Steps) if (Step.Label == State.FocusedStep) return Step.Kind == FeatureStepKind::Sketch;
    return false;
}

// Plasticity Q (command:boolean): needs two or more bodies picked in mode 4 (first = target, rest = tools).
void OpenBooleanSession(DraftingWorkspaceState& State)
{
    const size_t Picked = State.Document.Selection.Bodies.size();
    if (State.Document.Summary.Bodies.size() < 2)
    {
        Announce(State, "Boolean (Q) needs two bodies: construct one with R (New body) in Shift+C / V / X / Z");
        return;
    }
    if (Picked < 2)
    {
        Announce(State, "Boolean (Q): pick the target, then Shift-pick the tools in mode 4 (solid)");
        return;
    }
    State.Boolean.ActiveCondition = true;
    State.Boolean.Combination     = FeatureCombination::Union;
    Announce(State, Compose("Boolean: %s with %d tool(s) · Q union · W difference · E intersect · Enter applies",
                            DraftingBodyCaption(State.Document, State.Document.Selection.Bodies.front()).c_str(), int(Picked) - 1));
}

void ChooseBooleanCombination(DraftingWorkspaceState& State, FeatureCombination Combination)
{
    State.Boolean.Combination = Combination;
    Announce(State, std::string("Boolean: ") + FeatureCombinationLabel(Combination));
}

void ApplyBooleanSession(DraftingWorkspaceState& State)
{
    const std::vector<std::string> Picked = State.Document.Selection.Bodies;
    State.Boolean.ActiveCondition = false;
    if (Picked.size() < 2) { Announce(State, "Boolean: pick two or more bodies"); return; }
    const DraftingCommandOutcome Outcome = CombineDraftingBodies(State.Document, Picked.front(), std::vector<std::string>(Picked.begin() + 1, Picked.end()),
                                                                 State.Boolean.Combination);
    if (Outcome.AcceptedCondition)
    {
        ++State.Boolean.Commits;
        SelectDraftingName(State.Document, DraftingSelectionKind::Body, Picked.front(), false);
    }
    Conclude(State, Outcome);
}

// 💡 Plasticity C (command:cut): splits the picked body (mode 4; else the active body) by the focused sketch step or the
//    selected curve, a curve being projected along the view axis snapped to x / y / z (character line from the Top
//    view). The inside piece comes out picked in mode 4, so X deletes it right away (the loft → C → X panel flow).
void ApplyCut(DraftingWorkspaceState& State)
{
    DraftingCutRequest Request;
    if (!State.Document.Selection.Bodies.empty()) Request.Target = State.Document.Selection.Bodies.front();
    if (FocusedSketchCondition(State)) Request.Cutter = State.FocusedStep;
    else if (!State.Document.Selection.Curves.empty()) Request.Cutter = State.Document.Selection.Curves.front();
    else
    {
        Announce(State, "Cut (C): focus a sketch step or select a curve (the cutter), pick the body in mode 4 (else the active body)");
        return;
    }
    const DraftingCameraFrame Frame = ViewportFrame(State);
    int Axis = 0;
    for (int Index = 1; Index < 3; ++Index) if (std::fabs(Frame.Forward[Index]) > std::fabs(Frame.Forward[Axis])) Axis = Index;
    const double Sign = Frame.Forward[Axis] < 0.0 ? -1.0 : 1.0;
    Request.Direction = { Axis == 0 ? Sign : 0.0, Axis == 1 ? Sign : 0.0, Axis == 2 ? Sign : 0.0 };
    const DraftingCommandOutcome Outcome = CutDraftingBody(State.Document, Request);
    if (Outcome.AcceptedCondition)
    {
        State.FocusedStep = {};
        State.PickMode    = DraftingPickMode::Solid;
        SelectDraftingName(State.Document, DraftingSelectionKind::Body, State.Document.Tree.Steps.back().Label, false);
    }
    Conclude(State, Outcome);
}

void DrawBooleanCard(DraftingWorkspaceState& State)
{
    const std::vector<std::string>& Picked = State.Document.Selection.Bodies;
    BeginCard("Boolean");
    static const struct { const char* Label; FeatureCombination Combination; } Choices[] = {
        { "Union###BooleanUnion", FeatureCombination::Union }, { "Difference###BooleanDifference", FeatureCombination::Difference },
        { "Intersect###BooleanIntersect", FeatureCombination::Intersection } };
    const float Third = std::floor((ImGui::GetContentRegionAvail().x - 2.0f * ImGui::GetStyle().ItemSpacing.x) / 3.0f);
    for (int Index = 0; Index < 3; ++Index)
    {
        if (Index > 0) ImGui::SameLine();
        if (ToggleButton(State, Choices[Index].Label, State.Boolean.Combination == Choices[Index].Combination, ImVec2(Third, 0)))
            ChooseBooleanCombination(State, Choices[Index].Combination);
    }
    if (!Picked.empty())
    {
        ImGui::Text("Target  %s", DraftingBodyCaption(State.Document, Picked.front()).c_str());
        for (size_t Index = 1; Index < Picked.size(); ++Index)
            ImGui::TextDisabled("Tool %d  %s", int(Index), DraftingBodyCaption(State.Document, Picked[Index]).c_str());
    }
    ImGui::PushStyleColor(ImGuiCol_Text, Tint(MutedColour));
    ImGui::TextWrapped("Q union · W difference · E intersect · Enter applies · Esc cancels. The tools are consumed; the target keeps its key, matcap and history.");
    ImGui::PopStyleColor();
    if (PressButton(State, "Apply###BooleanApply", ImVec2(118, 0))) ApplyBooleanSession(State);
    ImGui::SameLine();
    if (PressButton(State, "Cancel###BooleanCancel", ImVec2(118, 0))) { State.Boolean.ActiveCondition = false; Announce(State, "Boolean cancelled"); }
    EndCard();
}

void ChooseProfileCombination(DraftingWorkspaceState& State, FeatureCombination Combination)
{
    State.Bevel.Combination = Combination;
    Announce(State, std::string("Extrude Boolean: ") + (Combination == FeatureCombination::New ? "New body" : FeatureCombinationLabel(Combination)));
}

// Polyline of every sketch step on its plane (solved positions), the focused one highlighted, used ones dimmed.
void DrawSketchOverlays(DraftingWorkspaceState& State, ImDrawList* Canvas, const DraftingCameraFrame& Frame)
{
    const DraftingDocument& Document = State.Document;
    for (const FeatureStep& Step : Document.Tree.Steps)
    {
        if (Step.Kind != FeatureStepKind::Sketch) continue;
        const auto Solved = Document.Regeneration.Sketches.find(Step.Label);
        const SketchModel& Sketch = Solved != Document.Regeneration.Sketches.end() ? Solved->second : Step.Sketch;
        const bool  Focused  = Step.Label == State.FocusedStep;
        const bool  Consumed = DraftingSketchConsumedCondition(Document, Step.Label);
        const ImU32 Colour   = Focused ? IM_COL32(0xff, 0xd2, 0x4d, 0xff) : Consumed ? IM_COL32(0x7f, 0x9c, 0xbf, 0x90) : IM_COL32(0x9f, 0xd8, 0xff, 0xff);
        OrthonormalFrame Plane = Step.Plane;
        Plane.Origin = Plane.Origin + Plane.ZAxis * EvaluateFeatureValue(Document.Tree, Step.Elevation);
        for (const SketchCurve& Curve : Sketch.Curves)
        {
            std::vector<ImVec2> Pixels;
            for (const Vector2& Sample : SampleSketchCurve(Sketch, Curve))
            {
                ImVec2 Pixel;
                if (ProjectWorldPoint(State, Frame, PlacePlanePoint(Plane, Sample), Pixel)) Pixels.push_back(Pixel);
            }
            if (Pixels.size() >= 2) Canvas->AddPolyline(Pixels.data(), int(Pixels.size()), Colour, Curve.Construction ? 1.0f : Focused ? 2.5f : 1.8f);
        }
    }
}

// The drawing in progress: the shape through the pointer, its clicks, a spline's control polygon.
void DrawDrawingPreview(DraftingWorkspaceState& State, ImDrawList* Canvas, const DraftingCameraFrame& Frame)
{
    DraftingDrawingSession& Drawing = State.Drawing;
    const OrthonormalFrame  Plane   = DraftingSketchPlane(Drawing.GridPlane);
    auto Pixel = [&](Vector2 Point, ImVec2& Out) { return ProjectWorldPoint(State, Frame, PlacePlanePoint(Plane, Point), Out); };
    SketchModel Sketch;
    const std::string Refusal = ComposeDraftingSketch(ComposeDrawingRequest(Drawing, true, false), Sketch);
    if (Refusal.empty())
    {
        for (const SketchCurve& Curve : Sketch.Curves)
        {
            std::vector<ImVec2> Pixels;
            for (const Vector2& Sample : SampleSketchCurve(Sketch, Curve)) { ImVec2 Out; if (Pixel(Sample, Out)) Pixels.push_back(Out); }
            if (Pixels.size() >= 2) Canvas->AddPolyline(Pixels.data(), int(Pixels.size()), ActiveColour, 2.0f);
        }
    }
    std::vector<Vector2> Marks = Drawing.Points;
    if (Drawing.PointerCondition) Marks.push_back(Drawing.Pointer);
    if (Drawing.Kind == DraftingDrawingKind::Curve && Drawing.Form != SketchSplineForm::Hermite && Drawing.Form != SketchSplineForm::CubicSpline && Marks.size() >= 2)
    {
        std::vector<ImVec2> Polygon;
        for (const Vector2& Mark : Marks) { ImVec2 Out; if (Pixel(Mark, Out)) Polygon.push_back(Out); }
        if (Polygon.size() >= 2) Canvas->AddPolyline(Polygon.data(), int(Polygon.size()), IM_COL32(0xaa, 0xaa, 0xaa, 0x90), 1.0f);
    }
    for (size_t Index = 0; Index < Marks.size(); ++Index)
    {
        ImVec2 Out;
        if (!Pixel(Marks[Index], Out)) continue;
        const bool Live = Index == Drawing.Points.size();
        Canvas->AddCircleFilled(Out, Live ? 4.5f : 3.5f, Live ? IM_COL32(0xff, 0xff, 0xff, 0xff) : ActiveColour);
    }
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    VEHICLE CURVES
//------------------------------------------------------------------------------------------------------------------------

constexpr int   CurveSamples = 96;                                                           // [-]  - Polyline points per fitted curve
constexpr float CurveReach   = 6.0f;                                                         // [px] - Click distance that picks a curve
constexpr float PoleReach    = 8.0f;                                                         // [px] - Press distance that grabs a pole

// Fitted SpaceCurve of a step (nullptr when the step is refused or not a curve).
const CurveFitOutcome* FindFittedCurve(const DraftingDocument& Document, const std::string& Label)
{
    const auto Found = Document.Regeneration.Curves.find(Label);
    return Found != Document.Regeneration.Curves.end() && Found->second.Curve.Definition ? &Found->second : nullptr;
}

std::string CaptionOf(const DraftingDocument& Document, const std::string& Label)
{
    const auto Found = Document.Captions.find(Label);
    return Found != Document.Captions.end() ? Found->second : Label;
}

// 1-based loft order of a selected curve, 0 when unselected.
int CurveOrder(const DraftingWorkspaceState& State, const std::string& Label)
{
    const std::vector<std::string>& Curves = State.Document.Selection.Curves;
    const auto Found = std::find(Curves.begin(), Curves.end(), Label);
    return Found == Curves.end() ? 0 : int(Found - Curves.begin()) + 1;
}

// Poles show on the selected curves, and on every curve in mode 1.
bool PolesShownCondition(const DraftingWorkspaceState& State, const std::string& Label)
{
    return State.PickMode == DraftingPickMode::ControlPoint || CurveOrder(State, Label) > 0;
}

bool CurveConsumedCondition(const DraftingDocument& Document, const std::string& Label)
{
    for (const FeatureStep& Step : Document.Tree.Steps)
        if ((Step.Kind == FeatureStepKind::Loft || Step.Kind == FeatureStepKind::Network) && std::find(Step.Sections.begin(), Step.Sections.end(), Label) != Step.Sections.end()) return true;
    return false;
}

// The definition as displayed: during a pole drag the grabbed pole follows the pointer.
NurbsCurveDefinition DisplayedDefinition(const DraftingWorkspaceState& State, const std::string& Label, const NurbsCurveDefinition& Definition)
{
    NurbsCurveDefinition Shown = Definition;
    const DraftingPoleDrag& Drag = State.PoleDrag;
    if (Drag.ActiveCondition && Drag.Label == Label && Drag.Pole >= 0 && Drag.Pole < int(Shown.Poles.size())) Shown.Poles[Drag.Pole] = Drag.Position;
    return Shown;
}

std::vector<ImVec2> ProjectFittedCurve(const DraftingWorkspaceState& State, const DraftingCameraFrame& Frame, const NurbsCurveDefinition& Definition)
{
    const NurbsDomain   Domain = MeasureNurbsCurveDomain(Definition);
    std::vector<ImVec2> Pixels;
    for (int Index = 0; Index <= CurveSamples; ++Index)
    {
        ImVec2 Pixel;
        const double τ = Domain.Low + (Domain.High - Domain.Low) * Index / CurveSamples;
        if (ProjectWorldPoint(State, Frame, EvaluateNurbsCurve(Definition, τ).Position, Pixel)) Pixels.push_back(Pixel);
    }
    return Pixels;
}

float DistanceToPolyline(const std::vector<ImVec2>& Pixels, ImVec2 Pointer)
{
    float Nearest = 1.0e9f;
    for (size_t Index = 1; Index < Pixels.size(); ++Index)
    {
        const ImVec2 Start = Pixels[Index - 1], Span(Pixels[Index].x - Start.x, Pixels[Index].y - Start.y);
        const float  Square = Span.x * Span.x + Span.y * Span.y;
        const float  Along  = Square > 0.0f ? std::clamp(((Pointer.x - Start.x) * Span.x + (Pointer.y - Start.y) * Span.y) / Square, 0.0f, 1.0f) : 0.0f;
        Nearest = std::min(Nearest, std::hypot(Pointer.x - Start.x - Along * Span.x, Pointer.y - Start.y - Along * Span.y));
    }
    return Nearest;
}

// Nearest curve within CurveReach of the pointer (frame pixels); empty when none.
std::string FindCurveNear(const DraftingWorkspaceState& State, const DraftingCameraFrame& Frame, ImVec2 Pointer)
{
    std::string Nearest;
    float       Best = CurveReach;
    for (const FeatureStep& Step : State.Document.Tree.Steps)
    {
        const CurveFitOutcome* Fit = Step.Kind == FeatureStepKind::SpaceCurve ? FindFittedCurve(State.Document, Step.Label) : nullptr;
        if (!Fit) continue;
        const float Distance = DistanceToPolyline(ProjectFittedCurve(State, Frame, *Fit->Curve.Definition), Pointer);
        if (Distance <= Best) { Best = Distance; Nearest = Step.Label; }
    }
    return Nearest;
}

// Nearest shown pole within PoleReach of the pointer.
bool FindPoleNear(const DraftingWorkspaceState& State, const DraftingCameraFrame& Frame, ImVec2 Pointer, std::string& Label, int& Pole)
{
    float Best = PoleReach;
    Pole = -1;
    for (const auto& [Candidate, Fit] : State.Document.Regeneration.Curves)
    {
        if (!Fit.Curve.Definition || !PolesShownCondition(State, Candidate)) continue;
        const std::vector<Vector3>& Poles = Fit.Curve.Definition->Poles;
        for (int Index = 0; Index < int(Poles.size()); ++Index)
        {
            ImVec2 Pixel;
            if (!ProjectWorldPoint(State, Frame, Poles[Index], Pixel)) continue;
            const float Distance = std::hypot(Pointer.x - Pixel.x, Pointer.y - Pixel.y);
            if (Distance <= Best) { Best = Distance; Label = Candidate; Pole = Index; }
        }
    }
    return Pole >= 0;
}

// 📝 Curvature comb (Plasticity / Alias style): teeth along −N with length κ·scale, N toward the centre of curvature,
//    κ = |C′ × C″| / |C′|³; the scale makes the longest tooth 15 % of the chord so every curve reads at any zoom.
void DrawCurvatureComb(const DraftingWorkspaceState& State, ImDrawList* Canvas, const DraftingCameraFrame& Frame, const NurbsCurveDefinition& Definition)
{
    constexpr int       Teeth  = 72;
    const NurbsDomain   Domain = MeasureNurbsCurveDomain(Definition);
    std::vector<Vector3> Bases, Tips;
    std::vector<double>  Curvatures;
    double               Peak = 0.0;
    for (int Index = 0; Index <= Teeth; ++Index)
    {
        const NurbsCurveDerivatives Sample = EvaluateNurbsCurveDerivatives(Definition, Domain.Low + (Domain.High - Domain.Low) * Index / Teeth);
        const Vector3 Binormal = Cross(Sample.First, Sample.Second);
        const double  Speed    = Length(Sample.First);
        const double  κ        = Speed > 1.0e-12 ? Length(Binormal) / (Speed * Speed * Speed) : 0.0;
        const Vector3 Inward   = Length(Binormal) > 1.0e-14 ? Normalize(Cross(Binormal, Sample.First)) : Vector3{};
        Bases.push_back(Sample.Position);
        Tips.push_back(Inward);
        Curvatures.push_back(κ);
        Peak = std::max(Peak, κ);
    }
    const double Chord = Distance(Bases.front(), Bases.back()) > 1.0e-9 ? Distance(Bases.front(), Bases.back()) : MeasureNurbsCurveScale(Definition);
    if (Peak <= 1.0e-12 || Chord <= 1.0e-12) return;
    const double        Scale = 0.15 * Chord / Peak;
    std::vector<ImVec2> Envelope;
    for (size_t Index = 0; Index < Bases.size(); ++Index)
    {
        ImVec2 Base, Tip;
        if (!ProjectWorldPoint(State, Frame, Bases[Index], Base) || !ProjectWorldPoint(State, Frame, Bases[Index] - Tips[Index] * (Curvatures[Index] * Scale), Tip)) continue;
        Canvas->AddLine(Base, Tip, IM_COL32(0xff, 0x8a, 0x5c, 0x70), 1.0f);
        Envelope.push_back(Tip);
    }
    if (Envelope.size() >= 2) Canvas->AddPolyline(Envelope.data(), int(Envelope.size()), IM_COL32(0xff, 0x8a, 0x5c, 0xd0), 1.4f);
}

// Every fitted SpaceCurve: polyline (selected: blue with its loft order, lofted: dimmed), combs on selected curves,
// poles and control polygon where shown, the dragged pole live.
void DrawCurveOverlays(DraftingWorkspaceState& State, ImDrawList* Canvas, const DraftingCameraFrame& Frame)
{
    const DraftingDocument& Document = State.Document;
    for (const FeatureStep& Step : Document.Tree.Steps)
    {
        const CurveFitOutcome* Fit = Step.Kind == FeatureStepKind::SpaceCurve ? FindFittedCurve(Document, Step.Label) : nullptr;
        if (!Fit) continue;
        const NurbsCurveDefinition Shown  = DisplayedDefinition(State, Step.Label, *Fit->Curve.Definition);
        const int                  Order  = CurveOrder(State, Step.Label);
        const bool                 Moving = State.PoleDrag.ActiveCondition && State.PoleDrag.Label == Step.Label;
        const std::vector<ImVec2>  Pixels = ProjectFittedCurve(State, Frame, Shown);
        const ImU32 Colour = Moving ? ActiveColour : Order > 0 ? SelectedColour : CurveConsumedCondition(Document, Step.Label) ? IM_COL32(0x7f, 0x9c, 0xbf, 0xb0) : IM_COL32(0x9f, 0xd8, 0xff, 0xff);
        if (Order > 0 && State.CombsEnabled) DrawCurvatureComb(State, Canvas, Frame, Shown);
        if (Pixels.size() >= 2) Canvas->AddPolyline(Pixels.data(), int(Pixels.size()), Colour, Order > 0 || Moving ? 2.6f : 1.6f);
        if (Order > 0 && !Pixels.empty())
        {
            const std::string Badge = std::to_string(Order);
            Canvas->AddCircleFilled(ImVec2(Pixels.front().x - 12, Pixels.front().y - 12), 8.0f, SelectedColour);
            Canvas->AddText(ImVec2(Pixels.front().x - 15, Pixels.front().y - 20), IM_COL32_WHITE, Badge.c_str());
        }
        if (!PolesShownCondition(State, Step.Label)) continue;
        std::vector<ImVec2> Polygon;
        for (const Vector3& Pole : Shown.Poles) { ImVec2 Pixel; if (ProjectWorldPoint(State, Frame, Pole, Pixel)) Polygon.push_back(Pixel); }
        if (Polygon.size() >= 2) Canvas->AddPolyline(Polygon.data(), int(Polygon.size()), IM_COL32(0xc8, 0xc8, 0xc8, 0x80), 1.0f);
        for (int Index = 0; Index < int(Shown.Poles.size()); ++Index)
        {
            ImVec2 Pixel;
            if (!ProjectWorldPoint(State, Frame, Shown.Poles[Index], Pixel)) continue;
            const bool  Grabbed = Moving && State.PoleDrag.Pole == Index;
            const bool  Hover   = !State.PoleDrag.ActiveCondition && State.PoleDrag.HoverLabel == Step.Label && State.PoleDrag.HoverPole == Index;
            const float Half    = Grabbed || Hover ? 5.0f : 3.5f;
            Canvas->AddRectFilled(ImVec2(Pixel.x - Half, Pixel.y - Half), ImVec2(Pixel.x + Half, Pixel.y + Half), Grabbed ? ActiveColour : Hover ? IM_COL32_WHITE : IM_COL32(0xff, 0xd2, 0x4d, 0xff));
            Canvas->AddRect(ImVec2(Pixel.x - Half, Pixel.y - Half), ImVec2(Pixel.x + Half, Pixel.y + Half), IM_COL32(0x10, 0x10, 0x10, 0xff));
        }
    }
}

// Zebra (V8): stripe count to the surface shader, and a finer tessellation so interpolated normals follow the surface.
void ToggleZebra(DraftingWorkspaceState& State)
{
    State.ZebraEnabled       = !State.ZebraEnabled;
    State.Style.ZebraStripes = State.ZebraEnabled ? ZebraStripeCount : 0.0f;
    RefreshDraftingScene(State, false);
    Announce(State, State.ZebraEnabled ? Compose("Zebra on · %d stripes · %d triangles", int(ZebraStripeCount), int(State.Scene.Corners.size() / 3)) : std::string("Zebra off"));
}

// Deviation colour: green at 0, yellow at half the limit, red at the limit and beyond.
ImU32 DeviationColour(double Distance)
{
    const double Fraction = std::clamp(Distance / DeviationLimit, 0.0, 1.0);
    const int    Red      = Fraction < 0.5 ? int(510.0 * Fraction) : 255;
    const int    Green    = Fraction < 0.5 ? 220 : int(220.0 * (1.0 - Fraction) * 2.0);
    return IM_COL32(Red, Green, 0x40, 0xff);
}

// 💡 V8 deviation map: every imported point gets a dot, and a needle along point → foot magnified so the worst needle
//    is 5 % of the scene radius; legend bottom-left (colour scale 0 … 0.2 mm, measured worst / mean, magnification).
void DrawDeviationOverlay(DraftingWorkspaceState& State, ImDrawList* Canvas, const DraftingCameraFrame& Frame)
{
    if (State.Deviation.Revision != State.Document.Revision || (State.Deviation.Needles.empty() && !State.Document.Regeneration.Panels.empty()))
        State.Deviation = MeasureDraftingDeviation(State.Document);
    const DraftingDeviationMap& Map = State.Deviation;
    if (Map.Needles.empty()) return;
    const double Gain = Map.Worst > 0.0 ? 0.05 * State.Camera.SceneRadius / Map.Worst : 1.0;
    for (const DraftingDeviationNeedle& Needle : Map.Needles)
    {
        ImVec2 Start, Tip;
        if (!ProjectWorldPoint(State, Frame, Needle.Point, Start)) continue;
        const ImU32 Colour = DeviationColour(Needle.Distance);
        if (ProjectWorldPoint(State, Frame, Needle.Point + (Needle.Foot - Needle.Point) * Gain, Tip)) Canvas->AddLine(Start, Tip, Colour, 1.6f);
        Canvas->AddCircleFilled(Start, 2.4f, Colour);
    }
    (void)Frame;
    const ImVec2 Corner(State.ViewportRect[0] + 16, State.ViewportRect[1] + State.ViewportRect[3] - 104);
    Canvas->AddRectFilled(Corner, ImVec2(Corner.x + 300, Corner.y + 62), IM_COL32(0x10, 0x13, 0x19, 0xe0), 6.0f);
    for (int Step = 0; Step < 100; ++Step)
    {
        const float Left = Corner.x + 10 + Step * 2.0f;
        Canvas->AddRectFilled(ImVec2(Left, Corner.y + 10), ImVec2(Left + 2.0f, Corner.y + 22), DeviationColour(DeviationLimit * Step / 99.0));
    }
    Canvas->AddText(ImVec2(Corner.x + 8, Corner.y + 24), IM_COL32(0xc8, 0xd0, 0xdc, 0xff), "0");
    Canvas->AddText(ImVec2(Corner.x + 92, Corner.y + 24), IM_COL32(0xc8, 0xd0, 0xdc, 0xff), "0.1");
    Canvas->AddText(ImVec2(Corner.x + 178, Corner.y + 24), IM_COL32(0xc8, 0xd0, 0xdc, 0xff), "0.2 mm");
    const std::string Summary = Compose("worst %.3f mm · mean %.3f mm · %d points · needles x%.0f", Map.Worst * 1000.0, Map.Mean * 1000.0, int(Map.Needles.size()), Gain);
    Canvas->AddText(ImVec2(Corner.x + 8, Corner.y + 42), IM_COL32(0xe8, 0xec, 0xf2, 0xff), Summary.c_str());
}

// Import curves… / --curves: VehicleCurves.toml → one SpaceCurve step per curve (one undo entry), framed.
void FrameSelection(DraftingWorkspaceState& State);

void ImportCurves(DraftingWorkspaceState& State, const std::string& Path)
{
    const VehicleCurveFile       File    = ReadVehicleCurves(Path);
    const DraftingCommandOutcome Outcome = ImportDraftingCurves(State.Document, File);
    Conclude(State, Outcome);
    if (!Outcome.AcceptedCondition) return;
    double Worst = 0.0;
    int    Poles = 0;
    for (const auto& [Label, Fit] : State.Document.Regeneration.Curves) { Worst = std::max(Worst, Fit.WorstDeviation); Poles += int(Fit.Curve.Definition->Poles.size()); }
    FrameSelection(State);
    Announce(State, Compose("Imported %d curve(s) from %s · %d poles · worst fit %.3f mm", int(File.Curves.size()), Path.c_str(), Poles, Worst * 1000.0));
}

// L (Plasticity loft): the selected curves, in pick order, become a panel (crossing curves: a Gordon network).
void LoftSelectedCurves(DraftingWorkspaceState& State)
{
    const std::vector<std::string> Curves = State.Document.Selection.Curves;
    const DraftingCommandOutcome   Outcome = LoftDraftingCurves(State.Document, Curves, State.LoftContinuity);
    Conclude(State, Outcome);
    if (!Outcome.AcceptedCondition) return;
    const bool NetworkCondition = !State.Document.Tree.Steps.empty() && State.Document.Tree.Steps.back().Kind == FeatureStepKind::Network;
    const char* Order = State.LoftContinuity == 2 ? " · G2" : State.LoftContinuity == 1 ? " · G1" : "";
    Announce(State, Compose("%s %d curve(s) → panel%s (%s) · %d panel(s) · %.0f ms", NetworkCondition ? "Network of" : "Lofted", int(Curves.size()), NetworkCondition ? "" : Order,
                            State.Document.Summary.SoundCondition ? "sound" : "audit findings", int(State.Document.Regeneration.Panels.size()), Outcome.Milliseconds));
}

// Alt+X (Plasticity mirror, gizmo x): every panel gets its image through x = 0; halves sew along the plane.
void MirrorPanels(DraftingWorkspaceState& State)
{
    const DraftingCommandOutcome Outcome = MirrorDraftingPanels(State.Document, 0);
    Conclude(State, Outcome);
    if (Outcome.AcceptedCondition) Announce(State, Compose("Mirrored through x = 0 → %d panel(s), %s · %.0f ms", int(State.Document.Regeneration.Panels.size()),
                                                           State.Document.Summary.SoundCondition ? "sound" : "audit findings", Outcome.Milliseconds));
}

// Close: plane caps on the planar rim loops → solid.
void ClosePanels(DraftingWorkspaceState& State)
{
    const DraftingCommandOutcome Outcome = CloseDraftingPanels(State.Document);
    Conclude(State, Outcome);
    if (Outcome.AcceptedCondition) Announce(State, Compose("Closed → solid · volume %.5f m³ · %s · %.0f ms", State.Document.Summary.Volume, State.Document.Summary.SoundCondition ? "sound" : "audit findings",
                                                           Outcome.Milliseconds));
}

void PickCurve(DraftingWorkspaceState& State, const std::string& Label, DraftingPickIntent Intent)
{
    const bool        Member  = CurveOrder(State, Label) > 0;
    const std::string Caption = CaptionOf(State.Document, Label);
    switch (Intent)
    {
    case DraftingPickIntent::Replace:
        SelectDraftingCurve(State.Document, Label, false);
        State.FocusedStep = Label;
        break;
    case DraftingPickIntent::Add:
    case DraftingPickIntent::Extend:
        if (!Member) SelectDraftingCurve(State.Document, Label, true);
        break;
    case DraftingPickIntent::Remove:
        if (Member) SelectDraftingCurve(State.Document, Label, true);
        break;
    }
    const CurveFitOutcome* Fit = FindFittedCurve(State.Document, Label);
    Announce(State, Compose("%s curve %s · %d poles · fit %.3f mm · %d selected", Intent == DraftingPickIntent::Remove ? "Removed" : "Selected", Caption.c_str(),
                            Fit ? int(Fit->Curve.Definition->Poles.size()) : 0, Fit ? Fit->WorstDeviation * 1000.0 : 0.0, int(State.Document.Selection.Curves.size())));
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
    if (State.PickMode == DraftingPickMode::ControlPoint || State.PickMode == DraftingPickMode::Edge || State.PickMode == DraftingPickMode::Any)
    {
        // Curves are overlays: within CurveReach they win over the face / edge underneath.
        const std::string Curve = FindCurveNear(State, Frame, ImVec2(State.ViewportRect[0] + Local.x, State.ViewportRect[1] + Local.y));
        if (!Curve.empty())
        {
            PickCurve(State, Curve, Intent);
            return;
        }
        if (State.PickMode == DraftingPickMode::ControlPoint)
        {
            if (Intent == DraftingPickIntent::Replace && !State.Document.Selection.Curves.empty())
            {
                ClearDraftingSelection(State.Document);
                Announce(State, "Selection cleared");
            }
            return;
        }
    }
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
        if (Intent == DraftingPickIntent::Replace && (!State.Document.Selection.Faces.empty() || !State.Document.Selection.Edges.empty()
                                                      || !State.Document.Selection.Curves.empty() || !State.Document.Selection.Bodies.empty()))
        {
            ClearDraftingSelection(State.Document);
            Announce(State, "Selection cleared");
        }
        return;
    }
    if (State.PickMode == DraftingPickMode::Solid)
    {
        // Mode 4 (solid): the body under the pointer, by its key. Shift adds (pick order = Boolean order), Ctrl removes.
        const uint32_t Index   = (Identity & ~IdentityEdgeBit) - 1u;
        const uint32_t BodyAt  = (Identity & IdentityEdgeBit) ? (Index < State.Scene.Edges.size() ? State.Scene.Edges[Index].Body : ~0u)
                                                              : (Index < State.Scene.Faces.size() ? State.Scene.Faces[Index].Body : ~0u);
        if (BodyAt >= State.Scene.Bodies.size()) return;
        const DraftingSceneBody&        Body     = State.Scene.Bodies[BodyAt];
        const std::vector<std::string>& Picked   = State.Document.Selection.Bodies;
        const bool                      Member   = std::find(Picked.begin(), Picked.end(), Body.Key) != Picked.end();
        if (Intent == DraftingPickIntent::Replace)
        {
            SelectDraftingName(State.Document, DraftingSelectionKind::Body, Body.Key, false);
            Announce(State, Compose("Selected %s (%d faces)", Body.Caption.c_str(), int(Body.FaceCount)));
        }
        else if (Intent == DraftingPickIntent::Add)
        {
            if (!Member) SelectDraftingName(State.Document, DraftingSelectionKind::Body, Body.Key, true);
            Announce(State, Compose("Added %s · %d bodies picked", Body.Caption.c_str(), int(State.Document.Selection.Bodies.size())));
        }
        else
        {
            if (Member) SelectDraftingName(State.Document, DraftingSelectionKind::Body, Body.Key, true);
            Announce(State, "Removed " + Body.Caption);
        }
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

// Curves wholly inside the screen box, appended in feature-tree order (the import order of sections).
void SelectCurvesInBox(DraftingWorkspaceState& State, const DraftingCameraFrame& Frame, ImVec2 Corner, ImVec2 Opposite, DraftingPickIntent Intent)
{
    const ImVec2 Low(std::min(Corner.x, Opposite.x), std::min(Corner.y, Opposite.y));
    const ImVec2 High(std::max(Corner.x, Opposite.x), std::max(Corner.y, Opposite.y));
    if (Intent == DraftingPickIntent::Replace) ClearDraftingSelection(State.Document);
    int Count = 0;
    for (const FeatureStep& Step : State.Document.Tree.Steps)
    {
        const CurveFitOutcome* Fit = Step.Kind == FeatureStepKind::SpaceCurve ? FindFittedCurve(State.Document, Step.Label) : nullptr;
        if (!Fit) continue;
        const std::vector<ImVec2> Pixels = ProjectFittedCurve(State, Frame, *Fit->Curve.Definition);
        if (int(Pixels.size()) != CurveSamples + 1) continue;                               // Partly off screen / behind the eye
        const bool Inside = std::all_of(Pixels.begin(), Pixels.end(), [&](ImVec2 Pixel) { return Pixel.x >= Low.x && Pixel.x <= High.x && Pixel.y >= Low.y && Pixel.y <= High.y; });
        if (!Inside) continue;
        const bool Member = CurveOrder(State, Step.Label) > 0;
        if (Intent == DraftingPickIntent::Remove ? Member : !Member) SelectDraftingCurve(State.Document, Step.Label, true);
        ++Count;
    }
    ++State.Box.Commits;
    Announce(State, Compose("Box %s %d curve(s) · %d selected", Intent == DraftingPickIntent::Remove ? "removed" : "selected", Count, int(State.Document.Selection.Curves.size())));
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
    ImGui::SameLine();
    if (PressButton(State, "Import curves…###HeaderImport")) ImGui::OpenPopup("Import curves");
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

    for (const char* Title : { "Open document", "Save document", "Import curves" })
    {
        const bool OpenCondition   = std::strcmp(Title, "Open document") == 0;
        const bool ImportCondition = std::strcmp(Title, "Import curves") == 0;
        if (ImGui::IsPopupOpen(Title) && State.PathText[0] == 0)
            std::snprintf(State.PathText.data(), State.PathText.size(), "%s",
                          (ImportCondition ? State.Options.CurvesPath : State.DocumentPath.empty() ? State.Options.DocumentPath : State.DocumentPath).c_str());
        if (ImGui::BeginPopupModal(Title, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::TextUnformatted(ImportCondition ? "Import VehicleCurves.toml (Blender ExportVehicleCurves add-on): each curve is fitted within 0.2 mm"
                                   : OpenCondition ? "Open a .drafting.toml document" : "Save the document as .drafting.toml");
            ImGui::SetNextItemWidth(420);
            const bool Entered = ImGui::InputText("##Path", State.PathText.data(), State.PathText.size(), ImGuiInputTextFlags_EnterReturnsTrue);
            RegisterWidget(State, "PathField");
            if (PressButton(State, ImportCondition ? "Import###PathImport" : OpenCondition ? "Open###PathOpen" : "Save###PathSave", ImVec2(120, 0)) || Entered)
            {
                if (ImportCondition) ImportCurves(State, State.PathText.data());
                else if (OpenCondition) OpenDocument(State, State.PathText.data(), true);
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
                                                                            { FeatureCombination::Intersection, "Intersection###CombinationIntersection" },
                                                                            { FeatureCombination::Separate, "New body###CombinationSeparate" } };
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
    {
        Field("Radius [m]###ConstructRadius", Form.Radius);
        Field("Height [m]###ConstructHeight", Form.Height);
        const char* Axes[3] = { "Axis x###ConstructAxisX", "Axis y###ConstructAxisY", "Axis z###ConstructAxisZ" };
        for (int Axis = 0; Axis < 3; ++Axis)
        {
            if (ToggleButton(State, Axes[Axis], Form.Axis == Axis, ImVec2(70, 0))) Form.Axis = Axis;
            if (Axis < 2) ImGui::SameLine(0, 4);
        }
        break;
    }
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
        if (!FirstBody && ImGui::IsKeyPressed(ImGuiKey_R, false)) Form.Combination = FeatureCombination::Separate;
        CreateCondition = ImGui::IsKeyPressed(ImGuiKey_Enter, false) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, false);
        if (ImGui::IsKeyPressed(ImGuiKey_Escape, false))
        {
            ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
            return;
        }
    }
    ImGui::PushStyleColor(ImGuiCol_Text, Tint(MutedColour));
    ImGui::TextUnformatted("Q union · W difference · E intersect · R new body · Enter create · Esc cancel");
    ImGui::PopStyleColor();
    ImGui::Spacing();
    if (PressButton(State, "Create###ConstructCreate", ImVec2(130, 0)) || CreateCondition)
    {
        DraftingConstructRequest Request = Form;
        if (FirstBody) Request.Combination = FeatureCombination::New;
        Request.Target = Request.Combination == FeatureCombination::Separate || FirstBody ? std::string{} : BooleanTarget(State);
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
    DrawStatTile("Volume", !Summary.PresentCondition ? std::string("-") : Summary.SheetCondition ? std::string("Open sheet") : Compose("%.4f m³", Summary.Volume), Tile);
    ImGui::Spacing();

    // Bodies (creation order): click picks (Shift adds, pick order = Boolean order), like mode 4 in the viewport.
    if (Summary.Bodies.size() > 1)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, Tint(MutedColour));
        ImGui::TextUnformatted("Bodies");
        ImGui::PopStyleColor();
        for (const DraftingBodyFigures& Figures : Summary.Bodies)
        {
            const std::vector<std::string>& Picked = State.Document.Selection.Bodies;
            const auto Found = std::find(Picked.begin(), Picked.end(), Figures.Key);
            const std::string Order = Found == Picked.end() ? std::string("   ") : Compose("%d  ", int(Found - Picked.begin()) + 1);
            const std::string Row = Order + Figures.Caption + "  · " + DraftingBodyMatcap(State.Document.Appearance, Figures.Key) + "###Body:" + Figures.Key;
            ImGui::PushStyleColor(ImGuiCol_Header, Tint(SelectedColour));
            if (ImGui::Selectable(Row.c_str(), Found != Picked.end()))
            {
                SelectDraftingName(State.Document, DraftingSelectionKind::Body, Figures.Key, ImGui::GetIO().KeyShift);
                State.PickMode = DraftingPickMode::Solid;
                Announce(State, Compose("%s · %d bodies picked", Figures.Caption.c_str(), int(State.Document.Selection.Bodies.size())));
            }
            RegisterWidget(State, Row.c_str());
            ImGui::PopStyleColor();
        }
        ImGui::Spacing();
    }

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
    if (ToggleButton(State, "Combs###CombToggle", State.CombsEnabled)) State.CombsEnabled = !State.CombsEnabled;
    ImGui::SameLine(0, 4);
    if (ToggleButton(State, "Zebra###ZebraToggle", State.ZebraEnabled)) ToggleZebra(State);
    ImGui::SameLine(0, 4);
    if (ToggleButton(State, "Deviation###DeviationToggle", State.DeviationEnabled)) State.DeviationEnabled = !State.DeviationEnabled;
    ImGui::SameLine(0, 4);
    if (ToggleButton(State, "Images###ImagesToggle", State.ImagePlanesEnabled)) State.ImagePlanesEnabled = !State.ImagePlanesEnabled;
    ImGui::SameLine(0, 4);
    if (PressButton(State, "Fit###FitView") && !State.Scene.Faces.empty()) FitDraftingCamera(State.Camera, State.Scene);
    ImGui::SameLine(0, 16);
    const std::pair<DraftingPickMode, const char*> Modes[] = { { DraftingPickMode::ControlPoint, "1 Point###Mode1" }, { DraftingPickMode::Edge, "2 Edge###Mode2" }, { DraftingPickMode::Face, "3 Face###Mode3" },
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

    // 📝 Mode 1 (Plasticity control-point mode): press on a shown pole grabs it, the pole follows the pointer in the
    //    screen-parallel plane through its start, the release commits one MoveDraftingPole; a press-release without
    //    movement is an ordinary click (selects the curve).
    DraftingPoleDrag& Drag = State.PoleDrag;
    bool PoleGesture = false;
    if (State.PickMode == DraftingPickMode::ControlPoint && !State.Drawing.ActiveCondition && !State.Bevel.ActiveCondition)
    {
        if (!Drag.ActiveCondition)
        {
            Drag.HoverPole = -1;
            if (Hovered && !OverCube) FindPoleNear(State, Frame, Io.MousePos, Drag.HoverLabel, Drag.HoverPole);
            if (Hovered && !OverCube && Drag.HoverPole >= 0 && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                const DraftingRay Ray = CastDraftingRay(Frame, Local.x, Local.y);
                Drag.ActiveCondition = true;
                Drag.Label           = Drag.HoverLabel;
                Drag.Pole            = Drag.HoverPole;
                Drag.Origin          = FindFittedCurve(State.Document, Drag.Label)->Curve.Definition->Poles[Drag.Pole];
                Drag.Position        = Drag.Origin;
                Drag.Facing          = { Ray.Direction[0], Ray.Direction[1], Ray.Direction[2] };
            }
        }
        if (Drag.ActiveCondition)
        {
            PoleGesture = true;
            const DraftingRay Ray = CastDraftingRay(Frame, Local.x, Local.y);
            const Vector3     Origin{ Ray.Origin[0], Ray.Origin[1], Ray.Origin[2] }, Direction{ Ray.Direction[0], Ray.Direction[1], Ray.Direction[2] };
            const double      Facing = Dot(Direction, Drag.Facing);
            const bool        Moved  = Io.MouseDragMaxDistanceSqr[ImGuiMouseButton_Left] > ClickTolerance * ClickTolerance;
            if (std::abs(Facing) > 1.0e-9 && Moved) Drag.Position = Origin + Direction * (Dot(Drag.Origin - Origin, Drag.Facing) / Facing);
            if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) || !ImGui::IsMouseDown(ImGuiMouseButton_Left))
            {
                Drag.ActiveCondition = false;
                if (Moved)
                {
                    const std::string            Caption = CaptionOf(State.Document, Drag.Label);
                    const DraftingCommandOutcome Outcome = MoveDraftingPole(State.Document, Drag.Label, Drag.Pole, Drag.Position);
                    Conclude(State, Outcome);
                    if (Outcome.AcceptedCondition)
                    {
                        ++Drag.Commits;
                        Announce(State, Compose("Moved pole %d of %s by %.2f mm · %.0f ms", Drag.Pole, Caption.c_str(), Distance(Drag.Position, Drag.Origin) * 1000.0, Outcome.Milliseconds));
                    }
                }
                else PickCurve(State, Drag.Label, PickIntentOf(Io));                         // A click on a pole selects its curve
            }
        }
    }

    if (PoleGesture)
    {
        // The pole drag owns the left button this frame.
    }
    else if (State.Drawing.ActiveCondition)
    {
        // 📝 Drawing: the pointer meets the construction plane, snaps to the 0.1 grid (Ctrl: free) and to the first point
        //    of a chain (closes it); click places, right-click finishes an open chain.
        DraftingDrawingSession& Drawing = State.Drawing;
        Vector2 Point;
        bool    Closing = false;
        Drawing.PointerCondition = Hovered && IntersectConstructionPlane(Frame, Local, Drawing.GridPlane, Point);
        if (Drawing.PointerCondition)
        {
            if (!Io.KeyCtrl) Point = { std::round(Point.X / DrawingSnap) * DrawingSnap, std::round(Point.Y / DrawingSnap) * DrawingSnap };
            ImVec2 First;
            const OrthonormalFrame Plane = DraftingSketchPlane(Drawing.GridPlane);
            if (DrawingClicks(Drawing.Kind) == 0 && Drawing.Points.size() >= 2 && ProjectWorldPoint(State, Frame, PlacePlanePoint(Plane, Drawing.Points.front()), First) &&
                std::hypot(Io.MousePos.x - First.x, Io.MousePos.y - First.y) <= ClosingTolerance)
            {
                Point   = Drawing.Points.front();
                Closing = true;
            }
            Drawing.Pointer = Point;
        }
        if (LeftClick && !OverCube && Drawing.PointerCondition) AppendDrawingPoint(State, Point, Closing);
        if (RightClick && !OverCube && State.Drawing.ActiveCondition) FinishDrawingSession(State, false);
    }
    else if (State.Bevel.ActiveCondition)
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
    else
    {
        DraftingBoxSelection& Box = State.Box;
        const bool BoxModeCondition = State.PickMode == DraftingPickMode::ControlPoint || State.PickMode == DraftingPickMode::Edge || State.PickMode == DraftingPickMode::Any;
        if (BoxModeCondition && Hovered && !OverCube && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            Box.ActiveCondition = true;
            Box.Start           = Io.MousePos;
        }
        const bool Moved = Io.MouseDragMaxDistanceSqr[ImGuiMouseButton_Left] > ClickTolerance * ClickTolerance;
        if (Box.ActiveCondition)
        {
            Box.End = Io.MousePos;
            if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) || !ImGui::IsMouseDown(ImGuiMouseButton_Left))
            {
                Box.ActiveCondition = false;
                if (Moved) SelectCurvesInBox(State, Frame, Box.Start, Box.End, PickIntentOf(Io));
                else if (LeftClick && !OverCube) PickAtPointer(State, Local, PickIntentOf(Io));
            }
        }
        else if (LeftClick && !OverCube)
        {
            PickAtPointer(State, Local, PickIntentOf(Io));
        }
    }

    // Overlays: sketch curves, the drawing in progress, status pills (top-left), interaction hint (bottom-left).
    Canvas->PushClipRect(Image, ImVec2(Image.x + Extent.x, Image.y + Extent.y), true);
    DrawSketchOverlays(State, Canvas, Frame);
    DrawCurveOverlays(State, Canvas, Frame);
    if (State.DeviationEnabled) DrawDeviationOverlay(State, Canvas, Frame);
    if (State.Drawing.ActiveCondition) DrawDrawingPreview(State, Canvas, Frame);
    if (State.Box.ActiveCondition && Io.MouseDragMaxDistanceSqr[ImGuiMouseButton_Left] > ClickTolerance * ClickTolerance)
    {
        const ImVec2 Low(std::min(State.Box.Start.x, State.Box.End.x), std::min(State.Box.Start.y, State.Box.End.y));
        const ImVec2 High(std::max(State.Box.Start.x, State.Box.End.x), std::max(State.Box.Start.y, State.Box.End.y));
        Canvas->AddRectFilled(Low, High, IM_COL32(0x4d, 0x9f, 0xff, 0x22));
        Canvas->AddRect(Low, High, IM_COL32(0x4d, 0x9f, 0xff, 0xcc));
    }
    Canvas->PopClipRect();
    ImVec2 Pill(Image.x + 12, Image.y + 12);
    static const char* ModeNames[] = { "", "Mode 1 · control points", "Mode 2 · edges", "Mode 3 · faces", "Mode 4 · solid", "Mode 5 · all" };
    DrawStatusPill(Canvas, Pill, ModeNames[int(State.PickMode)], SelectedColour);
    DrawStatusPill(Canvas, Pill, State.Camera.OrthographicEnabled ? "Parallel" : "Perspective", MutedColour);
    static const char* PlaneNames[] = { "Plane XY", "Plane XZ", "Plane YZ" };
    DrawStatusPill(Canvas, Pill, PlaneNames[std::clamp(State.Style.GridPlane, 0, 2)], AxisColours[State.Style.GridPlane == 2 ? 1 : 0]);
    const DraftingBodySummary& Summary = State.Document.Summary;
    if (Summary.PresentCondition)
        DrawStatusPill(Canvas, Pill, Summary.SheetCondition ? std::string(Summary.SoundCondition ? "Sound sheet" : "Sheet audit findings")
                                                            : Summary.Bodies.size() > 1
                                                            ? Compose("%d bodies · %s", int(Summary.Bodies.size()), Summary.SoundCondition ? "sound" : "audit findings")
                                                            : Compose("%s · genus %d", Summary.SoundCondition ? "Sound solid" : "Audit findings", Summary.Genus),
                       Summary.SoundCondition ? ActiveColour : RefusalColour);
    if (!State.Document.Regeneration.Curves.empty())
        DrawStatusPill(Canvas, Pill, Compose("%d curve(s) · %d selected", int(State.Document.Regeneration.Curves.size()), int(State.Document.Selection.Curves.size())), IM_COL32(0x9f, 0xd8, 0xff, 0xff));
    if (State.HoverIdentity != 0 && Hovered)
        DrawStatusPill(Canvas, Pill, NameDraftingIdentity(DisplayedDraftingScene(State), State.HoverIdentity), IM_COL32(0xff, 0xd2, 0x4d, 0xff));
    if (State.Bevel.ActiveCondition)
    {
        ImVec2 Below(Image.x + 12, Image.y + 44);
        const bool Refused = !State.Bevel.Refusal.empty();
        const char* Caption = State.Bevel.Verb == DraftingSessionVerb::Profile ? "Extrude sketch d" : State.Bevel.Verb == DraftingSessionVerb::Extrude ? "Extrude d"
                            : State.Bevel.Shape == BlendProfile::Fillet ? "Fillet r" : "Chamfer d";
        const std::string Sizes = State.Bevel.VariableEnabled && State.Bevel.Verb == DraftingSessionVerb::Blend
                                ? FormatLength(State.Bevel.Size) + " → " + FormatLength(State.Bevel.EndSize) + (State.Bevel.ReversedCondition ? " (flipped)" : "")
                                : FormatLength(State.Bevel.Size);
        const std::string Boolean = State.Bevel.Verb != DraftingSessionVerb::Profile ? std::string{}
                                  : std::string(" · ") + (State.Bevel.Combination == FeatureCombination::New ? "New" : FeatureCombinationLabel(State.Bevel.Combination));
        DrawStatusPill(Canvas, Below, Compose("%s %s m%s%s", Caption, Sizes.c_str(), Boolean.c_str(),
                                              State.Bevel.PinnedCondition ? " · pinned" : ""), Refused ? RefusalColour : ActiveColour);
        if (Refused) DrawStatusPill(Canvas, Below, State.Bevel.Refusal, RefusalColour);
    }
    if (Drag.ActiveCondition)
    {
        ImVec2 Below(Image.x + 12, Image.y + 44);
        const Vector3 Shift = Drag.Position - Drag.Origin;
        DrawStatusPill(Canvas, Below, Compose("Pole %d of %s · moved %.2f mm (%.2f, %.2f, %.2f)", Drag.Pole, CaptionOf(State.Document, Drag.Label).c_str(), Length(Shift) * 1000.0,
                                              Shift.X * 1000.0, Shift.Y * 1000.0, Shift.Z * 1000.0), ActiveColour);
    }
    if (State.Drawing.ActiveCondition)
    {
        ImVec2 Below(Image.x + 12, Image.y + 44);
        const DraftingDrawingSession& Drawing = State.Drawing;
        const std::string Pointer = Drawing.PointerCondition ? Compose(" · (%s, %s)", FormatLength(Drawing.Pointer.X).c_str(), FormatLength(Drawing.Pointer.Y).c_str()) : std::string{};
        DrawStatusPill(Canvas, Below, Compose("%s · %d point(s)%s", DrawingCaption(Drawing).c_str(), int(Drawing.Points.size()), Pointer.c_str()), ActiveColour);
        if (!Drawing.Refusal.empty()) DrawStatusPill(Canvas, Below, Drawing.Refusal, RefusalColour);
    }
    const char* Hint = State.Drawing.ActiveCondition
                           ? (State.Drawing.Kind == DraftingDrawingKind::Curve ? "Click points (Ctrl free) · 1 Hermite · 2 Bezier · 3 NURBS · 4 cubic · click first point closes · Enter / right-click finish · Ctrl+Z drop · Esc abort"
                              : State.Drawing.Kind == DraftingDrawingKind::Line ? "Click points (Ctrl free) · click first point closes · Enter / right-click finish · Ctrl+Z drop · Esc abort"
                              : State.Drawing.Kind == DraftingDrawingKind::Rectangle ? "Click two corners (Ctrl free) · Alt centre / corner · Ctrl+Z drop · Esc abort"
                              : State.Drawing.Kind == DraftingDrawingKind::Circle ? "Click centre then rim (Ctrl free) · V two-point / centre · Ctrl+Z drop · Esc abort"
                              : State.Drawing.Kind == DraftingDrawingKind::Arc ? "Click start, end, then a point on the arc (Ctrl free) · Ctrl+Z drop · Esc abort"
                              : "Click both cap centres, then the rim (Ctrl free) · Ctrl+Z drop · Esc abort")
                       : State.Bevel.ActiveCondition && State.Bevel.Verb == DraftingSessionVerb::Profile
                           ? "Move: distance (Shift fine) · Q union · W difference · E intersect · R new body · Enter / right-click finish · Esc abort"
                       : State.Bevel.ActiveCondition
                           ? "Move: size (Shift fine) · D fillet · C chamfer · V variable · F flip anchor · click edits selection · Enter / right-click finish · Esc abort"
                       : State.PickMode == DraftingPickMode::ControlPoint
                           ? "Mode 1: drag a pole (release commits, Esc drops) · click a curve (Shift add in loft order, Ctrl remove) · L loft · Ctrl+Z undo · 2/3/4/5 modes"
                           : "Middle orbit · right pan · wheel zoom · click select (Shift add, Ctrl remove, Alt chain) · 1-5 modes · B fillet · E extrude · L loft · Shift+A line · Shift+S curve";
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
    const bool            Profile = Bevel.Verb == DraftingSessionVerb::Profile;
    const bool            Extrude = Bevel.Verb == DraftingSessionVerb::Extrude || Profile;
    BeginCard(Profile ? "Extrude sketch" : Extrude ? "Extrude" : "Bevel");
    if (Profile)
    {
        // Plasticity extrude Boolean keys: q union · w difference · e intersect · r new body.
        static const struct { const char* Label; FeatureCombination Combination; } Choices[] = {
            { "Union (Q)###ProfileUnion", FeatureCombination::Union }, { "Difference (W)###ProfileDifference", FeatureCombination::Difference },
            { "Intersect (E)###ProfileIntersect", FeatureCombination::Intersection }, { "New body (R)###ProfileNew", FeatureCombination::Separate } };
        for (int Index = 0; Index < 4; ++Index)
        {
            if (Index % 2 == 1) ImGui::SameLine();
            if (ToggleButton(State, Choices[Index].Label, Bevel.Combination == Choices[Index].Combination, ImVec2(130, 0))) ChooseProfileCombination(State, Choices[Index].Combination);
        }
    }
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
    if (Profile)
    {
        const auto Caption = State.Document.Captions.find(Bevel.ProfileLabel);
        ImGui::TextWrapped("Profile: %s (%s)", Caption != State.Document.Captions.end() ? Caption->second.c_str() : "-", Bevel.ProfileLabel.c_str());
    }
    else ImGui::TextWrapped("%d %s: %s", int(Operands.size()), Extrude ? "face(s)" : "edge(s)", Operands.empty() ? "-" : Operands.front().c_str());
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
    if (Selection.Faces.empty() && Selection.Edges.empty() && Selection.Curves.empty())
    {
        ImGui::PushStyleColor(ImGuiCol_Text, Tint(MutedColour));
        ImGui::TextWrapped("Nothing selected. Click a curve, an edge or a face (1 control points · 2 edges · 3 faces · 4 solid · 5 all).");
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
    for (size_t Index = 0; Index < Selection.Curves.size(); ++Index)
    {
        const CurveFitOutcome* Fit = FindFittedCurve(State.Document, Selection.Curves[Index]);
        if (!Fit || ++Shown > 8) continue;
        ImGui::TextColored(Tint(Index + 1 == Selection.Curves.size() ? ActiveColour : SelectedColour), "Curve %d", int(Index + 1));
        ImGui::SameLine();
        ImGui::TextWrapped("%s · %d poles · fit %.3f mm", CaptionOf(State.Document, Selection.Curves[Index]).c_str(), int(Fit->Curve.Definition->Poles.size()), Fit->WorstDeviation * 1000.0);
    }
    if (Shown > 8) ImGui::Text("… %d more", Shown - 8);
    if (!Selection.Faces.empty() || !Selection.Edges.empty() || !Selection.Curves.empty())
    {
        if (PressButton(State, "Clear selection###SelectionClear"))
        {
            ClearDraftingSelection(State.Document);
            Announce(State, "Selection cleared");
        }
    }
    EndCard();

    if (!Selection.Curves.empty())
    {
        BeginCard("Curves");
        ImGui::PushStyleColor(ImGuiCol_Text, Tint(MutedColour));
        ImGui::TextWrapped("Loft runs through the curves in the order picked (badges 1, 2, …); curves that cross make a network. Mode 1 shows the poles: drag one to reshape.");
        ImGui::PopStyleColor();
        if (PressButton(State, "Loft  (L)###CurveLoft", ImVec2(140, 0), Selection.Curves.size() >= 2)) LoftSelectedCurves(State);
        ImGui::SameLine();
        if (ToggleButton(State, "Combs###CurveCombs", State.CombsEnabled, ImVec2(140, 0))) State.CombsEnabled = !State.CombsEnabled;
        // Continuity of the next loft to the panel it meets at its first / last section (V6).
        static const char* const ContinuityLabels[] = { "G0###LoftG0", "G1###LoftG1", "G2###LoftG2" };
        for (int Order = 0; Order < 3; ++Order)
        {
            if (Order) ImGui::SameLine();
            if (ToggleButton(State, ContinuityLabels[Order], State.LoftContinuity == Order, ImVec2(44, 0))) State.LoftContinuity = Order;
        }
        ImGui::SameLine();
        ImGui::TextDisabled("to the panel it meets");
        EndCard();
    }
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
    if (const CurveFitOutcome* Fit = Step->Kind == FeatureStepKind::SpaceCurve ? FindFittedCurve(State.Document, Label) : nullptr)
    {
        ImGui::Text("%d points → %d poles · degree %d · %d span(s)", int(Step->Points.size()), int(Fit->Curve.Definition->Poles.size()), Fit->Curve.Definition->Degree, Fit->Spans);
        ImGui::Text("Fit worst %.4f mm · mean %.4f mm (≤ %.2f mm)", Fit->WorstDeviation * 1000.0, Fit->MeanDeviation * 1000.0, EvaluateFeatureValue(State.Document.Tree, Step->Tolerance) * 1000.0);
        ImGui::Text("Fairing %.0e · curvature extrema %d · %d pole edit(s)%s", Fit->Fairing, Fit->Extrema, int(Step->PoleEdits.size()), Step->ClosedCondition ? " · closed" : "");
    }
    if (Step->Kind == FeatureStepKind::Loft || Step->Kind == FeatureStepKind::Network)
    {
        std::string Sections;
        for (const std::string& Section : Step->Sections) Sections += (Sections.empty() ? "" : Step->Kind == FeatureStepKind::Loft ? " → " : " · ") + CaptionOf(State.Document, Section);
        ImGui::TextWrapped("%s: %s", Step->Kind == FeatureStepKind::Loft ? "Sections" : "Network curves", Sections.c_str());
        if (Step->Kind == FeatureStepKind::Loft) ImGui::Text("Continuity G%d to the neighbouring panel", Step->Continuity);
    }
    if (Step->Kind == FeatureStepKind::Mirror) ImGui::Text("Plane through (%.3f, %.3f, %.3f), normal (%.0f, %.0f, %.0f)", Step->Plane.Origin.X, Step->Plane.Origin.Y, Step->Plane.Origin.Z,
                                                           Step->Plane.ZAxis.X, Step->Plane.ZAxis.Y, Step->Plane.ZAxis.Z);
    if (Step->Kind == FeatureStepKind::Close) ImGui::TextUnformatted("Plane caps on the planar rim loops of the panels");
    if (Step->Kind == FeatureStepKind::Cut)
    {
        const std::string Cutter = Step->Sections.empty() ? CaptionOf(State.Document, Step->Profile) : CaptionOf(State.Document, Step->Sections.front());
        ImGui::TextWrapped("Splits %s by %s%s · inside piece → %s", DraftingBodyCaption(State.Document, Step->Target).c_str(), Cutter.c_str(),
                           Step->Sections.empty() ? "" : Compose(" seen along (%.0f, %.0f, %.0f)", Step->Plane.ZAxis.X, Step->Plane.ZAxis.Y, Step->Plane.ZAxis.Z).c_str(),
                           DraftingBodyCaption(State.Document, Step->Label).c_str());
    }
    if (Step->Kind == FeatureStepKind::Remove) ImGui::Text("Deletes %s", DraftingBodyCaption(State.Document, Step->Target).c_str());
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
        if (Summary.Bodies.size() > 1)
        {
            // One line per body (caption · matcap · figures), then the totals.
            for (const DraftingBodyFigures& Figures : Summary.Bodies)
            {
                const bool Picked = std::find(State.Document.Selection.Bodies.begin(), State.Document.Selection.Bodies.end(), Figures.Key) != State.Document.Selection.Bodies.end();
                ImGui::TextColored(Tint(Picked ? SelectedColour : (Figures.SoundCondition ? ActiveColour : RefusalColour)), "%s", Figures.Caption.c_str());
                ImGui::SameLine();
                if (Figures.SheetCondition) ImGui::TextDisabled("sheet · %d faces · %s", Figures.Faces, DraftingBodyMatcap(State.Document.Appearance, Figures.Key).c_str());
                else ImGui::TextDisabled("%d faces · g%d · %.5f m³ · %s", Figures.Faces, Figures.Genus, Figures.Volume, DraftingBodyMatcap(State.Document.Appearance, Figures.Key).c_str());
            }
            ImGui::Text("%d bodies · Faces %d · Volume %.5f m³", int(Summary.Bodies.size()), Summary.Faces, Summary.Volume);
        }
        else
        {
            ImGui::Text("Faces %d · Edges %d · Vertices %d", Summary.Faces, Summary.Edges, Summary.Vertices);
            if (Summary.SheetCondition) ImGui::Text("Open sheet · %d face(s), no volume", Summary.Faces);
            else ImGui::Text("Genus %d · Volume %.5f m³", Summary.Genus, Summary.Volume);
        }
        // Surface panels (V5–V7): mirror through x = 0, close with plane caps.
        const int Panels = static_cast<int>(State.Document.Regeneration.Panels.size());
        if (Panels > 0)
        {
            ImGui::Text("%d surface panel(s)", Panels);
            if (PressButton(State, "Mirror x  (Alt+X)###PanelMirror", ImVec2(140, 0), Summary.SheetCondition)) MirrorPanels(State);
            ImGui::SameLine();
            if (PressButton(State, "Close → solid###PanelClose", ImVec2(140, 0), Summary.SheetCondition)) ClosePanels(State);
        }
        ImGui::TextColored(Tint(Summary.SoundCondition ? ActiveColour : RefusalColour), "%s", Summary.SoundCondition ? "Audit: sound" : "Audit: findings");
        ImGui::SameLine();
        ImGui::TextDisabled("· replay %.1f ms · %d triangles", Summary.RegenerationMs, int(State.Scene.Corners.size() / 3));
    }
    EndCard();
}

// 📝 Blueprint image planes (from a curves file's [[ImagePlanes]] or the document). Edits mark the document dirty; they are
//    not undo entries (reference pictures, not geometry).
void DrawImagePlaneCard(DraftingWorkspaceState& State)
{
    std::vector<DraftingImagePlane>& Planes = State.Document.ImagePlanes;
    if (Planes.empty()) return;
    BeginCard("Image planes");
    bool   EditedCondition = false;
    size_t Removal         = Planes.size();
    for (size_t Index = 0; Index < Planes.size(); ++Index)
    {
        DraftingImagePlane& Plane = Planes[Index];
        ImGui::PushID(int(Index));
        const std::string VisibleLabel = Plane.Name + "###PlaneVisible" + std::to_string(Index);
        if (ImGui::Checkbox(VisibleLabel.c_str(), &Plane.VisibleCondition)) EditedCondition = true;
        RegisterWidget(State, VisibleLabel.c_str());
        ImGui::SameLine();
        ImGui::SetNextItemWidth(110);
        float Opacity = float(Plane.Opacity);
        const std::string OpacityLabel = "###PlaneOpacity" + std::to_string(Index);
        if (ImGui::SliderFloat(OpacityLabel.c_str(), &Opacity, 0.05f, 1.0f, "ink %.2f"))
        {
            Plane.Opacity   = double(Opacity);
            EditedCondition = true;
        }
        RegisterWidget(State, OpacityLabel.c_str());
        ImGui::SameLine();
        const std::string RemoveLabel = "Remove###PlaneRemove" + std::to_string(Index);
        if (PressButton(State, RemoveLabel.c_str())) Removal = Index;
        const auto Found = std::find(State.Pictures.Paths.begin(), State.Pictures.Paths.end(), Plane.Path);
        if (Found != State.Pictures.Paths.end())
        {
            const size_t Picture = size_t(Found - State.Pictures.Paths.begin());
            if (!State.Pictures.Refusals[Picture].empty()) ImGui::TextColored(Tint(RefusalColour), "%s", State.Pictures.Refusals[Picture].c_str());
            else ImGui::TextDisabled("%u × %u px · %.3f × %.3f m", State.Pictures.Widths[Picture], State.Pictures.Heights[Picture], Modeller::Length(Plane.Right), Modeller::Length(Plane.Down));
        }
        ImGui::PopID();
    }
    if (Removal < Planes.size())
    {
        Announce(State, "Image plane removed: " + Planes[Removal].Name);
        Planes.erase(Planes.begin() + std::ptrdiff_t(Removal));
        EditedCondition = true;
    }
    if (EditedCondition)
    {
        State.DirtyCondition = true;
        State.AutosaveTimer  = State.Options.AutosaveDelay;
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

// 📝 Drawing tools on the construction plane (the viewport bar's plane at open). Plasticity keys where it has them.
void DrawDrawingCard(DraftingWorkspaceState& State)
{
    DraftingDrawingSession& Drawing = State.Drawing;
    if (!Drawing.ActiveCondition)
    {
        BeginCard("Draw");
        static const struct { const char* Label; DraftingDrawingKind Kind; } Tools[] = {
            { "Line (Shift+A)###DrawLine", DraftingDrawingKind::Line }, { "Rectangle (Shift+Q)###DrawRectangle", DraftingDrawingKind::Rectangle },
            { "Circle (Shift+W)###DrawCircle", DraftingDrawingKind::Circle }, { "Curve (Shift+S)###DrawCurve", DraftingDrawingKind::Curve },
            { "Arc (3 points)###DrawArc", DraftingDrawingKind::Arc }, { "Slot###DrawSlot", DraftingDrawingKind::Slot } };
        for (int Index = 0; Index < 6; ++Index)
        {
            if (Index % 2 == 1) ImGui::SameLine();
            if (PressButton(State, Tools[Index].Label, ImVec2(140, 0), !State.Bevel.ActiveCondition)) OpenDrawingSession(State, Tools[Index].Kind);
        }
        if (FocusedSketchCondition(State))
        {
            const bool Consumed = DraftingSketchConsumedCondition(State.Document, State.FocusedStep);
            if (PressButton(State, "Extrude sketch (E)###ExtrudeSketch", ImVec2(284, 0), !State.Bevel.ActiveCondition)) OpenSketchExtrudeSession(State);
            if (Consumed)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, Tint(MutedColour));
                ImGui::TextWrapped("This sketch already drives a feature; a second extrude reuses it.");
                ImGui::PopStyleColor();
            }
        }
        EndCard();
        return;
    }
    BeginCard(DraftingDrawingLabel(Drawing.Kind));
    static const char* Planes[] = { "XY", "XZ", "YZ" };
    ImGui::Text("Plane %s · %d point(s)", Planes[std::clamp(Drawing.GridPlane, 0, 2)], int(Drawing.Points.size()));
    if (Drawing.Kind == DraftingDrawingKind::Curve)
    {
        static const struct { const char* Label; SketchSplineForm Form; } Forms[] = {
            { "Hermite (1)###FormHermite", SketchSplineForm::Hermite }, { "Bezier (2)###FormBezier", SketchSplineForm::Bezier },
            { "NURBS (3)###FormNurbs", SketchSplineForm::Nurbs }, { "Cubic spline (4)###FormCubic", SketchSplineForm::CubicSpline } };
        for (int Index = 0; Index < 4; ++Index)
        {
            if (Index % 2 == 1) ImGui::SameLine();
            if (ToggleButton(State, Forms[Index].Label, Drawing.Form == Forms[Index].Form, ImVec2(140, 0))) { Drawing.Form = Forms[Index].Form; Drawing.Refusal.clear(); }
        }
    }
    if (Drawing.Kind == DraftingDrawingKind::Rectangle || Drawing.Kind == DraftingDrawingKind::Circle)
    {
        const char* Off = Drawing.Kind == DraftingDrawingKind::Rectangle ? "Corner###ModeCorner" : "Two-point###ModeTwoPoint";
        if (ToggleButton(State, Off, !Drawing.CentreCondition, ImVec2(140, 0))) Drawing.CentreCondition = false;
        ImGui::SameLine();
        if (ToggleButton(State, "Centre###ModeCentre", Drawing.CentreCondition, ImVec2(140, 0))) Drawing.CentreCondition = true;
    }
    const bool Chain = DrawingClicks(Drawing.Kind) == 0;
    if (Chain)
    {
        if (PressButton(State, "Finish open (Enter)###DrawFinish", ImVec2(140, 0), Drawing.Points.size() >= 2)) FinishDrawingSession(State, false);
        ImGui::SameLine();
        if (PressButton(State, "Close###DrawClose", ImVec2(140, 0), Drawing.Points.size() >= 3)) FinishDrawingSession(State, true);
    }
    if (PressButton(State, "Drop point (Ctrl+Z)###DrawRetract", ImVec2(140, 0), !Drawing.Points.empty())) RetractDrawingPoint(State);
    ImGui::SameLine();
    if (PressButton(State, "Abort (Esc)###DrawAbort", ImVec2(140, 0))) CancelDrawingSession(State);
    if (!Drawing.Refusal.empty())
    {
        ImGui::PushStyleColor(ImGuiCol_Text, Tint(RefusalColour));
        ImGui::TextWrapped("%s", Drawing.Refusal.c_str());
        ImGui::PopStyleColor();
    }
    EndCard();
}

void DrawInspector(DraftingWorkspaceState& State, ImVec2 Position, ImVec2 Size)
{
    BeginPanel("Inspector", Position, Size);
    PanelHeading("Inspector");
    if (State.Boolean.ActiveCondition) DrawBooleanCard(State);
    else if (State.Bevel.ActiveCondition) DrawBevelCard(State);
    else if (!State.Drawing.ActiveCondition) DrawSelectionCard(State);
    DrawDrawingCard(State);
    DrawStepCard(State);
    DrawBodyCard(State);
    DrawAppearanceCard(State);
    DrawImagePlaneCard(State);
    DrawHistoryCard(State);
    EndPanel();
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    SHORTCUTS + RECOVERY
//------------------------------------------------------------------------------------------------------------------------

// Plasticity Space (viewport:navigate:selection): frame the selected faces and edges, or the whole body.
void FrameSelection(DraftingWorkspaceState& State)
{
    const DraftingSelection& Selection = State.Document.Selection;
    if (State.Scene.Faces.empty() && State.Document.Regeneration.Curves.empty()) return;
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
    const auto IncludeCurve = [&](const std::string& Label)
    {
        const CurveFitOutcome* Fit = FindFittedCurve(State.Document, Label);
        if (!Fit) return;
        for (const Vector3& Pole : Fit->Curve.Definition->Poles)
        {
            const float Point[3] = { float(Pole.X), float(Pole.Y), float(Pole.Z) };
            Include(Point);
        }
    };
    for (const std::string& Label : Selection.Curves) IncludeCurve(Label);
    const bool Selected = Found;
    if (!Found)
    {
        // Nothing selected: the body and every curve.
        if (!State.Scene.Faces.empty())
        {
            const float Low[3]  = { float(State.Scene.BoundsLow[0]), float(State.Scene.BoundsLow[1]), float(State.Scene.BoundsLow[2]) };
            const float High[3] = { float(State.Scene.BoundsHigh[0]), float(State.Scene.BoundsHigh[1]), float(State.Scene.BoundsHigh[2]) };
            Include(Low);
            Include(High);
        }
        for (const auto& [Label, Fit] : State.Document.Regeneration.Curves) IncludeCurve(Label);
    }
    FitDraftingCamera(State.Camera, Bounds);
    State.Camera.SceneRadius = std::max(State.Camera.SceneRadius, 1.0);
    Announce(State, Selected ? "Framed the selection (Space)" : "Framed the document (Space)");
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

    // Mode 1 pole drag in progress: Esc drops it (no edit).
    if (State.PoleDrag.ActiveCondition)
    {
        if (Pressed(ImGuiKey_Escape))
        {
            State.PoleDrag.ActiveCondition = false;
            Announce(State, "Pole drag dropped");
        }
        return;
    }

    // body[command] — a drawing command (line, curve, corner-rectangle, center-circle; arc and slot have no default key).
    if (State.Drawing.ActiveCondition)
    {
        DraftingDrawingSession& Drawing = State.Drawing;
        if (Pressed(ImGuiKey_Enter) || Pressed(ImGuiKey_KeypadEnter)) FinishDrawingSession(State, false);
        else if (Pressed(ImGuiKey_Escape)) CancelDrawingSession(State);
        else if (Control && Pressed(ImGuiKey_Z)) RetractDrawingPoint(State);               // gizmo:line:undo / gizmo:curve:undo
        else if (Drawing.Kind == DraftingDrawingKind::Curve && !Control && !Alternate)
        {
            static const ImGuiKey Keys[] = { ImGuiKey_1, ImGuiKey_2, ImGuiKey_3, ImGuiKey_4 };
            static const SketchSplineForm Forms[] = { SketchSplineForm::Hermite, SketchSplineForm::Bezier, SketchSplineForm::Nurbs, SketchSplineForm::CubicSpline };
            for (int Index = 0; Index < 4; ++Index)
                if (Pressed(Keys[Index]))                                                     // gizmo:curve:hermite / bezier / nurbs / cubic-spline
                {
                    Drawing.Form = Forms[Index];
                    Drawing.Refusal.clear();
                    Announce(State, std::string("Curve form: ") + SketchSplineFormLabel(Drawing.Form));
                }
        }
        else if (Drawing.Kind == DraftingDrawingKind::Rectangle && (Pressed(ImGuiKey_LeftAlt) || Pressed(ImGuiKey_RightAlt)))
        {
            Drawing.CentreCondition = !Drawing.CentreCondition;                               // keyboard:rectangle:mode
            Announce(State, Drawing.CentreCondition ? "Rectangle: centre mode (first click is the centre)" : "Rectangle: corner mode");
        }
        else if (Drawing.Kind == DraftingDrawingKind::Circle && !Control && !Alternate && Pressed(ImGuiKey_V))
        {
            Drawing.CentreCondition = !Drawing.CentreCondition;                               // gizmo:circle:mode
            Announce(State, Drawing.CentreCondition ? "Circle: centre and rim" : "Circle: two diameter points");
        }
        return;
    }

    // body[command='boolean'] — Q union · W difference · E intersect · Enter applies · Esc cancels.
    if (State.Boolean.ActiveCondition)
    {
        if (Pressed(ImGuiKey_Enter) || Pressed(ImGuiKey_KeypadEnter)) ApplyBooleanSession(State);
        else if (Pressed(ImGuiKey_Escape)) { State.Boolean.ActiveCondition = false; Announce(State, "Boolean cancelled"); }
        else if (!Control && !Alternate)
        {
            if (Pressed(ImGuiKey_Q)) ChooseBooleanCombination(State, FeatureCombination::Union);            // gizmo:boolean:union
            if (Pressed(ImGuiKey_W)) ChooseBooleanCombination(State, FeatureCombination::Difference);       // gizmo:boolean:difference
            if (Pressed(ImGuiKey_E)) ChooseBooleanCombination(State, FeatureCombination::Intersection);     // gizmo:boolean:intersect
        }
        return;
    }

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
        if (State.Bevel.Verb == DraftingSessionVerb::Profile && !Control && !Alternate)
        {
            if (Pressed(ImGuiKey_Q)) ChooseProfileCombination(State, FeatureCombination::Union);        // keyboard:extrude:union
            if (Pressed(ImGuiKey_W)) ChooseProfileCombination(State, FeatureCombination::Difference);   // keyboard:extrude:difference
            if (Pressed(ImGuiKey_E)) ChooseProfileCombination(State, FeatureCombination::Intersection); // keyboard:extrude:intersect
            if (Pressed(ImGuiKey_R)) ChooseProfileCombination(State, FeatureCombination::Separate);     // keyboard:extrude:new-body
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
        if (Pressed(ImGuiKey_C) || Pressed(ImGuiKey_V)) Announce(State, "Copy / paste: not available yet");
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
        if (Pressed(ImGuiKey_X)) MirrorPanels(State);                                       // mirror (gizmo x: plane x = 0)
        if (Pressed(ImGuiKey_M)) RemoveMatcap(State);                                       // remove-material
        if (Pressed(ImGuiKey_H) || Pressed(ImGuiKey_G) || Pressed(ImGuiKey_Q)) Announce(State, "Not available in this modeller");
        return;
    }
    if (Shift)
    {
        if (Pressed(ImGuiKey_C) || Pressed(ImGuiKey_V)) OpenConstruct(State, DraftingConstructKind::Block);     // corner / centre box
        if (Pressed(ImGuiKey_X)) OpenConstruct(State, DraftingConstructKind::Cylinder);
        if (Pressed(ImGuiKey_Z)) OpenConstruct(State, DraftingConstructKind::Sphere);
        if (Pressed(ImGuiKey_R)) RepeatLastCommand(State);
        if (Pressed(ImGuiKey_D)) Announce(State, "Duplicate: not available yet");
        if (Pressed(ImGuiKey_A)) OpenDrawingSession(State, DraftingDrawingKind::Line);                  // line
        if (Pressed(ImGuiKey_S)) OpenDrawingSession(State, DraftingDrawingKind::Curve);                 // curve
        if (Pressed(ImGuiKey_Q)) OpenDrawingSession(State, DraftingDrawingKind::Rectangle);             // corner-rectangle
        if (Pressed(ImGuiKey_W)) OpenDrawingSession(State, DraftingDrawingKind::Circle);                // center-circle
        if (Pressed(ImGuiKey_H) || Pressed(ImGuiKey_P)) Announce(State, "Not available in this modeller");
        if (Pressed(ImGuiKey_1) || Pressed(ImGuiKey_2) || Pressed(ImGuiKey_3) || Pressed(ImGuiKey_4)) Announce(State, "Mode toggles: one mode at a time (2 / 3 / 4 / 5)");
        return;
    }

    // body:not([gizmo]) — no command running. The M picker takes Esc first.
    if (State.Matcaps.PickerOpen && Pressed(ImGuiKey_Escape))
    {
        State.Matcaps.PickerOpen = false;
        Announce(State, "Set material closed");
        return;
    }
    if (Pressed(ImGuiKey_M))                                                                // set-material
    {
        State.Matcaps.PickerOpen = State.Document.Summary.PresentCondition;
        if (!State.Matcaps.PickerOpen) Announce(State, "Set material needs a body");
    }
    if (Pressed(ImGuiKey_1)) State.PickMode = DraftingPickMode::ControlPoint;
    if (Pressed(ImGuiKey_L)) LoftSelectedCurves(State);                                     // loft
    if (Pressed(ImGuiKey_2)) State.PickMode = DraftingPickMode::Edge;
    if (Pressed(ImGuiKey_3)) State.PickMode = DraftingPickMode::Face;
    if (Pressed(ImGuiKey_4)) State.PickMode = DraftingPickMode::Solid;
    if (Pressed(ImGuiKey_5) || Pressed(ImGuiKey_Tab)) State.PickMode = DraftingPickMode::Any;
    if (Pressed(ImGuiKey_B)) OpenBevelSession(State, BlendProfile::Fillet);
    if (Pressed(ImGuiKey_E))
    {
        if (State.Document.Selection.Faces.empty() && FocusedSketchCondition(State)) OpenSketchExtrudeSession(State);
        else OpenExtrudeSession(State);
    }
    if (Pressed(ImGuiKey_Space)) FrameSelection(State);
    if (Pressed(ImGuiKey_Q)) OpenBooleanSession(State);                                    // command:boolean
    if (Pressed(ImGuiKey_G) || Pressed(ImGuiKey_R) || Pressed(ImGuiKey_S)) Announce(State, "Move / rotate / scale bodies: not available (face edits: Face edit card)");
    if (Pressed(ImGuiKey_H)) Announce(State, "Hide: not available (no per-face visibility)");
    if (Pressed(ImGuiKey_C)) ApplyCut(State);                                              // command:cut
    if (Pressed(ImGuiKey_J) || Pressed(ImGuiKey_P) || Pressed(ImGuiKey_T) || Pressed(ImGuiKey_O))
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
        else if (!State.Document.Selection.Bodies.empty())
        {
            Conclude(State, RemoveDraftingBodies(State.Document, State.Document.Selection.Bodies));          // delete picked bodies (mode 4)
        }
        else if (!State.Document.Selection.Faces.empty())
        {
            const DraftingCommandOutcome Outcome = ChangeDraftingSelection(State.Document, FaceOperationKind::Delete, 0.0);
            if (Outcome.AcceptedCondition) State.LastCommand = { DraftingRepeatKind::Face, BlendProfile::Fillet, 0.0, FaceOperationKind::Delete, 0.0, {} };
            Conclude(State, Outcome);
        }
        else Announce(State, "Delete needs picked bodies (mode 4), selected faces or a focused feature step");
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
    RefreshDraftingMatcaps(State);
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
    DrawMatcapPicker(State);
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

void RefreshDraftingMatcaps(DraftingWorkspaceState& State)
{
    DraftingMatcapShelf& Shelf = State.Matcaps;
    if (!Shelf.Library.Names.empty() && Shelf.Library.Key == DraftingMatcapKey(State.Document.Appearance)) return;
    Shelf.Library = AssembleDraftingMatcaps(State.Document.Appearance);
    Shelf.Generation += 1;
}

DraftingSceneStates DisplayedDraftingStates(const DraftingWorkspaceState& State)
{
    const DraftingScene& Scene = DisplayedDraftingScene(State);
    DraftingSceneStates States = &Scene == &State.Bevel.PreviewScene ? ComposeDraftingStates(Scene, State.Bevel.Preview->Selection, State.HoverIdentity)
                                                                      : ComposeDraftingStates(Scene, State.Document.Selection, State.ViewportHovered ? State.HoverIdentity : 0);
    // 📝 Matcap layers ride in bits 8 … 15 of each face word (DraftingSurface.slang).
    const std::vector<uint32_t> Layers = ComposeDraftingMatcapLayers(Scene, State.Document.Appearance, State.Matcaps.Library);
    for (size_t Index = 0; Index < States.Faces.size() && Index < Layers.size(); ++Index) States.Faces[Index] |= Layers[Index];
    return States;
}

void RefreshDraftingScene(DraftingWorkspaceState& State, bool DirtyCondition)
{
    State.Scene = BuildDraftingScene(State.Document, 1.5e-3, State.ZebraEnabled);
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
    Stream << "], \"Curves\": [";
    for (size_t Index = 0; Index < Document.Selection.Curves.size(); ++Index)
    {
        Stream << (Index ? ", " : "");
        AppendJsonText(Stream, CaptionOf(Document, Document.Selection.Curves[Index]));
    }
    Stream << "]},\n  \"Curves\": [";
    size_t Written = 0;
    for (const FeatureStep& Step : Document.Tree.Steps)
    {
        const CurveFitOutcome* Fit = Step.Kind == FeatureStepKind::SpaceCurve ? FindFittedCurve(Document, Step.Label) : nullptr;
        if (!Fit) continue;
        Stream << (Written++ ? ", " : "") << "{\"Caption\": ";
        AppendJsonText(Stream, CaptionOf(Document, Step.Label));
        Stream << ", \"Points\": " << Step.Points.size() << ", \"Poles\": " << Fit->Curve.Definition->Poles.size() << ", \"WorstDeviation\": " << Fit->WorstDeviation
               << ", \"PoleEdits\": " << Step.PoleEdits.size() << "}";
    }
    Stream << "],\n  \"Sheet\": " << (Summary.SheetCondition ? "true" : "false") << ",\n  \"PoleDrags\": " << State.PoleDrag.Commits << ",\n  \"PickMode\": " << int(State.PickMode) << ",\n  \"Hover\": ";
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
    Stream << ", \"Verb\": " << int(State.Bevel.Verb) << ", \"Profile\": ";
    AppendJsonText(Stream, State.Bevel.ProfileLabel);
    Stream << ", \"Combination\": " << int(State.Bevel.Combination) << "},\n";
    Stream << "  \"Drawing\": {\"Active\": " << (State.Drawing.ActiveCondition ? "true" : "false") << ", \"Kind\": ";
    AppendJsonText(Stream, DraftingDrawingLabel(State.Drawing.Kind));
    Stream << ", \"Plane\": " << State.Drawing.GridPlane << ", \"Form\": ";
    AppendJsonText(Stream, SketchSplineFormLabel(State.Drawing.Form));
    Stream << ", \"Centre\": " << (State.Drawing.CentreCondition ? "true" : "false") << ", \"Points\": [";
    for (size_t Index = 0; Index < State.Drawing.Points.size(); ++Index)
        Stream << (Index ? ", " : "") << "[" << State.Drawing.Points[Index].X << ", " << State.Drawing.Points[Index].Y << "]";
    Stream << "], \"Refusal\": ";
    AppendJsonText(Stream, State.Drawing.Refusal);
    Stream << "},\n  \"Undo\": " << Document.UndoEntries.size() << ",\n  \"Redo\": " << Document.RedoEntries.size() << ",\n  \"Welcome\": "
           << (State.WelcomeOpen ? "true" : "false") << ",\n  \"Status\": ";
    AppendJsonText(Stream, State.Status);
    Stream << ",\n  \"Viewport\": [" << State.ViewportRect[0] << ", " << State.ViewportRect[1] << ", " << State.ViewportRect[2] << ", " << State.ViewportRect[3] << "]\n}\n";
    return Stream.str();
}

} // namespace Frontier::DraftingWorkspace
