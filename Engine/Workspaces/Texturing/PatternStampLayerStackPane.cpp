/*============================================================================================================================================
                                                        PATTERNSTAMPLAYERSTACKPANE.CPP
============================================================================================================================================*/
// 🧩 Implementation of PatternStampLayerStackPane in Project Zero studio outliner styling wired live to PatternStampWorkspaceSession

//------------------------------------------------------------------------------------------------------------------------
//                                                        INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Engine/Workspaces/Texturing/PatternStampLayerStackPane.h"
#include "Engine/Workspaces/Texturing/StudioSurfaceKit.h"

#include <imgui.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <string>
#include <string_view>

namespace Frontier::Workspaces
{

using namespace StudioSurface;

namespace
{

[[nodiscard]] bool FoldedContains(std::string_view Haystack, std::string_view Needle) noexcept
{
    if (Needle.empty()) return true;
    if (Needle.size() > Haystack.size()) return false;
    for (std::size_t Pos = 0u; Pos + Needle.size() <= Haystack.size(); ++Pos)
    {
        bool Match = true;
        for (std::size_t Off = 0u; Off < Needle.size(); ++Off)
        {
            if (std::tolower(static_cast<unsigned char>(Haystack[Pos + Off])) !=
                std::tolower(static_cast<unsigned char>(Needle[Off])))
            {
                Match = false;
                break;
            }
        }
        if (Match) return true;
    }
    return false;
}

[[nodiscard]] ControlIcon ResolveLayerCategoryIcon(const PatternStampLayerSpec& Layer) noexcept
{
    switch (Layer.Category)
    {
        case PatternStampLayerCategory::Folder:       return ControlIcon::FolderClosed;
        case PatternStampLayerCategory::Typographic:  return ControlIcon::TextType;
        case PatternStampLayerCategory::GradientWash: return ControlIcon::PointGradient;
        case PatternStampLayerCategory::Motif:
        default:
            switch (Layer.MotifShape)
            {
                case PatternStampMotifShape::Hexagon:  return ControlIcon::ShapeHexagon;
                case PatternStampMotifShape::Square:
                case PatternStampMotifShape::RoundedBox: return ControlIcon::ShapeSquare;
                case PatternStampMotifShape::Diamond:  return ControlIcon::ShapeDiamond;
                case PatternStampMotifShape::Triangle: return ControlIcon::ShapeTriangle;
                case PatternStampMotifShape::Star:     return ControlIcon::ShapeStar;
                case PatternStampMotifShape::StripeBar:return ControlIcon::ShapeStripe;
                default:                               return ControlIcon::ShapeCircle;
            }
    }
}

[[nodiscard]] ControlIcon ResolveModifierIcon(PatternStampModifierCategory Category) noexcept
{
    switch (Category)
    {
        case PatternStampModifierCategory::NoiseScale:     return ControlIcon::NoiseWave;
        case PatternStampModifierCategory::GridArray:      return ControlIcon::LayoutGrid;
        case PatternStampModifierCategory::RadialArray:    return ControlIcon::SparklesAntiAliasing;
        case PatternStampModifierCategory::MirrorSymmetry: return ControlIcon::LayoutSplit;
        default:                                           return ControlIcon::ModifierStar;
    }
}

[[nodiscard]] std::string_view ResolveBlendTitle(uint8_t BlendMode) noexcept
{
    constexpr std::array<std::string_view, 6u> Titles {{ "Normal", "Multiply", "Screen", "Overlay", "Add", "Darken" }};
    return Titles[static_cast<std::size_t>(BlendMode) % Titles.size()];
}

} // namespace

PatternStampLayerStackPane::~PatternStampLayerStackPane() noexcept
{
    WorkspaceAppearance::ReleasePanePointerOwnership(this);
}

void PatternStampLayerStackPane::AttachSession(PatternStampWorkspaceSession* TargetSession) noexcept
{
    Session         = TargetSession;
    ScrollOffsetY   = 0.0f;
    RenamingLayerId = 0u;
}

void PatternStampLayerStackPane::PresentInImGuiPane(double DeltaSeconds, float ScaleFactor)
{
    const float  SafeScale = std::max(0.25f, ScaleFactor);
    const ImVec2 WinPos    = ImGui::GetWindowPos();
    const ImVec2 WinSize   = ImGui::GetWindowSize();
    const ImVec2 CursorPos = ImGui::GetCursorScreenPos();
    const float  TopY      = CursorPos.y - ImGui::GetStyle().WindowPadding.y;
    const float  PaneH     = std::max(1.0f, (WinPos.y + WinSize.y) - TopY);

    ImGui::InvisibleButton(
        "##PatternStampLayersCanvas",
        ImVec2(std::max(1.0f, ImGui::GetContentRegionAvail().x), std::max(1.0f, ImGui::GetContentRegionAvail().y)),
        ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);

    const Rectangle2D ScreenBounds { WinPos.x, TopY, WinSize.x, PaneH };
    const Rectangle2D LogicalBounds{ 0.0f, 0.0f, WinSize.x / SafeScale, PaneH / SafeScale };

    PixelSpace       Surface(ImGui::GetWindowDrawList(), { ScreenBounds.X, ScreenBounds.Y }, SafeScale);
    PaneInputReading Input = WorkspaceAppearance::CaptureImGuiPaneInput(ScreenBounds, SafeScale);
    WorkspaceAppearance::EnforcePanePointerOwnership(this, Input);

    Render(Surface, LogicalBounds, Input, DeltaSeconds);
    WorkspaceAppearance::UpdatePanePointerOwnership(this, ScreenBounds, SafeScale, Input, false);
}

void PatternStampLayerStackPane::Render(
    PixelSpace&             Surface,
    Rectangle2D             Bounds,
    const PaneInputReading& Input,
    double                  /*DeltaSeconds*/)
{
    const StudioToneSet Tone      = QueryStudioToneSet();
    const Rectangle2D   ShellRect = ConstructDockedSurfaceShell(Surface, Bounds, true, Tone);
    if (Session == nullptr)
    {
        return;
    }

    const bool PatternMode = (Session->Mode == PatternStampWorkspaceMode::Pattern);
    auto& Doc = Session->Document;

    constexpr float PadX    = 12.0f;
    constexpr float FooterH = 46.0f;
    const float     InnerW  = std::max(60.0f, ShellRect.Width - PadX * 2.0f);
    const float     LeftX   = ShellRect.X + PadX;
    float           CursorY = ShellRect.Y + 13.0f;

    // 1. Outliner Heading ("Layers" + "Pattern Studio · seamless tile" / "Stamp Studio · pbr decal" + round Folder/+ button)
    const std::string_view ModeSub = PatternMode ? "Pattern Studio · seamless tile" : "Stamp Studio · pbr decal";
    AddButtonAnchor = { LeftX + InnerW - 28.0f, CursorY + 4.0f, 28.0f, 28.0f };
    if (ConstructOutlinerHeading(
            Surface,
            { LeftX, CursorY, InnerW, 38.0f },
            "Layers",
            ModeSub,
            ControlIcon::FolderPlus,
            Input,
            Tone,
            Doc.Layers.size() >= PatternStampWorkspaceSession::MaxLayerLimit))
    {
        AddLayerSheetOpen = !AddLayerSheetOpen;
    }
    CursorY += 46.0f;

    // 2. Stat Tile Pair: VISIBLE & MODIFIERS
    const std::size_t VisibleLayers = Session->CountVisibleLayers();
    std::size_t NonFolderCount = 0u;
    std::size_t TotalMods      = 0u;
    for (const auto& L : Doc.Layers)
    {
        if (L.Category != PatternStampLayerCategory::Folder) ++NonFolderCount;
        TotalMods += L.Modifiers.size();
    }
    const std::size_t EnabledMods = Session->CountEnabledModifiers();

    char VisBuf[24] {};
    char ModBuf[24] {};
    std::snprintf(VisBuf, sizeof(VisBuf), "%zu/%zu", VisibleLayers, std::max<std::size_t>(1u, NonFolderCount));
    std::snprintf(ModBuf, sizeof(ModBuf), "%zu/%zu", EnabledMods, TotalMods);

    const OutlinerStatTileSpec VisStat {
        "VISIBLE",
        VisBuf,
        ControlIcon::EyeVisible,
        VisibleLayers == NonFolderCount && NonFolderCount > 0u,
        false,
        Session->LayerFilter == PatternStampLayerFilterMode::VisibleOnly,
    };
    const OutlinerStatTileSpec ModStat {
        "MODIFIERS",
        ModBuf,
        ControlIcon::ModifierStar,
        EnabledMods > 0u,
        false,
        FilterModsOnly,
    };
    const int32_t StatClicked = ConstructStatTilePair(Surface, { LeftX, CursorY, InnerW, 54.0f }, VisStat, ModStat, Input, Tone);
    if (StatClicked == 0 && !AddLayerSheetOpen)
    {
        Session->LayerFilter = (Session->LayerFilter == PatternStampLayerFilterMode::VisibleOnly)
            ? PatternStampLayerFilterMode::AllLayers
            : PatternStampLayerFilterMode::VisibleOnly;
    }
    else if (StatClicked == 1 && !AddLayerSheetOpen)
    {
        FilterModsOnly = !FilterModsOnly;
    }
    CursorY += 62.0f;

    // 3. Search + Filter row
    const bool FilterArmed = (Session->LayerFilter != PatternStampLayerFilterMode::AllLayers) || FilterModsOnly;
    if (ConstructSearchFilterRow(
            Surface,
            { LeftX, CursorY, InnerW, 32.0f },
            SearchBuffer,
            "Search layers, modifiers...",
            FilterArmed ? "Filtered" : "Filter",
            FilterArmed,
            Input,
            Tone) && !AddLayerSheetOpen)
    {
        if (Session->LayerFilter == PatternStampLayerFilterMode::AllLayers)
        {
            Session->LayerFilter = PatternStampLayerFilterMode::VisibleOnly;
        }
        else if (Session->LayerFilter == PatternStampLayerFilterMode::VisibleOnly)
        {
            Session->LayerFilter = PatternStampLayerFilterMode::HiddenOnly;
        }
        else
        {
            Session->LayerFilter = PatternStampLayerFilterMode::AllLayers;
            FilterModsOnly       = false;
        }
    }
    CursorY += 40.0f;

    // 4. Quick-add layer chips (+ Motif / + Text / + Gradient)
    const bool FullStack = (Doc.Layers.size() >= PatternStampWorkspaceSession::MaxLayerLimit);
    const std::array<OutlinerChipSpec, 3u> AddChips {{
        { "Motif",    {}, {}, false, true, false, FullStack },
        { "Text",     {}, {}, false, true, false, FullStack },
        { "Gradient", {}, {}, false, true, false, FullStack },
    }};
    float ChipsH = 0.0f;
    const int32_t ChipClicked = ConstructQuickChipStrip(Surface, LeftX, CursorY, InnerW, AddChips, Input, Tone, ChipsH);
    if (!AddLayerSheetOpen)
    {
        if (ChipClicked == 0) Session->AddLayer(PatternStampLayerCategory::Motif);
        else if (ChipClicked == 1) Session->AddLayer(PatternStampLayerCategory::Typographic);
        else if (ChipClicked == 2) Session->AddLayer(PatternStampLayerCategory::GradientWash);
    }
    CursorY += ChipsH + 8.0f;

    Surface.StrokeLine({ ShellRect.X + 1.0f, CursorY }, { ShellRect.Right() - 1.0f, CursorY }, Tone.Hairline, 1.0f);
    CursorY += 6.0f;

    // 5. Scrollable Layer Tree
    const Rectangle2D FooterRect { ShellRect.X + 1.0f, ShellRect.Bottom() - FooterH, ShellRect.Width - 2.0f, FooterH };
    const Rectangle2D TreeRect   { ShellRect.X + 4.0f, CursorY, ShellRect.Width - 8.0f, std::max(40.0f, FooterRect.Y - CursorY - 4.0f) };

    const Point2D Cursor = QueryInputCursor(Input);
    if (!AddLayerSheetOpen && TreeRect.Contains(Cursor) && std::fabs(Input.WheelDeltaY) > 1.0e-4f)
    {
        ScrollOffsetY = std::clamp(ScrollOffsetY - Input.WheelDeltaY * 34.0f, 0.0f, MaxScrollY);
    }

    const float RowLeftX = TreeRect.X + 4.0f;
    const float RowW     = TreeRect.Width - 8.0f;
    const float StartY   = TreeRect.Y - ScrollOffsetY;
    float       RowY     = StartY;
    constexpr float RowH   = 37.0f;
    constexpr float RowGap = 2.0f;
    const std::string_view Query = QueryEditTextView(SearchBuffer);
    const auto& ModLabels = PatternStampWorkspaceSession::InspectModifierLabels();

    Surface.PushClip(TreeRect);

    // 5a. Canvas Root Row
    {
        char CanvasMeta[72] {};
        std::snprintf(CanvasMeta, sizeof(CanvasMeta), "%s · %u px · %s",
            PatternMode ? "Canvas tile" : "Decal",
            Session->ResolutionPx,
            Doc.GroundEnabled ? "ground on" : "no ground");

        HierarchyRowSpec CanvasRow {};
        CanvasRow.DepthLevel   = 0;
        CanvasRow.Title        = Doc.DocumentName;
        CanvasRow.Subtitle     = CanvasMeta;
        CanvasRow.LeadIcon     = PatternMode ? ControlIcon::PatternTile : ControlIcon::StampTool;
        CanvasRow.LeadTileFill = Tone.AmberWash;
        CanvasRow.LeadIconInk  = Tone.Amber;
        CanvasRow.Expandable   = true;
        CanvasRow.Expanded     = RootExpanded || !Query.empty();
        CanvasRow.TagLabel     = PatternMode ? "SEAMLESS" : "CUTOUT";
        CanvasRow.TagStyle     = PatternMode ? TagTone::Green : TagTone::Blue;

        if (RowY + RowH >= TreeRect.Y && RowY <= TreeRect.Bottom())
        {
            const auto Hit = ConstructHierarchyRow(Surface, { RowLeftX, RowY, RowW, RowH }, CanvasRow, Input, Tone);
            if ((Hit.RowClicked || Hit.ChevronClicked) && !AddLayerSheetOpen)
            {
                RootExpanded = !RootExpanded;
            }
        }
        RowY += RowH + RowGap;
    }

    // 5b. Layer stack rendered top -> bottom (Doc.Layers reverse order)
    if (RootExpanded || !Query.empty())
    {
        for (std::size_t Rev = Doc.Layers.size(); Rev > 0u; --Rev)
        {
            const std::size_t Idx = Rev - 1u;
            const auto& Layer = Doc.Layers[Idx];
            if (Session->QueryUnderCollapsedFolder(Layer.Identity)) continue;
            if (Session->LayerFilter == PatternStampLayerFilterMode::VisibleOnly && !Layer.Visible) continue;
            if (Session->LayerFilter == PatternStampLayerFilterMode::HiddenOnly && Layer.Visible) continue;
            if (FilterModsOnly && Layer.Modifiers.empty()) continue;
            if (!FoldedContains(Layer.Name, Query)) continue;

            const bool Folder   = (Layer.Category == PatternStampLayerCategory::Folder);
            const bool Selected = (Layer.Identity == Doc.SelectedLayerId);
            const int32_t Depth = 1 + static_cast<int32_t>(Session->QueryLayerDepth(Layer.Identity));

            std::size_t ActiveLayerMods = 0u;
            for (const auto& Mod : Layer.Modifiers)
            {
                if (Mod.Enabled) ++ActiveLayerMods;
            }

            char LayerMeta[96] {};
            if (Folder)
            {
                std::snprintf(LayerMeta, sizeof(LayerMeta), "Group · %zu layers", Session->CountFolderImmediateMembers(Layer.Identity));
            }
            else if (!Layer.Modifiers.empty())
            {
                const std::string_view Blend = ResolveBlendTitle(Layer.BlendMode);
                std::snprintf(LayerMeta, sizeof(LayerMeta), "%.*s · %zu/%zu mods",
                    static_cast<int>(Blend.size()), Blend.data(), ActiveLayerMods, Layer.Modifiers.size());
            }
            else
            {
                const std::string_view Blend = ResolveBlendTitle(Layer.BlendMode);
                std::snprintf(LayerMeta, sizeof(LayerMeta), "%.*s · %.0f%%",
                    static_cast<int>(Blend.size()), Blend.data(), Layer.Opacity * 100.0f);
            }

            const bool Expandable = Folder ? (Session->CountFolderImmediateMembers(Layer.Identity) > 0u) : true;
            const bool Expanded   = Folder ? !Layer.CollapsedFolder : (Layer.ExpandedInStack || (Selected && !Layer.Modifiers.empty()));

            HierarchyRowSpec LSpec {};
            LSpec.DepthLevel      = Depth;
            LSpec.Title           = Layer.Name;
            LSpec.Subtitle        = LayerMeta;
            LSpec.LeadIcon        = ResolveLayerCategoryIcon(Layer);
            LSpec.LeadTileFill    = Folder ? Tone.TileSurface : Layer.FillTint.ScaledAlpha(0.26f);
            LSpec.LeadIconInk     = Folder ? Tone.TextMuted : Layer.FillTint;
            LSpec.Expandable      = Expandable;
            LSpec.Expanded        = Expanded;
            LSpec.Selected        = Selected;
            LSpec.Muted           = !Layer.Visible;
            LSpec.LockedSign      = Layer.Locked;
            LSpec.TagLabel        = Layer.Mask.Enabled ? "MASK" : "";
            LSpec.TagStyle        = TagTone::Neutral;
            LSpec.TrailingSwitch  = TrailingSwitchKind::Visibility;
            LSpec.TrailingEnabled = Layer.Visible;

            const Rectangle2D LRect { RowLeftX, RowY, RowW, RowH };
            if (RowY + RowH >= TreeRect.Y && RowY <= TreeRect.Bottom())
            {
                const auto Hit = ConstructHierarchyRow(Surface, LRect, LSpec, Input, Tone);
                if (!AddLayerSheetOpen)
                {
                    if (Hit.TrailingToggled)
                    {
                        Session->ToggleLayerVisible(Layer.Identity);
                    }
                    else if (Hit.ChevronClicked)
                    {
                        if (Folder) Session->ToggleFolderCollapsed(Layer.Identity);
                        else Session->ToggleLayerExpandedDrawer(Layer.Identity);
                    }
                    else if (Hit.RowClicked)
                    {
                        Session->SelectLayer(Layer.Identity);
                    }
                }
            }
            RowY += RowH + RowGap;

            // Nested modifier + mask + surface detail rows under expanded non-folder layers
            if (!Folder && Expanded)
            {
                for (std::size_t MIdx = 0u; MIdx < Layer.Modifiers.size(); ++MIdx)
                {
                    const auto& Mod = Layer.Modifiers[MIdx];
                    const std::size_t CatIdx = static_cast<std::size_t>(Mod.Category);
                    const char* ModName = (CatIdx < ModLabels.size()) ? ModLabels[CatIdx] : "Modifier";
                    char ModMeta[48] {};
                    std::snprintf(ModMeta, sizeof(ModMeta), "#%zu · %s", MIdx + 1u, Mod.Enabled ? "Active" : "Bypassed");

                    HierarchyRowSpec MSpec {};
                    MSpec.DepthLevel      = Depth + 1;
                    MSpec.Title           = ModName;
                    MSpec.Subtitle        = ModMeta;
                    MSpec.LeadIcon        = ResolveModifierIcon(Mod.Category);
                    MSpec.LeadTileFill    = Tone.InsetSurface;
                    MSpec.LeadIconInk     = Mod.Enabled ? Tone.Amber : Tone.TextFaint;
                    MSpec.Muted           = !Mod.Enabled || !Layer.Visible;
                    MSpec.TrailingSwitch  = TrailingSwitchKind::GreenCheck;
                    MSpec.TrailingEnabled = Mod.Enabled;

                    if (RowY + RowH >= TreeRect.Y && RowY <= TreeRect.Bottom())
                    {
                        const auto Hit = ConstructHierarchyRow(Surface, { RowLeftX, RowY, RowW, RowH }, MSpec, Input, Tone);
                        if (!AddLayerSheetOpen)
                        {
                            if (Hit.TrailingToggled)
                            {
                                Session->PushUndoSnapshot();
                                if (auto* Mutable = Session->FindLayerById(Layer.Identity))
                                {
                                    if (MIdx < Mutable->Modifiers.size())
                                    {
                                        Mutable->Modifiers[MIdx].Enabled = !Mutable->Modifiers[MIdx].Enabled;
                                        Session->RecomputeSummaryMetrics();
                                        Session->BumpRevision();
                                    }
                                }
                            }
                            else if (Hit.RowClicked)
                            {
                                Session->SelectLayer(Layer.Identity);
                            }
                        }
                    }
                    RowY += RowH + RowGap;
                }

                // Surface PBR channel summary nested row
                char SurfMeta[48] {};
                std::snprintf(SurfMeta, sizeof(SurfMeta), "%zu PBR channels", Layer.Channels.size());
                HierarchyRowSpec SurfSpec {};
                SurfSpec.DepthLevel   = Depth + 1;
                SurfSpec.Title        = "Surface";
                SurfSpec.Subtitle     = SurfMeta;
                SurfSpec.LeadIcon     = ControlIcon::LayersSlabs;
                SurfSpec.LeadTileFill = Tone.InsetSurface;
                SurfSpec.LeadIconInk  = Tone.TextMuted;
                SurfSpec.Muted        = !Layer.Visible;
                if (RowY + RowH >= TreeRect.Y && RowY <= TreeRect.Bottom())
                {
                    const auto Hit = ConstructHierarchyRow(Surface, { RowLeftX, RowY, RowW, RowH }, SurfSpec, Input, Tone);
                    if (Hit.RowClicked && !AddLayerSheetOpen)
                    {
                        Session->SelectLayer(Layer.Identity);
                    }
                }
                RowY += RowH + RowGap;
            }
        }
    }

    // 5c. Collapsible Starters Library Folder (all 6 Pattern or Stamp starter presets)
    {
        const auto& Presets = Session->QueryPresetDirectory();
        char StarterMeta[48] {};
        std::snprintf(StarterMeta, sizeof(StarterMeta), "%zu %s starters", Presets.size(), PatternMode ? "pattern" : "stamp");

        HierarchyRowSpec StartHead {};
        StartHead.DepthLevel   = 0;
        StartHead.Title        = "Starters";
        StartHead.Subtitle     = StarterMeta;
        StartHead.LeadIcon     = ControlIcon::FolderClosed;
        StartHead.LeadTileFill = Tone.InsetSurface;
        StartHead.LeadIconInk  = Tone.TextMuted;
        StartHead.Expandable   = true;
        StartHead.Expanded     = StartersExpanded || !Query.empty();
        StartHead.GroupStyle   = true;

        if (RowY + RowH >= TreeRect.Y && RowY <= TreeRect.Bottom())
        {
            const auto Hit = ConstructHierarchyRow(Surface, { RowLeftX, RowY, RowW, RowH }, StartHead, Input, Tone);
            if ((Hit.RowClicked || Hit.ChevronClicked) && !AddLayerSheetOpen)
            {
                StartersExpanded = !StartersExpanded;
            }
        }
        RowY += RowH + RowGap;

        if (StartersExpanded || !Query.empty())
        {
            for (std::size_t P = 0u; P < Presets.size(); ++P)
            {
                const auto& Preset = Presets[P];
                if (!FoldedContains(Preset.Title, Query) && !FoldedContains(Preset.Subtitle, Query)) continue;
                const bool Active = (P == Session->QueryActivePresetIndex());

                HierarchyRowSpec PRow {};
                PRow.DepthLevel      = 1;
                PRow.Title           = Preset.Title;
                PRow.Subtitle        = Preset.Subtitle;
                PRow.LeadSolidSwatch = true;
                PRow.LeadTileFill    = Preset.SwatchA;
                PRow.Selected        = Active;
                PRow.TagLabel        = Active ? "ACTIVE" : "";
                PRow.TagStyle        = TagTone::Green;

                if (RowY + RowH >= TreeRect.Y && RowY <= TreeRect.Bottom())
                {
                    const auto Hit = ConstructHierarchyRow(Surface, { RowLeftX, RowY, RowW, RowH }, PRow, Input, Tone);
                    if (Hit.RowClicked && !Active && !AddLayerSheetOpen)
                    {
                        Session->LoadPresetByIndex(P);
                    }
                }
                RowY += RowH + RowGap;
            }
        }
    }

    Surface.PopClip();
    const float TotalHeight = (RowY - StartY) + 8.0f;
    MaxScrollY              = std::max(0.0f, TotalHeight - TreeRect.Height);
    ScrollOffsetY           = std::clamp(ScrollOffsetY, 0.0f, MaxScrollY);

    // 6. Four-Column Outliner Footer (LAYERS / INSTANCES / SURFACE / SEAM or MARGIN)
    char LayerCntBuf[16] {};
    char InstBuf[16] {};
    char CovBuf[16] {};
    std::snprintf(LayerCntBuf, sizeof(LayerCntBuf), "%zu", Doc.Layers.size());
    if (Session->InstancesEvaluated >= 1000)
    {
        std::snprintf(InstBuf, sizeof(InstBuf), "%.1fk", static_cast<float>(Session->InstancesEvaluated) / 1000.0f);
    }
    else
    {
        std::snprintf(InstBuf, sizeof(InstBuf), "%d", Session->InstancesEvaluated);
    }
    std::snprintf(CovBuf, sizeof(CovBuf), "%d", Session->SurfaceFractionPct);

    const std::array<FooterMetricColumn, 4u> FooterCols {{
        { "LAYERS",    LayerCntBuf, "/32", (Doc.Layers.size() >= PatternStampWorkspaceSession::MaxLayerLimit) ? TagTone::Amber : TagTone::Neutral },
        { "INSTANCES", InstBuf,     "",    TagTone::Neutral },
        { "SURFACE",   CovBuf,      "%",   TagTone::Neutral },
        PatternMode
            ? FooterMetricColumn{ "SEAM",   "Clean", "",   TagTone::Green   }
            : FooterMetricColumn{ "MARGIN", "14",    "px", TagTone::Neutral },
    }};
    ConstructFourColumnFooter(Surface, FooterRect, FooterCols, Tone);

    // 7. Project Zero "+ Layer" Popover Sheet (preserves AddLayerSheetOpen for Exhibit 06!)
    if (AddLayerSheetOpen)
    {
        const std::array<PopoverOptionSpec, 4u> LayerKinds {{
            { "Motif layer",    "Vector shape",    Tone.Amber  },
            { "Text block",     "Engraved label",  Tone.Blue   },
            { "Gradient wash",  "Tile fill field", Tone.Violet },
            { "Group folder",   "Nest layers",     Tone.Green  },
        }};
        bool Dismissed = false;
        const int32_t Picked = ConstructStudioPopoverList(
            Surface, AddButtonAnchor, ShellRect, "NEW LAYER", LayerKinds, -1, Input, Tone, Dismissed);
        if (Picked >= 0)
        {
            Session->AddLayer(static_cast<PatternStampLayerCategory>(Picked));
            AddLayerSheetOpen = false;
        }
        else if (Dismissed)
        {
            AddLayerSheetOpen = false;
        }
    }
}

} // namespace Frontier::Workspaces
