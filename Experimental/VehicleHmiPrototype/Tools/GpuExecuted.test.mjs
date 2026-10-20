//============================================================================================================================================
//                                                         GPUEXECUTED.TEST.MJS
//============================================================================================================================================
// 🧩 Executed-shader checks: the real app runs headless on a SwiftShader Vulkan device (Tools/RunOnSoftwareGpu.mjs), so every WGSL program
//    really executes, and the captured frames are compared. Opt-in (minutes, needs registry.npmjs.org once for SwiftShader):
//
//        RUN_GPU=1 npm run test:gpu                       (SIZE, HARNESS_TIMEOUT and GPU_FRAMES override the defaults; GPU_ARTIFACT_DIR keeps the frames and logs)
//
// 📝 Frames are deterministic: the harness clock advances one 60 Hz step per executed frame and the TAA jitter and every random sequence
//    follow the frame index, so two runs with the same query and frame count produce the same image up to a tiny noise floor (about 40 of
//    20736 pixels, one 8-bit step). That lets a test assert EQUALITY to that floor (the light grid is an exact cull) as well as bounds.

import test from "node:test";
import assert from "node:assert/strict";
import fs from "node:fs";
import os from "node:os";
import path from "node:path";
import zlib from "node:zlib";
import { spawn } from "node:child_process";
import { fileURLToPath } from "node:url";

const Here = path.dirname(fileURLToPath(import.meta.url));
const Enabled = process.env.RUN_GPU === "1";
const Frames = Number(process.env.GPU_FRAMES || 16);   // past frame 15, where the old single surround quad used to win the depth test
const Scratch = fs.mkdtempSync(path.join(os.tmpdir(), "gpu-executed-"));

export function DecodePng(File)
{
    const Data = fs.readFileSync(File);
    let Offset = 8, Width = 0, Height = 0;
    const Parts = [];
    while (Offset < Data.length)
    {
        const Length = Data.readUInt32BE(Offset), Type = Data.toString("latin1", Offset + 4, Offset + 8);
        if (Type === "IHDR") { Width = Data.readUInt32BE(Offset + 8); Height = Data.readUInt32BE(Offset + 12); }
        if (Type === "IDAT") Parts.push(Data.subarray(Offset + 8, Offset + 8 + Length));
        Offset += 12 + Length;
    }
    const Raw = zlib.inflateSync(Buffer.concat(Parts)), Stride = Width * 4 + 1, Rgba = new Uint8Array(Width * Height * 4);
    for (let Row = 0; Row < Height; ++Row)
    {
        assert.equal(Raw[Row * Stride], 0, "the harness writes unfiltered PNG rows");
        Rgba.set(Raw.subarray(Row * Stride + 1, (Row + 1) * Stride), Row * Width * 4);
    }
    return { Width, Height, Rgba };
}

export function Compare(A, B)
{
    assert.equal(A.Width, B.Width); assert.equal(A.Height, B.Height);
    let Sum = 0, Max = 0, Different = 0;
    for (let Index = 0; Index < A.Rgba.length; Index += 4)
    {
        let Pixel = 0;
        for (let C = 0; C < 3; ++C) { const D = Math.abs(A.Rgba[Index + C] - B.Rgba[Index + C]); Sum += D; Pixel = Math.max(Pixel, D); }
        Max = Math.max(Max, Pixel);
        if (Pixel > 0) Different++;
    }
    return { MeanAbs: Sum / (A.Width * A.Height * 3), Max, Different, Pixels: A.Width * A.Height };
}

export function Region(Image, X0, Y0, X1, Y1)
{
    const Sum = [0, 0, 0]; let Count = 0;
    for (let Y = Math.floor(Image.Height * Y0); Y < Math.floor(Image.Height * Y1); ++Y)
        for (let X = Math.floor(Image.Width * X0); X < Math.floor(Image.Width * X1); ++X, ++Count)
            for (let C = 0; C < 3; ++C) Sum[C] += Image.Rgba[(Y * Image.Width + X) * 4 + C];
    return Sum.map((V) => V / Count);
}

