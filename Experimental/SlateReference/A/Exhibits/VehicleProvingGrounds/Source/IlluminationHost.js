import { VehicleOrbitCamera } from "./VehicleOrbitCamera.js";
import { DrivingCameraMotion } from "./DrivingCameraMotion.js";
import { ResolveHudStyle } from "./HudLayout.js";
import { VehicleAppearance } from "./VehicleAppearance.js";
import { LightingSystem, LightBufferBytes, ClusterCellBytes, ClusterIndexBytes, AutomotiveMode, RoofFixtureType } from "./LightingSystem.js";
//============================================================================================================================================
//                                                           ILLUMINATIONHOST.JS
//============================================================================================================================================
// 🧩 Exhibit entry point: bring-up of every stage, then the frame — drive, invalidate, relight, gather, resolve, present.

import { DeviceExchange } from "./DeviceExchange.js";
import { ShaderInterchange } from "./ShaderInterchange.js";
import { SceneStructure, OccupantStride, GroundLayout, OccupantRole } from "./FacetStructure.js";
import { ConstructMaterialSpecification, EncodeMaterialRecords, CourseMaterial, MarkedFamilyFirst, MarkedFamilyLast,
         MarkedFamilyExtraFirst, MarkedFamilyExtraLast, IorToF0 } from "./MaterialSpecification.js";
import { TyreMarkDepot, TyreMarkLayout } from "./TyreMarkDepot.js";
import { OccupantDistanceSpace } from "./OccupantDistanceSpace.js";
import { SceneDistanceSpace, ResolveMovedBounds } from "./SceneDistanceSpace.js";
import { RayTracingSpace } from "./RayTracingSpace.js";
import { LevelCatalog, ResolveLevelDescriptor } from "./LevelStructure.js";
import { SurfaceCardDepot } from "./SurfaceCardDepot.js";
import { AtmosphereIntegrator } from "./AtmosphereIntegrator.js";
import { ShadowProjection } from "./ShadowProjection.js";
import { SurfaceProjection } from "./SurfaceProjection.js";
import { NearestDepthSpace } from "./NearestDepthSpace.js";
import { ResamplingSequence } from "./ResamplingSequence.js";
import { VarianceIntegrator } from "./VarianceIntegrator.js";
import { DisplayProjection } from "./DisplayProjection.js";
import { HardwareMetrics } from "./HardwareMetrics.js";
import { ComposeTimingLog as ComposeTimingLogSnapshot } from "./TimingLog.js";
import { CameraMotionBlur } from "./CameraMotionBlur.js";
import { EditorShell } from "./EditorShell.js";
import { LightingPresets } from "./InspectorPages.js";
import { OverlayDock, OverlayKeys } from "./OverlayDock.js";
import { StressSequence } from "./StressSequence.js";
import { FractureSequence } from "./FractureSequence.js";
import { JoltWorld } from "./JoltWorld.js";
import { ConeSequence } from "./ConeSequence.js";
import { EffectSequence } from "./EffectSequence.js";
import { RecoveryDirector, RecoveryReasons } from "./RecoveryDirector.js";
import { TrainingSequence } from "./TrainingSequence.js";
import { SpawnPoint, ResolveCourseHeight, AssignCourseReliefEnabled, AssignCustomTerrainHeight, AssignExcavationCutouts } from "./CourseStructure.js";
import { BollardSequence } from "./BollardSequence.js";
import { TyreThermalSolver, IgnitionModes } from "./TyreThermalSolver.js";
import { SmokeSequence, ConstructGasParameters } from "./SmokeSequence.js";
import { VehicleSolver } from "./VehicleSolver.js";
import { DriverInputIntegrator } from "./DriverInputIntegrator.js";
import { ChaseCameraSolver, FreeCameraSolver } from "./ChaseCameraSolver.js";
import { ConstructViewTransform, ConstructPerspectiveTransform, MultiplyTransform, InvertTransform, NormalizeVector,
         SubtractVector, MultiplyAddVector, ProjectRadicalInverse, TransformPoint, TransformDirection, CrossVector, AddVector,
         ScaleVector, LengthVector, RotateVector } from "./TransformProjection.js";
import { VehicleVisualState, VehicleRecordBytes } from "./VehicleVisualState.js";
import { ConstructHullPoints, HullPointRadius } from "./HullStructure.js";
import { SparkSequence } from "./SparkSequence.js";
import { CabinSequence } from "./CabinSequence.js?overlay=13";
import { CabinLayout, CameraRigs, ResolveCockpitFrame, PlaceSeatedEye, ResolveTabletScale, ResolveHmiCamera } from "./CabinStructure.js";
import { DrivingHudSequence } from "./DrivingHudSequence.js";
import { DDayCombatSequence } from "./DDayCombatSequence.js";
import { CandyVillageSequence } from "./CandyVillageSequence.js";
import { FoundryConveyorSequence } from "./FoundryConveyorSequence.js";
import { DemolishRunSequence } from "./DemolishRunSequence.js";
import { PrepareDemolishCity } from "./DemolishRunStructure.js";
import { PistonLabyrinthSequence } from "./PistonLabyrinthSequence.js";
import { PrepareLabyrinthMaze } from "./PistonLabyrinthStructure.js";
import { VehicleGeometry } from "./VehicleSpecification.js";
import { PickVehicle } from "./VehiclePicking.js";
import { LobbyVenues, ResolveLobbyRequested, ResolveLobbyVenue, ResolveLobbyVenueDescriptor, ResolveLobbyStudioLights } from "./LobbyStructure.js";
import { EaseCinematic, InterpolateOrbitPose, ResolveLagBlend, BlendVector, ClampStudioEye, LobbyCinematicShots,
         EvaluateCinematicReelShot, BlendCinematicPoses } from "./LobbyCameraMotion.js";
import { LobbyRosterSequence } from "./LobbyRosterSequence.js";
import { LobbyAudio } from "./LobbyAudio.js";



const ShaderSources = [
    "SceneRecords.wgsl", "FrameDescriptor.wgsl", "PackingCodec.wgsl", "DiscrepancySequence.wgsl", "SkyProjection.wgsl",
    "DistanceRecords.wgsl", "DistanceTraversal.wgsl", "DistanceTransfer.wgsl", "DistanceUnion.wgsl", "CardProjection.wgsl",
    "CardSampling.wgsl", "CardCapture.wgsl", "CardIrradiance.wgsl", "CardDirect.wgsl", "CardRadiosity.wgsl", "CardFinal.wgsl",
    "AtmosphereScattering.wgsl", "SurfaceReflectance.wgsl", "ShadowRaster.wgsl", "SurfaceRaster.wgsl", "DirectRadiance.wgsl",
    "ReservoirRecords.wgsl", "ReservoirTemporal.wgsl", "ReservoirSpatial.wgsl", "ReflectionTraversal.wgsl", "RayTraversal.wgsl", "VarianceTemporal.wgsl",
    "VarianceWavelet.wgsl", "VarianceUpsample.wgsl", "InspectionMarch.wgsl", "DisplayRecombination.wgsl", "DisplayAntialias.wgsl",
    "DisplayExposure.wgsl", "DisplayPresent.wgsl", "FireIllumination.wgsl", "PlumeRecords.wgsl", "PlumeAdvection.wgsl", "PlumePressure.wgsl", "PlumeVorticity.wgsl", "PlumeMarch.wgsl",
    "FragmentDistance.wgsl", "EffectRecords.wgsl", "EffectRaster.wgsl", "VehicleVisual.wgsl", "SparkRaster.wgsl", "ForwardRaster.wgsl",
    "CabinOverlay.wgsl", "CabinVolume.wgsl", "ClusteredLights.wgsl", "ScreenTraversal.wgsl", "NearestDepthTransfer.wgsl",
    "NearestDepthReduction.wgsl"
];

// 📝 Quality grades trade cascade resolution, card density and traversal length; selected with ?quality=low|medium|high.
//    ScreenSteps bounds the nearest-depth walk of a screen trace (Lumen's screen-trace iteration limit).
const QualityGrades = {
    high:   { ClipResolution: 128, FinestVoxel: 0.125, CardExtent: 2048, CardDensity: 1.0,  VoxelScale: 1.0, ShadowExtent: 2048,
              Steps: 96, ScreenSteps: 64, RenderScale: 1.0,  TileBudget: 2048, PixelLimit: 1920 * 1080 },
    medium: { ClipResolution: 96,  FinestVoxel: 0.16,  CardExtent: 2048, CardDensity: 0.75, VoxelScale: 1.4, ShadowExtent: 2048,
              Steps: 80, ScreenSteps: 48, RenderScale: 0.85, TileBudget: 1024, PixelLimit: 1600 * 900 },
    low:    { ClipResolution: 64,  FinestVoxel: 0.25,  CardExtent: 1024, CardDensity: 0.5,  VoxelScale: 2.2, ShadowExtent: 1024,
              Steps: 64, ScreenSteps: 32, RenderScale: 0.6,  TileBudget: 384,  PixelLimit: 1280 * 720 }
};

const FeatureBits = { Indirect: 1, TemporalReuse: 2, SpatialReuse: 4, VisibilityReuse: 8, VarianceKernel: 16, Reflection: 32,
                      CardRadiosity: 64, Antialias: 128, DirectShadow: 256, AutoExposure: 512, RadianceRefresh: 1024,
                      ScreenTraversal: 8192, TriangleReflection: 16384 };

const TemporalRestartBit = 2048;   // [-] FeatureTemporalRestart in SceneRecords.wgsl

const PassNames = ["Distance union", "Card lighting", "Shadow", "Surface", "Depth pyramid", "Direct", "ReSTIR GI", "Diffuse reconstruction",
                   "Reflection", "Glossy reconstruction", "Resolve", "Smoke transport", "Smoke render"];

const FrameRecordExtent    = 528;   // [B] FrameRecord in SceneRecords.wgsl (132 32-bit fields incl. dynamic ground card/occupant indices)
const MaxOccupantCapacity  = 4096;  // [-] Fixed GPU OccupantRecords capacity so in-place scene swaps never recreate bind groups
const FireLightReach       = 14.0;  // [m] influence radius of a tyre fire's light

//------------------------------------------------------------------------------------------------------------------------
//                                                       BRING-UP
//------------------------------------------------------------------------------------------------------------------------

class IlluminationHost
{
    constructor()
    {
        const Query = new URLSearchParams(location.search);
        const RequestedLevel = Query.get("level");
        const LobbyRequested = RequestedLevel === "lobby" || (!RequestedLevel && ResolveLobbyRequested());
        this.Lobby = LobbyRequested;
        this.LobbyVenueKey = this.Lobby ? ResolveLobbyVenue() : "studio";
        this.LobbyVenue = ResolveLobbyVenueDescriptor(this.LobbyVenueKey);
        if (this.Lobby)
            try { sessionStorage.setItem("frontier.lobbyVenue", this.LobbyVenueKey); } catch {}
        this.LobbyAudio = new LobbyAudio();
        this.LevelKey = this.Lobby
            ? "lobby"
            : (LevelCatalog.some((Entry) => Entry.Key === RequestedLevel) ? RequestedLevel : "inversion");
        let SavedRoute = "inversion";
        try { SavedRoute = sessionStorage.getItem("frontier.selectedLevel") || "inversion"; } catch {}
        if (!LevelCatalog.some((Entry) => Entry.Key === SavedRoute))
            SavedRoute = "inversion";
        this.SelectedRouteLevel = this.Lobby ? SavedRoute : this.LevelKey;
        if (!this.Lobby)
            try { sessionStorage.setItem("frontier.selectedLevel", this.LevelKey); } catch {}
        if (typeof document !== "undefined" && document.body)
            document.body.classList.toggle("workspace-lobby", this.Lobby);

        if (this.LevelKey === "demolish")
        {
            const SeedParam = Query.get("seed");
            PrepareDemolishCity(SeedParam !== null && SeedParam !== "" ? Number(SeedParam) : null);
        }
        if (this.LevelKey === "labyrinth")
        {
            const SeedParam = Query.get("seed");
            PrepareLabyrinthMaze(SeedParam !== null && SeedParam !== "" ? Number(SeedParam) : null);
        }

        this.LevelDescriptor = ResolveLevelDescriptor(this.LevelKey, this.LobbyVenueKey);
        const VenueLight = this.Lobby ? this.LobbyVenue.Lighting : null;
        this.Spawn = this.LevelDescriptor.Spawn;
        this.LevelZones = this.LevelDescriptor.Zones;
        AssignCourseReliefEnabled(this.LevelDescriptor.Relief);
        AssignCustomTerrainHeight(this.LevelDescriptor.TerrainHeight || null);
        AssignExcavationCutouts(this.LevelDescriptor.PhysicsCutouts || []);
        this.GradeName = QualityGrades[Query.get("quality")] ? Query.get("quality") : "high";
        this.Grade = { ...QualityGrades[this.GradeName] };
        if (Query.has("scale"))
            this.Grade.RenderScale = Math.max(0.25, Math.min(2.0, Number(Query.get("scale"))));
        this.Settings = {
            Level: this.Lobby ? this.SelectedRouteLevel : this.LevelKey,
            AeroGripScale: this.LevelDescriptor.AeroGrip,
            Inspection: Number(Query.get("inspection") || 0), DistanceInspection: 1, InsetEnabled: false, SliceHeight: 0.6,
            HudVisible: false, PanelVisible: !this.Lobby,
            SmokeEnabled: !this.Lobby, SmokeResolution: 2, IgnitionMode: 0, ThermalGrip: false, LineLock: true,
            TyreMarks: !this.Lobby, TyreMarkStrength: 1.0,
            Gas: ConstructGasParameters(),
            SunElevation: VenueLight ? VenueLight.SunElevation : (this.LevelKey === "labyrinth" ? 54.0 : 38.0),
            SunAzimuth: VenueLight ? VenueLight.SunAzimuth : (this.LevelKey === "labyrinth" ? 132.0 : 145.0),
            SolarIlluminance: VenueLight ? VenueLight.SolarIlluminance : 12.0,
            SkyScale: VenueLight ? VenueLight.SkyScale : 1.0,
            EmissionScale: VenueLight ? VenueLight.EmissionScale : 1.0,
            ExposureCompensation: 0.0, AutoExposure: true, DirectShadow: true,
            Indirect: true, ScreenTraversal: Query.has("screentraces"), TemporalReuse: true, SpatialReuse: true, VisibilityReuse: true,
            RadianceRefresh: true,
            SpatialCount: 5, SpatialRadius: 20, TemporalTallyLimit: 20, IndirectScale: 1.0, VarianceKernel: true, Reflection: true,
            TriangleReflection: Query.has("triangles"),
            Antialias: true, TraceScale: 2, FreezeDistance: false, CardRadiosity: true, CardTileBudget: this.Grade.TileBudget, GlassIor: 1.5,
            Quality: ["low", "medium", "high"].indexOf(this.GradeName),
            RenderScale: this.Grade.RenderScale, LightingPreset: "Custom", TestPlume: 0,
            // 📝 Tyres: cold pressures per axle [kPa], wear, failure and optional keyboard countersteer.
            TyrePressureFront: 220, TyrePressureRear: 220, SteeringAssist: true, SteeringAssistStrength: 0.45,
            TyreWear: true, WearScale: 1.0, TyreFailure: true, Sparks: !this.Lobby,
            HudScale: 1, HudAcross: 0, HudDown: 0,
            CameraShake: true, CameraShakeStrength: 0.5, SpeedFov: true, SpeedFovBoost: 14,
            HmiScale: 1, HmiZoom: 1, CockpitFov: 70,
            CameraMode: Query.get("camera") || (Query.get("hmi") === "garage" ? "HMI" : (this.Lobby ? "Orbit" : "Chase"))
        };
        if (Query.has("preset"))
            this.PendingPreset = Query.get("preset");
        // 📝 Stress test: ?stresspool=K reserves K objects at bring-up (default 1536, clamped to the identity ceiling),
        //    ?stress=N enables N of them at start and ?stressmotion=0|1|2 sets none, one in ten or all of them moving —
        //    a URL that reproduces a benchmark exactly.
        this.StressCapacity = this.Lobby ? 0 : (Query.has("stresspool") ? Math.max(0, Number(Query.get("stresspool")) || 0) : 1536);
        this.StressInitial = Math.max(0, Number(Query.get("stress")) || 0);
        this.Settings.StressMotion = Math.max(0, Math.min(2, Number(Query.get("stressmotion")) || 0));
        this.FrameDurations = new Float32Array(240);
        this.FrameDurationCursor = 0;
        this.Keys = new Set();
        this.FrameIndex = 0;
        this.ActiveCamera = { Name: "Vehicle Proving Grounds camera", MotionBlur: new CameraMotionBlur() };
        this.PerformanceCapture = { Enabled: Query.get("perfLog") === "1" };
        this.PerformanceWindowSeconds = 0.0;
        this.PerformanceWindowSamples = this.ConstructPerformanceSamples();
        this.PerformanceLastWindowSamples = null;
        this.OptimizationRestorePoint = null;
        this.FreeCamera = false;
        this.Ready = false;
        this.ScriptedInput = null;
        // 🔍 Diagnostic only: ?omit=Union,Cards,Shadow,... leaves the named stages out of the frame.
        this.Omitted = new Set((Query.get("omit") || "").split(",").filter((Name) => Name.length > 0));
        // 🔍 Diagnostic only: ?capture=1 presents into an offscreen texture read back into a 2D canvas, for hosts
        //    (headless software adapters) whose WebGPU canvas presentation is unavailable.
        this.Capture = Query.get("capture") === "1";
        this.WorkspaceUiBound = false;
        this.BindWorkspaceAndLobbyUi();
    }

    ReportProgress(Caption, Fraction)
    {
        if (this.LastCaption !== Caption)
            console.info(`[bring-up] ${(performance.now() / 1000).toFixed(2)} s · ${Caption}`);
        this.LastCaption = Caption;
        const Overlay = document.getElementById("bring-up");
        if (!Overlay)
            return;
        Overlay.querySelector(".bring-up-caption").textContent = Caption;
        Overlay.querySelector(".bring-up-bar > div").style.width = `${Math.round(Math.min(1.0, Math.max(0.0, Fraction)) * 100)}%`;
    }

