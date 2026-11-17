//============================================================================================================================================
//                                                          DEMOLISHRUN.TEST.MJS
//============================================================================================================================================
// Verifies Demolish Run · Fracture City ("demolish"):
//  1. 16 new Demolition PBR materials (170..185) + 3 FractureMaterials (brick, garage, drywall) & watertight closed 2-manifold geometry.
//  2. True procedural generation & unique 3D plane-cut fracturing across random seeds (different street curves, houses, fractures).
//  3. Guaranteed open hollow drive-through garage bays & storefront interiors (zero blocking AABB inside driving tunnels).
//  4. Full SceneStructure("demolish") construction under IdentityCeiling + dynamic crane/ball/charge/collapse GPU transforms.
//  5. Real-time crane wrecking-ball strikes, proximity C4 detonations, tipping fractured facade slabs, rubble gravity & fracture coupling.

import test from "node:test";
import assert from "node:assert/strict";
import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

import { CourseMaterial, ConstructMaterialSpecification } from "../Source/MaterialSpecification.js";
import { FractureMaterials } from "../Source/FractureStructure.js";
import {
    DemolishPoolCounts,
    DemolishSpawn,
    DemolishFinishX,
    PrepareDemolishCity,
    BuildDemolishCity,
    SampleDemolishPathY,
    ResolveDemolishShapes
} from "../Source/DemolishRunStructure.js";
import { DemolishRunSequence } from "../Source/DemolishRunSequence.js";
import { SceneStructure, IdentityCeiling } from "../Source/FacetStructure.js";

const Here = path.dirname(fileURLToPath(import.meta.url));
const Root = path.resolve(Here, "..");

function QuantizeKey(P)
{
    return `${Math.round(P[0] * 2000)},${Math.round(P[1] * 2000)},${Math.round(P[2] * 2000)}`;
}

function VerifyClosedWatertightManifold(Name, Facets)
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

        const Pairs = [[A, B], [B, C], [C, A]];
        for (const [U, V] of Pairs)
        {
            const Key = `${QuantizeKey(U)}->${QuantizeKey(V)}`;
            DirectedEdges.set(Key, (DirectedEdges.get(Key) || 0) + 1);
        }
    };

    for (const F of Facets)
    {
        assert.ok(F.Family >= 0 && F.Family < 186, `${Name}: invalid material family ${F.Family}`);
        if (F.Corners.length === 3)
            AddTri(F.Corners[0], F.Corners[1], F.Corners[2]);
        else if (F.Corners.length === 4)
        {
            AddTri(F.Corners[0], F.Corners[1], F.Corners[2]);
            AddTri(F.Corners[0], F.Corners[2], F.Corners[3]);
        }
        else
            assert.fail(`${Name}: unsupported polygon arity ${F.Corners.length}`);
    }

    let Unmatched = 0;
    for (const [Key, Count] of DirectedEdges.entries())
    {
        const [U, V] = Key.split("->");
        const Rev = DirectedEdges.get(`${V}->${U}`) || 0;
        if (Rev !== Count)
            Unmatched++;
    }
    assert.equal(Unmatched, 0, `${Name}: ${Unmatched} unmatched half-edges (not closed 2-manifold)`);
    assert.ok(SignedVol6 > 1.0e-5, `${Name}: non-positive outward volume (${SignedVol6})`);
}

function BoxesOverlap(A, B)
{
    return A.Minimum[0] < B.Maximum[0] && A.Maximum[0] > B.Minimum[0]
        && A.Minimum[1] < B.Maximum[1] && A.Maximum[1] > B.Minimum[1]
        && A.Minimum[2] < B.Maximum[2] && A.Maximum[2] > B.Minimum[2];
}

