/*============================================================================================================================================
                                                        PATTERNSTAMPLAYERSTACKPANE.CPP
============================================================================================================================================*/
// 🧩 Pane-direct Layers Outliner for Pattern Studio & Stamp Studio (sharp entries, live procedural layer previews, green tick + eye + lock, TextureStudio + & footer)

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

[[nodiscard]] bool ContainsCaseInsensitive(std::string_view Haystack, std::string_view Needle) noexcept
{
    if (Needle.empty())
    {
        return true;
    }
    if (Needle.size() > Haystack.size())
    {
        return false;
    }
    for (std::size_t Start = 0u; Start + Needle.size() <= Haystack.size(); ++Start)
    {
        bool Match = true;
        for (std::size_t Idx = 0u; Idx < Needle.size(); ++Idx)
        {
            const unsigned char A = static_cast<unsigned char>(Haystack[Start + Idx]);
            const unsigned char B = static_cast<unsigned char>(Needle[Idx]);
            if (std::tolower(A) != std::tolower(B))
            {
                Match = false;
                break;
            }
        }
        if (Match)
        {
            return true;
        }
    }
    return false;
}

[[nodiscard]] std::string_view QueryBlendModeLabel(uint8_t BlendMode) noexcept
{
    switch (BlendMode)
    {
        case 1u: return "Multiply";
        case 2u: return "Screen";
        case 3u: return "Overlay";
        case 4u: return "Add";
        case 5u: return "Darken";
        default: return "Normal";
    }
}

[[nodiscard]] ControlIcon QueryModifierIcon(PatternStampModifierCategory Category) noexcept
{
    switch (Category)
    {
        case PatternStampModifierCategory::GridArray:     return ControlIcon::LayoutGrid;
        case PatternStampModifierCategory::NoiseScale:    return ControlIcon::VectorCurve;
        case PatternStampModifierCategory::RadialArray:   return ControlIcon::SparklesAntiAliasing;
        case PatternStampModifierCategory::MirrorSymmetry:return ControlIcon::LayoutSplit;
        case PatternStampModifierCategory::InsetEcho:     return ControlIcon::ShapeCircle;
        case PatternStampModifierCategory::ScatterDropout: return ControlIcon::SparklesAntiAliasing;
        case PatternStampModifierCategory::DomainWarp:    return ControlIcon::NoiseWave;
        case PatternStampModifierCategory::BevelEdgeWear: return ControlIcon::CubeShape;
        default:                                          return ControlIcon::SlidersQuality;
    }
}

[[nodiscard]] IconAccentTone QueryModifierAccent(PatternStampModifierCategory Category) noexcept
{
    switch (Category)
    {
        case PatternStampModifierCategory::GridArray:     return IconAccentTone::Blue;
        case PatternStampModifierCategory::NoiseScale:    return IconAccentTone::Amber;
        case PatternStampModifierCategory::RadialArray:   return IconAccentTone::Teal;
        case PatternStampModifierCategory::MirrorSymmetry:return IconAccentTone::Violet;
        case PatternStampModifierCategory::InsetEcho:     return IconAccentTone::Blue;
        case PatternStampModifierCategory::ScatterDropout: return IconAccentTone::Green;
        case PatternStampModifierCategory::DomainWarp:    return IconAccentTone::Violet;
        case PatternStampModifierCategory::BevelEdgeWear: return IconAccentTone::Amber;
        default:                                          return IconAccentTone::Slate;
    }
}

[[nodiscard]] const char* QueryModifierDisplayName(PatternStampModifierCategory Category) noexcept
{
    const auto& Labels = PatternStampWorkspaceSession::InspectModifierLabels();
    const std::size_t Idx = static_cast<std::size_t>(Category);
    return (Idx < Labels.size()) ? Labels[Idx] : "Modifier";
}