function Render(Name, Query, FrameCount = Frames, Extra = {})
{
    const Out = path.join(Scratch, `${Name}.png`);
    return new Promise((Resolve, Reject) =>
    {
        const Child = spawn(process.execPath, [path.join(Here, "RunOnSoftwareGpu.mjs"), process.execPath, path.join(Here, "HeadlessHarness.mjs"), "..", String(FrameCount),
                            `?capture=1&quality=low&${Query}`], {
            cwd: Here, env: { ...process.env, SIZE: process.env.SIZE || "320x180", OUT: Out, HARNESS_TIMEOUT: process.env.HARNESS_TIMEOUT || "1500000", ...Extra }
        });
        let Log = "";
        Child.stdout.on("data", (Chunk) => { Log += Chunk; });
        Child.stderr.on("data", (Chunk) => { Log += Chunk; });
        Child.on("exit", (Code) =>
        {
            // 📝 GPU_ARTIFACT_DIR: keep every rendered frame and its log (CI uploads the directory, so a failed comparison can be looked at)
            if (process.env.GPU_ARTIFACT_DIR) { fs.mkdirSync(process.env.GPU_ARTIFACT_DIR, { recursive: true }); fs.writeFileSync(path.join(process.env.GPU_ARTIFACT_DIR, `${Name}.log`), Log); if (fs.existsSync(Out)) fs.copyFileSync(Out, path.join(process.env.GPU_ARTIFACT_DIR, `${Name}.png`)); }
            (Code === 0 && fs.existsSync(Out)) ? Resolve({ Name, Out, Log, Image: DecodePng(Out) }) : Reject(new Error(`${Name} exited ${Code}\n${Log.slice(-1500)}`));
        });
    });
}

const Pair = (A, B) => Promise.all([A, B]);
const Cache = new Map();
const Once = (Name, Query, FrameCount, Extra) => { if (!Cache.has(Name)) Cache.set(Name, Render(Name, Query, FrameCount, Extra)); return Cache.get(Name); };

const GreyAsphalt = (Rgb, Label) =>
{
    const [R, G, B] = Rgb;
    assert.ok(G < R * 1.25 && G < B * 1.35 && R > 20, `${Label}: the ground must read as grey tarmac, not grass (R ${R.toFixed(0)} G ${G.toFixed(0)} B ${B.toFixed(0)})`);
};

test("executed: SDF frame is lit, finite and the tarmac stays tarmac through the old z-fight frames", { skip: !Enabled, timeout: 1800000 }, async () =>
{
    const Off = await Once("sdf-grid", "renderer=sdf");
    assert.match(Off.Log, /0 errors, 0 warnings/);
    GreyAsphalt(Region(Off.Image, 0.02, 0.8, 0.25, 0.97), "ground-left");
    GreyAsphalt(Region(Off.Image, 0.75, 0.8, 0.98, 0.97), "ground-right");
    const Sky = Region(Off.Image, 0.1, 0.02, 0.9, 0.12);
    assert.ok(Sky[2] > Sky[0] && Sky[2] > 120, `the sky is blue (${Sky.map((V) => V.toFixed(0))})`);
});

test("executed: the world light grid is an exact cull (SDF frame identical with every light tested)", { skip: !Enabled, timeout: 1800000 }, async () =>
{
    const [On, Off] = await Pair(Once("sdf-grid", "renderer=sdf"), Once("sdf-nogrid", "renderer=sdf&lightgrid=0"));
    const Difference = Compare(On.Image, Off.Image);
    // 📝 Measured noise floor: two IDENTICAL runs differ in ~40 of 20736 pixels by one 8-bit step (unordered atomics in the card relighting), so
    //    "equal" means at most one or two steps in a couple of percent of the pixels, not bit equality. A light the grid wrongly dropped would
    //    change whole beams by tens of steps.
    assert.ok(Difference.Max <= 2 && Difference.Different <= 0.02 * Difference.Pixels,
              `grid on vs off must agree to the noise floor, ${Difference.Different} of ${Difference.Pixels} pixels differ (max ${Difference.Max})`);
});

test("executed: screen traces are sane, finite and change the SDF image", { skip: !Enabled, timeout: 1800000 }, async () =>
{
    const [Off, On] = await Pair(Once("sdf-grid", "renderer=sdf"), Once("sdf-screen", "renderer=sdf&screentraces=1"));
    assert.match(On.Log, /0 errors, 0 warnings/);
    GreyAsphalt(Region(On.Image, 0.02, 0.8, 0.25, 0.97), "screen traces: ground-left");
    const Difference = Compare(Off.Image, On.Image);
    assert.ok(Difference.Different > 0, "enabling the screen traces must change some pixels (otherwise the gate does nothing)");
    assert.ok(Difference.MeanAbs < 6, `screen traces must stay close to the distance-field reference (mean abs ${Difference.MeanAbs.toFixed(2)} / 255)`);
});