    async ConstructExhibit()
    {
        this.ReportProgress("Negotiating the WebGPU device", 0.02);
        this.Exchange = new DeviceExchange();
        const Device = await this.Exchange.ConstructDevice();
        this.Exchange.ErrorListeners.push((Message) => this.ReportFailure(Message, false));
        this.Canvas = document.getElementById("viewport");
        if (this.Capture)
        {
            this.Exchange.CanvasFormat = "rgba8unorm";
        }
        else
        {
            this.Context = this.Canvas.getContext("webgpu");
            this.Context.configure({ device: Device, format: this.Exchange.CanvasFormat, alphaMode: "opaque",
                                     viewFormats: [`${this.Exchange.CanvasFormat}-srgb`] });
        }

        this.ReportProgress("Retrieving shaders and the ControlVehicle archives", 0.05);
        this.Shaders = new ShaderInterchange(this.Exchange, "Shader");
        await this.Shaders.RetrieveSources(ShaderSources);
        this.Scene = new SceneStructure();
        await this.Scene.ConstructScene("Archive", this.StressCapacity, this.LevelKey, this.LobbyVenueKey);
        this.Vehicle = new VehicleSolver(this.Scene.QueryObstacleBounds());
        this.Vehicle.Surfaces = this.LevelDescriptor.Surfaces;
        this.Vehicle.AeroGripScale = this.Settings.AeroGripScale;
        const SpawnBaseZ = this.Lobby ? 0.0 : ResolveCourseHeight(this.Spawn.Position[0], this.Spawn.Position[1]);
        this.Vehicle.Respawn(
            this.Lobby
                ? [0.0, 0.0, VehicleGeometry.CoMHeight]
                : [this.Spawn.Position[0], this.Spawn.Position[1], SpawnBaseZ + VehicleGeometry.CoMHeight + 0.02],
            this.Lobby ? 0.0 : this.Spawn.Yaw
        );
        // 📝 The body's own hull: ~700 points ray-cast from its facets (HullStructure.js) meet the course and the boxes.
        this.Vehicle.AssignHullPoints(ConstructHullPoints(this.Scene.FacetSets[this.Scene.VehicleBodySet].Facets), HullPointRadius);
        this.PositionVehicle();
        this.Scene.RetainOccupantTransform(this.Scene.VehicleOccupants.Body);
        for (const Wheel of this.Scene.VehicleOccupants.Wheels)
            this.Scene.RetainOccupantTransform(Wheel);

        // ① records every stage shares
        const Materials = ConstructMaterialSpecification();
        if (this.Lobby)
        {
            // Studio lenses: smoked glass, not a blown-out light bar in the showroom.
            Materials[CourseMaterial.Headlight].Emission = [0.0, 0.0, 0.0];
            Materials[CourseMaterial.Headlight].Albedo   = [0.05, 0.055, 0.06];
        }
        this.VertexRecords   = this.Exchange.ConstructRecordsFrom(this.Scene.EncodeVertexRecords(), GPUBufferUsage.VERTEX | GPUBufferUsage.COPY_DST, "Facet vertices");
        this.Materials = Materials;
        // 📝 COPY_DST: the derez palette (VFX editor) rewrites its three glow records in place.
        this.MaterialRecords = this.Exchange.ConstructRecordsFrom(EncodeMaterialRecords(Materials), GPUBufferUsage.STORAGE | GPUBufferUsage.COPY_DST, "Materials");
        this.FrameRecords    = this.Exchange.ConstructRecords(FrameRecordExtent, GPUBufferUsage.UNIFORM | GPUBufferUsage.COPY_DST, "Frame record");
        this.LightRecords    = this.Exchange.ConstructRecords(LightBufferBytes, GPUBufferUsage.STORAGE | GPUBufferUsage.COPY_DST, "Clustered light records");
        this.ClusterRecords  = { Cells:   this.Exchange.ConstructRecords(ClusterCellBytes, GPUBufferUsage.STORAGE | GPUBufferUsage.COPY_DST, "Light cluster cells"),
                                 Indices: this.Exchange.ConstructRecords(ClusterIndexBytes, GPUBufferUsage.STORAGE | GPUBufferUsage.COPY_DST, "Light cluster indices") };
        this.OccupantRecords = this.Exchange.ConstructRecords(Math.max(MaxOccupantCapacity, this.Scene.Occupants.length) * OccupantStride,
                                                              GPUBufferUsage.STORAGE | GPUBufferUsage.COPY_DST, "Occupant records");
        this.Lighting        = new LightingSystem();
        this.Lighting.ScrollingGrid = this.LevelKey === "inversion";
        if (this.Lobby)
            this.Lighting.StudioLights = ResolveLobbyStudioLights(this.LobbyVenueKey);

        // ② per-occupant distance volumes (the analogue of mesh distance fields), transferred on the device
        this.Volumes = new OccupantDistanceSpace(this.Exchange, this.Shaders);
        await this.Volumes.ConstructVolumes(this.Scene, this.Grade.VoxelScale, (Caption, Fraction) => this.ReportProgress(Caption, 0.08 + 0.55 * Fraction));

        // ③ surface cards, then the occupant records that reference volumes and cards
        this.ReportProgress("Allocating surface cards", 0.66);
        this.Cards = new SurfaceCardDepot(this.Exchange, this.Shaders, { Extent: this.Grade.CardExtent, DensityScale: this.Grade.CardDensity });
        this.Cards.AllocateCards(this.Scene);
        if (new URLSearchParams(location.search).has("warm"))
            this.Cards.WarmFrames = Number(new URLSearchParams(location.search).get("warm"));
        Device.queue.writeBuffer(this.OccupantRecords, 0, this.Scene.EncodeOccupantRecords(this.Volumes));
        this.Scene.ConstructDrawRuns();

        // 📝 The triangle hierarchy behind the reflection toggle: built once here, refit per frame below only while
        //    the toggle is on. The buffers still join the reflection bind group while off — unbound slots fail
        //    validation — so the toggle itself stays a one-frame feature-bit flip.
        this.ReportProgress("Building the triangle hierarchy", 0.69);
        this.RayTracing = new RayTracingSpace(this.Exchange);
        this.RayTracing.ConstructResources(this.Scene);

        this.StreetLampPoles = this.ResolveStreetLampPoles(this.LevelDescriptor);
        this.Constants = {
            MaximumTraversalSteps: { Type: "u32", Value: this.Grade.Steps },
            ScreenTraversalSteps:  { Type: "u32", Value: this.Grade.ScreenSteps },
            GroundPadMinimumX:     { Type: "f32", Value: GroundLayout.PadMinimum[0] },
            GroundPadMinimumY:     { Type: "f32", Value: GroundLayout.PadMinimum[1] },
            GroundTileExtent:      { Type: "f32", Value: GroundLayout.TileExtent },
            GroundTileCountX:      { Type: "u32", Value: GroundLayout.TileCount[0] },
            GroundTileCountY:      { Type: "u32", Value: GroundLayout.TileCount[1] },
            TyreMarkOriginX:       { Type: "f32", Value: TyreMarkLayout.Origin[0] },
            TyreMarkOriginY:       { Type: "f32", Value: TyreMarkLayout.Origin[1] },
            TyreMarkExtentX:       { Type: "f32", Value: TyreMarkLayout.Extent[0] },
            TyreMarkExtentY:       { Type: "f32", Value: TyreMarkLayout.Extent[1] },
            TyreMarkPatchWidth:    { Type: "f32", Value: TyreMarkLayout.PatchWidth },
            RimMarkFamily:         { Type: "u32", Value: CourseMaterial.RimScrape },
            TyreMarkFamily:        { Type: "u32", Value: CourseMaterial.TyreRubber },
            MarkedFamilyFirst:     { Type: "u32", Value: MarkedFamilyFirst },
            MarkedFamilyLast:      { Type: "u32", Value: MarkedFamilyLast },
            MarkedFamilyExtraFirst: { Type: "u32", Value: MarkedFamilyExtraFirst },
            MarkedFamilyExtraLast: { Type: "u32", Value: MarkedFamilyExtraLast },
            DerezGlowFamily:       { Type: "u32", Value: CourseMaterial.DerezGlow },
            DerezHotFamily:        { Type: "u32", Value: CourseMaterial.DerezHot },
            DerezGlitchFamily:     { Type: "u32", Value: CourseMaterial.DerezGlitch },
            VehicleGlassFamily:    { Type: "u32", Value: CourseMaterial.VehicleGlass },
            SteeringWheelFamily:   { Type: "u32", Value: CourseMaterial.SteeringWheel },
            TabletHousingFamily:  { Type: "u32", Value: CourseMaterial.TabletHousing },
            DisplayGlassFamily:   { Type: "u32", Value: CourseMaterial.DisplayGlass },
            TyreFamily:            { Type: "u32", Value: CourseMaterial.Tyre },
            CockpitFamilyFirst:    { Type: "u32", Value: CourseMaterial.CabinLeather },
            CockpitFamilyLast:     { Type: "u32", Value: CourseMaterial.DisplayGlass },
            SeatFabricFamily:      { Type: "u32", Value: CourseMaterial.SeatFabric },
            MirrorGlassFamily:     { Type: "u32", Value: CourseMaterial.MirrorGlass },
            ConeOrangeFamily:      { Type: "u32", Value: CourseMaterial.ConeOrange },
            ConeBandFamily:        { Type: "u32", Value: CourseMaterial.ConeBand },
            ConeBaseFamily:        { Type: "u32", Value: CourseMaterial.ConeBase },
            SurroundFamily:        { Type: "u32", Value: CourseMaterial.Surround }
        };
        const InheritedConstruct = this.Shaders.ConstructModule.bind(this.Shaders);
        this.Shaders.ConstructModule = (FileName, Constants = {}, Label = FileName) => InheritedConstruct(FileName, { ...this.Constants, ...Constants }, Label);

        // ④ the scene distance field
        this.ReportProgress("Building the scene distance field cascades", 0.70);
        this.Distance = new SceneDistanceSpace(this.Exchange, this.Shaders, { ClipCount: 4, Resolution: this.Grade.ClipResolution,
                                                                             FinestVoxel: this.Grade.FinestVoxel, BandVoxels: 6, HitFraction: 0.2 });
        this.Distance.Cutouts = this.LevelDescriptor.DistanceCutouts || [];
        await this.Distance.ConstructResources(this.Volumes.TextureView, this.OccupantRecords);
        this.Distance.UploadDistanceList(this.Scene.DistanceList);
        await this.Volumes.ConstructFragmentProgram();
        this.Stress = new StressSequence(this);
        // 📝 The rigid-body world every loose thing shares: debris and cones; the car's wheels can ride heavy pieces.
        this.Bodies = await JoltWorld.Create({
            CourseRelief: this.LevelDescriptor.Relief,
            TerrainHulls: this.LevelDescriptor.TerrainHulls
        });
        this.Bodies.AssignObstacles(this.Scene.QueryObstacleBounds());
        this.Vehicle.Support = this.Bodies;
        this.Cones = new ConeSequence(this);
        this.Bollards = new BollardSequence(this);
        this.Collapse = this.Scene.Collapse;
        this.Trams = this.Scene.Trams;
        if (this.Trams)
            this.Trams.RegisterDynamicObstacles(this.Vehicle.Obstacles);
        this.ServiceStations = this.Scene.ServiceStations;
        if (this.ServiceStations)
            this.ServiceStations.OnAnnounce = (Message) => this.Shell?.Announce(Message);
        this.TrafficLights = this.Scene.TrafficLights;
        if (this.TrafficLights)
        {
            this.TrafficLights.Advance(0.0);
            this.UploadTrafficLightOccupants();
        }
        this.RoadEditor = this.Scene.RoadEditor;
        if (this.RoadEditor)
            this.RoadEditor.AttachHost(this);
        this.ReportProgress("Baking fracture patterns", 0.72);
        this.Fracture = new FractureSequence(this);
        this.DrivingHud = new DrivingHudSequence(this);
        this.DDayCombat = new DDayCombatSequence(this);
        this.UploadDDayOccupants();
        this.CandyVillage = new CandyVillageSequence(this);
        this.UploadCandyOccupants();
        this.FoundryConveyor = new FoundryConveyorSequence(this);
        this.UploadFoundryOccupants();
        this.DemolishRun = new DemolishRunSequence(this);
        this.UploadDemolishOccupants();
        this.PistonLabyrinth = new PistonLabyrinthSequence(this);
        this.UploadLabyrinthOccupants();
        this.LobbyRoster = new LobbyRosterSequence(this);
        this.UploadLobbyOccupants();

        // ⑤ every remaining stage
        this.ReportProgress("Compiling lighting programs", 0.76);
        this.Atmosphere = new AtmosphereIntegrator(this.Exchange, this.Shaders);
        await this.Atmosphere.ConstructResources();
        this.Shadow = new ShadowProjection(this.Exchange, this.Shaders, this.Grade.ShadowExtent);
        await this.Shadow.ConstructResources(this.OccupantRecords);
        await this.Cards.ConstructResources(this.FrameRecords, this.OccupantRecords, this.MaterialRecords, this.Distance.TraversalLayout, this.LightRecords,
                                             this.ClusterRecords);
        this.SamplingGroup = this.Cards.ConstructSamplingGroup(this.OccupantRecords, this.Atmosphere.SkyView, this.Atmosphere.SkySampler);
        this.ReportProgress("Compiling surface and resampling programs", 0.84);
        this.Marks = new TyreMarkDepot(this.Exchange);
        // 📝 The vehicle record: rigid wheel damage flags, the steering wheel's turn and the glazing (VehicleVisual.wgsl).
        this.VehicleVisual = new VehicleVisualState();
        this.VehicleRecords = this.Exchange.ConstructRecords(VehicleRecordBytes, GPUBufferUsage.STORAGE | GPUBufferUsage.COPY_DST, "Vehicle record");
        this.Exchange.Device.queue.writeBuffer(this.VehicleRecords, 0, this.VehicleVisual.EncodeRecord(this.QueryCabinHeader(), this.Vehicle.Wheels, this.Vehicle.Orientation));
        this.Effects = new EffectSequence(this.Exchange, this.Shaders, this.Scene);
        this.Effects.ConstructVehicleCells(this.Vehicle);
        await this.Effects.ConstructPrograms(this.FrameRecords);
        this.Effects.ApplyPalette(this.MaterialRecords);
        await this.Shadow.ConstructMaskedProgram(this.OccupantRecords, this.Effects, this.VehicleRecords);
        this.Recovery = new RecoveryDirector(this);
        this.Surface = new SurfaceProjection(this.Exchange, this.Shaders);
        await this.Surface.ConstructPrograms(this.FrameRecords, this.OccupantRecords, this.MaterialRecords, this.Distance.TraversalLayout, this.Marks,
                                             this.Effects, this.VehicleRecords);
        this.NearestDepth = new NearestDepthSpace(this.Exchange, this.Shaders);
        await this.NearestDepth.ConstructPrograms();
        this.Resampling = new ResamplingSequence(this.Exchange, this.Shaders);
        await this.Resampling.ConstructPrograms(this.FrameRecords, this.MaterialRecords, this.Distance.TraversalLayout, this.Cards.SamplingLayout,
                                                this.NearestDepth.TraversalLayout);
        this.Diffuse = new VarianceIntegrator(this.Exchange, this.Shaders, "Diffuse", { MinimumWeight: 0.08, TallyLimit: 24, WaveletIterations: 3, ColourSigma: 4.0 });
        await this.Diffuse.ConstructPrograms(this.FrameRecords);
        this.Glossy = new VarianceIntegrator(this.Exchange, this.Shaders, "Glossy", { MinimumWeight: 0.12, TallyLimit: 12, WaveletIterations: 2, ColourSigma: 3.0 });
        await this.Glossy.ConstructPrograms(this.FrameRecords);
        this.Display = new DisplayProjection(this.Exchange, this.Shaders);
        await this.Display.ConstructPrograms(this.FrameRecords, this.MaterialRecords, this.Distance.TraversalLayout, this.Cards.SamplingLayout,
                                             this.OccupantRecords);
        this.ReportProgress("Compiling the cabin, mirrors and sparks", 0.89);
        this.Sparks = new SparkSequence(this.Exchange, this.Shaders);
        await this.Sparks.ConstructPrograms(this.FrameRecords);
        this.Cabin = new CabinSequence(this.Exchange, this.Shaders);
        this.VehicleCardsDirty = false;
        this.Cabin.Appearance = new VehicleAppearance(this.Materials, (Offset, Data) =>
        {
            this.Exchange.Device.queue.writeBuffer(this.MaterialRecords, Offset, Data);
            this.MarkVehicleAppearanceDirty();
        });
        if (this.Lobby)
        {
            // 📝 Configurator hero: deep graphite clearcoat on dark wheels with emissive cool-white running rim accents.
            //    Applied BEFORE CaptureCards() so Frame 0 SDF Surface Cards, reflections & GI match the hero finish!
            this.Cabin.Appearance.Apply({ Colour: "#33363a", Matte: false, DarkWheels: true, BlueRims: false });
            const Hub = this.Materials[CourseMaterial.Hub];
            Hub.Albedo   = [0.04, 0.045, 0.05];
            Hub.Emission = [2.4, 4.2, 5.0];
            Device.queue.writeBuffer(this.MaterialRecords, CourseMaterial.Hub * 48, EncodeMaterialRecords([Hub]));
            this.VehicleCardsDirty = false;
        }
        this.Cabin.AssignPressure(this.Settings.TyrePressureFront);
        await this.Cabin.ConstructResources({ FrameRecords: this.FrameRecords, OccupantRecords: this.OccupantRecords, MaterialRecords: this.MaterialRecords,
                                              EffectRecords: this.Effects.Records, VehicleRecords: this.VehicleRecords, Shadow: this.Shadow,
                                              Atmosphere: this.Atmosphere, Scene: this.Scene, AdaptationRecords: this.Display.AdaptationRecords });
        this.ReportProgress("Compiling drift smoke programs", 0.91);
        this.Thermal = new TyreThermalSolver(4);
        this.BurstPuffs = [];
        this.HeadYaw = 0.0;
        this.HeadPitch = 0.0;
        this.Smoke = new SmokeSequence(this.Exchange, this.Shaders, this.GradeName);
        await this.Smoke.ConstructPrograms(this.FrameRecords, this.Distance.TraversalLayout, this.Atmosphere, this.OccupantRecords, this.Shadow);
        this.Metrics = new HardwareMetrics(this.Exchange, PassNames);

        this.ReportProgress("Capturing surface cards", 0.94);
        this.Cards.CaptureCards(this.Scene, this.VertexRecords);
        this.VehicleCardsDirty = false;
        this.Driver = new DriverInputIntegrator();
        this.Chase = new ChaseCameraSolver();
        this.Chase.Surfaces = this.Vehicle?.Surfaces || this.LevelDescriptor.Surfaces || [];
        this.Orbit = new VehicleOrbitCamera();
        if (this.Lobby)
        {
            // 📝 Opening 5.5 s venue-tailored architectural reveal into signature hero framing.
            const VenueCam = this.LobbyVenue?.EntryCamera || {
                From: { Yaw: 2.35, Pitch: 0.14, Distance: 16.5 },
                To:   { Yaw: 0.92, Pitch: 0.24, Distance: 10.4 }
            };
            this.LobbyEntry = {
                Seconds: 0.0,
                Duration: 5.5,
                From: { ...VenueCam.From },
                To:   { ...VenueCam.To }
            };
            Object.assign(this.Orbit, this.LobbyEntry.From);
        }
        this.Flight = new FreeCameraSolver();
        this.Training = new TrainingSequence(this);
        this.ConstructPanel();
        this.BindWorkspaceAndLobbyUi();
        this.ConstructInput();
        this.ReconstructScreenTargets();
        if (this.StressInitial > 0 || this.Settings.StressMotion > 0)
        {
            this.Stress.AssignMotion(this.Settings.StressMotion);
            this.Stress.Assign(this.StressInitial);
        }
        await Device.queue.onSubmittedWorkDone();
        this.ReportProgress("Ready", 1.0);
        document.getElementById("bring-up")?.classList.add("hidden");
        if (this.Lobby)
        {
            this.LobbyIdleSince = performance.now();
            this.RevealLobby();
        }
        this.Ready = true;
        this.LastTime = performance.now();
        window.IlluminationExhibit = this;
        requestAnimationFrame((Now) => this.AdvanceFrame(Now));
    }

    RevealLobby()
    {
        if (this.LobbyPresented)
            return;
        this.LobbyPresented = true;
        const Veil = document.getElementById("cinematic-fade");
        if (Veil?.style)
        {
            Veil.style.transition = "opacity 2.4s ease";
            Veil.style.opacity = "0";
        }
    }

    ResolveStreetLampPoles(Descriptor)
    {
        const Poles = [];
        const Placements = Descriptor?.Placements ? Descriptor.Placements() : [];
        for (let Index = 0; Index < Placements.length; ++Index)
        {
            const P = Placements[Index];
            if (P.Shape === "PoleHead")
                Poles.push({ X: P.Position[0], Y: P.Position[1], Z: P.Position[2] });
            else if (P.Shape === "WorkLightTower")
                Poles.push({ X: P.Position[0], Y: P.Position[1], Z: P.Position[2] + 4.45, Colour: [1.0, 0.66, 0.16], Intensity: 82.0, Reach: 38.0 });
            else if (P.Shape === "AmberBeaconPost")
                Poles.push({ X: P.Position[0], Y: P.Position[1], Z: P.Position[2] + 1.25, Colour: [1.0, 0.52, 0.05], Intensity: 34.0, Reach: 18.0, PulseFreq: 5.2, Phase: Index * 0.7 });
            else if (P.Shape === "StationCanopyLamp")
                Poles.push({ X: P.Position[0], Y: P.Position[1], Z: P.Position[2] - 0.12, Colour: P.Name.includes("EV") ? [0.76, 0.94, 1.0] : [1.0, 0.92, 0.80], Intensity: 68.0, Reach: 28.0 });
            else if (P.Shape === "FactoryHighBayLamp")
                Poles.push({ X: P.Position[0], Y: P.Position[1], Z: P.Position[2] - 0.16, Colour: [0.95, 0.96, 1.0], Intensity: 74.0, Reach: 34.0 });
        }
        return Poles;
    }

