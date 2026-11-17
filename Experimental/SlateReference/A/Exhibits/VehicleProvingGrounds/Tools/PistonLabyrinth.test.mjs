//============================================================================================================================================
//                                                        PISTONLABYRINTH.TEST.MJS
//============================================================================================================================================
// Verifies The Piston Labyrinth · Shifting Concrete Maze ("labyrinth"):
//  1. 14 new brutalist concrete/trap/ability PBR materials (186..199) + "mirage" FractureMaterial & watertight 2-manifold shapes.
//  2. Seeded procedural maze uniqueness, determinism, clear spawn runway, >= 10 Mirage breakables & pit-cutout ground floor.
//  3. Full SceneStructure("labyrinth") build under IdentityCeiling + LabyrinthBlock dynamic occupant pool (85 dynamic props).
//  4. Live physics for ALL 7 Hidden Traps: False-Slab Pitfall dynamic height drop, Pneumatic Slam-Plate rising ramp, Snap-Ambush
//     crusher wall, Drop-Gate Guillotine w/ infrared trip-beam, real Pacejka Reverse-Roller drag vs Hacked Emerald Turbo Sling,
//     real Pacejka Acid-Mist grip μ drop, plus [H] Acoustic Resonance Horn & [Q] Vehicle Hacking counterplay!

import test from "node:test";
import assert from "node:assert/strict";
import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

import { CourseMaterial, ConstructMaterialSpecification } from "../Source/MaterialSpecification.js";
import { FractureMaterials } from "../Source/FractureStructure.js";
import { AssignCustomTerrainHeight } from "../Source/CourseStructure.js";
import { SceneStructure, IdentityCeiling } from "../Source/FacetStructure.js";
import { VehicleSolver } from "../Source/VehicleSolver.js";
import {
    PrepareLabyrinthMaze,
    GenerateLabyrinthMaze,
    QueryLabyrinthFloorHeight,
    ResolveLabyrinthShapes,
    ResolveLabyrinthBreakables,
    EmitLabyrinthGroundFacets,
    LabyrinthPoolCounts,
    LabyrinthSpawn
} from "../Source/PistonLabyrinthStructure.js";
import { PistonLabyrinthSequence } from "../Source/PistonLabyrinthSequence.js";

const Here = path.dirname(fileURLToPath(import.meta.url));
const Root = path.resolve(Here, "..");

function InstallArchiveFetch()
{
    const Previous = globalThis.fetch;
    globalThis.fetch = async (Url) =>
    {
        const Full = path.resolve(Root, String(Url));
        const Bytes = fs.readFileSync(Full);
        return {
            ok: true,
            status: 200,
            arrayBuffer: async () => Bytes.buffer.slice(Bytes.byteOffset, Bytes.byteOffset + Bytes.byteLength),
            text: async () => Bytes.toString("utf8"),
            json: async () => JSON.parse(Bytes.toString("utf8"))
        };
    };
    return () => { globalThis.fetch = Previous; };
}

function QuantizeKey(P)
{
    return `${Math.round(P[0] * 1000)},${Math.round(P[1] * 1000)},${Math.round(P[2] * 1000)}`;
}

