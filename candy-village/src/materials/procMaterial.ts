/**
 * Procedural PBR materials.
 *
 * A ProcMaterial is a MeshPhysicalMaterial whose fragment shader is patched to
 * evaluate a *surface function* per pixel, in material space (object-local
 * metres). Each surface function returns:
 *   - albedo (multiplied into the base colour),
 *   - absolute roughness,
 *   - an emissive mask,
 *   - and a height field whose screen-space gradient perturbs the normal
 *     (bump mapping without any texture or UV unwrap).
 *
 * Because everything is analytic noise, the detail is infinite-resolution and
 * the same material can be dropped on any mesh, including instanced geometry
 * and jiggling jelly bodies (the vertex shader keeps material coordinates on
 * the rest pose).
 */
import { Color, MeshPhysicalMaterial, Vector4 } from 'three';
import { GLSL_NOISE } from './glsl';

export type ProcKind =
  | 'gingerbread'
  | 'royalIcing'
  | 'chocolate'
  | 'chocolateFlow'
  | 'fudgeBrick'
  | 'peppermint'
  | 'jelly'
  | 'swirl'
  | 'licorice'
  | 'candyGround'
  | 'bullseye'
  | 'sugarGlass';

export interface ProcOptions {
  kind: ProcKind;
  /** Primary colour (meaning depends on kind). */
  colorA?: number | string | Color;
  /** Secondary colour. */
  colorB?: number | string | Color;
  /** Tertiary colour. */
  colorC?: number | string | Color;
  /**
   * Kind-specific parameter A (see KIND_DEFAULTS). Passed through uParams.x.
   * Generic: uParams.y = rim light, uParams.z = parameter B, uParams.w = emissive strength.
   */
  params?: Partial<{ x: number; y: number; z: number; w: number }>;
  /** Bump strength multiplier for the height field. 0 disables bump. */
  bump?: number;
  /** Enables a jiggle displacement in the vertex shader (used by jelly bodies). */
  jiggle?: boolean;
  /** Clear-coat strength for glossy candy finishes. */
  clearcoat?: number;
  /** Transparency for glass-like materials. */
  opacity?: number;
}

export interface ProcUniforms {
  uColA: { value: Color };
  uColB: { value: Color };
  uColC: { value: Color };
  uParams: { value: Vector4 };
  uBump: { value: number };
  uWobble: { value: number };
}

export interface ProcMaterialData {
  kind: ProcKind;
  uniforms: ProcUniforms;
}

/** Shared animation clock for all procedural materials. */
const sharedTime = { value: 0 };

export function setProcTime(seconds: number): void {
  sharedTime.value = seconds;
}

interface KindDefaults {
  colorA: string;
  colorB: string;
  colorC: string;
  params: [number, number, number, number];
  bump: number;
  clearcoat: number;
  roughness: number;
}

