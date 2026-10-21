//============================================================================================================================================
//                                                        MATERIALLAYERSTACK.JS
//============================================================================================================================================
// 🎛️ The card wall. Every control is the editor's own — slider-pill + split-value, toggle, axes (XYZ),
//    colour-field, readout, segmented mode — so a slab reads as a sibling of the Fracture/Gas cards.
//    All material math lives in MaterialStackModel.js; this file only renders state and writes it back.

import {
  SCHEMA, SAVE_KEY, LAYER_TYPES, TYPE_ORDER, PRESETS, PRESET_ORDER,
  NewLayer, DefaultStack, NormalizeStack, EffectiveF0, PatternSample,
  ShadeComposite, Tonemap, CompositeReadout, StackSummary, ExportStack, ImportStack,
  clamp, norm3,
} from "./MaterialStackModel.js";

const $ = (s) => document.querySelector(s);
function el(tag, attrs = {}, ...kids) {
  const node = document.createElement(tag);
  for (const [k, v] of Object.entries(attrs)) {
    if (v === undefined || v === null || v === false) continue;
    if (k === "class") node.className = v;
    else if (k === "text") node.textContent = v;
    else if (k.startsWith("on")) node.addEventListener(k.slice(2), v);
    else if (k === "value") node.value = v;
    else node.setAttribute(k, v === true ? "" : v);
  }
  for (const kid of kids.flat()) {
    if (kid === null || kid === undefined) continue;
    node.append(kid.nodeType ? kid : document.createTextNode(String(kid)));
  }
  return node;
}
const fmt = (v, d = 2) => (Math.round(v * 10 ** d) / 10 ** d).toFixed(d);
const rgbCss = (c) => `rgb(${c.map((x) => Math.round(clamp(x, 0, 1) * 255)).join(",")})`;
const rgbHex = (c) => "#" + c.map((x) => Math.round(clamp(x, 0, 1) * 255).toString(16).padStart(2, "0")).join("").toUpperCase();
const hexRgb = (h) => [1, 3, 5].map((i) => parseInt(h.slice(i, i + 2), 16) / 255);
const escapeHtml = (s) => String(s).replace(/[&<>"]/g, (c) => ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;" }[c]));
function thicknessLabel(um) {
  if (um < 1) return fmt(um * 1000, 0) + " nm";
  if (um < 1000) return fmt(um, um < 10 ? 1 : 0) + " µm";
  return fmt(um / 1000, 2) + " mm";
}

// ------------------------------------------------------------------ state
let stack = loadInitial();
let previewMode = "sphere";
let activePreset = "example";
const openSections = {};   // `${layerId}:${section}` -> bool (details state survives re-render)
const collapsedCards = new Set();
function loadInitial() {
  try {
    const saved = localStorage.getItem(SAVE_KEY);
    if (saved) { activePreset = null; return ImportStack(saved); }
  } catch { /* fall through to the default stack */ }
  return DefaultStack();
}
let saveTimer = 0, previewTimer = 0;
function autosave() {
  clearTimeout(saveTimer);
  saveTimer = setTimeout(() => {
    try { localStorage.setItem(SAVE_KEY, ExportStack(stack)); } catch { /* storage full/blocked */ }
  }, 250);
}
function scheduleComposite() {
  clearTimeout(previewTimer);
  previewTimer = setTimeout(renderComposite, 80);
}
function changed(layer) {
  if (layer) refreshLayerOutputs(layer);
  scheduleComposite();
  renderGraph();
  autosave();
}