test("Demolish Run PBR Materials (170..185), FractureMaterials & All Procedural + Dynamic Shapes Are Closed Watertight 2-Manifolds", () =>
{
    const Materials = ConstructMaterialSpecification();
    assert.equal(Materials.length, 186);
    assert.ok(Materials[CourseMaterial.DemoBrickRed]);
    assert.ok(Materials[CourseMaterial.DemoGarageDoor]);
    assert.ok(Materials[CourseMaterial.DemoCraneYellow]);
    assert.ok(Materials[CourseMaterial.DemoBlastEmissive].Emission[0] > 3.0);
    assert.ok(Materials[CourseMaterial.DemoFinishGreen].Emission[1] > 2.5);

    assert.ok(FractureMaterials.brick && FractureMaterials.brick.Skin === CourseMaterial.DemoBrickRed);
    assert.ok(FractureMaterials.garage && FractureMaterials.garage.Skin === CourseMaterial.DemoGarageDoor);
    assert.ok(FractureMaterials.drywall && FractureMaterials.drywall.Skin === CourseMaterial.DemoInteriorWall);

    PrepareDemolishCity(0x12345678);
    const Shapes = ResolveDemolishShapes();
    for (const [Name, Spec] of Object.entries(Shapes))
        VerifyClosedWatertightManifold(Name, Spec.Emit());
});

test("Procedural Generation & 3D Plane-Cut Fracturing Produce Unique Cities & Fractures On Every New Seed", () =>
{
    const CityA = BuildDemolishCity(0x11A2B3C4);
    const CityB = BuildDemolishCity(0x99F8E7D6);
    const CityRand1 = PrepareDemolishCity(null);
    const CityRand2 = PrepareDemolishCity(null);

    assert.notEqual(CityA.Seed, CityB.Seed);
    assert.notEqual(CityRand1.Seed, CityRand2.Seed);
    assert.notEqual(CityA.CollapseWallSeed, CityB.CollapseWallSeed);

    // Street path curvature differs between seeds
    let PathDiffSum = 0.0;
    for (let X = -100; X <= 180; X += 20)
        PathDiffSum += Math.abs(SampleDemolishPathY(CityA, X) - SampleDemolishPathY(CityB, X));
    assert.ok(PathDiffSum > 8.0, `Expected distinctly curving neighborhood streets, diff sum = ${PathDiffSum}`);

    // Breakable fracture seeds & hero building layouts differ
    assert.ok(CityA.Breakables.length >= 16 && CityB.Breakables.length >= 16);
    assert.notEqual(CityA.Breakables[0].Seed, CityB.Breakables[0].Seed);

    // Procedural fractured geometry differs in vertex coordinates
    const FirstA = CityA.BlockFacets[0][CityA.BlockFacets[0].length - 1].Corners[0];
    const FirstB = CityB.BlockFacets[0][CityB.BlockFacets[0].length - 1].Corners[0];
    const Delta = Math.hypot(FirstA[0] - FirstB[0], FirstA[1] - FirstB[1], FirstA[2] - FirstB[2]);
    assert.ok(Delta > 0.05, "Expected unique procedurally fractured building geometry across seeds");
});

test("Every Drive-Through Garage & Storefront Bay + Driveway Approach/Exit Is Physically Open Across Multiple Seeds", () =>
{
    for (const Seed of [0x10203040, 0x55AA66BB, 0xCAFEBABE, 0x77119933, 0x155ECE, 0x25E82E])
    {
        const City = BuildDemolishCity(Seed);
        assert.equal(City.HeroBuildings.length, 6);
        assert.ok(City.HeroBuildings.filter((H) => H.Kind === "garage").length >= 3, "At least 3 drive-through garages per run");

        for (const Hero of City.HeroBuildings)
        {
            // Full approach + interior + exit driving corridor must have ZERO blocking static colliders!
            const DriveCorridor = {
                Minimum: [Hero.EntryX - 8.0, Hero.Center[1] - 2.0, 0.15],
                Maximum: [Hero.ExitX + 8.0,  Hero.Center[1] + 2.0, 3.0]
            };
            for (const Coll of City.ExtraObstacles)
            {
                assert.ok(!BoxesOverlap(DriveCorridor, Coll),
                    `Seed ${Seed.toString(16)}: collider ${JSON.stringify(Coll)} overlaps open ${Hero.Kind} corridor at [${Hero.Center}]`);
            }
        }
    }
});