    // 📝 The Frontier Editor around the viewport; in Lobby mode, hide the editor columns so the studio fills the viewport.
    ConstructPanel()
    {
        this.StubShell ||= { Present() {}, Announce() {}, Controls: { Synchronise() {} } };
        this.StubDock  ||= { Synchronise() {}, Toggle() {}, PlaceInset() {}, InsetDrawn: false };
        if (this.Lobby)
        {
            this.Shell = this.StubShell;
            this.Dock  = this.StubDock;
            return;
        }
        if (!this.EditorShellInstance)
        {
            this.AdapterDescription = this.Exchange.QueryAdapterDescription();
            this.EditorShellInstance = new EditorShell(this);
            this.EditorDockInstance = new OverlayDock(this.Settings, (Name) =>
            {
                this.EditorShellInstance.Controls.Synchronise();
                if (Name === "Panel" || Name === "Clean")
                    this.ReconstructScreenTargets();
            });
        }
        this.Shell = this.EditorShellInstance;
        this.Dock  = this.EditorDockInstance;
        if (this.Fracture)
            this.Fracture.OnBreak = (Prop) =>
            {
                this.Shell?.Announce(`${Prop.Definition.Name} — ${Prop.Pieces.length} fragments`);
                this.DemolishRun?.OnWallBroken?.(Prop);
                this.PistonLabyrinth?.OnWallBroken?.(Prop);
            };
        if (this.PendingPreset)
        {
            this.ApplyPreset(this.PendingPreset);
            this.PendingPreset = null;
        }
        this.Shell.Controls.Synchronise();
        this.Dock.Synchronise();
    }

    // 📝 Shared header workspace tabs (Lobby ↔ Drive) + Level picker + minimal floating Lobby Dock (5 Venues, Stage, Cameras, Launch).
    BindWorkspaceAndLobbyUi()
    {
        if (typeof document === "undefined")
            return;
        document.body?.classList.toggle("workspace-lobby", this.Lobby);
        const BringUpHeading = document.querySelector("#bring-up h2");
        const VenueTitle = this.Lobby ? (this.LobbyVenue?.FullTitle || "Studio Lobby") : this.LevelDescriptor.Title;
        if (BringUpHeading)
            BringUpHeading.textContent = `Frontier · ${VenueTitle}`;
        document.title = `Frontier · ${VenueTitle}`;

        const Bar = document.getElementById("workspace-bar");
        if (Bar)
        {
            const LobbyTab = Bar.querySelector("[data-workspace-tab='lobby']");
            const DriveTab = Bar.querySelector("[data-workspace-tab='proving']");
            if (LobbyTab)
            {
                LobbyTab.classList.toggle("active", this.Lobby);
                LobbyTab.setAttribute("aria-selected", this.Lobby ? "true" : "false");
            }
            if (DriveTab)
            {
                DriveTab.classList.toggle("active", !this.Lobby);
                DriveTab.setAttribute("aria-selected", !this.Lobby ? "true" : "false");
            }
            const LevelSelect = document.getElementById("workspace-level-select");
            if (LevelSelect && LevelSelect.options.length > 0)
                LevelSelect.value = this.SelectedRouteLevel;
        }

        const LobbyHud = document.getElementById("lobby-hud");
        if (LobbyHud)
            LobbyHud.classList.toggle("hidden", !this.Lobby);
        const CinemaBar = document.getElementById("lobby-cinema-bar");
        if (CinemaBar && !this.Lobby)
            CinemaBar.classList.add("hidden");

        if (this.WorkspaceUiBound)
        {
            this.SyncLobbyHudUi();
            return;
        }
        this.WorkspaceUiBound = true;

        if (Bar)
        {
            Bar.querySelector("[data-workspace-tab='lobby']")?.addEventListener("click", () =>
            {
                if (!this.Lobby) this.BeginRoute("lobby");
            });
            Bar.querySelector("[data-workspace-tab='proving']")?.addEventListener("click", () =>
            {
                if (this.Lobby) this.BeginRoute(this.SelectedRouteLevel || "inversion");
            });

            const LevelSelect = document.getElementById("workspace-level-select");
            if (LevelSelect)
            {
                LevelSelect.innerHTML = "";
                for (const Entry of LevelCatalog)
                {
                    const Opt = document.createElement("option");
                    Opt.value = Entry.Key;
                    Opt.textContent = Entry.Title;
                    LevelSelect.appendChild(Opt);
                }
                LevelSelect.value = this.SelectedRouteLevel;
                LevelSelect.addEventListener("change", () =>
                {
                    this.SelectedRouteLevel = LevelSelect.value;
                    try { sessionStorage.setItem("frontier.selectedLevel", this.SelectedRouteLevel); } catch {}
                    this.SyncLobbyHudUi();
                    if (!this.Lobby)
                        this.BeginRoute(this.SelectedRouteLevel);
                });
            }
        }

        if (!LobbyHud)
            return;

        const VenueStrip = document.getElementById("lobby-venue-pills");
        if (VenueStrip)
        {
            VenueStrip.innerHTML = "";
            for (const Venue of LobbyVenues)
            {
                const Pill = document.createElement("button");
                Pill.type = "button";
                Pill.className = "lobby-venue-pill";
                Pill.dataset.venueKey = Venue.Key;
                Pill.title = `${Venue.FullTitle} — ${Venue.Subtitle}`;
                Pill.textContent = Venue.Title;
                Pill.addEventListener("click", () =>
                {
                    this.LobbyAudio?.Unlock();
                    this.BeginLobbyVenue(Venue.Key);
                });
                VenueStrip.appendChild(Pill);
            }
        }

        const LevelStrip = document.getElementById("lobby-level-pills");
        if (LevelStrip)
        {
            LevelStrip.innerHTML = "";
            for (const Entry of LevelCatalog)
            {
                const Pill = document.createElement("button");
                Pill.type = "button";
                Pill.className = "lobby-level-pill";
                Pill.dataset.levelKey = Entry.Key;
                Pill.title = Entry.Subtitle;
                Pill.textContent = Entry.Title.replace(" · Race to The Wall", "").replace(" · Sugar Siege", "");
                Pill.addEventListener("click", () =>
                {
                    this.LobbyAudio?.Unlock();
                    this.SelectedRouteLevel = Entry.Key;
                    try { sessionStorage.setItem("frontier.selectedLevel", Entry.Key); } catch {}
                    const HeaderSelect = document.getElementById("workspace-level-select");
                    if (HeaderSelect) HeaderSelect.value = Entry.Key;
                    this.SyncLobbyHudUi();
                });
                LevelStrip.appendChild(Pill);
            }
        }

        LobbyHud.querySelectorAll("[data-lobby-cam]").forEach((Btn) =>
        {
            Btn.addEventListener("click", () =>
            {
                this.LobbyAudio?.Unlock();
                const Target = Btn.getAttribute("data-lobby-cam");
                if (Target === "Cinematic")
                    this.ToggleLobbyCinematic();
                else
                {
                    this.NoteLobbyInteraction();
                    this.AssignCameraMode(Target);
                }
                this.SyncLobbyHudUi();
            });
        });

        document.getElementById("lobby-stage-btn")?.addEventListener("click", () =>
        {
            this.LobbyAudio?.Unlock();
            this.LobbyRoster?.ToggleStageLift();
            this.SyncLobbyHudUi();
        });
        document.getElementById("lobby-players-pill")?.addEventListener("click", () =>
        {
            this.LobbyAudio?.Unlock();
            this.NoteLobbyInteraction();
            this.AssignCameraMode("HMI");
            if (this.Cabin?.Screen)
                this.Cabin.Screen.PresentApplication("drive-room");
            this.SyncLobbyHudUi();
        });
        document.getElementById("lobby-audio-btn")?.addEventListener("click", () =>
        {
            this.LobbyAudio?.ToggleMute();
            this.SyncLobbyHudUi();
        });
        document.getElementById("lobby-cinema-next")?.addEventListener("click", () =>
        {
            this.LobbyAudio?.Unlock();
            this.SkipLobbyCinematicShot();
        });
        document.getElementById("lobby-launch-btn")?.addEventListener("click", () =>
        {
            this.LobbyAudio?.Unlock();
            this.BeginRoute(this.SelectedRouteLevel || "inversion");
        });

        this.SyncLobbyHudUi();
    }

    SyncLobbyHudUi()
    {
        if (!this.Lobby || typeof document === "undefined")
            return;
        const Hud = document.getElementById("lobby-hud");
        if (!Hud)
            return;

        Hud.classList.toggle("hmi-focus", this.Settings.CameraMode === "HMI");

        const Summary = this.LobbyRoster?.QueryRosterSummary();
        if (Summary)
        {
            const CountEl = document.getElementById("lobby-roster-count");
            if (CountEl) CountEl.textContent = `${Summary.ActiveCount} / ${Summary.MaxCount} Online`;
            const StageBtn = document.getElementById("lobby-stage-btn");
            if (StageBtn)
            {
                StageBtn.textContent = Summary.StageRaised ? "Stage ↓" : "Stage ↑";
                StageBtn.classList.toggle("active", Summary.StageRaised);
            }
        }

        const CinemaBar = document.getElementById("lobby-cinema-bar");
        const CinemaTitle = document.getElementById("lobby-cinema-title");
        if (CinemaBar)
        {
            const InCinema = Boolean(this.LobbyCinematic && this.Settings.CameraMode === "Orbit");
            CinemaBar.classList.toggle("hidden", !InCinema);
            if (InCinema && CinemaTitle)
            {
                const ActiveShot = LobbyCinematicShots[this.LobbyCinematic.Shot % LobbyCinematicShots.length];
                CinemaTitle.textContent = ActiveShot ? ActiveShot.Title : "Cinematic Reel";
            }
        }

        Hud.querySelectorAll("[data-lobby-cam]").forEach((Btn) =>
        {
            const Key = Btn.getAttribute("data-lobby-cam");
            const Active = Key === "Cinematic"
                ? Boolean(this.LobbyCinematic)
                : (!this.LobbyCinematic && this.Settings.CameraMode === Key);
            Btn.classList.toggle("active", Active);
        });

        Hud.querySelectorAll(".lobby-venue-pill").forEach((Pill) =>
        {
            Pill.classList.toggle("active", Pill.dataset.venueKey === this.LobbyVenueKey);
        });

        const AudioBtn = document.getElementById("lobby-audio-btn");
        if (AudioBtn && this.LobbyAudio)
        {
            const Muted = this.LobbyAudio.Muted;
            AudioBtn.textContent = Muted ? "Muted" : "Sound";
            AudioBtn.classList.toggle("muted", Muted);
            AudioBtn.setAttribute("aria-pressed", Muted ? "false" : "true");
        }

        Hud.querySelectorAll(".lobby-level-pill").forEach((Pill) =>
        {
            Pill.classList.toggle("active", Pill.dataset.levelKey === this.SelectedRouteLevel);
        });

        const LaunchBtn = document.getElementById("lobby-launch-btn");
        if (LaunchBtn)
        {
            const Chosen = LevelCatalog.find((E) => E.Key === this.SelectedRouteLevel);
            const ShortName = Chosen ? Chosen.Title.replace(" · Race to The Wall", "").replace(" · Sugar Siege", "") : "Level";
            LaunchBtn.textContent = `Drive ${ShortName} →`;
        }
    }

    // 📝 Every editor edit lands here once its value is written: most settings are read each frame and need nothing;
    //    these rebuild something or act.
    ApplySetting(Key)
    {
        if (Key === "HudVisible" || Key === "InsetEnabled")
            this.Dock.Synchronise();
        else if (Key === "TraceScale")
            this.ReconstructScreenTargets();
        else if (Key === "RenderScale")
        {
            this.Grade.RenderScale = this.Settings.RenderScale;
            this.ReconstructScreenTargets(true);
        }
        else if (Key === "FreezeDistance")
            this.Distance.Frozen = this.Settings.FreezeDistance;
        else if (Key === "TriangleReflection")
            this.PriorValid = false;
        else if (Key === "ScreenTraversal")
            this.PriorValid = false;
        else if (Key === "GlassIor")
        {
            // 📝 The garden glass keeps one index in the shader and one F0 in the record; the slider re-authors the
            //    record from the IOR and re-uploads that one family — see VehicleAppearance for the single-record write.
            const Ior = Math.max(1.0, Math.min(2.5, this.Settings.GlassIor));
            if (this.Materials)
            {
                this.Materials[CourseMaterial.ShowcaseGlass].SpecularLevel = IorToF0(Ior);
                if (this.Exchange && this.MaterialRecords)
                    this.Exchange.Device.queue.writeBuffer(this.MaterialRecords, CourseMaterial.ShowcaseGlass * 48,
                                                           EncodeMaterialRecords([this.Materials[CourseMaterial.ShowcaseGlass]]));
                this.PriorValid = false;
            }
        }
        else if (Key.startsWith("StressAdd"))
            this.Stress.Add(Number(Key.slice(9)));
        else if (Key === "StressClear")
            this.Stress.Assign(0);
        else if (Key === "StressFill")
            this.Stress.Assign(this.Stress.QueryCapacity());
        else if (Key === "StressMotion")
            this.Stress.AssignMotion(this.Settings.StressMotion);
        else if (Key === "SmokeResolution")
            this.Smoke.ConstructScreenTargets(this.ScreenExtent, this.Surface, this.Display, this.Settings.SmokeResolution, this.TraceExtent);
        else if (Key === "CoolTyres")
            this.RestoreTyres();
        else if (Key === "TyrePressure")
            this.InflateTyres(this.Settings.TyrePressureFront, this.Settings.TyrePressureRear);
        else if (Key === "SeeThroughGlass" && this.Cabin)
            this.Cabin.Settings.SeeThroughGlass = !!this.Settings.SeeThroughGlass;
        else if (Key === "ClearSmoke")
            this.Smoke.Clear();
        else if (Key === "Level")
        {
            this.BeginRoute(this.Settings.Level);
        }
        else if (Key === "AeroGripScale")
        {
            if (this.Vehicle)
                this.Vehicle.AeroGripScale = this.Settings.AeroGripScale;
        }
        else if (Key === "Quality")
        {
            const Query = new URLSearchParams(location.search);
            Query.set("quality", ["low", "medium", "high"][this.Settings.Quality]);
            location.search = Query.toString();
        }
        else if (["SunElevation", "SunAzimuth", "SolarIlluminance", "EmissionScale"].includes(Key))
            this.Settings.LightingPreset = "Custom";
    }

    ApplyPreset(Name)
    {
        if (!LightingPresets[Name])
            return;
        Object.assign(this.Settings, LightingPresets[Name]);
        this.Settings.LightingPreset = Name;
        if (this.Shell)
            this.Shell.Controls.Synchronise();
    }

    // 📝 Places the car at a zone of the grounds, at rest, wheels on the ground: derezzed here and rezzed there, or at
    //    once (Instant — scripts and the harness).
    TeleportVehicle(Zone, Instant = false, Done = null)
    {
        if (!Done && this.Training?.Active && this.Training.Active.Phase !== "armed")
            this.Training.Cancel();
        if (Zone?.Id === "trackI" && this.RoadEditor && this.RoadEditor.Project?.name !== "Spline & Highway Bridge Demo")
        {
            this.RoadEditor.DriveOnSelectedRoad();
            if (Done)
                Done();
            return;
        }
        const Destination = { Position: Zone.Position.slice(0, 2), Yaw: Zone.Yaw * Math.PI / 180.0 };
        if (this.FreeCamera)
            this.AssignCamera(false);
        if (!Instant && this.Recovery && !this.Recovery.Busy)
        {
            this.Recovery.Begin(RecoveryReasons.Teleport, Destination, () => { this.Shell.Announce(`${Zone.Name}`); if (Done) Done(); });
            return;
        }
        const DestBaseZ = ResolveCourseHeight(Destination.Position[0], Destination.Position[1]);
        this.Vehicle.Respawn([Destination.Position[0], Destination.Position[1], DestBaseZ + 0.63], Destination.Yaw);
        this.AfterVehicleMoved();
        this.Shell.Announce(`${Zone.Name}`);
        if (Done)
            Done();
    }

    // 📝 After the car is put somewhere new: inputs, camera and temporal history start over.
    AfterVehicleMoved()
    {
        this.Driver.Reset();
        this.Chase.Seeded = false;
        this.PriorValid = false;
        this.PositionVehicle();
        this.Scene.RetainOccupantTransform(this.Scene.VehicleOccupants.Body);
        for (const Wheel of this.Scene.VehicleOccupants.Wheels)
            this.Scene.RetainOccupantTransform(Wheel);
        if (this.Collapse)
            this.ResetCollapseDecks();
        if (this.ServiceStations)
            this.ResetServiceStations(false);
    }

    ResetServiceStations(ResetLevels = false)
    {
        if (!this.ServiceStations)
            return;
        this.ServiceStations.Reset(ResetLevels);
        if (this.Scene.ServiceBlock && this.Scene.ServiceBlock.Count > 0 && this.OccupantRecords && this.Volumes)
        {
            const Block = this.Scene.ServiceBlock;
            const Scalars = this.Scene.EncodeOccupantRange(this.Volumes, Block.First, Block.Count);
            this.Exchange.Device.queue.writeBuffer(this.OccupantRecords, Block.First * OccupantStride, Scalars);
        }
    }

    UploadTrafficLightOccupants()
    {
        if (!this.TrafficLights || !this.Scene.TrafficBlock || !this.OccupantRecords || !this.Volumes)
            return;
        const Device = this.Exchange.Device;
        const Block = this.Scene.TrafficBlock;
        if (Block.TotalCount > 0)
        {
            const HousingScalars = this.Scene.EncodeOccupantRange(this.Volumes, Block.HousingFirst, Block.TotalCount);
            Device.queue.writeBuffer(this.OccupantRecords, Block.HousingFirst * OccupantStride, HousingScalars);
            const LensCount = Block.TotalCount * 4;
            const LensScalars = this.Scene.EncodeOccupantRange(this.Volumes, Block.LensFirst, LensCount);
            Device.queue.writeBuffer(this.OccupantRecords, Block.LensFirst * OccupantStride, LensScalars);
        }
    }

    UploadDDayOccupants()
    {
        if (!this.DDayCombat?.Enabled || !this.Scene.DDayBlock || !this.OccupantRecords || !this.Volumes)
            return;
        const Block = this.Scene.DDayBlock;
        if (Block.Count > 0)
        {
            const Scalars = this.Scene.EncodeOccupantRange(this.Volumes, Block.First, Block.Count);
            this.Exchange.Device.queue.writeBuffer(this.OccupantRecords, Block.First * OccupantStride, Scalars);
        }
    }

    UploadCandyOccupants()
    {
        if (!this.CandyVillage?.Enabled || !this.Scene.CandyBlock || !this.OccupantRecords || !this.Volumes)
            return;
        const Block      = this.Scene.CandyBlock;
        const ExtraMeshes = (this.Scene.CandyWormBlock ? 1 : 0) + (this.Scene.CandyWaterBlock ? 1 : 0);
        const TotalCount = Block.Count + ExtraMeshes;
        if (TotalCount > 0)
        {
            const Scalars = this.Scene.EncodeOccupantRange(this.Volumes, Block.First, TotalCount);
            this.Exchange.Device.queue.writeBuffer(this.OccupantRecords, Block.First * OccupantStride, Scalars);
        }
    }

    UploadFoundryOccupants()
    {
        if (!this.FoundryConveyor?.Enabled || !this.Scene.FoundryBlock || !this.OccupantRecords || !this.Volumes)
            return;
        const Block = this.Scene.FoundryBlock;
        if (Block.Count > 0)
        {
            const Scalars = this.Scene.EncodeOccupantRange(this.Volumes, Block.First, Block.Count);
            this.Exchange.Device.queue.writeBuffer(this.OccupantRecords, Block.First * OccupantStride, Scalars);
        }
    }

    UploadDemolishOccupants()
    {
        if (!this.DemolishRun?.Enabled || !this.Scene.DemolishBlock || !this.OccupantRecords || !this.Volumes)
            return;
        const Block = this.Scene.DemolishBlock;
        if (Block.Count > 0)
        {
            const Scalars = this.Scene.EncodeOccupantRange(this.Volumes, Block.First, Block.Count);
            this.Exchange.Device.queue.writeBuffer(this.OccupantRecords, Block.First * OccupantStride, Scalars);
        }
    }

    // 📝 Generates a brand-new random procedurally fractured Demolition City seed and rebuilds in place (zero reload!).
    RegenerateDemolishCity(Seed = null)
    {
        return this.SwitchSceneInPlace("demolish", this.LobbyVenueKey || "studio", { Seed });
    }

