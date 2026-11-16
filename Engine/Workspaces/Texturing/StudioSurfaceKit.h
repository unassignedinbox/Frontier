/*============================================================================================================================================
                                                           STUDIOSURFACEKIT.H
============================================================================================================================================*/
// 🧩 Shared Project Zero surface tokens, outliner hierarchy rows, meter tiles, accordion rows, capsule tracks, switches, and popovers

#pragma once

//------------------------------------------------------------------------------------------------------------------------
//                                                        INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Engine/Components/Controls/ControlAppearance.h"
#include "Engine/Components/Controls/TextInputField.h"
#include "Engine/Components/Panes/WorkspaceAppearance.h"
#include "Engine/Components/PixelSpace.h"
#include "Engine/Components/VectorIcons/VectorIconRepository.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <span>
#include <string>
#include <string_view>

namespace Frontier::Workspaces::StudioSurface
{

//------------------------------------------------------------------------------------------------------------------------
//                                                      ENUMERATIONS
//------------------------------------------------------------------------------------------------------------------------

enum class MeterVariant : uint8_t
{
    None       = 0,
    BandDot    = 1, // Horizontal track with single positional indicator dot
    FillBar    = 2, // Solid horizontal fill bar
    StackBar   = 3, // Multi-segment proportional horizontal bar
    SwatchLine = 4, // Inline row of mini colour swatches
    StepTicks  = 5, // Discrete completion segments
};

enum class TagTone : uint8_t
{
    Neutral = 0, // Quiet grey capsule tag
    Amber   = 1, // Warm amber highlight (ACTIVE / STALE / BAKING)
    Green   = 2, // Fresh green highlight (SEAMLESS / READY / DONE)
    Blue    = 3, // Sky blue highlight (CUTOUT / CUSTOM)
    Crimson = 4, // Alert red highlight
};

enum class TrailingSwitchKind : uint8_t
{
    None       = 0,
    GreenCheck = 1, // 16px circular solid #34C759 check disc or hollow ring
    Visibility = 2, // Eye visible / hidden toggle button
};

//------------------------------------------------------------------------------------------------------------------------
//                                                       STRUCTURES
//------------------------------------------------------------------------------------------------------------------------

struct StudioToneSet
{
    ColorRgba DockBackdrop        {}; // #121212 outer dock backdrop
    ColorRgba OutlinerTop         {}; // #1E1E1E outliner block top
    ColorRgba OutlinerBottom      {}; // #171717 outliner block bottom
    ColorRgba OutlinerStroke      {}; // #2C2C2C outliner block stroke
    ColorRgba InspectorTop        {}; // #222222 inspector block top
    ColorRgba InspectorBottom     {}; // #1B1B1B inspector block bottom
    ColorRgba InspectorStroke     {}; // #2F2F2F inspector block stroke
    ColorRgba InsetSurface        {}; // #161616 dark stat tile / open editor fill
    ColorRgba TileSurface         {}; // #191919 stat pair & chip fill
    ColorRgba FoldSurface         {}; // #181818 bottom collapsible fold fill
    ColorRgba FooterSurface       {}; // #151515 bottom summary bar fill
    ColorRgba RecessedWell        {}; // #0E0E0E track / capsule well / readout fill
    ColorRgba ReadoutDark         {}; // #0D0D0D numeric readout fill
    ColorRgba ReadoutWarm         {}; // #1D180E driven amber readout fill
    ColorRgba RowHover            {}; // Subtle hover surface
    ColorRgba RowSelected         {}; // #262626 selected hierarchy row
    ColorRgba RowSelectedStroke   {}; // #444444 selected hierarchy border
    ColorRgba OpenRowStroke       {}; // Warm amber open accordion border
    ColorRgba Hairline            {}; // #262626 internal divider
    ColorRgba BorderSubtle        {}; // #2E2E2E standard control outline
    ColorRgba BorderStrong        {}; // #3F3F3F emphasized outline
    ColorRgba PrimaryFill         {}; // #E8E8E8 active segment & primary CTA fill
    ColorRgba PrimaryInk          {}; // #111111 crisp ink over PrimaryFill
    ColorRgba Amber               { 255u, 178u, 56u,  255u }; // #FFB238
    ColorRgba AmberWash           { 38u,  31u,  16u,  255u }; // #261F10
    ColorRgba AmberStroke         { 74u,  59u,  29u,  255u }; // #4A3B1D
    ColorRgba Green               { 70u,  210u, 103u, 255u }; // #46D267
    ColorRgba ToggleGreen         { 52u,  199u, 89u,  255u }; // #34C759
    ColorRgba GreenWash           { 26u,  47u,  32u,  255u };
    ColorRgba Blue                { 77u,  155u, 255u, 255u }; // #4D9BFF
    ColorRgba BlueWash            { 25u,  38u,  56u,  255u };
    ColorRgba Violet              { 196u, 140u, 255u, 255u }; // #C48CFF
    ColorRgba Crimson             { 255u, 107u, 94u,  255u }; // #FF6B5E
    ColorRgba CrimsonWash         { 54u,  26u,  25u,  255u };
    ColorRgba TextPrimary         {}; // #F0F0F0
    ColorRgba TextSecondary       {}; // #D4D4D4
    ColorRgba TextMuted           {}; // #8A8A8A
    ColorRgba TextFaint           {}; // #626262
    bool      LightSurface        = false;
};

struct OutlinerStatTileSpec
{
    std::string_view Label       {};
    std::string_view Reading     {};
    ControlIcon      Icon        = ControlIcon::CircleCheck;
    bool             ActiveGlow  = false; // Green disc when true
    bool             AmberGlow   = false; // Amber disc when true
    bool             FilterArmed = false; // Highlight tile border when active filter
};

struct OutlinerChipSpec
{
    std::string_view Label         {};
    ColorRgba        SwatchStart   { 0u, 0u, 0u, 0u };
    ColorRgba        SwatchEnd     { 0u, 0u, 0u, 0u };
    bool             ShowSwatch    = false;
    bool             ShowPlusIcon  = false;
    bool             Selected      = false;
    bool             Disabled      = false;
};

struct HierarchyRowSpec
{
    int32_t            DepthLevel       = 0;
    std::string_view   Title            {};
    std::string_view   Subtitle         {};
    std::string_view   TagLabel         {};
    TagTone            TagStyle         = TagTone::Neutral;
    ControlIcon        LeadIcon         = ControlIcon::LayersSlabs;
    ColorRgba          LeadTileFill     { 34u, 34u, 34u, 255u };
    ColorRgba          LeadIconInk      { 210u, 210u, 210u, 255u };
    bool               LeadSolidSwatch  = false; // Fill entire 22x22 tile with LeadTileFill colour
    bool               Expandable       = false;
    bool               Expanded         = false;
    bool               Selected         = false;
    bool               Muted            = false;
    bool               GroupStyle       = false; // Subtle section-group typography
    bool               LockedSign       = false;
    TrailingSwitchKind TrailingSwitch   = TrailingSwitchKind::None;
    bool               TrailingEnabled  = true;
    bool               PartialIndicator = false; // Small amber dot when some nested entries enabled
};

struct HierarchyRowResult
{
    bool RowClicked      = false;
    bool DoubleClicked   = false;
    bool ChevronClicked  = false;
    bool TrailingToggled = false;
    bool Hovered         = false;
};

struct FooterMetricColumn
{
    std::string_view Label   {};
    std::string_view Reading {};
    std::string_view Unit    {};
    TagTone          Tone    = TagTone::Neutral;
};

struct MeterSegment
{
    float     Share  = 0.0f;
    ColorRgba Colour { 210u, 210u, 210u, 255u };
};

struct MeterStatTileSpec
{
    std::string_view              Label         {};
    std::string_view              Reading       {};
    std::string_view              Unit          {};
    std::string_view              Caption       {};
    TagTone                       ReadingTone   = TagTone::Neutral;
    bool                          PrimaryAccent = false;
    MeterVariant                  MeterKind     = MeterVariant::FillBar;
    float                         MeterRatio    = 0.5f;
    float                         BandLow       = 0.0f;
    float                         BandHigh      = 1.0f;
    ColorRgba                     MeterColour   { 210u, 210u, 210u, 255u };
    std::array<MeterSegment, 4u>  StackParts    {};
    uint32_t                      StackCount    = 0u;
    std::array<ColorRgba, 6u>     Swatches      {};
    uint32_t                      SwatchCount   = 0u;
    int32_t                       StepTotal     = 0;
    int32_t                       StepDone      = 0;
};

struct AccordionRowSpec
{
    std::string_view Title           {};
    std::string_view Subtitle        {};
    std::string_view Readout         {};
    bool             DrivenWarm      = false; // Warm amber readout capsule
    bool             SolidSwatch     = false; // Solid colour swatch tile
    bool             ScalarBarSwatch = false; // Scalar fill swatch tile
    float            ScalarFill      = 0.5f;
    ColorRgba        SwatchColour    { 200u, 200u, 200u, 255u };
    ControlIcon      LeadIcon        = ControlIcon::SlidersQuality;
};

struct PopoverOptionSpec
{
    std::string_view Label       {};
    std::string_view Detail      {};
    ColorRgba        SwatchDot   { 0u, 0u, 0u, 0u };
};

struct InspectorHeadingResult
{
    bool UndoClicked  = false;
    bool RedoClicked  = false;
    bool ResetClicked = false;
};

struct InspectorFooterResult
{
    bool PrimaryClicked    = false;
    bool SecondaryAClicked = false;
    bool SecondaryBClicked = false;
};

//------------------------------------------------------------------------------------------------------------------------
//                                                  THEME & COLOUR HELPERS
//------------------------------------------------------------------------------------------------------------------------

[[nodiscard]] inline StudioToneSet QueryStudioToneSet() noexcept
{
    const ControlColorSet& Colors = ControlAppearance::Colors();
    StudioToneSet Tone {};
    Tone.LightSurface = Colors.LightSurface;

    if (!Colors.LightSurface)
    {
        Tone.DockBackdrop      = { 15u,  15u,  16u,  255u };
        Tone.OutlinerTop       = { 30u,  30u,  30u,  255u };
        Tone.OutlinerBottom    = { 23u,  23u,  23u,  255u };
        Tone.OutlinerStroke    = { 44u,  44u,  44u,  255u };
        Tone.InspectorTop      = { 34u,  34u,  34u,  255u };
        Tone.InspectorBottom   = { 27u,  27u,  27u,  255u };
        Tone.InspectorStroke   = { 47u,  47u,  47u,  255u };
        Tone.InsetSurface      = { 22u,  22u,  22u,  255u };
        Tone.TileSurface       = { 25u,  25u,  25u,  255u };
        Tone.FoldSurface       = { 24u,  24u,  24u,  255u };
        Tone.FooterSurface     = { 21u,  21u,  21u,  255u };
        Tone.RecessedWell      = { 14u,  14u,  14u,  255u };
        Tone.ReadoutDark       = { 13u,  13u,  13u,  255u };
        Tone.ReadoutWarm       = { 29u,  24u,  14u,  255u };
        Tone.RowHover          = { 34u,  34u,  34u,  255u };
        Tone.RowSelected       = { 38u,  38u,  38u,  255u };
        Tone.RowSelectedStroke = { 68u,  68u,  68u,  255u };
        Tone.OpenRowStroke     = { 255u, 178u, 56u,  76u  };
        Tone.Hairline          = { 38u,  38u,  38u,  255u };
        Tone.BorderSubtle      = { 45u,  45u,  45u,  255u };
        Tone.BorderStrong      = { 66u,  66u,  66u,  255u };
        Tone.PrimaryFill       = { 232u, 232u, 232u, 255u };
        Tone.PrimaryInk        = { 17u,  17u,  17u,  255u };
        Tone.TextPrimary       = { 240u, 240u, 240u, 255u };
        Tone.TextSecondary     = { 214u, 214u, 214u, 255u };
        Tone.TextMuted         = { 138u, 138u, 138u, 255u };
        Tone.TextFaint         = { 98u,  98u,  98u,  255u };
        return Tone;
    }

    // Crisp light-mode counterpart when ControlCentre selects Light appearance
    Tone.DockBackdrop      = { 234u, 236u, 240u, 255u };
    Tone.OutlinerTop       = { 248u, 249u, 251u, 255u };
    Tone.OutlinerBottom    = { 241u, 243u, 247u, 255u };
    Tone.OutlinerStroke    = { 210u, 214u, 222u, 255u };
    Tone.InspectorTop      = { 250u, 251u, 253u, 255u };
    Tone.InspectorBottom   = { 243u, 245u, 248u, 255u };
    Tone.InspectorStroke   = { 208u, 212u, 220u, 255u };
    Tone.InsetSurface      = { 236u, 239u, 244u, 255u };
    Tone.TileSurface       = { 240u, 242u, 246u, 255u };
    Tone.FoldSurface       = { 238u, 241u, 246u, 255u };
    Tone.FooterSurface     = { 233u, 236u, 241u, 255u };
    Tone.RecessedWell      = { 226u, 229u, 235u, 255u };
    Tone.ReadoutDark       = { 226u, 229u, 235u, 255u };
    Tone.ReadoutWarm       = { 252u, 240u, 214u, 255u };
    Tone.RowHover          = { 232u, 236u, 242u, 255u };
    Tone.RowSelected       = { 225u, 230u, 239u, 255u };
    Tone.RowSelectedStroke = { 184u, 192u, 206u, 255u };
    Tone.OpenRowStroke     = { 214u, 134u, 18u,  125u };
    Tone.Hairline          = { 216u, 220u, 227u, 255u };
    Tone.BorderSubtle      = { 206u, 211u, 220u, 255u };
    Tone.BorderStrong      = { 172u, 179u, 192u, 255u };
    Tone.PrimaryFill       = { 28u,  31u,  38u,  255u };
    Tone.PrimaryInk        = { 248u, 249u, 252u, 255u };
    Tone.Amber             = { 206u, 128u, 12u,  255u };
    Tone.AmberWash         = { 252u, 241u, 218u, 255u };
    Tone.AmberStroke       = { 232u, 188u, 114u, 255u };
    Tone.Green             = { 30u,  148u, 64u,  255u };
    Tone.ToggleGreen       = { 38u,  166u, 74u,  255u };
    Tone.GreenWash         = { 218u, 245u, 226u, 255u };
    Tone.Blue              = { 32u,  108u, 220u, 255u };
    Tone.BlueWash          = { 220u, 235u, 252u, 255u };
    Tone.Violet            = { 132u, 74u,  214u, 255u };
    Tone.Crimson           = { 206u, 52u,  44u,  255u };
    Tone.CrimsonWash       = { 252u, 224u, 222u, 255u };
    Tone.TextPrimary       = { 24u,  28u,  34u,  255u };
    Tone.TextSecondary     = { 54u,  60u,  70u,  255u };
    Tone.TextMuted         = { 104u, 112u, 124u, 255u };
    Tone.TextFaint         = { 138u, 145u, 156u, 255u };
    return Tone;
}

[[nodiscard]] inline ColorRgba ResolveTagBackground(TagTone ToneKind, const StudioToneSet& Tone) noexcept
{
    switch (ToneKind)
    {
        case TagTone::Amber:   return Tone.AmberWash;
        case TagTone::Green:   return Tone.GreenWash;
        case TagTone::Blue:    return Tone.BlueWash;
        case TagTone::Crimson: return Tone.CrimsonWash;
        default:               return Tone.RecessedWell;
    }
}

[[nodiscard]] inline ColorRgba ResolveTagInk(TagTone ToneKind, const StudioToneSet& Tone) noexcept
{
    switch (ToneKind)
    {
        case TagTone::Amber:   return Tone.Amber;
        case TagTone::Green:   return Tone.Green;
        case TagTone::Blue:    return Tone.Blue;
        case TagTone::Crimson: return Tone.Crimson;
        default:               return Tone.TextMuted;
    }
}

inline void ConstructCenteredWeightedText(
    PixelSpace&      Surface,
    FontWeightClass  Weight,
    Rectangle2D      Bounds,
    float            PixelSize,
    ColorRgba        Ink,
    std::string_view Utf8) noexcept
{
    if (Utf8.empty())
    {
        return;
    }
    const float TextW = Surface.MeasureTextWidth(PixelSize, Utf8.data(), Utf8.data() + Utf8.size());
    const float LeftX = Bounds.X + (Bounds.Width - TextW) * 0.5f;
    const float TopY  = Bounds.Center().Y - PixelSize * 0.54f;
    WorkspaceAppearance::ConstructWeightedText(Surface, Weight, { LeftX, TopY }, PixelSize, Ink, Utf8);
}

inline void ConstructCheckMark(PixelSpace& Surface, Point2D Center, float Scale, ColorRgba Ink, float Thickness = 1.6f) noexcept
{
    const std::array<Point2D, 3u> Strokes {{
        { Center.X - 3.3f * Scale, Center.Y + 0.1f * Scale },
        { Center.X - 0.8f * Scale, Center.Y + 2.6f * Scale },
        { Center.X + 3.6f * Scale, Center.Y - 2.6f * Scale },
    }};
    Surface.StrokePolyline(Strokes, Ink, Thickness, false);
}

inline void ConstructChevronMark(PixelSpace& Surface, Point2D Center, bool Expanded, float Size, ColorRgba Ink) noexcept
{
    if (Expanded)
    {
        const std::array<Point2D, 3u> Pts {{
            { Center.X - Size, Center.Y - Size * 0.45f },
            { Center.X,        Center.Y + Size * 0.55f },
            { Center.X + Size, Center.Y - Size * 0.45f },
        }};
        Surface.StrokePolyline(Pts, Ink, 1.35f, false);
    }
    else
    {
        const std::array<Point2D, 3u> Pts {{
            { Center.X - Size * 0.45f, Center.Y - Size },
            { Center.X + Size * 0.55f, Center.Y },
            { Center.X - Size * 0.45f, Center.Y + Size },
        }};
        Surface.StrokePolyline(Pts, Ink, 1.35f, false);
    }
}

[[nodiscard]] inline Point2D QueryInputCursor(const PaneInputReading& Input) noexcept
{
    return { Input.PrimaryPointer.X, Input.PrimaryPointer.Y };
}

[[nodiscard]] inline bool QueryPrimaryPressed(const PaneInputReading& Input) noexcept
{
    return Input.PrimaryPointer.Enabled && Input.PrimaryPointer.Pressed;
}

[[nodiscard]] inline bool QueryPrimaryDown(const PaneInputReading& Input) noexcept
{
    return Input.PrimaryPointer.Enabled && Input.PrimaryPointer.Down;
}

[[nodiscard]] inline std::string_view QueryEditTextView(const TextEditBuffer& Buffer) noexcept
{
    return std::string_view(Buffer.Text, static_cast<std::size_t>(Buffer.Length));
}

//------------------------------------------------------------------------------------------------------------------------
//                                                   SHELL CONTAINERS
//------------------------------------------------------------------------------------------------------------------------

inline Rectangle2D ConstructDockedSurfaceShell(
    PixelSpace&          Surface,
    Rectangle2D          PaneBounds,
    bool                 OutlinerVariant,
    const StudioToneSet& Tone) noexcept
{
    Surface.FillRectangle(PaneBounds, Tone.DockBackdrop, 0.0f);
    const Rectangle2D ShellRect = PaneBounds.Inset(6.0f);
    if (ShellRect.Width <= 24.0f || ShellRect.Height <= 24.0f)
    {
        return PaneBounds;
    }

    const ColorRgba TopFill    = OutlinerVariant ? Tone.OutlinerTop    : Tone.InspectorTop;
    const ColorRgba BottomFill = OutlinerVariant ? Tone.OutlinerBottom : Tone.InspectorBottom;
    const ColorRgba StrokeCol  = OutlinerVariant ? Tone.OutlinerStroke : Tone.InspectorStroke;
    constexpr float Radius     = 18.0f;

    Surface.FillRectangle(ShellRect, BottomFill, Radius);
    // Subtle top vertical sheen band approximating linear-gradient(135deg, top, bottom)
    Surface.PushClip(ShellRect);
    const float TopBandH = std::min(ShellRect.Height * 0.52f, 220.0f);
    Surface.FillRectangle({ ShellRect.X, ShellRect.Y, ShellRect.Width, TopBandH }, ColorRgba::Blend(TopFill, BottomFill, 0.25f), Radius);
    Surface.PopClip();
    Surface.StrokeRectangle(ShellRect, StrokeCol, Radius, 1.0f);
    return ShellRect;
}

inline bool ConstructRoundIconButton(
    PixelSpace&             Surface,
    Rectangle2D             ButtonRect,
    ControlIcon             Icon,
    const PaneInputReading& Input,
    const StudioToneSet&    Tone,
    bool                    Disabled = false,
    bool                    Accent   = false) noexcept
{
    const Point2D Cursor  = QueryInputCursor(Input);
    const bool    Hovered = !Disabled && ButtonRect.Contains(Cursor);
    const float   Radius  = ButtonRect.Height * 0.5f;

    const ColorRgba Fill = Accent
        ? Tone.AmberWash
        : (Hovered ? Tone.RowSelected : Tone.TileSurface);
    const ColorRgba Stroke = Accent
        ? Tone.AmberStroke
        : (Hovered ? Tone.BorderStrong : Tone.BorderSubtle);
    const ColorRgba Ink = Disabled
        ? Tone.TextFaint
        : (Accent ? Tone.Amber : (Hovered ? Tone.TextPrimary : Tone.TextMuted));

    Surface.FillRectangle(ButtonRect, Fill, Radius);
    Surface.StrokeRectangle(ButtonRect, Stroke, Radius, 1.0f);
    const float IconSize = std::min(ButtonRect.Width, ButtonRect.Height) * 0.48f;
    VectorIconRepository::ConstructControl(Surface, Icon, {
        ButtonRect.Center().X - IconSize * 0.5f,
        ButtonRect.Center().Y - IconSize * 0.5f,
        IconSize,
        Ink,
        1.9f,
    });
    return Hovered && QueryPrimaryPressed(Input);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                  OUTLINER PRIMITIVES
//------------------------------------------------------------------------------------------------------------------------

inline bool ConstructOutlinerHeading(
    PixelSpace&             Surface,
    Rectangle2D             HeadingRect,
    std::string_view        Title,
    std::string_view        Subtitle,
    ControlIcon             TrailingIcon,
    const PaneInputReading& Input,
    const StudioToneSet&    Tone,
    bool                    TrailingDisabled = false) noexcept
{
    WorkspaceAppearance::ConstructWeightedText(
        Surface, FontWeightClass::Light, { HeadingRect.X, HeadingRect.Y + 1.0f }, 19.0f, Tone.TextPrimary, Title);
    WorkspaceAppearance::ConstructWeightedText(
        Surface, FontWeightClass::Regular, { HeadingRect.X, HeadingRect.Y + 24.0f }, 10.0f, Tone.TextMuted, Subtitle);

    const Rectangle2D RoundBtn { HeadingRect.Right() - 28.0f, HeadingRect.Y + 4.0f, 28.0f, 28.0f };
    return ConstructRoundIconButton(Surface, RoundBtn, TrailingIcon, Input, Tone, TrailingDisabled, false);
}

inline int32_t ConstructStatTilePair(
    PixelSpace&                 Surface,
    Rectangle2D                 Area,
    const OutlinerStatTileSpec& LeftSpec,
    const OutlinerStatTileSpec& RightSpec,
    const PaneInputReading&     Input,
    const StudioToneSet&        Tone) noexcept
{
    constexpr float Gap = 8.0f;
    const float     TileW = (Area.Width - Gap) * 0.5f;
    const std::array<const OutlinerStatTileSpec*, 2u> Specs { &LeftSpec, &RightSpec };
    const Point2D Cursor = QueryInputCursor(Input);
    int32_t Clicked = -1;

    for (uint32_t Idx = 0u; Idx < 2u; ++Idx)
    {
        const OutlinerStatTileSpec& Spec = *Specs[Idx];
        const Rectangle2D TileRect { Area.X + static_cast<float>(Idx) * (TileW + Gap), Area.Y, TileW, Area.Height };
        const bool Hovered = TileRect.Contains(Cursor);

        const ColorRgba Fill = Spec.FilterArmed
            ? Tone.AmberWash
            : (Hovered ? Tone.RowHover : Tone.TileSurface);
        const ColorRgba Stroke = Spec.FilterArmed
            ? Tone.Amber
            : (Hovered ? Tone.BorderStrong : Tone.BorderSubtle);

        Surface.FillRectangle(TileRect, Fill, 16.0f);
        Surface.StrokeRectangle(TileRect, Stroke, 16.0f, 1.0f);

        const float   DiscRad = 12.5f;
        const Point2D DiscCenter { TileRect.X + 10.0f + DiscRad, TileRect.Center().Y };
        const ColorRgba DiscFill = Spec.ActiveGlow
            ? Tone.ToggleGreen
            : (Spec.AmberGlow ? Tone.Amber : Tone.RowSelected);
        Surface.FillCircle(DiscCenter, DiscRad, DiscFill);

        if (Spec.ActiveGlow && Spec.Icon == ControlIcon::CircleCheck)
        {
            ConstructCheckMark(Surface, DiscCenter, 1.15f, { 16u, 22u, 18u, 255u }, 1.85f);
        }
        else
        {
            const ColorRgba IconInk = (Spec.ActiveGlow || Spec.AmberGlow)
                ? ColorRgba{ 16u, 18u, 16u, 255u }
                : Tone.TextMuted;
            VectorIconRepository::ConstructControl(Surface, Spec.Icon, {
                DiscCenter.X - 6.0f,
                DiscCenter.Y - 6.0f,
                12.0f,
                IconInk,
                2.0f,
            });
        }

        const float TextLeft = DiscCenter.X + DiscRad + 8.0f;
        WorkspaceAppearance::ConstructWeightedText(
            Surface, FontWeightClass::Medium, { TextLeft, TileRect.Y + 9.0f }, 8.5f, Tone.TextMuted, Spec.Label);
        WorkspaceAppearance::ConstructWeightedText(
            Surface, FontWeightClass::Light, { TextLeft, TileRect.Y + 23.0f }, 18.0f, Tone.TextPrimary, Spec.Reading);

        if (Hovered && QueryPrimaryPressed(Input))
        {
            Clicked = static_cast<int32_t>(Idx);
        }
    }
    return Clicked;
}

inline bool ConstructSearchFilterRow(
    PixelSpace&             Surface,
    Rectangle2D             RowRect,
    TextEditBuffer&         SearchBuffer,
    std::string_view        Placeholder,
    std::string_view        FilterLabel,
    bool                    FilterActive,
    const PaneInputReading& Input,
    const StudioToneSet&    Tone) noexcept
{
    constexpr float Gap      = 7.0f;
    const float     FilterW  = std::clamp(Surface.MeasureTextWidth(10.0f, FilterLabel.data(), FilterLabel.data() + FilterLabel.size()) + 30.0f, 64.0f, 98.0f);
    const Rectangle2D SearchRect { RowRect.X, RowRect.Y, std::max(60.0f, RowRect.Width - FilterW - Gap), RowRect.Height };
    const Rectangle2D FilterRect { SearchRect.Right() + Gap, RowRect.Y, FilterW, RowRect.Height };
    const float       Radius     = RowRect.Height * 0.5f;

    // Capsule search bar
    Surface.FillRectangle(SearchRect, Tone.InsetSurface, Radius);
    Surface.StrokeRectangle(SearchRect, SearchBuffer.Active ? Tone.Amber : Tone.BorderSubtle, Radius, 1.0f);
    VectorIconRepository::ConstructControl(Surface, ControlIcon::SearchGlass, {
        SearchRect.X + 11.0f,
        SearchRect.Center().Y - 6.0f,
        12.0f,
        Tone.TextMuted,
        1.85f,
    });

    const Point2D Cursor = QueryInputCursor(Input);
    const Rectangle2D InputInner { SearchRect.X + 27.0f, SearchRect.Y + 1.0f, std::max(10.0f, SearchRect.Width - 36.0f), SearchRect.Height - 2.0f };
    if (QueryPrimaryPressed(Input) && SearchRect.Contains(Cursor) && !SearchBuffer.Active)
    {
        SearchBuffer.Begin(SearchBuffer.Text);
    }
    else if (QueryPrimaryPressed(Input) && !SearchRect.Contains(Cursor) && SearchBuffer.Active)
    {
        SearchBuffer.Finalize();
    }

    if (SearchBuffer.Active)
    {
        (void)TextInputField::Construct(Surface, InputInner, SearchBuffer, Input.PrimaryPointer, 10.5f);
    }
    else
    {
        const bool Empty = (SearchBuffer.Length == 0u);
        Surface.PushClip(InputInner);
        WorkspaceAppearance::ConstructWeightedText(
            Surface,
            FontWeightClass::Regular,
            { InputInner.X, SearchRect.Center().Y - 5.5f },
            10.5f,
            Empty ? Tone.TextFaint : Tone.TextPrimary,
            Empty ? Placeholder : QueryEditTextView(SearchBuffer));
        Surface.PopClip();
    }

    // Capsule Filter toggle button
    const bool FilterHover = FilterRect.Contains(Cursor);
    const ColorRgba FilterFill = FilterActive
        ? Tone.AmberWash
        : (FilterHover ? Tone.RowHover : Tone.InsetSurface);
    const ColorRgba FilterStroke = FilterActive
        ? Tone.Amber
        : (FilterHover ? Tone.BorderStrong : Tone.BorderSubtle);
    const ColorRgba FilterInk = FilterActive ? Tone.Amber : Tone.TextSecondary;

    Surface.FillRectangle(FilterRect, FilterFill, Radius);
    Surface.StrokeRectangle(FilterRect, FilterStroke, Radius, 1.0f);
    WorkspaceAppearance::ConstructWeightedText(
        Surface, FontWeightClass::Medium, { FilterRect.X + 12.0f, FilterRect.Center().Y - 5.2f }, 10.0f, FilterInk, FilterLabel);
    ConstructChevronMark(Surface, { FilterRect.Right() - 12.0f, FilterRect.Center().Y }, true, 3.0f, FilterInk);

    return FilterHover && QueryPrimaryPressed(Input);
}

inline int32_t ConstructQuickChipStrip(
    PixelSpace&                      Surface,
    float                            LeftX,
    float                            TopY,
    float                            MaxWidth,
    std::span<const OutlinerChipSpec> Chips,
    const PaneInputReading&          Input,
    const StudioToneSet&             Tone,
    float&                           OutConsumedHeight) noexcept
{
    const Point2D Cursor = QueryInputCursor(Input);
    constexpr float ChipH = 24.0f;
    constexpr float GapX  = 6.0f;
    constexpr float GapY  = 5.0f;

    float   CursorX = LeftX;
    float   CursorY = TopY;
    int32_t Clicked = -1;

    for (std::size_t Idx = 0u; Idx < Chips.size(); ++Idx)
    {
        const OutlinerChipSpec& Chip = Chips[Idx];
        const bool  SwatchPresent = Chip.ShowSwatch;
        const float LeadW         = (Chip.ShowPlusIcon || SwatchPresent) ? 14.0f : 0.0f;
        const float TextW         = Surface.MeasureTextWidth(9.8f, Chip.Label.data(), Chip.Label.data() + Chip.Label.size());
        const float ChipW         = std::min(MaxWidth, 18.0f + LeadW + TextW);

        if (CursorX > LeftX + 1.0f && CursorX + ChipW > LeftX + MaxWidth + 0.5f)
        {
            CursorX  = LeftX;
            CursorY += ChipH + GapY;
        }

        const Rectangle2D ChipRect { CursorX, CursorY, ChipW, ChipH };
        const bool Hovered = !Chip.Disabled && ChipRect.Contains(Cursor);

        const ColorRgba Fill = Chip.Selected
            ? Tone.PrimaryFill
            : (Hovered ? Tone.RowSelected : Tone.TileSurface);
        const ColorRgba Stroke = Chip.Selected
            ? Tone.PrimaryFill
            : (Hovered ? Tone.BorderStrong : Tone.BorderSubtle);
        const ColorRgba Ink = Chip.Disabled
            ? Tone.TextFaint
            : (Chip.Selected ? Tone.PrimaryInk : (Hovered ? Tone.TextPrimary : Tone.TextSecondary));

        Surface.FillRectangle(ChipRect, Fill, ChipH * 0.5f);
        Surface.StrokeRectangle(ChipRect, Stroke, ChipH * 0.5f, 1.0f);

        float TextX = ChipRect.X + 9.0f;
        if (SwatchPresent)
        {
            const Rectangle2D SwatchBox { ChipRect.X + 8.0f, ChipRect.Center().Y - 4.0f, 10.0f, 8.0f };
            Surface.FillRectangle({ SwatchBox.X, SwatchBox.Y, 5.5f, SwatchBox.Height }, Chip.SwatchStart, 3.0f);
            Surface.FillRectangle({ SwatchBox.X + 4.5f, SwatchBox.Y, 5.5f, SwatchBox.Height }, Chip.SwatchEnd, 3.0f);
            TextX += 14.0f;
        }
        else if (Chip.ShowPlusIcon)
        {
            VectorIconRepository::ConstructControl(Surface, ControlIcon::PlusAdd, {
                ChipRect.X + 8.0f,
                ChipRect.Center().Y - 4.5f,
                9.0f,
                Ink,
                2.1f,
            });
            TextX += 12.0f;
        }

        WorkspaceAppearance::ConstructWeightedText(
            Surface, FontWeightClass::Medium, { TextX, ChipRect.Center().Y - 5.2f }, 9.8f, Ink, Chip.Label);

        if (Hovered && QueryPrimaryPressed(Input))
        {
            Clicked = static_cast<int32_t>(Idx);
        }
        CursorX += ChipW + GapX;
    }

    OutConsumedHeight = (CursorY - TopY) + ChipH;
    return Clicked;
}

inline HierarchyRowResult ConstructHierarchyRow(
    PixelSpace&             Surface,
    Rectangle2D             RowRect,
    const HierarchyRowSpec& Spec,
    const PaneInputReading& Input,
    const StudioToneSet&    Tone) noexcept
{
    HierarchyRowResult Outcome {};
    const Point2D Cursor = QueryInputCursor(Input);
    Outcome.Hovered = RowRect.Contains(Cursor);

    if (Spec.Selected)
    {
        Surface.FillRectangle(RowRect, Tone.RowSelected, 8.0f);
        Surface.StrokeRectangle(RowRect, Tone.RowSelectedStroke, 8.0f, 1.0f);
        // Signature 3px amber left indicator bar
        Surface.FillRectangle({ RowRect.X + 1.0f, RowRect.Y + 4.0f, 3.0f, RowRect.Height - 8.0f }, Tone.Amber, 1.5f);
    }
    else if (Outcome.Hovered)
    {
        Surface.FillRectangle(RowRect, Tone.RowHover, 8.0f);
    }

    const float IndentX = RowRect.X + 8.0f + static_cast<float>(std::max(0, Spec.DepthLevel)) * 13.0f;
    float LeftX = IndentX;

    // Expand / collapse chevron hit target
    Rectangle2D ChevronRect { LeftX - 2.0f, RowRect.Y + 4.0f, 15.0f, RowRect.Height - 8.0f };
    if (Spec.Expandable)
    {
        ConstructChevronMark(Surface, { LeftX + 5.0f, RowRect.Center().Y }, Spec.Expanded, 3.0f, Tone.TextMuted);
    }
    LeftX += 14.0f;

    // 22x22 rounded icon or colour swatch tile
    const Rectangle2D LeadTile { LeftX, RowRect.Center().Y - 11.0f, 22.0f, 22.0f };
    const ColorRgba   TileFill = Spec.Muted ? Spec.LeadTileFill.ScaledAlpha(0.45f) : Spec.LeadTileFill;
    Surface.FillRectangle(LeadTile, TileFill, 6.5f);
    Surface.StrokeRectangle(LeadTile, Tone.BorderSubtle, 6.5f, 1.0f);
    if (!Spec.LeadSolidSwatch)
    {
        const ColorRgba IconInk = Spec.Muted ? Spec.LeadIconInk.ScaledAlpha(0.45f) : Spec.LeadIconInk;
        VectorIconRepository::ConstructControl(Surface, Spec.LeadIcon, {
            LeadTile.Center().X - 6.0f,
            LeadTile.Center().Y - 6.0f,
            12.0f,
            IconInk,
            1.9f,
        });
    }
    LeftX = LeadTile.Right() + 8.0f;

    // Right controls (trailing green check / visibility eye + tag chip)
    float RightX = RowRect.Right() - 8.0f;
    Rectangle2D TrailingRect {};
    if (Spec.TrailingSwitch != TrailingSwitchKind::None)
    {
        TrailingRect = { RightX - 18.0f, RowRect.Center().Y - 9.0f, 18.0f, 18.0f };
        const bool SwitchHover = TrailingRect.Contains(Cursor);

        if (Spec.TrailingSwitch == TrailingSwitchKind::GreenCheck)
        {
            const Point2D Centre = TrailingRect.Center();
            if (Spec.TrailingEnabled)
            {
                Surface.FillCircle(Centre, 7.5f, Tone.ToggleGreen);
                ConstructCheckMark(Surface, Centre, 0.85f, { 15u, 22u, 17u, 255u }, 1.65f);
            }
            else
            {
                Surface.FillCircle(Centre, 7.5f, SwitchHover ? Tone.RowHover : Tone.InsetSurface);
                Surface.StrokeCircle(Centre, 7.5f, SwitchHover ? Tone.TextMuted : Tone.BorderStrong, 1.25f);
                if (Spec.PartialIndicator)
                {
                    Surface.FillCircle(Centre, 2.5f, Tone.Amber);
                }
            }
        }
        else if (Spec.TrailingSwitch == TrailingSwitchKind::Visibility)
        {
            const ControlIcon EyeIcon = Spec.TrailingEnabled ? ControlIcon::EyeVisible : ControlIcon::EyeHidden;
            const ColorRgba   EyeInk  = Spec.TrailingEnabled
                ? (SwitchHover ? Tone.TextPrimary : Tone.TextMuted)
                : Tone.TextFaint;
            VectorIconRepository::ConstructControl(Surface, EyeIcon, {
                TrailingRect.Center().X - 6.0f,
                TrailingRect.Center().Y - 6.0f,
                12.0f,
                EyeInk,
                1.85f,
            });
        }
        RightX = TrailingRect.X - 6.0f;
    }

    if (!Spec.TagLabel.empty())
    {
        const float TagTextW = Surface.MeasureTextWidth(8.2f, Spec.TagLabel.data(), Spec.TagLabel.data() + Spec.TagLabel.size());
        const float TagW     = TagTextW + 10.0f;
        const Rectangle2D TagRect { RightX - TagW, RowRect.Center().Y - 8.0f, TagW, 16.0f };
        Surface.FillRectangle(TagRect, ResolveTagBackground(Spec.TagStyle, Tone), 5.0f);
        ConstructCenteredWeightedText(
            Surface, FontWeightClass::Medium, TagRect, 8.2f, ResolveTagInk(Spec.TagStyle, Tone), Spec.TagLabel);
        RightX = TagRect.X - 6.0f;
    }

    if (Spec.LockedSign)
    {
        VectorIconRepository::ConstructControl(Surface, ControlIcon::LockClosed, {
            RightX - 11.0f,
            RowRect.Center().Y - 5.5f,
            11.0f,
            Tone.TextFaint,
            1.85f,
        });
        RightX -= 15.0f;
    }

    // Clipped Title + Subtitle
    const float TextAreaW = std::max(16.0f, RightX - LeftX);
    Surface.PushClip({ LeftX, RowRect.Y, TextAreaW, RowRect.Height });
    const ColorRgba TitleInk = Spec.Muted
        ? Tone.TextFaint
        : (Spec.Selected ? Tone.TextPrimary : (Spec.GroupStyle ? Tone.TextSecondary : Tone.TextPrimary));
    const ColorRgba SubInk = Spec.Muted ? Tone.TextFaint : Tone.TextMuted;

    if (Spec.Subtitle.empty())
    {
        WorkspaceAppearance::ConstructWeightedText(
            Surface,
            Spec.GroupStyle ? FontWeightClass::Medium : FontWeightClass::Regular,
            { LeftX, RowRect.Center().Y - 5.5f },
            11.0f,
            TitleInk,
            Spec.Title);
    }
    else
    {
        WorkspaceAppearance::ConstructWeightedText(
            Surface,
            Spec.Selected ? FontWeightClass::Medium : FontWeightClass::Regular,
            { LeftX, RowRect.Y + 5.0f },
            11.0f,
            TitleInk,
            Spec.Title);
        WorkspaceAppearance::ConstructWeightedText(
            Surface, FontWeightClass::Regular, { LeftX, RowRect.Y + 20.0f }, 9.0f, SubInk, Spec.Subtitle);
    }
    Surface.PopClip();

    if (QueryPrimaryPressed(Input) && Outcome.Hovered)
    {
        if (Spec.TrailingSwitch != TrailingSwitchKind::None && TrailingRect.Contains(Cursor))
        {
            Outcome.TrailingToggled = true;
        }
        else if (Spec.Expandable && ChevronRect.Contains(Cursor))
        {
            Outcome.ChevronClicked = true;
        }
        else
        {
            Outcome.RowClicked = true;
        }
    }
    if (Input.DoubleClicked && Outcome.Hovered)
    {
        Outcome.DoubleClicked = true;
    }
    return Outcome;
}

inline void ConstructFourColumnFooter(
    PixelSpace&                                  Surface,
    Rectangle2D                                  FooterRect,
    const std::array<FooterMetricColumn, 4u>&    Columns,
    const StudioToneSet&                         Tone) noexcept
{
    Surface.FillRectangleBottomRounded(FooterRect, Tone.FooterSurface, 17.0f);
    Surface.StrokeLine({ FooterRect.X, FooterRect.Y }, { FooterRect.Right(), FooterRect.Y }, Tone.Hairline, 1.0f);

    const float ColW = FooterRect.Width * 0.25f;
    for (std::size_t Idx = 0u; Idx < 4u; ++Idx)
    {
        const FooterMetricColumn& Col = Columns[Idx];
        const float LeftX = FooterRect.X + static_cast<float>(Idx) * ColW + 10.0f;
        if (Idx > 0u)
        {
            Surface.StrokeLine(
                { FooterRect.X + static_cast<float>(Idx) * ColW, FooterRect.Y + 9.0f },
                { FooterRect.X + static_cast<float>(Idx) * ColW, FooterRect.Bottom() - 9.0f },
                Tone.Hairline,
                1.0f);
        }

        WorkspaceAppearance::ConstructWeightedText(
            Surface, FontWeightClass::Medium, { LeftX, FooterRect.Y + 8.0f }, 8.0f, Tone.TextFaint, Col.Label);

        const ColorRgba ReadInk = (Col.Tone == TagTone::Amber)
            ? Tone.Amber
            : (Col.Tone == TagTone::Green ? Tone.Green : Tone.TextPrimary);
        WorkspaceAppearance::ConstructWeightedText(
            Surface, FontWeightClass::Medium, { LeftX, FooterRect.Y + 21.0f }, 11.2f, ReadInk, Col.Reading);

        if (!Col.Unit.empty())
        {
            const float ValW = Surface.MeasureTextWidth(11.6f, Col.Reading.data(), Col.Reading.data() + Col.Reading.size());
            WorkspaceAppearance::ConstructWeightedText(
                Surface, FontWeightClass::Regular, { LeftX + ValW + 5.0f, FooterRect.Y + 23.0f }, 8.6f, Tone.TextMuted, Col.Unit);
        }
    }
}

//------------------------------------------------------------------------------------------------------------------------
//                                                  INSPECTOR PRIMITIVES
//------------------------------------------------------------------------------------------------------------------------

inline InspectorHeadingResult ConstructInspectorHeading(
    PixelSpace&             Surface,
    Rectangle2D             HeadingRect,
    ControlIcon             MarkIcon,
    std::string_view        Title,
    std::string_view        Eyebrow,
    bool                    ShowUndoRedo,
    bool                    UndoAvailable,
    bool                    RedoAvailable,
    bool                    ShowReset,
    const PaneInputReading& Input,
    const StudioToneSet&    Tone) noexcept
{
    InspectorHeadingResult Result {};

    // Signature 26x26 warm-amber mark tile
    const Rectangle2D MarkRect { HeadingRect.X, HeadingRect.Center().Y - 13.0f, 26.0f, 26.0f };
    Surface.FillRectangle(MarkRect, Tone.AmberWash, 8.5f);
    Surface.StrokeRectangle(MarkRect, Tone.AmberStroke, 8.5f, 1.0f);
    VectorIconRepository::ConstructControl(Surface, MarkIcon, {
        MarkRect.Center().X - 6.5f,
        MarkRect.Center().Y - 6.5f,
        13.0f,
        Tone.Amber,
        1.9f,
    });

    WorkspaceAppearance::ConstructWeightedText(
        Surface, FontWeightClass::Medium, { MarkRect.Right() + 9.0f, HeadingRect.Y + 5.0f }, 12.2f, Tone.TextPrimary, Title);
    WorkspaceAppearance::ConstructWeightedText(
        Surface, FontWeightClass::Medium, { MarkRect.Right() + 9.0f, HeadingRect.Y + 21.0f }, 8.0f, Tone.TextFaint, Eyebrow);

    float RightX = HeadingRect.Right();
    if (ShowReset)
    {
        const Rectangle2D Btn { RightX - 26.0f, HeadingRect.Center().Y - 13.0f, 26.0f, 26.0f };
        Result.ResetClicked = ConstructRoundIconButton(Surface, Btn, ControlIcon::RefreshCycle, Input, Tone);
        RightX = Btn.X - 5.0f;
    }
    if (ShowUndoRedo)
    {
        const Rectangle2D RedoBtn { RightX - 26.0f, HeadingRect.Center().Y - 13.0f, 26.0f, 26.0f };
        Result.RedoClicked = ConstructRoundIconButton(Surface, RedoBtn, ControlIcon::RedoArrow, Input, Tone, !RedoAvailable);
        RightX = RedoBtn.X - 5.0f;

        const Rectangle2D UndoBtn { RightX - 26.0f, HeadingRect.Center().Y - 13.0f, 26.0f, 26.0f };
        Result.UndoClicked = ConstructRoundIconButton(Surface, UndoBtn, ControlIcon::UndoArrow, Input, Tone, !UndoAvailable);
    }

    Surface.StrokeLine({ HeadingRect.X - 14.0f, HeadingRect.Bottom() }, { HeadingRect.Right() + 14.0f, HeadingRect.Bottom() }, Tone.Hairline, 1.0f);
    return Result;
}

inline bool ConstructIdentityBlock(
    PixelSpace&             Surface,
    Rectangle2D             BlockRect,
    std::string_view        Eyebrow,
    std::string_view        Name,
    std::string_view        Subtitle,
    std::string_view        SelectorLabel,
    bool                    SelectorOpen,
    const PaneInputReading& Input,
    const StudioToneSet&    Tone,
    Rectangle2D*            OutSelectorBounds = nullptr,
    bool                    ReserveLeftThumb  = false,
    Rectangle2D*            OutThumbBounds    = nullptr) noexcept
{
    float TextLeftX = BlockRect.X;
    if (ReserveLeftThumb)
    {
        const Rectangle2D ThumbRect { BlockRect.X, BlockRect.Y + 2.0f, 52.0f, 52.0f };
        Surface.FillRectangle(ThumbRect, Tone.InsetSurface, 12.0f);
        Surface.StrokeRectangle(ThumbRect, Tone.BorderSubtle, 12.0f, 1.0f);
        if (OutThumbBounds != nullptr)
        {
            *OutThumbBounds = ThumbRect;
        }
        TextLeftX = ThumbRect.Right() + 10.0f;
    }

    const float SelTextW = Surface.MeasureTextWidth(9.8f, SelectorLabel.data(), SelectorLabel.data() + SelectorLabel.size());
    const float SelW     = std::clamp(SelTextW + 26.0f, 68.0f, 118.0f);
    const Rectangle2D SelectorRect { BlockRect.Right() - SelW, BlockRect.Y + 14.0f, SelW, 27.0f };
    if (OutSelectorBounds != nullptr)
    {
        *OutSelectorBounds = SelectorRect;
    }

    const float TitleClipW = std::max(40.0f, SelectorRect.X - TextLeftX - 8.0f);
    const float TitleSize  = (ReserveLeftThumb || Name.size() > 20u) ? 14.8f : 17.0f;
    Surface.PushClip({ TextLeftX, BlockRect.Y, TitleClipW, BlockRect.Height });
    WorkspaceAppearance::ConstructWeightedText(
        Surface, FontWeightClass::Medium, { TextLeftX, BlockRect.Y + 1.0f }, 8.2f, Tone.TextFaint, Eyebrow);
    WorkspaceAppearance::ConstructWeightedText(
        Surface, FontWeightClass::Light, { TextLeftX, BlockRect.Y + 15.0f }, TitleSize, Tone.TextPrimary, Name);
    WorkspaceAppearance::ConstructWeightedText(
        Surface, FontWeightClass::Regular, { TextLeftX, BlockRect.Y + 37.0f }, 9.6f, Tone.TextMuted, Subtitle);
    Surface.PopClip();

    const Point2D Cursor = QueryInputCursor(Input);
    const bool    Hovered = SelectorRect.Contains(Cursor);
    Surface.FillRectangle(SelectorRect, SelectorOpen ? Tone.RowSelected : (Hovered ? Tone.RowHover : Tone.InsetSurface), 13.5f);
    Surface.StrokeRectangle(SelectorRect, SelectorOpen ? Tone.Amber : (Hovered ? Tone.BorderStrong : Tone.BorderSubtle), 13.5f, 1.0f);
    WorkspaceAppearance::ConstructWeightedText(
        Surface, FontWeightClass::Medium, { SelectorRect.X + 10.0f, SelectorRect.Center().Y - 5.2f }, 9.8f, Tone.TextSecondary, SelectorLabel);
    ConstructChevronMark(Surface, { SelectorRect.Right() - 11.0f, SelectorRect.Center().Y }, !SelectorOpen, 2.8f, Tone.TextMuted);

    return Hovered && QueryPrimaryPressed(Input);
}

inline void ConstructMeterStatTrio(
    PixelSpace&                              Surface,
    Rectangle2D                              Area,
    const std::array<MeterStatTileSpec, 3u>& Tiles,
    const StudioToneSet&                     Tone) noexcept
{
    constexpr float Gap   = 7.0f;
    const float     TileW = (Area.Width - Gap * 2.0f) / 3.0f;

    for (std::size_t Idx = 0u; Idx < 3u; ++Idx)
    {
        const MeterStatTileSpec& Spec = Tiles[Idx];
        const Rectangle2D TileRect { Area.X + static_cast<float>(Idx) * (TileW + Gap), Area.Y, TileW, Area.Height };

        const ColorRgba TileFill   = Spec.PrimaryAccent ? ColorRgba::Blend(Tone.InsetSurface, Tone.AmberWash, 0.55f) : Tone.InsetSurface;
        const ColorRgba TileStroke = Spec.PrimaryAccent ? Tone.AmberStroke : Tone.BorderSubtle;
        Surface.FillRectangle(TileRect, TileFill, 13.0f);
        Surface.StrokeRectangle(TileRect, TileStroke, 13.0f, 1.0f);

        const float PadX = 9.0f;
        Surface.PushClip(TileRect.InsetXY(PadX, 4.0f));
        WorkspaceAppearance::ConstructWeightedText(
            Surface, FontWeightClass::Medium, { TileRect.X + PadX, TileRect.Y + 8.0f }, 8.5f, Tone.TextMuted, Spec.Label);

        const ColorRgba ReadInk = (Spec.ReadingTone == TagTone::Amber)
            ? Tone.Amber
            : (Spec.ReadingTone == TagTone::Green ? Tone.Green
            : (Spec.ReadingTone == TagTone::Crimson ? Tone.Crimson
            : (Spec.PrimaryAccent ? Tone.Amber : Tone.TextPrimary)));
        WorkspaceAppearance::ConstructWeightedText(
            Surface, FontWeightClass::Light, { TileRect.X + PadX, TileRect.Y + 21.0f }, 16.0f, ReadInk, Spec.Reading);

        if (!Spec.Unit.empty())
        {
            const float ValW = Surface.MeasureTextWidth(16.6f, Spec.Reading.data(), Spec.Reading.data() + Spec.Reading.size());
            WorkspaceAppearance::ConstructWeightedText(
                Surface, FontWeightClass::Regular, { TileRect.X + PadX + ValW + 5.5f, TileRect.Y + 27.0f }, 8.6f, Tone.TextMuted, Spec.Unit);
        }

        // Inline Project Zero meter strip
        const Rectangle2D MeterRect { TileRect.X + PadX, TileRect.Y + 44.0f, TileRect.Width - PadX * 2.0f, 4.5f };
        if (Spec.MeterKind == MeterVariant::FillBar)
        {
            Surface.FillRectangle(MeterRect, Tone.RecessedWell, 2.25f);
            const float FillW = std::clamp(Spec.MeterRatio, 0.0f, 1.0f) * MeterRect.Width;
            if (FillW > 1.0f)
            {
                Surface.FillRectangle({ MeterRect.X, MeterRect.Y, FillW, MeterRect.Height }, Spec.MeterColour, 2.25f);
            }
        }
        else if (Spec.MeterKind == MeterVariant::BandDot)
        {
            Surface.FillRectangle(MeterRect, Tone.RecessedWell, 2.25f);
            const float LoX = MeterRect.X + std::clamp(Spec.BandLow, 0.0f, 1.0f) * MeterRect.Width;
            const float HiX = MeterRect.X + std::clamp(Spec.BandHigh, 0.0f, 1.0f) * MeterRect.Width;
            if (HiX > LoX + 1.0f)
            {
                Surface.FillRectangle({ LoX, MeterRect.Y, HiX - LoX, MeterRect.Height }, Tone.RowSelected, 2.0f);
            }
            const float DotX = MeterRect.X + std::clamp(Spec.MeterRatio, 0.0f, 1.0f) * MeterRect.Width;
            Surface.FillCircle({ DotX, MeterRect.Center().Y }, 3.2f, Spec.MeterColour);
        }
        else if (Spec.MeterKind == MeterVariant::StackBar)
        {
            Surface.FillRectangle(MeterRect, Tone.RecessedWell, 2.25f);
            float Total = 0.0f;
            for (uint32_t P = 0u; P < Spec.StackCount; ++P) Total += std::max(0.0f, Spec.StackParts[P].Share);
            if (Total > 1.0e-5f)
            {
                float RunX = MeterRect.X;
                for (uint32_t P = 0u; P < Spec.StackCount; ++P)
                {
                    const float SegW = (std::max(0.0f, Spec.StackParts[P].Share) / Total) * MeterRect.Width;
                    if (SegW > 0.5f)
                    {
                        Surface.FillRectangle({ RunX, MeterRect.Y, std::max(1.5f, SegW - 1.0f), MeterRect.Height }, Spec.StackParts[P].Colour, 2.0f);
                    }
                    RunX += SegW;
                }
            }
        }
        else if (Spec.MeterKind == MeterVariant::SwatchLine && Spec.SwatchCount > 0u)
        {
            const float GapS  = 3.0f;
            const float EachW = std::min(14.0f, (MeterRect.Width - GapS * static_cast<float>(Spec.SwatchCount - 1u)) / static_cast<float>(Spec.SwatchCount));
            for (uint32_t S = 0u; S < Spec.SwatchCount; ++S)
            {
                Surface.FillRectangle({ MeterRect.X + static_cast<float>(S) * (EachW + GapS), MeterRect.Y - 1.0f, EachW, 6.0f }, Spec.Swatches[S], 3.0f);
            }
        }
        else if (Spec.MeterKind == MeterVariant::StepTicks && Spec.StepTotal > 0)
        {
            const int32_t Count = std::clamp(Spec.StepTotal, 1, 10);
            const float   GapT  = 2.0f;
            const float   SegW  = (MeterRect.Width - GapT * static_cast<float>(Count - 1)) / static_cast<float>(Count);
            for (int32_t T = 0; T < Count; ++T)
            {
                const ColorRgba SegFill = (T < Spec.StepDone) ? Tone.Green : Tone.RecessedWell;
                Surface.FillRectangle({ MeterRect.X + static_cast<float>(T) * (SegW + GapT), MeterRect.Y, SegW, MeterRect.Height }, SegFill, 2.0f);
            }
        }

        WorkspaceAppearance::ConstructWeightedText(
            Surface, FontWeightClass::Regular, { TileRect.X + PadX, TileRect.Y + 55.0f }, 8.6f, Tone.TextFaint, Spec.Caption);
        Surface.PopClip();
    }
}

inline bool ConstructAccordionRowHeader(
    PixelSpace&             Surface,
    Rectangle2D             RowRect,
    const AccordionRowSpec& Spec,
    bool                    Expanded,
    const PaneInputReading& Input,
    const StudioToneSet&    Tone) noexcept
{
    const Point2D Cursor = QueryInputCursor(Input);
    const bool    Hovered = RowRect.Contains(Cursor);

    if (!Expanded && Hovered)
    {
        Surface.FillRectangle(RowRect, Tone.RowHover, 10.0f);
    }

    // Leading 22x22 live swatch or icon box
    const Rectangle2D SwatchRect { RowRect.X + 8.0f, RowRect.Center().Y - 11.0f, 22.0f, 22.0f };
    if (Spec.SolidSwatch)
    {
        Surface.FillRectangle(SwatchRect, Spec.SwatchColour, 6.5f);
        Surface.StrokeRectangle(SwatchRect, Tone.BorderStrong, 6.5f, 1.0f);
    }
    else if (Spec.ScalarBarSwatch)
    {
        Surface.FillRectangle(SwatchRect, Tone.RecessedWell, 6.5f);
        const float BarH = std::clamp(Spec.ScalarFill, 0.05f, 1.0f) * (SwatchRect.Height - 4.0f);
        Surface.FillRectangle(
            { SwatchRect.X + 2.0f, SwatchRect.Bottom() - 2.0f - BarH, SwatchRect.Width - 4.0f, BarH },
            Spec.DrivenWarm ? Tone.Amber : Tone.TextSecondary,
            3.5f);
        Surface.StrokeRectangle(SwatchRect, Tone.BorderSubtle, 6.5f, 1.0f);
    }
    else
    {
        Surface.FillRectangle(SwatchRect, Tone.TileSurface, 6.5f);
        Surface.StrokeRectangle(SwatchRect, Tone.BorderSubtle, 6.5f, 1.0f);
        VectorIconRepository::ConstructControl(Surface, Spec.LeadIcon, {
            SwatchRect.Center().X - 6.0f,
            SwatchRect.Center().Y - 6.0f,
            12.0f,
            Spec.DrivenWarm ? Tone.Amber : Tone.TextMuted,
            1.85f,
        });
    }

    // Right readout capsule
    const float ReadTextW = Surface.MeasureTextWidth(9.5f, Spec.Readout.data(), Spec.Readout.data() + Spec.Readout.size());
    const float ReadW     = std::clamp(ReadTextW + 16.0f, 40.0f, 118.0f);
    const Rectangle2D ReadRect { RowRect.Right() - ReadW - 8.0f, RowRect.Center().Y - 10.5f, ReadW, 21.0f };

    Surface.FillRectangle(ReadRect, Spec.DrivenWarm ? Tone.ReadoutWarm : Tone.ReadoutDark, 10.5f);
    Surface.StrokeRectangle(ReadRect, Spec.DrivenWarm ? Tone.AmberStroke : Tone.Hairline, 10.5f, 1.0f);
    ConstructCenteredWeightedText(
        Surface,
        FontWeightClass::Medium,
        ReadRect,
        9.4f,
        Spec.DrivenWarm ? Tone.Amber : Tone.TextSecondary,
        Spec.Readout);

    // Label + Hint
    const float TextLeftX = SwatchRect.Right() + 9.0f;
    const float TextClipW = std::max(24.0f, ReadRect.X - TextLeftX - 8.0f);
    Surface.PushClip({ TextLeftX, RowRect.Y, TextClipW, RowRect.Height });
    WorkspaceAppearance::ConstructWeightedText(
        Surface,
        Expanded ? FontWeightClass::Medium : FontWeightClass::Regular,
        { TextLeftX, RowRect.Y + 5.5f },
        11.2f,
        Tone.TextPrimary,
        Spec.Title);
    WorkspaceAppearance::ConstructWeightedText(
        Surface, FontWeightClass::Regular, { TextLeftX, RowRect.Y + 20.5f }, 9.0f, Tone.TextMuted, Spec.Subtitle);
    Surface.PopClip();

    return Hovered && QueryPrimaryPressed(Input);
}

inline int32_t ConstructSegmentedCapsuleBar(
    PixelSpace&                        Surface,
    Rectangle2D                        BarRect,
    std::span<const std::string_view>  Labels,
    int32_t                            SelectedIndex,
    const PaneInputReading&            Input,
    const StudioToneSet&               Tone) noexcept
{
    if (Labels.empty())
    {
        return -1;
    }

    const float Radius = BarRect.Height * 0.5f;
    Surface.FillRectangle(BarRect, Tone.RecessedWell, Radius);
    Surface.StrokeRectangle(BarRect, Tone.Hairline, Radius, 1.0f);

    const Point2D Cursor = QueryInputCursor(Input);
    const float   Pad    = 3.0f;
    const float   InnerW = BarRect.Width - Pad * 2.0f;
    const float   SegW   = InnerW / static_cast<float>(Labels.size());
    int32_t       Picked = -1;

    for (std::size_t Idx = 0u; Idx < Labels.size(); ++Idx)
    {
        const Rectangle2D SegRect {
            BarRect.X + Pad + static_cast<float>(Idx) * SegW,
            BarRect.Y + Pad,
            SegW,
            BarRect.Height - Pad * 2.0f,
        };
        const bool Selected = (static_cast<int32_t>(Idx) == SelectedIndex);
        const bool Hovered  = SegRect.Contains(Cursor);

        if (Selected)
        {
            Surface.FillRectangle(SegRect, Tone.PrimaryFill, SegRect.Height * 0.5f);
        }
        else if (Hovered)
        {
            Surface.FillRectangle(SegRect, Tone.RowHover, SegRect.Height * 0.5f);
        }

        const ColorRgba Ink = Selected ? Tone.PrimaryInk : (Hovered ? Tone.TextPrimary : Tone.TextMuted);
        ConstructCenteredWeightedText(
            Surface, Selected ? FontWeightClass::Medium : FontWeightClass::Regular, SegRect, 9.6f, Ink, Labels[Idx]);

        if (Hovered && QueryPrimaryPressed(Input))
        {
            Picked = static_cast<int32_t>(Idx);
        }
    }
    return Picked;
}

inline bool ConstructStudioKnobSlider(
    PixelSpace&             Surface,
    Rectangle2D             Area,
    int32_t                 SliderId,
    int32_t&                ActiveSliderId,
    std::string_view        Label,
    float&                  InOutReading,
    float                   MinLimit,
    float                   MaxLimit,
    std::string_view        UnitSuffix,
    int32_t                 Decimals,
    const PaneInputReading& Input,
    const StudioToneSet&    Tone) noexcept
{
    char NumBuf[32] {};
    if (Decimals <= 0)
    {
        std::snprintf(NumBuf, sizeof(NumBuf), "%d", static_cast<int>(std::lround(InOutReading)));
    }
    else if (Decimals == 1)
    {
        std::snprintf(NumBuf, sizeof(NumBuf), "%.1f", InOutReading);
    }
    else
    {
        std::snprintf(NumBuf, sizeof(NumBuf), "%.2f", InOutReading);
    }

    // Header row: Label left + split reading|unit capsule right
    WorkspaceAppearance::ConstructWeightedText(
        Surface, FontWeightClass::Regular, { Area.X, Area.Y + 2.0f }, 10.2f, Tone.TextMuted, Label);

    const float NumW  = Surface.MeasureTextWidth(9.6f, NumBuf) + 12.0f;
    const float UnitW = UnitSuffix.empty() ? 0.0f : (Surface.MeasureTextWidth(9.0f, UnitSuffix.data(), UnitSuffix.data() + UnitSuffix.size()) + 10.0f);
    const float BoxW  = NumW + UnitW;
    const Rectangle2D ReadBox { Area.Right() - BoxW, Area.Y, BoxW, 18.0f };
    Surface.FillRectangle(ReadBox, Tone.RecessedWell, 6.0f);
    if (UnitW > 1.0f)
    {
        Surface.FillRectangle({ ReadBox.X + NumW, ReadBox.Y, UnitW, ReadBox.Height }, Tone.TileSurface, 5.0f);
        ConstructCenteredWeightedText(
            Surface, FontWeightClass::Regular, { ReadBox.X + NumW, ReadBox.Y, UnitW, ReadBox.Height }, 8.8f, Tone.TextMuted, UnitSuffix);
    }
    Surface.StrokeRectangle(ReadBox, Tone.Hairline, 6.0f, 1.0f);
    ConstructCenteredWeightedText(
        Surface, FontWeightClass::Medium, { ReadBox.X, ReadBox.Y, NumW, ReadBox.Height }, 9.4f, Tone.TextPrimary, NumBuf);

    // 20px Project Zero recessed capsule track + solid 16px circular light thumb
    const Rectangle2D TrackRect { Area.X, Area.Y + 22.0f, Area.Width, 18.0f };
    const Point2D     Cursor = QueryInputCursor(Input);
    const bool        TrackHit = TrackRect.Contains(Cursor);

    if (TrackHit && QueryPrimaryPressed(Input))
    {
        ActiveSliderId = SliderId;
    }
    if (!QueryPrimaryDown(Input) && ActiveSliderId == SliderId)
    {
        ActiveSliderId = -1;
    }

    bool Changed = false;
    const float Span = std::max(1.0e-5f, MaxLimit - MinLimit);
    if (ActiveSliderId == SliderId && QueryPrimaryDown(Input))
    {
        const float Pad   = 9.0f;
        const float Ratio = std::clamp((Input.PrimaryPointer.X - (TrackRect.X + Pad)) / std::max(1.0f, TrackRect.Width - Pad * 2.0f), 0.0f, 1.0f);
        const float Next  = MinLimit + Ratio * Span;
        if (std::fabs(Next - InOutReading) > 1.0e-5f)
        {
            InOutReading = Next;
            Changed      = true;
        }
    }

    const float Norm     = std::clamp((InOutReading - MinLimit) / Span, 0.0f, 1.0f);
    const float ThumbRad = 7.5f;
    const float ThumbX   = TrackRect.X + ThumbRad + 2.0f + Norm * std::max(1.0f, TrackRect.Width - (ThumbRad + 2.0f) * 2.0f);

    Surface.FillRectangle(TrackRect, Tone.RecessedWell, TrackRect.Height * 0.5f);
    Surface.FillRectangle(
        { TrackRect.X + 2.0f, TrackRect.Y + 2.0f, std::max(TrackRect.Height - 4.0f, (ThumbX - TrackRect.X) + ThumbRad), TrackRect.Height - 4.0f },
        Tone.RowSelected,
        (TrackRect.Height - 4.0f) * 0.5f);
    Surface.StrokeRectangle(TrackRect, (ActiveSliderId == SliderId) ? Tone.Amber : Tone.BorderSubtle, TrackRect.Height * 0.5f, 1.0f);
    Surface.FillCircle({ ThumbX, TrackRect.Center().Y }, ThumbRad, Tone.PrimaryFill);
    return Changed;
}

inline bool ConstructStudioToggleRow(
    PixelSpace&             Surface,
    Rectangle2D             RowRect,
    std::string_view        Title,
    std::string_view        Hint,
    bool&                   InOutEnabled,
    const PaneInputReading& Input,
    const StudioToneSet&    Tone) noexcept
{
    const Point2D Cursor = QueryInputCursor(Input);
    const bool    Hovered = RowRect.Contains(Cursor);

    const Rectangle2D SwitchRect { RowRect.Right() - 36.0f, RowRect.Center().Y - 10.0f, 36.0f, 20.0f };
    const float       TextClipW = std::max(24.0f, SwitchRect.X - RowRect.X - 8.0f);

    Surface.PushClip({ RowRect.X, RowRect.Y, TextClipW, RowRect.Height });
    WorkspaceAppearance::ConstructWeightedText(
        Surface, FontWeightClass::Regular, { RowRect.X, RowRect.Y + (Hint.empty() ? 8.0f : 3.0f) }, 10.8f, Tone.TextPrimary, Title);
    if (!Hint.empty())
    {
        WorkspaceAppearance::ConstructWeightedText(
            Surface, FontWeightClass::Regular, { RowRect.X, RowRect.Y + 18.0f }, 8.8f, Tone.TextMuted, Hint);
    }
    Surface.PopClip();

    Surface.FillRectangle(SwitchRect, InOutEnabled ? Tone.ToggleGreen : Tone.RecessedWell, 10.0f);
    Surface.StrokeRectangle(SwitchRect, InOutEnabled ? Tone.ToggleGreen : Tone.BorderStrong, 10.0f, 1.0f);
    const float KnobX = InOutEnabled ? (SwitchRect.Right() - 10.0f) : (SwitchRect.X + 10.0f);
    const ColorRgba KnobCol = InOutEnabled ? ColorRgba{ 248u, 248u, 248u, 255u } : ColorRgba{ 146u, 146u, 152u, 255u };
    Surface.FillCircle({ KnobX, SwitchRect.Center().Y }, 7.2f, KnobCol);

    if (Hovered && QueryPrimaryPressed(Input))
    {
        InOutEnabled = !InOutEnabled;
        return true;
    }
    return false;
}

inline bool ConstructCollapsibleFoldHeader(
    PixelSpace&             Surface,
    Rectangle2D             FoldRect,
    std::string_view        Title,
    std::string_view        SummaryRight,
    bool                    Expanded,
    const PaneInputReading& Input,
    const StudioToneSet&    Tone) noexcept
{
    const Point2D Cursor = QueryInputCursor(Input);
    const bool    Hovered = FoldRect.Contains(Cursor);

    ConstructChevronMark(Surface, { FoldRect.X + 14.0f, FoldRect.Center().Y }, Expanded, 3.2f, Tone.TextMuted);
    WorkspaceAppearance::ConstructWeightedText(
        Surface, FontWeightClass::Medium, { FoldRect.X + 26.0f, FoldRect.Center().Y - 5.5f }, 11.2f, Tone.TextPrimary, Title);

    if (!SummaryRight.empty())
    {
        WorkspaceAppearance::ConstructRightAlignedText(
            Surface, FontWeightClass::Regular, { FoldRect.Right() - 12.0f, FoldRect.Center().Y - 5.0f }, 9.4f, Tone.TextMuted, SummaryRight);
    }
    return Hovered && QueryPrimaryPressed(Input);
}

inline InspectorFooterResult ConstructInspectorFooterBar(
    PixelSpace&             Surface,
    Rectangle2D             FooterRect,
    std::string_view        PrimaryLabel,
    std::string_view        SecondaryALabel,
    std::string_view        SecondaryBLabel,
    bool                    StatusPositive,
    std::string_view        StatusCaption,
    const PaneInputReading& Input,
    const StudioToneSet&    Tone) noexcept
{
    InspectorFooterResult Outcome {};
    Surface.FillRectangleBottomRounded(FooterRect, Tone.FooterSurface, 17.0f);
    Surface.StrokeLine({ FooterRect.X, FooterRect.Y }, { FooterRect.Right(), FooterRect.Y }, Tone.Hairline, 1.0f);

    const Point2D Cursor = QueryInputCursor(Input);
    const float   PadX = 12.0f;
    const float   BtnY = FooterRect.Y + 9.0f;
    constexpr float BtnH = 26.0f;
    constexpr float Gap  = 6.0f;

    const int32_t BtnCount = 1 + (!SecondaryALabel.empty() ? 1 : 0) + (!SecondaryBLabel.empty() ? 1 : 0);
    const float   TotalW   = FooterRect.Width - PadX * 2.0f;
    const float   PrimW    = (BtnCount == 1) ? TotalW : (BtnCount == 2 ? (TotalW - Gap) * 0.56f : (TotalW - Gap * 2.0f) * 0.42f);
    const float   SecW     = (BtnCount > 1) ? (TotalW - PrimW - Gap * static_cast<float>(BtnCount - 1)) / static_cast<float>(BtnCount - 1) : 0.0f;

    // White primary command capsule
    const Rectangle2D PrimRect { FooterRect.X + PadX, BtnY, PrimW, BtnH };
    const bool PrimHover = PrimRect.Contains(Cursor);
    Surface.FillRectangle(PrimRect, PrimHover ? ColorRgba{ 250u, 250u, 250u, 255u } : Tone.PrimaryFill, BtnH * 0.5f);
    ConstructCenteredWeightedText(
        Surface, FontWeightClass::Medium, PrimRect, 10.2f, Tone.PrimaryInk, PrimaryLabel);
    Outcome.PrimaryClicked = PrimHover && QueryPrimaryPressed(Input);

    float RunX = PrimRect.Right() + Gap;
    if (!SecondaryALabel.empty())
    {
        const Rectangle2D SecRect { RunX, BtnY, SecW, BtnH };
        const bool Hover = SecRect.Contains(Cursor);
        Surface.FillRectangle(SecRect, Hover ? Tone.RowSelected : Tone.FoldSurface, BtnH * 0.5f);
        Surface.StrokeRectangle(SecRect, Hover ? Tone.BorderStrong : Tone.BorderSubtle, BtnH * 0.5f, 1.0f);
        ConstructCenteredWeightedText(
            Surface, FontWeightClass::Medium, SecRect, 9.8f, Hover ? Tone.TextPrimary : Tone.TextSecondary, SecondaryALabel);
        Outcome.SecondaryAClicked = Hover && QueryPrimaryPressed(Input);
        RunX = SecRect.Right() + Gap;
    }
    if (!SecondaryBLabel.empty())
    {
        const Rectangle2D SecRect { RunX, BtnY, SecW, BtnH };
        const bool Hover = SecRect.Contains(Cursor);
        Surface.FillRectangle(SecRect, Hover ? Tone.RowSelected : Tone.FoldSurface, BtnH * 0.5f);
        Surface.StrokeRectangle(SecRect, Hover ? Tone.BorderStrong : Tone.BorderSubtle, BtnH * 0.5f, 1.0f);
        ConstructCenteredWeightedText(
            Surface, FontWeightClass::Medium, SecRect, 9.8f, Hover ? Tone.TextPrimary : Tone.TextSecondary, SecondaryBLabel);
        Outcome.SecondaryBClicked = Hover && QueryPrimaryPressed(Input);
    }

    // Quiet status indicator line below buttons
    const float StatusY = BtnY + BtnH + 10.0f;
    Surface.FillCircle({ FooterRect.X + PadX + 4.0f, StatusY + 4.5f }, 3.0f, StatusPositive ? Tone.Green : Tone.Amber);
    Surface.PushClip({ FooterRect.X + PadX + 12.0f, StatusY - 1.0f, FooterRect.Width - PadX * 2.0f - 12.0f, 14.0f });
    WorkspaceAppearance::ConstructWeightedText(
        Surface, FontWeightClass::Regular, { FooterRect.X + PadX + 12.0f, StatusY }, 9.0f, Tone.TextMuted, StatusCaption);
    Surface.PopClip();

    return Outcome;
}

inline int32_t ConstructStudioPopoverList(
    PixelSpace&                        Surface,
    Rectangle2D                        AnchorRect,
    Rectangle2D                        ClipBounds,
    std::string_view                   Heading,
    std::span<const PopoverOptionSpec> Options,
    int32_t                            SelectedIndex,
    const PaneInputReading&            Input,
    const StudioToneSet&               Tone,
    bool&                              OutDismissed) noexcept
{
    OutDismissed = false;
    const float RowH  = 30.0f;
    const float HeadH = Heading.empty() ? 8.0f : 26.0f;
    const float PopW  = std::clamp(std::max(AnchorRect.Width, 210.0f), 190.0f, std::max(190.0f, ClipBounds.Width - 16.0f));
    const float MaxH  = std::min(320.0f, ClipBounds.Height - 24.0f);
    const float PopH  = std::min(MaxH, HeadH + static_cast<float>(Options.size()) * RowH + 8.0f);

    float PopX = std::clamp(AnchorRect.Right() - PopW, ClipBounds.X + 8.0f, ClipBounds.Right() - PopW - 8.0f);
    float PopY = AnchorRect.Bottom() + 6.0f;
    if (PopY + PopH > ClipBounds.Bottom() - 8.0f)
    {
        PopY = std::max(ClipBounds.Y + 8.0f, AnchorRect.Y - PopH - 6.0f);
    }

    const Rectangle2D PopRect { PopX, PopY, PopW, PopH };
    Surface.FillRectangle({ PopRect.X, PopRect.Y + 4.0f, PopRect.Width, PopRect.Height }, { 0u, 0u, 0u, 110u }, 15.0f);
    Surface.FillRectangle(PopRect, Tone.OutlinerTop, 14.0f);
    Surface.StrokeRectangle(PopRect, Tone.BorderStrong, 14.0f, 1.0f);

    if (!Heading.empty())
    {
        WorkspaceAppearance::ConstructWeightedText(
            Surface, FontWeightClass::Medium, { PopRect.X + 12.0f, PopRect.Y + 8.0f }, 8.5f, Tone.TextFaint, Heading);
        Surface.StrokeLine({ PopRect.X + 8.0f, PopRect.Y + HeadH - 2.0f }, { PopRect.Right() - 8.0f, PopRect.Y + HeadH - 2.0f }, Tone.Hairline, 1.0f);
    }

    const Point2D Cursor = QueryInputCursor(Input);
    int32_t       Picked = -1;

    Surface.PushClip({ PopRect.X + 4.0f, PopRect.Y + HeadH, PopRect.Width - 8.0f, PopRect.Height - HeadH - 4.0f });
    for (std::size_t Idx = 0u; Idx < Options.size(); ++Idx)
    {
        const Rectangle2D OptionRect { PopRect.X + 6.0f, PopRect.Y + HeadH + static_cast<float>(Idx) * RowH, PopRect.Width - 12.0f, RowH - 2.0f };
        if (OptionRect.Bottom() > PopRect.Bottom() - 4.0f)
        {
            break;
        }
        const bool Selected = (static_cast<int32_t>(Idx) == SelectedIndex);
        const bool Hovered  = OptionRect.Contains(Cursor);

        if (Selected || Hovered)
        {
            Surface.FillRectangle(OptionRect, Selected ? Tone.RowSelected : Tone.RowHover, 8.0f);
        }

        float TextX = OptionRect.X + 8.0f;
        if (Options[Idx].SwatchDot.Alpha > 0u)
        {
            Surface.FillCircle({ TextX + 5.0f, OptionRect.Center().Y }, 5.0f, Options[Idx].SwatchDot);
            TextX += 16.0f;
        }

        WorkspaceAppearance::ConstructWeightedText(
            Surface,
            Selected ? FontWeightClass::Medium : FontWeightClass::Regular,
            { TextX, OptionRect.Center().Y - 5.2f },
            10.2f,
            Selected ? Tone.TextPrimary : Tone.TextSecondary,
            Options[Idx].Label);

        if (Selected)
        {
            ConstructCheckMark(Surface, { OptionRect.Right() - 12.0f, OptionRect.Center().Y }, 0.9f, Tone.Green, 1.7f);
        }
        else if (!Options[Idx].Detail.empty())
        {
            WorkspaceAppearance::ConstructRightAlignedText(
                Surface, FontWeightClass::Regular, { OptionRect.Right() - 8.0f, OptionRect.Center().Y - 4.8f }, 8.6f, Tone.TextFaint, Options[Idx].Detail);
        }

        if (Hovered && QueryPrimaryPressed(Input))
        {
            Picked = static_cast<int32_t>(Idx);
        }
    }
    Surface.PopClip();

    if (QueryPrimaryPressed(Input) && !PopRect.Contains(Cursor) && !AnchorRect.Contains(Cursor))
    {
        OutDismissed = true;
    }
    return Picked;
}

} // namespace Frontier::Workspaces::StudioSurface
