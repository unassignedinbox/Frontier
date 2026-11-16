// P01 interaction evidence harness — drives every P01 control inside jsdom with a
// stubbed 2D context (no pixels) and records state transitions as evidence.
// Run: node Exhibits/P01Cards/P01InteractionHarness.mjs
import { readFileSync, writeFileSync } from "node:fs";
import { JSDOM } from "/tmp/smoke/node_modules/jsdom/lib/api.js";

const html = readFileSync("Experimental/ComponentViewer/Prototypes/P01-InfographicCards.html", "utf8");
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

// Bounded rAF: run each loop a few frames so sweep/evaluation advance, then stop.
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
    return new Proxy({}, {
      get: (t, k) => {
        if (k === "canvas") return {};
        if (typeof k === "string") return (...a) => undefined;
        return undefined;
      },
      set: () => true
    });
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

await sleep(400); // let sparkline ticks + bounded rAF frames run

check("page boots 6 cards", () => {
  const cards = document.querySelectorAll("#CardGrid .card");
  assert(cards.length === 6, `expected 6, saw ${cards.length}`);
  return "6 cards present";
});

check("StrataPlot tier select S4", () => {
  const steps = $("StrataSteps").children;
  assert(steps.length === 5, `expected 5 tiers, saw ${steps.length}`);
  click(steps[4]);
  assert($("StrataNum").textContent.includes("52"), $("StrataNum").textContent);
  assert($("StrataWrong").textContent === "1.90%", $("StrataWrong").textContent);
  assert($("StrataBar").style.width === "72%", $("StrataBar").style.width);
  return "S4 · 52 tris · 1.90% · bar 72%";
});

check("Surface-optics slot chips toggle", () => {
  const chips = document.querySelectorAll("#SlotChips .chip");
  click(chips[2]); click(chips[3]);
  const on = document.querySelectorAll("#SlotChips .chip.on").length;
  assert(on === 4, `expected 4 lit, saw ${on}`);
  click(chips[0]);
  assert(document.querySelectorAll("#SlotChips .chip.on").length === 3, "untoggle failed");
  return "4 lit, untoggle to 3";
});

check("PolarDial joint steppers ±2°", () => {
  const before = parseFloat($("JointAngle").textContent);
  click($("JointPlus")); click($("JointPlus"));
  const after = parseFloat($("JointAngle").textContent);
  assert(Math.abs(after - before - 4) < 1e-9, `${before} -> ${after}`);
  click($("JointMinus"));
  assert(Math.abs(parseFloat($("JointAngle").textContent) - before - 2) < 1e-9, "minus failed");
  return `${before}° -> +4° -> +2°`;
});

check("PolarDial sweep toggles drive label", () => {
  click($("JointSweep"));
  assert($("JointSweep").classList.contains("live"), "sweep did not engage");
  assert($("JointDrive").textContent === "sweep", $("JointDrive").textContent);
  click($("JointSweep"));
  assert($("JointDrive").textContent === "hold at pose", $("JointDrive").textContent);
  return "sweep engaged + released";
});

check("DiscRow gear select", () => {
  const gears = document.querySelectorAll("#GearRow button");
  click(gears[3]);
  assert($("ShakeGear").textContent === "4", $("ShakeGear").textContent);
  return "gear 4";
});

check("RampButton throttle hold ramps", async () => {
  const pad = $("PadThrottle");
  pad.dispatchEvent(new window.Event("pointerdown", { bubbles: true }));
  await sleep(350);
  const mid = $("ThrottleVal").textContent;
  window.dispatchEvent(new window.Event("pointerup"));
  const end = $("ThrottleVal").textContent;
  assert(mid !== "0%", `no ramp, stayed ${mid}`);
  assert(end === "0%", `no release, stuck ${end}`);
  return `ramped to ${mid}, released to ${end}`;
});

check("Compile assembly seals record", () => {
  click($("CompileBtn"));
  assert($("CompileNote").textContent.includes("revision 25"), $("CompileNote").textContent);
  return "revision 25 sealed";
});

check("Filter pills isolate families", () => {
  const pills = document.querySelectorAll("#FilterRow .pill");
  const visible = () => [...document.querySelectorAll("#CardGrid .card")]
    .filter((c) => c.style.display !== "none").length;
  click([...pills].find((p) => p.dataset.filter === "eval"));
  assert(visible() === 1, `eval shows ${visible()}`);
  click([...pills].find((p) => p.dataset.filter === "assembly"));
  assert(visible() === 2, `assembly shows ${visible()}`);
  click([...pills].find((p) => p.dataset.filter === "all"));
  assert(visible() === 6, `all shows ${visible()}`);
  return "eval 1 · assembly 2 · all 6";
});

// Await async checks (harness runs them inline above; this keeps ordering simple).
for (const r of results) {
  if (r.detail instanceof Promise) {
    try { r.detail = await r.detail; } catch (e) { r.verdict = "FAIL"; r.detail = String(e.message); }
  }
}

const passed = results.filter((r) => r.verdict === "PASS").length;
const report = [
  "# P01 Interaction Evidence",
  "",
  `- Harness: \`Exhibits/P01Cards/P01InteractionHarness.mjs\` (jsdom, stubbed 2D context, bounded rAF).`,
  `- Date (UTC): 2026-11-16. Result: **${passed}/${results.length} checks passed**.`,
  `- Rendered-pixel capture: BLOCKED — no headless browser in sandbox. Visual check = live preview + user approval vote.`,
  "",
  "| Check | Verdict | Observed |",
  "|---|---|---|",
  ...results.map((r) => `| ${r.name} | ${r.verdict === "PASS" ? "PASS" : "FAIL"} | ${r.detail} |`),
  ""
].join("\n");
writeFileSync("Exhibits/P01Cards/P01InteractionReport.md", report);
console.log(report);
if (passed !== results.length) process.exit(1);