    UploadLabyrinthOccupants()
    {
        if (!this.PistonLabyrinth?.Enabled || !this.Scene.LabyrinthBlock || !this.OccupantRecords || !this.Volumes)
            return;
        const Block = this.Scene.LabyrinthBlock;
        if (Block.Count > 0)
        {
            const Scalars = this.Scene.EncodeOccupantRange(this.Volumes, Block.First, Block.Count);
            this.Exchange.Device.queue.writeBuffer(this.OccupantRecords, Block.First * OccupantStride, Scalars);
        }
    }

    // 📝 Generates a brand-new random procedural Piston Labyrinth seed and rebuilds in place (zero reload!).
    RegenerateLabyrinthMaze(Seed = null)
    {
        return this.SwitchSceneInPlace("labyrinth", this.LobbyVenueKey || "studio", { Seed });
    }

    UploadLobbyOccupants()
    {
        if (!this.LobbyRoster?.Enabled || !this.Scene.LobbyBlock || !this.OccupantRecords || !this.Volumes)
            return;
        const Block = this.Scene.LobbyBlock;
        if (Block.Count > 0)
        {
            const Scalars = this.Scene.EncodeOccupantRange(this.Volumes, Block.First, Block.Count);
            this.Exchange.Device.queue.writeBuffer(this.OccupantRecords, Block.First * OccupantStride, Scalars);
        }
    }

    UploadCollapseOccupants()
    {
        if (!this.Collapse || !this.Scene.CollapseBlock || !this.OccupantRecords || !this.Volumes)
            return;
        const Device = this.Exchange.Device;
        const Block = this.Scene.CollapseBlock;
        if (Block.ProxyCount > 0)
        {
            const ProxyScalars = this.Scene.EncodeOccupantRange(this.Volumes, Block.ProxyFirst, Block.ProxyCount);
            Device.queue.writeBuffer(this.OccupantRecords, Block.ProxyFirst * OccupantStride, ProxyScalars);
        }
        if (Block.Count > 0)
        {
            const PieceScalars = this.Scene.EncodeOccupantRange(this.Volumes, Block.First, Block.Count);
            Device.queue.writeBuffer(this.OccupantRecords, Block.First * OccupantStride, PieceScalars);
        }
    }

    ResetCollapseDecks()
    {
        if (!this.Collapse)
            return;
        this.Collapse.ResetAllPoses();
        this.UploadCollapseOccupants();
    }

    RebakeCollapseDecks(NewSeed = null)
    {
        if (!this.Collapse)
            return 0;
        const Seed = (NewSeed !== null && NewSeed !== undefined)
            ? (NewSeed >>> 0)
            : (((Math.random() * 0xFFFFFFFF) >>> 0) || 0xC011A95E);
        this.Collapse.BakeAllSpans(Seed, this.Exchange.Device, this.VertexRecords);
        this.UploadCollapseOccupants();
        if (this.Shell)
            this.Shell.Announce(`Collapse cracks rebaked · seed 0x${this.Collapse.SessionSeed.toString(16).toUpperCase()}`);
        return this.Collapse.SessionSeed;
    }

    // 📝 While derezzed the car leaves the raster, the distance field and the card lists — and the debris ignores it.
    AssignVehicleHidden(Hidden)
    {
        const Scene = this.Scene;
        const Indices = [Scene.VehicleOccupants.Body, ...Scene.VehicleOccupants.Wheels];
        const Bounds = Indices.map((Index) => Scene.QueryWorldBounds(Index));
        for (const Index of Indices)
            Scene.Occupants[Index].Inactive = Hidden;
        this.Vehicle.Hidden = Hidden;
        Scene.ConstructDrawRuns();
        this.Cards.RebuildTileLists(Scene);
        this.Distance.InvalidateBounds(Bounds);
    }

    //--------------------------------------------------------------------------------------------------------------------
    //                                                   TYRES
    //--------------------------------------------------------------------------------------------------------------------

    // 📝 The VehicleRecord's cabin header: the steering wheel (the front wheels' angle × the steering ratio, turned about
    //    the column), see-through glazing, fixed cabin frame and independent tablet scale.
    QueryCabinHeader()
    {
        const Steering = CabinLayout.Steering;
        const Angle = this.Vehicle ? this.Vehicle.SteerAngle * Steering.Ratio : 0.0;
        const Cockpit = ResolveCockpitFrame(this.Settings);
        const Tablet = { Centre: CabinLayout.Tablet.Centre, Scale: ResolveTabletScale(this.Settings.HmiScale) };
        if (this.Cabin) {
            this.Cabin.Cockpit = Cockpit;
            this.Cabin.TabletScale = Tablet.Scale;
        }
        return { SteeringAngle: Angle, SeeThroughGlass: this.Cabin ? this.Cabin.Settings.SeeThroughGlass : true,
                 Column: Steering.Pivot, Axis: Steering.Axis, Cockpit, Tablet };
    }

    // 📝 Fresh rubber and air on every corner: the thermal model at its cold pressures, no damage.
    RestoreTyres()
    {
        this.Thermal.ColdPressures = [this.Settings.TyrePressureFront, this.Settings.TyrePressureFront, this.Settings.TyrePressureRear,
                                      this.Settings.TyrePressureRear].map((Kilopascals) => Kilopascals * 1000.0);
        this.Thermal.Reset();
        this.Vehicle.RestoreTyres();
        this.Vehicle.AssignTyrePressures(this.Thermal.Wheels.map((Wheel) => Wheel.GaugePressure));
    }

    // 📝 The tyre bay: cold pressures per axle [kPa]; every intact tyre is pumped to them now.
    InflateTyres(Front, Rear)
    {
        this.Settings.TyrePressureFront = Front;
        this.Settings.TyrePressureRear = Rear;
        this.Thermal.Inflate([Front, Front, Rear, Rear].map((Kilopascals) => Kilopascals * 1000.0));
        if (this.Cabin)
            this.Cabin.AssignPressure(Front);
        if (this.Shell)
            this.Shell.Controls.Synchronise();
    }

    // 📝 A nail or a blowout in one tyre (the inspector's buttons).
    PunctureTyre(Wheel, Blowout)
    {
        if (this.Thermal.Puncture(Wheel, Blowout))
            this.Shell.Announce(`${["FL", "FR", "RL", "RR"][Wheel]} ${Blowout ? "blowout" : "puncture — slow leak"}`);
    }

    // 📝 A tyre letting go: the announcement, rubber thrown out of the wheel with the car's velocity, a puff of dust
    //    and rubber smoke from the wheel for a quarter of a second.
    OnTyreBurst(Burst)
    {
        const Names = ["front left", "front right", "rear left", "rear right"];
        this.Shell.Announce(`Tyre burst — ${Names[Burst.Wheel]} (${Burst.Cause})`);
        const Transform = this.Vehicle.QueryWheelTransform(Burst.Wheel);
        this.Effects.BurstTyre(Transform, [0.46, 0.17, 0.46], this.Vehicle.LinearVelocity);
        const Hub = Transform.slice(12, 15);
        this.BurstPuffs.push({ Position: [Hub[0], Hub[1], Math.max(0.05, Hub[2] - 0.35)], Seconds: 0.15 });
        this.BurstCount = (this.BurstCount || 0) + 1;
    }

    // 📝 A flat rolled to pieces: the rest of the rubber goes, and the wheel runs on its rim.
    OnTyreShred(Wheel)
    {
        const Names = ["front left", "front right", "rear left", "rear right"];
        this.Shell.Announce(`${Names[Wheel]} tyre shredded — on the rim`);
        this.Effects.BurstTyre(this.Vehicle.QueryWheelTransform(Wheel), [0.44, 0.15, 0.44], this.Vehicle.LinearVelocity);
    }

    // 📝 The burst puffs as smoke sources: a short dense pulse at the wheel.
    ResolveBurstPuffs(Seconds)
    {
        const Emissions = [];
        for (const Puff of this.BurstPuffs)
        {
            Puff.Seconds -= Seconds;
            Emissions.push({ Active: true, Burning: false, Strength: 1.2, Position: Puff.Position, Hub: Puff.Position, Radius: 0.4,
                             Expansion: 4.0, SmokeRate: 40.0, SootRate: 0.0, FlameRate: 0.0, HeatRate: 0.5, Velocity: [0.0, 0.0, 0.8],
                             Plume: null, CombustionPower: 0.0, Tread: [0.0, 0.0, 0.0] });
        }
        this.BurstPuffs = this.BurstPuffs.filter((Puff) => Puff.Seconds > 0.0);
        return Emissions;
    }

    // 📝 What the HMI shows live: speed, revs, gear, tyres, active venue/circuit, and Lobby player presence.
    QueryTyreLive()
    {
        const Telemetry = this.Vehicle.QueryTelemetry();
        const Treads = this.Thermal.QueryTreadFractions();
        return {
            Speed: Math.abs(Telemetry.Speed) * 3.6, Revolutions: Telemetry.EngineRPM, Gear: Telemetry.Gear,
            Temperatures: this.Thermal.Wheels.map((Wheel) => Wheel.Surface - 273.15),
            Pressures: this.Thermal.Wheels.map((Wheel) => Wheel.GaugePressure / 1000.0),
            Tread: Treads,
            States: this.Thermal.Wheels.map((Wheel, Index) => this.Vehicle.Wheels[Index].Shredded ? "on the rim" : Wheel.Burst ? "burst"
                                                              : Wheel.LeakRate > 0.0 ? "leaking" : ""),
            LevelKey: this.LevelKey,
            LevelTitle: this.LevelDescriptor?.Title || "Proving Grounds",
            VenueKey: this.LobbyVenueKey,
            VenueTitle: this.LobbyVenue?.FullTitle || "White Studio",
            VenueShort: this.LobbyVenue?.Title || "Studio",
            SelectedRoute: this.SelectedRouteLevel,
            Roster: this.LobbyRoster ? this.LobbyRoster.QueryRosterSummary() : null
        };
    }

    QueryInteriorCamera()
    {
        const Rig = CameraRigs[this.Settings.CameraMode];
        return !this.FreeCamera && Rig && Rig.Interior;
    }

    // 📝 Mirrors are drawn while they can be seen: from inside, or with the chase camera close to the car.
    QueryMirrorsVisible()
    {
        if (this.Vehicle.Hidden)
            return false;
        if (this.QueryInteriorCamera())
            return true;
        const Eye = this.CameraEye || this.Vehicle.Position;
        return LengthVector(SubtractVector(Eye, this.Vehicle.Position)) < 14.0;
    }

    ConstructInput()
    {
        if (this.Canvas.style) this.Canvas.style.touchAction = "none";
        window.addEventListener("keydown", (Event) =>
        {
            this.ServiceStations?.Audio?.Unlock();
            if (Event.target instanceof HTMLInputElement ||
                Event.target instanceof HTMLSelectElement ||
                Event.target instanceof HTMLTextAreaElement)
                return;
            if (this.RoadEditor?.ActiveWorkspace === "roadstudio")
            {
                if (this.RoadEditor.HandleStudioKeyDown(Event))
                    return;
                this.Keys.add(Event.code);
                return;
            }
            if (["Space", "ArrowUp", "ArrowDown", "ArrowLeft", "ArrowRight", "F8"].includes(Event.code))
                Event.preventDefault();
            if (Event.code === "F8" || (Event.code === "KeyF" && !Event.repeat))
                this.ToggleCamera();
            if (this.Lobby && Event.code !== "KeyV" && Event.code !== "KeyN") this.NoteLobbyInteraction();
            if (this.Lobby && (Event.code === "Escape" || Event.code === "KeyQ") && !Event.repeat)
                this.AssignCameraMode("Orbit");
            if (this.Lobby && Event.code === "KeyV" && !Event.repeat && !Event.ctrlKey && !Event.metaKey)
                this.ToggleLobbyCinematic();
            if (this.Lobby && Event.code === "KeyN" && !Event.repeat && !Event.ctrlKey && !Event.metaKey)
                this.SkipLobbyCinematicShot();
            if (Event.code === "KeyM" && !Event.repeat && !Event.ctrlKey && !Event.metaKey)
                this.AssignCameraMode(this.Lobby
                    ? (this.Settings.CameraMode === "HMI" ? "Orbit" : "HMI")
                    : (this.Settings.CameraMode === "HMI" ? "Cockpit" : "HMI"));
            if (Event.code === "KeyR" && !Event.repeat)
                this.RespawnVehicle();
            if (Event.code === "KeyT" && !Event.repeat)
                this.ResetArena();
            if (Event.code === "KeyC" && !Event.repeat && !Event.ctrlKey && !Event.metaKey)
                this.CycleCamera(Event.shiftKey ? -1 : 1);
            if (Event.code === "KeyU" && !Event.repeat && !Event.ctrlKey && !Event.metaKey && !Event.altKey)
                this.DrivingHud?.CycleDisplayStyle();
            if (Event.code === "KeyI" && !Event.repeat && !Event.ctrlKey && !Event.metaKey && !Event.altKey)
            {
                if (this.Lighting)
                {
                    const Mode = this.Lighting.CycleVehicleMode();
                    const Names = ["Off", "Low Beam", "High Beam", "Fog"];
                    this.Shell?.Announce(`Vehicle Lights: ${Names[Mode]}`);
                    this.Cards?.InvalidateLighting();
                    this.PriorValid = false;
                }
            }
            if (Event.code === "KeyL" && !Event.repeat && !Event.ctrlKey && !Event.metaKey && !Event.altKey)
            {
                if (this.Lighting)
                {
                    const Active = this.Lighting.ToggleRoofLight();
                    const FixtureName = this.Lighting.RoofFixture === RoofFixtureType.SlimLED ? "Slim LED Bar" : "Rally Pods";
                    this.Shell?.Announce(`Auxiliary Roof Light: ${Active ? "ON" : "OFF"} (${FixtureName})`);
                    this.Cards?.InvalidateLighting();
                    this.PriorValid = false;
                }
            }
            if (Event.code.startsWith("Digit") && !Event.repeat && !Event.ctrlKey && !Event.metaKey && !Event.altKey)
            {
                const ZoneSlot = Number(Event.code.slice(5)) - 1;
                if (ZoneSlot >= 0 && this.LevelZones && ZoneSlot < this.LevelZones.length)
                    this.TeleportVehicle(this.LevelZones[ZoneSlot]);
            }
            for (const [Name, Code] of Object.entries(OverlayKeys))
                if (Event.code === Code && !Event.repeat && !Event.ctrlKey && !Event.metaKey && !Event.altKey)
                    this.Dock.Toggle(Name);
            this.Keys.add(Event.code);
        });
        window.addEventListener("keyup", (Event) => this.Keys.delete(Event.code));
        window.addEventListener("blur", () => this.Keys.clear());
        let Dragging = false, LastX = 0, LastY = 0, Touching = false;
        // 📝 From inside the car a pointer on the tablet is a finger on its glass: the ray from the eye through the
        //    pixel, met with the screen, in the panel's own coordinates (CabinSequence.QueryTabletHit).
        const TabletHit = (Event) =>
        {
            if (!this.Cabin || !this.CameraState || this.FreeCamera)
                return null;
            const Bounds = this.Canvas.getBoundingClientRect();
            const X = ((Event.clientX - Bounds.left) / Math.max(Bounds.width, 1.0)) * 2.0 - 1.0;
            const Y = 1.0 - ((Event.clientY - Bounds.top) / Math.max(Bounds.height, 1.0)) * 2.0;
            const C = this.CameraState;
            const Direction = NormalizeVector(AddVector(C.Forward, AddVector(ScaleVector(C.Right, X * C.TanY * C.Aspect), ScaleVector(C.Up, Y * C.TanY))));
            return this.Cabin.QueryTabletHit(C.Position, Direction, this.Vehicle.QueryBodyTransform());
        };
        this.Canvas.addEventListener("pointerdown", (Event) =>
        {
            this.ServiceStations?.Audio?.Unlock();
            Dragging = true;
            LastX = Event.clientX;
            LastY = Event.clientY;
            if (this.Lobby)
            {
                this.NoteLobbyInteraction();
                this.LobbyEntry = null;
                this.LobbyTap = { X: Event.clientX, Y: Event.clientY, At: performance.now(), Mode: this.Settings.CameraMode, OnTablet: false };
            }
            this.Canvas.setPointerCapture(Event.pointerId);
            if (document.activeElement instanceof HTMLElement)
                document.activeElement.blur();
            const Hit = TabletHit(Event);
            if (this.Lobby && Hit && this.Settings.CameraMode === "Cockpit")
                this.AssignCameraMode("HMI");
            Touching = !!Hit && this.Cabin.ForwardPointer("down", Hit.Across, Hit.Down);
            if (this.LobbyTap) this.LobbyTap.OnTablet = !!Hit;
        });
        this.Canvas.addEventListener("pointerup", (Event) =>
        {
            Dragging = false;
            if (Touching)
            {
                const Hit = TabletHit(Event);
                this.Cabin.ForwardPointer(Hit ? "up" : "cancel", Hit ? Hit.Across : 0.0, Hit ? Hit.Down : 0.0);
                Touching = false;
            }
            if (this.Lobby && this.LobbyTap)
            {
                const Tap = this.LobbyTap;
                this.LobbyTap = null;
                const CleanTap = Math.hypot(Event.clientX - Tap.X, Event.clientY - Tap.Y) < 6.0 && performance.now() - Tap.At < 500.0;
                if (CleanTap && Tap.Mode === "Orbit" && this.ViewProjectionInverse)
                {
                    const Bounds = this.Canvas.getBoundingClientRect();
                    const Pick = PickVehicle(this.Scene, this.ViewProjectionInverse,
                        (Event.clientX - Bounds.left) / Math.max(Bounds.width, 1.0), (Event.clientY - Bounds.top) / Math.max(Bounds.height, 1.0));
                    if (Pick) this.AssignCameraMode("HMI");
                }
                else if (CleanTap && Tap.Mode === "HMI" && !Tap.OnTablet && !TabletHit(Event))
                    this.AssignCameraMode("Orbit");
            }
        });
        this.Canvas.addEventListener("pointercancel", () => {
            Dragging = false;
            if (Touching) this.Cabin.ForwardPointer("cancel", 0, 0);
            Touching = false;
        });
        this.Canvas.addEventListener("pointermove", (Event) =>
        {
            if (!Dragging)
            {
                const Hit = TabletHit(Event);
                if (Hit)
                    this.Cabin.ForwardPointer("hover", Hit.Across, Hit.Down);
                return;
            }
            const DeltaX = Event.clientX - LastX, DeltaY = Event.clientY - LastY;
            LastX = Event.clientX;
            LastY = Event.clientY;
            if (this.Lobby) this.NoteLobbyInteraction();
            if (Touching)
            {
                const Hit = TabletHit(Event);
                if (Hit)
                    this.Cabin.ForwardPointer("move", Hit.Across, Hit.Down);
                return;
            }
            if (!this.FreeCamera && this.Settings.CameraMode === "Orbit") {
                this.Orbit.Rotate(DeltaX, DeltaY);
                if (this.Lobby) this.LobbyOrbitHoldUntil = performance.now() + (this.LobbyUserControl ? 60000.0 : 5000.0);
                return;
            }
            if (!this.FreeCamera && CameraRigs[this.Settings.CameraMode])
            {
                // 📝 Inside: look around (the head turns about ±110°, nods ±45°); outside rigs are fixed.
                this.HeadYaw = Math.max(-1.9, Math.min(1.9, this.HeadYaw - DeltaX * 0.004));
                this.HeadPitch = Math.max(-0.8, Math.min(0.8, this.HeadPitch + DeltaY * 0.004));
                return;
            }
            if (this.FreeCamera)
            {
                this.Flight.Yaw -= DeltaX * 0.004;
                this.Flight.Pitch = Math.max(-1.5, Math.min(1.5, this.Flight.Pitch - DeltaY * 0.004));
            }
            else
            {
                this.Chase.OrbitYaw -= DeltaX * 0.006;
                this.Chase.OrbitPitch = Math.max(-0.25, Math.min(1.2, this.Chase.OrbitPitch + DeltaY * 0.004));
            }
        });
        this.Canvas.addEventListener("wheel", (Event) =>
        {
            Event.preventDefault();
            if (this.Lobby) this.NoteLobbyInteraction();
            if (!this.FreeCamera && this.Settings.CameraMode === "Orbit") {
                this.LobbyEntry = null;
                this.Orbit.Zoom(Event.deltaY);
                if (this.Lobby) this.LobbyOrbitHoldUntil = performance.now() + (this.LobbyUserControl ? 60000.0 : 5000.0);
                return;
            }
            if (!this.FreeCamera && this.Settings.CameraMode === "HMI") {
                this.Settings.HmiZoom = Math.max(0.65, Math.min(2, this.Settings.HmiZoom * Math.exp(Event.deltaY * 0.001)));
                return;
            }
            if (!this.FreeCamera && ["Cockpit", "Passenger"].includes(this.Settings.CameraMode)) {
                this.Settings.CockpitFov = Math.max(30, Math.min(90, this.Settings.CockpitFov + Event.deltaY * 0.025));
                return;
            }
            this.Chase.OrbitScale = Math.max(0.5, Math.min(3.0, this.Chase.OrbitScale * Math.exp(Event.deltaY * 0.001)));
        }, { passive: false });
        new ResizeObserver(() => this.ReconstructScreenTargets()).observe(this.Canvas);
    }

