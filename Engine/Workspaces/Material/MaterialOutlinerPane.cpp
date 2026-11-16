/*============================================================================================================================================
                                                          MATERIALOUTLINERPANE.CPP
============================================================================================================================================*/
// 🧩 Implementation of MaterialOutlinerPane in Project Zero studio visual language wired live to MaterialWorkspaceSession

//------------------------------------------------------------------------------------------------------------------------
//                                                        INCLUDES
//------------------------------------------------------------------------------------------------------------------------

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

namespace
{

using namespace StudioSurface;

[[nodiscard]] ColorRgba LinearArrayToSrgbRgba(const std::array<float, 4u>& Linear) noexcept
{
    auto Convert = [](float Channel) noexcept -> uint8_t
    {
        const float Clamped = std::clamp(Channel, 0.0f, 1.0f);
        const float Gamma   = std::pow(Clamped, 1.0f / 2.2f);
        return static_cast<uint8_t>(std::clamp(std::lround(Gamma * 255.0f), 0l, 255l));
    };
    return { Convert(Linear[0]), Convert(Linear[1]), Convert(Linear[2]), 255u };
}

[[nodiscard]] bool ContainsCaseFold(std::string_view Haystack, std::string_view Needle) noexcept
{
    if (Needle.empty()) return true;
    if (Needle.size() > Haystack.size()) return false;
    for (std::size_t Pos = 0u; Pos + Needle.size() <= Haystack.size(); ++Pos)
    {
        bool Match = true;
        for (std::size_t Off = 0u; Off < Needle.size(); ++Off)
        {
            const unsigned char A = static_cast<unsigned char>(Haystack[Pos + Off]);
            const unsigned char B = static_cast<unsigned char>(Needle[Off]);
            if (std::tolower(A) != std::tolower(B))
            {
                Match = false;
                break;
            }
        }
        if (Match) return true;
    }
    return false;
}

[[nodiscard]] ControlIcon ResolveLayerIcon(MaterialSpace::Dynamic::MaterialLayerCategory Category) noexcept
{
    using MaterialSpace::Dynamic::MaterialLayerCategory;
    switch (Category)
    {
        case MaterialLayerCategory::FinishLayer:            return ControlIcon::MaterialSphere;
        case MaterialLayerCategory::ProceduralFieldLayer:   return ControlIcon::NoiseWave;
        case MaterialLayerCategory::MaskModifierLayer:      return ControlIcon::MaskCutout;
        case MaterialLayerCategory::SurfaceTransitionLayer: return ControlIcon::LayersSlabs;
    }
    return ControlIcon::MaterialSphere;
}

struct MaterialChannelEntrySpec
{
    uint32_t         Ordinal    = 0u;
    uint32_t         GroupIndex = 0u;
    std::string_view Label      {};
    bool             ColourSlot = false;
    bool             DrivenSlot = false;
};

constexpr std::array<std::string_view, 4u> ChannelGroupTitles {{
    "Core",
    "Specular & Optical",
    "Dynamic & Relief",
    "Coat & Thin-Film",
}};

constexpr std::array<MaterialChannelEntrySpec, 12u> ChannelRegistry {{
    { 0u,  0u, "Base Colour",       true,  false },
    { 1u,  0u, "Roughness",         false, false },
    { 2u,  0u, "Metalness",         false, false },
    { 3u,  0u, "Secondary Colour",  true,  true  },
    { 4u,  1u, "Dielectric IOR",    false, false },
    { 5u,  1u, "Opacity",           false, false },
    { 6u,  2u, "Detail Scale",      false, true  },
    { 7u,  2u, "Relief Strength",   false, true  },
    { 8u,  2u, "Seam Glow",         false, true  },
    { 9u,  2u, "Transition Sweep",  false, true  },
    { 10u, 3u, "Thin-Film Depth",   false, true  },
    { 11u, 3u, "Accent Radiance",   true,  true  },
}};

} // namespace

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
    const float  SafeScale = std::max(0.25f, ScaleFactor);
    const ImVec2 WinPos    = ImGui::GetWindowPos();
    const ImVec2 WinSize   = ImGui::GetWindowSize();
    const ImVec2 CursorPos = ImGui::GetCursorScreenPos();
    const float  TopY      = CursorPos.y - ImGui::GetStyle().WindowPadding.y;
    const float  PaneH     = std::max(1.0f, (WinPos.y + WinSize.y) - TopY);

    ImGui::InvisibleButton(
        "##MaterialOutlinerSurface",
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

void MaterialOutlinerPane::Render(
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

    auto& Recipe  = Session->AccessActiveRecipe();
    auto& Stack   = Recipe.LayerStack;
    auto& Params  = Recipe.Parameters;

    constexpr float PadX       = 12.0f;
    constexpr float FooterH    = 46.0f;
    const float     InnerW     = std::max(60.0f, ShellRect.Width - PadX * 2.0f);
    const float     LeftX      = ShellRect.X + PadX;
    float           CursorY    = ShellRect.Y + 13.0f;

    // 1. Outliner heading + round '+' button (appends procedural surface layer)
    const bool AddClicked = ConstructOutlinerHeading(
        Surface,
        { LeftX, CursorY, InnerW, 38.0f },
        "Outliner",
        "Material Scene & Strata",
        ControlIcon::PlusAdd,
        Input,
        Tone,
        Stack.Layers.size() >= 16u);
    if (AddClicked)
    {
        Session->AppendProceduralLayer();
    }
    CursorY += 46.0f;

    // 2. Stat tile pair: LAYERS & DRIVEN
    const uint32_t VisibleLayers = Stack.CountVisibleLayers();
    const uint32_t TotalLayers   = static_cast<uint32_t>(Stack.Layers.size());
    uint32_t       DrivenCount   = 0u;
    for (const auto& Layer : Stack.Layers)
    {
        if (Layer.VisibilityEnabled && Layer.Category != MaterialSpace::Dynamic::MaterialLayerCategory::FinishLayer)
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
        VisibleLayers == TotalLayers && TotalLayers > 0u,
        false,
        FilterEnabledOnly,
    };
    const OutlinerStatTileSpec RightStat {
        "DRIVEN",
        DrivenBuf,
        ControlIcon::NoiseWave,
        DrivenCount > 0u,
        false,
        FilterDrivenOnly,
    };
    const int32_t StatClicked = ConstructStatTilePair(Surface, { LeftX, CursorY, InnerW, 54.0f }, LeftStat, RightStat, Input, Tone);
    if (StatClicked == 0)
    {
        FilterEnabledOnly = !FilterEnabledOnly;
    }
    else if (StatClicked == 1)
    {
        FilterDrivenOnly = !FilterDrivenOnly;
    }
    CursorY += 62.0f;

    // 3. Search + Filter capsule row
    const std::string_view FilterTitle = (FilterEnabledOnly || FilterDrivenOnly) ? "Filtered" : "Filter";
    const bool FilterClicked = ConstructSearchFilterRow(
        Surface,
        { LeftX, CursorY, InnerW, 32.0f },
        SearchBuffer,
        "Search layers, channels...",
        FilterTitle,
        FilterEnabledOnly || FilterDrivenOnly,
        Input,
        Tone);
    if (FilterClicked)
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
        }
    }
    CursorY += 40.0f;

    // 4. Quick-add layer chips matching HTML detail finish presets
    const std::array<OutlinerChipSpec, 3u> Chips {{
        { "Detail",     { 255u, 178u, 56u, 255u },  { 168u, 96u,  36u, 255u }, true, false, false, Stack.Layers.size() >= 16u },
        { "Procedural", { 77u,  155u, 255u, 255u }, { 48u,  208u, 176u, 255u }, true, false, false, Stack.Layers.size() >= 16u },
        { "Transition", { 196u, 140u, 255u, 255u }, { 255u, 107u, 94u, 255u },  true, false, false, Stack.Layers.size() >= 16u },
    }};
    float ChipsH = 0.0f;
    const int32_t ChipClicked = ConstructQuickChipStrip(Surface, LeftX, CursorY, InnerW, Chips, Input, Tone, ChipsH);
    if (ChipClicked == 0 || ChipClicked == 1)
    {
        Session->AppendProceduralLayer();
    }
    else if (ChipClicked == 2)
    {
        Session->AppendTransitionLayer();
    }
    CursorY += ChipsH + 8.0f;

    Surface.StrokeLine({ ShellRect.X + 1.0f, CursorY }, { ShellRect.Right() - 1.0f, CursorY }, Tone.Hairline, 1.0f);
    CursorY += 6.0f;

    // 5. Scrollable hierarchy viewport
    const Rectangle2D FooterRect { ShellRect.X + 1.0f, ShellRect.Bottom() - FooterH, ShellRect.Width - 2.0f, FooterH };
    const Rectangle2D TreeViewport { ShellRect.X + 4.0f, CursorY, ShellRect.Width - 8.0f, std::max(40.0f, FooterRect.Y - CursorY - 4.0f) };

    const Point2D Cursor = QueryInputCursor(Input);
    if (TreeViewport.Contains(Cursor) && std::fabs(Input.WheelDeltaY) > 1.0e-4f)
    {
        ScrollOffsetY = std::clamp(ScrollOffsetY - Input.WheelDeltaY * 34.0f, 0.0f, MaxScrollY);
    }

    const float RowW      = TreeViewport.Width - 8.0f;
    const float RowLeftX  = TreeViewport.X + 4.0f;
    const float StartY    = TreeViewport.Y - ScrollOffsetY;
    float       RowY      = StartY;
    constexpr float RowH  = 37.0f;
    constexpr float RowGap = 2.0f;
    const std::string_view Query = QueryEditTextView(SearchBuffer);

    Surface.PushClip(TreeViewport);

    // 5a. Active material root row
    char RootMeta[64] {};
    std::snprintf(RootMeta, sizeof(RootMeta), "%s · %u/%u strata", Recipe.CategoryLabel.c_str(), VisibleLayers, TotalLayers);
    {
        HierarchyRowSpec RootSpec {};
        RootSpec.DepthLevel   = 0;
        RootSpec.Title        = Recipe.DisplayTitle;
        RootSpec.Subtitle     = RootMeta;
        RootSpec.LeadIcon     = ControlIcon::MaterialSphere;
        RootSpec.LeadTileFill = Tone.AmberWash;
        RootSpec.LeadIconInk  = Tone.Amber;
        RootSpec.Expandable   = true;
        RootSpec.Expanded     = RootExpanded || !Query.empty();
        const auto Hit = ConstructHierarchyRow(Surface, { RowLeftX, RowY, RowW, RowH }, RootSpec, Input, Tone);
        if (Hit.ChevronClicked || Hit.RowClicked)
        {
            RootExpanded = !RootExpanded;
        }
        RowY += RowH + RowGap;
    }

    // 5b. Surface layers + active layer's grouped optical channels
    const std::size_t SelectedLayerIdx = Session->QuerySelectedLayerIndex();
    if (RootExpanded || !Query.empty())
    {
        for (std::size_t LayerIdx = 0u; LayerIdx < Stack.Layers.size(); ++LayerIdx)
        {
            const auto& Layer = Stack.Layers[LayerIdx];
            if (FilterEnabledOnly && !Layer.VisibilityEnabled)
            {
                continue;
            }
            const bool LayerSelected = (LayerIdx == SelectedLayerIdx);
            const std::string_view CatLabel = MaterialSpace::Dynamic::QueryLayerCategoryLabel(Layer.Category);
            char LayerMeta[64] {};
            std::snprintf(LayerMeta, sizeof(LayerMeta), "%.*s · %.0f%%",
                static_cast<int>(CatLabel.size()), CatLabel.data(), Layer.LayerOpacity * 100.0f);

            HierarchyRowSpec LayerSpec {};
            LayerSpec.DepthLevel      = 1;
            LayerSpec.Title           = Layer.LayerTitle;
            LayerSpec.Subtitle        = LayerMeta;
            LayerSpec.LeadIcon        = ResolveLayerIcon(Layer.Category);
            LayerSpec.LeadTileFill    = LayerSelected ? Tone.AmberWash : Tone.TileSurface;
            LayerSpec.LeadIconInk     = LayerSelected ? Tone.Amber : Tone.TextSecondary;
            LayerSpec.Expandable      = LayerSelected;
            LayerSpec.Expanded        = LayerSelected;
            LayerSpec.Selected        = LayerSelected && Session->SelectedChannelOrdinal >= 12u;
            LayerSpec.Muted           = !Layer.VisibilityEnabled;
            LayerSpec.TagLabel        = LayerSelected ? "ACTIVE" : "";
            LayerSpec.TagStyle        = TagTone::Amber;
            LayerSpec.TrailingSwitch  = TrailingSwitchKind::GreenCheck;
            LayerSpec.TrailingEnabled = Layer.VisibilityEnabled;

            if (RowY + RowH >= TreeViewport.Y && RowY <= TreeViewport.Bottom())
            {
                const auto Hit = ConstructHierarchyRow(Surface, { RowLeftX, RowY, RowW, RowH }, LayerSpec, Input, Tone);
                if (Hit.TrailingToggled)
                {
                    Session->ToggleLayerVisibility(LayerIdx);
                }
                else if (Hit.RowClicked)
                {
                    Session->SelectLayer(LayerIdx);
                }
            }
            RowY += RowH + RowGap;

            // Render channel groups under the selected layer
            if (LayerSelected)
            {
                for (uint32_t GroupIdx = 0u; GroupIdx < 4u; ++GroupIdx)
                {
                    uint32_t GroupMatched = 0u;
                    uint32_t GroupDriven  = 0u;
                    for (const auto& Ch : ChannelRegistry)
                    {
                        if (Ch.GroupIndex != GroupIdx) continue;
                        if (FilterDrivenOnly && !Ch.DrivenSlot) continue;
                        if (!ContainsCaseFold(Ch.Label, Query) && !ContainsCaseFold(ChannelGroupTitles[GroupIdx], Query)) continue;
                        ++GroupMatched;
                        if (Ch.DrivenSlot) ++GroupDriven;
                    }
                    if (GroupMatched == 0u) continue;

                    const bool GroupOpen = !Query.empty() || ((ExpandedGroupMask & (1u << GroupIdx)) != 0u);
                    char GroupMeta[48] {};
                    if (GroupDriven > 0u)
                    {
                        std::snprintf(GroupMeta, sizeof(GroupMeta), "%u ch · %u driven", GroupMatched, GroupDriven);
                    }
                    else
                    {
                        std::snprintf(GroupMeta, sizeof(GroupMeta), "%u ch", GroupMatched);
                    }

                    HierarchyRowSpec GrpSpec {};
                    GrpSpec.DepthLevel   = 2;
                    GrpSpec.Title        = ChannelGroupTitles[GroupIdx];
                    GrpSpec.Subtitle     = GroupMeta;
                    GrpSpec.LeadIcon     = (GroupIdx == 2u) ? ControlIcon::NoiseWave : ControlIcon::SlidersQuality;
                    GrpSpec.LeadTileFill = Tone.InsetSurface;
                    GrpSpec.LeadIconInk  = (GroupDriven > 0u) ? Tone.Amber : Tone.TextMuted;
                    GrpSpec.Expandable   = true;
                    GrpSpec.Expanded     = GroupOpen;
                    GrpSpec.GroupStyle   = true;

                    if (RowY + RowH >= TreeViewport.Y && RowY <= TreeViewport.Bottom())
                    {
                        const auto Hit = ConstructHierarchyRow(Surface, { RowLeftX, RowY, RowW, RowH }, GrpSpec, Input, Tone);
                        if (Hit.RowClicked || Hit.ChevronClicked)
                        {
                            ExpandedGroupMask ^= (1u << GroupIdx);
                        }
                    }
                    RowY += RowH + RowGap;

                    if (!GroupOpen) continue;

                    for (const auto& Ch : ChannelRegistry)
                    {
                        if (Ch.GroupIndex != GroupIdx) continue;
                        if (FilterDrivenOnly && !Ch.DrivenSlot) continue;
                        if (!ContainsCaseFold(Ch.Label, Query) && !ContainsCaseFold(ChannelGroupTitles[GroupIdx], Query)) continue;

                        char SlotMeta[48] {};
                        ColorRgba SwatchCol = Tone.TileSurface;
                        if (Ch.Ordinal == 0u)
                        {
                            SwatchCol = LinearArrayToSrgbRgba(Params.PrimaryColorRgba);
                            std::snprintf(SlotMeta, sizeof(SlotMeta), "Value · #%02x%02x%02x", SwatchCol.Red, SwatchCol.Green, SwatchCol.Blue);
                        }
                        else if (Ch.Ordinal == 1u)
                        {
                            const float Rough = std::clamp(Recipe.PrimaryFinish.Specular.Roughness + Params.RoughnessBias, 0.02f, 1.0f);
                            std::snprintf(SlotMeta, sizeof(SlotMeta), "Scalar · %.2f", Rough);
                        }
                        else if (Ch.Ordinal == 2u)
                        {
                            const float Metal = std::clamp(Recipe.PrimaryFinish.Specular.Metalness + Params.MetalnessBias, 0.0f, 1.0f);
                            std::snprintf(SlotMeta, sizeof(SlotMeta), "Scalar · %.2f", Metal);
                        }
                        else if (Ch.Ordinal == 3u)
                        {
                            SwatchCol = LinearArrayToSrgbRgba(Params.SecondaryColorRgba);
                            std::snprintf(SlotMeta, sizeof(SlotMeta), "Field · #%02x%02x%02x", SwatchCol.Red, SwatchCol.Green, SwatchCol.Blue);
                        }
                        else if (Ch.Ordinal == 4u)
                        {
                            std::snprintf(SlotMeta, sizeof(SlotMeta), "Scalar · %.2f", Params.OpticalIor);
                        }
                        else if (Ch.Ordinal == 5u)
                        {
                            std::snprintf(SlotMeta, sizeof(SlotMeta), "Scalar · %.0f%%", Params.OpacityScale * 100.0f);
                        }
                        else if (Ch.Ordinal == 6u)
                        {
                            std::snprintf(SlotMeta, sizeof(SlotMeta), "Field · %.1fx", Params.DetailScale);
                        }
                        else if (Ch.Ordinal == 7u)
                        {
                            std::snprintf(SlotMeta, sizeof(SlotMeta), "Field · %.2f", Params.ReliefStrength);
                        }
                        else if (Ch.Ordinal == 8u)
                        {
                            std::snprintf(SlotMeta, sizeof(SlotMeta), "Field · %.2f", Params.SeamGlowIntensity);
                        }
                        else if (Ch.Ordinal == 9u)
                        {
                            std::snprintf(SlotMeta, sizeof(SlotMeta), "Sweep · %.0f%%", Params.TransitionProgress * 100.0f);
                        }
                        else if (Ch.Ordinal == 10u)
                        {
                            std::snprintf(SlotMeta, sizeof(SlotMeta), "Wave · %.0f nm", Params.ThinFilmNanometers);
                        }
                        else
                        {
                            SwatchCol = LinearArrayToSrgbRgba(Params.AccentRadianceRgba);
                            std::snprintf(SlotMeta, sizeof(SlotMeta), "Glow · %.1fx", Params.AccentRadianceRgba[3]);
                        }

                        HierarchyRowSpec ChSpec {};
                        ChSpec.DepthLevel      = 3;
                        ChSpec.Title           = Ch.Label;
                        ChSpec.Subtitle        = SlotMeta;
                        ChSpec.LeadSolidSwatch = Ch.ColourSlot;
                        ChSpec.LeadTileFill    = Ch.ColourSlot ? SwatchCol : Tone.RecessedWell;
                        ChSpec.LeadIcon        = Ch.DrivenSlot ? ControlIcon::NoiseWave : ControlIcon::SlidersQuality;
                        ChSpec.LeadIconInk     = Ch.DrivenSlot ? Tone.Amber : Tone.TextSecondary;
                        ChSpec.Selected        = (Session->SelectedChannelOrdinal == Ch.Ordinal);
                        ChSpec.TagLabel        = Ch.DrivenSlot ? "FIELD" : "";
                        ChSpec.TagStyle        = TagTone::Amber;

                        if (RowY + RowH >= TreeViewport.Y && RowY <= TreeViewport.Bottom())
                        {
                            const auto Hit = ConstructHierarchyRow(Surface, { RowLeftX, RowY, RowW, RowH }, ChSpec, Input, Tone);
                            if (Hit.RowClicked)
                            {
                                Session->SelectedChannelOrdinal = Ch.Ordinal;
                            }
                        }
                        RowY += RowH + RowGap;
                    }
                }
            }
        }
    }

    // 5c. Preset Library section at bottom (all 13 dynamic recipes)
    {
        char LibMeta[48] {};
        std::snprintf(LibMeta, sizeof(LibMeta), "%zu materials", Session->PresetRepository.size());
        HierarchyRowSpec LibHead {};
        LibHead.DepthLevel   = 0;
        LibHead.Title        = "Library";
        LibHead.Subtitle     = LibMeta;
        LibHead.LeadIcon     = ControlIcon::FolderClosed;
        LibHead.LeadTileFill = Tone.InsetSurface;
        LibHead.LeadIconInk  = Tone.TextMuted;
        LibHead.Expandable   = true;
        LibHead.Expanded     = LibraryExpanded || !Query.empty();
        LibHead.GroupStyle   = true;

        if (RowY + RowH >= TreeViewport.Y && RowY <= TreeViewport.Bottom())
        {
            const auto Hit = ConstructHierarchyRow(Surface, { RowLeftX, RowY, RowW, RowH }, LibHead, Input, Tone);
            if (Hit.RowClicked || Hit.ChevronClicked)
            {
                LibraryExpanded = !LibraryExpanded;
            }
        }
        RowY += RowH + RowGap;

        if (LibraryExpanded || !Query.empty())
        {
            for (const auto& Preset : Session->PresetRepository)
            {
                if (!ContainsCaseFold(Preset.DisplayTitle, Query) && !ContainsCaseFold(Preset.CategoryLabel, Query))
                {
                    continue;
                }
                const bool ActivePreset = (Preset.PresetIdentity == Recipe.PresetIdentity);
                HierarchyRowSpec PresetSpec {};
                PresetSpec.DepthLevel      = 1;
                PresetSpec.Title           = Preset.DisplayTitle;
                PresetSpec.Subtitle        = Preset.CategoryLabel;
                PresetSpec.LeadSolidSwatch = true;
                PresetSpec.LeadTileFill    = LinearArrayToSrgbRgba(Preset.Parameters.PrimaryColorRgba);
                PresetSpec.Selected        = ActivePreset;
                PresetSpec.TagLabel        = ActivePreset ? "LOADED" : "";
                PresetSpec.TagStyle        = TagTone::Green;

                if (RowY + RowH >= TreeViewport.Y && RowY <= TreeViewport.Bottom())
                {
                    const auto Hit = ConstructHierarchyRow(Surface, { RowLeftX, RowY, RowW, RowH }, PresetSpec, Input, Tone);
                    if (Hit.RowClicked && !ActivePreset)
                    {
                        Session->SelectPresetIdentity(Preset.PresetIdentity);
                    }
                }
                RowY += RowH + RowGap;
            }
        }
    }

    Surface.PopClip();
    const float TotalHeight = (RowY - StartY) + 8.0f;
    MaxScrollY              = std::max(0.0f, TotalHeight - TreeViewport.Height);
    ScrollOffsetY           = std::clamp(ScrollOffsetY, 0.0f, MaxScrollY);

    // 6. Four-column physical telemetry footer (F0 / CLASS / GLOSS / ALBEDO)
    const float Ior   = std::clamp(Params.OpticalIor, 1.0f, 3.0f);
    const float F0    = std::pow((Ior - 1.0f) / (Ior + 1.0f), 2.0f) * 100.0f;
    const float Metal = std::clamp(Recipe.PrimaryFinish.Specular.Metalness + Params.MetalnessBias, 0.0f, 1.0f);
    const float Rough = std::clamp(Recipe.PrimaryFinish.Specular.Roughness + Params.RoughnessBias, 0.02f, 1.0f);
    const ColorRgba AlbedoSrgb = LinearArrayToSrgbRgba(Params.PrimaryColorRgba);
    const int AlbedoPct        = static_cast<int>(std::lround(AlbedoSrgb.Luminance() * 100.0f));

    char F0Buf[16] {};
    char AlbBuf[16] {};
    std::snprintf(F0Buf, sizeof(F0Buf), "%.1f", F0);
    std::snprintf(AlbBuf, sizeof(AlbBuf), "%d", AlbedoPct);

    const std::string_view ClassLabel = (Metal > 0.78f) ? "Metal" : ((Metal > 0.18f) ? "Hybrid" : "Dielectric");
    const std::string_view GlossLabel = (Rough < 0.16f) ? "Mirror" : ((Rough < 0.38f) ? "Glossy" : ((Rough < 0.68f) ? "Satin" : "Matte"));

    const std::array<FooterMetricColumn, 4u> FooterCols {{
        { "F0",     F0Buf,      "%", TagTone::Neutral },
        { "CLASS",  ClassLabel, "",  (Metal > 0.18f && Metal < 0.78f) ? TagTone::Amber : TagTone::Neutral },
        { "GLOSS",  GlossLabel, "",  TagTone::Neutral },
        { "ALBEDO", AlbBuf,     "%", (AlbedoPct < 4 || AlbedoPct > 86) ? TagTone::Amber : TagTone::Neutral },
    }};
    ConstructFourColumnFooter(Surface, FooterRect, FooterCols, Tone);
}

} // namespace Frontier::Workspaces
