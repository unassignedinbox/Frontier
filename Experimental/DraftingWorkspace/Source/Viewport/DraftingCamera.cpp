/*====================================================================================================================================
                                                       DRAFTINGCAMERA.CPP
====================================================================================================================================*/
// 🧩 Z-up orbit camera: orbit / pan / dolly, view presets, perspective or orthographic, Vulkan clip matrices, pixel rays

//------------------------------------------------------------------------------------------------------------------------
//                                                    INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Viewport/DraftingCamera.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace Frontier::DraftingWorkspace
{

namespace
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    INTERNAL FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

using Triple = std::array<double, 3>;
constexpr double HalfPi = 0.5 * std::numbers::pi;

[[nodiscard]] double Dot(const Triple& Left, const Triple& Right) { return Left[0] * Right[0] + Left[1] * Right[1] + Left[2] * Right[2]; }

[[nodiscard]] DraftingMatrix Multiply(const DraftingMatrix& Left, const DraftingMatrix& Right)
{
    DraftingMatrix Product{};
    for (int Column = 0; Column < 4; ++Column)
        for (int Row = 0; Row < 4; ++Row)
        {
            double Sum = 0.0;
            for (int Inner = 0; Inner < 4; ++Inner) Sum += Left[Inner * 4 + Row] * Right[Column * 4 + Inner];
            Product[Column * 4 + Row] = Sum;
        }
    return Product;
}

[[nodiscard]] std::array<double, 4> Transform(const DraftingMatrix& Matrix, const std::array<double, 4>& Point)
{
    std::array<double, 4> Result{};
    for (int Row = 0; Row < 4; ++Row)
        for (int Column = 0; Column < 4; ++Column) Result[Row] += Matrix[Column * 4 + Row] * Point[Column];
    return Result;
}

} // namespace

//------------------------------------------------------------------------------------------------------------------------
//                                                    PUBLIC FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

