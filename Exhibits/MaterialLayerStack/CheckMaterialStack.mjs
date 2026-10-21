// Checks for MaterialStackModel.js — run: node CheckMaterialStack.mjs
// The claims worth defending: the optics identities (F0, Schlick, Beer), patterns stay in [0,1] and are
// deterministic, normalization clamps anything a hand-edited import can contain, and the preview shader
// never returns NaN/negative light for any shipped preset.
import assert from "node:assert/strict";
import {
  F0FromIOR, Schlick, Beer, PatternSample, LayerAt, ShadeComposite, Tonemap,
  NormalizeStack, NormalizeLayer, DefaultStack, PRESETS, ExportStack, ImportStack,
  CompositeReadout, StackSummary, NewLayer, norm3,
} from "./MaterialStackModel.js";

let checks = 0;
const ok = (cond, msg) => { assert(cond, msg); checks++; };
const near = (a, b, eps = 1e-6) => Math.abs(a - b) < eps;

// --- optics identities
ok(near(F0FromIOR([1.5, 1.5, 1.5], [0, 0, 0])[0], 0.04), "dielectric IOR 1.5 → F0 0.04");
ok(F0FromIOR([1.3, 1.3, 1.3], [3, 3, 3])[0] > 0.6, "conductor k raises F0 well above dielectric");
ok(F0FromIOR([1.4, 1.4, 1.4], [6, 6, 6])[0] > 0.85, "high-k metal (aluminium-like) F0 approaches 0.9");
ok(near(Schlick([0.04, 0.04, 0.04], 1)[0], 0.04), "Schlick at normal incidence = F0");
ok(near(Schlick([0.04, 0.04, 0.04], 0)[0], 1), "Schlick at grazing = 1");
ok(Beer([0.5, 0.25, 1], 2, 0).every((c) => near(c, 1)), "Beer at zero thickness transmits everything");
const b1 = Beer([0.5, 0.5, 0.5], 2, 1000), b2 = Beer([0.5, 0.5, 0.5], 2, 2000);
ok(b1[0] > b2[0] && b2[0] > 0, "Beer darkens monotonically with thickness");
ok(near(b1[0], Math.pow(0.5, 0.5), 1e-9), "Beer ratio is thickness/distance");

// --- patterns: deterministic and in range
for (const pattern of ["noise", "voronoi", "flakes", "checker", "stripes", "weave"]) {
  const proc = { pattern, scale: [7, 5], seed: 3, octaves: 3, contrast: 1.4, invert: false };
  for (let i = 0; i < 40; i++) {
    const u = (i * 0.137) % 1, v = (i * 0.291) % 1;
    const a = PatternSample(proc, u, v), b = PatternSample(proc, u, v);
    ok(a === b, `${pattern} deterministic`);
    ok(a >= 0 && a <= 1, `${pattern} in [0,1]`);
  }
}
ok(PatternSample({ pattern: "none", scale: [1, 1], seed: 1, octaves: 1, contrast: 1 }, 0.3, 0.7) === 1, "pattern none = flat 1");

// --- normalization refuses nonsense instead of propagating it
const junk = NormalizeStack({ name: 42, layers: [{ type: "wat", metallic: 99, ior: [0.2, "x", 99], normal: [9, 9, 9], proc: { pattern: "plasma", seed: -5 } }] });
ok(junk.layers.length === 1 && junk.layers[0].metallic === 1, "metallic clamped");
ok(junk.layers[0].ior[0] === 1 && junk.layers[0].ior[2] === 4, "IOR clamped to [1,4]");
ok(junk.layers[0].proc.pattern === "none", "unknown pattern → none");
ok(NormalizeStack({}).layers.length >= 1, "empty input still yields a stack");
ok(NormalizeStack({ layers: [] }).layers.length >= 1, "zero layers refused");
const many = NormalizeStack({ layers: Array.from({ length: 40 }, () => ({})) });
ok(many.layers.length === 16, "layer count capped at 16");

// --- every preset shades to finite, non-negative light
const L = norm3([-0.45, 0.65, 0.62]);
for (const [key, preset] of Object.entries(PRESETS)) {
  const s = preset.make();
  for (const [u, v] of [[0.2, 0.3], [0.5, 0.5], [0.83, 0.17]]) {
    const rgb = ShadeComposite(s, { u, v, N: [0.2, 0.3, 0.93], L, V: [0, 0, 1], bg: [0.4, 0.4, 0.4] });
    ok(rgb.every((c) => Number.isFinite(c) && c >= 0), `${key} shades finite & ≥0`);
    ok(Tonemap(rgb).every((c) => c >= 0 && c <= 1), `${key} tonemaps into [0,1]`);
  }
  const eff = LayerAt(s.layers[0], 0.37, 0.62);
  ok(eff.coverage >= 0 && eff.coverage <= 1 && eff.thicknessUm >= 0, `${key} LayerAt sane`);
  const r = CompositeReadout(s);
  ok(r.totalThicknessUm > 0 && r.grade.startsWith("A") || r.grade.startsWith("B") || r.grade.startsWith("C") || r.grade.startsWith("D"), `${key} readout`);
}

// --- warnings catch the classic mistakes
const bad = NormalizeStack({ name: "bad", layers: [
  { ...NewLayer("diffuse", "Opaque lid"), coverage: 1, transmission: [0, 0, 0] },
  { ...NewLayer("transmission", "Hidden glass") },
  { ...NewLayer("metal", "Confused conductor"), transmission: [0.9, 0.9, 0.9] },
]});
const w = CompositeReadout(bad).warnings.join(" | ");
ok(/never be seen/.test(w), "warns: transmissive under opaque");
ok(/conductors don't transmit/.test(w), "warns: metallic + transmission");

// --- export/import round trip
const round = ImportStack(ExportStack(DefaultStack()));
ok(JSON.stringify(round.layers.map((l) => l.name)) === JSON.stringify(DefaultStack().layers.map((l) => l.name)), "round-trip keeps slab order/names");
assert.throws(() => ImportStack("{nope"), /Not valid JSON/); checks++;
assert.throws(() => ImportStack('{"layers": 3}'), /no layers/); checks++;
ok(/BASE/.test(StackSummary(DefaultStack())), "summary marks the base slab");
const single = NormalizeLayer({ type: "coat" });
ok(single.type === "coat" && single.ior[0] === 1.46, "template defaults survive normalization");

console.log(`PASS ${checks} material-stack checks`);
