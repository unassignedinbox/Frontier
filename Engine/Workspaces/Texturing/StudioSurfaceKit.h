/*============================================================================================================================================
                                                           STUDIOSURFACEKIT.H
============================================================================================================================================*/
// 🧩 Shared Frontier TextureStudio surface tokens, pane-direct fills, sharp outliner rows, white-on-color icons, KPI tiles, category groups, and sliders

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

enum class TagTone : uint8_t
{
    Neutral = 0, // Quiet grey capsule tag
    Amber   = 1, // Warm highlight (ACTIVE / STALE / BAKING)
    Green   = 2, // Fresh green highlight (SEAMLESS / READY / DONE)
    Blue    = 3, // Sky blue highlight (CUTOUT / CUSTOM)
    Crimson = 4, // Alert red highlight
};

enum class IconAccentTone : uint8_t
{
    Slate  = 0, // Neutral dark studio tile with white icon
    Green  = 1, // Solid green tile with white icon
    Blue   = 2, // Solid studio blue tile with white icon
    Amber  = 3, // Solid burnt-amber tile with white icon
    Violet = 4, // Solid studio violet tile with white icon
    Teal   = 5, // Solid studio teal tile with white icon
};

//------------------------------------------------------------------------------------------------------------------------
//                                                       STRUCTURES
//------------------------------------------------------------------------------------------------------------------------

struct StudioToneSet
{
    ColorRgba PaneSurface         {}; // Direct edge-to-edge outliner / inspector fill
    ColorRgba PaneHeaderSurface   {}; // Edge-to-edge top heading strip fill
    ColorRgba SectionHeaderBg     {}; // Categorized section top header strip
    ColorRgba SectionBodyBg       {}; // Categorized section body fill
    ColorRgba InsetSurface        {}; // Dark KPI tile & inset container fill
    ColorRgba TileSurface         {}; // Outliner stat tile & chip fill
    ColorRgba FooterSurface       {}; // Edge-to-edge bottom footer fill
    ColorRgba ToolbarSurface      {}; // Bottom outliner command strip fill
    ColorRgba GroundRowSurface    {}; // Bottom canvas/layer-0 reference row fill
    ColorRgba FieldWell           {}; // #050506 deep numeric capsule well
    ColorRgba UnitWell            {}; // #161618 split unit cell well
    ColorRgba RecessedWell        {}; // Track / recessed background fill
    ColorRgba SliderTrack         {}; // #232326 unfilled thick capsule slider track
    ColorRgba SliderFill          {}; // #4C4D52 filled thick capsule slider bar
    ColorRgba SliderThumb         {}; // #EBEBEB solid circular thumb
    ColorRgba RowSurface          {}; // #17181C sharp outliner row background
    ColorRgba RowHover            {}; // #1D1F24 hover row background
    ColorRgba RowSelected         {}; // #232520 selected row background
    ColorRgba RowSelectedStroke   {}; // Lime-cream subtle selected row stroke
    ColorRgba HighlightFill       {}; // #EFF4CF TextureStudio lime-cream highlight
    ColorRgba HighlightInk        {}; // #17190E crisp dark ink over HighlightFill
    ColorRgba HighlightMutedInk   {}; // Sub-label ink over HighlightFill
    ColorRgba Hairline            {}; // Subtle internal separator
    ColorRgba BorderSubtle        {}; // Standard control outline
    ColorRgba BorderStrong        {}; // Emphasized outline
    ColorRgba PrimaryFill         {}; // Active segment & primary CTA fill
    ColorRgba PrimaryInk          {}; // Crisp ink over PrimaryFill
    ColorRgba WhiteInk            { 255u, 255u, 255u, 255u };
    ColorRgba AccentGreen         { 46u,  168u, 82u,  255u }; // Solid green icon fill
    ColorRgba AccentBlue          { 52u,  118u, 214u, 255u }; // Solid blue icon fill
    ColorRgba AccentAmber         { 196u, 118u, 42u,  255u }; // Solid warm icon fill
    ColorRgba AccentViolet        { 124u, 86u,  196u, 255u }; // Solid violet icon fill
    ColorRgba AccentTeal          { 40u,  138u, 128u, 255u }; // Solid teal icon fill
    ColorRgba AccentSlate         { 48u,  51u,  58u,  255u }; // Solid neutral icon fill
    ColorRgba Green               { 70u,  210u, 103u, 255u };
    ColorRgba ToggleGreen         { 52u,  199u, 89u,  255u };
    ColorRgba GreenWash           { 26u,  47u,  32u,  255u };
    ColorRgba Amber               { 232u, 194u, 122u, 255u };
    ColorRgba AmberWash           { 44u,  37u,  22u,  255u };
    ColorRgba Blue                { 158u, 207u, 255u, 255u };
    ColorRgba BlueWash            { 25u,  38u,  56u,  255u };
    ColorRgba Crimson             { 255u, 107u, 98u,  255u };
    ColorRgba CrimsonWash         { 54u,  26u,  25u,  255u };
    ColorRgba TextPrimary         {};
    ColorRgba TextSecondary       {};
    ColorRgba TextMuted           {};
    ColorRgba TextFaint           {};
    bool      LightSurface        = false;
};

struct OutlinerStatTileSpec
{
    std::string_view Label       {};
    std::string_view Reading     {};
    ControlIcon      Icon        = ControlIcon::CircleCheck;
    IconAccentTone   IconTone    = IconAccentTone::Slate;
    bool             FilterArmed = false;
};

struct OutlinerChipSpec
{
    std::string_view Label        {};
    ColorRgba        SwatchStart  { 0u, 0u, 0u, 0u };
    ColorRgba        SwatchEnd    { 0u, 0u, 0u, 0u };
    bool             ShowSwatch   = false;
    bool             ShowPlusIcon = false;
    bool             Selected     = false;
    bool             Disabled     = false;
};

struct OutlinerHeaderResult
{
    bool FolderClicked = false;
    bool PlusClicked   = false;
};

struct HierarchyRowSpec
{
    int32_t          DepthLevel        = 0;
    std::string_view Title             {};
    std::string_view Subtitle          {};
    std::string_view TagLabel          {};
    TagTone          TagStyle          = TagTone::Neutral;
    ControlIcon      LeadIcon          = ControlIcon::LayersSlabs;
    IconAccentTone   LeadIconTone      = IconAccentTone::Slate;
    bool             CustomLeadPreview = false; // Caller paints live preview into *OutLeadPreviewRect
    float            LeadPreviewSize   = 28.0f; // 28..32px sharp square live preview box
    bool             Expandable        = false;
    bool             Expanded          = false;
    bool             Selected          = false;
    bool             Muted             = false;
    bool             GroupStyle        = false;
    bool             ShowGreenCheck    = false;
    bool             GreenCheckOn      = false;
    bool             PartialIndicator  = false;
    bool             ShowEyeToggle     = false;
    bool             EyeVisible        = true;
    bool             ShowLockToggle    = false;
    bool             LockEnabled       = false;
};

struct HierarchyRowResult
{
    bool        RowClicked      = false;
    bool        DoubleClicked   = false;
    bool        ChevronClicked  = false;
    bool        CheckToggled    = false;
    bool        EyeToggled      = false;
    bool        LockToggled     = false;
    bool        Hovered         = false;
    Rectangle2D LeadPreviewRect {};
};

struct OutlinerToolbarResult
{
    bool MoveUpClicked    = false;
    bool MoveDownClicked  = false;
    bool DuplicateClicked = false;
    bool MaskClicked      = false;
    bool DeleteClicked    = false;
};

struct ThreeMetricFooterCell
{
    std::string_view Label   {};
    std::string_view Reading {};
    TagTone          Tone    = TagTone::Neutral;
};

