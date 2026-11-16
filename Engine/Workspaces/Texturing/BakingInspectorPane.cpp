/*============================================================================================================================================
                                                           BAKINGINSPECTORPANE.CPP
============================================================================================================================================*/
// 🧩 Implementation of BakingInspectorPane in Project Zero studio styling wired live to BakingWorkspaceSession

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
    const StudioToneSet Tone      = QueryStudioToneSet();
    const Rectangle2D   ShellRect = ConstructDockedSurfaceShell(Surface, Bounds, false, Tone);
    if (Session == nullptr)
    {
        return;
    }

    constexpr float PadX    = 14.0f;
    constexpr float FooterH = 66.0f;
    const float     InnerW  = std::max(80.0f, ShellRect.Width - PadX * 2.0f);
    const float     LeftX   = ShellRect.X + PadX;
    float           TopY    = ShellRect.Y + 8.0f;

    // 1. Fixed Inspector Header Bar (amber BakeBox mark + Channel Baking / HIGH -> LOW CAGE + Reset)
    const InspectorHeadingResult HeadOut = ConstructInspectorHeading(
        Surface,
        { LeftX, TopY, InnerW, 42.0f },
        ControlIcon::BakeBox,
        "Map Baking",
        "HIGH -> LOW CAGE",
        false,
        false,
        false,
        true,
        Input,
        Tone);
    if (HeadOut.ResetClicked)
    {
        Session->ResetDefaults();
    }
    TopY += 50.0f;

    // 2. Scrollable Body
    const Rectangle2D FooterRect { ShellRect.X + 1.0f, ShellRect.Bottom() - FooterH, ShellRect.Width - 2.0f, FooterH };
    const Rectangle2D BodyRect   { ShellRect.X + 4.0f, TopY, ShellRect.Width - 8.0f, std::max(60.0f, FooterRect.Y - TopY - 4.0f) };

    const Point2D Cursor = QueryInputCursor(Input);
    if (!StudyPopoverOpen && BodyRect.Contains(Cursor) && std::fabs(Input.WheelDeltaY) > 1.0e-4f)
    {
        ScrollOffsetY = std::clamp(ScrollOffsetY - Input.WheelDeltaY * 36.0f, 0.0f, MaxScrollY);
    }

    const float StartY = BodyRect.Y + 4.0f - ScrollOffsetY;
    float       Y      = StartY;

    Surface.PushClip(BodyRect);

    // 2a. Identity Block (STUDY / Utah Teapot / Study selector)
    const auto& ActiveStudy = Session->QueryActiveStudyDescriptor();
    char TriSub[72] {};
    std::snprintf(TriSub, sizeof(TriSub), "%u low -> %u high tris", Session->QueryActiveLowTriangles(), Session->QueryActiveHighTriangles());
    if (ConstructIdentityBlock(
            Surface,
            { LeftX, Y, InnerW, 56.0f },
            "STUDY",
            ActiveStudy.Label,
            TriSub,
            "Study",
            StudyPopoverOpen,
            Input,
            Tone,
            &StudyAnchorRect))
    {
        StudyPopoverOpen = !StudyPopoverOpen;
    }
    Y += 64.0f;

    // 2a-bis. Low -> High Source Pair strip matching HTML BakeInspector
    {
        constexpr float SlotGap = 16.0f;
        const float SlotW = (InnerW - SlotGap) * 0.5f;
        const Rectangle2D LowSlot  { LeftX, Y, SlotW, 38.0f };
        const Rectangle2D HighSlot { LowSlot.Right() + SlotGap, Y, SlotW, 38.0f };

        auto ConstructSourceTile = [&](Rectangle2D Slot, std::string_view Title, std::string_view CustomLbl)
        {
            Surface.FillRectangle(Slot, Tone.InsetSurface, 11.0f);
            Surface.StrokeRectangle(Slot, Tone.BorderSubtle, 11.0f, 1.0f);
            VectorIconRepository::ConstructControl(Surface, ControlIcon::UploadImport, {
                Slot.X + 9.0f, Slot.Center().Y - 6.0f, 12.0f, Tone.TextMuted, 1.85f
            });
            Surface.PushClip({ Slot.X + 26.0f, Slot.Y + 2.0f, Slot.Width - 32.0f, Slot.Height - 4.0f });
            WorkspaceAppearance::ConstructWeightedText(Surface, FontWeightClass::Medium, { Slot.X + 26.0f, Slot.Y + 6.0f }, 10.0f, Tone.TextPrimary, Title);
            WorkspaceAppearance::ConstructWeightedText(
                Surface,
                FontWeightClass::Regular,
                { Slot.X + 26.0f, Slot.Y + 20.0f },
                8.8f,
                CustomLbl.empty() ? Tone.TextFaint : Tone.Blue,
                CustomLbl.empty() ? std::string_view("Study source") : CustomLbl);
            Surface.PopClip();
        };
        ConstructSourceTile(LowSlot, "Low", Session->CustomLowAssetLabel);
        ConstructSourceTile(HighSlot, "High", Session->CustomHighAssetLabel);
        WorkspaceAppearance::ConstructWeightedText(
            Surface, FontWeightClass::Regular, { LowSlot.Right() + 4.0f, Y + 12.0f }, 10.5f, Tone.TextFaint, "->");
        Y += 46.0f;
    }

    // 2b. Project Zero Bake Execution Bar (white primary Bake button + device segmented bar + amber progress track + quiet metrics)
    const std::size_t SelectedCount = Session->CountSelectedTargets();
    const Rectangle2D BakeBarRect { LeftX, Y, InnerW, 82.0f };
    Surface.FillRectangle(BakeBarRect, Tone.InsetSurface, 14.0f);
    Surface.StrokeRectangle(BakeBarRect, Tone.BorderSubtle, 14.0f, 1.0f);

    const float BarPad = 10.0f;
    const float BtnW   = std::clamp(InnerW * 0.44f, 100.0f, 148.0f);
    const Rectangle2D BakeBtn { BakeBarRect.X + BarPad, BakeBarRect.Y + 10.0f, BtnW, 28.0f };
    const bool BakeHover = !StudyPopoverOpen && BakeBtn.Contains(Cursor);

    char BakeLabel[32] {};
    std::snprintf(BakeLabel, sizeof(BakeLabel), Session->BakingActive ? "Baking..." : "Bake %zu maps", SelectedCount);
    const ColorRgba BakeFill = Session->BakingActive
        ? Tone.Crimson
        : (BakeHover ? ColorRgba{ 250u, 250u, 250u, 255u } : Tone.PrimaryFill);
    const ColorRgba BakeInk = Session->BakingActive ? ColorRgba{ 255u, 255u, 255u, 255u } : Tone.PrimaryInk;
    Surface.FillRectangle(BakeBtn, BakeFill, 14.0f);
    ConstructCenteredWeightedText(Surface, FontWeightClass::Medium, BakeBtn, 10.2f, BakeInk, BakeLabel);
    if (BakeHover && Input.PrimaryPointer.Pressed && SelectedCount > 0u)
    {
        Session->ExecuteBakeSimulation();
    }

    // Device segmented bar (Auto / GTX / RTX)
    const Rectangle2D ComputeRect { BakeBtn.Right() + 8.0f, BakeBtn.Y, BakeBarRect.Right() - BarPad - (BakeBtn.Right() + 8.0f), 28.0f };
    constexpr std::array<std::string_view, 3u> ComputeOptions {{ "Auto", "GTX", "RTX" }};
    const int32_t PickedCompute = ConstructSegmentedCapsuleBar(Surface, ComputeRect, ComputeOptions, static_cast<int32_t>(Session->ComputeDevice), Input, Tone);
    if (PickedCompute >= 0 && !StudyPopoverOpen)
    {
        Session->ComputeDevice = static_cast<BakerComputeDevice>(PickedCompute);
    }

    // Thin amber progress bar
    const Rectangle2D ProgTrack { BakeBarRect.X + BarPad, BakeBarRect.Y + 46.0f, BakeBarRect.Width - BarPad * 2.0f, 4.5f };
    Surface.FillRectangle(ProgTrack, Tone.RecessedWell, 2.25f);
    const float ProgFill = Session->SurfaceBaked ? 1.0f : std::clamp(Session->BakeProgress, 0.0f, 1.0f);
    if (ProgFill > 0.01f)
    {
        Surface.FillRectangle({ ProgTrack.X, ProgTrack.Y, ProgTrack.Width * ProgFill, ProgTrack.Height }, Session->SurfaceBaked ? Tone.Green : Tone.Amber, 2.25f);
    }

    // Quiet single-line metrics readout below progress track
    char MetricLine[96] {};
    const float OutMb = EstimateOutputMegabytes(*Session);
    std::snprintf(MetricLine, sizeof(MetricLine), "%.1f MB output · %upx · %d rays", OutMb, Session->TargetResolutionPx, Session->AoSampleCount);
    WorkspaceAppearance::ConstructWeightedText(
        Surface, FontWeightClass::Regular, { BakeBarRect.X + BarPad, BakeBarRect.Y + 58.0f }, 9.2f, Tone.TextMuted, MetricLine);

    const std::string_view ResultTag = Session->SurfaceBaked ? "Fresh" : "Unbaked";
    WorkspaceAppearance::ConstructRightAlignedText(
        Surface, FontWeightClass::Medium, { BakeBarRect.Right() - BarPad, BakeBarRect.Y + 58.0f }, 9.2f,
        Session->SurfaceBaked ? Tone.Green : Tone.Amber, ResultTag);

    Y += BakeBarRect.Height + 12.0f;

    // 2c. Open Target Editor (warm amber-outlined accordion container for Session->InspectedTarget)
    const auto& TargetDesc   = BakingWorkspaceSession::QueryTargetDescriptor(Session->InspectedTarget);
    const bool  TargetQueued = Session->SelectedTargets[static_cast<std::size_t>(Session->InspectedTarget)];
    const bool  TargetReady  = Session->QueryTargetReady(Session->InspectedTarget);
    const bool  ShowNormal   = QueryNormalTargetKind(Session->InspectedTarget);
    const bool  ShowAo       = QueryAoTargetKind(Session->InspectedTarget);
    const bool  ShowVolume   = (Session->InspectedTarget == BakerTargetId::VolumeSdf);

    float EditorContentH = 26.0f; // footer line
    if (ShowNormal) EditorContentH += 36.0f;
    if (ShowAo)     EditorContentH += 94.0f;
    if (ShowVolume) EditorContentH += 174.0f;
    if (!ShowNormal && !ShowAo && !ShowVolume) EditorContentH += 48.0f;

    const Rectangle2D TargetBlock { LeftX, Y, InnerW, 44.0f + EditorContentH };
    Surface.FillRectangle(TargetBlock, Tone.InsetSurface, 14.0f);
    Surface.StrokeRectangle(TargetBlock, Tone.OpenRowStroke, 14.0f, 1.0f);

    // Header inside open target block
    const Rectangle2D SwatchBox { TargetBlock.X + 10.0f, TargetBlock.Y + 10.0f, 24.0f, 24.0f };
    BakingTargetsPane::ConstructTargetPreviewSwatch(Surface, SwatchBox, Session->InspectedTarget, TargetReady);
    Surface.StrokeRectangle(SwatchBox, Tone.BorderStrong, 6.0f, 1.0f);

    WorkspaceAppearance::ConstructWeightedText(
        Surface, FontWeightClass::Medium, { SwatchBox.Right() + 9.0f, TargetBlock.Y + 9.0f }, 11.4f, Tone.TextPrimary, TargetDesc.Label);
    WorkspaceAppearance::ConstructWeightedText(
        Surface, FontWeightClass::Regular, { SwatchBox.Right() + 9.0f, TargetBlock.Y + 24.0f }, 9.0f, Tone.TextMuted, TargetDesc.ShortSummary);

    const std::string_view StatusLabel = TargetReady ? "READY" : (TargetQueued ? "QUEUED" : "SKIPPED");
    const Rectangle2D TagBox { TargetBlock.Right() - 62.0f, TargetBlock.Y + 13.0f, 52.0f, 18.0f };
    Surface.FillRectangle(TagBox, ResolveTagBackground(TargetReady ? TagTone::Green : (TargetQueued ? TagTone::Amber : TagTone::Neutral), Tone), 9.0f);
    ConstructCenteredWeightedText(
        Surface, FontWeightClass::Medium, TagBox, 8.4f, ResolveTagInk(TargetReady ? TagTone::Green : (TargetQueued ? TagTone::Amber : TagTone::Neutral), Tone), StatusLabel);

    float EdY = TargetBlock.Y + 44.0f;
    const float EdX = TargetBlock.X + 10.0f;
    const float EdW = TargetBlock.Width - 20.0f;
    Surface.StrokeLine({ EdX, EdY - 4.0f }, { EdX + EdW, EdY - 4.0f }, Tone.Hairline, 1.0f);

    if (ShowNormal)
    {
        constexpr std::array<std::string_view, 2u> Conventions {{ "OpenGL +Y", "DirectX -Y" }};
        const int32_t PickedConv = ConstructSegmentedCapsuleBar(Surface, { EdX, EdY + 2.0f, EdW, 26.0f }, Conventions, static_cast<int32_t>(Session->NormalConvention), Input, Tone);
        if (PickedConv >= 0 && !StudyPopoverOpen)
        {
            Session->NormalConvention = static_cast<TangentNormalConvention>(PickedConv);
        }
        EdY += 36.0f;
    }

    if (ShowAo)
    {
        float AoRays = static_cast<float>(Session->AoSampleCount);
        if (ConstructStudioKnobSlider(Surface, { EdX, EdY, EdW, 42.0f }, 301, ActiveSliderId, "AO ray samples", AoRays, 4.0f, 64.0f, "rays", 0, Input, Tone))
        {
            Session->AoSampleCount = static_cast<int32_t>(std::lround(AoRays));
        }
        EdY += 46.0f;

        (void)ConstructStudioKnobSlider(Surface, { EdX, EdY, EdW, 42.0f }, 302, ActiveSliderId, "Max ray distance", Session->AoMaxRayDistanceMeters, 0.05f, 2.0f, "m", 2, Input, Tone);
        EdY += 48.0f;
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
        const int32_t PickedGrid = ConstructSegmentedCapsuleBar(Surface, { EdX, EdY + 2.0f, EdW, 26.0f }, GridLabels, ActiveGrid, Input, Tone);
        if (PickedGrid >= 0 && !StudyPopoverOpen)
        {
            Session->AssignVolumeGridDim(GridDims[static_cast<std::size_t>(PickedGrid)]);
        }
        EdY += 36.0f;

        (void)ConstructStudioToggleRow(Surface, { EdX, EdY, EdW, 32.0f }, "Signed interior", "Negative distance inside solid", Session->SignedInteriorDistance, Input, Tone);
        EdY += 36.0f;

        (void)ConstructStudioKnobSlider(Surface, { EdX, EdY, EdW, 42.0f }, 303, ActiveSliderId, "Bounds padding", Session->VolumeBoundsPadding, 0.02f, 0.50f, "m", 2, Input, Tone);
        EdY += 46.0f;

        if (ConstructStudioKnobSlider(Surface, { EdX, EdY, EdW, 42.0f }, 304, ActiveSliderId, "Section depth", Session->SectionDepthNormalized, 0.0f, 1.0f, "", 2, Input, Tone))
        {
            Session->RefreshSdfSectionOnGpu();
        }
        EdY += 48.0f;
    }

    if (!ShowNormal && !ShowAo && !ShowVolume)
    {
        (void)ConstructStudioKnobSlider(Surface, { EdX, EdY, EdW, 42.0f }, 305, ActiveSliderId, "Front cage offset", Session->FrontCageOffsetMeters, 0.01f, 0.60f, "m", 2, Input, Tone);
        EdY += 48.0f;
    }

    // Subtle editor foot telemetry line
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
    WorkspaceAppearance::ConstructWeightedText(Surface, FontWeightClass::Regular, { EdX, EdY + 2.0f }, 8.6f, Tone.TextFaint, EdFoot);

    Y += TargetBlock.Height + 12.0f;

    // 2d. Collapsible "Output & cage" fold panel
    const float OutBodyH = OutputFoldExpanded ? 248.0f : 0.0f;
    const Rectangle2D OutFold { LeftX, Y, InnerW, 38.0f + OutBodyH };
    Surface.FillRectangle(OutFold, Tone.FoldSurface, 15.0f);
    Surface.StrokeRectangle(OutFold, Tone.BorderSubtle, 15.0f, 1.0f);

    char OutSummary[32] {};
    std::snprintf(OutSummary, sizeof(OutSummary), "%upx · %dpx pad", Session->TargetResolutionPx, Session->UvEdgePaddingPx);
    if (ConstructCollapsibleFoldHeader(Surface, { OutFold.X, OutFold.Y, OutFold.Width, 38.0f }, "Output & cage", OutSummary, OutputFoldExpanded, Input, Tone) && !StudyPopoverOpen)
    {
        OutputFoldExpanded = !OutputFoldExpanded;
    }

    if (OutputFoldExpanded)
    {
        const float FX = OutFold.X + 12.0f;
        const float FW = OutFold.Width - 24.0f;
        float       FY = OutFold.Y + 42.0f;

        constexpr std::array<std::string_view, 4u> ResLabels {{ "256", "512", "1K", "2K" }};
        constexpr std::array<uint32_t, 4u> ResVals { 256u, 512u, 1024u, 2048u };
        int32_t ActiveRes = 1;
        for (int32_t I = 0; I < 4; ++I)
        {
            if (ResVals[static_cast<std::size_t>(I)] == Session->TargetResolutionPx) ActiveRes = I;
        }
        const int32_t PickedRes = ConstructSegmentedCapsuleBar(Surface, { FX, FY, FW, 26.0f }, ResLabels, ActiveRes, Input, Tone);
        if (PickedRes >= 0 && !StudyPopoverOpen)
        {
            Session->AssignResolution(ResVals[static_cast<std::size_t>(PickedRes)]);
        }
        FY += 36.0f;

        float PadPx = static_cast<float>(Session->UvEdgePaddingPx);
        if (ConstructStudioKnobSlider(Surface, { FX, FY, FW, 42.0f }, 310, ActiveSliderId, "UV edge padding", PadPx, 0.0f, 32.0f, "px", 0, Input, Tone))
        {
            Session->UvEdgePaddingPx = static_cast<int32_t>(std::lround(PadPx));
        }
        FY += 46.0f;

        (void)ConstructStudioKnobSlider(Surface, { FX, FY, FW, 42.0f }, 311, ActiveSliderId, "Front cage offset", Session->FrontCageOffsetMeters, 0.01f, 0.60f, "m", 2, Input, Tone);
        FY += 46.0f;

        (void)ConstructStudioKnobSlider(Surface, { FX, FY, FW, 42.0f }, 312, ActiveSliderId, "Rear cage offset", Session->RearCageOffsetMeters, 0.01f, 0.60f, "m", 2, Input, Tone);
        FY += 48.0f;

        (void)ConstructStudioToggleRow(Surface, { FX, FY, FW, 30.0f }, "High-poly wireframe", "Overlay source topology in viewport", Session->ShowHighWireOverlay, Input, Tone);
        FY += 34.0f;
        (void)ConstructStudioToggleRow(Surface, { FX, FY, FW, 30.0f }, "Cage shell overlay", "Preview ray extrusion envelope", Session->ShowCageOverlay, Input, Tone);
    }
    Y += OutFold.Height + 12.0f;

    // 2e. Collapsible "3D Volume SDF" fold panel (always accessible when scrolling down, preserving Exhibit 10!)
    const float VolBodyH = VolumeFoldExpanded ? 114.0f : 0.0f;
    const Rectangle2D VolFold { LeftX, Y, InnerW, 38.0f + VolBodyH };
    Surface.FillRectangle(VolFold, Tone.FoldSurface, 15.0f);
    Surface.StrokeRectangle(VolFold, Tone.BorderSubtle, 15.0f, 1.0f);

    char VolSummary[32] {};
    std::snprintf(VolSummary, sizeof(VolSummary), "%u^3 %s", Session->VolumeGridDim, Session->SignedInteriorDistance ? "signed" : "unsigned");
    if (ConstructCollapsibleFoldHeader(Surface, { VolFold.X, VolFold.Y, VolFold.Width, 38.0f }, "Volume SDF", VolSummary, VolumeFoldExpanded, Input, Tone) && !StudyPopoverOpen)
    {
        VolumeFoldExpanded = !VolumeFoldExpanded;
    }

    if (VolumeFoldExpanded)
    {
        const float VX = VolFold.X + 12.0f;
        const float VW = VolFold.Width - 24.0f;
        float       VY = VolFold.Y + 42.0f;

        constexpr std::array<std::string_view, 3u> Axes {{ "Axis Z", "Axis Y", "Axis X" }};
        const int32_t PickedAxis = ConstructSegmentedCapsuleBar(Surface, { VX, VY, VW, 26.0f }, Axes, static_cast<int32_t>(Session->SectionAxis), Input, Tone);
        if (PickedAxis >= 0 && !StudyPopoverOpen)
        {
            Session->SectionAxis = static_cast<VolumeSectionAxis>(PickedAxis);
            Session->RefreshSdfSectionOnGpu();
        }
        VY += 36.0f;

        if (ConstructStudioKnobSlider(Surface, { VX, VY, VW, 42.0f }, 320, ActiveSliderId, "Slice depth", Session->SectionDepthNormalized, 0.0f, 1.0f, "", 2, Input, Tone))
        {
            Session->RefreshSdfSectionOnGpu();
        }
    }
    Y += VolFold.Height + 14.0f;

    Surface.PopClip();
    const float TotalHeight = (Y - StartY) + 6.0f;
    MaxScrollY              = std::max(0.0f, TotalHeight - BodyRect.Height);
    ScrollOffsetY           = std::clamp(ScrollOffsetY, 0.0f, MaxScrollY);

    // 3. Fixed Footer Controls (Send to Paint primary white capsule + Export PNG + .fvol + status)
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
            Options.push_back(PopoverOptionSpec{ St.Label, "", Tone.Amber });
        }
        bool Dismissed = false;
        const int32_t Picked = ConstructStudioPopoverList(
            Surface, StudyAnchorRect, ShellRect, "BAKE STUDY", Options, static_cast<int32_t>(Session->Study), Input, Tone, Dismissed);
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