const KIND_DEFAULTS: Record<ProcKind, KindDefaults> = {
  gingerbread:   { colorA: '#8f4c22', colorB: '#c98a4f', colorC: '#f3e4c8', params: [0, 0, 0, 0], bump: 1.0, clearcoat: 0, roughness: 1 },
  royalIcing:    { colorA: '#fff8f4', colorB: '#ff6fa8', colorC: '#ffffff', params: [0.28, 0, 0, 0], bump: 1.0, clearcoat: 0.3, roughness: 1 },
  chocolate:     { colorA: '#2a130a', colorB: '#6b3a1f', colorC: '#ffffff', params: [0.25, 0, 0, 0], bump: 0.6, clearcoat: 0.8, roughness: 1 },
  chocolateFlow: { colorA: '#2b1206', colorB: '#c27a2c', colorC: '#ffffff', params: [0.12, 0.6, 0, 0], bump: 1.0, clearcoat: 1.0, roughness: 1 },
  fudgeBrick:    { colorA: '#5a2c15', colorB: '#f2dcc0', colorC: '#3b1b0c', params: [1.0, 0.6, 0, 0], bump: 1.0, clearcoat: 0.2, roughness: 1 },
  peppermint:    { colorA: '#e8203a', colorB: '#fbfbfb', colorC: '#ffffff', params: [6, 0.15, 0.6, 0], bump: 0.5, clearcoat: 0.9, roughness: 1 },
  jelly:         { colorA: '#ffffff', colorB: '#ffffff', colorC: '#ffffff', params: [0.6, 0.6, 0, 0], bump: 1.0, clearcoat: 0.7, roughness: 1 },
  swirl:         { colorA: '#ff3d7f', colorB: '#fff3f7', colorC: '#ffffff', params: [3, 0.1, 0, 0], bump: 0.4, clearcoat: 0.9, roughness: 1 },
  licorice:      { colorA: '#120a0b', colorB: '#8a0d1c', colorC: '#ffffff', params: [0, 0.1, 0, 0], bump: 1.0, clearcoat: 0.4, roughness: 1 },
  candyGround:   { colorA: '#4b2615', colorB: '#8d4f2a', colorC: '#ffffff', params: [0.10, 0, 0, 0], bump: 1.0, clearcoat: 0, roughness: 1 },
  bullseye:      { colorA: '#e8203a', colorB: '#fffdf7', colorC: '#f5d7a1', params: [0, 0, 1, 0], bump: 0, clearcoat: 0.6, roughness: 1 },
  sugarGlass:    { colorA: '#ffc46b', colorB: '#ffffff', colorC: '#ffffff', params: [0, 0, 0, 1.6], bump: 0, clearcoat: 1.0, roughness: 1 },
};

/* -------------------------------------------------------------------------- */
/*                         Per-kind surface functions                         */
/* -------------------------------------------------------------------------- */

