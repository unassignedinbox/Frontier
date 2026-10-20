//============================================================================================================================================
//                                                            LIGHTINGSYSTEM.JS
//============================================================================================================================================
// 🧩 Two clustered-light systems over one world-space grid. REAL lights are the scene's photometric luminaires — point,
//    spot and IES-profile — driven by the poles, the vehicle and the roof. EMISSIVE lights are the glowing proxies of hot
//    surfaces (the sparks ground off the rims and the body), binned separately so their many short-lived sources never
//    compete with the real lights for a cell's slots.

import { IesProfileSamples, BuildBuiltinIesText, ParseIesProfile } from "./PhotometricProfiles.js";

// 📝 Light kinds as the shader reads them (ClusteredLights.wgsl, PositionType.w). Point is an isotropic omni; Spot is a cone
//    with inner and outer cosines; IES evaluates a measured vertical distribution from the light's own axis.
export const LightType = Object.freeze({
    Omni: 0,          // point: isotropic, inverse square with a reach window
    StreetLamp: 1,    // IES: roadway batwing luminaire on a pole
    LowBeam: 2,       // IES: automotive low beam
    HighBeam: 3,      // spot: collimated automotive high beam
    Fog: 4,           // spot: wide, low fog lamp
    Floodlight: 5,    // spot: roof bar or rally pod flood
    Spot: 6,          // spot: generic cone (work lamps, marshals' lights)
    Ies: 7            // IES: a generic measured luminaire (Profile carries its table)
});

// 📝 The tabulated profiles, parsed once. Each light carries its profile in its own record, so no extra binding is needed.
const ProfileCache = new Map();
export function ResolveIesProfile(Key)
{
    if (!ProfileCache.has(Key))
        ProfileCache.set(Key, ParseIesProfile(BuildBuiltinIesText(Key)).Samples);
    return ProfileCache.get(Key);
}

export const RoofFixtureType = Object.freeze({
    SlimLED: 0,
    RallyPod: 1
});

export const AutomotiveMode = Object.freeze({
    Off: 0,
    LowBeam: 1,
    HighBeam: 2,
    Fog: 3
});

// 📝 Record layout (48 floats = 192 B): PositionType, DirectionReach, ColourIntensity, Params, then the IES table.
//    Params: x = inner cos, y = outer cos (spots), z = unused, w = active. Profile holds IesProfileSamples values.
export const MaxSceneLights      = 32;                  // real luminaires; records 0 … 31
export const MaxEmissiveLights   = 64;                  // emissive proxies;  records 32 … 95
export const TotalLightRecords   = MaxSceneLights + MaxEmissiveLights;
export const LightRecordFloats   = 16 + IesProfileSamples;
export const LightBufferBytes    = TotalLightRecords * LightRecordFloats * 4;

// 16x16x4 spatial grid over the proving grounds [-80..180] in X, [-80..80] in Y, [-2..28] in Z.
// Long-reach beams overlap most of it, so the per-cell cap leaves headroom above the pole count of any one cell.
export const ClusterGridBounds = Object.freeze({
    Minimum: [-80.0, -80.0, -2.0],
    Extent:  [260.0, 160.0, 30.0],
    ResX: 16,
    ResY: 16,
    ResZ: 4,
    CellCount: 1024,
    MaxLightsPerCell: 16,          // real lights
    MaxEmissivePerCell: 8          // emissive proxies (short-lived, dense, one cell rarely needs more)
});

// 📝 Cluster assignment buffers. Header rows 0-2 carry the grid (resolution and the real cap; bitcast minimum; bitcast inverse
//    extent). Then two cell tables back to back: the REAL cells at row 3 and the EMISSIVE cells at row 3 + CellCount, each an
//    (offset, count) pair whose offset is absolute into ClusterIndices. The index buffer holds the real slices first, then the
//    emissive ones. ClusteredLights.wgsl reads the same constants, so WGSL never duplicates these numbers.
export const ClusterHeaderCells   = 3;
export const RealCellBase         = ClusterHeaderCells;
export const EmissiveCellBase     = ClusterHeaderCells + ClusterGridBounds.CellCount;
export const ClusterCellBytes     = (ClusterHeaderCells + 2 * ClusterGridBounds.CellCount) * 16;
export const ClusterIndexBytes    = ClusterGridBounds.CellCount * (ClusterGridBounds.MaxLightsPerCell + ClusterGridBounds.MaxEmissivePerCell) * 4;

