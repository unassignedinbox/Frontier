//============================================================================================================================================
//                                                        MATERIALSTACKMODEL.JS
//============================================================================================================================================
// 📦 Pure model for the layered procedural material stack (no DOM — CheckMaterialStack.mjs runs this).
//
//    A material is a vertical stack of slabs, in the sense of Unreal's Substrate: light/the camera meets
//    layers[0] (the outermost film) first and the base substrate last. Every slab carries the full
//    parameter set below; a layer *type* is only a template of starting values. Triples written XYZ in
//    the UI are per-channel quantities — IOR/extinction/F0/transmission/scatter-radius are RGB triples
//    (a coarse spectral sample), while Normal/Micro-normal are true tangent-space XYZ vectors.
//
//    The shading in ShadeComposite is a deliberate preview approximation (Schlick Fresnel + Beer-Lambert
//    attenuation + a normalized Blinn lobe), good enough to compare stacks, not a path tracer.

export const SCHEMA = "frontier.layerstack/1";
export const SAVE_KEY = "frontier.layerstack.v1";

// ------------------------------------------------------------------ small math
export const clamp = (v, a, b) => (v < a ? a : v > b ? b : v);
export const lerp = (a, b, t) => a + (b - a) * t;
export const mix3 = (a, b, t) => [lerp(a[0], b[0], t), lerp(a[1], b[1], t), lerp(a[2], b[2], t)];
export const mul3 = (a, b) => [a[0] * b[0], a[1] * b[1], a[2] * b[2]];
export const scale3 = (a, s) => [a[0] * s, a[1] * s, a[2] * s];
export const add3 = (a, b) => [a[0] + b[0], a[1] + b[1], a[2] + b[2]];
export const dot3 = (a, b) => a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
export const cross3 = (a, b) => [
  a[1] * b[2] - a[2] * b[1],
  a[2] * b[0] - a[0] * b[2],
  a[0] * b[1] - a[1] * b[0],
];
export function norm3(v) {
  const l = Math.hypot(v[0], v[1], v[2]);
  return l < 1e-8 ? [0, 0, 1] : [v[0] / l, v[1] / l, v[2] / l];
}
export const isNum = (v) => typeof v === "number" && Number.isFinite(v);
function num(v, fallback, a, b) {
  return clamp(isNum(v) ? v : fallback, a, b);
}
function vec(v, fallback, a, b) {
  const r = Array.isArray(v) ? v : fallback;
  return [0, 1, 2].map((i) => num(r[i], fallback[i], a, b));
}

// ------------------------------------------------------------------ slab templates
// Every slab stores every field; a template only chooses starting values (like a Substrate slab preset).
const BASE_FIELDS = {
  enabled: true, blend: "coat", weight: 1, coverage: 1,
  thicknessUm: 100,                       // physical film thickness (Beer + thin-film read this)
  colour: [0.5, 0.5, 0.5], colourB: [0.08, 0.08, 0.09],
  metallic: 0, roughness: 0.5, anisotropy: 0, specular: 1,
  ior: [1.5, 1.5, 1.5], extinction: [0, 0, 0],       // complex IOR (n, k) per RGB channel
  f0Mode: "auto", f0: [0.04, 0.04, 0.04], fresnelPower: 5,
  normal: [0, 0, 1], normalStrength: 1,
  microNormal: [0, 0, 1], microStrength: 0.35, microScale: 24,
  transmission: [0, 0, 0], attenuation: [1, 1, 1], attenuationDistanceMm: 2,
  subsurface: 0, scatterRadiusMm: [1.0, 0.5, 0.3],
  sheen: 0, sheenColour: [1, 1, 1], sheenRoughness: 0.5,
  thinFilm: 0, thinFilmThicknessNm: 450, thinFilmIor: 1.3,
  emission: [0, 0, 0], emissionIntensity: 0,
  proc: {
    pattern: "none", scale: [8, 8], seed: 1, octaves: 3, contrast: 1, invert: false,
    affects: { coverage: 0, colour: 0, roughness: 0, normal: 0, thickness: 0 },
  },
};
const clone = (o) => JSON.parse(JSON.stringify(o));
function slab(overrides) {
  const s = clone(BASE_FIELDS);
  for (const [k, v] of Object.entries(overrides || {})) {
    if (k === "proc") {
      Object.assign(s.proc, clone(v));
      if (v.affects) Object.assign(s.proc.affects, v.affects);
    } else s[k] = clone(v);
  }
  return s;
}

