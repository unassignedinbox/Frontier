/*====================================================================================================================================
                                                         SOLIDARCOUTLINER.CPP
====================================================================================================================================*/
// 🧩 Outliner panel, a 1:1 port of the HTML SolidArc Outliner (see header). Measurements are WorkspacePanel.css values;
//    each block names its selector so the port can be checked line by line against the style sheet.

//------------------------------------------------------------------------------------------------------------------------
//                                                    INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "UI/SolidArc/Interface/SolidArcOutliner.h"

#include "UI/SolidArc/Interface/SolidArcInterface.h"
#include "UI/SolidArc/Interface/SolidArcStyleSheet.h"

#include "Engine/Interface/ControlKit.h"

#include <algorithm>
#include <cctype>
#include <cstdio>

namespace Frontier::SolidArc
{

using namespace Frontier::GeometryProcessing;

namespace
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    CONSTANTS
//------------------------------------------------------------------------------------------------------------------------

constexpr uint32_t TextInk       = 0xeeeeee;                                                 // :root color
constexpr uint32_t MutedInk      = 0x858585;                                                 // --muted
constexpr uint32_t AmberInk      = 0xe5b369;                                                 // --amber (bodies)
constexpr uint32_t CyanInk       = 0x84bdc2;                                                 // --cyan (curves)
constexpr uint32_t BlueInk       = 0x7ba9d9;                                                 // --blue (surfaces)
constexpr uint32_t ActiveInk     = 0x33ff73;                                                 // .scene-row.active-selection
constexpr uint32_t SelectedInk   = 0x4099ff;                                                 // .scene-row.selected:not(.active-selection)
constexpr uint32_t HoverFill     = 0x292929;                                                 // button:hover
constexpr float    RowHeight     = 39.0f;                                                    // [px] - .scene-row
constexpr float    CompactHeight = 32.0f;                                                    // [px] - .compact .scene-row

//------------------------------------------------------------------------------------------------------------------------
//                                                    HELPERS
//------------------------------------------------------------------------------------------------------------------------

ColorQuad Ink(uint32_t Rgb, float Opacity = 1.0f)
{
    return SolidArcInk(Rgb, Opacity);
}

ColorQuad CategoryInk(SolidArcFigureCategory Category)
{
    switch (Category)
    {
    case SolidArcFigureCategory::Curve:   return Ink(CyanInk);                               // .colour-0
    case SolidArcFigureCategory::Surface: return Ink(BlueInk);                               // .colour-1
    default:                              return Ink(AmberInk);                              // .colour-2
    }
}

const char* CategoryTag(SolidArcFigureCategory Category)
{
    static const char* const Tags[] = { "CURVE", "SURFACE", "BODY" };
    return Tags[static_cast<int>(Category)];
}

std::string Lowered(std::string Text)
{
    for (char& Character : Text) Character = char(std::tolower(static_cast<unsigned char>(Character)));
    return Text;
}

std::string CaptionOfStep(const SolidArcDocument& Document, const std::string& Label)
{
    const auto Found = Document.Captions.find(Label);
    return Found != Document.Captions.end() && !Found->second.empty() ? Found->second : Label;
}

// WorkspacePanel.js FigureIcon: the glyph of a sketch follows its curves.
IconSymbol SketchIcon(const SketchModel& Sketch)
{
    int Lines = 0;
    for (const SketchCurve& Curve : Sketch.Curves)
    {
        if (Curve.Construction) continue;
        switch (Curve.Kind)
        {
        case SketchCurveKind::Circle: return IconSymbol::SolidArcCircle;
        case SketchCurveKind::Arc:    return IconSymbol::SolidArcArc;
        case SketchCurveKind::Spline:
            switch (Curve.Form)
            {
            case SketchSplineForm::Hermite: return IconSymbol::SolidArcHermite;
            case SketchSplineForm::Bezier:  return IconSymbol::SolidArcBezier;
            case SketchSplineForm::Nurbs:   return IconSymbol::SolidArcNurbs;
            default:                        return IconSymbol::SolidArcCurve;
            }
        case SketchCurveKind::Line: ++Lines; break;
        }
    }
    return Lines == 4 ? IconSymbol::SolidArcRectangle : Lines > 1 ? IconSymbol::SolidArcPolyline : IconSymbol::SolidArcEdge;
}

// .icon-button: 28 px circle, #888 glyph; hover #292929 / #fff.
bool IconButton(SolidArcWorkspaceState& State, SolidArcSheet& Sheet, const std::string& Key, const PlaneExtent& Extent, IconSymbol Symbol,
                float GlyphSize, ColorQuad Border = { 0.0f, 0.0f, 0.0f, 0.0f })
{
    const SolidArcPress Press = PressSolidArcRegion(State.Widgets, Key, Extent);
    Sheet.Box(Extent, Press.HoveredCondition ? Ink(HoverFill) : ColorQuad{ 0.0f, 0.0f, 0.0f, 0.0f }, 0.5f * Extent.Width(), Border);
    Sheet.Icon(Symbol, 0.5f * (Extent.MinimumX + Extent.MaximumX - GlyphSize), 0.5f * (Extent.MinimumY + Extent.MaximumY - GlyphSize), GlyphSize,
               Press.HoveredCondition ? Ink(0xffffff) : Ink(0x888888));
    return Press.ClickedCondition;
}

// The ↗ of the Construct launcher (DM Sans has no U+2197): a 45° stroke with its two head strokes.
void NorthEastArrow(SolidArcSheet& Sheet, float CentreX, float CentreY, ColorQuad Ink)
{
    const float Half = 3.5f;
    Sheet.Line(CentreX - Half, CentreY + Half, CentreX + Half, CentreY - Half, Ink, 1.1f);
    Sheet.Line(CentreX - 1.0f, CentreY - Half, CentreX + Half, CentreY - Half, Ink, 1.1f);
    Sheet.Line(CentreX + Half, CentreY - Half, CentreX + Half, CentreY + 1.0f, Ink, 1.1f);
}

} // namespace

