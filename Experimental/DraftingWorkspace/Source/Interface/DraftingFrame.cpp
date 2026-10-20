/*====================================================================================================================================
                                                       DRAFTINGFRAME.CPP
====================================================================================================================================*/
// 🧩 One workspace frame on Vulkan: viewport raster → sampled texture → ImGui (dynamic rendering) into any colour target

//------------------------------------------------------------------------------------------------------------------------
//                                                    INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Interface/DraftingFrame.h"

#include <imgui.h>
#include <imgui_impl_vulkan.h>

#include <algorithm>
#include <cmath>

namespace Frontier::DraftingWorkspace
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    PUBLIC FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

DraftingWorkspaceFrame::~DraftingWorkspaceFrame()
{
    Destroy();
}

bool DraftingWorkspaceFrame::Create(const DraftingDevice& OpenDevice, VkFormat TargetFormat, uint32_t ImageCount, std::string& Refusal)
{
    Device = &OpenDevice;
    if (!Raster.Create(OpenDevice, Refusal)) return false;

    ImGui_ImplVulkan_InitInfo Setup{};
    Setup.ApiVersion          = OpenDevice.ApiVersion;
    Setup.Instance            = OpenDevice.Instance;
    Setup.PhysicalDevice      = OpenDevice.Physical;
    Setup.Device              = OpenDevice.Device;
    Setup.QueueFamily         = OpenDevice.QueueFamily;
    Setup.Queue               = OpenDevice.Queue;
    Setup.DescriptorPoolSize  = 64;
    Setup.MinImageCount       = std::max<uint32_t>(2, ImageCount);
    Setup.ImageCount          = std::max<uint32_t>(2, ImageCount);
    Setup.UseDynamicRendering = true;
    Setup.PipelineInfoMain.PipelineRenderingCreateInfo                         = { VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
    Setup.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount    = 1;
    Setup.PipelineInfoMain.PipelineRenderingCreateInfo.pColorAttachmentFormats = &TargetFormat;   // deep-copied by the backend
    if (!ImGui_ImplVulkan_Init(&Setup))
    {
        Refusal = "ImGui Vulkan backend refused to initialise";
        return false;
    }
    BackendReady = true;
    return true;
}

void DraftingWorkspaceFrame::Destroy()
{
    if (!Device) return;
    vkDeviceWaitIdle(Device->Device);
    if (Texture != VK_NULL_HANDLE) ImGui_ImplVulkan_RemoveTexture(Texture);
    Texture     = VK_NULL_HANDLE;
    TextureView = VK_NULL_HANDLE;
    if (BackendReady) ImGui_ImplVulkan_Shutdown();
    BackendReady = false;
    Raster.Destroy();
    Device = nullptr;
}

void DraftingWorkspaceFrame::PrepareViewport(DraftingWorkspaceState& State)
{
    const uint32_t Width  = uint32_t(std::max(8.0f, std::round(State.ViewportRect[2])));
    const uint32_t Height = uint32_t(std::max(8.0f, std::round(State.ViewportRect[3])));
    Raster.Resize(Width, Height);
    if (Raster.ColourView() != TextureView)
    {
        // The previous frame has completed (fence / idle), so its descriptor set may be released now.
        if (Texture != VK_NULL_HANDLE) ImGui_ImplVulkan_RemoveTexture(Texture);
        Texture     = ImGui_ImplVulkan_AddTexture(Raster.ColourView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        TextureView = Raster.ColourView();
    }
    const DraftingScene& Scene = DisplayedDraftingScene(State);
    if (Scene.Revision != Raster.SceneRevision()) Raster.UploadScene(Scene);
    const DraftingSceneStates States = DisplayedDraftingStates(State);
    Raster.AssignStates(States.Faces, States.Edges);
    State.Style.EdgeStroke = State.Options.EdgeStroke;
}

void DraftingWorkspaceFrame::Record(VkCommandBuffer Commands, const DraftingWorkspaceState& State, ImDrawData* DrawLists, const DraftingFrameTarget& Target,
                                    bool ReadbackCondition)
{
    // ① viewport raster at its own size; the pointer probe feeds GPU identity hover / picking
    const DraftingCameraFrame Frame = FrameDraftingCamera(State.Camera, Raster.Width(), Raster.Height());
    const ImVec2 Pointer = ImGui::GetIO().MousePos;
    int ProbeX = -1;
    int ProbeY = -1;
    if (State.ViewportHovered && State.ViewportRect[2] > 0.0f && State.ViewportRect[3] > 0.0f)
    {
        ProbeX = int(std::floor((Pointer.x - State.ViewportRect[0]) * float(Raster.Width()) / State.ViewportRect[2]));
        ProbeY = int(std::floor((Pointer.y - State.ViewportRect[1]) * float(Raster.Height()) / State.ViewportRect[3]));
    }
    Raster.Record(Commands, Frame, State.Style, ReadbackCondition, ProbeX, ProbeY);

    // ② ImGui composes every panel (the viewport colour target is sampled as a texture)
    TransitionDraftingImage(Commands, Target.Image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    VkRenderingAttachmentInfo Colour{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
    Colour.imageView   = Target.View;
    Colour.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    Colour.loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR;
    Colour.storeOp     = VK_ATTACHMENT_STORE_OP_STORE;
    Colour.clearValue.color = { { 0.059f, 0.059f, 0.063f, 1.0f } };                          // #0f0f10 page background
    VkRenderingInfo Rendering{ VK_STRUCTURE_TYPE_RENDERING_INFO };
    Rendering.renderArea           = { { 0, 0 }, { Target.Width, Target.Height } };
    Rendering.layerCount           = 1;
    Rendering.colorAttachmentCount = 1;
    Rendering.pColorAttachments    = &Colour;
    vkCmdBeginRendering(Commands, &Rendering);
    if (DrawLists) ImGui_ImplVulkan_RenderDrawData(DrawLists, Commands);
    vkCmdEndRendering(Commands);
    TransitionDraftingImage(Commands, Target.Image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, Target.FinalLayout);
}

void DraftingWorkspaceFrame::Conclude(DraftingWorkspaceState& State) const
{
    State.HoverIdentity = State.ViewportHovered ? Raster.ProbedIdentity() : 0u;
}

} // namespace Frontier::DraftingWorkspace