export class LightingSystem {
    constructor() {
        this.Lights = [];
        this.EmissiveLights = [];
        this.VehicleLightMode = AutomotiveMode.LowBeam;
        this.RoofFixture = RoofFixtureType.SlimLED;
        this.RoofLightEnabled = false;
        this.BufferData = new Float32Array(TotalLightRecords * LightRecordFloats);
        this.ClusterCells = new Uint32Array((ClusterHeaderCells + 2 * ClusterGridBounds.CellCount) * 4);
        this.ClusterCellFloats = new Float32Array(this.ClusterCells.buffer);
        this.ClusterIndices = new Uint32Array(ClusterGridBounds.CellCount * (ClusterGridBounds.MaxLightsPerCell + ClusterGridBounds.MaxEmissivePerCell));
        this.ClusterOverflow = 0;
        this.EmissiveOverflow = 0;
    }

    SetVehicleMode(Mode) {
        this.VehicleLightMode = Mode;
    }

    CycleVehicleMode() {
        this.VehicleLightMode = (this.VehicleLightMode + 1) % 4;
        return this.VehicleLightMode;
    }

    ToggleRoofLight() {
        this.RoofLightEnabled = !this.RoofLightEnabled;
        return this.RoofLightEnabled;
    }

    SetRoofFixture(Type) {
        this.RoofFixture = Type;
    }

