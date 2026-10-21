/*====================================================================================================================================
                                                   DRAFTINGWORKSPACEHOST.CPP
====================================================================================================================================*/
// 🧩 Windowed DraftingWorkspace: GLFW window (no client API) + Vulkan swapchain, same interface and frame as the capture

//------------------------------------------------------------------------------------------------------------------------
//                                                    INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Document/DraftingDocumentCodec.h"
#include "Document/DraftingSpecimen.h"
#include "Interface/DraftingFrame.h"
#include "Interface/DraftingInterface.h"
#include "Interface/DraftingOptions.h"
#include "Viewport/DraftingDevice.h"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

using namespace Frontier::DraftingWorkspace;

namespace
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    STRUCTS
//------------------------------------------------------------------------------------------------------------------------

struct HostSwapchain
{
    VkSwapchainKHR           Swapchain = VK_NULL_HANDLE;                                     // [-]  - Presentation chain
    VkFormat                 Format    = VK_FORMAT_UNDEFINED;                                // [-]  - Image format (UNORM, like the capture)
    VkExtent2D               Extent    = {};                                                 // [px] - Image size
    std::vector<VkImage>     Images    = {};                                                 // [-]  - Owned by the swapchain
    std::vector<VkImageView> Views     = {};                                                 // [-]  - One per image
    std::vector<VkSemaphore> Finished  = {};                                                 // [-]  - Render → present, one per image
};

//------------------------------------------------------------------------------------------------------------------------
//                                                    SWAPCHAIN
//------------------------------------------------------------------------------------------------------------------------

void ReleaseSwapchain(const DraftingDevice& Device, HostSwapchain& Chain)
{
    for (VkImageView View : Chain.Views) vkDestroyImageView(Device.Device, View, nullptr);
    for (VkSemaphore Semaphore : Chain.Finished) vkDestroySemaphore(Device.Device, Semaphore, nullptr);
    Chain.Views.clear();
    Chain.Finished.clear();
    Chain.Images.clear();
}