struct KpiTileSpec
{
    std::string_view Label       {};
    std::string_view Reading     {};
    std::string_view Caption     {};
    bool             Highlighted = false; // Signature #EFF4CF pale lime-cream tile 1
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

struct PopoverOptionSpec
{
    std::string_view Label     {};
    std::string_view Detail    {};
    ColorRgba        SwatchDot { 0u, 0u, 0u, 0u };
};

//------------------------------------------------------------------------------------------------------------------------
//                                                  THEME & COLOUR QUERIES
//------------------------------------------------------------------------------------------------------------------------

[[nodiscard]] inline StudioToneSet QueryStudioToneSet() noexcept
{
    const ControlColorSet& Colors = ControlAppearance::Colors();
    StudioToneSet Tone {};
    Tone.LightSurface = Colors.LightSurface;

    if (!Colors.LightSurface)
    {
        Tone.PaneSurface       = { 20u,  21u,  24u,  255u }; // #141518 pane-direct surface
        Tone.PaneHeaderSurface = { 23u,  24u,  28u,  255u }; // #17181C heading strip
        Tone.SectionHeaderBg   = { 33u,  35u,  40u,  255u }; // #212328 category section header
        Tone.SectionBodyBg     = { 24u,  25u,  29u,  255u }; // #18191D category section body
        Tone.InsetSurface      = { 25u,  26u,  29u,  255u }; // #191A1D KPI dark tile
        Tone.TileSurface       = { 26u,  27u,  31u,  255u }; // #1A1B1F stat tile & chip
        Tone.FooterSurface     = { 16u,  17u,  20u,  255u }; // #101114 3-metric footer
        Tone.ToolbarSurface    = { 20u,  21u,  24u,  255u }; // #141518 command strip
        Tone.GroundRowSurface  = { 17u,  18u,  21u,  255u }; // #111215 layer-0 ground row
        Tone.FieldWell         = { 6u,   6u,   8u,   255u }; // #060608 numeric well
        Tone.UnitWell          = { 22u,  22u,  24u,  255u }; // #161618 split unit cell
        Tone.RecessedWell      = { 13u,  14u,  16u,  255u };
        Tone.SliderTrack       = { 35u,  35u,  38u,  255u }; // #232326
        Tone.SliderFill        = { 76u,  77u,  82u,  255u }; // #4C4D52
        Tone.SliderThumb       = { 235u, 235u, 235u, 255u }; // #EBEBEB
        Tone.RowSurface        = { 23u,  24u,  28u,  255u }; // #17181C sharp row
        Tone.RowHover          = { 29u,  31u,  36u,  255u }; // #1D1F24
        Tone.RowSelected       = { 35u,  37u,  32u,  255u }; // #232520 warm lime-tinted selection
        Tone.RowSelectedStroke = { 239u, 244u, 207u, 108u }; // rgba(239,244,207,0.42)
        Tone.HighlightFill     = { 239u, 244u, 207u, 255u }; // #EFF4CF
        Tone.HighlightInk      = { 23u,  25u,  14u,  255u }; // #17190E
        Tone.HighlightMutedInk = { 23u,  25u,  14u,  204u };
        Tone.Hairline          = { 38u,  40u,  45u,  255u };
        Tone.BorderSubtle      = { 44u,  46u,  52u,  255u };
        Tone.BorderStrong      = { 68u,  71u,  79u,  255u };
        Tone.PrimaryFill       = { 239u, 244u, 207u, 255u }; // #EFF4CF TextureStudio primary CTA
        Tone.PrimaryInk        = { 23u,  25u,  14u,  255u };
        Tone.TextPrimary       = { 242u, 243u, 245u, 255u };
        Tone.TextSecondary     = { 205u, 208u, 214u, 255u };
        Tone.TextMuted         = { 136u, 140u, 149u, 255u };
        Tone.TextFaint         = { 98u,  102u, 111u, 255u };
        return Tone;
    }

    // Crisp light-mode counterpart
    Tone.PaneSurface       = { 243u, 244u, 247u, 255u };
    Tone.PaneHeaderSurface = { 238u, 240u, 244u, 255u };
    Tone.SectionHeaderBg   = { 232u, 235u, 241u, 255u };
    Tone.SectionBodyBg     = { 248u, 249u, 251u, 255u };
    Tone.InsetSurface      = { 236u, 238u, 243u, 255u };
    Tone.TileSurface       = { 238u, 240u, 245u, 255u };
    Tone.FooterSurface     = { 233u, 236u, 241u, 255u };
    Tone.ToolbarSurface    = { 236u, 238u, 243u, 255u };
    Tone.GroundRowSurface  = { 231u, 234u, 240u, 255u };
    Tone.FieldWell         = { 255u, 255u, 255u, 255u };
    Tone.UnitWell          = { 236u, 239u, 244u, 255u };
    Tone.RecessedWell      = { 226u, 229u, 235u, 255u };
    Tone.SliderTrack       = { 216u, 220u, 228u, 255u };
    Tone.SliderFill        = { 148u, 155u, 168u, 255u };
    Tone.SliderThumb       = { 32u,  36u,  44u,  255u };
    Tone.RowSurface        = { 249u, 250u, 252u, 255u };
    Tone.RowHover          = { 236u, 240u, 246u, 255u };
    Tone.RowSelected       = { 226u, 233u, 244u, 255u };
    Tone.RowSelectedStroke = { 52u,  118u, 214u, 140u };
    Tone.HighlightFill     = { 30u,  35u,  44u,  255u };
    Tone.HighlightInk      = { 248u, 249u, 252u, 255u };
    Tone.HighlightMutedInk = { 214u, 218u, 226u, 215u };
    Tone.Hairline          = { 214u, 218u, 226u, 255u };
    Tone.BorderSubtle      = { 204u, 210u, 220u, 255u };
    Tone.BorderStrong      = { 166u, 174u, 188u, 255u };
    Tone.PrimaryFill       = { 26u,  30u,  38u,  255u };
    Tone.PrimaryInk        = { 248u, 249u, 252u, 255u };
    Tone.AccentSlate       = { 104u, 112u, 126u, 255u };
    Tone.Green             = { 28u,  148u, 62u,  255u };
    Tone.ToggleGreen       = { 34u,  166u, 72u,  255u };
    Tone.GreenWash         = { 218u, 245u, 226u, 255u };
    Tone.Amber             = { 186u, 112u, 16u,  255u };
    Tone.AmberWash         = { 252u, 241u, 218u, 255u };
    Tone.Blue              = { 32u,  108u, 220u, 255u };
    Tone.BlueWash          = { 220u, 235u, 252u, 255u };
    Tone.Crimson           = { 206u, 52u,  44u,  255u };
    Tone.CrimsonWash       = { 252u, 224u, 222u, 255u };
    Tone.TextPrimary       = { 24u,  28u,  34u,  255u };
    Tone.TextSecondary     = { 54u,  60u,  70u,  255u };
    Tone.TextMuted         = { 104u, 112u, 124u, 255u };
    Tone.TextFaint         = { 138u, 145u, 156u, 255u };
    return Tone;
}

[[nodiscard]] inline ColorRgba ResolveAccentFill(IconAccentTone Accent, const StudioToneSet& Tone) noexcept
{
    switch (Accent)
    {
        case IconAccentTone::Green:  return Tone.AccentGreen;
        case IconAccentTone::Blue:   return Tone.AccentBlue;
        case IconAccentTone::Amber:  return Tone.AccentAmber;
        case IconAccentTone::Violet: return Tone.AccentViolet;
        case IconAccentTone::Teal:   return Tone.AccentTeal;
        default:                     return Tone.AccentSlate;
    }
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

//------------------------------------------------------------------------------------------------------------------------
//                                                 TEXT & VECTOR MARKS
//------------------------------------------------------------------------------------------------------------------------

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

inline void ConstructCheckMark(PixelSpace& Surface, Point2D Center, float Scale, ColorRgba Ink, float Thickness = 1.65f) noexcept
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

// White icon rendered inside a solid coloured background tile (replaces yellow-on-yellow)
inline void ConstructWhiteIconTile(
    PixelSpace&          Surface,
    Rectangle2D          TileRect,
    ControlIcon          Icon,
    IconAccentTone       Accent,
    float                CornerRadius,
    const StudioToneSet& Tone,
    bool                 Muted = false) noexcept
{
    const ColorRgba Fill = Muted ? ResolveAccentFill(Accent, Tone).ScaledAlpha(0.45f) : ResolveAccentFill(Accent, Tone);
    Surface.FillRectangle(TileRect, Fill, CornerRadius);
    const float IconSize = std::min(TileRect.Width, TileRect.Height) * 0.54f;
    VectorIconRepository::ConstructControl(Surface, Icon, {
        TileRect.Center().X - IconSize * 0.5f,
        TileRect.Center().Y - IconSize * 0.5f,
        IconSize,
        Muted ? Tone.WhiteInk.ScaledAlpha(0.55f) : Tone.WhiteInk,
        1.95f,
    });
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
//                                              PANE-DIRECT SURFACE CONTAINER
//------------------------------------------------------------------------------------------------------------------------

// Applies surface colour directly onto the Outliner/Inspector pane edge-to-edge (zero nested rounded outer border)
inline Rectangle2D ConstructDockedSurfaceShell(
    PixelSpace&          Surface,
    Rectangle2D          PaneBounds,
    bool                 /*OutlinerVariant*/,
    const StudioToneSet& Tone) noexcept
{
    Surface.FillRectangle(PaneBounds, Tone.PaneSurface, 0.0f);
    return PaneBounds;
}

inline bool ConstructRoundIconButton(
    PixelSpace&             Surface,
    Rectangle2D             ButtonRect,
    ControlIcon             Icon,
    const PaneInputReading& Input,
    const StudioToneSet&    Tone,
    bool                    Disabled = false,
    bool                    Armed    = false) noexcept
{
    const Point2D Cursor  = QueryInputCursor(Input);
    const bool    Hovered = !Disabled && ButtonRect.Contains(Cursor);
    const float   Radius  = ButtonRect.Height * 0.5f;

    const ColorRgba Fill = Armed
        ? Tone.HighlightFill
        : (Hovered ? Tone.RowHover : Tone.TileSurface);
    const ColorRgba Stroke = Armed
        ? Tone.HighlightFill
        : (Hovered ? Tone.BorderStrong : Tone.BorderSubtle);
    const ColorRgba Ink = Disabled
        ? Tone.TextFaint
        : (Armed ? Tone.HighlightInk : (Hovered ? Tone.TextPrimary : Tone.TextSecondary));

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

// TextureStudio OLD heading bar: Title + mono count tag + optional FolderPlus button + circular '+' button
inline OutlinerHeaderResult ConstructOutlinerHeaderStrip(
    PixelSpace&             Surface,
    Rectangle2D             StripRect,
    std::string_view        Title,
    std::string_view        CountTag,
    bool                    ShowFolderButton,
    ControlIcon             PrimaryButtonIcon,
    bool                    PrimaryButtonArmed,
    const PaneInputReading& Input,
    const StudioToneSet&    Tone,
    Rectangle2D*            OutPrimaryButtonRect = nullptr) noexcept
{
    OutlinerHeaderResult Result {};
    Surface.FillRectangle(StripRect, Tone.PaneHeaderSurface, 0.0f);
    Surface.StrokeLine({ StripRect.X, StripRect.Bottom() }, { StripRect.Right(), StripRect.Bottom() }, Tone.Hairline, 1.0f);

    const float LeftX = StripRect.X + 12.0f;
    WorkspaceAppearance::ConstructWeightedText(
        Surface, FontWeightClass::Light, { LeftX, StripRect.Center().Y - 9.5f }, 17.5f, Tone.TextPrimary, Title);

    if (!CountTag.empty())
    {
        const float TitleW = Surface.MeasureTextWidth(17.8f, Title.data(), Title.data() + Title.size());
        const float TagW   = Surface.MeasureTextWidth(9.2f, CountTag.data(), CountTag.data() + CountTag.size()) + 14.0f;
        const Rectangle2D TagRect { LeftX + TitleW + 8.0f, StripRect.Center().Y - 9.0f, TagW, 18.0f };
        Surface.FillRectangle(TagRect, Tone.TileSurface, 9.0f);
        Surface.StrokeRectangle(TagRect, Tone.BorderSubtle, 9.0f, 1.0f);
        ConstructCenteredWeightedText(Surface, FontWeightClass::Medium, TagRect, 9.2f, Tone.TextMuted, CountTag);
    }

    float RightX = StripRect.Right() - 12.0f;
    const Rectangle2D PlusRect { RightX - 28.0f, StripRect.Center().Y - 14.0f, 28.0f, 28.0f };
    if (OutPrimaryButtonRect != nullptr)
    {
        *OutPrimaryButtonRect = PlusRect;
    }
    Result.PlusClicked = ConstructRoundIconButton(Surface, PlusRect, PrimaryButtonIcon, Input, Tone, false, PrimaryButtonArmed);
    RightX = PlusRect.X - 6.0f;

    if (ShowFolderButton)
    {
        const Rectangle2D FolderRect { RightX - 28.0f, StripRect.Center().Y - 14.0f, 28.0f, 28.0f };
        Result.FolderClicked = ConstructRoundIconButton(Surface, FolderRect, ControlIcon::FolderPlus, Input, Tone, false, false);
    }
    return Result;
}

inline int32_t ConstructStatTilePair(
    PixelSpace&                 Surface,
    Rectangle2D                 Area,
    const OutlinerStatTileSpec& LeftSpec,
    const OutlinerStatTileSpec& RightSpec,
    const PaneInputReading&     Input,
    const StudioToneSet&        Tone) noexcept
{
    constexpr float Gap   = 7.0f;
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
            ? Tone.RowSelected
            : (Hovered ? Tone.RowHover : Tone.TileSurface);
        const ColorRgba Stroke = Spec.FilterArmed
            ? Tone.RowSelectedStroke
            : (Hovered ? Tone.BorderStrong : Tone.BorderSubtle);

        Surface.FillRectangle(TileRect, Fill, 12.0f);
        Surface.StrokeRectangle(TileRect, Stroke, 12.0f, 1.0f);

        // White icon inside solid coloured circular background
        const float   DiscRad = 11.5f;
        const Point2D DiscCenter { TileRect.X + 10.0f + DiscRad, TileRect.Center().Y };
        Surface.FillCircle(DiscCenter, DiscRad, ResolveAccentFill(Spec.IconTone, Tone));

        if (Spec.Icon == ControlIcon::CircleCheck)
        {
            ConstructCheckMark(Surface, DiscCenter, 1.1f, Tone.WhiteInk, 1.85f);
        }
        else
        {
            VectorIconRepository::ConstructControl(Surface, Spec.Icon, {
                DiscCenter.X - 5.5f,
                DiscCenter.Y - 5.5f,
                11.0f,
                Tone.WhiteInk,
                1.95f,
            });
        }

        const float TextLeft = DiscCenter.X + DiscRad + 8.0f;
        WorkspaceAppearance::ConstructWeightedText(
            Surface, FontWeightClass::Medium, { TextLeft, TileRect.Y + 8.0f }, 8.4f, Tone.TextMuted, Spec.Label);
        WorkspaceAppearance::ConstructWeightedText(
            Surface, FontWeightClass::Light, { TextLeft, TileRect.Y + 21.0f }, 17.5f, Tone.TextPrimary, Spec.Reading);

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
    constexpr float Gap     = 7.0f;
    const float     FilterW = std::clamp(Surface.MeasureTextWidth(10.0f, FilterLabel.data(), FilterLabel.data() + FilterLabel.size()) + 30.0f, 64.0f, 96.0f);
    const Rectangle2D SearchRect { RowRect.X, RowRect.Y, std::max(60.0f, RowRect.Width - FilterW - Gap), RowRect.Height };
    const Rectangle2D FilterRect { SearchRect.Right() + Gap, RowRect.Y, FilterW, RowRect.Height };
    const float       Radius     = RowRect.Height * 0.5f;

    Surface.FillRectangle(SearchRect, Tone.FieldWell, Radius);
    Surface.StrokeRectangle(SearchRect, SearchBuffer.Active ? Tone.HighlightFill : Tone.BorderSubtle, Radius, 1.0f);
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

    const bool FilterHover = FilterRect.Contains(Cursor);
    const ColorRgba FilterFill = FilterActive
        ? Tone.HighlightFill
        : (FilterHover ? Tone.RowHover : Tone.TileSurface);
    const ColorRgba FilterStroke = FilterActive
        ? Tone.HighlightFill
        : (FilterHover ? Tone.BorderStrong : Tone.BorderSubtle);
    const ColorRgba FilterInk = FilterActive ? Tone.HighlightInk : Tone.TextSecondary;

    Surface.FillRectangle(FilterRect, FilterFill, Radius);
    Surface.StrokeRectangle(FilterRect, FilterStroke, Radius, 1.0f);
    WorkspaceAppearance::ConstructWeightedText(
        Surface, FontWeightClass::Medium, { FilterRect.X + 12.0f, FilterRect.Center().Y - 5.2f }, 10.0f, FilterInk, FilterLabel);
    ConstructChevronMark(Surface, { FilterRect.Right() - 12.0f, FilterRect.Center().Y }, true, 3.0f, FilterInk);

    return FilterHover && QueryPrimaryPressed(Input);
}

inline int32_t ConstructQuickChipStrip(
    PixelSpace&                       Surface,
    float                             LeftX,
    float                             TopY,
    float                             MaxWidth,
    std::span<const OutlinerChipSpec> Chips,
    const PaneInputReading&           Input,
    const StudioToneSet&              Tone,
    float&                            OutConsumedHeight) noexcept
{
    const Point2D Cursor = QueryInputCursor(Input);
    constexpr float ChipH = 24.0f;
    constexpr float GapX  = 5.0f;
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
            ? Tone.HighlightFill
            : (Hovered ? Tone.RowHover : Tone.TileSurface);
        const ColorRgba Stroke = Chip.Selected
            ? Tone.HighlightFill
            : (Hovered ? Tone.BorderStrong : Tone.BorderSubtle);
        const ColorRgba Ink = Chip.Disabled
            ? Tone.TextFaint
            : (Chip.Selected ? Tone.HighlightInk : (Hovered ? Tone.TextPrimary : Tone.TextSecondary));

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

// Sharp-cornered outliner entry (0px radius) preserving metadata, live preview slot, green tick, eye & lock buttons
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

    // Sharp 0px corners for all outliner entries (matches TextureStudio OLD .tp-layer-row)
    const ColorRgba BgFill = Spec.Selected
        ? Tone.RowSelected
        : (Outcome.Hovered ? Tone.RowHover : (Spec.GroupStyle ? Tone.PaneHeaderSurface : Tone.RowSurface));
    const ColorRgba BgStroke = Spec.Selected ? Tone.RowSelectedStroke : Tone.Hairline;
    Surface.FillRectangle(RowRect, BgFill, 0.0f);
    Surface.StrokeRectangle(RowRect, BgStroke, 0.0f, 1.0f);

    if (Spec.Selected)
    {
        // Sharp 3px left selection spine
        Surface.FillRectangle({ RowRect.X, RowRect.Y, 3.0f, RowRect.Height }, Tone.HighlightFill, 0.0f);
    }

    const float IndentX = RowRect.X + 7.0f + static_cast<float>(std::max(0, Spec.DepthLevel)) * 12.0f;
    float LeftX = IndentX;

    // Expand / collapse chevron hit target
    const Rectangle2D ChevronRect { LeftX - 2.0f, RowRect.Y + 3.0f, 15.0f, RowRect.Height - 6.0f };
    if (Spec.Expandable)
    {
        ConstructChevronMark(Surface, { LeftX + 5.0f, RowRect.Center().Y }, Spec.Expanded, 3.0f, Tone.TextMuted);
    }
    LeftX += 14.0f;

    // Sharp square preview box (live procedural preview or white icon on solid coloured fill)
    const float PreviewSz = std::clamp(Spec.LeadPreviewSize, 20.0f, RowRect.Height - 6.0f);
    const Rectangle2D LeadTile { LeftX, RowRect.Center().Y - PreviewSz * 0.5f, PreviewSz, PreviewSz };
    Outcome.LeadPreviewRect = LeadTile;

    if (Spec.CustomLeadPreview)
    {
        Surface.FillRectangle(LeadTile, Tone.FieldWell, 0.0f);
        Surface.StrokeRectangle(LeadTile, Tone.BorderSubtle, 0.0f, 1.0f);
    }
    else
    {
        ConstructWhiteIconTile(Surface, LeadTile, Spec.LeadIcon, Spec.LeadIconTone, 0.0f, Tone, Spec.Muted);
    }
    LeftX = LeadTile.Right() + 8.0f;

    // Trailing controls right-to-left: Green check -> Lock -> Eye -> Tag chip
    float RightX = RowRect.Right() - 7.0f;
    Rectangle2D CheckRect {};
    Rectangle2D LockRect {};
    Rectangle2D EyeRect {};

    if (Spec.ShowGreenCheck)
    {
        CheckRect = { RightX - 18.0f, RowRect.Center().Y - 9.0f, 18.0f, 18.0f };
        const bool CheckHover = CheckRect.Contains(Cursor);
        const Point2D Centre = CheckRect.Center();
        if (Spec.GreenCheckOn)
        {
            Surface.FillCircle(Centre, 7.5f, Tone.ToggleGreen);
            ConstructCheckMark(Surface, Centre, 0.85f, Tone.WhiteInk, 1.7f);
        }
        else
        {
            Surface.FillCircle(Centre, 7.5f, CheckHover ? Tone.RowHover : Tone.FieldWell);
            Surface.StrokeCircle(Centre, 7.5f, CheckHover ? Tone.TextMuted : Tone.BorderStrong, 1.25f);
            if (Spec.PartialIndicator)
            {
                Surface.FillCircle(Centre, 2.6f, Tone.Amber);
            }
        }
        RightX = CheckRect.X - 4.0f;
    }

    if (Spec.ShowLockToggle)
    {
        LockRect = { RightX - 18.0f, RowRect.Center().Y - 9.0f, 18.0f, 18.0f };
        const bool LockHover = LockRect.Contains(Cursor);
        if (Spec.LockEnabled)
        {
            Surface.FillRectangle(LockRect, Tone.AccentAmber, 0.0f);
        }
        else if (LockHover)
        {
            Surface.FillRectangle(LockRect, Tone.AccentSlate, 0.0f);
        }
        VectorIconRepository::ConstructControl(Surface, ControlIcon::LockClosed, {
            LockRect.Center().X - 5.2f,
            LockRect.Center().Y - 5.2f,
            10.4f,
            Spec.LockEnabled ? Tone.WhiteInk : (LockHover ? Tone.WhiteInk : Tone.TextFaint),
            1.85f,
        });
        RightX = LockRect.X - 3.0f;
    }

    if (Spec.ShowEyeToggle)
    {
        EyeRect = { RightX - 20.0f, RowRect.Center().Y - 10.0f, 20.0f, 20.0f };
        const bool EyeHover = EyeRect.Contains(Cursor);
        if (EyeHover)
        {
            Surface.FillRectangle(EyeRect, Tone.AccentSlate, 0.0f);
        }
        const ControlIcon EyeIcon = Spec.EyeVisible ? ControlIcon::EyeVisible : ControlIcon::EyeHidden;
        const ColorRgba   EyeInk  = Spec.EyeVisible
            ? (EyeHover ? Tone.WhiteInk : Tone.TextSecondary)
            : Tone.TextFaint;
        VectorIconRepository::ConstructControl(Surface, EyeIcon, {
            EyeRect.Center().X - 5.8f,
            EyeRect.Center().Y - 5.8f,
            11.6f,
            EyeInk,
            1.85f,
        });
        RightX = EyeRect.X - 4.0f;
    }

    if (!Spec.TagLabel.empty())
    {
        const float TagTextW = Surface.MeasureTextWidth(8.0f, Spec.TagLabel.data(), Spec.TagLabel.data() + Spec.TagLabel.size());
        const float TagW     = TagTextW + 10.0f;
        const Rectangle2D TagRect { RightX - TagW, RowRect.Center().Y - 8.0f, TagW, 16.0f };
        Surface.FillRectangle(TagRect, ResolveTagBackground(Spec.TagStyle, Tone), 0.0f);
        ConstructCenteredWeightedText(
            Surface, FontWeightClass::Medium, TagRect, 8.0f, ResolveTagInk(Spec.TagStyle, Tone), Spec.TagLabel);
        RightX = TagRect.X - 5.0f;
    }

    // Clipped Title + Metadata Subtitle
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
        const float TitleY = RowRect.Center().Y - 12.5f;
        WorkspaceAppearance::ConstructWeightedText(
            Surface,
            Spec.Selected ? FontWeightClass::Medium : FontWeightClass::Regular,
            { LeftX, TitleY },
            11.0f,
            TitleInk,
            Spec.Title);
        WorkspaceAppearance::ConstructWeightedText(
            Surface, FontWeightClass::Regular, { LeftX, TitleY + 14.5f }, 9.0f, SubInk, Spec.Subtitle);
    }
    Surface.PopClip();

    if (QueryPrimaryPressed(Input) && Outcome.Hovered)
    {
        if (Spec.ShowGreenCheck && CheckRect.Contains(Cursor))
        {
            Outcome.CheckToggled = true;
        }
        else if (Spec.ShowLockToggle && LockRect.Contains(Cursor))
        {
            Outcome.LockToggled = true;
        }
        else if (Spec.ShowEyeToggle && EyeRect.Contains(Cursor))
        {
            Outcome.EyeToggled = true;
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

// TextureStudio OLD bottom command toolbar (Up / Down  ---  Duplicate / Mask / Delete)
inline OutlinerToolbarResult ConstructOutlinerCommandToolbar(
    PixelSpace&             Surface,
    Rectangle2D             BarRect,
    bool                    AllowMoveUp,
    bool                    AllowMoveDown,
    bool                    AllowDuplicate,
    bool                    ShowMaskButton,
    bool                    AllowDelete,
    const PaneInputReading& Input,
    const StudioToneSet&    Tone) noexcept
{
    OutlinerToolbarResult Result {};
    Surface.FillRectangle(BarRect, Tone.ToolbarSurface, 0.0f);
    Surface.StrokeLine({ BarRect.X, BarRect.Y }, { BarRect.Right(), BarRect.Y }, Tone.Hairline, 1.0f);

    const Point2D Cursor = QueryInputCursor(Input);
    auto ConstructBtn = [&](Rectangle2D Rect, ControlIcon Icon, bool Enabled, bool Danger) -> bool
    {
        const bool Hovered = Enabled && Rect.Contains(Cursor);
        const ColorRgba Fill = Hovered
            ? (Danger ? Tone.CrimsonWash : Tone.AccentSlate)
            : Tone.TileSurface;
        const ColorRgba Stroke = Hovered
            ? (Danger ? Tone.Crimson : Tone.BorderStrong)
            : Tone.BorderSubtle;
        const ColorRgba Ink = !Enabled
            ? Tone.TextFaint
            : (Hovered && Danger ? Tone.Crimson : Tone.WhiteInk);

        Surface.FillRectangle(Rect, Fill, 0.0f);
        Surface.StrokeRectangle(Rect, Stroke, 0.0f, 1.0f);
        VectorIconRepository::ConstructControl(Surface, Icon, {
            Rect.Center().X - 6.0f,
            Rect.Center().Y - 6.0f,
            12.0f,
            Ink,
            1.85f,
        });
        return Hovered && QueryPrimaryPressed(Input);
    };

    const float BtnY = BarRect.Center().Y - 12.0f;
    const Rectangle2D UpRect   { BarRect.X + 10.0f, BtnY, 25.0f, 24.0f };
    const Rectangle2D DownRect { UpRect.Right() + 4.0f, BtnY, 25.0f, 24.0f };
    Result.MoveUpClicked   = ConstructBtn(UpRect, ControlIcon::ChevronUp, AllowMoveUp, false);
    Result.MoveDownClicked = ConstructBtn(DownRect, ControlIcon::ChevronDown, AllowMoveDown, false);

    float RightX = BarRect.Right() - 10.0f;
    const Rectangle2D DelRect { RightX - 25.0f, BtnY, 25.0f, 24.0f };
    Result.DeleteClicked = ConstructBtn(DelRect, ControlIcon::TrashDelete, AllowDelete, true);
    RightX = DelRect.X - 4.0f;

    if (ShowMaskButton)
    {
        const Rectangle2D MaskRect { RightX - 25.0f, BtnY, 25.0f, 24.0f };
        Result.MaskClicked = ConstructBtn(MaskRect, ControlIcon::MaskCutout, AllowDuplicate, false);
        RightX = MaskRect.X - 4.0f;
    }

    const Rectangle2D DupRect { RightX - 25.0f, BtnY, 25.0f, 24.0f };
    Result.DuplicateClicked = ConstructBtn(DupRect, ControlIcon::CopyDuplicate, AllowDuplicate, false);
    return Result;
}

// TextureStudio OLD Canvas/Layer-0 ground bar above the 3-metric footer
inline void ConstructOutlinerGroundRow(
    PixelSpace&          Surface,
    Rectangle2D          RowRect,
    ControlIcon          Icon,
    IconAccentTone       IconTone,
    std::string_view     Title,
    std::string_view     Subtitle,
    std::string_view     RightTag,
    const StudioToneSet& Tone) noexcept
{
    Surface.FillRectangle(RowRect, Tone.GroundRowSurface, 0.0f);
    Surface.StrokeLine({ RowRect.X, RowRect.Y }, { RowRect.Right(), RowRect.Y }, Tone.Hairline, 1.0f);

    const Rectangle2D IconRect { RowRect.X + 10.0f, RowRect.Center().Y - 11.0f, 22.0f, 22.0f };
    ConstructWhiteIconTile(Surface, IconRect, Icon, IconTone, 0.0f, Tone, false);

    float RightX = RowRect.Right() - 10.0f;
    if (!RightTag.empty())
    {
        const float TagW = Surface.MeasureTextWidth(8.6f, RightTag.data(), RightTag.data() + RightTag.size()) + 12.0f;
        const Rectangle2D TagRect { RightX - TagW, RowRect.Center().Y - 9.0f, TagW, 18.0f };
        Surface.FillRectangle(TagRect, Tone.TileSurface, 0.0f);
        Surface.StrokeRectangle(TagRect, Tone.BorderSubtle, 0.0f, 1.0f);
        ConstructCenteredWeightedText(Surface, FontWeightClass::Medium, TagRect, 8.6f, Tone.TextSecondary, RightTag);
        RightX = TagRect.X - 8.0f;
    }

    const float TextX = IconRect.Right() + 8.0f;
    Surface.PushClip({ TextX, RowRect.Y, std::max(20.0f, RightX - TextX), RowRect.Height });
    WorkspaceAppearance::ConstructWeightedText(
        Surface, FontWeightClass::Regular, { TextX, RowRect.Y + 6.0f }, 10.2f, Tone.TextSecondary, Title);
    WorkspaceAppearance::ConstructWeightedText(
        Surface, FontWeightClass::Regular, { TextX, RowRect.Y + 20.0f }, 8.6f, Tone.TextFaint, Subtitle);
    Surface.PopClip();
}

// TextureStudio OLD 3-Metric Outliner Footer (LAYERS 2 | VISIBLE 2 | MODS 4)
inline void ConstructThreeMetricFooter(
    PixelSpace&                                  Surface,
    Rectangle2D                                  FooterRect,
    const std::array<ThreeMetricFooterCell, 3u>& Cells,
    const StudioToneSet&                         Tone) noexcept
{
    Surface.FillRectangle(FooterRect, Tone.FooterSurface, 0.0f);
    Surface.StrokeLine({ FooterRect.X, FooterRect.Y }, { FooterRect.Right(), FooterRect.Y }, Tone.Hairline, 1.0f);

    const float ColW = FooterRect.Width / 3.0f;
    for (std::size_t Idx = 0u; Idx < 3u; ++Idx)
    {
        const ThreeMetricFooterCell& Cell = Cells[Idx];
        const float LblW  = Surface.MeasureTextWidth(8.6f, Cell.Label.data(), Cell.Label.data() + Cell.Label.size());
        const float ReadW = Surface.MeasureTextWidth(9.8f, Cell.Reading.data(), Cell.Reading.data() + Cell.Reading.size());
        const float PairW = LblW + 5.0f + ReadW;

        float StartX = FooterRect.X + 12.0f;
        if (Idx == 1u)
        {
            StartX = FooterRect.X + ColW + (ColW - PairW) * 0.5f;
        }
        else if (Idx == 2u)
        {
            StartX = FooterRect.Right() - 12.0f - PairW;
        }

        const float MidY = FooterRect.Center().Y;
        WorkspaceAppearance::ConstructWeightedText(
            Surface, FontWeightClass::Medium, { StartX, MidY - 4.8f }, 8.6f, Tone.TextFaint, Cell.Label);

        const ColorRgba Ink = (Cell.Tone == TagTone::Green)
            ? Tone.Green
            : (Cell.Tone == TagTone::Amber ? Tone.Amber : Tone.TextPrimary);
        WorkspaceAppearance::ConstructWeightedText(
            Surface, FontWeightClass::Medium, { StartX + LblW + 5.0f, MidY - 5.2f }, 9.8f, Ink, Cell.Reading);
    }
}

//------------------------------------------------------------------------------------------------------------------------
//                                        TEXTURESTUDIO OLD INSPECTOR PRIMITIVES
//------------------------------------------------------------------------------------------------------------------------

// Old TextureStudio Material Inspector Top Heading Strip (Title left, Mono Eyebrow + optional compact buttons right)
inline InspectorHeadingResult ConstructStudioInspectorHeader(
    PixelSpace&             Surface,
    Rectangle2D             StripRect,
    std::string_view        Title,
    std::string_view        CategoryEyebrow,
    bool                    ShowUndoRedo,
    bool                    UndoAvailable,
    bool                    RedoAvailable,
    bool                    ShowReset,
    const PaneInputReading& Input,
    const StudioToneSet&    Tone) noexcept
{
    InspectorHeadingResult Result {};
    Surface.FillRectangle(StripRect, Tone.PaneHeaderSurface, 0.0f);
    Surface.StrokeLine({ StripRect.X, StripRect.Bottom() }, { StripRect.Right(), StripRect.Bottom() }, Tone.Hairline, 1.0f);

    WorkspaceAppearance::ConstructWeightedText(
        Surface, FontWeightClass::Medium, { StripRect.X + 14.0f, StripRect.Center().Y - 6.8f }, 12.8f, Tone.TextPrimary, Title);

    float RightX = StripRect.Right() - 12.0f;
    if (ShowReset)
    {
        const Rectangle2D Btn { RightX - 24.0f, StripRect.Center().Y - 12.0f, 24.0f, 24.0f };
        Result.ResetClicked = ConstructRoundIconButton(Surface, Btn, ControlIcon::RefreshCycle, Input, Tone);
        RightX = Btn.X - 5.0f;
    }
    if (ShowUndoRedo)
    {
        const Rectangle2D RedoBtn { RightX - 24.0f, StripRect.Center().Y - 12.0f, 24.0f, 24.0f };
        Result.RedoClicked = ConstructRoundIconButton(Surface, RedoBtn, ControlIcon::RedoArrow, Input, Tone, !RedoAvailable);
        RightX = RedoBtn.X - 4.0f;

        const Rectangle2D UndoBtn { RightX - 24.0f, StripRect.Center().Y - 12.0f, 24.0f, 24.0f };
        Result.UndoClicked = ConstructRoundIconButton(Surface, UndoBtn, ControlIcon::UndoArrow, Input, Tone, !UndoAvailable);
        RightX = UndoBtn.X - 8.0f;
    }

    if (!CategoryEyebrow.empty())
    {
        WorkspaceAppearance::ConstructRightAlignedText(
            Surface, FontWeightClass::Medium, { RightX, StripRect.Center().Y - 4.8f }, 8.6f, Tone.TextFaint, CategoryEyebrow);
    }
    return Result;
}

// Old TextureStudio Object Heading (.object-heading: large 20px light title + mono uppercase subtitle + optional selector)
inline bool ConstructStudioObjectHeading(
    PixelSpace&             Surface,
    Rectangle2D             BlockRect,
    std::string_view        ObjectTitle,
    std::string_view        MonoSubtitle,
    std::string_view        SelectorLabel,
    bool                    SelectorOpen,
    const PaneInputReading& Input,
    const StudioToneSet&    Tone,
    Rectangle2D*            OutSelectorBounds = nullptr) noexcept
{
    bool SelectorClicked = false;
    float TitleRightLimit = BlockRect.Right();

    if (!SelectorLabel.empty())
    {
        const float SelW = std::clamp(Surface.MeasureTextWidth(9.6f, SelectorLabel.data(), SelectorLabel.data() + SelectorLabel.size()) + 26.0f, 64.0f, 108.0f);
        const Rectangle2D SelRect { BlockRect.Right() - SelW, BlockRect.Y + 4.0f, SelW, 25.0f };
        if (OutSelectorBounds != nullptr)
        {
            *OutSelectorBounds = SelRect;
        }
        const Point2D Cursor  = QueryInputCursor(Input);
        const bool    Hovered = SelRect.Contains(Cursor);
        Surface.FillRectangle(SelRect, SelectorOpen ? Tone.RowSelected : (Hovered ? Tone.RowHover : Tone.TileSurface), 12.5f);
        Surface.StrokeRectangle(SelRect, SelectorOpen ? Tone.HighlightFill : (Hovered ? Tone.BorderStrong : Tone.BorderSubtle), 12.5f, 1.0f);
        WorkspaceAppearance::ConstructWeightedText(
            Surface, FontWeightClass::Medium, { SelRect.X + 10.0f, SelRect.Center().Y - 5.0f }, 9.5f, Tone.TextSecondary, SelectorLabel);
        ConstructChevronMark(Surface, { SelRect.Right() - 10.0f, SelRect.Center().Y }, !SelectorOpen, 2.8f, Tone.TextMuted);
        SelectorClicked = Hovered && QueryPrimaryPressed(Input);
        TitleRightLimit = SelRect.X - 8.0f;
    }

    const float TitleSize = (ObjectTitle.size() > 22u) ? 16.5f : 19.5f;
    Surface.PushClip({ BlockRect.X, BlockRect.Y, std::max(40.0f, TitleRightLimit - BlockRect.X), BlockRect.Height - 18.0f });
    WorkspaceAppearance::ConstructWeightedText(
        Surface, FontWeightClass::Light, { BlockRect.X, BlockRect.Y + 2.0f }, TitleSize, Tone.TextPrimary, ObjectTitle);
    Surface.PopClip();

    Surface.PushClip({ BlockRect.X, BlockRect.Y + 26.0f, BlockRect.Width, 16.0f });
    WorkspaceAppearance::ConstructWeightedText(
        Surface, FontWeightClass::Medium, { BlockRect.X, BlockRect.Y + 28.0f }, 8.6f, Tone.TextMuted, MonoSubtitle);
    Surface.PopClip();

    Surface.StrokeLine({ BlockRect.X, BlockRect.Bottom() }, { BlockRect.Right(), BlockRect.Bottom() }, Tone.Hairline, 1.0f);
    return SelectorClicked;
}

// Old TextureStudio 3-Up KPI Info Tiles (.kpi-trio: Tile 0 lime-cream #EFF4CF highlight + Tiles 1 & 2 dark rounded)
inline void ConstructStudioKpiTrio(
    PixelSpace&                        Surface,
    Rectangle2D                        Area,
    const std::array<KpiTileSpec, 3u>& Tiles,
    const StudioToneSet&               Tone) noexcept
{
    constexpr float Gap   = 7.0f;
    const float     TileW = (Area.Width - Gap * 2.0f) / 3.0f;

    for (std::size_t Idx = 0u; Idx < 3u; ++Idx)
    {
        const KpiTileSpec& Spec = Tiles[Idx];
        const Rectangle2D  TileRect { Area.X + static_cast<float>(Idx) * (TileW + Gap), Area.Y, TileW, Area.Height };

        const ColorRgba Fill   = Spec.Highlighted ? Tone.HighlightFill : Tone.InsetSurface;
        const ColorRgba Stroke = Spec.Highlighted ? Tone.HighlightFill : Tone.BorderSubtle;
        Surface.FillRectangle(TileRect, Fill, 15.0f);
        if (!Spec.Highlighted)
        {
            Surface.StrokeRectangle(TileRect, Stroke, 15.0f, 1.0f);
        }

        const ColorRgba LabelInk   = Spec.Highlighted ? Tone.HighlightMutedInk : Tone.TextMuted;
        const ColorRgba ReadingInk = Spec.Highlighted ? Tone.HighlightInk      : Tone.TextPrimary;
        const ColorRgba SubInk     = Spec.Highlighted ? Tone.HighlightMutedInk : Tone.TextFaint;

        const float PadX = 10.0f;
        Surface.PushClip(TileRect.InsetXY(PadX, 4.0f));
        WorkspaceAppearance::ConstructWeightedText(
            Surface, FontWeightClass::Regular, { TileRect.X + PadX, TileRect.Y + 9.0f }, 9.0f, LabelInk, Spec.Label);
        WorkspaceAppearance::ConstructWeightedText(
            Surface, FontWeightClass::Regular, { TileRect.X + PadX, TileRect.Y + 23.5f }, 16.5f, ReadingInk, Spec.Reading);
        WorkspaceAppearance::ConstructWeightedText(
            Surface, FontWeightClass::Regular, { TileRect.X + PadX, TileRect.Y + 46.5f }, 8.6f, SubInk, Spec.Caption);
        Surface.PopClip();
    }
}

// Old TextureStudio Categorized Section Group (distinct Header strip with white-on-color icon + right tag, and distinct Body fill)
inline bool ConstructCategoryGroupBox(
    PixelSpace&             Surface,
    Rectangle2D             OuterRect,
    float                   HeaderHeight,
    ControlIcon             CategoryIcon,
    IconAccentTone          IconTone,
    std::string_view        Title,
    std::string_view        CategoryTag,
    bool                    Expanded,
    const PaneInputReading& Input,
    const StudioToneSet&    Tone) noexcept
{
    constexpr float Radius = 16.0f;
    const Rectangle2D HeaderRect { OuterRect.X, OuterRect.Y, OuterRect.Width, HeaderHeight };

    if (Expanded && OuterRect.Height > HeaderHeight + 2.0f)
    {
        Surface.FillRectangle(OuterRect, Tone.SectionBodyBg, Radius);
        Surface.PushClip(HeaderRect);
        Surface.FillRectangle({ HeaderRect.X, HeaderRect.Y, HeaderRect.Width, HeaderRect.Height + Radius }, Tone.SectionHeaderBg, Radius);
        Surface.PopClip();
        Surface.StrokeLine({ OuterRect.X + 1.0f, HeaderRect.Bottom() }, { OuterRect.Right() - 1.0f, HeaderRect.Bottom() }, Tone.Hairline, 1.0f);
        Surface.StrokeRectangle(OuterRect, Tone.BorderSubtle, Radius, 1.0f);
    }
    else
    {
        Surface.FillRectangle(HeaderRect, Tone.SectionHeaderBg, Radius);
        Surface.StrokeRectangle(HeaderRect, Tone.BorderSubtle, Radius, 1.0f);
    }

    // White icon inside coloured category square
    const Rectangle2D IconRect { HeaderRect.X + 11.0f, HeaderRect.Center().Y - 10.0f, 20.0f, 20.0f };
    ConstructWhiteIconTile(Surface, IconRect, CategoryIcon, IconTone, 5.5f, Tone, false);

    WorkspaceAppearance::ConstructWeightedText(
        Surface, FontWeightClass::Light, { IconRect.Right() + 8.0f, HeaderRect.Center().Y - 6.8f }, 13.0f, Tone.TextPrimary, Title);

    float RightX = HeaderRect.Right() - 12.0f;
    ConstructChevronMark(Surface, { RightX - 3.0f, HeaderRect.Center().Y }, Expanded, 3.0f, Tone.TextMuted);
    RightX -= 14.0f;

    if (!CategoryTag.empty())
    {
        WorkspaceAppearance::ConstructRightAlignedText(
            Surface, FontWeightClass::Medium, { RightX, HeaderRect.Center().Y - 4.5f }, 8.4f, Tone.TextFaint, CategoryTag);
    }

    const Point2D Cursor = QueryInputCursor(Input);
    return HeaderRect.Contains(Cursor) && QueryPrimaryPressed(Input);
}

inline int32_t ConstructSegmentedCapsuleBar(
    PixelSpace&                       Surface,
    Rectangle2D                       BarRect,
    std::span<const std::string_view> Labels,
    int32_t                           SelectedIndex,
    const PaneInputReading&           Input,
    const StudioToneSet&              Tone) noexcept
{
    if (Labels.empty())
    {
        return -1;
    }

    const float Radius = BarRect.Height * 0.5f;
    Surface.FillRectangle(BarRect, Tone.FieldWell, Radius);
    Surface.StrokeRectangle(BarRect, Tone.BorderSubtle, Radius, 1.0f);

    const Point2D Cursor = QueryInputCursor(Input);
    const float   Pad    = 2.5f;
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
            Surface.FillRectangle(SegRect, Tone.HighlightFill, SegRect.Height * 0.5f);
        }
        else if (Hovered)
        {
            Surface.FillRectangle(SegRect, Tone.RowHover, SegRect.Height * 0.5f);
        }

        const ColorRgba Ink = Selected ? Tone.HighlightInk : (Hovered ? Tone.TextPrimary : Tone.TextMuted);
        ConstructCenteredWeightedText(
            Surface, Selected ? FontWeightClass::Medium : FontWeightClass::Regular, SegRect, 9.5f, Ink, Labels[Idx]);

        if (Hovered && QueryPrimaryPressed(Input))
        {
            Picked = static_cast<int32_t>(Idx);
        }
    }
    return Picked;
}

// TextureStudio OLD Slider (.slider-control: Label on top; LEFT split reading|unit capsule + RIGHT thick capsule track & large thumb)
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

    // Label on top (.property-label)
    WorkspaceAppearance::ConstructWeightedText(
        Surface, FontWeightClass::Light, { Area.X, Area.Y + 1.0f }, 10.4f, Tone.TextSecondary, Label);

    const float RowY = Area.Y + 18.0f;
    constexpr float RowH = 26.0f;
    constexpr float Gap  = 8.0f;
    const float CapsuleW = std::min(82.0f, Area.Width * 0.38f);
    const Rectangle2D ReadCapsule { Area.X, RowY, CapsuleW, RowH };
    const Rectangle2D TrackRect   { ReadCapsule.Right() + Gap, RowY + 1.0f, std::max(44.0f, Area.Width - CapsuleW - Gap), RowH - 2.0f };

    // Left split reading | unit capsule (reading capsule + unit cell)
    const float CapRad = RowH * 0.5f;
    Surface.FillRectangle(ReadCapsule, Tone.FieldWell, CapRad);
    if (!UnitSuffix.empty() && CapsuleW >= 62.0f)
    {
        const float UnitW = 28.0f;
        const float NumW  = CapsuleW - UnitW;
        Surface.PushClip(ReadCapsule);
        Surface.FillRectangle({ ReadCapsule.X + NumW, ReadCapsule.Y, UnitW, RowH }, Tone.UnitWell, 0.0f);
        Surface.StrokeLine({ ReadCapsule.X + NumW, ReadCapsule.Y }, { ReadCapsule.X + NumW, ReadCapsule.Bottom() }, Tone.BorderSubtle, 1.0f);
        Surface.PopClip();
        ConstructCenteredWeightedText(Surface, FontWeightClass::Regular, { ReadCapsule.X, ReadCapsule.Y, NumW, RowH }, 10.2f, Tone.TextPrimary, NumBuf);
        ConstructCenteredWeightedText(Surface, FontWeightClass::Regular, { ReadCapsule.X + NumW, ReadCapsule.Y, UnitW, RowH }, 8.8f, Tone.TextMuted, UnitSuffix);
    }
    else
    {
        ConstructCenteredWeightedText(Surface, FontWeightClass::Regular, ReadCapsule, 10.2f, Tone.TextPrimary, NumBuf);
    }
    Surface.StrokeRectangle(ReadCapsule, (ActiveSliderId == SliderId) ? Tone.HighlightFill : Tone.BorderSubtle, CapRad, 1.0f);

    // Right thick capsule track + solid 22px light thumb (capsule track)
    const Point2D Cursor   = QueryInputCursor(Input);
    const bool    TrackHit = TrackRect.Contains(Cursor) || ReadCapsule.Contains(Cursor);

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
        const float Pad   = 11.0f;
        const float Ratio = std::clamp((Input.PrimaryPointer.X - (TrackRect.X + Pad)) / std::max(1.0f, TrackRect.Width - Pad * 2.0f), 0.0f, 1.0f);
        const float Next  = MinLimit + Ratio * Span;
        if (std::fabs(Next - InOutReading) > 1.0e-5f)
        {
            InOutReading = Next;
            Changed      = true;
        }
    }

    const float Norm     = std::clamp((InOutReading - MinLimit) / Span, 0.0f, 1.0f);
    const float TrackRad = TrackRect.Height * 0.5f;
    const float ThumbRad = (TrackRect.Height - 2.0f) * 0.5f;
    const float ThumbX   = TrackRect.X + ThumbRad + 1.0f + Norm * std::max(1.0f, TrackRect.Width - (ThumbRad + 1.0f) * 2.0f);

    Surface.FillRectangle(TrackRect, Tone.SliderTrack, TrackRad);
    const float FillW = std::max(TrackRect.Height, (ThumbX - TrackRect.X) + ThumbRad + 1.0f);
    Surface.FillRectangle({ TrackRect.X, TrackRect.Y, std::min(TrackRect.Width, FillW), TrackRect.Height }, Tone.SliderFill, TrackRad);
    Surface.FillCircle({ ThumbX, TrackRect.Center().Y }, ThumbRad, Tone.SliderThumb);
    return Changed;
}

// TextureStudio OLD Switch (.switch: 42x24; OFF=#232326 + light-grey knob; ON=#FFFFFF + dark knob)
inline bool ConstructStudioToggleRow(
    PixelSpace&             Surface,
    Rectangle2D             RowRect,
    std::string_view        Title,
    std::string_view        Hint,
    bool&                   InOutEnabled,
    const PaneInputReading& Input,
    const StudioToneSet&    Tone) noexcept
{
    const Point2D Cursor  = QueryInputCursor(Input);
    const bool    Hovered = RowRect.Contains(Cursor);

    const Rectangle2D SwitchRect { RowRect.Right() - 40.0f, RowRect.Center().Y - 11.0f, 40.0f, 22.0f };
    const float       TextClipW = std::max(24.0f, SwitchRect.X - RowRect.X - 8.0f);

    Surface.PushClip({ RowRect.X, RowRect.Y, TextClipW, RowRect.Height });
    WorkspaceAppearance::ConstructWeightedText(
        Surface, FontWeightClass::Light, { RowRect.X, RowRect.Y + (Hint.empty() ? 7.0f : 3.0f) }, 10.6f, Tone.TextSecondary, Title);
    if (!Hint.empty())
    {
        WorkspaceAppearance::ConstructWeightedText(
            Surface, FontWeightClass::Regular, { RowRect.X, RowRect.Y + 18.0f }, 8.8f, Tone.TextFaint, Hint);
    }
    Surface.PopClip();

    const ColorRgba TrackOn  = Tone.LightSurface ? ColorRgba{ 28u, 32u, 40u, 255u } : ColorRgba{ 255u, 255u, 255u, 255u };
    const ColorRgba KnobOn   = Tone.LightSurface ? ColorRgba{ 250u, 250u, 252u, 255u } : ColorRgba{ 17u, 17u, 17u, 255u };
    const ColorRgba TrackOff = Tone.SliderTrack;
    const ColorRgba KnobOff  = Tone.LightSurface ? ColorRgba{ 102u, 108u, 120u, 255u } : ColorRgba{ 188u, 188u, 192u, 255u };

    Surface.FillRectangle(SwitchRect, InOutEnabled ? TrackOn : TrackOff, 11.0f);
    Surface.StrokeRectangle(SwitchRect, InOutEnabled ? TrackOn : Tone.BorderSubtle, 11.0f, 1.0f);
    const float KnobX = InOutEnabled ? (SwitchRect.Right() - 11.0f) : (SwitchRect.X + 11.0f);
    Surface.FillCircle({ KnobX, SwitchRect.Center().Y }, 8.0f, InOutEnabled ? KnobOn : KnobOff);

    if (Hovered && QueryPrimaryPressed(Input))
    {
        InOutEnabled = !InOutEnabled;
        return true;
    }
    return false;
}

// Edge-to-edge Inspector Footer Bar (sharp pane edges, lime-cream primary CTA + dark secondary buttons + status line)
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
    Surface.FillRectangle(FooterRect, Tone.FooterSurface, 0.0f);
    Surface.StrokeLine({ FooterRect.X, FooterRect.Y }, { FooterRect.Right(), FooterRect.Y }, Tone.Hairline, 1.0f);

    const Point2D Cursor = QueryInputCursor(Input);
    const float   PadX   = 12.0f;
    const float   BtnY   = FooterRect.Y + 9.0f;
    constexpr float BtnH = 27.0f;
    constexpr float Gap  = 6.0f;

    const int32_t BtnCount = 1 + (!SecondaryALabel.empty() ? 1 : 0) + (!SecondaryBLabel.empty() ? 1 : 0);
    const float   TotalW   = FooterRect.Width - PadX * 2.0f;
    const float   PrimW    = (BtnCount == 1) ? TotalW : (BtnCount == 2 ? (TotalW - Gap) * 0.56f : (TotalW - Gap * 2.0f) * 0.44f);
    const float   SecW     = (BtnCount > 1) ? (TotalW - PrimW - Gap * static_cast<float>(BtnCount - 1)) / static_cast<float>(BtnCount - 1) : 0.0f;

    // Signature TextureStudio lime-cream primary command capsule
    const Rectangle2D PrimRect { FooterRect.X + PadX, BtnY, PrimW, BtnH };
    const bool PrimHover = PrimRect.Contains(Cursor);
    Surface.FillRectangle(PrimRect, PrimHover ? ColorRgba::Blend(Tone.PrimaryFill, Tone.WhiteInk, 0.25f) : Tone.PrimaryFill, BtnH * 0.5f);
    ConstructCenteredWeightedText(
        Surface, FontWeightClass::Medium, PrimRect, 10.2f, Tone.PrimaryInk, PrimaryLabel);
    Outcome.PrimaryClicked = PrimHover && QueryPrimaryPressed(Input);

    float RunX = PrimRect.Right() + Gap;
    if (!SecondaryALabel.empty())
    {
        const Rectangle2D SecRect { RunX, BtnY, SecW, BtnH };
        const bool Hover = SecRect.Contains(Cursor);
        Surface.FillRectangle(SecRect, Hover ? Tone.RowHover : Tone.TileSurface, BtnH * 0.5f);
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
        Surface.FillRectangle(SecRect, Hover ? Tone.RowHover : Tone.TileSurface, BtnH * 0.5f);
        Surface.StrokeRectangle(SecRect, Hover ? Tone.BorderStrong : Tone.BorderSubtle, BtnH * 0.5f, 1.0f);
        ConstructCenteredWeightedText(
            Surface, FontWeightClass::Medium, SecRect, 9.8f, Hover ? Tone.TextPrimary : Tone.TextSecondary, SecondaryBLabel);
        Outcome.SecondaryBClicked = Hover && QueryPrimaryPressed(Input);
    }

    const float StatusY = BtnY + BtnH + 9.0f;
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
    Surface.FillRectangle(PopRect, Tone.SectionHeaderBg, 14.0f);
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
