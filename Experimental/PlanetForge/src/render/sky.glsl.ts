/**
 * Shared sky model, used by the sky dome *and* by the water shader for
 * reflections. Sharing one function is what keeps a reflected sunset matching
 * the actual sunset instead of being an unrelated cubemap.
 */

export const SKY_GLSL = /* glsl */ `
vec3 skyRadiance(vec3 dir, vec3 sunDir, float turbidity) {
  float h = clamp(dir.y * 0.5 + 0.5, 0.0, 1.0);
  // Rayleigh-ish ramp: deep blue zenith to pale horizon.
  vec3 zenith = vec3(0.055, 0.145, 0.36);
  vec3 horizon = vec3(0.62, 0.72, 0.83);
  float t = pow(clamp(1.0 - dir.y, 0.0, 1.0), 2.4);
  vec3 col = mix(zenith, horizon, t);

  // Sun glow, Mie forward scattering.
  float mu = max(dot(dir, sunDir), 0.0);
  float glow = pow(mu, 8.0) * 0.22 + pow(mu, 220.0) * 2.4 + pow(mu, 2000.0) * 12.0;
  vec3 sunTint = mix(vec3(1.0, 0.55, 0.26), vec3(1.0, 0.96, 0.9), clamp(sunDir.y * 2.2, 0.0, 1.0));
  col += sunTint * glow;

  // Sunset band on the horizon opposite/around the sun.
  float horizonGlow = pow(clamp(1.0 - abs(dir.y) * 3.4, 0.0, 1.0), 2.0) * pow(mu, 2.0);
  col += vec3(0.9, 0.42, 0.18) * horizonGlow * 0.55 * clamp(1.0 - sunDir.y * 3.0, 0.0, 1.0);

  // Ground haze below the horizon so the sea meets the sky without a hard edge.
  col = mix(col, vec3(0.30, 0.36, 0.42), smoothstep(0.02, -0.14, dir.y));
  col *= mix(0.55, 1.0, smoothstep(-0.05, 0.35, dir.y));
  return col * turbidity;
}
`;
