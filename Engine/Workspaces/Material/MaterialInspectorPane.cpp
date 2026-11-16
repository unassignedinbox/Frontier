/*============================================================================================================================================
                                                         MATERIALINSPECTORPANE.CPP
============================================================================================================================================*/
// 🧩 Right-docked TextureStudio Material Inspector (pane-direct fill, TextureStudio OLD header & 3 KPI tiles, categorized groups & sliders)

#include "Engine/Workspaces/Material/MaterialInspectorPane.h"
#include "Engine/Workspaces/Texturing/StudioSurfaceKit.h"

#include <imgui.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

namespace Frontier::Workspaces
{

using namespace StudioSurface;

namespace
{

constexpr std::array<ColorRgba, 8u> kMaterialSwatches {{
    { 239u, 244u, 207u, 255u },
    { 229u, 118u, 78u,  255u },
    { 232u, 194u, 122u, 255u },
    { 70u,  210u, 103u, 255u },
    { 77u,  155u, 255u, 255u },
    { 133u, 222u, 255u, 255u },
    { 196u, 140u, 255u, 255u },
    { 67u,  65u,  64u,  255u },
}};

[[nodiscard]] ColorRgba LinearToSrgbSwatch(const std::array<float, 4u>& Linear) noexcept
{
    auto Convert = [](float V) noexcept -> uint8_t
    {
        const float Clamped = std::clamp(V, 0.0f, 1.0f);
        const float Gamma   = std::pow(Clamped, 1.0f / 2.2f);
        return static_cast<uint8_t>(std::clamp(std::lround(Gamma * 255.0f), 0l, 255l));
    };
    return { Convert(Linear[0]), Convert(Linear[1]), Convert(Linear[2]), 255u };
}

void AssignSrgbToLinearArray(ColorRgba Srgb, std::array<float, 4u>& OutLinear) noexcept
{
    auto Convert = [](uint8_t C) noexcept -> float
    {
        const float Norm = static_cast<float>(C) / 255.0f;
        return std::pow(Norm, 2.2f);
    };
    OutLinear[0] = Convert(Srgb.Red);
    OutLinear[1] = Convert(Srgb.Green);
    OutLinear[2] = Convert(Srgb.Blue);
    OutLinear[3] = 1.0f;
}

[[nodiscard]] std::string FormatHexLower(ColorRgba C) noexcept
{
    char Buf[12] {};
    std::snprintf(Buf, sizeof(Buf), "#%02x%02x%02x", C.Red, C.Green, C.Blue);
    return Buf;
}

} // namespace

//------------------------------------------------------------------------------------------------------------------------
//                                                    IMPLEMENTATION
//------------------------------------------------------------------------------------------------------------------------

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
    const ImVec2      Origin = ImGui::GetCursorScreenPos();
    const ImVec2      Avail  = ImGui::GetContentRegionAvail();
    const float       SafeScale = std::max(0.5f, ScaleFactor);
    const Rectangle2D ScreenBounds { Origin.x, Origin.y, std::max(1.0f, Avail.x), std::max(1.0f, Avail.y) };
    const Rectangle2D LocalBounds  { 0.0f, 0.0f, ScreenBounds.Width / SafeScale, ScreenBounds.Height / SafeScale };

    ImGui::InvisibleButton(
        "##MaterialInspectorCanvas",
        ImVec2(ScreenBounds.Width, ScreenBounds.Height),
        ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);

    PaneInputReading Input = WorkspaceAppearance::CaptureImGuiPaneInput(ScreenBounds, SafeScale);
    WorkspaceAppearance::EnforcePanePointerOwnership(this, Input);

    PixelSpace Surface(ImGui::GetWindowDrawList(), { ScreenBounds.X, ScreenBounds.Y }, SafeScale);
    Render(Surface, LocalBounds, Input, DeltaSeconds);

    WorkspaceAppearance::UpdatePanePointerOwnership(this, ScreenBounds, SafeScale, Input, ActiveSliderId >= 0);
}