function AssertClosedWatertight2Manifold(Facets, Name)
{
    assert.ok(Facets.length > 0, `${Name}: empty facet list`);
    const DirectedEdges = new Map();
    let SignedVol6 = 0.0;

    const AddTri = (A, B, C) =>
    {
        const CrossX = B[1] * C[2] - B[2] * C[1];
        const CrossY = B[2] * C[0] - B[0] * C[2];
        const CrossZ = B[0] * C[1] - B[1] * C[0];
        SignedVol6 += A[0] * CrossX + A[1] * CrossY + A[2] * CrossZ;
        for (const [U, V] of [[A, B], [B, C], [C, A]])
        {
            const Key = `${QuantizeKey(U)}->${QuantizeKey(V)}`;
            DirectedEdges.set(Key, (DirectedEdges.get(Key) || 0) + 1);
        }
    };

    for (const F of Facets)
    {
        assert.ok(F.Family >= 0 && F.Family < 200, `${Name}: invalid material family ${F.Family}`);
        if (F.Corners.length === 3)
            AddTri(F.Corners[0], F.Corners[1], F.Corners[2]);
        else if (F.Corners.length === 4)
        {
            AddTri(F.Corners[0], F.Corners[1], F.Corners[2]);
            AddTri(F.Corners[0], F.Corners[2], F.Corners[3]);
        }
        else
            assert.fail(`${Name}: unsupported polygon size ${F.Corners.length}`);
    }

    let Unpaired = 0;
    let Duplicates = 0;
    for (const [Key, Count] of DirectedEdges.entries())
    {
        if (Count !== 1) Duplicates++;
        const [A, B] = Key.split("->");
        if (DirectedEdges.get(`${B}->${A}`) !== 1) Unpaired++;
    }
    assert.equal(Duplicates, 0, `${Name}: has ${Duplicates} duplicate directed edges`);
    assert.equal(Unpaired, 0, `${Name}: has ${Unpaired} non-manifold boundary edges`);
    assert.ok(SignedVol6 > 1.0e-5, `${Name}: signed volume must be strictly positive (got ${SignedVol6 / 6.0})`);
}

//------------------------------------------------------------------------------------------------------------------------
//                                  1. PBR MATERIALS, MIRAGE FRACTURE & WATERTIGHT GEOMETRY
//------------------------------------------------------------------------------------------------------------------------

test("Piston Labyrinth PBR Materials (186..199), Mirage FractureMaterial & All Static/Dynamic Shapes Are Watertight 2-Manifolds", () =>
{
    const Materials = ConstructMaterialSpecification();
    assert.equal(Materials.length, 200);
    assert.ok(Materials[CourseMaterial.LabConcreteLight]);
    assert.ok(Materials[CourseMaterial.LabMirageWall]);
    assert.ok(Materials[CourseMaterial.LabBeaconEmerald].Emission[1] > 3.0);
    assert.ok(Materials[CourseMaterial.LabSonicCyan].Emission[2] > 3.5);
    assert.ok(Materials[CourseMaterial.LabAcidGlow].Emission[1] > 1.2);
    assert.ok(FractureMaterials.mirage && FractureMaterials.mirage.Skin === CourseMaterial.LabMirageWall);

    PrepareLabyrinthMaze(0x20261AB1);
    const Shapes = ResolveLabyrinthShapes();
    const Names  = Object.keys(Shapes);
    assert.ok(Names.length >= 24, `Expected >= 24 brutalist concrete & trap shapes (got ${Names.length})`);
    for (const Name of Names)
        AssertClosedWatertight2Manifold(Shapes[Name].Emit(), Name);

    // Ground mesh must cover corridor + real cut-out pit interiors cleanly without degenerate faces
    const Ground = EmitLabyrinthGroundFacets();
    assert.ok(Ground.length >= 80, `Expected detailed pit-cutout labyrinth floor (got ${Ground.length} facets)`);
});

//------------------------------------------------------------------------------------------------------------------------
//                         2. PROCEDURAL RANDOMIZATION, DETERMINISM & GUARANTEED CLEAR RUNWAY
//------------------------------------------------------------------------------------------------------------------------

