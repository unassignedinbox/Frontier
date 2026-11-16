// Real-Time GI-Lit 3D Mesh Overlay Engine for Frontier World Mirror
// Renders the exact Component Inspector BP_ControlVehicle (ControlVehicle{Body,Tyre,Rim}.facet + RearWing)
// and any Geometry-Compiled user meshes inside the Global Illumination viewport with:
//   - Sub-pixel occlusion against Pass-1 architectural primitives (WebGPU, WebGL2, and CPU-GTX identical)
//   - Direct Sun GGX + Clearcoat + Soft Shadows from world occluders
//   - Multi-Light Area Light illumination from all 12 ActiveScene.AreaLights
//   - Physically grounded GI hemisphere + floor/wall bounce + wheel-arch AO + ACES filmic tonemapping
// Always-on-top 3D Slate Transform Gizmo pass so gizmo arrows never clip behind high-poly meshes

import {
  GIZMO_VERTEX_SHADER,
  GIZMO_FRAGMENT_SHADER,
} from "./TransformGizmo3D.js";

export const VEHICLE_SHEET = {
  axleMount: [
    [1.7274, 1.0475, 0.0914],
    [1.7274, -1.0475, 0.0914],
    [-1.6686, 1.0475, 0.0914],
    [-1.6686, -1.0475, 0.0914],
  ],
  groundOffset: 0.50855 - 0.0914,
};

export const VEHICLE_PALETTE = {
  6: { name: "BodyCoat", albedo: [0.68, 0.8, 0.0], roughness: 0.20, metalness: 0.04, coat: 0.92 },
  7: { name: "VehicleGlass", albedo: [0.022, 0.026, 0.032], roughness: 0.045, metalness: 0.02, coat: 1.0 },
  8: { name: "VehiclePlastic", albedo: [0.015, 0.017, 0.021], roughness: 0.48, metalness: 0.06, coat: 0.12 },
  9: { name: "Tyre", albedo: [0.032, 0.033, 0.036], roughness: 0.78, metalness: 0.0, coat: 0.0 },
  10: { name: "Hub", albedo: [0.06, 0.22, 0.62], roughness: 0.22, metalness: 0.95, coat: 0.25, emission: [0.0, 0.033, 0.12] },
  11: { name: "Brake", albedo: [0.30, 0.075, 0.025], roughness: 0.29, metalness: 0.82, coat: 0.0 },
  12: { name: "VehicleTrim", albedo: [0.78, 0.80, 0.82], roughness: 0.44, metalness: 0.12, coat: 0.35 },
  13: { name: "Headlight", albedo: [0.92, 0.94, 0.98], roughness: 0.08, metalness: 0.0, coat: 0.9, emission: [2.4, 2.35, 2.15] },
  14: { name: "Taillight", albedo: [0.85, 0.08, 0.06], roughness: 0.10, metalness: 0.0, coat: 0.85, emission: [2.6, 0.08, 0.05] },
  15: { name: "VehicleDefault", albedo: [0.72, 0.74, 0.76], roughness: 0.46, metalness: 0.08, coat: 0.2 },
};

// Shared in-memory registry for Geometry-Compiled assets imported in this session
const CompiledMeshStore = new Map();
const CompiledMeshListeners = new Set();

export function RegisterCompiledMeshAsset(Asset) {
  if (!Asset || !Asset.id) return null;
  CompiledMeshStore.set(Asset.id, Asset);
  for (const Fn of CompiledMeshListeners) {
    try { Fn(Asset); } catch {}
  }
  return Asset;
}

export function GetCompiledMeshAsset(AssetId) {
  return AssetId ? CompiledMeshStore.get(AssetId) || null : null;
}

export function ListCompiledMeshAssets() {
  return [...CompiledMeshStore.values()];
}

export function SubscribeCompiledMeshAssets(Fn) {
  CompiledMeshListeners.add(Fn);
  return () => CompiledMeshListeners.delete(Fn);
}

function DecodeBase64ToArrayBuffer(B64) {
  const Bin = atob(B64);
  const Out = new Uint8Array(Bin.length);
  for (let I = 0; I < Bin.length; ++I) Out[I] = Bin.charCodeAt(I);
  return Out.buffer;
}

function ReadFacetBinary(Bytes) {
  const View = new DataView(Bytes);
  const Magic = String.fromCharCode(
    View.getUint8(0),
    View.getUint8(1),
    View.getUint8(2),
    View.getUint8(3),
  );
  if (Magic !== "FCT1") throw new Error(`Invalid FCT1 stream (${Magic})`);
  const VertexCount = View.getUint32(4, true);
  const FacetCount = View.getUint32(8, true);
  let At = 48;
  const Positions = new Float32Array(Bytes.slice(At, At + VertexCount * 12));
  At += VertexCount * 12;
  const Indices = new Uint32Array(Bytes.slice(At, At + FacetCount * 12));
  At += FacetCount * 12;
  const Families = new Uint32Array(Bytes.slice(At, At + FacetCount * 4));
  return { positions: Positions, indices: Indices, families: Families };
}

function BuildRearWingRawZUp() {
  const Pos = [];
  const Idx = [];
  const Fam = [];
  const WING = { aoa: 8, bodyHalfWidth: 1.2498, chordHalf: 0.17, planeHalfThick: 0.016, plateHalfThick: 0.008, planeY: 0.25 };
  const AddBoxYUp = (C, H, Family, Tilt = 0) => {
    const Base = Pos.length / 3;
    const Cs = Math.cos(Tilt), Sn = Math.sin(Tilt);
    for (let I = 0; I < 8; ++I) {
      const X = (I & 1 ? 1 : -1) * H[0];
      const Y = (I & 2 ? 1 : -1) * H[1];
      const Z = (I & 4 ? 1 : -1) * H[2];
      const Wx = C[0] + X * Cs - Y * Sn;
      const Wy = C[1] + X * Sn + Y * Cs;
      const Wz = C[2] + Z;
      // Convert Y-up (Wx, Wy, Wz) to Frontier Z-up (Wx, -Wz, Wy)
      Pos.push(Wx, -Wz, Wy);
    }
    const Quads = [[0, 2, 3, 1], [4, 5, 7, 6], [0, 1, 5, 4], [2, 6, 7, 3], [0, 4, 6, 2], [1, 3, 7, 5]];
    for (const [A, B, C2, D] of Quads) {
      Idx.push(Base + A, Base + B, Base + C2, Base + A, Base + C2, Base + D);
      Fam.push(Family, Family);
    }
  };
  const { aoa, bodyHalfWidth, chordHalf, planeHalfThick, plateHalfThick, planeY } = WING;
  const Tilt = (-aoa * Math.PI) / 180;
  const PlateZ = bodyHalfWidth - plateHalfThick;
  const Span = PlateZ - plateHalfThick;
  AddBoxYUp([0, planeY, 0], [chordHalf, planeHalfThick, Span], 8, Tilt);
  const TeX = -chordHalf * Math.cos(Tilt) - planeHalfThick * Math.sin(Tilt);
  const TeY = planeY - chordHalf * Math.sin(Tilt) + planeHalfThick * Math.cos(Tilt);
  AddBoxYUp(
    [TeX + 0.008 * Math.cos(Tilt) - 0.022 * Math.sin(Tilt), TeY + 0.008 * Math.sin(Tilt) + 0.022 * Math.cos(Tilt), 0],
    [0.008, 0.022, Span],
    6,
    Tilt,
  );
  for (const Z of [-PlateZ, PlateZ]) AddBoxYUp([-0.01, 0.23, Z], [0.2, 0.1, plateHalfThick], 6, 0);
  for (const Z of [-0.56, 0.56]) AddBoxYUp([0.02, 0.11, Z], [0.05, 0.11, 0.01], 10, 0);
  return { positions: new Float32Array(Pos), indices: new Uint32Array(Idx), families: new Uint32Array(Fam) };
}

// Build interleaved non-indexed vertex stream:
// [px, py, pz,  nx, ny, nz,  ar, ag, ab,  rough, metal, coat, family,  er, eg, eb] = 16 floats/vertex
const STRIDE_FLOATS = 16;

