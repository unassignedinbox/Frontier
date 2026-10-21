/*====================================================================================================================================
                                                    DRAFTINGRASTERIZATION.H
====================================================================================================================================*/
// 🧩 Viewport rasterization: offscreen colour + R32 identity + depth, surface / edge / grid programs, probe and readback

#pragma once

//------------------------------------------------------------------------------------------------------------------------
//                                                    INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Document/DraftingImagePlane.h"
#include "Document/DraftingScene.h"
#include "Viewport/DraftingCamera.h"
#include "Viewport/DraftingDevice.h"

#include <array>
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
    float ViewUp[4]         = {};                                                            // [-]  - Camera up, w: zebra stripe count (0 off)
    float ViewForward[4]    = {};                                                            // [-]  - Eye → target
    float DisplayStroke[4]  = {};                                                            // [px] - Width, height, stroke, depth bias
};
static_assert(sizeof(DraftingProjectionBlock) == 128, "DraftingProjection push constant must stay 128 bytes");

struct DraftingGridBlock
{
    float PlaneFromClip[16]  = {};                                                           // [-]   - Grid coordinates (u, v, n) ← clip
    float AlongU[4]          = {};                                                           // [0-1] - Colour of the axis along u
    float AlongV[4]          = {};                                                           // [0-1] - Colour of the axis along v
    float Reserved[4]        = {};                                                           // [-]   - Padding to 128 bytes
    float ReservedSecond[4]  = {};                                                           // [-]   - Padding to 128 bytes
};
static_assert(sizeof(DraftingGridBlock) == 128, "DraftingGridProjection push constant must stay 128 bytes");

struct DraftingImagePlaneBlock
{
    float ClipFromWorld[16] = {};                                                            // [-]   - Projection × view
    float Corner[4]         = {};                                                            // [len] - Crop top-left, w: opacity
    float Right[4]          = {};                                                            // [len] - Crop width span, w: mode (0 line art)
    float Down[4]           = {};                                                            // [len] - Crop height span
    float Crop[4]           = {};                                                            // [0-1] - u0, v0, u1, v1
};
static_assert(sizeof(DraftingImagePlaneBlock) == 128, "DraftingImagePlaneProjection push constant must stay 128 bytes");

// One image plane to draw this frame: the uploaded picture it samples and its world parallelogram.
struct DraftingImagePlaneDraw
{
    uint32_t              Picture = 0;                                                       // [idx] - Into the last UploadPictures list
    std::array<double, 3> Corner  = {};                                                      // [len] - Crop top-left (world)
    std::array<double, 3> Right   = {};                                                      // [len] - Crop width span
    std::array<double, 3> Down    = {};                                                      // [len] - Crop height span
    std::array<float, 4>  Crop    = { 0.0f, 0.0f, 1.0f, 1.0f };                              // [0-1] - Picture uv rectangle
    float                 Opacity = 0.6f;                                                    // [0-1] - Ink opacity
    float                 Mode    = 0.0f;                                                    // [-]   - 0 line art, 1 photograph
};

struct DraftingViewportStyle
{
    float Background[4]   = { 0.090f, 0.106f, 0.137f, 1.0f };                                // [0-1] - SolidArc viewport #171b23
    float EdgeStroke      = 1.5f;                                                            // [px]  - Edge stroke width
    float EdgeDepthBias   = 1.5e-5f;                                                         // [-]   - Clip-depth pull toward the eye
    bool  GridEnabled     = true;                                                            // [-]   - Drawing-plane grid
    int   GridPlane       = 0;                                                               // [-]   - 0 XY (Top/Bottom), 1 XZ (Front/Back), 2 YZ (Right/Left)
    bool  EdgesEnabled    = true;                                                            // [-]   - B-rep edges
    bool  SurfacesEnabled = true;                                                            // [-]   - B-rep faces
    float ZebraStripes    = 0.0f;                                                            // [-]   - Zebra stripes over the reflection angle (0 off, V8)
    std::vector<DraftingImagePlaneDraw> ImagePlanes = {};                                    // [-]   - Blueprint references, depth-tested behind the model
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
    // Replaces every image-plane texture (pictures with a refusal or no pixels get a 1 × 1 placeholder). Waits for the device.
    void UploadPictures(const std::vector<DraftingPicture>& Pictures);
    [[nodiscard]] size_t PictureCount() const { return Pictures.size(); }
    // Replaces the matcap texture array: Layers[k] becomes layer k (the surface program samples layer = face layer − 1).
    // Pictures must share one square extent; none → a 1-layer grey placeholder. Waits for the device.
    void UploadMatcaps(const std::vector<DraftingPicture>& Layers);
    [[nodiscard]] uint32_t MatcapLayerCount() const { return MatcapLayers; }

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
    [[nodiscard]] uint64_t    SceneGeneration() const { return UploadedGeneration; }

private:
    void CreateTargets(uint32_t Width, uint32_t Height);
    void DestroyTargets();
    void CreatePipelines();
    void ReleasePictures();

    struct PictureTexture
    {
        DraftingImage   Image = {};                                                          // [-] - RGBA8 sampled texture
        VkDescriptorSet Set   = VK_NULL_HANDLE;                                              // [-] - Set 1, binding 0
    };

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
    uint64_t       UploadedGeneration  = UINT64_MAX;

    VkDescriptorSetLayout BufferLayout     = VK_NULL_HANDLE;
    VkDescriptorPool      DescriptorPool   = VK_NULL_HANDLE;
    VkDescriptorSet       BufferSet        = VK_NULL_HANDLE;
    VkPipelineLayout      PipelineLayout   = VK_NULL_HANDLE;
    VkPipeline            SurfacePipeline  = VK_NULL_HANDLE;
    VkPipeline            EdgePipeline     = VK_NULL_HANDLE;
    VkPipeline            GridPipeline     = VK_NULL_HANDLE;
    VkDescriptorSetLayout PictureLayout    = VK_NULL_HANDLE;
    VkDescriptorPool      PicturePool      = VK_NULL_HANDLE;
    VkSampler             PictureSampler   = VK_NULL_HANDLE;
    VkPipelineLayout      PlaneLayout      = VK_NULL_HANDLE;
    VkPipeline            PlanePipeline    = VK_NULL_HANDLE;
    std::vector<PictureTexture> Pictures   = {};
    DraftingImage         MatcapArray      = {};                                             // [-] - Matcap layers (2D array)
    VkDescriptorPool      MatcapPool       = VK_NULL_HANDLE;                                 // [-] - Holds MatcapSet only
    VkDescriptorSet       MatcapSet        = VK_NULL_HANDLE;                                 // [-] - Set 1 of the surface program
    uint32_t              MatcapLayers     = 0;                                              // [-] - Uploaded layer count
};

} // namespace Frontier::DraftingWorkspace
