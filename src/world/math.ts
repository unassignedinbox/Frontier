/**
 * Frontier math helpers.
 *
 * All world-space distances are metres. JS numbers stay double precision in the
 * simulation/world model; only the camera-relative values handed to WebGL are
 * small enough for Float32 vertex buffers.
 */

export const TAU = Math.PI * 2;
export const PLANET_RADIUS = 6_371_000;

export function clamp(v: number, lo: number, hi: number): number {
  return v < lo ? lo : v > hi ? hi : v;
}

export function lerp(a: number, b: number, t: number): number {
  return a + (b - a) * t;
}

/** GLSL-style smoothstep. edge0 may be greater than edge1. */
export function smoothstep(edge0: number, edge1: number, x: number): number {
  const t = clamp((x - edge0) / (edge1 - edge0 || 1e-9), 0, 1);
  return t * t * (3 - 2 * t);
}

export function smoothMin(a: number, b: number, k: number): number {
  if (k <= 0) return Math.min(a, b);
  const h = clamp(0.5 + 0.5 * (b - a) / k, 0, 1);
  return lerp(b, a, h) - k * h * (1 - h);
}

export function smoothMax(a: number, b: number, k: number): number {
  if (k <= 0) return Math.max(a, b);
  const h = clamp(0.5 - 0.5 * (b - a) / k, 0, 1);
  return lerp(b, a, h) + k * h * (1 - h);
}

export function mixColor(a: RGB, b: RGB, t: number): RGB {
  return [lerp(a[0], b[0], t), lerp(a[1], b[1], t), lerp(a[2], b[2], t)];
}

export type RGB = [number, number, number];
export type Vec2 = [number, number];

/** Fast deterministic integer hash; no Math.sin and no allocation. */
export function hash2(ix: number, iy: number, seed: number): number {
  let h = Math.imul(ix | 0, 374761393) ^ Math.imul(iy | 0, 668265263) ^ Math.imul(seed | 0, 982451653);
  h = Math.imul(h ^ (h >>> 13), 1274126177);
  return ((h ^ (h >>> 16)) >>> 0) / 4294967295;
}

export function valueNoise(x: number, y: number, seed: number): number {
  const x0 = Math.floor(x);
  const y0 = Math.floor(y);
  const fx0 = x - x0;
  const fy0 = y - y0;
  const u = fx0 * fx0 * (3 - 2 * fx0);
  const v = fy0 * fy0 * (3 - 2 * fy0);
  const a = hash2(x0, y0, seed);
  const b = hash2(x0 + 1, y0, seed);
  const c = hash2(x0, y0 + 1, seed);
  const d = hash2(x0 + 1, y0 + 1, seed);
  return lerp(lerp(a, b, u), lerp(c, d, u), v);
}

export function fbm(x: number, y: number, seed: number, octaves = 5): number {
  let amp = 0.5;
  let freq = 1;
  let sum = 0;
  let norm = 0;
  for (let o = 0; o < octaves; o++) {
    sum += amp * valueNoise(x * freq, y * freq, seed + o * 1013);
    norm += amp;
    amp *= 0.5;
    freq *= 2.03;
  }
  return sum / (norm || 1);
}

export function ridgedNoise(x: number, y: number, seed: number, octaves = 4): number {
  let amp = 0.5;
  let freq = 1;
  let sum = 0;
  let norm = 0;
  for (let o = 0; o < octaves; o++) {
    const n = valueNoise(x * freq, y * freq, seed + o * 1777);
    sum += amp * (1 - Math.abs(n * 2 - 1));
    norm += amp;
    amp *= 0.5;
    freq *= 2.07;
  }
  return sum / (norm || 1);
}

export function distanceToSegment(px: number, pz: number, ax: number, az: number, bx: number, bz: number): number {
  const vx = bx - ax;
  const vz = bz - az;
  const wx = px - ax;
  const wz = pz - az;
  const len2 = vx * vx + vz * vz;
  const t = len2 > 0 ? clamp((wx * vx + wz * vz) / len2, 0, 1) : 0;
  const dx = wx - vx * t;
  const dz = wz - vz * t;
  return Math.sqrt(dx * dx + dz * dz);
}

