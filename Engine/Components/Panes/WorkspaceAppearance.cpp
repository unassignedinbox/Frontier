/*============================================================================================================================================
                                                           WORKSPACEAPPEARANCE.CPP
============================================================================================================================================*/
// 🧩 Shared surface color tokens, VisualToken, VectorGraphic, Workspace icons, indicators, signs, and symbols

//------------------------------------------------------------------------------------------------------------------------
//                                                        INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Engine/Components/Panes/WorkspaceAppearance.h"
#include "Engine/Components/VectorIcons/VectorIconRepository.h"
#include "Engine/Input/InputExtension.h"

#include "imgui.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <string>

namespace Frontier::WorkspaceAppearance
{

namespace
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    INTERNAL CONSTANTS
//------------------------------------------------------------------------------------------------------------------------

[[nodiscard]] ColorRgba ScaleAlpha(ColorRgba BaseColor, float Opacity) noexcept
{
    const float Clamped = std::clamp(Opacity, 0.0f, 1.0f);
    return {
        BaseColor.Red,
        BaseColor.Green,
        BaseColor.Blue,
        static_cast<uint8_t>(std::round(static_cast<float>(BaseColor.Alpha) * Clamped)),
    };
}

void EncodeUtf8Codepoint(uint32_t Codepoint, std::string& Destination)
{
    if (Codepoint == 0u || Codepoint == '\r' || Codepoint == '\n' || Codepoint < 32u)
    {
        return;
    }
    if (Codepoint < 0x80u)
    {
        Destination.push_back(static_cast<char>(Codepoint));
    }
    else if (Codepoint < 0x800u)
    {
        Destination.push_back(static_cast<char>(0xC0u | (Codepoint >> 6u)));
        Destination.push_back(static_cast<char>(0x80u | (Codepoint & 0x3Fu)));
    }
    else if (Codepoint < 0x10000u)
    {
        Destination.push_back(static_cast<char>(0xE0u | (Codepoint >> 12u)));
        Destination.push_back(static_cast<char>(0x80u | ((Codepoint >> 6u) & 0x3Fu)));
        Destination.push_back(static_cast<char>(0x80u | (Codepoint & 0x3Fu)));
    }
    else if (Codepoint <= 0x10FFFFu)
    {
        Destination.push_back(static_cast<char>(0xF0u | (Codepoint >> 18u)));
        Destination.push_back(static_cast<char>(0x80u | ((Codepoint >> 12u) & 0x3Fu)));
        Destination.push_back(static_cast<char>(0x80u | ((Codepoint >> 6u) & 0x3Fu)));
        Destination.push_back(static_cast<char>(0x80u | (Codepoint & 0x3Fu)));
    }
}

struct PointerOwnershipRegister
{
    const void*                    ActiveOwnerPane         = nullptr;
    bool                           ExternalDragSuppression = false;
    uint32_t                       OwnerStaleEnforceTicks  = 0u;
    std::array<Rectangle2D, 6u>    OverlayShields          {};
    uint32_t                       OverlayShieldCount      = 0u;
};

[[nodiscard]] PointerOwnershipRegister& AccessPointerOwnershipRegister() noexcept
{
    static PointerOwnershipRegister OwnershipRegister{};
    return OwnershipRegister;
}

[[nodiscard]] bool QueryOverlayShieldContains(Point2D PhysicalCursor) noexcept
{
    const auto& Reg = AccessPointerOwnershipRegister();
    const uint32_t Count = std::min<uint32_t>(Reg.OverlayShieldCount, static_cast<uint32_t>(Reg.OverlayShields.size()));
    for (uint32_t Idx = 0u; Idx < Count; ++Idx)
    {
        const Rectangle2D& Shield = Reg.OverlayShields[Idx];
        if (Shield.Width > 0.5f && Shield.Height > 0.5f && Shield.Contains(PhysicalCursor))
        {
            return true;
        }
    }
    return false;
}

} // namespace

//------------------------------------------------------------------------------------------------------------------------
//                                             INPUT & TYPOGRAPHY CONSTRUCTION
//------------------------------------------------------------------------------------------------------------------------