    // 📝 Real luminaires come from the poles and the vehicle; emissive proxies come from Emitters (see EmissiveSources.js).
    Update(VehicleTransform, Poles = [], Emitters = []) {
        this.Lights = [];

        // 1. Street lamps: IES roadway batwing, heads at ~10.8 m aimed down (-Z). The pole height is the lamp's own.
        for (const Pole of Poles) {
            if (this.Lights.length >= MaxSceneLights - 4) break;
            this.Lights.push({
                Type: LightType.StreetLamp,
                Position: [Pole.X, Pole.Y, Pole.Z || 10.8],
                Direction: [0.0, 0.0, -1.0],
                Colour: [1.0, 0.88, 0.65], // warm 3000 K LED
                Intensity: 45.0,
                Reach: 32.0,
                Params: [0.0, 0.0, 0.0, 1.0],
                Profile: ResolveIesProfile("RoadwayBatwing")
            });
        }

        // 2. Automotive lights attached to the ControlVehicle.
        if (VehicleTransform) {
            // Forward is +X, Left is +Y, Up is +Z in vehicle local space
            const M = VehicleTransform;
            const Fwd = [M[0], M[1], M[2]];
            const Left = [M[4], M[5], M[6]];
            const Up = [M[8], M[9], M[10]];
            const Pos = [M[12], M[13], M[14]];

            const TransformLocal = (Offset) => [
                Pos[0] + Fwd[0] * Offset[0] + Left[0] * Offset[1] + Up[0] * Offset[2],
                Pos[1] + Fwd[1] * Offset[0] + Left[1] * Offset[1] + Up[1] * Offset[2],
                Pos[2] + Fwd[2] * Offset[0] + Left[2] * Offset[1] + Up[2] * Offset[2]
            ];

            // Headlight housings on front bumper: X ≈ 2.2m, Y ≈ ±0.65m, Z ≈ 0.20m
            const HeadlightL = TransformLocal([2.2, 0.65, 0.20]);
            const HeadlightR = TransformLocal([2.2, -0.65, 0.20]);

            if (this.VehicleLightMode === AutomotiveMode.LowBeam) {
                // Low beam: IES automotive profile, ~60 m reach
                const LowBeam = (P) => ({
                    Type: LightType.LowBeam,
                    Position: P,
                    Direction: Fwd,
                    Colour: [0.92, 0.96, 1.0], // 6000 K crisp white LED
                    Intensity: 35.0,
                    Reach: 60.0,
                    Params: [0.0, 0.0, 0.0, 1.0],
                    Profile: ResolveIesProfile("AutomotiveLowBeam")
                });
                this.Lights.push(LowBeam(HeadlightL), LowBeam(HeadlightR));
            } else if (this.VehicleLightMode === AutomotiveMode.HighBeam) {
                // High beam: collimated spot, ~130 m reach
                const HighBeam = (P) => ({
                    Type: LightType.HighBeam,
                    Position: P,
                    Direction: Fwd,
                    Colour: [0.95, 0.98, 1.0],
                    Intensity: 75.0,
                    Reach: 130.0,
                    Params: [Math.cos(0.12), Math.cos(0.45), 0.0, 1.0],
                    Profile: ResolveIesProfile("AutomotiveHighBeam")
                });
                this.Lights.push(HighBeam(HeadlightL), HighBeam(HeadlightR));
            } else if (this.VehicleLightMode === AutomotiveMode.Fog) {
                // Fog lamp: low, wide spot, ~45 m reach
                const FogL = TransformLocal([2.15, 0.70, 0.05]);
                const FogR = TransformLocal([2.15, -0.70, 0.05]);
                const FogBeam = (P) => ({
                    Type: LightType.Fog,
                    Position: P,
                    Direction: Fwd,
                    Colour: [1.0, 0.90, 0.50], // 3000 K selective yellow fog
                    Intensity: 40.0,
                    Reach: 45.0,
                    Params: [Math.cos(0.75), Math.cos(1.2), 0.0, 1.0],
                    Profile: ResolveIesProfile("FogLamp")
                });
                this.Lights.push(FogBeam(FogL), FogBeam(FogR));
            }

            // 2b. Tail lamps: point (omni) luminaires at the rear bumper, lit whenever the head lamps are.
            if (this.VehicleLightMode !== AutomotiveMode.Off) {
                const Tail = (P) => ({
                    Type: LightType.Omni,
                    Position: P,
                    Direction: [-Fwd[0], -Fwd[1], -Fwd[2]],
                    Colour: [1.0, 0.08, 0.04],  // red LED
                    Intensity: 3.0,
                    Reach: 8.0,
                    Params: [0.0, 0.0, 0.0, 1.0],
                    Profile: null
                });
                this.Lights.push(Tail(TransformLocal([-2.15, 0.62, 0.45])), Tail(TransformLocal([-2.15, -0.62, 0.45])));
            }

            // 3. Roof floodlight component (Slim LED lightbar or rally pods): spots with the roof flood profile.
            if (this.RoofLightEnabled) {
                const RoofPos = TransformLocal([0.25, 0.0, 0.92]);
                if (this.RoofFixture === RoofFixtureType.SlimLED) {
                    this.Lights.push({
                        Type: LightType.Floodlight,
                        Position: RoofPos,
                        Direction: Fwd,
                        Colour: [0.90, 0.95, 1.0], // 6500 K bright LED bar
                        Intensity: 85.0,
                        Reach: 100.0,
                        Params: [Math.cos(30 * Math.PI / 180), Math.cos(45 * Math.PI / 180), 0.0, 1.0],
                        Profile: ResolveIesProfile("RoofFlood")
                    });
                } else {
                    const PodL = TransformLocal([0.25, 0.28, 0.92]);
                    const PodR = TransformLocal([0.25, -0.28, 0.92]);
                    const RallyBeam = (P) => ({
                        Type: LightType.Floodlight,
                        Position: P,
                        Direction: Fwd,
                        Colour: [1.0, 0.98, 0.92],
                        Intensity: 90.0,
                        Reach: 120.0,
                        Params: [Math.cos(22 * Math.PI / 180), Math.cos(38 * Math.PI / 180), 0.0, 1.0],
                        Profile: ResolveIesProfile("RoofFlood")
                    });
                    this.Lights.push(RallyBeam(PodL), RallyBeam(PodR));
                }
            }
        }

        // Emissive proxies: the hot-surface emitters, kept apart from the real luminaires.
        this.EmissiveLights = Emitters.slice(0, MaxEmissiveLights);

        this.PackRecords();
        this.AssignClusters();
        return this.BufferData;
    }

