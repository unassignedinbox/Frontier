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
    const std::set<std::string> Hidden = State.Document.Appearance.HiddenBodies;
    if (UndoDraftingCommand(State.Document))
    {
        if (AppearanceCondition && Hidden == State.Document.Appearance.HiddenBodies) MarkDirty(State);
        else RefreshDraftingScene(State, true);                                              // Geometry or visibility changed
        Announce(State, "Undo");
    }
}

void Redo(DraftingWorkspaceState& State)
{
    if (State.Bevel.ActiveCondition) return;
    const bool AppearanceCondition = !State.Document.RedoEntries.empty() && State.Document.RedoEntries.back().AppearanceCondition;
    const std::set<std::string> Hidden = State.Document.Appearance.HiddenBodies;
    if (RedoDraftingCommand(State.Document))
    {
        if (AppearanceCondition && Hidden == State.Document.Appearance.HiddenBodies) MarkDirty(State);
        else RefreshDraftingScene(State, true);                                              // Geometry or visibility changed
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
bool IntersectSketchPlane(const DraftingCameraFrame& Frame, ImVec2 Local, const OrthonormalFrame& Plane, Vector2& Point)
{
    const DraftingRay      Ray    = CastDraftingRay(Frame, Local.x, Local.y);
    const Vector3          Origin{ Ray.Origin[0], Ray.Origin[1], Ray.Origin[2] }, Direction{ Ray.Direction[0], Ray.Direction[1], Ray.Direction[2] };
    const double           Facing = Dot(Direction, Plane.ZAxis);
    if (std::abs(Facing) < 1.0e-9) return false;
    const Vector3 Hit = Origin + Direction * (Dot(Plane.Origin - Origin, Plane.ZAxis) / Facing);
    Point = { Dot(Hit - Plane.Origin, Plane.XAxis), Dot(Hit - Plane.Origin, Plane.YAxis) };
    return true;
}

bool IntersectConstructionPlane(const DraftingCameraFrame& Frame, ImVec2 Local, int GridPlane, Vector2& Point)
{
    return IntersectSketchPlane(Frame, Local, DraftingSketchPlane(GridPlane), Point);
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
    DraftingPointPicker& Picker = State.Picker;
    Picker.Placed = Picker.Current;                                                          // t / n / b use the snapped curve or face
    if (Picker.Current.Kind >= DraftingSnapKind::Endpoint && Picker.Current.Kind <= DraftingSnapKind::Face) ++Picker.Snapped;
    Picker.Lock = DraftingSnapLockKind::None;                                                // A lock serves one point (Plasticity)
    if (!Chain && Drawing.Points.size() >= DrawingClicks(Drawing.Kind)) FinishDrawingSession(State, false);
}

void RetractDrawingPoint(DraftingWorkspaceState& State)
{
    DraftingDrawingSession& Drawing = State.Drawing;
    if (Drawing.Points.empty()) { Announce(State, "No point to drop"); return; }
    Drawing.Points.pop_back();
    Drawing.Refusal.clear();
    State.Picker.Placed = {};
    State.Picker.Lock   = DraftingSnapLockKind::None;
    Announce(State, Compose("Point dropped · %d left", int(Drawing.Points.size())));
}

// 📝 Object snapping (Plasticity point picker). The catalogue of exact carriers follows the document revision, the
//    scene's faces and the bodies excluded (Move skips the bodies it carries).
const DraftingSectionClip* ActiveSectionClip(const DraftingWorkspaceState& State);

[[nodiscard]] bool PointSnapCondition(DraftingSnapKind Kind)
{
    return Kind >= DraftingSnapKind::Endpoint && Kind <= DraftingSnapKind::Perpendicular;
}

DraftingSnapCatalogue& SnapCatalogue(DraftingWorkspaceState& State, const std::vector<std::string>& Excluded)
{
    DraftingPointPicker& Picker = State.Picker;
    std::string Key;
    for (const std::string& Body : Excluded) Key += Body + ";";
    if (!Picker.Catalogue || Picker.Catalogue->Revision != State.Document.Revision || Picker.Catalogue->ExclusionKey != Key || Picker.SceneFaces != State.Scene.Faces.size())
    {
        Picker.Catalogue  = std::make_unique<DraftingSnapCatalogue>(CatalogueDraftingSnaps(State.Document, State.Scene, Excluded));
        Picker.SceneFaces = State.Scene.Faces.size();
    }
    return *Picker.Catalogue;
}

// The snap under the pointer (None when snapping is off or Ctrl is held).
DraftingSnap FindPointerSnap(DraftingWorkspaceState& State, const DraftingCameraFrame& Frame, ImVec2 Local, std::optional<Vector3> Reference,
                             std::vector<DraftingSnapPoint> Transient, const std::vector<std::string>& Excluded, bool FaceCondition)
{
    if (!State.Picker.Enabled || ImGui::GetIO().KeyCtrl) return {};
    DraftingSnapQuery Query;
    Query.Frame       = Frame;
    Query.PixelX      = Local.x;
    Query.PixelY      = Local.y;
    Query.Reference   = Reference;
    Query.Transient   = std::move(Transient);
    Query.XRayEnabled = State.Style.XRayEnabled;
    Query.Section     = ActiveSectionClip(State);
    Query.Filter      = State.Picker.Filter;
    Query.Filter.FaceEnabled = Query.Filter.FaceEnabled && FaceCondition;
    return FindDraftingSnap(SnapCatalogue(State, Excluded), State.Document, State.Scene, Query);
}

// n / b / t / x / y / z while drawing: a line through the last point the next point slides along; the same key again
// releases it. Directions leaving the drawing plane are refused (the sketch stays planar).
void ChooseSnapLock(DraftingWorkspaceState& State, DraftingSnapLockKind Kind)
{
    DraftingPointPicker&          Picker  = State.Picker;
    const DraftingDrawingSession& Drawing = State.Drawing;
    if (Picker.Lock == Kind)
    {
        Picker.Lock = DraftingSnapLockKind::None;
        Announce(State, std::string("Lock ") + DraftingSnapLockLabel(Kind) + " released");
        return;
    }
    if (Drawing.Points.empty()) { Announce(State, std::string("Lock ") + DraftingSnapLockLabel(Kind) + ": place a point first"); return; }
    const OrthonormalFrame Plane  = DraftingSketchPlane(Drawing.GridPlane);
    const Vector3          Origin = PlacePlanePoint(Plane, Drawing.Points.back());
    Vector3 Tangent = Picker.Placed.Tangent;
    if (Length(Tangent) < 0.5 && Drawing.Points.size() >= 2)
    {
        const Vector3 Chord = Origin - PlacePlanePoint(Plane, Drawing.Points[Drawing.Points.size() - 2]);
        if (Length(Chord) > 0.0) Tangent = Chord * (1.0 / Length(Chord));
    }
    Vector3 Normal = Picker.Placed.Normal;
    if (Length(Normal) < 0.5 && Length(Tangent) > 0.5) Normal = Normalize(Cross(Plane.ZAxis, Tangent));
    Vector3 Direction;
    switch (Kind)
    {
    case DraftingSnapLockKind::X:        Direction = { 1.0, 0.0, 0.0 }; break;
    case DraftingSnapLockKind::Y:        Direction = { 0.0, 1.0, 0.0 }; break;
    case DraftingSnapLockKind::Z:        Direction = { 0.0, 0.0, 1.0 }; break;
    case DraftingSnapLockKind::Tangent:  Direction = Tangent; break;
    case DraftingSnapLockKind::Normal:   Direction = Normal; break;
    case DraftingSnapLockKind::Binormal: Direction = Length(Tangent) > 0.5 && Length(Normal) > 0.5 ? Normalize(Cross(Tangent, Normal)) : Vector3{}; break;
    default: return;
    }
    static const char* Planes[] = { "XY", "XZ", "YZ" };
    if (Length(Direction) < 0.5)
    {
        Announce(State, std::string("Lock ") + DraftingSnapLockLabel(Kind) + ": the last point has no curve or face direction (snap it to a curve, or place two points)");
        return;
    }
    if (std::abs(Dot(Direction, Plane.ZAxis)) > 1.0e-9)
    {
        Announce(State, std::string("Lock ") + DraftingSnapLockLabel(Kind) + " leaves the " + Planes[std::clamp(Drawing.GridPlane, 0, 2)] + " drawing plane: refused");
        return;
    }
    Picker.Lock          = Kind;
    Picker.LockOrigin    = Origin;
    Picker.LockDirection = Direction;
    Announce(State, std::string("Lock ") + DraftingSnapLockLabel(Kind) + " through the last point · the same key releases it · object snaps align along it");
}

// An object snap carried into a sketch plane (its foot along the plane normal). On-curve snaps count only on curves in
// the plane: an edge above the plane that merely overlaps the pointer on screen would move the point away from where
// it was clicked (its distinct points still snap). Sets Picker.Current; nullopt when nothing snaps.
std::optional<Vector2> SnapOntoPlane(DraftingWorkspaceState& State, const DraftingCameraFrame& Frame, ImVec2 Local, const OrthonormalFrame& Plane,
                                     std::optional<Vector3> Reference, std::vector<DraftingSnapPoint> Transient, const std::vector<std::string>& Excluded)
{
    DraftingSnap Object = FindPointerSnap(State, Frame, Local, Reference, std::move(Transient), Excluded, false);
    if (Object.Kind == DraftingSnapKind::Curve && std::abs(Dot(Object.Position - Plane.Origin, Plane.ZAxis)) > 1.0e-9 * (1.0 + Length(Object.Position))) Object = {};
    State.Picker.Current = Object;
    if (Object.Kind == DraftingSnapKind::None) return std::nullopt;
    return Vector2{ Dot(Object.Position - Plane.Origin, Plane.XAxis), Dot(Object.Position - Plane.Origin, Plane.YAxis) };
}

// The drawing pointer: on the lock line (aligned with a point snap, else 0.1 steps), else on an object snap projected
// into the drawing plane, else on the 0.1 grid; Ctrl is free. Rectangle s squares the second corner.
void PlaceDrawingPointer(DraftingWorkspaceState& State, const DraftingCameraFrame& Frame, ImVec2 Local, Vector2& Point)
{
    DraftingDrawingSession& Drawing = State.Drawing;
    DraftingPointPicker&    Picker  = State.Picker;
    const OrthonormalFrame  Plane   = DraftingSketchPlane(Drawing.GridPlane);
    const bool              Free    = ImGui::GetIO().KeyCtrl;
    auto Flatten = [&](Vector3 World) { return Vector2{ Dot(World - Plane.Origin, Plane.XAxis), Dot(World - Plane.Origin, Plane.YAxis) }; };
    std::vector<DraftingSnapPoint> Chain;
    for (size_t Index = 0; Index < Drawing.Points.size(); ++Index)
        Chain.push_back({ DraftingSnapKind::Endpoint, PlacePlanePoint(Plane, Drawing.Points[Index]), {}, {}, Compose("Point %d", int(Index) + 1), {} });
    std::optional<Vector3> Reference;
    if (!Drawing.Points.empty()) Reference = PlacePlanePoint(Plane, Drawing.Points.back());
    Picker.Current = {};
    if (Picker.Lock != DraftingSnapLockKind::None)
    {
        const std::optional<double> Along = ApproachDraftingLine(CastDraftingRay(Frame, Local.x, Local.y), Picker.LockOrigin, Picker.LockDirection);
        if (Along)
        {
            double      Travel = *Along;
            std::string Note;
            const DraftingSnap Object = FindPointerSnap(State, Frame, Local, std::nullopt, Chain, {}, false);
            if (PointSnapCondition(Object.Kind))
            {
                Travel = Dot(Object.Position - Picker.LockOrigin, Picker.LockDirection);
                Note   = std::string(" · level with ") + DraftingSnapLabel(Object.Kind) + " " + Object.Source;
            }
            else if (!Free) Travel = std::round(Travel / DrawingSnap) * DrawingSnap;
            const Vector3 World = Picker.LockOrigin + Picker.LockDirection * Travel;
            Point          = Flatten(World);
            Picker.Current = { DraftingSnapKind::Axis, World, Picker.LockDirection, {}, std::string("Lock ") + DraftingSnapLockLabel(Picker.Lock) + Note, 0.0 };
        }
    }
    else
    {
        if (const std::optional<Vector2> Snapped = SnapOntoPlane(State, Frame, Local, Plane, Reference, Chain, {})) Point = *Snapped;
        else if (!Free) Point = { std::round(Point.X / DrawingSnap) * DrawingSnap, std::round(Point.Y / DrawingSnap) * DrawingSnap };
    }
    if (Drawing.Kind == DraftingDrawingKind::Rectangle && Drawing.SquareCondition && Drawing.Points.size() == 1)
    {
        const Vector2 From  = Drawing.Points.front();
        const double  Side  = std::max(std::abs(Point.X - From.X), std::abs(Point.Y - From.Y));
        Point = { From.X + (Point.X >= From.X ? Side : -Side), From.Y + (Point.Y >= From.Y ? Side : -Side) };
    }
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

[[nodiscard]] OrthonormalFrame ElevatedSketchPlane(const DraftingDocument& Document, const FeatureStep& Step)
{
    OrthonormalFrame Plane = Step.Plane;
    Plane.Origin = Plane.Origin + Plane.ZAxis * EvaluateFeatureValue(Document.Tree, Step.Elevation);
    return Plane;
}

void OpenTrimSession(DraftingWorkspaceState& State)
{
    const bool SketchCondition = std::any_of(State.Document.Tree.Steps.begin(), State.Document.Tree.Steps.end(), [](const FeatureStep& Step) { return Step.Kind == FeatureStepKind::Sketch; });
    if (!SketchCondition) { Announce(State, "Trim needs sketch curves: draw some first (Shift+A line, Shift+S curve, Shift+W circle …)"); return; }
    State.Trim = { true, false, {}, -1, {}, {}, {}, State.Trim.Commits };
    State.Drawing.ActiveCondition = false;
    State.Constrain.ActiveCondition = false;
    Announce(State, "Trim: click the piece of a curve to remove (between its crossings) · Esc ends");
}

void CloseTrimSession(DraftingWorkspaceState& State)
{
    State.Trim.ActiveCondition = false;
    State.Trim.HoverCondition  = false;
    State.Trim.Preview.clear();
    Announce(State, "Trim ended");
}

// 📝 The sketch curve nearest the pointer in pixels (≤ 8 px; later steps win ties), the pointer on its plane, and the
//    piece that would come off (red preview) — recomputed every frame from the solved sketches.
void HoverTrimPiece(DraftingWorkspaceState& State, const DraftingCameraFrame& Frame, ImVec2 Local, bool PointerCondition)
{
    DraftingTrimSession& Trim = State.Trim;
    Trim.HoverCondition = false;
    Trim.Preview.clear();
    Trim.Refusal.clear();
    if (!PointerCondition) return;
    const ImVec2       Mouse   = ImGui::GetIO().MousePos;
    double             Nearest = 8.0;                                                        // [px] - Pick radius
    const FeatureStep* Chosen  = nullptr;
    for (const FeatureStep& Step : State.Document.Tree.Steps)
    {
        if (Step.Kind != FeatureStepKind::Sketch) continue;
        const auto Solved = State.Document.Regeneration.Sketches.find(Step.Label);
        if (Solved == State.Document.Regeneration.Sketches.end()) continue;
        const OrthonormalFrame Plane = ElevatedSketchPlane(State.Document, Step);
        for (int Curve = 0; Curve < int(Solved->second.Curves.size()); ++Curve)
        {
            std::vector<ImVec2> Pixels;
            for (const Vector2& Sample : SampleSketchCurve(Solved->second, Solved->second.Curves[Curve]))
            {
                ImVec2 Pixel;
                if (ProjectWorldPoint(State, Frame, PlacePlanePoint(Plane, Sample), Pixel)) Pixels.push_back(Pixel);
            }
            for (size_t Index = 0; Index + 1 < Pixels.size(); ++Index)
            {
                const double AlongX = Pixels[Index + 1].x - Pixels[Index].x, AlongY = Pixels[Index + 1].y - Pixels[Index].y;
                const double Square = AlongX * AlongX + AlongY * AlongY;
                const double Share  = Square > 0.0 ? std::clamp(((Mouse.x - Pixels[Index].x) * AlongX + (Mouse.y - Pixels[Index].y) * AlongY) / Square, 0.0, 1.0) : 0.0;
                const double Gap    = std::hypot(Pixels[Index].x + AlongX * Share - Mouse.x, Pixels[Index].y + AlongY * Share - Mouse.y);
                if (Gap <= Nearest)
                {
                    Nearest    = Gap;
                    Chosen     = &Step;
                    Trim.Curve = Curve;
                }
            }
        }
    }
    if (!Chosen) return;
    const OrthonormalFrame Plane = ElevatedSketchPlane(State.Document, *Chosen);
    Vector2 Pick;
    if (!IntersectSketchPlane(Frame, Local, Plane, Pick)) return;
    Trim.Sketch         = Chosen->Label;
    Trim.Pick           = Pick;
    Trim.HoverCondition = true;
    const SketchTrimOutcome Outcome = PreviewDraftingTrim(State.Document, { Trim.Sketch, Trim.Curve, Trim.Pick });
    Trim.Refusal = Outcome.Refusal;
    if (Outcome.Refusal.empty()) for (const Vector2& Point : Outcome.Removed) Trim.Preview.push_back(PlacePlanePoint(Plane, Point));
}

void ApplyTrim(DraftingWorkspaceState& State)
{
    DraftingTrimSession& Trim = State.Trim;
    if (!Trim.HoverCondition) { Announce(State, "Trim: no curve under the pointer"); return; }
    const DraftingCommandOutcome Outcome = TrimDraftingSketch(State.Document, { Trim.Sketch, Trim.Curve, Trim.Pick });
    if (Outcome.AcceptedCondition)
    {
        ++Trim.Commits;
        Trim.HoverCondition = false;
        Trim.Preview.clear();
        const bool FocusedCondition = std::any_of(State.Document.Tree.Steps.begin(), State.Document.Tree.Steps.end(), [&](const FeatureStep& Step) { return Step.Label == State.FocusedStep; });
        if (!FocusedCondition) State.FocusedStep = {};
    }
    Conclude(State, Outcome);
}

void DrawTrimPreview(DraftingWorkspaceState& State, ImDrawList* Canvas, const DraftingCameraFrame& Frame)
{
    if (!State.Trim.ActiveCondition || State.Trim.Preview.size() < 2) return;
    std::vector<ImVec2> Pixels;
    for (const Vector3& Point : State.Trim.Preview) { ImVec2 Pixel; if (ProjectWorldPoint(State, Frame, Point, Pixel)) Pixels.push_back(Pixel); }
    if (Pixels.size() >= 2) Canvas->AddPolyline(Pixels.data(), int(Pixels.size()), IM_COL32(0xff, 0x4d, 0x4d, 0xff), 4.0f);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    CONSTRAINT SESSION (K)
//------------------------------------------------------------------------------------------------------------------------

std::string CaptionOf(const DraftingDocument& Document, const std::string& Label);
bool        CollectTypedAmount(std::string& Typed);
bool        ParseTypedAmount(const std::string& Typed, double& Value);

constexpr float  ConstraintReach   = 8.0f;                                                   // [px]  - Point / curve pick radius
constexpr float  GlyphOffset       = 14.0f;                                                  // [px]  - Glyph pill off its line
constexpr float  GlyphStride       = 24.0f;                                                  // [px]  - Next pill on the same line / point
constexpr float  DimensionOffset   = 22.0f;                                                  // [px]  - Dimension line off its feature
constexpr double ConstraintSnap    = 0.1;                                                    // [len] - Drag grid
constexpr ImU32  FreeColour        = IM_COL32(0x9f, 0xd8, 0xff, 0xff);                       // #9fd8ff  - Free point / curve
constexpr ImU32  ConstraintColour  = IM_COL32(0xe0, 0x8c, 0xff, 0xff);                       // #e08cff  - Glyphs (SolveSpace magenta)
constexpr ImU32  ReferenceColour   = IM_COL32(0xa8, 0xb0, 0xbc, 0xff);                       // #a8b0bc  - Reference (driven) dimensions
constexpr ImU32  LinkColour        = IM_COL32(0xff, 0xc1, 0x4d, 0xff);                       // #ffc14d  - Linked (projected) geometry, amber
constexpr ImU32  OtherSketchColour = IM_COL32(0xc9, 0xd1, 0xdc, 0xd0);                       // #c9d1dc  - Earlier sketches (dashed, pickable)
constexpr ImU32  OtherSketchCasing = IM_COL32(0x20, 0x24, 0x2c, 0xb0);                       // #20242c  - Their dark casing (reads on faces too)

// 📝 One clickable constraint mark in pixels: a pill (letter or stroke symbol) or a dimension (lines + value pill).
struct ConstraintGlyph
{
    int                                    Constraint = -1;                                   // [-]  - Index in the sketch
    ImVec2                                 Centre     = {};                                   // [px] - Pill centre
    ImVec2                                 HalfExtent = {};                                   // [px] - Pill half size
    std::string                            Text       = {};                                   // [-]  - Pill text (empty: Symbol)
    int                                    Symbol     = 0;                                    // [-]  - 0 text · 1 parallel strokes · 2 perpendicular strokes
    std::vector<std::pair<ImVec2, ImVec2>> Strokes    = {};                                   // [px] - Dimension / extension / leader lines
};

// The session sketch as regenerated: solved model, solve report, elevated plane; Sketch null when the step is gone.
struct ConstraintView
{
    const FeatureStep*        Step   = nullptr;                                              // [-] - Sketch step
    const SketchModel*        Sketch = nullptr;                                              // [-] - Solved sketch
    const SketchSolveOutcome* Solve  = nullptr;                                              // [-] - Its report (null: none)
    OrthonormalFrame          Plane  = {};                                                   // [-] - Elevated plane
};

std::string ConstraintSketchLabel(const DraftingWorkspaceState& State)
{
    const std::string Label = State.Constrain.ActiveCondition ? State.Constrain.Sketch : State.FocusedStep;
    for (const FeatureStep& Step : State.Document.Tree.Steps) if (Step.Label == Label && Step.Kind == FeatureStepKind::Sketch) return Label;
    return {};
}

[[nodiscard]] ConstraintView ViewConstraintSketch(const DraftingWorkspaceState& State, const std::string& Label)
{
    ConstraintView View;
    for (const FeatureStep& Step : State.Document.Tree.Steps) if (Step.Label == Label && Step.Kind == FeatureStepKind::Sketch) View.Step = &Step;
    if (!View.Step) return View;
    const auto Solved = State.Document.Regeneration.Sketches.find(Label);
    View.Sketch = Solved != State.Document.Regeneration.Sketches.end() ? &Solved->second : &View.Step->Sketch;
    for (const FeatureStepReport& Report : State.Document.Regeneration.Steps) if (Report.Label == Label) View.Solve = &Report.Solve;
    View.Plane = ElevatedSketchPlane(State.Document, *View.Step);
    return View;
}

[[nodiscard]] SketchConstraintStatus ConstraintStatusOf(const ConstraintView& View, int Constraint)
{
    if (!View.Solve || Constraint < 0 || Constraint >= int(View.Solve->Constraints.size())) return SketchConstraintStatus::Satisfied;
    return View.Solve->Constraints[Constraint].Status;
}

[[nodiscard]] ImVec2 Unit(ImVec2 Vector, ImVec2 Fallback)
{
    const float Size = std::hypot(Vector.x, Vector.y);
    return Size > 1.0e-6f ? ImVec2(Vector.x / Size, Vector.y / Size) : Fallback;
}

// 📝 Glyph layout from the solved sketch: dimensions stand outside (away from the sketch's pixel centroid) as offset
//    lines with the value, letter pills inside beside line middles (sliding along the line for each further pill), at
//    points and on circle tops; angles as an arc at the lines' meeting. A lone line: dimensions above, letters below.
std::vector<ConstraintGlyph> LayoutConstraintGlyphs(const DraftingWorkspaceState& State, const DraftingCameraFrame& Frame, const SketchModel& Sketch, const OrthonormalFrame& Plane)
{
    std::vector<ConstraintGlyph> Glyphs;
    std::map<int, int> CurveSlots, PointSlots;
    auto Pixel = [&](Vector2 Point) { ImVec2 Out(-1.0e6f, -1.0e6f); ProjectWorldPoint(State, Frame, PlacePlanePoint(Plane, Point), Out); return Out; };
    auto PointPixel = [&](int Point) { return Point >= 0 && Point < int(Sketch.Points.size()) ? Pixel(Sketch.Points[Point].Position) : ImVec2(-1.0e6f, -1.0e6f); };
    auto CurveOf = [&](int Curve) -> const SketchCurve* { return Curve >= 0 && Curve < int(Sketch.Curves.size()) ? &Sketch.Curves[Curve] : nullptr; };
    ImVec2 Centroid(0.0f, 0.0f);
    for (int Point = 0; Point < int(Sketch.Points.size()); ++Point) { const ImVec2 At = PointPixel(Point); Centroid.x += At.x / float(Sketch.Points.size()); Centroid.y += At.y / float(Sketch.Points.size()); }
    // Normal of the screen segment From → To pointing away from the centroid (up the screen when the centroid is on it).
    auto Outward = [&](ImVec2 From, ImVec2 To)
    {
        const ImVec2 Along = Unit(ImVec2(To.x - From.x, To.y - From.y), ImVec2(1, 0));
        ImVec2 Normal(-Along.y, Along.x);
        const float Side = Normal.x * (0.5f * (From.x + To.x) - Centroid.x) + Normal.y * (0.5f * (From.y + To.y) - Centroid.y);
        if (std::abs(Side) > 2.0f) return Side < 0.0f ? ImVec2(-Normal.x, -Normal.y) : Normal;
        return Normal.y > 0.0f || (Normal.y == 0.0f && Normal.x < 0.0f) ? ImVec2(-Normal.x, -Normal.y) : Normal;
    };
    auto Pill = [&](int Constraint, ImVec2 Centre, const std::string& Text, int Symbol)
    {
        ConstraintGlyph Glyph;
        Glyph.Constraint = Constraint;
        Glyph.Centre     = Centre;
        Glyph.Text       = Text;
        Glyph.Symbol     = Symbol;
        Glyph.HalfExtent = Symbol ? ImVec2(10.0f, 9.0f) : ImVec2(ImGui::CalcTextSize(Text.c_str()).x * 0.5f + 6.0f, 9.0f);
        Glyphs.push_back(Glyph);
        return &Glyphs.back();
    };
    // Screen frame of a line: start, end, unit direction and its outward normal.
    auto LineFrame = [&](const SketchCurve& Curve, ImVec2& Start, ImVec2& End, ImVec2& Along, ImVec2& Normal)
    {
        Start  = PointPixel(Curve.Start);
        End    = PointPixel(Curve.End);
        Along  = Unit(ImVec2(End.x - Start.x, End.y - Start.y), ImVec2(1, 0));
        Normal = Outward(Start, End);
    };
    auto OnCurve = [&](int Constraint, int Curve, const std::string& Text, int Symbol)
    {
        const SketchCurve* Shape = CurveOf(Curve);
        if (!Shape) return;
        const int Slot = CurveSlots[Curve]++;
        if (Shape->Kind == SketchCurveKind::Line)
        {
            ImVec2 Start, End, Along, Normal;
            LineFrame(*Shape, Start, End, Along, Normal);
            const float Shift = float(Slot) * GlyphStride;
            Pill(Constraint, ImVec2(0.5f * (Start.x + End.x) - Normal.x * GlyphOffset + Along.x * Shift, 0.5f * (Start.y + End.y) - Normal.y * GlyphOffset + Along.y * Shift), Text, Symbol);
        }
        else if (Shape->Kind == SketchCurveKind::Circle || Shape->Kind == SketchCurveKind::Arc)
        {
            const Vector2 Centre = Sketch.Points[Shape->Centre].Position;
            const double  Radius = MeasureSketchRadius(Sketch, *Shape);
            const ImVec2  Top    = Pixel({ Centre.X, Centre.Y + Radius });
            Pill(Constraint, ImVec2(Top.x + float(Slot) * GlyphStride, Top.y - GlyphOffset), Text, Symbol);
        }
    };
    auto AtPoint = [&](int Constraint, int Point, const std::string& Text)
    {
        const ImVec2 At   = PointPixel(Point);
        const int    Slot = PointSlots[Point]++;
        Pill(Constraint, ImVec2(At.x + 14.0f + float(Slot) * GlyphStride, At.y - 14.0f), Text, 0);
    };
    // Dimension between two screen points, offset along Normal, value pill in the middle.
    auto Dimension = [&](int Constraint, ImVec2 From, ImVec2 To, ImVec2 Normal, const std::string& Text)
    {
        const ImVec2 A(From.x + Normal.x * DimensionOffset, From.y + Normal.y * DimensionOffset);
        const ImVec2 B(To.x + Normal.x * DimensionOffset, To.y + Normal.y * DimensionOffset);
        ConstraintGlyph* Glyph = Pill(Constraint, ImVec2(0.5f * (A.x + B.x), 0.5f * (A.y + B.y)), Text, 0);
        Glyph->Strokes = { { A, B }, { From, ImVec2(A.x + Normal.x * 4.0f, A.y + Normal.y * 4.0f) }, { To, ImVec2(B.x + Normal.x * 4.0f, B.y + Normal.y * 4.0f) } };
    };
    for (int Index = 0; Index < int(Sketch.Constraints.size()); ++Index)
    {
        const SketchConstraint& Constraint = Sketch.Constraints[Index];
        switch (Constraint.Kind)
        {
        case SketchConstraintKind::Horizontal:    OnCurve(Index, Constraint.First, "H", 0); break;
        case SketchConstraintKind::Vertical:      OnCurve(Index, Constraint.First, "V", 0); break;
        case SketchConstraintKind::Parallel:      OnCurve(Index, Constraint.First, {}, 1); OnCurve(Index, Constraint.Second, {}, 1); break;
        case SketchConstraintKind::Perpendicular: OnCurve(Index, Constraint.First, {}, 2); OnCurve(Index, Constraint.Second, {}, 2); break;
        case SketchConstraintKind::EqualLength:
        case SketchConstraintKind::EqualRadius:   OnCurve(Index, Constraint.First, "=", 0); OnCurve(Index, Constraint.Second, "=", 0); break;
        case SketchConstraintKind::Coincident:    AtPoint(Index, Constraint.First, "C"); break;
        case SketchConstraintKind::Fixed:         AtPoint(Index, Constraint.First, "F"); break;
        case SketchConstraintKind::PointOnLine:
        case SketchConstraintKind::PointOnCircle: AtPoint(Index, Constraint.First, "O"); break;
        case SketchConstraintKind::Midpoint:      AtPoint(Index, Constraint.First, "M"); break;
        case SketchConstraintKind::Concentric:    if (const SketchCurve* Shape = CurveOf(Constraint.First)) AtPoint(Index, Shape->Centre, "CC"); break;
        case SketchConstraintKind::Tangent:
            if (const SketchCurve* Shape = CurveOf(Constraint.First); Shape && Shape->Kind == SketchCurveKind::Line) OnCurve(Index, Constraint.First, "T", 0);
            else OnCurve(Index, Constraint.Second, "T", 0);
            break;
        case SketchConstraintKind::SplineTangent:   AtPoint(Index, Constraint.Third, "G1"); break;
        case SketchConstraintKind::SplineCurvature: AtPoint(Index, Constraint.Third, "G2"); break;
        case SketchConstraintKind::PointOnSpline:   AtPoint(Index, Constraint.First, "O"); break;
        case SketchConstraintKind::SplineDirection: AtPoint(Index, Constraint.Third, std::abs(Constraint.Value) < 1.0e-12 ? "H" : "V"); break;
        case SketchConstraintKind::Symmetric:
        {
            const ImVec2 A = PointPixel(Constraint.First), B = PointPixel(Constraint.Second);
            Pill(Index, ImVec2(0.5f * (A.x + B.x), 0.5f * (A.y + B.y) - GlyphOffset), "Y", 0);
            break;
        }
        case SketchConstraintKind::Length:
            if (const SketchCurve* Shape = CurveOf(Constraint.First); Shape && Shape->Kind == SketchCurveKind::Line)
            {
                ImVec2 Start, End, Along, Normal;
                LineFrame(*Shape, Start, End, Along, Normal);
                Dimension(Index, Start, End, Normal, FormatLength(MeasureSketchConstraint(Sketch, Constraint)));
            }
            break;
        case SketchConstraintKind::Distance:
        {
            const ImVec2 A = PointPixel(Constraint.First), B = PointPixel(Constraint.Second);
            Dimension(Index, A, B, Outward(A, B), FormatLength(MeasureSketchConstraint(Sketch, Constraint)));
            break;
        }
        case SketchConstraintKind::HorizontalDistance:
        case SketchConstraintKind::VerticalDistance:
        {
            const bool Across = Constraint.Kind == SketchConstraintKind::HorizontalDistance;
            if (Constraint.Value == 0.0 && Constraint.Parameter.empty())
            {
                // Level / plumb points (H / V on two points): a letter between them.
                const ImVec2 A = PointPixel(Constraint.First), B = PointPixel(Constraint.Second);
                Pill(Index, ImVec2(0.5f * (A.x + B.x), 0.5f * (A.y + B.y) - GlyphOffset), Across ? "V" : "H", 0);
                break;
            }
            if (Constraint.First < 0 || Constraint.Second < 0 || Constraint.First >= int(Sketch.Points.size()) || Constraint.Second >= int(Sketch.Points.size())) break;
            const Vector2 First = Sketch.Points[Constraint.First].Position, Second = Sketch.Points[Constraint.Second].Position;
            const Vector2 Corner = Across ? Vector2{ Second.X, First.Y } : Vector2{ First.X, Second.Y };
            const ImVec2  A = Pixel(First), B = Pixel(Corner);
            Dimension(Index, A, B, Outward(A, B), FormatLength(std::abs(MeasureSketchConstraint(Sketch, Constraint))));
            break;
        }
        case SketchConstraintKind::Radius:
            if (const SketchCurve* Shape = CurveOf(Constraint.First); Shape && Shape->Centre >= 0)
            {
                const Vector2 Centre = Sketch.Points[Shape->Centre].Position;
                const double  Radius = MeasureSketchRadius(Sketch, *Shape);
                const double  Slant  = std::numbers::pi * 0.25;
                const ImVec2  Middle = Pixel(Centre), Rim = Pixel({ Centre.X + Radius * std::cos(Slant), Centre.Y + Radius * std::sin(Slant) });
                const ImVec2  Out    = Unit(ImVec2(Rim.x - Middle.x, Rim.y - Middle.y), ImVec2(0.7f, -0.7f));
                const std::string Text = "R" + FormatLength(Radius);
                const float   Reach  = ImGui::CalcTextSize(Text.c_str()).x * 0.5f + 12.0f;
                ConstraintGlyph* Glyph = Pill(Index, ImVec2(Rim.x + Out.x * Reach, Rim.y + Out.y * Reach), Text, 0);
                Glyph->Strokes = { { Middle, Rim } };
            }
            break;
        case SketchConstraintKind::Angle:
        {
            const SketchCurve* FirstLine = CurveOf(Constraint.First);
            const SketchCurve* SecondLine = CurveOf(Constraint.Second);
            if (!FirstLine || !SecondLine || FirstLine->Kind != SketchCurveKind::Line || SecondLine->Kind != SketchCurveKind::Line) break;
            const Vector2 P0 = Sketch.Points[FirstLine->Start].Position, P1 = Sketch.Points[FirstLine->End].Position;
            const Vector2 Q0 = Sketch.Points[SecondLine->Start].Position, Q1 = Sketch.Points[SecondLine->End].Position;
            const Vector2 U = P1 - P0, W = Q1 - Q0;
            const double  Turn = U.X * W.Y - U.Y * W.X;
            const Vector2 MiddleP = (P0 + P1) * 0.5, MiddleQ = (Q0 + Q1) * 0.5;
            Vector2 Meeting = MiddleQ;
            if (std::abs(Turn) > 1.0e-12 * std::max(1.0, Dot(U, U) + Dot(W, W)))
            {
                const Vector2 Gap = Q0 - P0;
                Meeting = P0 + U * ((Gap.X * W.Y - Gap.Y * W.X) / Turn);
            }
            const ImVec2 Hub = Pixel(Meeting);
            auto Towards = [&](Vector2 Middle, Vector2 Direction)
            {
                const Vector2 Away = Dot(Middle - Meeting, Direction) >= 0.0 ? Direction : Direction * -1.0;
                const ImVec2 Far = Pixel(Meeting + Away * (1.0 / std::max(1.0e-12, Length(Away))) * std::max(1.0e-6, 0.1 * Length(Middle - Meeting) + 1.0e-3));
                return Unit(ImVec2(Far.x - Hub.x, Far.y - Hub.y), ImVec2(1, 0));
            };
            const ImVec2 A = Towards(MiddleP, U), B = Towards(MiddleQ, W);
            float From = std::atan2(A.y, A.x), Sweep = std::atan2(B.y, B.x) - From;
            while (Sweep > float(std::numbers::pi)) Sweep -= 2.0f * float(std::numbers::pi);
            while (Sweep < -float(std::numbers::pi)) Sweep += 2.0f * float(std::numbers::pi);
            char Text[32];
            std::snprintf(Text, sizeof(Text), "%.4g°", std::abs(MeasureSketchConstraint(Sketch, Constraint)) * 180.0 / std::numbers::pi);
            const float  Middle = From + Sweep * 0.5f;
            ConstraintGlyph* Glyph = Pill(Index, ImVec2(Hub.x + std::cos(Middle) * 46.0f, Hub.y + std::sin(Middle) * 46.0f), Text, 0);
            constexpr int Steps = 16;
            for (int Step = 0; Step < Steps; ++Step)
            {
                const float T0 = From + Sweep * float(Step) / Steps, T1 = From + Sweep * float(Step + 1) / Steps;
                Glyph->Strokes.push_back({ ImVec2(Hub.x + std::cos(T0) * 28.0f, Hub.y + std::sin(T0) * 28.0f), ImVec2(Hub.x + std::cos(T1) * 28.0f, Hub.y + std::sin(T1) * 28.0f) });
            }
            break;
        }
        default: break;
        }
        // A reference (driven) dimension reads in parentheses, SolveSpace style.
        if (Constraint.DrivenCondition)
            for (ConstraintGlyph& Glyph : Glyphs)
                if (Glyph.Constraint == Index && !Glyph.Text.empty() && Glyph.Text.front() != '(')
                {
                    Glyph.Text         = "(" + Glyph.Text + ")";
                    Glyph.HalfExtent.x = ImGui::CalcTextSize(Glyph.Text.c_str()).x * 0.5f + 6.0f;
                }
    }
    return Glyphs;
}

// An earlier sketch step the session may link to, as regenerated.
struct OtherSketch
{
    std::string        Label  = {};                                                          // [-] - Sketch step label
    const SketchModel* Sketch = nullptr;                                                     // [-] - Solved sketch
    OrthonormalFrame   Plane  = {};                                                          // [-] - Elevated plane
};

[[nodiscard]] std::vector<OtherSketch> OtherSketches(const DraftingWorkspaceState& State)
{
    std::vector<OtherSketch> Sketches;
    for (const std::string& Label : DraftingLinkSources(State.Document, State.Constrain.Sketch))
    {
        const auto Solved = State.Document.Regeneration.Sketches.find(Label);
        if (Solved == State.Document.Regeneration.Sketches.end()) continue;
        for (const FeatureStep& Step : State.Document.Tree.Steps)
            if (Step.Label == Label && Step.Kind == FeatureStepKind::Sketch) Sketches.push_back({ Label, &Solved->second, ElevatedSketchPlane(State.Document, Step) });
    }
    return Sketches;
}

[[nodiscard]] bool SamePick(const DraftingSketchPick& First, const DraftingSketchPick& Second) noexcept
{
    return First.Sketch == Second.Sketch && First.Pick.Kind == Second.Pick.Kind && First.Pick.Index == Second.Pick.Index;
}

void OpenConstraintSession(DraftingWorkspaceState& State)
{
    std::string Label = FocusedSketchCondition(State) ? State.FocusedStep : std::string{};
    if (Label.empty())
        for (const FeatureStep& Step : State.Document.Tree.Steps) if (Step.Kind == FeatureStepKind::Sketch) Label = Step.Label;   // newest sketch
    if (Label.empty()) { Announce(State, "Constrain (K) needs a sketch: draw one first (Shift+A line, Shift+Q rectangle, Shift+W circle …)"); return; }
    const int Commits = State.Constrain.Commits;
    State.Constrain                 = {};
    State.Constrain.ActiveCondition = true;
    State.Constrain.Sketch          = Label;
    State.Constrain.Commits         = Commits;
    State.Drawing.ActiveCondition   = false;
    State.Trim.ActiveCondition      = false;
    State.FocusedStep               = Label;
    const ConstraintView View = ViewConstraintSketch(State, Label);
    Announce(State, Compose("Constrain %s · %d constraint(s) · DOF %d · click points / curves (earlier sketches link), then D N H V O Q M Y L [ ] C · Shift+D reference · drag points · Esc ends", CaptionOf(State.Document, Label).c_str(),
                            int(View.Sketch->Constraints.size()), View.Solve ? View.Solve->DegreesOfFreedom : -1));
}

void CloseConstraintSession(DraftingWorkspaceState& State)
{
    State.Constrain.ActiveCondition = false;
    State.Constrain.DragCondition   = false;
    State.Constrain.PressCondition  = false;
    Announce(State, "Constrain ended");
}

// Drops picks that no longer exist (after undo) and ends the session when its sketch is gone.
void ReconcileConstraintSession(DraftingWorkspaceState& State)
{
    DraftingConstraintSession& Session = State.Constrain;
    const ConstraintView View = ViewConstraintSketch(State, Session.Sketch);
    if (!View.Step) { CloseConstraintSession(State); return; }
    std::erase_if(Session.Picks, [&](const DraftingSketchPick& Pick)
    {
        const SketchModel* Sketch = View.Sketch;
        if (!Pick.Sketch.empty())
        {
            const auto Solved = State.Document.Regeneration.Sketches.find(Pick.Sketch);
            Sketch = Solved != State.Document.Regeneration.Sketches.end() ? &Solved->second : nullptr;
        }
        return !Sketch || Pick.Pick.Index < 0 || Pick.Pick.Index >= int(Pick.Pick.Kind == SketchPickKind::Point ? Sketch->Points.size() : Sketch->Curves.size());
    });
    if (Session.PickedConstraint >= int(View.Sketch->Constraints.size())) Session.PickedConstraint = -1;
}

// 📝 Under the pointer, in order: a point (≤ 8 px), a constraint glyph, a curve (≤ 8 px) of the session sketch, then a
//    point and a curve of an earlier sketch (for links).
void HoverConstraintTarget(DraftingWorkspaceState& State, const DraftingCameraFrame& Frame, const ConstraintView& View, const std::vector<ConstraintGlyph>& Glyphs)
{
    DraftingConstraintSession& Session = State.Constrain;
    Session.HoverCondition  = false;
    Session.HoverConstraint = -1;
    const ImVec2 Mouse   = ImGui::GetIO().MousePos;
    double       Nearest = ConstraintReach;
    auto NearestPoint = [&](const SketchModel& Sketch, const OrthonormalFrame& Plane, const std::string& Label)
    {
        for (int Point = 0; Point < int(Sketch.Points.size()); ++Point)
        {
            ImVec2 Pixel;
            if (!ProjectWorldPoint(State, Frame, PlacePlanePoint(Plane, Sketch.Points[Point].Position), Pixel)) continue;
            const double Gap = std::hypot(Pixel.x - Mouse.x, Pixel.y - Mouse.y);
            if (Gap <= Nearest) { Nearest = Gap; Session.HoverCondition = true; Session.Hover = { Label, { SketchPickKind::Point, Point } }; }
        }
    };
    auto NearestCurve = [&](const SketchModel& Sketch, const OrthonormalFrame& Plane, const std::string& Label)
    {
        for (int Curve = 0; Curve < int(Sketch.Curves.size()); ++Curve)
        {
            std::vector<ImVec2> Pixels;
            for (const Vector2& Sample : SampleSketchCurve(Sketch, Sketch.Curves[Curve]))
            {
                ImVec2 Pixel;
                if (ProjectWorldPoint(State, Frame, PlacePlanePoint(Plane, Sample), Pixel)) Pixels.push_back(Pixel);
            }
            for (size_t Index = 0; Index + 1 < Pixels.size(); ++Index)
            {
                const double AlongX = Pixels[Index + 1].x - Pixels[Index].x, AlongY = Pixels[Index + 1].y - Pixels[Index].y;
                const double Square = AlongX * AlongX + AlongY * AlongY;
                const double Share  = Square > 0.0 ? std::clamp(((Mouse.x - Pixels[Index].x) * AlongX + (Mouse.y - Pixels[Index].y) * AlongY) / Square, 0.0, 1.0) : 0.0;
                const double Gap    = std::hypot(Pixels[Index].x + AlongX * Share - Mouse.x, Pixels[Index].y + AlongY * Share - Mouse.y);
                if (Gap <= Nearest) { Nearest = Gap; Session.HoverCondition = true; Session.Hover = { Label, { SketchPickKind::Curve, Curve } }; }
            }
        }
    };
    NearestPoint(*View.Sketch, View.Plane, {});
    if (Session.HoverCondition) return;
    for (const ConstraintGlyph& Glyph : Glyphs)
        if (std::abs(Mouse.x - Glyph.Centre.x) <= Glyph.HalfExtent.x + 2.0f && std::abs(Mouse.y - Glyph.Centre.y) <= Glyph.HalfExtent.y + 2.0f) { Session.HoverConstraint = Glyph.Constraint; return; }
    NearestCurve(*View.Sketch, View.Plane, {});
    if (Session.HoverCondition) return;
    const std::vector<OtherSketch> Others = OtherSketches(State);
    for (const OtherSketch& Other : Others) NearestPoint(*Other.Sketch, Other.Plane, Other.Label);
    if (Session.HoverCondition) return;
    for (const OtherSketch& Other : Others) NearestCurve(*Other.Sketch, Other.Plane, Other.Label);
}

// "Line1.End", or "Line2.End of Rectangle1" for an earlier sketch's geometry.
[[nodiscard]] std::string NameSessionPick(const DraftingWorkspaceState& State, const DraftingSketchPick& Pick)
{
    const std::string Label  = Pick.Sketch.empty() ? State.Constrain.Sketch : Pick.Sketch;
    const auto        Solved = State.Document.Regeneration.Sketches.find(Label);
    if (Solved == State.Document.Regeneration.Sketches.end()) return "?";
    const std::string Name = NameSketchPick(Solved->second, Pick.Pick);
    return Pick.Sketch.empty() ? Name : Name + " of " + CaptionOf(State.Document, Pick.Sketch);
}

[[nodiscard]] std::string ListConstraintPicks(const DraftingWorkspaceState& State)
{
    const ConstraintView View = ViewConstraintSketch(State, State.Constrain.Sketch);
    std::string Names;
    if (!View.Sketch) return Names;
    for (const DraftingSketchPick& Pick : State.Constrain.Picks) Names += (Names.empty() ? "" : ", ") + NameSessionPick(State, Pick);
    return Names;
}

void ConcludeConstraint(DraftingWorkspaceState& State, const DraftingCommandOutcome& Outcome)
{
    DraftingConstraintSession& Session = State.Constrain;
    Session.Refusal = Outcome.AcceptedCondition ? std::string{} : Outcome.Refusal;
    if (Outcome.AcceptedCondition) ++Session.Commits;
    Conclude(State, Outcome);
}

void CommitConstraint(DraftingWorkspaceState& State, SketchConstraint Constraint)
{
    DraftingConstraintSession& Session = State.Constrain;
    const DraftingCommandOutcome Outcome = ConstrainDraftingSketch(State.Document, Session.Sketch, Constraint, Session.Picks);
    ConcludeConstraint(State, Outcome);
    if (!Outcome.AcceptedCondition) return;
    Session.Picks.clear();
    Session.PendingCondition = false;
    Session.Typed.clear();
    const ConstraintView View = ViewConstraintSketch(State, Session.Sketch);
    Session.PickedConstraint = View.Sketch && SketchDimensionCondition(Constraint.Kind) ? int(View.Sketch->Constraints.size()) - 1 : -1;
}

// 📝 A verb key: the picks become a constraint (D / N wait for a value first), or the refusal says why not.
void ApplyConstraintVerb(DraftingWorkspaceState& State, SketchConstraintVerb Verb)
{
    DraftingConstraintSession& Session = State.Constrain;
    const DraftingLinkedPicks Linked = LinkDraftingPicks(State.Document, Session.Sketch, Session.Picks);
    if (!Linked.Refusal.empty())
    {
        Session.Refusal = Linked.Refusal;
        Announce(State, "Refused: " + Linked.Refusal);
        return;
    }
    const SketchModel* const View = &Linked.Sketch;
    const SketchConstraintProposal Proposal = ProposeSketchConstraint(Linked.Sketch, Linked.Picks, Verb);
    if (!Proposal.Refusal.empty())
    {
        Session.Refusal = Proposal.Refusal;
        Announce(State, "Refused: " + Proposal.Refusal);
        return;
    }
    Session.Refusal.clear();
    if (Verb == SketchConstraintVerb::Dimension || Verb == SketchConstraintVerb::Angle)
    {
        Session.PendingCondition = true;
        Session.Proposed         = Proposal.Constraint;
        Session.Typed.clear();
        Session.PickedConstraint = -1;
        Announce(State, Compose("%s: type a value (now %s) · Enter applies · Esc drops", DescribeSketchConstraint(*View, Proposal.Constraint).c_str(),
                                Verb == SketchConstraintVerb::Angle ? Compose("%.4g°", Proposal.Constraint.Value * 180.0 / std::numbers::pi).c_str() : FormatLength(Proposal.Constraint.Value).c_str()));
        return;
    }
    CommitConstraint(State, Proposal.Constraint);
}

// Shift+D: the picks become a reference (driven) dimension at once; it reads, it never drives.
void ApplyConstraintReference(DraftingWorkspaceState& State)
{
    DraftingConstraintSession& Session = State.Constrain;
    const DraftingLinkedPicks Linked = LinkDraftingPicks(State.Document, Session.Sketch, Session.Picks);
    const SketchConstraintProposal Proposal = Linked.Refusal.empty() ? ProposeSketchReference(Linked.Sketch, Linked.Picks) : SketchConstraintProposal{ {}, Linked.Refusal };
    if (!Proposal.Refusal.empty())
    {
        Session.Refusal = Proposal.Refusal;
        Announce(State, "Refused: " + Proposal.Refusal);
        return;
    }
    Session.Refusal.clear();
    Session.PendingCondition = false;
    CommitConstraint(State, Proposal.Constraint);
}

// E: the picked dimension between driving and reference.
void TogglePickedReference(DraftingWorkspaceState& State)
{
    DraftingConstraintSession& Session = State.Constrain;
    if (Session.PickedConstraint < 0) { Announce(State, "Reference (E): click a dimension first"); return; }
    ConcludeConstraint(State, ToggleDraftingReference(State.Document, Session.Sketch, Session.PickedConstraint));
}

// Delete on one picked linked point / curve: the link dropped, the geometry stays (free).
void UnlinkPickedGeometry(DraftingWorkspaceState& State)
{
    DraftingConstraintSession& Session = State.Constrain;
    const DraftingCommandOutcome Outcome = UnlinkDraftingGeometry(State.Document, Session.Sketch, Session.Picks.front().Pick);
    ConcludeConstraint(State, Outcome);
    if (Outcome.AcceptedCondition) Session.Picks.clear();
}

void CommitPendingConstraint(DraftingWorkspaceState& State)
{
    DraftingConstraintSession& Session = State.Constrain;
    SketchConstraint Constraint = Session.Proposed;
    double Typed = 0.0;
    if (ParseTypedAmount(Session.Typed, Typed)) Constraint.Value = Constraint.Kind == SketchConstraintKind::Angle ? Typed * std::numbers::pi / 180.0 : Typed;
    CommitConstraint(State, Constraint);
}

void RemovePickedConstraint(DraftingWorkspaceState& State)
{
    DraftingConstraintSession& Session = State.Constrain;
    if (Session.PickedConstraint < 0) { Announce(State, "Remove: click a constraint glyph or dimension first"); return; }
    const DraftingCommandOutcome Outcome = RemoveDraftingConstraint(State.Document, Session.Sketch, Session.PickedConstraint);
    ConcludeConstraint(State, Outcome);
    if (Outcome.AcceptedCondition) Session.PickedConstraint = -1;
}

void EditPickedDimension(DraftingWorkspaceState& State)
{
    DraftingConstraintSession& Session = State.Constrain;
    const ConstraintView View = ViewConstraintSketch(State, Session.Sketch);
    double Value = 0.0;
    if (!View.Sketch || Session.PickedConstraint < 0 || !ParseTypedAmount(Session.Typed, Value)) return;
    const bool Angular = View.Sketch->Constraints[Session.PickedConstraint].Kind == SketchConstraintKind::Angle;
    ConcludeConstraint(State, ChangeDraftingDimension(State.Document, Session.Sketch, Session.PickedConstraint, Angular ? Value * std::numbers::pi / 180.0 : Value));
    Session.Typed.clear();
}

void ClickConstraintTarget(DraftingWorkspaceState& State)
{
    DraftingConstraintSession& Session = State.Constrain;
    Session.Refusal.clear();
    if (Session.HoverCondition)
    {
        const auto Found = std::find_if(Session.Picks.begin(), Session.Picks.end(), [&](const DraftingSketchPick& Pick) { return SamePick(Pick, Session.Hover); });
        if (Found != Session.Picks.end()) Session.Picks.erase(Found);
        else Session.Picks.push_back(Session.Hover);
        Session.PickedConstraint = -1;
        Session.PendingCondition = false;
        Session.Typed.clear();
        Announce(State, Session.Picks.empty() ? std::string("No picks") : "Picked " + ListConstraintPicks(State));
        return;
    }
    if (Session.HoverConstraint >= 0)
    {
        const ConstraintView View = ViewConstraintSketch(State, Session.Sketch);
        Session.PickedConstraint = Session.HoverConstraint;
        Session.Picks.clear();
        Session.PendingCondition = false;
        Session.Typed.clear();
        const SketchConstraint& Constraint = View.Sketch->Constraints[Session.PickedConstraint];
        Announce(State, Compose("%s%s%s · %s", Constraint.Label.c_str(), Constraint.Label.empty() ? "" : " ", DescribeSketchConstraint(*View.Sketch, Constraint).c_str(),
                                !SketchDimensionCondition(Constraint.Kind) ? "Delete removes" : Constraint.DrivenCondition ? "E makes it driving · Delete removes" : "type a value + Enter · E makes it a reference · Delete removes"));
        return;
    }
    Session.Picks.clear();
    Session.PickedConstraint = -1;
    Session.PendingCondition = false;
    Session.Typed.clear();
}

// 📝 Viewport gestures while K is open: hover, click picks, press-travel-release drags a point.
void DriveConstraintSession(DraftingWorkspaceState& State, const DraftingCameraFrame& Frame, ImVec2 Local, bool PointerCondition, bool LeftClick, bool RightClick)
{
    ReconcileConstraintSession(State);
    DraftingConstraintSession& Session = State.Constrain;
    if (!Session.ActiveCondition) return;
    const ConstraintView View = ViewConstraintSketch(State, Session.Sketch);
    ImGuiIO& Io = ImGui::GetIO();
    if (!Session.DragCondition)
    {
        if (PointerCondition) HoverConstraintTarget(State, Frame, View, LayoutConstraintGlyphs(State, Frame, *View.Sketch, View.Plane));
        else { Session.HoverCondition = false; Session.HoverConstraint = -1; }
    }
    if (PointerCondition && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && Session.HoverCondition && Session.Hover.Sketch.empty() && Session.Hover.Pick.Kind == SketchPickKind::Point)
    {
        Session.PressCondition = true;
        Session.DragPoint      = Session.Hover.Pick.Index;
    }
    if (Session.PressCondition && ImGui::IsMouseDown(ImGuiMouseButton_Left) && Io.MouseDragMaxDistanceSqr[ImGuiMouseButton_Left] > ClickTolerance * ClickTolerance)
    {
        Vector2 Target;
        if (IntersectSketchPlane(Frame, Local, View.Plane, Target))
        {
            // Object snaps first (never onto the dragged sketch itself), else the grid; Ctrl free
            if (const std::optional<Vector2> Snapped = SnapOntoPlane(State, Frame, Local, View.Plane, std::nullopt, {}, { Session.Sketch })) Target = *Snapped;
            else if (!Io.KeyCtrl) Target = { std::round(Target.X / ConstraintSnap) * ConstraintSnap, std::round(Target.Y / ConstraintSnap) * ConstraintSnap };
            if (!Session.DragCondition || Target.X != Session.DragTarget.X || Target.Y != Session.DragTarget.Y)
            {
                Session.DragCondition = true;
                Session.DragTarget    = Target;
                const SketchDragOutcome Preview = PreviewDraftingDrag(State.Document, Session.Sketch, Session.DragPoint, Target);
                Session.DragPreviewCondition = Preview.Refusal.empty();
                Session.Refusal              = Preview.Refusal;
                if (Session.DragPreviewCondition) Session.DragPreview = Preview.Sketch;
            }
        }
    }
    if (Session.PressCondition && (ImGui::IsMouseReleased(ImGuiMouseButton_Left) || !ImGui::IsMouseDown(ImGuiMouseButton_Left)))
    {
        const bool Dragged = Session.DragCondition;
        Session.PressCondition = false;
        Session.DragCondition  = false;
        if (Dragged)
        {
            if (Session.DragPreviewCondition && State.Picker.Current.Kind != DraftingSnapKind::None) ++State.Picker.Snapped;
            State.Picker.Current = {};
            if (Session.DragPreviewCondition) ConcludeConstraint(State, DragDraftingSketchPoint(State.Document, Session.Sketch, Session.DragPoint, Session.DragTarget));
            Session.DragPreviewCondition = false;
            return;
        }
    }
    if (LeftClick) ClickConstraintTarget(State);
    if (RightClick) CloseConstraintSession(State);
}

// Pixel polyline of one sketch curve.
[[nodiscard]] std::vector<ImVec2> CurvePixels(const DraftingWorkspaceState& State, const DraftingCameraFrame& Frame, const SketchModel& Sketch, const SketchCurve& Curve, const OrthonormalFrame& Plane)
{
    std::vector<ImVec2> Pixels;
    for (const Vector2& Sample : SampleSketchCurve(Sketch, Curve)) { ImVec2 Pixel; if (ProjectWorldPoint(State, Frame, PlacePlanePoint(Plane, Sample), Pixel)) Pixels.push_back(Pixel); }
    return Pixels;
}

// Dashed polyline (6 px on, 4 px off) over a dark casing: earlier sketches read on the background and on faces alike.
void DrawDashedPolyline(ImDrawList* Canvas, const std::vector<ImVec2>& Pixels, ImU32 Colour, float Width)
{
    Canvas->AddPolyline(Pixels.data(), int(Pixels.size()), OtherSketchCasing, Width + 2.0f);
    constexpr float On = 6.0f, Period = 10.0f;
    float Phase = 0.0f;                                                                      // [px] - Arc length into the current period
    for (size_t Index = 0; Index + 1 < Pixels.size(); ++Index)
    {
        const ImVec2 From = Pixels[Index], To = Pixels[Index + 1];
        const float  Span = std::hypot(To.x - From.x, To.y - From.y);
        float Travel = 0.0f;
        while (Travel < Span)
        {
            const float Step = std::min(Span - Travel, Phase < On ? On - Phase : Period - Phase);
            if (Phase < On && Span > 0.0f)
                Canvas->AddLine(ImVec2(From.x + (To.x - From.x) * Travel / Span, From.y + (To.y - From.y) * Travel / Span),
                                ImVec2(From.x + (To.x - From.x) * (Travel + Step) / Span, From.y + (To.y - From.y) * (Travel + Step) / Span), Colour, Width);
            Travel += Step;
            Phase   = std::fmod(Phase + Step, Period);
        }
    }
}

// 📝 The session sketch on top of the ordinary overlay: curves green when fully fixed, picks blue, hover white, points
//    (green fixed · light blue free), the drag preview, then every constraint glyph / dimension coloured by status.
void DrawConstraintOverlay(DraftingWorkspaceState& State, ImDrawList* Canvas, const DraftingCameraFrame& Frame)
{
    const DraftingConstraintSession& Session = State.Constrain;
    if (!Session.ActiveCondition) return;
    const ConstraintView View = ViewConstraintSketch(State, Session.Sketch);
    if (!View.Sketch) return;
    const bool         Dragging = Session.DragCondition && Session.DragPreviewCondition;
    const SketchModel& Shown    = Dragging ? Session.DragPreview : *View.Sketch;
    auto PickedIn = [&](const std::string& Label, SketchPickKind Kind, int Index) { return std::any_of(Session.Picks.begin(), Session.Picks.end(), [&](const DraftingSketchPick& Pick) { return SamePick(Pick, { Label, { Kind, Index } }); }); };
    auto HoveredIn = [&](const std::string& Label, SketchPickKind Kind, int Index) { return Session.HoverCondition && SamePick(Session.Hover, { Label, { Kind, Index } }); };
    auto Picked  = [&](SketchPickKind Kind, int Index) { return PickedIn({}, Kind, Index); };
    auto Hovered = [&](SketchPickKind Kind, int Index) { return HoveredIn({}, Kind, Index); };
    // Earlier sketches first, dimmed: their points and curves can be picked to link them in.
    for (const OtherSketch& Other : OtherSketches(State))
    {
        for (int Curve = 0; Curve < int(Other.Sketch->Curves.size()); ++Curve)
        {
            const std::vector<ImVec2> Pixels = CurvePixels(State, Frame, *Other.Sketch, Other.Sketch->Curves[Curve], Other.Plane);
            if (Pixels.size() < 2) continue;
            const bool Chosen = PickedIn(Other.Label, SketchPickKind::Curve, Curve), Over = HoveredIn(Other.Label, SketchPickKind::Curve, Curve);
            if (Chosen || Over) Canvas->AddPolyline(Pixels.data(), int(Pixels.size()), Chosen ? SelectedColour : IM_COL32(0xff, 0xff, 0xff, 0xff), 3.0f);
            else DrawDashedPolyline(Canvas, Pixels, OtherSketchColour, 1.5f);
        }
        for (int Point = 0; Point < int(Other.Sketch->Points.size()); ++Point)
        {
            ImVec2 Pixel;
            if (!ProjectWorldPoint(State, Frame, PlacePlanePoint(Other.Plane, Other.Sketch->Points[Point].Position), Pixel)) continue;
            const bool Chosen = PickedIn(Other.Label, SketchPickKind::Point, Point);
            if (!Chosen) Canvas->AddCircleFilled(Pixel, 4.5f, OtherSketchCasing);
            Canvas->AddCircleFilled(Pixel, Chosen ? 5.0f : 3.0f, Chosen ? SelectedColour : OtherSketchColour);
            if (HoveredIn(Other.Label, SketchPickKind::Point, Point)) Canvas->AddCircle(Pixel, 8.0f, IM_COL32(0xff, 0xff, 0xff, 0xff), 0, 1.5f);
        }
    }
    for (int Curve = 0; Curve < int(Shown.Curves.size()); ++Curve)
    {
        const std::vector<ImVec2> Pixels = CurvePixels(State, Frame, Shown, Shown.Curves[Curve], View.Plane);
        if (Pixels.size() < 2) continue;
        const bool  Fixed  = !Dragging && View.Solve && Curve < int(View.Solve->CurveFixed.size()) && View.Solve->CurveFixed[Curve];
        const bool  Linked = !Shown.Curves[Curve].Link.Sketch.empty();
        const ImU32 Colour = Picked(SketchPickKind::Curve, Curve) ? SelectedColour : Hovered(SketchPickKind::Curve, Curve) ? IM_COL32(0xff, 0xff, 0xff, 0xff) : Linked ? LinkColour : Fixed ? ActiveColour : FreeColour;
        Canvas->AddPolyline(Pixels.data(), int(Pixels.size()), Colour, Picked(SketchPickKind::Curve, Curve) || Hovered(SketchPickKind::Curve, Curve) ? 3.5f : 2.5f);
    }
    for (int Point = 0; Point < int(Shown.Points.size()); ++Point)
    {
        ImVec2 Pixel;
        if (!ProjectWorldPoint(State, Frame, PlacePlanePoint(View.Plane, Shown.Points[Point].Position), Pixel)) continue;
        const bool  Fixed  = !Dragging && View.Solve && Point < int(View.Solve->PointFixed.size()) && View.Solve->PointFixed[Point];
        const bool  Chosen = Picked(SketchPickKind::Point, Point) || (Dragging && Point == Session.DragPoint);
        const bool  Linked = !Shown.Points[Point].Link.Sketch.empty();
        Canvas->AddCircleFilled(Pixel, Chosen ? 5.5f : 4.0f, Chosen ? SelectedColour : Linked ? LinkColour : Fixed ? ActiveColour : FreeColour);
        if (Hovered(SketchPickKind::Point, Point)) Canvas->AddCircle(Pixel, 8.0f, IM_COL32(0xff, 0xff, 0xff, 0xff), 0, 1.5f);
    }
    for (const ConstraintGlyph& Glyph : LayoutConstraintGlyphs(State, Frame, Shown, View.Plane))
    {
        const SketchConstraintStatus Status = ConstraintStatusOf(View, Glyph.Constraint);
        const bool  Reading  = Glyph.Constraint >= 0 && Glyph.Constraint < int(Shown.Constraints.size()) && Shown.Constraints[Glyph.Constraint].DrivenCondition;
        const bool  Troubled = !Dragging && Status != SketchConstraintStatus::Satisfied && Status != SketchConstraintStatus::Driven;
        const bool  Chosen   = Glyph.Constraint == Session.PickedConstraint;
        const bool  Over     = Glyph.Constraint == Session.HoverConstraint;
        const ImU32 Accent   = Chosen ? SelectedColour : Troubled ? RefusalColour : Over ? IM_COL32(0xff, 0xff, 0xff, 0xff) : Reading ? ReferenceColour : ConstraintColour;
        for (const auto& [From, To] : Glyph.Strokes) Canvas->AddLine(From, To, Accent, 1.2f);
        const ImVec2 Low(Glyph.Centre.x - Glyph.HalfExtent.x, Glyph.Centre.y - Glyph.HalfExtent.y), High(Glyph.Centre.x + Glyph.HalfExtent.x, Glyph.Centre.y + Glyph.HalfExtent.y);
        Canvas->AddRectFilled(Low, High, IM_COL32(0x16, 0x1a, 0x21, 0xe0), 5.0f);
        Canvas->AddRect(Low, High, Accent, 5.0f, Chosen || Over ? 2.0f : 1.0f);
        if (Glyph.Symbol == 1)
        {
            for (const float Shift : { -3.0f, 3.0f }) Canvas->AddLine(ImVec2(Glyph.Centre.x + Shift - 3.0f, Glyph.Centre.y + 5.0f), ImVec2(Glyph.Centre.x + Shift + 3.0f, Glyph.Centre.y - 5.0f), Accent, 1.6f);
        }
        else if (Glyph.Symbol == 2)
        {
            Canvas->AddLine(ImVec2(Glyph.Centre.x - 6.0f, Glyph.Centre.y + 5.0f), ImVec2(Glyph.Centre.x + 6.0f, Glyph.Centre.y + 5.0f), Accent, 1.6f);
            Canvas->AddLine(ImVec2(Glyph.Centre.x, Glyph.Centre.y + 5.0f), ImVec2(Glyph.Centre.x, Glyph.Centre.y - 6.0f), Accent, 1.6f);
        }
        else
        {
            const ImVec2 Size = ImGui::CalcTextSize(Glyph.Text.c_str());
            Canvas->AddText(ImVec2(Glyph.Centre.x - Size.x * 0.5f, Glyph.Centre.y - Size.y * 0.5f), Accent, Glyph.Text.c_str());
        }
    }
}

// Status line for the K session: sketch, DOF / conflict state, constraint count, picks or the pending verb.
[[nodiscard]] std::string ConstraintSessionReadout(const DraftingWorkspaceState& State, bool& TroubleCondition)
{
    const DraftingConstraintSession& Session = State.Constrain;
    const ConstraintView View = ViewConstraintSketch(State, Session.Sketch);
    TroubleCondition = false;
    if (!View.Sketch) return "Constrain";
    std::string Freedom = "DOF ?";
    if (View.Solve)
    {
        TroubleCondition = !View.Solve->ConflictGroups.empty() || !View.Solve->Converged;
        Freedom = TroubleCondition ? std::string("conflicting") : View.Solve->DegreesOfFreedom == 0 ? std::string("fully constrained") : Compose("DOF %d", View.Solve->DegreesOfFreedom);
    }
    std::string Tail;
    if (Session.DragCondition) Tail = Compose(" · drag %s to (%s, %s)", NameSketchPick(*View.Sketch, { SketchPickKind::Point, Session.DragPoint }).c_str(), FormatLength(Session.DragTarget.X).c_str(), FormatLength(Session.DragTarget.Y).c_str());
    else if (Session.PendingCondition) Tail = " · " + DescribeSketchConstraint(*View.Sketch, Session.Proposed) + " · typed " + (Session.Typed.empty() ? std::string("(measured)") : Session.Typed);
    else if (Session.PickedConstraint >= 0)
    {
        const SketchConstraint& Constraint = View.Sketch->Constraints[Session.PickedConstraint];
        Tail = " · " + (Constraint.Label.empty() ? std::string{} : Constraint.Label + " ") + DescribeSketchConstraint(*View.Sketch, Constraint) + (Session.Typed.empty() ? std::string{} : " · typed " + Session.Typed);
    }
    else if (!Session.Picks.empty()) Tail = " · " + ListConstraintPicks(State);
    return Compose("Constrain %s · %s · %d constraint(s)%s", CaptionOf(State.Document, Session.Sketch).c_str(), Freedom.c_str(), int(View.Sketch->Constraints.size()), Tail.c_str());
}

// 📝 Keyboard while K is open (after the view keys): Ctrl+Z / Ctrl+Shift+Z stay live.
template <typename PressedKey>
void RouteConstraintKeys(DraftingWorkspaceState& State, PressedKey&& Pressed, bool Control, bool Shift, bool Alternate)
{
    DraftingConstraintSession& Session = State.Constrain;
    ReconcileConstraintSession(State);
    if (!Session.ActiveCondition) return;
    if (Control)
    {
        if (Pressed(ImGuiKey_Z)) { if (Shift) Redo(State); else Undo(State); ReconcileConstraintSession(State); }
        return;
    }
    const bool Typing = Session.PendingCondition || Session.PickedConstraint >= 0;
    const bool Empty  = Session.Typed.empty();
    if (Typing) CollectTypedAmount(Session.Typed);
    if (Pressed(ImGuiKey_Enter) || Pressed(ImGuiKey_KeypadEnter))
    {
        if (Session.PendingCondition) CommitPendingConstraint(State);
        else if (Session.PickedConstraint >= 0 && !Session.Typed.empty()) EditPickedDimension(State);
        return;
    }
    if (Pressed(ImGuiKey_Escape))
    {
        if (Session.PendingCondition) { Session.PendingCondition = false; Session.Typed.clear(); Announce(State, "Constraint dropped"); }
        else if (!Session.Typed.empty()) Session.Typed.clear();
        else if (!Session.Picks.empty() || Session.PickedConstraint >= 0) { Session.Picks.clear(); Session.PickedConstraint = -1; Session.Refusal.clear(); Announce(State, "Picks cleared"); }
        else CloseConstraintSession(State);
        return;
    }
    if (Alternate) return;
    if (Pressed(ImGuiKey_K)) { CloseConstraintSession(State); return; }
    if (Session.PickedConstraint >= 0 && !Session.PendingCondition && (Pressed(ImGuiKey_Delete) || Pressed(ImGuiKey_X) || (Empty && Pressed(ImGuiKey_Backspace))))
    {
        RemovePickedConstraint(State);
        return;
    }
    if (Session.PickedConstraint < 0 && Session.Picks.size() == 1 && Session.Picks.front().Sketch.empty() && (Pressed(ImGuiKey_Delete) || Pressed(ImGuiKey_X)))
    {
        const ConstraintView View = ViewConstraintSketch(State, Session.Sketch);
        if (View.Sketch && DraftingLinkedCondition(*View.Sketch, Session.Picks.front().Pick)) { UnlinkPickedGeometry(State); return; }
    }
    if (Shift)
    {
        if (Pressed(ImGuiKey_D)) ApplyConstraintReference(State);
        return;
    }
    if (Session.PickedConstraint >= 0 && !Session.PendingCondition && Empty && Pressed(ImGuiKey_E)) { TogglePickedReference(State); return; }
    static const std::pair<ImGuiKey, SketchConstraintVerb> Verbs[] = {
        { ImGuiKey_D, SketchConstraintVerb::Dimension }, { ImGuiKey_N, SketchConstraintVerb::Angle }, { ImGuiKey_H, SketchConstraintVerb::Horizontal },
        { ImGuiKey_V, SketchConstraintVerb::Vertical }, { ImGuiKey_O, SketchConstraintVerb::On }, { ImGuiKey_Q, SketchConstraintVerb::Equal },
        { ImGuiKey_M, SketchConstraintVerb::Midpoint }, { ImGuiKey_Y, SketchConstraintVerb::Symmetric }, { ImGuiKey_L, SketchConstraintVerb::Parallel },
        { ImGuiKey_LeftBracket, SketchConstraintVerb::Perpendicular }, { ImGuiKey_RightBracket, SketchConstraintVerb::Lock }, { ImGuiKey_C, SketchConstraintVerb::Curvature } };
    for (const auto& [Key, Verb] : Verbs) if (Pressed(Key)) { ApplyConstraintVerb(State, Verb); return; }
}

// Inspector card: the sketch's constraints (click picks, status words), DOF, the verb buttons and a dimension field.
void DrawConstraintCard(DraftingWorkspaceState& State)
{
    const std::string Label = ConstraintSketchLabel(State);
    if (Label.empty()) return;
    const ConstraintView View = ViewConstraintSketch(State, Label);
    if (!View.Sketch) return;
    DraftingConstraintSession& Session = State.Constrain;
    const bool Live = Session.ActiveCondition && Session.Sketch == Label;
    BeginCard("Constraints");
    const int Freedom = View.Solve ? View.Solve->DegreesOfFreedom : -1;
    const bool Trouble = View.Solve && (!View.Solve->ConflictGroups.empty() || !View.Solve->Converged);
    ImGui::PushStyleColor(ImGuiCol_Text, Tint(Trouble ? RefusalColour : Freedom == 0 ? ActiveColour : FreeColour));
    ImGui::TextUnformatted(Trouble ? "Conflicting constraints" : Freedom == 0 ? "Fully constrained (DOF 0)" : Compose("Under-constrained · DOF %d", Freedom).c_str());
    ImGui::PopStyleColor();
    for (int Index = 0; Index < int(View.Sketch->Constraints.size()); ++Index)
    {
        const SketchConstraint&      Constraint = View.Sketch->Constraints[Index];
        const SketchConstraintStatus Status     = ConstraintStatusOf(View, Index);
        const bool Quiet = Status == SketchConstraintStatus::Satisfied || Status == SketchConstraintStatus::Driven;
        const std::string Row = Compose("%s  %s%s###ConstraintRow%d", Constraint.Label.empty() ? "-" : Constraint.Label.c_str(), DescribeSketchConstraint(*View.Sketch, Constraint).c_str(),
                                        Quiet ? "" : Compose(" · %s", SketchConstraintStatusLabel(Status)).c_str(), Index);
        if (!Quiet || Constraint.DrivenCondition) ImGui::PushStyleColor(ImGuiCol_Text, Tint(Quiet ? ReferenceColour : RefusalColour));
        if (ImGui::Selectable(Row.c_str(), Live && Session.PickedConstraint == Index))
        {
            if (!Live) OpenConstraintSession(State);
            State.Constrain.PickedConstraint = Index;
            State.Constrain.Picks.clear();
            State.Constrain.PendingCondition = false;
            State.Constrain.Typed.clear();
        }
        RegisterWidget(State, Compose("ConstraintRow%d", Index).c_str());
        if (!Quiet || Constraint.DrivenCondition) ImGui::PopStyleColor();
    }
    if (Live)
    {
        ImGui::Spacing();
        static const std::pair<const char*, SketchConstraintVerb> Buttons[] = {
            { "D###ConstrainD", SketchConstraintVerb::Dimension }, { "N###ConstrainN", SketchConstraintVerb::Angle }, { "H###ConstrainH", SketchConstraintVerb::Horizontal },
            { "V###ConstrainV", SketchConstraintVerb::Vertical }, { "O###ConstrainO", SketchConstraintVerb::On }, { "Q###ConstrainQ", SketchConstraintVerb::Equal },
            { "M###ConstrainM", SketchConstraintVerb::Midpoint }, { "Y###ConstrainY", SketchConstraintVerb::Symmetric }, { "L###ConstrainL", SketchConstraintVerb::Parallel },
            { "[###ConstrainPerpendicular", SketchConstraintVerb::Perpendicular }, { "]###ConstrainLock", SketchConstraintVerb::Lock }, { "C###ConstrainC", SketchConstraintVerb::Curvature } };
        for (size_t Index = 0; Index < std::size(Buttons); ++Index)
        {
            if (Index) ImGui::SameLine(0, 3);
            if (PressButton(State, Buttons[Index].first, ImVec2(23, 0))) ApplyConstraintVerb(State, Buttons[Index].second);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", SketchConstraintVerbLabel(Buttons[Index].second));
        }
        if (PressButton(State, "Reference (Shift+D)###ConstrainReference", ImVec2(150, 0))) ApplyConstraintReference(State);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("A display-only dimension: it measures, it never drives");
        if (Session.PickedConstraint >= 0 && Session.PickedConstraint < int(View.Sketch->Constraints.size()))
        {
            const SketchConstraint& Constraint = View.Sketch->Constraints[Session.PickedConstraint];
            if (SketchDimensionCondition(Constraint.Kind))
            {
                if (PressButton(State, Constraint.DrivenCondition ? "Make driving (E)###ToggleReference" : "Make reference (E)###ToggleReference", ImVec2(150, 0))) TogglePickedReference(State);
            }
            if (SketchDimensionCondition(Constraint.Kind) && !Constraint.DrivenCondition)
            {
                const bool Angular = Constraint.Kind == SketchConstraintKind::Angle;
                double Value = MeasureSketchConstraint(*View.Sketch, Constraint) * (Angular ? 180.0 / std::numbers::pi : 1.0);
                ImGui::SetNextItemWidth(150);
                if (ImGui::InputDouble(Angular ? "deg###DimensionValue" : "m###DimensionValue", &Value, 0.0, 0.0, "%.6g", ImGuiInputTextFlags_EnterReturnsTrue))
                    ConcludeConstraint(State, ChangeDraftingDimension(State.Document, Session.Sketch, Session.PickedConstraint, Angular ? Value * std::numbers::pi / 180.0 : Value));
                RegisterWidget(State, "DimensionValue");
            }
            if (PressButton(State, "Remove###RemoveConstraint", ImVec2(118, 0))) RemovePickedConstraint(State);
        }
        if (!Session.Refusal.empty())
        {
            ImGui::PushStyleColor(ImGuiCol_Text, Tint(RefusalColour));
            ImGui::TextWrapped("%s", Session.Refusal.c_str());
            ImGui::PopStyleColor();
        }
    }
    else if (PressButton(State, "Constrain (K)###OpenConstrain", ImVec2(150, 0))) OpenConstraintSession(State);
    EndCard();
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    EDITING SESSIONS (G / R / S · SHIFT+D · O / P · J · H · CTRL+C / V)
//------------------------------------------------------------------------------------------------------------------------

std::string CaptionOf(const DraftingDocument& Document, const std::string& Label);

constexpr double MoveSnap   = 0.1;                                                           // [len] - G step
constexpr double TurnSnap   = 5.0;                                                           // [°]   - R step
constexpr double FactorSnap = 0.1;                                                           // [-]   - S step
constexpr double SweepSnap  = 0.01;                                                          // [len] - O / P step

[[nodiscard]] const char* TransformVerbLabel(DraftingTransformVerb Verb) noexcept
{
    return Verb == DraftingTransformVerb::Move ? "Move" : Verb == DraftingTransformVerb::Rotate ? "Rotate" : "Scale";
}

// The bodies a session acts on: body picks, else the owners of picked faces / edges.
[[nodiscard]] std::vector<std::string> PickedBodies(const DraftingDocument& Document)
{
    std::vector<std::string> Keys = Document.Selection.Bodies;
    auto Add = [&](const std::string& Key) { if (!Key.empty() && std::find(Keys.begin(), Keys.end(), Key) == Keys.end()) Keys.push_back(Key); };
    for (const std::string& Face : Document.Selection.Faces) Add(DraftingBodyOfName(Document, DraftingSelectionKind::Face, Face));
    for (const std::string& Edge : Document.Selection.Edges) Add(DraftingBodyOfName(Document, DraftingSelectionKind::Edge, Edge));
    return Keys;
}

// Bounds centre of the bodies in the shown scene (their tessellation corners).
[[nodiscard]] Vector3 MeasureBodiesCentre(const DraftingScene& Scene, const std::vector<std::string>& Keys)
{
    Vector3 Low{ 1.0e300, 1.0e300, 1.0e300 }, High{ -1.0e300, -1.0e300, -1.0e300 };
    for (const DraftingSceneBody& Body : Scene.Bodies)
    {
        if (std::find(Keys.begin(), Keys.end(), Body.Key) == Keys.end()) continue;
        for (uint32_t Face = Body.FirstFace; Face < Body.FirstFace + Body.FaceCount; ++Face)
            for (uint32_t Corner = Scene.Faces[Face].FirstCorner; Corner < Scene.Faces[Face].FirstCorner + Scene.Faces[Face].CornerCount; ++Corner)
            {
                const float* Position = Scene.Corners[Corner].Position;
                Low  = { std::min(Low.X, double(Position[0])), std::min(Low.Y, double(Position[1])), std::min(Low.Z, double(Position[2])) };
                High = { std::max(High.X, double(Position[0])), std::max(High.Y, double(Position[1])), std::max(High.Z, double(Position[2])) };
            }
    }
    return Low.X > High.X ? Vector3{} : (Low + High) * 0.5;
}

// Typed digits / point / minus from this frame's characters; false when nothing numeric was typed.
bool CollectTypedAmount(std::string& Typed)
{
    bool ChangedCondition = false;
    for (const ImWchar Character : ImGui::GetIO().InputQueueCharacters)
    {
        if ((Character >= '0' && Character <= '9') || Character == '.' || (Character == '-' && Typed.empty())) { Typed.push_back(char(Character)); ChangedCondition = true; }
    }
    if (ImGui::IsKeyPressed(ImGuiKey_Backspace, false) && !Typed.empty()) { Typed.pop_back(); ChangedCondition = true; }
    return ChangedCondition;
}

[[nodiscard]] bool ParseTypedAmount(const std::string& Typed, double& Value)
{
    if (Typed.empty() || Typed == "-" || Typed == ".") return false;
    char* End = nullptr;
    Value = std::strtod(Typed.c_str(), &End);
    return End && *End == '\0' && std::isfinite(Value);
}

void OpenTransformSession(DraftingWorkspaceState& State, DraftingTransformVerb Verb, bool DuplicateCondition)
{
    const std::vector<std::string> Keys = PickedBodies(State.Document);
    if (Keys.empty()) { Announce(State, std::string(DuplicateCondition ? "Duplicate" : TransformVerbLabel(Verb)) + " needs picked bodies (mode 4, or a face / edge of the body)"); return; }
    DraftingTransformSession& Transform = State.Transform;
    const int Commits = Transform.Commits;
    Transform                    = {};
    Transform.Commits            = Commits;
    Transform.ActiveCondition    = true;
    Transform.Verb               = Verb;
    Transform.DuplicateCondition = DuplicateCondition;
    Transform.Keys               = Keys;
    Transform.Pivot              = MeasureBodiesCentre(State.Scene, Keys);
    State.Drawing.ActiveCondition = false;
    State.Trim.ActiveCondition    = false;
    State.Constrain.ActiveCondition = false;
    Announce(State, Compose("%s%s %d body(ies) · X / Y / Z axis%s · type a value · click / Enter commits · Esc cancels", DuplicateCondition ? "Duplicate + " : "",
                            TransformVerbLabel(Verb), int(Keys.size()), Verb == DraftingTransformVerb::Move ? " · Shift+X / Y / Z plane" : ""));
}

void CancelTransformSession(DraftingWorkspaceState& State)
{
    State.Transform.ActiveCondition = false;
    State.Transform.Preview.reset();
    Announce(State, std::string(TransformVerbLabel(State.Transform.Verb)) + " cancelled");
}

[[nodiscard]] bool IdentityMapCondition(const AffineMap& Map)
{
    return Length(Map.ColumnX - Vector3{ 1.0, 0.0, 0.0 }) + Length(Map.ColumnY - Vector3{ 0.0, 1.0, 0.0 }) + Length(Map.ColumnZ - Vector3{ 0.0, 0.0, 1.0 }) + Length(Map.Shift) < 1.0e-12;
}

void ApplyTransformSession(DraftingWorkspaceState& State)
{
    DraftingTransformSession& Transform = State.Transform;
    if (!Transform.Refusal.empty()) { Announce(State, "Refused: " + Transform.Refusal); return; }
    if (!Transform.DuplicateCondition && IdentityMapCondition(Transform.Map)) { Transform.ActiveCondition = false; Transform.Preview.reset(); Announce(State, std::string(TransformVerbLabel(Transform.Verb)) + ": nothing changed"); return; }
    const DraftingCommandOutcome Outcome = Transform.DuplicateCondition ? DuplicateDraftingBodies(State.Document, Transform.Keys, Transform.Map)
                                                                        : TransformDraftingBodies(State.Document, Transform.Keys, Transform.Map, TransformVerbLabel(Transform.Verb));
    if (Outcome.AcceptedCondition)
    {
        ++Transform.Commits;
        Transform.ActiveCondition = false;
        Transform.Preview.reset();
        State.PickMode = DraftingPickMode::Solid;
    }
    Conclude(State, Outcome);
}

// Pointer → affine map (see DraftingTransformSession), then the staged preview when the map changed.
void DriveTransformSession(DraftingWorkspaceState& State, const DraftingCameraFrame& Frame, ImVec2 Local)
{
    DraftingTransformSession& Transform = State.Transform;
    ImGuiIO& Io = ImGui::GetIO();
    if (!Transform.StartCondition)
    {
        Transform.Start = Io.MousePos; Transform.StartLocal = Local; Transform.StartCondition = true;
        if (Transform.Verb == DraftingTransformVerb::Move)
        {
            // Plasticity move: the grab point snaps (a vertex of the body), then follows the pointer's snaps elsewhere.
            const DraftingSnap Anchor = FindPointerSnap(State, Frame, Local, std::nullopt, {}, {}, true);
            Transform.AnchorCondition = Anchor.Kind != DraftingSnapKind::None;
            Transform.Anchor          = Anchor.Position;
            Transform.AnchorSource    = Transform.AnchorCondition ? std::string(DraftingSnapLabel(Anchor.Kind)) + " " + Anchor.Source : std::string{};
        }
    }
    const Vector3 Forward{ Frame.Forward[0], Frame.Forward[1], Frame.Forward[2] };
    const Vector3 Axes[3] = { { 1.0, 0.0, 0.0 }, { 0.0, 1.0, 0.0 }, { 0.0, 0.0, 1.0 } };
    static const char* AxisNames[] = { "x", "y", "z" };
    const bool   SnapCondition = !Io.KeyCtrl;
    double       Typed         = 0.0;
    const bool   TypedCondition = ParseTypedAmount(Transform.Typed, Typed);
    auto Snap = [&](double Value, double Step) { return SnapCondition ? std::round(Value / Step) * Step : Value; };
    ImVec2 PivotPixel;
    const bool PivotCondition = ProjectWorldPoint(State, Frame, Transform.Pivot, PivotPixel);
    switch (Transform.Verb)
    {
    case DraftingTransformVerb::Move:
    {
        Vector3 Shift;
        if (TypedCondition)
        {
            Shift = Axes[std::max(0, Transform.Axis)] * Typed;                                 // Typed: along the axis (x when free)
            State.Picker.Current = {};
        }
        else
        {
            // The pointer's two rays meet a plane through the pivot: facing the eye (free), containing the axis, or normal to it.
            Vector3 Normal = Forward;
            if (Transform.Axis >= 0)
            {
                const Vector3 Axis = Axes[Transform.Axis];
                Normal = Transform.PlaneCondition ? Axis : Forward - Axis * Dot(Forward, Axis);
                if (Length(Normal) < 1.0e-6) Normal = Vector3{ Frame.Up[0], Frame.Up[1], Frame.Up[2] };
                Normal = Normal * (1.0 / Length(Normal));
            }
            auto Meet = [&](ImVec2 At, Vector3& Hit)
            {
                const DraftingRay Ray = CastDraftingRay(Frame, At.x, At.y);
                const Vector3 Origin{ Ray.Origin[0], Ray.Origin[1], Ray.Origin[2] }, Direction{ Ray.Direction[0], Ray.Direction[1], Ray.Direction[2] };
                const double Facing = Dot(Direction, Normal);
                if (std::abs(Facing) < 1.0e-9) return false;
                Hit = Origin + Direction * (Dot(Transform.Pivot - Origin, Normal) / Facing);
                return true;
            };
            Vector3 From, To;
            const DraftingSnap Target = Transform.AnchorCondition ? FindPointerSnap(State, Frame, Local, std::nullopt, {}, Transform.Keys, true) : DraftingSnap{};
            State.Picker.Current = Target;
            if (Target.Kind != DraftingSnapKind::None)
            {
                // Anchor onto the target snap, within the axis / plane constraint
                Shift = Target.Position - Transform.Anchor;
                if (Transform.Axis >= 0 && !Transform.PlaneCondition) Shift = Axes[Transform.Axis] * Dot(Shift, Axes[Transform.Axis]);
                else if (Transform.Axis >= 0) Shift = Shift - Axes[Transform.Axis] * Dot(Shift, Axes[Transform.Axis]);
            }
            else
            {
                if (Meet(Transform.StartLocal, From) && Meet(Local, To))
                {
                    Shift = To - From;
                    if (Transform.Axis >= 0 && !Transform.PlaneCondition) Shift = Axes[Transform.Axis] * Dot(Shift, Axes[Transform.Axis]);
                }
                Shift = { Snap(Shift.X, MoveSnap), Snap(Shift.Y, MoveSnap), Snap(Shift.Z, MoveSnap) };
            }
        }
        Transform.Map     = TranslationMap(Shift);
        Transform.Readout = Compose("Move%s%s (%s, %s, %s)", Transform.Axis < 0 ? " screen" : Transform.PlaneCondition ? " plane ⊥" : " ", Transform.Axis < 0 ? "" : AxisNames[Transform.Axis],
                                    FormatLength(Shift.X).c_str(), FormatLength(Shift.Y).c_str(), FormatLength(Shift.Z).c_str());
        if (!TypedCondition && State.Picker.Current.Kind != DraftingSnapKind::None)
            Transform.Readout += std::string(" · ") + Transform.AnchorSource + " → " + DraftingSnapLabel(State.Picker.Current.Kind) + " " + State.Picker.Current.Source;
        break;
    }
    case DraftingTransformVerb::Rotate:
    {
        // Screen sweep about the pivot (counter-clockwise on screen = about the axis towards the eye).
        Vector3 Axis = Transform.Axis < 0 ? Forward * -1.0 : Axes[Transform.Axis];
        double Degrees = Typed;
        if (!TypedCondition)
        {
            const double FirstX = Transform.Start.x - PivotPixel.x, FirstY = Transform.Start.y - PivotPixel.y;
            const double NowX   = Io.MousePos.x - PivotPixel.x, NowY = Io.MousePos.y - PivotPixel.y;
            Degrees = PivotCondition && std::hypot(FirstX, FirstY) > 4.0 && std::hypot(NowX, NowY) > 4.0
                    ? std::atan2(-(FirstX * NowY - FirstY * NowX), FirstX * NowX + FirstY * NowY) * 180.0 / std::numbers::pi : 0.0;
            if (Dot(Axis, Forward) > 0.0) Degrees = -Degrees;                                 // Axis points away from the eye
            Degrees = Snap(Degrees, TurnSnap);
        }
        Transform.Map     = RotationMap(Transform.Pivot, Axis, Degrees * std::numbers::pi / 180.0);
        Transform.Readout = Compose("Rotate %s %s°", Transform.Axis < 0 ? "screen" : AxisNames[Transform.Axis], FormatLength(Degrees).c_str());
        break;
    }
    case DraftingTransformVerb::Scale:
    {
        double Factor = Typed;
        if (!TypedCondition)
        {
            const double First = std::hypot(Transform.Start.x - PivotPixel.x, Transform.Start.y - PivotPixel.y);
            const double Now   = std::hypot(Io.MousePos.x - PivotPixel.x, Io.MousePos.y - PivotPixel.y);
            Factor = PivotCondition && First > 4.0 ? Snap(Now / First, FactorSnap) : 1.0;
        }
        Vector3 Factors{ Factor, Factor, Factor };
        if (Transform.Axis >= 0) Factors = { Transform.Axis == 0 ? Factor : 1.0, Transform.Axis == 1 ? Factor : 1.0, Transform.Axis == 2 ? Factor : 1.0 };
        Transform.Map     = ScalingMap(Transform.Pivot, Factors);
        Transform.Readout = Compose("Scale %s ×%s", Transform.Axis < 0 ? "xyz" : AxisNames[Transform.Axis], FormatLength(Factor).c_str());
        break;
    }
    }
    if (!Transform.Typed.empty()) Transform.Readout += " · typed " + Transform.Typed;
    // Staged preview (one regeneration per distinct map).
    const AffineMap& Map = Transform.Map;
    const bool Same = Transform.StagedCondition && Length(Map.ColumnX - Transform.StagedMap.ColumnX) + Length(Map.ColumnY - Transform.StagedMap.ColumnY) + Length(Map.ColumnZ - Transform.StagedMap.ColumnZ)
                                                   + Length(Map.Shift - Transform.StagedMap.Shift) == 0.0;
    if (Same) return;
    Transform.StagedMap       = Map;
    Transform.StagedCondition = true;
    Transform.Refusal.clear();
    if (!Transform.DuplicateCondition && IdentityMapCondition(Map)) { Transform.Preview.reset(); return; }
    Transform.Preview = std::make_unique<DraftingDocument>(CloneDraftingDocument(State.Document));
    const DraftingCommandOutcome Outcome = Transform.DuplicateCondition ? DuplicateDraftingBodies(*Transform.Preview, Transform.Keys, Map)
                                                                        : TransformDraftingBodies(*Transform.Preview, Transform.Keys, Map, TransformVerbLabel(Transform.Verb));
    if (!Outcome.AcceptedCondition) { Transform.Refusal = Legible(Outcome.Refusal); Transform.Preview.reset(); return; }
    Transform.PreviewScene = BuildDraftingScene(*Transform.Preview);
}

[[nodiscard]] const char* SweepVerbName(DraftingSweepVerb Verb)
{
    switch (Verb)
    {
    case DraftingSweepVerb::Offset:    return "Offset";
    case DraftingSweepVerb::Pipe:      return "Pipe";
    case DraftingSweepVerb::Revolve:   return "Revolve";
    case DraftingSweepVerb::Evolution: return "Evolution";
    }
    return "Sweep";
}

[[nodiscard]] const FeatureStep* SweepStep(const DraftingWorkspaceState& State, const std::string& Label)
{
    for (const FeatureStep& Step : State.Document.Tree.Steps) if (Step.Label == Label) return &Step;
    return nullptr;
}

[[nodiscard]] const SketchModel* SweepSketch(const DraftingWorkspaceState& State, const std::string& Label)
{
    const auto Solved = State.Document.Regeneration.Sketches.find(Label);
    return Solved == State.Document.Regeneration.Sketches.end() ? nullptr : &Solved->second;
}

// A sketch holding exactly one drawn open curve: a path, never a profile.
[[nodiscard]] bool SingleOpenCurveCondition(const DraftingWorkspaceState& State, const std::string& Label)
{
    const SketchModel* Sketch = SweepSketch(State, Label);
    if (!Sketch) return false;
    int  Drawn = 0;
    bool Open  = true;
    for (const SketchCurve& Curve : Sketch->Curves)
    {
        if (Curve.Construction) continue;
        ++Drawn;
        Open = Open && Curve.Kind != SketchCurveKind::Circle && !(Curve.Kind == SketchCurveKind::Spline && Curve.Points.size() > 1 && Curve.Points.front() == Curve.Points.back());
    }
    return Drawn == 1 && Open;
}

// The path O / P act on: the focused sketch step, else (P) the first selected curve, else the newest sketch step.
// Revolve / Evolution take the profile: the focused sketch, else the newest sketch that is not a lone open curve (a
// focused lone open curve is Evolution's path instead).
[[nodiscard]] std::string SweepPath(const DraftingWorkspaceState& State, DraftingSweepVerb Verb)
{
    const bool ProfileCondition = Verb == DraftingSweepVerb::Revolve || Verb == DraftingSweepVerb::Evolution;
    if (FocusedSketchCondition(State) && !(ProfileCondition && SingleOpenCurveCondition(State, State.FocusedStep))) return State.FocusedStep;
    if (Verb == DraftingSweepVerb::Pipe && !State.Document.Selection.Curves.empty()) return State.Document.Selection.Curves.front();
    for (auto Step = State.Document.Tree.Steps.rbegin(); Step != State.Document.Tree.Steps.rend(); ++Step)
        if (Step->Kind == FeatureStepKind::Sketch && !(ProfileCondition && SingleOpenCurveCondition(State, Step->Label))) return Step->Label;
    return {};
}

void OpenSweepSession(DraftingWorkspaceState& State, DraftingSweepVerb Verb)
{
    const std::string Path = SweepPath(State, Verb);
    if (Path.empty())
    {
        Announce(State, Verb == DraftingSweepVerb::Offset ? "Offset (O) needs a sketch: draw one first"
                        : Verb == DraftingSweepVerb::Pipe ? "Pipe (P) needs a path: draw a line / arc / curve or select a curve"
                        : Verb == DraftingSweepVerb::Revolve ? "Revolve (Alt+R) needs a profile: draw a closed sketch first"
                                                             : "Evolution (Shift+P) needs a profile: draw a closed sketch first");
        return;
    }
    DraftingSweepSession& Sweep = State.Sweep;
    const int Commits = Sweep.Commits;
    Sweep                 = {};
    Sweep.Commits         = Commits;
    Sweep.ActiveCondition = true;
    Sweep.Verb            = Verb;
    Sweep.Path            = Path;
    Sweep.Amount          = Verb == DraftingSweepVerb::Offset ? 0.1 : Verb == DraftingSweepVerb::Revolve ? 360.0 : 0.05;
    Sweep.Drift           = Sweep.Amount;
    Sweep.Combination     = State.Document.Regeneration.Bodies.empty() ? FeatureCombination::New : FeatureCombination::Separate;
    State.Drawing.ActiveCondition = false;
    State.Trim.ActiveCondition    = false;
    State.Constrain.ActiveCondition = false;
    const char* Booleans = " · Q union · W difference · E intersect · R new body";
    if (Verb == DraftingSweepVerb::Revolve)
    {
        Sweep.Axis = ProposeDraftingRevolveAxis(State.Document, Path);
        Announce(State, Compose("Revolve %s about %s · type degrees (360) · X / Y sketch axis · click one of its lines for the axis%s · click / Enter commits · Esc cancels",
                                CaptionOf(State.Document, Path).c_str(), DescribeDraftingRevolveAxis(State.Document, Path, Sweep.Axis).c_str(), Booleans));
        return;
    }
    if (Verb == DraftingSweepVerb::Evolution)
    {
        Sweep.Profile = Path;
        Sweep.Path    = FocusedSketchCondition(State) && SingleOpenCurveCondition(State, State.FocusedStep) ? State.FocusedStep : ProposeDraftingEvolutionPath(State.Document, Path);
        Announce(State, Compose("Evolution %s along %s · click another sketch's curve for the path%s · click / Enter commits · Esc cancels", CaptionOf(State.Document, Path).c_str(),
                                Sweep.Path.empty() ? "? (no path yet)" : CaptionOf(State.Document, Sweep.Path).c_str(), Booleans));
        return;
    }
    Announce(State, Compose("%s %s · move sideways (Shift fine, Ctrl free) or type a value%s · click / Enter commits · Esc cancels", SweepVerbName(Verb),
                            CaptionOf(State.Document, Path).c_str(), Verb == DraftingSweepVerb::Pipe ? Booleans : ""));
}

void CancelSweepSession(DraftingWorkspaceState& State)
{
    State.Sweep.ActiveCondition = false;
    State.Sweep.Preview.reset();
    Announce(State, std::string(SweepVerbName(State.Sweep.Verb)) + " cancelled");
}

void ChooseSweepCombination(DraftingWorkspaceState& State, FeatureCombination Combination)
{
    if (State.Sweep.Verb == DraftingSweepVerb::Offset) return;
    State.Sweep.Combination = State.Document.Regeneration.Bodies.empty() ? FeatureCombination::New : Combination;
    Announce(State, std::string(SweepVerbName(State.Sweep.Verb)) + ": " + (State.Sweep.Combination == FeatureCombination::New ? "New" : FeatureCombinationLabel(State.Sweep.Combination)));
}

// Revolve: X / Y turn about the sketch x- / y-axis.
void ChooseRevolveAxis(DraftingWorkspaceState& State, Vector2 Direction)
{
    if (State.Sweep.Verb != DraftingSweepVerb::Revolve) return;
    State.Sweep.Axis = { -1, { 0.0, 0.0 }, Direction };
    Announce(State, "Revolve about the " + DescribeDraftingRevolveAxis(State.Document, State.Sweep.Path, State.Sweep.Axis));
}

[[nodiscard]] DraftingCommandOutcome RunSweepVerb(const DraftingWorkspaceState& State, DraftingDocument& Document)
{
    const DraftingSweepSession& Sweep = State.Sweep;
    switch (Sweep.Verb)
    {
    case DraftingSweepVerb::Offset:    return OffsetDraftingSketch(Document, Sweep.Path, Sweep.Amount);
    case DraftingSweepVerb::Pipe:      return PipeDraftingPath(Document, Sweep.Path, Sweep.Amount, Sweep.Combination, BooleanTarget(State));
    case DraftingSweepVerb::Revolve:   return RevolveDraftingSketch(Document, Sweep.Path, Sweep.Axis, Sweep.Amount * std::numbers::pi / 180.0, Sweep.Combination, BooleanTarget(State));
    case DraftingSweepVerb::Evolution: return EvolveDraftingProfile(Document, Sweep.Profile, Sweep.Path, Sweep.Combination, BooleanTarget(State));
    }
    return {};
}

void ApplySweepSession(DraftingWorkspaceState& State)
{
    DraftingSweepSession& Sweep = State.Sweep;
    if (!Sweep.Refusal.empty()) { Announce(State, "Refused: " + Sweep.Refusal); return; }
    const DraftingCommandOutcome Outcome = RunSweepVerb(State, State.Document);
    if (Outcome.AcceptedCondition)
    {
        ++Sweep.Commits;
        Sweep.ActiveCondition = false;
        Sweep.Preview.reset();
        if (Sweep.Verb == DraftingSweepVerb::Offset) State.FocusedStep = State.Document.Tree.Steps.back().Label;   // O again offsets the offset
        else
        {
            State.FocusedStep = {};
            State.PickMode    = DraftingPickMode::Solid;
            if (State.Document.Tree.Steps.back().Combination == FeatureCombination::Separate || State.Document.Tree.Steps.back().Combination == FeatureCombination::New)
                SelectDraftingName(State.Document, DraftingSelectionKind::Body, State.Document.Tree.Steps.back().Label, false);
        }
    }
    Conclude(State, Outcome);
}

// Revolve / Evolution left-click: a line of the profile under the pointer becomes the axis, another sketch's curve the
// path; a click anywhere else commits.
void ClickSweepSession(DraftingWorkspaceState& State)
{
    DraftingSweepSession& Sweep = State.Sweep;
    const SketchModel* Sketch = Sweep.HoverCurve >= 0 ? SweepSketch(State, Sweep.HoverSketch) : nullptr;
    if (Sketch && Sweep.HoverCurve < int(Sketch->Curves.size()))
    {
        const SketchCurve& Curve = Sketch->Curves[Sweep.HoverCurve];
        if (Sweep.Verb == DraftingSweepVerb::Revolve)
        {
            Sweep.Axis = { Curve.Identifier, {}, { 0.0, 1.0 } };
            Announce(State, "Revolve about " + SketchCurveName(Curve));
            return;
        }
        if (Sweep.Verb == DraftingSweepVerb::Evolution)
        {
            Sweep.Path = Sweep.HoverSketch;
            Announce(State, "Evolution along " + CaptionOf(State.Document, Sweep.Path));
            return;
        }
    }
    ApplySweepSession(State);
}

// The curve under the pointer (≤ 8 px) that a click would take: Revolve the profile's lines, Evolution the drawn curves
// of the other sketches.
void HoverSweepCurve(DraftingWorkspaceState& State, const DraftingCameraFrame& Frame, bool HoveredCondition)
{
    DraftingSweepSession& Sweep = State.Sweep;
    Sweep.HoverSketch.clear();
    Sweep.HoverCurve = -1;
    if (!HoveredCondition) return;
    const ImVec2 Mouse   = ImGui::GetIO().MousePos;
    double       Nearest = 8.0;                                                              // [px] - Pick radius
    for (const FeatureStep& Step : State.Document.Tree.Steps)
    {
        if (Step.Kind != FeatureStepKind::Sketch) continue;
        const bool Candidate = Sweep.Verb == DraftingSweepVerb::Revolve ? Step.Label == Sweep.Path : Step.Label != Sweep.Profile;
        const SketchModel* Sketch = Candidate ? SweepSketch(State, Step.Label) : nullptr;
        if (!Sketch) continue;
        const OrthonormalFrame Plane = ElevatedSketchPlane(State.Document, Step);
        for (int Curve = 0; Curve < int(Sketch->Curves.size()); ++Curve)
        {
            const SketchCurve& Each = Sketch->Curves[Curve];
            if (Sweep.Verb == DraftingSweepVerb::Revolve ? Each.Kind != SketchCurveKind::Line : Each.Construction) continue;
            std::vector<ImVec2> Pixels;
            for (const Vector2& Sample : SampleSketchCurve(*Sketch, Each))
            {
                ImVec2 Pixel;
                if (ProjectWorldPoint(State, Frame, PlacePlanePoint(Plane, Sample), Pixel)) Pixels.push_back(Pixel);
            }
            for (size_t Index = 0; Index + 1 < Pixels.size(); ++Index)
            {
                const double AlongX = Pixels[Index + 1].x - Pixels[Index].x, AlongY = Pixels[Index + 1].y - Pixels[Index].y;
                const double Square = AlongX * AlongX + AlongY * AlongY;
                const double Share  = Square > 0.0 ? std::clamp(((Mouse.x - Pixels[Index].x) * AlongX + (Mouse.y - Pixels[Index].y) * AlongY) / Square, 0.0, 1.0) : 0.0;
                const double Gap    = std::hypot(Pixels[Index].x + AlongX * Share - Mouse.x, Pixels[Index].y + AlongY * Share - Mouse.y);
                if (Gap <= Nearest) { Nearest = Gap; Sweep.HoverSketch = Step.Label; Sweep.HoverCurve = Curve; }
            }
        }
    }
}

void DriveSweepSession(DraftingWorkspaceState& State, const DraftingCameraFrame& Frame, bool HoveredCondition)
{
    DraftingSweepSession& Sweep = State.Sweep;
    ImGuiIO& Io = ImGui::GetIO();
    double Typed = 0.0;
    if (Sweep.Verb == DraftingSweepVerb::Revolve || Sweep.Verb == DraftingSweepVerb::Evolution)
    {
        HoverSweepCurve(State, Frame, HoveredCondition);                                    // As trim: the click frame keeps its hover
        if (Sweep.Verb == DraftingSweepVerb::Revolve) Sweep.Amount = ParseTypedAmount(Sweep.Typed, Typed) ? Typed : 360.0;
        const std::string Key = Compose("%.17g|%d|%d|%.17g|%.17g|%.17g|%.17g|%s|%s|%s", Sweep.Amount, int(Sweep.Combination), Sweep.Axis.Line, Sweep.Axis.Origin.X, Sweep.Axis.Origin.Y,
                                        Sweep.Axis.Direction.X, Sweep.Axis.Direction.Y, Sweep.Path.c_str(), Sweep.Profile.c_str(), BooleanTarget(State).c_str());
        if (Key == Sweep.StagedKey) return;
        Sweep.StagedKey = Key;
        Sweep.Refusal.clear();
        Sweep.Preview = std::make_unique<DraftingDocument>(CloneDraftingDocument(State.Document));
        const DraftingCommandOutcome Outcome = RunSweepVerb(State, *Sweep.Preview);
        if (!Outcome.AcceptedCondition) { Sweep.Refusal = Legible(Outcome.Refusal); Sweep.Preview.reset(); return; }
        Sweep.PreviewScene = BuildDraftingScene(*Sweep.Preview);
        return;
    }
    if (ParseTypedAmount(Sweep.Typed, Typed)) Sweep.Amount = Typed;
    else
    {
        if (HoveredCondition && !ImGui::IsAnyMouseDown() && Io.MouseDelta.x != 0.0f) Sweep.Drift += Io.MouseDelta.x * Frame.WorldPerPixel * (Io.KeyShift ? 0.1 : 1.0) * 0.5;
        Sweep.Amount = Io.KeyCtrl ? Sweep.Drift : std::round(Sweep.Drift / SweepSnap) * SweepSnap;
    }
    if (Sweep.Verb == DraftingSweepVerb::Pipe) Sweep.Amount = std::max(Sweep.Amount, 0.001);
    if (Sweep.Amount == Sweep.StagedAmount && Sweep.Combination == Sweep.StagedCombination) return;
    Sweep.StagedAmount      = Sweep.Amount;
    Sweep.StagedCombination = Sweep.Combination;
    Sweep.Refusal.clear();
    Sweep.Outline.clear();
    if (Sweep.Verb == DraftingSweepVerb::Offset)
    {
        const SketchOffsetOutcome Outcome = PreviewDraftingOffset(State.Document, Sweep.Path, Sweep.Amount);
        if (!Outcome.Refusal.empty()) { Sweep.Refusal = Legible(Outcome.Refusal); return; }
        const FeatureStep* Step = SweepStep(State, Sweep.Path);
        if (!Step) return;
        const OrthonormalFrame Plane = ElevatedSketchPlane(State.Document, *Step);
        for (const SketchCurve& Curve : Outcome.Sketch.Curves)
        {
            std::vector<Vector3> Line;
            for (const Vector2& Sample : SampleSketchCurve(Outcome.Sketch, Curve)) Line.push_back(PlacePlanePoint(Plane, Sample));
            Sweep.Outline.push_back(std::move(Line));
        }
        return;
    }
    Sweep.Preview = std::make_unique<DraftingDocument>(CloneDraftingDocument(State.Document));
    const DraftingCommandOutcome Outcome = RunSweepVerb(State, *Sweep.Preview);
    if (!Outcome.AcceptedCondition) { Sweep.Refusal = Legible(Outcome.Refusal); Sweep.Preview.reset(); return; }
    Sweep.PreviewScene = BuildDraftingScene(*Sweep.Preview);
}

// One sketch curve in screen pixels (empty when the step or curve is gone).
[[nodiscard]] std::vector<ImVec2> SweepCurvePixels(DraftingWorkspaceState& State, const DraftingCameraFrame& Frame, const std::string& Label, int Curve)
{
    std::vector<ImVec2> Pixels;
    const FeatureStep* Step   = SweepStep(State, Label);
    const SketchModel* Sketch = SweepSketch(State, Label);
    if (!Step || !Sketch) return Pixels;
    const OrthonormalFrame Plane = ElevatedSketchPlane(State.Document, *Step);
    for (int Index = 0; Index < int(Sketch->Curves.size()); ++Index)
    {
        if (Curve >= 0 ? Index != Curve : Sketch->Curves[Index].Construction) continue;
        for (const Vector2& Sample : SampleSketchCurve(*Sketch, Sketch->Curves[Index])) { ImVec2 Pixel; if (ProjectWorldPoint(State, Frame, PlacePlanePoint(Plane, Sample), Pixel)) Pixels.push_back(Pixel); }
    }
    return Pixels;
}

void DrawSweepOutline(DraftingWorkspaceState& State, ImDrawList* Canvas, const DraftingCameraFrame& Frame)
{
    if (!State.Sweep.ActiveCondition) return;
    const DraftingSweepSession& Sweep = State.Sweep;
    for (const std::vector<Vector3>& Line : Sweep.Outline)
    {
        std::vector<ImVec2> Pixels;
        for (const Vector3& Point : Line) { ImVec2 Pixel; if (ProjectWorldPoint(State, Frame, Point, Pixel)) Pixels.push_back(Pixel); }
        if (Pixels.size() >= 2) Canvas->AddPolyline(Pixels.data(), int(Pixels.size()), IM_COL32(0x4d, 0xff, 0x9f, 0xff), 3.0f);
    }
    constexpr ImU32 AxisColour  = IM_COL32(0xff, 0xc8, 0x3d, 0xff);                         // Amber: axis / path
    constexpr ImU32 HoverColour = IM_COL32(0x4d, 0xd2, 0xff, 0xff);                         // Cyan: what a click takes
    if (Sweep.Verb == DraftingSweepVerb::Revolve)
    {
        const FeatureStep* Step   = SweepStep(State, Sweep.Path);
        const SketchModel* Sketch = SweepSketch(State, Sweep.Path);
        if (Step && Sketch)
        {
            Vector2 Origin = Sweep.Axis.Origin, Direction = Sweep.Axis.Direction;
            for (const SketchCurve& Curve : Sketch->Curves)
                if (Sweep.Axis.Line >= 0 && Curve.Identifier == Sweep.Axis.Line && Curve.Kind == SketchCurveKind::Line)
                {
                    Origin    = Sketch->Points[Curve.Start].Position;
                    Direction = Sketch->Points[Curve.End].Position - Origin;
                }
            const double Norm = std::hypot(Direction.X, Direction.Y);
            double Reach = 0.5;                                                              // [len] - Half length drawn: past the profile
            for (const SketchPoint& Point : Sketch->Points) Reach = std::max(Reach, 1.5 * std::hypot(Point.Position.X - Origin.X, Point.Position.Y - Origin.Y));
            if (Norm > 0.0)
            {
                const OrthonormalFrame Plane = ElevatedSketchPlane(State.Document, *Step);
                std::vector<ImVec2> Pixels;
                for (int Index = 0; Index <= 32; ++Index)
                {
                    const double Along = Reach * (Index / 16.0 - 1.0) / Norm;
                    ImVec2 Pixel;
                    if (ProjectWorldPoint(State, Frame, PlacePlanePoint(Plane, { Origin.X + Direction.X * Along, Origin.Y + Direction.Y * Along }), Pixel)) Pixels.push_back(Pixel);
                }
                if (Pixels.size() >= 2) DrawDashedPolyline(Canvas, Pixels, AxisColour, 2.0f);
            }
        }
    }
    if (Sweep.Verb == DraftingSweepVerb::Evolution && !Sweep.Path.empty())
    {
        const std::vector<ImVec2> Pixels = SweepCurvePixels(State, Frame, Sweep.Path, -1);
        if (Pixels.size() >= 2) Canvas->AddPolyline(Pixels.data(), int(Pixels.size()), AxisColour, 3.0f);
    }
    if (Sweep.HoverCurve >= 0)
    {
        const std::vector<ImVec2> Pixels = SweepCurvePixels(State, Frame, Sweep.HoverSketch, Sweep.HoverCurve);
        if (Pixels.size() >= 2) Canvas->AddPolyline(Pixels.data(), int(Pixels.size()), HoverColour, 4.0f);
    }
}

// J: the focused sketch (else the newest) welds with every coplanar sketch touching it.
void JoinSketches(DraftingWorkspaceState& State)
{
    std::string Base = FocusedSketchCondition(State) ? State.FocusedStep : std::string{};
    if (Base.empty()) for (auto Step = State.Document.Tree.Steps.rbegin(); Step != State.Document.Tree.Steps.rend() && Base.empty(); ++Step) if (Step->Kind == FeatureStepKind::Sketch) Base = Step->Label;
    if (Base.empty()) { Announce(State, "Join (J) needs sketches: draw some first"); return; }
    const std::vector<std::string> Group = DraftingJoinCandidates(State.Document, Base);
    if (Group.size() < 2) { Announce(State, "Join (J): no other sketch touches " + CaptionOf(State.Document, Base) + " (draw from its ends, or across it)"); return; }
    const DraftingCommandOutcome Outcome = JoinDraftingSketches(State.Document, Group);
    if (Outcome.AcceptedCondition) State.FocusedStep = Base;
    Conclude(State, Outcome);
}

// H / Shift+H / Alt+H / Ctrl+H: visibility is appearance (no regeneration) but the scene drops hidden bodies.
void ChangeVisibility(DraftingWorkspaceState& State, DraftingVisibilityVerb Verb)
{
    const DraftingCommandOutcome Outcome = ChangeDraftingVisibility(State.Document, Verb);
    if (Outcome.AcceptedCondition)
    {
        RefreshDraftingScene(State, true);
        Announce(State, Compose("%s · %d hidden", Outcome.Label.c_str(), int(State.Document.Appearance.HiddenBodies.size())));
    }
    else Announce(State, "Refused: " + Outcome.Refusal);
}

// Ctrl+C: remember the picked bodies and the step they stand after; Ctrl+V pastes them in place as they stood then.
void CopyBodies(DraftingWorkspaceState& State)
{
    const std::vector<std::string> Keys = PickedBodies(State.Document);
    if (Keys.empty() || State.Document.Tree.Steps.empty()) { Announce(State, "Copy (Ctrl+C) needs picked bodies (mode 4)"); return; }
    State.Clipboard = { Keys, State.Document.Tree.Steps.back().Label };
    std::string Captions;
    for (const std::string& Key : Keys) Captions += (Captions.empty() ? "" : ", ") + DraftingBodyCaption(State.Document, Key);
    Announce(State, "Copied " + Captions);
}

void PasteBodies(DraftingWorkspaceState& State)
{
    const DraftingCommandOutcome Outcome = DuplicateDraftingBodies(State.Document, State.Clipboard.Keys, TranslationMap({}), State.Clipboard.Source.empty() ? std::string("—") : State.Clipboard.Source);
    if (Outcome.AcceptedCondition) State.PickMode = DraftingPickMode::Solid;
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
            // 📝 Section (Alt+S): samples on the removed side break the polyline, as the model is clipped there.
            const float Width = Curve.Construction ? 1.0f : Focused ? 2.5f : 1.8f;
            std::vector<ImVec2> Pixels;
            const auto Flush = [&]()
            {
                if (Pixels.size() >= 2) Canvas->AddPolyline(Pixels.data(), int(Pixels.size()), Colour, Width);
                Pixels.clear();
            };
            const std::array<double, 4>& Cut = State.Style.SectionPlane;
            for (const Vector2& Sample : SampleSketchCurve(Sketch, Curve))
            {
                const Vector3 Point = PlacePlanePoint(Plane, Sample);
                if (State.Style.SectionEnabled && Cut[0] * Point.X + Cut[1] * Point.Y + Cut[2] * Point.Z + Cut[3] < 0.0) { Flush(); continue; }
                ImVec2 Pixel;
                if (ProjectWorldPoint(State, Frame, Point, Pixel)) Pixels.push_back(Pixel);
            }
            Flush();
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

// X-ray (Plasticity viewport:toggle-x-ray, Alt+Z): ghost faces at α, hidden edges drawn faded and picked / boxed through.
void ToggleXRay(DraftingWorkspaceState& State)
{
    State.Style.XRayEnabled = !State.Style.XRayEnabled;
    Announce(State, State.Style.XRayEnabled ? Compose("X-ray on · faces %d %% · hidden edges pickable", int(std::lround(State.Style.XRayOpacity * 100.0f)))
                                            : std::string("X-ray off"));
}

void DrawStatusPill(ImDrawList* Canvas, ImVec2& Cursor, const std::string& Text, ImU32 Accent);

// 📝 Snap glyphs (Fusion / Plasticity style): square endpoint, triangle midpoint, ringed centre, cross intersection,
//    ring + bar tangent, ⊥ perpendicular, dot on curve, dot + normal tick on face, dot on a lock line; the caption pill
//    sits at the pointer (+14, +14). The active lock draws as a dashed line through the last point.
void DrawSnapOverlay(DraftingWorkspaceState& State, ImDrawList* Canvas, const DraftingCameraFrame& Frame)
{
    constexpr ImU32 SnapColour    = IM_COL32(0xe6, 0xff, 0x4d, 0xff);                         // #e6ff4d - Snapped point
    constexpr ImU32 OutlineColour = IM_COL32(0x10, 0x12, 0x16, 0xd0);                         // Dark halo behind the glyph
    constexpr ImU32 LockColours[] = { 0, AxisColours[0], AxisColours[1], AxisColours[2], IM_COL32(0xff, 0xc8, 0x3d, 0xff), IM_COL32(0x4d, 0xd2, 0xff, 0xff), IM_COL32(0xe0, 0x8c, 0xff, 0xff) };
    const DraftingPointPicker& Picker = State.Picker;
    const bool Drawing  = State.Drawing.ActiveCondition;
    const bool Moving   = State.Transform.ActiveCondition && State.Transform.Verb == DraftingTransformVerb::Move;
    const bool Dragging = (State.Constrain.ActiveCondition && State.Constrain.DragCondition) || State.PoleDrag.ActiveCondition;
    if (!Drawing && !Moving && !Dragging) return;
    if (Drawing && Picker.Lock != DraftingSnapLockKind::None)
    {
        const double Span = 4000.0 * Frame.WorldPerPixel;
        std::vector<ImVec2> Pixels;
        for (int Index = -40; Index <= 40; ++Index)
        {
            ImVec2 Pixel;
            if (ProjectWorldPoint(State, Frame, Picker.LockOrigin + Picker.LockDirection * (Span * Index / 40.0), Pixel)) Pixels.push_back(Pixel);
        }
        if (Pixels.size() >= 2) DrawDashedPolyline(Canvas, Pixels, LockColours[int(Picker.Lock)], 1.5f);
    }
    const DraftingSnap& Snap = Picker.Current;
    if (Snap.Kind == DraftingSnapKind::None || Snap.Kind == DraftingSnapKind::Grid) return;
    ImVec2 At;
    if (!ProjectWorldPoint(State, Frame, Snap.Position, At)) return;
    auto Glyph = [&](ImU32 Colour, float Width)
    {
        const float R = 6.0f;
        switch (Snap.Kind)
        {
        case DraftingSnapKind::Endpoint: Canvas->AddRect(ImVec2(At.x - R, At.y - R), ImVec2(At.x + R, At.y + R), Colour, 0.0f, Width); break;
        case DraftingSnapKind::Midpoint: Canvas->AddTriangle(ImVec2(At.x, At.y - R - 1.0f), ImVec2(At.x - R, At.y + R - 1.0f), ImVec2(At.x + R, At.y + R - 1.0f), Colour, Width); break;
        case DraftingSnapKind::Centre:
            Canvas->AddCircle(At, R, Colour, 20, Width);
            Canvas->AddLine(ImVec2(At.x - R - 3.0f, At.y), ImVec2(At.x + R + 3.0f, At.y), Colour, Width * 0.5f);
            Canvas->AddLine(ImVec2(At.x, At.y - R - 3.0f), ImVec2(At.x, At.y + R + 3.0f), Colour, Width * 0.5f);
            break;
        case DraftingSnapKind::Intersection:
            Canvas->AddLine(ImVec2(At.x - R, At.y - R), ImVec2(At.x + R, At.y + R), Colour, Width);
            Canvas->AddLine(ImVec2(At.x - R, At.y + R), ImVec2(At.x + R, At.y - R), Colour, Width);
            break;
        case DraftingSnapKind::Tangent:
            Canvas->AddCircle(ImVec2(At.x, At.y + R * 0.5f), R * 0.8f, Colour, 20, Width);
            Canvas->AddLine(ImVec2(At.x - R - 3.0f, At.y - R * 0.3f), ImVec2(At.x + R + 3.0f, At.y - R * 0.3f), Colour, Width);
            break;
        case DraftingSnapKind::Perpendicular:
            Canvas->AddLine(ImVec2(At.x - R, At.y + R), ImVec2(At.x + R, At.y + R), Colour, Width);
            Canvas->AddLine(ImVec2(At.x, At.y + R), ImVec2(At.x, At.y - R), Colour, Width);
            Canvas->AddRect(ImVec2(At.x, At.y + R * 0.2f), ImVec2(At.x + R * 0.8f, At.y + R), Colour, 0.0f, Width * 0.5f);
            break;
        case DraftingSnapKind::Face:
        {
            ImVec2 Tip;
            if (ProjectWorldPoint(State, Frame, Snap.Position + Snap.Normal * (24.0 * Frame.WorldPerPixel), Tip)) Canvas->AddLine(At, Tip, Colour, Width);
            Canvas->AddCircleFilled(At, 3.5f, Colour);
            break;
        }
        default: Canvas->AddCircleFilled(At, 4.0f, Colour); break;
        }
    };
    Glyph(OutlineColour, 4.0f);
    Glyph(Snap.Kind == DraftingSnapKind::Axis ? LockColours[int(Picker.Lock)] : SnapColour, 2.0f);
    const ImVec2 Pointer = ImGui::GetIO().MousePos;
    std::string Text = Snap.Kind == DraftingSnapKind::Axis ? Snap.Source : std::string(DraftingSnapLabel(Snap.Kind)) + " · " + Snap.Source;
    if (Text.size() > 72) Text = Text.substr(0, 69) + "...";
    // Flip left / up near the viewport's right and bottom edges (the hint line lives along the bottom)
    const float  Width  = ImGui::CalcTextSize(Text.c_str()).x + 26.0f;
    const float  Right  = State.ViewportRect[0] + State.ViewportRect[2];
    const float  Bottom = State.ViewportRect[1] + State.ViewportRect[3];
    ImVec2 Caption(Pointer.x + 14.0f, Pointer.y + 14.0f);
    if (Caption.x + Width > Right - 8.0f) Caption.x = Pointer.x - 14.0f - Width;
    if (Caption.y + 26.0f > Bottom - 40.0f) Caption.y = Pointer.y - 14.0f - 26.0f;
    DrawStatusPill(Canvas, Caption, Text, Snap.Kind == DraftingSnapKind::Axis ? LockColours[int(Picker.Lock)] : SnapColour);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    JOINT (ALT+J)
//------------------------------------------------------------------------------------------------------------------------

// Fingerprint of a request: the staged preview is rebuilt only when it changes.
std::string JointRequestKey(const DraftingJointRequest& Request)
{
    return Compose("%d|%s|%d|%s|%d|%.17g|%.17g|%.17g|%.17g|%.17g|%.17g|%d", int(Request.MovingKind), Request.MovingName.c_str(), int(Request.FixedKind), Request.FixedName.c_str(),
                   int(Request.Kind), Request.Angle, Request.Offset, Request.Slide, Request.SecondSlide, Request.Pitch, Request.Roll, Request.Flipped ? 1 : 0);
}

void StageJointPreview(DraftingWorkspaceState& State)
{
    DraftingJointSession& Joint = State.Joint;
    if (!Joint.ActiveCondition || Joint.Phase < 2) return;
    const std::string Key = JointRequestKey(Joint.Request);
    if (Key == Joint.StagedKey) return;
    Joint.StagedKey = Key;
    Joint.Refusal.clear();
    Joint.Preview = std::make_unique<DraftingDocument>(CloneDraftingDocument(State.Document));
    const DraftingCommandOutcome Outcome = JointDraftingBodies(*Joint.Preview, Joint.Request);
    if (!Outcome.AcceptedCondition) { Joint.Refusal = Legible(Outcome.Refusal); Joint.Preview.reset(); return; }
    Joint.PreviewScene = BuildDraftingScene(*Joint.Preview);
}

void CloseJointSession(DraftingWorkspaceState& State, const char* Message)
{
    State.Joint.ActiveCondition = false;
    State.Joint.Preview.reset();
    State.Joint.StagedKey.clear();
    State.Joint.HoverName.clear();
    if (Message) Announce(State, Message);
}

void AnnounceJointAdjust(DraftingWorkspaceState& State)
{
    const DraftingJointSession& Joint = State.Joint;
    Announce(State, Compose("Joint: %s onto %s · R V S C P L B kind · drag the values · F flip · Enter applies", DraftingBodyCaption(State.Document, Joint.Moving.Body).c_str(),
                            DraftingBodyCaption(State.Document, Joint.Fixed.Body).c_str()));
}

// 💡 Alt+J (Fusion Joint; Plasticity has none). Two entities picked beforehand on two bodies skip the picking: the earlier
//    pick (pick order of one kind; for a face and an edge, the one picked last is fixed) is on the moving body.
void OpenJointSession(DraftingWorkspaceState& State)
{
    if (State.Document.Summary.Bodies.size() < 2)
    {
        Announce(State, "Joint (Alt+J) needs two bodies: construct one with R (New body) in Shift+C / V / X / Z");
        return;
    }
    DraftingJointSession& Joint   = State.Joint;
    const int             Commits = Joint.Commits;
    const JointKind       Kind    = Joint.Request.Kind;                                     // The last kind is kept, as the bevel keeps its shape
    CloseJointSession(State, nullptr);
    Joint                 = DraftingJointSession{};
    Joint.Commits         = Commits;
    Joint.Request.Kind    = Kind;
    Joint.ActiveCondition = true;
    const DraftingSelection& Selection = State.Document.Selection;
    std::vector<std::pair<DraftingSelectionKind, std::string>> Picks;
    if (Selection.Faces.size() + Selection.Edges.size() == 2)
    {
        constexpr DraftingSelectionKind Edge = DraftingSelectionKind::Edge, Face = DraftingSelectionKind::Face;
        if (Selection.Edges.size() == 2) Picks = { { Edge, Selection.Edges[0] }, { Edge, Selection.Edges[1] } };
        else if (Selection.Faces.size() == 2) Picks = { { Face, Selection.Faces[0] }, { Face, Selection.Faces[1] } };
        else if (Selection.LastKind == Edge) Picks = { { Face, Selection.Faces[0] }, { Edge, Selection.Edges[0] } };
        else Picks = { { Edge, Selection.Edges[0] }, { Face, Selection.Faces[0] } };
    }
    if (Picks.size() == 2)
    {
        const DraftingJointOrigin First  = ResolveDraftingJointOrigin(State.Document, Picks[0].first, Picks[0].second);
        const DraftingJointOrigin Second = ResolveDraftingJointOrigin(State.Document, Picks[1].first, Picks[1].second);
        if (First.Refusal.empty() && Second.Refusal.empty() && First.Body != Second.Body)
        {
            Joint.Moving             = First;
            Joint.Fixed              = Second;
            Joint.Request.MovingKind = Picks[0].first;
            Joint.Request.MovingName = Picks[0].second;
            Joint.Request.FixedKind  = Picks[1].first;
            Joint.Request.FixedName  = Picks[1].second;
            Joint.Phase              = 2;
            StageJointPreview(State);
            AnnounceJointAdjust(State);
            return;
        }
    }
    Announce(State, "Joint: pick the joint origin on the moving body (an edge or a face)");
}

void ChooseJointKind(DraftingWorkspaceState& State, JointKind Kind)
{
    State.Joint.Request.Kind = Kind;
    StageJointPreview(State);
    Announce(State, Compose("Joint: %s · %d degree(s) of freedom", JointKindLabel(Kind), JointFreedom(Kind)));
}

void FlipJoint(DraftingWorkspaceState& State)
{
    State.Joint.Request.Flipped = !State.Joint.Request.Flipped;
    StageJointPreview(State);
    Announce(State, State.Joint.Request.Flipped ? "Joint: flipped (origins aligned)" : "Joint: origins facing");
}

// The edge (within 8 px, else the face) under the pointer and the joint origin it offers — the hover glyph.
void HoverJointOrigin(DraftingWorkspaceState& State, const DraftingCameraFrame& Frame, ImVec2 Local, bool Hovered)
{
    DraftingJointSession& Joint = State.Joint;
    if (Joint.Phase >= 2) return;
    uint32_t Identity = Hovered ? State.HoverIdentity : 0;
    if (Hovered && !(Identity & IdentityEdgeBit))
    {
        const DraftingRay                 Ray    = CastDraftingRay(Frame, Local.x, Local.y);
        const std::optional<DraftingPick> Nearby = PickDraftingScene(State.Scene, Ray, 8.0 * Frame.WorldPerPixel, State.Style.XRayEnabled, ActiveSectionClip(State));
        if (Nearby && (Nearby->Identity & IdentityEdgeBit)) Identity = Nearby->Identity;
    }
    if (Identity == IdentitySectionCap) Identity = 0;
    const std::string           Name = Identity ? NameDraftingIdentity(State.Scene, Identity) : std::string();
    const DraftingSelectionKind Kind = Identity ? DraftingIdentityKind(Identity) : DraftingSelectionKind::Edge;
    if (Name == Joint.HoverName && Kind == Joint.HoverKind) return;
    Joint.HoverName = Name;
    Joint.HoverKind = Kind;
    Joint.Hover     = Name.empty() ? DraftingJointOrigin{} : ResolveDraftingJointOrigin(State.Document, Kind, Name);
}

void ClickJointSession(DraftingWorkspaceState& State)
{
    DraftingJointSession& Joint = State.Joint;
    if (Joint.Phase >= 2) return;
    if (Joint.HoverName.empty()) { Announce(State, "Joint: point at an edge or a face"); return; }
    if (!Joint.Hover.Refusal.empty()) { Announce(State, "Joint: " + Joint.Hover.Refusal); return; }
    if (Joint.Phase == 0)
    {
        Joint.Moving             = Joint.Hover;
        Joint.Request.MovingKind = Joint.HoverKind;
        Joint.Request.MovingName = Joint.HoverName;
        Joint.Phase              = 1;
        Announce(State, Compose("Joint: %s on %s moves · now pick the joint origin on the body it joins", Joint.Moving.Description.c_str(),
                                DraftingBodyCaption(State.Document, Joint.Moving.Body).c_str()));
        return;
    }
    if (Joint.Hover.Body == Joint.Moving.Body) { Announce(State, "Joint: pick the second joint origin on another body"); return; }
    Joint.Fixed             = Joint.Hover;
    Joint.Request.FixedKind = Joint.HoverKind;
    Joint.Request.FixedName = Joint.HoverName;
    Joint.Phase             = 2;
    StageJointPreview(State);
    AnnounceJointAdjust(State);
}

void ApplyJointSession(DraftingWorkspaceState& State)
{
    DraftingJointSession& Joint = State.Joint;
    if (Joint.Phase < 2) { Announce(State, "Joint: pick both joint origins first"); return; }
    const DraftingJointRequest Request = Joint.Request;
    CloseJointSession(State, nullptr);
    const DraftingCommandOutcome Outcome = JointDraftingBodies(State.Document, Request);
    if (Outcome.AcceptedCondition)
    {
        ++Joint.Commits;
        State.FocusedStep = State.Document.Tree.Steps.back().Label;                          // The step card drags its values
    }
    Conclude(State, Outcome);
}

void DrawJointCard(DraftingWorkspaceState& State)
{
    DraftingJointSession& Joint = State.Joint;
    BeginCard("Joint");
    static const struct { const char* Label; JointKind Kind; } Kinds[] = {
        { "Rigid (R)###JointRigid", JointKind::Rigid }, { "Revolute (V)###JointRevolute", JointKind::Revolute }, { "Slider (S)###JointSlider", JointKind::Slider },
        { "Cylindrical (C)###JointCylindrical", JointKind::Cylindrical }, { "Pin-slot (P)###JointPinSlot", JointKind::PinSlot }, { "Planar (L)###JointPlanar", JointKind::Planar },
        { "Ball (B)###JointBall", JointKind::Ball } };
    const float Half = std::floor((ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) / 2.0f);
    for (int Index = 0; Index < 7; ++Index)
    {
        if (Index % 2 == 1) ImGui::SameLine();
        if (ToggleButton(State, Kinds[Index].Label, Joint.Request.Kind == Kinds[Index].Kind, ImVec2(Half, 0))) ChooseJointKind(State, Kinds[Index].Kind);
    }
    const auto Origin = [&](const char* Caption, const DraftingJointOrigin& Picked)
    {
        ImGui::Text("%s  %s on %s", Caption, Picked.Description.c_str(), DraftingBodyCaption(State.Document, Picked.Body).c_str());
    };
    if (Joint.Phase == 0) ImGui::TextColored(Tint(ActiveColour), "1  Pick the origin on the moving body");
    if (Joint.Phase >= 1) Origin("Moving", Joint.Moving);
    if (Joint.Phase == 1) ImGui::TextColored(Tint(ActiveColour), "2  Pick the origin on the fixed body");
    if (Joint.Phase == 2)
    {
        Origin("Fixed ", Joint.Fixed);
        DraftingJointRequest& Request = Joint.Request;
        bool Changed = false;
        const auto Field = [&](const char* Label, double& Value, float Speed, const char* Format)
        {
            ImGui::SetNextItemWidth(150);
            Changed |= ImGui::DragScalar(Label, ImGuiDataType_Double, &Value, Speed, nullptr, nullptr, Format);
            RegisterWidget(State, Label);
        };
        const JointKind Kind = Request.Kind;
        Field(Kind == JointKind::Ball ? "Yaw###JointAngle" : "Angle###JointAngle", Request.Angle, 0.5f, "%.2f deg");
        if (Kind == JointKind::Ball)
        {
            Field("Pitch###JointPitch", Request.Pitch, 0.5f, "%.2f deg");
            Field("Roll###JointRoll", Request.Roll, 0.5f, "%.2f deg");
        }
        if (Kind == JointKind::Slider || Kind == JointKind::Cylindrical || Kind == JointKind::PinSlot || Kind == JointKind::Planar)
            Field(Kind == JointKind::Planar ? "Slide x###JointSlide" : "Slide###JointSlide", Request.Slide, 0.005f, "%.4f m");
        if (Kind == JointKind::Planar) Field("Slide y###JointSecondSlide", Request.SecondSlide, 0.005f, "%.4f m");
        Field("Offset###JointOffset", Request.Offset, 0.005f, "%.4f m");
        Changed |= ImGui::Checkbox("Flip (F)###JointFlip", &Request.Flipped);
        RegisterWidget(State, "Flip (F)###JointFlip");
        if (Changed) StageJointPreview(State);
        ImGui::TextDisabled("%s · %d degree(s) of freedom", JointKindLabel(Kind), JointFreedom(Kind));
    }
    if (!Joint.Refusal.empty())
    {
        ImGui::PushStyleColor(ImGuiCol_Text, Tint(RefusalColour));
        ImGui::TextWrapped("%s", Joint.Refusal.c_str());
        ImGui::PopStyleColor();
    }
    ImGui::PushStyleColor(ImGuiCol_Text, Tint(MutedColour));
    ImGui::TextWrapped("Edges give centres / midpoints, faces centroids / axes · R V S C P L B kind · drag a value · F flip · Enter applies · Esc cancels. "
                       "The moving body and every body jointed to it move; the fixed body stays.");
    ImGui::PopStyleColor();
    if (PressButton(State, "Apply###JointApply", ImVec2(118, 0), Joint.Phase == 2 && Joint.Refusal.empty())) ApplyJointSession(State);
    ImGui::SameLine();
    if (PressButton(State, "Cancel###JointCancel", ImVec2(118, 0))) CloseJointSession(State, "Joint cancelled");
    EndCard();
}

// 📝 Joint origin glyph: a ring in the origin's plane (while picking), z (blue, long) and x (red) arrows, a caption pill.
void DrawJointGlyph(DraftingWorkspaceState& State, ImDrawList* Canvas, const DraftingCameraFrame& Frame, const OrthonormalFrame& Origin, const std::string& Caption, ImU32 Accent,
                    bool RingCondition = true)
{
    constexpr ImU32 HaloColour = IM_COL32(0x10, 0x12, 0x16, 0xd0);
    const double    Unit       = Frame.WorldPerPixel;
    ImVec2 Centre, Tip;
    if (!ProjectWorldPoint(State, Frame, Origin.Origin, Centre)) return;
    std::vector<ImVec2> Ring;
    for (int Index = 0; Index <= 32; ++Index)
    {
        const double Turn = 2.0 * std::numbers::pi * Index / 32.0;
        ImVec2 Pixel;
        if (ProjectWorldPoint(State, Frame, Origin.Origin + Origin.XAxis * (18.0 * Unit * std::cos(Turn)) + Origin.YAxis * (18.0 * Unit * std::sin(Turn)), Pixel)) Ring.push_back(Pixel);
    }
    if (RingCondition && Ring.size() > 2)
    {
        Canvas->AddPolyline(Ring.data(), int(Ring.size()), HaloColour, 4.0f);
        Canvas->AddPolyline(Ring.data(), int(Ring.size()), Accent, 2.0f);
    }
    const auto Arrow = [&](Vector3 Direction, double Pixels, ImU32 Colour)
    {
        if (!ProjectWorldPoint(State, Frame, Origin.Origin + Direction * (Pixels * Unit), Tip)) return;
        Canvas->AddLine(Centre, Tip, HaloColour, 4.5f);
        Canvas->AddLine(Centre, Tip, Colour, 2.5f);
        Canvas->AddCircleFilled(Tip, 3.5f, Colour);
    };
    Arrow(Origin.XAxis, 30.0, AxisColours[0]);
    Arrow(Origin.ZAxis, 48.0, AxisColours[2]);
    Canvas->AddCircleFilled(Centre, 5.0f, HaloColour);
    Canvas->AddCircleFilled(Centre, 3.5f, Accent);
    if (!Caption.empty())
    {
        ImVec2 Pill(Centre.x + 14.0f, Centre.y + 6.0f);
        DrawStatusPill(Canvas, Pill, Caption, Accent);
    }
}

// Overlay of the Joint card (hovered origin, picked origins, the placed origin of the preview) and of a focused Joint step.
void DrawJointOverlay(DraftingWorkspaceState& State, ImDrawList* Canvas, const DraftingCameraFrame& Frame)
{
    constexpr ImU32 HoverColour  = IM_COL32(0x4d, 0xd2, 0xff, 0xff);                         // Cyan: what a click takes
    constexpr ImU32 PickedColour = IM_COL32(0xff, 0xc8, 0x3d, 0xff);                         // Amber: chosen origins
    const DraftingJointSession& Joint = State.Joint;
    if (Joint.ActiveCondition)
    {
        if (Joint.Phase < 2 && !Joint.HoverName.empty() && Joint.Hover.Refusal.empty()) DrawJointGlyph(State, Canvas, Frame, Joint.Hover.Frame, Joint.Hover.Description, HoverColour);
        if (Joint.Phase >= 1 && Joint.Phase < 2) DrawJointGlyph(State, Canvas, Frame, Joint.Moving.Frame, "moving", PickedColour);
        if (Joint.Phase == 2)
        {
            DrawJointGlyph(State, Canvas, Frame, Joint.Fixed.Frame, std::string(JointKindLabel(Joint.Request.Kind)) + " joint", PickedColour);
            if (Joint.Preview && !Joint.Preview->Regeneration.Joints.empty()) DrawJointGlyph(State, Canvas, Frame, Joint.Preview->Regeneration.Joints.back().MovingOrigin, "", HoverColour);
        }
        return;
    }
    if (State.Measure.ActiveCondition) return;                                              // Measure's own pills take the view
    const DraftingDocument& Shown = State.ParameterDrag.Preview ? *State.ParameterDrag.Preview : State.Document;
    for (const RegeneratedJoint& Placed : Shown.Regeneration.Joints)
        if (Placed.Label == State.FocusedStep) DrawJointGlyph(State, Canvas, Frame, Placed.FixedOrigin, std::string(JointKindLabel(Placed.Kind)) + " joint", PickedColour, false);
}

// Measure (I): recompute only when the selection or the document revision moved (a NURBS body takes ~1 s).
void RefreshMeasure(DraftingWorkspaceState& State)
{
    DraftingMeasureSession& Measure = State.Measure;
    if (!Measure.ActiveCondition) return;
    if (Measure.Refreshes > 0 && Measure.Measurement.Revision == State.Document.Revision
        && Measure.Measurement.SelectionKey == DraftingSelectionKey(State.Document.Selection)) return;
    Measure.Measurement = MeasureDraftingSelection(State.Document);
    Measure.Refreshes  += 1;
}

// Measure (I; Fusion Inspect → Measure, no Plasticity default): card + overlay of the selection's exact figures.
void ToggleMeasure(DraftingWorkspaceState& State)
{
    DraftingMeasureSession& Measure = State.Measure;
    Measure.ActiveCondition = !Measure.ActiveCondition;
    Measure.Refreshes       = 0;
    if (!Measure.ActiveCondition) { Announce(State, "Measure off"); return; }
    RefreshMeasure(State);
    const DraftingMeasurement& Measurement = Measure.Measurement;
    if (!Measurement.Refusal.empty()) Announce(State, Measurement.Refusal);
    else Announce(State, "Measure · " + Measurement.Subject + (Measurement.Rows.empty() ? std::string{} : " · " + Measurement.Rows.front().Caption + " " + Measurement.Rows.front().Value));
}

// 📝 Inspector card: subject, then caption / value rows (values wrap under long captions), timing and the selection rule.
void DrawMeasureCard(DraftingWorkspaceState& State)
{
    if (!State.Measure.ActiveCondition) return;
    const DraftingMeasurement& Measurement = State.Measure.Measurement;
    BeginCard("Measure");
    if (!Measurement.Refusal.empty())
    {
        ImGui::PushStyleColor(ImGuiCol_Text, Tint(RefusalColour));
        ImGui::TextWrapped("%s", Measurement.Refusal.c_str());
        ImGui::PopStyleColor();
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Text, Tint(ActiveColour));
        ImGui::TextWrapped("%s", Measurement.Subject.c_str());
        ImGui::PopStyleColor();
        for (const DraftingMeasureRow& Row : Measurement.Rows)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, Tint(MutedColour));
            ImGui::TextUnformatted(Row.Caption.c_str());
            ImGui::PopStyleColor();
            const float Indent = 128.0f;
            if (ImGui::CalcTextSize(Row.Caption.c_str()).x + 8.0f < Indent) ImGui::SameLine(Indent);
            else ImGui::Indent(16.0f);
            ImGui::TextUnformatted(Row.Value.c_str());
            if (ImGui::CalcTextSize(Row.Caption.c_str()).x + 8.0f >= Indent) ImGui::Unindent(16.0f);
        }
        ImGui::TextDisabled("exact · %.0f ms", Measurement.Milliseconds);
    }
    ImGui::PushStyleColor(ImGuiCol_Text, Tint(MutedColour));
    ImGui::TextWrapped("1 face / edge · 2 for distance + angle · body (4) or nothing for mass · 2 bodies (4) for interference");
    ImGui::PopStyleColor();
    if (PressButton(State, "Close (I)###MeasureClose")) ToggleMeasure(State);
    EndCard();
}

// 📝 Viewport overlay: amber dimension line between the closest points with end dots and a distance pill at its middle;
//    a centroid cross-hair for a body / face.
void DrawMeasureOverlay(DraftingWorkspaceState& State, ImDrawList* Canvas, const DraftingCameraFrame& Frame)
{
    if (!State.Measure.ActiveCondition) return;
    const DraftingMeasurement& Measurement = State.Measure.Measurement;
    constexpr ImU32 DimensionColour = IM_COL32(0xff, 0xc8, 0x3d, 0xff);                     // Amber: dimension
    constexpr ImU32 CentroidColour  = IM_COL32(0x4d, 0xd2, 0xff, 0xff);                     // Cyan: centre of mass / area
    ImVec2 First, Second;
    if (Measurement.DistanceCondition && ProjectWorldPoint(State, Frame, Measurement.First, First) && ProjectWorldPoint(State, Frame, Measurement.Second, Second))
    {
        Canvas->AddLine(First, Second, IM_COL32(0x10, 0x10, 0x10, 0xc0), 4.0f);
        Canvas->AddLine(First, Second, DimensionColour, 2.0f);
        Canvas->AddCircleFilled(First, 4.5f, DimensionColour);
        Canvas->AddCircleFilled(Second, 4.5f, DimensionColour);
        const std::string Label = Compose("%.4f m", Measurement.Distance) + (std::isnan(Measurement.Angle) ? std::string{} : Compose(" · %.2f°", Measurement.Angle));
        ImVec2 Pill(0.5f * (First.x + Second.x) + 10.0f, 0.5f * (First.y + Second.y) - 13.0f);
        DrawStatusPill(Canvas, Pill, Label, DimensionColour);
    }
    // Two bodies overlapping: the edges of the shared volume, red over everything.
    for (const std::vector<Vector3>& Polyline : Measurement.Overlap)
    {
        std::vector<ImVec2> Pixels;
        for (const Vector3& Point : Polyline)
        {
            ImVec2 Pixel;
            if (ProjectWorldPoint(State, Frame, Point, Pixel)) Pixels.push_back(Pixel);
        }
        if (Pixels.size() < 2) continue;
        Canvas->AddPolyline(Pixels.data(), int(Pixels.size()), IM_COL32(0x10, 0x10, 0x10, 0xc0), 4.5f);
        Canvas->AddPolyline(Pixels.data(), int(Pixels.size()), RefusalColour, 2.5f);
    }
    ImVec2 Centre;
    if (Measurement.CentroidCondition && ProjectWorldPoint(State, Frame, Measurement.Centroid, Centre))
    {
        Canvas->AddCircle(Centre, 7.0f, IM_COL32(0x10, 0x10, 0x10, 0xc0), 0, 4.0f);
        Canvas->AddCircle(Centre, 7.0f, CentroidColour, 0, 2.0f);
        Canvas->AddLine(ImVec2(Centre.x - 11, Centre.y), ImVec2(Centre.x + 11, Centre.y), CentroidColour, 1.5f);
        Canvas->AddLine(ImVec2(Centre.x, Centre.y - 11), ImVec2(Centre.x, Centre.y + 11), CentroidColour, 1.5f);
        ImVec2 Pill(Centre.x + 12.0f, Centre.y + 4.0f);
        DrawStatusPill(Canvas, Pill, "Centroid", CentroidColour);
    }
}

// Plane equation (n, w) of a section frame: the kept side n·P + w ≥ 0 is the frame's +Z side.
std::array<double, 4> SectionEquation(const OrthonormalFrame& Plane)
{
    return { Plane.ZAxis.X, Plane.ZAxis.Y, Plane.ZAxis.Z, -Dot(Plane.ZAxis, Plane.Origin) };
}

// Pill / announcement caption of the request ("Section y = 1.25", "Section face F3 −0.5").
std::string DescribeSectionRequest(const DraftingSectionRequest& Request)
{
    static const char* Axes[] = { "x", "y", "z" };
    const std::string Flip = Request.Flipped ? " (flipped)" : "";
    if (Request.Axis == DraftingSectionAxis::Face) return "Section on " + Request.Face + " at " + FormatLength(Request.Offset) + Flip;
    return std::string("Section ") + Axes[int(Request.Axis)] + " = " + FormatLength(Request.Offset) + Flip;
}

// Default offsets: world planes through the centre of the shown scene's bounds; a face plane halfway through the
// bounds behind the face (offset along the outward normal, negative = into the body).
double DefaultSectionOffset(const DraftingWorkspaceState& State, const DraftingSectionRequest& Request)
{
    const DraftingScene& Scene = State.Scene;
    if (Request.Axis != DraftingSectionAxis::Face)
    {
        const int Axis = int(Request.Axis);
        return 0.5 * (Scene.BoundsLow[Axis] + Scene.BoundsHigh[Axis]);
    }
    DraftingSectionRequest AtFace = Request;
    AtFace.Offset  = 0.0;
    AtFace.Flipped = false;
    const DraftingSection Plane = ResolveDraftingSectionPlane(State.Document, AtFace);
    if (!Plane.PlaneCondition) return 0.0;
    double Deepest = 0.0;                                                                    // [len] - Farthest corner along −outward
    for (int Corner = 0; Corner < 8; ++Corner)
    {
        const Vector3 Point{ (Corner & 1) ? Scene.BoundsHigh[0] : Scene.BoundsLow[0], (Corner & 2) ? Scene.BoundsHigh[1] : Scene.BoundsLow[1],
                             (Corner & 4) ? Scene.BoundsHigh[2] : Scene.BoundsLow[2] };
        Deepest = std::max(Deepest, Dot(Point - Plane.Plane.Origin, Plane.Plane.ZAxis));     // ZAxis = −outward at offset 0
    }
    return -0.5 * Deepest;
}

void StoreSectionOffsetText(DraftingSectionSession& Session)
{
    std::snprintf(Session.OffsetText.data(), Session.OffsetText.size(), "%.6g", Session.Request.Offset);
}

// 💡 Every frame while Section is on: the plane goes to the viewport programs (clip + cap) and the CPU pick; the exact
//    kernel section is recomputed only when the request or the document revision moved (0.2 … 2 s for curved bodies).
void RefreshSection(DraftingWorkspaceState& State)
{
    DraftingSectionSession& Session = State.Section;
    State.Style.SectionEnabled = false;
    if (!Session.ActiveCondition) return;
    const DraftingSection Plane = ResolveDraftingSectionPlane(State.Document, Session.Request);
    if (!Plane.PlaneCondition)
    {
        if (Session.Section.RequestKey != Plane.RequestKey || Session.Section.Revision != Plane.Revision) Session.Section = Plane;
        return;
    }
    State.Style.SectionEnabled = true;
    State.Style.SectionPlane   = SectionEquation(Plane.Plane);
    Session.Clip = ComposeDraftingSectionClip(DisplayedDraftingScene(State), State.Document, State.Style.SectionPlane);
    if (Session.Refreshes > 0 && Session.Section.Revision == State.Document.Revision && Session.Section.RequestKey == Plane.RequestKey) return;
    Session.Section    = SectionDraftingDocument(State.Document, Session.Request);
    Session.Refreshes += 1;
}

// The section clip for CPU picks while Section is on (null otherwise).
const DraftingSectionClip* ActiveSectionClip(const DraftingWorkspaceState& State)
{
    return State.Style.SectionEnabled ? &State.Section.Clip : nullptr;
}

void ChooseSectionAxis(DraftingWorkspaceState& State, DraftingSectionAxis Axis)
{
    DraftingSectionSession& Session = State.Section;
    Session.Request.Axis = Axis;
    if (Axis == DraftingSectionAxis::Face)
    {
        if (State.Document.Selection.Faces.empty()) { Announce(State, "Section face: select a planar face first (mode 3)"); Session.Request.Face = {}; }
        else Session.Request.Face = State.Document.Selection.Faces.back();
    }
    Session.Request.Offset = DefaultSectionOffset(State, Session.Request);
    StoreSectionOffsetText(Session);
    RefreshSection(State);
    Announce(State, Session.Section.Refusal.empty() ? DescribeSectionRequest(Session.Request) + Compose(" · A %.6g m²", Session.Section.Area) : Session.Section.Refusal);
}

// Section (Alt+S; Fusion Section Analysis, no Plasticity default): default plane y through the scene centre with the
// front half (facing the isometric camera) removed; a selected planar face starts a face section instead.
void ToggleSection(DraftingWorkspaceState& State)
{
    DraftingSectionSession& Session = State.Section;
    Session.ActiveCondition = !Session.ActiveCondition;
    Session.Refreshes       = 0;
    if (!Session.ActiveCondition)
    {
        State.Style.SectionEnabled = false;
        Announce(State, "Section off");
        return;
    }
    Session.Request.Flipped = false;
    ChooseSectionAxis(State, State.Document.Selection.Faces.empty() ? DraftingSectionAxis::Y : DraftingSectionAxis::Face);
    if (!Session.Section.Refusal.empty() && Session.Request.Axis == DraftingSectionAxis::Face) ChooseSectionAxis(State, DraftingSectionAxis::Y);
}

void AdjustSectionOffset(DraftingWorkspaceState& State, double Delta)
{
    State.Section.Request.Offset += Delta;
    StoreSectionOffsetText(State.Section);
    RefreshSection(State);
    Announce(State, DescribeSectionRequest(State.Section.Request));
}

void FlipSection(DraftingWorkspaceState& State)
{
    State.Section.Request.Flipped = !State.Section.Request.Flipped;
    RefreshSection(State);
    Announce(State, DescribeSectionRequest(State.Section.Request));
}

// 📝 Inspector card: plane family (X / Y / Z / Face), offset field (Enter applies; units as everywhere), − / + steps of
//    1/20 of the bounds diagonal, Flip, and the exact figures (u = plane x axis, v = plane y axis, about the centroid).
void DrawSectionCard(DraftingWorkspaceState& State)
{
    DraftingSectionSession& Session = State.Section;
    if (!Session.ActiveCondition) return;
    BeginCard("Section");
    const float Spacing = ImGui::GetStyle().ItemSpacing.x;
    const float Quarter = std::floor((ImGui::GetContentRegionAvail().x - 3.0f * Spacing) / 4.0f);
    static const char* AxisLabels[] = { "X###SectionAxisX", "Y###SectionAxisY", "Z###SectionAxisZ", "Face###SectionAxisFace" };
    for (int Axis = 0; Axis < 4; ++Axis)
    {
        if (Axis) ImGui::SameLine();
        if (ToggleButton(State, AxisLabels[Axis], int(Session.Request.Axis) == Axis, ImVec2(Quarter, 0))) ChooseSectionAxis(State, DraftingSectionAxis(Axis));
    }
    const DraftingScene& Scene = State.Scene;
    double Diagonal = 0.0;
    for (int Axis = 0; Axis < 3; ++Axis) Diagonal += (Scene.BoundsHigh[Axis] - Scene.BoundsLow[Axis]) * (Scene.BoundsHigh[Axis] - Scene.BoundsLow[Axis]);
    const double Step = std::max(1.0e-3, 0.05 * std::sqrt(Diagonal));
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 3.0f * (34.0f + Spacing) - 8.0f);
    if (ImGui::InputText("###SectionOffset", Session.OffsetText.data(), Session.OffsetText.size(), ImGuiInputTextFlags_EnterReturnsTrue))
    {
        double Value = 0.0;
        if (ParseDraftingLength(Session.OffsetText.data(), Value)) { Session.Request.Offset = Value; RefreshSection(State); Announce(State, DescribeSectionRequest(Session.Request)); }
        else Announce(State, std::string("Section offset: cannot read \"") + Session.OffsetText.data() + "\"");
    }
    RegisterWidget(State, "###SectionOffset");
    ImGui::SameLine();
    if (PressButton(State, "-###SectionCloser", ImVec2(34, 0))) AdjustSectionOffset(State, -Step);
    ImGui::SameLine();
    if (PressButton(State, "+###SectionFarther", ImVec2(34, 0))) AdjustSectionOffset(State, Step);
    ImGui::SameLine();
    if (ToggleButton(State, "F###SectionFlip", Session.Request.Flipped, ImVec2(34, 0))) FlipSection(State);
    const DraftingSection& Section = Session.Section;
    if (!Section.Refusal.empty())
    {
        ImGui::PushStyleColor(ImGuiCol_Text, Tint(RefusalColour));
        ImGui::TextWrapped("%s", Section.Refusal.c_str());
        ImGui::PopStyleColor();
    }
    else
    {
        const auto Row = [](const char* Caption, const std::string& Value)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, Tint(MutedColour));
            ImGui::TextUnformatted(Caption);
            ImGui::PopStyleColor();
            ImGui::SameLine(128.0f);
            ImGui::TextUnformatted(Value.c_str());
        };
        Row("Area", Compose("%.6g m²", Section.Area));
        Row("Perimeter", Compose("%.6g m", Section.Perimeter));
        Row("Centroid", Compose("(%.4f, %.4f, %.4f)", Section.Centroid.X, Section.Centroid.Y, Section.Centroid.Z));
        Row("Iuu Ivv", Compose("%.5g · %.5g m4", Section.SecondMoments[0], Section.SecondMoments[1]));
        Row("Iuv", Compose("%.5g m4", Section.SecondMoments[2]));
        Row("Pieces", Compose("%d rgn · %d loop · %d body", Section.Regions, Section.Loops, Section.Bodies));
        ImGui::TextDisabled("exact kernel cut · %.0f ms%s", Section.Milliseconds, Section.Sheets ? Compose(" · %d sheet(s) skipped", Section.Sheets).c_str() : "");
    }
    if (PressButton(State, "Close (Alt+S)###SectionClose")) ToggleSection(State);
    EndCard();
}

// 📝 The exact section outline (kernel polylines) over the viewport while the eye is on the removed side, where the cap
//    is visible; drawn only when the figures describe the plane in force (no stale outline during a recomputation).
void DrawSectionOutline(DraftingWorkspaceState& State, ImDrawList* Canvas, const DraftingCameraFrame& Frame)
{
    const DraftingSectionSession& Session = State.Section;
    if (!State.Style.SectionEnabled || !Session.Section.Refusal.empty() || Session.Section.RequestKey != DraftingSectionKey(Session.Request)
        || Session.Section.Revision != State.Document.Revision) return;
    const std::array<double, 4>& Plane = State.Style.SectionPlane;
    if (Plane[0] * Frame.Eye[0] + Plane[1] * Frame.Eye[1] + Plane[2] * Frame.Eye[2] + Plane[3] >= 0.0) return;
    for (const std::vector<Vector3>& Line : Session.Section.Outline)
    {
        std::vector<ImVec2> Pixels;
        for (const Vector3& Point : Line) { ImVec2 Pixel; if (ProjectWorldPoint(State, Frame, Point, Pixel)) Pixels.push_back(Pixel); }
        if (Pixels.size() >= 2) Canvas->AddPolyline(Pixels.data(), int(Pixels.size()), IM_COL32(0x2a, 0x10, 0x0c, 0xff), 2.0f);
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
        const std::optional<DraftingPick> Nearby = PickDraftingScene(State.Scene, Ray, 8.0 * Frame.WorldPerPixel, State.Style.XRayEnabled, ActiveSectionClip(State));
        Identity = Nearby && (Nearby->Identity & IdentityEdgeBit) ? Nearby->Identity : 0;
    }
    if (State.PickMode == DraftingPickMode::Face && (Identity & IdentityEdgeBit))
    {
        const std::optional<DraftingPick> Behind = PickDraftingScene(State.Scene, Ray, 0.0, false, ActiveSectionClip(State));
        Identity = Behind ? Behind->Identity : 0;
    }
    if (Identity == IdentitySectionCap) Identity = 0;                                        // The section cap is no entity
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
void SelectInBox(DraftingWorkspaceState& State, const DraftingCameraFrame& Frame, ImVec2 Corner, ImVec2 Opposite, DraftingPickIntent Intent)
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
    // B-rep edges (modes 2 / 5): wholly inside the box; hidden ones only in x-ray.
    State.Box.Edges = 0;
    if (State.PickMode != DraftingPickMode::ControlPoint)
    {
        const std::array<double, 2> LocalLow{ Low.x - State.ViewportRect[0], Low.y - State.ViewportRect[1] };
        const std::array<double, 2> LocalHigh{ High.x - State.ViewportRect[0], High.y - State.ViewportRect[1] };
        for (const uint32_t Index : EncloseDraftingEdges(State.Scene, Frame, LocalLow, LocalHigh, State.Style.XRayEnabled, ActiveSectionClip(State)))
        {
            const std::string& Name = State.Scene.Edges[Index].Name;
            const std::vector<std::string>& Edges = State.Document.Selection.Edges;
            const bool Member = std::find(Edges.begin(), Edges.end(), Name) != Edges.end();
            if (Intent == DraftingPickIntent::Remove ? Member : !Member) SelectDraftingName(State.Document, DraftingSelectionKind::Edge, Name, true);
            ++State.Box.Edges;
        }
    }
    ++State.Box.Commits;
    const char* Verb = Intent == DraftingPickIntent::Remove ? "removed" : "selected";
    if (State.Box.Edges > 0)
        Announce(State, Compose("Box %s %d curve(s) + %d edge(s)%s · %d edge(s) selected", Verb, Count, State.Box.Edges, State.Style.XRayEnabled ? " (x-ray)" : "",
                                int(State.Document.Selection.Edges.size())));
    else
        Announce(State, Compose("Box %s %d curve(s) · %d selected", Verb, Count, int(State.Document.Selection.Curves.size())));
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
    if (ToggleButton(State, "X-ray###XRayToggle", State.Style.XRayEnabled)) ToggleXRay(State);
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
    if (State.PickMode == DraftingPickMode::ControlPoint && !State.Drawing.ActiveCondition && !State.Bevel.ActiveCondition && !State.Constrain.ActiveCondition)
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
            if (Moved)
            {
                // Object snaps (points, curves, edges; never the dragged curve) place the pole exactly
                const DraftingSnap Object = FindPointerSnap(State, Frame, Local, std::nullopt, {}, { Drag.Label }, false);
                State.Picker.Current = Object;
                if (Object.Kind != DraftingSnapKind::None) Drag.Position = Object.Position;
            }
            if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) || !ImGui::IsMouseDown(ImGuiMouseButton_Left))
            {
                Drag.ActiveCondition = false;
                if (Moved && State.Picker.Current.Kind != DraftingSnapKind::None) ++State.Picker.Snapped;
                State.Picker.Current = {};
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
        if (!Drawing.PointerCondition) State.Picker.Current = {};
        if (Drawing.PointerCondition)
        {
            PlaceDrawingPointer(State, Frame, Local, Point);
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
    else if (State.Transform.ActiveCondition)
    {
        DriveTransformSession(State, Frame, Local);
        if (LeftClick && !OverCube) ApplyTransformSession(State);
        else if (RightClick && !OverCube) CancelTransformSession(State);
    }
    else if (State.Sweep.ActiveCondition)
    {
        DriveSweepSession(State, Frame, Hovered);
        if (LeftClick && !OverCube) ClickSweepSession(State);
        else if (RightClick && !OverCube) CancelSweepSession(State);
    }
    else if (State.Constrain.ActiveCondition)
    {
        DriveConstraintSession(State, Frame, Local, Hovered && !OverCube, LeftClick && !OverCube, RightClick && !OverCube);
    }
    else if (State.Trim.ActiveCondition)
    {
        HoverTrimPiece(State, Frame, Local, Hovered && !OverCube);
        if (LeftClick && !OverCube) ApplyTrim(State);
        if (RightClick && !OverCube) CloseTrimSession(State);
    }
    else if (State.Joint.ActiveCondition)
    {
        HoverJointOrigin(State, Frame, Local, Hovered && !OverCube);
        if (LeftClick && !OverCube) ClickJointSession(State);
        if (RightClick && !OverCube) CloseJointSession(State, "Joint cancelled");
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
                if (Moved) SelectInBox(State, Frame, Box.Start, Box.End, PickIntentOf(Io));
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
    DrawTrimPreview(State, Canvas, Frame);
    DrawConstraintOverlay(State, Canvas, Frame);
    DrawSweepOutline(State, Canvas, Frame);
    DrawSectionOutline(State, Canvas, Frame);
    DrawMeasureOverlay(State, Canvas, Frame);
    DrawJointOverlay(State, Canvas, Frame);
    DrawCurveOverlays(State, Canvas, Frame);
    if (State.DeviationEnabled) DrawDeviationOverlay(State, Canvas, Frame);
    if (State.Drawing.ActiveCondition) DrawDrawingPreview(State, Canvas, Frame);
    DrawSnapOverlay(State, Canvas, Frame);
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
    if (State.Style.XRayEnabled) DrawStatusPill(Canvas, Pill, "X-ray", IM_COL32(0xc7, 0xd4, 0xe8, 0xff));
    if (State.Measure.ActiveCondition) DrawStatusPill(Canvas, Pill, "Measure", IM_COL32(0xff, 0xc8, 0x3d, 0xff));
    if (State.Section.ActiveCondition) DrawStatusPill(Canvas, Pill, DescribeSectionRequest(State.Section.Request), IM_COL32(0xdb, 0x61, 0x4a, 0xff));
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
        DrawStatusPill(Canvas, Pill, State.HoverIdentity == IdentitySectionCap ? std::string("Section cap") : NameDraftingIdentity(DisplayedDraftingScene(State), State.HoverIdentity),
                       IM_COL32(0xff, 0xd2, 0x4d, 0xff));
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
    if (State.Trim.ActiveCondition)
    {
        ImVec2 Below(Image.x + 12, Image.y + 44);
        const DraftingTrimSession& Trim = State.Trim;
        if (!Trim.Refusal.empty()) DrawStatusPill(Canvas, Below, Trim.Refusal, RefusalColour);
        else DrawStatusPill(Canvas, Below, Compose("Trim · %d trimmed%s", Trim.Commits, Trim.HoverCondition ? " · click removes the red piece" : ""), ActiveColour);
    }
    if (State.Constrain.ActiveCondition)
    {
        ImVec2 Below(Image.x + 12, Image.y + 44);
        bool Trouble = false;
        const std::string Readout = ConstraintSessionReadout(State, Trouble);
        DrawStatusPill(Canvas, Below, Readout, Trouble ? RefusalColour : ActiveColour);
        if (!State.Constrain.Refusal.empty())
        {
            ImVec2 Next(Image.x + 12, Image.y + 76);
            DrawStatusPill(Canvas, Next, State.Constrain.Refusal, RefusalColour);
        }
    }
    if (State.Transform.ActiveCondition)
    {
        ImVec2 Below(Image.x + 12, Image.y + 44);
        const DraftingTransformSession& Transform = State.Transform;
        DrawStatusPill(Canvas, Below, Compose("%s%s · %d body(ies)", Transform.DuplicateCondition ? "Duplicate · " : "", Transform.Readout.c_str(), int(Transform.Keys.size())), Transform.Refusal.empty() ? ActiveColour : RefusalColour);
        if (!Transform.Refusal.empty()) DrawStatusPill(Canvas, Below, Transform.Refusal, RefusalColour);
    }
    if (State.Sweep.ActiveCondition)
    {
        ImVec2 Below(Image.x + 12, Image.y + 44);
        const DraftingSweepSession& Sweep = State.Sweep;
        const std::string Boolean = Sweep.Verb != DraftingSweepVerb::Offset ? std::string(" · ") + (Sweep.Combination == FeatureCombination::New ? "New" : FeatureCombinationLabel(Sweep.Combination)) : std::string{};
        const std::string Typed   = Sweep.Typed.empty() ? std::string{} : " · typed " + Sweep.Typed;
        if (Sweep.Verb == DraftingSweepVerb::Revolve)
            DrawStatusPill(Canvas, Below, Compose("Revolve %s %s° about %s%s%s", CaptionOf(State.Document, Sweep.Path).c_str(), FormatLength(Sweep.Amount).c_str(),
                                                  DescribeDraftingRevolveAxis(State.Document, Sweep.Path, Sweep.Axis).c_str(), Boolean.c_str(), Typed.c_str()), Sweep.Refusal.empty() ? ActiveColour : RefusalColour);
        else if (Sweep.Verb == DraftingSweepVerb::Evolution)
            DrawStatusPill(Canvas, Below, Compose("Evolution %s along %s%s", CaptionOf(State.Document, Sweep.Profile).c_str(), Sweep.Path.empty() ? "?" : CaptionOf(State.Document, Sweep.Path).c_str(), Boolean.c_str()),
                           Sweep.Refusal.empty() ? ActiveColour : RefusalColour);
        else
            DrawStatusPill(Canvas, Below, Compose("%s %s %s m%s%s", SweepVerbName(Sweep.Verb), CaptionOf(State.Document, Sweep.Path).c_str(), FormatLength(Sweep.Amount).c_str(), Boolean.c_str(), Typed.c_str()),
                           Sweep.Refusal.empty() ? ActiveColour : RefusalColour);
        if (Sweep.HoverCurve >= 0)
        {
            const SketchModel* Sketch = SweepSketch(State, Sweep.HoverSketch);
            if (Sketch && Sweep.HoverCurve < int(Sketch->Curves.size()))
                DrawStatusPill(Canvas, Below, Sweep.Verb == DraftingSweepVerb::Revolve ? "click: axis = " + SketchCurveName(Sketch->Curves[Sweep.HoverCurve]) : "click: path = " + CaptionOf(State.Document, Sweep.HoverSketch), ActiveColour);
        }
        if (!Sweep.Refusal.empty()) DrawStatusPill(Canvas, Below, Sweep.Refusal, RefusalColour);
    }
    const char* Hint = State.Transform.ActiveCondition
                           ? (State.Transform.Verb == DraftingTransformVerb::Move ? "Move: pointer snaps to points / edges / faces (Ctrl free) · X / Y / Z axis · Shift+X / Y / Z plane · G screen · type a distance · click / Enter commit · Esc / right-click cancel"
                              : State.Transform.Verb == DraftingTransformVerb::Rotate ? "Rotate: sweep about the centre (5° steps, Ctrl free) · X / Y / Z axis · R screen · type degrees · click / Enter commit · Esc / right-click cancel"
                              : "Scale: distance to the centre (×0.1 steps, Ctrl free) · X / Y / Z one axis · S uniform · type a factor · click / Enter commit · Esc / right-click cancel")
                       : State.Sweep.ActiveCondition
                           ? (State.Sweep.Verb == DraftingSweepVerb::Offset ? "Offset: move sideways (Shift fine, Ctrl free; negative = inward / right) · type a distance · click / Enter commit · Esc / right-click cancel"
                              : State.Sweep.Verb == DraftingSweepVerb::Revolve ? "Revolve: type degrees (360) · X / Y sketch axis · click a profile line for the axis · Q union · W difference · E intersect · R new body · click elsewhere / Enter commit · Esc cancel"
                              : State.Sweep.Verb == DraftingSweepVerb::Evolution ? "Evolution: click another sketch's curve for the path (or select a curve first) · Q union · W difference · E intersect · R new body · click elsewhere / Enter commit · Esc cancel"
                              : "Pipe: move sideways for the radius (Shift fine, Ctrl free) · type a radius · Q union · W difference · E intersect · R new body · click / Enter commit · Esc cancel")
                       : State.Drawing.ActiveCondition
                           ? (State.Drawing.Kind == DraftingDrawingKind::Curve ? "Click points (snaps, Ctrl free) · x y z t n b lock · 1 Hermite · 2 Bezier · 3 NURBS · 4 cubic · click first point closes · Enter / right-click finish · Ctrl+Z drop · Esc abort"
                              : State.Drawing.Kind == DraftingDrawingKind::Line ? "Click points (snaps, Ctrl free) · x y z t n b lock · click first point closes · Enter / right-click finish · Ctrl+Z drop · Esc abort"
                              : State.Drawing.Kind == DraftingDrawingKind::Rectangle ? "Click two corners (snaps, Ctrl free) · x y z t n b lock · s square · Alt centre / corner · Ctrl+Z drop · Esc abort"
                              : State.Drawing.Kind == DraftingDrawingKind::Circle ? "Click centre then rim (snaps, Ctrl free) · x y z t n b lock · V two-point / centre · Ctrl+Z drop · Esc abort"
                              : State.Drawing.Kind == DraftingDrawingKind::Arc ? "Click start, end, then a point on the arc (snaps, Ctrl free) · x y z t n b lock · Ctrl+Z drop · Esc abort"
                              : "Click both cap centres, then the rim (snaps, Ctrl free) · x y z t n b lock · Ctrl+Z drop · Esc abort")
                       : State.Constrain.ActiveCondition
                           ? "Constrain: click points / curves · D dimension · N angle · H / V · O on · Q equal · M midpoint · Y symmetric · L parallel / tangent · [ perpendicular · ] lock · drag a point · click a glyph: Delete removes, digits + Enter edit · Esc / K end"
                       : State.Trim.ActiveCondition
                           ? "Trim: hover a curve (the red piece comes off between its crossings) · click removes it · Esc / T / right-click end"
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
        // Staged from the document's checkpoints: only the new blend replays ("replayed 1/9").
        const int PreviewSteps = static_cast<int>(Bevel.Preview->Tree.Steps.size());
        ImGui::Text("Preview: %d faces · %.4f m³ · replayed %d/%d", Bevel.Preview->Summary.Faces, Bevel.Preview->Summary.Volume,
                    PreviewSteps - std::min(PreviewSteps, Bevel.Preview->Regeneration.Resumed), PreviewSteps);
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
    if (Step->Kind == FeatureStepKind::Transform || Step->Kind == FeatureStepKind::Duplicate)
    {
        const AffineMap& Map = Step->Placement;
        const Vector3 Factors{ Length(Map.ColumnX), Length(Map.ColumnY), Length(Map.ColumnZ) };
        const double Trace = Map.ColumnX.X / Factors.X + Map.ColumnY.Y / Factors.Y + Map.ColumnZ.Z / Factors.Z;
        const double Turn  = std::acos(std::clamp((Trace - 1.0) * 0.5, -1.0, 1.0)) * 180.0 / std::numbers::pi;   // [°] - Rotation part (uniform scale)
        const std::string Subject = DraftingBodyCaption(State.Document, Step->Target.empty() ? std::string("the active body") : Step->Target);
        ImGui::TextWrapped("%s %s · shift (%.3f, %.3f, %.3f) · turn %.2f° · scale (%.3f, %.3f, %.3f)%s", Step->Kind == FeatureStepKind::Transform ? "Moves" : "Copies", Subject.c_str(),
                           Map.Shift.X, Map.Shift.Y, Map.Shift.Z, Turn, Factors.X, Factors.Y, Factors.Z,
                           Step->Source.empty() ? "" : (" · as it stood after " + CaptionOf(State.Document, Step->Source)).c_str());
    }
    if (Step->Kind == FeatureStepKind::Revolve && !Step->Profile.empty() && CaptionOf(State.Document, Label).rfind("Revolve", 0) == 0)   // Alt+R revolves (not the sphere / torus tiles)
    {
        const DraftingRevolveAxis Axis{ Step->AxisIdentifier, Step->AxisOrigin, Step->AxisDirection };
        ImGui::TextWrapped("Turns %s %.2f° about %s", CaptionOf(State.Document, Step->Profile).c_str(), EvaluateFeatureValue(State.Document.Tree, Step->Angle) * 180.0 / std::numbers::pi,
                           DescribeDraftingRevolveAxis(State.Document, Step->Profile, Axis).c_str());
    }
    if (Step->Kind == FeatureStepKind::Evolution)
        ImGui::TextWrapped("Sweeps %s along %s (rotation-minimizing frames)", CaptionOf(State.Document, Step->Profile).c_str(), CaptionOf(State.Document, Step->Path).c_str());
    if (Step->Kind == FeatureStepKind::Joint)
        ImGui::TextWrapped("%s joint: %s onto %s · %d degree(s) of freedom%s · drag a value (Ctrl+click types it)", JointKindLabel(Step->Joint.Kind),
                           DraftingBodyCaption(State.Document, Step->Target).c_str(), DraftingBodyCaption(State.Document, Step->Tool).c_str(), JointFreedom(Step->Joint.Kind),
                           Step->Joint.FlippedCondition ? " · flipped" : "");
    if (Step->Kind == FeatureStepKind::Pipe)
    {
        const std::string Path = Step->Sections.empty() ? CaptionOf(State.Document, Step->Profile) : CaptionOf(State.Document, Step->Sections.front());
        ImGui::TextWrapped("Tube of radius %.4f along %s", EvaluateFeatureValue(State.Document.Tree, Step->Size), Path.c_str());
    }
    const auto Caption = State.Document.Captions.find(Label);
    const std::string Prefix = Caption == State.Document.Captions.end() ? Label + "." : Caption->second + ".";
    std::vector<FeatureParameter> Parameters;
    for (const FeatureParameter& Parameter : State.Document.Tree.Parameters)
        if (Parameter.Name.rfind(Prefix, 0) == 0) Parameters.push_back(Parameter);
    for (FeatureParameter& Parameter : Parameters)
    {
        const std::string Field = Parameter.Name.substr(Prefix.size()) + "###Parameter:" + Parameter.Name;
        if (Step->Kind == FeatureStepKind::Joint)
        {
            // Dragged: a staged document shows the motion (replayed from the joint); the document takes the value on release.
            DraftingParameterDrag& Drag    = State.ParameterDrag;
            const bool             Mine    = Drag.Parameter == Parameter.Name;
            const std::string      Suffix  = Parameter.Name.substr(Prefix.size());
            const bool             Angular = Suffix == "Angle" || Suffix == "Pitch" || Suffix == "Roll";
            double Dragged = Mine ? Drag.Value : Parameter.Value;
            ImGui::SetNextItemWidth(150);
            const bool Edited = ImGui::DragScalar(Field.c_str(), ImGuiDataType_Double, &Dragged, Angular ? 0.5f : 0.005f, nullptr, nullptr, Angular ? "%.2f deg" : "%.4f m");
            RegisterWidget(State, Field.c_str());
            if (Edited && Dragged != (Mine ? Drag.Value : Parameter.Value))
            {
                Drag.Parameter = Parameter.Name;
                Drag.Value     = Dragged;
                Drag.Preview   = std::make_unique<DraftingDocument>(CloneDraftingDocument(State.Document));
                const DraftingCommandOutcome Staged = AssignDraftingParameter(*Drag.Preview, Parameter.Name, Dragged);
                if (Staged.AcceptedCondition) { Drag.PreviewScene = BuildDraftingScene(*Drag.Preview); ++Drag.Stagings; }
                else { Drag.Preview.reset(); Announce(State, Staged.Refusal); }
            }
            if (ImGui::IsItemDeactivated() && Drag.Parameter == Parameter.Name)
            {
                const double Final = Drag.Value;
                Drag.Parameter.clear();
                Drag.Preview.reset();
                if (Final != Parameter.Value) { Conclude(State, AssignDraftingParameter(State.Document, Parameter.Name, Final)); break; }
            }
            continue;
        }
        double Value = Parameter.Value;
        ImGui::SetNextItemWidth(150);
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
        // Incremental replay: "replayed k/n" = the last k steps ran, the earlier ones were restored from their checkpoints.
        const int Steps    = static_cast<int>(State.Document.Tree.Steps.size());
        const int Replayed = Steps - std::min(Steps, State.Document.Regeneration.Resumed);
        ImGui::TextDisabled("· replayed %d/%d · %.1f ms · %d tris", Replayed, Steps, Summary.RegenerationMs, int(State.Scene.Corners.size() / 3));
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

// 📝 Object snaps while drawing or moving: master switch, per-kind filter, the active direction lock.
void DrawSnapCard(DraftingWorkspaceState& State)
{
    const bool Moving   = State.Transform.ActiveCondition && State.Transform.Verb == DraftingTransformVerb::Move;
    // 📝 Outside drawing and Move the card only joins while a sketch point (K) or a pole (mode 1) is actually held: the
    //    constraint rows and panel buttons below it must stay where the pointer expects them otherwise.
    const bool Dragging = (State.Constrain.ActiveCondition && State.Constrain.DragCondition) || State.PoleDrag.ActiveCondition;
    if (!State.Drawing.ActiveCondition && !Moving && !Dragging) return;
    DraftingPointPicker& Picker = State.Picker;
    BeginCard("Snaps");
    if (ToggleButton(State, "Object snaps###SnapEnabled", Picker.Enabled, ImVec2(140, 0))) Picker.Enabled = !Picker.Enabled;
    ImGui::SameLine();
    ImGui::Text("Lock %s", Picker.Lock == DraftingSnapLockKind::None ? "none" : DraftingSnapLockLabel(Picker.Lock));
    struct Choice { const char* Label; bool* Enabled; };
    DraftingSnapFilter& Filter = Picker.Filter;
    const Choice Choices[] = { { "Endpoint###SnapEndpoint", &Filter.EndpointEnabled }, { "Midpoint###SnapMidpoint", &Filter.MidpointEnabled },
                               { "Centre###SnapCentre", &Filter.CentreEnabled }, { "Intersection###SnapIntersection", &Filter.IntersectionEnabled },
                               { "Tangent###SnapTangent", &Filter.TangentEnabled }, { "Perpendicular###SnapPerpendicular", &Filter.PerpendicularEnabled },
                               { "On curve###SnapCurve", &Filter.CurveEnabled }, { "On face (Move)###SnapFace", &Filter.FaceEnabled } };
    ImGui::BeginDisabled(!Picker.Enabled);
    for (int Index = 0; Index < 8; ++Index)
    {
        if (Index % 2 == 1) ImGui::SameLine();
        if (ToggleButton(State, Choices[Index].Label, *Choices[Index].Enabled, ImVec2(140, 0))) *Choices[Index].Enabled = !*Choices[Index].Enabled;
    }
    ImGui::EndDisabled();
    ImGui::PushStyleColor(ImGuiCol_Text, Tint(MutedColour));
    ImGui::TextWrapped(State.Drawing.ActiveCondition ? "x y z t n b lock a direction through the last point (again releases) · s squares a rectangle · Ctrl held: free"
                       : Moving ? "The grab point snaps, then lands on the snap under the pointer · X / Y / Z still constrain · Ctrl held: free"
                       : State.Constrain.ActiveCondition ? "A dragged sketch point lands on points and in-plane curves of other geometry, else the grid · Ctrl held: free"
                                                         : "A dragged pole lands on points, curves and edges of other geometry · Ctrl held: free");
    ImGui::PopStyleColor();
    EndCard();
}

void DrawInspector(DraftingWorkspaceState& State, ImVec2 Position, ImVec2 Size)
{
    BeginPanel("Inspector", Position, Size);
    PanelHeading("Inspector");
    if (State.Boolean.ActiveCondition) DrawBooleanCard(State);
    else if (State.Joint.ActiveCondition) DrawJointCard(State);
    else if (State.Bevel.ActiveCondition) DrawBevelCard(State);
    else if (!State.Drawing.ActiveCondition) DrawSelectionCard(State);
    DrawMeasureCard(State);
    DrawSectionCard(State);
    DrawDrawingCard(State);
    DrawSnapCard(State);
    DrawConstraintCard(State);
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
        else if (Alternate && !Control && !Io.KeyShift && Pressed(ImGuiKey_Z)) ToggleXRay(State);       // viewport:toggle-x-ray stays live (hidden snaps)
        else if (!Control && !Alternate && !Io.KeyShift && (Pressed(ImGuiKey_X) || Pressed(ImGuiKey_Y) || Pressed(ImGuiKey_Z) || Pressed(ImGuiKey_T) || Pressed(ImGuiKey_N) || Pressed(ImGuiKey_B)))
        {
            // body[gizmo=point-picker]: x / y / z, set-tangent, set-normal, set-binormal
            ChooseSnapLock(State, Pressed(ImGuiKey_X) ? DraftingSnapLockKind::X : Pressed(ImGuiKey_Y) ? DraftingSnapLockKind::Y : Pressed(ImGuiKey_Z) ? DraftingSnapLockKind::Z
                                : Pressed(ImGuiKey_T) ? DraftingSnapLockKind::Tangent : Pressed(ImGuiKey_N) ? DraftingSnapLockKind::Normal : DraftingSnapLockKind::Binormal);
        }
        else if (Drawing.Kind == DraftingDrawingKind::Rectangle && !Control && !Alternate && Pressed(ImGuiKey_S))
        {
            Drawing.SquareCondition = !Drawing.SquareCondition;                               // body[gizmo=point-picker]: set-square
            Announce(State, Drawing.SquareCondition ? "Rectangle: square (s releases)" : "Rectangle: free sides");
        }
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

    // Joint (Alt+J) — R V S C P L B kind · F flip · Enter applies · Esc cancels (letters free inside the command).
    if (State.Joint.ActiveCondition)
    {
        if (Pressed(ImGuiKey_Enter) || Pressed(ImGuiKey_KeypadEnter)) ApplyJointSession(State);
        else if (Pressed(ImGuiKey_Escape)) CloseJointSession(State, "Joint cancelled");
        else if (!Control && !Alternate)
        {
            if (Pressed(ImGuiKey_R)) ChooseJointKind(State, JointKind::Rigid);
            if (Pressed(ImGuiKey_V)) ChooseJointKind(State, JointKind::Revolute);
            if (Pressed(ImGuiKey_S)) ChooseJointKind(State, JointKind::Slider);
            if (Pressed(ImGuiKey_C)) ChooseJointKind(State, JointKind::Cylindrical);
            if (Pressed(ImGuiKey_P)) ChooseJointKind(State, JointKind::PinSlot);
            if (Pressed(ImGuiKey_L)) ChooseJointKind(State, JointKind::Planar);
            if (Pressed(ImGuiKey_B)) ChooseJointKind(State, JointKind::Ball);
            if (Pressed(ImGuiKey_F)) FlipJoint(State);
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

    // [command='move' | 'rotate' | 'scale' | 'duplicate'] — X / Y / Z axis, Shift+X / Y / Z plane (move), the verb's own key
    // returns to screen / uniform, digits type the amount, Enter commits, Esc cancels.
    if (State.Transform.ActiveCondition)
    {
        DraftingTransformSession& Transform = State.Transform;
        if (CollectTypedAmount(Transform.Typed)) Transform.StagedCondition = false;
        if (Pressed(ImGuiKey_Enter) || Pressed(ImGuiKey_KeypadEnter)) ApplyTransformSession(State);
        else if (Pressed(ImGuiKey_Escape)) CancelTransformSession(State);
        else if (!Control && !Alternate)
        {
            static const ImGuiKey AxisKeys[] = { ImGuiKey_X, ImGuiKey_Y, ImGuiKey_Z };
            for (int Axis = 0; Axis < 3; ++Axis)
                if (Pressed(AxisKeys[Axis]))                                                  // gizmo:move:x … · shift-z → xy plane …
                {
                    Transform.Axis           = Axis;
                    Transform.PlaneCondition = Shift && Transform.Verb == DraftingTransformVerb::Move;
                }
            const ImGuiKey Own = Transform.Verb == DraftingTransformVerb::Move ? ImGuiKey_G : Transform.Verb == DraftingTransformVerb::Rotate ? ImGuiKey_R : ImGuiKey_S;
            if (Pressed(Own)) { Transform.Axis = -1; Transform.PlaneCondition = false; }       // gizmo:move:screen · rotate:screen · scale:xyz
        }
        return;
    }

    // [command='offset-curve' | 'pipe'] — digits type the amount, Q / W / E / R combine (pipe), Enter commits, Esc cancels.
    if (State.Sweep.ActiveCondition)
    {
        if (CollectTypedAmount(State.Sweep.Typed)) State.Sweep.StagedAmount = -1.0e300;
        if (Pressed(ImGuiKey_Enter) || Pressed(ImGuiKey_KeypadEnter)) ApplySweepSession(State);
        else if (Pressed(ImGuiKey_Escape)) CancelSweepSession(State);
        else if (!Control && !Alternate)
        {
            if (Pressed(ImGuiKey_Q)) ChooseSweepCombination(State, FeatureCombination::Union);        // keyboard:pipe:union
            if (Pressed(ImGuiKey_W)) ChooseSweepCombination(State, FeatureCombination::Difference);   // keyboard:pipe:difference
            if (Pressed(ImGuiKey_E)) ChooseSweepCombination(State, FeatureCombination::Intersection); // keyboard:pipe:intersect
            if (Pressed(ImGuiKey_R)) ChooseSweepCombination(State, FeatureCombination::Separate);     // keyboard:pipe:new-body
            if (Pressed(ImGuiKey_X)) ChooseRevolveAxis(State, { 1.0, 0.0 });                    // revolve: sketch x-axis
            if (Pressed(ImGuiKey_Y)) ChooseRevolveAxis(State, { 0.0, 1.0 });                    // revolve: sketch y-axis
            if (Pressed(ImGuiKey_A) && State.Sweep.Verb != DraftingSweepVerb::Offset && State.Sweep.Verb != DraftingSweepVerb::Pipe)
                Announce(State, "Angle: type degrees (gizmo:revolution:angle)");
        }
        return;
    }

    // Viewport navigation (any modifier state the keymap names).
    if (Pressed(ImGuiKey_Keypad1)) PresentView(State, Control ? DraftingViewPreset::Back : DraftingViewPreset::Front);
    if (Pressed(ImGuiKey_Keypad3)) PresentView(State, Control ? DraftingViewPreset::Left : DraftingViewPreset::Right);
    if (Pressed(ImGuiKey_Keypad7)) PresentView(State, Control ? DraftingViewPreset::Bottom : DraftingViewPreset::Top);
    if (Pressed(ImGuiKey_Keypad5)) State.Camera.OrthographicEnabled = !State.Camera.OrthographicEnabled;

    // [command='constrain'] — SolveSpace verbs on the picks, typed values, Delete removes a picked constraint (view keys live).
    if (State.Constrain.ActiveCondition)
    {
        RouteConstraintKeys(State, Pressed, Control, Shift, Alternate);
        return;
    }

    // body[command='trim'] — click pieces to remove · Esc / T ends (the view keys above stay live).
    if (State.Trim.ActiveCondition)
    {
        if (Pressed(ImGuiKey_Escape) || Pressed(ImGuiKey_T)) CloseTrimSession(State);
        return;
    }

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
        if (Pressed(ImGuiKey_C)) CopyBodies(State);                                         // edit:copy
        if (Pressed(ImGuiKey_V)) PasteBodies(State);                                        // edit:paste
        if (Pressed(ImGuiKey_H)) ChangeVisibility(State, DraftingVisibilityVerb::InvertHidden);   // invert-hidden
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
            else ToggleXRay(State);                                                     // viewport:toggle-x-ray
        }
        if (Pressed(ImGuiKey_E)) OpenSweepSession(State, DraftingSweepVerb::Evolution);     // command:evolution
        if (Pressed(ImGuiKey_S)) ToggleSection(State);                                      // section (no Plasticity default; Alt+S is free)
        if (Pressed(ImGuiKey_J)) OpenJointSession(State);                                   // joint (no Plasticity default; Alt+J is free)
        if (Pressed(ImGuiKey_R)) OpenSweepSession(State, DraftingSweepVerb::Revolve);       // revolution (no Plasticity default key)
        if (Pressed(ImGuiKey_X)) MirrorPanels(State);                                       // mirror (gizmo x: plane x = 0)
        if (Pressed(ImGuiKey_M)) RemoveMatcap(State);                                       // remove-material
        if (Pressed(ImGuiKey_H)) ChangeVisibility(State, DraftingVisibilityVerb::UnhideAll);       // unhide-all
        if (Pressed(ImGuiKey_G) || Pressed(ImGuiKey_Q)) Announce(State, "Not available in this modeller");
        return;
    }
    if (Shift)
    {
        if (Pressed(ImGuiKey_C) || Pressed(ImGuiKey_V)) OpenConstruct(State, DraftingConstructKind::Block);     // corner / centre box
        if (Pressed(ImGuiKey_X)) OpenConstruct(State, DraftingConstructKind::Cylinder);
        if (Pressed(ImGuiKey_Z)) OpenConstruct(State, DraftingConstructKind::Sphere);
        if (Pressed(ImGuiKey_R)) RepeatLastCommand(State);
        if (Pressed(ImGuiKey_D)) OpenTransformSession(State, DraftingTransformVerb::Move, true);        // duplicate (then move)
        if (Pressed(ImGuiKey_A)) OpenDrawingSession(State, DraftingDrawingKind::Line);                  // line
        if (Pressed(ImGuiKey_S)) OpenDrawingSession(State, DraftingDrawingKind::Curve);                 // curve
        if (Pressed(ImGuiKey_Q)) OpenDrawingSession(State, DraftingDrawingKind::Rectangle);             // corner-rectangle
        if (Pressed(ImGuiKey_W)) OpenDrawingSession(State, DraftingDrawingKind::Circle);                // center-circle
        if (Pressed(ImGuiKey_H)) ChangeVisibility(State, DraftingVisibilityVerb::HideUnselected);       // hide-unselected
        if (Pressed(ImGuiKey_P)) OpenSweepSession(State, DraftingSweepVerb::Evolution);     // command:evolution
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
    if (Pressed(ImGuiKey_G)) OpenTransformSession(State, DraftingTransformVerb::Move, false);       // command:move
    if (Pressed(ImGuiKey_R)) OpenTransformSession(State, DraftingTransformVerb::Rotate, false);     // command:rotate
    if (Pressed(ImGuiKey_S)) OpenTransformSession(State, DraftingTransformVerb::Scale, false);      // command:scale
    if (Pressed(ImGuiKey_H)) ChangeVisibility(State, DraftingVisibilityVerb::HideSelected);         // hide-selected
    if (Pressed(ImGuiKey_C)) ApplyCut(State);                                              // command:cut
    if (Pressed(ImGuiKey_T)) OpenTrimSession(State);                                       // command:trim
    if (Pressed(ImGuiKey_K)) OpenConstraintSession(State);                                 // constrain (K is free in Plasticity's map)
    if (Pressed(ImGuiKey_I)) ToggleMeasure(State);                                         // measure (I is free in Plasticity's map)
    if (Pressed(ImGuiKey_J)) JoinSketches(State);                                          // command:join-curves
    if (Pressed(ImGuiKey_O)) OpenSweepSession(State, DraftingSweepVerb::Offset);           // command:offset-curve
    if (Pressed(ImGuiKey_P)) OpenSweepSession(State, DraftingSweepVerb::Pipe);             // command:pipe
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
    RefreshMeasure(State);
    RefreshSection(State);
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
    if (State.Transform.ActiveCondition && State.Transform.Preview && State.Transform.Refusal.empty()) return State.Transform.PreviewScene;
    if (State.Sweep.ActiveCondition && State.Sweep.Preview && State.Sweep.Refusal.empty()) return State.Sweep.PreviewScene;
    if (State.Joint.ActiveCondition && State.Joint.Preview && State.Joint.Refusal.empty()) return State.Joint.PreviewScene;
    if (State.ParameterDrag.Preview) return State.ParameterDrag.PreviewScene;
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
                               : &Scene == &State.Transform.PreviewScene ? ComposeDraftingStates(Scene, State.Transform.Preview->Selection, 0)
                               : &Scene == &State.Sweep.PreviewScene ? ComposeDraftingStates(Scene, State.Sweep.Preview->Selection, 0)
                               : &Scene == &State.Joint.PreviewScene ? ComposeDraftingStates(Scene, State.Joint.Preview->Selection, 0)
                               : &Scene == &State.ParameterDrag.PreviewScene ? ComposeDraftingStates(Scene, State.ParameterDrag.Preview->Selection, 0)
                                                                      : ComposeDraftingStates(Scene, State.Document.Selection, State.ViewportHovered ? State.HoverIdentity : 0);
    // 📝 Matcap layers ride in bits 8 … 15 of each face word (DraftingSurface.slang).
    const std::vector<uint32_t> Layers = ComposeDraftingMatcapLayers(Scene, State.Document.Appearance, State.Matcaps.Library);
    for (size_t Index = 0; Index < States.Faces.size() && Index < Layers.size(); ++Index) States.Faces[Index] |= Layers[Index];
    // 📝 Sheet faces carry StateSheetFace (bit 16) so a section never caps an open panel.
    if (State.Style.SectionEnabled)
        for (size_t Index = 0; Index < States.Faces.size() && Index < State.Section.Clip.SheetFaces.size(); ++Index)
            if (State.Section.Clip.SheetFaces[Index]) States.Faces[Index] |= StateSheetFace;
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

std::string DraftingConstraintSketch(const DraftingWorkspaceState& State)
{
    return ConstraintSketchLabel(State);
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