PaneInputReading CaptureImGuiPaneInput(Rectangle2D ScreenBounds, float ScaleFactor) noexcept
{
    PaneInputReading Reading{};
    if (ImGui::GetCurrentContext() == nullptr)
    {
        return Reading;
    }

    const auto& Snapshot  = Input::InputExtension::InspectActiveEditorSnapshot();
    const float SafeScale = std::max(0.25f, ScaleFactor);

    if (Snapshot.PointerSeeded && Snapshot.PositionX > -1.0e8f && Snapshot.PositionY > -1.0e8f)
    {
        Reading.PrimaryPointer.X = (Snapshot.PositionX - ScreenBounds.X) / SafeScale;
        Reading.PrimaryPointer.Y = (Snapshot.PositionY - ScreenBounds.Y) / SafeScale;
        Reading.PointerDeltaX    = Snapshot.TickDeltaX / SafeScale;
        Reading.PointerDeltaY    = Snapshot.TickDeltaY / SafeScale;
    }

    const Point2D ScreenCursor{ Snapshot.PositionX, Snapshot.PositionY };
    const bool    InsideBounds  = Snapshot.PointerSeeded && ScreenBounds.Contains(ScreenCursor);
    const bool    HoveredWindow = InsideBounds && ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);

    Reading.PrimaryPointer.Enabled  = HoveredWindow || InsideBounds;
    Reading.PrimaryPointer.Down     = Snapshot.QueryButtonDown(0u);
    Reading.PrimaryPointer.Pressed  = Snapshot.QueryButtonPressed(0u);
    Reading.PrimaryPointer.Released = Snapshot.QueryButtonReleased(0u);

    Reading.SecondaryDown     = Snapshot.QueryButtonDown(1u);
    Reading.SecondaryPressed  = Snapshot.QueryButtonPressed(1u);
    Reading.SecondaryReleased = Snapshot.QueryButtonReleased(1u);
    Reading.MiddleDown        = Snapshot.QueryButtonDown(2u);
    Reading.MiddlePressed     = Snapshot.QueryButtonPressed(2u);
    Reading.MiddleReleased    = Snapshot.QueryButtonReleased(2u);
    Reading.DoubleClicked     = Snapshot.QueryButtonDoubleClicked(0u);
    Reading.WheelDeltaY       = (HoveredWindow || InsideBounds) ? Snapshot.ScrollDeltaY : 0.0f;

    Reading.ShiftHeld         = Snapshot.ShiftHeld;
    Reading.ControlHeld       = Snapshot.ControlHeld;
    Reading.AltHeld           = Snapshot.AltHeld;
    Reading.EnterPressed      = Snapshot.QueryKeyPressed(Input::InputLogicalKey::Enter, false) ||
                                Snapshot.QueryKeyPressed(Input::InputLogicalKey::KeypadEnter, false);
    Reading.EscapePressed     = Snapshot.QueryKeyPressed(Input::InputLogicalKey::Escape, false);
    Reading.DeletePressed     = Snapshot.QueryKeyPressed(Input::InputLogicalKey::Delete, false);
    Reading.BackspacePressed  = Snapshot.QueryKeyPressed(Input::InputLogicalKey::Backspace, true);
    Reading.TabPressed        = Snapshot.QueryKeyPressed(Input::InputLogicalKey::Tab, false);
    Reading.ArrowLeftPressed  = Snapshot.QueryKeyPressed(Input::InputLogicalKey::LeftArrow, true);
    Reading.ArrowRightPressed = Snapshot.QueryKeyPressed(Input::InputLogicalKey::RightArrow, true);
    Reading.HomePressed       = Snapshot.QueryKeyPressed(Input::InputLogicalKey::Home, false);
    Reading.EndPressed        = Snapshot.QueryKeyPressed(Input::InputLogicalKey::End, false);

    Reading.ShortcutAll       = Snapshot.ControlHeld && Snapshot.QueryKeyPressed(Input::InputLogicalKey::A, false);
    Reading.ShortcutFind      = Snapshot.ControlHeld && Snapshot.QueryKeyPressed(Input::InputLogicalKey::F, false);
    Reading.ShortcutGroup     = Snapshot.ControlHeld && Snapshot.QueryKeyPressed(Input::InputLogicalKey::G, false);
    Reading.ShortcutDuplicate = Snapshot.ControlHeld && Snapshot.QueryKeyPressed(Input::InputLogicalKey::D, false);
    Reading.ShortcutLinked    = Snapshot.ControlHeld && Snapshot.QueryKeyPressed(Input::InputLogicalKey::L, false);
    Reading.ShortcutInvert    = Snapshot.ControlHeld && Snapshot.QueryKeyPressed(Input::InputLogicalKey::I, false);
    Reading.ShortcutUnwrap    = Snapshot.ControlHeld && !Snapshot.ShiftHeld && !Snapshot.AltHeld &&
                                Snapshot.QueryKeyPressed(Input::InputLogicalKey::U, false);
    Reading.ShortcutUndo      = Snapshot.ControlHeld && !Snapshot.ShiftHeld && !Snapshot.AltHeld &&
                                Snapshot.QueryKeyPressed(Input::InputLogicalKey::Z, false);
    Reading.ShortcutRedo      = Snapshot.ControlHeld && !Snapshot.AltHeld &&
                                ((Snapshot.ShiftHeld && Snapshot.QueryKeyPressed(Input::InputLogicalKey::Z, false)) ||
                                 (!Snapshot.ShiftHeld && Snapshot.QueryKeyPressed(Input::InputLogicalKey::Y, false)));
    Reading.ToggleDiagnostics = Snapshot.QueryKeyPressed(Input::InputLogicalKey::F3, false);
    Reading.KeyboardFocused   = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

    for (const uint32_t Codepoint : Snapshot.QueryTypedCodepoints())
    {
        EncodeUtf8Codepoint(Codepoint, Reading.TypedUtf8);
    }

    const auto& Reg = AccessPointerOwnershipRegister();
    if (Reg.ActiveOwnerPane == nullptr &&
        !Reg.ExternalDragSuppression &&
        Snapshot.PointerSeeded &&
        QueryOverlayShieldContains(ScreenCursor))
    {
        SuppressPanePointerInput(Reading);
    }

    return Reading;
}

void SuppressPanePointerInput(PaneInputReading& InOutInput) noexcept
{
    InOutInput.PrimaryPointer.X        = -1.0e6f;
    InOutInput.PrimaryPointer.Y        = -1.0e6f;
    InOutInput.PrimaryPointer.Enabled  = false;
    InOutInput.PrimaryPointer.Pressed  = false;
    InOutInput.PrimaryPointer.Down     = false;
    InOutInput.PrimaryPointer.Released = false;
    InOutInput.SecondaryPressed        = false;
    InOutInput.SecondaryDown           = false;
    InOutInput.SecondaryReleased       = false;
    InOutInput.MiddlePressed           = false;
    InOutInput.MiddleDown              = false;
    InOutInput.MiddleReleased          = false;
    InOutInput.DoubleClicked           = false;
    InOutInput.WheelDeltaY             = 0.0f;
}

const void* QueryActivePointerOwner() noexcept
{
    return AccessPointerOwnershipRegister().ActiveOwnerPane;
}

bool QueryPointerBlockedForPane(const void* PaneIdentity) noexcept
{
    const auto& Reg = AccessPointerOwnershipRegister();
    return Reg.ActiveOwnerPane != nullptr && Reg.ActiveOwnerPane != PaneIdentity;
}

void AssignExternalDragSuppression(bool ExternalDragActive) noexcept
{
    AccessPointerOwnershipRegister().ExternalDragSuppression = ExternalDragActive;
}

void AssignOverlayPointerShield(Rectangle2D PrimaryPhysicalBounds, Rectangle2D SecondaryPhysicalBounds) noexcept
{
    auto& Reg              = AccessPointerOwnershipRegister();
    Reg.OverlayShieldCount = 0u;
    if (PrimaryPhysicalBounds.Width > 0.5f && PrimaryPhysicalBounds.Height > 0.5f)
    {
        Reg.OverlayShields[Reg.OverlayShieldCount++] = PrimaryPhysicalBounds;
    }
    if (SecondaryPhysicalBounds.Width > 0.5f && SecondaryPhysicalBounds.Height > 0.5f)
    {
        Reg.OverlayShields[Reg.OverlayShieldCount++] = SecondaryPhysicalBounds;
    }
}