    ToggleCamera()
    {
        this.AssignCamera(!this.FreeCamera);
    }

    // 📝 C cycles all rigs, including independent Orbit; Shift+C cycles backwards.
    CycleCamera(Step = 1)
    {
        if (this.Lobby)
        {
            const Sequence = ["Orbit", "Cockpit", "HMI"];
            const Index = Math.max(0, Sequence.indexOf(this.Settings.CameraMode));
            this.AssignCameraMode(Sequence[(Index + (Step < 0 ? -1 : 1) + Sequence.length) % Sequence.length]);
            return;
        }
        const Modes = Object.keys(CameraRigs);
        const Index = Math.max(0, Modes.indexOf(this.Settings.CameraMode));
        this.AssignCameraMode(Modes[(Index + Step + Modes.length) % Modes.length]);
    }

    AssignCameraMode(Mode)
    {
        if (!(Mode in CameraRigs))
            return;
        if (this.Lobby)
        {
            if (Mode !== "Orbit" && Mode !== "Cockpit" && Mode !== "HMI")
                return;
            this.LobbyCinematic = null;
            this.SetCinematicVeil(0);
            if (Mode !== this.Settings.CameraMode && this.CameraState && this.DisplayedTarget)
            {
                this.LobbyBlend = {
                    Seconds: 0.0,
                    Duration: Mode === "HMI" ? 2.1 : 1.7,
                    FromEye: (this.LobbyLagEye || this.CameraState.Position).slice(),
                    FromTarget: this.DisplayedTarget.slice(),
                    FromFov: this.DisplayedFov || 60.0
                };
                this.LobbyEntry = null;
            }
        }
        this.Settings.CameraMode = Mode;
        this.HeadYaw = 0.0;
        this.HeadPitch = 0.0;
        this.CameraMotion?.Reset();
        if (this.FreeCamera)
            this.FreeCamera = false;
        this.PriorValid = false;
        if (this.Shell)
        {
            this.Shell.Controls.Synchronise();
            this.Shell.Announce(`Camera: ${Mode}`);
        }
        if (this.Lobby)
            this.SyncLobbyHudUi();
    }

    // 📝 Zero-reload, zero-recompile live switch to any driving level (or back to the Showroom Lobby).
    BeginRoute(LevelKey)
    {
        const TargetKey = LevelKey === "lobby"
            ? "lobby"
            : (LevelCatalog.some((E) => E.Key === LevelKey) ? LevelKey : "inversion");
        if (TargetKey !== "lobby")
        {
            this.SelectedRouteLevel = TargetKey;
            try { sessionStorage.setItem("frontier.selectedLevel", TargetKey); } catch {}
        }
        return this.SwitchSceneInPlace(TargetKey, this.LobbyVenueKey || "studio");
    }

    // 📝 Zero-reload, zero-recompile live switch between all 5 Showroom Lobby venues
    //    (Studio, Monolith, Salt Flats, Oculus, Nocturne).
    BeginLobbyVenue(VenueKey)
    {
        const Target = LobbyVenues.some((V) => V.Key === VenueKey) ? VenueKey : "studio";
        if (this.Lobby && Target === this.LobbyVenueKey && !this.SceneSwitching)
            return;
        try { sessionStorage.setItem("frontier.lobbyVenue", Target); } catch {}
        const KeepCamera = (this.Lobby && (this.Settings.CameraMode === "HMI" || this.Settings.CameraMode === "Cockpit"))
            ? this.Settings.CameraMode
            : null;
        return this.SwitchSceneInPlace("lobby", Target, { PreserveCameraMode: KeepCamera });
    }

    // 📝 Live in-place scene swap: reuses the WebGPU device, ALL compiled WGSL shader modules, ALL compiled
    //    render/compute pipelines, and cached .facet vehicle meshes — ZERO page reload, ZERO shader recompile!
    async SwitchSceneInPlace(TargetLevelKey, TargetVenueKey = "studio", Options = {})
    {
        if (this.SceneSwitching)
        {
            this.PendingSwitch = { TargetLevelKey, TargetVenueKey, Options };
            return;
        }
        this.SceneSwitching = true;
        const SwitchStartMs = performance.now();
        try
        {
            const Device = this.Exchange.Device;
            await Device.queue.onSubmittedWorkDone();

            const WasLobby = this.Lobby;
            const NextLobby = TargetLevelKey === "lobby";
            const NextVenueKey = LobbyVenues.some((V) => V.Key === TargetVenueKey) ? TargetVenueKey : "studio";
            const NextLevelKey = NextLobby
                ? "lobby"
                : (LevelCatalog.some((E) => E.Key === TargetLevelKey) ? TargetLevelKey : "inversion");

            this.Lobby = NextLobby;
            this.LobbyVenueKey = NextVenueKey;
            this.LobbyVenue = ResolveLobbyVenueDescriptor(NextVenueKey);
            this.LevelKey = NextLevelKey;
            if (!NextLobby)
                this.SelectedRouteLevel = NextLevelKey;
            if (NextLevelKey === "demolish")
                PrepareDemolishCity(Options.Seed !== undefined ? Options.Seed : null);
            if (NextLevelKey === "labyrinth")
                PrepareLabyrinthMaze(Options.Seed !== undefined ? Options.Seed : null);
            this.LevelDescriptor = ResolveLevelDescriptor(NextLevelKey, NextVenueKey);
            this.Spawn = this.LevelDescriptor.Spawn;
            this.LevelZones = this.LevelDescriptor.Zones;

            AssignCourseReliefEnabled(this.LevelDescriptor.Relief);
            AssignCustomTerrainHeight(this.LevelDescriptor.TerrainHeight || null);
            AssignExcavationCutouts(this.LevelDescriptor.PhysicsCutouts || []);

            const Query = new URLSearchParams(location.search);
            Query.delete("lobby");
            Query.set("level", NextLevelKey);
            if (NextLobby)
                Query.set("venue", NextVenueKey);
            else
                Query.delete("venue");
            if (typeof history !== "undefined" && history.replaceState)
                history.replaceState(null, "", `?${Query.toString()}`);

            const VenueLight = NextLobby ? this.LobbyVenue.Lighting : null;
            this.StressCapacity = NextLobby ? 0 : (Query.has("stresspool") ? Math.max(0, Number(Query.get("stresspool")) || 0) : 1536);

            Object.assign(this.Settings, {
                Level: NextLobby ? this.SelectedRouteLevel : NextLevelKey,
                AeroGripScale: this.LevelDescriptor.AeroGrip,
                PanelVisible: !NextLobby,
                SmokeEnabled: !NextLobby,
                TyreMarks: !NextLobby,
                Sparks: !NextLobby,
                SunElevation: VenueLight ? VenueLight.SunElevation : (NextLevelKey === "labyrinth" ? 54.0 : 38.0),
                SunAzimuth: VenueLight ? VenueLight.SunAzimuth : (NextLevelKey === "labyrinth" ? 132.0 : 145.0),
                SolarIlluminance: VenueLight ? VenueLight.SolarIlluminance : 12.0,
                SkyScale: VenueLight ? VenueLight.SkyScale : 1.0,
                EmissionScale: VenueLight ? VenueLight.EmissionScale : 1.0,
                CameraMode: Options.PreserveCameraMode || (NextLobby ? "Orbit" : "Chase")
            });

            // ① Rebuild Scene (uses in-memory FacetCache for 0 ms vehicle .facet decode!)
            const NewScene = new SceneStructure();
            await NewScene.ConstructScene("Archive", this.StressCapacity, NextLevelKey, NextVenueKey);
            this.Scene = NewScene;

            // ② Rebuild vehicle solver & respawn onto the new level / single-hero Lobby floor
            this.Vehicle = new VehicleSolver(this.Scene.QueryObstacleBounds());
            this.Vehicle.Surfaces = this.LevelDescriptor.Surfaces;
            this.Vehicle.AeroGripScale = this.Settings.AeroGripScale;
            const SpawnBaseZ = NextLobby ? 0.0 : ResolveCourseHeight(this.Spawn.Position[0], this.Spawn.Position[1]);
            this.Vehicle.Respawn(
                NextLobby
                    ? [0.0, 0.0, VehicleGeometry.CoMHeight]
                    : [this.Spawn.Position[0], this.Spawn.Position[1], SpawnBaseZ + VehicleGeometry.CoMHeight + 0.02],
                NextLobby ? 0.0 : this.Spawn.Yaw
            );
            this.Vehicle.AssignHullPoints(ConstructHullPoints(this.Scene.FacetSets[this.Scene.VehicleBodySet].Facets), HullPointRadius);

            // Preserve user's exterior finish while switching Headlight lenses between showroom smoked glass & road beams
            const BaseMaterials = ConstructMaterialSpecification();
            if (NextLobby)
            {
                this.Materials[CourseMaterial.Headlight].Emission = [0.0, 0.0, 0.0];
                this.Materials[CourseMaterial.Headlight].Albedo   = [0.05, 0.055, 0.06];
            }
            else
            {
                this.Materials[CourseMaterial.Headlight].Emission = BaseMaterials[CourseMaterial.Headlight].Emission.slice();
                this.Materials[CourseMaterial.Headlight].Albedo   = BaseMaterials[CourseMaterial.Headlight].Albedo.slice();
            }
            Device.queue.writeBuffer(this.MaterialRecords, CourseMaterial.Headlight * 48,
                                     EncodeMaterialRecords([this.Materials[CourseMaterial.Headlight]]));

            // ③ Replace vertex buffer, rebuild SDF volumes & cascades, rebind surface cards & BVH (ZERO shader compile!)
            const PriorVertices = this.VertexRecords;
            this.VertexRecords = this.Exchange.ConstructRecordsFrom(this.Scene.EncodeVertexRecords(), GPUBufferUsage.VERTEX | GPUBufferUsage.COPY_DST, "Facet vertices");
            PriorVertices?.destroy();

            this.Lighting.ScrollingGrid = (NextLevelKey === "inversion");
            this.Lighting.StudioLights = NextLobby ? ResolveLobbyStudioLights(NextVenueKey) : [];

            await this.Volumes.ConstructVolumes(this.Scene, this.Grade.VoxelScale);
            this.Distance.Cutouts = this.LevelDescriptor.DistanceCutouts || [];
            this.Distance.RebindVolumes(this.Volumes.TextureView, this.OccupantRecords);
            this.Distance.UploadDistanceList(this.Scene.DistanceList);

            this.Cards.RebindCards(this.Scene, this.FrameRecords, this.OccupantRecords, this.MaterialRecords);
            this.PositionVehicle();
            this.Scene.RetainOccupantTransform(this.Scene.VehicleOccupants.Body);
            for (const Wheel of this.Scene.VehicleOccupants.Wheels)
                this.Scene.RetainOccupantTransform(Wheel);
            Device.queue.writeBuffer(this.OccupantRecords, 0, this.Scene.EncodeOccupantRecords(this.Volumes));
            this.Scene.ConstructDrawRuns();
            this.SamplingGroup = this.Cards.ConstructSamplingGroup(this.OccupantRecords, this.Atmosphere.SkyView, this.Atmosphere.SkySampler);

            this.RayTracing.ConstructResources(this.Scene);
            this.StreetLampPoles = this.ResolveStreetLampPoles(this.LevelDescriptor);

            // ④ Re-create Jolt physics & scene sequences for the new environment
            this.Bodies = await JoltWorld.Create({
                CourseRelief: this.LevelDescriptor.Relief,
                TerrainHulls: this.LevelDescriptor.TerrainHulls
            });
            this.Bodies.AssignObstacles(this.Scene.QueryObstacleBounds());
            this.Vehicle.Support = this.Bodies;
            this.Stress = new StressSequence(this);
            this.Cones = new ConeSequence(this);
            this.Bollards = new BollardSequence(this);
            this.Collapse = this.Scene.Collapse;
            this.Trams = this.Scene.Trams;
            if (this.Trams)
                this.Trams.RegisterDynamicObstacles(this.Vehicle.Obstacles);
            this.ServiceStations = this.Scene.ServiceStations;
            if (this.ServiceStations)
                this.ServiceStations.OnAnnounce = (Message) => this.Shell?.Announce(Message);
            this.TrafficLights = this.Scene.TrafficLights;
            if (this.TrafficLights)
            {
                this.TrafficLights.Advance(0.0);
                this.UploadTrafficLightOccupants();
            }
            this.RoadEditor = this.Scene.RoadEditor;
            if (this.RoadEditor)
                this.RoadEditor.AttachHost(this);
            this.Fracture = new FractureSequence(this);
            this.Fracture.OnBreak = (Prop) =>
            {
                this.Shell?.Announce(`${Prop.Definition.Name} — ${Prop.Pieces.length} fragments`);
                this.DemolishRun?.OnWallBroken?.(Prop);
                this.PistonLabyrinth?.OnWallBroken?.(Prop);
            };
            this.DDayCombat = new DDayCombatSequence(this);
            this.UploadDDayOccupants();
            this.CandyVillage?.Dispose?.();
            this.CandyVillage = new CandyVillageSequence(this);
            this.UploadCandyOccupants();
            this.FoundryConveyor?.Dispose?.();
            this.FoundryConveyor = new FoundryConveyorSequence(this);
            this.UploadFoundryOccupants();
            this.DemolishRun?.Dispose?.();
            this.DemolishRun = new DemolishRunSequence(this);
            this.UploadDemolishOccupants();
            this.PistonLabyrinth?.Dispose?.();
            this.PistonLabyrinth = new PistonLabyrinthSequence(this);
            this.UploadLabyrinthOccupants();
            this.LobbyRoster = new LobbyRosterSequence(this);
            this.UploadLobbyOccupants();
            this.Training = new TrainingSequence(this);

            this.Effects.Scene = this.Scene;
            this.Effects.ConstructVehicleCells(this.Vehicle);
            this.Effects.ClearVehicleMask();
            this.Effects.ApplyPalette(this.MaterialRecords);
            this.Marks?.Clear();
            this.Sparks?.Clear();
            this.Smoke?.Clear();
            this.BurstPuffs = [];
            this.Thermal = new TyreThermalSolver(4);
            this.VehicleVisual = new VehicleVisualState();
            Device.queue.writeBuffer(this.VehicleRecords, 0, this.VehicleVisual.EncodeRecord(this.QueryCabinHeader(), this.Vehicle.Wheels, this.Vehicle.Orientation));
            this.Cabin.RebindScene(this.Scene);
            this.DrivingHud?.TrackedTargets?.clear();
            this.DrivingHud?.BuildSpatialIndex?.();

            // ⑤ Capture new Surface Cards & rebind screen/resampling targets (for new BVH buffers & layout changes)
            this.Cards.CaptureCards(this.Scene, this.VertexRecords);
            this.VehicleCardsDirty = false;
            this.Chase.Surfaces = this.Vehicle.Surfaces || this.LevelDescriptor.Surfaces || [];
            this.Chase.Seeded = false;
            this.FreeCamera = false;
            this.LobbyCinematic = null;
            this.LobbyCinematicPose = null;
            this.LobbyBlend = null;
            this.LobbyLagEye = null;
            this.LobbyLagTarget = null;
            this.PriorLight = null;
            this.PreviousUnjittered = null;
            this.PriorValid = false;

            if (NextLobby)
            {
                const VenueCam = this.LobbyVenue?.EntryCamera || {
                    From: { Yaw: 2.0, Pitch: 0.14, Distance: 14.5 },
                    To:   { Yaw: 0.92, Pitch: 0.22, Distance: 10.2 }
                };
                this.LobbyEntry = {
                    Seconds: 0.0,
                    Duration: 1.65,
                    From: { ...VenueCam.From },
                    To:   { ...VenueCam.To }
                };
                Object.assign(this.Orbit, this.LobbyEntry.From);
                this.LobbyIdleSince = performance.now();
                this.RevealLobby();
            }
            else
            {
                this.LobbyEntry = null;
                this.LobbyAudio?.Stop();
            }

            this.ConstructPanel();
            this.BindWorkspaceAndLobbyUi();
            this.ReconstructScreenTargets(true);
            this.Display?.ResetExposure();

            await Device.queue.onSubmittedWorkDone();
            this.SceneSwitchCount = (this.SceneSwitchCount || 0) + 1;
            this.LastSwitchMs = Math.round(performance.now() - SwitchStartMs);
            console.info(`[scene-switch] #${this.SceneSwitchCount} → ${NextLevelKey}${NextLobby ? ` (${NextVenueKey})` : ""} in ${this.LastSwitchMs} ms · 0 reloads · ${this.Shaders.Modules.size} cached WGSL modules`);
            if (WasLobby !== NextLobby)
                this.SyncLobbyHudUi();
        }
        finally
        {
            this.SceneSwitching = false;
            if (this.PendingSwitch)
            {
                const Next = this.PendingSwitch;
                this.PendingSwitch = null;
                this.SwitchSceneInPlace(Next.TargetLevelKey, Next.TargetVenueKey, Next.Options);
            }
        }
    }

    CaptureCurrentCameraPose()
    {
        const Eye = (this.LobbyCinematicPose?.Eye || this.LobbyLagEye || this.CameraState?.Position || [7.5, -5.5, 2.0]).slice();
        const Target = (this.LobbyCinematicPose?.Target || this.LobbyLagTarget || this.DisplayedTarget || [0.0, 0.0, 0.72]).slice();
        const Fov = this.LobbyCinematicPose?.Fov || this.DisplayedFov || 45.0;
        return { ShotIndex: this.LobbyCinematic?.Shot ?? 0, ShotTitle: "", Duration: 1.0, Eye, Target, Fov };
    }

    ToggleLobbyCinematic()
    {
        if (!this.Lobby)
            return;
        if (this.LobbyCinematic)
        {
            this.LobbyCinematic = null;
            this.SetCinematicVeil(0);
            this.LobbyIdleSince = performance.now();
            this.LobbyUserControl = true;
        }
        else
        {
            const FromPose = this.CaptureCurrentCameraPose();
            this.LobbyEntry = null;
            this.LobbyBlend = null;
            this.Settings.CameraMode = "Orbit";
            this.LobbyCinematic = {
                Shot: 0,
                Seconds: 0.0,
                Phase: "Transition",
                TransitionDuration: 1.35,
                FromPose
            };
            this.LobbyCinematicPose = FromPose;
            this.SetCinematicVeil(0);
        }
        this.SyncLobbyHudUi();
    }

    NoteLobbyInteraction()
    {
        this.LobbyIdleSince = performance.now();
        this.LobbyUserControl = true;
        this.LobbyAudio?.Unlock();
        if (this.LobbyCinematic)
        {
            this.LobbyCinematic = null;
            this.SetCinematicVeil(0);
            this.SyncLobbyHudUi();
        }
    }

    LobbyIdleThreshold()
    {
        return this.LobbyUserControl ? 60000.0 : 25000.0;
    }

    SetCinematicVeil(Opacity)
    {
        const Veil = document.getElementById("cinematic-fade");
        if (Veil?.style)
        {
            Veil.style.transition = "opacity 0.65s ease";
            Veil.style.opacity = String(Opacity);
        }
    }

    // 📝 Called whenever Cabin.Appearance changes paint, finish, wheels, or rim accents: schedules a Surface Card
    //    recapture + lighting invalidation + temporal GI reset so SDF traces, reflections & GI immediately match!
    MarkVehicleAppearanceDirty()
    {
        this.VehicleCardsDirty = true;
        if (this.Effects && this.Materials)
            this.Effects.LightAlbedo = this.Materials[CourseMaterial.BodyCoat].Albedo;
    }

