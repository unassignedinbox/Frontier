/*====================================================================================================================================
                                                        SOLIDARCINTERFACE.CPP
====================================================================================================================================*/
// 🧩 SolidArc ImGui shell: header, Outliner, viewport, Inspector, bevel session, shortcuts, autosave / recovery

//------------------------------------------------------------------------------------------------------------------------
//                                                    INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "UI/SolidArc/Interface/SolidArcInterface.h"
#include "UI/SolidArc/Interface/SolidArcFileDialog.h"
#include "UI/SolidArc/Interface/SolidArcNavigation.h"
#include "UI/SolidArc/Interface/SolidArcOutliner.h"
#include "UI/SolidArc/Interface/SolidArcStyleSheet.h"

#include "Engine/GeometryProcessing/Diagnostics/OperationTrace.h"
#include "Engine/GeometryProcessing/Exchange/ExchangePath.h"
#include "Engine/Interface/ContentDirectory.h"
#include "Engine/Interface/ControlKit.h"
#include "Engine/Interface/IconPresentation.h"

#include <imgui_internal.h>

#include "UI/SolidArc/Document/SolidArcDocumentCodec.h"
#include "UI/SolidArc/Document/SolidArcSpecimen.h"
#include "UI/SolidArc/Document/VehicleCurveImport.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <cfloat>
#include <chrono>
#include <filesystem>
#include <functional>
#include <limits>
#include <numbers>
#include <numeric>
#include <set>
#include <sstream>
#include <utility>

namespace Frontier::SolidArc
{

using namespace Frontier::GeometryProcessing;

namespace
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    CONSTANTS
//------------------------------------------------------------------------------------------------------------------------

// 📝 SolidArc WorkspacePanel.css: header + document bar, Outliner 300 px | viewport | Inspector 330 px, 10 px gaps.
constexpr float  ZebraStripeCount = 64.0f;                                                  // [-]   - Zebra stripes over the reflection angle (V8)
constexpr double DeviationLimit   = 2.0e-4;                                                 // [len] - Red end of the deviation scale (fit tolerance)
constexpr float HeaderHeight    = 68.0f;                                                     // [px] - HTML .app-header
constexpr float DocumentHeight  = 34.0f;                                                     // [px] - HTML .document-bar
constexpr float StatusHeight    = 31.0f;                                                     // [px] - HTML .status-bar
constexpr float FrameMargin     = 12.0f;                                                     // [px] - HTML .workspace padding 0 12
constexpr float ColumnGap       = 10.0f;                                                     // [px] - HTML .workspace gap
constexpr float ActionBarHeight = 47.0f;                                                     // [px] - HTML .action-bar
constexpr float HistoryHeight   = 85.0f;                                                     // [px] - HTML .history-strip
constexpr float OutlinerWidth   = 300.0f;                                                    // [px] - Left column
constexpr float InspectorWidth  = 330.0f;                                                    // [px] - Right column
constexpr float ViewportBar     = 44.0f;                                                     // [px] - Viewport toolbar
constexpr float PanelRounding   = 16.0f;                                                     // [px] - Panel corner radius
constexpr float CubeExtent      = 92.0f;                                                     // [px] - Orientation cube box
constexpr float ClickTolerance  = 4.0f;                                                      // [px] - Press → release travel still counted as a click

constexpr ImU32 CardColour      = IM_COL32(0x22, 0x22, 0x22, 0xff);                          // #222
constexpr ImU32 RaisedColour    = IM_COL32(0x29, 0x29, 0x29, 0xff);                          // #292929
constexpr ImU32 SelectedColour  = IM_COL32(0x40, 0x99, 0xff, 0xff);                          // #4099ff
constexpr ImU32 ActiveColour    = IM_COL32(0x33, 0xff, 0x73, 0xff);                          // #33ff73
constexpr ImU32 MutedColour     = IM_COL32(0xaa, 0xaa, 0xaa, 0xff);                          // #aaa
constexpr ImU32 RefusalColour   = IM_COL32(0xff, 0x6b, 0x6b, 0xff);                          // Refused step / reason
constexpr ImU32 AxisColours[3]  = { IM_COL32(0xe0, 0x4b, 0x4b, 0xff), IM_COL32(0x4b, 0xc8, 0x5a, 0xff), IM_COL32(0x4b, 0x7c, 0xe8, 0xff) };

constexpr FaceOperationKind FaceVerbs[] = { FaceOperationKind::Push, FaceOperationKind::Offset, FaceOperationKind::Extrude, FaceOperationKind::Draft,
                                            FaceOperationKind::Extend, FaceOperationKind::Trim, FaceOperationKind::Scale, FaceOperationKind::Hollow,
                                            FaceOperationKind::Delete, FaceOperationKind::Inset };

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
void RegisterWidget(SolidArcWorkspaceState& State, const char* Label)
{
    const char* Marker = std::strstr(Label, "###");
    const std::string Key = Marker ? std::string(Marker + 3) : std::string(Label);
    const ImVec2 Low  = ImGui::GetItemRectMin();
    const ImVec2 High = ImGui::GetItemRectMax();
    State.Widgets.Record(Key, Low, High);
}

bool PressButton(SolidArcWorkspaceState& State, const char* Label, ImVec2 Size = ImVec2(0, 0), bool EnabledCondition = true)
{
    ImGui::BeginDisabled(!EnabledCondition);
    const bool Pressed = ImGui::Button(Label, Size);
    ImGui::EndDisabled();
    RegisterWidget(State, Label);
    return Pressed && EnabledCondition;
}

bool ToggleButton(SolidArcWorkspaceState& State, const char* Label, bool OnCondition, ImVec2 Size = ImVec2(0, 0))
{
    // HTML .button.active: #ffffff29 wash with white text; off is the plain #222 pill with #999 text.
    ImGui::PushStyleColor(ImGuiCol_Button, OnCondition ? Tint(IM_COL32(0xff, 0xff, 0xff, 0x29)) : Tint(IM_COL32(0x22, 0x22, 0x22, 0xff)));
    ImGui::PushStyleColor(ImGuiCol_Text, OnCondition ? ImVec4(1, 1, 1, 1) : Tint(IM_COL32(0x99, 0x99, 0x99, 0xff)));
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

void Announce(SolidArcWorkspaceState& State, const std::string& RawMessage)
{
    const std::string Message = Legible(RawMessage);
    State.Status = Message;
    FRONTIER_TRACE("Status", Message);
    State.Journal.push_back(Message);
    if (State.Journal.size() > 200) State.Journal.erase(State.Journal.begin());
}

void Conclude(SolidArcWorkspaceState& State, const SolidArcCommandOutcome& Outcome)
{
    if (Outcome.AcceptedCondition)
    {
        RefreshSolidArcScene(State, true);
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

// 📝 HTML .inspector-card as an ImGui child: radius 22, #1a1a1a, #ffffff03 hairline, padding 18; its .section-heading (h3
//    17 px Light, 8 px letter-spaced tag on the right) is drawn by the sheet, 13 px above the content.
constexpr float CardHeadingHeight = 17.0f * SolidArcStyleTokens::FontScale + 13.0f;

void DrawCardHeading(SolidArcSheet& Sheet, float Left, float Top, float Width, const char* Title, const char* Tag)
{
    const float Centre = Top + 0.5f * 17.0f * SolidArcStyleTokens::FontScale;
    Sheet.Text(Left, Centre, 17.0f, SolidArcInk(0xeeeeee), Title);
    if (Tag && *Tag) Sheet.TextRight(Left + Width, Centre, 8.0f, SolidArcInk(0x777777), Tag, SolidArcWeight::Light, 1.0f);
}

void BeginCard(const char* Title, const char* Tag = nullptr)
{
    ImGui::PushStyleColor(ImGuiCol_ChildBg, Tint(IM_COL32(0x1a, 0x1a, 0x1a, 0xff)));
    ImGui::PushStyleColor(ImGuiCol_Border, Tint(IM_COL32(0xff, 0xff, 0xff, 0x03)));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 22.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(18, 18));
    ImGui::BeginChild(Title, ImVec2(0, 0), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysUseWindowPadding | ImGuiChildFlags_Borders);
    ImGui::PopStyleColor();                                                                  // the card's own border only
    SolidArcSheet Sheet;
    if (Sheet.Begin())
    {
        const ImVec2 Origin = ImGui::GetCursorScreenPos();
        DrawCardHeading(Sheet, Origin.x, Origin.y, ImGui::GetContentRegionAvail().x, Title, Tag);
    }
    ImGui::Dummy(ImVec2(1.0f, CardHeadingHeight - ImGui::GetStyle().ItemSpacing.y));
}

void EndCard()
{
    ImGui::EndChild();
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor();
}

// Width of one of N buttons sharing the current card row (the card's work width, not what is left after SameLine).
ImVec2 RowSpan(int Count = 2)
{
    const float Spacing = ImGui::GetStyle().ItemSpacing.x;
    const float Width   = ImGui::GetCurrentWindow()->WorkRect.GetWidth();
    return ImVec2(std::floor((Width - float(Count - 1) * Spacing) / float(Count)), 0.0f);
}

const FeatureStepReport* FindReport(const SolidArcDocument& Document, const std::string& Label)
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

std::string PreviousRecoveryPath(const SolidArcWorkspaceState& State)
{
    return SolidArcPreviousRecoveryPath(State.Options);
}

bool WriteDocument(const SolidArcDocument& Document, const std::string& Path)
{
    std::error_code Failure;
    const std::filesystem::path Folder = std::filesystem::path(Path).parent_path();
    if (!Folder.empty()) std::filesystem::create_directories(Folder, Failure);
    return SaveSolidArcDocument(Document, Path);
}

void AdoptDocument(SolidArcWorkspaceState& State, SolidArcDocument Document, bool FitCondition)
{
    State.Bevel       = {};
    State.Document    = std::move(Document);
    State.FocusedStep = {};
    RefreshSolidArcScene(State, false);
    if (FitCondition && !State.Scene.Faces.empty()) FitSolidArcCamera(State.Camera, State.Scene);
}

bool OpenDocument(SolidArcWorkspaceState& State, const std::string& Path, bool AdoptPathCondition)
{
    const SolidArcDocumentFile File = LoadSolidArcDocumentFile(Path);
    if (!File.Refusal.empty())
    {
        Announce(State, "Open refused: " + File.Refusal);
        return false;
    }
    SolidArcDocument Document = CreateSolidArcDocument(File.Title);
    Document.Tree     = File.Tree;
    Document.Captions = File.Captions;
    Document.ImagePlanes = File.ImagePlanes;
    Document.Appearance  = File.Appearance;
    Document.DrawingSheet = File.DrawingSheet;
    RegenerateSolidArcDocument(Document, true);
    AdoptDocument(State, std::move(Document), true);
    if (AdoptPathCondition) State.DocumentPath = Path;
    Announce(State, Compose("Opened %s · %d steps", Path.c_str(), int(State.Document.Tree.Steps.size())));
    return true;
}

void SaveDocument(SolidArcWorkspaceState& State, const std::string& Path)
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

void StartNewDocument(SolidArcWorkspaceState& State)
{
    std::error_code Failure;
    if (std::filesystem::exists(State.Options.RecoveryPath))
        std::filesystem::copy_file(State.Options.RecoveryPath, PreviousRecoveryPath(State), std::filesystem::copy_options::overwrite_existing, Failure);
    else if (!State.Document.Tree.Steps.empty())
        WriteDocument(State.Document, PreviousRecoveryPath(State));
    AdoptDocument(State, CreateSolidArcDocument("Untitled"), false);
    State.DocumentPath = {};
    State.Camera       = OrbitCamera{ .FieldOfView = State.Options.FieldOfView, .OrthographicEnabled = State.Camera.OrthographicEnabled };
    Announce(State, "New document (previous recovery copy kept)");
}

void MarkDirty(SolidArcWorkspaceState& State)
{
    State.DirtyCondition = true;
    State.AutosaveTimer  = State.Options.AutosaveDelay;
}

// 📝 A matcap entry leaves the body (and its scene) as it is: only the state words change, so no re-tessellation.
void Undo(SolidArcWorkspaceState& State)
{
    if (State.Bevel.ActiveCondition) return;
    const bool AppearanceCondition = !State.Document.UndoEntries.empty() && State.Document.UndoEntries.back().AppearanceCondition;
    const std::set<std::string> Hidden = State.Document.Appearance.HiddenBodies;
    if (UndoSolidArcCommand(State.Document))
    {
        if (AppearanceCondition && Hidden == State.Document.Appearance.HiddenBodies) MarkDirty(State);
        else RefreshSolidArcScene(State, true);                                              // Geometry or visibility changed
        Announce(State, "Undo");
    }
}

void Redo(SolidArcWorkspaceState& State)
{
    if (State.Bevel.ActiveCondition) return;
    const bool AppearanceCondition = !State.Document.RedoEntries.empty() && State.Document.RedoEntries.back().AppearanceCondition;
    const std::set<std::string> Hidden = State.Document.Appearance.HiddenBodies;
    if (RedoSolidArcCommand(State.Document))
    {
        if (AppearanceCondition && Hidden == State.Document.Appearance.HiddenBodies) MarkDirty(State);
        else RefreshSolidArcScene(State, true);                                              // Geometry or visibility changed
        Announce(State, "Redo");
    }
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    MATCAPS
//------------------------------------------------------------------------------------------------------------------------

void ConcludeAppearance(SolidArcWorkspaceState& State, const SolidArcCommandOutcome& Outcome)
{
    if (Outcome.AcceptedCondition)
    {
        MarkDirty(State);
        Announce(State, Outcome.Label);
    }
    else Announce(State, "Refused: " + Outcome.Refusal);
}

// M picker / swatch click: the selected faces (or the body) take the matcap.
void AssignMatcap(SolidArcWorkspaceState& State, const std::string& Name)
{
    ConcludeAppearance(State, AssignSolidArcMatcap(State.Document, Name));
}

// Alt+M (Plasticity remove-material).
void RemoveMatcap(SolidArcWorkspaceState& State)
{
    ConcludeAppearance(State, RemoveSolidArcMatcap(State.Document));
}

// A user picture: the file stem becomes the matcap name ("Shop steel.png" → "Shopsteel").
void AddMatcapPicture(SolidArcWorkspaceState& State, const std::string& Path)
{
    std::string Stem = Path.substr(Path.find_last_of("/\\") == std::string::npos ? 0 : Path.find_last_of("/\\") + 1);
    Stem = Stem.substr(0, Stem.find_last_of('.'));
    std::erase_if(Stem, [](char Character) { return std::isspace(static_cast<unsigned char>(Character)) != 0; });
    ConcludeAppearance(State, AddSolidArcMatcapPicture(State.Document, Stem, Path));
    RefreshSolidArcMatcaps(State);
}

// 📝 The matcap the selection shows: faces first, else picked bodies, else the default; one name when all agree, else "mixed".
std::string SelectionMatcap(const SolidArcWorkspaceState& State)
{
    const SolidArcDocument&   Document   = State.Document;
    const SolidArcAppearance& Appearance = Document.Appearance;
    const auto FaceShows = [&](const std::string& Face)
    { return SolidArcFaceMatcap(Appearance, SolidArcBodyOfName(Document, SolidArcSelectionKind::Face, Face), Face); };
    if (!Document.Selection.Faces.empty())
    {
        const std::string Shown = FaceShows(Document.Selection.Faces.front());
        for (const std::string& Face : Document.Selection.Faces) if (FaceShows(Face) != Shown) return "mixed";
        return Shown;
    }
    if (Document.Selection.Bodies.empty()) return Appearance.BodyMatcap;
    const std::string Shown = SolidArcBodyMatcap(Appearance, Document.Selection.Bodies.front());
    for (const std::string& Key : Document.Selection.Bodies) if (SolidArcBodyMatcap(Appearance, Key) != Shown) return "mixed";
    return Shown;
}

// The body a Union / Difference / Intersection acts on: the first body picked in mode 4, else the active body (empty).
std::string BooleanTarget(const SolidArcWorkspaceState& State)
{
    return State.Document.Selection.Bodies.empty() ? std::string{} : State.Document.Selection.Bodies.front();
}

// 💡 Swatch grid (thumbnails when the frame registered them, else a flat colour from the sphere's lit side) in category
//    order Studio · Metal · Plastic · Other · Pictures. TitledCondition writes the category above each group (the picker);
//    otherwise one compact flow fills the available width (the Inspector). Columns = 0 fits as many as the width allows.
//    Returns the clicked matcap name or "". Widget keys: Prefix + name ("MatcapBrass", "PickMatcapBrass").
std::string DrawMatcapSwatches(SolidArcWorkspaceState& State, const char* Prefix, int Columns, float Side, bool TitledCondition)
{
    const SolidArcMatcapLibrary& Library = State.Matcaps.Library;
    const std::string Current = SelectionMatcap(State);
    std::string Chosen;
    std::vector<std::pair<std::string, std::vector<size_t>>> Groups = { { "Studio", { 0 } }, { "Metal", {} }, { "Plastic", {} }, { "Other", {} }, { "Pictures", {} } };
    const std::vector<SolidArcMatcapDescription>& BuiltIn = SolidArcBuiltInMatcaps();
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
                    const SolidArcPicture& Picture = Library.Pictures[Layer - 1];
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
void DrawAppearanceCard(SolidArcWorkspaceState& State)
{
    if (!State.Document.Summary.PresentCondition) return;
    BeginCard("Appearance");
    const size_t Faces   = State.Document.Selection.Faces.size();
    const std::string Shown = SelectionMatcap(State);
    const size_t Layer   = SolidArcMatcapLayer(State.Matcaps.Library, Shown);
    const std::string Caption = Shown == "mixed" ? std::string("mixed") : Layer < State.Matcaps.Library.Captions.size() ? State.Matcaps.Library.Captions[Layer] : Shown;
    const size_t Bodies  = State.Document.Selection.Bodies.size();
    const SolidArcAppearance& Appearance = State.Document.Appearance;
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
void DrawMatcapPicker(SolidArcWorkspaceState& State)
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

[[nodiscard]] std::vector<std::string> SelectedSketchLabels(const SolidArcDocument& Document);
[[nodiscard]] bool FocusedSketchCondition(const SolidArcWorkspaceState& State);

// B on sketches (Plasticity fillet-curve): the picked mode-1 points of one sketch, else every corner of the picked
// sketches, else of the focused sketch.
[[nodiscard]] SolidArcCornerRequest CornerTargets(const SolidArcWorkspaceState& State)
{
    SolidArcCornerRequest Request;
    if (!State.SketchPoints.Sketch.empty() && !State.SketchPoints.Points.empty())
    {
        Request.Sketches = { State.SketchPoints.Sketch };
        Request.Points   = State.SketchPoints.Points;
        return Request;
    }
    Request.Sketches = SelectedSketchLabels(State.Document);
    if (Request.Sketches.empty() && FocusedSketchCondition(State)) Request.Sketches = { State.FocusedStep };
    return Request;
}

// The corner request the session would apply.
[[nodiscard]] SolidArcCornerRequest ComposeCornerRequest(const SolidArcBevelSession& Bevel)
{
    SolidArcCornerRequest Request = Bevel.Corner;
    Request.Size  = Bevel.Size;
    Request.Shape = Bevel.Shape == BlendProfile::Fillet ? SketchCornerShape::Fillet : SketchCornerShape::Chamfer;
    return Request;
}

void WriteBevelText(SolidArcBevelSession& Bevel)
{
    std::snprintf(Bevel.SizeText.data(), Bevel.SizeText.size(), "%.4g", Bevel.Size);
    Bevel.SizeTypedCondition = false;
    std::snprintf(Bevel.EndSizeText.data(), Bevel.EndSizeText.size(), "%.4g", Bevel.EndSize);
    std::snprintf(Bevel.DepthText.data(), Bevel.DepthText.size(), "%.4g", Bevel.Depth);
}

// The blend the session would apply (variable sizes and the flip only when V is on).
[[nodiscard]] SolidArcBlendRequest ComposeBlendRequest(const SolidArcBevelSession& Bevel)
{
    SolidArcBlendRequest Request;
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
void ToggleBevelVariable(SolidArcWorkspaceState& State)
{
    SolidArcBevelSession& Bevel = State.Bevel;
    if (!Bevel.ActiveCondition || Bevel.Verb != SolidArcSessionVerb::Blend) return;
    Bevel.VariableEnabled = !Bevel.VariableEnabled;
    if (Bevel.VariableEnabled && !(Bevel.EndSize > 0.0)) Bevel.EndSize = std::round(0.5 * Bevel.Size * 1.0e4) * 1.0e-4;
    WriteBevelText(Bevel);
    Announce(State, Bevel.VariableEnabled ? Compose("Variable size: %s m at the anchor corner → %s m (F flips the anchor)", FormatLength(Bevel.Size).c_str(), FormatLength(Bevel.EndSize).c_str())
                                          : std::string("Constant size"));
}

// F: anchor each edge at its other end (the start size moves to the opposite corner).
void FlipBevelAnchor(SolidArcWorkspaceState& State)
{
    SolidArcBevelSession& Bevel = State.Bevel;
    if (!Bevel.ActiveCondition || Bevel.Verb != SolidArcSessionVerb::Blend || !Bevel.VariableEnabled) return;
    Bevel.ReversedCondition = !Bevel.ReversedCondition;
    Announce(State, Bevel.ReversedCondition ? "Anchor flipped: the start size is at each edge's end corner" : "Anchor: the start size is at each edge's start corner");
}

void OpenBevelSession(SolidArcWorkspaceState& State, BlendProfile Shape)
{
    if (State.Document.Selection.Edges.empty())
    {
        // 2D: sketch corners (picked points in mode 1, or whole sketches).
        SolidArcCornerRequest Corner = CornerTargets(State);
        if (Corner.Sketches.empty())
        {
            Announce(State, "Bevel needs selected edges (mode 2), sketch corner points (mode 1) or a picked sketch");
            return;
        }
        const double Previous = State.Bevel.Verb == SolidArcSessionVerb::Corner ? State.Bevel.Size : 0.0;
        State.Bevel                 = {};
        State.Bevel.ActiveCondition = true;
        State.Bevel.Verb            = SolidArcSessionVerb::Corner;
        State.Bevel.Shape           = Shape;
        State.Bevel.Size            = Previous > 0.0 ? Previous : 0.1;
        State.Bevel.Corner          = std::move(Corner);
        WriteBevelText(State.Bevel);
        Announce(State, std::string(Shape == BlendProfile::Fillet ? "Fillet" : "Chamfer") + " sketch corners (B): move horizontally · D fillet · C chamfer · Enter / right-click applies · Esc aborts");
        return;
    }
    const double Previous = State.Bevel.Verb == SolidArcSessionVerb::Blend ? State.Bevel.Size : 0.0;
    State.Bevel                 = {};
    State.Bevel.ActiveCondition = true;
    State.Bevel.Verb            = SolidArcSessionVerb::Blend;
    State.Bevel.Shape           = Shape;
    State.Bevel.Size            = Previous > 0.0 ? Previous : 0.25;
    WriteBevelText(State.Bevel);
    Announce(State, "Fillet (B): move horizontally · D fillet · C chamfer · V variable size · F flip its anchor · click edits the edges · Enter / right-click applies · Esc aborts");
}

// Plasticity E: extrude the selected faces; the pointer sets the signed distance like the fillet size.
void OpenExtrudeSession(SolidArcWorkspaceState& State)
{
    if (State.Document.Selection.Faces.empty())
    {
        Announce(State, "Extrude needs selected faces (mode 3)");
        return;
    }
    const double Remembered = State.LastCommand.Kind == SolidArcRepeatKind::Face && State.LastCommand.Operation == FaceOperationKind::Extrude ? State.LastCommand.Amount : 0.5;
    State.Bevel                 = {};
    State.Bevel.ActiveCondition = true;
    State.Bevel.Verb            = SolidArcSessionVerb::Extrude;
    State.Bevel.Size            = Remembered;
    WriteBevelText(State.Bevel);
    Announce(State, "Extrude (E): move horizontally · click edits the faces · Enter / right-click applies · Esc aborts");
}

// Plasticity O on faces (offset-curve of the face boundary) · Blender I: inset the selected planar faces. The pointer sets
// the inset distance (D) or the depth of the inner face (H: + boss, − pocket); Tab-free, the card fields take typed values.
void OpenInsetSession(SolidArcWorkspaceState& State)
{
    if (State.Document.Selection.Faces.empty())
    {
        Announce(State, "Inset needs selected planar faces (mode 3)");
        return;
    }
    const bool Remembered = State.LastCommand.Kind == SolidArcRepeatKind::Face && State.LastCommand.Operation == FaceOperationKind::Inset;
    State.Bevel                 = {};
    State.Bevel.ActiveCondition = true;
    State.Bevel.Verb            = SolidArcSessionVerb::Inset;
    State.Bevel.Size            = Remembered ? State.LastCommand.Amount : 0.1;
    State.Bevel.Depth           = Remembered ? State.LastCommand.Depth : 0.0;
    WriteBevelText(State.Bevel);
    Announce(State, "Inset (O / I): move horizontally · D distance · H depth (− pocket) · click edits the faces · Enter / right-click applies · Esc aborts");
}

// Profile extrude start: a Union pointing out of the curved face the sketch is tangent to starts sunk into it.
[[nodiscard]] double ProfileStart(const SolidArcBevelSession& Bevel)
{
    return Bevel.Combination == FeatureCombination::Union && Bevel.Size > 0.0 ? -Bevel.ProfileSink : 0.0;
}

void StageBevelPreview(SolidArcWorkspaceState& State)
{
    SolidArcBevelSession& Bevel = State.Bevel;
    if (!Bevel.ActiveCondition) return;
    if (Bevel.StagedSize == Bevel.Size && Bevel.StagedShape == Bevel.Shape && Bevel.PreviewVerb == Bevel.Verb &&
        Bevel.StagedVariable == Bevel.VariableEnabled && Bevel.StagedEndSize == Bevel.EndSize && Bevel.StagedReversed == Bevel.ReversedCondition &&
        Bevel.StagedCombination == Bevel.Combination && Bevel.StagedDepth == Bevel.Depth) return;
    // 💡 A slow preview (a blend along a cylinder–cylinder junction takes seconds) is rebuilt once the pointer rests
    //    (one size held for 0.15 s and 6 frames: a slow frame alone is no rest), not on every pixel of mouse adjust: the
    //    last preview stays up meanwhile. A typed (pinned) size, a new shape, verb or Boolean, and every fast preview
    //    still restage at once.
    const double Now = std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
    const bool SizeOnly = Bevel.Preview && Bevel.StagedShape == Bevel.Shape && Bevel.PreviewVerb == Bevel.Verb && Bevel.StagedVariable == Bevel.VariableEnabled &&
                          Bevel.StagedEndSize == Bevel.EndSize && Bevel.StagedReversed == Bevel.ReversedCondition && Bevel.StagedCombination == Bevel.Combination &&
                          Bevel.StagedDepth == Bevel.Depth;
    if (SizeOnly && !Bevel.PinnedCondition && Bevel.StageMs > 40.0)
    {
        if (Bevel.PendingSize != Bevel.Size) { Bevel.PendingSize = Bevel.Size; Bevel.PendingSince = Now; Bevel.PendingFrame = State.FrameNumber; return; }
        if (State.FrameNumber < Bevel.PendingFrame + 6 || Now - Bevel.PendingSince < 0.15) return;
    }
    Bevel.StagedDepth       = Bevel.Depth;
    Bevel.StagedCombination = Bevel.Combination;
    Bevel.StagedSize     = Bevel.Size;
    Bevel.StagedShape    = Bevel.Shape;
    Bevel.PreviewVerb    = Bevel.Verb;
    Bevel.StagedVariable = Bevel.VariableEnabled;
    Bevel.StagedEndSize  = Bevel.EndSize;
    Bevel.StagedReversed = Bevel.ReversedCondition;
    static const char* Verbs[] = { "Blend", "Extrude", "Profile", "Inset", "Corner" };
    FRONTIER_TRACE_SCOPE("Preview", std::string("StageBevelPreview · ") + Verbs[std::clamp(int(Bevel.Verb), 0, 4)] + " · size " + TraceNumber(Bevel.Size));
    SolidArcPhaseClock StageClock(State.Profile, "bevel preview · stage");
    Bevel.Preview     = std::make_unique<SolidArcDocument>(CloneSolidArcDocument(State.Document));
    const SolidArcCommandOutcome Outcome = Bevel.Verb == SolidArcSessionVerb::Profile ? ExtrudeSolidArcSketch(*Bevel.Preview, Bevel.ProfileLabel, Bevel.Size, Bevel.Combination, BooleanTarget(State), ProfileStart(Bevel))
                                         : Bevel.Verb == SolidArcSessionVerb::Extrude ? ChangeSolidArcSelection(*Bevel.Preview, FaceOperationKind::Extrude, Bevel.Size)
                                         : Bevel.Verb == SolidArcSessionVerb::Inset   ? InsetSolidArcSelection(*Bevel.Preview, Bevel.Size, Bevel.Depth)
                                         : Bevel.Verb == SolidArcSessionVerb::Corner  ? CutSolidArcSketchCorners(*Bevel.Preview, ComposeCornerRequest(Bevel))
                                                                                      : BlendSolidArcSelection(*Bevel.Preview, ComposeBlendRequest(Bevel));
    if (Outcome.AcceptedCondition)
    {
        Bevel.PreviewScene = BuildSolidArcScene(*Bevel.Preview);
        Bevel.Refusal      = Legible(Bevel.PreviewScene.Refusal);
    }
    else
    {
        Bevel.Refusal = Legible(Outcome.Refusal);
    }
    Bevel.StageMs = (std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count() - Now) * 1.0e3;
    ++Bevel.StageCount;
}

void ApplyBevelSession(SolidArcWorkspaceState& State)
{
    // A refusal of a stale preview (the pointer moved on while a slow preview waited) does not block: the kernel decides.
    if (!State.Bevel.ActiveCondition) return;
    // A size typed in the field but not entered (right-click straight after typing) is the size the user means.
    SolidArcBevelSession& Typed = State.Bevel;
    double TypedSize = 0.0;
    if (Typed.SizeTypedCondition && ParseSolidArcLength(Typed.SizeText.data(), TypedSize) && TypedSize != 0.0)
    {
        const bool Signed = Typed.Verb == SolidArcSessionVerb::Extrude || Typed.Verb == SolidArcSessionVerb::Profile;
        if (TypedSize < 0.0 && (Typed.Verb == SolidArcSessionVerb::Blend || Typed.Verb == SolidArcSessionVerb::Corner)) Typed.Shape = BlendProfile::Chamfer;
        Typed.Size            = Signed ? TypedSize : std::abs(TypedSize);
        Typed.PinnedCondition = true;
    }
    Typed.SizeTypedCondition = false;
    if (!State.Bevel.Refusal.empty() && State.Bevel.StagedSize == State.Bevel.Size)
    {
        Announce(State, "Not applied: " + State.Bevel.Refusal + " · change the size, or Esc");
        return;
    }
    const BlendProfile         Shape   = State.Bevel.Shape;
    const double               Size    = State.Bevel.Size;
    const SolidArcBlendRequest Request = ComposeBlendRequest(State.Bevel);
    State.Bevel.ActiveCondition = false;
    State.Bevel.Preview.reset();
    if (State.Bevel.Verb == SolidArcSessionVerb::Corner)
    {
        const SolidArcCommandOutcome Outcome = CutSolidArcSketchCorners(State.Document, ComposeCornerRequest(State.Bevel));
        if (Outcome.AcceptedCondition) State.SketchPoints.Points.clear();
        Conclude(State, Outcome);
        return;
    }
    if (State.Bevel.Verb == SolidArcSessionVerb::Profile)
    {
        const SolidArcCommandOutcome Outcome = ExtrudeSolidArcSketch(State.Document, State.Bevel.ProfileLabel, Size, State.Bevel.Combination, BooleanTarget(State),
                                                                     State.Bevel.Combination == FeatureCombination::Union && Size > 0.0 ? -State.Bevel.ProfileSink : 0.0);
        if (Outcome.AcceptedCondition) State.FocusedStep = {};
        Conclude(State, Outcome);
        return;
    }
    if (State.Bevel.Verb == SolidArcSessionVerb::Inset)
    {
        const double                 Depth   = State.Bevel.Depth;
        const SolidArcCommandOutcome Outcome = InsetSolidArcSelection(State.Document, Size, Depth);
        if (Outcome.AcceptedCondition) State.LastCommand = { SolidArcRepeatKind::Face, Shape, 0.0, FaceOperationKind::Inset, Size, {}, {}, Depth };
        Conclude(State, Outcome);
        return;
    }
    if (State.Bevel.Verb == SolidArcSessionVerb::Extrude)
    {
        const SolidArcCommandOutcome Outcome = ChangeSolidArcSelection(State.Document, FaceOperationKind::Extrude, Size);
        if (Outcome.AcceptedCondition) State.LastCommand = { SolidArcRepeatKind::Face, Shape, 0.0, FaceOperationKind::Extrude, Size, {} };
        Conclude(State, Outcome);
        return;
    }
    const SolidArcCommandOutcome Outcome = BlendSolidArcSelection(State.Document, Request);
    if (Outcome.AcceptedCondition) State.LastCommand = { SolidArcRepeatKind::Blend, Shape, Size, FaceOperationKind::Extrude, 0.0, {}, Request };
    Conclude(State, Outcome);
}

void CancelBevelSession(SolidArcWorkspaceState& State)
{
    if (!State.Bevel.ActiveCondition) return;
    State.Bevel.ActiveCondition = false;
    State.Bevel.Preview.reset();
    Announce(State, "Bevel cancelled (document untouched)");
}

// Positive signed size = fillet radius, negative = chamfer setback (SolidArc pointer convention).
void AdjustBevelSize(SolidArcWorkspaceState& State, double Delta)
{
    SolidArcBevelSession& Bevel = State.Bevel;
    if (Bevel.Verb == SolidArcSessionVerb::Inset)
    {
        if (Bevel.DepthEnabled) Bevel.Depth = std::round((Bevel.Depth + Delta) * 1.0e4) * 1.0e-4;          // [len] - Signed: + boss, − pocket
        else Bevel.Size = std::round(std::max(1.0e-3, Bevel.Size + Delta) * 1.0e4) * 1.0e-4;             // [len] - Inset distance > 0
        WriteBevelText(Bevel);
        return;
    }
    if (Bevel.Verb == SolidArcSessionVerb::Extrude || Bevel.Verb == SolidArcSessionVerb::Profile)
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

OrbitCameraFrame ViewportFrame(const SolidArcWorkspaceState& State);

[[nodiscard]] Vector3 PlacePlanePoint(const OrthonormalFrame& Plane, Vector2 Point)
{
    return Plane.Origin + Plane.XAxis * Point.X + Plane.YAxis * Point.Y;
}

// The drawing plane of a session: the grid plane, raised by Elevation along its normal (solids: XY at the first click).
// A sketch / curve the pointer may pick, drag or trim: neither hidden nor locked (Outliner eye / lock).
[[nodiscard]] bool CurveReachableCondition(const SolidArcDocument& Document, const std::string& Label)
{
    return !SolidArcCurveHiddenCondition(Document, Label) && !SolidArcLockedCondition(Document, Label);
}

[[nodiscard]] OrthonormalFrame DrawingPlane(const SolidArcDrawingSession& Drawing)
{
    if (Drawing.FacePlaneCondition) return Drawing.FacePlane;                              // Sketch / solid on a body face
    OrthonormalFrame Plane = SolidArcSketchPlane(Drawing.GridPlane);
    Plane.Origin = Plane.Origin + Plane.ZAxis * Drawing.Elevation;
    return Plane;
}

// Plane clicks before the height (or before the solid, when it has no height).
[[nodiscard]] size_t SolidPlaneClicks(SolidArcConstructKind Kind)
{
    return Kind == SolidArcConstructKind::Torus || Kind == SolidArcConstructKind::Tube ? 3 : 2;
}

[[nodiscard]] bool SolidLiftedCondition(SolidArcConstructKind Kind)
{
    return Kind != SolidArcConstructKind::Sphere && Kind != SolidArcConstructKind::Torus;
}

// Prompt for the next click of a solid session.
[[nodiscard]] const char* SolidPrompt(const SolidArcDrawingSession& Drawing)
{
    const size_t Placed = Drawing.Points.size();
    switch (Drawing.Solid)
    {
    case SolidArcConstructKind::Block:
    case SolidArcConstructKind::Wedge:    return Placed == 0 ? (Drawing.CentreCondition ? "click the centre" : "click the first corner") : Placed == 1 ? "click the opposite corner" : "move up, click the height";
    case SolidArcConstructKind::Cylinder:
    case SolidArcConstructKind::Cone:     return Placed == 0 ? "click the base centre" : Placed == 1 ? "click the radius" : Drawing.Solid == SolidArcConstructKind::Cone ? "move up, click the apex height" : "move up, click the height";
    case SolidArcConstructKind::Sphere:   return Placed == 0 ? "click the centre" : "click the radius";
    case SolidArcConstructKind::Torus:    return Placed == 0 ? "click the centre" : Placed == 1 ? "click the major radius (ring centre line)" : "click the tube (minor) radius";
    case SolidArcConstructKind::Prism:    return Placed == 0 ? "click the centre" : Placed == 1 ? "click a vertex (+ / − sides)" : "move up, click the height";
    case SolidArcConstructKind::Tube:     return Placed == 0 ? "click the centre" : Placed == 1 ? "click the outer radius" : Placed == 2 ? "click the bore radius" : "move up, click the height";
    }
    return "click";
}

void FinishSolidSession(SolidArcWorkspaceState& State);

// World point → frame pixel (absolute); false behind the eye.
bool ProjectWorldPoint(const SolidArcWorkspaceState& State, const OrbitCameraFrame& Frame, Vector3 Point, ImVec2& Pixel)
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
bool IntersectSketchPlane(const OrbitCameraFrame& Frame, ImVec2 Local, const OrthonormalFrame& Plane, Vector2& Point)
{
    const OrbitRay      Ray    = CastOrbitRay(Frame, Local.x, Local.y);
    const Vector3          Origin{ Ray.Origin[0], Ray.Origin[1], Ray.Origin[2] }, Direction{ Ray.Direction[0], Ray.Direction[1], Ray.Direction[2] };
    const double           Facing = Dot(Direction, Plane.ZAxis);
    if (std::abs(Facing) < 1.0e-9) return false;
    const Vector3 Hit = Origin + Direction * (Dot(Plane.Origin - Origin, Plane.ZAxis) / Facing);
    Point = { Dot(Hit - Plane.Origin, Plane.XAxis), Dot(Hit - Plane.Origin, Plane.YAxis) };
    return true;
}

[[nodiscard]] SolidArcDrawingRequest ComposeDrawingRequest(const SolidArcDrawingSession& Drawing, bool PointerCondition, bool ClosedCondition)
{
    SolidArcDrawingRequest Request;
    Request.Kind            = Drawing.Kind;
    Request.GridPlane       = Drawing.GridPlane;
    Request.Points          = Drawing.Points;
    Request.CentreCondition = Drawing.CentreCondition;
    Request.Form            = Drawing.Form;
    Request.ClosedCondition = ClosedCondition;
    Request.Sides           = Drawing.Sides;
    Request.PlaneCondition  = Drawing.FacePlaneCondition;
    Request.Plane           = Drawing.FacePlane;
    if (PointerCondition && Drawing.PointerCondition) Request.Points.push_back(Drawing.Pointer);
    return Request;
}

// Points that finish the tool on their last click (0: open-ended chain).
[[nodiscard]] size_t DrawingClicks(SolidArcDrawingKind Kind)
{
    switch (Kind)
    {
    case SolidArcDrawingKind::Rectangle:
    case SolidArcDrawingKind::Circle:
    case SolidArcDrawingKind::Polygon: return 2;
    case SolidArcDrawingKind::Arc:
    case SolidArcDrawingKind::Slot:   return 3;
    default:                          return 0;
    }
}

[[nodiscard]] std::string DrawingCaption(const SolidArcDrawingSession& Drawing)
{
    if (Drawing.SolidCondition) return std::string(SolidArcConstructLabel(Drawing.Solid)) + (Drawing.Solid == SolidArcConstructKind::Prism ? Compose(" · %d sides", Drawing.Sides) : std::string()) +
                                       " · " + (Drawing.Combination == GeometryProcessing::FeatureCombination::Separate ? "new body" : FeatureCombinationLabel(Drawing.Combination));
    std::string Caption = SolidArcDrawingLabel(Drawing.Kind);
    if (Drawing.Kind == SolidArcDrawingKind::Polygon) Caption += Compose(" · %d sides", Drawing.Sides);
    if (Drawing.Kind == SolidArcDrawingKind::Line) Caption += Drawing.SegmentCondition ? " · segment" : " · polyline";
    if (Drawing.Kind == SolidArcDrawingKind::Curve) Caption += std::string(" · ") + SketchSplineFormLabel(Drawing.Form);
    if (Drawing.Kind == SolidArcDrawingKind::Rectangle) Caption += Drawing.CentreCondition ? " · centre" : " · corner";
    if (Drawing.Kind == SolidArcDrawingKind::Circle) Caption += Drawing.CentreCondition ? " · centre" : " · two-point";
    if (Drawing.FacePlaneCondition) Caption += Drawing.FaceName.empty() ? std::string(" · level with the snap") : " · on face " + Drawing.FaceName;
    return Caption;
}

void OpenDrawingSession(SolidArcWorkspaceState& State, SolidArcDrawingKind Kind)
{
    if (State.Bevel.ActiveCondition) return;
    const SketchSplineForm Form  = State.Drawing.Form;
    const int              Sides = State.Drawing.Sides;
    State.Drawing                 = {};
    State.Drawing.Sides           = Sides;
    State.Drawing.ActiveCondition = true;
    State.Drawing.Kind            = Kind;
    State.Drawing.GridPlane       = std::clamp(State.Style.GridPlane, 0, 2);
    State.Drawing.CentreCondition = Kind == SolidArcDrawingKind::Circle;                     // Plasticity center-circle; V → two-point
    State.Drawing.Form            = Form;
    static const char* Hints[] = { "click points · click the first point closes · Enter / right-click finishes open · Ctrl+Z drops a point",
                                   "click two corners · Alt centre mode", "click centre and rim · V two-point mode", "click start, end, then a point on the arc",
                                   "click both cap centres, then the rim", "click points · 1 Hermite · 2 Bezier · 3 NURBS · 4 cubic spline · Enter / right-click finishes · Ctrl+Z drops a point",
                                   "click the centre, then a vertex · + / − sides" };
    static const char* Planes[] = { "XY", "XZ", "YZ" };
    Announce(State, std::string(SolidArcDrawingLabel(Kind)) + " on plane " + Planes[State.Drawing.GridPlane] + " (or the body face under the first click): " + Hints[int(Kind)]
                        + " · hold Ctrl to snap to ends / midpoints / centres, Alt to slide along a curve · Esc aborts");
}

void AdoptKnifeSketch(SolidArcWorkspaceState& State, const std::string& Sketch, const std::string& Face);

void CancelDrawingSession(SolidArcWorkspaceState& State)
{
    if (!State.Drawing.ActiveCondition) return;
    State.Drawing.ActiveCondition = false;
    Announce(State, std::string(State.Drawing.SolidCondition ? SolidArcConstructLabel(State.Drawing.Solid) : SolidArcDrawingLabel(State.Drawing.Kind)) + " aborted (document untouched)");
    AdoptKnifeSketch(State, {}, {});                                                         // A knife shape aborted: the knife closes
}

// Commits the collected points as a sketch step; the new sketch becomes the focused step (E extrudes it).
void FinishDrawingSession(SolidArcWorkspaceState& State, bool ClosedCondition)
{
    SolidArcDrawingSession& Drawing = State.Drawing;
    if (!Drawing.ActiveCondition) return;
    if (Drawing.SolidCondition) { FinishSolidSession(State); return; }
    const bool KnifeCondition = State.Knife.ActiveCondition && State.Knife.DrawPendingCondition;
    SolidArcDrawingRequest Request = ComposeDrawingRequest(Drawing, false, ClosedCondition);
    if (KnifeCondition) Request.FillCondition = true;                                        // A knife cutter is solid
    SketchModel Probe;
    const std::string Refusal = ComposeSolidArcSketch(Request, Probe);
    if (!Refusal.empty())
    {
        Drawing.Refusal = Legible(Refusal);
        Announce(State, Drawing.Refusal);
        return;
    }
    Drawing.ActiveCondition = false;
    const size_t Before = State.Document.Tree.Steps.size();
    const SolidArcCommandOutcome Outcome = DrawSolidArcSketch(State.Document, Request);
    if (Outcome.AcceptedCondition && State.Document.Tree.Steps.size() > Before) State.FocusedStep = State.Document.Tree.Steps.back().Label;
    Conclude(State, Outcome);
    if (KnifeCondition)
        AdoptKnifeSketch(State, Outcome.AcceptedCondition && State.Document.Tree.Steps.size() > Before ? State.Document.Tree.Steps.back().Label : std::string{},
                         Drawing.FacePlaneCondition ? Drawing.FaceName : std::string{});
}

void AppendDrawingPoint(SolidArcWorkspaceState& State, Vector2 Point, bool ClosingCondition)
{
    SolidArcDrawingSession& Drawing = State.Drawing;
    if (Drawing.SolidCondition)
    {
        // Plane clicks (the first sets the plane height), then the height click creates the solid.
        const size_t Needed = SolidPlaneClicks(Drawing.Solid);
        if (Drawing.Points.size() >= Needed) { FinishSolidSession(State); return; }
        if (!Drawing.Points.empty() && Distance(Drawing.Points.back(), Point) <= 1.0e-12) { Announce(State, "Same point twice: ignored"); return; }
        if (Drawing.Points.empty()) Drawing.Elevation = Drawing.PointerElevation;
        Drawing.Points.push_back(Point);
        Drawing.Refusal.clear();
        Drawing.Lift = 0.0;
        State.Picker.Placed = State.Picker.Current;
        State.Picker.Lock   = SolidArcSnapLockKind::None;
        if (Drawing.Points.size() >= Needed && !SolidLiftedCondition(Drawing.Solid)) { FinishSolidSession(State); return; }
        Announce(State, std::string(SolidArcConstructLabel(Drawing.Solid)) + ": " + SolidPrompt(Drawing));
        return;
    }
    const bool Chain = DrawingClicks(Drawing.Kind) == 0 && !Drawing.SegmentCondition;
    if (Chain && ClosingCondition) { FinishDrawingSession(State, true); return; }
    if (!Drawing.Points.empty() && Distance(Drawing.Points.back(), Point) <= 1.0e-12)
    {
        Announce(State, "Same point twice: ignored");
        return;
    }
    Drawing.Points.push_back(Point);
    Drawing.Refusal.clear();
    SolidArcPointPicker& Picker = State.Picker;
    Picker.Placed = Picker.Current;                                                          // t / n / b use the snapped curve or face
    if (Picker.Current.Kind >= SolidArcSnapKind::Endpoint && Picker.Current.Kind <= SolidArcSnapKind::Face) ++Picker.Snapped;
    Picker.Lock = SolidArcSnapLockKind::None;                                                // A lock serves one point (Plasticity)
    if (!Chain && Drawing.Points.size() >= (Drawing.SegmentCondition ? size_t(2) : DrawingClicks(Drawing.Kind))) FinishDrawingSession(State, false);
}

void RetractDrawingPoint(SolidArcWorkspaceState& State)
{
    SolidArcDrawingSession& Drawing = State.Drawing;
    if (Drawing.Points.empty()) { Announce(State, "No point to drop"); return; }
    Drawing.Points.pop_back();
    Drawing.Lift = 0.0;
    if (Drawing.SolidCondition && Drawing.Points.empty()) Drawing.Elevation = 0.0;
    Drawing.Refusal.clear();
    State.Picker.Placed = {};
    State.Picker.Lock   = SolidArcSnapLockKind::None;
    Announce(State, Compose("Point dropped · %d left", int(Drawing.Points.size())));
}

// 📝 Object snapping (Plasticity point picker). The catalogue of exact carriers follows the document revision, the
//    scene's faces and the bodies excluded (Move skips the bodies it carries).
const SolidArcSectionClip* ActiveSectionClip(const SolidArcWorkspaceState& State);

[[nodiscard]] bool PointSnapCondition(SolidArcSnapKind Kind)
{
    return Kind >= SolidArcSnapKind::Endpoint && Kind <= SolidArcSnapKind::Perpendicular;
}

SolidArcSnapCatalogue& SnapCatalogue(SolidArcWorkspaceState& State, const std::vector<std::string>& Excluded)
{
    SolidArcPointPicker& Picker = State.Picker;
    std::string Key;
    for (const std::string& Body : Excluded) Key += Body + ";";
    for (const std::string& Hidden : State.Document.Appearance.HiddenCurves) Key += "~" + Hidden + ";";   // Hidden curves offer no snaps
    if (!Picker.Catalogue || Picker.Catalogue->Revision != State.Document.Revision || Picker.Catalogue->ExclusionKey != Key || Picker.SceneFaces != State.Scene.Faces.size())
    {
        Picker.Catalogue  = std::make_unique<SolidArcSnapCatalogue>(CatalogueSolidArcSnaps(State.Document, State.Scene, Excluded));
        Picker.Catalogue->ExclusionKey = Key;                                                // Exclusions and hidden curves
        Picker.SceneFaces = State.Scene.Faces.size();
    }
    return *Picker.Catalogue;
}

// 📝 The snap under the pointer. Ctrl snaps to the important points only (ends, midpoints, centres, crossings, tangent /
//    square feet); Alt slides along the nearest line / curve / arc / edge (the point stays on it); Ctrl+Alt offers both,
//    the important points winning. Without either the pointer keeps the grid. AnchorCondition (the grab point of a move)
//    snaps without a key, faces and edges included.
SolidArcSnap FindPointerSnap(SolidArcWorkspaceState& State, const OrbitCameraFrame& Frame, ImVec2 Local, std::optional<Vector3> Reference,
                             std::vector<SolidArcSnapPoint> Transient, const std::vector<std::string>& Excluded, bool AnchorCondition)
{
    const ImGuiIO& Io             = ImGui::GetIO();
    const bool     AlongCondition = !AnchorCondition && Io.KeyAlt;                            // Alt: on the curve
    const bool     PointCondition = AnchorCondition || Io.KeyCtrl;                            // Ctrl: important points
    if (!State.Picker.Enabled || (!PointCondition && !AlongCondition)) return {};
    SolidArcPhaseClock Clock(State.Profile, "snaps");
    SolidArcSnapQuery Query;
    Query.Frame       = Frame;
    Query.PixelX      = Local.x;
    Query.PixelY      = Local.y;
    Query.Reference   = Reference;
    Query.Transient   = std::move(Transient);
    Query.XRayEnabled = State.Style.XRayEnabled;
    Query.Section     = ActiveSectionClip(State);
    Query.Filter      = State.Picker.Filter;
    if (!AnchorCondition)
    {
        Query.Filter.CurveEnabled = AlongCondition;                                          // Alt slides along curves
        Query.Filter.FaceEnabled  = false;
        if (!PointCondition)                                                                 // Alt alone: the curve only
        {
            Query.Filter.EndpointEnabled      = false;
            Query.Filter.MidpointEnabled      = false;
            Query.Filter.CentreEnabled        = false;
            Query.Filter.IntersectionEnabled  = false;
            Query.Filter.TangentEnabled       = false;
            Query.Filter.PerpendicularEnabled = false;
            Query.Transient.clear();
        }
    }
    const SolidArcSnap Found = FindSolidArcSnap(SnapCatalogue(State, Excluded), State.Document, State.Scene, Query);
    if (AlongCondition && Found.Kind == SolidArcSnapKind::Curve) State.Picker.AlongUsedCondition = true;   // Alt was a snap, not a tap
    return Found;
}

// n / b / t / x / y / z while drawing: a line through the last point the next point slides along; the same key again
// releases it. Directions leaving the drawing plane are refused (the sketch stays planar).
void ChooseSnapLock(SolidArcWorkspaceState& State, SolidArcSnapLockKind Kind)
{
    SolidArcPointPicker&          Picker  = State.Picker;
    const SolidArcDrawingSession& Drawing = State.Drawing;
    if (Picker.Lock == Kind)
    {
        Picker.Lock = SolidArcSnapLockKind::None;
        Announce(State, std::string("Lock ") + SolidArcSnapLockLabel(Kind) + " released");
        return;
    }
    if (Drawing.Points.empty()) { Announce(State, std::string("Lock ") + SolidArcSnapLockLabel(Kind) + ": place a point first"); return; }
    const OrthonormalFrame Plane  = DrawingPlane(Drawing);
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
    case SolidArcSnapLockKind::X:        Direction = { 1.0, 0.0, 0.0 }; break;
    case SolidArcSnapLockKind::Y:        Direction = { 0.0, 1.0, 0.0 }; break;
    case SolidArcSnapLockKind::Z:        Direction = { 0.0, 0.0, 1.0 }; break;
    case SolidArcSnapLockKind::Tangent:  Direction = Tangent; break;
    case SolidArcSnapLockKind::Normal:   Direction = Normal; break;
    case SolidArcSnapLockKind::Binormal: Direction = Length(Tangent) > 0.5 && Length(Normal) > 0.5 ? Normalize(Cross(Tangent, Normal)) : Vector3{}; break;
    default: return;
    }
    static const char* Planes[] = { "XY", "XZ", "YZ" };
    if (Length(Direction) < 0.5)
    {
        Announce(State, std::string("Lock ") + SolidArcSnapLockLabel(Kind) + ": the last point has no curve or face direction (snap it to a curve, or place two points)");
        return;
    }
    if (std::abs(Dot(Direction, Plane.ZAxis)) > 1.0e-9)
    {
        Announce(State, std::string("Lock ") + SolidArcSnapLockLabel(Kind) + " leaves the " + Planes[std::clamp(Drawing.GridPlane, 0, 2)] + " drawing plane: refused");
        return;
    }
    Picker.Lock          = Kind;
    Picker.LockOrigin    = Origin;
    Picker.LockDirection = Direction;
    Announce(State, std::string("Lock ") + SolidArcSnapLockLabel(Kind) + " through the last point · the same key releases it · object snaps align along it");
}

// An object snap carried into a sketch plane (its foot along the plane normal). On-curve snaps count only on curves in
// the plane: an edge above the plane that merely overlaps the pointer on screen would move the point away from where
// it was clicked (its distinct points still snap). Sets Picker.Current; nullopt when nothing snaps.
std::optional<Vector2> SnapOntoPlane(SolidArcWorkspaceState& State, const OrbitCameraFrame& Frame, ImVec2 Local, const OrthonormalFrame& Plane,
                                     std::optional<Vector3> Reference, std::vector<SolidArcSnapPoint> Transient, const std::vector<std::string>& Excluded)
{
    SolidArcSnap Object = FindPointerSnap(State, Frame, Local, Reference, std::move(Transient), Excluded, false);
    // A curve off the plane (an edge above the sketch, a loft rim) counts by its foot on the plane, and only when that
    // foot is still under the pointer (a top view onto the plane); in a slanted view it would jump away from the click.
    const double Height = Dot(Object.Position - Plane.Origin, Plane.ZAxis);
    if (Object.Kind == SolidArcSnapKind::Curve && std::abs(Height) > 1.0e-9 * (1.0 + Length(Object.Position)))
    {
        const Vector3 Foot = Object.Position - Plane.ZAxis * Height;
        ImVec2 Pixel;
        const ImVec2 Pointer(State.ViewportRect[0] + Local.x, State.ViewportRect[1] + Local.y);
        if (ProjectWorldPoint(State, Frame, Foot, Pixel) && std::hypot(Pixel.x - Pointer.x, Pixel.y - Pointer.y) <= 8.0f)
        {
            Object.Position = Foot;
            Object.Source  += " (foot on the plane)";
        }
        else Object = {};
    }
    State.Picker.Current = Object;
    if (Object.Kind == SolidArcSnapKind::None) return std::nullopt;
    return Vector2{ Dot(Object.Position - Plane.Origin, Plane.XAxis), Dot(Object.Position - Plane.Origin, Plane.YAxis) };
}

// The drawing pointer: on the lock line (aligned with a point snap, else 0.1 steps), else (Ctrl) on an important object
// point projected into the drawing plane, else on the 0.1 grid (Ctrl with nothing in reach: free). Rectangle s squares
// the second corner.
void PlaceDrawingPointer(SolidArcWorkspaceState& State, const OrbitCameraFrame& Frame, ImVec2 Local, Vector2& Point)
{
    SolidArcDrawingSession& Drawing = State.Drawing;
    SolidArcPointPicker&    Picker  = State.Picker;
    const OrthonormalFrame  Plane   = DrawingPlane(Drawing);
    const bool              Free    = ImGui::GetIO().KeyCtrl;
    auto Flatten = [&](Vector3 World) { return Vector2{ Dot(World - Plane.Origin, Plane.XAxis), Dot(World - Plane.Origin, Plane.YAxis) }; };
    std::vector<SolidArcSnapPoint> Chain;
    for (size_t Index = 0; Index < Drawing.Points.size(); ++Index)
        Chain.push_back({ SolidArcSnapKind::Endpoint, PlacePlanePoint(Plane, Drawing.Points[Index]), {}, {}, Compose("Point %d", int(Index) + 1), {} });
    std::optional<Vector3> Reference;
    if (!Drawing.Points.empty()) Reference = PlacePlanePoint(Plane, Drawing.Points.back());
    Picker.Current = {};
    if (Picker.Lock != SolidArcSnapLockKind::None)
    {
        const std::optional<double> Along = ApproachSolidArcLine(CastOrbitRay(Frame, Local.x, Local.y), Picker.LockOrigin, Picker.LockDirection);
        if (Along)
        {
            double      Travel = *Along;
            std::string Note;
            const SolidArcSnap Object = FindPointerSnap(State, Frame, Local, std::nullopt, Chain, {}, false);
            if (PointSnapCondition(Object.Kind))
            {
                Travel = Dot(Object.Position - Picker.LockOrigin, Picker.LockDirection);
                Note   = std::string(" · level with ") + SolidArcSnapLabel(Object.Kind) + " " + Object.Source;
            }
            else if (!Free) Travel = std::round(Travel / DrawingSnap) * DrawingSnap;
            const Vector3 World = Picker.LockOrigin + Picker.LockDirection * Travel;
            Point          = Flatten(World);
            Picker.Current = { SolidArcSnapKind::Axis, World, Picker.LockDirection, {}, std::string("Lock ") + SolidArcSnapLockLabel(Picker.Lock) + Note, 0.0 };
        }
    }
    else
    {
        if (const std::optional<Vector2> Snapped = SnapOntoPlane(State, Frame, Local, Plane, Reference, Chain, {})) Point = *Snapped;
        else if (!Free) Point = { std::round(Point.X / DrawingSnap) * DrawingSnap, std::round(Point.Y / DrawingSnap) * DrawingSnap };
    }
    if (Drawing.Kind == SolidArcDrawingKind::Rectangle && Drawing.SquareCondition && Drawing.Points.size() == 1 &&
        (!Drawing.SolidCondition || Drawing.Solid == SolidArcConstructKind::Block || Drawing.Solid == SolidArcConstructKind::Wedge))
    {
        const Vector2 From  = Drawing.Points.front();
        const double  Side  = std::max(std::abs(Point.X - From.X), std::abs(Point.Y - From.Y));
        Point = { From.X + (Point.X >= From.X ? Side : -Side), From.Y + (Point.Y >= From.Y ? Side : -Side) };
    }
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    CONSTRUCT SOLIDS (DRAWN)
//------------------------------------------------------------------------------------------------------------------------

// 📝 A Construct tile is drawn, not typed (SolidArc live reference, Plasticity box / cylinder / sphere commands): plane
//    clicks on the ground (XY at the height of the first click — a snap onto a body starts there), then the pointer
//    lifts the height along +z and a click creates the solid as a new body (R; Q / W / E combine with the active body).
//
//        Block · Wedge    corner, opposite corner, height        Sphere   centre, radius
//        Cylinder · Cone  centre, radius, height                 Torus    centre, major radius, minor radius
//        Prism            centre, vertex (± sides), height       Tube     centre, outer radius, bore radius, height

// The solid the clicks (and, with PointerCondition, the pointer) describe so far. Refusal text while incomplete.
std::string ComposeSolidRequest(const SolidArcDrawingSession& Drawing, bool PointerCondition, SolidArcConstructRequest& Request)
{
    std::vector<Vector2> Points = Drawing.Points;
    const size_t Needed = SolidPlaneClicks(Drawing.Solid);
    if (PointerCondition && Drawing.PointerCondition && Points.size() < Needed) Points.push_back(Drawing.Pointer);
    Request      = {};
    Request.Kind = Drawing.Solid;
    Request.Sides = std::clamp(Drawing.Sides, 3, 64);
    Request.PlaneCondition = Drawing.FacePlaneCondition;                                     // On a body face: face-frame coordinates
    if (Drawing.FacePlaneCondition) Request.Plane = Drawing.FacePlane;
    if (Points.size() < 2) return std::string(SolidArcConstructLabel(Drawing.Solid)) + ": " + SolidPrompt(Drawing);
    const bool   Lifted = Points.size() >= Needed && Drawing.Points.size() >= Needed;
    const double Lift   = Lifted ? Drawing.Lift : 0.0;
    const double Base   = Drawing.Elevation + std::min(0.0, Lift);                               // [len] - Bottom of an upright solid
    const Vector2 Centre = Points[0];
    const double  Reach  = Distance(Points[0], Points[1]);
    switch (Drawing.Solid)
    {
    case SolidArcConstructKind::Block:
    case SolidArcConstructKind::Wedge:
    {
        const Vector2 First  = Drawing.CentreCondition ? Points[0] * 2.0 - Points[1] : Points[0];
        const Vector2 Second = Points[1];
        Request.Position = { std::min(First.X, Second.X), std::min(First.Y, Second.Y), Base };
        Request.Width    = std::abs(Second.X - First.X);
        Request.Depth    = std::abs(Second.Y - First.Y);
        Request.Height   = std::abs(Lift);
        if (Request.Width < 1.0e-9 || Request.Depth < 1.0e-9) return "the base has no width or no depth";
        break;
    }
    case SolidArcConstructKind::Cylinder:
    case SolidArcConstructKind::Prism:
        Request.Position = { Centre.X, Centre.Y, Base };
        Request.Radius   = Reach;
        Request.Height   = std::abs(Lift);
        Request.Angle    = std::atan2(Points[1].Y - Centre.Y, Points[1].X - Centre.X);
        if (Reach < 1.0e-9) return "the radius is zero";
        break;
    case SolidArcConstructKind::Cone:
        Request.Position = { Centre.X, Centre.Y, Drawing.Elevation };
        Request.Radius   = Reach;
        Request.Height   = Lift;
        if (Reach < 1.0e-9) return "the radius is zero";
        break;
    case SolidArcConstructKind::Sphere:
        Request.Position = { Centre.X, Centre.Y, Drawing.Elevation };
        Request.Radius   = Reach;
        if (Reach < 1.0e-9) return "the radius is zero";
        return {};
    case SolidArcConstructKind::Torus:
        Request.Position = { Centre.X, Centre.Y, Drawing.Elevation };
        Request.Major    = Reach;
        Request.Minor    = Points.size() >= 3 ? std::abs(Distance(Points[0], Points[2]) - Reach) : 0.0;
        if (Reach < 1.0e-9) return "the major radius is zero";
        if (Points.size() < 3) return "Torus: " + std::string(SolidPrompt(Drawing));
        if (Request.Minor < 1.0e-9 || Request.Minor >= Request.Major) return "the tube radius must be between 0 and the major radius";
        return {};
    case SolidArcConstructKind::Tube:
    {
        Request.Position = { Centre.X, Centre.Y, Base };
        const double Other = Points.size() >= 3 ? Distance(Points[0], Points[2]) : 0.0;
        Request.Radius   = std::max(Reach, Other);
        Request.Inner    = std::min(Reach, Other);
        Request.Height   = std::abs(Lift);
        if (Reach < 1.0e-9) return "the radius is zero";
        if (Points.size() < 3) return "Tube: " + std::string(SolidPrompt(Drawing));
        if (Request.Inner < 1.0e-9 || Request.Radius - Request.Inner < 1.0e-9) return "the bore must be a smaller, non-zero radius";
        break;
    }
    }
    if (!Lifted) return std::string(SolidArcConstructLabel(Drawing.Solid)) + ": " + SolidPrompt(Drawing);
    if (std::abs(Lift) < 1.0e-9) return "the height is zero";
    return {};
}

void OpenSolidSession(SolidArcWorkspaceState& State, SolidArcConstructKind Kind)
{
    if (State.Bevel.ActiveCondition) return;
    const int Sides = State.Drawing.Sides;
    State.Drawing                 = {};
    State.Drawing.ActiveCondition = true;
    State.Drawing.SolidCondition  = true;
    State.Drawing.Solid           = Kind;
    State.Drawing.Kind            = SolidArcDrawingKind::Rectangle;                          // Unused for solids (pointer rules only)
    State.Drawing.GridPlane       = 0;
    State.Drawing.Sides           = Sides;
    State.Drawing.Combination     = GeometryProcessing::FeatureCombination::Separate;
    FRONTIER_TRACE("Construct", std::string("draw ") + SolidArcConstructLabel(Kind));
    Announce(State, std::string(SolidArcConstructLabel(Kind)) + ": " + SolidPrompt(State.Drawing) + " · snaps (Ctrl free) · new body (R) · Q / W / E combine · Tab exact size · Esc aborts");
}

// Creates the drawn solid; the session closes on success, stays open with the reason on a refusal.
void FinishSolidSession(SolidArcWorkspaceState& State)
{
    SolidArcDrawingSession& Drawing = State.Drawing;
    SolidArcConstructRequest Request;
    if (const std::string Refusal = ComposeSolidRequest(Drawing, false, Request); !Refusal.empty())
    {
        Drawing.Refusal = Legible(Refusal);
        Announce(State, Drawing.Refusal);
        return;
    }
    const bool FirstBody = !State.Document.Summary.PresentCondition;
    Request.Combination  = FirstBody && Drawing.Combination != GeometryProcessing::FeatureCombination::Separate ? GeometryProcessing::FeatureCombination::New : Drawing.Combination;
    Request.Target       = Request.Combination == GeometryProcessing::FeatureCombination::Separate || FirstBody ? std::string{} : BooleanTarget(State);
    if (Request.PlaneCondition && Request.Combination == GeometryProcessing::FeatureCombination::Union)
    {
        // The footprint on the tangent plane of a curved face: measured on a new-body copy (a touching Union refuses).
        SolidArcDocument         Probe     = CloneSolidArcDocument(State.Document);
        SolidArcConstructRequest Footprint = Request;
        Footprint.Combination            = GeometryProcessing::FeatureCombination::Separate;
        Footprint.Target.clear();
        const size_t Before = Probe.Tree.Steps.size();
        if (ConstructSolidArcPrimitive(Probe, Footprint).AcceptedCondition)
            for (size_t Index = Before; Index < Probe.Tree.Steps.size(); ++Index)
                if (Probe.Tree.Steps[Index].Kind == FeatureStepKind::Sketch) { Request.Start = -MeasureSolidArcProfileSink(Probe, State.Scene, Probe.Tree.Steps[Index].Label); break; }
    }
    Drawing.ActiveCondition = false;
    State.LastCommand = { SolidArcRepeatKind::Construct, BlendProfile::Fillet, 0.0, FaceOperationKind::Extrude, 0.0, Request };
    const SolidArcCommandOutcome Outcome = ConstructSolidArcPrimitive(State.Document, Request);
    if (Outcome.AcceptedCondition && Request.Combination == GeometryProcessing::FeatureCombination::Separate)
        SelectSolidArcName(State.Document, SolidArcSelectionKind::Body, State.Document.Regeneration.Active, false);
    Conclude(State, Outcome);
}

// Tab: the size form, filled with what is drawn so far (the session closes; Create in the form makes the solid).
void OpenSolidExactForm(SolidArcWorkspaceState& State)
{
    SolidArcDrawingSession& Drawing = State.Drawing;
    SolidArcConstructRequest Request;
    (void)ComposeSolidRequest(Drawing, true, Request);
    SolidArcConstructRequest& Form = State.ConstructForm;
    const SolidArcConstructRequest Defaults;
    Form             = Request;
    Form.Combination = Drawing.Combination;
    if (Form.Width < 1.0e-9) Form.Width = Defaults.Width;
    if (Form.Depth < 1.0e-9) Form.Depth = Defaults.Depth;
    if (std::abs(Form.Height) < 1.0e-9) Form.Height = Defaults.Height;
    if (Form.Radius < 1.0e-9) Form.Radius = Defaults.Radius;
    if (Form.Major < 1.0e-9) Form.Major = Defaults.Major;
    if (Form.Minor < 1.0e-9 || Form.Minor >= Form.Major) Form.Minor = std::min(Defaults.Minor, 0.5 * Form.Major);
    if (Form.Inner < 1.0e-9 || Form.Inner >= Form.Radius) Form.Inner = 0.6 * Form.Radius;
    if (Drawing.Points.empty()) Form.Position = { 0.0, 0.0, 0.0 };
    Drawing.ActiveCondition = false;
    State.ConstructOpen     = true;
}

// Height from the pointer: closest approach to the vertical line through the last click; 0.1 steps (Ctrl free), or
// level with a point snap.
void PlaceSolidLift(SolidArcWorkspaceState& State, const OrbitCameraFrame& Frame, ImVec2 Local)
{
    SolidArcDrawingSession& Drawing = State.Drawing;
    const OrthonormalFrame  Plane   = DrawingPlane(Drawing);
    const Vector3           Anchor  = PlacePlanePoint(Plane, Drawing.Points.back());
    const std::optional<double> Along = ApproachSolidArcLine(CastOrbitRay(Frame, Local.x, Local.y), Anchor, Plane.ZAxis);
    State.Picker.Current = {};
    if (!Along) return;
    double Lift = *Along;
    const SolidArcSnap Object = FindPointerSnap(State, Frame, Local, std::nullopt, {}, {}, false);
    if (PointSnapCondition(Object.Kind))
    {
        Lift = Dot(Object.Position - Anchor, Plane.ZAxis);
        State.Picker.Current = { SolidArcSnapKind::Axis, Anchor + Plane.ZAxis * Lift, Plane.ZAxis, {}, std::string("level with ") + SolidArcSnapLabel(Object.Kind) + " " + Object.Source, 0.0 };
    }
    else if (!ImGui::GetIO().KeyCtrl) Lift = std::round(Lift / DrawingSnap) * DrawingSnap;
    Drawing.Lift = Lift;
}

// Outward normal of a curved scene face at a ray hit: the facet holding the hit (smallest barycentric miss), its three
// exact corner normals blended barycentrically. Empty when the face has no facets.
[[nodiscard]] std::optional<Vector3> SceneNormalAt(const SolidArcScene& Scene, uint32_t Face, Vector3 Hit)
{
    const SolidArcSceneFace& Entry = Scene.Faces[Face];
    double  Best = std::numeric_limits<double>::infinity();
    Vector3 Normal{};
    for (uint32_t Corner = Entry.FirstCorner; Corner + 2 < Entry.FirstCorner + Entry.CornerCount; Corner += 3)
    {
        const auto At = [&](uint32_t Index) { const float* P = Scene.Corners[Index].Position; return Vector3{ P[0], P[1], P[2] }; };
        const auto Up = [&](uint32_t Index) { const float* N = Scene.Corners[Index].Normal; return Vector3{ N[0], N[1], N[2] }; };
        const Vector3 A = At(Corner), B = At(Corner + 1), C = At(Corner + 2);
        const Vector3 Facet = Cross(B - A, C - A);
        const double  Twice = Dot(Facet, Facet);
        if (!(Twice > 0.0)) continue;
        const double Alpha = Dot(Cross(C - B, Hit - B), Facet) / Twice;                      // Barycentric weights of A, B, C
        const double Beta  = Dot(Cross(A - C, Hit - C), Facet) / Twice;
        const double Gamma = 1.0 - Alpha - Beta;
        const double Miss  = std::max({ 0.0, -Alpha, -Beta, -Gamma }) + std::abs(Dot(Hit - A, Facet)) / std::sqrt(Twice);
        if (Miss >= Best) continue;
        Best   = Miss;
        Normal = Up(Corner) * std::clamp(Alpha, 0.0, 1.0) + Up(Corner + 1) * std::clamp(Beta, 0.0, 1.0) + Up(Corner + 2) * std::clamp(Gamma, 0.0, 1.0);
    }
    if (!(Length(Normal) > 0.0)) return std::nullopt;
    return Normalize(Normal);
}

// 📝 Sketch on a face (Plasticity): the body face under the pointer offers its plane — outward normal n̂, x̂ along world x
//    on level faces (else the horizontal n̂-perpendicular), ŷ = n̂ × x̂, origin at the foot of the world origin — so the
//    0.1 grid on the face lines up with the world grid. A planar face gives its exact plane (resolved once per face and
//    document revision); a curved face gives the tangent plane at the point under the pointer (Orient to faces).
bool ResolveHoveredFacePlane(SolidArcWorkspaceState& State, const OrbitCameraFrame& Frame, ImVec2 Local, std::string& Name, OrthonormalFrame& Plane)
{
    const OrbitRay                    Ray  = CastOrbitRay(Frame, Local.x, Local.y);
    const std::optional<SolidArcPick> Pick = PickSolidArcScene(State.Scene, Ray, 0.0, false, ActiveSectionClip(State));
    if (!Pick || Pick->Identity == 0 || Pick->Identity == IdentitySectionCap || (Pick->Identity & IdentityEdgeBit)) return false;
    const uint32_t Index = Pick->Identity - 1u;
    if (Index >= State.Scene.Faces.size()) return false;
    SolidArcDrawingSession& Drawing = State.Drawing;
    const bool Flat = State.Scene.Faces[Index].Surface == "Plane";
    if (!Flat && !State.Picker.FaceOrientEnabled) return false;
    Name = State.Scene.Faces[Index].Name;
    if (Flat && Name == Drawing.FaceName && Drawing.FaceRevision == State.Document.Revision) { Plane = Drawing.FacePlane; return true; }
    Vector3 Normal, Anchor;
    if (Flat)
    {
        const SolidArcJointOrigin Origin = ResolveSolidArcJointOrigin(State.Document, SolidArcSelectionKind::Face, Name);
        if (!Origin.Refusal.empty() || !(Length(Origin.Frame.ZAxis) > 0.0)) return false;
        Normal = Normalize(Origin.Frame.ZAxis);
        Anchor = Origin.Frame.Origin;
    }
    else
    {
        // The exact tangent plane: the facet hit carried to its foot on the face's surface, the surface normal there (the
        // blended facet normal only when the inversion fails). A facet hit alone sits up to the sag inside the surface.
        Anchor = Vector3{ Ray.Origin[0], Ray.Origin[1], Ray.Origin[2] } + Vector3{ Ray.Direction[0], Ray.Direction[1], Ray.Direction[2] } * Pick->Distance;
        const SolidArcFaceFoot Foot = ProjectOntoSolidArcFace(State.Document, Name, Anchor);
        if (Foot.FoundCondition && Length(Foot.Position - Anchor) < 4.0 * std::max(State.Scene.Sag, 1.0e-3))
        {
            Anchor = Foot.Position;
            Normal = Normalize(Foot.Normal);
        }
        else
        {
            const std::optional<Vector3> Tangent = SceneNormalAt(State.Scene, Index, Anchor);
            if (!Tangent) return false;
            Normal = *Tangent;
        }
    }
    Vector3 XAxis = { 1.0, 0.0, 0.0 };
    if (std::abs(Normal.Z) > 1.0 - 1.0e-12) Normal = { 0.0, 0.0, Normal.Z > 0.0 ? 1.0 : -1.0 };   // Level face
    else XAxis = Normalize(Cross(Vector3{ 0.0, 0.0, 1.0 }, Normal));
    Plane = { Normal * Dot(Anchor, Normal), XAxis, Cross(Normal, XAxis), Normal };
    Drawing.FaceName     = Flat ? Name : std::string();                                      // A tangent plane is never reused
    Drawing.FacePlane    = Plane;
    Drawing.FaceRevision = State.Document.Revision;
    if (!Flat) Name = State.Scene.Faces[Index].Name;
    return true;
}

// Before the first click of a sketch: the drawing plane follows the planar face under the pointer; off a face, a Ctrl
// snap keeps the face plane through it (else a grid-parallel plane through the snapped point); otherwise the grid plane.
void FollowFacePlane(SolidArcWorkspaceState& State, const OrbitCameraFrame& Frame, ImVec2 Local)
{
    SolidArcDrawingSession& Drawing = State.Drawing;
    if (Drawing.SolidCondition || !Drawing.Points.empty() || State.Picker.Lock != SolidArcSnapLockKind::None) return;
    std::string      Name;
    OrthonormalFrame Plane;
    if (State.Picker.FaceOrientEnabled && ResolveHoveredFacePlane(State, Frame, Local, Name, Plane))
    {
        Drawing.FacePlaneCondition = true;
        Drawing.FacePlane          = Plane;
        Drawing.FaceName           = Name;
        return;
    }
    const SolidArcSnap Object = FindPointerSnap(State, Frame, Local, std::nullopt, {}, {}, false);
    if (PointSnapCondition(Object.Kind))
    {
        const Vector3 Position = Object.Position;
        if (Drawing.FacePlaneCondition && std::abs(Dot(Position - Drawing.FacePlane.Origin, Drawing.FacePlane.ZAxis)) <= 1.0e-9 * (1.0 + Length(Position))) return;
        Plane = SolidArcSketchPlane(Drawing.GridPlane);
        Plane.Origin = Plane.Origin + Plane.ZAxis * Dot(Position - Plane.Origin, Plane.ZAxis);   // Level with the snapped point
        Drawing.FacePlaneCondition = std::abs(Dot(Position - SolidArcSketchPlane(Drawing.GridPlane).Origin, Plane.ZAxis)) > 1.0e-12;
        Drawing.FacePlane          = Plane;
        Drawing.FaceName.clear();                                                            // No face: the cache no longer holds
        return;
    }
    Drawing.FacePlaneCondition = false;
}

// Before the first click of a solid: Ctrl snaps onto an important point (the plane height there); else a level body
// face under the pointer is the floor, and the grid still holds on it.
bool PlaceSolidStart(SolidArcWorkspaceState& State, const OrbitCameraFrame& Frame, ImVec2 Local, Vector2& Point)
{
    SolidArcDrawingSession& Drawing = State.Drawing;
    const SolidArcSnap Object = FindPointerSnap(State, Frame, Local, std::nullopt, {}, {}, false);
    State.Picker.Current       = {};
    Drawing.FacePlaneCondition = false;
    if (PointSnapCondition(Object.Kind))
    {
        State.Picker.Current     = Object;
        Point                    = { Object.Position.X, Object.Position.Y };
        Drawing.PointerElevation = Object.Position.Z;
        return true;
    }
    std::string      Name;
    OrthonormalFrame Plane;
    if (!ResolveHoveredFacePlane(State, Frame, Local, Name, Plane)) return false;
    const bool Floor = Plane.ZAxis.Z > 0.999;                                                // An upward flat face: the floor rises there
    if (!Floor && !State.Picker.FaceOrientEnabled) return false;
    Vector2 Flat;
    if (!IntersectSketchPlane(Frame, Local, Plane, Flat)) return false;
    if (!Floor)
    {
        // 📝 Any other face (upright, tilted, curved: its tangent plane) carries the solid: its clicks are face coordinates,
        //    the height runs along the outward normal and the tile is built in that frame (Orient to faces).
        Point = { std::round(Flat.X / DrawingSnap) * DrawingSnap, std::round(Flat.Y / DrawingSnap) * DrawingSnap };
        Drawing.FacePlaneCondition    = true;
        Drawing.FacePlane             = Plane;
        Drawing.FaceName              = Name;
        State.Picker.Current.Kind     = SolidArcSnapKind::Face;
        State.Picker.Current.Position = PlacePlanePoint(Plane, Point);
        State.Picker.Current.Normal   = Plane.ZAxis;
        State.Picker.Current.Source   = "Face " + Name;
        Drawing.PointerElevation      = 0.0;
        return true;
    }
    Point = { std::round(Flat.X / DrawingSnap) * DrawingSnap, std::round(Flat.Y / DrawingSnap) * DrawingSnap };
    State.Picker.Current.Kind     = SolidArcSnapKind::Face;
    State.Picker.Current.Position = { Point.X, Point.Y, Plane.Origin.Z };
    State.Picker.Current.Normal   = Plane.ZAxis;
    State.Picker.Current.Source   = "Face " + Name;
    Drawing.PointerElevation      = Plane.Origin.Z;
    return true;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    LIVE DIMENSIONS
//------------------------------------------------------------------------------------------------------------------------

// 📝 Dimension lines while drawing (SolidArc live reference drawLiveDims): a white line with arrowheads between two world
//    points and its value in a dark pill, offset in pixels; angles as an arc with the degrees beside it.
constexpr ImU32 DimensionInk   = IM_COL32(0xff, 0xff, 0xff, 0xc0);                         // rgba(255,255,255,.75)
constexpr ImU32 DimensionHead  = IM_COL32(0xff, 0xff, 0xff, 0xd9);                         // rgba(255,255,255,.85)
constexpr ImU32 DimensionPill  = IM_COL32(0x00, 0x00, 0x00, 0xb8);                         // rgba(0,0,0,.72)
constexpr ImU32 AngleInk       = IM_COL32(0xff, 0xd2, 0x78, 0xe6);                         // rgba(255,210,120,.9)
constexpr ImU32 ConstructionInk = IM_COL32(0xff, 0xff, 0xff, 0x5a);                        // rgba(255,255,255,.35)

void DrawDimensionPill(ImDrawList* Canvas, ImVec2 Centre, const std::string& Text, ImU32 Ink = IM_COL32_WHITE)
{
    const ImVec2 Size = ImGui::CalcTextSize(Text.c_str());
    const float  Half = 0.5f * Size.x + 6.0f;
    Canvas->AddRectFilled(ImVec2(Centre.x - Half, Centre.y - 9.0f), ImVec2(Centre.x + Half, Centre.y + 9.0f), DimensionPill, 9.0f);
    Canvas->AddText(ImVec2(Centre.x - 0.5f * Size.x, Centre.y - 0.5f * Size.y), Ink, Text.c_str());
}

void DrawDimensionLine(const SolidArcWorkspaceState& State, const OrbitCameraFrame& Frame, ImDrawList* Canvas, Vector3 From, Vector3 To, const std::string& Text,
                       ImVec2 Offset = ImVec2(0.0f, 0.0f))
{
    ImVec2 Start, End;
    if (!ProjectWorldPoint(State, Frame, From, Start) || !ProjectWorldPoint(State, Frame, To, End)) return;
    Canvas->AddLine(Start, End, DimensionInk, 1.0f);
    const float Span = std::hypot(End.x - Start.x, End.y - Start.y);
    if (Span > 16.0f)
    {
        const float AlongX = (End.x - Start.x) / Span, AlongY = (End.y - Start.y) / Span;
        for (const auto& [Tip, Sense] : { std::pair<ImVec2, float>{ Start, 1.0f }, std::pair<ImVec2, float>{ End, -1.0f } })
            Canvas->AddTriangleFilled(Tip, ImVec2(Tip.x + Sense * AlongX * 7.0f - AlongY * 3.0f, Tip.y + Sense * AlongY * 7.0f + AlongX * 3.0f),
                                      ImVec2(Tip.x + Sense * AlongX * 7.0f + AlongY * 3.0f, Tip.y + Sense * AlongY * 7.0f - AlongX * 3.0f), DimensionHead);
    }
    DrawDimensionPill(Canvas, ImVec2(0.5f * (Start.x + End.x) + Offset.x, 0.5f * (Start.y + End.y) + Offset.y), Text);
}

// Arc at Vertex from ray A to ray B (plane coordinates, the shorter way) with its angle in degrees.
void DrawAngleDimension(const SolidArcWorkspaceState& State, const OrbitCameraFrame& Frame, ImDrawList* Canvas, const OrthonormalFrame& Plane, Vector2 Vertex, Vector2 A, Vector2 B)
{
    const double From  = std::atan2(A.Y - Vertex.Y, A.X - Vertex.X);
    double       Sweep = std::atan2(B.Y - Vertex.Y, B.X - Vertex.X) - From;
    while (Sweep > std::numbers::pi) Sweep -= 2.0 * std::numbers::pi;
    while (Sweep < -std::numbers::pi) Sweep += 2.0 * std::numbers::pi;
    const double Radius = 0.35 * std::min(Distance(A, Vertex), Distance(B, Vertex));
    if (Radius <= 0.0) return;
    std::vector<ImVec2> Pixels;
    for (int Index = 0; Index <= 16; ++Index)
    {
        const double θ = From + Sweep * Index / 16.0;
        ImVec2 Pixel;
        if (ProjectWorldPoint(State, Frame, PlacePlanePoint(Plane, { Vertex.X + std::cos(θ) * Radius, Vertex.Y + std::sin(θ) * Radius }), Pixel)) Pixels.push_back(Pixel);
    }
    if (Pixels.size() >= 2) Canvas->AddPolyline(Pixels.data(), int(Pixels.size()), AngleInk, 0, 1.2f);
    const double Middle = From + 0.5 * Sweep;
    ImVec2 Label;
    if (ProjectWorldPoint(State, Frame, PlacePlanePoint(Plane, { Vertex.X + std::cos(Middle) * Radius * 1.7, Vertex.Y + std::sin(Middle) * Radius * 1.7 }), Label))
        DrawDimensionPill(Canvas, Label, Compose("%.1f°", std::abs(Sweep) * 180.0 / std::numbers::pi), AngleInk);
}

void DrawConstructionLine(const SolidArcWorkspaceState& State, const OrbitCameraFrame& Frame, ImDrawList* Canvas, Vector3 From, Vector3 To)
{
    ImVec2 Start, End;
    if (!ProjectWorldPoint(State, Frame, From, Start) || !ProjectWorldPoint(State, Frame, To, End)) return;
    const float Span = std::hypot(End.x - Start.x, End.y - Start.y);
    for (float Travel = 0.0f; Travel < Span; Travel += 7.0f)
    {
        const float First = Travel / Span, Last = std::min(Travel + 3.0f, Span) / Span;
        Canvas->AddLine(ImVec2(Start.x + (End.x - Start.x) * First, Start.y + (End.y - Start.y) * First),
                        ImVec2(Start.x + (End.x - Start.x) * Last, Start.y + (End.y - Start.y) * Last), ConstructionInk, 1.0f);
    }
}

[[nodiscard]] std::string DimensionText(const char* Tag, double Value)
{
    return Tag[0] ? Compose("%s %.3f", Tag, Value) : Compose("%.3f", Value);
}

// Live dimensions of a 2-D drawing session (clicks + pointer).
void DrawSketchDimensions(const SolidArcWorkspaceState& State, const OrbitCameraFrame& Frame, ImDrawList* Canvas)
{
    const SolidArcDrawingSession& Drawing = State.Drawing;
    const OrthonormalFrame        Plane   = DrawingPlane(Drawing);
    std::vector<Vector2> Points = Drawing.Points;
    if (Drawing.PointerCondition) Points.push_back(Drawing.Pointer);
    if (Points.size() < 2) return;
    auto World = [&](Vector2 Point) { return PlacePlanePoint(Plane, Point); };
    const Vector2 First = Points[0], Second = Points[1];
    switch (Drawing.Kind)
    {
    case SolidArcDrawingKind::Line:
    case SolidArcDrawingKind::Curve:
    {
        const Vector2 From = Points[Points.size() - 2], To = Points.back();
        if (Distance(From, To) < 1.0e-12) return;
        DrawDimensionLine(State, Frame, Canvas, World(From), World(To), DimensionText("", Distance(From, To)), ImVec2(0.0f, -14.0f));
        if (Drawing.Kind == SolidArcDrawingKind::Curve) return;
        if (Points.size() >= 3) DrawAngleDimension(State, Frame, Canvas, Plane, From, Points[Points.size() - 3], To);
        else
        {
            const Vector2 Reference{ From.X + std::max(0.5 * Distance(From, To), 0.2), From.Y };
            DrawConstructionLine(State, Frame, Canvas, World(From), World(Reference));
            DrawAngleDimension(State, Frame, Canvas, Plane, From, Reference, To);
        }
        return;
    }
    case SolidArcDrawingKind::Rectangle:
    {
        const Vector2 Corner = Drawing.CentreCondition ? First * 2.0 - Second : First;
        const Vector2 Low{ std::min(Corner.X, Second.X), std::min(Corner.Y, Second.Y) }, High{ std::max(Corner.X, Second.X), std::max(Corner.Y, Second.Y) };
        const double  Gap = 0.08 * std::max(High.X - Low.X, High.Y - Low.Y) + 0.02;
        DrawDimensionLine(State, Frame, Canvas, World({ Low.X, Low.Y - Gap }), World({ High.X, Low.Y - Gap }), DimensionText("W", High.X - Low.X));
        DrawDimensionLine(State, Frame, Canvas, World({ High.X + Gap, Low.Y }), World({ High.X + Gap, High.Y }), DimensionText("H", High.Y - Low.Y));
        return;
    }
    case SolidArcDrawingKind::Circle:
    {
        const Vector2 Centre = Drawing.CentreCondition ? First : (First + Second) * 0.5;
        const double  Radius = Drawing.CentreCondition ? Distance(First, Second) : 0.5 * Distance(First, Second);
        if (Radius < 1.0e-12) return;
        DrawDimensionLine(State, Frame, Canvas, World(Centre), World(Second), DimensionText("R", Radius), ImVec2(0.0f, -14.0f));
        const Vector2 Diagonal{ Radius * 0.7071, -Radius * 0.7071 };
        DrawDimensionLine(State, Frame, Canvas, World(Centre - Diagonal), World(Centre + Diagonal), DimensionText("Ø", 2.0 * Radius), ImVec2(0.0f, 14.0f));
        return;
    }
    case SolidArcDrawingKind::Arc:
    {
        DrawDimensionLine(State, Frame, Canvas, World(First), World(Second), DimensionText("chord", Distance(First, Second)), ImVec2(0.0f, 14.0f));
        if (Points.size() < 3) return;
        SketchModel Sketch;
        SolidArcDrawingRequest Request;
        Request.Kind = SolidArcDrawingKind::Arc;
        Request.Points = { First, Second, Points[2] };
        if (!ComposeSolidArcSketch(Request, Sketch).empty() || Sketch.Points.empty()) return;
        const Vector2 Centre = Sketch.Points[0].Position;
        DrawDimensionLine(State, Frame, Canvas, World(Centre), World(Points[2]), DimensionText("R", Distance(Centre, Points[2])), ImVec2(0.0f, -14.0f));
        return;
    }
    case SolidArcDrawingKind::Slot:
    {
        DrawDimensionLine(State, Frame, Canvas, World(First), World(Second), DimensionText("L", Distance(First, Second)), ImVec2(0.0f, -14.0f));
        if (Points.size() < 3 || Distance(First, Second) < 1.0e-12) return;
        const Vector2 Along = (Second - First) * (1.0 / Distance(First, Second));
        const Vector2 Left{ -Along.Y, Along.X };
        const double  Radius = std::abs((Points[2].X - Second.X) * Left.X + (Points[2].Y - Second.Y) * Left.Y);
        DrawDimensionLine(State, Frame, Canvas, World(Second), World(Second + Left * Radius), DimensionText("R", Radius), ImVec2(14.0f, 0.0f));
        return;
    }
    case SolidArcDrawingKind::Polygon:
    {
        const int    Sides  = std::clamp(Drawing.Sides, 3, 64);
        const double Radius = Distance(First, Second);
        if (Radius < 1.0e-12) return;
        const double  Turn = std::atan2(Second.Y - First.Y, Second.X - First.X);
        const Vector2 Next{ First.X + Radius * std::cos(Turn + 2.0 * std::numbers::pi / Sides), First.Y + Radius * std::sin(Turn + 2.0 * std::numbers::pi / Sides) };
        DrawConstructionLine(State, Frame, Canvas, World(First), World(Second));
        DrawConstructionLine(State, Frame, Canvas, World(First), World(Next));
        DrawAngleDimension(State, Frame, Canvas, Plane, First, Second, Next);
        DrawDimensionLine(State, Frame, Canvas, World(First), World(Second), DimensionText("R", Radius), ImVec2(0.0f, -14.0f));
        DrawDimensionLine(State, Frame, Canvas, World(Second), World(Next), DimensionText("", Distance(Second, Next)), ImVec2(0.0f, 14.0f));
        ImVec2 Centre;
        if (ProjectWorldPoint(State, Frame, World(First), Centre))
            DrawDimensionPill(Canvas, ImVec2(Centre.x, Centre.y + 24.0f), Compose("%d sides · %.1f° interior", Sides, 180.0 * (Sides - 2) / Sides));
        return;
    }
    }
}

// Wireframe of the solid being drawn and its dimension lines (width, depth, height, radii).
void DrawSolidPreview(SolidArcWorkspaceState& State, ImDrawList* Canvas, const OrbitCameraFrame& Frame)
{
    const SolidArcDrawingSession& Drawing = State.Drawing;
    const OrthonormalFrame        Plane   = DrawingPlane(Drawing);
    SolidArcConstructRequest Request;
    const std::string Refusal = ComposeSolidRequest(Drawing, true, Request);
    const bool Lifted = Drawing.Points.size() >= SolidPlaneClicks(Drawing.Solid);
    // Request coordinates are world ones on the floor; on a body face they are face-frame ones, carried into the world.
    const auto World = [&](Vector3 Point) { return Request.PlaneCondition ? PlacePlanePoint(Request.Plane, { Point.X, Point.Y }) + Request.Plane.ZAxis * Point.Z : Point; };
    auto Pixel    = [&](Vector3 Point, ImVec2& Out) { return ProjectWorldPoint(State, Frame, World(Point), Out); };
    auto RawPixel = [&](Vector3 Point, ImVec2& Out) { return ProjectWorldPoint(State, Frame, Point, Out); };   // Already a world point
    const auto DrawDimensionLine = [&](const SolidArcWorkspaceState& Workspace, const OrbitCameraFrame& Camera, ImDrawList* List, Vector3 From, Vector3 To, const std::string& Text,
                                       ImVec2 Offset = ImVec2(0.0f, 0.0f)) { ::Frontier::SolidArc::DrawDimensionLine(Workspace, Camera, List, World(From), World(To), Text, Offset); };
    auto Stroke = [&](const std::vector<Vector3>& Chain, bool Closed, ImU32 Ink, float Width)
    {
        std::vector<ImVec2> Pixels;
        for (const Vector3& Point : Chain) { ImVec2 Out; if (Pixel(Point, Out)) Pixels.push_back(Out); }
        if (Pixels.size() >= 2) Canvas->AddPolyline(Pixels.data(), int(Pixels.size()), Ink, Closed ? ImDrawFlags_Closed : 0, Width);
    };
    auto Ring = [&](Vector3 Centre, double Radius, int Count, double Turn)
    {
        std::vector<Vector3> Chain;
        for (int Index = 0; Index < Count; ++Index)
        {
            const double θ = Turn + 2.0 * std::numbers::pi * Index / Count;
            Chain.push_back({ Centre.X + Radius * std::cos(θ), Centre.Y + Radius * std::sin(θ), Centre.Z });
        }
        return Chain;
    };
    const ImU32 Ink = ActiveColour;
    std::vector<Vector2> Marks = Drawing.Points;
    if (Drawing.PointerCondition && Drawing.Points.size() < SolidPlaneClicks(Drawing.Solid)) Marks.push_back(Drawing.Pointer);
    const bool Shaped = Marks.size() >= 2 && (Refusal.empty() || Request.Radius > 0.0 || Request.Width > 0.0 || Request.Major > 0.0);
    if (Shaped)
    {
        const Vector3 Spot = Request.Position;
        switch (Drawing.Solid)
        {
        case SolidArcConstructKind::Block:
        case SolidArcConstructKind::Wedge:
        {
            const double X0 = Spot.X, X1 = Spot.X + Request.Width, Y0 = Spot.Y, Y1 = Spot.Y + Request.Depth, Z0 = Spot.Z, Z1 = Spot.Z + Request.Height;
            const bool   Wedge = Drawing.Solid == SolidArcConstructKind::Wedge;
            Stroke({ { X0, Y0, Z0 }, { X1, Y0, Z0 }, { X1, Y1, Z0 }, { X0, Y1, Z0 } }, true, Ink, 2.0f);
            if (Request.Height > 0.0)
            {
                if (Wedge)
                {
                    Stroke({ { X0, Y0, Z1 }, { X0, Y1, Z1 } }, false, Ink, 2.0f);
                    for (const double Y : { Y0, Y1 }) Stroke({ { X0, Y, Z0 }, { X0, Y, Z1 }, { X1, Y, Z0 } }, false, Ink, 2.0f);
                }
                else
                {
                    Stroke({ { X0, Y0, Z1 }, { X1, Y0, Z1 }, { X1, Y1, Z1 }, { X0, Y1, Z1 } }, true, Ink, 2.0f);
                    for (const double X : { X0, X1 }) for (const double Y : { Y0, Y1 }) Stroke({ { X, Y, Z0 }, { X, Y, Z1 } }, false, Ink, 2.0f);
                }
            }
            const double Gap = 0.08 * std::max(Request.Width, Request.Depth) + 0.02;
            DrawDimensionLine(State, Frame, Canvas, { X0, Y0 - Gap, Z0 }, { X1, Y0 - Gap, Z0 }, DimensionText("W", Request.Width));
            DrawDimensionLine(State, Frame, Canvas, { X1 + Gap, Y0, Z0 }, { X1 + Gap, Y1, Z0 }, DimensionText("D", Request.Depth));
            if (Lifted) DrawDimensionLine(State, Frame, Canvas, { X0 - Gap, Y0 - Gap, Z0 }, { X0 - Gap, Y0 - Gap, Z0 + Request.Height }, DimensionText("H", Request.Height), ImVec2(-18.0f, 0.0f));
            break;
        }
        case SolidArcConstructKind::Cylinder:
        case SolidArcConstructKind::Prism:
        case SolidArcConstructKind::Cone:
        case SolidArcConstructKind::Tube:
        {
            const bool   Prism = Drawing.Solid == SolidArcConstructKind::Prism;
            const bool   Cone  = Drawing.Solid == SolidArcConstructKind::Cone;
            const int    Count = Prism ? std::clamp(Drawing.Sides, 3, 64) : 72;
            const double Turn  = Prism ? Request.Angle : 0.0;
            const double Top   = Spot.Z + Request.Height;
            Stroke(Ring(Spot, Request.Radius, Count, Turn), true, Ink, 2.0f);
            if (Drawing.Solid == SolidArcConstructKind::Tube && Request.Inner > 0.0) Stroke(Ring(Spot, Request.Inner, 72, 0.0), true, Ink, 2.0f);
            if (Lifted && std::abs(Request.Height) > 0.0)
            {
                const Vector3 Lid{ Spot.X, Spot.Y, Top };
                if (Cone)
                {
                    for (int Index = 0; Index < 4; ++Index)
                    {
                        const double θ = 0.5 * std::numbers::pi * Index;
                        Stroke({ { Spot.X + Request.Radius * std::cos(θ), Spot.Y + Request.Radius * std::sin(θ), Spot.Z }, Lid }, false, Ink, 2.0f);
                    }
                }
                else
                {
                    Stroke(Ring(Lid, Request.Radius, Count, Turn), true, Ink, 2.0f);
                    if (Drawing.Solid == SolidArcConstructKind::Tube && Request.Inner > 0.0) Stroke(Ring(Lid, Request.Inner, 72, 0.0), true, Ink, 2.0f);
                    const int Posts = Prism ? Count : 4;
                    for (int Index = 0; Index < Posts; ++Index)
                    {
                        const double θ = Turn + 2.0 * std::numbers::pi * Index / Posts;
                        const Vector3 Foot{ Spot.X + Request.Radius * std::cos(θ), Spot.Y + Request.Radius * std::sin(θ), Spot.Z };
                        Stroke({ Foot, { Foot.X, Foot.Y, Top } }, false, Ink, 2.0f);
                    }
                }
            }
            const Vector3 Rim{ Spot.X + Request.Radius * std::cos(Turn), Spot.Y + Request.Radius * std::sin(Turn), Spot.Z };
            DrawDimensionLine(State, Frame, Canvas, Spot, Rim, DimensionText("R", Request.Radius), ImVec2(0.0f, -14.0f));
            if (Drawing.Solid == SolidArcConstructKind::Tube && Request.Inner > 0.0)
                DrawDimensionLine(State, Frame, Canvas, Spot, { Spot.X - Request.Inner, Spot.Y, Spot.Z }, DimensionText("r", Request.Inner), ImVec2(0.0f, 14.0f));
            else if (!Prism)
            {
                // Diameter outside the base ring (on the far side from the radius line), so the pills never overlap.
                const double Out = Request.Radius * 1.25 + 0.05;
                DrawConstructionLine(State, Frame, Canvas, World({ Spot.X - Request.Radius, Spot.Y, Spot.Z }), World({ Spot.X - Request.Radius, Spot.Y - Out, Spot.Z }));
                DrawConstructionLine(State, Frame, Canvas, World({ Spot.X + Request.Radius, Spot.Y, Spot.Z }), World({ Spot.X + Request.Radius, Spot.Y - Out, Spot.Z }));
                DrawDimensionLine(State, Frame, Canvas, { Spot.X - Request.Radius, Spot.Y - Out, Spot.Z }, { Spot.X + Request.Radius, Spot.Y - Out, Spot.Z },
                                  DimensionText("Ø", 2.0 * Request.Radius));
            }
            if (Prism)
            {
                const double  Next = Turn + 2.0 * std::numbers::pi / Count;
                const Vector3 Corner{ Spot.X + Request.Radius * std::cos(Next), Spot.Y + Request.Radius * std::sin(Next), Spot.Z };
                DrawDimensionLine(State, Frame, Canvas, Rim, Corner, Compose("%d × %.3f", Count, Length(Corner - Rim)), ImVec2(0.0f, 14.0f));
            }
            if (Lifted)
            {
                const double  Side = Request.Radius + 0.08 * Request.Radius + 0.02;
                const Vector3 Foot{ Spot.X + Side * std::cos(Turn + 0.5 * std::numbers::pi), Spot.Y + Side * std::sin(Turn + 0.5 * std::numbers::pi), Spot.Z };
                DrawDimensionLine(State, Frame, Canvas, Foot, { Foot.X, Foot.Y, Top }, DimensionText("H", std::abs(Request.Height)), ImVec2(18.0f, 0.0f));
            }
            break;
        }
        case SolidArcConstructKind::Sphere:
        {
            const double R = Request.Radius;
            Stroke(Ring(Spot, R, 72, 0.0), true, Ink, 2.0f);
            std::vector<Vector3> Meridian, Cross;
            for (int Index = 0; Index < 72; ++Index)
            {
                const double θ = 2.0 * std::numbers::pi * Index / 72;
                Meridian.push_back({ Spot.X + R * std::cos(θ), Spot.Y, Spot.Z + R * std::sin(θ) });
                Cross.push_back({ Spot.X, Spot.Y + R * std::cos(θ), Spot.Z + R * std::sin(θ) });
            }
            Stroke(Meridian, true, Ink, 1.5f);
            Stroke(Cross, true, Ink, 1.5f);
            const Vector2 Rim = Marks[1];
            DrawDimensionLine(State, Frame, Canvas, Spot, { Rim.X, Rim.Y, Spot.Z }, DimensionText("R", R), ImVec2(0.0f, -14.0f));
            break;
        }
        case SolidArcConstructKind::Torus:
        {
            const double Major = Request.Major, Minor = Request.Minor;
            Stroke(Ring(Spot, Major, 96, 0.0), true, ConstructionInk, 1.0f);
            if (Minor > 0.0)
            {
                Stroke(Ring(Spot, Major + Minor, 96, 0.0), true, Ink, 2.0f);
                Stroke(Ring(Spot, Major - Minor, 96, 0.0), true, Ink, 2.0f);
                for (const double Sense : { 1.0, -1.0 })
                {
                    std::vector<Vector3> Tube;
                    for (int Index = 0; Index < 48; ++Index)
                    {
                        const double θ = 2.0 * std::numbers::pi * Index / 48;
                        Tube.push_back({ Spot.X + Sense * (Major + Minor * std::cos(θ)), Spot.Y, Spot.Z + Minor * std::sin(θ) });
                    }
                    Stroke(Tube, true, Ink, 1.5f);
                }
                DrawDimensionLine(State, Frame, Canvas, { Spot.X - Major, Spot.Y, Spot.Z }, { Spot.X - Major - Minor, Spot.Y, Spot.Z }, DimensionText("r", Minor), ImVec2(0.0f, 14.0f));
            }
            DrawDimensionLine(State, Frame, Canvas, Spot, { Spot.X + Major, Spot.Y, Spot.Z }, DimensionText("R", Major), ImVec2(0.0f, -14.0f));
            break;
        }
        }
    }
    for (size_t Index = 0; Index < Marks.size(); ++Index)
    {
        ImVec2 Out;
        if (!RawPixel(PlacePlanePoint(Plane, Marks[Index]), Out)) continue;
        const bool Live = Index == Drawing.Points.size();
        Canvas->AddCircleFilled(Out, Live ? 4.5f : 3.5f, Live ? IM_COL32(0xff, 0xff, 0xff, 0xff) : ActiveColour);
    }
    if (Lifted && !Drawing.Points.empty())
    {
        // The height line the pointer slides on.
        const Vector3 Anchor = PlacePlanePoint(Plane, Drawing.Points.back());
        DrawConstructionLine(State, Frame, Canvas, Anchor, Anchor + Plane.ZAxis * Drawing.Lift);
        ImVec2 Out;
        if (RawPixel(Anchor + Plane.ZAxis * Drawing.Lift, Out)) Canvas->AddCircleFilled(Out, 4.5f, IM_COL32_WHITE);
    }
}

// Plasticity E on a sketch region: the focused sketch step's profile, pointer-adjusted distance, Q/W/E/R Boolean.
void OpenSketchExtrudeSession(SolidArcWorkspaceState& State)
{
    const std::string Label = State.FocusedStep;
    const double Remembered = State.Bevel.Verb == SolidArcSessionVerb::Profile && State.Bevel.Size != 0.0 ? State.Bevel.Size : 0.5;
    State.Bevel                 = {};
    State.Bevel.ActiveCondition = true;
    State.Bevel.Verb            = SolidArcSessionVerb::Profile;
    State.Bevel.ProfileLabel    = Label;
    State.Bevel.ProfileSink     = MeasureSolidArcProfileSink(State.Document, State.Scene, Label);
    State.Bevel.Combination     = State.Document.Summary.PresentCondition ? FeatureCombination::Union : FeatureCombination::New;
    State.Bevel.Size            = Remembered;
    WriteBevelText(State.Bevel);
    const auto Caption = State.Document.Captions.find(Label);
    Announce(State, "Extrude " + (Caption != State.Document.Captions.end() ? Caption->second : Label) +
                    " (E): move horizontally · Q union · W difference · E intersect · R new body · Enter / right-click applies · Esc aborts");
}

[[nodiscard]] bool FocusedSketchCondition(const SolidArcWorkspaceState& State)
{
    if (State.FocusedStep.empty()) return false;
    for (const FeatureStep& Step : State.Document.Tree.Steps) if (Step.Label == State.FocusedStep) return Step.Kind == FeatureStepKind::Sketch;
    return false;
}

// Plasticity Q (command:boolean): needs two or more bodies picked in mode 4 (first = target, rest = tools).
void OpenBooleanSession(SolidArcWorkspaceState& State)
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
                            SolidArcBodyCaption(State.Document, State.Document.Selection.Bodies.front()).c_str(), int(Picked) - 1));
}

void ChooseBooleanCombination(SolidArcWorkspaceState& State, FeatureCombination Combination)
{
    State.Boolean.Combination = Combination;
    Announce(State, std::string("Boolean: ") + FeatureCombinationLabel(Combination));
}

void ApplyBooleanSession(SolidArcWorkspaceState& State)
{
    const std::vector<std::string> Picked = State.Document.Selection.Bodies;
    State.Boolean.ActiveCondition = false;
    if (Picked.size() < 2) { Announce(State, "Boolean: pick two or more bodies"); return; }
    const SolidArcCommandOutcome Outcome = CombineSolidArcBodies(State.Document, Picked.front(), std::vector<std::string>(Picked.begin() + 1, Picked.end()),
                                                                 State.Boolean.Combination);
    if (Outcome.AcceptedCondition)
    {
        ++State.Boolean.Commits;
        SelectSolidArcName(State.Document, SolidArcSelectionKind::Body, Picked.front(), false);
    }
    Conclude(State, Outcome);
}

// 💡 Plasticity C (command:cut): splits the picked body (mode 4; else the active body) by the focused sketch step or the
//    selected curve, a curve being projected along the view axis snapped to x / y / z (character line from the Top
//    view). The inside piece comes out picked in mode 4, so X deletes it right away (the loft → C → X panel flow).
void ApplyCut(SolidArcWorkspaceState& State)
{
    SolidArcCutRequest Request;
    if (!State.Document.Selection.Bodies.empty()) Request.Target = State.Document.Selection.Bodies.front();
    if (FocusedSketchCondition(State)) Request.Cutter = State.FocusedStep;
    else if (!State.Document.Selection.Curves.empty()) Request.Cutter = State.Document.Selection.Curves.front();
    else
    {
        Announce(State, "Cut (C): focus a sketch step or select a curve (the cutter), pick the body in mode 4 (else the active body)");
        return;
    }
    const OrbitCameraFrame Frame = ViewportFrame(State);
    int Axis = 0;
    for (int Index = 1; Index < 3; ++Index) if (std::fabs(Frame.Forward[Index]) > std::fabs(Frame.Forward[Axis])) Axis = Index;
    const double Sign = Frame.Forward[Axis] < 0.0 ? -1.0 : 1.0;
    Request.Direction = { Axis == 0 ? Sign : 0.0, Axis == 1 ? Sign : 0.0, Axis == 2 ? Sign : 0.0 };
    const SolidArcCommandOutcome Outcome = CutSolidArcBody(State.Document, Request);
    if (Outcome.AcceptedCondition)
    {
        State.FocusedStep = {};
        State.PickMode    = SolidArcPickMode::Solid;
        SelectSolidArcName(State.Document, SolidArcSelectionKind::Body, State.Document.Tree.Steps.back().Label, false);
    }
    Conclude(State, Outcome);
}

[[nodiscard]] OrthonormalFrame ElevatedSketchPlane(const SolidArcDocument& Document, const FeatureStep& Step)
{
    OrthonormalFrame Plane = Step.Plane;
    Plane.Origin = Plane.Origin + Plane.ZAxis * EvaluateFeatureValue(Document.Tree, Step.Elevation);
    return Plane;
}

// The sketch step called Label (nullptr: none).
[[nodiscard]] const FeatureStep* FindSketchStepByLabel(const SolidArcDocument& Document, const std::string& Label)
{
    for (const FeatureStep& Step : Document.Tree.Steps)
        if (Step.Label == Label && Step.Kind == FeatureStepKind::Sketch) return &Step;
    return nullptr;
}

// A sketch as the regeneration solved it (as authored when it did not solve).
[[nodiscard]] const SketchModel& SolvedSketchOf(const SolidArcDocument& Document, const FeatureStep& Step)
{
    const auto Solved = Document.Regeneration.Sketches.find(Step.Label);
    return Solved != Document.Regeneration.Sketches.end() ? Solved->second : Step.Sketch;
}

// "SketchPointSketch3.1": the script widget of a sketch point.
[[nodiscard]] std::string SketchPointWidget(const std::string& Label, int Point)
{
    std::string Name = "SketchPoint";
    for (char Letter : Label)
        if (Letter != '#') Name += Letter;
    return Name + "." + std::to_string(Point);
}

// The document the viewport overlays show: the staged preview while a gizmo or sketch-point drag runs.
[[nodiscard]] const SolidArcDocument& DisplayedSolidArcDocument(const SolidArcWorkspaceState& State)
{
    if (State.SketchPoints.DragCondition && State.SketchPoints.Preview && State.SketchPoints.Refusal.empty()) return *State.SketchPoints.Preview;
    if (State.Gizmo.DragCondition && State.Gizmo.Preview && State.Gizmo.Refusal.empty()) return *State.Gizmo.Preview;
    return State.Document;
}

std::vector<std::string> TouchedBodies(const SolidArcDocument& Document);
void CloseFillSession(SolidArcWorkspaceState& State);

void OpenTrimSession(SolidArcWorkspaceState& State)
{
    const bool SketchCondition = std::any_of(State.Document.Tree.Steps.begin(), State.Document.Tree.Steps.end(), [](const FeatureStep& Step) { return Step.Kind == FeatureStepKind::Sketch; });
    if (!SketchCondition) { Announce(State, "Trim needs sketch curves: draw some first (Shift+A line, Shift+S curve, Shift+W circle …)"); return; }
    State.Trim = { true, false, {}, -1, {}, {}, {}, State.Trim.Commits };
    State.Fill.ActiveCondition = false;
    State.Drawing.ActiveCondition = false;
    State.Constrain.ActiveCondition = false;
    Announce(State, "Trim: click the piece of a curve to remove (between its crossings) · Esc ends");
}

void CloseTrimSession(SolidArcWorkspaceState& State)
{
    State.Trim.ActiveCondition = false;
    State.Trim.HoverCondition  = false;
    State.Trim.Preview.clear();
    Announce(State, "Trim ended");
}

// 📝 The sketch curve nearest the pointer in pixels (≤ 8 px; later steps win ties), the pointer on its plane, and the
//    piece that would come off (red preview) — recomputed every frame from the solved sketches.
void HoverTrimPiece(SolidArcWorkspaceState& State, const OrbitCameraFrame& Frame, ImVec2 Local, bool PointerCondition)
{
    SolidArcTrimSession& Trim = State.Trim;
    Trim.HoverCondition = false;
    Trim.Preview.clear();
    Trim.Refusal.clear();
    if (!PointerCondition) return;
    const ImVec2       Mouse   = ImGui::GetIO().MousePos;
    double             Nearest = 8.0;                                                        // [px] - Pick radius
    const FeatureStep* Chosen  = nullptr;
    for (const FeatureStep& Step : State.Document.Tree.Steps)
    {
        if (Step.Kind != FeatureStepKind::Sketch || !CurveReachableCondition(State.Document, Step.Label)) continue;
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
    const SketchTrimOutcome Outcome = PreviewSolidArcTrim(State.Document, { Trim.Sketch, Trim.Curve, Trim.Pick });
    Trim.Refusal = Outcome.Refusal;
    if (Outcome.Refusal.empty()) for (const Vector2& Point : Outcome.Removed) Trim.Preview.push_back(PlacePlanePoint(Plane, Point));
}

void ApplyTrim(SolidArcWorkspaceState& State)
{
    SolidArcTrimSession& Trim = State.Trim;
    if (!Trim.HoverCondition) { Announce(State, "Trim: no curve under the pointer"); return; }
    const SolidArcCommandOutcome Outcome = TrimSolidArcSketch(State.Document, { Trim.Sketch, Trim.Curve, Trim.Pick });
    if (Outcome.AcceptedCondition)
    {
        // Detached trim (a later step reads the sketch): the original is hidden, the trimmed copy is the last step.
        if (SolidArcCurveHiddenCondition(State.Document, Trim.Sketch) && State.FocusedStep == Trim.Sketch && !State.Document.Tree.Steps.empty())
            State.FocusedStep = State.Document.Tree.Steps.back().Label;
        ++Trim.Commits;
        Trim.HoverCondition = false;
        Trim.Preview.clear();
        const bool FocusedCondition = std::any_of(State.Document.Tree.Steps.begin(), State.Document.Tree.Steps.end(), [&](const FeatureStep& Step) { return Step.Label == State.FocusedStep; });
        if (!FocusedCondition) State.FocusedStep = {};
    }
    Conclude(State, Outcome);
}

void DrawTrimPreview(SolidArcWorkspaceState& State, ImDrawList* Canvas, const OrbitCameraFrame& Frame)
{
    if (!State.Trim.ActiveCondition || State.Trim.Preview.size() < 2) return;
    std::vector<ImVec2> Pixels;
    for (const Vector3& Point : State.Trim.Preview) { ImVec2 Pixel; if (ProjectWorldPoint(State, Frame, Point, Pixel)) Pixels.push_back(Pixel); }
    if (Pixels.size() >= 2) Canvas->AddPolyline(Pixels.data(), int(Pixels.size()), IM_COL32(0xff, 0x4d, 0x4d, 0xff), 4.0f);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    FILL TOOL AND CLOSED-SKETCH TINTS
//------------------------------------------------------------------------------------------------------------------------

[[nodiscard]] float MeasurePixelArea(const std::vector<ImVec2>& Polygon)
{
    float Twice = 0.0f;
    for (size_t Index = 0; Index < Polygon.size(); ++Index)
    {
        const ImVec2& Here = Polygon[Index];
        const ImVec2& Next = Polygon[(Index + 1) % Polygon.size()];
        Twice += Here.x * Next.y - Next.x * Here.y;
    }
    return 0.5f * Twice;
}

// Proper crossing of the open segments AB and CD (shared end points do not count).
[[nodiscard]] bool PixelSegmentsCross(ImVec2 A, ImVec2 B, ImVec2 C, ImVec2 D)
{
    const auto Side = [](ImVec2 P, ImVec2 Q, ImVec2 R) { return (Q.x - P.x) * (R.y - P.y) - (Q.y - P.y) * (R.x - P.x); };
    const float First = Side(A, B, C), Second = Side(A, B, D), Third = Side(C, D, A), Fourth = Side(C, D, B);
    return ((First > 0.0f && Second < 0.0f) || (First < 0.0f && Second > 0.0f)) && ((Third > 0.0f && Fourth < 0.0f) || (Third < 0.0f && Fourth > 0.0f));
}

// 📝 A polygon with holes filled in one draw: each hole is bridged to the nearest polygon vertex it can see (keyhole),
//    rightmost hole first, then ImGui's ear clipping fills the single weakly simple outline.
void FillPixelPolygon(ImDrawList* Canvas, std::vector<ImVec2> Polygon, std::vector<std::vector<ImVec2>> Holes, ImU32 Colour)
{
    if (Polygon.size() < 3) return;
    if (MeasurePixelArea(Polygon) < 0.0f) std::reverse(Polygon.begin(), Polygon.end());
    std::erase_if(Holes, [](const std::vector<ImVec2>& Hole) { return Hole.size() < 3; });
    for (std::vector<ImVec2>& Hole : Holes) if (MeasurePixelArea(Hole) > 0.0f) std::reverse(Hole.begin(), Hole.end());
    const auto Rightmost = [](const std::vector<ImVec2>& Loop) { return size_t(std::max_element(Loop.begin(), Loop.end(), [](ImVec2 Left, ImVec2 Right) { return Left.x < Right.x; }) - Loop.begin()); };
    std::sort(Holes.begin(), Holes.end(), [&](const std::vector<ImVec2>& Left, const std::vector<ImVec2>& Right) { return Left[Rightmost(Left)].x > Right[Rightmost(Right)].x; });
    for (size_t HoleIndex = 0; HoleIndex < Holes.size(); ++HoleIndex)
    {
        const std::vector<ImVec2>& Hole = Holes[HoleIndex];
        const size_t Start  = Rightmost(Hole);
        const ImVec2 Anchor = Hole[Start];
        std::vector<size_t> Order(Polygon.size());
        std::iota(Order.begin(), Order.end(), size_t(0));
        const auto Gap = [&](size_t Index) { return (Polygon[Index].x - Anchor.x) * (Polygon[Index].x - Anchor.x) + (Polygon[Index].y - Anchor.y) * (Polygon[Index].y - Anchor.y); };
        std::sort(Order.begin(), Order.end(), [&](size_t Left, size_t Right) { return Gap(Left) < Gap(Right); });
        size_t Bridge = Order.front();
        for (size_t Candidate : Order)
        {
            bool Blocked = false;
            const ImVec2 Target = Polygon[Candidate];
            for (size_t Edge = 0; Edge < Polygon.size() && !Blocked; ++Edge) Blocked = PixelSegmentsCross(Anchor, Target, Polygon[Edge], Polygon[(Edge + 1) % Polygon.size()]);
            for (size_t Other = HoleIndex; Other < Holes.size() && !Blocked; ++Other)
                for (size_t Edge = 0; Edge < Holes[Other].size() && !Blocked; ++Edge) Blocked = PixelSegmentsCross(Anchor, Target, Holes[Other][Edge], Holes[Other][(Edge + 1) % Holes[Other].size()]);
            if (!Blocked) { Bridge = Candidate; break; }
        }
        std::vector<ImVec2> Joined(Polygon.begin(), Polygon.begin() + std::ptrdiff_t(Bridge) + 1);
        for (size_t Step = 0; Step <= Hole.size(); ++Step) Joined.push_back(Hole[(Start + Step) % Hole.size()]);
        Joined.insert(Joined.end(), Polygon.begin() + std::ptrdiff_t(Bridge), Polygon.end());
        Polygon = std::move(Joined);
    }
    Canvas->AddConcavePolyFilled(Polygon.data(), int(Polygon.size()), Colour);
}

// One profile loop as a closed polyline in plane coordinates (arcs ≤ 0.05 rad, 16 samples per spline span).
[[nodiscard]] std::vector<Vector2> SampleProfileLoop(const ProfileLoop& Loop)
{
    std::vector<Vector2> Points;
    for (const ProfileSegment& Segment : Loop.Segments)
    {
        int Count = 1;
        if (Segment.Kind == ProfileSegmentKind::Arc || Segment.Kind == ProfileSegmentKind::Circle) Count = std::max(4, int(std::ceil(std::fabs(Segment.Sweep) / 0.05)));
        if (Segment.Kind == ProfileSegmentKind::Spline) Count = 16 * std::max(1, int(Segment.Spline.Knots.size()) - 2 * Segment.Spline.Degree - 1);
        for (int Index = 0; Index < Count; ++Index) Points.push_back(EvaluateProfileSegment(Segment, double(Index) / Count));
    }
    return Points;
}

// 📝 The tint loops of a closed sketch (cached by a digest of the solved points, curves and fill flag); empty when open.
const SolidArcSketchTint& SketchTintOf(SolidArcWorkspaceState& State, const FeatureStep& Step, const SketchModel& Sketch)
{
    double Fingerprint = 1.0 + 7.0 * double(Sketch.Points.size()) + 13.0 * double(Sketch.Curves.size()) + (Step.FillCondition ? 0.5 : 0.0);
    for (size_t Index = 0; Index < Sketch.Points.size(); ++Index) Fingerprint += (1.37 * Sketch.Points[Index].Position.X + 2.11 * Sketch.Points[Index].Position.Y) * double(Index + 1);
    for (size_t Index = 0; Index < Sketch.Curves.size(); ++Index)
        Fingerprint += (Sketch.Curves[Index].Radius * 3.7 + double(Sketch.Curves[Index].Kind) * 0.3 + (Sketch.Curves[Index].Construction ? 0.11 : 0.0)) * double(Index + 1);
    SolidArcSketchTint& Tint = State.Tints[Step.Label];
    if (Tint.Fingerprint == Fingerprint) return Tint;
    Tint = {};
    Tint.Fingerprint   = Fingerprint;
    Tint.FillCondition = Step.FillCondition;
    const SketchProfile Profile = ExtractSketchProfile(Sketch);
    if (!Profile.Refusal.empty()) return Tint;
    for (size_t Outer = 0; Outer < Profile.Loops.size(); ++Outer)
    {
        if (Profile.Loops[Outer].Depth % 2 != 0) continue;
        Tint.Outers.push_back(SampleProfileLoop(Profile.Loops[Outer]));
        Tint.Holes.emplace_back();
        for (const ProfileLoop& Hole : Profile.Loops) if (Hole.Enclosure == int(Outer)) Tint.Holes.back().push_back(SampleProfileLoop(Hole));
    }
    return Tint;
}

// Closed sketches show a faint tint (unfilled, Plasticity's fresh curves) or a stronger one (filled: E makes a solid).
void DrawSketchTint(SolidArcWorkspaceState& State, ImDrawList* Canvas, const OrbitCameraFrame& Frame, const FeatureStep& Step, const SketchModel& Sketch,
                    const OrthonormalFrame& Plane, bool FocusedCondition)
{
    const SolidArcSketchTint& Tint = SketchTintOf(State, Step, Sketch);
    const ImU32 Colour = Tint.FillCondition ? IM_COL32(0x9f, 0xd8, 0xff, FocusedCondition ? 0x60 : 0x48) : IM_COL32(0x9f, 0xd8, 0xff, FocusedCondition ? 0x26 : 0x16);
    const auto Project = [&](const std::vector<Vector2>& Loop)
    {
        std::vector<ImVec2> Pixels;
        for (const Vector2& Point : Loop) { ImVec2 Pixel; if (ProjectWorldPoint(State, Frame, PlacePlanePoint(Plane, Point), Pixel)) Pixels.push_back(Pixel); }
        return Pixels.size() == Loop.size() ? Pixels : std::vector<ImVec2>{};
    };
    for (size_t Outer = 0; Outer < Tint.Outers.size(); ++Outer)
    {
        std::vector<std::vector<ImVec2>> Holes;
        for (const std::vector<Vector2>& Hole : Tint.Holes[Outer]) Holes.push_back(Project(Hole));
        FillPixelPolygon(Canvas, Project(Tint.Outers[Outer]), std::move(Holes), Colour);
    }
}

void OpenFillSession(SolidArcWorkspaceState& State)
{
    const bool SketchCondition = std::any_of(State.Document.Tree.Steps.begin(), State.Document.Tree.Steps.end(), [](const FeatureStep& Step) { return Step.Kind == FeatureStepKind::Sketch; });
    if (!SketchCondition) { Announce(State, "Fill needs closed sketch curves: draw some first (Shift+W circle, Shift+Q rectangle …)"); return; }
    const int Commits = State.Fill.Commits;
    State.Fill = {};
    State.Fill.ActiveCondition    = true;
    State.Fill.Commits            = Commits;
    State.Trim.ActiveCondition    = false;
    State.Drawing.ActiveCondition = false;
    State.Constrain.ActiveCondition = false;
    Announce(State, "Fill: hover a closed region (overlaps split into their own regions) · click fills it · Esc ends");
}

void CloseFillSession(SolidArcWorkspaceState& State)
{
    State.Fill.ActiveCondition = false;
    State.Fill.HoverCondition  = false;
    State.Fill.Outlines.clear();
    Announce(State, "Fill ended");
}

// 📝 The region under the pointer on the nearest shown sketch plane (each distinct plane tried once, nearest hit that
//    holds a region wins); cached while the pointer and the document stay put.
void HoverFillRegion(SolidArcWorkspaceState& State, const OrbitCameraFrame& Frame, ImVec2 Local, bool PointerCondition)
{
    SolidArcFillSession& Fill = State.Fill;
    if (!PointerCondition)
    {
        Fill.HoverCondition = false;
        Fill.Outlines.clear();
        Fill.Pointer = ImVec2(-1.0f, -1.0f);
        return;
    }
    const ImVec2 Mouse = ImGui::GetIO().MousePos;
    if (Mouse.x == Fill.Pointer.x && Mouse.y == Fill.Pointer.y && Fill.Revision == State.Document.Revision) return;
    Fill.Pointer        = Mouse;
    Fill.Revision       = State.Document.Revision;
    Fill.HoverCondition = false;
    Fill.Outlines.clear();
    Fill.WholeSketch.clear();
    Fill.Refusal = "no closed region under the pointer";
    std::vector<OrthonormalFrame> Tried;
    double Nearest = std::numeric_limits<double>::infinity();
    const Vector3 Eye{ Frame.Eye[0], Frame.Eye[1], Frame.Eye[2] };
    for (const FeatureStep& Step : State.Document.Tree.Steps)
    {
        if (Step.Kind != FeatureStepKind::Sketch || SolidArcCurveHiddenCondition(State.Document, Step.Label)) continue;
        const OrthonormalFrame Plane = ElevatedSketchPlane(State.Document, Step);
        const bool Seen = std::any_of(Tried.begin(), Tried.end(), [&](const OrthonormalFrame& Other)
        {
            return std::fabs(Dot(Other.ZAxis, Plane.ZAxis)) >= 1.0 - 1.0e-9 && std::fabs(Dot(Other.Origin - Plane.Origin, Plane.ZAxis)) <= 1.0e-9 * std::max(1.0, Length(Other.Origin - Plane.Origin));
        });
        if (Seen) continue;
        Tried.push_back(Plane);
        Vector2 Pick;
        if (!IntersectSketchPlane(Frame, Local, Plane, Pick)) continue;
        const double Depth = Length(PlacePlanePoint(Plane, Pick) - Eye);
        if (Depth >= Nearest) continue;
        const SolidArcRegionPreview Preview = PreviewSolidArcRegion(State.Document, { Step.Label, Pick });
        if (!Preview.Refusal.empty()) continue;
        Nearest             = Depth;
        Fill.HoverCondition = true;
        Fill.Sketch         = Step.Label;
        Fill.Pick           = Pick;
        Fill.WholeSketch    = Preview.WholeSketch;
        Fill.Refusal.clear();
        Fill.Outlines.clear();
        for (const std::vector<Vector2>& Outline : Preview.Region.Outlines)
        {
            Fill.Outlines.emplace_back();
            for (const Vector2& Point : Outline) Fill.Outlines.back().push_back(PlacePlanePoint(Preview.Plane, Point));
        }
    }
}

void ApplyFill(SolidArcWorkspaceState& State)
{
    SolidArcFillSession& Fill = State.Fill;
    if (!Fill.HoverCondition) { Announce(State, "Fill: no closed region under the pointer"); return; }
    const SolidArcCommandOutcome Outcome = FillSolidArcRegion(State.Document, { Fill.Sketch, Fill.Pick });
    if (Outcome.AcceptedCondition)
    {
        ++Fill.Commits;
        State.FocusedStep = !Fill.WholeSketch.empty() ? Fill.WholeSketch : State.Document.Tree.Steps.back().Label;   // E extrudes it next
        Fill.Revision = 0;                                                                   // Hover again on the new document
        Fill.HoverCondition = false;
        Fill.Outlines.clear();
    }
    Conclude(State, Outcome);
}

void DrawFillPreview(SolidArcWorkspaceState& State, ImDrawList* Canvas, const OrbitCameraFrame& Frame)
{
    if (!State.Fill.ActiveCondition || !State.Fill.HoverCondition || State.Fill.Outlines.empty()) return;
    std::vector<std::vector<ImVec2>> Loops;
    for (const std::vector<Vector3>& Outline : State.Fill.Outlines)
    {
        Loops.emplace_back();
        for (const Vector3& Point : Outline) { ImVec2 Pixel; if (ProjectWorldPoint(State, Frame, Point, Pixel)) Loops.back().push_back(Pixel); }
        if (Loops.back().size() != Outline.size()) return;
    }
    FillPixelPolygon(Canvas, Loops.front(), std::vector<std::vector<ImVec2>>(Loops.begin() + 1, Loops.end()), IM_COL32(0x4f, 0xd8, 0xe0, 0x70));
    for (const std::vector<ImVec2>& Loop : Loops) Canvas->AddPolyline(Loop.data(), int(Loop.size()), IM_COL32(0x4f, 0xd8, 0xe0, 0xff), ImDrawFlags_Closed, 2.0f);
}

// F: fill on / off for the focused and picked sketches (closed ones); with no sketch picked, the Fill tool opens.
void ToggleSketchFill(SolidArcWorkspaceState& State)
{
    std::vector<std::string> Labels = SelectedSketchLabels(State.Document);
    if (FocusedSketchCondition(State) && std::find(Labels.begin(), Labels.end(), State.FocusedStep) == Labels.end()) Labels.insert(Labels.begin(), State.FocusedStep);
    if (Labels.empty()) { State.Fill.ActiveCondition ? CloseFillSession(State) : OpenFillSession(State); return; }
    bool Filled = false;
    for (const FeatureStep& Step : State.Document.Tree.Steps) if (Step.Label == Labels.front()) Filled = Step.FillCondition;
    for (const std::string& Label : Labels) Conclude(State, FillSolidArcSketch(State.Document, Label, !Filled));
}

// Alt+N: flip normals of the picked sheets (bodies in mode 4, or the owners of picked faces / edges).
void FlipPickedNormals(SolidArcWorkspaceState& State)
{
    const std::vector<std::string> Keys = TouchedBodies(State.Document);
    if (Keys.empty()) { Announce(State, "Flip normals: pick a sheet first (a loft or walls: a body in mode 4, or one of its faces)"); return; }
    Conclude(State, FlipSolidArcBodies(State.Document, Keys));
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    CONSTRAINT SESSION (K)
//------------------------------------------------------------------------------------------------------------------------

std::string CaptionOf(const SolidArcDocument& Document, const std::string& Label);
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

std::string ConstraintSketchLabel(const SolidArcWorkspaceState& State)
{
    const std::string Label = State.Constrain.ActiveCondition ? State.Constrain.Sketch : State.FocusedStep;
    for (const FeatureStep& Step : State.Document.Tree.Steps) if (Step.Label == Label && Step.Kind == FeatureStepKind::Sketch) return Label;
    return {};
}

[[nodiscard]] ConstraintView ViewConstraintSketch(const SolidArcWorkspaceState& State, const std::string& Label)
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
std::vector<ConstraintGlyph> LayoutConstraintGlyphs(const SolidArcWorkspaceState& State, const OrbitCameraFrame& Frame, const SketchModel& Sketch, const OrthonormalFrame& Plane)
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

[[nodiscard]] std::vector<OtherSketch> OtherSketches(const SolidArcWorkspaceState& State)
{
    std::vector<OtherSketch> Sketches;
    for (const std::string& Label : SolidArcLinkSources(State.Document, State.Constrain.Sketch))
    {
        const auto Solved = State.Document.Regeneration.Sketches.find(Label);
        if (Solved == State.Document.Regeneration.Sketches.end()) continue;
        for (const FeatureStep& Step : State.Document.Tree.Steps)
            if (Step.Label == Label && Step.Kind == FeatureStepKind::Sketch) Sketches.push_back({ Label, &Solved->second, ElevatedSketchPlane(State.Document, Step) });
    }
    return Sketches;
}

[[nodiscard]] bool SamePick(const SolidArcSketchPick& First, const SolidArcSketchPick& Second) noexcept
{
    return First.Sketch == Second.Sketch && First.Pick.Kind == Second.Pick.Kind && First.Pick.Index == Second.Pick.Index;
}

void OpenConstraintSession(SolidArcWorkspaceState& State)
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
    State.Fill.ActiveCondition      = false;
    State.FocusedStep               = Label;
    const ConstraintView View = ViewConstraintSketch(State, Label);
    Announce(State, Compose("Constrain %s · %d constraint(s) · DOF %d · click points / curves (earlier sketches link), then D N H V O Q M Y L [ ] C · Shift+D reference · drag points · Esc ends", CaptionOf(State.Document, Label).c_str(),
                            int(View.Sketch->Constraints.size()), View.Solve ? View.Solve->DegreesOfFreedom : -1));
}

void CloseConstraintSession(SolidArcWorkspaceState& State)
{
    State.Constrain.ActiveCondition = false;
    State.Constrain.DragCondition   = false;
    State.Constrain.PressCondition  = false;
    Announce(State, "Constrain ended");
}

// Drops picks that no longer exist (after undo) and ends the session when its sketch is gone.
void ReconcileConstraintSession(SolidArcWorkspaceState& State)
{
    SolidArcConstraintSession& Session = State.Constrain;
    const ConstraintView View = ViewConstraintSketch(State, Session.Sketch);
    if (!View.Step) { CloseConstraintSession(State); return; }
    std::erase_if(Session.Picks, [&](const SolidArcSketchPick& Pick)
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
void HoverConstraintTarget(SolidArcWorkspaceState& State, const OrbitCameraFrame& Frame, const ConstraintView& View, const std::vector<ConstraintGlyph>& Glyphs)
{
    SolidArcConstraintSession& Session = State.Constrain;
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
[[nodiscard]] std::string NameSessionPick(const SolidArcWorkspaceState& State, const SolidArcSketchPick& Pick)
{
    const std::string Label  = Pick.Sketch.empty() ? State.Constrain.Sketch : Pick.Sketch;
    const auto        Solved = State.Document.Regeneration.Sketches.find(Label);
    if (Solved == State.Document.Regeneration.Sketches.end()) return "?";
    const std::string Name = NameSketchPick(Solved->second, Pick.Pick);
    return Pick.Sketch.empty() ? Name : Name + " of " + CaptionOf(State.Document, Pick.Sketch);
}

[[nodiscard]] std::string ListConstraintPicks(const SolidArcWorkspaceState& State)
{
    const ConstraintView View = ViewConstraintSketch(State, State.Constrain.Sketch);
    std::string Names;
    if (!View.Sketch) return Names;
    for (const SolidArcSketchPick& Pick : State.Constrain.Picks) Names += (Names.empty() ? "" : ", ") + NameSessionPick(State, Pick);
    return Names;
}

void ConcludeConstraint(SolidArcWorkspaceState& State, const SolidArcCommandOutcome& Outcome)
{
    SolidArcConstraintSession& Session = State.Constrain;
    Session.Refusal = Outcome.AcceptedCondition ? std::string{} : Outcome.Refusal;
    if (Outcome.AcceptedCondition) ++Session.Commits;
    Conclude(State, Outcome);
}

void CommitConstraint(SolidArcWorkspaceState& State, SketchConstraint Constraint)
{
    SolidArcConstraintSession& Session = State.Constrain;
    const SolidArcCommandOutcome Outcome = ConstrainSolidArcSketch(State.Document, Session.Sketch, Constraint, Session.Picks);
    ConcludeConstraint(State, Outcome);
    if (!Outcome.AcceptedCondition) return;
    Session.Picks.clear();
    Session.PendingCondition = false;
    Session.Typed.clear();
    const ConstraintView View = ViewConstraintSketch(State, Session.Sketch);
    Session.PickedConstraint = View.Sketch && SketchDimensionCondition(Constraint.Kind) ? int(View.Sketch->Constraints.size()) - 1 : -1;
}

// 📝 A verb key: the picks become a constraint (D / N wait for a value first), or the refusal says why not.
void ApplyConstraintVerb(SolidArcWorkspaceState& State, SketchConstraintVerb Verb)
{
    SolidArcConstraintSession& Session = State.Constrain;
    const SolidArcLinkedPicks Linked = LinkSolidArcPicks(State.Document, Session.Sketch, Session.Picks);
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
void ApplyConstraintReference(SolidArcWorkspaceState& State)
{
    SolidArcConstraintSession& Session = State.Constrain;
    const SolidArcLinkedPicks Linked = LinkSolidArcPicks(State.Document, Session.Sketch, Session.Picks);
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
void TogglePickedReference(SolidArcWorkspaceState& State)
{
    SolidArcConstraintSession& Session = State.Constrain;
    if (Session.PickedConstraint < 0) { Announce(State, "Reference (E): click a dimension first"); return; }
    ConcludeConstraint(State, ToggleSolidArcReference(State.Document, Session.Sketch, Session.PickedConstraint));
}

// Delete on one picked linked point / curve: the link dropped, the geometry stays (free).
void UnlinkPickedGeometry(SolidArcWorkspaceState& State)
{
    SolidArcConstraintSession& Session = State.Constrain;
    const SolidArcCommandOutcome Outcome = UnlinkSolidArcGeometry(State.Document, Session.Sketch, Session.Picks.front().Pick);
    ConcludeConstraint(State, Outcome);
    if (Outcome.AcceptedCondition) Session.Picks.clear();
}

void CommitPendingConstraint(SolidArcWorkspaceState& State)
{
    SolidArcConstraintSession& Session = State.Constrain;
    SketchConstraint Constraint = Session.Proposed;
    double Typed = 0.0;
    if (ParseTypedAmount(Session.Typed, Typed)) Constraint.Value = Constraint.Kind == SketchConstraintKind::Angle ? Typed * std::numbers::pi / 180.0 : Typed;
    CommitConstraint(State, Constraint);
}

void RemovePickedConstraint(SolidArcWorkspaceState& State)
{
    SolidArcConstraintSession& Session = State.Constrain;
    if (Session.PickedConstraint < 0) { Announce(State, "Remove: click a constraint glyph or dimension first"); return; }
    const SolidArcCommandOutcome Outcome = RemoveSolidArcConstraint(State.Document, Session.Sketch, Session.PickedConstraint);
    ConcludeConstraint(State, Outcome);
    if (Outcome.AcceptedCondition) Session.PickedConstraint = -1;
}

void EditPickedDimension(SolidArcWorkspaceState& State)
{
    SolidArcConstraintSession& Session = State.Constrain;
    const ConstraintView View = ViewConstraintSketch(State, Session.Sketch);
    double Value = 0.0;
    if (!View.Sketch || Session.PickedConstraint < 0 || !ParseTypedAmount(Session.Typed, Value)) return;
    const bool Angular = View.Sketch->Constraints[Session.PickedConstraint].Kind == SketchConstraintKind::Angle;
    ConcludeConstraint(State, ChangeSolidArcDimension(State.Document, Session.Sketch, Session.PickedConstraint, Angular ? Value * std::numbers::pi / 180.0 : Value));
    Session.Typed.clear();
}

void ClickConstraintTarget(SolidArcWorkspaceState& State)
{
    SolidArcConstraintSession& Session = State.Constrain;
    Session.Refusal.clear();
    if (Session.HoverCondition)
    {
        const auto Found = std::find_if(Session.Picks.begin(), Session.Picks.end(), [&](const SolidArcSketchPick& Pick) { return SamePick(Pick, Session.Hover); });
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
void DriveConstraintSession(SolidArcWorkspaceState& State, const OrbitCameraFrame& Frame, ImVec2 Local, bool PointerCondition, bool LeftClick, bool RightClick)
{
    ReconcileConstraintSession(State);
    SolidArcConstraintSession& Session = State.Constrain;
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
                const SketchDragOutcome Preview = PreviewSolidArcDrag(State.Document, Session.Sketch, Session.DragPoint, Target);
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
            if (Session.DragPreviewCondition && State.Picker.Current.Kind != SolidArcSnapKind::None) ++State.Picker.Snapped;
            State.Picker.Current = {};
            if (Session.DragPreviewCondition) ConcludeConstraint(State, DragSolidArcSketchPoint(State.Document, Session.Sketch, Session.DragPoint, Session.DragTarget));
            Session.DragPreviewCondition = false;
            return;
        }
    }
    if (LeftClick) ClickConstraintTarget(State);
    // Right-click: a pending constraint (its value typed or not) is committed like Enter; otherwise the session ends.
    if (RightClick)
    {
        if (Session.PendingCondition) CommitPendingConstraint(State);
        else CloseConstraintSession(State);
    }
}

// Pixel polyline of one sketch curve.
[[nodiscard]] std::vector<ImVec2> CurvePixels(const SolidArcWorkspaceState& State, const OrbitCameraFrame& Frame, const SketchModel& Sketch, const SketchCurve& Curve, const OrthonormalFrame& Plane)
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
void DrawConstraintOverlay(SolidArcWorkspaceState& State, ImDrawList* Canvas, const OrbitCameraFrame& Frame)
{
    const SolidArcConstraintSession& Session = State.Constrain;
    if (!Session.ActiveCondition) return;
    const ConstraintView View = ViewConstraintSketch(State, Session.Sketch);
    if (!View.Sketch) return;
    const bool         Dragging = Session.DragCondition && Session.DragPreviewCondition;
    const SketchModel& Shown    = Dragging ? Session.DragPreview : *View.Sketch;
    auto PickedIn = [&](const std::string& Label, SketchPickKind Kind, int Index) { return std::any_of(Session.Picks.begin(), Session.Picks.end(), [&](const SolidArcSketchPick& Pick) { return SamePick(Pick, { Label, { Kind, Index } }); }); };
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
[[nodiscard]] std::string ConstraintSessionReadout(const SolidArcWorkspaceState& State, bool& TroubleCondition)
{
    const SolidArcConstraintSession& Session = State.Constrain;
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
void RouteConstraintKeys(SolidArcWorkspaceState& State, PressedKey&& Pressed, bool Control, bool Shift, bool Alternate)
{
    SolidArcConstraintSession& Session = State.Constrain;
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
        if (View.Sketch && SolidArcLinkedCondition(*View.Sketch, Session.Picks.front().Pick)) { UnlinkPickedGeometry(State); return; }
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
void DrawConstraintCard(SolidArcWorkspaceState& State)
{
    const std::string Label = ConstraintSketchLabel(State);
    if (Label.empty()) return;
    const ConstraintView View = ViewConstraintSketch(State, Label);
    if (!View.Sketch) return;
    SolidArcConstraintSession& Session = State.Constrain;
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
                    ConcludeConstraint(State, ChangeSolidArcDimension(State.Document, Session.Sketch, Session.PickedConstraint, Angular ? Value * std::numbers::pi / 180.0 : Value));
                RegisterWidget(State, "DimensionValue");
            }
            if (PressButton(State, "Remove###RemoveConstraint", RowSpan())) RemovePickedConstraint(State);
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

std::string CaptionOf(const SolidArcDocument& Document, const std::string& Label);

constexpr double MoveSnap   = 0.1;                                                           // [len] - G step
constexpr double TurnSnap   = 5.0;                                                           // [°]   - R step
constexpr double FactorSnap = 0.1;                                                           // [-]   - S step
constexpr double SweepSnap  = 0.01;                                                          // [len] - O / P step

[[nodiscard]] const char* TransformVerbLabel(SolidArcTransformVerb Verb) noexcept
{
    return Verb == SolidArcTransformVerb::Move ? "Move" : Verb == SolidArcTransformVerb::Rotate ? "Rotate" : "Scale";
}

// The bodies a session acts on: body picks, else the owners of picked faces / edges.
[[nodiscard]] std::vector<std::string> PickedBodies(const SolidArcDocument& Document)
{
    std::vector<std::string> Keys = Document.Selection.Bodies;
    auto Add = [&](const std::string& Key) { if (!Key.empty() && std::find(Keys.begin(), Keys.end(), Key) == Keys.end()) Keys.push_back(Key); };
    for (const std::string& Face : Document.Selection.Faces) Add(SolidArcBodyOfName(Document, SolidArcSelectionKind::Face, Face));
    for (const std::string& Edge : Document.Selection.Edges) Add(SolidArcBodyOfName(Document, SolidArcSelectionKind::Edge, Edge));
    return Keys;
}

// 💡 Rigid preview: the shown scene with the keyed bodies' corners and edge segments carried by the map — what a move /
//    rotate / scale regeneration would tessellate, at the cost of a copy (no clone, no replay, no tessellation). Normals
//    follow the inverse transpose (M⁻ᵀ = [b×c | c×a | a×b] / det). The release still commits through the kernel.
[[nodiscard]] SolidArcScene MapSceneBodies(const SolidArcScene& Scene, const std::vector<std::string>& Keys, const AffineMap& Map)
{
    static uint64_t Counter = 0;
    SolidArcScene Mapped = Scene;
    Mapped.Generation    = (uint64_t(1) << 62) + ++Counter;                                  // Distinct from every built scene: re-upload
    const Vector3 NormalX = Cross(Map.ColumnY, Map.ColumnZ), NormalY = Cross(Map.ColumnZ, Map.ColumnX), NormalZ = Cross(Map.ColumnX, Map.ColumnY);
    const auto Carry = [&](float* Position)
    {
        const Vector3 Moved = Map.ColumnX * Position[0] + Map.ColumnY * Position[1] + Map.ColumnZ * Position[2] + Map.Shift;
        Position[0] = float(Moved.X), Position[1] = float(Moved.Y), Position[2] = float(Moved.Z);
    };
    for (const SolidArcSceneBody& Body : Mapped.Bodies)
    {
        if (std::find(Keys.begin(), Keys.end(), Body.Key) == Keys.end()) continue;
        for (uint32_t Face = Body.FirstFace; Face < Body.FirstFace + Body.FaceCount && Face < Mapped.Faces.size(); ++Face)
            for (uint32_t Corner = Mapped.Faces[Face].FirstCorner; Corner < Mapped.Faces[Face].FirstCorner + Mapped.Faces[Face].CornerCount && Corner < Mapped.Corners.size(); ++Corner)
            {
                SolidArcSceneCorner& Point = Mapped.Corners[Corner];
                Carry(Point.Position);
                const Vector3 Turned = Normalize(NormalX * Point.Normal[0] + NormalY * Point.Normal[1] + NormalZ * Point.Normal[2]);
                Point.Normal[0] = float(Turned.X), Point.Normal[1] = float(Turned.Y), Point.Normal[2] = float(Turned.Z);
            }
        for (uint32_t Edge = Body.FirstEdge; Edge < Body.FirstEdge + Body.EdgeCount && Edge < Mapped.Edges.size(); ++Edge)
            for (uint32_t Segment = Mapped.Edges[Edge].FirstSegment; Segment < Mapped.Edges[Edge].FirstSegment + Mapped.Edges[Edge].SegmentCount && Segment < Mapped.Segments.size(); ++Segment)
            {
                Carry(Mapped.Segments[Segment].Start);
                Carry(Mapped.Segments[Segment].End);
            }
    }
    if (!Mapped.Corners.empty())
    {
        Mapped.BoundsLow  = { 1.0e300, 1.0e300, 1.0e300 };
        Mapped.BoundsHigh = { -1.0e300, -1.0e300, -1.0e300 };
        for (const SolidArcSceneCorner& Point : Mapped.Corners)
            for (int Axis = 0; Axis < 3; ++Axis)
            {
                Mapped.BoundsLow[Axis]  = std::min(Mapped.BoundsLow[Axis], double(Point.Position[Axis]));
                Mapped.BoundsHigh[Axis] = std::max(Mapped.BoundsHigh[Axis], double(Point.Position[Axis]));
            }
    }
    Mapped.Milliseconds = 0.0;
    return Mapped;
}

// Bounds centre of the bodies in the shown scene (their tessellation corners).
[[nodiscard]] Vector3 MeasureBodiesCentre(const SolidArcScene& Scene, const std::vector<std::string>& Keys)
{
    Vector3 Low{ 1.0e300, 1.0e300, 1.0e300 }, High{ -1.0e300, -1.0e300, -1.0e300 };
    for (const SolidArcSceneBody& Body : Scene.Bodies)
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

void OpenTransformSession(SolidArcWorkspaceState& State, SolidArcTransformVerb Verb, bool DuplicateCondition)
{
    const std::vector<std::string> Keys = PickedBodies(State.Document);
    if (Keys.empty()) { Announce(State, std::string(DuplicateCondition ? "Duplicate" : TransformVerbLabel(Verb)) + " needs picked bodies (mode 4, or a face / edge of the body)"); return; }
    SolidArcTransformSession& Transform = State.Transform;
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
    State.Fill.ActiveCondition    = false;
    State.Constrain.ActiveCondition = false;
    Announce(State, Compose("%s%s %d body(ies) · X / Y / Z axis%s · type a value · click / Enter commits · Esc cancels", DuplicateCondition ? "Duplicate + " : "",
                            TransformVerbLabel(Verb), int(Keys.size()), Verb == SolidArcTransformVerb::Move ? " · Shift+X / Y / Z plane" : ""));
}

void CancelTransformSession(SolidArcWorkspaceState& State)
{
    State.Transform.ActiveCondition = false;
    State.Transform.RigidCondition  = false;
    State.Transform.Preview.reset();
    Announce(State, std::string(TransformVerbLabel(State.Transform.Verb)) + " cancelled");
}

[[nodiscard]] bool IdentityMapCondition(const AffineMap& Map)
{
    return Length(Map.ColumnX - Vector3{ 1.0, 0.0, 0.0 }) + Length(Map.ColumnY - Vector3{ 0.0, 1.0, 0.0 }) + Length(Map.ColumnZ - Vector3{ 0.0, 0.0, 1.0 }) + Length(Map.Shift) < 1.0e-12;
}

void ApplyTransformSession(SolidArcWorkspaceState& State)
{
    SolidArcTransformSession& Transform = State.Transform;
    if (!Transform.Refusal.empty()) { Announce(State, "Refused: " + Transform.Refusal); return; }
    if (!Transform.DuplicateCondition && IdentityMapCondition(Transform.Map)) { Transform.ActiveCondition = false; Transform.RigidCondition = false; Transform.Preview.reset(); Announce(State, std::string(TransformVerbLabel(Transform.Verb)) + ": nothing changed"); return; }
    const SolidArcCommandOutcome Outcome = Transform.DuplicateCondition ? DuplicateSolidArcBodies(State.Document, Transform.Keys, Transform.Map)
                                                                        : TransformSolidArcBodies(State.Document, Transform.Keys, Transform.Map, TransformVerbLabel(Transform.Verb));
    if (Outcome.AcceptedCondition)
    {
        ++Transform.Commits;
        Transform.ActiveCondition = false;
        Transform.RigidCondition  = false;
        Transform.Preview.reset();
        State.PickMode = SolidArcPickMode::Solid;
    }
    Conclude(State, Outcome);
}

// Pointer → affine map (see SolidArcTransformSession), then the staged preview when the map changed.
void DriveTransformSession(SolidArcWorkspaceState& State, const OrbitCameraFrame& Frame, ImVec2 Local)
{
    SolidArcTransformSession& Transform = State.Transform;
    ImGuiIO& Io = ImGui::GetIO();
    if (!Transform.StartCondition)
    {
        Transform.Start = Io.MousePos; Transform.StartLocal = Local; Transform.StartCondition = true;
        if (Transform.Verb == SolidArcTransformVerb::Move)
        {
            // Plasticity move: the grab point snaps (a vertex of the body), then follows the pointer's snaps elsewhere.
            const SolidArcSnap Anchor = FindPointerSnap(State, Frame, Local, std::nullopt, {}, {}, true);
            Transform.AnchorCondition = Anchor.Kind != SolidArcSnapKind::None;
            Transform.Anchor          = Anchor.Position;
            Transform.AnchorSource    = Transform.AnchorCondition ? std::string(SolidArcSnapLabel(Anchor.Kind)) + " " + Anchor.Source : std::string{};
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
    case SolidArcTransformVerb::Move:
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
                const OrbitRay Ray = CastOrbitRay(Frame, At.x, At.y);
                const Vector3 Origin{ Ray.Origin[0], Ray.Origin[1], Ray.Origin[2] }, Direction{ Ray.Direction[0], Ray.Direction[1], Ray.Direction[2] };
                const double Facing = Dot(Direction, Normal);
                if (std::abs(Facing) < 1.0e-9) return false;
                Hit = Origin + Direction * (Dot(Transform.Pivot - Origin, Normal) / Facing);
                return true;
            };
            Vector3 From, To;
            const SolidArcSnap Target = Transform.AnchorCondition ? FindPointerSnap(State, Frame, Local, std::nullopt, {}, Transform.Keys, false) : SolidArcSnap{};   // Ctrl
            State.Picker.Current = Target;
            if (Target.Kind != SolidArcSnapKind::None)
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
        if (!TypedCondition && State.Picker.Current.Kind != SolidArcSnapKind::None)
            Transform.Readout += std::string(" · ") + Transform.AnchorSource + " → " + SolidArcSnapLabel(State.Picker.Current.Kind) + " " + State.Picker.Current.Source;
        break;
    }
    case SolidArcTransformVerb::Rotate:
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
    case SolidArcTransformVerb::Scale:
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
    Transform.RigidCondition  = false;
    if (!Transform.DuplicateCondition && IdentityMapCondition(Map)) { Transform.Preview.reset(); return; }
    if (!Transform.DuplicateCondition)
    {
        // A move / rotate / scale of whole bodies: carry the shown triangles (the kernel runs once, on commit).
        SolidArcPhaseClock RigidClock(State.Profile, "transform preview · rigid");
        Transform.Preview.reset();
        Transform.PreviewScene   = MapSceneBodies(State.Scene, Transform.Keys, Map);
        Transform.RigidCondition = true;
        return;
    }
    FRONTIER_TRACE_SCOPE("Preview", "StageTransformPreview · " + Transform.Readout);
    SolidArcPhaseClock StageClock(State.Profile, "transform preview · stage");
    Transform.Preview = std::make_unique<SolidArcDocument>(CloneSolidArcDocument(State.Document));
    const SolidArcCommandOutcome Outcome = Transform.DuplicateCondition ? DuplicateSolidArcBodies(*Transform.Preview, Transform.Keys, Map)
                                                                        : TransformSolidArcBodies(*Transform.Preview, Transform.Keys, Map, TransformVerbLabel(Transform.Verb));
    if (!Outcome.AcceptedCondition) { Transform.Refusal = Legible(Outcome.Refusal); Transform.Preview.reset(); return; }
    Transform.PreviewScene = BuildSolidArcScene(*Transform.Preview);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    TRANSFORM GIZMO
//------------------------------------------------------------------------------------------------------------------------

// No other session owns the pointer (the gizmo is a no-command tool, like Plasticity's selection gizmo).
[[nodiscard]] bool GizmoAvailableCondition(const SolidArcWorkspaceState& State)
{
    return State.Gizmo.Modes != 0 && !State.DrawingSheet.ActiveCondition && !State.Drawing.ActiveCondition && !State.Transform.ActiveCondition && !State.Sweep.ActiveCondition && !State.Knife.ActiveCondition
        && !State.Constrain.ActiveCondition && !State.Trim.ActiveCondition && !State.Fill.ActiveCondition && !State.Joint.ActiveCondition && !State.Bevel.ActiveCondition && !State.PoleDrag.ActiveCondition
        && !State.Box.ActiveCondition && !State.SketchPoints.DragCondition;
}

[[nodiscard]] const char* GizmoTargetLabel(SolidArcGizmoTarget Target) noexcept
{
    switch (Target)
    {
    case SolidArcGizmoTarget::Faces:    return "Faces";
    case SolidArcGizmoTarget::Bodies:   return "Bodies";
    case SolidArcGizmoTarget::Sketches: return "Sketches";
    case SolidArcGizmoTarget::Points:   return "Sketch points";
    case SolidArcGizmoTarget::None:     break;
    }
    return "Nothing";
}

[[nodiscard]] std::string GizmoModesLabel(uint32_t Modes)
{
    if (Modes == 0) return "off";
    std::string Text;
    if (Modes & GizmoTranslate) Text += "move";
    if (Modes & GizmoRotate)    Text += Text.empty() ? "rotate" : " + rotate";
    if (Modes & GizmoScale)     Text += Text.empty() ? "scale" : " + scale";
    return Text;
}

// The sketch steps among the selected curve labels (loft sections the gizmo can carry).
[[nodiscard]] std::vector<std::string> PickedSketches(const SolidArcDocument& Document)
{
    std::vector<std::string> Labels;
    for (const std::string& Label : Document.Selection.Curves)
        for (const FeatureStep& Step : Document.Tree.Steps)
            if (Step.Label == Label && Step.Kind == FeatureStepKind::Sketch) { Labels.push_back(Label); break; }
    return Labels;
}

// 📝 Target and frame from the selection: faces → area-weighted centroid, z = mean outward normal; bodies → bounds
//    centre, world axes; sketches → curve-sample centroid, the first sketch's plane axes. Handles that do not apply are
//    hidden (faces and sketches have no per-axis scale; sketches cannot scale at all).
void ResolveGizmoTarget(SolidArcWorkspaceState& State)
{
    SolidArcGizmoSession&   Gizmo    = State.Gizmo;
    const SolidArcDocument& Document = State.Document;
    Gizmo.Target = SolidArcGizmoTarget::None;
    Gizmo.Names.clear();
    Gizmo.Points.clear();
    Gizmo.Pose.Hidden = 0;
    Gizmo.Pose.Axes[0] = { 1.0, 0.0, 0.0 };
    Gizmo.Pose.Axes[1] = { 0.0, 1.0, 0.0 };
    Gizmo.Pose.Axes[2] = { 0.0, 0.0, 1.0 };
    constexpr uint32_t AxisScales = (1u << uint32_t(SolidArcGizmoHandle::ScaleX)) | (1u << uint32_t(SolidArcGizmoHandle::ScaleY)) | (1u << uint32_t(SolidArcGizmoHandle::ScaleZ));
    constexpr uint32_t CentreBit  = 1u << uint32_t(SolidArcGizmoHandle::Centre);
    if (State.PickMode == SolidArcPickMode::ControlPoint)
    {
        // 📝 Mode 1: the picked sketch points, framed in their sketch plane: x / y cones, the xy square, the z arc (turn
        //    in the plane) and x / y cylinders (scale about the points' centre; a single point only moves).
        SolidArcSketchPointSession& Points = State.SketchPoints;
        const FeatureStep*          Step   = FindSketchStepByLabel(Document, Points.Sketch);
        if (!Step) { Points.Points.clear(); Points.Sketch.clear(); return; }
        const SketchModel& Sketch = SolvedSketchOf(Document, *Step);
        std::erase_if(Points.Points, [&](int Point) { return Point < 0 || Point >= int(Sketch.Points.size()); });
        if (Points.Points.empty()) { Points.Sketch.clear(); return; }
        const OrthonormalFrame Plane = ElevatedSketchPlane(Document, *Step);
        Vector3 Sum;
        for (int Point : Points.Points) Sum = Sum + PlacePlanePoint(Plane, Sketch.Points[Point].Position);
        constexpr auto Bit = [](SolidArcGizmoHandle Handle) { return 1u << uint32_t(Handle); };
        uint32_t Hidden = Bit(SolidArcGizmoHandle::TranslateZ) | Bit(SolidArcGizmoHandle::PlaneYZ) | Bit(SolidArcGizmoHandle::PlaneXZ) | Bit(SolidArcGizmoHandle::RotateX)
                        | Bit(SolidArcGizmoHandle::RotateY) | Bit(SolidArcGizmoHandle::ScaleZ);
        if (Points.Points.size() == 1) Hidden |= Bit(SolidArcGizmoHandle::RotateZ) | Bit(SolidArcGizmoHandle::ScaleX) | Bit(SolidArcGizmoHandle::ScaleY);
        if (!(Gizmo.Modes & GizmoTranslate) && (Points.Points.size() == 1 || !(Gizmo.Modes & GizmoScale))) Hidden |= CentreBit;
        Gizmo.Target       = SolidArcGizmoTarget::Points;
        Gizmo.Names        = { Points.Sketch };
        Gizmo.Points       = Points.Points;
        Gizmo.Pose.Origin  = Sum * (1.0 / double(Points.Points.size()));
        Gizmo.Pose.Axes[0] = Plane.XAxis;
        Gizmo.Pose.Axes[1] = Plane.YAxis;
        Gizmo.Pose.Axes[2] = Plane.ZAxis;
        Gizmo.Pose.Hidden  = Hidden;
        return;
    }
    if (State.PickMode == SolidArcPickMode::Face && !Document.Selection.Faces.empty())
    {
        Vector3 Centre, Normal;
        double  Area = 0.0;
        for (const SolidArcSceneFace& Face : State.Scene.Faces)
        {
            if (std::find(Document.Selection.Faces.begin(), Document.Selection.Faces.end(), Face.Name) == Document.Selection.Faces.end()) continue;
            for (uint32_t Corner = Face.FirstCorner; Corner + 2 < Face.FirstCorner + Face.CornerCount; Corner += 3)
            {
                const SolidArcSceneCorner* Triangle = &State.Scene.Corners[Corner];
                Vector3 Points[3], Normals;
                for (int Index = 0; Index < 3; ++Index)
                {
                    Points[Index] = { Triangle[Index].Position[0], Triangle[Index].Position[1], Triangle[Index].Position[2] };
                    Normals       = Normals + Vector3{ Triangle[Index].Normal[0], Triangle[Index].Normal[1], Triangle[Index].Normal[2] };
                }
                const double Piece = 0.5 * Length(Cross(Points[1] - Points[0], Points[2] - Points[0]));
                Centre = Centre + (Points[0] + Points[1] + Points[2]) * (Piece / 3.0);
                Normal = Normal + Normals * Piece;
                Area  += Piece;
            }
        }
        if (Area <= 0.0 || Length(Normal) < 1.0e-12) return;
        const Vector3 Up    = Normal * (1.0 / Length(Normal));
        const Vector3 Seed  = std::abs(Up.X) < 0.9 ? Vector3{ 1.0, 0.0, 0.0 } : Vector3{ 0.0, 1.0, 0.0 };
        const Vector3 Along = Normalize(Seed - Up * Dot(Seed, Up));
        Gizmo.Target       = SolidArcGizmoTarget::Faces;
        Gizmo.Names        = Document.Selection.Faces;
        Gizmo.Pose.Origin  = Centre * (1.0 / Area);
        Gizmo.Pose.Axes[0] = Along;
        Gizmo.Pose.Axes[1] = Cross(Up, Along);
        Gizmo.Pose.Axes[2] = Up;
        Gizmo.Pose.Hidden  = AxisScales | ((Gizmo.Modes & (GizmoTranslate | GizmoScale)) ? 0u : CentreBit);
        return;
    }
    // 📝 Edges alone raise no gizmo: they are picked for fillets, and a body gizmo would sit on the next Shift+click.
    const std::vector<std::string> Keys = (Document.Selection.Bodies.empty() && Document.Selection.Faces.empty()) ? std::vector<std::string>{} : PickedBodies(Document);
    if (!Keys.empty())
    {
        Gizmo.Target      = SolidArcGizmoTarget::Bodies;
        Gizmo.Names       = Keys;
        Gizmo.Pose.Origin = MeasureBodiesCentre(State.Scene, Keys);
        Gizmo.Pose.Hidden = (Gizmo.Modes & (GizmoTranslate | GizmoScale)) ? 0u : CentreBit;
        return;
    }
    const std::vector<std::string> Sketches = PickedSketches(Document);
    if (Sketches.empty()) return;
    Vector3 Sum;
    int     Count = 0;
    for (const std::string& Label : Sketches)
    {
        const auto Found = std::find_if(Document.Tree.Steps.begin(), Document.Tree.Steps.end(), [&](const FeatureStep& Step) { return Step.Label == Label; });
        const OrthonormalFrame Plane  = ElevatedSketchPlane(Document, *Found);
        const auto             Solved = Document.Regeneration.Sketches.find(Label);
        const SketchModel&     Sketch = Solved != Document.Regeneration.Sketches.end() ? Solved->second : Found->Sketch;
        if (Count == 0)
        {
            Gizmo.Pose.Axes[0] = Plane.XAxis;
            Gizmo.Pose.Axes[1] = Plane.YAxis;
            Gizmo.Pose.Axes[2] = Plane.ZAxis;
        }
        for (const SketchCurve& Curve : Sketch.Curves)
            for (const Vector2& Sample : SampleSketchCurve(Sketch, Curve)) { Sum = Sum + PlacePlanePoint(Plane, Sample); ++Count; }
    }
    if (Count == 0) return;
    Gizmo.Target      = SolidArcGizmoTarget::Sketches;
    Gizmo.Names       = Sketches;
    Gizmo.Pose.Origin = Sum * (1.0 / Count);
    Gizmo.Pose.Hidden = AxisScales | ((Gizmo.Modes & GizmoTranslate) ? 0u : CentreBit);
}

// 📝 Centre ring: on faces it scales whenever scale is on (faces have no axis cylinders); otherwise it moves in the
//    screen plane with move on, scales uniformly with scale alone.
[[nodiscard]] bool GizmoCentreScaleCondition(const SolidArcGizmoSession& Gizmo)
{
    if (!(Gizmo.Pose.Modes & GizmoScale)) return false;
    return Gizmo.Target == SolidArcGizmoTarget::Faces || !(Gizmo.Pose.Modes & GizmoTranslate);
}

[[nodiscard]] bool GizmoRotateHandle(SolidArcGizmoHandle Handle) noexcept
{
    return Handle >= SolidArcGizmoHandle::RotateX && Handle <= SolidArcGizmoHandle::RotateZ;
}

[[nodiscard]] bool GizmoScaleHandle(const SolidArcGizmoSession& Gizmo, SolidArcGizmoHandle Handle)
{
    return (Handle >= SolidArcGizmoHandle::ScaleX && Handle <= SolidArcGizmoHandle::ScaleZ) || (Handle == SolidArcGizmoHandle::Centre && GizmoCentreScaleCondition(Gizmo));
}

// The rigid / affine map of the current drag (bodies and sketches).
[[nodiscard]] AffineMap GizmoMap(const SolidArcGizmoSession& Gizmo)
{
    const SolidArcGizmoHandle Handle = Gizmo.Grab;
    const int                 Axis   = GizmoHandleAxis(Handle);
    if (GizmoRotateHandle(Handle)) return RotationMap(Gizmo.Pose.Origin, Gizmo.Pose.Axes[Axis], Gizmo.Angle);
    if (GizmoScaleHandle(Gizmo, Handle))
    {
        if (Axis < 0) return ScalingMap(Gizmo.Pose.Origin, Vector3{ Gizmo.Factor, Gizmo.Factor, Gizmo.Factor });
        // Along the frame axis a (world axes for bodies, the sketch plane axes for points): M = I + (f − 1)·a·aᵀ.
        const Vector3 Along = Gizmo.Pose.Axes[Axis];
        const double  Grow  = Gizmo.Factor - 1.0;
        AffineMap Map;
        Map.ColumnX = Vector3{ 1.0, 0.0, 0.0 } + Along * (Grow * Along.X);
        Map.ColumnY = Vector3{ 0.0, 1.0, 0.0 } + Along * (Grow * Along.Y);
        Map.ColumnZ = Vector3{ 0.0, 0.0, 1.0 } + Along * (Grow * Along.Z);
        const Vector3 Origin = Gizmo.Pose.Origin;
        Map.Shift = Origin - (Map.ColumnX * Origin.X + Map.ColumnY * Origin.Y + Map.ColumnZ * Origin.Z);
        return Map;
    }
    return TranslationMap(Gizmo.Shift);
}

[[nodiscard]] bool GizmoIdentityCondition(const SolidArcGizmoSession& Gizmo)
{
    if (GizmoRotateHandle(Gizmo.Grab)) return std::abs(Gizmo.Angle) < 1.0e-12;
    if (GizmoScaleHandle(Gizmo, Gizmo.Grab)) return std::abs(Gizmo.Factor - 1.0) < 1.0e-12;
    return Length(Gizmo.Shift) < 1.0e-12;
}

// 💡 One command per drag: bodies → Transform steps, sketches → plane moves, faces → face operations. The face along
//    its normal (z handle) is a push / pull (Offset); any other move is a Tweak along the shift; rotate turns each face
//    about the axis through its centre; scale scales each face about its centre.
[[nodiscard]] SolidArcCommandOutcome ApplyGizmoCommand(SolidArcDocument& Document, const SolidArcGizmoSession& Gizmo)
{
    const SolidArcGizmoHandle Handle = Gizmo.Grab;
    const int                 Axis   = GizmoHandleAxis(Handle);
    const char* Verb = GizmoRotateHandle(Handle) ? "Rotate" : GizmoScaleHandle(Gizmo, Handle) ? "Scale" : "Move";
    switch (Gizmo.Target)
    {
    case SolidArcGizmoTarget::Bodies:   return TransformSolidArcBodies(Document, Gizmo.Names, GizmoMap(Gizmo), Verb);
    case SolidArcGizmoTarget::Sketches: return PlaceSolidArcSketches(Document, Gizmo.Names, GizmoMap(Gizmo), Verb);
    case SolidArcGizmoTarget::Points:
    {
        // Each picked point mapped in the world, then carried back into its sketch plane.
        const FeatureStep* Step = Gizmo.Names.empty() ? nullptr : FindSketchStepByLabel(Document, Gizmo.Names[0]);
        if (!Step) break;
        const SketchModel&     Sketch = SolvedSketchOf(Document, *Step);
        const OrthonormalFrame Plane  = ElevatedSketchPlane(Document, *Step);
        const AffineMap        Map    = GizmoMap(Gizmo);
        std::vector<Vector2>   Targets;
        for (int Point : Gizmo.Points)
        {
            if (Point < 0 || Point >= int(Sketch.Points.size())) break;
            const Vector3 Start = PlacePlanePoint(Plane, Sketch.Points[Point].Position);
            const Vector3 Moved = Map.ColumnX * Start.X + Map.ColumnY * Start.Y + Map.ColumnZ * Start.Z + Map.Shift;
            Targets.push_back({ Dot(Moved - Plane.Origin, Plane.XAxis), Dot(Moved - Plane.Origin, Plane.YAxis) });
        }
        return DragSolidArcSketchPoints(Document, Gizmo.Names[0], Gizmo.Points, Targets);
    }
    case SolidArcGizmoTarget::Faces:
        if (GizmoRotateHandle(Handle)) return ChangeSolidArcSelection(Document, FaceOperationKind::Rotate, Gizmo.Angle, Gizmo.Pose.Axes[Axis]);
        if (GizmoScaleHandle(Gizmo, Handle)) return ChangeSolidArcSelection(Document, FaceOperationKind::Scale, Gizmo.Factor);
        if (Handle == SolidArcGizmoHandle::TranslateZ) return ChangeSolidArcSelection(Document, FaceOperationKind::Offset, Dot(Gizmo.Shift, Gizmo.Pose.Axes[2]));
        return ChangeSolidArcSelection(Document, FaceOperationKind::Tweak, Length(Gizmo.Shift), Normalize(Gizmo.Shift));
    case SolidArcGizmoTarget::None: break;
    }
    SolidArcCommandOutcome Refused;
    Refused.Refusal = "gizmo: nothing to transform";
    return Refused;
}

void CancelGizmoDrag(SolidArcWorkspaceState& State, const std::string& Message)
{
    SolidArcGizmoSession& Gizmo = State.Gizmo;
    Gizmo.DragCondition = false;
    Gizmo.Grab          = SolidArcGizmoHandle::None;
    Gizmo.Preview.reset();
    Gizmo.RigidCondition = false;
    Gizmo.StagedKey.clear();
    Gizmo.Refusal.clear();
    if (!Message.empty()) Announce(State, Message);
}

void CommitGizmoDrag(SolidArcWorkspaceState& State)
{
    SolidArcGizmoSession& Gizmo = State.Gizmo;
    if (GizmoIdentityCondition(Gizmo)) { CancelGizmoDrag(State, "Gizmo: nothing changed"); return; }
    const std::string            Readout = Gizmo.Readout;
    const SolidArcCommandOutcome Outcome = ApplyGizmoCommand(State.Document, Gizmo);
    CancelGizmoDrag(State, {});
    Conclude(State, Outcome);
    if (Outcome.AcceptedCondition)
    {
        ++Gizmo.Commits;
        Announce(State, Compose("Gizmo %s · %s · %.0f ms", Readout.c_str(), Outcome.Label.c_str(), Outcome.Milliseconds));
    }
}

// Pointer → value of the held handle (start frame fixed), readout, then one staged preview per distinct value.
void DriveGizmoDrag(SolidArcWorkspaceState& State, const OrbitCameraFrame& Frame, ImVec2 Local)
{
    SolidArcGizmoSession&    Gizmo = State.Gizmo;
    const SolidArcGizmoPose& Pose  = Gizmo.Pose;
    const ImGuiIO&           Io    = ImGui::GetIO();
    const bool               Snap  = !Io.KeyCtrl;                                            // Snapping on by default, Ctrl free
    const OrbitRay           Ray   = CastOrbitRay(Frame, Local.x, Local.y);
    const int                Axis  = GizmoHandleAxis(Gizmo.Grab);
    const Vector3            Forward{ Frame.Forward[0], Frame.Forward[1], Frame.Forward[2] };
    const auto Snapped = [&](double Value, double Step) { return Snap ? std::round(Value / Step) * Step : Value; };
    const char* Name = GizmoHandleLabel(Gizmo.Grab);
    if (GizmoRotateHandle(Gizmo.Grab))
    {
        if (const std::optional<double> Raw = MeasureGizmoAngle(Ray, Pose.Origin, Pose.Axes[Axis], Pose.Axes[(Axis + 1) % 3]))
        {
            double Step = *Raw - Gizmo.GrabValue;                                            // unwrap: the drag may pass ±π
            while (Step > std::numbers::pi)  Step -= 2.0 * std::numbers::pi;
            while (Step < -std::numbers::pi) Step += 2.0 * std::numbers::pi;
            Gizmo.Sweep    += Step;
            Gizmo.GrabValue    = *Raw;
        }
        Gizmo.Angle   = Snapped(Gizmo.Sweep * 180.0 / std::numbers::pi, TurnSnap) * std::numbers::pi / 180.0;
        Gizmo.Readout = Compose("%s %.1f°", Name, Gizmo.Angle * 180.0 / std::numbers::pi);
    }
    else if (GizmoScaleHandle(Gizmo, Gizmo.Grab))
    {
        double Ratio = 1.0;
        if (Axis >= 0)
        {
            const std::optional<double> Parameter = MeasureAxisParameter(Ray, Pose.Origin, Pose.Axes[Axis]);
            if (Parameter && std::abs(Gizmo.GrabValue) > 1.0e-12) Ratio = *Parameter / Gizmo.GrabValue;
        }
        else
        {
            ImVec2 Centre;
            if (ProjectWorldPoint(State, Frame, Pose.Origin, Centre))
            {
                const double First = std::hypot(Gizmo.GrabPixel.x + State.ViewportRect[0] - Centre.x, Gizmo.GrabPixel.y + State.ViewportRect[1] - Centre.y);
                const double Now   = std::hypot(Io.MousePos.x - Centre.x, Io.MousePos.y - Centre.y);
                if (First > 2.0) Ratio = Now / First;
            }
        }
        Gizmo.Factor  = std::max(0.05, Snapped(Ratio, FactorSnap));
        Gizmo.Readout = Compose("%s %.3fx", Axis >= 0 ? Name : "uniform scale", Gizmo.Factor);
    }
    else if (Gizmo.Grab >= SolidArcGizmoHandle::TranslateX && Gizmo.Grab <= SolidArcGizmoHandle::TranslateZ)
    {
        if (const std::optional<double> Parameter = MeasureAxisParameter(Ray, Pose.Origin, Pose.Axes[Axis]))
            Gizmo.Shift = Pose.Axes[Axis] * Snapped(*Parameter - Gizmo.GrabValue, MoveSnap);
        Gizmo.Readout = Compose("%s %s", Name, FormatLength(Dot(Gizmo.Shift, Pose.Axes[Axis])).c_str());
    }
    else
    {
        // Plane quad (normal = its axis) or the centre ring (screen plane): the hit slides from the press hit.
        const Vector3 Normal = Axis >= 0 ? Pose.Axes[Axis] : Forward;
        if (const std::optional<Vector3> Hit = IntersectGizmoPlane(Ray, Pose.Origin, Normal))
        {
            const Vector3 Slide = *Hit - Gizmo.GrabPoint;
            if (Axis >= 0)
            {
                const Vector3 U = Pose.Axes[(Axis + 1) % 3], V = Pose.Axes[(Axis + 2) % 3];
                Gizmo.Shift = U * Snapped(Dot(Slide, U), MoveSnap) + V * Snapped(Dot(Slide, V), MoveSnap);
            }
            else Gizmo.Shift = { Snapped(Slide.X, MoveSnap), Snapped(Slide.Y, MoveSnap), Snapped(Slide.Z, MoveSnap) };
        }
        Gizmo.Readout = Compose("%s (%s, %s, %s)", Axis >= 0 ? Name : "screen move", FormatLength(Gizmo.Shift.X).c_str(), FormatLength(Gizmo.Shift.Y).c_str(), FormatLength(Gizmo.Shift.Z).c_str());
    }
    if (Gizmo.Target == SolidArcGizmoTarget::Faces)
        Gizmo.Readout += GizmoRotateHandle(Gizmo.Grab) ? " · face rotate" : GizmoScaleHandle(Gizmo, Gizmo.Grab) ? " · face scale"
                       : Gizmo.Grab == SolidArcGizmoHandle::TranslateZ ? " · push / pull" : " · face tweak";
    if (Gizmo.Target == SolidArcGizmoTarget::Points) Gizmo.Readout += Compose(" · %d sketch point%s", int(Gizmo.Points.size()), Gizmo.Points.size() == 1 ? "" : "s");
    // Staged preview: one regeneration per distinct value.
    const std::string Key = Compose("%d %.12g %.12g %.12g %.12g %.12g", int(Gizmo.Grab), Gizmo.Shift.X, Gizmo.Shift.Y, Gizmo.Shift.Z, Gizmo.Angle, Gizmo.Factor);
    if (Key == Gizmo.StagedKey) return;
    Gizmo.StagedKey = Key;
    Gizmo.Refusal.clear();
    Gizmo.RigidCondition = false;
    if (GizmoIdentityCondition(Gizmo)) { Gizmo.Preview.reset(); return; }
    if (Gizmo.Target == SolidArcGizmoTarget::Bodies)
    {
        // Whole bodies: carry the shown triangles (the kernel runs once, on release).
        SolidArcPhaseClock RigidClock(State.Profile, "gizmo preview · rigid");
        Gizmo.Preview.reset();
        Gizmo.PreviewScene   = MapSceneBodies(State.Scene, Gizmo.Names, GizmoMap(Gizmo));
        Gizmo.RigidCondition = true;
        return;
    }
    FRONTIER_TRACE_SCOPE("Preview", "StageGizmoPreview · " + Gizmo.Readout);
    SolidArcPhaseClock StageClock(State.Profile, "gizmo preview · stage");
    Gizmo.Preview = std::make_unique<SolidArcDocument>(CloneSolidArcDocument(State.Document));
    const SolidArcCommandOutcome Outcome = ApplyGizmoCommand(*Gizmo.Preview, Gizmo);
    if (!Outcome.AcceptedCondition) { Gizmo.Refusal = Legible(Outcome.Refusal); Gizmo.Preview.reset(); return; }
    Gizmo.PreviewScene = BuildSolidArcScene(*Gizmo.Preview);
}

// 📝 Script evidence: each drawn handle is a widget ("GizmoTranslateZ", "GizmoPlaneXY", "GizmoRotateX", "GizmoScaleY",
//    "GizmoCentre") at a point that picks it — tips, cylinder middles, the plane quads' free corners, arc middles.
void RecordGizmoWidgets(SolidArcWorkspaceState& State, const OrbitCameraFrame& Frame)
{
    const SolidArcGizmoSession& Gizmo = State.Gizmo;
    if (Gizmo.Target == SolidArcGizmoTarget::None) return;
    uint32_t Drawn = 0;
    for (const SolidArcGizmoVertex& Vertex : Gizmo.Geometry.Vertices) Drawn |= 1u << Vertex.Handle;
    static const char* Keys[] = { "", "GizmoTranslateX", "GizmoTranslateY", "GizmoTranslateZ", "GizmoPlaneYZ", "GizmoPlaneXZ", "GizmoPlaneXY",
                                  "GizmoRotateX", "GizmoRotateY", "GizmoRotateZ", "GizmoScaleX", "GizmoScaleY", "GizmoScaleZ", "GizmoCentre" };
    const SolidArcGizmoPose& Pose = Gizmo.Pose;
    for (int Code = 1; Code <= 13; ++Code)
    {
        if (!(Drawn & (1u << Code))) continue;
        const int Axis = (Code - 1) % 3, U = (Axis + 1) % 3, V = (Axis + 2) % 3;
        Vector3 Local;
        if (Code <= 3)       Local = Pose.Axes[Axis] * 0.95;
        else if (Code <= 6)  Local = Pose.Axes[U] * 0.92 + Pose.Axes[V] * 0.82;
        else if (Code <= 9)  Local = (Pose.Axes[U] + Pose.Axes[V]) * (0.62 * 0.95 / std::numbers::sqrt2);
        else if (Code <= 12) Local = Pose.Axes[Axis] * 0.67;
        else                 Local = Vector3{ Frame.Right[0], Frame.Right[1], Frame.Right[2] } * 0.16;
        ImVec2 Pixel;
        if (ProjectWorldPoint(State, Frame, Pose.Origin + Local * Pose.Size, Pixel))
            State.Widgets.Record(Keys[Code], ImVec2(Pixel.x - 2.0f, Pixel.y - 2.0f), ImVec2(Pixel.x + 2.0f, Pixel.y + 2.0f));
    }
}

// 💡 Every viewport frame: resolve the target (unless a handle is held), size the gizmo to GizmoPixels on screen, hover
//    pick, press / drag / release; the triangles go to Style.Gizmo for the GPU and stay here for the pick. Returns true
//    when the gizmo owns the left button this frame (a held handle, or the pointer over one), so the selection click and
//    box selection stand aside.
bool DriveGizmo(SolidArcWorkspaceState& State, const OrbitCameraFrame& Frame, ImVec2 Local, bool PointerCondition)
{
    SolidArcGizmoSession& Gizmo = State.Gizmo;
    State.Style.Gizmo = {};
    Gizmo.Geometry    = {};
    if (!GizmoAvailableCondition(State))
    {
        if (Gizmo.DragCondition) CancelGizmoDrag(State, {});
        Gizmo.Hover  = SolidArcGizmoHandle::None;
        Gizmo.Target = SolidArcGizmoTarget::None;
        return false;
    }
    if (!Gizmo.DragCondition) ResolveGizmoTarget(State);
    if (Gizmo.Target == SolidArcGizmoTarget::None) { Gizmo.Hover = SolidArcGizmoHandle::None; return false; }
    Gizmo.Pose.Modes = Gizmo.Modes;
    Gizmo.Pose.Size  = MeasureGizmoSize(Frame, Gizmo.Pose.Origin, GizmoPixels);
    Gizmo.Geometry   = BuildSolidArcGizmo(Gizmo.Pose, Frame, Gizmo.DragCondition ? SolidArcGizmoHandle::None : Gizmo.Hover, Gizmo.Grab);
    bool Owned = false;
    if (!Gizmo.DragCondition)
    {
        Gizmo.Hover = PointerCondition && !ImGui::IsMouseDown(ImGuiMouseButton_Middle) && !ImGui::IsMouseDown(ImGuiMouseButton_Right)
                    ? PickSolidArcGizmo(Gizmo.Geometry, Frame, Local.x, Local.y, 6.0) : SolidArcGizmoHandle::None;
        Owned = Gizmo.Hover != SolidArcGizmoHandle::None;
        if (Owned && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            // Press: the handle's reference value at the pointer (axis parameter, angle, plane / screen hit).
            const OrbitRay Ray  = CastOrbitRay(Frame, Local.x, Local.y);
            const int      Axis = GizmoHandleAxis(Gizmo.Hover);
            const Vector3  Forward{ Frame.Forward[0], Frame.Forward[1], Frame.Forward[2] };
            Gizmo.DragCondition = true;
            Gizmo.Grab          = Gizmo.Hover;
            Gizmo.GrabPixel     = Local;
            Gizmo.Shift         = {};
            Gizmo.Angle         = 0.0;
            Gizmo.Sweep         = 0.0;
            Gizmo.Factor        = 1.0;
            Gizmo.GrabValue     = 0.0;
            Gizmo.GrabPoint     = Gizmo.Pose.Origin;
            Gizmo.StagedKey     = {};
            Gizmo.Refusal       = {};
            Gizmo.Readout       = GizmoHandleLabel(Gizmo.Grab);
            if (GizmoRotateHandle(Gizmo.Grab))
            {
                Gizmo.GrabValue = MeasureGizmoAngle(Ray, Gizmo.Pose.Origin, Gizmo.Pose.Axes[Axis], Gizmo.Pose.Axes[(Axis + 1) % 3]).value_or(0.0);
            }
            else if (Axis >= 0 && (Gizmo.Grab <= SolidArcGizmoHandle::TranslateZ || Gizmo.Grab >= SolidArcGizmoHandle::ScaleX))
                Gizmo.GrabValue = MeasureAxisParameter(Ray, Gizmo.Pose.Origin, Gizmo.Pose.Axes[Axis]).value_or(0.0);
            else
                Gizmo.GrabPoint = IntersectGizmoPlane(Ray, Gizmo.Pose.Origin, Axis >= 0 ? Gizmo.Pose.Axes[Axis] : Forward).value_or(Gizmo.Pose.Origin);
        }
    }
    if (Gizmo.DragCondition)
    {
        Owned = true;
        DriveGizmoDrag(State, Frame, Local);
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) || !ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            if (!Gizmo.Refusal.empty()) CancelGizmoDrag(State, "Refused: " + Gizmo.Refusal);
            else CommitGizmoDrag(State);
        }
        else if (Length(Gizmo.Shift) > 0.0)
        {
            // The gizmo rides with a move (axes stay fixed for the whole drag).
            SolidArcGizmoPose Riding = Gizmo.Pose;
            Riding.Origin = Riding.Origin + Gizmo.Shift;
            Gizmo.Geometry = BuildSolidArcGizmo(Riding, Frame, SolidArcGizmoHandle::None, Gizmo.Grab);
        }
    }
    if (Gizmo.Target != SolidArcGizmoTarget::None) State.Style.Gizmo = Gizmo.Geometry;
    if (!Gizmo.DragCondition) RecordGizmoWidgets(State, Frame);
    return Owned;
}

// y (free in Plasticity's map): off → move → rotate → scale → move + rotate → rotate + scale → all three → off.
void CycleGizmoModes(SolidArcWorkspaceState& State)
{
    static const uint32_t Cycle[] = { 0u, GizmoTranslate, GizmoRotate, GizmoScale, GizmoTranslate | GizmoRotate, GizmoRotate | GizmoScale, GizmoTranslate | GizmoRotate | GizmoScale };
    size_t Index = 0;
    while (Index < std::size(Cycle) && Cycle[Index] != State.Gizmo.Modes) ++Index;
    State.Gizmo.Modes = Cycle[(Index + 1) % std::size(Cycle)];
    Announce(State, "Gizmo: " + GizmoModesLabel(State.Gizmo.Modes));
}

[[nodiscard]] const char* SweepVerbName(SolidArcSweepVerb Verb)
{
    switch (Verb)
    {
    case SolidArcSweepVerb::Offset:    return "Offset";
    case SolidArcSweepVerb::Pipe:      return "Pipe";
    case SolidArcSweepVerb::Revolve:   return "Revolve";
    case SolidArcSweepVerb::Evolution: return "Evolution";
    }
    return "Sweep";
}

[[nodiscard]] const FeatureStep* SweepStep(const SolidArcWorkspaceState& State, const std::string& Label)
{
    for (const FeatureStep& Step : State.Document.Tree.Steps) if (Step.Label == Label) return &Step;
    return nullptr;
}

[[nodiscard]] const SketchModel* SweepSketch(const SolidArcWorkspaceState& State, const std::string& Label)
{
    const auto Solved = State.Document.Regeneration.Sketches.find(Label);
    return Solved == State.Document.Regeneration.Sketches.end() ? nullptr : &Solved->second;
}

// A sketch holding exactly one drawn open curve: a path, never a profile.
[[nodiscard]] bool SingleOpenCurveCondition(const SolidArcWorkspaceState& State, const std::string& Label)
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
[[nodiscard]] std::string SweepPath(const SolidArcWorkspaceState& State, SolidArcSweepVerb Verb)
{
    const bool ProfileCondition = Verb == SolidArcSweepVerb::Revolve || Verb == SolidArcSweepVerb::Evolution;
    if (FocusedSketchCondition(State) && !(ProfileCondition && SingleOpenCurveCondition(State, State.FocusedStep))) return State.FocusedStep;
    if (Verb == SolidArcSweepVerb::Pipe && !State.Document.Selection.Curves.empty()) return State.Document.Selection.Curves.front();
    for (auto Step = State.Document.Tree.Steps.rbegin(); Step != State.Document.Tree.Steps.rend(); ++Step)
        if (Step->Kind == FeatureStepKind::Sketch && !(ProfileCondition && SingleOpenCurveCondition(State, Step->Label))) return Step->Label;
    return {};
}

void OpenSweepSession(SolidArcWorkspaceState& State, SolidArcSweepVerb Verb)
{
    const std::string Path = SweepPath(State, Verb);
    if (Path.empty())
    {
        Announce(State, Verb == SolidArcSweepVerb::Offset ? "Offset (O) needs a sketch: draw one first"
                        : Verb == SolidArcSweepVerb::Pipe ? "Pipe (P) needs a path: draw a line / arc / curve or select a curve"
                        : Verb == SolidArcSweepVerb::Revolve ? "Revolve (Alt+R) needs a profile: draw a closed sketch first"
                                                             : "Evolution (Shift+P) needs a profile: draw a closed sketch first");
        return;
    }
    SolidArcSweepSession& Sweep = State.Sweep;
    const int Commits = Sweep.Commits;
    Sweep                 = {};
    Sweep.Commits         = Commits;
    Sweep.ActiveCondition = true;
    Sweep.Verb            = Verb;
    Sweep.Path            = Path;
    Sweep.Amount          = Verb == SolidArcSweepVerb::Offset ? 0.1 : Verb == SolidArcSweepVerb::Revolve ? 360.0 : 0.05;
    Sweep.Drift           = Sweep.Amount;
    Sweep.Combination     = State.Document.Regeneration.Bodies.empty() ? FeatureCombination::New : FeatureCombination::Separate;
    State.Drawing.ActiveCondition = false;
    State.Trim.ActiveCondition    = false;
    State.Fill.ActiveCondition    = false;
    State.Constrain.ActiveCondition = false;
    const char* Booleans = " · Q union · W difference · E intersect · R new body";
    if (Verb == SolidArcSweepVerb::Revolve)
    {
        Sweep.Axis = ProposeSolidArcRevolveAxis(State.Document, Path);
        Announce(State, Compose("Revolve %s about %s · type degrees (360) · X / Y sketch axis · click one of its lines for the axis%s · click / Enter commits · Esc cancels",
                                CaptionOf(State.Document, Path).c_str(), DescribeSolidArcRevolveAxis(State.Document, Path, Sweep.Axis).c_str(), Booleans));
        return;
    }
    if (Verb == SolidArcSweepVerb::Evolution)
    {
        Sweep.Profile = Path;
        Sweep.Path    = FocusedSketchCondition(State) && SingleOpenCurveCondition(State, State.FocusedStep) ? State.FocusedStep : ProposeSolidArcEvolutionPath(State.Document, Path);
        Announce(State, Compose("Evolution %s along %s · click another sketch's curve for the path%s · click / Enter commits · Esc cancels", CaptionOf(State.Document, Path).c_str(),
                                Sweep.Path.empty() ? "? (no path yet)" : CaptionOf(State.Document, Sweep.Path).c_str(), Booleans));
        return;
    }
    Announce(State, Compose("%s %s · move sideways (Shift fine, Ctrl free) or type a value%s · click / Enter commits · Esc cancels", SweepVerbName(Verb),
                            CaptionOf(State.Document, Path).c_str(), Verb == SolidArcSweepVerb::Pipe ? Booleans : ""));
}

void CancelSweepSession(SolidArcWorkspaceState& State)
{
    State.Sweep.ActiveCondition = false;
    State.Sweep.Preview.reset();
    Announce(State, std::string(SweepVerbName(State.Sweep.Verb)) + " cancelled");
}

void ChooseSweepCombination(SolidArcWorkspaceState& State, FeatureCombination Combination)
{
    if (State.Sweep.Verb == SolidArcSweepVerb::Offset) return;
    State.Sweep.Combination = State.Document.Regeneration.Bodies.empty() ? FeatureCombination::New : Combination;
    Announce(State, std::string(SweepVerbName(State.Sweep.Verb)) + ": " + (State.Sweep.Combination == FeatureCombination::New ? "New" : FeatureCombinationLabel(State.Sweep.Combination)));
}

// Revolve: X / Y turn about the sketch x- / y-axis.
void ChooseRevolveAxis(SolidArcWorkspaceState& State, Vector2 Direction)
{
    if (State.Sweep.Verb != SolidArcSweepVerb::Revolve) return;
    State.Sweep.Axis = { -1, { 0.0, 0.0 }, Direction };
    Announce(State, "Revolve about the " + DescribeSolidArcRevolveAxis(State.Document, State.Sweep.Path, State.Sweep.Axis));
}

[[nodiscard]] SolidArcCommandOutcome RunSweepVerb(const SolidArcWorkspaceState& State, SolidArcDocument& Document)
{
    const SolidArcSweepSession& Sweep = State.Sweep;
    switch (Sweep.Verb)
    {
    case SolidArcSweepVerb::Offset:    return OffsetSolidArcSketch(Document, Sweep.Path, Sweep.Amount);
    case SolidArcSweepVerb::Pipe:      return PipeSolidArcPath(Document, Sweep.Path, Sweep.Amount, Sweep.Combination, BooleanTarget(State));
    case SolidArcSweepVerb::Revolve:   return RevolveSolidArcSketch(Document, Sweep.Path, Sweep.Axis, Sweep.Amount * std::numbers::pi / 180.0, Sweep.Combination, BooleanTarget(State));
    case SolidArcSweepVerb::Evolution: return EvolveSolidArcProfile(Document, Sweep.Profile, Sweep.Path, Sweep.Combination, BooleanTarget(State));
    }
    return {};
}

void ApplySweepSession(SolidArcWorkspaceState& State)
{
    SolidArcSweepSession& Sweep = State.Sweep;
    if (!Sweep.Refusal.empty()) { Announce(State, "Refused: " + Sweep.Refusal); return; }
    const SolidArcCommandOutcome Outcome = RunSweepVerb(State, State.Document);
    if (Outcome.AcceptedCondition)
    {
        ++Sweep.Commits;
        Sweep.ActiveCondition = false;
        Sweep.Preview.reset();
        if (Sweep.Verb == SolidArcSweepVerb::Offset) State.FocusedStep = State.Document.Tree.Steps.back().Label;   // O again offsets the offset
        else
        {
            State.FocusedStep = {};
            State.PickMode    = SolidArcPickMode::Solid;
            if (State.Document.Tree.Steps.back().Combination == FeatureCombination::Separate || State.Document.Tree.Steps.back().Combination == FeatureCombination::New)
                SelectSolidArcName(State.Document, SolidArcSelectionKind::Body, State.Document.Tree.Steps.back().Label, false);
        }
    }
    Conclude(State, Outcome);
}

// Revolve / Evolution left-click: a line of the profile under the pointer becomes the axis, another sketch's curve the
// path; a click anywhere else commits.
void ClickSweepSession(SolidArcWorkspaceState& State)
{
    SolidArcSweepSession& Sweep = State.Sweep;
    const SketchModel* Sketch = Sweep.HoverCurve >= 0 ? SweepSketch(State, Sweep.HoverSketch) : nullptr;
    if (Sketch && Sweep.HoverCurve < int(Sketch->Curves.size()))
    {
        const SketchCurve& Curve = Sketch->Curves[Sweep.HoverCurve];
        if (Sweep.Verb == SolidArcSweepVerb::Revolve)
        {
            Sweep.Axis = { Curve.Identifier, {}, { 0.0, 1.0 } };
            Announce(State, "Revolve about " + SketchCurveName(Curve));
            return;
        }
        if (Sweep.Verb == SolidArcSweepVerb::Evolution)
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
void HoverSweepCurve(SolidArcWorkspaceState& State, const OrbitCameraFrame& Frame, bool HoveredCondition)
{
    SolidArcSweepSession& Sweep = State.Sweep;
    Sweep.HoverSketch.clear();
    Sweep.HoverCurve = -1;
    if (!HoveredCondition) return;
    const ImVec2 Mouse   = ImGui::GetIO().MousePos;
    double       Nearest = 8.0;                                                              // [px] - Pick radius
    for (const FeatureStep& Step : State.Document.Tree.Steps)
    {
        if (Step.Kind != FeatureStepKind::Sketch) continue;
        const bool Candidate = Sweep.Verb == SolidArcSweepVerb::Revolve ? Step.Label == Sweep.Path : Step.Label != Sweep.Profile;
        const SketchModel* Sketch = Candidate ? SweepSketch(State, Step.Label) : nullptr;
        if (!Sketch) continue;
        const OrthonormalFrame Plane = ElevatedSketchPlane(State.Document, Step);
        for (int Curve = 0; Curve < int(Sketch->Curves.size()); ++Curve)
        {
            const SketchCurve& Each = Sketch->Curves[Curve];
            if (Sweep.Verb == SolidArcSweepVerb::Revolve ? Each.Kind != SketchCurveKind::Line : Each.Construction) continue;
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

void DriveSweepSession(SolidArcWorkspaceState& State, const OrbitCameraFrame& Frame, bool HoveredCondition)
{
    SolidArcSweepSession& Sweep = State.Sweep;
    ImGuiIO& Io = ImGui::GetIO();
    double Typed = 0.0;
    if (Sweep.Verb == SolidArcSweepVerb::Revolve || Sweep.Verb == SolidArcSweepVerb::Evolution)
    {
        HoverSweepCurve(State, Frame, HoveredCondition);                                    // As trim: the click frame keeps its hover
        if (Sweep.Verb == SolidArcSweepVerb::Revolve) Sweep.Amount = ParseTypedAmount(Sweep.Typed, Typed) ? Typed : 360.0;
        const std::string Key = Compose("%.17g|%d|%d|%.17g|%.17g|%.17g|%.17g|%s|%s|%s", Sweep.Amount, int(Sweep.Combination), Sweep.Axis.Line, Sweep.Axis.Origin.X, Sweep.Axis.Origin.Y,
                                        Sweep.Axis.Direction.X, Sweep.Axis.Direction.Y, Sweep.Path.c_str(), Sweep.Profile.c_str(), BooleanTarget(State).c_str());
        if (Key == Sweep.StagedKey) return;
        Sweep.StagedKey = Key;
        Sweep.Refusal.clear();
        Sweep.Preview = std::make_unique<SolidArcDocument>(CloneSolidArcDocument(State.Document));
        const SolidArcCommandOutcome Outcome = RunSweepVerb(State, *Sweep.Preview);
        if (!Outcome.AcceptedCondition) { Sweep.Refusal = Legible(Outcome.Refusal); Sweep.Preview.reset(); return; }
        Sweep.PreviewScene = BuildSolidArcScene(*Sweep.Preview);
        return;
    }
    if (ParseTypedAmount(Sweep.Typed, Typed)) Sweep.Amount = Typed;
    else
    {
        if (HoveredCondition && !ImGui::IsAnyMouseDown() && Io.MouseDelta.x != 0.0f) Sweep.Drift += Io.MouseDelta.x * Frame.WorldPerPixel * (Io.KeyShift ? 0.1 : 1.0) * 0.5;
        Sweep.Amount = Io.KeyCtrl ? Sweep.Drift : std::round(Sweep.Drift / SweepSnap) * SweepSnap;
    }
    if (Sweep.Verb == SolidArcSweepVerb::Pipe) Sweep.Amount = std::max(Sweep.Amount, 0.001);
    if (Sweep.Amount == Sweep.StagedAmount && Sweep.Combination == Sweep.StagedCombination) return;
    Sweep.StagedAmount      = Sweep.Amount;
    Sweep.StagedCombination = Sweep.Combination;
    Sweep.Refusal.clear();
    Sweep.Outline.clear();
    if (Sweep.Verb == SolidArcSweepVerb::Offset)
    {
        const SketchOffsetOutcome Outcome = PreviewSolidArcOffset(State.Document, Sweep.Path, Sweep.Amount);
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
    Sweep.Preview = std::make_unique<SolidArcDocument>(CloneSolidArcDocument(State.Document));
    const SolidArcCommandOutcome Outcome = RunSweepVerb(State, *Sweep.Preview);
    if (!Outcome.AcceptedCondition) { Sweep.Refusal = Legible(Outcome.Refusal); Sweep.Preview.reset(); return; }
    Sweep.PreviewScene = BuildSolidArcScene(*Sweep.Preview);
}

// One sketch curve in screen pixels (empty when the step or curve is gone).
[[nodiscard]] std::vector<ImVec2> SweepCurvePixels(SolidArcWorkspaceState& State, const OrbitCameraFrame& Frame, const std::string& Label, int Curve)
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

void DrawSweepOutline(SolidArcWorkspaceState& State, ImDrawList* Canvas, const OrbitCameraFrame& Frame)
{
    if (!State.Sweep.ActiveCondition) return;
    const SolidArcSweepSession& Sweep = State.Sweep;
    for (const std::vector<Vector3>& Line : Sweep.Outline)
    {
        std::vector<ImVec2> Pixels;
        for (const Vector3& Point : Line) { ImVec2 Pixel; if (ProjectWorldPoint(State, Frame, Point, Pixel)) Pixels.push_back(Pixel); }
        if (Pixels.size() >= 2) Canvas->AddPolyline(Pixels.data(), int(Pixels.size()), IM_COL32(0x4d, 0xff, 0x9f, 0xff), 3.0f);
    }
    constexpr ImU32 AxisColour  = IM_COL32(0xff, 0xc8, 0x3d, 0xff);                         // Amber: axis / path
    constexpr ImU32 HoverColour = IM_COL32(0x4d, 0xd2, 0xff, 0xff);                         // Cyan: what a click takes
    if (Sweep.Verb == SolidArcSweepVerb::Revolve)
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
    if (Sweep.Verb == SolidArcSweepVerb::Evolution && !Sweep.Path.empty())
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

// Picked sketches (Shift-click in the viewport / Outliner), in pick order; space curves are left out.
[[nodiscard]] std::vector<std::string> SelectedSketchLabels(const SolidArcDocument& Document)
{
    std::vector<std::string> Labels;
    for (const std::string& Label : Document.Selection.Curves)
    {
        const auto Found = std::find_if(Document.Tree.Steps.begin(), Document.Tree.Steps.end(), [&](const FeatureStep& Step) { return Step.Label == Label; });
        if (Found != Document.Tree.Steps.end() && Found->Kind == FeatureStepKind::Sketch && std::find(Labels.begin(), Labels.end(), Label) == Labels.end()) Labels.push_back(Label);
    }
    return Labels;
}

// J / Inspector Join: two or more picked sketches join into the first picked (touching or not: one sketch, several
// profiles); otherwise the focused sketch (else the newest) welds with every coplanar sketch touching it.
void JoinSketches(SolidArcWorkspaceState& State)
{
    const std::vector<std::string> Picked = SelectedSketchLabels(State.Document);
    if (Picked.size() >= 2)
    {
        const SolidArcCommandOutcome Outcome = JoinSolidArcSketches(State.Document, Picked);
        if (Outcome.AcceptedCondition)
        {
            // The first picked sketch carries the join, unless a later step reads it (kept hidden): then the join is the
            // new last step.
            State.FocusedStep = SolidArcCurveHiddenCondition(State.Document, Picked.front()) ? State.Document.Tree.Steps.back().Label : Picked.front();
            State.Document.Selection.Curves = { State.FocusedStep };
        }
        Conclude(State, Outcome);
        return;
    }
    std::string Base = FocusedSketchCondition(State) ? State.FocusedStep : std::string{};
    if (Base.empty()) for (auto Step = State.Document.Tree.Steps.rbegin(); Step != State.Document.Tree.Steps.rend() && Base.empty(); ++Step) if (Step->Kind == FeatureStepKind::Sketch) Base = Step->Label;
    if (Base.empty()) { Announce(State, "Join (J) needs sketches: draw some first"); return; }
    const std::vector<std::string> Group = SolidArcJoinCandidates(State.Document, Base);
    if (Group.size() < 2) { Announce(State, "Join (J): no other sketch touches " + CaptionOf(State.Document, Base) + " (draw from its ends, or across it)"); return; }
    const SolidArcCommandOutcome Outcome = JoinSolidArcSketches(State.Document, Group);
    if (Outcome.AcceptedCondition) State.FocusedStep = Base;
    Conclude(State, Outcome);
}

// H / Shift+H / Alt+H / Ctrl+H: visibility is appearance (no regeneration) but the scene drops hidden bodies.
void ChangeVisibility(SolidArcWorkspaceState& State, SolidArcVisibilityVerb Verb)
{
    const SolidArcCommandOutcome Outcome = ChangeSolidArcVisibility(State.Document, Verb);
    if (Outcome.AcceptedCondition)
    {
        RefreshSolidArcScene(State, true);
        Announce(State, Compose("%s · %d hidden", Outcome.Label.c_str(), int(State.Document.Appearance.HiddenBodies.size())));
    }
    else Announce(State, "Refused: " + Outcome.Refusal);
}

// Ctrl+C: remember the picked bodies and the step they stand after; Ctrl+V pastes them in place as they stood then.
void CopyBodies(SolidArcWorkspaceState& State)
{
    const std::vector<std::string> Keys = PickedBodies(State.Document);
    if (Keys.empty() || State.Document.Tree.Steps.empty()) { Announce(State, "Copy (Ctrl+C) needs picked bodies (mode 4)"); return; }
    State.Clipboard = { Keys, State.Document.Tree.Steps.back().Label };
    std::string Captions;
    for (const std::string& Key : Keys) Captions += (Captions.empty() ? "" : ", ") + SolidArcBodyCaption(State.Document, Key);
    Announce(State, "Copied " + Captions);
}

void PasteBodies(SolidArcWorkspaceState& State)
{
    const SolidArcCommandOutcome Outcome = DuplicateSolidArcBodies(State.Document, State.Clipboard.Keys, TranslationMap({}), State.Clipboard.Source.empty() ? std::string("—") : State.Clipboard.Source);
    if (Outcome.AcceptedCondition) State.PickMode = SolidArcPickMode::Solid;
    Conclude(State, Outcome);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    KNIFE
//------------------------------------------------------------------------------------------------------------------------

SolidArcPickIntent PickIntentOf(const ImGuiIO& Io);
void               PickAtPointer(SolidArcWorkspaceState& State, ImVec2 Local, SolidArcPickIntent Intent);

[[nodiscard]] double KnifeClock()
{
    return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

[[nodiscard]] const char* KnifeModeLabel(SolidArcKnifeMode Mode) noexcept
{
    switch (Mode)
    {
    case SolidArcKnifeMode::Sketch: return "Sketch";
    case SolidArcKnifeMode::Insert: return "Insert";
    case SolidArcKnifeMode::Object: return "Object";
    }
    return "Sketch";
}

[[nodiscard]] const char* KnifeCombinationLabel(FeatureCombination Combination) noexcept
{
    switch (Combination)
    {
    case FeatureCombination::Union:        return "Add";
    case FeatureCombination::Intersection: return "Intersect";
    case FeatureCombination::Separate:     return "Separate";
    default:                               return "Cut";
    }
}

// Box of one scene body's facets; false when the scene does not show it.
bool MeasureSceneBody(const SolidArcScene& Scene, const std::string& Key, Vector3& Low, Vector3& High)
{
    const auto Body = std::find_if(Scene.Bodies.begin(), Scene.Bodies.end(), [&](const SolidArcSceneBody& Each) { return Each.Key == Key; });
    if (Body == Scene.Bodies.end() || Body->FaceCount == 0) return false;
    Low  = { 1.0e300, 1.0e300, 1.0e300 };
    High = { -1.0e300, -1.0e300, -1.0e300 };
    for (uint32_t Face = Body->FirstFace; Face < Body->FirstFace + Body->FaceCount; ++Face)
        for (uint32_t Corner = Scene.Faces[Face].FirstCorner; Corner < Scene.Faces[Face].FirstCorner + Scene.Faces[Face].CornerCount; ++Corner)
        {
            const float* Point = Scene.Corners[Corner].Position;
            Low  = { std::min(Low.X, double(Point[0])), std::min(Low.Y, double(Point[1])), std::min(Low.Z, double(Point[2])) };
            High = { std::max(High.X, double(Point[0])), std::max(High.Y, double(Point[1])), std::max(High.Z, double(Point[2])) };
        }
    return true;
}

// The body key of the face a scene pick landed on ("" for edges, caps and background).
[[nodiscard]] std::string PickedSceneBody(const SolidArcScene& Scene, const std::optional<SolidArcPick>& Pick)
{
    if (!Pick || Pick->Identity == 0 || Pick->Identity == IdentitySectionCap || (Pick->Identity & IdentityEdgeBit)) return {};
    const uint32_t Face = Pick->Identity - 1u;
    if (Face >= Scene.Faces.size() || Scene.Faces[Face].Body >= Scene.Bodies.size()) return {};
    return Scene.Bodies[Scene.Faces[Face].Body].Key;
}

// 📝 Lift of a cutter top above its plane: 1 % of the target's size (≥ 1 mm) so a flush face is cleared, never touched.
[[nodiscard]] double KnifeLift(const SolidArcScene& Scene, const std::string& Target)
{
    Vector3 Low, High;
    if (!MeasureSceneBody(Scene, Target, Low, High)) return 1.0e-3;
    return std::max(1.0e-3, 0.01 * std::max({ High.X - Low.X, High.Y - Low.Y, High.Z - Low.Z }));
}

// 💡 The body a sketch cutter cuts: the owner of the face it was drawn on, else the first body met by a ray down the
//    sketch normal through the middle of its curves (from just above the plane, then from outside the scene), else the
//    first picked body, else the only body.
[[nodiscard]] std::string KnifeSketchTarget(const SolidArcWorkspaceState& State, const std::string& Label)
{
    const SolidArcKnifeSession& Knife = State.Knife;
    if (!Knife.Face.empty())
    {
        const std::string Owner = SolidArcBodyOfName(State.Document, SolidArcSelectionKind::Face, Knife.Face);
        if (!Owner.empty()) return Owner;
    }
    const auto Step   = std::find_if(State.Document.Tree.Steps.begin(), State.Document.Tree.Steps.end(), [&](const FeatureStep& Each) { return Each.Label == Label; });
    const auto Solved = State.Document.Regeneration.Sketches.find(Label);
    if (Step != State.Document.Tree.Steps.end() && Solved != State.Document.Regeneration.Sketches.end())
    {
        Vector2 Sum{};
        int     Count = 0;
        for (const SketchCurve& Curve : Solved->second.Curves)
        {
            if (Curve.Construction) continue;
            for (const Vector2& Sample : SampleSketchCurve(Solved->second, Curve)) { Sum = Sum + Sample; ++Count; }
        }
        if (Count > 0)
        {
            const OrthonormalFrame Plane  = ElevatedSketchPlane(State.Document, *Step);
            const Vector3          Middle = PlacePlanePoint(Plane, Sum * (1.0 / Count));
            const double           Reach  = 2.0 * std::hypot(State.Scene.BoundsHigh[0] - State.Scene.BoundsLow[0], State.Scene.BoundsHigh[1] - State.Scene.BoundsLow[1],
                                                             State.Scene.BoundsHigh[2] - State.Scene.BoundsLow[2]) + 1.0;
            for (const double Height : { 1.0e-3, Reach })
            {
                const Vector3 Start = Middle + Plane.ZAxis * Height;
                OrbitRay      Ray;
                Ray.Origin    = { Start.X, Start.Y, Start.Z };
                Ray.Direction = { -Plane.ZAxis.X, -Plane.ZAxis.Y, -Plane.ZAxis.Z };
                const std::string Owner = PickedSceneBody(State.Scene, PickSolidArcScene(State.Scene, Ray, 0.0, false, nullptr));
                if (!Owner.empty()) return Owner;
            }
        }
    }
    if (!BooleanTarget(State).empty()) return BooleanTarget(State);
    return State.Scene.Bodies.size() == 1 ? State.Scene.Bodies.front().Key : std::string{};
}

// Sketch cutter span along the plane normal: Cut / Intersect / Separate from a lift above the plane down to the depth
// (through: past both sides of the target); Add rises from just inside the face up to the depth (a boss).
void KnifeSketchSpan(const SolidArcWorkspaceState& State, const std::string& Target, double& Top, double& Bottom)
{
    const SolidArcKnifeSession& Knife = State.Knife;
    const double Lift = KnifeLift(State.Scene, Target);
    Top    = Lift;
    Bottom = -Knife.Depth;
    if (Knife.Combination == FeatureCombination::Union) { Top = Knife.Depth; Bottom = -Lift; return; }
    if (!Knife.ThroughCondition) return;
    const auto Step = std::find_if(State.Document.Tree.Steps.begin(), State.Document.Tree.Steps.end(), [&](const FeatureStep& Each) { return Each.Label == Knife.Sketch; });
    const auto Body = std::find_if(State.Scene.Bodies.begin(), State.Scene.Bodies.end(), [&](const SolidArcSceneBody& Each) { return Each.Key == Target; });
    if (Step == State.Document.Tree.Steps.end() || Body == State.Scene.Bodies.end()) return;
    const OrthonormalFrame Plane = ElevatedSketchPlane(State.Document, *Step);
    double Above = 0.0, Below = 0.0;                                                         // [len] - Body reach either side of the plane
    for (uint32_t Face = Body->FirstFace; Face < Body->FirstFace + Body->FaceCount; ++Face)
        for (uint32_t Corner = State.Scene.Faces[Face].FirstCorner; Corner < State.Scene.Faces[Face].FirstCorner + State.Scene.Faces[Face].CornerCount; ++Corner)
        {
            const float*  Point = State.Scene.Corners[Corner].Position;
            const double  Along = Dot(Vector3{ Point[0], Point[1], Point[2] } - Plane.Origin, Plane.ZAxis);
            Above = std::max(Above, Along);
            Below = std::min(Below, Along);
        }
    const double Margin = Lift + 0.02 * (Above - Below);
    Top    = Above + Margin;
    Bottom = Below - Margin;
}

[[nodiscard]] SolidArcCommandOutcome RefusedKnife(const std::string& Reason)
{
    SolidArcCommandOutcome Outcome;
    Outcome.Refusal = Reason;
    return Outcome;
}

// The knife as it stands, applied to Document (the workspace document or a preview clone).
[[nodiscard]] SolidArcCommandOutcome RunKnife(const SolidArcWorkspaceState& State, SolidArcDocument& Document)
{
    const SolidArcKnifeSession& Knife = State.Knife;
    switch (Knife.Mode)
    {
    case SolidArcKnifeMode::Sketch:
    {
        if (Knife.Sketch.empty()) return RefusedKnife("draw a closed shape for the knife (Circle / Rectangle / Polygon / Slot / Line) or focus a closed sketch");
        const std::string Target = KnifeSketchTarget(State, Knife.Sketch);
        double Top = 0.0, Bottom = 0.0;
        KnifeSketchSpan(State, Target, Top, Bottom);
        return KnifeSolidArcSketch(Document, Knife.Sketch, Top, Bottom, Knife.Combination, Target);
    }
    case SolidArcKnifeMode::Insert:
    {
        if (Knife.Insert < 0 || Knife.Insert >= int(Knife.Library.Inserts.size())) return RefusedKnife("pick an insert from the library");
        if (!Knife.PlacedCondition && !Knife.HoverCondition) return RefusedKnife("point at a body face to place the insert");
        const SolidArcInsert&   Insert = Knife.Library.Inserts[Knife.Insert];
        SolidArcInsertPlacement Placement;
        Placement.Name        = Insert.Name;
        Placement.Exchange    = Insert.Exchange;
        Placement.Map         = PlaceSolidArcInsert(Knife.Spot, Knife.Size, Knife.Turn, Knife.Combination == FeatureCombination::Union);
        Placement.Combination = Knife.Combination;
        Placement.Target      = Knife.SpotBody;
        return KnifeSolidArcInsert(Document, Placement);
    }
    case SolidArcKnifeMode::Object:
    {
        const std::vector<std::string>& Picked = State.Document.Selection.Bodies;
        if (Picked.size() < 2) return RefusedKnife("pick the body to cut, then Shift-pick the cutter bodies (mode 4)");
        if (Knife.Combination == FeatureCombination::Separate) return RefusedKnife("Separate is for sketch and insert cutters (Q / W / E here)");
        return KnifeSolidArcBodies(Document, Picked.front(), std::vector<std::string>(Picked.begin() + 1, Picked.end()), Knife.Combination, Knife.KeepCondition);
    }
    }
    return RefusedKnife("unknown knife mode");
}

void EnsureKnifeLibrary(SolidArcWorkspaceState& State)
{
    SolidArcKnifeSession& Knife = State.Knife;
    if (Knife.LibraryCondition) return;
    FRONTIER_TRACE_SCOPE("Knife", "library");
    Knife.Library          = AssembleSolidArcInserts(State.Options.InsertFolder);
    Knife.LibraryCondition = true;
    Knife.Insert           = std::clamp(Knife.Insert, 0, std::max(0, int(Knife.Library.Inserts.size()) - 1));
    for (const std::string& Note : Knife.Library.Notes) FRONTIER_TRACE("Knife", "insert skipped: " + Note);
}

void DropKnifePreview(SolidArcKnifeSession& Knife)
{
    Knife.Preview.reset();
    Knife.StagedKey.clear();
    Knife.Refusal.clear();
}

void ChooseKnifeMode(SolidArcWorkspaceState& State, SolidArcKnifeMode Mode)
{
    SolidArcKnifeSession& Knife = State.Knife;
    Knife.Mode = Mode;
    DropKnifePreview(Knife);
    if (Mode != SolidArcKnifeMode::Sketch && Knife.DrawPendingCondition)
    {
        Knife.DrawPendingCondition    = false;
        State.Drawing.ActiveCondition = false;
    }
    if (Mode == SolidArcKnifeMode::Insert)
    {
        EnsureKnifeLibrary(State);
        Knife.PlacedCondition = false;
        Knife.Typed.clear();
        Announce(State, Compose("Knife · Insert %s · point at a face, click to place, move to size, [ / ] turn · %d insert(s) in %.0f ms",
                                Knife.Library.Inserts.empty() ? "(none)" : Knife.Library.Inserts[Knife.Insert].Name.c_str(), int(Knife.Library.Inserts.size()), Knife.Library.Milliseconds));
        return;
    }
    if (Mode == SolidArcKnifeMode::Object)
    {
        if (Knife.Combination == FeatureCombination::Separate) Knife.Combination = FeatureCombination::Difference;
        Announce(State, "Knife · Object · the first picked body is cut by the others (Shift-pick cutters) · K keeps the cutters · click / right-click / Enter applies");
        return;
    }
    Announce(State, Knife.Sketch.empty() ? "Knife · Sketch · draw a closed shape on a face (card: Circle / Rectangle / Polygon / Slot / Line)"
                                         : "Knife · Sketch " + CaptionOf(State.Document, Knife.Sketch) + " · move sideways for the depth · T through");
}

void ChooseKnifeCombination(SolidArcWorkspaceState& State, FeatureCombination Combination)
{
    if (State.Knife.Mode == SolidArcKnifeMode::Object && Combination == FeatureCombination::Separate) return;
    State.Knife.Combination = Combination;
    Announce(State, std::string("Knife: ") + KnifeCombinationLabel(Combination));
}

// Starts a drawing for the knife: the shape is filled, and once finished becomes the knife's cutter on that face.
void DrawKnifeShape(SolidArcWorkspaceState& State, SolidArcDrawingKind Kind)
{
    SolidArcKnifeSession& Knife = State.Knife;
    Knife.Mode = SolidArcKnifeMode::Sketch;
    Knife.Sketch.clear();
    Knife.Face.clear();
    DropKnifePreview(Knife);
    OpenDrawingSession(State, Kind);
    Knife.DrawPendingCondition = State.Drawing.ActiveCondition;
}

void OpenKnifeSession(SolidArcWorkspaceState& State)
{
    if (State.Bevel.ActiveCondition) { Announce(State, "Knife: finish the bevel first"); return; }
    if (!State.Document.Summary.PresentCondition) { Announce(State, "Knife needs a body to cut: make one first (Shift+C box, E extrude …)"); return; }
    SolidArcKnifeSession& Knife = State.Knife;
    const int  Commits = Knife.Commits;
    const int  Insert  = Knife.Insert;
    const bool Through = Knife.ThroughCondition;
    SolidArcInsertLibrary Library   = std::move(Knife.Library);
    const bool            Assembled = Knife.LibraryCondition;
    Knife                  = {};
    Knife.Commits          = Commits;
    Knife.Insert           = Insert;
    Knife.ThroughCondition = Through;
    Knife.Library          = std::move(Library);
    Knife.LibraryCondition = Assembled;
    Knife.ActiveCondition  = true;
    State.Sweep.ActiveCondition     = false;
    State.Sweep.Preview.reset();
    State.Trim.ActiveCondition      = false;
    State.Fill.ActiveCondition      = false;
    State.Constrain.ActiveCondition = false;
    State.Transform.ActiveCondition = false;
    State.Boolean.ActiveCondition   = false;
    State.Drawing.ActiveCondition   = false;
    std::string Sketch = FocusedSketchCondition(State) ? State.FocusedStep : std::string{};
    if (Sketch.empty())
        for (const std::string& Label : SelectedSketchLabels(State.Document)) { Sketch = Label; break; }
    if (State.Document.Selection.Bodies.size() >= 2) { ChooseKnifeMode(State, SolidArcKnifeMode::Object); return; }
    if (!Sketch.empty())
    {
        Knife.Sketch = Sketch;
        ChooseKnifeMode(State, SolidArcKnifeMode::Sketch);
        return;
    }
    DrawKnifeShape(State, SolidArcDrawingKind::Circle);
    Announce(State, "Knife · draw a circle on a face (card: Rectangle / Polygon / Slot / Line) · then move sideways for the depth · Tab: Insert library · Esc cancels");
}

void CloseKnifeSession(SolidArcWorkspaceState& State, const std::string& Message)
{
    SolidArcKnifeSession& Knife = State.Knife;
    if (Knife.DrawPendingCondition) State.Drawing.ActiveCondition = false;
    Knife.ActiveCondition      = false;
    Knife.DrawPendingCondition = false;
    DropKnifePreview(Knife);
    if (!Message.empty()) Announce(State, Message);
}

// FinishDrawingSession / CancelDrawingSession: the shape drawn for the knife (empty Sketch: refused or aborted).
void AdoptKnifeSketch(SolidArcWorkspaceState& State, const std::string& Sketch, const std::string& Face)
{
    SolidArcKnifeSession& Knife = State.Knife;
    if (!Knife.ActiveCondition || !Knife.DrawPendingCondition) return;
    Knife.DrawPendingCondition = false;
    if (Sketch.empty()) { CloseKnifeSession(State, "Knife cancelled (document untouched)"); return; }
    Knife.Sketch = Sketch;
    Knife.Face   = Face;
    Knife.Drift  = Knife.Depth;
    DropKnifePreview(Knife);
    Announce(State, "Knife · " + CaptionOf(State.Document, Sketch) + " into " + SolidArcBodyCaption(State.Document, KnifeSketchTarget(State, Sketch)) +
                        " · move sideways for the depth (Shift fine, Ctrl free) or type it · T through · Q add · W cut · E intersect · R separate · click / right-click / Enter applies");
}

void ApplyKnifeSession(SolidArcWorkspaceState& State)
{
    SolidArcKnifeSession& Knife = State.Knife;
    if (Knife.DrawPendingCondition) { Announce(State, "Knife: finish the shape first"); return; }
    const SolidArcCommandOutcome Outcome = RunKnife(State, State.Document);
    if (Outcome.AcceptedCondition)
    {
        ++Knife.Commits;
        CloseKnifeSession(State, {});
        State.FocusedStep = {};
        State.PickMode    = SolidArcPickMode::Solid;
    }
    else Knife.Refusal = Legible(Outcome.Refusal);
    Conclude(State, Outcome);
}

// Insert hover: the face under the pointer gives the spot (the hit carried onto its plane) and the body it cuts.
void HoverKnifeSpot(SolidArcWorkspaceState& State, const OrbitCameraFrame& Frame, ImVec2 Local)
{
    SolidArcKnifeSession& Knife = State.Knife;
    Knife.HoverCondition = false;
    const OrbitRay                    Ray  = CastOrbitRay(Frame, Local.x, Local.y);
    const std::optional<SolidArcPick> Pick = PickSolidArcScene(State.Scene, Ray, 0.0, false, ActiveSectionClip(State));
    const std::string                 Body = PickedSceneBody(State.Scene, Pick);
    std::string      Name;
    OrthonormalFrame Plane;
    if (Body.empty() || !ResolveHoveredFacePlane(State, Frame, Local, Name, Plane)) return;
    const Vector3 Hit = Vector3{ Ray.Origin[0], Ray.Origin[1], Ray.Origin[2] } + Vector3{ Ray.Direction[0], Ray.Direction[1], Ray.Direction[2] } * Pick->Distance;
    Plane.Origin = Hit - Plane.ZAxis * Dot(Hit - Plane.Origin, Plane.ZAxis);
    Knife.HoverCondition = true;
    Knife.Spot           = Plane;
    if (Body != Knife.SpotBody && !Knife.SizedCondition)
    {
        Vector3 Low, High;
        if (MeasureSceneBody(State.Scene, Body, Low, High))
            Knife.Size = std::max(0.01, std::round(0.25 * std::max({ High.X - Low.X, High.Y - Low.Y, High.Z - Low.Z }) / 0.01) * 0.01);
    }
    Knife.SpotBody = Body;
}

void DriveKnifeSession(SolidArcWorkspaceState& State, const OrbitCameraFrame& Frame, ImVec2 Local, bool HoveredCondition)
{
    SolidArcKnifeSession& Knife = State.Knife;
    ImGuiIO& Io = ImGui::GetIO();
    const bool Moving = Io.MouseDelta.x != 0.0f || Io.MouseDelta.y != 0.0f;
    if (Moving) Knife.QuietSince = KnifeClock();
    double Typed = 0.0;
    const bool TypedCondition = ParseTypedAmount(Knife.Typed, Typed) && Typed > 0.0;
    const double Drift = HoveredCondition && !ImGui::IsAnyMouseDown() ? Io.MouseDelta.x * Frame.WorldPerPixel * (Io.KeyShift ? 0.1 : 1.0) * 0.5 : 0.0;
    const auto Snapped = [&](double Value) { return Io.KeyCtrl ? Value : std::round(Value / SweepSnap) * SweepSnap; };
    if (Knife.Mode == SolidArcKnifeMode::Sketch)
    {
        if (TypedCondition) Knife.Depth = Typed;
        else
        {
            Knife.Drift += Drift;
            Knife.Depth  = std::max(0.001, Snapped(Knife.Drift));
        }
    }
    else if (Knife.Mode == SolidArcKnifeMode::Insert)
    {
        if (!Knife.PlacedCondition)
        {
            if (HoveredCondition) HoverKnifeSpot(State, Frame, Local);
            if (Moving && Knife.Preview) DropKnifePreview(Knife);                            // The ghost leads; the cut follows at rest
            if (TypedCondition) { Knife.Size = Typed; Knife.SizedCondition = true; }
        }
        else if (TypedCondition) Knife.Size = Typed;
        else
        {
            Knife.Drift += Drift;
            Knife.Size   = std::max(0.01, Snapped(Knife.Drift));
        }
    }
    const SolidArcSelection& Selection = State.Document.Selection;
    std::string Picked;
    for (const std::string& Key : Selection.Bodies) Picked += Key + ",";
    const std::string Key = Compose("%d|%d|%s|%.17g|%d|%d|%d|%d|%.17g,%.17g,%.17g|%.17g,%.17g,%.17g|%.17g|%.17g|%d|%s|%s|%llu", int(Knife.Mode), int(Knife.Combination), Knife.Sketch.c_str(),
                                    Knife.Depth, Knife.ThroughCondition ? 1 : 0, Knife.Insert, Knife.HoverCondition ? 1 : 0, Knife.PlacedCondition ? 1 : 0, Knife.Spot.Origin.X, Knife.Spot.Origin.Y,
                                    Knife.Spot.Origin.Z, Knife.Spot.ZAxis.X, Knife.Spot.ZAxis.Y, Knife.Spot.ZAxis.Z, Knife.Size, Knife.Turn, Knife.KeepCondition ? 1 : 0, Knife.SpotBody.c_str(),
                                    Picked.c_str(), static_cast<unsigned long long>(State.Document.Revision));
    if (Key == Knife.StagedKey || Knife.DrawPendingCondition) return;
    const double Quiet = KnifeClock() - Knife.QuietSince;
    if (Knife.Mode == SolidArcKnifeMode::Insert && !Knife.PlacedCondition && Quiet < 0.3) return;   // Hover: stage at rest only
    if (Knife.StageMilliseconds > 120.0 && Quiet < 0.25) return;                                  // Slow stage: wait for a pause
    Knife.StagedKey = Key;
    Knife.Refusal.clear();
    if (Knife.Mode == SolidArcKnifeMode::Insert && !Knife.PlacedCondition && !Knife.HoverCondition) { Knife.Preview.reset(); return; }
    SolidArcPhaseClock Clock(State.Profile, "viewport · knife stage");
    const double Start = KnifeClock();
    Knife.Target  = Knife.Mode == SolidArcKnifeMode::Sketch ? KnifeSketchTarget(State, Knife.Sketch)
                  : Knife.Mode == SolidArcKnifeMode::Insert ? Knife.SpotBody : (Selection.Bodies.empty() ? std::string{} : Selection.Bodies.front());
    Knife.Preview = std::make_unique<SolidArcDocument>(CloneSolidArcDocument(State.Document));
    const SolidArcCommandOutcome Outcome = RunKnife(State, *Knife.Preview);
    if (!Outcome.AcceptedCondition) { Knife.Refusal = Legible(Outcome.Refusal); Knife.Preview.reset(); }
    else Knife.PreviewScene = BuildSolidArcScene(*Knife.Preview);
    Knife.StageMilliseconds = (KnifeClock() - Start) * 1.0e3;
}

// Left-click: Insert places (then applies), Object picks bodies, Sketch applies.
void ClickKnifeSession(SolidArcWorkspaceState& State, ImVec2 Local)
{
    SolidArcKnifeSession& Knife = State.Knife;
    if (Knife.Mode == SolidArcKnifeMode::Object) { PickAtPointer(State, Local, PickIntentOf(ImGui::GetIO())); return; }
    if (Knife.Mode == SolidArcKnifeMode::Insert && !Knife.PlacedCondition)
    {
        if (!Knife.HoverCondition) { Announce(State, "Knife: point at a body face to place the insert"); return; }
        Knife.PlacedCondition = true;
        Knife.Drift           = Knife.Size;
        Announce(State, Compose("Knife · %s placed on %s · move sideways to size it (Shift fine, Ctrl free) or type a width · [ / ] turn 15° · click / right-click / Enter applies",
                                Knife.Library.Inserts[Knife.Insert].Name.c_str(), SolidArcBodyCaption(State.Document, Knife.SpotBody).c_str()));
        return;
    }
    ApplyKnifeSession(State);
}

// Insert ghost: the cutter's edges at the spot (amber; cyan once placed).
void DrawKnifeGhost(SolidArcWorkspaceState& State, ImDrawList* Canvas, const OrbitCameraFrame& Frame)
{
    const SolidArcKnifeSession& Knife = State.Knife;
    if (!Knife.ActiveCondition || Knife.Mode != SolidArcKnifeMode::Insert || !(Knife.HoverCondition || Knife.PlacedCondition)) return;
    if (Knife.Insert < 0 || Knife.Insert >= int(Knife.Library.Inserts.size())) return;
    const AffineMap Map    = PlaceSolidArcInsert(Knife.Spot, Knife.Size, Knife.Turn, Knife.Combination == FeatureCombination::Union);
    const ImU32     Colour = Knife.PlacedCondition ? IM_COL32(0x4d, 0xd2, 0xff, 0xff) : IM_COL32(0xff, 0xc8, 0x3d, 0xff);
    for (const std::array<Vector3, 2>& Segment : Knife.Library.Inserts[Knife.Insert].Segments)
    {
        ImVec2 Start, End;
        const Vector3 From = Map.ColumnX * Segment[0].X + Map.ColumnY * Segment[0].Y + Map.ColumnZ * Segment[0].Z + Map.Shift;
        const Vector3 To   = Map.ColumnX * Segment[1].X + Map.ColumnY * Segment[1].Y + Map.ColumnZ * Segment[1].Z + Map.Shift;
        if (ProjectWorldPoint(State, Frame, From, Start) && ProjectWorldPoint(State, Frame, To, End)) Canvas->AddLine(Start, End, Colour, 1.5f);
    }
    ImVec2 Centre;
    if (ProjectWorldPoint(State, Frame, Knife.Spot.Origin, Centre)) Canvas->AddCircle(Centre, 4.0f, Colour, 16, 1.5f);
}

[[nodiscard]] std::string KnifeReadout(const SolidArcWorkspaceState& State)
{
    const SolidArcKnifeSession& Knife = State.Knife;
    const std::string Typed = Knife.Typed.empty() ? std::string{} : " · typed " + Knife.Typed;
    if (Knife.Mode == SolidArcKnifeMode::Sketch)
    {
        if (Knife.Sketch.empty()) return "Knife · draw a closed shape";
        return Compose("Knife %s → %s · %s · %s%s", CaptionOf(State.Document, Knife.Sketch).c_str(), Knife.Target.empty() ? "?" : SolidArcBodyCaption(State.Document, Knife.Target).c_str(),
                       KnifeCombinationLabel(Knife.Combination), Knife.ThroughCondition && Knife.Combination != FeatureCombination::Union ? "through" : ("depth " + FormatLength(Knife.Depth) + " m").c_str(),
                       Typed.c_str());
    }
    if (Knife.Mode == SolidArcKnifeMode::Insert)
    {
        if (Knife.Library.Inserts.empty()) return "Knife · the insert library is empty";
        return Compose("Knife %s · %s · width %s m · %.0f°%s%s", Knife.Library.Inserts[Knife.Insert].Name.c_str(), KnifeCombinationLabel(Knife.Combination), FormatLength(Knife.Size).c_str(),
                       Knife.Turn * 180.0 / std::numbers::pi, Knife.PlacedCondition ? " · placed" : Knife.HoverCondition ? (" · on " + SolidArcBodyCaption(State.Document, Knife.SpotBody)).c_str() : " · point at a face",
                       Typed.c_str());
    }
    const std::vector<std::string>& Picked = State.Document.Selection.Bodies;
    return Compose("Knife %s · %s · %d cutter(s)%s", Picked.empty() ? "?" : SolidArcBodyCaption(State.Document, Picked.front()).c_str(), KnifeCombinationLabel(Knife.Combination),
                   std::max(0, int(Picked.size()) - 1), Knife.KeepCondition ? " · kept" : "");
}

void DrawKnifeCard(SolidArcWorkspaceState& State)
{
    SolidArcKnifeSession& Knife = State.Knife;
    BeginCard("Knife", KnifeModeLabel(Knife.Mode));
    static const struct { const char* Label; SolidArcKnifeMode Mode; } Modes[] = {
        { "Sketch###KnifeModeSketch", SolidArcKnifeMode::Sketch }, { "Insert###KnifeModeInsert", SolidArcKnifeMode::Insert }, { "Object###KnifeModeObject", SolidArcKnifeMode::Object } };
    for (int Index = 0; Index < 3; ++Index)
    {
        if (Index > 0) ImGui::SameLine();
        if (ToggleButton(State, Modes[Index].Label, Knife.Mode == Modes[Index].Mode, RowSpan(3)) && Knife.Mode != Modes[Index].Mode) ChooseKnifeMode(State, Modes[Index].Mode);
    }
    static const struct { const char* Label; FeatureCombination Combination; } Choices[] = {
        { "Cut###KnifeCut", FeatureCombination::Difference }, { "Add###KnifeAdd", FeatureCombination::Union },
        { "Intersect###KnifeIntersect", FeatureCombination::Intersection }, { "Separate###KnifeSeparate", FeatureCombination::Separate } };
    for (int Index = 0; Index < 4; ++Index)
    {
        if (Index > 0) ImGui::SameLine();
        if (ToggleButton(State, Choices[Index].Label, Knife.Combination == Choices[Index].Combination, RowSpan(4))) ChooseKnifeCombination(State, Choices[Index].Combination);
    }
    if (Knife.Mode == SolidArcKnifeMode::Sketch)
    {
        if (Knife.Sketch.empty() || Knife.DrawPendingCondition)
        {
            ImGui::TextDisabled("%s", Knife.DrawPendingCondition ? (std::string("Drawing a ") + SolidArcDrawingLabel(State.Drawing.Kind) + " for the knife").c_str() : "Draw a shape for the knife");
            static const struct { const char* Label; SolidArcDrawingKind Kind; } Shapes[] = {
                { "Circle###KnifeDrawCircle", SolidArcDrawingKind::Circle }, { "Rectangle###KnifeDrawRectangle", SolidArcDrawingKind::Rectangle },
                { "Polygon###KnifeDrawPolygon", SolidArcDrawingKind::Polygon }, { "Slot###KnifeDrawSlot", SolidArcDrawingKind::Slot }, { "Line###KnifeDrawLine", SolidArcDrawingKind::Line } };
            for (int Index = 0; Index < 5; ++Index)
            {
                if (Index > 0 && Index != 3) ImGui::SameLine();
                if (ToggleButton(State, Shapes[Index].Label, Knife.DrawPendingCondition && State.Drawing.Kind == Shapes[Index].Kind, RowSpan(3))) DrawKnifeShape(State, Shapes[Index].Kind);
            }
        }
        else
        {
            ImGui::Text("Cutter  %s", CaptionOf(State.Document, Knife.Sketch).c_str());
            ImGui::Text("Target  %s", Knife.Target.empty() ? "?" : SolidArcBodyCaption(State.Document, Knife.Target).c_str());
            ImGui::Text("Depth   %s m%s", FormatLength(Knife.Depth).c_str(), Knife.Typed.empty() ? "" : (" (typed " + Knife.Typed + ")").c_str());
            if (ToggleButton(State, "Through###KnifeThrough", Knife.ThroughCondition, RowSpan(2))) { Knife.ThroughCondition = !Knife.ThroughCondition; Announce(State, Knife.ThroughCondition ? "Knife: through" : "Knife: to depth"); }
            ImGui::SameLine();
            if (PressButton(State, "New shape###KnifeNewShape", RowSpan(2))) DrawKnifeShape(State, SolidArcDrawingKind::Circle);
        }
    }
    else if (Knife.Mode == SolidArcKnifeMode::Insert)
    {
        for (int Index = 0; Index < int(Knife.Library.Inserts.size()); ++Index)
        {
            if (Index % 2 == 1) ImGui::SameLine();
            const std::string Label = Knife.Library.Inserts[Index].Name + "###KnifeInsert" + std::to_string(Index);
            if (ToggleButton(State, Label.c_str(), Knife.Insert == Index, RowSpan(2)))
            {
                Knife.Insert = Index;
                DropKnifePreview(Knife);
                Announce(State, "Knife insert: " + Knife.Library.Inserts[Index].Name + (Knife.Library.Inserts[Index].Path.empty() ? std::string(" (built in)") : " (" + Knife.Library.Inserts[Index].Path + ")"));
            }
        }
        ImGui::Text("Width  %s m · turn %.0f°%s", FormatLength(Knife.Size).c_str(), Knife.Turn * 180.0 / std::numbers::pi, Knife.PlacedCondition ? " · placed" : "");
        for (const std::string& Note : Knife.Library.Notes) ImGui::TextDisabled("%s", Note.c_str());
        const bool OneBody = State.Document.Selection.Bodies.size() == 1;
        if (PressButton(State, "Save picked body###KnifeSaveInsert", RowSpan(2), OneBody))
        {
            const std::string Key = State.Document.Selection.Bodies.front();
            std::string Path;
            const std::string Refusal = SaveSolidArcInsert(State.Document, Key, State.Options.InsertFolder, SolidArcBodyCaption(State.Document, Key), Path);
            if (!Refusal.empty()) Announce(State, "Refused: " + Legible(Refusal));
            else
            {
                Knife.LibraryCondition = false;
                EnsureKnifeLibrary(State);
                Announce(State, "Insert saved: " + Path + Compose(" · library %d insert(s)", int(Knife.Library.Inserts.size())));
            }
        }
        ImGui::SameLine();
        if (PressButton(State, "Reload###KnifeReload", RowSpan(2))) { Knife.LibraryCondition = false; EnsureKnifeLibrary(State); DropKnifePreview(Knife); }
    }
    else
    {
        const std::vector<std::string>& Picked = State.Document.Selection.Bodies;
        ImGui::Text("Target  %s", Picked.empty() ? "?" : SolidArcBodyCaption(State.Document, Picked.front()).c_str());
        for (size_t Index = 1; Index < Picked.size(); ++Index) ImGui::TextDisabled("Cutter %d  %s", int(Index), SolidArcBodyCaption(State.Document, Picked[Index]).c_str());
        if (ToggleButton(State, "Keep cutters###KnifeKeep", Knife.KeepCondition, RowSpan(1))) { Knife.KeepCondition = !Knife.KeepCondition; Announce(State, Knife.KeepCondition ? "Knife: cutters kept" : "Knife: cutters consumed"); }
    }
    if (!Knife.Refusal.empty())
    {
        ImGui::PushStyleColor(ImGuiCol_Text, Tint(RefusalColour));
        ImGui::TextWrapped("%s", Knife.Refusal.c_str());
        ImGui::PopStyleColor();
    }
    ImGui::PushStyleColor(ImGuiCol_Text, Tint(MutedColour));
    ImGui::TextWrapped("Tab mode · Q add · W cut · E intersect · R separate · T through · [ / ] turn · K keep · type a value · click / right-click / Enter applies · Esc cancels");
    ImGui::PopStyleColor();
    if (PressButton(State, "Apply###KnifeApply", RowSpan())) ApplyKnifeSession(State);
    ImGui::SameLine();
    if (PressButton(State, "Cancel###KnifeCancel", RowSpan())) CloseKnifeSession(State, "Knife cancelled");
    EndCard();
}

void DrawBooleanCard(SolidArcWorkspaceState& State)
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
        ImGui::Text("Target  %s", SolidArcBodyCaption(State.Document, Picked.front()).c_str());
        for (size_t Index = 1; Index < Picked.size(); ++Index)
            ImGui::TextDisabled("Tool %d  %s", int(Index), SolidArcBodyCaption(State.Document, Picked[Index]).c_str());
    }
    ImGui::PushStyleColor(ImGuiCol_Text, Tint(MutedColour));
    ImGui::TextWrapped("Q union · W difference · E intersect · Enter applies · Esc cancels. The tools are consumed; the target keeps its key, matcap and history.");
    ImGui::PopStyleColor();
    if (PressButton(State, "Apply###BooleanApply", RowSpan())) ApplyBooleanSession(State);
    ImGui::SameLine();
    if (PressButton(State, "Cancel###BooleanCancel", RowSpan())) { State.Boolean.ActiveCondition = false; Announce(State, "Boolean cancelled"); }
    EndCard();
}

void ChooseProfileCombination(SolidArcWorkspaceState& State, FeatureCombination Combination)
{
    State.Bevel.Combination = Combination;
    Announce(State, std::string("Extrude Boolean: ") + (Combination == FeatureCombination::New ? "New body" : FeatureCombinationLabel(Combination)));
}

// Polyline of every sketch step on its plane (solved positions), the focused one highlighted, used ones dimmed.
void DrawSketchOverlays(SolidArcWorkspaceState& State, ImDrawList* Canvas, const OrbitCameraFrame& Frame)
{
    const SolidArcDocument& Document = DisplayedSolidArcDocument(State);                     // live while a drag stages a preview
    for (const FeatureStep& Step : Document.Tree.Steps)
    {
        if (Step.Kind != FeatureStepKind::Sketch || SolidArcCurveHiddenCondition(Document, Step.Label)) continue;   // Outliner eye
        const auto Solved = Document.Regeneration.Sketches.find(Step.Label);
        const SketchModel& Sketch = Solved != Document.Regeneration.Sketches.end() ? Solved->second : Step.Sketch;
        const bool  Locked   = SolidArcLockedCondition(Document, Step.Label);
        const bool  Focused  = Step.Label == State.FocusedStep
                            || std::find(Document.Selection.Curves.begin(), Document.Selection.Curves.end(), Step.Label) != Document.Selection.Curves.end();   // picked (loft order, gizmo)
        const bool  Consumed = SolidArcSketchConsumedCondition(Document, Step.Label);
        const ImU32 Colour   = Focused ? IM_COL32(0xff, 0xd2, 0x4d, 0xff) : Consumed || Locked ? IM_COL32(0x7f, 0x9c, 0xbf, 0x90) : IM_COL32(0x9f, 0xd8, 0xff, 0xff);
        OrthonormalFrame Plane = Step.Plane;
        Plane.Origin = Plane.Origin + Plane.ZAxis * EvaluateFeatureValue(Document.Tree, Step.Elevation);
        if (!Consumed || Focused) DrawSketchTint(State, Canvas, Frame, Step, Sketch, Plane, Focused);   // a solid covers a consumed one
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
        if (State.PickMode != SolidArcPickMode::ControlPoint || Locked) continue;              // A locked sketch offers no points
        // 📝 Mode 1: Bezier / NURBS control polygons (their points are poles off the curve), then every defining point.
        for (const SketchCurve& Curve : Sketch.Curves)
        {
            if (Curve.Kind != SketchCurveKind::Spline || (Curve.Form != SketchSplineForm::Bezier && Curve.Form != SketchSplineForm::Nurbs)) continue;
            std::vector<ImVec2> Hull;
            for (int Point : Curve.Points)
            {
                ImVec2 Pixel;
                if (Point >= 0 && Point < int(Sketch.Points.size()) && ProjectWorldPoint(State, Frame, PlacePlanePoint(Plane, Sketch.Points[Point].Position), Pixel)) Hull.push_back(Pixel);
            }
            if (Hull.size() >= 2) Canvas->AddPolyline(Hull.data(), int(Hull.size()), IM_COL32(0xc8, 0xc8, 0xc8, 0xa0), 1.0f);
        }
        const SolidArcSketchPointSession& Points = State.SketchPoints;
        for (int Point = 0; Point < int(Sketch.Points.size()); ++Point)
        {
            ImVec2 Pixel;
            if (!ProjectWorldPoint(State, Frame, PlacePlanePoint(Plane, Sketch.Points[Point].Position), Pixel)) continue;
            const bool  Held   = Points.DragCondition && Points.DragSketch == Step.Label && Points.DragPoint == Point;
            const bool  Picked = Held || (Points.Sketch == Step.Label && std::find(Points.Points.begin(), Points.Points.end(), Point) != Points.Points.end());
            const bool  Hover  = !Points.DragCondition && Points.HoverSketch == Step.Label && Points.HoverPoint == Point;
            const float Radius = Hover || Held ? 5.5f : 4.5f;                                  // [px] - Round point handle
            Canvas->AddCircleFilled(Pixel, Radius, Picked ? IM_COL32(0xff, 0xd2, 0x4d, 0xff) : IM_COL32(0x12, 0x16, 0x1c, 0xff), 20);
            Canvas->AddCircle(Pixel, Radius, Hover || Held ? IM_COL32(0xff, 0xff, 0xff, 0xff) : Colour, 20, 1.5f);
        }
    }
}

// Nearest shown sketch point within PoleReach of Pointer (mode 1); false when none.
bool FindSketchPointNear(const SolidArcWorkspaceState& State, const OrbitCameraFrame& Frame, ImVec2 Pointer, std::string& Label, int& Point)
{
    float Best = 8.0f;
    Point = -1;
    for (const FeatureStep& Step : State.Document.Tree.Steps)
    {
        if (Step.Kind != FeatureStepKind::Sketch || !CurveReachableCondition(State.Document, Step.Label)) continue;
        const SketchModel&     Sketch = SolvedSketchOf(State.Document, Step);
        const OrthonormalFrame Plane  = ElevatedSketchPlane(State.Document, Step);
        for (int Index = 0; Index < int(Sketch.Points.size()); ++Index)
        {
            ImVec2 Pixel;
            if (!ProjectWorldPoint(State, Frame, PlacePlanePoint(Plane, Sketch.Points[Index].Position), Pixel)) continue;
            const float Reach = std::hypot(Pointer.x - Pixel.x, Pointer.y - Pixel.y);
            if (Reach <= Best) { Best = Reach; Label = Step.Label; Point = Index; }
        }
    }
    return Point >= 0;
}

// Script evidence: each sketch point is a widget in mode 1 (SketchPointWidget).
void RecordSketchPointWidgets(SolidArcWorkspaceState& State, const OrbitCameraFrame& Frame)
{
    if (State.PickMode != SolidArcPickMode::ControlPoint) return;
    for (const FeatureStep& Step : State.Document.Tree.Steps)
    {
        if (Step.Kind != FeatureStepKind::Sketch || !CurveReachableCondition(State.Document, Step.Label)) continue;
        const SketchModel&     Sketch = SolvedSketchOf(State.Document, Step);
        const OrthonormalFrame Plane  = ElevatedSketchPlane(State.Document, Step);
        for (int Index = 0; Index < int(Sketch.Points.size()); ++Index)
        {
            ImVec2 Pixel;
            if (ProjectWorldPoint(State, Frame, PlacePlanePoint(Plane, Sketch.Points[Index].Position), Pixel))
                State.Widgets.Record(SketchPointWidget(Step.Label, Index), ImVec2(Pixel.x - 2.0f, Pixel.y - 2.0f), ImVec2(Pixel.x + 2.0f, Pixel.y + 2.0f));
        }
    }
}

SolidArcPickIntent PickIntentOf(const ImGuiIO& Io);

// A click on a sketch point: replace / add / remove in the picked set (one sketch at a time).
void PickSketchPoint(SolidArcWorkspaceState& State, const std::string& Label, int Point, SolidArcPickIntent Intent)
{
    SolidArcSketchPointSession& Points = State.SketchPoints;
    const auto Found = std::find(Points.Points.begin(), Points.Points.end(), Point);
    const bool Member = Points.Sketch == Label && Found != Points.Points.end();
    if (Intent == SolidArcPickIntent::Remove) { if (Member) Points.Points.erase(Found); }
    else if (Intent == SolidArcPickIntent::Add && Points.Sketch == Label) { if (!Member) Points.Points.push_back(Point); }
    else
    {
        Points.Sketch = Label;
        Points.Points = { Point };
    }
    if (Points.Points.empty()) Points.Sketch.clear();
    const FeatureStep* Step = FindSketchStepByLabel(State.Document, Label);
    Announce(State, Compose("%s · point %d · %d picked · the gizmo moves them in the sketch plane (drag a point to move it directly)",
                            Step ? CaptionOf(State.Document, Label).c_str() : Label.c_str(), Point, int(Points.Points.size())));
}

// Mode-1 box: the sketch points inside it (of the first sketch that has any) join / replace / leave the picked set.
// False when the box holds no sketch point (the ordinary box selection runs instead).
bool PickSketchPointsInBox(SolidArcWorkspaceState& State, const OrbitCameraFrame& Frame, ImVec2 Start, ImVec2 End, SolidArcPickIntent Intent)
{
    const float Left = std::min(Start.x, End.x), Right = std::max(Start.x, End.x), Top = std::min(Start.y, End.y), Bottom = std::max(Start.y, End.y);
    SolidArcSketchPointSession& Points = State.SketchPoints;
    for (const FeatureStep& Step : State.Document.Tree.Steps)
    {
        if (Step.Kind != FeatureStepKind::Sketch || !CurveReachableCondition(State.Document, Step.Label)) continue;
        if (Intent != SolidArcPickIntent::Replace && !Points.Sketch.empty() && Step.Label != Points.Sketch) continue;
        const SketchModel&     Sketch = SolvedSketchOf(State.Document, Step);
        const OrthonormalFrame Plane  = ElevatedSketchPlane(State.Document, Step);
        std::vector<int>       Inside;
        for (int Index = 0; Index < int(Sketch.Points.size()); ++Index)
        {
            ImVec2 Pixel;
            if (ProjectWorldPoint(State, Frame, PlacePlanePoint(Plane, Sketch.Points[Index].Position), Pixel) && Pixel.x >= Left && Pixel.x <= Right && Pixel.y >= Top && Pixel.y <= Bottom)
                Inside.push_back(Index);
        }
        if (Inside.empty()) continue;
        if (Intent == SolidArcPickIntent::Replace || Points.Sketch.empty()) { Points.Sketch = Step.Label; Points.Points.clear(); }
        for (int Index : Inside)
        {
            const auto Found = std::find(Points.Points.begin(), Points.Points.end(), Index);
            if (Intent == SolidArcPickIntent::Remove) { if (Found != Points.Points.end()) Points.Points.erase(Found); }
            else if (Found == Points.Points.end()) Points.Points.push_back(Index);
        }
        if (Points.Points.empty()) Points.Sketch.clear();
        Announce(State, Compose("%s · %d sketch points picked · the gizmo moves, turns and scales them in the sketch plane", CaptionOf(State.Document, Step.Label).c_str(), int(Points.Points.size())));
        return true;
    }
    return false;
}

void CancelSketchPointDrag(SolidArcWorkspaceState& State)
{
    SolidArcSketchPointSession& Points = State.SketchPoints;
    Points.DragCondition = false;
    Points.Preview.reset();
    Points.StagedKey.clear();
    Points.Refusal.clear();
    State.Picker.Current = {};
}

// 💡 Direct drag of a held sketch point: the pointer meets the sketch plane, 0.1 grid and object snaps (Ctrl free); each
//    distinct target stages a preview (the sketch re-solves, extrudes / lofts regenerate live); the release commits one
//    DragSolidArcSketchPoint, a release without movement picks the point.
void DriveSketchPointDrag(SolidArcWorkspaceState& State, const OrbitCameraFrame& Frame, ImVec2 Local)
{
    SolidArcSketchPointSession& Points = State.SketchPoints;
    const ImGuiIO&              Io     = ImGui::GetIO();
    const FeatureStep*          Step   = FindSketchStepByLabel(State.Document, Points.DragSketch);
    if (!Step) { CancelSketchPointDrag(State); return; }
    const OrthonormalFrame Plane = ElevatedSketchPlane(State.Document, *Step);
    const bool             Moved = Io.MouseDragMaxDistanceSqr[ImGuiMouseButton_Left] > ClickTolerance * ClickTolerance;
    if (Moved)
    {
        if (const std::optional<Vector3> Hit = IntersectGizmoPlane(CastOrbitRay(Frame, Local.x, Local.y), Plane.Origin, Plane.ZAxis))
        {
            Vector2 Flat{ Dot(*Hit - Plane.Origin, Plane.XAxis), Dot(*Hit - Plane.Origin, Plane.YAxis) };
            // Grid steps; Ctrl: the important points of other figures (free where none is in reach).
            if (const std::optional<Vector2> Snap = SnapOntoPlane(State, Frame, Local, Plane, std::nullopt, {}, { Points.DragSketch })) Flat = *Snap;
            else if (!Io.KeyCtrl) Flat = { std::round(Flat.X / 0.1) * 0.1, std::round(Flat.Y / 0.1) * 0.1 };
            Points.Target = Flat;
        }
    }
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) || !ImGui::IsMouseDown(ImGuiMouseButton_Left))
    {
        const std::string Label   = Points.DragSketch;
        const int         Point   = Points.DragPoint;
        const std::string Refusal = Points.Refusal;
        const bool        Shifted = Distance(Points.Target, Points.Origin) > 1.0e-12;
        CancelSketchPointDrag(State);
        if (!Moved) { PickSketchPoint(State, Label, Point, PickIntentOf(Io)); return; }
        if (!Shifted) { Announce(State, "Point drag: nothing changed"); return; }
        if (!Refusal.empty()) { Announce(State, "Refused: " + Refusal); return; }
        const SolidArcCommandOutcome Outcome = DragSolidArcSketchPoint(State.Document, Label, Point, Points.Target);
        Conclude(State, Outcome);
        if (Outcome.AcceptedCondition)
        {
            ++Points.Commits;
            Points.Sketch = Label;
            Points.Points = { Point };
            Announce(State, Compose("%s · %.0f ms", Outcome.Label.c_str(), Outcome.Milliseconds));
        }
        return;
    }
    if (!Moved) return;
    const std::string Key = Compose("%.12g %.12g", Points.Target.X, Points.Target.Y);
    if (Key == Points.StagedKey) return;
    Points.StagedKey = Key;
    Points.Refusal.clear();
    if (Distance(Points.Target, Points.Origin) <= 1.0e-12) { Points.Preview.reset(); return; }
    Points.Preview = std::make_unique<SolidArcDocument>(CloneSolidArcDocument(State.Document));
    const SolidArcCommandOutcome Outcome = DragSolidArcSketchPoint(*Points.Preview, Points.DragSketch, Points.DragPoint, Points.Target);
    if (!Outcome.AcceptedCondition) { Points.Refusal = Legible(Outcome.Refusal); Points.Preview.reset(); return; }
    Points.PreviewScene = BuildSolidArcScene(*Points.Preview);
}

// The drawing in progress: the shape through the pointer, its clicks, a spline's control polygon.
void DrawDrawingPreview(SolidArcWorkspaceState& State, ImDrawList* Canvas, const OrbitCameraFrame& Frame)
{
    SolidArcDrawingSession& Drawing = State.Drawing;
    if (Drawing.SolidCondition) { DrawSolidPreview(State, Canvas, Frame); return; }
    const OrthonormalFrame  Plane   = DrawingPlane(Drawing);
    auto Pixel = [&](Vector2 Point, ImVec2& Out) { return ProjectWorldPoint(State, Frame, PlacePlanePoint(Plane, Point), Out); };
    SketchModel Sketch;
    const std::string Refusal = ComposeSolidArcSketch(ComposeDrawingRequest(Drawing, true, false), Sketch);
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
    if (Drawing.Kind == SolidArcDrawingKind::Curve && Drawing.Form != SketchSplineForm::Hermite && Drawing.Form != SketchSplineForm::CubicSpline && Marks.size() >= 2)
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
    DrawSketchDimensions(State, Frame, Canvas);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    VEHICLE CURVES
//------------------------------------------------------------------------------------------------------------------------

constexpr int   CurveSamples = 96;                                                           // [-]  - Polyline points per fitted curve
constexpr float CurveReach   = 6.0f;                                                         // [px] - Click distance that picks a curve
constexpr float PoleReach    = 8.0f;                                                         // [px] - Press distance that grabs a pole

// Fitted SpaceCurve of a step (nullptr when the step is refused or not a curve).
const CurveFitOutcome* FindFittedCurve(const SolidArcDocument& Document, const std::string& Label)
{
    const auto Found = Document.Regeneration.Curves.find(Label);
    return Found != Document.Regeneration.Curves.end() && Found->second.Curve.Definition ? &Found->second : nullptr;
}

std::string CaptionOf(const SolidArcDocument& Document, const std::string& Label)
{
    const auto Found = Document.Captions.find(Label);
    return Found != Document.Captions.end() ? Found->second : Label;
}

// 1-based loft order of a selected curve, 0 when unselected.
int CurveOrder(const SolidArcWorkspaceState& State, const std::string& Label)
{
    const std::vector<std::string>& Curves = State.Document.Selection.Curves;
    const auto Found = std::find(Curves.begin(), Curves.end(), Label);
    return Found == Curves.end() ? 0 : int(Found - Curves.begin()) + 1;
}

// Poles show on the selected curves, and on every curve in mode 1.
bool PolesShownCondition(const SolidArcWorkspaceState& State, const std::string& Label)
{
    return State.PickMode == SolidArcPickMode::ControlPoint || CurveOrder(State, Label) > 0;
}

bool CurveConsumedCondition(const SolidArcDocument& Document, const std::string& Label)
{
    for (const FeatureStep& Step : Document.Tree.Steps)
        if ((Step.Kind == FeatureStepKind::Loft || Step.Kind == FeatureStepKind::Network) && std::find(Step.Sections.begin(), Step.Sections.end(), Label) != Step.Sections.end()) return true;
    return false;
}

// The definition as displayed: during a pole drag the grabbed pole follows the pointer.
NurbsCurveDefinition DisplayedDefinition(const SolidArcWorkspaceState& State, const std::string& Label, const NurbsCurveDefinition& Definition)
{
    NurbsCurveDefinition Shown = Definition;
    const SolidArcPoleDrag& Drag = State.PoleDrag;
    if (Drag.ActiveCondition && Drag.Label == Label && Drag.Pole >= 0 && Drag.Pole < int(Shown.Poles.size())) Shown.Poles[Drag.Pole] = Drag.Position;
    return Shown;
}

std::vector<ImVec2> ProjectFittedCurve(const SolidArcWorkspaceState& State, const OrbitCameraFrame& Frame, const NurbsCurveDefinition& Definition)
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
std::string FindCurveNear(const SolidArcWorkspaceState& State, const OrbitCameraFrame& Frame, ImVec2 Pointer)
{
    std::string Nearest;
    float       Best = CurveReach;
    for (const FeatureStep& Step : State.Document.Tree.Steps)
    {
        if (!CurveReachableCondition(State.Document, Step.Label)) continue;                  // Hidden or locked
        if (Step.Kind == FeatureStepKind::Sketch && !SolidArcSketchConsumedCondition(State.Document, Step.Label))
        {
            // Free sketches (loft sections, paths) pick like curves; an extruded sketch stays with its body.
            const OrthonormalFrame Plane  = ElevatedSketchPlane(State.Document, Step);
            const auto             Solved = State.Document.Regeneration.Sketches.find(Step.Label);
            const SketchModel&     Sketch = Solved != State.Document.Regeneration.Sketches.end() ? Solved->second : Step.Sketch;
            for (const SketchCurve& Curve : Sketch.Curves)
            {
                if (Curve.Construction) continue;
                std::vector<ImVec2> Pixels;
                for (const Vector2& Sample : SampleSketchCurve(Sketch, Curve)) { ImVec2 Pixel; if (ProjectWorldPoint(State, Frame, PlacePlanePoint(Plane, Sample), Pixel)) Pixels.push_back(Pixel); }
                const float Distance = DistanceToPolyline(Pixels, Pointer);
                if (Distance <= Best) { Best = Distance; Nearest = Step.Label; }
            }
            continue;
        }
        const CurveFitOutcome* Fit = Step.Kind == FeatureStepKind::SpaceCurve ? FindFittedCurve(State.Document, Step.Label) : nullptr;
        if (!Fit) continue;
        const float Distance = DistanceToPolyline(ProjectFittedCurve(State, Frame, *Fit->Curve.Definition), Pointer);
        if (Distance <= Best) { Best = Distance; Nearest = Step.Label; }
    }
    return Nearest;
}

// Nearest shown pole within PoleReach of the pointer.
bool FindPoleNear(const SolidArcWorkspaceState& State, const OrbitCameraFrame& Frame, ImVec2 Pointer, std::string& Label, int& Pole)
{
    float Best = PoleReach;
    Pole = -1;
    for (const auto& [Candidate, Fit] : State.Document.Regeneration.Curves)
    {
        if (!Fit.Curve.Definition || !PolesShownCondition(State, Candidate) || !CurveReachableCondition(State.Document, Candidate)) continue;
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
void DrawCurvatureComb(const SolidArcWorkspaceState& State, ImDrawList* Canvas, const OrbitCameraFrame& Frame, const NurbsCurveDefinition& Definition)
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
void DrawCurveOverlays(SolidArcWorkspaceState& State, ImDrawList* Canvas, const OrbitCameraFrame& Frame)
{
    const SolidArcDocument& Document = State.Document;
    for (const FeatureStep& Step : Document.Tree.Steps)
    {
        const CurveFitOutcome* Fit = Step.Kind == FeatureStepKind::SpaceCurve ? FindFittedCurve(Document, Step.Label) : nullptr;
        if (!Fit || SolidArcCurveHiddenCondition(Document, Step.Label)) continue;           // Outliner eye
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
            const float Radius  = Grabbed || Hover ? 5.5f : 4.0f;                              // [px] - Round pole handle
            Canvas->AddCircleFilled(Pixel, Radius, Grabbed ? ActiveColour : Hover ? IM_COL32_WHITE : IM_COL32(0xff, 0xd2, 0x4d, 0xff), 20);
            Canvas->AddCircle(Pixel, Radius, IM_COL32(0x10, 0x10, 0x10, 0xff), 20, 1.0f);
        }
    }
}

// X-ray (Plasticity viewport:toggle-x-ray, Alt+Z): ghost faces at α, hidden edges drawn faded and picked / boxed through.
void ToggleXRay(SolidArcWorkspaceState& State)
{
    State.Style.XRayEnabled = !State.Style.XRayEnabled;
    Announce(State, State.Style.XRayEnabled ? Compose("X-ray on · faces %d %% · hidden edges pickable", int(std::lround(State.Style.XRayOpacity * 100.0f)))
                                            : std::string("X-ray off"));
}

void DrawStatusPill(ImDrawList* Canvas, ImVec2& Cursor, const std::string& Text, ImU32 Accent);

// 📝 Snap glyphs (Fusion / Plasticity style): square endpoint, triangle midpoint, ringed centre, cross intersection,
//    ring + bar tangent, ⊥ perpendicular, dot on curve, dot + normal tick on face, dot on a lock line; the caption pill
//    sits at the pointer (+14, +14). The active lock draws as a dashed line through the last point.
void DrawSnapOverlay(SolidArcWorkspaceState& State, ImDrawList* Canvas, const OrbitCameraFrame& Frame)
{
    constexpr ImU32 SnapColour    = IM_COL32(0xe6, 0xff, 0x4d, 0xff);                         // #e6ff4d - Snapped point
    constexpr ImU32 OutlineColour = IM_COL32(0x10, 0x12, 0x16, 0xd0);                         // Dark halo behind the glyph
    constexpr ImU32 LockColours[] = { 0, AxisColours[0], AxisColours[1], AxisColours[2], IM_COL32(0xff, 0xc8, 0x3d, 0xff), IM_COL32(0x4d, 0xd2, 0xff, 0xff), IM_COL32(0xe0, 0x8c, 0xff, 0xff) };
    const SolidArcPointPicker& Picker = State.Picker;
    const bool Drawing  = State.Drawing.ActiveCondition;
    const bool Moving   = State.Transform.ActiveCondition && State.Transform.Verb == SolidArcTransformVerb::Move;
    const bool Dragging = (State.Constrain.ActiveCondition && State.Constrain.DragCondition) || State.PoleDrag.ActiveCondition;
    if (!Drawing && !Moving && !Dragging) return;
    if (Drawing && Picker.Lock != SolidArcSnapLockKind::None)
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
    const SolidArcSnap& Snap = Picker.Current;
    if (Snap.Kind == SolidArcSnapKind::None || Snap.Kind == SolidArcSnapKind::Grid) return;
    ImVec2 At;
    if (!ProjectWorldPoint(State, Frame, Snap.Position, At)) return;
    auto Glyph = [&](ImU32 Colour, float Width)
    {
        const float R = 6.0f;
        switch (Snap.Kind)
        {
        case SolidArcSnapKind::Endpoint: Canvas->AddRect(ImVec2(At.x - R, At.y - R), ImVec2(At.x + R, At.y + R), Colour, 0.0f, Width); break;
        case SolidArcSnapKind::Midpoint: Canvas->AddTriangle(ImVec2(At.x, At.y - R - 1.0f), ImVec2(At.x - R, At.y + R - 1.0f), ImVec2(At.x + R, At.y + R - 1.0f), Colour, Width); break;
        case SolidArcSnapKind::Centre:
            Canvas->AddCircle(At, R, Colour, 20, Width);
            Canvas->AddLine(ImVec2(At.x - R - 3.0f, At.y), ImVec2(At.x + R + 3.0f, At.y), Colour, Width * 0.5f);
            Canvas->AddLine(ImVec2(At.x, At.y - R - 3.0f), ImVec2(At.x, At.y + R + 3.0f), Colour, Width * 0.5f);
            break;
        case SolidArcSnapKind::Intersection:
            Canvas->AddLine(ImVec2(At.x - R, At.y - R), ImVec2(At.x + R, At.y + R), Colour, Width);
            Canvas->AddLine(ImVec2(At.x - R, At.y + R), ImVec2(At.x + R, At.y - R), Colour, Width);
            break;
        case SolidArcSnapKind::Tangent:
            Canvas->AddCircle(ImVec2(At.x, At.y + R * 0.5f), R * 0.8f, Colour, 20, Width);
            Canvas->AddLine(ImVec2(At.x - R - 3.0f, At.y - R * 0.3f), ImVec2(At.x + R + 3.0f, At.y - R * 0.3f), Colour, Width);
            break;
        case SolidArcSnapKind::Perpendicular:
            Canvas->AddLine(ImVec2(At.x - R, At.y + R), ImVec2(At.x + R, At.y + R), Colour, Width);
            Canvas->AddLine(ImVec2(At.x, At.y + R), ImVec2(At.x, At.y - R), Colour, Width);
            Canvas->AddRect(ImVec2(At.x, At.y + R * 0.2f), ImVec2(At.x + R * 0.8f, At.y + R), Colour, 0.0f, Width * 0.5f);
            break;
        case SolidArcSnapKind::Face:
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
    Glyph(Snap.Kind == SolidArcSnapKind::Axis ? LockColours[int(Picker.Lock)] : SnapColour, 2.0f);
    const ImVec2 Pointer = ImGui::GetIO().MousePos;
    std::string Text = Snap.Kind == SolidArcSnapKind::Axis ? Snap.Source : std::string(SolidArcSnapLabel(Snap.Kind)) + " · " + Snap.Source;
    if (Text.size() > 72) Text = Text.substr(0, 69) + "...";
    // Flip left / up near the viewport's right and bottom edges (the hint line lives along the bottom)
    const float  Width  = ImGui::CalcTextSize(Text.c_str()).x + 26.0f;
    const float  Right  = State.ViewportRect[0] + State.ViewportRect[2];
    const float  Bottom = State.ViewportRect[1] + State.ViewportRect[3];
    ImVec2 Caption(Pointer.x + 14.0f, Pointer.y + 14.0f);
    if (Caption.x + Width > Right - 8.0f) Caption.x = Pointer.x - 14.0f - Width;
    if (Caption.y + 26.0f > Bottom - 40.0f) Caption.y = Pointer.y - 14.0f - 26.0f;
    DrawStatusPill(Canvas, Caption, Text, Snap.Kind == SolidArcSnapKind::Axis ? LockColours[int(Picker.Lock)] : SnapColour);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    JOINT (ALT+J)
//------------------------------------------------------------------------------------------------------------------------

// Fingerprint of a request: the staged preview is rebuilt only when it changes.
std::string JointRequestKey(const SolidArcJointRequest& Request)
{
    return Compose("%d|%s|%d|%s|%d|%.17g|%.17g|%.17g|%.17g|%.17g|%.17g|%d", int(Request.MovingKind), Request.MovingName.c_str(), int(Request.FixedKind), Request.FixedName.c_str(),
                   int(Request.Kind), Request.Angle, Request.Offset, Request.Slide, Request.SecondSlide, Request.Pitch, Request.Roll, Request.Flipped ? 1 : 0);
}

void StageJointPreview(SolidArcWorkspaceState& State)
{
    SolidArcJointSession& Joint = State.Joint;
    if (!Joint.ActiveCondition || Joint.Phase < 2) return;
    const std::string Key = JointRequestKey(Joint.Request);
    if (Key == Joint.StagedKey) return;
    Joint.StagedKey = Key;
    Joint.Refusal.clear();
    Joint.Preview = std::make_unique<SolidArcDocument>(CloneSolidArcDocument(State.Document));
    const SolidArcCommandOutcome Outcome = JointSolidArcBodies(*Joint.Preview, Joint.Request);
    if (!Outcome.AcceptedCondition) { Joint.Refusal = Legible(Outcome.Refusal); Joint.Preview.reset(); return; }
    Joint.PreviewScene = BuildSolidArcScene(*Joint.Preview);
}

void CloseJointSession(SolidArcWorkspaceState& State, const char* Message)
{
    State.Joint.ActiveCondition = false;
    State.Joint.Preview.reset();
    State.Joint.StagedKey.clear();
    State.Joint.HoverName.clear();
    if (Message) Announce(State, Message);
}

void AnnounceJointAdjust(SolidArcWorkspaceState& State)
{
    const SolidArcJointSession& Joint = State.Joint;
    Announce(State, Compose("Joint: %s onto %s · R V S C P L B kind · drag the values · F flip · Enter / right-click applies", SolidArcBodyCaption(State.Document, Joint.Moving.Body).c_str(),
                            SolidArcBodyCaption(State.Document, Joint.Fixed.Body).c_str()));
}

// 💡 Alt+J (Fusion Joint; Plasticity has none). Two entities picked beforehand on two bodies skip the picking: the earlier
//    pick (pick order of one kind; for a face and an edge, the one picked last is fixed) is on the moving body.
void OpenJointSession(SolidArcWorkspaceState& State)
{
    if (State.Document.Summary.Bodies.size() < 2)
    {
        Announce(State, "Joint (Alt+J) needs two bodies: construct one with R (New body) in Shift+C / V / X / Z");
        return;
    }
    SolidArcJointSession& Joint   = State.Joint;
    const int             Commits = Joint.Commits;
    const JointKind       Kind    = Joint.Request.Kind;                                     // The last kind is kept, as the bevel keeps its shape
    CloseJointSession(State, nullptr);
    Joint                 = SolidArcJointSession{};
    Joint.Commits         = Commits;
    Joint.Request.Kind    = Kind;
    Joint.ActiveCondition = true;
    const SolidArcSelection& Selection = State.Document.Selection;
    std::vector<std::pair<SolidArcSelectionKind, std::string>> Picks;
    if (Selection.Faces.size() + Selection.Edges.size() == 2)
    {
        constexpr SolidArcSelectionKind Edge = SolidArcSelectionKind::Edge, Face = SolidArcSelectionKind::Face;
        if (Selection.Edges.size() == 2) Picks = { { Edge, Selection.Edges[0] }, { Edge, Selection.Edges[1] } };
        else if (Selection.Faces.size() == 2) Picks = { { Face, Selection.Faces[0] }, { Face, Selection.Faces[1] } };
        else if (Selection.LastKind == Edge) Picks = { { Face, Selection.Faces[0] }, { Edge, Selection.Edges[0] } };
        else Picks = { { Edge, Selection.Edges[0] }, { Face, Selection.Faces[0] } };
    }
    if (Picks.size() == 2)
    {
        const SolidArcJointOrigin First  = ResolveSolidArcJointOrigin(State.Document, Picks[0].first, Picks[0].second);
        const SolidArcJointOrigin Second = ResolveSolidArcJointOrigin(State.Document, Picks[1].first, Picks[1].second);
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

void ChooseJointKind(SolidArcWorkspaceState& State, JointKind Kind)
{
    State.Joint.Request.Kind = Kind;
    StageJointPreview(State);
    Announce(State, Compose("Joint: %s · %d degree(s) of freedom", JointKindLabel(Kind), JointFreedom(Kind)));
}

void FlipJoint(SolidArcWorkspaceState& State)
{
    State.Joint.Request.Flipped = !State.Joint.Request.Flipped;
    StageJointPreview(State);
    Announce(State, State.Joint.Request.Flipped ? "Joint: flipped (origins aligned)" : "Joint: origins facing");
}

// The edge (within 8 px, else the face) under the pointer and the joint origin it offers — the hover glyph.
void HoverJointOrigin(SolidArcWorkspaceState& State, const OrbitCameraFrame& Frame, ImVec2 Local, bool Hovered)
{
    SolidArcJointSession& Joint = State.Joint;
    if (Joint.Phase >= 2) return;
    uint32_t Identity = Hovered ? State.HoverIdentity : 0;
    if (Hovered && !(Identity & IdentityEdgeBit))
    {
        const OrbitRay                 Ray    = CastOrbitRay(Frame, Local.x, Local.y);
        const std::optional<SolidArcPick> Nearby = PickSolidArcScene(State.Scene, Ray, 8.0 * Frame.WorldPerPixel, State.Style.XRayEnabled, ActiveSectionClip(State));
        if (Nearby && (Nearby->Identity & IdentityEdgeBit)) Identity = Nearby->Identity;
    }
    if (Identity == IdentitySectionCap) Identity = 0;
    const std::string           Name = Identity ? NameSolidArcIdentity(State.Scene, Identity) : std::string();
    const SolidArcSelectionKind Kind = Identity ? SolidArcIdentityKind(Identity) : SolidArcSelectionKind::Edge;
    if (Name == Joint.HoverName && Kind == Joint.HoverKind) return;
    Joint.HoverName = Name;
    Joint.HoverKind = Kind;
    Joint.Hover     = Name.empty() ? SolidArcJointOrigin{} : ResolveSolidArcJointOrigin(State.Document, Kind, Name);
}

void ClickJointSession(SolidArcWorkspaceState& State)
{
    SolidArcJointSession& Joint = State.Joint;
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
                                SolidArcBodyCaption(State.Document, Joint.Moving.Body).c_str()));
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

void ApplyJointSession(SolidArcWorkspaceState& State)
{
    SolidArcJointSession& Joint = State.Joint;
    if (Joint.Phase < 2) { Announce(State, "Joint: pick both joint origins first"); return; }
    const SolidArcJointRequest Request = Joint.Request;
    CloseJointSession(State, nullptr);
    const SolidArcCommandOutcome Outcome = JointSolidArcBodies(State.Document, Request);
    if (Outcome.AcceptedCondition)
    {
        ++Joint.Commits;
        State.FocusedStep = State.Document.Tree.Steps.back().Label;                          // The step card drags its values
    }
    Conclude(State, Outcome);
}

void DrawJointCard(SolidArcWorkspaceState& State)
{
    SolidArcJointSession& Joint = State.Joint;
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
    const auto Origin = [&](const char* Caption, const SolidArcJointOrigin& Picked)
    {
        ImGui::Text("%s  %s on %s", Caption, Picked.Description.c_str(), SolidArcBodyCaption(State.Document, Picked.Body).c_str());
    };
    if (Joint.Phase == 0) ImGui::TextColored(Tint(ActiveColour), "1  Pick the origin on the moving body");
    if (Joint.Phase >= 1) Origin("Moving", Joint.Moving);
    if (Joint.Phase == 1) ImGui::TextColored(Tint(ActiveColour), "2  Pick the origin on the fixed body");
    if (Joint.Phase == 2)
    {
        Origin("Fixed ", Joint.Fixed);
        SolidArcJointRequest& Request = Joint.Request;
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
    if (PressButton(State, "Apply###JointApply", RowSpan(), Joint.Phase == 2 && Joint.Refusal.empty())) ApplyJointSession(State);
    ImGui::SameLine();
    if (PressButton(State, "Cancel###JointCancel", RowSpan())) CloseJointSession(State, "Joint cancelled");
    EndCard();
}

// 📝 Joint origin glyph: a ring in the origin's plane (while picking), z (blue, long) and x (red) arrows, a caption pill.
void DrawJointGlyph(SolidArcWorkspaceState& State, ImDrawList* Canvas, const OrbitCameraFrame& Frame, const OrthonormalFrame& Origin, const std::string& Caption, ImU32 Accent,
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
void DrawJointOverlay(SolidArcWorkspaceState& State, ImDrawList* Canvas, const OrbitCameraFrame& Frame)
{
    constexpr ImU32 HoverColour  = IM_COL32(0x4d, 0xd2, 0xff, 0xff);                         // Cyan: what a click takes
    constexpr ImU32 PickedColour = IM_COL32(0xff, 0xc8, 0x3d, 0xff);                         // Amber: chosen origins
    const SolidArcJointSession& Joint = State.Joint;
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
    const SolidArcDocument& Shown = State.ParameterDrag.Preview ? *State.ParameterDrag.Preview : State.Document;
    for (const RegeneratedJoint& Placed : Shown.Regeneration.Joints)
        if (Placed.Label == State.FocusedStep) DrawJointGlyph(State, Canvas, Frame, Placed.FixedOrigin, std::string(JointKindLabel(Placed.Kind)) + " joint", PickedColour, false);
}

// Measure (I): recompute only when the selection or the document revision moved (a NURBS body takes ~1 s).
void RefreshMeasure(SolidArcWorkspaceState& State)
{
    SolidArcMeasureSession& Measure = State.Measure;
    if (!Measure.ActiveCondition) return;
    if (Measure.Refreshes > 0 && Measure.Measurement.Revision == State.Document.Revision
        && Measure.Measurement.SelectionKey == SolidArcSelectionKey(State.Document.Selection)) return;
    Measure.Measurement = MeasureSolidArcSelection(State.Document);
    Measure.Refreshes  += 1;
}

// Measure (I; Fusion Inspect → Measure, no Plasticity default): card + overlay of the selection's exact figures.
void ToggleMeasure(SolidArcWorkspaceState& State)
{
    SolidArcMeasureSession& Measure = State.Measure;
    Measure.ActiveCondition = !Measure.ActiveCondition;
    Measure.Refreshes       = 0;
    if (!Measure.ActiveCondition) { Announce(State, "Measure off"); return; }
    RefreshMeasure(State);
    const SolidArcMeasurement& Measurement = Measure.Measurement;
    if (!Measurement.Refusal.empty()) Announce(State, Measurement.Refusal);
    else Announce(State, "Measure · " + Measurement.Subject + (Measurement.Rows.empty() ? std::string{} : " · " + Measurement.Rows.front().Caption + " " + Measurement.Rows.front().Value));
}

// 📝 Inspector card: subject, then caption / value rows (values wrap under long captions), timing and the selection rule.
void DrawMeasureCard(SolidArcWorkspaceState& State)
{
    if (!State.Measure.ActiveCondition) return;
    const SolidArcMeasurement& Measurement = State.Measure.Measurement;
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
        for (const SolidArcMeasureRow& Row : Measurement.Rows)
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
void DrawMeasureOverlay(SolidArcWorkspaceState& State, ImDrawList* Canvas, const OrbitCameraFrame& Frame)
{
    if (!State.Measure.ActiveCondition) return;
    const SolidArcMeasurement& Measurement = State.Measure.Measurement;
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
std::string DescribeSectionRequest(const SolidArcSectionRequest& Request)
{
    static const char* Axes[] = { "x", "y", "z" };
    const std::string Flip = Request.Flipped ? " (flipped)" : "";
    if (Request.Axis == SolidArcSectionAxis::Face) return "Section on " + Request.Face + " at " + FormatLength(Request.Offset) + Flip;
    return std::string("Section ") + Axes[int(Request.Axis)] + " = " + FormatLength(Request.Offset) + Flip;
}

// Default offsets: world planes through the centre of the shown scene's bounds; a face plane halfway through the
// bounds behind the face (offset along the outward normal, negative = into the body).
double DefaultSectionOffset(const SolidArcWorkspaceState& State, const SolidArcSectionRequest& Request)
{
    const SolidArcScene& Scene = State.Scene;
    if (Request.Axis != SolidArcSectionAxis::Face)
    {
        const int Axis = int(Request.Axis);
        return 0.5 * (Scene.BoundsLow[Axis] + Scene.BoundsHigh[Axis]);
    }
    SolidArcSectionRequest AtFace = Request;
    AtFace.Offset  = 0.0;
    AtFace.Flipped = false;
    const SolidArcSection Plane = ResolveSolidArcSectionPlane(State.Document, AtFace);
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

void StoreSectionOffsetText(SolidArcSectionSession& Session)
{
    std::snprintf(Session.OffsetText.data(), Session.OffsetText.size(), "%.6g", Session.Request.Offset);
}

// 💡 Every frame while Section is on: the plane goes to the viewport programs (clip + cap) and the CPU pick; the exact
//    kernel section is recomputed only when the request or the document revision moved (0.2 … 2 s for curved bodies).
void RefreshSection(SolidArcWorkspaceState& State)
{
    SolidArcSectionSession& Session = State.Section;
    State.Style.SectionEnabled = false;
    if (!Session.ActiveCondition) return;
    const SolidArcSection Plane = ResolveSolidArcSectionPlane(State.Document, Session.Request);
    if (!Plane.PlaneCondition)
    {
        if (Session.Section.RequestKey != Plane.RequestKey || Session.Section.Revision != Plane.Revision) Session.Section = Plane;
        return;
    }
    State.Style.SectionEnabled = true;
    State.Style.SectionPlane   = SectionEquation(Plane.Plane);
    Session.Clip = ComposeSolidArcSectionClip(DisplayedSolidArcScene(State), State.Document, State.Style.SectionPlane);
    if (Session.Refreshes > 0 && Session.Section.Revision == State.Document.Revision && Session.Section.RequestKey == Plane.RequestKey) return;
    Session.Section    = SectionSolidArcDocument(State.Document, Session.Request);
    Session.Refreshes += 1;
}

// The section clip for CPU picks while Section is on (null otherwise).
const SolidArcSectionClip* ActiveSectionClip(const SolidArcWorkspaceState& State)
{
    return State.Style.SectionEnabled ? &State.Section.Clip : nullptr;
}

void ChooseSectionAxis(SolidArcWorkspaceState& State, SolidArcSectionAxis Axis)
{
    SolidArcSectionSession& Session = State.Section;
    Session.Request.Axis = Axis;
    if (Axis == SolidArcSectionAxis::Face)
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
void ToggleSection(SolidArcWorkspaceState& State)
{
    SolidArcSectionSession& Session = State.Section;
    Session.ActiveCondition = !Session.ActiveCondition;
    Session.Refreshes       = 0;
    if (!Session.ActiveCondition)
    {
        State.Style.SectionEnabled = false;
        Announce(State, "Section off");
        return;
    }
    Session.Request.Flipped = false;
    ChooseSectionAxis(State, State.Document.Selection.Faces.empty() ? SolidArcSectionAxis::Y : SolidArcSectionAxis::Face);
    if (!Session.Section.Refusal.empty() && Session.Request.Axis == SolidArcSectionAxis::Face) ChooseSectionAxis(State, SolidArcSectionAxis::Y);
}

void AdjustSectionOffset(SolidArcWorkspaceState& State, double Delta)
{
    State.Section.Request.Offset += Delta;
    StoreSectionOffsetText(State.Section);
    RefreshSection(State);
    Announce(State, DescribeSectionRequest(State.Section.Request));
}

void FlipSection(SolidArcWorkspaceState& State)
{
    State.Section.Request.Flipped = !State.Section.Request.Flipped;
    RefreshSection(State);
    Announce(State, DescribeSectionRequest(State.Section.Request));
}

// 📝 Inspector card: plane family (X / Y / Z / Face), offset field (Enter applies; units as everywhere), − / + steps of
//    1/20 of the bounds diagonal, Flip, and the exact figures (u = plane x axis, v = plane y axis, about the centroid).
void DrawSectionCard(SolidArcWorkspaceState& State)
{
    SolidArcSectionSession& Session = State.Section;
    if (!Session.ActiveCondition) return;
    BeginCard("Section");
    const float Spacing = ImGui::GetStyle().ItemSpacing.x;
    const float Quarter = std::floor((ImGui::GetContentRegionAvail().x - 3.0f * Spacing) / 4.0f);
    static const char* AxisLabels[] = { "X###SectionAxisX", "Y###SectionAxisY", "Z###SectionAxisZ", "Face###SectionAxisFace" };
    for (int Axis = 0; Axis < 4; ++Axis)
    {
        if (Axis) ImGui::SameLine();
        if (ToggleButton(State, AxisLabels[Axis], int(Session.Request.Axis) == Axis, ImVec2(Quarter, 0))) ChooseSectionAxis(State, SolidArcSectionAxis(Axis));
    }
    const SolidArcScene& Scene = State.Scene;
    double Diagonal = 0.0;
    for (int Axis = 0; Axis < 3; ++Axis) Diagonal += (Scene.BoundsHigh[Axis] - Scene.BoundsLow[Axis]) * (Scene.BoundsHigh[Axis] - Scene.BoundsLow[Axis]);
    const double Step = std::max(1.0e-3, 0.05 * std::sqrt(Diagonal));
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 3.0f * (34.0f + Spacing) - 8.0f);
    if (ImGui::InputText("###SectionOffset", Session.OffsetText.data(), Session.OffsetText.size(), ImGuiInputTextFlags_EnterReturnsTrue))
    {
        double Value = 0.0;
        if (ParseSolidArcLength(Session.OffsetText.data(), Value)) { Session.Request.Offset = Value; RefreshSection(State); Announce(State, DescribeSectionRequest(Session.Request)); }
        else Announce(State, std::string("Section offset: cannot read \"") + Session.OffsetText.data() + "\"");
    }
    RegisterWidget(State, "###SectionOffset");
    ImGui::SameLine();
    if (PressButton(State, "-###SectionCloser", ImVec2(34, 0))) AdjustSectionOffset(State, -Step);
    ImGui::SameLine();
    if (PressButton(State, "+###SectionFarther", ImVec2(34, 0))) AdjustSectionOffset(State, Step);
    ImGui::SameLine();
    if (ToggleButton(State, "F###SectionFlip", Session.Request.Flipped, ImVec2(34, 0))) FlipSection(State);
    const SolidArcSection& Section = Session.Section;
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
void DrawSectionOutline(SolidArcWorkspaceState& State, ImDrawList* Canvas, const OrbitCameraFrame& Frame)
{
    const SolidArcSectionSession& Session = State.Section;
    if (!State.Style.SectionEnabled || !Session.Section.Refusal.empty() || Session.Section.RequestKey != SolidArcSectionKey(Session.Request)
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

// 💡 Drawing sheet (Alt+D): recomposed only when the document revision or the settings moved (14 ms … 1 s; a section
//    view costs the most).
void RefreshDrawingSheet(SolidArcWorkspaceState& State)
{
    SolidArcDrawingSheetSession& Session = State.DrawingSheet;
    if (!Session.ActiveCondition) return;
    const std::string Key = SolidArcDrawingSheetCacheKey(State.Document, State.Document.DrawingSheet);
    if (Session.Refreshes > 0 && Session.Sheet.Revision == State.Document.Revision && Session.Sheet.SettingsKey == Key) return;
    Session.Sheet      = ComposeSolidArcDrawingSheet(State.Document, State.Document.DrawingSheet);
    Session.Refreshes += 1;
}

std::string DescribeDrawingSheet(const SolidArcWorkspaceState& State)
{
    const SolidArcDrawingSheet& Sheet = State.DrawingSheet.Sheet;
    if (!Sheet.Refusal.empty()) return Sheet.Refusal;
    std::string Text = Compose("Drawing sheet %s · %s · ", GeometryProcessing::DrawingPaperLabel(State.Document.DrawingSheet.Paper), Sheet.Sheet.ScaleLabel.c_str());
    Text += Sheet.Sheet.Angle == GeometryProcessing::DrawingProjectionAngle::First ? "first angle" : "third angle";
    Text += Compose(" · %d views · %d dimensions", int(Sheet.Sheet.Views.size()), int(Sheet.Sheet.Dimensions.size()));
    return Text;
}

void StoreDrawingSheetTexts(SolidArcWorkspaceState& State)
{
    SolidArcDrawingSheetSession& Session = State.DrawingSheet;
    const SolidArcTitleFields&   Title   = State.Document.DrawingSheet.Title;
    std::snprintf(Session.TitleText.data(), Session.TitleText.size(), "%s", Title.Title.c_str());
    std::snprintf(Session.NumberText.data(), Session.NumberText.size(), "%s", Title.DrawingNumber.c_str());
    std::snprintf(Session.AuthorText.data(), Session.AuthorText.size(), "%s", Title.Author.c_str());
    std::snprintf(Session.MaterialText.data(), Session.MaterialText.size(), "%s", Title.Material.c_str());
}

void ToggleDrawingSheet(SolidArcWorkspaceState& State)
{
    SolidArcDrawingSheetSession& Session = State.DrawingSheet;
    Session.ActiveCondition = !Session.ActiveCondition;
    Session.Refreshes       = 0;
    if (!Session.ActiveCondition) { Announce(State, "Drawing sheet off"); return; }
    Session.Zoom = 1.0f;
    Session.Pan  = {};
    StoreDrawingSheetTexts(State);
    RefreshDrawingSheet(State);
    Announce(State, DescribeDrawingSheet(State));
}

// A settings change: saved with the document (dirty, autosaved), not an undo step; the sheet recomposes at once.
void ReviseDrawingSheet(SolidArcWorkspaceState& State, const std::function<void(SolidArcDrawingSheetSettings&)>& Change)
{
    Change(State.Document.DrawingSheet);
    State.DirtyCondition = true;
    RefreshDrawingSheet(State);
    Announce(State, DescribeDrawingSheet(State));
}

// − / + step through the ISO 5455 scales from the one in force (the automatic choice included); Auto returns to 0.
void StepDrawingSheetScale(SolidArcWorkspaceState& State, int Direction)
{
    const std::vector<double> Scales  = GeometryProcessing::EnumerateStandardScales();                 // Largest first
    const double              Current = State.Document.DrawingSheet.Scale > 0.0 ? State.Document.DrawingSheet.Scale : State.DrawingSheet.Sheet.Sheet.Scale;
    int Nearest = 0;
    for (int Index = 1; Index < int(Scales.size()); ++Index)
        if (std::fabs(std::log(Scales[Index] / std::max(Current, 1.0e-12))) < std::fabs(std::log(Scales[Nearest] / std::max(Current, 1.0e-12)))) Nearest = Index;
    const int Next = std::clamp(Nearest - Direction, 0, int(Scales.size()) - 1);
    ReviseDrawingSheet(State, [&](SolidArcDrawingSheetSettings& Settings) { Settings.Scale = Scales[Next]; });
}

// Section A-A follows the Section card's plane while Alt+S is on, else y through the scene centre (the Front direction).
void ToggleDrawingSheetSection(SolidArcWorkspaceState& State)
{
    SolidArcSectionRequest Plane = State.Section.ActiveCondition ? State.Section.Request : SolidArcSectionRequest{ SolidArcSectionAxis::Y, 0.0, false, {} };
    if (!State.Section.ActiveCondition) Plane.Offset = DefaultSectionOffset(State, Plane);
    ReviseDrawingSheet(State, [&](SolidArcDrawingSheetSettings& Settings)
    {
        Settings.SectionEnabled = !Settings.SectionEnabled;
        if (Settings.SectionEnabled) Settings.Section = Plane;
    });
}

void ExportDrawingSheet(SolidArcWorkspaceState& State, const char* Extension)
{
    SolidArcDrawingSheetSession& Session = State.DrawingSheet;
    RefreshDrawingSheet(State);
    Session.ExportPath    = SolidArcDrawingSheetExportPath(State.DocumentPath.empty() ? State.Options.DocumentPath : State.DocumentPath, Extension);  // as Ctrl+S
    Session.ExportRefusal = ExportSolidArcDrawingSheet(Session.Sheet, Session.ExportPath);
    Session.ExportBytes   = 0.0;
    std::error_code Failure;
    if (Session.ExportRefusal.empty()) Session.ExportBytes = double(std::filesystem::file_size(Session.ExportPath, Failure));
    if (Session.ExportRefusal.empty()) Session.Exports += 1;
    Announce(State, Session.ExportRefusal.empty() ? Compose("Exported %s (%.0f bytes)", Session.ExportPath.c_str(), Session.ExportBytes) : Session.ExportRefusal);
}

// 📝 Inspector card: paper A4 … A0, first / third angle, scale (− / Auto / +), views and line kinds, the title block
//    fields (Enter applies) and SVG / DXF export beside the document.
void DrawDrawingSheetCard(SolidArcWorkspaceState& State)
{
    SolidArcDrawingSheetSession& Session  = State.DrawingSheet;
    if (!Session.ActiveCondition) return;
    const SolidArcDrawingSheetSettings& Settings = State.Document.DrawingSheet;
    BeginCard("Drawing sheet");
    const float Spacing = ImGui::GetStyle().ItemSpacing.x;
    const float Fifth   = std::floor((ImGui::GetContentRegionAvail().x - 4.0f * Spacing) / 5.0f);
    const float Half    = std::floor((ImGui::GetContentRegionAvail().x - Spacing) / 2.0f);
    static const char* PaperLabels[] = { "A4###SheetPaperA4", "A3###SheetPaperA3", "A2###SheetPaperA2", "A1###SheetPaperA1", "A0###SheetPaperA0" };
    for (int Paper = 0; Paper < 5; ++Paper)
    {
        if (Paper) ImGui::SameLine();
        if (ToggleButton(State, PaperLabels[Paper], int(Settings.Paper) == Paper, ImVec2(Fifth, 0)))
            ReviseDrawingSheet(State, [&](SolidArcDrawingSheetSettings& Revised) { Revised.Paper = GeometryProcessing::DrawingPaperSize(Paper); });
    }
    if (ToggleButton(State, "First angle###SheetFirstAngle", Settings.Angle == GeometryProcessing::DrawingProjectionAngle::First, ImVec2(Half, 0)))
        ReviseDrawingSheet(State, [](SolidArcDrawingSheetSettings& Revised) { Revised.Angle = GeometryProcessing::DrawingProjectionAngle::First; });
    ImGui::SameLine();
    if (ToggleButton(State, "Third angle###SheetThirdAngle", Settings.Angle == GeometryProcessing::DrawingProjectionAngle::Third, ImVec2(Half, 0)))
        ReviseDrawingSheet(State, [](SolidArcDrawingSheetSettings& Revised) { Revised.Angle = GeometryProcessing::DrawingProjectionAngle::Third; });
    const std::string ScaleCaption = (Settings.Scale > 0.0 ? "Scale " : "Auto ") + Session.Sheet.Sheet.ScaleLabel + "###SheetScaleAuto";
    if (PressButton(State, "-###SheetScaleSmaller", ImVec2(34, 0))) StepDrawingSheetScale(State, -1);
    ImGui::SameLine();
    if (ToggleButton(State, ScaleCaption.c_str(), Settings.Scale == 0.0, ImVec2(ImGui::GetContentRegionAvail().x - 34.0f - Spacing, 0)))
        ReviseDrawingSheet(State, [](SolidArcDrawingSheetSettings& Revised) { Revised.Scale = 0.0; });
    ImGui::SameLine();
    if (PressButton(State, "+###SheetScaleLarger", ImVec2(34, 0))) StepDrawingSheetScale(State, 1);
    if (ToggleButton(State, "Isometric###SheetIsometric", Settings.IsometricEnabled, ImVec2(Half, 0)))
        ReviseDrawingSheet(State, [](SolidArcDrawingSheetSettings& Revised) { Revised.IsometricEnabled = !Revised.IsometricEnabled; });
    ImGui::SameLine();
    if (ToggleButton(State, "Section A-A###SheetSection", Settings.SectionEnabled, ImVec2(Half, 0))) ToggleDrawingSheetSection(State);
    if (ToggleButton(State, "Hidden lines###SheetHidden", Settings.HiddenLinesEnabled, ImVec2(Half, 0)))
        ReviseDrawingSheet(State, [](SolidArcDrawingSheetSettings& Revised) { Revised.HiddenLinesEnabled = !Revised.HiddenLinesEnabled; });
    ImGui::SameLine();
    if (ToggleButton(State, "Dimensions###SheetDimensions", Settings.DimensionsEnabled, ImVec2(Half, 0)))
        ReviseDrawingSheet(State, [](SolidArcDrawingSheetSettings& Revised) { Revised.DimensionsEnabled = !Revised.DimensionsEnabled; });

    const auto Field = [&](const char* Caption, const char* Identifier, std::array<char, 96>& Text, std::string SolidArcTitleFields::*Member)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, Tint(MutedColour));
        ImGui::TextUnformatted(Caption);
        ImGui::PopStyleColor();
        ImGui::SameLine(96.0f);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        const bool Entered = ImGui::InputText(Identifier, Text.data(), Text.size(), ImGuiInputTextFlags_EnterReturnsTrue);
        if (Entered || ImGui::IsItemDeactivatedAfterEdit())
        {
            const std::string Value = Text.data();
            if (State.Document.DrawingSheet.Title.*Member != Value) ReviseDrawingSheet(State, [&](SolidArcDrawingSheetSettings& Revised) { Revised.Title.*Member = Value; });
        }
        RegisterWidget(State, Identifier);
    };
    Field("Title", "###SheetTitle", Session.TitleText, &SolidArcTitleFields::Title);
    Field("Number", "###SheetNumber", Session.NumberText, &SolidArcTitleFields::DrawingNumber);
    Field("Drawn by", "###SheetAuthor", Session.AuthorText, &SolidArcTitleFields::Author);
    Field("Material", "###SheetMaterial", Session.MaterialText, &SolidArcTitleFields::Material);

    const SolidArcDrawingSheet& Sheet = Session.Sheet;
    if (!Sheet.Refusal.empty())
    {
        ImGui::PushStyleColor(ImGuiCol_Text, Tint(RefusalColour));
        ImGui::TextWrapped("%s", Sheet.Refusal.c_str());
        ImGui::PopStyleColor();
    }
    else
    {
        std::string Views;
        for (const GeometryProcessing::PlacedDrawingView& View : Sheet.Sheet.Views) Views += (Views.empty() ? "" : " · ") + View.Label;
        ImGui::TextWrapped("%s", Views.c_str());
        ImGui::TextDisabled("%d body · %d dimensions · %d lines · %.0f ms%s", Sheet.Bodies, int(Sheet.Sheet.Dimensions.size()), int(Sheet.Sheet.Lines.size()), Sheet.Milliseconds,
                            Sheet.Sheets ? Compose(" · %d sheet(s) skipped", Sheet.Sheets).c_str() : "");
    }
    if (PressButton(State, "Export SVG###SheetExportSvg", ImVec2(Half, 0))) ExportDrawingSheet(State, ".svg");
    ImGui::SameLine();
    if (PressButton(State, "Export DXF###SheetExportDxf", ImVec2(Half, 0))) ExportDrawingSheet(State, ".dxf");
    if (!Session.ExportPath.empty())
    {
        ImGui::PushStyleColor(ImGuiCol_Text, Tint(Session.ExportRefusal.empty() ? MutedColour : RefusalColour));
        ImGui::TextWrapped("%s", Session.ExportRefusal.empty() ? Compose("%s · %.0f bytes", std::filesystem::path(Session.ExportPath).filename().string().c_str(), Session.ExportBytes).c_str()
                                                                : Session.ExportRefusal.c_str());
        ImGui::PopStyleColor();
    }
    if (PressButton(State, "Fit###SheetFit", ImVec2(Half, 0))) { Session.Zoom = 1.0f; Session.Pan = {}; }
    ImGui::SameLine();
    if (PressButton(State, "Close (Alt+D)###SheetClose", ImVec2(Half, 0))) ToggleDrawingSheet(State);
    EndCard();
}

// Dashes along a pixel polyline: Pattern alternates drawn / blank lengths [px] (empty = continuous).
void DrawPatternedPolyline(ImDrawList* Canvas, const std::vector<ImVec2>& Points, const std::vector<float>& Pattern, ImU32 Colour, float Width)
{
    if (Points.size() < 2) return;
    if (Pattern.empty()) { Canvas->AddPolyline(Points.data(), int(Points.size()), Colour, Width); return; }
    size_t Phase     = 0;
    float  Remaining = Pattern[0];
    for (size_t Index = 1; Index < Points.size(); ++Index)
    {
        ImVec2       From   = Points[Index - 1];
        const ImVec2 To     = Points[Index];
        float        Length = std::hypot(To.x - From.x, To.y - From.y);
        if (Length <= 0.0f) continue;
        const ImVec2 Along((To.x - From.x) / Length, (To.y - From.y) / Length);
        while (Length > 0.0f)
        {
            const float  Piece = std::min(Remaining, Length);
            const ImVec2 End(From.x + Along.x * Piece, From.y + Along.y * Piece);
            if (Phase % 2 == 0) Canvas->AddLine(From, End, Colour, Width);
            From       = End;
            Length    -= Piece;
            Remaining -= Piece;
            if (Remaining <= 1.0e-4f) { Phase = (Phase + 1) % Pattern.size(); Remaining = Pattern[Phase]; }
        }
    }
}

// 💡 The paper in the viewport: fitted with a 24 px margin × Zoom, centre offset by Pan; layer widths and dash patterns
//    of ISO 128-2 scaled to the zoom (≥ 1 px); texts by cap height (≈ 0.7 em), rotated by turning their vertices about
//    the baseline anchor. ⌀ prints as Ø (the interface font has no U+2300; exports keep ⌀ / %%c).
void DrawDrawingSheetPaper(SolidArcWorkspaceState& State, ImDrawList* Canvas, ImVec2 Image, ImVec2 Extent, bool Hovered)
{
    SolidArcDrawingSheetSession& Session = State.DrawingSheet;
    RefreshDrawingSheet(State);
    ImGuiIO& Io = ImGui::GetIO();
    Canvas->AddRectFilled(Image, ImVec2(Image.x + Extent.x, Image.y + Extent.y), IM_COL32(0x3a, 0x3f, 0x4a, 0xff), PanelRounding, ImDrawFlags_RoundCornersBottom);
    const GeometryProcessing::DrawingSheet& Sheet = Session.Sheet.Sheet;
    const GeometryProcessing::Vector2 Paper = Session.Sheet.Refusal.empty() ? GeometryProcessing::Vector2{ Sheet.Width, Sheet.Height } : GeometryProcessing::DrawingPaperDimensions(State.Document.DrawingSheet.Paper);
    const float  Fit    = std::max(1.0e-3f, std::min((Extent.x - 48.0f) / float(Paper.X), (Extent.y - 48.0f) / float(Paper.Y)));
    const ImVec2 Centre(Image.x + 0.5f * Extent.x, Image.y + 0.5f * Extent.y);
    if (Hovered && Io.MouseWheel != 0.0f)
    {
        const float  Before = Fit * Session.Zoom;
        const ImVec2 Corner(Centre.x + Session.Pan[0] - 0.5f * float(Paper.X) * Before, Centre.y + Session.Pan[1] - 0.5f * float(Paper.Y) * Before);
        const ImVec2 Under((Io.MousePos.x - Corner.x) / Before, (Io.MousePos.y - Corner.y) / Before);    // [mm] - Paper point under the pointer
        Session.Zoom = std::clamp(Session.Zoom * std::pow(1.2f, Io.MouseWheel), 0.25f, 40.0f);
        const float After = Fit * Session.Zoom;
        Session.Pan = { Io.MousePos.x - Under.x * After - Centre.x + 0.5f * float(Paper.X) * After, Io.MousePos.y - Under.y * After - Centre.y + 0.5f * float(Paper.Y) * After };
    }
    if ((ImGui::IsItemActive() || Hovered) && (ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 0.0f) || ImGui::IsMouseDragging(ImGuiMouseButton_Right, ClickTolerance)))
    {
        Session.Pan[0] += Io.MouseDelta.x;
        Session.Pan[1] += Io.MouseDelta.y;
    }
    if (Hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) { Session.Zoom = 1.0f; Session.Pan = {}; }
    const float  Scale = Fit * Session.Zoom;                                                  // [px/mm]
    const ImVec2 Corner(Centre.x + Session.Pan[0] - 0.5f * float(Paper.X) * Scale, Centre.y + Session.Pan[1] - 0.5f * float(Paper.Y) * Scale);
    const auto   Map = [&](const GeometryProcessing::Vector2& Point) { return ImVec2(Corner.x + float(Point.X) * Scale, Corner.y + float(Paper.Y - Point.Y) * Scale); };

    Canvas->PushClipRect(Image, ImVec2(Image.x + Extent.x, Image.y + Extent.y), true);
    Canvas->AddRectFilled(ImVec2(Corner.x + 4, Corner.y + 4), ImVec2(Corner.x + float(Paper.X) * Scale + 4, Corner.y + float(Paper.Y) * Scale + 4), IM_COL32(0, 0, 0, 0x50));
    Canvas->AddRectFilled(Corner, ImVec2(Corner.x + float(Paper.X) * Scale, Corner.y + float(Paper.Y) * Scale), IM_COL32_WHITE);
    if (!Session.Sheet.Refusal.empty())
    {
        const ImVec2 Size = ImGui::CalcTextSize(Session.Sheet.Refusal.c_str());
        Canvas->AddText(ImVec2(Centre.x + Session.Pan[0] - 0.5f * Size.x, Centre.y + Session.Pan[1] - 0.5f * Size.y), IM_COL32(0xb0, 0x30, 0x20, 0xff), Session.Sheet.Refusal.c_str());
        Canvas->PopClipRect();
        return;
    }
    struct LayerStyle { float Width; std::vector<float> Dashes; };                            // [mm]
    static const LayerStyle Styles[] = { { 0.50f, {} }, { 0.25f, { 3.0f, 1.5f } }, { 0.18f, {} }, { 0.50f, {} }, { 0.18f, {} }, { 0.25f, {} },
                                         { 0.18f, { 8.0f, 1.5f, 1.0f, 1.5f } }, { 0.50f, { 8.0f, 1.5f, 1.0f, 1.5f } }, { 0.70f, {} }, { 0.35f, {} } };
    const ImU32 Ink = IM_COL32(0x10, 0x12, 0x16, 0xff);
    for (const GeometryProcessing::SheetPolyline& Line : Sheet.Lines)
    {
        const LayerStyle& Style = Styles[std::min<size_t>(size_t(Line.Layer), std::size(Styles) - 1)];
        std::vector<ImVec2> Pixels;
        Pixels.reserve(Line.Points.size());
        for (const GeometryProcessing::Vector2& Point : Line.Points) Pixels.push_back(Map(Point));
        std::vector<float> Dashes;
        for (const float Dash : Style.Dashes) Dashes.push_back(std::max(1.5f, Dash * Scale));
        DrawPatternedPolyline(Canvas, Pixels, Dashes, Ink, std::max(1.0f, Style.Width * Scale));
    }
    for (const GeometryProcessing::SheetSolid& Solid : Sheet.Solids) Canvas->AddTriangleFilled(Map(Solid.Corners[0]), Map(Solid.Corners[1]), Map(Solid.Corners[2]), Ink);
    ImFont* Font = ImGui::GetFont();
    for (const GeometryProcessing::SheetText& Text : Sheet.Texts)
    {
        const float Pixels = float(Text.Height) * Scale / 0.70f;                             // [px] - Em from the cap height
        if (Pixels < 3.0f) continue;
        std::string Content = Text.Content;
        for (size_t At = Content.find("⌀"); At != std::string::npos; At = Content.find("⌀", At)) Content.replace(At, std::strlen("⌀"), "Ø");
        const float  Width  = Font->CalcTextSizeA(Pixels, FLT_MAX, 0.0f, Content.c_str()).x;
        const float  Ascent = Font->GetFontBaked(Pixels)->Ascent;
        const ImVec2 Anchor = Map(Text.Position);
        const float  Shift  = Text.Anchor == GeometryProcessing::SheetTextAnchor::Middle ? -0.5f * Width : Text.Anchor == GeometryProcessing::SheetTextAnchor::End ? -Width : 0.0f;
        const int    First  = Canvas->VtxBuffer.Size;
        Canvas->AddText(Font, Pixels, ImVec2(Anchor.x + Shift, Anchor.y - Ascent), Ink, Content.c_str());
        if (Text.Angle == 0.0) continue;
        const float Cosine = float(std::cos(Text.Angle)), Sine = float(std::sin(Text.Angle));      // Paper 𝑦 up, screen 𝑦 down
        for (int Index = First; Index < Canvas->VtxBuffer.Size; ++Index)
        {
            ImVec2&     Position = Canvas->VtxBuffer[Index].pos;
            const float Across   = Position.x - Anchor.x, Down = Position.y - Anchor.y;
            Position = ImVec2(Anchor.x + Across * Cosine + Down * Sine, Anchor.y - Across * Sine + Down * Cosine);
        }
    }
    Canvas->PopClipRect();
}

// Zebra (V8): stripe count to the surface shader, and a finer tessellation so interpolated normals follow the surface.
void ToggleZebra(SolidArcWorkspaceState& State)
{
    State.ZebraEnabled       = !State.ZebraEnabled;
    State.Style.ZebraStripes = State.ZebraEnabled ? ZebraStripeCount : 0.0f;
    RefreshSolidArcScene(State, false);
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
void DrawDeviationOverlay(SolidArcWorkspaceState& State, ImDrawList* Canvas, const OrbitCameraFrame& Frame)
{
    if (State.Deviation.Revision != State.Document.Revision || (State.Deviation.Needles.empty() && !State.Document.Regeneration.Panels.empty()))
        State.Deviation = MeasureSolidArcDeviation(State.Document);
    const SolidArcDeviationMap& Map = State.Deviation;
    if (Map.Needles.empty()) return;
    const double Gain = Map.Worst > 0.0 ? 0.05 * State.Camera.SceneRadius / Map.Worst : 1.0;
    for (const SolidArcDeviationNeedle& Needle : Map.Needles)
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
void FrameSelection(SolidArcWorkspaceState& State);

void ImportCurves(SolidArcWorkspaceState& State, const std::string& Path)
{
    const VehicleCurveFile       File    = ReadVehicleCurves(Path);
    const SolidArcCommandOutcome Outcome = ImportSolidArcCurves(State.Document, File);
    Conclude(State, Outcome);
    if (!Outcome.AcceptedCondition) return;
    double Worst = 0.0;
    int    Poles = 0;
    for (const auto& [Label, Fit] : State.Document.Regeneration.Curves) { Worst = std::max(Worst, Fit.WorstDeviation); Poles += int(Fit.Curve.Definition->Poles.size()); }
    FrameSelection(State);
    Announce(State, Compose("Imported %d curve(s) from %s · %d poles · worst fit %.3f mm", int(File.Curves.size()), Path.c_str(), Poles, Worst * 1000.0));
}

// 💡 Import… with a .step / .stp file (or Open… with one, as Plasticity opens STEP into the scene): every solid becomes
//    an Import step and a body of its own (one undo entry); the new bodies are selected and framed.
void ImportStep(SolidArcWorkspaceState& State, const std::string& Path)
{
    SolidArcExchangeSession& Session = State.Exchange;
    const auto Start = std::chrono::steady_clock::now();
    const SolidArcStepReading    Reading = ReadSolidArcStepFile(Path);
    const size_t                 Before  = State.Document.Tree.Steps.size();
    const SolidArcCommandOutcome Outcome = ImportSolidArcStep(State.Document, Reading);
    Session.ImportPath         = Path;
    Session.ImportRefusal      = Outcome.AcceptedCondition ? std::string{} : Outcome.Refusal;
    Session.ImportSolids       = Outcome.AcceptedCondition ? int(Reading.Texts.size()) : 0;
    Session.ImportFaces        = Outcome.AcceptedCondition ? Reading.Faces : 0;
    Session.ImportRepairs      = Outcome.AcceptedCondition ? Reading.ReorientedFaces + Reading.TolerantEdges : 0;
    Session.ImportMilliseconds = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - Start).count();
    Conclude(State, Outcome);
    if (!Outcome.AcceptedCondition) return;
    ++Session.Imports;
    SolidArcSelection Picked;
    Picked.LastKind = SolidArcSelectionKind::Body;
    for (size_t Index = Before; Index < State.Document.Tree.Steps.size(); ++Index) Picked.Bodies.push_back(State.Document.Tree.Steps[Index].Label);
    State.Document.Selection = Picked;
    FrameSelection(State);
    const char* Unit = Reading.FileUnitMillimetres == 25.4 ? "in" : Reading.FileUnitMillimetres == 1000.0 ? "m" : Reading.FileUnitMillimetres == 1.0 ? "mm" : "mm×";
    Announce(State, Compose("Imported %s · %d solid(s) · %d face(s) · file unit %s · %d loop(s) reoriented · %d tolerant edge(s) · %.0f ms", Path.c_str(), Session.ImportSolids,
                            Session.ImportFaces, Unit, Reading.ReorientedFaces, Reading.TolerantEdges, Session.ImportMilliseconds));
}

// Export… (or Save as… with a .step / .stp / .stl name, as Plasticity exports by file type): the selected bodies, else
// every shown body, in millimetres; the document path is left as it is.
void ExportBodies(SolidArcWorkspaceState& State, const std::string& Path)
{
    SolidArcExchangeSession& Session = State.Exchange;
    Session.Export = ExportSolidArcBodies(State.Document, Path);
    if (!Session.Export.Refusal.empty()) { Announce(State, "Refused: " + Session.Export.Refusal); return; }
    ++Session.Exports;
    const SolidArcExportOutcome& Export = Session.Export;
    const std::string Figures = Export.Format == SolidArcExchangeFormat::Step ? Compose("%d face(s)%s", Export.Faces, Export.Approximated > 0 ? Compose(" · %d certified stand-in(s)", Export.Approximated).c_str() : "")
                                                                             : Compose("%d facet(s)", Export.Triangles);
    Announce(State, Compose("Exported %d body(ies) as %s → %s · %s · %.0f bytes · %.0f ms", int(Export.Bodies.size()), SolidArcExchangeFormatLabel(Export.Format), Path.c_str(),
                            Figures.c_str(), Export.Bytes, Export.Milliseconds));
}

// Import… dispatches by extension: .step / .stp → bodies, .toml → VehicleCurves.
void ImportFile(SolidArcWorkspaceState& State, const std::string& Path)
{
    const SolidArcExchangeFormat Format = SolidArcExchangeFormatOf(Path);
    if (Format == SolidArcExchangeFormat::Step) ImportStep(State, Path);
    else if (Format == SolidArcExchangeFormat::Toml) ImportCurves(State, Path);
    else
    {
        State.Exchange.ImportPath    = Path;
        State.Exchange.ImportRefusal = "Import: " + Path + " is neither .step / .stp nor a VehicleCurves .toml";
        Announce(State, "Refused: " + State.Exchange.ImportRefusal);
    }
}

// L (Plasticity loft): the selected curves, in pick order. Closed sections loft a solid shell of its own (the skin and
// its two plane caps); open sections — or G1 / G2 to a neighbour — loft a panel; crossing curves make a Gordon network.
void LoftSelectedCurves(SolidArcWorkspaceState& State)
{
    const std::vector<std::string> Curves = State.Document.Selection.Curves;
    SolidArcCommandOutcome         Outcome;
    bool                           SolidCondition = false;
    if (State.LoftContinuity == 0 && Curves.size() >= 2)
    {
        Outcome        = LoftSolidArcCurves(State.Document, Curves, 0, true);                // Refused (document untouched) when a section is open
        SolidCondition = Outcome.AcceptedCondition;
    }
    if (!SolidCondition) Outcome = LoftSolidArcCurves(State.Document, Curves, State.LoftContinuity);
    Conclude(State, Outcome);
    if (!Outcome.AcceptedCondition) return;
    if (SolidCondition)
    {
        Announce(State, Compose("Lofted %d closed section(s) into a solid shell (2 plane caps) · %s · %.0f ms", int(Curves.size()),
                                State.Document.Summary.SoundCondition ? "sound" : "audit findings", Outcome.Milliseconds));
        return;
    }
    const bool NetworkCondition = !State.Document.Tree.Steps.empty() && State.Document.Tree.Steps.back().Kind == FeatureStepKind::Network;
    const char* Order = State.LoftContinuity == 2 ? " · G2" : State.LoftContinuity == 1 ? " · G1" : "";
    Announce(State, Compose("%s %d curve(s) into a panel%s (%s) · %d panel(s) · open sections stay a sheet · %.0f ms", NetworkCondition ? "Network of" : "Lofted", int(Curves.size()),
                            NetworkCondition ? "" : Order, State.Document.Summary.SoundCondition ? "sound" : "audit findings", int(State.Document.Regeneration.Panels.size()),
                            Outcome.Milliseconds));
}

// Alt+X (Plasticity mirror, gizmo x): every panel gets its image through x = 0; halves sew along the plane.
void MirrorPanels(SolidArcWorkspaceState& State)
{
    const SolidArcCommandOutcome Outcome = MirrorSolidArcPanels(State.Document, 0);
    Conclude(State, Outcome);
    if (Outcome.AcceptedCondition) Announce(State, Compose("Mirrored through x = 0 → %d panel(s), %s · %.0f ms", int(State.Document.Regeneration.Panels.size()),
                                                           State.Document.Summary.SoundCondition ? "sound" : "audit findings", Outcome.Milliseconds));
}

// Close: plane caps on the planar rim loops → solid.
void ClosePanels(SolidArcWorkspaceState& State)
{
    const SolidArcCommandOutcome Outcome = CloseSolidArcPanels(State.Document);
    Conclude(State, Outcome);
    if (Outcome.AcceptedCondition) Announce(State, Compose("Closed → solid · volume %.5f m³ · %s · %.0f ms", State.Document.Summary.Volume, State.Document.Summary.SoundCondition ? "sound" : "audit findings",
                                                           Outcome.Milliseconds));
}

void PickCurve(SolidArcWorkspaceState& State, const std::string& Label, SolidArcPickIntent Intent)
{
    const bool        Member  = CurveOrder(State, Label) > 0;
    const std::string Caption = CaptionOf(State.Document, Label);
    switch (Intent)
    {
    case SolidArcPickIntent::Replace:
        SelectSolidArcCurve(State.Document, Label, false);
        State.FocusedStep = Label;
        break;
    case SolidArcPickIntent::Add:
    case SolidArcPickIntent::Extend:
        if (!Member) SelectSolidArcCurve(State.Document, Label, true);
        break;
    case SolidArcPickIntent::Remove:
        if (Member) SelectSolidArcCurve(State.Document, Label, true);
        break;
    }
    const CurveFitOutcome* Fit = FindFittedCurve(State.Document, Label);
    if (!Fit)
    {
        Announce(State, Compose("%s sketch %s · %d selected · L lofts the picks in order · the gizmo moves / turns them", Intent == SolidArcPickIntent::Remove ? "Removed" : "Selected",
                                Caption.c_str(), int(State.Document.Selection.Curves.size())));
        return;
    }
    Announce(State, Compose("%s curve %s · %d poles · fit %.3f mm · %d selected", Intent == SolidArcPickIntent::Remove ? "Removed" : "Selected", Caption.c_str(),
                            Fit ? int(Fit->Curve.Definition->Poles.size()) : 0, Fit ? Fit->WorstDeviation * 1000.0 : 0.0, int(State.Document.Selection.Curves.size())));
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    VIEWPORT PICKING
//------------------------------------------------------------------------------------------------------------------------

// Named views also choose the drawing plane (and its grid); Isometric / free orbit keep the active one.
void PresentView(SolidArcWorkspaceState& State, OrbitViewPreset Preset)
{
    PresentOrbitView(State.Camera, Preset);
    switch (Preset)
    {
    case OrbitViewPreset::Top:
    case OrbitViewPreset::Bottom: State.Style.GridPlane = 0; break;
    case OrbitViewPreset::Front:
    case OrbitViewPreset::Back:   State.Style.GridPlane = 1; break;
    case OrbitViewPreset::Right:
    case OrbitViewPreset::Left:   State.Style.GridPlane = 2; break;
    case OrbitViewPreset::Isometric: break;
    }
}

OrbitCameraFrame ViewportFrame(const SolidArcWorkspaceState& State)
{
    return FrameOrbitCamera(State.Camera, std::max(1.0f, State.ViewportRect[2]), std::max(1.0f, State.ViewportRect[3]));
}

// 💡 The GPU identity under the pointer is primary (identity target readback); modes 2 / 3 retarget it with the
//    exact CPU ray pick so an edge stroke never blocks a face click and a face never blocks a nearby edge.
void PickAtPointer(SolidArcWorkspaceState& State, ImVec2 Local, SolidArcPickIntent Intent)
{
    const OrbitCameraFrame Frame = ViewportFrame(State);
    const OrbitRay         Ray   = CastOrbitRay(Frame, Local.x, Local.y);
    uint32_t                  Identity = State.HoverIdentity;
    if (State.PickMode == SolidArcPickMode::ControlPoint || State.PickMode == SolidArcPickMode::Edge || State.PickMode == SolidArcPickMode::Any)
    {
        // Curves are overlays: within CurveReach they win over the face / edge underneath.
        const std::string Curve = FindCurveNear(State, Frame, ImVec2(State.ViewportRect[0] + Local.x, State.ViewportRect[1] + Local.y));
        if (!Curve.empty())
        {
            PickCurve(State, Curve, Intent);
            return;
        }
        if (State.PickMode == SolidArcPickMode::ControlPoint)
        {
            if (Intent == SolidArcPickIntent::Replace && !State.Document.Selection.Curves.empty())
            {
                ClearSolidArcSelection(State.Document);
                Announce(State, "Selection cleared");
            }
            return;
        }
    }
    if (State.PickMode == SolidArcPickMode::Edge && (Identity == 0 || !(Identity & IdentityEdgeBit)))
    {
        const std::optional<SolidArcPick> Nearby = PickSolidArcScene(State.Scene, Ray, 8.0 * Frame.WorldPerPixel, State.Style.XRayEnabled, ActiveSectionClip(State));
        Identity = Nearby && (Nearby->Identity & IdentityEdgeBit) ? Nearby->Identity : 0;
    }
    if (State.PickMode == SolidArcPickMode::Face && (Identity & IdentityEdgeBit))
    {
        const std::optional<SolidArcPick> Behind = PickSolidArcScene(State.Scene, Ray, 0.0, false, ActiveSectionClip(State));
        Identity = Behind ? Behind->Identity : 0;
    }
    if (Identity == IdentitySectionCap) Identity = 0;                                        // The section cap is no entity
    if (Identity != 0)
    {
        // A locked body (Outliner lock) stays in view but takes no pick.
        const uint32_t Index  = (Identity & ~IdentityEdgeBit) - 1u;
        const uint32_t BodyAt = (Identity & IdentityEdgeBit) ? (Index < State.Scene.Edges.size() ? State.Scene.Edges[Index].Body : ~0u)
                                                             : (Index < State.Scene.Faces.size() ? State.Scene.Faces[Index].Body : ~0u);
        if (BodyAt < State.Scene.Bodies.size() && SolidArcLockedCondition(State.Document, State.Scene.Bodies[BodyAt].Key))
        {
            Announce(State, State.Scene.Bodies[BodyAt].Caption + " is locked (Outliner lock to unlock)");
            return;
        }
    }
    if (Identity == 0)
    {
        if (Intent == SolidArcPickIntent::Replace && (!State.Document.Selection.Faces.empty() || !State.Document.Selection.Edges.empty()
                                                      || !State.Document.Selection.Curves.empty() || !State.Document.Selection.Bodies.empty()))
        {
            ClearSolidArcSelection(State.Document);
            Announce(State, "Selection cleared");
        }
        return;
    }
    if (State.PickMode == SolidArcPickMode::Solid)
    {
        // Mode 4 (solid): the body under the pointer, by its key. Shift adds (pick order = Boolean order), Ctrl removes.
        const uint32_t Index   = (Identity & ~IdentityEdgeBit) - 1u;
        const uint32_t BodyAt  = (Identity & IdentityEdgeBit) ? (Index < State.Scene.Edges.size() ? State.Scene.Edges[Index].Body : ~0u)
                                                              : (Index < State.Scene.Faces.size() ? State.Scene.Faces[Index].Body : ~0u);
        if (BodyAt >= State.Scene.Bodies.size()) return;
        const SolidArcSceneBody&        Body     = State.Scene.Bodies[BodyAt];
        const std::vector<std::string>& Picked   = State.Document.Selection.Bodies;
        const bool                      Member   = std::find(Picked.begin(), Picked.end(), Body.Key) != Picked.end();
        if (Intent == SolidArcPickIntent::Replace)
        {
            SelectSolidArcName(State.Document, SolidArcSelectionKind::Body, Body.Key, false);
            Announce(State, Compose("Selected %s (%d faces)", Body.Caption.c_str(), int(Body.FaceCount)));
        }
        else if (Intent == SolidArcPickIntent::Add)
        {
            if (!Member) SelectSolidArcName(State.Document, SolidArcSelectionKind::Body, Body.Key, true);
            Announce(State, Compose("Added %s · %d bodies picked", Body.Caption.c_str(), int(State.Document.Selection.Bodies.size())));
        }
        else
        {
            if (Member) SelectSolidArcName(State.Document, SolidArcSelectionKind::Body, Body.Key, true);
            Announce(State, "Removed " + Body.Caption);
        }
        return;
    }
    const std::string           Name = NameSolidArcIdentity(State.Scene, Identity);
    const SolidArcSelectionKind Kind = SolidArcIdentityKind(Identity);
    const auto Member = [&](const std::string& Candidate)
    {
        const std::vector<std::string>& Names = Kind == SolidArcSelectionKind::Edge ? State.Document.Selection.Edges : State.Document.Selection.Faces;
        return std::find(Names.begin(), Names.end(), Candidate) != Names.end();
    };
    switch (Intent)
    {
    case SolidArcPickIntent::Replace:
        SelectSolidArcName(State.Document, Kind, Name, false);
        Announce(State, "Selected " + Name);
        break;
    case SolidArcPickIntent::Add:
        if (!Member(Name)) SelectSolidArcName(State.Document, Kind, Name, true);
        Announce(State, "Added " + Name);
        break;
    case SolidArcPickIntent::Remove:
        if (Member(Name)) SelectSolidArcName(State.Document, Kind, Name, true);
        Announce(State, "Removed " + Name);
        break;
    case SolidArcPickIntent::Extend:
    {
        const std::vector<std::string> Chain = Kind == SolidArcSelectionKind::Edge ? SolidArcTangentChain(State.Document, Name) : std::vector<std::string>{ Name };
        for (const std::string& Link : Chain)
        {
            if (!Member(Link)) SelectSolidArcName(State.Document, Kind, Link, true);
        }
        Announce(State, Compose("Extended along the tangent chain: %d %s", int(Chain.size()), Kind == SolidArcSelectionKind::Edge ? "edge(s)" : "face(s)"));
        break;
    }
    }
}

// Curves wholly inside the screen box, appended in feature-tree order (the import order of sections).
void SelectInBox(SolidArcWorkspaceState& State, const OrbitCameraFrame& Frame, ImVec2 Corner, ImVec2 Opposite, SolidArcPickIntent Intent)
{
    const ImVec2 Low(std::min(Corner.x, Opposite.x), std::min(Corner.y, Opposite.y));
    const ImVec2 High(std::max(Corner.x, Opposite.x), std::max(Corner.y, Opposite.y));
    if (Intent == SolidArcPickIntent::Replace) ClearSolidArcSelection(State.Document);
    int Count = 0;
    for (const FeatureStep& Step : State.Document.Tree.Steps)
    {
        const CurveFitOutcome* Fit = Step.Kind == FeatureStepKind::SpaceCurve ? FindFittedCurve(State.Document, Step.Label) : nullptr;
        if (!Fit || !CurveReachableCondition(State.Document, Step.Label)) continue;
        const std::vector<ImVec2> Pixels = ProjectFittedCurve(State, Frame, *Fit->Curve.Definition);
        if (int(Pixels.size()) != CurveSamples + 1) continue;                               // Partly off screen / behind the eye
        const bool Inside = std::all_of(Pixels.begin(), Pixels.end(), [&](ImVec2 Pixel) { return Pixel.x >= Low.x && Pixel.x <= High.x && Pixel.y >= Low.y && Pixel.y <= High.y; });
        if (!Inside) continue;
        const bool Member = CurveOrder(State, Step.Label) > 0;
        if (Intent == SolidArcPickIntent::Remove ? Member : !Member) SelectSolidArcCurve(State.Document, Step.Label, true);
        ++Count;
    }
    // B-rep edges (modes 2 / 5): wholly inside the box; hidden ones only in x-ray.
    State.Box.Edges = 0;
    if (State.PickMode != SolidArcPickMode::ControlPoint)
    {
        const std::array<double, 2> LocalLow{ Low.x - State.ViewportRect[0], Low.y - State.ViewportRect[1] };
        const std::array<double, 2> LocalHigh{ High.x - State.ViewportRect[0], High.y - State.ViewportRect[1] };
        for (const uint32_t Index : EncloseSolidArcEdges(State.Scene, Frame, LocalLow, LocalHigh, State.Style.XRayEnabled, ActiveSectionClip(State)))
        {
            const std::string& Name = State.Scene.Edges[Index].Name;
            const uint32_t     Body = State.Scene.Edges[Index].Body;
            if (Body < State.Scene.Bodies.size() && SolidArcLockedCondition(State.Document, State.Scene.Bodies[Body].Key)) continue;   // Locked body
            const std::vector<std::string>& Edges = State.Document.Selection.Edges;
            const bool Member = std::find(Edges.begin(), Edges.end(), Name) != Edges.end();
            if (Intent == SolidArcPickIntent::Remove ? Member : !Member) SelectSolidArcName(State.Document, SolidArcSelectionKind::Edge, Name, true);
            ++State.Box.Edges;
        }
    }
    ++State.Box.Commits;
    const char* Verb = Intent == SolidArcPickIntent::Remove ? "removed" : "selected";
    if (State.Box.Edges > 0)
        Announce(State, Compose("Box %s %d curve(s) + %d edge(s)%s · %d edge(s) selected", Verb, Count, State.Box.Edges, State.Style.XRayEnabled ? " (x-ray)" : "",
                                int(State.Document.Selection.Edges.size())));
    else
        Announce(State, Compose("Box %s %d curve(s) · %d selected", Verb, Count, int(State.Document.Selection.Curves.size())));
}

SolidArcPickIntent PickIntentOf(const ImGuiIO& Io)
{
    if (Io.KeyAlt) return SolidArcPickIntent::Extend;
    if (Io.KeyCtrl) return SolidArcPickIntent::Remove;
    if (Io.KeyShift) return SolidArcPickIntent::Add;
    return SolidArcPickIntent::Replace;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    HEADER + DOCUMENT BAR
//------------------------------------------------------------------------------------------------------------------------

// The Open / Save / Import / Export path forms and the Examples list (on the header ID stack).
// 📝 Construct › Image plane: decode the picture (refuse unreadable files), size it Width wide with the picture's aspect,
//    centred on the origin of the chosen view plane, upright (top of the picture up, or −y on the top plane).
bool AddImagePlane(SolidArcWorkspaceState& State)
{
    const std::string Path = State.PicturePath.data();
    if (Path.empty()) { Announce(State, "Image plane: type a picture path (or Browse…)"); return false; }
    const std::string     DocumentPath = State.DocumentPath.empty() ? State.Options.DocumentPath : State.DocumentPath;
    const SolidArcPicture Picture      = DecodeSolidArcPicture(ResolveSolidArcImagePath(Path, DocumentPath));
    if (!Picture.Refusal.empty() || Picture.Width == 0 || Picture.Height == 0)
    {
        Announce(State, "Image plane refused: " + (Picture.Refusal.empty() ? std::string("empty picture") : Picture.Refusal));
        return false;
    }
    const double Width  = std::clamp(double(State.PictureWidth), 1.0e-3, 1.0e4);                      // [len]
    const double Height = Width * double(Picture.Height) / double(Picture.Width);                    // [len]
    SolidArcImagePlane Plane;
    std::string Stem = std::filesystem::path(Path).stem().string();
    if (Stem.empty()) Stem = "Picture";
    Plane.Name = Stem;
    for (int Suffix = 2; std::any_of(State.Document.ImagePlanes.begin(), State.Document.ImagePlanes.end(), [&](const SolidArcImagePlane& Other) { return Other.Name == Plane.Name; }); ++Suffix)
        Plane.Name = Stem + " " + std::to_string(Suffix);
    Plane.Path = Path;
    static const char* const PlaneNames[3] = { "top (XY)", "front (XZ)", "right (YZ)" };
    const int Choice = std::clamp(State.PicturePlane, 0, 2);
    if (Choice == 0) { Plane.Corner = { -0.5 * Width, 0.5 * Height, 0.0 }; Plane.Right = { Width, 0.0, 0.0 }; Plane.Down = { 0.0, -Height, 0.0 }; }
    if (Choice == 1) { Plane.Corner = { -0.5 * Width, 0.0, 0.5 * Height }; Plane.Right = { Width, 0.0, 0.0 }; Plane.Down = { 0.0, 0.0, -Height }; }
    if (Choice == 2) { Plane.Corner = { 0.0, -0.5 * Width, 0.5 * Height }; Plane.Right = { 0.0, Width, 0.0 }; Plane.Down = { 0.0, 0.0, -Height }; }
    State.Document.ImagePlanes.push_back(Plane);
    State.ImagePlanesEnabled = true;
    State.DirtyCondition     = true;
    State.AutosaveTimer      = State.Options.AutosaveDelay;
    Announce(State, Compose("Image plane %s: %u × %u px on the %s plane, %.3f × %.3f m (Inspector: opacity, crop, placement)", Plane.Name.c_str(), Picture.Width, Picture.Height,
                            PlaneNames[Choice], Width, Height));
    return true;
}

void DrawImagePlanePopup(SolidArcWorkspaceState& State)
{
    if (!ImGui::BeginPopupModal("Image plane", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) return;
    ImGui::TextUnformatted("A reference picture (PNG / JPEG / BMP / GIF) on a view plane, to trace over · not part of the history");
    ImGui::SetNextItemWidth(SolidArcFileDialogAvailableCondition() ? 360.0f : 460.0f);
    const bool Entered = ImGui::InputText("##PicturePath", State.PicturePath.data(), State.PicturePath.size(), ImGuiInputTextFlags_EnterReturnsTrue);
    RegisterWidget(State, "PicturePathField");
    if (SolidArcFileDialogAvailableCondition())
    {
        ImGui::SameLine();
        if (PressButton(State, "Browse…###PictureBrowse", ImVec2(90, 0)))
        {
            const std::string Chosen = BrowseSolidArcPicture();
            if (!Chosen.empty()) std::snprintf(State.PicturePath.data(), State.PicturePath.size(), "%s", Chosen.c_str());
        }
    }
    ImGui::TextUnformatted("Plane");
    ImGui::SameLine();
    ImGui::RadioButton("Top (XY)###PictureTop", &State.PicturePlane, 0);
    RegisterWidget(State, "PictureTop");
    ImGui::SameLine();
    ImGui::RadioButton("Front (XZ)###PictureFront", &State.PicturePlane, 1);
    RegisterWidget(State, "PictureFront");
    ImGui::SameLine();
    ImGui::RadioButton("Right (YZ)###PictureRight", &State.PicturePlane, 2);
    RegisterWidget(State, "PictureRight");
    ImGui::SetNextItemWidth(140);
    ImGui::InputFloat("Width (m)###PictureWidth", &State.PictureWidth, 0.5f, 1.0f, "%.3f");
    RegisterWidget(State, "PictureWidth");
    if (PressButton(State, "Add###PictureAdd", ImVec2(120, 0)) || Entered)
    {
        if (AddImagePlane(State))
        {
            State.PicturePath[0] = 0;
            ImGui::CloseCurrentPopup();
        }
    }
    ImGui::SameLine();
    if (PressButton(State, "Cancel###PictureCancel", ImVec2(120, 0))) ImGui::CloseCurrentPopup();
    ImGui::EndPopup();
}

void DrawPathPopups(SolidArcWorkspaceState& State)
{
    DrawImagePlanePopup(State);
    for (const char* Title : { "Open document", "Save document", "Import", "Export" })
    {
        const bool OpenCondition   = std::strcmp(Title, "Open document") == 0;
        const bool ImportCondition = std::strcmp(Title, "Import") == 0;
        const bool ExportCondition = std::strcmp(Title, "Export") == 0;
        const std::string DocumentPath = State.DocumentPath.empty() ? State.Options.DocumentPath : State.DocumentPath;
        if (ImGui::IsPopupOpen(Title) && State.PathText[0] == 0)
            std::snprintf(State.PathText.data(), State.PathText.size(), "%s",
                          (ImportCondition ? (State.Exchange.Export.Format == SolidArcExchangeFormat::Step && State.Exchange.Export.Refusal.empty() ? State.Exchange.Export.Path : State.Options.CurvesPath) : ExportCondition ? SolidArcExportPath(DocumentPath, ".step") : DocumentPath).c_str());
        if (ImGui::BeginPopupModal(Title, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            const size_t ExportCount = SolidArcExportBodies(State.Document).size();
            const std::string Prompt = ImportCondition ? std::string("Import a STEP file (.step / .stp: every solid becomes a body) or VehicleCurves.toml (curves fitted within 0.2 mm)")
                                     : ExportCondition ? Compose("Export %d %s body(ies) in millimetres: .step / .stp (STEP AP214, exact B-rep) or .stl (binary facets)", int(ExportCount),
                                                                 State.Document.Selection.Bodies.empty() ? "shown" : "selected")
                                     : OpenCondition   ? std::string("Open a .solidarc document (a .step / .stp file is imported into this one)")
                                                       : std::string("Save the document as .solidarc (.step / .stp / .stl exports the bodies instead)");
            ImGui::TextUnformatted(Prompt.c_str());
            ImGui::SetNextItemWidth(420);
            const bool Entered = ImGui::InputText("##Path", State.PathText.data(), State.PathText.size(), ImGuiInputTextFlags_EnterReturnsTrue);
            RegisterWidget(State, "PathField");
            if (PressButton(State, ImportCondition ? "Import###PathImport" : ExportCondition ? "Export###PathExport" : OpenCondition ? "Open###PathOpen" : "Save###PathSave", ImVec2(120, 0)) || Entered)
            {
                const std::string Path = State.PathText.data();
                const SolidArcExchangeFormat Format = SolidArcExchangeFormatOf(Path);
                if (ImportCondition) ImportFile(State, Path);
                else if (ExportCondition) ExportBodies(State, Path);
                else if (OpenCondition && Format == SolidArcExchangeFormat::Step) ImportStep(State, Path);
                else if (OpenCondition) OpenDocument(State, Path, true);
                else if (Format == SolidArcExchangeFormat::Step || Format == SolidArcExchangeFormat::Stl) ExportBodies(State, Path);
                else SaveDocument(State, Path);
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
    if (ImGui::BeginPopup("Examples"))
    {
        ImGui::TextUnformatted("Examples");
        ImGui::Separator();
        if (ImGui::Selectable("Bracket specimen · block, hole, fillets###ExampleBracket"))
        {
            AdoptDocument(State, BuildSolidArcSpecimen(), true);
            State.DocumentPath.clear();
            Announce(State, "Example: Bracket specimen");
        }
        RegisterWidget(State, "ExampleBracket");
        ImGui::EndPopup();
    }
}

// 📝 HTML .button: 35 px pill, #222, #ffffff0d border, padding 0 16, 11 px; hover #292929 / #fff. Glyph (17 px, gap 8)
//    leads the label when given. Returns the press; the rectangle is recorded under Key for the scripts.
bool SheetButton(SolidArcWorkspaceState& State, SolidArcSheet& Sheet, const char* Key, float Left, float Centre, const char* Label,
                 bool EnabledCondition = true, IconSymbol Glyph = IconSymbol::Count, float* Width = nullptr)
{
    const bool        GlyphCondition = Glyph != IconSymbol::Count;
    const float       Extent         = 2.0f + 32.0f + Sheet.Measure(Label, 11.0f) + (GlyphCondition ? 17.0f + 8.0f : 0.0f);
    const PlaneExtent Box            = Spanning(Left, Centre - 17.5f, Extent, 35.0f);
    const SolidArcPress Press        = PressSolidArcRegion(State.Widgets, Key, Box, EnabledCondition);
    const float       Opacity        = EnabledCondition ? 1.0f : 0.3f;                      // button:disabled
    const ColorQuad   Ink            = ControlKit::Faded(SolidArcInk(Press.HoveredCondition ? 0xffffff : 0xeeeeee), Opacity);
    Sheet.Box(Box, ControlKit::Faded(SolidArcInk(Press.HoveredCondition ? 0x292929 : 0x222222), Opacity), 24.0f, ControlKit::Faded(SolidArcWash(0x0d), Opacity));
    float Cursor = Left + 17.0f;
    if (GlyphCondition)
    {
        Sheet.Icon(Glyph, Cursor, Centre - 8.5f, 17.0f, Ink);
        Cursor += 17.0f + 8.0f;
    }
    Sheet.Text(Cursor, Centre, 11.0f, Ink, Label);
    if (Width) *Width = Extent;
    return Press.ClickedCondition;
}

// 📝 HTML .text-button: 11 px #969696, padding 6 8, radius 16, gap 8, glyph 13 after the label when given.
bool SheetTextButton(SolidArcWorkspaceState& State, SolidArcSheet& Sheet, const char* Key, float Right, float Centre, const char* Label,
                     IconSymbol Glyph = IconSymbol::Count, float* Width = nullptr)
{
    const bool        GlyphCondition = Glyph != IconSymbol::Count;
    const float       Extent         = 16.0f + Sheet.Measure(Label, 11.0f) + (GlyphCondition ? 8.0f + 13.0f : 0.0f);
    const float       Height         = 11.0f * SolidArcStyleTokens::FontScale + 12.0f;
    const PlaneExtent Box            = Spanning(Right - Extent, Centre - 0.5f * Height, Extent, Height);
    const SolidArcPress Press        = PressSolidArcRegion(State.Widgets, Key, Box);
    const ColorQuad   Ink            = SolidArcInk(Press.HoveredCondition ? 0xffffff : 0x969696);
    if (Press.HoveredCondition) Sheet.Box(Box, SolidArcInk(0x292929), 16.0f);
    const float Advance = Sheet.Text(Box.MinimumX + 8.0f, Centre, 11.0f, Ink, Label);
    if (GlyphCondition) Sheet.Icon(Glyph, Box.MinimumX + 8.0f + Advance + 8.0f, Centre - 6.5f, 13.0f, Ink);
    if (Width) *Width = Extent;
    return Press.ClickedCondition;
}

// 📝 HTML .icon-button: circle of Size, glyph centred (#888, hover #292929 / #fff), optional hairline border.
bool SheetIconButton(SolidArcWorkspaceState& State, SolidArcSheet& Sheet, const char* Key, float CentreX, float CentreY, float Size, IconSymbol Glyph,
                     float GlyphSize, uint32_t BorderAlpha = 0, bool EnabledCondition = true, bool ActiveCondition = false)
{
    const PlaneExtent   Box   = Spanning(CentreX - 0.5f * Size, CentreY - 0.5f * Size, Size, Size);
    const SolidArcPress Press = PressSolidArcRegion(State.Widgets, Key, Box, EnabledCondition);
    const float         Opacity = EnabledCondition ? 1.0f : 0.3f;
    const bool          Lit     = Press.HoveredCondition || ActiveCondition;                 // .icon-button.active: #292929 / #e0e0e0
    Sheet.Box(Box, Lit ? SolidArcInk(0x292929) : SolidArcInk(0, 0.0f), 0.5f * Size,
              BorderAlpha ? ControlKit::Faded(SolidArcWash(BorderAlpha), Opacity) : SolidArcInk(0, 0.0f));
    Sheet.Icon(Glyph, CentreX - 0.5f * GlyphSize, CentreY - 0.5f * GlyphSize, GlyphSize,
               ControlKit::Faded(SolidArcInk(Press.HoveredCondition ? 0xffffff : ActiveCondition ? 0xe0e0e0 : 0x888888), Opacity));
    return Press.ClickedCondition;
}

// A borderless, transparent ImGui window over Extent that the sheet draws into (it owns the ID stack of its buttons).
// 📝 ImGui inserts NoBringToFrontOnFocus windows at the *back* of the display order when created; overlays drawn over
//    another sheet window (the viewport chrome) pass OverlayCondition so they are appended in front of it instead.
void BeginSheetWindow(const char* Name, ImVec2 Position, ImVec2 Size, bool OverlayCondition = false)
{
    ImGui::SetNextWindowPos(Position);
    ImGui::SetNextWindowSize(Size);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::Begin(Name, nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground |
                                (OverlayCondition ? 0 : ImGuiWindowFlags_NoBringToFrontOnFocus) | ImGuiWindowFlags_NoScrollWithMouse);
}

void EndSheetWindow()
{
    ImGui::End();
    ImGui::PopStyleVar(2);
}

// 📝 HTML .app-header (68 px, padding 0 24, gap 28): the document dock (tab + new), the Workspace / Examples nav, and the
//    header end (Import, Export, Save as, kernel tag, help, Save). Opens the path popups on its own ID stack.
void DrawHeader(SolidArcWorkspaceState& State, const ImVec2 Frame)
{
    BeginSheetWindow("Header", ImVec2(0, 0), ImVec2(Frame.x, HeaderHeight));
    SolidArcSheet Sheet;
    Sheet.Begin();
    const float Centre = 0.5f * HeaderHeight;

    // Header end, laid out from the right: Save .solidarc, help, kernel tag, Save as…, Export…, Import… (gap 15).
    float Right = Frame.x - 24.0f;
    const auto Measured = [&](const char* Label, bool GlyphCondition) { return 2.0f + 32.0f + Sheet.Measure(Label, 11.0f) + (GlyphCondition ? 25.0f : 0.0f); };
    Right -= Measured("Save .solidarc", true);
    if (SheetButton(State, Sheet, "HeaderSave", Right, Centre, "Save .solidarc", true, IconSymbol::SolidArcSave))
    {
        if (State.DocumentPath.empty()) State.PopupRequest = "Save document";
        else SaveDocument(State, State.DocumentPath);
    }
    Right -= 15.0f + 28.0f;
    SheetIconButton(State, Sheet, "HeaderHelp", Right + 14.0f, Centre, 28.0f, IconSymbol::SolidArcHelp, 17.0f);
    const char* Kernel = "Native CAD · Vulkan viewport";
    Right -= 15.0f + Sheet.Measure(Kernel, 10.0f) + 12.0f;
    Sheet.Surface().FillRectangle(Spanning(Right, Centre - 2.5f, 5.0f, 5.0f), SolidArcInk(0x34c759), 2.5f);
    Sheet.Text(Right + 12.0f, Centre, 10.0f, SolidArcInk(0x919191), Kernel);
    const struct { const char* Key; const char* Label; const char* Popup; } Ends[] = {
        { "HeaderSaveAs", "Save as…", "Save document" }, { "HeaderExport", "Export…", "Export" }, { "HeaderImport", "Import…", "Import" } };
    for (const auto& End : Ends)
    {
        Right -= 15.0f + Measured(End.Label, false);
        if (SheetButton(State, Sheet, End.Key, Right, Centre, End.Label)) State.PopupRequest = End.Popup;
    }
    const float EndLeft = Right;

    // Document dock (flex 1, 150 … 660 px): one tab (186 px trapezoid, 24 px, top padding 4) and the 24 px + button.
    const float NavWidth = 2.0f * 46.0f + SolidArcSheet().Measure("Workspace", 12.0f) + Sheet.Measure("Examples", 12.0f) + 8.0f + 2.0f;
    const float Dock     = std::clamp(EndLeft - 28.0f - NavWidth - 28.0f - 24.0f, 150.0f, 660.0f);
    {
        const float  TabLeft = 24.0f, TabTop = Centre - 14.0f + 4.0f;
        const PlanePoint Corners[4] = { { TabLeft + 14.0f, TabTop }, { TabLeft + 186.0f - 14.0f, TabTop }, { TabLeft + 186.0f, TabTop + 24.0f }, { TabLeft, TabTop + 24.0f } };
        Sheet.Surface().FillPolygon(Corners, 4, SolidArcInk(0x1a1a1a));                     // .document-tab.active
        const float TabCentre = TabTop + 12.0f;
        Sheet.Icon(IconSymbol::SolidArcBody, TabLeft + 20.0f, TabCentre - 6.5f, 13.0f, SolidArcInk(0xeeeeee));
        const std::string Caption = State.Document.Title + ".solidarc";
        const float LabelRight = TabLeft + 186.0f - 20.0f - 15.0f - 7.0f - (State.DirtyCondition ? 4.0f + 7.0f : 0.0f);
        Sheet.TextClipped(TabLeft + 20.0f + 13.0f + 7.0f, TabCentre, LabelRight - (TabLeft + 40.0f), 11.0f, SolidArcInk(0xeeeeee), Caption.c_str());
        if (State.DirtyCondition) Sheet.Surface().FillRectangle(Spanning(LabelRight + 7.0f, TabCentre - 2.0f, 4.0f, 4.0f), SolidArcInk(0x999999), 2.0f);
        const float NewCentre = std::min(TabLeft + 186.0f, 24.0f + Dock - 24.0f) + 8.0f + 12.0f;
        if (SheetIconButton(State, Sheet, "HeaderNew", NewCentre, Centre, 24.0f, IconSymbol::SolidArcPlus, 13.0f, 0x12)) StartNewDocument(State);
    }

    // Nav: Workspace (active, #242424, #ffffff0c border) and Examples; 38 px pills, padding 0 23, 12 px.
    float Cursor = 24.0f + Dock + 28.0f;
    for (const char* Label : { "Workspace", "Examples" })
    {
        const bool        Active = std::strcmp(Label, "Workspace") == 0;
        const PlaneExtent Pill   = Spanning(Cursor, Centre - 19.0f, 46.0f + Sheet.Measure(Label, 12.0f) + (Active ? 2.0f : 0.0f), 38.0f);
        const SolidArcPress Press = PressSolidArcRegion(State.Widgets, std::string("Nav") + Label, Pill);
        Sheet.Box(Pill, Active ? SolidArcInk(0x242424) : Press.HoveredCondition ? SolidArcInk(0x292929) : SolidArcInk(0, 0.0f), 30.0f,
                  Active ? SolidArcWash(0x0c) : SolidArcInk(0, 0.0f));
        Sheet.Text(Pill.MinimumX + 23.0f + (Active ? 1.0f : 0.0f), Centre, 12.0f, SolidArcInk(Active ? 0xeaeaea : Press.HoveredCondition ? 0xffffff : 0x939393), Label);
        if (Press.ClickedCondition && !Active) State.PopupRequest = "Examples";
        Cursor = Pill.MaximumX + 8.0f;
    }

    if (!State.PopupRequest.empty())
    {
        ImGui::OpenPopup(State.PopupRequest.c_str());                                        // Buttons, Ctrl+O / Ctrl+Shift+S
        State.PopupRequest.clear();
    }
    DrawPathPopups(State);
    EndSheetWindow();
}

// 📝 HTML .document-bar (34 px, padding 0 25, gap 9, 11 px): file glyph, title, .solidarc, dirty dot, divider, location;
//    on the right Restore previous (when an earlier session left one) and Open document.
void DrawDocumentBar(SolidArcWorkspaceState& State, const ImVec2 Frame)
{
    BeginSheetWindow("Documents", ImVec2(0, HeaderHeight), ImVec2(Frame.x, DocumentHeight));
    SolidArcSheet Sheet;
    Sheet.Begin();
    const float Centre = HeaderHeight + 0.5f * DocumentHeight;
    float       Cursor = 25.0f;
    Sheet.Icon(IconSymbol::SolidArcFile, Cursor, Centre - 7.0f, 14.0f, SolidArcInk(0x888888));
    Cursor += 14.0f + 9.0f;
    Cursor += Sheet.Text(Cursor, Centre, 11.0f, SolidArcInk(0xeeeeee), State.Document.Title.c_str()) + 9.0f;
    Cursor += Sheet.Text(Cursor, Centre, 10.0f, SolidArcInk(0x717171), ".solidarc") + 9.0f;
    if (State.DirtyCondition)
    {
        Sheet.Surface().FillRectangle(Spanning(Cursor + 2.0f, Centre - 2.0f, 4.0f, 4.0f), SolidArcInk(0xaaaaaa), 2.0f);
        Cursor += 2.0f + 4.0f + 9.0f;
    }
    Sheet.Surface().FillRectangle(Spanning(Cursor + 9.0f, Centre - 7.5f, 1.0f, 15.0f), SolidArcWash(0x10));
    Cursor += 9.0f + 1.0f + 9.0f + 9.0f;
    const std::string Location = State.DocumentPath.empty() ? std::string("SolidArc / Unsaved") : "SolidArc / " + State.DocumentPath;
    Sheet.Text(Cursor, Centre, 10.0f, SolidArcInk(0x717171), Location.c_str());

    float Right = Frame.x - 25.0f, Width = 0.0f;
    if (SheetTextButton(State, Sheet, "HeaderOpen", Right, Centre, "Open document", IconSymbol::SolidArcFolder, &Width)) State.PopupRequest = "Open document";
    Right -= Width + 9.0f;
    if (std::filesystem::exists(PreviousRecoveryPath(State)) && SheetTextButton(State, Sheet, "RestorePrevious", Right, Centre, "Restore previous", IconSymbol::SolidArcUndo))
        OpenDocument(State, PreviousRecoveryPath(State), false);
    EndSheetWindow();
}

// 📝 HTML .status-bar (31 px, padding 0 21, gap 14, 9 px #8c8c8c): kernel state, figures, the last message; on the right the
//    frame time and device, Metres · Z up, SOLIDARC NATIVE.
void DrawStatusBar(SolidArcWorkspaceState& State, const ImVec2 Frame)
{
    BeginSheetWindow("Status", ImVec2(0, Frame.y - StatusHeight), ImVec2(Frame.x, StatusHeight));
    SolidArcSheet Sheet;
    Sheet.Begin();
    const float     Centre = Frame.y - 0.5f * StatusHeight;
    const ColorQuad Ink    = SolidArcInk(0x8c8c8c);
    float           Cursor = 21.0f;
    Sheet.Surface().FillRectangle(Spanning(Cursor, Centre - 2.5f, 5.0f, 5.0f), SolidArcInk(0x34c759), 2.5f);
    Cursor += 12.0f + Sheet.Text(Cursor + 12.0f, Centre, 9.0f, Ink, "GeometryProcessing ready · Vulkan viewport") + 14.0f;
    const std::string Figures = Compose("%d faces · %d edges", State.Document.Summary.Faces, State.Document.Summary.Edges);
    Cursor += Sheet.Text(Cursor, Centre, 9.0f, Ink, Figures.c_str()) + 14.0f;

    float Right = Frame.x - 21.0f;
    Right -= Sheet.TextRight(Right, Centre, 8.0f, SolidArcInk(0x666666), "NATIVE 0.1") + 7.0f;
    Right -= Sheet.TextRight(Right, Centre, 9.0f, Ink, "SOLIDARC") + 14.0f + 9.0f;
    Sheet.Surface().FillRectangle(Spanning(Right - 1.0f, Centre - 7.5f, 1.0f, 15.0f), SolidArcWash(0x10));
    Right -= 1.0f + 9.0f + 14.0f;
    Right -= Sheet.TextRight(Right, Centre, 9.0f, Ink, "Metres · Z up") + 14.0f + 9.0f;
    Sheet.Surface().FillRectangle(Spanning(Right - 1.0f, Centre - 7.5f, 1.0f, 15.0f), SolidArcWash(0x10));
    Right -= 1.0f + 9.0f + 14.0f;
    const std::string Device = Compose("%.1f ms frame · %s", State.FrameMs, State.DeviceLabel.c_str());
    Right -= Sheet.TextRight(Right, Centre, 9.0f, Ink, Device.c_str()) + 14.0f;
#ifdef FRONTIER_DEVELOPMENT_TRACE
    // 📝 The log is one click away: the button opens the trace panel (F12), which names SolidArcTrace.log beside the exe.
    float             LogWidth = 0.0f;
    const std::string LogLabel = State.Profile.SlowFrames > 0 ? Compose("Log · %llu slow", static_cast<unsigned long long>(State.Profile.SlowFrames)) : std::string("Log (F12)");
    if (SheetTextButton(State, Sheet, "StatusLog", Right, Centre, LogLabel.c_str(), IconSymbol::Count, &LogWidth)) State.TraceShown = !State.TraceShown;
    Right -= LogWidth + 10.0f;
#endif
    const std::string Message = Legible(State.Status);
    Sheet.TextClipped(Cursor, Centre, std::max(0.0f, Right - Cursor), 9.0f, SolidArcInk(0xb0b0b0), Message.c_str());
    EndSheetWindow();
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    OUTLINER
//------------------------------------------------------------------------------------------------------------------------

void DrawConstructDialog(SolidArcWorkspaceState& State)
{
    if (State.ConstructOpen)
    {
        ImGui::OpenPopup("Construct");
        State.ConstructOpen = false;
    }
    State.ConstructShown = false;
    if (!ImGui::BeginPopupModal("Construct", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) return;
    State.ConstructShown = true;
    SolidArcConstructRequest& Form = State.ConstructForm;
    ImGui::Text("%s", SolidArcConstructLabel(Form.Kind));
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
    case SolidArcConstructKind::Block:
        Field("Width [m]###ConstructWidth", Form.Width);
        Field("Depth [m]###ConstructDepth", Form.Depth);
        Field("Height [m]###ConstructHeight", Form.Height);
        break;
    case SolidArcConstructKind::Cylinder:
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
    case SolidArcConstructKind::Sphere:
        Field("Radius [m]###ConstructRadius", Form.Radius);
        break;
    case SolidArcConstructKind::Torus:
        Field("Major [m]###ConstructMajor", Form.Major);
        Field("Minor [m]###ConstructMinor", Form.Minor);
        break;
    case SolidArcConstructKind::Cone:
        Field("Radius [m]###ConstructRadius", Form.Radius);
        Field("Height [m]###ConstructHeight", Form.Height);
        break;
    case SolidArcConstructKind::Wedge:
        Field("Width [m]###ConstructWidth", Form.Width);
        Field("Depth [m]###ConstructDepth", Form.Depth);
        Field("Height [m]###ConstructHeight", Form.Height);
        break;
    case SolidArcConstructKind::Prism:
    {
        Field("Radius [m]###ConstructRadius", Form.Radius);
        Field("Height [m]###ConstructHeight", Form.Height);
        ImGui::SetNextItemWidth(160);
        ImGui::InputInt("Sides###ConstructSides", &Form.Sides);
        RegisterWidget(State, "Sides###ConstructSides");
        Form.Sides = std::clamp(Form.Sides, 3, 64);
        double Degrees = Form.Angle * 180.0 / std::numbers::pi;
        Field("Turn [°]###ConstructTurn", Degrees);
        Form.Angle = Degrees * std::numbers::pi / 180.0;
        break;
    }
    case SolidArcConstructKind::Tube:
        Field("Radius [m]###ConstructRadius", Form.Radius);
        Field("Inner [m]###ConstructInner", Form.Inner);
        Field("Height [m]###ConstructHeight", Form.Height);
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
        SolidArcConstructRequest Request = Form;
        if (FirstBody) Request.Combination = FeatureCombination::New;
        Request.Target = Request.Combination == FeatureCombination::Separate || FirstBody ? std::string{} : BooleanTarget(State);
        const bool FitCondition = FirstBody;
        State.LastCommand = { SolidArcRepeatKind::Construct, BlendProfile::Fillet, 0.0, FaceOperationKind::Extrude, 0.0, Request };
        Conclude(State, ConstructSolidArcPrimitive(State.Document, Request));
        if (FitCondition && !State.Scene.Faces.empty()) FitSolidArcCamera(State.Camera, State.Scene);
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (PressButton(State, "Cancel###ConstructCancel", ImVec2(130, 0))) ImGui::CloseCurrentPopup();
    ImGui::EndPopup();
}

// 📝 Construct menu (index.html #construct-menu): 520 px popover under the viewport Construct chip, header (dot,
//    Construct, Tab, close), a 132 px section column (Solids · Sketch), 4-column catalogue tiles of 72 px (the menu grows
//    past 330 px with the rows). A tile starts the drawing command in the viewport (live dimension lines); Tab inside a
//    solid command opens the exact-size form. Sketch carries every 2-D primitive and every curve form as its own tile.
struct SolidArcConstructTile
{
    const char*           Label;                                                             // [-] - Caption
    const char*           Key;                                                               // [-] - Widget name
    IconSymbol            Glyph;                                                             // [-] - 24 px glyph
    bool                  SolidCondition;                                                    // [-] - Solid (else a sketch tool)
    SolidArcConstructKind Solid;                                                             // [-] - Solid kind
    SolidArcDrawingKind   Sketch;                                                            // [-] - Sketch tool
    bool                  CentreCondition;                                                   // [-] - Sketch tool in centre mode
    bool                  SegmentCondition;                                                  // [-] - Line finishes on its second click
    SketchSplineForm      Form;                                                              // [-] - Curve form of a curve tile
};

constexpr SolidArcConstructTile ConstructSolids[] = {
    { "Box", "ConstructBlock", IconSymbol::SolidArcBody, true, SolidArcConstructKind::Block, {}, false, false, SketchSplineForm::Hermite },
    { "Cylinder", "ConstructCylinder", IconSymbol::SolidArcCylinder, true, SolidArcConstructKind::Cylinder, {}, false, false, SketchSplineForm::Hermite },
    { "Cone", "ConstructCone", IconSymbol::SolidArcCone, true, SolidArcConstructKind::Cone, {}, false, false, SketchSplineForm::Hermite },
    { "Sphere", "ConstructSphere", IconSymbol::SolidArcSphere, true, SolidArcConstructKind::Sphere, {}, false, false, SketchSplineForm::Hermite },
    { "Torus", "ConstructTorus", IconSymbol::SolidArcRing, true, SolidArcConstructKind::Torus, {}, false, false, SketchSplineForm::Hermite },
    { "Wedge", "ConstructWedge", IconSymbol::SolidArcWedge, true, SolidArcConstructKind::Wedge, {}, false, false, SketchSplineForm::Hermite },
    { "Prism", "ConstructPrism", IconSymbol::SolidArcPrism, true, SolidArcConstructKind::Prism, {}, false, false, SketchSplineForm::Hermite },
    { "Tube", "ConstructTube", IconSymbol::SolidArcTube, true, SolidArcConstructKind::Tube, {}, false, false, SketchSplineForm::Hermite } };

// Lines · rectangles · circles and arcs · slot and polygon · the four curve forms (SketchSplineForm).
constexpr SolidArcConstructTile ConstructSketches[] = {
    { "Line", "ConstructSketchSegment", IconSymbol::SolidArcEdge, false, {}, SolidArcDrawingKind::Line, false, true, SketchSplineForm::Hermite },
    { "Polyline", "ConstructSketchLine", IconSymbol::SolidArcPolyline, false, {}, SolidArcDrawingKind::Line, false, false, SketchSplineForm::Hermite },
    { "Rectangle", "ConstructSketchRectangle", IconSymbol::SolidArcRectangle, false, {}, SolidArcDrawingKind::Rectangle, false, false, SketchSplineForm::Hermite },
    { "Centre rect", "ConstructSketchRectangleCentre", IconSymbol::SolidArcCentrerect, false, {}, SolidArcDrawingKind::Rectangle, true, false, SketchSplineForm::Hermite },
    { "Circle", "ConstructSketchCircleCentre", IconSymbol::SolidArcCircle, false, {}, SolidArcDrawingKind::Circle, true, false, SketchSplineForm::Hermite },
    { "2-pt circle", "ConstructSketchCircle", IconSymbol::SolidArcCentre, false, {}, SolidArcDrawingKind::Circle, false, false, SketchSplineForm::Hermite },
    { "Arc", "ConstructSketchArc", IconSymbol::SolidArcArc, false, {}, SolidArcDrawingKind::Arc, false, false, SketchSplineForm::Hermite },
    { "Slot", "ConstructSketchSlot", IconSymbol::SolidArcSlot, false, {}, SolidArcDrawingKind::Slot, false, false, SketchSplineForm::Hermite },
    { "Polygon", "ConstructSketchPolygon", IconSymbol::SolidArcPolygon, false, {}, SolidArcDrawingKind::Polygon, false, false, SketchSplineForm::Hermite },
    { "Hermite", "ConstructSketchCurve", IconSymbol::SolidArcHermite, false, {}, SolidArcDrawingKind::Curve, false, false, SketchSplineForm::Hermite },
    { "Bezier", "ConstructSketchBezier", IconSymbol::SolidArcBezier, false, {}, SolidArcDrawingKind::Curve, false, false, SketchSplineForm::Bezier },
    { "NURBS", "ConstructSketchNurbs", IconSymbol::SolidArcNurbs, false, {}, SolidArcDrawingKind::Curve, false, false, SketchSplineForm::Nurbs },
    { "Cubic spline", "ConstructSketchCubic", IconSymbol::SolidArcCurve, false, {}, SolidArcDrawingKind::Curve, false, false, SketchSplineForm::CubicSpline } };

// Reference: a picture on a view plane (blueprints to trace over); the tile opens the Image plane form.
constexpr SolidArcConstructTile ConstructReferences[] = {
    { "Image plane", "ConstructImagePlane", IconSymbol::SolidArcImage, false, {}, {}, false, false, SketchSplineForm::Hermite } };

constexpr int ConstructSolidCount     = int(sizeof(ConstructSolids) / sizeof(ConstructSolids[0]));
constexpr int ConstructSketchCount    = int(sizeof(ConstructSketches) / sizeof(ConstructSketches[0]));
constexpr int ConstructReferenceCount = int(sizeof(ConstructReferences) / sizeof(ConstructReferences[0]));

// Menu height [px]: header 41 + tile rows (72, gap 6, padding 10) + footer 28, never under the reference 330.
[[nodiscard]] float ConstructMenuHeight(int Count)
{
    const int Rows = (Count + 3) / 4;
    return std::max(330.0f, 41.0f + 10.0f + float(Rows) * 72.0f + float(std::max(0, Rows - 1)) * 6.0f + 10.0f + 28.0f + 2.0f);
}

void DrawConstructMenu(SolidArcWorkspaceState& State, ImVec2 Anchor)
{
    if (State.ConstructMenuRequest)
    {
        ImGui::OpenPopup("ConstructMenu");
        State.ConstructMenuRequest = false;
    }
    // ⚠️ The section shown this frame is fixed here: a section click below changes State.ConstructSection, and the
    //    tiles must still come from the array TileCount was measured on (the next frame draws the new section).
    const int   Displayed  = std::clamp(State.ConstructSection, 0, 2);
    const int   TileCount  = Displayed == 2 ? ConstructReferenceCount : Displayed == 1 ? ConstructSketchCount : ConstructSolidCount;
    const float MenuHeight = ConstructMenuHeight(TileCount);
    ImGui::SetNextWindowPos(Anchor);
    ImGui::SetNextWindowSize(ImVec2(520, MenuHeight));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0, 0, 0, 0));
    const bool Shown = ImGui::BeginPopup("ConstructMenu", ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
    if (!Shown) return;
    SolidArcSheet Sheet;
    Sheet.Begin();
    const PlaneExtent Menu = Spanning(Anchor.x, Anchor.y, 520.0f, MenuHeight);
    Sheet.Box(Menu, SolidArcInk(0x121417), 14.0f, SolidArcWash(0x22));
    const float Left = Menu.MinimumX + 1.0f, Top = Menu.MinimumY + 1.0f, Right = Menu.MaximumX - 1.0f, Bottom = Menu.MaximumY - 1.0f;
    // header — 40 px, #ffffff08, #ffffff12 bottom border, padding 0 12 0 16, gap 12.
    Sheet.Surface().FillRectangle(PlaneExtent{ Left, Top, Right, Top + 40.0f }, SolidArcWash(0x08), 0.0f);
    Sheet.Line(Left, Top + 40.5f, Right, Top + 40.5f, SolidArcWash(0x12));
    const float HeaderCentre = Top + 20.0f;
    Sheet.Surface().FillRectangle(Spanning(Left + 16.0f, HeaderCentre - 4.0f, 8.0f, 8.0f), SolidArcInk(0x4fd8e0), 4.0f);
    Sheet.Text(Left + 16.0f + 8.0f + 12.0f, HeaderCentre, 12.0f, SolidArcInk(0xdddddd), "Construct", SolidArcWeight::Regular);
    if (SheetIconButton(State, Sheet, "ConstructMenuClose", Right - 12.0f - 12.0f, HeaderCentre, 24.0f, IconSymbol::SolidArcClose, 17.0f)) ImGui::CloseCurrentPopup();
    Sheet.TextRight(Right - 12.0f - 24.0f - 12.0f, HeaderCentre, 10.0f, SolidArcInk(0xdddddd), "Tab");
    // #construct-sections — 132 px, #0003, #ffffff12 right border, padding 8 6; button 36 px, radius 7, 11 px.
    const float Body = Top + 41.0f;
    Sheet.Surface().FillRectangle(PlaneExtent{ Left, Body, Left + 132.0f, Bottom }, SolidArcInk(0x000000, 0.2f), 0.0f);
    Sheet.Line(Left + 131.5f, Body, Left + 131.5f, Bottom, SolidArcWash(0x12));
    // Sections: the reference catalogue's Solids and Sketch (3-D primitives become bodies, 2-D tools become sketches), and
    // Reference (image planes, outside the history).
    static const char* const Sections[3] = { "Solids", "Sketch", "Reference" };
    static const int         Counts[3]   = { ConstructSolidCount, ConstructSketchCount, ConstructReferenceCount };
    for (int Index = 0; Index < 3; ++Index)
    {
        const PlaneExtent   Section = Spanning(Left + 6.0f, Body + 8.0f + float(Index) * 40.0f, 132.0f - 12.0f - 1.0f, 36.0f);
        const SolidArcPress Press   = PressSolidArcRegion(State.Widgets, std::string("ConstructSection") + Sections[Index], Section);
        if (Press.ClickedCondition) State.ConstructSection = Index;
        const bool Chosen = State.ConstructSection == Index;
        if (Chosen || Press.HoveredCondition) Sheet.Box(Section, SolidArcWash(Chosen ? 0x16 : 0x0c), 7.0f);
        Sheet.Text(Section.MinimumX + 10.0f, Section.MinimumY + 18.0f, 11.0f, SolidArcInk(Chosen ? 0xeeeeee : 0xaaaaaa), Sections[Index]);
        Sheet.TextRight(Section.MaximumX - 10.0f, Section.MinimumY + 18.0f, 9.0f, SolidArcInk(0x777777), Compose("%d", Counts[Index]).c_str());
    }
    // #construct-tiles — 4 columns, gap 6, padding 10; .catalogue-tile 72 px, #ffffff0c, #ffffff12 border, radius 8, glyph 24.
    const SolidArcConstructTile* Tiles     = Displayed == 2 ? ConstructReferences : Displayed == 1 ? ConstructSketches : ConstructSolids;
    const float                  TilesLeft = Left + 132.0f + 10.0f;
    const float                  TileWidth = 0.25f * (Right - TilesLeft - 10.0f - 3.0f * 6.0f);
    for (int Index = 0; Index < TileCount; ++Index)
    {
        const SolidArcConstructTile& Entry = Tiles[Index];
        const PlaneExtent   Tile  = Spanning(TilesLeft + float(Index % 4) * (TileWidth + 6.0f), Body + 10.0f + float(Index / 4) * (72.0f + 6.0f), TileWidth, 72.0f);
        const std::string   Name  = Entry.Key;
        const SolidArcPress Press = PressSolidArcRegion(State.Widgets, Name, Tile);
        Sheet.Box(Tile, SolidArcWash(Press.HoveredCondition ? 0x1e : 0x0c), 8.0f, SolidArcWash(Press.HoveredCondition ? 0x3c : 0x12));
        const float Stack = 24.0f + 9.0f + 10.0f * SolidArcStyleTokens::FontScale;
        const float GlyphTop = Tile.MinimumY + 0.5f * (72.0f - Stack);
        Sheet.Icon(Entry.Glyph, 0.5f * (Tile.MinimumX + Tile.MaximumX) - 12.0f, GlyphTop, 24.0f, SolidArcInk(0xdddddd));
        const float Width = Sheet.Measure(Entry.Label, 10.0f);
        Sheet.Text(0.5f * (Tile.MinimumX + Tile.MaximumX - Width), GlyphTop + 24.0f + 9.0f + 0.5f * 10.0f * SolidArcStyleTokens::FontScale, 10.0f,
                   SolidArcInk(Press.HoveredCondition ? 0xeeeeee : 0xaaaaaa), Entry.Label);
        if (Press.ClickedCondition)
        {
            ImGui::CloseCurrentPopup();
            if (Displayed == 2) State.PopupRequest = "Image plane";
            else if (Entry.SolidCondition) OpenSolidSession(State, Entry.Solid);
            else
            {
                OpenDrawingSession(State, Entry.Sketch);
                State.Drawing.CentreCondition  = Entry.CentreCondition;
                State.Drawing.SegmentCondition = Entry.SegmentCondition;
                if (Entry.Sketch == SolidArcDrawingKind::Curve) State.Drawing.Form = Entry.Form;
                if (Entry.SegmentCondition) Announce(State, "Line: click the start, then the end · Esc aborts");
                if (Entry.Sketch == SolidArcDrawingKind::Curve) Announce(State, std::string("Curve · ") + SketchSplineFormLabel(Entry.Form) + ": click points · 1-4 switch form · Enter / right-click finishes · Ctrl+Z drops a point · Esc aborts");
            }
        }
    }
    // .construct-catalogue footer — 28 px, #ffffff12 top border, padding 8 10, 9 px #828282.
    Sheet.Line(Left + 132.0f, Bottom - 28.0f + 0.5f, Right, Bottom - 28.0f + 0.5f, SolidArcWash(0x12));
    Sheet.Text(Left + 132.0f + 10.0f, Bottom - 14.0f, 9.0f, SolidArcInk(0x828282), Displayed == 2 ? "A picture on the top, front or right plane to trace over (not in the history)"
                                                                                   : Displayed == 1 ? "Click a tool, then draw it in the viewport · Enter / right-click finishes"
                                                                                       : "Click a solid, then draw it in the viewport · Tab types exact sizes");
    ImGui::EndPopup();
}

// Outliner eye: a body leaves view and picking (Plasticity H); a sketch / curve also leaves snapping, and the focus.
void ToggleFigureVisibility(SolidArcWorkspaceState& State, const SolidArcFigure& Figure)
{
    const bool BodyCondition = Figure.Source == SolidArcFigureSource::Body;
    const SolidArcCommandOutcome Outcome = BodyCondition ? ToggleSolidArcBodyVisibility(State.Document, Figure.Key) : ToggleSolidArcCurveVisibility(State.Document, Figure.Key);
    if (!Outcome.AcceptedCondition) { Announce(State, "Refused: " + Outcome.Refusal); return; }
    const bool Hidden = BodyCondition ? State.Document.Appearance.HiddenBodies.contains(Figure.Key) : SolidArcCurveHiddenCondition(State.Document, Figure.Key);
    if (BodyCondition) RefreshSolidArcScene(State, true);
    else
    {
        if (Hidden && State.FocusedStep == Figure.Key) State.FocusedStep.clear();
        MarkDirty(State);
    }
    Announce(State, Compose("%s %s", Figure.Name.c_str(), Hidden ? "hidden" : "shown"));
}

// Outliner lock: the figure stays in view but takes no pick, drag, gizmo or trim until unlocked.
void ToggleFigureLock(SolidArcWorkspaceState& State, const SolidArcFigure& Figure)
{
    const SolidArcCommandOutcome Outcome = ToggleSolidArcLock(State.Document, Figure.Key);
    if (!Outcome.AcceptedCondition) { Announce(State, "Refused: " + Outcome.Refusal); return; }
    const bool Locked = SolidArcLockedCondition(State.Document, Figure.Key);
    if (Locked && State.FocusedStep == Figure.Key) State.FocusedStep.clear();
    if (Locked && State.SketchPoints.Sketch == Figure.Key) State.SketchPoints = {};
    MarkDirty(State);
    Announce(State, Compose("%s %s", Figure.Name.c_str(), Locked ? "locked (shown, not pickable)" : "unlocked"));
}

// The Outliner panel (SolidArcOutliner) and what its rows ask for.
void DrawOutliner(SolidArcWorkspaceState& State, ImVec2 Position, ImVec2 Size)
{
    BeginSheetWindow("Outliner", Position, Size);
    const SolidArcOutlinerIntent Intent = ConstructSolidArcOutliner(State, Spanning(Position.x, Position.y, Size.x, Size.y));
    switch (Intent.Verb)
    {
    case SolidArcOutlinerVerb::Pick:
        if (Intent.Figure.LockedCondition)
        {
            Announce(State, Intent.Figure.Name + " is locked · its lock button frees it");
            break;
        }
        if (Intent.Figure.HiddenCondition && Intent.Figure.Source != SolidArcFigureSource::Body)
        {
            Announce(State, Intent.Figure.Name + " is hidden · its eye shows it");
            break;
        }
        switch (Intent.Figure.Source)
        {
        case SolidArcFigureSource::Body:
            SelectSolidArcName(State.Document, SolidArcSelectionKind::Body, Intent.Figure.Key, Intent.AdditiveCondition);
            State.PickMode = SolidArcPickMode::Solid;
            Announce(State, Compose("%s · %d bodies picked", Intent.Figure.Name.c_str(), int(State.Document.Selection.Bodies.size())));
            break;
        case SolidArcFigureSource::Sketch:
            // Click focuses and picks the sketch (gizmo, loft); Shift+click adds it in loft order; a second click lets go.
            // Only a focused sketch that is still picked lets go: after a loft (which clears the picks) the focus stays,
            // and the next click must pick it again, not drop it (the loft that seemed to ignore its first section).
            if (Intent.AdditiveCondition) PickCurve(State, Intent.Figure.Key, SolidArcPickIntent::Add);
            else if (State.FocusedStep == Intent.Figure.Key && CurveOrder(State, Intent.Figure.Key) > 0)
            {
                State.FocusedStep = {};
                SelectSolidArcCurve(State.Document, Intent.Figure.Key, true);
            }
            else PickCurve(State, Intent.Figure.Key, SolidArcPickIntent::Replace);
            break;
        case SolidArcFigureSource::SpaceCurve:
            PickCurve(State, Intent.Figure.Key, Intent.AdditiveCondition ? SolidArcPickIntent::Add : SolidArcPickIntent::Replace);
            break;
        }
        break;
    case SolidArcOutlinerVerb::ToggleVisibility:
        ToggleFigureVisibility(State, Intent.Figure);
        break;
    case SolidArcOutlinerVerb::ToggleLock:
        ToggleFigureLock(State, Intent.Figure);
        break;
    case SolidArcOutlinerVerb::Construct:
        State.ConstructMenuRequest = true;
        break;
    case SolidArcOutlinerVerb::None:
        break;
    }
    if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_F, false) && !ImGui::GetIO().WantTextInput) State.Outliner.SearchFocusRequest = true;
    DrawConstructDialog(State);
    EndSheetWindow();
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    VIEWPORT
//------------------------------------------------------------------------------------------------------------------------

struct CubeFace
{
    std::array<double, 3> Normal = {};                                                       // [-] - Outward world normal
    OrbitViewPreset    Preset = OrbitViewPreset::Top;                                  // [-] - View it presents
    const char*           Label  = "";                                                       // [-] - Face caption
};

// 💡 Orientation cube: the six faces projected with the camera basis, back faces culled, painter-sorted; a click
//    presents that view. Returns true when the pointer is over the cube (viewport gestures are then ignored).
// 📝 The cube follows the viewport projection: parallel draws it orthographically; perspective puts an eye at E on −Forward
//    with the viewport's own field of view (E = 1.6 / tan(fov / 2), the cube filling that cone) and divides by E + depth,
//    the near corners spreading and the far ones drawing in. A face is seen when the eye is outside its plane (n·eye > 1).
bool DrawOrientationCube(SolidArcWorkspaceState& State, ImDrawList* Canvas, ImVec2 Corner, bool ClickCondition)
{
    static const CubeFace Faces[] = {
        { { 0, 0, 1 }, OrbitViewPreset::Top, "Top" },     { { 0, 0, -1 }, OrbitViewPreset::Bottom, "Bottom" },
        { { 0, -1, 0 }, OrbitViewPreset::Front, "Front" }, { { 0, 1, 0 }, OrbitViewPreset::Back, "Back" },
        { { 1, 0, 0 }, OrbitViewPreset::Right, "Right" },  { { -1, 0, 0 }, OrbitViewPreset::Left, "Left" },
    };
    const OrbitCameraFrame Frame  = ViewportFrame(State);
    const ImVec2              Centre(Corner.x + CubeExtent * 0.5f, Corner.y + CubeExtent * 0.5f);
    const bool                PerspectiveCondition = !Frame.OrthographicEnabled;
    const float               Scale  = CubeExtent * (PerspectiveCondition ? 0.27f : 0.29f);
    // 📝 A face shows while EyeDistance · facing > 1: at 6 a face 10° off edge-on still shows (a low orbit keeps Top
    //    clickable), where the old 3.5 hid everything under 17°.
    const double              EyeDistance = std::max(6.0, 2.4 / std::tan(0.5 * std::clamp(State.Camera.FieldOfView, 0.1, 2.5))); // [cube] - Eye to cube centre
    auto Project = [&](const std::array<double, 3>& Point)
    {
        const double Across = Point[0] * Frame.Right[0] + Point[1] * Frame.Right[1] + Point[2] * Frame.Right[2];
        const double Upward = Point[0] * Frame.Up[0] + Point[1] * Frame.Up[1] + Point[2] * Frame.Up[2];
        const double Depth  = Point[0] * Frame.Forward[0] + Point[1] * Frame.Forward[1] + Point[2] * Frame.Forward[2];
        const double Spread = PerspectiveCondition ? EyeDistance / std::max(0.25, EyeDistance + Depth) : 1.0;
        return ImVec2(Centre.x + float(Across * Spread) * Scale, Centre.y - float(Upward * Spread) * Scale);
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
        if ((PerspectiveCondition ? EyeDistance * Facing - 1.0 : Facing) <= 1.0e-3) continue;
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
        State.Widgets.Record(Key, ImVec2(Point.x - 2, Point.y - 2), ImVec2(Point.x + 2, Point.y + 2));
    }
    if (Hovered && ClickCondition)
    {
        PresentView(State, Hovered->Preset);
        Announce(State, std::string("View ") + Hovered->Label + " (orientation cube)");
    }
    return Inside;
}

// 📝 HTML #gizmo-readout look for the session readouts: #171717e8, #ffffff18 hairline, radius 12, padding 8 14, 11 px #eee;
//    a 7 px accent dot leads. Cursor advances past the pill (gap 6).
void DrawStatusPill(ImDrawList* Canvas, ImVec2& Cursor, const std::string& Text, ImU32 Accent)
{
    (void)Canvas;
    SolidArcSheet Sheet;
    if (!Sheet.Begin()) return;
    const ImVec4      Dot    = ImGui::ColorConvertU32ToFloat4(Accent);
    const float       Height = 2.0f + 16.0f + 11.0f * SolidArcStyleTokens::FontScale;
    const PlaneExtent Pill   = Spanning(Cursor.x, Cursor.y, 2.0f + 28.0f + 7.0f + 8.0f + Sheet.Measure(Text.c_str(), 11.0f), Height);
    const float       Centre = Cursor.y + 0.5f * Height;
    Sheet.Box(Pill, SolidArcInk(0x171717, 0xe8 / 255.0f), 12.0f, SolidArcWash(0x18));
    Sheet.Surface().FillRectangle(Spanning(Pill.MinimumX + 15.0f, Centre - 3.5f, 7.0f, 7.0f), ColorQuad{ Dot.x, Dot.y, Dot.z, Dot.w }, 3.5f);
    Sheet.Text(Pill.MinimumX + 15.0f + 7.0f + 8.0f, Centre, 11.0f, SolidArcInk(0xeeeeee), Text.c_str());
    Cursor.x = Pill.MaximumX + 6.0f;
}

std::vector<std::string> TouchedBodies(const SolidArcDocument& Document);

// HTML #camera-view: the preset the camera sits on, else "Orbit" (the select's disabled first option).
const char* CurrentViewLabel(const OrbitCamera& Camera)
{
    constexpr double Tolerance = 1.0e-2;                                                     // [rad] - Same view
    for (OrbitViewPreset Preset : { OrbitViewPreset::Isometric, OrbitViewPreset::Front, OrbitViewPreset::Right, OrbitViewPreset::Top, OrbitViewPreset::Back,
                                    OrbitViewPreset::Left, OrbitViewPreset::Bottom })
    {
        OrbitCamera Probe = Camera;
        PresentOrbitView(Probe, Preset);
        const bool   Polar = std::abs(Probe.Elevation) > 1.5;                                // Top / Bottom: any azimuth
        const double Turn  = std::remainder(Camera.Azimuth - Probe.Azimuth, 2.0 * std::numbers::pi);
        if (std::abs(Camera.Elevation - Probe.Elevation) < Tolerance && (Polar || std::abs(Turn) < Tolerance)) return OrbitViewLabel(Preset);
    }
    return "Orbit";
}

// The ⌄ of a select, as two strokes (DM Sans has no chevron glyph).
void SelectChevron(SolidArcSheet& Sheet, float CentreX, float CentreY, ColorQuad Ink)
{
    Sheet.Line(CentreX - 3.5f, CentreY - 1.75f, CentreX, CentreY + 1.75f, Ink, 1.3f);
    Sheet.Line(CentreX, CentreY + 1.75f, CentreX + 3.5f, CentreY - 1.75f, Ink, 1.3f);
}

// 📝 Popup list under a select (#121417, #ffffff22 hairline, radius 12): one Selectable per entry, keyed for the scripts.
bool BeginSelectList(const char* Name, ImVec2 Anchor)
{
    ImGui::SetNextWindowPos(Anchor);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6, 6));
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 12.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 2));
    ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(0.0f, 0.5f));
    ImGui::PushStyleColor(ImGuiCol_PopupBg, Tint(IM_COL32(0x12, 0x14, 0x17, 0xff)));
    ImGui::PushStyleColor(ImGuiCol_Border, Tint(IM_COL32(0xff, 0xff, 0xff, 0x22)));
    ImGui::PushStyleColor(ImGuiCol_Header, Tint(IM_COL32(0xff, 0xff, 0xff, 0x15)));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, Tint(IM_COL32(0x29, 0x29, 0x29, 0xff)));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, Tint(IM_COL32(0x30, 0x30, 0x30, 0xff)));
    const bool Shown = ImGui::BeginPopup(Name, ImGuiWindowFlags_NoMove);
    if (!Shown)
    {
        ImGui::PopStyleColor(5);
        ImGui::PopStyleVar(4);
        return false;
    }
    ImGui::PushFont(static_cast<ImFont*>(QuerySolidArcTypefaces().Light), 11.0f * SolidArcStyleTokens::FontScale);
    return true;
}

// 📝 The entry colours stay pushed while the list is open (Selectable reads Header* when it draws).
void EndSelectList()
{
    ImGui::PopFont();
    ImGui::EndPopup();
    ImGui::PopStyleColor(5);
    ImGui::PopStyleVar(4);
}

bool SelectListEntry(SolidArcWorkspaceState& State, const std::string& Label, bool SelectedCondition)
{
    const bool Pressed = ImGui::Selectable(Label.c_str(), SelectedCondition, ImGuiSelectableFlags_None, ImVec2(150.0f, 26.0f));
    RegisterWidget(State, Label.c_str());
    return Pressed;
}

// 📝 HTML .viewport-bar (44 px, padding 0 12, gap 4): the Construct chip, a divider, the camera-view select … the
//    Perspective text button and the edges / fit / maximize icon buttons.
void DrawViewportBar(SolidArcWorkspaceState& State, SolidArcSheet& Sheet, const PlaneExtent& Bar)
{
    const float Centre = 0.5f * (Bar.MinimumY + Bar.MaximumY);
    float       Cursor = Bar.MinimumX + 12.0f;

    // .construct-chip — #202020, #ffffff12 hairline, radius 20, padding 7 13, gap 9; plus glyph 15, 11 px #e0e0e0.
    const float         ChipHeight = 2.0f + 14.0f + 11.0f * SolidArcStyleTokens::FontScale;
    const PlaneExtent   Chip       = Spanning(Cursor, Centre - 0.5f * ChipHeight, 2.0f + 26.0f + 15.0f + 9.0f + Sheet.Measure("Construct", 11.0f), ChipHeight);
    const SolidArcPress Construct  = PressSolidArcRegion(State.Widgets, "ViewportConstruct", Chip);
    const ColorQuad     ChipInk    = SolidArcInk(Construct.HoveredCondition ? 0xffffff : 0xe0e0e0);
    Sheet.Box(Chip, SolidArcInk(Construct.HoveredCondition ? 0x292929 : 0x202020), 20.0f, SolidArcWash(0x12));
    Sheet.Icon(IconSymbol::SolidArcPlus, Chip.MinimumX + 14.0f, Centre - 7.5f, 15.0f, ChipInk);
    Sheet.Text(Chip.MinimumX + 14.0f + 15.0f + 9.0f, Centre, 11.0f, ChipInk, "Construct");
    if (Construct.ClickedCondition) State.ConstructMenuRequest = true;
    Cursor = Chip.MaximumX + 4.0f;

    // .bar-divider — 1 × 15, #ffffff10, margin 0 9.
    Sheet.Surface().FillRectangle(Spanning(Cursor + 9.0f, Centre - 7.5f, 1.0f, 15.0f), SolidArcWash(0x10));
    Cursor += 9.0f + 1.0f + 9.0f + 4.0f;

    // #camera-view — 98 px select, 11 px #a0a0a0, padding 5; lists the seven presets.
    const PlaneExtent   Select = Spanning(Cursor, Centre - 13.0f, 98.0f, 26.0f);
    const SolidArcPress View   = PressSolidArcRegion(State.Widgets, "ViewMenu", Select);
    if (View.HoveredCondition) Sheet.Box(Select, SolidArcInk(0x292929), 13.0f);
    const ColorQuad ViewInk = SolidArcInk(View.HoveredCondition ? 0xffffff : 0xa0a0a0);
    Sheet.Text(Select.MinimumX + 5.0f, Centre, 11.0f, ViewInk, CurrentViewLabel(State.Camera));
    SelectChevron(Sheet, Select.MaximumX - 12.0f, Centre, ViewInk);
    if (View.ClickedCondition) ImGui::OpenPopup("ViewMenu");
    if (BeginSelectList("ViewMenu", ImVec2(Select.MinimumX, Select.MaximumY + 4.0f)))
    {
        const char* Current = CurrentViewLabel(State.Camera);
        for (OrbitViewPreset Preset : { OrbitViewPreset::Isometric, OrbitViewPreset::Front, OrbitViewPreset::Right, OrbitViewPreset::Top, OrbitViewPreset::Back,
                                        OrbitViewPreset::Left, OrbitViewPreset::Bottom })
        {
            const std::string Label = std::string(OrbitViewLabel(Preset)) + "###View" + OrbitViewLabel(Preset);
            if (SelectListEntry(State, Label, std::strcmp(Current, OrbitViewLabel(Preset)) == 0))
            {
                PresentView(State, Preset);
                Announce(State, std::string("View ") + OrbitViewLabel(Preset));
            }
        }
        EndSelectList();
    }

    // Right end, right to left: maximize, fit, edges (28 px icon buttons, gap 4), then the projection text button.
    float Right = Bar.MaximumX - 12.0f;
    if (SheetIconButton(State, Sheet, "ViewportMaximize", Right - 14.0f, Centre, 28.0f, IconSymbol::SolidArcMaximize, 18.0f, 0, true, State.ViewportMaximizedEnabled))
        State.ViewportMaximizedEnabled = !State.ViewportMaximizedEnabled;
    Right -= 28.0f + 4.0f;
    if (SheetIconButton(State, Sheet, "FitView", Right - 14.0f, Centre, 28.0f, IconSymbol::SolidArcFocus, 18.0f) && !State.Scene.Faces.empty())
        FitSolidArcCamera(State.Camera, State.Scene);
    Right -= 28.0f + 4.0f;
    if (SheetIconButton(State, Sheet, "EdgeToggle", Right - 14.0f, Centre, 28.0f, IconSymbol::SolidArcBody, 18.0f, 0, true, State.Style.EdgesEnabled))
        State.Style.EdgesEnabled = !State.Style.EdgesEnabled;
    Right -= 28.0f + 4.0f;
    if (SheetTextButton(State, Sheet, "Projection", Right, Centre, State.Camera.OrthographicEnabled ? "Parallel" : "Perspective"))
        State.Camera.OrthographicEnabled = !State.Camera.OrthographicEnabled;
}

// 📝 HTML .viewport-label (top 21, left 20, gap 9): the NURBS / B-REP pill (8 px, 1.2 spacing, #161a21ae, radius 10),
//    the selection caption (10 px #afb0b3) and the active mode pills; below, the 9 px #8a8f98 kernel line.
void DrawViewportLabel(SolidArcWorkspaceState& State, SolidArcSheet& Sheet, ImVec2 Image, bool HoveredCondition)
{
    const float PillHeight = 2.0f + 10.0f + 8.0f * SolidArcStyleTokens::FontScale;
    float       Cursor     = Image.x + 20.0f;
    const float Top        = Image.y + 21.0f;
    const float Centre     = Top + 0.5f * PillHeight;
    const PlaneExtent Pill = Spanning(Cursor, Top, 2.0f + 16.0f + Sheet.Measure("NURBS / B-REP", 8.0f, SolidArcWeight::Light, 1.2f), PillHeight);
    Sheet.Box(Pill, SolidArcInk(0x161a21, 0xae / 255.0f), 10.0f, SolidArcWash(0x10));
    Sheet.Text(Pill.MinimumX + 9.0f, Centre, 8.0f, SolidArcInk(0xdddddd), "NURBS / B-REP", SolidArcWeight::Light, 1.2f);
    Cursor = Pill.MaximumX + 9.0f;

    const SolidArcSelection&       Selection = State.Document.Selection;
    const std::vector<std::string> Bodies    = TouchedBodies(State.Document);
    const int Picked = int(Bodies.size() + Selection.Curves.size());
    std::string Caption = State.Document.Title;
    if (!Selection.Faces.empty() || !Selection.Edges.empty())
        Caption = Compose("%d face%s · %d edge%s · ", int(Selection.Faces.size()), Selection.Faces.size() == 1 ? "" : "s", int(Selection.Edges.size()),
                          Selection.Edges.size() == 1 ? "" : "s") + (Bodies.empty() ? Caption : SolidArcBodyCaption(State.Document, Bodies.front()));
    else if (Picked > 0)
        Caption = Compose("%d selected · ", Picked) + (Bodies.empty() ? CaptionOf(State.Document, Selection.Curves.front()) : SolidArcBodyCaption(State.Document, Bodies.front()));
    Cursor += Sheet.Text(Cursor, Centre, 10.0f, SolidArcInk(0xafb0b3), Caption.c_str()) + 9.0f;

    // Modes the viewport is in (X-ray, Measure, Section) and the hovered figure, as accent pills on the same row.
    const auto ModePill = [&](const std::string& Text, uint32_t Accent)
    {
        const PlaneExtent Box = Spanning(Cursor, Top, 2.0f + 16.0f + 5.0f + 7.0f + Sheet.Measure(Text.c_str(), 8.0f, SolidArcWeight::Light, 1.2f), PillHeight);
        Sheet.Box(Box, SolidArcInk(0x161a21, 0xae / 255.0f), 10.0f, SolidArcWash(0x10));
        Sheet.Surface().FillRectangle(Spanning(Box.MinimumX + 8.0f, Centre - 2.5f, 5.0f, 5.0f), SolidArcInk(Accent), 2.5f);
        Sheet.Text(Box.MinimumX + 8.0f + 5.0f + 7.0f, Centre, 8.0f, SolidArcInk(0xdddddd), Text.c_str(), SolidArcWeight::Light, 1.2f);
        Cursor = Box.MaximumX + 9.0f;
    };
    if (State.Style.XRayEnabled) ModePill("X-RAY · ALT+Z", 0xc7d4e8);
    if (State.Measure.ActiveCondition) ModePill("MEASURE", 0xffc83d);
    if (State.Section.ActiveCondition) ModePill(DescribeSectionRequest(State.Section.Request), 0xdb614a);
    if (State.HoverIdentity != 0 && HoveredCondition)
        ModePill(State.HoverIdentity == IdentitySectionCap ? std::string("Section cap") : NameSolidArcIdentity(DisplayedSolidArcScene(State), State.HoverIdentity), 0xffd24d);

    // Second row: the kernel, the drawing plane, the audit and the curve count.
    static const char* const PlaneNames[] = { "Plane XY", "Plane XZ", "Plane YZ" };
    std::string Line = std::string("Native C++ · Vulkan viewport · ") + PlaneNames[std::clamp(State.Style.GridPlane, 0, 2)];
    const SolidArcBodySummary& Summary = State.Document.Summary;
    if (Summary.PresentCondition)
        Line += Summary.SheetCondition        ? std::string(Summary.SoundCondition ? " · Sound sheet" : " · Sheet audit findings")
              : Summary.Bodies.size() > 1     ? Compose(" · %d bodies · %s", int(Summary.Bodies.size()), Summary.SoundCondition ? "sound" : "audit findings")
                                              : Compose(" · %s · genus %d", Summary.SoundCondition ? "Sound solid" : "Audit findings", Summary.Genus);
    if (!State.Document.Regeneration.Curves.empty()) Line += Compose(" · %d curve(s)", int(State.Document.Regeneration.Curves.size()));
    Sheet.Text(Image.x + 20.0f, Top + PillHeight + 9.0f + 1.0f + 0.5f * 9.0f * SolidArcStyleTokens::FontScale, 9.0f, SolidArcInk(0x8a8f98), Line.c_str());
}

// 📝 HTML .viewport-instructions (bottom 12, left 17, 9 px, gap 12, wrapping): the first group #8f9297, the rest #747b84.
//    The native hint's " · " groups wrap upwards inside Width.
void DrawViewportInstructions(SolidArcSheet& Sheet, float Left, float Bottom, float Width, const std::string& Hint)
{
    std::vector<std::string> Groups;
    for (size_t Start = 0; Start <= Hint.size();)
    {
        const size_t Split = Hint.find(" · ", Start);
        Groups.push_back(Hint.substr(Start, Split == std::string::npos ? std::string::npos : Split - Start));
        if (Split == std::string::npos) break;
        Start = Split + std::strlen(" · ");
    }
    std::vector<std::vector<size_t>> Lines(1);
    float Run = 0.0f;
    for (size_t Index = 0; Index < Groups.size(); ++Index)
    {
        const float Extent = Sheet.Measure(Groups[Index].c_str(), 9.0f);
        if (!Lines.back().empty() && Run + 12.0f + Extent > Width)
        {
            Lines.emplace_back();
            Run = 0.0f;
        }
        Run += (Lines.back().empty() ? 0.0f : 12.0f) + Extent;
        Lines.back().push_back(Index);
    }
    const float LineHeight = 9.0f * SolidArcStyleTokens::FontScale + 3.0f;
    for (size_t Row = 0; Row < Lines.size(); ++Row)
    {
        const float Centre = Bottom - float(Lines.size() - 1 - Row) * LineHeight - 0.5f * 9.0f * SolidArcStyleTokens::FontScale;
        float       Cursor = Left;
        for (const size_t Index : Lines[Row])
            Cursor += Sheet.Text(Cursor, Centre, 9.0f, SolidArcInk(Index == 0 ? 0x8f9297 : 0x747b84), Groups[Index].c_str()) + 12.0f;
    }
}

// 📝 HTML .selection-tools (top 98, left 17): #15191edd column, #ffffff0a hairline, radius 22, padding 7 5, gap 5;
//    30 px round buttons (#a7abb2, active #ffffff15 / #eee, glyph 16). Native order: modes 4 · 3 · 2 · 1 · 5, a rule,
//    then X-ray (Alt+Z). Its own window, so the viewport surface underneath does not take the clicks.
void DrawSelectionRail(SolidArcWorkspaceState& State, ImVec2 Image)
{
    struct RailButton
    {
        const char*      Key;
        IconSymbol       Glyph;
        SolidArcPickMode Mode;
    };
    static const RailButton Buttons[] = { { "Mode4", IconSymbol::SolidArcBody, SolidArcPickMode::Solid }, { "Mode3", IconSymbol::SolidArcSurface, SolidArcPickMode::Face },
                                          { "Mode2", IconSymbol::SolidArcEdge, SolidArcPickMode::Edge },  { "Mode1", IconSymbol::SolidArcPoints, SolidArcPickMode::ControlPoint } };   // 5 (any) stays on the keyboard, as in the HTML rail
    constexpr float Button = 30.0f, Gap = 5.0f, Rule = 9.0f;
    const float Height = 2.0f + 14.0f + float(std::size(Buttons) + 1) * Button + float(std::size(Buttons) + 1) * Gap + Rule;
    const ImVec2 Corner(Image.x + 17.0f, Image.y + 98.0f);
    BeginSheetWindow("ViewportRail", Corner, ImVec2(2.0f + 10.0f + Button, Height), true);
    SolidArcSheet Sheet;
    Sheet.Begin();
    const PlaneExtent Rail = Spanning(Corner.x, Corner.y, 2.0f + 10.0f + Button, Height);
    Sheet.Box(Rail, SolidArcInk(0x15191e, 0xdd / 255.0f), 22.0f, SolidArcWash(0x0a));
    const float Middle = 0.5f * (Rail.MinimumX + Rail.MaximumX);
    float       Y      = Rail.MinimumY + 1.0f + 7.0f;
    const auto Round = [&](const char* Key, IconSymbol Glyph, bool Active, const char* Tip)
    {
        const PlaneExtent   Box   = Spanning(Middle - 0.5f * Button, Y, Button, Button);
        const SolidArcPress Press = PressSolidArcRegion(State.Widgets, Key, Box);
        if (Press.HoveredCondition) ImGui::SetTooltip("%s", Tip);
        if (Active || Press.HoveredCondition) Sheet.Box(Box, Active ? SolidArcWash(0x15) : SolidArcInk(0x292929), 0.5f * Button);
        Sheet.Icon(Glyph, Middle - 8.0f, Y + 0.5f * Button - 8.0f, 16.0f, SolidArcInk(Active || Press.HoveredCondition ? 0xeeeeee : 0xa7abb2));
        Y += Button + Gap;
        return Press.ClickedCondition;
    };
    static const char* const Tips[] = { "Solid mode (4): pick bodies", "Face mode (3): pick faces", "Edge mode (2): pick edges", "Point mode (1): curves, poles and sketch points" };
    for (size_t Index = 0; Index < std::size(Buttons); ++Index)
        if (Round(Buttons[Index].Key, Buttons[Index].Glyph, State.PickMode == Buttons[Index].Mode, Tips[Index])) State.PickMode = Buttons[Index].Mode;
    Sheet.Line(Middle - 9.0f, Y + 0.5f * Rule - 0.5f * Gap, Middle + 9.0f, Y + 0.5f * Rule - 0.5f * Gap, SolidArcWash(0x14));
    Y += Rule;
    if (Round("XRayToggle", IconSymbol::SolidArcLayers, State.Style.XRayEnabled, State.Style.XRayEnabled ? "X-ray on (Alt+Z): bodies are see-through · click for solid shading"
                                                                                                           : "X-ray (Alt+Z): see through bodies · off by default"))
        ToggleXRay(State);
    EndSheetWindow();
}

// 📝 HTML .gizmo-toolbar slot (top 18, right 15): #191919e8 pill, #ffffff12 hairline, padding 4 5 4 12, gap 8; a 10 px
//    #999 label and an 11 px select. Native: the display overlays (grid, combs, zebra, deviation, reference images).
void DrawDisplayMenu(SolidArcWorkspaceState& State, ImVec2 Image, float Width)
{
    struct DisplayToggle
    {
        const char* Label;
        bool*       Enabled;
    };
    const DisplayToggle Toggles[] = { { "Grid###GridToggle", &State.Style.GridEnabled }, { "Combs###CombToggle", &State.CombsEnabled },
                                      { "Zebra###ZebraToggle", &State.ZebraEnabled },    { "Deviation###DeviationToggle", &State.DeviationEnabled },
                                      { "Images###ImagesToggle", &State.ImagePlanesEnabled } };
    std::string Summary;
    for (const DisplayToggle& Toggle : Toggles)
        if (*Toggle.Enabled) Summary += (Summary.empty() ? "" : " · ") + std::string(Toggle.Label, std::strstr(Toggle.Label, "###"));
    if (Summary.empty()) Summary = "Off";

    SolidArcSheet Probe;
    Probe.Begin();
    const float  Height = 2.0f + 8.0f + 26.0f;
    const float  Extent = 2.0f + 12.0f + Probe.Measure("Display", 10.0f) + 8.0f + 7.0f + Probe.Measure(Summary.c_str(), 11.0f) + 22.0f + 5.0f;
    const ImVec2 Corner(Image.x + Width - 15.0f - Extent, Image.y + 18.0f);
    BeginSheetWindow("ViewportDisplay", Corner, ImVec2(Extent, Height), true);
    SolidArcSheet Sheet;
    Sheet.Begin();
    const PlaneExtent Pill   = Spanning(Corner.x, Corner.y, Extent, Height);
    const float       Centre = 0.5f * (Pill.MinimumY + Pill.MaximumY);
    Sheet.Box(Pill, SolidArcInk(0x191919, 0xe8 / 255.0f), 18.0f, SolidArcWash(0x12));
    const float Label = Sheet.Text(Pill.MinimumX + 1.0f + 12.0f, Centre, 10.0f, SolidArcInk(0x999999), "Display");
    const PlaneExtent   Select = PlaneExtent{ Pill.MinimumX + 1.0f + 12.0f + Label + 8.0f, Centre - 13.0f, Pill.MaximumX - 1.0f - 5.0f, Centre + 13.0f };
    const SolidArcPress Press  = PressSolidArcRegion(State.Widgets, "DisplayMenu", Select);
    if (Press.HoveredCondition) Sheet.Box(Select, SolidArcInk(0x292929), 13.0f);
    const ColorQuad Ink = SolidArcInk(Press.HoveredCondition ? 0xffffff : 0xdddddd);
    Sheet.Text(Select.MinimumX + 7.0f, Centre, 11.0f, Ink, Summary.c_str());
    SelectChevron(Sheet, Select.MaximumX - 11.0f, Centre, Ink);
    if (Press.ClickedCondition) ImGui::OpenPopup("DisplayMenu");
    if (BeginSelectList("DisplayMenu", ImVec2(Pill.MaximumX - 162.0f, Pill.MaximumY + 6.0f)))
    {
        for (const DisplayToggle& Toggle : Toggles)
        {
            if (!SelectListEntry(State, Toggle.Label, *Toggle.Enabled)) continue;
            if (Toggle.Enabled == &State.ZebraEnabled) ToggleZebra(State);
            else *Toggle.Enabled = !*Toggle.Enabled;
        }
        EndSelectList();
    }
    EndSheetWindow();
}

// 📝 Gizmo chips (under the Display pill, same #191919e8 pill): Move / Rotate / Scale toggle independently, so any mix
//    (move + rotate, rotate + scale, all three) is one click away; y cycles the same set from the keyboard.
void DrawGizmoChips(SolidArcWorkspaceState& State, ImVec2 Image, float Width)
{
    struct GizmoChip
    {
        const char* Key;
        const char* Label;
        uint32_t    Mode;
    };
    static const GizmoChip Chips[] = { { "GizmoMove", "Move", GizmoTranslate }, { "GizmoRotate", "Rotate", GizmoRotate }, { "GizmoScale", "Scale", GizmoScale } };
    SolidArcSheet Probe;
    Probe.Begin();
    float Extent = 2.0f + 12.0f + Probe.Measure("Gizmo", 10.0f) + 8.0f + 5.0f;
    for (const GizmoChip& Chip : Chips) Extent += Probe.Measure(Chip.Label, 11.0f) + 16.0f + 3.0f;
    const float  Height = 2.0f + 8.0f + 26.0f;
    const ImVec2 Corner(Image.x + Width - 15.0f - Extent, Image.y + 18.0f + Height + 8.0f);
    BeginSheetWindow("ViewportGizmo", Corner, ImVec2(Extent, Height), true);
    SolidArcSheet Sheet;
    Sheet.Begin();
    const PlaneExtent Pill   = Spanning(Corner.x, Corner.y, Extent, Height);
    const float       Centre = 0.5f * (Pill.MinimumY + Pill.MaximumY);
    Sheet.Box(Pill, SolidArcInk(0x191919, 0xe8 / 255.0f), 18.0f, SolidArcWash(0x12));
    float X = Pill.MinimumX + 1.0f + 12.0f;
    X += Sheet.Text(X, Centre, 10.0f, SolidArcInk(0x999999), "Gizmo") + 8.0f;
    for (const GizmoChip& Chip : Chips)
    {
        const float         Span   = Probe.Measure(Chip.Label, 11.0f) + 16.0f;
        const PlaneExtent   Box    = PlaneExtent{ X, Centre - 13.0f, X + Span, Centre + 13.0f };
        const SolidArcPress Press  = PressSolidArcRegion(State.Widgets, Chip.Key, Box);
        const bool          Active = (State.Gizmo.Modes & Chip.Mode) != 0;
        if (Active || Press.HoveredCondition) Sheet.Box(Box, Active ? SolidArcWash(0x15) : SolidArcInk(0x292929), 13.0f);
        Sheet.Text(X + 8.0f, Centre, 11.0f, SolidArcInk(Active ? 0xffffff : Press.HoveredCondition ? 0xdddddd : 0x8a8f96), Chip.Label);
        if (Press.ClickedCondition)
        {
            State.Gizmo.Modes ^= Chip.Mode;
            Announce(State, "Gizmo: " + GizmoModesLabel(State.Gizmo.Modes));
        }
        X += Span + 3.0f;
    }
    EndSheetWindow();
}

// 📝 HTML .sketch-toolbar (bottom 52, centred): #171717f5, #343434 hairline, radius 13, padding 5, gap 4; buttons
//    padding 9 12, radius 8, gap 7, glyph 17, #aaa (active / hover #303030 / #fff), a 1 × 22 #3a3a3a divider after Select.
//    Select ends Trim, Cut splits the body by the focused sketch / picked curve (C), Trim opens the trim session (T).
void DrawSketchToolbar(SolidArcWorkspaceState& State, ImVec2 Image, ImVec2 Extent)
{
    const bool SketchCondition = std::any_of(State.Document.Tree.Steps.begin(), State.Document.Tree.Steps.end(), [](const FeatureStep& Step) { return Step.Kind == FeatureStepKind::Sketch; });
    const bool CutCondition    = FocusedSketchCondition(State) || !State.Document.Selection.Curves.empty();
    struct ToolButton
    {
        const char* Key;
        IconSymbol  Glyph;
        const char* Label;
        bool        Active;
        bool        Enabled;
    };
    const ToolButton Tools[] = { { "SketchSelect", IconSymbol::SolidArcSelect, "Select", !State.Trim.ActiveCondition && !State.Fill.ActiveCondition, true },
                                 { "SketchCut", IconSymbol::SolidArcCut, "Cut", false, CutCondition && !State.Bevel.ActiveCondition },
                                 { "SketchTrim", IconSymbol::SolidArcTrim, "Trim", State.Trim.ActiveCondition, SketchCondition },
                                 { "SketchFill", IconSymbol::SolidArcFill, "Fill", State.Fill.ActiveCondition, SketchCondition },
                                 { "SketchKnife", IconSymbol::SolidArcSubtract, "Knife", State.Knife.ActiveCondition, State.Document.Summary.PresentCondition && !State.Bevel.ActiveCondition } };
    SolidArcSheet Probe;
    Probe.Begin();
    float Widths[5], Total = 2.0f + 10.0f + 4.0f * 4.0f + 1.0f + 6.0f + 4.0f;
    for (int Index = 0; Index < 5; ++Index) Total += Widths[Index] = 24.0f + 17.0f + 7.0f + Probe.Measure(Tools[Index].Label, 13.0f);
    const float  ButtonHeight = 18.0f + std::max(17.0f, 13.0f * SolidArcStyleTokens::FontScale);
    const float  Height       = 2.0f + 10.0f + ButtonHeight;
    const ImVec2 Corner(std::floor(Image.x + 0.5f * (Extent.x - Total)), Image.y + Extent.y - 52.0f - Height);
    BeginSheetWindow("ViewportTools", Corner, ImVec2(Total, Height), true);
    SolidArcSheet Sheet;
    Sheet.Begin();
    const PlaneExtent Bar    = Spanning(Corner.x, Corner.y, Total, Height);
    const float       Centre = 0.5f * (Bar.MinimumY + Bar.MaximumY);
    Sheet.Box(Bar, SolidArcInk(0x171717, 0xf5 / 255.0f), 13.0f, SolidArcInk(0x343434));
    float Cursor = Bar.MinimumX + 1.0f + 5.0f;
    for (int Index = 0; Index < 5; ++Index)
    {
        const ToolButton&   Tool    = Tools[Index];
        const PlaneExtent   Box     = Spanning(Cursor, Centre - 0.5f * ButtonHeight, Widths[Index], ButtonHeight);
        const SolidArcPress Press   = PressSolidArcRegion(State.Widgets, Tool.Key, Box, Tool.Enabled);
        const float         Opacity = Tool.Enabled ? 1.0f : 0.32f;
        const bool          Lit     = Tool.Active || Press.HoveredCondition;
        if (Lit) Sheet.Box(Box, SolidArcInk(0x303030), 8.0f);
        const ColorQuad Ink = ControlKit::Faded(SolidArcInk(Lit ? 0xffffff : 0xaaaaaa), Opacity);
        Sheet.Icon(Tool.Glyph, Box.MinimumX + 12.0f, Centre - 8.5f, 17.0f, Ink);
        Sheet.Text(Box.MinimumX + 12.0f + 17.0f + 7.0f, Centre, 13.0f, Ink, Tool.Label);
        if (Press.ClickedCondition)
        {
            if (Index == 0 && State.Trim.ActiveCondition) CloseTrimSession(State);
            if (Index == 0 && State.Fill.ActiveCondition) CloseFillSession(State);
            if (Index == 1) ApplyCut(State);
            if (Index == 2) State.Trim.ActiveCondition ? CloseTrimSession(State) : OpenTrimSession(State);
            if (Index == 3) State.Fill.ActiveCondition ? CloseFillSession(State) : OpenFillSession(State);
            if (Index == 4) State.Knife.ActiveCondition ? CloseKnifeSession(State, "Knife closed") : OpenKnifeSession(State);
        }
        Cursor = Box.MaximumX + 4.0f;
        if (Index == 0)
        {
            Sheet.Surface().FillRectangle(Spanning(Cursor + 3.0f, Centre - 11.0f, 1.0f, 22.0f), SolidArcInk(0x3a3a3a));
            Cursor += 3.0f + 1.0f + 3.0f + 4.0f;
        }
    }
    EndSheetWindow();
}

// WorkspacePanel.js history-step: the glyph and the one-word caption of a feature step.
std::pair<IconSymbol, const char*> StepGlyph(const FeatureStep& Step)
{
    switch (Step.Kind)
    {
    case FeatureStepKind::Sketch:        return { IconSymbol::SolidArcCurve, "sketch" };
    case FeatureStepKind::Extrude:       return { IconSymbol::SolidArcExtrude, "extrude" };
    case FeatureStepKind::Revolve:       return { IconSymbol::SolidArcRotate, "revolve" };
    case FeatureStepKind::Blend:         return Step.Shape == BlendProfile::Fillet ? std::pair{ IconSymbol::SolidArcFillet, "fillet" } : std::pair{ IconSymbol::SolidArcChamfer, "chamfer" };
    case FeatureStepKind::FaceOperation: return { IconSymbol::SolidArcMove, "face" };
    case FeatureStepKind::SpaceCurve:    return { IconSymbol::SolidArcNurbs, "curve" };
    case FeatureStepKind::Loft:          return { IconSymbol::SolidArcSurface, "loft" };
    case FeatureStepKind::Network:       return { IconSymbol::SolidArcCage, "network" };
    case FeatureStepKind::Mirror:        return { IconSymbol::SolidArcLayers, "mirror" };
    case FeatureStepKind::Close:         return { IconSymbol::SolidArcBody, "close" };
    case FeatureStepKind::Combine:
        return Step.Combination == FeatureCombination::Difference     ? std::pair{ IconSymbol::SolidArcSubtract, "subtract" }
             : Step.Combination == FeatureCombination::Intersection   ? std::pair{ IconSymbol::SolidArcIntersect, "intersect" }
                                                                      : std::pair{ IconSymbol::SolidArcUnion, "union" };
    case FeatureStepKind::Cut:           return { IconSymbol::SolidArcCut, "cut" };
    case FeatureStepKind::Remove:        return { IconSymbol::SolidArcTrash, "remove" };
    case FeatureStepKind::Transform:     return { IconSymbol::SolidArcMove, "move" };
    case FeatureStepKind::Duplicate:     return { IconSymbol::SolidArcBody, "copy" };
    case FeatureStepKind::Pipe:          return { IconSymbol::SolidArcSurface, "pipe" };
    case FeatureStepKind::Evolution:     return { IconSymbol::SolidArcExtrude, "sweep" };
    case FeatureStepKind::Joint:         return { IconSymbol::SolidArcLink, "joint" };
    case FeatureStepKind::Import:        return { IconSymbol::SolidArcFile, "import" };
    case FeatureStepKind::Flip:          return { IconSymbol::SolidArcFlip, "flip" };
    }
    return { IconSymbol::SolidArcBody, "step" };
}

// 📝 HTML .action-bar (47 px: undo, redo, divider, Construction history, step count … Commands) above the .history-strip
//    (85 px, #131313 → #171717): one 59 px tile per feature step, numbered, the newest amber, the focused one blue.
void DrawConstructionHistory(SolidArcWorkspaceState& State, ImVec2 Position, float Width)
{
    SolidArcSheet Sheet;
    Sheet.Begin();
    const float Left = Position.x, Top = Position.y, Right = Left + Width;
    const float Centre = Top + 0.5f * ActionBarHeight;
    Sheet.Line(Left, Top + 0.5f, Right, Top + 0.5f, SolidArcWash(0x08));
    const bool Editable = !State.Bevel.ActiveCondition;
    float Cursor = Left + 13.0f;
    if (SheetIconButton(State, Sheet, "HeaderUndo", Cursor + 12.0f, Centre, 24.0f, IconSymbol::SolidArcUndo, 15.0f, 0, Editable && !State.Document.UndoEntries.empty())) Undo(State);
    Cursor += 24.0f + 5.0f;
    if (SheetIconButton(State, Sheet, "HeaderRedo", Cursor + 12.0f, Centre, 24.0f, IconSymbol::SolidArcRedo, 15.0f, 0, Editable && !State.Document.RedoEntries.empty())) Redo(State);
    Cursor += 24.0f + 5.0f + 9.0f;
    Sheet.Surface().FillRectangle(Spanning(Cursor, Centre - 7.5f, 1.0f, 15.0f), SolidArcWash(0x10));
    Cursor += 1.0f + 9.0f + 5.0f;
    float Caption = 0.0f;
    const float CaptionWidth = 16.0f + Sheet.Measure("Construction history", 11.0f);
    if (SheetTextButton(State, Sheet, "HistoryToggle", Cursor + CaptionWidth, Centre, "Construction history", IconSymbol::Count, &Caption))
        State.HistoryFoldedEnabled = !State.HistoryFoldedEnabled;
    Cursor += Caption + 5.0f;
    const std::vector<FeatureStep>& Steps = State.Document.Tree.Steps;
    Sheet.Text(Cursor, Centre, 10.0f, SolidArcInk(0x717171), Compose("%d steps", int(Steps.size())).c_str());
    {
        const float       CommandsWidth = 16.0f + 13.0f + 8.0f + Sheet.Measure("Commands", 11.0f);
        const float       Height        = 11.0f * SolidArcStyleTokens::FontScale + 12.0f;
        const PlaneExtent Box           = Spanning(Right - 13.0f - CommandsWidth, Centre - 0.5f * Height, CommandsWidth, Height);
        const SolidArcPress Press       = PressSolidArcRegion(State.Widgets, "CommandsToggle", Box);
        const ColorQuad   Ink           = SolidArcInk(Press.HoveredCondition ? 0xffffff : 0x969696);
        if (Press.HoveredCondition) Sheet.Box(Box, SolidArcInk(0x292929), 16.0f);
        Sheet.Icon(IconSymbol::SolidArcTerminal, Box.MinimumX + 8.0f, Centre - 6.5f, 13.0f, Ink);
        Sheet.Text(Box.MinimumX + 8.0f + 13.0f + 8.0f, Centre, 11.0f, Ink, "Commands");
        if (Press.ClickedCondition) ImGui::OpenPopup("Commands");
    }
    if (ImGui::BeginPopup("Commands"))
    {
        ImGui::TextUnformatted("Command log");
        ImGui::Separator();
        const size_t First = State.Journal.size() > 40 ? State.Journal.size() - 40 : 0;
        for (size_t Index = First; Index < State.Journal.size(); ++Index) ImGui::TextUnformatted(Legible(State.Journal[Index]).c_str());
        if (State.Journal.empty()) ImGui::TextUnformatted("Nothing yet.");
        ImGui::EndPopup();
    }
    if (State.HistoryFoldedEnabled) return;

    const float StripTop = Top + ActionBarHeight;
    ImGui::GetWindowDrawList()->AddRectFilledMultiColor(ImVec2(Left, StripTop), ImVec2(Right, StripTop + HistoryHeight), IM_COL32(0x13, 0x13, 0x13, 0xff),
                                                        IM_COL32(0x13, 0x13, 0x13, 0xff), IM_COL32(0x17, 0x17, 0x17, 0xff), IM_COL32(0x17, 0x17, 0x17, 0xff));
    Sheet.Line(Left, StripTop + 0.5f, Right, StripTop + 0.5f, SolidArcWash(0x09));
    const ImVec2 Restore = ImGui::GetCursorScreenPos();
    ImGui::SetCursorScreenPos(ImVec2(Left, StripTop + 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 6.0f);
    ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, ImVec4(1, 1, 1, 0.12f));
    ImGui::BeginChild("HistoryStrip", ImVec2(Width, HistoryHeight - 1.0f), ImGuiChildFlags_None, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_HorizontalScrollbar);
    SolidArcSheet Strip;
    Strip.Begin();
    const ImVec2 Origin = ImGui::GetCursorScreenPos();
    float        TileLeft = Origin.x + 14.0f;
    const float  TileTop  = Origin.y + 8.0f + 5.0f;
    for (size_t Index = 0; Index < Steps.size(); ++Index)
    {
        const FeatureStep&       Step    = Steps[Index];
        const auto [Glyph, Word]         = StepGlyph(Step);
        const FeatureStepReport* Report  = FindReport(State.Document, Step.Label);
        const bool               Refused = Report && Report->Status == FeatureStepStatus::Refused;
        const bool               Newest  = Index + 1 == Steps.size();
        const bool               Focused = State.FocusedStep == Step.Label;
        const float              TileWidth = std::max(53.0f, Strip.Measure(Word, 8.0f) + 12.0f);
        const PlaneExtent        Tile    = Spanning(TileLeft, TileTop, TileWidth, 59.0f);
        const SolidArcPress      Press   = PressSolidArcRegion(State.Widgets, "Step:" + Step.Label, Tile);
        const ColorQuad          Fill    = Newest ? SolidArcInk(0x28241e) : Press.HoveredCondition ? SolidArcInk(0x222222) : SolidArcInk(0x1a1a1a);
        const ColorQuad          Border  = Focused ? SolidArcInk(0x4099ff) : Newest ? SolidArcInk(0xc2a16c, float(0x55) / 255.0f) : SolidArcWash(0x0a);
        Strip.Box(Tile, Fill, 10.0f, Border);
        Strip.Text(Tile.MinimumX + 1.0f + 5.0f, Tile.MinimumY + 1.0f + 2.0f + 0.5f * 7.0f * SolidArcStyleTokens::FontScale, 7.0f, SolidArcInk(0x666666),
                   Compose("%02d", int(Index) + 1).c_str());
        const float GlyphTop = Tile.MinimumY + 1.0f + 8.0f + 0.5f * (57.0f - 8.0f - (16.0f + 5.0f + 8.0f * SolidArcStyleTokens::FontScale));
        Strip.Icon(Glyph, 0.5f * (Tile.MinimumX + Tile.MaximumX) - 8.0f, GlyphTop, 16.0f, Refused ? SolidArcInk(0xff6b6b) : SolidArcInk(0xb7a68c));
        const float WordWidth = Strip.Measure(Word, 8.0f);
        Strip.Text(0.5f * (Tile.MinimumX + Tile.MaximumX - WordWidth), GlyphTop + 16.0f + 5.0f + 0.5f * 8.0f * SolidArcStyleTokens::FontScale, 8.0f,
                   Refused ? SolidArcInk(0xff6b6b) : SolidArcInk(0xa1a1a1), Word);
        if (Press.HoveredCondition)
        {
            const std::string Tip = DescribeSolidArcStep(State.Document, Step) + (Refused ? "\nRefused: " + Legible(Report->Refusal) : std::string());
            ImGui::SetTooltip("%s", Tip.c_str());
        }
        if (Press.ClickedCondition) State.FocusedStep = Focused ? std::string() : Step.Label;
        TileLeft = Tile.MaximumX + 5.0f;
    }
    ImGui::SetCursorScreenPos(Origin);
    ImGui::Dummy(ImVec2(TileLeft - Origin.x + 14.0f - 5.0f, 1.0f));
    if (State.HistoryCount != Steps.size())
    {
        State.HistoryCount = Steps.size();
        ImGui::SetScrollX(ImGui::GetScrollMaxX() + 1.0e6f);                                  // newest step in view
    }
    ImGui::EndChild();
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
    ImGui::SetCursorScreenPos(Restore);
}

void DrawViewport(SolidArcWorkspaceState& State, ImTextureID ViewportTexture, ImVec2 Position, ImVec2 Size)
{
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
    ImGui::SetNextWindowPos(Position);
    ImGui::SetNextWindowSize(Size);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, PanelRounding);
    ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                                      ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoScrollbar |
                                      ImGuiWindowFlags_NoScrollWithMouse);
    ImDrawList* Canvas = ImGui::GetWindowDrawList();
    // 📝 HTML .scene-panel: #151515, radius 23, #ffffff08 hairline; the .viewport (#171b23) between the bar and the history.
    SolidArcSheet Chrome;
    Chrome.Begin();
    Chrome.Box(Spanning(Position.x, Position.y, Size.x, Size.y), SolidArcInk(0x151515), SolidArcStyleTokens::PanelRadius, SolidArcWash(0x08));
    DrawViewportBar(State, Chrome, Spanning(Position.x, Position.y, Size.x, ViewportBar));
    DrawConstructMenu(State, ImVec2(Position.x + 13.0f, Position.y + 48.0f));              // under the Construct chip

    const ImVec2 Image(Position.x, Position.y + ViewportBar);
    const float  History = ActionBarHeight + (State.HistoryFoldedEnabled ? 0.0f : HistoryHeight);
    const ImVec2 Extent(Size.x, std::max(8.0f, Size.y - ViewportBar - History));
    State.ViewportRect = { Image.x, Image.y, Extent.x, Extent.y };
    {
        const ImVec2 Cursor = ImGui::GetCursorScreenPos();
        DrawConstructionHistory(State, ImVec2(Position.x, Image.y + Extent.y), Size.x);
        ImGui::SetCursorScreenPos(Cursor);
    }
    ImGui::SetCursorScreenPos(Image);
    ImGui::InvisibleButton("ViewportSurface", Extent, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);
    RegisterWidget(State, "ViewportSurface");
    const bool Hovered = ImGui::IsItemHovered();
    State.ViewportHovered = Hovered;
    State.Style.Gizmo     = {};
    if (State.DrawingSheet.ActiveCondition)
    {
        State.ViewportHovered = false;
        DrawDrawingSheetPaper(State, Canvas, Image, Extent, Hovered);
        Canvas->AddText(ImVec2(Image.x + 14, Image.y + Extent.y - 26), MutedColour, "Drawing sheet: wheel zoom · middle / right drag pan · double-click fit · card: paper, angle, scale, views, title, export · Esc / Alt+D model");
        ImGui::End();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor();
        return;
    }
    Canvas->AddRectFilled(Image, ImVec2(Image.x + Extent.x, Image.y + Extent.y), IM_COL32(0x17, 0x1b, 0x23, 0xff));
    if (ViewportTexture) Canvas->AddImage(ViewportTexture, Image, ImVec2(Image.x + Extent.x, Image.y + Extent.y));

    // 📝 Gestures (Plasticity orbit-controls / viewport-selector): middle drag orbit (yaw about world z, elevation about
    //    camera right), right drag pan, wheel dolly; LMB click replaces the selection, Shift+click adds, Ctrl+click
    //    removes, Alt+click adds the edge's tangent chain. In a command: click edits the selection, right-click finishes.
    ImGuiIO&     Io        = ImGui::GetIO();
    const ImVec2 Local(Io.MousePos.x - Image.x, Io.MousePos.y - Image.y);
    const ImVec2 CubeCorner(Image.x + Extent.x - CubeExtent - 18, Image.y + Extent.y - CubeExtent - 45);     // HTML .orientation: right 18, bottom 45
    const bool   LeftClick  = Hovered && ImGui::IsMouseReleased(ImGuiMouseButton_Left) && Io.MouseDragMaxDistanceSqr[ImGuiMouseButton_Left] <= ClickTolerance * ClickTolerance;
    const bool   RightClick = Hovered && ImGui::IsMouseReleased(ImGuiMouseButton_Right) && Io.MouseDragMaxDistanceSqr[ImGuiMouseButton_Right] <= ClickTolerance * ClickTolerance;
    const bool   OverCube   = DrawOrientationCube(State, Canvas, CubeCorner, LeftClick);
    const OrbitCameraFrame Frame = ViewportFrame(State);
    SolidArcNavigationPointer Navigation;
    Navigation.DeltaX              = Io.MouseDelta.x;
    Navigation.DeltaY              = Io.MouseDelta.y;
    Navigation.WheelSteps          = Io.MouseWheel;
    Navigation.MiddleDragCondition = ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 0.0f);
    Navigation.RightDragCondition  = ImGui::IsMouseDragging(ImGuiMouseButton_Right, ClickTolerance);
    Navigation.CapturedCondition   = ImGui::IsItemActive() || Hovered;
    Navigation.HoveredCondition    = Hovered;
    NavigateSolidArcPointer(State.Camera, Frame, Navigation);
    // Transform gizmo (no command running): a held or hovered handle owns the left button this frame.
    bool GizmoOwned = false;
    {
        SolidArcPhaseClock Clock(State.Profile, "viewport · gizmo");
        GizmoOwned = DriveGizmo(State, Frame, Local, Hovered && !OverCube);
    }

    // 📝 Mode 1 (Plasticity control-point mode): press on a shown pole grabs it, the pole follows the pointer in the
    //    screen-parallel plane through its start, the release commits one MoveSolidArcPole; a press-release without
    //    movement is an ordinary click (selects the curve).
    // 📝 Mode 1 sketch points (SolidArcSketchPointSession): press on a point holds it, the drag moves it in its sketch
    //    plane with a live preview, a press-release without movement picks it for the gizmo.
    SolidArcSketchPointSession& SketchPoints = State.SketchPoints;
    bool PointGesture = false;
    if (State.PickMode == SolidArcPickMode::ControlPoint && !State.Drawing.ActiveCondition && !State.Bevel.ActiveCondition && !State.Constrain.ActiveCondition
        && !State.PoleDrag.ActiveCondition && !GizmoOwned)
    {
        if (!SketchPoints.DragCondition)
        {
            SketchPoints.HoverPoint = -1;
            if (Hovered && !OverCube) FindSketchPointNear(State, Frame, Io.MousePos, SketchPoints.HoverSketch, SketchPoints.HoverPoint);
            if (SketchPoints.HoverPoint >= 0 && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                const FeatureStep* Step = FindSketchStepByLabel(State.Document, SketchPoints.HoverSketch);
                SketchPoints.DragCondition = Step != nullptr;
                SketchPoints.DragSketch    = SketchPoints.HoverSketch;
                SketchPoints.DragPoint     = SketchPoints.HoverPoint;
                SketchPoints.Origin        = Step ? SolvedSketchOf(State.Document, *Step).Points[SketchPoints.HoverPoint].Position : Vector2{};
                SketchPoints.Target        = SketchPoints.Origin;
                SketchPoints.StagedKey.clear();
                SketchPoints.Refusal.clear();
                SketchPoints.Preview.reset();
            }
        }
        if (SketchPoints.DragCondition)
        {
            PointGesture = true;
            DriveSketchPointDrag(State, Frame, Local);
        }
    }
    else
    {
        SketchPoints.HoverPoint = -1;
        if (SketchPoints.DragCondition) CancelSketchPointDrag(State);
    }
    if (State.PickMode != SolidArcPickMode::ControlPoint && !SketchPoints.Points.empty())
    {
        SketchPoints.Points.clear();
        SketchPoints.Sketch.clear();
    }
    if (!SketchPoints.DragCondition) RecordSketchPointWidgets(State, Frame);

    SolidArcPoleDrag& Drag = State.PoleDrag;
    bool PoleGesture = false;
    if (!PointGesture && !GizmoOwned && State.PickMode == SolidArcPickMode::ControlPoint && !State.Drawing.ActiveCondition && !State.Bevel.ActiveCondition && !State.Constrain.ActiveCondition)
    {
        if (!Drag.ActiveCondition)
        {
            Drag.HoverPole = -1;
            if (Hovered && !OverCube) FindPoleNear(State, Frame, Io.MousePos, Drag.HoverLabel, Drag.HoverPole);
            if (Hovered && !OverCube && Drag.HoverPole >= 0 && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                const OrbitRay Ray = CastOrbitRay(Frame, Local.x, Local.y);
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
            const OrbitRay Ray = CastOrbitRay(Frame, Local.x, Local.y);
            const Vector3     Origin{ Ray.Origin[0], Ray.Origin[1], Ray.Origin[2] }, Direction{ Ray.Direction[0], Ray.Direction[1], Ray.Direction[2] };
            const double      Facing = Dot(Direction, Drag.Facing);
            const bool        Moved  = Io.MouseDragMaxDistanceSqr[ImGuiMouseButton_Left] > ClickTolerance * ClickTolerance;
            if (std::abs(Facing) > 1.0e-9 && Moved) Drag.Position = Origin + Direction * (Dot(Drag.Origin - Origin, Drag.Facing) / Facing);
            if (Moved)
            {
                // Object snaps (points, curves, edges; never the dragged curve) place the pole exactly
                const SolidArcSnap Object = FindPointerSnap(State, Frame, Local, std::nullopt, {}, { Drag.Label }, false);
                State.Picker.Current = Object;
                if (Object.Kind != SolidArcSnapKind::None) Drag.Position = Object.Position;
            }
            if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) || !ImGui::IsMouseDown(ImGuiMouseButton_Left))
            {
                Drag.ActiveCondition = false;
                if (Moved && State.Picker.Current.Kind != SolidArcSnapKind::None) ++State.Picker.Snapped;
                State.Picker.Current = {};
                if (Moved)
                {
                    const std::string            Caption = CaptionOf(State.Document, Drag.Label);
                    const SolidArcCommandOutcome Outcome = MoveSolidArcPole(State.Document, Drag.Label, Drag.Pole, Drag.Position);
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

    {
    SolidArcPhaseClock PointerClock(State.Profile, "viewport · pointer");               // Sessions, snaps, picks
    if (PoleGesture || PointGesture)
    {
        // The pole / sketch-point drag owns the left button this frame.
    }
    else if (State.Drawing.ActiveCondition)
    {
        // 📝 Drawing: the pointer meets the construction plane, snaps to the 0.1 grid (Ctrl: free) and to the first point
        //    of a chain (closes it); click places, right-click finishes an open chain.
        SolidArcDrawingSession& Drawing = State.Drawing;
        Vector2 Point;
        bool    Closing = false;
        const bool LiftStage = Drawing.SolidCondition && Drawing.Points.size() >= SolidPlaneClicks(Drawing.Solid);
        Drawing.PointerElevation = 0.0;
        if (Hovered) FollowFacePlane(State, Frame, Local);                                   // Sketch: plane of the face under the first click
        Drawing.PointerCondition = Hovered && IntersectSketchPlane(Frame, Local, DrawingPlane(Drawing), Point);
        if (LiftStage)
        {
            // Height stage: the pointer slides on the vertical line through the last click (the plane may be edge-on).
            Drawing.PointerCondition = Hovered;
            if (Hovered) PlaceSolidLift(State, Frame, Local);
            Point           = Drawing.Points.back();
            Drawing.Pointer = Point;
        }
        else if (Drawing.SolidCondition && Drawing.Points.empty() && Hovered && PlaceSolidStart(State, Frame, Local, Point))
        {
            Drawing.PointerCondition = true;                                                 // First click onto a body: the plane rises there
            Drawing.Pointer          = Point;
        }
        else if (!Drawing.PointerCondition) State.Picker.Current = {};
        if (Drawing.PointerCondition && !LiftStage && !(Drawing.SolidCondition && Drawing.Points.empty() && State.Picker.Current.Kind != SolidArcSnapKind::None))
        {
            PlaceDrawingPointer(State, Frame, Local, Point);
            ImVec2 First;
            const OrthonormalFrame Plane = DrawingPlane(Drawing);
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
        if ((LeftClick || RightClick) && !OverCube) ApplyTransformSession(State);   // Plasticity: click, right-click or Enter commits
    }
    else if (State.Knife.ActiveCondition)
    {
        DriveKnifeSession(State, Frame, Local, Hovered && !OverCube);
        if (LeftClick && !OverCube) ClickKnifeSession(State, Local);
        else if (RightClick && !OverCube)
        {
            if (State.Knife.Mode == SolidArcKnifeMode::Insert && !State.Knife.PlacedCondition && State.Knife.HoverCondition) State.Knife.PlacedCondition = true;   // Place and apply
            ApplyKnifeSession(State);                                                        // Right-click applies like Enter
        }
    }
    else if (State.Sweep.ActiveCondition)
    {
        DriveSweepSession(State, Frame, Hovered);
        if (LeftClick && !OverCube) ClickSweepSession(State);
        else if (RightClick && !OverCube) ApplySweepSession(State);                       // Right-click commits like Enter
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
    else if (State.Fill.ActiveCondition)
    {
        HoverFillRegion(State, Frame, Local, Hovered && !OverCube);
        if (LeftClick && !OverCube) ApplyFill(State);
        if (RightClick && !OverCube) CloseFillSession(State);
    }
    else if (State.Joint.ActiveCondition)
    {
        HoverJointOrigin(State, Frame, Local, Hovered && !OverCube);
        if (LeftClick && !OverCube) ClickJointSession(State);
        if (RightClick && !OverCube) ApplyJointSession(State);                           // Right-click applies like Enter
    }
    else if (State.Bevel.ActiveCondition)
    {
        const bool Idle = !ImGui::IsAnyMouseDown();
        if (Hovered && Idle && (!State.Bevel.PinnedCondition || (State.Bevel.Verb == SolidArcSessionVerb::Inset && State.Bevel.DepthEnabled)) && Io.MouseDelta.x != 0.0f)
            AdjustBevelSize(State, Io.MouseDelta.x * Frame.WorldPerPixel * (Io.KeyShift ? 0.1 : 1.0) * 0.5);
        if (LeftClick && !OverCube)
        {
            PickAtPointer(State, Local, PickIntentOf(Io));
            State.Bevel.StagedSize = -1.0;                                                   // Selection changed: rebuild the preview
        }
        if (RightClick && !OverCube) ApplyBevelSession(State);
    }
    else if (State.Boolean.ActiveCondition && RightClick && !OverCube)
    {
        ApplyBooleanSession(State);                                                          // Right-click applies like Enter
    }
    else if (GizmoOwned)
    {
        // The gizmo took the press / hover: no selection click, no box.
    }
    else
    {
        SolidArcBoxSelection& Box = State.Box;
        const bool BoxModeCondition = State.PickMode == SolidArcPickMode::ControlPoint || State.PickMode == SolidArcPickMode::Edge || State.PickMode == SolidArcPickMode::Any;
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
                if (Moved && State.PickMode == SolidArcPickMode::ControlPoint && PickSketchPointsInBox(State, Frame, Box.Start, Box.End, PickIntentOf(Io))) {}
                else if (Moved) SelectInBox(State, Frame, Box.Start, Box.End, PickIntentOf(Io));
                else if (LeftClick && !OverCube)
                {
                    if (State.PickMode == SolidArcPickMode::ControlPoint && PickIntentOf(Io) == SolidArcPickIntent::Replace)
                    {
                        SketchPoints.Points.clear();
                        SketchPoints.Sketch.clear();
                    }
                    PickAtPointer(State, Local, PickIntentOf(Io));
                }
            }
        }
        else if (LeftClick && !OverCube)
        {
            PickAtPointer(State, Local, PickIntentOf(Io));
        }
    }

    }

    // Overlays: sketch curves, the drawing in progress, status pills (top-left), interaction hint (bottom-left).
    Canvas->PushClipRect(Image, ImVec2(Image.x + Extent.x, Image.y + Extent.y), true);
    { SolidArcPhaseClock Clock(State.Profile, "overlay · DrawSketchOverlays"); DrawSketchOverlays(State, Canvas, Frame); }
    { SolidArcPhaseClock Clock(State.Profile, "overlay · DrawTrimPreview"); DrawTrimPreview(State, Canvas, Frame); }
    { SolidArcPhaseClock Clock(State.Profile, "overlay · DrawFillPreview"); DrawFillPreview(State, Canvas, Frame); }
    { SolidArcPhaseClock Clock(State.Profile, "overlay · DrawConstraintOverlay"); DrawConstraintOverlay(State, Canvas, Frame); }
    { SolidArcPhaseClock Clock(State.Profile, "overlay · DrawSweepOutline"); DrawSweepOutline(State, Canvas, Frame); }
    { SolidArcPhaseClock Clock(State.Profile, "overlay · DrawKnifeGhost"); DrawKnifeGhost(State, Canvas, Frame); }
    { SolidArcPhaseClock Clock(State.Profile, "overlay · DrawSectionOutline"); DrawSectionOutline(State, Canvas, Frame); }
    { SolidArcPhaseClock Clock(State.Profile, "overlay · DrawMeasureOverlay"); DrawMeasureOverlay(State, Canvas, Frame); }
    { SolidArcPhaseClock Clock(State.Profile, "overlay · DrawJointOverlay"); DrawJointOverlay(State, Canvas, Frame); }
    { SolidArcPhaseClock Clock(State.Profile, "overlay · DrawCurveOverlays"); DrawCurveOverlays(State, Canvas, Frame); }
    if (State.DeviationEnabled) { SolidArcPhaseClock Clock(State.Profile, "overlay · DrawDeviationOverlay"); DrawDeviationOverlay(State, Canvas, Frame); }
    if (State.Drawing.ActiveCondition) { SolidArcPhaseClock Clock(State.Profile, "overlay · DrawDrawingPreview"); DrawDrawingPreview(State, Canvas, Frame); }
    { SolidArcPhaseClock Clock(State.Profile, "overlay · DrawSnapOverlay"); DrawSnapOverlay(State, Canvas, Frame); }
    if (State.Box.ActiveCondition && Io.MouseDragMaxDistanceSqr[ImGuiMouseButton_Left] > ClickTolerance * ClickTolerance)
    {
        const ImVec2 Low(std::min(State.Box.Start.x, State.Box.End.x), std::min(State.Box.Start.y, State.Box.End.y));
        const ImVec2 High(std::max(State.Box.Start.x, State.Box.End.x), std::max(State.Box.Start.y, State.Box.End.y));
        Canvas->AddRectFilled(Low, High, IM_COL32(0x4d, 0x9f, 0xff, 0x22));
        Canvas->AddRect(Low, High, IM_COL32(0x4d, 0x9f, 0xff, 0xcc));
    }
    Canvas->PopClipRect();
    {
        SolidArcSheet Label;
        Label.Begin();
        DrawViewportLabel(State, Label, Image, Hovered);
    }
    if (State.Bevel.ActiveCondition)
    {
        ImVec2 Below(Image.x + 72, Image.y + 64);
        const bool Refused = !State.Bevel.Refusal.empty();
        const char* Caption = State.Bevel.Verb == SolidArcSessionVerb::Profile ? "Extrude sketch d" : State.Bevel.Verb == SolidArcSessionVerb::Extrude ? "Extrude d"
                            : State.Bevel.Verb == SolidArcSessionVerb::Inset ? (State.Bevel.DepthEnabled ? "Inset · H depth · d" : "Inset · D distance · d")
                            : State.Bevel.Verb == SolidArcSessionVerb::Corner ? (State.Bevel.Shape == BlendProfile::Fillet ? "Sketch fillet r" : "Sketch chamfer d")
                            : State.Bevel.Shape == BlendProfile::Fillet ? "Fillet r" : "Chamfer d";
        const std::string Sizes = State.Bevel.VariableEnabled && State.Bevel.Verb == SolidArcSessionVerb::Blend
                                ? FormatLength(State.Bevel.Size) + " → " + FormatLength(State.Bevel.EndSize) + (State.Bevel.ReversedCondition ? " (flipped)" : "")
                                : State.Bevel.Verb == SolidArcSessionVerb::Inset ? FormatLength(State.Bevel.Size) + " m · depth " + FormatLength(State.Bevel.Depth)
                                : FormatLength(State.Bevel.Size);
        const std::string Boolean = State.Bevel.Verb != SolidArcSessionVerb::Profile ? std::string{}
                                  : std::string(" · ") + (State.Bevel.Combination == FeatureCombination::New ? "New" : FeatureCombinationLabel(State.Bevel.Combination));
        DrawStatusPill(Canvas, Below, Compose("%s %s m%s%s", Caption, Sizes.c_str(), Boolean.c_str(),
                                              State.Bevel.PinnedCondition ? " · pinned" : ""), Refused ? RefusalColour : ActiveColour);
        if (Refused) DrawStatusPill(Canvas, Below, State.Bevel.Refusal, RefusalColour);
    }
    if (State.Gizmo.DragCondition)
    {
        ImVec2 Below(Image.x + 72, Image.y + 64);
        const SolidArcGizmoSession& Gizmo = State.Gizmo;
        DrawStatusPill(Canvas, Below, Compose("Gizmo · %s %d · %s%s", GizmoTargetLabel(Gizmo.Target), int(Gizmo.Names.size()), Gizmo.Readout.c_str(), ImGui::GetIO().KeyCtrl ? " · free" : " · snapped"),
                       Gizmo.Refusal.empty() ? ActiveColour : RefusalColour);
        if (!Gizmo.Refusal.empty()) DrawStatusPill(Canvas, Below, Gizmo.Refusal, RefusalColour);
    }
    if (Drag.ActiveCondition)
    {
        ImVec2 Below(Image.x + 72, Image.y + 64);
        const Vector3 Shift = Drag.Position - Drag.Origin;
        DrawStatusPill(Canvas, Below, Compose("Pole %d of %s · moved %.2f mm (%.2f, %.2f, %.2f)", Drag.Pole, CaptionOf(State.Document, Drag.Label).c_str(), Length(Shift) * 1000.0,
                                              Shift.X * 1000.0, Shift.Y * 1000.0, Shift.Z * 1000.0), ActiveColour);
    }
    if (State.Drawing.ActiveCondition)
    {
        ImVec2 Below(Image.x + 72, Image.y + 64);
        const SolidArcDrawingSession& Drawing = State.Drawing;
        const std::string Pointer = Drawing.PointerCondition ? Compose(" · (%s, %s)", FormatLength(Drawing.Pointer.X).c_str(), FormatLength(Drawing.Pointer.Y).c_str()) : std::string{};
        if (Drawing.SolidCondition)
            DrawStatusPill(Canvas, Below, Compose("%s · %s%s", DrawingCaption(Drawing).c_str(), SolidPrompt(Drawing),
                                                  Drawing.Points.size() >= SolidPlaneClicks(Drawing.Solid) ? Compose(" · height %s", FormatLength(Drawing.Lift).c_str()).c_str() : Pointer.c_str()), ActiveColour);
        else DrawStatusPill(Canvas, Below, Compose("%s · %d point(s)%s", DrawingCaption(Drawing).c_str(), int(Drawing.Points.size()), Pointer.c_str()), ActiveColour);
        if (!Drawing.Refusal.empty()) DrawStatusPill(Canvas, Below, Drawing.Refusal, RefusalColour);
    }
    if (State.Trim.ActiveCondition)
    {
        ImVec2 Below(Image.x + 72, Image.y + 64);
        const SolidArcTrimSession& Trim = State.Trim;
        if (!Trim.Refusal.empty()) DrawStatusPill(Canvas, Below, Trim.Refusal, RefusalColour);
        else DrawStatusPill(Canvas, Below, Compose("Trim · %d trimmed%s", Trim.Commits, Trim.HoverCondition ? " · click removes the red piece" : ""), ActiveColour);
    }
    if (State.Fill.ActiveCondition)
    {
        ImVec2 Below(Image.x + 72, Image.y + 64);
        const SolidArcFillSession& Fill = State.Fill;
        DrawStatusPill(Canvas, Below, Compose("Fill · %d filled · %s", Fill.Commits, Fill.HoverCondition ? (Fill.WholeSketch.empty() ? "click fills this region into a new sketch" : Compose("click fills %s", CaptionOf(State.Document, Fill.WholeSketch).c_str()).c_str()) : Fill.Refusal.c_str()), ActiveColour);
    }
    if (State.Constrain.ActiveCondition)
    {
        ImVec2 Below(Image.x + 72, Image.y + 64);
        bool Trouble = false;
        const std::string Readout = ConstraintSessionReadout(State, Trouble);
        DrawStatusPill(Canvas, Below, Readout, Trouble ? RefusalColour : ActiveColour);
        if (!State.Constrain.Refusal.empty())
        {
            ImVec2 Next(Image.x + 72, Image.y + 96);
            DrawStatusPill(Canvas, Next, State.Constrain.Refusal, RefusalColour);
        }
    }
    if (State.Transform.ActiveCondition)
    {
        ImVec2 Below(Image.x + 72, Image.y + 64);
        const SolidArcTransformSession& Transform = State.Transform;
        DrawStatusPill(Canvas, Below, Compose("%s%s · %d body(ies)", Transform.DuplicateCondition ? "Duplicate · " : "", Transform.Readout.c_str(), int(Transform.Keys.size())), Transform.Refusal.empty() ? ActiveColour : RefusalColour);
        if (!Transform.Refusal.empty()) DrawStatusPill(Canvas, Below, Transform.Refusal, RefusalColour);
    }
    if (State.Knife.ActiveCondition && !State.Knife.DrawPendingCondition)
    {
        ImVec2 Below(Image.x + 72, Image.y + 64);
        DrawStatusPill(Canvas, Below, KnifeReadout(State), State.Knife.Refusal.empty() ? ActiveColour : RefusalColour);
        if (!State.Knife.Refusal.empty()) DrawStatusPill(Canvas, Below, State.Knife.Refusal, RefusalColour);
    }
    if (State.Sweep.ActiveCondition)
    {
        ImVec2 Below(Image.x + 72, Image.y + 64);
        const SolidArcSweepSession& Sweep = State.Sweep;
        const std::string Boolean = Sweep.Verb != SolidArcSweepVerb::Offset ? std::string(" · ") + (Sweep.Combination == FeatureCombination::New ? "New" : FeatureCombinationLabel(Sweep.Combination)) : std::string{};
        const std::string Typed   = Sweep.Typed.empty() ? std::string{} : " · typed " + Sweep.Typed;
        if (Sweep.Verb == SolidArcSweepVerb::Revolve)
            DrawStatusPill(Canvas, Below, Compose("Revolve %s %s° about %s%s%s", CaptionOf(State.Document, Sweep.Path).c_str(), FormatLength(Sweep.Amount).c_str(),
                                                  DescribeSolidArcRevolveAxis(State.Document, Sweep.Path, Sweep.Axis).c_str(), Boolean.c_str(), Typed.c_str()), Sweep.Refusal.empty() ? ActiveColour : RefusalColour);
        else if (Sweep.Verb == SolidArcSweepVerb::Evolution)
            DrawStatusPill(Canvas, Below, Compose("Evolution %s along %s%s", CaptionOf(State.Document, Sweep.Profile).c_str(), Sweep.Path.empty() ? "?" : CaptionOf(State.Document, Sweep.Path).c_str(), Boolean.c_str()),
                           Sweep.Refusal.empty() ? ActiveColour : RefusalColour);
        else
            DrawStatusPill(Canvas, Below, Compose("%s %s %s m%s%s", SweepVerbName(Sweep.Verb), CaptionOf(State.Document, Sweep.Path).c_str(), FormatLength(Sweep.Amount).c_str(), Boolean.c_str(), Typed.c_str()),
                           Sweep.Refusal.empty() ? ActiveColour : RefusalColour);
        if (Sweep.HoverCurve >= 0)
        {
            const SketchModel* Sketch = SweepSketch(State, Sweep.HoverSketch);
            if (Sketch && Sweep.HoverCurve < int(Sketch->Curves.size()))
                DrawStatusPill(Canvas, Below, Sweep.Verb == SolidArcSweepVerb::Revolve ? "click: axis = " + SketchCurveName(Sketch->Curves[Sweep.HoverCurve]) : "click: path = " + CaptionOf(State.Document, Sweep.HoverSketch), ActiveColour);
        }
        if (!Sweep.Refusal.empty()) DrawStatusPill(Canvas, Below, Sweep.Refusal, RefusalColour);
    }
    const char* Hint = State.Gizmo.DragCondition
                           ? "Gizmo: drag the handle (snaps: 0.1 move · 5° turn · ×0.1 scale; Ctrl free) · release commits · Esc cancels"
                       : State.Gizmo.Hover != SolidArcGizmoHandle::None
                           ? "Gizmo: press and drag · cones move along an axis · squares move in a plane · arcs rotate · cylinders scale · ring screen move / uniform scale · y cycles modes"
                       : State.Transform.ActiveCondition
                           ? (State.Transform.Verb == SolidArcTransformVerb::Move ? "Move: pointer snaps to points / edges / faces (Ctrl free) · X / Y / Z axis · Shift+X / Y / Z plane · G screen · type a distance · click / right-click / Enter commit · Esc cancel"
                              : State.Transform.Verb == SolidArcTransformVerb::Rotate ? "Rotate: sweep about the centre (5° steps, Ctrl free) · X / Y / Z axis · R screen · type degrees · click / right-click / Enter commit · Esc cancel"
                              : "Scale: distance to the centre (×0.1 steps, Ctrl free) · X / Y / Z one axis · S uniform · type a factor · click / right-click / Enter commit · Esc cancel")
                       : State.Knife.ActiveCondition && !State.Knife.DrawPendingCondition
                           ? (State.Knife.Mode == SolidArcKnifeMode::Insert ? "Knife insert: point at a face · click places · move sideways to size (Shift fine, Ctrl free) · [ / ] turn 15° · type a width · Q add · W cut · E intersect · R separate · click / right-click / Enter apply · Tab mode · Esc cancel"
                              : State.Knife.Mode == SolidArcKnifeMode::Object ? "Knife object: the first picked body is cut by the others (Shift-click adds cutters) · K keep cutters · Q add · W cut · E intersect · right-click / Enter apply · Tab mode · Esc cancel"
                              : "Knife sketch: move sideways for the depth (Shift fine, Ctrl free) · type a depth · T through · Q add (boss) · W cut · E intersect · R separate · click / right-click / Enter apply · Tab mode · Esc cancel")
                       : State.Sweep.ActiveCondition
                           ? (State.Sweep.Verb == SolidArcSweepVerb::Offset ? "Offset: move sideways (Shift fine, Ctrl free; negative = inward / right) · type a distance · click / right-click / Enter commit · Esc cancel"
                              : State.Sweep.Verb == SolidArcSweepVerb::Revolve ? "Revolve: type degrees (360) · X / Y sketch axis · click a profile line for the axis · Q union · W difference · E intersect · R new body · click elsewhere / Enter commit · Esc cancel"
                              : State.Sweep.Verb == SolidArcSweepVerb::Evolution ? "Evolution: click another sketch's curve for the path (or select a curve first) · Q union · W difference · E intersect · R new body · click elsewhere / Enter commit · Esc cancel"
                              : "Pipe: move sideways for the radius (Shift fine, Ctrl free) · type a radius · Q union · W difference · E intersect · R new body · click / Enter commit · Esc cancel")
                       : State.Drawing.ActiveCondition
                           ? (State.Drawing.SolidCondition ? "Draw on the ground or any body face (oriented to it; snaps, Ctrl free) · move out for the height (0.1 steps) · R new body · Q union · W difference · E intersect · Tab exact size · Ctrl+Z drop · Esc abort"
                              : State.Drawing.Kind == SolidArcDrawingKind::Polygon ? "Click the centre, then a vertex (snaps, Ctrl free) · + / − sides · x y z t n b lock · Ctrl+Z drop · Esc abort"
                              : State.Drawing.Kind == SolidArcDrawingKind::Curve ? "Click points (grid; Ctrl ends / midpoints / centres, Alt slides along a curve; a body face under the first click carries the sketch) · x y z t n b lock · 1 Hermite · 2 Bezier · 3 NURBS · 4 cubic · click first point closes · Enter / right-click finish · Ctrl+Z drop · Esc abort"
                              : State.Drawing.Kind == SolidArcDrawingKind::Line ? "Click points (grid; Ctrl ends / midpoints / centres, Alt slides along a curve; a body face under the first click carries the sketch) · x y z t n b lock · click first point closes · Enter / right-click finish · Ctrl+Z drop · Esc abort"
                              : State.Drawing.Kind == SolidArcDrawingKind::Rectangle ? "Click two corners (grid; Ctrl ends / midpoints / centres, Alt slides along a curve; a body face under the first click carries the sketch) · x y z t n b lock · s square · Alt centre / corner · Ctrl+Z drop · Esc abort"
                              : State.Drawing.Kind == SolidArcDrawingKind::Circle ? "Click centre then rim (grid; Ctrl ends / midpoints / centres, Alt slides along a curve; a body face under the first click carries the sketch) · x y z t n b lock · V two-point / centre · Ctrl+Z drop · Esc abort"
                              : State.Drawing.Kind == SolidArcDrawingKind::Arc ? "Click start, end, then a point on the arc (grid; Ctrl ends / midpoints / centres, Alt slides along a curve; a body face under the first click carries the sketch) · x y z t n b lock · Ctrl+Z drop · Esc abort"
                              : "Click both cap centres, then the rim (grid; Ctrl ends / midpoints / centres, Alt slides along a curve; a body face under the first click carries the sketch) · x y z t n b lock · Ctrl+Z drop · Esc abort")
                       : State.Constrain.ActiveCondition
                           ? "Constrain: click points / curves · D dimension · N angle · H / V · O on · Q equal · M midpoint · Y symmetric · L parallel / tangent · [ perpendicular · ] lock · drag a point · click a glyph: Delete removes, digits + Enter edit · Esc / K end"
                       : State.Trim.ActiveCondition
                           ? "Trim: hover a curve (the red piece comes off between its crossings) · click removes it · Esc / T / right-click end"
                       : State.Fill.ActiveCondition
                           ? "Fill: hover inside closed curves (overlaps are their own regions) · click fills the highlighted region · E then extrudes a solid · Esc / F / right-click end"
                       : State.Bevel.ActiveCondition && State.Bevel.Verb == SolidArcSessionVerb::Profile
                           ? "Move: distance (Shift fine) · Q union · W difference · E intersect · R new body · Enter / right-click finish · Esc abort"
                       : State.Bevel.ActiveCondition && State.Bevel.Verb == SolidArcSessionVerb::Inset
                           ? "Move: inset distance or depth (Shift fine) · D distance · H depth (− pocket) · click edits the faces · Enter / right-click finish · Esc abort"
                       : State.Bevel.ActiveCondition
                           ? "Move: size (Shift fine) · D fillet · C chamfer · V variable · F flip anchor · click edits selection · Enter / right-click finish · Esc abort"
                       : State.PickMode == SolidArcPickMode::ControlPoint
                           ? "Mode 1: drag a sketch point or pole (live, release commits, Esc drops; Ctrl ends / midpoints, Alt along a curve) · click a point for the gizmo (Shift add, box picks) · click a curve · L loft · 2/3/4/5 modes"
                           : "Middle orbit · right pan · wheel zoom · click select (Shift add, Ctrl remove, Alt chain) · 1-5 modes · B fillet · E extrude · L loft · Shift+A line · Shift+S curve";
    {
        SolidArcSheet Instructions;
        Instructions.Begin();
        DrawViewportInstructions(Instructions, Image.x + 17.0f, Image.y + Extent.y - 12.0f, Extent.x - 34.0f - CubeExtent - 18.0f, Hint);
    }

    // The overlay clusters are top-level windows of their own (begun here so they measure inside a window).
    DrawSelectionRail(State, Image);
    DrawDisplayMenu(State, Image, Extent.x);
    DrawGizmoChips(State, Image, Extent.x);
    DrawSketchToolbar(State, Image, Extent);
    ImGui::End();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    INSPECTOR
//------------------------------------------------------------------------------------------------------------------------

void DrawBevelCard(SolidArcWorkspaceState& State)
{
    SolidArcBevelSession& Bevel   = State.Bevel;
    const bool            Profile = Bevel.Verb == SolidArcSessionVerb::Profile;
    const bool            Inset   = Bevel.Verb == SolidArcSessionVerb::Inset;
    const bool            Extrude = Bevel.Verb == SolidArcSessionVerb::Extrude || Profile || Inset;
    const bool            Corner  = Bevel.Verb == SolidArcSessionVerb::Corner;
    BeginCard(Inset ? "Inset" : Profile ? "Extrude sketch" : Extrude ? "Extrude" : Corner ? "Sketch corners" : "Bevel");
    if (Inset)
    {
        // The pointer drives one value: D the inset distance, H the depth of the inner face (+ boss, − pocket).
        if (ToggleButton(State, "Distance  (D)###InsetDistance", !Bevel.DepthEnabled, RowSpan())) Bevel.DepthEnabled = false;
        ImGui::SameLine();
        if (ToggleButton(State, "Depth  (H)###InsetDepth", Bevel.DepthEnabled, RowSpan())) Bevel.DepthEnabled = true;
    }
    if (Profile)
    {
        // Plasticity extrude Boolean keys: q union · w difference · e intersect · r new body.
        static const struct { const char* Label; FeatureCombination Combination; } Choices[] = {
            { "Union (Q)###ProfileUnion", FeatureCombination::Union }, { "Difference (W)###ProfileDifference", FeatureCombination::Difference },
            { "Intersect (E)###ProfileIntersect", FeatureCombination::Intersection }, { "New body (R)###ProfileNew", FeatureCombination::Separate } };
        for (int Index = 0; Index < 4; ++Index)
        {
            if (Index % 2 == 1) ImGui::SameLine();
            if (ToggleButton(State, Choices[Index].Label, Bevel.Combination == Choices[Index].Combination, RowSpan())) ChooseProfileCombination(State, Choices[Index].Combination);
        }
    }
    if (!Extrude)
    {
        if (ToggleButton(State, "Fillet  (D)###BevelFillet", Bevel.Shape == BlendProfile::Fillet, RowSpan())) Bevel.Shape = BlendProfile::Fillet;
        ImGui::SameLine();
        if (ToggleButton(State, "Chamfer  (C)###BevelChamfer", Bevel.Shape == BlendProfile::Chamfer, RowSpan())) Bevel.Shape = BlendProfile::Chamfer;
    }
    ImGui::SetNextItemWidth(110);
    if (ImGui::InputText(Extrude ? "Distance###BevelSize" : Bevel.Shape == BlendProfile::Fillet ? "Radius###BevelSize" : "Setback###BevelSize", Bevel.SizeText.data(),
                         Bevel.SizeText.size(), ImGuiInputTextFlags_EnterReturnsTrue))
    {
        double Parsed = 0.0;
        if (ParseSolidArcLength(Bevel.SizeText.data(), Parsed) && Parsed != 0.0)
        {
            if (Parsed < 0.0 && !Extrude) Bevel.Shape = BlendProfile::Chamfer;
            Bevel.Size            = Extrude && !Inset ? Parsed : std::abs(Parsed);
            Bevel.PinnedCondition = true;
            Announce(State, Compose(Inset ? "Inset distance %s m (pinned)" : "Bevel size %s m (pinned)", FormatLength(Bevel.Size).c_str()));
        }
        else
        {
            Announce(State, std::string("Unreadable size: ") + Bevel.SizeText.data());
        }
        WriteBevelText(Bevel);
    }
    else if (ImGui::IsItemEdited()) Bevel.SizeTypedCondition = true;
    RegisterWidget(State, "BevelSize");
    ImGui::SameLine();
    if (PressButton(State, "Mouse adjust###BevelMouse", ImVec2(0, 0), Bevel.PinnedCondition)) Bevel.PinnedCondition = false;
    if (Inset)
    {
        ImGui::SetNextItemWidth(110);
        if (ImGui::InputText("Depth###InsetDepthText", Bevel.DepthText.data(), Bevel.DepthText.size(), ImGuiInputTextFlags_EnterReturnsTrue))
        {
            double Parsed = 0.0;
            if (ParseSolidArcLength(Bevel.DepthText.data(), Parsed))
            {
                Bevel.Depth = Parsed;
                Announce(State, Compose("Inset depth %s m", FormatLength(Bevel.Depth).c_str()));
            }
            else
            {
                Announce(State, std::string("Unreadable depth: ") + Bevel.DepthText.data());
            }
            WriteBevelText(Bevel);
        }
        RegisterWidget(State, "InsetDepthText");
        ImGui::PushStyleColor(ImGuiCol_Text, Tint(MutedColour));
        ImGui::TextWrapped("Border ring %s m wide · inner face %s m along the normal (0 = flat split, − pocket)", FormatLength(Bevel.Size).c_str(), FormatLength(Bevel.Depth).c_str());
        ImGui::PopStyleColor();
    }
    if (!Extrude && !Corner)
    {
        // Variable size (V): the end size field and the anchor flip (F).
        if (ToggleButton(State, "Variable  (V)###BevelVariable", Bevel.VariableEnabled, RowSpan())) ToggleBevelVariable(State);
        if (Bevel.VariableEnabled)
        {
            ImGui::SameLine();
            if (PressButton(State, "Flip anchor  (F)###BevelFlip", RowSpan())) FlipBevelAnchor(State);
            ImGui::SetNextItemWidth(110);
            if (ImGui::InputText("End size###BevelEndSize", Bevel.EndSizeText.data(), Bevel.EndSizeText.size(), ImGuiInputTextFlags_EnterReturnsTrue))
            {
                double Parsed = 0.0;
                if (ParseSolidArcLength(Bevel.EndSizeText.data(), Parsed) && Parsed > 0.0)
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
        if (ProfileStart(Bevel) < 0.0)
            ImGui::TextWrapped("On a curved face: starts %s inside it, so the union closes", FormatLength(Bevel.ProfileSink).c_str());
    }
    else if (Corner)
    {
        std::string Owners;
        for (const std::string& Sketch : Bevel.Corner.Sketches)
        {
            const auto Caption = State.Document.Captions.find(Sketch);
            Owners += (Owners.empty() ? "" : ", ") + (Caption != State.Document.Captions.end() ? Caption->second : Sketch);
        }
        ImGui::TextWrapped("%s of %s · lines meeting at a corner get a %s", Bevel.Corner.Points.empty() ? "Every corner" : Compose("%d picked corner(s)", int(Bevel.Corner.Points.size())).c_str(),
                           Owners.c_str(), Bevel.Shape == BlendProfile::Fillet ? "tangent arc" : "straight cut");
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
    if (PressButton(State, "Apply###BevelApply", RowSpan(), Bevel.Refusal.empty())) ApplyBevelSession(State);
    ImGui::SameLine();
    if (PressButton(State, "Cancel###BevelCancel", RowSpan())) CancelBevelSession(State);
    EndCard();
}

// Tool cards of the picked figures (Geometry tab): curves (loft, combs, continuity) and face edit.
void DrawSelectionToolCards(SolidArcWorkspaceState& State)
{
    const SolidArcSelection& Selection = State.Document.Selection;

    if (!Selection.Curves.empty())
    {
        BeginCard("Curves");
        ImGui::PushStyleColor(ImGuiCol_Text, Tint(MutedColour));
        ImGui::TextWrapped("Loft runs through the curves in the order picked (badges 1, 2, …); curves that cross make a network. Mode 1 shows the poles: drag one to reshape.");
        ImGui::PopStyleColor();
        if (PressButton(State, "Loft  (L)###CurveLoft", RowSpan(), Selection.Curves.size() >= 2)) LoftSelectedCurves(State);
        ImGui::SameLine();
        const size_t Sketches = SelectedSketchLabels(State.Document).size();
        if (PressButton(State, "Join  (J)###CurveJoin", RowSpan(), Sketches >= 2 && !State.Bevel.ActiveCondition)) JoinSketches(State);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            ImGui::SetTooltip("%s", Sketches >= 2 ? "Join the picked sketches into one (in pick order); a sketch a body still reads is kept hidden"
                                                  : "Shift-click two or more coplanar sketches to join them");
        // Plasticity fillet-curve: B rounds every corner of the picked sketches (or the picked mode-1 corner points).
        const bool CornerCondition = !CornerTargets(State).Sketches.empty() && !State.Bevel.ActiveCondition;
        if (PressButton(State, "Fillet  (B)###CurveFillet", RowSpan(), CornerCondition)) OpenBevelSession(State, BlendProfile::Fillet);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            ImGui::SetTooltip("Round the corners where two lines meet (every corner of the picked sketches, or the corner points picked in mode 1)");
        ImGui::SameLine();
        if (PressButton(State, "Chamfer###CurveChamfer", RowSpan(), CornerCondition)) OpenBevelSession(State, BlendProfile::Chamfer);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) ImGui::SetTooltip("Cut the corners straight, the same distance back along both lines");
        if (ToggleButton(State, "Combs###CurveCombs", State.CombsEnabled, ImVec2(70, 0))) State.CombsEnabled = !State.CombsEnabled;
        ImGui::SameLine();
        // Continuity of the next loft to the panel it meets at its first / last section (V6).
        static const char* const ContinuityLabels[] = { "G0###LoftG0", "G1###LoftG1", "G2###LoftG2" };
        for (int Order = 0; Order < 3; ++Order)
        {
            if (Order) ImGui::SameLine();
            if (ToggleButton(State, ContinuityLabels[Order], State.LoftContinuity == Order, ImVec2(44, 0))) State.LoftContinuity = Order;
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Continuity of the next loft to the panel it meets at its first / last section");
        }
        EndCard();
    }
    if (!Selection.Faces.empty())
    {
        BeginCard("Face edit");
        int Current = 0;
        for (int Index = 0; Index < int(std::size(FaceVerbs)); ++Index)
            if (FaceVerbs[Index] == State.FaceVerb) Current = Index;
        ImGui::SetNextItemWidth(150);
        if (ImGui::BeginCombo("Verb###FaceVerb", SolidArcFaceOperationLabel(State.FaceVerb)))
        {
            for (int Index = 0; Index < int(std::size(FaceVerbs)); ++Index)
            {
                const std::string Label = std::string(SolidArcFaceOperationLabel(FaceVerbs[Index])) + "###FaceVerb:" + SolidArcFaceOperationLabel(FaceVerbs[Index]);
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
            const SolidArcCommandOutcome Outcome = State.FaceVerb == FaceOperationKind::Inset ? InsetSolidArcSelection(State.Document, Amount, 0.0)
                                                                                              : ChangeSolidArcSelection(State.Document, State.FaceVerb, Amount);
            if (Outcome.AcceptedCondition) State.LastCommand = { SolidArcRepeatKind::Face, BlendProfile::Fillet, 0.0, State.FaceVerb, Amount, {} };
            Conclude(State, Outcome);
        }
        EndCard();
    }
}

void DrawStepCard(SolidArcWorkspaceState& State)
{
    const FeatureStep* Step = nullptr;
    for (const FeatureStep& Candidate : State.Document.Tree.Steps)
        if (Candidate.Label == State.FocusedStep) Step = &Candidate;
    if (!Step) return;
    const FeatureStepReport* Report = FindReport(State.Document, Step->Label);
    const std::string        Label  = Step->Label;
    BeginCard("Feature");
    ImGui::TextWrapped("%s", DescribeSolidArcStep(State.Document, *Step).c_str());
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
        ImGui::TextWrapped("Splits %s by %s%s · inside piece → %s", SolidArcBodyCaption(State.Document, Step->Target).c_str(), Cutter.c_str(),
                           Step->Sections.empty() ? "" : Compose(" seen along (%.0f, %.0f, %.0f)", Step->Plane.ZAxis.X, Step->Plane.ZAxis.Y, Step->Plane.ZAxis.Z).c_str(),
                           SolidArcBodyCaption(State.Document, Step->Label).c_str());
    }
    if (Step->Kind == FeatureStepKind::Remove) ImGui::Text("Deletes %s", SolidArcBodyCaption(State.Document, Step->Target).c_str());
    if (Step->Kind == FeatureStepKind::Transform || Step->Kind == FeatureStepKind::Duplicate)
    {
        const AffineMap& Map = Step->Placement;
        const Vector3 Factors{ Length(Map.ColumnX), Length(Map.ColumnY), Length(Map.ColumnZ) };
        const double Trace = Map.ColumnX.X / Factors.X + Map.ColumnY.Y / Factors.Y + Map.ColumnZ.Z / Factors.Z;
        const double Turn  = std::acos(std::clamp((Trace - 1.0) * 0.5, -1.0, 1.0)) * 180.0 / std::numbers::pi;   // [°] - Rotation part (uniform scale)
        const std::string Subject = SolidArcBodyCaption(State.Document, Step->Target.empty() ? std::string("the active body") : Step->Target);
        ImGui::TextWrapped("%s %s · shift (%.3f, %.3f, %.3f) · turn %.2f° · scale (%.3f, %.3f, %.3f)%s", Step->Kind == FeatureStepKind::Transform ? "Moves" : "Copies", Subject.c_str(),
                           Map.Shift.X, Map.Shift.Y, Map.Shift.Z, Turn, Factors.X, Factors.Y, Factors.Z,
                           Step->Source.empty() ? "" : (" · as it stood after " + CaptionOf(State.Document, Step->Source)).c_str());
    }
    if (Step->Kind == FeatureStepKind::Revolve && !Step->Profile.empty() && CaptionOf(State.Document, Label).rfind("Revolve", 0) == 0)   // Alt+R revolves (not the sphere / torus tiles)
    {
        const SolidArcRevolveAxis Axis{ Step->AxisIdentifier, Step->AxisOrigin, Step->AxisDirection };
        ImGui::TextWrapped("Turns %s %.2f° about %s", CaptionOf(State.Document, Step->Profile).c_str(), EvaluateFeatureValue(State.Document.Tree, Step->Angle) * 180.0 / std::numbers::pi,
                           DescribeSolidArcRevolveAxis(State.Document, Step->Profile, Axis).c_str());
    }
    if (Step->Kind == FeatureStepKind::Evolution)
        ImGui::TextWrapped("Sweeps %s along %s (rotation-minimizing frames)", CaptionOf(State.Document, Step->Profile).c_str(), CaptionOf(State.Document, Step->Path).c_str());
    if (Step->Kind == FeatureStepKind::Joint)
        ImGui::TextWrapped("%s joint: %s onto %s · %d degree(s) of freedom%s · drag a value (Ctrl+click types it)", JointKindLabel(Step->Joint.Kind),
                           SolidArcBodyCaption(State.Document, Step->Target).c_str(), SolidArcBodyCaption(State.Document, Step->Tool).c_str(), JointFreedom(Step->Joint.Kind),
                           Step->Joint.FlippedCondition ? " · flipped" : "");
    if (Step->Kind == FeatureStepKind::Flip)
        ImGui::TextWrapped("Normals of %s turned over (every face's sense reversed) · Alt+N on it again removes this step", SolidArcBodyCaption(State.Document, Step->Target).c_str());
    if (Step->Kind == FeatureStepKind::Import)
    {
        // The embedded file's PRODUCT name and size; faces are named Face(k) in file order.
        const size_t      Mark    = Step->Exchange.find("PRODUCT('");
        const std::string Product = Mark == std::string::npos ? std::string("?") : Step->Exchange.substr(Mark + 9, Step->Exchange.find('\'', Mark + 9) - Mark - 9);
        ImGui::TextWrapped("STEP body %s embedded (%.1f kB, %.0f mm per model unit) · faces %s.Face(1 …) in file order", Product.c_str(), Step->Exchange.size() / 1000.0,
                           Step->ExchangeUnit, Label.c_str());
    }
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
            SolidArcParameterDrag& Drag    = State.ParameterDrag;
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
                Drag.Preview   = std::make_unique<SolidArcDocument>(CloneSolidArcDocument(State.Document));
                const SolidArcCommandOutcome Staged = AssignSolidArcParameter(*Drag.Preview, Parameter.Name, Dragged);
                if (Staged.AcceptedCondition) { Drag.PreviewScene = BuildSolidArcScene(*Drag.Preview); ++Drag.Stagings; }
                else { Drag.Preview.reset(); Announce(State, Staged.Refusal); }
            }
            if (ImGui::IsItemDeactivated() && Drag.Parameter == Parameter.Name)
            {
                const double Final = Drag.Value;
                Drag.Parameter.clear();
                Drag.Preview.reset();
                if (Final != Parameter.Value) { Conclude(State, AssignSolidArcParameter(State.Document, Parameter.Name, Final)); break; }
            }
            continue;
        }
        double Value = Parameter.Value;
        ImGui::SetNextItemWidth(150);
        ImGui::InputDouble(Field.c_str(), &Value, 0.0, 0.0, "%.4f");
        RegisterWidget(State, Field.c_str());
        if (ImGui::IsItemDeactivatedAfterEdit() && Value != Parameter.Value)
        {
            Conclude(State, AssignSolidArcParameter(State.Document, Parameter.Name, Value));
            break;
        }
    }
    const FeatureStep* Current = nullptr;                                                    // A parameter edit above may have moved Step
    for (const FeatureStep& Candidate : State.Document.Tree.Steps) if (Candidate.Label == Label) Current = &Candidate;
    if (Current && Current->Kind == FeatureStepKind::Sketch)
    {
        // Fill on / off (closed sketches only): filled loops extrude into solids, unfilled ones into walls (a sheet).
        const auto Solved = State.Document.Regeneration.Sketches.find(Label);
        const bool Closed = Solved != State.Document.Regeneration.Sketches.end() && ExtractSketchProfile(Solved->second).Refusal.empty();
        bool       Filled = Current->FillCondition;
        if (Closed)
        {
            const bool Toggled = ImGui::Checkbox("Filled  (F)###StepFill", &Filled);
            RegisterWidget(State, "StepFill");
            ImGui::SameLine();
            ImGui::TextDisabled("%s", Filled ? "E extrudes a solid" : "E extrudes walls (an open sheet)");
            if (Toggled)
            {
                Conclude(State, FillSolidArcSketch(State.Document, Label, Filled));
                EndCard();
                return;                                                                      // The step may have moved
            }
        }
        else ImGui::TextDisabled("Open curves: nothing to fill (close the loop, or use the Fill tool on crossings)");
    }
    if (PressButton(State, "Delete step  (Del)###StepDelete"))
    {
        Conclude(State, DeleteSolidArcStep(State.Document, Label));
        State.FocusedStep = {};
    }
    EndCard();
}

void DrawBodyCard(SolidArcWorkspaceState& State)
{
    const SolidArcBodySummary& Summary = State.Document.Summary;
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
            for (const SolidArcBodyFigures& Figures : Summary.Bodies)
            {
                const bool Picked = std::find(State.Document.Selection.Bodies.begin(), State.Document.Selection.Bodies.end(), Figures.Key) != State.Document.Selection.Bodies.end();
                ImGui::TextColored(Tint(Picked ? SelectedColour : (Figures.SoundCondition ? ActiveColour : RefusalColour)), "%s", Figures.Caption.c_str());
                ImGui::SameLine();
                if (Figures.SheetCondition) ImGui::TextDisabled("sheet · %d faces · %s", Figures.Faces, SolidArcBodyMatcap(State.Document.Appearance, Figures.Key).c_str());
                else ImGui::TextDisabled("%d faces · g%d · %.5f m³ · %s", Figures.Faces, Figures.Genus, Figures.Volume, SolidArcBodyMatcap(State.Document.Appearance, Figures.Key).c_str());
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
            if (PressButton(State, "Mirror x  (Alt+X)###PanelMirror", RowSpan(), Summary.SheetCondition)) MirrorPanels(State);
            ImGui::SameLine();
            if (PressButton(State, "Close → solid###PanelClose", RowSpan(), Summary.SheetCondition)) ClosePanels(State);
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
void DrawImagePlaneCard(SolidArcWorkspaceState& State)
{
    std::vector<SolidArcImagePlane>& Planes = State.Document.ImagePlanes;
    if (Planes.empty()) return;
    BeginCard("Image planes");
    bool   EditedCondition = false;
    size_t Removal         = Planes.size();
    for (size_t Index = 0; Index < Planes.size(); ++Index)
    {
        SolidArcImagePlane& Plane = Planes[Index];
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
            else ImGui::TextDisabled("%u × %u px · %.3f × %.3f m", State.Pictures.Widths[Picture], State.Pictures.Heights[Picture], GeometryProcessing::Length(Plane.Right), GeometryProcessing::Length(Plane.Down));
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

void DrawHistoryCard(SolidArcWorkspaceState& State)
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
void DrawDrawingCard(SolidArcWorkspaceState& State)
{
    SolidArcDrawingSession& Drawing = State.Drawing;
    if (!Drawing.ActiveCondition)
    {
        BeginCard("Draw");
        static const struct { const char* Label; SolidArcDrawingKind Kind; } Tools[] = {
            { "Line (Shift+A)###DrawLine", SolidArcDrawingKind::Line }, { "Rectangle (Shift+Q)###DrawRectangle", SolidArcDrawingKind::Rectangle },
            { "Circle (Shift+W)###DrawCircle", SolidArcDrawingKind::Circle }, { "Curve (Shift+S)###DrawCurve", SolidArcDrawingKind::Curve },
            { "Arc (3 points)###DrawArc", SolidArcDrawingKind::Arc }, { "Slot###DrawSlot", SolidArcDrawingKind::Slot },
            { "Polygon###DrawPolygon", SolidArcDrawingKind::Polygon } };
        for (int Index = 0; Index < 7; ++Index)
        {
            if (Index % 2 == 1) ImGui::SameLine();
            if (PressButton(State, Tools[Index].Label, RowSpan(), !State.Bevel.ActiveCondition)) OpenDrawingSession(State, Tools[Index].Kind);
        }
        if (FocusedSketchCondition(State))
        {
            const bool Consumed = SolidArcSketchConsumedCondition(State.Document, State.FocusedStep);
            if (PressButton(State, "Extrude sketch (E)###ExtrudeSketch", ImVec2(-1, 0), !State.Bevel.ActiveCondition)) OpenSketchExtrudeSession(State);
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
    if (Drawing.SolidCondition)
    {
        BeginCard(SolidArcConstructLabel(Drawing.Solid));
        ImGui::TextWrapped("%s", SolidPrompt(Drawing));
        static const std::pair<FeatureCombination, const char*> Combinations[] = { { FeatureCombination::Separate, "New body (R)###SolidSeparate" },
            { FeatureCombination::Union, "Union (Q)###SolidUnion" }, { FeatureCombination::Difference, "Difference (W)###SolidDifference" },
            { FeatureCombination::Intersection, "Intersect (E)###SolidIntersection" } };
        for (int Index = 0; Index < 4; ++Index)
        {
            if (Index % 2 == 1) ImGui::SameLine();
            if (ToggleButton(State, Combinations[Index].second, Drawing.Combination == Combinations[Index].first, RowSpan())) Drawing.Combination = Combinations[Index].first;
        }
        if (Drawing.Solid == SolidArcConstructKind::Prism)
        {
            ImGui::SetNextItemWidth(-1);
            ImGui::SliderInt("###SolidSides", &Drawing.Sides, 3, 24, "%d sides (+ / −)");
            RegisterWidget(State, "###SolidSides");
        }
        if (Drawing.Solid == SolidArcConstructKind::Block || Drawing.Solid == SolidArcConstructKind::Wedge)
        {
            if (ToggleButton(State, "Corner###SolidCorner", !Drawing.CentreCondition, RowSpan())) Drawing.CentreCondition = false;
            ImGui::SameLine();
            if (ToggleButton(State, "Centre (Alt)###SolidCentre", Drawing.CentreCondition, RowSpan())) Drawing.CentreCondition = true;
        }
        if (PressButton(State, "Exact size (Tab)###ConstructExact", RowSpan())) { OpenSolidExactForm(State); EndCard(); return; }
        ImGui::SameLine();
        if (PressButton(State, "Abort (Esc)###DrawAbort", RowSpan())) CancelDrawingSession(State);
        if (PressButton(State, "Drop point (Ctrl+Z)###DrawRetract", ImVec2(-1, 0), !Drawing.Points.empty())) RetractDrawingPoint(State);
        if (!Drawing.Refusal.empty())
        {
            ImGui::PushStyleColor(ImGuiCol_Text, Tint(RefusalColour));
            ImGui::TextWrapped("%s", Drawing.Refusal.c_str());
            ImGui::PopStyleColor();
        }
        EndCard();
        return;
    }
    BeginCard(SolidArcDrawingLabel(Drawing.Kind));
    static const char* Planes[] = { "XY", "XZ", "YZ" };
    if (Drawing.FacePlaneCondition) ImGui::Text("On face %s · %d point(s)", Drawing.FaceName.c_str(), int(Drawing.Points.size()));
    else                            ImGui::Text("Plane %s · %d point(s)", Planes[std::clamp(Drawing.GridPlane, 0, 2)], int(Drawing.Points.size()));
    if (Drawing.Kind == SolidArcDrawingKind::Curve)
    {
        static const struct { const char* Label; SketchSplineForm Form; } Forms[] = {
            { "Hermite (1)###FormHermite", SketchSplineForm::Hermite }, { "Bezier (2)###FormBezier", SketchSplineForm::Bezier },
            { "NURBS (3)###FormNurbs", SketchSplineForm::Nurbs }, { "Cubic spline (4)###FormCubic", SketchSplineForm::CubicSpline } };
        for (int Index = 0; Index < 4; ++Index)
        {
            if (Index % 2 == 1) ImGui::SameLine();
            if (ToggleButton(State, Forms[Index].Label, Drawing.Form == Forms[Index].Form, RowSpan())) { Drawing.Form = Forms[Index].Form; Drawing.Refusal.clear(); }
        }
    }
    if (Drawing.Kind == SolidArcDrawingKind::Polygon)
    {
        ImGui::SetNextItemWidth(-1);
        ImGui::SliderInt("###DrawSides", &Drawing.Sides, 3, 24, "%d sides (+ / −)");
        RegisterWidget(State, "###DrawSides");
    }
    if (Drawing.Kind == SolidArcDrawingKind::Rectangle || Drawing.Kind == SolidArcDrawingKind::Circle)
    {
        const char* Off = Drawing.Kind == SolidArcDrawingKind::Rectangle ? "Corner###ModeCorner" : "Two-point###ModeTwoPoint";
        if (ToggleButton(State, Off, !Drawing.CentreCondition, RowSpan())) Drawing.CentreCondition = false;
        ImGui::SameLine();
        if (ToggleButton(State, "Centre###ModeCentre", Drawing.CentreCondition, RowSpan())) Drawing.CentreCondition = true;
    }
    const bool Chain = DrawingClicks(Drawing.Kind) == 0;
    if (Chain)
    {
        if (PressButton(State, "Finish open (Enter)###DrawFinish", RowSpan(), Drawing.Points.size() >= 2)) FinishDrawingSession(State, false);
        ImGui::SameLine();
        if (PressButton(State, "Close###DrawClose", RowSpan(), Drawing.Points.size() >= 3)) FinishDrawingSession(State, true);
    }
    if (PressButton(State, "Drop point (Ctrl+Z)###DrawRetract", RowSpan(), !Drawing.Points.empty())) RetractDrawingPoint(State);
    ImGui::SameLine();
    if (PressButton(State, "Abort (Esc)###DrawAbort", RowSpan())) CancelDrawingSession(State);
    if (!Drawing.Refusal.empty())
    {
        ImGui::PushStyleColor(ImGuiCol_Text, Tint(RefusalColour));
        ImGui::TextWrapped("%s", Drawing.Refusal.c_str());
        ImGui::PopStyleColor();
    }
    EndCard();
}

// 📝 Object snaps while drawing or moving: master switch, per-kind filter, the active direction lock.
void DrawSnapCard(SolidArcWorkspaceState& State)
{
    const bool Moving   = State.Transform.ActiveCondition && State.Transform.Verb == SolidArcTransformVerb::Move;
    // 📝 Outside drawing and Move the card only joins while a sketch point (K) or a pole (mode 1) is actually held: the
    //    constraint rows and panel buttons below it must stay where the pointer expects them otherwise.
    const bool Dragging = (State.Constrain.ActiveCondition && State.Constrain.DragCondition) || State.PoleDrag.ActiveCondition;
    if (!State.Drawing.ActiveCondition && !Moving && !Dragging) return;
    SolidArcPointPicker& Picker = State.Picker;
    BeginCard("Snaps");
    if (ToggleButton(State, "Object snaps###SnapEnabled", Picker.Enabled, RowSpan())) Picker.Enabled = !Picker.Enabled;
    ImGui::SameLine();
    ImGui::Text("Lock %s", Picker.Lock == SolidArcSnapLockKind::None ? "none" : SolidArcSnapLockLabel(Picker.Lock));
    if (State.Drawing.ActiveCondition)
    {
        // Sketches and Construct solids drawn on a body face take its orientation (curved faces: the tangent plane there).
        if (ToggleButton(State, "Orient to faces###FaceOrient", Picker.FaceOrientEnabled, RowSpan())) Picker.FaceOrientEnabled = !Picker.FaceOrientEnabled;
        ImGui::SameLine();
        ImGui::Text("%s", State.Drawing.FacePlaneCondition ? "on a face" : "grid plane");
    }
    struct Choice { const char* Label; bool* Enabled; };
    SolidArcSnapFilter& Filter = Picker.Filter;
    const Choice Choices[] = { { "Endpoint###SnapEndpoint", &Filter.EndpointEnabled }, { "Midpoint###SnapMidpoint", &Filter.MidpointEnabled },
                               { "Centre###SnapCentre", &Filter.CentreEnabled }, { "Intersection###SnapIntersection", &Filter.IntersectionEnabled },
                               { "Tangent###SnapTangent", &Filter.TangentEnabled }, { "Perpendicular###SnapPerpendicular", &Filter.PerpendicularEnabled },
                               { "On curve###SnapCurve", &Filter.CurveEnabled }, { "On face (Move)###SnapFace", &Filter.FaceEnabled } };
    ImGui::BeginDisabled(!Picker.Enabled);
    for (int Index = 0; Index < 8; ++Index)
    {
        if (Index % 2 == 1) ImGui::SameLine();
        if (ToggleButton(State, Choices[Index].Label, *Choices[Index].Enabled, RowSpan())) *Choices[Index].Enabled = !*Choices[Index].Enabled;
    }
    ImGui::EndDisabled();
    ImGui::PushStyleColor(ImGuiCol_Text, Tint(MutedColour));
    ImGui::TextWrapped(State.Drawing.ActiveCondition ? "x y z t n b lock a direction through the last point (again releases) · s squares a rectangle · Ctrl held: ends / midpoints / centres (free where none) · Alt held: along a line / curve / arc"
                       : Moving ? "The grab point snaps, then lands on the snap under the pointer · X / Y / Z still constrain · Ctrl held: free"
                       : State.Constrain.ActiveCondition ? "A dragged sketch point lands on points and in-plane curves of other geometry, else the grid · Ctrl held: important points (free where none) · Alt held: along a curve"
                                                         : "A dragged pole lands on points, curves and edges of other geometry · Ctrl held: free");
    ImGui::PopStyleColor();
    EndCard();
}

//------------------------------------------------------------------------------------------------------------------------
//                                                 INSPECTOR LAYOUT
//------------------------------------------------------------------------------------------------------------------------

void OpenConstruct(SolidArcWorkspaceState& State, SolidArcConstructKind Kind);

constexpr float CardSpacing   = 10.0f;                                                       // [px] - .inspector-card margin-bottom
constexpr float CardRounding  = 22.0f;                                                       // [px] - .inspector-card border-radius
constexpr float CardInset     = 18.0f;                                                       // [px] - .inspector-card padding
constexpr float ParagraphLine = 9.0f * 1.85f;                                                // [px] - .inspector-card p line box (9 px × 1.85)

// Words of Text broken into lines no wider than Width at CssSize (CSS white-space: normal).
std::vector<std::string> WrapLines(SolidArcSheet& Sheet, const std::string& Text, float CssSize, float Width)
{
    std::vector<std::string> Lines;
    std::string              Line;
    std::istringstream       Words(Text);
    std::string              Word;
    while (Words >> Word)
    {
        const std::string Candidate = Line.empty() ? Word : Line + " " + Word;
        if (!Line.empty() && Sheet.Measure(Candidate.c_str(), CssSize) > Width)
        {
            Lines.push_back(Line);
            Line = Word;
        }
        else Line = Candidate;
    }
    if (!Line.empty()) Lines.push_back(Line);
    return Lines;
}

// .inspector-card p — 9 px #888 on 1.85 line boxes; returns the height drawn.
float DrawParagraph(SolidArcSheet& Sheet, float Left, float Top, float Width, const std::string& Text, bool CentredCondition = false)
{
    const std::vector<std::string> Lines = WrapLines(Sheet, Text, 9.0f, Width);
    for (size_t Index = 0; Index < Lines.size(); ++Index)
    {
        const float Centre = Top + (float(Index) + 0.5f) * ParagraphLine;
        const float Offset = CentredCondition ? 0.5f * (Width - Sheet.Measure(Lines[Index].c_str(), 9.0f)) : 0.0f;
        Sheet.Text(Left + Offset, Centre, 9.0f, SolidArcInk(0x888888), Lines[Index].c_str());
    }
    return float(Lines.size()) * ParagraphLine;
}

float MeasureParagraph(SolidArcSheet& Sheet, float Width, const std::string& Text)
{
    return float(WrapLines(Sheet, Text, 9.0f, Width).size()) * ParagraphLine;
}

// 📝 A card drawn by the sheet sits in the ImGui flow: its box is drawn at the cursor and a dummy of its height follows
//    (+1, so the 9 px item spacing makes the 10 px card margin).
PlaneExtent PlaceSheetCard(SolidArcSheet& Sheet, float Height)
{
    const ImVec2      Origin = ImGui::GetCursorScreenPos();
    const PlaneExtent Card   = Spanning(Origin.x, Origin.y, ImGui::GetContentRegionAvail().x, Height);
    Sheet.Box(Card, SolidArcInk(0x1a1a1a), CardRounding, SolidArcWash(0x03));
    return Card;
}

void CloseSheetCard(const PlaneExtent& Card)
{
    ImGui::Dummy(ImVec2(Card.Width(), Card.Height() + CardSpacing - ImGui::GetStyle().ItemSpacing.y));
}

// 📝 HTML .operation-button — 62 px, #212121, #ffffff05 hairline, radius 13; glyph 19 #c3c3c3 over a 9 px #aaa caption.
bool OperationButton(SolidArcWorkspaceState& State, SolidArcSheet& Sheet, const std::string& Key, const PlaneExtent& Box, IconSymbol Glyph,
                     const char* Caption, bool EnabledCondition = true)
{
    const SolidArcPress Press   = PressSolidArcRegion(State.Widgets, Key, Box, EnabledCondition);
    const float         Opacity = EnabledCondition ? 1.0f : 0.3f;
    Sheet.Box(Box, ControlKit::Faded(SolidArcInk(Press.HoveredCondition ? 0x292929 : 0x212121), Opacity), 13.0f, ControlKit::Faded(SolidArcWash(0x05), Opacity));
    const float Stack    = 19.0f + 9.0f + 9.0f * SolidArcStyleTokens::FontScale;
    const float GlyphTop = Box.MinimumY + 0.5f * (Box.Height() - Stack);
    const float Middle   = 0.5f * (Box.MinimumX + Box.MaximumX);
    Sheet.Icon(Glyph, Middle - 9.5f, GlyphTop, 19.0f, ControlKit::Faded(SolidArcInk(Press.HoveredCondition ? 0xffffff : 0xc3c3c3), Opacity));
    const float Width = Sheet.Measure(Caption, 9.0f);
    Sheet.Text(Middle - 0.5f * Width, GlyphTop + 19.0f + 9.0f + 0.5f * 9.0f * SolidArcStyleTokens::FontScale, 9.0f,
               ControlKit::Faded(SolidArcInk(Press.HoveredCondition ? 0xffffff : 0xaaaaaa), Opacity), Caption);
    return Press.ClickedCondition;
}

struct InspectorOperation
{
    std::string Key      = {};                                                               // [-] - Widget key after "Operation:"
    IconSymbol  Glyph    = IconSymbol::SolidArcBody;                                         // [-] - 19 px glyph
    std::string Caption  = {};                                                               // [-] - Caption under it
    bool        Enabled  = true;                                                             // [-] - Clickable
};

// .operation-grid — 3 columns, gap 7, 62 px rows. Returns the key pressed (empty when none).
std::string DrawOperationGrid(SolidArcWorkspaceState& State, SolidArcSheet& Sheet, float Left, float Top, float Width, const std::vector<InspectorOperation>& Operations)
{
    std::string Pressed;
    const float Column = (Width - 2.0f * 7.0f) / 3.0f;
    for (size_t Index = 0; Index < Operations.size(); ++Index)
    {
        const PlaneExtent Box = Spanning(Left + float(Index % 3) * (Column + 7.0f), Top + float(Index / 3) * (62.0f + 7.0f), Column, 62.0f);
        if (OperationButton(State, Sheet, "Operation:" + Operations[Index].Key, Box, Operations[Index].Glyph, Operations[Index].Caption.c_str(), Operations[Index].Enabled))
            Pressed = Operations[Index].Key;
    }
    return Pressed;
}

float OperationGridHeight(size_t Count)
{
    const size_t Rows = (Count + 2) / 3;
    return Rows == 0 ? 0.0f : float(Rows) * 62.0f + float(Rows - 1) * 7.0f;
}

void RunOperation(SolidArcWorkspaceState& State, const std::string& Key)
{
    if (Key == "Fillet") OpenBevelSession(State, BlendProfile::Fillet);
    else if (Key == "Chamfer") OpenBevelSession(State, BlendProfile::Chamfer);
    else if (Key == "ExtrudeFaces") OpenExtrudeSession(State);
    else if (Key == "Inset") OpenInsetSession(State);
    else if (Key == "ExtrudeSketch") OpenSketchExtrudeSession(State);
    else if (Key == "Revolve") OpenSweepSession(State, SolidArcSweepVerb::Revolve);
    else if (Key == "Loft") LoftSelectedCurves(State);
    else if (Key == "Join") JoinSketches(State);
    else if (Key == "FillSketch") ToggleSketchFill(State);
    else if (Key == "FlipNormals") FlipPickedNormals(State);
    else if (Key == "Move") OpenTransformSession(State, SolidArcTransformVerb::Move, false);
    else if (Key == "Rotate") OpenTransformSession(State, SolidArcTransformVerb::Rotate, false);
    else if (Key == "Scale") OpenTransformSession(State, SolidArcTransformVerb::Scale, false);
    else if (Key == "Block") OpenConstruct(State, SolidArcConstructKind::Block);
    else if (Key == "Circle") OpenDrawingSession(State, SolidArcDrawingKind::Circle);
    else if (Key == "Rectangle") OpenDrawingSession(State, SolidArcDrawingKind::Rectangle);
    else if (Key == "Extrude")
    {
        if (State.Document.Selection.Faces.empty() && FocusedSketchCondition(State)) OpenSketchExtrudeSession(State);
        else OpenExtrudeSession(State);
    }
    else if (Key == "Union" || Key == "Subtract" || Key == "Intersect")
    {
        State.Boolean.Combination = Key == "Union" ? FeatureCombination::Union : Key == "Subtract" ? FeatureCombination::Difference : FeatureCombination::Intersection;
        ApplyBooleanSession(State);
    }
}

// The bodies the selection touches, in pick order: body picks, then the owners of picked faces and edges.
std::vector<std::string> TouchedBodies(const SolidArcDocument& Document)
{
    std::vector<std::string> Keys = Document.Selection.Bodies;
    const auto Touch = [&](SolidArcSelectionKind Kind, const std::string& Name)
    {
        const std::string Key = SolidArcBodyOfName(Document, Kind, Name);
        if (!Key.empty() && std::find(Keys.begin(), Keys.end(), Key) == Keys.end()) Keys.push_back(Key);
    };
    for (const std::string& Face : Document.Selection.Faces) Touch(SolidArcSelectionKind::Face, Face);
    for (const std::string& Edge : Document.Selection.Edges) Touch(SolidArcSelectionKind::Edge, Edge);
    return Keys;
}

const SolidArcBodyFigures* FiguresOf(const SolidArcDocument& Document, const std::string& Key)
{
    for (const SolidArcBodyFigures& Figures : Document.Summary.Bodies)
        if (Figures.Key == Key) return &Figures;
    return nullptr;
}

// 📝 HTML .intro-card (nothing picked): body glyph 35 #777, "Select geometry", the hint; then "Start constructing" with
//    Block · Circle · Rectangle · Extrude.
void DrawIntroCards(SolidArcWorkspaceState& State, SolidArcSheet& Sheet)
{
    const float  Width   = ImGui::GetContentRegionAvail().x;
    const float  Inner   = Width - 44.0f;
    const std::string Hint = "Pick a body, sketch or surface in the viewport or outliner to inspect its native geometry.";
    const float  HintHeight = MeasureParagraph(Sheet, Inner, Hint);
    const float  Height  = 30.0f + 35.0f + 20.0f + 18.0f * SolidArcStyleTokens::FontScale + 15.0f + HintHeight + 30.0f;
    const PlaneExtent Intro = PlaceSheetCard(Sheet, Height);
    float Y = Intro.MinimumY + 30.0f;
    Sheet.Icon(IconSymbol::SolidArcBody, 0.5f * (Intro.MinimumX + Intro.MaximumX) - 17.5f, Y, 35.0f, SolidArcInk(0x777777));
    Y += 35.0f + 20.0f;
    const float TitleWidth = Sheet.Measure("Select geometry", 18.0f, SolidArcWeight::Light, -0.6f);
    Sheet.Text(0.5f * (Intro.MinimumX + Intro.MaximumX - TitleWidth), Y + 0.5f * 18.0f * SolidArcStyleTokens::FontScale, 18.0f, SolidArcInk(0xeeeeee),
               "Select geometry", SolidArcWeight::Light, -0.6f);
    Y += 18.0f * SolidArcStyleTokens::FontScale + 15.0f;
    DrawParagraph(Sheet, Intro.MinimumX + 22.0f, Y, Inner, Hint, true);
    CloseSheetCard(Intro);

    const std::vector<InspectorOperation> Operations = { { "Block", IconSymbol::SolidArcBody, "Block" }, { "Circle", IconSymbol::SolidArcCircle, "Circle" },
                                                         { "Rectangle", IconSymbol::SolidArcRectangle, "Rectangle" },
                                                         { "Extrude", IconSymbol::SolidArcExtrude, "Extrude", FocusedSketchCondition(State) } };
    const std::string Lead       = "Create a profile, then extrude it into a solid.";
    const float       CardInner  = Width - 2.0f * CardInset;
    const float       LeadHeight = MeasureParagraph(Sheet, CardInner, Lead);
    const PlaneExtent Start = PlaceSheetCard(Sheet, 2.0f * CardInset + 17.0f * SolidArcStyleTokens::FontScale + LeadHeight + 16.0f + OperationGridHeight(Operations.size()));
    const float Left = Start.MinimumX + CardInset;
    Y = Start.MinimumY + CardInset;
    Sheet.Text(Left, Y + 0.5f * 17.0f * SolidArcStyleTokens::FontScale, 17.0f, SolidArcInk(0xeeeeee), "Start constructing");
    Y += 17.0f * SolidArcStyleTokens::FontScale;
    Y += DrawParagraph(Sheet, Left, Y, CardInner, Lead) + 16.0f;
    const std::string Pressed = DrawOperationGrid(State, Sheet, Left, Y, CardInner, Operations);
    CloseSheetCard(Start);
    if (!Pressed.empty()) RunOperation(State, Pressed);
}

// 📝 HTML .selection-card: the 39 px glyph disc (amber body, blue sheet, cyan curve), the name (20 px Light), the class
//    line (8 px, letter-spaced) and the eye; then three count tiles (9 px caption, 22 px Light value).
void DrawSelectionSummary(SolidArcWorkspaceState& State, SolidArcSheet& Sheet)
{
    const SolidArcSelection&       Selection = State.Document.Selection;
    const std::vector<std::string> Bodies    = TouchedBodies(State.Document);
    IconSymbol  Glyph = IconSymbol::SolidArcBody;
    uint32_t    Ink   = 0xe5b369;
    std::string Name, Kind, BodyKey;
    std::array<std::pair<const char*, std::string>, 3> Counts = {};
    int Others = 0;
    if (!Bodies.empty())
    {
        BodyKey = Selection.LastKind == SolidArcSelectionKind::Face && !Selection.Faces.empty()   ? SolidArcBodyOfName(State.Document, SolidArcSelectionKind::Face, Selection.Faces.back())
                : Selection.LastKind == SolidArcSelectionKind::Edge && !Selection.Edges.empty()   ? SolidArcBodyOfName(State.Document, SolidArcSelectionKind::Edge, Selection.Edges.back())
                                                                                                  : Bodies.front();
        if (BodyKey.empty()) BodyKey = Bodies.front();
        const SolidArcBodyFigures* Figures = FiguresOf(State.Document, BodyKey);
        const bool Sheeted = Figures && Figures->SheetCondition;
        Glyph = Sheeted ? IconSymbol::SolidArcSurface : IconSymbol::SolidArcBody;
        Ink   = Sheeted ? 0x7ba9d9 : 0xe5b369;
        Name  = SolidArcBodyCaption(State.Document, BodyKey);
        Kind  = Sheeted ? "OPEN SHEET" : "SOLID BODY";
        if (!Selection.Faces.empty()) Kind += Compose(" · %d FACE%s", int(Selection.Faces.size()), Selection.Faces.size() == 1 ? "" : "S");
        if (!Selection.Edges.empty()) Kind += Compose(" · %d EDGE%s", int(Selection.Edges.size()), Selection.Edges.size() == 1 ? "" : "S");
        Counts = { { { "Faces", std::to_string(Figures ? Figures->Faces : 0) }, { "Edges", std::to_string(Figures ? Figures->Edges : 0) },
                     { "Vertices", std::to_string(Figures ? Figures->Vertices : 0) } } };
        Others = int(Bodies.size()) - 1 + int(Selection.Curves.size());
    }
    else
    {
        const std::string&     Label = Selection.Curves.back();
        const CurveFitOutcome* Fit   = FindFittedCurve(State.Document, Label);
        Glyph  = IconSymbol::SolidArcNurbs;
        Ink    = 0x84bdc2;
        Name   = CaptionOf(State.Document, Label);
        Kind   = "SPACE CURVE · NURBS";
        Counts = { { { "Poles", Fit ? std::to_string(Fit->Curve.Definition->Poles.size()) : std::string("-") },
                     { "Degree", Fit ? std::to_string(Fit->Curve.Definition->Degree) : std::string("-") },
                     { "Fit mm", Fit ? Compose("%.2f", Fit->WorstDeviation * 1000.0) : std::string("-") } } };
        const auto Solved = State.Document.Regeneration.Sketches.find(Label);
        if (!Fit && Solved != State.Document.Regeneration.Sketches.end())
        {
            // A picked sketch (loft section / gizmo target): its curves, points and constraints.
            Kind   = "SKETCH · LOFT SECTION";
            Counts = { { { "Curves", std::to_string(Solved->second.Curves.size()) }, { "Points", std::to_string(Solved->second.Points.size()) },
                         { "Constraints", std::to_string(Solved->second.Constraints.size()) } } };
        }
        Others = int(Selection.Curves.size()) - 1;
    }
    if (Others > 0) Kind += Compose(" · +%d SELECTED", Others);

    // .selection-card padding 19 17; heading 45 px (name line + 9 + class line), margin 20; tiles 67 px.
    const float NameLine  = 20.0f * SolidArcStyleTokens::FontScale;
    const float ClassLine = 8.0f * SolidArcStyleTokens::FontScale;
    const float Heading   = std::max(39.0f, NameLine + 9.0f + ClassLine);
    const float TileValue = 22.0f * SolidArcStyleTokens::FontScale;
    const float TileHeight = 2.0f + 20.0f + 9.0f * SolidArcStyleTokens::FontScale + 5.0f + TileValue;
    const PlaneExtent Card = PlaceSheetCard(Sheet, 19.0f + Heading + 20.0f + TileHeight + 19.0f);
    const float Left  = Card.MinimumX + 17.0f;
    const float Right = Card.MaximumX - 17.0f;
    const float Top   = Card.MinimumY + 19.0f;
    const float Middle = Top + 0.5f * Heading;
    Sheet.Surface().FillRectangle(Spanning(Left, Middle - 19.5f, 39.0f, 39.0f), SolidArcWash(0x04), 19.5f);
    Sheet.Icon(Glyph, Left + 8.5f, Middle - 11.0f, 22.0f, SolidArcInk(Ink));
    const float TextLeft  = Left + 39.0f + 10.0f;
    const float TextRight = Right - (BodyKey.empty() ? 0.0f : 28.0f + 10.0f);
    const float NameTop   = Middle - 0.5f * (NameLine + 9.0f + ClassLine);
    Sheet.TextClipped(TextLeft, NameTop + 0.5f * NameLine, TextRight - TextLeft, 20.0f, SolidArcInk(0xeeeeee), Name.c_str());
    Sheet.TextClipped(TextLeft, NameTop + NameLine + 9.0f + 0.5f * ClassLine, TextRight - TextLeft, 8.0f, SolidArcInk(0x777777), Kind.c_str());
    if (!BodyKey.empty())
    {
        const bool Hidden = State.Document.Appearance.HiddenBodies.contains(BodyKey);
        if (SheetIconButton(State, Sheet, "SelectedVisibility", Right - 14.0f, Middle, 28.0f, Hidden ? IconSymbol::SolidArcHidden : IconSymbol::SolidArcEye, 17.0f))
        {
            const SolidArcCommandOutcome Outcome = ToggleSolidArcBodyVisibility(State.Document, BodyKey);
            if (Outcome.AcceptedCondition) RefreshSolidArcScene(State, true);
            Announce(State, Outcome.AcceptedCondition ? Name + (Hidden ? " shown" : " hidden") : "Refused: " + Outcome.Refusal);
        }
    }
    // .geometry-counts — 3 columns, gap 7; tile #ffffff03, #ffffff06 hairline, radius 14, padding 10.
    const float TilesTop = Top + Heading + 20.0f;
    const float Column   = (Right - Left - 14.0f) / 3.0f;
    for (int Index = 0; Index < 3; ++Index)
    {
        const PlaneExtent Tile = Spanning(Left + float(Index) * (Column + 7.0f), TilesTop, Column, TileHeight);
        Sheet.Box(Tile, SolidArcWash(0x03), 14.0f, SolidArcWash(0x06));
        Sheet.Text(Tile.MinimumX + 11.0f, Tile.MinimumY + 11.0f + 0.5f * 9.0f * SolidArcStyleTokens::FontScale, 9.0f, SolidArcInk(0x888888), Counts[size_t(Index)].first);
        Sheet.Text(Tile.MinimumX + 11.0f, Tile.MaximumY - 11.0f - 0.5f * TileValue, 22.0f, SolidArcInk(0xeeeeee), Counts[size_t(Index)].second.c_str());
    }
    CloseSheetCard(Card);
}

// 📝 HTML .ordered-selection (two or more bodies): numbered rows in pick order — the first is the Boolean target (ACTIVE,
//    green), the rest are tools (SECTION, blue) — Reverse order, then Union / Subtract / Intersect and the operand note.
void DrawOrderedSelection(SolidArcWorkspaceState& State, SolidArcSheet& Sheet)
{
    std::vector<std::string>& Picked = State.Document.Selection.Bodies;
    const float       Width = ImGui::GetContentRegionAvail().x - 2.0f * CardInset;
    const std::string Note  = "3D booleans: first minus the rest; Reverse order swaps operands. Refused results leave the document untouched; Undo restores the inputs.";
    const float       Row   = 13.0f * SolidArcStyleTokens::FontScale + 16.0f;
    const float       Height = 2.0f * CardInset + CardHeadingHeight + float(Picked.size()) * Row + 14.0f + 35.0f + 14.0f + 17.0f * SolidArcStyleTokens::FontScale + 10.0f +
                               OperationGridHeight(3) + 12.0f + MeasureParagraph(Sheet, Width, Note);
    const PlaneExtent Card = PlaceSheetCard(Sheet, Height);
    const float Left = Card.MinimumX + CardInset;
    float       Y    = Card.MinimumY + CardInset;
    DrawCardHeading(Sheet, Left, Y, Width, Compose("%d bodies selected", int(Picked.size())).c_str(), "CLICK ORDER");
    Y += CardHeadingHeight;
    for (size_t Index = 0; Index < Picked.size(); ++Index)
    {
        const bool     Active = Index == 0;
        const uint32_t Ink    = Active ? 0x33ff73 : 0x4099ff;
        const float    Centre = Y + 0.5f * Row;
        const float    Number = Sheet.Text(Left, Centre, 13.0f, SolidArcInk(Ink), std::to_string(Index + 1).c_str());
        const float    TagWidth = Sheet.TextRight(Left + Width, Centre, 9.0f, SolidArcInk(Ink), Active ? "ACTIVE" : "SECTION");
        Sheet.TextClipped(Left + Number + 8.0f, Centre, Width - Number - 16.0f - TagWidth, 13.0f, SolidArcInk(Ink), SolidArcBodyCaption(State.Document, Picked[Index]).c_str(),
                          SolidArcWeight::Regular);
        Sheet.Line(Left, Y + Row - 0.5f, Left + Width, Y + Row - 0.5f, SolidArcWash(0x0d));
        Y += Row;
    }
    Y += 14.0f;
    if (SheetButton(State, Sheet, "ReverseOrder", Left, Y + 17.5f, "Reverse order"))
    {
        std::reverse(Picked.begin(), Picked.end());
        Announce(State, "Order reversed: " + SolidArcBodyCaption(State.Document, Picked.front()) + " is the target");
    }
    Y += 35.0f + 14.0f;
    Sheet.Text(Left, Y + 0.5f * 17.0f * SolidArcStyleTokens::FontScale, 17.0f, SolidArcInk(0xeeeeee), "Compatible operations");
    Y += 17.0f * SolidArcStyleTokens::FontScale + 10.0f;
    const std::string Pressed = DrawOperationGrid(State, Sheet, Left, Y, Width, { { "Union", IconSymbol::SolidArcUnion, "Union" }, { "Subtract", IconSymbol::SolidArcSubtract, "Subtract" },
                                                                                 { "Intersect", IconSymbol::SolidArcIntersect, "Intersect" } });
    Y += OperationGridHeight(3) + 12.0f;
    DrawParagraph(Sheet, Left, Y, Width, Note);
    CloseSheetCard(Card);
    if (!Pressed.empty()) RunOperation(State, Pressed);
}

// 📝 HTML Bevel history (EDITABLE FEATURES): every fillet / chamfer step (click focuses it), then the Bevel / Chamfer · B button.
void DrawBevelHistory(SolidArcWorkspaceState& State, SolidArcSheet& Sheet)
{
    std::vector<const FeatureStep*> Blends;
    for (const FeatureStep& Step : State.Document.Tree.Steps)
        if (Step.Kind == FeatureStepKind::Blend) Blends.push_back(&Step);
    const float       Width = ImGui::GetContentRegionAvail().x - 2.0f * CardInset;
    const std::string Empty = "No bevels yet. Pick solid edges (mode 2), then B.";
    const float       Row   = 11.0f * SolidArcStyleTokens::FontScale + 14.0f;
    const float       Body  = Blends.empty() ? MeasureParagraph(Sheet, Width, Empty) + 16.0f : float(Blends.size()) * Row + 14.0f;
    const PlaneExtent Card  = PlaceSheetCard(Sheet, 2.0f * CardInset + CardHeadingHeight + Body + 35.0f);
    const float Left = Card.MinimumX + CardInset;
    float       Y    = Card.MinimumY + CardInset;
    DrawCardHeading(Sheet, Left, Y, Width, "Bevel history", "EDITABLE FEATURES");
    Y += CardHeadingHeight;
    if (Blends.empty()) Y += DrawParagraph(Sheet, Left, Y, Width, Empty) + 16.0f;
    for (const FeatureStep* Step : Blends)
    {
        const PlaneExtent   Line    = Spanning(Left - 8.0f, Y, Width + 16.0f, Row);
        const SolidArcPress Press   = PressSolidArcRegion(State.Widgets, "Bevel:" + Step->Label, Line);
        const bool          Focused = State.FocusedStep == Step->Label;
        if (Press.HoveredCondition || Focused) Sheet.Box(Line, Focused ? SolidArcWash(0x10) : SolidArcInk(0x222222), 10.0f);
        Sheet.Icon(Step->Shape == BlendProfile::Fillet ? IconSymbol::SolidArcFillet : IconSymbol::SolidArcChamfer, Left, Y + 0.5f * Row - 7.0f, 14.0f, SolidArcInk(0xb7a68c));
        Sheet.TextClipped(Left + 22.0f, Y + 0.5f * Row, Width - 22.0f, 11.0f, SolidArcInk(Focused ? 0xffffff : 0xcccccc), DescribeSolidArcStep(State.Document, *Step).c_str());
        if (Press.ClickedCondition) State.FocusedStep = Focused ? std::string() : Step->Label;
        Y += Row;
    }
    if (!Blends.empty()) Y += 14.0f;
    const bool Edges = !State.Document.Selection.Edges.empty() && !State.Bevel.ActiveCondition;
    if (SheetButton(State, Sheet, "BevelLauncher", Left, Y + 17.5f, "Bevel / Chamfer · B", Edges)) OpenBevelSession(State, BlendProfile::Fillet);
    CloseSheetCard(Card);
}

// 📝 HTML .inspector-tabs — #070707 pill (34 px, #ffffff08 hairline, padding 3, margin 13 2 16); tabs 10 px, active #292929.
void DrawInspectorTabs(SolidArcWorkspaceState& State, SolidArcSheet& Sheet)
{
    const ImVec2      Origin = ImGui::GetCursorScreenPos();
    const float       Spacing = ImGui::GetStyle().ItemSpacing.y;
    const PlaneExtent Bar    = Spanning(Origin.x + 2.0f, Origin.y + 13.0f - CardSpacing, ImGui::GetContentRegionAvail().x - 4.0f, 34.0f);
    Sheet.Box(Bar, SolidArcInk(0x070707), 30.0f, SolidArcWash(0x08));
    const struct { SolidArcInspectorTab Tab; const char* Label; const char* Key; } Tabs[] = {
        { SolidArcInspectorTab::Geometry, "Geometry", "InspectorGeometry" }, { SolidArcInspectorTab::Transform, "Transform", "InspectorTransform" },
        { SolidArcInspectorTab::Topology, "Topology", "InspectorTopology" } };
    float Widths[3], Total = 0.0f;
    for (int Index = 0; Index < 3; ++Index) Total += Widths[Index] = 32.0f + Sheet.Measure(Tabs[Index].Label, 10.0f);
    const float Gap    = (Bar.Width() - 2.0f - 6.0f - Total) / 2.0f;                         // justify-content: space-between
    float       Cursor = Bar.MinimumX + 1.0f + 3.0f;
    const float Height = 10.0f * SolidArcStyleTokens::FontScale + 12.0f;
    for (int Index = 0; Index < 3; ++Index)
    {
        const PlaneExtent   Button = Spanning(Cursor, 0.5f * (Bar.MinimumY + Bar.MaximumY) - 0.5f * Height, Widths[Index], Height);
        const SolidArcPress Press  = PressSolidArcRegion(State.Widgets, Tabs[Index].Key, Button);
        const bool          Active = State.InspectorTab == Tabs[Index].Tab;
        if (Active || Press.HoveredCondition) Sheet.Box(Button, SolidArcInk(0x292929), 30.0f);
        Sheet.Text(Button.MinimumX + 16.0f, 0.5f * (Button.MinimumY + Button.MaximumY), 10.0f, SolidArcInk(Active ? 0xeeeeee : Press.HoveredCondition ? 0xffffff : 0x858585),
                   Tabs[Index].Label);
        if (Press.ClickedCondition) State.InspectorTab = Tabs[Index].Tab;
        Cursor += Widths[Index] + Gap;
    }
    ImGui::Dummy(ImVec2(Bar.Width(), Bar.MaximumY + 16.0f - Origin.y - Spacing));
}

// World bounds of the picked bodies (else every shown body) and of the picked curves' poles.
bool SelectionBounds(const SolidArcWorkspaceState& State, std::array<double, 3>& Low, std::array<double, 3>& High)
{
    Low  = { 1.0e300, 1.0e300, 1.0e300 };
    High = { -1.0e300, -1.0e300, -1.0e300 };
    bool Found = false;
    const auto Include = [&](double X, double Y, double Z)
    {
        const double Point[3] = { X, Y, Z };
        for (int Axis = 0; Axis < 3; ++Axis)
        {
            Low[size_t(Axis)]  = std::min(Low[size_t(Axis)], Point[Axis]);
            High[size_t(Axis)] = std::max(High[size_t(Axis)], Point[Axis]);
        }
        Found = true;
    };
    const std::vector<std::string> Bodies = TouchedBodies(State.Document);
    const std::set<std::string>    Keys(Bodies.begin(), Bodies.end());
    for (const SolidArcSceneFace& Face : State.Scene.Faces)
    {
        if (!Keys.empty() && (Face.Body >= State.Scene.Bodies.size() || !Keys.contains(State.Scene.Bodies[Face.Body].Key))) continue;
        for (uint32_t Corner = Face.FirstCorner; Corner < Face.FirstCorner + Face.CornerCount; ++Corner)
        {
            const float* Point = State.Scene.Corners[Corner].Position;
            Include(Point[0], Point[1], Point[2]);
        }
    }
    for (const std::string& Label : State.Document.Selection.Curves)
        if (const CurveFitOutcome* Fit = FindFittedCurve(State.Document, Label))
            for (const Vector3& Pole : Fit->Curve.Definition->Poles) Include(Pole.X, Pole.Y, Pole.Z);
    return Found;
}

// 📝 HTML Bounds (WORLD · m): three cells (gap 8, 33 px, #101010, radius 12, padding 0 9) — axis letter 10 px in its axis
//    colour, the extent 11 px, the unit 8 px.
void DrawBoundsCard(SolidArcWorkspaceState& State, SolidArcSheet& Sheet, const char* Title)
{
    std::array<double, 3> Low, High;
    if (!SelectionBounds(State, Low, High)) return;
    const float       Width = ImGui::GetContentRegionAvail().x - 2.0f * CardInset;
    const PlaneExtent Card  = PlaceSheetCard(Sheet, 2.0f * CardInset + CardHeadingHeight + 33.0f);
    const float Left = Card.MinimumX + CardInset;
    const float Top  = Card.MinimumY + CardInset;
    DrawCardHeading(Sheet, Left, Top, Width, Title, "WORLD · m");
    const float Column = (Width - 16.0f) / 3.0f;
    const struct { const char* Axis; uint32_t Ink; } Axes[3] = { { "X", 0xe88585 }, { "Y", 0xa3c78e }, { "Z", 0x84aad5 } };
    for (int Index = 0; Index < 3; ++Index)
    {
        const PlaneExtent Cell   = Spanning(Left + float(Index) * (Column + 8.0f), Top + CardHeadingHeight, Column, 33.0f);
        const float       Centre = 0.5f * (Cell.MinimumY + Cell.MaximumY);
        Sheet.Box(Cell, SolidArcInk(0x101010), 12.0f);
        Sheet.Text(Cell.MinimumX + 9.0f, Centre, 10.0f, SolidArcInk(Axes[Index].Ink), Axes[Index].Axis);
        const float Unit = Sheet.TextRight(Cell.MaximumX - 9.0f, Centre, 8.0f, SolidArcInk(0x888888), "m");
        const std::string Value = Compose("%.4g", High[size_t(Index)] - Low[size_t(Index)]);
        Sheet.TextRight(Cell.MaximumX - 9.0f - Unit - 6.0f, Centre, 11.0f, SolidArcInk(0xeeeeee), Value.c_str());
    }
    CloseSheetCard(Card);
}

// 📝 HTML Operations (NATIVE SOLVERS): the commands this selection takes, then "n faces · m edges selected".
void DrawOperationsCard(SolidArcWorkspaceState& State, SolidArcSheet& Sheet)
{
    const SolidArcSelection&        Selection = State.Document.Selection;
    std::vector<InspectorOperation> Operations;
    if (!Selection.Edges.empty() || !CornerTargets(State).Sketches.empty())
    {
        // Edges: solid blends; otherwise sketch corners (picked mode-1 points or picked / focused sketches).
        Operations.push_back({ "Fillet", IconSymbol::SolidArcFillet, "Fillet · B" });
        Operations.push_back({ "Chamfer", IconSymbol::SolidArcChamfer, "Chamfer" });
    }
    if (!Selection.Faces.empty())
    {
        Operations.push_back({ "ExtrudeFaces", IconSymbol::SolidArcExtrude, "Extrude · E" });
        Operations.push_back({ "Inset", IconSymbol::SolidArcInset, "Inset · O / I" });
    }
    if (Selection.Faces.empty() && FocusedSketchCondition(State))
    {
        Operations.push_back({ "ExtrudeSketch", IconSymbol::SolidArcExtrude, "Extrude · E" });
        Operations.push_back({ "Revolve", IconSymbol::SolidArcRotate, "Revolve" });
        const auto Solved = State.Document.Regeneration.Sketches.find(State.FocusedStep);
        if (Solved != State.Document.Regeneration.Sketches.end() && ExtractSketchProfile(Solved->second).Refusal.empty())
        {
            bool Filled = false;
            for (const FeatureStep& Step : State.Document.Tree.Steps) if (Step.Label == State.FocusedStep) Filled = Step.FillCondition;
            Operations.push_back({ "FillSketch", IconSymbol::SolidArcFill, Filled ? "Unfill · F" : "Fill · F" });
        }
    }
    {
        bool SheetCondition = false;
        for (const std::string& Key : TouchedBodies(State.Document))
            if (const SolidArcBodyFigures* Figures = FiguresOf(State.Document, Key)) SheetCondition = SheetCondition || Figures->SheetCondition;
        if (SheetCondition) Operations.push_back({ "FlipNormals", IconSymbol::SolidArcFlip, "Flip normals · Alt+N" });
    }
    if (Selection.Curves.size() >= 2) Operations.push_back({ "Loft", IconSymbol::SolidArcSurface, "Loft · L" });
    if (SelectedSketchLabels(State.Document).size() >= 2) Operations.push_back({ "Join", IconSymbol::SolidArcLink, "Join · J" });
    if (!Selection.Bodies.empty())
    {
        Operations.push_back({ "Move", IconSymbol::SolidArcMove, "Move · G" });
        Operations.push_back({ "Rotate", IconSymbol::SolidArcRotate, "Rotate · R" });
        Operations.push_back({ "Scale", IconSymbol::SolidArcMaximize, "Scale · S" });
    }
    if (Operations.empty() || Selection.Bodies.size() >= 2) return;
    for (InspectorOperation& Operation : Operations) Operation.Enabled = !State.Bevel.ActiveCondition;
    const float       Width = ImGui::GetContentRegionAvail().x - 2.0f * CardInset;
    const std::string Note  = Compose("%d face%s · %d edge%s selected", int(Selection.Faces.size()), Selection.Faces.size() == 1 ? "" : "s", int(Selection.Edges.size()), Selection.Edges.size() == 1 ? "" : "s");
    const PlaneExtent Card  = PlaceSheetCard(Sheet, 2.0f * CardInset + CardHeadingHeight + OperationGridHeight(Operations.size()) + 15.0f + ParagraphLine);
    const float Left = Card.MinimumX + CardInset;
    float       Y    = Card.MinimumY + CardInset;
    DrawCardHeading(Sheet, Left, Y, Width, "Operations", "NATIVE SOLVERS");
    Y += CardHeadingHeight;
    const std::string Pressed = DrawOperationGrid(State, Sheet, Left, Y, Width, Operations);
    Y += OperationGridHeight(Operations.size()) + 15.0f;
    DrawParagraph(Sheet, Left, Y, Width, Note);
    CloseSheetCard(Card);
    if (!Pressed.empty()) RunOperation(State, Pressed);
}

// 📝 Transform tab: G / R / S on the picked bodies (Plasticity transform gizmos with typed values in the viewport).
void DrawTransformCard(SolidArcWorkspaceState& State, SolidArcSheet& Sheet)
{
    const float       Width = ImGui::GetContentRegionAvail().x - 2.0f * CardInset;
    const bool        Picked = !State.Document.Selection.Bodies.empty() && !State.Bevel.ActiveCondition;
    const std::string Lead  = Picked ? "Move, rotate or scale the picked bodies: drag in the viewport or type a value, Enter commits one step."
                                     : "Pick bodies in mode 4 (solid) to move, rotate or scale them.";
    const float       LeadHeight = MeasureParagraph(Sheet, Width, Lead);
    const PlaneExtent Card = PlaceSheetCard(Sheet, 2.0f * CardInset + CardHeadingHeight + LeadHeight + 16.0f + OperationGridHeight(3));
    const float Left = Card.MinimumX + CardInset;
    float       Y    = Card.MinimumY + CardInset;
    DrawCardHeading(Sheet, Left, Y, Width, "Transform", "G · R · S");
    Y += CardHeadingHeight;
    Y += DrawParagraph(Sheet, Left, Y, Width, Lead) + 16.0f;
    const std::string Pressed = DrawOperationGrid(State, Sheet, Left, Y, Width, { { "Move", IconSymbol::SolidArcMove, "Move · G", Picked },
                                                                                 { "Rotate", IconSymbol::SolidArcRotate, "Rotate · R", Picked },
                                                                                 { "Scale", IconSymbol::SolidArcMaximize, "Scale · S", Picked } });
    CloseSheetCard(Card);
    if (!Pressed.empty()) RunOperation(State, Pressed);
}

// Topology tab: what is picked, face by face and edge by edge.
void DrawPickedCard(SolidArcWorkspaceState& State)
{
    const SolidArcSelection& Selection = State.Document.Selection;
    if (Selection.Faces.empty() && Selection.Edges.empty() && Selection.Curves.empty()) return;
    BeginCard("Picked", "CLICK ORDER");
    int Shown = 0;
    for (const SolidArcSceneFace& Face : State.Scene.Faces)
    {
        if (std::find(Selection.Faces.begin(), Selection.Faces.end(), Face.Name) == Selection.Faces.end() || ++Shown > 12) continue;
        const bool Active = Selection.LastKind == SolidArcSelectionKind::Face && Selection.Faces.back() == Face.Name;
        ImGui::TextColored(Tint(Active ? ActiveColour : SelectedColour), "Face");
        ImGui::SameLine();
        ImGui::TextWrapped("%s · %s · %.4f m²", Face.Name.c_str(), Face.Surface.c_str(), Face.Area);
    }
    for (const SolidArcSceneEdge& Edge : State.Scene.Edges)
    {
        if (std::find(Selection.Edges.begin(), Selection.Edges.end(), Edge.Name) == Selection.Edges.end() || ++Shown > 12) continue;
        const bool Active = Selection.LastKind == SolidArcSelectionKind::Edge && Selection.Edges.back() == Edge.Name;
        ImGui::TextColored(Tint(Active ? ActiveColour : SelectedColour), "Edge");
        ImGui::SameLine();
        ImGui::TextWrapped("%s · %s · %.4f m", Edge.Name.c_str(), Edge.Curve.c_str(), Edge.Length);
    }
    for (size_t Index = 0; Index < Selection.Curves.size(); ++Index)
    {
        const CurveFitOutcome* Fit = FindFittedCurve(State.Document, Selection.Curves[Index]);
        if (!Fit || ++Shown > 12) continue;
        ImGui::TextColored(Tint(Index + 1 == Selection.Curves.size() ? ActiveColour : SelectedColour), "Curve %d", int(Index + 1));
        ImGui::SameLine();
        ImGui::TextWrapped("%s · %d poles · fit %.3f mm", CaptionOf(State.Document, Selection.Curves[Index]).c_str(), int(Fit->Curve.Definition->Poles.size()), Fit->WorstDeviation * 1000.0);
    }
    if (Shown > 12) ImGui::TextDisabled("… %d more", Shown - 12);
    if (PressButton(State, "Clear selection###SelectionClear"))
    {
        ClearSolidArcSelection(State.Document);
        Announce(State, "Selection cleared");
    }
    EndCard();
}

// 📝 HTML .inspector card controls: .button pills (#222, #ffffff0d hairline), #0a0a0a fields (radius 14), light DM Sans
//    11 px text, thin scrollbar; pushed around the Inspector body only.
void PushInspectorStyle()
{
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 14.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(14.0f, 6.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 9.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, 12.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, 18.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 6.0f);
    ImGui::PushStyleColor(ImGuiCol_Text, Tint(IM_COL32(0xee, 0xee, 0xee, 0xff)));
    ImGui::PushStyleColor(ImGuiCol_TextDisabled, Tint(IM_COL32(0x88, 0x88, 0x88, 0xff)));
    ImGui::PushStyleColor(ImGuiCol_Button, Tint(IM_COL32(0x22, 0x22, 0x22, 0xff)));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Tint(IM_COL32(0x29, 0x29, 0x29, 0xff)));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, Tint(IM_COL32(0x33, 0x33, 0x33, 0xff)));
    ImGui::PushStyleColor(ImGuiCol_Border, Tint(IM_COL32(0xff, 0xff, 0xff, 0x0d)));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, Tint(IM_COL32(0x0a, 0x0a, 0x0a, 0xff)));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, Tint(IM_COL32(0x14, 0x14, 0x14, 0xff)));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, Tint(IM_COL32(0x1c, 0x1c, 0x1c, 0xff)));
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, Tint(IM_COL32(0xdf, 0xdf, 0xdf, 0xff)));
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, Tint(IM_COL32(0xff, 0xff, 0xff, 0xff)));
    ImGui::PushStyleColor(ImGuiCol_CheckMark, Tint(IM_COL32(0xed, 0xed, 0xed, 0xff)));
    ImGui::PushStyleColor(ImGuiCol_Header, Tint(IM_COL32(0xff, 0xff, 0xff, 0x16)));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, Tint(IM_COL32(0x29, 0x29, 0x29, 0xff)));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, Tint(IM_COL32(0x33, 0x33, 0x33, 0xff)));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, ImVec4(1, 1, 1, 0.12f));
    ImGui::PushStyleColor(ImGuiCol_Separator, Tint(IM_COL32(0xff, 0xff, 0xff, 0x0d)));
}

void PopInspectorStyle()
{
    ImGui::PopStyleColor(18);
    ImGui::PopStyleVar(7);
}

// 📝 HTML .panel.inspector: heading (Inspector 18 px, inspect button → Topology), the scrolling #inspector-body (padding
//    0 10 10) and the footer. Sessions (Boolean, Joint, Bevel) take the top; otherwise the selection summary, the ordered
//    selection, Bevel history, the tabs and the tab's cards.
void DrawInspector(SolidArcWorkspaceState& State, ImVec2 Position, ImVec2 Size)
{
    BeginSheetWindow("Inspector", Position, Size);
    SolidArcSheet Sheet;
    Sheet.Begin();
    const PlaneExtent Panel = Spanning(Position.x, Position.y, Size.x, Size.y);
    Sheet.Box(Panel, SolidArcInk(0x121212), SolidArcStyleTokens::PanelRadius, SolidArcWash(0x08));
    const float Left  = Panel.MinimumX + 1.0f;
    const float Top   = Panel.MinimumY + 1.0f;
    const float Right = Panel.MaximumX - 1.0f;
    const float HeadingCentre = Top + 20.0f + 14.0f;
    Sheet.Text(Left + 19.0f, HeadingCentre, 18.0f, SolidArcInk(0xeeeeee), "Inspector", SolidArcWeight::Light, -0.6f);
    if (SheetIconButton(State, Sheet, "InspectorInspect", Right - 19.0f - 14.0f, HeadingCentre, 28.0f, IconSymbol::SolidArcInspect, 17.0f))
        State.InspectorTab = SolidArcInspectorTab::Topology;
    // .panel-footer — 32 px, #ffffff0a top border, padding 8 13, 9 px #737373, link glyph 11, gap 6.
    const float FooterTop = Panel.MaximumY - 1.0f - 32.0f;
    Sheet.Line(Left, FooterTop + 0.5f, Right, FooterTop + 0.5f, SolidArcWash(0x0a));
    Sheet.Icon(IconSymbol::SolidArcLink, Left + 13.0f, FooterTop + 16.0f - 5.5f, 11.0f, SolidArcInk(0x737373));
    Sheet.Text(Left + 13.0f + 11.0f + 6.0f, FooterTop + 16.0f, 9.0f, SolidArcInk(0x737373), "Operations run in Engine/GeometryProcessing");

    const float BodyTop = Top + 64.0f;
    ImGui::SetCursorScreenPos(ImVec2(Left + 10.0f, BodyTop));
    PushInspectorStyle();
    ImGui::BeginChild("InspectorBody", ImVec2(Right - Left - 20.0f, FooterTop - 10.0f - BodyTop), ImGuiChildFlags_None, ImGuiWindowFlags_NoBackground);
    ImGui::PushFont(static_cast<ImFont*>(QuerySolidArcTypefaces().Light), 11.0f * SolidArcStyleTokens::FontScale);
    SolidArcSheet Body;
    Body.Begin();
    const SolidArcSelection& Selection = State.Document.Selection;
    const bool Picked = !Selection.Faces.empty() || !Selection.Edges.empty() || !Selection.Bodies.empty() || !Selection.Curves.empty();
    if (State.Knife.ActiveCondition) DrawKnifeCard(State);
    else if (State.Boolean.ActiveCondition) DrawBooleanCard(State);
    else if (State.Joint.ActiveCondition) DrawJointCard(State);
    else if (State.Bevel.ActiveCondition) DrawBevelCard(State);
    else if (!State.Drawing.ActiveCondition)
    {
        if (!Picked) DrawIntroCards(State, Body);
        else DrawSelectionSummary(State, Body);
        if (Selection.Bodies.size() >= 2) DrawOrderedSelection(State, Body);
    }
    if (State.Document.Summary.PresentCondition && !State.Bevel.ActiveCondition) DrawBevelHistory(State, Body);
    DrawInspectorTabs(State, Body);
    switch (State.InspectorTab)
    {
    case SolidArcInspectorTab::Geometry:
        if (!State.Drawing.ActiveCondition && !State.Bevel.ActiveCondition && !State.Boolean.ActiveCondition && !State.Joint.ActiveCondition && !State.Knife.ActiveCondition) DrawSelectionToolCards(State);
        DrawStepCard(State);
        DrawBoundsCard(State, Body, "Bounds");
        if (!State.Drawing.ActiveCondition) DrawOperationsCard(State, Body);
        DrawMeasureCard(State);
        DrawSectionCard(State);
        DrawDrawingSheetCard(State);
        DrawDrawingCard(State);
        DrawSnapCard(State);
        DrawConstraintCard(State);
        DrawBodyCard(State);
        DrawAppearanceCard(State);
        DrawImagePlaneCard(State);
        break;
    case SolidArcInspectorTab::Transform:
        DrawBoundsCard(State, Body, "Bounds");
        DrawTransformCard(State, Body);
        break;
    case SolidArcInspectorTab::Topology:
        DrawBodyCard(State);
        DrawPickedCard(State);
        DrawHistoryCard(State);
        break;
    }
    ImGui::PopFont();
    ImGui::EndChild();
    PopInspectorStyle();
    EndSheetWindow();
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    SHORTCUTS + RECOVERY
//------------------------------------------------------------------------------------------------------------------------

// Plasticity Space (viewport:navigate:selection): frame the selected faces and edges, or the whole body.
void FrameSelection(SolidArcWorkspaceState& State)
{
    const SolidArcSelection& Selection = State.Document.Selection;
    if (State.Scene.Faces.empty() && State.Document.Regeneration.Curves.empty()) return;
    SolidArcScene Bounds;
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
    for (const SolidArcSceneFace& Face : State.Scene.Faces)
    {
        if (std::find(Selection.Faces.begin(), Selection.Faces.end(), Face.Name) == Selection.Faces.end()) continue;
        for (uint32_t Corner = Face.FirstCorner; Corner < Face.FirstCorner + Face.CornerCount; ++Corner) Include(State.Scene.Corners[Corner].Position);
    }
    for (const SolidArcSceneEdge& Edge : State.Scene.Edges)
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
    FitSolidArcCamera(State.Camera, Bounds);
    State.Camera.SceneRadius = std::max(State.Camera.SceneRadius, 1.0);
    Announce(State, Selected ? "Framed the selection (Space)" : "Framed the document (Space)");
}

// Plasticity Shift+R (edit:repeat-last-command) on the current selection.
void RepeatLastCommand(SolidArcWorkspaceState& State)
{
    const SolidArcRepeat& Last = State.LastCommand;
    switch (Last.Kind)
    {
    case SolidArcRepeatKind::None:      Announce(State, "Nothing to repeat yet"); return;
    case SolidArcRepeatKind::Blend:     Conclude(State, BlendSolidArcSelection(State.Document, Last.Blend)); return;
    case SolidArcRepeatKind::Face:      Conclude(State, Last.Operation == FaceOperationKind::Inset ? InsetSolidArcSelection(State.Document, Last.Amount, Last.Depth)
                                                                                              : ChangeSolidArcSelection(State.Document, Last.Operation, Last.Amount)); return;
    case SolidArcRepeatKind::Construct: Conclude(State, ConstructSolidArcPrimitive(State.Document, Last.Construct)); return;
    }
}

// Plasticity Shift+C / V / X / Z (box, cylinder, sphere, …): the drawn solid command; Tab inside it opens the size form.
void OpenConstruct(SolidArcWorkspaceState& State, SolidArcConstructKind Kind)
{
    OpenSolidSession(State, Kind);
}

// 📝 Plasticity default keymap (nkallen/plasticity src/startup/default-keymap.ts). Commands this modeller does not have
//    (control points, curves, hide, materials, groups, clipboard, booleans between bodies) announce that instead of
//    silently doing something else; no non-Plasticity shortcut remains.
void ApplyShortcuts(SolidArcWorkspaceState& State)
{
    ImGuiIO& Io = ImGui::GetIO();
#ifdef FRONTIER_DEVELOPMENT_TRACE
    if (ImGui::IsKeyPressed(ImGuiKey_F12, false)) State.TraceShown = !State.TraceShown;          // development: operation trace panel
#endif
    if (Io.WantTextInput || ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel)) return;
    const bool Control   = Io.KeyCtrl;
    const bool Shift     = Io.KeyShift;
    const bool Alternate = Io.KeyAlt;
    if (ImGui::IsKeyPressed(ImGuiKey_LeftAlt, false) || ImGui::IsKeyPressed(ImGuiKey_RightAlt, false)) State.Picker.AlongUsedCondition = false;
    auto Pressed = [](ImGuiKey Key) { return ImGui::IsKeyPressed(Key, false); };

    // Drawing sheet (Alt+D): the paper replaces the model — Esc or Alt+D returns, Ctrl+S still saves.
    if (State.DrawingSheet.ActiveCondition)
    {
        if (Pressed(ImGuiKey_Escape) || (Alternate && !Control && !Shift && Pressed(ImGuiKey_D))) ToggleDrawingSheet(State);
        else if (Control && !Alternate && !Shift && Pressed(ImGuiKey_S)) SaveDocument(State, State.DocumentPath.empty() ? State.Options.DocumentPath : State.DocumentPath);
        return;
    }

    // Gizmo handle held: Esc drops the drag (no edit).
    if (State.Gizmo.DragCondition)
    {
        if (Pressed(ImGuiKey_Escape)) CancelGizmoDrag(State, "Gizmo cancelled");
        return;
    }

    // Mode 1 pole drag in progress: Esc drops it (no edit).
    if (State.SketchPoints.DragCondition)
    {
        if (Pressed(ImGuiKey_Escape))
        {
            CancelSketchPointDrag(State);
            Announce(State, "Point drag dropped (document untouched)");
        }
        return;
    }
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
        SolidArcDrawingSession& Drawing = State.Drawing;
        if (Pressed(ImGuiKey_Enter) || Pressed(ImGuiKey_KeypadEnter)) FinishDrawingSession(State, false);
        else if (Pressed(ImGuiKey_Escape)) CancelDrawingSession(State);
        else if (Control && Pressed(ImGuiKey_Z)) RetractDrawingPoint(State);               // gizmo:line:undo / gizmo:curve:undo
        else if (Alternate && !Control && !Io.KeyShift && Pressed(ImGuiKey_Z)) ToggleXRay(State);       // viewport:toggle-x-ray stays live (hidden snaps)
        else if (Drawing.SolidCondition && Pressed(ImGuiKey_Tab)) OpenSolidExactForm(State);   // Exact sizes in the Construct form
        else if (Drawing.SolidCondition && !Control && !Alternate && (Pressed(ImGuiKey_Q) || Pressed(ImGuiKey_W) || Pressed(ImGuiKey_E) || Pressed(ImGuiKey_R)))
        {
            // Plasticity boolean keys inside a creation command: Q union · W difference · E intersection · R new body.
            Drawing.Combination = Pressed(ImGuiKey_Q) ? GeometryProcessing::FeatureCombination::Union : Pressed(ImGuiKey_W) ? GeometryProcessing::FeatureCombination::Difference
                                : Pressed(ImGuiKey_E) ? GeometryProcessing::FeatureCombination::Intersection : GeometryProcessing::FeatureCombination::Separate;
            Announce(State, std::string(SolidArcConstructLabel(Drawing.Solid)) + ": " + (Drawing.Combination == GeometryProcessing::FeatureCombination::Separate ? "new body" : FeatureCombinationLabel(Drawing.Combination)));
        }
        else if ((Drawing.Kind == SolidArcDrawingKind::Polygon || (Drawing.SolidCondition && Drawing.Solid == SolidArcConstructKind::Prism)) && !Control && !Alternate &&
                 (Pressed(ImGuiKey_Equal) || Pressed(ImGuiKey_KeypadAdd) || Pressed(ImGuiKey_Minus) || Pressed(ImGuiKey_KeypadSubtract)))
        {
            Drawing.Sides = std::clamp(Drawing.Sides + (Pressed(ImGuiKey_Minus) || Pressed(ImGuiKey_KeypadSubtract) ? -1 : 1), 3, 64);
            Drawing.Refusal.clear();
            Announce(State, Compose("%d sides", Drawing.Sides));
        }
        else if (!Control && !Alternate && !Io.KeyShift && (Pressed(ImGuiKey_X) || Pressed(ImGuiKey_Y) || Pressed(ImGuiKey_Z) || Pressed(ImGuiKey_T) || Pressed(ImGuiKey_N) || Pressed(ImGuiKey_B)))
        {
            // body[gizmo=point-picker]: x / y / z, set-tangent, set-normal, set-binormal
            ChooseSnapLock(State, Pressed(ImGuiKey_X) ? SolidArcSnapLockKind::X : Pressed(ImGuiKey_Y) ? SolidArcSnapLockKind::Y : Pressed(ImGuiKey_Z) ? SolidArcSnapLockKind::Z
                                : Pressed(ImGuiKey_T) ? SolidArcSnapLockKind::Tangent : Pressed(ImGuiKey_N) ? SolidArcSnapLockKind::Normal : SolidArcSnapLockKind::Binormal);
        }
        else if (Drawing.Kind == SolidArcDrawingKind::Rectangle && !Control && !Alternate && Pressed(ImGuiKey_S))
        {
            Drawing.SquareCondition = !Drawing.SquareCondition;                               // body[gizmo=point-picker]: set-square
            Announce(State, Drawing.SquareCondition ? "Rectangle: square (s releases)" : "Rectangle: free sides");
        }
        else if (Drawing.Kind == SolidArcDrawingKind::Curve && !Control && !Alternate)
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
        else if (Drawing.Kind == SolidArcDrawingKind::Rectangle && (ImGui::IsKeyReleased(ImGuiKey_LeftAlt) || ImGui::IsKeyReleased(ImGuiKey_RightAlt)) &&
                 !std::exchange(State.Picker.AlongUsedCondition, false))
        {
            // 📝 A tap of Alt toggles centre mode on release; an Alt held to slide along a curve does not.
            Drawing.CentreCondition = !Drawing.CentreCondition;                               // keyboard:rectangle:mode
            Announce(State, Drawing.CentreCondition ? "Rectangle: centre mode (first click is the centre)" : "Rectangle: corner mode");
        }
        else if (Drawing.Kind == SolidArcDrawingKind::Circle && !Control && !Alternate && Pressed(ImGuiKey_V))
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
        if ((State.Bevel.Verb == SolidArcSessionVerb::Blend || State.Bevel.Verb == SolidArcSessionVerb::Corner) && !Control && !Alternate)
        {
            if (Pressed(ImGuiKey_D)) State.Bevel.Shape = BlendProfile::Fillet;                // gizmo:fillet-solid:fillet
            if (Pressed(ImGuiKey_C)) State.Bevel.Shape = BlendProfile::Chamfer;               // gizmo:fillet-solid:chamfer
            if (Pressed(ImGuiKey_V)) ToggleBevelVariable(State);                              // gizmo:fillet-solid:add (variable size)
            if (Pressed(ImGuiKey_F)) FlipBevelAnchor(State);                                  // as gizmo:place:flip
        }
        if (State.Bevel.Verb == SolidArcSessionVerb::Inset && !Control && !Alternate)
        {
            if (Pressed(ImGuiKey_D)) State.Bevel.DepthEnabled = false;                        // as gizmo:offset-face:distance
            if (Pressed(ImGuiKey_H)) State.Bevel.DepthEnabled = true;                         // depth (height) of the inner face
        }
        if (State.Bevel.Verb == SolidArcSessionVerb::Profile && !Control && !Alternate)
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
        SolidArcTransformSession& Transform = State.Transform;
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
                    Transform.PlaneCondition = Shift && Transform.Verb == SolidArcTransformVerb::Move;
                }
            const ImGuiKey Own = Transform.Verb == SolidArcTransformVerb::Move ? ImGuiKey_G : Transform.Verb == SolidArcTransformVerb::Rotate ? ImGuiKey_R : ImGuiKey_S;
            if (Pressed(Own)) { Transform.Axis = -1; Transform.PlaneCondition = false; }       // gizmo:move:screen · rotate:screen · scale:xyz
        }
        return;
    }

    // Shift+K knife — digits type the depth / width, Q / W / E / R combine, T through, K keep, [ / ] turn, Tab mode.
    if (State.Knife.ActiveCondition && !State.Knife.DrawPendingCondition)
    {
        SolidArcKnifeSession& Knife = State.Knife;
        if (CollectTypedAmount(Knife.Typed)) Knife.StagedKey.clear();
        if (Pressed(ImGuiKey_Enter) || Pressed(ImGuiKey_KeypadEnter)) ApplyKnifeSession(State);
        else if (Pressed(ImGuiKey_Escape))
        {
            if (Knife.Mode == SolidArcKnifeMode::Insert && Knife.PlacedCondition) { Knife.PlacedCondition = false; Knife.Typed.clear(); Announce(State, "Knife: insert lifted (point at a face again)"); }
            else CloseKnifeSession(State, "Knife cancelled (document untouched)");
        }
        else if (!Control && !Alternate)
        {
            if (Pressed(ImGuiKey_Q)) ChooseKnifeCombination(State, FeatureCombination::Union);          // keyboard:knife:add
            if (Pressed(ImGuiKey_W)) ChooseKnifeCombination(State, FeatureCombination::Difference);     // keyboard:knife:cut
            if (Pressed(ImGuiKey_E)) ChooseKnifeCombination(State, FeatureCombination::Intersection);   // keyboard:knife:intersect
            if (Pressed(ImGuiKey_R)) ChooseKnifeCombination(State, FeatureCombination::Separate);       // keyboard:knife:separate
            if (Pressed(ImGuiKey_T) && Knife.Mode == SolidArcKnifeMode::Sketch) { Knife.ThroughCondition = !Knife.ThroughCondition; Announce(State, Knife.ThroughCondition ? "Knife: through" : "Knife: to depth"); }
            if (Pressed(ImGuiKey_K) && Knife.Mode == SolidArcKnifeMode::Object) { Knife.KeepCondition = !Knife.KeepCondition; Announce(State, Knife.KeepCondition ? "Knife: cutters kept" : "Knife: cutters consumed"); }
            if (Pressed(ImGuiKey_LeftBracket) || Pressed(ImGuiKey_RightBracket))
            {
                Knife.Turn = std::remainder(Knife.Turn + (Pressed(ImGuiKey_LeftBracket) ? -1.0 : 1.0) * std::numbers::pi / 12.0, 2.0 * std::numbers::pi);
                Announce(State, Compose("Knife: turn %.0f°", Knife.Turn * 180.0 / std::numbers::pi));
            }
            if (Pressed(ImGuiKey_Tab)) ChooseKnifeMode(State, SolidArcKnifeMode((int(Knife.Mode) + 1) % 3));
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
            if (Pressed(ImGuiKey_A) && State.Sweep.Verb != SolidArcSweepVerb::Offset && State.Sweep.Verb != SolidArcSweepVerb::Pipe)
                Announce(State, "Angle: type degrees (gizmo:revolution:angle)");
        }
        return;
    }

    // Viewport navigation (any modifier state the keymap names).
    SolidArcNavigationKeypad Keypad;
    Keypad.FrontPressed      = Pressed(ImGuiKey_Keypad1);
    Keypad.RightPressed      = Pressed(ImGuiKey_Keypad3);
    Keypad.TopPressed        = Pressed(ImGuiKey_Keypad7);
    Keypad.ProjectionPressed = Pressed(ImGuiKey_Keypad5);
    Keypad.ControlCondition  = Control;
    if (const std::optional<OrbitViewPreset> View = NavigateSolidArcKeypad(State.Camera, Keypad)) PresentView(State, *View);

    // [command='constrain'] — SolveSpace verbs on the picks, typed values, Delete removes a picked constraint (view keys live).
    if (State.Constrain.ActiveCondition)
    {
        RouteConstraintKeys(State, Pressed, Control, Shift, Alternate);
        return;
    }

    // body[command='trim'] — click pieces to remove · Enter / right-click / Esc / T ends (the view keys above stay live).
    if (State.Trim.ActiveCondition)
    {
        if (Pressed(ImGuiKey_Escape) || Pressed(ImGuiKey_Enter) || Pressed(ImGuiKey_KeypadEnter) || Pressed(ImGuiKey_T)) CloseTrimSession(State);
        return;
    }

    // Fill tool — click regions to fill · Enter / right-click / Esc / F ends (the view keys above stay live).
    if (State.Fill.ActiveCondition)
    {
        if (Pressed(ImGuiKey_Escape) || Pressed(ImGuiKey_Enter) || Pressed(ImGuiKey_KeypadEnter) || Pressed(ImGuiKey_F)) CloseFillSession(State);
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
        if (Pressed(ImGuiKey_H)) ChangeVisibility(State, SolidArcVisibilityVerb::InvertHidden);   // invert-hidden
        if (Pressed(ImGuiKey_G)) Announce(State, "Group: not available");
        if (Pressed(ImGuiKey_D)) Announce(State, "Place: not available");
        if (Pressed(ImGuiKey_1) || Pressed(ImGuiKey_2) || Pressed(ImGuiKey_3) || Pressed(ImGuiKey_4)) Announce(State, "Selection convert: not available yet");
        return;
    }
    if (Alternate)
    {
        if (Pressed(ImGuiKey_A)) { ClearSolidArcSelection(State.Document); State.FocusedStep = {}; Announce(State, "Selection cleared"); }
        if (Pressed(ImGuiKey_Z))
        {
            if (Shift) { State.Style.GridEnabled = !State.Style.GridEnabled; Announce(State, State.Style.GridEnabled ? "Overlays on" : "Overlays off"); }
            else ToggleXRay(State);                                                     // viewport:toggle-x-ray
        }
        if (Pressed(ImGuiKey_E)) OpenSweepSession(State, SolidArcSweepVerb::Evolution);     // command:evolution
        if (Pressed(ImGuiKey_S)) ToggleSection(State);                                      // section (no Plasticity default; Alt+S is free)
        if (Pressed(ImGuiKey_D)) ToggleDrawingSheet(State);                                 // drawing sheet (no Plasticity default; Alt+D is free)
        if (Pressed(ImGuiKey_J)) OpenJointSession(State);                                   // joint (no Plasticity default; Alt+J is free)
        if (Pressed(ImGuiKey_R)) OpenSweepSession(State, SolidArcSweepVerb::Revolve);       // revolution (no Plasticity default key)
        if (Pressed(ImGuiKey_X)) MirrorPanels(State);                                       // mirror (gizmo x: plane x = 0)
        if (Pressed(ImGuiKey_M)) RemoveMatcap(State);                                       // remove-material
        if (Pressed(ImGuiKey_H)) ChangeVisibility(State, SolidArcVisibilityVerb::UnhideAll);       // unhide-all
        if (Pressed(ImGuiKey_N)) FlipPickedNormals(State);                                  // flip normals (Alt+N is free in Plasticity's map)
        if (Pressed(ImGuiKey_G) || Pressed(ImGuiKey_Q)) Announce(State, "Not available in this modeller");
        return;
    }
    if (Shift)
    {
        if (Pressed(ImGuiKey_C) || Pressed(ImGuiKey_V))                                     // corner / centre box
        {
            const bool Centre = Pressed(ImGuiKey_V);
            OpenConstruct(State, SolidArcConstructKind::Block);
            State.Drawing.CentreCondition = Centre;
        }
        if (Pressed(ImGuiKey_X)) OpenConstruct(State, SolidArcConstructKind::Cylinder);
        if (Pressed(ImGuiKey_Z)) OpenConstruct(State, SolidArcConstructKind::Sphere);
        if (Pressed(ImGuiKey_R)) RepeatLastCommand(State);
        if (Pressed(ImGuiKey_D)) OpenTransformSession(State, SolidArcTransformVerb::Move, true);        // duplicate (then move)
        if (Pressed(ImGuiKey_A)) OpenDrawingSession(State, SolidArcDrawingKind::Line);                  // line
        if (Pressed(ImGuiKey_S)) OpenDrawingSession(State, SolidArcDrawingKind::Curve);                 // curve
        if (Pressed(ImGuiKey_Q)) OpenDrawingSession(State, SolidArcDrawingKind::Rectangle);             // corner-rectangle
        if (Pressed(ImGuiKey_W)) OpenDrawingSession(State, SolidArcDrawingKind::Circle);                // center-circle
        if (Pressed(ImGuiKey_H)) ChangeVisibility(State, SolidArcVisibilityVerb::HideUnselected);       // hide-unselected
        if (Pressed(ImGuiKey_P)) OpenSweepSession(State, SolidArcSweepVerb::Evolution);     // command:evolution
        if (Pressed(ImGuiKey_K)) OpenKnifeSession(State);                                   // knife (KitOps-style cutters; free in Plasticity's map)
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
    if (Pressed(ImGuiKey_1)) State.PickMode = SolidArcPickMode::ControlPoint;
    if (Pressed(ImGuiKey_L)) LoftSelectedCurves(State);                                     // loft
    if (Pressed(ImGuiKey_2)) State.PickMode = SolidArcPickMode::Edge;
    if (Pressed(ImGuiKey_3)) State.PickMode = SolidArcPickMode::Face;
    if (Pressed(ImGuiKey_4)) State.PickMode = SolidArcPickMode::Solid;
    if (Pressed(ImGuiKey_5) || Pressed(ImGuiKey_Tab)) State.PickMode = SolidArcPickMode::Any;
    if (Pressed(ImGuiKey_B)) OpenBevelSession(State, BlendProfile::Fillet);
    if (Pressed(ImGuiKey_E))
    {
        if (State.Document.Selection.Faces.empty() && FocusedSketchCondition(State)) OpenSketchExtrudeSession(State);
        else OpenExtrudeSession(State);
    }
    if (Pressed(ImGuiKey_Space)) FrameSelection(State);
    if (Pressed(ImGuiKey_Q)) OpenBooleanSession(State);                                    // command:boolean
    if (Pressed(ImGuiKey_G)) OpenTransformSession(State, SolidArcTransformVerb::Move, false);       // command:move
    if (Pressed(ImGuiKey_R)) OpenTransformSession(State, SolidArcTransformVerb::Rotate, false);     // command:rotate
    if (Pressed(ImGuiKey_S)) OpenTransformSession(State, SolidArcTransformVerb::Scale, false);      // command:scale
    if (Pressed(ImGuiKey_H)) ChangeVisibility(State, SolidArcVisibilityVerb::HideSelected);         // hide-selected
    if (Pressed(ImGuiKey_C)) ApplyCut(State);                                              // command:cut
    if (Pressed(ImGuiKey_T)) OpenTrimSession(State);                                       // command:trim
    if (Pressed(ImGuiKey_F)) ToggleSketchFill(State);                                      // fill on / off (f is free in Plasticity's map)
    if (Pressed(ImGuiKey_K)) OpenConstraintSession(State);                                 // constrain (K is free in Plasticity's map)
    // I: inset with faces selected (Blender habit), else measure (I is free in Plasticity's map).
    if (Pressed(ImGuiKey_I)) { if (State.Document.Selection.Faces.empty()) ToggleMeasure(State); else OpenInsetSession(State); }
    if (Pressed(ImGuiKey_J)) JoinSketches(State);                                          // command:join-curves
    // command:offset-curve — on selected faces it offsets the face boundary inward: the inset.
    if (Pressed(ImGuiKey_O)) { if (State.Document.Selection.Faces.empty()) OpenSweepSession(State, SolidArcSweepVerb::Offset); else OpenInsetSession(State); }
    if (Pressed(ImGuiKey_P)) OpenSweepSession(State, SolidArcSweepVerb::Pipe);             // command:pipe
    if (Pressed(ImGuiKey_Y)) CycleGizmoModes(State);                                       // gizmo modes (y is free in Plasticity's map)
    if (Pressed(ImGuiKey_Escape))
    {
        ClearSolidArcSelection(State.Document);
        State.FocusedStep = {};
        Announce(State, "Selection cleared");
    }
    if (Pressed(ImGuiKey_X) || Pressed(ImGuiKey_Delete) || Pressed(ImGuiKey_Backspace))
    {
        if (!State.FocusedStep.empty())
        {
            Conclude(State, DeleteSolidArcStep(State.Document, State.FocusedStep));
            State.FocusedStep = {};
        }
        else if (!State.Document.Selection.Bodies.empty())
        {
            Conclude(State, RemoveSolidArcBodies(State.Document, State.Document.Selection.Bodies));          // delete picked bodies (mode 4)
        }
        else if (!State.Document.Selection.Faces.empty())
        {
            const SolidArcCommandOutcome Outcome = ChangeSolidArcSelection(State.Document, FaceOperationKind::Delete, 0.0);
            if (Outcome.AcceptedCondition) State.LastCommand = { SolidArcRepeatKind::Face, BlendProfile::Fillet, 0.0, FaceOperationKind::Delete, 0.0, {} };
            Conclude(State, Outcome);
        }
        else Announce(State, "Delete needs picked bodies (mode 4), selected faces or a focused feature step");
    }
}

void DrawWelcome(SolidArcWorkspaceState& State)
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

void AdvanceAutosave(SolidArcWorkspaceState& State, double DeltaSeconds)
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

void ConfigureSolidArcInterface(const SolidArcOptions& Options)
{
    ImGuiIO& Io = ImGui::GetIO();
    Io.IniFilename = nullptr;
    Io.LogFilename = nullptr;
    // 📝 Content root, first one holding DM Sans: the Frontier checkout the build came from (development), the folder whose
    //    EngineContent sits beside or above the executable (installed package, upstream ResolveContentDirectory), the
    //    working directory.
    const auto HoldsTypeface = [](const std::filesystem::path& Candidate)
    {
        std::error_code Failure;
        return std::filesystem::exists(Candidate / "EngineContent/FontArchives/DMSans/DMSans-Regular.ttf", Failure)
            || std::filesystem::exists(Candidate / "EngineContent/FontArchives/DM Sans/DMSans-Regular.ttf", Failure);
    };
    std::filesystem::path Root = ".";
    if (const std::filesystem::path Packaged = ResolveContentDirectory("EngineContent").parent_path(); HoldsTypeface(Packaged)) Root = Packaged;
#ifdef SOLIDARC_CONTENT_ROOT
    if (HoldsTypeface(SOLIDARC_CONTENT_ROOT)) Root = SOLIDARC_CONTENT_ROOT;                      // Development checkout wins
#endif
    std::filesystem::path Family;
    for (const char* Candidate : { "EngineContent/FontArchives/DMSans", "EngineContent/FontArchives/DM Sans" })
        if (std::filesystem::exists(Root / Candidate / "DMSans-Regular.ttf")) { Family = Root / Candidate; break; }
    const std::string FontPath = !Options.FontPath.empty() ? Options.FontPath : Family.empty() ? std::string() : (Family / "DMSans-Regular.ttf").string();
    SolidArcTypefaces Typefaces;
    if (!FontPath.empty() && std::filesystem::exists(FontPath))
    {
        ImFont* Regular = Io.Fonts->AddFontFromFileTTF(FontPath.c_str(), Options.FontSize);  // First font: the ImGui default
        const std::filesystem::path LightPath = std::filesystem::path(FontPath).parent_path() / "DMSans-Light.ttf";
        ImFont* Light = std::filesystem::exists(LightPath) ? Io.Fonts->AddFontFromFileTTF(LightPath.string().c_str(), Options.FontSize) : Regular;
        Typefaces = { Light, Regular };
    }
    else Io.Fonts->AddFontDefault();
    ConfigureSolidArcTypefaces(Typefaces);
    IconPresentation::Attach(Root / "EngineContent/Icons");

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

void InitialiseSolidArcWorkspace(SolidArcWorkspaceState& State, const SolidArcOptions& Options)
{
    State.Options                    = Options;
    State.Document                   = CreateSolidArcDocument("Untitled");
    State.Camera                     = OrbitCamera{};
    State.Camera.FieldOfView         = Options.FieldOfView;
    State.Camera.OrthographicEnabled = Options.OrthographicEnabled;
    State.Style.EdgeStroke           = Options.EdgeStroke;
    State.Style.GridEnabled          = Options.GridEnabled;
    State.WelcomeOpen                = !Options.RecoveryPath.empty() && std::filesystem::exists(Options.RecoveryPath);
    RefreshSolidArcScene(State, false);
    if (!Options.Refusal.empty()) Announce(State, "Options: " + Options.Refusal);
}

// 📝 F12 (development builds): the newest operation trace lines, the same text SolidArcTrace.log receives. A filter keeps
//    the lines containing its text; Copy puts the shown lines on the clipboard for a bug report.
void DrawTracePanel(SolidArcWorkspaceState& State)
{
#ifdef FRONTIER_DEVELOPMENT_TRACE
    if (!State.TraceShown) return;
    const ImVec2 Display = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowSize(ImVec2(std::min(980.0f, Display.x - 40.0f), 360.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(20.0f, Display.y - 400.0f), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Operation trace (F12)###OperationTrace", &State.TraceShown)) { ImGui::End(); return; }
    const std::filesystem::path Path = OperationTracePath();
    const std::string           Where = Path.empty() ? std::string("in memory only") : DescribeUtf8Path(Path);
    ImGui::TextDisabled("Log file: %s", Where.c_str());
    ImGui::SameLine();
    if (!Path.empty() && ImGui::SmallButton("Copy path###TraceCopyPath")) ImGui::SetClipboardText(Where.c_str());
    ImGui::TextDisabled("Frames over %.0f ms: %llu · slowest %.0f ms · each slow frame writes a \"Frame\" line with its costliest phases",
                        State.Profile.SlowThreshold, static_cast<unsigned long long>(State.Profile.SlowFrames), State.Profile.WorstMs);
    char Filter[128] = {};
    std::snprintf(Filter, sizeof(Filter), "%s", State.TraceFilter.c_str());
    ImGui::SetNextItemWidth(260.0f);
    if (ImGui::InputTextWithHint("###TraceFilter", "filter (Blend, Replay, refused …)", Filter, sizeof(Filter))) State.TraceFilter = Filter;
    ImGui::SameLine();
    const bool CopyCondition = ImGui::Button("Copy###TraceCopy");
    ImGui::SameLine();
    if (ImGui::Button("Slow frames###TraceSlow")) State.TraceFilter = State.TraceFilter == "slow frame" ? std::string() : std::string("slow frame");
    ImGui::SameLine();
    if (ImGui::Button("Refusals###TraceRefused")) State.TraceFilter = State.TraceFilter == "refused" ? std::string() : std::string("refused");
    const std::vector<std::string> Lines = RecentOperationTrace(1500);
    std::string Shown;
    ImGui::BeginChild("###TraceLines", ImVec2(0, 0), ImGuiChildFlags_Borders, ImGuiWindowFlags_HorizontalScrollbar);
    for (const std::string& Line : Lines)
    {
        if (!State.TraceFilter.empty() && Line.find(State.TraceFilter) == std::string::npos) continue;
        const bool Refused = Line.find("refused") != std::string::npos || Line.find("NOT converged") != std::string::npos || Line.find("slow") != std::string::npos;
        if (Refused) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.62f, 0.45f, 1.0f));
        ImGui::TextUnformatted(Line.c_str());
        if (Refused) ImGui::PopStyleColor();
        if (CopyCondition) Shown += Line + "\n";
    }
    static size_t Seen = 0;
    if (OperationTraceRevision() != Seen && ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 40.0f) ImGui::SetScrollHereY(1.0f);
    Seen = OperationTraceRevision();
    ImGui::EndChild();
    if (CopyCondition) ImGui::SetClipboardText(Shown.c_str());
    ImGui::End();
#else
    State.TraceShown = false;
#endif
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    FRAME TIMING
//------------------------------------------------------------------------------------------------------------------------

[[nodiscard]] double SteadySeconds()
{
    return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

SolidArcPhaseClock::SolidArcPhaseClock(SolidArcFrameProfile& Profile, const char* Name) : Profile(Profile), Name(Name), Start(SteadySeconds())
{
}

SolidArcPhaseClock::~SolidArcPhaseClock()
{
    const double Milliseconds = (SteadySeconds() - Start) * 1.0e3;
    for (SolidArcFramePhase& Phase : Profile.Phases)
        if (Phase.Name == Name) { Phase.Milliseconds += Milliseconds; ++Phase.Calls; return; }
    Profile.Phases.push_back({ Name, Milliseconds, 1 });
}

// What owns the pointer this frame, for the slow-frame line ("bevel session", "gizmo drag", "idle", …).
[[nodiscard]] std::string ActiveSessionLabel(const SolidArcWorkspaceState& State)
{
    if (State.Bevel.ActiveCondition) return State.Bevel.Preview ? "bevel session (preview staged)" : "bevel session";
    if (State.Transform.ActiveCondition) return std::string(TransformVerbLabel(State.Transform.Verb)) + " session";
    if (State.Gizmo.DragCondition) return "gizmo drag";
    if (State.Drawing.ActiveCondition) return State.Drawing.SolidCondition ? "Construct drawing" : "sketch drawing";
    if (State.SketchPoints.DragCondition) return "sketch point drag";
    if (State.PoleDrag.ActiveCondition) return "pole drag";
    if (State.Constrain.ActiveCondition) return "constraint session";
    if (State.Trim.ActiveCondition) return "trim session";
    if (State.Fill.ActiveCondition) return "fill session";
    if (State.Sweep.ActiveCondition) return "sweep session";
    if (State.Knife.ActiveCondition) return "knife session";
    if (State.Joint.ActiveCondition) return "joint session";
    if (ImGui::IsAnyMouseDown()) return "pointer held";
    return "idle";
}

void ConcludeSolidArcFrame(SolidArcWorkspaceState& State)
{
    SolidArcFrameProfile& Profile = State.Profile;
    const double Total = State.FrameMs + Profile.GpuWaitMs;
    Profile.WorstMs = std::max(Profile.WorstMs, Total);
    const double Now  = SteadySeconds();
    const bool   Slow = Total >= Profile.SlowThreshold && State.FrameNumber > 2;            // Start-up frames load fonts and matcaps
    if (Slow && Now - Profile.LineSeconds < Profile.LineGap) ++Profile.Suppressed;
    else if (Slow)
    {
        // 📝 Phases nest ("viewport" holds "viewport · overlays"), so the list is a ranking, not a sum.
        std::vector<SolidArcFramePhase> Ranked = Profile.Phases;
        std::sort(Ranked.begin(), Ranked.end(), [](const SolidArcFramePhase& Left, const SolidArcFramePhase& Right) { return Left.Milliseconds > Right.Milliseconds; });
        const SolidArcScene& Scene = DisplayedSolidArcScene(State);
        std::string Line = Compose("slow frame %llu · %.0f ms (CPU %.0f · interface %.0f · upload %.0f · GPU wait %.0f) · %s · %zu triangles · %zu steps",
                                   static_cast<unsigned long long>(State.FrameNumber), Total, State.FrameMs, Profile.InterfaceMs, Profile.UploadMs, Profile.GpuWaitMs,
                                   ActiveSessionLabel(State).c_str(), Scene.Corners.size() / 3, State.Document.Tree.Steps.size());
        int Listed = 0;
        for (const SolidArcFramePhase& Phase : Ranked)
        {
            if (Listed == 6 || Phase.Milliseconds < 1.0) break;
            Line += Phase.Calls > 1 ? Compose(" · %s %.0f ms ×%d", Phase.Name, Phase.Milliseconds, Phase.Calls) : Compose(" · %s %.0f ms", Phase.Name, Phase.Milliseconds);
            ++Listed;
        }
        if (Profile.Suppressed > 0) Line += Compose(" · +%llu slow frame%s since the last line", static_cast<unsigned long long>(Profile.Suppressed), Profile.Suppressed == 1 ? "" : "s");
        FRONTIER_TRACE("Frame", Line);
        ++Profile.SlowFrames;
        Profile.Suppressed  = 0;
        Profile.LineSeconds = Now;
    }
    Profile.Phases.clear();
    Profile.InterfaceMs = 0.0;
}

AffineMap GizmoPreviewMap(const SolidArcGizmoSession& Gizmo)
{
    return GizmoMap(Gizmo);
}

double RigidPreviewVolume(const SolidArcDocument& Document, const std::vector<std::string>& Keys, const AffineMap& Map)
{
    const double Determinant = Dot(Map.ColumnX, Cross(Map.ColumnY, Map.ColumnZ));
    double       Volume      = 0.0;
    for (const SolidArcBodyFigures& Body : Document.Summary.Bodies)
        Volume += std::find(Keys.begin(), Keys.end(), Body.Key) != Keys.end() ? Body.Volume * std::abs(Determinant) : Body.Volume;
    return Volume;
}

void DrawSolidArcInterface(SolidArcWorkspaceState& State, ImTextureID ViewportTexture, double DeltaSeconds)
{
    const double Entered = SteadySeconds();
    SolidArcFrameProfile& Profile = State.Profile;
    State.Widgets.Clear();
    { SolidArcPhaseClock Clock(Profile, "matcaps"); RefreshSolidArcMatcaps(State); }
    const ImVec2 Frame = ImGui::GetIO().DisplaySize;
    { SolidArcPhaseClock Clock(Profile, "bevel preview"); StageBevelPreview(State); }
    { SolidArcPhaseClock Clock(Profile, "measure"); RefreshMeasure(State); }
    { SolidArcPhaseClock Clock(Profile, "section"); RefreshSection(State); }
    // 📝 HTML grid at 1600 px: header 68, document bar 34, columns 300 | 1fr | 330 (gap 10, margin 12), status bar 31.
    { SolidArcPhaseClock Clock(Profile, "header"); DrawHeader(State, Frame); DrawDocumentBar(State, Frame); }
    const float Top    = HeaderHeight + DocumentHeight;
    const float Height = std::max(100.0f, Frame.y - Top - StatusHeight);
    const float Middle = std::max(100.0f, Frame.x - 2 * FrameMargin - 2 * ColumnGap - OutlinerWidth - InspectorWidth);
    if (State.ViewportMaximizedEnabled)
    {
        // HTML maximize: the scene panel takes the whole row (Outliner and Inspector step aside).
        SolidArcPhaseClock Clock(Profile, "viewport");
        DrawViewport(State, ViewportTexture, ImVec2(FrameMargin, Top), ImVec2(Frame.x - 2 * FrameMargin, Height));
    }
    else
    {
        { SolidArcPhaseClock Clock(Profile, "outliner"); DrawOutliner(State, ImVec2(FrameMargin, Top), ImVec2(OutlinerWidth, Height)); }
        { SolidArcPhaseClock Clock(Profile, "viewport"); DrawViewport(State, ViewportTexture, ImVec2(FrameMargin + OutlinerWidth + ColumnGap, Top), ImVec2(Middle, Height)); }
        { SolidArcPhaseClock Clock(Profile, "inspector"); DrawInspector(State, ImVec2(FrameMargin + OutlinerWidth + ColumnGap + Middle + ColumnGap, Top), ImVec2(InspectorWidth, Height)); }
    }
    { SolidArcPhaseClock Clock(Profile, "status bar"); DrawStatusBar(State, Frame); }
    DrawMatcapPicker(State);
    DrawWelcome(State);
    { SolidArcPhaseClock Clock(Profile, "trace panel"); DrawTracePanel(State); }
    { SolidArcPhaseClock Clock(Profile, "shortcuts"); ApplyShortcuts(State); }
    { SolidArcPhaseClock Clock(Profile, "autosave"); AdvanceAutosave(State, DeltaSeconds); }
    { SolidArcPhaseClock Clock(Profile, "bevel preview"); StageBevelPreview(State); }
    State.FrameNumber += 1;
    Profile.InterfaceMs = (SteadySeconds() - Entered) * 1.0e3;
}

const SolidArcScene& DisplayedSolidArcScene(const SolidArcWorkspaceState& State)
{
    if (State.Bevel.ActiveCondition && State.Bevel.Preview && State.Bevel.Refusal.empty()) return State.Bevel.PreviewScene;
    if (State.Transform.ActiveCondition && (State.Transform.Preview || State.Transform.RigidCondition) && State.Transform.Refusal.empty()) return State.Transform.PreviewScene;
    if (State.Sweep.ActiveCondition && State.Sweep.Preview && State.Sweep.Refusal.empty()) return State.Sweep.PreviewScene;
    if (State.Knife.ActiveCondition && State.Knife.Preview && State.Knife.Refusal.empty()) return State.Knife.PreviewScene;
    if (State.Joint.ActiveCondition && State.Joint.Preview && State.Joint.Refusal.empty()) return State.Joint.PreviewScene;
    if (State.ParameterDrag.Preview) return State.ParameterDrag.PreviewScene;
    if (State.Gizmo.DragCondition && (State.Gizmo.Preview || State.Gizmo.RigidCondition) && State.Gizmo.Refusal.empty()) return State.Gizmo.PreviewScene;
    if (State.SketchPoints.DragCondition && State.SketchPoints.Preview && State.SketchPoints.Refusal.empty()) return State.SketchPoints.PreviewScene;
    return State.Scene;
}

void RefreshSolidArcMatcaps(SolidArcWorkspaceState& State)
{
    SolidArcMatcapShelf& Shelf = State.Matcaps;
    if (!Shelf.Library.Names.empty() && Shelf.Library.Key == SolidArcMatcapKey(State.Document.Appearance)) return;
    Shelf.Library = AssembleSolidArcMatcaps(State.Document.Appearance);
    Shelf.Generation += 1;
}

SolidArcSceneStates DisplayedSolidArcStates(const SolidArcWorkspaceState& State)
{
    const SolidArcScene& Scene = DisplayedSolidArcScene(State);
    SolidArcSceneStates States = &Scene == &State.Bevel.PreviewScene ? ComposeSolidArcStates(Scene, State.Bevel.Preview->Selection, State.HoverIdentity)
                               : &Scene == &State.Transform.PreviewScene ? ComposeSolidArcStates(Scene, State.Transform.Preview ? State.Transform.Preview->Selection : State.Document.Selection, 0)
                               : &Scene == &State.Sweep.PreviewScene ? ComposeSolidArcStates(Scene, State.Sweep.Preview->Selection, 0)
                               : &Scene == &State.Joint.PreviewScene ? ComposeSolidArcStates(Scene, State.Joint.Preview->Selection, 0)
                               : &Scene == &State.ParameterDrag.PreviewScene ? ComposeSolidArcStates(Scene, State.ParameterDrag.Preview->Selection, 0)
                               : &Scene == &State.Gizmo.PreviewScene ? ComposeSolidArcStates(Scene, State.Gizmo.Preview ? State.Gizmo.Preview->Selection : State.Document.Selection, 0)
                                                                      : ComposeSolidArcStates(Scene, State.Document.Selection, State.ViewportHovered ? State.HoverIdentity : 0);
    // 📝 Matcap layers ride in bits 8 … 15 of each face word (SolidArcSurface.slang).
    const std::vector<uint32_t> Layers = ComposeSolidArcMatcapLayers(Scene, State.Document.Appearance, State.Matcaps.Library);
    for (size_t Index = 0; Index < States.Faces.size() && Index < Layers.size(); ++Index) States.Faces[Index] |= Layers[Index];
    // 📝 Sheet faces carry StateSheetFace (bit 16) so a section never caps an open panel.
    if (State.Style.SectionEnabled)
        for (size_t Index = 0; Index < States.Faces.size() && Index < State.Section.Clip.SheetFaces.size(); ++Index)
            if (State.Section.Clip.SheetFaces[Index]) States.Faces[Index] |= StateSheetFace;
    return States;
}

void RefreshSolidArcScene(SolidArcWorkspaceState& State, bool DirtyCondition)
{
    SolidArcPhaseClock Clock(State.Profile, "scene rebuild");
    State.Scene = BuildSolidArcScene(State.Document, 1.5e-3, State.ZebraEnabled);
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

std::string SolidArcPreviousRecoveryPath(const SolidArcOptions& Options)
{
    const std::string& Path   = Options.RecoveryPath;
    const std::string  Suffix = ".solidarc";
    if (Path.size() > Suffix.size() && Path.compare(Path.size() - Suffix.size(), Suffix.size(), Suffix) == 0)
        return Path.substr(0, Path.size() - Suffix.size()) + ".previous" + Suffix;
    return Path + ".previous";
}

bool ParseSolidArcLength(const std::string& Text, double& Value)
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

std::string SolidArcConstraintSketch(const SolidArcWorkspaceState& State)
{
    return ConstraintSketchLabel(State);
}

std::string DescribeSolidArcWorkspace(const SolidArcWorkspaceState& State)
{
    const SolidArcDocument&    Document = State.Document;
    const SolidArcBodySummary& Summary  = Document.Summary;
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
        AppendJsonText(Stream, DescribeSolidArcStep(Document, Step));
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
    AppendJsonText(Stream, State.HoverIdentity ? NameSolidArcIdentity(DisplayedSolidArcScene(State), State.HoverIdentity) : std::string());
    Stream << ",\n  \"Camera\": {\"Azimuth\": " << State.Camera.Azimuth << ", \"Elevation\": " << State.Camera.Elevation << ", \"Distance\": " << State.Camera.Distance
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
    Stream << ", \"Combination\": " << int(State.Bevel.Combination) << ", \"Depth\": " << State.Bevel.Depth << ", \"DepthPointer\": " << (State.Bevel.DepthEnabled ? "true" : "false") << "},\n";
    Stream << "  \"Drawing\": {\"Active\": " << (State.Drawing.ActiveCondition ? "true" : "false") << ", \"Kind\": ";
    AppendJsonText(Stream, SolidArcDrawingLabel(State.Drawing.Kind));
    Stream << ", \"Plane\": " << State.Drawing.GridPlane << ", \"Form\": ";
    AppendJsonText(Stream, SketchSplineFormLabel(State.Drawing.Form));
    Stream << ", \"Centre\": " << (State.Drawing.CentreCondition ? "true" : "false") << ", \"Points\": [";
    for (size_t Index = 0; Index < State.Drawing.Points.size(); ++Index)
        Stream << (Index ? ", " : "") << "[" << State.Drawing.Points[Index].X << ", " << State.Drawing.Points[Index].Y << "]";
    Stream << "], \"Solid\": " << (State.Drawing.SolidCondition ? "true" : "false") << ", \"SolidKind\": ";
    AppendJsonText(Stream, SolidArcConstructLabel(State.Drawing.Solid));
    Stream << ", \"Lift\": " << State.Drawing.Lift << ", \"Elevation\": " << State.Drawing.Elevation << ", \"Sides\": " << State.Drawing.Sides
           << ", \"Combination\": " << int(State.Drawing.Combination) << ", \"Refusal\": ";
    AppendJsonText(Stream, State.Drawing.Refusal);
    Stream << "},\n  \"Undo\": " << Document.UndoEntries.size() << ",\n  \"Redo\": " << Document.RedoEntries.size() << ",\n  \"Welcome\": "
           << (State.WelcomeOpen ? "true" : "false") << ",\n  \"Status\": ";
    AppendJsonText(Stream, State.Status);
    Stream << ",\n  \"Viewport\": [" << State.ViewportRect[0] << ", " << State.ViewportRect[1] << ", " << State.ViewportRect[2] << ", " << State.ViewportRect[3] << "]\n}\n";
    return Stream.str();
}

} // namespace Frontier::SolidArc
