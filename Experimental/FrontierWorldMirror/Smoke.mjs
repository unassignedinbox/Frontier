//==========================================================================================================================================
//                                                          SMOKE.MJS
//==========================================================================================================================================
// Live-bundle smoke for Experimental/FrontierWorldMirror: boots the built index.html in jsdom (WebGL mocked away so the
// zero-WebGL CpuGtxRenderer path takes over) and asserts the default Frontier Lobby scene, its exhibits, the physical
// environment rows and the Editor Camera component rows all mount with zero console errors.
//
//   npm install --prefix /tmp/smoke jsdom@25
//   node Experimental/FrontierWorldMirror/Build.mjs
//   JSDOM_DIR=/tmp/smoke node Experimental/FrontierWorldMirror/Smoke.mjs

import fs from "node:fs";
import path from "node:path";
import { createRequire } from "node:module";
import { fileURLToPath } from "node:url";

const Folder = path.dirname(fileURLToPath(import.meta.url));
const Require = createRequire(
  path.resolve(process.env.JSDOM_DIR || process.cwd()) + "/",
);
const { JSDOM, VirtualConsole } = Require("jsdom");

const Html = fs.readFileSync(path.join(Folder, "index.html"), "utf8");
const Errors = [];
const Console = new VirtualConsole();
Console.on("jsdomError", (Event) => {
  const Message = String(Event?.message || Event);
  if (!/Not implemented|Could not parse CSS/.test(Message)) Errors.push("jsdomError: " + Message);
});
Console.on("error", (...Args) =>
  Errors.push(
    "console.error: " + Args.map((Item) => (Item && Item.stack) || String(Item)).join(" "),
  ),
);

const Dom = new JSDOM(Html, {
  url: "http://localhost:8098/",
  runScripts: "dangerously",
  pretendToBeVisual: true,
  virtualConsole: Console,
});
const Window = Dom.window;
Window.ResizeObserver = class {
  observe() {}
  unobserve() {}
  disconnect() {}
};
if (!Window.matchMedia)
  Window.matchMedia = () => ({
    matches: false,
    addListener() {},
    removeListener() {},
    addEventListener() {},
    removeEventListener() {},
  });
// Small layout keeps the CPU GTX fallback frame budget low while the smoke runs.
Object.defineProperty(Window.HTMLElement.prototype, "clientWidth", { get: () => 320 });
Object.defineProperty(Window.HTMLElement.prototype, "clientHeight", { get: () => 180 });

await new Promise((Resolve) => setTimeout(Resolve, 9000));

const Document = Window.document;
const Body = Document.body.textContent || "";
const Outliner = Document.querySelector(".outliner")?.textContent || Document.body.textContent || "";
const NativeIconNames = new Set(
  [...Document.querySelectorAll("[data-native-icon]")].map((Element) =>
    Element.getAttribute("data-native-icon"),
  ),
);
const Checks = [
  ["Editor mounts", Boolean(Document.querySelector("#Editor")?.childElementCount)],
  ["Default project is Frontier Lobby", Body.includes("Frontier Lobby")],
  ["ControlVehicle outliner row", Outliner.includes("ControlVehicle")],
  ["Atrium Display Aircraft outliner row", Outliner.includes("Atrium Display Aircraft")],
  ["Lobby Travertine Floor outliner row", Outliner.includes("Lobby Travertine Floor")],
  ["Lobby Glazed Portal outliner row", Outliner.includes("Lobby Glazed Portal")],
  ["Sun environment row", Outliner.includes("Sun")],
  ["Volumetric Clouds environment row", Outliner.includes("Volumetric Clouds")],
  ["Editor Camera Lens Flare component", Outliner.includes("Lens Flare")],
  ["Editor Camera Tonemapping component", Outliner.includes("Tonemapping")],
  ["Editor Camera Render Settings component", Outliner.includes("Render Settings")],
  ["Atmosphere row uses the native sky-scattering icon", NativeIconNames.has("sky-scattering")],
  ["Volumetric Clouds row uses the native clouds icon", NativeIconNames.has("clouds")],
  ["Cone Slalom template retired", !Body.includes("Cone Slalom")],
];

// The geometry Overview tab is a quick-tile dashboard (chips + counters + a readiness
// score) instead of the retired row-by-row "Object overview" card.
const RowFor = (Name) =>
  [...Document.querySelectorAll(".outliner-row")].find((Element) =>
    (Element.textContent || "").includes(Name),
  );
RowFor("Lobby Travertine Floor")?.click();
await new Promise((Resolve) => setTimeout(Resolve, 400));
Document.querySelector('.insp-segment[aria-controls="insp-page-overview"]')?.click();
await new Promise((Resolve) => setTimeout(Resolve, 400));
const OverviewPage = Document.querySelector("#insp-page-overview");
Checks.push(
  ["Geometry Overview tab renders as quick tiles", Document.querySelectorAll("[data-asset-chip]").length >= 4],
  ["Geometry Overview tab shows readiness counters", Document.querySelectorAll("[data-asset-counter]").length === 2],
  ["Geometry Overview tab shows the readiness score", Boolean(Document.querySelector("[data-asset-score]"))],
  ["Object overview card retired", !(OverviewPage?.textContent || "").includes("Object overview")],
);

let Failures = 0;
for (const [Name, Passed] of Checks) {
  if (!Passed) Failures += 1;
  console.log(`${Passed ? "PASS" : "FAIL"}  ${Name}`);
}
for (const Error of Errors.slice(0, 8)) console.log("ERR   " + Error);
console.log(
  Failures === 0 && Errors.length === 0
    ? "[Smoke] ALL CHECKS PASSED — Frontier Lobby default scene boots with zero console errors"
    : `[Smoke] FAILED (${Failures} checks, ${Errors.length} console errors)`,
);
process.exit(Failures === 0 && Errors.length === 0 ? 0 : 1);