export const LAYER_TYPES = {
  base: {
    label: "Base substrate", glyph: "▦",
    make: () => slab({ thicknessUm: 1500, colour: [0.22, 0.23, 0.25], roughness: 0.62, metallic: 0.05 }),
  },
  diffuse: {
    label: "Diffuse film", glyph: "◐",
    make: () => slab({ thicknessUm: 60, colour: [0.55, 0.32, 0.18], roughness: 0.78 }),
  },
  metal: {
    label: "Metallic layer", glyph: "◆",
    make: () => slab({
      thicknessUm: 25, colour: [0.62, 0.65, 0.68], metallic: 1, roughness: 0.34,
      ior: [1.35, 1.25, 1.45], extinction: [3.1, 2.7, 2.3],
    }),
  },
  coat: {
    label: "Outer coat / clearcoat", glyph: "◎",
    make: () => slab({
      thicknessUm: 35, colour: [1, 1, 1], roughness: 0.06, ior: [1.46, 1.46, 1.46],
      transmission: [0.96, 0.96, 0.96], specular: 1.15,
    }),
  },
  transmission: {
    label: "Transmission film", glyph: "◍",
    make: () => slab({
      thicknessUm: 800, colour: [0.92, 0.98, 0.96], roughness: 0.12, ior: [1.52, 1.515, 1.51],
      transmission: [1, 1, 1], attenuation: [0.75, 0.95, 0.88], attenuationDistanceMm: 4,
    }),
  },
  subsurface: {
    label: "Subsurface / wax", glyph: "☁",
    make: () => slab({
      thicknessUm: 1200, colour: [0.9, 0.82, 0.72], roughness: 0.45,
      subsurface: 0.85, scatterRadiusMm: [1.6, 0.9, 0.55],
    }),
  },
  sheen: {
    label: "Sheen / fuzz", glyph: "≈",
    make: () => slab({
      thicknessUm: 12, colour: [0.35, 0.1, 0.22], roughness: 0.9,
      sheen: 1, sheenColour: [1, 0.55, 0.75], sheenRoughness: 0.45, blend: "add", coverage: 0.85,
    }),
  },
  thinfilm: {
    label: "Thin-film iridescence", glyph: "◌",
    make: () => slab({
      thicknessUm: 0.45, colour: [1, 1, 1], roughness: 0.08,
      thinFilm: 1, thinFilmThicknessNm: 480, thinFilmIor: 1.32,
      transmission: [0.9, 0.9, 0.9], blend: "add", coverage: 0.9,
    }),
  },
  emission: {
    label: "Emissive film", glyph: "☀",
    make: () => slab({
      thicknessUm: 8, colour: [0.05, 0.05, 0.06], roughness: 0.5, blend: "add",
      emission: [1, 0.45, 0.12], emissionIntensity: 2.4,
    }),
  },
};
export const TYPE_ORDER = ["coat", "thinfilm", "transmission", "metal", "diffuse", "subsurface", "sheen", "emission", "base"];

let nextId = 1;
export function NewLayer(type, name) {
  const t = LAYER_TYPES[type] ? type : "diffuse";
  const layer = LAYER_TYPES[t].make();
  layer.id = "layer-" + Date.now().toString(36) + "-" + nextId++;
  layer.type = t;
  layer.name = name || LAYER_TYPES[t].label;
  return layer;
}
function L(type, name, overrides) {
  const layer = NewLayer(type, name);
  const o = slab(overrides || {});
  for (const k of Object.keys(overrides || {})) {
    if (k === "proc") { Object.assign(layer.proc, o.proc); Object.assign(layer.proc.affects, o.proc.affects); }
    else layer[k] = o[k];
  }
  return layer;
}