// ------------------------------------------------------------------ field builders (editor vocabulary)
function setFill(range) {
  const min = parseFloat(range.min), max = parseFloat(range.max), v = parseFloat(range.value);
  range.style.setProperty("--fill", clamp(((v - min) / (max - min)) * 100, 0, 100) + "%");
}
/** .field > span + .slider-pill > range + .split-value(number + unit) */
function sliderField(label, get, set, { min = 0, max = 1, step = 0.01, unit = "", digits = 2 } = {}) {
  const number = el("input", { type: "number", step, value: fmt(get(), digits), "aria-label": label + " value" });
  const range = el("input", { type: "range", min, max, step, value: get(), "aria-label": label });
  setFill(range);
  range.addEventListener("input", () => {
    const v = parseFloat(range.value); setFill(range); number.value = fmt(v, digits); set(v);
  });
  number.addEventListener("change", () => {
    const v = clamp(parseFloat(number.value) || 0, min, max);
    number.value = fmt(v, digits); range.value = v; setFill(range); set(v);
  });
  return el("label", { class: "field" },
    el("span", { text: label }),
    el("span", { class: "slider-pill" }, range,
      el("span", { class: "split-value" }, number, el("small", { text: unit || "·" }))));
}
/** .axes — XYZ (or RGB / XY) triple with the editor's axis colours. */
function axesField(label, get, set, { labels = ["X", "Y", "Z"], min = -4, max = 4, step = 0.01 } = {}) {
  const current = get();
  const inputs = labels.map((axis, i) => {
    const input = el("input", { type: "number", step, value: fmt(current[i], 3), "aria-label": `${label} ${axis}` });
    input.addEventListener("change", () => {
      const next = get().slice();
      next[i] = clamp(parseFloat(input.value) || 0, min, max);
      input.value = fmt(next[i], 3); set(next);
    });
    return el("label", {}, el("b", { text: axis }), input);
  });
  return el("div", { class: "field" }, el("span", { text: label }), el("div", { class: "axes" }, inputs));
}
function colourField(label, get, set) {
  const output = el("output", { text: rgbHex(get()) });
  const swatch = el("input", { type: "color", value: rgbHex(get()), "aria-label": label });
  swatch.addEventListener("input", () => { output.textContent = swatch.value.toUpperCase(); set(hexRgb(swatch.value)); });
  return el("div", { class: "colour-field" }, el("span", { text: label }), output, swatch);
}
function switchRow(label, get, set) {
  const btn = el("button", {
    class: "toggle" + (get() ? " on" : ""), role: "switch", "aria-checked": String(!!get()), "aria-label": label,
    onclick: () => { const v = !get(); btn.className = "toggle" + (v ? " on" : ""); btn.setAttribute("aria-checked", String(v)); set(v); },
  }, el("i"));
  return el("label", { class: "switch-row" }, el("span", { text: label }), btn);
}
function segmented(label, options, get, set) {
  const group = el("div", { class: "segmented", role: "group", "aria-label": label });
  for (const [value, text] of options) {
    const b = el("button", {
      text, "aria-pressed": String(get() === value),
      onclick: () => {
        set(value);
        group.querySelectorAll("button").forEach((x) => x.setAttribute("aria-pressed", String(x === b)));
      },
    });
    group.append(b);
  }
  return el("div", { class: "field" }, el("span", { text: label }), group);
}
function selectField(label, options, get, set) {
  const select = el("select", { "aria-label": label });
  for (const [value, text] of options) select.append(el("option", { value, text, ...(get() === value ? { selected: true } : {}) }));
  select.addEventListener("change", () => set(select.value));
  return el("label", { class: "field" }, el("span", { text: label }), select);
}
function readout(label, value) {
  return el("div", { class: "readout" }, el("span", { text: label }), el("output", { text: value }));
}