    SkipLobbyCinematicShot()
    {
        if (!this.Lobby)
            return;
        if (!this.LobbyCinematic)
        {
            this.ToggleLobbyCinematic();
            return;
        }
        const Cinema = this.LobbyCinematic;
        const FromPose = this.CaptureCurrentCameraPose();
        Cinema.Shot = (Cinema.Shot + 1) % LobbyCinematicShots.length;
        Cinema.Seconds = 0.0;
        Cinema.Phase = "Transition";
        Cinema.TransitionDuration = 0.90;
        Cinema.FromPose = FromPose;
        this.SetCinematicVeil(0);
        this.SyncLobbyHudUi();
    }

    // 📝 8-shot Continuous One-Take Automotive Film Director reel with dynamic lift spotlighting & venue bounds.
    ResolveLobbyCinematicPose(Shot, Fraction)
    {
        const HeroPos = this.Vehicle ? this.Vehicle.Position : [0.0, 0.0, 0.52];
        const ActiveLift = this.LobbyRoster ? this.LobbyRoster.QueryActiveLiftFocus() : null;
        return EvaluateCinematicReelShot(Shot, Fraction, HeroPos, ActiveLift, this.Clock || 0.0, this.LobbyVenue?.CameraBounds);
    }

    // 📝 ZERO-FADE continuous Steadicam director: smoothly sweeps along a polar crane arc from shot to shot
    //    with zero black cuts, zero fade-outs, and zero car-body clipping!
    AdvanceLobbyCinematic(Δτ)
    {
        const Cinema = this.LobbyCinematic;
        if (!Cinema)
            return;
        Cinema.Seconds += Δτ;
        const ActiveSpec = LobbyCinematicShots[Cinema.Shot % LobbyCinematicShots.length];
        const ShotDuration = ActiveSpec ? ActiveSpec.Duration : 7.0;
        if (Cinema.Phase === "Transition")
        {
            const BridgeDuration = Cinema.TransitionDuration || 2.0;
            const Progress = Math.min(1.0, Cinema.Seconds / BridgeDuration);
            const HeroPos = this.Vehicle ? this.Vehicle.Position : [0.0, 0.0, 0.52];
            const TargetStart = this.ResolveLobbyCinematicPose(Cinema.Shot, 0.0);
            this.LobbyCinematicPose = BlendCinematicPoses(
                Cinema.FromPose || TargetStart,
                TargetStart,
                Progress,
                HeroPos,
                this.LobbyVenue?.CameraBounds
            );
            if (Progress >= 1.0)
            {
                Cinema.Phase = "Shot";
                Cinema.Seconds = 0.0;
                Cinema.FromPose = null;
            }
        }
        else
        {
            const Progress = Math.min(1.0, Cinema.Seconds / ShotDuration);
            this.LobbyCinematicPose = this.ResolveLobbyCinematicPose(Cinema.Shot, Progress);
            if (Cinema.Seconds >= ShotDuration)
            {
                Cinema.FromPose = this.LobbyCinematicPose;
                Cinema.Shot = (Cinema.Shot + 1) % LobbyCinematicShots.length;
                Cinema.Phase = "Transition";
                Cinema.Seconds = 0.0;
                Cinema.TransitionDuration = 2.05;
                this.SyncLobbyHudUi();
            }
        }
    }

    AssignCamera(Free)
    {
        if (this.Lobby)
            return;
        if (Free && !this.FreeCamera)
            this.Flight.AssignFromChase(this.Chase.Eye, this.Chase.Target);
        this.FreeCamera = Free;
        if (this.Shell)
            this.Shell.Controls.Synchronise();
    }

    // 📝 R: derez, and rez at the active drill's start (or the spawn).
    RespawnVehicle(Instant = false)
    {
        if (this.Lobby)
            return;
        const Drill = this.Training ? this.Training.QueryActiveDrill() : null;
        const BaseSpawn = this.Spawn || SpawnPoint;
        const Destination = Drill ? { Position: Drill.Teleport.Position.slice(), Yaw: Drill.Teleport.Yaw * Math.PI / 180.0 }
                                  : { Position: BaseSpawn.Position.slice(0, 2), Yaw: BaseSpawn.Yaw };
        // 📝 The rez puts fresh tyres on (RecoveryDirector → RestoreTyres); an instant respawn does it here.
        if (!Instant && this.Recovery && this.Recovery.Begin(RecoveryReasons.Respawn, Destination, () => { if (this.Training) this.Training.OnRespawn(); }))
            return;
        if (Instant || !this.Recovery || !this.Recovery.Busy)
        {
            const RespBaseZ = ResolveCourseHeight(Destination.Position[0], Destination.Position[1]);
            this.Vehicle.Respawn([Destination.Position[0], Destination.Position[1], RespBaseZ + 0.58], Destination.Yaw);
            this.RestoreTyres();
            this.AfterVehicleMoved();
        }
    }

    // 📝 T: every displaced cone rezzes back onto its mark and every broken prop is rebuilt.
    ResetArena()
    {
        if (this.Lobby)
            return;
        const Cones = this.Cones.ResetCones(null, true);
        let Props = 0;
        for (const Prop of this.Fracture.Props)
            if (Prop.State !== "intact")
            {
                this.Fracture.ReassembleProp(Prop);
                Props++;
            }
        if (this.Collapse)
            this.ResetCollapseDecks();
        if (this.ServiceStations)
            this.ResetServiceStations(true);
        this.Shell.Announce(`Arena reset — ${Cones} cones, ${Props} props`);
    }

    //--------------------------------------------------------------------------------------------------------------------
    //                                                SCREEN TARGETS
    //--------------------------------------------------------------------------------------------------------------------

    ReconstructScreenTargets(Force = false)
    {
        if (!this.Surface)
            return;
        if (Force)
            this.ScreenExtent = null;
        let Ratio = Math.min(window.devicePixelRatio || 1.0, 2.0) * this.Grade.RenderScale;
        const Pixels = this.Canvas.clientWidth * this.Canvas.clientHeight * Ratio * Ratio;
        if (Pixels > this.Grade.PixelLimit)
            Ratio *= Math.sqrt(this.Grade.PixelLimit / Pixels);
        const Width  = Math.max(64, Math.floor(this.Canvas.clientWidth * Ratio));
        const Height = Math.max(64, Math.floor(this.Canvas.clientHeight * Ratio));
        if (this.Dock)
            this.Dock.PlaceInset(this.Canvas, [Width, Height]);
        const Scale = this.Settings.TraceScale;
        if (this.ScreenExtent && this.ScreenExtent[0] === Width && this.ScreenExtent[1] === Height && this.TraceScaleUsed === Scale)
            return;
        this.Canvas.width = Width;
        this.Canvas.height = Height;
        this.ScreenExtent = [Width, Height];
        this.TraceScaleUsed = Scale;
        this.TraceExtent = [Math.ceil(Width / Scale), Math.ceil(Height / Scale)];
        this.Surface.ConstructTargets(Width, Height, this.Shadow, this.Atmosphere, this.LightRecords, this.ClusterRecords);
        this.NearestDepth.ConstructTargets(this.ScreenExtent, this.Surface);
        this.Resampling.ConstructTargets(this.TraceExtent, this.Surface, this.RayTracing);
        this.Diffuse.ConstructTargets(this.TraceExtent, this.ScreenExtent, this.Surface,
                                      [this.Resampling.Views.Irradiance, this.Resampling.Views.Irradiance], this.Resampling.Views.Guides);
        this.Glossy.ConstructTargets(this.TraceExtent, this.ScreenExtent, this.Surface,
                                     [this.Resampling.Views.Reflection, this.Resampling.Views.Reflection], this.Resampling.Views.Guides);
        this.Display.ConstructTargets(this.ScreenExtent, this.Surface, this.Resampling, this.Diffuse, this.Glossy, this.Cards);
        this.NearestDepth.ConstructTraversalGroups(this.Surface, this.Display);
        this.Smoke.ConstructScreenTargets(this.ScreenExtent, this.Surface, this.Display, this.Settings.SmokeResolution, this.TraceExtent);
        this.PriorValid = false;
        if (this.Capture)
        {
            if (this.CaptureTexture)
                this.CaptureTexture.destroy();
            this.CaptureTexture = this.Exchange.ConstructTexture({ label: "Capture", size: [Width, Height], format: "rgba8unorm",
                                                                   usage: GPUTextureUsage.RENDER_ATTACHMENT | GPUTextureUsage.COPY_SRC,
                                                                   viewFormats: ["rgba8unorm-srgb"] });
            this.CaptureView = this.CaptureTexture.createView({ format: "rgba8unorm-srgb" });
            this.CaptureStride = Math.ceil(Width * 4 / 256) * 256;
            this.CaptureRecords = this.Exchange.ConstructRecords(this.CaptureStride * Height, GPUBufferUsage.MAP_READ | GPUBufferUsage.COPY_DST, "Capture readback");
            this.CapturePending = false;
        }
    }

    //--------------------------------------------------------------------------------------------------------------------
    //                                                    FRAME
    //--------------------------------------------------------------------------------------------------------------------

    PositionVehicle()
    {
        if (this.Lobby && this.Vehicle && this.LobbyRoster)
            this.Vehicle.Position[2] = VehicleGeometry.CoMHeight + (this.LobbyRoster.StageZ || 0.0);
        const Body = this.Vehicle ? this.Vehicle.QueryBodyTransform() : null;
        if (!Body)
            return;
        this.Scene.AssignOccupantTransform(this.Scene.VehicleOccupants.Body, Body);
        for (let Wheel = 0; Wheel < 4; ++Wheel)
            this.Scene.AssignOccupantTransform(this.Scene.VehicleOccupants.Wheels[Wheel], this.Vehicle.QueryWheelTransform(Wheel));
    }

    // 📝 The Smoke + Fire editor's test emitter: a steady smoke source, or a burning one, 3 m ahead of the car — so
    //    every slider can be judged without driving. It is one more emission beside the four tyres.
    ResolveTestPlume(Burning)
    {
        const Forward = this.Vehicle.QueryForward();
        const Point = [this.Vehicle.Position[0] + 3.0 * Forward[0], this.Vehicle.Position[1] + 3.0 * Forward[1], 0.05];
        // 📝 The fire matches one burning tyre of TyreThermalSolver (60 kW: 2 g/s of rubber at 30 MJ/kg) — the flame
        //    at the hub and the soot plume 0.55 m above it, with the same rates — so what is tuned here is what burns.
        return {
            Active: true, Burning, Strength: 1.6,
            Position: Burning ? [Point[0], Point[1], 0.25] : Point,
            Hub: [Point[0], Point[1], 0.35],
            Radius: Burning ? 0.45 : 0.35,
            Expansion: Burning ? 5.0 : 0.6,
            SmokeRate: Burning ? 0.0 : 26.0,
            SootRate: 0.0,
            FlameRate: Burning ? 12.0 : 0.0,
            HeatRate: Burning ? 39.0 : 1.2,
            Velocity: [0.0, 0.0, Burning ? 1.2 : 0.35],
            Plume: Burning ? { Offset: [0.0, 0.0, 0.65], Radius: 0.45, SmokeRate: 12.0, SootRate: 18.0, HeatRate: 21.0, Expansion: 2.5 } : null,
            CombustionPower: Burning ? 60000.0 : 0.0
        };
    }

    ResolveCamera(Δτ)
    {
        const Telemetry = this.Vehicle.QueryTelemetry();
        const VehicleUp = RotateVector(this.Vehicle.Orientation, [0.0, 0.0, 1.0]);
        const SurfaceContact = this.Vehicle.Wheels.some((Wheel) => Wheel.Contact);
        this.Chase.AdvanceChase(this.Vehicle.Position, this.Vehicle.QueryForward(), Telemetry.Speed, Δτ, VehicleUp, SurfaceContact);
        let Eye, Target, FieldOfView, Up = [0.0, 0.0, 1.0], Near = 0.05;
        const Rig = CameraRigs[this.Settings.CameraMode];
        if (this.FreeCamera)
        {
            this.Flight.AdvanceFlight(this.Keys, Δτ);
            Eye = this.Flight.Eye;
            Target = MultiplyAddVector(Eye, this.Flight.ResolveForward(), 10.0);
            FieldOfView = this.Flight.FieldOfView;
        }
        else if (Rig?.OrbitVehicle) {
            const View = this.Orbit.Resolve(this.Vehicle.Position, ResolveCourseHeight);
            Eye = View.Eye; Target = View.Target; FieldOfView = View.Fov;
        }
        else if (Rig?.FocusTablet) {
            const Body = this.Vehicle.QueryBodyTransform();
            const Focus = ResolveHmiCamera(this.Settings.HmiScale, this.ScreenExtent[0] / this.ScreenExtent[1], this.Settings.HmiZoom);
            Eye = TransformPoint(Body, Focus.Eye); Target = TransformPoint(Body, Focus.Target);
            Up = NormalizeVector(TransformDirection(Body, Focus.Up)); FieldOfView = Focus.Fov; Near = 0.01;
        }
        else if (Rig)
        {
            // 📝 A rig fixed to the car: it rolls and pitches with the body; the interior rigs look around by drag.
            const Body = this.Vehicle.QueryBodyTransform();
            // 📝 Fixed authored seated eyes; HUD sizing is independent of physical camera/cabin poses.
            Eye = TransformPoint(Body, Rig.Hip ? PlaceSeatedEye(Rig.Eye, Rig.Hip, ResolveCockpitFrame(this.Settings)) : Rig.Eye);
            let Look;
            if (Rig.Target)
                Look = NormalizeVector(SubtractVector(Rig.Target, Rig.Eye));
            else
            {
                const Pitch = Rig.Pitch + (Rig.Interior ? this.HeadPitch : 0.0), Yaw = Rig.Interior ? this.HeadYaw : 0.0;
                Look = [Math.cos(Pitch) * Math.cos(Yaw), Math.cos(Pitch) * Math.sin(Yaw), -Math.sin(Pitch)];
            }
            Target = MultiplyAddVector(Eye, TransformDirection(Body, Look), 10.0);
            Up = NormalizeVector(TransformDirection(Body, [0.0, 0.0, 1.0]));
            FieldOfView = ["Cockpit", "Passenger"].includes(this.Settings.CameraMode) ? this.Settings.CockpitFov : Rig.Fov;
            Near = Rig.Interior ? 0.02 : 0.03;
        }
        else
        {
            Eye = this.Chase.Eye;
            Target = this.Chase.Target;
            Up = this.Chase.Up || [0.0, 0.0, 1.0];
            FieldOfView = 60; // speed widening is smoothed once below for all driving cameras
        }
        this.CameraMotion ||= new DrivingCameraMotion();
        const Motion = this.CameraMotion.Apply({ Eye, Target, Up, Fov: FieldOfView }, Δτ, this.Vehicle.LinearVelocity, {
            Active: !this.Lobby && !this.FreeCamera && !this.Vehicle.Hidden && ["Chase", "Cockpit", "Passenger", "Hood"].includes(this.Settings.CameraMode),
            Shake: this.Settings.CameraShake, Strength: this.Settings.CameraShakeStrength,
            SpeedFov: this.Settings.SpeedFov, FovBoost: this.Settings.SpeedFovBoost
        });
        Eye = Motion.Eye; Target = Motion.Target; Up = Motion.Up; FieldOfView = Motion.Fov;
        if (this.Lobby)
        {
            if (this.LobbyCinematic && this.LobbyCinematicPose && this.Settings.CameraMode === "Orbit")
            {
                Eye = this.LobbyCinematicPose.Eye;
                Target = this.LobbyCinematicPose.Target;
                FieldOfView = this.LobbyCinematicPose.Fov;
            }
            if (this.LobbyBlend)
            {
                const Blend = this.LobbyBlend;
                Blend.Seconds += Δτ;
                const F = EaseCinematic(Blend.Seconds / Blend.Duration);
                Eye = Blend.FromEye.map((V, I) => V + (Eye[I] - V) * F);
                Target = Blend.FromTarget.map((V, I) => V + (Target[I] - V) * F);
                FieldOfView = Blend.FromFov + (FieldOfView - Blend.FromFov) * F;
                if (Blend.Seconds >= Blend.Duration)
                    this.LobbyBlend = null;
                this.LobbyLagEye = Eye.slice();
                this.LobbyLagTarget = Target.slice();
            }
            else if (Rig?.OrbitVehicle && !this.LobbyCinematic)
            {
                if (!this.LobbyLagEye) { this.LobbyLagEye = Eye.slice(); this.LobbyLagTarget = Target.slice(); }
                Eye = this.LobbyLagEye = BlendVector(this.LobbyLagEye, Eye, ResolveLagBlend(Δτ, 0.34));
                Target = this.LobbyLagTarget = BlendVector(this.LobbyLagTarget, Target, ResolveLagBlend(Δτ, 0.20));
            }
            else
            {
                this.LobbyLagEye = Eye.slice();
                this.LobbyLagTarget = Target.slice();
            }
            if (this.Settings.CameraMode === "Orbit")
                Eye = ClampStudioEye(Eye, this.LobbyVenue?.CameraBounds);
            this.DisplayedTarget = Target.slice();
            this.DisplayedFov = FieldOfView;
        }
        const Forward = NormalizeVector(SubtractVector(Target, Eye));
        this.CameraEye = Eye;
        const Right = NormalizeVector(CrossVector(Forward, Up));
        const TanY = Math.tan(0.5 * FieldOfView * Math.PI / 180.0);
        this.CameraState = { Position: Eye.slice(), Forward, Right, Up: CrossVector(Right, Forward), TanY, Aspect: this.ScreenExtent[0] / this.ScreenExtent[1] };
        return { Position: Eye, Target, Forward, Up, VerticalFieldOfView: FieldOfView * Math.PI / 180.0,
                 Aspect: this.ScreenExtent[0] / this.ScreenExtent[1], Near };
    }

    EncodeFrameRecord(Camera, Light)
    {
        const [Width, Height] = this.ScreenExtent;
        const Jittered = this.Settings.Antialias;
        const Sequence = (this.FrameIndex % 16) + 1;
        const JitterX = Jittered ? (ProjectRadicalInverse(Sequence, 2) - 0.5) * 2.0 / Width : 0.0;
        const JitterY = Jittered ? (ProjectRadicalInverse(Sequence, 3) - 0.5) * 2.0 / Height : 0.0;
        const View = ConstructViewTransform(Camera.Position, Camera.Target, Camera.Up || [0.0, 0.0, 1.0]);
        const Projection = ConstructPerspectiveTransform(Camera.VerticalFieldOfView, Camera.Aspect, Camera.Near, JitterX, JitterY);
        const Unjittered = MultiplyTransform(ConstructPerspectiveTransform(Camera.VerticalFieldOfView, Camera.Aspect, Camera.Near), View);
        const ViewProjection = MultiplyTransform(Projection, View);
        this.ViewProjectionInverse = InvertTransform(ViewProjection);
        const Previous = this.PreviousUnjittered || Unjittered;
        this.PreviousUnjittered = Unjittered;

        const Bytes = new ArrayBuffer(FrameRecordExtent);
        const Scalars = new Float32Array(Bytes);
        const Integers = new Uint32Array(Bytes);
        Scalars.set(ViewProjection, 0);
        Scalars.set(InvertTransform(ViewProjection), 16);
        Scalars.set(Unjittered, 32);
        Scalars.set(Previous, 48);
        Scalars.set(View, 64);
        Scalars.set(Camera.Position, 80);
        Integers[83] = this.FrameIndex;
        Scalars.set(Light.Direction, 84);
        Scalars[87] = performance.now() * 0.001;
        Scalars.set(Light.Illuminance, 88);
        Scalars[91] = this.Settings.SkyScale;
        Scalars.set([Width, Height, 1.0 / Width, 1.0 / Height], 92);
        Scalars.set([this.TraceExtent[0], this.TraceExtent[1], 1.0 / this.TraceExtent[0], 1.0 / this.TraceExtent[1]], 96);
        Scalars.set([JitterX, JitterY], 100);
        Integers[102] = this.TraceScaleUsed;
        let Features = 0;
        for (const [Key, Bit] of Object.entries(FeatureBits))
            if (this.Settings[Key]) Features |= Bit;
        if (!this.PriorValid)
        {
            Features &= ~(FeatureBits.TemporalReuse);
            Features |= TemporalRestartBit;
        }
        if (this.Smoke.Resident && this.Settings.SmokeEnabled && !this.Omitted.has("Smoke")) Features |= 4096;
        // 📝 The camera component's 8-bit strength occupies reserved feature bits 16–23; the rest of the frame record stays ABI-stable.
        if (this.Settings.Inspection === 0)
            Features |= Math.round(this.ActiveCamera.MotionBlur.ResolveStrength() * 255.0) << 16;
        Integers[103] = Features;
        Integers[104] = this.Settings.Inspection;
        Scalars[105] = this.Settings.TemporalTallyLimit;
        Scalars[106] = this.Settings.SpatialRadius;
        Integers[107] = this.Settings.SpatialCount;
        Scalars.set(Light.NightSky, 108);
        Scalars[111] = this.Settings.EmissionScale;
        Scalars[112] = this.Settings.SliceHeight;
        Scalars[113] = this.Settings.ExposureCompensation;
        Scalars[114] = this.Settings.IndirectScale;
        Integers[115] = this.Settings.DistanceInspection;
        Scalars.set(this.Focus, 116);
        Integers[119] = (this.Dock ? this.Dock.InsetDrawn : this.Settings.InsetEnabled) ? 1 : 0;
        const Fire = this.Smoke.FireLight;
        Scalars.set(Fire.Position, 120);
        Scalars[123] = this.Settings.SmokeEnabled ? Fire.Intensity : 0.0;
        Scalars.set(Fire.Colour, 124);
        Scalars[127] = FireLightReach;
        Scalars[128] = Camera.Near;
        Integers[129] = this.Cards.GroundFirstCard;
        Integers[130] = this.Cards.GroundStripCard;
        Integers[131] = this.Scene.GroundOccupant;
        this.Effects.LightAlbedo = this.Materials[CourseMaterial.BodyCoat].Albedo;
        this.Exchange.Device.queue.writeBuffer(this.FrameRecords, 0, Bytes);
    }