test("SceneStructure('demolish') Constructs Cleanly Under IdentityCeiling & Writes Dynamic GPU Crane/Wall/Ball Transforms", async () =>
{
    const PrevFetch = globalThis.fetch;
    globalThis.fetch = async (Url) =>
    {
        const DiskPath = path.join(Root, String(Url).replace(/^\/+/, ""));
        const Buf = fs.readFileSync(DiskPath);
        return {
            ok: true,
            status: 200,
            arrayBuffer: async () => Buf.buffer.slice(Buf.byteOffset, Buf.byteOffset + Buf.byteLength)
        };
    };

    try
    {
        PrepareDemolishCity(0x2026DE01);
        const Scene = new SceneStructure();
        await Scene.ConstructScene("Archive", 0, "demolish");

        assert.ok(Scene.BaseDistanceCount < IdentityCeiling - 313,
            `BaseDistanceCount ${Scene.BaseDistanceCount} must stay safely below IdentityCeiling ${IdentityCeiling}`);
        assert.ok(Scene.Breakables.length >= 16, `Expected >= 16 breakable walls/doors/windows, got ${Scene.Breakables.length}`);
        assert.ok(Scene.ExtraObstacles.length >= 40, `Expected >= 40 exact building wall colliders, got ${Scene.ExtraObstacles.length}`);
        assert.ok(Scene.DemolishBlock, "Expected Scene.DemolishBlock allocated");

        const ExpectedDynamic = DemolishPoolCounts.CraneBases + DemolishPoolCounts.CraneTowers + DemolishPoolCounts.CraneJibs
                              + DemolishPoolCounts.CraneCables + DemolishPoolCounts.WreckingBalls + DemolishPoolCounts.ChargePacks
                              + DemolishPoolCounts.CollapseWalls + DemolishPoolCounts.FallingChunks;
        assert.equal(Scene.DemolishBlock.Count, ExpectedDynamic);

        // QueryObstacleBounds must include our hollow-building wall AABBs
        const AllObs = Scene.QueryObstacleBounds();
        assert.ok(AllObs.length >= Scene.ExtraObstacles.length);

        const MockHost = {
            LevelKey: "demolish",
            Scene,
            Vehicle: {
                Position:        [DemolishSpawn.Position[0], 0.0, 0.5],
                LinearVelocity:  [0.0, 0.0, 0.0],
                AngularVelocity: [0.0, 0.0, 0.0]
            }
        };
        const Seq = new DemolishRunSequence(MockHost);
        assert.equal(Seq.Enabled, true);

        // Verify crane bases, towers, wrecking balls & collapse walls are positioned above ground via Occ.Transform!
        const CraneBaseOcc = Scene.Occupants[Scene.DemolishBlock.CraneBases[0]];
        const BallOcc      = Scene.Occupants[Scene.DemolishBlock.WreckingBalls[0]];
        const WallOcc      = Scene.Occupants[Scene.DemolishBlock.CollapseWalls[0]];
        assert.equal(CraneBaseOcc.Inactive, false);
        assert.ok(CraneBaseOcc.Transform[14] >= -0.01 && CraneBaseOcc.Transform[14] < 1.0, "Crane base grounded at Z=0");
        assert.ok(BallOcc.Transform[14] > 4.0 && BallOcc.Transform[14] < 15.0, `Wrecking ball suspended aloft (Z=${BallOcc.Transform[14]})`);
        assert.ok(WallOcc.Transform[14] >= 0.0 && WallOcc.Transform[14] < 1.0, "Collapsing facade anchored at ground");
    }
    finally
    {
        globalThis.fetch = PrevFetch;
    }
});