test("executed: BVH path tracer + screen traces + light grid render a sane, tarmac-grey frame", { skip: !Enabled, timeout: 1800000 }, async () =>
{
    const [Bvh, BvhScreen] = await Pair(Once("bvh", "renderer=bvh"), Once("bvh-screen", "renderer=bvh&screentraces=1"));
    for (const Run of [Bvh, BvhScreen])
    {
        assert.match(Run.Log, /0 errors, 0 warnings/);
        GreyAsphalt(Region(Run.Image, 0.02, 0.8, 0.25, 0.97), `${Run.Name}: ground-left`);
    }
    const Difference = Compare(Bvh.Image, BvhScreen.Image);
    assert.ok(Difference.Different > 0, "enabling the screen traces must change some BVH pixels");
    assert.ok(Difference.MeanAbs < 10, `BVH with screen traces must stay close to the pure BVH path tracer (mean abs ${Difference.MeanAbs.toFixed(2)} / 255)`);
});

process.on("exit", () => { try { fs.rmSync(Scratch, { recursive: true, force: true }); } catch { /* best effort */ } });

test("executed: emissive clusters on vs off at night: both lit and sane, and the partition neither adds nor removes energy", { skip: !Enabled, timeout: 1800000 }, async () =>
{
    // 📝 What this can and cannot show. At night the lamps and the tail strip are lit, but the analytic street lamps still dominate, so the structure moves few pixels: this is a smoke
    //    and BIAS test (the partition of unity must not change the frame's mean light anywhere), not a variance test. The estimator itself is verified where it can be: against the analytic
    //    irradiance in EmissiveLights.test.mjs, and the shader's sampler against its CPU twin over 3000 queries. The state hook proves each run really had the structure on or off.
    const Hooks = { HOOKS: path.join(Here, "EmissiveStateHooks.mjs") };
    const [On, Off] = await Pair(Once("night-emissive", "renderer=bvh&preset=Night", Frames, Hooks), Once("night-noemissive", "renderer=bvh&preset=Night&emissive=0", Frames, Hooks));
    const State = (Run) => JSON.parse(Run.Log.split("\n").find((Line) => Line.startsWith("EMISSIVESTATE ")).slice(14));
    assert.ok(State(On).Active && State(On).Instances >= 50 && State(On).Triangles >= 90, `emissive ON really ran the structure: ${JSON.stringify(State(On))}`);
    assert.ok(!State(Off).Active && !State(Off).Setting, `emissive OFF did not: ${JSON.stringify(State(Off))}`);
    for (const Run of [On, Off]) assert.match(Run.Log, /0 errors, 0 warnings/);
    const Night = Region(On.Image, 0.1, 0.02, 0.9, 0.12);
    assert.ok(Night[2] > Night[0] && Night[2] < 140, `a night sky is dark blue (${Night.map((V) => V.toFixed(0))})`);
    const Mean = (Image) => { let Sum = 0; for (let Index = 0; Index < Image.Rgba.length; Index += 4) Sum += 0.2126 * Image.Rgba[Index] + 0.7152 * Image.Rgba[Index + 1] + 0.0722 * Image.Rgba[Index + 2]; return Sum / (Image.Rgba.length / 4); };
    assert.ok(Mean(On.Image) > 25 && Mean(Off.Image) > 25, "both nights are lit by the lamps, not black");
    assert.ok(Math.abs(Mean(On.Image) - Mean(Off.Image)) < 0.03 * Mean(Off.Image), `the frame's mean light agrees to 3 %: ${Mean(On.Image).toFixed(2)} vs ${Mean(Off.Image).toFixed(2)}`);
    // measured at 16 frames: every region's mean differs by at most 1.0 of 255, with mixed signs; 4 is four times that and still catches a 5 % energy error in a region of luminance 80
    for (const [Name, Box] of Object.entries({ sky: [0.1, 0.02, 0.9, 0.12], ground: [0.02, 0.8, 0.98, 0.97], car: [0.35, 0.55, 0.65, 0.85], ahead: [0.4, 0.3, 0.6, 0.5], left: [0, 0.3, 0.3, 0.7], right: [0.7, 0.3, 1, 0.7] }))
    {
        const A = Region(On.Image, ...Box), B = Region(Off.Image, ...Box);
        for (let C = 0; C < 3; ++C) assert.ok(Math.abs(A[C] - B[C]) < 4, `${Name} channel ${C}: ${A[C].toFixed(1)} with the structure, ${B[C].toFixed(1)} without`);
    }
    const Difference = Compare(On.Image, Off.Image);
    assert.ok(Difference.MeanAbs < 4, `the frames stay close (mean abs ${Difference.MeanAbs.toFixed(2)} / 255)`);
});