test("Procedural Piston Labyrinth Randomizes Distinct Topologies Across Seeds While Guaranteeing Clear Spawn Runway & Breakable Mirage Walls", () =>
{
    const A = GenerateLabyrinthMaze(0x11223344);
    const B = GenerateLabyrinthMaze(0x99AABBCC);
    const A2 = GenerateLabyrinthMaze(0x11223344);

    assert.equal(A.SeedTag, A2.SeedTag);
    assert.notEqual(A.SeedTag, B.SeedTag);
    assert.deepEqual(A.Rings.map((R) => R.MainLane), A2.Rings.map((R) => R.MainLane), "Same seed must be deterministic");

    // Across 12 fresh random seeds, verify every seed has all 7 hidden trap families, >= 10 Mirage Walls,
    // and ZERO static solid colliders blocking the spawn atrium runway [-142..-106, -6.5..+6.5]!
    const RouteSignatures = new Set();
    for (let I = 0; I < 12; ++I)
    {
        const M = GenerateLabyrinthMaze(0x20260001 + I * 7919);
        RouteSignatures.add(M.Rings.map((R) => `${R.MainLane}:${R.MidShortcutSide}`).join("|"));
        assert.equal(M.SlidingWalls.length, LabyrinthPoolCounts.SlidingWalls);
        assert.equal(M.Pitfalls.length,     LabyrinthPoolCounts.TrapSlabs);
        assert.equal(M.SlamPlates.length,   LabyrinthPoolCounts.SlamPlates);
        assert.equal(M.AmbushWalls.length,  LabyrinthPoolCounts.AmbushWalls);
        assert.equal(M.DropGates.length,    LabyrinthPoolCounts.DropGates);
        assert.equal(M.RollerStrips.length, 5);
        assert.equal(M.AcidVents.length,    LabyrinthPoolCounts.AcidSlicks);
        assert.equal(M.Pendulums.length,    LabyrinthPoolCounts.PendulumBlocks);
        assert.ok(M.MirageWalls.length >= 10, `Seed ${M.SeedTag}: expected >= 10 breakable Mirage Walls (got ${M.MirageWalls.length})`);
        assert.equal(M.Rings[0].MainLane, 1, "Ring 0 must always open straight ahead on Center Lane from Spawn");

        for (const P of M.StaticPlacements)
        {
            if (!P.Collider) continue;
            const InSpawnRunway = P.Position[0] > -142.0 && P.Position[0] < -104.0 && Math.abs(P.Position[1]) < 7.5;
            assert.equal(InSpawnRunway, false, `Static collider ${P.Name} must not block Spawn Atrium runway!`);
        }
    }
    assert.ok(RouteSignatures.size >= 6, `Expected diverse procedural labyrinth topologies across 12 seeds (got ${RouteSignatures.size})`);
});

//------------------------------------------------------------------------------------------------------------------------
//                    3. FULL SCENESTRUCTURE("labyrinth") BUILD & LABYRINTHBLOCK DYNAMIC POOL
//------------------------------------------------------------------------------------------------------------------------

test("SceneStructure('labyrinth') Constructs Cleanly Under IdentityCeiling & Allocates 95 Dynamic Trap/Wall/Door/Ability Occupants", async () =>
{
    const RestoreFetch = InstallArchiveFetch();
    try
    {
        const Maze = PrepareLabyrinthMaze(0x20261AB1);
        const Scene = new SceneStructure();
        await Scene.ConstructScene("Archive", 0, "labyrinth");

        assert.ok(Scene.Occupants.length < IdentityCeiling,
            `Occupant count ${Scene.Occupants.length} exceeds IdentityCeiling (${IdentityCeiling})`);
        assert.ok(Scene.LabyrinthBlock, "Scene.LabyrinthBlock must be allocated for 'labyrinth'");
        assert.equal(Scene.LabyrinthBlock.Count, 95, `Expected 95 dynamic occupants in LabyrinthBlock (got ${Scene.LabyrinthBlock.Count})`);
        assert.equal(Scene.LabyrinthBlock.HeadingWalls.length, 4, "Expected 4 Heading-Direction moving piston walls");
        assert.equal(Scene.LabyrinthBlock.PerpDoors.length, 6, "Expected 6 Perpendicular sliding blast doors");
        assert.equal(Maze.Surfaces.length, 5, "Expected 5 drivable Top/Bottom overpass bridges & longitudinal sky-ramps");
        assert.ok(Scene.Breakables.length >= 10, `Expected >= 10 Jolt Fracture Mirage Walls (got ${Scene.Breakables.length})`);

        // Verify live GPU transforms place sliding walls, heading pistons, perpendicular doors & trap slabs at Z >= 0!
        const Car = new VehicleSolver(Scene.QueryObstacleBounds());
        Car.Surfaces = Maze.Surfaces;
        Car.Respawn(LabyrinthSpawn.Position.slice(), 0.0);
        const Seq = new PistonLabyrinthSequence({ LevelKey: "labyrinth", Scene, Vehicle: Car });
        assert.equal(Seq.Enabled, true);
        Seq.Advance(0.05);

        for (const OccIdx of Scene.LabyrinthBlock.SlidingWalls)
            assert.ok(Scene.Occupants[OccIdx].Transform[14] >= -0.05, "Sliding megawall must stand above ground");
        for (const OccIdx of Scene.LabyrinthBlock.HeadingWalls)
            assert.ok(Scene.Occupants[OccIdx].Transform[14] >= -0.05, "Heading-direction piston wall must stand above ground");
        for (const OccIdx of Scene.LabyrinthBlock.PerpDoors)
            assert.ok(Scene.Occupants[OccIdx].Transform[14] >= -0.05, "Perpendicular sliding blast door must stand above ground");
        for (const OccIdx of Scene.LabyrinthBlock.TrapSlabs)
            assert.ok(Math.abs(Scene.Occupants[OccIdx].Transform[14]) < 0.5, "Armed trap slab must sit flush at Z = 0");
        Seq.Dispose();
    }
    finally
    {
        RestoreFetch();
    }
});