void AppendOverlayPointerShield(Rectangle2D AdditionalPhysicalBounds) noexcept
{
    auto& Reg = AccessPointerOwnershipRegister();
    if (AdditionalPhysicalBounds.Width > 0.5f &&
        AdditionalPhysicalBounds.Height > 0.5f &&
        Reg.OverlayShieldCount < static_cast<uint32_t>(Reg.OverlayShields.size()))
    {
        Reg.OverlayShields[Reg.OverlayShieldCount++] = AdditionalPhysicalBounds;
    }
}

void ClearOverlayPointerShield() noexcept
{
    auto& Reg              = AccessPointerOwnershipRegister();
    Reg.OverlayShieldCount = 0u;
}

bool QueryOverlayPointerBlockedAt(Point2D PhysicalCursor) noexcept
{
    return QueryOverlayShieldContains(PhysicalCursor);
}

void EnforcePanePointerOwnership(const void* PaneIdentity, PaneInputReading& InOutInput) noexcept
{
    auto& Reg = AccessPointerOwnershipRegister();
    if (Reg.ActiveOwnerPane == nullptr)
    {
        return;
    }
    if (Reg.ActiveOwnerPane == PaneIdentity)
    {
        Reg.OwnerStaleEnforceTicks        = 0u;
        InOutInput.PrimaryPointer.Enabled = true;
        return;
    }

    const auto& RawSnapshot = Input::InputExtension::InspectActiveEditorSnapshot();
    const bool  RawButtonsDown =
        RawSnapshot.QueryButtonDown(0u) ||
        RawSnapshot.QueryButtonDown(1u) ||
        RawSnapshot.QueryButtonDown(2u);
    if (!RawButtonsDown)
    {
        ++Reg.OwnerStaleEnforceTicks;
        if (Reg.OwnerStaleEnforceTicks > 16u)
        {
            Reg.ActiveOwnerPane        = nullptr;
            Reg.OwnerStaleEnforceTicks = 0u;
            return;
        }
    }
    else
    {
        Reg.OwnerStaleEnforceTicks = 0u;
    }

    SuppressPanePointerInput(InOutInput);
}

void UpdatePanePointerOwnership(
    const void*             PaneIdentity,
    Rectangle2D             ScreenBounds,
    float                   ScaleFactor,
    const PaneInputReading& Input,
    bool                    ActiveDragCaptured) noexcept
{
    if (PaneIdentity == nullptr)
    {
        return;
    }

    auto&       Reg         = AccessPointerOwnershipRegister();
    const float SafeScale   = std::max(0.25f, ScaleFactor);
    const Point2D PhysicalCursor{
        ScreenBounds.X + Input.PrimaryPointer.X * SafeScale,
        ScreenBounds.Y + Input.PrimaryPointer.Y * SafeScale,
    };
    const bool AnyButtonDown = Input.PrimaryPointer.Down || Input.MiddleDown || Input.SecondaryDown;

    if (Reg.ActiveOwnerPane == PaneIdentity)
    {
        Reg.OwnerStaleEnforceTicks = 0u;
        if (!ActiveDragCaptured && !AnyButtonDown)
        {
            Reg.ActiveOwnerPane = nullptr;
        }
        return;
    }

    if (Reg.ActiveOwnerPane == nullptr && !Reg.ExternalDragSuppression)
    {
        if (ActiveDragCaptured || (ScreenBounds.Contains(PhysicalCursor) && AnyButtonDown))
        {
            Reg.ActiveOwnerPane        = PaneIdentity;
            Reg.OwnerStaleEnforceTicks = 0u;
        }
    }
}

void ReleasePanePointerOwnership(const void* PaneIdentity) noexcept
{
    auto& Reg = AccessPointerOwnershipRegister();
    if (Reg.ActiveOwnerPane == PaneIdentity)
    {
        Reg.ActiveOwnerPane        = nullptr;
        Reg.OwnerStaleEnforceTicks = 0u;
    }
}

void ResetAllPanePointerOwnership() noexcept
{
    auto& Reg = AccessPointerOwnershipRegister();
    Reg.ActiveOwnerPane         = nullptr;
    Reg.ExternalDragSuppression = false;
    Reg.OwnerStaleEnforceTicks  = 0u;
}

void ForwardInputToTextBuffer(TextEditBuffer& Buffer, const PaneInputReading& Input, double Δt) noexcept
{
    Buffer.BlinkPhase = std::fmod(
        Buffer.BlinkPhase + static_cast<float>(std::max(0.0, Δt)),
        ControlMetrics::CaretBlinkSeconds);
    if (!Buffer.Active)
    {
        return;
    }

    if (Input.ShortcutAll)
    {
        Buffer.SelectAll();
    }
    if (Input.BackspacePressed)
    {
        Buffer.Backspace();
    }
    if (Input.DeletePressed)
    {
        Buffer.DeleteForward();
    }
    if (Input.ArrowLeftPressed)
    {
        Buffer.MoveCaret(-1, Input.ShiftHeld);
    }
    if (Input.ArrowRightPressed)
    {
        Buffer.MoveCaret(1, Input.ShiftHeld);
    }
    if (Input.HomePressed)
    {
        Buffer.MoveHome(Input.ShiftHeld);
    }
    if (Input.EndPressed)
    {
        Buffer.MoveEnd(Input.ShiftHeld);
    }
    if (!Input.TypedUtf8.empty() && !Input.ControlHeld)
    {
        Buffer.Insert(Input.TypedUtf8.c_str());
    }
}

ImFont* ResolveWeightedFont(FontWeightClass Weight, bool Monospace) noexcept
{
    const TypographyRepository* Repo = TypographyRepository::QueryCurrent();
    if (Repo == nullptr)
    {
        return nullptr;
    }

    auto TryFamily = [&](std::string_view Family) -> ImFont*
    {
        const int32_t GroupIdx = Repo->ResolveGroupIndex(Family);
        if (GroupIdx >= 0)
        {
            return Repo->QueryFont(static_cast<uint32_t>(GroupIdx), Weight);
        }
        return nullptr;
    };

    if (Monospace)
    {
        if (ImFont* Mono = TryFamily("JetBrains Mono"))
        {
            return Mono;
        }
    }

    if (ImFont* Primary = TryFamily("DM Sans"))
    {
        return Primary;
    }
    if (ImFont* Secondary = TryFamily("Inter"))
    {
        return Secondary;
    }
    if (Repo->QueryGroupCount() > 0u)
    {
        return Repo->QueryFont(0u, FontWeightClass::Regular);
    }
    return nullptr;
}

