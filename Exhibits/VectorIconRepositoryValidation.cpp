/*============================================================================================================================================
                                                    VECTORICONREPOSITORYVALIDATION.CPP
============================================================================================================================================*/
// 🧩 Validation suite for Frontier::VectorIconRepository: shared compiled icon memory, cache reuse, 100% catalog coverage, and centering invariants

//------------------------------------------------------------------------------------------------------------------------
//                                                        INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Engine/Components/PixelSpace.h"
#include "Engine/Components/VectorIcons/VectorIconRepository.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <cmath>
#include <cstdio>
#include <limits>
#include <string_view>

namespace
{

using Frontier::CapabilityIcon;
using Frontier::ColorRgba;
using Frontier::ComponentModeIcon;
using Frontier::ControlIcon;
using Frontier::FractureAlgorithmIcon;
using Frontier::NavigationIcon;
using Frontier::PixelSpace;
using Frontier::Point2D;
using Frontier::PrimitiveShapeIcon;
using Frontier::Rectangle2D;
using Frontier::StudioIcon;
using Frontier::VectorIconAlignment;
using Frontier::VectorIconEntry;
using Frontier::VectorIconPlacement;
using Frontier::VectorIconRepository;
using Frontier::VectorIconStorageMetrics;

bool VerifyAssertion(bool Condition, const char* Description) noexcept
{
    if (!Condition)
    {
        std::fprintf(stderr, "[VectorIconRepositoryValidation] FAILED: %s\n", Description);
        return false;
    }
    return true;
}

struct VertexExtentSummary
{
    float MinX      = std::numeric_limits<float>::max();
    float MinY      = std::numeric_limits<float>::max();
    float MaxX      = std::numeric_limits<float>::lowest();
    float MaxY      = std::numeric_limits<float>::lowest();
    int   VtxCount  = 0;
    bool  AllFinite = true;
};

[[nodiscard]] VertexExtentSummary MeasureEmittedVertices(const ImDrawList& DrawList, int StartVtx) noexcept
{
    VertexExtentSummary Summary{};
    Summary.VtxCount = DrawList.VtxBuffer.Size - StartVtx;
    for (int Index = StartVtx; Index < DrawList.VtxBuffer.Size; ++Index)
    {
        const ImVec2 Pos = DrawList.VtxBuffer[Index].pos;
        if (!std::isfinite(Pos.x) || !std::isfinite(Pos.y))
        {
            Summary.AllFinite = false;
            continue;
        }
        if (Pos.x < Summary.MinX) Summary.MinX = Pos.x;
        if (Pos.y < Summary.MinY) Summary.MinY = Pos.y;
        if (Pos.x > Summary.MaxX) Summary.MaxX = Pos.x;
        if (Pos.y > Summary.MaxY) Summary.MaxY = Pos.y;
    }
    return Summary;
}

bool ValidateSharedStorageAndCatalog()
{
    const VectorIconStorageMetrics Initial = VectorIconRepository::QuerySharedStorageMetrics();
    if (!VerifyAssertion(Initial.BuiltinIconCount >= 200u, "Shared repository must register all 8 icon catalogs (>= 200 built-in icons)"))
        return false;
    if (!VerifyAssertion(Initial.CachedVertexCount > 2000u && Initial.CachedContourCount > 250u,
            "Shared repository must precompile vertices and contours once in process memory"))
        return false;
    if (!VerifyAssertion(Initial.SharedMemoryBytes > 0u && Initial.SharedMemoryBytes < 512u * 1024u,
            "Shared compiled icon storage must remain compact (< 512 KB)"))
        return false;

    const VectorIconStorageMetrics Repeat = VectorIconRepository::QuerySharedStorageMetrics();
    if (!VerifyAssertion(
            Repeat.BuiltinIconCount == Initial.BuiltinIconCount &&
            Repeat.CachedVertexCount == Initial.CachedVertexCount &&
            Repeat.CachedContourCount == Initial.CachedContourCount,
            "Repeated storage queries must reference the exact same singleton buffers"))
        return false;

    // NavigationIcon full catalog check
    for (uint8_t Idx = 0u; Idx < static_cast<uint8_t>(NavigationIcon::Count); ++Idx)
    {
        const auto             Icon  = static_cast<NavigationIcon>(Idx);
        const VectorIconEntry& Entry = VectorIconRepository::QueryNavigation(Icon);
        Point2D                MinPt{};
        Point2D                MaxPt{};
        if (!VerifyAssertion(!Entry.SvgPath.empty(), "Every NavigationIcon must expose a non-empty SvgPath"))
            return false;
        if (!VerifyAssertion(!VectorIconRepository::FlattenSvgPath(Entry.SvgPath).empty(), "Every NavigationIcon must flatten to non-empty contours"))
            return false;
        if (!VerifyAssertion(VectorIconRepository::QueryNavigationBounds(Icon, MinPt, MaxPt), "Every NavigationIcon must have valid precompiled bounds"))
            return false;
        if (!VerifyAssertion(MaxPt.X >= MinPt.X && MaxPt.Y >= MinPt.Y, "NavigationIcon bounds must be ordered"))
            return false;
    }

    // ControlIcon full catalog check (all 133 entries)
    for (uint8_t Idx = 0u; Idx < static_cast<uint8_t>(ControlIcon::Count); ++Idx)
    {
        const auto             Icon  = static_cast<ControlIcon>(Idx);
        const VectorIconEntry& Entry = VectorIconRepository::QueryControl(Icon);
        Point2D                MinPt{};
        Point2D                MaxPt{};
        if (!VerifyAssertion(!Entry.SvgPath.empty(), "Every ControlIcon must expose a non-empty SvgPath"))
            return false;
        if (!VerifyAssertion(!VectorIconRepository::FlattenSvgPath(Entry.SvgPath).empty(), "Every ControlIcon must flatten to non-empty contours"))
            return false;
        if (!VerifyAssertion(VectorIconRepository::QueryControlBounds(Icon, MinPt, MaxPt), "Every ControlIcon must have valid precompiled bounds"))
            return false;
        if (!VerifyAssertion(MaxPt.X >= MinPt.X && MaxPt.Y >= MinPt.Y, "ControlIcon bounds must be ordered"))
            return false;
    }

    return true;
}

bool ValidateSquareFittingAndCentering(ImDrawList& DrawList)
{
    // 1. ComputeFittedSquare centering invariants across aspect ratios
    const Rectangle2D WideBox{ 10.0f, 20.0f, 80.0f, 30.0f };
    const Rectangle2D FittedWide = VectorIconRepository::ComputeFittedSquare(WideBox);
    if (!VerifyAssertion(
            std::fabs(FittedWide.Width - 30.0f) < 1e-4f &&
            std::fabs(FittedWide.Height - 30.0f) < 1e-4f &&
            std::fabs(FittedWide.Center().X - WideBox.Center().X) < 1e-4f &&
            std::fabs(FittedWide.Center().Y - WideBox.Center().Y) < 1e-4f,
            "ComputeFittedSquare must center square inside landscape container"))
        return false;

    const Rectangle2D TallBox{ 15.0f, 5.0f, 24.0f, 60.0f };
    const Rectangle2D FittedExplicit = VectorIconRepository::ComputeFittedSquare(TallBox, 16.0f);
    if (!VerifyAssertion(
            std::fabs(FittedExplicit.Width - 16.0f) < 1e-4f &&
            std::fabs(FittedExplicit.Height - 16.0f) < 1e-4f &&
            std::fabs(FittedExplicit.Center().X - TallBox.Center().X) < 1e-4f &&
            std::fabs(FittedExplicit.Center().Y - TallBox.Center().Y) < 1e-4f,
            "ComputeFittedSquare with explicit extent must center exactly"))
        return false;

    PixelSpace      Surface{ &DrawList, Point2D{ 0.0f, 0.0f }, 1.0f, nullptr };
    const ColorRgba White{ 240u, 240u, 240u, 255u };

    // 2. Geometric centering of asymmetric chevron places emitted vertex center on target center
    const Rectangle2D AsymmetricHost{ 100.0f, 200.0f, 44.0f, 32.0f };
    const int         BeforeChevron = DrawList.VtxBuffer.Size;
    VectorIconRepository::ConstructControlInBounds(
        Surface, ControlIcon::ChevronForward, AsymmetricHost, White, 18.0f, 1.5f, VectorIconAlignment::GeometricCentered);
    const VertexExtentSummary ChevronExtent = MeasureEmittedVertices(DrawList, BeforeChevron);
    const float               ChevronMidX   = 0.5f * (ChevronExtent.MinX + ChevronExtent.MaxX);
    const float               ChevronMidY   = 0.5f * (ChevronExtent.MinY + ChevronExtent.MaxY);
    if (!VerifyAssertion(
            ChevronExtent.VtxCount > 0 && ChevronExtent.AllFinite &&
            std::fabs(ChevronMidX - AsymmetricHost.Center().X) < 0.75f &&
            std::fabs(ChevronMidY - AsymmetricHost.Center().Y) < 0.75f,
            "GeometricCentered ChevronForward must center its emitted geometry on host bounds center"))
        return false;

    // 3. Optical centering of PlaySolid inside a circle shifts triangle slightly right for visual balance
    const Point2D PlayCenter{ 300.0f, 220.0f };
    const int     BeforePlay = DrawList.VtxBuffer.Size;
    VectorIconRepository::ConstructControlCentered(
        Surface, ControlIcon::PlaySolid, PlayCenter, 24.0f, White, 0.0f, VectorIconAlignment::ViewBoxCentered);
    const VertexExtentSummary PlayExtent = MeasureEmittedVertices(DrawList, BeforePlay);
    const float               PlayMidX   = 0.5f * (PlayExtent.MinX + PlayExtent.MaxX);
    const float               PlayMidY   = 0.5f * (PlayExtent.MinY + PlayExtent.MaxY);
    if (!VerifyAssertion(
            PlayExtent.VtxCount > 0 && PlayExtent.AllFinite &&
            PlayMidX > PlayCenter.X &&
            std::fabs(PlayMidY - PlayCenter.Y) < 0.75f,
            "PlaySolid must apply right optical balance while staying vertically centered"))
        return false;

    return true;
}

bool ValidateAllFamiliesRasterizeAndDynamicCache(ImDrawList& DrawList)
{
    PixelSpace        Surface{ &DrawList, Point2D{ 0.0f, 0.0f }, 1.0f, nullptr };
    const Rectangle2D Box{ 40.0f, 40.0f, 32.0f, 32.0f };
    const ColorRgba   Ink{ 230u, 232u, 238u, 255u };
    const ColorRgba   Cutout{ 24u, 24u, 26u, 255u };

    // Navigation icons
    for (uint8_t Idx = 0u; Idx < static_cast<uint8_t>(NavigationIcon::Count); ++Idx)
    {
        const int Before = DrawList.VtxBuffer.Size;
        VectorIconRepository::ConstructNavigationInBounds(Surface, static_cast<NavigationIcon>(Idx), Box, Ink, 20.0f);
        const VertexExtentSummary Ext = MeasureEmittedVertices(DrawList, Before);
        if (!VerifyAssertion(Ext.VtxCount > 0 && Ext.AllFinite, "Every NavigationIcon must rasterize finite vertices"))
            return false;
    }

    // Control icons
    for (uint8_t Idx = 0u; Idx < static_cast<uint8_t>(ControlIcon::Count); ++Idx)
    {
        const int Before = DrawList.VtxBuffer.Size;
        VectorIconRepository::ConstructControlInBounds(Surface, static_cast<ControlIcon>(Idx), Box, Ink, 18.0f);
        const VertexExtentSummary Ext = MeasureEmittedVertices(DrawList, Before);
        if (!VerifyAssertion(Ext.VtxCount > 0 && Ext.AllFinite, "Every ControlIcon must rasterize finite vertices"))
            return false;
    }

    // Primitive shapes
    for (uint8_t Idx = 0u; Idx < static_cast<uint8_t>(PrimitiveShapeIcon::Count); ++Idx)
    {
        const int Before = DrawList.VtxBuffer.Size;
        VectorIconRepository::ConstructPrimitiveInBounds(Surface, static_cast<PrimitiveShapeIcon>(Idx), Box, 1.0f);
        const VertexExtentSummary Ext = MeasureEmittedVertices(DrawList, Before);
        if (!VerifyAssertion(Ext.VtxCount > 0 && Ext.AllFinite, "Every PrimitiveShapeIcon must rasterize finite vertices"))
            return false;
    }

    // Fracture algorithms
    for (uint8_t Idx = 0u; Idx < static_cast<uint8_t>(FractureAlgorithmIcon::Count); ++Idx)
    {
        const int Before = DrawList.VtxBuffer.Size;
        VectorIconRepository::ConstructFractureAlgorithmInBounds(Surface, static_cast<FractureAlgorithmIcon>(Idx), Box, Ink);
        const VertexExtentSummary Ext = MeasureEmittedVertices(DrawList, Before);
        if (!VerifyAssertion(Ext.VtxCount > 0 && Ext.AllFinite, "Every FractureAlgorithmIcon must rasterize finite vertices"))
            return false;
    }

    // Capabilities
    for (uint8_t Idx = 0u; Idx < static_cast<uint8_t>(CapabilityIcon::Count); ++Idx)
    {
        const int Before = DrawList.VtxBuffer.Size;
        VectorIconRepository::ConstructCapabilityInBounds(Surface, static_cast<CapabilityIcon>(Idx), Box, Ink, Cutout);
        const VertexExtentSummary Ext = MeasureEmittedVertices(DrawList, Before);
        if (!VerifyAssertion(Ext.VtxCount > 0 && Ext.AllFinite, "Every CapabilityIcon must rasterize finite vertices"))
            return false;
    }

    // Component modes
    for (uint8_t Idx = 0u; Idx < static_cast<uint8_t>(ComponentModeIcon::Count); ++Idx)
    {
        const int Before = DrawList.VtxBuffer.Size;
        VectorIconRepository::ConstructComponentModeInBounds(Surface, static_cast<ComponentModeIcon>(Idx), Box, Ink);
        const VertexExtentSummary Ext = MeasureEmittedVertices(DrawList, Before);
        if (!VerifyAssertion(Ext.VtxCount > 0 && Ext.AllFinite, "Every ComponentModeIcon must rasterize finite vertices"))
            return false;
    }

    // Studio icons
    for (uint8_t Idx = 0u; Idx < static_cast<uint8_t>(StudioIcon::Count); ++Idx)
    {
        const int Before = DrawList.VtxBuffer.Size;
        VectorIconRepository::ConstructStudioInBounds(Surface, static_cast<StudioIcon>(Idx), Box, Ink, 1.0f, Cutout);
        const VertexExtentSummary Ext = MeasureEmittedVertices(DrawList, Before);
        if (!VerifyAssertion(Ext.VtxCount > 0 && Ext.AllFinite, "Every StudioIcon must rasterize finite vertices"))
            return false;
    }

    // Force models (all 19 force models)
    for (uint32_t Idx = 0u; Idx < 19u; ++Idx)
    {
        const int Before = DrawList.VtxBuffer.Size;
        VectorIconRepository::ConstructForceModelInBounds(Surface, Idx, Box, Ink);
        const VertexExtentSummary Ext = MeasureEmittedVertices(DrawList, Before);
        if (!VerifyAssertion(Ext.VtxCount > 0 && Ext.AllFinite, "Every ForceModel icon must rasterize finite vertices"))
            return false;
    }

    // Dynamic SVG path cache hit/miss verification
    constexpr std::string_view CustomSvg = "M4 12C4 7.5 7.5 4 12 4S20 7.5 20 12 16.5 20 12 20 4 16.5 4 12ZM9 12L11 14L15 10";
    const VectorIconStorageMetrics BeforeDyn = VectorIconRepository::QuerySharedStorageMetrics();
    VectorIconPlacement Placement{};
    Placement.X           = 10.0f;
    Placement.Y           = 10.0f;
    Placement.Size        = 24.0f;
    Placement.Ink         = Ink;
    Placement.StrokeWidth = 1.5f;

    VectorIconRepository::StrokeSvgPath(Surface, CustomSvg, Placement, 24.0f);
    const VectorIconStorageMetrics AfterFirst = VectorIconRepository::QuerySharedStorageMetrics();
    VectorIconRepository::StrokeSvgPath(Surface, CustomSvg, Placement, 24.0f);
    VectorIconRepository::FillSvgPath(Surface, CustomSvg, Placement, 24.0f);
    const VectorIconStorageMetrics AfterReuse = VectorIconRepository::QuerySharedStorageMetrics();

    if (!VerifyAssertion(
            AfterFirst.DynamicCachedPaths == BeforeDyn.DynamicCachedPaths + 1u &&
            AfterFirst.DynamicCacheMisses == BeforeDyn.DynamicCacheMisses + 1u,
            "First dynamic SVG path must compile and insert once"))
        return false;
    if (!VerifyAssertion(
            AfterReuse.DynamicCachedPaths == AfterFirst.DynamicCachedPaths &&
            AfterReuse.DynamicCacheMisses == AfterFirst.DynamicCacheMisses &&
            AfterReuse.DynamicCacheHits   == AfterFirst.DynamicCacheHits + 2u,
            "Subsequent dynamic SVG calls must reuse compiled contours with zero cache growth"))
        return false;

    return true;
}

} // namespace

