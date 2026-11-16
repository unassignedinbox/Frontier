/*============================================================================================================================================
                                                        PATTERNSTAMPINSPECTORPANE.CPP
============================================================================================================================================*/
// 🧩 Right-docked TextureStudio Inspector for Pattern Studio & Stamp Studio (pane-direct fill, TextureStudio header & KPI tiles, categorized groups & sliders)

#include "Engine/Workspaces/Texturing/PatternStampInspectorPane.h"
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

constexpr std::array<ColorRgba, 8u> kQuickSwatches {{
    { 239u, 244u, 207u, 255u },
    { 232u, 194u, 122u, 255u },
    { 229u, 118u, 78u,  255u },
    { 70u,  210u, 103u, 255u },
    { 77u,  155u, 255u, 255u },
    { 196u, 140u, 255u, 255u },
    { 138u, 142u, 152u, 255u },
    { 28u,  30u,  36u,  255u },
}};

[[nodiscard]] std::string FormatHexColor(ColorRgba C) noexcept
{
    char Buf[12] {};
    std::snprintf(Buf, sizeof(Buf), "#%02x%02x%02x", C.Red, C.Green, C.Blue);
    return Buf;
}

[[nodiscard]] const char* QueryMotifName(PatternStampMotifShape Shape) noexcept
{
    const auto& Labels = PatternStampWorkspaceSession::InspectMotifShapeLabels();
    const std::size_t Idx = static_cast<std::size_t>(Shape);
    return (Idx < Labels.size()) ? Labels[Idx] : "Motif";
}

[[nodiscard]] const char* QueryFinishName(PatternStampFinishPreset Preset) noexcept
{
    const auto& Labels = PatternStampWorkspaceSession::InspectFinishLabels();
    const std::size_t Idx = static_cast<std::size_t>(Preset);
    return (Idx < Labels.size()) ? Labels[Idx] : "Custom";
}

[[nodiscard]] const char* QueryModifierTitle(PatternStampModifierCategory Cat) noexcept
{
    const auto& Labels = PatternStampWorkspaceSession::InspectModifierLabels();
    const std::size_t Idx = static_cast<std::size_t>(Cat);
    return (Idx < Labels.size()) ? Labels[Idx] : "Modifier";
}

[[nodiscard]] IconAccentTone QueryModifierAccentTone(PatternStampModifierCategory Cat) noexcept
{
    switch (Cat)
    {
        case PatternStampModifierCategory::GridArray:     return IconAccentTone::Blue;
        case PatternStampModifierCategory::NoiseScale:    return IconAccentTone::Amber;
        case PatternStampModifierCategory::RadialArray:   return IconAccentTone::Teal;
        case PatternStampModifierCategory::MirrorSymmetry:return IconAccentTone::Violet;
        case PatternStampModifierCategory::InsetEcho:     return IconAccentTone::Blue;
        case PatternStampModifierCategory::ScatterDropout: return IconAccentTone::Green;
        default:                                          return IconAccentTone::Violet;
    }
}

[[nodiscard]] ControlIcon QueryModifierVectorIcon(PatternStampModifierCategory Cat) noexcept
{
    switch (Cat)
    {
        case PatternStampModifierCategory::GridArray:     return ControlIcon::LayoutGrid;
        case PatternStampModifierCategory::NoiseScale:    return ControlIcon::VectorCurve;
        case PatternStampModifierCategory::RadialArray:   return ControlIcon::SparklesAntiAliasing;
        case PatternStampModifierCategory::MirrorSymmetry:return ControlIcon::LayoutSplit;
        case PatternStampModifierCategory::InsetEcho:     return ControlIcon::ShapeCircle;
        case PatternStampModifierCategory::ScatterDropout: return ControlIcon::SparklesAntiAliasing;
        default:                                          return ControlIcon::NoiseWave;
    }
}

[[nodiscard]] bool ConstructCapsuleDropdownRow(
    PixelSpace&             Surface,
    Rectangle2D             RowRect,
    std::string_view        Label,
    std::string_view        CurrentText,
    bool                    Open,
    const PaneInputReading& Input,
    const StudioToneSet&    Tone,
    Rectangle2D*            OutControlRect = nullptr) noexcept
{
    WorkspaceAppearance::ConstructWeightedText(
        Surface, FontWeightClass::Light, { RowRect.X, RowRect.Y + 1.0f }, 10.4f, Tone.TextSecondary, Label);

    const Rectangle2D ControlRect { RowRect.X, RowRect.Y + 18.0f, RowRect.Width, 28.0f };
    if (OutControlRect != nullptr)
    {
        *OutControlRect = ControlRect;
    }
    const Point2D Cursor  = QueryInputCursor(Input);
    const bool    Hovered = ControlRect.Contains(Cursor);

    Surface.FillRectangle(ControlRect, Hovered ? Tone.RowHover : Tone.FieldWell, 14.0f);
    Surface.StrokeRectangle(ControlRect, Open ? Tone.HighlightFill : (Hovered ? Tone.BorderStrong : Tone.BorderSubtle), 14.0f, 1.0f);
    WorkspaceAppearance::ConstructWeightedText(
        Surface, FontWeightClass::Regular, { ControlRect.X + 12.0f, ControlRect.Center().Y - 5.5f }, 10.6f, Tone.TextPrimary, CurrentText);
    Surface.StrokeLine({ ControlRect.Right() - 28.0f, ControlRect.Y + 4.0f }, { ControlRect.Right() - 28.0f, ControlRect.Bottom() - 4.0f }, Tone.Hairline, 1.0f);
    ConstructChevronMark(Surface, { ControlRect.Right() - 14.0f, ControlRect.Center().Y }, !Open, 3.0f, Tone.TextMuted);
    return Hovered && QueryPrimaryPressed(Input);
}

} // namespace

