import { test } from "node:test";
import assert from "node:assert/strict";
import { readFileSync } from "node:fs";
import { fileURLToPath } from "node:url";
import path from "node:path";

import {
    LightingSystem, LightType, MaxSceneLights, MaxEmissiveLights, TotalLightRecords, LightRecordFloats,
    ClusterGridBounds, ClusterHeaderCells, RealCellBase, EmissiveCellBase
} from "../Source/LightingSystem.js";
import { ParseIesProfile, BuildBuiltinIesText, BuiltinIesDistributions, WriteIesProfile, IesProfileSamples } from "../Source/PhotometricProfiles.js";
import { GatherEmissiveLights, BlackbodyColour } from "../Source/EmissiveSources.js";

// 📝 Column-major 4×4 transform of the car: forward +X, left +Y, up +Z, placed at the origin plus an offset.
function VehicleAt(X, Y, Z)
{
    return [1, 0, 0, 0,  0, 1, 0, 0,  0, 0, 1, 0,  X, Y, Z, 1];
}

const Here = path.dirname(fileURLToPath(import.meta.url));
const Root = path.resolve(Here, "..");
const ShaderPath = (Name) => path.join(Root, "Shader", Name);
const Real = new Set([LightType.Omni, LightType.StreetLamp, LightType.LowBeam, LightType.HighBeam, LightType.Fog,
                      LightType.Floodlight, LightType.Spot, LightType.Ies]);

// 📐 The cell index holding a world position, as the shader's ResolveClusterRangeAt computes it.
function CellOf(Position)
{
    const Grid = ClusterGridBounds;
    const Scaled = Position.map((Value, Axis) =>
        Math.min(1, Math.max(0, (Value - Grid.Minimum[Axis]) / Grid.Extent[Axis])));
    const Cell = [Math.min(Grid.ResX - 1, Math.floor(Scaled[0] * Grid.ResX)),
                  Math.min(Grid.ResY - 1, Math.floor(Scaled[1] * Grid.ResY)),
                  Math.min(Grid.ResZ - 1, Math.floor(Scaled[2] * Grid.ResZ))];
    return (Cell[2] * Grid.ResY + Cell[1]) * Grid.ResX + Cell[0];
}

function CellRange(System, Base, Cell)
{
    const Row = (Base + Cell) * 4;
    return { Offset: System.ClusterCells[Row], Count: System.ClusterCells[Row + 1] };
}

function IndicesOf(System, Range)
{
    return Array.from(System.ClusterIndices.slice(Range.Offset, Range.Offset + Range.Count));
}

// -----------------------------------------------------------------------------------------------------------------------------------
test("two clustered-light systems: real luminaires and emissive proxies are binned into separate cell tables", () =>
{
    const System = new LightingSystem();
    const Emitters = GatherEmissiveLights([{ Position: [5.0, 0.0, 0.5], Kelvin: 1500, Size: 1.0 }]);
    System.Update(VehicleAt(0.0, 0.0, 0.0), [{ X: 10.0, Y: 0.0, Z: 10.8 }], Emitters);

    // Real luminaires occupy records 0…31, emissive proxies records 32…95, in one buffer.
    assert.equal(System.BufferData.length, TotalLightRecords * LightRecordFloats);
    assert.equal(TotalLightRecords, MaxSceneLights + MaxEmissiveLights);
    const RecordKind = (Index) => System.BufferData[Index * LightRecordFloats + 3];
    assert.ok(Real.has(RecordKind(0)), "record 0 is a real luminaire");
    assert.equal(System.Lights.length > 0, true);
    for (let Light = 0; Light < System.Lights.length; ++Light)
        assert.ok(Real.has(RecordKind(Light)), `real record ${Light} carries a real kind`);
    assert.equal(System.EmissiveLights.length, 1);
    assert.equal(RecordKind(MaxSceneLights), LightType.Omni, "the first emissive proxy sits at record 32 as a point");

    // The street lamp's cell lists only real records; the emitter's cell lists only emissive records.
    const LampCell = CellOf([10.0, 0.0, 10.8]);
    const LampIndices = IndicesOf(System, CellRange(System, RealCellBase, LampCell));
    assert.ok(LampIndices.length > 0, "the pole lamp is binned in its real cell");
    assert.ok(LampIndices.every((Index) => Index < MaxSceneLights), "real cells hold only real records");

    const EmitCell = CellOf([5.0, 0.0, 0.5]);
    const EmitIndices = IndicesOf(System, CellRange(System, EmissiveCellBase, EmitCell));
    assert.ok(EmitIndices.length > 0, "the spark is binned in its emissive cell");
    assert.ok(EmitIndices.every((Index) => Index >= MaxSceneLights && Index < TotalLightRecords),
              "emissive cells hold only emissive records");
    assert.equal(EmissiveCellBase, ClusterHeaderCells + ClusterGridBounds.CellCount, "the emissive table follows the real table");
});

