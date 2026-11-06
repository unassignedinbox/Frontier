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
#ifdef FRONTIER_WITH_RESTIR
#include "Frontier/Game/RestirPresentation.h"
#include <memory>
#endif
#include "Projects/Everest/Source/Navigation/NavigationApp.h"
#include "Projects/Everest/Source/Session/SessionPolicy.h"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <fstream>
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

bool Present(GLFWwindow* Window, GLuint Texture, const Frontier::PresentationFrame& Frame,
             bool MenuVisible, size_t SelectedDestination, const Everest::NavigationApp& Navigation,
             const Frontier::LobbySession& Game, const std::string& CapturePath)
{
    int Width = 0, Height = 0;
    glfwGetFramebufferSize(Window, &Width, &Height);
    if (Width <= 0 || Height <= 0)
        return CapturePath.empty();
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
    bool Captured = true;
    if (!CapturePath.empty())
    {
        // Read the actual composed window back buffer, after the menu and before swap.
        std::vector<unsigned char> Pixels(static_cast<size_t>(Width) * Height * 3);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadPixels(0, 0, Width, Height, GL_RGB, GL_UNSIGNED_BYTE, Pixels.data());
        std::ofstream File(CapturePath, std::ios::binary);
        File << "P6\n" << Width << ' ' << Height << "\n255\n";
        for (int Y = Height - 1; Y >= 0; --Y)
            File.write(reinterpret_cast<const char*>(Pixels.data() + static_cast<size_t>(Y) * Width * 3), Width * 3);
        Captured = static_cast<bool>(File) && glGetError() == GL_NO_ERROR;
    }
    glfwSwapBuffers(Window);
    return Captured;
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
    std::string CapturePath;
#ifdef FRONTIER_WITH_RESTIR
    std::string ContentDirectory = "EngineContent";
#endif
    for (int Index = 1; Index < ArgumentCount; ++Index)
        if (std::strcmp(Arguments[Index], "--archive") == 0 && Index + 1 < ArgumentCount)
            ArchiveDirectory = Arguments[++Index];
#ifdef FRONTIER_WITH_RESTIR
        else if (std::strcmp(Arguments[Index], "--content") == 0 && Index + 1 < ArgumentCount)
            ContentDirectory = Arguments[++Index];
#endif
        else if (std::strcmp(Arguments[Index], "--capture") == 0 && Index + 1 < ArgumentCount)
            CapturePath = Arguments[++Index];

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
    glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    LobbySession Game(Configuration, Everest::ConstructEverestSessionPolicy);
    Everest::NavigationApp Navigation(Configuration);
    InputEncoder Encoder;
    CameraController Camera;
    GLuint Texture = 0;
    glGenTextures(1, &Texture);
    bool MenuVisible = true;
    size_t SelectedDestination = 1;   // default Lobby selection is the next route: Proving Grounds
    std::string LastLevel = Game.QueryCurrentLevelPath();
    double PreviousTime = glfwGetTime();
    static const InputRoutingPolicy FallbackRouting{};
#ifdef FRONTIER_WITH_RESTIR
    std::unique_ptr<RestirPresentation> Restir;
    std::array<float,3> RestirOrigin{};
    std::string RestirLevel;
#endif
    int Result = 0;
    int PresentedFrames = 0;
    std::vector<PresentationVehicle> Vehicles(1);
    Vehicles[0].Body = &Body;
    Vehicles[0].Tyre = &Tyre;
    Vehicles[0].Rim = &Rim;

    while (!glfwWindowShouldClose(Window))
    {
        glfwPollEvents();
        const double Now = glfwGetTime();
        const float Step = std::clamp(static_cast<float>(Now - PreviousTime), 1.0f / 240.0f, 1.0f / 20.0f);
        PreviousTime = Now;

        const SessionPolicy* Policy = Game.QuerySession().QueryPolicy();
        const InputRoutingPolicy& Routing = Policy ? Policy->QueryInputRoutingPolicy() : FallbackRouting;
        const InputControlMode Mode = Routing.AllowDriving && !MenuVisible
                                    ? InputControlMode::Driving : InputControlMode::Navigation;
        InputPacket Packet = Encoder.SampleGlfwWindow(Window, Step, Mode);
        Packet = Routing.Route(Packet);
        if (!Encoder.WasActionPressed(InputActionResetUnit))
            Packet.ActionBits &= ~static_cast<uint32_t>(InputActionResetUnit);

        if (Routing.AllowLevelNavigation && Mode == InputControlMode::Driving
            && Encoder.WasActionPressed(InputActionMenu))
        {
            MenuVisible = true;
            SelectedDestination = Game.QueryCurrentLevelPath() == Configuration.LobbyLevelPath ? 1 : 0;
        }
        if (MenuVisible)
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
        if (!Encoder.WasActionPressed(InputActionCycleCamera))
            CameraPacket.ActionBits &= ~static_cast<uint32_t>(InputActionCycleCamera);
        Camera.Step(CameraPacket, UpdatedRouting, Unit);
        Vehicles[0].PositionMetres = Unit ? Unit->QueryPositionMetres() : std::array<float, 3>{ 0.0f, 0.0f, 0.53f };
        Vehicles[0].YawDegrees = Unit ? Unit->QueryYawDegrees() : 0.0f;
        PresentationCamera View = Camera.Resolve(UpdatedRouting, Unit);
        PresentationFrame Image;
#ifdef FRONTIER_WITH_RESTIR
        if (!Restir || RestirLevel != Level)
        {
            Restir.reset();
            glfwSetWindowTitle(Window, "Project Everest | Starting Vulkan ReSTIR...");
            Restir = std::make_unique<RestirPresentation>();
            std::string Diagnostic;
            const bool Lobby = !UpdatedRouting.AllowDriving;
            if (!Restir->Initialize(ContentDirectory, 480, 312, Diagnostic, Lobby))
            {
                std::fprintf(stderr, "EverestWindow: ReSTIR initialization failed: %s\n", Diagnostic.c_str());
                Result = 1; break;
            }
            RestirLevel = Level;
            RestirOrigin = Restir->VehiclePosition();
        }
        if (UpdatedRouting.AllowDriving)
        {
            Restir->SetVehiclePose(Vehicles[0].PositionMetres, Vehicles[0].YawDegrees);
            View.FocusMetres[0] += RestirOrigin[0]; View.FocusMetres[1] += RestirOrigin[1];
            View.FocusMetres[2] += RestirOrigin[2] - 0.53f;
        }
        std::string Diagnostic;
        if (!Restir->Render(View, Step, Image, Diagnostic))
        {
            std::fprintf(stderr, "EverestWindow: ReSTIR refresh failed: %s\n", Diagnostic.c_str());
            Result = 1; break;
        }
        const std::string LevelLabel = UpdatedRouting.AllowDriving ? "Proving Grounds / Vulkan ReSTIR" : "Lobby / Vulkan ReSTIR";
#else
        Image = UpdatedRouting.AllowDriving ? RenderProvingGroundsFrame(Vehicles, View, 960, 624)
                                            : RenderLobbyFrame(Vehicles, View, 960, 624);
        const std::string LevelLabel = UpdatedRouting.AllowDriving ? "Proving Grounds / software" : "Lobby / software";
#endif
        glfwSetWindowTitle(Window, ResolveControlsTitle(LevelLabel, UpdatedRouting, MenuVisible,
                                                        Navigation.QueryDestinations(), SelectedDestination).c_str());
        const bool CaptureNow = !CapturePath.empty() && ++PresentedFrames == 12;
        if (!Present(Window, Texture, Image, MenuVisible, SelectedDestination, Navigation, Game,
                     CaptureNow ? CapturePath : std::string{}))
        {
            std::fprintf(stderr, "EverestWindow: could not capture window framebuffer\n");
            Result = 1; break;
        }
        if (CaptureNow) break;
    }

    glDeleteTextures(1, &Texture);
    glfwDestroyWindow(Window);
    glfwTerminate();
    return Result;
}