const KIND_CODE: Record<ProcKind, string> = {
  /* Baked gingerbread: mottled bake, flour dust, air pores. */
  gingerbread: /* glsl */ `
    float procHeight(vec3 p, vec3 n) {
      vec3 c; vec2 w = worley3(p * 26.0, c);
      float pores = smoothstep(0.16, 0.05, w.x);
      return 0.012 * fbm2(p * 5.0) + 0.002 * snoise(p * 48.0) - 0.004 * pores;
    }
    void procSample(vec3 p, vec3 n, out vec3 alb, out float rough, out float emis) {
      float bake = fbm2(p * 2.2);
      float grain = snoise(p * 18.0);
      vec3 c; vec2 w = worley3(p * 26.0, c);
      float pores = smoothstep(0.16, 0.05, w.x);
      vec3 base = mix(uColA, uColB, smoothstep(-0.6, 0.6, bake));
      base *= 0.9 + 0.12 * grain;
      float dust = smoothstep(0.45, 0.9, snoise(p * 34.0 + 5.0));
      base = mix(base, uColC, dust * 0.30);
      base *= 1.0 - pores * 0.45;
      alb = base;
      rough = 0.80 + 0.15 * grain;
      emis = 0.0;
    }`,

  /* Piped royal icing with pastel sprinkles. uParams.x = sprinkle density. */
  royalIcing: /* glsl */ `
    float procHeight(vec3 p, vec3 n) {
      float ridge = 0.5 + 0.5 * sin((p.x + p.y * 0.6) * 140.0 + snoise(p * 9.0) * 2.0);
      return 0.0012 * ridge + 0.0006 * snoise(p * 40.0);
    }
    void procSample(vec3 p, vec3 n, out vec3 alb, out float rough, out float emis) {
      vec3 c; vec2 w = worley3(p * 16.0, c);
      float has = step(1.0 - uParams.x, c.x);
      float spr = has * (1.0 - smoothstep(0.17, 0.21, w.x));
      vec3 col = uColA * (0.97 + 0.03 * snoise(p * 30.0));
      col = mix(col, candyPalette(c.y), spr);
      alb = col;
      rough = mix(0.22, 0.40, spr);
      emis = 0.0;
    }`,

  /* Glossy dark chocolate with tempering swirls and fat bloom. uParams.x = bloom. */
  chocolate: /* glsl */ `
    float procHeight(vec3 p, vec3 n) {
      return 0.002 * sin(p.y * 22.0 + fbm2(p * 3.0) * 4.0) + 0.0006 * snoise(p * 60.0);
    }
    void procSample(vec3 p, vec3 n, out vec3 alb, out float rough, out float emis) {
      float sw = fbm2(p * vec3(2.0, 5.0, 2.0));
      float band = 0.5 + 0.5 * sin(p.y * 16.0 + sw * 5.0);
      vec3 col = uColA * (0.82 + 0.25 * band);
      float bl = smoothstep(0.45, 0.8, fbm2(p * 7.0 + 3.0)) * uParams.x;
      col = mix(col, uColC * 0.45 + uColB * 0.2, bl);
      alb = col;
      rough = 0.16 + 0.12 * bl + 0.04 * snoise(p * 30.0);
      emis = 0.0;
    }`,

  /* Animated chocolate river. uParams.x = flow speed (m/s). */
  chocolateFlow: /* glsl */ `
    float procHeight(vec3 p, vec3 n) {
      vec3 q = vec3(p.x - uTime * uParams.x, p.y, p.z);
      return 0.02 * fbm2(vec3(q.x * 0.9, q.z * 2.2, uTime * 0.15))
           + 0.003 * snoise(vec3(q.x * 6.0, q.z * 12.0, uTime * 0.4));
    }
    void procSample(vec3 p, vec3 n, out vec3 alb, out float rough, out float emis) {
      vec3 q = vec3(p.x - uTime * uParams.x, p.y, p.z);
      float f = fbm2(vec3(q.x * 0.5, q.z * 1.5, uTime * 0.1));
      float streak = smoothstep(0.1, 0.6, snoise(vec3(q.x * 1.2, q.z * 6.0, 0.0)));
      alb = mix(uColA, uColB, clamp(streak * 0.45 + 0.2 * f, 0.0, 1.0));
      rough = 0.035 + 0.08 * (1.0 - streak);
      emis = 0.0;
    }`,

  /* Chocolate-brick fudge walls with caramel mortar. uParams.x = brick scale. */
  fudgeBrick: /* glsl */ `
    vec4 brickInfo(vec3 p, vec3 n) {
      vec2 uv = abs(n.z) > abs(n.x) ? p.xy : p.zy;
      uv *= 1.0 / max(uParams.x, 0.3);
      vec2 bs = vec2(0.34, 0.13);
      float row = floor(uv.y / bs.y);
      uv.x += mod(row, 2.0) * 0.5 * bs.x;
      vec2 cellId = floor(uv / bs);
      vec2 f = fract(uv / bs);
      vec2 d = min(f, 1.0 - f) * bs;
      float edge = min(d.x, d.y);
      float mortar = 1.0 - smoothstep(0.010, 0.020, edge);
      float h = phash3(vec3(cellId, 3.0)).x;
      return vec4(mortar, h, edge, 0.0);
    }
    float procHeight(vec3 p, vec3 n) {
      vec4 b = brickInfo(p, n);
      return -0.010 * b.x + 0.002 * snoise(p * 30.0) * (1.0 - b.x);
    }
    void procSample(vec3 p, vec3 n, out vec3 alb, out float rough, out float emis) {
      vec4 b = brickInfo(p, n);
      vec3 brick = uColA * (0.85 + 0.3 * b.y);
      vec3 c; vec2 w = worley3(p * 55.0, c);
      float chip = smoothstep(0.12, 0.07, w.x) * step(0.55, c.x);
      brick = mix(brick, uColC, chip * 0.8);
      brick *= 0.92 + 0.08 * snoise(p * 22.0);
      alb = mix(brick, uColB, b.x);
      rough = mix(0.30, 0.9, b.x);
      emis = 0.0;
    }`,

  /* Peppermint / candy-cane helical stripes around the local Y axis.
     uParams.x = stripe count (integer), uParams.z = helix pitch. */
  peppermint: /* glsl */ `
    float procHeight(vec3 p, vec3 n) {
      return 0.0005 * snoise(p * 80.0);
    }
    void procSample(vec3 p, vec3 n, out vec3 alb, out float rough, out float emis) {
      float ang = atan(p.z, p.x) / 6.28318;
      float s = fract(ang * uParams.x + p.y * uParams.z);
      float m = smoothstep(0.47, 0.53, s);
      alb = mix(uColA, uColB, m) * (0.97 + 0.03 * snoise(p * 60.0));
      rough = 0.22 + 0.03 * snoise(p * 40.0);
      emis = 0.0;
    }`,

  /* Colloidal jelly-bean gel with sanded sugar crystals. uParams.x = sugar amount. */
  jelly: /* glsl */ `
    float procHeight(vec3 p, vec3 n) {
      vec3 c; vec2 w = worley3(p * 46.0, c);
      float sugar = smoothstep(0.24, 0.12, w.x) * uParams.x * step(0.45, c.x);
      return 0.004 * sugar + 0.001 * snoise(p * 5.0);
    }
    void procSample(vec3 p, vec3 n, out vec3 alb, out float rough, out float emis) {
      vec3 c; vec2 w = worley3(p * 46.0, c);
      float sugar = smoothstep(0.24, 0.12, w.x) * uParams.x * step(0.45, c.x);
      float mottle = 0.5 + 0.5 * snoise(p * 3.0);
      vec3 col = uColA * (0.90 + 0.16 * mottle);
      col = mix(col, vec3(1.0, 0.98, 0.95), sugar * 0.8);
      alb = col;
      rough = mix(0.16, 0.55, sugar);
      emis = 0.0;
    }`,

  /* Spiral lollipop / gumdrop swirl. uParams.x = number of arms, uParams.w = glow. */
  swirl: /* glsl */ `
    float procHeight(vec3 p, vec3 n) {
      return 0.0008 * snoise(p * 40.0);
    }
    void procSample(vec3 p, vec3 n, out vec3 alb, out float rough, out float emis) {
      float phi = atan(p.z, p.x) / 6.28318;
      float s = fract(phi * uParams.x + p.y * 2.2 + 0.08 * snoise(p * 4.0));
      float m = smoothstep(0.45, 0.55, s);
      alb = mix(uColA, uColB, m);
      rough = 0.18;
      emis = 0.0;
    }`,

  /* Black licorice with braided strands. */
  licorice: /* glsl */ `
    float procHeight(vec3 p, vec3 n) {
      return 0.0015 * sin(p.y * 60.0 + atan(p.z, p.x) * 6.0);
    }
    void procSample(vec3 p, vec3 n, out vec3 alb, out float rough, out float emis) {
      float braid = 0.5 + 0.5 * sin(p.y * 18.0 + atan(p.z, p.x) * 3.0);
      alb = mix(uColA, uColB, smoothstep(0.75, 0.95, braid) * 0.6) * (0.85 + 0.15 * braid);
      rough = 0.32 + 0.12 * braid;
      emis = 0.0;
    }`,

  /* Chocolate-brownie ground with optional confetti sprinkles. uParams.x = confetti density. */
  candyGround: /* glsl */ `
    float procHeight(vec3 p, vec3 n) {
      vec3 c; vec2 w = worley3(p * 9.0, c);
      float conf = smoothstep(0.22, 0.14, w.x) * step(1.0 - uParams.x, c.x);
      return 0.05 * fbm2(p * 1.5) + 0.01 * snoise(p * 14.0) + 0.006 * conf;
    }
    void procSample(vec3 p, vec3 n, out vec3 alb, out float rough, out float emis) {
      float f = fbm2(p * 1.2);
      float g = snoise(p * 12.0);
      vec3 soil = mix(uColA, uColB, smoothstep(-0.5, 0.5, f));
      soil *= 0.9 + 0.1 * g;
      vec3 c; vec2 w = worley3(p * 9.0, c);
      float conf = smoothstep(0.22, 0.14, w.x) * step(1.0 - uParams.x, c.x);
      alb = mix(soil, candyPalette(c.y), conf);
      rough = mix(0.85, 0.35, conf);
      emis = 0.0;
    }`,

  /* Concentric scoring rings. uParams.z = outer radius in metres. */
  bullseye: /* glsl */ `
    float procHeight(vec3 p, vec3 n) { return 0.0; }
    void procSample(vec3 p, vec3 n, out vec3 alb, out float rough, out float emis) {
      float r = length(p.xz) / max(uParams.z, 0.001);
      float ring = floor(r * 3.0);
      vec3 col = mod(ring, 2.0) < 1.0 ? uColA : uColB;
      if (r > 1.0) col = uColC;
      alb = col * (0.97 + 0.03 * snoise(p * 40.0));
      rough = 0.30;
      emis = 0.0;
    }`,

  /* Warm sugar-glass windows that glow from within. */
  sugarGlass: /* glsl */ `
    float procHeight(vec3 p, vec3 n) { return 0.0; }
    void procSample(vec3 p, vec3 n, out vec3 alb, out float rough, out float emis) {
      float streak = snoise(p * vec3(6.0, 1.0, 6.0)) * 0.05;
      alb = uColA * (0.95 + streak);
      rough = 0.08;
      emis = 1.0;
    }`,
};