//------------------------------------------------------------------------------------------------------------------------
//                                                    PUBLIC FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

std::vector<SolidArcFigure> CollectSolidArcFigures(const SolidArcWorkspaceState& State)
{
    const SolidArcDocument&  Document = State.Document;
    const SolidArcSelection& Picked   = Document.Selection;
    std::vector<SolidArcFigure> Figures;
    for (const SolidArcBodyFigures& Body : Document.Summary.Bodies)
    {
        SolidArcFigure Figure;
        Figure.Key             = Body.Key;
        Figure.Name            = Body.Caption;
        Figure.Category        = Body.SheetCondition ? SolidArcFigureCategory::Surface : SolidArcFigureCategory::Body;
        Figure.Source          = SolidArcFigureSource::Body;
        Figure.Icon            = Body.SheetCondition ? IconSymbol::SolidArcSurface : IconSymbol::SolidArcBody;
        Figure.HiddenCondition = Document.Appearance.HiddenBodies.contains(Body.Key);
        const auto Order = std::find(Picked.Bodies.begin(), Picked.Bodies.end(), Body.Key);
        Figure.Rank = Order == Picked.Bodies.end() ? -1 : int(Order - Picked.Bodies.begin());
        Figure.SelectedCondition = Figure.Rank >= 0;
        for (const std::string& Face : Picked.Faces)
            if (!Figure.SelectedCondition && SolidArcBodyOfName(Document, SolidArcSelectionKind::Face, Face) == Body.Key) Figure.SelectedCondition = true;
        for (const std::string& Edge : Picked.Edges)
            if (!Figure.SelectedCondition && SolidArcBodyOfName(Document, SolidArcSelectionKind::Edge, Edge) == Body.Key) Figure.SelectedCondition = true;
        Figures.push_back(std::move(Figure));
    }
    for (const FeatureStep& Step : Document.Tree.Steps)
    {
        SolidArcFigure Figure;
        Figure.Key      = Step.Label;
        Figure.Name     = CaptionOfStep(Document, Step.Label);
        Figure.Category = SolidArcFigureCategory::Curve;
        if (Step.Kind == FeatureStepKind::Sketch && !SolidArcSketchConsumedCondition(Document, Step.Label))
        {
            const auto Solved = Document.Regeneration.Sketches.find(Step.Label);
            Figure.Source            = SolidArcFigureSource::Sketch;
            Figure.Icon              = SketchIcon(Solved != Document.Regeneration.Sketches.end() ? Solved->second : Step.Sketch);
            Figure.SelectedCondition = State.FocusedStep == Step.Label;
            Figure.Rank              = Figure.SelectedCondition ? 0 : -1;
        }
        else if (Step.Kind == FeatureStepKind::SpaceCurve)
        {
            const auto Order = std::find(Picked.Curves.begin(), Picked.Curves.end(), Step.Label);
            Figure.Source            = SolidArcFigureSource::SpaceCurve;
            Figure.Icon              = IconSymbol::SolidArcNurbs;
            Figure.Rank              = Order == Picked.Curves.end() ? -1 : int(Order - Picked.Curves.begin());
            Figure.SelectedCondition = Figure.Rank >= 0;
        }
        else
        {
            continue;
        }
        Figures.push_back(std::move(Figure));
    }
    return Figures;
}