//------------------------------------------------------------------------------------------------------------------------
//                                                    IMPLEMENTATION
//------------------------------------------------------------------------------------------------------------------------

PatternStampInspectorPane::~PatternStampInspectorPane() noexcept
{
    WorkspaceAppearance::ReleasePanePointerOwnership(this);
}

void PatternStampInspectorPane::AttachSession(PatternStampWorkspaceSession* TargetSession) noexcept
{
    Session = TargetSession;
}

void PatternStampInspectorPane::PresentInImGuiPane(double DeltaSeconds, float ScaleFactor)
{
    const ImVec2      Origin = ImGui::GetCursorScreenPos();
    const ImVec2      Avail  = ImGui::GetContentRegionAvail();
    const float       SafeScale = std::max(0.5f, ScaleFactor);
    const Rectangle2D ScreenBounds { Origin.x, Origin.y, std::max(1.0f, Avail.x), std::max(1.0f, Avail.y) };
    const Rectangle2D LocalBounds  { 0.0f, 0.0f, ScreenBounds.Width / SafeScale, ScreenBounds.Height / SafeScale };

    ImGui::InvisibleButton(
        "##PatternStampInspectorCanvas",
        ImVec2(ScreenBounds.Width, ScreenBounds.Height),
        ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);

    PaneInputReading Input = WorkspaceAppearance::CaptureImGuiPaneInput(ScreenBounds, SafeScale);
    WorkspaceAppearance::EnforcePanePointerOwnership(this, Input);

    PixelSpace Surface(ImGui::GetWindowDrawList(), { ScreenBounds.X, ScreenBounds.Y }, SafeScale);
    Render(Surface, LocalBounds, Input, DeltaSeconds);

    WorkspaceAppearance::UpdatePanePointerOwnership(this, ScreenBounds, SafeScale, Input, ActiveSliderId >= 0);
}

