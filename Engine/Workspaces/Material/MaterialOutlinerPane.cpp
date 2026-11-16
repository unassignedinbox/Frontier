/*============================================================================================================================================
                                                         MATERIALOUTLINERPANE.CPP
============================================================================================================================================*/
// 🧩 Left-docked Material Outliner pane (pane-direct surface, sharp entries, live material/channel previews, green tick + eye + lock, TextureStudio footer)

#include "Engine/Workspaces/Material/MaterialOutlinerPane.h"
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

[[nodiscard]] bool ContainsInsensitive(std::string_view Haystack, std::string_view Needle) noexcept
{
    if (Needle.empty()) return true;
    if (Needle.size() > Haystack.size()) return false;
    for (std::size_t Start = 0u; Start + Needle.size() <= Haystack.size(); ++Start)
    {
        bool Same = true;
        for (std::size_t Idx = 0u; Idx < Needle.size(); ++Idx)
        {
            if (std::tolower(static_cast<unsigned char>(Haystack[Start + Idx])) !=
                std::tolower(static_cast<unsigned char>(Needle[Idx])))
            {
                Same = false;
                break;
            }
        }
        if (Same) return true;
    }
    return false;
}

[[nodiscard]] ColorRgba LinearArrayToSrgbRgba(const std::array<float, 4u>& Linear) noexcept
{
    auto Convert = [](float V) noexcept -> uint8_t
    {
        const float Clamped = std::clamp(V, 0.0f, 1.0f);
        const float Gamma   = std::pow(Clamped, 1.0f / 2.2f);
        return static_cast<uint8_t>(std::clamp(std::lround(Gamma * 255.0f), 0l, 255l));
    };
    return { Convert(Linear[0]), Convert(Linear[1]), Convert(Linear[2]), 255u };
}

[[nodiscard]] std::string FormatHexRgba(ColorRgba C) noexcept
{
    char Buf[12] {};
    std::snprintf(Buf, sizeof(Buf), "#%02X%02X%02X", C.Red, C.Green, C.Blue);
    return Buf;
}

[[nodiscard]] std::string_view DescribeLayerCategory(MaterialSpace::Dynamic::MaterialLayerCategory Cat) noexcept
{
    using MaterialSpace::Dynamic::MaterialLayerCategory;
    switch (Cat)
    {
        case MaterialLayerCategory::FinishLayer:            return "Finish layer";
        case MaterialLayerCategory::ProceduralFieldLayer:   return "Procedural field";
        case MaterialLayerCategory::MaskModifierLayer:      return "Mask modifier";
        case MaterialLayerCategory::SurfaceTransitionLayer: return "Transition blend";
        default:                                            return "Surface layer";
    }
}

// Live material orb preview for the root recipe and finish layers
void ConstructMaterialOrbPreview(
    PixelSpace&                                          Surface,
    Rectangle2D                                          ThumbRect,
    const MaterialSpace::Dynamic::DynamicMaterialRecipe& Recipe,
    bool                                                 Muted) noexcept
{
    const float Alpha = Muted ? 0.42f : 1.0f;
    const ColorRgba Pri = LinearArrayToSrgbRgba(Recipe.Parameters.PrimaryColorRgba).ScaledAlpha(Alpha);
    const ColorRgba Sec = LinearArrayToSrgbRgba(Recipe.Parameters.SecondaryColorRgba).ScaledAlpha(Alpha);
    const ColorRgba Acc = LinearArrayToSrgbRgba(Recipe.Parameters.AccentRadianceRgba).ScaledAlpha(Alpha);

    Surface.PushClip(ThumbRect.Inset(1.0f));
    Surface.FillRectangle(ThumbRect, { 14u, 15u, 18u, 255u }, 0.0f);
    const Point2D C = ThumbRect.Center();
    const float   R = ThumbRect.Width * 0.38f;
    Surface.FillCircle(C, R, Pri);
    // Specular rim & procedural vein accent matching active recipe
    Surface.StrokeCircle(C, R * 0.92f, Sec.ScaledAlpha(0.75f), 1.3f);
    if (Recipe.Parameters.SeamGlowIntensity > 0.25f)
    {
        const std::array<Point2D, 3u> Vein {{
            { C.X - R * 0.55f, C.Y + R * 0.35f },
            { C.X + R * 0.05f, C.Y - R * 0.08f },
            { C.X + R * 0.60f, C.Y - R * 0.42f },
        }};
        Surface.StrokePolyline(Vein, Acc, 1.5f, false);
    }
    Surface.FillCircle({ C.X - R * 0.30f, C.Y - R * 0.32f }, R * 0.24f, ColorRgba{ 255u, 255u, 255u, static_cast<uint8_t>(115.0f * Alpha) });
    Surface.PopClip();
}