SolidArcOutlinerIntent ConstructSolidArcOutliner(SolidArcWorkspaceState& State, const PlaneExtent& Panel)
{
    SolidArcOutlinerIntent Intent;
    SolidArcSheet          Sheet;
    if (!Sheet.Begin()) return Intent;
    SolidArcOutlinerState&            Outliner = State.Outliner;
    const std::vector<SolidArcFigure> Figures  = CollectSolidArcFigures(State);
    const ColorQuad                   Clear    = { 0.0f, 0.0f, 0.0f, 0.0f };

    // .panel — border 1 px #ffffff08, --panel, radius 23; children sit in the content box.
    Sheet.Box(Panel, Ink(0x121212), SolidArcStyleTokens::PanelRadius, SolidArcWash(0x08));
    Sheet.Surface().PushClip(Panel);
    const float Left  = Panel.MinimumX + 1.0f;
    const float Top   = Panel.MinimumY + 1.0f;
    const float Width = Panel.Width() - 2.0f;
    const float Right = Left + Width;

    // .panel-heading — padding 20 19 16, min-height 64, gap 10: h2 18 px (−0.6 letter spacing), small 10 px, rows button.
    const float HeadingCentre = Top + 20.0f + 14.0f;
    float       Cursor        = Left + 19.0f;
    Cursor += Sheet.Text(Cursor, HeadingCentre, 18.0f, Ink(TextInk), "Outliner", SolidArcWeight::Light, -0.6f) + 10.0f;
    char Count[64];
    std::snprintf(Count, sizeof(Count), "Scene · %d figures", int(Figures.size()));
    Sheet.Text(Cursor, HeadingCentre, 10.0f, Ink(MutedInk), Count);
    if (IconButton(State, Sheet, "OutlinerCompact", Spanning(Right - 19.0f - 28.0f, HeadingCentre - 14.0f, 28.0f, 28.0f), IconSymbol::SolidArcRows, 17.0f))
        Outliner.CompactEnabled = !Outliner.CompactEnabled;
    float Y = Top + 64.0f;

    // .stat-tiles — 2 columns, gap 10, margin 0 12 12; tile 64 px, #1c1c1c, hairline, radius 18, padding 11 14.
    const float TileWidth = 0.5f * (Width - 24.0f - 10.0f);
    int Bodies = 0, Sketches = 0;
    for (const SolidArcFigure& Figure : Figures)
    {
        Bodies   += Figure.Category == SolidArcFigureCategory::Body;
        Sketches += Figure.Category == SolidArcFigureCategory::Curve;
    }
    const struct { IconSymbol Icon; uint32_t Tint; const char* Label; int Value; } Tiles[2] = {
        { IconSymbol::SolidArcBody, AmberInk, "Bodies", Bodies }, { IconSymbol::SolidArcCurve, CyanInk, "Sketches", Sketches } };
    for (int Index = 0; Index < 2; ++Index)
    {
        const PlaneExtent Tile = Spanning(Left + 12.0f + float(Index) * (TileWidth + 10.0f), Y, TileWidth, 64.0f);
        Sheet.Box(Tile, Ink(0x1c1c1c), 18.0f, SolidArcWash(0x0c));
        Sheet.Icon(Tiles[Index].Icon, Tile.MinimumX + 15.0f, Tile.MinimumY + 12.0f, 18.0f, Ink(Tiles[Index].Tint));
        Sheet.Text(Tile.MinimumX + 15.0f, Tile.MinimumY + 12.0f + 18.0f + 4.0f + 0.5f * 11.0f * SolidArcStyleTokens::FontScale, 11.0f, Ink(0x939393),
                   Tiles[Index].Label);
        char Value[16];
        std::snprintf(Value, sizeof(Value), "%d", Tiles[Index].Value);
        Sheet.TextRight(Tile.MaximumX - 1.0f - 15.0f, Tile.MaximumY - 1.0f - 9.0f - 0.5f * 26.0f * SolidArcStyleTokens::FontScale, 26.0f, Ink(TextInk), Value);
    }
    Y += 64.0f + 12.0f;

    // .search — 40 px pill, #222, #ffffff10 border, padding 0 12, gap 9: glyph 14, input 11 px #ccc, kbd 9 px #606060.
    const PlaneExtent Search = Spanning(Left + 12.0f, Y, Width - 24.0f, 40.0f);
    const float       SearchCentre = Y + 20.0f;
    Sheet.Box(Search, Ink(0x222222), 20.0f, SolidArcWash(0x10));
    Sheet.Icon(IconSymbol::SolidArcSearch, Search.MinimumX + 13.0f, SearchCentre - 7.0f, 14.0f, Ink(0x7c7c7c));
    const float KeyWidth = Sheet.TextRight(Search.MaximumX - 13.0f, SearchCentre, 9.0f, Ink(0x606060), "Ctrl F");
    {
        const float InputLeft  = Search.MinimumX + 13.0f + 14.0f + 9.0f;
        const float InputWidth = Search.MaximumX - 13.0f - KeyWidth - 9.0f - InputLeft;
        const float FontSize   = 11.0f * SolidArcStyleTokens::FontScale;
        ImGui::PushFont(static_cast<ImFont*>(QuerySolidArcTypefaces().Light), FontSize);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));                // #ccc
        ImGui::PushStyleColor(ImGuiCol_TextDisabled, ImVec4(0x8b / 255.0f, 0x8b / 255.0f, 0x8b / 255.0f, 1.0f)); // dark-scheme placeholder
        const ImVec2 Restore = ImGui::GetCursorScreenPos();
        ImGui::SetCursorScreenPos(ImVec2(InputLeft, SearchCentre - 0.5f * FontSize));
        ImGui::SetNextItemWidth(InputWidth);
        if (Outliner.SearchFocusRequest)
        {
            ImGui::SetKeyboardFocusHere();
            Outliner.SearchFocusRequest = false;
        }
        ImGui::InputTextWithHint("##SceneSearch", "Search scene…", Outliner.Search.data(), Outliner.Search.size());
        State.Widgets["SceneSearch"] = ImVec4(Search.MinimumX, Search.MinimumY, Search.MaximumX, Search.MaximumY);
        ImGui::SetCursorScreenPos(Restore);
        ImGui::PopStyleColor(5);
        ImGui::PopStyleVar(2);
        ImGui::PopFont();
    }
    Y += 40.0f;

    // .filter-chips — margin 10 12 11, gap 5; chip 10 px, padding 5 9, radius 14, #ffffff0d border; active #262626 / #ddd.
    Y += 10.0f;
    const float ChipHeight = 10.0f * SolidArcStyleTokens::FontScale + 10.0f + 2.0f;
    const struct { const char* Label; int Filter; } Chips[] = { { "All", -1 }, { "Bodies", 2 }, { "Sketches", 0 }, { "Surfaces", 1 } };
    Cursor = Left + 12.0f;
    for (const auto& Chip : Chips)
    {
        const float       ChipWidth = Sheet.Measure(Chip.Label, 10.0f) + 18.0f + 2.0f;
        const PlaneExtent Extent    = Spanning(Cursor, Y, ChipWidth, ChipHeight);
        const SolidArcPress Press   = PressSolidArcRegion(State.Widgets, std::string("Filter") + Chip.Label, Extent);
        const bool        Active    = Outliner.Filter == Chip.Filter;
        Sheet.Box(Extent, Press.HoveredCondition ? Ink(HoverFill) : Active ? Ink(0x262626) : Clear, 14.0f, SolidArcWash(0x0d));
        Sheet.Text(Cursor + 10.0f, Y + 0.5f * ChipHeight, 10.0f, Press.HoveredCondition ? Ink(0xffffff) : Active ? Ink(0xdddddd) : Ink(0x979797), Chip.Label);
        if (Press.ClickedCondition) Outliner.Filter = Chip.Filter;
        Cursor += ChipWidth + 5.0f;
    }
    Y += ChipHeight + 11.0f;

    // .collection — margin 0 12 8, 11 px: fold text-button (⌄, folder glyph 13, caption) and the 24 px + button.
    const float CollectionHeight = 11.0f * SolidArcStyleTokens::FontScale + 12.0f;
    const float CollectionCentre = Y + 0.5f * CollectionHeight;
    {
        const float ChevronWidth = Sheet.Measure("⌄ \xC2\xA0", 11.0f);
        const float CaptionWidth = Sheet.Measure("Geometry collection", 11.0f);
        const PlaneExtent Fold   = Spanning(Left + 12.0f, Y, 8.0f + ChevronWidth + 8.0f + 13.0f + 8.0f + CaptionWidth + 8.0f, CollectionHeight);
        const SolidArcPress Press = PressSolidArcRegion(State.Widgets, "OutlinerFold", Fold);
        const ColorQuad   Tint   = Press.HoveredCondition ? Ink(0xffffff) : Ink(0x969696);
        Sheet.Box(Fold, Press.HoveredCondition ? Ink(HoverFill) : Clear, 16.0f);
        float FoldCursor = Fold.MinimumX + 8.0f;
        FoldCursor += Sheet.Text(FoldCursor, CollectionCentre, 11.0f, Tint, Outliner.FoldedEnabled ? "› \xC2\xA0" : "⌄ \xC2\xA0") + 8.0f;
        Sheet.Icon(IconSymbol::SolidArcFolder, FoldCursor, CollectionCentre - 6.5f, 13.0f, Tint);
        Sheet.Text(FoldCursor + 13.0f + 8.0f, CollectionCentre, 11.0f, Tint, "Geometry collection");
        if (Press.ClickedCondition) Outliner.FoldedEnabled = !Outliner.FoldedEnabled;
        if (IconButton(State, Sheet, "OutlinerAdd", Spanning(Right - 12.0f - 24.0f, CollectionCentre - 12.0f, 24.0f, 24.0f), IconSymbol::SolidArcPlus, 13.0f,
                       SolidArcWash(0x08)))
            Intent.Verb = SolidArcOutlinerVerb::Construct;
    }
    Y += CollectionHeight + 8.0f;

    // #scene-rows — min 80 px, max 35vh, scrolls; .scene-row 39 px (compact 32), margin 0 10, padding 0 9 0 14, gap 9.
    const std::string Query = Lowered(Outliner.Search.data());
    std::vector<const SolidArcFigure*> Shown;
    for (const SolidArcFigure& Figure : Figures)
        if ((Outliner.Filter < 0 || static_cast<int>(Figure.Category) == Outliner.Filter) && Lowered(Figure.Name).find(Query) != std::string::npos)
            Shown.push_back(&Figure);
    const float Pitch      = Outliner.CompactEnabled ? CompactHeight : RowHeight;
    const float Limit      = 0.35f * ImGui::GetIO().DisplaySize.y;
    const float EmptyBlock = 18.0f + 2.0f * 18.0f + 18.0f;                                   // .empty-note: padding 18, two 1.8 em lines
    const float RowsHeight = Outliner.FoldedEnabled ? 0.0f : std::clamp(Shown.empty() ? EmptyBlock : Pitch * float(Shown.size()), 80.0f, Limit);
    if (RowsHeight > 0.0f)
    {
        const ImVec2 Restore = ImGui::GetCursorScreenPos();
        ImGui::SetCursorScreenPos(ImVec2(Left, Y));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 6.0f);
        ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, ImVec4(1, 1, 1, 0.12f));
        ImGui::BeginChild("SceneRows", ImVec2(Width, RowsHeight), ImGuiChildFlags_None, ImGuiWindowFlags_NoBackground);
        SolidArcSheet Rows;
        Rows.Begin();
        const float RowsTop = ImGui::GetCursorScreenPos().y;
        if (Shown.empty())
        {
            Rows.Text(Left + 18.0f, RowsTop + 18.0f + 9.0f, 10.0f, Ink(MutedInk), "No matching geometry.");
            Rows.Text(Left + 18.0f, RowsTop + 18.0f + 27.0f, 10.0f, Ink(MutedInk), "Construct a shape or open a document.");
        }
        for (size_t Index = 0; Index < Shown.size(); ++Index)
        {
            const SolidArcFigure& Figure  = *Shown[Index];
            const PlaneExtent     Row     = Spanning(Left + 10.0f, RowsTop + float(Index) * Pitch, Width - 20.0f, Pitch);
            const float           Centre  = 0.5f * (Row.MinimumY + Row.MaximumY);
            const float           Opacity = Figure.HiddenCondition ? 0.45f : 1.0f;            // .hidden-figure
            const PlaneExtent     Eye     = Spanning(Row.MaximumX - 1.0f - 9.0f - 23.0f, Centre - 11.5f, 23.0f, 23.0f);
            const SolidArcPress   Press   = PressSolidArcRegion(State.Widgets, "Figure:" + Figure.Key, PlaneExtent{ Row.MinimumX, Row.MinimumY, Eye.MinimumX - 4.0f, Row.MaximumY });
            const bool            Active  = Figure.SelectedCondition && Figure.Rank == 0;
            const ColorQuad       Accent  = Active ? Ink(ActiveInk) : Ink(SelectedInk);
            if (Figure.SelectedCondition)
            {
                Rows.Box(Row, ControlKit::Faded(SolidArcWash(0x16), Opacity), 2.0f, ControlKit::Faded(SolidArcWash(0x20), Opacity));
                Rows.Surface().FillRectangle(Spanning(Row.MinimumX, Row.MinimumY, 3.0f, Pitch), ControlKit::Faded(Accent, Opacity));   // :before rail
            }
            else if (Press.HoveredCondition)
            {
                Rows.Box(Row, SolidArcWash(0x04), 2.0f);
            }
            float RowCursor = Row.MinimumX + 1.0f + 14.0f;
            Rows.Icon(Figure.Icon, RowCursor + 4.5f, Centre - 8.5f, 17.0f, ControlKit::Faded(Figure.SelectedCondition ? Accent : CategoryInk(Figure.Category), Opacity));
            RowCursor += 26.0f + 9.0f;
            char Tag[32];
            if (Figure.Rank >= 0) std::snprintf(Tag, sizeof(Tag), "%d %s", Figure.Rank + 1, Active ? "ACTIVE" : "SELECTED");
            else std::snprintf(Tag, sizeof(Tag), "%s", CategoryTag(Figure.Category));
            const float TagWidth = Rows.TextRight(Eye.MinimumX - 9.0f, Centre, 8.0f, ControlKit::Faded(Figure.SelectedCondition ? Accent : Ink(0x666666), Opacity), Tag,
                                                  SolidArcWeight::Light, 0.5f);
            Rows.TextClipped(RowCursor, Centre, Eye.MinimumX - 9.0f - TagWidth - 9.0f - RowCursor, 11.0f, ControlKit::Faded(Ink(TextInk), Opacity), Figure.Name.c_str());
            const SolidArcPress Toggle = PressSolidArcRegion(State.Widgets, "Visibility:" + Figure.Key, Eye, Figure.Source == SolidArcFigureSource::Body);
            if (Toggle.HoveredCondition) Rows.Box(Eye, Ink(HoverFill), 11.5f);
            const ColorQuad EyeInk = Figure.Source != SolidArcFigureSource::Body ? Ink(0x858585, 0.3f) : Toggle.HoveredCondition ? Ink(0xffffff) : Ink(0x858585);
            Rows.Icon(Figure.HiddenCondition ? IconSymbol::SolidArcHidden : IconSymbol::SolidArcEye, Eye.MinimumX + 4.5f, Eye.MinimumY + 4.5f, 14.0f,
                      ControlKit::Faded(EyeInk, Opacity));
            if (Toggle.ClickedCondition)
            {
                Intent.Verb   = SolidArcOutlinerVerb::ToggleVisibility;
                Intent.Figure = Figure;
            }
            else if (Press.ClickedCondition)
            {
                Intent.Verb              = SolidArcOutlinerVerb::Pick;
                Intent.Figure            = Figure;
                Intent.AdditiveCondition = ImGui::GetIO().KeyShift || ImGui::GetIO().KeyCtrl;
            }
        }
        ImGui::SetCursorScreenPos(ImVec2(Left, RowsTop));
        ImGui::Dummy(ImVec2(1.0f, std::max(Shown.empty() ? EmptyBlock : Pitch * float(Shown.size()), 1.0f)));
        ImGui::EndChild();
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(2);
        ImGui::SetCursorScreenPos(Restore);
    }
    Y += RowsHeight;

    // .outliner-caption — padding 16 21, gap 6, 9 px #6d6d6d, link glyph 12.
    Sheet.Icon(IconSymbol::SolidArcLink, Left + 21.0f, Y + 16.0f, 12.0f, Ink(0x6d6d6d));
    Sheet.Text(Left + 21.0f + 12.0f + 6.0f, Y + 16.0f + 6.0f, 9.0f, Ink(0x6d6d6d), "Native B-rep · editable construction");
    Y += 16.0f + 12.0f + 16.0f;

    // .panel-footer — min-height 32 at the bottom, #ffffff0a top border, padding 8 13, 9 px #737373, shield glyph 11.
    const float FooterTop = Panel.MaximumY - 1.0f - 32.0f;
    Sheet.Line(Left, FooterTop + 0.5f, Right, FooterTop + 0.5f, SolidArcWash(0x0a));
    Sheet.Icon(IconSymbol::SolidArcShield, Left + 13.0f, FooterTop + 16.0f - 5.5f, 11.0f, Ink(0x737373));
    Sheet.Text(Left + 13.0f + 11.0f + 6.0f, FooterTop + 16.0f, 9.0f, Ink(0x737373), "Native geometry · Vulkan viewport");

    // .construct-library — #ffffff08 top border, padding 16 12; .construct-launcher #202020, #ffffff12, radius 14, padding 16 12.
    Sheet.Line(Left, Y + 0.5f, Right, Y + 0.5f, SolidArcWash(0x08));
    const float       LineOne  = 13.0f * SolidArcStyleTokens::FontScale;
    const float       LineTwo  = 10.0f * SolidArcStyleTokens::FontScale;
    const PlaneExtent Launcher = Spanning(Left + 12.0f, Y + 1.0f + 16.0f, Width - 24.0f, LineOne + 7.0f + LineTwo + 32.0f + 2.0f);
    if (Launcher.MaximumY < FooterTop)
    {
        const SolidArcPress Press = PressSolidArcRegion(State.Widgets, "ConstructLauncher", Launcher);
        const ColorQuad     Tint  = Press.HoveredCondition ? Ink(0xffffff) : Ink(0xe0e0e0);
        const float         Centre = 0.5f * (Launcher.MinimumY + Launcher.MaximumY);
        Sheet.Box(Launcher, Press.HoveredCondition ? Ink(HoverFill) : Ink(0x202020), 14.0f, SolidArcWash(0x12));
        Sheet.Icon(IconSymbol::SolidArcConstruct, Launcher.MinimumX + 13.0f, Centre - 8.5f, 17.0f, Tint);
        const float TextLeft = Launcher.MinimumX + 13.0f + 17.0f + 9.0f;
        const float TextTop  = Launcher.MinimumY + 17.0f;
        Sheet.Text(TextLeft, TextTop + 0.5f * LineOne, 13.0f, Tint, "Construct geometry");
        Sheet.Text(TextLeft, TextTop + LineOne + 7.0f + 0.5f * LineTwo, 10.0f, Ink(0x898989), "Block · Cylinder · Sphere · Torus");
        NorthEastArrow(Sheet, Launcher.MaximumX - 13.0f - 4.0f, Centre, Tint);
        if (Press.ClickedCondition) Intent.Verb = SolidArcOutlinerVerb::Construct;
    }
    Sheet.Surface().PopClip();
    return Intent;
}

} // namespace Frontier::SolidArc
