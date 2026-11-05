// ====================================================================================================================================
//                                                     PROFILETLASSKIP.MJS
// ====================================================================================================================================
// 🧩 Measures the TLAS skip (patch 0003) in the lobby: CPU time per UpdateTlas call with the skip on and with it forced off, the skip
//    ratio, and a byte comparison of the forced rebuild against the uploaded nodes.
//
// 📝 CPU timing is plain JavaScript on the page's main thread, so it is not distorted by the GPU backend. Frame rates are not used:
//    SwiftShader is GPU-bound and says nothing about a real adapter.
//
// Usage: node Exhibits/VehicleLobbyPrototype/ProfileTlasSkip.mjs   (lobby server on LOBBY_ORIGIN, default http://127.0.0.1:8080)

import { createRequire } from "node:module";
import { fileURLToPath } from "node:url";
import { dirname, join } from "node:path";
import { writeFileSync } from "node:fs";

const Here = dirname(fileURLToPath(import.meta.url));
const Require = createRequire(join(Here, "..", "..", "ExternalPackages", "CaptureTools", "package.json"));
const { chromium } = Require("playwright-core");
const ChromiumPackage = (await import(Require.resolve("@sparticuz/chromium"))).default;

const Origin = process.env.LOBBY_ORIGIN || "http://127.0.0.1:8080";
const FramesPerPhase = Number(process.env.FRAMES || 8);
const sleep = (Milliseconds) => new Promise((Resolve) => setTimeout(Resolve, Milliseconds));

const Browser = await chromium.launch({
    executablePath: await ChromiumPackage.executablePath(),
    env: { ...process.env, LD_LIBRARY_PATH: process.env.LD_LIBRARY_PATH || "/tmp/al2023/lib",
           VK_ICD_FILENAMES: process.env.VK_ICD_FILENAMES || "/tmp/vk_swiftshader_icd.json" },
    args: [
        ...ChromiumPackage.args.filter((Argument) => !["--single-process", "--in-process-gpu"].includes(Argument)),
        "--enable-unsafe-webgpu", "--use-webgpu-adapter=swiftshader", "--enable-features=Vulkan",
        "--use-vulkan=swiftshader", "--disable-vulkan-surface"
    ],
    headless: true
});
const Page = await (await Browser.newContext({ viewport: { width: 1440, height: 900 } })).newPage();
const PageErrors = [];
Page.on("pageerror", (Error0) => PageErrors.push(String(Error0).slice(0, 300)));

// 📝 PAGE_PATH selects the scene: the lobby by default, or /index.html?quality=low for the Proving Grounds level.
const PagePath = process.env.PAGE_PATH || "/lobby/index.html?scene=studio&quality=low";
await Page.goto(`${Origin}${PagePath}`, { waitUntil: "domcontentloaded" });
for (let Attempt = 0; Attempt < 200; ++Attempt) {
    if (await Page.evaluate(() => (window.IlluminationExhibit?.FrameIndex ?? 0) >= 3).catch(() => false)) break;
    await sleep(1500);
}
await sleep(20000);   // let the parked car settle on its suspension before counting

// 📝 Part 1: direct UpdateTlas calls with unchanged inputs, skip on and forced off. Synchronous, so frame pacing does not matter.
// 📝 Part 2: a few real frames with the skip on, to count skips while the renderer runs (SwiftShader frames are slow, so this is short).
const Result = await Page.evaluate(async ({ Loops, InFrame }) => {
    const Host = window.IlluminationExhibit;
    const Rt = Host.RayTracing;
    const Original = Rt.UpdateTlas;
    const Timed = (Forced, Count) => {
        Rt.TlasForceRebuild = Forced;
        const Skips = Rt.TlasSkipCount || 0, Rebuilds = Rt.TlasRebuildCount || 0;
        const Start = performance.now();
        for (let I = 0; I < Count; ++I) Original.call(Rt, Host.Scene, Host.Vehicle);
        const Total = performance.now() - Start;
        Rt.TlasForceRebuild = false;
        return { Forced, Calls: Count, MeanMs: Total / Count,
                 Skipped: (Rt.TlasSkipCount || 0) - Skips, Rebuilt: (Rt.TlasRebuildCount || 0) - Rebuilds };
    };
    Timed(false, 20);          // warm-up
    Timed(true, 20);
    const Cached = Timed(false, Loops);
    const Forced = Timed(true, Loops);

    // Byte comparison: nodes uploaded on the skip path against a forced rebuild of the same inputs.
    const Bytes = Rt.UsedTlasNodes * 32;
    Original.call(Rt, Host.Scene, Host.Vehicle);
    const Before = new Uint8Array(Rt.AllNodeBytes.slice(0, Bytes));
    Rt.TlasForceRebuild = true;
    Original.call(Rt, Host.Scene, Host.Vehicle);
    Rt.TlasForceRebuild = false;
    const After = new Uint8Array(Rt.AllNodeBytes.slice(0, Bytes));
    let FirstDiff = -1;
    for (let I = 0; I < Bytes; ++I) if (Before[I] !== After[I]) { FirstDiff = I; break; }

    // In-frame counts: let the renderer run a few frames with the skip on.
    const InFrameSkips = Rt.TlasSkipCount || 0, InFrameRebuilds = Rt.TlasRebuildCount || 0;
    const Begin = Host.FrameIndex;
    await new Promise((Resolve) => {
        const Check = () => (Host.FrameIndex >= Begin + InFrame ? Resolve() : setTimeout(Check, 200));
        Check();
    });
    return {
        Cached, Forced,
        ByteComparison: { Bytes, Identical: FirstDiff < 0, FirstDifferentByte: FirstDiff, UsedTlasNodes: Rt.UsedTlasNodes, ActiveInstances: Rt.ActiveInstances },
        InFrame: { Frames: Host.FrameIndex - Begin, Skipped: (Rt.TlasSkipCount || 0) - InFrameSkips, Rebuilt: (Rt.TlasRebuildCount || 0) - InFrameRebuilds }
    };
}, { Loops: Number(process.env.LOOPS || 300), InFrame: FramesPerPhase });

const Summary = {
    Origin, Cached: Result.Cached, Forced: Result.Forced, ByteComparison: Result.ByteComparison, InFrame: Result.InFrame,
    MeanUpdateMsSaved: Result.Forced.MeanMs - Result.Cached.MeanMs,
    PageErrors: PageErrors.slice(0, 5)
};
const Scene = PagePath.startsWith("/lobby/") ? "lobby" : "level";
writeFileSync(join(Here, `tlas-skip-profile-${Scene}.json`), JSON.stringify({ ...Summary, PagePath }, null, 2));
console.log(JSON.stringify(Summary, null, 2));
await Browser.close();
process.exit(Result.ByteComparison.Identical && PageErrors.length === 0 ? 0 : 1);