export function HexToLinear(Hex, Fallback = [0.72, 0.74, 0.78]) {
  if (typeof Hex !== "string" || !/^#[0-9a-f]{6}$/i.test(Hex.trim())) return Fallback;
  const S = Hex.trim().slice(1);
  return [
    Math.pow(parseInt(S.slice(0, 2), 16) / 255, 2.2),
    Math.pow(parseInt(S.slice(2, 4), 16) / 255, 2.2),
    Math.pow(parseInt(S.slice(4, 6), 16) / 255, 2.2),
  ];
}

function PackMeshTriangles(Raw, { TransformFn = null, Palette = VEHICLE_PALETTE, CreaseDeg = 46, SmoothAll = false } = {}) {
  const { positions, indices, families } = Raw;
  const TriCount = Math.floor(indices.length / 3);
  const VCount = Math.floor(positions.length / 3);
  const WorldPos = new Float32Array(VCount * 3);
  for (let I = 0; I < VCount; ++I) {
    let X = positions[I * 3];
    let Y = positions[I * 3 + 1];
    let Z = positions[I * 3 + 2];
    if (TransformFn) {
      const T = TransformFn(X, Y, Z);
      X = T[0]; Y = T[1]; Z = T[2];
    }
    WorldPos[I * 3] = X;
    WorldPos[I * 3 + 1] = Y;
    WorldPos[I * 3 + 2] = Z;
  }

  // Compute face normals & adjacency for crease-aware shading
  const FaceNorm = new Float32Array(TriCount * 3);
  const VertFaces = new Array(VCount);
  for (let T = 0; T < TriCount; ++T) {
    const I0 = indices[T * 3];
    const I1 = indices[T * 3 + 1];
    const I2 = indices[T * 3 + 2];
    const Ax = WorldPos[I0 * 3], Ay = WorldPos[I0 * 3 + 1], Az = WorldPos[I0 * 3 + 2];
    const Bx = WorldPos[I1 * 3] - Ax, By = WorldPos[I1 * 3 + 1] - Ay, Bz = WorldPos[I1 * 3 + 2] - Az;
    const Cx = WorldPos[I2 * 3] - Ax, Cy = WorldPos[I2 * 3 + 1] - Ay, Cz = WorldPos[I2 * 3 + 2] - Az;
    let Nx = By * Cz - Bz * Cy;
    let Ny = Bz * Cx - Bx * Cz;
    let Nz = Bx * Cy - By * Cx;
    const Len = Math.hypot(Nx, Ny, Nz) || 1.0;
    FaceNorm[T * 3] = Nx / Len;
    FaceNorm[T * 3 + 1] = Ny / Len;
    FaceNorm[T * 3 + 2] = Nz / Len;
    for (const Vi of [I0, I1, I2]) {
      if (!VertFaces[Vi]) VertFaces[Vi] = [];
      VertFaces[Vi].push(T);
    }
  }

  const CosCrease = SmoothAll ? -2.0 : Math.cos((CreaseDeg * Math.PI) / 180);
  const Out = new Float32Array(TriCount * 3 * STRIDE_FLOATS);
  let Off = 0;

  for (let T = 0; T < TriCount; ++T) {
    const Fam = families ? families[T] : 15;
    const Spec = (Palette && Palette[Fam]) || VEHICLE_PALETTE[Fam] || VEHICLE_PALETTE[15];
    const Albedo = Array.isArray(Spec.albedo) ? Spec.albedo : HexToLinear(Spec.color, [0.7, 0.72, 0.75]);
    const Rough = Number.isFinite(Spec.roughness) ? Spec.roughness : 0.45;
    const Metal = Number.isFinite(Spec.metalness) ? Spec.metalness : 0.05;
    const Coat = Number.isFinite(Spec.coat) ? Spec.coat : (Fam === 6 ? 0.9 : 0.15);
    const Emit = Array.isArray(Spec.emission) ? Spec.emission : [0, 0, 0];
    const Fnx = FaceNorm[T * 3], Fny = FaceNorm[T * 3 + 1], Fnz = FaceNorm[T * 3 + 2];

    for (let K = 0; K < 3; ++K) {
      const Vi = indices[T * 3 + K];
      let Nx = Fnx, Ny = Fny, Nz = Fnz;
      const Adj = VertFaces[Vi];
      if (Adj && Adj.length > 1) {
        let Sx = 0, Sy = 0, Sz = 0;
        for (let J = 0; J < Adj.length; ++J) {
          const Ot = Adj[J];
          const Ox = FaceNorm[Ot * 3], Oy = FaceNorm[Ot * 3 + 1], Oz = FaceNorm[Ot * 3 + 2];
          if (SmoothAll || Fnx * Ox + Fny * Oy + Fnz * Oz >= CosCrease) {
            Sx += Ox; Sy += Oy; Sz += Oz;
          }
        }
        const Sl = Math.hypot(Sx, Sy, Sz);
        if (Sl > 1e-5) { Nx = Sx / Sl; Ny = Sy / Sl; Nz = Sz / Sl; }
      }
      Out[Off++] = WorldPos[Vi * 3];
      Out[Off++] = WorldPos[Vi * 3 + 1];
      Out[Off++] = WorldPos[Vi * 3 + 2];
      Out[Off++] = Nx;
      Out[Off++] = Ny;
      Out[Off++] = Nz;
      Out[Off++] = Albedo[0];
      Out[Off++] = Albedo[1];
      Out[Off++] = Albedo[2];
      Out[Off++] = Rough;
      Out[Off++] = Metal;
      Out[Off++] = Coat;
      Out[Off++] = Fam;
      Out[Off++] = Emit[0];
      Out[Off++] = Emit[1];
      Out[Off++] = Emit[2];
    }
  }
  return Out;
}

function ConcatFloat32(Arrays) {
  let Total = 0;
  for (const A of Arrays) Total += A.length;
  const Out = new Float32Array(Total);
  let Off = 0;
  for (const A of Arrays) { Out.set(A, Off); Off += A.length; }
  return Out;
}

let CachedVehicleStreams = null;

export function EnsureVehicleStreamsSync() {
  if (CachedVehicleStreams) return CachedVehicleStreams;
  const NativeFacets = typeof window !== "undefined" ? window.NativeAssets?.ControlVehicleFacets : null;
  if (!NativeFacets || !NativeFacets.body || !NativeFacets.tyre || !NativeFacets.rim) return null;

  const BodyRaw = ReadFacetBinary(DecodeBase64ToArrayBuffer(NativeFacets.body));
  const TyreRaw = ReadFacetBinary(DecodeBase64ToArrayBuffer(NativeFacets.tyre));
  const RimRaw = ReadFacetBinary(DecodeBase64ToArrayBuffer(NativeFacets.rim));
  const WingRaw = BuildRearWingRawZUp();

  const Gz = VEHICLE_SHEET.groundOffset;
  const BodyStream = PackMeshTriangles(BodyRaw, {
    TransformFn: (X, Y, Z) => [X, Y, Z + Gz],
    CreaseDeg: 44,
  });

  const Parts = [BodyStream];
  for (const Mount of VEHICLE_SHEET.axleMount) {
    const Flip = Mount[1] < 0 ? -1 : 1;
    const Wx = Mount[0], Wy = Mount[1], Wz = Gz + Mount[2];
    const WheelXf = (X, Y, Z) => [Wx + X * Flip, Wy + Y * Flip, Wz + Z];
    Parts.push(PackMeshTriangles(TyreRaw, { TransformFn: WheelXf, SmoothAll: true }));
    Parts.push(PackMeshTriangles(RimRaw, { TransformFn: WheelXf, CreaseDeg: 46 }));
  }

  const WingStream = PackMeshTriangles(WingRaw, {
    TransformFn: (X, Y, Z) => [X - 2.9822, Y, Z + Gz + 0.8367],
    CreaseDeg: 35,
  });

  CachedVehicleStreams = {
    core: ConcatFloat32(Parts),
    wing: WingStream,
  };
  return CachedVehicleStreams;
}

export function BuildCompiledAssetStream(Asset) {
  const Geom = Asset?.out?.geometry || Asset?.out?.source;
  if (!Geom || !Geom.positions || !Geom.indices) return null;
  const Pos = Geom.positions;
  // Convert Geometry Compile Y-up (x, y, z) -> Frontier Z-up (x, -z, y) and rest bottom cleanly on z = 0
  let MinZ = Infinity;
  let MinX = Infinity, MaxX = -Infinity, MinY = Infinity, MaxY = -Infinity;
  for (let I = 0; I < Pos.length; I += 3) {
    const X = Pos[I], Y = -Pos[I + 2], Z = Pos[I + 1];
    if (X < MinX) MinX = X;
    if (X > MaxX) MaxX = X;
    if (Y < MinY) MinY = Y;
    if (Y > MaxY) MaxY = Y;
    if (Z < MinZ) MinZ = Z;
  }
  const Cx = (MinX + MaxX) * 0.5;
  const Cy = (MinY + MaxY) * 0.5;
  const ZLift = Number.isFinite(MinZ) ? -MinZ : 0;

  // Build material palette map for compiled asset families
  const Palette = {};
  const Fams = Geom.families ? [...new Set(Geom.families)] : [0];
  Fams.forEach((Fam, Idx) => {
    const Spec = Asset.palette && Asset.palette[Fam];
    if (Spec) {
      Palette[Fam] = {
        albedo: Array.isArray(Spec.albedo) ? Spec.albedo : HexToLinear(Spec.color, [0.7, 0.72, 0.76]),
        roughness: Spec.roughness ?? 0.42,
        metalness: Spec.metalness ?? 0.12,
        coat: Spec.coat ?? 0.15,
        emission: Spec.emission || [0, 0, 0],
      };
    } else {
      const Hue = ((Idx * 0.161 + 0.08) % 1) * Math.PI * 2;
      const Base = Fams.length > 1
        ? [0.42 + 0.18 * Math.cos(Hue), 0.44 + 0.18 * Math.cos(Hue - 2.09), 0.48 + 0.18 * Math.cos(Hue + 2.09)]
        : [0.56, 0.58, 0.62];
      Palette[Fam] = { albedo: Base, roughness: 0.44, metalness: 0.10, coat: 0.18, emission: [0, 0, 0] };
    }
  });

  return PackMeshTriangles(
    { positions: Pos, indices: Geom.indices, families: Geom.families || null },
    {
      TransformFn: (X, Y, Z) => [X - Cx, -Z - Cy, Y + ZLift],
      Palette,
      CreaseDeg: 48,
    },
  );
}

// =========================================================================
// WebGL2 Shaders — GI-Lit OpenPBR Clearcoat + Exact World Occlusion + Gizmo
// =========================================================================

const MESH_VS = `#version 300 es
precision highp float;
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 aAlbedo;
layout(location = 3) in vec4 aPbr;     // x=roughness, y=metalness, z=coat, w=family
layout(location = 4) in vec3 aEmit;

uniform mat4 uViewProj;
uniform mat4 uModel;
uniform mat3 uNormalMat;

out vec3 vWorldPos;
out vec3 vWorldNormal;
out vec3 vAlbedo;
out vec4 vPbr;
out vec3 vEmit;

void main() {
  vec4 wp = uModel * vec4(aPos, 1.0);
  vWorldPos = wp.xyz;
  vWorldNormal = normalize(uNormalMat * aNormal);
  vAlbedo = aAlbedo;
  vPbr = aPbr;
  vEmit = aEmit;
  gl_Position = uViewProj * wp;
}
`;

const MAX_OCCLUDERS = 96;
const MAX_AREA_LIGHTS = 12;

const MESH_FS = `#version 300 es
precision highp float;

in vec3 vWorldPos;
in vec3 vWorldNormal;
in vec3 vAlbedo;
in vec4 vPbr;
in vec3 vEmit;

uniform vec3  uCamOrigin;
uniform vec3  uCamFwd;
uniform vec3  uCamRight;
uniform vec3  uCamUp;
uniform float uOrthoHalfH;
uniform float uExposure;
uniform int   uViewMode;

uniform vec3  uSunDir;
uniform vec3  uSunIrradiance;
uniform vec3  uSkyRadiance;

uniform int   uLightCount;
uniform vec4  uLightCenterW[${MAX_AREA_LIGHTS}];   // xyz=center, w=halfW
uniform vec4  uLightNormalH[${MAX_AREA_LIGHTS}];   // xyz=normal, w=halfH
uniform vec4  uLightRadOmni[${MAX_AREA_LIGHTS}];   // xyz=radiance, w=omniFlag

uniform int   uOccCount;
uniform sampler2D uOccTex; // width=5, height=96: col0=center+shape, col1=halfExt+flags, col2=c0, col3=c1, col4=c2

uniform vec3  uPaintOverride;
uniform float uPaintOverrideOn;
uniform float uHeadlightsOn;
uniform float uIsSelected;
uniform float uGroundZ;

out vec4 fragColor;

const float PI = 3.14159265359;

vec3 invRot(vec3 v, vec3 c0, vec3 c1, vec3 c2) {
  return vec3(dot(v, c0), dot(v, c1), dot(v, c2));
}

float intersectPrimOcc(vec3 lo, vec3 ld, vec3 h, int shape, float tMax) {
  if (shape == 1) {
    vec3 invH = 1.0 / max(h, vec3(1e-4));
    vec3 o = lo * invH;
    vec3 d = ld * invH;
    float a = dot(d, d);
    float b = dot(o, d);
    float c = dot(o, o) - 1.0;
    float disc = b * b - a * c;
    if (disc < 0.0) return tMax;
    float t = (-b - sqrt(disc)) / a;
    return (t > 0.002 && t < tMax) ? t : tMax;
  }
  if (shape == 2) {
    vec2 invR = 1.0 / max(h.xy, vec2(1e-4));
    vec2 o = lo.xy * invR;
    vec2 d = ld.xy * invR;
    float a = dot(d, d);
    float b = dot(o, d);
    float c = dot(o, o) - 1.0;
    float best = tMax;
    float disc = b * b - a * c;
    if (disc >= 0.0 && a > 1e-7) {
      float sq = sqrt(disc);
      float t0 = (-b - sq) / a;
      if (t0 > 0.002 && t0 < best && abs(lo.z + ld.z * t0) <= h.z) best = t0;
      float t1 = (-b + sq) / a;
      if (t1 > 0.002 && t1 < best && abs(lo.z + ld.z * t1) <= h.z) best = t1;
    }
    if (abs(ld.z) > 1e-6) {
      for (int s = -1; s <= 1; s += 2) {
        float tc = (float(s) * h.z - lo.z) / ld.z;
        if (tc > 0.002 && tc < best) {
          vec2 p = (lo.xy + ld.xy * tc) * invR;
          if (dot(p, p) <= 1.0) best = tc;
        }
      }
    }
    return best;
  }
  // Box / generic bounding prism
  vec3 invD = 1.0 / mix(ld, sign(ld + 1e-9) * 1e-6, lessThan(abs(ld), vec3(1e-6)));
  vec3 t0 = (-h - lo) * invD;
  vec3 t1 = ( h - lo) * invD;
  vec3 tmin = min(t0, t1);
  vec3 tmax = max(t0, t1);
  float tEnter = max(max(tmin.x, tmin.y), tmin.z);
  float tExit  = min(min(tmax.x, tmax.y), tmax.z);
  if (tExit < max(tEnter, 0.002) || tEnter >= tMax) return tMax;
  return tEnter > 0.002 ? tEnter : tMax;
}

bool isOccludedByWorld(vec3 ro, vec3 rd, float meshT) {
  float limit = meshT - 0.006;
  if (limit <= 0.01) return false;
  for (int i = 0; i < ${MAX_OCCLUDERS}; ++i) {
    if (i >= uOccCount) break;
    vec4 ps = texelFetch(uOccTex, ivec2(0, i), 0);
    vec4 hf = texelFetch(uOccTex, ivec2(1, i), 0);
    if (hf.w > 0.5) continue; // glass pane -> allow car to show through glass walls
    vec3 oc = ps.xyz - ro;
    float proj = dot(oc, rd);
    if (proj < -length(hf.xyz) || proj - length(hf.xyz) > limit) continue;
    float perp2 = dot(oc, oc) - proj * proj;
    if (perp2 > dot(hf.xyz, hf.xyz) * 1.45) continue;
    vec3 c0 = texelFetch(uOccTex, ivec2(2, i), 0).xyz;
    vec3 c1 = texelFetch(uOccTex, ivec2(3, i), 0).xyz;
    vec3 c2 = texelFetch(uOccTex, ivec2(4, i), 0).xyz;
    vec3 lo = invRot(-oc, c0, c1, c2);
    vec3 ld = invRot(rd, c0, c1, c2);
    float tHit = intersectPrimOcc(lo, ld, hf.xyz, int(ps.w + 0.5), limit);
    if (tHit < limit) return true;
  }
  return false;
}

float evalWorldShadow(vec3 pos, vec3 dir) {
  vec3 ro = pos + dir * 0.025;
  float vis = 1.0;
  for (int i = 0; i < ${MAX_OCCLUDERS}; ++i) {
    if (i >= uOccCount) break;
    vec4 ps = texelFetch(uOccTex, ivec2(0, i), 0);
    vec4 hf = texelFetch(uOccTex, ivec2(1, i), 0);
    if (hf.w > 0.5) continue;
    vec3 oc = ps.xyz - ro;
    float proj = dot(oc, dir);
    if (proj < -0.25 || proj > 26.0) continue;
    float perp2 = dot(oc, oc) - proj * proj;
    if (perp2 > dot(hf.xyz, hf.xyz) * 1.55) continue;
    vec3 c0 = texelFetch(uOccTex, ivec2(2, i), 0).xyz;
    vec3 c1 = texelFetch(uOccTex, ivec2(3, i), 0).xyz;
    vec3 c2 = texelFetch(uOccTex, ivec2(4, i), 0).xyz;
    vec3 lo = invRot(-oc, c0, c1, c2);
    vec3 ld = invRot(dir, c0, c1, c2);
    float tHit = intersectPrimOcc(lo, ld, hf.xyz, int(ps.w + 0.5), 26.0);
    if (tHit < 25.9) { vis *= 0.24; if (vis < 0.28) break; }
  }
  return vis;
}

float D_GGX(float NoH, float a) {
  float a2 = a * a;
  float d = (NoH * NoH) * (a2 - 1.0) + 1.0;
  return a2 / max(PI * d * d, 1e-5);
}

float G_Smith(float NoV, float NoL, float a) {
  float k = (a + 1.0) * (a + 1.0) * 0.125;
  float g1 = NoV / (NoV * (1.0 - k) + k);
  float g2 = NoL / (NoL * (1.0 - k) + k);
  return g1 * g2;
}

vec3 F_Schlick(float cosT, vec3 F0) {
  return F0 + (1.0 - F0) * pow(clamp(1.0 - cosT, 0.0, 1.0), 5.0);
}

vec3 evalBrdf(vec3 N, vec3 V, vec3 L, vec3 albedo, float rough, float metal, float coat) {
  float NoL = max(dot(N, L), 0.0);
  if (NoL <= 0.0) return vec3(0.0);
  float NoV = max(dot(N, V), 0.001);
  vec3 H = normalize(V + L);
  float NoH = max(dot(N, H), 0.0);
  float VoH = max(dot(V, H), 0.0);

  vec3 F0 = mix(vec3(0.04), albedo, metal);
  float a = max(0.045, rough * rough);
  float D = D_GGX(NoH, a);
  float G = G_Smith(NoV, NoL, rough);
  vec3 F = F_Schlick(VoH, F0);
  vec3 spec = (D * G * F) / max(4.0 * NoV * NoL, 1e-4);
  vec3 diff = (1.0 - F) * (1.0 - metal) * (albedo / PI);

  // Automotive OpenPBR clearcoat specular lobe
  float cD = D_GGX(NoH, 0.065);
  float cG = G_Smith(NoV, NoL, 0.08);
  float cF = 0.04 + 0.96 * pow(1.0 - VoH, 5.0);
  vec3 coatSpec = vec3(coat * 0.42 * cD * cG * cF / max(4.0 * NoV * NoL, 1e-4));

  return (diff + spec + coatSpec) * NoL;
}

vec3 acesFilm(vec3 x) {
  const float a = 2.51, b = 0.03, c = 2.43, d = 0.59, e = 0.14;
  return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main() {
  // Compute camera ray matching Pass 1 for pixel-exact architectural occlusion
  vec3 ro, rd;
  float meshT;
  if (uOrthoHalfH > 0.0) {
    vec3 delta = vWorldPos - uCamOrigin;
    float dAlong = dot(delta, uCamFwd);
    ro = vWorldPos - uCamFwd * dAlong;
    rd = uCamFwd;
    meshT = dAlong;
  } else {
    ro = uCamOrigin;
    vec3 d = vWorldPos - uCamOrigin;
    meshT = length(d);
    rd = d / max(meshT, 1e-5);
  }

  if (isOccludedByWorld(ro, rd, meshT)) discard;

  vec3 N = normalize(vWorldNormal);
  if (!gl_FrontFacing) N = -N;
  vec3 V = -rd;

  int fam = int(vPbr.w + 0.5);
  vec3 albedo = vAlbedo;
  if (fam == 6 && uPaintOverrideOn > 0.5) {
    albedo = uPaintOverride;
  }
  float rough = clamp(vPbr.x, 0.03, 0.96);
  float metal = clamp(vPbr.y, 0.0, 1.0);
  float coat  = clamp(vPbr.z, 0.0, 1.0);
  vec3 emit   = vEmit;
  if (fam == 13 && uHeadlightsOn < 0.5) emit = vec3(0.0);

  // Diagnostic view modes
  if (uViewMode == 2) {
    fragColor = vec4(pow(clamp(albedo, 0.0, 1.0), vec3(1.0 / 2.2)), 1.0);
    return;
  }
  if (uViewMode == 10) {
    fragColor = vec4(N * 0.5 + 0.5, 1.0);
    return;
  }
  if (uViewMode == 11) {
    fragColor = vec4(vec3(rough), 1.0);
    return;
  }
  if (uViewMode == 12) {
    fragColor = vec4(vec3(metal), 1.0);
    return;
  }

  // Ground-contact & wheel-arch ambient occlusion
  float hAbove = max(0.0, vWorldPos.z - uGroundZ);
  float heightAo = mix(0.28, 1.0, smoothstep(0.0, 0.48, hAbove));
  float upBias = 0.5 + 0.5 * N.z;
  float ao = mix(0.42, 1.0, upBias) * heightAo;

  // 1) Direct Sun illumination + world shadow
  vec3 sunL = normalize(uSunDir);
  float sunVis = (dot(N, sunL) > 0.0) ? evalWorldShadow(vWorldPos, sunL) : 0.0;
  vec3 color = evalBrdf(N, V, sunL, albedo, rough, metal, coat) * uSunIrradiance * sunVis;

  // 2) Live Area Lights (Lobby overhead softboxes, feature wall cove, reception/lounge, pit trusses)
  vec3 R = reflect(-V, N);
  for (int i = 0; i < ${MAX_AREA_LIGHTS}; ++i) {
    if (i >= uLightCount) break;
    vec3 lPos = uLightCenterW[i].xyz;
    float hw  = max(0.08, uLightCenterW[i].w);
    vec3 lNrm = normalize(uLightNormalH[i].xyz);
    float hh  = max(0.08, uLightNormalH[i].w);
    vec3 lRad = uLightRadOmni[i].xyz;
    float omni = uLightRadOmni[i].w;

    vec3 toL = lPos - vWorldPos;
    float dist = max(0.25, length(toL));
    vec3 L = toL / dist;
    float area = 4.0 * hw * hh;
    float facing = omni > 0.5 ? 1.0 : smoothstep(-0.15, 0.55, dot(lNrm, -L));
    float atten = (area * facing) / (dist * dist + area * 0.45 + 0.55);

    // Closest point on reflection ray for lush clearcoat softbox highlights along roof & hood
    float rProj = max(0.0, dot(toL, R));
    vec3 closest = vWorldPos + R * rProj;
    float specDist = length(closest - lPos);
    float lRadSize = max(hw, hh) * 1.15;
    float specHit = smoothstep(lRadSize * 2.2 + rough * 2.8, lRadSize * 0.25, specDist);
    vec3 Lspec = normalize(mix(L, R, clamp(specHit * (1.0 - rough * 0.75), 0.0, 0.78)));

    vec3 dTerm = evalBrdf(N, V, L, albedo, rough, metal, coat * 0.35);
    vec3 sTerm = evalBrdf(N, V, Lspec, albedo, max(0.04, rough * 0.7), metal, coat);
    color += mix(dTerm, sTerm, 0.55) * lRad * atten * 0.85;
  }

  // 3) Multi-bounce GI hemisphere + warm architectural floor/wall bounce + sky specular reflection
  vec3 skyCol = max(uSkyRadiance, vec3(0.08, 0.11, 0.16));
  vec3 warmWallBounce = vec3(0.28, 0.20, 0.14) * max(0.18, length(uSunIrradiance) * 0.12);
  vec3 floorBounce = vec3(0.14, 0.14, 0.16) * (0.65 + 0.35 * max(0.0, uSunDir.z));
  vec3 hemi = mix(floorBounce, skyCol * 1.35 + warmWallBounce * 0.45, upBias);
  // Subtle lateral terracotta/travertine bounce tint
  vec3 latTint = mix(vec3(0.32, 0.16, 0.11), vec3(0.18, 0.22, 0.28), clamp(N.x * 0.5 + 0.5, 0.0, 1.0));
  vec3 giIrr = (hemi + latTint * 0.18 * (1.0 - abs(N.z))) * ao;

  vec3 Fenv = F_Schlick(max(dot(N, V), 0.0), mix(vec3(0.04), albedo, metal));
  vec3 envRefl = mix(floorBounce * 0.85, skyCol * 1.75 + uSunIrradiance * 0.18, smoothstep(-0.18, 0.65, R.z));
  // Architectural ceiling studio strip reflection hint for glossy paint & glass
  float stripGlow = smoothstep(0.55, 0.94, R.z) * (1.0 - rough * 0.85);
  envRefl += vec3(0.65, 0.62, 0.58) * stripGlow * (0.45 + coat * 0.65);

  color += (1.0 - Fenv) * (1.0 - metal) * albedo * giIrr * 0.92;
  color += Fenv * envRefl * (1.0 - rough * 0.55) * mix(0.45, 1.0, ao);
  color += vec3(coat * 0.22 * pow(1.0 - max(dot(N, V), 0.0), 4.0)) * envRefl * ao;
  color += emit;

  // Selection Fresnel Rim Glow (#F6C66B Frontier Amber)
  if (uIsSelected > 0.5) {
    float rim = pow(clamp(1.0 - max(dot(N, V), 0.0), 0.0, 1.0), 2.8);
    color += vec3(1.15, 0.78, 0.24) * rim * 0.95;
  }

  vec3 mapped = acesFilm(max(vec3(0.0), color) * uExposure);
  vec3 srgb = pow(mapped, vec3(1.0 / 2.2));
  fragColor = vec4(srgb, 1.0);
}
`;

function CompileShader(Gl, Type, Src) {
  const Sh = Gl.createShader(Type);
  Gl.shaderSource(Sh, Src);
  Gl.compileShader(Sh);
  if (!Gl.getShaderParameter(Sh, Gl.COMPILE_STATUS)) {
    const Msg = Gl.getShaderInfoLog(Sh);
    Gl.deleteShader(Sh);
    throw new Error(`WorldMeshOverlay shader error: ${Msg}`);
  }
  return Sh;
}

function LinkProgram(Gl, VsSrc, FsSrc) {
  const Vs = CompileShader(Gl, Gl.VERTEX_SHADER, VsSrc);
  const Fs = CompileShader(Gl, Gl.FRAGMENT_SHADER, FsSrc);
  const Prog = Gl.createProgram();
  Gl.attachShader(Prog, Vs);
  Gl.attachShader(Prog, Fs);
  Gl.linkProgram(Prog);
  Gl.deleteShader(Vs);
  Gl.deleteShader(Fs);
  if (!Gl.getProgramParameter(Prog, Gl.LINK_STATUS)) {
    const Msg = Gl.getProgramInfoLog(Prog);
    Gl.deleteProgram(Prog);
    throw new Error(`WorldMeshOverlay link error: ${Msg}`);
  }
  return Prog;
}

export function ComposeModelMat4(Pos, RotRad, Scale) {
  const [Rx, Ry, Rz] = RotRad;
  const Cx = Math.cos(Rx), Sx = Math.sin(Rx);
  const Cy = Math.cos(Ry), Sy = Math.sin(Ry);
  const Cz = Math.cos(Rz), Sz = Math.sin(Rz);

  const R00 = Cz * Cy, R01 = Cz * Sy * Sx - Sz * Cx, R02 = Cz * Sy * Cx + Sz * Sx;
  const R10 = Sz * Cy, R11 = Sz * Sy * Sx + Cz * Cx, R12 = Sz * Sy * Cx - Cz * Sx;
  const R20 = -Sy,     R21 = Cy * Sx,                R22 = Cy * Cx;

  const [Sx0, Sy0, Sz0] = Scale;
  const Model = new Float32Array([
    R00 * Sx0, R10 * Sx0, R20 * Sx0, 0,
    R01 * Sy0, R11 * Sy0, R21 * Sy0, 0,
    R02 * Sz0, R12 * Sz0, R22 * Sz0, 0,
    Pos[0],    Pos[1],    Pos[2],    1,
  ]);
  const Normal = new Float32Array([
    R00 / Sx0, R10 / Sx0, R20 / Sx0,
    R01 / Sy0, R11 / Sy0, R21 / Sy0,
    R02 / Sz0, R12 / Sz0, R22 / Sz0,
  ]);
  return { Model, Normal };
}

function ComputeViewProj(Scene, Aspect) {
  const Origin = Scene.CameraOrigin || [5.5, -5.6, 2.05];
  const Target = Scene.CameraTarget || [0, 1.0, 0.6];
  const UpHint = Scene.WorldUp || [0, 0, 1];

  let Fx = Target[0] - Origin[0], Fy = Target[1] - Origin[1], Fz = Target[2] - Origin[2];
  const Fl = Math.hypot(Fx, Fy, Fz) || 1.0;
  Fx /= Fl; Fy /= Fl; Fz /= Fl;

  let Rx = Fy * UpHint[2] - Fz * UpHint[1];
  let Ry = Fz * UpHint[0] - Fx * UpHint[2];
  let Rz = Fx * UpHint[1] - Fy * UpHint[0];
  let Rl = Math.hypot(Rx, Ry, Rz);
  if (Rl < 1e-5) { Rx = 1; Ry = 0; Rz = 0; Rl = 1; }
  Rx /= Rl; Ry /= Rl; Rz /= Rl;

  const Ux = Ry * Fz - Rz * Fy;
  const Uy = Rz * Fx - Rx * Fz;
  const Uz = Rx * Fy - Ry * Fx;

  const ZNear = 0.08, ZFar = 120.0;
  const OrthoH = Number(Scene.OrthoHalfHeight || 0);
  let P00, P11, P22, P23, P32, P33;
  if (OrthoH > 0) {
    P00 = 1.0 / (OrthoH * Aspect);
    P11 = 1.0 / OrthoH;
    P22 = -2.0 / (ZFar - ZNear);
    P32 = -(ZFar + ZNear) / (ZFar - ZNear);
    P23 = 0.0;
    P33 = 1.0;
  } else {
    const TanHalf = Math.tan((Scene.VerticalFovRad || 0.72) * 0.5);
    P00 = 1.0 / (Aspect * TanHalf);
    P11 = 1.0 / TanHalf;
    P22 = -(ZFar + ZNear) / (ZFar - ZNear);
    P32 = (-2.0 * ZFar * ZNear) / (ZFar - ZNear);
    P23 = -1.0;
    P33 = 0.0;
  }

  // View matrix rows: R, U, -F
  const Tx = -(Rx * Origin[0] + Ry * Origin[1] + Rz * Origin[2]);
  const Ty = -(Ux * Origin[0] + Uy * Origin[1] + Uz * Origin[2]);
  const Tz = (Fx * Origin[0] + Fy * Origin[1] + Fz * Origin[2]);

  // Column-major P * V
  const Vp = new Float32Array([
    P00 * Rx, P11 * Ux, P22 * -Fx, P23 * -Fx,
    P00 * Ry, P11 * Uy, P22 * -Fy, P23 * -Fy,
    P00 * Rz, P11 * Uz, P22 * -Fz, P23 * -Fz,
    P00 * Tx, P11 * Ty, P22 * Tz + P32, P23 * Tz + P33,
  ]);
  return {
    ViewProj: Vp,
    CamFwd: [Fx, Fy, Fz],
    CamRight: [Rx, Ry, Rz],
    CamUp: [Ux, Uy, Uz],
  };
}

export class WorldMeshOverlayRenderer {
  constructor(Canvas) {
    this.Canvas = Canvas;
    this.Gl = Canvas.getContext("webgl2", {
      alpha: true,
      antialias: true,
      depth: true,
      premultipliedAlpha: false,
      preserveDrawingBuffer: false,
    });
    this.Ctx2d = this.Gl ? null : Canvas.getContext("2d");
    this.Supported = Boolean(this.Gl || this.Ctx2d);
    if (!this.Gl) return;

    const Gl = this.Gl;
    this.MeshProg = LinkProgram(Gl, MESH_VS, MESH_FS);
    this.GizmoProg = LinkProgram(Gl, GIZMO_VERTEX_SHADER, GIZMO_FRAGMENT_SHADER);

    this.Loc = {
      uViewProj: Gl.getUniformLocation(this.MeshProg, "uViewProj"),
      uModel: Gl.getUniformLocation(this.MeshProg, "uModel"),
      uNormalMat: Gl.getUniformLocation(this.MeshProg, "uNormalMat"),
      uCamOrigin: Gl.getUniformLocation(this.MeshProg, "uCamOrigin"),
      uCamFwd: Gl.getUniformLocation(this.MeshProg, "uCamFwd"),
      uCamRight: Gl.getUniformLocation(this.MeshProg, "uCamRight"),
      uCamUp: Gl.getUniformLocation(this.MeshProg, "uCamUp"),
      uOrthoHalfH: Gl.getUniformLocation(this.MeshProg, "uOrthoHalfH"),
      uExposure: Gl.getUniformLocation(this.MeshProg, "uExposure"),
      uViewMode: Gl.getUniformLocation(this.MeshProg, "uViewMode"),
      uSunDir: Gl.getUniformLocation(this.MeshProg, "uSunDir"),
      uSunIrradiance: Gl.getUniformLocation(this.MeshProg, "uSunIrradiance"),
      uSkyRadiance: Gl.getUniformLocation(this.MeshProg, "uSkyRadiance"),
      uLightCount: Gl.getUniformLocation(this.MeshProg, "uLightCount"),
      uLightCenterW: Gl.getUniformLocation(this.MeshProg, "uLightCenterW"),
      uLightNormalH: Gl.getUniformLocation(this.MeshProg, "uLightNormalH"),
      uLightRadOmni: Gl.getUniformLocation(this.MeshProg, "uLightRadOmni"),
      uOccCount: Gl.getUniformLocation(this.MeshProg, "uOccCount"),
      uOccTex: Gl.getUniformLocation(this.MeshProg, "uOccTex"),
      uPaintOverride: Gl.getUniformLocation(this.MeshProg, "uPaintOverride"),
      uPaintOverrideOn: Gl.getUniformLocation(this.MeshProg, "uPaintOverrideOn"),
      uHeadlightsOn: Gl.getUniformLocation(this.MeshProg, "uHeadlightsOn"),
      uIsSelected: Gl.getUniformLocation(this.MeshProg, "uIsSelected"),
      uGroundZ: Gl.getUniformLocation(this.MeshProg, "uGroundZ"),
    };
    this.GizmoLoc = {
      uViewProj: Gl.getUniformLocation(this.GizmoProg, "uViewProj"),
      uHighlightedHandle: Gl.getUniformLocation(this.GizmoProg, "uHighlightedHandle"),
    };

    // Occluder RGBA32F data texture (5 x MAX_OCCLUDERS)
    this.OccData = new Float32Array(5 * MAX_OCCLUDERS * 4);
    this.OccTex = Gl.createTexture();
    Gl.bindTexture(Gl.TEXTURE_2D, this.OccTex);
    Gl.texParameteri(Gl.TEXTURE_2D, Gl.TEXTURE_MIN_FILTER, Gl.NEAREST);
    Gl.texParameteri(Gl.TEXTURE_2D, Gl.TEXTURE_MAG_FILTER, Gl.NEAREST);
    Gl.texParameteri(Gl.TEXTURE_2D, Gl.TEXTURE_WRAP_S, Gl.CLAMP_TO_EDGE);
    Gl.texParameteri(Gl.TEXTURE_2D, Gl.TEXTURE_WRAP_T, Gl.CLAMP_TO_EDGE);
    Gl.texImage2D(Gl.TEXTURE_2D, 0, Gl.RGBA32F, 5, MAX_OCCLUDERS, 0, Gl.RGBA, Gl.FLOAT, this.OccData);

    // Gizmo VAO + dynamic VBO (matching TransformGizmo3D.js 12-float layout)
    this.GizmoVao = Gl.createVertexArray();
    this.GizmoVbo = Gl.createBuffer();
    Gl.bindVertexArray(this.GizmoVao);
    Gl.bindBuffer(Gl.ARRAY_BUFFER, this.GizmoVbo);
    const GStride = 12 * 4;
    Gl.enableVertexAttribArray(0);
    Gl.vertexAttribPointer(0, 3, Gl.FLOAT, false, GStride, 0);
    Gl.enableVertexAttribArray(1);
    Gl.vertexAttribPointer(1, 3, Gl.FLOAT, false, GStride, 12);
    Gl.enableVertexAttribArray(2);
    Gl.vertexAttribPointer(2, 4, Gl.FLOAT, false, GStride, 24);
    Gl.enableVertexAttribArray(3);
    Gl.vertexAttribPointer(3, 2, Gl.FLOAT, false, GStride, 40);
    Gl.bindVertexArray(null);

    this.LightCenterW = new Float32Array(MAX_AREA_LIGHTS * 4);
    this.LightNormalH = new Float32Array(MAX_AREA_LIGHTS * 4);
    this.LightRadOmni = new Float32Array(MAX_AREA_LIGHTS * 4);

    this.GpuBuffers = new Map();
    this._EnsureVehicleGpu();
  }

  _UploadStream(Key, FloatData) {
    if (!FloatData || FloatData.length === 0) return null;
    const Gl = this.Gl;
    const Prev = this.GpuBuffers.get(Key);
    if (Prev) {
      Gl.deleteVertexArray(Prev.vao);
      Gl.deleteBuffer(Prev.vbo);
    }
    const Vao = Gl.createVertexArray();
    const Vbo = Gl.createBuffer();
    Gl.bindVertexArray(Vao);
    Gl.bindBuffer(Gl.ARRAY_BUFFER, Vbo);
    Gl.bufferData(Gl.ARRAY_BUFFER, FloatData, Gl.STATIC_DRAW);
    const Stride = STRIDE_FLOATS * 4;
    Gl.enableVertexAttribArray(0);
    Gl.vertexAttribPointer(0, 3, Gl.FLOAT, false, Stride, 0);
    Gl.enableVertexAttribArray(1);
    Gl.vertexAttribPointer(1, 3, Gl.FLOAT, false, Stride, 12);
    Gl.enableVertexAttribArray(2);
    Gl.vertexAttribPointer(2, 3, Gl.FLOAT, false, Stride, 24);
    Gl.enableVertexAttribArray(3);
    Gl.vertexAttribPointer(3, 4, Gl.FLOAT, false, Stride, 36);
    Gl.enableVertexAttribArray(4);
    Gl.vertexAttribPointer(4, 3, Gl.FLOAT, false, Stride, 52);
    Gl.bindVertexArray(null);
    const Entry = { vao: Vao, vbo: Vbo, count: Math.floor(FloatData.length / STRIDE_FLOATS) };
    this.GpuBuffers.set(Key, Entry);
    return Entry;
  }

  _EnsureVehicleGpu() {
    if (this.GpuBuffers.has("vehicle-core")) return true;
    const Streams = EnsureVehicleStreamsSync();
    if (!Streams) return false;
    this._UploadStream("vehicle-core", Streams.core);
    this._UploadStream("vehicle-wing", Streams.wing);
    return true;
  }

  _EnsureCompiledAssetGpu(AssetId) {
    const Asset = GetCompiledMeshAsset(AssetId);
    if (!Asset) return null;
    const Key = `compiled:${AssetId}:${Asset.compiledAt || 0}`;
    if (this.GpuBuffers.has(Key)) return this.GpuBuffers.get(Key);
    const Stream = BuildCompiledAssetStream(Asset);
    if (!Stream) return null;
    return this._UploadStream(Key, Stream);
  }

  _RenderSoftware2d(ActiveScene, TargetW, TargetH, SelectedId) {
    const Ctx = this.Ctx2d;
    if (!Ctx) return;
    Ctx.clearRect(0, 0, TargetW, TargetH);
    const Instances = ActiveScene?.MeshInstances || [];
    if (Instances.length === 0) return;

    const Aspect = TargetW / Math.max(1, TargetH);
    const { ViewProj, CamFwd } = ComputeViewProj(ActiveScene, Aspect);
    const SunDir = ActiveScene.SunDir || [0.55, -0.45, 0.7];
    const SunLen = Math.hypot(SunDir[0], SunDir[1], SunDir[2]) || 1;
    const Sx = SunDir[0] / SunLen, Sy = SunDir[1] / SunLen, Sz = SunDir[2] / SunLen;
    const HalfW = TargetW * 0.5;
    const HalfH = TargetH * 0.5;
    const Tris = [];

    const PushStream = (Stream, Model, Normal, CustomPaint, PaintLin, HeadOn) => {
      if (!Stream) return;
      const VertCount = Math.floor(Stream.length / STRIDE_FLOATS);
      for (let I = 0; I + 2 < VertCount; I += 3) {
        const B0 = I * STRIDE_FLOATS;
        const B1 = (I + 1) * STRIDE_FLOATS;
        const B2 = (I + 2) * STRIDE_FLOATS;
        const P0x = Model[0] * Stream[B0] + Model[4] * Stream[B0 + 1] + Model[8] * Stream[B0 + 2] + Model[12];
        const P0y = Model[1] * Stream[B0] + Model[5] * Stream[B0 + 1] + Model[9] * Stream[B0 + 2] + Model[13];
        const P0z = Model[2] * Stream[B0] + Model[6] * Stream[B0 + 1] + Model[10] * Stream[B0 + 2] + Model[14];
        const P1x = Model[0] * Stream[B1] + Model[4] * Stream[B1 + 1] + Model[8] * Stream[B1 + 2] + Model[12];
        const P1y = Model[1] * Stream[B1] + Model[5] * Stream[B1 + 1] + Model[9] * Stream[B1 + 2] + Model[13];
        const P1z = Model[2] * Stream[B1] + Model[6] * Stream[B1 + 1] + Model[10] * Stream[B1 + 2] + Model[14];
        const P2x = Model[0] * Stream[B2] + Model[4] * Stream[B2 + 1] + Model[8] * Stream[B2 + 2] + Model[12];
        const P2y = Model[1] * Stream[B2] + Model[5] * Stream[B2 + 1] + Model[9] * Stream[B2 + 2] + Model[13];
        const P2z = Model[2] * Stream[B2] + Model[6] * Stream[B2 + 1] + Model[10] * Stream[B2 + 2] + Model[14];

        const C0w = ViewProj[3] * P0x + ViewProj[7] * P0y + ViewProj[11] * P0z + ViewProj[15];
        const C1w = ViewProj[3] * P1x + ViewProj[7] * P1y + ViewProj[11] * P1z + ViewProj[15];
        const C2w = ViewProj[3] * P2x + ViewProj[7] * P2y + ViewProj[11] * P2z + ViewProj[15];
        if (C0w <= 0.05 || C1w <= 0.05 || C2w <= 0.05) continue;

        const X0 = ((ViewProj[0] * P0x + ViewProj[4] * P0y + ViewProj[8] * P0z + ViewProj[12]) / C0w + 1) * HalfW;
        const Y0 = (1 - (ViewProj[1] * P0x + ViewProj[5] * P0y + ViewProj[9] * P0z + ViewProj[13]) / C0w) * HalfH;
        const X1 = ((ViewProj[0] * P1x + ViewProj[4] * P1y + ViewProj[8] * P1z + ViewProj[12]) / C1w + 1) * HalfW;
        const Y1 = (1 - (ViewProj[1] * P1x + ViewProj[5] * P1y + ViewProj[9] * P1z + ViewProj[13]) / C1w) * HalfH;
        const X2 = ((ViewProj[0] * P2x + ViewProj[4] * P2y + ViewProj[8] * P2z + ViewProj[12]) / C2w + 1) * HalfW;
        const Y2 = (1 - (ViewProj[1] * P2x + ViewProj[5] * P2y + ViewProj[9] * P2z + ViewProj[13]) / C2w) * HalfH;

        const Nx0 = Normal[0] * Stream[B0 + 3] + Normal[3] * Stream[B0 + 4] + Normal[6] * Stream[B0 + 5];
        const Ny0 = Normal[1] * Stream[B0 + 3] + Normal[4] * Stream[B0 + 4] + Normal[7] * Stream[B0 + 5];
        const Nz0 = Normal[2] * Stream[B0 + 3] + Normal[5] * Stream[B0 + 4] + Normal[8] * Stream[B0 + 5];
        const Nl = Math.hypot(Nx0, Ny0, Nz0) || 1;
        let Nx = Nx0 / Nl, Ny = Ny0 / Nl, Nz = Nz0 / Nl;
        if (Nx * CamFwd[0] + Ny * CamFwd[1] + Nz * CamFwd[2] > 0) {
          Nx = -Nx; Ny = -Ny; Nz = -Nz;
        }
        const Fam = Math.round(Stream[B0 + 12]);
        const Ar = Fam === 6 && CustomPaint ? PaintLin[0] : Stream[B0 + 6];
        const Ag = Fam === 6 && CustomPaint ? PaintLin[1] : Stream[B0 + 7];
        const Ab = Fam === 6 && CustomPaint ? PaintLin[2] : Stream[B0 + 8];
        const EmitScale = Fam === 13 && !HeadOn ? 0 : 1;
        const Diff = Math.max(0, Nx * Sx + Ny * Sy + Nz * Sz) * 0.95 + (0.28 + 0.22 * Nz);
        const R = Math.min(255, Math.round(Math.pow(Math.min(1, Ar * Diff + Stream[B0 + 13] * EmitScale), 1 / 2.2) * 255));
        const G = Math.min(255, Math.round(Math.pow(Math.min(1, Ag * Diff + Stream[B0 + 14] * EmitScale), 1 / 2.2) * 255));
        const B = Math.min(255, Math.round(Math.pow(Math.min(1, Ab * Diff + Stream[B0 + 15] * EmitScale), 1 / 2.2) * 255));
        Tris.push({ z: (C0w + C1w + C2w) * 0.3333, x0: X0, y0: Y0, x1: X1, y1: Y1, x2: X2, y2: Y2, c: `rgb(${R},${G},${B})` });
      }
    };

    const VehicleStreams = EnsureVehicleStreamsSync();
    for (const Inst of Instances) {
      const { Model, Normal } = ComposeModelMat4(Inst.Position, Inst.RotationRad, Inst.Scale);
      const CustomPaint =
        typeof Inst.PaintHex === "string" &&
        /^#[0-9a-f]{6}$/i.test(Inst.PaintHex) &&
        Inst.PaintHex.toLowerCase() !== "#e4e440" &&
        Inst.PaintHex.toLowerCase() !== "#d4d4d4";
      const PaintLin = CustomPaint ? HexToLinear(Inst.PaintHex, [0.68, 0.8, 0.0]) : [0.68, 0.8, 0.0];
      const HeadOn = Inst.HeadlightsOn !== false;
      if (Inst.Kind === "BP_ControlVehicle" && VehicleStreams) {
        PushStream(VehicleStreams.core, Model, Normal, CustomPaint, PaintLin, HeadOn);
        if (Inst.WingVisible !== false) {
          PushStream(VehicleStreams.wing, Model, Normal, CustomPaint, PaintLin, HeadOn);
        }
      } else if (Inst.CompiledAssetId) {
        const Asset = GetCompiledMeshAsset(Inst.CompiledAssetId);
        if (Asset) PushStream(BuildCompiledAssetStream(Asset), Model, Normal, CustomPaint, PaintLin, HeadOn);
      }
    }

    Tris.sort((A, B) => B.z - A.z);
    for (let I = 0; I < Tris.length; ++I) {
      const T = Tris[I];
      Ctx.fillStyle = T.c;
      Ctx.beginPath();
      Ctx.moveTo(T.x0, T.y0);
      Ctx.lineTo(T.x1, T.y1);
      Ctx.lineTo(T.x2, T.y2);
      Ctx.closePath();
      Ctx.fill();
    }
  }

  Render(ActiveScene, Width, Height, SelectedId, ViewMode = 0, GizmoGeo = null, HighlightedHandle = 0) {
    if (!this.Supported) return;
    const TargetW = Math.max(1, Math.round(Width));
    const TargetH = Math.max(1, Math.round(Height));
    if (this.Canvas.width !== TargetW || this.Canvas.height !== TargetH) {
      this.Canvas.width = TargetW;
      this.Canvas.height = TargetH;
    }
    if (!this.Gl) {
      this._RenderSoftware2d(ActiveScene, TargetW, TargetH, SelectedId);
      return;
    }
    const Gl = this.Gl;

    Gl.viewport(0, 0, TargetW, TargetH);
    Gl.clearColor(0, 0, 0, 0);
    Gl.clearDepth(1.0);
    Gl.clear(Gl.COLOR_BUFFER_BIT | Gl.DEPTH_BUFFER_BIT);

    const Instances = ActiveScene?.MeshInstances || [];
    if (Instances.length === 0) return;

    const Aspect = TargetW / Math.max(1, TargetH);
    const { ViewProj, CamFwd, CamRight, CamUp } = ComputeViewProj(ActiveScene, Aspect);

    if (Instances.length > 0) {
      this._EnsureVehicleGpu();

      // Pack non-proxy architectural primitives for pixel-exact world occlusion & soft shadows
      const Prims = ActiveScene.Primitives || [];
      let OccCount = 0;
      for (let I = 0; I < Prims.length && OccCount < MAX_OCCLUDERS; ++I) {
        const P = Prims[I];
        if (P.MeshProxy) continue;
        const Base = OccCount * 5 * 4;
        this.OccData[Base + 0] = P.Center[0];
        this.OccData[Base + 1] = P.Center[1];
        this.OccData[Base + 2] = P.Center[2];
        this.OccData[Base + 3] = P.Shape;
        this.OccData[Base + 4] = P.HalfExtents[0];
        this.OccData[Base + 5] = P.HalfExtents[1];
        this.OccData[Base + 6] = P.HalfExtents[2];
        this.OccData[Base + 7] = P.GlassShell ? 1.0 : 0.0;
        const [Rx, Ry, Rz] = P.RotationRad || [0, 0, 0];
        const Cx = Math.cos(Rx), Sx = Math.sin(Rx);
        const Cy = Math.cos(Ry), Sy = Math.sin(Ry);
        const Cz = Math.cos(Rz), Sz = Math.sin(Rz);
        this.OccData[Base + 8] = Cz * Cy;
        this.OccData[Base + 9] = Sz * Cy;
        this.OccData[Base + 10] = -Sy;
        this.OccData[Base + 11] = 0;
        this.OccData[Base + 12] = Cz * Sy * Sx - Sz * Cx;
        this.OccData[Base + 13] = Sz * Sy * Sx + Cz * Cx;
        this.OccData[Base + 14] = Cy * Sx;
        this.OccData[Base + 15] = 0;
        this.OccData[Base + 16] = Cz * Sy * Cx + Sz * Sx;
        this.OccData[Base + 17] = Sz * Sy * Cx - Cz * Sx;
        this.OccData[Base + 18] = Cy * Cx;
        this.OccData[Base + 19] = 0;
        OccCount++;
      }
      Gl.activeTexture(Gl.TEXTURE0);
      Gl.bindTexture(Gl.TEXTURE_2D, this.OccTex);
      Gl.pixelStorei(Gl.UNPACK_FLIP_Y_WEBGL, false);
      const OccRows = Math.max(1, OccCount);
      Gl.texSubImage2D(
        Gl.TEXTURE_2D,
        0,
        0,
        0,
        5,
        OccRows,
        Gl.RGBA,
        Gl.FLOAT,
        this.OccData.subarray(0, 5 * OccRows * 4),
      );

      // Pack ActiveScene Area Lights
      const Lights = ActiveScene.AreaLights || [];
      const LCount = Math.min(MAX_AREA_LIGHTS, Lights.length);
      for (let I = 0; I < LCount; ++I) {
        const L = Lights[I];
        this.LightCenterW[I * 4 + 0] = L.Center[0];
        this.LightCenterW[I * 4 + 1] = L.Center[1];
        this.LightCenterW[I * 4 + 2] = L.Center[2];
        this.LightCenterW[I * 4 + 3] = L.HalfWidth || 0.5;
        this.LightNormalH[I * 4 + 0] = L.Normal[0];
        this.LightNormalH[I * 4 + 1] = L.Normal[1];
        this.LightNormalH[I * 4 + 2] = L.Normal[2];
        this.LightNormalH[I * 4 + 3] = L.HalfHeight || 0.5;
        this.LightRadOmni[I * 4 + 0] = L.Radiance[0];
        this.LightRadOmni[I * 4 + 1] = L.Radiance[1];
        this.LightRadOmni[I * 4 + 2] = L.Radiance[2];
        this.LightRadOmni[I * 4 + 3] = L.Omni ? 1.0 : 0.0;
      }

      Gl.useProgram(this.MeshProg);
      Gl.enable(Gl.DEPTH_TEST);
      Gl.depthFunc(Gl.LEQUAL);
      Gl.depthMask(true);
      Gl.disable(Gl.BLEND);
      Gl.disable(Gl.CULL_FACE);

      Gl.uniformMatrix4fv(this.Loc.uViewProj, false, ViewProj);
      Gl.uniform3fv(this.Loc.uCamOrigin, ActiveScene.CameraOrigin);
      Gl.uniform3fv(this.Loc.uCamFwd, CamFwd);
      Gl.uniform3fv(this.Loc.uCamRight, CamRight);
      Gl.uniform3fv(this.Loc.uCamUp, CamUp);
      Gl.uniform1f(this.Loc.uOrthoHalfH, Number(ActiveScene.OrthoHalfHeight || 0));
      Gl.uniform1f(this.Loc.uExposure, Number(ActiveScene.ExposureGain ?? 1.0));
      Gl.uniform1i(this.Loc.uViewMode, Number(ViewMode || 0));
      Gl.uniform3fv(this.Loc.uSunDir, ActiveScene.SunDir || [0.55, -0.45, 0.70]);
      Gl.uniform3fv(this.Loc.uSunIrradiance, ActiveScene.SunIrradiance || [5.4, 4.8, 4.1]);
      Gl.uniform3fv(this.Loc.uSkyRadiance, ActiveScene.SkyRadiance || [0.18, 0.28, 0.44]);
      Gl.uniform1i(this.Loc.uLightCount, LCount);
      Gl.uniform4fv(this.Loc.uLightCenterW, this.LightCenterW);
      Gl.uniform4fv(this.Loc.uLightNormalH, this.LightNormalH);
      Gl.uniform4fv(this.Loc.uLightRadOmni, this.LightRadOmni);
      Gl.uniform1i(this.Loc.uOccCount, OccCount);
      Gl.uniform1i(this.Loc.uOccTex, 0);

      for (const Inst of Instances) {
        const { Model, Normal } = ComposeModelMat4(Inst.Position, Inst.RotationRad, Inst.Scale);
        Gl.uniformMatrix4fv(this.Loc.uModel, false, Model);
        Gl.uniformMatrix3fv(this.Loc.uNormalMat, false, Normal);
        Gl.uniform1f(this.Loc.uIsSelected, SelectedId === Inst.Id ? 1.0 : 0.0);
        Gl.uniform1f(this.Loc.uGroundZ, Inst.Position[2]);
        Gl.uniform1f(this.Loc.uHeadlightsOn, Inst.HeadlightsOn !== false ? 1.0 : 0.0);

        const CustomPaint =
          typeof Inst.PaintHex === "string" &&
          /^#[0-9a-f]{6}$/i.test(Inst.PaintHex) &&
          Inst.PaintHex.toLowerCase() !== "#e4e440" &&
          Inst.PaintHex.toLowerCase() !== "#d4d4d4";
        if (CustomPaint) {
          Gl.uniform3fv(this.Loc.uPaintOverride, HexToLinear(Inst.PaintHex, [0.68, 0.8, 0.0]));
          Gl.uniform1f(this.Loc.uPaintOverrideOn, 1.0);
        } else {
          Gl.uniform3f(this.Loc.uPaintOverride, 0.68, 0.8, 0.0);
          Gl.uniform1f(this.Loc.uPaintOverrideOn, 0.0);
        }

        if (Inst.Kind === "BP_ControlVehicle") {
          const Core = this.GpuBuffers.get("vehicle-core");
          if (Core) {
            Gl.bindVertexArray(Core.vao);
            Gl.drawArrays(Gl.TRIANGLES, 0, Core.count);
          }
          if (Inst.WingVisible !== false) {
            const Wing = this.GpuBuffers.get("vehicle-wing");
            if (Wing) {
              Gl.bindVertexArray(Wing.vao);
              Gl.drawArrays(Gl.TRIANGLES, 0, Wing.count);
            }
          }
        } else if (Inst.CompiledAssetId) {
          const Mesh = this._EnsureCompiledAssetGpu(Inst.CompiledAssetId);
          if (Mesh) {
            Gl.bindVertexArray(Mesh.vao);
            Gl.drawArrays(Gl.TRIANGLES, 0, Mesh.count);
          }
        }
      }
      Gl.bindVertexArray(null);
    }

    // Always-on-top 3D Slate Transform Gizmo (prevents vehicle/meshes from occluding gizmo handles!)
    if (GizmoGeo && (GizmoGeo.OpaqueVertexCount > 0 || GizmoGeo.TranslucentVertexCount > 0)) {
      Gl.clear(Gl.DEPTH_BUFFER_BIT);
      Gl.enable(Gl.DEPTH_TEST);
      Gl.depthFunc(Gl.LEQUAL);
      Gl.disable(Gl.CULL_FACE);
      Gl.useProgram(this.GizmoProg);
      Gl.uniformMatrix4fv(this.GizmoLoc.uViewProj, false, ViewProj);
      Gl.uniform1f(this.GizmoLoc.uHighlightedHandle, Number(HighlightedHandle || 0));
      Gl.bindVertexArray(this.GizmoVao);
      Gl.bindBuffer(Gl.ARRAY_BUFFER, this.GizmoVbo);

      if (GizmoGeo.OpaqueVertexCount > 0 && GizmoGeo.OpaqueBuffer) {
        Gl.disable(Gl.BLEND);
        Gl.depthMask(true);
        Gl.bufferData(Gl.ARRAY_BUFFER, GizmoGeo.OpaqueBuffer, Gl.STREAM_DRAW);
        Gl.drawArrays(Gl.TRIANGLES, 0, GizmoGeo.OpaqueVertexCount);
      }
      if (GizmoGeo.TranslucentVertexCount > 0 && GizmoGeo.TranslucentBuffer) {
        Gl.enable(Gl.BLEND);
        Gl.blendFunc(Gl.SRC_ALPHA, Gl.ONE_MINUS_SRC_ALPHA);
        Gl.depthMask(false);
        Gl.bufferData(Gl.ARRAY_BUFFER, GizmoGeo.TranslucentBuffer, Gl.STREAM_DRAW);
        Gl.drawArrays(Gl.TRIANGLES, 0, GizmoGeo.TranslucentVertexCount);
        Gl.depthMask(true);
        Gl.disable(Gl.BLEND);
      }
      Gl.bindVertexArray(null);
    }
  }

  Dispose() {
    if (!this.Gl) return;
    const Gl = this.Gl;
    for (const Entry of this.GpuBuffers.values()) {
      Gl.deleteVertexArray(Entry.vao);
      Gl.deleteBuffer(Entry.vbo);
    }
    this.GpuBuffers.clear();
    Gl.deleteVertexArray(this.GizmoVao);
    Gl.deleteBuffer(this.GizmoVbo);
    Gl.deleteTexture(this.OccTex);
    Gl.deleteProgram(this.MeshProg);
    Gl.deleteProgram(this.GizmoProg);
  }
}