test("Real-Time Cranes, C4 Detonations, Collapsing Facades, Rubble Physics & Wall Break Coupling Advance Cleanly", () =>
{
    PrepareDemolishCity(0x33445566);
    const TotalDyn = DemolishPoolCounts.CraneBases + DemolishPoolCounts.CraneTowers + DemolishPoolCounts.CraneJibs
                   + DemolishPoolCounts.CraneCables + DemolishPoolCounts.WreckingBalls + DemolishPoolCounts.ChargePacks
                   + DemolishPoolCounts.CollapseWalls + DemolishPoolCounts.FallingChunks;
    const MockOccupants = Array.from({ length: TotalDyn }, () => ({ Transform: null, PreviousTransform: null, Moving: false, Inactive: false }));
    let Cursor = 0;
    const Slice = (N) => Array.from({ length: N }, () => Cursor++);
    const MockBlock = {
        First:         0,
        CraneBases:    Slice(DemolishPoolCounts.CraneBases),
        CraneTowers:   Slice(DemolishPoolCounts.CraneTowers),
        CraneJibs:     Slice(DemolishPoolCounts.CraneJibs),
        CraneCables:   Slice(DemolishPoolCounts.CraneCables),
        WreckingBalls: Slice(DemolishPoolCounts.WreckingBalls),
        ChargePacks:   Slice(DemolishPoolCounts.ChargePacks),
        CollapseWalls: Slice(DemolishPoolCounts.CollapseWalls),
        FallingChunks: Slice(DemolishPoolCounts.FallingChunks),
        Count:         TotalDyn
    };

    let Bursts = 0;
    let SparksCount = 0;
    const MockCar = { Position: [-110.0, 0.0, 0.5], LinearVelocity: [12.0, 0.0, 0.0], AngularVelocity: [0.0, 0.0, 0.0] };
    const MockHost = {
        LevelKey: "demolish",
        Scene:    { Occupants: MockOccupants, DemolishBlock: MockBlock },
        Vehicle:  MockCar,
        Smoke:    { Sources: { QueueBurst: () => { Bursts++; } } },
        Sparks:   { Emit: (List) => { SparksCount += List.length; } },
        Effects:  { BurstTyre: () => {} },
        DrivingHud: { FallbackFuel: 0.4, FallbackBattery: 0.3, Audio: { PlayExplosionBoom: () => {} } }
    };

    const Seq = new DemolishRunSequence(MockHost);
    const Ball0Start = Seq.Cranes[0].BallPos.slice();

    // Advance 6 seconds while driving forward through blocks 0..1 -> cranes swing & strike, C4 detonates, walls tip!
    for (let Step = 0; Step < 300; ++Step)
    {
        MockCar.Position[0] += 0.25;
        Seq.Advance(0.02);
    }

    // Verify cranes actively swung their wrecking balls
    const BallMove = Math.hypot(Seq.Cranes[0].BallPos[0] - Ball0Start[0], Seq.Cranes[0].BallPos[1] - Ball0Start[1]);
    assert.ok(BallMove > 0.5, `Expected crane wrecking ball motion > 0.5 m, got ${BallMove}`);
    assert.ok(Seq.ExplosionsFired >= 2, `Expected crane & C4 explosions >= 2, got ${Seq.ExplosionsFired}`);
    assert.ok(Bursts >= 2 && SparksCount >= 10, "Expected real volumetric smoke bursts & sparks");
    assert.ok(Seq.CollapseWalls.some((W) => W.TiltAngle > 1.0), "Expected at least one fractured facade wall to collapse after C4 blast");
    assert.ok(Seq.FallingChunks.some((C) => C.Active), "Expected fractured masonry boulders spawned & tumbling");
    assert.ok(MockHost.DrivingHud.FallbackBattery >= 0.75, "Battery kept cleanly topped up");

    // Verify FractureSequence.OnBreak hook updates HUD counters & emits dust
    Seq.OnWallBroken({ Definition: { DemolishTag: "garage_door", Centre: [-90, 4, 1.4] }, Pieces: [1, 2, 3, 4, 5] });
    Seq.OnWallBroken({ Definition: { DemolishTag: "brick_wall",  Centre: [-82, 4, 1.4] }, Pieces: [1, 2, 3, 4] });
    assert.equal(Seq.WallsSmashed, 2);
    assert.equal(Seq.GaragesBreached, 1);

    // Verify extraction finish detection
    MockCar.Position[0] = DemolishFinishX + 2.0;
    Seq.Advance(0.02);
    assert.equal(Seq.Finished, true);
});