void ConstructWeightedText(
    PixelSpace&      Surface,
    FontWeightClass  Weight,
    Point2D          Position,
    float            PixelSize,
    ColorRgba        Ink,
    std::string_view Text,
    bool             Monospace) noexcept
{
    if (Text.empty() || Ink.Alpha == 0u || PixelSize <= 0.0f)
    {
        return;
    }

    ImFont* Resolved = ResolveWeightedFont(Weight, Monospace);
    if (Resolved != nullptr)
    {
        Surface.PushTypeface(Resolved);
    }
    Surface.Text(Position, PixelSize, Ink, Text.data(), Text.data() + Text.size());
    if (Resolved != nullptr)
    {
        Surface.PopTypeface();
    }
}

float MeasureWeightedTextWidth(
    PixelSpace&      Surface,
    FontWeightClass  Weight,
    float            PixelSize,
    std::string_view Text,
    bool             Monospace) noexcept
{
    if (Text.empty() || PixelSize <= 0.0f)
    {
        return 0.0f;
    }

    ImFont* Resolved = ResolveWeightedFont(Weight, Monospace);
    if (Resolved != nullptr)
    {
        Surface.PushTypeface(Resolved);
    }
    const float Width = Surface.MeasureTextWidth(PixelSize, Text.data(), Text.data() + Text.size());
    if (Resolved != nullptr)
    {
        Surface.PopTypeface();
    }
    return Width;
}

void ConstructTruncatedText(
    PixelSpace&      Surface,
    FontWeightClass  Weight,
    Point2D          Position,
    float            MaxWidth,
    float            PixelSize,
    ColorRgba        Ink,
    std::string_view Text,
    bool             Monospace) noexcept
{
    if (Text.empty() || MaxWidth <= 2.0f || Ink.Alpha == 0u)
    {
        return;
    }

    if (MeasureWeightedTextWidth(Surface, Weight, PixelSize, Text, Monospace) <= MaxWidth)
    {
        ConstructWeightedText(Surface, Weight, Position, PixelSize, Ink, Text, Monospace);
        return;
    }

    std::string Candidate{};
    Candidate.reserve(Text.size() + 3u);
    for (std::size_t Count = Text.size(); Count > 0u; --Count)
    {
        Candidate.assign(Text.data(), Count);
        Candidate.append("...");
        if (MeasureWeightedTextWidth(Surface, Weight, PixelSize, Candidate, Monospace) <= MaxWidth)
        {
            ConstructWeightedText(Surface, Weight, Position, PixelSize, Ink, Candidate, Monospace);
            return;
        }
    }
}

void ConstructRightAlignedText(
    PixelSpace&      Surface,
    FontWeightClass  Weight,
    Point2D          RightTopAnchor,
    float            PixelSize,
    ColorRgba        Ink,
    std::string_view Text,
    bool             Monospace) noexcept
{
    const float Width = MeasureWeightedTextWidth(Surface, Weight, PixelSize, Text, Monospace);
    ConstructWeightedText(Surface, Weight, { RightTopAnchor.X - Width, RightTopAnchor.Y }, PixelSize, Ink, Text, Monospace);
}

//------------------------------------------------------------------------------------------------------------------------
//                                         VISUALTOKEN & VECTORGRAPHIC PRIMITIVES
//------------------------------------------------------------------------------------------------------------------------

ColorRgba QueryDockedPaneBackground() noexcept
{
    if (ImGui::GetCurrentContext() != nullptr)
    {
        const ImVec4 WinBg = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
        return ColorRgba{
            static_cast<uint8_t>(std::clamp(std::lround(WinBg.x * 255.0f), 0L, 255L)),
            static_cast<uint8_t>(std::clamp(std::lround(WinBg.y * 255.0f), 0L, 255L)),
            static_cast<uint8_t>(std::clamp(std::lround(WinBg.z * 255.0f), 0L, 255L)),
            static_cast<uint8_t>(std::clamp(std::lround(WinBg.w * 255.0f), 0L, 255L)),
        };
    }
    return WorkspaceSurfaceColors::DockBackground;
}

ColorRgba QueryCategoryAccent(ConstructSlotCategory Category, ColorRgba FolderTint) noexcept
{
    switch (Category)
    {
        case ConstructSlotCategory::Folder:
            return FolderTint;
        case ConstructSlotCategory::GeometryVolume:
            return { 56, 156, 255, 255 };  // #389CFF (editor-cube)
        case ConstructSlotCategory::LayerGroup:
            return { 192, 174, 212, 255 }; // #C0AED4 (collection-stacked-layers)
        case ConstructSlotCategory::PointLuminaire:
            return { 255, 205, 56, 255 };  // #FFCD38 (editor-point-light)
        case ConstructSlotCategory::OpticsCamera:
            return { 160, 164, 182, 255 }; // #A0A4B6 (camera)
        case ConstructSlotCategory::ForceField:
            return { 104, 220, 255, 255 }; // #68DCFF (aerodynamic cyan streamline)
    }
    return WorkspaceSurfaceColors::PrimaryInk;
}

VisualToken QueryCategoryVisualToken(ConstructSlotCategory Category, float Extent, ColorRgba FolderTint) noexcept
{
    VisualToken Token{};
    Token.Shape     = VisualTokenShape::Circle;
    Token.FillColor = QueryCategoryAccent(Category, FolderTint);
    Token.Extent    = Extent;
    return Token;
}

const char* QueryCategoryDefaultLabel(ConstructSlotCategory Category) noexcept
{
    switch (Category)
    {
        case ConstructSlotCategory::Folder:
            return "Collection";
        case ConstructSlotCategory::GeometryVolume:
            return "Type 1";
        case ConstructSlotCategory::LayerGroup:
            return "Type 2";
        case ConstructSlotCategory::PointLuminaire:
            return "Type 3";
        case ConstructSlotCategory::OpticsCamera:
            return "Type 4";
        case ConstructSlotCategory::ForceField:
            return "Force Field";
    }
    return "Entry";
}