// Live procedural pattern preview for procedural/mask/transition layers
void ConstructMaterialLayerPreview(
    PixelSpace&                                          Surface,
    Rectangle2D                                          ThumbRect,
    const MaterialSpace::Dynamic::MaterialLayerEntry&    Layer,
    const MaterialSpace::Dynamic::DynamicMaterialRecipe& Recipe,
    bool                                                 Muted) noexcept
{
    using MaterialSpace::Dynamic::MaterialLayerCategory;
    if (Layer.Category == MaterialLayerCategory::FinishLayer)
    {
        ConstructMaterialOrbPreview(Surface, ThumbRect, Recipe, Muted);
        return;
    }

    const float Alpha = Muted ? 0.42f : 1.0f;
    const ColorRgba Pri = LinearArrayToSrgbRgba(Recipe.Parameters.PrimaryColorRgba).ScaledAlpha(Alpha);
    const ColorRgba Sec = LinearArrayToSrgbRgba(Recipe.Parameters.SecondaryColorRgba).ScaledAlpha(Alpha);
    const ColorRgba Acc = LinearArrayToSrgbRgba(Recipe.Parameters.AccentRadianceRgba).ScaledAlpha(Alpha);

    Surface.PushClip(ThumbRect.Inset(1.0f));
    Surface.FillRectangle(ThumbRect, { 15u, 17u, 21u, 255u }, 0.0f);

    if (Layer.Category == MaterialLayerCategory::SurfaceTransitionLayer)
    {
        const float HalfW = ThumbRect.Width * 0.5f;
        Surface.FillRectangle({ ThumbRect.X, ThumbRect.Y, HalfW, ThumbRect.Height }, Pri, 0.0f);
        Surface.FillRectangle({ ThumbRect.X + HalfW, ThumbRect.Y, HalfW, ThumbRect.Height }, Sec, 0.0f);
        Surface.StrokeLine({ ThumbRect.Center().X, ThumbRect.Y }, { ThumbRect.Center().X, ThumbRect.Bottom() }, Acc, 2.0f);
    }
    else if (Layer.Category == MaterialLayerCategory::MaskModifierLayer)
    {
        const float Step = ThumbRect.Width * 0.25f;
        for (int R = 0; R < 4; ++R)
        {
            for (int C = 0; C < 4; ++C)
            {
                const ColorRgba CellCol = ((R + C) & 1) ? Acc : ColorRgba{ 24u, 26u, 32u, 255u };
                Surface.FillRectangle({ ThumbRect.X + static_cast<float>(C) * Step, ThumbRect.Y + static_cast<float>(R) * Step, Step, Step }, CellCol, 0.0f);
            }
        }
    }
    else
    {
        // Procedural field miniature contour lattice
        const Point2D Mid = ThumbRect.Center();
        const float   Step = ThumbRect.Width * 0.27f;
        for (int Ry = -1; Ry <= 1; ++Ry)
        {
            for (int Rx = -1; Rx <= 1; ++Rx)
            {
                const float Px = Mid.X + static_cast<float>(Rx) * Step + ((Ry & 1) ? Step * 0.35f : 0.0f);
                const float Py = Mid.Y + static_cast<float>(Ry) * Step;
                Surface.FillCircle({ Px, Py }, 2.2f, ((Rx + Ry) & 1) ? Sec : Acc);
            }
        }
    }
    Surface.PopClip();
}