// ------------------------------------------------------------------ layer card
function section(layer, key, title, children, openDefault = false) {
  const id = layer.id + ":" + key;
  const d = el("details", { class: "subsection", ...(openSections[id] ?? openDefault ? { open: true } : {}) },
    el("summary", { text: title }), children);
  d.addEventListener("toggle", () => { openSections[id] = d.open; });
  return d;
}
const PATTERN_OPTIONS = [
  ["none", "None — flat values"], ["noise", "Value noise (fBm)"], ["voronoi", "Voronoi distance"],
  ["flakes", "Flakes (Voronoi cells)"], ["checker", "Checker"], ["stripes", "Stripes"], ["weave", "Weave 2×2"],
];
function layerCard(layer, index) {
  const isBase = index === stack.layers.length - 1;
  const isOuter = index === 0;
  const set = (fn) => (v) => { fn(v); changed(layer); };
  const card = el("section", { class: "generic-card layer-card" + (layer.enabled ? "" : " disabled"), "data-layer-id": layer.id });

  const nameInput = el("input", { class: "layer-name", type: "text", value: layer.name, maxlength: 48, "aria-label": "Layer name" });
  nameInput.addEventListener("input", () => { layer.name = nameInput.value; renderGraph(); autosave(); });
  const typeSelect = el("select", { "aria-label": "Layer type (template)", style: "width:auto;font-size:11px;padding:2px 6px;border-radius:8px" });
  for (const key of Object.keys(LAYER_TYPES))
    typeSelect.append(el("option", { value: key, text: LAYER_TYPES[key].label, ...(layer.type === key ? { selected: true } : {}) }));
  typeSelect.addEventListener("change", () => {
    const keep = { id: layer.id, name: layer.name, blend: layer.blend, coverage: layer.coverage, proc: layer.proc };
    Object.assign(layer, LAYER_TYPES[typeSelect.value].make(), keep, { type: typeSelect.value });
    renderStack(); changed(layer);
  });
  const enableBtn = el("button", {
    class: "toggle" + (layer.enabled ? " on" : ""), role: "switch", "aria-checked": String(layer.enabled),
    "aria-label": "Enable " + layer.name, title: "Enable layer",
    onclick: () => { layer.enabled = !layer.enabled; renderStack(); changed(layer); },
  }, el("i"));
  const head = el("header", {},
    el("div", { class: "layer-title" },
      el("div", { class: "layer-glyph", text: LAYER_TYPES[layer.type].glyph }),
      el("div", { style: "min-width:0;flex:1" },
        el("h3", {}, nameInput),
        el("small", {}, `SLAB ${index + 1} / ${stack.layers.length}`, el("span", { class: "pos-chip", text: isBase && isOuter ? "ONLY LAYER" : isOuter ? "OUTER · FIRST HIT" : isBase ? "BASE SUBSTRATE" : "BURIED FILM" })),
        el("div", { style: "margin-top:5px" }, typeSelect))),
    el("div", { class: "layer-actions" },
      el("button", { class: "icon-btn", text: "↑", title: "Move outward (up)", disabled: isOuter || undefined, onclick: () => moveLayer(index, -1) }),
      el("button", { class: "icon-btn", text: "↓", title: "Move inward (down)", disabled: isBase || undefined, onclick: () => moveLayer(index, 1) }),
      el("button", { class: "icon-btn", text: "⧉", title: "Duplicate slab", onclick: () => duplicateLayer(index) }),
      el("button", { class: "icon-btn", text: collapsedCards.has(layer.id) ? "+" : "−", title: "Collapse card", onclick: (e) => {
          collapsedCards.has(layer.id) ? collapsedCards.delete(layer.id) : collapsedCards.add(layer.id); renderStack();
        } }),
      el("button", { class: "icon-btn", text: "✕", title: "Delete slab", disabled: stack.layers.length <= 1 || undefined, onclick: () => { stack.layers.splice(index, 1); renderStack(); changed(null); } }),
      enableBtn));

  const summary = el("div", { class: "layer-summary", "data-role": "summary" });
  card.append(head, summary);
  if (collapsedCards.has(layer.id)) { refreshSummary(layer, summary); return card; }

  const body = el("div", {});
  body.append(
    section(layer, "stacking", "Stacking · how this slab sits on the one below", [
      segmented("Blend onto stack below", [["coat", "Coat (physical)"], ["mix", "Mix (horizontal)"], ["add", "Add (glow/film)"]],
        () => layer.blend, set((v) => (layer.blend = v))),
      sliderField("Blend weight (Mix/Add)", () => layer.weight, set((v) => (layer.weight = v)), { unit: "×" }),
      sliderField("Coverage", () => layer.coverage, set((v) => (layer.coverage = v)), { unit: "α" }),
      sliderField("Film thickness", () => layer.thicknessUm, set((v) => (layer.thicknessUm = v)), { min: 0, max: 5000, step: 0.5, unit: "µm", digits: 1 }),
      el("p", { "data-role": "thickness-readout" }),
    ], true),

    section(layer, "surface", "Surface · colour / metal / roughness", [
      colourField("Base colour", () => layer.colour, set((v) => (layer.colour = v))),
      colourField("Procedural colour B", () => layer.colourB, set((v) => (layer.colourB = v))),
      sliderField("Metallic", () => layer.metallic, set((v) => (layer.metallic = v)), { unit: "met" }),
      sliderField("Roughness", () => layer.roughness, set((v) => (layer.roughness = v)), { unit: "rgh" }),
      sliderField("Anisotropy (−1 vertical · +1 horizontal)", () => layer.anisotropy, set((v) => (layer.anisotropy = v)), { min: -1, unit: "an" }),
      sliderField("Specular intensity", () => layer.specular, set((v) => (layer.specular = v)), { max: 2, unit: "×" }),
    ], true),

    section(layer, "optics", "Optics · IOR / extinction / Fresnel", [
      axesField("IOR n — XYZ (per-channel R/G/B)", () => layer.ior, set((v) => (layer.ior = v)), { step: 0.01 }),
      axesField("Extinction k — XYZ (conductors)", () => layer.extinction, set((v) => (layer.extinction = v)), { step: 0.05 }),
      segmented("F0 source", [["auto", "Auto from IOR / metallic"], ["manual", "Manual F0 XYZ"]],
        () => layer.f0Mode, (v) => { layer.f0Mode = v; renderStack(); changed(layer); }),
      ...(layer.f0Mode === "manual"
        ? [axesField("F0 — XYZ reflectance at 0°", () => layer.f0, set((v) => (layer.f0 = v)), { step: 0.005 })]
        : [el("div", { "data-role": "f0-readout" })]),
      sliderField("Fresnel power (Schlick exponent)", () => layer.fresnelPower, set((v) => (layer.fresnelPower = v)), { min: 1, max: 10, step: 0.1, unit: "exp", digits: 1 }),
    ]),

    section(layer, "normals", "Normals · macro + micro", [
      axesField("Normal — XYZ (tangent space)", () => layer.normal, set((v) => (layer.normal = v)), { step: 0.01 }),
      el("div", { class: "row-btns" }, el("button", { text: "Normalize normal", onclick: () => { layer.normal = norm3(layer.normal); renderStack(); changed(layer); } })),
      sliderField("Normal strength", () => layer.normalStrength, set((v) => (layer.normalStrength = v)), { max: 3, unit: "×" }),
      axesField("Micro normal — XYZ", () => layer.microNormal, set((v) => (layer.microNormal = v)), { step: 0.01 }),
      sliderField("Micro strength", () => layer.microStrength, set((v) => (layer.microStrength = v)), { max: 2, unit: "×" }),
      sliderField("Micro scale (detail frequency)", () => layer.microScale, set((v) => (layer.microScale = v)), { min: 1, max: 256, step: 1, unit: "/uv", digits: 0 }),
    ]),

    section(layer, "volume", "Transmission · volume · sheen · film · emission", [
      axesField("Transmission — XYZ amount per channel", () => layer.transmission, set((v) => (layer.transmission = v)), { step: 0.01 }),
      colourField("Attenuation colour (Beer)", () => layer.attenuation, set((v) => (layer.attenuation = v))),
      sliderField("Attenuation distance", () => layer.attenuationDistanceMm, set((v) => (layer.attenuationDistanceMm = v)), { min: 0.05, max: 100, step: 0.05, unit: "mm" }),
      sliderField("Subsurface amount", () => layer.subsurface, set((v) => (layer.subsurface = v)), { unit: "sss" }),
      axesField("Scatter radius — XYZ mean-free-path (mm)", () => layer.scatterRadiusMm, set((v) => (layer.scatterRadiusMm = v)), { step: 0.05 }),
      sliderField("Sheen amount", () => layer.sheen, set((v) => (layer.sheen = v)), { unit: "shn" }),
      colourField("Sheen colour", () => layer.sheenColour, set((v) => (layer.sheenColour = v))),
      sliderField("Sheen roughness", () => layer.sheenRoughness, set((v) => (layer.sheenRoughness = v)), { unit: "rgh" }),
      sliderField("Thin-film amount", () => layer.thinFilm, set((v) => (layer.thinFilm = v)), { unit: "film" }),
      sliderField("Thin-film thickness", () => layer.thinFilmThicknessNm, set((v) => (layer.thinFilmThicknessNm = v)), { min: 0, max: 1500, step: 1, unit: "nm", digits: 0 }),
      sliderField("Thin-film IOR", () => layer.thinFilmIor, set((v) => (layer.thinFilmIor = v)), { min: 1, max: 3, step: 0.01, unit: "n" }),
      colourField("Emission colour", () => layer.emission, set((v) => (layer.emission = v))),
      sliderField("Emission intensity", () => layer.emissionIntensity, set((v) => (layer.emissionIntensity = v)), { max: 16, step: 0.1, unit: "EV", digits: 1 }),
    ]),

    section(layer, "procedural", "Procedural · generated in-shader, no textures", [
      el("div", { class: "proc-visual" }, el("canvas", { width: 220, height: 110, "data-role": "proc-canvas", "aria-label": "Procedural pattern preview" })),
      selectField("Pattern", PATTERN_OPTIONS, () => layer.proc.pattern, set((v) => (layer.proc.pattern = v))),
      axesField("Pattern scale — XY tiles per UV", () => [layer.proc.scale[0], layer.proc.scale[1]], (v) => { layer.proc.scale = [v[0], v[1]]; changed(layer); }, { labels: ["X", "Y"], step: 0.25 }),
      sliderField("Seed", () => layer.proc.seed, set((v) => (layer.proc.seed = Math.round(v))), { min: 0, max: 99, step: 1, unit: "#", digits: 0 }),
      sliderField("Octaves (noise)", () => layer.proc.octaves, set((v) => (layer.proc.octaves = Math.round(v))), { min: 1, max: 6, step: 1, unit: "oct", digits: 0 }),
      sliderField("Contrast", () => layer.proc.contrast, set((v) => (layer.proc.contrast = v)), { max: 4, unit: "×" }),
      switchRow("Invert pattern", () => layer.proc.invert, set((v) => (layer.proc.invert = v))),
      sliderField("Affects → Coverage mask", () => layer.proc.affects.coverage, set((v) => (layer.proc.affects.coverage = v)), { unit: "→α" }),
      sliderField("Affects → Colour A→B mix", () => layer.proc.affects.colour, set((v) => (layer.proc.affects.colour = v)), { unit: "→col" }),
      sliderField("Affects → Roughness", () => layer.proc.affects.roughness, set((v) => (layer.proc.affects.roughness = v)), { unit: "→rgh" }),
      sliderField("Affects → Normal (bump)", () => layer.proc.affects.normal, set((v) => (layer.proc.affects.normal = v)), { unit: "→n" }),
      sliderField("Affects → Thickness", () => layer.proc.affects.thickness, set((v) => (layer.proc.affects.thickness = v)), { unit: "→µm" }),
    ], true),

    el("div", { class: "card-foot", "data-role": "foot" }),
  );
  card.append(body);
  refreshLayerOutputs(layer, card);
  return card;
}
function refreshSummary(layer, node) {
  const f0 = EffectiveF0(layer);
  node.replaceChildren(
    el("span", {}, el("span", { class: "swatch-dot", style: `background:${rgbCss(layer.colour)}` }), rgbHex(layer.colour)),
    el("span", {}, "F0 ", el("b", { text: f0.map((c) => fmt(c)).join(" / ") })),
    el("span", {}, "rough ", el("b", { text: fmt(layer.roughness) })),
    el("span", {}, "metal ", el("b", { text: fmt(layer.metallic) })),
    el("span", {}, "thick ", el("b", { text: thicknessLabel(layer.thicknessUm) })),
    el("span", {}, layer.proc.pattern !== "none" ? `proc ${layer.proc.pattern}` : "flat"),
  );
}
function refreshLayerOutputs(layer, cardEl) {
  const card = cardEl || document.querySelector(`[data-layer-id="${layer.id}"]`);
  if (!card) return;
  const summary = card.querySelector('[data-role="summary"]');
  if (summary) refreshSummary(layer, summary);
  const thick = card.querySelector('[data-role="thickness-readout"]');
  if (thick) thick.textContent = `= ${thicknessLabel(layer.thicknessUm)} of film — feeds Beer attenuation and the cost of the slab below it.`;
  const f0r = card.querySelector('[data-role="f0-readout"]');
  if (f0r) {
    const f0 = EffectiveF0(layer);
    f0r.replaceChildren(readout("F0 (auto, per channel)", f0.map((c) => fmt(c, 3)).join(" / ")));
  }
  const foot = card.querySelector('[data-role="foot"]');
  if (foot) {
    foot.replaceChildren(
      readout("IOR (n / k, R)", `${fmt(layer.ior[0])} / ${fmt(layer.extinction[0])}`),
      readout("Transmission × Beer here", (() => {
        const t = layer.transmission.map((c, i) => fmt(c * Math.pow(layer.attenuation[i], (layer.thicknessUm / 1000) / layer.attenuationDistanceMm), 2));
        return t.join(" / ");
      })()),
    );
  }
  const canvas = card.querySelector('[data-role="proc-canvas"]');
  if (canvas) drawPattern(canvas, layer);
}
function drawPattern(canvas, layer) {
  const ctx = canvas.getContext("2d");
  const W = canvas.width, H = canvas.height, img = ctx.createImageData(W, H);
  for (let y = 0; y < H; y++) for (let x = 0; x < W; x++) {
    const p = PatternSample(layer.proc, x / W, y / H);
    const mix = layer.proc.affects.colour > 0 ? p : p;
    const r = layer.proc.affects.colour > 0 ? layer.colour[0] + (layer.colourB[0] - layer.colour[0]) * mix : p;
    const g = layer.proc.affects.colour > 0 ? layer.colour[1] + (layer.colourB[1] - layer.colour[1]) * mix : p;
    const b = layer.proc.affects.colour > 0 ? layer.colour[2] + (layer.colourB[2] - layer.colour[2]) * mix : p;
    const o = (y * W + x) * 4;
    img.data[o] = r * 255; img.data[o + 1] = g * 255; img.data[o + 2] = b * 255; img.data[o + 3] = 255;
  }
  ctx.putImageData(img, 0, 0);
}
function moveLayer(index, dir) {
  const j = index + dir;
  if (j < 0 || j >= stack.layers.length) return;
  [stack.layers[index], stack.layers[j]] = [stack.layers[j], stack.layers[index]];
  renderStack(); changed(null);
}
function duplicateLayer(index) {
  const copy = JSON.parse(JSON.stringify(stack.layers[index]));
  copy.id = "layer-" + Date.now().toString(36); copy.name = (copy.name + " copy").slice(0, 48);
  stack.layers.splice(index, 0, copy);
  renderStack(); changed(null);
}
function renderStack() {
  const host = $("#stack");
  host.replaceChildren();
  stack.layers.forEach((layer, i) => {
    if (i > 0) {
      const above = stack.layers[i - 1];
      host.append(el("div", { class: "blend-link" },
        el("span", { class: "dir", text: "▼ light continues" }),
        el("b", { text: above.blend.toUpperCase() + (above.blend === "coat" ? "" : " ×" + fmt(above.weight)) }),
        el("span", { class: "dir", text: "into" })));
    }
    host.append(layerCard(layer, i));
  });
}

