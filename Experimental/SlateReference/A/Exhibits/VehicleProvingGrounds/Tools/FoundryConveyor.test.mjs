//============================================================================================================================================
//                                                      FOUNDRYCONVEYOR.TEST.MJS
//============================================================================================================================================
// Verifies every core system of the Assembly Line · Conveyor Gauntlet ("foundry") level:
//   1. Foundry PBR material palette (160..169) & watertight closed 2-manifold geometry on all 19 Foundry shapes
//   2. Variable-speed 3-lane conveyor belt velocity field (slow/fast/counter/express + SpeedMultiplier)
//   3. Full SceneStructure build for "foundry" & FoundryBlock dynamic occupant pool allocation
//   4. REAL VehicleSolver moving-belt tyre contact physics (wheels visibly spin & locked tyres ride belt velocity!)
//   5. GPU Occ.Transform placement above ground (Z >= 0) for all 72 belt slats, 42 cargo objects, machines & 6 MagBots!
//   6. Dual-target Electromagnet Robot attraction (lifts ferromagnetic cargo parts off belt AND pulls player vehicle!)
//   7. Active slowdown hazards (Stamping Presses, Pneumatic Side-Pushers, Rotary Sweepers, Eddy-Brake Scanner) & Finish Gate

import test from "node:test";
import assert from "node:assert/strict";

import { CourseMaterial, ConstructMaterialSpecification, EncodeMaterialRecords } from "../Source/MaterialSpecification.js";
import { AssignCourseReliefEnabled } from "../Source/CourseStructure.js";
import { SceneStructure } from "../Source/FacetStructure.js";
import { ResolveLevelDescriptor } from "../Source/LevelStructure.js";
import { VehicleSolver } from "../Source/VehicleSolver.js";
import {
    FoundryDynamicShapes,
    FoundryGoalX,
    FoundryMagRobotSites,
    FoundryPoolCounts,
    FoundryStaticShapes,
    QueryFoundryConveyorVelocity
} from "../Source/FoundryConveyorStructure.js";
import { FoundryConveyorSequence } from "../Source/FoundryConveyorSequence.js";

function VerifyWatertightClosedMesh(Name, Facets)
{
    assert.ok(Array.isArray(Facets) && Facets.length >= 12, `${Name}: expected >= 12 triangles, got ${Facets?.length}`);
    const EdgeMap = new Map();
    const Quant = (V) => Math.round(V * 2000.0);
    const KeyOf = (P) => `${Quant(P[0])},${Quant(P[1])},${Quant(P[2])}`;

    let SignedVol6 = 0.0;
    for (const F of Facets)
    {
        const [A, B, C] = F.Corners;
        for (const P of [A, B, C])
            for (let K = 0; K < 3; ++K)
                assert.ok(Number.isFinite(P[K]), `${Name}: non-finite vertex coordinate`);
        SignedVol6 += A[0] * (B[1] * C[2] - B[2] * C[1])
                    + A[1] * (B[2] * C[0] - B[0] * C[2])
                    + A[2] * (B[0] * C[1] - B[1] * C[0]);

        const KA = KeyOf(A), KB = KeyOf(B), KC = KeyOf(C);
        for (const [U, V] of [[KA, KB], [KB, KC], [KC, KA]])
        {
            const DirKey = `${U}->${V}`;
            EdgeMap.set(DirKey, (EdgeMap.get(DirKey) || 0) + 1);
        }
    }
    let OpenEdges = 0;
    for (const [Key, Count] of EdgeMap.entries())
    {
        const [U, V] = Key.split("->");
        if ((EdgeMap.get(`${V}->${U}`) || 0) !== Count)
            OpenEdges++;
    }
    assert.equal(OpenEdges, 0, `${Name}: expected watertight closed mesh with 0 open edges, found ${OpenEdges}`);
    assert.ok(SignedVol6 > 0.0, `${Name}: expected outward-oriented positive volume, got ${SignedVol6 / 6.0}`);
}

