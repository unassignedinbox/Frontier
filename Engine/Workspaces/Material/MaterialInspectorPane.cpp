/*============================================================================================================================================
                                                         MATERIALINSPECTORPANE.CPP
============================================================================================================================================*/
// 🧩 Implementation of MaterialInspectorPane in Project Zero studio visual language wired live to MaterialWorkspaceSession

//------------------------------------------------------------------------------------------------------------------------
//                                                        INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Engine/Workspaces/Material/MaterialInspectorPane.h"
#include "Engine/Workspaces/Texturing/StudioSurfaceKit.h"

#include <imgui.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

namespace Frontier::Workspaces
{

namespace
{

using namespace StudioSurface;

[[nodiscard]] ColorRgba LinearToSrgbSwatch(const std::array<float, 4u>& Linear) noexcept
{
    auto Convert = [](float Channel) noexcept -> uint8_t
    {
        const float Clamped = std::clamp(Channel, 0.0f, 1.0f);
        const float Gamma   = std::pow(Clamped, 1.0f / 2.2f);
        return static_cast<uint8_t>(std::clamp(std::lround(Gamma * 255.0f), 0l, 255l));
    };
    return { Convert(Linear[0]), Convert(Linear[1]), Convert(Linear[2]), 255u };
}

void AssignSrgbToLinearArray(ColorRgba Swatch, std::array<float, 4u>& OutLinear) noexcept
{
    OutLinear[0] = std::pow(static_cast<float>(Swatch.Red) / 255.0f, 2.2f);
    OutLinear[1] = std::pow(static_cast<float>(Swatch.Green) / 255.0f, 2.2f);
    OutLinear[2] = std::pow(static_cast<float>(Swatch.Blue) / 255.0f, 2.2f);
}

[[nodiscard]] bool MatchSubstringFold(std::string_view Haystack, std::string_view Needle) noexcept
{
    if (Needle.empty()) return true;
    if (Needle.size() > Haystack.size()) return false;
    for (std::size_t Pos = 0u; Pos + Needle.size() <= Haystack.size(); ++Pos)
    {
        bool Hit = true;
        for (std::size_t Off = 0u; Off < Needle.size(); ++Off)
        {
            if (std::tolower(static_cast<unsigned char>(Haystack[Pos + Off])) !=
                std::tolower(static_cast<unsigned char>(Needle[Off])))
            {
                Hit = false;
                break;
            }
        }
        if (Hit) return true;
    }
    return false;
}

struct InspectorChannelMeta
{
    uint32_t         Ordinal    = 0u;
    uint32_t         GroupIdx   = 0u;
    std::string_view Title      {};
    std::string_view Hint       {};
    bool             ColourSlot = false;
    bool             DrivenSlot = false;
    float            MinLimit   = 0.0f;
    float            MaxLimit   = 1.0f;
    std::string_view UnitSuffix {};
    int32_t          Decimals   = 2;
};

constexpr std::array<std::string_view, 4u> GroupHeadings {{
    "Core",
    "Specular & Optical",
    "Dynamic & Relief",
    "Coat & Thin-Film",
}};

constexpr std::array<InspectorChannelMeta, 12u> ChannelDefs {{
    { 0u,  0u, "Base colour",      "Diffuse / conductor albedo",   true,  false, 0.0f,  1.0f,    "",   2 },
    { 1u,  0u, "Roughness",        "Microfacet GGX spread",        false, false, 0.02f, 1.0f,    "",   2 },
    { 2u,  0u, "Metalness",        "Dielectric to conductor",      false, false, 0.0f,  1.0f,    "",   2 },
    { 3u,  0u, "Secondary colour", "Transition / vein pigment",    true,  true,  0.0f,  1.0f,    "",   2 },
    { 4u,  1u, "Dielectric IOR",   "Fresnel F0 refractive index",  false, false, 1.0f,  2.5f,    "",   2 },
    { 5u,  1u, "Opacity",          "Global translucency scale",    false, false, 0.0f,  1.0f,    "",   2 },
    { 6u,  2u, "Detail scale",     "Procedural spatial frequency", false, true,  0.5f,  16.0f,   "x",  1 },
    { 7u,  2u, "Relief strength",  "Normal perturbation depth",    false, true,  0.0f,  2.0f,    "",   2 },
    { 8u,  2u, "Seam glow",        "Luminous fracture gain",       false, true,  0.0f,  4.0f,    "x",  2 },
    { 9u,  2u, "Transition sweep", "Dissolve / scan progression",  false, true,  0.0f,  1.0f,    "",   2 },
    { 10u, 3u, "Thin-film depth",  "Optical Airy film thickness",  false, true,  120.f, 1200.0f, "nm", 0 },
    { 11u, 3u, "Accent radiance",  "Emissive highlight tint",      true,  true,  0.0f,  4.0f,    "x",  2 },
}};

constexpr std::array<ColorRgba, 8u> QuickStudioSwatches {{
    { 232u, 226u, 214u, 255u },
    { 126u, 88u,  59u,  255u },
    { 255u, 118u, 42u,  255u },
    { 238u, 178u, 56u,  255u },
    { 70u,  188u, 112u, 255u },
    { 52u,  168u, 232u, 255u },
    { 138u, 98u,  218u, 255u },
    { 36u,  40u,  48u,  255u },
}};

} // namespace

MaterialInspectorPane::~MaterialInspectorPane() noexcept
{
    WorkspaceAppearance::ReleasePanePointerOwnership(this);
}

void MaterialInspectorPane::AttachSession(MaterialWorkspaceSession* TargetSession) noexcept
{
    Session = TargetSession;
}

void MaterialInspectorPane::PresentInImGuiPane(double DeltaSeconds, float ScaleFactor)
{
    const float  SafeScale = std::max(0.25f, ScaleFactor);
    const ImVec2 WinPos    = ImGui::GetWindowPos();
    const ImVec2 WinSize   = ImGui::GetWindowSize();
    const ImVec2 CursorPos = ImGui::GetCursorScreenPos();
    const float  TopY      = CursorPos.y - ImGui::GetStyle().WindowPadding.y;
    const float  PaneH     = std::max(1.0f, (WinPos.y + WinSize.y) - TopY);

    ImGui::InvisibleButton(
        "##MaterialInspectorSurface",
        ImVec2(std::max(1.0f, ImGui::GetContentRegionAvail().x), std::max(1.0f, ImGui::GetContentRegionAvail().y)),
        ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);

    const Rectangle2D ScreenBounds { WinPos.x, TopY, WinSize.x, PaneH };
    const Rectangle2D LogicalBounds{ 0.0f, 0.0f, WinSize.x / SafeScale, PaneH / SafeScale };

    PixelSpace       Surface(ImGui::GetWindowDrawList(), { ScreenBounds.X, ScreenBounds.Y }, SafeScale);
    PaneInputReading Input = WorkspaceAppearance::CaptureImGuiPaneInput(ScreenBounds, SafeScale);
    WorkspaceAppearance::EnforcePanePointerOwnership(this, Input);

    Render(Surface, LogicalBounds, Input, DeltaSeconds);
    WorkspaceAppearance::UpdatePanePointerOwnership(this, ScreenBounds, SafeScale, Input, ActiveSliderId >= 0);
}

void MaterialInspectorPane::Render(
    PixelSpace&             Surface,
    Rectangle2D             Bounds,
    const PaneInputReading& Input,
    double                  /*DeltaSeconds*/)
{
    const StudioToneSet Tone      = QueryStudioToneSet();
    const Rectangle2D   ShellRect = ConstructDockedSurfaceShell(Surface, Bounds, false, Tone);
    if (Session == nullptr)
    {
        return;
    }

    auto& Recipe = Session->AccessActiveRecipe();
    auto& Params = Recipe.Parameters;

    constexpr float PadX    = 14.0f;
    constexpr float FooterH = 66.0f;
    const float     InnerW  = std::max(80.0f, ShellRect.Width - PadX * 2.0f);
    const float     LeftX   = ShellRect.X + PadX;
    float           TopY    = ShellRect.Y + 8.0f;

    // 1. Fixed Inspector Header Bar (amber mark + Material Inspector / PBR LOOK + Reset)
    const InspectorHeadingResult HeadOut = ConstructInspectorHeading(
        Surface,
        { LeftX, TopY, InnerW, 42.0f },
        ControlIcon::MaterialSphere,
        "Material Inspector",
        "LOOK-DEV PBR",
        false,
        false,
        false,
        true,
        Input,
        Tone);
    if (HeadOut.ResetClicked)
    {
        Session->SelectPresetIdentity(Recipe.PresetIdentity);
    }
    TopY += 50.0f;

    // 2. Scrollable Body between Header and Footer
    const Rectangle2D FooterRect { ShellRect.X + 1.0f, ShellRect.Bottom() - FooterH, ShellRect.Width - 2.0f, FooterH };
    const Rectangle2D BodyRect   { ShellRect.X + 4.0f, TopY, ShellRect.Width - 8.0f, std::max(60.0f, FooterRect.Y - TopY - 4.0f) };

    const Point2D Cursor = QueryInputCursor(Input);
    if (!PresetPopoverOpen && BodyRect.Contains(Cursor) && std::fabs(Input.WheelDeltaY) > 1.0e-4f)
    {
        ScrollOffsetY = std::clamp(ScrollOffsetY - Input.WheelDeltaY * 36.0f, 0.0f, MaxScrollY);
    }

    const float StartY = BodyRect.Y + 4.0f - ScrollOffsetY;
    float       Y      = StartY;
    const float BodyX  = LeftX;

    Surface.PushClip(BodyRect);

    // 2a. Identity Block (MATERIAL / DisplayTitle / Preset Selector)
    char SubtitleBuf[96] {};
    std::snprintf(SubtitleBuf, sizeof(SubtitleBuf), "%s · OpenPBR 1.1 · %zu strata",
        Recipe.CategoryLabel.c_str(), Recipe.LayerStack.Layers.size());
    const bool SelectorClicked = ConstructIdentityBlock(
        Surface,
        { BodyX, Y, InnerW, 56.0f },
        "MATERIAL",
        Recipe.DisplayTitle,
        SubtitleBuf,
        "Material",
        PresetPopoverOpen,
        Input,
        Tone,
        &PresetAnchorRect,
        false,
        nullptr);
    if (SelectorClicked)
    {
        PresetPopoverOpen = !PresetPopoverOpen;
    }
    Y += 64.0f;

    // 2b. Three Project Zero Meter Stat Tiles (Response / Albedo / Signal)
    const float Rough = std::clamp(Recipe.PrimaryFinish.Specular.Roughness + Params.RoughnessBias, 0.02f, 1.0f);
    const float Metal = std::clamp(Recipe.PrimaryFinish.Specular.Metalness + Params.MetalnessBias, 0.0f, 1.0f);
    const float Ior   = std::clamp(Params.OpticalIor, 1.0f, 3.0f);
    const float F0Pct = std::pow((Ior - 1.0f) / (Ior + 1.0f), 2.0f) * 100.0f;
    const ColorRgba AlbedoSwatch = LinearToSrgbSwatch(Params.PrimaryColorRgba);
    const float     AlbedoLum    = AlbedoSwatch.Luminance();
    const int       AlbedoPct  = static_cast<int>(std::lround(AlbedoLum * 100.0f));
    const bool      AlbedoSafe = (AlbedoPct >= 4 && AlbedoPct <= 86);

    char RespBuf[24] {};
    char RespCap[32] {};
    char AlbBuf[16] {};
    char SigBuf[16] {};
    std::snprintf(RespBuf, sizeof(RespBuf), "%.2f", Rough);
    std::snprintf(RespCap, sizeof(RespCap), "F0 %.1f%% · %s", F0Pct, (Metal > 0.7f) ? "Metal" : "Diel");
    std::snprintf(AlbBuf, sizeof(AlbBuf), "%d", AlbedoPct);
    std::snprintf(SigBuf, sizeof(SigBuf), "7/12");

    std::array<MeterStatTileSpec, 3u> StatTiles {};
    StatTiles[0].Label         = "Response";
    StatTiles[0].Reading       = RespBuf;
    StatTiles[0].Caption       = RespCap;
    StatTiles[0].PrimaryAccent = true;
    StatTiles[0].MeterKind     = MeterVariant::BandDot;
    StatTiles[0].MeterRatio    = Rough;
    StatTiles[0].BandLow       = 0.12f;
    StatTiles[0].BandHigh      = 0.65f;
    StatTiles[0].MeterColour   = Tone.Amber;

    StatTiles[1].Label       = "Albedo";
    StatTiles[1].Reading     = AlbBuf;
    StatTiles[1].Unit        = "%";
    StatTiles[1].Caption     = AlbedoSafe ? "Physically safe" : "Extreme range";
    StatTiles[1].ReadingTone = AlbedoSafe ? TagTone::Neutral : TagTone::Amber;
    StatTiles[1].MeterKind   = MeterVariant::BandDot;
    StatTiles[1].MeterRatio  = AlbedoLum;
    StatTiles[1].BandLow     = 0.04f;
    StatTiles[1].BandHigh    = 0.86f;
    StatTiles[1].MeterColour = AlbedoSafe ? Tone.Green : Tone.Amber;

    StatTiles[2].Label         = "Signal";
    StatTiles[2].Reading       = SigBuf;
    StatTiles[2].Caption       = "Procedural driven";
    StatTiles[2].MeterKind     = MeterVariant::StackBar;
    StatTiles[2].StackCount    = 3u;
    StatTiles[2].StackParts[0] = { 5.0f, Tone.Amber };
    StatTiles[2].StackParts[1] = { 2.0f, Tone.Blue };
    StatTiles[2].StackParts[2] = { 5.0f, Tone.RowSelected };

    ConstructMeterStatTrio(Surface, { BodyX, Y, InnerW, 74.0f }, StatTiles, Tone);
    Y += 84.0f;

    // 2c. Channels heading + '+ Layer' capsule + search input
    WorkspaceAppearance::ConstructWeightedText(
        Surface, FontWeightClass::Medium, { BodyX, Y + 4.0f }, 12.0f, Tone.TextPrimary, "Channels");

    const Rectangle2D AddLayerBtn { BodyX + InnerW - 88.0f, Y, 88.0f, 24.0f };
    const bool AddLayerHover = AddLayerBtn.Contains(Cursor);
    Surface.FillRectangle(AddLayerBtn, AddLayerHover ? Tone.RowSelected : Tone.TileSurface, 12.0f);
    Surface.StrokeRectangle(AddLayerBtn, AddLayerHover ? Tone.BorderStrong : Tone.BorderSubtle, 12.0f, 1.0f);
    ConstructCenteredWeightedText(
        Surface, FontWeightClass::Medium, AddLayerBtn, 9.5f, Tone.TextSecondary, "+ Layer");
    if (AddLayerHover && QueryPrimaryPressed(Input))
    {
        Session->AppendProceduralLayer();
    }
    Y += 32.0f;

    // Quiet inline channel search bar
    const Rectangle2D SearchBox { BodyX, Y, InnerW, 28.0f };
    Surface.FillRectangle(SearchBox, Tone.InsetSurface, 14.0f);
    Surface.StrokeRectangle(SearchBox, ChannelSearchBuffer.Active ? Tone.Amber : Tone.Hairline, 14.0f, 1.0f);
    VectorIconRepository::ConstructControl(Surface, ControlIcon::SearchGlass, {
        SearchBox.X + 10.0f, SearchBox.Center().Y - 5.5f, 11.0f, Tone.TextFaint, 1.85f
    });
    if (QueryPrimaryPressed(Input) && !PresetPopoverOpen)
    {
        if (SearchBox.Contains(Cursor) && !ChannelSearchBuffer.Active)
        {
            ChannelSearchBuffer.Begin(ChannelSearchBuffer.Text);
        }
        else if (!SearchBox.Contains(Cursor) && ChannelSearchBuffer.Active)
        {
            ChannelSearchBuffer.Finalize();
        }
    }
    const Rectangle2D SearchInner { SearchBox.X + 26.0f, SearchBox.Y + 1.0f, std::max(10.0f, SearchBox.Width - 34.0f), SearchBox.Height - 2.0f };
    if (ChannelSearchBuffer.Active)
    {
        (void)TextInputField::Construct(Surface, SearchInner, ChannelSearchBuffer, Input.PrimaryPointer, 10.0f);
    }
    else
    {
        const bool Empty = (ChannelSearchBuffer.Length == 0u);
        WorkspaceAppearance::ConstructWeightedText(
            Surface,
            FontWeightClass::Regular,
            { SearchInner.X, SearchBox.Center().Y - 5.2f },
            10.0f,
            Empty ? Tone.TextFaint : Tone.TextPrimary,
            Empty ? std::string_view("Filter channels...") : QueryEditTextView(ChannelSearchBuffer));
    }
    Y += 36.0f;

    // 2d. Grouped Channel Accordion Rows with inline Project Zero editors
    const std::string_view FilterQuery = QueryEditTextView(ChannelSearchBuffer);
    bool ParameterEdited = false;

    for (uint32_t Grp = 0u; Grp < 4u; ++Grp)
    {
        uint32_t GroupCount = 0u;
        uint32_t DrivenCount = 0u;
        bool     ContainsOpen = false;
        for (const auto& Def : ChannelDefs)
        {
            if (Def.GroupIdx != Grp) continue;
            if (!MatchSubstringFold(Def.Title, FilterQuery) && !MatchSubstringFold(GroupHeadings[Grp], FilterQuery)) continue;
            ++GroupCount;
            if (Def.DrivenSlot) ++DrivenCount;
            if (Session->SelectedChannelOrdinal == Def.Ordinal) ContainsOpen = true;
        }
        if (GroupCount == 0u) continue;

        const bool GroupOpen = !FilterQuery.empty() || ContainsOpen || ((OpenGroupMask & (1u << Grp)) != 0u);
        const Rectangle2D GroupHeadRect { BodyX, Y, InnerW, 24.0f };
        const bool GroupHover = GroupHeadRect.Contains(Cursor);
        ConstructChevronMark(Surface, { GroupHeadRect.X + 6.0f, GroupHeadRect.Center().Y }, GroupOpen, 2.8f, Tone.TextMuted);
        WorkspaceAppearance::ConstructWeightedText(
            Surface, FontWeightClass::Medium, { GroupHeadRect.X + 18.0f, GroupHeadRect.Center().Y - 5.2f }, 10.2f,
            GroupHover ? Tone.TextPrimary : Tone.TextSecondary, GroupHeadings[Grp]);

        char CountBuf[40] {};
        if (DrivenCount > 0u)
        {
            std::snprintf(CountBuf, sizeof(CountBuf), "%u · %u driven", GroupCount, DrivenCount);
        }
        else
        {
            std::snprintf(CountBuf, sizeof(CountBuf), "%u", GroupCount);
        }
        WorkspaceAppearance::ConstructRightAlignedText(
            Surface, FontWeightClass::Regular, { GroupHeadRect.Right() - 4.0f, GroupHeadRect.Center().Y - 4.8f }, 8.8f,
            (DrivenCount > 0u) ? Tone.Amber : Tone.TextFaint, CountBuf);
        if (GroupHover && QueryPrimaryPressed(Input) && !PresetPopoverOpen)
        {
            OpenGroupMask ^= (1u << Grp);
        }
        Y += 27.0f;

        if (!GroupOpen) continue;

        for (const auto& Def : ChannelDefs)
        {
            if (Def.GroupIdx != Grp) continue;
            if (!MatchSubstringFold(Def.Title, FilterQuery) && !MatchSubstringFold(GroupHeadings[Grp], FilterQuery)) continue;

            const bool RowExpanded = (Session->SelectedChannelOrdinal == Def.Ordinal);
            float* ScalarRef  = nullptr;
            std::array<float, 4u>* ColourRef = nullptr;
            float DisplayVal  = 0.0f;

            switch (Def.Ordinal)
            {
                case 0u: ColourRef = &Params.PrimaryColorRgba; break;
                case 1u: DisplayVal = std::clamp(Recipe.PrimaryFinish.Specular.Roughness + Params.RoughnessBias, 0.02f, 1.0f); break;
                case 2u: DisplayVal = std::clamp(Recipe.PrimaryFinish.Specular.Metalness + Params.MetalnessBias, 0.0f, 1.0f); break;
                case 3u: ColourRef = &Params.SecondaryColorRgba; break;
                case 4u: ScalarRef = &Params.OpticalIor; DisplayVal = Params.OpticalIor; break;
                case 5u: ScalarRef = &Params.OpacityScale; DisplayVal = Params.OpacityScale; break;
                case 6u: ScalarRef = &Params.DetailScale; DisplayVal = Params.DetailScale; break;
                case 7u: ScalarRef = &Params.ReliefStrength; DisplayVal = Params.ReliefStrength; break;
                case 8u: ScalarRef = &Params.SeamGlowIntensity; DisplayVal = Params.SeamGlowIntensity; break;
                case 9u: ScalarRef = &Params.TransitionProgress; DisplayVal = Params.TransitionProgress; break;
                case 10u: ScalarRef = &Params.ThinFilmNanometers; DisplayVal = Params.ThinFilmNanometers; break;
                case 11u: ColourRef = &Params.AccentRadianceRgba; break;
                default: break;
            }

            char ReadoutBuf[32] {};
            AccordionRowSpec RowSpec {};
            RowSpec.Title      = Def.Title;
            RowSpec.Subtitle   = Def.Hint;
            RowSpec.DrivenWarm = Def.DrivenSlot;
            if (ColourRef != nullptr)
            {
                const ColorRgba Sw = LinearToSrgbSwatch(*ColourRef);
                std::snprintf(ReadoutBuf, sizeof(ReadoutBuf), "#%02x%02x%02x", Sw.Red, Sw.Green, Sw.Blue);
                RowSpec.SolidSwatch  = true;
                RowSpec.SwatchColour = Sw;
            }
            else
            {
                if (Def.Decimals <= 0)
                {
                    std::snprintf(ReadoutBuf, sizeof(ReadoutBuf), "%.0f %.*s", DisplayVal, static_cast<int>(Def.UnitSuffix.size()), Def.UnitSuffix.data());
                }
                else if (Def.Decimals == 1)
                {
                    std::snprintf(ReadoutBuf, sizeof(ReadoutBuf), "%.1f%.*s", DisplayVal, static_cast<int>(Def.UnitSuffix.size()), Def.UnitSuffix.data());
                }
                else
                {
                    std::snprintf(ReadoutBuf, sizeof(ReadoutBuf), "%.2f%.*s", DisplayVal, static_cast<int>(Def.UnitSuffix.size()), Def.UnitSuffix.data());
                }
                RowSpec.ScalarBarSwatch = true;
                RowSpec.ScalarFill      = (DisplayVal - Def.MinLimit) / std::max(1.0e-5f, Def.MaxLimit - Def.MinLimit);
            }
            RowSpec.Readout = ReadoutBuf;

            constexpr float HeadH    = 38.0f;
            const float     EditorH  = RowExpanded ? (Def.ColourSlot ? 168.0f : 94.0f) : 0.0f;
            const float     TotalH   = HeadH + EditorH;
            const Rectangle2D BlockRect { BodyX, Y, InnerW, TotalH };

            if (RowExpanded)
            {
                Surface.FillRectangle(BlockRect, Tone.InsetSurface, 14.0f);
                Surface.StrokeRectangle(BlockRect, Tone.OpenRowStroke, 14.0f, 1.0f);
            }

            const bool HeadClicked = ConstructAccordionRowHeader(Surface, { BodyX, Y, InnerW, HeadH }, RowSpec, RowExpanded, Input, Tone);
            if (HeadClicked && !PresetPopoverOpen)
            {
                Session->SelectedChannelOrdinal = RowExpanded ? 99u : Def.Ordinal;
            }

            if (RowExpanded)
            {
                const float EdX = BodyX + 10.0f;
                const float EdW = InnerW - 20.0f;
                float       EdY = Y + HeadH + 4.0f;

                Surface.StrokeLine({ EdX, EdY - 4.0f }, { EdX + EdW, EdY - 4.0f }, Tone.Hairline, 1.0f);

                if (ColourRef != nullptr)
                {
                    ColorRgba CurrentSwatch = LinearToSrgbSwatch(*ColourRef);
                    // Swatch preview bar + quick studio dots
                    const float DotGap = 6.0f;
                    const float DotD   = 18.0f;
                    for (std::size_t S = 0u; S < QuickStudioSwatches.size(); ++S)
                    {
                        const Rectangle2D DotRect { EdX + static_cast<float>(S) * (DotD + DotGap), EdY + 2.0f, DotD, DotD };
                        Surface.FillRectangle(DotRect, QuickStudioSwatches[S], 9.0f);
                        Surface.StrokeRectangle(DotRect, Tone.BorderStrong, 9.0f, 1.0f);
                        if (!PresetPopoverOpen && DotRect.Contains(Cursor) && QueryPrimaryPressed(Input))
                        {
                            AssignSrgbToLinearArray(QuickStudioSwatches[S], *ColourRef);
                            ParameterEdited = true;
                        }
                    }
                    EdY += 28.0f;

                    float RedVal   = static_cast<float>(CurrentSwatch.Red);
                    float GreenVal = static_cast<float>(CurrentSwatch.Green);
                    float BlueVal  = static_cast<float>(CurrentSwatch.Blue);
                    const bool ChangedR = ConstructStudioKnobSlider(Surface, { EdX, EdY, EdW, 40.0f }, 100 + static_cast<int32_t>(Def.Ordinal) * 4 + 0, ActiveSliderId, "Red", RedVal, 0.0f, 255.0f, "", 0, Input, Tone);
                    EdY += 44.0f;
                    const bool ChangedG = ConstructStudioKnobSlider(Surface, { EdX, EdY, EdW, 40.0f }, 100 + static_cast<int32_t>(Def.Ordinal) * 4 + 1, ActiveSliderId, "Green", GreenVal, 0.0f, 255.0f, "", 0, Input, Tone);
                    EdY += 44.0f;
                    const bool ChangedB = ConstructStudioKnobSlider(Surface, { EdX, EdY, EdW, 40.0f }, 100 + static_cast<int32_t>(Def.Ordinal) * 4 + 2, ActiveSliderId, "Blue", BlueVal, 0.0f, 255.0f, "", 0, Input, Tone);
                    if (ChangedR || ChangedG || ChangedB)
                    {
                        CurrentSwatch.Red   = static_cast<uint8_t>(std::clamp(std::lround(RedVal), 0l, 255l));
                        CurrentSwatch.Green = static_cast<uint8_t>(std::clamp(std::lround(GreenVal), 0l, 255l));
                        CurrentSwatch.Blue  = static_cast<uint8_t>(std::clamp(std::lround(BlueVal), 0l, 255l));
                        AssignSrgbToLinearArray(CurrentSwatch, *ColourRef);
                        ParameterEdited = true;
                    }
                }
                else
                {
                    // Driver mode segmented capsule bar
                    constexpr std::array<std::string_view, 4u> DriverTabs {{ "Value", "Clock", "Pulse", "Climate" }};
                    const int32_t CurDriver = (Params.PrimaryDriverOrdinal == 0u) ? 0
                        : ((Params.PrimaryDriverOrdinal == 4u) ? 2 : ((Params.PrimaryDriverOrdinal == 5u) ? 3 : 1));
                    const int32_t PickedDriver = ConstructSegmentedCapsuleBar(Surface, { EdX, EdY + 2.0f, EdW, 26.0f }, DriverTabs, CurDriver, Input, Tone);
                    if (PickedDriver >= 0 && !PresetPopoverOpen)
                    {
                        constexpr std::array<uint32_t, 4u> DriverLookup { 0u, 1u, 4u, 5u };
                        Params.PrimaryDriverOrdinal = DriverLookup[static_cast<std::size_t>(PickedDriver)];
                        ParameterEdited             = true;
                    }
                    EdY += 36.0f;

                    float KnobVal = DisplayVal;
                    if (ConstructStudioKnobSlider(
                            Surface,
                            { EdX, EdY, EdW, 42.0f },
                            200 + static_cast<int32_t>(Def.Ordinal),
                            ActiveSliderId,
                            Def.Title,
                            KnobVal,
                            Def.MinLimit,
                            Def.MaxLimit,
                            Def.UnitSuffix,
                            Def.Decimals,
                            Input,
                            Tone))
                    {
                        if (Def.Ordinal == 1u)
                        {
                            Params.RoughnessBias = std::clamp(KnobVal - Recipe.PrimaryFinish.Specular.Roughness, -0.5f, 0.5f);
                        }
                        else if (Def.Ordinal == 2u)
                        {
                            Params.MetalnessBias = std::clamp(KnobVal - Recipe.PrimaryFinish.Specular.Metalness, -0.5f, 0.5f);
                        }
                        else if (ScalarRef != nullptr)
                        {
                            *ScalarRef = KnobVal;
                        }
                        ParameterEdited = true;
                    }
                }
            }

            Y += TotalH + 4.0f;
        }
        Y += 6.0f;
    }

    // 2e. Collapsible bottom fold: Surface finish
    const float FoldBodyH = FinishFoldExpanded ? 156.0f : 0.0f;
    const Rectangle2D FoldRect { BodyX, Y, InnerW, 38.0f + FoldBodyH };
    Surface.FillRectangle(FoldRect, Tone.FoldSurface, 15.0f);
    Surface.StrokeRectangle(FoldRect, Tone.BorderSubtle, 15.0f, 1.0f);

    const std::string_view FinishSummary = Recipe.DoubleSidedSurface ? "Double-sided" : "Single-sided";
    if (ConstructCollapsibleFoldHeader(Surface, { FoldRect.X, FoldRect.Y, FoldRect.Width, 38.0f }, "Surface finish", FinishSummary, FinishFoldExpanded, Input, Tone) && !PresetPopoverOpen)
    {
        FinishFoldExpanded = !FinishFoldExpanded;
    }
    if (FinishFoldExpanded)
    {
        const float FX = FoldRect.X + 12.0f;
        const float FW = FoldRect.Width - 24.0f;
        float       FY = FoldRect.Y + 42.0f;

        if (ConstructStudioToggleRow(Surface, { FX, FY, FW, 32.0f }, "Double-sided", "Render front and back faces", Recipe.DoubleSidedSurface, Input, Tone))
        {
            ParameterEdited = true;
        }
        FY += 36.0f;
        if (ConstructStudioToggleRow(Surface, { FX, FY, FW, 32.0f }, "Transparent", "Alpha blend / refractive coat", Recipe.TransparentSurface, Input, Tone))
        {
            ParameterEdited = true;
        }
        FY += 36.0f;
        if (ConstructStudioToggleRow(Surface, { FX, FY, FW, 32.0f }, "Turntable playback", "Advance procedural simulation clock", Session->PlaybackRunning, Input, Tone))
        {
            ParameterEdited = true;
        }
        FY += 38.0f;

        constexpr std::array<std::string_view, 4u> Specimens {{ "Ball", "Sphere", "Cube", "Slab" }};
        const int32_t CurSpec = static_cast<int32_t>(Session->ActiveSpecimen) % 4;
        const int32_t PickedSpec = ConstructSegmentedCapsuleBar(Surface, { FX, FY, FW, 26.0f }, Specimens, CurSpec, Input, Tone);
        if (PickedSpec >= 0)
        {
            Session->ActiveSpecimen = static_cast<PreviewSpecimenGeometry>(PickedSpec);
            ParameterEdited         = true;
        }
    }
    Y += FoldRect.Height + 14.0f;

    if (ParameterEdited)
    {
        MaterialSpace::Dynamic::SynchronizeRecipeParameters(Recipe);
        Session->DirtyPreviewRequested = true;
    }

    Surface.PopClip();
    const float TotalHeight = (Y - StartY) + 6.0f;
    MaxScrollY              = std::max(0.0f, TotalHeight - BodyRect.Height);
    ScrollOffsetY           = std::clamp(ScrollOffsetY, 0.0f, MaxScrollY);

    // 3. Fixed Footer (Bake primary white capsule + Impact Pulse + Reset + status dot)
    const InspectorFooterResult FootOut = ConstructInspectorFooterBar(
        Surface,
        FooterRect,
        "Bake Maps",
        "Impact",
        "Reset",
        !Session->DirtyPreviewRequested || Session->ChannelBakeCompleted,
        Session->StatusMessage,
        Input,
        Tone);
    if (FootOut.PrimaryClicked)
    {
        Session->RequestChannelSnapshotBake();
    }
    else if (FootOut.SecondaryAClicked)
    {
        Session->TriggerViewportImpactPulse({ 0.30f, 0.28f, 0.91f });
    }
    else if (FootOut.SecondaryBClicked)
    {
        Session->SelectPresetIdentity(Recipe.PresetIdentity);
    }

    // 4. Overlay Project Zero Material Library Popover (rendered unclipped over surface)
    if (PresetPopoverOpen)
    {
        std::vector<PopoverOptionSpec> Options {};
        Options.reserve(Session->PresetRepository.size());
        int32_t SelectedIdx = 0;
        for (std::size_t Idx = 0u; Idx < Session->PresetRepository.size(); ++Idx)
        {
            const auto& Preset = Session->PresetRepository[Idx];
            if (Preset.PresetIdentity == Recipe.PresetIdentity)
            {
                SelectedIdx = static_cast<int32_t>(Idx);
            }
            Options.push_back(PopoverOptionSpec{
                Preset.DisplayTitle,
                Preset.CategoryLabel,
                LinearToSrgbSwatch(Preset.Parameters.PrimaryColorRgba),
            });
        }
        bool Dismissed = false;
        const int32_t Picked = ConstructStudioPopoverList(
            Surface, PresetAnchorRect, ShellRect, "MATERIAL LIBRARY", Options, SelectedIdx, Input, Tone, Dismissed);
        if (Picked >= 0 && static_cast<std::size_t>(Picked) < Session->PresetRepository.size())
        {
            Session->SelectPresetIdentity(Session->PresetRepository[static_cast<std::size_t>(Picked)].PresetIdentity);
            PresetPopoverOpen = false;
        }
        else if (Dismissed)
        {
            PresetPopoverOpen = false;
        }
    }
}

} // namespace Frontier::Workspaces