    // 📝 Real records fill 0…31, emissive records 32…95; every record is written so no stale profile survives a light change.
    PackRecords() {
        this.BufferData.fill(0);
        const Write = (Record, Index) => {
            const Offset = Index * LightRecordFloats;
            const B = this.BufferData;
            B[Offset + 0] = Record.Position[0]; B[Offset + 1] = Record.Position[1]; B[Offset + 2] = Record.Position[2]; B[Offset + 3] = Record.Type;
            B[Offset + 4] = Record.Direction[0]; B[Offset + 5] = Record.Direction[1]; B[Offset + 6] = Record.Direction[2]; B[Offset + 7] = Record.Reach;
            B[Offset + 8] = Record.Colour[0]; B[Offset + 9] = Record.Colour[1]; B[Offset + 10] = Record.Colour[2]; B[Offset + 11] = Record.Intensity;
            B[Offset + 12] = Record.Params[0]; B[Offset + 13] = Record.Params[1]; B[Offset + 14] = Record.Params[2]; B[Offset + 15] = Record.Params[3];
            if (Record.Profile) B.set(Record.Profile, Offset + 16);
        };
        for (let Light = 0; Light < Math.min(this.Lights.length, MaxSceneLights); Light++)
            Write(this.Lights[Light], Light);
        for (let Emitter = 0; Emitter < this.EmissiveLights.length; Emitter++)
            Write(this.EmissiveLights[Emitter], MaxSceneLights + Emitter);
    }