// ------------------------------------------------------------------ normalization
export function NormalizeLayer(input) {
  const t = input && LAYER_TYPES[input.type] ? input.type : "diffuse";
  const d = LAYER_TYPES[t].make();
  const s = input && typeof input === "object" ? input : {};
  const p = s.proc && typeof s.proc === "object" ? s.proc : {};
  const pa = p.affects && typeof p.affects === "object" ? p.affects : {};
  return {
    id: typeof s.id === "string" && s.id ? s.id : "layer-" + nextId++,
    name: typeof s.name === "string" && s.name.trim() ? s.name.slice(0, 48) : LAYER_TYPES[t].label,
    type: t,
    enabled: s.enabled !== false,
    blend: ["coat", "mix", "add"].includes(s.blend) ? s.blend : d.blend,
    weight: num(s.weight, d.weight, 0, 2),
    coverage: num(s.coverage, d.coverage, 0, 1),
    thicknessUm: num(s.thicknessUm, d.thicknessUm, 0, 20000),
    colour: vec(s.colour, d.colour, 0, 1), colourB: vec(s.colourB, d.colourB, 0, 1),
    metallic: num(s.metallic, d.metallic, 0, 1),
    roughness: num(s.roughness, d.roughness, 0, 1),
    anisotropy: num(s.anisotropy, d.anisotropy, -1, 1),
    specular: num(s.specular, d.specular, 0, 3),
    ior: vec(s.ior, d.ior, 1, 4), extinction: vec(s.extinction, d.extinction, 0, 12),
    f0Mode: s.f0Mode === "manual" ? "manual" : "auto",
    f0: vec(s.f0, d.f0, 0, 1), fresnelPower: num(s.fresnelPower, d.fresnelPower, 1, 10),
    normal: vec(s.normal, d.normal, -1, 1), normalStrength: num(s.normalStrength, d.normalStrength, 0, 3),
    microNormal: vec(s.microNormal, d.microNormal, -1, 1),
    microStrength: num(s.microStrength, d.microStrength, 0, 3),
    microScale: num(s.microScale, d.microScale, 1, 256),
    transmission: vec(s.transmission, d.transmission, 0, 1),
    attenuation: vec(s.attenuation, d.attenuation, 0, 1),
    attenuationDistanceMm: num(s.attenuationDistanceMm, d.attenuationDistanceMm, 0.01, 1000),
    subsurface: num(s.subsurface, d.subsurface, 0, 1),
    scatterRadiusMm: vec(s.scatterRadiusMm, d.scatterRadiusMm, 0, 20),
    sheen: num(s.sheen, d.sheen, 0, 1), sheenColour: vec(s.sheenColour, d.sheenColour, 0, 1),
    sheenRoughness: num(s.sheenRoughness, d.sheenRoughness, 0, 1),
    thinFilm: num(s.thinFilm, d.thinFilm, 0, 1),
    thinFilmThicknessNm: num(s.thinFilmThicknessNm, d.thinFilmThicknessNm, 0, 3000),
    thinFilmIor: num(s.thinFilmIor, d.thinFilmIor, 1, 3),
    emission: vec(s.emission, d.emission, 0, 1),
    emissionIntensity: num(s.emissionIntensity, d.emissionIntensity, 0, 32),
    proc: {
      pattern: ["none", "noise", "voronoi", "checker", "stripes", "weave", "flakes"].includes(p.pattern) ? p.pattern : "none",
      scale: [num(Array.isArray(p.scale) ? p.scale[0] : undefined, d.proc.scale[0], 0.25, 256),
              num(Array.isArray(p.scale) ? p.scale[1] : undefined, d.proc.scale[1], 0.25, 256)],
      seed: Math.round(num(p.seed, d.proc.seed, 0, 9999)),
      octaves: Math.round(num(p.octaves, d.proc.octaves, 1, 6)),
      contrast: num(p.contrast, d.proc.contrast, 0, 4),
      invert: !!p.invert,
      affects: {
        coverage: num(pa.coverage, 0, 0, 1), colour: num(pa.colour, 0, 0, 1),
        roughness: num(pa.roughness, 0, 0, 1), normal: num(pa.normal, 0, 0, 1),
        thickness: num(pa.thickness, 0, 0, 1),
      },
    },
  };
}
export function NormalizeStack(input) {
  const s = input && typeof input === "object" ? input : {};
  const layers = Array.isArray(s.layers) && s.layers.length
    ? s.layers.slice(0, 16).map(NormalizeLayer)
    : DefaultStack().layers;
  return {
    schema: SCHEMA,
    name: typeof s.name === "string" && s.name.trim() ? s.name.slice(0, 64) : "Untitled material",
    layers,
  };
}