int main()
{
    ImGui::CreateContext();
    ImGuiIO& Io = ImGui::GetIO();
    Io.DisplaySize = ImVec2(1280.0f, 720.0f);
    Io.DeltaTime   = 1.0f / 60.0f;
    unsigned char* Pixels = nullptr;
    int            Width  = 0;
    int            Height = 0;
    Io.Fonts->GetTexDataAsRGBA32(&Pixels, &Width, &Height);

    ImGui::NewFrame();
    ImDrawList* ForegroundList = ImGui::GetForegroundDrawList();

    bool Passed = (ForegroundList != nullptr);
    Passed = Passed && ValidateSharedStorageAndCatalog();
    Passed = Passed && ValidateSquareFittingAndCentering(*ForegroundList);
    Passed = Passed && ValidateAllFamiliesRasterizeAndDynamicCache(*ForegroundList);
    ImGui::Render();

    const VectorIconStorageMetrics FinalMetrics = VectorIconRepository::QuerySharedStorageMetrics();
    std::printf(
        "[VectorIconRepositoryValidation] %s | BuiltinIcons=%zu CachedVertices=%zu CachedContours=%zu SharedBytes=%zu DynamicCached=%zu Hits=%zu\n",
        Passed ? "PASSED" : "FAILED",
        FinalMetrics.BuiltinIconCount,
        FinalMetrics.CachedVertexCount,
        FinalMetrics.CachedContourCount,
        FinalMetrics.SharedMemoryBytes,
        FinalMetrics.DynamicCachedPaths,
        FinalMetrics.DynamicCacheHits);

    ImGui::DestroyContext();
    return Passed ? 0 : 1;
}