DraftingMatrix InvertDraftingMatrix(const DraftingMatrix& Source)
{
    // 📝 Gauss–Jordan with partial pivoting on an augmented [M | I] (row-major working copy).
    double Work[4][8];
    for (int Row = 0; Row < 4; ++Row)
        for (int Column = 0; Column < 4; ++Column)
        {
            Work[Row][Column]     = Source[Column * 4 + Row];
            Work[Row][Column + 4] = Row == Column ? 1.0 : 0.0;
        }
    for (int Pivot = 0; Pivot < 4; ++Pivot)
    {
        int Best = Pivot;
        for (int Row = Pivot + 1; Row < 4; ++Row) if (std::abs(Work[Row][Pivot]) > std::abs(Work[Best][Pivot])) Best = Row;
        if (Best != Pivot) for (int Column = 0; Column < 8; ++Column) std::swap(Work[Pivot][Column], Work[Best][Column]);
        const double Scale = Work[Pivot][Pivot];
        if (std::abs(Scale) < 1.0e-300) return DraftingMatrix{ 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
        for (int Column = 0; Column < 8; ++Column) Work[Pivot][Column] /= Scale;
        for (int Row = 0; Row < 4; ++Row)
        {
            if (Row == Pivot) continue;
            const double Factor = Work[Row][Pivot];
            for (int Column = 0; Column < 8; ++Column) Work[Row][Column] -= Factor * Work[Pivot][Column];
        }
    }
    DraftingMatrix Inverse{};
    for (int Row = 0; Row < 4; ++Row)
        for (int Column = 0; Column < 4; ++Column) Inverse[Column * 4 + Row] = Work[Row][Column + 4];
    return Inverse;
}

DraftingCameraFrame FrameDraftingCamera(const DraftingCamera& Camera, double Width, double Height)
{
    DraftingCameraFrame Frame;
    Frame.Width  = std::max(1.0, Width);
    Frame.Height = std::max(1.0, Height);
    Frame.OrthographicEnabled = Camera.OrthographicEnabled;
    const double Aspect = Frame.Width / Frame.Height;
    const Triple Toward = { std::cos(Camera.Pitch) * std::cos(Camera.Yaw), std::cos(Camera.Pitch) * std::sin(Camera.Yaw), std::sin(Camera.Pitch) };
    Frame.Eye     = { Camera.Target[0] + Camera.Distance * Toward[0], Camera.Target[1] + Camera.Distance * Toward[1], Camera.Target[2] + Camera.Distance * Toward[2] };
    Frame.Forward = { -Toward[0], -Toward[1], -Toward[2] };
    // 📝 Right depends on the yaw only, so Top / Bottom (pitch ±π/2) need no special case.
    Frame.Right   = { -std::sin(Camera.Yaw), std::cos(Camera.Yaw), 0.0 };
    Frame.Up      = { Frame.Right[1] * Frame.Forward[2] - Frame.Right[2] * Frame.Forward[1],
                      Frame.Right[2] * Frame.Forward[0] - Frame.Right[0] * Frame.Forward[2],
                      Frame.Right[0] * Frame.Forward[1] - Frame.Right[1] * Frame.Forward[0] };

    // ① view: world → camera (x right, y up, looking down −z)
    DraftingMatrix View{};
    for (int Axis = 0; Axis < 3; ++Axis)
    {
        View[Axis * 4 + 0] = Frame.Right[Axis];
        View[Axis * 4 + 1] = Frame.Up[Axis];
        View[Axis * 4 + 2] = -Frame.Forward[Axis];
    }
    View[12] = -Dot(Frame.Right, Frame.Eye);
    View[13] = -Dot(Frame.Up, Frame.Eye);
    View[14] = Dot(Frame.Forward, Frame.Eye);
    View[15] = 1.0;

    // ② projection: Vulkan clip (y down, depth 0 near → 1 far); the far plane leaves room for the grid to fade out
    const double Radius   = std::max(Camera.SceneRadius, 1.0e-3);
    const double Far      = Camera.Distance * 12.0 + Radius * 4.0;
    const double HalfView = Camera.Distance * std::tan(0.5 * Camera.FieldOfView);           // [len] - Half height at the target
    DraftingMatrix Projection{};
    if (Camera.OrthographicEnabled)
    {
        const double Near = -(Camera.Distance + Radius * 4.0);
        Projection[0]  = 1.0 / (HalfView * Aspect);
        Projection[5]  = -1.0 / HalfView;
        Projection[10] = -1.0 / (Far - Near);
        Projection[14] = -Near / (Far - Near);
        Projection[15] = 1.0;
    }
    else
    {
        const double Near  = std::max(Camera.Distance * 0.01, 1.0e-4);
        const double Focal = 1.0 / std::tan(0.5 * Camera.FieldOfView);
        Projection[0]  = Focal / Aspect;
        Projection[5]  = -Focal;
        Projection[10] = Far / (Near - Far);
        Projection[11] = -1.0;
        Projection[14] = Near * Far / (Near - Far);
    }
    Frame.ClipFromWorld = Multiply(Projection, View);
    Frame.WorldFromClip = InvertDraftingMatrix(Frame.ClipFromWorld);
    Frame.WorldPerPixel = 2.0 * HalfView / Frame.Height;
    return Frame;
}

DraftingRay CastDraftingRay(const DraftingCameraFrame& Frame, double PixelX, double PixelY)
{
    const double DeviceX = 2.0 * PixelX / Frame.Width - 1.0;
    const double DeviceY = 2.0 * PixelY / Frame.Height - 1.0;
    const std::array<double, 4> Near = Transform(Frame.WorldFromClip, { DeviceX, DeviceY, 0.0, 1.0 });
    const std::array<double, 4> Far  = Transform(Frame.WorldFromClip, { DeviceX, DeviceY, 1.0, 1.0 });
    const Triple From = { Near[0] / Near[3], Near[1] / Near[3], Near[2] / Near[3] };
    const Triple To   = { Far[0] / Far[3], Far[1] / Far[3], Far[2] / Far[3] };
    Triple Direction  = { To[0] - From[0], To[1] - From[1], To[2] - From[2] };
    const double Length = std::sqrt(Dot(Direction, Direction));
    for (double& Component : Direction) Component /= Length;
    return { From, Direction };
}

void OrbitDraftingCamera(DraftingCamera& Camera, double DeltaX, double DeltaY)
{
    Camera.Yaw  -= DeltaX * 0.008;
    Camera.Pitch = std::clamp(Camera.Pitch + DeltaY * 0.008, -HalfPi, HalfPi);
}

void PanDraftingCamera(DraftingCamera& Camera, const DraftingCameraFrame& Frame, double DeltaX, double DeltaY)
{
    for (int Axis = 0; Axis < 3; ++Axis)
        Camera.Target[Axis] += (-DeltaX * Frame.Right[Axis] + DeltaY * Frame.Up[Axis]) * Frame.WorldPerPixel;
}

void DollyDraftingCamera(DraftingCamera& Camera, double WheelSteps)
{
    Camera.Distance = std::clamp(Camera.Distance * std::exp(-0.12 * WheelSteps), 1.0e-3, 1.0e5);
}

void PresentDraftingView(DraftingCamera& Camera, DraftingViewPreset Preset)
{
    switch (Preset)
    {
    case DraftingViewPreset::Isometric: Camera.Yaw = -0.25 * std::numbers::pi; Camera.Pitch = std::atan(1.0 / std::sqrt(2.0)); break;
    case DraftingViewPreset::Top:       Camera.Yaw = -HalfPi;                  Camera.Pitch = HalfPi;                          break;
    case DraftingViewPreset::Bottom:    Camera.Yaw = -HalfPi;                  Camera.Pitch = -HalfPi;                         break;
    case DraftingViewPreset::Front:     Camera.Yaw = -HalfPi;                  Camera.Pitch = 0.0;                             break;
    case DraftingViewPreset::Back:      Camera.Yaw = HalfPi;                   Camera.Pitch = 0.0;                             break;
    case DraftingViewPreset::Right:     Camera.Yaw = 0.0;                      Camera.Pitch = 0.0;                             break;
    case DraftingViewPreset::Left:      Camera.Yaw = std::numbers::pi;         Camera.Pitch = 0.0;                             break;
    }
}

void FitDraftingCamera(DraftingCamera& Camera, const DraftingScene& Scene)
{
    Triple Span{};
    for (int Axis = 0; Axis < 3; ++Axis)
    {
        Camera.Target[Axis] = 0.5 * (Scene.BoundsLow[Axis] + Scene.BoundsHigh[Axis]);
        Span[Axis]          = Scene.BoundsHigh[Axis] - Scene.BoundsLow[Axis];
    }
    Camera.SceneRadius = std::max(0.5 * std::sqrt(Dot(Span, Span)), 1.0e-3);
    Camera.Distance    = Camera.SceneRadius / std::sin(0.5 * Camera.FieldOfView) * 1.08;
}

std::vector<uint32_t> EncloseDraftingEdges(const DraftingScene& Scene, const DraftingCameraFrame& Frame, std::array<double, 2> Low,
                                           std::array<double, 2> High, bool XRayEnabled, const DraftingSectionClip* Section)
{
    const auto Kept = [&](const float Point[3])
    {
        return !Section || Section->Plane[0] * Point[0] + Section->Plane[1] * Point[1] + Section->Plane[2] * Point[2] + Section->Plane[3] >= 0.0;
    };
    // World point → viewport pixel; false behind the eye.
    const auto Project = [&](const float Point[3], std::array<double, 2>& Pixel)
    {
        double Clip[4] = {};
        for (int Row = 0; Row < 4; ++Row)
            Clip[Row] = Frame.ClipFromWorld[0 * 4 + Row] * Point[0] + Frame.ClipFromWorld[1 * 4 + Row] * Point[1] + Frame.ClipFromWorld[2 * 4 + Row] * Point[2]
                      + Frame.ClipFromWorld[3 * 4 + Row];
        if (Clip[3] <= 1.0e-12) return false;
        Pixel = { (Clip[0] / Clip[3] * 0.5 + 0.5) * Frame.Width, (Clip[1] / Clip[3] * 0.5 + 0.5) * Frame.Height };
        return true;
    };
    const auto Inside = [&](const std::array<double, 2>& Pixel) { return Pixel[0] >= Low[0] && Pixel[0] <= High[0] && Pixel[1] >= Low[1] && Pixel[1] <= High[1]; };
    Triple Span{};
    for (int Axis = 0; Axis < 3; ++Axis) Span[Axis] = Scene.BoundsHigh[Axis] - Scene.BoundsLow[Axis];
    const double Slack = 2.0 * Frame.WorldPerPixel + 1.0e-4 * std::sqrt(Dot(Span, Span));            // [len]

    std::vector<uint32_t> Enclosed;
    for (uint32_t EdgeIndex = 0; EdgeIndex < Scene.Edges.size(); ++EdgeIndex)
    {
        const DraftingSceneEdge& Edge = Scene.Edges[EdgeIndex];
        if (Edge.SegmentCount == 0) continue;
        bool WhollyCondition = true;
        for (uint32_t Index = Edge.FirstSegment; Index < Edge.FirstSegment + Edge.SegmentCount && WhollyCondition; ++Index)
        {
            std::array<double, 2> Start{}, End{};
            WhollyCondition = Project(Scene.Segments[Index].Start, Start) && Project(Scene.Segments[Index].End, End) && Inside(Start) && Inside(End);
        }
        if (!WhollyCondition) continue;
        bool KeptCondition = false;
        for (uint32_t Index = Edge.FirstSegment; Index < Edge.FirstSegment + Edge.SegmentCount && !KeptCondition; ++Index)
            KeptCondition = Kept(Scene.Segments[Index].Start) || Kept(Scene.Segments[Index].End);
        if (!KeptCondition) continue;
        bool VisibleCondition = XRayEnabled;
        for (uint32_t Index = Edge.FirstSegment; Index < Edge.FirstSegment + Edge.SegmentCount && !VisibleCondition; ++Index)
        {
            const DraftingSceneSegment& Segment = Scene.Segments[Index];
            const float Middle[3] = { 0.5f * (Segment.Start[0] + Segment.End[0]), 0.5f * (Segment.Start[1] + Segment.End[1]), 0.5f * (Segment.Start[2] + Segment.End[2]) };
            std::array<double, 2> Pixel{};
            if (!Kept(Middle) || !Project(Middle, Pixel)) continue;
            const DraftingRay Ray   = CastDraftingRay(Frame, Pixel[0], Pixel[1]);
            const double      Reach = (Middle[0] - Ray.Origin[0]) * Ray.Direction[0] + (Middle[1] - Ray.Origin[1]) * Ray.Direction[1] + (Middle[2] - Ray.Origin[2]) * Ray.Direction[2];
            const std::optional<DraftingPick> Face = PickDraftingScene(Scene, Ray, 0.0, false, Section);
            VisibleCondition = !Face || Face->Distance >= Reach - Slack;
        }
        if (VisibleCondition) Enclosed.push_back(EdgeIndex);
    }
    return Enclosed;
}

const char* DraftingViewLabel(DraftingViewPreset Preset) noexcept
{
    switch (Preset)
    {
    case DraftingViewPreset::Isometric: return "Isometric";
    case DraftingViewPreset::Top:       return "Top";
    case DraftingViewPreset::Bottom:    return "Bottom";
    case DraftingViewPreset::Front:     return "Front";
    case DraftingViewPreset::Back:      return "Back";
    case DraftingViewPreset::Right:     return "Right";
    case DraftingViewPreset::Left:      return "Left";
    }
    return "View";
}

} // namespace Frontier::DraftingWorkspace
