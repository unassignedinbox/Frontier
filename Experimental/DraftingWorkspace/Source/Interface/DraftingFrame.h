/*====================================================================================================================================
                                                        DRAFTINGFRAME.H
====================================================================================================================================*/
// 🧩 One workspace frame on Vulkan: viewport raster → sampled texture → ImGui (dynamic rendering) into any colour target

#pragma once

//------------------------------------------------------------------------------------------------------------------------
//                                                    INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Interface/DraftingInterface.h"
#include "Viewport/DraftingDevice.h"
#include "Viewport/DraftingRasterization.h"

#include <volk.h>

#include <string>
#include <vector>

struct ImDrawData;

namespace Frontier::DraftingWorkspace
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    STRUCTS
//------------------------------------------------------------------------------------------------------------------------

// 📝 Where the composed frame goes: a swapchain image (host) or an offscreen image (capture).
struct DraftingFrameTarget
{
    VkImage       Image       = VK_NULL_HANDLE;                                                  // [-]  - Colour image
    VkImageView   View        = VK_NULL_HANDLE;                                                  // [-]  - Its view
    uint32_t      Width       = 0;                                                               // [px] - Extent
    uint32_t      Height      = 0;                                                               // [px] - Extent
    VkImageLayout FinalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;                            // [-]  - PRESENT_SRC_KHR or TRANSFER_SRC_OPTIMAL
};

//------------------------------------------------------------------------------------------------------------------------
//                                                    CLASSES
//------------------------------------------------------------------------------------------------------------------------

class DraftingWorkspaceFrame
{
public:
    DraftingWorkspaceFrame() = default;
    DraftingWorkspaceFrame(const DraftingWorkspaceFrame&)            = delete;
    DraftingWorkspaceFrame& operator=(const DraftingWorkspaceFrame&) = delete;
    ~DraftingWorkspaceFrame();

    // ImGui context must exist (fonts configured). TargetFormat is the composed frame's colour format.
    bool Create(const DraftingDevice& Device, VkFormat TargetFormat, uint32_t ImageCount, std::string& Refusal);
    void Destroy();

    // 💡 Before ImGui::NewFrame: follows the viewport rectangle of the previous frame (resize + texture re-registration),
    //    uploads the displayed scene when its revision changed and assigns the selection / hover state words.
    void PrepareViewport(DraftingWorkspaceState& State);

    // Records the viewport raster (with the pointer probe) and the ImGui draw lists into Target.
    void Record(VkCommandBuffer Commands, const DraftingWorkspaceState& State, ImDrawData* DrawLists, const DraftingFrameTarget& Target, bool ReadbackCondition);

    // After the frame's fence: the identity under the pointer becomes the next frame's hover.
    void Conclude(DraftingWorkspaceState& State) const;

    [[nodiscard]] ImTextureID ViewportTexture() const { return ImTextureID(Texture); }

    DraftingRasterization Raster = {};                                                       // [-] - Viewport targets + pipelines

private:
    const DraftingDevice* Device       = nullptr;                                            // [-] - Borrowed
    VkDescriptorSet       Texture      = VK_NULL_HANDLE;                                     // [-] - ImGui texture of the colour target
    VkImageView           TextureView  = VK_NULL_HANDLE;                                     // [-] - View it was registered with
    bool                  BackendReady = false;                                              // [-] - ImGui_ImplVulkan_Init succeeded
    std::vector<DraftingImage>   Thumbnails      = {};                                       // [-] - Matcap swatches (64 × 64)
    std::vector<VkDescriptorSet> ThumbnailSets   = {};                                       // [-] - Their ImGui textures
    uint64_t                     MatcapGeneration = 0;                                       // [-] - Library generation uploaded
    void ReleaseThumbnails();
    void UploadMatcaps(DraftingWorkspaceState& State);
};

} // namespace Frontier::DraftingWorkspace
