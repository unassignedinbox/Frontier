import * as THREE from 'three';

/**
 * Ocean materials. There is deliberately no wave texture here.
 *
 * NEAR_WATER receives vertex heights from ShallowWaterSolver. FAR_WATER
 * displaces tessellated clipmap geometry with a small sum of directional
 * Gerstner-like basis waves as an open-boundary condition. Both paths shade
 * from geometric derivatives; no normal map or scrolling image is sampled.
 */

const vertexShader = /* glsl */`
precision highp float;
#include <common>
#include <logdepthbuf_pars_vertex>

uniform float uTime;
uniform vec2 uCameraWorld;
uniform vec2 uWind;
uniform float uWaveHeight;
uniform float uWaveSpeed;

out vec3 vWorldPos;
out vec2 vWorldXZ;
out float vWet;
out float vDepth;
out float vFoam;
out float vDistance;

#ifdef NEAR_WATER
in float wet;
in float depth;
#endif
#ifdef FAR_WATER
in float depth;
#endif

vec2 waveCoord(vec3 renderPos) {
  // modelMatrix positions are camera-relative; add the double-precision frame
  // origin handed in as a uniform, then wrap to keep float precision useful.
  return mod(renderPos.xz + uCameraWorld, 220000.0);
}

void main() {
  vec4 local = vec4(position, 1.0);
  vWet = 1.0;
  vDepth = 100.0;
  vFoam = 0.0;

  #ifdef FAR_WATER
    vec4 world0 = modelMatrix * local;
    vec2 p = waveCoord(world0.xyz);
    vec2 w = normalize(uWind + vec2(0.001, 0.001));
    vec2 d1 = normalize(w + vec2(0.30, -0.12));
    vec2 d2 = normalize(vec2(-w.y, w.x) + w * 0.35);
    vec2 d3 = normalize(w * 0.7 + vec2(-0.45, 0.55));
    vec2 d4 = normalize(w * 0.4 + vec2(0.62, -0.78));
    float t = uTime * uWaveSpeed;
    float amp = uWaveHeight * (0.45 + 0.55 * clamp(length(uWind), 0.0, 1.0));

    float k1 = 6.2831853 / 38.0;
    float k2 = 6.2831853 / 91.0;
    float k3 = 6.2831853 / 190.0;
    float k4 = 6.2831853 / 420.0;
    float h = 0.0;
    h += amp * 0.46 * sin(dot(p, d1) * k1 - t * 1.35);
    h += amp * 0.34 * sin(dot(p, d2) * k2 - t * 0.95);
    h += amp * 0.22 * sin(dot(p, d3) * k3 - t * 0.62);
    h += amp * 0.12 * sin(dot(p, d4) * k4 - t * 0.38);
    local.y += h;
  #else
    vWet = wet;
    vDepth = depth;
    vFoam = clamp(1.0 - depth / 3.2, 0.0, 1.0) * smoothstep(0.02, 0.25, wet);
  #endif

  #ifdef FAR_WATER
    // Coarse rings carry SDF-derived bathymetry as a vertex attribute so they
    // are clipped at the shoreline instead of painting water over continents.
    vDepth = depth;
    vWet = smoothstep(0.0, 1.5, depth);
    vFoam = clamp(1.0 - depth / 4.0, 0.0, 1.0) * vWet;
  #endif

  vec4 world = modelMatrix * local;
  vWorldPos = world.xyz;
  vWorldXZ = waveCoord(world.xyz);
  vDistance = length(vWorldPos);
  gl_Position = projectionMatrix * modelViewMatrix * local;
  #include <logdepthbuf_vertex>
}
`;