// ------------------------------------------------------------------ composite card
const previewCanvas = $("#preview");
const off = document.createElement("canvas");
function renderPreview() {
  const W = 170, H = 150;
  off.width = W; off.height = H;
  const ctx = off.getContext("2d");
  const img = ctx.createImageData(W, H);
  const L = norm3([-0.45, 0.65, 0.62]);
  const checker = (u, v) => ((Math.floor(u * 8) + Math.floor(v * 8)) % 2 === 0 ? [0.62, 0.64, 0.66] : [0.2, 0.21, 0.23]);
  for (let y = 0; y < H; y++) for (let x = 0; x < W; x++) {
    const px = x / W, py = y / H;
    let rgb;
    if (previewMode === "sphere") {
      const dx = (px - 0.5) * 2.15, dy = (py - 0.5) * 2.15, r2 = dx * dx + dy * dy;
      if (r2 > 1) {
        const edge = r2 < 1.12 ? 0.55 : 1;
        rgb = checker(px, py).map((c) => c * edge * 0.5);
      } else {
        const N = [dx, -dy, Math.sqrt(1 - r2)];
        rgb = ShadeComposite(stack, { u: px * 1.6, v: py * 1.6, N, L, V: [0, 0, 1], bg: checker(px, py) });
      }
    } else {
      const V = norm3([(px - 0.5) * 1.7, 0.35, 1]);   // fresnel sweep left→right
      rgb = ShadeComposite(stack, { u: px * 2, v: py * 2, N: [0, 0, 1], L, V, bg: checker(px * 2, py * 2) });
    }
    const t = Tonemap(rgb), o = (y * W + x) * 4;
    img.data[o] = t[0] * 255; img.data[o + 1] = t[1] * 255; img.data[o + 2] = t[2] * 255; img.data[o + 3] = 255;
  }
  ctx.putImageData(img, 0, 0);
  const big = previewCanvas.getContext("2d");
  big.imageSmoothingEnabled = true;
  big.clearRect(0, 0, previewCanvas.width, previewCanvas.height);
  big.drawImage(off, 0, 0, previewCanvas.width, previewCanvas.height);
}
function renderCrossSection() {
  const on = stack.layers;
  const heights = on.map((l) => 5 + Math.log10(1 + l.thicknessUm) * 11);
  const total = heights.reduce((a, b) => a + b, 0) || 1;
  let y = 4; const H = 104;
  let svg = `<text x="2" y="9" fill="#6d6d6d" font-size="7.5" letter-spacing="1">AIR / CAMERA</text>`;
  on.forEach((l, i) => {
    const h = Math.max(5, (heights[i] / total) * H);
    svg += `<rect x="2" y="${y.toFixed(1)}" width="168" height="${Math.max(3, h - 1.5).toFixed(1)}" rx="3" fill="${rgbCss(l.colour)}" opacity="${l.enabled ? 0.92 : 0.25}" stroke="#ffffff22"/>`;
    svg += `<text x="178" y="${(y + h / 2 + 3).toFixed(1)}" fill="#a9a9a9" font-size="8.5">${escapeHtml(l.name.slice(0, 26))}</text>`;
    svg += `<text x="178" y="${(y + h / 2 + 12).toFixed(1)}" fill="#666" font-size="7.5">${thicknessLabel(l.thicknessUm)} · ${l.blend}</text>`;
    y += h;
  });
  $("#cross").innerHTML = svg;
}
function renderComposite() {
  renderPreview();
  renderCrossSection();
  const r = CompositeReadout(stack);
  $("#composite-state").replaceChildren(el("p", { class: "state-line" }, el("i"),
    el("span", { text: `${r.enabledCount}/${r.layerCount} slabs live · first hit: ${r.surfaceName} · F0 ${r.surfaceF0.map((c) => fmt(c)).join("/")}` })));
  $("#composite-readouts").replaceChildren(
    el("div", { class: "metric" }, thicknessLabel(r.totalThicknessUm).split(" ")[0], el("small", { text: thicknessLabel(r.totalThicknessUm).split(" ")[1] + " total film" })),
    readout("Stack transmittance (through all slabs)", r.transmittance.map((c) => fmt(c)).join(" / ")),
    readout("Dominant opaque slab · metallic / roughness", `${fmt(r.dominantMetallic)} / ${fmt(r.dominantRoughness)}`),
    readout("Estimated shader cost", `${r.cost} · grade ${r.grade}`),
  );
  $("#composite-warnings").replaceChildren(...r.warnings.map((w) =>
    el("p", { class: "state-line warn" }, el("i"), el("span", { text: w }))));
}
function renderGraph() { $("#slab-graph").textContent = StackSummary(stack); }