test("real luminaires cover point, spot and IES kinds in the vehicle's lighting modes", () =>
{
    const System = new LightingSystem();
    System.SetVehicleMode(2);   // high beam: spots; the tail lamps are points; the poles add IES luminaires
    System.Update(VehicleAt(0.0, 0.0, 0.0), [{ X: 10.0, Y: 0.0, Z: 10.8 }], []);
    const Kinds = new Set(System.Lights.map((Light) => Light.Type));
    assert.ok(Kinds.has(LightType.Omni), "a point light (tail lamp)");
    assert.ok(Kinds.has(LightType.HighBeam), "a spot (high beam)");
    assert.ok(Kinds.has(LightType.StreetLamp), "an IES luminaire (street lamp)");

    System.SetVehicleMode(1);   // low beam: IES headlights
    System.Update(VehicleAt(0.0, 0.0, 0.0), [], []);
    assert.ok(System.Lights.some((Light) => Light.Type === LightType.LowBeam && Light.Profile?.length === IesProfileSamples),
              "low beams carry a measured IES table");
    assert.ok(System.Lights.every((Light) => !Light.Profile || Light.Profile.length === IesProfileSamples));
});

test("emissive proxies follow their particles: hot ones light, cool ones do not, brightest first, capped", () =>
{
    const Particles = [
        { Position: [0, 0, 0], Kelvin: 900 },        // too cool to glow
        { Position: [1, 0, 0], Kelvin: 1300, Size: 1 },
        { Position: [2, 0, 0], Kelvin: 2400, Size: 1 },
        { Position: [3, 0, 0], Kelvin: 1700, Size: 1 }
    ];
    const Emitters = GatherEmissiveLights(Particles, 2);
    assert.equal(Emitters.length, 2, "capped at the limit");
    assert.deepEqual(Emitters[0].Position, [2, 0, 0], "the hottest spark is first");
    assert.ok(Emitters.every((Emitter) => Emitter.Type === LightType.Omni && Emitter.Reach > 0));
    const Cap = GatherEmissiveLights(Array.from({ length: 200 }, (_, I) => ({ Position: [I, 0, 0], Kelvin: 1500 })));
    assert.equal(Cap.length, 64);
});

test("blackbody colour is warm at low temperature and whitens as it heats, normalised per channel", () =>
{
    const Cool = BlackbodyColour(1500);
    const Hot = BlackbodyColour(6500);
    assert.equal(Cool[0], 1.0, "red saturates at 1500 K");
    assert.ok(Cool[2] < Cool[1] && Cool[1] < Cool[0], "a 1500 K spark is red-orange: r > g > b");
    assert.ok(Hot[2] > 0.9 && Hot[0] > 0.9, "a 6500 K source is close to white");
});

