// ====================================================================================================================================
//                                                     DIAGNOSETLASMOVERS.MJS
// ====================================================================================================================================
// 🔍 Lists the TLAS leaves whose bounds change between two consecutive frames in the lobby. Explains why the TLAS skip
//    (patch 0003) does not fire on a parked car. Writes tlas-movers-lobby.json next to this file.
// Usage: node Exhibits/VehicleLobbyPrototype/DiagnoseTlasMovers.mjs   (lobby server on 127.0.0.1:8080)

import { createRequire } from "node:module";
import { fileURLToPath } from "node:url";
import { dirname, join } from "node:path";
import { writeFileSync } from "node:fs";
const Here = dirname(fileURLToPath(import.meta.url));
const Require = createRequire(join(Here, "..", "..", "ExternalPackages", "CaptureTools", "package.json"));
const { chromium } = Require("playwright-core");
const ChromiumPackage = (await import(Require.resolve("@sparticuz/chromium"))).default;
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));
const Browser = await chromium.launch({
    executablePath: await ChromiumPackage.executablePath(),
    env: { ...process.env, LD_LIBRARY_PATH: "/tmp/al2023/lib", VK_ICD_FILENAMES: "/tmp/vk_swiftshader_icd.json" },
    args: [...ChromiumPackage.args.filter((a) => !["--single-process", "--in-process-gpu"].includes(a)),
        "--enable-unsafe-webgpu", "--use-webgpu-adapter=swiftshader", "--enable-features=Vulkan", "--use-vulkan=swiftshader", "--disable-vulkan-surface"],
    headless: true
});
const Page = await (await Browser.newContext({ viewport: { width: 1440, height: 900 } })).newPage();
await Page.goto("http://127.0.0.1:8080/lobby/index.html?scene=studio&quality=low", { waitUntil: "domcontentloaded" });
for (let i = 0; i < 200; ++i) { if (await Page.evaluate(() => (window.IlluminationExhibit?.FrameIndex ?? 0) >= 3).catch(() => false)) break; await sleep(1500); }
const Out = await Page.evaluate(async () => {
    const Host = window.IlluminationExhibit, Rt = Host.RayTracing;
    const Snap = () => ({ B: Array.from(Rt.TlasKeyBounds.slice(0, 6 * 40)), I: Array.from(Rt.TlasKeyIdentity.slice(0, 2 * 40)), N: Rt.TlasKeyCount });
    const Frame = (n) => new Promise((res) => { const t = Host.FrameIndex + n; const c = () => (Host.FrameIndex >= t ? res() : setTimeout(c, 100)); c(); });
    await Frame(1);
    const A = Snap();
    await Frame(1);
    const B = Snap();
    const Movers = [];
    for (let k = 0; k < A.N; ++k) {
        let d = 0;
        for (let j = 0; j < 6; ++j) d = Math.max(d, Math.abs(A.B[6 * k + j] - B.B[6 * k + j]));
        if (d > 0) {
            const Occ = Host.Scene.Occupants[B.I[2 * k]];
            Movers.push({ Leaf: k, Occupant: B.I[2 * k], MaxDelta: d, Name: Occ?.Name ?? Occ?.Label ?? Occ?.Id ?? Object.keys(Occ || {}).slice(0, 8).join(",") });
        }
    }
    return { Count: B.N, Movers: Movers.slice(0, 20), MoverCount: Movers.length, MoverTotalLeaves: A.N };
}, null);
writeFileSync(join(Here, "tlas-movers-lobby.json"), JSON.stringify(Out, null, 2));
console.log(JSON.stringify(Out, null, 2));
await Browser.close();