// Live procedural preview inside each sharp outliner layer thumbnail (matches real canvas composition: grid/hex rings, dots, motifs, gradients, text)
void ConstructLiveLayerPreview(
    PixelSpace&                  Surface,
    Rectangle2D                  ThumbRect,
    const PatternStampLayerSpec& Layer,
    bool                         PatternMode,
    const StudioToneSet&         Tone) noexcept
{
    Surface.PushClip(ThumbRect.Inset(1.0f));

    // Checkerboard backdrop for transparent decals, deep dark tile for seamless patterns
    if (!PatternMode)
    {
        constexpr float Cell = 5.0f;
        const ColorRgba CheckA = Tone.LightSurface ? ColorRgba{ 230u, 232u, 236u, 255u } : ColorRgba{ 26u, 28u, 32u, 255u };
        const ColorRgba CheckB = Tone.LightSurface ? ColorRgba{ 210u, 214u, 220u, 255u } : ColorRgba{ 16u, 17u, 20u, 255u };
        for (float Py = ThumbRect.Y; Py < ThumbRect.Bottom(); Py += Cell)
        {
            for (float Px = ThumbRect.X; Px < ThumbRect.Right(); Px += Cell)
            {
                const int Ix = static_cast<int>((Px - ThumbRect.X) / Cell);
                const int Iy = static_cast<int>((Py - ThumbRect.Y) / Cell);
                Surface.FillRectangle({ Px, Py, Cell, Cell }, ((Ix + Iy) & 1) ? CheckA : CheckB, 0.0f);
            }
        }
    }
    else
    {
        Surface.FillRectangle(ThumbRect, Tone.LightSurface ? ColorRgba{ 30u, 33u, 40u, 255u } : ColorRgba{ 14u, 16u, 20u, 255u }, 0.0f);
    }

    if (Layer.Category == PatternStampLayerCategory::Folder)
    {
        ConstructWhiteIconTile(Surface, ThumbRect, ControlIcon::FolderOpen, IconAccentTone::Amber, 0.0f, Tone, !Layer.Visible);
        Surface.PopClip();
        return;
    }

    const float AlphaScale = Layer.Visible ? 1.0f : 0.42f;
    const ColorRgba InkA = (Layer.FillTint.Alpha > 20u ? Layer.FillTint : Tone.HighlightFill).ScaledAlpha(AlphaScale);
    const ColorRgba InkB = (Layer.FillTintB.Alpha > 20u ? Layer.FillTintB : InkA).ScaledAlpha(AlphaScale);
    const float Cx = ThumbRect.Center().X;
    const float Cy = ThumbRect.Center().Y;

    if (Layer.Category == PatternStampLayerCategory::GradientWash)
    {
        Surface.FillRectangle({ ThumbRect.X + 2.0f, ThumbRect.Y + 2.0f, (ThumbRect.Width - 4.0f) * 0.55f, ThumbRect.Height - 4.0f }, InkA, 0.0f);
        Surface.FillRectangle({ ThumbRect.X + 2.0f + (ThumbRect.Width - 4.0f) * 0.45f, ThumbRect.Y + 2.0f, (ThumbRect.Width - 4.0f) * 0.55f, ThumbRect.Height - 4.0f }, InkB, 0.0f);
        Surface.PopClip();
        return;
    }

    if (Layer.Category == PatternStampLayerCategory::Typographic)
    {
        ConstructCenteredWeightedText(Surface, FontWeightClass::Bold, ThumbRect, 14.0f, InkA, "Aa");
        Surface.PopClip();
        return;
    }

    bool GridActive   = false;
    bool RadialActive = false;
    bool HexStagger   = false;
    for (const auto& Mod : Layer.Modifiers)
    {
        if (!Mod.Enabled) continue;
        if (Mod.Category == PatternStampModifierCategory::GridArray)
        {
            GridActive = true;
            HexStagger = (Mod.GridLayout == 1u || Mod.GridLayout == 2u || Mod.GridLayout == 3u);
        }
        else if (Mod.Category == PatternStampModifierCategory::RadialArray)
        {
            RadialActive = true;
        }
    }

    if (GridActive)
    {
        const float Step = ThumbRect.Width * 0.27f;
        for (int Row = -1; Row <= 1; ++Row)
        {
            for (int Col = -1; Col <= 1; ++Col)
            {
                const float OffsetX = (HexStagger && (Row & 1)) ? (Step * 0.32f) : 0.0f;
                const float DotX    = Cx + static_cast<float>(Col) * Step + OffsetX;
                const float DotY    = Cy + static_cast<float>(Row) * Step;
                const float Rad     = ((Row == 0 && Col == 0) ? 2.7f : 2.1f);
                const ColorRgba DotInk = (((Row + Col) & 1) == 0) ? InkA : InkB;
                if (Layer.MotifShape == PatternStampMotifShape::Ring)
                {
                    Surface.StrokeCircle({ DotX, DotY }, Rad, DotInk, 1.25f);
                }
                else if (Layer.MotifShape == PatternStampMotifShape::Square || Layer.MotifShape == PatternStampMotifShape::StripeBar)
                {
                    Surface.FillRectangle({ DotX - Rad, DotY - Rad, Rad * 2.0f, Rad * 2.0f }, DotInk, 0.0f);
                }
                else
                {
                    Surface.FillCircle({ DotX, DotY }, Rad, DotInk);
                }
            }
        }
        Surface.PopClip();
        return;
    }

    if (RadialActive)
    {
        const float OrbitR = ThumbRect.Width * 0.30f;
        for (int Idx = 0; Idx < 8; ++Idx)
        {
            const float Angle = static_cast<float>(Idx) * (6.2831853f / 8.0f);
            const Point2D Pt { Cx + std::cos(Angle) * OrbitR, Cy + std::sin(Angle) * OrbitR };
            Surface.FillCircle(Pt, 2.0f, (Idx & 1) ? InkB : InkA);
        }
        Surface.FillCircle({ Cx, Cy }, 2.6f, InkA);
        Surface.PopClip();
        return;
    }

    // Single motif live silhouette
    const float R = ThumbRect.Width * 0.34f;
    switch (Layer.MotifShape)
    {
        case PatternStampMotifShape::Circle:
            Surface.FillCircle({ Cx, Cy }, R, InkA);
            break;
        case PatternStampMotifShape::Ring:
            Surface.StrokeCircle({ Cx, Cy }, R * 0.92f, InkA, 2.4f);
            break;
        case PatternStampMotifShape::Square:
            Surface.FillRectangle({ Cx - R * 0.85f, Cy - R * 0.85f, R * 1.7f, R * 1.7f }, InkA, 0.0f);
            break;
        case PatternStampMotifShape::StripeBar:
            Surface.FillRectangle({ Cx - R, Cy - 3.2f, R * 2.0f, 6.4f }, InkA, 0.0f);
            break;
        case PatternStampMotifShape::Diamond:
        {
            const std::array<Point2D, 4u> Pts {{
                { Cx, Cy - R }, { Cx + R, Cy }, { Cx, Cy + R }, { Cx - R, Cy },
            }};
            Surface.FillPolygon(Pts, InkA);
            break;
        }
        case PatternStampMotifShape::Triangle:
        {
            const std::array<Point2D, 3u> Pts {{
                { Cx, Cy - R }, { Cx + R * 0.92f, Cy + R * 0.82f }, { Cx - R * 0.92f, Cy + R * 0.82f },
            }};
            Surface.FillPolygon(Pts, InkA);
            break;
        }
        default:
        {
            std::array<Point2D, 6u> Hex {};
            for (std::size_t K = 0u; K < 6u; ++K)
            {
                const float A = static_cast<float>(K) * (6.2831853f / 6.0f) - 1.5707963f;
                Hex[K] = { Cx + std::cos(A) * R, Cy + std::sin(A) * R };
            }
            Surface.FillPolygon(Hex, InkA);
            break;
        }
    }
    Surface.PopClip();
}