bool BuildSwapchain(const DraftingDevice& Device, VkSurfaceKHR Surface, GLFWwindow* Window, HostSwapchain& Chain)
{
    VkSurfaceCapabilitiesKHR Capabilities{};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(Device.Physical, Surface, &Capabilities);
    uint32_t FormatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(Device.Physical, Surface, &FormatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> Formats(FormatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(Device.Physical, Surface, &FormatCount, Formats.data());
    if (Formats.empty()) return false;
    VkSurfaceFormatKHR Chosen = Formats.front();
    for (const VkSurfaceFormatKHR& Candidate : Formats)
        if ((Candidate.format == VK_FORMAT_B8G8R8A8_UNORM || Candidate.format == VK_FORMAT_R8G8B8A8_UNORM) && Candidate.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            Chosen = Candidate;

    int FramebufferWidth = 0, FramebufferHeight = 0;
    glfwGetFramebufferSize(Window, &FramebufferWidth, &FramebufferHeight);
    VkExtent2D Extent = Capabilities.currentExtent;
    if (Extent.width == UINT32_MAX)
    {
        Extent.width  = std::clamp(uint32_t(FramebufferWidth), Capabilities.minImageExtent.width, Capabilities.maxImageExtent.width);
        Extent.height = std::clamp(uint32_t(FramebufferHeight), Capabilities.minImageExtent.height, Capabilities.maxImageExtent.height);
    }
    if (Extent.width == 0 || Extent.height == 0) return false;
    uint32_t ImageCount = Capabilities.minImageCount + 1;
    if (Capabilities.maxImageCount > 0) ImageCount = std::min(ImageCount, Capabilities.maxImageCount);

    VkSwapchainCreateInfoKHR Request{ VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
    Request.surface          = Surface;
    Request.minImageCount    = ImageCount;
    Request.imageFormat      = Chosen.format;
    Request.imageColorSpace  = Chosen.colorSpace;
    Request.imageExtent      = Extent;
    Request.imageArrayLayers = 1;
    Request.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    Request.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    Request.preTransform     = Capabilities.currentTransform;
    Request.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    Request.presentMode      = VK_PRESENT_MODE_FIFO_KHR;
    Request.clipped          = VK_TRUE;
    Request.oldSwapchain     = Chain.Swapchain;
    VkSwapchainKHR Created   = VK_NULL_HANDLE;
    if (vkCreateSwapchainKHR(Device.Device, &Request, nullptr, &Created) != VK_SUCCESS) return false;
    ReleaseSwapchain(Device, Chain);
    if (Chain.Swapchain != VK_NULL_HANDLE) vkDestroySwapchainKHR(Device.Device, Chain.Swapchain, nullptr);
    Chain.Swapchain = Created;
    Chain.Format    = Chosen.format;
    Chain.Extent    = Extent;

    uint32_t Count = 0;
    vkGetSwapchainImagesKHR(Device.Device, Chain.Swapchain, &Count, nullptr);
    Chain.Images.resize(Count);
    vkGetSwapchainImagesKHR(Device.Device, Chain.Swapchain, &Count, Chain.Images.data());
    for (VkImage Image : Chain.Images)
    {
        VkImageViewCreateInfo ViewRequest{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        ViewRequest.image            = Image;
        ViewRequest.viewType         = VK_IMAGE_VIEW_TYPE_2D;
        ViewRequest.format           = Chain.Format;
        ViewRequest.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        VkImageView View = VK_NULL_HANDLE;
        vkCreateImageView(Device.Device, &ViewRequest, nullptr, &View);
        Chain.Views.push_back(View);
        VkSemaphoreCreateInfo SemaphoreRequest{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
        VkSemaphore Semaphore = VK_NULL_HANDLE;
        vkCreateSemaphore(Device.Device, &SemaphoreRequest, nullptr, &Semaphore);
        Chain.Finished.push_back(Semaphore);
    }
    return true;
}

void ReportGlfwFailure(int Code, const char* Description)
{
    std::fprintf(stderr, "GLFW %d: %s\n", Code, Description);
}

} // namespace

//------------------------------------------------------------------------------------------------------------------------
//                                                    ENTRY POINT
//------------------------------------------------------------------------------------------------------------------------

int main(int Count, char** Arguments)
{
    std::string OptionsPath = "DraftingWorkspace.toml";
    std::string OpenPath;
    std::string CurvesPath;
    bool        Specimen = false;
    for (int Index = 1; Index < Count; ++Index)
    {
        const std::string Flag = Arguments[Index];
        if (Flag == "--options" && Index + 1 < Count) OptionsPath = Arguments[++Index];
        else if (Flag == "--open" && Index + 1 < Count) OpenPath = Arguments[++Index];
        else if (Flag == "--curves" && Index + 1 < Count) CurvesPath = Arguments[++Index];
        else if (Flag == "--specimen") Specimen = true;
    }
    DraftingOptions Options = LoadDraftingOptions(OptionsPath);
    if (!CurvesPath.empty()) Options.CurvesPath = CurvesPath;                                // Import curves… starts here

    // ① window without a client API (Vulkan presents), instance extensions from GLFW
    glfwSetErrorCallback(ReportGlfwFailure);
    if (!glfwInit())
    {
        std::fprintf(stderr, "no windowing platform (headless build); use DraftingWorkspaceCapture for offscreen frames\n");
        return 77;
    }
    if (!glfwVulkanSupported())
    {
        std::fprintf(stderr, "GLFW reports no Vulkan loader on this platform\n");
        glfwTerminate();
        return 77;
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* Window = glfwCreateWindow(Options.WindowWidth, Options.WindowHeight, "DraftingWorkspace", nullptr, nullptr);
    if (!Window) return 1;
    uint32_t                 ExtensionCount = 0;
    const char**             Extensions     = glfwGetRequiredInstanceExtensions(&ExtensionCount);
    DraftingDeviceSettings   DeviceSettings{ .DriverPath = Options.DriverPath, .PresentationEnabled = true, .ValidationEnabled = Options.ValidationEnabled };
    for (uint32_t Index = 0; Index < ExtensionCount; ++Index) DeviceSettings.InstanceExtensions.emplace_back(Extensions[Index]);
    DraftingDevice Device;
    std::string    Refusal;
    if (!Device.Open(DeviceSettings, Refusal))
    {
        std::fprintf(stderr, "no Vulkan device: %s\n", Refusal.c_str());
        return 77;
    }
    VkSurfaceKHR Surface = VK_NULL_HANDLE;
    if (glfwCreateWindowSurface(Device.Instance, Window, nullptr, &Surface) != VK_SUCCESS)
    {
        std::fprintf(stderr, "window surface refused\n");
        return 1;
    }
    VkBool32 PresentCondition = VK_FALSE;
    vkGetPhysicalDeviceSurfaceSupportKHR(Device.Physical, Device.QueueFamily, Surface, &PresentCondition);
    if (!PresentCondition)
    {
        std::fprintf(stderr, "the graphics queue cannot present to this window\n");
        return 1;
    }
    HostSwapchain Chain;
    if (!BuildSwapchain(Device, Surface, Window, Chain)) return 1;

    // ② ImGui (GLFW input + Vulkan renderer) and the workspace
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ConfigureDraftingInterface(Options);
    ImGui_ImplGlfw_InitForVulkan(Window, true);
    DraftingWorkspaceFrame Frame;
    if (!Frame.Create(Device, Chain.Format, uint32_t(Chain.Images.size()), Refusal))
    {
        std::fprintf(stderr, "workspace frame refused: %s\n", Refusal.c_str());
        return 1;
    }
    DraftingWorkspaceState State;
    InitialiseDraftingWorkspace(State, Options);
    State.DeviceLabel = Device.Describe();
    if (Specimen)
    {
        State.WelcomeOpen = false;
        State.Document    = BuildDraftingSpecimen();
        RefreshDraftingScene(State, false);
        FitDraftingCamera(State.Camera, State.Scene);
    }
    if (!OpenPath.empty())
    {
        const DraftingDocumentFile File = LoadDraftingDocumentFile(OpenPath);
        if (File.Refusal.empty())
        {
            State.WelcomeOpen       = false;
            State.Document          = CreateDraftingDocument(File.Title);
            State.Document.Tree     = File.Tree;
            State.Document.Captions = File.Captions;
            State.Document.ImagePlanes = File.ImagePlanes;
            RegenerateDraftingDocument(State.Document, true);
            RefreshDraftingScene(State, false);
            FitDraftingCamera(State.Camera, State.Scene);
            State.DocumentPath = OpenPath;
        }
    }

    // ③ one frame in flight: fence → conclude previous → interface → acquire → record → submit → present
    VkCommandBufferAllocateInfo Allocation{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    Allocation.commandPool        = Device.CommandPool;
    Allocation.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    Allocation.commandBufferCount = 1;
    VkCommandBuffer Commands = VK_NULL_HANDLE;
    vkAllocateCommandBuffers(Device.Device, &Allocation, &Commands);
    VkSemaphoreCreateInfo SemaphoreRequest{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    VkSemaphore Acquired = VK_NULL_HANDLE;
    vkCreateSemaphore(Device.Device, &SemaphoreRequest, nullptr, &Acquired);
    VkFenceCreateInfo FenceRequest{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    FenceRequest.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    VkFence InFlight = VK_NULL_HANDLE;
    vkCreateFence(Device.Device, &FenceRequest, nullptr, &InFlight);

    bool RebuildCondition = false;
    auto Previous         = std::chrono::steady_clock::now();
    bool FirstFrame       = true;
    while (!glfwWindowShouldClose(Window))
    {
        glfwPollEvents();
        int FramebufferWidth = 0, FramebufferHeight = 0;
        glfwGetFramebufferSize(Window, &FramebufferWidth, &FramebufferHeight);
        if (FramebufferWidth == 0 || FramebufferHeight == 0)
        {
            glfwWaitEvents();
            continue;
        }
        if (RebuildCondition || uint32_t(FramebufferWidth) != Chain.Extent.width || uint32_t(FramebufferHeight) != Chain.Extent.height)
        {
            vkDeviceWaitIdle(Device.Device);
            if (!BuildSwapchain(Device, Surface, Window, Chain)) continue;
            RebuildCondition = false;
        }

        vkWaitForFences(Device.Device, 1, &InFlight, VK_TRUE, UINT64_MAX);
        if (!FirstFrame) Frame.Conclude(State);
        FirstFrame = false;
        const auto Now   = std::chrono::steady_clock::now();
        const double Delta = std::chrono::duration<double>(Now - Previous).count();
        Previous         = Now;

        Frame.PrepareViewport(State);
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        DrawDraftingInterface(State, Frame.ViewportTexture(), Delta);
        ImGui::Render();

        uint32_t ImageIndex = 0;
        const VkResult Acquisition = vkAcquireNextImageKHR(Device.Device, Chain.Swapchain, UINT64_MAX, Acquired, VK_NULL_HANDLE, &ImageIndex);
        if (Acquisition == VK_ERROR_OUT_OF_DATE_KHR)
        {
            RebuildCondition = true;
            FirstFrame       = true;
            continue;
        }
        vkResetFences(Device.Device, 1, &InFlight);
        vkResetCommandBuffer(Commands, 0);
        VkCommandBufferBeginInfo Begin{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        Begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(Commands, &Begin);
        const DraftingFrameTarget Target{ Chain.Images[ImageIndex], Chain.Views[ImageIndex], Chain.Extent.width, Chain.Extent.height, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR };
        Frame.Record(Commands, State, ImGui::GetDrawData(), Target, false);
        vkEndCommandBuffer(Commands);

        const VkPipelineStageFlags WaitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        VkSubmitInfo Submission{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
        Submission.waitSemaphoreCount   = 1;
        Submission.pWaitSemaphores      = &Acquired;
        Submission.pWaitDstStageMask    = &WaitStage;
        Submission.commandBufferCount   = 1;
        Submission.pCommandBuffers      = &Commands;
        Submission.signalSemaphoreCount = 1;
        Submission.pSignalSemaphores    = &Chain.Finished[ImageIndex];
        vkQueueSubmit(Device.Queue, 1, &Submission, InFlight);

        VkPresentInfoKHR Presentation{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
        Presentation.waitSemaphoreCount = 1;
        Presentation.pWaitSemaphores    = &Chain.Finished[ImageIndex];
        Presentation.swapchainCount     = 1;
        Presentation.pSwapchains        = &Chain.Swapchain;
        Presentation.pImageIndices      = &ImageIndex;
        const VkResult Presented = vkQueuePresentKHR(Device.Queue, &Presentation);
        if (Presented == VK_ERROR_OUT_OF_DATE_KHR || Presented == VK_SUBOPTIMAL_KHR) RebuildCondition = true;
        State.FrameMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - Now).count();
    }

    // ④ flush a pending autosave, then release in reverse order
    vkDeviceWaitIdle(Device.Device);
    if (State.AutosaveTimer >= 0.0) SaveDraftingDocument(State.Document, Options.RecoveryPath);
    vkDestroyFence(Device.Device, InFlight, nullptr);
    vkDestroySemaphore(Device.Device, Acquired, nullptr);
    vkFreeCommandBuffers(Device.Device, Device.CommandPool, 1, &Commands);
    Frame.Destroy();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    ReleaseSwapchain(Device, Chain);
    vkDestroySwapchainKHR(Device.Device, Chain.Swapchain, nullptr);
    vkDestroySurfaceKHR(Device.Instance, Surface, nullptr);
    Device.Close();
    glfwDestroyWindow(Window);
    glfwTerminate();
    return 0;
}
