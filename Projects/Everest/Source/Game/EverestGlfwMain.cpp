/*==========================================================================================================================================
                                                            EVERESTGLFWMAIN.CPP
==========================================================================================================================================*/
// 🧩 GLFW interactive Project Everest front-end: the policy selects Lobby navigation-only input or full
//    vehicle driving, while the camera consumes the same packet's mouse deltas and camera-cycle action.
//
//      EverestWindow [--archive DIR]

#include "Frontier/Engine/Input/InputEncoder.h"
#include "Frontier/Engine/Session/LobbySession.h"
#include "Frontier/Game/CameraController.h"
#include "Frontier/Game/SoftwarePresentation.h"
#include "Projects/Everest/Source/Game/SmokeFireEditor.h"
#include "Projects/Everest/Source/Navigation/NavigationApp.h"
#include "Projects/Everest/Source/Session/SessionPolicy.h"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl2.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace
{

void DrawRect(float X, float Y, float Width, float Height, float R, float G, float B, float A)
{
    glColor4f(R, G, B, A);
    glBegin(GL_QUADS);
    glVertex2f(X, Y); glVertex2f(X + Width, Y); glVertex2f(X + Width, Y + Height); glVertex2f(X, Y + Height);
    glEnd();
}

void Present(GLFWwindow* Window, GLuint Texture, const Frontier::PresentationFrame& Frame,
             bool MenuVisible, size_t SelectedDestination, const Everest::NavigationApp& Navigation,
             const Frontier::LobbySession& Game)
{
    int Width = 0, Height = 0;
    glfwGetFramebufferSize(Window, &Width, &Height);
    if (Width <= 0 || Height <= 0)
        return;
    glViewport(0, 0, Width, Height);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glClearColor(0.04f, 0.05f, 0.07f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, 1.0, 1.0, 0.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, Texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, Frame.Width, Frame.Height, 0, GL_RGB, GL_UNSIGNED_BYTE, Frame.Rgb.data());
    glColor4f(1, 1, 1, 1);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex2f(0, 0);
    glTexCoord2f(1, 0); glVertex2f(1, 0);
    glTexCoord2f(1, 1); glVertex2f(1, 1);
    glTexCoord2f(0, 1); glVertex2f(0, 1);
    glEnd();
    glDisable(GL_TEXTURE_2D);

    if (MenuVisible)
    {
        // 📝 A legible, font-free menu marker; exact labels and controls are also shown in the window title.
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        DrawRect(0.025f, 0.035f, 0.30f, 0.20f, 0.02f, 0.04f, 0.06f, 0.86f);
        const auto& Destinations = Navigation.QueryDestinations();
        for (size_t Index = 0; Index < Destinations.size(); ++Index)
        {
            const bool Current = Navigation.IsCurrent(Game, Destinations[Index]);
            const bool Selected = Index == SelectedDestination;
            const float Y = 0.075f + static_cast<float>(Index) * 0.075f;
            DrawRect(0.045f, Y, 0.26f, 0.055f,
                     Current ? 0.05f : (Selected ? 0.12f : 0.18f),
                     Current ? 0.48f : (Selected ? 0.42f : 0.20f),
                     Current ? 0.38f : (Selected ? 0.72f : 0.24f), 0.95f);
        }
        glDisable(GL_BLEND);
    }
}

std::string ResolveControlsTitle(const std::string& Level, const Frontier::InputRoutingPolicy& Policy,
                                 bool MenuVisible, const std::vector<Everest::NavigationDestination>& Destinations,
                                 size_t Selection)
{
    std::string Title = "Project Everest | " + Level + " | " + std::string(Policy.ControlsHint);
    if (MenuVisible)
    {
        Title += " | routes:";
        for (size_t Index = 0; Index < Destinations.size(); ++Index)
            Title += " " + std::to_string(Index + 1) + ") " + Destinations[Index].Label;
        if (Selection < Destinations.size())
            Title += " | selected: " + Destinations[Selection].Label + " (Up/Down, Enter, Esc)";
    }
    return Title;
}

} // namespace

