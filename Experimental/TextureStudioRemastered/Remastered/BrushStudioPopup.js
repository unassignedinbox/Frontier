/*==========================================================================================================================================
                                                         BRUSHSTUDIOPOPUP.JS
==========================================================================================================================================*/
/* Procreate-style Brush Studio & Per-Tool Context Menu for TextureStudio Remastered:
   - Anchored, draggable context popover triggered via active-tool click, viewport right-click, Q / Tab, or Inspector launcher
   - 102-brush searchable library with live rendered stroke thumbnails, custom/duplicate brushes, and .brush.json import/export
   - Full 14-section Brush Studio (139 normalized fields, 16 procedural shapes, 12 grains, SVG pressure/tilt curves, 22-ch ChannelStack)
   - Dedicated multi-section per-tool studios for all 10 tools (Paint, Smudge, Erase, Fill, Stamp, Gradient, Text, Curve, Move, Eyedropper)
   - Toggleable live Drawing Pad previewer (Pad.js) and real-time 2-way sync with TexturePaintWorkspace's WebGL2/CPU 3D+2D paint engine. */

import { icon } from "./Icons.js";
import { Esc, Knob, ColourField, Stops, SwitchRow, Segments, Dropdown, Clamp, Pct, Fixed, SyncSiblings, Download, History } from "./CardKit.js";
import * as M from "./BrushModel.js";
import {
  TOOLS,
  TOOL,
  CreateTools,
  SetToolValue,
  TOOL_STACKS,
  VisibleToolFields,
  ERASERS,
  STUDIOS,
  TOOL_COLOURS,
  PALETTE,
  ToolChanged,
  ToolChangedBySection,
  SectionGroups,
  ToolDefault,
} from "./ToolModel.js";
import { ChannelStack, StackAction, StackUi, ParseStackBind, VisibleCardFields } from "./ChannelStack.js";
import { paintTool, toolSchema, toolArtURL } from "../Engine/PaintToolCatalogue.js";
import { RenderStroke } from "./Painter.js";
import { PreviewPoints, sampleCurve } from "./BrushEngine.js";
import { ShapeThumb, GrainThumb, Tip, GrainValues, GRAIN_SIZE } from "./Sources.js";
import { CreatePad } from "./Pad.js";

const STORAGE = "Frontier.TextureStudioRemastered.BrushStudio.v1";
const DEFAULT_ID = M.PAINT_TOOLS.find((T) => T.id === "brush-round-soft")?.id || M.PAINT_TOOLS[0]?.id || "brush-round-soft";
const Equal = (A, B) => JSON.stringify(A) === JSON.stringify(B);
const FamilyLabel = (Key) => M.PAINT_FAMILIES.find((F) => F.key === Key)?.label || Key;
const SECTION_COLOURS = Object.freeze({
  quick: "var(--highlight-bg)",
  stroke: "#6ea8ff",
  stabilization: "#9ecfff",
  taper: "#c49bff",
  shape: "#e8c27a",
  grain: "#d8a45c",
  rendering: "#ff8c7a",
  wet: "#5cd4c8",
  colour: "#ff6b9a",
  dynamics: "#7ac98f",
  pen: "#a6b8ff",
  properties: "#c8cbd4",
  materials: "#eff4cf",
  instrument: "#e8c27a",
  about: "#848484",
});

function HexToRgb(H) {
  const N = parseInt(String(H || "#ffffff").slice(1), 16) || 0;
  return [(N >> 16) & 255, (N >> 8) & 255, N & 255];
}
function RgbToHsl(R, G, B) {
  R /= 255; G /= 255; B /= 255;
  const Hi = Math.max(R, G, B), Lo = Math.min(R, G, B), D = Hi - Lo, L = (Hi + Lo) / 2;
  if (!D) return [0, 0, Math.round(L * 100)];
  const S = D / (1 - Math.abs(2 * L - 1));
  const H = (Hi === R ? ((G - B) / D + (G < B ? 6 : 0)) : Hi === G ? ((B - R) / D + 2) : ((R - G) / D + 4)) * 60;
  return [Math.round(H), Math.round(S * 100), Math.round(L * 100)];
}

function CurveSvg(Pts) {
  const W = 320, H = 132, P = 10, Iw = W - P * 2, Ih = H - P * 2;
  const X = (U) => P + U * Iw, Y = (V) => H - P - V * Ih;
  const Grid = [0.25, 0.5, 0.75]
    .map((F) => `<line x1="${X(F)}" y1="${P}" x2="${X(F)}" y2="${H - P}"/><line x1="${P}" y1="${Y(F)}" x2="${W - P}" y2="${Y(F)}"/>`)
    .join("");
  const D = [];
  for (let I = 0; I <= 48; I++) {
    const U = I / 48;
    D.push(`${I ? "L" : "M"}${X(U).toFixed(1)},${Y(sampleCurve(Pts, U)).toFixed(1)}`);
  }
  const Line = D.join(" ");
  const Area = `${Line} L${X(1)},${Y(0)} L${X(0)},${Y(0)} Z`;
  const Dots = Pts.map(([Px, Py], I) => `<circle class="bs-curve-pt" data-pt="${I}" cx="${X(Px).toFixed(1)}" cy="${Y(Py).toFixed(1)}" r="5.5"/>`).join("");
  return `<rect class="bs-curve-bg" x="1" y="1" width="${W - 2}" height="${H - 2}" rx="10"/><g class="bs-curve-grid">${Grid}</g><path class="bs-curve-fill" d="${Area}"/><path class="bs-curve-line" d="${Line}"/>${Dots}`;
}

function PresetMiniSvg(Pts) {
  const D = [];
  for (let I = 0; I <= 14; I++) {
    const U = I / 14;
    D.push(`${I ? "L" : "M"}${(2 + U * 20).toFixed(1)},${(14 - sampleCurve(Pts, U) * 12).toFixed(1)}`);
  }
  return `<svg viewBox="0 0 24 16" aria-hidden="true"><path d="${D.join(" ")}" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round"/></svg>`;
}

const SameCurve = (A, B) => Array.isArray(A) && Array.isArray(B) && A.length === B.length && A.every((P, I) => Math.abs(P[0] - B[I][0]) < 0.01 && Math.abs(P[1] - B[I][1]) < 0.01);

const Thumbs = new Map();
function StrokeThumb(Brush, W = 176, H = 30) {
  const Key = `${Brush.id}:${W}x${H}:${JSON.stringify([Brush.values["shape.source"], Brush.values["shape.roundness"], Brush.values["shape.hardness"], Brush.values["shape.scatter"], Brush.values["shape.count"], Brush.values["stroke.spacing"], Brush.values["stroke.jitterLateral"], Brush.values["grain.source"], Brush.values["grain.depth"], Brush.values["taper.size"], Brush.values["wet.dilution"], Brush.values["prop.size"], Brush.values["prop.opacity"], Brush.values["render.flow"], Brush.values["render.blend"], Brush.values["colour.mode"], Brush.values["colour.gradient"]])}`;
  if (Thumbs.has(Key)) return Thumbs.get(Key);
  const C = document.createElement("canvas");
  C.width = W;
  C.height = H;
  const Ctx = C.getContext("2d");
  if (Ctx) {
    const V = {
      ...Brush.values,
      "colour.primary": "#eff4cf",
      "colour.secondary": "#e8c27a",
      "prop.size": Clamp(Brush.values["prop.size"], 12, 46),
      "prop.opacity": Math.max(Brush.values["prop.opacity"], 50),
    };
    const S = Math.max(4, Math.min(H * 0.44, 4 + V["prop.size"] * 0.2));
    RenderStroke(Ctx, { ...Brush, values: V }, PreviewPoints(W, H, 14, 0.36), { Seed: Brush.id.length * 73 + 9, Scale: S / Math.max(1, V["prop.size"]) });
  }
  const Url = C.toDataURL("image/png");
  if (Thumbs.size > 320) Thumbs.clear();
  Thumbs.set(Key, Url);
  return Url;
}

function PaintStampPreview(Canvas, V) {
  const Ctx = Canvas.getContext("2d");
  if (!Ctx) return;
  const W = Canvas.width, H = Canvas.height;
  Ctx.clearRect(0, 0, W, H);
  const T = Tip(V["shape.source"], V["shape.hardness"], V["shape.roundness"], V["shape.invert"], V["shape.filter"]);
  Ctx.save();
  Ctx.translate(W / 2, H / 2);
  Ctx.rotate((V["shape.rotation"] * Math.PI) / 180);
  const S = Math.min(W, H) * 0.84;
  Ctx.drawImage(T, -S / 2, -S / 2, S, S);
  Ctx.restore();
}

function PaintGrainPreview(Canvas, V) {
  const Ctx = Canvas.getContext("2d");
  if (!Ctx) return;
  const W = Canvas.width, H = Canvas.height;
  const Gr = GrainValues(V["grain.source"], V["grain.brightness"], V["grain.contrast"], V["grain.invert"]);
  const Img = Ctx.createImageData(W, H);
  const Scale = Math.max(0.25, V["grain.scale"] / 100);
  const Ang = (V["grain.rotation"] * Math.PI) / 180, Ca = Math.cos(Ang), Sa = Math.sin(Ang);
  const D = V["grain.depth"] / 100;
  for (let Y = 0; Y < H; Y++) {
    for (let X = 0; X < W; X++) {
      const Rx = (X * Ca - Y * Sa) / Scale, Ry = (X * Sa + Y * Ca) / Scale;
      const Gx = ((Math.floor(Rx) % GRAIN_SIZE) + GRAIN_SIZE) % GRAIN_SIZE;
      const Gy = ((Math.floor(Ry) % GRAIN_SIZE) + GRAIN_SIZE) % GRAIN_SIZE;
      const Val = Math.round((1 - D + D * (Gr[Gy * GRAIN_SIZE + Gx] / 255)) * 230);
      const O = (Y * W + X) * 4;
      Img.data[O] = Val;
      Img.data[O + 1] = Val;
      Img.data[O + 2] = Math.round(Val * 0.94);
      Img.data[O + 3] = 255;
    }
  }
  Ctx.putImageData(Img, 0, 0);
}

// Convert BrushModel procedural Tip canvas into an RGBA surface-map object usable by TexturePaintWorkspace's GPU/CPU engine!
const TipSurfaceCache = new Map();
function BuildWorkspaceTipData(V) {
  const src = V["shape.source"] || "soft";
  const hard = Math.round(V["shape.hardness"] ?? 70);
  const round = Math.round(V["shape.roundness"] ?? 100);
  const inv = !!V["shape.invert"];
  const key = `${src}|${hard}|${round}|${inv}`;
  if (TipSurfaceCache.has(key)) return TipSurfaceCache.get(key);
  const cv = Tip(src, hard, round, inv, V["shape.filter"] !== false);
  const w = cv.width, h = cv.height;
  const cx = cv.getContext("2d", { willReadFrequently: true });
  if (!cx) return null;
  const rgba = cx.getImageData(0, 0, w, h).data;
  // Normalize into TexturePaintEngine's stamp/tip map format (alpha mask in albedo[3], white RGB)
  const albedo = new Uint8ClampedArray(w * h * 4);
  for (let i = 0; i < w * h; i++) {
    const a = rgba[i * 4 + 3];
    albedo[i * 4] = 255;
    albedo[i * 4 + 1] = 255;
    albedo[i * 4 + 2] = 255;
    albedo[i * 4 + 3] = a;
  }
  const obj = { width: w, heightPx: h, albedo, rgba: albedo, name: `BrushTip:${src}` };
  if (TipSurfaceCache.size > 48) TipSurfaceCache.clear();
  TipSurfaceCache.set(key, obj);
  return obj;
}

const SHAPE_TO_BUILTIN_TIP = {
  hard: "round",
  soft: "round",
  airbrush: "round",
  ink: "round",
  chisel: "chisel",
  calligraphy: "chisel",
  square: "chisel",
  chalk: "chalk",
  charcoal: "chalk",
  splatter: "splatter",
  noise: "splatter",
  acrylic: "sponge",
  gouache: "sponge",
  watercolour: "sponge",
  brush: "sponge",
  bristle: "chalk",
  star: "custom",
};

const COLOUR_MODE_MAP = {
  "None": "solid",
  "Stroke": "stroke-gradient",
  "Stamp": "multicolour",
  "Velocity": "velocity",
  "Temperature": "temperature",
  "Rainbow": "rainbow",
};

