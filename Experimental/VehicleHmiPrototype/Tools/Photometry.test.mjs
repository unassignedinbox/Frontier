//============================================================================================================================================
//                                                         PHOTOMETRY.TEST.MJS
//============================================================================================================================================
// 🧩 IES (LM-63 Type C) photometry for the clustered lights: the parser, the packed atlas, the shapes of the shipped profiles, the light-type mapping, and (opt-in,
//    RUN_GPU=1) the shader's lookup executed on a software GPU against the CPU reference.

import test from "node:test";
import assert from "node:assert/strict";
import fs from "node:fs";
import path from "node:path";
import { spawnSync } from "node:child_process";
import { fileURLToPath } from "node:url";
import { ParseIes, SampleIes, ConstructPhotometricAtlas, SampleAtlas, PhotometryProfileFiles, PhotometryProfile, PhotometrySize, PhotometryBytes, PhotometryVectorCount, PhotometryTexelCount } from "../Source/Photometry.js";
import { LightingSystem, LightType } from "../Source/LightingSystem.js";

const Here = path.dirname(fileURLToPath(import.meta.url)), Root = path.resolve(Here, "..");
const Texts = Object.fromEntries(PhotometryProfileFiles.map((Name) => [Name, fs.readFileSync(path.join(Root, "Profiles", `${Name}.ies`), "utf8")]));
const Atlas = ConstructPhotometricAtlas(Texts);

test("every shipped profile is a valid LM-63 Type C table, labelled as authored", () =>
{
    for (const [Name, Text] of Object.entries(Texts))
    {
        const Profile = ParseIes(Text);
        assert.equal(Profile.Vertical.length, 91, Name); assert.equal(Profile.Horizontal.length, 37, Name);
        assert.ok(Profile.Peak > 0 && Profile.Candela.every((Value) => Value >= 0), Name);
        assert.match(Text, /authored demonstration profile - not measured photometry/, `${Name} says what it is`);
    }
});

test("the parser rejects what it cannot honour", () =>
{
    const Good = Texts.LowBeam;
    assert.throws(() => ParseIes(Good.replace("TILT=NONE", "TILT=INCLUDE")), /TILT=NONE/);
    assert.throws(() => ParseIes(Good.split("\n").slice(0, 8).join("\n")), /incomplete/);
    assert.throws(() => ParseIes(Good.replace("1 1000 1 91 37 1 2", "1 1000 1 91 37 2 2")), /Type C/);
    assert.throws(() => ParseIes(Good.replace("0 2 4 6", "0 6 4 2")), /increase from zero/);
    assert.throws(() => ParseIes(Good.replace("748.36765", "-3.0")), /nonnegative/);
});

test("the atlas: sizes agree with the shader's constants and the packing is a faithful 8-bit copy of the files", () =>
{
    assert.equal(PhotometryTexelCount, 7 * 37 * 91); assert.equal(PhotometryVectorCount, 1474); assert.equal(PhotometryBytes, 1474 * 16);
    const Shader = fs.readFileSync(path.join(Root, "Shader", "LightClusterSpace.wgsl"), "utf8");
    assert.match(Shader, /PhotometryAzimuthBins\s*:\s*u32\s*=\s*37u/); assert.match(Shader, /PhotometryPolarBins\s*:\s*u32\s*=\s*91u/);
    assert.match(Shader, /PhotometryTexelsPerProfile\s*:\s*u32\s*=\s*3367u/); assert.match(Shader, /Atlas\s*:\s*array<vec4u, 1474>/);
    PhotometryProfileFiles.forEach((Name, Index) =>
    {
        const Profile = ParseIes(Texts[Name]);
        let Worst = 0;
        for (let H = 0; H < 37; H += 3)
            for (let V = 0; V < 91; V += 2)
                Worst = Math.max(Worst, Math.abs(SampleIes(Profile, 360 * H / 36, 180 * V / 90) - Atlas[(Index * 37 + H) * 91 + V] / 255));
        assert.ok(Worst <= 0.5 / 255 + 1e-9, `${Name}: quantisation error ${Worst}`);
        const Peak = Math.max(...Atlas.subarray(Index * 3367, (Index + 1) * 3367));
        assert.equal(Peak, 255, `${Name} is peak-normalised`);
    });
});