test("IES parser reads LM-63 text, resamples the vertical curve, and normalises it to the peak", () =>
{
    const Batwing = ParseIesProfile(BuildBuiltinIesText("RoadwayBatwing"));
    assert.equal(Batwing.Samples.length, IesProfileSamples);
    assert.equal(Math.max(...Batwing.Samples), 1.0, "normalised to the peak");
    const Peak = Batwing.Samples.indexOf(1.0);
    const PeakAngle = 180 * Peak / (IesProfileSamples - 1);
    assert.ok(PeakAngle > 55 && PeakAngle < 100, `the batwing peaks off-axis, at ${PeakAngle.toFixed(0)} degrees`);

    const Pencil = ParseIesProfile(BuildBuiltinIesText("AutomotiveHighBeam"));
    assert.equal(Pencil.Samples[0], 1.0, "a high beam is brightest along its axis");
    assert.ok(Pencil.Samples[8] < 0.01, "and nearly dark 45 degrees off it");
});

test("the IES round trip reproduces the distribution it was written from", () =>
{
    const Distribution = (Angle) => Math.cos(Angle) * Math.cos(Angle);   // cos² lobe, peak on axis
    const Text = WriteIesProfile({ Name: "RoundTrip", Description: "test", Distribution });
    const Parsed = ParseIesProfile(Text);
    assert.equal(Parsed.Vertical.length, 37, "five-degree steps from 0 to 180");
    for (let Sample = 0; Sample < IesProfileSamples; ++Sample)
    {
        const Angle = Math.PI * Sample / (IesProfileSamples - 1);
        assert.ok(Math.abs(Parsed.Samples[Sample] - Distribution(Angle)) < 0.02, `sample ${Sample}`);
    }
});

test("the IES parser rejects tilt data and truncated tables rather than inventing a distribution", () =>
{
    const Good = BuildBuiltinIesText("FogLamp");
    assert.throws(() => ParseIesProfile(Good.replace("TILT=NONE", "TILT=INCLUDE")), /tilt/i);
    const Cut = Good.split("\n").slice(0, 12).join("\n");
    assert.throws(() => ParseIesProfile(Cut));
    assert.throws(() => ParseIesProfile("IESNA:LM-63-2019\n[TEST] none\n"), /TILT/);
    assert.ok(Object.keys(BuiltinIesDistributions).length >= 5);
});

test("the shaders read the same record layout and cell tables the host writes", () =>
{
    const Clustered = readFileSync(ShaderPath("ClusteredLights.wgsl"), "utf8");
    const Direct = readFileSync(ShaderPath("DirectRadiance.wgsl"), "utf8");
    const Card = readFileSync(ShaderPath("CardDirect.wgsl"), "utf8");

    // 4 vec4f (64 B) + 8 vec4f of profile (128 B) = 48 floats per record, matching LightRecordFloats.
    assert.match(Clustered, /Profile\s*:\s*array<vec4f,\s*8>/);
    assert.equal(4 * 4 + 8 * 4, LightRecordFloats);
    assert.match(Clustered, /const IesSampleCount\s*:\s*u32\s*=\s*32u;/);
    assert.equal(IesProfileSamples, 32);

    // The cell bases agree with the JS constants.
    assert.match(Clustered, new RegExp(`const RealCellBase\\s*:\\s*u32\\s*=\\s*${RealCellBase}u;`));
    assert.match(Clustered, new RegExp(`const CellCount\\s*:\\s*u32\\s*=\\s*${ClusterGridBounds.CellCount}u;`));
    assert.match(Clustered, new RegExp(`const EmissiveCellBase\\s*:\\s*u32\\s*=\\s*3u \\+ ${ClusterGridBounds.CellCount}u;`));

    // Both lighting programs evaluate the emissive system, and the direct pass sees all 96 records.
    assert.match(Direct, /ResolveEmissiveClusterRange\(/);
    assert.match(Direct, /array<LightRecord,\s*96>/);
    assert.match(Card, /ResolveEmissiveClusterRange\(/);
});
