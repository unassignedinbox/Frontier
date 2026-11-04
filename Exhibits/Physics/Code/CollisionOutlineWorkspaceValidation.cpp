/*==========================================================================================================================================
                                               COLLISIONOUTLINEWORKSPACEVALIDATION.CPP
==========================================================================================================================================*/
// 🧩 Real workspace input and draw commands over a synthetic viewport background, without a GPU.

#include "Engine/Interface/IconPresentation.h"
#include "Engine/Interface/ViewportChromeControls.h"
#include "EverestWorkspace.h"
#include "Exhibits/ControlCentre/Code/CpuRasterization.h"
#include "Frontier/Extensions/Rendering/SceneGeometryOccupancy.h"
#include "Frontier/Extensions/Physics/PhysicsExtension.h"
#include "UI/Editor/ControlCentrePanel.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <stb_image_write.h>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <vector>

namespace
{
void Require(bool Value, const char* Name) { if (!Value) throw std::runtime_error(Name); std::printf("PASS %s\n", Name); }
}
int main(int Count, char** Args)
{
    using namespace Frontier;
    if (Count != 4) return 1;
    try
    {
        const std::filesystem::path Content = std::filesystem::absolute(Args[1]), Project = std::filesystem::absolute(Args[2]), Scratch = Args[3];
        std::filesystem::create_directories(Scratch);
        std::filesystem::copy_file(Project / "Content/Levels/Default.levelspace", Scratch / "Default.levelspace", std::filesystem::copy_options::overwrite_existing);
        ImGui::CreateContext();
        ImGuiIO& IO = ImGui::GetIO();
        IO.DisplaySize = {1920,1080}; IO.DeltaTime = 1.0f/30.0f; IO.IniFilename = nullptr;
        IO.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        IO.BackendFlags |= ImGuiBackendFlags_RendererHasTextures | ImGuiBackendFlags_RendererHasVtxOffset;
        ImGui::GetPlatformIO().Renderer_TextureMaxWidth = 4096; ImGui::GetPlatformIO().Renderer_TextureMaxHeight = 4096;
        IconPresentation::Attach(Content / "Icons");
        {
            ActivationOwnership Ownership(new ControlCentrePanel(Content / "FontArchives", Scratch / "Personal.projectspace", Project / "Everest.projectspace"));
            auto* Centre = static_cast<ControlCentrePanel*>(Ownership.get());
            Require(Centre->Initialize() && Centre->Activate(), "control centre initialized");
            Everest::EverestWorkspace Workspace(*Centre, Project);
            std::string Error;
            Require(Workspace.QueryWorldSpace().LoadLevelSpace(Scratch / "Default.levelspace", &Error), "scratch level loaded");
            GlobalIllumination::SceneStructure Scene;
            const auto Archive = Content / "EnvironmentArchives/ProvingGround";
            Require(Scene.LoadFacetSpace(Archive / "ProvingGround.facetspace", &Error), "proving-ground archive loaded");
            std::vector<uint8_t> Materials;
            if (std::ifstream Stream(Archive / "Materials.bin", std::ios::binary); Stream)
                Materials.assign(std::istreambuf_iterator<char>(Stream), std::istreambuf_iterator<char>());
            SceneGeometryOccupancy Occupancy;
            Occupancy.Reset(&Scene, Materials);
            constexpr ImTextureID ViewportTag = 0x5ce7e5;
            Workspace.PresentSceneViewport = [&](const SceneViewportRequest& Request) {
                (void)Occupancy.ResolvePlacements(Workspace.QueryWorldSpace());
                SceneViewportImage Image;
                Image.Texture = ViewportTag; Image.Readout.Presenting = true;
                Image.Readout.Geometry = Occupancy.QueryResolutions();
                Image.Readout.GeometryGeneration = Occupancy.QueryGeneration();
                if (Request.CaptureCollisions) Image.Readout.CollisionSurfaces = Occupancy.CaptureCollisionSurfaces(Workspace.QueryWorldSpace());
                return Image;
            };
            Workspace.AccessViewportFlight().Position = {15, -12, 7};
            Workspace.AccessViewportFlight().Heading = 56;
            Workspace.AccessViewportFlight().Elevation = -20;
            auto Tick = [&]() {
                Centre->PrepareInterface(); ImGui::NewFrame(); (void)Centre->Integrate(1.0/30.0);
                (void)Workspace.ConfigurePlatform(); Workspace.ConstructInterface(1.0/30.0); Centre->ConstructInterface();
                ImGui::Render(); FrontierExhibit::AcknowledgeTextures();
            };
            for (int I=0; I<3; ++I) Tick();
            Workspace.ToggleSimulation(); Tick();
            Require(Workspace.AccessPhysics() && Workspace.AccessPhysics()->QueryRunning(), "simulation starts through facet capture");
            Require(!Workspace.QueryCollisionOutlinesVisible(), "outlines initially off");
            ImGuiWindow* Viewport = nullptr;
            for (ImGuiWindow* Window : ImGui::GetCurrentContext()->Windows)
                if (std::string(Window->Name).find("Viewport") != std::string::npos && Window->WasActive && Window->Size.x > 300) { Viewport=Window; break; }
            Require(Viewport != nullptr, "viewport window found");
            const float Scale = Centre->QueryAppearance().QueryApplied().InterfaceScale / 100.0f;
            const float Right = Viewport->WorkRect.Max.x - 12*Scale;
            const float Button = 34*Scale, Gap = 8*Scale;
            const ImVec2 Point(Right-4*Button-4*Gap-132*Scale+Button*.5f,
                               Viewport->WorkRect.Min.y + Frontier::ViewportChromeControls::BarHeight*Scale*.5f);
            IO.AddMousePosEvent(Point.x, Point.y); Tick(); IO.AddMouseButtonEvent(0,true); Tick(); IO.AddMouseButtonEvent(0,false); Tick();
            Require(Workspace.QueryCollisionOutlinesVisible(), "viewport button turns outlines on");
            Tick(); // 📝 The viewport is drawn before its bar handles the click.
            const ImDrawData* Data = ImGui::GetDrawData();
            size_t OutlineVertices = 0;
            for (const ImDrawList* List : Data->CmdLists)
                for (const ImDrawVert& Vertex : List->VtxBuffer)
                    if (Vertex.col == IM_COL32(66, 224, 204, 210)) ++OutlineVertices;
            Require(OutlineVertices > 0, "visible fixed triangle outline commands generated");
            std::vector<unsigned char> Pixels(1920u*1080u*3u, 14u);
            std::vector<unsigned char> Background(4u*4u*4u, 38u);
            for (size_t I=3; I<Background.size(); I+=4) Background[I]=255;
            const std::function<FrontierExhibit::Image(ImTextureID)> Resolve = [&](ImTextureID T) {
                return T == ViewportTag ? FrontierExhibit::Image{Background.data(),4,4,4} : FrontierExhibit::Image{};
            };
            for (const ImDrawList* List : Data->CmdLists)
                FrontierExhibit::RasterizeCommands(List, Pixels.data(), 1920,1080,Data->DisplayPos,Data->FramebufferScale,nullptr,&Resolve);
            Require(stbi_write_png((Scratch / "CollisionOutlineWorkspace.png").string().c_str(),1920,1080,3,Pixels.data(),1920*3)!=0, "workspace draw commands captured");
            IO.AddMouseButtonEvent(0,true); Tick(); IO.AddMouseButtonEvent(0,false); Tick();
            Require(!Workspace.QueryCollisionOutlinesVisible(), "viewport button turns outlines off");
            IO.AddKeyEvent(ImGuiMod_Alt, true); IO.AddKeyEvent(ImGuiKey_LeftAlt, true); IO.AddKeyEvent(ImGuiKey_C, true); Tick();
            Require(Workspace.QueryCollisionOutlinesVisible(), "Alt+C turns collision outlines on");
            IO.AddKeyEvent(ImGuiKey_C, false); IO.AddKeyEvent(ImGuiKey_LeftAlt, false); IO.AddKeyEvent(ImGuiMod_Alt, false); Tick();
            Workspace.ToggleSimulation();
        }
        ImGui::DestroyContext();
        return 0;
    }
    catch (const std::exception& Error) { std::printf("FAIL %s\n", Error.what()); return 1; }
}
