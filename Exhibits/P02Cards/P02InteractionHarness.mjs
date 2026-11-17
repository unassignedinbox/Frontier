// P02 interaction evidence harness — drives every P02 control inside jsdom with a
// stubbed 2D context (no pixels) and records state transitions as evidence.
// Run: node Exhibits/P02Cards/P02InteractionHarness.mjs
import { readFileSync, writeFileSync } from "node:fs";
import { JSDOM } from "/tmp/smoke/node_modules/jsdom/lib/api.js";

const html = readFileSync("Experimental/Prototypes/P02-QuietInstruments.html", "utf8");
const results = [];
function check(name, fn) {
  try {
    const detail = fn();
    results.push({ name, verdict: "PASS", detail });
  } catch (e) {
    results.push({ name, verdict: "FAIL", detail: String(e && e.message || e) });
  }
}
function assert(cond, msg) { if (!cond) throw new Error(msg); }

let rafBudget = 40;
function makeRaf(window) {
  return (cb) => {
    if (rafBudget <= 0) return 0;
    rafBudget -= 1;
    return window.setTimeout(() => cb(performance.now()), 0);
  };
}
function stubCanvas(window) {
  window.HTMLCanvasElement.prototype.getContext = function () {
    return new Proxy({}, { get: () => (...a) => undefined, set: () => true });
  };
}

const dom = new JSDOM(html, {
  runScripts: "dangerously",
  pretendToBeVisual: true,
  beforeParse(window) {
    stubCanvas(window);
    window.requestAnimationFrame = makeRaf(window);
  }
});
const { window } = dom;
const { document } = window;
const $ = (id) => document.getElementById(id);
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));
const click = (el) => el.dispatchEvent(new window.MouseEvent("click", { bubbles: true }));
const pointer = (el, type, x, y) =>
  el.dispatchEvent(new window.MouseEvent(type, { bubbles: true, clientX: x, clientY: y }));

await sleep(300);

check("page boots 4 cards + compile strip", () => {
  const cards = document.querySelectorAll("#CardGrid .card");
  assert(cards.length === 4, `expected 4, saw ${cards.length}`);
  assert($("CompileBtn") !== null, "compile strip missing");
  return "4 cards + strip present";
});

check("tier rail S4 retunes both cards", () => {
  const stops = document.querySelectorAll("#TierRail .stop");
  assert(stops.length === 5, `expected 5 stops, saw ${stops.length}`);
  click(stops[4]);
  assert($("TierNum").textContent === "52", $("TierNum").textContent);
  assert($("TierWrong").textContent === "1.90%", $("TierWrong").textContent);
  assert($("ComplexNum").textContent === "52", "cross-card link failed: " + $("ComplexNum").textContent);
  return "S4 · 52 tris · wrong 1.90% · complex follows";
});

check("tier rail arrow key steps back", () => {
  $("TierRail").dispatchEvent(new window.KeyboardEvent("keydown", { key: "ArrowUp", bubbles: true }));
  assert($("TierRead").textContent === "S3", $("TierRead").textContent);
  return "ArrowUp S4 -> S3";
});

check("history scrubber drags", () => {
  pointer($("ComplexPlot"), "pointerdown", 60, 10);
  pointer(window, "pointermove", 60, 10);
  window.dispatchEvent(new window.MouseEvent("pointerup", { bubbles: true }));
  assert($("ScrubRead").textContent.startsWith("T-"), $("ScrubRead").textContent);
  return $("ScrubRead").textContent;
});

check("dial needle drags to limit", () => {
  pointer($("JointDial"), "pointerdown", 170, 0);
  window.dispatchEvent(new window.MouseEvent("pointerup", { bubbles: true }));
  assert($("JointAngle").textContent === "32.0", $("JointAngle").textContent);
  assert($("JointLoad").textContent === "1.2 kN", $("JointLoad").textContent);
  return "drag -> 32.0° · 1.2 kN";
});

check("dial double-tap centres", () => {
  $("JointDial").dispatchEvent(new window.MouseEvent("dblclick", { bubbles: true }));
  assert($("JointAngle").textContent === "0.0", $("JointAngle").textContent);
  return "0.0°";
});

check("sweep toggles drive label", () => {
  click($("JointSweep"));
  assert($("JointDrive").textContent === "sweep", $("JointDrive").textContent);
  click($("JointSweep"));
  assert($("JointDrive").textContent === "hold at pose", $("JointDrive").textContent);
  return "sweep on/off";
});

check("throttle strip pull sets value", () => {
  pointer($("ThrottleStrip"), "pointerdown", 10, 30);
  window.dispatchEvent(new window.MouseEvent("pointerup", { bubbles: true }));
  assert($("ThrottleVal").textContent === "80%", $("ThrottleVal").textContent);
  return "pull -> 80%";
});

check("gear DiscRow selects", () => {
  const gears = document.querySelectorAll("#GearRow button");
  click(gears[3]);
  assert(gears[3].classList.contains("on"), "gear 4 not lit");
  return "gear 4";
});

check("card headers fold/unfold", () => {
  const card = $("CardTiers");
  click(card.querySelector(".card-head"));
  assert(card.classList.contains("open"), "did not unfold");
  click(card.querySelector(".card-head"));
  assert(!card.classList.contains("open"), "did not fold");
  return "fold/unfold ok";
});

check("optics chips retune count", () => {
  const chips = document.querySelectorAll("#OpticsChips .chip");
  click(chips[2]); click(chips[3]);
  assert($("OpticsCount").textContent === "4 of 4 lit", $("OpticsCount").textContent);
  return "4 of 4 lit";
});

check("compile strip seals revision 25", () => {
  click($("CompileBtn"));
  assert($("RevNum").textContent === "25", $("RevNum").textContent);
  return "revision 25";
});

check("filter pills isolate families", () => {
  const pills = document.querySelectorAll("#FilterRow .pill[data-filter]");
  const visible = () => [...document.querySelectorAll("#CardGrid .card")]
    .filter((c) => c.style.display !== "none").length;
  click([...pills].find((p) => p.dataset.filter === "eval"));
  assert(visible() === 1, `eval shows ${visible()}`);
  click([...pills].find((p) => p.dataset.filter === "asset"));
  assert(visible() === 2, `asset shows ${visible()}`);
  click([...pills].find((p) => p.dataset.filter === "all"));
  assert(visible() === 4, `all shows ${visible()}`);
  return "eval 1 · asset 2 · all 4";
});

const passed = results.filter((r) => r.verdict === "PASS").length;
const report = [
  "# P02 Interaction Evidence",
  "",
  `- Harness: \`Exhibits/P02Cards/P02InteractionHarness.mjs\` (jsdom, stubbed 2D context, bounded rAF).`,
  `- Date (UTC): 2026-11-16. Result: **${passed}/${results.length} checks passed**.`,
  `- Rendered-pixel capture: BLOCKED — no headless browser in sandbox. Visual check = live preview + user approval vote.`,
  "",
  "| Check | Verdict | Observed |",
  "|---|---|---|",
  ...results.map((r) => `| ${r.name} | ${r.verdict === "PASS" ? "PASS" : "FAIL"} | ${r.detail} |`),
  ""
].join("\n");
writeFileSync("Exhibits/P02Cards/P02InteractionReport.md", report);
console.log(report);
if (passed !== results.length) process.exit(1);
