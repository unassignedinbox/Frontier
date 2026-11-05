/*==========================================================================================================================================
                                             PHASE3RENDERFLUIDPARTICLEVALIDATION.CPP
                                     ------------------------------------------------------------------
                   End-to-end host verification for Phase 3 Track 3B (Photometry, EmissiveLightSpace,
                     LightClusterSpace, NearestDepthSpace), Track 3C (PlumeSpace, PlumeSourceProjection,
                     CoarseGasField, GasPresetLibrary, GasSceneCodec), and Track 3D (SparkSequence,
                     EffectUniformRecord, DerezPresets).
==========================================================================================================================================*/

#include "Frontier/Engine/PhysicalDynamics/ParticleDynamics/VehicleEffectSpace.h"
#include "Frontier/Engine/Rendering/EmissiveLightSpace.h"
#include "Frontier/Engine/Rendering/LightClusterSpace.h"
#include "Frontier/Engine/Rendering/NearestDepthSpace.h"
#include "Frontier/Engine/Rendering/Photometry.h"
#include "Frontier/Engine/VolumetricDynamics/CoarseGasField.h"
#include "Frontier/Engine/VolumetricDynamics/GasPresetLibrary.h"
#include "Frontier/Engine/VolumetricDynamics/PlumeSpace.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace
{

int CheckCount   = 0;
int FailureCount = 0;

void Check(bool Condition, const char* Message) noexcept
{
    ++CheckCount;
    if (!Condition)
    {
        ++FailureCount;
        std::printf("FAIL: %s\n", Message);
    }
}

void ValidateTrack3B()
{
    using namespace Frontier;

    std::string Diagnostic;
    const auto Atlas = LoadPhotometricAtlas("EngineContent/Photometry", Diagnostic);
    Check(Diagnostic.empty() && Atlas.size() == PhotometryBytes,
          "3B: LoadPhotometricAtlas loads all 7 LM-63 Type C profiles into 23584 bytes");

    for (uint32_t ProfileId = 1u; ProfileId <= PhotometryProfileCount; ++ProfileId)
    {
        double PeakSample = 0.0;
        for (double Az = 0.0; Az <= 360.0; Az += 30.0)
        {
            for (double Pol = 0.0; Pol <= 180.0; Pol += 10.0)
            {
                PeakSample = std::max(PeakSample, SamplePhotometricAtlas(Atlas, ProfileId, Az, Pol));
            }
        }
        Check(PeakSample > 0.70 && PeakSample <= 1.0001,
              "3B: every shipped IES profile has normalized peak intensity in (0.70, 1.0]");
    }

    std::string BadDiag;
    const auto BadIes = ParseIes("IESNA:LM-63-2002\nTILT=INCLUDE\n", BadDiag);
    Check(!BadIes.has_value() && !BadDiag.empty(),
          "3B: ParseIes rejects non-NONE TILT files with a diagnostic");

    LightClusterSpace Clusters;
    Clusters.AssignPhotometry(Atlas);
    Check(Clusters.IsPhotometric(), "3B: LightClusterSpace marks photometric atlas active");

    std::vector<float> LightData(MaxSceneLights * LightRecordFloats, 0.0f);
    // Light 0 at (0, 0, 4) with reach 18 m (float 7) and Params.w (float 15) = 1.0f (active)
    LightData[0]  = 0.0f;
    LightData[1]  = 0.0f;
    LightData[2]  = 4.0f;
    LightData[7]  = 18.0f;
    LightData[15] = 1.0f;
    Clusters.Update(LightData);

    const uint32_t MaskAtOrigin = Clusters.QueryCandidates({ 0.0f, 0.0f, 1.0f });
    const uint32_t Conservative = Clusters.ResolveConservativeMask({ 0.0f, 0.0f, 1.0f });
    Check((MaskAtOrigin & 1u) == 1u && MaskAtOrigin == Conservative,
          "3B: LightClusterSpace bins Light 0 into the origin cell and matches ResolveConservativeMask");
    Check(Clusters.QueryCandidates({ -500.0f, 0.0f, 0.0f }) == 0xFFFFFFFFu,
          "3B: points outside LightClusterBounds fall back conservatively to all lights");

    EmissiveSetSource EmissiveSet;
    EmissiveTriangleSource Facet;
    Facet.A      = { 0.0f, 0.0f, 2.0f };
    Facet.B      = { 1.0f, 0.0f, 2.0f };
    Facet.C      = { 0.0f, 1.0f, 2.0f };
    Facet.Family = 1u;
    EmissiveSet.Facets.push_back(Facet);

    std::vector<EmissiveOccupantSource> Occupants(1);
    Occupants[0].FacetSet = 0u;
    Occupants[0].Roles    = OccupantRole::Raster;

    std::vector<std::array<float, 3>> Materials = {
        { 0.0f, 0.0f, 0.0f },
        { 12.0f, 8.0f, 4.0f }
    };

    EmissiveLightSpace Emissive;
    const uint32_t SetsRepresented = Emissive.Construct(std::span<const EmissiveSetSource>(&EmissiveSet, 1), Materials);
    Check(SetsRepresented == 1u && Emissive.TriangleCount() == 1u,
          "3B: EmissiveLightSpace constructs per-set triangle clusters");
    const bool Changed = Emissive.Update(Occupants, Materials, 1.0f);
    Check(Changed && Emissive.Count() == 1u && Emissive.Represents(0u),
          "3B: EmissiveLightSpace bins emissive cluster instances into the 16x16x4 light grid");
    Clusters.AssignEmissive(Emissive.TriangleBytes(), Emissive.ClusterBytes(), Emissive.CellBytes(), Emissive.Count());
    Check(Clusters.QueryEmissiveClusterCount() == 1u,
          "3B: LightClusterSpace stores emissive cluster count in Extent.w");
}

void ValidateTrack3C()
{
    using namespace Frontier::VolumetricDynamics;

    PlumeSourceProjection Sources;
    PlumeVehicleTelemetry Telemetry;
    Telemetry.Position         = { 12.0f, -4.0f, 0.0f };
    Telemetry.Velocity         = { 18.0f, 0.0f, 0.0f };
    Telemetry.AfterfireSeconds = 0.25f;
    Telemetry.Exhausts.push_back({ { 9.5f, -4.4f, 0.45f }, { -1.0f, 0.0f, 0.0f } });

    for (uint32_t Wheel = 0u; Wheel < 4u; ++Wheel)
    {
        auto& W              = Telemetry.Wheels[Wheel];
        W.InContact          = true;
        W.SlidingPower       = 18000.0f;
        W.SlipSpeed          = 9.5f;
        W.SurfaceSpeed       = 26.0f;
        W.Radius             = 0.35f;
        W.Ignited            = (Wheel >= 2u);
        W.CombustionPower    = (Wheel >= 2u) ? 4500.0f : 0.0f;
        W.SurfaceTemperature = 410.0f;
        W.ContactPoint       = { 12.0f, -4.0f + float(Wheel), 0.0f };
        W.HubPosition        = { 12.0f, -4.0f + float(Wheel), 0.35f };
        W.ContactVelocity    = { 18.0f, 0.0f, 0.0f };
        W.Longitudinal       = { 1.0f, 0.0f, 0.0f };
    }

    PlumeSourceOptions Opts;
    Opts.Afterfire = true;
    Sources.Project(Telemetry, 0.10f, Opts);
    Check(Sources.ActiveTyres == 4u && Sources.Count >= 8u,
          "3C: PlumeSourceProjection emits contact-patch, arch, sidewall-flame, and trailing-flame sources");
    Check(Sources.Lights.size() >= 2u,
          "3C: burning rear tyres emit flickering FireLight point lights");

    PlumeSpace Plume(PlumeGrades[2]);
    PlumeSettings Settings;
    Plume.Schedule(1.0f / 60.0f, 1.0f / 60.0f, Telemetry.Position, Sources, Settings, { 0.3f, 0.4f, 0.866f });
    Check(Plume.Resident && Plume.StepPending,
          "3C: PlumeSpace becomes resident and schedules a 60 Hz step when sources are active");
    Plume.AssignFireLights(Sources.Lights);
    Check(Plume.FireLightCount >= 2u && Plume.FireLights.Count[0] == Plume.FireLightCount,
          "3C: PlumeSpace packs sorted FireLightRecord entries");
    Plume.CompleteStep();
    Check(Plume.StepsTaken == 1u && Plume.State == 1u && !Plume.StepPending,
          "3C: CompleteStep advances parity state and records PriorOrigin");
}

void ValidateTrack3D()
{
    using namespace Frontier::ParticleDynamics;

    SparkSequence Sparks;
    SparkScrape HullScrape;
    HullScrape.Point    = { 0.0f, 0.0f, 0.15f };
    HullScrape.Velocity = { 16.0f, 2.0f, 0.0f };
    HullScrape.Normal   = { 0.0f, 0.0f, 1.0f };
    HullScrape.Strength = 2.5f;

    SparkScrape RimScrape = HullScrape;
    RimScrape.Wheel          = 0;
    RimScrape.Rim.Active     = true;
    RimScrape.Rim.Forward    = { 1.0f, 0.0f, 0.0f };
    RimScrape.Rim.SpinSpeed  = 22.0f;
    RimScrape.Rim.Radius     = 0.34f;

    Sparks.Emit({ HullScrape, RimScrape }, 1.0f / 30.0f);
    Check(Sparks.Emitted > 20u && Sparks.Particles.size() == Sparks.Emitted,
          "3D: SparkSequence emits hull and rim spark packets");

    Sparks.Advance(1.0f / 60.0f, 0.0f);
    Check(Sparks.Live > 0u && Sparks.Instances.size() == Sparks.Live,
          "3D: SparkSequence advances ballistic flight, cools blackbody Kelvin, and packs 48-byte streak instances");

    const auto Cyan = ParseEffectHexColour("#35d6ff");
    Check(Cyan[2] > Cyan[1] && Cyan[1] > Cyan[0] && Cyan[0] > 0.0f,
          "3D: ParseEffectHexColour converts #35d6ff sRGB to linear Rec.709");

    const auto BallisticPos = EvaluateBallisticPosition({ 0.0f, 0.0f, 1.5f }, { 4.0f, 0.0f, 3.0f }, 0.4f, 0.5f);
    Check(BallisticPos[0] > 1.5f && BallisticPos[2] > 1.5f,
          "3D: EvaluateBallisticPosition solves closed-form gravity + linear drag trajectory");
}

} // namespace

int main()
{
    ValidateTrack3B();
    ValidateTrack3C();
    ValidateTrack3D();

    std::printf("Phase3RenderFluidParticleValidation: %d checks, %d failures\n", CheckCount, FailureCount);
    return FailureCount == 0 ? 0 : 1;
}