int main(int ArgumentCount, char** Arguments)
{
    using namespace Frontier;
    SessionConfiguration Configuration{};
    std::string ArchiveDirectory = "Experimental/VehicleHmiPrototype/Archive";
    for (int Index = 1; Index < ArgumentCount; ++Index)
        if (std::strcmp(Arguments[Index], "--archive") == 0 && Index + 1 < ArgumentCount)
            ArchiveDirectory = Arguments[++Index];

    const FacetArchiveData Body = LoadFacetArchive(ArchiveDirectory + "/ControlVehicleBody.facet");
    const FacetArchiveData Tyre = LoadFacetArchive(ArchiveDirectory + "/ControlVehicleTyre.facet");
    const FacetArchiveData Rim = LoadFacetArchive(ArchiveDirectory + "/ControlVehicleRim.facet");
    if (!Body.IsValid() || !Tyre.IsValid() || !Rim.IsValid())
    {
        std::fprintf(stderr, "EverestWindow: vehicle facet archives not found or invalid under %s\n", ArchiveDirectory.c_str());
        return 1;
    }

    if (!glfwInit())
    {
        std::fprintf(stderr, "EverestWindow: GLFW initialization failed\n");
        return 1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    GLFWwindow* Window = glfwCreateWindow(1000, 650, "Project Everest", nullptr, nullptr);
    if (!Window)
    {
        std::fprintf(stderr, "EverestWindow: could not create a GLFW/OpenGL window\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(Window);
    glfwSwapInterval(1);
    glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGuiStyle& Style = ImGui::GetStyle();
    Style.WindowRounding = 0.0f;
    Style.FrameRounding = 4.0f;
    Style.ScrollbarRounding = 4.0f;
    Style.WindowPadding = ImVec2(14.0f, 12.0f);
    Style.FramePadding = ImVec2(8.0f, 5.0f);
    Style.ItemSpacing = ImVec2(8.0f, 6.0f);
    Style.Colors[ImGuiCol_WindowBg] = ImVec4(0.086f, 0.086f, 0.086f, 0.97f);
    Style.Colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
    Style.Colors[ImGuiCol_Header] = ImVec4(0.22f, 0.18f, 0.15f, 1.0f);
    Style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.78f, 0.78f, 0.78f, 1.0f);
    Style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.91f, 0.52f, 0.24f, 1.0f);
    ImGui_ImplGlfw_InitForOpenGL(Window, true);
    ImGui_ImplOpenGL2_Init();

    LobbySession Game(Configuration, Everest::ConstructEverestSessionPolicy);
    Everest::NavigationApp Navigation(Configuration);
    InputEncoder Encoder;
    CameraController Camera;
    SmokeFluidSettings SmokeSettings{};
    SmokeFluidSystem Smoke(SmokeSettings);
    Everest::SmokeFireEditor SmokeEditor;
    GLuint Texture = 0;
    glGenTextures(1, &Texture);
    bool MenuVisible = true;
    size_t SelectedDestination = 1;   // default Lobby selection is the next route: Proving Grounds
    std::string LastLevel = Game.QueryCurrentLevelPath();
    double PreviousTime = glfwGetTime();
    static const InputRoutingPolicy FallbackRouting{};
    std::vector<PresentationVehicle> Vehicles(1);
    Vehicles[0].Body = &Body;
    Vehicles[0].Tyre = &Tyre;
    Vehicles[0].Rim = &Rim;

    while (!glfwWindowShouldClose(Window))
    {
        glfwPollEvents();
        ImGui_ImplOpenGL2_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        SmokeEditor.Draw(SmokeSettings, Smoke.QueryMetrics(), Smoke.QueryExtent(), Smoke.IsResident());
        Smoke.Configure(SmokeSettings);
        if (SmokeEditor.ConsumeClearRequest()) Smoke.Clear();
        const double Now = glfwGetTime();
        const float Step = std::clamp(static_cast<float>(Now - PreviousTime), 1.0f / 240.0f, 1.0f / 20.0f);
        PreviousTime = Now;

        const SessionPolicy* Policy = Game.QuerySession().QueryPolicy();
        const InputRoutingPolicy& Routing = Policy ? Policy->QueryInputRoutingPolicy() : FallbackRouting;
        const InputControlMode Mode = Routing.AllowDriving && !MenuVisible
                                    ? InputControlMode::Driving : InputControlMode::Navigation;
        InputPacket Packet = Encoder.SampleGlfwWindow(Window, Step, Mode);
        Packet = Routing.Route(Packet);
        const ImGuiIO& Ui = ImGui::GetIO();
        if (Ui.WantCaptureKeyboard)
        {
            const uint64_t Sequence = Packet.SequenceIndex;
            Packet = {};
            Packet.SequenceIndex = Sequence;
        }
        const float LeftPanelEdge = std::clamp(Ui.DisplaySize.x * 0.23f, 220.0f, 288.0f);
        const float RightPanelEdge = Ui.DisplaySize.x - std::clamp(Ui.DisplaySize.x * 0.30f, 330.0f, 390.0f);
        const bool Looking = Ui.MouseDown[1] && Ui.MousePos.x > LeftPanelEdge && Ui.MousePos.x < RightPanelEdge
                           && Ui.MousePos.y < Ui.DisplaySize.y - 26.0f;
        if (!Looking) Packet.LookYawDelta = Packet.LookPitchDelta = 0.0f;
        if (!Encoder.WasActionPressed(InputActionResetUnit))
            Packet.ActionBits &= ~static_cast<uint32_t>(InputActionResetUnit);

        if (!Ui.WantCaptureKeyboard && Routing.AllowLevelNavigation && Mode == InputControlMode::Driving
            && Encoder.WasActionPressed(InputActionMenu))
        {
            MenuVisible = true;
            SelectedDestination = Game.QueryCurrentLevelPath() == Configuration.LobbyLevelPath ? 1 : 0;
        }
        if (MenuVisible && !Ui.WantCaptureKeyboard)
        {
            if (Encoder.WasActionPressed(InputActionNavigatePrev))
                SelectedDestination = (SelectedDestination + Navigation.QueryDestinations().size() - 1)
                                    % Navigation.QueryDestinations().size();
            if (Encoder.WasActionPressed(InputActionNavigateNext))
                SelectedDestination = (SelectedDestination + 1) % Navigation.QueryDestinations().size();
            if (Encoder.WasActionPressed(InputActionConfirm)
                && Navigation.Navigate(Game, SelectedDestination + 1))
                MenuVisible = !Game.QuerySession().QueryPolicy()->QueryInputRoutingPolicy().AllowDriving;
            if (Encoder.WasActionPressed(InputActionBack) && Routing.AllowDriving)
                MenuVisible = false;
        }

        if (!Game.QuerySession().QueryPolicy()->QueryInputRoutingPolicy().AllowDriving)
            MenuVisible = true;   // the Lobby is always in Navigation control mode; never a driving level
        Game.SubmitLocalInput(Packet);
        Game.Step(Step);
        const SessionPolicy* UpdatedPolicy = Game.QuerySession().QueryPolicy();
        const InputRoutingPolicy& UpdatedRouting = UpdatedPolicy->QueryInputRoutingPolicy();
        const std::string& Level = Game.QueryCurrentLevelPath();
        if (Level != LastLevel)
        {
            LastLevel = Level;
            MenuVisible = !UpdatedRouting.AllowDriving;
            SelectedDestination = Level == Configuration.LobbyLevelPath ? 1 : 0;
        }

        const DeployableUnit* Unit = Game.QueryLocalVehicle();
        InputPacket CameraPacket = Packet;
        if (Ui.WantCaptureKeyboard || !Encoder.WasActionPressed(InputActionCycleCamera))
            CameraPacket.ActionBits &= ~static_cast<uint32_t>(InputActionCycleCamera);
        Camera.Step(CameraPacket, UpdatedRouting, Unit);
        Vehicles[0].PositionMetres = Unit ? Unit->QueryPositionMetres() : std::array<float, 3>{ 0.0f, 0.0f, 0.53f };
        Vehicles[0].YawDegrees = Unit ? Unit->QueryYawDegrees() : 0.0f;
        SmokeSdfWorld FluidWorld{};
        FluidWorld.VehiclePosition = { Vehicles[0].PositionMetres[0], Vehicles[0].PositionMetres[1], Vehicles[0].PositionMetres[2] };
        FluidWorld.VehicleYawDegrees = Vehicles[0].YawDegrees;
        FluidWorld.VehicleEnabled = Unit != nullptr;
        if (SmokeSettings.TestPlume != 0 && SmokeSettings.Enabled)
        {
            const float Yaw = Vehicles[0].YawDegrees * 0.01745329252f;
            SmokeSource TestSource{};
            TestSource.Position = { Vehicles[0].PositionMetres[0] + 3.2f * std::cos(Yaw),
                                    Vehicles[0].PositionMetres[1] + 3.2f * std::sin(Yaw),
                                    Vehicles[0].PositionMetres[2] + 0.95f };
            TestSource.RadiusMetres = SmokeSettings.TestPlume == 2 ? 0.45f : 0.75f;
            TestSource.SmokeRate = SmokeSettings.TestPlume == 2 ? 2.5f : 12.0f;
            TestSource.HeatRate = SmokeSettings.TestPlume == 2 ? 30.0f : 2.0f;
            TestSource.FlameRate = SmokeSettings.TestPlume == 2 ? 5.0f : 0.0f;
            TestSource.SootRate = SmokeSettings.TestPlume == 2 ? 2.0f : 0.15f;
            TestSource.RemainingSeconds = 1.0f;
            TestSource.Persistent = true;
            Smoke.AddSource(TestSource);
        }
        else Smoke.ClearPersistentSources();
        Smoke.Advance(Step, FluidWorld);
        const PresentationCamera View = Camera.Resolve(UpdatedRouting, Unit);
        const PresentationFrame Image = UpdatedRouting.AllowDriving
            ? RenderProvingGroundsFrame(Vehicles, View, 960, 624, &Smoke)
            : RenderLobbyFrame(Vehicles, View, 960, 624, &Smoke);

        const std::string LevelLabel = UpdatedRouting.AllowDriving ? "Proving Grounds" : "Lobby";
        glfwSetWindowTitle(Window, ResolveControlsTitle(LevelLabel, UpdatedRouting, MenuVisible,
                                                        Navigation.QueryDestinations(), SelectedDestination).c_str());
        Present(Window, Texture, Image, MenuVisible, SelectedDestination, Navigation, Game);
        ImGui::Render();
        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(Window);
    }

    glDeleteTextures(1, &Texture);
    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(Window);
    glfwTerminate();
    return 0;
}