// ------------------------------------------------------------------ side cards wiring
function renderPresets() {
  const host = $("#presets");
  host.replaceChildren(...PRESET_ORDER.map((key) => el("button", {
    text: PRESETS[key].label, "aria-pressed": String(activePreset === key),
    onclick: () => {
      stack = PRESETS[key].make(); activePreset = key;
      $("#material-name").value = stack.name;
      renderPresets(); renderStack(); changed(null);
    },
  })));
}
function renderAddButtons() {
  $("#add-layer").replaceChildren(...TYPE_ORDER.map((key) => el("button", {
    text: `${LAYER_TYPES[key].glyph} ${LAYER_TYPES[key].label}`,
    onclick: () => {
      const layer = NewLayer(key);
      if (key === "base") stack.layers.push(layer); else stack.layers.unshift(layer);
      activePreset = null; renderPresets(); renderStack(); changed(null);
    },
  })));
}
$("#material-name").addEventListener("input", (e) => { stack.name = e.target.value; renderGraph(); autosave(); });
$("#preview-mode").addEventListener("click", (e) => {
  const b = e.target.closest("button"); if (!b) return;
  previewMode = b.dataset.mode;
  document.querySelectorAll("#preview-mode button").forEach((x) => x.setAttribute("aria-pressed", String(x === b)));
  renderPreview();
});
function note(msg) { $("#data-note").textContent = msg; }
async function copyText(text, what) {
  try { await navigator.clipboard.writeText(text); note(what + " copied to clipboard."); }
  catch { note("Clipboard blocked by the browser — select the text manually instead."); }
}
$("#copy-graph").addEventListener("click", () => copyText(StackSummary(stack), "Slab summary"));
$("#copy-json").addEventListener("click", () => copyText(ExportStack(stack), "JSON"));
$("#export-json").addEventListener("click", () => {
  const blob = new Blob([ExportStack(stack)], { type: "application/json" });
  const a = el("a", { href: URL.createObjectURL(blob), download: (stack.name || "material").replace(/[^\w-]+/g, "-") + ".layerstack.json" });
  a.click(); setTimeout(() => URL.revokeObjectURL(a.href), 2000);
});
$("#reset-stack").addEventListener("click", () => {
  try { localStorage.removeItem(SAVE_KEY); } catch { /* ignore */ }
  stack = DefaultStack(); activePreset = "example";
  $("#material-name").value = stack.name; renderPresets(); renderStack(); changed(null);
  note("Reset to the example stack. Autosave resumes on your next edit.");
});
$("#import-apply").addEventListener("click", () => {
  try {
    stack = ImportStack($("#import-text").value); activePreset = null;
    $("#material-name").value = stack.name; renderPresets(); renderStack(); changed(null);
    note(`Imported “${stack.name}” — ${stack.layers.length} slabs.`);
  } catch (e) { note("Import failed: " + e.message); }
});
$("#import-file-btn").addEventListener("click", () => $("#import-file").click());
$("#import-file").addEventListener("change", async (e) => {
  const f = e.target.files[0]; if (!f) return;
  $("#import-text").value = await f.text();
  $("#import-apply").click();
});

// ------------------------------------------------------------------ boot
$("#material-name").value = stack.name;
renderPresets();
renderAddButtons();
renderStack();
renderGraph();
renderComposite();