test("Foundry PBR Materials (160..169) & All Static + Dynamic Shapes Are Closed Watertight 2-Manifolds", () =>
{
    const Materials = ConstructMaterialSpecification();
    assert.equal(Materials.length, 186, "Expected 186 total PBR materials");
    const Encoded = EncodeMaterialRecords(Materials);
    assert.equal(Encoded.length, 186 * 12);

    const PeakEmissive = (Id) => Math.max(...(Materials[Id].Emission || [0, 0, 0]));
    assert.ok(PeakEmissive(CourseMaterial.FoundryMagCyan) > 2.0,    "FoundryMagCyan must glow brightly");
    assert.ok(PeakEmissive(CourseMaterial.FoundryMolten) > 2.0,     "FoundryMolten must glow brightly");
    assert.ok(PeakEmissive(CourseMaterial.FoundryBoostGreen) > 1.5, "FoundryBoostGreen must glow");

    const AllShapes = { ...FoundryStaticShapes, ...FoundryDynamicShapes };
    const ShapeKeys = Object.keys(AllShapes);
    assert.ok(ShapeKeys.length >= 19, `Expected >= 19 Foundry shapes, got ${ShapeKeys.length}`);
    for (const Key of ShapeKeys)
    {
        const Facets = AllShapes[Key].Emit();
        VerifyWatertightClosedMesh(Key, Facets);
    }
});

test("Variable-Speed 3-Lane Conveyor Belt Speed Field Modulates Across Sectors & Multipliers", () =>
{
    // Sector 1: Intake Slow/Fast pulse active right at spawn (-132, 0) and (-85, 0)
    const IntakeVals = [];
    for (let T = 0; T < 12; T += 0.5)
        IntakeVals.push(QueryFoundryConveyorVelocity(-132, 0, T, 1.0).SpeedX);
    const MinIntake = Math.min(...IntakeVals);
    const MaxIntake = Math.max(...IntakeVals);
    assert.ok(MinIntake < -8.0 && MaxIntake > -4.0 && (MaxIntake - MinIntake) > 5.0,
        `Intake belt should oscillate meaningfully fast & slow (got [${MinIntake.toFixed(2)}, ${MaxIntake.toFixed(2)}])`);

    // Edge-to-edge coverage between guardrails (|Y| <= 15.2)
    assert.equal(QueryFoundryConveyorVelocity(-100, 14.5, 0.0, 1.0).OnBelt, true);

    // Sector 4: Turbo Express Center Belt flows FORWARD (+X) while Outer Belts counter-flow (-X)!
    const ExpressCenter = QueryFoundryConveyorVelocity(125, 0.0, 2.0, 1.0);
    const ExpressOuter  = QueryFoundryConveyorVelocity(125, 8.7, 2.0, 1.0);
    assert.equal(ExpressCenter.OnBelt, true);
    assert.ok(ExpressCenter.SpeedX > 6.0, `Express center lane must push forward (+X), got ${ExpressCenter.SpeedX}`);
    assert.ok(ExpressOuter.SpeedX < -4.0, `Express outer lanes must counter-flow (-X), got ${ExpressOuter.SpeedX}`);

    // Sector 5: Final Overdrive Surge counter-flows strongly against the finish line!
    const OverdriveSlow = QueryFoundryConveyorVelocity(205, 0.0, 1.0, 0.65);
    const OverdriveFast = QueryFoundryConveyorVelocity(205, 0.0, 1.0, 1.40);
    assert.ok(OverdriveFast.SpeedX < OverdriveSlow.SpeedX - 4.0, "SpeedMultiplier=1.4 must strengthen belt speed");

    // Off-belt loading dock behind the intake (X = -160) reports OnBelt === false
    assert.equal(QueryFoundryConveyorVelocity(-160, 0, 0, 1.0).OnBelt, false);
});