// ------------------------------------------------------------------ optics
/** Normal-incidence reflectance from complex IOR (n, k), per channel. */
export function F0FromIOR(ior, extinction) {
  return [0, 1, 2].map((i) => {
    const n = ior[i], k = extinction ? extinction[i] : 0;
    return clamp(((n - 1) * (n - 1) + k * k) / ((n + 1) * (n + 1) + k * k), 0, 1);
  });
}
/** The F0 a slab actually shades with: manual, or IOR-derived, tinted toward base colour by metallic. */
export function EffectiveF0(layer) {
  if (layer.f0Mode === "manual") return layer.f0.slice();
  const dielectric = F0FromIOR(layer.ior, layer.extinction);
  return mix3(dielectric, layer.colour, layer.metallic * 0.9);
}
export function Schlick(f0, cosTheta, power = 5) {
  const m = Math.pow(1 - clamp(cosTheta, 0, 1), power);
  return [0, 1, 2].map((i) => clamp(f0[i] + (1 - f0[i]) * m, 0, 1));
}
/** Beer-Lambert transmittance of `attenuation` colour over a film of thicknessUm. */
export function Beer(attenuation, attenuationDistanceMm, thicknessUm) {
  const ratio = Math.max(0, thicknessUm / 1000) / Math.max(attenuationDistanceMm, 1e-4);
  return attenuation.map((c) => Math.pow(clamp(c, 0, 1), ratio));
}

// ------------------------------------------------------------------ procedural patterns
function Hash2(x, y, seed) {
  let h = Math.imul(x | 0, 374761393) ^ Math.imul(y | 0, 668265263) ^ Math.imul(seed | 0, 974634211);
  h = Math.imul(h ^ (h >>> 13), 1274126177);
  return ((h ^ (h >>> 16)) >>> 0) / 4294967296;
}
function ValueNoise(x, y, seed) {
  const xi = Math.floor(x), yi = Math.floor(y), xf = x - xi, yf = y - yi;
  const s = (t) => t * t * (3 - 2 * t);
  const a = Hash2(xi, yi, seed), b = Hash2(xi + 1, yi, seed);
  const c = Hash2(xi, yi + 1, seed), d = Hash2(xi + 1, yi + 1, seed);
  return lerp(lerp(a, b, s(xf)), lerp(c, d, s(xf)), s(yf));
}
function Voronoi(x, y, seed, cellValue) {
  const xi = Math.floor(x), yi = Math.floor(y);
  let f1 = 8, cell = 0;
  for (let j = -1; j <= 1; j++) for (let i = -1; i <= 1; i++) {
    const cx = xi + i, cy = yi + j;
    const px = cx + Hash2(cx, cy, seed), py = cy + Hash2(cy, cx, seed + 7);
    const d = (x - px) * (x - px) + (y - py) * (y - py);
    if (d < f1) { f1 = d; cell = Hash2(cx, cy, seed + 13); }
  }
  return cellValue ? cell : clamp(Math.sqrt(f1) * 1.15, 0, 1);
}
/** Sample a layer's procedural pattern at uv in [0,1]². Deterministic; result in [0,1]. */
export function PatternSample(proc, u, v) {
  if (!proc || proc.pattern === "none") return 1;
  const x = u * proc.scale[0], y = v * proc.scale[1];
  let out;
  switch (proc.pattern) {
    case "noise": {
      let amp = 0.55, sum = 0, norm = 0, fx = x, fy = y;
      for (let o = 0; o < proc.octaves; o++) {
        sum += amp * ValueNoise(fx, fy, proc.seed + o * 31); norm += amp; amp *= 0.5; fx *= 2.03; fy *= 2.03;
      }
      out = sum / Math.max(norm, 1e-5);
      break;
    }
    case "voronoi": out = Voronoi(x, y, proc.seed, false); break;
    case "flakes": out = Voronoi(x, y, proc.seed, true); break;
    case "checker": out = (Math.floor(x) + Math.floor(y)) % 2 === 0 ? 1 : 0.12; break;
    case "stripes": out = 0.5 + 0.5 * Math.sin(x * Math.PI * 2); break;
    case "weave": {
      const over = (Math.floor(x) + Math.floor(y)) % 2 === 0;
      const thread = over ? Math.abs(Math.sin((x % 1) * Math.PI)) : Math.abs(Math.sin((y % 1) * Math.PI));
      out = 0.25 + 0.75 * thread * (over ? 1 : 0.72);
      break;
    }
    default: out = 1;
  }
  out = clamp((out - 0.5) * proc.contrast + 0.5, 0, 1);
  return proc.invert ? 1 - out : out;
}
export function PatternGradient(proc, u, v) {
  const e = 1 / 128;
  const dx = PatternSample(proc, u + e, v) - PatternSample(proc, u - e, v);
  const dy = PatternSample(proc, u, v + e) - PatternSample(proc, u, v - e);
  return [dx, dy];
}