const char* QueryProjectionPresetLabel(ViewportProjectionPreset Preset) noexcept
{
    switch (Preset)
    {
        case ViewportProjectionPreset::Perspective:
            return "Perspective";
        case ViewportProjectionPreset::Orthographic:
            return "Orthographic";
        case ViewportProjectionPreset::Top:
            return "Top";
        case ViewportProjectionPreset::Front:
            return "Front";
        case ViewportProjectionPreset::Right:
            return "Right";
        case ViewportProjectionPreset::Count:
            break;
    }
    return "Perspective";
}

void ConstructVisualToken(
    PixelSpace&        Surface,
    Point2D            Center,
    const VisualToken& Token,
    float              Opacity) noexcept
{
    const float     Half       = Token.Extent * 0.5f;
    const ColorRgba FillRgba   = ScaleAlpha(Token.FillColor, Opacity);
    const ColorRgba StrokeRgba = ScaleAlpha(Token.StrokeColor, Opacity);

    if (Token.Shape == VisualTokenShape::Circle)
    {
        if (FillRgba.Alpha > 0u)
        {
            Surface.FillCircle(Center, Half, FillRgba);
        }
        if (StrokeRgba.Alpha > 0u && Token.StrokeWidth > 0.0f)
        {
            Surface.StrokeCircle(Center, Half + 2.2f, StrokeRgba, Token.StrokeWidth);
        }
        return;
    }

    const Rectangle2D Bounds{ Center.X - Half, Center.Y - Half, Token.Extent, Token.Extent };
    const float       Corner = (Token.Shape == VisualTokenShape::RoundedSquare) ? Token.CornerRadius : 0.0f;
    if (FillRgba.Alpha > 0u)
    {
        Surface.FillRectangle(Bounds, FillRgba, Corner);
    }
    if (StrokeRgba.Alpha > 0u && Token.StrokeWidth > 0.0f)
    {
        Surface.StrokeRectangle(Bounds, StrokeRgba, Corner, Token.StrokeWidth);
    }
}

void ConstructVectorGraphic(
    PixelSpace&          Surface,
    const VectorGraphic& Graphic,
    Point2D              TopLeft,
    float                Extent,
    ColorRgba            Ink) noexcept
{
    if (Graphic.SvgPath.empty() || Extent <= 0.0f || Ink.Alpha == 0u)
    {
        return;
    }

    const VectorIconPlacement Placement{ TopLeft.X, TopLeft.Y, Extent, Ink, Graphic.StrokeWidth };
    if (Graphic.Filled)
    {
        VectorIconRepository::FillSvgPath(Surface, Graphic.SvgPath, Placement, Graphic.ViewBox);
    }
    else
    {
        VectorIconRepository::StrokeSvgPath(Surface, Graphic.SvgPath, Placement, Graphic.ViewBox);
    }
}

//------------------------------------------------------------------------------------------------------------------------
//                                  UNIFIED STUDIO ICONS (DELEGATED TO VECTORICONREPOSITORY)
//------------------------------------------------------------------------------------------------------------------------

void ConstructShowcaseFolderIcon(
    PixelSpace& Surface,
    Point2D     TopLeft,
    float       Extent,
    ColorRgba   AccentTint,
    float       Opacity) noexcept
{
    VectorIconRepository::ConstructStudioInBounds(
        Surface, StudioIcon::ShowcaseFolder, { TopLeft.X, TopLeft.Y, Extent, Extent }, AccentTint, Opacity);
}

void ConstructGenericFolderIcon(
    PixelSpace& Surface,
    Point2D     TopLeft,
    float       Extent,
    ColorRgba   AccentTint,
    float       Opacity) noexcept
{
    VectorIconRepository::ConstructStudioInBounds(
        Surface, StudioIcon::GenericFolder, { TopLeft.X, TopLeft.Y, Extent, Extent }, AccentTint, Opacity);
}

void ConstructCameraIcon(
    PixelSpace& Surface,
    Point2D     TopLeft,
    float       Extent,
    float       Opacity) noexcept
{
    VectorIconRepository::ConstructStudioInBounds(
        Surface, StudioIcon::CameraOptics, { TopLeft.X, TopLeft.Y, Extent, Extent }, WorkspaceSurfaceColors::PrimaryInk, Opacity);
}

void ConstructSlotCategoryIcon(
    PixelSpace&           Surface,
    ConstructSlotCategory Category,
    Point2D               TopLeft,
    float                 Extent,
    ColorRgba             FolderTint,
    float                 Opacity) noexcept
{
    const Rectangle2D Box{ TopLeft.X, TopLeft.Y, Extent, Extent };
    switch (Category)
    {
        case ConstructSlotCategory::Folder:
            VectorIconRepository::ConstructStudioInBounds(Surface, StudioIcon::ShowcaseFolder, Box, FolderTint, Opacity);
            break;
        case ConstructSlotCategory::GeometryVolume:
            VectorIconRepository::ConstructStudioInBounds(Surface, StudioIcon::GeometryCube, Box, FolderTint, Opacity);
            break;
        case ConstructSlotCategory::LayerGroup:
            VectorIconRepository::ConstructStudioInBounds(Surface, StudioIcon::StackedLayers, Box, FolderTint, Opacity);
            break;
        case ConstructSlotCategory::PointLuminaire:
            VectorIconRepository::ConstructStudioInBounds(Surface, StudioIcon::PointLuminaire, Box, FolderTint, Opacity);
            break;
        case ConstructSlotCategory::OpticsCamera:
            VectorIconRepository::ConstructStudioInBounds(Surface, StudioIcon::CameraOptics, Box, FolderTint, Opacity);
            break;
        case ConstructSlotCategory::ForceField:
            VectorIconRepository::ConstructStudioInBounds(Surface, StudioIcon::ForceStream, Box, FolderTint, Opacity);
            break;
    }
}

void ConstructFilterSlidersIcon(
    PixelSpace& Surface,
    Point2D     TopLeft,
    float       Extent,
    ColorRgba   Ink,
    ColorRgba   KnobFill) noexcept
{
    VectorIconRepository::ConstructControlInBounds(
        Surface, ControlIcon::FilterSliders, { TopLeft.X, TopLeft.Y, Extent, Extent }, Ink, Extent,
        std::max(1.2f, Extent * (1.5f / 24.0f)), VectorIconAlignment::ViewBoxCentered, KnobFill);
}

