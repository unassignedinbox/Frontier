/*==========================================================================================================================================
                                                            LOBBYSESSIONMAIN.CPP
==========================================================================================================================================*/
// 🧩 The game, headless: opens on the Lobby (the default level), joins the local player under the Lobby
//    policy, steps the session, exercises the debug level switch, and — with --render — draws the studio
//    through the software presentation so the C++ Lobby can be seen without a GPU.
//
//      LobbySession [--archive DIR] [--render out.ppm] [--guests N] [--xray] [--scan SECONDS] [--ghost] [--wireframe]
//
//    The facet archives are the vehicle's own (Experimental/VehicleHmiPrototype/Archive by default).

#include "Frontier/Engine/Effects/GasParameters.h"
#include "Frontier/Engine/Effects/PlumeSolver.h"
#include "Frontier/Engine/Session/LobbySession.h"
#include "Frontier/Game/SoftwarePresentation.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <cstring>
#include <string>

namespace
{
using namespace Frontier;

// 📝 sRGB hex (the configurator's config.ts) to the linear albedo the presentation shades in.
std::array<float, 3> LinearFromHex(const char* Hex)
{
    unsigned int Packed = 0;
    std::sscanf(Hex, "%x", &Packed);
    const float Srgb[3] = { static_cast<float>((Packed >> 16) & 255) / 255.0f,
                            static_cast<float>((Packed >> 8) & 255) / 255.0f,
                            static_cast<float>(Packed & 255) / 255.0f };
    return { std::pow(Srgb[0], 2.2f), std::pow(Srgb[1], 2.2f), std::pow(Srgb[2], 2.2f) };
}

struct PaintChoice { const char* Id; const char* Hex; bool Metallic; };
// 📝 COLORS from the reference configurator's data/config.ts, verbatim.
constexpr PaintChoice PaintChoices[8] = {
    { "silver", "c2c8d0", true }, { "black", "17191d", true }, { "red", "8a1a1d", true },
    { "blue", "1c3f73", true },   { "sand", "b2946a", false }, { "green", "33442f", false },
    { "white", "e7ebef", true },  { "copper", "8c5a2e", true },
};

const PaintChoice* FindPaint(const std::string& IdOrHex)
{
    for (const PaintChoice& Choice : PaintChoices)
        if (IdOrHex == Choice.Id || IdOrHex == Choice.Hex || IdOrHex == std::string("#") + Choice.Hex)
            return &Choice;
    return nullptr;
}

// 📝 The raycast half of the reference's drag-and-drop: an NDC point unprojects through the Garage
//    camera and the first body facet it strikes becomes the ripple's centre (Truck.tsx applyReq path).
bool PickBodyPoint(const FacetArchiveData& Body, float NdcX, float NdcY, std::array<float, 3>& OutPoint)
{
    constexpr float Pi = 3.14159265358979f;
    const float Yaw = 48.24f * Pi / 180.0f, Pitch = 12.98f * Pi / 180.0f, Distance = 10.47f;
    const std::array<float, 3> Focus = { 0.0f, 0.0f, 0.55f };
    const std::array<float, 3> Eye = { Focus[0] + Distance * std::cos(Pitch) * std::cos(Yaw),
                                       Focus[1] + Distance * std::cos(Pitch) * std::sin(Yaw),
                                       Focus[2] + Distance * std::sin(Pitch) };
    auto Sub = [](const std::array<float, 3>& A, const std::array<float, 3>& B) {
        return std::array<float, 3>{ A[0] - B[0], A[1] - B[1], A[2] - B[2] }; };
    auto Cross = [](const std::array<float, 3>& A, const std::array<float, 3>& B) {
        return std::array<float, 3>{ A[1] * B[2] - A[2] * B[1], A[2] * B[0] - A[0] * B[2], A[0] * B[1] - A[1] * B[0] }; };
    auto Normalize = [](std::array<float, 3> V) {
        const float L = std::sqrt(V[0] * V[0] + V[1] * V[1] + V[2] * V[2]);
        if (L > 1e-8f) { V[0] /= L; V[1] /= L; V[2] /= L; }
        return V; };
    const std::array<float, 3> Forward = Normalize(Sub(Focus, Eye));
    const std::array<float, 3> Right = Normalize(Cross(Forward, { 0.0f, 0.0f, 1.0f }));
    const std::array<float, 3> Up = Cross(Right, Forward);
    const float TanHalf = std::tan(0.5f * 34.0f * Pi / 180.0f), Aspect = 640.0f / 400.0f;
    std::array<float, 3> Direction = Forward;
    for (int C = 0; C < 3; ++C)
        Direction[C] += Right[C] * (NdcX * TanHalf * Aspect) + Up[C] * (NdcY * TanHalf);
    Direction = Normalize(Direction);

    float Best = 1e30f;
    bool Hit = false;
    for (uint32_t Facet = 0; Facet < Body.FacetCount; ++Facet)
    {
        std::array<float, 3> Corner[3];
        for (int K = 0; K < 3; ++K)
        {
            const uint32_t Vertex = Body.Facets[Facet * 3 + K];
            Corner[K] = { Body.Positions[Vertex * 3], Body.Positions[Vertex * 3 + 1],
                          Body.Positions[Vertex * 3 + 2] - 0.11285f + 0.53f };
        }
        const auto E1 = Sub(Corner[1], Corner[0]), E2 = Sub(Corner[2], Corner[0]);
        const auto P = Cross(Direction, E2);
        const float Det = E1[0] * P[0] + E1[1] * P[1] + E1[2] * P[2];
        if (std::fabs(Det) < 1e-9f) continue;
        const auto T = Sub(Eye, Corner[0]);
        const float U = (T[0] * P[0] + T[1] * P[1] + T[2] * P[2]) / Det;
        if (U < 0.0f || U > 1.0f) continue;
        const auto Q = Cross(T, E1);
        const float V = (Direction[0] * Q[0] + Direction[1] * Q[1] + Direction[2] * Q[2]) / Det;
        if (V < 0.0f || U + V > 1.0f) continue;
        const float Travel = (E2[0] * Q[0] + E2[1] * Q[1] + E2[2] * Q[2]) / Det;
        if (Travel > 0.15f && Travel < Best)
        {
            Best = Travel; Hit = true;
            for (int C = 0; C < 3; ++C) OutPoint[C] = Eye[C] + Direction[C] * Travel;
        }
    }
    return Hit;
}

// 📝 One plume frame: the solver's own density/heat, projected down the Y axis (the side elevation a
//    tyre-smoke plume is judged by), heat tinting the smoke toward flame colour. No other data source.
bool WritePlumeFrame(const std::string& Path, const PlumeSolver& Solver, float Albedo)
{
    const auto& Extent = Solver.QueryExtent();
    const int Scale = 6, Width = Extent[0] * Scale, Height = Extent[2] * Scale;
    std::ofstream Stream(Path, std::ios::binary);
    if (!Stream) return false;
    Stream << "P6\n" << Width << " " << Height << "\n255\n";
    for (int Py = 0; Py < Height; ++Py)
    {
        const int Z = Extent[2] - 1 - Py / Scale;
        for (int Px = 0; Px < Width; ++Px)
        {
            const int X = Px / Scale;
            float Column = 0.0f, HeatColumn = 0.0f;
            for (int Y = 0; Y < Extent[1]; ++Y)
            {
                Column += Solver.QueryDensity(X, Y, Z);
                HeatColumn += Solver.QueryHeat(X, Y, Z);
            }
            const float Smoke = 1.0f - std::exp(-Column * 0.02f);
            const float Heat = std::clamp(HeatColumn * 0.03f, 0.0f, 1.0f);
            float Rgb[3] = { 0.04f + Smoke * Albedo, 0.05f + Smoke * Albedo, 0.07f + Smoke * Albedo };
            Rgb[0] += Heat * Smoke * 0.55f; Rgb[1] += Heat * Smoke * 0.18f; Rgb[2] *= 1.0f - Heat * 0.6f;
            for (int C = 0; C < 3; ++C)
                Stream.put(static_cast<char>(std::lround(std::pow(std::clamp(Rgb[C], 0.0f, 1.0f), 1.0f / 2.2f) * 255.0f)));
        }
    }
    return static_cast<bool>(Stream);
}

} // namespace