void MaterialInspectorPane::Render(
    PixelSpace&             Surface,
    Rectangle2D             Bounds,
    const PaneInputReading& Input,
    double                  /*DeltaSeconds*/)
{
    if (Session == nullptr || Bounds.Width < 60.0f || Bounds.Height < 60.0f)
    {
        return;
    }

    const StudioToneSet Tone   = QueryStudioToneSet();
    const Rectangle2D   Pane   = ConstructDockedSurfaceShell(Surface, Bounds, false, Tone);
    auto&               Recipe = Session->AccessActiveRecipe();
    auto&               Params = Recipe.Parameters;
    auto&               Finish = Recipe.PrimaryFinish;

    const float PadX   = 12.0f;
    const float InnerW = std::max(80.0f, Pane.Width - PadX * 2.0f);
    const float LeftX  = Pane.X + PadX;

    // 1. TextureStudio OLD Material Inspector Header Strip ("Material Inspector" | "OPENPBR LOOK" + Reset)
    const Rectangle2D HeaderStrip { Pane.X, Pane.Y, Pane.Width, 42.0f };
    const InspectorHeadingResult HeadRes = ConstructStudioInspectorHeader(
        Surface, HeaderStrip, "Material Inspector", "OPENPBR LOOK", false, false, false, true, Input, Tone);
    if (HeadRes.ResetClicked)
    {
        Session->SelectPresetIdentity(Recipe.PresetIdentity);
    }

    // Fixed bottom footer
    constexpr float FooterH = 62.0f;
    const Rectangle2D FooterRect { Pane.X, Pane.Bottom() - FooterH, Pane.Width, FooterH };
    const Rectangle2D BodyRect   { Pane.X, HeaderStrip.Bottom(), Pane.Width, std::max(40.0f, FooterRect.Y - HeaderStrip.Bottom()) };

    const Point2D Cursor = QueryInputCursor(Input);
    if (!PresetPopoverOpen && BodyRect.Contains(Cursor) && std::fabs(Input.WheelDeltaY) > 1.0e-4f)
    {
        ScrollOffsetY = std::clamp(ScrollOffsetY - Input.WheelDeltaY * 34.0f, 0.0f, MaxScrollY);
    }

    Surface.PushClip(BodyRect);
    float Y = BodyRect.Y + 10.0f - ScrollOffsetY;

    // 2a. TextureStudio OLD Object Heading (large 20px light title + mono uppercase subtitle + Material switcher)
    char SubLine[96] {};
    std::snprintf(SubLine, sizeof(SubLine), "%s · OPENPBR 1.1 · %zu LAYERS",
        Recipe.CategoryLabel.c_str(), Recipe.LayerStack.Layers.size());
    if (ConstructStudioObjectHeading(
            Surface,
            { LeftX, Y, InnerW, 46.0f },
            Recipe.DisplayTitle,
            SubLine,
            "Material",
            PresetPopoverOpen,
            Input,
            Tone,
            &PresetAnchorRect))
    {
        PresetPopoverOpen = !PresetPopoverOpen;
    }
    Y += 56.0f;

    // 2b. TextureStudio OLD 3-Up KPI Info Tiles (Tile 1 pale lime-cream #EFF4CF: Roughness; Tile 2: Metalness; Tile 3: Layers)
    const float    RoughEff      = std::clamp(Finish.Specular.Roughness + Params.RoughnessBias, 0.02f, 1.0f);
    const float    MetalEff      = std::clamp(Finish.Specular.Metalness + Params.MetalnessBias, 0.0f, 1.0f);
    const uint32_t VisibleLayers = Recipe.LayerStack.CountVisibleLayers();
    const uint32_t TotalLayers   = static_cast<uint32_t>(Recipe.LayerStack.Layers.size());

    char RoughBuf[24] {}, MetalBuf[24] {}, LayerBuf[24] {};
    std::snprintf(RoughBuf, sizeof(RoughBuf), "%.2f", RoughEff);
    std::snprintf(MetalBuf, sizeof(MetalBuf), "%d%%", static_cast<int>(std::lround(MetalEff * 100.0f)));
    std::snprintf(LayerBuf, sizeof(LayerBuf), "%u/%u", VisibleLayers, TotalLayers);

    const std::array<KpiTileSpec, 3u> KpiTiles {{
        { "Roughness", RoughBuf, "Microfacet", true },
        { "Metalness", MetalBuf, MetalEff > 0.5f ? "Conductor" : "Dielectric", false },
        { "Layers",    LayerBuf, "12 ch", false },
    }};
    ConstructStudioKpiTrio(Surface, { LeftX, Y, InnerW, 64.0f }, KpiTiles, Tone);
    Y += 74.0f;

    const bool Interactive  = !PresetPopoverOpen;
    constexpr float HeadH   = 38.0f;
    const float     BodyPad = 12.0f;
    const float     CtrlW   = InnerW - BodyPad * 2.0f;

    // 2c. Category Group 1: Surface & Albedo (SURFACE — white icon on blue tile + TextureStudio sliders)
    {
        const float SurfBodyH = SurfaceGroupOpen ? (10.0f + 34.0f + 52.0f * 5.0f + 10.0f) : 0.0f;
        const float BoxH     = HeadH + SurfBodyH;

        if (ConstructCategoryGroupBox(
                Surface,
                { LeftX, Y, InnerW, BoxH },
                HeadH,
                ControlIcon::MaterialSphere,
                IconAccentTone::Blue,
                "Surface & Albedo",
                "SURFACE",
                SurfaceGroupOpen,
                Input,
                Tone) && Interactive)
        {
            SurfaceGroupOpen = !SurfaceGroupOpen;
        }

        if (SurfaceGroupOpen)
        {
            float CY = Y + HeadH + 10.0f;
            const float CX = LeftX + BodyPad;

            ColorRgba Albedo = LinearToSrgbSwatch(Params.PrimaryColorRgba);
            const std::string HexTxt = FormatHexLower(Albedo);

            // Albedo colour swatch strip + quick dots
            const Rectangle2D SwatchRow { CX, CY, CtrlW, 26.0f };
            Surface.FillRectangle(SwatchRow, Tone.FieldWell, 13.0f);
            Surface.StrokeRectangle(SwatchRow, Tone.BorderSubtle, 13.0f, 1.0f);
            Surface.FillCircle({ SwatchRow.X + 14.0f, SwatchRow.Center().Y }, 7.5f, Albedo);
            Surface.StrokeCircle({ SwatchRow.X + 14.0f, SwatchRow.Center().Y }, 7.5f, Tone.BorderStrong, 1.0f);

            const float DotStartX = SwatchRow.X + 30.0f;
            const float DotStep   = std::min(19.0f, (CtrlW - 96.0f) / static_cast<float>(kMaterialSwatches.size()));
            for (std::size_t S = 0u; S < kMaterialSwatches.size(); ++S)
            {
                const Point2D DotC { DotStartX + static_cast<float>(S) * DotStep + 6.0f, SwatchRow.Center().Y };
                Surface.FillCircle(DotC, 5.8f, kMaterialSwatches[S]);
                if (Rectangle2D{ DotC.X - 7.0f, DotC.Y - 7.0f, 14.0f, 14.0f }.Contains(Cursor) && QueryPrimaryPressed(Input) && Interactive)
                {
                    AssignSrgbToLinearArray(kMaterialSwatches[S], Params.PrimaryColorRgba);
                    Albedo = kMaterialSwatches[S];
                    Session->DirtyPreviewRequested = true;
                }
            }
            WorkspaceAppearance::ConstructRightAlignedText(
                Surface, FontWeightClass::Medium, { SwatchRow.Right() - 10.0f, SwatchRow.Center().Y - 5.0f }, 9.4f, Tone.TextMuted, HexTxt);
            CY += 34.0f;

            float R = static_cast<float>(Albedo.Red);
            float G = static_cast<float>(Albedo.Green);
            float B = static_cast<float>(Albedo.Blue);
            if (ConstructStudioKnobSlider(Surface, { CX, CY, CtrlW, 46.0f }, 10, ActiveSliderId, "Albedo Red", R, 0.0f, 255.0f, "", 0, Input, Tone) && Interactive)
            {
                Albedo.Red = static_cast<uint8_t>(std::clamp(R, 0.0f, 255.0f));
                AssignSrgbToLinearArray(Albedo, Params.PrimaryColorRgba);
                Session->DirtyPreviewRequested = true;
            }
            CY += 52.0f;
            if (ConstructStudioKnobSlider(Surface, { CX, CY, CtrlW, 46.0f }, 11, ActiveSliderId, "Albedo Green", G, 0.0f, 255.0f, "", 0, Input, Tone) && Interactive)
            {
                Albedo.Green = static_cast<uint8_t>(std::clamp(G, 0.0f, 255.0f));
                AssignSrgbToLinearArray(Albedo, Params.PrimaryColorRgba);
                Session->DirtyPreviewRequested = true;
            }
            CY += 52.0f;
            if (ConstructStudioKnobSlider(Surface, { CX, CY, CtrlW, 46.0f }, 12, ActiveSliderId, "Albedo Blue", B, 0.0f, 255.0f, "", 0, Input, Tone) && Interactive)
            {
                Albedo.Blue = static_cast<uint8_t>(std::clamp(B, 0.0f, 255.0f));
                AssignSrgbToLinearArray(Albedo, Params.PrimaryColorRgba);
                Session->DirtyPreviewRequested = true;
            }
            CY += 52.0f;

            float NextRough = RoughEff;
            if (ConstructStudioKnobSlider(Surface, { CX, CY, CtrlW, 46.0f }, 13, ActiveSliderId, "Roughness", NextRough, 0.02f, 1.0f, "", 2, Input, Tone) && Interactive)
            {
                Params.RoughnessBias           = std::clamp(NextRough - Finish.Specular.Roughness, -0.5f, 0.5f);
                Session->DirtyPreviewRequested = true;
            }
            CY += 52.0f;

            float NextMetal = MetalEff;
            if (ConstructStudioKnobSlider(Surface, { CX, CY, CtrlW, 46.0f }, 14, ActiveSliderId, "Metalness", NextMetal, 0.0f, 1.0f, "", 2, Input, Tone) && Interactive)
            {
                Params.MetalnessBias           = std::clamp(NextMetal - Finish.Specular.Metalness, -0.5f, 0.5f);
                Session->DirtyPreviewRequested = true;
            }
        }
        Y += BoxH + 10.0f;
    }

    // 2d. Category Group 2: Specular & Optical (OPTICAL — white icon on violet tile)
    {
        const float OptBodyH = OpticalGroupOpen ? (10.0f + 52.0f * 4.0f + 10.0f) : 0.0f;
        const float BoxH    = HeadH + OptBodyH;

        if (ConstructCategoryGroupBox(
                Surface,
                { LeftX, Y, InnerW, BoxH },
                HeadH,
                ControlIcon::SparklesAntiAliasing,
                IconAccentTone::Violet,
                "Specular & Optical",
                "OPTICAL",
                OpticalGroupOpen,
                Input,
                Tone) && Interactive)
        {
            OpticalGroupOpen = !OpticalGroupOpen;
        }

        if (OpticalGroupOpen)
        {
            float CY = Y + HeadH + 10.0f;
            const float CX = LeftX + BodyPad;

            if (ConstructStudioKnobSlider(Surface, { CX, CY, CtrlW, 46.0f }, 20, ActiveSliderId, "Specular IOR", Params.OpticalIor, 1.0f, 2.4f, "ior", 2, Input, Tone) && Interactive)
            {
                Session->DirtyPreviewRequested = true;
            }
            CY += 52.0f;
            if (ConstructStudioKnobSlider(Surface, { CX, CY, CtrlW, 46.0f }, 21, ActiveSliderId, "Clearcoat Weight", Finish.Clearcoat.CoatWeight, 0.0f, 1.0f, "", 2, Input, Tone) && Interactive)
            {
                Session->DirtyPreviewRequested = true;
            }
            CY += 52.0f;
            if (ConstructStudioKnobSlider(Surface, { CX, CY, CtrlW, 46.0f }, 22, ActiveSliderId, "Clearcoat Roughness", Finish.Clearcoat.CoatRoughness, 0.01f, 1.0f, "", 2, Input, Tone) && Interactive)
            {
                Session->DirtyPreviewRequested = true;
            }
            CY += 52.0f;
            if (ConstructStudioKnobSlider(Surface, { CX, CY, CtrlW, 46.0f }, 23, ActiveSliderId, "Thin-Film Thickness", Params.ThinFilmNanometers, 120.0f, 900.0f, "nm", 0, Input, Tone) && Interactive)
            {
                Session->DirtyPreviewRequested = true;
            }
        }
        Y += BoxH + 10.0f;
    }

    // 2e. Category Group 3: Dynamic & Relief (DYNAMIC — white icon on amber tile)
    {
        const float DynBodyH = DynamicGroupOpen ? (10.0f + 52.0f * 4.0f + 10.0f) : 0.0f;
        const float BoxH    = HeadH + DynBodyH;

        if (ConstructCategoryGroupBox(
                Surface,
                { LeftX, Y, InnerW, BoxH },
                HeadH,
                ControlIcon::NoiseWave,
                IconAccentTone::Amber,
                "Dynamic & Relief",
                "DYNAMIC",
                DynamicGroupOpen,
                Input,
                Tone) && Interactive)
        {
            DynamicGroupOpen = !DynamicGroupOpen;
        }

        if (DynamicGroupOpen)
        {
            float CY = Y + HeadH + 10.0f;
            const float CX = LeftX + BodyPad;

            if (ConstructStudioKnobSlider(Surface, { CX, CY, CtrlW, 46.0f }, 30, ActiveSliderId, "Detail Scale", Params.DetailScale, 0.5f, 12.0f, "x", 1, Input, Tone) && Interactive)
            {
                Session->DirtyPreviewRequested = true;
            }
            CY += 52.0f;
            if (ConstructStudioKnobSlider(Surface, { CX, CY, CtrlW, 46.0f }, 31, ActiveSliderId, "Relief Strength", Params.ReliefStrength, 0.0f, 2.0f, "", 2, Input, Tone) && Interactive)
            {
                Session->DirtyPreviewRequested = true;
            }
            CY += 52.0f;
            if (ConstructStudioKnobSlider(Surface, { CX, CY, CtrlW, 46.0f }, 32, ActiveSliderId, "Seam Radiance", Params.SeamGlowIntensity, 0.0f, 4.0f, "x", 2, Input, Tone) && Interactive)
            {
                Session->DirtyPreviewRequested = true;
            }
            CY += 52.0f;
            if (ConstructStudioKnobSlider(Surface, { CX, CY, CtrlW, 46.0f }, 33, ActiveSliderId, "Transition Sweep", Params.TransitionProgress, 0.0f, 1.0f, "", 2, Input, Tone) && Interactive)
            {
                Session->DirtyPreviewRequested = true;
            }
        }
        Y += BoxH + 10.0f;
    }

    // 2f. Category Group 4: Shading Options (OPTIONS — white icon on teal tile)
    {
        const float OptsBodyH = OptionsGroupOpen ? (10.0f + 34.0f * 3.0f + 10.0f) : 0.0f;
        const float BoxH     = HeadH + OptsBodyH;

        if (ConstructCategoryGroupBox(
                Surface,
                { LeftX, Y, InnerW, BoxH },
                HeadH,
                ControlIcon::SlidersQuality,
                IconAccentTone::Teal,
                "Shading Options",
                "OPENPBR",
                OptionsGroupOpen,
                Input,
                Tone) && Interactive)
        {
            OptionsGroupOpen = !OptionsGroupOpen;
        }

        if (OptionsGroupOpen)
        {
            float CY = Y + HeadH + 10.0f;
            const float CX = LeftX + BodyPad;

            if (ConstructStudioToggleRow(Surface, { CX, CY, CtrlW, 28.0f }, "Transparent Surface", "", Recipe.TransparentSurface, Input, Tone) && Interactive)
            {
                Session->DirtyPreviewRequested = true;
            }
            CY += 34.0f;
            if (ConstructStudioToggleRow(Surface, { CX, CY, CtrlW, 28.0f }, "Double-Sided Shading", "", Recipe.DoubleSidedSurface, Input, Tone) && Interactive)
            {
                Session->DirtyPreviewRequested = true;
            }
            CY += 34.0f;
            (void)ConstructStudioToggleRow(Surface, { CX, CY, CtrlW, 28.0f }, "Turntable Clock", "", Session->PlaybackRunning, Input, Tone);
        }
        Y += BoxH + 10.0f;
    }

    const float TotalHeight = (Y + ScrollOffsetY) - (BodyRect.Y + 10.0f) + 12.0f;
    MaxScrollY              = std::max(0.0f, TotalHeight - BodyRect.Height);
    ScrollOffsetY           = std::clamp(ScrollOffsetY, 0.0f, MaxScrollY);
    Surface.PopClip();

    // 3. Edge-to-Edge Inspector Footer
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
        Session->TriggerViewportImpactPulse({ 0.45f, 0.35f, 0.82f });
    }
    else if (FootOut.SecondaryBClicked)
    {
        Session->SelectPresetIdentity(Recipe.PresetIdentity);
    }

    // 4. Overlay Material Preset Library Popover
    if (PresetPopoverOpen)
    {
        std::vector<PopoverOptionSpec> Options;
        Options.reserve(Session->PresetRepository.size());
        int32_t ActiveIdx = -1;
        for (std::size_t P = 0u; P < Session->PresetRepository.size(); ++P)
        {
            const auto& Preset = Session->PresetRepository[P];
            if (Preset.RecipeId == Recipe.RecipeId)
            {
                ActiveIdx = static_cast<int32_t>(P);
            }
            Options.push_back({ Preset.DisplayTitle, Preset.CategoryLabel, LinearToSrgbSwatch(Preset.Parameters.PrimaryColorRgba) });
        }
        bool Dismissed = false;
        const int32_t Picked = ConstructStudioPopoverList(
            Surface, PresetAnchorRect, Pane, "MATERIAL PRESETS", Options, ActiveIdx, Input, Tone, Dismissed);
        if (Picked >= 0 && static_cast<std::size_t>(Picked) < Session->PresetRepository.size())
        {
            Session->SelectPresetIdentifier(Session->PresetRepository[static_cast<std::size_t>(Picked)].RecipeId);
            PresetPopoverOpen = false;
        }
        else if (Dismissed)
        {
            PresetPopoverOpen = false;
        }
    }
}

} // namespace Frontier::Workspaces