/* -------------------------------------------------------------------------- */
/*                               Shader patching                              */
/* -------------------------------------------------------------------------- */

const VERTEX_HEADER = /* glsl */ `
varying vec3 vMatPos;
varying vec3 vMatNormal;
varying vec3 vNM0;
varying vec3 vNM1;
varying vec3 vNM2;
uniform float uTime;
uniform float uWobble;
`;

const VERTEX_BODY = /* glsl */ `
#include <begin_vertex>
vMatPos = position;
vMatNormal = normalize(objectNormal);
mat3 nmat = normalMatrix;
#ifdef USE_INSTANCING
nmat = normalMatrix * mat3(instanceMatrix);
#endif
vNM0 = nmat[0];
vNM1 = nmat[1];
vNM2 = nmat[2];
// Jelly jiggle: travelling sinusoidal wobble along normals (zero when uWobble = 0).
float wob = sin(uTime * 7.0 + position.y * 5.0 + position.x * 4.0 + position.z * 3.0);
transformed += objectNormal * uWobble * wob * (0.6 + 0.4 * position.y);
`;

const FRAGMENT_HEADER = (kind: ProcKind): string => /* glsl */ `
varying vec3 vMatPos;
varying vec3 vMatNormal;
varying vec3 vNM0;
varying vec3 vNM1;
varying vec3 vNM2;
uniform float uTime;
uniform vec3 uColA;
uniform vec3 uColB;
uniform vec3 uColC;
uniform vec4 uParams;
uniform float uBump;
${GLSL_NOISE}
// ---- surface kind: ${kind} ----
${KIND_CODE[kind]}
`;