int main(int ArgumentCount, char** Arguments)
{
    using namespace Frontier;

    std::string ArchiveDirectory = "Experimental/VehicleHmiPrototype/Archive";
    std::string RenderPath;
    int Guests = 0;
    bool XRay = false;
    float ScanSeconds = 1.6f;
    int InspectionMode = 0;   // 1 ghost, 2 wireframe (3 = scan, via --scan/--xray)
    std::string GaragePath, PlumePath, PlumeFramePrefix;
    std::string PaintId = "red", FromPaintId = "silver", FinishId = "gloss";
    float RippleSeconds = 0.675f;
    bool HasNdc = false; float NdcX = 0.0f, NdcY = 0.0f;
    for (int Index = 1; Index < ArgumentCount; ++Index)
    {
        if (std::strcmp(Arguments[Index], "--archive") == 0 && Index + 1 < ArgumentCount)
            ArchiveDirectory = Arguments[++Index];
        else if (std::strcmp(Arguments[Index], "--render") == 0 && Index + 1 < ArgumentCount)
            RenderPath = Arguments[++Index];
        else if (std::strcmp(Arguments[Index], "--xray") == 0)
            XRay = true;
        else if (std::strcmp(Arguments[Index], "--ghost") == 0)
            InspectionMode = 1;
        else if (std::strcmp(Arguments[Index], "--wireframe") == 0)
            InspectionMode = 2;
        else if (std::strcmp(Arguments[Index], "--scan") == 0 && Index + 1 < ArgumentCount)
        { XRay = true; ScanSeconds = std::atof(Arguments[++Index]); }
        else if (std::strcmp(Arguments[Index], "--garage") == 0 && Index + 1 < ArgumentCount)
            GaragePath = Arguments[++Index];
        else if (std::strcmp(Arguments[Index], "--plume") == 0 && Index + 1 < ArgumentCount)
            PlumePath = Arguments[++Index];
        else if (std::strcmp(Arguments[Index], "--plume-frames") == 0 && Index + 1 < ArgumentCount)
            PlumeFramePrefix = Arguments[++Index];
        else if (std::strcmp(Arguments[Index], "--paint") == 0 && Index + 1 < ArgumentCount)
            PaintId = Arguments[++Index];
        else if (std::strcmp(Arguments[Index], "--from-paint") == 0 && Index + 1 < ArgumentCount)
            FromPaintId = Arguments[++Index];
        else if (std::strcmp(Arguments[Index], "--finish") == 0 && Index + 1 < ArgumentCount)
            FinishId = Arguments[++Index];
        else if (std::strcmp(Arguments[Index], "--ripple") == 0 && Index + 1 < ArgumentCount)
            RippleSeconds = std::atof(Arguments[++Index]);
        else if (std::strcmp(Arguments[Index], "--paint-ndc") == 0 && Index + 2 < ArgumentCount)
        { HasNdc = true; NdcX = std::atof(Arguments[++Index]); NdcY = std::atof(Arguments[++Index]); }
        else if (std::strcmp(Arguments[Index], "--guests") == 0 && Index + 1 < ArgumentCount)
            Guests = std::atoi(Arguments[++Index]);
    }


    if (!PlumePath.empty() || !PlumeFramePrefix.empty())
    {
        // The plume, run and drawn on its own: "Thick burnout" through the ported editor mapping.
        GasParameters Gas;
        Gas.ApplyPreset("Thick burnout");
        const PlumeSettings Settings = ResolvePlumeSettings(Gas);
        PlumeSolver Solver(PlumeGrade{ "Proof", { 48, 48, 24 }, 0.35f, 8, 32 });
        const std::vector<PlumeSource> Sources = { PlumeSource{ { -5.0f, 0.0f, 0.6f }, { 3.5f, 0.0f, 1.6f }, 1.1f, 3.0f, 0.8f } };
        int Frames = 0;
        for (int Step = 0; Step <= 120; ++Step)
        {
            if (!PlumeFramePrefix.empty() && Step % 10 == 0)
                WritePlumeFrame(PlumeFramePrefix + (Frames < 10 ? "0" : "") + std::to_string(Frames) + ".ppm", Solver, Settings.Albedo),
                ++Frames;
            if (Step < 120)
                Solver.Step(1.0f / 30.0f, Settings, Sources);
        }
        if (!PlumePath.empty() && !WritePlumeFrame(PlumePath, Solver, Settings.Albedo))
        { std::printf("plume: could not write %s\n", PlumePath.c_str()); return 1; }
        std::printf("plume: total density %.2f, mean divergence %.4f, frames %d\n",
                    Solver.QueryTotalDensity(), Solver.QueryMeanDivergence(), Frames);
        return 0;
    }

    if (!GaragePath.empty())
    {
        const FacetArchiveData Body = LoadFacetArchive(ArchiveDirectory + "/ControlVehicleBody.facet");
        const FacetArchiveData Tyre = LoadFacetArchive(ArchiveDirectory + "/ControlVehicleTyre.facet");
        const FacetArchiveData Rim = LoadFacetArchive(ArchiveDirectory + "/ControlVehicleRim.facet");
        if (!Body.IsValid()) { std::printf("garage: vehicle archives not found under %s\n", ArchiveDirectory.c_str()); return 1; }
        const PaintChoice* Next = FindPaint(PaintId);
        const PaintChoice* Previous = FindPaint(FromPaintId);
        if (!Next || !Previous) { std::printf("garage: unknown paint (ids: silver black red blue sand green white copper)\n"); return 1; }
        PresentationVehicle Vehicle{};
        Vehicle.Body = &Body; Vehicle.Tyre = &Tyre; Vehicle.Rim = &Rim;
        Vehicle.PositionMetres = { 0.0f, 0.0f, 0.53f };
        Vehicle.YawDegrees = 0.0f;   // 📝 no rotation: fixed vehicle, fixed camera, every frame
        Vehicle.Paint = LinearFromHex(Previous->Hex);
        // 📝 The touch point: the raycast hit of a drop, else the reference's nose default (2.75, 1.25, 0)
        //    mapped onto this vehicle (nose x, centreline, mid-height).
        std::array<float, 3> Centre = { Body.BoundsMaximum[0], 0.0f,
            0.53f + 0.5f * (Body.BoundsMinimum[2] + Body.BoundsMaximum[2]) - 0.11285f };
        const char* OriginName = "nose default (tap)";
        if (HasNdc && PickBodyPoint(Body, NdcX, NdcY, Centre)) OriginName = "raycast drop point";
        const float Roughness = FinishId == "matte" ? 0.72f : (Next->Metallic ? 0.26f : 0.45f);
        const float Metalness = Next->Metallic ? 0.9f : 0.35f;
        Vehicle.BodyRipple.Begin(Centre, LinearFromHex(Next->Hex), Roughness, Metalness);
        if (Vehicle.BodyRipple.Step(RippleSeconds))
            Vehicle.Paint = Vehicle.BodyRipple.NextColour;   // finishRun(): commit on completion
        if (!RenderGaragePpm(GaragePath, Vehicle)) { std::printf("garage: could not write %s\n", GaragePath.c_str()); return 1; }
        std::printf("garage: %s at t=%.2fs from %s, origin %s (%.2f, %.2f, %.2f), radius %.2f m\n",
                    GaragePath.c_str(), RippleSeconds, Previous->Id, OriginName,
                    Centre[0], Centre[1], Centre[2], Vehicle.BodyRipple.RadiusMetres);
        return 0;
    }

    LobbySession Game;
    const SessionSpace& Session = Game.QuerySession();
    std::printf("level: %s (policy: %s)\n", Game.QueryCurrentLevelPath().c_str(),
                Session.QueryPolicy() ? Session.QueryPolicy()->QueryPolicyName().data() : "none");
    std::printf("policy: maximumPlayers=%u respawn=%s derez=%s\n",
                Session.QueryPolicy() ? Session.QueryPolicy()->QueryMaximumPlayers() : 0,
                Session.QueryPolicy() && Session.QueryPolicy()->QueryRespawnPolicy().IsRespawnAllowed() ? "allowed" : "disabled",
                Session.QueryPolicy() && Session.QueryPolicy()->AllowsDerez() ? "allowed" : "disabled");

    // Fill the studio from the same field as the prototype: guests join until the policy says full.
    static const char* Names[4] = { "KESTREL", "MIRAGE", "ORION", "VEGA-9" };
    static const char* Ranks[4] = { "DIAMOND I", "SILVER III", "BRONZE I", "GOLD III" };
    for (int Guest = 0; Guest < Guests && Guest < 4; ++Guest)
        Game.AccessSession().AddPlayer(Names[Guest], Ranks[Guest]);
    const PlayerToken Extra = Game.AccessSession().AddPlayer("EXTRA", "BRONZE I");
    std::printf("players: %u (a sixth join %s — the policy holds five)\n", Session.QueryConnectedCount(),
                Extra.IsValid() ? "was ACCEPTED" : "was refused");

    for (int Step = 0; Step < 120; ++Step)
        Game.Step(1.0f / 60.0f);

    if (DeployableUnit* Vehicle = Game.QueryLocalVehicle())
    {
        const std::array<float, 3>& Position = Vehicle->QueryPositionMetres();
        std::printf("vehicle: stand (%.2f, %.2f, %.2f) yaw %.1f, handbrake latched: %s\n",
                    Position[0], Position[1], Position[2], Vehicle->QueryYawDegrees(),
                    Vehicle->IsHandbrakeLatched() ? "yes (parked)" : "no");
        std::printf("cabin: steering wheel %.1f deg, tablet powered: %s, content: %s\n",
                    Vehicle->QueryCabin().Wheel.AngleDegrees,
                    Vehicle->QueryCabin().Tablet.Powered ? "yes" : "no",
                    Vehicle->QueryCabin().Tablet.IsEmpty() ? "empty (no HMI yet)" : "showing");
    }

    // Driving works even parked-awareness: throttle releases the latch, the wheel follows the steer axis.
    InputPacket Drive{};
    Drive.ThrottleAxis = 1.0f;
    Drive.SteerAxis = 0.5f;
    for (int Step = 0; Step < 60; ++Step)
    {
        Game.SubmitLocalInput(Drive);
        Game.Step(1.0f / 60.0f);
    }
    if (DeployableUnit* Vehicle = Game.QueryLocalVehicle())
        std::printf("after 1 s of throttle: speed %.2f m/s, wheel %.1f deg, handbrake latched: %s\n",
                    Vehicle->QueryForwardSpeedMps(), Vehicle->QueryCabin().Wheel.AngleDegrees,
                    Vehicle->IsHandbrakeLatched() ? "yes" : "no (released)");

#if FRONTIER_DEBUG_LEVEL_SWITCH
    Game.HandleDebugKeys(true, true, true, false, -1);   // Ctrl+Shift+L opens the popup
    std::printf("debug popup (Ctrl+Shift+L): %s\n", Game.IsDebugPopupOpen() ? "open" : "closed");
    Game.HandleDebugKeys(false, false, false, false, 3); // digit 3: Proving Grounds under RacePolicy
    std::printf("after debug switch: level %s, policy %s, players preserved: %zu\n",
                Game.QueryCurrentLevelPath().c_str(),
                Game.QuerySession().QueryPolicy() ? Game.QuerySession().QueryPolicy()->QueryPolicyName().data() : "none",
                Game.QuerySession().QueryPlayers().size());
#endif

    if (!RenderPath.empty())
    {
        const FacetArchiveData Body = LoadFacetArchive(ArchiveDirectory + "/ControlVehicleBody.facet");
        const FacetArchiveData Tyre = LoadFacetArchive(ArchiveDirectory + "/ControlVehicleTyre.facet");
        const FacetArchiveData Rim = LoadFacetArchive(ArchiveDirectory + "/ControlVehicleRim.facet");
        if (!Body.IsValid())
        {
            std::printf("render: vehicle archives not found under %s\n", ArchiveDirectory.c_str());
            return 1;
        }
        // Return to the lobby for the portrait and stand the fleet as the session placed it.
#if FRONTIER_DEBUG_LEVEL_SWITCH
        Game.SwitchLevelForDebug(0);
#endif
        std::vector<PresentationVehicle> Vehicles;
        static const std::array<float, 3> Paints[5] = {
            { 0.20f, 0.21f, 0.23f }, { 0.35f, 0.55f, 0.42f }, { 0.55f, 0.45f, 0.28f },
            { 0.38f, 0.48f, 0.62f }, { 0.58f, 0.40f, 0.36f } };
        size_t Index = 0;
        for (const PlayerProfile& Profile : Game.QuerySession().QueryPlayers())
        {
            (void)Profile;
            if (const DeployableUnit* Unit = Game.QuerySession().FindUnit(Profile.Token))
            {
                PresentationVehicle Vehicle{};
                Vehicle.Body = &Body;
                Vehicle.Tyre = &Tyre;
                Vehicle.Rim = &Rim;
                Vehicle.PositionMetres = Unit->QueryPositionMetres();
                Vehicle.YawDegrees = Unit->QueryYawDegrees();
                Vehicle.Paint = Paints[Index % 5];
                if (Index == 0 && (XRay || InspectionMode > 0))
                {
                    Vehicle.Inspection.View = XRay ? InspectionView::Scan
                                                   : static_cast<InspectionView>(InspectionMode);
                    Vehicle.Inspection.ScanSeconds = ScanSeconds;
                }
                Vehicles.push_back(Vehicle);
            }
            ++Index;
        }
        if (!RenderLobbyPpm(RenderPath, Vehicles, 38.0f, 14.0f, 15.5f))
        {
            std::printf("render: could not write %s\n", RenderPath.c_str());
            return 1;
        }
        std::printf("render: %s (%zu vehicles, body %u facets)\n", RenderPath.c_str(), Vehicles.size(), Body.FacetCount);
    }
    return 0;
}