    AdvanceFrame(Now)
    {
        if (this.Failed)
            return;
        if (this.SceneSwitching)
        {
            this.LastTime = Now;
            requestAnimationFrame((Next) => this.AdvanceFrame(Next));
            return;
        }
        // 📝 At most two frames in flight: without it a slow adapter queues work faster than it retires it and the
        //    controls lag by seconds. A swapchain throttles on its own; a capture target does not.
        if ((this.FramesInFlight || 0) >= 2)
        {
            requestAnimationFrame((Next) => this.AdvanceFrame(Next));
            return;
        }
        const CpuFrameStart = performance.now();
        const FrameDeltaMs = Math.max(0.0, Now - (this.PriorFrameTime || Now));
        const Δτ = Math.min(0.1, Math.max(0.0, (Now - this.LastTime) * 0.001));
        this.LastTime = Now;
        const Hud = document.getElementById("hud");
        if (Hud?.style) Object.assign(Hud.style, ResolveHudStyle(this.Settings));
        this.FrameSeconds = (this.FrameSeconds || Δτ) * 0.9 + Δτ * 0.1;
        this.FrameDurations[this.FrameDurationCursor] = FrameDeltaMs;
        this.FrameDurationCursor = (this.FrameDurationCursor + 1) % this.FrameDurations.length;
        this.PriorFrameTime = Now;
        const Device = this.Exchange.Device;
        const Parity = this.FrameIndex & 1;
        const CpuUpdateStart = performance.now();

        // ① drive: keyboard (or a scripted command) through the input ramps into the vehicle solver
        const Keys = this.FreeCamera ? new Set(["Space"]) : this.Keys;
        let Command = this.ScriptedInput ? this.ScriptedInput : this.Driver.Advance(Keys, Δτ, this.Vehicle.QuerySteeringFeedback(),
            { Enabled: this.Settings.SteeringAssist && !this.FreeCamera, Strength: this.Settings.SteeringAssistStrength });
        // 📝 Derezzing with momentum: the car carries on under the solver with nobody at the controls.
        if (this.Vehicle.Coasting)
            Command = { Throttle: 0.0, Brake: 0.0, Steer: 0.0, Handbrake: false };
        if (this.Lobby)
            Command = { Throttle: 0.0, Brake: 0.0, Steer: 0.0, Handbrake: true };
        const Step = this.FixedStep ? this.FixedStep : Δτ;
        this.Vehicle.AssignAssists({ LineLock: this.Settings.LineLock });
        if (this.Lobby)
            this.Effects.ClearVehicleMask();
        else
            this.Recovery.Advance(Step);
        this.Fracture.ResolveImpacts(this.Vehicle, Step);
        if (!this.Lobby)
            this.Vehicle.Advance(Command, Step);
        this.Vehicle.AdvanceTyreWear(Step);
        const DDayMoved    = this.DDayCombat ? this.DDayCombat.Advance(Step) : false;
        const CandyMoved   = this.CandyVillage ? this.CandyVillage.Advance(Step) : false;
        const FoundryMoved = this.FoundryConveyor ? this.FoundryConveyor.Advance(Step) : false;
        const DemolishMoved = this.DemolishRun ? this.DemolishRun.Advance(Step) : false;
        const LabyrinthMoved = this.PistonLabyrinth ? this.PistonLabyrinth.Advance(Step) : false;
        const LobbyMoved   = this.LobbyRoster ? this.LobbyRoster.Advance(Step) : false;
        const VehicleEvents = this.Vehicle.ResolveEvents();
        this.PositionVehicle();
        this.Bodies.Advance(Step, this.Vehicle);
        this.Fracture.Advance(Step, this.Vehicle);
        this.Cones.Advance(Step);
        if (this.Bollards)
            this.Bollards.Advance(Step);
        const CollapseMoved = this.Collapse
            ? this.Collapse.Advance(this.Vehicle.Position, this.Vehicle.LinearVelocity, Step, this.Sparks)
            : false;
        const TramsMoved = this.Trams
            ? this.Trams.Advance(this.Vehicle, Step)
            : false;
        const ServiceMoved = this.ServiceStations
            ? this.ServiceStations.Advance(this.Vehicle, Step, this.Sparks)
            : false;
        this.DrivingHud?.Advance(Step);
        const TrafficLightsMoved = this.TrafficLights
            ? this.TrafficLights.Advance(Step)
            : false;
        this.Stress.Advance(Step);
        this.Effects.Advance(Step);
        this.Training.Advance(Step);

        // ①b tyres: sliding heat into tread, carcass and gas; ignition; the wheels' smoke and fire sources
        const Readings = this.Vehicle.DrainWheelReadings();
        const Ignition = IgnitionModes[this.Settings.IgnitionMode] || IgnitionModes[0];
        Object.assign(this.Thermal.Options, { Wear: this.Settings.TyreWear, WearScale: this.Settings.WearScale, Failure: this.Settings.TyreFailure });
        this.Thermal.Advance(Readings, Readings[0].Seconds, Ignition);
        this.Vehicle.AssignGrip(this.Settings.ThermalGrip ? this.Thermal.Wheels.map((Wheel) => Wheel.Grip) : [1.0, 1.0, 1.0, 1.0]);
        // 📝 Pressure from the gas law feeds the soft tyre; pinch cuts leak; bursts and shreds throw rubber; scrapes spark.
        for (const Pinch of VehicleEvents.Pinches)
            if (this.Thermal.Puncture(Pinch.Wheel, false, "pinch cut"))
                this.Shell.Announce(`${["FL", "FR", "RL", "RR"][Pinch.Wheel]} pinch cut — leaking`);
        for (const Burst of this.Thermal.DrainEvents())
            this.OnTyreBurst(Burst);
        this.Vehicle.AssignTyrePressures(this.Thermal.Wheels.map((Wheel) => Wheel.GaugePressure));
        for (const Wheel of VehicleEvents.Shreds)
            this.OnTyreShred(Wheel);
        this.Sparks.Settings.Enabled = this.Settings.Sparks;
        this.Sparks.Emit(VehicleEvents.Sparks, Step);
        this.Sparks.Advance(Step, this.Vehicle.Obstacles, this.Bodies.Bodies, this.Vehicle.Surfaces);
        Device.queue.writeBuffer(this.VehicleRecords, 0, this.VehicleVisual.EncodeRecord(this.QueryCabinHeader(), this.Vehicle.Wheels, this.Vehicle.Orientation));
        const Pumped = this.Cabin.Advance(Δτ, this.QueryTyreLive());
        if (this.Settings.CameraMode === "HMI" && this.Cabin.Metrics.TabletUploads !== this.PriorTabletUploads)
        {
            this.PriorTabletUploads = this.Cabin.Metrics.TabletUploads;
            this.PriorValid = false;
        }
        if (this.QueryInteriorCamera() && this.Cabin.Metrics.GlassHudUploads !== this.PriorGlassHudUploads)
        {
            this.PriorGlassHudUploads = this.Cabin.Metrics.GlassHudUploads;
            this.PriorValid = false;
        }
        if (this.Cabin.Screen?.RouteRequest)
        {
            const Route = this.Cabin.Screen.RouteRequest;
            this.Cabin.Screen.RouteRequest = null;
            this.BeginRoute(Route);
        }
        if (this.Cabin.Screen?.VenueRequest)
        {
            const Venue = this.Cabin.Screen.VenueRequest;
            this.Cabin.Screen.VenueRequest = null;
            this.BeginLobbyVenue(Venue);
        }
        if (this.Cabin.Screen && this.Cabin.Screen.StageLiftRequest !== null)
        {
            const Raised = this.Cabin.Screen.StageLiftRequest;
            this.Cabin.Screen.StageLiftRequest = null;
            this.LobbyAudio?.Unlock();
            this.LobbyRoster?.ToggleStageLift(Raised);
            this.SyncLobbyHudUi();
        }
        if (this.Cabin.Screen && this.Cabin.Screen.TogglePeerRequest !== null)
        {
            const PeerIdx = this.Cabin.Screen.TogglePeerRequest;
            this.Cabin.Screen.TogglePeerRequest = null;
            this.LobbyRoster?.TogglePlayerConnection(PeerIdx);
            this.SyncLobbyHudUi();
        }
        if (Pumped !== null)
        {
            this.InflateTyres(Pumped, Pumped);
            this.Shell.Announce(`Tyres pumped to ${Pumped} kPa`);
        }
        const Emissions = this.Thermal.ResolveEmissions(Readings, Readings[0].Seconds);
        Emissions.push(...this.ResolveBurstPuffs(Step));
        if (this.Collapse)
            Emissions.push(...this.Collapse.QuerySmokeEmissions());
        if (this.DDayCombat)
            Emissions.push(...this.DDayCombat.QueryPlumeEmissions());
        if (this.CandyVillage)
            Emissions.push(...this.CandyVillage.QueryPlumeEmissions());
        if (this.DemolishRun)
            Emissions.push(...this.DemolishRun.QueryPlumeEmissions());
        if (this.PistonLabyrinth)
            Emissions.push(...this.PistonLabyrinth.QueryPlumeEmissions());
        if (this.Settings.TestPlume > 0)
            Emissions.push(this.ResolveTestPlume(this.Settings.TestPlume === 2));
        this.Marks.Advance(Readings, { Enabled: this.Settings.TyreMarks, Strength: this.Settings.TyreMarkStrength });

        // ② camera, light, cascade focus
        if (this.Lobby && this.Settings.CameraMode === "Orbit" && !this.LobbyEntry && !this.LobbyBlend)
        {
            if (this.LobbyCinematic)
                this.AdvanceLobbyCinematic(Δτ);
            else if (performance.now() - (this.LobbyIdleSince || 0) > this.LobbyIdleThreshold())
            {
                const FromPose = this.CaptureCurrentCameraPose();
                this.LobbyCinematic = { Shot: 0, Seconds: 0.0, Phase: "Transition", TransitionDuration: 1.6, FromPose };
                this.LobbyCinematicPose = FromPose;
                this.SyncLobbyHudUi();
            }
        }
        if (this.Lobby && this.LobbyEntry && this.Settings.CameraMode === "Orbit")
        {
            const Entry = this.LobbyEntry;
            Entry.Seconds += Δτ;
            Object.assign(this.Orbit, InterpolateOrbitPose(Entry.From, Entry.To, EaseCinematic(Entry.Seconds / Entry.Duration)));
            if (Entry.Seconds >= Entry.Duration)
            {
                this.LobbyEntry = null;
                this.LobbyIdleSince = performance.now();
            }
        }
        if (this.Lobby && !this.LobbyEntry && this.Settings.CameraMode === "Orbit" && this.Orbit)
            this.Orbit.Distance = Math.min(this.Orbit.Distance, Math.max(15.5, (this.LobbyVenue?.CameraBounds?.MaxRadius ?? 18.4) - 2.0));
        if (this.Lobby && !this.LobbyEntry && !this.LobbyCinematic && this.Settings.CameraMode === "Orbit" && this.Orbit && performance.now() > (this.LobbyOrbitHoldUntil || 0))
            this.Orbit.Yaw = (this.Orbit.Yaw + Δτ * 0.06) % (2.0 * Math.PI);
        if (this.Lobby && (this.FrameIndex % 12) === 0)
            this.SyncLobbyHudUi();

        const Camera = this.ResolveCamera(Δτ);
        if (this.Lobby && this.LobbyAudio && this.LobbyRoster)
            this.LobbyAudio.Update(this.LobbyRoster, this.CameraState);
        this.Focus = MultiplyAddVector(Camera.Position, Camera.Forward, 0.45 * this.Grade.ClipResolution * this.Grade.FinestVoxel);
        const Light = this.Atmosphere.ResolveLight(this.Settings.SunElevation, this.Settings.SunAzimuth, this.Settings.SolarIlluminance);
        const Brightness = Light.Illuminance[0] + Light.Illuminance[1] + Light.Illuminance[2];
        if (this.PriorLight)
        {
            const Cosine = Light.Direction[0] * this.PriorLight.Direction[0] + Light.Direction[1] * this.PriorLight.Direction[1]
                         + Light.Direction[2] * this.PriorLight.Direction[2];
            const Ratio = Math.abs(Brightness - this.PriorLight.Brightness) / Math.max(Brightness, this.PriorLight.Brightness, 1.0e-4);
            if (Cosine < Math.cos(10.0 * Math.PI / 180.0) || Ratio > 0.3)
            {
                this.Cards.InvalidateLighting();
                this.PriorValid = false;
            }
        }
        this.PriorLight = { Direction: Light.Direction.slice(), Brightness };
        this.Smoke.Advance(this.FixedStep ? this.FixedStep : Δτ, {
            Enabled: this.Settings.SmokeEnabled, Emissions, Vehicle: this.Vehicle, Light, SkyScale: this.Settings.SkyScale,
            Gas: this.Settings.Gas, Readings, Thermal: this.Thermal });
        this.EncodeFrameRecord(Camera, Light);
        Device.queue.writeBuffer(this.OccupantRecords, 0, this.Scene.EncodeOccupantRecords(this.Volumes, this.Scene.DynamicCount));
        if (CollapseMoved)
            this.UploadCollapseOccupants();
        if (TramsMoved && this.Scene.TramBlock && this.Scene.TramBlock.Count > 0)
        {
            const TramBlock = this.Scene.TramBlock;
            const TramScalars = this.Scene.EncodeOccupantRange(this.Volumes, TramBlock.First, TramBlock.Count);
            Device.queue.writeBuffer(this.OccupantRecords, TramBlock.First * OccupantStride, TramScalars);
        }
        if (ServiceMoved && this.Scene.ServiceBlock && this.Scene.ServiceBlock.Count > 0)
        {
            const ServiceBlock = this.Scene.ServiceBlock;
            const ServiceScalars = this.Scene.EncodeOccupantRange(this.Volumes, ServiceBlock.First, ServiceBlock.Count);
            Device.queue.writeBuffer(this.OccupantRecords, ServiceBlock.First * OccupantStride, ServiceScalars);
        }
        if (TrafficLightsMoved)
            this.UploadTrafficLightOccupants();
        if (DDayMoved)
            this.UploadDDayOccupants();
        if (CandyMoved)
            this.UploadCandyOccupants();
        if (FoundryMoved)
            this.UploadFoundryOccupants();
        if (DemolishMoved)
            this.UploadDemolishOccupants();
        if (LabyrinthMoved)
            this.UploadLabyrinthOccupants();
        if (LobbyMoved)
            this.UploadLobbyOccupants();

        if (this.Lighting && this.LightRecords)
        {
            const ActivePoles = [
                ...this.StreetLampPoles,
                ...(this.TrafficLights ? this.TrafficLights.QueryLightPoles() : []),
                ...(this.DDayCombat ? this.DDayCombat.QueryDynamicLightRecords() : []),
                ...(this.CandyVillage ? this.CandyVillage.QueryDynamicLightRecords() : []),
                ...(this.FoundryConveyor ? this.FoundryConveyor.QueryDynamicLightRecords() : []),
                ...(this.DemolishRun ? this.DemolishRun.QueryDynamicLightRecords() : []),
                ...(this.PistonLabyrinth ? this.PistonLabyrinth.QueryDynamicLightRecords() : [])
            ];
            const LightData = this.Lighting.Update(this.Vehicle.QueryBodyTransform(), ActivePoles, this.Clock);
            Device.queue.writeBuffer(this.LightRecords, 0, LightData);
            Device.queue.writeBuffer(this.ClusterRecords.Cells, 0, this.Lighting.ClusterCells);
            Device.queue.writeBuffer(this.ClusterRecords.Indices, 0, this.Lighting.ClusterIndices);
        }

        // 📝 Refit the triangle hierarchy only while triangle-traced reflections are on: off, the bound buffers keep
        //    their bring-up contents and cost nothing per frame. The vehicle rides along so shredded wheels dim the
        //    response the traversal reads back.
        if (this.RayTracing && this.Settings.TriangleReflection)
            this.RayTracing.UpdateTlas(this.Scene, this.Vehicle);

        // ③ invalidation and relighting schedules; the union's active list follows activity
        if (this.VehicleCardsDirty)
        {
            // 📝 Re-capture SDF Surface Cards when vehicle paint / finish / rims change so SDF views, reflections & GI match!
            this.VehicleCardsDirty = false;
            this.Cards.CaptureCards(this.Scene, this.VertexRecords);
            this.Cards.WarmFrames = Math.max(this.Cards.WarmFrames, 3);
            this.PriorValid = false;
        }
        if (this.Scene.ConstructDistanceList())
            this.Distance.UploadDistanceList(this.Scene.DistanceList);
        this.Distance.ScheduleInvalidation(this.Focus, ResolveMovedBounds(this.Scene, this.Distance.Frozen));
        this.Cards.ScheduleTiles(this.Settings.CardTileBudget);
        this.Shadow.ResolveCascades(Camera, Light.Direction);
        const CpuSceneUpdateMs = performance.now() - CpuUpdateStart;

        // ④ record
        const CpuCommandStart = performance.now();
        const Encoder = Device.createCommandEncoder({ label: `Frame ${this.FrameIndex}` });
        const Included = (Name) => !this.Omitted.has(Name);
        this.Atmosphere.RecordSkyView(Encoder, this.Settings.SolarIlluminance);
        // 📝 Fragments born this frame get their distance volumes before the union reads them.
        this.Volumes.RecordFragments(Encoder);
        if (Included("Union"))
            this.Distance.RecordUnion(Encoder, this.Scene.QueryUnionCount(), this.FrameIndex, this.Metrics.QueryWrites("Distance union"));
        if (Included("Smoke"))
            this.Smoke.RecordTransport(Encoder, this.Distance.TraversalGroup, this.Metrics.QueryWrites("Smoke transport"));
        if (Included("Shadow"))
            this.Shadow.RecordCascades(Encoder, this.Scene, this.VertexRecords, this.Metrics.QueryWrites("Shadow"));
        if (Included("Cards"))
            this.Cards.RecordLighting(Encoder, this.Distance.TraversalGroup, this.SamplingGroup, this.Settings.CardRadiosity,
                                      this.Metrics.QueryWrites("Card lighting"));
        this.Surface.RecordSurface(Encoder, this.Scene, this.VertexRecords, this.Metrics.QueryWrites("Surface"));
        // 📝 The pyramid is skipped with the traces off — no dispatch, no timestamp writes — so the "Depth pyramid"
        //    duration reads zero until the toggle (or ?screentraces=1) asks for it. The bound groups still exist.
        if (this.Settings.ScreenTraversal)
            this.NearestDepth.RecordReduction(Encoder, Parity, this.Metrics.QueryWrites("Depth pyramid"));
        const ScreenGroup = this.NearestDepth.TraversalGroups[Parity];
        if (Included("Cabin"))
            this.Cabin.RecordViews(Encoder, this.Scene, this.VertexRecords, this.Vehicle.QueryBodyTransform(), this.QueryMirrorsVisible());
        if (Included("Direct"))
            this.Surface.RecordDirect(Encoder, this.Distance.TraversalGroup, this.Metrics.QueryWrites("Direct"));
        if (this.Settings.Indirect && Included("Resampling"))
        {
            this.Resampling.RecordResampling(Encoder, Parity, this.Distance.TraversalGroup, this.SamplingGroup, ScreenGroup,
                                             this.Metrics.QueryWrites("ReSTIR GI"));
            if (Included("Diffuse"))
                this.Diffuse.Record(Encoder, Parity, this.Settings.VarianceKernel, this.Metrics.QueryWrites("Diffuse reconstruction"));
            if (this.Settings.Reflection && Included("Reflection"))
            {
                this.Resampling.RecordReflection(Encoder, this.Distance.TraversalGroup, this.SamplingGroup, ScreenGroup,
                                                 this.Metrics.QueryWrites("Reflection"));
                this.Glossy.Record(Encoder, Parity, this.Settings.VarianceKernel, this.Metrics.QueryWrites("Glossy reconstruction"));
            }
        }
        if (this.Settings.Inspection === 6 && Included("Inspection"))
            this.Display.RecordInspection(Encoder, this.Distance.TraversalGroup, this.SamplingGroup);
        if (Included("Smoke") && this.Smoke.Resident)
            this.Smoke.RecordMarch(Encoder, this.Metrics.QueryWrites("Smoke render"));
        if (Included("Resolve"))
        {
            // Slate plume is marched and recombined before TAA, as in the requested reference.
            const Composer = null;
            // 📝 The cabin (glass, screens, mirrors) draws before the antialiasing; sparks after it.
            const Overlay = this.Settings.Inspection === 0 && Included("Cabin")
                          ? (Target, View) => this.Cabin.RecordOverlay(Target, View, this.Surface.Views.Depth) : null;
            const PostOverlay = this.Sparks.Live > 0 && this.Settings.Inspection === 0 && Included("Sparks")
                              ? (Target, View) => this.Sparks.Record(Target, View, this.Surface.Views.Depth) : null;
            this.Display.RecordResolve(Encoder, Parity, this.Metrics.QueryWrites("Resolve"), Composer, Overlay, PostOverlay);
        }
        let CaptureReady = false;
        if (Included("Present"))
        {
            const Target = this.Capture ? this.CaptureView : this.Context.getCurrentTexture().createView({ format: `${this.Exchange.CanvasFormat}-srgb` });
            this.Display.RecordPresent(Encoder, Parity, Target, this.Distance.TraversalGroup);
            if (this.Capture && !this.CapturePending)
            {
                Encoder.copyTextureToBuffer({ texture: this.CaptureTexture }, { buffer: this.CaptureRecords, bytesPerRow: this.CaptureStride },
                                            [this.ScreenExtent[0], this.ScreenExtent[1]]);
                CaptureReady = true;
            }
        }
        const Timed = this.Metrics.RecordResolve(Encoder);
        const CommandBuffer = Encoder.finish();
        const CpuCommandEncodeMs = performance.now() - CpuCommandStart;
        const CpuSubmitStart = performance.now();
        Device.queue.submit([CommandBuffer]);
        const CpuQueueSubmitMs = performance.now() - CpuSubmitStart;
        this.FramesInFlight = (this.FramesInFlight || 0) + 1;
        Device.queue.onSubmittedWorkDone().then(() => { this.FramesInFlight--; });
        if (Timed)
            this.Metrics.CollectDurations();
        if (CaptureReady)
            this.ProjectCapture(this.FrameIndex);
        this.PriorValid = true;

        if (!this.Lobby)
            this.PresentMeasurements(Now, Light);
        this.DrivingHud?.Render(this.CameraState);
        this.RoadEditor?.RenderViewportGizmos?.(this.CameraState);
        const CpuCallbackMs = performance.now() - CpuFrameStart;
        this.RecordPerformanceSample(FrameDeltaMs, FrameDeltaMs * 0.001, CpuCallbackMs, CpuSceneUpdateMs, CpuCommandEncodeMs, CpuQueueSubmitMs);
        this.FrameIndex++;
        requestAnimationFrame((Next) => this.AdvanceFrame(Next));
    }