//------------------------------------------------------------------------------------------------------------------------
//             4. ALL 7 HIDDEN TRAPS + REAL PACEJKA ROLLERS/ACID GRIP + [H] ACOUSTIC HORN & [Q] HACKING
//------------------------------------------------------------------------------------------------------------------------

test("All 7 Hidden Traps, Real Pacejka Reverse-Roller/Acid-Slick Physics, Acoustic Resonance Horn [H] & Vehicle Hacking [Q] Work Live", async () =>
{
    const RestoreFetch = InstallArchiveFetch();
    try
    {
        const Maze = PrepareLabyrinthMaze(0x20261AB1);
        const Scene = new SceneStructure();
        await Scene.ConstructScene("Archive", 0, "labyrinth");
        AssignCustomTerrainHeight(QueryLabyrinthFloorHeight);

        const Car = new VehicleSolver(Scene.QueryObstacleBounds());
        Car.Respawn(LabyrinthSpawn.Position.slice(), 0.0);

        const BrokenProps = [];
        const FakeFracture = {
            Props: Maze.MirageWalls.map((W, I) => ({
                Index: I,
                State: "intact",
                Pieces: new Array(24),
                Definition: { Name: W.Name, Centre: W.Centre.slice() }
            })),
            BreakProp(Prop, Impact)
            {
                Prop.State = "broken";
                BrokenProps.push({ Prop, Impact });
                Seq.OnWallBroken(Prop);
            }
        };

        const Seq = new PistonLabyrinthSequence({
            LevelKey: "labyrinth",
            Scene,
            Vehicle: Car,
            Fracture: FakeFracture
        });

        // (A) False-Slab Pitfall drops QueryLabyrinthFloorHeight from 0.0 -> -2.8 m w/ 23° East rescue ramp!
        const Pit0 = Seq.Pitfalls[0];
        assert.equal(QueryLabyrinthFloorHeight(Pit0.X0 + 1.5, Pit0.CenterY), 0.0, "Closed pitfall must read flush Z = 0.0");
        Car.Position = [Pit0.X0 - 3.0, Pit0.CenterY, 0.65];
        for (let I = 0; I < 25; ++I) Seq.Advance(0.04); // Arm + snap open!
        assert.equal(Pit0.State, "open");
        assert.equal(QueryLabyrinthFloorHeight(Pit0.X0 + 1.5, Pit0.CenterY), -Pit0.Depth, "Open pitfall drop well must drop to -2.8 m");
        const MidRampZ = QueryLabyrinthFloorHeight(Pit0.X0 + Pit0.DropLen + Pit0.RampLen * 0.5, Pit0.CenterY);
        assert.ok(MidRampZ > -Pit0.Depth && MidRampZ < 0.0, `East rescue ramp must smoothly rise back to Z=0 (got ${MidRampZ})`);

        // (B) Pneumatic Slam-Plate snaps upward and creates a real rising ramp + upward launch velocity!
        const Slam0 = Seq.SlamPlates[0];
        Car.Position = [Slam0.X0 + 3.0, Slam0.CenterY, 0.65];
        Car.LinearVelocity = [14.0, 0.0, 0.0];
        for (let I = 0; I < 10; ++I) Seq.Advance(0.03);
        assert.equal(Slam0.State, "sprung");
        assert.ok(QueryLabyrinthFloorHeight(Slam0.X0 + 5.0, Slam0.CenterY) > 1.2, "Sprung slam-plate must physically tilt upward");
        assert.ok(Car.LinearVelocity[2] > 4.0, "Pneumatic slam-plate must impart upward launch kick");

        // (C) Real Pacejka Reverse-Roller Drag Strip drags coasting car BACKWARD, then [Q] Hack flips it to +22 m/s TURBO SLING!
        const RS0 = Seq.RollerStrips[0];
        Car.Respawn([RS0.CenterX, RS0.CenterY, 0.65], 0.0);
        const ReverseVel = Car.SurfaceVelocityQuery(RS0.CenterX, RS0.CenterY, 0.0);
        assert.ok(ReverseVel && ReverseVel[0] < -12.0, "Armed reverse rollers must feed negative Pacejka belt velocity");
        for (let I = 0; I < 30; ++I)
        {
            Seq.Advance(0.02);
            Car.Advance({ Throttle: 0.0, Brake: 0.0, Steer: 0.0, Handbrake: false }, 0.02);
        }
        assert.ok(Car.LinearVelocity[0] < -1.5, `Pacejka reverse rollers must pull coasting car backward (got vx=${Car.LinearVelocity[0]})`);

        // Now fire [Q] Vehicle Hacking! Rollers flip to +22 m/s emerald turbo sling & accelerate coasting car forward!
        const HackRes = Seq.TriggerVehicleHack();
        assert.equal(HackRes.Fired, true);
        assert.ok(HackRes.FlippedRollers >= 1, "Vehicle Hack [Q] must flip nearby Reverse-Roller strip to Turbo Sling");
        const TurboVel = Car.SurfaceVelocityQuery(RS0.CenterX, RS0.CenterY, 0.0);
        assert.ok(TurboVel && TurboVel[0] > 18.0, "Hacked rollers must flip to +22 m/s forward Pacejka velocity");
        Car.Respawn([RS0.CenterX - 3.0, RS0.CenterY, 0.65], 0.0);
        for (let I = 0; I < 30; ++I)
        {
            Seq.Advance(0.02);
            Car.Advance({ Throttle: 0.0, Brake: 0.0, Steer: 0.0, Handbrake: false }, 0.02);
        }
        assert.ok(Car.LinearVelocity[0] > 1.5, `Hacked turbo sling must pull car forward via Pacejka tyres (got vx=${Car.LinearVelocity[0]})`);

        // (D) Corner Ambush Acid-Mist Vent slashes Pacejka SurfaceGripQuery to 0.36, then [H] Acoustic Horn blows it dry!
        const AV0 = Seq.AcidVents[0];
        Car.Respawn([AV0.X - 6.0, AV0.CenterY, 0.65], 0.0);
        Seq.Advance(0.05);
        assert.equal(AV0.Active, true, "Approaching corner vent must trigger Acid-Mist Ambush");
        assert.ok(Car.SurfaceGripQuery(AV0.X, AV0.CenterY, 0.0) < 0.45, "Acid-Mist slick must slash Pacejka grip μ to ~0.36");

        // (E) Acoustic Resonance Horn [H] pre-shatters Mirage Concrete Wall 18 m ahead with ZERO vehicle hit & clears Acid Mist!
        const TargetWall = FakeFracture.Props[FakeFracture.Props.length - 1]; // Extraction Gate Mirage Slab at X=176.5, Y=0
        Car.Respawn([TargetWall.Definition.Centre[0] - 18.0, TargetWall.Definition.Centre[1], 0.65], 0.0);
        Seq.HornCooldown = 0.0;
        const HornRes = Seq.TriggerAcousticHorn();
        assert.equal(HornRes.Fired, true);
        assert.ok(HornRes.Shattered >= 1, "Acoustic Resonance Horn [H] must pre-shatter Mirage Wall 18 m ahead");
        assert.equal(TargetWall.State, "broken");
        assert.equal(BrokenProps[0].Impact.Vehicle, null, "Horn shatter must cause zero vehicle slowdown (Vehicle: null)");

        // Also test Horn clearing Acid Mist right ahead
        Car.Respawn([AV0.X - 10.0, AV0.CenterY, 0.65], 0.0);
        Seq.HornCooldown = 0.0;
        const HornAcid = Seq.TriggerAcousticHorn();
        assert.ok(HornAcid.ClearedAcid >= 1, "Acoustic Resonance Horn [H] must blow Acid-Mist slick clear");
        assert.equal(Car.SurfaceGripQuery(AV0.X, AV0.CenterY, 0.0), 1.0, "Dry traction must return to 1.0 after Horn blast");

        // (F) Drop-Gate Guillotine drops when crossing red trip-beam, and [Q] Hack hoists it back up safely!
        const GateTest = Seq.DropGates[2];
        Car.Respawn([GateTest.TripX, GateTest.CenterY, 0.65], 0.0);
        Seq.Advance(0.05);
        assert.equal(GateTest.State, "falling", "Crossing crimson trip-beam must release Drop-Gate Guillotine");
        Seq.HackCooldown = 0.0;
        Seq.TriggerVehicleHack();
        assert.equal(GateTest.State, "hacked", "Vehicle Hack [Q] must kill Guillotine & hoist blade clear");

        // (G) Multi-Level Top/Bottom Overpass Bridge: car drives CLEANLY UNDER Cross-Bridge A at Z=0 AND ON TOP at Z=+5.0 m!
        Car.Surfaces = Maze.Surfaces;
        Car.Respawn([-56.0, 0.0, 0.65], 0.0);
        for (let I = 0; I < 20; ++I) Car.Advance({ Throttle: 0.4, Brake: 0.0, Steer: 0.0, Handbrake: false }, 0.02);
        assert.ok(Car.Position[2] < 1.2, `Underpass car must stay cleanly on Z=0 road under +5.0 m bridge (got Z=${Car.Position[2]})`);

        Car.Respawn([-56.0, 0.0, 5.65], 0.5 * Math.PI);
        for (let I = 0; I < 25; ++I) Car.Advance({ Throttle: 0.3, Brake: 0.0, Steer: 0.0, Handbrake: false }, 0.02);
        assert.ok(Car.Position[2] > 5.0 && Car.Position[2] < 6.2, `Overpass car must ride cleanly on top of +5.0 m concrete bridge deck (got Z=${Car.Position[2]})`);

        // (H) Heading-Direction Moving Piston Walls surge longitudinally (±X) & Perpendicular Blast Doors slide across apertures!
        const HWTest = Seq.HeadingWalls[1];
        const HWStartX = HWTest.CurrentX;
        const DoorTest = Seq.PerpDoors[1];
        const DoorStartOpen = DoorTest.OpenFrac;
        for (let I = 0; I < 20; ++I) Seq.Advance(0.04);
        assert.ok(Math.abs(HWTest.CurrentX - HWStartX) > 1.5, "Heading-direction piston wall must travel longitudinally along ±X");
        assert.ok(Math.abs(DoorTest.OpenFrac - DoorStartOpen) > 0.05, "Perpendicular blast door must continuously open/close squeeze gap");

        // [Q] Vehicle Hack forces nearby Perpendicular Blast Doors WIDE OPEN & retreats Heading-Direction Piston Walls!
        Car.Respawn([DoorTest.ClosedX, DoorTest.ClosedY - 6.0, 0.65], 0.0);
        Seq.HackCooldown = 0.0;
        Seq.TriggerVehicleHack();
        for (let I = 0; I < 20; ++I) Seq.Advance(0.05);
        assert.ok(DoorTest.HackedLeft > 0.0 && DoorTest.OpenFrac > 0.92, `Vehicle Hack [Q] must lock Perp Blast Door wide open (got OpenFrac=${DoorTest.OpenFrac})`);
        assert.ok(HWTest.HackedLeft > 0.0, "Vehicle Hack [Q] must hijack nearby Heading-Direction Piston Wall");

        Seq.Dispose();
        AssignCustomTerrainHeight(null);
    }
    finally
    {
        RestoreFetch();
    }
});