const OPACITY_MODE_MAP = {
  "Constant": "constant",
  "Fade out": "fade-out",
  "Fade in": "fade-in",
  "Bell": "bell",
  "Pulse": "pulse",
  "Velocity": "velocity",
  "Curve": "curve",
};

const FONT_CSS_TO_WORKSPACE = {
  sans: "DM Sans",
  serif: "Georgia",
  mono: "ui-monospace",
  condensed: "Arial Narrow",
  display: "Impact",
  script: "Brush Script MT",
};

const QuickBrushGroups = Object.freeze([
  { Title: "Active Brush", Fields: [{ K: "brushcard", T: "brushcard" }] },
  { Title: "Colour & Swatches", Fields: [{ K: "colours", T: "colours" }] },
  { Title: "Core Controls", Fields: [
    { K: "@prop.size", T: "brush" },
    { K: "@prop.opacity", T: "brush" },
    { K: "@render.flow", T: "brush" },
    { K: "@shape.hardness", T: "brush" },
    { K: "@stab.streamline", T: "brush", L: "StreamLine" },
    { K: "@stroke.spacing", T: "brush" },
  ] },
  { Title: "PBR Channels", Hint: "Switch channels on/off for 3D + 2D painting", Fields: [{ K: "@mat.stack", T: "brush" }] },
]);

