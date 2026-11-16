/*============================================================================================================================================
                                                           BAKINGINSPECTORPANE.CPP
============================================================================================================================================*/
// 🧩 Right-docked TextureStudio Baking Inspector (pane-direct surface, TextureStudio OLD header & 3 KPI tiles, categorized groups & sliders)

//------------------------------------------------------------------------------------------------------------------------
//                                                        INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Engine/Workspaces/Texturing/BakingInspectorPane.h"
#include "Engine/Workspaces/Texturing/BakingTargetsPane.h"
#include "Engine/Workspaces/Texturing/StudioSurfaceKit.h"

#include <imgui.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <string_view>
#include <vector>

namespace Frontier::Workspaces
{

using namespace StudioSurface;

namespace
{

[[nodiscard]] bool QueryNormalTargetKind(BakerTargetId Target) noexcept
{
    return Target == BakerTargetId::TangentNormal ||
           Target == BakerTargetId::WorldNormal ||
           Target == BakerTargetId::ModelSpaceNormal ||
           Target == BakerTargetId::BentNormal ||
           Target == BakerTargetId::BevelNormal;
}

[[nodiscard]] bool QueryAoTargetKind(BakerTargetId Target) noexcept
{
    return Target == BakerTargetId::AmbientOcclusion ||
           Target == BakerTargetId::SecondaryAo ||
           Target == BakerTargetId::BentNormal ||
           Target == BakerTargetId::Thickness ||
           Target == BakerTargetId::Curvature;
}

[[nodiscard]] float EstimateOutputMegabytes(const BakingWorkspaceSession& Session) noexcept
{
    float Total = 0.0f;
    const float Side = static_cast<float>(Session.TargetResolutionPx);
    for (std::size_t Idx = 0u; Idx < BakingWorkspaceSession::TargetCount; ++Idx)
    {
        if (!Session.SelectedTargets[Idx]) continue;
        if (static_cast<BakerTargetId>(Idx) == BakerTargetId::VolumeSdf)
        {
            const float D = static_cast<float>(Session.VolumeGridDim);
            Total += (D * D * D * 4.0f) / (1024.0f * 1024.0f);
        }
        else
        {
            Total += (Side * Side * 4.0f) / (1024.0f * 1024.0f);
        }
    }
    return Total;
}

} // namespace

BakingInspectorPane::~BakingInspectorPane() noexcept
{
    WorkspaceAppearance::ReleasePanePointerOwnership(this);
}

void BakingInspectorPane::AttachSession(BakingWorkspaceSession* TargetSession) noexcept
{
    Session        = TargetSession;
    ScrollOffsetY  = 0.0f;
    ActiveSliderId = -1;
}

void BakingInspectorPane::PresentInImGuiPane(double DeltaSeconds, float ScaleFactor)
{
    const float  SafeScale = std::max(0.25f, ScaleFactor);
    const ImVec2 WinPos    = ImGui::GetWindowPos();
    const ImVec2 WinSize   = ImGui::GetWindowSize();
    const ImVec2 CursorPos = ImGui::GetCursorScreenPos();
    const float  TopY      = CursorPos.y - ImGui::GetStyle().WindowPadding.y;
    const float  PaneH     = std::max(1.0f, (WinPos.y + WinSize.y) - TopY);

    ImGui::InvisibleButton(
        "##BakingInspectorCanvas",
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

void BakingInspectorPane::Render(
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
    const float         PadX   = 12.0f;
    const float         InnerW = std::max(80.0f, Pane.Width - PadX * 2.0f);
    const float         LeftX  = Pane.X + PadX;

    // 1. TextureStudio OLD Inspector Header Bar ("Baking Inspector" | "HIGH -> LOW CAGE" + Reset)
    const Rectangle2D HeaderStrip { Pane.X, Pane.Y, Pane.Width, 42.0f };
    const InspectorHeadingResult HeadOut = ConstructStudioInspectorHeader(
        Surface, HeaderStrip, "Baking Inspector", "HIGH -> LOW CAGE", false, false, false, true, Input, Tone);
    if (HeadOut.ResetClicked)
    {
        Session->ResetDefaults();
    }

    constexpr float FooterH = 62.0f;
    const Rectangle2D FooterRect { Pane.X, Pane.Bottom() - FooterH, Pane.Width, FooterH };
    const Rectangle2D BodyRect   { Pane.X, HeaderStrip.Bottom(), Pane.Width, std::max(60.0f, FooterRect.Y - HeaderStrip.Bottom()) };

    const Point2D Cursor = QueryInputCursor(Input);
    if (!StudyPopoverOpen && BodyRect.Contains(Cursor) && std::fabs(Input.WheelDeltaY) > 1.0e-4f)
    {
        ScrollOffsetY = std::clamp(ScrollOffsetY - Input.WheelDeltaY * 36.0f, 0.0f, MaxScrollY);
    }

    const float StartY = BodyRect.Y + 10.0f - ScrollOffsetY;
    float       Y      = StartY;
    const bool  Interactive = !StudyPopoverOpen;

    Surface.PushClip(BodyRect);

    // 2a. TextureStudio OLD Object Heading (large 20px light study title + mono uppercase subtitle + Study switcher)
    const auto& ActiveStudy = Session->QueryActiveStudyDescriptor();
    char StudySub[80] {};
    std::snprintf(StudySub, sizeof(StudySub), "LOW %u -> HIGH %u TRIS · %uPX",
        Session->QueryActiveLowTriangles(), Session->QueryActiveHighTriangles(), Session->TargetResolutionPx);
    if (ConstructStudioObjectHeading(
            Surface,
            { LeftX, Y, InnerW, 46.0f },
            ActiveStudy.Label,
            StudySub,
            "Study",
            StudyPopoverOpen,
            Input,
            Tone,
            &StudyAnchorRect))
    {
        StudyPopoverOpen = !StudyPopoverOpen;
    }
    Y += 56.0f;

    // 2b. TextureStudio OLD 3-Up KPI Info Tiles (Tile 1 lime-cream #EFF4CF: Queued; Tile 2: Output MB; Tile 3: Samples)
    const std::size_t SelectedCount = Session->CountSelectedTargets();
    const float       OutMb         = EstimateOutputMegabytes(*Session);
    char QueuedVal[20] {}, ResSub[20] {}, MbVal[20] {}, RaySub[24] {};
    std::snprintf(QueuedVal, sizeof(QueuedVal), "%zu/%zu", SelectedCount, BakingWorkspaceSession::TargetCount);
    std::snprintf(ResSub,    sizeof(ResSub),    "%upx RGBA8", Session->TargetResolutionPx);
    std::snprintf(MbVal,     sizeof(MbVal),     "%.1f MB", OutMb);
    std::snprintf(RaySub,    sizeof(RaySub),    "%d AO rays", Session->AoSampleCount);

    const std::array<KpiTileSpec, 3u> KpiTiles {{
        { "Queued", QueuedVal, ResSub, true },
        { "Output", MbVal,     Session->SurfaceBaked ? "Baked" : "Pending", false },
        { "Status", Session->SurfaceBaked ? "Fresh" : "Ready", RaySub, false },
    }};
    ConstructStudioKpiTrio(Surface, { LeftX, Y, InnerW, 64.0f }, KpiTiles, Tone);
    Y += 74.0f;

    constexpr float GroupHeadH = 38.0f;
    const float     BodyPad    = 12.0f;
    const float     CtrlW      = InnerW - BodyPad * 2.0f;

    // 2c. Category Group 1: Bake Queue & Sources (EXECUTE — white icon on green tile)
    {
        const float BodyH = ExecuteGroupOpen ? (10.0f + 38.0f + 10.0f + 28.0f + 10.0f + 24.0f + 8.0f) : 0.0f;
        const float BoxH  = GroupHeadH + BodyH;
        if (ConstructCategoryGroupBox(
                Surface, { LeftX, Y, InnerW, BoxH }, GroupHeadH,
                ControlIcon::BakeBox, IconAccentTone::Green, "Bake Queue & Sources", "EXECUTE", ExecuteGroupOpen, Input, Tone) && Interactive)
        {
            ExecuteGroupOpen = !ExecuteGroupOpen;
        }

        if (ExecuteGroupOpen)
        {
            const float CX = LeftX + BodyPad;
            float       CY = Y + GroupHeadH + 10.0f;

            // Low -> High Source Pair strip
            constexpr float SlotGap = 16.0f;
            const float SlotW = (CtrlW - SlotGap) * 0.5f;
            const Rectangle2D LowSlot  { CX, CY, SlotW, 38.0f };
            const Rectangle2D HighSlot { LowSlot.Right() + SlotGap, CY, SlotW, 38.0f };

            auto ConstructSourceTile = [&](Rectangle2D Slot, std::string_view Title, std::string_view CustomLbl)
            {
                Surface.FillRectangle(Slot, Tone.FieldWell, 10.0f);
                Surface.StrokeRectangle(Slot, Tone.BorderSubtle, 10.0f, 1.0f);
                VectorIconRepository::ConstructControl(Surface, ControlIcon::UploadImport, {
                    Slot.X + 9.0f, Slot.Center().Y - 6.0f, 12.0f, Tone.TextMuted, 1.85f
                });
                Surface.PushClip({ Slot.X + 26.0f, Slot.Y + 2.0f, Slot.Width - 30.0f, Slot.Height - 4.0f });
                WorkspaceAppearance::ConstructWeightedText(Surface, FontWeightClass::Medium, { Slot.X + 26.0f, Slot.Y + 6.0f }, 9.8f, Tone.TextPrimary, Title);
                WorkspaceAppearance::ConstructWeightedText(
                    Surface, FontWeightClass::Regular, { Slot.X + 26.0f, Slot.Y + 20.0f }, 8.6f,
                    CustomLbl.empty() ? Tone.TextFaint : Tone.AccentBlue,
                    CustomLbl.empty() ? std::string_view("Study source") : CustomLbl);
                Surface.PopClip();
            };
            ConstructSourceTile(LowSlot, "Low", Session->CustomLowAssetLabel);
            ConstructSourceTile(HighSlot, "High", Session->CustomHighAssetLabel);
            WorkspaceAppearance::ConstructWeightedText(
                Surface, FontWeightClass::Regular, { LowSlot.Right() + 4.0f, CY + 12.0f }, 10.0f, Tone.TextFaint, "->");
            CY += 48.0f;

            // Primary Bake trigger + Compute device selector
            const float BtnW = std::clamp(CtrlW * 0.45f, 96.0f, 144.0f);
            const Rectangle2D BakeBtn { CX, CY, BtnW, 28.0f };
            const bool BakeHover = Interactive && BakeBtn.Contains(Cursor);

            char BakeLabel[32] {};
            std::snprintf(BakeLabel, sizeof(BakeLabel), Session->BakingActive ? "Baking..." : "Bake %zu", SelectedCount);
            const ColorRgba BakeFill = Session->BakingActive
                ? Tone.Crimson
                : (BakeHover ? ColorRgba{ 246u, 250u, 222u, 255u } : Tone.HighlightFill);
            const ColorRgba BakeInk = Session->BakingActive ? ColorRgba{ 255u, 255u, 255u, 255u } : Tone.HighlightInk;
            Surface.FillRectangle(BakeBtn, BakeFill, 14.0f);
            ConstructCenteredWeightedText(Surface, FontWeightClass::Medium, BakeBtn, 10.0f, BakeInk, BakeLabel);
            if (BakeHover && QueryPrimaryPressed(Input) && SelectedCount > 0u)
            {
                Session->ExecuteBakeSimulation();
            }

            const Rectangle2D ComputeRect { BakeBtn.Right() + 8.0f, CY, CtrlW - BtnW - 8.0f, 28.0f };
            constexpr std::array<std::string_view, 3u> ComputeOptions {{ "Auto", "GTX", "RTX" }};
            const int32_t PickedCompute = ConstructSegmentedCapsuleBar(Surface, ComputeRect, ComputeOptions, static_cast<int32_t>(Session->ComputeDevice), Input, Tone);
            if (PickedCompute >= 0 && Interactive)
            {
                Session->ComputeDevice = static_cast<BakerComputeDevice>(PickedCompute);
            }
            CY += 38.0f;

            // Progress bar & telemetry line
            const Rectangle2D ProgTrack { CX, CY, CtrlW, 6.0f };
            Surface.FillRectangle(ProgTrack, Tone.FieldWell, 3.0f);
            Surface.StrokeRectangle(ProgTrack, Tone.BorderSubtle, 3.0f, 1.0f);
            const float ProgFill = Session->SurfaceBaked ? 1.0f : std::clamp(Session->BakeProgress, 0.0f, 1.0f);
            if (ProgFill > 0.01f)
            {
                Surface.FillRectangle({ ProgTrack.X, ProgTrack.Y, ProgTrack.Width * ProgFill, ProgTrack.Height },
                    Session->SurfaceBaked ? Tone.AccentGreen : Tone.HighlightFill, 3.0f);
            }

            char MetricLine[80] {};
            std::snprintf(MetricLine, sizeof(MetricLine), "%.1f MB · %upx · %d rays", OutMb, Session->TargetResolutionPx, Session->AoSampleCount);
            WorkspaceAppearance::ConstructWeightedText(Surface, FontWeightClass::Regular, { CX, CY + 10.0f }, 8.9f, Tone.TextMuted, MetricLine);
            WorkspaceAppearance::ConstructRightAlignedText(
                Surface, FontWeightClass::Medium, { CX + CtrlW, CY + 10.0f }, 8.9f,
                Session->SurfaceBaked ? Tone.AccentGreen : Tone.AccentAmber,
                Session->SurfaceBaked ? "Fresh" : "Unbaked");
        }
        Y += BoxH + 10.0f;
    }

    // 2d. Category Group 2: Active Target Settings (TARGET — white icon on blue tile)
    {
        const auto& TargetDesc   = BakingWorkspaceSession::QueryTargetDescriptor(Session->InspectedTarget);
        const bool  TargetQueued = Session->SelectedTargets[static_cast<std::size_t>(Session->InspectedTarget)];
        const bool  TargetReady  = Session->QueryTargetReady(Session->InspectedTarget);
        const bool  ShowNormal   = QueryNormalTargetKind(Session->InspectedTarget);
        const bool  ShowAo       = QueryAoTargetKind(Session->InspectedTarget);
        const bool  ShowVolume   = (Session->InspectedTarget == BakerTargetId::VolumeSdf);

        float BodyH = 0.0f;
        if (TargetGroupOpen)
        {
            BodyH = 10.0f + 36.0f + 18.0f; // target banner + telemetry line
            if (ShowNormal) BodyH += 36.0f;
            if (ShowAo)     BodyH += 104.0f;
            if (ShowVolume) BodyH += 174.0f;
            if (!ShowNormal && !ShowAo && !ShowVolume) BodyH += 52.0f;
            BodyH += 8.0f;
        }
        const float BoxH = GroupHeadH + BodyH;
        if (ConstructCategoryGroupBox(
                Surface, { LeftX, Y, InnerW, BoxH }, GroupHeadH,
                ControlIcon::SparklesAntiAliasing, IconAccentTone::Blue, "Active Target", "TARGET", TargetGroupOpen, Input, Tone) && Interactive)
        {
            TargetGroupOpen = !TargetGroupOpen;
        }

        if (TargetGroupOpen)
        {
            const float CX = LeftX + BodyPad;
            float       CY = Y + GroupHeadH + 10.0f;

            // Active Target Swatch + Title + Status Tag
            const Rectangle2D SwatchBox { CX, CY, 26.0f, 26.0f };
            BakingTargetsPane::ConstructTargetPreviewSwatch(Surface, SwatchBox, Session->InspectedTarget, TargetReady);
            Surface.StrokeRectangle(SwatchBox, Tone.BorderStrong, 0.0f, 1.0f);

            WorkspaceAppearance::ConstructWeightedText(Surface, FontWeightClass::Medium, { SwatchBox.Right() + 9.0f, CY + 1.0f }, 11.0f, Tone.TextPrimary, TargetDesc.Label);
            WorkspaceAppearance::ConstructWeightedText(Surface, FontWeightClass::Regular, { SwatchBox.Right() + 9.0f, CY + 15.0f }, 8.8f, Tone.TextMuted, TargetDesc.ShortSummary);

            const std::string_view StatusLabel = TargetReady ? "READY" : (TargetQueued ? "QUEUED" : "SKIPPED");
            const TagTone          StatusTone  = TargetReady ? TagTone::Green : (TargetQueued ? TagTone::Amber : TagTone::Neutral);
            const Rectangle2D      TagBox { CX + CtrlW - 54.0f, CY + 4.0f, 54.0f, 18.0f };
            Surface.FillRectangle(TagBox, ResolveTagBackground(StatusTone, Tone), 9.0f);
            ConstructCenteredWeightedText(Surface, FontWeightClass::Medium, TagBox, 8.2f, ResolveTagInk(StatusTone, Tone), StatusLabel);
            CY += 36.0f;

            if (ShowNormal)
            {
                constexpr std::array<std::string_view, 2u> Conventions {{ "OpenGL +Y", "DirectX -Y" }};
                const int32_t PickedConv = ConstructSegmentedCapsuleBar(Surface, { CX, CY, CtrlW, 26.0f }, Conventions, static_cast<int32_t>(Session->NormalConvention), Input, Tone);
                if (PickedConv >= 0 && Interactive)
                {
                    Session->NormalConvention = static_cast<TangentNormalConvention>(PickedConv);
                }
                CY += 36.0f;
            }

            if (ShowAo)
            {
                float AoRays = static_cast<float>(Session->AoSampleCount);
                if (ConstructStudioKnobSlider(Surface, { CX, CY, CtrlW, 46.0f }, 301, ActiveSliderId, "AO Ray Samples", AoRays, 4.0f, 64.0f, "rays", 0, Input, Tone) && Interactive)
                {
                    Session->AoSampleCount = static_cast<int32_t>(std::lround(AoRays));
                }
                CY += 52.0f;
                (void)ConstructStudioKnobSlider(Surface, { CX, CY, CtrlW, 46.0f }, 302, ActiveSliderId, "Max Ray Distance", Session->AoMaxRayDistanceMeters, 0.05f, 2.0f, "m", 2, Input, Tone);
                CY += 52.0f;
            }

            if (ShowVolume)
            {
                constexpr std::array<std::string_view, 4u> GridLabels {{ "24^3", "36^3", "48^3", "64^3" }};
                constexpr std::array<uint32_t, 4u> GridDims { 24u, 36u, 48u, 64u };
                int32_t ActiveGrid = 1;
                for (int32_t I = 0; I < 4; ++I)
                {
                    if (GridDims[static_cast<std::size_t>(I)] == Session->VolumeGridDim) ActiveGrid = I;
                }
                const int32_t PickedGrid = ConstructSegmentedCapsuleBar(Surface, { CX, CY, CtrlW, 26.0f }, GridLabels, ActiveGrid, Input, Tone);
                if (PickedGrid >= 0 && Interactive)
                {
                    Session->AssignVolumeGridDim(GridDims[static_cast<std::size_t>(PickedGrid)]);
                }
                CY += 34.0f;

                (void)ConstructStudioToggleRow(Surface, { CX, CY, CtrlW, 30.0f }, "Signed Interior", "Negative distance inside solid", Session->SignedInteriorDistance, Input, Tone);
                CY += 36.0f;

                (void)ConstructStudioKnobSlider(Surface, { CX, CY, CtrlW, 46.0f }, 303, ActiveSliderId, "Bounds Padding", Session->VolumeBoundsPadding, 0.02f, 0.50f, "m", 2, Input, Tone);
                CY += 52.0f;

                if (ConstructStudioKnobSlider(Surface, { CX, CY, CtrlW, 46.0f }, 304, ActiveSliderId, "Section Depth", Session->SectionDepthNormalized, 0.0f, 1.0f, "", 2, Input, Tone) && Interactive)
                {
                    Session->RefreshSdfSectionOnGpu();
                }
                CY += 52.0f;
            }

            if (!ShowNormal && !ShowAo && !ShowVolume)
            {
                (void)ConstructStudioKnobSlider(Surface, { CX, CY, CtrlW, 46.0f }, 305, ActiveSliderId, "Front Cage Offset", Session->FrontCageOffsetMeters, 0.01f, 0.60f, "m", 2, Input, Tone);
                CY += 52.0f;
            }

            char EdFoot[72] {};
            if (ShowVolume)
            {
                const uint32_t Vox = Session->VolumeGridDim * Session->VolumeGridDim * Session->VolumeGridDim;
                std::snprintf(EdFoot, sizeof(EdFoot), "volumeSdf · %u voxels", Vox);
            }
            else
            {
                std::snprintf(EdFoot, sizeof(EdFoot), "%s · %upx RGBA8", TargetDesc.Label, Session->TargetResolutionPx);
            }
            WorkspaceAppearance::ConstructWeightedText(Surface, FontWeightClass::Regular, { CX, CY }, 8.6f, Tone.TextFaint, EdFoot);
        }
        Y += BoxH + 10.0f;
    }

    // 2e. Category Group 3: Output & Cage (OUTPUT — white icon on amber tile)
    {
        const float BodyH = OutputGroupOpen ? (10.0f + 34.0f + 52.0f * 3.0f + 34.0f * 2.0f + 8.0f) : 0.0f;
        const float BoxH  = GroupHeadH + BodyH;
        if (ConstructCategoryGroupBox(
                Surface, { LeftX, Y, InnerW, BoxH }, GroupHeadH,
                ControlIcon::SlidersQuality, IconAccentTone::Amber, "Output & Cage", "OUTPUT", OutputGroupOpen, Input, Tone) && Interactive)
        {
            OutputGroupOpen = !OutputGroupOpen;
        }

        if (OutputGroupOpen)
        {
            const float CX = LeftX + BodyPad;
            float       CY = Y + GroupHeadH + 10.0f;

            constexpr std::array<std::string_view, 4u> ResLabels {{ "256", "512", "1K", "2K" }};
            constexpr std::array<uint32_t, 4u> ResVals { 256u, 512u, 1024u, 2048u };
            int32_t ActiveRes = 1;
            for (int32_t I = 0; I < 4; ++I)
            {
                if (ResVals[static_cast<std::size_t>(I)] == Session->TargetResolutionPx) ActiveRes = I;
            }
            const int32_t PickedRes = ConstructSegmentedCapsuleBar(Surface, { CX, CY, CtrlW, 26.0f }, ResLabels, ActiveRes, Input, Tone);
            if (PickedRes >= 0 && Interactive)
            {
                Session->AssignResolution(ResVals[static_cast<std::size_t>(PickedRes)]);
            }
            CY += 34.0f;

            float PadPx = static_cast<float>(Session->UvEdgePaddingPx);
            if (ConstructStudioKnobSlider(Surface, { CX, CY, CtrlW, 46.0f }, 310, ActiveSliderId, "UV Edge Padding", PadPx, 0.0f, 32.0f, "px", 0, Input, Tone) && Interactive)
            {
                Session->UvEdgePaddingPx = static_cast<int32_t>(std::lround(PadPx));
            }
            CY += 52.0f;

            (void)ConstructStudioKnobSlider(Surface, { CX, CY, CtrlW, 46.0f }, 311, ActiveSliderId, "Front Cage Offset", Session->FrontCageOffsetMeters, 0.01f, 0.60f, "m", 2, Input, Tone);
            CY += 52.0f;

            (void)ConstructStudioKnobSlider(Surface, { CX, CY, CtrlW, 46.0f }, 312, ActiveSliderId, "Rear Cage Offset", Session->RearCageOffsetMeters, 0.01f, 0.60f, "m", 2, Input, Tone);
            CY += 52.0f;

            (void)ConstructStudioToggleRow(Surface, { CX, CY, CtrlW, 28.0f }, "High-Poly Wireframe", "", Session->ShowHighWireOverlay, Input, Tone);
            CY += 34.0f;
            (void)ConstructStudioToggleRow(Surface, { CX, CY, CtrlW, 28.0f }, "Cage Shell Overlay", "", Session->ShowCageOverlay, Input, Tone);
        }
        Y += BoxH + 10.0f;
    }

    // 2f. Category Group 4: Volume SDF Slice (VOLUME — white icon on violet tile)
    {
        const float BodyH = VolumeGroupOpen ? (10.0f + 34.0f + 52.0f + 8.0f) : 0.0f;
        const float BoxH  = GroupHeadH + BodyH;
        if (ConstructCategoryGroupBox(
                Surface, { LeftX, Y, InnerW, BoxH }, GroupHeadH,
                ControlIcon::VolumeBoxes, IconAccentTone::Violet, "Volume SDF Slice", "VOLUME", VolumeGroupOpen, Input, Tone) && Interactive)
        {
            VolumeGroupOpen = !VolumeGroupOpen;
        }

        if (VolumeGroupOpen)
        {
            const float CX = LeftX + BodyPad;
            float       CY = Y + GroupHeadH + 10.0f;

            constexpr std::array<std::string_view, 3u> Axes {{ "Axis Z", "Axis Y", "Axis X" }};
            const int32_t PickedAxis = ConstructSegmentedCapsuleBar(Surface, { CX, CY, CtrlW, 26.0f }, Axes, static_cast<int32_t>(Session->SectionAxis), Input, Tone);
            if (PickedAxis >= 0 && Interactive)
            {
                Session->SectionAxis = static_cast<VolumeSectionAxis>(PickedAxis);
                Session->RefreshSdfSectionOnGpu();
            }
            CY += 34.0f;

            if (ConstructStudioKnobSlider(Surface, { CX, CY, CtrlW, 46.0f }, 320, ActiveSliderId, "Slice Depth", Session->SectionDepthNormalized, 0.0f, 1.0f, "", 2, Input, Tone) && Interactive)
            {
                Session->RefreshSdfSectionOnGpu();
            }
        }
        Y += BoxH + 12.0f;
    }

    const float TotalHeight = (Y - StartY) + 6.0f;
    MaxScrollY              = std::max(0.0f, TotalHeight - BodyRect.Height);
    ScrollOffsetY           = std::clamp(ScrollOffsetY, 0.0f, MaxScrollY);
    Surface.PopClip();

    // 3. Edge-to-Edge Inspector Footer (Send to Paint lime-cream primary button + PNG + .fvol + status)
    const InspectorFooterResult FootOut = ConstructInspectorFooterBar(
        Surface,
        FooterRect,
        Session->BridgedToPaint ? "Synced to Paint" : "Send to Paint",
        "PNG",
        ".fvol",
        Session->SurfaceBaked || Session->BridgedToPaint,
        Session->ProgressStatus,
        Input,
        Tone);
    if (FootOut.PrimaryClicked)
    {
        Session->SendBakedTargetsToPaint();
    }
    else if (FootOut.SecondaryAClicked)
    {
        (void)Session->ExportInspectedTargetPng();
    }
    else if (FootOut.SecondaryBClicked)
    {
        (void)Session->ExportVolumeSdfFvol();
    }

    // 4. Overlay Study Library Popover
    if (StudyPopoverOpen)
    {
        const auto& Studies = BakingWorkspaceSession::QueryStudyRegistry();
        std::vector<PopoverOptionSpec> Options {};
        Options.reserve(Studies.size());
        for (const auto& St : Studies)
        {
            Options.push_back(PopoverOptionSpec{ St.Label, "", Tone.AccentAmber });
        }
        bool Dismissed = false;
        const int32_t Picked = ConstructStudioPopoverList(
            Surface, StudyAnchorRect, Pane, "BAKE STUDY", Options, static_cast<int32_t>(Session->Study), Input, Tone, Dismissed);
        if (Picked >= 0 && static_cast<std::size_t>(Picked) < Studies.size())
        {
            Session->SelectStudy(Studies[static_cast<std::size_t>(Picked)].Study);
            StudyPopoverOpen = false;
        }
        else if (Dismissed)
        {
            StudyPopoverOpen = false;
        }
    }
}

} // namespace Frontier::Workspaces
