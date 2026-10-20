/*====================================================================================================================================
                                                    DRAFTINGRASTERIZATION.H
====================================================================================================================================*/
// 🧩 Viewport rasterization: offscreen colour + R32 identity + depth, surface / edge / grid programs, probe and readback

#pragma once

//------------------------------------------------------------------------------------------------------------------------
//                                                    INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Document/DraftingScene.h"
#include "Viewport/DraftingCamera.h"
#include "Viewport/DraftingDevice.h"

#include <cstdint>
#include <string>
#include <vector>

namespace Frontier::DraftingWorkspace
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    CONSTANTS
//------------------------------------------------------------------------------------------------------------------------

// 📝 Selection-state bits per face / edge (mirrored in Shaders/DraftingProjection.slang).
constexpr uint32_t StateSelected = 1u;
constexpr uint32_t StateActive   = 2u;
constexpr uint32_t StateHovered  = 4u;

//------------------------------------------------------------------------------------------------------------------------
//                                                    STRUCTS
//------------------------------------------------------------------------------------------------------------------------

// 📝 Host mirror of DraftingProjection (128 bytes, column-major float matrices).
struct DraftingProjectionBlock
{
    float ClipFromWorld[16] = {};                                                            // [-]  - Projection × view
    float ViewRight[4]      = {};                                                            // [-]  - w: orthographic flag
    float ViewUp[4]         = {};                                                            // [-]  - Camera up
    float ViewForward[4]    = {};                                                            // [-]  - Eye → target
    float DisplayStroke[4]  = {};                                                            // [px] - Width, height, stroke, depth bias
};
static_assert(sizeof(DraftingProjectionBlock) == 128, "DraftingProjection push constant must stay 128 bytes");

struct DraftingGridBlock
{
    float ClipFromWorld[16] = {};                                                            // [-] - Projection × view
    float WorldFromClip[16] = {};                                                            // [-] - Inverse
};
static_assert(sizeof(DraftingGridBlock) == 128, "DraftingGridProjection push constant must stay 128 bytes");

struct DraftingViewportStyle
{
    float Background[4]   = { 0.090f, 0.106f, 0.137f, 1.0f };                                // [0-1] - SolidArc viewport #171b23
    float EdgeStroke      = 1.5f;                                                            // [px]  - Edge stroke width
    float EdgeDepthBias   = 1.5e-5f;                                                         // [-]   - Clip-depth pull toward the eye
    bool  GridEnabled     = true;                                                            // [-]   - Ground grid
    bool  EdgesEnabled    = true;                                                            // [-]   - B-rep edges
    bool  SurfacesEnabled = true;                                                            // [-]   - B-rep faces
};

//------------------------------------------------------------------------------------------------------------------------
//                                                    RASTERIZATION
//------------------------------------------------------------------------------------------------------------------------

// 💡 Borrowed device; the caller owns submission and fences (as the SultanAladin/Frontier SurfaceRasterization).
//    Record() leaves the colour target in SHADER_READ_ONLY_OPTIMAL for the interface to sample.
class DraftingRasterization
{
public:
    DraftingRasterization() = default;
    DraftingRasterization(const DraftingRasterization&)            = delete;
    DraftingRasterization& operator=(const DraftingRasterization&) = delete;
    ~DraftingRasterization();

    bool Create(const DraftingDevice& Device, std::string& Refusal);
    void Destroy();

    // Recreates the targets when the extent changes (waits for the device first).
    void Resize(uint32_t Width, uint32_t Height);
    void UploadScene(const DraftingScene& Scene);
    void AssignStates(const std::vector<uint32_t>& FaceStates, const std::vector<uint32_t>& EdgeStates);

    // Records the viewport. ProbeX / ProbeY ≥ 0 copy that identity texel; ReadbackCondition copies both whole targets.
    void Record(VkCommandBuffer Commands, const DraftingCameraFrame& Frame, const DraftingViewportStyle& Style, bool ReadbackCondition,
                int ProbeX, int ProbeY);

    // Valid after the recorded work completed.
    [[nodiscard]] uint32_t        ProbedIdentity() const;
    [[nodiscard]] const uint32_t* ReadbackColour() const;                                    // RGBA8, row-major, Width × Height
    [[nodiscard]] const uint32_t* ReadbackIdentity() const;                                  // R32, row-major, Width × Height

    [[nodiscard]] VkImageView ColourView() const { return Colour.View; }
    [[nodiscard]] uint32_t    Width() const { return Colour.Width; }
    [[nodiscard]] uint32_t    Height() const { return Colour.Height; }
    [[nodiscard]] uint64_t    SceneRevision() const { return UploadedRevision; }

private:
    void CreateTargets(uint32_t Width, uint32_t Height);
    void DestroyTargets();
    void CreatePipelines();

    const DraftingDevice* Device = nullptr;

    DraftingImage  Colour            = {};
    DraftingImage  Identity          = {};
    DraftingImage  Depth             = {};
    DraftingBuffer ColourReadback    = {};
    DraftingBuffer IdentityReadback  = {};
    DraftingBuffer ProbeReadback     = {};
    DraftingBuffer CornerBuffer      = {};
    DraftingBuffer SegmentBuffer     = {};
    DraftingBuffer FaceStateBuffer   = {};
    DraftingBuffer EdgeStateBuffer   = {};
    uint32_t       CornerCount       = 0;
    uint32_t       SegmentCount      = 0;
    uint32_t       FaceCount         = 0;
    uint32_t       EdgeCount         = 0;
    uint64_t       UploadedRevision  = UINT64_MAX;

    VkDescriptorSetLayout BufferLayout     = VK_NULL_HANDLE;
    VkDescriptorPool      DescriptorPool   = VK_NULL_HANDLE;
    VkDescriptorSet       BufferSet        = VK_NULL_HANDLE;
    VkPipelineLayout      PipelineLayout   = VK_NULL_HANDLE;
    VkPipeline            SurfacePipeline  = VK_NULL_HANDLE;
    VkPipeline            EdgePipeline     = VK_NULL_HANDLE;
    VkPipeline            GridPipeline     = VK_NULL_HANDLE;
};

} // namespace Frontier::DraftingWorkspace
