// ====================================================================================================================================
//                                                     CAPTUREGARAGEVIEWS.MJS
// ====================================================================================================================================
// 🧩 Opens the Garage on the tablet of the VehicleHMI demo (Experimental/VehicleHmiPrototype), then presses each inspection view
//    button (Solid, Ghost, Wireframe, Scan) and saves the tablet canvas as a PNG. Reports the appearance view after each press.
//
// 📝 Runs the same WebGPU path as the demo on SwiftShader (CPU emulation). The tablet pixels are real renderer output, but the frame
//    rate is not a performance measure.
//
// Usage: DEMO_ORIGIN=http://127.0.0.1:8090 node Exhibits/VehicleHmiDemo/CaptureGarageViews.mjs

import { createRequire } from "node:module";
import { fileURLToPath } from "node:url";
import { dirname, join } from "node:path";
import { writeFileSync } from "node:fs";

const Here = dirname(fileURLToPath(import.meta.url));
const Require = createRequire(join(Here, "..", "..", "ExternalPackages", "CaptureTools", "package.json"));
const { chromium } = Require("playwright-core");
const ChromiumPackage = (await import(Require.resolve("@sparticuz/chromium"))).default;

const Origin = process.env.DEMO_ORIGIN || "http://127.0.0.1:8090";
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

await Page.goto(`${Origin}/index.html?quality=low`, { waitUntil: "domcontentloaded" });
for (let Attempt = 0; Attempt < 300; ++Attempt) {
    if (await Page.evaluate(() => (window.IlluminationExhibit?.FrameIndex ?? 0) >= 3).catch(() => false)) break;
    await sleep(1500);
}

const Modes = ["Solid", "Ghost", "Wireframe", "Scan"];
const Steps = await Page.evaluate(async (ModeNames) => {
    const Host = window.IlluminationExhibit;
    const Screen = Host.Cabin.Screen;
    const WaitFrames = (Count) => new Promise((Resolve) => {
        const Target = Host.FrameIndex + Count;
        const Check = () => (Host.FrameIndex >= Target ? Resolve() : setTimeout(Check, 100));
        Check();
    });
    Screen.PresentApplication("garage");
    Screen.Dirty = true;
    await WaitFrames(3);
    const Out = [];
    for (let Mode = 0; Mode < ModeNames.length; ++Mode) {
        const Region = Screen.Regions.find((R) => R.Identifier === "view:" + Mode);
        if (Region && Region.Action) Region.Action();
        await WaitFrames(3);
        Out.push({
            Mode: ModeNames[Mode],
            RegionFound: Boolean(Region),
            AppearanceView: Host.Cabin.Appearance?.View ?? null,
            Png: Screen.Canvas.toDataURL("image/png")
        });
    }
    return Out;
}, Modes);

const Summary = [];
for (const Step of Steps) {
    const Name = `garage-view-${Step.Mode.toLowerCase()}.png`;
    writeFileSync(join(Here, Name), Buffer.from(Step.Png.split(",")[1], "base64"));
    Summary.push({ Mode: Step.Mode, RegionFound: Step.RegionFound, AppearanceView: Step.AppearanceView, Png: Name });
}
const Report = { Origin, Views: Summary, PageErrors: PageErrors.slice(0, 5) };
writeFileSync(join(Here, "garage-views.json"), JSON.stringify(Report, null, 2));
console.log(JSON.stringify(Report, null, 2));
await Browser.close();
process.exit(PageErrors.length === 0 && Summary.every((S) => S.RegionFound) ? 0 : 1);