test("Real VehicleSolver Pure Contact-Patch Conveyor Physics: No Stationary Burnout, Real Belt Transport & Differential Yaw", () =>
{
    AssignCourseReliefEnabled(false);
    try
    {
        // ① Coasting hands-off on -10 m/s belt: tyres spin & ribbed-belt rolling drag carries car cleanly backward (ZERO burnout!)
        const Car = new VehicleSolver([]);
        Car.Respawn([0.0, 0.0, 0.65], 0.0);
        Car.SurfaceVelocityQuery = () => [-10.0, 0.0, 0.0];
        for (let Step = 0; Step < 25; ++Step)
            Car.Advance({ Throttle: 0.0, Brake: 0.0, Steer: 0.0, Handbrake: false }, 0.02);
        assert.ok(Car.Wheels[0].Spin > 10.0, `Front wheel must visibly spin on moving belt, got Spin=${Car.Wheels[0].Spin.toFixed(2)}`);
        assert.ok(Car.LinearVelocity[0] < -1.0, `Coasting car must be carried backward by ribbed belt, got Vx=${Car.LinearVelocity[0].toFixed(2)}`);
        assert.ok(Car.Wheels[2].Skid < 0.20, `Coasting on belt must not falsely burn out, got Skid=${Car.Wheels[2].Skid.toFixed(2)}`);

        // ② Holding brakes on -10 m/s belt locks tyres onto exact belt speed (-10 m/s)
        Car.Respawn([0.0, 0.0, 0.65], 0.0);
        for (let Step = 0; Step < 60; ++Step)
            Car.Advance({ Throttle: 0.0, Brake: 1.0, Steer: 0.0, Handbrake: true }, 0.02);
        assert.ok(Math.abs(Car.LinearVelocity[0] - (-10.0)) < 0.5, `Braked car must ride at exact -10 m/s belt speed, got Vx=${Car.LinearVelocity[0].toFixed(2)}`);

        // ③ Full throttle (W) on -10 m/s counter-belt MUST climb cleanly forward WITHOUT burning out in place!
        Car.Respawn([0.0, 0.0, 0.65], 0.0);
        for (let Step = 0; Step < 150; ++Step)
            Car.Advance({ Throttle: 1.0, Brake: 0.0, Steer: 0.0, Handbrake: false }, 0.02);
        assert.ok(Car.Position[0] > 14.0 && Car.LinearVelocity[0] > 10.0,
            `Full throttle on -10 m/s counter-belt must drive forward cleanly (not burn out in place!), got X=${Car.Position[0].toFixed(2)}, Vx=${Car.LinearVelocity[0].toFixed(2)}`);
        const CounterDist = Car.Position[0];

        // ④ Full throttle (W) on +10 m/s Turbo Express belt goes noticeably farther than on -10 m/s counter-belt!
        Car.Respawn([0.0, 0.0, 0.65], 0.0);
        Car.SurfaceVelocityQuery = () => [10.0, 0.0, 0.0];
        for (let Step = 0; Step < 150; ++Step)
            Car.Advance({ Throttle: 1.0, Brake: 0.0, Steer: 0.0, Handbrake: false }, 0.02);
        assert.ok(Car.Position[0] > CounterDist + 3.0,
            `Turbo Express +10 m/s belt (${Car.Position[0].toFixed(2)}m) must outpace -10 m/s counter-belt (${CounterDist.toFixed(2)}m)`);

        // ⑤ Straddling -8 m/s (left) and +10 m/s (right) lanes generates real Newtonian differential yaw torque!
        Car.Respawn([0.0, 0.0, 0.65], 0.0);
        Car.SurfaceVelocityQuery = (_X, Y) => [Y > 0.0 ? -8.0 : 10.0, 0.0, 0.0];
        for (let Step = 0; Step < 80; ++Step)
            Car.Advance({ Throttle: 0.4, Brake: 0.0, Steer: 0.0, Handbrake: false }, 0.02);
        assert.ok(Math.abs(Car.AngularVelocity[2]) > 0.015,
            `Straddling opposing-speed conveyor lanes must generate real yaw torque, got wz=${Car.AngularVelocity[2].toFixed(4)}`);
    }
    finally
    {
        AssignCourseReliefEnabled(true);
    }
});

