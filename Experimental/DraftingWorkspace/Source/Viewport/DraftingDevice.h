/*====================================================================================================================================
                                                        DRAFTINGDEVICE.H
====================================================================================================================================*/
// 🧩 Vulkan 1.3 device provisioning (volk; system loader or an explicit driver such as SwiftShader), buffers and images

#pragma once

//------------------------------------------------------------------------------------------------------------------------
//                                                    INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include <volk.h>

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Frontier::DraftingWorkspace
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    STRUCTS
//------------------------------------------------------------------------------------------------------------------------

struct DraftingDeviceSettings
{
    std::string              DriverPath          = {};                                       // [-] - ICD library loaded directly (empty: system loader)
    std::vector<std::string> InstanceExtensions  = {};                                       // [-] - Extra (e.g. GLFW surface extensions)
    bool                     PresentationEnabled = false;                                    // [-] - Enable VK_KHR_swapchain on the device
    bool                     ValidationEnabled   = false;                                    // [-] - VK_LAYER_KHRONOS_validation when present
};

struct DraftingBuffer
{
    VkBuffer       Buffer = VK_NULL_HANDLE;                                                  // [-] - Buffer
    VkDeviceMemory Memory = VK_NULL_HANDLE;                                                  // [-] - Backing allocation
    VkDeviceSize   Size   = 0;                                                               // [B] - Capacity
    void*          Mapped = nullptr;                                                         // [-] - Persistent host mapping (host-visible only)
};

struct DraftingImage
{
    VkImage        Image  = VK_NULL_HANDLE;                                                  // [-] - Image
    VkDeviceMemory Memory = VK_NULL_HANDLE;                                                  // [-] - Backing allocation
    VkImageView    View   = VK_NULL_HANDLE;                                                  // [-] - Whole-image view
    VkFormat       Format = VK_FORMAT_UNDEFINED;                                             // [-] - Texel format
    uint32_t       Width  = 0;                                                               // [px] - Extent
    uint32_t       Height = 0;                                                               // [px] - Extent
};

//------------------------------------------------------------------------------------------------------------------------
//                                                    DEVICE
//------------------------------------------------------------------------------------------------------------------------

// 📝 One graphics queue, one command pool, synchronous helpers. Vulkan 1.3 core: dynamic rendering + synchronization2.
class DraftingDevice
{
public:
    DraftingDevice() = default;
    DraftingDevice(const DraftingDevice&)            = delete;
    DraftingDevice& operator=(const DraftingDevice&) = delete;
    ~DraftingDevice();

    // Returns false with Refusal filled (no loader, no driver, no 1.3 device, …).
    bool Open(const DraftingDeviceSettings& Settings, std::string& Refusal);
    void Close();

    [[nodiscard]] uint32_t FindMemoryType(uint32_t TypeBits, VkMemoryPropertyFlags Required) const;
    [[nodiscard]] std::string Describe() const;

    // Records with Record into a one-time command buffer, submits it and waits for completion.
    void SubmitImmediately(const std::function<void(VkCommandBuffer)>& Record) const;

    [[nodiscard]] DraftingBuffer CreateBuffer(VkDeviceSize Size, VkBufferUsageFlags Usage, bool HostVisibleCondition) const;
    void                         DestroyBuffer(DraftingBuffer& Buffer) const;
    [[nodiscard]] DraftingImage  CreateImage(uint32_t Width, uint32_t Height, VkFormat Format, VkImageUsageFlags Usage, VkImageAspectFlags Aspect) const;
    void                         DestroyImage(DraftingImage& Image) const;

    VkInstance                       Instance       = VK_NULL_HANDLE;
    VkPhysicalDevice                 Physical       = VK_NULL_HANDLE;
    VkDevice                         Device         = VK_NULL_HANDLE;
    VkQueue                          Queue          = VK_NULL_HANDLE;
    uint32_t                         QueueFamily    = 0;
    VkCommandPool                    CommandPool    = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties       Properties     = {};
    VkPhysicalDeviceMemoryProperties MemoryLayout   = {};
    uint32_t                         ApiVersion     = VK_API_VERSION_1_3;

private:
    void* DriverLibrary = nullptr;                                                           // [-] - dlopen / LoadLibrary token of DriverPath
};

// Image layout transition (synchronization2, whole image, one aspect).
void TransitionDraftingImage(VkCommandBuffer Commands, VkImage Image, VkImageAspectFlags Aspect, VkImageLayout From, VkImageLayout To);

} // namespace Frontier::DraftingWorkspace
