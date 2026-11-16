/*============================================================================================================================================
                                                         VECTORICONREPOSITORY.CPP
============================================================================================================================================*/
// 🧩 Single dedicated shared-memory icon repository and rasterizer for FrontierEditor and all standalone workspace executables.

//------------------------------------------------------------------------------------------------------------------------
//                                                        INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Engine/Components/VectorIcons/VectorIconRepository.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <mutex>
#include <string>
#include <unordered_map>

namespace Frontier
{

namespace
{

constexpr float       PiConstant     = 3.14159265358979323846f;
constexpr int         CurveSegments  = 12;
constexpr float       ArcStepRadians = 0.24f;
constexpr std::size_t StackScratchN  = 192u;

[[nodiscard]] ColorRgba FadeColorAlpha(ColorRgba Ink, float Factor) noexcept
{
    const float Clamped = std::clamp(Factor, 0.0f, 1.0f);
    Ink.Alpha = static_cast<uint8_t>(std::lround(static_cast<float>(Ink.Alpha) * Clamped));
    return Ink;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    SVG PATH DECODER
//------------------------------------------------------------------------------------------------------------------------

struct SvgPathScanner
{
    std::string_view Source;
    std::size_t      Cursor = 0u;

    void SkipSeparators() noexcept
    {
        while (Cursor < Source.size())
        {
            const unsigned char Ch = static_cast<unsigned char>(Source[Cursor]);
            if (std::isspace(Ch) || Ch == ',')
                ++Cursor;
            else
                break;
        }
    }

    [[nodiscard]] bool AtEnd() noexcept
    {
        SkipSeparators();
        return Cursor >= Source.size();
    }

    [[nodiscard]] bool PeekCommand(char& OutCommand) noexcept
    {
        SkipSeparators();
        if (Cursor >= Source.size())
            return false;
        const unsigned char Ch = static_cast<unsigned char>(Source[Cursor]);
        if (std::isalpha(Ch))
        {
            OutCommand = static_cast<char>(Ch);
            return true;
        }
        return false;
    }

    [[nodiscard]] bool ReadScalar(float& OutScalar) noexcept
    {
        SkipSeparators();
        if (Cursor >= Source.size())
            return false;
        const std::size_t Start = Cursor;
        if (Source[Cursor] == '+' || Source[Cursor] == '-')
            ++Cursor;
        bool DigitSeen = false;
        while (Cursor < Source.size() && std::isdigit(static_cast<unsigned char>(Source[Cursor])))
        {
            DigitSeen = true;
            ++Cursor;
        }
        if (Cursor < Source.size() && Source[Cursor] == '.')
        {
            ++Cursor;
            while (Cursor < Source.size() && std::isdigit(static_cast<unsigned char>(Source[Cursor])))
            {
                DigitSeen = true;
                ++Cursor;
            }
        }
        if (!DigitSeen)
        {
            Cursor = Start;
            return false;
        }
        if (Cursor < Source.size() && (Source[Cursor] == 'e' || Source[Cursor] == 'E'))
        {
            const std::size_t ExpMark = Cursor++;
            if (Cursor < Source.size() && (Source[Cursor] == '+' || Source[Cursor] == '-'))
                ++Cursor;
            bool ExpDigits = false;
            while (Cursor < Source.size() && std::isdigit(static_cast<unsigned char>(Source[Cursor])))
            {
                ExpDigits = true;
                ++Cursor;
            }
            if (!ExpDigits)
                Cursor = ExpMark;
        }
        char Buffer[64]{};
        const std::size_t Len = std::min(Cursor - Start, sizeof(Buffer) - 1u);
        for (std::size_t Index = 0u; Index < Len; ++Index)
            Buffer[Index] = Source[Start + Index];
        OutScalar = std::strtof(Buffer, nullptr);
        return true;
    }

    [[nodiscard]] bool ReadArcBit(bool& OutBit) noexcept
    {
        SkipSeparators();
        if (Cursor >= Source.size())
            return false;
        if (Source[Cursor] == '0' || Source[Cursor] == '1')
        {
            OutBit = (Source[Cursor] == '1');
            ++Cursor;
            return true;
        }
        return false;
    }
};

void AppendCubicBezier(std::vector<Point2D>& Out, Point2D P0, Point2D P1, Point2D P2, Point2D P3, int Segments) noexcept
{
    for (int Step = 1; Step <= Segments; ++Step)
    {
        const float T   = static_cast<float>(Step) / static_cast<float>(Segments);
        const float Inv = 1.0f - T;
        const float B0  = Inv * Inv * Inv;
        const float B1  = 3.0f * Inv * Inv * T;
        const float B2  = 3.0f * Inv * T * T;
        const float B3  = T * T * T;
        Out.push_back({
            B0 * P0.X + B1 * P1.X + B2 * P2.X + B3 * P3.X,
            B0 * P0.Y + B1 * P1.Y + B2 * P2.Y + B3 * P3.Y,
        });
    }
}

void AppendQuadraticBezier(std::vector<Point2D>& Out, Point2D P0, Point2D P1, Point2D P2, int Segments) noexcept
{
    for (int Step = 1; Step <= Segments; ++Step)
    {
        const float T   = static_cast<float>(Step) / static_cast<float>(Segments);
        const float Inv = 1.0f - T;
        Out.push_back({
            Inv * Inv * P0.X + 2.0f * Inv * T * P1.X + T * T * P2.X,
            Inv * Inv * P0.Y + 2.0f * Inv * T * P1.Y + T * T * P2.Y,
        });
    }
}

void AppendEllipticalArc(
    std::vector<Point2D>& Out,
    Point2D               P0,
    float                 Rx,
    float                 Ry,
    float                 RotationDegrees,
    bool                  LargeArc,
    bool                  Sweep,
    Point2D               P1,
    float                 StepRadians) noexcept
{
    if (Rx == 0.0f || Ry == 0.0f || (P0.X == P1.X && P0.Y == P1.Y))
    {
        Out.push_back(P1);
        return;
    }
    Rx = std::fabs(Rx);
    Ry = std::fabs(Ry);

    const float Phi    = RotationDegrees * PiConstant / 180.0f;
    const float CosPhi = std::cos(Phi);
    const float SinPhi = std::sin(Phi);
    const float Dx     = (P0.X - P1.X) * 0.5f;
    const float Dy     = (P0.Y - P1.Y) * 0.5f;
    const float X1p    = CosPhi * Dx + SinPhi * Dy;
    const float Y1p    = -SinPhi * Dx + CosPhi * Dy;

    const float Lambda = (X1p * X1p) / (Rx * Rx) + (Y1p * Y1p) / (Ry * Ry);
    if (Lambda > 1.0f)
    {
        const float ScaleUp = std::sqrt(Lambda);
        Rx *= ScaleUp;
        Ry *= ScaleUp;
    }

    const float Rx2       = Rx * Rx;
    const float Ry2       = Ry * Ry;
    float       Numerator = Rx2 * Ry2 - Rx2 * Y1p * Y1p - Ry2 * X1p * X1p;
    if (Numerator < 0.0f)
        Numerator = 0.0f;
    const float Denom = Rx2 * Y1p * Y1p + Ry2 * X1p * X1p;
    float Coefficient = (Denom > 1.0e-12f) ? std::sqrt(Numerator / Denom) : 0.0f;
    if (LargeArc == Sweep)
        Coefficient = -Coefficient;

    const float Cxp = Coefficient * (Rx * Y1p / Ry);
    const float Cyp = Coefficient * -(Ry * X1p / Rx);
    const float Cx  = CosPhi * Cxp - SinPhi * Cyp + (P0.X + P1.X) * 0.5f;
    const float Cy  = SinPhi * Cxp + CosPhi * Cyp + (P0.Y + P1.Y) * 0.5f;

    auto ComputeAngle = [](float Ux, float Uy, float Vx, float Vy) noexcept -> float {
        const float Dot = Ux * Vx + Uy * Vy;
        const float Len = std::sqrt((Ux * Ux + Uy * Uy) * (Vx * Vx + Vy * Vy));
        if (Len <= 1.0e-12f)
            return 0.0f;
        float Ang = std::acos(std::fmax(-1.0f, std::fmin(1.0f, Dot / Len)));
        if (Ux * Vy - Uy * Vx < 0.0f)
            Ang = -Ang;
        return Ang;
    };

    const float Theta1     = ComputeAngle(1.0f, 0.0f, (X1p - Cxp) / Rx, (Y1p - Cyp) / Ry);
    float       DeltaTheta = ComputeAngle((X1p - Cxp) / Rx, (Y1p - Cyp) / Ry, (-X1p - Cxp) / Rx, (-Y1p - Cyp) / Ry);
    if (!Sweep && DeltaTheta > 0.0f)
        DeltaTheta -= 2.0f * PiConstant;
    if (Sweep && DeltaTheta < 0.0f)
        DeltaTheta += 2.0f * PiConstant;

    const int Segments = std::max(1, static_cast<int>(std::ceil(std::fabs(DeltaTheta) / StepRadians)));
    for (int Step = 1; Step <= Segments; ++Step)
    {
        const float Angle = Theta1 + DeltaTheta * static_cast<float>(Step) / static_cast<float>(Segments);
        const float Ex    = Rx * std::cos(Angle);
        const float Ey    = Ry * std::sin(Angle);
        Out.push_back({ CosPhi * Ex - SinPhi * Ey + Cx, SinPhi * Ex + CosPhi * Ey + Cy });
    }
}

//------------------------------------------------------------------------------------------------------------------------
//                                             UNIFIED ICON SPECIFICATION TABLES
//------------------------------------------------------------------------------------------------------------------------

const std::array<VectorIconEntry, static_cast<std::size_t>(NavigationIcon::Count)> NavigationEntries = {{
    {"ArrowUp",                "M12 19V5M5 12l7-7 7 7",                                                                        24.0f, 2.0f},
    {"ArrowDown",              "M12 5v14M19 12l-7 7-7-7",                                                                      24.0f, 2.0f},
    {"ArrowLeft",              "M19 12H5M12 19l-7-7 7-7",                                                                      24.0f, 2.0f},
    {"ArrowRight",             "M5 12h14M12 5l7 7-7 7",                                                                        24.0f, 2.0f},
    {"ChevronUp",              "M18 15l-6-6-6 6",                                                                              24.0f, 2.0f},
    {"ChevronDown",            "M6 9l6 6 6-6",                                                                                 24.0f, 2.0f},
    {"ChevronLeft",            "M15 18l-6-6 6-6",                                                                              24.0f, 2.0f},
    {"ChevronRight",           "M9 18l6-6-6-6",                                                                                24.0f, 2.0f},
    {"ChevronsUp",             "M17 11l-5-5-5 5M17 18l-5-5-5 5",                                                               24.0f, 2.0f},
    {"ChevronsDown",           "M7 13l5 5 5-5M7 6l5 5 5-5",                                                                    24.0f, 2.0f},
    {"ChevronsLeft",           "M11 17l-5-5 5-5M18 17l-5-5 5-5",                                                               24.0f, 2.0f},
    {"ChevronsRight",          "M13 17l5-5-5-5M6 17l5-5-5-5",                                                                  24.0f, 2.0f},
    {"CornerDownRight",        "M15 10l5 5-5 5M4 4v7a4 4 0 0 0 4 4h12",                                                        24.0f, 2.0f},
    {"CornerDownLeft",         "M9 10l-5 5 5 5M20 4v7a4 4 0 0 1-4 4H4",                                                        24.0f, 2.0f},
    {"RotateClockwise",        "M21 12a9 9 0 1 1-9-9c2.52 0 4.93 1 6.74 2.74L21 8M21 3v5h-5",                                  24.0f, 2.0f},
    {"RotateCounterClockwise", "M3 12a9 9 0 1 0 9-9 9.75 9.75 0 0 0-6.74 2.74L3 8M3 3v5h5",                                    24.0f, 2.0f},
    {"ExpandDiagonal",         "M15 3h6v6M9 21H3v-6M21 3l-7 7M3 21l7-7",                                                       24.0f, 2.0f},
    {"CollapseDiagonal",       "M4 14h6v6M20 10h-6V4M14 10l7-7M3 21l7-7",                                                      24.0f, 2.0f},
}};

const std::array<VectorIconEntry, static_cast<std::size_t>(ControlIcon::Count)> ControlEntries = {{
    // 0..85: Existing ControlIcon ordinals
    {"SettingsGear", "M9.671 4.136a2.34 2.34 0 0 1 4.659 0 2.34 2.34 0 0 0 3.319 1.915 2.34 2.34 0 0 1 2.33 4.033 2.34 2.34 0 0 0 0 3.831 2.34 2.34 0 0 1-2.33 4.033 2.34 2.34 0 0 0-3.319 1.915 2.34 2.34 0 0 1-4.659 0 2.34 2.34 0 0 0-3.32-1.915 2.34 2.34 0 0 1-2.33-4.033 2.34 2.34 0 0 0 0-3.831A2.34 2.34 0 0 1 6.35 6.051a2.34 2.34 0 0 0 3.319-1.915M15 12a3 3 0 1 1-6 0 3 3 0 0 1 6 0z", 24.0f, 2.0f},
    {"AppearanceSwatches", "M12 22a1 1 0 0 1 0-20 10 9 0 0 1 10 9 5 5 0 0 1-5 5h-2.25a1.75 1.75 0 0 0-1.4 2.8l.3.4a1.75 1.75 0 0 1-1.4 2.8zM14 6.5a.5.5 0 1 1-1 0 .5.5 0 0 1 1 0zM18 10.5a.5.5 0 1 1-1 0 .5.5 0 0 1 1 0zM7 12.5a.5.5 0 1 1-1 0 .5.5 0 0 1 1 0zM9 7.5a.5.5 0 1 1-1 0 .5.5 0 0 1 1 0z", 24.0f, 2.0f},
    {"DisplayMonitor", "M4 3h16a2 2 0 0 1 2 2v10a2 2 0 0 1-2 2H4a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2zM8 21h8M12 17v4", 24.0f, 2.0f},
    {"InputDevices", "M12 22s8-4 8-10V5l-8-3-8 3v7c0 6 8 10 8 10z", 24.0f, 2.0f},
    {"NotificationsBell", "M10.268 21a2 2 0 0 0 3.464 0M3.262 15.326A1 1 0 0 0 4 17h16a1 1 0 0 0 .74-1.673C19.41 13.956 18 12.499 18 8A6 6 0 0 0 6 8c0 4.499-1.411 5.956-2.738 7.326", 24.0f, 2.0f},
    {"WirelessSignal", "M12 20h.01M2 8.82a15 15 0 0 1 20 0M5 12.859a10 10 0 0 1 14 0M8.5 16.429a5 5 0 0 1 7 0", 24.0f, 2.0f},
    {"BluetoothSymbol", "M7 7l10 10-5 5V2l5 5L7 17", 24.0f, 2.0f},
    {"MoonDisturbance", "M21 12.79A9 9 0 1 1 11.21 3 7 7 0 0 0 21 12.79z", 24.0f, 2.0f},
    {"VolumeSpeaker", "M11 5L6 9H2v6h4l5 4V5zm8.07-.07a10 10 0 0 1 0 14.14M15.54 8.46a5 5 0 0 1 0 7.07", 24.0f, 2.0f},
    {"SunIllumination", "M16 12a4 4 0 1 1-8 0 4 4 0 0 1 8 0zM12 2v2M12 20v2M4.93 4.93l1.41 1.41M17.66 17.66l1.41 1.41M2 12h2M20 12h2M6.34 17.66l-1.41 1.41M19.07 4.93l-1.41 1.41", 24.0f, 2.0f},
    {"SparklesAntiAliasing", "M11.017 2.814a1 1 0 0 1 1.966 0l1.051 5.558a2 2 0 0 0 1.594 1.594l5.558 1.051a1 1 0 0 1 0 1.966l-5.558 1.051a2 2 0 0 0-1.594 1.594l-1.051 5.558a1 1 0 0 1-1.966 0l-1.051-5.558a2 2 0 0 0-1.594-1.594l-5.558-1.051a1 1 0 0 1 0-1.966l5.558-1.051a2 2 0 0 0 1.594-1.594zM20 2v4M22 4h-4M6 20a2 2 0 1 1-4 0 2 2 0 0 1 4 0z", 24.0f, 2.0f},
    {"GaugeRate", "m12 14 4-4M3.34 19a10 10 0 1 1 17.32 0", 24.0f, 2.0f},
    {"SlidersQuality", "M10 5H3M12 19H3M14 3v4M16 17v4M21 12h-9M21 19h-5M21 5h-7M8 10v4M8 12H3", 24.0f, 2.0f},
    {"VideoRenderScale", "m16 13 5.223 3.482a.5.5 0 0 0 .777-.416V7.87a.5.5 0 0 0-.752-.432L16 10.5M4 6h10a2 2 0 0 1 2 2v8a2 2 0 0 1-2 2H4a2 2 0 0 1-2-2V8a2 2 0 0 1 2-2z", 24.0f, 2.0f},
    {"CloseCross", "M18 6 6 18M6 6l12 12", 24.0f, 2.0f},
    {"ChevronBack", "m15 18-6-6 6-6", 24.0f, 2.0f},
    {"ChevronForward", "m9 18 6-6-6-6", 24.0f, 2.0f},
    {"ShieldInput", "M20 13c0 5-3.5 7.5-7.66 8.95a1 1 0 0 1-.67-.01C7.5 20.5 4 18 4 13V6a1 1 0 0 1 1-1c2 0 4.5-1.2 6.24-2.72a1.17 1.17 0 0 1 1.52 0C14.51 3.81 17 5 19 5a1 1 0 0 1 1 1z", 24.0f, 2.0f},
    {"ChevronDown", "m6 9 6 6 6-6", 24.0f, 2.0f},
    {"ChevronUp", "m18 15-6-6-6 6", 24.0f, 2.0f},
    {"TriangleAlert", "m21.73 18-8-14a2 2 0 0 0-3.48 0l-8 14A2 2 0 0 0 4 21h16a2 2 0 0 0 1.73-3ZM12 9v4M12 17h.01", 24.0f, 2.0f},
    {"CircleCheck", "M22 11.08V12a10 10 0 1 1-5.93-9.14M9 11l3 3L22 4", 24.0f, 2.0f},
    {"CircleNotice", "M12 2a10 10 0 1 1 0 20 10 10 0 0 1 0-20zM12 16v-4M12 8h.01", 24.0f, 2.0f},
    {"OctagonAlert", "M12 16h.01M12 8v4M15.312 2a2 2 0 0 1 1.414.586l4.688 4.688A2 2 0 0 1 22 8.688v6.624a2 2 0 0 1-.586 1.414l-4.688 4.688a2 2 0 0 1-1.414.586H8.688a2 2 0 0 1-1.414-.586l-4.688-4.688A2 2 0 0 1 2 15.312V8.688a2 2 0 0 1 .586-1.414l4.688-4.688A2 2 0 0 1 8.688 2z", 24.0f, 2.0f},
    {"EyeVisible", "M2.062 12.348a1 1 0 0 1 0-.696 10.75 10.75 0 0 1 19.876 0 1 1 0 0 1 0 .696 10.75 10.75 0 0 1-19.876 0M15 12a3 3 0 1 1-6 0 3 3 0 0 1 6 0z", 24.0f, 2.0f},
    {"EyeHidden", "M10.733 5.076a10.744 10.744 0 0 1 11.205 6.575 1 1 0 0 1 0 .696 10.747 10.747 0 0 1-1.444 2.49M14.084 14.158a3 3 0 0 1-4.242-4.242M17.479 17.499a10.75 10.75 0 0 1-15.417-5.151 1 1 0 0 1 0-.696 10.75 10.75 0 0 1 4.446-5.143M2 2l20 20", 24.0f, 2.0f},
    {"LockClosed", "M5 13a2 2 0 0 1 2-2h10a2 2 0 0 1 2 2v6a2 2 0 0 1-2 2H7a2 2 0 0 1-2-2zM7 11V7a5 5 0 0 1 10 0v4", 24.0f, 2.0f},
    {"LockOpen", "M5 13a2 2 0 0 1 2-2h10a2 2 0 0 1 2 2v6a2 2 0 0 1-2 2H7a2 2 0 0 1-2-2zM7 11V7a5 5 0 0 1 9.9-1", 24.0f, 2.0f},
    {"MotionActivity", "M22 12h-2.48a2 2 0 0 0-1.93 1.46l-2.35 8.36a.25.25 0 0 1-.48 0L9.24 2.18a.25.25 0 0 0-.48 0l-2.35 8.36A2 2 0 0 1 4.49 12H2", 24.0f, 2.0f},
    {"FolderClosed", "M20 20a2 2 0 0 0 2-2V8a2 2 0 0 0-2-2h-7.9a2 2 0 0 1-1.69-.9L9.6 3.9A2 2 0 0 0 7.93 3H4a2 2 0 0 0-2 2v13a2 2 0 0 0 2 2z", 24.0f, 2.0f},
    {"FolderOpen", "M2 7.5V5a2 2 0 0 1 2-2h3.9a2 2 0 0 1 1.69.9l.81 1.2a2 2 0 0 0 1.67.9H20a2 2 0 0 1 2 2v1.5M2.239 18.578A2 2 0 0 0 4 20h16a2 2 0 0 0 1.964-1.618l1.402-7A1 1 0 0 0 22.386 10H1.614a1 1 0 0 0-.98 1.382z", 24.0f, 2.0f},
    {"CubeShape", "M21 8a2 2 0 0 0-1-1.73l-7-4a2 2 0 0 0-2 0l-7 4A2 2 0 0 0 3 8v8a2 2 0 0 0 1 1.73l7 4a2 2 0 0 0 2 0l7-4A2 2 0 0 0 21 16zM3.3 7l8.7 5 8.7-5M12 22V12", 24.0f, 2.0f},
    {"SearchGlass", "M18 11a7 7 0 1 1-14 0 7 7 0 0 1 14 0zM21 21l-4.35-4.35", 24.0f, 2.0f},
    {"PlusAdd", "M5 12h14M12 5v14", 24.0f, 2.0f},
    {"TrashDelete", "M3 6h18M8 6V4a2 2 0 0 1 2-2h4a2 2 0 0 1 2 2v2M19 6l-1 14a2 2 0 0 1-2 2H8a2 2 0 0 1-2-2L5 6M10 11v6M14 11v6", 24.0f, 2.0f},
    {"LayoutSplit", "M5 3h14a2 2 0 0 1 2 2v14a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2zM12 3v18", 24.0f, 2.0f},
    {"LayoutPanelLeft", "M5 3h14a2 2 0 0 1 2 2v14a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2zM9 3v18", 24.0f, 2.0f},
    {"LayoutPanelRight", "M5 3h14a2 2 0 0 1 2 2v14a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2zM15 3v18", 24.0f, 2.0f},
    {"CameraBody", "M16 8a2 2 0 0 0-2-2H4a2 2 0 0 0-2 2v8a2 2 0 0 0 2 2h10a2 2 0 0 0 2-2zM16 10l5.24-3.14a.5.5 0 0 1 .76.43v9.42a.5.5 0 0 1-.76.43L16 14", 24.0f, 2.0f},
    {"LayersSlabs", "M12.83 2.18a2 2 0 0 0-1.66 0L2.6 6.08a1 1 0 0 0 0 1.83l8.58 3.91a2 2 0 0 0 1.66 0l8.58-3.9a1 1 0 0 0 0-1.83zM2 12a1 1 0 0 0 .58.91l8.6 3.91a2 2 0 0 0 1.65 0l8.58-3.9A1 1 0 0 0 22 12M2 17a1 1 0 0 0 .58.91l8.6 3.91a2 2 0 0 0 1.65 0l8.58-3.9A1 1 0 0 0 22 17", 24.0f, 2.0f},
    {"SaveDisk", "M15.2 3a2 2 0 0 1 1.4.6l3.8 3.8a2 2 0 0 1 .6 1.4V19a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2zM17 21v-7a1 1 0 0 0-1-1H8a1 1 0 0 0-1 1v7M7 3v4a1 1 0 0 0 1 1h7", 24.0f, 2.0f},
    {"ExternalOpen", "M15 3h6v6M10 14L21 3M18 13v6a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V8a2 2 0 0 1 2-2h6", 24.0f, 2.0f},
    {"LightbulbHint", "M15 14c.2-1 .7-1.7 1.5-2.5 1-.9 1.5-2.2 1.5-3.5A6 6 0 0 0 6 8c0 1 .2 2.2 1.5 3.5.7.7 1.3 1.5 1.5 2.5M9 18h6M10 22h4", 24.0f, 2.0f},
    {"QuestionCircle", "M12 2a10 10 0 1 1 0 20 10 10 0 0 1 0-20zM9.09 9a3 3 0 0 1 5.83 1c0 2-3 3-3 3M12 17h.01", 24.0f, 2.0f},
    {"WrenchRepair", "M14.7 6.3a1 1 0 0 0 0 1.4l1.6 1.6a1 1 0 0 0 1.4 0l3.77-3.77a6 6 0 0 1-7.94 7.94l-6.91 6.91a2.12 2.12 0 0 1-3-3l6.91-6.91a6 6 0 0 1 7.94-7.94l-3.76 3.76z", 24.0f, 2.0f},
    {"RefreshCycle", "M3 12a9 9 0 0 1 15-6.7L21 8M21 3v5h-5M21 12a9 9 0 0 1-15 6.7L3 16M3 21v-5h5", 24.0f, 2.0f},
    {"LayoutGrid", "M4 4h6a1 1 0 0 1 1 1v5a1 1 0 0 1-1 1H4a1 1 0 0 1-1-1V5a1 1 0 0 1 1-1zM14 4h6a1 1 0 0 1 1 1v5a1 1 0 0 1-1 1h-6a1 1 0 0 1-1-1V5a1 1 0 0 1 1-1zM4 14h6a1 1 0 0 1 1 1v5a1 1 0 0 1-1 1H4a1 1 0 0 1-1-1v-5a1 1 0 0 1 1-1zM14 14h6a1 1 0 0 1 1 1v5a1 1 0 0 1-1 1h-6a1 1 0 0 1-1-1v-5a1 1 0 0 1 1-1z", 24.0f, 2.0f},
    {"LayoutList", "M4 6h.01M9 6h11M4 12h.01M9 12h11M4 18h.01M9 18h11", 24.0f, 2.0f},
    {"LayoutColumns", "M5 4h14a2 2 0 0 1 2 2v12a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V6a2 2 0 0 1 2-2zM9.5 4v16M14.5 4v16", 24.0f, 2.0f},
    {"LayoutRows", "M5 4h14a2 2 0 0 1 2 2v12a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V6a2 2 0 0 1 2-2zM3 9.5h18M3 14.5h18", 24.0f, 2.0f},
    {"MaterialSphere", "M3 12a9 9 0 1 0 18 0a9 9 0 1 0-18 0M12 3a9 9 0 0 0 0 18M12 3c2.8 0 5 4 5 9s-2.2 9-5 9", 24.0f, 2.0f},
    {"PatternTile", "M5.5 3h13A2.5 2.5 0 0 1 21 5.5v13a2.5 2.5 0 0 1-2.5 2.5h-13A2.5 2.5 0 0 1 3 18.5v-13A2.5 2.5 0 0 1 5.5 3zM3 9h18M3 15h18M9 3v18M15 3v18", 24.0f, 2.0f},
    {"StampTool", "M5 18h14v3H5zM7 14h10a2 2 0 0 1 2 2v2H5v-2a2 2 0 0 1 2-2zM12 14V9.5c0-1.2 2.2-2.2 2.2-4.2a2.2 2.2 0 1 0-4.4 0c0 2 2.2 3 2.2 4.2", 24.0f, 2.0f},
    {"BrushTool", "M18.5 2.5a2.8 2.8 0 0 1 3 3L11.8 15.2l-3.6.6.6-3.6zM6.2 15.4c-1.8 0-3.2 1.4-3.2 3.2 0 .9-.7 1.6-1.5 1.9 1.3.7 3.3.9 4.8.2 1.3-.6 2.1-1.9 2.1-3.3a2.2 2.2 0 0 0-2.2-2z", 24.0f, 2.0f},
    {"ModifierStar", "M12 3v3M12 18v3M3 12h3M18 12h3M8 12a4 4 0 1 0 8 0a4 4 0 1 0-8 0", 24.0f, 2.0f},
    {"NoiseWave", "M3 13c2.5-6 4.5-6 6 0s3.5 6 6 0 3.5-6 6 0M3 8c1.5-2.5 3-2.5 4.5 0s3 2.5 4.5 0 3-2.5 4.5 0 3 2.5 4.5 0", 24.0f, 2.0f},
    {"FolderPlus", "M3 7a2 2 0 0 1 2-2h4.2l2 2.2H19a2 2 0 0 1 2 2V18a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2zM12 11v6M9 14h6", 24.0f, 2.0f},
    {"MaskCutout", "M6.5 3.5h11a3 3 0 0 1 3 3v11a3 3 0 0 1-3 3h-11a3 3 0 0 1-3-3v-11a3 3 0 0 1 3-3zM7.5 12a4.5 4.5 0 1 0 9 0a4.5 4.5 0 1 0-9 0", 24.0f, 2.0f},
    {"CopyDuplicate", "M11 9h7a2 2 0 0 1 2 2v7a2 2 0 0 1-2 2h-7a2 2 0 0 1-2-2v-7a2 2 0 0 1 2-2zM5 15H4a2 2 0 0 1-2-2V4a2 2 0 0 1 2-2h9a2 2 0 0 1 2 2v1", 24.0f, 2.0f},
    {"DragGrip", "M7.7 6a1.3 1.3 0 1 0 2.6 0a1.3 1.3 0 1 0-2.6 0M13.7 6a1.3 1.3 0 1 0 2.6 0a1.3 1.3 0 1 0-2.6 0M7.7 12a1.3 1.3 0 1 0 2.6 0a1.3 1.3 0 1 0-2.6 0M13.7 12a1.3 1.3 0 1 0 2.6 0a1.3 1.3 0 1 0-2.6 0M7.7 18a1.3 1.3 0 1 0 2.6 0a1.3 1.3 0 1 0-2.6 0M13.7 18a1.3 1.3 0 1 0 2.6 0a1.3 1.3 0 1 0-2.6 0", 24.0f, 2.0f},
    {"VectorPointer", "M5 3l14 7-6 2-2 6z", 24.0f, 2.0f},
    {"VectorCurve", "M4 18C9 18 10 6 20 6M2.2 18a1.8 1.8 0 1 0 3.6 0a1.8 1.8 0 1 0-3.6 0M10.2 12a1.8 1.8 0 1 0 3.6 0a1.8 1.8 0 1 0-3.6 0M18.2 6a1.8 1.8 0 1 0 3.6 0a1.8 1.8 0 1 0-3.6 0", 24.0f, 2.0f},
    {"VectorPen", "M12 19l7-7 3 3-7 7-3-3zM18 13l-1.5-7.5L2 2l3.5 14.5L13 18l5-5zM2 2l7.586 7.586M9 11a2 2 0 1 0 4 0a2 2 0 1 0-4 0", 24.0f, 2.0f},
    {"EditFocus", "M4 9V5h4M16 5h4v4M20 15v4h-4M8 19H4v-4M9.8 12a2.2 2.2 0 1 0 4.4 0a2.2 2.2 0 1 0-4.4 0", 24.0f, 2.0f},
    {"PointGradient", "M4 17a3 3 0 1 0 6 0a3 3 0 1 0-6 0M14 7a3 3 0 1 0 6 0a3 3 0 1 0-6 0M14.8 17a2.2 2.2 0 1 0 4.4 0a2.2 2.2 0 1 0-4.4 0M9.2 14.8L14.8 9.2M10 17h4.5M17 10v4.5", 24.0f, 2.0f},
    {"ShapeSquare", "M6 4h12a2 2 0 0 1 2 2v12a2 2 0 0 1-2 2H6a2 2 0 0 1-2-2V6a2 2 0 0 1 2-2z", 24.0f, 2.0f},
    {"ShapeCircle", "M3.5 12a8.5 8.5 0 1 0 17 0a8.5 8.5 0 1 0-17 0", 24.0f, 2.0f},
    {"ShapeDiamond", "M12 3l9 9-9 9-9-9z", 24.0f, 2.0f},
    {"ShapeTriangle", "M12 4l9 16H3z", 24.0f, 2.0f},
    {"ShapeStar", "M12 3l2.7 5.6 6.2.9-4.5 4.4 1.1 6.1L12 17l-5.5 3 1.1-6.1-4.5-4.4 6.2-.9z", 24.0f, 2.0f},
    {"ShapeHexagon", "M12 3l7.8 4.5v9L12 21l-7.8-4.5v-9z", 24.0f, 2.0f},
    {"ShapeStripe", "M4 9h16v6H4z", 24.0f, 2.0f},
    {"TextType", "M5 6V4h14v2M12 4v16M9 20h6", 24.0f, 2.0f},
    {"HandPan", "M18 11V6a1.5 1.5 0 0 0-3 0v4M15 10V4.5a1.5 1.5 0 0 0-3 0V10M12 10V5.5a1.5 1.5 0 0 0-3 0v6.5L7.3 10.3a1.5 1.5 0 0 0-2.1 2.1l4.1 5.8A4.5 4.5 0 0 0 13 20h2.5a4.5 4.5 0 0 0 4.5-4.5V11a1.5 1.5 0 0 0-2 0z", 24.0f, 2.0f},
    {"ImageTile", "M5.5 4h13A2.5 2.5 0 0 1 21 6.5v11a2.5 2.5 0 0 1-2.5 2.5h-13A2.5 2.5 0 0 1 3 17.5v-11A2.5 2.5 0 0 1 5.5 4zM7 9.5a1.5 1.5 0 1 0 3 0a1.5 1.5 0 1 0-3 0M21 15l-4.5-4.5L9 18", 24.0f, 2.0f},
    {"UndoArrow", "M9 14L4 9l5-5M4 9h10.5a5.5 5.5 0 0 1 0 11H11", 24.0f, 2.0f},
    {"RedoArrow", "M15 14l5-5-5-5M20 9H9.5a5.5 5.5 0 0 0 0 11H13", 24.0f, 2.0f},
    {"DownloadExport", "M12 3v12m0 0l-4-4m4 4l4-4M4 17v2a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2v-2", 24.0f, 2.0f},
    {"EraserTool", "M7 21l-4.3-4.3a2.4 2.4 0 0 1 0-3.4l9.6-9.6a2.4 2.4 0 0 1 3.4 0l5.6 5.6a2.4 2.4 0 0 1 0 3.4L13 21M22 21H7M5 11l9 9", 24.0f, 2.0f},
    {"FillBucket", "M19 11l-8-8l-8.6 8.6a2 2 0 0 0 0 2.8l5.2 5.2a2 2 0 0 0 2.8 0L19 11ZM5 2l5 5M2 13h15M22 20a2 2 0 1 1-4 0c0-1.6 1.7-2.4 2-4c0.3 1.6 2 2.4 2 4Z", 24.0f, 2.0f},
    {"PipetteDropper", "M2 22l1-1h3l9-9M3 21v-3l9-9M15 6l3.4-3.4a2.1 2.1 0 1 1 3 3L18 9M14 5l5 5", 24.0f, 2.0f},
    {"MoveArrows", "M12 3v18M3 12h18M9.5 5.5L12 3l2.5 2.5M9.5 18.5L12 21l2.5-2.5M5.5 9.5L3 12l2.5 2.5M18.5 9.5L21 12l-2.5 2.5", 24.0f, 2.0f},
    {"UploadImport", "M12 15V3M12 3l-4 4M12 3l4 4M4 17v2a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2v-2", 24.0f, 2.0f},
    {"BakeBox", "M12 3l8 4.5v9L12 21l-8-4.5v-9zM12 12l8-4.5M12 12v9M12 12L4 7.5M10.6 7.5a1.4 1.4 0 1 0 2.8 0a1.4 1.4 0 1 0-2.8 0", 24.0f, 2.0f},
    {"VolumeBoxes", "M6 4h8a2 2 0 0 1 2 2v8a2 2 0 0 1-2 2H6a2 2 0 0 1-2-2V6a2 2 0 0 1 2-2zM8 16v2a2 2 0 0 0 2 2h8a2 2 0 0 0 2-2v-8a2 2 0 0 0-2-2h-2", 24.0f, 2.0f},
    {"PinAnchor", "M12 17v5M9 10.8a3 3 0 0 1 1-2.2l1-.9V4h2v3.7l1 .9a3 3 0 0 1 1 2.2V14H9z", 24.0f, 2.0f},

    // 86..127: Unified workspace, transport, viewport, particle, fracture, and UV rail icons
    {"CheckMark",            "M4.5 12.5l5 5L19.5 7",                                                                                               24.0f, 2.2f, false},
    {"CheckCircleFilled",    "M12 2a10 10 0 1 0 0 20a10 10 0 1 0 0-20Z",                                                                           24.0f, 2.2f, true,  "M7.4 12.3l3.1 3.1L16.8 9", false},
    {"PlaySolid",            "M6.5 3.6L20.4 12L6.5 20.4Z",                                                                                         24.0f, 1.0f, true,  {}, true, {}, true, 1.0f, 0.55f, 0.0f},
    {"PauseSolid",           "M5.5 4h4.5v16H5.5ZM14 4h4.5v16H14Z",                                                                                 24.0f, 1.0f, true},
    {"SkipToStartSolid",     "M5.5 4.8h2.6v14.4H5.5ZM18.5 5.2L10 12L18.5 18.8Z",                                                                   24.0f, 1.0f, true},
    {"StepForwardSolid",     "M5.5 5.2L14 12L5.5 18.8ZM15.9 4.8h2.6v14.4h-2.6Z",                                                                   24.0f, 1.0f, true},
    {"ResetArc",             "M4.6 11.4A7.6 7.6 0 1 0 7.4 5.5M4.6 5v6.4H11",                                                                       24.0f, 2.0f, false},
    {"EditPencilSolid",      "M15 2l7 7-12 12-7-7ZM3 15.5l5.5 5.5-6.5 1Z",                                                                         24.0f, 1.0f, true},
    {"SimulateFlaskSolid",   "M6 1h12v3H6ZM9 4h6v4.5H9ZM9 8h6l7 11.5-2 3.5H4l-2-3.5Z",                                                             24.0f, 1.0f, true,  "M11 8.5h2l4 7H7Z", true},
    {"ConstructCubePlus",    "M17 11h3v4h4v3h-4v4h-3v-4h-4v-3h4Z",                                                                                 24.0f, 1.0f, true,  {}, true, "M8 2l8 4v7l-8 4-7.5-4V6Z", true, 0.65f},
    {"FocusReticleSolid",    "M2 2h7v3H5v4H2ZM15 2h7v7h-3V5h-4ZM2 15h3v4h4v3H2ZM19 15h3v7h-7v-3h4ZM12 9a3 3 0 1 0 0 6a3 3 0 1 0 0-6Z",             24.0f, 1.0f, true},
    {"FocusBoundsCorners",   "M3 8V5a2 2 0 0 1 2-2h3M16 3h3a2 2 0 0 1 2 2v3M21 16v3a2 2 0 0 1-2 2h-3M8 21H5a2 2 0 0 1-2-2v-3",                     24.0f, 2.0f, false},
    {"SplitViewportSolid",   "M2 4h20v16H2Z",                                                                                                      24.0f, 1.0f, true,  "M5 7h6v10H5ZM14 7h5v10h-5Z", true},
    {"DiagnosticsBarsSolid", "M2 13h4v9H2ZM10 7h4v15h-4ZM18 2h4v20h-4Z",                                                                           24.0f, 1.0f, true},
    {"ViewportGearSolid",    "m9 1 6 0 1 4 3-1 3 5-3 3 3 3-3 5-3-1-1 4H9l-1-4-3 1-3-5 3-3-3-3 3-5 3 1Z",                                           24.0f, 1.0f, true,  "M12 7.4a4.6 4.6 0 1 0 0 9.2a4.6 4.6 0 1 0 0-9.2Z", true},
    {"FilterSliders",        "M4 6h1.9M10.1 6h9.9M8 3.9a2.1 2.1 0 1 0 0 4.2a2.1 2.1 0 1 0 0-4.2ZM4 12h9.9M18.1 12h1.9M16 9.9a2.1 2.1 0 1 0 0 4.2a2.1 2.1 0 1 0 0-4.2ZM4 18h3.9M12.1 18h7.9M10 15.9a2.1 2.1 0 1 0 0 4.2a2.1 2.1 0 1 0 0-4.2Z", 24.0f, 1.8f, false},
    {"FilterFunnel",         "M22 3H2l8 9.46V19l4 2v-8.54L22 3Z",                                                                                  24.0f, 1.8f, false},
    {"LightningBoltSolid",   "M13 2L4.2 13.4H11.5L10.3 22L19.8 10.6H12.5L13 2Z",                                                                   24.0f, 1.0f, true},
    {"DiceIsometric",        "M21 8a2 2 0 0 0-1-1.73l-7-4a2 2 0 0 0-2 0l-7 4A2 2 0 0 0 3 8v8a2 2 0 0 0 1 1.73l7 4a2 2 0 0 0 2 0l7-4A2 2 0 0 0 21 16ZM3.3 7l8.7 5 8.7-5M12 22V12M12 7.4h.01M7.8 14.4h.01M16.2 14.4h.01", 24.0f, 1.8f, false},
    {"SeedsNetwork",         "M12 4.5a2.1 2.1 0 1 0 0 4.2a2.1 2.1 0 1 0 0-4.2ZM6 15.3a2.1 2.1 0 1 0 0 4.2a2.1 2.1 0 1 0 0-4.2ZM18 15.3a2.1 2.1 0 1 0 0 4.2a2.1 2.1 0 1 0 0-4.2ZM12 8.7L7.3 15.6M12 8.7L16.7 15.6M8.1 17.4H15.9", 24.0f, 1.7f, false},
    {"ArrowNorthEast",       "M7 17L17 7M9 7h8v8",                                                                                                 24.0f, 1.8f, false},
    {"WireCubeIsometric",    "M21 8a2 2 0 0 0-1-1.73l-7-4a2 2 0 0 0-2 0l-7 4A2 2 0 0 0 3 8v8a2 2 0 0 0 1 1.73l7 4a2 2 0 0 0 2 0l7-4A2 2 0 0 0 21 16ZM3.3 7l8.7 5 8.7-5M12 22V12", 24.0f, 1.8f, false},
    {"WireSphere",           "M12 2.5a9.5 9.5 0 1 0 0 19a9.5 9.5 0 1 0 0-19ZM2.5 12h19M12 2.5a14 14 0 0 1 0 19M12 2.5a14 14 0 0 0 0 19",           24.0f, 1.7f, false},
    {"GridAxes",             "M3 8.5h18M3 15.5h18M8.5 3v18M15.5 3v18",                                                                             24.0f, 1.8f, false},
    {"PulseWave",            "M2 12h4l2.4-7 4.2 14 2.4-7H22",                                                                                      24.0f, 1.8f, false},
    {"TriangleWire",         "M12 3.5L21 19.5H3Z",                                                                                                 24.0f, 1.8f, false},
    {"SparkleFourPoint",     "M12 2.5L14.6 9.4L21.5 12L14.6 14.6L12 21.5L9.4 14.6L2.5 12L9.4 9.4Z",                                                24.0f, 1.8f, false},
    {"WindStreamlines",      "M3 7h11.5a2.4 2.4 0 1 0-2.2-3.2M3 12h15a2.4 2.4 0 1 1-2.2 3.2M3 17h9.5a2.1 2.1 0 1 1-1.9 2.9",                       24.0f, 1.8f, false},
    {"MinimizeUnderscore",   "M5 17.5h14",                                                                                                         24.0f, 2.0f, false},
    {"MaximizeSquare",       "M5 5h14v14H5ZM9 5v4H5",                                                                                              24.0f, 1.8f, false},
    {"LogDrawerLines",       "M4 4h16a2 2 0 0 1 2 2v12a2 2 0 0 1-2 2H4a2 2 0 0 1-2-2V6a2 2 0 0 1 2-2ZM7 9.5h10M7 14.5h6",                          24.0f, 1.8f, false},
    {"MinimapColumns",       "M4 4h16a2 2 0 0 1 2 2v12a2 2 0 0 1-2 2H4a2 2 0 0 1-2-2V6a2 2 0 0 1 2-2ZM11 4v16M14 9h5M14 14h4",                     24.0f, 1.8f, false},
    {"CursorPointer",        "M5 3.5L19.5 10.5L13 13L10.5 19.5Z",                                                                                  24.0f, 1.8f, false},
    {"MarqueeDashed",        "M4 7V4h3M10 4h4M17 4h3v3M20 10v4M20 17v3h-3M14 20h-4M7 20H4v-3M4 14v-4",                                             24.0f, 1.8f, false},
    {"CircleSelectBrush",    "M12 3.5a8.5 8.5 0 1 0 0 17a8.5 8.5 0 1 0 0-17ZM12 10.4a1.6 1.6 0 1 0 0 3.2a1.6 1.6 0 1 0 0-3.2Z",                    24.0f, 1.8f, false},
    {"LassoFreehand",        "M12 4.5c4.8 0 8.5 2.7 8.5 6.2s-3.7 6.2-8.5 6.2c-4.8 0-8.5-2.7-8.5-6.2S7.2 4.5 12 4.5ZM12 16.9l-2 3.3M12 16.9l2.4 2.8", 24.0f, 1.8f, false},
    {"RotateOrbit",          "M20 12a8 8 0 1 1-2.4-5.7M20 4.5v5h-5",                                                                               24.0f, 1.8f, false},
    {"ScaleDiagonal",        "M15 4h5v5M9 20H4v-5M20 4L4 20",                                                                                      24.0f, 1.8f, false},
    {"ScissorsCut",          "M6.5 15a2.5 2.5 0 1 0 0 5a2.5 2.5 0 1 0 0-5ZM17.5 15a2.5 2.5 0 1 0 0 5a2.5 2.5 0 1 0 0-5ZM8.2 15.6L17.5 4.5M15.8 15.6L6.5 4.5", 24.0f, 1.8f, false},
    {"SeamKnife",            "M5 19L19 5M9.5 9.5l3 3M12.5 12.5l3 3",                                                                               24.0f, 1.8f, false},
    {"StitchWeld",           "M4 6h5v12H4ZM15 6h5v12h-5ZM9 9.5h6M9 14.5h6",                                                                        24.0f, 1.8f, false},
    {"RelaxWave",            "M12 4.5a7.5 7.5 0 1 0 0 15a7.5 7.5 0 1 0 0-15ZM12 8.2a3.8 3.8 0 1 0 0 7.6a3.8 3.8 0 1 0 0-7.6Z",                     24.0f, 1.8f, false},
    {"OrbitRings",           "M8.8 5.8a6.2 6.2 0 1 0 0 12.4a6.2 6.2 0 1 0 0-12.4ZM15.2 5.8a6.2 6.2 0 1 0 0 12.4a6.2 6.2 0 1 0 0-12.4Z",            24.0f, 1.8f, false},
    {"TargetBullseye",       "M12 4.5a7.5 7.5 0 1 0 0 15a7.5 7.5 0 1 0 0-15ZM12 9.8a2.2 2.2 0 1 0 0 4.4a2.2 2.2 0 1 0 0-4.4Z",                     24.0f, 1.8f, false},
    {"BallTrajectory",       "M12 4.5a7.5 7.5 0 1 0 0 15a7.5 7.5 0 1 0 0-15ZM6.2 9.6L17.8 14.8",                                                   24.0f, 1.8f, false},
    {"CrosshairProbe",       "M12 5.8a6.2 6.2 0 1 0 0 12.4a6.2 6.2 0 1 0 0-12.4ZM3 12h18M12 3v18",                                                 24.0f, 1.7f, false},
    {"TurntableOrbit",       "M12 5a7 7 0 1 0 0 14a7 7 0 1 0 0-14ZM19 10.2a1.8 1.8 0 1 0 0 3.6a1.8 1.8 0 1 0 0-3.6Z",                              24.0f, 1.8f, false},
}};

constexpr std::array<std::string_view, 19> ForceModelSvgPaths = {{
    "M2 6h8.5a2 2 0 1 0-1.9-2.6M2 10h11a1.8 1.8 0 1 1-1.6 2.5M2 13h6.5a1.5 1.5 0 1 1-1.3 2.2",
    "M8 14V3M4.8 6.2L8 3l3.2 3.2",
    "M8 1v2M8 13v2M1 8h2M13 8h2",
    "M2.5 2.5L5.8 5.8M13.5 2.5l-3.3 3.3M2.5 13.5l3.3-3.3M13.5 13.5l-3.3-3.3",
    "M5.5 5.5L2.2 2.2M10.5 5.5l3.3-3.3M5.5 10.5l-3.3 3.3M10.5 10.5l3.3 3.3M2.2 4.5V2.2h2.3M13.8 4.5V2.2h-2.3",
    "M3 13L13 3M4.5 5.5l2.5 2.5M11.5 10.5L9 8",
    "M8 2v2M8 12v2M2 8h2M12 8h2",
    "M8 8a1.2 1.2 0 1 1-1.2-1.2a2.4 2.4 0 0 1 2.4 2.4A3.8 3.8 0 0 1 4 8a5.2 5.2 0 0 1 10.2-1",
    "M12.8 6.2A5.2 5.2 0 1 0 11.5 12M10.2 6.5h3.2V3.3",
    "M5.5 3v10M10.5 3v10M2 8h3.5M14 8h-3.5",
    "M2.5 7.5a2.8 2.8 0 1 1 5.2 1.4a2.8 2.8 0 1 0 5.6 1M11.8 5.2a2 2 0 0 0-3-.2",
    "M2 5.5h8a1.9 1.9 0 1 0-1.7-2.6M2 10h9.5a2.3 2.3 0 1 1-1.9 3.2M10.5 3.5a2 2 0 0 1 3 1.5",
    "M11 4.5l2.5-2M11 2.5h2.5V5M3 5c1.8-1.6 4.5-1.6 6.2 0",
    "M2.5 11.5L5 7l2.2 3.2L10 4.5l2 4l2-2.5",
    "M2.5 9.5c1.5-4.5 3.5-4.5 4.5 0s3 4.5 4.5 0s1.8-4.5 2.5 0M2 4h12M11.8 2.2L14 4l-2.2 1.8",
    "M6.5 4.5C3 4.5 2 8 2 8s1 3.5 4.5 3.5M9.5 4.5C13 4.5 14 8 14 8s-1 3.5-4.5 3.5",
    "M2 5c3-2 6 2 9 0l2-1M2 9c3-2 6 2 9 0l2-1M2 13c3-2 6 2 9 0l2-1M11.5 3.2L14 4l-.8 2.4",
    "M2 12.5C5 12.5 5 3.5 8 3.5s3 9 6 9M12.2 10.5L14 12.5l-2 1.8",
    "M2 12.5h12M4 5c2.5 4.5 5.5 4.5 8 0M8 3.5v6.5M6 8l2 2l2-2",
}};

constexpr std::array<std::string_view, static_cast<std::size_t>(FractureAlgorithmIcon::Count)> FractureAlgorithmSvgPaths = {{
    // Voronoi
    "M12 2L4 7v10l8 5 8-5V7l-8-5Z M12 12L4 7 M12 12l8-5 M12 12v10 M8 14.5l4-2.5 4 2.5",
    // Uniform
    "M3 3h18v18H3Z M9 3v18 M15 3v18 M3 9h18 M3 15h18",
    // Radial
    "M12 3a9 9 0 1 0 0 18a9 9 0 1 0 0-18Z M12 7.5a4.5 4.5 0 1 0 0 9a4.5 4.5 0 1 0 0-9Z M12 3v18 M3 12h18 M5.6 5.6l12.8 12.8 M18.4 5.6L5.6 18.4",
    // PlanarCuts
    "M4 6h16 M4 10h16 M4 14h16 M4 18h16",
    // Brick
    "M3 4h18v16H3Z M3 9.33h18 M3 14.66h18 M10 4v5.33 M15 9.33v5.33 M9 14.66V20",
    // Split
    "M4 4l16 16 M4 14l4-4 2 2 6-6",
}};

struct PrimitiveSvgSpec
{
    std::string_view FillPath;
    std::string_view StrokePath;
    std::string_view AuxPath;
};

constexpr std::array<PrimitiveSvgSpec, static_cast<std::size_t>(PrimitiveShapeIcon::Count)> PrimitiveSpecs = {{
    // Box
    {
        "M128 40L204 84L204 172L128 216L52 172L52 84Z",
        "M128 40L204 84L204 172L128 216L52 172L52 84Z M52 84L128 128L204 84 M128 128L128 216",
        {},
    },
    // Sphere
    {
        "M128 44A84 84 0 1 0 128 212A84 84 0 1 0 128 44Z",
        "M128 44A84 84 0 1 0 128 212A84 84 0 1 0 128 44Z M44 128A84 28 0 0 0 212 128",
        "M128 44A34 84 0 0 1 128 212 M128 44A34 84 0 0 0 128 212",
    },
    // Cylinder
    {
        "M64 74A64 22 0 1 0 192 74A64 22 0 1 0 64 74Z M64 74V182A64 22 0 0 0 192 182V74Z",
        "M64 74A64 22 0 1 0 192 74A64 22 0 1 0 64 74Z M64 74V182A64 22 0 0 0 192 182V74",
        {},
    },
    // Cone
    {
        "M128 44L198 186A70 22 0 0 1 58 186Z",
        "M128 44L198 186A70 22 0 0 1 58 186Z",
        "M128 44L128 186",
    },
    // Capsule
    {
        "M84 92A44 44 0 0 1 172 92V164A44 44 0 0 1 84 164Z",
        "M84 92A44 44 0 0 1 172 92V164A44 44 0 0 1 84 164Z",
        "M84 92A44 14 0 0 0 172 92 M84 164A44 14 0 0 0 172 164",
    },
    // Torus
    {
        "M128 74A88 54 0 1 0 128 182A88 54 0 1 0 128 74Z",
        "M128 74A88 54 0 1 0 128 182A88 54 0 1 0 128 74Z",
        "M128 106A38 22 0 1 0 128 150A38 22 0 1 0 128 106Z",
    },
    // Pipe
    {
        "M58 78A70 24 0 1 0 198 78A70 24 0 1 0 58 78Z M58 78V178A70 24 0 0 0 198 178V78Z",
        "M58 78A70 24 0 1 0 198 78A70 24 0 1 0 58 78Z M58 78V178A70 24 0 0 0 198 178V78",
        "M90 78A38 13 0 1 0 166 78A38 13 0 1 0 90 78Z",
    },
    // Pane
    {
        "M42 74L196 54L214 66L214 182L60 202L42 190Z",
        "M42 74L196 54L214 66L214 182L60 202L42 190Z M60 86L214 66 M60 86L60 202 M42 74L60 86",
        {},
    },
    // HexColumn
    {
        "M96 52L160 52L192 72L192 184L160 204L96 204L64 184L64 72Z",
        "M96 52L160 52L192 72L160 92L96 92L64 72Z M64 72V184L96 204L160 204L192 184V72 M96 92V204 M160 92V204",
        {},
    },
}};

//------------------------------------------------------------------------------------------------------------------------
//                                      PROCESS-WIDE SHARED COMPILED ICON GEOMETRY
//------------------------------------------------------------------------------------------------------------------------

struct CompiledContourSpan
{
    uint32_t FirstVertex = 0u;
    uint32_t VertexCount = 0u;
    bool     Closed      = false;
};

struct CompiledPathRange
{
    uint32_t FirstContour = 0u;
    uint32_t ContourCount = 0u;
    Point2D  BoundsMin    = { 0.0f, 0.0f };
    Point2D  BoundsMax    = { 0.0f, 0.0f };
    bool     BoundsValid  = false;
};

struct SharedIconStorage
{
    std::vector<Point2D>             Vertices {};
    std::vector<CompiledContourSpan> Contours {};

    std::array<CompiledPathRange, static_cast<std::size_t>(NavigationIcon::Count)>        NavigationPaths {};
    std::array<CompiledPathRange, static_cast<std::size_t>(ControlIcon::Count)>           ControlPrimaryPaths {};
    std::array<CompiledPathRange, static_cast<std::size_t>(ControlIcon::Count)>           ControlCutoutPaths {};
    std::array<CompiledPathRange, static_cast<std::size_t>(ControlIcon::Count)>           ControlAccentPaths {};
    std::array<CompiledPathRange, 19u>                                                    ForceModelPaths {};
    std::array<CompiledPathRange, static_cast<std::size_t>(FractureAlgorithmIcon::Count)> FractureAlgorithmPaths {};
    std::array<CompiledPathRange, static_cast<std::size_t>(PrimitiveShapeIcon::Count)>    PrimitiveFillPaths {};
    std::array<CompiledPathRange, static_cast<std::size_t>(PrimitiveShapeIcon::Count)>    PrimitiveStrokePaths {};
    std::array<CompiledPathRange, static_cast<std::size_t>(PrimitiveShapeIcon::Count)>    PrimitiveAuxPaths {};

    CompiledPathRange CapabilityEyeOuterPath {};
    CompiledPathRange StudioCollectionFillPath {};
    CompiledPathRange StudioCollectionStrokePath {};
    CompiledPathRange StudioGearFillPath {};
    CompiledPathRange StudioBakeUpperFillPath {};
    CompiledPathRange StudioBakeLowerFillPath {};
    CompiledPathRange StudioBakeGridStrokePath {};

    std::size_t BuiltinIconCount = 0u;

    CompiledPathRange AppendSvg(std::string_view SvgPath) noexcept
    {
        CompiledPathRange Range {};
        if (SvgPath.empty())
            return Range;

        const std::vector<VectorIconRepository::Contour> RawContours = VectorIconRepository::FlattenSvgPath(SvgPath);
        Range.FirstContour = static_cast<uint32_t>(Contours.size());

        float MinX = std::numeric_limits<float>::max();
        float MinY = std::numeric_limits<float>::max();
        float MaxX = std::numeric_limits<float>::lowest();
        float MaxY = std::numeric_limits<float>::lowest();

        for (const auto& Raw : RawContours)
        {
            if (Raw.Points.empty())
                continue;

            const uint32_t FirstVtx   = static_cast<uint32_t>(Vertices.size());
            bool           ClosedLoop = Raw.Closed;

            for (const Point2D& Pt : Raw.Points)
            {
                if (Vertices.size() > FirstVtx)
                {
                    const Point2D Prev = Vertices.back();
                    const float   Dx   = Pt.X - Prev.X;
                    const float   Dy   = Pt.Y - Prev.Y;
                    if (Dx * Dx + Dy * Dy < 1.0e-6f)
                        continue;
                }
                Vertices.push_back(Pt);
                MinX = std::min(MinX, Pt.X);
                MinY = std::min(MinY, Pt.Y);
                MaxX = std::max(MaxX, Pt.X);
                MaxY = std::max(MaxY, Pt.Y);
            }

            uint32_t CountVtx = static_cast<uint32_t>(Vertices.size()) - FirstVtx;
            if (CountVtx >= 3u)
            {
                const Point2D Front = Vertices[FirstVtx];
                const Point2D Back  = Vertices.back();
                const float   Dx    = Back.X - Front.X;
                const float   Dy    = Back.Y - Front.Y;
                if (Dx * Dx + Dy * Dy < 1.0e-4f)
                {
                    Vertices.pop_back();
                    --CountVtx;
                    ClosedLoop = true;
                }
            }

            if (CountVtx > 0u)
            {
                Contours.push_back({ FirstVtx, CountVtx, ClosedLoop });
                ++Range.ContourCount;
            }
        }

        if (Range.ContourCount > 0u && MinX <= MaxX && MinY <= MaxY)
        {
            Range.BoundsMin   = { MinX, MinY };
            Range.BoundsMax   = { MaxX, MaxY };
            Range.BoundsValid = true;
        }
        return Range;
    }

    SharedIconStorage()
    {
        Vertices.reserve(8192u);
        Contours.reserve(1024u);

        for (std::size_t I = 0u; I < NavigationEntries.size(); ++I)
            NavigationPaths[I] = AppendSvg(NavigationEntries[I].SvgPath);

        for (std::size_t I = 0u; I < ControlEntries.size(); ++I)
        {
            ControlPrimaryPaths[I] = AppendSvg(ControlEntries[I].SvgPath);
            ControlCutoutPaths[I]  = AppendSvg(ControlEntries[I].CutoutSvgPath);
            ControlAccentPaths[I]  = AppendSvg(ControlEntries[I].AccentSvgPath);
        }

        for (std::size_t I = 0u; I < ForceModelSvgPaths.size(); ++I)
            ForceModelPaths[I] = AppendSvg(ForceModelSvgPaths[I]);

        for (std::size_t I = 0u; I < FractureAlgorithmSvgPaths.size(); ++I)
            FractureAlgorithmPaths[I] = AppendSvg(FractureAlgorithmSvgPaths[I]);

        for (std::size_t I = 0u; I < PrimitiveSpecs.size(); ++I)
        {
            PrimitiveFillPaths[I]   = AppendSvg(PrimitiveSpecs[I].FillPath);
            PrimitiveStrokePaths[I] = AppendSvg(PrimitiveSpecs[I].StrokePath);
            PrimitiveAuxPaths[I]    = AppendSvg(PrimitiveSpecs[I].AuxPath);
        }

        CapabilityEyeOuterPath = AppendSvg(
            "M1 12C3.7 6.8 7.4 4 12 4C16.6 4 20.3 6.8 23 12C20.3 17.2 16.6 20 12 20C7.4 20 3.7 17.2 1 12Z");

        StudioCollectionFillPath = AppendSvg(
            "M128 32L178 60L178 118L128 146L78 118L78 60Z "
            "M80 116L128 144L128 202L80 228L32 202L32 144Z "
            "M176 116L224 144L224 202L176 228L128 202L128 144Z");
        StudioCollectionStrokePath = AppendSvg(
            "M128 32L178 60L178 118L128 146L78 118L78 60Z M78 60L128 88L178 60 M128 88L128 146 "
            "M80 116L128 144L128 202L80 228L32 202L32 144Z M32 144L80 172L128 144 M80 172L80 228 "
            "M176 116L224 144L224 202L176 228L128 202L128 144Z M128 144L176 172L224 144 M176 172L176 228");

        StudioGearFillPath = AppendSvg(
            "M110 22L146 22L152 50L174 59L198 44L224 70L209 94L218 116L246 122L246 158L218 164L209 186"
            "L224 210L198 236L174 221L152 230L146 252L110 252L104 230L82 221L58 236L32 210L47 186L38 164"
            "L10 158L10 122L38 116L47 94L32 70L58 44L82 59L104 50Z");

        StudioBakeUpperFillPath  = AppendSvg("M128 30L212 78L128 126L44 78Z");
        StudioBakeLowerFillPath  = AppendSvg("M44 78L128 126L212 78L212 178L128 226L44 178Z");
        StudioBakeGridStrokePath = AppendSvg(
            "M128 30L212 78L212 178L128 226L44 178L44 78Z "
            "M44 78L128 126L212 78 M128 126L128 226 "
            "M72 62L156 110 M100 46L184 94 M184 62L100 110 M156 46L72 94");

        BuiltinIconCount =
            NavigationEntries.size() +
            ControlEntries.size() +
            ForceModelSvgPaths.size() +
            FractureAlgorithmSvgPaths.size() +
            PrimitiveSpecs.size() +
            static_cast<std::size_t>(CapabilityIcon::Count) +
            static_cast<std::size_t>(ComponentModeIcon::Count) +
            static_cast<std::size_t>(StudioIcon::Count);

        Vertices.shrink_to_fit();
        Contours.shrink_to_fit();
    }
};

[[nodiscard]] const SharedIconStorage& QuerySharedStorage() noexcept
{
    static const SharedIconStorage Instance{};
    return Instance;
}

//------------------------------------------------------------------------------------------------------------------------
//                                      DYNAMIC SHARED CACHE FOR EXTERNAL SVG PATHS
//------------------------------------------------------------------------------------------------------------------------

struct DynamicCompiledPath
{
    std::vector<Point2D>             Vertices {};
    std::vector<CompiledContourSpan> Contours {};
    Point2D                          BoundsMin   = { 0.0f, 0.0f };
    Point2D                          BoundsMax   = { 0.0f, 0.0f };
    bool                             BoundsValid = false;
};

struct DynamicPathCache
{
    std::mutex                                          Guard {};
    std::unordered_map<std::string, DynamicCompiledPath> Entries {};
    std::size_t                                         Hits   = 0u;
    std::size_t                                         Misses = 0u;
};

[[nodiscard]] DynamicPathCache& QueryDynamicCache() noexcept
{
    static DynamicPathCache Instance{};
    return Instance;
}

[[nodiscard]] const DynamicCompiledPath& ResolveDynamicPath(std::string_view SvgPath) noexcept
{
    DynamicPathCache&             Cache = QueryDynamicCache();
    std::lock_guard<std::mutex>   Lock(Cache.Guard);
    const std::string             Key(SvgPath);
    auto                          Found = Cache.Entries.find(Key);
    if (Found != Cache.Entries.end())
    {
        ++Cache.Hits;
        return Found->second;
    }

    ++Cache.Misses;
    DynamicCompiledPath Compiled {};
    const std::vector<VectorIconRepository::Contour> RawContours = VectorIconRepository::FlattenSvgPath(SvgPath);

    float MinX = std::numeric_limits<float>::max();
    float MinY = std::numeric_limits<float>::max();
    float MaxX = std::numeric_limits<float>::lowest();
    float MaxY = std::numeric_limits<float>::lowest();

    for (const auto& Raw : RawContours)
    {
        if (Raw.Points.empty())
            continue;
        const uint32_t FirstVtx   = static_cast<uint32_t>(Compiled.Vertices.size());
        bool           ClosedLoop = Raw.Closed;
        for (const Point2D& Pt : Raw.Points)
        {
            if (Compiled.Vertices.size() > FirstVtx)
            {
                const Point2D Prev = Compiled.Vertices.back();
                const float   Dx   = Pt.X - Prev.X;
                const float   Dy   = Pt.Y - Prev.Y;
                if (Dx * Dx + Dy * Dy < 1.0e-6f)
                    continue;
            }
            Compiled.Vertices.push_back(Pt);
            MinX = std::min(MinX, Pt.X);
            MinY = std::min(MinY, Pt.Y);
            MaxX = std::max(MaxX, Pt.X);
            MaxY = std::max(MaxY, Pt.Y);
        }
        uint32_t CountVtx = static_cast<uint32_t>(Compiled.Vertices.size()) - FirstVtx;
        if (CountVtx >= 3u)
        {
            const Point2D Front = Compiled.Vertices[FirstVtx];
            const Point2D Back  = Compiled.Vertices.back();
            const float   Dx    = Back.X - Front.X;
            const float   Dy    = Back.Y - Front.Y;
            if (Dx * Dx + Dy * Dy < 1.0e-4f)
            {
                Compiled.Vertices.pop_back();
                --CountVtx;
                ClosedLoop = true;
            }
        }
        if (CountVtx > 0u)
            Compiled.Contours.push_back({ FirstVtx, CountVtx, ClosedLoop });
    }

    if (!Compiled.Contours.empty() && MinX <= MaxX && MinY <= MaxY)
    {
        Compiled.BoundsMin   = { MinX, MinY };
        Compiled.BoundsMax   = { MaxX, MaxY };
        Compiled.BoundsValid = true;
    }

    auto [Inserted, Ok] = Cache.Entries.emplace(Key, std::move(Compiled));
    return Inserted->second;
}

//------------------------------------------------------------------------------------------------------------------------
//                                  ZERO-ALLOCATION UNIFIED RASTERIZATION ENGINE
//------------------------------------------------------------------------------------------------------------------------

void RasterizeCompiledSpans(
    PixelSpace&                Surface,
    const Point2D*             VertexBuffer,
    const CompiledContourSpan* ContourBuffer,
    uint32_t                   FirstContour,
    uint32_t                   ContourCount,
    Point2D                    BoundsMin,
    Point2D                    BoundsMax,
    bool                       BoundsValid,
    const VectorIconPlacement& Placement,
    float                      ViewBox,
    bool                       Filled,
    float                      OpticalOffsetX = 0.0f,
    float                      OpticalOffsetY = 0.0f) noexcept
{
    if (ContourCount == 0u || Placement.Size <= 0.0f || ViewBox <= 0.0f || Placement.Ink.Alpha == 0u)
        return;

    const float Scale = Placement.Size / ViewBox;
    float       OriginX = Placement.X + OpticalOffsetX * Scale;
    float       OriginY = Placement.Y + OpticalOffsetY * Scale;
    if (Placement.Alignment == VectorIconAlignment::GeometricCentered && BoundsValid)
    {
        const float InkCenterX = (BoundsMin.X + BoundsMax.X) * 0.5f;
        const float InkCenterY = (BoundsMin.Y + BoundsMax.Y) * 0.5f;
        const float TargetCX   = Placement.X + Placement.Size * 0.5f;
        const float TargetCY   = Placement.Y + Placement.Size * 0.5f;
        OriginX = TargetCX - InkCenterX * Scale + OpticalOffsetX * Scale;
        OriginY = TargetCY - InkCenterY * Scale + OpticalOffsetY * Scale;
    }

    const float Thickness = Placement.AbsoluteStroke
        ? ((Placement.StrokeWidth > 0.0f) ? Placement.StrokeWidth : std::clamp(Placement.Size * 0.085f, 1.2f, 2.25f))
        : std::max(1.0f, Placement.StrokeWidth * Scale);

    Point2D                                StackPts[StackScratchN];
    thread_local std::vector<Point2D>      OverflowPts;

    for (uint32_t C = 0u; C < ContourCount; ++C)
    {
        const CompiledContourSpan& Span = ContourBuffer[FirstContour + C];
        if (Span.VertexCount == 0u)
            continue;

        Point2D* Dest = nullptr;
        if (Span.VertexCount <= StackScratchN)
        {
            Dest = StackPts;
        }
        else
        {
            OverflowPts.resize(Span.VertexCount);
            Dest = OverflowPts.data();
        }

        const Point2D* Src = VertexBuffer + Span.FirstVertex;
        for (uint32_t V = 0u; V < Span.VertexCount; ++V)
        {
            Dest[V] = { OriginX + Src[V].X * Scale, OriginY + Src[V].Y * Scale };
        }

        const std::span<const Point2D> PolySpan(Dest, Span.VertexCount);

        if (Filled)
        {
            if (Span.Closed && Span.VertexCount >= 3u)
                Surface.FillPolygon(PolySpan, Placement.Ink);
            continue;
        }

        if (Span.VertexCount == 1u)
        {
            Surface.FillCircle(Dest[0], std::max(0.75f, 0.5f * Thickness), Placement.Ink);
            continue;
        }

        if (Span.VertexCount == 2u)
        {
            const float Dx = Dest[1].X - Dest[0].X;
            const float Dy = Dest[1].Y - Dest[0].Y;
            if (Dx * Dx + Dy * Dy < 0.04f)
            {
                Surface.FillCircle(Dest[0], std::max(0.75f, 0.5f * Thickness), Placement.Ink);
                continue;
            }
        }

        Surface.StrokePolyline(PolySpan, Placement.Ink, Thickness, Span.Closed);
        if (!Span.Closed && Thickness >= 1.15f)
        {
            const float CapR = 0.5f * Thickness;
            Surface.FillCircle(Dest[0], CapR, Placement.Ink);
            Surface.FillCircle(Dest[Span.VertexCount - 1u], CapR, Placement.Ink);
        }
    }
}

void RasterizeSharedRange(
    PixelSpace&                Surface,
    const SharedIconStorage&   Storage,
    const CompiledPathRange&   Range,
    const VectorIconPlacement& Placement,
    float                      ViewBox,
    bool                       Filled,
    float                      OpticalOffsetX = 0.0f,
    float                      OpticalOffsetY = 0.0f) noexcept
{
    RasterizeCompiledSpans(
        Surface,
        Storage.Vertices.data(),
        Storage.Contours.data(),
        Range.FirstContour,
        Range.ContourCount,
        Range.BoundsMin,
        Range.BoundsMax,
        Range.BoundsValid,
        Placement,
        ViewBox,
        Filled,
        OpticalOffsetX,
        OpticalOffsetY);
}

void ConstructTreeBranchMark(
    PixelSpace& Surface,
    Point2D     Origin,
    float       ScaleFactor,
    ColorRgba   StrokeInk,
    float       StrokeWidth) noexcept
{
    const auto SX = [&](float X) noexcept -> float { return Origin.X + X * ScaleFactor; };
    const auto SY = [&](float Y) noexcept -> float { return Origin.Y + Y * ScaleFactor; };
    const float TopRx = std::max(0.6f, 1.5f * ScaleFactor);

    Surface.StrokeRectangle({ SX(11.0f), SY(3.0f), 10.0f * ScaleFactor, 7.0f * ScaleFactor }, StrokeInk, TopRx, StrokeWidth);
    Surface.StrokeLine({ SX(16.0f), SY(10.0f) }, { SX(16.0f), SY(17.0f) }, StrokeInk, StrokeWidth);
    const std::array<Point2D, 4> Fork = {{
        { SX(6.0f),  SY(22.0f) },
        { SX(6.0f),  SY(17.0f) },
        { SX(26.0f), SY(17.0f) },
        { SX(26.0f), SY(22.0f) },
    }};
    Surface.StrokePolyline(Fork, StrokeInk, StrokeWidth, false);
    Surface.StrokeRectangle({ SX(2.0f),  SY(22.0f), 9.0f * ScaleFactor, 7.0f * ScaleFactor }, StrokeInk, TopRx, StrokeWidth);
    Surface.StrokeRectangle({ SX(21.0f), SY(22.0f), 9.0f * ScaleFactor, 7.0f * ScaleFactor }, StrokeInk, TopRx, StrokeWidth);
}

} // namespace

//------------------------------------------------------------------------------------------------------------------------
//                                              UNIFIED CENTERING & QUERIES
//------------------------------------------------------------------------------------------------------------------------

Rectangle2D VectorIconRepository::ComputeFittedSquare(Rectangle2D Bounds, float ExplicitSize) noexcept
{
    const float Side = (ExplicitSize > 0.0f)
        ? ExplicitSize
        : std::max(0.0f, std::min(Bounds.Width, Bounds.Height));
    return {
        Bounds.X + (Bounds.Width - Side) * 0.5f,
        Bounds.Y + (Bounds.Height - Side) * 0.5f,
        Side,
        Side,
    };
}

const VectorIconEntry& VectorIconRepository::QueryNavigation(NavigationIcon Icon) noexcept
{
    const std::size_t Index = static_cast<std::size_t>(Icon);
    return Index < NavigationEntries.size() ? NavigationEntries[Index] : NavigationEntries[0];
}

const VectorIconEntry& VectorIconRepository::QueryControl(ControlIcon Icon) noexcept
{
    const std::size_t Index = static_cast<std::size_t>(Icon);
    return Index < ControlEntries.size() ? ControlEntries[Index] : ControlEntries[0];
}

bool VectorIconRepository::QueryNavigationBounds(NavigationIcon Icon, Point2D& OutMin, Point2D& OutMax) noexcept
{
    const std::size_t        Index   = static_cast<std::size_t>(Icon);
    const SharedIconStorage& Storage = QuerySharedStorage();
    if (Index >= Storage.NavigationPaths.size() || !Storage.NavigationPaths[Index].BoundsValid)
        return false;
    OutMin = Storage.NavigationPaths[Index].BoundsMin;
    OutMax = Storage.NavigationPaths[Index].BoundsMax;
    return true;
}

bool VectorIconRepository::QueryControlBounds(ControlIcon Icon, Point2D& OutMin, Point2D& OutMax) noexcept
{
    const std::size_t        Index   = static_cast<std::size_t>(Icon);
    const SharedIconStorage& Storage = QuerySharedStorage();
    if (Index >= Storage.ControlPrimaryPaths.size() || !Storage.ControlPrimaryPaths[Index].BoundsValid)
        return false;
    OutMin = Storage.ControlPrimaryPaths[Index].BoundsMin;
    OutMax = Storage.ControlPrimaryPaths[Index].BoundsMax;
    return true;
}

VectorIconStorageMetrics VectorIconRepository::QuerySharedStorageMetrics() noexcept
{
    const SharedIconStorage& Storage = QuerySharedStorage();
    DynamicPathCache&        Dynamic = QueryDynamicCache();
    std::lock_guard<std::mutex> Lock(Dynamic.Guard);

    VectorIconStorageMetrics Metrics {};
    Metrics.BuiltinIconCount   = Storage.BuiltinIconCount;
    Metrics.CachedContourCount = Storage.Contours.size();
    Metrics.CachedVertexCount  = Storage.Vertices.size();
    Metrics.SharedMemoryBytes  = Storage.Vertices.size() * sizeof(Point2D)
                               + Storage.Contours.size() * sizeof(CompiledContourSpan);
    Metrics.DynamicCachedPaths = Dynamic.Entries.size();
    Metrics.DynamicCacheHits   = Dynamic.Hits;
    Metrics.DynamicCacheMisses = Dynamic.Misses;
    return Metrics;
}

//------------------------------------------------------------------------------------------------------------------------
//                                               SVG FLATTENING & CACHED PATHS
//------------------------------------------------------------------------------------------------------------------------

std::vector<VectorIconRepository::Contour> VectorIconRepository::FlattenSvgPath(std::string_view SvgPath) noexcept
{
    std::vector<Contour> Contours;
    SvgPathScanner       Scanner{ SvgPath };

    Point2D Current{};
    Point2D SubpathStart{};
    Point2D LastControl{};
    char    Command         = 0;
    char    PreviousCommand = 0;

    auto BeginContour = [&](Point2D Start) {
        Contours.push_back({});
        Contours.back().Points.push_back(Start);
        SubpathStart = Start;
    };
    auto EnsureContour = [&]() {
        if (Contours.empty() || Contours.back().Closed)
            BeginContour(Current);
    };

    while (!Scanner.AtEnd())
    {
        char Next = 0;
        if (Scanner.PeekCommand(Next))
        {
            Command = Next;
            ++Scanner.Cursor;
        }
        else if (Command == 0)
        {
            break;
        }
        else if (Command == 'M')
        {
            Command = 'L';
        }
        else if (Command == 'm')
        {
            Command = 'l';
        }

        const bool    Relative = std::islower(static_cast<unsigned char>(Command)) != 0;
        const Point2D OriginPt = Relative ? Current : Point2D{};

        switch (std::toupper(static_cast<unsigned char>(Command)))
        {
        case 'M':
        {
            float X = 0.0f, Y = 0.0f;
            if (!Scanner.ReadScalar(X) || !Scanner.ReadScalar(Y))
                return Contours;
            Current = { OriginPt.X + X, OriginPt.Y + Y };
            BeginContour(Current);
            break;
        }
        case 'L':
        {
            float X = 0.0f, Y = 0.0f;
            if (!Scanner.ReadScalar(X) || !Scanner.ReadScalar(Y))
                return Contours;
            EnsureContour();
            Current = { OriginPt.X + X, OriginPt.Y + Y };
            Contours.back().Points.push_back(Current);
            break;
        }
        case 'H':
        {
            float X = 0.0f;
            if (!Scanner.ReadScalar(X))
                return Contours;
            EnsureContour();
            Current.X = OriginPt.X + X;
            Contours.back().Points.push_back(Current);
            break;
        }
        case 'V':
        {
            float Y = 0.0f;
            if (!Scanner.ReadScalar(Y))
                return Contours;
            EnsureContour();
            Current.Y = OriginPt.Y + Y;
            Contours.back().Points.push_back(Current);
            break;
        }
        case 'C':
        {
            float X1 = 0.0f, Y1 = 0.0f, X2 = 0.0f, Y2 = 0.0f, X = 0.0f, Y = 0.0f;
            if (!Scanner.ReadScalar(X1) || !Scanner.ReadScalar(Y1) || !Scanner.ReadScalar(X2) ||
                !Scanner.ReadScalar(Y2) || !Scanner.ReadScalar(X) || !Scanner.ReadScalar(Y))
                return Contours;
            EnsureContour();
            const Point2D P1{ OriginPt.X + X1, OriginPt.Y + Y1 };
            const Point2D P2{ OriginPt.X + X2, OriginPt.Y + Y2 };
            const Point2D P3{ OriginPt.X + X, OriginPt.Y + Y };
            AppendCubicBezier(Contours.back().Points, Current, P1, P2, P3, CurveSegments);
            LastControl = P2;
            Current     = P3;
            break;
        }
        case 'S':
        {
            float X2 = 0.0f, Y2 = 0.0f, X = 0.0f, Y = 0.0f;
            if (!Scanner.ReadScalar(X2) || !Scanner.ReadScalar(Y2) || !Scanner.ReadScalar(X) || !Scanner.ReadScalar(Y))
                return Contours;
            EnsureContour();
            const char    PrevUpper = static_cast<char>(std::toupper(static_cast<unsigned char>(PreviousCommand)));
            const Point2D P1        = (PrevUpper == 'C' || PrevUpper == 'S')
                                          ? Point2D{ 2.0f * Current.X - LastControl.X, 2.0f * Current.Y - LastControl.Y }
                                          : Current;
            const Point2D P2{ OriginPt.X + X2, OriginPt.Y + Y2 };
            const Point2D P3{ OriginPt.X + X, OriginPt.Y + Y };
            AppendCubicBezier(Contours.back().Points, Current, P1, P2, P3, CurveSegments);
            LastControl = P2;
            Current     = P3;
            break;
        }
        case 'Q':
        {
            float X1 = 0.0f, Y1 = 0.0f, X = 0.0f, Y = 0.0f;
            if (!Scanner.ReadScalar(X1) || !Scanner.ReadScalar(Y1) || !Scanner.ReadScalar(X) || !Scanner.ReadScalar(Y))
                return Contours;
            EnsureContour();
            const Point2D P1{ OriginPt.X + X1, OriginPt.Y + Y1 };
            const Point2D P2{ OriginPt.X + X, OriginPt.Y + Y };
            AppendQuadraticBezier(Contours.back().Points, Current, P1, P2, CurveSegments);
            LastControl = P1;
            Current     = P2;
            break;
        }
        case 'T':
        {
            float X = 0.0f, Y = 0.0f;
            if (!Scanner.ReadScalar(X) || !Scanner.ReadScalar(Y))
                return Contours;
            EnsureContour();
            const char    PrevUpper = static_cast<char>(std::toupper(static_cast<unsigned char>(PreviousCommand)));
            const Point2D P1        = (PrevUpper == 'Q' || PrevUpper == 'T')
                                          ? Point2D{ 2.0f * Current.X - LastControl.X, 2.0f * Current.Y - LastControl.Y }
                                          : Current;
            const Point2D P2{ OriginPt.X + X, OriginPt.Y + Y };
            AppendQuadraticBezier(Contours.back().Points, Current, P1, P2, CurveSegments);
            LastControl = P1;
            Current     = P2;
            break;
        }
        case 'A':
        {
            float Rx = 0.0f, Ry = 0.0f, Rot = 0.0f, X = 0.0f, Y = 0.0f;
            bool  Large = false, Sweep = false;
            if (!Scanner.ReadScalar(Rx) || !Scanner.ReadScalar(Ry) || !Scanner.ReadScalar(Rot) ||
                !Scanner.ReadArcBit(Large) || !Scanner.ReadArcBit(Sweep) || !Scanner.ReadScalar(X) || !Scanner.ReadScalar(Y))
                return Contours;
            EnsureContour();
            const Point2D EndPt{ OriginPt.X + X, OriginPt.Y + Y };
            AppendEllipticalArc(Contours.back().Points, Current, Rx, Ry, Rot, Large, Sweep, EndPt, ArcStepRadians);
            Current = EndPt;
            break;
        }
        case 'Z':
        {
            if (!Contours.empty())
                Contours.back().Closed = true;
            Current = SubpathStart;
            break;
        }
        default:
            return Contours;
        }
        PreviousCommand = Command;
    }
    return Contours;
}

void VectorIconRepository::StrokeSvgPath(
    PixelSpace&                Surface,
    std::string_view           SvgPath,
    const VectorIconPlacement& Placement,
    float                      ViewBox) noexcept
{
    if (SvgPath.empty())
        return;
    const DynamicCompiledPath& Path = ResolveDynamicPath(SvgPath);
    RasterizeCompiledSpans(
        Surface,
        Path.Vertices.data(),
        Path.Contours.data(),
        0u,
        static_cast<uint32_t>(Path.Contours.size()),
        Path.BoundsMin,
        Path.BoundsMax,
        Path.BoundsValid,
        Placement,
        ViewBox,
        false);
}

void VectorIconRepository::FillSvgPath(
    PixelSpace&                Surface,
    std::string_view           SvgPath,
    const VectorIconPlacement& Placement,
    float                      ViewBox) noexcept
{
    if (SvgPath.empty())
        return;
    const DynamicCompiledPath& Path = ResolveDynamicPath(SvgPath);
    RasterizeCompiledSpans(
        Surface,
        Path.Vertices.data(),
        Path.Contours.data(),
        0u,
        static_cast<uint32_t>(Path.Contours.size()),
        Path.BoundsMin,
        Path.BoundsMax,
        Path.BoundsValid,
        Placement,
        ViewBox,
        true);
}

//------------------------------------------------------------------------------------------------------------------------
//                                           UNIFIED NAVIGATION & CONTROL ICONS
//------------------------------------------------------------------------------------------------------------------------

void VectorIconRepository::ConstructNavigation(
    PixelSpace&                Surface,
    NavigationIcon             Icon,
    const VectorIconPlacement& Placement) noexcept
{
    const SharedIconStorage& Storage = QuerySharedStorage();
    const std::size_t        Index   = static_cast<std::size_t>(Icon);
    const std::size_t        SafeIdx = Index < NavigationEntries.size() ? Index : 0u;
    const VectorIconEntry&   Entry   = NavigationEntries[SafeIdx];

    RasterizeSharedRange(
        Surface,
        Storage,
        Storage.NavigationPaths[SafeIdx],
        Placement,
        Entry.ViewBox,
        Entry.Filled,
        Entry.OpticalOffsetX,
        Entry.OpticalOffsetY);
}

void VectorIconRepository::ConstructNavigationInBounds(
    PixelSpace&         Surface,
    NavigationIcon      Icon,
    Rectangle2D         Bounds,
    ColorRgba           Ink,
    float               ExplicitSize,
    float               ScreenStrokePx,
    VectorIconAlignment Alignment) noexcept
{
    const Rectangle2D Square = ComputeFittedSquare(Bounds, ExplicitSize);
    VectorIconPlacement Placement {};
    Placement.X              = Square.X;
    Placement.Y              = Square.Y;
    Placement.Size           = Square.Width;
    Placement.Ink            = Ink;
    Placement.StrokeWidth    = ScreenStrokePx;
    Placement.Alignment      = Alignment;
    Placement.AbsoluteStroke = true;
    ConstructNavigation(Surface, Icon, Placement);
}

void VectorIconRepository::ConstructControl(
    PixelSpace&                Surface,
    ControlIcon                Icon,
    const VectorIconPlacement& Placement) noexcept
{
    const SharedIconStorage& Storage = QuerySharedStorage();
    const std::size_t        Index   = static_cast<std::size_t>(Icon);
    const std::size_t        SafeIdx = Index < ControlEntries.size() ? Index : 0u;
    const VectorIconEntry&   Entry   = ControlEntries[SafeIdx];

    if (Storage.ControlAccentPaths[SafeIdx].ContourCount > 0u)
    {
        VectorIconPlacement AccentPlace = Placement;
        AccentPlace.Ink                 = FadeColorAlpha(Placement.Ink, Entry.AccentOpacity);
        RasterizeSharedRange(
            Surface,
            Storage,
            Storage.ControlAccentPaths[SafeIdx],
            AccentPlace,
            Entry.ViewBox,
            Entry.AccentFilled,
            Entry.OpticalOffsetX,
            Entry.OpticalOffsetY);
    }

    RasterizeSharedRange(
        Surface,
        Storage,
        Storage.ControlPrimaryPaths[SafeIdx],
        Placement,
        Entry.ViewBox,
        Entry.Filled,
        Entry.OpticalOffsetX,
        Entry.OpticalOffsetY);

    if (Storage.ControlCutoutPaths[SafeIdx].ContourCount > 0u)
    {
        VectorIconPlacement CutoutPlace = Placement;
        CutoutPlace.Ink                 = Placement.InteriorCutout;
        if (!Entry.CutoutFilled && CutoutPlace.StrokeWidth <= 0.0f)
        {
            CutoutPlace.AbsoluteStroke = true;
            CutoutPlace.StrokeWidth    = std::clamp(Placement.Size * 0.14f, 1.45f, 2.4f);
        }
        RasterizeSharedRange(
            Surface,
            Storage,
            Storage.ControlCutoutPaths[SafeIdx],
            CutoutPlace,
            Entry.ViewBox,
            Entry.CutoutFilled,
            Entry.OpticalOffsetX,
            Entry.OpticalOffsetY);
    }
}

void VectorIconRepository::ConstructControlInBounds(
    PixelSpace&         Surface,
    ControlIcon         Icon,
    Rectangle2D         Bounds,
    ColorRgba           Ink,
    float               ExplicitSize,
    float               ScreenStrokePx,
    VectorIconAlignment Alignment,
    ColorRgba           InteriorCutout) noexcept
{
    const Rectangle2D Square = ComputeFittedSquare(Bounds, ExplicitSize);
    VectorIconPlacement Placement {};
    Placement.X              = Square.X;
    Placement.Y              = Square.Y;
    Placement.Size           = Square.Width;
    Placement.Ink            = Ink;
    Placement.StrokeWidth    = ScreenStrokePx;
    Placement.InteriorCutout = InteriorCutout;
    Placement.Alignment      = Alignment;
    Placement.AbsoluteStroke = true;
    ConstructControl(Surface, Icon, Placement);
}

void VectorIconRepository::ConstructControlCentered(
    PixelSpace&         Surface,
    ControlIcon         Icon,
    Point2D             Center,
    float               Size,
    ColorRgba           Ink,
    float               ScreenStrokePx,
    VectorIconAlignment Alignment,
    ColorRgba           InteriorCutout) noexcept
{
    const Rectangle2D Bounds{ Center.X - Size * 0.5f, Center.Y - Size * 0.5f, Size, Size };
    ConstructControlInBounds(Surface, Icon, Bounds, Ink, Size, ScreenStrokePx, Alignment, InteriorCutout);
}

//------------------------------------------------------------------------------------------------------------------------
//                                           UNIFIED DOMAIN & STUDIO ICONS
//------------------------------------------------------------------------------------------------------------------------

void VectorIconRepository::ConstructPrimitiveInBounds(
    PixelSpace&        Surface,
    PrimitiveShapeIcon Shape,
    Rectangle2D        Bounds,
    float              Opacity) noexcept
{
    const Rectangle2D Square = ComputeFittedSquare(Bounds);
    if (Square.Width <= 2.0f || Opacity <= 0.0f)
        return;

    const SharedIconStorage& Storage = QuerySharedStorage();
    const std::size_t        Idx     = static_cast<std::size_t>(Shape);
    const std::size_t        SafeIdx = Idx < PrimitiveSpecs.size() ? Idx : 0u;

    const ColorRgba BlueFill   = FadeColorAlpha({ 23u, 75u, 136u, 255u }, Opacity);
    const ColorRgba BlueStroke = FadeColorAlpha({ 56u, 156u, 255u, 255u }, Opacity);
    const ColorRgba SoftStroke = FadeColorAlpha({ 56u, 156u, 255u, 195u }, Opacity);
    const ColorRgba HoleDark   = FadeColorAlpha({ 18u, 18u, 20u, 255u }, Opacity);
    const float     MainW      = std::max(1.25f, Square.Width * 0.068f);
    const float     AuxW       = std::max(1.0f, Square.Width * 0.048f);

    VectorIconPlacement FillPlace {};
    FillPlace.X    = Square.X;
    FillPlace.Y    = Square.Y;
    FillPlace.Size = Square.Width;
    FillPlace.Ink  = BlueFill;

    VectorIconPlacement StrokePlace {};
    StrokePlace.X              = Square.X;
    StrokePlace.Y              = Square.Y;
    StrokePlace.Size           = Square.Width;
    StrokePlace.Ink            = BlueStroke;
    StrokePlace.StrokeWidth    = MainW;
    StrokePlace.AbsoluteStroke = true;

    RasterizeSharedRange(Surface, Storage, Storage.PrimitiveFillPaths[SafeIdx], FillPlace, 256.0f, true);
    RasterizeSharedRange(Surface, Storage, Storage.PrimitiveStrokePaths[SafeIdx], StrokePlace, 256.0f, false);

    if (Storage.PrimitiveAuxPaths[SafeIdx].ContourCount > 0u)
    {
        if (Shape == PrimitiveShapeIcon::Torus || Shape == PrimitiveShapeIcon::Pipe)
        {
            VectorIconPlacement HolePlace = FillPlace;
            HolePlace.Ink                 = HoleDark;
            RasterizeSharedRange(Surface, Storage, Storage.PrimitiveAuxPaths[SafeIdx], HolePlace, 256.0f, true);

            VectorIconPlacement RingStroke = StrokePlace;
            RingStroke.StrokeWidth         = std::max(1.1f, MainW * 0.85f);
            RasterizeSharedRange(Surface, Storage, Storage.PrimitiveAuxPaths[SafeIdx], RingStroke, 256.0f, false);
        }
        else
        {
            VectorIconPlacement AuxPlace = StrokePlace;
            AuxPlace.Ink                 = SoftStroke;
            AuxPlace.StrokeWidth         = AuxW;
            RasterizeSharedRange(Surface, Storage, Storage.PrimitiveAuxPaths[SafeIdx], AuxPlace, 256.0f, false);
        }
    }
}

ColorRgba VectorIconRepository::QueryForceModelTint(uint32_t ForceModelOrdinal) noexcept
{
    static constexpr std::array<ColorRgba, 19> ModelTints = {{
        { 137u, 224u, 196u, 255u }, // 0 LinearWind
        { 112u, 207u, 151u, 255u }, // 1 Buoyancy
        { 224u, 131u, 104u, 255u }, // 2 RadialBurst
        { 142u, 182u, 224u, 255u }, // 3 Attractor
        { 217u, 158u, 108u, 255u }, // 4 Repulsor
        { 123u, 198u, 219u, 255u }, // 5 AxisAttractor
        { 129u, 199u, 132u, 255u }, // 6 Containment
        { 185u, 169u, 105u, 255u }, // 7 RotationalColumn
        { 200u, 176u, 116u, 255u }, // 8 Torque
        { 161u, 152u, 204u, 255u }, // 9 Drag
        { 217u, 181u, 108u, 255u }, // 10 CurlNoise
        { 116u, 198u, 184u, 255u }, // 11 AtmosphericGust
        { 112u, 207u, 190u, 255u }, // 12 VorticityConfinement
        { 209u, 194u, 122u, 255u }, // 13 BrownianNoise
        { 116u, 200u, 224u, 255u }, // 14 LorentzField
        { 198u, 139u, 217u, 255u }, // 15 MagneticDipole
        { 100u, 181u, 246u, 255u }, // 16 Stream
        { 157u, 180u, 226u, 255u }, // 17 SplineForce
        { 142u, 199u, 158u, 255u }, // 18 BoundaryAdhesion
    }};
    return (ForceModelOrdinal < ModelTints.size()) ? ModelTints[ForceModelOrdinal] : ModelTints[0];
}

void VectorIconRepository::ConstructForceModelInBounds(
    PixelSpace& Surface,
    uint32_t    ForceModelOrdinal,
    Rectangle2D Bounds,
    ColorRgba   AccentInk,
    float       Opacity) noexcept
{
    const Rectangle2D Square = ComputeFittedSquare(Bounds);
    if (Square.Width <= 2.0f || Opacity <= 0.0f)
        return;

    const ColorRgba Ink    = FadeColorAlpha(AccentInk, Opacity);
    const float     Scale  = Square.Width / 16.0f;
    const float     Stroke = std::max(1.1f, 1.4f * Scale);
    const SharedIconStorage& Storage = QuerySharedStorage();

    auto P = [&](float X, float Y) noexcept -> Point2D {
        return { Square.X + X * Scale, Square.Y + Y * Scale };
    };
    auto StrokeDashedRing = [&](float Cx, float Cy, float Radius, int DashCount) noexcept {
        constexpr float TwoPi = 6.2831853f;
        const float     Step  = TwoPi / static_cast<float>(DashCount);
        for (int Seg = 0; Seg < DashCount; ++Seg)
        {
            const float A0 = static_cast<float>(Seg) * Step;
            const float A1 = A0 + Step * 0.54f;
            const std::array<Point2D, 3> Arc = {{
                P(Cx + Radius * std::cos(A0), Cy + Radius * std::sin(A0)),
                P(Cx + Radius * std::cos(0.5f * (A0 + A1)), Cy + Radius * std::sin(0.5f * (A0 + A1))),
                P(Cx + Radius * std::cos(A1), Cy + Radius * std::sin(A1)),
            }};
            Surface.StrokePolyline(Arc, Ink, Stroke, false);
        }
    };

    if (ForceModelOrdinal < Storage.ForceModelPaths.size())
    {
        VectorIconPlacement Place {};
        Place.X              = Square.X;
        Place.Y              = Square.Y;
        Place.Size           = Square.Width;
        Place.Ink            = Ink;
        Place.StrokeWidth    = Stroke;
        Place.AbsoluteStroke = true;
        RasterizeSharedRange(Surface, Storage, Storage.ForceModelPaths[ForceModelOrdinal], Place, 16.0f, false);
    }

    switch (ForceModelOrdinal)
    {
    case 1u:
        Surface.StrokeCircle(P(8.0f, 11.5f), 1.7f * Scale, Ink, Stroke);
        break;
    case 2u:
        Surface.StrokeCircle(P(8.0f, 8.0f), 2.2f * Scale, Ink, Stroke);
        StrokeDashedRing(8.0f, 8.0f, 5.5f, 8);
        break;
    case 3u:
        Surface.FillCircle(P(8.0f, 8.0f), 1.8f * Scale, Ink);
        break;
    case 4u:
        Surface.StrokeCircle(P(8.0f, 8.0f), 1.8f * Scale, Ink, Stroke);
        break;
    case 6u:
        Surface.StrokeCircle(P(8.0f, 8.0f), 6.0f * Scale, Ink, Stroke);
        StrokeDashedRing(8.0f, 8.0f, 3.0f, 6);
        break;
    case 8u:
        Surface.StrokeCircle(P(8.0f, 8.0f), 1.4f * Scale, Ink, Stroke);
        break;
    case 12u:
        Surface.StrokeCircle(P(6.5f, 9.5f), 3.0f * Scale, Ink, Stroke);
        break;
    case 13u:
        Surface.FillCircle(P(2.5f, 11.5f), 1.0f * Scale, Ink);
        Surface.FillCircle(P(14.0f, 6.0f), 1.0f * Scale, Ink);
        break;
    case 15u:
        Surface.StrokeRectangle({ Square.X + 6.6f * Scale, Square.Y + 4.5f * Scale, 2.8f * Scale, 7.0f * Scale }, Ink, 1.0f * Scale, Stroke);
        break;
    default:
        if (ForceModelOrdinal >= Storage.ForceModelPaths.size())
            Surface.StrokeCircle(P(8.0f, 8.0f), 4.0f * Scale, Ink, Stroke);
        break;
    }
}

void VectorIconRepository::ConstructFractureAlgorithmInBounds(
    PixelSpace&           Surface,
    FractureAlgorithmIcon Algorithm,
    Rectangle2D           Bounds,
    ColorRgba             Ink) noexcept
{
    const Rectangle2D Square = ComputeFittedSquare(Bounds);
    if (Square.Width <= 2.0f)
        return;

    const SharedIconStorage& Storage = QuerySharedStorage();
    const std::size_t        Idx     = static_cast<std::size_t>(Algorithm);
    const std::size_t        SafeIdx = Idx < Storage.FractureAlgorithmPaths.size() ? Idx : 0u;

    VectorIconPlacement Place {};
    Place.X              = Square.X;
    Place.Y              = Square.Y;
    Place.Size           = Square.Width;
    Place.Ink            = Ink;
    Place.StrokeWidth    = std::clamp(Square.Width * 0.075f, 1.25f, 1.85f);
    Place.AbsoluteStroke = true;

    RasterizeSharedRange(Surface, Storage, Storage.FractureAlgorithmPaths[SafeIdx], Place, 24.0f, false);
}

void VectorIconRepository::ConstructCapabilityInBounds(
    PixelSpace&    Surface,
    CapabilityIcon Capability,
    Rectangle2D    Bounds,
    ColorRgba      Ink,
    ColorRgba      InteriorCutout) noexcept
{
    const Rectangle2D Square = ComputeFittedSquare(Bounds);
    if (Square.Width <= 2.0f)
        return;

    const float Extent = Square.Width;
    const auto  PX     = [&](float X) noexcept -> float { return Square.X + (X / 24.0f) * Extent; };
    const auto  PY     = [&](float Y) noexcept -> float { return Square.Y + (Y / 24.0f) * Extent; };

    switch (Capability)
    {
    case CapabilityIcon::Visibility:
    {
        const SharedIconStorage& Storage = QuerySharedStorage();
        VectorIconPlacement Place {};
        Place.X    = Square.X;
        Place.Y    = Square.Y;
        Place.Size = Extent;
        Place.Ink  = Ink;
        RasterizeSharedRange(Surface, Storage, Storage.CapabilityEyeOuterPath, Place, 24.0f, true);
        const Point2D Center{ PX(12.0f), PY(12.0f) };
        Surface.FillCircle(Center, (4.8f / 24.0f) * Extent, InteriorCutout);
        Surface.FillCircle(Center, (2.8f / 24.0f) * Extent, Ink);
        break;
    }
    case CapabilityIcon::ShadowCaster:
    {
        const std::array<Point2D, 5> CastPoly = {{
            { PX(13.0f), PY(8.0f)  },
            { PX(22.0f), PY(17.0f) },
            { PX(14.0f), PY(22.0f) },
            { PX(6.0f),  PY(15.0f) },
            { PX(13.0f), PY(15.0f) },
        }};
        Surface.FillPolygon(CastPoly, FadeColorAlpha(Ink, 0.42f));
        Surface.FillRectangle({ PX(3.0f), PY(5.0f), PX(13.0f) - PX(3.0f), PY(15.0f) - PY(5.0f) }, Ink, 1.0f);
        break;
    }
    case CapabilityIcon::DynamicMotion:
    {
        const std::array<Point2D, 3> Wedge = {{
            { PX(10.0f), PY(4.0f)  },
            { PX(22.0f), PY(12.0f) },
            { PX(10.0f), PY(20.0f) },
        }};
        Surface.FillPolygon(Wedge, Ink);
        Surface.FillRectangle({ PX(1.0f), PY(6.0f),  PX(7.0f) - PX(1.0f), PY(9.0f) - PY(6.0f)   }, Ink, 0.6f);
        Surface.FillRectangle({ PX(0.0f), PY(11.0f), PX(7.0f) - PX(0.0f), PY(13.0f) - PY(11.0f) }, Ink, 0.6f);
        Surface.FillRectangle({ PX(1.0f), PY(15.0f), PX(7.0f) - PX(1.0f), PY(18.0f) - PY(15.0f) }, Ink, 0.6f);
        break;
    }
    case CapabilityIcon::TransformLock:
    {
        const float ShackleStroke = std::max(1.6f, (2.6f / 24.0f) * Extent);
        Surface.StrokeCircle({ PX(12.0f), PY(8.0f) }, (3.6f / 24.0f) * Extent, Ink, ShackleStroke);
        Surface.FillRectangle(
            { PX(5.0f), PY(10.0f), PX(19.0f) - PX(5.0f), PY(22.0f) - PY(10.0f) },
            Ink,
            (3.0f / 24.0f) * Extent);
        break;
    }
    case CapabilityIcon::Count:
        break;
    }
}

void VectorIconRepository::ConstructComponentModeInBounds(
    PixelSpace&       Surface,
    ComponentModeIcon Mode,
    Rectangle2D       Bounds,
    ColorRgba         Ink) noexcept
{
    const Rectangle2D Square = ComputeFittedSquare(Bounds);
    if (Square.Width <= 2.0f)
        return;

    const float Extent = Square.Width;
    const float L      = Square.X + Extent * 0.18f;
    const float R      = Square.X + Extent * 0.82f;
    const float T      = Square.Y + Extent * 0.18f;
    const float B      = Square.Y + Extent * 0.82f;

    switch (Mode)
    {
    case ComponentModeIcon::Vertex:
        Surface.FillCircle({ L, T }, 1.9f, Ink);
        Surface.FillCircle({ R, T }, 1.9f, Ink);
        Surface.FillCircle({ L, B }, 1.9f, Ink);
        Surface.FillCircle({ R, B }, 1.9f, Ink);
        break;
    case ComponentModeIcon::Edge:
        Surface.StrokeLine({ L, B }, { R, T }, Ink, 1.6f);
        Surface.FillCircle({ L, B }, 1.8f, Ink);
        Surface.FillCircle({ R, T }, 1.8f, Ink);
        break;
    case ComponentModeIcon::Face:
    {
        ColorRgba Soft = Ink;
        Soft.Alpha     = 72u;
        Surface.FillRectangle({ L, T, R - L, B - T }, Soft, 1.5f);
        Surface.StrokeRectangle({ L, T, R - L, B - T }, Ink, 1.5f, 1.3f);
        break;
    }
    case ComponentModeIcon::Island:
        Surface.StrokeRectangle({ Square.X + Extent * 0.12f, Square.Y + Extent * 0.14f, Extent * 0.42f, Extent * 0.42f }, Ink, 1.5f, 1.25f);
        Surface.StrokeRectangle({ Square.X + Extent * 0.46f, Square.Y + Extent * 0.44f, Extent * 0.42f, Extent * 0.42f }, Ink, 1.5f, 1.25f);
        break;
    case ComponentModeIcon::Count:
        break;
    }
}

void VectorIconRepository::ConstructStudioInBounds(
    PixelSpace& Surface,
    StudioIcon  Icon,
    Rectangle2D Bounds,
    ColorRgba   PrimaryTint,
    float       Opacity,
    ColorRgba   InteriorCutout) noexcept
{
    const Rectangle2D Square = ComputeFittedSquare(Bounds);
    if (Square.Width <= 2.0f || Opacity <= 0.0f)
        return;

    const float Extent   = Square.Width;
    const auto  ProjectX = [&](float SvgX) noexcept -> float { return Square.X + (SvgX / 256.0f) * Extent; };
    const auto  ProjectY = [&](float SvgY) noexcept -> float { return Square.Y + (SvgY / 256.0f) * Extent; };

    switch (Icon)
    {
    case StudioIcon::ShowcaseFolder:
    {
        const Rectangle2D BackRect{
            ProjectX(29.0f), ProjectY(55.0f),
            ProjectX(224.0f) - ProjectX(29.0f), ProjectY(201.0f) - ProjectY(55.0f),
        };
        const float BackCorner   = (20.0f / 256.0f) * Extent;
        const float PocketStroke = std::max(0.9f, Extent * 0.03125f);
        Surface.FillRectangle(BackRect, FadeColorAlpha({ 48u, 48u, 48u, 255u }, Opacity), BackCorner);
        Surface.StrokeRectangle(BackRect, FadeColorAlpha({ 85u, 85u, 85u, 255u }, Opacity), BackCorner, PocketStroke);

        const std::array<Point2D, 4> LeftSheet = {{
            { ProjectX(46.0f),  ProjectY(63.0f)  },
            { ProjectX(143.0f), ProjectY(55.0f)  },
            { ProjectX(149.0f), ProjectY(122.0f) },
            { ProjectX(52.0f),  ProjectY(130.0f) },
        }};
        Surface.FillPolygon(LeftSheet, FadeColorAlpha({ 222u, 222u, 219u, 245u }, Opacity));

        const std::array<Point2D, 4> RightSheet = {{
            { ProjectX(132.0f), ProjectY(66.0f)  },
            { ProjectX(206.0f), ProjectY(78.0f)  },
            { ProjectX(198.0f), ProjectY(134.0f) },
            { ProjectX(124.0f), ProjectY(124.0f) },
        }};
        Surface.FillPolygon(RightSheet, FadeColorAlpha({ 222u, 222u, 219u, 255u }, Opacity));

        if (Extent >= 20.0f)
        {
            ConstructTreeBranchMark(
                Surface, { ProjectX(60.0f), ProjectY(69.0f) },
                (0.78f / 256.0f) * Extent, FadeColorAlpha({ 120u, 122u, 120u, 115u }, Opacity), 0.85f);
        }

        const std::array<Point2D, 10> FrontPocket = {{
            { ProjectX(29.0f),  ProjectY(92.0f)  },
            { ProjectX(42.0f),  ProjectY(75.0f)  },
            { ProjectX(112.0f), ProjectY(75.0f)  },
            { ProjectX(146.0f), ProjectY(98.0f)  },
            { ProjectX(212.0f), ProjectY(98.0f)  },
            { ProjectX(224.0f), ProjectY(110.0f) },
            { ProjectX(224.0f), ProjectY(204.0f) },
            { ProjectX(212.0f), ProjectY(214.0f) },
            { ProjectX(41.0f),  ProjectY(214.0f) },
            { ProjectX(29.0f),  ProjectY(204.0f) },
        }};
        Surface.FillPolygon(FrontPocket, FadeColorAlpha({ 60u, 60u, 60u, 255u }, Opacity));
        Surface.StrokePolyline(FrontPocket, FadeColorAlpha({ 85u, 85u, 85u, 255u }, Opacity), PocketStroke, true);

        const Rectangle2D SubTile{
            ProjectX(44.0f), ProjectY(159.0f),
            ProjectX(87.0f) - ProjectX(44.0f), ProjectY(202.0f) - ProjectY(159.0f),
        };
        const float SubCorner = (11.0f / 256.0f) * Extent;
        Surface.FillRectangle(SubTile, FadeColorAlpha({ 57u, 57u, 57u, 255u }, Opacity), SubCorner);
        Surface.FillRectangle(SubTile, FadeColorAlpha(PrimaryTint, Opacity * 0.14f), SubCorner);
        Surface.StrokeRectangle(SubTile, FadeColorAlpha({ 85u, 85u, 85u, 255u }, Opacity), SubCorner, 0.9f);
        ConstructTreeBranchMark(
            Surface, { ProjectX(52.0f), ProjectY(167.0f) },
            (0.84375f / 256.0f) * Extent, FadeColorAlpha(PrimaryTint, Opacity), std::max(1.0f, Extent * 0.042f));
        break;
    }

    case StudioIcon::GenericFolder:
    {
        const std::array<Point2D, 8> BackTab = {{
            { ProjectX(26.0f),  ProjectY(56.0f)  },
            { ProjectX(40.0f),  ProjectY(46.0f)  },
            { ProjectX(103.0f), ProjectY(46.0f)  },
            { ProjectX(127.0f), ProjectY(66.0f)  },
            { ProjectX(218.0f), ProjectY(66.0f)  },
            { ProjectX(230.0f), ProjectY(78.0f)  },
            { ProjectX(230.0f), ProjectY(198.0f) },
            { ProjectX(26.0f),  ProjectY(198.0f) },
        }};
        Surface.FillPolygon(BackTab, FadeColorAlpha({ 48u, 48u, 48u, 255u }, Opacity));
        Surface.StrokePolyline(BackTab, FadeColorAlpha({ 85u, 85u, 85u, 255u }, Opacity), 1.0f, true);

        const Rectangle2D FrontBody{
            ProjectX(26.0f), ProjectY(88.0f),
            ProjectX(230.0f) - ProjectX(26.0f), ProjectY(210.0f) - ProjectY(88.0f),
        };
        const float Corner = (18.0f / 256.0f) * Extent;
        Surface.FillRectangle(FrontBody, FadeColorAlpha({ 60u, 60u, 60u, 255u }, Opacity), Corner);
        Surface.StrokeRectangle(FrontBody, FadeColorAlpha({ 85u, 85u, 85u, 255u }, Opacity), Corner, 1.0f);

        const float RuleStroke = std::max(1.2f, (8.0f / 256.0f) * Extent);
        Surface.StrokeLine({ ProjectX(82.0f), ProjectY(129.0f) }, { ProjectX(174.0f), ProjectY(129.0f) }, FadeColorAlpha(PrimaryTint, Opacity), RuleStroke);
        Surface.StrokeLine({ ProjectX(82.0f), ProjectY(151.0f) }, { ProjectX(142.0f), ProjectY(151.0f) }, FadeColorAlpha(PrimaryTint, Opacity), RuleStroke);
        break;
    }

    case StudioIcon::SceneFolder:
    {
        const Rectangle2D BackPlate{
            ProjectX(28.0f), ProjectY(52.0f),
            ProjectX(228.0f) - ProjectX(28.0f), ProjectY(206.0f) - ProjectY(52.0f),
        };
        const float BackRx = (20.0f / 256.0f) * Extent;
        Surface.FillRectangle(BackPlate, FadeColorAlpha({ 50u, 50u, 52u, 255u }, Opacity), BackRx);
        Surface.StrokeRectangle(BackPlate, FadeColorAlpha({ 92u, 92u, 96u, 255u }, Opacity), BackRx, 1.0f);

        const std::array<Point2D, 4> SheetA = {{
            { ProjectX(50.0f),  ProjectY(68.0f)  },
            { ProjectX(144.0f), ProjectY(60.0f)  },
            { ProjectX(148.0f), ProjectY(124.0f) },
            { ProjectX(54.0f),  ProjectY(130.0f) },
        }};
        const std::array<Point2D, 4> SheetB = {{
            { ProjectX(126.0f), ProjectY(70.0f)  },
            { ProjectX(202.0f), ProjectY(80.0f)  },
            { ProjectX(196.0f), ProjectY(134.0f) },
            { ProjectX(122.0f), ProjectY(124.0f) },
        }};
        Surface.FillPolygon(SheetA, FadeColorAlpha({ 220u, 219u, 214u, 240u }, Opacity));
        Surface.FillPolygon(SheetB, FadeColorAlpha({ 234u, 233u, 228u, 250u }, Opacity));

        const std::array<Point2D, 10> Pocket = {{
            { ProjectX(28.0f),  ProjectY(100.0f) },
            { ProjectX(42.0f),  ProjectY(84.0f)  },
            { ProjectX(108.0f), ProjectY(84.0f)  },
            { ProjectX(134.0f), ProjectY(104.0f) },
            { ProjectX(214.0f), ProjectY(104.0f) },
            { ProjectX(228.0f), ProjectY(118.0f) },
            { ProjectX(228.0f), ProjectY(204.0f) },
            { ProjectX(214.0f), ProjectY(218.0f) },
            { ProjectX(42.0f),  ProjectY(218.0f) },
            { ProjectX(28.0f),  ProjectY(204.0f) },
        }};
        Surface.FillPolygon(Pocket, FadeColorAlpha({ 64u, 64u, 66u, 255u }, Opacity));
        Surface.StrokePolyline(Pocket, FadeColorAlpha({ 102u, 102u, 106u, 255u }, Opacity), 1.0f, true);
        break;
    }

    case StudioIcon::CollectionCubes:
    {
        const SharedIconStorage& Storage = QuerySharedStorage();
        VectorIconPlacement FillP { Square.X, Square.Y, Extent, FadeColorAlpha({ 72u, 52u, 118u, 255u }, Opacity), 1.0f };
        VectorIconPlacement StrkP { Square.X, Square.Y, Extent, FadeColorAlpha({ 184u, 146u, 255u, 255u }, Opacity), std::max(1.1f, Extent * 0.055f) };
        StrkP.AbsoluteStroke = true;
        RasterizeSharedRange(Surface, Storage, Storage.StudioCollectionFillPath, FillP, 256.0f, true);
        RasterizeSharedRange(Surface, Storage, Storage.StudioCollectionStrokePath, StrkP, 256.0f, false);
        break;
    }

    case StudioIcon::CameraOptics:
    {
        const Rectangle2D Body{
            ProjectX(28.0f), ProjectY(55.0f),
            ProjectX(228.0f) - ProjectX(28.0f), ProjectY(209.0f) - ProjectY(55.0f),
        };
        const float BodyRx = (23.0f / 256.0f) * Extent;
        Surface.FillRectangle(Body, FadeColorAlpha({ 54u, 55u, 63u, 255u }, Opacity), BodyRx);
        Surface.StrokeRectangle(Body, FadeColorAlpha({ 134u, 136u, 150u, 220u }, Opacity), BodyRx, std::max(1.0f, Extent * 0.045f));

        const Point2D LensCenter{ ProjectX(128.0f), ProjectY(134.0f) };
        Surface.FillCircle(LensCenter, (58.0f / 256.0f) * Extent, FadeColorAlpha({ 42u, 44u, 52u, 255u }, Opacity));
        Surface.StrokeCircle(LensCenter, (58.0f / 256.0f) * Extent, FadeColorAlpha({ 146u, 146u, 158u, 165u }, Opacity), 1.0f);
        Surface.FillCircle(LensCenter, (42.0f / 256.0f) * Extent, FadeColorAlpha({ 16u, 17u, 24u, 255u }, Opacity));
        Surface.StrokeCircle(LensCenter, (42.0f / 256.0f) * Extent, FadeColorAlpha({ 119u, 119u, 131u, 150u }, Opacity), 0.9f);
        Surface.FillCircle({ ProjectX(118.0f), ProjectY(118.0f) }, std::max(1.1f, (11.0f / 256.0f) * Extent), FadeColorAlpha({ 226u, 230u, 242u, 165u }, Opacity));

        const Rectangle2D FlashWindow{
            ProjectX(184.0f), ProjectY(71.0f),
            std::max(2.0f, (22.0f / 256.0f) * Extent), std::max(1.6f, (16.0f / 256.0f) * Extent),
        };
        Surface.FillRectangle(FlashWindow, FadeColorAlpha({ 220u, 222u, 235u, 225u }, Opacity), 1.0f);
        break;
    }

    case StudioIcon::GeometryCube:
        ConstructPrimitiveInBounds(Surface, PrimitiveShapeIcon::Box, Square, Opacity);
        break;

    case StudioIcon::StackedLayers:
    {
        const auto Slab = [&](float CenterY, ColorRgba Fill, ColorRgba Edge) {
            const float S = Extent / 48.0f;
            const std::array<Point2D, 4> Diamond = {{
                { Square.X + 5.0f * S,  Square.Y + CenterY * S           },
                { Square.X + 24.0f * S, Square.Y + (CenterY - 10.0f) * S },
                { Square.X + 43.0f * S, Square.Y + CenterY * S           },
                { Square.X + 24.0f * S, Square.Y + (CenterY + 11.0f) * S },
            }};
            Surface.FillPolygon(Diamond, FadeColorAlpha(Fill, Opacity));
            Surface.StrokePolyline(Diamond, FadeColorAlpha(Edge, Opacity), std::max(1.1f, 2.2f * S), true);
        };
        Slab(30.0f, { 118u, 102u, 143u, 255u }, { 160u, 142u, 184u, 255u });
        Slab(23.0f, { 160u, 139u, 185u, 255u }, { 192u, 174u, 212u, 255u });
        Slab(16.0f, { 197u, 179u, 218u, 255u }, { 223u, 210u, 236u, 255u });
        break;
    }

    case StudioIcon::PointLuminaire:
    {
        const Point2D   Center{ ProjectX(128.0f), ProjectY(128.0f) };
        const float     InnerRadius = (32.0f / 256.0f) * Extent;
        const float     Stroke      = std::max(1.3f, (7.0f / 256.0f) * Extent);
        const ColorRgba RayInk      = FadeColorAlpha({ 255u, 205u, 56u, 255u }, Opacity);
        Surface.FillCircle(Center, InnerRadius, FadeColorAlpha({ 240u, 173u, 34u, 255u }, Opacity));
        Surface.StrokeCircle(Center, InnerRadius, RayInk, Stroke);

        const float InnerRay = (63.0f / 256.0f) * Extent;
        const float OuterRay = (81.0f / 256.0f) * Extent;
        for (int Ray = 0; Ray < 8; ++Ray)
        {
            const float Angle = static_cast<float>(Ray) * 0.78539816339f;
            const float CosA  = std::cos(Angle);
            const float SinA  = std::sin(Angle);
            Surface.StrokeLine(
                { Center.X + CosA * InnerRay, Center.Y + SinA * InnerRay },
                { Center.X + CosA * OuterRay, Center.Y + SinA * OuterRay },
                RayInk, Stroke);
        }
        break;
    }

    case StudioIcon::ForceStream:
    {
        const ColorRgba StreamInk = FadeColorAlpha({ 104u, 220u, 255u, 255u }, Opacity);
        const ColorRgba CrestInk  = FadeColorAlpha({ 255u, 180u, 84u, 255u }, Opacity);
        const float     StrokeW   = std::max(1.4f, (13.0f / 256.0f) * Extent);
        const std::array<Point2D, 4> TopArc = {{
            { ProjectX(34.0f), ProjectY(82.0f) }, { ProjectX(96.0f), ProjectY(54.0f) },
            { ProjectX(162.0f), ProjectY(92.0f) }, { ProjectX(218.0f), ProjectY(68.0f) },
        }};
        const std::array<Point2D, 4> MidStream = {{
            { ProjectX(24.0f), ProjectY(128.0f) }, { ProjectX(104.0f), ProjectY(128.0f) },
            { ProjectX(158.0f), ProjectY(128.0f) }, { ProjectX(226.0f), ProjectY(128.0f) },
        }};
        const std::array<Point2D, 4> BottomArc = {{
            { ProjectX(34.0f), ProjectY(176.0f) }, { ProjectX(96.0f), ProjectY(202.0f) },
            { ProjectX(162.0f), ProjectY(164.0f) }, { ProjectX(210.0f), ProjectY(186.0f) },
        }};
        Surface.StrokePolyline(TopArc, StreamInk, StrokeW, false);
        Surface.StrokePolyline(MidStream, CrestInk, StrokeW, false);
        Surface.StrokePolyline(BottomArc, StreamInk, StrokeW, false);

        const std::array<Point2D, 3> ArrowTip = {{
            { ProjectX(198.0f), ProjectY(104.0f) },
            { ProjectX(234.0f), ProjectY(128.0f) },
            { ProjectX(198.0f), ProjectY(152.0f) },
        }};
        Surface.FillPolygon(ArrowTip, CrestInk);
        break;
    }

    case StudioIcon::MaterialSphere:
    case StudioIcon::MaterialCutCube:
    {
        const Point2D   C = Square.Center();
        const float     R = Extent * 0.44f;
        const ColorRgba PrimaryColor = FadeColorAlpha(PrimaryTint, Opacity);
        const ColorRgba DarkRim {
            static_cast<uint8_t>(std::max(0, static_cast<int>(PrimaryColor.Red)   - 48)),
            static_cast<uint8_t>(std::max(0, static_cast<int>(PrimaryColor.Green) - 48)),
            static_cast<uint8_t>(std::max(0, static_cast<int>(PrimaryColor.Blue)  - 48)),
            PrimaryColor.Alpha,
        };
        const ColorRgba LitCenter {
            static_cast<uint8_t>(std::min(255, static_cast<int>(PrimaryColor.Red)   + 42)),
            static_cast<uint8_t>(std::min(255, static_cast<int>(PrimaryColor.Green) + 42)),
            static_cast<uint8_t>(std::min(255, static_cast<int>(PrimaryColor.Blue)  + 42)),
            PrimaryColor.Alpha,
        };
        Surface.FillCircle(C, R, DarkRim);
        Surface.FillCircle({ C.X - R * 0.10f, C.Y - R * 0.12f }, R * 0.84f, PrimaryColor);
        Surface.FillCircle({ C.X - R * 0.24f, C.Y - R * 0.26f }, R * 0.46f, LitCenter);
        Surface.FillCircle({ C.X - R * 0.34f, C.Y - R * 0.36f }, R * 0.18f, FadeColorAlpha({ 255u, 255u, 255u, 155u }, Opacity));

        if (Icon == StudioIcon::MaterialCutCube)
        {
            const std::array<Point2D, 3> Wedge = {{
                C,
                { C.X + R * 0.94f, C.Y - R * 0.34f },
                { C.X + R * 0.72f, C.Y + R * 0.68f },
            }};
            Surface.FillPolygon(Wedge, FadeColorAlpha({ 18u, 18u, 20u, 210u }, Opacity));
            Surface.StrokeLine(C, Wedge[1], FadeColorAlpha({ 238u, 238u, 236u, 165u }, Opacity), 1.0f);
            Surface.StrokeLine(C, Wedge[2], FadeColorAlpha({ 238u, 238u, 236u, 165u }, Opacity), 1.0f);
        }
        Surface.StrokeCircle(C, R, FadeColorAlpha({ 255u, 255u, 255u, 42u }, Opacity), 1.0f);
        break;
    }

    case StudioIcon::SimulateGearPlay:
    {
        const SharedIconStorage& Storage = QuerySharedStorage();
        VectorIconPlacement GearPlace { Square.X, Square.Y, Extent, FadeColorAlpha({ 86u, 134u, 104u, 255u }, Opacity), 1.0f };
        RasterizeSharedRange(Surface, Storage, Storage.StudioGearFillPath, GearPlace, 256.0f, true);
        Surface.FillCircle({ ProjectX(128.0f), ProjectY(138.0f) }, (46.0f / 256.0f) * Extent, InteriorCutout);
        ConstructControlCentered(
            Surface,
            ControlIcon::PlaySolid,
            { ProjectX(130.0f), ProjectY(138.0f) },
            Extent * 0.36f,
            FadeColorAlpha({ 102u, 212u, 128u, 255u }, Opacity));
        break;
    }

    case StudioIcon::BakeGrid:
    {
        const SharedIconStorage& Storage = QuerySharedStorage();
        VectorIconPlacement UpperP { Square.X, Square.Y, Extent, FadeColorAlpha({ 116u, 96u, 52u, 255u }, Opacity), 1.0f };
        VectorIconPlacement LowerP { Square.X, Square.Y, Extent, FadeColorAlpha({ 78u, 62u, 36u, 255u }, Opacity), 1.0f };
        VectorIconPlacement StrkP  { Square.X, Square.Y, Extent, FadeColorAlpha({ 240u, 198u, 108u, 255u }, Opacity), std::max(1.1f, Extent * 0.055f) };
        StrkP.AbsoluteStroke = true;
        RasterizeSharedRange(Surface, Storage, Storage.StudioBakeLowerFillPath, LowerP, 256.0f, true);
        RasterizeSharedRange(Surface, Storage, Storage.StudioBakeUpperFillPath, UpperP, 256.0f, true);
        RasterizeSharedRange(Surface, Storage, Storage.StudioBakeGridStrokePath, StrkP, 256.0f, false);
        break;
    }

    case StudioIcon::Count:
        break;
    }
}

} // namespace Frontier
