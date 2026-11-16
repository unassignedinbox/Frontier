/*============================================================================================================================================
                                                            BAKINGTARGETSPANE.CPP
============================================================================================================================================*/
// 🧩 Implementation of BakingTargetsPane in Project Zero Queue Outliner styling wired live to BakingWorkspaceSession

//------------------------------------------------------------------------------------------------------------------------
//                                                        INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Engine/Workspaces/Texturing/BakingTargetsPane.h"
#include "Engine/Workspaces/Texturing/StudioSurfaceKit.h"

#include <imgui.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <string_view>

namespace Frontier::Workspaces
{

using namespace StudioSurface;

namespace
{

[[nodiscard]] bool SubstringMatchFold(std::string_view Haystack, std::string_view Needle) noexcept
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

[[nodiscard]] ColorRgba ResolveGroupAccent(BakerTargetGroup Group, const StudioToneSet& Tone) noexcept
{
    switch (Group)
    {
        case BakerTargetGroup::Normals:         return { 124u, 143u, 255u, 255u };
        case BakerTargetGroup::Surface:         return Tone.Amber;
        case BakerTargetGroup::DiagnosticMasks: return Tone.Green;
        case BakerTargetGroup::VolumeDistance:  return Tone.Violet;
        default:                                return Tone.TextMuted;
    }
}

[[nodiscard]] ControlIcon ResolveGroupIcon(BakerTargetGroup Group) noexcept
{
    switch (Group)
    {
        case BakerTargetGroup::Normals:         return ControlIcon::SparklesAntiAliasing;
        case BakerTargetGroup::Surface:         return ControlIcon::MaterialSphere;
        case BakerTargetGroup::DiagnosticMasks: return ControlIcon::MaskCutout;
        case BakerTargetGroup::VolumeDistance:  return ControlIcon::VolumeBoxes;
        default:                                return ControlIcon::LayersSlabs;
    }
}

[[nodiscard]] float EstimateTargetMegabytes(const BakingWorkspaceSession& Session, BakerTargetId Target) noexcept
{
    if (Target == BakerTargetId::VolumeSdf)
    {
        const float Dim = static_cast<float>(Session.VolumeGridDim);
        return (Dim * Dim * Dim * 4.0f) / (1024.0f * 1024.0f);
    }
    const float Side = static_cast<float>(Session.TargetResolutionPx);
    return (Side * Side * 4.0f) / (1024.0f * 1024.0f);
}

[[nodiscard]] double EstimateTotalRays(const BakingWorkspaceSession& Session) noexcept
{
    const double CoveredTexels = static_cast<double>(Session.TargetResolutionPx) * static_cast<double>(Session.TargetResolutionPx) * 0.72;
    double Total = 0.0;
    for (std::size_t Idx = 0u; Idx < BakingWorkspaceSession::TargetCount; ++Idx)
    {
        if (!Session.SelectedTargets[Idx]) continue;
        const auto Target = static_cast<BakerTargetId>(Idx);
        if (Target == BakerTargetId::VolumeSdf)
        {
            const double D = static_cast<double>(Session.VolumeGridDim);
            Total += D * D * D * 6.0;
        }
        else if (Target == BakerTargetId::AmbientOcclusion || Target == BakerTargetId::SecondaryAo ||
                 Target == BakerTargetId::BentNormal || Target == BakerTargetId::Thickness)
        {
            Total += CoveredTexels * static_cast<double>(std::max(1, Session.AoSampleCount));
        }
        else
        {
            Total += CoveredTexels;
        }
    }
    return Total;
}

void FormatCompactCount(double Count, char* Buffer, std::size_t Capacity) noexcept
{
    if (Count >= 1.0e6)
    {
        std::snprintf(Buffer, Capacity, "%.1fM", Count / 1.0e6);
    }
    else if (Count >= 1.0e3)
    {
        std::snprintf(Buffer, Capacity, "%.0fk", Count / 1.0e3);
    }
    else
    {
        std::snprintf(Buffer, Capacity, "%.0f", Count);
    }
}

} // namespace

BakingTargetsPane::~BakingTargetsPane() noexcept
{
    WorkspaceAppearance::ReleasePanePointerOwnership(this);
}

void BakingTargetsPane::AttachSession(BakingWorkspaceSession* TargetSession) noexcept
{
    Session       = TargetSession;
    ScrollOffsetY = 0.0f;
}

void BakingTargetsPane::PresentInImGuiPane(double DeltaSeconds, float ScaleFactor)
{
    const float  SafeScale = std::max(0.25f, ScaleFactor);
    const ImVec2 WinPos    = ImGui::GetWindowPos();
    const ImVec2 WinSize   = ImGui::GetWindowSize();
    const ImVec2 CursorPos = ImGui::GetCursorScreenPos();
    const float  TopY      = CursorPos.y - ImGui::GetStyle().WindowPadding.y;
    const float  PaneH     = std::max(1.0f, (WinPos.y + WinSize.y) - TopY);

    ImGui::InvisibleButton(
        "##BakingTargetsCanvas",
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

void BakingTargetsPane::ConstructTargetPreviewSwatch(
    PixelSpace&   Surface,
    Rectangle2D   SwatchBounds,
    BakerTargetId Target,
    bool          /*Ready*/) noexcept
{
    Surface.PushClip(SwatchBounds);
    const float Cx = SwatchBounds.X + SwatchBounds.Width * 0.5f;
    const float Cy = SwatchBounds.Y + SwatchBounds.Height * 0.5f;
    const float R  = std::min(SwatchBounds.Width, SwatchBounds.Height) * 0.40f;
    constexpr float Rad = 6.0f;

    switch (Target)
    {
        case BakerTargetId::VolumeSdf:
            Surface.FillRectangle(SwatchBounds, { 42u, 26u, 64u, 255u }, Rad);
            Surface.StrokeCircle({ Cx, Cy }, R * 0.92f, { 196u, 140u, 255u, 235u }, 1.4f);
            Surface.FillCircle({ Cx, Cy }, R * 0.45f, { 196u, 140u, 255u, 210u });
            break;
        case BakerTargetId::TangentNormal:
            Surface.FillRectangle(SwatchBounds, { 114u, 122u, 236u, 255u }, Rad);
            Surface.FillCircle({ Cx - R * 0.15f, Cy - R * 0.15f }, R * 0.72f, { 166u, 168u, 255u, 230u });
            break;
        case BakerTargetId::WorldNormal:
            Surface.FillRectangle(SwatchBounds, { 68u, 124u, 196u, 255u }, Rad);
            Surface.FillCircle({ Cx, Cy }, R * 0.80f, { 122u, 218u, 152u, 230u });
            Surface.FillCircle({ Cx + R * 0.22f, Cy - R * 0.18f }, R * 0.45f, { 236u, 118u, 132u, 210u });
            break;
        case BakerTargetId::ModelSpaceNormal:
            Surface.FillRectangle(SwatchBounds, { 172u, 96u, 178u, 255u }, Rad);
            Surface.FillCircle({ Cx, Cy }, R * 0.75f, { 118u, 206u, 198u, 220u });
            break;
        case BakerTargetId::BentNormal:
            Surface.FillRectangle(SwatchBounds, { 78u, 98u, 186u, 255u }, Rad);
            Surface.FillCircle({ Cx - R * 0.12f, Cy - R * 0.14f }, R * 0.75f, { 176u, 204u, 246u, 225u });
            break;
        case BakerTargetId::BevelNormal:
            Surface.FillRectangle(SwatchBounds, { 98u, 104u, 228u, 255u }, Rad);
            Surface.StrokeRectangle(SwatchBounds.Inset(4.0f), { 198u, 204u, 255u, 235u }, 3.0f, 1.6f);
            break;
        case BakerTargetId::AmbientOcclusion:
            Surface.FillRectangle(SwatchBounds, { 184u, 188u, 194u, 255u }, Rad);
            Surface.FillCircle({ Cx, Cy }, R * 0.85f, { 96u, 100u, 106u, 255u });
            Surface.FillCircle({ Cx, Cy }, R * 0.45f, { 232u, 234u, 238u, 255u });
            break;
        case BakerTargetId::SecondaryAo:
            Surface.FillRectangle(SwatchBounds, { 132u, 136u, 142u, 255u }, Rad);
            Surface.FillCircle({ Cx, Cy }, R * 0.75f, { 62u, 66u, 72u, 255u });
            Surface.FillCircle({ Cx, Cy }, R * 0.40f, { 192u, 196u, 202u, 255u });
            break;
        case BakerTargetId::Curvature:
            Surface.FillRectangle(SwatchBounds, { 96u, 102u, 110u, 255u }, Rad);
            Surface.FillCircle({ Cx - R * 0.25f, Cy }, R * 0.65f, { 94u, 204u, 166u, 225u });
            Surface.FillCircle({ Cx + R * 0.25f, Cy }, R * 0.55f, { 232u, 122u, 102u, 220u });
            break;
        case BakerTargetId::Convexity:
            Surface.FillRectangle(SwatchBounds, { 38u, 42u, 48u, 255u }, Rad);
            Surface.StrokeCircle({ Cx, Cy }, R * 0.72f, { 232u, 236u, 242u, 240u }, 1.8f);
            break;
        case BakerTargetId::Concavity:
            Surface.FillRectangle(SwatchBounds, { 38u, 42u, 48u, 255u }, Rad);
            Surface.FillCircle({ Cx, Cy }, R * 0.70f, { 216u, 98u, 82u, 235u });
            break;
        case BakerTargetId::Thickness:
            Surface.FillRectangle(SwatchBounds, { 76u, 36u, 24u, 255u }, Rad);
            Surface.FillCircle({ Cx, Cy }, R * 0.80f, { 236u, 146u, 74u, 240u });
            break;
        case BakerTargetId::HeightDisplacement:
            Surface.FillRectangle(SwatchBounds, { 54u, 58u, 64u, 255u }, Rad);
            Surface.FillCircle({ Cx, Cy }, R * 0.72f, { 224u, 228u, 234u, 245u });
            break;
        case BakerTargetId::TopDownDust:
            Surface.FillRectangle(SwatchBounds, { 58u, 60u, 66u, 255u }, Rad);
            Surface.FillRectangle({ SwatchBounds.X, SwatchBounds.Y, SwatchBounds.Width, SwatchBounds.Height * 0.52f }, { 216u, 198u, 162u, 240u }, 0.0f);
            break;
        case BakerTargetId::UvIslandId:
            Surface.FillRectangle(SwatchBounds, { 77u, 155u, 255u, 255u }, Rad);
            Surface.FillRectangle({ SwatchBounds.X, SwatchBounds.Y, SwatchBounds.Width * 0.5f, SwatchBounds.Height * 0.5f }, { 255u, 107u, 94u, 255u }, 0.0f);
            Surface.FillRectangle({ Cx, Cy, SwatchBounds.Width * 0.5f, SwatchBounds.Height * 0.5f }, { 70u, 210u, 103u, 255u }, 0.0f);
            break;
        case BakerTargetId::PartMaterialId:
            Surface.FillRectangle(SwatchBounds, { 196u, 140u, 255u, 255u }, Rad);
            Surface.FillRectangle({ SwatchBounds.X + SwatchBounds.Width * 0.34f, SwatchBounds.Y, SwatchBounds.Width * 0.33f, SwatchBounds.Height }, { 70u, 210u, 103u, 255u }, 0.0f);
            Surface.FillRectangle({ SwatchBounds.X + SwatchBounds.Width * 0.67f, SwatchBounds.Y, SwatchBounds.Width * 0.33f, SwatchBounds.Height }, { 255u, 107u, 94u, 255u }, 0.0f);
            break;
        case BakerTargetId::WorldPosition:
            Surface.FillRectangle(SwatchBounds, { 72u, 156u, 102u, 255u }, Rad);
            Surface.FillCircle({ Cx - R * 0.2f, Cy + R * 0.15f }, R * 0.62f, { 228u, 88u, 96u, 220u });
            Surface.FillCircle({ Cx + R * 0.2f, Cy - R * 0.15f }, R * 0.58f, { 88u, 156u, 238u, 220u });
            break;
        case BakerTargetId::UvWireframe:
        default:
            Surface.FillRectangle(SwatchBounds, { 24u, 28u, 34u, 255u }, Rad);
            Surface.StrokeLine({ SwatchBounds.X + 4.0f, SwatchBounds.Y + 4.0f }, { SwatchBounds.Right() - 4.0f, SwatchBounds.Bottom() - 4.0f }, { 77u, 155u, 255u, 230u }, 1.2f);
            Surface.StrokeLine({ SwatchBounds.Right() - 4.0f, SwatchBounds.Y + 4.0f }, { SwatchBounds.X + 4.0f, SwatchBounds.Bottom() - 4.0f }, { 77u, 155u, 255u, 230u }, 1.2f);
            break;
    }
    Surface.PopClip();
}

void BakingTargetsPane::Render(
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

    constexpr float PadX    = 12.0f;
    constexpr float FooterH = 46.0f;
    const float     InnerW  = std::max(60.0f, ShellRect.Width - PadX * 2.0f);
    const float     LeftX   = ShellRect.X + PadX;
    float           CursorY = ShellRect.Y + 13.0f;

    // 1. Outliner Heading ("Queue" + active study & resolution + Reset round button)
    const auto& ActiveStudy = Session->QueryActiveStudyDescriptor();
    char SubTitleBuf[64] {};
    std::snprintf(SubTitleBuf, sizeof(SubTitleBuf), "%s · %upx", ActiveStudy.Label, Session->TargetResolutionPx);
    if (ConstructOutlinerHeading(Surface, { LeftX, CursorY, InnerW, 38.0f }, "Queue", SubTitleBuf, ControlIcon::RefreshCycle, Input, Tone))
    {
        Session->ResetDefaults();
    }
    CursorY += 46.0f;

    // 2. Stat Tile Pair: QUEUED & READY / UNBAKED
    const std::size_t SelectedCount = Session->CountSelectedTargets();
    uint32_t ReadyCount = 0u;
    for (std::size_t Idx = 0u; Idx < BakingWorkspaceSession::TargetCount; ++Idx)
    {
        if (Session->QueryTargetReady(static_cast<BakerTargetId>(Idx)))
        {
            ++ReadyCount;
        }
    }

    char QueuedBuf[24] {};
    char ReadyBuf[24] {};
    std::snprintf(QueuedBuf, sizeof(QueuedBuf), "%zu/%zu", SelectedCount, BakingWorkspaceSession::TargetCount);
    if (ReadyCount > 0u)
    {
        std::snprintf(ReadyBuf, sizeof(ReadyBuf), "%u", ReadyCount);
    }
    else
    {
        std::snprintf(ReadyBuf, sizeof(ReadyBuf), "%zu", SelectedCount);
    }

    const OutlinerStatTileSpec LeftStat {
        "QUEUED",
        QueuedBuf,
        ControlIcon::CircleCheck,
        SelectedCount > 0u,
        false,
        FilterQueuedOnly,
    };
    const OutlinerStatTileSpec RightStat {
        (ReadyCount > 0u) ? "READY" : "UNBAKED",
        ReadyBuf,
        ControlIcon::BakeBox,
        ReadyCount > 0u,
        ReadyCount == 0u && SelectedCount > 0u,
        FilterReadyOnly,
    };
    const int32_t StatClicked = ConstructStatTilePair(Surface, { LeftX, CursorY, InnerW, 54.0f }, LeftStat, RightStat, Input, Tone);
    if (StatClicked == 0)
    {
        FilterQueuedOnly = !FilterQueuedOnly;
    }
    else if (StatClicked == 1)
    {
        FilterReadyOnly = !FilterReadyOnly;
    }
    CursorY += 62.0f;

    // 3. Search + Filter row
    const bool FilterActive = FilterQueuedOnly || FilterReadyOnly;
    if (ConstructSearchFilterRow(
            Surface,
            { LeftX, CursorY, InnerW, 32.0f },
            SearchBuffer,
            "Search targets, groups...",
            FilterActive ? "Filtered" : "Filter",
            FilterActive,
            Input,
            Tone))
    {
        if (!FilterQueuedOnly && !FilterReadyOnly)
        {
            FilterQueuedOnly = true;
        }
        else if (FilterQueuedOnly)
        {
            FilterQueuedOnly = false;
            FilterReadyOnly  = true;
        }
        else
        {
            FilterReadyOnly = false;
        }
    }
    CursorY += 40.0f;

    // 4. Quick preset chips (PBR / PBR + Volume / Normals / All)
    const std::array<OutlinerChipSpec, 4u> PresetChips {{
        { "PBR Core",     {}, {}, false, false, Session->QueryPresetMatched(BakerQuickPreset::PbrStandard),   false },
        { "PBR + Volume", {}, {}, false, false, Session->QueryPresetMatched(BakerQuickPreset::PbrPlusVolume), false },
        { "Normals",      {}, {}, false, false, Session->QueryPresetMatched(BakerQuickPreset::Normals),       false },
        { "All",          {}, {}, false, false, Session->QueryPresetMatched(BakerQuickPreset::SelectAll),     false },
    }};
    float ChipsH = 0.0f;
    const int32_t PresetClicked = ConstructQuickChipStrip(Surface, LeftX, CursorY, InnerW, PresetChips, Input, Tone, ChipsH);
    if (PresetClicked >= 0)
    {
        Session->ApplyQuickPreset(static_cast<BakerQuickPreset>(PresetClicked));
    }
    CursorY += ChipsH + 8.0f;

    Surface.StrokeLine({ ShellRect.X + 1.0f, CursorY }, { ShellRect.Right() - 1.0f, CursorY }, Tone.Hairline, 1.0f);
    CursorY += 6.0f;

    // 5. Scrollable Queue Hierarchy Tree
    const Rectangle2D FooterRect { ShellRect.X + 1.0f, ShellRect.Bottom() - FooterH, ShellRect.Width - 2.0f, FooterH };
    const Rectangle2D TreeRect   { ShellRect.X + 4.0f, CursorY, ShellRect.Width - 8.0f, std::max(40.0f, FooterRect.Y - CursorY - 4.0f) };

    const Point2D Cursor = QueryInputCursor(Input);
    if (TreeRect.Contains(Cursor) && std::fabs(Input.WheelDeltaY) > 1.0e-4f)
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

    Surface.PushClip(TreeRect);

    // 5a. Active Study Root Row
    {
        char StudyMeta[64] {};
        char ResTag[16] {};
        std::snprintf(StudyMeta, sizeof(StudyMeta), "High/Low · %uk tris", Session->QueryActiveHighTriangles() / 1000u);
        if (Session->TargetResolutionPx >= 1024u)
        {
            std::snprintf(ResTag, sizeof(ResTag), "%uK", Session->TargetResolutionPx / 1024u);
        }
        else
        {
            std::snprintf(ResTag, sizeof(ResTag), "%u", Session->TargetResolutionPx);
        }

        HierarchyRowSpec StudyRow {};
        StudyRow.DepthLevel   = 0;
        StudyRow.Title        = ActiveStudy.Label;
        StudyRow.Subtitle     = StudyMeta;
        StudyRow.LeadIcon     = ControlIcon::BakeBox;
        StudyRow.LeadTileFill = Tone.AmberWash;
        StudyRow.LeadIconInk  = Tone.Amber;
        StudyRow.TagLabel     = ResTag;
        StudyRow.TagStyle     = TagTone::Neutral;
        if (RowY + RowH >= TreeRect.Y && RowY <= TreeRect.Bottom())
        {
            (void)ConstructHierarchyRow(Surface, { RowLeftX, RowY, RowW, RowH }, StudyRow, Input, Tone);
        }
        RowY += RowH + RowGap;
    }

    // 5b. Categorized target groups (Normals, Surface, Masks & IDs, Volume & Distance)
    const auto& Registry = BakingWorkspaceSession::QueryTargetRegistry();
    constexpr std::array<BakerTargetGroup, 4u> GroupOrder {{
        BakerTargetGroup::Normals,
        BakerTargetGroup::Surface,
        BakerTargetGroup::DiagnosticMasks,
        BakerTargetGroup::VolumeDistance,
    }};

    for (const BakerTargetGroup Group : GroupOrder)
    {
        const uint32_t GroupIdx   = static_cast<uint32_t>(Group);
        const char*    GroupTitle = BakingWorkspaceSession::QueryGroupHeading(Group);

        uint32_t TotalInGroup   = 0u;
        uint32_t QueuedInGroup  = 0u;
        uint32_t VisibleInGroup = 0u;
        float    GroupMb        = 0.0f;

        for (const auto& Desc : Registry)
        {
            if (Desc.Group != Group) continue;
            ++TotalInGroup;
            const std::size_t Idx = static_cast<std::size_t>(Desc.Id);
            const bool Queued     = Session->SelectedTargets[Idx];
            const bool Ready      = Session->QueryTargetReady(Desc.Id);
            if (Queued)
            {
                ++QueuedInGroup;
                GroupMb += EstimateTargetMegabytes(*Session, Desc.Id);
            }
            if (FilterQueuedOnly && !Queued) continue;
            if (FilterReadyOnly && !Ready) continue;
            if (!SubstringMatchFold(Desc.Label, Query) && !SubstringMatchFold(GroupTitle, Query)) continue;
            ++VisibleInGroup;
        }
        if (VisibleInGroup == 0u) continue;

        const bool GroupOpen = !Query.empty() || ((ExpandedGroupMask & (1u << GroupIdx)) != 0u);
        char GroupMeta[64] {};
        if (QueuedInGroup > 0u)
        {
            std::snprintf(GroupMeta, sizeof(GroupMeta), "%u/%u queued · %.1f MB", QueuedInGroup, TotalInGroup, GroupMb);
        }
        else
        {
            std::snprintf(GroupMeta, sizeof(GroupMeta), "0/%u queued", TotalInGroup);
        }

        HierarchyRowSpec GrpRow {};
        GrpRow.DepthLevel       = 0;
        GrpRow.Title            = GroupTitle;
        GrpRow.Subtitle         = GroupMeta;
        GrpRow.LeadIcon         = ResolveGroupIcon(Group);
        GrpRow.LeadTileFill     = Tone.InsetSurface;
        GrpRow.LeadIconInk      = ResolveGroupAccent(Group, Tone);
        GrpRow.Expandable       = true;
        GrpRow.Expanded         = GroupOpen;
        GrpRow.GroupStyle       = true;
        GrpRow.TrailingSwitch   = TrailingSwitchKind::GreenCheck;
        GrpRow.TrailingEnabled  = (QueuedInGroup == TotalInGroup && TotalInGroup > 0u);
        GrpRow.PartialIndicator = (QueuedInGroup > 0u && QueuedInGroup < TotalInGroup);

        if (RowY + RowH >= TreeRect.Y && RowY <= TreeRect.Bottom())
        {
            const auto Hit = ConstructHierarchyRow(Surface, { RowLeftX, RowY, RowW, RowH }, GrpRow, Input, Tone);
            if (Hit.TrailingToggled)
            {
                const bool EnableAll = (QueuedInGroup < TotalInGroup);
                for (const auto& Desc : Registry)
                {
                    if (Desc.Group == Group)
                    {
                        Session->SelectedTargets[static_cast<std::size_t>(Desc.Id)] = EnableAll;
                    }
                }
            }
            else if (Hit.ChevronClicked || Hit.RowClicked)
            {
                ExpandedGroupMask ^= (1u << GroupIdx);
            }
        }
        RowY += RowH + RowGap;

        if (!GroupOpen) continue;

        for (const auto& Desc : Registry)
        {
            if (Desc.Group != Group) continue;
            const std::size_t Idx = static_cast<std::size_t>(Desc.Id);
            const bool Queued     = Session->SelectedTargets[Idx];
            const bool Ready      = Session->QueryTargetReady(Desc.Id);
            if (FilterQueuedOnly && !Queued) continue;
            if (FilterReadyOnly && !Ready) continue;
            if (!SubstringMatchFold(Desc.Label, Query) && !SubstringMatchFold(GroupTitle, Query)) continue;

            const bool Inspected = (Session->InspectedTarget == Desc.Id);
            char TargetMeta[64] {};
            if (Queued)
            {
                std::snprintf(TargetMeta, sizeof(TargetMeta), "%s · %.1f MB", Desc.ShortSummary, EstimateTargetMegabytes(*Session, Desc.Id));
            }
            else
            {
                std::snprintf(TargetMeta, sizeof(TargetMeta), "%s", Desc.ShortSummary);
            }

            HierarchyRowSpec RowSpec {};
            RowSpec.DepthLevel      = 1;
            RowSpec.Title           = Desc.Label;
            RowSpec.Subtitle        = TargetMeta;
            RowSpec.LeadSolidSwatch = true;
            RowSpec.LeadTileFill    = Tone.RecessedWell;
            RowSpec.Selected        = Inspected;
            RowSpec.Muted           = !Queued;
            RowSpec.TagLabel        = Ready ? "READY" : (Session->BakingActive && Queued ? "BAKING" : "");
            RowSpec.TagStyle        = Ready ? TagTone::Green : TagTone::Amber;
            RowSpec.TrailingSwitch  = TrailingSwitchKind::GreenCheck;
            RowSpec.TrailingEnabled = Queued;

            const Rectangle2D TargetRect { RowLeftX, RowY, RowW, RowH };
            if (RowY + RowH >= TreeRect.Y && RowY <= TreeRect.Bottom())
            {
                const auto Hit = ConstructHierarchyRow(Surface, TargetRect, RowSpec, Input, Tone);
                // Overlay custom procedural target swatch over the 22x22 leading tile
                const Rectangle2D SwatchBox { TargetRect.X + 8.0f + 13.0f + 14.0f, TargetRect.Center().Y - 11.0f, 22.0f, 22.0f };
                ConstructTargetPreviewSwatch(Surface, SwatchBox, Desc.Id, Ready);
                Surface.StrokeRectangle(SwatchBox, Inspected ? Tone.Amber : Tone.BorderSubtle, 6.0f, 1.0f);

                if (Hit.TrailingToggled)
                {
                    Session->ToggleTargetSelected(Desc.Id);
                }
                else if (Hit.RowClicked)
                {
                    Session->InspectTarget(Desc.Id);
                }
            }
            RowY += RowH + RowGap;
        }
    }

    // 5c. Collapsible Studies Library Folder
    {
        const auto& Studies = BakingWorkspaceSession::QueryStudyRegistry();
        char StudiesMeta[32] {};
        std::snprintf(StudiesMeta, sizeof(StudiesMeta), "%zu study targets", Studies.size());

        HierarchyRowSpec StudHead {};
        StudHead.DepthLevel   = 0;
        StudHead.Title        = "Studies";
        StudHead.Subtitle     = StudiesMeta;
        StudHead.LeadIcon     = ControlIcon::FolderClosed;
        StudHead.LeadTileFill = Tone.InsetSurface;
        StudHead.LeadIconInk  = Tone.TextMuted;
        StudHead.Expandable   = true;
        StudHead.Expanded     = StudiesExpanded || !Query.empty();
        StudHead.GroupStyle   = true;

        if (RowY + RowH >= TreeRect.Y && RowY <= TreeRect.Bottom())
        {
            const auto Hit = ConstructHierarchyRow(Surface, { RowLeftX, RowY, RowW, RowH }, StudHead, Input, Tone);
            if (Hit.RowClicked || Hit.ChevronClicked)
            {
                StudiesExpanded = !StudiesExpanded;
            }
        }
        RowY += RowH + RowGap;

        if (StudiesExpanded || !Query.empty())
        {
            for (const auto& Study : Studies)
            {
                if (!SubstringMatchFold(Study.Label, Query)) continue;
                const bool Active = (Study.Study == Session->Study);
                char TriMeta[48] {};
                std::snprintf(TriMeta, sizeof(TriMeta), "%uk low · %uk high", Study.LowTris / 1000u, Study.HighTris / 1000u);

                HierarchyRowSpec SRow {};
                SRow.DepthLevel   = 1;
                SRow.Title        = Study.Label;
                SRow.Subtitle     = TriMeta;
                SRow.LeadIcon     = ControlIcon::CubeShape;
                SRow.LeadTileFill = Active ? Tone.AmberWash : Tone.TileSurface;
                SRow.LeadIconInk  = Active ? Tone.Amber : Tone.TextMuted;
                SRow.Selected     = Active;
                SRow.TagLabel     = Active ? "ACTIVE" : "";
                SRow.TagStyle     = TagTone::Green;

                if (RowY + RowH >= TreeRect.Y && RowY <= TreeRect.Bottom())
                {
                    const auto Hit = ConstructHierarchyRow(Surface, { RowLeftX, RowY, RowW, RowH }, SRow, Input, Tone);
                    if (Hit.RowClicked && !Active)
                    {
                        Session->SelectStudy(Study.Study);
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

    // 6. Four-column Baking Queue Footer (OUTPUT / RAYS / GRID / DEVICE)
    float TotalMb = 0.0f;
    for (std::size_t Idx = 0u; Idx < BakingWorkspaceSession::TargetCount; ++Idx)
    {
        if (Session->SelectedTargets[Idx])
        {
            TotalMb += EstimateTargetMegabytes(*Session, static_cast<BakerTargetId>(Idx));
        }
    }
    char OutBuf[20] {};
    char RayBuf[20] {};
    char GridBuf[20] {};
    std::snprintf(OutBuf, sizeof(OutBuf), "%.1f", TotalMb);
    FormatCompactCount(EstimateTotalRays(*Session), RayBuf, sizeof(RayBuf));
    std::snprintf(GridBuf, sizeof(GridBuf), "%u", Session->TargetResolutionPx);

    const std::string_view ComputeLabel = (Session->ComputeDevice == BakerComputeDevice::RtxHardwareRayQuery)
        ? "RTX"
        : ((Session->ComputeDevice == BakerComputeDevice::GtxComputeBvh4) ? "GTX" : "AUTO");

    const std::array<FooterMetricColumn, 4u> FooterCols {{
        { "OUTPUT", OutBuf,       "MB", TagTone::Neutral },
        { "RAYS",   RayBuf,       "",   TagTone::Neutral },
        { "RES",    GridBuf,      "px", TagTone::Neutral },
        { "DEVICE", ComputeLabel, "",   TagTone::Green   },
    }};
    ConstructFourColumnFooter(Surface, FooterRect, FooterCols, Tone);
}

} // namespace Frontier::Workspaces