test("the profiles have the shapes their names claim", () =>
{
    const At = (Name, Azimuth, Polar) => SampleAtlas(Atlas, PhotometryProfile[Name], Azimuth, Polar);
    // low beam: a sharp cutoff above the horizon (C0 is up, polar 6° above the axis) and a spread to the sides
    assert.ok(At("LowBeam", 0, 6) < 0.05 && At("LowBeam", 180, 6) > 0.3, "low beam: dark above its cutoff, lit below it");
    assert.ok(At("LowBeam", 270, 12) > 0.3 && At("LowBeam", 90, 12) > 0.3, "low beam spreads to both sides");
    // high beam: tight and symmetric
    assert.ok(At("HighBeam", 0, 0) > 0.9 && At("HighBeam", 90, 25) < 0.3 && Math.abs(At("HighBeam", 90, 10) - At("HighBeam", 270, 10)) < 0.05, "high beam: tight and symmetric");
    // fog: wider than low beam laterally; flood wider than spot
    assert.ok(At("FogBeam", 90, 40) > At("LowBeam", 90, 40) + 0.05 || At("FogBeam", 90, 40) > 0.1, "fog beam is wide");
    assert.ok(At("RoofFlood", 90, 30) > At("RoofSpot", 90, 30) + 0.1, "the flood pods throw wider than the spots");
    // roadway luminaire: a downlight, nothing behind it
    assert.ok(At("Roadway", 0, 40) > 0.7 && At("Roadway", 0, 120) < 1e-3, "roadway: down-light with nothing behind it");
    assert.equal(SampleAtlas(Atlas, 0, 12, 34), 1.0, "profile 0 is isotropic");
});

test("every light type the lighting system emits resolves to a photometric profile", () =>
{
    const Shader = fs.readFileSync(path.join(Root, "Shader", "LightClusterSpace.wgsl"), "utf8");
    const Mapping = Object.fromEntries([...Shader.matchAll(/if \(Type == (\d)u\) \{ return (?:select\()?(\d)u(?:, (\d)u, Light\.Params\.z > 0\.5\))?; \}/g)].map((Match) => [Number(Match[1]), Match[3] ? [Number(Match[2]), Number(Match[3])] : [Number(Match[2])]]));
    assert.deepEqual(Mapping, { 1: [1], 2: [3], 3: [4], 4: [5], 5: [6, 7] }, "the shader maps type → profile as documented");
    const Types = new Set();
    const Identity = new Float64Array([1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0.5, 1]);
    // every vehicle mode, the roof pods in both fixture styles, and a street lamp: together they cover every type the system can emit
    for (const Mode of [0, 1, 2, 3])
        for (const Fixture of [0, 1])
        {
            const System = new LightingSystem();
            System.SetVehicleMode(Mode); System.SetRoofFixture(Fixture); System.RoofLightEnabled = true;
            const Data = System.Update(Identity, [{ X: 5, Y: 5, Z: 10.8 }]);
            for (let Index = 0; Index < 32; ++Index) if (Data[Index * 16 + 15] > 0.5) Types.add(Data[Index * 16 + 3] + ":" + Data[Index * 16 + 14]);
        }
    const Seen = new Set([...Types].map((Key) => Number(Key.split(":")[0])));
    assert.ok([1, 2, 3, 4, 5].every((Type) => Seen.has(Type)), `the system emits every light type 1 to 5 (saw ${[...Types]})`);
    assert.ok(Types.has("5:0") && Types.has("5:1"), "both flood variants occur (SlimLED flood, rally spot pods)");
    for (const Type of Seen) assert.ok(Type === 0 || Mapping[Type], `type ${Type} has a profile or is omnidirectional`);
    assert.ok(LightType.StreetLamp === 1 && LightType.Floodlight === 5);
});

test("executed: the shader's profile lookup matches the CPU reference for every light type on a software GPU", { skip: process.env.RUN_GPU !== "1", timeout: 900000 }, () =>
{
    const Run = spawnSync(process.execPath, [path.join(Here, "RunOnSoftwareGpu.mjs"), process.execPath, path.join(Here, "IesParityGpu.mjs")], { cwd: Here, encoding: "utf8", timeout: 800000 });
    assert.equal(Run.status, 0, (Run.stdout + Run.stderr).slice(-1500));
    const Result = JSON.parse(Run.stdout.split("\n").find((Line) => Line.startsWith("RESULT ")).slice(7));
    assert.ok(Result.Finite && Result.NonZero > 200, `lit directions found (${Result.NonZero}) and finite`);
    assert.ok(Result.Worst < 0.02 && Result.Mean < 0.002, `GPU vs CPU: worst ${Result.Worst.toFixed(4)} (${JSON.stringify(Result.WorstCase)}), mean ${Result.Mean.toFixed(5)}`);
});