    // 📝 Bins both systems into the grid. A light overlaps each cell its reach box touches — a conservative over-approximation
    //    for beams, exact for omnis. Slices are fixed-stride; a cell past its cap keeps its first lights and counts the rest in
    //    the overflow counters rather than failing.
    AssignClusters()
    {
        const Grid = ClusterGridBounds;
        const ResX = Grid.ResX, ResY = Grid.ResY, ResZ = Grid.ResZ;
        const Min = Grid.Minimum;
        const InvX = 1.0 / Grid.Extent[0], InvY = 1.0 / Grid.Extent[1], InvZ = 1.0 / Grid.Extent[2];
        const Cells = this.ClusterCells, Floats = this.ClusterCellFloats, Indices = this.ClusterIndices;
        Cells[0] = ResX; Cells[1] = ResY; Cells[2] = ResZ; Cells[3] = Grid.MaxLightsPerCell;
        Floats[4] = Min[0]; Floats[5] = Min[1]; Floats[6] = Min[2]; Floats[7] = 0.0;
        Floats[8] = InvX; Floats[9] = InvY; Floats[10] = InvZ; Floats[11] = 0.0;
        Cells.fill(0, ClusterHeaderCells * 4);
        this.ClusterOverflow = 0;
        this.EmissiveOverflow = 0;
        const ClampAxis = (Centre, Reach, Origin, Inverse, Res) =>
        {
            const Low = Math.floor((Centre - Reach - Origin) * Inverse * Res);
            const High = Math.floor((Centre + Reach - Origin) * Inverse * Res);
            return [Math.max(0, Low), Math.min(Res - 1, High)];
        };
        const Bin = (Records, FirstRecord, CellBase, IndexBase, Cap, Counter) =>
        {
            for (let Index = 0; Index < Records.length; ++Index)
            {
                const Record = Records[Index];
                if (Record.Reach <= 0.0)
                    continue;
                const [MinX, MaxX] = ClampAxis(Record.Position[0], Record.Reach, Min[0], InvX, ResX);
                const [MinY, MaxY] = ClampAxis(Record.Position[1], Record.Reach, Min[1], InvY, ResY);
                const [MinZ, MaxZ] = ClampAxis(Record.Position[2], Record.Reach, Min[2], InvZ, ResZ);
                if (MinX > MaxX || MinY > MaxY || MinZ > MaxZ)
                    continue;
                for (let Cz = MinZ; Cz <= MaxZ; ++Cz)
                    for (let Cy = MinY; Cy <= MaxY; ++Cy)
                        for (let Cx = MinX; Cx <= MaxX; ++Cx)
                        {
                            const Cell = (Cz * ResY + Cy) * ResX + Cx;
                            const Base = (CellBase + Cell) * 4;
                            const Count = Cells[Base + 1];
                            if (Count >= Cap)
                            {
                                Counter.Overflow++;
                                continue;
                            }
                            if (Count === 0)
                                Cells[Base] = IndexBase + Cell * Cap;
                            Indices[IndexBase + Cell * Cap + Count] = FirstRecord + Index;
                            Cells[Base + 1] = Count + 1;
                        }
            }
        };
        const RealCounter = { Overflow: 0 }, EmissiveCounter = { Overflow: 0 };
        Bin(this.Lights, 0, RealCellBase, 0, Grid.MaxLightsPerCell, RealCounter);
        Bin(this.EmissiveLights, MaxSceneLights, EmissiveCellBase, Grid.CellCount * Grid.MaxLightsPerCell,
            Grid.MaxEmissivePerCell, EmissiveCounter);
        this.ClusterOverflow = RealCounter.Overflow;
        this.EmissiveOverflow = EmissiveCounter.Overflow;
    }
}

// Construct polygonal facets for vehicle roof auxiliary fixtures
import { EmitBoxFacets } from "./CourseStructure.js";
import { CourseMaterial } from "./MaterialSpecification.js";
import { ResolveCoMAuthoredZ } from "./VehicleSpecification.js";

export function ConstructRoofFixtureFacets(Type) {
    const Result = [];
    const Rebase = ResolveCoMAuthoredZ();
    const Box = (Centre, Size, Family) => {
        for (const F of EmitBoxFacets(...Size, Family)) {
            Result.push({
                Family,
                Corners: F.Corners.map(P => [
                    P[0] + Centre[0],
                    P[1] + Centre[1],
                    P[2] + Centre[2] - Rebase
                ])
            });
        }
    };

    if (Type === RoofFixtureType.SlimLED) {
        // Slim low-profile multi-lens LED lightbar
        // Mounting brackets on roof edge
        Box([0.25, 0.44, 1.35], [0.10, 0.04, 0.05], CourseMaterial.SteelDark);
        Box([0.25, -0.44, 1.35], [0.10, 0.04, 0.05], CourseMaterial.SteelDark);
        // Black extruded aluminium housing
        Box([0.25, 0.0, 1.38], [0.12, 0.94, 0.07], CourseMaterial.VehiclePlastic);
        // Optical poly-lens emitter face
        Box([0.31, 0.0, 1.38], [0.02, 0.88, 0.05], CourseMaterial.Headlight);
    } else {
        // Dual heavy-duty rally auxiliary light pods
        for (const Y of [0.32, -0.32]) {
            // Steel stanchion mount
            Box([0.25, Y, 1.35], [0.12, 0.08, 0.05], CourseMaterial.SteelDark);
            // Deep cylindrical/box rally housing
            Box([0.25, Y, 1.40], [0.16, 0.20, 0.16], CourseMaterial.VehiclePlastic);
            // Large circular optic lens face
            Box([0.33, Y, 1.40], [0.03, 0.16, 0.14], CourseMaterial.Headlight);
        }
    }
    return Result;
}
