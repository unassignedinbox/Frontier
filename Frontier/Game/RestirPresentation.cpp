#include "Frontier/Game/RestirPresentation.h"
#include "Frontier/Engine/Rendering/GlobalIllumination/IlluminationHost.h"
#include "Frontier/Engine/Rendering/GlobalIllumination/OccupantRelocation.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iterator>

namespace Frontier
{
using namespace GlobalIllumination;
struct RestirPresentation::Impl
{
    DeviceExchange Exchange;
    std::unique_ptr<IlluminationHost> Host;
    EnvironmentSettings Environment;
    int Width=0, Height=0;
    double Elapsed=0;
    std::array<int32_t, 5> VehicleIndices{-1,-1,-1,-1,-1};
    std::array<Transform, 5> VehicleInitial{};
    Vector3 Origin{};
};
RestirPresentation::RestirPresentation() : State(std::make_unique<Impl>()) {}
RestirPresentation::~RestirPresentation() = default;
static bool ReadArchive(const std::filesystem::path& Path, std::vector<uint8_t>& Bytes, std::string& Diagnostic)
{
    std::ifstream Input(Path, std::ios::binary);
    if (!Input) { Diagnostic = "Missing scene archive: " + Path.string(); return false; }
    Bytes.assign(std::istreambuf_iterator<char>(Input), std::istreambuf_iterator<char>());
    return true;
}
bool RestirPresentation::Initialize(const std::string& ContentRoot, int Width, int Height, std::string& Diagnostic)
{
    if (Width < 64 || Height < 64) { Diagnostic = "ReSTIR dimensions must be at least 64 pixels"; return false; }
    auto Root = std::filesystem::path(ContentRoot);
    auto Scene = Root / "EnvironmentArchives/ProvingGround";
    if (!State->Exchange.Initialize("", &Diagnostic)) return false;
    IlluminationAssets Assets;
    Assets.FacetSpace = Scene / "ProvingGround.facetspace";
    Assets.OmitVehicle = false;
    if (!ReadArchive(Scene / "Materials.bin", Assets.MaterialProfiles, Diagnostic)) return false;
    IlluminationSettings Settings;
    Settings.Quality = "low";
    Settings.RayTracing = true;
    State->Width=Width; State->Height=Height;
    State->Host = std::make_unique<IlluminationHost>(State->Exchange, Root / "ShaderArchives/GlobalIllumination");
    if (!State->Host->Construct(Assets, Settings, {static_cast<uint32_t>(Width),static_cast<uint32_t>(Height)}, &Diagnostic)) return false;
    const std::pair<const char*, std::shared_ptr<DeviceRegion>> Profiles[] = {
        {"VehicleProfile.bin", State->Host->VehicleRegion}, {"EffectProfile.bin", State->Host->EffectRegion},
        {"CloudProfile.bin", State->Host->CloudParams}, {"CloudShadowProfile.bin", State->Host->CloudShadowParams} };
    for (const auto& [Name, Region] : Profiles)
    {
        std::vector<uint8_t> Bytes;
        if (!ReadArchive(Scene / Name, Bytes, Diagnostic)) return false;
        if (!Region || Bytes.size() > Region->Capacity) { Diagnostic = std::string("Invalid profile size: ")+Name; return false; }
        State->Exchange.WriteRegion(Region, 0, Bytes.data(), Bytes.size());
    }
    const auto& World = State->Host->Scene;
    State->VehicleIndices = {World.VehicleBody, World.VehicleWheels[0], World.VehicleWheels[1],
                             World.VehicleWheels[2], World.VehicleWheels[3]};
    for (size_t I=0; I<State->VehicleIndices.size(); ++I)
    {
        const int32_t Id=State->VehicleIndices[I];
        if (Id<0 || static_cast<size_t>(Id)>=World.Occupants.size())
        { Diagnostic="Proving-ground archive does not contain a complete vehicle"; return false; }
        State->VehicleInitial[I]=World.Occupants[static_cast<size_t>(Id)].Current;
    }
    State->Origin={State->VehicleInitial[0][12],State->VehicleInitial[0][13],State->VehicleInitial[0][14]};
    State->Environment.SunAutoPosition = false;
    State->Environment.DayAnimate = false;
    return true;
}
std::array<float, 3> RestirPresentation::VehiclePosition() const
{
    if (!State->Host) return {};
    const auto& Scene=State->Host->Scene;
    if (Scene.VehicleBody < 0 || static_cast<size_t>(Scene.VehicleBody)>=Scene.Occupants.size()) return {};
    const auto& T=Scene.Occupants[static_cast<size_t>(Scene.VehicleBody)].Current;
    return {static_cast<float>(T[12]),static_cast<float>(T[13]),static_cast<float>(T[14])};
}
void RestirPresentation::SetVehiclePose(const std::array<float, 3>& Position, float YawDegrees)
{
    if (!State->Host) return;
    constexpr double Pi=3.14159265358979323846;
    const double Half=YawDegrees*Pi/360.0;
    const auto& O=State->Origin;
    const Transform ToWorld=ConstructRigidTransform({O[0]+Position[0],O[1]+Position[1],O[2]+Position[2]-0.53},
                                                    {0,0,std::sin(Half),std::cos(Half)});
    const Transform FromOrigin=ConstructRigidTransform({-O[0],-O[1],-O[2]}, {0,0,0,1});
    const Transform Change=MultiplyTransform(ToWorld,FromOrigin);
    std::vector<OccupantPlacement> Placements;
    for(size_t I=0;I<State->VehicleIndices.size();++I)
        Placements.push_back({static_cast<uint32_t>(State->VehicleIndices[I]),MultiplyTransform(Change,State->VehicleInitial[I])});
    RelocateOccupants(*State->Host,Placements);
}
bool RestirPresentation::Render(const PresentationCamera& Camera, double DeltaSeconds, PresentationFrame& Out, std::string& Diagnostic)
{
    if (!State->Host) { Diagnostic="ReSTIR is not initialized"; return false; }
    auto& Host=*State->Host;
    constexpr double Pi=3.14159265358979323846;
    const double Yaw=Camera.YawDegrees*Pi/180.0, Pitch=Camera.PitchDegrees*Pi/180.0;
    const Vector3 Target{Camera.FocusMetres[0],Camera.FocusMetres[1],Camera.FocusMetres[2]};
    const Vector3 Position{Target[0]+Camera.DistanceMetres*std::cos(Pitch)*std::cos(Yaw),
                           Target[1]+Camera.DistanceMetres*std::cos(Pitch)*std::sin(Yaw),Target[2]+Camera.DistanceMetres*std::sin(Pitch)};
    RefreshInputs Inputs;
    Inputs.Camera.Position=Position; Inputs.Camera.Target=Target;
    Inputs.Camera.Forward=NormalizeVector(SubtractVector(Target,Position));
    Inputs.Camera.Up={0,0,1}; Inputs.Camera.VerticalFieldOfView=55.0*Pi/180.0;
    Inputs.Camera.Aspect=static_cast<double>(State->Width)/State->Height; Inputs.Camera.Near=0.05;
    Inputs.Focus=MultiplyAddVector(Position,Inputs.Camera.Forward,0.45*Host.Grade.ClipResolution*Host.Grade.FinestVoxel);
    State->Elapsed += std::max(0.0,DeltaSeconds);
    Inputs.External.ElapsedSeconds=State->Elapsed;
    Inputs.External.PresentationControl=(1u<<8)|(1000u<<20);
    Inputs.External.FireBlock={0,0,0,0,1,0.46f,0.14f,14};
    Inputs.Environment=&State->Environment;
    if (State->VehicleIndices[0]>=0)
        Inputs.VehicleTransform=Host.Scene.Occupants[static_cast<size_t>(State->VehicleIndices[0])].Current;
    Inputs.CelestialDelta=std::max(0.0,DeltaSeconds); Inputs.LuminairesLive=true;
    if (!Host.AdvanceRefresh(&Inputs,nullptr,nullptr,&Diagnostic)) return false;
    std::vector<uint8_t> Rgba;
    if (!State->Exchange.ReadTexture(Host.CaptureTexture,0,0,Rgba) || Rgba.size()!=static_cast<size_t>(State->Width)*State->Height*4)
    { Diagnostic="Failed to read ReSTIR output"; return false; }
    Out.Width=State->Width; Out.Height=State->Height; Out.Rgb.resize(static_cast<size_t>(Out.Width)*Out.Height*3);
    for (size_t I=0;I<Out.Rgb.size()/3;++I) for(int C=0;C<3;++C) Out.Rgb[I*3+C]=Rgba[I*4+C];
    return true;
}
}