void ConstructCanvasRootPreview(
    PixelSpace&                         Surface,
    Rectangle2D                         ThumbRect,
    const PatternStampWorkspaceSession& Session,
    const StudioToneSet&                Tone) noexcept
{
    const auto& Layers = Session.Document.Layers;
    if (!Layers.empty())
    {
        ConstructLiveLayerPreview(Surface, ThumbRect, Layers.front(), Session.Mode == PatternStampWorkspaceMode::Pattern, Tone);
        return;
    }
    ConstructWhiteIconTile(Surface, ThumbRect, ControlIcon::LayersSlabs, IconAccentTone::Teal, 0.0f, Tone, false);
}

} // namespace

//------------------------------------------------------------------------------------------------------------------------
//                                                    IMPLEMENTATION
//------------------------------------------------------------------------------------------------------------------------

PatternStampLayerStackPane::~PatternStampLayerStackPane() noexcept
{
    WorkspaceAppearance::ReleasePanePointerOwnership(this);
}

void PatternStampLayerStackPane::AttachSession(PatternStampWorkspaceSession* TargetSession) noexcept
{
    Session = TargetSession;
}

void PatternStampLayerStackPane::PresentInImGuiPane(double DeltaSeconds, float ScaleFactor)
{
    const ImVec2      Origin = ImGui::GetCursorScreenPos();
    const ImVec2      Avail  = ImGui::GetContentRegionAvail();
    const float       SafeScale = std::max(0.5f, ScaleFactor);
    const Rectangle2D ScreenBounds { Origin.x, Origin.y, std::max(1.0f, Avail.x), std::max(1.0f, Avail.y) };
    const Rectangle2D LocalBounds  { 0.0f, 0.0f, ScreenBounds.Width / SafeScale, ScreenBounds.Height / SafeScale };

    ImGui::InvisibleButton(
        "##PatternStampLayersCanvas",
        ImVec2(ScreenBounds.Width, ScreenBounds.Height),
        ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);

    PaneInputReading Input = WorkspaceAppearance::CaptureImGuiPaneInput(ScreenBounds, SafeScale);
    WorkspaceAppearance::EnforcePanePointerOwnership(this, Input);

    PixelSpace Surface(ImGui::GetWindowDrawList(), { ScreenBounds.X, ScreenBounds.Y }, SafeScale);
    Render(Surface, LocalBounds, Input, DeltaSeconds);

    WorkspaceAppearance::UpdatePanePointerOwnership(this, ScreenBounds, SafeScale, Input, false);
}