    // 🔍 Copies the read-back present target into the 2D capture canvas laid over the viewport.
    async ProjectCapture(CapturedFrame)
    {
        this.CapturePending = true;
        const [Width, Height] = this.ScreenExtent;
        const Records = this.CaptureRecords;
        await Records.mapAsync(GPUMapMode.READ);
        if (Records !== this.CaptureRecords)
            return;
        const Source = new Uint8Array(Records.getMappedRange());
        const Pixels = new Uint8ClampedArray(Width * Height * 4);
        for (let Row = 0; Row < Height; ++Row)
            Pixels.set(Source.subarray(Row * this.CaptureStride, Row * this.CaptureStride + Width * 4), Row * Width * 4);
        Records.unmap();
        const Canvas = document.getElementById("capture");
        Canvas.width = Width;
        Canvas.height = Height;
        Canvas.getContext("2d").putImageData(new ImageData(Pixels, Width, Height), 0, 0);
        this.CapturedFrame = CapturedFrame;
        this.CapturePending = false;
    }

    PresentMeasurements(Now, Light)
    {
        const Telemetry = this.Vehicle.QueryTelemetry();
        const Counts = this.Distance.FrameLevelCounts;
        const Rate = (1.0 / Math.max(this.FrameSeconds, 1.0e-4)).toFixed(0);
        const Screen = `${this.ScreenExtent[0]}×${this.ScreenExtent[1]}`;
        const Trace = `${this.TraceExtent[0]}×${this.TraceExtent[1]}`;
        const Speed = (Math.abs(Telemetry.Speed) * 3.6).toFixed(0).padStart(3);
        const Rewritten = Counts.map((Count, Level) => `C${Level}:${String(Count).padStart(4)}`).join("  ");
        // 📝 1 % low: the frame rate the slowest 1 % of the last 240 frames run at — the stutter an average hides.
        const Durations = Array.from(this.FrameDurations).filter((Duration) => Duration > 0.0).sort((Left, Right) => Left - Right);
        const Slowest = Durations.length ? Durations[Math.min(Durations.length - 1, Math.floor(0.99 * Durations.length))] : 0.0;
        this.LowRate = Slowest > 0.0 ? (1000.0 / Slowest).toFixed(0) : "–";
        const Lines = [
            `${Rate} fps · ${(this.FrameSeconds * 1000).toFixed(1)} ms · 1% low ${this.LowRate} fps · ${Screen} · trace ${Trace}`,
            `ControlVehicle  ${Speed} km/h · gear ${Telemetry.Gear} · ${Telemetry.EngineRPM.toFixed(0)} rpm · ${Telemetry.Contacts}/4 contact`
                + (Telemetry.Burnout ? ` · BURNOUT ${(Math.abs(Telemetry.RearSpin) * 3.6).toFixed(0)} km/h wheelspin` : ""),
            `GDF bricks rewritten  ${Rewritten}${this.Distance.Frozen ? "  (frozen)" : ""} · card tiles ${this.Cards.ScheduledTiles}`
        ];
        if (this.TrafficLights)
        {
            const NearSig = this.TrafficLights.QueryNearestSignal(this.Vehicle.Position);
            if (NearSig && NearSig.Distance < 95.0)
            {
                Lines.push(`Intersection signal  ${NearSig.Name} (${NearSig.Distance.toFixed(0)} m) · ${NearSig.Aspect} ${NearSig.RemainingSeconds.toFixed(1)}s`);
            }
        }
        if (this.Stress.QueryActive() > 0)
            Lines.push(`Stress objects  ${this.Stress.QueryActive()} (${this.Stress.Moving.length} moving) · ${this.Scene.DrawRuns.length} draw runs`);
        if (this.Fracture.Metrics.ActiveFragments > 0)
            Lines.push(`Fragments  ${this.Fracture.Metrics.ActiveFragments} live (${this.Fracture.Metrics.Sleeping} asleep, ${this.Fracture.Metrics.InField || 0} in the field) · last cut ${this.Fracture.Metrics.LastMilliseconds.toFixed(1)} ms`);
        if (this.Bodies.Metrics.Awake > 0)
            Lines.push(`Rigid bodies  ${this.Bodies.Metrics.Awake} awake of ${this.Bodies.Metrics.Bodies} · ${this.Bodies.Metrics.Contacts} contacts · ${this.Bodies.Metrics.Milliseconds.toFixed(2)} ms`);
        const Hottest = Math.max(...this.Thermal.Wheels.map((Wheel) => Wheel.Surface));
        if (Hottest > 323.15 || this.Smoke.Resident)
        {
            const Names = ["FL", "FR", "RL", "RR"];
            const Tyres = this.Thermal.Wheels.map((Wheel, Index) => `${Names[Index]} ${(Wheel.Surface - 273.15).toFixed(0)}°C${Wheel.Ignited ? " 🔥" : ""}`);
            Lines.push(`Tyres  ${Tyres.join("  ")}${this.Smoke.Resident ? `  · smoke ${this.Smoke.Metrics.Sources} sources` : ""}`);
        }
        if (this.Metrics.Enabled)
        {
            const Timings = Object.entries(this.Metrics.Durations).filter(([, Milliseconds]) => Milliseconds > 0.0);
            Lines.push(...Timings.map(([Name, Milliseconds]) => `  ${Name.padEnd(24)}${Milliseconds.toFixed(2).padStart(7)} ms`));
        }
        const Pressures = this.Thermal.Wheels.map((Wheel, Index) => {
            const State = this.Vehicle.Wheels[Index].Shredded ? " rim" : Wheel.Burst ? " burst" : Wheel.LeakRate > 0.0 ? " leak" : "";
            return `${["FL", "FR", "RL", "RR"][Index]} ${(Wheel.GaugePressure / 1000.0).toFixed(0)} kPa${State}`; });
        if (Pressures.some((Line) => /burst|rim|leak/.test(Line)) || this.Sparks.Live > 0)
            Lines.push(`Tyres  ${Pressures.join("  ")}${this.Sparks.Live > 0 ? `  · ${this.Sparks.Live} sparks` : ""}`);
        Lines.push("W/S throttle·brake  W+S burnout  A/D steer  Space handbrake  R respawn  T reset arena  F8 free camera  C camera rigs");
        Lines.push("H readout  U AR-HUD  J distance slice  P editor  V clean view");
        this.Shell.Present(Now, Lines);
    }

    QueryFieldLines()
    {
        const Resolution = this.Grade.ClipResolution;
        const Lines = this.Distance.Clips.map((Clip, Level) =>
            `cascade ${Level}: ${Resolution}³ · voxel ${(Clip.VoxelExtent * 100).toFixed(1)} cm · span ${(Clip.VoxelExtent * Resolution).toFixed(0)} m`);
        Lines.push(`distance occupants: ${this.Scene.QueryUnionCount()} of ${this.Scene.DistanceCount} · volumes: ${this.Volumes.Volumes.length}`);
        Lines.push(`bricks rewritten since start: ${this.Distance.TotalBricks}`);
        return Lines;
    }

    QueryDepotLines()
    {
        const Occupied = (this.Cards.OccupiedTexels / 1.0e6).toFixed(2);
        return [`cards: ${this.Cards.Cards.length} · texel space ${this.Grade.CardExtent}² · occupied ${Occupied} M`,
                `tiles: ${this.Cards.DynamicTiles.length} moving (every frame) + ${this.Cards.StaticTiles.length} static`];
    }

    ConstructPerformanceSamples()
    {
        return { frame_delta_ms: [], callback_cpu_ms: [], scene_update_cpu_ms: [], command_encode_cpu_ms: [], queue_submit_cpu_ms: [] };
    }

    RecordPerformanceSample(FrameDeltaMs, DeltaSeconds, CallbackCpuMs, SceneUpdateCpuMs, CommandEncodeCpuMs, QueueSubmitCpuMs)
    {
        const Samples = this.PerformanceWindowSamples;
        Samples.frame_delta_ms.push(FrameDeltaMs);
        Samples.callback_cpu_ms.push(CallbackCpuMs);
        Samples.scene_update_cpu_ms.push(SceneUpdateCpuMs);
        Samples.command_encode_cpu_ms.push(CommandEncodeCpuMs);
        Samples.queue_submit_cpu_ms.push(QueueSubmitCpuMs);
        // 📝 Keep manual captures bounded when automatic logging is off; normal one-second windows reset below.
        for (const Values of Object.values(Samples))
            if (Values.length > 512) Values.shift();
        this.PerformanceWindowSeconds += DeltaSeconds;
        if (!this.PerformanceCapture.Enabled || this.PerformanceWindowSeconds < 1.0)
            return;
        const WindowSeconds = this.PerformanceWindowSeconds;
        const Log = this.ComposeTimingLog(Samples, WindowSeconds);
        console.info(`vehiclehmi_perf_v1 ${JSON.stringify(JSON.parse(Log))}`);
        this.PerformanceLastWindowSamples = Samples;
        this.PerformanceWindowSamples = this.ConstructPerformanceSamples();
        this.PerformanceWindowSeconds = 0.0;
    }

    SetPerformanceLogging(Enabled)
    {
        this.PerformanceCapture.Enabled = !!Enabled;
        this.PerformanceWindowSeconds = 0.0;
        this.PerformanceWindowSamples = this.ConstructPerformanceSamples();
        this.Shell?.Announce(Enabled
            ? "Performance logging on — vehiclehmi_perf_v1 records will appear in the console once per second."
            : "Performance logging paused; copy or download the latest capture whenever you like.");
    }

    ApplyFastPerformanceProfile()
    {
        if (!this.OptimizationRestorePoint)
        {
            const Keys = ["Quality", "RenderScale", "TraceScale", "ScreenTraversal", "Reflection", "TriangleReflection", "CardRadiosity",
                          "DirectShadow", "SmokeEnabled", "SpatialCount", "SpatialRadius", "CardTileBudget", "VarianceKernel", "Antialias"];
            this.OptimizationRestorePoint = {
                GradeName: this.GradeName,
                Grade: { ...this.Grade },
                Settings: Object.fromEntries(Keys.map((Key) => [Key, this.Settings[Key]]))
            };
        }
        this.GradeName = "low";
        this.Grade = { ...QualityGrades.low };
        Object.assign(this.Settings, {
            Quality: 0, RenderScale: QualityGrades.low.RenderScale, TraceScale: 4,
            ScreenTraversal: true, Reflection: false, TriangleReflection: false, CardRadiosity: false,
            DirectShadow: false, SmokeEnabled: false, SpatialCount: 2, SpatialRadius: 8,
            CardTileBudget: QualityGrades.low.TileBudget
        });
        this.PriorValid = false;
        this.ReconstructScreenTargets(true);
        this.Shell?.Controls.Synchronise();
        this.Shell?.Announce("Fast profile applied: low resolution, quarter-resolution tracing, reflections and smoke off. Previous settings are saved.");
    }

    RestorePerformanceProfile()
    {
        if (!this.OptimizationRestorePoint)
        {
            this.Shell?.Announce("No saved settings yet — apply the fast profile first.");
            return;
        }
        this.GradeName = this.OptimizationRestorePoint.GradeName;
        this.Grade = { ...this.OptimizationRestorePoint.Grade };
        Object.assign(this.Settings, this.OptimizationRestorePoint.Settings);
        this.OptimizationRestorePoint = null;
        this.PriorValid = false;
        this.ReconstructScreenTargets(true);
        this.Shell?.Controls.Synchronise();
        this.Shell?.Announce("Saved render settings restored.");
    }

    // 📝 The shareable v1 record keeps main-thread wall time separate from asynchronously resolved GPU pass timestamps.
    ComposeTimingLog(CpuSamples = this.PerformanceLastWindowSamples || this.PerformanceWindowSamples,
                     WindowSeconds = this.PerformanceWindowSeconds)
    {
        const Settings = {
            quality_grade: this.GradeName,
            render_scale: this.Grade.RenderScale,
            trace_scale: this.Settings.TraceScale,
            screen_traversal: this.Settings.ScreenTraversal,
            indirect_lighting: this.Settings.Indirect,
            temporal_reuse: this.Settings.TemporalReuse,
            spatial_reuse: this.Settings.SpatialReuse,
            visibility_reuse: this.Settings.VisibilityReuse,
            reflections: this.Settings.Reflection,
            triangle_reflections: this.Settings.TriangleReflection,
            card_radiosity: this.Settings.CardRadiosity,
            direct_shadows: this.Settings.DirectShadow,
            smoke: this.Settings.SmokeEnabled,
            denoiser: this.Settings.VarianceKernel,
            antialiasing: this.Settings.Antialias,
            camera_motion_blur: this.ActiveCamera.MotionBlur.Enabled,
            camera_motion_blur_strength: this.ActiveCamera.MotionBlur.Strength,
            stress_motion: this.Settings.StressMotion
        };
        return ComposeTimingLogSnapshot({
            GradeName: this.GradeName, ScreenExtent: this.ScreenExtent, TraceScaleUsed: this.TraceScaleUsed,
            TraceExtent: this.TraceExtent, FrameIndex: this.FrameIndex, Settings, FrameSeconds: this.FrameSeconds,
            LowRate: this.LowRate, MetricsEnabled: this.Metrics?.Enabled, PassNames: this.Metrics?.PassNames,
            Durations: this.Metrics?.Durations, LastDurations: this.Metrics?.LastDurations, PassSamples: this.Metrics?.PassSamples,
            CpuSamples, WindowSeconds, LightCount: this.Lighting ? this.Lighting.Lights.length : 0,
            ClusterOverflow: this.Lighting ? this.Lighting.ClusterOverflow : 0,
            ActiveStressObjects: this.Stress ? this.Stress.QueryActive() : 0,
            MovingStressObjects: this.Stress?.Moving?.length || 0,
            DrawRuns: this.Scene?.DrawRuns?.length || 0,
            DistanceBricks: this.Distance?.FrameBricks?.length ? this.Distance.FrameBricks.length / 4 : 0,
            CardTiles: this.Cards?.ScheduledTiles || 0,
            AdapterInfo: this.Exchange?.QueryAdapterDescription() || null,
            UserAgent: navigator.userAgent,
            CaptureEnabled: this.Capture
        });
    }

    // 📝 Clipboard and JSON download provide simple shareable captures; console output remains a fallback.
    async CopyTimingLog()
    {
        const Log = this.ComposeTimingLog();
        console.info(`vehiclehmi_perf_v1 ${Log}`);
        try
        {
            if (!navigator.clipboard || !navigator.clipboard.writeText)
                throw new Error("no clipboard");
            await navigator.clipboard.writeText(Log);
            this.Shell.Announce("JSON performance capture copied — share it here for analysis.");
        }
        catch (Failure)
        {
            this.Shell.Announce("Clipboard blocked — copy vehiclehmi_perf_v1 from the developer console (F12).");
        }
    }

    DownloadTimingLog()
    {
        const Blob_ = new Blob([this.ComposeTimingLog()], { type: "application/json" });
        const Address = URL.createObjectURL(Blob_);
        const Link = document.createElement("a");
        Link.href = Address;
        Link.download = `vehiclehmi-perf-${new Date().toISOString().replace(/[:.]/g, "-")}.json`;
        Link.click();
        setTimeout(() => URL.revokeObjectURL(Address), 1000);
        this.Shell.Announce("Performance JSON downloaded — attach it here for analysis.");
    }

    ReportFailure(Message, Fatal)
    {
        const Panel = document.getElementById("failure");
        Panel.classList.remove("hidden");
        const Line = document.createElement("div");
        Line.textContent = Message;
        Panel.appendChild(Line);
        if (Fatal)
            this.Failed = true;
    }
}

const Host = new IlluminationHost();
Host.ConstructExhibit().catch((Failure) =>
{
    console.error(Failure);
    Host.ReportFailure(String(Failure && Failure.message ? Failure.message : Failure), true);
    if (Host.Lobby)
        Host.RevealLobby();
    const Overlay = document.getElementById("bring-up");
    if (Overlay)
        Overlay.classList.add("hidden");
});