/** Resolve a slab's effective values at a point, after procedural modulation. */
export function LayerAt(layer, u, v) {
  const p = PatternSample(layer.proc, u, v);
  const a = layer.proc.affects;
  const colourTarget = mix3(layer.colour, layer.colourB, p);
  const grad = a.normal > 0 ? PatternGradient(layer.proc, u, v) : [0, 0];
  const micro = PatternSample({ ...layer.proc, pattern: layer.proc.pattern === "none" ? "noise" : layer.proc.pattern, scale: [layer.microScale, layer.microScale], octaves: 2 }, u, v);
  const tsNormal = norm3([
    layer.normal[0] * layer.normalStrength + layer.microNormal[0] * layer.microStrength + grad[0] * a.normal * -3 + (micro - 0.5) * layer.microStrength * 0.6,
    layer.normal[1] * layer.normalStrength + layer.microNormal[1] * layer.microStrength + grad[1] * a.normal * -3,
    Math.max(0.05, layer.normal[2]),
  ]);
  return {
    pattern: p,
    coverage: clamp(layer.coverage * lerp(1, p, a.coverage), 0, 1),
    colour: mix3(layer.colour, colourTarget, a.colour),
    roughness: clamp(lerp(layer.roughness, p, a.roughness * 0.85), 0, 1),
    thicknessUm: layer.thicknessUm * lerp(1, 0.15 + 0.85 * p, a.thickness),
    tsNormal,
  };
}

// ------------------------------------------------------------------ composite preview shading
function tangentFrame(n) {
  const t = Math.abs(n[1]) < 0.92 ? cross3(n, [0, 1, 0]) : cross3(n, [1, 0, 0]);
  const T = norm3(t);
  return [T, cross3(n, T)];
}
/**
 * Shade one point of the stack preview. `s`: {u, v, N, L, V, bg}
 * Iterates outermost → base; `throughput` is how much of the camera path still reaches this slab.
 */
export function ShadeComposite(stack, s) {
  const H = norm3(add3(s.L, s.V));
  let out = [0, 0, 0];
  let throughput = [1, 1, 1];
  for (const layer of stack.layers) {
    if (!layer.enabled) continue;
    const eff = LayerAt(layer, s.u, s.v);
    const [T, B] = tangentFrame(s.N);
    const ts = eff.tsNormal;
    const N = norm3([
      T[0] * ts[0] + B[0] * ts[1] + s.N[0] * ts[2],
      T[1] * ts[0] + B[1] * ts[1] + s.N[1] * ts[2],
      T[2] * ts[0] + B[2] * ts[1] + s.N[2] * ts[2],
    ]);
    const ndv = clamp(dot3(N, s.V), 0, 1);
    const ndlRaw = dot3(N, s.L);
    const wrap = layer.subsurface * 0.55;
    const ndl = clamp((ndlRaw + wrap) / (1 + wrap), 0, 1);
    const ndh = clamp(dot3(N, H), 0, 1);
    const f0 = EffectiveF0(layer);
    const F = scale3(Schlick(f0, ndv, layer.fresnelPower), clamp(layer.specular, 0, 2));
    const specPow = Math.pow(2, lerp(3, 9.5, (1 - eff.roughness) * (1 - eff.roughness)));
    const spec = Math.pow(ndh, specPow) * (specPow + 8) / 42 * (ndlRaw > 0 ? 1 : 0);
    const trans = mul3(layer.transmission, Beer(layer.attenuation, layer.attenuationDistanceMm, eff.thicknessUm));
    // A transmissive slab does not also diffuse its tint at full strength — that double-counts the same
    // photons (a white clearcoat would paint the stack white). Diffuse belongs to the opaque fraction;
    // the small constant is a hemisphere fill so the preview's dark side stays readable.
    const opaqueFraction = 1 - (trans[0] + trans[1] + trans[2]) / 3;
    const diffuse = scale3(eff.colour, (1 - layer.metallic) * (ndl * 0.92 + 0.055) * opaqueFraction);
    // thin-film: cosine palette keyed to film thickness and view angle
    let film = [0, 0, 0];
    if (layer.thinFilm > 0) {
      const phase = layer.thinFilmThicknessNm / 160 * (0.7 + 0.6 * (1 - ndv)) * layer.thinFilmIor;
      film = [0, 1, 2].map((i) => (0.5 + 0.5 * Math.cos(2 * Math.PI * (phase + i / 3))) * spec * layer.thinFilm * 2.2);
    }
    const sheenTerm = scale3(layer.sheenColour, layer.sheen * Math.pow(1 - ndv, 2.5 + layer.sheenRoughness * 2) * (0.25 + 0.75 * ndl) * 0.8);
    const specTerm = mul3(F, [spec * ndl, spec * ndl, spec * ndl]);
    const slabLight = add3(add3(diffuse, specTerm), add3(sheenTerm, film));
    const w = layer.blend === "coat" ? eff.coverage : eff.coverage * clamp(layer.weight, 0, 2);
    out = add3(out, mul3(throughput, scale3(slabLight, w)));
    // emission radiates toward camera, attenuated only by the slabs above it (already folded into throughput)
    out = add3(out, mul3(throughput, scale3(layer.emission, layer.emissionIntensity * eff.coverage * 0.35)));
    if (layer.blend === "add") {
      throughput = mul3(throughput, [1, 1, 1].map(() => 1 - w * 0.12));
    } else if (layer.blend === "mix") {
      throughput = scale3(throughput, 1 - clamp(w, 0, 1) * 0.9);
    } else {
      throughput = [0, 1, 2].map((i) => throughput[i] * ((1 - eff.coverage) + eff.coverage * trans[i] * (1 - F[i] * 0.55)));
    }
    if (throughput[0] < 0.003 && throughput[1] < 0.003 && throughput[2] < 0.003) break;
  }
  return add3(out, mul3(throughput, s.bg || [0, 0, 0]));
}
export function Tonemap(rgb) {
  return rgb.map((c) => {
    const t = clamp(c / (1 + c) * 1.28, 0, 1);
    return Math.pow(t, 1 / 2.2);
  });
}