void ConstructEditIcon(PixelSpace& Surface, Point2D TopLeft, float Extent, ColorRgba Ink) noexcept
{
    VectorIconRepository::ConstructControlInBounds(
        Surface, ControlIcon::EditPencilSolid, { TopLeft.X, TopLeft.Y, Extent, Extent }, Ink, Extent);
}

void ConstructSimulateIcon(
    PixelSpace& Surface,
    Point2D     TopLeft,
    float       Extent,
    ColorRgba   Ink,
    ColorRgba   InteriorCutout) noexcept
{
    VectorIconRepository::ConstructControlInBounds(
        Surface, ControlIcon::SimulateFlaskSolid, { TopLeft.X, TopLeft.Y, Extent, Extent }, Ink, Extent,
        0.0f, VectorIconAlignment::ViewBoxCentered, InteriorCutout);
}

void ConstructPlayIcon(PixelSpace& Surface, Point2D TopLeft, float Extent, ColorRgba Ink) noexcept
{
    VectorIconRepository::ConstructControlInBounds(
        Surface, ControlIcon::PlaySolid, { TopLeft.X, TopLeft.Y, Extent, Extent }, Ink, Extent);
}

void ConstructPauseIcon(PixelSpace& Surface, Point2D TopLeft, float Extent, ColorRgba Ink) noexcept
{
    VectorIconRepository::ConstructControlInBounds(
        Surface, ControlIcon::PauseSolid, { TopLeft.X, TopLeft.Y, Extent, Extent }, Ink, Extent);
}

void ConstructConstructIcon(PixelSpace& Surface, Point2D TopLeft, float Extent, ColorRgba Ink) noexcept
{
    VectorIconRepository::ConstructControlInBounds(
        Surface, ControlIcon::ConstructCubePlus, { TopLeft.X, TopLeft.Y, Extent, Extent }, Ink, Extent);
}

void ConstructFocusIcon(PixelSpace& Surface, Point2D TopLeft, float Extent, ColorRgba Ink) noexcept
{
    VectorIconRepository::ConstructControlInBounds(
        Surface, ControlIcon::FocusReticleSolid, { TopLeft.X, TopLeft.Y, Extent, Extent }, Ink, Extent);
}

void ConstructComponentModeIcon(PixelSpace& Surface, Point2D Origin, float Extent, uint32_t ModeIdx, ColorRgba Ink) noexcept
{
    VectorIconRepository::ConstructComponentModeInBounds(
        Surface, static_cast<ComponentModeIcon>(std::min(ModeIdx, 3u)), { Origin.X, Origin.Y, Extent, Extent }, Ink);
}

void ConstructSplitViewportIcon(
    PixelSpace& Surface,
    Point2D     TopLeft,
    float       Extent,
    ColorRgba   Ink,
    ColorRgba   InteriorCutout) noexcept
{
    VectorIconRepository::ConstructControlInBounds(
        Surface, ControlIcon::SplitViewportSolid, { TopLeft.X, TopLeft.Y, Extent, Extent }, Ink, Extent,
        0.0f, VectorIconAlignment::ViewBoxCentered, InteriorCutout);
}

void ConstructDiagnosticsIcon(PixelSpace& Surface, Point2D TopLeft, float Extent, ColorRgba Ink) noexcept
{
    VectorIconRepository::ConstructControlInBounds(
        Surface, ControlIcon::DiagnosticsBarsSolid, { TopLeft.X, TopLeft.Y, Extent, Extent }, Ink, Extent);
}

void ConstructViewportSettingsIcon(
    PixelSpace& Surface,
    Point2D     TopLeft,
    float       Extent,
    ColorRgba   Ink,
    ColorRgba   InteriorCutout) noexcept
{
    VectorIconRepository::ConstructControlInBounds(
        Surface, ControlIcon::ViewportGearSolid, { TopLeft.X, TopLeft.Y, Extent, Extent }, Ink, Extent,
        0.0f, VectorIconAlignment::ViewBoxCentered, InteriorCutout);
}

void ConstructCapabilityIcon(
    PixelSpace&      Surface,
    Point2D          TopLeft,
    float            Extent,
    CapabilityIconId Capability,
    ColorRgba        Ink,
    ColorRgba        InteriorCutout) noexcept
{
    CapabilityIcon Mapped = CapabilityIcon::Visibility;
    switch (Capability)
    {
        case CapabilityIconId::Visibility:    Mapped = CapabilityIcon::Visibility;    break;
        case CapabilityIconId::ShadowCaster:  Mapped = CapabilityIcon::ShadowCaster;  break;
        case CapabilityIconId::DynamicMotion: Mapped = CapabilityIcon::DynamicMotion; break;
        case CapabilityIconId::TransformLock: Mapped = CapabilityIcon::TransformLock; break;
    }
    VectorIconRepository::ConstructCapabilityInBounds(
        Surface, Mapped, { TopLeft.X, TopLeft.Y, Extent, Extent }, Ink, InteriorCutout);
}

//------------------------------------------------------------------------------------------------------------------------
//                                      SEMANTIC INDICATORS, SIGNS, AND SYMBOLS
//------------------------------------------------------------------------------------------------------------------------

void ConstructVisibilityIndicator(
    PixelSpace& Surface,
    Point2D     Center,
    bool        Visible,
    float       Opacity) noexcept
{
    if (Visible)
    {
        VisualToken Disc{};
        Disc.Shape     = VisualTokenShape::Circle;
        Disc.FillColor = WorkspaceSurfaceColors::StatusGreen;
        Disc.Extent    = 15.6f;
        ConstructVisualToken(Surface, Center, Disc, Opacity);
        ConstructCheckSymbol(Surface, Center, 8.0f, ScaleAlpha(WorkspaceSurfaceColors::StatusGreenInk, Opacity), 1.8f);
    }
    else
    {
        VisualToken Disc{};
        Disc.Shape       = VisualTokenShape::Circle;
        Disc.FillColor   = { 42, 42, 42, 255 };
        Disc.StrokeColor = { 76, 76, 76, 255 };
        Disc.Extent      = 14.0f;
        Disc.StrokeWidth = 1.0f;
        ConstructVisualToken(Surface, Center, Disc, Opacity);
    }
}