const fragmentShader = /* glsl */`
precision highp float;
#include <common>
#include <logdepthbuf_pars_fragment>

uniform float uTime;
uniform vec3 uSunDir;
uniform vec3 uDeepColor;
uniform vec3 uShallowColor;
uniform vec3 uFoamColor;
uniform vec3 uHorizonColor;
uniform float uOpacity;

in vec3 vWorldPos;
in vec2 vWorldXZ;
in float vWet;
in float vDepth;
in float vFoam;
in float vDistance;

layout(location = 0) out vec4 outColor;

void main() {
  #include <logdepthbuf_fragment>
  if (vWet < 0.055 || vDepth <= 0.02) discard;

  // Geometric normal from the displaced triangle, in render/world axes.
  vec3 dpdx = dFdx(vWorldPos);
  vec3 dpdy = dFdy(vWorldPos);
  vec3 n = normalize(cross(dpdx, dpdy));
  if (n.y < 0.0) n = -n;

  // Tiny procedural capillary slope. This is maths, not a texture lookup.
  vec2 rp = vWorldXZ * 0.31;
  vec2 ripple = vec2(
    sin(rp.x + uTime * 1.7) * cos(rp.y * 0.83 - uTime * 1.15),
    cos(rp.x * 0.77 - uTime * 1.35) * sin(rp.y + uTime * 0.95)
  );
  n = normalize(n + vec3(ripple.x, 0.0, ripple.y) * 0.035);

  vec3 viewDir = normalize(-vWorldPos);
  vec3 sun = normalize(uSunDir);
  float diffuse = max(dot(n, sun), 0.0);
  float fresnel = pow(1.0 - clamp(dot(n, viewDir), 0.0, 1.0), 3.0);
  float spec = pow(max(dot(reflect(-sun, n), viewDir), 0.0), 90.0);

  float depthMix = clamp(vDepth / 46.0, 0.0, 1.0);
  vec3 base = mix(uShallowColor, uDeepColor, depthMix);
  vec3 color = base * (0.30 + 0.70 * diffuse);
  color += vec3(0.48, 0.72, 0.95) * fresnel * 0.42;
  color += vec3(1.0, 0.98, 0.90) * spec * 0.32;
  color = mix(color, uFoamColor, clamp(vFoam, 0.0, 1.0) * 0.78);

  float fog = 1.0 - exp(-vDistance * 0.0000095);
  color = mix(color, uHorizonColor, clamp(fog, 0.0, 0.72));

  float alpha = uOpacity * mix(0.56, 0.94, fresnel);
  alpha *= smoothstep(0.02, 0.30, vWet);
  outColor = vec4(color, clamp(alpha, 0.0, 0.96));
}
`;

export interface WaterMaterialBundle {
  near: THREE.ShaderMaterial;
  far: THREE.ShaderMaterial;
  uniforms: Record<string, THREE.IUniform>;
}

export function createWaterMaterials(): WaterMaterialBundle {
  const uniforms: Record<string, THREE.IUniform> = {
    uTime: { value: 0 },
    uCameraWorld: { value: new THREE.Vector2(0, 0) },
    uWind: { value: new THREE.Vector2(0.7, 0.5) },
    uWaveHeight: { value: 1.15 },
    uWaveSpeed: { value: 1 },
    uSunDir: { value: new THREE.Vector3(0.38, 0.86, 0.34).normalize() },
    uDeepColor: { value: new THREE.Vector3(0.018, 0.105, 0.205) },
    uShallowColor: { value: new THREE.Vector3(0.075, 0.36, 0.42) },
    uFoamColor: { value: new THREE.Vector3(0.82, 0.92, 0.94) },
    uHorizonColor: { value: new THREE.Vector3(0.47, 0.65, 0.78) },
    uOpacity: { value: 0.82 },
  };
  const common = {
    uniforms,
    vertexShader,
    fragmentShader,
    glslVersion: THREE.GLSL3,
    transparent: true,
    depthWrite: false,
    depthTest: true,
    side: THREE.DoubleSide,
    toneMapped: false,
    blending: THREE.NormalBlending,
  };
  const near = new THREE.ShaderMaterial({ ...common, defines: { NEAR_WATER: '' } });
  const far = new THREE.ShaderMaterial({
    ...common,
    defines: { FAR_WATER: '' },
    polygonOffset: true,
    polygonOffsetFactor: 1,
    polygonOffsetUnits: 1,
  });
  return { near, far, uniforms };
}