// ------------------------------------------------------------------ readouts, cost, warnings
export function CompositeReadout(stack) {
  const on = stack.layers.filter((l) => l.enabled);
  const totalThicknessUm = on.reduce((a, l) => a + l.thicknessUm, 0);
  const surface = on[0] || null;
  let transmittance = [1, 1, 1];
  for (const l of on) {
    const t = mul3(l.transmission, Beer(l.attenuation, l.attenuationDistanceMm, l.thicknessUm));
    transmittance = [0, 1, 2].map((i) => transmittance[i] * ((1 - l.coverage) + l.coverage * t[i]));
  }
  const firstOpaque = on.find((l) => (l.transmission[0] + l.transmission[1] + l.transmission[2]) / 3 < 0.35);
  let cost = 0;
  for (const l of on) {
    cost += 10;
    if ((l.transmission[0] + l.transmission[1] + l.transmission[2]) > 0.05) cost += 9;
    if (l.sheen > 0.02) cost += 4;
    if (l.subsurface > 0.02) cost += 6;
    if (l.thinFilm > 0.02) cost += 5;
    if (l.emissionIntensity > 0.02) cost += 2;
    if (l.proc.pattern !== "none") cost += 2 + l.proc.octaves;
  }
  const warnings = [];
  const names = new Set();
  stack.layers.forEach((l, idx) => {
    if (names.has(l.id)) warnings.push("Duplicate layer id " + l.id);
    names.add(l.id);
    const f0 = EffectiveF0(l);
    if (f0.some((c) => c > 1 || c < 0)) warnings.push(`“${l.name}”: F0 leaves [0,1] — energy will blow out.`);
    if (l.ior.some((n) => n < 1)) warnings.push(`“${l.name}”: IOR < 1 is not a passive material.`);
    if (l.metallic > 0.5 && (l.transmission[0] + l.transmission[1] + l.transmission[2]) > 0.3)
      warnings.push(`“${l.name}”: conductors don't transmit — metallic ${(l.metallic * 100) | 0}% with transmission will look wrong.`);
    if (l.enabled && l.emissionIntensity > 0.05) {
      const cover = stack.layers.slice(0, idx).filter((x) => x.enabled)
        .reduce((a, x) => a * (1 - x.coverage * (1 - (x.transmission[0] + x.transmission[1] + x.transmission[2]) / 3)), 1);
      if (cover < 0.25) warnings.push(`“${l.name}”: emissive layer is buried under nearly opaque slabs — barely visible.`);
    }
    if (l.enabled && (l.transmission[0] + l.transmission[1] + l.transmission[2]) / 3 > 0.5) {
      const above = stack.layers.slice(0, idx).find((x) => x.enabled && x.coverage > 0.97 && (x.transmission[0] + x.transmission[1] + x.transmission[2]) / 3 < 0.1);
      if (above) warnings.push(`“${l.name}”: transmissive slab sits under opaque “${above.name}” — it can never be seen.`);
    }
  });
  if (on.length > 6) warnings.push(`${on.length} enabled slabs — past 6, most engines start folding layers. Watch the cost grade.`);
  if (!on.length) warnings.push("Every layer is disabled — the preview shows straight through to the background.");
  return {
    totalThicknessUm,
    layerCount: stack.layers.length, enabledCount: on.length,
    surfaceF0: surface ? EffectiveF0(surface) : [0, 0, 0],
    surfaceName: surface ? surface.name : "—",
    dominantMetallic: firstOpaque ? firstOpaque.metallic : 0,
    dominantRoughness: firstOpaque ? firstOpaque.roughness : (surface ? surface.roughness : 0),
    transmittance,
    cost, grade: cost <= 34 ? "A · cheap" : cost <= 64 ? "B · moderate" : cost <= 96 ? "C · heavy" : "D · fold me",
    warnings,
  };
}
export function StackSummary(stack) {
  const lines = stack.layers.map((l, i) => {
    const op = i === stack.layers.length - 1 ? "BASE" : l.blend.toUpperCase();
    const f0 = EffectiveF0(l).map((c) => c.toFixed(2)).join("/");
    return `${op.padEnd(5)} ${l.enabled ? "●" : "○"} ${l.name} [${LAYER_TYPES[l.type].label}] ${l.thicknessUm}µm rough ${l.roughness.toFixed(2)} metal ${l.metallic.toFixed(2)} F0 ${f0}` +
      (l.proc.pattern !== "none" ? ` proc:${l.proc.pattern}×${l.proc.scale[0]}` : "");
  });
  return `${stack.name} — ${SCHEMA}\n${lines.join("\n")}`;
}