void ConstructVerticalScrollIndicator(
    PixelSpace& Surface,
    Rectangle2D ViewportBounds,
    float       ContentHeight,
    float       ScrollOffset,
    Point2D     Cursor) noexcept
{
    if (ContentHeight <= ViewportBounds.Height + 1.0f || ViewportBounds.Height <= 24.0f)
    {
        return;
    }

    const float       TrackWidth  = 5.0f;
    const float       TrackMargin = 3.0f;
    const Rectangle2D TrackBounds{
        ViewportBounds.Right() - TrackWidth - TrackMargin,
        ViewportBounds.Y + 4.0f,
        TrackWidth,
        ViewportBounds.Height - 8.0f,
    };

    const float VisibleRatio = std::clamp(ViewportBounds.Height / ContentHeight, 0.08f, 1.0f);
    const float ThumbHeight  = std::max(24.0f, TrackBounds.Height * VisibleRatio);
    const float MaxScroll    = std::max(1.0f, ContentHeight - ViewportBounds.Height);
    const float Normalized   = std::clamp(ScrollOffset / MaxScroll, 0.0f, 1.0f);
    const float ThumbY       = TrackBounds.Y + (TrackBounds.Height - ThumbHeight) * Normalized;

    const Rectangle2D ThumbBounds{ TrackBounds.X, ThumbY, TrackWidth, ThumbHeight };
    const bool        Hovered = ThumbBounds.Contains(Cursor) || TrackBounds.Contains(Cursor);

    Surface.FillRectangle(TrackBounds, { 255, 255, 255, 10 }, 2.5f);
    Surface.FillRectangle(ThumbBounds, Hovered ? ColorRgba{ 138, 138, 138, 195 } : ColorRgba{ 98, 98, 98, 145 }, 2.5f);
}

void ConstructCautionTriangleSign(
    PixelSpace& Surface,
    Point2D     Center,
    float       Extent,
    ColorRgba   Ink) noexcept
{
    const float Half = Extent * 0.5f;
    const std::array<Point2D, 3> Triangle = {{
        { Center.X,               Center.Y - Half * 0.88f },
        { Center.X + Half * 0.95f, Center.Y + Half * 0.78f },
        { Center.X - Half * 0.95f, Center.Y + Half * 0.78f },
    }};
    Surface.StrokePolyline(Triangle, Ink, 1.35f, true);
}

void ConstructOrientationAxesSign(
    PixelSpace&              Surface,
    Rectangle2D              PaneBounds,
    ViewportProjectionPreset Projection) noexcept
{
    const Point2D   TriadOrigin{ PaneBounds.Right() - 36.0f, PaneBounds.Bottom() - 28.0f };
    constexpr float AxisLength = 19.0f;

    const ColorRgba RedAxis{ 226, 86, 86, 235 };
    const ColorRgba GreenAxis{ 94, 201, 118, 235 };
    const ColorRgba BlueAxis{ 88, 152, 238, 235 };

    Point2D AxisX{ TriadOrigin.X + AxisLength * 0.88f, TriadOrigin.Y + AxisLength * 0.34f };
    Point2D AxisY{ TriadOrigin.X - AxisLength * 0.76f, TriadOrigin.Y + AxisLength * 0.42f };
    Point2D AxisZ{ TriadOrigin.X, TriadOrigin.Y - AxisLength };

    if (Projection == ViewportProjectionPreset::Top)
    {
        AxisX = { TriadOrigin.X + AxisLength, TriadOrigin.Y };
        AxisY = { TriadOrigin.X, TriadOrigin.Y - AxisLength };
        AxisZ = { TriadOrigin.X - AxisLength * 0.45f, TriadOrigin.Y + AxisLength * 0.45f };
    }
    else if (Projection == ViewportProjectionPreset::Front)
    {
        AxisX = { TriadOrigin.X + AxisLength, TriadOrigin.Y };
        AxisZ = { TriadOrigin.X, TriadOrigin.Y - AxisLength };
        AxisY = { TriadOrigin.X - AxisLength * 0.45f, TriadOrigin.Y + AxisLength * 0.45f };
    }
    else if (Projection == ViewportProjectionPreset::Right)
    {
        AxisY = { TriadOrigin.X + AxisLength, TriadOrigin.Y };
        AxisZ = { TriadOrigin.X, TriadOrigin.Y - AxisLength };
        AxisX = { TriadOrigin.X - AxisLength * 0.45f, TriadOrigin.Y + AxisLength * 0.45f };
    }

    Surface.StrokeLine(TriadOrigin, AxisX, RedAxis, 1.7f);
    Surface.StrokeLine(TriadOrigin, AxisY, GreenAxis, 1.7f);
    Surface.StrokeLine(TriadOrigin, AxisZ, BlueAxis, 1.7f);

    Surface.FillCircle(AxisX, 3.0f, RedAxis);
    Surface.FillCircle(AxisY, 3.0f, GreenAxis);
    Surface.FillCircle(AxisZ, 3.0f, BlueAxis);
    Surface.FillCircle(TriadOrigin, 2.4f, { 210, 210, 210, 255 });

    ConstructWeightedText(Surface, FontWeightClass::Medium, { AxisX.X + 4.0f, AxisX.Y - 6.0f }, 9.0f, RedAxis, "X");
    ConstructWeightedText(Surface, FontWeightClass::Medium, { AxisY.X - 10.0f, AxisY.Y - 5.0f }, 9.0f, GreenAxis, "Y");
    ConstructWeightedText(Surface, FontWeightClass::Medium, { AxisZ.X - 3.0f, AxisZ.Y - 14.0f }, 9.0f, BlueAxis, "Z");
}

void ConstructCheckSymbol(
    PixelSpace& Surface,
    Point2D     Center,
    float       Extent,
    ColorRgba   Ink,
    float       Thickness) noexcept
{
    VectorIconRepository::ConstructControlCentered(
        Surface, ControlIcon::CheckMark, Center, Extent * 1.18f, Ink, Thickness, VectorIconAlignment::GeometricCentered);
}

