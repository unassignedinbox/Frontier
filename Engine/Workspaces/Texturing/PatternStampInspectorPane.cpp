/*============================================================================================================================================
                                                        PATTERNSTAMPINSPECTORPANE.CPP
============================================================================================================================================*/
// 🧩 Implementation of PatternStampInspectorPane in Project Zero studio inspector styling wired live to PatternStampWorkspaceSession

//------------------------------------------------------------------------------------------------------------------------
//                                                        INCLUDES
//------------------------------------------------------------------------------------------------------------------------

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

constexpr std::array<ColorRgba, 8u> StudioSwatchRow {{
    { 239u, 244u, 207u, 255u },
    { 255u, 178u, 56u,  255u },
    { 255u, 107u, 94u,  255u },
    { 70u,  210u, 103u, 255u },
    { 77u,  155u, 255u, 255u },
    { 196u, 140u, 255u, 255u },
    { 148u, 154u, 166u, 255u },
    { 26u,  28u,  34u,  255u },
}};

[[nodiscard]] std::string_view QueryBlendLabel(uint8_t Mode) noexcept
{
    constexpr std::array<std::string_view, 6u> Labels {{ "Normal", "Multiply", "Screen", "Overlay", "Add", "Darken" }};
    return Labels[static_cast<std::size_t>(Mode) % Labels.size()];
}

void ConstructMiniatureDocumentPreview(
    PixelSpace&                          Surface,
    Rectangle2D                          ThumbRect,
    const PatternStampWorkspaceSession&  Session,
    const StudioToneSet&                 Tone) noexcept
{
    Surface.PushClip(ThumbRect);
    if (Session.Document.GroundEnabled)
    {
        Surface.FillRectangle(ThumbRect, Session.Document.GroundTintA, 11.0f);
    }
    else
    {
        // Subtle checkerboard for transparent decal preview
        constexpr float Cell = 8.0f;
        for (float Py = ThumbRect.Y; Py < ThumbRect.Bottom(); Py += Cell)
        {
            for (float Px = ThumbRect.X; Px < ThumbRect.Right(); Px += Cell)
            {
                const int Ix = static_cast<int>((Px - ThumbRect.X) / Cell);
                const int Iy = static_cast<int>((Py - ThumbRect.Y) / Cell);
                const ColorRgba Checker = ((Ix + Iy) % 2 == 0) ? ColorRgba{ 22u, 22u, 24u, 255u } : ColorRgba{ 32u, 32u, 36u, 255u };
                Surface.FillRectangle({ Px, Py, Cell, Cell }, Checker, 0.0f);
            }
        }
    }

    const float Cx = ThumbRect.Center().X;
    const float Cy = ThumbRect.Center().Y;
    for (std::size_t Idx = 0u; Idx < Session.Document.Layers.size(); ++Idx)
    {
        const auto& Layer = Session.Document.Layers[Idx];
        if (!Layer.Visible || Layer.Category == PatternStampLayerCategory::Folder) continue;

        const ColorRgba Tint = Layer.FillTint.ScaledAlpha(std::clamp(Layer.Opacity, 0.25f, 1.0f));
        if (Session.Mode == PatternStampWorkspaceMode::Pattern)
        {
            const float Step = 13.0f;
            const float Rad  = (Idx == 0u) ? 4.2f : 2.3f;
            for (int Ry = -1; Ry <= 1; ++Ry)
            {
                for (int Rx = -1; Rx <= 1; ++Rx)
                {
                    const float ShiftX = ((Ry & 1) != 0) ? (Step * 0.5f) : 0.0f;
                    const Point2D Pt { Cx + static_cast<float>(Rx) * Step + ShiftX, Cy + static_cast<float>(Ry) * Step };
                    if (Idx == 0u) Surface.FillCircle(Pt, Rad, Tint);
                    else Surface.StrokeCircle(Pt, Rad, Tint, 1.2f);
                }
            }
        }
        else
        {
            const float Rad = 17.0f - static_cast<float>(Idx) * 4.0f;
            if (Rad > 3.0f)
            {
                if (Layer.StrokeWidthPx > 0.5f && !Layer.FillEnabled)
                {
                    Surface.StrokeCircle({ Cx, Cy }, Rad, Layer.StrokeTint, 1.8f);
                }
                else
                {
                    Surface.FillCircle({ Cx, Cy }, Rad, Tint);
                }
            }
        }
    }
    Surface.PopClip();
    Surface.StrokeRectangle(ThumbRect, Tone.BorderStrong, 12.0f, 1.0f);
}

} // namespace

PatternStampInspectorPane::~PatternStampInspectorPane() noexcept
{
    WorkspaceAppearance::ReleasePanePointerOwnership(this);
}

void PatternStampInspectorPane::AttachSession(PatternStampWorkspaceSession* TargetSession) noexcept
{
    Session        = TargetSession;
    ScrollOffsetY  = 0.0f;
    ActiveSliderId = -1;
    OpenPopover    = ActivePopoverKind::None;
}