/** Signed distance to a polygon: negative inside, positive outside. */
export function polygonSdf(points: Vec2[], x: number, z: number): number {
  if (points.length < 3) return 1e9;
  let inside = false;
  let minDist2 = Infinity;
  for (let i = 0, j = points.length - 1; i < points.length; j = i++) {
    const ax = points[j][0], az = points[j][1];
    const bx = points[i][0], bz = points[i][1];
    if ((az > z) !== (bz > z) && x < ((bx - ax) * (z - az)) / (bz - az || 1e-9) + ax) inside = !inside;
    const d = distanceToSegment(x, z, ax, az, bx, bz);
    if (d * d < minDist2) minDist2 = d * d;
  }
  const d = Math.sqrt(minDist2);
  return inside ? -d : d;
}

/** Local tangent-plane geographic conversion around the world origin. */
export function geoToLocal(lonDeg: number, latDeg: number, originLonDeg: number, originLatDeg: number): Vec2 {
  const lat0 = originLatDeg * Math.PI / 180;
  const dLon = ((lonDeg - originLonDeg + 540) % 360 - 180) * Math.PI / 180;
  const dLat = (latDeg - originLatDeg) * Math.PI / 180;
  return [PLANET_RADIUS * Math.cos(lat0) * dLon, -PLANET_RADIUS * dLat];
}

export function localToGeo(x: number, z: number, originLonDeg: number, originLatDeg: number): { lon: number; lat: number } {
  const lat0 = originLatDeg * Math.PI / 180;
  const lon = originLonDeg + (x / (PLANET_RADIUS * Math.cos(lat0))) * 180 / Math.PI;
  const lat = originLatDeg - (z / PLANET_RADIUS) * 180 / Math.PI;
  return { lon: ((lon + 540) % 360) - 180, lat };
}

/** Orthographic sphere projection, used by the planet map. */
export function projectOrthographic(lonDeg: number, latDeg: number, centerLonDeg: number, centerLatDeg: number): { x: number; y: number; visible: boolean } {
  const lon = lonDeg * Math.PI / 180;
  const lat = latDeg * Math.PI / 180;
  const l0 = centerLonDeg * Math.PI / 180;
  const p0 = centerLatDeg * Math.PI / 180;
  const dLon = lon - l0;
  const cosLat = Math.cos(lat);
  const x = cosLat * Math.sin(dLon);
  const y = Math.cos(p0) * Math.sin(lat) - Math.sin(p0) * cosLat * Math.cos(dLon);
  const visible = Math.sin(p0) * Math.sin(lat) + Math.cos(p0) * cosLat * Math.cos(dLon) > 0;
  return { x, y, visible };
}

export function unprojectOrthographic(x: number, y: number, centerLonDeg: number, centerLatDeg: number): { lon: number; lat: number } | null {
  const rho = Math.sqrt(x * x + y * y);
  if (rho > 1) return null;
  const p0 = centerLatDeg * Math.PI / 180;
  const l0 = centerLonDeg * Math.PI / 180;
  if (rho < 1e-8) return { lon: centerLonDeg, lat: centerLatDeg };
  const c = Math.asin(rho);
  const sinC = Math.sin(c);
  const cosC = Math.cos(c);
  const lat = Math.asin(cosC * Math.sin(p0) + (y * sinC * Math.cos(p0)) / rho);
  const lon = l0 + Math.atan2(x * sinC, rho * cosC * Math.cos(p0) - y * sinC * Math.sin(p0));
  return { lon: lon * 180 / Math.PI, lat: lat * 180 / Math.PI };
}

export function formatMetres(m: number): string {
  const a = Math.abs(m);
  if (a >= 1e6) return `${(m / 1e6).toFixed(2)} Mm`;
  if (a >= 1e3) return `${(m / 1e3).toFixed(2)} km`;
  return `${m.toFixed(1)} m`;
}

export function formatLatLon(lat: number, lon: number): string {
  const ns = lat >= 0 ? 'N' : 'S';
  const ew = lon >= 0 ? 'E' : 'W';
  return `${Math.abs(lat).toFixed(3)}°${ns}, ${Math.abs(lon).toFixed(3)}°${ew}`;
}
