/*==========================================================================================================================================
                                                     GEOMETRYCOLLISIONVALIDATION.CPP
==========================================================================================================================================*/
// 🧩 The actual proving-ground ramp: a bounds baseline, placed facets, Jolt ray and vehicle drive.

#include "Frontier/Extensions/Rendering/SceneGeometryOccupancy.h"
#include "Frontier/Extensions/Physics/PhysicsExtension.h"
#include "Frontier/Engine/Rendering/GlobalIllumination/SceneStructure.h"
#include "Frontier/Engine/Scene/WorldSpace.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

using namespace Frontier;

namespace
{
int Checks = 0, Failures = 0;
void Expect(bool Value, const char* Description)
{
    ++Checks;
    if (!Value) ++Failures;
    std::printf("%s %s\n", Value ? "PASS" : "FAIL", Description);
}
} // namespace

int main(int Count, char** Arguments)
{
    if (Count != 3) return 1;
    const std::filesystem::path Project = Arguments[1], Archive = Arguments[2];
    WorldSpace Loaded;
    std::string Diagnostic;
    if (!Loaded.LoadLevelSpace(Project / "Content/Levels/Default.levelspace", &Diagnostic)) { std::puts(Diagnostic.c_str()); return 1; }
    const GeometryRecord* Source = nullptr;
    for (const SceneRecordBase* Record : Loaded.FindAllByType("Geometry"))
        if (Record->QueryRecordName() == "Jump ramp") Source = dynamic_cast<const GeometryRecord*>(Record);
    if (!Source) { std::puts("ramp not found"); return 1; }
    GlobalIllumination::SceneStructure Scene;
    if (!Scene.LoadFacetSpace(Archive / "ProvingGround.facetspace", &Diagnostic)) { std::puts(Diagnostic.c_str()); return 1; }
    std::vector<uint8_t> Materials;
    if (std::ifstream Stream(Archive / "Materials.bin", std::ios::binary); Stream)
        Materials.assign(std::istreambuf_iterator<char>(Stream), std::istreambuf_iterator<char>());
    WorldSpace World;
    World.Clear();
    const RecordToken RampToken = World.ConstructRecord("Geometry", "Jump ramp");
    auto* Ramp = dynamic_cast<GeometryRecord*>(World.FindRecord(RampToken));
    if (!Ramp) return 1;
    Ramp->AccessConfiguration() = Source->QueryConfiguration();
    SceneGeometryOccupancy Occupancy;
    Occupancy.Reset(&Scene, Materials);
    (void)Occupancy.ResolvePlacements(World);
    const auto Resolution = Occupancy.QueryResolutions()->find(RampToken.Value);
    if (Resolution == Occupancy.QueryResolutions()->end()) return 1;
    Ramp->AccessConfiguration().Resolved = Resolution->second;
    Expect(Resolution->second.Shown == 1, "actual ramp resolves to one shown record");
    const auto Surfaces = Occupancy.CaptureCollisionSurfaces(World);
    const auto Found = Surfaces->find(RampToken.Value);
    Expect(Found != Surfaces->end() && !Found->second.Corners.empty(), "placed ramp facets captured");
    if (Found == Surfaces->end()) return 1;

    const RecordToken VehicleToken = World.ConstructRecord("Vehicle", "Ramp vehicle");
    auto* Vehicle = dynamic_cast<VehicleRecord*>(World.FindRecord(VehicleToken));
    if (!Vehicle) return 1;
    Vehicle->AccessConfiguration().Spatial.PositionMetres[0] = 12.0f;
    Vehicle->AccessConfiguration().Spatial.PositionMetres[1] = 1.0f;
    Vehicle->AccessConfiguration().Spatial.PositionMetres[2] = 0.0f;
    PhysicsExtensionSettings Settings;
    Settings.WriteRegister = false;
    auto Drive = [&](bool UseFacets)
    {
        Settings.CollisionSurfaces = UseFacets ? Surfaces : nullptr;
        PhysicsExtension Physics;
        (void)Physics.Begin(World, Settings);
        if (UseFacets)
        {
            bool Triangles = false;
            for (size_t Index = 0; Index < Physics.QuerySpace()->QueryBodyCount(); ++Index)
                Triangles |= Physics.QuerySpace()->QueryDescriptor(static_cast<uint32_t>(Index)).Record == RampToken.Value &&
                             Physics.QuerySpace()->QueryDescriptor(static_cast<uint32_t>(Index)).Shape == PhysicsShape::Triangles;
            Expect(Triangles, "ramp enters Jolt as a triangle body");
            PhysicsRay Ray;
            Ray.OriginMetres = {24.0, 1.0, 4.0};
            Ray.LengthMetres = 6.0;
            PhysicsRayHit Hit;
            Expect(Physics.QuerySpace()->CastRay(Ray, Hit) && Hit.Normal[2] > 0.9 && std::abs(Hit.Normal[0]) > 0.1,
                   "ray sees the real slope rather than a horizontal box top");
        }
        Physics.AssignVehicleInput(PhysicsVehicleInput{1.0f, 0.0f, 0.0f, false});
        double Farthest = 0.0, Height = 0.0;
        for (int Step = 0; Step < 1440; ++Step)
        {
            Physics.AdvanceSteps(1);
            const auto& Views = Physics.QuerySnapshot().Vehicles;
            if (Views.empty()) break;
            const auto& Position = Views.front().Telemetry.Chassis.Position;
            Farthest = std::max(Farthest, static_cast<double>(Position.x));
            if (Position.x > 24.0f && Position.x < 28.0f) Height = std::max(Height, static_cast<double>(Position.z));
        }
        std::printf("%s: farthest x %.2f m, ramp centre height %.2f m\n", UseFacets ? "facets" : "bounds", Farthest, Height);
        Physics.End(World);
        return std::array<double, 2>{Farthest, Height};
    };
    const auto Bounds = Drive(false);
    const auto Facets = Drive(true);
    Expect(Bounds[0] < 22.0, "bounds-only baseline blocks the test vehicle");
    Expect(Facets[0] > 28.0 && Facets[1] > 1.1, "vehicle drives up and beyond the actual ramp");
    const auto FirstBefore = Found->second.Corners.front();
    Ramp->AccessConfiguration().Spatial.PositionMetres[0] += 5.0f;
    Ramp->AccessConfiguration().AdvanceRevision();
    (void)Occupancy.ResolvePlacements(World);
    const auto Moved = Occupancy.CaptureCollisionSurfaces(World);
    const auto MovedRamp = Moved->find(RampToken.Value);
    Expect(MovedRamp != Moved->end() && std::abs(MovedRamp->second.Corners.front()[0] - FirstBefore[0] - 5.0f) < 0.001f,
           "captured facets follow edited placement");
    Ramp->AccessConfiguration().Physics = true;
    Ramp->AccessConfiguration().AdvanceRevision();
    (void)Occupancy.ResolvePlacements(World);
    Expect(Occupancy.CaptureCollisionSurfaces(World)->count(RampToken.Value) == 0, "moving geometry is not captured as fixed triangles");
    Ramp->AccessConfiguration().Physics = false;
    Ramp->AccessConfiguration().Visible = false;
    Ramp->AccessConfiguration().AdvanceRevision();
    (void)Occupancy.ResolvePlacements(World);
    Expect(Occupancy.CaptureCollisionSurfaces(World)->count(RampToken.Value) == 0, "hidden geometry is excluded");
    std::printf("%d / %d checks passed\n", Checks - Failures, Checks);
    return Failures == 0 ? 0 : 1;
}