void PatternStampInspectorPane::PresentInImGuiPane(double DeltaSeconds, float ScaleFactor)
{
    const float  SafeScale = std::max(0.25f, ScaleFactor);
    const ImVec2 WinPos    = ImGui::GetWindowPos();
    const ImVec2 WinSize   = ImGui::GetWindowSize();
    const ImVec2 CursorPos = ImGui::GetCursorScreenPos();
    const float  TopY      = CursorPos.y - ImGui::GetStyle().WindowPadding.y;
    const float  PaneH     = std::max(1.0f, (WinPos.y + WinSize.y) - TopY);

    ImGui::InvisibleButton(
        "##PatternStampInspectorCanvas",
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

void PatternStampInspectorPane::Render(
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

    const bool PatternMode = (Session->Mode == PatternStampWorkspaceMode::Pattern);
    auto& Doc              = Session->Document;
    PatternStampLayerSpec* SelectedLayer = Session->InspectSelectedLayer();

    constexpr float PadX    = 14.0f;
    constexpr float FooterH = 66.0f;
    const float     InnerW  = std::max(80.0f, ShellRect.Width - PadX * 2.0f);
    const float     LeftX   = ShellRect.X + PadX;
    float           TopY    = ShellRect.Y + 8.0f;

    // 1. Fixed Inspector Header Bar (amber PatternTile / StampTool mark + Studio Title + Undo / Redo / Reset)
    const InspectorHeadingResult HeadOut = ConstructInspectorHeading(
        Surface,
        { LeftX, TopY, InnerW, 42.0f },
        PatternMode ? ControlIcon::PatternTile : ControlIcon::StampTool,
        PatternMode ? "Pattern Studio" : "Stamp Studio",
        PatternMode ? "SEAMLESS TILE" : "PBR DECAL",
        true,
        Session->QueryCanUndo(),
        Session->QueryCanRedo(),
        true,
        Input,
        Tone);
    if (HeadOut.UndoClicked)
    {
        (void)Session->ApplyUndo();
    }
    else if (HeadOut.RedoClicked)
    {
        (void)Session->ApplyRedo();
    }
    else if (HeadOut.ResetClicked)
    {
        Session->LoadPresetByIndex(Session->QueryActivePresetIndex());
    }
    TopY += 50.0f;

    // 2. Scrollable Body
    const Rectangle2D FooterRect { ShellRect.X + 1.0f, ShellRect.Bottom() - FooterH, ShellRect.Width - 2.0f, FooterH };
    const Rectangle2D BodyRect   { ShellRect.X + 4.0f, TopY, ShellRect.Width - 8.0f, std::max(60.0f, FooterRect.Y - TopY - 4.0f) };

    const Point2D Cursor = QueryInputCursor(Input);
    const bool PopoverBlocked = (OpenPopover != ActivePopoverKind::None);
    if (!PopoverBlocked && BodyRect.Contains(Cursor) && std::fabs(Input.WheelDeltaY) > 1.0e-4f)
    {
        ScrollOffsetY = std::clamp(ScrollOffsetY - Input.WheelDeltaY * 36.0f, 0.0f, MaxScrollY);
    }

    const float StartY = BodyRect.Y + 4.0f - ScrollOffsetY;
    float       Y      = StartY;

    Surface.PushClip(BodyRect);

    // 2a. Identity Block with live 52x52 miniature tile/decal preview + Starter preset picker
    char DocSub[72] {};
    std::snprintf(DocSub, sizeof(DocSub), "%u px · %zu layers · %zu mods",
        Session->ResolutionPx, Doc.Layers.size(), Session->CountEnabledModifiers());
    Rectangle2D ThumbBounds {};
    Rectangle2D StarterAnchor {};
    if (ConstructIdentityBlock(
            Surface,
            { LeftX, Y, InnerW, 56.0f },
            PatternMode ? "PATTERN" : "STAMP",
            Doc.DocumentName,
            DocSub,
            "Starter",
            OpenPopover == ActivePopoverKind::StarterLibrary,
            Input,
            Tone,
            &StarterAnchor,
            true,
            &ThumbBounds))
    {
        OpenPopover   = (OpenPopover == ActivePopoverKind::StarterLibrary) ? ActivePopoverKind::None : ActivePopoverKind::StarterLibrary;
        PopoverAnchor = StarterAnchor;
    }
    ConstructMiniatureDocumentPreview(Surface, ThumbBounds, *Session, Tone);
    Y += 66.0f;

    // 2b. Three Project Zero Meter Stat Tiles (Instances / Surface / Swatches)
    char InstVal[16] {};
    char InstCap[32] {};
    char SurfVal[16] {};
    char SwatchCnt[16] {};
    std::snprintf(InstVal, sizeof(InstVal), "%d", Session->InstancesEvaluated);
    std::snprintf(InstCap, sizeof(InstCap), "%zu visible · %.1f-%.1fx", Session->CountVisibleLayers(), Session->MinScaleReading, Session->MaxScaleReading);
    std::snprintf(SurfVal, sizeof(SurfVal), "%d", Session->SurfaceFractionPct);

    std::array<ColorRgba, 6u> DistinctTints {};
    uint32_t DistinctCount = 0u;
    auto PushDistinct = [&](ColorRgba C) noexcept
    {
        if (DistinctCount >= 6u) return;
        for (uint32_t K = 0u; K < DistinctCount; ++K)
        {
            if (DistinctTints[K] == C) return;
        }
        DistinctTints[DistinctCount++] = C;
    };
    for (const auto& L : Doc.Layers)
    {
        if (L.Category != PatternStampLayerCategory::Folder) PushDistinct(L.FillTint);
    }
    if (Doc.GroundEnabled) PushDistinct(Doc.GroundTintA);
    std::snprintf(SwatchCnt, sizeof(SwatchCnt), "%u", std::max(1u, DistinctCount));

    std::array<MeterStatTileSpec, 3u> StatTiles {};
    StatTiles[0].Label         = "Instances";
    StatTiles[0].Reading       = InstVal;
    StatTiles[0].Caption       = InstCap;
    StatTiles[0].PrimaryAccent = true;
    StatTiles[0].MeterKind     = MeterVariant::StackBar;
    StatTiles[0].StackCount    = static_cast<uint32_t>(std::min<std::size_t>(4u, std::max<std::size_t>(1u, Doc.Layers.size())));
    for (uint32_t S = 0u; S < StatTiles[0].StackCount; ++S)
    {
        const ColorRgba LCol = (S < Doc.Layers.size()) ? Doc.Layers[S].FillTint : Tone.Amber;
        StatTiles[0].StackParts[S] = { static_cast<float>(StatTiles[0].StackCount - S), LCol };
    }

    StatTiles[1].Label       = "Surface";
    StatTiles[1].Reading     = SurfVal;
    StatTiles[1].Unit        = "%";
    StatTiles[1].Caption     = PatternMode ? "Seamless tile" : "Clean decal";
    StatTiles[1].MeterKind   = MeterVariant::FillBar;
    StatTiles[1].MeterRatio  = std::clamp(static_cast<float>(Session->SurfaceFractionPct) / 100.0f, 0.0f, 1.0f);
    StatTiles[1].MeterColour = (DistinctCount > 0u) ? DistinctTints[0] : Tone.PrimaryFill;

    StatTiles[2].Label       = "Swatches";
    StatTiles[2].Reading     = SwatchCnt;
    StatTiles[2].Unit        = (DistinctCount == 1u) ? "colour" : "colours";
    StatTiles[2].Caption     = "Authored tints";
    StatTiles[2].MeterKind   = MeterVariant::SwatchLine;
    StatTiles[2].Swatches    = DistinctTints;
    StatTiles[2].SwatchCount = DistinctCount;

    ConstructMeterStatTrio(Surface, { LeftX, Y, InnerW, 74.0f }, StatTiles, Tone);
    Y += 86.0f;

    bool DocumentMutated = false;

    // 2c. Selected Layer Accordion Property Rows (Shape / Fill / Outline / Placement / Blend / Surface PBR / Mask)
    if (SelectedLayer != nullptr)
    {
        WorkspaceAppearance::ConstructWeightedText(
            Surface, FontWeightClass::Medium, { LeftX, Y + 2.0f }, 12.0f, Tone.TextPrimary, SelectedLayer->Name);
        WorkspaceAppearance::ConstructRightAlignedText(
            Surface, FontWeightClass::Regular, { LeftX + InnerW, Y + 3.0f }, 9.2f, Tone.TextMuted, "Selected layer");
        Y += 24.0f;

        const auto& ShapeLabels  = PatternStampWorkspaceSession::InspectMotifShapeLabels();
        const auto& FinishLabels = PatternStampWorkspaceSession::InspectFinishLabels();
        const std::size_t ShapeIdx  = static_cast<std::size_t>(SelectedLayer->MotifShape);
        const std::size_t FinishIdx = static_cast<std::size_t>(SelectedLayer->FinishPreset);
        const char* ShapeName  = (ShapeIdx < ShapeLabels.size()) ? ShapeLabels[ShapeIdx] : "Circle";
        const char* FinishName = (FinishIdx < FinishLabels.size()) ? FinishLabels[FinishIdx] : "Printed Dye";

        char FillHex[16] {};
        char StrokeRead[24] {};
        char PlaceRead[32] {};
        char BlendRead[32] {};
        char SurfRead[32] {};
        std::snprintf(FillHex, sizeof(FillHex), "#%02x%02x%02x", SelectedLayer->FillTint.Red, SelectedLayer->FillTint.Green, SelectedLayer->FillTint.Blue);
        if (SelectedLayer->StrokeWidthPx > 0.1f)
        {
            std::snprintf(StrokeRead, sizeof(StrokeRead), "%.1f px", SelectedLayer->StrokeWidthPx);
        }
        else
        {
            std::snprintf(StrokeRead, sizeof(StrokeRead), "None");
        }
        std::snprintf(PlaceRead, sizeof(PlaceRead), "%.0fx%.0f", SelectedLayer->WidthPx, SelectedLayer->HeightPx);
        const std::string_view BlendLbl = QueryBlendLabel(SelectedLayer->BlendMode);
        std::snprintf(BlendRead, sizeof(BlendRead), "%.*s %.0f%%", static_cast<int>(BlendLbl.size()), BlendLbl.data(), SelectedLayer->Opacity * 100.0f);
        std::snprintf(SurfRead, sizeof(SurfRead), "%zu channels", SelectedLayer->Channels.size());

        auto RenderPropertyAccordion = [&](int32_t RowIdx, const AccordionRowSpec& Spec, float EditorHeight, auto&& BuildBody)
        {
            const bool Expanded = (OpenPropertyRow == RowIdx);
            constexpr float HeadH = 38.0f;
            const float TotalH = HeadH + (Expanded ? EditorHeight : 0.0f);
            const Rectangle2D BlockRect { LeftX, Y, InnerW, TotalH };

            if (Expanded)
            {
                Surface.FillRectangle(BlockRect, Tone.InsetSurface, 14.0f);
                Surface.StrokeRectangle(BlockRect, Tone.OpenRowStroke, 14.0f, 1.0f);
            }
            if (ConstructAccordionRowHeader(Surface, { LeftX, Y, InnerW, HeadH }, Spec, Expanded, Input, Tone) && !PopoverBlocked)
            {
                OpenPropertyRow = Expanded ? -1 : RowIdx;
            }
            if (Expanded)
            {
                const float EdX = LeftX + 10.0f;
                const float EdW = InnerW - 20.0f;
                const float EdY = Y + HeadH + 4.0f;
                Surface.StrokeLine({ EdX, EdY - 4.0f }, { EdX + EdW, EdY - 4.0f }, Tone.Hairline, 1.0f);
                BuildBody(EdX, EdY, EdW);
            }
            Y += TotalH + 4.0f;
        };

        // Row 0: Shape
        AccordionRowSpec ShapeSpec {};
        ShapeSpec.Title    = "Shape";
        ShapeSpec.Subtitle = "Vector motif geometry";
        ShapeSpec.Readout  = ShapeName;
        ShapeSpec.LeadIcon = ControlIcon::ShapeHexagon;
        RenderPropertyAccordion(0, ShapeSpec, 134.0f, [&](float EdX, float EdY, float EdW)
        {
            const Rectangle2D ShapeDrop { EdX, EdY + 2.0f, EdW, 28.0f };
            const bool Hover = ShapeDrop.Contains(Cursor);
            Surface.FillRectangle(ShapeDrop, Hover ? Tone.RowSelected : Tone.RecessedWell, 10.0f);
            Surface.StrokeRectangle(ShapeDrop, Tone.BorderSubtle, 10.0f, 1.0f);
            WorkspaceAppearance::ConstructWeightedText(Surface, FontWeightClass::Medium, { ShapeDrop.X + 10.0f, ShapeDrop.Center().Y - 5.2f }, 10.0f, Tone.TextPrimary, ShapeName);
            ConstructChevronMark(Surface, { ShapeDrop.Right() - 12.0f, ShapeDrop.Center().Y }, true, 3.0f, Tone.TextMuted);
            if (Hover && Input.PrimaryPointer.Pressed && !PopoverBlocked)
            {
                OpenPopover   = ActivePopoverKind::MotifShapeList;
                PopoverAnchor = ShapeDrop;
            }
            if (ConstructStudioKnobSlider(Surface, { EdX, EdY + 38.0f, EdW, 42.0f }, 401, ActiveSliderId, "Motif width", SelectedLayer->WidthPx, 12.0f, 420.0f, "px", 0, Input, Tone))
            {
                SelectedLayer->HeightPx = SelectedLayer->WidthPx;
                DocumentMutated = true;
            }
            if (ConstructStudioKnobSlider(Surface, { EdX, EdY + 84.0f, EdW, 42.0f }, 402, ActiveSliderId, "Rotation", SelectedLayer->RotationDeg, -180.0f, 180.0f, "deg", 0, Input, Tone))
            {
                DocumentMutated = true;
            }
        });

        // Row 1: Fill
        AccordionRowSpec FillSpec {};
        FillSpec.Title        = "Fill";
        FillSpec.Subtitle     = "Solid or gradient fill";
        FillSpec.Readout      = SelectedLayer->FillEnabled ? FillHex : "Off";
        FillSpec.SolidSwatch  = true;
        FillSpec.SwatchColour = SelectedLayer->FillTint;
        RenderPropertyAccordion(1, FillSpec, 194.0f, [&](float EdX, float EdY, float EdW)
        {
            constexpr std::array<std::string_view, 4u> Modes {{ "Solid", "Linear", "Radial", "Point" }};
            const int32_t PickedMode = ConstructSegmentedCapsuleBar(Surface, { EdX, EdY + 2.0f, EdW, 26.0f }, Modes, static_cast<int32_t>(SelectedLayer->FillSource), Input, Tone);
            if (PickedMode >= 0 && !PopoverBlocked)
            {
                SelectedLayer->FillSource  = static_cast<PatternStampFillSourceMode>(PickedMode);
                SelectedLayer->FillEnabled = true;
                DocumentMutated            = true;
            }
            EdY += 34.0f;

            // Quick studio swatch dots
            const float DotD = 18.0f;
            const float GapD = 6.0f;
            for (std::size_t S = 0u; S < StudioSwatchRow.size(); ++S)
            {
                const Rectangle2D DRect { EdX + static_cast<float>(S) * (DotD + GapD), EdY + 2.0f, DotD, DotD };
                Surface.FillRectangle(DRect, StudioSwatchRow[S], 9.0f);
                Surface.StrokeRectangle(DRect, (SelectedLayer->FillTint == StudioSwatchRow[S]) ? Tone.Amber : Tone.BorderStrong, 9.0f, 1.0f);
                if (!PopoverBlocked && DRect.Contains(Cursor) && Input.PrimaryPointer.Pressed)
                {
                    SelectedLayer->FillTint    = StudioSwatchRow[S];
                    SelectedLayer->FillEnabled = true;
                    DocumentMutated            = true;
                }
            }
            EdY += 28.0f;

            float R = static_cast<float>(SelectedLayer->FillTint.Red);
            float G = static_cast<float>(SelectedLayer->FillTint.Green);
            float B = static_cast<float>(SelectedLayer->FillTint.Blue);
            const bool ChR = ConstructStudioKnobSlider(Surface, { EdX, EdY, EdW, 40.0f }, 410, ActiveSliderId, "Red", R, 0.0f, 255.0f, "", 0, Input, Tone);
            const bool ChG = ConstructStudioKnobSlider(Surface, { EdX, EdY + 42.0f, EdW, 40.0f }, 411, ActiveSliderId, "Green", G, 0.0f, 255.0f, "", 0, Input, Tone);
            const bool ChB = ConstructStudioKnobSlider(Surface, { EdX, EdY + 84.0f, EdW, 40.0f }, 412, ActiveSliderId, "Blue", B, 0.0f, 255.0f, "", 0, Input, Tone);
            if (ChR || ChG || ChB)
            {
                SelectedLayer->FillTint.Red   = static_cast<uint8_t>(std::clamp(std::lround(R), 0l, 255l));
                SelectedLayer->FillTint.Green = static_cast<uint8_t>(std::clamp(std::lround(G), 0l, 255l));
                SelectedLayer->FillTint.Blue  = static_cast<uint8_t>(std::clamp(std::lround(B), 0l, 255l));
                SelectedLayer->FillEnabled    = true;
                DocumentMutated               = true;
            }
        });

        // Row 2: Outline
        AccordionRowSpec OutSpec {};
        OutSpec.Title        = "Outline";
        OutSpec.Subtitle     = "Vector stroke weight & tint";
        OutSpec.Readout      = StrokeRead;
        OutSpec.SolidSwatch  = (SelectedLayer->StrokeWidthPx > 0.1f);
        OutSpec.SwatchColour = SelectedLayer->StrokeTint;
        OutSpec.LeadIcon     = ControlIcon::VectorCurve;
        RenderPropertyAccordion(2, OutSpec, 82.0f, [&](float EdX, float EdY, float EdW)
        {
            if (ConstructStudioKnobSlider(Surface, { EdX, EdY + 2.0f, EdW, 42.0f }, 420, ActiveSliderId, "Stroke width", SelectedLayer->StrokeWidthPx, 0.0f, 28.0f, "px", 1, Input, Tone))
            {
                DocumentMutated = true;
            }
            const float DotD = 18.0f;
            for (std::size_t S = 0u; S < StudioSwatchRow.size(); ++S)
            {
                const Rectangle2D DRect { EdX + static_cast<float>(S) * (DotD + 6.0f), EdY + 50.0f, DotD, DotD };
                Surface.FillRectangle(DRect, StudioSwatchRow[S], 9.0f);
                Surface.StrokeRectangle(DRect, Tone.BorderStrong, 9.0f, 1.0f);
                if (!PopoverBlocked && DRect.Contains(Cursor) && Input.PrimaryPointer.Pressed)
                {
                    SelectedLayer->StrokeTint = StudioSwatchRow[S];
                    DocumentMutated           = true;
                }
            }
        });

        // Row 3: Placement
        AccordionRowSpec PlaceSpec {};
        PlaceSpec.Title    = "Placement";
        PlaceSpec.Subtitle = "Tile anchor & transform";
        PlaceSpec.Readout  = PlaceRead;
        PlaceSpec.LeadIcon = ControlIcon::MoveArrows;
        RenderPropertyAccordion(3, PlaceSpec, 96.0f, [&](float EdX, float EdY, float EdW)
        {
            if (ConstructStudioKnobSlider(Surface, { EdX, EdY + 2.0f, EdW, 42.0f }, 430, ActiveSliderId, "Anchor X", SelectedLayer->CenterX, 0.0f, 512.0f, "px", 0, Input, Tone))
            {
                DocumentMutated = true;
            }
            if (ConstructStudioKnobSlider(Surface, { EdX, EdY + 48.0f, EdW, 42.0f }, 431, ActiveSliderId, "Anchor Y", SelectedLayer->CenterY, 0.0f, 512.0f, "px", 0, Input, Tone))
            {
                DocumentMutated = true;
            }
        });

        // Row 4: Blend & Opacity
        AccordionRowSpec BlendSpec {};
        BlendSpec.Title           = "Blend & opacity";
        BlendSpec.Subtitle        = "Layer compositing mode";
        BlendSpec.Readout         = BlendRead;
        BlendSpec.ScalarBarSwatch = true;
        BlendSpec.ScalarFill      = SelectedLayer->Opacity;
        RenderPropertyAccordion(4, BlendSpec, 84.0f, [&](float EdX, float EdY, float EdW)
        {
            constexpr std::array<std::string_view, 4u> Blends {{ "Normal", "Mult", "Screen", "Overlay" }};
            const int32_t Picked = ConstructSegmentedCapsuleBar(Surface, { EdX, EdY + 2.0f, EdW, 26.0f }, Blends, std::min<int32_t>(3, SelectedLayer->BlendMode), Input, Tone);
            if (Picked >= 0 && !PopoverBlocked)
            {
                SelectedLayer->BlendMode = static_cast<uint8_t>(Picked);
                DocumentMutated          = true;
            }
            float OpPct = SelectedLayer->Opacity * 100.0f;
            if (ConstructStudioKnobSlider(Surface, { EdX, EdY + 36.0f, EdW, 42.0f }, 440, ActiveSliderId, "Opacity", OpPct, 0.0f, 100.0f, "%", 0, Input, Tone))
            {
                SelectedLayer->Opacity = std::clamp(OpPct / 100.0f, 0.0f, 1.0f);
                DocumentMutated        = true;
            }
        });

        // Row 5: Surface (PBR Channels)
        const float PbrBodyH = 44.0f + static_cast<float>(SelectedLayer->Channels.size()) * 46.0f + 34.0f;
        AccordionRowSpec SurfSpec {};
        SurfSpec.Title      = "Surface (PBR)";
        SurfSpec.Subtitle   = FinishName;
        SurfSpec.Readout    = SurfRead;
        SurfSpec.DrivenWarm = !SelectedLayer->Channels.empty();
        SurfSpec.LeadIcon   = ControlIcon::MaterialSphere;
        RenderPropertyAccordion(5, SurfSpec, PbrBodyH, [&](float EdX, float EdY, float EdW)
        {
            // Finish preset trigger + '+ Channel' button
            const float HalfW = (EdW - 6.0f) * 0.5f;
            const Rectangle2D FinBtn { EdX, EdY + 2.0f, HalfW, 26.0f };
            const Rectangle2D AddChBtn { FinBtn.Right() + 6.0f, EdY + 2.0f, HalfW, 26.0f };

            Surface.FillRectangle(FinBtn, FinBtn.Contains(Cursor) ? Tone.RowSelected : Tone.RecessedWell, 10.0f);
            Surface.StrokeRectangle(FinBtn, Tone.BorderSubtle, 10.0f, 1.0f);
            ConstructCenteredWeightedText(Surface, FontWeightClass::Medium, FinBtn, 9.4f, Tone.TextSecondary, FinishName);
            if (FinBtn.Contains(Cursor) && Input.PrimaryPointer.Pressed && !PopoverBlocked)
            {
                OpenPopover   = ActivePopoverKind::FinishPresetList;
                PopoverAnchor = FinBtn;
            }

            Surface.FillRectangle(AddChBtn, AddChBtn.Contains(Cursor) ? Tone.AmberWash : Tone.TileSurface, 10.0f);
            Surface.StrokeRectangle(AddChBtn, AddChBtn.Contains(Cursor) ? Tone.Amber : Tone.BorderSubtle, 10.0f, 1.0f);
            ConstructCenteredWeightedText(Surface, FontWeightClass::Medium, AddChBtn, 9.4f, Tone.Amber, "+ Channel");
            if (AddChBtn.Contains(Cursor) && Input.PrimaryPointer.Pressed && !PopoverBlocked)
            {
                OpenPopover   = ActivePopoverKind::AddChannelList;
                PopoverAnchor = AddChBtn;
            }
            EdY += 36.0f;

            for (std::size_t C = 0u; C < SelectedLayer->Channels.size(); ++C)
            {
                auto& Ch = SelectedLayer->Channels[C];
                const auto& Desc = PatternStampWorkspaceSession::QueryChannelDescriptor(Ch.Key);
                if (ConstructStudioKnobSlider(
                        Surface,
                        { EdX, EdY, EdW, 42.0f },
                        500 + static_cast<int32_t>(C),
                        ActiveSliderId,
                        Desc.Label,
                        Ch.ScalarReading,
                        Desc.MinScalar,
                        Desc.MaxScalar,
                        "",
                        2,
                        Input,
                        Tone))
                {
                    DocumentMutated = true;
                }
                EdY += 46.0f;
            }
        });

        // Row 6: Mask
        AccordionRowSpec MaskSpec {};
        MaskSpec.Title      = "Mask";
        MaskSpec.Subtitle   = "Procedural layer cutout";
        MaskSpec.Readout    = SelectedLayer->Mask.Enabled ? "Active" : "Off";
        MaskSpec.DrivenWarm = SelectedLayer->Mask.Enabled;
        MaskSpec.LeadIcon   = ControlIcon::MaskCutout;
        RenderPropertyAccordion(6, MaskSpec, 128.0f, [&](float EdX, float EdY, float EdW)
        {
            if (ConstructStudioToggleRow(Surface, { EdX, EdY, EdW, 30.0f }, "Enable mask", "Clip motif alpha with procedural field", SelectedLayer->Mask.Enabled, Input, Tone))
            {
                DocumentMutated = true;
            }
            EdY += 34.0f;
            constexpr std::array<std::string_view, 3u> MaskModes {{ "Noise", "Radial", "Stripe" }};
            const int32_t PickedM = ConstructSegmentedCapsuleBar(Surface, { EdX, EdY, EdW, 26.0f }, MaskModes, static_cast<int32_t>(SelectedLayer->Mask.Mode), Input, Tone);
            if (PickedM >= 0 && !PopoverBlocked)
            {
                SelectedLayer->Mask.Mode = static_cast<PatternStampMaskMode>(PickedM);
                DocumentMutated          = true;
            }
            EdY += 34.0f;
            if (ConstructStudioKnobSlider(Surface, { EdX, EdY, EdW, 42.0f }, 460, ActiveSliderId, "Threshold", SelectedLayer->Mask.Threshold, 0.0f, 1.0f, "", 2, Input, Tone))
            {
                DocumentMutated = true;
            }
        });

        Y += 10.0f;

        // 2d. Modifiers Section + '+ Modifier' Capsule + Quiet Expandable Modifier Rows
        WorkspaceAppearance::ConstructWeightedText(
            Surface, FontWeightClass::Medium, { LeftX, Y + 4.0f }, 12.0f, Tone.TextPrimary, "Modifiers");

        const Rectangle2D AddModBtn { LeftX + InnerW - 92.0f, Y, 92.0f, 24.0f };
        const bool AddModHover = AddModBtn.Contains(Cursor);
        Surface.FillRectangle(AddModBtn, AddModHover ? Tone.RowSelected : Tone.TileSurface, 12.0f);
        Surface.StrokeRectangle(AddModBtn, AddModHover ? Tone.BorderStrong : Tone.BorderSubtle, 12.0f, 1.0f);
        ConstructCenteredWeightedText(Surface, FontWeightClass::Medium, AddModBtn, 9.4f, Tone.TextSecondary, "+ Modifier");
        if (AddModHover && Input.PrimaryPointer.Pressed && !PopoverBlocked)
        {
            OpenPopover   = ActivePopoverKind::AddModifierList;
            PopoverAnchor = AddModBtn;
        }
        Y += 32.0f;

        const auto& ModLabels = PatternStampWorkspaceSession::InspectModifierLabels();
        int32_t RemoveModIdx  = -1;

        for (std::size_t MIdx = 0u; MIdx < SelectedLayer->Modifiers.size(); ++MIdx)
        {
            auto& Mod = SelectedLayer->Modifiers[MIdx];
            const std::size_t CatOrd = static_cast<std::size_t>(Mod.Category);
            const char* ModTitle = (CatOrd < ModLabels.size()) ? ModLabels[CatOrd] : "Modifier";
            const bool Expanded  = (OpenModifierIndex == static_cast<int32_t>(MIdx));

            char ModRead[32] {};
            switch (Mod.Category)
            {
                case PatternStampModifierCategory::GridArray:
                    std::snprintf(ModRead, sizeof(ModRead), "%dx%d", Mod.Columns, Mod.Rows);
                    break;
                case PatternStampModifierCategory::RadialArray:
                    std::snprintf(ModRead, sizeof(ModRead), "%d pts", Mod.RadialCount);
                    break;
                case PatternStampModifierCategory::NoiseScale:
                    std::snprintf(ModRead, sizeof(ModRead), "%.1f-%.1fx", Mod.MinScale, Mod.MaxScale);
                    break;
                default:
                    std::snprintf(ModRead, sizeof(ModRead), Mod.Enabled ? "On" : "Off");
                    break;
            }

            AccordionRowSpec MSpec {};
            MSpec.Title      = ModTitle;
            MSpec.Subtitle   = Mod.Enabled ? "Active modifier" : "Bypassed";
            MSpec.Readout    = ModRead;
            MSpec.DrivenWarm = Mod.Enabled;
            MSpec.LeadIcon   = ControlIcon::ModifierStar;

            constexpr float HeadH  = 38.0f;
            const float     EdH    = Expanded ? 128.0f : 0.0f;
            const Rectangle2D MBox { LeftX, Y, InnerW, HeadH + EdH };
            if (Expanded)
            {
                Surface.FillRectangle(MBox, Tone.InsetSurface, 14.0f);
                Surface.StrokeRectangle(MBox, Tone.OpenRowStroke, 14.0f, 1.0f);
            }
            if (ConstructAccordionRowHeader(Surface, { LeftX, Y, InnerW, HeadH }, MSpec, Expanded, Input, Tone) && !PopoverBlocked)
            {
                OpenModifierIndex = Expanded ? -1 : static_cast<int32_t>(MIdx);
            }

            if (Expanded)
            {
                const float EdX = LeftX + 10.0f;
                const float EdW = InnerW - 20.0f;
                float       EdY = Y + HeadH + 4.0f;
                Surface.StrokeLine({ EdX, EdY - 4.0f }, { EdX + EdW, EdY - 4.0f }, Tone.Hairline, 1.0f);

                if (ConstructStudioToggleRow(Surface, { EdX, EdY, EdW, 30.0f }, "Modifier active", "", Mod.Enabled, Input, Tone))
                {
                    DocumentMutated = true;
                }
                EdY += 34.0f;

                if (Mod.Category == PatternStampModifierCategory::GridArray)
                {
                    float Cols = static_cast<float>(Mod.Columns);
                    if (ConstructStudioKnobSlider(Surface, { EdX, EdY, EdW, 42.0f }, 600 + static_cast<int32_t>(MIdx) * 2, ActiveSliderId, "Columns", Cols, 2.0f, 24.0f, "", 0, Input, Tone))
                    {
                        Mod.Columns     = static_cast<int32_t>(std::lround(Cols));
                        Mod.Rows        = Mod.Columns;
                        DocumentMutated = true;
                    }
                }
                else if (Mod.Category == PatternStampModifierCategory::RadialArray)
                {
                    float Pts = static_cast<float>(Mod.RadialCount);
                    if (ConstructStudioKnobSlider(Surface, { EdX, EdY, EdW, 42.0f }, 600 + static_cast<int32_t>(MIdx) * 2, ActiveSliderId, "Radial count", Pts, 3.0f, 32.0f, "", 0, Input, Tone))
                    {
                        Mod.RadialCount = static_cast<int32_t>(std::lround(Pts));
                        DocumentMutated = true;
                    }
                }
                else
                {
                    if (ConstructStudioKnobSlider(Surface, { EdX, EdY, EdW, 42.0f }, 600 + static_cast<int32_t>(MIdx) * 2, ActiveSliderId, "Frequency", Mod.Frequency, 0.5f, 12.0f, "x", 1, Input, Tone))
                    {
                        DocumentMutated = true;
                    }
                }
                EdY += 48.0f;

                const Rectangle2D DelBtn { EdX, EdY, 84.0f, 22.0f };
                const bool DelHover = DelBtn.Contains(Cursor);
                Surface.FillRectangle(DelBtn, DelHover ? Tone.CrimsonWash : Tone.RecessedWell, 11.0f);
                Surface.StrokeRectangle(DelBtn, DelHover ? Tone.Crimson : Tone.Hairline, 11.0f, 1.0f);
                ConstructCenteredWeightedText(Surface, FontWeightClass::Medium, DelBtn, 9.0f, Tone.Crimson, "Remove");
                if (DelHover && Input.PrimaryPointer.Pressed && !PopoverBlocked)
                {
                    RemoveModIdx = static_cast<int32_t>(MIdx);
                }
            }
            Y += HeadH + EdH + 4.0f;
        }

        if (RemoveModIdx >= 0)
        {
            (void)Session->RemoveModifierFromSelectedLayer(static_cast<std::size_t>(RemoveModIdx));
            OpenModifierIndex = -1;
        }
    }

    Y += 8.0f;

    // 2e. Collapsible Document Fold (Ground toggle + Ground swatches + Resolution segmented bar)
    const float DocBodyH = DocumentFoldExpanded ? 114.0f : 0.0f;
    const Rectangle2D DocFold { LeftX, Y, InnerW, 38.0f + DocBodyH };
    Surface.FillRectangle(DocFold, Tone.FoldSurface, 15.0f);
    Surface.StrokeRectangle(DocFold, Tone.BorderSubtle, 15.0f, 1.0f);

    char DocFoldSum[32] {};
    std::snprintf(DocFoldSum, sizeof(DocFoldSum), "%u px · %s", Session->ResolutionPx, Doc.GroundEnabled ? "ground" : "alpha");
    if (ConstructCollapsibleFoldHeader(Surface, { DocFold.X, DocFold.Y, DocFold.Width, 38.0f }, "Document", DocFoldSum, DocumentFoldExpanded, Input, Tone) && !PopoverBlocked)
    {
        DocumentFoldExpanded = !DocumentFoldExpanded;
    }

    if (DocumentFoldExpanded)
    {
        const float DX = DocFold.X + 12.0f;
        const float DW = DocFold.Width - 24.0f;
        float       DY = DocFold.Y + 42.0f;

        if (ConstructStudioToggleRow(Surface, { DX, DY, DW, 32.0f }, "Canvas ground", "Fill background behind motifs", Doc.GroundEnabled, Input, Tone))
        {
            DocumentMutated = true;
        }
        DY += 38.0f;

        constexpr std::array<std::string_view, 4u> ResLabels {{ "256", "512", "1K", "2K" }};
        constexpr std::array<uint32_t, 4u> ResOptions { 256u, 512u, 1024u, 2048u };
        int32_t CurRes = 2;
        for (int32_t I = 0; I < 4; ++I)
        {
            if (ResOptions[static_cast<std::size_t>(I)] == Session->ResolutionPx) CurRes = I;
        }
        const int32_t PickedRes = ConstructSegmentedCapsuleBar(Surface, { DX, DY, DW, 26.0f }, ResLabels, CurRes, Input, Tone);
        if (PickedRes >= 0 && !PopoverBlocked)
        {
            Session->AssignResolution(ResOptions[static_cast<std::size_t>(PickedRes)]);
            DocumentMutated = true;
        }
    }
    Y += DocFold.Height + 14.0f;

    if (DocumentMutated)
    {
        Session->RecomputeSummaryMetrics();
        Session->BumpRevision();
    }

    Surface.PopClip();
    const float TotalHeight = (Y - StartY) + 6.0f;
    MaxScrollY              = std::max(0.0f, TotalHeight - BodyRect.Height);
    ScrollOffsetY           = std::clamp(ScrollOffsetY, 0.0f, MaxScrollY);

    // 3. Fixed Footer Bar (.svg white primary CTA + Duplicate + Reset + status indicator)
    const std::string_view StatusText = Session->StatusNotice.empty()
        ? (PatternMode ? "Seamless tile ready" : "PBR decal ready")
        : std::string_view(Session->StatusNotice);
    const InspectorFooterResult FootOut = ConstructInspectorFooterBar(
        Surface,
        FooterRect,
        ".svg",
        "Duplicate",
        "Reset",
        true,
        StatusText,
        Input,
        Tone);
    if (FootOut.PrimaryClicked)
    {
        Session->StatusNotice = "SVG document serialized";
    }
    else if (FootOut.SecondaryAClicked)
    {
        (void)Session->DuplicateSelectedLayer();
    }
    else if (FootOut.SecondaryBClicked)
    {
        Session->LoadPresetByIndex(Session->QueryActivePresetIndex());
    }

    // 4. Overlay Project Zero Popovers
    if (OpenPopover != ActivePopoverKind::None)
    {
        std::vector<PopoverOptionSpec> Options {};
        std::string_view Heading {};
        int32_t SelectedIdx = -1;

        if (OpenPopover == ActivePopoverKind::StarterLibrary)
        {
            Heading = PatternMode ? "PATTERN STARTERS" : "STAMP STARTERS";
            SelectedIdx = static_cast<int32_t>(Session->QueryActivePresetIndex());
            for (const auto& P : Session->QueryPresetDirectory())
            {
                Options.push_back(PopoverOptionSpec{ P.Title, P.Subtitle, P.SwatchA });
            }
        }
        else if (OpenPopover == ActivePopoverKind::MotifShapeList && SelectedLayer != nullptr)
        {
            Heading = "MOTIF SHAPE";
            SelectedIdx = static_cast<int32_t>(SelectedLayer->MotifShape);
            for (const char* Lbl : PatternStampWorkspaceSession::InspectMotifShapeLabels())
            {
                Options.push_back(PopoverOptionSpec{ Lbl, "", {} });
            }
        }
        else if (OpenPopover == ActivePopoverKind::FinishPresetList && SelectedLayer != nullptr)
        {
            Heading = "SURFACE FINISH";
            SelectedIdx = static_cast<int32_t>(SelectedLayer->FinishPreset);
            for (const char* Lbl : PatternStampWorkspaceSession::InspectFinishLabels())
            {
                Options.push_back(PopoverOptionSpec{ Lbl, "", {} });
            }
        }
        else if (OpenPopover == ActivePopoverKind::AddChannelList)
        {
            Heading = "ADD PBR CHANNEL";
            for (const auto Key : Session->QueryAvailableChannelsForSelectedLayer())
            {
                const auto& Desc = PatternStampWorkspaceSession::QueryChannelDescriptor(Key);
                Options.push_back(PopoverOptionSpec{ Desc.Label, Desc.Subtitle, Desc.ColorChannel ? Desc.DefaultTint : ColorRgba{} });
            }
        }
        else if (OpenPopover == ActivePopoverKind::AddModifierList)
        {
            Heading = "ADD MODIFIER";
            for (const char* Lbl : PatternStampWorkspaceSession::InspectModifierLabels())
            {
                Options.push_back(PopoverOptionSpec{ Lbl, "", Tone.Amber });
            }
        }

        bool Dismissed = false;
        const int32_t Picked = ConstructStudioPopoverList(
            Surface, PopoverAnchor, ShellRect, Heading, Options, SelectedIdx, Input, Tone, Dismissed);

        if (Picked >= 0)
        {
            if (OpenPopover == ActivePopoverKind::StarterLibrary)
            {
                Session->LoadPresetByIndex(static_cast<std::size_t>(Picked));
            }
            else if (OpenPopover == ActivePopoverKind::MotifShapeList && SelectedLayer != nullptr)
            {
                SelectedLayer->MotifShape = static_cast<PatternStampMotifShape>(Picked);
                Session->RecomputeSummaryMetrics();
                Session->BumpRevision();
            }
            else if (OpenPopover == ActivePopoverKind::FinishPresetList)
            {
                Session->ApplyFinishPresetToSelectedLayer(static_cast<PatternStampFinishPreset>(Picked));
            }
            else if (OpenPopover == ActivePopoverKind::AddChannelList)
            {
                const auto Avail = Session->QueryAvailableChannelsForSelectedLayer();
                if (static_cast<std::size_t>(Picked) < Avail.size())
                {
                    (void)Session->AddChannelToSelectedLayer(Avail[static_cast<std::size_t>(Picked)]);
                }
            }
            else if (OpenPopover == ActivePopoverKind::AddModifierList)
            {
                (void)Session->AddModifierToSelectedLayer(static_cast<PatternStampModifierCategory>(Picked));
            }
            OpenPopover = ActivePopoverKind::None;
        }
        else if (Dismissed)
        {
            OpenPopover = ActivePopoverKind::None;
        }
    }
}

} // namespace Frontier::Workspaces
