// ====================================================================================================================================
//                                                    CAPTURELOBBYPROOF.MJS
// ====================================================================================================================================
// 🧩 Drives the car configurator lobby in headless Chromium (SwiftShader WebGPU) and saves real screenshots and a result log.
//
// 📝 Scenario: (1) one member opens the lobby; (2) a peer joins; (3) the peer leaves; (4) a second peer joins and the lobby member
//    presses Ready while the peer is not ready; (5) the second peer presses Ready, and the lobby travels to the next level.
//    Every expected value is printed beside the observed value; the run exits 1 on any mismatch.
//
// Usage: (from the repository root, after Automation/RestoreVehicleHmiRenderer.py and Tools/Serve.py on port 8080)
//        node Exhibits/VehicleLobbyPrototype/CaptureLobbyProof.mjs
//        Environment: LOBBY_ORIGIN (default http://127.0.0.1:8080), OUTPUT_DIRECTORY (default this folder).

import { createRequire } from "node:module";
import { fileURLToPath } from "node:url";
import { dirname, join } from "node:path";
import { writeFileSync } from "node:fs";

const Here = dirname(fileURLToPath(import.meta.url));
const RepositoryRoot = join(Here, "..", "..");
const Require = createRequire(join(RepositoryRoot, "ExternalPackages", "CaptureTools", "package.json"));
const { chromium } = Require("playwright-core");
const ChromiumPackage = (await import(Require.resolve("@sparticuz/chromium"))).default;

const Origin = process.env.LOBBY_ORIGIN || "http://127.0.0.1:8080";
const OutputDirectory = process.env.OUTPUT_DIRECTORY || Here;
const LobbyUrl = `${Origin}/lobby/index.html?scene=studio&quality=low`;
const Results = [];
const StartedAt = Date.now();

function Record(Name, Expected, Observed) {
    const Pass = JSON.stringify(Expected) === JSON.stringify(Observed);
    Results.push({ Name, Expected, Observed, Pass });
    console.log(`${Pass ? "PASS" : "FAIL"}  ${Name}  expected=${JSON.stringify(Expected)}  observed=${JSON.stringify(Observed)}`);
    return Pass;
}

async function WaitUntil(Predicate, TimeoutMilliseconds, Label) {
    const Deadline = Date.now() + TimeoutMilliseconds;
    while (Date.now() < Deadline) {
        try {
            if (await Predicate()) return true;
        } catch {
            // The page may be navigating; keep polling until the deadline.
        }
        await new Promise((Resolve) => setTimeout(Resolve, 1000));
    }
    console.log(`TIMEOUT ${Label} after ${TimeoutMilliseconds} ms`);
    return false;
}

const Executable = await ChromiumPackage.executablePath();
const Browser = await chromium.launch({
    executablePath: Executable,
    env: { ...process.env,
           LD_LIBRARY_PATH: process.env.LD_LIBRARY_PATH || "/tmp/al2023/lib",
           VK_ICD_FILENAMES: process.env.VK_ICD_FILENAMES || "/tmp/vk_swiftshader_icd.json" },
    args: [
        ...ChromiumPackage.args.filter((Argument) => !["--single-process", "--in-process-gpu"].includes(Argument)),
        "--enable-unsafe-webgpu", "--use-webgpu-adapter=swiftshader", "--enable-features=Vulkan",
        "--use-vulkan=swiftshader", "--disable-vulkan-surface"
    ],
    headless: true
});

const Context = await Browser.newContext({ viewport: { width: 1440, height: 900 } });
const Member = await Context.newPage();
const MemberErrors = [];
Member.on("pageerror", (Error0) => MemberErrors.push(String(Error0).slice(0, 300)));
Member.on("console", (Message) => { if (Message.type() === "error") MemberErrors.push(Message.text().slice(0, 300)); });

const RosterCount = () => Member.evaluate(() => document.getElementById("lobby-count")?.textContent ?? "");
const StatusText = () => Member.evaluate(() => document.getElementById("lobby-status")?.textContent ?? "");

// (1) One member opens the lobby.
await Member.goto(LobbyUrl, { waitUntil: "domcontentloaded" });
const Booted = await WaitUntil(() => Member.evaluate(() => (window.IlluminationExhibit?.FrameIndex ?? 0) >= 3), 300000, "studio frames");
Record("studio renders frames", true, Booted);
await WaitUntil(() => Member.evaluate(() => window.IlluminationExhibit?.Cabin?.Appearance != null), 60000, "appearance");
Record("roster starts with one member", "1 player", await RosterCount());
await Member.screenshot({ path: join(OutputDirectory, "lobby-one-member.png"), timeout: 240000 });

// (2) A peer joins.
const Peer1 = await Context.newPage();
await Peer1.goto(`${Origin}/lobby/peer.html`, { waitUntil: "domcontentloaded" });
await WaitUntil(async () => (await RosterCount()) === "2 players", 15000, "peer join");
Record("peer join adds a member", "2 players", await RosterCount());
await Member.screenshot({ path: join(OutputDirectory, "lobby-two-members.png"), timeout: 240000 });

// (3) The peer leaves by closing its tab.
await Peer1.close();
await WaitUntil(async () => (await RosterCount()) === "1 player", 15000, "peer leave");
Record("peer leave removes the member", "1 player", await RosterCount());

// (4) A second peer joins (not ready). The member presses Ready and waits.
const Peer2 = await Context.newPage();
await Peer2.goto(`${Origin}/lobby/peer.html`, { waitUntil: "domcontentloaded" });
await WaitUntil(async () => (await RosterCount()) === "2 players", 15000, "second peer join");
// 📝 Buttons are clicked with element.click() inside the page: Playwright waits for animation frames, which the SwiftShader
//    render starves, so its actionability checks time out. The click still runs the page's own click handler.
await Member.evaluate(() => document.getElementById("lobby-ready").click());
await WaitUntil(async () => (await StatusText()).includes("Waiting"), 10000, "waiting status");
Record("ready waits for the other member", "Ready. Waiting for 1 more to press Ready.", await StatusText());
await Member.screenshot({ path: join(OutputDirectory, "lobby-waiting-for-ready.png"), timeout: 240000 });

// (5) The second peer presses Ready. Every member is ready, so the lobby travels to the next level.
await Peer2.evaluate(() => document.getElementById("peer-ready").click());
// 📝 waitForURL is event-driven: the next level compiles shaders on the main thread, which stalls evaluate() polling.
const Travelled = await Member.waitForURL((Url) => Url.pathname === "/index.html", { timeout: 120000 }).then(() => true, () => false);
Record("all ready moves the room to the next level", true, Travelled);
await Peer2.close();
await WaitUntil(() => Member.evaluate(() => (window.IlluminationExhibit?.FrameIndex ?? 0) >= 3), 300000, "level frames");
await Member.screenshot({ path: join(OutputDirectory, "next-level-after-ready.png"), timeout: 240000 });

const Summary = {
    Origin, StartedAt: new Date(StartedAt).toISOString(), DurationSeconds: Math.round((Date.now() - StartedAt) / 1000),
    Results, MemberErrors: MemberErrors.slice(0, 10),
    Passed: Results.filter((Entry) => Entry.Pass).length, Failed: Results.filter((Entry) => !Entry.Pass).length
};
writeFileSync(join(OutputDirectory, "capture-result.json"), JSON.stringify(Summary, null, 2));
console.log(`Passed ${Summary.Passed}, failed ${Summary.Failed}. Result: ${join(OutputDirectory, "capture-result.json")}`);
await Browser.close();
process.exit(Summary.Failed === 0 ? 0 : 1);