test("SceneStructure Constructs Foundry Level Cleanly & Updates GPU Occ.Transform Above Ground", async () =>
{
    const Desc = ResolveLevelDescriptor("foundry");
    assert.equal(Desc.Key, "foundry");
    assert.equal(Desc.Title, "Assembly Line · Conveyor");

    const Fs   = await import("node:fs/promises");
    const Path = await import("node:path");
    const Root = Path.resolve(import.meta.dirname, "..");
    const PrevFetch = globalThis.fetch;
    globalThis.fetch = async (Url) =>
    {
        const Rel   = String(Url).replace(/^\.\//, "").split("?")[0];
        const Bytes = await Fs.readFile(Path.join(Root, Rel));
        return { ok: true, arrayBuffer: async () => Bytes.buffer.slice(Bytes.byteOffset, Bytes.byteOffset + Bytes.byteLength) };
    };
    const Scene = new SceneStructure();
    try
    {
        await Scene.ConstructScene("Archive", 0, "foundry");
    }
    finally
    {
        globalThis.fetch = PrevFetch;
    }
    assert.ok(Scene.FoundryBlock, "Scene.FoundryBlock must be allocated for foundry level");
    const ExpectedTotal = Object.values(FoundryPoolCounts).reduce((A, B) => A + B, 0);
    assert.equal(Scene.FoundryBlock.Count, ExpectedTotal, `Expected ${ExpectedTotal} dynamic Foundry occupants`);
    assert.equal(Scene.FoundryBlock.MagHeads.length, FoundryMagRobotSites.length);

    // Verify FoundryConveyorSequence moves real Scene.Occupants[].Transform ABOVE GROUND (Z >= 0, NEVER stuck at -40!)
    const Car = new VehicleSolver([]);
    Car.Respawn([-132.0, 0.0, 0.65], 0.0);
    const Seq = new FoundryConveyorSequence({ LevelKey: "foundry", Scene, Vehicle: Car });
    Seq.Advance(0.05);

    const CheckVisible = (Ids, Label) =>
    {
        for (const Id of Ids)
        {
            const Occ = Scene.Occupants[Id];
            assert.equal(Occ.Inactive, false, `${Label} occupant ${Id} must be active`);
            assert.ok(Occ.Transform[14] >= -0.1, `${Label} occupant ${Id} must sit above ground (Z >= 0), got Z=${Occ.Transform[14]}`);
        }
    };
    CheckVisible(Scene.FoundryBlock.BeltSlats,   "BeltSlats");
    CheckVisible(Scene.FoundryBlock.CargoCrates, "CargoCrates");
    CheckVisible(Scene.FoundryBlock.CargoDrums,  "CargoDrums");
    CheckVisible(Scene.FoundryBlock.CargoIngots, "CargoIngots");
    CheckVisible(Scene.FoundryBlock.PressRams,   "PressRams");
    CheckVisible(Scene.FoundryBlock.PusherRams,  "PusherRams");
    CheckVisible(Scene.FoundryBlock.SweeperBars, "SweeperBars");
    CheckVisible(Scene.FoundryBlock.MagHeads,    "MagHeads");

    // Verify slats actually translate along X over time!
    const X0 = Scene.Occupants[Scene.FoundryBlock.BeltSlats[0]].Transform[12];
    Seq.Advance(0.05);
    const X1 = Scene.Occupants[Scene.FoundryBlock.BeltSlats[0]].Transform[12];
    assert.notEqual(X0, X1, "Conveyor belt slats must physically scroll along X every tick!");
});

test("Conveyor Physics, Dual-Target Electromagnet Robots (Parts + Car), Hazards & Finish Gate", () =>
{
    const MockOccupants = [];
    const Pool = {};
    let NextIdx = 0;
    const MakeIds = (N) =>
    {
        const Arr = [];
        for (let I = 0; I < N; ++I)
        {
            MockOccupants.push({ Transform: new Float32Array(16), PreviousTransform: new Float32Array(16), Inactive: true });
            Arr.push(NextIdx++);
        }
        return Arr;
    };
    Pool.First        = 0;
    Pool.BeltSlats    = MakeIds(FoundryPoolCounts.BeltSlats);
    Pool.CargoCrates  = MakeIds(FoundryPoolCounts.CargoCrates);
    Pool.CargoDrums   = MakeIds(FoundryPoolCounts.CargoDrums);
    Pool.CargoIngots  = MakeIds(FoundryPoolCounts.CargoIngots);
    Pool.PressRams    = MakeIds(FoundryPoolCounts.PressRams);
    Pool.DangerPads   = MakeIds(FoundryPoolCounts.DangerPads);
    Pool.PusherRams   = MakeIds(FoundryPoolCounts.PusherRams);
    Pool.SweeperBars  = MakeIds(FoundryPoolCounts.SweeperBars);
    Pool.MagBases     = MakeIds(FoundryPoolCounts.MagBases);
    Pool.MagArms      = MakeIds(FoundryPoolCounts.MagArms);
    Pool.MagHeads     = MakeIds(FoundryPoolCounts.MagHeads);
    Pool.MagRings     = MakeIds(FoundryPoolCounts.MagRings);
    Pool.ScannerBeams = MakeIds(FoundryPoolCounts.ScannerBeams);
    Pool.Count        = NextIdx;

    const MockCar = {
        Position:       [205.0, 0.0, 0.55],
        LinearVelocity: [0.0, 0.0, 0.0],
        Throttle:       0.0,
        Respawn(Pos) { this.Position = Pos.slice(); this.LinearVelocity = [0, 0, 0]; }
    };
    const MockHost = {
        LevelKey: "foundry",
        Scene: { Occupants: MockOccupants, FoundryBlock: Pool, ConstructDrawRuns() {} },
        Vehicle: MockCar
    };

    const Seq = new FoundryConveyorSequence(MockHost);
    assert.equal(Seq.Enabled, true);
    assert.equal(typeof MockCar.SurfaceVelocityQuery, "function", "Foundry must attach Vehicle.SurfaceVelocityQuery");

    // ① SurfaceVelocityQuery provides real moving-belt surface velocity (ZERO fake chassis push in Advance!)
    Seq.Advance(0.05);
    assert.equal(MockCar.LinearVelocity[0], 0.0, "Advance() must NOT directly push chassis LinearVelocity for belt physics");
    const OverdriveVel = MockCar.SurfaceVelocityQuery(205.0, 0.0, 0.0);
    assert.ok(OverdriveVel && OverdriveVel[0] < -4.0, `Sector 5 belt surface velocity must be negative, got ${OverdriveVel}`);

    // ② Turbo Express center lane provides positive forward belt surface velocity (+X) in Sector 4
    const ExpressVel = MockCar.SurfaceVelocityQuery(125.0, 0.0, 0.0);
    assert.ok(ExpressVel && ExpressVel[0] > 6.0, `Sector 4 Express center belt surface velocity must be positive, got ${ExpressVel}`);
    MockCar.Position = [125.0, 0.0, 0.55];
    MockCar.LinearVelocity = [0.0, 0.0, 0.0];
    Seq.Advance(0.05);
    assert.ok(Seq.CurrentBelt.SpeedX > 6.0, `CurrentBelt telemetry must reflect Express center lane, got ${Seq.CurrentBelt.SpeedX}`);

    // ③ Dual-Target Electromagnet Robot pulls BOTH ferromagnetic Cargo Parts AND the Player's Vehicle!
    // MagBot-1 sits on the North rail (X = -92, Y = +13.8, ReachY = +5.8) → pulls car in +Y!
    let CarWasPulled   = false;
    let PartWasLifted  = false;
    let PartWasDropped = false;
    const TestCrate = Seq.CargoItems.find((C) => C.Magnetic);
    TestCrate.Pos = [-92.0, 6.2, 0.02];

    for (let Step = 0; Step < 220; ++Step)
    {
        MockCar.Position = [-92.0, 1.0, 0.55];
        MockCar.LinearVelocity = [0.0, 0.0, 0.0];
        Seq.Advance(0.04);
        if (Seq.ActiveMagnetPull > 4.0 && MockCar.LinearVelocity[1] > 0.15)
            CarWasPulled = true;
        if ((TestCrate.State === "attracted" || TestCrate.State === "captured") && TestCrate.Pos[2] > 1.2)
            PartWasLifted = true;
        if (PartWasLifted && TestCrate.State === "falling")
            PartWasDropped = true;
    }
    assert.ok(CarWasPulled,   "Energized Electromagnet Robot must physically pull player vehicle toward its head (+Y)");
    assert.ok(PartWasLifted,  "Energized Electromagnet Robot must lift ferromagnetic cargo parts off the belt");
    assert.ok(PartWasDropped, "De-energized Electromagnet Robot must release & drop captured parts back toward belt");

    // ④ Stamping Press #1 (X = -108) slam cuts forward speed drastically when car is beneath a slamming ram!
    let PressSlammed = false;
    for (let Step = 0; Step < 120; ++Step)
    {
        MockCar.Position = [-108.0, 0.0, 0.55];
        MockCar.LinearVelocity = [20.0, 0.0, 0.0];
        Seq.Advance(0.04);
        if (MockCar.LinearVelocity[0] < 5.0)
        {
            PressSlammed = true;
            break;
        }
    }
    assert.ok(PressSlammed, "Hydraulic Stamping Press slam must crush player forward velocity");

    // ⑤ Thermal Eddy-Current Brake Scanner Gate triggers EddyBrakeTimer when RED beam is active!
    let EddyBraked = false;
    for (let Step = 0; Step < 180; ++Step)
    {
        MockCar.Position = [136.0, 0.0, 0.55];
        MockCar.LinearVelocity = [18.0, 0.0, 0.0];
        Seq.Advance(0.04);
        if (Seq.EddyBrakeTimer > 0.5)
        {
            EddyBraked = true;
            break;
        }
    }
    assert.ok(EddyBraked, "Thermal Eddy-Current Brake Scanner Gate must activate eddy-brake drag on RED beam");

    // ⑥ Dynamic lights emit cyan electromagnet spotlights + emerald Finish Gate beacon
    const Lights = Seq.QueryDynamicLightRecords();
    assert.ok(Lights.length >= 1, "Must emit at least Finish Gate + active MagBot dynamic lights");

    // ⑦ Crossing QC Finish Gate (X >= 238 m) completes the run and records BestRunSec!
    MockCar.Position = [FoundryGoalX + 1.5, 0.0, 0.55];
    Seq.Advance(0.04);
    assert.equal(Seq.Finished, true, "Crossing FoundryGoalX must mark run Finished");
    assert.ok(Seq.BestRunSec !== null && Seq.BestRunSec > 0.0, "BestRunSec must be recorded on finish");
});