// Live mini preview for each optical channel row (colour swatch or calibrated greyscale fill bar)
void ConstructChannelSwatchPreview(
    PixelSpace&          Surface,
    Rectangle2D          ThumbRect,
    bool                 SolidColor,
    ColorRgba            SwatchColor,
    float                ScalarFill,
    const StudioToneSet& Tone) noexcept
{
    Surface.PushClip(ThumbRect.Inset(1.0f));
    if (SolidColor)
    {
        Surface.FillRectangle(ThumbRect, SwatchColor, 0.0f);
    }
    else
    {
        Surface.FillRectangle(ThumbRect, Tone.FieldWell, 0.0f);
        const float FillH = std::clamp(ScalarFill, 0.08f, 1.0f) * ThumbRect.Height;
        Surface.FillRectangle({ ThumbRect.X, ThumbRect.Bottom() - FillH, ThumbRect.Width, FillH }, SwatchColor, 0.0f);
    }
    Surface.PopClip();
}

} // namespace

//------------------------------------------------------------------------------------------------------------------------
//                                                    IMPLEMENTATION
//------------------------------------------------------------------------------------------------------------------------

MaterialOutlinerPane::~MaterialOutlinerPane() noexcept
{
    WorkspaceAppearance::ReleasePanePointerOwnership(this);
}

void MaterialOutlinerPane::AttachSession(MaterialWorkspaceSession* TargetSession) noexcept
{
    Session = TargetSession;
}

void MaterialOutlinerPane::PresentInImGuiPane(double DeltaSeconds, float ScaleFactor)
{
    const ImVec2      Origin = ImGui::GetCursorScreenPos();
    const ImVec2      Avail  = ImGui::GetContentRegionAvail();
    const float       SafeScale = std::max(0.5f, ScaleFactor);
    const Rectangle2D ScreenBounds { Origin.x, Origin.y, std::max(1.0f, Avail.x), std::max(1.0f, Avail.y) };
    const Rectangle2D LocalBounds  { 0.0f, 0.0f, ScreenBounds.Width / SafeScale, ScreenBounds.Height / SafeScale };

    ImGui::InvisibleButton(
        "##MaterialOutlinerCanvas",
        ImVec2(ScreenBounds.Width, ScreenBounds.Height),
        ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);

    PaneInputReading Input = WorkspaceAppearance::CaptureImGuiPaneInput(ScreenBounds, SafeScale);
    WorkspaceAppearance::EnforcePanePointerOwnership(this, Input);

    PixelSpace Surface(ImGui::GetWindowDrawList(), { ScreenBounds.X, ScreenBounds.Y }, SafeScale);
    Render(Surface, LocalBounds, Input, DeltaSeconds);

    WorkspaceAppearance::UpdatePanePointerOwnership(this, ScreenBounds, SafeScale, Input, false);
}