export function createBrushStudioController(bridge) {
  // Load / initialize brush catalogue & tool studio state
  const St = {
    ActiveId: DEFAULT_ID,
    Brushes: {},
    Custom: [],
    Recent: [DEFAULT_ID],
    Open: new Set(["brushes"]),
    Filter: "",
    Section: "quick",
    Tool: bridge.getTool?.() || "paint",
    Studio: bridge.getTool?.() || "paint",
    ToolSection: Object.fromEntries(Object.entries(STUDIOS).map(([K, S]) => [K, S.Sections[0].Id])),
    Tools: CreateTools(),
    StackUi: {},
    Colours: [...PALETTE.slice(0, 8)],
    Paper: "paper",
    Live: true,
    Preview: true,
    LibOpen: false,
    PadOpen: false,
    OpenPopup: false,
    PopupPos: null,
  };

  try {
    const Saved = JSON.parse(localStorage.getItem(STORAGE) || "null");
    if (Saved && typeof Saved === "object") {
      if (Saved.Brushes && typeof Saved.Brushes === "object") {
        for (const [Id, B] of Object.entries(Saved.Brushes)) {
          if (B && B.values) St.Brushes[Id] = M.NormaliseBrush(B);
        }
      }
      if (Array.isArray(Saved.Custom)) St.Custom = Saved.Custom.filter((Id) => St.Brushes[Id]);
      if (Saved.ActiveId && (St.Brushes[Saved.ActiveId] || paintTool(Saved.ActiveId))) St.ActiveId = Saved.ActiveId;
    }
  } catch { /* ignore private storage */ }

  const Brush = (Id = St.ActiveId) => {
    if (!St.Brushes[Id]) St.Brushes[Id] = M.FromInstrument(Id);
    return St.Brushes[Id];
  };
  Brush(St.ActiveId);

  let EraseMemo = null, SmudgeMemo = null;
  const EraseBrush = () => {
    const E = St.Tools.erase, Src = Brush();
    const Key = `${E.useBrush}|${E.instrument}|${E.size}|${E.opacity}|${E.flow}|${E.hardness}|${E.spacing}|${St.ActiveId}`;
    if (EraseMemo?.Key === Key) return EraseMemo.Brush;
    const Base = E.useBrush ? Src.values : M.FromInstrument(E.instrument).values;
    const V = M.Normalise({ ...Base, "prop.size": E.size, "prop.opacity": E.opacity, "render.flow": E.flow, "shape.hardness": E.hardness, "stroke.spacing": E.spacing });
    EraseMemo = { Key, Brush: { ...Src, values: V } };
    return EraseMemo.Brush;
  };
  const SmudgeBrush = () => {
    const S = St.Tools.smudge, Src = Brush();
    const Key = `${S.useBrush}|${S.size}|${S.hardness}|${S.spacing}|${S.streamline}|${S.strength}|${St.ActiveId}`;
    if (SmudgeMemo?.Key === Key) return SmudgeMemo.Brush;
    const V = S.useBrush
      ? M.Normalise({ ...Src.values, "stroke.spacing": S.spacing, "stab.streamline": S.streamline, "prop.opacity": 100 })
      : M.Normalise({ ...M.DEFAULTS, "shape.source": "round", "shape.hardness": S.hardness, "prop.size": S.size, "prop.maxSize": 60, "prop.opacity": 100, "stroke.spacing": S.spacing, "stab.streamline": S.streamline, "wet.pull": S.strength });
    SmudgeMemo = { Key, Brush: { ...Src, values: V } };
    return SmudgeMemo.Brush;
  };

  function Save() {
    try {
      const Dirty = {};
      for (const [Id, B] of Object.entries(St.Brushes)) {
        if (St.Custom.includes(Id) || M.Changed(B).size) Dirty[Id] = B;
      }
      localStorage.setItem(STORAGE, JSON.stringify({ ActiveId: St.ActiveId, Custom: St.Custom, Brushes: Dirty }));
    } catch { /* quota */ }
  }

  const StackUiOf = (Bind) => (St.StackUi[Bind] ??= StackUi());
  const IsBrushStudio = () => St.Studio === "paint";

  function StackOf(Base) {
    const [Scope, A, B2] = Base.split("|");
    if (Scope === "b" && A === "mat.stack") {
      const Br = Brush();
      return { Base, L: M.BRUSH_STACK, Flat: Br.values, Reset: Br.reset.values, Kind: "brush" };
    }
    if (Scope === "t" && B2 === "stack") {
      const L = TOOL_STACKS[A];
      if (!L) return null;
      return { Base, L, Flat: St.Tools[A], Reset: CreateTools()[A], Kind: "tool", Tool: A };
    }
    return null;
  }

  // ------------------------------------------------------------------------------------------------ 2-Way Bridge -> TexturePaintWorkspace
  let SyncLock = false;
  function SyncToWorkspace(opts = {}) {
    if (SyncLock) return;
    SyncLock = true;
    try {
      const B = Brush();
      const V = B.values;
      const T = St.Tools;
      const wb = bridge.getBrush();
      if (!wb) return;

      // Primary colour & active brush identity
      wb.color = V["colour.primary"] || wb.color;
      wb.activeBrushId = B.id;
      wb.activeBrushName = B.name;

      // Core brush dimensions & feel (Paint / Curve / Smudge / Erase)
      const activeTool = St.Tool;
      if (activeTool === "erase") {
        const er = T.erase;
        const sizePct = Clamp(er.size, 1, 100);
        wb.size = 0.04 + (sizePct / 100) * 0.72;
        wb.pixelRadius = Math.max(4, Math.round(4 + (sizePct / 100) * 96));
        wb.opacity = Clamp(er.opacity / 100, 0.02, 1);
        wb.flow = Clamp(er.flow / 100, 0.02, 1);
        wb.hardness = Clamp((er.useBrush ? V["shape.hardness"] : er.hardness) / 100, 0, 1);
        wb.spacing = Clamp((er.useBrush ? V["stroke.spacing"] : er.spacing) / 100, 0.04, 1);
      } else if (activeTool === "smudge") {
        const sm = T.smudge;
        const sizePct = Clamp(sm.useBrush ? V["prop.size"] : sm.size, 1, 100);
        wb.size = 0.04 + (sizePct / 100) * 0.72;
        wb.pixelRadius = Math.max(4, Math.round(4 + (sizePct / 100) * 96));
        wb.opacity = Clamp((sm.strength ?? 70) / 100, 0.08, 1);
        wb.flow = Clamp(((sm.strength ?? 70) / 100) * 0.65, 0.05, 0.9);
        wb.hardness = Clamp((sm.useBrush ? V["shape.hardness"] : sm.hardness) / 100, 0, 1);
        wb.spacing = Clamp((sm.spacing ?? 8) / 100, 0.05, 0.6);
        wb.stabilizer = Clamp(((sm.streamline ?? 25) / 100) * 0.8, 0, 0.82);
        wb.smudgeStrength = Clamp((sm.strength ?? 70) / 100, 0.05, 1);
        wb.smudgeFingerPaint = Clamp((sm.paint ?? 0) / 100, 0, 1);
      } else {
        const sizePct = Clamp(V["prop.size"], 1, 100);
        wb.size = 0.04 + (sizePct / 100) * 0.72;
        wb.pixelRadius = Math.max(4, Math.round(4 + (sizePct / 100) * 96));
        wb.opacity = Clamp(V["prop.opacity"] / 100, 0.02, 1);
        wb.flow = Clamp(V["render.flow"] / 100, 0.02, 1);
        wb.hardness = Clamp(V["shape.hardness"] / 100, 0, 1);
        wb.roundness = Clamp((V["shape.roundness"] ?? 100) / 100, 0.12, 1);
        wb.angle = Number(V["shape.rotation"] || 0);
        wb.spacing = Clamp((V["stroke.spacing"] ?? 18) / 100, 0.04, 1);
        wb.followDirection = !!V["shape.azimuth"] || V["shape.rotateMode"] === "Follow";
        wb.stabilizer = Clamp(((V["stab.streamline"] ?? 18) / 100) * 0.82, 0, 0.85);
      }

      // Tip shape mapping: use procedural Tip bitmap for distinctive Procreate shapes!
      const shapeSrc = V["shape.source"] || "soft";
      if (!wb._hasUserUploadedTip) {
        if (shapeSrc === "soft" || shapeSrc === "hard" || shapeSrc === "airbrush") {
          wb.tip = "round";
          wb.customTipData = null;
        } else {
          const tipSurf = BuildWorkspaceTipData(V);
          if (tipSurf) {
            wb.tip = "custom";
            wb.customTipData = tipSurf;
            wb.customTipName = shapeSrc;
          } else {
            wb.tip = SHAPE_TO_BUILTIN_TIP[shapeSrc] || "round";
          }
        }
      }

      // Jitter
      wb.jitter = {
        size: Clamp((V["dyn.jitterSize"] || 0) / 100, 0, 1),
        angle: Clamp((V["shape.scatter"] || 0) / 100, 0, 1),
        scatter: Clamp((V["stroke.jitterLateral"] || 0) / 100, 0, 1),
        flow: Clamp((V["dyn.jitterOpacity"] || 0) / 100, 0, 1),
      };

      // Colour Dynamics & Pressure Curves
      const dynMode = COLOUR_MODE_MAP[V["colour.mode"]] || "solid";
      const opMode = OPACITY_MODE_MAP[V["render.opacityMode"]] || "constant";
      const gradPresetId = V["colour.gradient"] || "ember";
      const gradPreset = M.COLOUR_GRADIENT_PRESETS.find((g) => g.id === gradPresetId);
      wb.colourDynamics = {
        ...wb.colourDynamics,
        mode: dynMode,
        gradientPreset: gradPresetId,
        gradientStops: gradPreset ? [...gradPreset.stops] : (wb.colourDynamics?.gradientStops || ["#ff3d2e", "#ff9d3b", "#ffe38e"]),
        temperatureShift: Clamp((V["colour.temp"] || 0) / 100, -1, 1),
        opacityMode: opMode,
        opacityJitter: Clamp((V["dyn.jitterOpacity"] || 0) / 100, 0, 1),
        hueJitter: Clamp(((V["colour.hueStamp"] || V["colour.hueStroke"] || 0)) / 100, 0, 1),
        satJitter: Clamp(((V["colour.satStamp"] || V["colour.satStroke"] || 0)) / 100, 0, 1),
        valJitter: Clamp(((V["colour.lightStamp"] || V["colour.lightStroke"] || 0)) / 100, 0, 1),
        strokeGradient: dynMode === "stroke-gradient",
      };
      if (Array.isArray(V["pen.pressureCurve"]) && V["pen.pressureCurve"].length >= 2) {
        wb.curves = {
          ...(wb.curves || {}),
          sizeOverStroke: V["pen.pressureCurve"].map((p) => [...p]),
        };
      }

      // PBR Channels (22-channel ChannelStack -> TexturePaintWorkspace channels + values!)
      const applyStackToChannelMap = (stackFlat, linkSpec, targetMap) => {
        if (!targetMap) return;
        const read = linkSpec.Read(stackFlat);
        for (const k of Object.keys(targetMap)) targetMap[k] = false;
        for (const ch of read.on || []) targetMap[ch] = true;
        if (read.per?.roughness?.value !== undefined) wb.roughness = Clamp(read.per.roughness.value / 100, 0, 1);
        if (read.per?.metalness?.value !== undefined) wb.metalness = Clamp(read.per.metalness.value / 100, 0, 1);
        if (read.per?.height?.value !== undefined) wb.height = Clamp(read.per.height.value / 100, -1, 1);
        if (wb.extValues && read.per) {
          for (const [chId, cfg] of Object.entries(read.per)) {
            if (chId === "albedo" || chId === "roughness" || chId === "metalness" || chId === "height") continue;
            if (cfg && typeof cfg.value === "number") wb.extValues[chId] = Clamp(cfg.value / 100, 0, 1);
            else if (cfg && typeof cfg.value === "string" && cfg.value.startsWith("#")) wb.extValues[chId] = cfg.value;
            if (chId === "emissive" && typeof cfg?.strength === "number") wb.extValues.emissiveIntensity = Clamp((cfg.strength / 100) * 3, 0, 4);
          }
        }
      };

      applyStackToChannelMap(V, M.BRUSH_STACK, wb.paintChannels);
      applyStackToChannelMap(T.erase, TOOL_STACKS.erase, wb.eraseChannels);
      applyStackToChannelMap(T.fill, TOOL_STACKS.fill, wb.fillChannels);
      applyStackToChannelMap(T.gradient, TOOL_STACKS.gradient, wb.gradientChannels);
      applyStackToChannelMap(T.text, TOOL_STACKS.text, wb.textChannels);

      // Stamp settings
      wb.stampScale = Clamp((T.stamp.size ?? 24) / 60, 0.08, 1.2);
      wb.stampRotation = Number(T.stamp.rotation || 0);
      wb.stampOpacity = Clamp((T.stamp.opacity ?? 100) / 100, 0.05, 1);
      wb.stampTint = T.stamp.tint || "#ffffff";
      wb.stampHeightStrength = Clamp(((T.stamp.height ?? 15) + 100) / 200, 0, 1);

      // Text settings
      if (typeof T.text.content === "string" && T.text.content.length) wb.textContent = T.text.content;
      wb.textFontFamily = FONT_CSS_TO_WORKSPACE[T.text.font] || "DM Sans";
      wb.textFontWeight = Number(T.text.weight || 700);
      wb.textItalic = !!T.text.italic;
      wb.textPixelSize = Clamp(Number(T.text.size || 72), 18, 220);
      wb.textLetterSpacing = Number(T.text.tracking || 0);
      wb.textLineHeight = Number(T.text.leading || 1.15);
      wb.textAlign = String(T.text.align || "center").toLowerCase();
      wb.textColor = T.text.colour || wb.color;
      wb.textOutlineWidth = Number(T.text.outline || 0);
      wb.textOutlineColor = T.text.outlineColour || "#141619";
      wb.textOpacity = Clamp((T.text.opacity ?? 100) / 100, 0.05, 1);
      wb.textRotation = Number(T.text.rotation || 0);

      bridge.onBrushSynced?.({ live: !!opts.live, brush: B, tool: St.Tool });
    } finally {
      SyncLock = false;
    }
  }

  function SyncFromWorkspace() {
    if (SyncLock) return;
    SyncLock = true;
    try {
      const wb = bridge.getBrush();
      if (!wb) return;
      const B = Brush();
      if (wb.color && M.HEX.test(wb.color)) {
        St.Brushes[St.ActiveId] = M.SetValue(B, "colour.primary", wb.color.toLowerCase());
      }
      if (typeof wb.textContent === "string") St.Tools.text.content = wb.textContent;
      if (wb.textColor && M.HEX.test(wb.textColor)) St.Tools.text.colour = wb.textColor;
    } finally {
      SyncLock = false;
    }
  }

  // Initial sync so TexturePaintWorkspace receives the active brush metadata immediately
  SyncToWorkspace({ live: false });

  // ------------------------------------------------------------------------------------------------ History
  const H = History(80);
  function Step(Back) {
    const Snap = H.Step(Back, { Id: St.ActiveId, Brush: Brush() });
    if (!Snap) return;
    St.ActiveId = Snap.Id;
    St.Brushes[Snap.Id] = M.NormaliseBrush(Snap.Brush);
    EraseMemo = SmudgeMemo = null;
    Save();
    SyncToWorkspace({ live: false });
    if (St.OpenPopup) RenderAll();
  }

  // ------------------------------------------------------------------------------------------------ DOM Mount (Popover)
  let Popover = null;
  let Pad = null;
  let DragState = null;
  let CurveDrag = null;

  function RememberColour(Hex) {
    if (!M.HEX.test(Hex)) return;
    const L = Hex.toLowerCase();
    St.Colours = [L, ...St.Colours.filter((C) => C !== L)].slice(0, 12);
  }

  function SelectBrush(Id) {
    if (!St.Brushes[Id] && !paintTool(Id)) return;
    H.Begin({ Id: St.ActiveId, Brush: Brush() });
    St.ActiveId = Id;
    Brush(Id);
    const wb = bridge.getBrush();
    if (wb) wb._hasUserUploadedTip = false;
    St.Recent = [Id, ...St.Recent.filter((R) => R !== Id)].slice(0, 6);
    const Fam = Brush().family;
    if (Fam) St.Open.add(Fam);
    H.Settle({ Id, Brush: Brush() });
    Save();
    SyncToWorkspace({ live: false });
    if (St.OpenPopup) {
      RenderHead();
      RenderLibrary();
      RenderSections();
      RenderSettings();
      if (Pad && St.PadOpen) Pad.Replay();
    }
  }

  function EditBrush(Fn, { Live = false } = {}) {
    if (!Live) H.Begin({ Id: St.ActiveId, Brush: Brush() });
    St.Brushes[St.ActiveId] = Fn(Brush());
    if (!Live) H.Settle({ Id: St.ActiveId, Brush: Brush() });
    EraseMemo = SmudgeMemo = null;
    Save();
    SyncToWorkspace({ live: Live });
    if (St.OpenPopup) {
      RenderHead();
      RefreshEdited();
      PaintPreviews();
      if (Pad && St.PadOpen) Pad.Schedule();
    }
  }

  function Write(Bind, Val, { Live = false } = {}) {
    const At = ParseStackBind(Bind);
    if (At) {
      const S = StackOf(At.Base);
      if (!S) return;
      const Patch = S.L.Field(S.Flat, At.Ch, At.Key, Val);
      ApplyStackPatch(S, Patch, { Live });
      return;
    }
    const StopMatch = /^(.*):(\d+)$/.exec(Bind);
    if (StopMatch) {
      const [, BaseBind, IdxStr] = StopMatch;
      const Idx = Number(IdxStr);
      const [Sc, A0, B0] = BaseBind.split("|");
      if (Sc === "t" && Array.isArray(St.Tools[A0]?.[B0])) {
        const Next = [...St.Tools[A0][B0]];
        Next[Idx] = Val;
        Write(BaseBind, Next, { Live });
        return;
      }
    }
    const [Scope, A, B2] = Bind.split("|");
    if (Scope === "b") {
      const wb = bridge.getBrush();
      if (A === "shape.source" && wb) wb._hasUserUploadedTip = false;
      EditBrush((Br) => M.SetValue(Br, A, Val), { Live });
    } else if (Scope === "p") {
      EditBrush((Br) => M.SetParam(Br, A, Val), { Live });
    } else if (Scope === "m") {
      St.Brushes[St.ActiveId] = { ...Brush(), [A]: String(Val).slice(0, 80) };
      Save();
      SyncToWorkspace({ live: Live });
      if (St.OpenPopup) RenderHead();
    } else if (Scope === "t") {
      St.Tools = SetToolValue(St.Tools, A, B2, Val);
      EraseMemo = SmudgeMemo = null;
      SyncToWorkspace({ live: Live });
      if (St.OpenPopup) {
        if (!Live) {
          RenderHead();
          RenderSections();
          RenderSettings();
        }
        if (Pad && St.PadOpen) Pad.Schedule();
      }
    }
  }

  function ApplyStackPatch(S, Patch, { Live = false } = {}) {
    if (S.Kind === "brush") {
      EditBrush((Br) => ({ ...Br, values: M.Normalise({ ...Br.values, ...Patch }) }), { Live });
    } else {
      let Next = St.Tools;
      for (const [K, V] of Object.entries(Patch)) Next = SetToolValue(Next, S.Tool, K, V);
      St.Tools = Next;
      SyncToWorkspace({ live: Live });
      if (St.OpenPopup && !Live) {
        RenderHead();
        RenderSections();
      }
    }
  }

  // ------------------------------------------------------------------------------------------------ Field Renderers
  function SectionChangedCount(SecId) {
    const B = Brush();
    if (SecId === "quick") return M.Changed(B).size;
    return M.ChangedBySection(B)[SecId] || 0;
  }

  function BrushFieldHtml(F) {
    const B = Brush(), V = B.values, R = B.reset.values, Val = V[F.K], Edited = !Equal(Val, R[F.K]);
    const Bind = `b|${F.K}`;
    const ResetBtn = Edited ? `<button type="button" class="bs-reset" data-act="field-reset" data-bind="${Esc(Bind)}" title="Reset ${Esc(F.L)}">${icon("undo", 11)}</button>` : "";
    if (F.T === "slider") {
      const Unit = F.Unit === "°" ? "°" : F.Unit === "%" ? "%" : F.Unit || "";
      const Step = F.Step ?? 1;
      const Hint = F.Hint ? `<small>${Esc(F.Hint)}</small>` : "";
      return `<div class="mi-knob ${Edited ? "edited" : ""}" data-field="${Esc(Bind)}">
        <div class="mi-knob-label"><span>${Esc(F.L)}</span>${Hint}${ResetBtn}</div>
        <div class="slider-pill">
          <div class="split-value"><input type="number" data-bind="${Esc(Bind)}" min="${F.Min}" max="${F.Max}" step="${Step}" value="${Val}" aria-label="${Esc(F.L)}"><small>${Esc(Unit || "—")}</small></div>
          <input type="range" data-bind="${Esc(Bind)}" min="${F.Min}" max="${F.Max}" step="${Step}" value="${Val}" style="--fill:${Pct(Val, F.Min, F.Max)}" aria-label="${Esc(F.L)}">
        </div>
      </div>`;
    }
    if (F.T === "switch") {
      return `<div class="bs-switch ${Edited ? "edited" : ""}" data-field="${Esc(Bind)}">${SwitchRow("toggle", !!Val, F.L, F.Hint || "", `data-bind="${Esc(Bind)}"`)}${ResetBtn}</div>`;
    }
    if (F.T === "seg") {
      const Opts = F.Opts.map((O) => (typeof O === "string" ? [O, O] : [O.Label, O.Id]));
      return `<div class="bs-field" data-field="${Esc(Bind)}">
        <div class="bs-label ${Edited ? "edited" : ""}"><span>${Esc(F.L)}${F.Hint ? `<small>${Esc(F.Hint)}</small>` : ""}</span>${ResetBtn}</div>
        ${Segments("set", Opts, Val, { Label: F.L, Fill: F.Opts.length <= 4, Extra: `data-bind="${Esc(Bind)}"` })}
      </div>`;
    }
    if (F.T === "select") {
      const Opts = F.Opts.map((O) => (typeof O === "string" ? [O, O] : [O.Label, O.Id, null, O.Hint]));
      return `<div class="bs-field" data-field="${Esc(Bind)}">
        <div class="bs-label ${Edited ? "edited" : ""}"><span>${Esc(F.L)}${F.Hint ? `<small>${Esc(F.Hint)}</small>` : ""}</span>${ResetBtn}</div>
        ${Dropdown("set", Opts, Val, { Label: F.L, Search: Opts.length > 8, Extra: `data-bind="${Esc(Bind)}"` })}
      </div>`;
    }
    if (F.T === "source") {
      const Items = F.Kind === "grain" ? M.GRAINS : M.SHAPES;
      const ThumbFn = F.Kind === "grain" ? GrainThumb : ShapeThumb;
      const Opts = Items.map((Item) => [Item.Label, Item.Id, `<img src="${ThumbFn(Item.Id)}" alt="">`, Item.Hint]);
      return `<div class="bs-field" data-field="${Esc(Bind)}">
        <div class="bs-label ${Edited ? "edited" : ""}"><span>${Esc(F.L)}</span>${ResetBtn}</div>
        <div class="bs-source-pick">
          <img class="bs-source-big" src="${ThumbFn(Val)}" alt="${Esc(Val)}">
          ${Dropdown("set", Opts, Val, { Label: F.L, Class: "bs-dd-thumbs", Search: true, Extra: `data-bind="${Esc(Bind)}"` })}
        </div>
      </div>`;
    }
    if (F.T === "render") {
      const Opts = M.RENDER_MODES.map((Mode, I) => [Mode.Label, Mode.Id, `<span class="bs-render-art m${I}"></span>`, Mode.Hint]);
      return `<div class="bs-field" data-field="${Esc(Bind)}">
        <div class="bs-label ${Edited ? "edited" : ""}"><span>${Esc(F.L)}</span>${ResetBtn}</div>
        ${Dropdown("set", Opts, Val, { Label: F.L, Class: "bs-dd-render", Extra: `data-bind="${Esc(Bind)}"` })}
      </div>`;
    }
    if (F.T === "gradient") {
      const Opts = M.COLOUR_GRADIENT_PRESETS.map((Gr) => [Gr.label, Gr.id, `<span class="bs-dd-grad" style="background:linear-gradient(90deg,${Gr.stops.join(",")})"></span>`]);
      return `<div class="bs-field" data-field="${Esc(Bind)}">
        <div class="bs-label ${Edited ? "edited" : ""}"><span>${Esc(F.L)}</span>${ResetBtn}</div>
        ${Dropdown("set", Opts, Val, { Label: F.L, Extra: `data-bind="${Esc(Bind)}"` })}
      </div>`;
    }
    if (F.T === "colour") {
      return `<div class="bs-field" data-field="${Esc(Bind)}">
        <div class="bs-label ${Edited ? "edited" : ""}"><span>${Esc(F.L)}</span>${ResetBtn}</div>
        ${ColourField(Bind, F.L, Val || "#e8c27a", F.Hint || "sRGB")}
      </div>`;
    }
    if (F.T === "curve") {
      const Presets = Object.entries(M.CURVE_PRESETS)
        .map(([K, P]) => `<button type="button" class="bs-preset ${SameCurve(Val, P) ? "active" : ""}" data-act="curve-preset" data-bind="${Esc(Bind)}" data-value="${K}" title="${K}">${PresetMiniSvg(P)}</button>`)
        .join("");
      return `<div class="bs-curve" data-curve="${Esc(Bind)}">
        <div class="bs-label ${Edited ? "edited" : ""}"><span>${Esc(F.L)}<small>Drag points · dbl-click add/remove</small></span>${ResetBtn}</div>
        <svg class="bs-curve-svg" viewBox="0 0 320 132">${CurveSvg(Val)}</svg>
        <div class="bs-curve-presets">${Presets}</div>
      </div>`;
    }
    if (F.T === "stack") {
      const Full = M.BRUSH_STACK.Read(V), Base = M.BRUSH_STACK.Read(R);
      const EditedCh = (C) => Full.on.includes(C.Id) !== Base.on.includes(C.Id) || JSON.stringify(Full.per[C.Id]) !== JSON.stringify(Base.per[C.Id]);
      return ChannelStack({
        Value: Full,
        Ui: StackUiOf(Bind),
        Bind,
        Card: M.BRUSH_STACK.Card,
        Edited: EditedCh,
        Albedo: V["colour.primary"] || "#e8c27a",
        Title: F.L || "Paint channels",
        Hint: "this brush",
        Tone: "blue",
      });
    }
    if (F.T === "preview") {
      return `<div class="bs-preview-row">
        <canvas data-preview="${F.Kind}" width="84" height="84"></canvas>
        <div><b>Live ${F.Kind === "grain" ? "tileable grain" : "brush tip"}</b><small>${F.Kind === "grain" ? "Updates live with scale, rotation, depth & brightness." : "Updates live with hardness, roundness, rotation & invert. Applied directly to 3D + 2D painting."}</small></div>
      </div>`;
    }
    return "";
  }

  function ToolFieldHtml(Tool, F) {
    if (F.T === "brush") {
      const Key = F.K.slice(1);
      const Spec = M.FIELDS[Key];
      if (!Spec) return "";
      return BrushFieldHtml(F.L ? { ...Spec, L: F.L } : Spec);
    }
    const TVal = St.Tools[Tool];
    const Val = TVal?.[F.K];
    const Def = ToolDefault(Tool, F.K);
    const Edited = "Def" in F && JSON.stringify(Val) !== JSON.stringify(Def);
    const Bind = `t|${Tool}|${F.K}`;
    const ResetBtn = Edited ? `<button type="button" class="bs-reset" data-act="field-reset" data-bind="${Esc(Bind)}" title="Reset ${Esc(F.L || F.K)}">${icon("undo", 11)}</button>` : "";

    if (F.T === "brushcard") {
      const Br = Brush();
      const RecentHtml = St.Recent.slice(0, 5)
        .map((Id) => {
          const Rb = Brush(Id);
          return `<button type="button" class="bs-recent ${Id === St.ActiveId ? "active" : ""}" data-act="pick" data-id="${Esc(Id)}" title="${Esc(Rb.name)}"><img src="${StrokeThumb(Rb, 64, 24)}" alt="${Esc(Rb.name)}"></button>`;
        })
        .join("");
      return `<div class="bs-brushcard">
        <img class="bs-card-thumb" src="${StrokeThumb(Br, 260, 40)}" alt="${Esc(Br.name)}">
        <div class="bs-brushcard-row">
          <span><b>${Esc(Br.name)}</b><small>${Esc(FamilyLabel(Br.family))} · ${M.Changed(Br).size ? `${M.Changed(Br).size} edited` : "catalogue default"}</small></span>
          <button type="button" class="mi-text-btn" data-act="toggle-lib">${St.LibOpen ? "Hide Library" : "Brush Library"}</button>
          ${St.Studio !== "paint" ? `<button type="button" class="mi-text-btn" data-act="studio">Edit Brush</button>` : ""}
        </div>
        <div class="bs-brushcard-recent">${RecentHtml}</div>
      </div>`;
    }
    if (F.T === "colours") {
      const V = Brush().values;
      const P = V["colour.primary"] || "#e8c27a", S = V["colour.secondary"] || "#18191c";
      const Swatches = [...new Set([...St.Colours, ...PALETTE])].slice(0, 14)
        .map((C) => `<button type="button" class="bs-sw ${C.toLowerCase() === P.toLowerCase() ? "active" : ""}" style="background:${C}" data-act="swatch" data-value="${C}" title="${C}"></button>`)
        .join("");
      return `<div class="bs-colours">
        <div class="bs-discs">
          <label class="bs-disc primary" style="background:${P}" title="Primary colour"><input type="color" data-bind="b|colour.primary" value="${P}"></label>
          <label class="bs-disc secondary" style="background:${S}" title="Secondary colour"><input type="color" data-bind="b|colour.secondary" value="${S}"></label>
          <button type="button" class="icon-btn bs-swap" data-act="swap-colours" title="Swap colours">${icon("swap", 11)}</button>
        </div>
        <div class="bs-colours-side">
          <input class="mi-hex" type="text" data-bind="b|colour.primary" data-hex value="${P.toUpperCase()}" aria-label="Primary colour hex">
          <div class="bs-swatches">${Swatches}</div>
        </div>
      </div>`;
    }
    if (F.T === "slider") {
      const Step = F.Step ?? 1;
      const Unit = F.Unit ?? "%";
      return `<div class="mi-knob ${Edited ? "edited" : ""}" data-field="${Esc(Bind)}">
        <div class="mi-knob-label"><span>${Esc(F.L)}</span>${F.Hint ? `<small>${Esc(F.Hint)}</small>` : ""}${ResetBtn}</div>
        <div class="slider-pill">
          <div class="split-value"><input type="number" data-bind="${Esc(Bind)}" min="${F.Min}" max="${F.Max}" step="${Step}" value="${Val}" aria-label="${Esc(F.L)}"><small>${Esc(Unit || "—")}</small></div>
          <input type="range" data-bind="${Esc(Bind)}" min="${F.Min}" max="${F.Max}" step="${Step}" value="${Val}" style="--fill:${Pct(Val, F.Min, F.Max)}" aria-label="${Esc(F.L)}">
        </div>
      </div>`;
    }
    if (F.T === "switch") {
      return `<div class="bs-switch ${Edited ? "edited" : ""}" data-field="${Esc(Bind)}">${SwitchRow("toggle", !!Val, F.L, F.Hint || "", `data-bind="${Esc(Bind)}"`)}${ResetBtn}</div>`;
    }
    if (F.T === "seg") {
      const Opts = F.Opts.map((O) => (typeof O === "string" ? [O, O] : [O.Label, O.Id]));
      return `<div class="bs-field" data-field="${Esc(Bind)}">
        <div class="bs-label ${Edited ? "edited" : ""}"><span>${Esc(F.L)}${F.Hint ? `<small>${Esc(F.Hint)}</small>` : ""}</span>${ResetBtn}</div>
        ${Segments("set", Opts, Val, { Label: F.L, Fill: F.Opts.length <= 4, Extra: `data-bind="${Esc(Bind)}"` })}
      </div>`;
    }
    if (F.T === "select") {
      const Opts = F.Opts.map((O) => (typeof O === "string" ? [O, O] : [O.Label, O.Id, null, O.Hint]));
      return `<div class="bs-field" data-field="${Esc(Bind)}">
        <div class="bs-label ${Edited ? "edited" : ""}"><span>${Esc(F.L)}</span>${ResetBtn}</div>
        ${Dropdown("set", Opts, Val, { Label: F.L, Extra: `data-bind="${Esc(Bind)}"` })}
      </div>`;
    }
    if (F.T === "colour") {
      return `<div class="bs-field" data-field="${Esc(Bind)}">
        <div class="bs-label ${Edited ? "edited" : ""}"><span>${Esc(F.L)}</span>${ResetBtn}</div>
        ${ColourField(Bind, F.L, Val || "#e8c27a", F.Hint || "sRGB")}
      </div>`;
    }
    if (F.T === "text") {
      return `<div class="bs-field" data-field="${Esc(Bind)}">
        <div class="bs-label"><span>${Esc(F.L)}</span>${ResetBtn}</div>
        <textarea class="bs-text" data-bind="${Esc(Bind)}" placeholder="Type text to stamp…">${Esc(Val || "")}</textarea>
      </div>`;
    }
    if (F.T === "stops") {
      return `<div class="bs-field" data-field="${Esc(Bind)}">
        <div class="bs-label"><span>${Esc(F.L)}</span>${ResetBtn}</div>
        ${Stops(Bind, Val || ["#eff4cf", "#e8c27a"])}
      </div>`;
    }
    if (F.T === "source") {
      const Items = F.Kind === "grain" ? M.GRAINS : M.SHAPES;
      const ThumbFn = F.Kind === "grain" ? GrainThumb : ShapeThumb;
      return `<div class="bs-field" data-field="${Esc(Bind)}">
        <div class="bs-label"><span>${Esc(F.L)}</span>${ResetBtn}</div>
        <div class="bs-source-grid">${Items.map((Item) => `<button type="button" class="bs-source ${Val === Item.Id ? "active" : ""}" data-act="set" data-bind="${Esc(Bind)}" data-value="${Item.Id}"><img src="${ThumbFn(Item.Id)}" alt=""><span>${Esc(Item.Label)}</span></button>`).join("")}</div>
      </div>`;
    }
    if (F.T === "instrument") {
      return `<div class="bs-field" data-field="${Esc(Bind)}">
        <div class="bs-label"><span>${Esc(F.L)}</span>${ResetBtn}</div>
        <div class="bs-erasers">${ERASERS.map((Id) => {
          const Info = paintTool(Id);
          return `<button type="button" class="bs-eraser ${Val === Id ? "active" : ""}" data-act="set" data-bind="${Esc(Bind)}" data-value="${Esc(Id)}"><img src="${toolArtURL(Info)}" alt=""><span>${Esc(Info?.label || Id)}</span></button>`;
        }).join("")}</div>
      </div>`;
    }
    if (F.T === "stack") {
      const L = F.Stack || TOOL_STACKS[Tool];
      if (!L) return "";
      const Full = L.Read(TVal), Base = L.Read(CreateTools()[Tool]);
      const EditedCh = (C) => Full.on.includes(C.Id) !== Base.on.includes(C.Id) || JSON.stringify(Full.per[C.Id]) !== JSON.stringify(Base.per[C.Id]);
      return ChannelStack({
        Value: Full,
        Ui: StackUiOf(Bind),
        Bind,
        Card: L.Card,
        Edited: EditedCh,
        Albedo: Brush().values["colour.primary"] || "#e8c27a",
        Title: F.L || "Channels",
        Hint: `${TOOL[Tool]?.Label?.toLowerCase() || Tool} tool`,
        Tone: "blue",
      });
    }
    if (F.T === "sample") {
      const Last = TVal.last;
      if (!Last?.Hex) {
        return `<div class="bs-sample empty"><i></i><span><b>No sample yet</b><small>Click anywhere on the 3D mesh, 2D UV canvas, or preview pad to sample PBR colour.</small></span></div>`;
      }
      const [R, G, B2] = HexToRgb(Last.Hex);
      const [Hu, Sa, Li] = RgbToHsl(R, G, B2);
      return `<div class="bs-sample">
        <i style="background:${Last.Hex}"></i>
        <span><b>${Last.Hex.toUpperCase()}</b><small>RGB ${R}, ${G}, ${B2} · HSL ${Hu}° ${Sa}% ${Li}%${Last.meta ? ` · ${Esc(Last.meta)}` : ""}</small></span>
        <div class="bs-sample-use">
          <button type="button" class="mi-text-btn" data-act="use-sample" data-value="primary">Use as Primary</button>
          <button type="button" class="mi-text-btn" data-act="use-sample" data-value="secondary">Use as Secondary</button>
        </div>
      </div>`;
    }
    if (F.T === "history") {
      const Hist = TVal.history || [];
      return Hist.length
        ? `<div class="bs-swatches big">${Hist.map((C) => `<button type="button" class="bs-sw" style="background:${C}" data-act="swatch" data-value="${C}" title="${C}"></button>`).join("")}</div>`
        : `<p class="mi-note">Sampled colours appear here automatically.</p>`;
    }
    if (F.T === "nudge") {
      return `<div class="bs-nudge">
        <button type="button" class="icon-btn" data-act="nudge" data-x="-1" data-y="0" title="Nudge left">←</button>
        <button type="button" class="icon-btn" data-act="nudge" data-x="1" data-y="0" title="Nudge right">→</button>
        <button type="button" class="icon-btn" data-act="nudge" data-x="0" data-y="-1" title="Nudge up">↑</button>
        <button type="button" class="icon-btn" data-act="nudge" data-x="0" data-y="1" title="Nudge down">↓</button>
        <small>Offset ${TVal.dx || 0}, ${TVal.dy || 0} px</small>
      </div>`;
    }
    if (F.T === "readout") {
      const Info = bridge.getToolReadout?.(Tool) || TOOL[Tool]?.Verb || "";
      return `<p class="bs-readout">${Esc(Info)}</p>`;
    }
    if (F.T === "actions") {
      return `<div class="bs-actions">${F.Items.map(([Label, Act, Primary]) => `<button type="button" class="${Primary ? "primary" : ""}" data-act="${Esc(Act)}">${Esc(Label)}</button>`).join("")}</div>`;
    }
    return "";
  }

  function InstrumentSectionHtml() {
    const B = Brush(), ToolInfo = paintTool(B.instrument), Schema = toolSchema(ToolInfo);
    const Fields = (Schema?.fields || []).map((F) => {
      const Bind = `p|${F.key}`, Val = B.params[F.key] ?? F.default;
      const Edited = !Equal(Val, B.reset.params[F.key]);
      if (F.type === "toggle") return `<div class="bs-switch ${Edited ? "edited" : ""}">${SwitchRow("toggle", !!Val, F.label, "", `data-bind="${Esc(Bind)}"`)}</div>`;
      if (F.type === "select") {
        const Opts = (F.options || []).map((O) => (typeof O === "string" ? [O, O] : [O.label, O.value]));
        return `<div class="bs-field"><div class="bs-label ${Edited ? "edited" : ""}"><span>${Esc(F.label)}</span></div>${Dropdown("set", Opts, Val, { Label: F.label, Extra: `data-bind="${Esc(Bind)}"` })}</div>`;
      }
      return `<div class="mi-knob ${Edited ? "edited" : ""}"><div class="mi-knob-label"><span>${Esc(F.label)}</span></div><div class="slider-pill">
        <div class="split-value"><input type="number" data-bind="${Esc(Bind)}" min="${F.min ?? 0}" max="${F.max ?? 1}" step="${F.step ?? 0.01}" value="${Val}"><small>${Esc(F.unit || "—")}</small></div>
        <input type="range" data-bind="${Esc(Bind)}" min="${F.min ?? 0}" max="${F.max ?? 1}" step="${F.step ?? 0.01}" value="${Val}" style="--fill:${Pct(Val, F.min ?? 0, F.max ?? 1)}">
      </div></div>`;
    }).join("");
    return `<div class="bs-instrument">
      ${ToolInfo ? `<img src="${toolArtURL(ToolInfo)}" alt="${Esc(ToolInfo.label || B.name)}">` : ""}
      <div><b>${Esc(ToolInfo?.label || B.name)}</b><small>${Esc(ToolInfo?.family ? `${ToolInfo.family}${ToolInfo.nib ? ` · ${ToolInfo.nib}` : ""}` : "Procedural catalogue instrument mapped live into the 3D/2D GPU brush engine.")}</small></div>
    </div>
    ${Fields ? `<div class="bs-group"><div class="bs-group-title"><b>Instrument Controls</b><small>Synced to Brush Studio</small></div>${Fields}</div>` : `<p class="mi-note">This brush has no extra catalogue-specific parameters.</p>`}`;
  }

  function AboutSectionHtml() {
    const B = Brush(), Diff = M.Changed(B);
    const BySecMap = M.ChangedBySection(B); const BySec = M.SECTIONS.map((S) => [S, BySecMap[S.Id] || 0]).filter(([, N]) => N > 0);
    return `<div class="bs-about-card">
      <img class="bs-about-stroke" src="${StrokeThumb(B, 340, 62)}" alt="${Esc(B.name)}">
      <div class="bs-about-meta">
        <canvas class="bs-about-stamp" data-preview="shape" width="52" height="52"></canvas>
        <div><b>${Esc(B.name)}</b><small>${Esc(B.family)} · ${Diff.size ? `${Diff.size} modified settings` : "Matches reset point"}</small></div>
      </div>
      ${B.signature ? `<div class="bs-signature">${Esc(B.signature)}</div>` : ""}
    </div>
    <div class="bs-group">
      <div class="bs-group-title"><b>Identity</b></div>
      <div class="bs-field"><div class="bs-label"><span>Brush name</span></div><input class="bs-input" type="text" data-bind="m|name" value="${Esc(B.name)}"></div>
      <div class="bs-field"><div class="bs-label"><span>Author</span></div><input class="bs-input" type="text" data-bind="m|author" value="${Esc(B.author || "")}"></div>
      <div class="bs-field"><div class="bs-label"><span>Signature</span></div><input class="bs-input script" type="text" data-bind="m|signature" value="${Esc(B.signature || "")}"></div>
    </div>
    ${BySec.length ? `<div class="bs-group"><div class="bs-group-title"><b>Modified Sections</b><small>${Diff.size} total</small></div>${BySec.map(([S, N]) => `<button type="button" class="bs-about-row" data-act="section" data-section="${S.Id}"><i style="width:${Math.min(100, N * 18)}%"></i><span>${Esc(S.Label)}</span><b>${N}</b></button>`).join("")}</div>` : ""}
    <div class="bs-group">
      <div class="bs-group-title"><b>Brush Actions</b></div>
      <div class="bs-actions">
        <button type="button" class="primary" data-act="duplicate">Duplicate to My Brushes</button>
        <button type="button" data-act="reset-point">Save Reset Point</button>
      </div>
      <div class="bs-actions">
        <button type="button" data-act="reset-all">Reset Brush</button>
        <button type="button" data-act="export">Export .brush.json</button>
        <button type="button" data-act="import">Import JSON…</button>
      </div>
    </div>`;
  }

  // ------------------------------------------------------------------------------------------------ Render Parts
  function RenderHead() {
    if (!Popover) return;
    const El = Popover.querySelector(".tsr-bp-head");
    if (!El) return;
    const ToolMeta = TOOL[St.Studio] || TOOL.paint;
    const B = Brush();
    const DiffN = IsBrushStudio() ? M.Changed(B).size : ToolChanged(St.Studio, St.Tools[St.Studio]).size;
    const Sub = IsBrushStudio()
      ? `${B.name} · ${DiffN ? `${DiffN} edited` : "102-brush catalogue"}`
      : `${ToolMeta.Verb}${DiffN ? ` · ${DiffN} edited` : ""}`;
    El.innerHTML = `
      <span class="bs-glyph" style="--c:${TOOL_COLOURS[St.Studio] || "var(--warm)"}">${icon(ToolMeta.Glyph, 17)}</span>
      <div class="tsr-bp-title">
        <b>${Esc(STUDIOS[St.Studio]?.Title || `${ToolMeta.Label} Studio`)}</b>
        <small>${Esc(Sub)}</small>
      </div>
      <div class="tsr-bp-tools">
        <button type="button" class="icon-btn ${St.LibOpen ? "on" : ""}" data-act="toggle-lib" title="Toggle 102-Brush Library">${icon("grid", 14)}</button>
        <button type="button" class="icon-btn ${St.PadOpen ? "on" : ""}" data-act="pad-toggle" title="Toggle Live Drawing Test Pad">${icon("brush", 14)}</button>
        <button type="button" class="icon-btn" data-act="undo" ${H.Past.length > 0 ? "" : "disabled"} title="Undo Brush Change">${icon("undo", 14)}</button>
        <button type="button" class="icon-btn" data-act="redo" ${H.Future.length > 0 ? "" : "disabled"} title="Redo Brush Change">${icon("redo", 14)}</button>
        <button type="button" class="tsr-bp-done" data-act="close-popup" title="Close (Esc / Q)">Done</button>
      </div>`;
  }

  function RenderToolRail() {
    if (!Popover) return;
    const Rail = Popover.querySelector(".tsr-bp-rail");
    if (!Rail) return;
    Rail.innerHTML = TOOLS.map((T, Idx) => {
      const Sep = (Idx === 3 || Idx === 8) ? `<span class="tsr-bp-rail-sep"></span>` : "";
      const On = St.Tool === T.Id;
      return `${Sep}<button type="button" class="tsr-bp-rail-btn ${On ? "active" : ""}" data-act="tool" data-tool="${T.Id}" title="${Esc(T.Label)} (${T.Key})">
        <span class="bs-glyph" style="--c:${TOOL_COLOURS[T.Id] || "var(--text-soft)"}">${icon(T.Glyph, 13)}</span>
        <span>${Esc(T.Label)}</span>
      </button>`;
    }).join("");
  }

  function RenderLibrary() {
    if (!Popover) return;
    const Lib = Popover.querySelector(".tsr-bp-lib");
    if (!Lib) return;
    Lib.hidden = !St.LibOpen;
    Popover.classList.toggle("with-lib", St.LibOpen);
    if (!St.LibOpen) return;

    const Q = St.Filter.trim().toLowerCase();
    const Fams = M.PAINT_FAMILIES || [];
    const Match = (T) => !Q || T.name.toLowerCase().includes(Q) || (T.family || "").toLowerCase().includes(Q);

    let Html = "";
    if (St.Custom.length) {
      const Mine = St.Custom.map((Id) => Brush(Id)).filter(Match);
      if (Mine.length) {
        Html += `<div class="bs-fam open">
          <div class="bs-fam-head"><i style="background:var(--highlight-bg)"></i><b>My Brushes</b><small>${Mine.length}</small></div>
          ${Mine.map((B) => `<button type="button" class="bs-lib-row ${B.id === St.ActiveId ? "active" : ""}" data-act="pick" data-id="${Esc(B.id)}">
            <img class="bs-thumb" src="${StrokeThumb(B)}" alt="">
            <span class="bs-lib-name"><i class="bs-dot"></i><b>${Esc(B.name)}</b></span>
          </button>`).join("")}
        </div>`;
      }
    }

    for (const Fam of Fams) {
      const Items = M.PAINT_TOOLS.filter((T) => T.family === Fam.key && Match(T));
      if (!Items.length) continue;
      const Open = !!Q || St.Open.has(Fam.key);
      Html += `<div class="bs-fam ${Open ? "open" : ""}">
        <button type="button" class="bs-fam-head" data-act="family" data-family="${Esc(Fam.key)}">
          <span class="mi-chev">${icon("chevron", 11)}</span>
          <i style="background:${Fam.color || "var(--warm)"}"></i>
          <b>${Esc(Fam.label)}</b>
          <small>${Items.length}</small>
        </button>
        ${Open ? Items.map((T) => {
          const B = Brush(T.id);
          const Dirty = M.Changed(B).size > 0;
          return `<button type="button" class="bs-lib-row ${B.id === St.ActiveId ? "active" : ""}" data-act="pick" data-id="${Esc(B.id)}">
            <img class="bs-thumb" src="${StrokeThumb(B)}" alt="">
            <span class="bs-lib-name">${Dirty ? `<i class="bs-dot"></i>` : ""}<b>${Esc(B.name)}</b></span>
          </button>`;
        }).join("") : ""}
      </div>`;
    }

    const ListEl = Lib.querySelector(".tsr-bp-lib-list");
    if (ListEl) ListEl.innerHTML = Html || `<p class="mi-empty">No brushes match “${Esc(St.Filter)}”.</p>`;
    const CountEl = Lib.querySelector(".tsr-bp-lib-count");
    if (CountEl) CountEl.textContent = `${M.PAINT_TOOLS.length + St.Custom.length}`;
  }

  function RenderSections() {
    if (!Popover) return;
    const Rail = Popover.querySelector(".bs-sections");
    if (!Rail) return;
    if (IsBrushStudio()) {
      const QuickRow = `<button type="button" class="bs-sec ${St.Section === "quick" ? "active" : ""}" data-act="section" data-section="quick">
        <span class="bs-glyph" style="--c:var(--highlight-bg)">${icon("spark", 14)}</span>
        <span class="bs-sec-label">Quick Context</span>
      </button>`;
      const SecRows = M.SECTIONS.map((S) => {
        const N = SectionChangedCount(S.Id);
        return `<button type="button" class="bs-sec ${St.Section === S.Id ? "active" : ""}" data-act="section" data-section="${S.Id}">
          <span class="bs-glyph" style="--c:${SECTION_COLOURS[S.Id] || "var(--text-soft)"}">${icon(S.Glyph, 14)}</span>
          <span class="bs-sec-label">${Esc(S.Label)}</span>
          ${N ? `<i class="bs-sec-count">${N}</i>` : ""}
        </button>`;
      }).join("");
      Rail.innerHTML = `<span class="bs-sec-cap">Quick</span>${QuickRow}<span class="bs-sec-cap">Brush Studio</span>${SecRows}`;
    } else {
      const Studio = STUDIOS[St.Studio];
      const ActiveSec = St.ToolSection[St.Studio] || Studio.Sections[0].Id;
      const Counts = ToolChangedBySection(St.Studio, St.Tools[St.Studio]);
      Rail.innerHTML = `<span class="bs-sec-cap">${Esc(TOOL[St.Studio]?.Label || "Tool")} Studio</span>` +
        Studio.Sections.map((S) => {
          const N = Counts[S.Id] || 0;
          return `<button type="button" class="bs-sec ${ActiveSec === S.Id ? "active" : ""}" data-act="section" data-section="${S.Id}">
            <span class="bs-glyph" style="--c:${TOOL_COLOURS[St.Studio] || "var(--warm)"}">${icon(S.Glyph, 14)}</span>
            <span class="bs-sec-label">${Esc(S.Label)}</span>
            ${N ? `<i class="bs-sec-count">${N}</i>` : ""}
          </button>`;
        }).join("") +
        `<span class="bs-sec-cap">Shared</span>
        <button type="button" class="bs-sec" data-act="studio">
          <span class="bs-glyph" style="--c:var(--warm)">${icon("brush", 14)}</span>
          <span class="bs-sec-label">Open Brush Studio</span>
        </button>`;
    }
  }

  function WorkspaceExtraActionsHtml(ToolId) {
    if (ToolId === "paint") {
      return `<div class="bs-group"><div class="bs-group-title"><b>3D / 2D Canvas Actions</b></div>
        <div class="bs-actions">
          <button type="button" data-act="ws-upload-tip">Load Custom Alpha Tip…</button>
          <button type="button" data-act="toggle-lib">${St.LibOpen ? "Hide Brush Library" : "Browse 102 Brushes"}</button>
        </div>
      </div>`;
    }
    if (ToolId === "stamp") {
      return `<div class="bs-group"><div class="bs-group-title"><b>3D / 2D Stamp Actions</b></div>
        <div class="bs-actions">
          <button type="button" class="primary" data-act="ws-place-stamp">Place Stamp at UV Center</button>
          <button type="button" data-act="ws-upload-stamp">Import Stamp Image…</button>
        </div>
      </div>`;
    }
    if (ToolId === "text") {
      return `<div class="bs-group"><div class="bs-group-title"><b>3D / 2D Text Actions</b></div>
        <div class="bs-actions">
          <button type="button" class="primary" data-act="ws-stamp-text">Stamp Text at UV Center</button>
        </div>
      </div>`;
    }
    if (ToolId === "curve") {
      return `<div class="bs-group"><div class="bs-group-title"><b>3D Surface Curve Actions</b></div>
        <div class="bs-actions">
          <button type="button" class="primary" data-act="ws-curve-stroke">Stroke 3D Curve (Enter)</button>
          <button type="button" data-act="ws-curve-undo">Undo Pt</button>
          <button type="button" data-act="ws-curve-clear">Clear</button>
        </div>
      </div>`;
    }
    if (ToolId === "gradient") {
      return `<div class="bs-group"><div class="bs-group-title"><b>3D Space Gradient Actions</b></div>
        <div class="bs-actions">
          <button type="button" class="primary" data-act="ws-gradient-apply">Bake 3D Gradient Now</button>
        </div>
      </div>`;
    }
    if (ToolId === "fill") {
      return `<div class="bs-group"><div class="bs-group-title"><b>Fill Scope (3D / 2D Mesh)</b></div>
        ${Segments("set", [["Part", "Part"], ["Island", "Island"], ["All", "All"]], bridge.getFillScopeLabel?.() || "Part", { Label: "Fill Scope", Fill: true, Extra: `data-bind="ws|fillScope"` })}
      </div>`;
    }
    return "";
  }

  function RenderSettings() {
    if (!Popover) return;
    const Panel = Popover.querySelector(".bs-settings");
    if (!Panel) return;

    if (IsBrushStudio()) {
      if (St.Section === "quick") {
        const GroupsHtml = QuickBrushGroups.map((G) => `<div class="bs-group">
          <div class="bs-group-title"><b>${Esc(G.Title)}</b>${G.Hint ? `<small>${Esc(G.Hint)}</small>` : ""}</div>
          ${G.Fields.map((F) => ToolFieldHtml("paint", F)).join("")}
        </div>`).join("");
        Panel.innerHTML = `<div class="bs-set-head">
          <span class="bs-glyph" style="--c:var(--highlight-bg)">${icon("spark", 18)}</span>
          <div><h2>Quick Context</h2><small>Live brush controls · Right-click or Q anytime</small></div>
          <button type="button" class="mi-text-btn" data-act="reset-all">Reset brush</button>
        </div>
        <div class="bs-set-body">${GroupsHtml}${WorkspaceExtraActionsHtml("paint")}</div>`;
      } else {
        const Sec = M.SECTIONS.find((S) => S.Id === St.Section) || M.SECTIONS[0];
        let Body = "";
        if (Sec.Id === "instrument") Body = InstrumentSectionHtml();
        else if (Sec.Id === "about") Body = AboutSectionHtml();
        else {
          const V = Brush().values;
          Body = Sec.Groups.map((G) => {
            const Fields = G.Fields.filter((F) => !F.When || F.When(V));
            if (!Fields.length) return "";
            return `<div class="bs-group">
              <div class="bs-group-title"><b>${Esc(G.Title)}</b>${G.Hint ? `<small>${Esc(G.Hint)}</small>` : ""}</div>
              ${Fields.map(BrushFieldHtml).join("")}
            </div>`;
          }).join("");
          if (Sec.Id === "shape") Body += WorkspaceExtraActionsHtml("paint");
        }
        Panel.innerHTML = `<div class="bs-set-head">
          <span class="bs-glyph" style="--c:${SECTION_COLOURS[Sec.Id] || "var(--warm)"}">${icon(Sec.Glyph, 18)}</span>
          <div><h2>${Esc(Sec.Label)}</h2><small>${Esc(Sec.Hint || "")}</small></div>
          ${Sec.Id !== "about" ? `<button type="button" class="mi-text-btn" data-act="reset-section">Reset section</button>` : ""}
        </div>
        <div class="bs-set-body">${Body}</div>`;
      }
    } else {
      const Studio = STUDIOS[St.Studio];
      const SecId = St.ToolSection[St.Studio] || Studio.Sections[0].Id;
      const Sec = Studio.Sections.find((S) => S.Id === SecId) || Studio.Sections[0];
      const TVal = St.Tools[St.Studio];
      let Body = "";
      if (Sec.Id === "about") {
        const TM = TOOL[St.Studio];
        Body = `<div class="bs-tool-card">
          <span class="bs-glyph" style="--c:${TOOL_COLOURS[St.Studio]}">${icon(TM.Glyph, 28)}</span>
          <div><b>${Esc(TM.Label)}</b><small>${Esc(TM.Verb)} · Shortcut <kbd>${Esc(TM.Key)}</kbd></small></div>
        </div>
        <div class="bs-group"><div class="bs-group-title"><b>Reset</b></div>
          <div class="bs-actions"><button type="button" data-act="tool-reset">Reset ${Esc(TM.Label)} defaults</button></div>
        </div>`;
      } else {
        const Groups = SectionGroups(St.Studio, Sec);
        Body = Groups.map((G) => {
          const Fields = VisibleToolFields(G, TVal, Brush().values);
          if (!Fields.length) return "";
          return `<div class="bs-group">
            <div class="bs-group-title"><b>${Esc(G.Title)}</b>${G.Hint ? `<small>${Esc(G.Hint)}</small>` : ""}</div>
            ${Fields.map((F) => ToolFieldHtml(St.Studio, F)).join("")}
          </div>`;
        }).join("") + WorkspaceExtraActionsHtml(St.Studio);
      }
      Panel.innerHTML = `<div class="bs-set-head">
        <span class="bs-glyph" style="--c:${TOOL_COLOURS[St.Studio] || "var(--warm)"}">${icon(Sec.Glyph, 18)}</span>
        <div><h2>${Esc(Sec.Label)}</h2><small>${Esc(Sec.Hint || TOOL[St.Studio]?.Verb || "")}</small></div>
        ${Sec.Id !== "about" ? `<button type="button" class="mi-text-btn" data-act="reset-section">Reset section</button>` : ""}
      </div>
      <div class="bs-set-body">${Body}</div>`;
    }

    PaintPreviews();
  }

  function PaintPreviews() {
    if (!Popover) return;
    const V = Brush().values;
    for (const Cv of Popover.querySelectorAll("canvas[data-preview]")) {
      if (Cv.dataset.preview === "grain") PaintGrainPreview(Cv, V);
      else PaintStampPreview(Cv, V);
    }
  }

  function RefreshEdited() {
    if (!Popover) return;
    RenderSections();
  }

  function RenderPadBar() {
    if (!Popover) return;
    const PadCol = Popover.querySelector(".tsr-bp-pad");
    if (!PadCol) return;
    PadCol.hidden = !St.PadOpen;
    Popover.classList.toggle("with-pad", St.PadOpen);
    if (!St.PadOpen) return;
    const Bar = PadCol.querySelector(".bs-padbar");
    if (Bar) {
      const Papers = [["paper", "#efece4", "Paper"], ["grey", "#8a8a86", "Mid grey"], ["dark", "#141518", "Dark"]];
      Bar.innerHTML = `
        <div class="bs-padbar-title"><b>Live Test Pad</b><small>Draw to test stroke & channels</small></div>
        <div class="bs-papers">${Papers.map(([Id, Col, L]) => `<button type="button" class="bs-paper ${St.Paper === Id ? "active" : ""}" style="--p:${Col}" data-act="paper" data-value="${Id}" title="${L}"></button>`).join("")}</div>
        <button type="button" class="bs-ibtn" data-act="clear" title="Clear Pad">${icon("trash", 13)}</button>`;
    }
    const Stage = PadCol.querySelector(".bs-stage");
    if (Stage) Stage.dataset.paper = St.Paper;
  }

  function RenderAll() {
    if (!Popover) return;
    Popover.classList.toggle("compact-tool", !IsBrushStudio());
    RenderHead();
    RenderToolRail();
    RenderLibrary();
    RenderSections();
    RenderSettings();
    RenderPadBar();
  }

  function RedrawCurves() {
    if (!Popover) return;
    for (const C of Popover.querySelectorAll("[data-curve]")) {
      const Key = C.dataset.curve.slice(2);
      const Svg = C.querySelector("svg.bs-curve-svg");
      if (Svg) Svg.innerHTML = CurveSvg(Brush().values[Key]);
    }
    RefreshEdited();
  }

  // ------------------------------------------------------------------------------------------------ Create & Position Popover
  function ClampPosition(x, y) {
    if (!Popover) return;
    const Rect = Popover.getBoundingClientRect();
    const W = Rect.width || 820;
    const H2 = Rect.height || 600;
    const MaxX = Math.max(12, window.innerWidth - W - 12);
    const MaxY = Math.max(58, window.innerHeight - H2 - 16);
    const L = Math.round(Clamp(x ?? (window.innerWidth - W) * 0.5, 12, MaxX));
    const T = Math.round(Clamp(y ?? 64, 58, MaxY));
    Popover.style.left = `${L}px`;
    Popover.style.top = `${T}px`;
    St.PopupPos = { x: L, y: T };
  }

  function EnsurePopover() {
    if (Popover) return Popover;
    Popover = document.createElement("section");
    Popover.className = "tsr-scope tsr-brush-popover";
    Popover.setAttribute("role", "dialog");
    Popover.setAttribute("aria-label", "Brush Studio and Tool Context Menu");
    Popover.innerHTML = `
      <header class="tsr-bp-head"></header>
      <nav class="tsr-bp-rail" aria-label="Paint Tools"></nav>
      <div class="tsr-bp-body">
        <aside class="tsr-bp-lib" hidden>
          <div class="tsr-bp-lib-top"><b>Brush Library</b><small class="tsr-bp-lib-count">102</small></div>
          <label class="mi-search tsr-bp-lib-search">${icon("search", 13)}<input type="search" data-act="filter" placeholder="Search 102 brushes…" aria-label="Search brushes"></label>
          <div class="tsr-bp-lib-list"></div>
        </aside>
        <nav class="bs-sections" aria-label="Studio Sections"></nav>
        <div class="bs-settings"></div>
        <aside class="tsr-bp-pad" hidden>
          <div class="bs-padbar"></div>
          <div class="bs-stage" data-paper="paper">
            <canvas class="bs-canvas"></canvas>
            <canvas class="bs-guide"></canvas>
          </div>
          <div class="bs-foot"><span class="bs-stat"><b>Live</b>Preview synchronized to 3D/2D canvas</span></div>
        </aside>
      </div>
      <input type="file" class="bs-file" accept=".json,application/json" hidden>
    `;

    // Mount Pad previewer onto the hidden pad stage
    const PadStage = Popover.querySelector(".bs-stage");
    Pad = CreatePad(PadStage, {
      Brush,
      Tools: () => St.Tools,
      Tool: () => St.Tool,
      View: () => "albedo",
      Live: () => St.Live,
      Preview: () => St.Preview,
      EraserBrush: EraseBrush,
      SmudgeBrush: SmudgeBrush,
      Emit: (Kind, Data) => {
        if (Kind === "sample" && Data?.Hex) {
          recordSampledColour(Data.Hex, "from pad");
        } else if (Kind === "tool-set" && Array.isArray(Data)) {
          const [Tool, Key, Value] = Data;
          St.Tools = { ...St.Tools, [Tool]: { ...St.Tools[Tool], [Key]: Value } };
          Pad?.Schedule?.();
          SyncToWorkspace({ live: true });
        }
      },
    });

    // Header drag-to-reposition
    const Head = Popover.querySelector(".tsr-bp-head");
    Head.addEventListener("pointerdown", (E) => {
      if (E.target.closest("button")) return;
      const R = Popover.getBoundingClientRect();
      DragState = { ox: E.clientX - R.left, oy: E.clientY - R.top };
      Head.setPointerCapture?.(E.pointerId);
    });
    Head.addEventListener("pointermove", (E) => {
      if (!DragState) return;
      ClampPosition(E.clientX - DragState.ox, E.clientY - DragState.oy);
    });
    Head.addEventListener("pointerup", () => { DragState = null; });

    // Input events
    Popover.addEventListener("input", (E) => {
      const El = E.target;
      if (El.dataset.act === "cs-search") {
        StackUiOf(El.dataset.bind).Query = El.value;
        const Pos = El.selectionStart;
        RenderSettings();
        const N = Popover.querySelector(`[data-act="cs-search"][data-bind="${CSS.escape(El.dataset.bind)}"]`);
        N?.focus();
        try { N?.setSelectionRange(Pos, Pos); } catch {}
        return;
      }
      if (El.dataset.act === "filter") {
        St.Filter = El.value;
        const Pos = El.selectionStart;
        RenderLibrary();
        const N = Popover.querySelector('[data-act="filter"]');
        N?.focus();
        try { N?.setSelectionRange(Pos, Pos); } catch {}
        return;
      }
      const Bind = El.dataset.bind;
      if (!Bind || El.type === "file") return;
      if (El.dataset.hex !== undefined) {
        const V = El.value.trim().startsWith("#") ? El.value.trim() : `#${El.value.trim()}`;
        if (M.HEX.test(V)) {
          Write(Bind, V.toLowerCase(), { Live: true });
          SyncSiblings(Popover, El);
        }
        return;
      }
      if (El.type === "range" || El.type === "number" || El.type === "color") {
        SyncSiblings(Popover, El);
        if (El.type === "color") {
          const Disc = El.closest(".bs-disc");
          if (Disc) Disc.style.background = El.value;
        }
        Write(Bind, El.value, { Live: true });
        return;
      }
      if (El.tagName === "TEXTAREA" || El.tagName === "INPUT") {
        Write(Bind, El.value, { Live: true });
      }
    });

    Popover.addEventListener("change", (E) => {
      const El = E.target;
      if (El.classList.contains("bs-file")) {
        const F = El.files?.[0];
        if (F) {
          F.text().then((Txt) => {
            try {
              const B = M.Import(Txt);
              if (St.Brushes[B.id] || paintTool(B.id)) B.id = `${B.instrument}~${Math.random().toString(36).slice(2, 7)}`;
              St.Brushes[B.id] = B;
              St.Custom = [B.id, ...St.Custom];
              SelectBrush(B.id);
              bridge.toast?.(`Imported brush "${B.name}"`);
            } catch (Err) {
              bridge.toast?.(`Brush import failed: ${Err.message}`);
            }
          });
        }
        El.value = "";
        return;
      }
      const Bind = El.dataset.bind;
      if (!Bind) return;
      if (El.tagName === "SELECT") { Write(Bind, El.value); return; }
      if (El.type === "color" && Bind.startsWith("b|")) RememberColour(El.value);
      if (Bind.startsWith("b|") || Bind.startsWith("p|") || Bind.startsWith("m|")) {
        H.Settle({ Id: St.ActiveId, Brush: Brush() });
        RenderHead();
      }
    });

    // Click actions
    Popover.addEventListener("click", (E) => {
      const El = E.target.closest("[data-act]");
      if (!El || El.disabled || El.tagName === "INPUT") return;
      const Act = El.dataset.act;
      const Bind = El.dataset.bind;

      if (Bind === "ws|fillScope" && Act === "set") {
        bridge.setFillScope?.(String(El.dataset.value || "Part").toLowerCase());
        RenderSettings();
        return;
      }

      switch (Act) {
        case "close-popup":
          close();
          break;
        case "toggle-lib":
          St.LibOpen = !St.LibOpen;
          RenderHead();
          RenderLibrary();
          RenderSettings();
          ClampPosition(St.PopupPos?.x, St.PopupPos?.y);
          break;
        case "pad-toggle":
          St.PadOpen = !St.PadOpen;
          RenderHead();
          RenderPadBar();
          ClampPosition(St.PopupPos?.x, St.PopupPos?.y);
          if (St.PadOpen && Pad) {
            setTimeout(() => { Pad.Resize(); Pad.Replay(); }, 20);
          }
          break;
        case "pick":
          SelectBrush(El.dataset.id);
          break;
        case "family": {
          const K = El.dataset.family;
          St.Open.has(K) ? St.Open.delete(K) : St.Open.add(K);
          RenderLibrary();
          break;
        }
        case "section": {
          const Id = El.dataset.section;
          if (IsBrushStudio()) St.Section = Id;
          else St.ToolSection[St.Studio] = Id;
          RenderSections();
          RenderSettings();
          const SetEl = Popover.querySelector(".bs-settings");
          if (SetEl) SetEl.scrollTop = 0;
          break;
        }
        case "tool": {
          const NextTool = El.dataset.tool;
          setTool(NextTool, { notifyWorkspace: true });
          break;
        }
        case "studio":
          St.Studio = "paint";
          St.Section = "stroke";
          RenderAll();
          break;
        case "set":
          Write(Bind, El.dataset.value);
          RenderSettings();
          break;
        case "toggle": {
          const [Scope, A, B2] = Bind.split("|");
          if (Scope === "b") EditBrush((Br) => M.SetValue(Br, A, !Br.values[A]));
          else if (Scope === "p") EditBrush((Br) => M.SetParam(Br, A, !Br.params[A]));
          else if (Scope === "t") Write(Bind, !St.Tools[A][B2]);
          RenderSettings();
          break;
        }
        case "cs-open":
        case "cs-group":
        case "cs-toggle":
        case "cs-group-toggle":
        case "cs-bulk": {
          const S = StackOf(Bind);
          if (!S) break;
          const Res = StackAction(Act, El, S.L.Read(S.Flat), StackUiOf(Bind), S.L.Card);
          if (Res?.Refused) bridge.toast?.(Res.Refused);
          else if (Res?.Next) ApplyStackPatch(S, S.L.Assign(Res.Next));
          RenderSettings();
          break;
        }
        case "curve-preset": {
          const [, Key] = Bind.split("|");
          EditBrush((Br) => M.SetValue(Br, Key, structuredClone(M.CURVE_PRESETS[El.dataset.value])));
          RedrawCurves();
          break;
        }
        case "stop-add":
        case "stop-remove": {
          const Target = El.dataset.target;
          const [Scope, A, B2] = Target.split("|");
          const List = Scope === "b" ? Brush().values[A] : St.Tools[A][B2];
          const Next = Act === "stop-add" ? [...List, List.at(-1)] : List.slice(0, -1);
          Write(Target, Next);
          RenderSettings();
          break;
        }
        case "field-reset": {
          const At = ParseStackBind(Bind);
          if (At) {
            const S = StackOf(At.Base);
            if (S) {
              ApplyStackPatch(S, S.L.Field(S.Flat, At.Ch, At.Key, S.L.Read(S.Reset).per[At.Ch]?.[At.Key]));
              RenderSettings();
            }
            break;
          }
          const [Scope, A, B2] = Bind.split("|"), Br = Brush();
          if (Scope === "b") EditBrush((B0) => ({ ...B0, values: M.Normalise({ ...B0.values, [A]: structuredClone(Br.reset.values[A]) }) }));
          else if (Scope === "p") EditBrush((B0) => M.SetParam(B0, A, Br.reset.params[A]));
          else if (Scope === "t") Write(Bind, ToolDefault(A, B2));
          RenderSettings();
          break;
        }
        case "reset-section": {
          if (IsBrushStudio()) {
            const Sec = M.SECTIONS.find((S) => S.Id === St.Section), R = Brush().reset;
            if (Sec?.Id === "instrument") {
              EditBrush((B0) => ({ ...B0, params: structuredClone(R.params), values: M.Normalise(M.ApplyInstrumentParams(B0.values, R.params)) }));
            } else if (Sec) {
              EditBrush((B0) => {
                const V = { ...B0.values };
                for (const G of Sec.Groups) for (const F of G.Fields) V[F.K] = structuredClone(R.values[F.K]);
                return { ...B0, values: M.Normalise(V) };
              });
            }
            RenderSettings();
            bridge.toast?.(`${Sec?.Label || "Section"} reset`);
          } else {
            const Studio = STUDIOS[St.Studio];
            const Sec = Studio.Sections.find((S) => S.Id === (St.ToolSection[St.Studio] || Studio.Sections[0].Id));
            if (Sec) {
              for (const G of SectionGroups(St.Studio, Sec)) {
                for (const F of G.Fields) {
                  if (!F.K.startsWith("@") && "Def" in F && F.T !== "history") {
                    St.Tools = SetToolValue(St.Tools, St.Studio, F.K, ToolDefault(St.Studio, F.K));
                  }
                }
              }
              SyncToWorkspace({ live: false });
              RenderAll();
              bridge.toast?.(`${Sec.Label} reset`);
            }
          }
          break;
        }
        case "reset-all":
          EditBrush((B0) => M.ResetBrush(B0));
          RenderAll();
          bridge.toast?.("Brush reset to default");
          break;
        case "reset-point":
          EditBrush((B0) => M.MakeResetPoint(B0));
          RenderAll();
          bridge.toast?.("Saved new brush reset point");
          break;
        case "duplicate": {
          const D = M.Duplicate(Brush());
          D.reset = structuredClone({ values: D.values, params: D.params, name: D.name });
          St.Brushes[D.id] = D;
          St.Custom = [D.id, ...St.Custom];
          SelectBrush(D.id);
          St.Section = "about";
          RenderAll();
          bridge.toast?.(`Duplicated "${D.name}" to My Brushes`);
          break;
        }
        case "export": {
          const B = Brush();
          Download(M.Export(B), `${B.name.replace(/\W+/g, "-").toLowerCase()}.brush.json`, "application/json");
          bridge.toast?.(`Exported "${B.name}.brush.json"`);
          break;
        }
        case "import":
          Popover.querySelector(".bs-file")?.click();
          break;
        case "undo":
          Step(true);
          break;
        case "redo":
          Step(false);
          break;
        case "tool-reset": {
          const Fresh = CreateTools()[St.Studio];
          St.Tools = { ...St.Tools, [St.Studio]: { ...Fresh, history: St.Tools[St.Studio].history ?? Fresh.history } };
          SyncToWorkspace({ live: false });
          RenderAll();
          bridge.toast?.(`${TOOL[St.Studio]?.Label || "Tool"} reset`);
          break;
        }
        case "swap-colours": {
          const V = Brush().values;
          EditBrush((B0) => M.SetValue(M.SetValue(B0, "colour.primary", V["colour.secondary"]), "colour.secondary", V["colour.primary"]));
          RenderSettings();
          break;
        }
        case "swatch": {
          const Key = St.Studio === "eyedropper" && St.Tools.eyedropper.target === "Secondary" ? "colour.secondary" : "colour.primary";
          EditBrush((B0) => M.SetValue(B0, Key, El.dataset.value));
          RenderSettings();
          break;
        }
        case "use-sample": {
          const Last = St.Tools.eyedropper.last;
          if (Last?.Hex) {
            const Key = El.dataset.value === "secondary" ? "colour.secondary" : "colour.primary";
            EditBrush((B0) => M.SetValue(B0, Key, Last.Hex));
            bridge.toast?.(`Applied ${Last.Hex.toUpperCase()} to ${El.dataset.value}`);
          }
          break;
        }
        case "nudge": {
          const D = E.shiftKey ? 10 : 1;
          const dx = D * Number(El.dataset.x || 0), dy = D * Number(El.dataset.y || 0);
          St.Tools.move = { ...St.Tools.move, dx: (St.Tools.move.dx || 0) + dx, dy: (St.Tools.move.dy || 0) + dy };
          bridge.onMoveNudge?.(dx, dy);
          RenderSettings();
          break;
        }
        case "paper":
          St.Paper = El.dataset.value;
          RenderPadBar();
          break;
        case "clear":
          Pad?.Clear();
          break;
        // Workspace-wired direct actions
        case "ws-upload-tip":
          bridge.triggerUploadTip?.();
          break;
        case "ws-upload-stamp":
          bridge.triggerUploadStamp?.();
          break;
        case "ws-place-stamp":
          bridge.placeStampAtCenter?.();
          break;
        case "ws-stamp-text":
        case "text-apply":
          bridge.stampTextAtCenter?.();
          break;
        case "ws-curve-stroke":
        case "curve-commit":
          bridge.strokeCurve?.();
          break;
        case "ws-curve-undo":
        case "curve-undo":
          bridge.undoCurvePoint?.();
          break;
        case "ws-curve-clear":
        case "curve-clear":
          bridge.clearCurve?.();
          break;
        case "ws-gradient-apply":
        case "gradient-apply":
          bridge.applyGradient?.();
          break;
        default:
          break;
      }
    });

    // Interactive SVG Curve Dragging
    const CurvePoint = (Svg, E) => {
      const R = Svg.getBoundingClientRect(), K = 320 / (R.width || 320);
      return [Clamp(((E.clientX - R.left) * K - 10) / 300, 0, 1), Clamp((122 - (E.clientY - R.top) * K) / 112, 0, 1)];
    };
    Popover.addEventListener("pointerdown", (E) => {
      const Svg = E.target.closest?.("svg.bs-curve-svg");
      if (!Svg) return;
      const Key = Svg.closest("[data-curve]").dataset.curve.slice(2);
      const Pts = Brush().values[Key];
      const [X, Y] = CurvePoint(Svg, E);
      const I = Pts.findIndex(([Px, Py]) => Math.hypot((Px - X) * 300, (Py - Y) * 112) < 13);
      if (I < 0) return;
      E.preventDefault();
      Svg.setPointerCapture?.(E.pointerId);
      CurveDrag = { Svg, Key, I };
      H.Begin({ Id: St.ActiveId, Brush: Brush() });
    });
    Popover.addEventListener("pointermove", (E) => {
      if (!CurveDrag) return;
      const { Svg, Key, I } = CurveDrag;
      const Pts = Brush().values[Key].map((P) => [...P]);
      let [X, Y] = CurvePoint(Svg, E);
      if (I === 0) X = Pts[0][0] === 0 ? 0 : Math.min(X, Pts[1][0] - 0.01);
      else if (I === Pts.length - 1) X = Pts[I][0] === 1 ? 1 : Math.max(X, Pts[I - 1][0] + 0.01);
      else X = Clamp(X, Pts[I - 1][0] + 0.01, Pts[I + 1][0] - 0.01);
      Pts[I] = [+X.toFixed(3), +Y.toFixed(3)];
      St.Brushes[St.ActiveId] = M.SetValue(Brush(), Key, Pts);
      Svg.innerHTML = CurveSvg(Brush().values[Key]);
      SyncToWorkspace({ live: true });
    });
    const EndCurve = () => {
      if (!CurveDrag) return;
      CurveDrag = null;
      H.Settle({ Id: St.ActiveId, Brush: Brush() });
      Save();
      RenderHead();
      RefreshEdited();
    };
    Popover.addEventListener("pointerup", EndCurve);
    Popover.addEventListener("pointercancel", EndCurve);
    Popover.addEventListener("dblclick", (E) => {
      const Svg = E.target.closest?.("svg.bs-curve-svg");
      if (!Svg) return;
      const Key = Svg.closest("[data-curve]").dataset.curve.slice(2);
      const Pts = Brush().values[Key].map((P) => [...P]);
      const [X, Y] = CurvePoint(Svg, E);
      const I = Pts.findIndex(([Px, Py]) => Math.hypot((Px - X) * 300, (Py - Y) * 112) < 13);
      let Next = null;
      if (I > 0 && I < Pts.length - 1) Next = Pts.filter((_, J) => J !== I);
      else if (I < 0 && Pts.length < 12) Next = [...Pts, [+X.toFixed(3), +Y.toFixed(3)]].sort((A, B) => A[0] - B[0]);
      if (Next) {
        EditBrush((B0) => M.SetValue(B0, Key, Next));
        RedrawCurves();
      }
    });

    return Popover;
  }

  // ------------------------------------------------------------------------------------------------ Public API
  function open(x = null, y = null, opts = {}) {
    SyncFromWorkspace();
    const curTool = opts.tool || bridge.getTool?.() || St.Tool;
    if (TOOL[curTool]) {
      St.Tool = curTool;
      St.Studio = curTool;
    }
    if (opts.openLibrary !== undefined) St.LibOpen = !!opts.openLibrary;
    else if (St.Studio === "paint" && !St.OpenPopup && !opts.keepLayout) St.LibOpen = true;
    if (opts.section) {
      if (St.Studio === "paint") St.Section = opts.section;
      else St.ToolSection[St.Studio] = opts.section;
    }

    const El = EnsurePopover();
    if (!El.isConnected) document.body.append(El);
    St.OpenPopup = true;
    RenderAll();
    ClampPosition(x ?? St.PopupPos?.x, y ?? St.PopupPos?.y);
    if (St.PadOpen && Pad) {
      setTimeout(() => { Pad.Resize(); Pad.Replay(); }, 25);
    }
    bridge.onOpenChange?.(true);
  }

  function close() {
    if (!St.OpenPopup) return;
    St.OpenPopup = false;
    Popover?.remove();
    bridge.onOpenChange?.(false);
  }

  function toggle(x = null, y = null, opts = {}) {
    if (St.OpenPopup && !opts.forceOpen) close();
    else open(x, y, opts);
  }

  function setTool(toolId, { notifyWorkspace = false } = {}) {
    if (!TOOL[toolId]) return;
    St.Tool = toolId;
    St.Studio = toolId;
    if (notifyWorkspace) bridge.selectWorkspaceTool?.(toolId);
    SyncToWorkspace({ live: false });
    if (St.OpenPopup) {
      RenderAll();
      ClampPosition(St.PopupPos?.x, St.PopupPos?.y);
    }
  }

  function equipExternalBrush(raw) {
    if (!raw) return;
    if (raw.id && (St.Brushes[raw.id] || paintTool(raw.id))) {
      SelectBrush(raw.id);
      return;
    }
    if (raw.tip) {
      const tipToShape = { round: "soft", chisel: "calligraphy", chalk: "chalk", splatter: "splatter", sponge: "acrylic" };
      EditBrush((Br) => {
        let next = M.SetValue(Br, "shape.source", tipToShape[raw.tip] || "soft");
        if (typeof raw.hardness === "number") next = M.SetValue(next, "shape.hardness", Math.round(raw.hardness * 100));
        return next;
      });
    }
  }

  function recordSampledColour(hex, meta = "") {
    if (!M.HEX.test(hex)) return;
    const L = hex.toLowerCase();
    const Prev = St.Tools.eyedropper.history || [];
    St.Tools.eyedropper = {
      ...St.Tools.eyedropper,
      last: { Hex: L, meta },
      history: [L, ...Prev.filter((C) => C !== L)].slice(0, 12),
    };
    RememberColour(L);
    const Key = St.Tools.eyedropper.target === "Secondary" ? "colour.secondary" : "colour.primary";
    St.Brushes[St.ActiveId] = M.SetValue(Brush(), Key, L);
    if (St.OpenPopup && (St.Studio === "eyedropper" || St.Section === "quick" || St.Section === "colour")) {
      RenderSettings();
    }
  }

  function getLauncherSummary() {
    const B = Brush();
    const DiffN = M.Changed(B).size;
    return {
      id: B.id,
      name: B.name,
      family: B.family,
      editedCount: DiffN,
      strokeUrl: StrokeThumb(B, 240, 32),
      toolId: St.Tool,
      toolLabel: TOOL[St.Tool]?.Label || "Brush",
      activeColor: B.values["colour.primary"] || "#e8c27a",
    };
  }

  function dispose() {
    close();
    Pad?.Destroy();
    Popover = null;
  }

  return {
    open,
    close,
    toggle,
    isOpen: () => St.OpenPopup,
    setTool,
    selectBrushById: SelectBrush,
    equipExternalBrush,
    recordSampledColour,
    syncFromWorkspace: SyncFromWorkspace,
    syncToWorkspace: SyncToWorkspace,
    getLauncherSummary,
    dispose,
  };
}
