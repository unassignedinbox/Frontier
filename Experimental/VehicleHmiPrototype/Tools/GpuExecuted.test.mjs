//============================================================================================================================================
//                                                         GPUEXECUTED.TEST.MJS
//============================================================================================================================================
// 🧩 Executed-shader checks: the real app runs headless on a SwiftShader Vulkan device (Tools/RunOnSoftwareGpu.mjs), so every WGSL program
//    really executes, and the captured frames are compared. Opt-in (minutes, needs registry.npmjs.org once for SwiftShader):
//
//        RUN_GPU=1 npm run test:gpu                       (SIZE, HARNESS_TIMEOUT and GPU_FRAMES override the defaults)
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
        Child.on("exit", (Code) => (Code === 0 && fs.existsSync(Out)) ? Resolve({ Name, Out, Log, Image: DecodePng(Out) }) : Reject(new Error(`${Name} exited ${Code}\n${Log.slice(-1500)}`)));
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

test("executed: GTX frame is lit, finite and the tarmac stays tarmac through the old z-fight frames", { skip: !Enabled, timeout: 1800000 }, async () =>
{
    const Off = await Once("gtx-grid", "renderer=gtx");
    assert.match(Off.Log, /0 errors, 0 warnings/);
    GreyAsphalt(Region(Off.Image, 0.02, 0.8, 0.25, 0.97), "ground-left");
    GreyAsphalt(Region(Off.Image, 0.75, 0.8, 0.98, 0.97), "ground-right");
    const Sky = Region(Off.Image, 0.1, 0.02, 0.9, 0.12);
    assert.ok(Sky[2] > Sky[0] && Sky[2] > 120, `the sky is blue (${Sky.map((V) => V.toFixed(0))})`);
});

test("executed: the world light grid is an exact cull (GTX frame identical with every light tested)", { skip: !Enabled, timeout: 1800000 }, async () =>
{
    const [On, Off] = await Pair(Once("gtx-grid", "renderer=gtx"), Once("gtx-nogrid", "renderer=gtx&lightgrid=0"));
    const Difference = Compare(On.Image, Off.Image);
    // 📝 Measured noise floor: two IDENTICAL runs differ in ~40 of 20736 pixels by one 8-bit step (unordered atomics in the card relighting), so
    //    "equal" means at most one or two steps in a couple of percent of the pixels, not bit equality. A light the grid wrongly dropped would
    //    change whole beams by tens of steps.
    assert.ok(Difference.Max <= 2 && Difference.Different <= 0.02 * Difference.Pixels,
              `grid on vs off must agree to the noise floor, ${Difference.Different} of ${Difference.Pixels} pixels differ (max ${Difference.Max})`);
});

test("executed: screen traces are sane, finite and change the GTX image", { skip: !Enabled, timeout: 1800000 }, async () =>
{
    const [Off, On] = await Pair(Once("gtx-grid", "renderer=gtx"), Once("gtx-screen", "renderer=gtx&screentraces=1"));
    assert.match(On.Log, /0 errors, 0 warnings/);
    GreyAsphalt(Region(On.Image, 0.02, 0.8, 0.25, 0.97), "screen traces: ground-left");
    const Difference = Compare(Off.Image, On.Image);
    assert.ok(Difference.Different > 0, "enabling the screen traces must change some pixels (otherwise the gate does nothing)");
    assert.ok(Difference.MeanAbs < 6, `screen traces must stay close to the distance-field reference (mean abs ${Difference.MeanAbs.toFixed(2)} / 255)`);
});

test("executed: RTX path tracer + screen traces + light grid render a sane, tarmac-grey frame", { skip: !Enabled, timeout: 1800000 }, async () =>
{
    const [Rtx, RtxScreen] = await Pair(Once("rtx", "renderer=rtx"), Once("rtx-screen", "renderer=rtx&screentraces=1"));
    for (const Run of [Rtx, RtxScreen])
    {
        assert.match(Run.Log, /0 errors, 0 warnings/);
        GreyAsphalt(Region(Run.Image, 0.02, 0.8, 0.25, 0.97), `${Run.Name}: ground-left`);
    }
    const Difference = Compare(Rtx.Image, RtxScreen.Image);
    assert.ok(Difference.Different > 0, "enabling the screen traces must change some RTX pixels");
    assert.ok(Difference.MeanAbs < 10, `RTX with screen traces must stay close to the pure BVH path tracer (mean abs ${Difference.MeanAbs.toFixed(2)} / 255)`);
});

process.on("exit", () => { try { fs.rmSync(Scratch, { recursive: true, force: true }); } catch { /* best effort */ } });