const FRAGMENT_BODY = /* glsl */ `
#include <color_fragment>
vec3 pAlb = vec3(1.0);
float pRough = 1.0;
float pEmis = 0.0;
procSample(vMatPos, normalize(vMatNormal), pAlb, pRough, pEmis);
diffuseColor.rgb *= pAlb;
`;

const FRAGMENT_ROUGHNESS = /* glsl */ `
float roughnessFactor = clamp(roughness * pRough, 0.04, 1.0);
`;

const FRAGMENT_NORMAL = /* glsl */ `
#include <normal_fragment_maps>
if (uBump > 0.0) {
  vec3 nObj = normalize(vMatNormal);
  float e = 0.0025;
  float h0 = procHeight(vMatPos, nObj);
  vec3 g = vec3(
    procHeight(vMatPos + vec3(e, 0.0, 0.0), nObj) - h0,
    procHeight(vMatPos + vec3(0.0, e, 0.0), nObj) - h0,
    procHeight(vMatPos + vec3(0.0, 0.0, e), nObj) - h0
  ) / e;
  mat3 nm = mat3(vNM0, vNM1, vNM2);
  vec3 gv = nm * g * uBump;
  normal = normalize(normal - (gv - dot(gv, normal) * normal));
}
`;

const FRAGMENT_EMISSIVE = /* glsl */ `
#include <emissivemap_fragment>
totalEmissiveRadiance += pAlb * pEmis * uParams.w;
{
  float rimF = 1.0 - clamp(dot(normalize(normal), normalize(vViewPosition)), 0.0, 1.0);
  totalEmissiveRadiance += diffuseColor.rgb * pow(rimF, 3.0) * uParams.y * 0.45;
}
`;

