/*============================================================================================================================================
                                                          VECTORICONREPOSITORY.H
============================================================================================================================================*/
// 🧩 Single dedicated shared-memory icon repository and rasterizer for FrontierEditor and all standalone workspace executables.

#pragma once

//------------------------------------------------------------------------------------------------------------------------
//                                                        INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Engine/Components/PixelSpace.h"

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace Frontier
{

//------------------------------------------------------------------------------------------------------------------------
//                                                      ENUMERATIONS
//------------------------------------------------------------------------------------------------------------------------

enum class NavigationIcon : uint8_t
{
    ArrowUp,
    ArrowDown,
    ArrowLeft,
    ArrowRight,
    ChevronUp,
    ChevronDown,
    ChevronLeft,
    ChevronRight,
    ChevronsUp,
    ChevronsDown,
    ChevronsLeft,
    ChevronsRight,
    CornerDownRight,
    CornerDownLeft,
    RotateClockwise,
    RotateCounterClockwise,
    ExpandDiagonal,
    CollapseDiagonal,
    Count,
};

enum class ControlIcon : uint8_t
{
    // Existing ordinals 0..85 preserved intact
    SettingsGear,
    AppearanceSwatches,
    DisplayMonitor,
    InputDevices,
    NotificationsBell,
    WirelessSignal,
    BluetoothSymbol,
    MoonDisturbance,
    VolumeSpeaker,
    SunIllumination,
    SparklesAntiAliasing,
    GaugeRate,
    SlidersQuality,
    VideoRenderScale,
    CloseCross,
    ChevronBack,
    ChevronForward,
    ShieldInput,
    ChevronDown,
    ChevronUp,
    TriangleAlert,
    CircleCheck,
    CircleNotice,
    OctagonAlert,
    EyeVisible,
    EyeHidden,
    LockClosed,
    LockOpen,
    MotionActivity,
    FolderClosed,
    FolderOpen,
    CubeShape,
    SearchGlass,
    PlusAdd,
    TrashDelete,
    LayoutSplit,
    LayoutPanelLeft,
    LayoutPanelRight,
    CameraBody,
    LayersSlabs,
    SaveDisk,
    ExternalOpen,
    LightbulbHint,
    QuestionCircle,
    WrenchRepair,
    RefreshCycle,
    LayoutGrid,
    LayoutList,
    LayoutColumns,
    LayoutRows,
    MaterialSphere,
    PatternTile,
    StampTool,
    BrushTool,
    ModifierStar,
    NoiseWave,
    FolderPlus,
    MaskCutout,
    CopyDuplicate,
    DragGrip,
    VectorPointer,
    VectorCurve,
    VectorPen,
    EditFocus,
    PointGradient,
    ShapeSquare,
    ShapeCircle,
    ShapeDiamond,
    ShapeTriangle,
    ShapeStar,
    ShapeHexagon,
    ShapeStripe,
    TextType,
    HandPan,
    ImageTile,
    UndoArrow,
    RedoArrow,
    DownloadExport,
    EraserTool,
    FillBucket,
    PipetteDropper,
    MoveArrows,
    UploadImport,
    BakeBox,
    VolumeBoxes,
    PinAnchor,

    // Unified workspace, transport, viewport, particle, fracture, and UV rail icons (86..)
    CheckMark,
    CheckCircleFilled,
    PlaySolid,
    PauseSolid,
    SkipToStartSolid,
    StepForwardSolid,
    ResetArc,
    EditPencilSolid,
    SimulateFlaskSolid,
    ConstructCubePlus,
    FocusReticleSolid,
    FocusBoundsCorners,
    SplitViewportSolid,
    DiagnosticsBarsSolid,
    ViewportGearSolid,
    FilterSliders,
    FilterFunnel,
    LightningBoltSolid,
    DiceIsometric,
    SeedsNetwork,
    ArrowNorthEast,
    WireCubeIsometric,
    WireSphere,
    GridAxes,
    PulseWave,
    TriangleWire,
    SparkleFourPoint,
    WindStreamlines,
    MinimizeUnderscore,
    MaximizeSquare,
    LogDrawerLines,
    MinimapColumns,
    CursorPointer,
    MarqueeDashed,
    CircleSelectBrush,
    LassoFreehand,
    RotateOrbit,
    ScaleDiagonal,
    ScissorsCut,
    SeamKnife,
    StitchWeld,
    RelaxWave,
    OrbitRings,
    TargetBullseye,
    BallTrajectory,
    CrosshairProbe,
    TurntableOrbit,

    Count,
};

enum class PrimitiveShapeIcon : uint8_t
{
    Box       = 0u,
    Sphere    = 1u,
    Cylinder  = 2u,
    Cone      = 3u,
    Capsule   = 4u,
    Torus     = 5u,
    Pipe      = 6u,
    Pane      = 7u,
    HexColumn = 8u,
    Count     = 9u,
};

enum class FractureAlgorithmIcon : uint8_t
{
    Voronoi    = 0u,
    Uniform    = 1u,
    Radial     = 2u,
    PlanarCuts = 3u,
    Brick      = 4u,
    Split      = 5u,
    Count      = 6u,
};

enum class CapabilityIcon : uint8_t
{
    Visibility    = 0u,
    ShadowCaster  = 1u,
    DynamicMotion = 2u,
    TransformLock = 3u,
    Count         = 4u,
};

enum class ComponentModeIcon : uint8_t
{
    Vertex = 0u,
    Edge   = 1u,
    Face   = 2u,
    Island = 3u,
    Count  = 4u,
};

enum class StudioIcon : uint8_t
{
    ShowcaseFolder   = 0u,
    GenericFolder    = 1u,
    SceneFolder      = 2u,
    CollectionCubes  = 3u,
    CameraOptics     = 4u,
    GeometryCube     = 5u,
    StackedLayers    = 6u,
    PointLuminaire   = 7u,
    ForceStream      = 8u,
    MaterialSphere   = 9u,
    MaterialCutCube  = 10u,
    SimulateGearPlay = 11u,
    BakeGrid         = 12u,
    Count            = 13u,
};

enum class VectorIconAlignment : uint8_t
{
    ViewBoxCentered   = 0u, // Centers the [0..ViewBox] square cleanly inside target bounds
    GeometricCentered = 1u, // Centers the exact compiled ink bounding box onto target bounds center
};

//------------------------------------------------------------------------------------------------------------------------
//                                                       STRUCTURES
//------------------------------------------------------------------------------------------------------------------------

struct VectorIconEntry
{
    std::string_view Name;
    std::string_view SvgPath;
    float            ViewBox        = 24.0f;
    float            StrokeWidth    = 2.0f;
    bool             Filled         = false;
    std::string_view CutoutSvgPath  = {};
    bool             CutoutFilled   = true;
    std::string_view AccentSvgPath  = {};
    bool             AccentFilled   = true;
    float            AccentOpacity  = 1.0f;
    float            OpticalOffsetX = 0.0f;
    float            OpticalOffsetY = 0.0f;
};

struct VectorIconPlacement
{
    float               X              = 0.0f;
    float               Y              = 0.0f;
    float               Size           = 20.0f;
    ColorRgba           Ink            = { 240u, 240u, 240u, 255u };
    float               StrokeWidth    = 2.0f;
    ColorRgba           InteriorCutout = { 24u, 24u, 26u, 255u };
    VectorIconAlignment Alignment      = VectorIconAlignment::ViewBoxCentered;
    bool                AbsoluteStroke = false;
};

struct VectorIconStorageMetrics
{
    std::size_t BuiltinIconCount   = 0u;
    std::size_t CachedContourCount = 0u;
    std::size_t CachedVertexCount  = 0u;
    std::size_t SharedMemoryBytes  = 0u;
    std::size_t DynamicCachedPaths = 0u;
    std::size_t DynamicCacheHits   = 0u;
    std::size_t DynamicCacheMisses = 0u;
};

//------------------------------------------------------------------------------------------------------------------------
//                                                  VECTORICONREPOSITORY
//------------------------------------------------------------------------------------------------------------------------

class VectorIconRepository final
{
public:
    struct Contour
    {
        std::vector<Point2D> Points;
        bool                 Closed = false;
    };

    // Unified square-fitting & centering computation shared by every icon across Editor and standalone apps.
    [[nodiscard]] static Rectangle2D ComputeFittedSquare(Rectangle2D Bounds, float ExplicitSize = 0.0f) noexcept;

    [[nodiscard]] static const VectorIconEntry& QueryNavigation(NavigationIcon Icon) noexcept;
    [[nodiscard]] static const VectorIconEntry& QueryControl(ControlIcon Icon) noexcept;
    [[nodiscard]] static bool                   QueryNavigationBounds(NavigationIcon Icon, Point2D& OutMin, Point2D& OutMax) noexcept;
    [[nodiscard]] static bool                   QueryControlBounds(ControlIcon Icon, Point2D& OutMin, Point2D& OutMax) noexcept;
    [[nodiscard]] static VectorIconStorageMetrics QuerySharedStorageMetrics() noexcept;

    [[nodiscard]] static std::vector<Contour> FlattenSvgPath(std::string_view SvgPath) noexcept;

    static void StrokeSvgPath(
        PixelSpace&                Surface,
        std::string_view           SvgPath,
        const VectorIconPlacement& Placement,
        float                      ViewBox = 24.0f) noexcept;

    static void FillSvgPath(
        PixelSpace&                Surface,
        std::string_view           SvgPath,
        const VectorIconPlacement& Placement,
        float                      ViewBox = 24.0f) noexcept;

    static void ConstructNavigation(
        PixelSpace&                Surface,
        NavigationIcon             Icon,
        const VectorIconPlacement& Placement) noexcept;

    static void ConstructNavigationInBounds(
        PixelSpace&         Surface,
        NavigationIcon      Icon,
        Rectangle2D         Bounds,
        ColorRgba           Ink,
        float               ExplicitSize   = 0.0f,
        float               ScreenStrokePx = 0.0f,
        VectorIconAlignment Alignment      = VectorIconAlignment::ViewBoxCentered) noexcept;

    static void ConstructControl(
        PixelSpace&                Surface,
        ControlIcon                Icon,
        const VectorIconPlacement& Placement) noexcept;

    static void ConstructControlInBounds(
        PixelSpace&         Surface,
        ControlIcon         Icon,
        Rectangle2D         Bounds,
        ColorRgba           Ink,
        float               ExplicitSize   = 0.0f,
        float               ScreenStrokePx = 0.0f,
        VectorIconAlignment Alignment      = VectorIconAlignment::ViewBoxCentered,
        ColorRgba           InteriorCutout = { 24u, 24u, 26u, 255u }) noexcept;

    static void ConstructControlCentered(
        PixelSpace&         Surface,
        ControlIcon         Icon,
        Point2D             Center,
        float               Size,
        ColorRgba           Ink,
        float               ScreenStrokePx = 0.0f,
        VectorIconAlignment Alignment      = VectorIconAlignment::ViewBoxCentered,
        ColorRgba           InteriorCutout = { 24u, 24u, 26u, 255u }) noexcept;

    static void ConstructPrimitiveInBounds(
        PixelSpace&        Surface,
        PrimitiveShapeIcon Shape,
        Rectangle2D        Bounds,
        float              Opacity = 1.0f) noexcept;

    [[nodiscard]] static ColorRgba QueryForceModelTint(uint32_t ForceModelOrdinal) noexcept;

    static void ConstructForceModelInBounds(
        PixelSpace& Surface,
        uint32_t    ForceModelOrdinal,
        Rectangle2D Bounds,
        ColorRgba   AccentInk,
        float       Opacity = 1.0f) noexcept;

    static void ConstructFractureAlgorithmInBounds(
        PixelSpace&           Surface,
        FractureAlgorithmIcon Algorithm,
        Rectangle2D           Bounds,
        ColorRgba             Ink) noexcept;

    static void ConstructCapabilityInBounds(
        PixelSpace&    Surface,
        CapabilityIcon Capability,
        Rectangle2D    Bounds,
        ColorRgba      Ink,
        ColorRgba      InteriorCutout) noexcept;

    static void ConstructComponentModeInBounds(
        PixelSpace&       Surface,
        ComponentModeIcon Mode,
        Rectangle2D       Bounds,
        ColorRgba         Ink) noexcept;

    static void ConstructStudioInBounds(
        PixelSpace& Surface,
        StudioIcon  Icon,
        Rectangle2D Bounds,
        ColorRgba   PrimaryTint    = { 174u, 204u, 174u, 255u },
        float       Opacity        = 1.0f,
        ColorRgba   InteriorCutout = { 24u, 24u, 26u, 255u }) noexcept;
};

} // namespace Frontier
