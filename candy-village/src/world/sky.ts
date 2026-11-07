/**
 * Candy sky: pastel gradient, a sun glow, and procedural marshmallow clouds.
 * The same dome also seeds the image-based lighting environment (PMREM), so
 * glossy candy surfaces reflect a sky rather than a flat colour.
 */
import { BackSide, Color, Mesh, ShaderMaterial, SphereGeometry, Vector3 } from 'three';
import { GLSL_NOISE } from '../materials/glsl';

export const SUN_DIRECTION = new Vector3(0.55, 0.62, 0.35).normalize();

export function buildSky(): { mesh: Mesh; material: ShaderMaterial } {
  const material = new ShaderMaterial({
    side: BackSide,
    depthWrite: false,
    uniforms: {
      uSunDir: { value: SUN_DIRECTION.clone() },
      uTime: { value: 0 },
      uZenith: { value: new Color('#5aa6ff') },
      uHorizon: { value: new Color('#ffc2dd') },
      uSunTint: { value: new Color('#fff1cf') },
    },
    vertexShader: /* glsl */ `
      varying vec3 vDir;
      void main() {
        vDir = normalize(position);
        vec4 p = modelViewMatrix * vec4(position, 1.0);
        gl_Position = projectionMatrix * p;
        gl_Position.z = gl_Position.w; // keep the dome at the far plane
      }`,
    fragmentShader: /* glsl */ `
      varying vec3 vDir;
      uniform vec3 uSunDir;
      uniform float uTime;
      uniform vec3 uZenith;
      uniform vec3 uHorizon;
      uniform vec3 uSunTint;
      ${GLSL_NOISE}
      void main() {
        vec3 d = normalize(vDir);
        float h = clamp(d.y, -0.2, 1.0);
        vec3 col = mix(uHorizon, uZenith, smoothstep(0.0, 0.55, h));
        col = mix(col, uHorizon * 0.95, smoothstep(0.0, -0.2, d.y));

        float sd = max(dot(d, uSunDir), 0.0);
        col += uSunTint * (pow(sd, 400.0) * 3.0 + pow(sd, 10.0) * 0.35);

        // Marshmallow clouds projected onto a plane above the horizon.
        vec2 uv = d.xz / max(d.y + 0.15, 0.08) * 0.9;
        float n = fbm4(vec3(uv + vec2(uTime * 0.012, 0.0), 0.7)) * 0.5 + 0.5;
        float cloud = smoothstep(0.52, 0.78, n) * smoothstep(0.05, 0.3, d.y);
        vec3 cloudCol = mix(vec3(1.0, 0.92, 0.96), uSunTint, sd * 0.6);
        col = mix(col, cloudCol, cloud * 0.9);

        gl_FragColor = vec4(col, 1.0);
      }`,
  });
  const geo = new SphereGeometry(900, 32, 16);
  const mesh = new Mesh(geo, material);
  mesh.name = 'Sky';
  mesh.frustumCulled = false;
  return { mesh, material };
}