/* -------------------------------------------------------------------------- */
/*                                  Factory                                   */
/* -------------------------------------------------------------------------- */

export class ProcMaterial extends MeshPhysicalMaterial {
  readonly proc: ProcMaterialData;

  constructor(opts: ProcOptions) {
    const d = KIND_DEFAULTS[opts.kind];
    const p = opts.params ?? {};
    const params = new Vector4(
      p.x ?? d.params[0],
      p.y ?? d.params[1],
      p.z ?? d.params[2],
      p.w ?? d.params[3],
    );
    const uniforms: ProcUniforms = {
      uColA: { value: new Color(opts.colorA ?? d.colorA) },
      uColB: { value: new Color(opts.colorB ?? d.colorB) },
      uColC: { value: new Color(opts.colorC ?? d.colorC) },
      uParams: { value: params },
      uBump: { value: opts.bump ?? d.bump },
      uWobble: { value: 0 },
    };

    super({
      color: 0xffffff,
      roughness: d.roughness,
      metalness: 0,
      clearcoat: opts.clearcoat ?? d.clearcoat,
      clearcoatRoughness: 0.18,
      transparent: (opts.opacity ?? 1) < 1,
      opacity: opts.opacity ?? 1,
    });

    this.proc = { kind: opts.kind, uniforms };
    const kind = opts.kind;
    const jiggle = opts.jiggle ?? false;
    this.name = `proc:${kind}`;

    this.onBeforeCompile = (shader) => {
      shader.uniforms.uTime = sharedTime;
      shader.uniforms.uColA = uniforms.uColA;
      shader.uniforms.uColB = uniforms.uColB;
      shader.uniforms.uColC = uniforms.uColC;
      shader.uniforms.uParams = uniforms.uParams;
      shader.uniforms.uBump = uniforms.uBump;
      shader.uniforms.uWobble = uniforms.uWobble;

      shader.vertexShader = shader.vertexShader
        .replace('#include <common>', `#include <common>\n${VERTEX_HEADER}`)
        .replace('#include <begin_vertex>', VERTEX_BODY);

      shader.fragmentShader = shader.fragmentShader
        .replace('#include <common>', `#include <common>\n${FRAGMENT_HEADER(kind)}`)
        .replace('#include <color_fragment>', FRAGMENT_BODY)
        .replace('#include <roughnessmap_fragment>', FRAGMENT_ROUGHNESS)
        .replace('#include <normal_fragment_maps>', FRAGMENT_NORMAL)
        .replace('#include <emissivemap_fragment>', FRAGMENT_EMISSIVE);
    };

    // Distinct program per kind (and per jiggle variant) so shaders stay lean.
    this.customProgramCacheKey = () => `proc:${kind}:${jiggle ? 'j' : 's'}`;
  }

  /** Set the jelly wobble amplitude in metres (0 = rigid). */
  setWobble(amount: number): void {
    this.proc.uniforms.uWobble.value = amount;
  }
}

export function createProcMaterial(opts: ProcOptions): ProcMaterial {
  return new ProcMaterial(opts);
}

/** Convenience: a list of every kind, useful for validation and tests. */
export const PROC_KINDS = Object.keys(KIND_CODE) as ProcKind[];