void PatternStampInspectorPane::Render(
    PixelSpace&             Surface,
    Rectangle2D             Bounds,
    const PaneInputReading& Input,
    double                  /*DeltaSeconds*/)
{
    if (Session == nullptr || Bounds.Width < 60.0f || Bounds.Height < 60.0f)
    {
        return;
    }

    const StudioToneSet Tone        = QueryStudioToneSet();
    const Rectangle2D   Pane        = ConstructDockedSurfaceShell(Surface, Bounds, false, Tone);
    const bool          PatternMode = (Session->Mode == PatternStampWorkspaceMode::Pattern);
    auto&               Doc         = Session->Document;
    PatternStampLayerSpec* Layer    = Session->InspectSelectedLayer();

    const float PadX   = 12.0f;
    const float InnerW = std::max(80.0f, Pane.Width - PadX * 2.0f);
    const float LeftX  = Pane.X + PadX;

    // 1. TextureStudio OLD Inspector Top Strip (NO amber square icon)
    const Rectangle2D HeaderStrip { Pane.X, Pane.Y, Pane.Width, 42.0f };
    const InspectorHeadingResult HeadRes = ConstructStudioInspectorHeader(
        Surface,
        HeaderStrip,
        PatternMode ? "Layer & Modifiers" : "Stamp & Modifiers",
        PatternMode ? "VECTOR · PBR" : "DECAL · PBR",
        true,
        Session->QueryCanUndo(),
        Session->QueryCanRedo(),
        true,
        Input,
        Tone);
    if (HeadRes.UndoClicked)
    {
        Session->ApplyUndo();
    }
    else if (HeadRes.RedoClicked)
    {
        Session->ApplyRedo();
    }
    else if (HeadRes.ResetClicked)
    {
        Session->LoadPresetByIndex(Session->QueryActivePresetIndex());
    }

    // Reserve fixed bottom footer bar
    constexpr float FooterH = 62.0f;
    const Rectangle2D FooterRect { Pane.X, Pane.Bottom() - FooterH, Pane.Width, FooterH };
    const Rectangle2D BodyRect   { Pane.X, HeaderStrip.Bottom(), Pane.Width, std::max(40.0f, FooterRect.Y - HeaderStrip.Bottom()) };

    const Point2D Cursor = QueryInputCursor(Input);
    if (OpenPopover == ActivePopoverKind::None && BodyRect.Contains(Cursor) && std::fabs(Input.WheelDeltaY) > 1.0e-4f)
    {
        ScrollOffsetY = std::clamp(ScrollOffsetY - Input.WheelDeltaY * 34.0f, 0.0f, MaxScrollY);
    }

    Surface.PushClip(BodyRect);
    float Y = BodyRect.Y + 10.0f - ScrollOffsetY;

    // 2a. TextureStudio OLD Object Heading (large 20px light title + mono uppercase subtitle + Starter switcher)
    char ActiveLayerSub[128] {};
    if (Layer != nullptr)
    {
        std::snprintf(ActiveLayerSub, sizeof(ActiveLayerSub), "ACTIVE LAYER: %s", Layer->Name.c_str());
    }
    else
    {
        std::snprintf(ActiveLayerSub, sizeof(ActiveLayerSub), "%u PX · %zu LAYERS", Session->ResolutionPx, Doc.Layers.size());
    }
    Rectangle2D StarterBtnRect {};
    if (ConstructStudioObjectHeading(
            Surface,
            { LeftX, Y, InnerW, 46.0f },
            Doc.DocumentName,
            ActiveLayerSub,
            "Starter",
            OpenPopover == ActivePopoverKind::StarterLibrary,
            Input,
            Tone,
            &StarterBtnRect))
    {
        OpenPopover   = (OpenPopover == ActivePopoverKind::StarterLibrary) ? ActivePopoverKind::None : ActivePopoverKind::StarterLibrary;
        PopoverAnchor = StarterBtnRect;
    }
    Y += 56.0f;

    // 2b. TextureStudio OLD 3-Up KPI Info Tiles (Tile 1 pale lime-cream #EFF4CF highlight, Tiles 2 & 3 dark — zero meter bars)
    char InstBuf[24] {};
    char SpanBuf[32] {};
    char SurfBuf[24] {};
    char LayerSubBuf[32] {};
    std::snprintf(InstBuf, sizeof(InstBuf), "%d", Session->InstancesEvaluated);
    std::snprintf(SpanBuf, sizeof(SpanBuf), "%.2f-%.2f", Session->MinScaleReading, Session->MaxScaleReading);
    std::snprintf(SurfBuf, sizeof(SurfBuf), "%d%%", Session->SurfaceFractionPct);
    std::snprintf(LayerSubBuf, sizeof(LayerSubBuf), "%zu layers", Doc.Layers.size());

    const std::array<KpiTileSpec, 3u> KpiTiles {{
        { "Instances",  InstBuf, PatternMode ? "Evaluated" : "Decal", true },
        { "Scale Span", SpanBuf, Session->CountEnabledModifiers() > 0u ? "Noise Mod" : "Uniform", false },
        { "Surface",    SurfBuf, LayerSubBuf, false },
    }};
    ConstructStudioKpiTrio(Surface, { LeftX, Y, InnerW, 64.0f }, KpiTiles, Tone);
    Y += 74.0f;

    const bool Interactive = (OpenPopover == ActivePopoverKind::None);

    // 2c. Category Group 1: Layer Properties (MOTIF — distinct header with white icon on blue tile + categorized body with TextureStudio sliders)
    if (Layer != nullptr)
    {
        constexpr float GroupHeadH = 38.0f;
        const float BodyPad       = 12.0f;
        const float CtrlW         = InnerW - BodyPad * 2.0f;
        const bool  ShowMotif     = (Layer->Category == PatternStampLayerCategory::Motif);
        const bool  ShowRgb       = Layer->FillEnabled;

        // Compute exact body height for crisp outer section outline
        float LayerBodyH = 0.0f;
        if (LayerGroupOpen)
        {
            LayerBodyH += 10.0f;                                // top pad
            if (ShowMotif) LayerBodyH += 52.0f;                 // Motif Shape dropdown
            LayerBodyH += 34.0f;                                // Fill Enabled switch
            LayerBodyH += 50.0f;                                // Fill Source segmented bar
            LayerBodyH += 34.0f;                                // Fill Swatch + quick colour dots
            if (ShowRgb) LayerBodyH += 52.0f * 3.0f;            // R / G / B TextureStudio sliders
            LayerBodyH += 52.0f;                                // Stroke Outline slider
            LayerBodyH += 50.0f;                                // Blend Mode bar
            LayerBodyH += 52.0f;                                // Layer Opacity slider
            LayerBodyH += 52.0f * 5.0f;                         // Center X, Center Y, Width, Height, Rotation sliders
            LayerBodyH += 10.0f;                                // bottom pad
        }
        const float BoxH = GroupHeadH + LayerBodyH;
        const Rectangle2D GroupRect { LeftX, Y, InnerW, BoxH };

        if (ConstructCategoryGroupBox(
                Surface,
                GroupRect,
                GroupHeadH,
                ControlIcon::ShapeHexagon,
                IconAccentTone::Blue,
                "Layer Properties",
                ShowMotif ? "MOTIF" : "LAYER",
                LayerGroupOpen,
                Input,
                Tone) && Interactive)
        {
            LayerGroupOpen = !LayerGroupOpen;
        }

        if (LayerGroupOpen)
        {
            float CY = Y + GroupHeadH + 10.0f;
            const float CX = LeftX + BodyPad;

            if (ShowMotif)
            {
                Rectangle2D ShapeDropRect {};
                if (ConstructCapsuleDropdownRow(
                        Surface,
                        { CX, CY, CtrlW, 46.0f },
                        "Motif Shape",
                        QueryMotifName(Layer->MotifShape),
                        OpenPopover == ActivePopoverKind::MotifShapeList,
                        Input,
                        Tone,
                        &ShapeDropRect) && Interactive)
                {
                    OpenPopover   = ActivePopoverKind::MotifShapeList;
                    PopoverAnchor = ShapeDropRect;
                }
                CY += 52.0f;
            }

            if (ConstructStudioToggleRow(Surface, { CX, CY, CtrlW, 28.0f }, "Fill Enabled", "", Layer->FillEnabled, Input, Tone) && Interactive)
            {
                Session->BumpRevision();
            }
            CY += 34.0f;

            // Fill Source segmented capsule bar
            WorkspaceAppearance::ConstructWeightedText(Surface, FontWeightClass::Light, { CX, CY }, 10.4f, Tone.TextSecondary, "Fill Source");
            constexpr std::array<std::string_view, 4u> FillLabels {{ "Solid", "Linear", "Radial", "Point" }};
            const int32_t PickedFill = ConstructSegmentedCapsuleBar(
                Surface, { CX, CY + 17.0f, CtrlW, 27.0f }, FillLabels, static_cast<int32_t>(Layer->FillSource), Input, Tone);
            if (PickedFill >= 0 && Interactive)
            {
                Session->PushUndoSnapshot();
                Layer->FillSource = static_cast<PatternStampFillSourceMode>(PickedFill);
                Session->BumpRevision();
            }
            CY += 50.0f;

            // Fill Colour bar + quick colour dots
            const std::string HexStr = FormatHexColor(Layer->FillTint);
            const Rectangle2D SwatchRow { CX, CY, CtrlW, 26.0f };
            Surface.FillRectangle(SwatchRow, Tone.FieldWell, 13.0f);
            Surface.StrokeRectangle(SwatchRow, Tone.BorderSubtle, 13.0f, 1.0f);
            Surface.FillCircle({ SwatchRow.X + 14.0f, SwatchRow.Center().Y }, 7.5f, Layer->FillTint);
            Surface.StrokeCircle({ SwatchRow.X + 14.0f, SwatchRow.Center().Y }, 7.5f, Tone.BorderStrong, 1.0f);

            const float DotStartX = SwatchRow.X + 30.0f;
            const float DotStep   = std::min(19.0f, (CtrlW - 96.0f) / static_cast<float>(kQuickSwatches.size()));
            for (std::size_t S = 0u; S < kQuickSwatches.size(); ++S)
            {
                const Point2D DotC { DotStartX + static_cast<float>(S) * DotStep + 6.0f, SwatchRow.Center().Y };
                Surface.FillCircle(DotC, 5.8f, kQuickSwatches[S]);
                const bool DotHit = Rectangle2D{ DotC.X - 7.0f, DotC.Y - 7.0f, 14.0f, 14.0f }.Contains(Cursor);
                if (DotHit && QueryPrimaryPressed(Input) && Interactive)
                {
                    Session->PushUndoSnapshot();
                    Layer->FillTint = kQuickSwatches[S];
                    Session->BumpRevision();
                }
            }
            WorkspaceAppearance::ConstructRightAlignedText(
                Surface, FontWeightClass::Medium, { SwatchRow.Right() - 10.0f, SwatchRow.Center().Y - 5.0f }, 9.4f, Tone.TextMuted, HexStr);
            CY += 34.0f;

            if (ShowRgb)
            {
                float R = static_cast<float>(Layer->FillTint.Red);
                float G = static_cast<float>(Layer->FillTint.Green);
                float B = static_cast<float>(Layer->FillTint.Blue);
                if (ConstructStudioKnobSlider(Surface, { CX, CY, CtrlW, 46.0f }, 101, ActiveSliderId, "Fill Red", R, 0.0f, 255.0f, "", 0, Input, Tone) && Interactive)
                {
                    Layer->FillTint.Red = static_cast<uint8_t>(std::clamp(R, 0.0f, 255.0f));
                    Session->BumpRevision();
                }
                CY += 52.0f;
                if (ConstructStudioKnobSlider(Surface, { CX, CY, CtrlW, 46.0f }, 102, ActiveSliderId, "Fill Green", G, 0.0f, 255.0f, "", 0, Input, Tone) && Interactive)
                {
                    Layer->FillTint.Green = static_cast<uint8_t>(std::clamp(G, 0.0f, 255.0f));
                    Session->BumpRevision();
                }
                CY += 52.0f;
                if (ConstructStudioKnobSlider(Surface, { CX, CY, CtrlW, 46.0f }, 103, ActiveSliderId, "Fill Blue", B, 0.0f, 255.0f, "", 0, Input, Tone) && Interactive)
                {
                    Layer->FillTint.Blue = static_cast<uint8_t>(std::clamp(B, 0.0f, 255.0f));
                    Session->BumpRevision();
                }
                CY += 52.0f;
            }

            if (ConstructStudioKnobSlider(Surface, { CX, CY, CtrlW, 46.0f }, 104, ActiveSliderId, "Stroke Outline", Layer->StrokeWidthPx, 0.0f, 24.0f, "px", 1, Input, Tone) && Interactive)
            {
                Session->BumpRevision();
            }
            CY += 52.0f;

            // Blend Mode segmented bar
            WorkspaceAppearance::ConstructWeightedText(Surface, FontWeightClass::Light, { CX, CY }, 10.4f, Tone.TextSecondary, "Blend Mode");
            constexpr std::array<std::string_view, 4u> BlendLabels {{ "Normal", "Multiply", "Screen", "Overlay" }};
            const int32_t PickedBlend = ConstructSegmentedCapsuleBar(
                Surface, { CX, CY + 17.0f, CtrlW, 27.0f }, BlendLabels, std::min<int32_t>(3, static_cast<int32_t>(Layer->BlendMode)), Input, Tone);
            if (PickedBlend >= 0 && Interactive)
            {
                Session->PushUndoSnapshot();
                Layer->BlendMode = static_cast<uint8_t>(PickedBlend);
                Session->BumpRevision();
            }
            CY += 50.0f;

            if (ConstructStudioKnobSlider(Surface, { CX, CY, CtrlW, 46.0f }, 105, ActiveSliderId, "Layer Opacity", Layer->Opacity, 0.0f, 1.0f, "", 2, Input, Tone) && Interactive)
            {
                Session->BumpRevision();
            }
            CY += 52.0f;

            if (ConstructStudioKnobSlider(Surface, { CX, CY, CtrlW, 46.0f }, 106, ActiveSliderId, "Center X", Layer->CenterX, 0.0f, 512.0f, "px", 0, Input, Tone) && Interactive)
            {
                Session->BumpRevision();
            }
            CY += 52.0f;
            if (ConstructStudioKnobSlider(Surface, { CX, CY, CtrlW, 46.0f }, 107, ActiveSliderId, "Center Y", Layer->CenterY, 0.0f, 512.0f, "px", 0, Input, Tone) && Interactive)
            {
                Session->BumpRevision();
            }
            CY += 52.0f;
            if (ConstructStudioKnobSlider(Surface, { CX, CY, CtrlW, 46.0f }, 108, ActiveSliderId, "Width", Layer->WidthPx, 4.0f, 512.0f, "px", 0, Input, Tone) && Interactive)
            {
                Session->BumpRevision();
            }
            CY += 52.0f;
            if (ConstructStudioKnobSlider(Surface, { CX, CY, CtrlW, 46.0f }, 109, ActiveSliderId, "Height", Layer->HeightPx, 4.0f, 512.0f, "px", 0, Input, Tone) && Interactive)
            {
                Session->BumpRevision();
            }
            CY += 52.0f;
            if (ConstructStudioKnobSlider(Surface, { CX, CY, CtrlW, 46.0f }, 110, ActiveSliderId, "Rotation", Layer->RotationDeg, -180.0f, 180.0f, "deg", 0, Input, Tone) && Interactive)
            {
                Session->BumpRevision();
            }
        }
        Y += BoxH + 10.0f;

        // 2d. Category Group 2: PBR Surface Channels (distinct header with white icon on amber tile + categorized body)
        char SurfTagBuf[24] {};
        std::snprintf(SurfTagBuf, sizeof(SurfTagBuf), "%zu ACTIVE", Layer->Channels.size());
        std::size_t ScalarChCount = 0u;
        for (const auto& Ch : Layer->Channels)
        {
            if (!PatternStampWorkspaceSession::QueryChannelDescriptor(Ch.Key).ColorChannel)
            {
                ++ScalarChCount;
            }
        }
        const std::size_t ShownChCount = std::min<std::size_t>(ScalarChCount, 4u);
        const float SurfBodyH = SurfaceGroupOpen
            ? (10.0f + 52.0f + 34.0f + static_cast<float>(ShownChCount) * 52.0f + 10.0f)
            : 0.0f;
        const float SurfBoxH = GroupHeadH + SurfBodyH;

        if (ConstructCategoryGroupBox(
                Surface,
                { LeftX, Y, InnerW, SurfBoxH },
                GroupHeadH,
                ControlIcon::LayersSlabs,
                IconAccentTone::Amber,
                "PBR Surface Channels",
                SurfTagBuf,
                SurfaceGroupOpen,
                Input,
                Tone) && Interactive)
        {
            SurfaceGroupOpen = !SurfaceGroupOpen;
        }

        if (SurfaceGroupOpen)
        {
            float CY = Y + GroupHeadH + 10.0f;
            const float CX = LeftX + BodyPad;

            Rectangle2D FinishDropRect {};
            if (ConstructCapsuleDropdownRow(
                    Surface,
                    { CX, CY, CtrlW, 46.0f },
                    "Surface Finish",
                    QueryFinishName(Layer->FinishPreset),
                    OpenPopover == ActivePopoverKind::FinishPresetList,
                    Input,
                    Tone,
                    &FinishDropRect) && Interactive)
            {
                OpenPopover   = ActivePopoverKind::FinishPresetList;
                PopoverAnchor = FinishDropRect;
            }
            CY += 52.0f;

            if (ConstructStudioToggleRow(Surface, { CX, CY, CtrlW, 28.0f }, "Procedural Mask", "", Layer->Mask.Enabled, Input, Tone) && Interactive)
            {
                Session->BumpRevision();
            }
            CY += 34.0f;

            std::size_t Drawn = 0u;
            for (std::size_t CIdx = 0u; CIdx < Layer->Channels.size() && Drawn < 4u; ++CIdx)
            {
                auto& Ch = Layer->Channels[CIdx];
                const auto& Desc = PatternStampWorkspaceSession::QueryChannelDescriptor(Ch.Key);
                if (Desc.ColorChannel) continue;
                if (ConstructStudioKnobSlider(
                        Surface,
                        { CX, CY, CtrlW, 46.0f },
                        200 + static_cast<int32_t>(CIdx),
                        ActiveSliderId,
                        Desc.Label,
                        Ch.ScalarReading,
                        Desc.MinScalar,
                        Desc.MaxScalar,
                        "",
                        2,
                        Input,
                        Tone) && Interactive)
                {
                    Session->BumpRevision();
                }
                CY += 52.0f;
                ++Drawn;
            }
        }
        Y += SurfBoxH + 10.0f;

        // 2e. Category Group 3: Non-Destructive Modifiers (white icon on violet tile + categorized modifier sub-blocks)
        char ModTagBuf[24] {};
        std::snprintf(ModTagBuf, sizeof(ModTagBuf), "%zu MODS", Layer->Modifiers.size());
        float ModBodyH = 0.0f;
        if (ModifierGroupOpen)
        {
            ModBodyH += 10.0f + 32.0f; // top pad + "+ Modifier" button
            if (Layer->Modifiers.empty())
            {
                ModBodyH += 26.0f;
            }
            else
            {
                for (const auto& Mod : Layer->Modifiers)
                {
                    ModBodyH += 36.0f; // modifier header strip
                    if (Mod.Enabled)
                    {
                        if (Mod.Category == PatternStampModifierCategory::GridArray ||
                            Mod.Category == PatternStampModifierCategory::NoiseScale ||
                            Mod.Category == PatternStampModifierCategory::RadialArray)
                        {
                            ModBodyH += 52.0f * 2.0f + 8.0f;
                        }
                        else
                        {
                            ModBodyH += 52.0f + 8.0f;
                        }
                    }
                    ModBodyH += 8.0f;
                }
            }
            ModBodyH += 8.0f;
        }
        const float ModBoxH = GroupHeadH + ModBodyH;

        if (ConstructCategoryGroupBox(
                Surface,
                { LeftX, Y, InnerW, ModBoxH },
                GroupHeadH,
                ControlIcon::SparklesAntiAliasing,
                IconAccentTone::Violet,
                "Modifiers",
                ModTagBuf,
                ModifierGroupOpen,
                Input,
                Tone) && Interactive)
        {
            ModifierGroupOpen = !ModifierGroupOpen;
        }

        if (ModifierGroupOpen)
        {
            float CY = Y + GroupHeadH + 10.0f;
            const float CX = LeftX + BodyPad;

            const Rectangle2D AddModBtn { CX, CY, CtrlW, 26.0f };
            const bool AddModHover = AddModBtn.Contains(Cursor);
            Surface.FillRectangle(AddModBtn, AddModHover ? Tone.RowHover : Tone.FieldWell, 13.0f);
            Surface.StrokeRectangle(AddModBtn, AddModHover ? Tone.HighlightFill : Tone.BorderSubtle, 13.0f, 1.0f);
            ConstructCenteredWeightedText(Surface, FontWeightClass::Medium, AddModBtn, 9.8f, Tone.TextPrimary, "+ Add Modifier");
            if (AddModHover && QueryPrimaryPressed(Input) && Interactive)
            {
                OpenPopover   = ActivePopoverKind::AddModifierList;
                PopoverAnchor = AddModBtn;
            }
            CY += 32.0f;

            if (Layer->Modifiers.empty())
            {
                WorkspaceAppearance::ConstructWeightedText(
                    Surface, FontWeightClass::Regular, { CX, CY + 4.0f }, 9.8f, Tone.TextFaint, "No modifiers attached to this layer.");
            }
            else
            {
                for (std::size_t MIdx = 0u; MIdx < Layer->Modifiers.size(); ++MIdx)
                {
                    auto& Mod = Layer->Modifiers[MIdx];
                    const int32_t SliderSlot = 300 + static_cast<int32_t>(MIdx) * 10;

                    const Rectangle2D ModHead { CX, CY, CtrlW, 30.0f };
                    Surface.FillRectangle(ModHead, Tone.FieldWell, 8.0f);
                    Surface.StrokeRectangle(ModHead, Tone.Hairline, 8.0f, 1.0f);
                    ConstructWhiteIconTile(
                        Surface,
                        { ModHead.X + 5.0f, ModHead.Center().Y - 9.0f, 18.0f, 18.0f },
                        QueryModifierVectorIcon(Mod.Category),
                        Mod.Enabled ? QueryModifierAccentTone(Mod.Category) : IconAccentTone::Slate,
                        4.0f,
                        Tone,
                        !Mod.Enabled);
                    WorkspaceAppearance::ConstructWeightedText(
                        Surface, FontWeightClass::Medium, { ModHead.X + 30.0f, ModHead.Center().Y - 5.5f }, 10.4f, Tone.TextPrimary, QueryModifierTitle(Mod.Category));

                    // Toggle switch on right of modifier header
                    const Rectangle2D TogRect { ModHead.Right() - 44.0f, ModHead.Y + 4.0f, 38.0f, 22.0f };
                    if (ConstructStudioToggleRow(Surface, TogRect, "", "", Mod.Enabled, Input, Tone) && Interactive)
                    {
                        Session->RecomputeSummaryMetrics();
                        Session->BumpRevision();
                    }
                    CY += 36.0f;

                    if (Mod.Enabled)
                    {
                        if (Mod.Category == PatternStampModifierCategory::GridArray)
                        {
                            float Cols = static_cast<float>(Mod.Columns);
                            float Rows = static_cast<float>(Mod.Rows);
                            if (ConstructStudioKnobSlider(Surface, { CX, CY, CtrlW, 46.0f }, SliderSlot + 1, ActiveSliderId, "Columns", Cols, 1.0f, 18.0f, "col", 0, Input, Tone) && Interactive)
                            {
                                Mod.Columns = std::clamp(static_cast<int32_t>(std::lround(Cols)), 1, 18);
                                Session->RecomputeSummaryMetrics();
                                Session->BumpRevision();
                            }
                            CY += 52.0f;
                            if (ConstructStudioKnobSlider(Surface, { CX, CY, CtrlW, 46.0f }, SliderSlot + 2, ActiveSliderId, "Rows", Rows, 1.0f, 18.0f, "row", 0, Input, Tone) && Interactive)
                            {
                                Mod.Rows = std::clamp(static_cast<int32_t>(std::lround(Rows)), 1, 18);
                                Session->RecomputeSummaryMetrics();
                                Session->BumpRevision();
                            }
                            CY += 52.0f + 8.0f;
                        }
                        else if (Mod.Category == PatternStampModifierCategory::NoiseScale)
                        {
                            if (ConstructStudioKnobSlider(Surface, { CX, CY, CtrlW, 46.0f }, SliderSlot + 1, ActiveSliderId, "Max Scale", Mod.MaxScale, 0.5f, 3.0f, "x", 2, Input, Tone) && Interactive)
                            {
                                Session->RecomputeSummaryMetrics();
                                Session->BumpRevision();
                            }
                            CY += 52.0f;
                            if (ConstructStudioKnobSlider(Surface, { CX, CY, CtrlW, 46.0f }, SliderSlot + 2, ActiveSliderId, "Noise Frequency", Mod.Frequency, 0.5f, 12.0f, "Hz", 1, Input, Tone) && Interactive)
                            {
                                Session->BumpRevision();
                            }
                            CY += 52.0f + 8.0f;
                        }
                        else if (Mod.Category == PatternStampModifierCategory::RadialArray)
                        {
                            float Count = static_cast<float>(Mod.RadialCount);
                            if (ConstructStudioKnobSlider(Surface, { CX, CY, CtrlW, 46.0f }, SliderSlot + 1, ActiveSliderId, "Radial Count", Count, 2.0f, 24.0f, "", 0, Input, Tone) && Interactive)
                            {
                                Mod.RadialCount = std::clamp(static_cast<int32_t>(std::lround(Count)), 2, 24);
                                Session->RecomputeSummaryMetrics();
                                Session->BumpRevision();
                            }
                            CY += 52.0f;
                            if (ConstructStudioKnobSlider(Surface, { CX, CY, CtrlW, 46.0f }, SliderSlot + 2, ActiveSliderId, "Orbit Radius", Mod.RadiusPx, 16.0f, 220.0f, "px", 0, Input, Tone) && Interactive)
                            {
                                Session->BumpRevision();
                            }
                            CY += 52.0f + 8.0f;
                        }
                        else
                        {
                            if (ConstructStudioKnobSlider(Surface, { CX, CY, CtrlW, 46.0f }, SliderSlot + 1, ActiveSliderId, "Strength", Mod.WarpAmplitude, 0.0f, 64.0f, "px", 1, Input, Tone) && Interactive)
                            {
                                Session->BumpRevision();
                            }
                            CY += 52.0f + 8.0f;
                        }
                    }
                    CY += 8.0f;
                }
            }
        }
        Y += ModBoxH + 10.0f;
    }

    // 2f. Category Group 4: Document & Canvas (white icon on teal tile)
    {
        constexpr float GroupHeadH = 38.0f;
        const float BodyPad       = 12.0f;
        const float CtrlW         = InnerW - BodyPad * 2.0f;
        const float DocBodyH      = DocumentGroupOpen ? (10.0f + 50.0f + 34.0f + 10.0f) : 0.0f;
        const float DocBoxH      = GroupHeadH + DocBodyH;

        char DocTag[24] {};
        std::snprintf(DocTag, sizeof(DocTag), "%u PX", Session->ResolutionPx);
        if (ConstructCategoryGroupBox(
                Surface,
                { LeftX, Y, InnerW, DocBoxH },
                GroupHeadH,
                ControlIcon::LayoutGrid,
                IconAccentTone::Teal,
                "Document & Canvas",
                DocTag,
                DocumentGroupOpen,
                Input,
                Tone) && Interactive)
        {
            DocumentGroupOpen = !DocumentGroupOpen;
        }

        if (DocumentGroupOpen)
        {
            float CY = Y + GroupHeadH + 10.0f;
            const float CX = LeftX + BodyPad;

            WorkspaceAppearance::ConstructWeightedText(Surface, FontWeightClass::Light, { CX, CY }, 10.4f, Tone.TextSecondary, "Output Resolution");
            constexpr std::array<std::string_view, 4u> ResLabels {{ "256", "512", "1024", "2048" }};
            constexpr std::array<uint32_t, 4u> ResVals {{ 256u, 512u, 1024u, 2048u }};
            int32_t CurResIdx = 2;
            for (std::size_t R = 0u; R < ResVals.size(); ++R)
            {
                if (Session->ResolutionPx == ResVals[R]) CurResIdx = static_cast<int32_t>(R);
            }
            const int32_t PickedRes = ConstructSegmentedCapsuleBar(Surface, { CX, CY + 17.0f, CtrlW, 27.0f }, ResLabels, CurResIdx, Input, Tone);
            if (PickedRes >= 0 && Interactive)
            {
                Session->AssignResolution(ResVals[static_cast<std::size_t>(PickedRes)]);
            }
            CY += 50.0f;

            if (ConstructStudioToggleRow(Surface, { CX, CY, CtrlW, 28.0f }, "Canvas Ground Fill", "", Doc.GroundEnabled, Input, Tone) && Interactive)
            {
                Session->BumpRevision();
            }
        }
        Y += DocBoxH + 10.0f;
    }

    const float TotalHeight = (Y + ScrollOffsetY) - (BodyRect.Y + 10.0f) + 12.0f;
    MaxScrollY              = std::max(0.0f, TotalHeight - BodyRect.Height);
    ScrollOffsetY           = std::clamp(ScrollOffsetY, 0.0f, MaxScrollY);
    Surface.PopClip();

    // 3. Edge-to-Edge Inspector Footer
    const std::string_view StatusTxt = Session->StatusNotice.empty()
        ? (PatternMode ? "Seamless tile ready" : "Decal stack ready")
        : std::string_view(Session->StatusNotice);
    const InspectorFooterResult FootOut = ConstructInspectorFooterBar(
        Surface, FooterRect, ".svg", "Duplicate", "Reset", true, StatusTxt, Input, Tone);
    if (FootOut.PrimaryClicked)
    {
        const std::string SvgData = Session->ExportDocumentToSvgString();
        Session->StatusNotice = "Exported SVG (" + std::to_string(SvgData.size()) + " B)";
    }
    else if (FootOut.SecondaryAClicked)
    {
        Session->DuplicateSelectedLayer();
    }
    else if (FootOut.SecondaryBClicked)
    {
        Session->LoadPresetByIndex(Session->QueryActivePresetIndex());
    }

    // 4. Overlay Popovers
    if (OpenPopover == ActivePopoverKind::StarterLibrary)
    {
        const auto& Presets = Session->QueryPresetDirectory();
        std::vector<PopoverOptionSpec> Options;
        Options.reserve(Presets.size());
        for (const auto& P : Presets)
        {
            Options.push_back(PopoverOptionSpec{ P.Title, P.Subtitle, P.SwatchA });
        }
        bool Dismissed = false;
        const int32_t Picked = ConstructStudioPopoverList(
            Surface, PopoverAnchor, Pane, PatternMode ? "PATTERN STARTERS" : "STAMP STARTERS", Options,
            static_cast<int32_t>(Session->QueryActivePresetIndex()), Input, Tone, Dismissed);
        if (Picked >= 0)
        {
            Session->LoadPresetByIndex(static_cast<std::size_t>(Picked));
            OpenPopover = ActivePopoverKind::None;
        }
        else if (Dismissed)
        {
            OpenPopover = ActivePopoverKind::None;
        }
    }
    else if (OpenPopover == ActivePopoverKind::MotifShapeList && Layer != nullptr)
    {
        const auto& Labels = PatternStampWorkspaceSession::InspectMotifShapeLabels();
        std::vector<PopoverOptionSpec> Options;
        Options.reserve(Labels.size());
        for (const char* Lbl : Labels)
        {
            Options.push_back({ Lbl, "", {} });
        }
        bool Dismissed = false;
        const int32_t Picked = ConstructStudioPopoverList(
            Surface, PopoverAnchor, Pane, "MOTIF SHAPE", Options, static_cast<int32_t>(Layer->MotifShape), Input, Tone, Dismissed);
        if (Picked >= 0)
        {
            Session->PushUndoSnapshot();
            Layer->MotifShape = static_cast<PatternStampMotifShape>(Picked);
            Session->BumpRevision();
            OpenPopover = ActivePopoverKind::None;
        }
        else if (Dismissed)
        {
            OpenPopover = ActivePopoverKind::None;
        }
    }
    else if (OpenPopover == ActivePopoverKind::FinishPresetList && Layer != nullptr)
    {
        const auto& Labels = PatternStampWorkspaceSession::InspectFinishLabels();
        std::vector<PopoverOptionSpec> Options;
        Options.reserve(Labels.size());
        for (const char* Lbl : Labels)
        {
            Options.push_back({ Lbl, "", {} });
        }
        bool Dismissed = false;
        const int32_t Picked = ConstructStudioPopoverList(
            Surface, PopoverAnchor, Pane, "SURFACE FINISH", Options, static_cast<int32_t>(Layer->FinishPreset), Input, Tone, Dismissed);
        if (Picked >= 0)
        {
            Session->ApplyFinishPresetToSelectedLayer(static_cast<PatternStampFinishPreset>(Picked));
            OpenPopover = ActivePopoverKind::None;
        }
        else if (Dismissed)
        {
            OpenPopover = ActivePopoverKind::None;
        }
    }
    else if (OpenPopover == ActivePopoverKind::AddModifierList && Layer != nullptr)
    {
        const auto& Labels = PatternStampWorkspaceSession::InspectModifierLabels();
        std::vector<PopoverOptionSpec> Options;
        Options.reserve(Labels.size());
        for (const char* Lbl : Labels)
        {
            Options.push_back({ Lbl, "", Tone.AccentViolet });
        }
        bool Dismissed = false;
        const int32_t Picked = ConstructStudioPopoverList(
            Surface, PopoverAnchor, Pane, "ADD MODIFIER", Options, -1, Input, Tone, Dismissed);
        if (Picked >= 0)
        {
            Session->AddModifierToSelectedLayer(static_cast<PatternStampModifierCategory>(Picked));
            OpenPopover = ActivePopoverKind::None;
        }
        else if (Dismissed)
        {
            OpenPopover = ActivePopoverKind::None;
        }
    }
}

} // namespace Frontier::Workspaces