void PatternStampLayerStackPane::Render(
    PixelSpace&             Surface,
    Rectangle2D             Bounds,
    const PaneInputReading& Input,
    double                  /*DeltaSeconds*/)
{
    if (Session == nullptr || Bounds.Width < 40.0f || Bounds.Height < 40.0f)
    {
        return;
    }

    const StudioToneSet Tone        = QueryStudioToneSet();
    const Rectangle2D   Pane        = ConstructDockedSurfaceShell(Surface, Bounds, true, Tone);
    const bool          PatternMode = (Session->Mode == PatternStampWorkspaceMode::Pattern);
    const auto&         Layers      = Session->Document.Layers;

    const float PadX   = 11.0f;
    const float InnerW = std::max(80.0f, Pane.Width - PadX * 2.0f);
    const float LeftX  = Pane.X + PadX;

    // 1. TextureStudio OLD Outliner Heading Strip ("Layers  N / 32" + FolderPlus + circular '+' button)
    char CountTagBuf[24] {};
    std::snprintf(CountTagBuf, sizeof(CountTagBuf), "%zu / 32", Layers.size());
    const Rectangle2D HeaderStrip { Pane.X, Pane.Y, Pane.Width, 44.0f };
    const OutlinerHeaderResult HeadOut = ConstructOutlinerHeaderStrip(
        Surface,
        HeaderStrip,
        "Layers",
        CountTagBuf,
        true,
        ControlIcon::PlusAdd,
        AddLayerSheetOpen,
        Input,
        Tone,
        &AddButtonAnchor);
    if (HeadOut.FolderClicked)
    {
        AddLayerSheetOpen = false;
        Session->AddLayer(PatternStampLayerCategory::Folder);
    }
    if (HeadOut.PlusClicked)
    {
        AddLayerSheetOpen = !AddLayerSheetOpen;
    }

    float CursorY = HeaderStrip.Bottom() + 9.0f;

    // 2. Stat tile pair: VISIBLE & MODIFIERS (white icons on solid coloured circular backgrounds)
    const std::size_t TotalLayers   = Layers.size();
    const std::size_t VisibleLayers = Session->CountVisibleLayers();
    std::size_t       TotalMods     = 0u;
    for (const auto& L : Layers)
    {
        TotalMods += L.Modifiers.size();
    }
    const std::size_t EnabledMods = Session->CountEnabledModifiers();

    char VisBuf[24] {};
    char ModBuf[24] {};
    std::snprintf(VisBuf, sizeof(VisBuf), "%zu/%zu", VisibleLayers, TotalLayers);
    std::snprintf(ModBuf, sizeof(ModBuf), "%zu/%zu", EnabledMods, TotalMods);

    const OutlinerStatTileSpec LeftStat {
        "VISIBLE",
        VisBuf,
        ControlIcon::EyeVisible,
        VisibleLayers > 0u ? IconAccentTone::Green : IconAccentTone::Slate,
        Session->LayerFilter == PatternStampLayerFilterMode::VisibleOnly,
    };
    const OutlinerStatTileSpec RightStat {
        "MODIFIERS",
        ModBuf,
        ControlIcon::SparklesAntiAliasing,
        EnabledMods > 0u ? IconAccentTone::Blue : IconAccentTone::Slate,
        FilterModsOnly,
    };
    const int32_t StatClick = ConstructStatTilePair(Surface, { LeftX, CursorY, InnerW, 50.0f }, LeftStat, RightStat, Input, Tone);
    if (StatClick == 0)
    {
        Session->LayerFilter = (Session->LayerFilter == PatternStampLayerFilterMode::VisibleOnly)
            ? PatternStampLayerFilterMode::AllLayers
            : PatternStampLayerFilterMode::VisibleOnly;
    }
    else if (StatClick == 1)
    {
        FilterModsOnly = !FilterModsOnly;
    }
    CursorY += 58.0f;

    // 3. Search input + Filter button
    const bool FilterArmed = (Session->LayerFilter != PatternStampLayerFilterMode::AllLayers) || FilterModsOnly;
    const std::string_view FilterText = (Session->LayerFilter == PatternStampLayerFilterMode::VisibleOnly)
        ? "Visible"
        : (FilterModsOnly ? "Mods" : "Filter");
    if (ConstructSearchFilterRow(
            Surface, { LeftX, CursorY, InnerW, 29.0f }, SearchBuffer, "Find a layer...", FilterText, FilterArmed, Input, Tone))
    {
        if (Session->LayerFilter == PatternStampLayerFilterMode::AllLayers && !FilterModsOnly)
        {
            Session->LayerFilter = PatternStampLayerFilterMode::VisibleOnly;
        }
        else if (Session->LayerFilter == PatternStampLayerFilterMode::VisibleOnly)
        {
            Session->LayerFilter = PatternStampLayerFilterMode::AllLayers;
            FilterModsOnly       = true;
        }
        else
        {
            Session->LayerFilter = PatternStampLayerFilterMode::AllLayers;
            FilterModsOnly       = false;
        }
    }
    Session->LayerSearch = std::string(QueryEditTextView(SearchBuffer));
    CursorY += 36.0f;

    // 4. Quick-add layer chips (+ Motif / + Text / + Gradient)
    const std::array<OutlinerChipSpec, 3u> QuickChips {{
        { "Motif",    {}, {}, false, true, false, false },
        { "Text",     {}, {}, false, true, false, false },
        { "Gradient", {}, {}, false, true, false, false },
    }};
    float ChipsH = 0.0f;
    const int32_t PickedChip = ConstructQuickChipStrip(Surface, LeftX, CursorY, InnerW, QuickChips, Input, Tone, ChipsH);
    if (PickedChip == 0)
    {
        Session->AddLayer(PatternStampLayerCategory::Motif);
    }
    else if (PickedChip == 1)
    {
        Session->AddLayer(PatternStampLayerCategory::Typographic);
    }
    else if (PickedChip == 2)
    {
        Session->AddLayer(PatternStampLayerCategory::GradientWash);
    }
    CursorY += ChipsH + 8.0f;

    // Reserve TextureStudio OLD Bottom Footer Stack: Command Toolbar (34px) + Canvas Ground Row (38px) + 3-Metric Footer (28px)
    constexpr float ToolbarH = 34.0f;
    constexpr float GroundH  = 38.0f;
    constexpr float MetricsH = 28.0f;
    const float FooterTotalH = ToolbarH + GroundH + MetricsH;
    const Rectangle2D ToolbarRect { Pane.X, Pane.Bottom() - FooterTotalH, Pane.Width, ToolbarH };
    const Rectangle2D GroundRect  { Pane.X, ToolbarRect.Bottom(), Pane.Width, GroundH };
    const Rectangle2D MetricsRect { Pane.X, GroundRect.Bottom(), Pane.Width, MetricsH };

    const Rectangle2D TreeRect { Pane.X, CursorY, Pane.Width, std::max(40.0f, ToolbarRect.Y - CursorY - 4.0f) };
    const Point2D Cursor = QueryInputCursor(Input);
    if (!AddLayerSheetOpen && TreeRect.Contains(Cursor) && std::fabs(Input.WheelDeltaY) > 1.0e-4f)
    {
        ScrollOffsetY = std::clamp(ScrollOffsetY - Input.WheelDeltaY * 32.0f, 0.0f, MaxScrollY);
    }

    // 5. Scrollable Sharp-Cornered Hierarchy Tree (NO Starters folder!)
    Surface.PushClip(TreeRect);
    float RowY = TreeRect.Y - ScrollOffsetY;
    const float RowX = LeftX;
    const float RowW = InnerW;
    const std::string_view SearchNeedle = QueryEditTextView(SearchBuffer);

    // Document root row (live preview + metadata + SEAMLESS/CUTOUT tag)
    {
        char CanvasMeta[72] {};
        std::snprintf(CanvasMeta, sizeof(CanvasMeta), "%s · %u px · %s",
            PatternMode ? "Canvas tile" : "Decal",
            Session->ResolutionPx,
            Session->Document.GroundEnabled ? "ground on" : "no ground");

        HierarchyRowSpec RootSpec {};
        RootSpec.DepthLevel        = 0;
        RootSpec.Title             = Session->Document.DocumentName;
        RootSpec.Subtitle          = CanvasMeta;
        RootSpec.TagLabel          = PatternMode ? "SEAMLESS" : "CUTOUT";
        RootSpec.TagStyle          = PatternMode ? TagTone::Green : TagTone::Blue;
        RootSpec.CustomLeadPreview = true;
        RootSpec.LeadPreviewSize   = 28.0f;
        RootSpec.Expandable        = true;
        RootSpec.Expanded          = RootExpanded;
        RootSpec.GroupStyle        = true;

        const Rectangle2D RowRect { RowX, RowY, RowW, 40.0f };
        const HierarchyRowResult Res = ConstructHierarchyRow(Surface, RowRect, RootSpec, Input, Tone);
        ConstructCanvasRootPreview(Surface, Res.LeadPreviewRect, *Session, Tone);
        if (!AddLayerSheetOpen && (Res.ChevronClicked || Res.RowClicked))
        {
            RootExpanded = !RootExpanded;
        }
        RowY += 43.0f;
    }

    // Layer rows (sharp 0px corners, real procedural layer previews, metadata, green tick + eye + lock)
    if (RootExpanded)
    {
        const std::size_t LayerCount = Layers.size();
        for (std::size_t RevIdx = 0u; RevIdx < LayerCount; ++RevIdx)
        {
            const std::size_t LIdx  = LayerCount - 1u - RevIdx;
            const auto&       Layer = Layers[LIdx];

            if (Session->QueryUnderCollapsedFolder(Layer.Identity))
            {
                continue;
            }
            if (Session->LayerFilter == PatternStampLayerFilterMode::VisibleOnly && !Layer.Visible)
            {
                continue;
            }
            if (FilterModsOnly && Layer.Modifiers.empty())
            {
                continue;
            }
            if (!SearchNeedle.empty() && !ContainsCaseInsensitive(Layer.Name, SearchNeedle))
            {
                continue;
            }

            const bool     Selected = (Layer.Identity == Session->Document.SelectedLayerId);
            const uint32_t Depth    = Session->QueryLayerDepth(Layer.Identity);
            const bool     NestedEntries = (Layer.Category == PatternStampLayerCategory::Folder) || !Layer.Modifiers.empty() || !Layer.Channels.empty();

            char LayerMeta[96] {};
            if (Layer.Category == PatternStampLayerCategory::Folder)
            {
                std::snprintf(LayerMeta, sizeof(LayerMeta), "Group · %zu layers", Session->CountFolderImmediateMembers(Layer.Identity));
            }
            else if (!Layer.Modifiers.empty())
            {
                std::size_t ActiveModCount = 0u;
                for (const auto& M : Layer.Modifiers)
                {
                    if (M.Enabled) ++ActiveModCount;
                }
                const std::string_view BlendTxt = QueryBlendModeLabel(Layer.BlendMode);
                std::snprintf(LayerMeta, sizeof(LayerMeta), "%.*s · %.0f%% · %zu/%zu mods",
                    static_cast<int>(BlendTxt.size()), BlendTxt.data(),
                    Layer.Opacity * 100.0f, ActiveModCount, Layer.Modifiers.size());
            }
            else
            {
                const std::string_view BlendTxt = QueryBlendModeLabel(Layer.BlendMode);
                std::snprintf(LayerMeta, sizeof(LayerMeta), "%.*s · %.0f%%",
                    static_cast<int>(BlendTxt.size()), BlendTxt.data(), Layer.Opacity * 100.0f);
            }

            HierarchyRowSpec Spec {};
            Spec.DepthLevel        = 1 + static_cast<int32_t>(Depth);
            Spec.Title             = Layer.Name;
            Spec.Subtitle          = LayerMeta;
            Spec.CustomLeadPreview = true;
            Spec.LeadPreviewSize   = 32.0f;
            Spec.Expandable        = NestedEntries;
            Spec.Expanded          = (Layer.Category == PatternStampLayerCategory::Folder)
                ? !Layer.CollapsedFolder
                : (Layer.ExpandedInStack || Selected);
            Spec.Selected          = Selected;
            Spec.Muted             = !Layer.Visible;
            Spec.ShowGreenCheck    = true;
            Spec.GreenCheckOn      = Layer.Visible;
            Spec.ShowEyeToggle     = true;
            Spec.EyeVisible        = Layer.Visible;
            Spec.ShowLockToggle    = true;
            Spec.LockEnabled       = Layer.Locked;

            const Rectangle2D RowRect { RowX, RowY, RowW, 44.0f };
            const HierarchyRowResult Res = ConstructHierarchyRow(Surface, RowRect, Spec, Input, Tone);
            ConstructLiveLayerPreview(Surface, Res.LeadPreviewRect, Layer, PatternMode, Tone);

            // Inline rename editing when double-clicked
            if (RenamingLayerId == Layer.Identity && InlineRenameBuffer.Active)
            {
                const Rectangle2D RenameRect { Res.LeadPreviewRect.Right() + 6.0f, RowRect.Y + 4.0f, std::max(60.0f, RowRect.Width - 138.0f), 22.0f };
                Surface.FillRectangle(RenameRect, Tone.FieldWell, 0.0f);
                Surface.StrokeRectangle(RenameRect, Tone.HighlightFill, 0.0f, 1.0f);
                (void)TextInputField::Construct(Surface, RenameRect, InlineRenameBuffer, Input.PrimaryPointer, 10.5f);
                if (!InlineRenameBuffer.Active)
                {
                    if (auto* Mutable = Session->FindLayerById(Layer.Identity); Mutable != nullptr && InlineRenameBuffer.Length > 0u)
                    {
                        Mutable->Name = std::string(QueryEditTextView(InlineRenameBuffer));
                        Session->BumpRevision();
                    }
                    RenamingLayerId = 0u;
                }
            }

            if (!AddLayerSheetOpen)
            {
                if (Res.CheckToggled || Res.EyeToggled)
                {
                    Session->ToggleLayerVisible(Layer.Identity);
                }
                else if (Res.LockToggled)
                {
                    Session->ToggleLayerLocked(Layer.Identity);
                }
                else if (Res.ChevronClicked)
                {
                    if (Layer.Category == PatternStampLayerCategory::Folder)
                    {
                        Session->ToggleFolderCollapsed(Layer.Identity);
                    }
                    else
                    {
                        Session->ToggleLayerExpandedDrawer(Layer.Identity);
                    }
                }
                else if (Res.DoubleClicked)
                {
                    RenamingLayerId = Layer.Identity;
                    InlineRenameBuffer.Begin(Layer.Name.c_str());
                }
                else if (Res.RowClicked)
                {
                    Session->SelectLayer(Layer.Identity);
                }
            }

            RowY += 46.0f;

            // Nested modifier rows & surface summary row under expanded layer
            const bool ShowNested = (Layer.Category != PatternStampLayerCategory::Folder) && (Layer.ExpandedInStack || Selected);
            if (ShowNested)
            {
                for (std::size_t MIdx = 0u; MIdx < Layer.Modifiers.size(); ++MIdx)
                {
                    const auto& Mod = Layer.Modifiers[MIdx];
                    char ModMeta[72] {};
                    std::snprintf(ModMeta, sizeof(ModMeta), "#%zu · %s", MIdx + 1u, Mod.Enabled ? "Active" : "Bypassed");

                    HierarchyRowSpec ModSpec {};
                    ModSpec.DepthLevel      = 2 + static_cast<int32_t>(Depth);
                    ModSpec.Title           = QueryModifierDisplayName(Mod.Category);
                    ModSpec.Subtitle        = ModMeta;
                    ModSpec.LeadIcon        = QueryModifierIcon(Mod.Category);
                    ModSpec.LeadIconTone    = Mod.Enabled ? QueryModifierAccent(Mod.Category) : IconAccentTone::Slate;
                    ModSpec.LeadPreviewSize = 22.0f;
                    ModSpec.Muted           = !Mod.Enabled;
                    ModSpec.ShowGreenCheck  = true;
                    ModSpec.GreenCheckOn    = Mod.Enabled;

                    const Rectangle2D ModRect { RowX, RowY, RowW, 34.0f };
                    const HierarchyRowResult ModRes = ConstructHierarchyRow(Surface, ModRect, ModSpec, Input, Tone);
                    if (!AddLayerSheetOpen && (ModRes.CheckToggled || ModRes.RowClicked))
                    {
                        Session->SelectLayer(Layer.Identity);
                        if (ModRes.CheckToggled)
                        {
                            if (auto* Mutable = Session->FindLayerById(Layer.Identity); Mutable != nullptr && MIdx < Mutable->Modifiers.size())
                            {
                                Session->PushUndoSnapshot();
                                Mutable->Modifiers[MIdx].Enabled = !Mutable->Modifiers[MIdx].Enabled;
                                Session->RecomputeSummaryMetrics();
                                Session->BumpRevision();
                            }
                        }
                    }
                    RowY += 36.0f;
                }

                // Surface PBR channel summary nested row
                char SurfMeta[64] {};
                std::snprintf(SurfMeta, sizeof(SurfMeta), "%zu PBR channels", Layer.Channels.size());
                HierarchyRowSpec SurfSpec {};
                SurfSpec.DepthLevel      = 2 + static_cast<int32_t>(Depth);
                SurfSpec.Title           = "Surface";
                SurfSpec.Subtitle        = SurfMeta;
                SurfSpec.LeadIcon        = ControlIcon::LayersSlabs;
                SurfSpec.LeadIconTone    = IconAccentTone::Teal;
                SurfSpec.LeadPreviewSize = 22.0f;

                const Rectangle2D SurfRect { RowX, RowY, RowW, 33.0f };
                const HierarchyRowResult SurfRes = ConstructHierarchyRow(Surface, SurfRect, SurfSpec, Input, Tone);
                if (!AddLayerSheetOpen && SurfRes.RowClicked)
                {
                    Session->SelectLayer(Layer.Identity);
                }
                RowY += 35.0f;
            }
        }
    }

    const float ContentH = (RowY + ScrollOffsetY) - TreeRect.Y;
    MaxScrollY           = std::max(0.0f, ContentH - TreeRect.Height);
    ScrollOffsetY        = std::clamp(ScrollOffsetY, 0.0f, MaxScrollY);
    Surface.PopClip();

    // 6. TextureStudio OLD Bottom Command Toolbar (Up / Down  ---  Duplicate / Mask / Delete)
    const PatternStampLayerSpec* SelectedLayer = Session->InspectSelectedLayer();
    const bool LayerSelected = (SelectedLayer != nullptr);
    const OutlinerToolbarResult ToolOut = ConstructOutlinerCommandToolbar(
        Surface,
        ToolbarRect,
        LayerSelected && Layers.size() > 1u,
        LayerSelected && Layers.size() > 1u,
        LayerSelected,
        true,
        LayerSelected && Layers.size() > 1u,
        Input,
        Tone);
    if (LayerSelected)
    {
        if (ToolOut.MoveUpClicked)
        {
            Session->MoveLayerStep(SelectedLayer->Identity, 1);
        }
        else if (ToolOut.MoveDownClicked)
        {
            Session->MoveLayerStep(SelectedLayer->Identity, -1);
        }
        else if (ToolOut.DuplicateClicked)
        {
            Session->DuplicateSelectedLayer();
        }
        else if (ToolOut.MaskClicked)
        {
            Session->ToggleSelectedLayerMask();
        }
        else if (ToolOut.DeleteClicked)
        {
            Session->DeleteSelectedLayer();
        }
    }

    // 7. TextureStudio OLD Ground Canvas Row
    char GroundTitle[64] {};
    std::snprintf(GroundTitle, sizeof(GroundTitle), "%s · %ux%u",
        PatternMode ? "Canvas Tile" : "Decal Canvas", Session->ResolutionPx, Session->ResolutionPx);
    ConstructOutlinerGroundRow(
        Surface,
        GroundRect,
        ControlIcon::LayersSlabs,
        IconAccentTone::Slate,
        GroundTitle,
        Session->Document.GroundEnabled ? "Solid canvas ground" : "Transparent alpha cutout",
        PatternMode ? "Seamless" : "Cutout",
        Tone);

    // 8. TextureStudio OLD 3-Metric Outliner Footer (LAYERS | VISIBLE | MODS)
    char FLayers[16] {};
    char FVis[16] {};
    char FMods[16] {};
    std::snprintf(FLayers, sizeof(FLayers), "%zu", TotalLayers);
    std::snprintf(FVis, sizeof(FVis), "%zu", VisibleLayers);
    std::snprintf(FMods, sizeof(FMods), "%zu", EnabledMods);
    const std::array<ThreeMetricFooterCell, 3u> FooterCells {{
        { "LAYERS",  FLayers, TagTone::Neutral },
        { "VISIBLE", FVis,    VisibleLayers > 0u ? TagTone::Green : TagTone::Neutral },
        { "MODS",    FMods,   TagTone::Neutral },
    }};
    ConstructThreeMetricFooter(Surface, MetricsRect, FooterCells, Tone);

    // 9. Add Layer Overlay Popover anchored to the TextureStudio '+' button
    if (AddLayerSheetOpen)
    {
        const std::array<PopoverOptionSpec, 4u> AddOptions {{
            { "Motif layer",   "Vector shape",  Tone.AccentBlue   },
            { "Text block",    "Engraved type", Tone.AccentTeal   },
            { "Gradient wash", "Tile fill field", Tone.AccentViolet },
            { "Group folder",  "Nest layers",   Tone.AccentAmber  },
        }};
        bool Dismissed = false;
        const int32_t Picked = ConstructStudioPopoverList(
            Surface, AddButtonAnchor, Pane, "ADD LAYER", AddOptions, -1, Input, Tone, Dismissed);
        if (Picked == 0)
        {
            Session->AddLayer(PatternStampLayerCategory::Motif);
            AddLayerSheetOpen = false;
        }
        else if (Picked == 1)
        {
            Session->AddLayer(PatternStampLayerCategory::Typographic);
            AddLayerSheetOpen = false;
        }
        else if (Picked == 2)
        {
            Session->AddLayer(PatternStampLayerCategory::GradientWash);
            AddLayerSheetOpen = false;
        }
        else if (Picked == 3)
        {
            Session->AddLayer(PatternStampLayerCategory::Folder);
            AddLayerSheetOpen = false;
        }
        else if (Dismissed)
        {
            AddLayerSheetOpen = false;
        }
    }
}

} // namespace Frontier::Workspaces