void MaterialOutlinerPane::Render(
    PixelSpace&             Surface,
    Rectangle2D             Bounds,
    const PaneInputReading& Input,
    double                  /*DeltaSeconds*/)
{
    if (Session == nullptr || Bounds.Width < 40.0f || Bounds.Height < 40.0f)
    {
        return;
    }

    const StudioToneSet Tone   = QueryStudioToneSet();
    const Rectangle2D   Pane   = ConstructDockedSurfaceShell(Surface, Bounds, true, Tone);
    auto&               Recipe = Session->AccessActiveRecipe();
    auto&               Stack  = Recipe.LayerStack;
    const auto&         Params = Recipe.Parameters;

    const float PadX   = 11.0f;
    const float InnerW = std::max(80.0f, Pane.Width - PadX * 2.0f);
    const float LeftX  = Pane.X + PadX;

    const uint32_t VisibleLayers = Stack.CountVisibleLayers();
    const uint32_t TotalLayers   = static_cast<uint32_t>(Stack.Layers.size());

    // 1. TextureStudio OLD Outliner Heading Strip ("Outliner  N/M" + circular '+' button)
    char CountTagBuf[24] {};
    std::snprintf(CountTagBuf, sizeof(CountTagBuf), "%u / %u", VisibleLayers, std::max<uint32_t>(1u, TotalLayers));
    const Rectangle2D HeaderStrip { Pane.X, Pane.Y, Pane.Width, 44.0f };
    const OutlinerHeaderResult HeadOut = ConstructOutlinerHeaderStrip(
        Surface, HeaderStrip, "Outliner", CountTagBuf, false, ControlIcon::PlusAdd, AddLayerPopoverOpen, Input, Tone, &AddButtonAnchor);
    if (HeadOut.PlusClicked)
    {
        AddLayerPopoverOpen = !AddLayerPopoverOpen;
    }

    float CursorY = HeaderStrip.Bottom() + 9.0f;

    // 2. Stat tile pair: LAYERS & DRIVEN (white icons on solid coloured circular discs)
    uint32_t DrivenCount = 0u;
    for (const auto& L : Stack.Layers)
    {
        if (L.VisibilityEnabled && L.Category != MaterialSpace::Dynamic::MaterialLayerCategory::FinishLayer)
        {
            DrivenCount += 2u;
        }
    }
    DrivenCount = std::min<uint32_t>(DrivenCount + 2u, 12u);

    char LayerBuf[24] {};
    char DrivenBuf[24] {};
    std::snprintf(LayerBuf, sizeof(LayerBuf), "%u/%u", VisibleLayers, TotalLayers);
    std::snprintf(DrivenBuf, sizeof(DrivenBuf), "%u/12", DrivenCount);

    const OutlinerStatTileSpec LeftStat {
        "LAYERS",
        LayerBuf,
        ControlIcon::CircleCheck,
        VisibleLayers > 0u ? IconAccentTone::Green : IconAccentTone::Slate,
        FilterEnabledOnly,
    };
    const OutlinerStatTileSpec RightStat {
        "DRIVEN",
        DrivenBuf,
        ControlIcon::VectorCurve,
        DrivenCount > 0u ? IconAccentTone::Blue : IconAccentTone::Slate,
        FilterDrivenOnly,
    };
    const int32_t StatClick = ConstructStatTilePair(Surface, { LeftX, CursorY, InnerW, 50.0f }, LeftStat, RightStat, Input, Tone);
    if (StatClick == 0)
    {
        FilterEnabledOnly = !FilterEnabledOnly;
    }
    else if (StatClick == 1)
    {
        FilterDrivenOnly = !FilterDrivenOnly;
    }
    CursorY += 58.0f;

    // 3. Search input + Filter toggle
    const bool FilterActive = FilterEnabledOnly || FilterDrivenOnly;
    const std::string_view FilterText = FilterDrivenOnly ? "Driven" : (FilterEnabledOnly ? "Active" : "Filter");
    if (ConstructSearchFilterRow(
            Surface, { LeftX, CursorY, InnerW, 29.0f }, SearchBuffer, "Search layers, channels...", FilterText, FilterActive, Input, Tone))
    {
        if (!FilterEnabledOnly && !FilterDrivenOnly)
        {
            FilterDrivenOnly = true;
        }
        else if (FilterDrivenOnly)
        {
            FilterDrivenOnly  = false;
            FilterEnabledOnly = true;
        }
        else
        {
            FilterEnabledOnly = false;
            FilterDrivenOnly  = false;
        }
    }
    CursorY += 36.0f;

    // 4. Quick-add layer chips (Detail / Procedural / Transition)
    const std::array<OutlinerChipSpec, 3u> QuickChips {{
        { "Detail",     { 196u, 118u, 42u,  255u }, { 232u, 194u, 122u, 255u }, true, false, false, false },
        { "Procedural", { 52u,  118u, 214u, 255u }, { 158u, 207u, 255u, 255u }, true, false, false, false },
        { "Transition", { 124u, 86u,  196u, 255u }, { 224u, 140u, 255u, 255u }, true, false, false, false },
    }};
    float ChipsH = 0.0f;
    const int32_t PickedChip = ConstructQuickChipStrip(Surface, LeftX, CursorY, InnerW, QuickChips, Input, Tone, ChipsH);
    if (PickedChip == 0 || PickedChip == 1)
    {
        Session->AppendProceduralLayer();
    }
    else if (PickedChip == 2)
    {
        Session->AppendTransitionLayer();
    }
    CursorY += ChipsH + 8.0f;

    // Reserve TextureStudio OLD Bottom Footer Stack: Command Toolbar (34px) + OpenPBR Ground Row (38px) + 3-Metric Footer (28px)
    constexpr float ToolbarH = 34.0f;
    constexpr float GroundH  = 38.0f;
    constexpr float MetricsH = 28.0f;
    const float FooterTotalH = ToolbarH + GroundH + MetricsH;
    const Rectangle2D ToolbarRect { Pane.X, Pane.Bottom() - FooterTotalH, Pane.Width, ToolbarH };
    const Rectangle2D GroundRect  { Pane.X, ToolbarRect.Bottom(), Pane.Width, GroundH };
    const Rectangle2D MetricsRect { Pane.X, GroundRect.Bottom(), Pane.Width, MetricsH };

    const Rectangle2D TreeRect { Pane.X, CursorY, Pane.Width, std::max(40.0f, ToolbarRect.Y - CursorY - 4.0f) };
    const Point2D Cursor = QueryInputCursor(Input);
    if (!AddLayerPopoverOpen && TreeRect.Contains(Cursor) && std::fabs(Input.WheelDeltaY) > 1.0e-4f)
    {
        ScrollOffsetY = std::clamp(ScrollOffsetY - Input.WheelDeltaY * 32.0f, 0.0f, MaxScrollY);
    }

    // 5. Scrollable Sharp-Cornered Hierarchy Tree (NO Library / Presets folder!)
    Surface.PushClip(TreeRect);
    float RowY = TreeRect.Y - ScrollOffsetY;
    const float RowX = LeftX;
    const float RowW = InnerW;
    const std::string_view SearchNeedle = QueryEditTextView(SearchBuffer);

    // Root recipe node (live lit material orb preview)
    {
        char RootSub[64] {};
        std::snprintf(RootSub, sizeof(RootSub), "%s · %u/%u layers", Recipe.CategoryLabel.c_str(), VisibleLayers, TotalLayers);

        HierarchyRowSpec RootSpec {};
        RootSpec.DepthLevel        = 0;
        RootSpec.Title             = Recipe.DisplayTitle;
        RootSpec.Subtitle          = RootSub;
        RootSpec.CustomLeadPreview = true;
        RootSpec.LeadPreviewSize   = 28.0f;
        RootSpec.Expandable        = true;
        RootSpec.Expanded          = RootExpanded;
        RootSpec.GroupStyle        = true;

        const Rectangle2D RowRect { RowX, RowY, RowW, 40.0f };
        const HierarchyRowResult Res = ConstructHierarchyRow(Surface, RowRect, RootSpec, Input, Tone);
        ConstructMaterialOrbPreview(Surface, Res.LeadPreviewRect, Recipe, false);
        if (!AddLayerPopoverOpen && (Res.ChevronClicked || Res.RowClicked))
        {
            RootExpanded = !RootExpanded;
        }
        RowY += 43.0f;
    }

    const std::size_t SelectedLayerIdx = Session->QuerySelectedLayerIndex();

    if (RootExpanded)
    {
        for (std::size_t LIdx = 0u; LIdx < Stack.Layers.size(); ++LIdx)
        {
            const auto& Layer = Stack.Layers[LIdx];
            if (FilterEnabledOnly && !Layer.VisibilityEnabled)
            {
                continue;
            }
            const bool LayerMatchesSearch = ContainsInsensitive(Layer.LayerTitle, SearchNeedle);
            const bool SelectedLayer      = (LIdx == SelectedLayerIdx);
            const bool LayerLocked        = ((LockedLayerMask & (1u << static_cast<uint32_t>(LIdx & 31u))) != 0u);

            char LayerSub[72] {};
            const std::string_view CatStr = DescribeLayerCategory(Layer.Category);
            std::snprintf(LayerSub, sizeof(LayerSub), "%.*s · %.0f%%",
                static_cast<int>(CatStr.size()), CatStr.data(), Layer.LayerOpacity * 100.0f);

            if (LayerMatchesSearch || SelectedLayer)
            {
                HierarchyRowSpec Spec {};
                Spec.DepthLevel        = 1;
                Spec.Title             = Layer.LayerTitle;
                Spec.Subtitle          = LayerSub;
                Spec.TagLabel          = SelectedLayer ? "ACTIVE" : "";
                Spec.TagStyle          = TagTone::Amber;
                Spec.CustomLeadPreview = true;
                Spec.LeadPreviewSize   = 30.0f;
                Spec.Expandable        = SelectedLayer;
                Spec.Expanded          = SelectedLayer;
                Spec.Selected          = SelectedLayer;
                Spec.Muted             = !Layer.VisibilityEnabled;
                Spec.ShowGreenCheck    = true;
                Spec.GreenCheckOn      = Layer.VisibilityEnabled;
                Spec.ShowEyeToggle     = true;
                Spec.EyeVisible        = Layer.VisibilityEnabled;
                Spec.ShowLockToggle    = true;
                Spec.LockEnabled       = LayerLocked;

                const Rectangle2D RowRect { RowX, RowY, RowW, 42.0f };
                const HierarchyRowResult Res = ConstructHierarchyRow(Surface, RowRect, Spec, Input, Tone);
                ConstructMaterialLayerPreview(Surface, Res.LeadPreviewRect, Layer, Recipe, !Layer.VisibilityEnabled);

                if (!AddLayerPopoverOpen)
                {
                    if (Res.CheckToggled || Res.EyeToggled)
                    {
                        Session->ToggleLayerVisibility(LIdx);
                    }
                    else if (Res.LockToggled)
                    {
                        LockedLayerMask ^= (1u << static_cast<uint32_t>(LIdx & 31u));
                    }
                    else if (Res.RowClicked)
                    {
                        Session->SelectLayer(LIdx);
                        Session->SelectedChannelOrdinal = 0u;
                    }
                }
                RowY += 44.0f;
            }

            // Nested optical channels under the active finish layer
            if (SelectedLayer)
            {
                const ColorRgba PriSwatch = LinearArrayToSrgbRgba(Params.PrimaryColorRgba);
                const ColorRgba SecSwatch = LinearArrayToSrgbRgba(Params.SecondaryColorRgba);
                const ColorRgba AccSwatch = LinearArrayToSrgbRgba(Params.AccentRadianceRgba);
                const float RoughEff      = std::clamp(Recipe.PrimaryFinish.Specular.Roughness + Params.RoughnessBias, 0.02f, 1.0f);
                const float MetalEff      = std::clamp(Recipe.PrimaryFinish.Specular.Metalness + Params.MetalnessBias, 0.0f, 1.0f);

                struct ChannelEntry
                {
                    uint32_t    Ordinal;
                    const char* Name;
                    std::string Subtitle;
                    bool        SolidSwatch;
                    ColorRgba   SwatchColor;
                    float       ScalarFill;
                    bool        Driven;
                };

                char RoughSub[32] {}, MetalSub[32] {}, IorSub[32] {}, CoatSub[32] {}, ScaleSub[32] {}, ReliefSub[32] {}, GlowSub[32] {};
                std::snprintf(RoughSub,  sizeof(RoughSub),  "Scalar · %.2f", RoughEff);
                std::snprintf(MetalSub,  sizeof(MetalSub),  "Scalar · %.2f", MetalEff);
                std::snprintf(IorSub,    sizeof(IorSub),    "IOR · %.2f", Params.OpticalIor);
                std::snprintf(CoatSub,   sizeof(CoatSub),   "Weight · %.2f", Recipe.PrimaryFinish.Clearcoat.CoatWeight);
                std::snprintf(ScaleSub,  sizeof(ScaleSub),  "Field · %.1fx", Params.DetailScale);
                std::snprintf(ReliefSub, sizeof(ReliefSub), "Field · %.2f", Params.ReliefStrength);
                std::snprintf(GlowSub,   sizeof(GlowSub),   "Radiance · %.2f", Params.SeamGlowIntensity);

                const std::array<ChannelEntry, 7u> Channels {{
                    { 0u, "Surface Colour", "sRGB · " + FormatHexRgba(PriSwatch), true,  PriSwatch, 1.0f, false },
                    { 1u, "Roughness",      RoughSub,                             false, ColorRgba{ 168u, 174u, 186u, 255u }, RoughEff, false },
                    { 2u, "Metalness",      MetalSub,                             false, ColorRgba{ 138u, 166u, 214u, 255u }, MetalEff, false },
                    { 3u, "Secondary Tint", "Field · " + FormatHexRgba(SecSwatch),true,  SecSwatch, 1.0f, true  },
                    { 4u, "Specular IOR",   IorSub,                               false, ColorRgba{ 190u, 200u, 216u, 255u }, (Params.OpticalIor - 1.0f) / 1.4f, false },
                    { 5u, "Relief & Scale", ReliefSub,                            false, AccSwatch, std::clamp(Params.ReliefStrength * 0.6f, 0.1f, 1.0f), true },
                    { 6u, "Seam Radiance",  GlowSub,                              false, AccSwatch, std::clamp(Params.SeamGlowIntensity * 0.25f, 0.1f, 1.0f), true },
                }};

                for (const auto& Ch : Channels)
                {
                    if (FilterDrivenOnly && !Ch.Driven) continue;
                    if (!ContainsInsensitive(Ch.Name, SearchNeedle)) continue;

                    HierarchyRowSpec ChSpec {};
                    ChSpec.DepthLevel        = 2;
                    ChSpec.Title             = Ch.Name;
                    ChSpec.Subtitle          = Ch.Subtitle;
                    ChSpec.TagLabel          = Ch.Driven ? "FIELD" : "";
                    ChSpec.TagStyle          = TagTone::Amber;
                    ChSpec.CustomLeadPreview = true;
                    ChSpec.LeadPreviewSize   = 22.0f;
                    ChSpec.Selected          = (Session->SelectedChannelOrdinal > 0u && Session->SelectedChannelOrdinal == Ch.Ordinal);

                    const Rectangle2D ChRect { RowX, RowY, RowW, 34.0f };
                    const HierarchyRowResult ChRes = ConstructHierarchyRow(Surface, ChRect, ChSpec, Input, Tone);
                    ConstructChannelSwatchPreview(Surface, ChRes.LeadPreviewRect, Ch.SolidSwatch, Ch.SwatchColor, Ch.ScalarFill, Tone);
                    if (!AddLayerPopoverOpen && ChRes.RowClicked)
                    {
                        Session->SelectedChannelOrdinal = Ch.Ordinal;
                    }
                    RowY += 36.0f;
                }
            }
        }
    }

    const float ContentH = (RowY + ScrollOffsetY) - TreeRect.Y;
    MaxScrollY           = std::max(0.0f, ContentH - TreeRect.Height);
    ScrollOffsetY        = std::clamp(ScrollOffsetY, 0.0f, MaxScrollY);
    Surface.PopClip();

    // 6. TextureStudio OLD Bottom Command Toolbar
    const OutlinerToolbarResult ToolOut = ConstructOutlinerCommandToolbar(
        Surface,
        ToolbarRect,
        SelectedLayerIdx > 0u,
        SelectedLayerIdx + 1u < Stack.Layers.size(),
        true,
        false,
        Stack.Layers.size() > 1u,
        Input,
        Tone);
    if (ToolOut.MoveUpClicked && SelectedLayerIdx > 0u)
    {
        if (Stack.ReorderLayer(SelectedLayerIdx, SelectedLayerIdx - 1u))
        {
            Session->SelectLayer(SelectedLayerIdx - 1u);
            Session->DirtyPreviewRequested = true;
        }
    }
    else if (ToolOut.MoveDownClicked && SelectedLayerIdx + 1u < Stack.Layers.size())
    {
        if (Stack.ReorderLayer(SelectedLayerIdx, SelectedLayerIdx + 1u))
        {
            Session->SelectLayer(SelectedLayerIdx + 1u);
            Session->DirtyPreviewRequested = true;
        }
    }
    else if (ToolOut.DuplicateClicked)
    {
        Session->AppendProceduralLayer();
    }
    else if (ToolOut.DeleteClicked && Stack.Layers.size() > 1u)
    {
        Session->RemoveSelectedLayer();
    }

    // 7. TextureStudio OLD OpenPBR Ground Reference Row
    ConstructOutlinerGroundRow(
        Surface,
        GroundRect,
        ControlIcon::MaterialSphere,
        IconAccentTone::Teal,
        "Layer 0 · OpenPBR Surface",
        "10-lobe direct studio shading",
        "OpenPBR",
        Tone);

    // 8. TextureStudio OLD 3-Metric Outliner Footer (LAYERS | DRIVEN | ALBEDO)
    const ColorRgba AlbedoSrgb = LinearArrayToSrgbRgba(Params.PrimaryColorRgba);
    const int AlbedoPct        = static_cast<int>(std::lround(AlbedoSrgb.Luminance() * 100.0f));
    char FLayers[16] {}, FDriven[16] {}, FAlbedo[16] {};
    std::snprintf(FLayers, sizeof(FLayers), "%u", TotalLayers);
    std::snprintf(FDriven, sizeof(FDriven), "%u", DrivenCount);
    std::snprintf(FAlbedo, sizeof(FAlbedo), "%d%%", AlbedoPct);
    const std::array<ThreeMetricFooterCell, 3u> FooterCells {{
        { "LAYERS", FLayers, TagTone::Neutral },
        { "DRIVEN", FDriven, DrivenCount > 0u ? TagTone::Green : TagTone::Neutral },
        { "ALBEDO", FAlbedo, TagTone::Neutral },
    }};
    ConstructThreeMetricFooter(Surface, MetricsRect, FooterCells, Tone);

    // 9. Add Layer Overlay Popover anchored to '+' button
    if (AddLayerPopoverOpen)
    {
        const std::array<PopoverOptionSpec, 2u> AddOptions {{
            { "Procedural field layer", "Noise & fracture detail", Tone.AccentBlue   },
            { "Transition layer",       "Dual-finish blend sweep", Tone.AccentViolet },
        }};
        bool Dismissed = false;
        const int32_t Picked = ConstructStudioPopoverList(
            Surface, AddButtonAnchor, Pane, "ADD MATERIAL LAYER", AddOptions, -1, Input, Tone, Dismissed);
        if (Picked == 0)
        {
            Session->AppendProceduralLayer();
            AddLayerPopoverOpen = false;
        }
        else if (Picked == 1)
        {
            Session->AppendTransitionLayer();
            AddLayerPopoverOpen = false;
        }
        else if (Dismissed)
        {
            AddLayerPopoverOpen = false;
        }
    }
}

} // namespace Frontier::Workspaces