// ------------------------------------------------------------------ import / export
export function ExportStack(stack) {
  return JSON.stringify({ schema: SCHEMA, exportedAt: new Date().toISOString(), ...NormalizeStack(stack) }, null, 2);
}
export function ImportStack(text) {
  let parsed;
  try { parsed = JSON.parse(text); } catch (e) { throw new Error("Not valid JSON: " + e.message); }
  if (!parsed || typeof parsed !== "object" || !Array.isArray(parsed.layers))
    throw new Error("JSON has no layers[] — expected a " + SCHEMA + " export.");
  return NormalizeStack(parsed);
}

// ------------------------------------------------------------------ presets (layers listed outermost → base)
export function DefaultStack() {
  return NormalizeStack({
    name: "Coated metal — your example, fleshed out",
    layers: [
      L("coat", "Layer 3 · Outer coat", { thicknessUm: 35, roughness: 0.07, transmission: [0.97, 0.97, 0.97] }),
      L("metal", "Layer 2 · Metallic colour", {
        thicknessUm: 18, colour: [0.07, 0.16, 0.5], colourB: [0.35, 0.55, 0.95], metallic: 0.9, roughness: 0.38,
        proc: { pattern: "flakes", scale: [26, 26], seed: 7, affects: { colour: 0.85, roughness: 0.5, normal: 0.35 } },
      }),
      L("base", "Layer 1 · Base substrate", { thicknessUm: 1500, colour: [0.1, 0.11, 0.13], roughness: 0.55 }),
    ],
  });
}
export const PRESETS = {
  example: { label: "Your example (coat / metal / base)", make: () => DefaultStack() },
  carPaint: {
    label: "Automotive flake paint",
    make: () => NormalizeStack({
      name: "Automotive flake paint",
      layers: [
        L("coat", "Clearcoat", { thicknessUm: 38, roughness: 0.045, microScale: 90, microStrength: 0.12 }),
        L("metal", "Flake colour coat", {
          thicknessUm: 14, colour: [0.42, 0.05, 0.08], colourB: [0.85, 0.3, 0.28], metallic: 0.82, roughness: 0.42,
          proc: { pattern: "flakes", scale: [34, 34], seed: 21, affects: { colour: 0.9, roughness: 0.55, normal: 0.5 } },
        }),
        L("diffuse", "Primer", { thicknessUm: 60, colour: [0.16, 0.16, 0.17], roughness: 0.8 }),
        L("base", "Panel substrate", { thicknessUm: 2000, metallic: 0.65, colour: [0.3, 0.31, 0.33], roughness: 0.5 }),
      ],
    }),
  },
  frostedGlass: {
    label: "Frosted tinted glass",
    make: () => NormalizeStack({
      name: "Frosted tinted glass",
      layers: [
        L("transmission", "Frosted skin", {
          thicknessUm: 90, roughness: 0.42, transmission: [0.94, 0.94, 0.94],
          proc: { pattern: "noise", scale: [60, 60], seed: 4, octaves: 3, affects: { roughness: 0.7, normal: 0.8 } },
        }),
        L("transmission", "Tinted body", {
          thicknessUm: 2600, roughness: 0.05, transmission: [1, 1, 1],
          attenuation: [0.55, 0.9, 0.78], attenuationDistanceMm: 2.2,
        }),
      ],
    }),
  },
  carbonFibre: {
    label: "Carbon fibre 2×2 twill",
    make: () => NormalizeStack({
      name: "Carbon fibre 2×2 twill",
      layers: [
        L("coat", "Epoxy gloss", { thicknessUm: 30, roughness: 0.12 }),
        L("base", "Woven tows", {
          thicknessUm: 900, colour: [0.05, 0.05, 0.06], colourB: [0.28, 0.29, 0.32], metallic: 0.25, roughness: 0.38, anisotropy: 0.7,
          proc: { pattern: "weave", scale: [9, 9], seed: 2, affects: { colour: 0.95, roughness: 0.4, normal: 0.85 } },
        }),
      ],
    }),
  },
  velvet: {
    label: "Velvet",
    make: () => NormalizeStack({
      name: "Velvet",
      layers: [
        L("sheen", "Pile sheen", { colour: [0.22, 0.03, 0.12], sheenColour: [1, 0.42, 0.62], sheenRoughness: 0.5 }),
        L("base", "Dyed cloth", {
          thicknessUm: 1100, colour: [0.3, 0.04, 0.16], colourB: [0.18, 0.02, 0.1], roughness: 0.96,
          proc: { pattern: "noise", scale: [44, 44], seed: 9, octaves: 4, affects: { colour: 0.55, normal: 0.4 } },
        }),
      ],
    }),
  },
  wax: {
    label: "Wax / marble subsurface",
    make: () => NormalizeStack({
      name: "Wax / marble subsurface",
      layers: [
        L("coat", "Polished skin", { thicknessUm: 20, roughness: 0.22, transmission: [0.9, 0.9, 0.9] }),
        L("subsurface", "Scattering body", {
          thicknessUm: 2400, colour: [0.93, 0.88, 0.8], colourB: [0.72, 0.68, 0.66], roughness: 0.5,
          subsurface: 0.95, scatterRadiusMm: [2.2, 1.1, 0.6],
          proc: { pattern: "voronoi", scale: [5, 5], seed: 12, affects: { colour: 0.5, thickness: 0.3 } },
        }),
      ],
    }),
  },
  iridescent: {
    label: "Iridescent film on black",
    make: () => NormalizeStack({
      name: "Iridescent film on black",
      layers: [
        L("thinfilm", "Soap film", {
          colour: [0, 0, 0], // a film has no diffuse body of its own — only interference colour
          thinFilmThicknessNm: 520,
          proc: { pattern: "noise", scale: [4, 4], seed: 3, octaves: 3, affects: { thickness: 0.85 } },
        }),
        L("base", "Black glossy base", { thicknessUm: 1300, colour: [0.015, 0.015, 0.02], roughness: 0.16, metallic: 0.35 }),
      ],
    }),
  },
  oxideMetal: {
    label: "Brushed steel + oxide",
    make: () => NormalizeStack({
      name: "Brushed steel + oxide",
      layers: [
        L("thinfilm", "Oxide tint", { thinFilm: 0.55, thinFilmThicknessNm: 300, coverage: 0.7, proc: { pattern: "noise", scale: [7, 7], seed: 5, affects: { coverage: 0.8, thickness: 0.6 } } }),
        L("metal", "Brushed steel", {
          thicknessUm: 1600, colour: [0.66, 0.68, 0.7], roughness: 0.36, anisotropy: 0.85,
          proc: { pattern: "stripes", scale: [140, 3], seed: 1, affects: { roughness: 0.45, normal: 0.55 } },
        }),
      ],
    }),
  },
};
export const PRESET_ORDER = ["example", "carPaint", "frostedGlass", "carbonFibre", "velvet", "wax", "iridescent", "oxideMetal"];