void ConstructChevronSymbol(
    PixelSpace& Surface,
    Point2D     Center,
    float       Extent,
    bool        Expanded,
    ColorRgba   Ink) noexcept
{
    VectorIconRepository::ConstructControlCentered(
        Surface,
        Expanded ? ControlIcon::ChevronDown : ControlIcon::ChevronForward,
        Center,
        Extent * 1.15f,
        Ink,
        1.5f,
        VectorIconAlignment::GeometricCentered);
}

void ConstructResetSymbol(
    PixelSpace& Surface,
    Point2D     Center,
    float       Extent,
    ColorRgba   Ink,
    float       Thickness) noexcept
{
    VectorIconRepository::ConstructControlCentered(
        Surface, ControlIcon::ResetArc, Center, Extent, Ink, Thickness, VectorIconAlignment::ViewBoxCentered);
}

ColorRgba QueryForceModelTint(uint32_t ForceModelOrdinal) noexcept
{
    return VectorIconRepository::QueryForceModelTint(ForceModelOrdinal);
}

void ConstructForceModelIcon(
    PixelSpace& Surface,
    uint32_t    ForceModelOrdinal,
    Point2D     TopLeft,
    float       Extent,
    ColorRgba   AccentInk,
    float       Opacity) noexcept
{
    VectorIconRepository::ConstructForceModelInBounds(
        Surface, ForceModelOrdinal, { TopLeft.X, TopLeft.Y, Extent, Extent }, AccentInk, Opacity);
}

void ConstructSectionHeading(
    PixelSpace&      Surface,
    float            LeftX,
    float            TopY,
    float            Width,
    std::string_view Title,
    std::string_view RightTag,
    ControlIcon      Icon,
    ColorRgba        IconTint) noexcept
{
    const ControlColorSet& Colors = ControlAppearance::Colors();
    const bool             HasIcon = (Icon != ControlIcon::Count);
    if (HasIcon)
    {
        VectorIconRepository::ConstructControl(
            Surface, Icon, VectorIconPlacement{ LeftX, TopY + 1.0f, 14.0f, IconTint, 1.8f });
    }
    else
    {
        Surface.FillRectangle({ LeftX, TopY + 2.0f, 3.0f, 14.0f }, Colors.Accent.ToRgba8(), 1.5f);
    }

    ConstructWeightedText(
        Surface,
        FontWeightClass::Regular,
        { LeftX + (HasIcon ? 21.0f : 10.0f), TopY },
        12.0f,
        Colors.Text.ToRgba8(),
        Title);

    if (!RightTag.empty())
    {
        const float       TagW    = MeasureWeightedTextWidth(Surface, FontWeightClass::Regular, 8.5f, RightTag) + 12.0f;
        const Rectangle2D TagRect { LeftX + Width - TagW, TopY - 1.0f, TagW, 18.0f };
        const ColorQuad   WellCol = ColorQuad::Blend(Colors.Field, Colors.Panel, 0.35f);
        const ColorQuad   EdgeCol = ColorQuad::Blend(Colors.StrokeStrong, Colors.Text, 0.14f);
        Surface.FillRectangle(TagRect, WellCol.ToRgba8(), 9.0f);
        Surface.StrokeRectangle(TagRect, EdgeCol.ToRgba8(), 9.0f, 1.0f);
        ConstructWeightedText(
            Surface,
            FontWeightClass::Regular,
            { TagRect.X + 6.0f, TagRect.Y + 3.5f },
            8.5f,
            Colors.TextDim.ToRgba8(),
            RightTag);
    }
}

bool ConstructCapsuleToggleRow(
    PixelSpace&            Surface,
    Rectangle2D            RowRect,
    std::string_view       Label,
    bool&                  InOutEnabled,
    Point2D                Cursor,
    bool                   Pressed,
    const OptionalTooltip& Tooltip,
    std::string_view       DetailFirstLine,
    std::string_view       DetailSecondLine) noexcept
{
    const ControlColorSet& Colors = ControlAppearance::Colors();
    ConstructWeightedText(
        Surface, FontWeightClass::Regular, { RowRect.X, RowRect.Y }, 11.5f, Colors.Text.ToRgba8(), Label);
    if (!DetailFirstLine.empty())
    {
        ConstructTruncatedText(
            Surface, FontWeightClass::Regular, { RowRect.X, RowRect.Y + 16.0f }, RowRect.Width - 54.0f,
            9.5f, Colors.TextDim.ToRgba8(), DetailFirstLine);
        if (!DetailSecondLine.empty())
        {
            ConstructTruncatedText(
                Surface, FontWeightClass::Regular, { RowRect.X, RowRect.Y + 27.0f }, RowRect.Width - 54.0f,
                9.5f, Colors.TextDim.ToRgba8(), DetailSecondLine);
        }
    }

    const Rectangle2D TrackCapsule{ RowRect.Right() - 44.0f, RowRect.Y + 3.0f, 44.0f, 25.0f };
    const bool        Hovered = RowRect.Contains(Cursor);
    if (Hovered)
    {
        std::string     TitleStorage{};
        OptionalTooltip Effective = Tooltip;
        if (!Effective.Enabled)
        {
            TitleStorage.assign(Label);
            Effective = OptionalTooltip::FromTitled(TitleStorage.c_str(), "Toggle this workspace setting.");
        }
        ControlAppearance::RecordHoveredTooltip(RowRect, Effective);
    }

    Surface.FillRectangle(
        TrackCapsule, InOutEnabled ? ColorRgba{ 255, 255, 255, 255 } : ColorRgba{ 35, 35, 35, 255 }, 13.0f);
    Surface.StrokeRectangle(
        TrackCapsule, InOutEnabled ? ColorRgba{ 255, 255, 255, 255 } : ColorRgba{ 255, 255, 255, 26 }, 13.0f, 1.0f);
    const float KnobX = InOutEnabled ? (TrackCapsule.Right() - 13.0f) : (TrackCapsule.X + 13.0f);
    Surface.FillCircle(
        { KnobX, TrackCapsule.Y + 12.5f }, 9.5f,
        InOutEnabled ? ColorRgba{ 17, 17, 17, 255 } : ColorRgba{ 188, 188, 188, 255 });

    if (Hovered && Pressed)
    {
        InOutEnabled = !InOutEnabled;
        return true;
    }
    return false;
}

} // namespace Frontier::WorkspaceAppearance
